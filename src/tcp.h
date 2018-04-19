#pragma once

#include <stdbool.h>
#include <stdlib.h>

#define RWS 150
#define SWS 150
#define MAX_SEQ_NO 300
#define FRAME_SIZE 1000


/* 
 * Define TCP packet 
 */
typedef struct _tcp_packet {
    int32_t seq_num;
    int32_t ack_num;
    int32_t length;      /* length of the content */
    char *content;
} TCP_packet;

/*
 * Local buffer 
 */
typedef struct _Buffer_frame {
    char data[FRAME_SIZE];
    size_t length;
    size_t seq_num;
} Buffer_frame;
