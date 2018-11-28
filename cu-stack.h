/* Generic stack. */
#pragma once

#include <cu-list.h>
#include <stdlib.h>

typedef struct {
    CUList *head;

    size_t length;
} CUStack;

/* Initialize the stack. */
void cu_stack_init(CUStack *stack);

/* Clear the stack. */
void cu_stack_clear(CUStack *stack, CUDestroyNotifyFunc notify);

/* Push to stack. */
void cu_stack_push(CUStack *stack, void *data);

/* Pop from stack. */
void *cu_stack_pop(CUStack *stack);

/* Peek stack */
void *cu_stack_peek(CUStack *stack);
