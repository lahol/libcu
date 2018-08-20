#pragma once

#include <stdint.h>
#include <cu-memory.h>
#include <cu-list.h>

#ifdef __x86_32__
#define CU_POINTER_TO_UINT(p) ((uint32_t)(p))
#define CU_UINT_TO_POINTER(u) ((void *)(uint32_t)(u))
#else
#define CU_POINTER_TO_UINT(p) ((uint32_t)(uint64_t)(p))
#define CU_UINT_TO_POINTER(u) ((void *)(uint64_t)(uint32_t)(u))
#endif

#define cu_likely(x)   __builtin_expect((x), 1)
#define cu_unlikely(x) __builtin_expect((x), 0)
