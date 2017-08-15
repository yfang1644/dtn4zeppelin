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

typedef struct
{
    BpSAP    sap;
    int    running;
} BptestState;

BptestState *_bptestState(BptestState *newState)
{
    void    *value;
    BptestState    *state;

    if (newState) {           /*    Add task variable.    */
        value = (void *) (newState);
        state = (BptestState *) sm_TaskVar(&value);
    } else {              /*    Retrieve task variable.    */
        state = (BptestState *) sm_TaskVar(NULL);
    }

    return state;
}

int  main(int argc, char **argv)
{
    BptestState    state = { NULL, 1  };
    char    *ownEid = NULL;
    char    *destEid = NULL;
    char    *fileName = NULL;
    char    *classOfService = NULL;
    int    ttl = 300;    /*  Time To Live in seconds */

    int  priority = BP_STD_PRIORITY;
    BpCustodySwitch    custodySwitch = NoCustodyRequested;
    Sdr     sdr;
    Object  txExtent;
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

    if (bp_attach() < 0) {
        perror("Can't attach to BP.\n");
        return 0;
    }

    if (bp_open(ownEid, &state.sap) < 0) {
        printf("Can't open own endpoint. (%s)\n", ownEid);
        return 0;
    }

    oK(_bptestState(&state));

    sdr = bp_get_sdr();
    oK(sdr_begin_xn(sdr));
    if (sdr_heap_depleted(sdr)) {
        sdr_exit_xn(sdr);
        bp_close(state.sap);
        perror("Low on heap space, can't send data.\n");
        return 0;
    }

    while(state.running) {
        scanf("%s", buf);
        txExtent = sdr_malloc(sdr, strlen(buf));
        sdr_write(sdr, txExtent, buf, strlen(buf));

        bundleZco = ionCreateZco(ZcoSdrSource,
                                 txExtent,
                                 0,
                                 strlen(buf),
                                 priority,
                                 0,
                                 ZcoOutbound,
                                 NULL);
        if (bundleZco == 0 || bundleZco == (Object) ERROR) {
            printf("bpsender can't create ZCO.\n");
            break;
        }
        if (bp_send(state.sap, destEid, NULL, ttl, priority, custodySwitch,
                    0, 0, NULL, bundleZco, &newBundle) <= 0) {
                        printf("bpsender can't send data in bundle.\n");
        }
        break;
    }

    bp_close(state.sap);
    CHKZERO(sdr_begin_xn(sdr));

    if (sdr_end_xn(sdr) < 0) {
        return -1;
    }

    bp_detach();
    return 0;
}
