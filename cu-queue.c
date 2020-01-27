#include "cu-queue.h"
#include "cu.h"
#include <memory.h>

#if DEBUG
#include <stdio.h>
#include <inttypes.h>
#endif

#define CONCAT_INTERMEDIATE(a,b) a##b
#define CONCAT(a,b) CONCAT_INTERMEDIATE(a,b)
#define BUILD_FUNC(f) CONCAT(QUEUE_PREFIX, _ ## f)

#if ((!defined(QUEUE_LOCKED) || !QUEUE_LOCKED) && (!defined(QUEUE_FIXED_SIZE) || !QUEUE_FIXED_SIZE))
#define QUEUE_PREFIX cu_queue
#define QUEUE_TYPE CUQueue
#define QUEUE_LOCK(q)
#define QUEUE_UNLOCK(q)
#elif (defined(QUEUE_LOCKED) && QUEUE_LOCKED)
#define QUEUE_PREFIX cu_queue_locked
#define QUEUE_TYPE CUQueueLocked
#define QUEUE_LOCK(q)   pthread_mutex_lock(&(q)->lock)
#define QUEUE_UNLOCK(q) pthread_mutex_unlock(&(q)->lock)
#else
#define QUEUE_PREFIX cu_queue_fixed_size
#define QUEUE_TYPE CUQueueFixedSize
#define QUEUE_LOCK(q)
#define QUEUE_UNLOCK(q)
#endif

/* Initialize the queue. */
void BUILD_FUNC(init)(QUEUE_TYPE *queue
#ifdef QUEUE_FIXED_SIZE
        , size_t element_size, size_t group_size
#endif
        )
{
    if (cu_unlikely(!queue))
        return;
    memset(queue, 0, sizeof(QUEUE_TYPE));
#if QUEUE_LOCKED
    pthread_mutex_init(&queue->lock, NULL);
#endif
#if QUEUE_FIXED_SIZE
    queue->pool = cu_fixed_size_memory_pool_new(element_size + sizeof(CUList), group_size);
    queue->element_size = element_size;
#endif
}

/* Clear the queue. */
void BUILD_FUNC(clear)(QUEUE_TYPE *queue, CUDestroyNotifyFunc notify)
{
    if (cu_unlikely(!queue))
        return;

#if QUEUE_FIXED_SIZE
    cu_fixed_size_memory_pool_clear(queue->pool);
#else
    cu_list_free_full(queue->head, notify);
#endif

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
#if QUEUE_FIXED_SIZE
    cu_fixed_size_memory_pool_destroy(queue->pool);
#endif
}

/* Push to the end of the queue. */
void BUILD_FUNC(push_tail)(QUEUE_TYPE *queue, void *data)
{
    if (cu_unlikely(!queue))
        return;
#if QUEUE_FIXED_SIZE
    CUList *entry = cu_fixed_size_memory_pool_alloc(queue->pool);
    /* Set data pointer to the end of the structure. */
    entry->data = entry + 1;
    memcpy(entry->data, data, queue->element_size);
#else
    CUList *entry = cu_alloc(sizeof(CUList));
    entry->data = data;
#endif
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
#if QUEUE_FIXED_SIZE
bool
#else
void *
#endif
BUILD_FUNC(pop_head)(QUEUE_TYPE *queue
#if QUEUE_FIXED_SIZE
        , void *output
#endif
        )
{
    if (cu_unlikely(!queue))
#if QUEUE_FIXED_SIZE
        return false;
#else
        return NULL;
#endif

#if QUEUE_FIXED_SIZE
    bool have_data = false;
#else
    void *data = NULL;
#endif
    CUList *tmp;

    QUEUE_LOCK(queue);

    if (queue->head) {
        tmp = queue->head->next;
#if QUEUE_FIXED_SIZE
        if (output)
            memcpy(output, queue->head->data, queue->element_size);
        cu_fixed_size_memory_pool_free(queue->pool, queue->head);
        have_data = true;
#else
        data = queue->head->data;
        cu_free(queue->head);
#endif

        queue->head = tmp;

        if (queue->head)
            queue->head->prev = NULL;
        else
            queue->tail = NULL;
        --queue->length;
    }

    QUEUE_UNLOCK(queue);

#if QUEUE_FIXED_SIZE
    return have_data;
#else
    return data;
#endif
}

/* Peek the head of the queue. */
#if QUEUE_FIXED_SIZE
bool
#else
void *
#endif
BUILD_FUNC(peek_head)(QUEUE_TYPE *queue
#if QUEUE_FIXED_SIZE
        , void *output
#endif
        )
{
#if QUEUE_FIXED_SIZE
    bool result = false;
#else
    void *result = NULL;
#endif
    QUEUE_LOCK(queue);
    if (queue && queue->head) {
#if QUEUE_FIXED_SIZE
        result = true;
        if (output)
            memcpy(output, queue->head->data, queue->element_size);
#else
        result = queue->head->data;
#endif
    }
    QUEUE_UNLOCK(queue);
    return result;
}

/* Pop some element matching a criterion. */
#if QUEUE_FIXED_SIZE
bool
#else
void *
#endif
BUILD_FUNC(pop_custom)(QUEUE_TYPE *queue, CUCompareFunc compare, void *userdata
#if QUEUE_FIXED_SIZE
        , void *output
#endif
        )
{
    if (cu_unlikely(!queue))
#if QUEUE_FIXED_SIZE
        return false;
#else
        return NULL;
#endif

#if QUEUE_FIXED_SIZE
        bool have_data = false;
#else
        void *data = NULL;
#endif
        CUList *tmp;

        QUEUE_LOCK(queue);

        for (tmp = queue->head; tmp; tmp = tmp->next) {
            if (compare(tmp->data, userdata) == 0) {
                if (tmp->prev)
                    tmp->prev->next = tmp->next;
                else
                    queue->head = tmp->next;
                if (tmp->next)
                    tmp->next->prev = tmp->prev;
                else
                    queue->tail = tmp->prev;
#if QUEUE_FIXED_SIZE
                if (output)
                    memcpy(output, tmp->data, queue->element_size);
                cu_fixed_size_memory_pool_free(queue->pool, tmp);
                have_data = true;
#else
                data = tmp->data;
                cu_free(tmp);
#endif
                --queue->length;
                break;
            }
        }

        QUEUE_UNLOCK(queue);

#if QUEUE_FIXED_SIZE
        return have_data;
#else
        return data;
#endif
}
