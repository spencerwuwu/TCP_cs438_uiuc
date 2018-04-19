#pragma once

#include "tcp.h"

/* Transition Diagram of 
 * Sender / RECEIVER TCP State
 */
enum Sender_State {
    CLOSED,
    LISTEN,
    SYN_RCVD,
    ESTABLISHED,
    FIN_WAIT1,
    FIN_WAIT2,
    CLOSING,
    TIME_WAIT
};

/* Define parameters needed 
 * for sender
 */
typedef struct _tcp_sender {
    int LAR;    // Last Ack Received
    int LFS;    // Last Frame Sent
    int present[SWS];
    char *packet[SWS];
    int status;
} Sender_info;


/* 
 * Initial set up for sender
 */
Sender_info *init_sender();
void setup_buff(char *filename, size_t bytes);
size_t read_file_line(int sockfd, char *msg, size_t length);

/* Generate the packet with the following format:
 *      0 - 3   seq_num (1 byte)
 *      4 - 7   ack_num (1 byte)
 *      8 - 11  length  (4 bytes)
 *      12 ~     content
 */
char *build_msg_packet(TCP_packet *packet);


/* Calculate the new RTT time */
void calculate_new_rtt(int time);


