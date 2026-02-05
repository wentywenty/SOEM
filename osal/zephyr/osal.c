#include <osal_defs.h>
#include <osal.h>
#include <zephyr/kernel.h>

void osal_get_monotonic_time(ec_timet *ts)
{
    uint64_t ticks = k_uptime_ticks();
    uint64_t ns = k_ticks_to_ns_floor64(ticks);
    ts->tv_sec = (uint32_t)(ns / 1000000000ULL);
    ts->tv_nsec = (uint32_t)(ns % 1000000000ULL);
}

ec_timet osal_current_time(void)
{
    ec_timet ts;
    osal_get_monotonic_time(&ts);
    return ts;
}

void osal_time_diff(ec_timet *start, ec_timet *end, ec_timet *diff)
{
    if (end->tv_nsec < start->tv_nsec) {
        diff->tv_sec = end->tv_sec - start->tv_sec - 1;
        diff->tv_nsec = 1000000000 + end->tv_nsec - start->tv_nsec;
    } else {
        diff->tv_sec = end->tv_sec - start->tv_sec;
        diff->tv_nsec = end->tv_nsec - start->tv_nsec;
    }
}

void osal_timer_start(osal_timert *self, uint32 timeout_usec)
{
    ec_timet now, diff;
    now = osal_current_time();
    diff.tv_sec = timeout_usec / 1000000;
    diff.tv_nsec = (timeout_usec % 1000000) * 1000;
    self->stop_time.tv_sec = now.tv_sec + diff.tv_sec;
    self->stop_time.tv_nsec = now.tv_nsec + diff.tv_nsec;
    if (self->stop_time.tv_nsec >= 1000000000) {
        self->stop_time.tv_sec++;
        self->stop_time.tv_nsec -= 1000000000;
    }
}

boolean osal_timer_is_expired(osal_timert *self)
{
    ec_timet now;
    now = osal_current_time();
    if (now.tv_sec > self->stop_time.tv_sec) return TRUE;
    if (now.tv_sec < self->stop_time.tv_sec) return FALSE;
    return (now.tv_nsec >= self->stop_time.tv_nsec) ? TRUE : FALSE;
}

int osal_usleep(uint32 usec) { k_usleep(usec); return 0; }
int osal_monotonic_sleep(ec_timet *ts) { return 0; }
void *osal_malloc(size_t size) { return k_malloc(size); }
void osal_free(void *ptr) { k_free(ptr); }
int osal_thread_create(void *thandle, int stacksize, void *func, void *param) { return 0; }
int osal_thread_create_rt(void *thandle, int stacksize, void *func, void *param) { return 0; }
void *osal_mutex_create(void) {
    struct k_mutex *m = k_malloc(sizeof(struct k_mutex));
    if (m) k_mutex_init(m);
    return m;
}
void osal_mutex_destroy(void *mutex) { k_free(mutex); }
void osal_mutex_lock(void *mutex) { k_mutex_lock((struct k_mutex *)mutex, K_FOREVER); }
void osal_mutex_unlock(void *mutex) { k_mutex_unlock((struct k_mutex *)mutex); }
