#include "terminal.hpp"
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <poll.h>
#include <cerrno>
#include <csignal>
#include <deque>

struct termios Terminal::orig_termios;
bool Terminal::is_raw_mode = false;
namespace {
volatile sig_atomic_t interrupted = 0;
bool closed = false;
std::deque<unsigned char> pending;
void onSignal(int signal) { interrupted = signal; }

// Periodically recheck the signal flag, including when a signal arrives just
// before poll(). Never leave the terminal waiting indefinitely after shutdown.
int readByte(int timeout) {
    if (!pending.empty()) {
        const int value = pending.front();
        pending.pop_front();
        return value;
    }
    while (!closed && !interrupted) {
        pollfd input{STDIN_FILENO, POLLIN, 0};
        const int ready = poll(&input, 1, timeout < 0 ? 100 : timeout);
        if (ready == 0) {
            if (timeout >= 0) return -2;
            continue;
        }
        if (ready < 0) {
            if (errno == EINTR) continue;
            closed = true;
            return -1;
        }
        if (interrupted) return -1;
        unsigned char value;
        const ssize_t count = read(STDIN_FILENO, &value, 1);
        if (count == 1) return value;
        if (count < 0 && errno == EINTR) continue;
        closed = true;
    }
    return -1;
}
}

void Terminal::installSignalHandlers() {
    struct sigaction action{};
    action.sa_handler = onSignal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    for (int signal : {SIGINT, SIGTERM, SIGHUP, SIGQUIT}) sigaction(signal, &action, nullptr);
}

bool Terminal::inputClosed() { return closed || interrupted != 0; }

/*
 * Enables raw terminal mode.
 */
bool Terminal::enableRawMode() {
    if (is_raw_mode) return true;
    if (tcgetattr(STDIN_FILENO, &orig_termios) != 0) return false;
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_cflag &= ~(CSIZE | PARENB);
    raw.c_cflag |= CS8;
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0) {
        is_raw_mode = true;
        atexit(disableRawMode);
        return true;
    }
    return false;
}

/*
 * Restores original terminal settings.
 */
void Terminal::disableRawMode() {
    if (!is_raw_mode) return;
    while (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) < 0 && errno == EINTR) {}
    is_raw_mode = false;
}

/*
 * Reads a single keystroke, interpreting ANSI escapes for arrows.
 */
int Terminal::readKey() {
    // POSIX read() does not flush C++ output as std::cin would. Make prompts
    // visible before waiting, even when stdout is fully buffered.
    std::cout.flush();
    if (inputClosed()) return -1;
    const int c = readByte(-1);
    if (c < 0) return -1;
    if (c == 3) {
        interrupted = SIGINT;
        return -1;
    }
    
    if (c == '\x1b') {
        const int prefix = readByte(50);
        if (prefix < 0) return inputClosed() ? -1 : ESC;
        if (prefix != '[' && prefix != 'O') {
            pending.push_front(static_cast<unsigned char>(prefix));
            return ESC;
        }
        // Consume complete CSI/SS3 sequences, including modifier parameters,
        // so unsupported keys cannot become movement or menu input.
        for (int length = 0; length < 16; ++length) {
            const int next = readByte(50);
            if (next < 0) return inputClosed() ? -1 : ESC;
            if (next < 0x40 || next > 0x7e) continue;
            switch (next) {
                case 'A': return ARROW_UP;
                case 'B': return ARROW_DOWN;
                case 'C': return ARROW_RIGHT;
                case 'D': return ARROW_LEFT;
            }
            return 0;
        }
        return 0;
    } else {
        return c == '\n' ? ENTER : c;
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
