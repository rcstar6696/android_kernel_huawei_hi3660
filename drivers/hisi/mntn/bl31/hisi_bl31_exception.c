/*
 * Hisilicon BL31 exception driver .
 *
 * Copyright (c) 2012-2013 Linaro Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/kthread.h>
#include <global_ddr_map.h>
#include <linux/hisi/rdr_pub.h>
#include <linux/hisi/util.h>
#include <linux/hisi/rdr_hisi_platform.h>
#include "hisi_bl31_exception.h"
#include <soc_sctrl_interface.h>
#include <soc_acpu_baseaddr_interface.h>
#include <bl31_platform_memory_def.h>
#include <linux/compiler.h>
#include <asm/compiler.h>
#include <linux/debugfs.h>

struct semaphore bl31_panic_sem;
static void *sctrl_base;

u32 get_bl31_exception_flag(void)
{
	u32 val = 0;

	if (NULL != sctrl_base)
		val = readl(SOC_SCTRL_SCBAKDATA5_ADDR(sctrl_base));/*lint !e732 */

	return val;
}

void bl31_panic_ipi_handle(void)
{
	pr_crit("bl31 panic handler in kernel.\n");
	up(&bl31_panic_sem);
}
/*lint -e715 -e838*/
int bl31_panic_thread(void *arg)
{
	while (1) {
		down(&bl31_panic_sem);

		pr_err("bl31 panic trigger system_error.\n");
		rdr_syserr_process_for_ap((u32)MODID_AP_S_BL31_PANIC, 0ULL, 0ULL);
		break;
	}
	return 0;
}

noinline int atfd_hisi_service_bl31_dbg_smc(u64 function_id, u64 arg0, u64 arg1, u64 arg2)
{
	asm volatile (
		__asmeq("%0", "x0")
		__asmeq("%1", "x1")
		__asmeq("%2", "x2")
		__asmeq("%3", "x3")
		"smc	#0\n"
		: "+r" (function_id)
		: "r" (arg0), "r" (arg1), "r" (arg2));

	return (int)function_id;
}

static int bl31_panic_debug_show(struct seq_file *s, void *unused)
{
	int ret;

	ret = atfd_hisi_service_bl31_dbg_smc((u64)BL31_DEBUG_FN_VAL,
					    0ULL,
					    0ULL,
					    0ULL);
	if (!ret)
		return -EPERM;
	return 0;
}
/*lint +e715 +e838*/
static int bl31_panic_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, bl31_panic_debug_show, &inode->i_private);
}
/*lint -e64 -e785*/
static const struct file_operations bl31_panic_debug_fops = {
	.owner	    = THIS_MODULE,
	.open	    = bl31_panic_debug_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

/*lint +e64 +e785*/
/*lint -e838*/
static int __init hisi_bl31_panic_init(void)
{
	struct rdr_exception_info_s einfo;
	struct device_node *np = NULL;
	uint32_t data[2] = { 0 };
	phys_addr_t bl31_smem_base;
	void *bl31_ctrl_addr;
	void *bl31_ctrl_addr_phys;
	s32 ret;

	sctrl_base = (void *)ioremap((phys_addr_t)SOC_ACPU_SCTRL_BASE_ADDR, 0x1000ULL);
	if (!sctrl_base) {
		pr_err("sctrl ioremap faild.\n");
		return -EPERM;
	}

	bl31_smem_base = HISI_SUB_RESERVED_BL31_SHARE_MEM_PHYMEM_BASE;
	np = of_find_compatible_node(NULL, NULL, "hisilicon, bl31_mntn");
	if (!np) {
		pr_err("%s: no compatible node found.\n", __func__);
		goto err1;
	}

	ret = of_property_read_u32_array(np, "hisi,bl31-share-mem", &data[0], 2UL);
	if (ret) {
		pr_err("%s , get val mem compatible node err.\n",
		     __func__);
		goto err1;
	}

	bl31_ctrl_addr_phys = (void *)(bl31_smem_base + data[0]);
	bl31_ctrl_addr = (void *)ioremap(bl31_smem_base + data[0], (u64)data[1]);
	if (NULL == bl31_ctrl_addr) {
		pr_err
		    ("%s: %d: allocate memory for bl31_ctrl_addr failed.\n",
		     __func__, __LINE__);
		goto err1;
	}

	pr_err("bl31_ctrl_addr_phys:%pK,bl31_ctrl_addr:%pK.\n", bl31_ctrl_addr_phys, bl31_ctrl_addr);

	/*register rdr exception type*/
	memset((void *)&einfo, 0, sizeof(struct rdr_exception_info_s));
	einfo.e_modid = MODID_AP_S_BL31_PANIC;
	einfo.e_modid_end = MODID_AP_S_BL31_PANIC;
	einfo.e_process_priority = RDR_ERR;
	einfo.e_reboot_priority = RDR_REBOOT_NOW;
	einfo.e_notify_core_mask = RDR_AP;
	einfo.e_reset_core_mask = RDR_AP;
	einfo.e_reentrant = (u32)RDR_REENTRANT_DISALLOW;
	einfo.e_exce_type = AP_S_BL31_PANIC;
	einfo.e_from_core = RDR_AP;
	memcpy((void *)einfo.e_from_module, (const void *)"RDR BL31 PANIC", sizeof("RDR BL31 PANIC"));
	memcpy((void *)einfo.e_desc, (const void *)"RDR BL31 PANIC",
			sizeof("RDR BL31 PANIC"));
	ret = (s32)rdr_register_exception(&einfo);
	if (!ret)
		pr_err("register bl31 exception fail.\n");

	sema_init(&bl31_panic_sem, 0);
	if (!kthread_run(bl31_panic_thread, NULL, "bl31_exception"))
		pr_err("create bl31_exception_thread faild.\n");


	/*enable bl31 switch:route to kernel*/
	writel((u32)0x1, bl31_ctrl_addr);
	goto succ;

err1:
	iounmap(sctrl_base);
	return -EPERM;
succ:

	return 0;
}
/*lint +e838*/
/*lint -e528 -esym(528,*)*/
/*lint -e753 -esym(753,*)*/
module_init(hisi_bl31_panic_init);
MODULE_DESCRIPTION("hisi bl31 exception driver");
MODULE_ALIAS("hisi bl31 exception module");
MODULE_LICENSE("GPL");
/*lint -e528 +esym(528,*)*/
/*lint -e753 +esym(753,*)*/
