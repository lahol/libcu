#include "cu-memory.h"
#include <stdlib.h>
#include <memory.h>
#include <assert.h>

#include <stdint.h>
#include "cu.h"
#include "cu-heap.h"
#include "cu-btree.h"

#ifdef DEBUG
#include <stdio.h>
#include <inttypes.h>
#endif

static CUMemoryHandler memhandler = {
    .alloc   = malloc,
    .realloc = realloc,
    .free    = free
};

void *cu_alloc(size_t size)
{
    void *ptr = memhandler.alloc(size);
    if (!ptr && size)
        exit(1);
    return ptr;
}

void *cu_alloc0(size_t size)
{
    void *ptr = cu_alloc(size);
    if (ptr && size)
        memset(ptr, 0, size);
    return ptr;
}

void cu_free(void *ptr)
{
    if (ptr)
        memhandler.free(ptr);
}

void *cu_realloc(void *ptr, size_t size)
{
    ptr = memhandler.realloc(ptr, size);
    if (!ptr && size)
        exit(1);
    return ptr;
}

void cu_set_memory_handler(CUMemoryHandler *handler)
{
    if (handler) {
        memhandler.alloc = handler->alloc ? handler->alloc : malloc;
        memhandler.realloc = handler->realloc ? handler->realloc : realloc;
        memhandler.free = handler->free ? handler->free : free;
    }
    else {
        memhandler.alloc = malloc;
        memhandler.realloc = realloc;
        memhandler.free = free;
    }
}

/****************************
 *  Fixed size memory pool.
 ****************************/
/* Size of the header in each group, (head id, number initialized, number free, position in heap; each 4 bytes) */
#define MEMORY_GROUP_HEADER_SIZE       16

#define MEMORY_GROUP_HEADER_HEAD(group) (*((uint32_t *)((group))))
#define MEMORY_GROUP_HEADER_NUM_INIT(group) (*((uint32_t *)((void *)(group) + 4)))
#define MEMORY_GROUP_HEADER_NUM_FREE(group) (*((uint32_t *)((void *)(group) + 8)))
#define MEMORY_GROUP_HEADER_HEAP_POS(group) (*((uint32_t *)((void *)(group) + 12)))

#define MEMORY_GROUP_ELEMENT(group, block, element_size) (*((uint32_t *)((void *)(group) +\
                MEMORY_GROUP_HEADER_SIZE + (block) * (element_size))))
#define MEMORY_GROUP_ELEMENT_PTR(group, block, element_size) ((void *)((void *)(group) +\
                MEMORY_GROUP_HEADER_SIZE + (block) * (element_size)))

/* Default allocation size for a memory group. */
#ifndef CFG_FM_POOL_DEFAULT_GROUP_ALLOC_SIZE
#define CFG_FM_POOL_DEFAULT_GROUP_ALLOC_SIZE 16384
#endif

struct _CUFixedSizeMemoryPool {
    uint32_t group_size;    /* number of elements in each group. */
    uint32_t element_size;  /* size of each element. */
    size_t alloc_size;      /* size to allocate per group. */

    size_t total_free;      /* number of free elements in the whole pool. */

    /* FIXME: use a balanced tree here. */
    CUHeap free_memory;
    CUBTree *managed_memory;

    uint32_t release_empty_groups : 1; /* If the alloc count of a group drops to zero, free this block. */
};

static
void _cu_fixed_size_memory_pool_set_heap_position(void *mem_group, uint32_t pos, CUFixedSizeMemoryPool *pool)
{
    MEMORY_GROUP_HEADER_HEAP_POS(mem_group) = pos;
}

static
int _cu_fixed_size_memory_pool_compare_free_space(void *gr1, void *gr2, CUFixedSizeMemoryPool *pool)
{
    /* Less free space should be on top. */
    if (MEMORY_GROUP_HEADER_NUM_FREE(gr1) > MEMORY_GROUP_HEADER_NUM_FREE(gr2))
        return -1;
    if (MEMORY_GROUP_HEADER_NUM_FREE(gr1) < MEMORY_GROUP_HEADER_NUM_FREE(gr2))
        return 1;
    return 0;
}

static
int _cu_fixed_size_memory_pool_compare_memory_range(void *ptr, void *group, CUFixedSizeMemoryPool *pool)
{
    /* If the pointer is left of the group start, the pointer is smaller.
     * If it is outside the group range, it is larger.
     * Otherwise, it is inside the group and considered equal
     */
    if (ptr < group)
        return 1;
    if (ptr > group + pool->alloc_size)
        return -1;
    return 0;
}

static
void *_cu_fixed_size_memory_pool_group_new(CUFixedSizeMemoryPool *pool)
{
    void *group = cu_alloc(pool->alloc_size);
    MEMORY_GROUP_HEADER_HEAD(group) = 0;
    MEMORY_GROUP_HEADER_NUM_INIT(group) = 0;
    MEMORY_GROUP_HEADER_NUM_FREE(group) = pool->group_size;

    pool->total_free += pool->group_size;

    cu_btree_insert(pool->managed_memory, group, group);

#ifdef DEBUG
    fprintf(stderr, "new memory group %p, head: %u, free: %u, init: %u\n", group,
            MEMORY_GROUP_HEADER_HEAD(group), MEMORY_GROUP_HEADER_NUM_FREE(group), MEMORY_GROUP_HEADER_NUM_INIT(group));
#endif

    return group;
}

#if 0
static
void _cu_fixed_size_memory_pool_group_free(CUFixedSizeMemoryPool *pool, CUList *group_link)
{
    cu_free(group_link->data);
    pool->memory_groups = cu_list_delete_link(pool->memory_groups, group_link);
    pool->total_free -= pool->group_size;
}
#endif

/* Create a new memory pool in which all elements have size element_size. The pool internally
 * will be group by blocks of group_size elements. Set this to 0 to get a reasonable default
 * size.
 */
CUFixedSizeMemoryPool *cu_fixed_size_memory_pool_new(size_t element_size, size_t group_size)
{
    CUFixedSizeMemoryPool *pool = cu_alloc0(sizeof(CUFixedSizeMemoryPool));

    pool->element_size = ROUND_TO_8(element_size);
    if (pool->element_size == 0)
        pool->element_size = 8;
    assert(pool->element_size <= (CFG_FM_POOL_DEFAULT_GROUP_ALLOC_SIZE - MEMORY_GROUP_HEADER_SIZE));
    if (group_size == 0)
        pool->group_size = (CFG_FM_POOL_DEFAULT_GROUP_ALLOC_SIZE - MEMORY_GROUP_HEADER_SIZE) / pool->element_size;
    else
        pool->group_size = group_size;

    pool->alloc_size = pool->group_size * pool->element_size + MEMORY_GROUP_HEADER_SIZE;

    cu_heap_init_full(&pool->free_memory,
                      (CUCompareDataFunc)_cu_fixed_size_memory_pool_compare_free_space,
                      pool,
                      (CUHeapSetPositionCallback)_cu_fixed_size_memory_pool_set_heap_position,
                      pool);
    pool->managed_memory = cu_btree_new_full((CUCompareDataFunc)_cu_fixed_size_memory_pool_compare_memory_range,
                                             pool,
                                             NULL,                             /* Do not free keys (group indices). */
                                             (CUDestroyNotifyFunc)cu_free,     /* Free groups (value). */
                                             false);                           /* Do not use fixes size memory pool (recursion!). */
#ifdef DEBUG
    fprintf(stderr, "new pool, element_size: %u, group_size: %u, alloc_size: %zu\n",
            pool->element_size, pool->group_size, pool->alloc_size);
#endif

    return pool;
}

/* If set, release memory of empty groups. */
void cu_fixed_size_memory_pool_release_empty_groups(CUFixedSizeMemoryPool *pool, bool do_release)
{
    if (cu_unlikely(!pool))
        return;
    pool->release_empty_groups = do_release;

    /* If set, release all empty groups. */
    if (do_release) {
#if 0
        CUList *link = pool->memory_groups;
        CUList *next;

        while (link) {
            next = link->next;
            if (MEMORY_GROUP_HEADER_NUM_FREE(link->data) == pool->group_size) {
                _cu_fixed_size_memory_pool_group_free(pool, link);
            }
            link = next;
        }
#endif
    }
}

/* Clear all data from the pool. */
void cu_fixed_size_memory_pool_clear(CUFixedSizeMemoryPool *pool)
{
    if (pool) {
        cu_heap_clear(&pool->free_memory, NULL);
        cu_btree_clear(pool->managed_memory);
        pool->total_free = 0;
    }
}

/* Destroy the pool. */
void cu_fixed_size_memory_pool_destroy(CUFixedSizeMemoryPool *pool)
{
    if (pool) {
        cu_fixed_size_memory_pool_clear(pool);
        cu_btree_destroy(pool->managed_memory);
        cu_free(pool);
    }
}

/* Get a new element from the pool. */
void *cu_fixed_size_memory_pool_alloc(CUFixedSizeMemoryPool *pool)
{
    if (cu_unlikely(!pool))
        return NULL;
    /* The heap is ordered in such a way, that groups with the least free space are on top.
     * If the memory group was the one with the least free space, it will end up on top again.
     * Therefore, there is no need to pull it from the heap and put it back on again.
     */
    void *mem_group = cu_heap_peek_root(&pool->free_memory);
    bool is_new_group = false;
    if (cu_unlikely(!mem_group)) {
        /* No free memory available. */
        mem_group = _cu_fixed_size_memory_pool_group_new(pool);
        is_new_group = true;
    }
    assert(mem_group != NULL);

#ifdef DEBUG
    fprintf(stderr, "alloc in group %p, head: %u, free: %u, init: %u\n",
            mem_group, MEMORY_GROUP_HEADER_HEAD(mem_group), MEMORY_GROUP_HEADER_NUM_FREE(mem_group),
            MEMORY_GROUP_HEADER_NUM_INIT(mem_group));
#endif

    /* If there are any uninitialized elements in this group, initialize the next one. */
    if (MEMORY_GROUP_HEADER_NUM_INIT(mem_group) < pool->group_size) {
        MEMORY_GROUP_ELEMENT(mem_group, MEMORY_GROUP_HEADER_NUM_INIT(mem_group), pool->element_size) =
            MEMORY_GROUP_HEADER_NUM_INIT(mem_group) + 1;
        ++MEMORY_GROUP_HEADER_NUM_INIT(mem_group);
    }

    /* The block of the next unused element is stored in the header. */
    void *ret = MEMORY_GROUP_ELEMENT_PTR(mem_group, MEMORY_GROUP_HEADER_HEAD(mem_group), pool->element_size);
    --MEMORY_GROUP_HEADER_NUM_FREE(mem_group);
    --pool->total_free;
    if (MEMORY_GROUP_HEADER_NUM_FREE(mem_group)) {
        /* If there are unsued elements left, store the value from the last memory location (the link to
         * the next unused element) in the head. */
        MEMORY_GROUP_HEADER_HEAD(mem_group) = *((uint32_t *)ret);

        /* We still have free memory in this group. Push it back to the heap. */
        if (is_new_group)
            cu_heap_insert(&pool->free_memory, mem_group);
    }
    else {
        /* There are no unused elements in this group. Set to invalid. */
        MEMORY_GROUP_HEADER_HEAD(mem_group) = 0xffffffff;

        /* If this was on top of the heap, remove it. */
        if (cu_likely(!is_new_group))
            cu_heap_pop_root(&pool->free_memory);
    }

#ifdef DEBUG
    fprintf(stderr, "allocated %p from group %p, new head: %u\n",
            ret, mem_group, MEMORY_GROUP_HEADER_HEAD(mem_group));
#endif

    return ret;
}

/* Return an element to the pool. */
bool cu_fixed_size_memory_pool_free(CUFixedSizeMemoryPool *pool, void *ptr)
{
    if (cu_unlikely(!pool))
        return false;
    void *mem_group = NULL;
    if (!cu_btree_find(pool->managed_memory, ptr, &mem_group)) {
        return false;
    }

#ifdef DEBUG
    fprintf(stderr, "ptr %p in group %p\n", ptr, mem_group);
#endif

    uint32_t index = (ptr - mem_group - MEMORY_GROUP_HEADER_SIZE) / pool->element_size;

    MEMORY_GROUP_ELEMENT(mem_group, index, pool->element_size) = MEMORY_GROUP_HEADER_HEAD(mem_group);
    MEMORY_GROUP_HEADER_HEAD(mem_group) = index;
    ++MEMORY_GROUP_HEADER_NUM_FREE(mem_group);
    ++pool->total_free;
#ifdef DEBUG
    fprintf(stderr, "free, index: %u, new head: %u, free: %u, init: %u, heappos: %u\n",
            index, MEMORY_GROUP_HEADER_HEAD(mem_group), MEMORY_GROUP_HEADER_NUM_FREE(mem_group),
            MEMORY_GROUP_HEADER_NUM_INIT(mem_group), MEMORY_GROUP_HEADER_HEAP_POS(mem_group));
#endif

    if (MEMORY_GROUP_HEADER_NUM_FREE(mem_group) > 1) {
        /* Group is already on the heap. Update its position. */
        cu_heap_update(&pool->free_memory, MEMORY_GROUP_HEADER_HEAP_POS(mem_group));
    }
    else {
        /* Otherwise put this group back on the heap. */
        cu_heap_insert(&pool->free_memory, mem_group);
    }
#if DEBUG
    uint32_t j;
    for (j = 0; j < pool->free_memory.length; ++j) {
        fprintf(stderr, "heap[%u]: %p (free: %u)\n", j, pool->free_memory.data[j],
                MEMORY_GROUP_HEADER_NUM_FREE(pool->free_memory.data[j]));
    }
#endif

    return true;
}

/* Determine whether the memory is managed by the pool. */
bool cu_fixed_size_memory_pool_is_managed(CUFixedSizeMemoryPool *pool, void *ptr)
{
    if (cu_unlikely(!pool))
        return false;
    return cu_btree_find(pool->managed_memory, ptr, NULL);
}
