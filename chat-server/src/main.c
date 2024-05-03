/*
* Filename:		main.c
* Project:		CHAT-SYSTEM/chat-server
* By:			Ekaterina (Kate) Stroganova
* Date:			March 25, 2024
* Description:  This file contains the main function for starting the chat server.
*/

#include "../inc/chatServer.h"
#include "../inc/serverIPC.h"

int main(int argc, char* argv[])
{
    runServer();
    
    return 0;
}
