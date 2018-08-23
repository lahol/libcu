#include "cu-mixed-heap-list.h"
#include "cu-memory.h"
#include <string.h>
#include "cu.h"

static __attribute__((always_inline)) inline
void *_cu_mixed_heap_list_move_offset_pointer(void *ptr, void *src_base, void *dst_base)
{
    return ptr ? dst_base + (ptr - src_base) : NULL;
}


/* heaplist elements:
 *  [heapindex|ptr_prev|ptr_next|<reserved>|<data>]
 *  4*void *+ data_size
 */

const size_t heaplist_data_offset = 4 * sizeof(void *);
const size_t heaplist_list_prev_offset = sizeof(void *);
const size_t heaplist_list_next_offset = 2 * sizeof(void *);

#define HEAPLIST_LIST_PREV_PTR(element) (*((void **)((element) + heaplist_list_prev_offset)))
#define HEAPLIST_LIST_NEXT_PTR(element) (*((void **)((element) + heaplist_list_next_offset)))
#define HEAPLIST_LIST_DATA_PTR(element) (((void *)((element) + heaplist_data_offset)))
#define HEAPLIST_LIST_INDEX_PTR(element) (*((void **)(element)))

static inline
void cu_mixed_heap_list_shallow_init(CUMixedHeapList *heaplist,
                                  CUMixedHeapListClass *cls,
                                  size_t max_length)
{
    heaplist->cls = *cls;
    heaplist->max_length = max_length;

    heaplist->element_size = 4 * sizeof(void *) + cls->data_size;
    heaplist->size = max_length * heaplist->element_size;
    heaplist->data = cu_alloc0(heaplist->size);
    heaplist->heap = cu_alloc(max_length * sizeof(void *));

}

void cu_mixed_heap_list_init(CUMixedHeapList *heaplist,
                          CUMixedHeapListClass *cls,
                          size_t max_length)
{
    if (!cls || !heaplist)
        return;
    cu_mixed_heap_list_shallow_init(heaplist, cls, max_length);

    heaplist->length = 0;
    heaplist->list = NULL;
    size_t j;
    for (j = 0; j < max_length; ++j) {
        heaplist->heap[j] = j * heaplist->element_size + heaplist->data;
    }
}

static inline
void cu_mixed_heap_list_shallow_clear(CUMixedHeapList *heaplist)
{
    size_t offset;
    if (heaplist->cls.clear_func) {
        for (offset = heaplist_data_offset; offset < heaplist->size; offset += heaplist->element_size)
            heaplist->cls.clear_func(heaplist->data + offset);
    }
    cu_free(heaplist->data);
    cu_free(heaplist->heap);
}

void cu_mixed_heap_list_clear(CUMixedHeapList *heaplist)
{
    cu_mixed_heap_list_shallow_clear(heaplist);
    memset(heaplist, 0, sizeof(CUMixedHeapList));
}

void cu_mixed_heap_list_copy(CUMixedHeapList *dst,
                          CUMixedHeapList *src)
{
    if (dst == src)
        return;
    if (dst->max_length != src->max_length) {
        cu_mixed_heap_list_shallow_clear(dst);
        cu_mixed_heap_list_shallow_init(dst, &src->cls, src->max_length);
    }
    size_t j;
    for (j = 0; j < src->max_length; ++j) {
        dst->heap[j] = _cu_mixed_heap_list_move_offset_pointer(src->heap[j], src->data, dst->data);
    }
    dst->length = src->length;
    memcpy(dst->data, src->data, src->element_size * src->max_length);
    for (j = 0; j < src->max_length; ++j) {
        HEAPLIST_LIST_PREV_PTR(dst->data + j * dst->element_size) =
            _cu_mixed_heap_list_move_offset_pointer(HEAPLIST_LIST_PREV_PTR(src->data + j * src->element_size),
                    src->data, dst->data);
        HEAPLIST_LIST_NEXT_PTR(dst->data + j * dst->element_size) =
            _cu_mixed_heap_list_move_offset_pointer(HEAPLIST_LIST_NEXT_PTR(src->data + j * src->element_size),
                    src->data, dst->data);
    }

    dst->list = _cu_mixed_heap_list_move_offset_pointer(src->list, src->data, dst->data);
}

static inline
void _cu_mixed_heap_exchange_links(CUMixedHeapList *heaplist, size_t j1, size_t j2)
{
    void *tmp;
    tmp = heaplist->heap[j1];
    heaplist->heap[j1] = heaplist->heap[j2];
    heaplist->heap[j2] = tmp;
    HEAPLIST_LIST_INDEX_PTR(heaplist->heap[j1]) = CU_UINT_TO_POINTER(j1);
    HEAPLIST_LIST_INDEX_PTR(heaplist->heap[j2]) = CU_UINT_TO_POINTER(j2);
}

static inline
void _cu_mixed_heap_upheap(CUMixedHeapList *heaplist, size_t k)
{
    while (k && heaplist->cls.compare_heap_func(HEAPLIST_LIST_DATA_PTR(heaplist->heap[(k-1)/2]), HEAPLIST_LIST_DATA_PTR(heaplist->heap[k])) < 0) {
        _cu_mixed_heap_exchange_links(heaplist, k, (k-1)/2);
        k=(k-1)/2;
    }
}

static inline
void _cu_mixed_heap_downheap(CUMixedHeapList *heaplist, size_t k)
{
    size_t j;
    while (2*k + 1 < heaplist->length) {
        j = 2*k + 1;
        /* select larger child */
        if (j < heaplist->length - 1 && heaplist->cls.compare_heap_func(HEAPLIST_LIST_DATA_PTR(heaplist->heap[j]), HEAPLIST_LIST_DATA_PTR(heaplist->heap[j+1])) < 0)
            ++j;
        if (heaplist->cls.compare_heap_func(HEAPLIST_LIST_DATA_PTR(heaplist->heap[k]), HEAPLIST_LIST_DATA_PTR(heaplist->heap[j])) > 0)
            break;
        _cu_mixed_heap_exchange_links(heaplist, k, j);
        k = j;
    }
}

static inline
void _cu_mixed_heap_reheap(CUMixedHeapList *heaplist, size_t k)
{
    /* check if parent is of smaller priority */
    if (k && heaplist->cls.compare_heap_func(HEAPLIST_LIST_DATA_PTR(heaplist->heap[(k-1)/2]),
                                             HEAPLIST_LIST_DATA_PTR(heaplist->heap[k])) < 0) {
        _cu_mixed_heap_upheap(heaplist, k);
    }
    else if ((2*k + 1 < heaplist->length && heaplist->cls.compare_heap_func(HEAPLIST_LIST_DATA_PTR(heaplist->heap[k]),
                                                                        HEAPLIST_LIST_DATA_PTR(heaplist->heap[2*k+1])) < 0) ||
             (2*k + 2 < heaplist->length && heaplist->cls.compare_heap_func(HEAPLIST_LIST_DATA_PTR(heaplist->heap[k]),
                                                                          HEAPLIST_LIST_DATA_PTR(heaplist->heap[2*k+2])) < 0)) {
        _cu_mixed_heap_downheap(heaplist, k);
    }
}

static inline
void _cu_mixed_heap_list_list_insert(CUMixedHeapList *heaplist, void *element)
{
    void *prev = NULL, *next;
    for (next = heaplist->list;
         next && heaplist->cls.compare_list_func(HEAPLIST_LIST_DATA_PTR(element), HEAPLIST_LIST_DATA_PTR(next)) >= 0;
         prev = next, next = HEAPLIST_LIST_NEXT_PTR(next)) {
    }
    HEAPLIST_LIST_NEXT_PTR(element) = next;
    HEAPLIST_LIST_PREV_PTR(element) = prev;
    if (next)
        HEAPLIST_LIST_PREV_PTR(next) = element;
    if (prev)
        HEAPLIST_LIST_NEXT_PTR(prev) = element;
    else
        heaplist->list = element;
}

static inline
void *_cu_mixed_heap_list_heap_insert(CUMixedHeapList *heaplist)
{
    void *ins = heaplist->heap[heaplist->length];
    HEAPLIST_LIST_INDEX_PTR(ins) = CU_UINT_TO_POINTER(heaplist->length);
    _cu_mixed_heap_upheap(heaplist, heaplist->length++);

    return ins;
}

static inline
void _cu_mixed_heap_list_list_remove(CUMixedHeapList *heaplist, void *element)
{
    void *prev = HEAPLIST_LIST_PREV_PTR(element);
    void *next = HEAPLIST_LIST_NEXT_PTR(element);
    if (next)
        HEAPLIST_LIST_PREV_PTR(next) = HEAPLIST_LIST_PREV_PTR(element);
    if (prev)
        HEAPLIST_LIST_NEXT_PTR(prev) = HEAPLIST_LIST_NEXT_PTR(element);
    else
        heaplist->list = next;
}

void *cu_mixed_heap_list_alloc(CUMixedHeapList *heaplist)
{
    if (heaplist->length >= heaplist->max_length)
        return NULL;
    return HEAPLIST_LIST_DATA_PTR(heaplist->heap[heaplist->length]);
}

void cu_mixed_heap_list_insert_last_alloc(CUMixedHeapList *heaplist)
{
    void *ins = _cu_mixed_heap_list_heap_insert(heaplist);
    _cu_mixed_heap_list_list_insert(heaplist, ins);
}

void cu_mixed_heap_list_insert_last_alloc_before(CUMixedHeapList *heaplist, void *sibling)
{
    sibling -= heaplist_data_offset;
    void *ins = _cu_mixed_heap_list_heap_insert(heaplist);
    void *prev = HEAPLIST_LIST_PREV_PTR(sibling);
    HEAPLIST_LIST_NEXT_PTR(ins) = sibling;
    HEAPLIST_LIST_PREV_PTR(sibling) = ins;
    HEAPLIST_LIST_PREV_PTR(ins) = prev;
    if (prev) {
        HEAPLIST_LIST_NEXT_PTR(prev) = ins;
    }
    else {
        heaplist->list = ins;
    }
}

void cu_mixed_heap_list_insert_last_alloc_after(CUMixedHeapList *heaplist, void *sibling)
{
    sibling -= heaplist_data_offset;
    void *ins = _cu_mixed_heap_list_heap_insert(heaplist);
    void *next = HEAPLIST_LIST_NEXT_PTR(sibling);
    HEAPLIST_LIST_PREV_PTR(ins) = sibling;
    HEAPLIST_LIST_NEXT_PTR(sibling) = ins;
    HEAPLIST_LIST_NEXT_PTR(ins) = next;
    if (next) {
        HEAPLIST_LIST_PREV_PTR(next) = ins;
    }
}

void *cu_mixed_heap_list_peek_heap_root(CUMixedHeapList *heaplist)
{
    if (heaplist->length == 0)
        return NULL;
    return HEAPLIST_LIST_DATA_PTR(heaplist->heap[0]);
}

void cu_mixed_heap_list_remove_heap_root(CUMixedHeapList *heaplist)
{
    if (heaplist->length == 0)
        return;
    void *rm = heaplist->heap[0];
    _cu_mixed_heap_exchange_links(heaplist, 0, --heaplist->length);
    _cu_mixed_heap_downheap(heaplist, 0);
    _cu_mixed_heap_list_list_remove(heaplist, rm);
}

void cu_mixed_heap_list_remove(CUMixedHeapList *heaplist, void *rm)
{
    if (heaplist->length == 0 || rm == NULL)
        return;

    rm -= heaplist_data_offset;
    size_t heap_pos = CU_POINTER_TO_UINT(HEAPLIST_LIST_INDEX_PTR(rm));

    _cu_mixed_heap_exchange_links(heaplist, heap_pos, --heaplist->length);
    /* was downheap */
    _cu_mixed_heap_reheap(heaplist, heap_pos);
    _cu_mixed_heap_list_list_remove(heaplist, rm);
}

void *cu_mixed_heap_list_get_list_head(CUMixedHeapList *heaplist)
{
    if (heaplist->list)
        return HEAPLIST_LIST_DATA_PTR(heaplist->list);
    return NULL;
}

void *cu_mixed_heap_list_get_list_next(void *current)
{
    if (!current)
        return NULL;
    current -= heaplist_data_offset;
    void *next = HEAPLIST_LIST_NEXT_PTR(current);
    if (next)
       return HEAPLIST_LIST_DATA_PTR(next);
    return NULL;
}

void *cu_mixed_heap_list_get_list_prev(void *current)
{
    if (!current)
        return NULL;
    current -= heaplist_data_offset;
    void *prev = HEAPLIST_LIST_PREV_PTR(current);
    if (prev)
        return HEAPLIST_LIST_DATA_PTR(prev);
    return NULL;
}

void cu_mixed_heap_list_update_list(CUMixedHeapList *heaplist, void *element)
{
    element -= heaplist_data_offset;
    _cu_mixed_heap_list_list_remove(heaplist, element);
    _cu_mixed_heap_list_list_insert(heaplist, element);
}

void cu_mixed_heap_list_update_heap(CUMixedHeapList *heaplist, void *element)
{
    element -= heaplist_data_offset;
    _cu_mixed_heap_reheap(heaplist, CU_POINTER_TO_UINT(HEAPLIST_LIST_INDEX_PTR(element)));
}

uint_fast32_t cu_mixed_heap_list_get_heap_pos(void *element)
{
    if (!element)
        return -1;
    element -= heaplist_data_offset;
    return CU_POINTER_TO_UINT(HEAPLIST_LIST_INDEX_PTR(element));
}
