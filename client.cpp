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
#include <errno.h> // not sure if necessary

#define TIMEOUT_SEC 0
#define TIMEOUT_USEC 20000

/* Global Variables */
int num_sequences = 2, packet_size = 128;

/* Prototypes */
void error(const char *);
int checksum(char *);
void makepacket(uint8_t, uint16_t, char *, char *);
void gremlin(char *, int, int, int, int, struct sockaddr_in, unsigned int,
	struct sockaddr_in);
void PUT_func(char *, int, int, int, int, struct sockaddr_in, unsigned int,
	struct sockaddr_in);

int main(int argc, char *argv[])
{
	if (argc < 4) {
		cout << "Usage: client <server> <port> <function>" << endl;
		exit(1);
	}
	int sock, n;
	unsigned int length;
	struct sockaddr_in server, from;
	struct hostent *hp;
	//char buffer[packet_size]; // should the 256 be packet_size? make in PUT func?

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		error("Error: Could not connect to socekt.");
	}

	// Set the timeout.
	struct timeval tv;
	tv.tv_sec = TIMEOUT_SEC; 
	tv.tv_usec = TIMEOUT_USEC; 
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv,
		sizeof(struct timeval));
	
	server.sin_family = AF_INET;
	hp = gethostbyname(argv[1]);
	if (hp <= 0) { // should it be "==" or "<="?
		error("Error: Server could not be found.");
	}
	bcopy((char *) hp->h_addr, (char *) &server.sin_addr, hp->h_length);
	server.sin_port = htons(atoi(argv[2]));
	length = sizeof(struct sockaddr_in); // rename length? should be serv_length
	switch(string(argv[3])) {
		case "PUT"
			if (argc == 7) {
				PUT_func(argv[4], atoi(argv[5]), atoi(argv[6]), sock, n, server,
					length, from);
			} else {
				cout << "Usage: client <server> <port> <function> ";
				cout << "PUT <filename> <damaged %> <lost %>" << endl;
				exit(1);
			}
			break;
		default:
			cout << "Error: Invalid function." << endl;
			cout << "Function List:" << endl;
			cout << "\tPUT <filename> <damaged %> <lost %>" << endl;
			exit(1);
	}
	return 0;
}

	/*printf("Please enter the message: ");
	bzero(buffer, 256);
	fgets(buffer, 255, stdin);*/
	n = sendto(sock, buffer, strlen(buffer), 0,
		(const struct sockaddr *) &server, length);
	if (n < 0) {
		error("Sendto");
	}
	n = recvfrom(sock, buffer, 256, 0, (struct sockaddr *) &from, &length);
	if (n < 0) {
		error("recvfrom");
	}
	write(1, "Got an ack: ", 12);
	write(1, buffer, n);
	close(sock);

void error(const char *msg) {
	perror(msg);
	exit(0);
}

int checksum(char *msg) {
	return int(std::accumulate(msg, msg + sizeof(msg), BYTE(0)));
}

void makepacket(unsigned int sequence, char *data, char buffer[packet_size]) {
	uint8_t seq = sequence;
	memcpy(buffer + 0, &seq, 1);
	int checksum = checksum(data);
	uint16_t chk = checksum; // might need to be uint32_t for larger packet sizes
	memcpy(buffer + 1, &chk, 2);
	memcpy(buffer + 3, data, packet_size - 3);
}

void PUT_func(char *buffer, int damaged, int lost, int socket, int newsocket,
	struct sockaddr_in server, unsigned int length, struct sockaddr_in from) {
	
}

void PUT_func(char *filename, int damaged, int lost, int socket, int newsocket,
	struct sockaddr_in server, unsigned int length, struct sockaddr_in from) {
	char buffer[packet_size];
	
	// Send initial request
	bool acknowledged = false;
	while (!acknowledged) {
		bzero(buffer, packet_size);
		buffer = "PUT " + string(filename); // should it be &filename?
		newsocket = sendto(socket, buffer, strlen(buffer), 0,
			(const struct sockaddr *) &server, length);
		if (newsocket < 0) {
			error("Error: Could not send to socket.");
		}
		cout << "Sending PUT request for " << filename << endl;
		newsocket = recv(socket, buffer, packet_size, 0);
		if (newsocket < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				cout << "Error: Timed out on server acknowledgement." << endl;
				cout << "Another PUT request will be sent." << endl;
			} else {
				error("Error: Could not receive message from server.");
			}
		} else {
			if (string(buffer) == "ACK") {
				cout << "PUT request acknowledged." << endl;
				acknowledged = true;
			} else {
				cout << "PUT request not acknowledged.\nError Message:" << endl;
				cout << string(buffer) << endl;
				cout << "Another PUT request will be sent." << endl;
			}
		}
	}
	
	// Open file to read byte by btye
	FILE *infile;
	infile = fopen(string(filename), "rb");
	if (infile == NULL) {
		error("Error: Could not open file.");
	}
	int sequence = 0;
	bool retransmit = false;
	char data[packet_size - 3];
	while(!feof(infile)){
		if (!retransmit) {
			bzero(data, packet_size - 3);
			size_t = numread;
			numread = fread(data, 1, packet_size - 3, infile);
			if (numread != packet_size - 3 && !feof(infile)) {
				error("Error: Could not read file.");
			}
		}
		bzero(buffer, packet_size);
		makepacket(sequence, data, buffer);
		gremlin(buffer, damaged, lost, socket, newsocket, server, length, from);
	}
}
