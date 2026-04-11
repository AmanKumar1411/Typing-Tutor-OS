#include "keyboard.h"

#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

static struct termios original_termios;
static int original_flags = 0;
static int keyboard_ready = 0;

int keyboard_init(void) {
    struct termios raw;
    int flags;

    if (!isatty(STDIN_FILENO)) {
        return 0;
    }

    if (tcgetattr(STDIN_FILENO, &original_termios) == -1) {
        return 0;
    }

    raw = original_termios;
    raw.c_iflag &= (tcflag_t) ~(ICRNL | IXON);
    raw.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1) {
        return 0;
    }

    flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags == -1) {
        tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
        return 0;
    }
    original_flags = flags;

    if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1) {
        tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
        return 0;
    }

    keyboard_ready = 1;
    return 1;
}

void keyboard_shutdown(void) {
    if (keyboard_ready) {
        tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
        fcntl(STDIN_FILENO, F_SETFL, original_flags);
        keyboard_ready = 0;
    }
}

int keyboard_read_char_nonblocking(void) {
    unsigned char ch;
    ssize_t bytes_read;

    do {
        bytes_read = read(STDIN_FILENO, &ch, 1);
    } while (bytes_read < 0 && errno == EINTR);

    if (bytes_read == 1) {
        return (int)ch;
    }

    return KEY_NONE;
}
