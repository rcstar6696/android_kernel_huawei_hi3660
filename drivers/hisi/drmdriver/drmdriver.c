
/*lint -e715 -esym(715,*) */
/*lint -e818 -esym(818,*) */
#include <asm/compiler.h>
#include <linux/dma-mapping.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <global_ddr_map.h>
#include <linux/hisi/hisi_drmdriver.h>

#define ATFD_MEM_START_ADDRESS  "hisi,share-memory-drm"


/* the interface is used as access some secure registers from unsecure-Kernel layer.
 * main_fun_id--input parameter, fixed as ACCESS_REGISTER_FN_MAIN_ID,equal to 0xc500aa01
 * buff_addr_phy--input parameter for write, output para for read, the allocated buffer address in Kernel, must be physical address
 * data_len--the buffer size, unit is Bytes
 * sub_fun_id--the sub function id, started by 0x55bbcce0, the index increase one by one
*/
noinline int atfd_hisi_service_access_register_smc(u64 main_fun_id, u64 buff_addr_phy, u64 data_len, u64 sub_fun_id)
{
    asm volatile(
            __asmeq("%0", "x0")
            __asmeq("%1", "x1")
            __asmeq("%2", "x2")
            __asmeq("%3", "x3")
            "smc    #0\n"
        : "+r" (main_fun_id)
        : "r" (buff_addr_phy), "r" (data_len), "r" (sub_fun_id));

    return (int)main_fun_id;
}

static ATFD_DATA g_atfd_config;
void configure_master_security(unsigned int is_security, int master_id)
{
	if (master_id >= MASTER_ID_MAX || master_id == MASTER_DSS_ID || master_id < 0) {
		printk(KERN_ERR "%s %d, invalid master_id=%d.\n", __func__, __LINE__, (int)master_id);
		return;
	}
	if (0 != is_security  && 1 != is_security) {
		printk(KERN_ERR "%s %d, invalid is_security=%d.\n", __func__, __LINE__, is_security);
		return;
	}
	is_security |= 0xabcde0;
	(void)atfd_hisi_service_access_register_smc(ACCESS_REGISTER_FN_MAIN_ID,
			(u64)is_security, (u64)master_id,/*lint !e571*/
			(u64)ACCESS_REGISTER_FN_SUB_ID_MASTER_SECURITY_CONFIG);
	return;
}
EXPORT_SYMBOL_GPL(configure_master_security);

void configure_dss_register_security(uint32_t addr, uint32_t val, uint8_t bw, uint8_t bs)
{
	struct dss_reg {
		uint32_t addr;
		uint32_t val;
		uint8_t bw;
		uint8_t bs;
	} dss_data;
	dss_data.addr = addr;
	dss_data.val = val;
	dss_data.bw = bw;
	dss_data.bs = bs;
	if (!g_atfd_config.buf_virt_addr) {
		printk(KERN_ERR "%s %d, no available mem\n", __func__, __LINE__);
		return;
	}
	if (DRMDRIVER_MODULE_INIT_SUCCESS_FLG != g_atfd_config.module_init_success_flg) {
		printk(KERN_ERR "%s %d, module is not ready now\n", __func__, __LINE__);
		return;
	}
	BUG_ON(in_interrupt());
	mutex_lock(&g_atfd_config.atfd_mutex);
	memcpy((struct dss_reg*)g_atfd_config.buf_virt_addr, &dss_data, sizeof(dss_data));
	(void)atfd_hisi_service_access_register_smc(ACCESS_REGISTER_FN_MAIN_ID, g_atfd_config.buf_phy_addr, (u64)MASTER_DSS_ID, ACCESS_REGISTER_FN_SUB_ID_MASTER_SECURITY_CONFIG);
	mutex_unlock(&g_atfd_config.atfd_mutex);
	return;
}
EXPORT_SYMBOL_GPL(configure_dss_register_security);

int configure_dss_service_security(unsigned int master_op_type, unsigned int channel, unsigned int mode)
{
	u64 value;
	if ((mode >= MAX_COMPOSE_MODE) || (channel >= MAX_DSS_CHN_IDX)) {
		printk(KERN_ERR "%s %d, invalid mode=%d, channel=%d", __func__, __LINE__, mode, channel);
		return -1;
	}
	if (master_op_type >= (unsigned int)MASTER_OP_SECURITY_MAX) {
		printk(KERN_ERR "%s %d, invalid master_op_type=%d", __func__, __LINE__, master_op_type);
		return -1;
	}
	value = (((u64)mode << 48) | ((u64)channel << 32) | master_op_type | 0xabcde0);
	return atfd_hisi_service_access_register_smc(ACCESS_REGISTER_FN_MAIN_ID,
		value, (u64)MASTER_DSS_ID, (u64)ACCESS_REGISTER_FN_SUB_ID_MASTER_SECURITY_CONFIG);
}
EXPORT_SYMBOL_GPL(configure_dss_service_security);

static int __init hisi_drm_driver_init(void)
{
	int ret = 0;
	struct device_node *np = NULL;
	uint32_t data[2] = {0};
	phys_addr_t bl31_smem_base = HISI_SUB_RESERVED_BL31_SHARE_MEM_PHYMEM_BASE;

	memset((void *)&g_atfd_config, 0, sizeof(g_atfd_config));
	np = of_find_compatible_node(NULL, NULL, ATFD_MEM_START_ADDRESS);
    if (!np) {
        printk(KERN_ERR "%s: no compatible node found.\n", __func__);
        return -ENODEV;
    }
	if (0 != (ret = of_property_read_u32_array(np, "reg", &data[0], 2))) {
        if (-EINVAL == ret)
            printk(KERN_ERR "%s in reg node does not exist\n", "reg");
        else
            printk(KERN_ERR "%s in reg node has invalid value\n", "reg");
		return ret;
    }
	g_atfd_config.buf_phy_addr = bl31_smem_base + data[0];
    g_atfd_config.buf_virt_addr = (unsigned char *)ioremap(bl31_smem_base + data[0], data[1]);
    if (NULL == g_atfd_config.buf_virt_addr) {
        ret = -EFAULT;
        printk(KERN_ERR "%s: %d: allocate memory for g_atfd_config.buf_virt_addr failed.\n", __func__, __LINE__);
		return ret;
    }
	mutex_init(&(g_atfd_config.atfd_mutex));
	g_atfd_config.module_init_success_flg = DRMDRIVER_MODULE_INIT_SUCCESS_FLG;
	return ret;
}

arch_initcall(hisi_drm_driver_init);
MODULE_DESCRIPTION("Hisilicon drm driver module");
MODULE_AUTHOR("lvtaolong@huawei.com.sh");
MODULE_LICENSE("GPL");

/*lint +e715 +esym(715,*) */
/*lint +e818 +esym(818,*) */
