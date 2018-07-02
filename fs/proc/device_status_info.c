

#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/printk.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <chipset_common/hwstatuscheck/device_status_info.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
#include <linux/module.h>
#endif
static uint32_t check_device_status_flag_long = 0;

int set_hw_check_dev_status_flag( int check_id )
{
    if( (check_id >= 0) && ( check_id < DEV_CHECK_STATUS_MAX ) )
    {
        check_device_status_flag_long = check_device_status_flag_long | (1 << check_id);
        return 0;
    }
    else
    {
        pr_err("Device %d  is unknown in list \n",check_id);
        return -1;
    }
}


EXPORT_SYMBOL(set_hw_check_dev_status_flag);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0))
static int check_dev_status_proc_show(struct seq_file *m, void *v)
{
    return seq_printf(m,"%x\n",check_device_status_flag_long);
}
#else
void check_dev_status_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m,"%x\n",check_device_status_flag_long);
}
#endif

static int check_device_status_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, check_dev_status_proc_show, NULL);
}

static ssize_t check_devces_status_proc_write(struct file *file, const char *buffer, size_t count, loff_t *off)
{
    int err = 0;
    int user_check_id = 0;
    err = copy_from_user((&user_check_id), buffer, sizeof(int));
    if(err)
    {
        pr_err("%s, unable to copy data from userspace, err: %d\n", __func__, err);
        return -EFAULT;
    }
    return set_hw_check_dev_status_flag(user_check_id);
}

static const struct file_operations check_dev_status_fops =
{
    .open		= check_device_status_proc_open,
    .write      = check_devces_status_proc_write,
    .read		= seq_read,
    .llseek		= seq_lseek,
    .release	= single_release,
};

static int __init proc_check_device_status_init(void)
{
    proc_create("device_status_info", 0, NULL, &check_dev_status_fops);
    return 0;
}

module_init(proc_check_device_status_init);
