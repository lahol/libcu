#include "cu-fixed-stack.h"
#include "cu-memory.h"
#include "cu.h"

CUFixedStack *cu_fixed_stack_new(CUFixedStackClass *cls, size_t max_length)
{
    CUFixedStack *stack = cu_alloc0(sizeof(CUFixedStack));
    cu_fixed_stack_init(stack, cls, max_length);
    return stack;
}

void cu_fixed_stack_init(CUFixedStack *stack, CUFixedStackClass *cls, size_t max_length)
{
    stack->cls = *cls;
    if (cls->align) {
        /* round up to multiples of 16 bytes */
        if (stack->cls.element_size & 0xf)
            stack->cls.element_size = ROUND_TO_16(stack->cls.element_size);
        stack->total_element_size = stack->cls.element_size + cls->extra_data_size;
        if (stack->total_element_size & 0xf)
            stack->total_element_size = ROUND_TO_16(stack->total_element_size);
    }
    else {
        stack->total_element_size = cls->element_size + cls->extra_data_size;
    }
    stack->size = max_length * stack->total_element_size;
    cu_alloc_aligned0((void **)&stack->data, stack->size);
    stack->length = 0;
    if (cls->setup_proc) {
        void *element_ptr;
        for (element_ptr = stack->data;
             element_ptr < (void *)(stack->data + stack->size);
             element_ptr += stack->total_element_size) {
            cls->setup_proc(element_ptr, element_ptr + stack->cls.element_size, cls->extra_data_size);
        }
    }
}

void cu_fixed_stack_clear(CUFixedStack *stack)
{
    size_t offset;
    if (stack) {
        if (stack->cls.clear_func) {
            for (offset = 0; offset < stack->size; offset += stack->total_element_size) {
                stack->cls.clear_func(&stack->data[offset]);
            }
        }
        free(stack->data);
        stack->data = NULL;
        stack->size = 0;
        stack->length = 0;
    }
}

/* Same as clear, but do not free data */
void cu_fixed_stack_reset(CUFixedStack *stack)
{
    if (stack) {
        if (stack->cls.clear_func) {
            size_t offset;
            for (offset = 0; offset < stack->size; offset += stack->total_element_size) {
                stack->cls.clear_func(&stack->data[offset]);
            }
        }
        if (stack->cls.setup_proc) {
            void *element_ptr;
            for (element_ptr = stack->data;
                 element_ptr < (void *)(stack->data + stack->size);
                 element_ptr += stack->total_element_size) {
                stack->cls.setup_proc(element_ptr, element_ptr + stack->cls.element_size, stack->cls.extra_data_size);
            }
        }
        stack->length = 0;
    }
}

void cu_fixed_stack_free(CUFixedStack *stack)
{
    cu_fixed_stack_clear(stack);
    cu_free(stack);
}

void *cu_fixed_stack_peek(CUFixedStack *stack)
{
    if (stack->length == 0)
        return NULL;
    size_t offset = (stack->length - 1) * stack->total_element_size;
    if (offset >= stack->size)
        return NULL;
    return (void *)(stack->data + offset);
}

/* get the next element from stack */
void *cu_fixed_stack_fetch_next(CUFixedStack *stack)
{
    size_t offset = stack->length * stack->total_element_size;
    if (offset >= stack->size)
        return NULL;
    return (void *)(stack->data + offset);
}

void *cu_fixed_stack_pop(CUFixedStack *stack)
{
    if (stack->length == 0)
        return NULL;
    size_t offset = (--stack->length) * stack->total_element_size;
    return (void *)(stack->data + offset);
}

/* increment pointer to last fetched element */
void cu_fixed_stack_push(CUFixedStack *stack)
{
    ++stack->length;
}

void *cu_fixed_stack_get_tail(CUFixedStack *stack)
{
    if (stack->length == 0)
        return NULL;
    size_t offset = (stack->length - 1) * stack->total_element_size;
    return (void *)(stack->data + offset);
}

void *cu_fixed_stack_get_head(CUFixedStack *stack)
{
    if (stack->length == 0)
        return NULL;
    return stack->data;
}

void *cu_fixed_stack_previous(CUFixedStack *stack, void *current)
{
    if (stack->data == current)
        return NULL;
    return (void *)(current - stack->total_element_size);
}

void *cu_fixed_stack_next(CUFixedStack *stack, void *current)
{
    if (current >= (void *)(stack->data + (stack->length - 1) * stack->total_element_size))
        return NULL;
    return (void *)(current + stack->total_element_size);
}
