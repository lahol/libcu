#include "cu-queue.h"
#include "cu.h"
#include <memory.h>

/* Initialize the queue. */
void cu_queue_init(CUQueue *queue)
{
    if (cu_unlikely(!queue))
        return;
    memset(queue, 0, sizeof(CUQueue));
}

/* Clear the queue. */
void cu_queue_clear(CUQueue *queue, CUDestroyNotifyFunc notify)
{
    if (cu_unlikely(!queue))
        return;

    cu_list_free_full(queue->head, notify);
    memset(queue, 0, sizeof(CUQueue));
}

/* Push to the end of the queue. */
void cu_queue_push_tail(CUQueue *queue, void *data)
{
    if (cu_unlikely(!queue))
        return;
    CUList *entry = cu_alloc(sizeof(CUList));
    entry->data = data;
    entry->prev = queue->tail;
    entry->next = NULL;

    if (queue->tail)
        queue->tail->next = entry;
    else
        queue->head = entry;

    queue->tail = entry;
    ++queue->length;
}

/* Pop from the head of the queue. */
void *cu_queue_pop_head(CUQueue *queue)
{
    if (cu_unlikely(!queue))
        return NULL;

    void *data = NULL;
    CUList *tmp;

    if (queue->head) {
        tmp = queue->head->next;
        data = queue->head->data;
        cu_free(queue->head);

        queue->head = tmp;

        if (queue->head)
            queue->head->prev = NULL;
        else
            queue->tail = NULL;
        --queue->length;
    }

    return data;
}

/* Peek the head of the queue. */
void *cu_queue_peek_head(CUQueue *queue)
{
    if (queue && queue->head)
        return queue->head->data;
    return NULL;
}
