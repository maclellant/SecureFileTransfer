#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <algorithm>
#include <stdint.h>
#include <errno.h>
#include <iostream>
#include <numeric>

#define TIMEOUT_SEC 0
#define TIMEOUT_USEC 10000

#define TIMEOUT_PACKET_COUNT 15

#define SEQUENCE_COUNT 2
#define PACKET_SIZE 128

#define ACK 0
#define NAK 1
#define GET 2
#define PUT 3
#define DEL 4
#define TRN 5

/* Prototypes */
void error(const char *);
int checksum(char *, size_t);
void makepacket(uint8_t, uint8_t, char*, uint, char[PACKET_SIZE]);
bool gremlin(char *, int, int);
void PUT_func(char *, int, int, int, struct sockaddr_in);

int main(int argc, char *argv[])
{
	if (argc != 8) {
		std::cout << "Usage: " << argv[0] << " ";
		std::cout << "<client-port> <server-IP> <server-port> <func> <filename> ";
		std::cout << "<corrupt %%> <loss %%>" << std::endl;
		exit(EXIT_FAILURE);
	}

	unsigned short client_port = (unsigned short) strtoul(argv[1], NULL, 0);
	unsigned short server_port = (unsigned short) strtoul(argv[3], NULL, 0);
	char* type = argv[4];
	char* filename = argv[5];
	float corrupt_chance = atof(argv[6]);
	float loss_chance = atof(argv[7]);

	int sockfd;
	uint8_t packet_type = 255;
	struct sockaddr_in server;
	struct sockaddr_in client;
	socklen_t slen = sizeof(server);
	
	// Parse the given server IP address.
	if (inet_aton(argv[2], &server.sin_addr) == 0)
	{
		std::cerr << "Error: Given IP address not valid: " << argv[2] << std::endl;
		exit(EXIT_FAILURE);
	}

	// Parse the function code
	if (!strcmp(type, "PUT"))
		packet_type = PUT;
	else if (!strcmp(type, "GET"))
		packet_type = GET;
	else if (!strcmp(type, "DEL"))
		packet_type = DEL;

	if (packet_type == 255)
	{
		std::cerr << "Error: Given function comand not valid: " << type << std::endl;
		std::cerr << "Please use \"PUT\"" << std::endl;
		exit(EXIT_FAILURE);
	}

	//char buffer[packet_size]; // should the 256 be packet_size? make in PUT func?
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		error("Error: Could not create client socket.");
	}

	// Set the timeout.
	struct timeval tv;
	tv.tv_sec = TIMEOUT_SEC; 
	tv.tv_usec = TIMEOUT_USEC; 
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv,
			sizeof(struct timeval));
	
	// Set all the information on the client address struct
	client.sin_family = AF_INET;
	client.sin_port = htons(client_port);
	client.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sockfd, (struct sockaddr*) &client, sizeof(client)) == -1)
	{
		std::cerr << "Error: Could not bind client process to port " << client_port << std::endl;
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(server_port);

	std::cout << "Attempting to talk wth server at " << argv[2] << ":" << argv[3] << std::endl;


	// Begin sending loop
	PUT_func(filename, corrupt_chance, loss_chance, sockfd, server);

	exit(EXIT_SUCCESS);
}

void error(const char *msg) {
    std::cerr << msg << std::endl;
    exit(EXIT_FAILURE);
}

int checksum(char *msg, size_t len) {
    return int(std::accumulate(msg, msg + len, (unsigned char)0));
}

void makepacket(uint8_t type, uint8_t sequence, char *data, uint data_length,
        char buffer[PACKET_SIZE]) {
    uint8_t tp = type;
    memcpy(buffer + 0, &tp, 1);
    uint8_t seq = sequence;
    memcpy(buffer + 1, &seq, 1);
    int chk_sum = checksum(data, data_length);
    uint16_t chk = (uint16_t) chk_sum; // might need to be uint32_t for larger packet sizes
    memcpy(buffer + 2, &chk, 2);
    uint16_t len = (uint16_t) data_length;
    memcpy(buffer + 4, &len, 2);
    memcpy(buffer + 6, data, data_length);
}

bool gremlin(char *buffer, int damaged, int lost) {
    if(damaged < 0) 
    {
        damaged = 0;
    }
    if(damaged > 100) 
    {
        damaged = 100;
    }
    if(lost < 0) 
    {
        lost = 0;
    }
    if(lost > 100) 
    {
        lost = 100;
    }
    int damage_roll = rand() % 101;
    int lost_roll = rand() % 101;
    int num_dam = rand() % 101;
    if(lost_roll <= lost) 
    {
        return false;
    }
    if(damage_roll < damaged) {
        if(num_dam <= 70) 
        {
            int damaged_packet = rand() % 122 + 6;
            buffer[damaged_packet] = ~buffer[damaged_packet];
        }
        else if(num_dam <= 90) 
        {
            int damaged_packet = rand() % 122 + 6;
            buffer[damaged_packet] = ~buffer[damaged_packet];

            int previously_damaged = damaged_packet;
            while(previously_damaged == damaged_packet) {
                damaged_packet = rand() % 122 + 6;
            }
            buffer[damaged_packet] = ~buffer[damaged_packet];
        }
        else
        {
            int damaged_packet = rand() % 122 + 6;
            buffer[damaged_packet] = ~buffer[damaged_packet];

            int previously_damaged = damaged_packet;
            while(previously_damaged == damaged_packet) {
                damaged_packet = rand() % 122 + 6;
            }
            buffer[damaged_packet] = ~buffer[damaged_packet];

            int previously_damaged2 = damaged_packet;
            while(damaged_packet == previously_damaged2 || damaged_packet
                == previously_damaged) 
            {
                damaged_packet = rand() % 122 + 6;
            }
            buffer[damaged_packet] = ~buffer[damaged_packet];
        }
    }
    return true;
}

void PUT_func(char *filename, int damaged, int lost, int sockfd, struct sockaddr_in server) {
    char buffer[PACKET_SIZE];
	char data[PACKET_SIZE - 6];
	char output[PACKET_SIZE -6 + 1];
	socklen_t slen = sizeof(server);

	int timeout_count = 0;
	
	// Send initial request
	bool acknowledged = false;
	while (!acknowledged) {
		bzero(buffer, PACKET_SIZE);
		bzero(data, PACKET_SIZE - 6);
		snprintf(data, PACKET_SIZE - 6, "%s", filename);
		makepacket(PUT, 0, data, strlen(filename), buffer);

		std::cout << "PUT: Sending PUT request for " << filename << std::endl << std::endl;
		if (sendto(sockfd, buffer, PACKET_SIZE, 0,
				(const struct sockaddr *) &server, slen) == -1) {
			close(sockfd);
			std::cerr << "Error sending PUT request" << std::endl;
			exit(EXIT_FAILURE);
		}

		if (recv(sockfd, buffer, PACKET_SIZE, 0) == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				std::cout << "TIMEOUT: PUT request not acknowledged" << std::endl << std::endl;
				++timeout_count;
				if (timeout_count > TIMEOUT_PACKET_COUNT) {
					std::cerr << "TERMINATING TRANSFER: Server not responding" << std::endl;
					close(sockfd);
					exit(EXIT_FAILURE);
				}
			} else {
				close(sockfd);
				std::cerr << "Error: Could not receive message from server." << std::endl;
				exit(EXIT_FAILURE);
			}
		} else {
			timeout_count = 0;
			if (buffer[0] == ACK) {
				std::cout << "ACKNOWLEDGED: PUT request accepted" << std::endl << std::endl;
				acknowledged = true;
			} else {
				std::cout << "NOT ACKNOWLEDGED: PUT request not acknowledged" << std::endl << std::endl;
			}
		}
	}
	
	// Open file to read byte by btye
	FILE *infile;
	infile = fopen(filename, "rb");
	if (infile == NULL) {
		close(sockfd);
		std::cerr << "Error: Could not open file: " << filename << std::endl;
		exit(EXIT_FAILURE);
	}

	uint8_t sequence = 0;
	size_t numread = 0;
	while(!feof(infile)){	
		bzero(data, PACKET_SIZE - 6);
		numread = fread(data, 1, PACKET_SIZE - 6, infile);
		if (numread != PACKET_SIZE - 6 && !feof(infile)) {
			fclose(infile);
			close(sockfd);
			std::cerr << "Error: Could not properly read file" << std::endl;
			exit(EXIT_FAILURE);
		}

		bool acked = false;
		while (!acked) {
			bzero(buffer, PACKET_SIZE);
			makepacket(TRN, sequence, data, (uint16_t)numread, buffer);

			memset(output,'\0',PACKET_SIZE-6+1);
        	memcpy(output, data, 48);
			std::cout << "SENDING: sequence " << (int)sequence << std::endl;
			std::cout << "Data: " << output << std::endl << std::endl;

			if (gremlin(buffer, damaged, lost)) {
			    if (sendto(sockfd, buffer, PACKET_SIZE, 0,
				    (const struct sockaddr *) &server, slen) == -1) {
				    close(sockfd);
				    std::cerr << "Error: sending data to server" << std::endl;
				    exit(EXIT_FAILURE);
			    }
            }

			if (recv(sockfd, buffer, PACKET_SIZE, 0) == -1) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					std::cout << "TIMEOUT: data transfer not acknowledged" << std::endl << std::endl;
					++timeout_count;
					if (timeout_count > TIMEOUT_PACKET_COUNT) {
						std::cerr << "TERMINATING TRANSFER: Server not responding" << std::endl;
						close(sockfd);
						exit(EXIT_FAILURE);
					}
				} else {
					close(sockfd);
					std::cerr << "Error: Could not receive message from server." << std::endl;
					exit(EXIT_FAILURE);
				}
			} else {
				timeout_count = 0;
				if (buffer[0] == ACK) {
					std::cout << "ACKNOWLEDGED: sequence " << (int)sequence << ": data packet accepted" << std::endl << std::endl << std::endl;
					acked = true;
				} else {
					std::cout << "NOT ACKNOWLEDGED: sequence " << (int)sequence << ": data packet corrupted" << std::endl << std::endl << std::endl;
				}
			}
		}

		sequence = (sequence + 1) % SEQUENCE_COUNT;
	}

	// Send the final packet to close the file and finish the transfer.
	bool acked = false;
	while (!acked) {
		bzero(buffer, PACKET_SIZE);
		bzero(data, PACKET_SIZE - 6);
		makepacket(TRN, sequence, data, 0, buffer);

		std::cout << "SENDING: Close file transfer packet" << std::endl << std::endl;

		if (gremlin(buffer, damaged, lost)) {
		    if (sendto(sockfd, buffer, PACKET_SIZE, 0,
			    (const struct sockaddr *) &server, slen) == -1) {
			    close(sockfd);
			    std::cerr << "Error: sending data to server" << std::endl;
			    exit(EXIT_FAILURE);
		    }
        }

		if (recv(sockfd, buffer, PACKET_SIZE, 0) == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				std::cout << "TIMEOUT: data transfer not acknowledged" << std::endl << std::endl;
				++timeout_count;
				if (timeout_count > TIMEOUT_PACKET_COUNT) {
					std::cerr << "TERMINATING TRANSFER: Server not responding" << std::endl;
					close(sockfd);
					exit(EXIT_FAILURE);
				}
			} else {
				close(sockfd);
				std::cerr << "Error: Could not receive message from server." << std::endl;
				exit(EXIT_FAILURE);
			}
		} else {
			timeout_count = 0;
			if (buffer[0] == ACK) {
				std::cout << "ACKNOWLEDGED: data packet accepted" << std::endl << std::endl;
				acked = true;
			} else {
				std::cout << "NOT ACKNOWLEDGED: data packet corrupted" << std::endl << std::endl;
			}
		}
	}

	std::cout << "FINISHED: File completely sent: " << filename << std::endl << std::endl;
}
