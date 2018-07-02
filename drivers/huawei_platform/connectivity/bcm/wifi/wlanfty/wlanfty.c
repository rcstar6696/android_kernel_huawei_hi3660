#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/uaccess.h>


#define MAX_CMD_LEN 32
extern int wlanfty_status_value;
extern int dhd_wlanfty_ver(void);
extern void wlanfty_register_handle(void);
//BIT 0 is reserve
#define WLANFTY_STATUS_HALTED		(1 << 1)
#define WLANFTY_STATUS_KSO_ERROR	(1 << 2)
#define WLANFTY_STATUS_RECOVERY		(1 << 3)

MODULE_LICENSE("GPL v2");

HWLOG_REGIST();

//clear node wlanfty_status
void clear_wlanfty_status(void) {
	wlanfty_status_value = 0;
	hwlog_err("clear wlanfty_status\n");
}

static int wlanfty_status_open(struct inode *inode, struct file *filp) {
	hwlog_info("Open wlanfty_status OK\n");
	return 0;
}

static int wlanfty_status_release(struct inode *inode, struct file *filp) {
	hwlog_info("wlanfty_status is released\n");
	return 0;
}

static int wlanfty_ver_open(struct inode *inode, struct file *filp) {
	hwlog_info("Open wlanfty_status OK\n");
	return 0;
}

static int wlanfty_ver_release(struct inode *inode, struct file *filp) {
   	hwlog_info("node_wlanfty_ver is released\n");
	return 0;
}

static ssize_t wlanfty_status_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
	size_t copy_size = 0;
   	char g_cmd_buff[MAX_CMD_LEN + 1] = {0};

	if (buf == NULL || count == 0) {
		hwlog_info("%s buf is not enough count %d \n", __FUNCTION__, count);
		return 0;
	}

	if(*ppos > 0) {
		hwlog_info("%s read is done ,no need to read again\n", __FUNCTION__);
		return 0;
	}

	memset(g_cmd_buff, 0, sizeof(g_cmd_buff));
	copy_size =  snprintf(g_cmd_buff, sizeof(g_cmd_buff),"%x", wlanfty_status_value);

	if(wlanfty_status_value)
		hwlog_err("%s value = %x error\n", __FUNCTION__, wlanfty_status_value);

	if (copy_size > count) {
		hwlog_info("%s count %d is small\n", __FUNCTION__, count);
		return 0;
	}
	if(copy_to_user(buf, g_cmd_buff, copy_size) == 0) {
		hwlog_info("read %d byte to user\n", copy_size);
		*ppos += copy_size;
		clear_wlanfty_status();
		return copy_size;
	} else {
		hwlog_err("%s read byte to user fail\n", __FUNCTION__);
		clear_wlanfty_status();
		return 0;
	}
}

static ssize_t wlanfty_ver_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
	size_t copy_size = 0;
	char g_cmd_buff[MAX_CMD_LEN + 1] = {0};
	int value;

	if (buf == NULL || count == 0) {
                hwlog_info("%s buf is not enough count %d \n", __FUNCTION__, count);
                return 0;
        }

 	if(*ppos > 0) {
		hwlog_info("%s read is done ,no need to read again\n", __FUNCTION__);
		return 0;
	}
	value = dhd_wlanfty_ver();
	memset(g_cmd_buff, 0, sizeof(g_cmd_buff));
	copy_size =  snprintf(g_cmd_buff, sizeof(g_cmd_buff),"%d", value);

	if(value)
	    hwlog_err("%s value = %d error\n", __FUNCTION__, value);

	if (copy_size > count) {
                hwlog_info("%s count %d is small \n", __FUNCTION__, count);
                return 0;
        }

	if(copy_to_user(buf, g_cmd_buff, copy_size) == 0) {
		hwlog_info("read %d byte to user\n", copy_size);
		*ppos += copy_size;
		return copy_size;
 	} else {
		hwlog_err("%s read byte to user fail\n", __FUNCTION__);
		return 0;
	}
}

static const struct file_operations wlanfty_status_fops = {
	.owner          = THIS_MODULE,
	.open           = wlanfty_status_open,
	.release        = wlanfty_status_release,
	.read           = wlanfty_status_read,
};

static struct miscdevice wlanfty_status = {
	.minor          = MISC_DYNAMIC_MINOR,
	.name           = "wlanfty_status",
	.fops           = &wlanfty_status_fops,
};

static const struct file_operations wlanfty_ver_fops = {
	.owner          = THIS_MODULE,
	.open           = wlanfty_ver_open,
	.release        = wlanfty_ver_release,
	.read           = wlanfty_ver_read,
};

static struct miscdevice wlanfty_ver = {
	.minor          = MISC_DYNAMIC_MINOR,
	.name           = "wlanfty_ver",
	.fops           = &wlanfty_ver_fops,
};

static int __init wlanfty_init(void)
{
	int ret;
	ret = misc_register(&wlanfty_ver);
	if(ret) {
		hwlog_err("Init wlanfty_ver error\n");
		return ret;
	}

	ret = misc_register(&wlanfty_status);
	if(ret) {
		hwlog_err("Init wlanfty_status error\n");
		return ret;
	}
	return 0;
}
static void __exit wlanfty_exit(void)
{
	misc_deregister(&wlanfty_ver);
	misc_deregister(&wlanfty_status);
}

module_init(wlanfty_init);
module_exit(wlanfty_exit);
