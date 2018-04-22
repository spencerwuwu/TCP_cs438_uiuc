#include "tcp_sender.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/time.h>

/*
 * Global variables
 */
extern Sender_info *Sender;
extern Buffer_Frame *Buffer_frame;
extern size_t Frame_num;


Sender_info *init_sender() {
    Sender_info *sender = malloc(sizeof(Sender_info));
    int i = 0;
    for (i = 0; i < SWS; i++) {
        sender->present[i] = -1;
        sender->packet[i] = NULL;
    }
    sender->LAR = -1;
    sender->LFS = -1;
    sender->status = LISTEN;
    return sender;
}

void setup_buff(char *filename, size_t bytes) {
    int file_fd = open(filename, O_RDONLY);
    Frame_num = bytes / FRAME_SIZE;
    if (bytes % FRAME_SIZE) Frame_num++;

    Buffer_frame = calloc(Frame_num, sizeof(Buffer_frame));
    size_t bytes_read = 0;
    for (size_t i = 0; i < Frame_num; i++) {
        bytes_read = read_file_line(file_fd, Buffer_frame[i].data, FRAME_SIZE);
        Buffer_frame[i].length = bytes_read;
        Buffer_frame[i].seq_num = i % MAX_SEQ_NO;
        write(STDERR_FILENO, Buffer_frame[i].data, bytes_read);
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

char *build_msg_packet(Buffer_Frame frame) {
    char *msg = calloc(frame.length + SEND_HEADER, sizeof(char)); 
    msg[0] = 'S';
    msg[1] = 'E';
    msg[2] = frame.seq_num;

    char *size = (char *)&frame.length;
    msg[3] = size[0];
    msg[4] = size[1];
    msg[5] = size[2];
    msg[6] = size[3];


    int i = 0;
    for ( ; i < frame.length; i++) {
        msg[SEND_HEADER + i] = frame.data[i];
    }

    return msg;
}
