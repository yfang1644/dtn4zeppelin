/*
 * ============================================================================
 *
 *       Filename:  bpreceiver.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/15/2017 02:42:36 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Fang Yuan (yfang@nju.edu.cn)
 *   Organization:  nju
 *
 * ============================================================================
 */

/*
    Modified from bprecvfile.c (ion-3.6.0)
    Original author: Scott Burleigh, Jet Propulsion Laboratory
 */

#include <bp.h>

#define    BPRECVBUFSZ    (65536)

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

void handleQuit()
{
    BptestState    *state;

    isignal(SIGINT, handleQuit);

    state = _bptestState(NULL);
    bp_interrupt(state->sap);
    state->running = 0;
}

int receiveFile(Sdr sdr, BpDelivery *dlv)
{
    char    buffer[BPRECVBUFSZ];
    int        contentLength;
    int        remainingLength;
    ZcoReader    reader;
    int        recvLength;

    contentLength = zco_source_data_length(sdr, dlv->adu);

    zco_start_receiving(dlv->adu, &reader);
    remainingLength = contentLength;
    oK(sdr_begin_xn(sdr));
    while (remainingLength > 0)
    {
        recvLength = BPRECVBUFSZ;
        recvLength = MIN(BPRECVBUFSZ, remainingLength);

        if (zco_receive_source(sdr, &reader, recvLength, buffer) < 0) {
            perror("bprecvfile: can't receive bundle content.");
            oK(sdr_end_xn(sdr));
            return -1;
        }

        if (write(STDOUT_FILENO, buffer, recvLength) < 1) {
            perror("bprecvfile: can't write to test file.");
            oK(sdr_end_xn(sdr));
            return -1;
        }

        remainingLength -= recvLength;
    }

    if (sdr_end_xn(sdr) < 0) {
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    char  *ownEid;

    BptestState    state = { NULL, 1 };
    Sdr  sdr;
    BpDelivery    dlv;

	if (argc > 1) {
		ownEid = argv[1];
	} else {
        printf("Usage: %s <own endpoint ID>\n", argv[0]);
        return 0;
    }

    if (bp_attach() < 0) {
        perror("Can't attach to BP.\n");
        return -1;
    }

    if (bp_open(ownEid, &state.sap) < 0) {
        printf("Can't open own endpoint. (%s)\n", ownEid);
        return -1;
    }

    oK(_bptestState(&state));
    sdr = bp_get_sdr();
    isignal(SIGINT, handleQuit);

    while (state.running)
    {
        if (bp_receive(state.sap, &dlv, BP_BLOCKING) < 0) {
            printf("bprecvfile bundle reception failed.\n");
            state.running = 0;
            continue;
        }

        switch (dlv.result) {
        case BpEndpointStopped:
            state.running = 0;
            break;        /*    Out of switch.        */

        case BpPayloadPresent:
            if (receiveFile(sdr, &dlv) < 0) {
                printf("bprecvfile cannot continue.\n");
                state.running = 0;
            }

            break;
        default:
            break;        /*    Out of switch.        */
        }

        bp_release_delivery(&dlv, 1);
    }

    bp_close(state.sap);

    bp_detach();
    return 0;
}
