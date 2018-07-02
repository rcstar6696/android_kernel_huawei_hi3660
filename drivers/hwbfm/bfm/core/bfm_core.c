/**
    @copyright: Huawei Technologies Co., Ltd. 2016-xxxx. All rights reserved.

    @file: bfm_core.c

    @brief: define the core's external public enum/macros/interface for BFM (Boot Fail Monitor)

    @version: 2.0

    @author: QiDechun ID: 216641

    @date: 2016-08-17

    @history:
*/

/*----includes-----------------------------------------------------------------------*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/moduleparam.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/time.h>
#include <linux/kthread.h>
#include <linux/vmalloc.h>
#include <linux/reboot.h>
#include <linux/delay.h>
#include <linux/dirent.h>
#include <linux/statfs.h>
#include <chipset_common/bfmr/bfm/core/bfm_core.h>
#include <chipset_common/bfmr/bfm/chipsets/bfm_chipsets.h>
#include <chipset_common/bfmr/bfm/core/bfm_timer.h>
#include <chipset_common/bfmr/bfr/core/bfr_core.h>


/*----local macroes------------------------------------------------------------------*/

#define bfmr_llseek no_llseek
#define BFM_BFI_FILE_NAME "bootFail_info.txt"
#define BFM_RECOVERY_FILE_NAME "recovery_info.txt"
#define BFM_RECOVERY_METHOD_FIELD "rcvMethod:"
#define BFM_RECOVERY_SUCCESS_FIELD "rcvResult:1\r\n"
#define BFM_RECOVERY_RESULT_FIELD "rcvResult:"
#define BFM_LOG_END_TAG_PER_LINE "\r\n"
#define BFM_BYTES_REDUNDANT_ON_LOG_PART (4 * 1024)
#define BFM_DONE_FILE_NAME "DONE"
#define BFM_BFI_FILE_CONTENT_FORMAT \
    "time:%s\r\n" \
    "bootFailErrno:%s_%s_%s\r\n" \
    "boot_stage:%s_%s\r\n" \
    "isSystemRooted:%d\r\n" \
    "isUserPerceptible:%d\r\n" \
    "SpaceLeftOnData:%lldMB [%lld%%]\r\n" \
    "iNodesUsedOnData:%lld [%lld%%]\r\n" \
    "\r\n" \
    "time:0x%llx\r\n" \
    "bootFailErrno:0x%x\r\n" \
    "boot_stage:0x%x\r\n" \
    "isSystemRooted:%d\r\n" \
    "isUserPerceptible:%d\r\n"\
    "dmdErrNo:%d\r\n"\
    "bootup_time:%dS\r\n" \
    "\r\n"\
    "the bootlock field in cmdline is: [%s] this time\r\n"

#define BFM_RCV_FILE_CONTENT_FORMAT \
    "rcvMethod:%s\r\n" \
    "rcvResult:%s\r\n" \
    "\r\n" \
    "rcvMethod:%d\r\n" \
    "rcvResult:%d\r\n"

#define BFM_RECOVERY_SUCCESS_STR "success"
#define BFM_RECOVERY_SUCCESS_INT_VALUE 1
#define BFM_RECOVERY_FAIL_STR "fail"
#define BFM_RECOVERY_FAIL_INT_VALUE 0
#define BFM_PBL_LOG_MAX_LEN (BFMR_SIZE_1K)
#define BFM_BOOLTOADER1_LOG_MAX_LEN (BFMR_SIZE_1K)
#define BFM_BOOLTOADER2_LOG_MAX_LEN (128 * BFMR_SIZE_1K)
#define BFM_KMSG_LOG_MAX_LEN (BFMR_SIZE_1K * BFMR_SIZE_1K)
#define BFM_KMSG_TEXT_LOG_MAX_LEN (512 * BFMR_SIZE_1K)
#define BFM_LOG_RAMOOPS_LOG_MAX_LEN (128 * BFMR_SIZE_1K)
#define BFM_APP_BETA_LOGCAT_LOG_MAX_LEN (1024 * BFMR_SIZE_1K)
#define BFM_CRITICAL_PROCESS_CRASH_LOG_MAX_LEN (128 * BFMR_SIZE_1K)
#define BFM_VM_TOMBSTONES_LOG_MAX_LEN (256 * BFMR_SIZE_1K)
#define BFM_VM_CRASH_LOG_MAX_LEN (128 * BFMR_SIZE_1K)
#define BFM_VM_WATCHDOG_LOG_MAX_LEN (256 * BFMR_SIZE_1K)
#define BFM_NORMAL_FRAMEWORK_BOOTFAIL_LOG_MAX_LEN (256 * BFMR_SIZE_1K)
#define BFM_FIXED_FRAMEWORK_BOOTFAIL_LOG_MAX_LEN (256 * BFMR_SIZE_1K)
#define BFM_BOOTFAIL_INFO_LOG_MAX_LEN (BFMR_SIZE_1K)
#define BFM_RECOVERY_INFO_LOG_MAX_LEN (BFMR_SIZE_1K)
#define BFM_BASIC_LOG_MAX_LEN (BFM_BOOTFAIL_INFO_LOG_MAX_LEN + BFM_RECOVERY_INFO_LOG_MAX_LEN)
#define BFM_BFMR_TEMP_BUF_LOG_MAX_LEN (BFM_BOOLTOADER1_LOG_MAX_LEN + BFM_BOOLTOADER2_LOG_MAX_LEN\
    + BFM_KMSG_LOG_MAX_LEN + BFM_LOG_RAMOOPS_LOG_MAX_LEN)

#define BFM_TOMBSTONE_FILE_NAME_TAG "tombstone"
#define BFM_SYSTEM_SERVER_CRASH_FILE_NAME_TAG "system_server_crash"
#define BFM_SYSTEM_SERVER_WATCHDOG_FILE_NAME_TAG "system_server_watchdog"
#define BFM_SAVE_LOG_INTERVAL_FOR_EACH_LOG (1000)
#define BFM_US_PERSECOND (1000000)
#define BFM_SAVE_LOG_MAX_TIME (6000)

#define BFMR_RECOVERT_MODE_KEYWORD_1 "rebooting into recovery mode"
#define BFMR_RECOVERT_MODE_KEYWORD_2 "exited 4 times in 4 minutes"
#define BFM_WAIT_DATA_PART_TIME_OUT (5)
#define BFM_BOOTUP_SLOWLY_THRESHOLD_IN_SECOND (5 * 60)


/*----local prototypes----------------------------------------------------------------*/

typedef struct
{
    bfmr_log_type_e log_type;
    unsigned int buf_len;
    char *buf;
} bfm_log_type_buffer_info_t;

typedef struct
{
    bfmr_bootfail_errno_e bootfail_errno;
    char *desc;
} bfm_boot_fail_no_desc_t;

typedef struct
{
    bfmr_boot_stage_e boot_stage;
    long long log_size_allowed;
} bfm_log_size_param_t;


/*----global variables-----------------------------------------------------------------*/


/*----global function prototypes--------------------------------------------------------*/


/*----local function prototypes---------------------------------------------------------*/

static int __init early_parse_bfmr_enable_flag(char *p);
static long bfmr_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int bfmr_open(struct inode *inode, struct file *file);
static ssize_t bfmr_read(struct file *file, char __user *buf, size_t count, loff_t *pos);
static ssize_t bfmr_write(struct file *file, const char *data, size_t len, loff_t *ppos);
static int bfmr_release(struct inode *inode, struct file *file);
static int bfm_update_info_for_each_log(void);
static int bfm_update_recovery_result(char *precovery_info_file_path,
    int real_recovery_method,
    bool boot_success);
static int bfm_notify_boot_success(void *param);
static int bfm_lookup_dir_by_create_time(const char *root,
    char *log_path,
    size_t log_path_len,
    int find_oldest_log);
static int bfm_read_recovery_info(const char *precovery_info_file_path,
    char *data_buf, long data_buf_len);
static int bfm_find_newest_log(char *log_path, size_t log_path_len);
static void bfm_wait_for_compeletion_of_processing_boot_fail(void);

static void bfm_delete_oldest_log(long long bytes_need_this_time);
static long long bfm_get_basic_space_for_each_bootfail_log(bfmr_bootfail_errno_e bootfail_errno);
static long long bfm_get_extra_space_for_each_bootfail_log(bfm_process_bootfail_param_t *pparam);
static int bfm_save_extra_bootfail_logs(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam);
static int bfm_capture_and_save_bl1_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam);
static int bfm_capture_and_save_bl2_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam);
static int bfm_capture_and_save_kernel_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam);
static int bfm_capture_and_save_ramoops_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam);
static int bfm_capture_and_save_native_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam);
static int bfm_capture_and_save_framework_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam);
static bool bfm_is_log_existed(unsigned long long rtc_time, unsigned int bootfaiL_errno);
static int bfm_capture_and_save_bootfail_log(bfm_process_bootfail_param_t *pparam);
static void bfm_process_after_save_bootfail_log(void);
static char* bfm_get_boot_stage_name(unsigned int boot_stage);
static char* bfm_get_boot_fail_no_desc(bfmr_bootfail_errno_e bootfail_errno, bfm_process_bootfail_param_t *pparam);
static int bfm_save_bootfail_info_txt(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam);
static int bfm_save_recovery_info_txt(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam);
static char *bfm_get_file_name(char *file_full_path);
static int bfm_process_upper_layer_boot_fail(void *param);
static int bfm_process_bottom_layer_boot_fail(void *param);
static int bfmr_capture_and_save_bottom_layer_boot_fail_log(void);
/**
    @function: static int bfmr_capture_and_save_log(bfmr_log_type_e type, bfmr_log_dst_t *dst)
    @brief: capture and save log, initialised in core.

    @param: src [in] log src info.
    @param: dst [in] infomation about the media of saving log.

    @return: 0 - succeeded; -1 - failed.

    @note: it need be initialized in bootloader and kernel.
*/
static int bfmr_capture_and_save_log(bfmr_log_src_t *src, bfmr_log_dst_t *dst, bfm_process_bootfail_param_t *pparam);
static unsigned long long bfm_get_system_time(void);
static void bfmr_get_critical_process_name(const char *kmsg, size_t kmsg_size, char *pbuf, size_t buf_size);
static int bfm_get_fs_state(const char *pmount_point, struct statfs *pstatfsbuf);


/*----local variables-----------------------------------------------------------------*/

static const struct file_operations s_bfmr_fops = {
    .owner	 = THIS_MODULE,
    .unlocked_ioctl = bfmr_ioctl,
    .open = bfmr_open,
    .read = bfmr_read,
    .write = bfmr_write,
    .release = bfmr_release,
    .llseek = bfmr_llseek,
};

static struct miscdevice s_bfmr_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = BFMR_DEV_NAME,
    .fops = &s_bfmr_fops,
};


static DEFINE_MUTEX(s_process_boot_stage_mutex);
static DEFINE_MUTEX(s_process_boot_fail_mutex);
static DECLARE_COMPLETION(s_process_boot_fail_comp);

/* modifiy this array if you add new log type */
static bfm_log_type_buffer_info_t s_log_type_buffer_info[LOG_TYPE_MAX_COUNT] = {
    {LOG_TYPE_BOOTLOADER_1, BFM_BOOLTOADER1_LOG_MAX_LEN, NULL},
    {LOG_TYPE_BOOTLOADER_2, BFM_BOOLTOADER2_LOG_MAX_LEN, NULL},
    {LOG_TYPE_BFMR_TEMP_BUF, BFM_BFMR_TEMP_BUF_LOG_MAX_LEN, NULL},
    {LOG_TYPE_ANDROID_KMSG, BFM_KMSG_LOG_MAX_LEN, NULL},
    {LOG_TYPE_RAMOOPS, BFM_LOG_RAMOOPS_LOG_MAX_LEN, NULL},
    {LOG_TYPE_BETA_APP_LOGCAT, BFM_APP_BETA_LOGCAT_LOG_MAX_LEN, NULL},
    {LOG_TYPE_CRITICAL_PROCESS_CRASH, BFM_CRITICAL_PROCESS_CRASH_LOG_MAX_LEN, NULL},
    {LOG_TYPE_VM_TOMBSTONES, BFM_VM_TOMBSTONES_LOG_MAX_LEN, NULL},
    {LOG_TYPE_VM_CRASH, BFM_VM_CRASH_LOG_MAX_LEN, NULL},
    {LOG_TYPE_VM_WATCHDOG, BFM_VM_WATCHDOG_LOG_MAX_LEN, NULL},
    {LOG_TYPE_NORMAL_FRAMEWORK_BOOTFAIL_LOG, BFM_NORMAL_FRAMEWORK_BOOTFAIL_LOG_MAX_LEN, NULL},
    {LOG_TYPE_FIXED_FRAMEWORK_BOOTFAIL_LOG, BFM_FIXED_FRAMEWORK_BOOTFAIL_LOG_MAX_LEN, NULL},
    {LOG_TYPE_TEXT_KMSG, BFM_KMSG_TEXT_LOG_MAX_LEN, NULL},
};

/* modifiy this array according to the log count captured for each bootfail */
static bfm_log_size_param_t s_log_size_param[] = {
    {(PBL_STAGE << 24), BFM_PBL_LOG_MAX_LEN + BFM_BASIC_LOG_MAX_LEN},
    {(BL1_STAGE << 24), BFM_BOOLTOADER1_LOG_MAX_LEN + BFM_BASIC_LOG_MAX_LEN},
    {(BL2_STAGE << 24), BFM_BOOLTOADER1_LOG_MAX_LEN + BFM_BOOLTOADER2_LOG_MAX_LEN + BFM_BASIC_LOG_MAX_LEN},
    {(KERNEL_STAGE << 24), BFM_BOOLTOADER1_LOG_MAX_LEN + BFM_BOOLTOADER2_LOG_MAX_LEN + BFM_KMSG_LOG_MAX_LEN
        + BFM_LOG_RAMOOPS_LOG_MAX_LEN + BFM_BASIC_LOG_MAX_LEN},
    {(NATIVE_STAGE << 24), BFM_BOOLTOADER1_LOG_MAX_LEN + BFM_BOOLTOADER2_LOG_MAX_LEN + BFM_KMSG_LOG_MAX_LEN
        + BFM_LOG_RAMOOPS_LOG_MAX_LEN + BFM_VM_TOMBSTONES_LOG_MAX_LEN + BFM_BASIC_LOG_MAX_LEN},
    {(ANDROID_FRAMEWORK_STAGE << 24), BFM_BOOLTOADER1_LOG_MAX_LEN + BFM_BOOLTOADER2_LOG_MAX_LEN
        + BFM_KMSG_LOG_MAX_LEN + BFM_LOG_RAMOOPS_LOG_MAX_LEN + BFM_VM_WATCHDOG_LOG_MAX_LEN + BFM_BASIC_LOG_MAX_LEN},
};

/* modifiy this array if you add new boot fail no */
static bfm_boot_fail_no_desc_t s_bootfail_errno_desc[] =
{
    {BL1_DDR_INIT_FAIL, "bl1 ddr init failed"},
    {BL1_EMMC_INIT_FAIL, "bl1 emmc init failed"},
    {BL1_BL2_LOAD_FAIL, "load image image failed"},
    {BL1_BL2_VERIFY_FAIL, "verify image failed"},
    {BL2_EMMC_INIT_FAIL, "bl2 emmc init failed"},
    {BL1_WDT, "bl1 wdt"},
    {BL1_VRL_LOAD_FAIL, "bl1 vrl load error"},
    {BL1_VRL_VERIFY_FAIL, "bl1 vrl verify image error"},
    {BL1_ERROR_GROUP_BOOT, "bl1 group boot error"},
    {BL1_ERROR_GROUP_BUSES, "bl1 group buses error"},
    {BL1_ERROR_GROUP_BAM, "bl1 group bam"},
    {BL1_ERROR_GROUP_BUSYWAIT, "bl1 group busy wait"},
    {BL1_ERROR_GROUP_CLK, "bl1 group clock"},
    {BL1_ERROR_GROUP_CRYPTO, "bl1 group crypto"},
    {BL1_ERROR_GROUP_DAL, "bl1 group dal"},
    {BL1_ERROR_GROUP_DEVPROG, "bl1 group devprog"},
    {BL1_ERROR_GROUP_DEVPROG_DDR, "bl1 group devprog ddr"},
    {BL1_ERROR_GROUP_EFS, "bl1 group efs"},
    {BL1_ERROR_GROUP_EFS_LITE, "bl1 group efs lite"},
    {BL1_ERROR_GROUP_HOTPLUG, "bl1 group hot-plug"},
    {BL1_ERROR_GROUP_HSUSB, "bl1 group high speed usb"},
    {BL1_ERROR_GROUP_ICB, "bl1 group icb"},
    {BL1_ERROR_GROUP_LIMITS, "bl1 group limits"},
    {BL1_ERROR_GROUP_MHI, "bl1 group mhi"},
    {BL1_ERROR_GROUP_PCIE, "bl1 group pcie"},
    {BL1_ERROR_GROUP_PLATFOM, "bl1 group platform"},
    {BL1_ERROR_GROUP_PMIC, "bl1 group pmic"},
    {BL1_ERROR_GROUP_POWER, "bl1 group power"},
    {BL1_ERROR_GROUP_PRNG, "bl1 group prng"},
    {BL1_ERROR_GROUP_QUSB, "bl1 group qusb"},
    {BL1_ERROR_GROUP_SECIMG, "bl1 group secimg"},
    {BL1_ERROR_GROUP_SECBOOT, "bl1 group secboot"},
    {BL1_ERROR_GROUP_SECCFG, "bl1 group seccfg"},
    {BL1_ERROR_GROUP_SMEM, "bl1 group smem"},
    {BL1_ERROR_GROUP_SPMI, "bl1 group spmi"},
    {BL1_ERROR_GROUP_SUBSYS, "bl1 group subsystem"},
    {BL1_ERROR_GROUP_TLMM, "bl1 group tlmm"},
    {BL1_ERROR_GROUP_TSENS, "bl1 group tsensor"},
    {BL1_ERROR_GROUP_HWENGINES, "bl1 group hwengines"},
    {BL1_ERROR_GROUP_IMAGE_VERSION, "bl1 group image version"},
    {BL1_ERROR_GROUP_SECURITY, "bl1 group system security"},
    {BL1_ERROR_GROUP_STORAGE, "bl1 group storage"},
    {BL1_ERROR_GROUP_SYSTEMDRIVERS, "bl1 group system drivers"},
    {BL1_ERROR_GROUP_EXCEPTIONS, "bl1 group exceptions"},
    {BL1_ERROR_GROUP_MPROC, "bl1 group mppoc"},
    {BL2_PANIC, "bl2 panic"},
    {BL2_WDT, "bl2 wdt"},
    {BL2_PL1_OCV_ERROR, "ocv error"},
    {BL2_PL1_BAT_TEMP_ERROR, "battery temperature error"},
    {BL2_PL1_MISC_ERROR, "misc part dmaged"},
    {BL2_PL1_DTIMG_ERROR, "dt image dmaged"},
    {BL2_PL1_LOAD_OTHER_IMGS_ERRNO, "image dmaged"},
    {BL2_PL1_KERNEL_IMG_ERROR, "kernel image verify failed"},
    {BL2_MMC_INIT_FAILED, "bl2 mmc init error"},
    {BL2_QSEECOM_START_ERROR, "bl2 qseecom start error"},
    {BL2_RPMB_INIT_FAILED, "bl2 rpmb init failed"},
    {BL2_LOAD_SECAPP_FAILED, "bl2 load secapp failed"},
    {BL2_ABOOT_DLKEY_DETECTED, "bl2 dlkey failed"},
    {BL2_ABOOT_NORMAL_BOOT_FAIL, "bl2 aboot normal boot failed"},
    {BL2_BR_POWERON_BY_SMPL, "bl2 poweron by smpl"},
    {BL2_FASTBOOT_S_LOADLPMCU_FAIL, "bl2 load lpmcu failed"},
    {BL2_FASTBOOT_S_IMG_VERIFY_FAIL, "bl2 image verify failed"},
    {BL2_FASTBOOT_S_SOC_TEMP_ERR, "bl2 soc tmp err"},
    {KERNEL_AP_PANIC, "kernel ap panic"},
    {KERNEL_EMMC_INIT_FAIL, "kernel emm init failed"},
    {KERNEL_AP_WDT, "kernel ap wdt"},
    {KERNEL_PRESS10S, "kernel press10s"},
    {KERNEL_BOOT_TIMEOUT, "kernel boot timeout"},
    {KERNEL_AP_COMBINATIONKEY, "kernel ap combinationkey"},
    {KERNEL_AP_S_ABNORMAL, "kernel ap abnormal"},
    {KERNEL_AP_S_TSENSOR0, "kernel ap tsensor0"},
    {KERNEL_AP_S_TSENSOR1, "kernel ap tsensor1"},
    {KERNEL_LPM3_S_GLOBALWDT, "kernel lpm3 global wdt"},
    {KERNEL_G3D_S_G3DTSENSOR, "kernel g3d g3dtsensor"},
    {KERNEL_LPM3_S_LPMCURST, "kernel lpm3 lp mcu reset"},
    {KERNEL_CP_S_CPTSENSOR, "kernel cp cpt sensor"},
    {KERNEL_IOM3_S_IOMCURST, "kernel iom3 io mcu reset"},
    {KERNEL_ASP_S_ASPWD, "kernel asp as pwd"},
    {KERNEL_CP_S_CPWD, "kernel cp cp pwd"},
    {KERNEL_IVP_S_IVPWD, "kernel ivp iv pwd"},
    {KERNEL_ISP_S_ISPWD, "kernel isp is pwd"},
    {KERNEL_AP_S_DDR_UCE_WD, "kernel ap ddr uce wd"},
    {KERNEL_AP_S_DDR_FATAL_INTER, "kernel ap ddr fatal inter"},
    {KERNEL_AP_S_DDR_SEC, "kernel ap ddr sec"},
    {KERNEL_AP_S_MAILBOX, "kernel ap mailbox"},
    {KERNEL_CP_S_MODEMDMSS, "kernel cp modem dmss"},
    {KERNEL_CP_S_MODEMNOC, "kernel cp modem noc"},
    {KERNEL_CP_S_MODEMAP, "kernel cp modem ap"},
    {KERNEL_CP_S_EXCEPTION, "kernel cp exception"},
    {KERNEL_CP_S_RESETFAIL, "kernel cp reset failed"},
    {KERNEL_CP_S_NORMALRESET, "kernel cp normal reset"},
    {KERNEL_CP_S_ABNORMAL, "kernel cp abnormal"},
    {KERNEL_LPM3_S_EXCEPTION, "kernel lpm3 exception"},
    {KERNEL_HIFI_S_EXCEPTION, "kernel hisi exception"},
    {KERNEL_HIFI_S_RESETFAIL, "kernel hisi reset failed"},
    {KERNEL_ISP_S_EXCEPTION, "kernel isp exception"},
    {KERNEL_IVP_S_EXCEPTION, "kernel ivp exception"},
    {KERNEL_IOM3_S_EXCEPTION, "kernel iom3 exception"},
    {KERNEL_TEE_S_EXCEPTION, "kernel tee exception"},
    {KERNEL_MMC_S_EXCEPTION, "kernel mmc exception"},
    {KERNEL_CODECHIFI_S_EXCEPTION, "kernel codec hifi exception"},
    {KERNEL_CP_S_RILD_EXCEPTION, "kernel cp rild exception"},
    {KERNEL_CP_S_3RD_EXCEPTION, "kernel cp 3rd exception"},
    {KERNEL_IOM3_S_USER_EXCEPTION, "kernel iom3 user exception"},
    {KERNEL_MODEM_PANIC, "kernel modem panic"},
    {KERNEL_VENUS_PANIC, "kernel venus panic"},
    {KERNEL_WCNSS_PANIC, "kernel wcnss panic"},
    {KERNEL_SENSOR_PANIC, "kernel sensor panic"},
    {KERNEL_OCBC_S_WD, "kernel ocbc wd"},
    {KERNEL_AP_S_NOC, "kernel noc"},
    {KERNEL_AP_S_RESUME_SLOWY, "kernel resume slowly"},
    {KERNEL_AP_S_F2FS, "kernel f2fs abnormal"},
    {KERNLE_AP_S_BL31_PANIC, "kernel bl31 panic"},
    {KERNLE_HISEE_S_EXCEPTION, "kernel hisee exception"},
    {KERNEL_AP_S_PMU, "kernel ap pmu"},
    {KERNEL_AP_S_SMPL, "kernel ap smpl"},
    {KERNLE_AP_S_SCHARGER, "kernel charger abnormal"},
    {SYSTEM_MOUNT_FAIL, "system part mount failed"},
    {SECURITY_FAIL, "security failed"},
    {CRITICAL_SERVICE_FAIL_TO_START, "zygote or system_server start failed"},
    {DATA_MOUNT_FAILED_AND_ERASED, "data part mount failed"},
    {DATA_MOUNT_RO, "data part mounted ro"},
    {DATA_NOSPC, "no space on data part"},
    {VENDOR_MOUNT_FAIL, "vendor part mount failed"},
    {NATIVE_PANIC, "native critical fail"},
    {SYSTEM_SERVICE_LOAD_FAIL, "system service load failed"},
    {PREBOOT_BROADCAST_FAIL, "preboot broadcast failed"},
    {VM_OAT_FILE_DAMAGED, "ota file damaged"},
    {PACKAGE_MANAGER_SETTING_FILE_DAMAGED, "package manager setting file damaged"},
    {BOOTUP_SLOWLY, "bootup slowly"},
};

static bool s_is_bootup_successfully = false;

/*----function definitions--------------------------------------------------------------*/

static void bfmr_get_critical_process_name(const char *kmsg, size_t kmsg_size, char *pbuf, size_t buf_size)
{
    char *ptemp = NULL;
    char *pstart = NULL;
    int useless_num = 0;
    int kmsg_header_size = bfm_get_kmsg_log_header_size();

    if (unlikely((NULL == kmsg) || (NULL == pbuf) || (kmsg_header_size > kmsg_size)))
    {
        BFMR_PRINT_INVALID_PARAMS("kmsg: %p, pbuf: %p, kmsg_size:%d\n", kmsg, pbuf, (int)kmsg_size);
        goto __out;
    }

    ptemp = strstr(kmsg + kmsg_header_size, BFMR_RECOVERT_MODE_KEYWORD_1);
    if (NULL == ptemp)
    {
        BFMR_PRINT_ERR("Can not find: [%s]\n", BFMR_RECOVERT_MODE_KEYWORD_1);
        ptemp = strstr(kmsg + kmsg_header_size, BFMR_RECOVERT_MODE_KEYWORD_2);
        if (NULL == ptemp)
        {
            BFMR_PRINT_ERR("Can not find: [%s]\n", BFMR_RECOVERT_MODE_KEYWORD_2);
            goto __out;
        }
    }

    while (((--ptemp) >= kmsg) && ('\'' != *ptemp))
    {
        ;
    }

    pstart = --ptemp;
    if (pstart <= kmsg)
    {
        BFMR_PRINT_ERR("Invalid kmsg data!\n");
        goto __out;
    }

    while (((--pstart) >= kmsg) && ('\'' != *pstart))
    {
        ;
    }

    pstart++;
    ptemp++;
    if (pstart >= kmsg)
    {
        strncpy(pbuf, pstart, BFMR_MIN(ptemp - pstart, buf_size - 1));
    }

__out:
    return;
}


static char* bfm_get_boot_fail_no_desc(bfmr_bootfail_errno_e bootfail_errno, bfm_process_bootfail_param_t *pparam)
{
    int i = 0;
    int count = sizeof(s_bootfail_errno_desc) / sizeof(s_bootfail_errno_desc[0]);

    if (unlikely(NULL == pparam))
    {
        BFMR_PRINT_INVALID_PARAMS("pparam: %p\n", pparam);
        return "unknown";
    }

    if (CRITICAL_SERVICE_FAIL_TO_START == bootfail_errno)
    {
        return pparam->critical_process_name;
    }

    for (i = 0; i < count; i++)
    {
        if (bootfail_errno == s_bootfail_errno_desc[i].bootfail_errno)
        {
            return s_bootfail_errno_desc[i].desc;
        }
    }

    return "unknown";
}


static char* bfm_get_boot_stage_name(unsigned int boot_stage)
{
    char *boot_stage_name = NULL;

    if (bfmr_is_bl1_stage(boot_stage))
    {
        boot_stage_name = "BL1";
    }
    else if (bfmr_is_bl2_stage(boot_stage))
    {
        boot_stage_name = "BL2";
    }
    else if (bfmr_is_kernel_stage(boot_stage))
    {
        boot_stage_name = "kernel";
    }
    else if (bfmr_is_native_stage(boot_stage))
    {
            boot_stage_name = "native";
    }
    else if (bfmr_is_android_framework_stage(boot_stage))
    {
        boot_stage_name = "framework";
    }
    else if (bfmr_is_boot_success(boot_stage))
    {
        boot_stage_name = "boot-success";
    }
    else
    {
        boot_stage_name = "unknown";
    }

    return boot_stage_name;
}


static int bfm_get_fs_state(const char *pmount_point, struct statfs *pstatfsbuf)
{
    mm_segment_t old_fs;
    int ret = -1;

    if (unlikely((NULL == pmount_point) || (NULL == pstatfsbuf)))
    {
        BFMR_PRINT_INVALID_PARAMS("pmount_point: %p, pstatfsbuf: %p\n", pmount_point, pstatfsbuf);
        return -1;
    }

    old_fs = get_fs();
    set_fs(KERNEL_DS);
    memset((void *)pstatfsbuf, 0, sizeof(struct statfs));
    ret = sys_statfs(pmount_point, pstatfsbuf);
    set_fs(old_fs);

    return ret;
}


static int bfm_save_bootfail_info_txt(bfmr_log_dst_t *pdst, bfmr_log_src_t *psrc, bfm_process_bootfail_param_t *pparam)
{
    int ret = -1;
    char *pdata = NULL;
    struct statfs statfsbuf = {0};

    if (unlikely(NULL == pparam))
    {
        BFMR_PRINT_INVALID_PARAMS("pparam: %p\n", pparam);
        return -1;
    }

    pdata = (char *)bfmr_malloc(BFMR_TEMP_BUF_LEN);
    if (NULL == pdata)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        return -1;
    }
    memset(pdata, 0, BFMR_TEMP_BUF_LEN);

    ret = bfmr_wait_for_part_mount_with_timeout("/data", BFM_WAIT_DATA_PART_TIME_OUT);
    if (0 == ret)
    {
        ret = bfm_get_fs_state("/data", &statfsbuf);
    }

    (void)snprintf(pdata, BFMR_TEMP_BUF_LEN - 1, BFM_BFI_FILE_CONTENT_FORMAT,
        bfmr_convert_rtc_time_to_asctime(pparam->bootfail_time),
        bfm_get_platform_name(),
        bfm_get_boot_stage_name((unsigned int)pparam->boot_stage),
        bfm_get_boot_fail_no_desc(pparam->bootfail_errno, pparam),
        bfm_get_platform_name(),
        bfm_get_boot_stage_name((unsigned int)pparam->boot_stage),
        pparam->is_system_rooted,
        pparam->is_user_sensible,
        (0 == ret) ? ((long long)(statfsbuf.f_bavail * statfsbuf.f_bsize) / (long long)(BFMR_SIZE_1K * BFMR_SIZE_1K)) : (0L),
        (0 == ret) ? ((statfsbuf.f_blocks <= 0) ? (0LL) : ((long long)(100 * statfsbuf.f_bavail) / (long long)statfsbuf.f_blocks)) : (0LL),
        (0 == ret) ? ((long long)(statfsbuf.f_files - statfsbuf.f_ffree)) : (0L),
        (0 == ret) ? ((statfsbuf.f_files <= 0) ? (0LL) : ((long long)(100 * (statfsbuf.f_files - statfsbuf.f_ffree)) / (long long)statfsbuf.f_files)): (0LL),
        pparam->bootfail_time,
        (unsigned int)pparam->bootfail_errno,
        (unsigned int)pparam->boot_stage,
        pparam->is_system_rooted,
        pparam->is_user_sensible,
        pparam->dmd_num,
        (unsigned int)pparam->bootup_time,
        bfm_get_bootlock_value_from_cmdline());

        switch (pdst->type)
        {
        case DST_FILE:
            {
                ret = bfmr_save_log(pparam->bootfail_log_dir, BFM_BFI_FILE_NAME, (void *)pdata, strlen(pdata), 0);
                if (0 != ret)
                {
                    BFMR_PRINT_ERR("save [%s] failed!\n", BFM_BFI_FILE_NAME);
                }
                break;
            }
        case DST_RAW_PART:
            {
                bfmr_save_log_to_raw_part(pdst->dst_info.raw_part.raw_part_name,
                                          pdst->dst_info.raw_part.offset,
                                          (void *)pdata, strlen(pdata));
                psrc->log_type = LOG_TYPE_BFM_BFI_LOG;
                strncpy(psrc->src_log_file_path, BFM_BFI_FILE_NAME, BFMR_MAX_PATH);
                bfmr_update_raw_log_info(psrc, pdst, strlen(pdata));
                break;
            }
        case DST_MEMORY_BUFFER:
        default:
            {
                bfmr_save_log_to_mem_buffer(pdst->dst_info.buffer.addr, pdst->dst_info.buffer.len, (void *)pdata, strlen(pdata));
                break;
            }
        }

__out:
    bfmr_free(pdata);

    return ret;
}


static int bfm_get_recovery_result(bfr_suggested_recovery_method_e suggested_recovery_method)
{
    return (DO_NOTHING == suggested_recovery_method)
        ? (int)(BFM_RECOVERY_SUCCESS_INT_VALUE)
        : (int)(BFM_RECOVERY_FAIL_INT_VALUE);
}


static int bfm_save_recovery_info_txt(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam)
{
    int ret = -1;
    char *pdata = NULL;

    if (unlikely(NULL == pparam))
    {
        BFMR_PRINT_INVALID_PARAMS("pparam: %p\n", pparam);
        return -1;
    }

    pdata = (char *)bfmr_malloc(BFMR_TEMP_BUF_LEN);
    if (NULL == pdata)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        return -1;
    }
    memset(pdata, 0, BFMR_TEMP_BUF_LEN);

    (void)snprintf(pdata, BFMR_TEMP_BUF_LEN - 1, BFM_RCV_FILE_CONTENT_FORMAT,
        bfr_get_recovery_method_desc(pparam->recovery_method),
        (BFM_RECOVERY_SUCCESS_INT_VALUE == bfm_get_recovery_result(pparam->suggested_recovery_method))
        ?  BFM_RECOVERY_SUCCESS_STR : BFM_RECOVERY_FAIL_STR,
        pparam->recovery_method,
        bfm_get_recovery_result(pparam->suggested_recovery_method));

        switch (pdst->type)
        {
        case DST_FILE:
            {
                ret = bfmr_save_log(pparam->bootfail_log_dir, BFM_RECOVERY_FILE_NAME, (void *)pdata, strlen(pdata), 0);
                if (0 != ret)
                {
                    BFMR_PRINT_ERR("save [%s] failed!\n", BFM_RECOVERY_FILE_NAME);
                }
                break;
            }
        case DST_RAW_PART:
            {
                bfmr_save_log_to_raw_part(pdst->dst_info.raw_part.raw_part_name,
                                          pdst->dst_info.raw_part.offset,
                                          (void *)pdata, strlen(pdata));
                psrc->log_type = LOG_TYPE_BFM_RECOVERY_LOG;
                strncpy(psrc->src_log_file_path, BFM_RECOVERY_FILE_NAME, BFMR_MAX_PATH);
                bfmr_update_raw_log_info(psrc, pdst, strlen(pdata));
                break;
            }
        case DST_MEMORY_BUFFER:
        default:
            {
                bfmr_save_log_to_mem_buffer(pdst->dst_info.buffer.addr, pdst->dst_info.buffer.len, (void *)pdata, strlen(pdata));
                break;
            }
        }

__out:
    bfmr_free(pdata);

    return ret;
}


int bfm_get_log_count(char *bfmr_log_root_path)
{
    int fd = -1;
    int num;
    int log_count = 0;
    void *buf = NULL;
    char *full_path = NULL;
    struct linux_dirent64 *dirp;
    mm_segment_t oldfs;

    if (unlikely((NULL == bfmr_log_root_path)))
    {
        BFMR_PRINT_INVALID_PARAMS("bfmr_log_root_path: %p\n", bfmr_log_root_path);
        return 0;
    }

    oldfs = get_fs();
    set_fs(KERNEL_DS);

    fd = sys_open(bfmr_log_root_path, O_RDONLY, 0);
    if (fd < 0)
    {
        BFMR_PRINT_ERR("open [%s] failed!\n", bfmr_log_root_path);
        goto __out;
    }

    buf = bfmr_malloc(BFMR_MAX_PATH);
    if (NULL == buf)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }

    full_path = (char *)bfmr_malloc(BFMR_MAX_PATH);
    if (NULL == full_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }

    dirp = buf;
    num = sys_getdents64(fd, dirp, BFMR_MAX_PATH);
    while (num > 0)
    {
        while (num > 0)
        {
            bfm_stat_t st;
            int ret;

            memset(full_path, 0, BFMR_MAX_PATH);
            snprintf(full_path, BFMR_MAX_PATH - 1, "%s/%s", bfmr_log_root_path, dirp->d_name);
            if ((0 == strcmp(dirp->d_name, ".")) || (0 == strcmp(dirp->d_name, "..")))
            {
                num -= dirp->d_reclen;
                dirp = (void *)dirp + dirp->d_reclen;
                continue;
            }

            memset((void *)&st, 0, sizeof(bfm_stat_t));
            ret = bfm_sys_lstat(full_path, &st);
            if ((0 == ret)
                && (S_ISDIR(st.st_mode))
                && (0 != strcmp(dirp->d_name, BFM_UPLOADING_DIR_NAME)))
            {
                log_count++;
            }

            num -= dirp->d_reclen;
            dirp = (void *)dirp + dirp->d_reclen;
        }

        dirp = buf;
        memset(buf, 0, BFMR_MAX_PATH);
        num = sys_getdents64(fd, dirp, BFMR_MAX_PATH);
    }

__out:
    if (fd >= 0)
    {
        sys_close(fd);
    }
    set_fs(oldfs);

    BFMR_PRINT_DBG("Log count: %d\n", log_count);

    bfmr_free(buf);
    bfmr_free(full_path);

    return log_count;
}


void bfm_delete_dir(char *log_path)
{
    int fd = -1;
    void *buf = NULL;
    char *full_path = NULL;
    struct linux_dirent64 *dirp;
    int num;
    mm_segment_t oldfs;

    oldfs = get_fs();
    set_fs(KERNEL_DS);
    fd = sys_open(log_path, O_RDONLY, 0);
    if (fd < 0)
    {
        BFMR_PRINT_ERR("open [%s] failed![ret = %d]\n", log_path, fd);
        goto __out;
    }

    buf =(char *)bfmr_malloc(BFMR_MAX_PATH);
    if (NULL == buf)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset(buf, 0, BFMR_MAX_PATH);

    full_path = (char *)bfmr_malloc(BFMR_MAX_PATH);
    if (NULL == full_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset(full_path, 0, BFMR_MAX_PATH);

    dirp = buf;
    num = sys_getdents64(fd, dirp, BFMR_MAX_PATH);
    while (num > 0)
    {
        while (num > 0)
        {
            bfm_stat_t st;
            int ret;

            memset(full_path, 0, BFMR_MAX_PATH);
            memset((void *)&st, 0, sizeof(bfm_stat_t));
            snprintf(full_path, BFMR_MAX_PATH - 1, "%s/%s", log_path, dirp->d_name);
            if ((0 != strcmp(dirp->d_name, ".")) && (0 != strcmp(dirp->d_name, "..")))
            {
                ret = bfm_sys_lstat(full_path, &st);
                if (0 == ret)
                {
                    if (S_ISDIR(st.st_mode))
                    {
                        sys_rmdir(full_path);
                    }
                    else
                    {
                        sys_unlink(full_path);
                    }
                }
            }
            num -= dirp->d_reclen;
            dirp = (void *)dirp + dirp->d_reclen;
        }
        dirp = buf;
        memset(buf, 0, BFMR_MAX_PATH);
        num = sys_getdents64(fd, dirp, BFMR_MAX_PATH);
    }

__out:
    if (fd >= 0)
    {
        sys_close(fd);
    }

    /* remove the log path */
    sys_rmdir(log_path);
    set_fs(oldfs);

    bfmr_free(buf);
    bfmr_free(full_path);
}


static void bfm_delete_oldest_log(long long bytes_need_this_time)
{
    int log_count = 0;
    char *log_path = NULL;
    long long available_space = 0LL;
    bool should_delete_oldest_log = false;

    log_count = bfm_get_log_count(bfm_get_bfmr_log_root_path());
    BFMR_PRINT_KEY_INFO("There are %d logs in %s\n", log_count, bfm_get_bfmr_log_root_path());
    if (log_count < BFM_LOG_MAX_COUNT)
    {
        available_space = bfmr_get_fs_available_space(bfm_get_bfmr_log_part_mount_point());
        if (available_space < bytes_need_this_time)
        {
            should_delete_oldest_log = true;
        }
    }
    else
    {
        should_delete_oldest_log = true;
    }

    if (!should_delete_oldest_log)
    {
        goto __out;
    }

    log_path = (char *)bfmr_malloc(BFMR_MAX_PATH);
    if (NULL == log_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset(log_path, 0, BFMR_MAX_PATH);

    (void)bfm_lookup_dir_by_create_time(bfm_get_bfmr_log_root_path(), log_path, BFMR_MAX_PATH, 1);
    if (0 != strlen(log_path))
    {
        bfm_delete_dir(log_path);
    }

__out:
    bfmr_free(log_path);
}


static void bfm_process_after_save_bootfail_log(void)
{
    BFMR_PRINT_KEY_INFO("restart system now!\n");
    kernel_restart(NULL);
    return;
}


static long long bfm_get_basic_space_for_each_bootfail_log(bfmr_bootfail_errno_e bootfail_errno)
{
    int i = 0;
    int count = sizeof(s_log_size_param) / sizeof(s_log_size_param[0]);

    for (i = 0; i < count; i++)
    {
        if (bfmr_get_boot_stage_from_bootfail_errno(bootfail_errno) == s_log_size_param[i].boot_stage)
        {
            return s_log_size_param[i].log_size_allowed;
        }
    }

    return (long long)(BFM_BOOTFAIL_INFO_LOG_MAX_LEN + BFM_RECOVERY_INFO_LOG_MAX_LEN);
}


static long long bfm_get_extra_space_for_each_bootfail_log(bfm_process_bootfail_param_t *pparam)
{
    long long bytes_need = 0LL;

    if (unlikely(NULL == pparam))
    {
        BFMR_PRINT_INVALID_PARAMS("pparam: %p\n", pparam);
        return 0LL;
    }

    if (0 != strcmp(pparam->user_log_path, BFM_FRAMEWORK_BOOTFAIL_LOG_PATH))
    {
        bytes_need = (long long)(bfmr_get_file_length(BFM_LOGCAT_FILE_PATH)
            + bfmr_get_file_length(BFM_FRAMEWORK_BOOTFAIL_LOG_PATH)
            + bfmr_get_file_length(pparam->user_log_path));
    }
    else
    {
        bytes_need = (long long)(bfmr_get_file_length(BFM_LOGCAT_FILE_PATH)
            + bfmr_get_file_length(pparam->user_log_path)); 
    }

    return bytes_need;
}


static int bfm_save_extra_bootfail_logs(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam)
{
    int ret = -1;
    char src_path[BFMR_SIZE_256] = {0};

    if (unlikely((NULL == pdst) || (NULL == psrc) || (NULL == pparam)))
    {
        BFMR_PRINT_INVALID_PARAMS("pdst: %p, psrc: %p, pparam: %p\n", pdst, psrc, pparam);
        return -1;
    }

    /* 1. save logcat */
    memset((void *)pparam->bootfail_log_path, 0, sizeof(pparam->bootfail_log_path));
    (void)snprintf(pparam->bootfail_log_path, sizeof(pparam->bootfail_log_path) - 1,
        "%s/%s", pparam->bootfail_log_dir, BFM_LOGCAT_FILE_NAME);
    if (bfm_get_symbol_link_path(BFM_LOGCAT_FILE_PATH, src_path, sizeof(src_path)))
    {
        (void)snprintf(pparam->bootfail_log_path, sizeof(pparam->bootfail_log_path) - 1,
            "%s/%s", pparam->bootfail_log_dir, BFM_LOGCAT_GZ_FILE_NAME);
    }
    psrc->log_type = LOG_TYPE_BETA_APP_LOGCAT;
    ret = bfmr_capture_and_save_log(psrc, pdst, pparam);

    /* 2. save framework bootfail log */
    if ((pparam->boot_stage >= ANDROID_FRAMEWORK_STAGE_START)
        && (0 != strcmp(pparam->user_log_path, BFM_FRAMEWORK_BOOTFAIL_LOG_PATH)))
    {
        memset((void *)pparam->bootfail_log_path, 0, sizeof(pparam->bootfail_log_path));
        (void)snprintf(pparam->bootfail_log_path, sizeof(pparam->bootfail_log_path) - 1,
            "%s/%s", pparam->bootfail_log_dir, BFM_FRAMEWORK_BOOTFAIL_LOG_FILE_NAME);
        psrc->log_type = LOG_TYPE_FIXED_FRAMEWORK_BOOTFAIL_LOG;
        ret = bfmr_capture_and_save_log(psrc, pdst, pparam);
        bfmr_change_file_ownership(BFM_FRAMEWORK_BOOTFAIL_LOG_PATH, pparam->user_space_log_uid,
            pparam->user_space_log_gid);
    }

    return ret;
}


static int bfm_capture_and_save_bl1_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam)
{
    if (unlikely((NULL == pdst) || (NULL == psrc) || (NULL == pparam)))
    {
        BFMR_PRINT_INVALID_PARAMS("pdst: %p, psrc: %p, pparam: %p\n", pdst, psrc, pparam);
        return -1;
    }

    memset((void *)pparam->bootfail_log_path, 0, sizeof(pparam->bootfail_log_path));
    (void)snprintf(pparam->bootfail_log_path, sizeof(pparam->bootfail_log_path) - 1,
        "%s/%s", pparam->bootfail_log_dir, bfm_get_bl1_bootfail_log_name());
    psrc->log_type = LOG_TYPE_BOOTLOADER_1;

    return bfmr_capture_and_save_log(psrc, pdst, pparam);
}


static int bfm_capture_and_save_bl2_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam)
{
    if (unlikely((NULL == pdst) || (NULL == psrc) || (NULL == pparam)))
    {
        BFMR_PRINT_INVALID_PARAMS("pdst: %p, psrc: %p, pparam: %p\n", pdst, psrc, pparam);
        return -1;
    }

    memset((void *)pparam->bootfail_log_path, 0, sizeof(pparam->bootfail_log_path));
    (void)snprintf(pparam->bootfail_log_path, sizeof(pparam->bootfail_log_path) - 1,
        "%s/%s", pparam->bootfail_log_dir, bfm_get_bl2_bootfail_log_name());
    psrc->log_type = LOG_TYPE_BOOTLOADER_2;

    return bfmr_capture_and_save_log(psrc, pdst, pparam);
}


static int bfm_capture_and_save_kernel_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam)
{
    bool file_existed = false;

    if (unlikely((NULL == pdst) || (NULL == psrc) || (NULL == pparam)))
    {
        BFMR_PRINT_INVALID_PARAMS("pdst: %p, psrc: %p, pparam: %p\n", pdst, psrc, pparam);
        return -1;
    }

    /* capture last kmsg of zip type on Beta version */
    memset((void *)pparam->bootfail_log_path, 0, sizeof(pparam->bootfail_log_path));
    file_existed = bfmr_is_file_existed(BFM_BETA_KMSG_LOG_PATH);
    BFMR_PRINT_KEY_INFO("[%s] %s!\n", BFM_BETA_KMSG_LOG_PATH, (file_existed) ? ("exists") : ("dosen't exist"));
    if (bfm_is_beta_version() && file_existed)
    {
        (void)snprintf(pparam->bootfail_log_path, sizeof(pparam->bootfail_log_path) - 1,
            "%s/%s.android.gz", pparam->bootfail_log_dir, bfm_get_kernel_bootfail_log_name());
        psrc->log_type = LOG_TYPE_ANDROID_KMSG;
        (void)bfmr_capture_and_save_log(psrc, pdst, pparam);
    }

    /* capture last kmsg of text type */
    memset((void *)pparam->bootfail_log_path, 0, sizeof(pparam->bootfail_log_path));
    (void)snprintf(pparam->bootfail_log_path, sizeof(pparam->bootfail_log_path) - 1,
        "%s/%s", pparam->bootfail_log_dir, bfm_get_kernel_bootfail_log_name());
    psrc->log_type = LOG_TYPE_TEXT_KMSG;

    return bfmr_capture_and_save_log(psrc, pdst, pparam);
}


static int bfm_capture_and_save_ramoops_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam)
{
    if (unlikely((NULL == pdst) || (NULL == psrc) || (NULL == pparam)))
    {
        BFMR_PRINT_INVALID_PARAMS("pdst: %p, psrc: %p, pparam: %p\n", pdst, psrc, pparam);
        return -1;
    }

    memset((void *)pparam->bootfail_log_path, 0, sizeof(pparam->bootfail_log_path));
    (void)snprintf(pparam->bootfail_log_path, sizeof(pparam->bootfail_log_path) - 1,
        "%s/%s", pparam->bootfail_log_dir, bfm_get_ramoops_bootfail_log_name());
    psrc->log_type = LOG_TYPE_RAMOOPS;

    return bfmr_capture_and_save_log(psrc, pdst, pparam);
}


static char *bfm_get_file_name(char *file_full_path)
{
    char *ptemp = NULL;

    if (unlikely((NULL == file_full_path)))
    {
        BFMR_PRINT_INVALID_PARAMS("file_full_path: %p\n", file_full_path);
        return NULL;
    }

    ptemp = strrchr(file_full_path, '/');
    if (NULL == ptemp)
    {
        return file_full_path;
    }

    return (ptemp + 1);
}


static int bfm_capture_and_save_native_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam)
{
    int ret = -1;

    if (unlikely((NULL == pdst) || (NULL == psrc) || (NULL == pparam)))
    {
        BFMR_PRINT_INVALID_PARAMS("pdst: %p, psrc: %p, pparam: %p\n", pdst, psrc, pparam);
        return -1;
    }

    if (0 == strlen(pparam->user_log_path))
    {
        BFMR_PRINT_KEY_INFO("user log path hasn't been set!\n");
        return -1;
    }
    else
    {
        memset((void *)pparam->bootfail_log_path, 0, sizeof(pparam->bootfail_log_path));
        (void)snprintf(pparam->bootfail_log_path, sizeof(pparam->bootfail_log_path) - 1,
            "%s/%s", pparam->bootfail_log_dir, bfm_get_file_name(pparam->user_log_path));
        if (NULL != strstr(pparam->user_log_path, BFM_TOMBSTONE_FILE_NAME_TAG))
        {
            psrc->log_type = LOG_TYPE_VM_TOMBSTONES;
        }
        else if (NULL != strstr(pparam->user_log_path, BFM_SYSTEM_SERVER_CRASH_FILE_NAME_TAG))
        {
            psrc->log_type = LOG_TYPE_VM_CRASH;
        }
        else if (NULL != strstr(pparam->user_log_path, BFM_SYSTEM_SERVER_WATCHDOG_FILE_NAME_TAG))
        {
            psrc->log_type = LOG_TYPE_VM_WATCHDOG;
        }
        else if (NULL != strstr(pparam->user_log_path, BFM_CRITICAL_PROCESS_CRASH_LOG_NAME))
        {
            psrc->log_type = LOG_TYPE_CRITICAL_PROCESS_CRASH;
        }
        else
        {
            BFMR_PRINT_KEY_INFO("Invalid user bootfail log!\n");
            return -1;
        }
    }

    ret = bfmr_capture_and_save_log(psrc, pdst, pparam);
    bfmr_change_file_ownership(pparam->user_log_path, pparam->user_space_log_uid, pparam->user_space_log_gid);

    /* remove critical process crash log in /cache */
    if (NULL != strstr(pparam->user_log_path, BFM_CRITICAL_PROCESS_CRASH_LOG_NAME))
    {
        bfmr_unlink_file(pparam->user_log_path);
    }

    return ret;
}


static int bfm_capture_and_save_framework_bootfail_log(bfmr_log_dst_t *pdst,
    bfmr_log_src_t *psrc,
    bfm_process_bootfail_param_t *pparam)
{
    int ret = -1;

    if (unlikely((NULL == pdst) || (NULL == psrc) || (NULL == pparam)))
    {
        BFMR_PRINT_INVALID_PARAMS("pdst: %p, psrc: %p, pparam: %p\n", pdst, psrc, pparam);
        return -1;
    }

    if (0 == strlen(pparam->user_log_path))
    {
        BFMR_PRINT_KEY_INFO("user log path hasn't been set!\n");
        return -1;
    }

    memset((void *)pparam->bootfail_log_path, 0, sizeof(pparam->bootfail_log_path));
    (void)snprintf(pparam->bootfail_log_path, sizeof(pparam->bootfail_log_path) - 1,
        "%s/%s", pparam->bootfail_log_dir, bfm_get_file_name(pparam->user_log_path));
    psrc->log_type = LOG_TYPE_NORMAL_FRAMEWORK_BOOTFAIL_LOG;
    ret = bfmr_capture_and_save_log(psrc, pdst, pparam);
    bfmr_change_file_ownership(pparam->user_log_path, pparam->user_space_log_uid, pparam->user_space_log_gid);

    return ret;
}


static bool bfm_is_log_existed(unsigned long long rtc_time, unsigned int bootfaiL_errno)
{
    char *log_full_path = NULL;
    bool ret = false;

    log_full_path = (char *)bfmr_malloc(BFMR_MAX_PATH);
    if (NULL == log_full_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset((void *)log_full_path, 0, BFMR_MAX_PATH);

    snprintf(log_full_path, BFMR_MAX_PATH - 1, "%s/" BFM_BOOTFAIL_LOG_DIR_NAME_FORMAT,
        bfm_get_bfmr_log_root_path(), bfmr_convert_rtc_time_to_asctime(rtc_time), bootfaiL_errno);
    ret = bfmr_is_dir_existed(log_full_path);
    BFMR_PRINT_KEY_INFO("[%s] %s\n", log_full_path, (ret) ? ("exists!") : ("does't exist!"));

__out:
    bfmr_free(log_full_path);

    return ret;
}


static int bfm_capture_and_save_bootfail_log(bfm_process_bootfail_param_t *pparam)
{
    int ret = -1;
    bfmr_log_dst_t dst;
    bfmr_log_src_t src;

    if (unlikely(NULL == pparam))
    {
        BFMR_PRINT_INVALID_PARAMS("pparam: %p\n", pparam);
        return -1;
    }

    memset((void *)&dst, 0, sizeof(dst));
    memset((void *)&src, 0, sizeof(src));
    dst.type = pparam->dst_type;
    src.log_save_additional_context = (void *)pparam;
    switch (dst.type)
    {
    case DST_FILE:
        {
            dst.dst_info.filename = pparam->bootfail_log_path;
            src.src_log_file_path = pparam->user_log_path;
            src.log_save_context = pparam->log_save_context;
            if (!bfmr_is_part_mounted_rw(bfm_get_bfmr_log_part_mount_point()))
            {
                BFMR_PRINT_ERR("the log part hasn't been mounted to: %s\n", bfm_get_bfmr_log_part_mount_point());
                goto __out;
            }

            /* create log root path */
            (void)bfmr_create_log_path(bfm_get_bfmr_log_root_path());

            /* create uploading path */
            (void)bfmr_create_log_path(bfm_get_bfmr_log_uploading_path());

            /* check if the log is existed or not */
            if (bfm_is_log_existed(pparam->bootfail_time, (unsigned int)pparam->bootfail_errno))
            {
                ret = 0;
                goto __out;
            }

            /* delet oldest log */
            if (0 == pparam->save_bottom_layer_bootfail_log)
            {
                bfm_delete_oldest_log(bfm_get_basic_space_for_each_bootfail_log(pparam->bootfail_errno)
                    + bfm_get_extra_space_for_each_bootfail_log(pparam));
            }
            else
            {
                bfm_delete_oldest_log(bfm_get_basic_space_for_each_bootfail_log(pparam->bootfail_errno));    
            }

            /* create boot fail log dir */
            (void)snprintf(pparam->bootfail_log_dir, sizeof(pparam->bootfail_log_dir) - 1,
                "%s/" BFM_BOOTFAIL_LOG_DIR_NAME_FORMAT, bfm_get_bfmr_log_root_path(),
                bfmr_convert_rtc_time_to_asctime(pparam->bootfail_time), (unsigned int)pparam->bootfail_errno);
            bfmr_create_log_path(pparam->bootfail_log_dir);

            break;
        }
    case DST_RAW_PART:
        {
            dst.dst_info.raw_part.raw_part_name = bfm_get_raw_part_name();
            dst.dst_info.raw_part.offset += bfm_get_raw_part_offset();
            src.src_log_file_path = pparam->user_log_path;
            (void)snprintf(pparam->bootfail_log_dir, sizeof(pparam->bootfail_log_dir) - 1,
                "%s/" BFM_BOOTFAIL_LOG_DIR_NAME_FORMAT, bfm_get_bfmr_log_root_path(),
                bfmr_convert_rtc_time_to_asctime(pparam->bootfail_time), (unsigned int)pparam->bootfail_errno);
            bfmr_alloc_and_init_raw_log_info(pparam, &dst);
            src.log_save_context = pparam->log_save_context;
            break;
        }
    case DST_MEMORY_BUFFER:
        {
            break;
        }
    default:
        {
            BFMR_PRINT_ERR("Invalid dst type: %d\n", dst.type);
            goto __out;
        }
    }

    ret = 0;
    switch (bfmr_get_boot_stage_from_bootfail_errno(pparam->bootfail_errno))
    {
    case BL1_STAGE:
        {
            BFMR_PRINT_KEY_INFO("Boot fail @ BL1, bootfail_errno: 0x%x\n", (unsigned int)pparam->bootfail_errno);
            bfm_capture_and_save_bl1_bootfail_log(&dst, &src, pparam);
            break;
        }
    case BL2_STAGE:
        {
            BFMR_PRINT_KEY_INFO("Boot fail @ BL2, bootfail_errno: 0x%x\n", (unsigned int)pparam->bootfail_errno);
            bfm_capture_and_save_bl1_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_bl2_bootfail_log(&dst, &src, pparam);
            break;
        }
    case KERNEL_STAGE:
        {
            BFMR_PRINT_KEY_INFO("Boot fail @ Kernel, bootfail_errno: 0x%x\n", (unsigned int)pparam->bootfail_errno);
            bfm_capture_and_save_bl1_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_bl2_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_kernel_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_ramoops_bootfail_log(&dst, &src, pparam);
            break;
        }
    case NATIVE_STAGE:
        {
            BFMR_PRINT_KEY_INFO("Boot fail @ Native, bootfail_errno: 0x%x\n", (unsigned int)pparam->bootfail_errno);
            bfm_capture_and_save_bl1_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_bl2_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_kernel_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_ramoops_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_native_bootfail_log(&dst, &src, pparam);
            break;
        }
    case ANDROID_FRAMEWORK_STAGE:
        {
            BFMR_PRINT_KEY_INFO("Boot fail @ Framework, bootfail_errno: 0x%x\n", (unsigned int)pparam->bootfail_errno);
            bfm_capture_and_save_bl1_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_bl2_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_kernel_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_ramoops_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_native_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_framework_bootfail_log(&dst, &src, pparam);
            break;
        }
    default:
        {
            BFMR_PRINT_KEY_INFO("Boot fail @ Unknown stage, bootfail_errno: 0x%x\n", (unsigned int)pparam->bootfail_errno);
            bfm_capture_and_save_bl1_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_bl2_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_kernel_bootfail_log(&dst, &src, pparam);
            bfm_capture_and_save_ramoops_bootfail_log(&dst, &src, pparam);
            break;
        }
    }

    /* save extra logs such as: logcat */
    if (0 ==pparam->save_bottom_layer_bootfail_log)
    {
        bfm_save_extra_bootfail_logs(&dst, &src, pparam);
    }

    /* save bootFail_info.txt */
    bfm_save_bootfail_info_txt(&dst, &src, pparam);

    /* save recovery_info.txt */
    bfm_save_recovery_info_txt(&dst, &src, pparam);

    if (DST_RAW_PART == dst.type)
    {
        bfmr_save_and_free_raw_log_info(pparam);
    }

__out:
    return ret;
}


static int bfm_process_upper_layer_boot_fail(void *param)
{
    bfm_process_bootfail_param_t *pparam = (bfm_process_bootfail_param_t*)param;

    if (unlikely(NULL == param))
    {
        BFMR_PRINT_INVALID_PARAMS("param: %p\n", param);
        goto __out;
    }

    if ((0 != strlen(pparam->user_log_path)) && (pparam->user_space_read_len <= 0))
    {
        uid_t uid = 0;
        gid_t gid = 0;

        /* if the user space log's original ownership has not been gotten correctly, we can not change its onwership here */
        if ((0 == bfmr_get_uid_gid(&uid, &gid)) && (-1 != (int)pparam->user_space_log_uid)
            && (-1 != (int)pparam->user_space_log_gid))
        {
            bfmr_change_file_ownership(pparam->user_log_path, uid, gid);
        }
    }

    /* 1. this case means the boot fail has been resolved */
    if (DO_NOTHING == pparam->suggested_recovery_method)
    {
        BFMR_PRINT_KEY_INFO("suggested recovery method is: \"DO_NOTHING\"\n");
        (void)bfm_capture_and_save_do_nothing_bootfail_log(pparam);
        goto __out;
    }

    /* 2. process boot fail furtherly in platform */
    (void)bfm_platform_process_boot_fail(pparam);

    if (0 == pparam->bootfail_can_only_be_processed_in_platform)
    {
        /* 3. capture and save log for most boot fail errno */
        bfm_capture_and_save_bootfail_log(pparam);

        /* 4. post process after save bootfail log */
        bfm_process_after_save_bootfail_log();
    }

__out:
    if (NULL != pparam->user_space_log_buf)
    {
        bfmr_free(pparam->user_space_log_buf);
    }

    if (NULL != param)
    {
        bfmr_free(param);
    }
    msleep(BFM_SAVE_LOG_INTERVAL_FOR_EACH_LOG);
    complete(&s_process_boot_fail_comp);
    return 0;
}


static void bfm_wait_for_compeletion_of_processing_boot_fail(void)
{
    while (1)
    {
        msleep_interruptible(BFM_BLOCK_CALLING_PROCESS_INTERVAL);
    }
}


/**
    @function: int boot_fail_err(bfmr_bootfail_errno_e bootfail_errno,
        bfr_suggested_recovery_method_e suggested_recovery_method,
        char *log_path)
    @brief: save the log and do proper recovery actions when meet with error during system booting process.

    @param: bootfail_errno [in], boot fail error no.
    @param: suggested_recovery_method [in], suggested recovery method, if you don't know, please transfer NO_SUGGESTION for it
    @param: log_path [in], path of log file, if an additional log file existed and need to be saved.

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int boot_fail_err(bfmr_bootfail_errno_e bootfail_errno,
    bfr_suggested_recovery_method_e suggested_recovery_method,
    char *log_path)
{
    bfmr_detail_boot_stage_e boot_stage;
    bfm_process_bootfail_param_t *pparam = NULL;
    static bool s_is_comp_init = false;

    BFMR_PRINT_ENTER();
    if (!bfmr_has_been_enabled())
    {
        BFMR_PRINT_KEY_INFO("BFMR is disabled!\n");
        return 0;
    }

    mutex_lock(&s_process_boot_fail_mutex);
    if (!s_is_comp_init)
    {
        complete(&s_process_boot_fail_comp);
        s_is_comp_init = true;
    }
    bfmr_get_boot_stage(&boot_stage);
    if (bfmr_is_boot_success(boot_stage))
    {
        if ((CRITICAL_SERVICE_FAIL_TO_START != bootfail_errno) && (BOOTUP_SLOWLY != bootfail_errno))
        {
            BFMR_PRINT_ERR("Error: can't set errno [%x] after device boot success!\n", (unsigned int)bootfail_errno);
            goto __out;
        }

        BFMR_PRINT_KEY_INFO("%s", (CRITICAL_SERVICE_FAIL_TO_START == bootfail_errno)
            ? ("critical process work abnormally after boot success") : ("bootup slowly!\n"));
    }

    if (DO_NOTHING == suggested_recovery_method)
    {
        if (wait_for_completion_timeout(&s_process_boot_fail_comp, msecs_to_jiffies(BFM_SAVE_LOG_MAX_TIME)) == 0)
        {
            BFMR_PRINT_KEY_INFO("last boot_err is in processing, this error skip for DO_NOTHING!\n");
            goto __out;
        }
    }
    else
    {
        wait_for_completion(&s_process_boot_fail_comp);
    }

    pparam = (bfm_process_bootfail_param_t*)bfmr_malloc(sizeof(bfm_process_bootfail_param_t));
    if (NULL == pparam)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }
    memset((void *)pparam, 0, sizeof(bfm_process_bootfail_param_t));

    pparam->bootfail_errno = bootfail_errno;
    pparam->boot_stage = boot_stage;
    pparam->is_user_sensible = (suggested_recovery_method != DO_NOTHING) ? (1) : (0);
    pparam->bootfail_errno = bootfail_errno;
    pparam->suggested_recovery_method = suggested_recovery_method;
    pparam->user_space_log_uid = (uid_t)-1;
    pparam->user_space_log_gid = (gid_t)-1;
    if (NULL != log_path)
    {
        strncpy(pparam->user_log_path, log_path, BFMR_MIN(sizeof(pparam->user_log_path) - 1, strlen(log_path)));
        (void)bfmr_get_file_ownership(pparam->user_log_path, &pparam->user_space_log_uid, &pparam->user_space_log_gid);
        pparam->user_space_log_len = bfmr_get_file_length(pparam->user_log_path);
        if (pparam->user_space_log_len > 0)
        {
            pparam->user_space_log_buf = (char *)bfmr_malloc(pparam->user_space_log_len + 1);
            if (NULL != pparam->user_space_log_buf)
            {
                memset(pparam->user_space_log_buf, 0, pparam->user_space_log_len + 1);
                pparam->user_space_read_len = bfmr_full_read_with_file_path(pparam->user_log_path,
                    pparam->user_space_log_buf, pparam->user_space_log_len);
                BFMR_PRINT_KEY_INFO("Read file [%s] [%ld] Bytes successsfully, its length is [%ld] Bytes!\n",
                    pparam->user_log_path, pparam->user_space_read_len, pparam->user_space_log_len);
            }
        }
    }
    pparam->bootfail_time = bfm_get_system_time();
    pparam->bootup_time = bfmr_get_bootup_time();
    pparam->is_user_sensible = bfm_is_user_sensible_bootfail(pparam->bootfail_errno, pparam->suggested_recovery_method),
    pparam->is_system_rooted = bfm_is_system_rooted();
    pparam->bootfail_can_only_be_processed_in_platform = 0;
    pparam->capture_and_save_bootfail_log = (void *)bfm_capture_and_save_bootfail_log;
    kthread_run(bfm_process_upper_layer_boot_fail, (void *)pparam, "bfm_process_upper_layer_boot_fail");

    if (DO_NOTHING != suggested_recovery_method)
    {
        bfm_wait_for_compeletion_of_processing_boot_fail();
    }
__out:
    mutex_unlock(&s_process_boot_fail_mutex);
    BFMR_PRINT_EXIT();

    return 0;
}


/**
    @function: int bfmr_set_boot_stage(bfmr_detail_boot_stage_e boot_stage)
    @brief: get current boot stage during boot process.

    @param: boot_stage [in], boot stage to be set.

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_set_boot_stage(bfmr_detail_boot_stage_e boot_stage)
{
    mutex_lock(&s_process_boot_stage_mutex);
    (void)bfm_set_boot_stage(boot_stage);
    if (bfmr_is_boot_success(boot_stage))
    {
        BFMR_PRINT_KEY_INFO("boot success!\n");
        bfm_stop_boot_timer();
        kthread_run(bfm_notify_boot_success, NULL, "bfm_notify_boot_success");
    }
    mutex_unlock(&s_process_boot_stage_mutex);

    return 0;
}


/**
    @function: int bfmr_get_boot_stage(bfmr_detail_boot_stage_e *pboot_stage)
    @brief: get current boot stage during boot process.

    @param: pboot_stage [out], buffer storing the boot stage.

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_get_boot_stage(bfmr_detail_boot_stage_e *pboot_stage)
{
    int ret = -1;
    bfmr_detail_boot_stage_e boot_stage;

    mutex_lock(&s_process_boot_stage_mutex);
    ret = bfm_get_boot_stage(&boot_stage);
    *pboot_stage = boot_stage;
    mutex_unlock(&s_process_boot_stage_mutex);

    return ret;
}


static int bfm_update_info_for_each_log(void)
{
    int ret = -1;
    int fd = -1;
    void *buf = NULL;
    char *full_path = NULL;
    char *target_file_path = NULL;
    struct linux_dirent64 *dirp;
    int num;
    mm_segment_t oldfs;
    char c = '\0';
    char *bfmr_log_root_path = bfm_get_bfmr_log_root_path();
    bfr_real_recovery_info_t real_recovery_info = {0};
    int log_count = bfm_get_log_count(bfm_get_bfmr_log_root_path());
    int index = 0;
    int offset = 0;
    bool get_real_recovery_info_successfully = false;
    
    oldfs = get_fs();
    set_fs(KERNEL_DS);

    if (0 == strlen(bfmr_log_root_path))
    {
        BFMR_PRINT_ERR("get root path of bfmr log failed!\n");
        goto __out;
    }

    fd = sys_open(bfmr_log_root_path, O_RDONLY, 0);
    if (fd < 0)
    {
        BFMR_PRINT_ERR("open [%s] failed!\n", bfmr_log_root_path);
        goto __out;
    }

    buf = bfmr_malloc(BFMR_MAX_PATH);
    if (NULL == buf)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }

    full_path = (char *)bfmr_malloc(BFMR_MAX_PATH);
    if (NULL == full_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }

    target_file_path = (char *)bfmr_malloc(BFMR_MAX_PATH);
    if (NULL == target_file_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }

    /* get the real recovery info firstly */
    memset((void *)&real_recovery_info, 0, sizeof(real_recovery_info));
    get_real_recovery_info_successfully = (0 == bfr_get_real_recovery_info(&real_recovery_info)) ? (true) : (false);
    log_count = (log_count <= 0) ? (0) : (log_count);
    offset = (get_real_recovery_info_successfully) ? ((log_count > real_recovery_info.record_count)
        ? (log_count - real_recovery_info.record_count) : (real_recovery_info.record_count - log_count)) : (-1);

    dirp = buf;
    num = sys_getdents64(fd, dirp, BFMR_MAX_PATH);
    while (num > 0)
    {
        while (num > 0)
        {
            bfm_stat_t st;
            int ret;

            memset(full_path, 0, BFMR_MAX_PATH);
            snprintf(full_path, BFMR_MAX_PATH - 1, "%s/%s", bfmr_log_root_path, dirp->d_name);
            if ((0 == strcmp(dirp->d_name, ".")) || (0 == strcmp(dirp->d_name, "..")))
            {
                goto __continue;
            }

            memset((void *)&st, 0, sizeof(bfm_stat_t));
            ret = bfm_sys_lstat(full_path, &st);
            if (0 != ret)
            {
                BFMR_PRINT_ERR("newlstat %s failed!\n", full_path);
                goto __continue;
            }

            if (!S_ISDIR(st.st_mode))
            {
                BFMR_PRINT_ERR("%s is not a dir!\n", full_path);
                goto __continue;
            }

            /* Note: We must exclude the uploaded dir */
            if (0 == strcmp(dirp->d_name, BFM_UPLOADING_DIR_NAME))
            {
                BFMR_PRINT_ERR("%s must be excluded!\n", full_path);
                goto __continue;
            }

            /* 1. update the recovery method for each log */
            memset((void *)target_file_path, 0, BFMR_MAX_PATH);
            snprintf(target_file_path, BFMR_TEMP_BUF_LEN - 1, "%s/%s", full_path, BFM_RECOVERY_FILE_NAME);
            if (log_count >= real_recovery_info.record_count)
            {
                bfm_update_recovery_result(target_file_path, get_real_recovery_info_successfully
                    ? ((index >= offset) ? (real_recovery_info.recovery_method[index - offset]) : (-1)) : (-1),
                    (index == (log_count - 1)) ? (true) : (false));
            }
            else
            {
                  bfm_update_recovery_result(target_file_path, get_real_recovery_info_successfully
                    ? (real_recovery_info.recovery_method[index + offset]) : (-1),
                    (index == (log_count - 1)) ? (true) : (false));      
            }

            /* 2. create done file for each log */
            memset((void *)target_file_path, 0, BFMR_MAX_PATH);
            snprintf(target_file_path, BFMR_MAX_PATH - 1, "%s/%s", full_path, BFM_DONE_FILE_NAME);
            if (!bfmr_is_file_existed(target_file_path))
            {
                (void)bfmr_save_log(full_path, BFM_DONE_FILE_NAME, (void *)&c, sizeof(char), 0);
            }
            index++;

__continue:
            num -= dirp->d_reclen;
            dirp = (void *)dirp + dirp->d_reclen;
        }
        dirp = buf;
        memset(buf, 0, BFMR_MAX_PATH);
        num = sys_getdents64(fd, dirp, BFMR_MAX_PATH);
    }

__out:
    if (fd >= 0)
    {
        sys_close(fd);
    }
    set_fs(oldfs);

    bfmr_free(buf);
    bfmr_free(full_path);
    bfmr_free(target_file_path);

    return ret;
}


static int bfm_lookup_dir_by_create_time(const char *root,
    char *log_path,
    size_t log_path_len,
    int find_oldest_log)
{
    int fd = -1;
    int num;
    int log_count = 0;
    void *buf = NULL;
    char *full_path = NULL;
    struct linux_dirent64 *dirp;
    mm_segment_t oldfs;
    unsigned long long special_time = 0;
    unsigned long long cur_time = 0;

    if (unlikely(NULL == log_path))
    {
        BFMR_PRINT_INVALID_PARAMS("log_path: %p\n", log_path);
        return -1;
    }

    oldfs = get_fs();
    set_fs(KERNEL_DS);

    memset((void *)log_path, 0, log_path_len);
    fd = sys_open(root, O_RDONLY, 0);
    if (fd < 0)
    {
        BFMR_PRINT_ERR("open [%s] failed!\n", root);
        goto __out;
    }

    buf = bfmr_malloc(BFMR_MAX_PATH);
    if (NULL == buf)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }

    full_path = (char *)bfmr_malloc(BFMR_MAX_PATH);
    if (NULL == full_path)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        goto __out;
    }

    dirp = buf;
    num = sys_getdents64(fd, dirp, BFMR_MAX_PATH);
    while (num > 0)
    {
        while (num > 0)
        {
            bfm_stat_t st;
            int ret;

            memset(full_path, 0, BFMR_MAX_PATH);
            snprintf(full_path, BFMR_MAX_PATH - 1, "%s/%s", root, dirp->d_name);
            if ((0 == strcmp(dirp->d_name, ".")) || (0 == strcmp(dirp->d_name, "..")))
            {
                num -= dirp->d_reclen;
                dirp = (void *)dirp + dirp->d_reclen;
                continue;
            }

            memset((void *)&st, 0, sizeof(bfm_stat_t));
            ret = bfm_sys_lstat(full_path, &st);
            if (0 != ret)
            {
                num -= dirp->d_reclen;
                dirp = (void *)dirp + dirp->d_reclen;
                BFMR_PRINT_ERR("newlstat %s failed!\n", full_path);
                continue;
            }

            if (!S_ISDIR(st.st_mode))
            {
                num -= dirp->d_reclen;
                dirp = (void *)dirp + dirp->d_reclen;
                BFMR_PRINT_ERR("%s is not a dir!\n", full_path);
                continue;
            }

            /* Note: We must exclude the uploaded dir */
            if (0 == strcmp(dirp->d_name, BFM_UPLOADING_DIR_NAME))
            {
                num -= dirp->d_reclen;
                dirp = (void *)dirp + dirp->d_reclen;
                BFMR_PRINT_ERR("%s must be excluded!\n", full_path);
                continue;
            }

            cur_time = (unsigned long long)st.st_mtime * BFM_US_PERSECOND + st.st_mtime_nsec / 1000;
            if (0 == special_time)
            {
                strncpy(log_path, full_path, log_path_len - 1);
                special_time = cur_time;
            }
            else
            {
                if (0 != find_oldest_log)
                {
                    if (special_time > cur_time)
                    {
                        strncpy(log_path, full_path, log_path_len - 1);
                        special_time = cur_time;
                    }
                }
                else
                {
                    if (special_time < cur_time)
                    {
                        strncpy(log_path, full_path, log_path_len - 1);
                        special_time = cur_time;
                    }
                }
            }
            log_count++;
            num -= dirp->d_reclen;
            dirp = (void *)dirp + dirp->d_reclen;
        }
        dirp = buf;
        memset(buf, 0, BFMR_MAX_PATH);
        num = sys_getdents64(fd, dirp, BFMR_MAX_PATH);
    }

__out:
    if (fd >= 0)
    {
        sys_close(fd);
    }
    set_fs(oldfs);

    BFMR_PRINT_KEY_INFO("Find %d log in %s, the %s log path is: %s\n", log_count, root,
        (0 == find_oldest_log) ? ("newest") : ("oldest"), log_path);

    bfmr_free(buf);
    bfmr_free(full_path);

    return 0;
}


static int bfm_find_newest_log(char *log_path, size_t log_path_len)
{
    int ret = -1;

    if (unlikely((NULL == log_path)))
    {
        BFMR_PRINT_INVALID_PARAMS("log_path_len: %p\n", log_path);
        return -1;
    }

    ret = bfm_lookup_dir_by_create_time(bfm_get_bfmr_log_root_path(), log_path, log_path_len, 0);
    if (0 == strlen(log_path))
    {
        return -1;
    }
    else
    {
        BFMR_PRINT_ERR("The newest log is: %s\n", log_path);
    }

    return 0;
}


static int bfm_read_recovery_info(const char *precovery_info_file_path,
    char *data_buf, long data_buf_len)
{
    mm_segment_t old_fs;
    int fd = -1;
    long bytes_read = 0;
    int ret = -1;

    if (unlikely((NULL == precovery_info_file_path) || (NULL == data_buf)))
    {
        BFMR_PRINT_INVALID_PARAMS("precovery_info_file_path: %p data_buf: %p\n", precovery_info_file_path, data_buf);
        return -1;
    }

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    fd = sys_open(precovery_info_file_path, O_RDONLY, 0);
    if (fd < 0)
    {
        BFMR_PRINT_ERR("Open [%s] failed!fd: %d\n", precovery_info_file_path, fd);
        goto __out;
    }

    bytes_read = bfmr_full_read(fd, data_buf, data_buf_len);
    if (bytes_read != data_buf_len)
    {
        BFMR_PRINT_ERR("read [%s] failed!bytes_read: %ld, it should be: %ld\n",
            precovery_info_file_path, bytes_read, data_buf_len);
        goto __out;
    }
    else
    {
        ret = 0;
    }

__out:
    if (fd >= 0)
    {
        sys_close(fd);
    }

    set_fs(old_fs);

    return ret;
}


static bool bfm_is_valid_recovery_info(char *pdata)
{
    char *ptemp = NULL;

    if (unlikely((NULL == pdata)))
    {
        BFMR_PRINT_INVALID_PARAMS("pdata: %p\n", pdata);
        return false;
    }

    ptemp = strstr(pdata, BFM_RECOVERY_RESULT_FIELD);
    if (NULL == ptemp)
    {
        return false;
    }

    ptemp = strstr(ptemp + strlen(BFM_RECOVERY_RESULT_FIELD), BFM_RECOVERY_RESULT_FIELD);
    if (NULL == ptemp)
    {
        return false;
    }

    ptemp = strstr(ptemp, BFM_LOG_END_TAG_PER_LINE);
    if (NULL == ptemp)
    {
        return false;
    }

    return true;
}


static int bfm_update_recovery_result(char *precovery_info_file_path,
    int real_recovery_method,
    bool boot_success)
{
    int fd = -1;
    int ret = -1;
    int recovery_method = -1;
    char *pfile_data = NULL;
    char *ppart_file_data = NULL;
    char *ptemp = NULL;
    long file_len = 0L;
    long bytes_write = 0L;
    long bytes_to_write = 0L;
    mm_segment_t oldfs;

    if (unlikely((NULL == precovery_info_file_path)))
    {
        BFMR_PRINT_INVALID_PARAMS("precovery_info_file_path: %p\n", precovery_info_file_path);
        return -1;
    }

    oldfs = get_fs();
    set_fs(KERNEL_DS);

    file_len = bfmr_get_file_length(precovery_info_file_path);
    pfile_data = (char *)bfmr_malloc(file_len + 1);
    if (NULL == pfile_data)
    {
        BFMR_PRINT_ERR("%s(): bfmr_malloc failed!\n", __func__);
        goto __out;
    }
    memset((void *)pfile_data, 0, file_len + 1);

    if (0 != bfm_read_recovery_info(precovery_info_file_path, pfile_data, file_len))
    {
        BFMR_PRINT_ERR("Read [%s] failed!\n", precovery_info_file_path);
        goto __out;
    }

    /* 1. find the last fileld "rcvMethod:" */
    ppart_file_data = (char *)bfmr_malloc(BFMR_TEMP_BUF_LEN);
    if (NULL == ppart_file_data)
    {
        BFMR_PRINT_ERR("%s(): bfmr_malloc failed!\n", __func__);
        goto __out;
    }
    memset(ppart_file_data, 0, BFMR_TEMP_BUF_LEN);

    /* 2. check if the log is valid */
    if (!bfm_is_valid_recovery_info(pfile_data))
    {
        BFMR_PRINT_ERR("[%s] has invalid data:\n%s\n", precovery_info_file_path, pfile_data);
        recovery_method = real_recovery_method;
        goto __save_log;
    }

    /* 3. check if the boot fail has been recoveried successfully? */
    if (NULL != strstr(pfile_data, BFM_RECOVERY_SUCCESS_FIELD))
    {
        ret = 0;
        BFMR_PRINT_KEY_INFO("recovery success result has been set!\n");
        goto __out;
    }

    /* 4. check if the user has changed the recovery method */
    ptemp = bfmr_reverse_find_string(pfile_data, BFM_RECOVERY_METHOD_FIELD);
    if (NULL == ptemp)
    {
        BFMR_PRINT_ERR("Invalid file data:\n%s\n in file [%s]!\n", pfile_data, precovery_info_file_path);
        goto __out;
    }

    recovery_method = (int)simple_strtol(ptemp + strlen(BFM_RECOVERY_METHOD_FIELD), NULL, 10);
    if ((recovery_method == real_recovery_method) && (!boot_success))
    {
        /* if this is not the last boot fail and the recovery method is the same, go to out */
        ret = 0;
        BFMR_PRINT_KEY_INFO("recovery_method: %d, real_recovery_method: %d, boot_success: %d\n",
            recovery_method, real_recovery_method, boot_success);
        goto __out;
    }
    recovery_method = (-1 == real_recovery_method) ? (recovery_method) : (real_recovery_method);

__save_log:
    (void)snprintf(ppart_file_data, BFMR_TEMP_BUF_LEN - 1, BFM_RCV_FILE_CONTENT_FORMAT,
        bfr_get_recovery_method_desc(recovery_method), (boot_success ? BFM_RECOVERY_SUCCESS_STR : BFM_RECOVERY_FAIL_STR),
        recovery_method, (boot_success ? BFM_RECOVERY_SUCCESS_INT_VALUE : BFM_RECOVERY_FAIL_INT_VALUE));
    fd = sys_open(precovery_info_file_path, O_RDWR | O_TRUNC, 0);
    if (fd < 0)
    {
        BFMR_PRINT_ERR("sys_open [%s] failed![fd = %d]\n", precovery_info_file_path, fd);
        goto __out;
    }

    bytes_to_write = (long)strlen(ppart_file_data);
    bytes_write = bfmr_full_write(fd, ppart_file_data, bytes_to_write);
    if (bytes_write != bytes_to_write)
    {
        BFMR_PRINT_ERR("bfmr_full_write [%s] failed!bytes_write:%ld, it should be:%ld\n",
            precovery_info_file_path, bytes_write, bytes_to_write);
        goto __out;
    }

    ret = 0;
    bfmr_change_own_mode(precovery_info_file_path, BFMR_AID_ROOT, BFMR_AID_SYSTEM, BFMR_FILE_LIMIT);

__out:
    if (fd >= 0)
    {
        sys_fsync(fd);
        sys_close(fd);
    }

    set_fs(oldfs);

    bfmr_free(pfile_data);
    bfmr_free(ppart_file_data);

    return ret;
}


static int bfm_notify_boot_success(void *param)
{
    /* 1. notify boot success event to the BFR */
    boot_status_notify(1);

    /* 2. let platfrom process the boot success */
    bfm_platform_process_boot_success();

    /* 3. update recovery result in recovery_info.txt */
    mutex_lock(&s_process_boot_fail_mutex);
    bfm_update_info_for_each_log();
    mutex_unlock(&s_process_boot_fail_mutex);


    /* 4. check bootup slowly event */
    unsigned int bootup_time = bfmr_get_bootup_time();
    if ((bootup_time >= BFM_BOOTUP_SLOWLY_THRESHOLD_IN_SECOND) && (!s_is_bootup_successfully))
    {
        BFMR_PRINT_ERR("bootup time[%dS] is too long.\n", bootup_time);
        boot_fail_err(BOOTUP_SLOWLY, DO_NOTHING, NULL);
    }

    s_is_bootup_successfully = true;
    return 0;
}


/**
    @function: int bfmr_get_timer_state(int *state)
    @brief: get state of the timer.

    @param: state [out], the state of the boot timer.

    @return: 0 - success, -1 - failed.

    @note:
        1. this fuction only need be initialized in kernel.
        2. if *state == 0, the boot timer is disabled, if *state == 1, the boot timer is enbaled.
*/
int bfmr_get_timer_state(int *state)
{
    return bfm_get_boot_timer_state(state);
}


/**
    @function: int bfm_enable_timer(void)
    @brief: enbale timer.

    @param: none.

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_enable_timer(void)
{
    return bfmr_resume_boot_timer();
}


/**
    @function: int bfm_disable_timer(void)
    @brief: disable timer.

    @param: none.

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_disable_timer(void)
{
    return bfm_suspend_boot_timer();
}


/**
    @function: int bfm_set_timer_timeout_value(unsigned int timeout)
    @brief: set timeout value of the kernel timer. Note: the timer which control the boot procedure is in the kernel.

    @param: timeout [in] timeout value (unit: msec).

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_set_timer_timeout_value(unsigned int timeout)
{
    return bfm_set_boot_timer_timeout_value(timeout);
}


/**
    @function: int bfm_get_timer_timeout_value(unsigned int *timeout)
    @brief: get timeout value of the kernel timer. Note: the timer which control the boot procedure is in the kernel.

    @param: timeout [in] buffer will store the timeout value (unit: msec).

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_get_timer_timeout_value(unsigned int *timeout)
{
    return bfm_get_boot_timer_timeout_value(timeout);
}


/**
    @function: int bfmr_action_timer_ctl(struct action_ioctl_data *pact_data)
    @brief: acttion timers opertion

    @param: pact_data  ationt ctl data

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
static int bfmr_action_timer_ctl(struct action_ioctl_data *pact_data)
{
    int ret = -EPERM;
    switch (pact_data->op)
    {
    case ACT_TIMER_START:
        {
            ret = bfm_action_timer_start(pact_data->action_name, pact_data->action_timer_timeout_value);
            break;
        }
    case ACT_TIMER_STOP:
        {
            ret = bfm_action_timer_stop(pact_data->action_name);
            break;
        }
    case ACT_TIMER_PAUSE:
        {
            ret = bfm_action_timer_pause(pact_data->action_name);
            break;
        }
    case ACT_TIMER_RESUME:
        {
            ret = bfm_action_timer_resume(pact_data->action_name);
            break;
        }
    default:
        {
            break;
        }
    }

    return ret;
}

static unsigned long long bfm_get_system_time(void)
{
    struct timeval tv = {0};

    do_gettimeofday(&tv);

    return (unsigned long long)tv.tv_sec;
}


/**
    @function: int bfmr_capture_and_save_log(bfmr_log_type_e type, bfmr_log_dst_t *dst)
    @brief: capture and save log, initialised in core.

    @param: src [in] log src info.
    @param: dst [in] infomation about the media of saving log.

    @return: 0 - succeeded; -1 - failed.

    @note: it need be initialized in bootloader and kernel.
*/
static int bfmr_capture_and_save_log(bfmr_log_src_t *src, bfmr_log_dst_t *dst, bfm_process_bootfail_param_t *pparam)
{
    int i = 0;
    bool is_valid_log_type = false;
    int count = sizeof(s_log_type_buffer_info) / sizeof(s_log_type_buffer_info[0]);
    unsigned int bytes_read = 0;

    if (unlikely((NULL == src) || (NULL == dst) || (NULL == pparam)))
    {
        BFMR_PRINT_INVALID_PARAMS("src: %p, dst: %p, pparam: %p\n", (void *)src, (void *)dst, (void *)pparam);
        return -1;
    }

    for (i = 0; i < count; i++)
    {
        if (src->log_type != s_log_type_buffer_info[i].log_type)
        {
            continue;
        }

        s_log_type_buffer_info[i].buf = bfmr_malloc(s_log_type_buffer_info[i].buf_len + 1);
        if (NULL == s_log_type_buffer_info[i].buf)
        {
            BFMR_PRINT_ERR("bfmr_malloc failed!\n");
            return -1;
        }
        memset(s_log_type_buffer_info[i].buf, 0, s_log_type_buffer_info[i].buf_len + 1);
        is_valid_log_type = true;
        break;
    }

    if (!is_valid_log_type)
    {
        BFMR_PRINT_ERR("Invalid log type: [%d]\n", (int)(src->log_type));
        return -1;
    }

    bytes_read = bfmr_capture_log_from_system(s_log_type_buffer_info[i].buf,
        s_log_type_buffer_info[i].buf_len, src, 0);
    switch (dst->type)
    {
    case DST_FILE:
        {
            bfmr_save_log_to_fs(dst->dst_info.filename, s_log_type_buffer_info[i].buf, bytes_read, 0);
            break;
        }
    case DST_RAW_PART:
        {
            if (dst->dst_info.raw_part.offset >=0) 
            {
                bfmr_save_log_to_raw_part(dst->dst_info.raw_part.raw_part_name,
                    (unsigned long long)dst->dst_info.raw_part.offset,
                    s_log_type_buffer_info[i].buf, bytes_read);
                bfmr_update_raw_log_info(src, dst, bytes_read);
            }
            else
            {
                BFMR_PRINT_ERR("dst->dst_info.raw_part.offset is negative [%d]\n", dst->dst_info.raw_part.offset);
            }
            break;
        }
    case DST_MEMORY_BUFFER:
    default:
        {
            bfmr_save_log_to_mem_buffer(dst->dst_info.buffer.addr, dst->dst_info.buffer.len, s_log_type_buffer_info[i].buf, bytes_read);
            break;
        }
    }

    if ((CRITICAL_SERVICE_FAIL_TO_START == pparam->bootfail_errno) && (LOG_TYPE_TEXT_KMSG == src->log_type))
    {
        bfmr_get_critical_process_name(s_log_type_buffer_info[i].buf, s_log_type_buffer_info[i].buf_len,
            pparam->critical_process_name, sizeof(pparam->critical_process_name));
    }

    bfmr_free(s_log_type_buffer_info[i].buf);
    s_log_type_buffer_info[i].buf = NULL;

    return 0;
}


static long bfmr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    unsigned int timeout_value = -1;
    bfmr_detail_boot_stage_e boot_stage;
    struct bfmr_dev_path *dev_path = NULL;
    struct bfmr_boot_fail_info *pboot_fail_info = NULL;
    struct action_ioctl_data *pact_data = NULL;

    if (!bfmr_has_been_enabled())
    {
        BFMR_PRINT_KEY_INFO("BFMR is disabled!\n");
        return -EPERM;
    }

    if (NULL == (void *)arg)
    {
        BFMR_PRINT_INVALID_PARAMS("arg: %p\n", (void *)arg);
        return -EINVAL;
    }

    switch (cmd)
    {
    case BFMR_GET_TIMER_STATUS:
        {
            int state;

            bfmr_get_timer_state(&state);
            copy_to_user((int *)arg, &state, sizeof(state));
            BFMR_PRINT_KEY_INFO("short timer stats is: %d\n", state);
            break;
        }
    case BFMR_ENABLE_TIMER:
        {
            bfmr_enable_timer();
            break;
        }
    case BFMR_DISABLE_TIMER:
        {
            bfmr_disable_timer();
            break;
        }
    case BFMR_GET_TIMER_TIMEOUT_VALUE:
        {
            bfmr_get_timer_timeout_value(&timeout_value);
            copy_to_user((int *)arg, &timeout_value, sizeof(timeout_value));
            BFMR_PRINT_KEY_INFO("short timer timeout value is: %d\n", timeout_value);
            break;
        }
    case BFMR_SET_TIMER_TIMEOUT_VALUE:
        {
            copy_from_user(&timeout_value, (int *)arg, sizeof(timeout_value));
            bfmr_set_timer_timeout_value(timeout_value);
            BFMR_PRINT_KEY_INFO("set short timer timeout value to: %d\n", timeout_value);
            break;
        }
    case BFMR_GET_BOOT_STAGE:
        {
            bfmr_get_boot_stage(&boot_stage);
            copy_to_user((bfmr_detail_boot_stage_e *)arg, &boot_stage, sizeof(boot_stage));
            BFMR_PRINT_KEY_INFO("bfmr_bootstage is: 0x%08x\n", (unsigned int)boot_stage);
            break;
        }
    case BFMR_SET_BOOT_STAGE:
        {
            bfmr_detail_boot_stage_e old_boot_stage;

            bfmr_get_boot_stage(&old_boot_stage);
            copy_from_user(&boot_stage, (int *)arg, sizeof(boot_stage));
            BFMR_PRINT_KEY_INFO("set bfmr_bootstage to: 0x%08x\n", (unsigned int)boot_stage);
            bfmr_set_boot_stage(boot_stage);

            break;
        }
    case BFMR_PROCESS_BOOT_FAIL:
        {
            pboot_fail_info = (struct bfmr_boot_fail_info *)bfmr_malloc(sizeof(struct bfmr_boot_fail_info));
            if (NULL == pboot_fail_info)
            {
                BFMR_PRINT_ERR("bfmr_malloc failed!\n");
                ret = -ENOMEM;
                break;
            }
            memset((void *)pboot_fail_info, 0, sizeof(struct bfmr_boot_fail_info));
            copy_from_user(pboot_fail_info, ((struct bfmr_boot_fail_info *)arg), sizeof(struct bfmr_boot_fail_info));
            pboot_fail_info->log_path[sizeof(pboot_fail_info->log_path) - 1] = '\0';
            BFMR_PRINT_KEY_INFO("bootfail_errno: 0x%08x, suggested_recovery_method: %d, log_file [%s]'s lenth:%ld\n",
                (unsigned int)pboot_fail_info->boot_fail_no, (int)pboot_fail_info->suggested_recovery_method,
                pboot_fail_info->log_path, bfmr_get_file_length(pboot_fail_info->log_path));
            (void)boot_fail_err(pboot_fail_info->boot_fail_no, pboot_fail_info->suggested_recovery_method, pboot_fail_info->log_path);
            bfmr_free(pboot_fail_info);
            break;
        }
    case BFMR_GET_DEV_PATH:
        {
            break;
        }
    case BFMR_ENABLE_CTRL:
        {
            int bfmr_enbale_flag;
            if (0 != copy_from_user(&bfmr_enbale_flag, (int *)arg, sizeof(int)))
            {
                BFMR_PRINT_KEY_INFO("Failed to copy bfmr enable flag from user!\n");
                ret = -EFAULT;
                break;
            }
            BFMR_PRINT_KEY_INFO("set bfmr enable flag: 0x%08x\n", (unsigned int)bfmr_enbale_flag);
            bfmr_enable_ctl(bfmr_enbale_flag);  
            break;
        }
    case BFMR_ACTION_TIMER_CTL:
        {
            pact_data = (struct action_ioctl_data *)bfmr_malloc(sizeof(struct action_ioctl_data));
            if (NULL == pact_data) {
                BFMR_PRINT_ERR("bfmr_malloc failed!\n");
                ret = -ENOMEM;
                break;
            }

            if (0 != copy_from_user(pact_data, arg, sizeof(struct action_ioctl_data))) {
                BFMR_PRINT_KEY_INFO("Failed to copy acttion_ioctl_data from user buffer!\n");
                bfmr_free(pact_data);
                ret = -EFAULT;
                break;
            }

            pact_data->action_name[BFMR_ACTION_NAME_LEN-1] = '\0';
            BFMR_PRINT_KEY_INFO("set bfmr action timer: %x, %s, %d\n",
                       pact_data->op, pact_data->action_name, pact_data->action_timer_timeout_value);
            ret = bfmr_action_timer_ctl(pact_data);
            bfmr_free(pact_data);
            break;
        }
    default:
        {
            BFMR_PRINT_ERR("Invalid CMD: 0x%x\n", cmd);
            ret = -EFAULT;
            break;
        }
    }

    return ret;
}


static int bfmr_open(struct inode *inode, struct file *file)
{
    return nonseekable_open(inode, file);
}


static ssize_t bfmr_write(struct file *file, const char *data, size_t len, loff_t *ppos)
{
    return len;
}


static ssize_t bfmr_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
    return count;
}


static int bfmr_release(struct inode *inode, struct file *file)
{
    return 0;
}


static int bfm_process_bottom_layer_boot_fail(void *param)
{
    bfmr_log_src_t src;
    int i = 0;
    int ret = -1;
    int count = sizeof(s_log_type_buffer_info) / sizeof(s_log_type_buffer_info[0]);
    unsigned int bytes_read = 0U;
    bool find_log_type_buffer_info = false;

    mutex_lock(&s_process_boot_fail_mutex);
    memset((void *)&src, 0, sizeof(src));
    src.log_type = LOG_TYPE_BFMR_TEMP_BUF;
    for (i = 0; i < count; i++)
    {
        if (LOG_TYPE_BFMR_TEMP_BUF != s_log_type_buffer_info[i].log_type)
        {
            continue;
        }

        /* update the buf_len firstly */
        s_log_type_buffer_info[i].buf_len = bfm_get_dfx_log_length();
        s_log_type_buffer_info[i].buf = bfmr_malloc(s_log_type_buffer_info[i].buf_len + 1);
        if (NULL == s_log_type_buffer_info[i].buf)
        {
            BFMR_PRINT_ERR("bfmr_malloc failed!\n");
            goto __out;
        }
        memset(s_log_type_buffer_info[i].buf, 0, s_log_type_buffer_info[i].buf_len + 1);
        find_log_type_buffer_info = true;
        break;
    }
    if (!find_log_type_buffer_info)
    {
        BFMR_PRINT_ERR("Can not find log buffer info for \"LOG_TYPE_BFMR_TEMP_BUF\"\n");
        goto __out;
    }

    bytes_read = bfmr_capture_log_from_system(s_log_type_buffer_info[i].buf,
        s_log_type_buffer_info[i].buf_len, &src, 0);
    if (0U == bytes_read)
    {
        ret = 0;
        BFMR_PRINT_ERR("There is no bottom layer bootfail log!\n");
        goto __out;
    }

    ret = bfm_parse_and_save_bottom_layer_bootfail_log(
        (bfm_process_bootfail_param_t *)param,
        s_log_type_buffer_info[i].buf,
        s_log_type_buffer_info[i].buf_len);
    if (0 != ret)
    {
        BFMR_PRINT_ERR("Failed to save bottom layer bootfail log!\n");
        goto __out;      
    }

__out:
    if (NULL != s_log_type_buffer_info[i].buf)
    {
        bfmr_free(s_log_type_buffer_info[i].buf);
        s_log_type_buffer_info[i].buf = NULL;
    }
    bfmr_free(param);
    mutex_unlock(&s_process_boot_fail_mutex);

    return ret;
}


static int bfmr_capture_and_save_bottom_layer_boot_fail_log(void)
{
    struct task_struct *tsk;
    bfm_process_bootfail_param_t *pparam = NULL;

    pparam = (bfm_process_bootfail_param_t *)bfmr_malloc(sizeof(bfm_process_bootfail_param_t));
    if (NULL == pparam)
    {
        BFMR_PRINT_ERR("bfmr_malloc failed!\n");
        return -1;
    }
    memset((void *)pparam, 0, sizeof(bfm_process_bootfail_param_t));

    tsk = kthread_run(bfm_process_bottom_layer_boot_fail, (void *)pparam, "save_bl_bf_log");
    if (IS_ERR(tsk))
    {
        BFMR_PRINT_ERR("Failed to create thread to save bottom layer bootfail log!\n");
        return -1;
    }

    return 0;
}


/**
    @function: int bfm_init(void)
    @brief: init bfm module in kernel.

    @param: none.

    @return: 0 - succeeded; -1 - failed.

    @note: it need be initialized in bootloader and kernel.
*/
int bfm_init(void)
{
    int ret = -1;
    bfm_chipsets_init_param_t chipsets_init_param;

    if (!bfmr_has_been_enabled())
    {
        BFMR_PRINT_KEY_INFO("BFMR is disabled!\n");
        return 0;
    }

    ret = misc_register(&s_bfmr_miscdev);
    if (0 != ret)
    {
        BFMR_PRINT_ERR("misc_register failed, ret: %d.\n", ret);
        return ret;
    }

    (void)bfm_init_boot_timer();
    memset((void *)&chipsets_init_param, 0, sizeof(bfm_chipsets_init_param_t));
    chipsets_init_param.log_saving_param.capture_and_save_bootfail_log = bfm_capture_and_save_bootfail_log;
    bfm_chipsets_init(&chipsets_init_param);
    bfmr_capture_and_save_bottom_layer_boot_fail_log();

    return 0;
}

