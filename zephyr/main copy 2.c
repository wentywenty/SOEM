#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/ethernet.h>

/* 这是一个纯粹的 Raw Socket 发包测试 */
void main(void) {
    printk("Starting Raw Socket Test...\n");
    k_sleep(K_SECONDS(2));

    /* 1. 创建原始套接字 */
    int sock = zsock_socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
    if (sock < 0) {
        printk("❌ Error: Failed to create socket (errno %d)\n", errno);
        return;
    }
    printk("✅ Socket created successfully.\n");

    /* 2. 构造一个假的 EtherCAT 广播包 */
    /* 目的MAC(广播) + 源MAC(假) + 类型(0x88A4) + 数据 */
    uint8_t frame[] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* Dst: Broadcast */
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, /* Src: Dummy */
        0x88, 0xa4,                         /* Type: EtherCAT */
        0x48, 0x45, 0x4c, 0x4c, 0x4f        /* Data: "HELLO" */
    };

    /* 3. 循环发送 */
    int count = 0;
    while (1) {
        /* 在 Zephyr 中，未绑定的 raw socket 可能需要指定接口，或者让系统路由 */
        /* 为了简单，我们尝试直接 send。如果失败，可能需要 bind */
        ssize_t sent = zsock_send(sock, frame, sizeof(frame), 0);

        if (sent > 0) {
            printk("Packet %d sent! (%d bytes)\n", ++count, (int)sent);
        } else {
            printk("❌ Send failed (errno %d)\n", errno);
        }

        k_sleep(K_SECONDS(1));
    }
}
