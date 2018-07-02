#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/smp.h>
#include <linux/kmsg_dump.h>
#include <linux/blkdev.h>
#include <linux/hisi/rdr_hisi_platform.h>


#include <asm/stacktrace.h>
#include <asm/exception.h>
#include <asm/system_misc.h>
#include <asm/cacheflush.h>

#include <linux/hisi/hisi_ddr.h>
#include <linux/hisi/mntn_record_sp.h>
#include <linux/hisi/eeye_ftrace_pub.h>

#include "bl31/hisi_bl31_exception.h"
static u32 fiq_dump_flag;

void hisi_mntn_inform(void)
{
	if (get_bl31_exception_flag() == BL31_PANIC_MAGIC)
		bl31_panic_ipi_handle();
	else {
		dmss_ipi_handler();
	}
}

asmlinkage void fiq_dump(struct pt_regs *regs, unsigned int esr)
{
	fiq_dump_flag = 0xdeaddead;
	bust_spinlocks(1);
	flush_ftrace_buffer_cache();

	printk_level_setup(LOGLEVEL_DEBUG);
	pr_crit("fiq_dump begin!\n");
	dmss_fiq_handler();
	console_verbose();
	show_regs(regs);
	blk_power_off_flush(BLK_FLUSH_NORMAL);/*Flush the storage device cache*/
	dump_stack();
	smp_send_stop();
	hisiap_nmi_notify_lpm3();
	last_task_stack_dump();
	regs_dump(); /*"sctrl", "pctrl", "peri_crg", "gic"*/

	save_module_dump_mem();
	pr_crit("fiq_dump end\n");
	mntn_show_stack_cpustall();
	kmsg_dump(KMSG_DUMP_PANIC);
	flush_cache_all();

	asm("b .");
}
