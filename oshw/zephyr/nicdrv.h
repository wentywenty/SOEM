#ifndef _nicdrvh_
#define _nicdrvh_

#include <zephyr/kernel.h>
#include "osal.h"
/* 关键：改为 ec_type.h (必须在 osal.h 之后，因可能依赖 osal 类型) */
#include <soem/ec_type.h>

/* 结构体定义保持不变，此处省略以节省空间... */
/* 请保留之前发给您的完整结构体定义 */
typedef struct
{
   int *sock;
   ec_bufT (*txbuf)[EC_MAXBUF];
   int (*txbuflength)[EC_MAXBUF];
   ec_bufT *tempbuf;
   ec_bufT (*rxbuf)[EC_MAXBUF];
   int (*rxbufstat)[EC_MAXBUF];
   int (*rxsa)[EC_MAXBUF];
   uint64 rxcnt;
} ec_stackT;

typedef struct
{
   ec_stackT stack;
   int sockhandle;
   ec_bufT rxbuf[EC_MAXBUF];
   int rxbufstat[EC_MAXBUF];
   int rxsa[EC_MAXBUF];
   ec_bufT tempinbuf;
} ecx_redportt;

typedef struct
{
   ec_stackT stack;
   int sockhandle;
   ec_bufT rxbuf[EC_MAXBUF];
   int rxbufstat[EC_MAXBUF];
   int rxsa[EC_MAXBUF];
   ec_bufT tempinbuf;
   int tempinbufs;
   ec_bufT txbuf[EC_MAXBUF];
   int txbuflength[EC_MAXBUF];
   ec_bufT txbuf2;
   int txbuflength2;
   uint8 lastidx;
   int redstate;
   ecx_redportt *redport;
   osal_mutext getindex_mutex;
   osal_mutext tx_mutex;
   osal_mutext rx_mutex;
   /* Added missing fields */
   int maxpacket;
   int rxbuflength[EC_MAXBUF];
} ecx_portt;

extern const uint16 priMAC[3];
extern const uint16 secMAC[3];

void ec_setupheader(void *p);
int ecx_setupnic(ecx_portt *port, const char *ifname, int secondary);
int ecx_closenic(ecx_portt *port);
void ecx_setbufstat(ecx_portt *port, uint8 idx, int bufstat);
uint8 ecx_getindex(ecx_portt *port);
int ecx_outframe(ecx_portt *port, uint8 idx, int stacknumber);
int ecx_outframe_red(ecx_portt *port, uint8 idx);
int ecx_waitinframe(ecx_portt *port, uint8 idx, int timeout);
int ecx_srconfirm(ecx_portt *port, uint8 idx, int timeout);

#endif
