/* Generic double-ended queue with lock.
 * The implementation of the standard queue is used. We could do this also in the header.
 * however, we want to hide the ugly stuff from the user, so we repeat the declaration here. */
#pragma once

#include <cu-list.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct {
    CUList *head;
    CUList *tail;

    size_t length;

    pthread_mutex_t lock;
} CUQueueLocked;


/* Initialize the queue. */
void cu_queue_locked_init(CUQueueLocked *queue);

/* Clear the queue. */
void cu_queue_locked_clear(CUQueueLocked *queue, CUDestroyNotifyFunc notify);

/* Destroy the queue. */
void cu_queue_locked_destroy(CUQueueLocked *queue, CUDestroyNotifyFunc notify);

/* Push to the end of the queue. */
void cu_queue_locked_push_tail(CUQueueLocked *queue, void *data);

/* Pop from the head of the queue. */
void *cu_queue_locked_pop_head(CUQueueLocked *queue);

/* Peek the head of the queue. */
void *cu_queue_locked_peek_head(CUQueueLocked *queue);

/* Pop a custom element from the queue. */
void *cu_queue_locked_pop_custom(CUQueueLocked *queue, CUCompareFunc compare, void *userdata);
