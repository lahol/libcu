/** @file cu-mixed-heap-list.h
 *  Data structure organizing the elements in a heap and list at one time.
 *  This can be used to order elements by two criteria, keeping the advantages
 *  of either structure. Thus we can access a largest element in O(1) (or O(log n))
 *  for reorganization, as well as keep neighboring relationships in the list.
 *  @defgroup CUMixedHeapList Mixed heap and list.
 *  @{
 */
#pragma once

#include <stdlib.h>
#include <stdint.h>

/** @brief Callback to clear an element.
 *  @param[in] 1 Pointer to the element to clear.
 */
typedef void (*CUMixedHeapListClearFunc)(void *);

/** @brief Copy an element to another one.
 *  @param[in] 1 Pointer to the destination element.
 *  @param[in] 2 Pointer to the source element.
 */
typedef void (*CUMixedHeapListCopyFunc)(void *, void *);

/** @brief Compare two elements.
 *  @param[in] 1 Pointer to the first element.
 *  @param[in] 2 Pointer to the second element.
 *  @return A value large 0 if the second element is larger than the first.
 */
typedef int (*CUMixedHeapListCompareFunc)(void *, void *);

/** @brief Main configuration of the heap list.
 */
typedef struct {
    size_t data_size; /**< Size of a single element.*/
    CUMixedHeapListClearFunc clear_func; /**< Function to clear an element. */
    CUMixedHeapListCopyFunc copy_func; /**< Function to copy an element. */
    CUMixedHeapListCompareFunc compare_heap_func; /**< Compare two elements for the heap. */
    CUMixedHeapListCompareFunc compare_list_func; /**< Compare two elements for the list. */
} CUMixedHeapListClass;

/** @brief A mixed heap and list.
 */
typedef struct {
    CUMixedHeapListClass cls; /**< The configuration of the heap list. */
    size_t length; /**< The number of elements in the structure. */
    size_t max_length; /**< The maximum capacity of the structure. */
    uint8_t *data; /**< Data containing the elements. */
    void **heap; /**< Array of pointers to data, managed as heap. */
    void *list; /**< Pointer to the head of the list. */

    /* private */
    size_t element_size; /**< Size of a single element, including metadata. */
    size_t size; /**< Total allocated size. */
} CUMixedHeapList;

/** @brief Initialize a mixed heap list.
 *  @param[in] heaplist The heap list to initialize.
 *  @param[in] cls The configuration of the structure.
 *  @param[in] max_length The maximum capacity of the structure.
 */
void cu_mixed_heap_list_init(CUMixedHeapList *heaplist,
                          CUMixedHeapListClass *cls,
                          size_t max_length);

/** @brief Clear a mixed heap list and free all resources.
 *  @param[in] heaplist The heap list to clear.
 */
void cu_mixed_heap_list_clear(CUMixedHeapList *heaplist);

/** @brief Copy a mixed heap list.
 *  @param[in] dst The destination.
 *  @param[in] src The source.
 */
void cu_mixed_heap_list_copy(CUMixedHeapList *dst,
                          CUMixedHeapList *src);

/** @brief Get a new element from the structure.
 *  @details As all memory is managed inside the structure, we have to get the memory from here.
 *  @param[in] heaplist The mixed heap list.
 *  @return A pointer to a newly reserved element, or @a NULL if all elements have been used.
 */
void *cu_mixed_heap_list_alloc(CUMixedHeapList *heaplist);

/** @brief Insert the last allocated element, maintaining the order.
 *  @param[in] heaplist The heap list to insert into.
 */
void cu_mixed_heap_list_insert_last_alloc(CUMixedHeapList *heaplist);

/** @brief Insert the last allocated element before another element.
 *  @param[in] heaplist The heap list to insert into.
 *  @param[in] sibling The sibling before which to insert.
 */
void cu_mixed_heap_list_insert_last_alloc_before(CUMixedHeapList *heaplist, void *sibling);

/** @brief Insert the last allocated element after another element.
 *  @param[in] heaplist The heap list to insert into.
 *  @param[in] sibling The sibling after which to insert.
 */
void cu_mixed_heap_list_insert_last_alloc_after(CUMixedHeapList *heaplist, void *sibling);

/** @brief Return a pointer to the element on top of the heap, without removing it.
 *  @param[in] heaplist The heap list to peek from.
 *  @return A pointer to the element on top of the heap.
 */
void *cu_mixed_heap_list_peek_heap_root(CUMixedHeapList *heaplist);

/** @brief Remove the root of the heap.
 *  @param[in] heaplist The heap list to remove from.
 */
void cu_mixed_heap_list_remove_heap_root(CUMixedHeapList *heaplist);

/** @brief Remove a custom element from the heap.
 *  @param[in] heaplist The heap list to remove from.
 *  @param[in] rm The element to be removed.
 */
void cu_mixed_heap_list_remove(CUMixedHeapList *heaplist, void *rm);

/** @brief Get the head of the list in the heap list.
 *  @param[in] heaplist The heap for which to get the list head.
 *  @return A pointer to the data element at the head of the list.
 */
void *cu_mixed_heap_list_get_list_head(CUMixedHeapList *heaplist);

/** @brief Get the next element in the list of the heap list.
 *  @param[in] current The element for which to determine the next element.
 *  @return A pointer to the element next after current, or @a NULL if there is none.
 */
void *cu_mixed_heap_list_get_list_next(void *current);

/** @brief Get the previous element in the list of the heap list.
 *  @param[in] current The element for which to determine the previous element.
 *  @return A pointer to the element before the current, or @a NULL if there is none.
 */
void *cu_mixed_heap_list_get_list_prev(void *current);

/** @brief Update the list of the heap list after an element has changed.
 *  @param[in] heaplist The heap list to update.
 *  @param[in] element The element requireing the heap list to be updated.
 */
void cu_mixed_heap_list_update_list(CUMixedHeapList *heaplist, void *element);

/** @brief Update the heap of the heap list after an element has changed.
 *  @param[in] heaplist The heap list to update.
 *  @param[in] element The element requireing the heap list to be updated.
 */
void cu_mixed_heap_list_update_heap(CUMixedHeapList *heaplist, void *element);

/** @brief Get the index in the heap of a given element.
 *  @param[in] element The element for which to get the heap index.
 *  @return The index of the element in the heap.
 */
uint_fast32_t cu_mixed_heap_list_get_heap_pos(void *element);

/** @} */
