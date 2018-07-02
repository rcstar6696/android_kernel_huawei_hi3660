#include <linux/delay.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/mmc/cmdq_hci.h>
#include "hisi_mmc_debug.h"

#ifdef CONFIG_MMC_CQ_HCI
int halt_flag = 0;
int retry_flag = 0;
int clear_task_flag = 0;
int resend_flag = 0;
int dcmd_debug_flag = 0;

/*ecall command invoke this 5 function to trigger reset process*/
void cmdq_set_halt_flag(void)
{
	halt_flag = 1;
}

void cmdq_set_retry_flag(void)
{
	retry_flag = 1;
}

void cmdq_set_clear_task_flag(void)
{
	clear_task_flag = 1;
}

void cmdq_set_resend_flag(void)
{
	resend_flag = 1;
}

void cmdq_set_dcmd_debug_flag(void)
{
	dcmd_debug_flag = 1;
}

void cmdq_clear_dcmd_debug_flag(void)
{
	dcmd_debug_flag = 0;
}

void cmdq_dcmd_wait(void)
{
	int count= 0;

	while (dcmd_debug_flag) {
		resend_flag = 1;
		msleep(10);
		count ++;
		/* if wait time is more than 5 second, we think the cmdq_irq
		   does not trigger the resend process, break for next loop*/
		if (count > 500)
			break;
	}
}

u32 cmdq_trigger_irq_err(u32 err)
{
	if (resend_flag)
		err = 1;
	return err;
}

u32 cmdq_trigger_irq_resend(u32 err_info, u32 mask)
{
	if (resend_flag) {
		err_info |= mask;
		resend_flag = 0;
	}
	return err_info;
}

u32 cmdq_trigger_resend(u32 val, u32 mask)
{
	if (retry_flag || clear_task_flag)
		val |= mask;
	return val;
}

extern int cmdq_reset(struct cmdq_host *cq_host);
int cmdq_halt_trigger_reset(struct cmdq_host *cq_host)
{
	if (halt_flag) {
		pr_err("manual tirgger halt reset\n");
		halt_flag = 0;
		cmdq_reset(cq_host);
		return 1;
	}

	return 0;
}

int cmdq_retry_trigger_reset(struct cmdq_host *cq_host)
{
	if (retry_flag) {
		pr_err("manual tirgger retry reset\n");
		retry_flag = 0;
		cmdq_reset(cq_host);
		return 1;
	}

	return 0;
}

int cmdq_clear_task_trigger_reset(struct cmdq_host *cq_host)
{
	if (clear_task_flag) {
		pr_err("manual tirgger clear task reset\n");
		clear_task_flag = 0;
		cmdq_reset(cq_host);
		return 1;
	}

	return 0;
}
#endif
