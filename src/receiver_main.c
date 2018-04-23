#include "tcp_receiver.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h> 
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
/*
 * Global variables
 */
Receiver_info *Receiver;

/* 
 * Static variables 
 */
static int socket_UDP;
static int file_fd;

/*
 * Function Declaration
 */
void setup_UDP(char *port);
void setup_filefd(char *filename);
void receive_packet();

int main(int argc, char** argv)
{
	unsigned short int udpPort;
	
	if(argc != 3)
	{
		fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
		exit(1);
	}
	
	udpPort = (unsigned short int)atoi(argv[1]);
	
    setup_UDP(argv[1]);

    setup_filefd(argv[2]);
    
    Receiver = init_receiver();

    receive_packet();


    close(socket_UDP);
}

void setup_UDP(char *port) {
    struct addrinfo hints, *servinfo;
    
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

    int rv, yes;
	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}

    if ((socket_UDP = socket(servinfo->ai_family, servinfo->ai_socktype,
                    servinfo->ai_protocol)) == -1) {
        perror(NULL);
        exit(1);
    }

    if (setsockopt(socket_UDP, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
        perror(NULL);
        exit(1);
    }

    if (bind(socket_UDP, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        close(socket_UDP);
        perror(NULL);
        exit(1);
    }

}

void setup_filefd(char *filename) {
    file_fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (file_fd < 0) {
        perror("open file error");
        exit(1);
    }
}

void receive_packet() {
    char from_addr[100];
    struct sockaddr_in sender_addr;
    socklen_t sender_addrLen;
    unsigned char recvBuf[1500];
    unsigned char msg[100];
    msg[0] = 'A';
    msg[1] = 'C';

    int bytesRecvd;
    while(1) {
        if ((bytesRecvd = recvfrom(socket_UDP, recvBuf, 1500 , 0, 
                        (struct sockaddr*)&sender_addr, &sender_addrLen)) == -1) {
            perror("connectivity listener: recvfrom failed");
            exit(1);
        }
        inet_ntop(AF_INET, &sender_addr.sin_addr, from_addr, 100);
        msg[2] = recvBuf[2];

        if (recvBuf[0] == 'S' && recvBuf[1] == 'E') {
            sendto(socket_UDP, msg, 3, 0, (struct sockaddr*)&sender_addr, sizeof(sender_addr));
            fprintf(stderr, "ACK %d %zu\n", recvBuf[2], bytesRecvd);
            handle_sender_msg(recvBuf, bytesRecvd, file_fd);
        }
    }
}

