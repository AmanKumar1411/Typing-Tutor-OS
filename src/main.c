#include "keyboard.h"
#include "math_utils.h"
#include "memory.h"
#include "screen.h"
#include "string_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#define INITIAL_TARGET_CAPACITY 1024
#define INITIAL_INPUT_CAPACITY 512
#define INITIAL_WORD_STREAM_WORDS 140
#define EXTEND_WORD_STREAM_WORDS 80
#define TARGET_AHEAD_THRESHOLD 220
#define HISTORY_FILE "typing_history.dat"

typedef enum DifficultyMode {
    DIFFICULTY_EASY = 0,
    DIFFICULTY_MEDIUM = 1,
    DIFFICULTY_HARD = 2
} DifficultyMode;

typedef enum AppState {
    STATE_SETUP,
    STATE_TYPING,
    STATE_SUMMARY,
    STATE_HISTORY,
    STATE_DASHBOARD,
    STATE_QUIT
} AppState;

typedef struct WordProgress {
    size_t active_word_index;
    size_t total_words_typed;
    size_t correct_words;
    size_t incorrect_words;
} WordProgress;

static const int TIME_OPTIONS[] = {15, 30, 60, 120, 300};
static const size_t TIME_OPTION_COUNT = sizeof(TIME_OPTIONS) / sizeof(TIME_OPTIONS[0]);

static const char *const EASY_WORDS[] = {
    "sad", "dad", "ask", "fall", "add", "lad", "glass", "salsa", "jazz", "flask",
    "all", "as", "fad", "salad", "lass", "flag", "half", "dash", "shall", "fads",
    "alkali", "alas", "sassy", "gala", "task", "deal", "flash", "alpha", "ska", "lag"
};

static const char *const MEDIUM_WORDS[] = {
    "system", "memory", "typing", "coding", "terminal", "keyboard", "accuracy", "practice", "session", "logic",
    "module", "function", "buffer", "pointer", "compile", "runtime", "screen", "stream", "status", "result",
    "dynamic", "update", "streak", "summary", "debug", "timing", "process", "output", "input", "layout",
    "engine", "thread", "safety", "design", "review", "metrics", "word", "random", "refresh", "format"
};

static const char *const HARD_WORDS[] = {
    "quantum", "puzzle", "maximum", "complex", "pixel", "vortex", "oxygen", "rhythm", "jackpot", "zigzag",
    "whiskey", "jovial", "quartz", "xylophone", "zombie", "awkward", "jukebox", "mnemonic", "sphinx", "vigilant",
    "cryptic", "pharaoh", "whiplash", "fizzbuzz", "jazzy", "keymix", "waveform", "outjump", "buckshot", "zephyrs",
    "fluency", "backdrop", "wavelength", "hardware", "microchip", "benchmark", "workflow", "jackknife", "highway", "context"
};

static SessionRecord g_session_history[MAX_SESSION_HISTORY];
static size_t g_history_count = 0;
static int g_session_counter = 0;
static ScreenSummaryStats g_last_summary;
static int g_has_last_summary = 0;

static double time_now_seconds(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + ((double)tv.tv_usec / 1000000.0);
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

static const char *difficulty_name(DifficultyMode difficulty) {
    if (difficulty == DIFFICULTY_EASY) {
        return "Easy";
    }
    if (difficulty == DIFFICULTY_HARD) {
        return "Hard";
    }
    return "Medium";
}

static const char *const *get_word_bank(DifficultyMode difficulty, size_t *word_count) {
    if (difficulty == DIFFICULTY_EASY) {
        *word_count = sizeof(EASY_WORDS) / sizeof(EASY_WORDS[0]);
        return EASY_WORDS;
    }

    if (difficulty == DIFFICULTY_HARD) {
        *word_count = sizeof(HARD_WORDS) / sizeof(HARD_WORDS[0]);
        return HARD_WORDS;
    }

    *word_count = sizeof(MEDIUM_WORDS) / sizeof(MEDIUM_WORDS[0]);
    return MEDIUM_WORDS;
}

static void append_random_words(
    char **target_stream,
    size_t *target_len,
    size_t *target_capacity,
    DifficultyMode difficulty,
    size_t words_to_add
) {
    size_t bank_count;
    const char *const *bank = get_word_bank(difficulty, &bank_count);
    size_t i;

    for (i = 0; i < words_to_add; i++) {
        const char *word = bank[rand() % (int)bank_count];
        size_t word_len = str_length(word);
        size_t needed = *target_len + word_len + 2;

        if (needed > *target_capacity) {
            size_t new_capacity = *target_capacity;
            while (needed > new_capacity) {
                new_capacity *= 2;
            }
            *target_stream = (char *)mem_resize(*target_stream, new_capacity);
            mem_set(*target_stream + *target_capacity, '\0', new_capacity - *target_capacity);
            *target_capacity = new_capacity;
        }

        if (*target_len > 0) {
            (*target_stream)[*target_len] = ' ';
            (*target_len)++;
        }

        mem_copy(*target_stream + *target_len, word, word_len);
        *target_len += word_len;
        (*target_stream)[*target_len] = '\0';
    }
}

static void ensure_input_capacity(char **input_buffer, size_t *input_capacity, size_t needed_len) {
    if (needed_len + 1 > *input_capacity) {
        size_t new_capacity = *input_capacity;
        while (needed_len + 1 > new_capacity) {
            new_capacity *= 2;
        }

        *input_buffer = (char *)mem_resize(*input_buffer, new_capacity);
        mem_set(*input_buffer + *input_capacity, '\0', new_capacity - *input_capacity);
        *input_capacity = new_capacity;
    }
}

static void compute_word_progress(
    const char *target_stream,
    const char *input,
    size_t input_len,
    int include_partial_current,
    WordProgress *progress
) {
    size_t target_pos = 0;
    size_t input_pos = 0;

    progress->active_word_index = 0;
    progress->total_words_typed = 0;
    progress->correct_words = 0;
    progress->incorrect_words = 0;

    while (input_pos < input_len) {
        size_t input_word_start = input_pos;
        size_t input_word_len;
        int input_word_completed;
        size_t target_word_start;
        size_t target_word_len;
        int is_correct;

        while (input_pos < input_len && input[input_pos] != ' ') {
            input_pos++;
        }

        input_word_len = input_pos - input_word_start;
        input_word_completed = (input_pos < input_len && input[input_pos] == ' ');

        if (input_word_completed) {
            input_pos++;
        } else if (!include_partial_current) {
            break;
        }

        if (input_word_len == 0) {
            if (input_word_completed) {
                progress->active_word_index++;
            }
            continue;
        }

        while (target_stream[target_pos] == ' ') {
            target_pos++;
        }

        target_word_start = target_pos;
        while (target_stream[target_pos] != '\0' && target_stream[target_pos] != ' ') {
            target_pos++;
        }
        target_word_len = target_pos - target_word_start;
        if (target_stream[target_pos] == ' ') {
            target_pos++;
        }

        is_correct = segment_equals(
            target_stream + target_word_start,
            target_word_len,
            input + input_word_start,
            input_word_len
        );

        progress->total_words_typed++;
        if (is_correct) {
            progress->correct_words++;
        } else {
            progress->incorrect_words++;
        }

        if (input_word_completed) {
            progress->active_word_index++;
        }
    }
}

static void build_live_stats(
    const char *target_stream,
    const char *input,
    size_t input_len,
    double elapsed_seconds,
    int timer_started,
    int selected_time_seconds,
    double personal_best_wpm,
    ScreenLiveStats *stats
) {
    WordProgress progress;
    size_t correct_chars = str_count_correct(target_stream, input, input_len);

    compute_word_progress(target_stream, input, input_len, 0, &progress);

    stats->remaining_seconds = timer_started
        ? ((elapsed_seconds < (double)selected_time_seconds)
            ? ((double)selected_time_seconds - elapsed_seconds)
            : 0.0)
        : (double)selected_time_seconds;
    stats->accuracy = math_percent(correct_chars, input_len);
    stats->wpm = math_wpm(input_len, elapsed_seconds);
    stats->personal_best_wpm = personal_best_wpm;
    stats->active_word_index = progress.active_word_index;
}

static void save_history_to_file(void) {
    FILE *f = fopen(HISTORY_FILE, "wb");
    if (!f) return;
    fwrite(&g_history_count, sizeof(g_history_count), 1, f);
    fwrite(g_session_history, sizeof(SessionRecord), g_history_count, f);
    fclose(f);
}

static void load_history_from_file(void) {
    FILE *f = fopen(HISTORY_FILE, "rb");
    if (!f) return;
    if (fread(&g_history_count, sizeof(g_history_count), 1, f) != 1) {
        g_history_count = 0;
    }
    if (g_history_count > MAX_SESSION_HISTORY) {
        g_history_count = 0;
    }
    if (g_history_count > 0) {
        if (fread(g_session_history, sizeof(SessionRecord), g_history_count, f) != g_history_count) {
            g_history_count = 0;
        }
    }
    fclose(f);
    g_session_counter = g_history_count;
}

static void add_session_to_history(const SessionRecord *record) {
    if (g_history_count < MAX_SESSION_HISTORY) {
        g_session_history[g_history_count++] = *record;
        save_history_to_file();
    }
}

static void build_dashboard(PerformanceDashboard *dashboard) {
    size_t i;
    dashboard->total_sessions_completed = g_history_count;
    dashboard->total_time_typing_seconds = 0;
    dashboard->total_words_typed = 0;
    dashboard->best_wpm_ever = 0;
    dashboard->best_accuracy_ever = 0;
    double total_wpm = 0;
    double total_accuracy = 0;

    for (i = 0; i < g_history_count; i++) {
        dashboard->total_time_typing_seconds += g_session_history[i].session_duration_seconds;
        dashboard->total_words_typed += g_session_history[i].total_words_typed;
        total_wpm += g_session_history[i].final_wpm;
        total_accuracy += g_session_history[i].final_accuracy;
        if (g_session_history[i].final_wpm > dashboard->best_wpm_ever) {
            dashboard->best_wpm_ever = g_session_history[i].final_wpm;
        }
        if (g_session_history[i].final_accuracy > dashboard->best_accuracy_ever) {
            dashboard->best_accuracy_ever = g_session_history[i].final_accuracy;
        }
    }

    if (g_history_count > 0) {
        dashboard->average_wpm = total_wpm / g_history_count;
        dashboard->average_accuracy = total_accuracy / g_history_count;
    } else {
        dashboard->average_wpm = 0;
        dashboard->average_accuracy = 0;
    }
}

static void remember_last_summary(const ScreenSummaryStats *summary) {
    g_last_summary = *summary;
    g_has_last_summary = 1;
}

static int restore_last_summary_from_history(double personal_best_wpm) {
    const SessionRecord *record;

    if (g_history_count == 0) {
        return 0;
    }

    record = &g_session_history[g_history_count - 1];
    g_last_summary.session_number = record->session_number;
    g_last_summary.time_mode_seconds = record->time_mode_seconds;
    g_last_summary.final_wpm = record->final_wpm;
    g_last_summary.final_accuracy = record->final_accuracy;
    g_last_summary.total_words_typed = record->total_words_typed;
    g_last_summary.correct_words = record->correct_words;
    g_last_summary.incorrect_words = record->incorrect_words;
    g_last_summary.personal_best_wpm = personal_best_wpm;
    g_last_summary.is_new_personal_best = 0;
    g_has_last_summary = 1;

    return 1;
}

static AppState state_after_secondary_back(void) {
    if (g_has_last_summary) {
        return STATE_SUMMARY;
    }

    return STATE_SETUP;
}

static AppState run_setup_screen(size_t *selected_time_index, DifficultyMode *selected_difficulty) {
    int needs_render = 1;

    while (1) {
        int key;

        if (needs_render) {
            screen_render_setup(*selected_time_index, (int)(*selected_difficulty), TIME_OPTIONS, TIME_OPTION_COUNT);
            needs_render = 0;
        }

        key = keyboard_read_char_nonblocking();

        if (key == KEY_ESC || key == 3) {
            return STATE_QUIT;
        }

        if (key >= '1' && key <= '5') {
            size_t next_index = (size_t)(key - '1');
            if (next_index < TIME_OPTION_COUNT) {
                if (*selected_time_index != next_index) {
                    *selected_time_index = next_index;
                    needs_render = 1;
                }
            }
        } else if (key == 'e' || key == 'E') {
            if (*selected_difficulty != DIFFICULTY_EASY) {
                *selected_difficulty = DIFFICULTY_EASY;
                needs_render = 1;
            }
        } else if (key == 'm' || key == 'M') {
            if (*selected_difficulty != DIFFICULTY_MEDIUM) {
                *selected_difficulty = DIFFICULTY_MEDIUM;
                needs_render = 1;
            }
        } else if (key == 'h' || key == 'H') {
            if (*selected_difficulty != DIFFICULTY_HARD) {
                *selected_difficulty = DIFFICULTY_HARD;
                needs_render = 1;
            }
        } else if (key == KEY_ENTER || key == '\n') {
            return STATE_TYPING;
        }

        usleep(10000);
    }
}

static AppState run_typing_session(int time_limit_seconds, DifficultyMode difficulty, double *personal_best_wpm) {
    char *target_stream;
    char *input_buffer;
    size_t target_capacity = INITIAL_TARGET_CAPACITY;
    size_t target_len = 0;
    size_t input_capacity = INITIAL_INPUT_CAPACITY;
    size_t input_len = 0;
    double start_time = 0.0;
    double elapsed_seconds = 0.0;
    double last_render_time = 0.0;
    int timer_started = 0;
    int finished = 0;
    int stdout_is_tty = isatty(STDOUT_FILENO);
    int needs_render = 1;

    target_stream = (char *)mem_alloc(target_capacity);
    target_stream[0] = '\0';
    append_random_words(&target_stream, &target_len, &target_capacity, difficulty, INITIAL_WORD_STREAM_WORDS);

    input_buffer = (char *)mem_alloc(input_capacity);
    mem_set(input_buffer, '\0', input_capacity);

    screen_clear();

    while (!finished) {
        int key = keyboard_read_char_nonblocking();

        if (key == KEY_ESC || key == 3) {
            mem_free(input_buffer);
            mem_free(target_stream);
            return STATE_QUIT;
        }

        if (key == KEY_TAB) {
            mem_free(input_buffer);
            mem_free(target_stream);
            return STATE_TYPING;
        }

        if (key == KEY_BACKSPACE || key == KEY_DELETE) {
            if (input_len > 0) {
                input_len--;
                input_buffer[input_len] = '\0';
                needs_render = 1;
            }
        } else if (key >= 32 && key <= 126) {
            int accept_char = 1;

            if (key == ' ') {
                if (input_len == 0 || input_buffer[input_len - 1] == ' ') {
                    accept_char = 0;
                }
            }

            if (accept_char) {
                if (!timer_started && key != ' ') {
                    start_time = time_now_seconds();
                    timer_started = 1;
                }

                if (target_len < input_len + TARGET_AHEAD_THRESHOLD) {
                    append_random_words(
                        &target_stream,
                        &target_len,
                        &target_capacity,
                        difficulty,
                        EXTEND_WORD_STREAM_WORDS
                    );
                }

                ensure_input_capacity(&input_buffer, &input_capacity, input_len + 1);

                {
                    size_t type_limit = math_min_size(target_len, input_capacity - 1);
                    if (input_len < type_limit) {
                        input_buffer[input_len] = (char)key;
                        input_len++;
                        input_buffer[input_len] = '\0';
                        needs_render = 1;
                    }
                }
            }
        }

        if (timer_started) {
            elapsed_seconds = time_now_seconds() - start_time;
            if (elapsed_seconds >= (double)time_limit_seconds) {
                elapsed_seconds = (double)time_limit_seconds;
                finished = 1;
            }
        } else {
            elapsed_seconds = 0.0;
        }

        {
            double current_time = time_now_seconds();
            double time_since_render = current_time - last_render_time;
            
            if (stdout_is_tty && (needs_render || (timer_started && time_since_render >= 0.016))) {
                ScreenLiveStats live_stats;
                build_live_stats(
                    target_stream,
                    input_buffer,
                    input_len,
                    elapsed_seconds,
                    timer_started,
                    time_limit_seconds,
                    *personal_best_wpm,
                    &live_stats
                );
                screen_render_typing(target_stream, input_buffer, input_len, &live_stats);
                needs_render = 0;
                last_render_time = current_time;
            }
        }

        usleep(1000);
    }

    {
        WordProgress summary_progress;
        ScreenSummaryStats summary;
        SessionRecord new_record;
        size_t correct_chars = str_count_correct(target_stream, input_buffer, input_len);
        double final_accuracy = math_percent(correct_chars, input_len);
        double final_wpm = math_wpm(input_len, elapsed_seconds);
        int is_new_best = 0;

        compute_word_progress(target_stream, input_buffer, input_len, 1, &summary_progress);

        if (final_wpm > *personal_best_wpm) {
            *personal_best_wpm = final_wpm;
            is_new_best = 1;
        }

        g_session_counter++;
        summary.session_number = g_session_counter;
        summary.time_mode_seconds = time_limit_seconds;
        summary.final_wpm = final_wpm;
        summary.final_accuracy = final_accuracy;
        summary.total_words_typed = summary_progress.total_words_typed;
        summary.correct_words = summary_progress.correct_words;
        summary.incorrect_words = summary_progress.incorrect_words;
        summary.personal_best_wpm = *personal_best_wpm;
        summary.is_new_personal_best = is_new_best;

        new_record.session_number = g_session_counter;
        new_record.time_mode_seconds = time_limit_seconds;
        new_record.difficulty_name = difficulty_name(difficulty);
        new_record.final_wpm = final_wpm;
        new_record.final_accuracy = final_accuracy;
        new_record.total_words_typed = summary_progress.total_words_typed;
        new_record.correct_words = summary_progress.correct_words;
        new_record.incorrect_words = summary_progress.incorrect_words;
        new_record.session_duration_seconds = elapsed_seconds;
        add_session_to_history(&new_record);

        remember_last_summary(&summary);
    }

    mem_free(input_buffer);
    mem_free(target_stream);

    return STATE_SUMMARY;
}

static AppState run_summary_screen(void) {
    if (!g_has_last_summary) {
        return STATE_SETUP;
    }

    screen_render_summary(&g_last_summary);

    while (1) {
        int key = keyboard_read_char_nonblocking();

        if (key == KEY_NONE) {
            usleep(10000);
            continue;
        }

        if (key == KEY_ESC || key == 3) return STATE_QUIT;
        if (key == KEY_ENTER || key == '\n') return STATE_SETUP;
        if (key == KEY_TAB) return STATE_TYPING;
        if (key == 'h' || key == 'H') return STATE_HISTORY;
        if (key == 'd' || key == 'D') return STATE_DASHBOARD;
    }
}

static AppState run_history_screen(void) {
    int page = 0;
    while (1) {
        screen_render_history(g_session_history, g_history_count, page);
        int key = keyboard_read_char_nonblocking();
        if (key == 'b' || key == 'B' || key == KEY_ESC || key == 3) {
            return state_after_secondary_back();
        }
        usleep(10000);
    }
}

static AppState run_dashboard_screen(void) {
    PerformanceDashboard dashboard;
    build_dashboard(&dashboard);
    while (1) {
        screen_render_dashboard(&dashboard);
        int key = keyboard_read_char_nonblocking();
        if (key == 'b' || key == 'B' || key == KEY_ESC || key == 3) {
            return state_after_secondary_back();
        }
        usleep(10000);
    }
}

int main(void) {
    size_t selected_time_index = 2;
    DifficultyMode selected_difficulty = DIFFICULTY_MEDIUM;
    double personal_best_wpm = 0.0;
    AppState current_state = STATE_SETUP;

    {
        struct timeval seed_tv;
        gettimeofday(&seed_tv, NULL);
        srand((unsigned int)(seed_tv.tv_sec ^ seed_tv.tv_usec));
    }

    if (!keyboard_init()) {
        fprintf(stderr, "failed to initialize keyboard\n");
        return 1;
    }

    load_history_from_file();
    {
        PerformanceDashboard db;
        build_dashboard(&db);
        personal_best_wpm = db.best_wpm_ever;
        restore_last_summary_from_history(personal_best_wpm);
    }

    while (current_state != STATE_QUIT) {
        switch (current_state) {
            case STATE_SETUP:
                current_state = run_setup_screen(&selected_time_index, &selected_difficulty);
                break;
            case STATE_TYPING:
                current_state = run_typing_session(TIME_OPTIONS[selected_time_index], selected_difficulty, &personal_best_wpm);
                break;
            case STATE_SUMMARY:
                current_state = run_summary_screen();
                break;
            case STATE_HISTORY:
                current_state = run_history_screen();
                break;
            case STATE_DASHBOARD:
                current_state = run_dashboard_screen();
                break;
            case STATE_QUIT:
                break;
        }
    }

    keyboard_shutdown();

    screen_clear();
    printf("Goodbye.\n");
    return 0;
}
