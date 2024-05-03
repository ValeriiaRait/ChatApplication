/*
* Filename:		commonMessaging.h
* Project:		CHAT-SYSTEM/common
* By:			Ekaterina (Kate) Stroganova
* Date:			March 25, 2024
* Description:  This header file contains function prototypes for messaging for the CHAT-SYSTEM system.
*/

#ifndef COMMONMESSAGING_H_INCLUDED
#define COMMONMESSAGING_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLIENT_IP_LENGTH 15 // IP length is 15 + 1 for null-terminator
#define CLIENT_USERID_LENGTH 5 // User ID maximum length is 5 + 1 for null-terminator
#define BROADCAST_MESSAGE_LENGTH 40 // Message length maximum is 40 + 1 for null-terminator
#define CLIENT_MESSAGE_LENGTH 80 // Client to server message length maximum is 80 + 1 for null-terminator
#define MAX_BROADCASTS_PER_MSG 2
#define JSON_LENGTH 256

// Message structs
typedef struct Broadcast
{
    char clientIP[CLIENT_IP_LENGTH + 1];
    char clientUserID[CLIENT_USERID_LENGTH + 1];
    char message[BROADCAST_MESSAGE_LENGTH + 1];
} Broadcast;

typedef struct ClientMessage
{
    char clientUserID[CLIENT_USERID_LENGTH + 1];
    char message[CLIENT_MESSAGE_LENGTH + 1];
} ClientMessage;

// (De)Serialization functions
char* broadcastToJson(Broadcast* bcast);
struct Broadcast* jsonToBroadcast(const char* json_str);

char* clientMessageToJson(ClientMessage* msg);
struct ClientMessage* jsonToClientMessage(const char* json_str);

#endif // COMMONMESSAGING_H_INCLUDED
