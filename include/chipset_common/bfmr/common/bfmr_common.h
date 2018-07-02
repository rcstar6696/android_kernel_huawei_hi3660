/**
    @copyright: Huawei Technologies Co., Ltd. 2016-xxxx. All rights reserved.

    @file: bfmr_common.h

    @brief: define the common external public enum/macros/interface for BFMR (Boot Fail Monitor and Recovery)

    @version: 2.0

    @author: QiDechun ID: 216641

    @date: 2016-08-17

    @history:
*/

#ifndef BFMR_COMMON_H
#define BFMR_COMMON_H


/*----includes-----------------------------------------------------------------------*/

#include <linux/vmalloc.h>
#include <linux/types.h>


/*----c++ support-------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif


/*----export prototypes---------------------------------------------------------------*/

typedef enum
{
    LOG_TYPE_BOOTLOADER_1 = 0,
    LOG_TYPE_BOOTLOADER_2,
    LOG_TYPE_BFMR_TEMP_BUF,
    LOG_TYPE_KMSG,
    LOG_TYPE_ANDROID_KMSG = LOG_TYPE_KMSG,
    LOG_TYPE_TEXT_KMSG,
    LOG_TYPE_RAMOOPS,
    LOG_TYPE_BETA_APP_LOGCAT,
    LOG_TYPE_CRITICAL_PROCESS_CRASH,
    LOG_TYPE_VM_TOMBSTONES,
    LOG_TYPE_VM_CRASH,
    LOG_TYPE_VM_WATCHDOG,
    LOG_TYPE_NORMAL_FRAMEWORK_BOOTFAIL_LOG,
    LOG_TYPE_FIXED_FRAMEWORK_BOOTFAIL_LOG,
    LOG_TYPE_BFM_BFI_LOG,
    LOG_TYPE_BFM_RECOVERY_LOG,
    LOG_TYPE_MAX_COUNT
} bfmr_log_type_e;

typedef enum
{
    DST_FILE = 0,
    DST_RAW_PART,
    DST_MEMORY_BUFFER,
} bfmr_dst_type_e;

typedef struct
{
    bfmr_dst_type_e type;

    union
    {
        char *filename;
        struct
        {
            char *addr;
            int len;
        } buffer;
        struct
        {
            char *raw_part_name;
            int offset;
        } raw_part;
    } dst_info;
} bfmr_log_dst_t;

typedef struct
{
    bfmr_log_type_e log_type;
    char *src_log_file_path;
    void *log_save_context;
    void *log_save_additional_context;
} bfmr_log_src_t;

#if BITS_PER_LONG == 32
            typedef   struct stat64   bfm_stat_t;
            #define   bfm_sys_lstat sys_lstat64
#else
            typedef   struct stat bfm_stat_t;
            #define   bfm_sys_lstat sys_newlstat
#endif

/*----export macroes-----------------------------------------------------------------*/

#define BFMR_AID_ROOT 0
#define BFMR_AID_SYSTEM 1000

#define BFMR_DIR_LIMIT 0775
#define BFMR_FILE_LIMIT 0664

#ifdef __KERNEL__
#define bfmr_malloc vmalloc
#define bfmr_free vfree
#else
#define bfmr_malloc malloc
#define bfmr_free free
#endif

#define BFMR_SIZE_32 ((unsigned int)32)
#define BFMR_SIZE_64 ((unsigned int)64)
#define BFMR_SIZE_128 ((unsigned int)128)
#define BFMR_SIZE_256 ((unsigned int)256)
#define BFMR_SIZE_512 ((unsigned int)512)
#define BFMR_SIZE_1K ((unsigned int)1024)
#define BFMR_SIZE_2K ((unsigned int)2048)
#define BFMR_SIZE_4K ((unsigned int)4096)

#define BFMR_DEV_FULL_PATH_MAX_LEN BFMR_SIZE_256
#define BFMR_TEMP_BUF_LEN BFMR_SIZE_4K
#define BFMR_MAX_PATH BFMR_SIZE_4K

#define ENTER_ERECOVERY_BY_PRESS_KEY (2001)
#define ENTER_ERECOVERY_BY_ADB_CMD (2002)
#define ENTER_ERECOVERY_BECAUSE_UNLOCK (2004)
#define ENTER_ERECOVERY_BECAUSE_SYSTEM_DAMAGED (2005)
#define ENTER_ERECOVERY_BECAUSE_HUAWEI_VERIFY_FAILED (2006)
#define ENTER_ERECOVERY_BECAUSE_HUAWEI_GOOGLE_VERIFY_FAILED (2007)
#define ENTER_ERECOVERY_REASON_BECAUSE_BOOTLOADER_BOOT_FAIL (2008)
#define ENTER_ERECOVERY_REASON_BECAUSE_KERNEL_BOOT_FAIL (2009)
#define ENTER_ERECOVERY_BECAUSE_SYSTEM_MOUNT_FAILED (2010)
#define ENTER_ERECOVERY_BECAUSE_DATA_MOUNT_FAILED (2011)
#define ENTER_ERECOVERY_BECAUSE_DATA_MOUNT_RO (2012)
#define ENTER_ERECOVERY_BECAUSE_KEY_PROCESS_START_FAILED (2013)
#define ENTER_ERECOVERY_BECAUSE_RECOVERY_PROCESS_CRASH (2014)
#define ENTER_ERECOVERY_BECAUSE_AP_CRASH_REPEATEDLY (2015)
#define ENTER_ERECOVERY_BECAUSE_NON_AP_CRASH_REPEATEDLY (2016)
#define ENTER_ERECOVERY_BECAUSE_APP_BOOT_FAIL (2017)
#define ENTER_ERECOVERY_BECAUSE_SECURITY_FAIL (2018)
#define ENTER_ERECOVERY_BECAUSE_VENDOR_MOUNT_FAILED (2019)
#define ENTER_ERECOVERY_BECAUSE_CUST_MOUNT_FAILED (2020)
#define ENTER_ERECOVERY_BECAUSE_PRODUCT_MOUNT_FAILED (2021)
#define ENTER_ERECOVERY_BECAUSE_VERSION_MOUNT_FAILED (2022)
#define ENTER_ERECOVERY_UNKNOWN (2099)

#define BFMR_PRINT_INVALID_PARAMS(format, ...) do {printk(KERN_ERR "func: %s line: %d invalid parameters: " format, __func__, __LINE__, ##__VA_ARGS__);} while (0)
#define BFMR_PRINT_ERR(format, ...)  do {printk(KERN_ERR "func: %s line: %d, " format, __func__, __LINE__, ##__VA_ARGS__);} while (0)
#define BFMR_PRINT_KEY_INFO(format, ...)  do {printk(KERN_ERR "func: %s line: %d, " format, __func__, __LINE__, ##__VA_ARGS__);} while (0)
#define BFMR_PRINT_SIMPLE_INFO(args...)  do {printk(KERN_ERR args);} while (0)
#define BFMR_PRINT_DBG(format, ...) do {printk(KERN_DEBUG "func: %s line: %d, " format, __func__, __LINE__, ##__VA_ARGS__);} while (0)
#define BFMR_PRINT_ENTER(format, ...) \
do\
{\
    printk(KERN_INFO ">>>>enter func: %s, line: %d.\n" format, __func__, __LINE__, ##__VA_ARGS__);\
} while (0)

#define BFMR_PRINT_EXIT(format, ...) \
do\
{\
    printk(KERN_INFO "<<<<exit func: %s, line: %d.\n" format, __func__, __LINE__, ##__VA_ARGS__);\
} while (0)

#define BFMR_BOOTLOCK_FIELD_NAME "bootlock"
#define BFMR_ENABLE_FIELD_NAME "hw_bfm_enable"
#define BFR_RRECORD_PART_NAME "rrecord"
#define BFR_RRECORD_PART_MAX_COUNT (2)
#define BFMR_DEV_NAME "hw_bfm"
#define BFMR_DEV_PATH "/dev/hw_bfm"
#define BFM_LOG_MAX_COUNT (10)


/*----global variables----------------------------------------------------------------*/


/*----export function prototypes--------------------------------------------------------*/

void bfmr_create_crc32_table(void);
unsigned int bfmr_get_crc32(unsigned char *pbuf, unsigned int data_len);

/**
    @function: int bfm_get_device_full_path(char *dev_name, char *path_buf, unsigned int path_buf_len)
    @brief: get full path of the "dev_name".

    @param: dev_name [in] device name such as: boot/recovery/rrecord.
    @param: path_buf [out] buffer will store the full path of "dev_name".
    @param: path_buf_len [in] length of the path_buf.

    @return: 0 - succeeded; -1 - failed.

    @note:
*/
int bfmr_get_device_full_path(char *dev_name, char *path_buf, unsigned int path_buf_len);
int bfmr_read_emmc_raw_part(const char *dev_path,
    unsigned long long offset,
    char *buf,
    unsigned long long buf_size);
int bfmr_write_emmc_raw_part(const char *dev_path,
    unsigned long long offset,
    char *buf,
    unsigned long long buf_size);
void bfmr_change_own_mode(char *path, int uid, int gid, int mode);
void bfmr_change_file_ownership(char *path, uid_t uid, gid_t gid);
void bfmr_change_file_mode(char *path, umode_t mode);
int bfmr_get_file_ownership(char *pfile_path, uid_t *puid, gid_t *pgid);
bool bfmr_is_file_existed(char *pfile_path);
bool bfmr_is_dir_existed(char *pdir_path);
int bfmr_save_log(char *logpath, char *filename, void *buf, unsigned int len, unsigned int is_append);
long bfmr_get_proc_file_length(const char *pfile_path);
bool bfmr_is_part_mounted_rw(const char *pmount_point);
long bfmr_get_file_length(const char *pfile_path);
long long bfmr_get_fs_available_space(const char *pmount_point);
bool bfmr_is_part_ready_without_timeout(char *dev_name);
int bfmr_wait_for_part_mount_without_timeout(const char *pmount_point);
int bfmr_wait_for_part_mount_with_timeout(const char *pmount_point, int timeouts);
int bfmr_create_log_path(char *path);
char* bfmr_convert_rtc_time_to_asctime(unsigned long long rtc_time);
char* bfm_get_bootlock_value_from_cmdline(void);
bool bfmr_has_been_enabled(void);
void bfmr_enable_ctl(int enable_flag);
char* bfmr_reverse_find_string(const char *psrc, const char *pstr_to_be_found);
bool bfm_get_symbol_link_path(char *file_path, char *psrc_path, size_t src_path_size);
long bfmr_full_read(int fd, char *buf, size_t buf_size);
long bfmr_full_write(int fd, char *buf, size_t buf_size);
long bfmr_full_read_with_file_path(const char *pfile_path, char *buf, size_t buf_size);
long bfmr_full_write_with_file_path(const char *pfile_path, char *buf, size_t buf_size);
void bfmr_unlink_file(char *pfile_path);
int bfmr_get_uid_gid(uid_t *puid, gid_t *pgid);
unsigned int bfmr_get_bootup_time(void);
int bfmr_common_init(void);

#ifdef __cplusplus
}
#endif

#endif

