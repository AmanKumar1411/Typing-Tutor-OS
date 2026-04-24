#ifndef SCREEN_H
#define SCREEN_H

#include <stddef.h>

typedef struct ScreenLiveStats {
    double elapsed_seconds;
    double remaining_seconds;
    double accuracy;
    double wpm;
    size_t correct_chars;
    size_t errors;
    size_t total_words_typed;
    size_t correct_words;
    size_t incorrect_words;
    size_t current_streak;
    size_t best_streak;
    size_t active_word_index;
    int timer_started;
    int selected_time_seconds;
    const char *difficulty_name;
} ScreenLiveStats;

typedef struct ScreenSummaryStats {
    double final_wpm;
    double final_accuracy;
    size_t total_words_typed;
    size_t correct_words;
    size_t incorrect_words;
    size_t current_streak;
    size_t best_streak;
    double personal_best_wpm;
    int is_new_personal_best;
    int timed_out;
} ScreenSummaryStats;

void screen_clear(void);
void screen_render_setup(size_t selected_time_index, int selected_difficulty, const int *time_options, size_t time_option_count);
void screen_render_typing(const char *target_stream, const char *input, size_t input_len, const ScreenLiveStats *stats);
void screen_render_summary(const ScreenSummaryStats *summary);

#endif
