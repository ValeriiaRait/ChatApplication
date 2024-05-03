/*
* Filename:		serverIPC.h
* Project:		CHAT-SYSTEM/chat-server
* By:			Ekaterina (Kate) Stroganova
* Date:			March 25, 2024
* Description:  This header file contains function prototypes for IPC protocol for the CHAT-SYSTEM system.
*/

#ifndef SERVERIPC_H_INCLUDED
#define SERVERIPC_H_INCLUDED

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>

#include "../../common/inc/commonMessaging.h"

#define MAX_CLIENTS 10

#define SUCCESS 0
#define SOCKET_ERROR -1
#define MSG_Q_ERROR -2
#define SHARED_MEM_ERROR -3

#define MSG_QUEUE_SECRET 11111
#define MSG_QUEUE_PATH "."
#define SHARED_MEM_SECRET 16535
#define SHARED_MEM_PATH "."

#define ENTRY_NOT_FOUND_OR_NULL -1
#define TOO_MANY_CLIENTS -2

// Structs for message queue and shared memory

typedef struct
{
    long type;
    Broadcast broadcastMessage; 
} QueueMessageEnvelope;

typedef struct
{
    pthread_t threadID;
    char clientIP[CLIENT_IP_LENGTH + 1];
    char clientUserID[CLIENT_USERID_LENGTH + 1];
    int clientSocket;
} ClientState;


typedef struct
{
    int msgQueueID;
    int serverSocket;
    int numClients;
    int serverIsRunning;
    pthread_mutex_t mutex;
    ClientState connectedClients[MAX_CLIENTS];
} SharedData;


// Sockets
int setupServerSocket(uint16_t serverPort);
int closeServerSocket(int serverSocket);

// Message Queue
int setupMessageQueue();
int closeMessageQueue(int msgQID);

// Shared memory
int setupSharedMemory();
int initSharedMemory(int sharedMemID, int msgQID, int serverSocket);
int closeSharedMemory(int sharedMemID);

// SharedData processing
SharedData* getSharedData(int sharedMemID);
int findThreadIDInList(pthread_t threadID, SharedData* sharedDataP);
int findUserInList(const char* clientIP, const char* clientUserID, SharedData* sharedDataP);
int addToList(pthread_t threadID, const char* clientIP, const char* clientUserID, int clientSocket, SharedData* sharedDataP);
int removeFromList(int entryIndex, SharedData* sharedDataP);

// For testing
void printSharedData(SharedData* sharedDataP);

#endif //SERVERIPC_H_INCLUDED