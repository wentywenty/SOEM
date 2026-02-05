#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/sys/byteorder.h> /* 用于大小端转换 sys_cpu_to_le16 */

/* EtherCAT 标准 EtherType */
#define ECAT_ETHERTYPE 0x88A4

/* EtherCAT 命令码 */
#define EC_CMD_APRD 0x01 /* Auto Increment Physical Read */
#define EC_CMD_BWR  0x08 /* Broadcast Write */

/* 发送间隔 */
#define SLEEP_TIME_MS 1000

/* * 定义 EtherCAT 帧头结构 (2 Bytes)
 * 使用 __packed 防止编译器进行字节对齐填充
 */
struct ec_frame_hdr {
    uint16_t len_type;
    /* 结构: Length(11bit) | Reserved(1bit) | Type(4bit)
     * 注意：EtherCAT 数据内部通常是 Little Endian
     */
} __packed;

/* * 定义 EtherCAT 子报文头结构 (10 Bytes)
 */
struct ec_datagram_hdr {
    uint8_t  cmd;       /* Command (e.g., APRD, BWR) */
    uint8_t  idx;       /* Index (Sequence number) */
    uint16_t adp;       /* Address Position (Slave Address) */
    uint16_t ado;       /* Address Offset (Register Address) */
    uint16_t len_c;     /* Length (11bit) + R/W/M bits */
    uint16_t irq;       /* Interrupt / Event */
} __packed;

int main(void)
{
    int sock;
    int ret;
    struct sockaddr_ll src_addr;
    struct sockaddr_ll dst_addr;
    struct net_if *iface;

    printk("Starting EtherCAT Master Sender...\n");

    /* 1. 获取接口 */
    iface = net_if_get_default();
    if (!iface) {
        printk("Error: No default interface.\n");
        return 0;
    }

    /* 2. 创建 Socket */
    sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock < 0) {
        printk("Failed to create socket: %d\n", -errno);
        return 0;
    }

    /* 3. 绑定 Bind */
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.sll_family = AF_PACKET;
    src_addr.sll_ifindex = net_if_get_by_iface(iface);
    if (bind(sock, (struct sockaddr *)&src_addr, sizeof(src_addr)) < 0) {
        printk("Bind failed: %d\n", -errno);
        return 0;
    }

    /* 4. 准备目标地址 (Sendto 需要) */
    memset(&dst_addr, 0, sizeof(dst_addr));
    dst_addr.sll_family = AF_PACKET;
    dst_addr.sll_ifindex = net_if_get_by_iface(iface);
    dst_addr.sll_protocol = htons(ETH_P_ALL);

    /* --- 构建 EtherCAT 包 --- */

    /* 定义缓冲区 (最大 MTU) */
    uint8_t buffer[1514];
    memset(buffer, 0, sizeof(buffer));

    /* 指针映射 */
    struct net_eth_hdr *eth = (struct net_eth_hdr *)buffer;
    struct ec_frame_hdr *ec_frame = (struct ec_frame_hdr *)(buffer + sizeof(struct net_eth_hdr));
    struct ec_datagram_hdr *ec_dgram = (struct ec_datagram_hdr *)(buffer + sizeof(struct net_eth_hdr) + sizeof(struct ec_frame_hdr));

    /* 负载数据区指针 (紧接在子报文头后面) */
    uint8_t *dgram_data = (uint8_t *)ec_dgram + sizeof(struct ec_datagram_hdr);
    /* WKC 指针 (紧接在数据后面) */
    uint16_t *wkc;

    /* 设置要读取的数据长度 (例如读取 4 字节的 Vendor ID) */
    uint16_t data_len = 4;

    /* A. 填充 Ethernet Header */
    /* EtherCAT 广播通常发往 FF:FF:FF:FF:FF:FF，或者是第一个从站 */
    memset(&eth->dst, 0xFF, 6);
    struct net_linkaddr *ll_addr = net_if_get_link_addr(iface);
    memcpy(&eth->src, ll_addr->addr, 6);
    eth->type = htons(ECAT_ETHERTYPE); /* 0x88A4 Big Endian */

    /* B. 填充 EtherCAT 子报文 (Datagram) */
    ec_dgram->cmd = EC_CMD_APRD; /* APRD: 自动增量读取 */
    ec_dgram->idx = 0x01;        /* 帧序号 */

    /* ADP: 对于 APRD，0x0000 表示第一个从站，-1 (0xFFFF) 表示第二个，以此类推 */
    ec_dgram->adp = sys_cpu_to_le16(0x0000);

    /* ADO: 寄存器地址。例如 0x0000 是 Type/Revision */
    ec_dgram->ado = sys_cpu_to_le16(0x0000);

    /* Length: 11bit 长度。最高位是 M (More) bit，这里是最后一个报文，所以为 0 */
    ec_dgram->len_c = sys_cpu_to_le16(data_len);

    ec_dgram->irq = 0x0000;

    /* 初始化数据区为 0 (等待从站填充) */
    memset(dgram_data, 0, data_len);

    /* C. 处理 Working Counter (WKC) */
    /* WKC 位于数据之后，2字节 */
    wkc = (uint16_t *)(dgram_data + data_len);
    *wkc = 0x0000; // 初始化为0

    /* D. 填充 EtherCAT 帧头 */
    /* 计算 EtherCAT 负载总长度 = Datagram Header + Data + WKC */
    uint16_t ec_payload_len = sizeof(struct ec_datagram_hdr) + data_len + 2;

    /* Frame Header = Length(11bit) | Reserved(1bit) | Type(4bit)
     * Type 1 = EtherCAT Commands
     * 组合方式: Length | (Type << 12)
     */
    uint16_t hdr_val = ec_payload_len | (1 << 12);
    ec_frame->len_type = sys_cpu_to_le16(hdr_val); /* 必须转为小端 */

    /* 计算最终发送的总字节数 */
    size_t total_len = sizeof(struct net_eth_hdr) + sizeof(struct ec_frame_hdr) + ec_payload_len;

    while (1) {
        ret = sendto(sock, buffer, total_len, 0,
                     (const struct sockaddr *)&dst_addr, sizeof(dst_addr));

        if (ret < 0) {
            printk("Send failed: %d\n", -errno);
        } else {
            printk("Sent EtherCAT frame (%d bytes). Idx: %d\n", ret, ec_dgram->idx);
            /* 每次发送增加序号，方便 Wireshark 观察 */
            ec_dgram->idx++;
        }

        k_msleep(SLEEP_TIME_MS);
    }
    return 0;
}
