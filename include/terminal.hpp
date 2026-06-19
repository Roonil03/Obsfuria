#ifndef TERMINAL_HPP
#define TERMINAL_HPP

#include <termios.h>

/*
 * Provides raw terminal I/O, ANSI control, and non-blocking key reads.
 */
class Terminal {
private:
    static struct termios orig_termios;
    static bool is_raw_mode;

public:
    /*
     * Enables raw terminal mode.
     */
    static void enableRawMode();
    
    /*
     * Restores the original terminal mode.
     */
    static void disableRawMode();
    
    /*
     * Reads a single character from stdin (blocking). Returns special codes for arrows.
     */
    static int readKey();
    
    /*
     * Clears the terminal screen.
     */
    static void clearScreen();
    
    /*
     * Moves the cursor to x, y (1-indexed).
     */
    static void moveCursor(int x, int y);
    
    enum KeyCodes {
        ARROW_UP = 1000,
        ARROW_DOWN,
        ARROW_RIGHT,
        ARROW_LEFT,
        ENTER = 13,
        ESC = 27
    };
};

#endif // TERMINAL_HPP
