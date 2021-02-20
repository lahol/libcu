/** @file cu-heap.h
 *  A heap with some additional handling of positional information.
 *  @defgroup CUHeap Heap allowing positional information for its elements.
 *  @{
 */
#pragma once

#include <stdint.h>
#include <cu-types.h>

/** @brief Callback informing about a change of the element’s position.
 *  @param[in] 1 Pointer to the element that changed its position.
 *  @param[in] 2 The new position index of the element.
 *  @param[in] 3 Pointer to user defined data.
 */
typedef void (*CUHeapSetPositionCallback)(void *, uint32_t, void *);

/** @brief The heap.
 */
typedef struct {
    uint32_t max_length; /**< Maximal capacity of the heap. */
    uint32_t length; /**< The number of elements on the heap. */
    void **data; /**< Array of pointers to the data on the heap. */

    CUCompareDataFunc compare; /**< Callback to compare to elements on the heap. */
    void *compare_data; /**< User defined data to pass as third element to compare(). */

    CUHeapSetPositionCallback set_position_cb; /**< Callback to inform about the changed position of an element. */
    void *set_position_cb_data; /**< User defined data to pass as third element to set_position_cb(). */
} CUHeap;

/** @brief Initialize a heap.
 *  @param[in] heap Pointer to the heap to initialize.
 *  @param[in] compare Pointer to the callback to compare two elements.
 *  @param[in] compare_data Pointer to the data passed as third element to compare().
 */
void cu_heap_init(CUHeap *heap, CUCompareDataFunc compare, void *compare_data);

/** @brief Initialize a heap with more control.
 *  @param[in] heap Pointer to the heap to initialize.
 *  @param[in] compare Pointer to the callback to compare two elements.
 *  @param[in] compare_data Pointer to the data passed as third element to compare().
 *  @param[in] position_cb Pointer to the callback to inform about the changed position in the heap.
 *  @param[in] position_data Pointer to the data passed as third element to position_cb().
 */
void cu_heap_init_full(CUHeap *heap, CUCompareDataFunc compare, void *compare_data,
                                     CUHeapSetPositionCallback position_cb, void *position_data);

/** @brief Remove all elements and clear the heap.
 *  @param[in] heap Pointer to the heap to clear.
 *  @param[in] destroy_data Function to call to free the resources of each element on the heap.
 */
void cu_heap_clear(CUHeap *heap, CUDestroyNotifyFunc destroy_data);

/** @brief Insert an element on the heap.
 *  @param[in] heap Pointer to the heap to insert into.
 *  @param[in] element Pointer to the element to be inserted.
 */
void cu_heap_insert(CUHeap *heap, void *element);

/** @brief Return the element on top of the heap and remove it.
 *  @param[in] heap Pointer to the heap to pop from.
 *  @return Pointer to the element on top of the heap, or @a NULL if the heap was empty.
 */
void *cu_heap_pop_root(CUHeap *heap);

/** @brief Return the element on top of the heap without removing it.
 *  @param[in] heap Pointer to the heap to peek from.
 *  @return Pointer to the element on top of the heap, or @a NULL if the heap was empty.
 */
void *cu_heap_peek_root(CUHeap *heap);

/** @brief Update the heap, e.g., if an element has changed.
 *  @param[in] heap Pointer to the heap to be updated.
 *  @param[in] pos The index of the element requireing the update.
 */
void cu_heap_update(CUHeap *heap, uint32_t pos);

/** @brief Remove an arbitrary element from the heap.
 *  @param[in] heap Pointer to the heap to remove the element from.
 *  @param[in] pos The index of the element to be removed.
 */
void cu_heap_remove(CUHeap *heap, uint32_t pos);

/** @} */
