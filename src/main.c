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

typedef enum DifficultyMode {
    DIFFICULTY_EASY = 0,
    DIFFICULTY_MEDIUM = 1,
    DIFFICULTY_HARD = 2
} DifficultyMode;

typedef enum SessionAction {
    ACTION_QUIT = 0,
    ACTION_RESTART_SAME = 1,
    ACTION_TO_SETUP = 2
} SessionAction;

typedef struct WordProgress {
    size_t active_word_index;
    size_t total_words_typed;
    size_t correct_words;
    size_t incorrect_words;
    size_t current_streak;
    size_t best_streak;
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
    size_t current_streak = 0;

    progress->active_word_index = 0;
    progress->total_words_typed = 0;
    progress->correct_words = 0;
    progress->incorrect_words = 0;
    progress->current_streak = 0;
    progress->best_streak = 0;

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
            current_streak++;
            if (current_streak > progress->best_streak) {
                progress->best_streak = current_streak;
            }
        } else {
            progress->incorrect_words++;
            current_streak = 0;
        }

        progress->current_streak = current_streak;

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
    DifficultyMode difficulty,
    ScreenLiveStats *stats
) {
    WordProgress progress;
    size_t correct_chars = str_count_correct(target_stream, input, input_len);
    size_t errors = (input_len >= correct_chars) ? (input_len - correct_chars) : 0;

    compute_word_progress(target_stream, input, input_len, 0, &progress);

    stats->elapsed_seconds = elapsed_seconds;
    stats->remaining_seconds = timer_started
        ? ((elapsed_seconds < (double)selected_time_seconds)
            ? ((double)selected_time_seconds - elapsed_seconds)
            : 0.0)
        : (double)selected_time_seconds;
    stats->accuracy = math_percent(correct_chars, input_len);
    stats->wpm = math_wpm(input_len, elapsed_seconds);
    stats->correct_chars = correct_chars;
    stats->errors = errors;
    stats->total_words_typed = progress.total_words_typed;
    stats->correct_words = progress.correct_words;
    stats->incorrect_words = progress.incorrect_words;
    stats->current_streak = progress.current_streak;
    stats->best_streak = progress.best_streak;
    stats->active_word_index = progress.active_word_index;
    stats->timer_started = timer_started;
    stats->selected_time_seconds = selected_time_seconds;
    stats->difficulty_name = difficulty_name(difficulty);
}

static SessionAction run_setup_screen(size_t *selected_time_index, DifficultyMode *selected_difficulty) {
    while (1) {
        int key;

        screen_render_setup(*selected_time_index, (int)(*selected_difficulty), TIME_OPTIONS, TIME_OPTION_COUNT);
        key = keyboard_read_char_nonblocking();

        if (key == KEY_ESC || key == 3) {
            return ACTION_QUIT;
        }

        if (key >= '1' && key <= '5') {
            size_t next_index = (size_t)(key - '1');
            if (next_index < TIME_OPTION_COUNT) {
                *selected_time_index = next_index;
            }
        } else if (key == 'e' || key == 'E') {
            *selected_difficulty = DIFFICULTY_EASY;
        } else if (key == 'm' || key == 'M') {
            *selected_difficulty = DIFFICULTY_MEDIUM;
        } else if (key == 'h' || key == 'H') {
            *selected_difficulty = DIFFICULTY_HARD;
        } else if (key == KEY_ENTER || key == '\n') {
            return ACTION_RESTART_SAME;
        }

        usleep(10000);
    }
}

static SessionAction run_typing_session(int time_limit_seconds, DifficultyMode difficulty, double *personal_best_wpm) {
    char *target_stream;
    char *input_buffer;
    size_t target_capacity = INITIAL_TARGET_CAPACITY;
    size_t target_len = 0;
    size_t input_capacity = INITIAL_INPUT_CAPACITY;
    size_t input_len = 0;
    double start_time = 0.0;
    double elapsed_seconds = 0.0;
    int timer_started = 0;
    int finished = 0;
    int timed_out = 0;

    target_stream = (char *)mem_alloc(target_capacity);
    target_stream[0] = '\0';
    append_random_words(&target_stream, &target_len, &target_capacity, difficulty, INITIAL_WORD_STREAM_WORDS);

    input_buffer = (char *)mem_alloc(input_capacity);
    mem_set(input_buffer, '\0', input_capacity);

    while (!finished) {
        int key = keyboard_read_char_nonblocking();

        if (key == KEY_ESC || key == 3) {
            mem_free(input_buffer);
            mem_free(target_stream);
            return ACTION_QUIT;
        }

        if (key == KEY_TAB) {
            mem_free(input_buffer);
            mem_free(target_stream);
            return ACTION_RESTART_SAME;
        }

        if (key == KEY_BACKSPACE || key == KEY_DELETE) {
            if (input_len > 0) {
                input_len--;
                input_buffer[input_len] = '\0';
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
                    }
                }
            }
        }

        if (timer_started) {
            elapsed_seconds = time_now_seconds() - start_time;
            if (elapsed_seconds >= (double)time_limit_seconds) {
                elapsed_seconds = (double)time_limit_seconds;
                timed_out = 1;
                finished = 1;
            }
        } else {
            elapsed_seconds = 0.0;
        }

        {
            ScreenLiveStats live_stats;
            build_live_stats(
                target_stream,
                input_buffer,
                input_len,
                elapsed_seconds,
                timer_started,
                time_limit_seconds,
                difficulty,
                &live_stats
            );
            screen_render_typing(target_stream, input_buffer, input_len, &live_stats);
        }

        usleep(10000);
    }

    {
        WordProgress summary_progress;
        ScreenSummaryStats summary;
        size_t correct_chars = str_count_correct(target_stream, input_buffer, input_len);
        double final_accuracy = math_percent(correct_chars, input_len);
        double final_wpm = math_wpm(input_len, elapsed_seconds);
        int is_new_best = 0;

        compute_word_progress(target_stream, input_buffer, input_len, 1, &summary_progress);

        if (final_wpm > *personal_best_wpm) {
            *personal_best_wpm = final_wpm;
            is_new_best = 1;
        }

        summary.final_wpm = final_wpm;
        summary.final_accuracy = final_accuracy;
        summary.total_words_typed = summary_progress.total_words_typed;
        summary.correct_words = summary_progress.correct_words;
        summary.incorrect_words = summary_progress.incorrect_words;
        summary.current_streak = summary_progress.current_streak;
        summary.best_streak = summary_progress.best_streak;
        summary.personal_best_wpm = *personal_best_wpm;
        summary.is_new_personal_best = is_new_best;
        summary.timed_out = timed_out;

        screen_render_summary(&summary);
    }

    while (1) {
        int key = keyboard_read_char_nonblocking();

        if (key == KEY_NONE) {
            usleep(10000);
            continue;
        }

        mem_free(input_buffer);
        mem_free(target_stream);

        if (key == KEY_ESC || key == 3) {
            return ACTION_QUIT;
        }

        if (key == KEY_ENTER || key == '\n') {
            return ACTION_TO_SETUP;
        }

        return ACTION_RESTART_SAME;
    }
}

int main(void) {
    size_t selected_time_index = 2;
    DifficultyMode selected_difficulty = DIFFICULTY_MEDIUM;
    int show_setup = 1;
    double personal_best_wpm = 0.0;

    {
        struct timeval seed_tv;
        gettimeofday(&seed_tv, NULL);
        srand((unsigned int)(seed_tv.tv_sec ^ seed_tv.tv_usec));
    }

    if (!keyboard_init()) {
        fprintf(stderr, "failed to initialize keyboard\n");
        return 1;
    }

    while (1) {
        SessionAction action;

        if (show_setup) {
            action = run_setup_screen(&selected_time_index, &selected_difficulty);
            if (action == ACTION_QUIT) {
                break;
            }
        }

        action = run_typing_session(TIME_OPTIONS[selected_time_index], selected_difficulty, &personal_best_wpm);

        if (action == ACTION_QUIT) {
            break;
        }

        if (action == ACTION_TO_SETUP) {
            show_setup = 1;
        } else {
            show_setup = 0;
        }
    }

    keyboard_shutdown();

    screen_clear();
    printf("Goodbye.\n");
    return 0;
}