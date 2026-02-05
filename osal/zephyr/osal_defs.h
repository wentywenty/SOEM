#ifndef _OSAL_DEFS_H_
#define _OSAL_DEFS_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 结构体打包宏 (GCC/Zephyr) */
#define OSAL_PACKED_BEGIN
#define OSAL_PACKED __attribute__((__packed__))
#define OSAL_PACKED_END

/* 互斥锁类型映射 */
#define osal_mutext struct k_mutex

/* 线程定义 */
#define OSAL_THREAD_HANDLE struct k_thread *
#define OSAL_THREAD_FUNC void
#define OSAL_THREAD_FUNC_RT void
#define OSAL_THREAD_PRIORITY_LOWEST K_PRIO_PREEMPT(10)

/* 日志打印宏 */
#define EC_PRINT(...) printk(__VA_ARGS__)

/* 时间结构体 (成员名必须是 tv_sec/tv_nsec 以匹配 osal.h 的宏) */
typedef struct {
   uint32_t tv_sec;
   uint32_t tv_nsec;
} ec_timet;

#ifdef __cplusplus
}
#endif

#endif
