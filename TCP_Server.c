/*
 * TCP Server
 * gcc -Wall -pedantic -c lilog.c -o lilog.o
 * gcc -Wall -pedantic -c lilib.c -o lilib.o
 * gcc -Wall -pedantic TCP_Server.c lilib.o lilog.o -l ws2_32 -o TCP_Server.exe
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

#include "lilib.h"

typedef struct User
{
	int user_socket;
	char *nick;
	struct User *next;
} User;

int initialization();
int connection(int internet_socket);
void execution(int internet_socket, User *head);
void cleanup(int internet_socket, int client_internet_socket);

void addUser(int internet_socket, char *nick, User *head);
void removeUser(int socket, User *head);
void sendMessage(char *message, int socketSender, User *head);
User *getLastUser(User *head);
char *getUserNick(int socket, User *head);
void dumpUsers(User *head);

int main() // int argc, char * argv[]
{
	User *head = NULL; // Creating dummy head for linked list
	head = (User *) malloc(sizeof(User)); // Assigning memory for dummy
	head->user_socket = -1; // Setting dummy socket to -1
	head->nick = malloc(strlen("Dummy")); // Assigning memory for dummy nick
	memset(head->nick, '\0', strlen("Dummy")); // Setting dummy nick to '\0'
	strcpy(head->nick, "Dummy"); // Copying "Dummy" to dummy nick
	head->next = NULL; // Setting dummy next to NULL

	pid_t cPid = -1;

	//////////////////
	//Initialization//
	//////////////////

	fprintf(stdout, "Initialization\n");
	OSInit();

	int internet_socket = initialization();

	//////////////
	//Connection//
	//////////////

	int newSocket = 0;
	struct sockaddr_in newAddr;
	socklen_t addr_size;
	while (1)
	{
		newSocket = accept(internet_socket, (struct sockaddr *) &newAddr, &addr_size);
		if (newSocket < 0)
		{
			break;
		}

		fprintf(stdout, "Connection from: %s:%i", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));

		if ((cPid = fork()) == 0)
		{
			close(internet_socket);
			while (1)
			{
				execution(internet_socket, head);
			}
		}
	}

//	fprintf(stdout, "Connection\n");
//	int client_internet_socket = connection(internet_socket);
//
//	/////////////
//	//Execution//
//	/////////////
//
//	fprintf(stdout, "Execution\n");
//	while (1)
//	{
//		execution(client_internet_socket, head);
//	}

	free(head);

	////////////
	//Clean up//
	////////////

	fprintf(stdout, "Clean up\n");
	cleanup(internet_socket, newSocket);

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
	internet_address_setup.ai_socktype = SOCK_STREAM;
	internet_address_setup.ai_flags = AI_PASSIVE;
	int getaddrinfo_return = getaddrinfo(NULL, "25042", &internet_address_setup, &internet_address_result);
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
				perror("bind");
				close(internet_socket);
			}
			else
			{
				//Step 1.4
				int listen_return = listen(internet_socket, 1);
				if (listen_return == -1)
				{
					close(internet_socket);
					perror("listen");
				}
				else
				{
					break;
				}
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

int connection(int internet_socket)
{
	//Step 2.1
	struct sockaddr_storage client_internet_address;
	socklen_t client_internet_address_length = sizeof client_internet_address;
	int client_socket = accept(internet_socket, (struct sockaddr *) &client_internet_address, &client_internet_address_length);
	if (client_socket == -1)
	{
		perror("accept");
		close(internet_socket);
		exit(3);
	}
	return client_socket;
}

void execution(int internet_socket, User *head)
{
	// Prep
	int number_of_bytes_received = 0;
	char buffer[1000];
	// Wait for data
	number_of_bytes_received = recv(internet_socket, buffer, (sizeof buffer) - 1, 0);
	if (number_of_bytes_received == -1)
	{
		perror("recv");
	}
	else
	{
		buffer[number_of_bytes_received] = '\0';
		printf("Received : %s\n", buffer);
	}

	// Get command
	char *cmdBuffer = NULL;
	char *cmdDataBuffer = NULL;
	if (buffer[0] == '!')
	{
		cmdBuffer = malloc(indexOf(buffer, ':'));
		cmdDataBuffer = malloc((strlen(buffer) - indexOf(buffer, ':')) + 1);
		memset(cmdBuffer, '\0', indexOf(buffer, ':'));
		memset(cmdDataBuffer, '\0', (strlen(buffer) - indexOf(buffer, ':')) + 1);
		subString(buffer, cmdBuffer, 1, (indexOf(buffer, ':') - 2));
		subString(buffer, cmdDataBuffer, indexOf(buffer, ':') + 1, (int) (strlen(buffer) - indexOf(buffer, ':')));
		fprintf(stdout, "Command: %s :: %i\n", cmdBuffer, strlen(cmdBuffer));
		fprintf(stdout, "Command data: %s :: %i\n", cmdDataBuffer, strlen(cmdDataBuffer));
	}

	int number_of_bytes_send = 0;

	if (strcmp(cmdBuffer, "connect") == 0)
	{
		// Create new user
		fprintf(stdout, "Creating new user\n");
		addUser(internet_socket, cmdDataBuffer, head);
		// Send ack
		number_of_bytes_send = send(internet_socket, "ok", 2, 0);
	}
	else if (strcmp(cmdBuffer, "disconnect") == 0)
	{
		// Remove user
		removeUser(internet_socket, head);
		// Send ack
		number_of_bytes_send = send(internet_socket, "okD", 3, 0);
	}
	else if (strcmp(cmdBuffer, "message") == 0)
	{
		// Send message to all users
		sendMessage(cmdDataBuffer, internet_socket, head);
		// Send ack
		number_of_bytes_send = send(internet_socket, "ok", 2, 0);
	}
//	else if (strcmp(cmdBuffer, "shutdown") == 0)
//	{
//		// Shutting down server
//		fprintf(stdout, "Shutting down\n");
//		// send shutdown message to users
//		// Send ack
//		number_of_bytes_send = send(internet_socket, "ok", 2, 0);
//	}
	else if (strcmp(cmdBuffer, "users") == 0)
	{
		// Dumping users
		fprintf(stdout, "Dumping users\n");
		dumpUsers(head);
		// Send ack
//		number_of_bytes_send = send(internet_socket, "ok", 2, 0);
	}
	else // No known command
	{
		// Send nak
		number_of_bytes_send = send(internet_socket, "nok", 3, 0);
	}

	if (number_of_bytes_send == -1)
	{
		perror("send");
	}

	free(cmdBuffer);
	free(cmdDataBuffer);
}

void cleanup(int internet_socket, int client_internet_socket)
{
	//Step 4.2
	int how = -1;
	#ifdef _WIN32
	how = SD_BOTH;
	#else
	how = SHUT_RDWR;
	#endif
	int shutdown_return = shutdown(client_internet_socket, how);
	if (shutdown_return == -1)
	{
		perror("shutdown");
	}

	//Step 4.1
	close(client_internet_socket);
	close(internet_socket);
}

void addUser(int internet_socket, char *nick, User *head)
{
	// Get the last user
	User *lastUser = getLastUser(head);
	// Make a new user
	User *newUser = NULL;
	newUser = (User *) malloc(sizeof(User));
	newUser->user_socket = internet_socket;
	newUser->nick = malloc(strlen(nick));
	memset(newUser->nick, '\0', strlen(nick));
	strcpy(newUser->nick, nick);
	newUser->next = NULL;
	// Add the new user to the list
	lastUser->next = newUser;
	liLog(1, __FILE__, __LINE__, 1, "Added %s after %s.", newUser->nick, lastUser->nick);
}

void removeUser(int socket, User *head)
{
	User *pointer = NULL;
	User *prev = NULL;

	pointer = head;

	while (pointer->user_socket != socket)
	{
		prev = pointer;
		pointer = pointer->next;
	}

	liLog(1, __FILE__, __LINE__, 1, "Removing %s.", pointer->nick);
	fprintf(stdout, "%s disconnected\n", pointer->nick);
	prev->next = pointer->next;
	free(pointer);
}

void sendMessage(char *message, int socketSender, User *head)
{
	User *pointer = NULL;
	pointer = head;

	char *nick = NULL;
	nick = malloc(strlen(getUserNick(socketSender, head)) + 2);
	memset(nick, '\0', (strlen(getUserNick(socketSender, head)) + 2));
	strcpy(nick, getUserNick(socketSender, head));
	strcpy(nick + strlen(nick), ": ");

	char *messageBuffer = NULL;
	messageBuffer = malloc(strlen(message) + 25);
	memset(messageBuffer, '\0', (strlen(message) + 25));

	strcpy(messageBuffer, nick);
	strcpy(messageBuffer + strlen(nick), message);

	int number_of_bytes_send = 0;

	while (pointer != NULL)
	{
		// Send message to user
		if ((pointer->user_socket != -1) && (pointer->user_socket != socketSender))
		{
//			fprintf(stdout, "Sending %s to: %s\n", messageBuffer, pointer->nick);
			number_of_bytes_send = send(pointer->user_socket, messageBuffer, (int) strlen(messageBuffer), 0);
			if (number_of_bytes_send == -1)
			{
				perror("send");
			}
		}
		// Getting next user
		pointer = pointer->next;
	}

	free(pointer);
	free(nick);
	free(messageBuffer);
}

User *getLastUser(User *head)
{
	User *pointer = NULL;
	pointer = head;
	while (pointer->next != NULL)
	{
		pointer = pointer->next;
	}

	return pointer;
}

char *getUserNick(int socket, User *head)
{
	User *pointer = NULL;
	pointer = head;

	while (pointer->user_socket != socket)
	{
		pointer = pointer->next;
	}

	return pointer->nick;
}

void dumpUsers(User *head)
{
	User *pointer = NULL;
	pointer = head;
	char textBuffer[50];
	do
	{
		fprintf(stdout, "User: %s connected on socket: %i\n", pointer->nick, pointer->user_socket);
		memset(textBuffer, '\0', 50);
		snprintf(textBuffer, 50, "User: %s connected on socket: %i", pointer->nick, pointer->user_socket);
		sendMessage(textBuffer, -1, head);
		pointer = pointer->next;
	} while (pointer->next != NULL);

	memset(textBuffer, '\0', 50);
	snprintf(textBuffer, 50, "User: %s connected on socket: %i", pointer->nick, pointer->user_socket);
	sendMessage(textBuffer, -1, head);
	fprintf(stdout, "User: %s connected on socket: %i\n", pointer->nick, pointer->user_socket);
}
