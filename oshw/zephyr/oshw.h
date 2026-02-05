#ifndef _OSHW_ZEPHYR_H_
#define _OSHW_ZEPHYR_H_

#include "osal.h"
#include <soem/ec_type.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration for ec_adaptert */
typedef struct ec_adapter ec_adaptert;

uint16 oshw_htons(const uint16 host);
uint16 oshw_ntohs(const uint16 network);
ec_adaptert *oshw_find_adapters(void);
void oshw_free_adapters(ec_adaptert *adapter);

#ifdef __cplusplus
}
#endif

#endif /* _OSHW_ZEPHYR_H_ */
