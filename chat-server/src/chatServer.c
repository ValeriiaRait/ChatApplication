/*
* Filename:		chatServer.c
* Project:		CHAT-SYSTEM/chat-server
* By:			Ekaterina (Kate) Stroganova
* Date:			March 25, 2024
* Description:  This file contains source code for server operations for the CHAT-SYSTEM system.
*
*               The chat server system consists of 4 different types of threads:
*                   - The main thread, which accepts new connections from clients.
*                   - Client handler threads, which register clients, receives their 
*                     messages and forwards the messages to the message queue. 
*                   - The client monitor thread, which monitors the number of clients still connected
*                     and initiates a server shutdown when all clients have disconnected.
*                   - The chat broadcaster thread, which receives messages from the message queue
*                     and broadcasts them to all connected clients.
*               
*               Some important design choices:
*                   - Server state and information about connected clients is maintained in 
*                     a SharedData struct, where each client is described by a ClientState struct.
*                   - When a new client connects to the server, it must register by sending a 
*                     ">>hello<<" message. The client handler will then retrieve the client's IP
*                     along with its user ID.
*                   - When a client sends a ">>bye<<" message, the client handler will stop listening
*                     for messages from the client, remove it from the list of active cliients
*                     and close the socket associated with this client.
*                   - When the client monitor finds that all clients are disconnected, the server
*                     socket is closed. When this is detected by the main thread, the message queue, 
*                     shared memory and server socket are all closed and cleaned up.
*               
*               The main SharedData structure maintains server state information, and is held in
*               shared memory. The information it contains includes:
*                   - The message queue ID (for client handler and chat broadcaster threads).
*                   - The server socket (for the client monitor to close the socket).
*                   - The number of currently connected clients.
*                   - The server's status (for the monitor thread to signal the main thread
*                     that the server should be shutting down, i.e., accept() call is supposed 
*                     to give an error).
*                   - A mutex to synchronize and protect access to the SharedData members.
*                   - An array of ClientState structs, one struct for each connected client.
*               
*               Each client's ClientState struct contains:
*                   - The thread ID for the client's handler. (Used to remove the client from the list)
*                   - The client's IP address as a string.
*                   - The client's user ID.
*                   - The client's socket.
*
*               Messages FROM the client TO the server are sent as JSON serialized ClientMessage.
*               Broadcasts FROM the server TO the client are sent as JSON serialized Broadcast.
*               
*               The ClientMessage struct consists of:
*                   - The client's user ID (if this is not provided, the server will reject the connection)
*                   - The client's message (of max length 80)
*
*               The Broadcast struct consists of:
*                   - The client's IP address.
*                   - The client's user ID (if this is not provided, the server will reject the connection)
*                   - The client's message (of max length 40)
*               
*               NOTE: Currently, no reply is sent to clients when invalid registration is received.
*                     Invalid registration is due to either duplicate IP/UserID pair OR faulty/missing registration message.
*/

#include "../inc/chatServer.h"


/*
* Function:     setupServer
* Purpose:      Sets up the server by creating a message queue, shared memory and socket.
*
* Inputs:       int*    msgQID          Pointer to store the message queue ID.
*               int*    sharedMemID     Pointer to store the shared memory ID.
*               int*    serverSocket    Pointer to store server socket.
*
* Outputs:      None.
*
* Returns:      int     retVal          Status of the setup operation (SUCCESS or CREATE_FAILED).
*/
int setupServer(int* msgQID, int* sharedMemID, int* serverSocket)
{
    int retVal = SUCCESS;

    // Get/create message queue
    *msgQID = setupMessageQueue();

    // Get/create server socket
    *serverSocket = setupServerSocket(SERVER_PORT);

    // Get/create shared memory
    *sharedMemID = setupSharedMemory();
    if (initSharedMemory(*sharedMemID, *msgQID, *serverSocket) == SHARED_MEM_ERROR)
    {
        retVal = SETUP_ERROR;
    }

    // Check if any issues
    if (*msgQID == MSG_Q_ERROR || *serverSocket == SOCKET_ERROR)
    {
        
        retVal = SETUP_ERROR;
    }

    if (*sharedMemID == SHARED_MEM_ERROR)
    {
        closeMessageQueue(*msgQID);
        retVal = SETUP_ERROR;
    }

    if (*serverSocket == SOCKET_ERROR)
    {
        closeMessageQueue(*msgQID);
        closeSharedMemory(*sharedMemID);
        retVal = SETUP_ERROR;
    }

    return retVal;
}


/*
 * Function:     cleanUpServer
 * Purpose:      Stops the server by cleaning up the message queue, shared memory and socket.
 *
 * Inputs:       int     msgQID          Message queue ID.
 *               int     sharedMemID     Shared memory ID.
 *               int     serverSocket    Server socket.
 *
 * Outputs:      None
 *
 * Returns:      int                     Status of the operation (SUCCESS or EXIT_FAILURE).
 */
int cleanUpServer(int msgQID, int sharedMemID, int serverSocket)
{
    int retVal = SUCCESS;

    // Clean up queue and shared memory
    if ((closeMessageQueue(msgQID)) == MSG_Q_ERROR)
    {
        retVal = EXIT_ERROR;
    }

    if ((closeSharedMemory(sharedMemID)) == SHARED_MEM_ERROR)
    {
        retVal = EXIT_ERROR;
    }

    // Remember to close socket as a last step
    if ((closeServerSocket(serverSocket)) == -1 )
    {
        retVal = EXIT_ERROR;
    }
    
    return retVal;
}


/*
* Function:     runServer
* Purpose:      Runs the server, accepting incoming connections and creating handler threads for each client.
*
* Inputs:       None
*
* Outputs:      None
*
* Returns:      int                     0 if successful, otherwise an error code.
*/
int runServer()
{
    int retVal = SUCCESS;

    int msgQID, shrdMemID, clientSocket, serverSocket, totalConnections;
    socklen_t clientLen;
    struct sockaddr_in clientAddress;

    // Get message queue ID, shared memory ID and server socket
    if (setupServer(&msgQID, &shrdMemID, &serverSocket) == SETUP_ERROR)
    {
        return SETUP_ERROR;
    }

    SharedData* sharedDataP = getSharedData(shrdMemID);
    //sharedDataP->numClients = 3;

    totalConnections = 0;

    #ifdef TESTING
        printf("Server started - accepting connections!\n");
    #endif

    while (RUNNING)
    {
        clientLen = sizeof(clientAddress);

        // Blocking call to accept() - should unblock if client connects or socket shuts down
        if ((clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientLen)) < 0) {
            pthread_mutex_lock(&sharedDataP->mutex);

            if (sharedDataP->serverIsRunning)
            {
                // Server is still supposed to be running - unexpected error occurred!
                perror("[SERVER] : accept() FAILED\n");
                retVal = SOCKET_ERROR;
            }

            pthread_mutex_unlock(&sharedDataP->mutex);
            break;
        }

        // New client has connected - prep data for sending to clientHandler
        NewClient newClient = {.clientSocket = clientSocket, .sharedDataP = sharedDataP};
        pthread_t newClientThread;

        // Start client handler
        if (pthread_create(&newClientThread, NULL, clientHandler, (void*)&newClient) != 0) {
            perror("pthread_create");
            retVal = THREAD_ERROR;
            break;
        }

        // Start client monitor and broadcaster AFTER first client has already connected
        // Monitor and broadcaster should wait MONITOR_STARTUP_SLEEP_LENGTH amount of seconds before starting
        // To make sure client has enough time to register
        if (totalConnections == 0)
        {
            pthread_t monitorThread, broadcasterThread;

            //sleep(THREAD_STARTUP_SLEEP_LENGTH);

            // Start client monitor
            if (pthread_create(&monitorThread, NULL, clientConnectionMonitor, sharedDataP) != 0) {
                perror("pthread_create");
                retVal = THREAD_ERROR;
                break;
            }

            // Start broadcaster
            if (pthread_create(&broadcasterThread, NULL, chatBroadcaster, sharedDataP) != 0) {
                perror("pthread_create");
                retVal = THREAD_ERROR;
                break;
            }
        }

        totalConnections++;
    }

    // Socket is probably already closed at this stage, but attempting to close it again should
    // not cause any issues
    retVal = cleanUpServer(msgQID, shrdMemID, serverSocket);
    
    // Sleep here to make sure all threads are stopped - alternatively, could wait and join threads?
    sleep(THREAD_STARTUP_SHUTDOWN_SLEEP_LENGTH);

    #ifdef TESTING
        printf("Server stopped - should be clean!\n");
    #endif

    return retVal;
}


/*
* Function:     clientConnectionMonitor
* Purpose:      Monitors client connections and stops the server if there are no active clients.
*
* Inputs:       void*       arg         A pointer to the shared data structure.
*
* Outputs:      None
*
* Returns:      void*
*/
void* clientConnectionMonitor(void* arg)
{
    SharedData* sharedDataP = (SharedData*) arg;

    int serverIsRunning = STOPPING;

    // Sleep before starting monitoring
    //sleep(THREAD_STARTUP_SLEEP_LENGTH);
    while (!serverIsRunning)
    {
        // Lock
        pthread_mutex_lock(&sharedDataP->mutex);

        if (sharedDataP->numClients > 0)
        {
            serverIsRunning = RUNNING;
        }

        // Unlock
        pthread_mutex_unlock(&sharedDataP->mutex);

        usleep(THREAD_LOOP_SLEEP_LENGTH); // Sleep for 10 milliseconds
    }
    

    #ifdef TESTING
        printf("Client monitor started running!\n");
    #endif

     // Increment counter in a loop
    while (serverIsRunning) 
    {
        // Lock mutex before accessing shared data
        pthread_mutex_lock(&sharedDataP->mutex);

        if (sharedDataP->numClients <= 0) 
        {
            closeServerSocket(sharedDataP->serverSocket);
            sharedDataP->serverIsRunning = 0;
            serverIsRunning = STOPPING;
        }

        // Unlock mutex after accessing shared data
        pthread_mutex_unlock(&sharedDataP->mutex);

        usleep(THREAD_LOOP_SLEEP_LENGTH); // Sleep for 10 milliseconds
    }

    #ifdef TESTING
        printf("Client monitor stopping!\n");
    #endif

    pthread_exit(NULL);
}


/*
* Function:     clientHandler
* Purpose:      Handles communication with a client, including registration and message processing.
*
* Inputs:       void*       arg         A pointer to the NewClient structure containing client socket and shared data pointer.
*
* Outputs:      None
*
* Returns:      void*
*/
void* clientHandler (void* arg)
{
    NewClient* newClientP = (NewClient*) arg;
    pthread_t threadID = pthread_self();

    #ifdef TESTING
        //printf("New client on thread ID: %lu\n", threadID);
    #endif

    // Retrieve necessary data
    int clientSocket = newClientP->clientSocket;
    SharedData* sharedDataP = newClientP->sharedDataP;
    //int msgQID = sharedDataP->msgQueueID; // Should be safe to access without mutex since it should never change

    // Get client IP
    char* clientIP = getClientIP(clientSocket);
    if (clientIP == NULL)
    {
        perror("getClientIP");
        close(clientSocket);
        pthread_exit(NULL);
    }


    // Attempt to register client
    if (processMessage(clientSocket, clientIP, sharedDataP, IS_REGISTRATION) != MESSAGE_PROCESS_SUCCESS)
    {
        // Client failed to register correctly
        close(clientSocket);
        free(clientIP);
        pthread_exit(NULL);
    }

    // Client registered - loop until quit
    int processResult;
    while (RUNNING)
    {
        processResult = processMessage(clientSocket, clientIP, sharedDataP, IS_MESSAGE);

        if (processResult != MESSAGE_PROCESS_SUCCESS)
        {
            break;
        }
    }

    // Remove client from list
    // Lock mutex
    pthread_mutex_lock(&sharedDataP->mutex);

    int clientIndex = findThreadIDInList(threadID, sharedDataP);
    removeFromList(clientIndex, sharedDataP);

    #ifdef TESTING
        printf("\nClient '%s' from '%s' disconnected!\n", sharedDataP->connectedClients[clientIndex].clientUserID, clientIP);
        printSharedData(sharedDataP);
    #endif

    // Unlock mutex
    pthread_mutex_unlock(&sharedDataP->mutex);

    // Clean up
    close(clientSocket);
    free(clientIP);
    pthread_exit(NULL);
}


/*
* Function:     chatBroadcaster
* Purpose:      Broadcasts messages to all connected clients.
*
* Inputs:       void*       arg         A pointer to the shared data structure.
*
* Outputs:      None
*
* Returns:      void*
*/
void* chatBroadcaster(void* arg)
{
    SharedData* sharedDataP = (SharedData*) arg;

    int msgQID = sharedDataP->msgQueueID;

    QueueMessageEnvelope envelope;

    int serverIsRunning = STOPPING;

    //sleep(THREAD_STARTUP_SLEEP_LENGTH);
    while (!serverIsRunning)
    {
        // Lock
        pthread_mutex_lock(&sharedDataP->mutex);

        if (sharedDataP->numClients > 0)
        {
            serverIsRunning = RUNNING;
        }

        // Unlock
        pthread_mutex_unlock(&sharedDataP->mutex);

        usleep(THREAD_LOOP_SLEEP_LENGTH); // Sleep for 10 milliseconds
    }

    #ifdef TESTING
        printf("Chat broadcaster started running!\n");
    #endif

    while (serverIsRunning) {

        // Lock mutex
        //pthread_mutex_lock(&sharedDataP->mutex);

        if (sharedDataP->numClients <= 0) 
        {
            serverIsRunning = STOPPING;
            break;
        }

        // Receive message envelope from the message queue
        if (msgrcv(msgQID, &envelope, sizeof(Broadcast), TYPE_SERVERMESSAGE, IPC_NOWAIT) == -1) {
            // Error occured - if error is ENOMSG, then no problem just continue; otherwise break
            if (errno != ENOMSG)
            {
                serverIsRunning = STOPPING;
                perror("msgrcv");
                break;
            }
        }
        else
        {
            // Message received from queue - broadcast to all clients!
            char* broadcastMsg = broadcastToJson(&envelope.broadcastMessage);

            // Lock
            pthread_mutex_lock(&sharedDataP->mutex);

            int clientSocket;

            // Broadcast broadcastMsg to all clients
            for (int i = 0; i < sharedDataP->numClients; i++)
            {
                clientSocket = sharedDataP->connectedClients[i].clientSocket;
                send(clientSocket, broadcastMsg, strlen(broadcastMsg), 0);
            }

            // Unlock
            pthread_mutex_unlock(&sharedDataP->mutex);      

            #ifdef TESTING
                printf("\nBroadcasting '%s' to all clients.\n", broadcastMsg);
            #endif

            free(broadcastMsg);                                    

        }

        // Unlock mutex
        //pthread_mutex_unlock(&sharedDataP->mutex);

        
        usleep(THREAD_LOOP_SLEEP_LENGTH);
    }

    #ifdef TESTING
        printf("Chat broadcaster stopping!\n");
    #endif

    pthread_exit(NULL);
}


/*
* Function:     processMessage
* Purpose:      Processes messages received from clients, including registration and normal messages.
*
* Inputs:       int             clientSocket        The socket file descriptor of the client.
*               const char*     clientIP            The IP address of the client.
*               SharedData*     sharedDataP         Pointer to the shared data structure.
*               int             isRegistration      Flag indicating if the message is a registration message.
*
* Outputs:      None
*
* Returns:      int                                 MESSAGE_PROCESS_SUCCESS if successful, MESSAGE_PROCESS_FAILED if failed to process message, MESSAGE_PROCESS_QUIT if the message indicates the client is quitting.
*/
int processMessage(int clientSocket, const char* clientIP, SharedData* sharedDataP, int isRegistration)
{
    int retVal = MESSAGE_PROCESS_SUCCESS;

    pthread_t threadID = pthread_self();

    #ifdef TESTING
        //printf("Process Message thread ID: %lu\n", threadID);
    #endif

    char readBuffer[JSON_LENGTH];
    int numBytesRead;

    memset(readBuffer, 0, JSON_LENGTH);

    // Read message
    numBytesRead = read(clientSocket, readBuffer, JSON_LENGTH);

    // Message successfully read! Try to deserialize
    ClientMessage* clientMessage = jsonToClientMessage(readBuffer);

    // Check number of bytes for an error - in theory, if client closes connection, should be here
    if (numBytesRead == -1 || strlen(clientMessage->clientUserID) == 0 || isWhitespace(clientMessage->clientUserID) == 1)
    {
        #ifdef TESTING
            printf("Read from socket failed - Client may have died or tried to register with empty UserID!\n");
        #endif

        if (isRegistration)
        {
            sendServerMessage(clientSocket, SERVER_REGISTRATION_FAIL_MSG);
        }

        return MESSAGE_PROCESS_FAILED;
    }

    if (isRegistration)
    {
        // Lock mutex
        pthread_mutex_lock(&sharedDataP->mutex);

        // Registration so check for ">>hello<<" message AND for non-duplicate/unregistered user
        int foundIndex = findUserInList(clientIP, clientMessage->clientUserID, sharedDataP);

        if (strncmp(clientMessage->message, SERVER_REGISTRATION_MSG, sizeof(SERVER_REGISTRATION_MSG)) == 0
            && foundIndex == ENTRY_NOT_FOUND_OR_NULL)
        {
            // Valid registration - add to list
            if (addToList(threadID, clientIP, clientMessage->clientUserID, clientSocket, sharedDataP) == TOO_MANY_CLIENTS)
            {
                retVal = MESSAGE_PROCESS_FAILED;
                #ifdef TESTING
                    printf("\nClient '%s' from '%s' failed to connect - maximum clients already reached!\n", clientMessage->clientUserID, clientIP);
                #endif
            }
            else
            {
                sendServerMessage(clientSocket, SERVER_REGISTRATION_SUCCESS_MSG);
                #ifdef TESTING
                    printf("\nClient '%s' from '%s' connected!\n", clientMessage->clientUserID, clientIP);
                    printSharedData(sharedDataP);
                #endif 
            }

            
        }
        else
        {
            retVal = REGISTRATION_FAILED;
            // User failed to register - send reply
            sendServerMessage(clientSocket, SERVER_REGISTRATION_FAIL_MSG);

            #ifdef TESTING
                printf("\nClient '%s' from '%s' attempted to register with already existing User ID or without correct registration message!\n", clientMessage->clientUserID, clientIP);
            #endif
        }

        // Unlock mutex
        pthread_mutex_unlock(&sharedDataP->mutex);
    }
    else
    {
        // Lock mutex
        pthread_mutex_lock(&sharedDataP->mutex);

        // Normal message, check for ">>bye<<"
        if (strncmp(clientMessage->message, SERVER_QUIT_MSG, sizeof(SERVER_QUIT_MSG)) == 0)
        {
            int clientIndex = findThreadIDInList(threadID, sharedDataP);
            removeFromList(clientIndex, sharedDataP);

            retVal = MESSAGE_PROCESS_QUIT;
        }
        else
        {
            // Normal message! Send to message queue
            sendMessageToQueue(clientIP, clientMessage, sharedDataP);
        }

        // Unlock mutex
        pthread_mutex_unlock(&sharedDataP->mutex);
    }

    // Clean up memory
    free(clientMessage);

    return retVal;
}


/*
* Function:     sendMessageToQueue
* Purpose:      Sends a message received from a client to the message queue for broadcasting.
*
* Inputs:       const char*         clientIP            The IP address of the client.
*               ClientMessage*      clientMessageP      Pointer to the ClientMessage structure containing the client's message.
*               SharedData*         sharedDataP         Pointer to the shared data structure.
*
* Outputs:      None
*
* Returns:      int                                     SUCCESS if successful, MESSAGE_PROCESS_FAILED if failed to send message to queue.
*/
int sendMessageToQueue(const char* clientIP, ClientMessage* clientMessageP, SharedData* sharedDataP)
{
    int retVal = SUCCESS;

    int msgQID = sharedDataP->msgQueueID;

    // Message elements
    int numMessages = 1;
    Broadcast broadcastMessages[MAX_BROADCASTS_PER_MSG];
    QueueMessageEnvelope envelope;

    if (strlen(clientMessageP->message) > BROADCAST_MESSAGE_LENGTH)
    {
        numMessages = 2;
    }

    // Split message if needed
    if (numMessages == 2)
    {
        splitString(clientMessageP->message, broadcastMessages[0].message, broadcastMessages[1].message, BROADCAST_MESSAGE_LENGTH);
    }
    else
    {
        strncpy(broadcastMessages[0].message, clientMessageP->message, BROADCAST_MESSAGE_LENGTH);
        broadcastMessages[0].message[BROADCAST_MESSAGE_LENGTH] = '\0'; // Ensure null termination
    }

    // Lock mutex
    //pthread_mutex_lock(&sharedDataP->mutex);

    for (int i = 0; i < numMessages; i++)
    {
        // Copy client IP
        strncpy(broadcastMessages[i].clientIP, clientIP, CLIENT_IP_LENGTH);
        broadcastMessages[i].clientIP[CLIENT_IP_LENGTH] = '\0'; // Ensure null termination

        // Copy client user ID
        strncpy(broadcastMessages[i].clientUserID, clientMessageP->clientUserID, CLIENT_USERID_LENGTH);
        broadcastMessages[i].clientUserID[CLIENT_USERID_LENGTH] = '\0'; // Ensure null termination

        // Fill message envelope
        envelope.type = TYPE_SERVERMESSAGE;
        envelope.broadcastMessage = broadcastMessages[i];

        // Send to message queue at msgQID
        if (msgsnd(msgQID, (void *)&envelope, sizeof(Broadcast), 0) == -1) {
            perror("mq_send");
            retVal = MESSAGE_PROCESS_FAILED; 
        }

        #ifdef TESTING
            printf("\nMESSAGE '%s' from '%s' SENT TO QUEUE\n", broadcastMessages[i].message, broadcastMessages[i].clientUserID);
        #endif
    }

    // Unlock mutex
    //pthread_mutex_unlock(&sharedDataP->mutex);

    

    return retVal;
}


/*
* Function:     getClientIP
* Purpose:      Retrieves the IP address of a client given its socket.
*
* Inputs:       int                 clientSocket        The socket descriptor of the client.
*
* Outputs:      None
*
* Returns:      char*                                   A string containing the client's IP address, or NULL on failure.
*/
char* getClientIP(int clientSocket) {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    char* clientIP = malloc(INET_ADDRSTRLEN);

    if (clientIP == NULL) {
        perror("malloc");
        return NULL;
    }

    // Get the client's address
    if (getpeername(clientSocket, (struct sockaddr *)&clientAddr, &clientAddrLen) == -1) {
        perror("getpeername");
        free(clientIP);
        return NULL;
    }

    // Convert the binary IP address to a human-readable string
    if (inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN) == NULL) {
        perror("inet_ntop");
        free(clientIP);
        return NULL;
    }

    return clientIP;
}


/*
* Function:     splitString
* Purpose:      Splits a string into two parts based on a maximum size.
*
* Inputs:       const char*         input               The input string to split.
*               char*               firstPart           Pointer to a character array to store the first part of the split string.
*               char*               secondPart          Pointer to a character array to store the second part of the split string.
*               int                 maxSize             The maximum size for the first part of the split string.
*
* Outputs:      None
*
* Returns:      void
*/
void splitString(const char *input, char *firstPart, char *secondPart, int maxSize) 
{
    // Ensure input is not NULL
    if (input == NULL || firstPart == NULL || secondPart == NULL) {
        return;
    }

    // Initialize variables
    int inputLen = strlen(input);
    int splitPoint = -1;

    // Find the split point based on whitespace
    if (inputLen > maxSize)
    {
        int index = maxSize - 1;

        for (int i = index; i >= 0; i--) {
            if (isspace(input[i])) {
                splitPoint = i;
                break; // Stop searching when the first whitespace is found
            }
        }
    }

    // If no whitespace found or splitting by whitespace exceeds maxSize, split directly down the middle
    if (splitPoint == -1 || inputLen - splitPoint >= maxSize - 1) {
        splitPoint = maxSize; // Split directly down the middle
    }

    // Copy the substrings
    strncpy(firstPart, input, splitPoint);
    firstPart[splitPoint] = '\0'; // Null-terminate the first part

    // Skip leading whitespace in the second part
    while (isspace(input[splitPoint]) && splitPoint < inputLen) {
        splitPoint++;
    }
    strncpy(secondPart, input + splitPoint, inputLen - splitPoint);
    secondPart[inputLen - splitPoint] = '\0'; // Null-terminate the second part
}


/*
* Function:     sendServerMessage
* Purpose:      Sends a message from the server to a specific client
*
* Inputs:       int                 clientSocket              Client's socket.
*               const char*         serverMessage             Message from server to send.
*
* Outputs:      None
*
* Returns:      void
*/
void sendServerMessage(int clientSocket, const char* serverMessage)
{

    Broadcast serverBroadcast = {.clientIP = "", .clientUserID = ""};
    strncpy(serverBroadcast.message, serverMessage, BROADCAST_MESSAGE_LENGTH + 1); // Copy the message

    char* broadcastJSON = broadcastToJson(&serverBroadcast);

    send(clientSocket, broadcastJSON, strlen(broadcastJSON), 0);

    free(broadcastJSON);
}


/*
* Function:     isWhitespace
* Purpose:      Checks if string is whitespace or empty
*
* Inputs:       const char*         str                 The string to check.
*
* Outputs:      None
*
* Returns:      void
*/
int isWhitespace(const char *str) 
{
    while (*str) 
    {
        if (!isspace(*str))
            return 0; // Not whitespace
        str++;
    }
    return 1; // All characters are whitespace or it's an empty string
}