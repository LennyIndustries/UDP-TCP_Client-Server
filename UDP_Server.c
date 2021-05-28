/*
 * UDP Server
 * gcc -Wall -pedantic UDP_Server.c -l ws2_32 -o UDP_Server.exe
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
#include <poll.h>
void OSInit( void ) {}
void OSCleanup( void ) {}
#endif

#define DATA "Test Data String"
#define ACK "ok"
#define NAK "nok"
#define TIMEOUT 5
//#define ARTIFICIAL_FAIL 2 // Use this to send less packets the requested, forcing a timeout on the client, make sure that the total send packets is still > 0

int initialization();
void execution(int internet_socket, char *looping);
void cleanup(int internet_socket);

int timeoutUDP(int socket, int tSec);

int main() // int argc, char * argv[]
{
	//////////////////
	//Initialization//
	//////////////////

	OSInit();

	int internet_socket = initialization();

	/////////////
	//Execution//
	/////////////

	// Setting up loop
	char *looping = NULL; // It was planned to use an other thread to stop the server, now the client requests a stop
	looping = malloc(sizeof(char));
	memset(looping, 0, sizeof(char));

	*looping = 1; // Set loop to loop

	while (*looping)
	{
		execution(internet_socket, looping); // Keep doing this until a shutdown comes in from client
	}

	free(looping);

	////////////
	//Clean up//
	////////////

	cleanup(internet_socket);

	OSCleanup();

	return 0;
}

int initialization()
{
	//Step 1.1
	struct addrinfo internet_address_setup;
	struct addrinfo *internet_address_result;
	memset(&internet_address_setup, 0, sizeof internet_address_setup);
	internet_address_setup.ai_family = AF_INET;
	internet_address_setup.ai_socktype = SOCK_DGRAM;
	internet_address_setup.ai_flags = AI_PASSIVE;
	int getaddrinfo_return = getaddrinfo(NULL, "24042", &internet_address_setup, &internet_address_result);
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
			int bind_return = bind(internet_socket, internet_address_result_iterator->ai_addr, internet_address_result_iterator->ai_addrlen);
			if (bind_return == -1)
			{
				close(internet_socket);
				perror("bind");
			}
			else
			{
				break;
			}
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

void execution(int internet_socket, char *looping)
{
	// Preparing
	char timesToSend = 0;
	int number_of_bytes_received = 0;
	char buffer[1000];
	char AckNakBuffer[1000];
	struct sockaddr_storage client_internet_address;
	socklen_t client_internet_address_length = sizeof client_internet_address;
	// Timeout stuff
	int selectTiming = 0;
	// Waiting for data
	number_of_bytes_received = recvfrom(internet_socket, buffer, (sizeof buffer) - 1, 0, (struct sockaddr *) &client_internet_address, &client_internet_address_length);
	if (number_of_bytes_received == -1) // Error
	{
		perror("recvfrom");
	}
	else // Got data
	{
		buffer[number_of_bytes_received] = '\0';
		printf("Received : %s\n", buffer);
	}

	// ACK / NAK
	// Making sure the data is correct
	int number_of_bytes_send = 0;
	number_of_bytes_send = sendto(internet_socket, buffer, (int) strlen(buffer), 0, (struct sockaddr *) &client_internet_address, client_internet_address_length);
	if (number_of_bytes_send == -1)
	{
		perror("sendto");
	}

	selectTiming = timeoutUDP(internet_socket, TIMEOUT); // Setting a timeout on the request
	switch (selectTiming)
	{
		case 0: // Timed out
			// Timed out, do whatever you want to handle this situation
			printf("Request timed out\n");
			return;
		case -1: // Error
			// Error occurred, maybe we should display an error message?
			// Need more tweaking here and the recvfromTimeOutUDP()...
			perror("select");
			return;
		default: // Normal
		{
			number_of_bytes_received = recvfrom(internet_socket, AckNakBuffer, (sizeof AckNakBuffer) - 1, 0, (struct sockaddr *) &client_internet_address, &client_internet_address_length);
			if (number_of_bytes_received == -1)
			{
				perror("recvfrom");
			}
			else
			{
				AckNakBuffer[number_of_bytes_received] = '\0';
				printf("Received : %s\n", AckNakBuffer);
			}

			if (strcmp(AckNakBuffer, ACK) != 0) // Check for ACK, if the data got corrupted stop anyway
			{
				number_of_bytes_send = 0;
				// Sending to the client that the server will not send data
				number_of_bytes_send = sendto(internet_socket, "cancel", 6, 0, (struct sockaddr *) &client_internet_address, client_internet_address_length);
				if (number_of_bytes_send == -1)
				{
					perror("sendto");
				}
				return;
			}
		}
	}

	// Checking if shutdown (HLT) was requested
	if (strcmp(buffer, "HLT") == 0)
	{
		// Stopping the loop
		printf("Shutting down.\n");
		*looping = 0;
	}
	else
	{
		// Preparing to precess data
		timesToSend = atoi(buffer);
		#ifdef ARTIFICIAL_FAIL
		timesToSend -= ARTIFICIAL_FAIL; // artificially create packet los
		#endif
		// Sending data
		for (char i = 0; i < timesToSend; i++)
		{
			number_of_bytes_send = sendto(internet_socket, DATA, strlen(DATA), 0, (struct sockaddr *) &client_internet_address, client_internet_address_length);
			if (number_of_bytes_send == -1)
			{
				perror("sendto");
			}
		}
	}
}

void cleanup(int internet_socket)
{
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
	pfds[0].fd = socket;
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
