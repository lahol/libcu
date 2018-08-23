#pragma once

#include <stdint.h>
#include <cu-memory.h>
#include <cu-list.h>
#include <cu-queue.h>

#ifdef __x86_32__
#define CU_POINTER_TO_UINT(p) ((uint32_t)(p))
#define CU_UINT_TO_POINTER(u) ((void *)(uint32_t)(u))
#else
#define CU_POINTER_TO_UINT(p) ((uint32_t)(uint64_t)(p))
#define CU_UINT_TO_POINTER(u) ((void *)(uint64_t)(uint32_t)(u))
#endif

/* FIXME: Does this work if we are already a multiple of 16? */
#define ROUND_TO_16(n) ((((n) >> 4) + 1) << 4)

#define cu_likely(x)   __builtin_expect((x), 1)
#define cu_unlikely(x) __builtin_expect((x), 0)
