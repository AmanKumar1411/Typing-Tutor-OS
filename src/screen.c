#include "screen.h"

#include <stdio.h>
#include <unistd.h>

void screen_clear(void) {
    if (isatty(STDOUT_FILENO)) {
        printf("\033[H\033[2J\033[3J");
    }
}

void screen_render(
    const char *target,
    const char *input,
    size_t input_len,
    double elapsed_seconds,
    double accuracy,
    double wpm,
    size_t correct_chars,
    int finished
) {
    size_t i;
    size_t errors;
    int use_color;

    screen_clear();
    use_color = isatty(STDOUT_FILENO);
    errors = (input_len >= correct_chars) ? (input_len - correct_chars) : 0;

    printf("=== Real Time Typing Tutor (Custom C Libraries) ===\n\n");
    printf("Type the text below. Press ENTER to finish or ESC to quit.\n\n");

    printf("Target:\n%s\n\n", target);
    printf("Your Input:\n");
    for (i = 0; i < input_len; i++) {
        int is_correct = (target[i] != '\0' && input[i] == target[i]);
        if (use_color) {
            printf("%s%c", is_correct ? "\033[32m" : "\033[31m", input[i]);
        } else {
            printf("%c", input[i]);
        }
    }
    if (use_color) {
        printf("\033[0m");
    }
    printf("\n\n");

    printf("Stats:\n");
    printf("Time: %.2f sec\n", elapsed_seconds);
    printf("Correct Chars: %zu\n", correct_chars);
    printf("Errors: %zu\n", errors);
    printf("Accuracy: %.2f%%\n", accuracy);
    printf("WPM: %.2f\n", wpm);

    if (finished) {
        printf("\nSession complete. Press any key to exit.\n");
    }

    fflush(stdout);
}
