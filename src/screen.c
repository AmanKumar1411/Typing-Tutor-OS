#include "screen.h"

#include <stdio.h>
#include <unistd.h>

#define WINDOW_WORD_COUNT 32
#define WORDS_PER_LINE 8

static int screen_use_color(void) {
    return isatty(STDOUT_FILENO);
}

static int segment_equals(const char *left, size_t left_len, const char *right, size_t right_len) {
    size_t i;

    if (left_len != right_len) {
        return 0;
    }

    for (i = 0; i < left_len; i++) {
        if (left[i] != right[i]) {
            return 0;
        }
    }

    return 1;
}

void screen_clear(void) {
    if (isatty(STDOUT_FILENO)) {
        printf("\033[H\033[J");
    }
}

void screen_render_setup(size_t selected_time_index, int selected_difficulty, const int *time_options, size_t time_option_count) {
    size_t i;
    int use_color = screen_use_color();

    screen_clear();

    printf("====================== Typing Tutor ======================\n");
    printf("         Monkeytype-Inspired Terminal Practice\n");
    printf("==========================================================\n\n");

    printf("Select Time Mode (press 1-5):\n");
    for (i = 0; i < time_option_count; i++) {
        int is_selected = (i == selected_time_index);
        if (is_selected && use_color) {
            printf("\033[1;30;46m %zu) %d sec \033[0m  ", i + 1, time_options[i]);
        } else if (is_selected) {
            printf("[%zu) %d sec]  ", i + 1, time_options[i]);
        } else {
            printf(" %zu) %d sec   ", i + 1, time_options[i]);
        }
    }

    printf("\n\nSelect Difficulty (press E / M / H):\n");

    if (selected_difficulty == 0 && use_color) {
        printf("\033[1;30;42m Easy \033[0m  ");
    } else if (selected_difficulty == 0) {
        printf("[Easy]  ");
    } else {
        printf(" Easy   ");
    }

    if (selected_difficulty == 1 && use_color) {
        printf("\033[1;30;43m Medium \033[0m  ");
    } else if (selected_difficulty == 1) {
        printf("[Medium]  ");
    } else {
        printf(" Medium   ");
    }

    if (selected_difficulty == 2 && use_color) {
        printf("\033[1;37;41m Hard \033[0m\n");
    } else if (selected_difficulty == 2) {
        printf("[Hard]\n");
    } else {
        printf(" Hard\n");
    }

    printf("\nControls:\n");
    printf("- ENTER: start test with selected settings\n");
    printf("- ESC: quit application\n");
    printf("\nTimer starts only on your first valid key press.\n");

    fflush(stdout);
}

void screen_render_typing(const char *target_stream, const char *input, size_t input_len, const ScreenLiveStats *stats) {
    size_t target_pos = 0;
    size_t input_pos = 0;
    size_t word_index = 0;
    size_t printed_words = 0;
    size_t start_word;
    size_t end_word;
    size_t tail_start;
    int use_color = screen_use_color();

    if (stats->active_word_index > 6) {
        start_word = stats->active_word_index - 6;
    } else {
        start_word = 0;
    }
    end_word = start_word + WINDOW_WORD_COUNT;

    tail_start = 0;
    if (input_len > 90) {
        tail_start = input_len - 90;
    }

    screen_clear();

    printf("==================== Typing Tutor ====================\n");
    printf("Mode: %d sec  |  Difficulty: %s  |  ESC Quit  |  TAB Restart\n",
           stats->selected_time_seconds,
           stats->difficulty_name);
    printf("------------------------------------------------------\n");
    printf("Time: %.1fs elapsed | %.1fs left | WPM: %.2f | Accuracy: %.2f%%\n",
           stats->elapsed_seconds,
           stats->remaining_seconds,
           stats->wpm,
           stats->accuracy);
    printf("Chars: %zu correct / %zu errors | Words: %zu total (%zu ok, %zu wrong)\n",
           stats->correct_chars,
           stats->errors,
           stats->total_words_typed,
           stats->correct_words,
           stats->incorrect_words);
    printf("Streak: current %zu | best %zu\n",
           stats->current_streak,
           stats->best_streak);
    printf("------------------------------------------------------\n\n");

    printf("Word Stream:\n");

    while (target_stream[target_pos] != '\0' && word_index < end_word) {
        size_t target_word_start = target_pos;
        size_t target_word_len;
        size_t input_word_start = input_pos;
        size_t input_word_len;
        int input_word_completed;

        while (target_stream[target_pos] != '\0' && target_stream[target_pos] != ' ') {
            target_pos++;
        }
        target_word_len = target_pos - target_word_start;
        if (target_stream[target_pos] == ' ') {
            target_pos++;
        }

        while (input_pos < input_len && input[input_pos] != ' ') {
            input_pos++;
        }
        input_word_len = input_pos - input_word_start;
        input_word_completed = (input_pos < input_len && input[input_pos] == ' ');
        if (input_word_completed) {
            input_pos++;
        }

        if (word_index >= start_word) {
            int is_completed_word = (word_index < stats->active_word_index);
            int is_active_word = (word_index == stats->active_word_index);

            if (printed_words > 0) {
                printf(" ");
            }

            if (is_completed_word) {
                int is_correct = segment_equals(
                    target_stream + target_word_start,
                    target_word_len,
                    input + input_word_start,
                    input_word_len
                );

                if (use_color && is_correct) {
                    printf("\033[32m%.*s\033[0m", (int)target_word_len, target_stream + target_word_start);
                } else if (use_color) {
                    printf("\033[31m%.*s\033[0m", (int)target_word_len, target_stream + target_word_start);
                } else if (is_correct) {
                    printf("+%.*s", (int)target_word_len, target_stream + target_word_start);
                } else {
                    printf("-%.*s", (int)target_word_len, target_stream + target_word_start);
                }
            } else if (is_active_word) {
                if (use_color) {
                    printf("\033[1;30;43m%.*s\033[0m", (int)target_word_len, target_stream + target_word_start);
                } else {
                    printf("[%.*s]", (int)target_word_len, target_stream + target_word_start);
                }
            } else {
                if (use_color) {
                    printf("\033[2m%.*s\033[0m", (int)target_word_len, target_stream + target_word_start);
                } else {
                    printf("%.*s", (int)target_word_len, target_stream + target_word_start);
                }
            }

            printed_words++;
            if (printed_words % WORDS_PER_LINE == 0) {
                printf("\n");
            }
        }

        word_index++;
    }

    if (printed_words % WORDS_PER_LINE != 0) {
        printf("\n");
    }

    printf("\nYour Input:\n");
    if (tail_start > 0) {
        printf("...");
    }

    if (input_len > tail_start) {
        printf("%.*s", (int)(input_len - tail_start), input + tail_start);
    }

    if (use_color) {
        printf("\033[36m|\033[0m");
    } else {
        printf("|");
    }

    printf("\n\n");
    printf("Footer: timer starts on first key | smooth real-time stream typing\n");

    fflush(stdout);
}

void screen_render_summary(const ScreenSummaryStats *summary) {
    int use_color = screen_use_color();

    screen_clear();

    printf("==================== Session Summary ====================\n\n");

    if (summary->timed_out) {
        printf("Result: Time limit reached successfully.\n\n");
    } else {
        printf("Result: Session finished before timer end.\n\n");
    }

    printf("Final WPM: %.2f\n", summary->final_wpm);
    printf("Final Accuracy: %.2f%%\n", summary->final_accuracy);
    printf("Total Words Typed: %zu\n", summary->total_words_typed);
    printf("Correct Words: %zu\n", summary->correct_words);
    printf("Incorrect Words: %zu\n", summary->incorrect_words);
    printf("Current Streak: %zu\n", summary->current_streak);
    printf("Best Streak: %zu\n", summary->best_streak);

    if (summary->is_new_personal_best && use_color) {
        printf("Personal Best WPM: \033[32m%.2f (NEW BEST)\033[0m\n", summary->personal_best_wpm);
    } else if (summary->is_new_personal_best) {
        printf("Personal Best WPM: %.2f (NEW BEST)\n", summary->personal_best_wpm);
    } else {
        printf("Personal Best WPM: %.2f\n", summary->personal_best_wpm);
    }

    printf("\nControls:\n");
    printf("- TAB: restart instantly with same settings\n");
    printf("- ENTER: continue to setup screen\n");
    printf("- ESC: quit\n");
    printf("- Any other key: restart with same settings\n");

    fflush(stdout);
}
