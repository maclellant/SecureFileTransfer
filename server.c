/// @file server.c
///
/// Creates a server process that listens on a certain port for a client to connect
/// and begin transmitting a file.
///
/// @author Tavis Maclellan

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h> 
#include <string.h>
#include <stdint.h>
#include <iostream>

#define PACKET_LEN 128
#define SERVER_PORT 10050

#define ACK 0
#define NAK 1
#define GET 2
#define PUT 3
#define DEL 4
#define TRN 5

int main(int argc, char** argv)
{
	if (argc > 1)
	{
		perror("Error: No command line arguments expected\n");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
    int sockfd = -1; 

    socklen_t slen = sizeof(client_addr);
    char buffer[PACKET_LEN];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
    	perror("Error: Could not create socket\n");
    	exit(EXIT_FAILURE);
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1)
    {
    	fprintf(stderr, "Error: Could not bind to port\n");
    	close(sockfd);
    	exit(EXIT_FAILURE);
    }

    printf("Successfully bound server to port %d and listening for clients...\n\n", SERVER_PORT);
	
	bool wait_put = true;
	FILE *outfile;
	bool send_ack = false;
	
    while(1)
    {
		uint8_t cur_seq = 0;
		uint8_t exp_seq = 0;
	
        if (recvfrom(sockfd, buffer, PACKET_LEN, 0, (struct sockaddr*)&client_addr, &slen)==-1)
		{
	    	fprintf(stderr, "Error: Could not receive from client\n");
	    	close(sockfd);
	    	exit(EXIT_FAILURE);
    	}
		
		//Getting packet sections
		uint8_t * packet_type = (uint8_t*) buffer + 0;
		uint8_t * seq_num = (uint8_t*) buffer + 1;
		uint16_t * checksum = (uint16_t*) buffer + 2;
		uint16_t * data_size = (uint16_t*) buffer + 4;
		char* data = buffer + 6;
		cur_seq = *seq_num;
		
		// Insert checksum here
		
		
		
		switch(packet_type) {
			case ACK:
				printf("Why are you receiving an ACK?");
			break;
			case NAK:
				printf("Why are you receiving a NAK?");
			break;
			case GET:
				printf("Why are you receiving a GET?");
			break;
			case PUT:
				if(wait_put) {
					outfile = fopen(data, "wb");
					wait_put = false;
					send_ack = true;
				}
				else {
					printf("Another transfer already in progress.");
					send_ack = false;
				}
			break;
			case DEL:
				printf("Why are you receiving a DEL?");
			break;
			case TRN:
				if(!wait_put && exp_seq == *seq_num) {
					if(data_size > 0) {
						fwrite(data, 1, data_size, outfile);
					}
					else {
						fclose(outfile);
						wait_put = true;
					}
					send_ack = true;
				}
				else {
					printf("Transfer has not yet been initialized.");
					send_ack = false;
				}
			break;
			default:
				printf("Packet type not supported");
			break;
		}
		
		
        printf("Received packet from %s:%d\nData: %s\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);
        
		if(send_ack) {
			printf("Sending ACK to client...\n");

			snprintf(buffer, PACKET_LEN, "Acknowledged");
			bzero(buffer, 128);
			packet_type = 0;
			seq_num = cur_seq;
		
			if (sendto(sockfd, buffer, PACKET_LEN, 0, (struct sockaddr*) &client_addr, slen) == -1)
			{
				perror("Error: could not send acknowledge to client\n");
				close(sockfd);
				exit(EXIT_FAILURE);
			}
			//Updating the expected sequence number.
			exp_seq = (++exp_seq) % 2;
		}
    }
 
    close(sockfd);
    exit(EXIT_SUCCESS);
}