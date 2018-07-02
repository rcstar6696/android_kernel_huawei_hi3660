#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/stat.h>
#include <linux/spinlock.h>
#include <linux/notifier.h>
#include <linux/kprobes.h>
#include <linux/reboot.h>
#include <linux/io.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <asm/barrier.h>
#include <linux/platform_device.h>
#include <linux/of_fdt.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/of_address.h>
#include <linux/kallsyms.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/syscalls.h>
#include <chipset_common/bfmr/public/bfmr_public.h>
#include <chipset_common/bfmr/bfm/core/bfm_core.h>
#include <chipset_common/bfmr/bfm/chipsets/qcom/bfm_qcom.h>

struct boot_log_struct *boot_log = NULL;
extern void msm_trigger_wdog_bark(void);

static u32 hwboot_calculate_checksum(unsigned char *addr, u32 len)
{
    int i;
    uint8_t *w = addr;
    u32 sum = 0;

    for(i=0;i<len;i++)
    {
        sum += *w++;
    }

    return sum;
}

static int hwboot_match_checksum(struct boot_log_struct *bl)
{

    u32 cksum_calc = hwboot_calculate_checksum((uint8_t *)bl, BOOT_LOG_CHECK_SUM_SIZE);

    if (cksum_calc != bl->hash_code )
    {
        pr_notice("hwboot_match_checksum: Checksum error\r\n");
        return 1;
    }
    return 0;
}

void qcom_set_boot_stage(bfmr_detail_boot_stage_e stage)
{
    if(NULL != boot_log)
    {
        boot_log->boot_stage = stage;
    }
       return ;
}
EXPORT_SYMBOL(qcom_set_boot_stage);

u32 qcom_get_boot_stage(void)
{
    if(NULL != boot_log)
    {
        pr_info(" boot_stage = 0x%08x\n", boot_log->boot_stage);
        return boot_log->boot_stage;
    }
    pr_err("get boot stage fail\n");
    return 0;
}
EXPORT_SYMBOL(qcom_get_boot_stage);

static unsigned long long qcom_get_system_time(void)
{
    struct timeval tv = {0};

    do_gettimeofday(&tv);

    return (unsigned long long)tv.tv_sec;
}

int qcom_set_boot_fail_flag(bfmr_bootfail_errno_e bootfail_errno)
{
    int ret = -1;
    if(NULL != boot_log)
    {
        if(!boot_log->boot_error_no)
        {
            boot_log->boot_error_no = bootfail_errno;
            boot_log->rcv_method = BFM_CAN_NOT_CALL_TRY_TO_RECOVERY;
            boot_log->rtc_time = 0;
            boot_log->isUserPerceptiable = 1;
            ret = 0;
        }
    }
    return ret;
}
EXPORT_SYMBOL(qcom_set_boot_fail_flag);

static struct bootlog_inject_struct *bootlog_inject =NULL;
void  hwboot_fail_init_struct(void)
{
    u64 *fseq,*nseq;
    u32 *fidx;
    void * boot_log_virt = ioremap_nocache(HWBOOT_LOG_INFO_START_BASE,HWBOOT_LOG_INFO_SIZE);
    boot_log = (struct boot_log_struct *)(boot_log_virt);
    bootlog_inject = (struct bootlog_inject_struct *)(boot_log_virt + sizeof(struct boot_log_struct) + sizeof(struct bootlog_read_struct));

    pr_notice("hwboot:boot_log=%p\n", boot_log);

    if(NULL != boot_log)
    {
        if(hwboot_match_checksum(boot_log))
        {
            pr_err("hwboot checksum fail\n");
            return;
        }

        /* save log address and size */
        boot_log->kernel_addr = virt_to_phys((void *)log_buf_addr_get());
        boot_log->kernel_log_buf_size = log_buf_len_get();
        boot_log->boot_stage = KERNEL_STAGE_START;

        hwboot_get_printk_buf_info(&fseq, &fidx, &nseq);
        boot_log->klog_first_seq_addr = virt_to_phys((void *)fseq);
        boot_log->klog_first_idx_addr = virt_to_phys((void *)fidx);
        boot_log->klog_next_seq_addr =  virt_to_phys((void *)nseq);

        pr_notice("hwboot: sbl_log=%x %d\n", boot_log->sbl_addr, boot_log->sbl_log_buf_size);
        pr_notice("hwboot: aboot=%x %d\n", boot_log->aboot_addr, boot_log->aboot_log_buf_size);
        pr_notice("hwboot: kernel=%x %d\n", boot_log->kernel_addr, boot_log->kernel_log_buf_size);

        boot_log->hash_code = hwboot_calculate_checksum((u8 *)boot_log, BOOT_LOG_CHECK_SUM_SIZE);
        //iounmap((void*)boot_log_virt);
    }
    else {
        pr_notice("hwboot: bootlog is null\n");
    }

    return;
}
EXPORT_SYMBOL(hwboot_fail_init_struct);

bool check_bootfail_inject(u32 err_code)
{
    if (bootlog_inject->flag == HWBOOT_FAIL_INJECT_MAGIC) {
        if(err_code == bootlog_inject->inject_boot_fail_no) {
            bootlog_inject->flag = 0;
            bootlog_inject->inject_boot_fail_no = 0;
            return true;
        }
    }
    return false;
}
EXPORT_SYMBOL(check_bootfail_inject);

void hwboot_clear_magic(void)
{
    boot_log->boot_magic = 0;
    return;
}
EXPORT_SYMBOL(hwboot_clear_magic);

#define ATS_SIZE    8
#define DATA_ATS    "/data/time/ats_1"
#define LOG_ATS     "/log/time/ats_1"
#define LOG_ATS_DIR     "/log/time"
#define FILE_LIMIT  0660
#define DIR_LIMIT  0770
#define WAIT_ATS_SECS  2
#define MSECS_ATS_READY_DELAY (3000)
#define MSECS_SYNC_ATS_INTERVAL (2000)

struct hwboot_ats_data {
    struct task_struct *boot_ats_task;
    struct completion boot_ats_complete;
    char ats_1[ATS_SIZE];
    char last_ats_1[ATS_SIZE];
};

int boot_wait_partition(char *path, int timeouts)
{
    struct kstat m_stat;
    mm_segment_t old_fs;
    int timeo;

    if (path == NULL) {
        pr_err("invalid  parameter. path:%p\n", path);
        return -1;
    }

    timeo = timeouts;

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    while (0 != vfs_stat(path, &m_stat)) {
        current->state = TASK_INTERRUPTIBLE;
        (void)schedule_timeout(HZ / 10); /*wait for 1/10 second */
        if (timeouts-- < 0) {
            set_fs(old_fs);
            pr_err("%d:rdr:wait partiton[%s] fail. use [%d]'s . skip!\n",
            __LINE__, path, timeo);
            return -1;
        }
    }
    set_fs(old_fs);

    return 0;
}

static int read_ats_file(char *ats_name, char *ats_value)
{
    mm_segment_t old_fs;
    long length;
    int fd;
    int ret = 0;

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    fd = sys_open(ats_name, O_RDONLY, 0);
    if (fd < 0)
    {
        ret = -1;
        pr_err("open %s error fd=%d\n",ats_name,fd);
        goto __out;
    }

    length = bfmr_full_read(fd, ats_value, ATS_SIZE);
    if (ATS_SIZE != length)
    {
        ret = -1;
        pr_err("read ats error \n");
    }

__out:
    if (fd >= 0)
    {
        sys_close(fd);
    }
    set_fs(old_fs);
    return ret;
}

static int write_ats_file(char *ats_path, char *ats_name, char *ats_value)
{
    mm_segment_t old_fs;
    long length;
    int fd;
    int fd_path;
    int ret = 0;

    old_fs = get_fs();
    set_fs(KERNEL_DS);

    /*1. check log-time directory */
    fd_path = sys_access(ats_path, 0);
    if (0 != fd_path) {
        pr_err("%s:need create dir %s !\n",__func__,ats_path);
        fd_path = sys_mkdir(ats_path, DIR_LIMIT);
        if (fd_path < 0) {
            ret = -1;
            pr_err("%s: create dir %s failed! ret = %d\n",__func__, ats_path, fd_path);
            set_fs(old_fs);
            return ret;

        }
    }

    /* 2. open file for writing */
    fd = sys_open(ats_name, O_WRONLY|O_CREAT|O_TRUNC, FILE_LIMIT);
    if (fd < 0)
    {
        ret = -1;
        pr_err("Open file [%s] failed!fd: %d\n", ats_name, fd);
        goto __out;
    }

    /* 3. write data to file */
    length = bfmr_full_write(fd, ats_value, ATS_SIZE);
    if ((long)ATS_SIZE != length)
    {
        ret = -1;
        pr_err("write file [%s] failed!bytes_write: %ld, it shoule be: %ld\n",
            ats_name, (long)length, (long)ATS_SIZE);
        goto __out;
    }

__out:
    if (fd >= 0)
    {
        sys_sync();
        sys_close(fd);
    }
    set_fs(old_fs);

    return ret;
}

static void sync_ats(void * arg, int wait_sec)
{
    int i;
    struct hwboot_ats_data *ats_dt =
               (struct hwboot_ats_data *)arg;

    pr_err("%s start\n",__func__);

    /*check if need reread ats from data*/
    i = wait_sec;
    read_ats_file(LOG_ATS,ats_dt->last_ats_1);
    do {
        msleep(MSECS_SYNC_ATS_INTERVAL);
        /* read ats */
        if(read_ats_file(DATA_ATS,ats_dt->ats_1))
        {
          continue;
        }
        if(memcmp(ats_dt->last_ats_1, ats_dt->ats_1, ATS_SIZE)) {
            memcpy(ats_dt->last_ats_1, ats_dt->ats_1, ATS_SIZE);
            write_ats_file(LOG_ATS_DIR, LOG_ATS, ats_dt->ats_1);
        }
    } while(i--);

    pr_err("%s end\n",__func__);
    return;
}

struct hwboot_ats_data *boot_ats_data = NULL;
void boot_ats_task_wakeup(void)
{
  pr_err("%s start\n",__func__);
  if(boot_ats_data == NULL)
  {
    return;
  }
  complete(&boot_ats_data->boot_ats_complete);
  pr_err("%s end\n",__func__);
}

static __ref int boot_ats_kthread(void *arg)
{
    struct hwboot_ats_data *ats_dt =
                 (struct hwboot_ats_data *)arg;
    struct sched_param param = {.sched_priority = MAX_RT_PRIO-1};

    pr_err("%s start\n",__func__);
    sched_setscheduler(current, SCHED_FIFO, &param);
    while (boot_wait_partition("/data/lost+found", 1000) != 0)
    {
       ;
    }
    if (boot_wait_partition("/log/boot_fail", 1000) != 0)
    {
        pr_err("%s wait log partition fail\n",__func__);
        return 0;
    }
    msleep(MSECS_ATS_READY_DELAY);

    boot_ats_data = ats_dt;
    sync_ats(arg,WAIT_ATS_SECS);
    while (!kthread_should_stop()) {
       while (wait_for_completion_interruptible(
            &ats_dt->boot_ats_complete) != 0)
            ;
       reinit_completion(&ats_dt->boot_ats_complete);
       sync_ats(arg,WAIT_ATS_SECS);
    }
    pr_err("%s end\n",__func__);
    return 0;
}

int boot_ats_init(void)
{
    int ret = 0;
    struct hwboot_ats_data *ats_dt;

    pr_err("%s start\n",__func__);
    ats_dt = kzalloc(sizeof(struct hwboot_ats_data), GFP_KERNEL);
    if (!ats_dt)
    {
        return -EIO;
    }
    ats_dt->boot_ats_task = kthread_create(boot_ats_kthread, ats_dt,
                                              "hwboot_ats");
    if (IS_ERR(ats_dt->boot_ats_task)) {
        ret = PTR_ERR(ats_dt->boot_ats_task);
        goto err;
    }

    init_completion(&ats_dt->boot_ats_complete);
    memset(ats_dt->ats_1, 0, ATS_SIZE);
    memset(ats_dt->last_ats_1, 0, ATS_SIZE);
    wake_up_process(ats_dt->boot_ats_task);
    pr_err("%s end\n",__func__);
    return 0;

    err:
       pr_err("%s faild\n",__func__);
       kzfree(ats_dt);
       return ret;
}

static u64 get_ats_1_secs(void)
{

    u64 ats_tmp = 0;

    if(NULL == boot_log)
    {
      return ats_tmp;
    }

    ats_tmp = boot_log->ats_1[1];
    ats_tmp = (ats_tmp << 32) + boot_log->ats_1[0];

    pr_err("%s:ats_tmp=%llu\n",__func__,ats_tmp);

    return div_u64(ats_tmp,1000);
}

long long bfm_hctosys(unsigned long long current_secs, bool is_do_set_time)
{
    long long ats_1_secs;
    long long time_1980;
    struct timespec tv = {
        .tv_nsec = NSEC_PER_SEC >> 1,
    };

    time_1980 = mktime(1980, 1, 1, 0, 0, 0);
    ats_1_secs = get_ats_1_secs();
    if(0 == ats_1_secs)
    {
      ats_1_secs = time_1980;
    }

    tv.tv_sec = (long long)current_secs;
    if (tv.tv_sec < time_1980)
    {
        tv.tv_sec += ats_1_secs;
        if (is_do_set_time)
        {
            do_settimeofday(&tv);
        }
    }

    return tv.tv_sec;
}

int qcom_hwboot_fail_init(void)
{
    pr_err("%s start\n",__func__);
    boot_ats_init();

    if(check_bootfail_inject(KERNEL_AP_PANIC))
    {
        panic("hwboot: inject KERNEL_AP_PANIC");
    }
    if(check_bootfail_inject(KERNEL_AP_WDT))
    {
        msm_trigger_wdog_bark();
    }
    if(check_bootfail_inject(KERNEL_BOOT_TIMEOUT))
    {
        boot_fail_err(KERNEL_BOOT_TIMEOUT, NO_SUGGESTION, NULL);
    }

    pr_err("%s end\n",__func__);
    return 0;
}

