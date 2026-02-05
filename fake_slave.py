from scapy.all import *
from scapy.layers.l2 import Ether

def packet_handler(pkt):
    # 1. 过滤：只处理 EtherCAT 协议 (0x88A4)
    if not pkt.haslayer(Ether) or pkt.type != 0x88a4:
        return

    # 2. 伪造回复包
    reply = pkt.copy()

    # 交换 MAC 地址
    master_mac = pkt.src
    reply.dst = master_mac
    reply.src = "02:00:00:00:00:99"  # 假装是从站

    # 3. 修改 WKC (证明从站存在)
    raw_payload = bytes(reply.payload)
    if len(raw_payload) > 2:
        pkt_bytes = bytearray(bytes(reply))

        # 将最后两个字节 (WKC) 设为 1
        pkt_bytes[-2] = 0x01
        pkt_bytes[-1] = 0x00

        # 4. 发送回去
        try:
            sendp(pkt_bytes, iface=chosen_iface, verbose=False)
            print(f"[*] 已回复 EtherCAT 包 -> Zephyr (WKC=1)")
        except Exception as e:
            print(f"发送失败: {e}")

if __name__ == "__main__":
    print("-" * 60)
    print("★ 请直接输入 TAP 网卡的序号 (Index)")
    print("★ 根据刚才的记录，你的 TAP 网卡序号是: 26")
    print("-" * 60)

    idx_str = input("请输入序号 (直接输 26): ")

    try:
        idx = int(idx_str)
        # 使用 IFACES 对象获取网卡，这是最稳的方法
        chosen_iface = IFACES.dev_from_index(idx)

        print(f"\n成功选中: {chosen_iface.name}")
        print(f"说明: {chosen_iface.description}")
        print("正在监听 EtherCAT (0x88a4) ... 请启动 QEMU！")

        # 开始抓包
        sniff(iface=chosen_iface, prn=packet_handler, filter="ether proto 0x88a4")

    except Exception as e:
        print(f"\n❌ 出错了: {e}")
        print("请确保输入的是数字 26")
