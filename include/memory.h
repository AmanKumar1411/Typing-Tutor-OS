#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

void *mem_alloc(size_t size);
void mem_free(void *ptr);
void *mem_resize(void *ptr, size_t new_size);
void mem_copy(char *dest, const char *src, size_t count);
void mem_set(char *dest, char value, size_t count);

#endif
