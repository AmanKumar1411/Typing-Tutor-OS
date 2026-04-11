#include "math_utils.h"

double math_percent(size_t part, size_t whole) {
    if (whole == 0) {
        return 0.0;
    }
    return ((double)part * 100.0) / (double)whole;
}

double math_wpm(size_t typed_chars, double elapsed_seconds) {
    double minutes;
    if (elapsed_seconds <= 0.0) {
        return 0.0;
    }
    minutes = elapsed_seconds / 60.0;
    return ((double)typed_chars / 5.0) / minutes;
}

size_t math_min_size(size_t a, size_t b) {
    if (a < b) {
        return a;
    }
    return b;
}
