#pragma once

#include <stdlib.h>
#include <stdint.h>

typedef void (*CUFixedStackClearElementFunc)(void *);
typedef void (*CUFixedStackElementCopyFunc)(void *, void *);
/* setup an element, i.e., give pointer to extra data in second argument */
typedef void (*CUFixedStackElementSetupProc)(void *, void *, size_t);

typedef struct {
    size_t element_size;
    size_t extra_data_size;    /* extra data */
    uint8_t align;            /* just a flag */
    CUFixedStackClearElementFunc clear_func;
    CUFixedStackElementSetupProc setup_proc;
} CUFixedStackClass;

typedef struct {
    CUFixedStackClass cls;
    size_t size;
    size_t total_element_size;
    size_t length;
    uint8_t *data;
} CUFixedStack;

CUFixedStack *cu_fixed_stack_new(CUFixedStackClass *cls, size_t max_length);
void cu_fixed_stack_init(CUFixedStack *stack, CUFixedStackClass *cls, size_t max_length);
void cu_fixed_stack_clear(CUFixedStack *stack);
void cu_fixed_stack_reset(CUFixedStack *stack);
void cu_fixed_stack_free(CUFixedStack *stack);
void *cu_fixed_stack_peek(CUFixedStack *stack);
/* get the next element from stack */
void *cu_fixed_stack_fetch_next(CUFixedStack *stack);
void *cu_fixed_stack_pop(CUFixedStack *stack);
/* increment pointer to last fetched element */
void cu_fixed_stack_push(CUFixedStack *stack);

void *cu_fixed_stack_get_tail(CUFixedStack *stack);
void *cu_fixed_stack_get_head(CUFixedStack *stack);
void *cu_fixed_stack_previous(CUFixedStack *stack, void *current);
void *cu_fixed_stack_next(CUFixedStack *stack, void *current);

/* Convenience wrappers for just keeping pointers on the stack. */
CUFixedStack *cu_fixed_pointer_stack_new(size_t max_length);
void cu_fixed_pointer_stack_init(CUFixedStack *stack, size_t max_length);
void cu_fixed_pointer_stack_push(CUFixedStack *stack, void *data);
void *cu_fixed_pointer_stack_pop(CUFixedStack *stack);
