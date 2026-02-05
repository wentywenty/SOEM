#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>

/* 定义一个自定义的 EtherType */
#define MY_CUSTOM_ETHERTYPE 0x88a4

/* 发送间隔 */
#define SLEEP_TIME_MS 1000

int main(void)
{
    int sock;
    int ret;
    struct sockaddr_ll src_addr;
    struct sockaddr_ll dst_addr; // [修改点1] 新增目标地址结构
    struct net_if *iface;

    printk("Starting Raw L2 Sender...\n");

    /* 1. 获取默认网络接口 */
    iface = net_if_get_default();
    if (!iface) {
        printk("Error: No default network interface found.\n");
        return 0;
    }

    /* 2. 创建 AF_PACKET RAW Socket */
    // 注意：第三个参数 protocol 最好使用 network byte order
    sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    if (sock < 0) {
        printk("Failed to create socket: %d\n", -errno);
        return 0;
    }

    /* 3. 绑定 Socket 到特定接口 */
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.sll_family = AF_PACKET;
    src_addr.sll_ifindex = net_if_get_by_iface(iface);

    ret = bind(sock, (struct sockaddr *)&src_addr, sizeof(src_addr));
    if (ret < 0) {
        printk("Failed to bind socket: %d\n", -errno);
        return 0;
    }

    /* [修改点2] 准备 sendto 需要的目标地址结构 */
    /* 对于 SOCK_RAW，虽然以太网头里已经有了 MAC，但内核需要 ifindex 来决定走哪个网卡 */
    memset(&dst_addr, 0, sizeof(dst_addr));
    dst_addr.sll_family = AF_PACKET;
    dst_addr.sll_ifindex = net_if_get_by_iface(iface);
    dst_addr.sll_protocol = htons(ETH_P_ALL);
    /* 注意：因为我们自己填充了 Ethernet Header，这里的 sll_addr (目的MAC) 可以不填，
       内核会直接发送 buffer 里的内容。但 ifindex 是必须的。 */


    /* 准备发送缓冲区 */
    uint8_t buffer[128];
    struct net_eth_hdr *eth_header = (struct net_eth_hdr *)buffer;
    uint8_t *payload = buffer + sizeof(struct net_eth_hdr);
    const char *msg = "Hello L2 World!";
    int payload_len = strlen(msg);

    /* 4. 填充 Ethernet 头部 */
    /* 目的 MAC: 广播 */
    memset(&eth_header->dst, 0xFF, 6);

    /* 源 MAC */
    struct net_linkaddr *ll_addr = net_if_get_link_addr(iface);
    memcpy(&eth_header->src, ll_addr->addr, 6);

    /* EtherType */
    eth_header->type = htons(MY_CUSTOM_ETHERTYPE);

    /* 5. 填充 Payload */
    memcpy(payload, msg, payload_len);

    /* 总发送长度 */
    size_t send_len = sizeof(struct net_eth_hdr) + payload_len;

    while (1) {
        /* [修改点3] 使用 sendto 替代 send */
        /* sendto 允许显式传入 dst_addr，从而告诉内核 ifindex */
        ret = sendto(sock, buffer, send_len, 0,
                     (const struct sockaddr *)&dst_addr, sizeof(dst_addr));

        if (ret < 0) {
            printk("Send failed: %d\n", -errno);
        } else {
            printk("Sent %d bytes of raw L2 data.\n", ret);
        }

        k_msleep(SLEEP_TIME_MS);
    }
    return 0;
}
