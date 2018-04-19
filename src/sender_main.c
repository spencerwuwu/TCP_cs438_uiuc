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

/*
 * Global variables
 */
Sender_info *sender;
Buffer_frame *buffer_frame;
size_t frame_num;

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

    setup_buff(argv[3], numBytes);
	setup_UDP(argv[1], udpPort);

    // Start thread for send and receive
	pthread_t send_tid;
	pthread_create(&send_tid, 0, reliable_send, (void*)0);

	pthread_t receive_tid;
	pthread_create(&receive_tid, 0, receive_reply, (void*)0);

    while (1) {
        sendto(socket_UDP, "gg\n", 3, 0,
                    (struct sockaddr*)&receiver_addr, sizeof(receiver_addr));
        sleep(5);
    }

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

void *reliable_send() {
}

void *receive_reply() {
}
