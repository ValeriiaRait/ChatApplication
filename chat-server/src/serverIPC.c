/*
* Filename:		serverIPC.c
* Project:		CHAT-SYSTEM/chat-server
* By:			Ekaterina (Kate) Stroganova
* Date:			March 25, 2024
* Description:  This file contains source code for IPC protocol for the CHAT-SYSTEM system.
*/

#include "../inc/serverIPC.h"

/*
* Function:     setupServerSocket
* Purpose:      Sets up a server socket and binds it to a specified port for listening to incoming connections.
*
* Inputs:       uint16_t        serverPort      The port on which the server socket will listen for incoming connections.
*
* Outputs:      None
*
* Returns:      int             The server socket file descriptor if successful, otherwise SOCKET_ERROR.
*/
int setupServerSocket(uint16_t serverPort)
{
    int serverSocket;
    struct sockaddr_in serverAddress;

    // Try to get a socket, return if error
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        perror("[SERVER] : socket() FAILED");
        return SOCKET_ERROR;
    }

    // Initialize server address
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(serverPort);

    // Bind server socket
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("[SERVER] : bind() FAILED");
        close(serverSocket);
        return SOCKET_ERROR;
    }

    // Listen for incoming connections
    if (listen(serverSocket, MAX_CLIENTS) < 0) {
        perror("[SERVER] : listen() FAILED");
        close(serverSocket);
        return SOCKET_ERROR;
    }

    return serverSocket;
}


/*
* Function:     closeServerSocket
* Purpose:      Closes a server socket.
*
* Inputs:       int         serverSocket    The file descriptor of the server socket to be closed.
*
* Outputs:      None
*
* Returns:      int                         0 if successful, otherwise an error code.
*/
int closeServerSocket(int serverSocket)
{
    shutdown(serverSocket, SHUT_RDWR);
    return close(serverSocket);
}


// Message Queue

/*
* Function:     setupMessageQueue
* Purpose:      Sets up the message queue.
*
* Inputs:       None
*
* Outputs:      None
*
* Returns:      int         Message queue ID if successful, otherwise CREATE_FAILED.
*/
int setupMessageQueue()
{
    int msgQID;
    key_t msgQKey;

    // Allocate key (Using constants defined in commonIPC.h)
    if ((msgQKey = ftok(MSG_QUEUE_PATH, MSG_QUEUE_SECRET)) == -1)
    {
        return MSG_Q_ERROR;
    }

    // Check if queue already exists
    if ((msgQID = msgget(msgQKey, 0)) == -1)
    {
        // Queue doesn't exist! Create a message queue
        if ((msgQID = msgget(msgQKey, IPC_CREAT | 0666)) == -1) 
        {
            return MSG_Q_ERROR;
        }
    }

    return msgQID;
}


/*
* Function:     closeMessageQueue
* Purpose:      Closes a message queue and releases associated system resources.
*
* Inputs:       int         msgQID      The ID of the message queue to be closed.
*
* Outputs:      None
*
* Returns:      int                     0 if successful, otherwise an error code.
*/
int closeMessageQueue(int msgQID)
{
    int retVal = SUCCESS;

    if ((msgctl(msgQID, IPC_RMID, (struct msqid_ds*) NULL)) == -1 )
    {
        perror("msgctl");
        retVal = MSG_Q_ERROR;
    }

    return retVal;
}


// Shared memory

/*
* Function:     setupSharedMemory
* Purpose:      Sets up the shared memory for inter-process communication.
*
* Inputs:       int             msgQID          The message queue ID.
*               int             serverSocket    The server socket.
*
* Outputs:      None
*
* Returns:      int                             The shared memory ID if setup is successful,
*                                               SHARED_MEM_ERROR if an error occurs during setup.
*/
int setupSharedMemory(int msgQID, int serverSocket)
{
    int sharedMemID;
    key_t shmKey;

    if ((shmKey = ftok(SHARED_MEM_PATH, SHARED_MEM_SECRET)) == -1)
    {
        return SHARED_MEM_ERROR;
    }

    // Check if shared memory already exists
    if ((sharedMemID = shmget(shmKey, sizeof(SharedData), 0)) == -1)
    {
        // Shared memory doesn't exist! Create shared memory block
        if ((sharedMemID = shmget(shmKey, sizeof(SharedData), IPC_CREAT | 0666)) == -1) 
        {
            return SHARED_MEM_ERROR;
        }
    }

    return sharedMemID;
}


/*
* Function:     initSharedMemory
* Purpose:      Initializes a shared memory segment with necessary data structures and attributes.
*
* Inputs:       int     sharedMemID     The ID of the shared memory segment to be initialized.
*               int     msgQID          Message queue ID associated with the shared memory.
*               int     serverSocket    Server socket file descriptor associated with the shared memory.
*
* Outputs:      None
*
* Returns:      int                     0 if successful, otherwise an error code.
*/
int initSharedMemory(int sharedMemID, int msgQID, int serverSocket)
{
    int retVal = SUCCESS;
    
    // Initialize shared data
    SharedData* sharedDataP = getSharedData(sharedMemID);
    sharedDataP->msgQueueID = msgQID;
    sharedDataP->numClients = 0;
    sharedDataP->serverSocket = serverSocket;
    sharedDataP->serverIsRunning = 1;

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        sharedDataP->connectedClients[i].clientIP[0] = 0;
        sharedDataP->connectedClients[i].clientUserID[0] = 0;
        sharedDataP->connectedClients[i].threadID = 0;
        sharedDataP->connectedClients[i].clientSocket = 0;
    }

    // Initialize mutex
    if (pthread_mutex_init(&sharedDataP->mutex, NULL) != 0) {
        perror("pthread_mutex_init");
        retVal = SHARED_MEM_ERROR;
    }

    return retVal;
}


/*
* Function:     closeSharedMemory
* Purpose:      Closes a shared memory segment and releases associated system resources.
*
* Inputs:       int     sharedMemID     The ID of the shared memory segment to be closed.
*
* Outputs:      None
*
* Returns:      int                     0 if successful, otherwise an error code.
*/
int closeSharedMemory(int sharedMemID)
{
    int retVal = SUCCESS;

    SharedData* sharedDataP = getSharedData(sharedMemID);

    // Clean up mutex first
    pthread_mutex_destroy(&sharedDataP->mutex);

    // Detach and remove shared memory segment
    if (shmdt(sharedDataP) == -1) {
        perror("shmdt");
        retVal = SHARED_MEM_ERROR;
    }

    if (shmctl(sharedMemID, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        retVal = SHARED_MEM_ERROR;
    }

    return retVal;
}


/*
* Function:     getSharedData
* Purpose:      Retrieves the shared data from shared memory.
*
* Inputs:       int             sharedMemID     Shared memory ID.
*
* Outputs:      None
*
* Returns:      SharedData*                     Pointer to shared data.
*/
SharedData* getSharedData(int sharedMemID)
{
    SharedData* sharedData;
    sharedData = (SharedData *)shmat (sharedMemID, NULL, 0);
	
    return sharedData;
}


/*
* Function:     findThreadIDInList
* Purpose:      Searches for a client thread ID in the connected clients list.
*               NOTE: Make sure to lock and unlock mutex before and after calling this function!
*
* Inputs:       pthread_t           threadID            The thread ID of the client to find.
*               SharedData*         sharedDataP         Pointer to the shared data structure containing the list of connected clients.
*
* Outputs:      None
*
* Returns:      int                                     The index of the client in the list if found, or ENTRY_NOT_FOUND_OR_NULL if not found or sharedDataP is NULL.
*/
int findThreadIDInList(pthread_t threadID, SharedData* sharedDataP)
{
    // Check for null pointers
    if (sharedDataP == NULL) {
        return ENTRY_NOT_FOUND_OR_NULL;
    }

    int foundIndex = ENTRY_NOT_FOUND_OR_NULL;

    for (int i = 0; i < sharedDataP->numClients; i++)
    {
        if (sharedDataP->connectedClients[i].threadID == threadID)
        {
            // Match found
            foundIndex = i;
            break;
        }
    }

    return foundIndex;
}


/*
* Function:     findUserInList
* Purpose:      Finds the index of a given client in the client list. 
*               NOTE: Make sure to lock and unlock mutex before and after calling this function!
*
* Inputs:       const char*     clientIP        Client IP C-string
*               const char*     clientUserID    Client user ID C-string
*               SharedData*     sharedDataP     Pointer to shared data
*
* Outputs:      None
*
* Returns:      int                         Index of the client in the client list if found, otherwise PID_NOT_FOUND.
*/
int findUserInList(const char* clientIP, const char* clientUserID, SharedData* sharedDataP)
{
    // Check for null pointers
    if (clientIP == NULL || clientUserID == NULL || sharedDataP == NULL) {
        return ENTRY_NOT_FOUND_OR_NULL;
    }

    int foundIndex = ENTRY_NOT_FOUND_OR_NULL;

    for (int i = 0; i < sharedDataP->numClients; i++)
    {
        if (strncmp(sharedDataP->connectedClients[i].clientIP, clientIP, CLIENT_IP_LENGTH) == 0 &&
            strncmp(sharedDataP->connectedClients[i].clientUserID, clientUserID, CLIENT_USERID_LENGTH) == 0)
        {
            // Match found
            foundIndex = i;
            break;
        }
    }

    return foundIndex;
}


/*
* Function:     addToList
* Purpose:      Adds a new client to the list of connected clients in the shared data structure.
*               NOTE: Make sure to lock and unlock mutex before and after calling this function!
* Inputs:       pthread_t           threadID            The thread ID of the client to add.
*               const char*         clientIP            The IP address of the client.
*               const char*         clientUserID        The user ID of the client.
*               int                 clientSocket        The socket associated with the client.
*               SharedData*         sharedDataP         Pointer to the shared data structure.
*
* Outputs:      None
*
* Returns:      int                                     SUCCESS if the client is added successfully,
*                                                       TOO_MANY_CLIENTS if the maximum number of clients is reached,
*                                                       ENTRY_NOT_FOUND_OR_NULL if clientIP, clientUserID, or sharedDataP is NULL.
*/
int addToList(pthread_t threadID, const char* clientIP, const char* clientUserID, int clientSocket, SharedData* sharedDataP)
{
    // Check for null pointers
    if (clientIP == NULL || clientUserID == NULL || sharedDataP == NULL) {
        return ENTRY_NOT_FOUND_OR_NULL;
    }

    int retVal = SUCCESS;
    
    int currentIndex = sharedDataP->numClients;

    if (currentIndex < MAX_CLIENTS)
    {
        // Copy IP
        strncpy(sharedDataP->connectedClients[currentIndex].clientIP, clientIP, sizeof(sharedDataP->connectedClients[currentIndex].clientIP) - 1);
        sharedDataP->connectedClients[currentIndex].clientIP[sizeof(sharedDataP->connectedClients[currentIndex].clientIP) - 1] = '\0'; // Ensure null-termination

        // Copy user ID
        strncpy(sharedDataP->connectedClients[currentIndex].clientUserID, clientUserID, sizeof(sharedDataP->connectedClients[currentIndex].clientUserID) - 1);
        sharedDataP->connectedClients[currentIndex].clientUserID[sizeof(sharedDataP->connectedClients[currentIndex].clientUserID) - 1] = '\0'; // Ensure null-termination
        
        // Copy socket
        sharedDataP->connectedClients[currentIndex].clientSocket = clientSocket;

        // Copy thread ID
        sharedDataP->connectedClients[currentIndex].threadID = threadID;

        // Update number of DCs
        sharedDataP->numClients++;

    }
    else
    {
        retVal = TOO_MANY_CLIENTS;
    }

    return retVal;
}


/*
 * Function:     removeFromList
 * Purpose:      Removes a client from the client list.
 *               NOTE: Make sure to lock and unlock mutex before and after calling this function!
 * Inputs:       int             entryIndex      Index of the client to be removed.
 *               SharedData*     sharedDataP     Pointer to shared data
 *
 * Outputs:      sharedDataP                     Updates the shared data.
 *
 * Returns:      int                             Index of the removed Data Creator in the master list or PID_NOT_FOUND if not found.
 */
int removeFromList(int entryIndex, SharedData* sharedDataP)
{
    if (entryIndex != ENTRY_NOT_FOUND_OR_NULL)
    {
        // Entry exists, so remove by shifting all entries to the right, to the left by one,
        // starting at the entry's index. 

        // First decrement number of clients
        sharedDataP->numClients--;

        // This will copy the contents of the next entry up until the penultimate.
        // The caveat is that the entry that used to be the last is still populated, but
        // so long as the numClients is accurate, this should not cause any issues.
        for (int i = entryIndex; i < sharedDataP->numClients; i++)
        {
            sharedDataP->connectedClients[i] = sharedDataP->connectedClients[i+1];
        }

        // Clean up straggler
        int numClients = sharedDataP->numClients;
        if (numClients < MAX_CLIENTS)
        {
            sharedDataP->connectedClients[numClients].clientIP[0] = 0;
            sharedDataP->connectedClients[numClients].clientUserID[0] = 0;
            sharedDataP->connectedClients[numClients].threadID = 0;
            sharedDataP->connectedClients[numClients].clientSocket = 0;
        }
    }
    else
    {
        return ENTRY_NOT_FOUND_OR_NULL;
    }

    return entryIndex;
}


/*
 * Function:     printSharedData
 * Purpose:      Prints the contents of the shared data.
 *               NOTE: Make sure to lock and unlock mutex before and after calling this function!
 *
 * Inputs:       SharedData*    sharedDataP     Pointer to the shared data.
 *
 * Outputs:      None
 *
 * Returns:      void
 */
void printSharedData(SharedData* sharedDataP)
{
    printf("\nMessage queue ID: %d  |  # of clients: %d\nAll clients:\n", sharedDataP->msgQueueID, sharedDataP->numClients);
    for (int i = 0; i < sharedDataP->numClients; i++)
    {
        printf("\tThread ID: %lu  |  IP: %s  |  UserID: %s\n", 
            sharedDataP->connectedClients[i].threadID, 
            sharedDataP->connectedClients[i].clientIP, 
            sharedDataP->connectedClients[i].clientUserID);
    }

    printf("\n");
}
