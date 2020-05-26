#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "cu.h"
#include "cu-heap.h"
#include "cu-avl-tree.h"

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
    CUAVLTree *btree = cu_avl_tree_new(NULL, NULL, NULL, NULL);

#define TOTAL_COUNT 20
    uint32_t j, ins[TOTAL_COUNT];
    for (j = 0; j < 20; ++j) {
        ins[j] = (uint32_t)(rand() % 64);
        fprintf(stderr, "insert: %u\n", ins[j]);
        cu_avl_tree_insert(btree, CU_UINT_TO_POINTER(ins[j]), CU_UINT_TO_POINTER(ins[j]));
#ifdef DEBUG_BTREE_DOT
        fprintf(stdout, "digraph G%u {\n", j);
        cu_avl_tree_foreach(btree, (CUTraverseFunc)visit_node, NULL);
        fprintf(stdout, "}\n");
#endif
    }

#ifdef DEBUG_BTREE_DOT
        fprintf(stdout, "digraph G%u {\n", j);
#endif
    cu_avl_tree_foreach(btree, (CUTraverseFunc)visit_node, NULL);
#ifdef DEBUG_BTREE_DOT
        fprintf(stdout, "}\n");
#endif

    uint32_t index;
    for (j = 0; j < 100; ++j) {
        index = (uint32_t)(rand() % TOTAL_COUNT);
        fprintf(stderr, "remove: %u\n", ins[index]);
        if (cu_avl_tree_remove(btree, CU_UINT_TO_POINTER(ins[index]))) {
            fprintf(stderr, " -> removed\n");
#ifdef DEBUG_BTREE_DOT
            fprintf(stdout, "digraph G%u {\n", j + TOTAL_COUNT);
            cu_avl_tree_foreach(btree, (CUTraverseFunc)visit_node, NULL);
            fprintf(stdout, "}\n");
#endif
        }
    }

#ifdef DEBUG_BTREE_DOT
        fprintf(stdout, "digraph G%u {\n", j + TOTAL_COUNT);
#endif
    cu_avl_tree_foreach(btree, (CUTraverseFunc)visit_node, NULL);
#ifdef DEBUG_BTREE_DOT
        fprintf(stdout, "}\n");
#endif

    cu_avl_tree_destroy(btree);

    return 0;
}
