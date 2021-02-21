/** @file cu.h
 *  Main file including every module and providing some useful macros.
 *  @defgroup libcu Common macros.
 *  @{
 */
#pragma once

#include <stdint.h>
#include <cu-memory.h>
#include <cu-list.h>
#include <cu-queue.h>
#include <cu-queue-fixed-size.h>
#include <cu-queue-locked.h>
#include <cu-stack.h>
#include <cu-timer.h>
#include <cu-avl-tree.h>
#include <cu-fixed-stack.h>
#include <cu-heap.h>
#include <cu-mixed-heap-list.h>
#include <cu-types.h>

#if __WORDSIZE == 32
/** @brief Cast a pointer to an uint32_t. */
#define CU_POINTER_TO_UINT(p) ((uint32_t)(p))
/** @brief Cast an uint32_t to a pointer. */
#define CU_UINT_TO_POINTER(u) ((void *)(uint32_t)(u))
#else
/** @brief Cast a pointer to an uint32_t. */
#define CU_POINTER_TO_UINT(p) ((uint32_t)(uint64_t)(p))
/** @brief Cast an uint32_t to a pointer. */
#define CU_UINT_TO_POINTER(u) ((void *)(uint64_t)(uint32_t)(u))
#endif

/** @brief Round a number up to a multiple of 4. */
#define ROUND_TO_4(n)  ((((n) + 3)  >> 2) << 2)

/** @brief Round a number up to a multiple of 8. */
#define ROUND_TO_8(n)  ((((n) + 7)  >> 3) << 3)

/** @brief Round a number up to a multiple of 16. */
#define ROUND_TO_16(n) ((((n) + 15) >> 4) << 4)

/** @brief Compiler hint that a condition is expected to be true. */
#define cu_likely(x)   __builtin_expect((x), 1)

/** @brief Compiler hint that a condition is expected to be false. */
#define cu_unlikely(x) __builtin_expect((x), 0)

/** @} */
