#include "keyboard.h"
#include "math_utils.h"
#include "memory.h"
#include "screen.h"
#include "string_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

static double time_now_seconds(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + ((double)tv.tv_usec / 1000000.0);
}

static char *load_text_file(const char *path) {
    FILE *fp = fopen(path, "r");
    size_t capacity = 128;
    size_t len = 0;
    int ch;
    char *buffer;

    if (fp == NULL) {
        return NULL;
    }

    buffer = (char *)mem_alloc(capacity);
    buffer[0] = '\0';

    while ((ch = fgetc(fp)) != EOF) {
        if (len + 1 >= capacity) {
            size_t new_capacity = capacity * 2;
            buffer = (char *)mem_resize(buffer, new_capacity);
            mem_set(buffer + capacity, '\0', new_capacity - capacity);
            capacity = new_capacity;
        }

        if (ch == '\n' || ch == '\r' || ch == '\t') {
            ch = ' ';
        }

        buffer[len] = (char)ch;
        len++;
        buffer[len] = '\0';
    }

    fclose(fp);

    if (len == 0) {
        mem_free(buffer);
        return NULL;
    }

    return buffer;
}

int main(int argc, char **argv) {
    const char *default_text = "systems programming in c requires precision and practice";
    char *loaded_text = NULL;
    const char *target_text = default_text;
    size_t target_len;
    size_t input_capacity;
    char *input_buffer;
    size_t input_len = 0;
    double start_time = 0.0;
    double current_time;
    int finished = 0;
    int timer_started = 0;
    int exit_by_escape = 0;

    if (argc > 1) {
        loaded_text = load_text_file(argv[1]);
        if (loaded_text != NULL) {
            target_text = loaded_text;
        }
    }

    target_len = str_length(target_text);
    input_capacity = target_len + 2;
    input_buffer = (char *)mem_alloc(input_capacity);
    mem_set(input_buffer, '\0', input_capacity);

    if (!keyboard_init()) {
        fprintf(stderr, "failed to initialize keyboard\n");
        mem_free(input_buffer);
        mem_free(loaded_text);
        return 1;
    }

    while (!finished) {
        int key = keyboard_read_char_nonblocking();

        if (key != KEY_NONE) {
            if (key == KEY_ESC) {
                finished = 1;
                exit_by_escape = 1;
            } else if (key == 3) {
                finished = 1;
                exit_by_escape = 1;
            } else if (key == KEY_ENTER || key == '\n') {
                finished = 1;
            } else if (key == KEY_BACKSPACE || key == KEY_DELETE) {
                if (input_len > 0) {
                    input_len--;
                    input_buffer[input_len] = '\0';
                }
            } else if (key >= 32 && key <= 126) {
                size_t type_limit = math_min_size(target_len, input_capacity - 1);

                if (input_len + 1 >= input_capacity) {
                    size_t new_capacity = input_capacity * 2;
                    input_buffer = (char *)mem_resize(input_buffer, new_capacity);
                    mem_set(input_buffer + input_capacity, '\0', new_capacity - input_capacity);
                    input_capacity = new_capacity;
                }

                if (input_len < type_limit) {
                    input_buffer[input_len] = (char)key;
                    input_len++;
                    input_buffer[input_len] = '\0';
                    if (!timer_started) {
                        start_time = time_now_seconds();
                        timer_started = 1;
                    }
                }
            }
        }

        current_time = time_now_seconds();
        {
            double elapsed = timer_started ? (current_time - start_time) : 0.0;
            size_t correct = str_count_correct(target_text, input_buffer, input_len);
            double accuracy = math_percent(correct, input_len);
            double wpm = math_wpm(input_len, elapsed);
            screen_render(target_text, input_buffer, input_len, elapsed, accuracy, wpm, correct, 0);

            if (input_len >= target_len && str_equals(target_text, input_buffer)) {
                screen_render(target_text, input_buffer, input_len, elapsed, accuracy, wpm, correct, 1);
                finished = 1;
            }
        }

        usleep(10000);
    }

    keyboard_shutdown();

    {
        double total_elapsed = timer_started ? (time_now_seconds() - start_time) : 0.0;
        size_t correct_total = str_count_correct(target_text, input_buffer, input_len);
        double final_accuracy = math_percent(correct_total, input_len);
        double final_wpm = math_wpm(input_len, total_elapsed);

        screen_render(
            target_text,
            input_buffer,
            input_len,
            total_elapsed,
            final_accuracy,
            final_wpm,
            correct_total,
            1
        );
    }

    if (!exit_by_escape) {
        while (keyboard_read_char_nonblocking() == KEY_NONE) {
            usleep(10000);
        }
    }

    mem_free(input_buffer);
    mem_free(loaded_text);
    return 0;
}
