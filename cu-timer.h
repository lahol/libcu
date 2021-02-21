/** @file cu-timer.h
 *  Providing simple timers with a resolution in seconds.
 *  At most @a CFG_CU_MAX_TIMERS are supported, which defaults to 32.
 *  @defgroup CUTimer Simple timers.
 *  @{
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

/** @brief Opaque structure representing a single timer.
 */
typedef struct _CUTimer CUTimer;

/** @brief Function to call each time the timer fires.
 *  @param[in] 1 User defined data provided at startup.
 *  @retval true Continue running the timer.
 *  @retval false Stop execution of the timer.
 */
typedef bool (*CUTimerCallback)(void *);

/** @brief Initialize and run a timer.
 *  @param[in] interval The number of seconds between two consecutive runs.
 *  @param[in] cb Function to call at each invocation.
 *  @param[in] userdata User defined data to pass to the callback.
 *  @return A handle to the newly created timer interval.
 */
CUTimer *cu_timer_start(uint32_t interval, CUTimerCallback cb, void *userdata);

/** @brief Reset the elapsed time of a timer.
 *  @param[in] timer The handle of the timer.
 */
void cu_timer_reset(CUTimer *timer);

/** @brief Stop a timer.
 *  @details The timer gets unarmed and the reference is dropped.
 *  @param[in] timer The handle of the timer.
 */
void cu_timer_stop(CUTimer *timer);

/** @brief Increase the reference count of a timer.
 *  @param[in] timer The handle of the timer.
 */
void cu_timer_ref(CUTimer *timer);

/** @brief Decrease the reference count of a timer.
 *  @details If the reference count drops to zero, free all resources held by the timer.
 *  @param[in] timer The handle of the timer.
 */
void cu_timer_unref(CUTimer *timer);

/** @} */
