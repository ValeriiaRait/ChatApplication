/*
* Filename:		commonMessaging.h
* Project:		CHAT-SYSTEM/common
* By:			Ekaterina (Kate) Stroganova
* Date:			March 22, 2024
* Description:  This C file contains implementations for messaging for the CHAT-SYSTEM system.
*/

#include "../inc/commonMessaging.h"

/*
* Function:       broadcastToJson
* Purpose:        Serialize a Broadcast struct to a JSON-formatted string.
*
* Inputs:         struct Broadcast* bcast  Pointer to the Broadcast struct to be serialized.
*
* Outputs:        None
*
* Returns:        char*  Pointer to the allocated memory containing the JSON string.
*                        This memory must be freed by the caller.
*/
char* broadcastToJson(struct Broadcast* bcast) 
{
    char* json_str = (char*)malloc(JSON_LENGTH); // Allocate memory for JSON string
    sprintf(json_str, "{\"clientIP\":\"%s\",\"clientUserID\":\"%s\",\"message\":\"%s\"}",
            bcast->clientIP, bcast->clientUserID, bcast->message);
    return json_str;
}

/*
* Function:       jsonToBroadcast
* Purpose:        Deserialize a JSON-formatted string to a Broadcast struct.
*
* Inputs:         const char* json_str  JSON-formatted string to be deserialized.
*
* Outputs:        None
*
* Returns:        struct Broadcast*  Pointer to the allocated memory containing the deserialized Broadcast struct.
*                                    This memory must be freed by the caller.
*/
struct Broadcast* jsonToBroadcast(const char* json_str) 
{
    struct Broadcast* bcast = (struct Broadcast*)malloc(sizeof(struct Broadcast));

    if (bcast == NULL) {
        return NULL; // Memory allocation failed
    }
    
    // Initialize fields to empty strings
    bcast->clientIP[0] = '\0';
    bcast->clientUserID[0] = '\0';
    bcast->message[0] = '\0';

    // Parse JSON string
    const char* clientIP_start = strstr(json_str, "\"clientIP\":\"");
    if (clientIP_start != NULL) {
        sscanf(clientIP_start, "\"clientIP\":\"%15[^\"]\"", bcast->clientIP);
    }

    const char* clientUserID_start = strstr(json_str, "\"clientUserID\":\"");
    if (clientUserID_start != NULL) {
        sscanf(clientUserID_start, "\"clientUserID\":\"%5[^\"]\"", bcast->clientUserID);
    }

    const char* message_start = strstr(json_str, "\"message\":\"");
    if (message_start != NULL) {
        sscanf(message_start, "\"message\":\"%40[^\"]\"", bcast->message);
    }
    
    return bcast;
}

/*
* Function:       clientMessageToJson
* Purpose:        Serialize a ClientMessage struct to a JSON-formatted string.
*
* Inputs:         struct ClientMessage* msg  Pointer to the ClientMessage struct to be serialized.
*
* Outputs:        None
*
* Returns:        char*  Pointer to the allocated memory containing the JSON string.
*                        This memory must be freed by the caller.
*/
char* clientMessageToJson(struct ClientMessage* msg) 
{
    char* json_str = (char*)malloc(JSON_LENGTH); // Allocate memory for JSON string
    sprintf(json_str, "{\"clientUserID\":\"%s\",\"message\":\"%s\"}",
            msg->clientUserID, msg->message);
    return json_str;
}

/*
* Function:       jsonToClientMessage
* Purpose:        Deserialize a JSON-formatted string to a ClientMessage struct.
*
* Inputs:         const char* json_str  JSON-formatted string to be deserialized.
*
* Outputs:        None
*
* Returns:        struct ClientMessage*  Pointer to the allocated memory containing the deserialized ClientMessage struct.
*                                        This memory must be freed by the caller.
*/
struct ClientMessage* jsonToClientMessage(const char* json_str) 
{
    struct ClientMessage* msg = (struct ClientMessage*)malloc(sizeof(struct ClientMessage));
    if (msg == NULL) {
        return NULL; // Memory allocation failed
    }
    
    // Initialize fields to empty strings
    msg->clientUserID[0] = '\0';
    msg->message[0] = '\0';

    // Parse JSON string
    const char* clientUserID_start = strstr(json_str, "\"clientUserID\":\"");
    if (clientUserID_start != NULL) {
        sscanf(clientUserID_start, "\"clientUserID\":\"%5[^\"]\"", msg->clientUserID);
    }

    const char* message_start = strstr(json_str, "\"message\":\"");
    if (message_start != NULL) {
        sscanf(message_start, "\"message\":\"%80[^\"]\"", msg->message);
    }
    
    return msg;
}