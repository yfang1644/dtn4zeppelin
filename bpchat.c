/*
 * bpchat.c
 * Andrew Jenkins <andrew.jenkins@colorado.edu>
 * Reads lines from stdin and sends those in bundles.
 * Receives bundles and writes them to stdout.
 */

#include <stdlib.h>
#include <stdio.h>
#include <bp.h>

static BpSAP                sap;
static Sdr                  sdr;
static pthread_mutex_t      sdrmutex = PTHREAD_MUTEX_INITIALIZER;
static BpCustodySwitch      custodySwitch = NoCustodyRequested;

const char usage[] =
"Usage: bpchat <source EID> <dest EID> [ct]\n\n"
"Reads lines from stdin and sends these lines in bundles.\n"
"Receives bundles and writes them to stdout.\n"
"If \"ct\" is specified, sent bundles have the custody transfer flag set\n";

static void *       sendLines(void *args)
{
    char *destEid = (char *)args;
    Object          bundleZco, bundlePayload;
    Object          newBundle;   /* We never use but bp_send requires it. */
    int             lineLength = 0;
    char            lineBuffer[1024];

    while(1) {
        /* Read a line from stdin */
        if(fgets(lineBuffer, sizeof(lineBuffer), stdin) == NULL) {
            fprintf(stderr, "EOF\n");
            bp_interrupt(sap);
            break;
        }

        lineLength = strlen(lineBuffer);

        /* Wrap the linebuffer in a bundle payload. */
        if(pthread_mutex_lock(&sdrmutex) != 0) {
            printf("Couldn't take sdr mutex.\n");
            break;
        }

        oK(sdr_begin_xn(sdr));
        bundlePayload = sdr_malloc(sdr, lineLength);
        if(bundlePayload) {
            sdr_write(sdr, bundlePayload, lineBuffer, lineLength);
        }

        if(sdr_end_xn(sdr) < 0) {
            pthread_mutex_unlock(&sdrmutex);
            bp_close(sap);
            printf("No space for bpchat payload.\n");
            break;
        }

        bundleZco = ionCreateZco(ZcoSdrSource, bundlePayload, 0,
            lineLength, BP_STD_PRIORITY, 0, ZcoOutbound, NULL);
        pthread_mutex_unlock(&sdrmutex);

        if(bundleZco == 0 || bundleZco == (Object) ERROR) {
            bp_close(sap);
            printf("bpchat can't create bundle ZCO.\n");
            break;
        }

        /* Send the bundle payload. */
        if(bp_send(sap, destEid, NULL, 86400, BP_STD_PRIORITY,
                custodySwitch, 0, 0, NULL, bundleZco,
                &newBundle) <= 0) {
            printf("bpchat can't send bundle.\n");
            break;
        }
    }
    return NULL;
}

static void *       recvBundles(void *args)
{
    BpDelivery      dlv;
    ZcoReader       reader;
    char            buffer[1024];
    int             bundleLenRemaining;
    int             rc;
    int             bytesToRead;

    while(1) {
        if(bp_receive(sap, &dlv, BP_BLOCKING) < 0) {
            printf("bpchat bundle reception failed.\n");
            break;
        }

        if(dlv.result == BpReceptionInterrupted || dlv.adu == 0) {
            bp_release_delivery(&dlv, 1);
            continue;
        }

        if(dlv.result == BpEndpointStopped) {
            bp_release_delivery(&dlv, 1);
            break;
        }

        if(pthread_mutex_lock(&sdrmutex) != 0) {
            printf("Couldn't take sdr mutex.\n");
            break;
        }

        oK(sdr_begin_xn(sdr));
        bundleLenRemaining = zco_source_data_length(sdr, dlv.adu);
        zco_start_receiving(dlv.adu, &reader);
        while(bundleLenRemaining > 0) {
            bytesToRead = MIN(bundleLenRemaining, sizeof(buffer)-1);
            rc = zco_receive_source(sdr, &reader, bytesToRead,
                    buffer);
            if(rc < 0) break;
            bundleLenRemaining -= rc;
            printf("%.*s", rc, buffer);
            fflush(stdout);
        }

        pthread_mutex_unlock(&sdrmutex);
        bp_release_delivery(&dlv, 1);

        if (sdr_end_xn(sdr) < 0) {
            break;
        }
    }
    return NULL;
}

int main(int argc, char **argv)
{
    pthread_t    sendLinesThread;
    pthread_t    recvBundlesThread;
    char    *ownEid  = (argc > 1 ? argv[1] : NULL);
    char    *destEid = (argc > 2 ? argv[2] : NULL);
    char      *ctArg = (argc > 3 ? argv[3] : NULL);

    if(argc < 2 || (argv[1][0] == '-')) {
        fprintf(stderr, usage);
        exit(1);
    }

    if(ctArg && strncmp(ctArg, "ct", 3) == 0) {
        custodySwitch = SourceCustodyRequired;
    }

    if(bp_attach() < 0) {
        printf("Can't bp_attach()\n");
        exit(1);
    }

    if(bp_open(ownEid, &sap) < 0) {
        printf("Can't open own endpoint (%s).\n", ownEid);
        exit(1);
    }

    sdr = bp_get_sdr();

    /* Start receiver thread and sender thread. */
    if(pthread_create(&sendLinesThread, NULL, sendLines, destEid) < 0) {
        printf("Can't make sendLines thread.\n");
        bp_interrupt(sap);
        exit(1);
    }

    if(pthread_create(&recvBundlesThread, NULL, recvBundles, NULL) < 0) {
        printf("Can't make recvBundles thread.\n");
        bp_interrupt(sap);
        exit(1);
    }

    pthread_join(sendLinesThread, NULL);
    pthread_join(recvBundlesThread, NULL);

    bp_close(sap);
    bp_detach();
    return 0;
}
