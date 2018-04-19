#include "tcp_sender.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

/*
 * Global variables
 */
extern Sender_info *sender;
extern Buffer_frame *buffer_frame;
extern size_t frame_num;


Sender_info *init_sender() {
    int i = 0;
    for (i = 0; i < SWS; i++) sender->present[i] = 0;
}

void setup_buff(char *filename, size_t bytes) {
    int file_fd = open(filename, O_RDONLY);
    frame_num = bytes / FRAME_SIZE;
    if (bytes % FRAME_SIZE) frame_num++;

    buffer_frame = calloc(frame_num, sizeof(Buffer_frame));
    size_t bytes_read = 0;
    for (size_t i = 0; i < frame_num; i++) {
        bytes_read = read_file_line(file_fd, buffer_frame[i].data, FRAME_SIZE);
        buffer_frame[i].length = bytes_read;
        buffer_frame[i].seq_num = i;
    }
}

size_t read_file_line(int sockfd, char *msg, size_t length) {
    size_t n = 0, rc;
    char c;
    char *ptr = msg;
    while (n < length) {
        if ((rc = read(sockfd, &c, 1)) == 1) {
            *ptr++ = c;
        } else if (rc == 0) {
            break;      // EOF, no data read
        } else {
            if (errno == EINTR) continue;
            else {
                perror("client read error");
                exit(1);
            }
        }
        n++;
    }
    return n;
}

char *build_msg_packet(TCP_packet *packet) {
    char *msg = calloc(packet->length + 12, sizeof(char)); 
    char *size = calloc(sizeof(char), 4);
    size = (char *)&packet->seq_num;
    msg[0] = size[0];
    msg[1] = size[1];
    msg[2] = size[2];
    msg[3] = size[3];

    size = (char *)&packet->ack_num;
    msg[4] = size[0];
    msg[5] = size[1];
    msg[6] = size[2];
    msg[7] = size[3];

    size = (char *)&packet->length;
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
