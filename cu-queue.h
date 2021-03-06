/** @file cu-queue.h
 *  Generic double-ended queue.
 *  @defgroup CUQueue Double ended queue.
 *  @{
 */
#pragma once

#include <cu-list.h>
#include <stdlib.h>

/** @brief A double ended queue. */
typedef struct {
    CUList *head; /**< Pointer to the head of the queue. */
    CUList *tail; /**< Pointer to the tail of the queue. */

    size_t length; /**< Total number of elements in the queue. */
} CUQueue;


/** @brief Initialize a queue.
 *  @param[in] queue The queue to initialize.
 */
void cu_queue_init(CUQueue *queue);

/** @brief Clear a queue.
 *  @details All entries of the queue are removed and @a notify is called for every entry.
 *           The queue itself is ready for use again.
 *  @param[in] queue The queue to clear.
 *  @param[in] notify Function to call for each data element to free its resources.
 */
void cu_queue_clear(CUQueue *queue, CUDestroyNotifyFunc notify);

/** @brief Destroy a queue.
 *  @param[in] queue The queue to destroy.
 *  @param[in] notify Function to call for each data element to free its resources.
 */
void cu_queue_destroy(CUQueue *queue, CUDestroyNotifyFunc notify);

/** @brief Push to the end of the queue.
 *  @param[in] queue The queue to push to.
 *  @param[in] data The data to insert in the queue.
 */
void cu_queue_push_tail(CUQueue *queue, void *data);

/** Pop from the head of the queue.
 *  @param[in] queue The queue to pop from.
 *  @return The data of the head or @a NULL if the queue was empty.
 */
void *cu_queue_pop_head(CUQueue *queue);

/** @brief Return the data from the head of the queue but do not remove it.
 *  @param[in] queue The queue for which we want to get the data.
 *  @return The data of the head or @a NULL if the queue was empty.
 */
void *cu_queue_peek_head(CUQueue *queue);

/** @brief Get the first element from the queue that matches a given criterion.
 *  @param[in] queue The queue for which we want to get an element.
 *  @param[in] compare A function returning 0 if the element matches the criterion.
 *  @param[in] userdata The data passed as the second element to @a compare.
 *  @return The element matching the criterion, or @a NULL if no such element was found.
 */
void *cu_queue_pop_custom(CUQueue *queue, CUCompareFunc compare, void *userdata);

/** @brief Free all elements matching a criterion.
 *  @param[in] queue The queue for which we want to clear the elements.
 *  @param[in] compare A function returning 0 if an element matches the criterion.
 *  @param[in] notify The function to call for each matching element to free its resources.
 *  @param[in] userdata Data passed as the second element to @a compare.
 */
void cu_queue_clear_matching(CUQueue *queue, CUCompareFunc compare, CUDestroyNotifyFunc notify, void *userdata);

/** @brief Remove a link from the queue without freeing its data.
 *  @param[in] queue The queue for which we want to remove the link.
 *  @param[in] link The link to remove. No check is done if this link is really in the queue.
 */
void cu_queue_delete_link(CUQueue *queue, CUList *link);

/** @brief Run callback for each element in the queue.
 *  @param[in] queue The queue.
 *  @param[in] callback Function to call for each element until @a false is returned.
 *  @param[in] userdata User-defined data to pass to the callback.
 */
void cu_queue_foreach(CUQueue *queue, CUForeachFunc callback, void *userdata);

/** @} */
