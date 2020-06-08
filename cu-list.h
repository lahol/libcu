/* A generic doubly-linked list. */
#pragma once

#include <cu-types.h>

typedef struct _CUList CUList;

struct _CUList {
    void *data;
    CUList *prev;
    CUList *next;
};

/* Add data to the beginning of list and return the new head of the list. */
CUList *cu_list_prepend(CUList *list, void *data);

/* Add data to the end of the list and return the new head of the list. */
CUList *cu_list_append(CUList *list, void *data);

/* Add data after a given link and return new head of the list. */
CUList *cu_list_insert_after(CUList *list, CUList *llink, void *data);

/* Reverse list and return the new head. */
CUList *cu_list_reverse(CUList *list);

/* Delete some link from list and return the new head. */
CUList *cu_list_delete_link(CUList *list, CUList *link);

/* Remove first element matching the data from the list and return the new head. */
CUList *cu_list_remove(CUList *list, void *data);

/* Get the last element in the list. */
CUList *cu_list_last(CUList *list);

/* Just for symmetry, get the first element of the list. */
CUList *cu_list_first(CUList *list);

/* Find an element in the list.
 * data is passed as the second element to compare. */
CUList *cu_list_find_custom(CUList *list, void *data, CUCompareFunc compare);

/* Delete an element matching a criterion and return new head.
 * data is passed as the second element to compare. */
CUList *cu_list_remove_custom(CUList *list, void *data, CUCompareFunc compare);

/* Convenience macros for previous and next list element. */
#define cu_list_previous(list) ((list) ? (list)->prev : NULL)
#define cu_list_next(list) ((list) ? (list)->next : NULL)

/* Free the whole list. Call notify for each data element. */
void cu_list_free_full(CUList *list, CUDestroyNotifyFunc notify);

/* Call func for each element in the list. Stop if func returns false. */
void cu_list_foreach(CUList *list, CUForeachFunc func, void *userdata);
