#ifndef _OSAL_ZEPHYR_DEFS_H_
#define _OSAL_ZEPHYR_DEFS_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>

/* --- 1. 基础数据类型 (SOEM 依赖) --- */
typedef uint8_t boolean;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef float float32;
typedef double float64;

/* --- 2. TRUE/FALSE --- */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* * 1. 定义 ec_timet
 * osal.h 中的 osal_timert 结构体依赖这个类型，
 * 所以必须在包含 osal.h 之前（也就是在这个文件中）定义它。
 */
#define ec_timet struct timespec

/* * 2. 关键：OSAL_PACKED 宏定义
 * 这是解决几百个 "no member named" 错误的核心。
 */
#ifndef OSAL_PACKED
#define OSAL_PACKED_BEGIN
#define OSAL_PACKED __attribute__((packed))
#define OSAL_PACKED_END
#endif

/* * 3. OSAL 句柄映射
 */
#define OSAL_THREAD_HANDLE  k_tid_t
#define OSAL_THREAD_FUNC    void
#define OSAL_THREAD_FUNC_RT void
#define osal_mutext         struct k_mutex *

/* * 4. 打印宏
 */
#define OSAL_PRINTF(...)    printk(__VA_ARGS__)
#define OSAL_ERROR(...)     printk("Error: " __VA_ARGS__)

#ifdef EC_DEBUG
#define EC_PRINT(...) printk(__VA_ARGS__)
#else
#define EC_PRINT(...) \
   do                 \
   {                  \
   } while (0)
#endif

#endif /* _OSAL_ZEPHYR_DEFS_H_ */
