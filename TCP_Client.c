/*
 * TCP Client
 * gcc -Wall -pedantic -c lilog.c -o lilog.o
 * gcc -Wall -pedantic -c lilib.c -o lilib.o
 * gcc -Wall -pedantic TCP_Client.c lilib.o lilog.o -l ws2_32 -o TCP_Client.exe
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
int OSInit( void ) {}
int OSCleanup( void ) {}
#endif

#include "lilib.h"

int initialization();
void execution(int internet_socket);
void cleanup(int internet_socket);

void requestConnect(int internet_socket);

int main() // int argc, char *argv[]
{
	liLog(1, __FILE__, __LINE__, 0, "Starting program, clearing log.");
	fprintf(stdout, "Welcome %s to LICS:\nLenny Industries Chat Service.\n\n", getenv("USERNAME"));
	//////////////////
	//Initialization//
	//////////////////

	liLog(1, __FILE__, __LINE__, 1, "Initialization");
	OSInit();

	int internet_socket = initialization();

	/////////////
	//Execution//
	/////////////

	liLog(1, __FILE__, __LINE__, 1, "Execution");
	execution(internet_socket);


	////////////
	//Clean up//
	////////////

	liLog(1, __FILE__, __LINE__, 1, "Clean up");
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
	internet_address_setup.ai_family = AF_UNSPEC;
	internet_address_setup.ai_socktype = SOCK_STREAM;
	char ip[1000] = {'\0'};
	fprintf(stdout, "Enter the server IPv4: ");
	scanf(" %s", ip); // Request target ip (if it is wrong it will throw an error later, if it does not go to a replying server, it will timeout later)
	fflush(stdin);
	int getaddrinfo_return = getaddrinfo(ip, "25042", &internet_address_setup, &internet_address_result);
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
			int connect_return = connect(internet_socket, internet_address_result_iterator->ai_addr, internet_address_result_iterator->ai_addrlen);
			if (connect_return == -1)
			{
				perror("connect");
				close(internet_socket);
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

void execution(int internet_socket)
{
	requestConnect(internet_socket);

	int number_of_bytes_received = 0;
	char buffer[1000];
	number_of_bytes_received = recv(internet_socket, buffer, (sizeof buffer) - 1, 0);
	if (number_of_bytes_received == -1)
	{
		perror("recv");
	}
	else
	{
		buffer[number_of_bytes_received] = '\0';
		liLog(1, __FILE__, __LINE__, 1, "Received: %s", buffer);
	}

	if (strcmp(buffer, "ok") == 0)
	{
		fprintf(stdout, "Connected\n\n");
	}
	else
	{
		fprintf(stdout, "An error occurred.\nTerminating program.\n");
		return;
	}

	int number_of_bytes_send = 0;
	char messageBuffer[1009];
	do
	{
		memset(buffer, '\0', 1000);
		memset(messageBuffer, '\0', 1009);
		fprintf(stdout, "Message: ");

		if (fgets(buffer, sizeof(buffer), stdin) == NULL)
		{
			fprintf(stderr, "Error in fgets()\n");
			liLog(3, __FILE__, __LINE__, 1, "Error in fgets()");
			exit(EXIT_FAILURE);
		}

		fflush(stdin);

		buffer[strlen(buffer) - 1] = '\0'; // The last character is \n

		if ((strcmp(buffer, "/disconnect") != 0) && (strcmp(buffer, "/shutdown") != 0) && (strcmp(buffer, "/users") != 0))
		{
			strcpy(messageBuffer, "!message:");
			strcpy(messageBuffer + 9, buffer);

			number_of_bytes_send = send(internet_socket, messageBuffer, (int) strlen(messageBuffer), 0);
			if (number_of_bytes_send == -1)
			{
				perror("send");
			}
		}
		else
		{
			if (strcmp(buffer, "/disconnect") == 0)
			{
				memset(messageBuffer, '\0', 1000);
				strcpy(messageBuffer, "!disconnect:");
			}
			else if (strcmp(buffer, "/shutdown") == 0)
			{
				memset(messageBuffer, '\0', 1000);
				strcpy(messageBuffer, "!shutdown:");
			}
			else if (strcmp(buffer, "/users") == 0)
			{
				memset(messageBuffer, '\0', 1000);
				strcpy(messageBuffer, "!users:");
			}
			else
			{
				fprintf(stdout, "Unknown error!\n");
				return;
			}

			number_of_bytes_send = send(internet_socket, messageBuffer, (int) strlen(messageBuffer), 0);
			if (number_of_bytes_send == -1)
			{
				perror("send");
			}
		}

		if ((strcmp(buffer, "/shutdown") == 0) || (strcmp(buffer, "/disconnect") == 0))
		{
			break;
		}

		memset(buffer, '\0', 1000);

		number_of_bytes_received = recv(internet_socket, buffer, (sizeof buffer) - 1, 0);
		if (number_of_bytes_received == -1)
		{
			perror("recv");
		}
		else
		{
			buffer[number_of_bytes_received] = '\0';
			liLog(1, __FILE__, __LINE__, 1, "Received: %s", buffer);
			if (strcmp(buffer, "ok") != 0)
			{
				fprintf(stdout, "%s\n", buffer);
			}
		}
	} while (1);
}

void cleanup(int internet_socket)
{
	//Step 3.2
	int shutdown_return = shutdown(internet_socket, SD_SEND);
	if (shutdown_return == -1)
	{
		perror("shutdown");
	}

	//Step 3.1
	close(internet_socket);
}

void requestConnect(int internet_socket) // https://stackoverflow.com/questions/42265038/how-to-check-if-user-enters-blank-line-in-scanf-in-c
{
	char buffer[1000] = {'\0'};
	char *nick = NULL;

	do
	{
		printf("Nickname (max 25 or blank) : ");

		if (fgets(buffer, sizeof(buffer), stdin) == NULL)
		{
			fprintf(stderr, "Error in fgets()\n");
			liLog(3, __FILE__, __LINE__, 1, "Error in fgets()");
			exit(EXIT_FAILURE);
		}

		fflush(stdin);

		if (strlen(buffer) > 25)
		{
			fprintf(stdout, "Too long!\n");
			memset(buffer, '\0', 1000);
		}
		else if (buffer[0] == '\n')
		{
			subString(getenv("USERNAME"), buffer, 0, 24); // getenv("USERNAME") // "Test_Data_123456789_123456789_123"
		}
		else
		{
			buffer[strlen(buffer) - 1] = '\0'; // The last character is \n
		}
	} while (buffer[0] == '\0');

	nick = (char *) malloc(strlen(buffer) + 9 + 1);
	memset(nick, '\0', strlen(buffer) + 9 + 1);
	strcpy(nick, "!connect:");
	strcpy(nick + 9, buffer);

	liLog(1, __FILE__, __LINE__, 1, "Sending command to server: %s", nick);
	int number_of_bytes_send = 0;
	number_of_bytes_send = send(internet_socket, nick, (int) strlen(nick), 0);
	if (number_of_bytes_send == -1)
	{
		perror("send");
	}

	free(nick);
}
