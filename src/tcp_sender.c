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
#include <sys/io.h>
#include <sys/mman.h>

/*
 * Global variables
 */
extern Sender_info *Sender;
extern Buffer_Frame *Buffer_frame;
extern size_t Frame_num;

unsigned char *data_pool;

Sender_info *init_sender() {
    Sender_info *sender = malloc(sizeof(Sender_info));
    int i = 0;
    for (i = 0; i < SWS; i++) {
        sender->present[i] = -1;
        sender->buff[i] = NULL;
        sender->re_send[i] = 0;
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

    data_pool = (char *) mmap (0, data_size, PROT_READ, MAP_PRIVATE, file_fd, 0);

    Buffer_frame = calloc(Frame_num, sizeof(Buffer_Frame));
    size_t bytes_read = 0;
    size_t bytes_to_read = 0;
    for (size_t i = 0; i < Frame_num; i++) {
        if ((i == Frame_num - 1) && (data_size % FRAME_SIZE)) bytes_to_read = data_size % FRAME_SIZE;
        else bytes_to_read = FRAME_SIZE;
        Buffer_frame[i].packet = calloc(bytes_to_read + SEND_HEADER, sizeof(unsigned char));
        //bytes_read = read_file_line(file_fd, Buffer_frame[i].packet + SEND_HEADER, bytes_to_read);

        /*
        Buffer_frame[i].length = bytes_read;
        Buffer_frame[i].seq_num = i % MAX_SEQ_NO;
        Buffer_frame[i].packet_len = bytes_read + SEND_HEADER;
        Buffer_frame[i].packet[0] = 'S';
        Buffer_frame[i].packet[1] = 'E';
        Buffer_frame[i].packet[2] = Buffer_frame[i].seq_num;

        Buffer_frame[i].packet[3] = bytes_read / 256;
        Buffer_frame[i].packet[4] = bytes_read % 256;
        Buffer_frame[i].packet[5] = 0;
           */
           //bytes_read = read_file_line(file_fd, Buffer_frame[i].data, FRAME_SIZE);
        bytes_read = bytes_to_read;
        Buffer_frame[i].length = bytes_read;
        Buffer_frame[i].data = data_pool + i * FRAME_SIZE;
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
    msg[2] = frame.seq_num / 256;

    msg[3] = frame.length / 256;
    msg[4] = frame.length % 256;
    msg[5] = 0;
    msg[6] = frame.seq_num % 256;


    int i = 0;
    for ( ; i < frame.length; i++) {
        msg[SEND_HEADER + i] = frame.data[i];
    }

    return msg;
}

int calculate_new_rtt(int RTT, int time) {
    if (time < 0) return RTT;
    float result = RTT * RTT_ALPHA + (1 - RTT_ALPHA) * time;
    //fprintf(stderr, "RTT: %d %d %f\n", RTT, time, result);
    return (int)result;
}
