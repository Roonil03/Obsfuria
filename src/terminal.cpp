#include "terminal.hpp"
#include <unistd.h>
#include <cstdlib>
#include <iostream>

struct termios Terminal::orig_termios;
bool Terminal::is_raw_mode = false;

/*
 * Enables raw terminal mode.
 */
void Terminal::enableRawMode() {
    if (is_raw_mode) return;
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_cflag |= (CS8);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    is_raw_mode = true;
}

/*
 * Restores original terminal settings.
 */
void Terminal::disableRawMode() {
    if (!is_raw_mode) return;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    is_raw_mode = false;
}

/*
 * Reads a single keystroke, interpreting ANSI escapes for arrows.
 */
int Terminal::readKey() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1) exit(1);
    }
    
    if (c == '\x1b') {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
        
        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return ARROW_UP;
                case 'B': return ARROW_DOWN;
                case 'C': return ARROW_RIGHT;
                case 'D': return ARROW_LEFT;
            }
        }
        return '\x1b';
    } else {
        return c;
    }
}

/*
 * Clears the screen and resets cursor using ANSI escapes.
 */
void Terminal::clearScreen() {
    std::cout << "\x1b[2J\x1b[H";
    std::cout.flush();
}

/*
 * Moves cursor to (x, y) 1-indexed.
 */
void Terminal::moveCursor(int x, int y) {
    std::cout << "\x1b[" << y << ";" << x << "H";
    std::cout.flush();
}
