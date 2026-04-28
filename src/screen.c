#include "screen.h"

#include <stdio.h>
#include <unistd.h>

static int screen_use_color(void) {
    return isatty(STDOUT_FILENO);
}

static char ascii_lower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return (char)(c + ('a' - 'A'));
    }

    return c;
}

static int is_word_compare_char(char c) {
    return (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9');
}

static int chars_match_for_flow(char left, char right) {
    return ascii_lower(left) == ascii_lower(right);
}

static int word_segments_match(const char *target, size_t target_len, const char *input, size_t input_len) {
    size_t target_index = 0;
    size_t input_index = 0;

    while (target_index < target_len || input_index < input_len) {
        while (target_index < target_len && !is_word_compare_char(target[target_index])) {
            target_index++;
        }

        while (input_index < input_len && !is_word_compare_char(input[input_index])) {
            input_index++;
        }

        if (target_index == target_len || input_index == input_len) {
            break;
        }

        if (!chars_match_for_flow(target[target_index], input[input_index])) {
            return 0;
        }

        target_index++;
        input_index++;
    }

    while (target_index < target_len && !is_word_compare_char(target[target_index])) {
        target_index++;
    }

    while (input_index < input_len && !is_word_compare_char(input[input_index])) {
        input_index++;
    }

    return target_index == target_len && input_index == input_len;
}

static size_t compare_char_index_at(const char *text, size_t len, size_t position) {
    size_t i;
    size_t compare_index = 0;

    for (i = 0; i < position && i < len; i++) {
        if (is_word_compare_char(text[i])) {
            compare_index++;
        }
    }

    return compare_index;
}

static int get_compare_char_at(const char *text, size_t len, size_t compare_index, char *out) {
    size_t i;
    size_t current_index = 0;

    for (i = 0; i < len; i++) {
        if (!is_word_compare_char(text[i])) {
            continue;
        }

        if (current_index == compare_index) {
            *out = text[i];
            return 1;
        }

        current_index++;
    }

    return 0;
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

    printf("\n\nSelect Difficulty (press E / M / h):\n");

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
    printf("- H: view history\n");
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
    int use_color = screen_use_color();

    start_word = stats->word_window_start_index;
    end_word = start_word + SCREEN_VISIBLE_WORD_COUNT;

    printf("\033[H");

    if (use_color) {
        printf("\033[36m%.1fs \033[0m", stats->remaining_seconds);
        printf("\033[35m%.2f WPM \033[0m", stats->wpm);
        printf("\033[33m%.2f%% Acc \033[0m", stats->accuracy);
        printf("\033[2m(PB: %.2f)\033[0m", stats->personal_best_wpm);
    } else {
        printf("%.1fs | %.2f WPM | %.2f%% Acc | PB: %.2f",
               stats->remaining_seconds,
               stats->wpm,
               stats->accuracy,
               stats->personal_best_wpm);
    }
    printf("\033[K\n\n");

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
                int is_correct = word_segments_match(
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
                size_t i;
                for (i = 0; i < target_word_len; i++) {
                    char target_char = target_stream[target_word_start + i];

                    if (is_word_compare_char(target_char)) {
                        char input_char;
                        size_t compare_index = compare_char_index_at(
                            target_stream + target_word_start,
                            target_word_len,
                            i
                        );

                        if (get_compare_char_at(input + input_word_start, input_word_len, compare_index, &input_char)) {
                            if (chars_match_for_flow(input_char, target_char)) {
                                if (use_color) printf("\033[32m%c\033[0m", target_char);
                                else printf("%c", target_char);
                            } else {
                                if (use_color) printf("\033[31m%c\033[0m", target_char);
                                else printf("%c", target_char);
                            }
                        } else {
                            if (use_color) printf("\033[2m%c\033[0m", target_char);
                            else printf("%c", target_char);
                        }
                    } else if (i < input_word_len && input[input_word_start + i] == target_char) {
                        if (use_color) printf("\033[32m%c\033[0m", target_char);
                        else printf("%c", target_char);
                    } else {
                        if (use_color) printf("\033[2m%c\033[0m", target_char);
                        else printf("%c", target_char);
                    }
                }
            } else {
                if (use_color) {
                    printf("\033[2m%.*s\033[0m", (int)target_word_len, target_stream + target_word_start);
                } else {
                    printf("%.*s", (int)target_word_len, target_stream + target_word_start);
                }
            }

            printed_words++;
            if (printed_words > 0 && printed_words % SCREEN_WORDS_PER_LINE == 0) {
                printf("\033[K\n");
            }
        }

        word_index++;
    }

    printf("\033[J");
    fflush(stdout);
}

void screen_render_summary(const ScreenSummaryStats *summary) {
    int use_color = screen_use_color();

    screen_clear();

    printf("==================== Session Summary ====================\n\n");

    printf("Session #%d (%d sec, %s)\n\n", summary->session_number, summary->time_mode_seconds, "Medium");

    printf("  WPM: %.2f\n", summary->final_wpm);
    printf("  Acc: %.2f%%\n", summary->final_accuracy);
    printf("  Words: %zu (%zu correct, %zu incorrect)\n", summary->total_words_typed, summary->correct_words, summary->incorrect_words);

    if (summary->is_new_personal_best && use_color) {
        printf("\n  \033[32mNew Personal Best WPM!\033[0m\n");
    } else {
        printf("\n  Personal Best: %.2f WPM\n", summary->personal_best_wpm);
    }

    printf("\n------------------------------------------------------\n");
    printf("  [TAB] Restart  |  [ENTER] Setup  |  [H] History\n");
    printf("  [D] Dashboard  |  [ESC] Quit\n");
    printf("------------------------------------------------------\n");

    fflush(stdout);
}

void screen_render_history(const SessionRecord *history, size_t history_count, int current_page) {
    size_t i;
    size_t page_size = 10;
    size_t total_pages = history_count == 0 ? 1 : (history_count + page_size - 1) / page_size;
    size_t start_index = current_page * page_size;
    size_t end_index = start_index + page_size;
    if (end_index > history_count) {
        end_index = history_count;
    }

    screen_clear();
    printf("==================== Session History ====================\n\n");

    if (history_count == 0) {
        printf("  No sessions completed yet.\n");
    } else {
        printf("  %-5s %-8s %-10s %-10s %-10s %-10s\n", "#", "Time", "Diff", "WPM", "Acc", "Words");
        printf("  ------------------------------------------------------\n");
        for (i = start_index; i < end_index; i++) {
            const SessionRecord *r = &history[i];
            printf("  %-5d %-8d %-10s %-10.2f %-10.2f %-10zu\n",
                   r->session_number,
                   r->time_mode_seconds,
                   r->difficulty_name,
                   r->final_wpm,
                   r->final_accuracy,
                   r->total_words_typed);
        }
    }

    printf("\n------------------------------------------------------\n");
    printf("  Page %d/%zu | [N] Next | [P] Prev | [B] Back\n", current_page + 1, total_pages);
    printf("------------------------------------------------------\n");
    fflush(stdout);
}

void screen_render_dashboard(const PerformanceDashboard *dashboard) {
    screen_clear();
    printf("==================== Performance Dashboard ====================\n\n");

    printf("  Total Sessions: %zu\n", dashboard->total_sessions_completed);
    printf("  Total Time Typing: %.1f seconds\n", dashboard->total_time_typing_seconds);
    printf("  Total Words Typed: %zu\n", dashboard->total_words_typed);
    printf("\n");
    printf("  Best WPM Ever: %.2f\n", dashboard->best_wpm_ever);
    printf("  Best Accuracy Ever: %.2f%%\n", dashboard->best_accuracy_ever);
    printf("\n");
    printf("  Average WPM: %.2f\n", dashboard->average_wpm);
    printf("  Average Accuracy: %.2f%%\n", dashboard->average_accuracy);


    printf("\n------------------------------------------------------\n");
    printf("  [B] Back to Summary\n");
    printf("------------------------------------------------------\n");
    fflush(stdout);
}
