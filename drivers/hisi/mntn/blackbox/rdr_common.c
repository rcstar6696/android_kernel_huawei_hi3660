#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/rtc.h>
#include <linux/notifier.h>
#include <linux/suspend.h>
#include <linux/atomic.h>
#include <linux/delay.h>
#include <linux/reboot.h>
#include <linux/vmalloc.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/version.h>

#include <linux/hisi/rdr_pub.h>

#include "rdr_inner.h"
#include "rdr_field.h"
#include "rdr_print.h"
#include "rdr_debug.h"



/*
 * 函数名: bbox_check_edition
 * 函数参数:
 *     void
 * 返回值:
 *     unsigned int:	返回版本信息
 *				0x01        EDITION_USER
 *				0x02        EDITION_INTERNAL_BETA
 *				0x03        EDITION_OVERSEA_BETA
 */
unsigned int bbox_check_edition(void)
{
	char tmp;
	unsigned int type;
	int fd;
	long cnt;
	unsigned int dump_flag;

	fd = (int)sys_open(FILE_EDITION, O_RDONLY, FILE_LIMIT);
	if (fd < 0) {
		pr_err("[%s]: open %s failed, return [%d]\n", __func__,
			FILE_EDITION, fd);
		return EDITION_USER;
	}

	cnt = sys_read((unsigned int)fd, &tmp, sizeof(tmp));
	if (cnt < 0) {
		pr_err("[%s]: read %s failed, return [%ld]\n",
			__func__, FILE_EDITION, cnt);
		dump_flag = EDITION_USER;
		goto out;
	}

	if (tmp >= START_CHAR_0 && tmp <= END_CHAR_9) {
		type = (unsigned int)(unsigned char)(tmp - START_CHAR_0);

		if (OVERSEA_USER == type) {
			BB_PRINT_DBG("%s: The edition is Oversea BETA, type is %#x\n", __func__, type);
			dump_flag = EDITION_OVERSEA_BETA;
		} else if (BETA_USER == type) {
			BB_PRINT_DBG("%s: The edition is Internal BETA, type is %#x\n", __func__, type);
			dump_flag =  EDITION_INTERNAL_BETA;
		} else if (COMMERCIAL_USER == type){
			BB_PRINT_DBG("%s: The edition is Commercial User, type is %#x\n", __func__, type);
			dump_flag = EDITION_USER;
		} else {
			BB_PRINT_DBG("%s: The edition is default User, type is %#x\n", __func__, type);
			dump_flag = EDITION_USER;
		}
	} else {
		BB_PRINT_ERR("%s: The edition is default User, please check %s\n", __func__, FILE_EDITION);
		dump_flag = EDITION_USER;
	}

out:
	sys_close((unsigned int)fd);
	return dump_flag;
}

void rdr_get_builddatetime(u8 *out)
{
	u8 *pout = out;
	u8 *p = (u8 *) __DATE__;	/* Nov 11 2013 */
	int cnt = RDR_BUILD_DATE_TIME_LEN;

	if (NULL == out) {
		BB_PRINT_ERR("[%s], out is null!\n", __func__);
		return;
	}
	memset((void *)out, 0, RDR_BUILD_DATE_TIME_LEN);

	while (*p) {
		if (!cnt)
			return;
		if (*p != ' ') {
			*pout++ = *p++;
			cnt--;
		} else
			p++;
	}

	p = (u8 *) __TIME__;	/* 11:04:08 */
	while (*p) {
		if (!cnt)
			return;
		if (*p != ':') {
			*pout++ = *p++;
			cnt--;
		} else
			p++;
	}
	out[RDR_BUILD_DATE_TIME_LEN - 1] = '\0';
	return;
}

/*
 * func name: rdr_get_tick
 * .
 * func args:
 *  char*  buf,   return data buf.
 *  int    len    buf len.
 * return
 *	<0 fail
 *	0  success
 */
u64 rdr_get_tick(void)
{
	/* use only one int value to save time: */
	struct timespec uptime;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,4,0))
	do_posix_clock_monotonic_gettime(&uptime);
#else
	ktime_get_ts(&uptime);
#endif
	get_monotonic_boottime(&uptime);
	return (u64)uptime.tv_sec;
}

/*
 * func name: rdr_get_timestamp
 * .
 * func args:
 *  char*  buf,   return data buf.
 *  int    len    buf len.
 * return
 *	<0 fail
 *	0  success
 */
char *rdr_get_timestamp(void)
{
	struct rtc_time tm;
	struct timeval tv;
	static char databuf[DATA_MAXLEN + 1];
	BB_PRINT_START();

	memset(databuf, 0, DATA_MAXLEN + 1);

	memset(&tv, 0, sizeof(struct timeval));
	memset(&tm, 0, sizeof(struct rtc_time));
	do_gettimeofday(&tv);
	tv.tv_sec -= (long)sys_tz.tz_minuteswest * 60;
	rtc_time_to_tm(tv.tv_sec, &tm);

	snprintf(databuf, DATA_MAXLEN + 1, "%04d%02d%02d%02d%02d%02d",
		 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		 tm.tm_hour, tm.tm_min, tm.tm_sec);

	BB_PRINT_DBG("rdr: rdr_get_timestamp [%s] !\n", databuf);
	BB_PRINT_END();
	return databuf;
}

/*Add, please keep the same as definition in reboot_reason.c in fastboot !!!!*/
struct cmdword reboot_reason_map[] = {
	{"AP_S_COLDBOOT", AP_S_COLDBOOT},
	{"bootloader", BOOTLOADER},
	{"recovery", RECOVERY},
	{"resetfactory", RESETFACTORY},
	{"resetuser", RESETUSER},
	{"sdupdate", SDUPDATE},
	{"chargereboot", CHARGEREBOOT},
	{"resize", RESIZE},
	{"erecovery", ERECOVERY},
	{"usbupdate", USBUPDATE},
	{"cust", CUST},
	{"oem_rtc", OEM_RTC},
	{"unknown", UNKNOWN},
	{"mountfail", MOUNTFAIL},
	{"hungdetect", HUNGDETECT},
	{"coldboot", COLDBOOT},
	{"updatedataimg", UPDATEDATAIMG},
	{"AP_S_FASTBOOTFLASH", AP_S_FASTBOOTFLASH},
	{"AP_S_ABNORMAL", AP_S_ABNORMAL},
	{"AP_S_TSENSOR0", AP_S_TSENSOR0},
	{"AP_S_TSENSOR1", AP_S_TSENSOR1},
	{"AP_S_AWDT", AP_S_AWDT},
	{"CHARGER_S_WDT", CHARGER_S_WDT},
	{"G3D_S_G3DTSENSOR", G3D_S_G3DTSENSOR},
	{"LPM3_S_LPMCURST", LPM3_S_LPMCURST},
	{"CP_S_CPTSENSOR", CP_S_CPTSENSOR},
	{"IOM3_S_IOMCURST", IOM3_S_IOMCURST},
	{"ASP_S_ASPWD", ASP_S_ASPWD},
	{"CP_S_CPWD", CP_S_CPWD},
	{"IVP_S_IVPWD", IVP_S_IVPWD},
	{"ISP_S_ISPWD", ISP_S_ISPWD},
	{"AP_S_DDR_UCE_WD", AP_S_DDR_UCE_WD},
	{"AP_S_DDR_FATAL_INTER", AP_S_DDR_FATAL_INTER},
	{"OCBC_S_WD", OCBC_S_WD},
	{"AP_S_PANIC", AP_S_PANIC},
	{"AP_S_NOC", AP_S_NOC},
	{"AP_S_RESUME_SLOWY", AP_S_RESUME_SLOWY},
	{"AP_S_DDRC_SEC", AP_S_DDRC_SEC},
	{"AP_S_F2FS", AP_S_F2FS},
	{"AP_S_COMBINATIONKEY", AP_S_COMBINATIONKEY},
	{"AP_S_BL31_PANIC", AP_S_BL31_PANIC},
	{"AP_S_MAILBOX", AP_S_MAILBOX},
	{"AP_S_HHEE_PANIC", AP_S_HHEE_PANIC},
	{"CP_S_MODEMDMSS", CP_S_MODEMDMSS},
	{"CP_S_MODEMAP", CP_S_MODEMAP},
	{"CP_S_MODEMNOC", CP_S_MODEMNOC},
	{"CP_S_EXCEPTION", CP_S_EXCEPTION},
	{"CP_S_RESETFAIL", CP_S_RESETFAIL},
	{"CP_S_NORMALRESET", CP_S_NORMALRESET},
	{"CP_S_RILD_EXCEPTION", CP_S_RILD_EXCEPTION},
	{"CP_S_DRV_EXC",CP_S_DRV_EXC},
	{"CP_S_PAM_EXC", CP_S_PAM_EXC},
	{"CP_S_GUAS_EXC",CP_S_GUAS_EXC},
	{"CP_S_CTTF_EXC", CP_S_CTTF_EXC},
	{"CP_S_CAS_CPROC_EXC", CP_S_CAS_CPROC_EXC},
	{"CP_S_GUDSP_EXC",CP_S_GUDSP_EXC},
	{"CP_S_TLPS_EXC", CP_S_TLPS_EXC},
	{"CP_S_TLDSP_EXC",CP_S_TLDSP_EXC},
	{"CP_S_CPHY_EXC", CP_S_CPHY_EXC},
	{"CP_S_GUCNAS_EXC",CP_S_GUCNAS_EXC},
	{"CP_S_3RD_EXCEPTION", CP_S_3RD_EXCEPTION},
	{"LPM3_S_EXCEPTION", LPM3_S_EXCEPTION},
	{"SOCHIFI_S_EXCEPTION", SOCHIFI_S_EXCEPTION},
	{"HIFI_S_RESETFAIL", HIFI_S_RESETFAIL},
	{"ISP_S_EXCEPTION", ISP_S_EXCEPTION},
	{"IVP_S_EXCEPTION", IVP_S_EXCEPTION},
	{"IOM3_S_EXCEPTION", IOM3_S_EXCEPTION},
	{"TEE_S_EXCEPTION", TEE_S_EXCEPTION},
	{"MMC_S_EXCEPTION", MMC_S_EXCEPTION},
	{"CODECHIFI_S_EXCEPTION", CODECHIFI_S_EXCEPTION},
	{"IOM3_S_USER_EXCEPTION", IOM3_S_USER_EXCEPTION},
	{"HISEE_S_EXCEPTION", HISEE_S_EXCEPTION},
	{"reserved4", RESERVED4},
	{"AP_S_PMU", AP_S_PMU},
	{"AP_S_SMPL", AP_S_SMPL},
	{"AP_S_SCHARGER", AP_S_SCHARGER},
	{"VolUsbUpdate", BR_KEY_VOLUMN_DOWN_UP_UPDATE_USB},
	{"ForceSdUpdate", BR_KEY_VOLUMN_DOWN_UP_UPDATE_SD_FORCE},
	{"VolUpRecovery", BR_KEY_VOLUMN_UP},
	{"press1s", BR_KEY_POWERON_PRESS_1S},
	{"press10s", BR_KEY_POWERON_PRESS_10S},
	{"CheckpointRecovery", BR_CHECKPOINT_RECOVERY},
	{"CheckpointErecovery", BR_CHECKPOINT_ERECOVERY},
	{"CheckpointSdUpdate", BR_CHECKPOINT_SDUPDATE},
	{"CheckpointUsbUpdate", BR_CHECKPOINT_USBUPDATE},
	{"CheckpointResetFactory", BR_CHECKPOINT_RESETFACTORY},
	{"CheckpointHotaUpdate", BR_CHECKPOINT_HOTAUPDATE},
	{"PoweronNoBat", BR_POWERON_BY_USB_NO_BAT},
	{"nogui", BR_NOGUI},
	{"Factory", BR_FACTORY_VERSION},
	{"ResetHappen", BR_RESET_HAPPEN},
	{"OemRtc", BR_POWEROFF_ALARM},
	{"PoweroffCharging", BR_POWEROFF_CHARGE},
	{"SMPL", BR_POWERON_BY_SMPL},
	{"CheckpointUpdateDataimg", BR_CHECKPOINT_UPDATEDATAIMG},
	{"CPU_BUCK", BR_REBOOT_CPU_BUCK},
	{"XLOADER_S_DDRINIT_FAIL", XLOADER_S_DDRINIT_FAIL},
	{"XLOADER_S_EMMCINIT_FAIL", XLOADER_S_EMMCINIT_FAIL},
	{"XLOADER_S_LOAD_FAIL", XLOADER_S_LOAD_FAIL},
	{"XLOADER_S_VERIFY_FAIL", XLOADER_S_VERIFY_FAIL},
	{"XLOADER_S_WATCHDOG", XLOADER_S_WATCHDOG},
	{"FASTBOOT_EMMCINIT_FAIL", FASTBOOT_S_EMMCINIT_FAIL},
	{"FASTBOOT_S_PANIC", FASTBOOT_S_PANIC},
	{"FASTBOOT_S_WATCHDOG", FASTBOOT_S_WATCHDOG},
	{"AP_S_PRESS6S", AP_S_PRESS6S},
	{"FASTBOOT_S_OCV_VOL_ERR", FASTBOOT_S_OCV_VOL_ERR},
	{"FASTBOOT_S_BAT_TEMP_ERR", FASTBOOT_S_BAT_TEMP_ERR},
	{"FASTBOOT_S_MISC_ERR", FASTBOOT_S_MISC_ERR},
	{"FB_S_LD_DTIMG_ERR", FASTBOOT_S_LOAD_DTIMG_ERR},
	{"FB_S_LD_OTHR_IMGS_ERR", FASTBOOT_S_LOAD_OTHER_IMGS_ERR},
	{"FB_S_KERNEL_IMG_ERR", FASTBOOT_S_KERNEL_IMG_ERR},
	{"FASTBOOT_LOADLPMCU_FAIL", FASTBOOT_S_LOADLPMCU_FAIL},
	{"FASTBOOT_VERIFY_FAIL", FASTBOOT_S_IMG_VERIFY_FAIL},
	{"FASTBOOT_SOC_TEMP_HIGH", FASTBOOT_S_SOC_TEMP_ERR},
	{"FASTBOOT_FLASHCERT_FAIL", FASTBOOT_S_FLASH_PROTECTION_PANIC},
	{"BFM_S_NATIVE_BOOT_FAIL", BFM_S_BOOT_NATIVE_BOOT_FAIL},
	{"BFM_S_BOOT_TIMEOUT", BFM_S_BOOT_TIMEOUT},
	{"BFM_S_FW_BOOT_FAIL", BFM_S_BOOT_FRAMEWORK_BOOT_FAIL},
	{"BFM_S_NATIVE_DATA_FAIL", BFM_S_BOOT_NATIVE_DATA_FAIL},
};

struct blackbox_modid_list g_modid_list[] = {
	{HISI_BB_MOD_MODEM_DRV_START, HISI_BB_MOD_MODEM_DRV_END, "MODEM DRV"},
	{HISI_BB_MOD_MODEM_OSA_START, HISI_BB_MOD_MODEM_OSA_END, "MODEM OSA"},
	{HISI_BB_MOD_MODEM_OM_START, HISI_BB_MOD_MODEM_OM_END, "MODEM OM"},
	{HISI_BB_MOD_MODEM_GU_L2_START, HISI_BB_MOD_MODEM_GU_L2_END,
	 "MODEM GU L2"},
	{HISI_BB_MOD_MODEM_GU_WAS_START, HISI_BB_MOD_MODEM_GU_WAS_END,
	 "MODEM GU WAS"},
	{HISI_BB_MOD_MODEM_GU_GAS_START, HISI_BB_MOD_MODEM_GU_GAS_END,
	 "MODEM GU GAS"},
	{HISI_BB_MOD_MODEM_GU_NAS_START, HISI_BB_MOD_MODEM_GU_NAS_END,
	 "MODEM GU NAS"},
	{HISI_BB_MOD_MODEM_GU_DSP_START, HISI_BB_MOD_MODEM_GU_DSP_END,
	 "MODEM GU DSP"},
	{(unsigned int)HISI_BB_MOD_AP_START, (unsigned int)HISI_BB_MOD_AP_END, "ap"},
	{(unsigned int)HISI_BB_MOD_FASTBOOT_START, (unsigned int)HISI_BB_MOD_FASTBOOT_END, "fastboot"},
	{(unsigned int)HISI_BB_MOD_ISP_START, (unsigned int)HISI_BB_MOD_ISP_END, "isp"},
	{(unsigned int)HISI_BB_MOD_EMMC_START, (unsigned int)HISI_BB_MOD_EMMC_END, "emmc"},
	{(unsigned int)HISI_BB_MOD_CP_START, (unsigned int)HISI_BB_MOD_CP_END, "cp"},
	{(unsigned int)HISI_BB_MOD_TEE_START, (unsigned int)HISI_BB_MOD_TEE_END, "tee"},
	{(unsigned int)HISI_BB_MOD_HIFI_START, (unsigned int)HISI_BB_MOD_HIFI_END, "hifi"},
	{(unsigned int)HISI_BB_MOD_LPM_START, (unsigned int)HISI_BB_MOD_LPM_END, "lpm3"},
	{(unsigned int)HISI_BB_MOD_IOM_START, (unsigned int)HISI_BB_MOD_IOM_END, "iom3"},
	{(u32)HISI_BB_MOD_HISEE_START, (u32)HISI_BB_MOD_HISEE_END, "hisee"},
	{(unsigned int)HISI_BB_MOD_BFM_START, (unsigned int)HISI_BB_MOD_BFM_END, "bfm"},
	{(unsigned int)HISI_BB_MOD_RESERVED_START, (unsigned int)HISI_BB_MOD_RESERVED_END,
	 "blackbox reserved"},
	{(unsigned int)HISI_BB_MOD_MODEM_LPS_START, (unsigned int)HISI_BB_MOD_MODEM_LPS_END, "MODEM LPS"},
	{(unsigned int)HISI_BB_MOD_MODEM_LMSP_START, (unsigned int)HISI_BB_MOD_MODEM_LMSP_END,
	 "MODEM LMSP"},
	{(unsigned int)HISI_BB_MOD_RANDOM_ALLOCATED_START, (unsigned int)HISI_BB_MOD_RANDOM_ALLOCATED_END,
	 "blackbox random allocated"},
};

u32 get_reboot_reason_map_size(void)
{
	return (u32) (sizeof(reboot_reason_map) / sizeof(reboot_reason_map[0]));
}

/*
 * func name: rdr_get_exception_core
 * get exception core str for core id.
 * func args:
 *    u64 coreid
 * return value
 *	NULL  error
 *	!NULL core str.
 */
char *rdr_get_exception_type(u64 e_exce_type)
{
	int i;
	char *type = "UNDEF";

	for (i = 0; (unsigned int)i < get_reboot_reason_map_size(); i++) {
		if (reboot_reason_map[i].num == e_exce_type) {
			return (char *)reboot_reason_map[i].name;
		}
	}

	return type;
}

char *exception_core[RDR_CORE_MAX + 1] = {
	"AP",
	"CP",
	"TEEOS",
	"HIFI",
	"LPM3",
	"IOM3",
	"ISP",
	"IVP",
	"EMMC",
	"MODEMAP",
	"CLK",
	"REGULATOR",
	"BFM",
	"HISEE",
	"UNDEF",
};

/*
 * func name: rdr_get_exception_core
 * get exception core str for core id.
 * func args:
 *    u64 coreid
 * return value
 *	NULL  error
 *	!NULL core str.
 */
char *rdr_get_exception_core(u64 coreid)
{
	char *core = NULL;
	switch (coreid) {
	case RDR_AP:
		core = exception_core[0];
		break;
	case RDR_CP:
		core = exception_core[1];
		break;
	case RDR_TEEOS:
		core = exception_core[2];
		break;
	case RDR_HIFI:
		core = exception_core[3];
		break;
	case RDR_LPM3:
		core = exception_core[4];
		break;
	case RDR_IOM3:
		core = exception_core[5];
		break;
	case RDR_ISP:
		core = exception_core[6];
		break;
	case RDR_IVP:
		core = exception_core[7];
		break;
	case RDR_EMMC:
		core = exception_core[8];
		break;
	case RDR_MODEMAP:
		core = exception_core[9];
		break;
	case RDR_CLK:
		core = exception_core[10];
		break;
	case RDR_REGULATOR:
		core = exception_core[11];
		break;
	case RDR_BFM:
		core = exception_core[12];
		break;
	case RDR_HISEE:
		core = exception_core[13];
		break;
	default:
		core = exception_core[14];
		break;
	}
	return core;
}

atomic_t volatile bb_in_suspend = ATOMIC_INIT(0);
atomic_t volatile bb_in_reboot = ATOMIC_INIT(0);
atomic_t volatile bb_in_saving = ATOMIC_INIT(0);

int rdr_get_suspend_state(void)
{
	return atomic_read(&bb_in_suspend);
}

int rdr_get_reboot_state(void)
{
	return atomic_read(&bb_in_reboot);
}

int rdr_set_saving_state(int state)
{
	return atomic_set(&bb_in_saving, state);/*lint !e1058 !e446 */
}

static struct notifier_block bb_suspend_notifier;
static int bb_suspend_nb(struct notifier_block *this,
			 unsigned long event, void *ptr)
{
	switch (event) {
	case PM_POST_HIBERNATION:
	case PM_POST_SUSPEND:
		BB_PRINT_DBG("%s: resume +\n", __func__);
		atomic_set(&bb_in_suspend, 0);/*lint !e1058 !e446 */
		BB_PRINT_DBG("%s: resume -\n", __func__);
		break;

	case PM_HIBERNATION_PREPARE:
	case PM_SUSPEND_PREPARE:
		BB_PRINT_DBG("%s: suspend +\n", __func__);
		atomic_set(&bb_in_suspend, 1);/*lint !e1058 !e446 */
		while (1) {
			if (atomic_read(&bb_in_saving))
				msleep(1000);
			else
				break;
		}
		BB_PRINT_DBG("%s: suspend -\n", __func__);
		break;
	default:
		return NOTIFY_DONE;
	}
	return NOTIFY_OK;
}

static int bb_reboot_nb(struct notifier_block *nb, unsigned long foo, void *bar)
{
	int i = 10;
	/* prevent access the emmc now: */
	BB_PRINT_DBG("%s: shutdown +\n", __func__);
	atomic_set(&bb_in_reboot, 1);/*lint !e1058 !e446 */
	while (i--) {
		if (atomic_read(&bb_in_saving))
			msleep(1000);
		else
			break;
		BB_PRINT_DBG("rdr:is saving rdr, wait 1s ...\n");
	}
	rdr_field_reboot_done();
	BB_PRINT_DBG("%s: shutdown -\n", __func__);

	return 0;
}

static u64 RESERVED_RDR_PHYMEM_ADDR;
static u64 RESERVED_RDR_PHYMEM_SIZE;
static RDR_NVE g_nve;
static u64 g_max_logsize;
static u32 g_wait_dumplog_timeout;
static u32 g_max_reboot_times;

u32 rdr_get_reboot_times(void)
{
	return g_max_reboot_times;
}

u64 rdr_reserved_phymem_addr(void)
{
	return RESERVED_RDR_PHYMEM_ADDR;
}

u64 rdr_reserved_phymem_size(void)
{
	return RESERVED_RDR_PHYMEM_SIZE;
}

int rdr_get_dumplog_timeout(void)
{
	return g_wait_dumplog_timeout;
}

u64 rdr_get_logsize(void)
{
	return g_max_logsize;
}

RDR_NVE rdr_get_nve(void)
{
	return g_nve;
}

char *blackbox_get_modid_str(u32 modid)
{
	u32 i;
	u32 modid_size = sizeof(g_modid_list) / sizeof(g_modid_list[0]);
	for (i = 0; i < modid_size; ++i) {
		if (modid >= g_modid_list[i].modid_span_little
		    && modid <= g_modid_list[i].modid_span_big) {
			return g_modid_list[i].modid_str;
		}
	}

	return "error,modid not found";

}

#include <linux/of.h>
#include <linux/of_address.h>
int rdr_common_early_init(void)
{
	int i, ret, len;
	struct device_node *np = NULL;
	struct resource res;
	const char *prdr_dumpctrl;
	u32 value, j;
	u32 data[RDR_CORE_MAX];
	struct device_node *bbox_addr;

	np = of_find_compatible_node(NULL, NULL, "hisilicon,rdr");
	if (!np) {
		BB_PRINT_ERR("[%s], find rdr_memory node fail!\n", __func__);
		return -ENODEV;
	}

	bbox_addr = of_parse_phandle(np, "bbox_addr", 0);
	if (!bbox_addr) {
		BB_PRINT_ERR("[%s], no bbox_addr phandle\n", __func__);
		return -ENODEV;
	}

	ret = of_address_to_resource(bbox_addr, 0, &res);
	of_node_put(bbox_addr);
	if (ret) {
		BB_PRINT_ERR("failed to translate bbox_addr to resource: %d\n", ret);
		return ret;
	}

	RESERVED_RDR_PHYMEM_ADDR = res.start;
	RESERVED_RDR_PHYMEM_SIZE = resource_size(&res);

	BB_PRINT_DBG("[%s], get phymem addr [%llx] size [%llx]in dts!\n",
		     __func__, RESERVED_RDR_PHYMEM_ADDR,
		     RESERVED_RDR_PHYMEM_SIZE);

	ret = of_property_read_string(np, "rdr-dumpctl", &prdr_dumpctrl);
	if (ret < 0 || NULL == prdr_dumpctrl
	    || strlen(prdr_dumpctrl) > 16) {
		BB_PRINT_ERR("[%s], find rdr-dumpctl node fail! [%s]\n",
			     __func__, prdr_dumpctrl);
		return ret;
	}
	BB_PRINT_DBG("[%s], get prdr_dumpctrl [%s] in dts!\n", __func__,
		     prdr_dumpctrl);
	g_nve = 0;
	len = strlen(prdr_dumpctrl);
	for (i = --len; i >= 0; i--) {
		if (prdr_dumpctrl[i] == '1')
			g_nve |= (u64)1 << (len - i);
	}
	BB_PRINT_DBG("[%s], get nve [0x%llx] in dts!\n", __func__, g_nve);
	ret = of_property_read_u32(np, "rdr-log-max-size", &value);
	if (ret) {
		BB_PRINT_ERR("[%s], cannot find rdr-log-max-size in dts!\n",
			     __func__);
		return ret;
	}
	g_max_logsize = value;
	BB_PRINT_DBG("[%s], get rdr-log-max-size [0x%llx] in dts!\n", __func__,
		     g_max_logsize);

	ret = of_property_read_u32(np, "wait-dumplog-timeout", &value);
	if (ret) {
		BB_PRINT_ERR("[%s], cannot find wait-dumplog-timeout in dts!\n",
			     __func__);
		return ret;
	}
	g_wait_dumplog_timeout = value;
	BB_PRINT_DBG("[%s], get g_wait_dumplog_timeout [0x%x] in dts!\n",
		     __func__, g_wait_dumplog_timeout);

	ret = of_property_read_u32(np, "rdr_area_num", &value);
	if (ret) {
		BB_PRINT_ERR("[%s], cannot find rdr_area_num in dts!\n",
			     __func__);
		return ret;
	}
	BB_PRINT_DBG("[%s], get rdr_area_num [0x%x] in dts!\n", __func__,
		     value);

	if (value > RDR_CORE_MAX) {
		BB_PRINT_ERR("[%s], invaild core num in dts!\n", __func__);
		return -1;
	}
	ret = of_property_read_u32_array(np, "rdr_area_sizes", &data[0], value);
	if (ret) {
		BB_PRINT_ERR("[%s], cannot find rdr_area_sizes in dts!\n",
			     __func__);
		return ret;
	}
	rdr_set_area_info(0, value);
	for (j = 1; j < value; j++) {
		rdr_set_area_info(j, data[j]);
		BB_PRINT_DBG("[%s], get rdr_area_num[%u]:[0x%x] in dts!\n",
			     __func__, j, data[j]);
	}

	ret = of_property_read_u32(np, "unexpected-max-reboot-times", &value);
	if (ret) {
		BB_PRINT_ERR("[%s], cannot find unexpected-max-reboot-times in dts!\n",
			     __func__);
		return ret;
	}
	g_max_reboot_times = value;
	BB_PRINT_DBG("[%s], unexpected-max-reboot-times [0x%x] in dts!\n", __func__,
		     value);

	return ret;
}

static struct notifier_block bb_reboot_notifier;
int rdr_common_init(void)
{

	/* Register to get PM events */
	bb_suspend_notifier.notifier_call = bb_suspend_nb;
	bb_suspend_notifier.priority = -1;
	if (register_pm_notifier(&bb_suspend_notifier)) {
		BB_PRINT_ERR("%s: Failed to register for PM events\n",
			     __func__);
		return -1;
	}

	bb_reboot_notifier.notifier_call = bb_reboot_nb;
	bb_reboot_notifier.priority = -1;
	if (register_reboot_notifier(&bb_reboot_notifier)) {
		BB_PRINT_ERR("%s: Failed to register for Reboot events\n",
			     __func__);
		return -1;
	}
	return 0;

}

static char g_reboot_reason[RDR_REBOOT_REASON_LEN] = "undef";
static u32 g_reboot_type;
char *rdr_get_reboot_reason(void)
{
	return g_reboot_reason;
}

u32 rdr_get_reboot_type(void)
{
	return g_reboot_type;
}

static int __init early_parse_reboot_reason_cmdline(char *reboot_reason_cmdline)
{
	int i;
	printk("[%s]\n", __func__);
	memset(g_reboot_reason, 0x0, RDR_REBOOT_REASON_LEN);
	memcpy(g_reboot_reason, reboot_reason_cmdline, RDR_REBOOT_REASON_LEN - 1);

	for (i = 0; (u32)i < get_reboot_reason_map_size(); i++) {
		if (!strncmp((char *)reboot_reason_map[i].name, g_reboot_reason, RDR_REBOOT_REASON_LEN)) {
			g_reboot_type = reboot_reason_map[i].num;
			break;
		}
	}
	printk("[%s][%s][%d]\n", __func__, g_reboot_reason, g_reboot_type);
	return 0;
}

early_param("reboot_reason", early_parse_reboot_reason_cmdline);

void *bbox_vmap(phys_addr_t paddr, size_t size)
{
	int i;
	void *vaddr = NULL;
	pgprot_t pgprot;
	unsigned long offset;
	int pages_count;
	struct page **pages;

	offset = paddr & ~PAGE_MASK;
	paddr &= PAGE_MASK;
	pages_count = PAGE_ALIGN(size + offset) / PAGE_SIZE;

	pages = kzalloc(sizeof(struct page *) * pages_count, GFP_KERNEL);
	if (!pages)
		return NULL;

	pgprot = pgprot_writecombine(PAGE_KERNEL);

	for (i = 0; i < pages_count; i++) {
		*(pages + i) = phys_to_page(paddr + PAGE_SIZE * i);
	}

	vaddr = vmap(pages, pages_count, VM_MAP, pgprot);
	kfree(pages);
	if (NULL == vaddr)
		return NULL;

	return offset + (char *)vaddr;
}

void *hisi_bbox_map(phys_addr_t paddr, size_t size)
{
	void *vaddr = NULL;

	if (paddr < RESERVED_RDR_PHYMEM_ADDR || !size ||
	    (paddr + size) >
	    (RESERVED_RDR_PHYMEM_ADDR + RESERVED_RDR_PHYMEM_SIZE)) {
		BB_PRINT_ERR("Error BBox memory.\n");
		return NULL;
	}

	if (pfn_valid(RESERVED_RDR_PHYMEM_ADDR >> PAGE_SHIFT)) {
		vaddr = bbox_vmap(paddr, size);
	} else {
		vaddr = ioremap_wc(paddr, size);
	}

	return vaddr;
}

void hisi_bbox_unmap(const void *vaddr)
{
	if (pfn_valid(RESERVED_RDR_PHYMEM_ADDR >> PAGE_SHIFT)) {
		vunmap(vaddr);
	} else {
		iounmap((void __iomem *)vaddr);
	}
}

/*******************************************************************************
Function:       bbox_save_done
Description:	每次异常对应的log目录保存完毕后，需要调用此函数表示这个目录已经
		记录完毕了，logserver可以将其打包上传了。
Input:	logpath:异常对应的保存log的目录；
		step:异常log保存到了哪一步，是否还需要继续使用的标记；
Output:		    NA
Return:		    NA
Other:          used by rdr_core.c、rdr_hisi_ap_adapter.c
********************************************************************************/
void bbox_save_done(char *logpath, u32 step)
{
	int fd = 0, ret = 0;
	char path[PATH_MAXLEN];

	BB_PRINT_START();
	if (!logpath
	    || (PATH_MAXLEN <
		strlen(logpath) + strlen(BBOX_SAVE_DONE_FILENAME) + 1)) {
		BB_PRINT_ERR("logpath is invalid.\n");
		return;
	}

	BB_PRINT_ERR("logpath is [%s], step is [%d]\n", logpath, step);
	if (BBOX_SAVE_STEP_DONE == step) {

		/*组合done文件的绝对路径，作为sys_mkdir的参数 */
		memset(path, 0, PATH_MAXLEN);
		memcpy(path, logpath, strlen(logpath));
		strncat(path, BBOX_SAVE_DONE_FILENAME,
			((PATH_MAXLEN - 1) - strlen(path)));


		/*在时间戳目录下面创建done文件 */
		fd = sys_open(path, O_CREAT | O_WRONLY, FILE_LIMIT);
		if (fd < 0) {
			BB_PRINT_ERR("sys_mkdir [%s] error, fd is [%d]\n", path,
				     fd);
			return;
		}
		sys_close(fd);

		/*根据权限要求，hisi_logs目录及子目录群组调整为root-system */
		ret =
		    (int)bbox_chown((const char __user *)path, ROOT_UID,
				    SYSTEM_GID, false);
		if (ret) {
			BB_PRINT_ERR
			    ("chown %s uid [%d] gid [%d] failed err [%d]!\n",
			     PATH_ROOT, ROOT_UID, SYSTEM_GID, ret);
		}
	}

	BB_PRINT_END();
}

/*******************************************************************************
Function:		rdr_record_reboot_times2mem
Description:	save reboot times to specified memory.
Input:		NA
Output:		NA
Return:		NA
Other:		NA
********************************************************************************/
void rdr_record_reboot_times2mem(void)
{
	struct rdr_struct_s *pbb;
	BB_PRINT_START();
	pbb = rdr_get_pbb();
	pbb->top_head.reserve = RDR_UNEXPECTED_REBOOT_MARK_ADDR;
	BB_PRINT_END();
}

/*******************************************************************************
Function:		rdr_reset_reboot_times
Description:	reset the file saving reboot times.
Input:		NA
Output:		NA
Return:		NA
Other:		NA
********************************************************************************/
void rdr_reset_reboot_times(void)
{
	struct file *fp;
	ssize_t length;
	char buf;

	BB_PRINT_START();
	fp = filp_open(RDR_REBOOT_TIMES_FILE, O_CREAT | O_RDWR, 0660);
	if (IS_ERR(fp))	{
		BB_PRINT_ERR("rdr:%s(),open %s fail\n", __func__,
			    RDR_REBOOT_TIMES_FILE);
		return;
	}
	buf = 0;
	vfs_llseek(fp, 0L, SEEK_SET);
	length = vfs_write(fp, &buf, sizeof(buf), &(fp->f_pos));/*lint !e613 */
	if (length == sizeof(buf)) {
		vfs_fsync(fp, 0);
	}

	filp_close(fp, NULL);/*lint !e668 */
	BB_PRINT_END();
}

/*******************************************************************************
Function:		rdr_record_erecovery_reason
Description:	write the enter reason of erecovery to /cache/recovery/last_erecovery_entry.
Input:		NA
Output:		NA
Return:		NA
Other:		NA
********************************************************************************/
void rdr_record_erecovery_reason(void)
{
	struct file *fp;
	ssize_t length;
	char *e_reason  = "erecovery_enter_reason:=2015\n";
	BB_PRINT_START();

	fp = filp_open(RDR_ERECOVERY_REASON_FILE, O_CREAT | O_RDWR, 0660);
	if (IS_ERR(fp)) {
		BB_PRINT_ERR("rdr:%s(),open %s first fail,error No. %pK\n", __func__, RDR_ERECOVERY_REASON_FILE, fp);
		rdr_create_dir("/cache/recovery/");
		fp = filp_open(RDR_ERECOVERY_REASON_FILE, O_CREAT | O_RDWR | O_TRUNC, 0660);
		if (IS_ERR(fp)) {
			BB_PRINT_ERR("rdr:%s(),open %s second fail,error No. %pK\n", __func__, RDR_ERECOVERY_REASON_FILE, fp);
			return;
		}
	}
	vfs_llseek(fp, 0L, SEEK_SET);
	length = vfs_write(fp, e_reason, strlen(e_reason) + 1, &(fp->f_pos));/* lint !e613 */
	if (length == (strlen(e_reason) + 1)) {
		vfs_fsync(fp, 0);
	}

	filp_close(fp, NULL);/*lint !e668 */
	BB_PRINT_END();
}

/*******************************************************************************
Function:		rdr_record_reboot_times2file
Description:	record the reboot times to file.
Input:		NA
Output:		NA
Return:		int: reboot times.
Other:		NA
********************************************************************************/
int rdr_record_reboot_times2file(void)
{
	struct file *fp;
	ssize_t length;
	char buf  = 0;

	BB_PRINT_START();
	fp = filp_open(RDR_REBOOT_TIMES_FILE, O_CREAT | O_RDWR, 0660);
	if (IS_ERR(fp)) {
		BB_PRINT_ERR("rdr:%s(),open %s fail\n", __func__,
			   RDR_REBOOT_TIMES_FILE);
		return 0;
	}

	vfs_llseek(fp, 0L, SEEK_SET);
	length = vfs_read(fp, &buf, sizeof(buf), &fp->f_pos);/*lint !e613 */
	if (length == 0 || buf == 0) {
		buf = 0;
	}
	buf++;

	vfs_llseek(fp, 0L, SEEK_SET);
	length = vfs_write(fp, &buf, sizeof(buf), &(fp->f_pos));/*lint !e613 */
	if (length == sizeof(buf)) {
		vfs_fsync(fp, 0);
	}

	filp_close(fp, NULL);/*lint !e668 */
	BB_PRINT_END();
	return buf;
}
