/*
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 */

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ETH_P_ECAT
#undef ETH_P_ECAT
#endif

#include "osal.h"
#include "nicdrv.h"
#include <soem/ec_main.h>
#include "oshw.h"

/* Global socket handle */
static int g_sock = -1;
static struct net_if *g_iface = NULL;

uint16 oshw_htons(const uint16 host)
{
   return htons(host);
}

uint16 oshw_ntohs(const uint16 network)
{
   return ntohs(network);
}

/* Create list over available network adapters. */
ec_adaptert *oshw_find_adapters(void)
{
   ec_adaptert *ret_adapter = NULL;

   /* Allocate memory for adapter struct */
   ret_adapter = (ec_adaptert *)calloc(1, sizeof(ec_adaptert));
   if (!ret_adapter)
   {
      return NULL;
   }

   /* Hardcode adapter name to "eth0" */
   strcpy(ret_adapter->name, "eth0");
   strcpy(ret_adapter->desc, "Zephyr Default Ethernet Interface");
   ret_adapter->next = NULL;

   return ret_adapter;
}

void oshw_free_adapters(ec_adaptert *adapter)
{
   ec_adaptert *curr = adapter;
   ec_adaptert *next;

   while (curr)
   {
      next = curr->next;
      free(curr);
      curr = next;
   }
}

/* ==========================================================
 * Driver Abstraction Implementation
 * ========================================================== */

int oshw_mac_init(const uint8 *mac_address)
{
   struct sockaddr_ll src_addr;

   /* 1. Get default interface */
   g_iface = net_if_get_default();
   if (!g_iface)
   {
      printk("OSHW: No default network interface found\n");
      return 0;
   }

   /* 2. Create AF_PACKET RAW Socket using zsock_ API */
   g_sock = zsock_socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
   if (g_sock < 0)
   {
      printk("OSHW: Socket creation failed: %d\n", -errno);
      return 0;
   }

   /* 3. Bind to interface */
   memset(&src_addr, 0, sizeof(src_addr));
   src_addr.sll_family = AF_PACKET;
   src_addr.sll_ifindex = net_if_get_by_iface(g_iface);

   if (zsock_bind(g_sock, (struct sockaddr *)&src_addr, sizeof(src_addr)) < 0)
   {
      printk("OSHW: Bind failed: %d\n", -errno);
      zsock_close(g_sock);
      g_sock = -1;
      return 0;
   }

   return 1;
}

int oshw_mac_send(const void *payload, size_t tot_len)
{
   if (g_sock < 0 || !g_iface) return 0;

   struct sockaddr_ll dst_addr;
   int ret;

   memset(&dst_addr, 0, sizeof(dst_addr));
   dst_addr.sll_family = AF_PACKET;
   dst_addr.sll_ifindex = net_if_get_by_iface(g_iface);
   dst_addr.sll_protocol = htons(ETH_P_ALL);

   ret = zsock_sendto(g_sock, payload, tot_len, 0,
                      (const struct sockaddr *)&dst_addr, sizeof(dst_addr));

   if (ret < 0)
   {
      return 0;
   }
   return ret;
}

int oshw_mac_recv(void *buffer, size_t buffer_length)
{
   if (g_sock < 0) return 0;

   int ret = zsock_recv(g_sock, buffer, buffer_length, ZSOCK_MSG_DONTWAIT);

   if (ret < 0)
   {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
      {
         return 0;
      }
      return 0;
   }

   return ret;
}
