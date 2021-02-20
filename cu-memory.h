/** @file cu-memory.h
 *  Handle memory.
 *  @defgroup CUMemory Memory handling.
 *  @{
 */
#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>

/** @brief Allocate memory.
 *  @details If no memory could be allocated, terminate the program. Thus, always returns a valid pointer.
 *  @param[in] size The amount of memory to allocate.
 *  @return Pointer to the newly allocated memory.
 */
void *cu_alloc(size_t size);

/** @brief Allocate memory and initialize it to zero.
 *  @param[in] size The amount of memory to allocate.
 *  @return Pointer to the newly allocated memory.
 */
void *cu_alloc0(size_t size);

/** @brief Return memory to the system.
 *  @param[in] ptr The memory to return to the system.
 */
void cu_free(void *ptr);

/** @brief Reallocate memory and keep the contents.
 *  @param[in] ptr Pointer to the memory area to resize.
 *  @param[in] size The new size of the memory area.
 *  @return Pointer to the resized memory area, which may have changed.
 */
void *cu_realloc(void *ptr, size_t size);

/** @brief Class to set other memory handling functions instead of the standard functions.
 *  @details These have the same signature as the standard glibc malloc/realloc/free functions,
 *  which are used by default. However, we provide this mechanism to allow other types of memory management.
 */
typedef struct {
    /** @brief Allocate a new area of memory.
     *  @param[in] 1 The size requested for the new memory area.
     *  @return A pointer to the newly allocated memory.
     */
    void *(*alloc)(size_t);

    /** @brief Resize a memory area.
     *  @param[in] 1 Pointer to the current memory area.
     *  @param[in] 2 The new size of the memory area.
     *  @return A pointer to the resized memory, which may have changed.
     */
    void *(*realloc)(void *, size_t);

    /** @brief Free memory.
     *  @param[in] 1 The memory to free.
     */
    void (*free)(void *);
} CUMemoryHandler;

/** @brief Set an alternative memory handler.
 *  @param[in] handler The alternative memory handler.
 */
void cu_set_memory_handler(CUMemoryHandler *handler);

/** @brief Allocate aligned memory.
 *  @param[out] ptr The newly allocated memory area.
 *  @param[in] size The requested size of the memory area.
 */
static __attribute__((always_inline)) inline
void cu_alloc_aligned(void **ptr, size_t size)
{
    if (__builtin_expect((posix_memalign(ptr, 16, size) != 0), 0))
        exit(1);
}

/** @brief Allocate aligned memory and init it with 0.
 *  @param[out] ptr The newly allocated memory area.
 *  @param[in] size The requested size of the memory area.
 */
static __attribute__((always_inline)) inline
void cu_alloc_aligned0(void **ptr, size_t size)
{
    cu_alloc_aligned(ptr, size);
    memset(*ptr, 0, size);
}

/** @brief A pool of memory containing for elements of the same size.
 *  @details Internally, the memory is arranged in groups. For each group, the
 *           alloc/free operations can be done in O(1) time. Finding the right group
 *           requires O(1) for alloc and O(log n) for free.
 */
typedef struct _CUFixedSizeMemoryPool CUFixedSizeMemoryPool;

/** @brief Create a new memory pool in which all elements have size element_size.
 *  @details The pool internally will be group by blocks of @a group_size elements.
 *  Set this to 0 to get a reasonable default size.
 *  @param[in] element_size The size of a single element.
 *  @param[in] group_size The number of elements in each memory group.
 *  @return A pointer to a new memory pool.
 */
CUFixedSizeMemoryPool *cu_fixed_size_memory_pool_new(size_t element_size, size_t group_size);

/** @brief Configure the memory pool to free groups that get empty instead of keeping them around.
 *  @param[in] pool The memory pool to configure.
 *  @param[in] do_release If @a true, free a memory group as soon as there are no allocated elements
 *                        in that group. Otherwise, keep the groups around.
 */
void cu_fixed_size_memory_pool_release_empty_groups(CUFixedSizeMemoryPool *pool, bool do_release);

/** @brief Clear all data from the pool.
 *  @param[in] pool The memory pool for which all data to clear.
 */
void cu_fixed_size_memory_pool_clear(CUFixedSizeMemoryPool *pool);

/** @brief Destroy the pool.
 *  @param[in] pool Destory the memory pool.
 */
void cu_fixed_size_memory_pool_destroy(CUFixedSizeMemoryPool *pool);

/** @brief Get a new element from the pool.
 *  @param[in] pool The pool handling the memory.
 *  @return Pointer to the newly allocated memory.
 */
void *cu_fixed_size_memory_pool_alloc(CUFixedSizeMemoryPool *pool);

/** @brief Return an element to the pool.
 *  @param[in] pool The pool handling the memory.
 *  @param[in] ptr The memory to return to the pool.
 *  @retval true If the memory could be returned.
 *  @retval false If the memory was not managed by the pool.
 */
bool cu_fixed_size_memory_pool_free(CUFixedSizeMemoryPool *pool, void *ptr);

/** @brief Determine whether the memory is managed by the pool.
 *  @param[in] pool The pool handling the memory.
 *  @param[in] ptr The memory for which to check whether it is managed.
 *  @retval true The memory is managed by the pool.
 *  @retval false The memory is not managed by the pool.
 */
bool cu_fixed_size_memory_pool_is_managed(CUFixedSizeMemoryPool *pool, void *ptr);

/** @} */
