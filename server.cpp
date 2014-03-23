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
#include <numeric>

#define PACKET_SIZE 128
#define SERVER_PORT 10050

#define ACK 0
#define NAK 1
#define GET 2
#define PUT 3
#define DEL 4
#define TRN 5

/* Prototypes */
int checksum(char *, size_t);

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
    char buffer [PACKET_SIZE];

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
	int exp_seq = 0;
    int cur_seq = 0;
    
    while(1)
    {
        if (wait_put)
            std::cout << "Waiting for file control message" << std::endl;
        else
            std::cout << "Waiting for data packet: " << exp_seq << std::endl;
	
        if (recvfrom(sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr*)&client_addr, &slen)==-1)
		{
	    	fprintf(stderr, "Error: Could not receive from client\n");
	    	close(sockfd);
            exit(EXIT_FAILURE);
        }
        
        //Getting packet sections
        uint8_t * packet_type = (uint8_t*) (buffer + 0);
        uint8_t * seq_num = (uint8_t*) (buffer + 1);
        uint16_t * chk = (uint16_t*) (buffer + 2);
        uint16_t * data_size = (uint16_t*) (buffer + 4);
        char* data = (char*) (buffer + 6);
        cur_seq = (int) *seq_num;

        char terminated_data[PACKET_SIZE-6+1];
        memset(terminated_data,'\0',PACKET_SIZE-6+1);
        memcpy(terminated_data, data, *data_size);
        
        //std::cout << "Here is a breakpoint" << std::endl;
        
        switch(*packet_type) {
            case ACK:
                printf("Why are you receiving an ACK?\n");
            break;
            case NAK:
                printf("Why are you receiving a NAK?\n");
            break;
            case GET:
                printf("Why are you receiving a GET?\n");
            break;
            case PUT:
                if(wait_put) {
                    outfile = fopen(terminated_data, "wb");
                    wait_put = false;
                    send_ack = true;
                    exp_seq = 0;
                }
                else {
                    printf("Another transfer already in progress.\n");
                    send_ack = false;
                }
            break;
            case DEL:
                printf("Why are you receiving a DEL?\n");
            break;
            case TRN:
                if(!wait_put) {
                    if(exp_seq == cur_seq) {
                        int recv_chk = checksum(data, *data_size);
                        if (*chk == recv_chk) {
                            if(*data_size > 0) {
                            fwrite(data, 1, *data_size, outfile);
                                //Updating the expected sequence number.
                                exp_seq = (exp_seq + 1) % 2;
                            }
                            else {
                                fclose(outfile);
                                wait_put = true;
                                exp_seq = 0;
                            }
	                        send_ack = true;
                        }
                        else {
                            std::cout << "Error: Incorrect checksum value." << std::endl;
                            std::cout << "Expected: " << *chk << std::endl;
                            std::cout << "Received: " << recv_chk << std::endl;
                            send_ack = false;
                        }
                    }
                    else {
                        std::cout << "Error: Unexpected sequence number." << std::endl;
                        std::cout << "Expected: " << exp_seq << std::endl;
                        std::cout << "Received: " << cur_seq << std::endl;
                        send_ack = false;
                    }
                }
                else {
                    printf("Transfer has not yet been initialized.\n");
                    send_ack = false;
                }
            break;
            default:
                printf("Packet type not supported.\n");
            break;
        }



		if (terminated_data != NULL)
            printf("Received packet from %s:%d\nChecksum: %d\nData: %s\n",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), *chk, terminated_data);
        else
            printf("Received empty packet from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
		if(send_ack) {
			printf("Sending ACK to client...\n");
			bzero(buffer, 128);

			*packet_type = ACK;
			*seq_num = cur_seq;
            *data_size = 0;
		
			if (sendto(sockfd, buffer, PACKET_SIZE, 0, (struct sockaddr*) &client_addr, slen) == -1)
			{
				perror("Error: could not send acknowledge to client\n");
				close(sockfd);
				exit(EXIT_FAILURE);
			}
		}
    }
 
    close(sockfd);
    exit(EXIT_SUCCESS);
}

int checksum(char *msg, size_t len) {
	return int(std::accumulate(msg, msg + len, (unsigned char) 0));
}
