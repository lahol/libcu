#include <cu-queue-locked.h>

#define QUEUE_LOCKED 1
#include "cu-queue.c"
#undef QUEUE_LOCKED
