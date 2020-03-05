#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <cu.h>
#include <stdatomic.h>
#include <pthread.h>

#if DEBUG
#include <stdio.h>
#endif

#ifndef CFG_CU_MAX_TIMERS
#define CFG_CU_MAX_TIMERS           32
#endif

struct _CUTimer {
    CUTimerCallback callback;
    void *callback_data;

    uint32_t interval;
    timer_t timerid;

    atomic_int reference_count;

    uint32_t in_use : 1; /* This object is in use for a timer object. */
    uint32_t armed : 1;  /* The timer is armed. */
};

CUTimer _cu_timers[CFG_CU_MAX_TIMERS];
uint32_t _cu_timers_count = 0;
pthread_mutex_t _cu_timers_lock = PTHREAD_MUTEX_INITIALIZER;

CUTimer *_cu_timer_acquire(void)
{
    CUTimer *result = NULL;
    pthread_mutex_lock(&_cu_timers_lock);
    uint32_t j;
    for (j = 0; j < CFG_CU_MAX_TIMERS; ++j) {
        if (!_cu_timers[j].in_use) {
            _cu_timers[j].in_use = 1;
            ++_cu_timers_count;
            result = &_cu_timers[j];
            break;
        }
    }
    pthread_mutex_unlock(&_cu_timers_lock);
    return result;
}

void _cu_timer_release(CUTimer *timer)
{
    pthread_mutex_lock(&_cu_timers_lock);
    memset(timer, 0, sizeof(CUTimer));
    --_cu_timers_count;
    pthread_mutex_unlock(&_cu_timers_lock);
}

void cu_timer_ref(CUTimer *timer)
{
    if (timer)
        atomic_fetch_add(&timer->reference_count, 1);
}

void cu_timer_unref(CUTimer *timer)
{
    if (timer && atomic_fetch_sub(&timer->reference_count, 1) == 1) {
#if DEBUG
        fprintf(stderr, "FREE timer %p\n", timer);
#endif
        _cu_timer_release(timer);
    }
}

void _cu_timer_handle(CUTimer *timer)
{
    if (cu_unlikely(timer == NULL))
        return;
    if (cu_unlikely(timer->callback == NULL))
        return;
    if (cu_unlikely(!timer->armed))
        return;

    cu_timer_ref(timer);

#if DEBUG
    fprintf(stderr, "HANDLE timer %p\n", timer);
#endif
    bool result = timer->callback(timer->callback_data);
    if (!result) {
#if DEBUG
        fprintf(stderr, "STOP due to callback result, timer %p\n", timer);
#endif
        cu_timer_stop(timer);
    }

    cu_timer_unref(timer);
}

int _cu_timer_reset_expire(CUTimer *timer)
{
#if DEBUG
    fprintf(stderr, "RESET EXPIRE timer %p\n", timer);
#endif
    struct itimerspec its;
    its.it_value.tv_sec = timer->interval;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    return timer_settime(timer->timerid, 0, &its, NULL);
}

CUTimer *cu_timer_start(uint32_t interval, CUTimerCallback cb, void *userdata)
{
    CUTimer *timer = _cu_timer_acquire();
    if (!timer)
        return NULL;
#if DEBUG
    fprintf(stderr, "START timer %p, interval %u\n", timer, interval);
#endif
    timer->callback = cb;
    timer->callback_data = userdata;
    timer->interval = interval;
    atomic_store(&timer->reference_count, 1);

    struct sigevent se;

    /* man 7 sigevent */
    memset(&se, 0, sizeof(struct sigevent));
    se.sigev_notify = SIGEV_THREAD;
    se.sigev_value.sival_ptr = timer;
    se.sigev_notify_function = (void (*)(union sigval))_cu_timer_handle;

    /* man 2 timer_create */
    if (timer_create(CLOCK_MONOTONIC, &se, &timer->timerid) == -1) {
        cu_timer_unref(timer);
        return NULL;
    }

    if (_cu_timer_reset_expire(timer) == -1) {
        timer_delete(timer->timerid);
        cu_timer_unref(timer);
        return NULL;
    }

    timer->armed = 1;

    return timer;
}

void cu_timer_reset(CUTimer *timer)
{
    if (cu_unlikely(timer == NULL))
        return;
    _cu_timer_reset_expire(timer);
}

void cu_timer_stop(CUTimer *timer)
{
    if (cu_unlikely(timer == NULL || !timer->armed))
        return;
#if DEBUG
    fprintf(stderr, "STOP timer %p\n", timer);
#endif
    /* Disarm timer */
    timer->armed = 0;

    struct itimerspec its;
    memset(&its, 0, sizeof(struct itimerspec));

    timer_settime(timer->timerid, 0, &its, NULL);
    timer_delete(timer->timerid);

    cu_timer_unref(timer);
}
