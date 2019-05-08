#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef void (*CUDestroyNotifyFunc)(void *);

/* a, b, data,
 * return < 0 if a > b, > 0 if a < b */
typedef int (*CUCompareDataFunc)(void *, void *, void *);

/* key, value, data */
typedef bool (*CUTraverseFunc)(void *, void *, void *);
