/*
 * UDP Client
 * gcc -Wall -pedantic UDP_Client.c -l ws2_32 -o UDP_Client.exe
 * Leander Dumas
 * Lenny Industries
 */
#ifdef _WIN32
#define _WIN32_WINNT _WIN32_WINNT_WIN7

#include <winsock2.h> //for all socket programming
#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
#include <stdio.h> //for fprintf, perror
#include <unistd.h> //for close
#include <stdlib.h> //for exit
#include <string.h> //for memset

void OSInit(void)
{
	WSADATA wsaData;
	int WSAError = WSAStartup(MAKEWORD(2, 0), &wsaData);
	if (WSAError != 0)
	{
		fprintf(stderr, "WSAStartup errno = %d\n", WSAError);
		exit(-1);
	}
}

void OSCleanup(void)
{
	WSACleanup();
}

#define perror(string) fprintf( stderr, string ": WSA errno = %d\n", WSAGetLastError() )
#else
#include <sys/socket.h> //for sockaddr, socket, socket
#include <sys/types.h> //for size_t
#include <netdb.h> //for getaddrinfo
#include <netinet/in.h> //for sockaddr_in
#include <arpa/inet.h> //for htons, htonl, inet_pton, inet_ntop
#include <errno.h> //for errno
#include <stdio.h> //for fprintf, perror
#include <unistd.h> //for close
#include <stdlib.h> //for exit
#include <string.h> //for memset
void OSInit( void ) {}
void OSCleanup( void ) {}
#endif

#define DATA "Test Data String"
#define ACK "ok"
#define NAK "nok"

int initialization(struct sockaddr **internet_address, socklen_t *internet_address_length);
void execution(int internet_socket, struct sockaddr *internet_address, socklen_t internet_address_length, char packets, char tSec);
void cleanup(int internet_socket, struct sockaddr *internet_address);

int timeoutUDP(int socket, int tSec);

int main() // int argc, char * argv[]
{
	char packets = 0;
	int input = 0;
	char inputBuffer[1000] = {'\0'};
	char tSec = 0;

	//////////////////
	//Initialization//
	//////////////////

	OSInit();

	struct sockaddr *internet_address = NULL;
	socklen_t internet_address_length = 0;
	int internet_socket = initialization(&internet_address, &internet_address_length);

	/////////////
	//Execution//
	/////////////

	do
	{
		printf("How many packets do you want to send? (1 - 127) : ");
		scanf(" %s", inputBuffer);
		fflush(stdin);
		if (strcmp(inputBuffer, "shutdown") == 0)
		{
			printf("Shutting down server.\n");
			packets = -1;
			tSec = 1;
			break;
		}
		input = atoi(inputBuffer);
		if ((input > 0) && (input <= 127))
		{
			packets = (char) input;
			printf("Requesting %u packets\n", packets);
		}
		else
		{
			printf("Input out of range. (1 - 127)\n");
		}
	} while (packets == 0);

	while (tSec == 0)
	{
		printf("Set packet timeout (1 - 60) : ");
		scanf(" %s", inputBuffer);
		fflush(stdin);
		input = atoi(inputBuffer);
		if ((input >= 1) && (input <= 60))
		{
			tSec = input;
			printf("Timeout set to: %i sec\n", tSec);
		}
		else
		{
			printf("Input out of range. (1 - 60)\n");
		}
	}

	execution(internet_socket, internet_address, internet_address_length, packets, tSec);

	////////////
	//Clean up//
	////////////

	cleanup(internet_socket, internet_address);

	OSCleanup();

	return 0;
}

int initialization(struct sockaddr **internet_address, socklen_t *internet_address_length)
{
	//Step 1.1
	struct addrinfo internet_address_setup;
	struct addrinfo *internet_address_result;
	memset(&internet_address_setup, 0, sizeof internet_address_setup);
	internet_address_setup.ai_family = AF_UNSPEC;
	internet_address_setup.ai_socktype = SOCK_DGRAM;
	char ip[1000] = {'\0'};
	printf("IPv4: ");
	scanf(" %s", ip); // Request target ip (if it is wrong it will throw an error later, if it does not go to a replying server, it will timeout later)
	fflush(stdin);
	int getaddrinfo_return = getaddrinfo(ip, "24042", &internet_address_setup, &internet_address_result);
	if (getaddrinfo_return != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(getaddrinfo_return));
		exit(1);
	}

	int internet_socket = -1;
	struct addrinfo *internet_address_result_iterator = internet_address_result;
	while (internet_address_result_iterator != NULL)
	{
		//Step 1.2
		internet_socket = socket(internet_address_result_iterator->ai_family, internet_address_result_iterator->ai_socktype, internet_address_result_iterator->ai_protocol);
		if (internet_socket == -1)
		{
			perror("socket");
		}
		else
		{
			//Step 1.3
			*internet_address_length = internet_address_result_iterator->ai_addrlen;
			*internet_address = (struct sockaddr *) malloc(internet_address_result_iterator->ai_addrlen);
			memcpy(*internet_address, internet_address_result_iterator->ai_addr, internet_address_result_iterator->ai_addrlen);
			break;
		}
		internet_address_result_iterator = internet_address_result_iterator->ai_next;
	}

	freeaddrinfo(internet_address_result);

	if (internet_socket == -1)
	{
		fprintf(stderr, "socket: no valid socket address found\n");
		exit(2);
	}

	return internet_socket;
}

void execution(int internet_socket, struct sockaddr *internet_address, socklen_t internet_address_length, char packets, char tSec)
{
	// Preparing
	char packetBuffer[4] = {'\0'};
	// Timeout stuff
	int selectTiming = 0;

	// Checking if shutdown was requested
	if (packets == -1)
	{
		strcpy(packetBuffer, "HLT");
	}
	else
	{
		itoa(packets, packetBuffer, 10);
	}

	// Sending data to server
	int number_of_bytes_send = 0;
	number_of_bytes_send = sendto(internet_socket, packetBuffer, (int) strlen(packetBuffer), 0, internet_address, internet_address_length);
	if (number_of_bytes_send == -1)
	{
		perror("sendto");
	}

	// Preparing for data reception and check
	char correctData = 0;
	int number_of_bytes_received = 0;
	char buffer[1000];

	// ACK / NAK & timeout
	selectTiming = timeoutUDP(internet_socket, tSec); // Timeout
	switch (selectTiming)
	{
		case 0: // Timed out
			printf("Request timed out\n");
			return;
		case -1: // Error
			perror("select");
			return;
		default: // Normal
		{
			number_of_bytes_received = recvfrom(internet_socket, buffer, (sizeof buffer) - 1, 0, internet_address, &internet_address_length); // Get data
			if (number_of_bytes_received == -1)
			{
				perror("recvfrom");
			}
			else
			{
				int number_of_bytes_send = 0;
				buffer[number_of_bytes_received] = '\0';
				if (strcmp(buffer, packetBuffer) == 0) // Check return vs send value
				{
					number_of_bytes_send = sendto(internet_socket, ACK, strlen(ACK), 0, internet_address, internet_address_length); // Match
					if (number_of_bytes_send == -1)
					{
						perror("sendto");
					}
				}
				else
				{
					number_of_bytes_send = sendto(internet_socket, NAK, strlen(NAK), 0, internet_address, internet_address_length); // Mismatch
					if (number_of_bytes_send == -1)
					{
						perror("sendto");
					}
				}
			}
		}
	}

	// Receiving data & timeout
	for (char i = 0; i < packets; i++) // Loop for requested packets
	{
		selectTiming = timeoutUDP(internet_socket, tSec); // Timeout
		switch (selectTiming)
		{
			case 0: // Timed out
				printf("Packet %i timed out\n", (i + 1));
				break;
			case -1: // Error
				perror("select");
				break;
			default: // Normal
			{
				number_of_bytes_received = recvfrom(internet_socket, buffer, (sizeof buffer) - 1, 0, internet_address, &internet_address_length); // Get data
				if (number_of_bytes_received > 0)
				{
					buffer[number_of_bytes_received] = '\0';
					if (strcmp(buffer, DATA) == 0) // The data matches, count it
					{
						correctData++;
					}
					else if (strcmp(buffer, "cancel") == 0) // Check if server did not send cancel, this means the server did get a "nok"
					{
						printf("Server failed to receive data correctly.\n");
						return;
					}
					else // The data is wrong, don't count it
					{
						printf("Packet %i does not match: %s\n", (i + 1), buffer);
					}
				}
				else
				{
					perror("recvfrom");
				}
			}
		}
	}

	// Printing stats
	printf("Packets received: %i / %i | %i%%", correctData, packets, (int) (((float) correctData / (float) packets) * 100));
}

void cleanup(int internet_socket, struct sockaddr *internet_address)
{
	//Step 3.2
	free(internet_address);

	//Step 3.1
	close(internet_socket);
}

// Timeout function
int timeoutUDP(int socket, int tSec)
{
	#ifdef _WIN32 // Windows, use select
	struct timeval timeout;
	fd_set fds;

	timeout.tv_sec = tSec;
	timeout.tv_usec = 0;

	FD_ZERO(&fds);
	FD_SET(socket, &fds);

	return select(0, &fds, 0, 0, &timeout);

	#else // Unix, use poll

	struct pollfd pfds[1];
	pfds[0] =.fd = socket;
	pfds[0].events = POLLIN;

	int timeout = poll(pfds, 1, (tSec * 1000));

	if (timeout == 0)
	{
		return 0;
	}
	else
	{
		int pollInHappened = pfds[0].revents & POLLIN;

		if (pollInHappened)
		{
			return 1;
		}
		else
		{
			return -1;
		}
	}
	#endif
}
