#pragma once

#include <stdlib.h>
#include <stdint.h>

typedef void (*CUMixedHeapListClearFunc)(void *);
typedef void (*CUMixedHeapListCopyFunc)(void *, void *);
typedef int (*CUMixedHeapListCompareFunc)(void *, void *);

typedef struct {
    size_t data_size;
    CUMixedHeapListClearFunc clear_func;
    CUMixedHeapListCopyFunc copy_func;
    CUMixedHeapListCompareFunc compare_heap_func;
    CUMixedHeapListCompareFunc compare_list_func;
} CUMixedHeapListClass;

/* Allow this to be allocated mostly on the stack. */
typedef struct {
    CUMixedHeapListClass cls;
    size_t length;
    size_t max_length;
    uint8_t *data;
    void **heap;
    void *list;

    /* private */
    size_t element_size;
    size_t size;
} CUMixedHeapList;

void cu_mixed_heap_list_init(CUMixedHeapList *heaplist,
                          CUMixedHeapListClass *cls,
                          size_t max_length);
void cu_mixed_heap_list_clear(CUMixedHeapList *heaplist);
void cu_mixed_heap_list_copy(CUMixedHeapList *dst,
                          CUMixedHeapList *src);

void *cu_mixed_heap_list_alloc(CUMixedHeapList *heaplist);
void cu_mixed_heap_list_insert_last_alloc(CUMixedHeapList *heaplist);
void cu_mixed_heap_list_insert_last_alloc_before(CUMixedHeapList *heaplist, void *sibling);
void cu_mixed_heap_list_insert_last_alloc_after(CUMixedHeapList *heaplist, void *sibling);

void *cu_mixed_heap_list_peek_heap_root(CUMixedHeapList *heaplist);
void cu_mixed_heap_list_remove_heap_root(CUMixedHeapList *heaplist);
void cu_mixed_heap_list_remove(CUMixedHeapList *heaplist, void *rm);

void *cu_mixed_heap_list_get_list_head(CUMixedHeapList *heaplist);
void *cu_mixed_heap_list_get_list_next(void *current);
void *cu_mixed_heap_list_get_list_prev(void *current);

void cu_mixed_heap_list_update_list(CUMixedHeapList *heaplist, void *element);
void cu_mixed_heap_list_update_heap(CUMixedHeapList *heaplist, void *element);

uint_fast32_t cu_mixed_heap_list_get_heap_pos(void *element);

void cu_mixed_heap_list_debug_heap(CUMixedHeapList *heaplist);
