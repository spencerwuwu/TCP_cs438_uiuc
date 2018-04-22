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
long int RTT = 30 * 1000; // initial RTT to 30 ms with us

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


    // Start thread for send and receive
	pthread_t send_tid;
	pthread_create(&send_tid, 0, reliable_send, (void*)0);

	pthread_t receive_tid;
	pthread_create(&receive_tid, 0, receive_reply, (void*)0);


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

    while (Sender->LAR < Frame_num) {
        for (int i = 0; i < SWS; i++) {
            size_t target = Sender->LAR + i + 1;
            pthread_mutex_lock(&mutex);
            if (Sender->present[i] == -1) {
                // Initial frame
                if (target > Frame_num) continue;
                send_msg(Sender->packet[i], Sender->packet_len[i]);
                gettimeofday(&Sender->send_time[i], 0);
                sleep(0.5);

            } else if (Sender->present[i] == 0) {
                // See if we have to re-send it
                gettimeofday(&current, 0);
                if (current.tv_usec - Sender->send_time[i].tv_usec > RTT) {
                    send_msg(Sender->packet[i], Sender->packet_len[i]);
                    gettimeofday(&Sender->send_time[i], 0);
                } else {
                    continue;
                }
            }
            pthread_mutex_unlock(&mutex);
        } // End of for loop of sliding window
    }
}

void *receive_reply() {
    struct sockaddr_in sender_addr;
    socklen_t sender_addrLen;
    char recvBuf[1400];
    int bytesRecvd;

    while (1) {
        sender_addrLen = sizeof(sender_addr);
        if ((bytesRecvd = recvfrom(socket_UDP, recvBuf, 1400 , 0, 
                        (struct sockaddr*)&sender_addr, &sender_addrLen)) == -1) {
            perror("connectivity listener: recvfrom failed");
            exit(1);
        }
        if (recvBuf[0] == 'A' && recvBuf[1] == 'C') {
            int index = recvBuf[2];
            pthread_mutex_lock(&mutex);
            pthread_mutex_unlock(&mutex);
        }

    }
}

void send_msg(char *msg, size_t length) {
    write(STDERR_FILENO, msg, 2);
    fprintf(stderr, "%d\n", msg[2]);
    sendto(socket_UDP, msg, length, 0, (struct sockaddr*)&receiver_addr, sizeof(receiver_addr));
}

