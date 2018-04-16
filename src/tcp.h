#pragma once

#include <stdbool.h>
#include <stdlib.h>

#define RWS 8
#define SWS 8
#define MAX_SEQ_NO 16
#define FRAME_SIZE 1000


/* Transition Diagram of 
 * Sender TCP State
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

/* Transition Diagram of 
 * Receiver TCP State
 */
enum Sender_State {
    CLOSED,
    SYN_SENT,
    ESTABLISHED,
    CLOSE_WAIT,
    LAST_ACK,
};

/* Define TCP packet 
 *
 */
struct _tcp_packet {
    int32_t seq_num;
    int32_t ack_num;
    int32_t length;      /* length of the content */
    char *content;
} TCP_packet;
