#include "cu-heap.h"
#include "cu-memory.h"
#include "cu.h"
#include <assert.h>

void cu_heap_init_full(CUHeap *heap, CUCompareDataFunc compare, void *compare_data,
                                     CUHeapSetPositionCallback position_cb, void *position_data)
{
    memset(heap, 0, sizeof(CUHeap));

    heap->compare = compare;
    heap->compare_data = compare_data;

    heap->set_position_cb = position_cb;
    heap->set_position_cb_data = position_data;
}

void cu_heap_init(CUHeap *heap, CUCompareDataFunc compare, void *compare_data)
{
    cu_heap_init_full(heap, compare, compare_data, NULL, NULL);
}

void cu_heap_clear(CUHeap *heap, CUDestroyNotifyFunc destroy_data)
{
    if (heap) {
        if (destroy_data) {
            uint32_t j;
            for (j = 0; j < heap->length; ++j)
                destroy_data(heap->data[j]);
        }
        cu_free(heap->data);
        memset(heap, 0, sizeof(CUHeap));
    }
}

static inline
void _cu_heap_exchange_links(CUHeap *heap, uint32_t j1, uint32_t j2)
{
    void *t = heap->data[j1];
    heap->data[j1] = heap->data[j2];
    heap->data[j2] = t;
    if (heap->set_position_cb) {
        heap->set_position_cb(heap->data[j1], j1, heap->set_position_cb_data);
        heap->set_position_cb(heap->data[j2], j2, heap->set_position_cb_data);
    }
}

static
void _cu_heap_upheap(CUHeap *heap, uint32_t pos)
{
    while (pos && heap->compare(heap->data[(pos-1)/2], heap->data[pos], heap->compare_data) < 0) {
        _cu_heap_exchange_links(heap, pos, (pos-1)/2);
        pos = (pos-1)/2;
    }
}

static
void _cu_heap_downheap(CUHeap *heap, uint32_t pos)
{
    uint32_t j;
    while (2 * pos + 1 < heap->length) {
        j = 2 * pos + 1;
        /* select larger child */
        if (j < heap->length - 1 && heap->compare(heap->data[j], heap->data[j+1], heap->compare_data) < 0)
            ++j;
        if (heap->compare(heap->data[pos], heap->data[j], heap->compare_data) > 0)
            break;
        _cu_heap_exchange_links(heap, pos, j);
        pos = j;
    }
}

static
void _cu_heap_reheap(CUHeap *heap, uint32_t pos)
{
    if (pos && heap->compare(heap->data[(pos-1)/2], heap->data[pos], heap->compare_data) < 0) {
        _cu_heap_upheap(heap, pos);
    }
    else if ((2*pos + 1 < heap->length && heap->compare(heap->data[pos], heap->data[2*pos+1], heap->compare_data) < 0) ||
             (2*pos + 2 < heap->length && heap->compare(heap->data[pos], heap->data[2*pos+2], heap->compare_data) < 0)) {
        _cu_heap_downheap(heap, pos);
    }
}

void cu_heap_insert(CUHeap *heap, void *element)
{
    if (cu_unlikely(!heap))
        return;
    if (cu_unlikely(heap->length == heap->max_length)) {
        heap->max_length += 512; /* Let heap grow linearly (on 64 bit systems, use increments of 4K). */
        heap->data = cu_realloc(heap->data, heap->max_length * sizeof(void *));
    }
    assert(heap->max_length);

    heap->data[heap->length] = element;
    if (heap->set_position_cb)
        heap->set_position_cb(element, heap->length, heap->set_position_cb_data);
    _cu_heap_upheap(heap, heap->length++);
}

void *cu_heap_pop_root(CUHeap *heap)
{
    if (cu_unlikely(!heap || !heap->data || heap->length == 0))
        return NULL;

    void *data = heap->data[0];

    _cu_heap_exchange_links(heap, 0, --heap->length);
    _cu_heap_downheap(heap, 0);
    
    if (heap->set_position_cb)
        heap->set_position_cb(data, (uint32_t)(-1), heap->set_position_cb_data);

    return data;
}

void *cu_heap_peek_root(CUHeap *heap)
{
    if (cu_unlikely(!heap || !heap->data || !heap->length))
        return NULL;
    return heap->data[0];
}

void cu_heap_update(CUHeap *heap, uint32_t pos)
{
    if (cu_unlikely(!heap || !heap->data || pos >= heap->length))
        return;
    _cu_heap_reheap(heap, pos);
}

void cu_heap_remove(CUHeap *heap, uint32_t pos)
{
    if (cu_unlikely(!heap || !heap->data || pos >= heap->length))
        return;
    _cu_heap_exchange_links(heap, pos, --heap->length);
    /* Can happen in both directions, e.g., if we are in another subtree. */
    _cu_heap_reheap(heap, pos);
}
