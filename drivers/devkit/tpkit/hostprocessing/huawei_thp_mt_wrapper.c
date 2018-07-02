/*
 * Huawei Touchscreen Driver
 *
 * Copyright (C) 2017 Huawei Device Co.Ltd
 * License terms: GNU General Public License (GPL) version 2
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/slab.h>
#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#endif
#include "huawei_thp_mt_wrapper.h"
#include "huawei_thp.h"


#define DEVICE_NAME	  "input_mt_wrapper"

static struct thp_mt_wrapper_data *g_thp_mt_wrapper = 0;

static long thp_mt_wrapper_ioctl_set_coordinate(unsigned long arg)
{
	long ret = 0;
	void __user *argp = (void __user *)arg;
	struct input_dev *input_dev = g_thp_mt_wrapper->input_dev;
	struct thp_mt_wrapper_ioctl_touch_data data;
	u8 i;

	if (arg == 0) {
		THP_LOG_ERR("%s:arg is null.\n", __func__);
		return -EINVAL;
	}

	if (copy_from_user(&data, argp,
			sizeof(struct thp_mt_wrapper_ioctl_touch_data))) {
		THP_LOG_ERR("Failed to copy_from_user().\n");
		return -EFAULT;
	}

	for (i = 0; i < INPUT_MT_WRAPPER_MAX_FINGERS; i++) {
#ifdef TYPE_B_PROTOCOL
		input_mt_slot(input_dev, i);
		input_mt_report_slot_state(input_dev,
			data.touch[i].tool_type, data.touch[i].valid != 0);
#endif

		if (data.touch[i].valid != 0) {
			input_report_abs(input_dev, ABS_MT_POSITION_X,
						data.touch[i].x);
			input_report_abs(input_dev, ABS_MT_POSITION_Y,
						data.touch[i].y);
			input_report_abs(input_dev, ABS_MT_PRESSURE,
						data.touch[i].pressure);
			input_report_abs(input_dev, ABS_MT_TRACKING_ID,
						data.touch[i].tracking_id);
			input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR,
						data.touch[i].major);
			input_report_abs(input_dev, ABS_MT_TOUCH_MINOR,
						data.touch[i].minor);
			input_report_abs(input_dev, ABS_MT_ORIENTATION,
						data.touch[i].orientation);
			input_report_abs(input_dev, ABS_MT_TOOL_TYPE,
						data.touch[i].tool_type);
#ifndef TYPE_B_PROTOCOL
			input_mt_sync(input_dev);
#endif
		}
	}

	/* BTN_TOUCH DOWN */
	if (data.t_num > 0)
		input_report_key(input_dev, BTN_TOUCH, 1);

	/* BTN_TOUCH UP */
	if (data.t_num == 0) {
#ifndef TYPE_B_PROTOCOL
		input_mt_sync(input_dev);
#endif
		input_report_key(input_dev, BTN_TOUCH, 0);
	}

	input_sync(input_dev);

	return ret;
}

static void thp_mt_wrapper_release_coordinate(void)
{

	struct input_dev *input_dev = g_thp_mt_wrapper->input_dev;
	struct thp_mt_wrapper_ioctl_touch_data data;
	u8 i;

	memset(&data, 0, sizeof(data));

	for (i = 0; i < INPUT_MT_WRAPPER_MAX_FINGERS; i++) {
		input_report_abs(input_dev, ABS_MT_POSITION_X,
						 data.touch[i].x);
		input_report_abs(input_dev, ABS_MT_POSITION_Y,
						 data.touch[i].y);
		input_report_abs(input_dev, ABS_MT_PRESSURE,
						 data.touch[i].pressure);
		input_report_abs(input_dev, ABS_MT_TRACKING_ID,
						 data.touch[i].tracking_id);
		input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR,
						 data.touch[i].major);
		input_report_abs(input_dev, ABS_MT_TOUCH_MINOR,
						 data.touch[i].minor);
		input_report_abs(input_dev, ABS_MT_ORIENTATION,
						 data.touch[i].orientation);
		input_report_abs(input_dev, ABS_MT_TOOL_TYPE,
						 data.touch[i].tool_type);
		input_mt_sync(input_dev);
	}

	/* SYN_REPORT */
	input_report_key(input_dev, BTN_TOUCH, 0);
	input_sync(input_dev);
}



static int thp_mt_wrapper_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int thp_mt_wrapper_release(struct inode *inode,
						struct file *filp)
{
	return 0;
}

static long thp_mt_wrapper_ioctl(struct file *filp, unsigned int cmd,
				unsigned long arg)
{
	long ret;

	switch (cmd) {
	case INPUT_MT_WRAPPER_IOCTL_CMD_SET_COORDINATES:
		ret = thp_mt_wrapper_ioctl_set_coordinate(arg);
		break;

	default:
		THP_LOG_ERR("cmd unkown.\n");
		ret = -EINVAL;
	}

	return ret;
}

int thp_daemeon_suspend_resume_notify(int status)
{
	struct input_dev *input_dev = g_thp_mt_wrapper->input_dev;

	if (status == THP_RESUME) {
		THP_LOG_INFO("%s:input_mt_wrapper resume\n", __func__);
		kobject_uevent(&input_dev->dev.kobj, KOBJ_ONLINE);
	} else if (status == THP_SUSPEND) {
		THP_LOG_INFO("%s:input_mt_wrapper suspend\n", __func__);
		thp_mt_wrapper_release_coordinate();
		kobject_uevent(&input_dev->dev.kobj, KOBJ_OFFLINE);
	}

	return 0;
}

static const struct file_operations g_thp_mt_wrapper_fops = {
	.owner = THIS_MODULE,
	.open = thp_mt_wrapper_open,
	.release = thp_mt_wrapper_release,
	.unlocked_ioctl = thp_mt_wrapper_ioctl,
};

static struct miscdevice g_thp_mt_wrapper_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &g_thp_mt_wrapper_fops,
};

static void set_default_input_config(struct thp_input_dev_config *input_config)
{
	input_config->abs_max_x = THP_MT_WRAPPER_MAX_X;
	input_config->abs_max_y = THP_MT_WRAPPER_MAX_Y;
	input_config->abs_max_z = THP_MT_WRAPPER_MAX_Z;
	input_config->tracking_id_max = THP_MT_WRAPPER_MAX_FINGERS;
	input_config->major_max = THP_MT_WRAPPER_MAX_MAJOR;
	input_config->minor_max = THP_MT_WRAPPER_MAX_MINOR;
	input_config->orientation_min = THP_MT_WRAPPER_MIN_ORIENTATION;
	input_config->orientation_max = THP_MT_WRAPPER_MAX_ORIENTATION;
	input_config->tool_type_max = THP_MT_WRAPPER_TOOL_TYPE_MAX;
}
static int thp_parse_input_config(struct thp_input_dev_config *config)
{
	int rc = 0;
	struct device_node *thp_dev_node = NULL;

	thp_dev_node = of_find_compatible_node(NULL, NULL,
					THP_INPUT_DEV_COMPATIBLE);
	if (!thp_dev_node) {
		THP_LOG_INFO("%s:not found node, use defatle config\n",
					__func__);
		goto use_defaule;
	}

	rc = of_property_read_u32(thp_dev_node, "abs_max_x",
						&config->abs_max_x);
	if (rc) {
		THP_LOG_ERR("%s:abs_max_x not config, use deault\n", __func__);
		config->abs_max_x = THP_MT_WRAPPER_MAX_X;
	}

	rc = of_property_read_u32(thp_dev_node, "abs_max_y",
						&config->abs_max_y);
	if (rc) {
		THP_LOG_ERR("%s:abs_max_y not config, use deault\n", __func__);
		config->abs_max_y = THP_MT_WRAPPER_MAX_Y;
	}

	rc = of_property_read_u32(thp_dev_node, "abs_max_z",
						&config->abs_max_z);
	if (rc) {
		THP_LOG_ERR("%s:abs_max_z not config, use deault\n", __func__);
		config->abs_max_z = THP_MT_WRAPPER_MAX_Z;
	}

	rc = of_property_read_u32(thp_dev_node, "tracking_id_max",
						&config->tracking_id_max);
	if (rc) {
		THP_LOG_ERR("%s:tracking_id_max not config, use deault\n",
				__func__);
		config->tracking_id_max = THP_MT_WRAPPER_MAX_FINGERS;
	}

	rc = of_property_read_u32(thp_dev_node, "major_max",
						&config->major_max);
	if (rc) {
		THP_LOG_ERR("%s:major_max not config, use deault\n", __func__);
		config->major_max = THP_MT_WRAPPER_MAX_MAJOR;
	}

	rc = of_property_read_u32(thp_dev_node, "minor_max",
						&config->minor_max);
	if (rc) {
		THP_LOG_ERR("%s:minor_max not config, use deault\n", __func__);
		config->minor_max = THP_MT_WRAPPER_MAX_MINOR;
	}

	rc = of_property_read_u32(thp_dev_node, "orientation_min",
						&config->orientation_min);
	if (rc) {
		THP_LOG_ERR("%s:orientation_min not config, use deault\n",
				__func__);
		config->orientation_min = THP_MT_WRAPPER_MIN_ORIENTATION;
	}

	rc = of_property_read_u32(thp_dev_node, "orientation_max",
					&config->orientation_max);
	if (rc) {
		THP_LOG_ERR("%s:orientation_max not config, use deault\n",
				__func__);
		config->orientation_max = THP_MT_WRAPPER_MAX_ORIENTATION;
	}

	rc = of_property_read_u32(thp_dev_node, "tool_type_max",
					&config->tool_type_max);
	if (rc) {
		THP_LOG_ERR("%s:tool_type_max not config, use deault\n",
				__func__);
		config->tool_type_max = THP_MT_WRAPPER_TOOL_TYPE_MAX;
	}

	return 0;

use_defaule:
	set_default_input_config(config);
	return 0;
}

int thp_mt_wrapper_init(void)
{
	struct input_dev *input_dev;
	struct thp_input_dev_config *input_config;
	static struct thp_mt_wrapper_data *mt_wrapper;
	int rc;

	if (g_thp_mt_wrapper) {
		THP_LOG_ERR("%s:thp_mt_wrapper have inited, exit\n", __func__);
		return 0;
	}

	mt_wrapper = kzalloc(sizeof(struct thp_mt_wrapper_data), GFP_KERNEL);
	if (!mt_wrapper) {
		THP_LOG_ERR("%s:out of memory\n", __func__);
		return -ENOMEM;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		THP_LOG_ERR("%s:Unable to allocated input device\n", __func__);
		kfree(mt_wrapper);
		return	-ENODEV;
	}

	input_dev->name = THP_INPUT_DEVICE_NAME;

	rc = thp_parse_input_config(&mt_wrapper->input_dev_config);
	if (rc)
		THP_LOG_ERR("%s: parse config fail\n", __func__);

	__set_bit(EV_SYN, input_dev->evbit);
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(BTN_TOUCH, input_dev->keybit);
	__set_bit(BTN_TOOL_FINGER, input_dev->keybit);
	__set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

	input_set_abs_params(input_dev, ABS_X,
			     0, mt_wrapper->input_dev_config.abs_max_x - 1, 0, 0);
	input_set_abs_params(input_dev, ABS_Y,
			     0, mt_wrapper->input_dev_config.abs_max_y - 1, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE,
			0, mt_wrapper->input_dev_config.abs_max_z, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X,
			0, mt_wrapper->input_dev_config.abs_max_x - 1, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,
			0, mt_wrapper->input_dev_config.abs_max_y - 1, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE,
			0, mt_wrapper->input_dev_config.abs_max_z, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID,
		0, mt_wrapper->input_dev_config.tracking_id_max - 1, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR,
			0, mt_wrapper->input_dev_config.major_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MINOR,
			0, mt_wrapper->input_dev_config.minor_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_ORIENTATION,
			mt_wrapper->input_dev_config.orientation_min,
			mt_wrapper->input_dev_config.orientation_max, 0, 0);
#ifdef TYPE_B_PROTOCOL
	input_mt_init_slots(input_dev, THP_MT_WRAPPER_MAX_FINGERS);
#endif

	rc = input_register_device(input_dev);
	if (rc) {
		THP_LOG_ERR("%s:failed to register input device\n", __func__);
		goto input_dev_reg_err;
	}

	rc = misc_register(&g_thp_mt_wrapper_misc_device);
	if (rc) {
		THP_LOG_ERR("%s:failed to register misc device\n", __func__);
		goto misc_dev_reg_err;
	}

	mt_wrapper->input_dev = input_dev;
	g_thp_mt_wrapper = mt_wrapper;
	return 0;

fb_notify_reg_err:
	misc_deregister(&g_thp_mt_wrapper_misc_device);
misc_dev_reg_err:
	input_unregister_device(input_dev);
input_dev_reg_err:
	kfree(mt_wrapper);

	return rc;
}
EXPORT_SYMBOL(thp_mt_wrapper_init);

void thp_mt_wrapper_exit(void)
{
	if (!g_thp_mt_wrapper)
		return;

	input_unregister_device(g_thp_mt_wrapper->input_dev);
	misc_deregister(&g_thp_mt_wrapper_misc_device);
}
EXPORT_SYMBOL(thp_mt_wrapper_exit);

