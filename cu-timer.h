#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct _CUTimer CUTimer;

typedef bool (*CUTimerCallback)(void *);

CUTimer *cu_timer_start(uint32_t interval, CUTimerCallback cb, void *userdata);
void cu_timer_reset(CUTimer *timer);
void cu_timer_stop(CUTimer *timer);
void cu_timer_ref(CUTimer *timer);
void cu_timer_unref(CUTimer *timer);
