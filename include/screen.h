#ifndef SCREEN_H
#define SCREEN_H

#include <stddef.h>

void screen_clear(void);
void screen_render(
    const char *target,
    const char *input,
    size_t input_len,
    double elapsed_seconds,
    double accuracy,
    double wpm,
    size_t correct_chars,
    int finished
);

#endif
