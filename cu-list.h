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

/* Free the whole list. Call notify for each data element. */
void cu_list_free_full(CUList *list, CUDestroyNotifyFunc notify);
