#include <nicdrv.h>
#include <oshw.h>

void ec_setupheader(void *p)
{
    (void)p;
}

int ecx_setupnic(ecx_portt *port, const char *ifname, int secondary)
{
    (void)ifname;
    (void)secondary;

    /* 初始化在 nicdrv.h 定义好的互斥锁 */
    k_mutex_init(&port->tx_mutex);
    k_mutex_init(&port->rx_mutex);
    k_mutex_init(&port->getindex_mutex);

    return 1;
}

int ecx_closenic(ecx_portt *port)
{
    (void)port;
    return 0;
}

int ecx_outframe(ecx_portt *port, uint8 idx, int sock)
{
    (void)port;
    (void)idx;
    (void)sock;
    return 0;
}

int ecx_outframe_red(ecx_portt *port, uint8 idx)
{
    (void)port;
    (void)idx;
    return 0;
}

void ecx_setbufstat(ecx_portt *port, uint8 idx, int bufstat)
{
    (void)port;
    (void)idx;
    (void)bufstat;
}

uint8 ecx_getindex(ecx_portt *port)
{
    (void)port;
    return 0;
}

int ecx_waitinframe(ecx_portt *port, uint8 idx, int timeout)
{
    (void)port;
    (void)idx;
    (void)timeout;
    return 0;
}

int ecx_srconfirm(ecx_portt *port, uint8 idx, int timeout)
{
    (void)port;
    (void)idx;
    (void)timeout;
    return 0;
}
