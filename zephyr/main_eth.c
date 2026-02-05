#include <zephyr/kernel.h>
#include <soem/soem.h>

/* 全局变量 */
ecx_contextt ecx_context;
char IOmap[4096];
#define ec_slavecount ecx_context.slavecount

/* 包装函数 */
int ec_init(const char * ifname) { return ecx_init(&ecx_context, ifname); }
int ec_config_init(uint8 usetable) { return ecx_config_init(&ecx_context); } /* 修复了参数问题 */
int ec_config_map(void *pIOmap) { return ecx_config_map_group(&ecx_context, pIOmap, 0); }
int ec_configdc(void) { return ecx_configdc(&ecx_context); }
void ec_close(void) { ecx_close(&ecx_context); }

void main(void) {
    printk("SOEM Zephyr Sample Started.\n");

    /* === 修改1：启动前先睡 3 秒，等待 Windows 网卡就绪 === */
    printk("Waiting for network interface to stabilize (3s)...\n");
    k_sleep(K_SECONDS(3));

    /* 初始化网卡 */
    if (ec_init("eth0")) {
        printk("SOEM Initialized (Socket Opened).\n");

        /* === 修改2：循环重试扫描 === */
        int retry_count = 0;
        while (1) {
            printk("Scanning for slaves (Attempt %d)...\n", ++retry_count);

            /* 发送扫描广播 */
            if (ec_config_init(FALSE) > 0) {
                printk(">>> SUCCESS: %d Slaves found! <<<\n", ec_slavecount);
                break; /* 成功了！跳出循环 */
            } else {
                printk("No slaves found. Retrying in 1s...\n");
                k_sleep(K_SECONDS(1)); /* 失败了，睡 1 秒再试 */
            }
        }

        /* 只有扫到从站才会执行到这里 */
        ec_config_map(&IOmap);
        ec_configdc();
        printk("IO Map configured. System Ready.\n");

        ec_close();
    } else {
        printk("SOEM Init Failed (Socket error).\n");
    }
}
