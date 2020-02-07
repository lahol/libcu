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

/* Destroy the queue. */
void cu_queue_destroy(CUQueue *queue, CUDestroyNotifyFunc notify);

/* Push to the end of the queue. */
void cu_queue_push_tail(CUQueue *queue, void *data);

/* Pop from the head of the queue. */
void *cu_queue_pop_head(CUQueue *queue);

/* Peek the head of the queue. */
void *cu_queue_peek_head(CUQueue *queue);

/* Pop a custom element from the queue. */
void *cu_queue_pop_custom(CUQueue *queue, CUCompareFunc compare, void *userdata);

/* Remove a link from the queue. */
void cu_queue_delete_link(CUQueue *queue, CUList *link);

/* Run callback for each element. */
void cu_queue_foreach(CUQueue *queue, CUForeachFunc callback, void *userdata);
