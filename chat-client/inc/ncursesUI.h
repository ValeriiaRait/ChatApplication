/*
* Filename:		outp_inp_handlers.h
* Project:		CHAT-SYSTEM/chat-client
* By:			Valeriia Rait
* Date:			March 27, 2024
* Description:  This C file contains implementation for input/output action fromthe client side
*/
#include "chatClient.h"


/**
 * Function:       init_ncurses
 * Description:    initializing the ncurses library for creating a text-based user interface in the
 *                 terminal, setting up the terminal to handle keyboard input and output
 *                 display more flexibly. two main windows using ncurses: one for input
 *                 at the bottom of the terminal and another for output at the top.
 *
 * Inputs:         None
 *
 * Outputs:        windoes for input and output
 * Returns:        None
 */
void init_ncurses() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    
    // Initialize input window at the bottom
    input_win = create_newwin(3, COLS - 2, LINES - 3, 1);
    //scrollok(input_win, TRUE);
    
    // Initialize output window at the top
    output_win = create_newwin(LINES - 4, COLS - 2, 0, 1);
    scrollok(output_win, TRUE);
}



/**
 * Function:       create_newwin
 * Description:    creating and initializing a new window for the ncurses interface,
 *                 allocating new window with the specified dimensions and position, then draws a
 *                 border around it. used by `init_ncurses` to create both the input and
 *                 output windows for the chat application.
 *
 * Inputs:
 *   int height -  height of the new window.
 *   int width -   width of the new window.
 *   int starty -   y-coordinate of the upper left corner of the new window.
 *   int startx -   x-coordinate of the upper left corner of the new window.
 *
 * Outputs:        new window is created and initialized
 *
 * Returns:
 *   WINDOW* - A pointer to the newly created ncurses window
 */
WINDOW *create_newwin(int height, int width, int starty, int startx) {
     WINDOW *local_win;
    local_win = newwin(height, width, starty, startx);
    box(local_win, 0, 0);
    wrefresh(local_win);
    return local_win;
}