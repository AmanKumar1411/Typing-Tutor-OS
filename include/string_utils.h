#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <stddef.h>

size_t str_length(const char *text);
int str_equals(const char *left, const char *right);
size_t str_count_correct(const char *target, const char *input, size_t input_len);
size_t str_count_words(const char *text, size_t len);

#endif
