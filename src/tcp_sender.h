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
    int status;

    /* structures for sliding windows */
    int LAR;    // Last Ack Received
    int LFS;    // Last Frame Sent
    int present[SWS];
    Buffer_Frame *buff[SWS];
    struct timeval send_time[SWS];
    int re_send[SWS];
} Sender_info;


/* 
 * Initial set up for sender
 */
Sender_info *init_sender();
void setup_buff(char *filename, size_t bytes);
size_t read_file_line(int sockfd, char *msg, size_t length);

/* Generate the packet with the following format:
 *      0 - 1   "SE" (2 byte)
 *      2       seq_num (1 byte)
 *      3       length / 256 
 *      4       length % 256 (2 bytes)
 *      5 ~     content
 */
unsigned char *build_msg_packet(Buffer_Frame frame);


/* Calculate the new RTT time */
int calculate_new_rtt(int RTT, int time);


