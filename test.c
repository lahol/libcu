#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "cu.h"
#include "cu-heap.h"
#include "cu-btree.h"

int cmp_uint(void *a, void *b, void *data)
{
    fprintf(stderr, "compare %u, %u\n", CU_POINTER_TO_UINT(a), CU_POINTER_TO_UINT(b));
    if (CU_POINTER_TO_UINT(a) < CU_POINTER_TO_UINT(b))
        return 1;
    if (CU_POINTER_TO_UINT(a) > CU_POINTER_TO_UINT(b))
        return -1;
    return 0;
}

static
bool visit_node(void *key, void *value, void *nil)
{
    fprintf(stderr, "visit: %u\n", CU_POINTER_TO_UINT(key));
    return true;
}

int main(int argc, char **argv)
{
#if 0
    CUHeap heap;
    cu_heap_init(&heap, (CUCompareDataFunc)cmp_uint, NULL);

    uint32_t j;
    uint32_t ins;
#if 0
    for (j = 0; j < 100; ++j) {
        ins = (uint32_t)(rand() % 32);
        fprintf(stderr, "insert %u\n", ins);
        cu_heap_insert(&heap, CU_UINT_TO_POINTER(ins));
    }
#endif
    cu_heap_insert(&heap, CU_UINT_TO_POINTER(5));
    cu_heap_insert(&heap, CU_UINT_TO_POINTER(7));
    cu_heap_insert(&heap, CU_UINT_TO_POINTER(3));

    fprintf(stderr, "start to pop\n");
    void *r;
    j = 0;
    while (heap.length) {
        ++j;
        r = cu_heap_pop_root(&heap);
        fprintf(stdout, "pop: %u\n", CU_POINTER_TO_UINT(r));
    }
    fprintf(stderr, "popped %u\n", j);

    cu_heap_clear(&heap, NULL);
#endif
    CUBTree *btree = cu_btree_new(NULL, NULL, NULL, NULL);
    uint32_t j, ins;
    for (j = 0; j < 20; ++j) {
        ins = (uint32_t)(rand() % 64);
        fprintf(stderr, "insert: %u\n", ins);
        cu_btree_insert(btree, CU_UINT_TO_POINTER(ins), CU_UINT_TO_POINTER(ins));
#ifdef DEBUG_BTREE_DOT
        fprintf(stdout, "digraph G%u {\n", j);
        cu_btree_foreach(btree, (CUTraverseFunc)visit_node, NULL);
        fprintf(stdout, "}\n");
#endif
    }

#ifdef DEBUG_BTREE_DOT
        fprintf(stdout, "digraph G%u {\n", j);
    cu_btree_foreach(btree, (CUTraverseFunc)visit_node, NULL);
        fprintf(stdout, "}\n");
#endif

    cu_btree_destroy(btree);

    return 0;
}
