/** @file cu-list.h
 *  A generic doubly-linked list.
 *  @defgroup CUList Doubly-linked list.
 *  @{
 */
#pragma once

#include <cu-types.h>

typedef struct _CUList CUList;

/** @brief A list element.
 */
struct _CUList {
    void *data; /**< Pointer to the data of the list element. */
    CUList *prev; /**< Pointer to the previous list link. */
    CUList *next; /**< Pointer to the next list link. */
};

/** @brief Add data to the beginning of a list.
 *  @param[in] list The head of the list or @a NULL if the list was empty.
 *  @param[in] data Pointer to the data to insert.
 *  @return The new head of the list.
 */
CUList *cu_list_prepend(CUList *list, void *data);

/** @brief Add data to the end of a list.
 *  @param[in] list The head of the list or @a NULL if the list was empty.
 *  @param[in] data Pointer to the data to insert.
 *  @return The new head of the list.
 */
CUList *cu_list_append(CUList *list, void *data);

/** @brief Add data after a given link.
 *  @param[in] list The head of the list or @a NULL if the list was empty.
 *  @param[in] llink The predecessor of the new element ort @a NULL to insert at the beginning.
 *  @param[in] data The data to insert into the list.
 *  @return The new head of the list.
 */
CUList *cu_list_insert_after(CUList *list, CUList *llink, void *data);

/** @brief Reverse a list.
 *  @param[in] list The current head of the list.
 *  @return the new head of the reversed list.
 */
CUList *cu_list_reverse(CUList *list);

/** @brief Delete a link from list.
 *  @param[in] list The current head of the list.
 *  @param[in] link The link to remove from the list.
 *  @return The new head of the list.
 */
CUList *cu_list_delete_link(CUList *list, CUList *link);

/** @brief Remove first element containing the data from the list.
 *  @param[in] list The current head of the list.
 *  @param[in] data The data to remove from the list
 *  @return The new head of the list.
 */
CUList *cu_list_remove(CUList *list, void *data);

/** @brief Return the last element of a list.
 *  @param[in] list The current head of the list.
 *  @return The last list element, or @a NULL if the list was empty.
 */
CUList *cu_list_last(CUList *list);

/** @brief Get the first element of the list.
 *  @details This is just for symmetry.
 *  @param[in] list The current head of the list.
 *  @return The first list element, or @a NULL if the list was empty.
 */
CUList *cu_list_first(CUList *list);

/** @brief Find an element in the list.
 *  @param[in] list The current head of the list.
 *  @param[in] data Passed as the second element to compare().
 *  @param[in] compare Function to match the data of the element with @a data.
 *  @return Link to the first occurrence of the data, or @a NULL if the list does not contain @a data.
 */
CUList *cu_list_find_custom(CUList *list, void *data, CUCompareFunc compare);

/** @brief Delete an element matching a criterion.
 *  @param[in] list The current head of the list.
 *  @param[in] data Passed as the second element to compare().
 *  @param[in] compare Function to match the data of the element with @a data.
 *  @return The first link matching the criterion, or @a NULL if no element matches.
 */
CUList *cu_list_remove_custom(CUList *list, void *data, CUCompareFunc compare);

/* Convenience macros for previous and next list element. */
/** @brief Get the predecessor link of a list link.
 *  @param[in] list The link to the list element.
 *  @return The predecessor link, or @a NULL if there is none.
 */
#define cu_list_previous(list) ((list) ? (list)->prev : NULL)

/** @brief Get the successor link of a list link.
 *  @param[in] list The link to the list element.
 *  @return The successor link, or @a NULL if there is none.
 */
#define cu_list_next(list) ((list) ? (list)->next : NULL)

/** @brief Free a whole list and its elements
 *  @param[in] list The head of the list to free.
 *  @param[in] notify Function to call for each data element to free its resources.
 */
void cu_list_free_full(CUList *list, CUDestroyNotifyFunc notify);

/** @brief Call a function for each element in a list.
 *  @details Enumeration is stopped if func() returns @a false.
 *  @param[in] list The head of the list.
 *  @param[in] func Function to call for each element.
 *  @param[in] userdata Data passed to func().
 */
void cu_list_foreach(CUList *list, CUForeachFunc func, void *userdata);

/** @} */
