/* Handle allocations/frees. */
#pragma once

#include <stdlib.h>

/* These are wrappers to allow other alloc/free functions. */
void *cu_alloc(size_t size);
void cu_free(void *ptr);
void *cu_realloc(void *ptr, size_t size);

/* The memory handler. These have the same signature as the standard
 * glibc malloc/realloc/free functions, which are used by default.
 * However, we provide this mechanism to allow other types of memory management.
 */
typedef struct {
    void *(*alloc)(size_t);
    void *(*realloc)(void *, size_t);
    void (*free)(void *);
} CUMemoryHandler;

/* Set an alternative memory handler. */
void cu_set_memory_handler(CUMemoryHandler *handler);
