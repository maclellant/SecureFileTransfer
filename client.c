/// @file client.c
///
/// Creates a client process that connects to a server and transmits a file
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
#include <errno.h>

#define PACKET_LEN 128
#define SERVER_PORT 10050
#define CLIENT_PORT 10051

#define TIMEOUT_SEC 0
#define TIMEOUT_USEC 10000
 
void err(char *s)
{
    perror(s);
    exit(1);
}
 
int main(int argc, char** argv)
{
    if(argc != 2)
    {
        fprintf(stderr, "Usage : %s <Server-IP>\n",argv[0]);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    int sockfd = -1;

    socklen_t slen = sizeof(server_addr);
    char buffer[PACKET_LEN]; 
 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
        perror("Error: could not create socket\n");
        exit(EXIT_FAILURE);
    }

    struct timeval tv;
    tv.tv_sec = TIMEOUT_SEC; 
    tv.tv_usec = TIMEOUT_USEC; 

    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));

    bzero(&client_addr, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(CLIENT_PORT);
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr*) &client_addr, sizeof(client_addr)) == -1)
    {
        fprintf(stderr, "Error: Cound not bind client process to port %d\n", CLIENT_PORT);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
 
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_aton(argv[1], &server_addr.sin_addr)==0)
    {
        fprintf(stderr, "Error: Given IP address not valid: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    printf("Attempting to talk with server at %s:%d...\n\n", argv[1], SERVER_PORT);
 
    while(1)
    {
        printf("Enter data to send(Type exit and press enter to exit) : ");
        scanf("%[^\n]",buffer);
        getchar();
        if(strcmp(buffer,"exit") == 0)
        {
            close(sockfd);
            exit(EXIT_SUCCESS);
        }
 
        if (sendto(sockfd, buffer, PACKET_LEN, 0, (struct sockaddr*)&server_addr, slen) == -1)
        {
            fprintf(stderr, "Error: Could not send message to the server\n");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        printf("Sending data to server at %s:%d...\n", argv[1], SERVER_PORT);

        if (recv(sockfd, buffer, PACKET_LEN, 0) == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                fprintf(stderr, "Error: timed out on server acknoledgement\n");
                errno = 0;
            }
            else
            {
                fprintf(stderr, "Error: could not receive message from server\n");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
        }        
        else
        {
            printf("Received Acknoledgement: %s\n", buffer);
        }
    }
 
    close(sockfd);
    exit(EXIT_SUCCESS);
}
