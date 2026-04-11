#include "memory.h"

#include <stdio.h>
#include <stdlib.h>

#define MEMORY_POOL_SIZE 100000

typedef struct MemoryBlock {
    size_t size;
    int is_free;
    struct MemoryBlock *next;
} MemoryBlock;

static union {
    max_align_t align;
    unsigned char bytes[MEMORY_POOL_SIZE];
} memory_pool;

static MemoryBlock *memory_head = NULL;
static size_t memory_used = 0;

static size_t aligned_size(size_t size) {
    size_t alignment = sizeof(max_align_t);
    size_t mask = alignment - 1;
    return (size + mask) & ~mask;
}

static size_t header_size(void) {
    return aligned_size(sizeof(MemoryBlock));
}

static MemoryBlock *block_from_ptr(void *ptr) {
    return (MemoryBlock *)((unsigned char *)ptr - header_size());
}

static void *ptr_from_block(MemoryBlock *block) {
    return (void *)((unsigned char *)block + header_size());
}

static void split_block(MemoryBlock *block, size_t requested_size) {
    size_t block_header = header_size();

    if (block->size <= requested_size + block_header + sizeof(max_align_t)) {
        return;
    }

    {
        unsigned char *new_block_addr = (unsigned char *)ptr_from_block(block) + requested_size;
        MemoryBlock *new_block = (MemoryBlock *)new_block_addr;
        new_block->size = block->size - requested_size - block_header;
        new_block->is_free = 1;
        new_block->next = block->next;
        block->size = requested_size;
        block->next = new_block;
    }
}

static void merge_forward(MemoryBlock *block) {
    size_t block_header = header_size();

    while (block->next != NULL && block->next->is_free) {
        block->size += block_header + block->next->size;
        block->next = block->next->next;
    }
}

static MemoryBlock *find_free_block(size_t size) {
    MemoryBlock *cur = memory_head;

    while (cur != NULL) {
        if (cur->is_free && cur->size >= size) {
            return cur;
        }
        cur = cur->next;
    }

    return NULL;
}

static MemoryBlock *append_block(size_t size) {
    size_t block_header = header_size();
    size_t needed = block_header + size;
    MemoryBlock *new_block;

    if (memory_used + needed > MEMORY_POOL_SIZE) {
        return NULL;
    }

    new_block = (MemoryBlock *)(memory_pool.bytes + memory_used);
    new_block->size = size;
    new_block->is_free = 0;
    new_block->next = NULL;
    memory_used += needed;

    if (memory_head == NULL) {
        memory_head = new_block;
        return new_block;
    }

    {
        MemoryBlock *cur = memory_head;
        while (cur->next != NULL) {
            cur = cur->next;
        }
        cur->next = new_block;
    }

    return new_block;
}

void *mem_alloc(size_t size) {
    MemoryBlock *block;
    size_t requested;

    if (size == 0) {
        return NULL;
    }

    requested = aligned_size(size);
    block = find_free_block(requested);

    if (block != NULL) {
        block->is_free = 0;
        split_block(block, requested);
        return ptr_from_block(block);
    }

    block = append_block(requested);
    if (block == NULL) {
        fprintf(stderr, "memory pool exhausted\n");
        exit(1);
    }

    return ptr_from_block(block);
}

void mem_free(void *ptr) {
    MemoryBlock *block;
    MemoryBlock *cur;
    MemoryBlock *prev = NULL;

    if (ptr == NULL) {
        return;
    }

    block = block_from_ptr(ptr);
    block->is_free = 1;
    merge_forward(block);

    cur = memory_head;
    while (cur != NULL && cur != block) {
        prev = cur;
        cur = cur->next;
    }

    if (prev != NULL && prev->is_free) {
        merge_forward(prev);
    }
}

void *mem_resize(void *ptr, size_t new_size) {
    MemoryBlock *block;
    size_t requested;

    if (ptr == NULL) {
        return mem_alloc(new_size);
    }

    if (new_size == 0) {
        mem_free(ptr);
        return NULL;
    }

    requested = aligned_size(new_size);
    block = block_from_ptr(ptr);

    if (block->size >= requested) {
        split_block(block, requested);
        return ptr;
    }

    if (block->next != NULL && block->next->is_free) {
        size_t combined = block->size + header_size() + block->next->size;
        if (combined >= requested) {
            merge_forward(block);
            split_block(block, requested);
            return ptr;
        }
    }

    {
        void *new_ptr = mem_alloc(requested);
        size_t copy_count = block->size;
        if (copy_count > requested) {
            copy_count = requested;
        }
        mem_copy((char *)new_ptr, (const char *)ptr, copy_count);
        mem_free(ptr);
        return new_ptr;
    }
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
