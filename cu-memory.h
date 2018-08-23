/* Handle allocations/frees. */
#pragma once

#include <stdlib.h>
#include <memory.h>

/* These are wrappers to allow other alloc/free functions. */
void *cu_alloc(size_t size);
void *cu_alloc0(size_t size);
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

static __attribute__((always_inline)) inline
void cu_alloc_aligned(void **ptr, size_t size)
{
    if (__builtin_expect((posix_memalign(ptr, 16, size) != 0), 0))
        exit(1);
}

static __attribute__((always_inline)) inline
void cu_alloc_aligned0(void **ptr, size_t size)
{
    cu_alloc_aligned(ptr, size);
    memset(*ptr, 0, size);
}
