#include "memory.h"

#include <stdio.h>
#include <stdlib.h>

void *mem_alloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "memory allocation failed\n");
        exit(1);
    }
    return ptr;
}

void mem_free(void *ptr) {
    if (ptr != NULL) {
        free(ptr);
    }
}

void *mem_resize(void *ptr, size_t new_size) {
    void *new_ptr = realloc(ptr, new_size);
    if (new_ptr == NULL) {
        fprintf(stderr, "memory reallocation failed\n");
        exit(1);
    }
    return new_ptr;
}

void mem_copy(char *dest, const char *src, size_t count) {
    size_t i;
    for (i = 0; i < count; i++) {
        dest[i] = src[i];
    }
}

void mem_set(char *dest, char value, size_t count) {
    size_t i;
    for (i = 0; i < count; i++) {
        dest[i] = value;
    }
}
