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
#include <fcntl.h>
#include <sys/stat.h>

/*
 * Global variables
 */
Sender_info *Sender;
Buffer_Frame *Buffer_frame;
size_t Frame_num;
int RTT = 100; // initial RTT to 100 ms
long int congestion_init = 100 * 20 * 1000; // Initial to be RTT/50

/* 
 * Static variables 
 */
static int socket_UDP;
static struct sockaddr_in receiver_addr;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static struct timespec congestion_sleep;

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

    int debug_fd = open("debug", O_RDWR);
	
	if(argc != 5) {
		fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
		exit(1);
	}
	udpPort = (unsigned short int)atoi(argv[2]);
	numBytes = atoll(argv[4]);

    // set up socket and wait for first ack to determine a rtt
	setup_UDP(argv[1], udpPort);

    setup_buff(argv[3], numBytes);
    Sender = init_sender();

    fprintf(stderr, "Finish init buff\n");
    // Finish initializing

    congestion_sleep.tv_sec = 0;
    congestion_sleep.tv_nsec = RTT * 20 * 1000;

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

}

/* Sending all packets */
void *reliable_send() {
    struct timeval current;
    struct timeval time_diff;

    struct timespec sleepFor;
    sleepFor.tv_sec = 0;
    sleepFor.tv_nsec = RTT * 500 * 1000; // RTT/2

    // Initializing state
    while (Sender->status == LISTEN) {
        send_msg("IN", 2);
        nanosleep(&sleepFor, 0);
    }

    while (Sender->LAR < (int)Frame_num) {
        int idx;
        for (int i = 0; i < SWS; i++) {
            pthread_mutex_lock(&mutex);

            if (Sender->LAR >= (int)Frame_num) {
                pthread_mutex_unlock(&mutex);
                break;
            }

            if (Sender->LAR >= 0) idx = (i + Sender->LAR) % SWS;
            else idx = i;

            if (Sender->present[idx] == -1) {
                // Initial
                if (idx >= Frame_num) {
                    pthread_mutex_unlock(&mutex);
                    Sender->present[idx] = 2;
                    continue;
                }
                Sender->present[idx] = 0;
                Sender->buff[idx] = &Buffer_frame[idx];
                send_msg(Sender->buff[idx]->packet, Sender->buff[idx]->packet_len);
                gettimeofday(&Sender->send_time[idx], 0);
                pthread_mutex_unlock(&mutex);
                nanosleep(&congestion_sleep, 0);

            } else if (Sender->present[idx] == -2) {
                // New update buff
                Sender->present[idx] = 0;
                send_msg(Sender->buff[idx]->packet, Sender->buff[idx]->packet_len);
                gettimeofday(&Sender->send_time[idx], 0);
                pthread_mutex_unlock(&mutex);
                nanosleep(&congestion_sleep, 0);

            } else if (Sender->present[idx] == 0) {
                // If not ack yet
                gettimeofday(&current, 0);
                timersub(&current, &Sender->send_time[idx], &time_diff);

                if (time_diff.tv_sec == 0 && time_diff.tv_usec / 1000 > RTT) {
                    Sender->re_send[idx]++;
                    Sender->buff[idx]->packet[5] = Sender->re_send[idx];
                    send_msg(Sender->buff[idx]->packet, Sender->buff[idx]->packet_len);
                    gettimeofday(&Sender->send_time[idx], 0);
                    RTT = RTT + 10;
                }
                pthread_mutex_unlock(&mutex);

            } else {
                // It is finished
                pthread_mutex_unlock(&mutex);
            }
        } // End of for loop of sliding window
    }

    sleepFor.tv_nsec = RTT * 50 * 1000; // RTT/20
    for (int k = 0; k < 5; k++) {
        if (Sender->status != ESTABLISHED) break;
        send_msg("EN", 2);
        nanosleep(&sleepFor, 0);
    }
    exit(0);
}

/* Upon receiving a message */
void *receive_reply() {
    struct sockaddr_in sender_addr;
    socklen_t sender_addrLen;
    unsigned char recvBuf[1400];
    int bytesRecvd;
    struct timeval current, time_diff;

    while (Sender->LAR < (int)Frame_num) {
        sender_addrLen = sizeof(sender_addr);
        if ((bytesRecvd = recvfrom(socket_UDP, recvBuf, 1400 , 0, 
                        (struct sockaddr*)&sender_addr, &sender_addrLen)) == -1) {
            perror("connectivity listener: recvfrom failed");
            exit(1);
        }
        if (recvBuf[0] == 'I' && recvBuf[1] == 'C' && recvBuf[2] == 'K') {  // ICK -> for initializing a connection
            if (Sender->status == LISTEN) Sender->status = ESTABLISHED;
        } else if (recvBuf[0] == 'A' && recvBuf[1] == 'C') {                // AC<seq_num>
            int seq_num = recvBuf[2];

            // Sender->LAR is initialized as -1
            int idx, i;
            int LAR = 0;
            if (Sender->LAR >= 0) LAR = Sender->LAR;
            if ((seq_num - (LAR % MAX_SEQ_NO) < SWS) || 
                    (seq_num + MAX_SEQ_NO - (LAR % MAX_SEQ_NO) < SWS)) {
                pthread_mutex_lock(&mutex);
                idx = (seq_num % SWS);
                
                if (!Sender->present[idx]) {
                    Sender->present[idx] = 1;
                    if (Sender->LAR < 0 && idx == 0) Sender->LAR = 0;

                    // Update RTT & congestion window
                    gettimeofday(&current, 0);
                    if (recvBuf[3] == Sender->re_send[idx]) {
                        timersub(&current, &Sender->send_time[idx], &time_diff);
                        if (time_diff.tv_sec == 0) 
                            RTT = calculate_new_rtt(RTT, time_diff.tv_usec / 1000);

                        // I restrict the time not to grow too big or small
                        if (congestion_sleep.tv_nsec > congestion_init / 100) 
                            congestion_sleep.tv_nsec = congestion_sleep.tv_nsec * 0.8;
                    } else {
                        if (congestion_sleep.tv_nsec < congestion_init * 100)
                            congestion_sleep.tv_nsec = congestion_sleep.tv_nsec / 0.8;
                    }
                }
                if (Sender->LAR < 0) {
                    pthread_mutex_unlock(&mutex);
                    break;
                }

                // Perform sliding window
                for (i = 0; i < RWS; i++) {
                    idx = (i + Sender->LAR) % RWS;
                    if (Sender->present[idx] != 1) break;
                    size_t next_target = i + Sender->LAR + SWS;
                    if (next_target >= Frame_num) {
                        Sender->present[idx] = 2;
                    } else {
                        Sender->buff[idx] = &Buffer_frame[next_target];
                        Sender->present[idx] = -2;
                        Sender->re_send[idx] = 0;
                    }
                }
                Sender->LAR += i;
                pthread_mutex_unlock(&mutex);
            }

        } else if (recvBuf[0] == 'K' && recvBuf[1] == 'K' && recvBuf[2] == 'K') {
            Sender->status = CLOSED;
            exit(0);
        }
    }
}

/* A wrapping function for sendto */
void send_msg(char *msg, size_t length) {
    sendto(socket_UDP, msg, length, 0, (struct sockaddr*)&receiver_addr, sizeof(receiver_addr));
}

