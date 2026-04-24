#ifndef KEYBOARD_H
#define KEYBOARD_H

enum KeyCode {
    KEY_NONE = 0,
    KEY_BACKSPACE = 8,
    KEY_TAB = 9,
    KEY_ENTER = 13,
    KEY_ESC = 27,
    KEY_DELETE = 127
};

int keyboard_init(void);
void keyboard_shutdown(void);
int keyboard_read_char_nonblocking(void);

#endif
