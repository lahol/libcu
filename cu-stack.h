/** @file cu-stack.h
 *  A generic stack.
 *  @defgroup CUStack Generic, simple stack.
 *  @{
 */
#pragma once

#include <cu-list.h>
#include <stdlib.h>

/** @brief A generic stack, holding pointers to its elements.
 */
typedef struct {
    CUList *head; /**< Pointer to the top link of the stack. */

    size_t length; /**< Total number of elements currently in the stack. */
} CUStack;

/** @brief Initialize a stack.
 *  @param[in] stack Pointer to the stack to initialize.
 */
void cu_stack_init(CUStack *stack);

/** @brief Clear a stack.
 *  @param[in] stack Pointer to the stack to clear.
 *  @param[in] notify Function to call to free the resources used by each element.
 */
void cu_stack_clear(CUStack *stack, CUDestroyNotifyFunc notify);

/** @brief Push an element to a stack.
 *  @param[in] stack Pointer to the stack to push to.
 *  @param[in] data Pointer to the data to push to the stack.
 */
void cu_stack_push(CUStack *stack, void *data);

/** @brief Return a pointer to the topmost element on a stack and remove it.
 *  @param[in] stack Pointer to the stack to pop from.
 *  @return Pointer to the element formerly on top of the stack, or @a NULL if the stack was empty.
 */
void *cu_stack_pop(CUStack *stack);

/** @brief Return a pointer to the topmost element on a stack without removing it
 *  @param[in] stack Pointer to the stack to peek from.
 *  @return Pointer to the element on top of the stack, or @a NULL if the stack was empty.
 */
void *cu_stack_peek(CUStack *stack);

/** @} */
