/*
* Filename:		chatClient.h
* Project:		CHAT-SYSTEM/chat-client
* By:			Valeriia rait
* Date:			March 27, 2024
* Description:  This C file contains implementations for client chat.
*/

#ifndef CHAT_CLIENT_H
#define CHAT_CLIENT_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <ncurses.h>

#include "../../common/inc/commonMessaging.h"

#define PORT_NUM 30000
#define HISTORY_SIZE 10 //max number of messages to dusplay at a time
#define MESSAGE_MAX_LENGTH 80 //max message length


pthread_mutex_t ncurses_mutex = PTHREAD_MUTEX_INITIALIZER;
int sockfd;
WINDOW *input_win, *output_win;
char currentUserID[CLIENT_USERID_LENGTH];

//prototypes
//struct Broadcast* jsonToBroadcast(const char* json_str);
//struct ClientMessage* jsonToClientMessage(const char* json_str);
WINDOW *create_newwin(int height, int width, int starty, int startx);
void display_message(WINDOW *win, const char *ip, const char *username, const char *msg, const char *direction);



/**
 * Typedef Struct:  MessageHistory
 * Description:     Stores a history of chat messages. It keeps track of the messages in an array and
 *                  pointers (indices) to the start and end of the message history and a count
 *                  of the total messages currently stored. Allowing the
 *                  display of recent chat messages without exceeding memory limits.
 *
 *   char messages[HISTORY_SIZE][MESSAGE_MAX_LENGTH] - array of characters acting as a circular
 *                                                     buffer for storing messages.HISTORY_SIZE -
 *                                                     maximum number of messages that can be
 *                                                     stored at any time.
 *
 *   int start - index of the oldest message in the buffer.
 *
 *   int end - index of the most recent message in the buffer. 
 *
 *   int count - number of messages stored in the buffer. This helps to identify whether
 *               the buffer is full (count == HISTORY_SIZE) in order to
 *               overwrite old messages or simply adding new ones.
 */
typedef struct {
    char messages[HISTORY_SIZE][MESSAGE_MAX_LENGTH];
    int start;
    int end;
    int count;
} MessageHistory;
MessageHistory messageHistory = { .start = 0, .end = 0, .count = 0 };



/**
 * Function:       parseArguments
 * Purpose:        Parses the command line arguments to extract user ID and server name
 *
 * Inputs:
 *   int argc - number of command line arguments
 *   char *argv[] - array of arguments
 *   char *userID
 *   size_t userIDSize - size of the userID buffer
 *   char *serverName - buffer to store the server name extracted from arguments.
 *   size_t serverNameSize - size of the serverName buffer.
 *
 * Outputs:        None
 *
 * Returns:        None
 */
void parseArguments(int argc, char *argv[], char *userID, size_t userIDSize, char *serverName, size_t serverNameSize) {
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-user", 5) == 0) {
            strncpy(userID, argv[i] + 5, userIDSize - 1);
            userID[userIDSize - 1] = '\0'; //null-termination
        } else if (strncmp(argv[i], "-server", 7) == 0) {
            strncpy(serverName, argv[i] + 7, serverNameSize - 1);
            serverName[serverNameSize - 1] = '\0'; //null-termination
        }
    }
}



/**
 * Function:       connectToServer
 * Purpose:        create connection to the server at the specified port
 *
 * Inputs:
 *   const char* serverName - name of the server to connect to
 *   int port - port number on which to connect to the server
 *
 * Outputs:        NOne
 *
 * Returns:
 *   int - socket if the connection is successful, or an error code otherwise
 */
int connectToServer(const char* serverName, int port) {
 
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    //creating the socket and checking for its creation
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){ 
    perror("ERROR opening socket");
    return 0;
    }


    //get the ip address of the hostname
    server = gethostbyname(serverName);
    if (server == NULL) {
        fprintf(stderr,"ERROR, host was not found\n");
        return -1;
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    //copy server's IP Address to serv_addr
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);

    //setting port number
    serv_addr.sin_port = htons(port);

    // connecting
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        return -1;
    }

    return sockfd;
}

#endif