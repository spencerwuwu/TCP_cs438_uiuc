#include "tcp_sender.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h> 
#include <netdb.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

/*
 * Global variables
 */
Sender_info *Sender;
Buffer_Frame *Buffer_frame;
size_t Frame_num;
long int RTT = 300; // initial RTT to 300 ms

/* 
 * Static variables 
 */
static int socket_UDP;
static struct sockaddr_in receiver_addr;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Function Declaration
 */

void setup_UDP(char *hostname, unsigned short int port);
void send_msg(char *msg, size_t length);
void *reliable_send();
void *receive_reply();

int main(int argc, char** argv) {
	unsigned short int udpPort;
	size_t numBytes;
	
	if(argc != 5) {
		fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
		exit(1);
	}
	udpPort = (unsigned short int)atoi(argv[2]);
	numBytes = atoll(argv[4]);

    // set up socket and wait for first ack to determine a rtt
	setup_UDP(argv[1], udpPort);

    /*
    while (1) {
        sendto(socket_UDP, "gg\n", 3, 0,
                    (struct sockaddr*)&receiver_addr, sizeof(receiver_addr));
        nanosleep(&sleepFor, 0);
    }
    */
    setup_buff(argv[3], numBytes);
    Sender = init_sender();


    // Start thread for send and receive
	pthread_t send_tid;
	pthread_create(&send_tid, 0, reliable_send, (void*)0);

	pthread_t receive_tid;
	pthread_create(&receive_tid, 0, receive_reply, (void*)0);

    pthread_join(send_tid, NULL);
    pthread_join(receive_tid, NULL);

    return 0;
} 

void setup_UDP(char *hostname, unsigned short int port) {
	memset(&receiver_addr, 0, sizeof(receiver_addr));
    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(port);
	inet_pton(AF_INET, hostname, &receiver_addr.sin_addr);

	if((socket_UDP = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}


    /*
    struct timeval current;

    struct timespec sleepFor;
    sleepFor.tv_sec = 0;
    sleepFor.tv_nsec = 30 * 1000 * 1000; // 30 ms
    
    int seq_num = 0;
    char *msg = calloc(10, sizeof(char));
    while (1) {
        if (sender->status != LISTEN) {
            break;
        }
        sprintf(msg, "ini_%d.", seq_num);
        sendto(socket_UDP, msg, 10, 0,
                    (struct sockaddr*)&receiver_addr, sizeof(receiver_addr));
        nanosleep(&sleepFor, 0);
    }
    free(msg);
    */
}

void *reliable_send() {
    struct timeval current;

    struct timespec sleepFor;
    sleepFor.tv_sec = 0;
    sleepFor.tv_nsec = RTT * 1000 * 1000; // 300 ms

    while (Sender->status == LISTEN) {
        send_msg("IN", 2);
        nanosleep(&sleepFor, 0);
    }

    while (Sender->LAR < (int)Frame_num) {
        int idx;
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < SWS; i++) {
            if (Sender->LAR >= 0) idx = (i + Sender->LAR) % SWS;
            else idx = i;
            if (Sender->present[idx] == -1) {
                // Initial
                if (idx >= Frame_num) {
                    Sender->present[idx] = 2;
                    continue;
                }
                Sender->present[i] = 0;
                Sender->buff[i] = &Buffer_frame[i];
                send_msg(Sender->buff[i]->packet, Sender->buff[i]->packet_len);
                gettimeofday(&Sender->send_time[i], 0);
                fprintf(stderr, "on %zu %zu %d\n", i, Sender->buff[i]->packet_len, Sender->buff[i]->packet[2]);
            } else if (Sender->present[idx] == -2) {
                // New update buff
                fprintf(stderr, "on %zu\n", idx);
                Sender->present[idx] = 0;
                send_msg(Sender->buff[idx]->packet, Sender->buff[idx]->packet_len);
                gettimeofday(&Sender->send_time[idx], 0);

            } else if (Sender->present[idx] == 0) {
                // If not ack yet
                gettimeofday(&current, 0);
                if (current.tv_usec - Sender->send_time[idx].tv_usec > RTT) {
                    fprintf(stderr, "re %zu\n", idx);
                    send_msg(Sender->buff[idx]->packet, Sender->buff[idx]->packet_len);
                    gettimeofday(&Sender->send_time[idx], 0);
                    //RTT = RTT + 2;
                }
            } else if (Sender->present[idx] == 2) {
                // It is finished
            }
        } // End of for loop of sliding window
        pthread_mutex_unlock(&mutex);
    }

    for (int k = 0; k < 10; k++) {
        if (Sender->status != ESTABLISHED) break;
        send_msg("EN", 2);
        nanosleep(&sleepFor, 0);
    }
}

void *receive_reply() {
    struct sockaddr_in sender_addr;
    socklen_t sender_addrLen;
    unsigned char recvBuf[1400];
    int bytesRecvd;

    while (Sender->LAR < (int)Frame_num) {
        sender_addrLen = sizeof(sender_addr);
        if ((bytesRecvd = recvfrom(socket_UDP, recvBuf, 1400 , 0, 
                        (struct sockaddr*)&sender_addr, &sender_addrLen)) == -1) {
            perror("connectivity listener: recvfrom failed");
            exit(1);
        }
        if (recvBuf[0] == 'I' && recvBuf[1] == 'C' && recvBuf[2] == 'K') {
            if (Sender->status == LISTEN) Sender->status = ESTABLISHED;
        } else if (recvBuf[0] == 'A' && recvBuf[1] == 'C') {
            int seq_num = recvBuf[2];
            fprintf(stderr, "AC %d\n", seq_num);

            int idx, i;
            pthread_mutex_lock(&mutex);
            idx = (seq_num % RWS);
            if (!Sender->present[idx]) {
                Sender->present[idx] = 1;
                if (Sender->LAR < 0 && idx == 0) Sender->LAR = 0;
            }

            for (i = 0; i < RWS; i++) {
                idx = (i + Sender->LAR) % RWS;
                if (Sender->present[idx] != 1) break;
                size_t next_target = i + Sender->LAR + SWS;
                if (next_target > Frame_num) {
                    Sender->present[idx] = 2;
                } else {
                    Sender->buff[idx] = &Buffer_frame[next_target];
                    Sender->present[idx] = -2;
                    //fprintf(stderr, "next %zu\n", next_target);
                }
            }
            Sender->LAR += i;
            fprintf(stderr, "LAR %d %d\n", Sender->LAR, Frame_num);
            pthread_mutex_unlock(&mutex);

        } else if (recvBuf[0] == 'K' && recvBuf[1] == 'K' && recvBuf[2] == 'K') {
            Sender->status = CLOSED;
        }
    }
}

void send_msg(char *msg, size_t length) {
    sendto(socket_UDP, msg, length, 0, (struct sockaddr*)&receiver_addr, sizeof(receiver_addr));
}

