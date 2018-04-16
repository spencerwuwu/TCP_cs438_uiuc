#include "tcp_sender.h"

#include <stdio.h>
#include <string.h>

extern Sender_info *sender;

Sender_info *init_sender() {
}

char *build_msg_packet(TCP_packet *packet) {
    char *msg = calloc(packet->length + 12, sizeof(char)); 
    char size[4] = (char *)&packet->seq_num;
    msg[0] = size[0];
    msg[1] = size[1];
    msg[2] = size[2];
    msg[3] = size[3];

    size[4] = (char *)&packet->ack_num;
    msg[4] = size[0];
    msg[5] = size[1];
    msg[6] = size[2];
    msg[7] = size[3];

    size[4] = (char *)&packet->length;
    msg[8] = size[0];
    msg[9] = size[1];
    msg[10] = size[2];
    msg[11] = size[3];

    int i = 0;
    for ( ; i < packet->length; i++) {
        msg[12 + i] = packet->content[i];
    }

    return msg;
}
