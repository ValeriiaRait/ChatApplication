/*
* Filename:		outp_inp_handlers.h
* Project:		CHAT-SYSTEM/chat-client
* By:			Valeriia Rait
* Date:			March 27, 2024
* Description:  This C file contains implementation for input/output action fromthe client side
*/
#include "chatClient.h"

/**
 * Function:       input_handler
 * Description:    handling user input for sending messages in the chat client and sends the messages them to the server. 
 *                 created its own thread anf handles all the input
 * 
 * Inputs:
 *                void *arg - user ID in this context, used for identifying the sender of messages.
 * 
 * Outputs:       messages in input window
 * 
 * Returns:       None
 */
void *input_handler(void *arg) {
    char *userID = (char*)arg;
    char message[CLIENT_MESSAGE_LENGTH + 1];
   
    
    while (1) {
        pthread_mutex_lock(&ncurses_mutex);
    
        werase(input_win);
        box(input_win, 0, 0);
        mvwprintw(input_win, 1, 1, "Enter message: ");   //promting the user
        wrefresh(input_win);
        echo();

        pthread_mutex_unlock(&ncurses_mutex);
        
        wgetnstr(input_win, message, CLIENT_MESSAGE_LENGTH);

        // check if exit command was entered
        if (strcmp(message, ">>bye<<") == 0) {
            send(sockfd, message, strlen(message), 0);
            break;
        }
        
        // create a message strucr and serialize it
        struct ClientMessage clientMsg;
        strncpy(clientMsg.clientUserID, userID, CLIENT_USERID_LENGTH);
        strncpy(clientMsg.message, message, CLIENT_MESSAGE_LENGTH);
        
        char* jsonMsg = clientMessageToJson(&clientMsg);
        if (send(sockfd, jsonMsg, strlen(jsonMsg), 0) < 0) {
            perror("send failed");
        }
        free(jsonMsg);    
        pthread_mutex_lock(&ncurses_mutex);
        
        //clear the input
        werase(input_win);
        box(input_win, 0, 0);
        wrefresh(input_win);
        pthread_mutex_unlock(&ncurses_mutex);
    }
    pthread_exit(NULL);
    
}



/**
 * Function:       output_handler
 * Description:    recieving messages from the server and displays them to the user. Creates
 *                 operates in its own thread, continuously reading from the socket. Uses mutex lock
 *                 to ensure that access to the ncurses window is thread-safe.
 * Outputs:         the recieved messages are displayed in the output window
 * 
 * Returns:        None
 */
void *output_handler(void *unused) {
    char buffer[JSON_LENGTH];

    while (1) {
        memset(buffer, 0, JSON_LENGTH);
        int bytes_received = recv(sockfd, buffer, JSON_LENGTH, 0);

        if (bytes_received <= 0) {
            break;
        }
        
        struct Broadcast* bcast = jsonToBroadcast(buffer);
        if (bcast) {
            pthread_mutex_lock(&ncurses_mutex);

            // check if this is a failure message
            if (bcast->clientUserID[0] == '\0' && bcast->clientIP[0] == '\0' && 
                strcmp(bcast->message, ">>failed<<") == 0) {
                // failure message, signal the main thread to close
                pthread_mutex_unlock(&ncurses_mutex);
                free(bcast);
                break; //quit
            }

             const char* direction = strcmp(bcast->clientUserID, currentUserID) == 0 ? ">>" : "<<";

            display_message(output_win, bcast->clientIP, bcast->clientUserID, bcast->message, direction);
            pthread_mutex_unlock(&ncurses_mutex);
            free(bcast);
        }
    }
    pthread_exit(NULL);
}



/**
 * Function:       add_message_to_history
 * Description:    adding message to the chat history. updates the message history structure by
 *                 adding a new message and adjusting indices accordingly
 * 
 * Inputs:
 *   const char *message -  message to be added to the history
 * 
 * Outputs:       if the history is full, the oldest message is overwritten.
 * 
 * Returns:       None
 */
void add_message_to_history(const char *message) {
    strncpy(messageHistory.messages[messageHistory.end], message, MESSAGE_MAX_LENGTH - 1);
    messageHistory.messages[messageHistory.end][MESSAGE_MAX_LENGTH - 1] = '\0';
    messageHistory.end = (messageHistory.end + 1) % HISTORY_SIZE;
    if (messageHistory.count < HISTORY_SIZE) {
        messageHistory.count++;
    } else {
        messageHistory.start = (messageHistory.start + 1) % HISTORY_SIZE;
    }
}



/**
 * Function:       display_message
 * Purpose:        Displays a message in output window, with the message formatted to include the sender's information and direction of the message
 *
 * Inputs:
 *   WINDOW *win - window where to siaplay the message
 *   const char *ip - ip address of the sender
 *   const char *username - user id of the sender
 *   const char *msg - message text to be displayed
 *   const char *direction - direction of the message ("incoming"/"outgoing")
 *
 * Outputs:        message is displayed in the specified window.
 *
 * Returns:        None
 */
void display_message(WINDOW *win, const char *ip, const char *username, const char *msg, const char *direction) {
    char time_str[9];
    time_t current_time = time(NULL);
    struct tm *tm_info = localtime(&current_time);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);

    char formatted_message[MESSAGE_MAX_LENGTH];
    snprintf(formatted_message, MESSAGE_MAX_LENGTH, "%-15s [%-5s] %2s %-40s (%s)\n", ip, username, direction, msg, time_str);

    
    add_message_to_history(formatted_message); //add new message

    //udjust the ui
    werase(win);
    box(win, 0, 0);

    // display the last up to 10 messages from history
    int historyIndex = messageHistory.start;
    for (int i = 0; i < messageHistory.count; i++) {
        wprintw(win, "%s", messageHistory.messages[historyIndex]);
        historyIndex = (historyIndex + 1) % HISTORY_SIZE;
    }

    // refresh the winfow with new info
    wrefresh(win);
}