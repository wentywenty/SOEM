#include "osal.h"
#include <zephyr/kernel.h>
#include <stdlib.h>

/* --- 时间相关 --- */

void osal_get_monotonic_time(ec_timet *ts)
{
    int64_t ms = k_uptime_get();
    ts->tv_sec = ms / 1000;
    ts->tv_nsec = (ms % 1000) * 1000000;
}

ec_timet osal_current_time(void)
{
    ec_timet ts;
    osal_get_monotonic_time(&ts);
    return ts;
}

void osal_time_diff(ec_timet *start, ec_timet *end, ec_timet *diff)
{
    if ((end->tv_nsec - start->tv_nsec) < 0) {
        diff->tv_sec = end->tv_sec - start->tv_sec - 1;
        diff->tv_nsec = end->tv_nsec - start->tv_nsec + 1000000000;
    } else {
        diff->tv_sec = end->tv_sec - start->tv_sec;
        diff->tv_nsec = end->tv_nsec - start->tv_nsec;
    }
}

void osal_timer_start(osal_timert *self, uint32 timeout_usec)
{
    struct timespec start_time;
    struct timespec timeout;

    osal_get_monotonic_time(&start_time);
    timeout.tv_sec = timeout_usec / 1000000;
    timeout.tv_nsec = (timeout_usec % 1000000) * 1000;

    self->stop_time.tv_sec = start_time.tv_sec + timeout.tv_sec;
    self->stop_time.tv_nsec = start_time.tv_nsec + timeout.tv_nsec;

    if (self->stop_time.tv_nsec >= 1000000000) {
        self->stop_time.tv_sec++;
        self->stop_time.tv_nsec -= 1000000000;
    }
}

boolean osal_timer_is_expired(osal_timert *self)
{
    struct timespec current_time;
    osal_get_monotonic_time(&current_time);

    if (current_time.tv_sec > self->stop_time.tv_sec) return TRUE;
    if (current_time.tv_sec < self->stop_time.tv_sec) return FALSE;

    return (current_time.tv_nsec >= self->stop_time.tv_nsec) ? TRUE : FALSE;
}

int osal_usleep(uint32 usec)
{
    k_usleep(usec);
    return 0;
}

int osal_monotonic_sleep(ec_timet *ts)
{
    ec_timet current, diff;
    osal_get_monotonic_time(&current);
    osal_time_diff(&current, ts, &diff);

    if (diff.tv_sec >= 0) {
        /* 将秒和纳秒统一转换为微秒 (int64_t) */
        int64_t usec = (int64_t)diff.tv_sec * 1000000 + (diff.tv_nsec / 1000);
        if (usec > 0) {
            k_sleep(K_USEC(usec));
        }
    }
    return 0;
}

/* --- 内存相关 --- */

void *osal_malloc(size_t size)
{
    return k_malloc(size);
}

void osal_free(void *ptr)
{
    k_free(ptr);
}

/* --- 线程相关 --- */

struct zephyr_thread_wrapper {
    struct k_thread thread_data;
    k_thread_stack_t *stack_mem;
};

/* * 注意：osal.h 定义的 thandle 是 void*，不是 void** * 这意味着我们把 tid (指针) 写入 thandle 指向的内存
 */
static int osal_thread_create_base(void *thandle, int stacksize, void *func, void *param, int priority)
{
    struct zephyr_thread_wrapper *wrapper;

    wrapper = k_malloc(sizeof(struct zephyr_thread_wrapper));
    if (!wrapper) return 0;

    size_t actual_stack_size = Z_KERNEL_STACK_SIZE_ADJUST(stacksize);
    if (actual_stack_size < 1024) actual_stack_size = 1024;

    wrapper->stack_mem = k_aligned_alloc(Z_KERNEL_STACK_OBJ_ALIGN, actual_stack_size);
    if (!wrapper->stack_mem) {
        k_free(wrapper);
        return 0;
    }

    k_tid_t tid = k_thread_create(&wrapper->thread_data,
                                  wrapper->stack_mem,
                                  actual_stack_size,
                                  (k_thread_entry_t)func,
                                  param, NULL, NULL,
                                  priority,
                                  0, K_NO_WAIT);

    if (thandle) {
        /* 假设 thandle 是指向 k_tid_t 的指针 */
        *(k_tid_t *)thandle = tid;
    }
    return 1;
}

int osal_thread_create(void *thandle, int stacksize, void *func, void *param)
{
    return osal_thread_create_base(thandle, stacksize, func, param, 6);
}

int osal_thread_create_rt(void *thandle, int stacksize, void *func, void *param)
{
    return osal_thread_create_base(thandle, stacksize, func, param, 1);
}

/* --- 互斥锁相关 --- */

/* * 修正：完全匹配 osal.h 的签名 void *osal_mutex_create(void)
 */
void *osal_mutex_create(void)
{
    struct k_mutex *m = k_malloc(sizeof(struct k_mutex));
    if (m) {
        k_mutex_init(m);
        return (void*)m;
    }
    return NULL;
}

void osal_mutex_destroy(void *mutex)
{
    if (mutex) {
        k_free(mutex);
    }
}

void osal_mutex_lock(void *mutex)
{
    if (mutex) {
        k_mutex_lock((struct k_mutex *)mutex, K_FOREVER);
    }
}

void osal_mutex_unlock(void *mutex)
{
    if (mutex) {
        k_mutex_unlock((struct k_mutex *)mutex);
    }
}
