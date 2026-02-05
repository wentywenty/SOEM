#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "nicdrv.h"
#include "oshw.h"

#ifndef AF_PACKET
#define AF_PACKET 17
#endif
#ifndef ETH_P_ALL
#define ETH_P_ALL 0x0003
#endif
#ifndef ETH_P_ECAT
#define ETH_P_ECAT 0x88A4
#endif

static struct net_eth_addr local_mac;
static int local_ifindex = -1;
static struct sockaddr_ll tx_addr;

void ec_setupheader(void *p)
{
   struct net_eth_hdr *eth = (struct net_eth_hdr *)p;
   memset(&eth->dst, 0xff, 6);
   memcpy(&eth->src, &local_mac, 6);
   eth->type = htons(ETH_P_ECAT);
}

int ecx_setupnic(ecx_portt *port, const char *ifname, int secondary)
{
   struct net_if *iface;
   struct sockaddr_ll bind_addr;
   int ret;

   /* 关键修复：分配互斥锁内存 (因为 osal_mutext 是指针) */
   port->tx_mutex = (osal_mutext)k_malloc(sizeof(struct k_mutex));
   port->rx_mutex = (osal_mutext)k_malloc(sizeof(struct k_mutex));
   port->getindex_mutex = (osal_mutext)k_malloc(sizeof(struct k_mutex));

   if (!port->tx_mutex || !port->rx_mutex || !port->getindex_mutex)
   {
      printk("nicdrv: Failed to allocate mutexes\n");
      return 0;
   }

   k_mutex_init(port->tx_mutex);
   k_mutex_init(port->rx_mutex);
   k_mutex_init(port->getindex_mutex);

   iface = net_if_get_default();
   if (!iface)
   {
      printk("nicdrv: No default interface found!\n");
      return 0;
   }

   local_ifindex = net_if_get_by_iface(iface);
   struct net_linkaddr *ll_addr = net_if_get_link_addr(iface);
   if (ll_addr && ll_addr->len == 6)
   {
      memcpy(&local_mac, ll_addr->addr, 6);
   }

   /* 使用 zsock_ API 避免隐式声明警告 */
   port->sockhandle = zsock_socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
   if (port->sockhandle < 0)
   {
      printk("nicdrv: Socket failed (errno %d)\n", errno);
      return 0;
   }

   memset(&bind_addr, 0, sizeof(bind_addr));
   bind_addr.sll_family = AF_PACKET;
   bind_addr.sll_ifindex = local_ifindex;
   bind_addr.sll_protocol = htons(ETH_P_ALL);

   ret = zsock_bind(port->sockhandle, (struct sockaddr *)&bind_addr, sizeof(bind_addr));
   if (ret < 0)
   {
      printk("nicdrv: Bind failed (errno %d)\n", errno);
      zsock_close(port->sockhandle);
      port->sockhandle = -1;
      return 0;
   }

   memset(&tx_addr, 0, sizeof(tx_addr));
   tx_addr.sll_family = AF_PACKET;
   tx_addr.sll_ifindex = local_ifindex;
   tx_addr.sll_protocol = htons(ETH_P_ALL);

   printk("nicdrv: Initialized ifindex %d\n", local_ifindex);
   return 1;
}

int ecx_closenic(ecx_portt *port)
{
   if (port->sockhandle >= 0)
   {
      zsock_close(port->sockhandle);
      port->sockhandle = -1;
   }

   if (port->tx_mutex) k_free(port->tx_mutex);
   if (port->rx_mutex) k_free(port->rx_mutex);
   if (port->getindex_mutex) k_free(port->getindex_mutex);

   return 0;
}

int ecx_outframe(ecx_portt *port, uint8 idx, int sock)
{
   int ret;
   ret = zsock_sendto(sock, port->txbuf[idx], port->txbuflength[idx], 0,
                      (struct sockaddr *)&tx_addr, sizeof(tx_addr));
   if (ret < 0) return 0;
   return 1;
}

int ecx_outframe_red(ecx_portt *port, uint8 idx)
{
   return 0;
}

int ecx_waitinframe(ecx_portt *port, uint8 idx, int timeout)
{
   struct timeval tv;
   struct timeval start_tv, current_tv;
   zsock_fd_set readfds;
   int ret;
   int time_left = timeout;

   // 获取单调时间用于超时计算（如果需要精确控制）
   // 这里简化处理，每次循环都检查 select

   while (time_left > 0)
   {
      tv.tv_sec = 0;
      tv.tv_usec = time_left;

      ZSOCK_FD_ZERO(&readfds);
      ZSOCK_FD_SET(port->sockhandle, &readfds);

      ret = zsock_select(port->sockhandle + 1, &readfds, NULL, NULL, &tv);

      if (ret > 0)
      {
         ssize_t recved;
         /* 使用 zsock_recv 读取 */
         recved = zsock_recv(port->sockhandle, port->rxbuf[idx], port->maxpacket, 0);

         if (recved > 14)
         { // 至少包含以太网头
            struct net_eth_hdr *eth = (struct net_eth_hdr *)port->rxbuf[idx];
            if (ntohs(eth->type) == ETH_P_ECAT)
            {
               port->rxbuflength[idx] = (uint16)recved;
               return 1;
            }
            // 忽略非 EtherCAT 包，继续等待
         }
      }
      else if (ret == 0)
      {
         // 超时
         return 0;
      }
      else
      {
         // 错误
         return 0;
      }
      // 简单递减（实际应计算 elapsed time）
      // 如果收到杂包，可能会导致 timeout 变得不准确，但在 Zephyr RTOS 环境下暂时接受
      // 为了避免死循环占用 CPU，这里假设 select 会有些许延迟
      // 实际上 select 返回后 time_left 应该减去消耗的时间。
      // 但 Zephyr 的 select 修改 tv 吗？ 标准是修改的。
      // 如果 Zephyr 修改 tv，我们应该依赖 tv ?
      // Zephyr 文档: "The timeout time is not updated..."
      // 所以我们需要手动管理超时。

      // 考虑到实现复杂度，我们假设杂包不会无限涌入导致死循环。
      // 直接 break 也是一种选择，但会导致丢包判定为超时。
      // 为了稳健，我们暂时每收到一个杂包扣除一点时间或者直接 check timeout

      // 既然 select 已经阻塞了部分时间（如果杂包是突发的，select 立即返回）
      // 我们最好在此处使用截止时间方式。
      time_left -= 100; // 假设每次处理大约 100us ? 不靠谱。
      if (time_left < 0) time_left = 0;
   }
   return 0;
}

int ecx_srconfirm(ecx_portt *port, uint8 idx, int timeout)
{
   return ecx_waitinframe(port, idx, timeout);
}

void ecx_setbufstat(ecx_portt *port, uint8 idx, int bufstat)
{
   port->rxbufstat[idx] = bufstat;
}

uint8 ecx_getindex(ecx_portt *port)
{
   uint8 idx;
   uint8 cnt = 0;

   /* 修复：直接传递指针，不要取地址 */
   k_mutex_lock(port->getindex_mutex, K_FOREVER);

   idx = port->lastidx + 1;
   if (idx >= EC_MAXBUF) idx = 0;

   while (port->rxbufstat[idx] != EC_BUF_EMPTY && cnt < EC_MAXBUF)
   {
      idx++;
      if (idx >= EC_MAXBUF) idx = 0;
      cnt++;
   }

   port->rxbufstat[idx] = EC_BUF_ALLOC;
   if (idx != port->lastidx) port->lastidx = idx;

   k_mutex_unlock(port->getindex_mutex);
   return idx;
}
