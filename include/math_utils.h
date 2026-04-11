#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <stddef.h>

double math_percent(size_t part, size_t whole);
double math_wpm(size_t typed_chars, double elapsed_seconds);
size_t math_min_size(size_t a, size_t b);

#endif
