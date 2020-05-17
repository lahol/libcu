#pragma once

#include <stdint.h>
#include <cu-types.h>

/* Callback informing about a change of the element’s position.
 * element, position, userdata
 */
typedef void (*CUHeapSetPositionCallback)(void *, uint32_t, void *);

typedef struct {
    uint32_t max_length;
    uint32_t length;
    void **data;

    CUCompareDataFunc compare;
    void *compare_data;

    CUHeapSetPositionCallback set_position_cb;
    void *set_position_cb_data;
} CUHeap;

void cu_heap_init(CUHeap *heap, CUCompareDataFunc compare, void *compare_data);
void cu_heap_init_full(CUHeap *heap, CUCompareDataFunc compare, void *compare_data,
                                     CUHeapSetPositionCallback position_cb, void *position_data);
void cu_heap_clear(CUHeap *heap);
void cu_heap_insert(CUHeap *heap, void *element);
void *cu_heap_pop_root(CUHeap *heap);
void cu_heap_update(CUHeap *heap, uint32_t pos);
