/*
* Filename:		chatServer.h
* Project:		CHAT-SYSTEM/chat-server
* By:			Ekaterina (Kate) Stroganova
* Date:			March 25, 2024
* Description:  This header file contains function prototypes for server operations for the CHAT-SYSTEM system.
*/

#ifndef CHATSERVER_H_INCLUDED
#define CHATSERVER_H_INCLUDED

#include <ctype.h>
#include "serverIPC.h"

//#define TESTING // Uncomment for testing!

#define SERVER_PORT 30000

#define SUCCESS 0
#define SETUP_ERROR -1
#define EXIT_ERROR -2
#define THREAD_ERROR -3

#define THREAD_STARTUP_SHUTDOWN_SLEEP_LENGTH 2     // 2 seconds
#define THREAD_LOOP_SLEEP_LENGTH 10000    // 10 milliseconds

#define RUNNING 1
#define STOPPING 0

#define MESSAGE_PROCESS_QUIT 1
#define MESSAGE_PROCESS_SUCCESS 0
#define MESSAGE_PROCESS_FAILED -1
#define REGISTRATION_FAILED -2

#define IS_REGISTRATION 1
#define IS_MESSAGE 0
#define SERVER_REGISTRATION_MSG ">>hello<<"
#define SERVER_QUIT_MSG ">>bye<<"
#define SERVER_REGISTRATION_SUCCESS_MSG ">>success<<"
#define SERVER_REGISTRATION_FAIL_MSG ">>failed<<"

#define TYPE_SERVERMESSAGE 1

typedef struct NewClient
{
    int clientSocket;
    SharedData* sharedDataP;
} NewClient;

// Set-up
int setupServer(int* msgQID, int* sharedMemID, int* serverSocket);
int cleanUpServer(int msgQID, int sharedMemID, int serverSocket);

// Main server loop/thread
int runServer();

// Threads
void* clientConnectionMonitor(void* arg);
void* clientHandler (void* arg); 
void* chatBroadcaster(void* arg);

// Helper functions
int processMessage(int clientSocket, const char* clientIP, SharedData* sharedDataP, int isRegistration);
int sendMessageToQueue(const char* clientIP, ClientMessage* clientMessageP, SharedData* sharedDataP);
char* getClientIP(int clientSocket);
void splitString(const char *input, char *firstPart, char *secondPart, int maxSize);
void sendServerMessage(int clientSocket, const char* serverMessage);
int isWhitespace(const char *str);

#endif //CHATSERVER_H_INCLUDED