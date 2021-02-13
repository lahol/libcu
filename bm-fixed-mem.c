#include <stdio.h>
#include <time.h>
#include "cu.h"
#include "cu-memory.h"
#include <stdint.h>
#include <inttypes.h>

int main(int argc, char **argv)
{
    size_t element_size = 152;
    size_t group_size = 0; /*16384/element_size*/;

    uint64_t alloc_count = 1e7;
    uint64_t j;

    clock_t starttime = clock();
    clock_t now;
    CUFixedSizeMemoryPool *pool = cu_fixed_size_memory_pool_new(element_size, group_size);
    cu_fixed_size_memory_pool_release_empty_groups(pool, true);

    uint32_t k;
    for (k = 0; k < 2; ++k) {
        now = clock();
        fprintf(stdout, "pool(%zu, %zu) new: %fs, alloc %" PRIu64 "\n",
                element_size, group_size, (double)(now - starttime)/CLOCKS_PER_SEC, alloc_count);
        starttime = now;
        CUList *list = NULL;
        for (j = 0; j < alloc_count; ++j) {
            list = cu_list_prepend(list, cu_fixed_size_memory_pool_alloc(pool));
        }
        now = clock();
        fprintf(stdout, "alloc %" PRIu64 ": %fs\n", alloc_count, (double)(now - starttime)/CLOCKS_PER_SEC);
        starttime = now;
        while (list) {
            cu_fixed_size_memory_pool_free(pool, list->data);
            list = cu_list_delete_link(list, list);
    /*        if (++j == alloc_count/2)
                cu_fixed_size_memory_pool_release_empty_groups(pool, true);*/
        }
        now = clock();
        fprintf(stdout, "free %" PRIu64 ": %fs\n", alloc_count, (double)(now - starttime)/CLOCKS_PER_SEC);
    }

    return 0;
}
