#pragma once

#include <stdbool.h>
#include <stdlib.h>

#define RWS 120
#define SWS 120
#define MAX_SEQ_NO 240
#define FRAME_SIZE 1000

#define SEND_HEADER 6
#define RTT_ALPHA 0.6


/* 
 * Define TCP packet 
 */
typedef struct _tcp_packet {
    int32_t seq_num;
    int32_t ack_num;
    int32_t length;      /* length of the content */
    char *content;
    struct timeval sent_time;
} TCP_packet;

/* 
 * Define TCP ack packet
 */
typedef struct _tcp_ack {
    int32_t ack_num;
} TCP_ack;

/*
 * Sender Local buffer 
 */
typedef struct _Buffer_frame {
    //unsigned char data[FRAME_SIZE];
    unsigned char *data;
    size_t length;
    int seq_num;

    /* packet */
    unsigned char *packet;
    size_t packet_len;
} Buffer_Frame;
