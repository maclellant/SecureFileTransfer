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

#define PACKET_LEN 128
#define SERVER_PORT 10050

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

    while(1)
    {
        if (recvfrom(sockfd, buffer, PACKET_LEN, 0, (struct sockaddr*)&client_addr, &slen)==-1)
		{
	    	fprintf(stderr, "Error: Could not receive from client\n");
	    	close(sockfd);
	    	exit(EXIT_FAILURE);
    	}

        printf("Received packet from %s:%d\nData: %s\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buffer);
        

        printf("Sending ACK to client...\n");

        snprintf(buffer, PACKET_LEN, "Acknoledged");
        if (sendto(sockfd, buffer, PACKET_LEN, 0, (struct sockaddr*) &client_addr, slen) == -1)
        {
        	perror("Error: could not send acknowledge to client\n");
        	close(sockfd);
        	exit(EXIT_FAILURE);
        }
    }
 
    close(sockfd);
    exit(EXIT_SUCCESS);
}