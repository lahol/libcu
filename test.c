#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "cu.h"
#include "cu-heap.h"

int cmp_uint(void *a, void *b, void *data)
{
    if (CU_POINTER_TO_UINT(a) < CU_POINTER_TO_UINT(b))
        return 1;
    if (CU_POINTER_TO_UINT(a) > CU_POINTER_TO_UINT(b))
        return -1;
    return 0;
}

int main(int argc, char **argv)
{
    CUHeap heap;
    cu_heap_init(&heap, (CUCompareDataFunc)cmp_uint, NULL);

    uint32_t j;
    uint32_t ins;
    for (j = 0; j < 100; ++j) {
        ins = (uint32_t)(rand() % 32);
        fprintf(stderr, "insert %u\n", ins);
        cu_heap_insert(&heap, CU_UINT_TO_POINTER(ins));
    }

    fprintf(stderr, "start to pop\n");
    void *r;
    j = 0;
    while (heap.length) {
        ++j;
        r = cu_heap_pop_root(&heap);
        fprintf(stdout, "pop: %u\n", CU_POINTER_TO_UINT(r));
    }
    fprintf(stderr, "popped %u\n", j);

    cu_heap_clear(&heap);
    return 0;
}
