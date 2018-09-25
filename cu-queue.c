#include "cu-queue.h"
#include "cu.h"
#include <memory.h>

#define CONCAT_INTERMEDIATE(a,b) a##b
#define CONCAT(a,b) CONCAT_INTERMEDIATE(a,b)
#define BUILD_FUNC(f) CONCAT(QUEUE_PREFIX, _ ## f)

#if (!defined(QUEUE_LOCKED) || !QUEUE_LOCKED)
#define QUEUE_PREFIX cu_queue
#define QUEUE_TYPE CUQueue
#define QUEUE_LOCK(q)
#define QUEUE_UNLOCK(q)
#else
#define QUEUE_PREFIX cu_queue_locked
#define QUEUE_TYPE CUQueueLocked
#define QUEUE_LOCK(q)   pthread_mutex_lock(&(q)->lock)
#define QUEUE_UNLOCK(q) pthread_mutex_unlock(&(q)->lock)
#endif

/* Initialize the queue. */
void BUILD_FUNC(init)(QUEUE_TYPE *queue)
{
    if (cu_unlikely(!queue))
        return;
    memset(queue, 0, sizeof(QUEUE_TYPE));
#if QUEUE_LOCKED
    pthread_mutex_init(&queue->lock, NULL);
#endif
}

/* Clear the queue. */
void BUILD_FUNC(clear)(QUEUE_TYPE *queue, CUDestroyNotifyFunc notify)
{
    if (cu_unlikely(!queue))
        return;

    cu_list_free_full(queue->head, notify);

    queue->head = NULL;
    queue->tail = NULL;
    queue->length = 0;
}

/* Destroy the queue. For CUQueue this just calls clear. For CUQueueLocked it calls clear and
 * destroys the mutex.
 */
void BUILD_FUNC(destroy)(QUEUE_TYPE *queue, CUDestroyNotifyFunc notify)
{
    BUILD_FUNC(clear)(queue, notify);
#if QUEUE_LOCKED
    pthread_mutex_destroy(&queue->lock);
#endif
}

/* Push to the end of the queue. */
void BUILD_FUNC(push_tail)(QUEUE_TYPE *queue, void *data)
{
    if (cu_unlikely(!queue))
        return;
    CUList *entry = cu_alloc(sizeof(CUList));
    entry->data = data;
    entry->next = NULL;

    QUEUE_LOCK(queue);

    entry->prev = queue->tail;

    if (queue->tail)
        queue->tail->next = entry;
    else
        queue->head = entry;

    queue->tail = entry;
    ++queue->length;

    QUEUE_UNLOCK(queue);
}

/* Pop from the head of the queue. */
void *BUILD_FUNC(pop_head)(QUEUE_TYPE *queue)
{
    if (cu_unlikely(!queue))
        return NULL;

    void *data = NULL;
    CUList *tmp;

    QUEUE_LOCK(queue);

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

    QUEUE_UNLOCK(queue);

    return data;
}

/* Peek the head of the queue. */
void *BUILD_FUNC(peek_head)(QUEUE_TYPE *queue)
{
    void *result = NULL;
    QUEUE_LOCK(queue);
    if (queue && queue->head)
        result = queue->head->data;
    QUEUE_UNLOCK(queue);
    return result;
}
