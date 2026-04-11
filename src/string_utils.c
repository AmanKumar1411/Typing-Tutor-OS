#include "string_utils.h"

size_t str_length(const char *text) {
    size_t len = 0;
    while (text[len] != '\0') {
        len++;
    }
    return len;
}

int str_equals(const char *left, const char *right) {
    size_t i = 0;
    while (left[i] != '\0' && right[i] != '\0') {
        if (left[i] != right[i]) {
            return 0;
        }
        i++;
    }
    return left[i] == right[i];
}

size_t str_count_correct_prefix(const char *target, const char *input, size_t input_len) {
    size_t i;
    size_t count = 0;
    for (i = 0; i < input_len; i++) {
        if (target[i] == '\0') {
            break;
        }
        if (target[i] == input[i]) {
            count++;
        }
    }
    return count;
}

size_t str_count_words(const char *text, size_t len) {
    size_t i;
    size_t words = 0;
    int in_word = 0;

    for (i = 0; i < len; i++) {
        char c = text[i];
        int is_space = (c == ' ' || c == '\n' || c == '\t' || c == '\r');

        if (!is_space && !in_word) {
            words++;
            in_word = 1;
        } else if (is_space) {
            in_word = 0;
        }
    }

    return words;
}
