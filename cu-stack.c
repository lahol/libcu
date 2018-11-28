#include "cu-stack.h"
#include <memory.h>

/* Initialize the stack. */
void cu_stack_init(CUStack *stack)
{
    memset(stack, 0, sizeof(CUStack));
}

/* Clear the stack. */
void cu_stack_clear(CUStack *stack, CUDestroyNotifyFunc notify)
{
    cu_list_free_full(stack->head, notify);
    memset(stack, 0, sizeof(CUStack));
}

/* Push to stack. */
void cu_stack_push(CUStack *stack, void *data)
{
    stack->head = cu_list_prepend(stack->head, data);
    ++stack->length;
}

/* Pop from stack. */
void *cu_stack_pop(CUStack *stack)
{
    if (!stack->length)
        return NULL;
    void *data = stack->head->data;
    stack->head = cu_list_delete_link(stack->head, stack->head);
    --stack->length;

    return data;
}

/* Peek stack */
void *cu_stack_peek(CUStack *stack)
{
    return (stack->head ? stack->head->data : NULL);
}
