#include "tcp_receiver.h"

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
extern Receiver_info *Receiver;



Receiver_info *init_receiver() {
    Receiver_info *receiver = malloc(sizeof(Receiver_info));
    receiver->NFE = 0;
    receiver->status = CLOSED;
    for (int i = 0; i < RWS; i++) receiver->present[i] = 0;
    return receiver;
}


void handle_sender_msg(unsigned char *msg, size_t length, int filefd) {
    int seq_num = msg[2] * 256 + msg[6];
    size_t size = msg[3] * 256 + msg[4];

    //fprintf(stderr, "size %zu\n", size);
    recv_frame(msg + SEND_HEADER, size, seq_num, filefd);
}

void recv_frame(char *data, size_t length, int seq_num, int filefd) {
    int idx, i;
    if ((seq_num - (Receiver->NFE % MAX_SEQ_NO) < RWS) || 
            (seq_num + MAX_SEQ_NO - (Receiver->NFE % MAX_SEQ_NO) < RWS)) {
        idx = (seq_num % RWS);

        if (!Receiver->present[idx]) {
            Receiver->present[idx] = 1;
            memcpy(Receiver->buf[idx], data, length);
            Receiver->buf_len[idx] = length;
        }
        for (i = 0; i < RWS; i++) {
            idx = (i + Receiver->NFE) % RWS;

            if (!Receiver->present[idx]) break;
            write_to_filefd(filefd, Receiver->buf_len[idx], Receiver->buf[idx]);
            Receiver->present[idx] = 0;
        }
        Receiver->NFE += i;
        //fprintf(stderr, "NFE %d %d\n", seq_num, Receiver->NFE);
    }
}

ssize_t write_to_filefd(int filefd, size_t length, char *data) {
    ssize_t bytes = 0;
    int rc = 0;
    while (bytes < (ssize_t)length) {
        rc = write(filefd, data + bytes, length - bytes);
        //write(STDERR_FILENO, data + bytes, length - bytes);
        if (rc == 0) return 0;
        else if (rc == -1) {
            if (errno == EINTR) continue;
            else return -1;
        }
        bytes += rc;
    }
    return bytes;
}
