#pragma once

#include "tcp.h"
#include <string.h>


/* Transition Diagram of 
 * Receiver TCP State
 */
enum Receiver_State {
    CLOSED,
    SYN_SENT,
    ESTABLISHED,
    CLOSE_WAIT,
    LAST_ACK,
};

/* Define parameters needed 
 * for receiver
 */
typedef struct _tcp_receiver {
    int status;
    /* structures for sliding windows */
    int NFE;    // Next Frame Expected
    int present[RWS];
    char buf[RWS][FRAME_SIZE];
    size_t buf_len[RWS];

} Receiver_info;

/* 
 * Initial set up for sender
 */
Receiver_info *init_receiver();

void handle_sender_msg(unsigned char *msg, size_t length, int filefd);
void recv_frame(char *data, size_t length, int seq_num, int filefd);
ssize_t write_to_filefd(int filefd, size_t length, char *data);
