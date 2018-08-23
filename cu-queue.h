/* Generic double-ended queue. */
#pragma once

#include <cu-list.h>
#include <stdlib.h>

typedef struct {
    CUList *head;
    CUList *tail;

    size_t length;
} CUQueue;


/* Initialize the queue. */
void cu_queue_init(CUQueue *queue);

/* Clear the queue. */
void cu_queue_clear(CUQueue *queue, CUDestroyNotifyFunc notify);

/* Push to the end of the queue. */
void cu_queue_push_tail(CUQueue *queue, void *data);

/* Pop from the head of the queue. */
void *cu_queue_pop_head(CUQueue *queue);

/* Peek the head of the queue. */
void *cu_queue_peek_head(CUQueue *queue);
