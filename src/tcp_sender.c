#include "tcp_sender.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
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
        sender->buff[i] = NULL;
    }
    sender->LAR = -1;
    sender->LFS = -1;
    sender->status = LISTEN;
    return sender;
}

void setup_buff(char *filename, size_t bytes) {
    int file_fd = open(filename, O_RDONLY);
    if (file_fd < 0) {
        perror("open");
        exit(1);
    }
    struct stat local_stat;
    if (fstat(file_fd, &local_stat) < 0) {
        close(file_fd);
        return;
    }
    size_t data_size = bytes < local_stat.st_size ? bytes : local_stat.st_size;
    //size_t data_size = local_stat.st_size;

    Frame_num = data_size / FRAME_SIZE;
    if (data_size % FRAME_SIZE) Frame_num++;

    Buffer_frame = calloc(Frame_num, sizeof(Buffer_Frame));
    size_t bytes_read = 0;
    for (size_t i = 0; i < Frame_num; i++) {
        bytes_read = read_file_line(file_fd, Buffer_frame[i].data, FRAME_SIZE);
        Buffer_frame[i].length = bytes_read;
        Buffer_frame[i].seq_num = i % MAX_SEQ_NO;
        Buffer_frame[i].packet = build_msg_packet(Buffer_frame[i]);
        Buffer_frame[i].packet_len = bytes_read + SEND_HEADER;
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

unsigned char *build_msg_packet(Buffer_Frame frame) {
    unsigned char *msg = malloc(frame.length + SEND_HEADER); 
    msg[0] = 'S';
    msg[1] = 'E';
    msg[2] = frame.seq_num;

    msg[3] = frame.length / 256;
    msg[4] = frame.length % 256;


    int i = 0;
    for ( ; i < frame.length; i++) {
        msg[SEND_HEADER + i] = frame.data[i];
    }

    return msg;
}
