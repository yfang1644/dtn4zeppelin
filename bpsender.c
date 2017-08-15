/*
 * ============================================================================
 *
 *       Filename:  bpsender.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/15/2017 02:44:35 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Fang Yuan (yfang@nju.edu.cn)
 *   Organization:  nju
 *
 * ============================================================================
 */

/*
    Modified from bpsendfile.c (ion-3.6.0)
    Original author: Scott Burleigh, Jet Propulsion Laboratory
 */

#include <bp.h>
#define  MAXLINE    1024

int run_bpsendfile(char *ownEid, char *destEid, int ttl, char *svcClass)
{

    return 0;
}

int  main(int argc, char **argv)
{
    char    *ownEid = NULL;
    char    *destEid = NULL;
    char    *fileName = NULL;
    char    *classOfService = NULL;
    int    ttl = 300;    /*  Time To Live in seconds */

    int  priority = 0;
    BpAncillaryData    ancillaryData = { 0, 0, 0 };
    BpCustodySwitch    custodySwitch = NoCustodyRequested;
    BpSAP   sap;
    Sdr     sdr;
    Object  txExtent;
    int     aduLength;
    Object  bundleZco;
    Object  newBundle;
    char buf[MAXLINE];

    if (argc > 1)
        ownEid = argv[1];
    if (argc > 2)
        destEid = argv[2];

    if (ownEid == NULL || destEid == NULL) {
        printf("Usage: %s <own endpoint ID> <dest. endpoint ID>",
               argv[0]);
        return 0;
    }

    if (classOfService == NULL) {
        priority = BP_STD_PRIORITY;
    } else {
        if (!bp_parse_class_of_service(classOfService, &ancillaryData,
                &custodySwitch, &priority)) {
            printf("Invalid class of service for bpsendfile.(%s)\n",
                    classOfService);
            return 0;
        }
    }

    if (bp_attach() < 0) {
        perror("Can't attach to BP.\n");
        return 0;
    }

    if (bp_open(ownEid, &sap) < 0) {
        printf("Can't open own endpoint. (%s)\n", ownEid);
        return 0;
    }

    sdr = bp_get_sdr();
    oK(sdr_begin_xn(sdr));
    if (sdr_heap_depleted(sdr)) {
        sdr_exit_xn(sdr);
        bp_close(sap);
        perror("Low on heap space, can't send data.\n");
        return 0;
    }

    txExtent = sdr_malloc(sdr, sizeof(buf));

    sdr_write(sdr, txExtent, buf, strlen(buf) - 1);

    bundleZco = ionCreateZco(ZcoSdrSource, txExtent, 0, sizeof(buf)-1,
            priority, ancillaryData.ordinal, ZcoOutbound, NULL);
    if (bundleZco == 0 || bundleZco == (Object) ERROR) {
        printf("bpsender can't create ZCO.\n");
    } else {
        if (bp_send(sap, destEid, NULL, ttl, priority, custodySwitch,
            0, 0, &ancillaryData, bundleZco, &newBundle) <= 0)
        {
            printf("bpsender  can't send data in bundle.\n");
        }
    }

    bp_close(sap);

    if (sdr_end_xn(sdr) < 0) {
        return -1;
    }

    bp_detach();
    return 0;
}
