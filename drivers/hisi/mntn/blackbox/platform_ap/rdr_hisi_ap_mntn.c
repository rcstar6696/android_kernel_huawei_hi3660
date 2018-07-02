#include <linux/fs.h>
#include <linux/io.h>
#include <linux/threads.h>
#include <linux/kallsyms.h>
#include <linux/reboot.h>
#include <linux/input.h>
#include <linux/syscalls.h>

#include <asm/ptrace.h>

#include <linux/hisi/util.h>
#include <linux/hisi/rdr_hisi_platform.h>
#include <linux/hisi/hisi_bootup_keypoint.h>
#include <linux/hisi/hisi_powerkey_event.h>

static u32 reboot_reason_flag;



static int reentrant_exception_num = 0;

void reentrant_exception(void)
{
	reentrant_exception_num++;

	if (reentrant_exception_num > 5)
		machine_restart("AP_S_PANIC");
}

int rdr_press_key_to_fastboot(struct notifier_block *nb,
		unsigned long event, void *buf)
{
	if (event != HISI_PRESS_KEY_UP)
		return -1;

	if (check_himntn(HIMNTN_PRESS_KEY_TO_FASTBOOT)) {
		if ((VOL_UPDOWN_PRESS & gpio_key_vol_updown_press_get())
				== VOL_UPDOWN_PRESS) {
			gpio_key_vol_updown_press_set_zero();
			if (is_gpio_key_vol_updown_pressed()) {
				pr_err("[%s]Powerkey+VolUp_key+VolDn_key\n", __func__);
				rdr_syserr_process_for_ap(MODID_AP_S_COMBINATIONKEY, 0, 0);
			}
		}
	}
	if (AP_S_PRESS6S == get_reboot_reason())
		set_reboot_reason(reboot_reason_flag);
	return 0;
}

void rdr_long_press_powerkey(void)
{
	if (STAGE_BOOTUP_END != get_boot_keypoint()) {
		pr_err("press6s in boot\n");
		save_log_to_dfx_tempbuffer(AP_S_PRESS6S);
		sys_sync();
	}else {
		reboot_reason_flag = get_reboot_reason();
		set_reboot_reason(AP_S_PRESS6S);
		pr_err("press6s:reboot_reason_flag=%u\n", reboot_reason_flag);
	}
}
