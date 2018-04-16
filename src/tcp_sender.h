#pragma once

#include "tcp.h"

/* Define parameters needed 
 * for sender
 */
struct _tcp_sender {
    int LAR;
    int LFS;
    int NFE;
    int LFA;
    int present[RWS];
} Sender_info;


Sender_info *init_sender();

/* Generate the packet with the following format:
 *      0 - 3   seq_num (1 byte)
 *      4 - 7   ack_num (1 byte)
 *      8 - 11  length  (4 bytes)
 *      12 ~     content
 */
char *build_msg_packet(TCP_packet *packet);


/* Calculate the new RTT time */
void calculate_new_rtt(int time);
