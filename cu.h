#pragma once

#include <stdint.h>
#include <cu-memory.h>
#include <cu-list.h>
#include <cu-queue.h>
#include <cu-stack.h>

#if __WORDSIZE == 32
#define CU_POINTER_TO_UINT(p) ((uint32_t)(p))
#define CU_UINT_TO_POINTER(u) ((void *)(uint32_t)(u))
#else
#define CU_POINTER_TO_UINT(p) ((uint32_t)(uint64_t)(p))
#define CU_UINT_TO_POINTER(u) ((void *)(uint64_t)(uint32_t)(u))
#endif

#define ROUND_TO_4(n)  ((((n) + 3)  >> 2) << 2)
#define ROUND_TO_8(n)  ((((n) + 7)  >> 3) << 3)
#define ROUND_TO_16(n) ((((n) + 15) >> 4) << 4)

#define cu_likely(x)   __builtin_expect((x), 1)
#define cu_unlikely(x) __builtin_expect((x), 0)
