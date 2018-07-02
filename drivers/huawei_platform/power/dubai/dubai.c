/*
 * DUBAI drvier.
 *
 * Copyright (C) 2017 Huawei Device Co.,Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include <huawei_platform/power/dubai/dubai_common.h>

#define DUBAI_MAGIC 'k'
#define IOCTL_GPU_ENABLE _IOW(DUBAI_MAGIC, 1, bool)
#define IOCTL_GPU_INFO_GET _IOR(DUBAI_MAGIC, 2, struct dev_transmit_t)
#define IOCTL_PID_CPUTIME_REQUEST _IOW(DUBAI_MAGIC, 3, struct dev_transmit_t)
#define IOCTL_PROCESS_NAME_GET _IOWR(DUBAI_MAGIC, 4, struct dev_transmit_t)
#define IOCTL_LOG_STATS_ENABLE _IOW(DUBAI_MAGIC, 5, bool)
#define IOCTL_KWORKER_INFO_REQUEST _IOW(DUBAI_MAGIC, 6, long long)
#define IOCTL_UEVENT_INFO_REQUEST _IOW(DUBAI_MAGIC, 7, long long)

static long dubai_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int rc = 0;
	void __user *argp = (void __user *)arg;

	switch (cmd) {
	case IOCTL_GPU_ENABLE: {
		bool enable;

		if (copy_from_user(&enable, argp, sizeof(bool))) {
			DUBAI_LOGE("Failed to set gpu statistics enable");
			rc = -EFAULT;
			break;
		}
		dubai_set_gpu_enable(enable);
		break;
	}
	case IOCTL_GPU_INFO_GET: {
		rc = dubai_get_gpu_info(arg);
		break;
	}
	case IOCTL_PID_CPUTIME_REQUEST: {
		int size;
		struct polling_event *event = NULL;
		struct dev_transmit_t *transmit = NULL;

		size = sizeof(struct dev_transmit_t)
			+ sizeof(struct polling_event);
		transmit = kzalloc(size, GFP_KERNEL);
		if (transmit == NULL) {
			DUBAI_LOGE("Failed to allocate memory");
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(transmit, argp, size)) {
			rc = -EFAULT;
			kfree(transmit);
			break;
		}

		event = (struct polling_event *)transmit->data;
		rc = dubai_get_pid_cputime(event);
		kfree(transmit);
		break;
	}
	case IOCTL_PROCESS_NAME_GET: {
		int size;
		struct process_name *process = NULL;
		struct dev_transmit_t *transmit = NULL;

		size = sizeof(struct dev_transmit_t)
			+ sizeof(struct process_name);
		transmit = kzalloc(size, GFP_KERNEL);
		if (transmit == NULL) {
			DUBAI_LOGE("Failed to allocate memory");
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(transmit, argp, size)) {
			rc = -EFAULT;
			kfree(transmit);
			break;
		}

		process = (struct process_name *)transmit->data;
		dubai_get_process_name(process);
		if (copy_to_user(argp, transmit, size))
			rc = -EFAULT;
		kfree(transmit);
		break;
	}
	case IOCTL_LOG_STATS_ENABLE: {
		bool enable;

		if (copy_from_user(&enable, argp, sizeof(bool))) {
			DUBAI_LOGE("Failed to get kworker enable value");
			rc = -EFAULT;
			break;
		}
		dubai_log_stats_enable(enable);
		break;
	}
	case IOCTL_KWORKER_INFO_REQUEST: {
		long long timestamp;

		if (copy_from_user(&timestamp, argp, sizeof(long long))) {
			DUBAI_LOGE("Failed to get timestamp");
			rc = -EFAULT;
			break;
		}
		rc = dubai_get_kworker_info(timestamp);
		break;
	}
	case IOCTL_UEVENT_INFO_REQUEST: {
		long long timestamp;

		if (copy_from_user(&timestamp, argp, sizeof(long long))) {
			DUBAI_LOGE("Failed to get timestamp");
			rc = -EFAULT;
			break;
		}
		rc = dubai_get_uevent_info(timestamp);
		break;
	}
	default:
		rc = -EINVAL;
		break;
	}

	return rc;
}

#ifdef CONFIG_COMPAT
static long dubai_compat_ioctl(struct file *filp,
			unsigned int cmd, unsigned long arg)
{
	return dubai_ioctl(filp, cmd, (unsigned long) compat_ptr(arg));
}
#endif

static int dubai_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int dubai_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations dubai_device_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = dubai_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= dubai_compat_ioctl,
#endif
	.open = dubai_open,
	.release = dubai_release,
};

static struct miscdevice dubai_device = {
	.name = "dubai",
	.fops = &dubai_device_fops,
	.minor = MISC_DYNAMIC_MINOR,
};

static int __init dubai_init(void)
{
	int ret = 0;

	dubai_gpu_init();
	dubai_pid_cputime_init();
	dubai_stats_init();

	ret = misc_register(&dubai_device);
	if (ret) {
		DUBAI_LOGE("Failed to register dubai device");
		goto out;
	}

	DUBAI_LOGD("DUBAI module initialize success");
out:
	return ret;
}

static void __exit dubai_exit(void)
{
	dubai_gpu_exit();
	dubai_pid_cputime_exit();
	dubai_stats_exit();
	buffered_log_release();
}

late_initcall(dubai_init);
module_exit(dubai_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yu Peng, <pengyu7@huawei.com>");
MODULE_DESCRIPTION("Huawei Device Usage Big-data Analytics Initiative Driver");
