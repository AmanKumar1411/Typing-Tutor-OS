#ifndef SCREEN_H
#define SCREEN_H

#include <stdbool.h>
#include <stddef.h>

#define MAX_SESSION_HISTORY 100
#define SCREEN_WORDS_PER_LINE 8
#define SCREEN_VISIBLE_LINES 4
#define SCREEN_VISIBLE_WORD_COUNT (SCREEN_WORDS_PER_LINE * SCREEN_VISIBLE_LINES)
#define SESSION_DIFFICULTY_NAME_SIZE 16

typedef struct ScreenLiveStats {
    double remaining_seconds;
    double accuracy;
    double wpm;
    double personal_best_wpm;
    size_t active_word_index;
    size_t word_window_start_index;
} ScreenLiveStats;

typedef struct SessionRecord {
    int session_number;
    int time_mode_seconds;
    char difficulty_name[SESSION_DIFFICULTY_NAME_SIZE];
    double final_wpm;
    double final_accuracy;
    size_t total_words_typed;
    size_t correct_words;
    size_t incorrect_words;
    double session_duration_seconds;
} SessionRecord;

typedef struct PerformanceDashboard {
    size_t total_sessions_completed;
    double total_time_typing_seconds;
    size_t total_words_typed;
    double best_wpm_ever;
    double best_accuracy_ever;
    double average_wpm;
    double average_accuracy;
} PerformanceDashboard;

typedef struct ScreenSummaryStats {
    double final_wpm;
    double final_accuracy;
    size_t total_words_typed;
    size_t correct_words;
    size_t incorrect_words;
    double personal_best_wpm;
    int is_new_personal_best;
    int session_number;
    int time_mode_seconds;
} ScreenSummaryStats;

void screen_clear(void);
void screen_render_setup(size_t selected_time_index, int selected_difficulty, const int *time_options, size_t time_option_count);
void screen_render_typing(const char *target_stream, const char *input, size_t input_len, const ScreenLiveStats *stats);
void screen_render_summary(const ScreenSummaryStats *summary);
void screen_render_history(const SessionRecord *history, size_t history_count, int current_page);
void screen_render_dashboard(const PerformanceDashboard *dashboard);

#endif
