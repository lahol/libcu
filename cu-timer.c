#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <cu.h>
#include <stdatomic.h>

struct _CUTimer {
    CUTimerCallback callback;
    void *callback_data;
    timer_t timerid;

    atomic_int reference_count;
};

void cu_timer_ref(CUTimer *timer)
{
    if (timer)
        atomic_fetch_add(&timer->reference_count, 1);
}

void cu_timer_unref(CUTimer *timer)
{
    if (timer && atomic_fetch_sub(&timer->reference_count, 1) == 1)
        cu_free(timer);
}

void _cu_timer_handle(CUTimer *timer)
{
    if (cu_unlikely(timer == NULL))
        return;
    if (cu_unlikely(timer->callback == NULL))
        return;

    bool result = timer->callback(timer->callback_data);
    if (!result)
        cu_timer_stop(timer);
}

CUTimer *cu_timer_start(uint32_t interval, CUTimerCallback cb, void *userdata)
{
    CUTimer *timer = cu_alloc0(sizeof(CUTimer));
    timer->callback = cb;
    timer->callback_data = userdata;
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

    struct itimerspec its;
    its.it_value.tv_sec = interval;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    if (timer_settime(timer->timerid, 0, &its, NULL) == -1) {
        timer_delete(timer->timerid);
        cu_timer_unref(timer);
        return NULL;
    }

    return timer;
}

void cu_timer_stop(CUTimer *timer)
{
    if (cu_unlikely(timer == NULL))
        return;
    /* Disarm timer */
    struct itimerspec its;
    memset(&its, 0, sizeof(struct itimerspec));

    timer_settime(timer->timerid, 0, &its, NULL);
    timer_delete(timer->timerid);

    cu_timer_unref(timer);
}
