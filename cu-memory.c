#include "cu-memory.h"
#include <stdlib.h>
#include <memory.h>

#include <stdint.h>
#include "cu.h"
#include "cu-list.h"

#ifdef DEBUG
#include <stdio.h>
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
/* Size of the header in each group, (head id, number initialized, number free, reserved; each 4 bytes) */
#define MEMORY_GROUP_HEADER_SIZE       16

#define MEMORY_GROUP_HEADER_HEAD(group) (*((uint32_t *)((group))))
#define MEMORY_GROUP_HEADER_NUM_INIT(group) (*((uint32_t *)((group) + 4)))
#define MEMORY_GROUP_HEADER_NUM_FREE(group) (*((uint32_t *)((group) + 8)))

#define MEMORY_GROUP_ELEMENT(group, block, element_size) (*((uint32_t *)((group) +\
                MEMORY_GROUP_HEADER_SIZE + (block) * (element_size))))
#define MEMORY_GROUP_ELEMENT_PTR(group, block, element_size) ((void *)((group) +\
                MEMORY_GROUP_HEADER_SIZE + (block) * (element_size)))

struct _CUFixedSizeMemoryPool {
    uint32_t group_size;    /* number of elements in each group. */
    uint32_t element_size;  /* size of each element. */
    size_t alloc_size;      /* size to allocate per group. */

    size_t total_free;      /* number of free elements in the whole pool. */

    /* FIXME: use a balanced tree here. */
    CUList *memory_groups;
};

void *_cu_fixed_size_memory_pool_group_new(CUFixedSizeMemoryPool *pool)
{
    void *group = cu_alloc(pool->alloc_size);
    MEMORY_GROUP_HEADER_HEAD(group) = 0;
    MEMORY_GROUP_HEADER_NUM_INIT(group) = 0;
    MEMORY_GROUP_HEADER_NUM_FREE(group) = pool->group_size;

    pool->total_free += pool->group_size;

    pool->memory_groups = cu_list_prepend(pool->memory_groups, group);

#ifdef DEBUG
    fprintf(stderr, "new memory group %p, head: %u, free: %u, init: %u\n", group,
            MEMORY_GROUP_HEADER_HEAD(group), MEMORY_GROUP_HEADER_NUM_FREE(group), MEMORY_GROUP_HEADER_NUM_INIT(group));
#endif

    return group;
}

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
    if (group_size == 0)
        pool->group_size = (4096 - MEMORY_GROUP_HEADER_SIZE) / element_size;
    else
        pool->group_size = group_size;

    pool->alloc_size = pool->group_size * pool->element_size + MEMORY_GROUP_HEADER_SIZE;

#ifdef DEBUG
    fprintf(stderr, "new pool, element_size: %u, group_size: %u, alloc_size: %zu\n",
            pool->element_size, pool->group_size, pool->alloc_size);
#endif

    _cu_fixed_size_memory_pool_group_new(pool);

    return pool;
}

/* Clear all data from the pool. */
void cu_fixed_size_memory_pool_clear(CUFixedSizeMemoryPool *pool)
{
    if (pool) {
        cu_list_free_full(pool->memory_groups, (CUDestroyNotifyFunc)cu_free);
        pool->memory_groups = NULL;
        pool->total_free = 0;
    }
}

/* Destroy the pool. */
void cu_fixed_size_memory_pool_destroy(CUFixedSizeMemoryPool *pool)
{
    if (pool) {
        cu_list_free_full(pool->memory_groups, (CUDestroyNotifyFunc)cu_free);
        cu_free(pool);
    }
}

/* Get a new element from the pool. */
void *cu_fixed_size_memory_pool_alloc(CUFixedSizeMemoryPool *pool)
{
    void *mem_group = NULL;
    /* Get the first memory group with unused elements. */
    if (cu_unlikely(!pool->total_free)) {
        mem_group = _cu_fixed_size_memory_pool_group_new(pool);
    }
    else {
        CUList *tmp;
        for (tmp = pool->memory_groups; tmp; tmp = tmp->next) {
            if (MEMORY_GROUP_HEADER_NUM_FREE(tmp->data)) {
                mem_group = tmp->data;
                break;
            }
        }
    }

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
    }
    else {
        /* There are no unused elements in this group. Set to invalid. */
        MEMORY_GROUP_HEADER_HEAD(mem_group) = 0xffffffff;
    }

#ifdef DEBUG
    fprintf(stderr, "allocated %p from group %p, new head: %u\n",
            ret, mem_group, MEMORY_GROUP_HEADER_HEAD(mem_group));
#endif

    return ret;
}

/* Return an element to the pool. */
void cu_fixed_size_memory_pool_free(CUFixedSizeMemoryPool *pool, void *ptr)
{
    CUList *tmp;
    void *mem_group = NULL;
    for (tmp = pool->memory_groups; tmp; tmp = tmp->next) {
        if (ptr > tmp->data && ptr <= tmp->data + pool->alloc_size) {
            mem_group = tmp->data;
            break;
        }
    }

#ifdef DEBUG
    fprintf(stderr, "ptr %p in group %p\n", ptr, mem_group);
#endif

    if (!mem_group)
        return;

    uint32_t index = (ptr - mem_group - MEMORY_GROUP_HEADER_SIZE) / pool->element_size;

    MEMORY_GROUP_ELEMENT(mem_group, index, pool->element_size) = MEMORY_GROUP_HEADER_HEAD(mem_group);
    MEMORY_GROUP_HEADER_HEAD(mem_group) = index;
    ++MEMORY_GROUP_HEADER_NUM_FREE(mem_group);
    ++pool->total_free;
#ifdef DEBUG
    fprintf(stderr, "free, index: %u, new head: %u, free: %u, init: %u\n",
            index, MEMORY_GROUP_HEADER_HEAD(mem_group), MEMORY_GROUP_HEADER_NUM_FREE(mem_group),
            MEMORY_GROUP_HEADER_NUM_INIT(mem_group));
#endif
}
