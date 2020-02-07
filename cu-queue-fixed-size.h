/* Generic double-ended queue for fixed size elements, using the fixed size memory pool.
 * Note that elements may become invalid after the next allocation.
 * The implementation of the standard queue is used. We could do this also in the header.
 * however, we want to hide the ugly stuff from the user, so we repeat the declaration here. */
#pragma once

#include <cu-list.h>
#include <stdlib.h>
#include <pthread.h>
#include <cu-memory.h>

typedef struct {
    CUList *head;
    CUList *tail;

    size_t length;

    CUFixedSizeMemoryPool *pool;
    size_t element_size;
} CUQueueFixedSize;


/* Initialize the queue. */
void cu_queue_fixed_size_init(CUQueueFixedSize *queue, size_t element_size, size_t group_size);

/* Clear the queue. notify must not free the pointer itself. */
void cu_queue_fixed_size_clear(CUQueueFixedSize *queue, CUDestroyNotifyFunc notify);

/* Destroy the queue. notify must not free the pointer itself. */
void cu_queue_fixed_size_destroy(CUQueueFixedSize *queue, CUDestroyNotifyFunc notify);

/* Push to the end of the queue. */
void cu_queue_fixed_size_push_tail(CUQueueFixedSize *queue, void *data);

/* Pop from the head of the queue. */
bool cu_queue_fixed_size_pop_head(CUQueueFixedSize *queue, void *output);

/* Peek the head of the queue. */
bool cu_queue_fixed_size_peek_head(CUQueueFixedSize *queue, void *output);

/* Pop a custom element from the queue. */
bool cu_queue_fixed_size_pop_custom(CUQueueFixedSize *queue, CUCompareFunc compare, void *userdata, void *output);

/* Remove a link from the queue. */
void cu_queue_fixed_size_delete_link(CUQueueFixedSize *queue, CUList *link);

/* Run callback for each element. */
void cu_queue_fixed_size_foreach(CUQueueFixedSize *queue, CUForeachFunc callback, void *userdata);
