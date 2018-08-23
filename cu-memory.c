#include "cu-memory.h"
#include <stdlib.h>
#include <memory.h>

static CUMemoryHandler memhandler = {
    .alloc   = malloc,
    .realloc = realloc,
    .free    = free
};

void *cu_alloc(size_t size)
{
    void *ptr = memhandler.alloc(size);
    if (!ptr && size)
        exit(1);
    return ptr;
}

void *cu_alloc0(size_t size)
{
    void *ptr = cu_alloc(size);
    if (ptr && size)
        memset(ptr, 0, size);
    return ptr;
}

void cu_free(void *ptr)
{
    if (ptr)
        memhandler.free(ptr);
}

void *cu_realloc(void *ptr, size_t size)
{
    ptr = memhandler.realloc(ptr, size);
    if (!ptr && size)
        exit(1);
    return ptr;
}

void cu_set_memory_handler(CUMemoryHandler *handler)
{
    if (handler) {
        memhandler.alloc = handler->alloc ? handler->alloc : malloc;
        memhandler.realloc = handler->realloc ? handler->realloc : realloc;
        memhandler.free = handler->free ? handler->free : free;
    }
    else {
        memhandler.alloc = malloc;
        memhandler.realloc = realloc;
        memhandler.free = free;
    }
}
