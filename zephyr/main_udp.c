#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <errno.h>
#include <stdio.h>

#define UDP_PORT 9999

void main(void) {
    printk("=== Starting Standard UDP Test (Zephyr Native API) ===\n");

    /* Wait for network stack to initialize */
    k_sleep(K_SECONDS(3));

    /* 1. Create UDP Socket using Zephyr API */
    int sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        printk("❌ Socket creation failed: errno %d\n", errno);
        return;
    }
    printk("✅ UDP Socket created (fd=%d)\n", sock);

    /* 2. Enable Broadcast */
    int broadcast_enable = 1;
    int ret = zsock_setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));
    if (ret < 0) {
        printk("⚠️ Warning: Failed to set SO_BROADCAST\n");
    }

    /* 3. Prepare Destination Address */
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(UDP_PORT);

    /* Convert IP string using Zephyr API */
    zsock_inet_pton(AF_INET, "255.255.255.255", &dest_addr.sin_addr);

    int count = 0;
    while (1) {
        char payload[32];
        snprintf(payload, sizeof(payload), "HELLO_UDP_%d", ++count);

        /* 4. Send Data using Zephyr API */
        ssize_t len = zsock_sendto(sock, payload, strlen(payload), 0,
                             (struct sockaddr *)&dest_addr, sizeof(dest_addr));

        if (len > 0) {
            printk("📡 Sent UDP packet %d: %s\n", count, payload);
        } else {
            printk("❌ Send failed: errno %d\n", errno);
        }

        k_sleep(K_SECONDS(1));
    }
}
