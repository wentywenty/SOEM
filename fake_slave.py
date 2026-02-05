import time
from scapy.all import *
from scapy.layers.l2 import Ether

# === 新增功能：主动发送测试包 ===
def send_initial_packets(iface, count=5):
    print(f"\n[*] 正在发送 {count} 个测试包 (证明我还活着)...")

    # 构造一个“广播”的 EtherCAT 包
    # 目的地址: ff:ff:ff:ff:ff:ff (广播，确保 Zephyr 能收到，不管它的 MAC 是什么)
    # 源地址: 02:00:00:00:00:99 (假冒的从站地址)
    # 类型: 0x88A4 (EtherCAT)
    # 负载: 一些简单的文本，方便你在 Wireshark 里认出来
    pkt = Ether(dst="ff:ff:ff:ff:ff:ff", src="02:00:00:00:00:99", type=0x88a4) / Raw(load=b"HELLO_ZEPHYR_IM_ALIVE")

    for i in range(count):
        try:
            sendp(pkt, iface=iface, verbose=False)
            print(f"    -> [发送] 测试包 {i+1}/{count}")
            time.sleep(0.5) # 稍微停顿一下，别发太快
        except Exception as e:
            print(f"    ❌ 发送失败: {e}")

    print("[*] 测试包发送完毕，准备进入监听模式...\n")

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
        # 使用 IFACES 对象获取网卡
        chosen_iface = IFACES.dev_from_index(idx)

        print(f"\n成功选中: {chosen_iface.name}")
        print(f"说明: {chosen_iface.description}")

        # === 这里调用主动发包函数 ===
        send_initial_packets(chosen_iface)

        print("正在监听 EtherCAT (0x88a4) ... 请启动 QEMU！")

        # 开始抓包
        sniff(iface=chosen_iface, prn=packet_handler, filter="ether proto 0x88a4")

    except Exception as e:
        print(f"\n❌ 出错了: {e}")
        print("请确保输入的是数字 26")
