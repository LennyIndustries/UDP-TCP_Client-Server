/*
 * TCP Server 2
 * gcc -Wall -pedantic -c lilog.c -o lilog.o
 * gcc -Wall -pedantic -c lilib.c -o lilib.o
 * gcc -Wall -pedantic TCP_Server2.c lilib.o lilog.o -l ws2_32 -o TCP_Server2.exe
 *
 * Based on : https://github.com/lborg019/c-multiclient-tcp-chat/blob/master/server.c
 *
 * Leander Dumas
 * Lenny Industries
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "lilib.h"

#define PORT "25042"

typedef struct User
{
	int user_socket;
	char *nick;
	struct User *next;
} User;

void runServer(fd_set master, int listener, User *head);
void *get_in_addr(struct sockaddr *sa);
void solveMessage(char *message, int socket, fd_set *master, User *head, char *returnMessage);
void addUser(int internet_socket, char *nick, User *head);
void removeUser(int socket, User *head);
void sendMessage(char *message, int socketSender, User *head);
User *getLastUser(User *head);
char *getUserNick(int socket, User *head);
void dumpUsers(User *head, char *nick);


int main(void)
{
	fd_set master; // Master file descriptor list
	int listener; // Listening socket descriptor
	int yes = 1; // For setsockopt() SO_REUSEADDR, below
	int rv = 0;
	struct addrinfo hints, *ai, *p;

	FD_ZERO(&master); // Set master to '0'

	// Creating a dummy user AKA Server
	User *head = NULL; // Creating dummy head for linked list
	head = (User *) malloc(sizeof(User)); // Assigning memory for dummy
	head->user_socket = -1; // Setting dummy socket to -1
	head->nick = malloc(strlen("Server")); // Assigning memory for dummy nick
	memset(head->nick, '\0', strlen("Server")); // Setting dummy nick to '\0'
	strcpy(head->nick, "Server"); // Copying "Dummy" to dummy nick
	head->next = NULL; // Setting dummy next to NULL

	// Fetch a socket, bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0)
	{
		fprintf(stderr, "Select server: %s\n", gai_strerror(rv));
		exit(1);
	}

	for (p = ai; p != NULL; p = p->ai_next)
	{
		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0)
		{
			continue;
		}

		// Lose the pesky "address already in use" error message
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
		{
			close(listener);
			continue;
		}

		break;
	}

	// If we got here, bonding failed.
	if (p == NULL)
	{
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}

	freeaddrinfo(ai); // All done; free memory

	puts("Binding successful!");
	// Hey listen
	if (listen(listener, 10) == -1)
	{
		perror("listen");
		exit(3);
	}
	puts("Server ready. Listening....");

	// Add the listener to the master set
	FD_SET(listener, &master);

	runServer(master, listener, head);

	return 0;
}

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in *) sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

void runServer(fd_set master, int listener, User *head)
{
	fd_set read_fds; // Temp file descriptor list for select()
	FD_ZERO(&read_fds); // Clear the temp set

	int fd_max = 0; // Max. amount of file descriptors
	// Keep track of the biggest file descriptor
	fd_max = listener; // (it's this one so far)

	socklen_t addr_len;

	int new_fd = 0; // Newly accept()ed socket descriptor

	struct sockaddr_storage remote_addr; // Client address

	char buffer[1027] = {'\0'}; // Buffer for data from the client
	char returnMessage[1027] = {'\0'}; // Buffer for data to the client
	int n_bytes;

	char remoteIP[INET6_ADDRSTRLEN];

	// Main loop
	while (1)
	{
		read_fds = master;
		if (select(fd_max + 1, &read_fds, NULL, NULL, NULL) == -1)
		{
			perror("select");
			exit(4);
		}

		// Run through the existing connections looking for data to read
		for (int i = 0; i <= fd_max; i++)
		{
			if (FD_ISSET(i, &read_fds))
			{
				// We got one
				if (i == listener)
				{
					// Handle new connections
					addr_len = sizeof remote_addr; new_fd = accept(listener, (struct sockaddr *) &remote_addr, &addr_len);
					if (new_fd == -1)
					{
						perror("accept");
					}
					else
					{
						FD_SET(new_fd, &master); // Pass to master set
						if (new_fd > fd_max)
						{   // Keep track of the max
							fd_max = new_fd;
						}
						printf("Select server: new connection from %s on socket %d\n", inet_ntop(remote_addr.ss_family, get_in_addr((struct sockaddr *) &remote_addr), remoteIP, INET6_ADDRSTRLEN), new_fd);
					}
				}
				else
				{
					// Handle data from a client
					if ((n_bytes = recv(i, buffer, sizeof buffer, 0)) <= 0)
					{
						// Got error or connection closed by client
						if (n_bytes == 0)
						{
							// Connection closed
							printf("Select server: socket %d disconnected\n", i);
						}
						else
						{
							perror("recv");
						}
						close(i);
						FD_CLR(i, &master); // Remove from master set
					}
					else
					{
						// We got some data from a client
						solveMessage(buffer, i, &master, head, returnMessage);
						sendMessage(returnMessage, i, head);
					}
				} // Handle data from client
			} // New incoming connection
		} // Looping through file descriptors
		// Clearing buffers
		memset(buffer, '\0', 1027);
		memset(returnMessage, '\0', 1027);
	} // While (1)
}

void solveMessage(char *message, int socket, fd_set *master, User *head, char *returnMessage)
{
	// Get command
	char *cmdBuffer = NULL;
	char *cmdDataBuffer = NULL;
	if (message[0] == '!')
	{
		cmdBuffer = malloc(indexOf(message, ':'));
		cmdDataBuffer = malloc((strlen(message) - indexOf(message, ':')) + 1);

		memset(cmdBuffer, '\0', indexOf(message, ':'));
		memset(cmdDataBuffer, '\0', (strlen(message) - indexOf(message, ':')) + 1);

		subString(message, cmdBuffer, 1, (indexOf(message, ':') - 2));
		subString(message, cmdDataBuffer, indexOf(message, ':') + 1, (int) (strlen(message) - indexOf(message, ':')));

		fprintf(stdout, "Command: %s :: %i\n", cmdBuffer, strlen(cmdBuffer));
		fprintf(stdout, "Command data: %s :: %i\n", cmdDataBuffer, strlen(cmdDataBuffer));
	}

	char tmpNick[25] = {'\0'};
	if (strcmp(cmdBuffer, "connect") == 0)
	{
		// Create new user
		fprintf(stdout, "Creating new user\n");
		addUser(socket, cmdDataBuffer, head); // Add a user to the list
		sprintf(returnMessage, "%s connected.", cmdDataBuffer);
	}
	else if (strcmp(cmdBuffer, "disconnect") == 0)
	{
		strcpy(tmpNick, getUserNick(socket, head)); // Can't put it in before if, on connect the user does not exist in the list
		// Send message on server
		fprintf(stdout, "%s disconnecting from socket %i\n", tmpNick, socket);
		// Remove user
		removeUser(socket, head);
		// Send ok to client
		int number_of_bytes_send = 0;
		number_of_bytes_send = send(socket, "disconnect_ok", 13, 0);
		if (number_of_bytes_send == -1)
		{
			perror("send");
		}
		// Free up the slot
		close(socket);
		FD_CLR(socket, master); // remove from master set
		// Setting up return message
		sprintf(returnMessage, "%s disconnected.", tmpNick);
		fprintf(stdout, "### %s disconnected.\n", tmpNick);
	}
	else if (strcmp(cmdBuffer, "message") == 0)
	{
		// Send message to all users
		sprintf(returnMessage, "%s", cmdDataBuffer);
	}
	else if (strcmp(cmdBuffer, "users") == 0)
	{
		strcpy(tmpNick, getUserNick(socket, head));
		// Dumping users
		fprintf(stdout, "Dumping users\n");
		dumpUsers(head, tmpNick);
		sprintf(returnMessage, "User_Dump");
	}
	else // No known command
	{
		sprintf(returnMessage, "--ERROR-- Unable to solve message: %s --ERROR--", message);
	}

	free(cmdBuffer);
	free(cmdDataBuffer);
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
	fprintf(stdout, "Disconnecting %s\n", pointer->nick);
	prev->next = pointer->next; // Fix link
	free(pointer); // Remove user
}

void sendMessage(char *message, int socketSender, User *head)
{
	if ((strstr(message, "disconnected")) || (strstr(message, "connected"))) // Send from server
	{
		socketSender = -1;
	}
	else if (strstr(message, "User_Dump")) // Don't send anything
	{
		return;
	}

	User *pointer = NULL;
	pointer = head;

	char *nick = NULL; // Setup nick
	nick = malloc(strlen(getUserNick(socketSender, head)) + 2);
	memset(nick, '\0', (strlen(getUserNick(socketSender, head)) + 2));
	strcpy(nick, getUserNick(socketSender, head));
	strcpy(nick + strlen(nick), ": ");

	char *messageBuffer = NULL; // Setup buffer
	messageBuffer = malloc(strlen(message) + strlen(nick));
	memset(messageBuffer, '\0', (strlen(message) + strlen(nick)));

	strcpy(messageBuffer, nick); // Put the nick into the buffer
	strcpy(messageBuffer + strlen(nick), message); // Put the message after nick

	int number_of_bytes_send = 0;

	while (pointer != NULL) // Send to all users
	{
		// Send message to users
		if ((pointer->user_socket != -1) && (pointer->user_socket != socketSender)) // Except server & sender
		{
			number_of_bytes_send = send(pointer->user_socket, messageBuffer, (int) strlen(messageBuffer), 0);
			if (number_of_bytes_send == -1)
			{
				perror("send");
			}
		}
		// Getting next user
		pointer = pointer->next;
	}

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

void dumpUsers(User *head, char *nick)
{
	User *pointer = NULL;
	pointer = head;
	char textBuffer[50];
	snprintf(textBuffer, 50, "Dumping users on request of: %s.", nick);
	sendMessage(textBuffer, -1, head);
	do
	{
		fprintf(stdout, "User: %s connected on socket: %i\n", pointer->nick, pointer->user_socket);
		memset(textBuffer, '\0', 50);
		snprintf(textBuffer, 50, "User: %s connected on socket: %i\n", pointer->nick, pointer->user_socket);
		sendMessage(textBuffer, -1, head);
		pointer = pointer->next;
	} while (pointer->next != NULL);

	memset(textBuffer, '\0', 50);
	snprintf(textBuffer, 50, "User: %s connected on socket: %i", pointer->nick, pointer->user_socket);
	sendMessage(textBuffer, -1, head);
	fprintf(stdout, "User: %s connected on socket: %i\n", pointer->nick, pointer->user_socket);
}
