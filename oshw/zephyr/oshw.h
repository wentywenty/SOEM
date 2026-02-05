#ifndef _OSHW_H_
#define _OSHW_H_
#include <zephyr/sys/byteorder.h>
#define oshw_htons(x) sys_cpu_to_be16(x)
#define oshw_ntohs(x) sys_be16_to_cpu(x)
#define oshw_htonl(x) sys_cpu_to_be32(x)
#define oshw_ntohl(x) sys_be32_to_cpu(x)
#endif
