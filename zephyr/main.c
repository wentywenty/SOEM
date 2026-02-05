#include <zephyr/kernel.h>
#include <soem/soem.h>

/* 1. 定义全局 Context */
ecx_contextt ecx_context;
char IOmap[4096];

/* 2. 定义旧版宏/包装函数映射 */
/* 将 ec_slavecount 宏定义为访问全局 context 的成员 */
#define ec_slavecount ecx_context.slavecount

/* 包装函数实现 */
int ec_init(const char * ifname)
{
   return ecx_init(&ecx_context, ifname);
}

int ec_config_map(void *pIOmap)
{
   /* config_map_group, group 0 means all */
   return ecx_config_map_group(&ecx_context, pIOmap, 0);
}

int ec_configdc(void)
{
   return ecx_configdc(&ecx_context);
}

void ec_close(void)
{
   ecx_close(&ecx_context);
}

/* 3. 主程序 */
void main(void) {
    printk("SOEM Zephyr Sample Started.\n");

    /* 初始化 EtherCAT */
    if (ec_init("eth0")) {
        printk("SOEM Initialized (Stub Success).\n");

        /* 配置 IOmap */
        ec_config_map(&IOmap);
        ec_configdc();

        printk("Slaves found: %d\n", ec_slavecount);

        ec_close();
    } else {
        printk("SOEM Init Failed.\n");
    }
}
