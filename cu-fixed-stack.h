/** @file cu-fixed-stack.h
 *  Provide a stack of elements of a fixed size with a predefined maximal number of elements.
 */
#pragma once

#include <stdlib.h>
#include <stdint.h>

/** @brief Callback function to clear the data of an element.
 *  @param[in] 1 Pointer to the element to clear.
 */
typedef void (*CUFixedStackClearElementFunc)(void *);

/** @brief Callback to copy an element to another one.
 *  @param[in] 1 The destination element.
 *  @param[in] 2 The source element.
 */
typedef void (*CUFixedStackElementCopyFunc)(void *, void *);

/** @brief Setup an element, i.e., give pointer to extra data in second argument.
 *  @details The additional data is layout directly after the element itself, so no
 *           extra allocation is necessary.
 *  @param[in] 1 Pointer to the element to set up.
 *  @param[in] 2 Pointer to the additional data.
 *  @param[in] 3 Size of the additional data.
 */
typedef void (*CUFixedStackElementSetupProc)(void *, void *, size_t);

/** @brief Class of the stack to hold all required information to handle the stack.
 */
typedef struct {
    size_t element_size; /**< Size of a single element. */
    size_t extra_data_size; /**< Size of additional data for the element. */
    uint8_t align; /**< Whether to align the elements in memory. */
    CUFixedStackClearElementFunc clear_func; /**< Function to clear the data of a single element. */
    CUFixedStackElementSetupProc setup_proc; /**< Function to setup a new element on the stack. */
} CUFixedStackClass;

/** @brief Stack to handle a fixed size stack.
 */
typedef struct {
    CUFixedStackClass cls; /**< The main configuration for the stack. */
    size_t size; /**< The total allocated size of the stack. */
    size_t total_element_size; /**< Total element size, including extra data and padding. */
    size_t length; /**< Total number of valid elements currently on the stack. */
    uint8_t *data; /**< The actual data of the stack. */
} CUFixedStack;

/** @brief Create a new fixed size stack.
 *  @param[in] cls Pointer to the configuration of the stack.
 *  @param[in] max_length Maximum capacity of the stack.
 *  @return Pointer to a newly allocated and initialized stack.
 */
CUFixedStack *cu_fixed_stack_new(CUFixedStackClass *cls, size_t max_length);

/** @brief Initialize a fixed stack.
 *  @details All future elements are set up by calling the setup_proc, if given.
 *  @param[in,out] stack Pointer to a stack that is to be initialized.
 *  @param[in] cls Pointer to the configuration of the stack.
 *  @param[in] max_length Maximum capacity of the stack.
 */
void cu_fixed_stack_init(CUFixedStack *stack, CUFixedStackClass *cls, size_t max_length);

/** @brief Clear all internal resources of a stack and set back to an uninitialized, but sane state.
 *  @param[in] stack Pointer to a stack that is to be cleared.
 */
void cu_fixed_stack_clear(CUFixedStack *stack);

/** @brief Clear all elements and set back to an initialized state.
 *  @param[in] stack Pointer to a stack that is to be reset.
 */
void cu_fixed_stack_reset(CUFixedStack *stack);

/** @brief Free all resources of a stack.
 *  @param[in] stack Pointer to a stack to free.
 */
void cu_fixed_stack_free(CUFixedStack *stack);

/** @brief Return a pointer to the element on top of the stack without removing it.
 *  @param[in] stack Pointer to the stack from which to peek.
 *  @return Pointer to the top element or @a NULL if the stack is empty.
 */
void *cu_fixed_stack_peek(CUFixedStack *stack);

/** @brief Return a pointer to the element that is pushed next to the stack.
 *  @details Since all elements in the stack are in a single memory block, you cannot push
 *           arbitrary data to the stack. Instead use this function instead of an alloc.
 *           All elements are set up during stack initialization, so no extra action is
 *           necessary. In particular, no @a memset() should be used to clear the current data.
 *  @param[in] stack Pointer to the stack for which to return the next element.
 *  @return Pointer to the next available element on the stack, or @a NULL if the stack is full.
 */
void *cu_fixed_stack_fetch_next(CUFixedStack *stack);

/** @brief Return a pointer to the element on top of the stack and remove it.
 *  @details Be aware that due to the internal data representation, the returned element becomes
 *           Invalid on the next call to cu_fixed_stack_fetch_next().
 *  @param[in] stack Pointer to the stack from which to pop.
 *  @return Pointer to the element prior on top, or @a NULL if the stack was empty.
 */
void *cu_fixed_stack_pop(CUFixedStack *stack);

/** @brief Advance the next pointer of the stack.
 *  @details This is basically putting the element returned by cu_fixed_stack_fetch_next() on top of the stack.
 *  @param[in] stack Pointer to the stack to push to.
 */
void cu_fixed_stack_push(CUFixedStack *stack);

/** @brief Return a pointer to the last element on the stack.
 *  @details This is in fact just like cu_fixed_stack_peek().
 *  @param[in] stack Pointer to the stack to get the tail from.
 *  @return Pointer to the last element on the stack or @a NULL if the stack is empty.
 */
void *cu_fixed_stack_get_tail(CUFixedStack *stack);

/** @brief Return a pointer to the first element on the stack.
 *  @param[in] stack Pointer to the stack to get the head from.
 *  @return Pointer to the first element on the stack or @a NULL if the stack is empty.
 */
void *cu_fixed_stack_get_head(CUFixedStack *stack);

/** @brief Return a pointer to the element before a given element on the stack.
 *  @param[in] stack Pointer to the stack to get the element from.
 *  @param[in] current Pointer to the element for which to determine the predecessor.
 *  @return Pointer to the predecessor of @a current, or @a NULL if the element was the head.
 */
void *cu_fixed_stack_previous(CUFixedStack *stack, void *current);

/** @brief Return a pointer to the element after a given element on the stack.
 *  @param[in] stack Pointer to the stack to get the element from.
 *  @param[in] current Pointer to the element for which to determine the successor.
 *  @return Pointer to the successor of @a current, or @a NULL if the element was the topmost element.
 */
void *cu_fixed_stack_next(CUFixedStack *stack, void *current);

/** @defgroup FixedPointerStack Convenience wrappers for just keeping pointers on the stack.
 *  @{
 */

/** @brief Create a new fixed pointer stack.
 *  @details Creates a new fixed size stack, only handling pointers, with no special initialization.
 *  @param[in] max_length The maximal capacity of the stack.
 *  @return Pointer to the newly allocated stack.
 */
CUFixedStack *cu_fixed_pointer_stack_new(size_t max_length);

/** @brief Initialize a pointer stack.
 *  @param[in,out] stack Pointer to the stack to initialize.
 *  @param[in] max_length The maximal capacity of the stack.
 */
void cu_fixed_pointer_stack_init(CUFixedStack *stack, size_t max_length);

/** @brief Push a pointer to the stack.
 *  @param[in] stack Pointer to the stack to push to.
 *  @param[in] data The pointer value to push to the top of the stack.
 */
void cu_fixed_pointer_stack_push(CUFixedStack *stack, void *data);

/** @brief Return the pointer on top of the stack and remove it.
 *  @param[in] stack Pointer to the stack to pop from.
 *  @return The value of the pointer on top of the stack.
 */
void *cu_fixed_pointer_stack_pop(CUFixedStack *stack);

/** @brief Return the pointer on top of the stack without removing it.
 *  @param[in] stack Pointer to the stack to peek from.
 *  @return The value of the pointer on top of the stack.
 */
void *cu_fixed_pointer_stack_peek(CUFixedStack *stack);

/** @} */
