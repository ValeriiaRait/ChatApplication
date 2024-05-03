/*
* Filename:		main.c
* Project:		CHAT-SYSTEM/chat-client
* By:			Valeriia Rait
* Date:			March 27, 2024
* Description:  This C file contains the main  program for the client side part
*/
#include "../inc/chatClient.h"
#include "../inc/outp_inp_handlers.h"
#include "../inc/ncursesUI.h"


int main(int argc, char* argv[]) {
    char userID[256];
    char serverName[256];

    //buffers for id and server name
    memset(userID, 0, sizeof(userID));
    memset(serverName, 0, sizeof(serverName));

    // parsing command-line arguments
    parseArguments(argc, argv, userID, sizeof(userID), serverName, sizeof(serverName));

    // check if w parsed successfully
    if (strlen(userID) == 0 || strlen(serverName) == 0) {
        fprintf(stderr, "Usage: %s -user<UserID> -server<ServerName>\n", argv[0]);
        return 1;
    }
    strncpy(currentUserID, userID, sizeof(currentUserID) - 1);

    sockfd = connectToServer(serverName, PORT_NUM);
     char helloMessage[CLIENT_MESSAGE_LENGTH + 1];
     const char* helloMsgContent = ">>hello<<";
    
    strncpy(helloMessage, helloMsgContent, CLIENT_MESSAGE_LENGTH);
    helloMessage[CLIENT_MESSAGE_LENGTH] = '\0'; // Ensure null termination


    struct ClientMessage clientMsg;
    strncpy(clientMsg.clientUserID, userID, CLIENT_USERID_LENGTH - 1);
    clientMsg.clientUserID[CLIENT_USERID_LENGTH - 1] = '\0'; //null-termination
    strncpy(clientMsg.message, helloMessage, CLIENT_MESSAGE_LENGTH);
    clientMsg.message[CLIENT_MESSAGE_LENGTH] = '\0'; // null termination

    // Serialize the client message to JSON
    char* jsonMsg = clientMessageToJson(&clientMsg);
    if (jsonMsg == NULL) {
        perror("Serialization failed");
    
    } else {
    
        if (send(sockfd, jsonMsg, strlen(jsonMsg), 0) < 0) {
            perror("send failed");
        }
        free(jsonMsg); 
    }

    // Check server response
    char buffer[JSON_LENGTH];
    memset(buffer, 0, JSON_LENGTH);
    int bytes_received = recv(sockfd, buffer, JSON_LENGTH, 0);
    if (bytes_received <= 0) {
        perror("Failed to receive data from server");
        return 1;
    }
    struct Broadcast* bcast = jsonToBroadcast(buffer);
    if (bcast == NULL) {
        printf("Failed to parse server response.\n");
        return 1;
    }
    // Check if this is a failure message
    if (bcast->clientUserID[0] == '\0' && bcast->clientIP[0] == '\0' && 
        strcmp(bcast->message, ">>failed<<") == 0) {
        // Failure message, signal the main thread to close
        free(bcast);
        perror("Server registration failed");
        return 1;
    }
    free(bcast);    

    // Start ncurses and threads
    init_ncurses();
    if (sockfd < 0) {
    perror("Socket not initialized");
    return 1;
    }

    pthread_t input_thread, output_thread;
    pthread_create(&input_thread, NULL, input_handler, (void*)userID);
    pthread_create(&output_thread, NULL, output_handler, NULL);

    pthread_join(input_thread, NULL);
    pthread_join(output_thread, NULL);

    //cleaning up
    endwin();
    close(sockfd);
    pthread_mutex_destroy(&ncurses_mutex);
    
    return 0;
}



