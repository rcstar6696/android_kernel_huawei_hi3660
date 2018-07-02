#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/notifier.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <huawei_platform/power/huawei_charger.h>
#ifdef CONFIG_DIRECT_CHARGER
#include <huawei_platform/power/direct_charger.h>
#endif
#ifdef  CONFIG_HUAWEI_USB_SHORT_CIRCUIT_PROTECT
#include <huawei_platform/power/usb_short_circuit_protect.h>
#endif
#include <huawei_platform/power/vbat_ovp.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mtd/hisi_nve_interface.h>

#define HWLOG_TAG vbat_ovp
HWLOG_REGIST();

static int batovp_value = 0;

static struct vbat_ovp_device_info *g_di;
ATOMIC_NOTIFIER_HEAD(vbat_ovp_notifier_list);
int vbat_ovp_set_input_current_flag = 0;
struct vbat_ovp_ops* g_vbat_ovp_ops;
struct completion usb_detach_completion;
/*******************************************************************************************
Function:	read_batovp_value_from_nv
Description:   读取NV项中的batovp 数据
Data Accessed:  无
Data Updated:   无
Input:         无
Output:         无
Return:         成功或者失败信息: 0->成功, -1->失败
*******************************************************************************************/
static int read_batovp_value_from_nv(void)
{
	int ret = 0;
	struct hisi_nve_info_user user_info;

	memset(&user_info, 0, sizeof(user_info));

	user_info.nv_operation = NV_READ_TAG;
	user_info.nv_number = BATOVP_DATA_NV_NUM;
	user_info.valid_size = BATOVP_DATA_NV_SIZE;
	strncpy(user_info.nv_name, "BATOVP", sizeof(user_info.nv_name));
	user_info.nv_name[sizeof(user_info.nv_name) - 1] = '\0';
	ret = hisi_nve_direct_access(&user_info);
	if (ret != 0) {
		printk(KERN_ERR "batovp: nve_direct_access read error(%d)\n", ret);
		return -1;
	}
/*
	batovp_value[0] = (int8_t) user_info.nv_data[0];
	batovp_value[1] = (int8_t) user_info.nv_data[1];
	batovp_value[2] = (int8_t) user_info.nv_data[2];
	batovp_value[3] = (int8_t) user_info.nv_data[3];
	printk(KERN_DEBUG "batovp: read batovp_value (0x%x 0x%x 0x%x 0x%x =%d)\n",
		 batovp_value[0], batovp_value[1], batovp_value[2], batovp_value[3], *(int *)batovp_value);
*/
	*((char *)&batovp_value + 0) = (int8_t) user_info.nv_data[0];
	*((char *)&batovp_value + 1) = (int8_t) user_info.nv_data[1];
	*((char *)&batovp_value + 2) = (int8_t) user_info.nv_data[2];
	*((char *)&batovp_value + 3) = (int8_t) user_info.nv_data[3];
	printk(KERN_DEBUG "batovp: read batovp_value (0x%x 0x%x 0x%x 0x%x =%d)\n",
						*((char *)&batovp_value + 0), *((char *)&batovp_value + 1),
						*((char *)&batovp_value + 2), *((char *)&batovp_value + 3),
						batovp_value);

	return 0;
}
/*******************************************************************************************
Function:	write_batovp_value_to_nv
Description:  将temp数据写入NV 项中
Data Accessed:  无
Data Updated:   无
Input:        batttery ovp 次数
Output:         无
Return:         成功或者失败信息: 0->成功, -1->失败
*******************************************************************************************/
static int write_batovp_value_to_nv(char *temp)
{
	int ret = 0;
	struct hisi_nve_info_user user_info;

	if (temp == NULL) {
		printk(KERN_ERR "batovp: write_batovp_value_to_nv fail, invalid para!\n");
		return -1;
	}
	memset(&user_info, 0, sizeof(user_info));
	user_info.nv_operation = NV_WRITE_TAG;
	user_info.nv_number = BATOVP_DATA_NV_NUM;
	user_info.valid_size = BATOVP_DATA_NV_SIZE;
	strncpy(user_info.nv_name, "BATOVP", sizeof(user_info.nv_name));
	user_info.nv_name[sizeof(user_info.nv_name) - 1] = '\0';
	printk(KERN_DEBUG "batovp: nve_direct_access write temp (0x%x 0x%x 0x%x 0x%x)\n", temp[0],
		 temp[1], temp[2], temp[3]);
	user_info.nv_data[0] = temp[0];
	user_info.nv_data[1] = temp[1];
	user_info.nv_data[2] = temp[2];
	user_info.nv_data[3] = temp[3];
	ret = hisi_nve_direct_access(&user_info);
	if (ret != 0) {
		printk(KERN_ERR "batovp: nve_direct_access write error(%d)\n", ret);
		return -1;
	}
	printk(KERN_DEBUG "batovp: nve_direct_access write nv_data (0x%x 0x%x 0x%x 0x%x)\n",
		 user_info.nv_data[0], user_info.nv_data[1],
		 user_info.nv_data[2], user_info.nv_data[3]);

	return ret;
}

static ssize_t vbatovp_notify_store(struct device *dev,
                          struct device_attribute *attr, const char *buf, size_t size)
{
	struct vbat_ovp_device_info *di = g_di;
	di->vbat_ovp_status = 1;
	vbat_ovp_send_uevent();
	hwlog_info("%s:send uevent\n",__func__);
	hwlog_info("%s:vbat_ovp_status = %d done\n",__func__,di->vbat_ovp_status);
	return size;
}

static ssize_t vbatovp_notify_show(struct device *dev, struct device_attribute *attr,
                char *buf)
{
	return 0;
}

static DEVICE_ATTR(vbatovp_notify_ctrl, S_IRUGO | S_IWUSR, vbatovp_notify_show, vbatovp_notify_store);
static void vbat_ovp_test_print_flag(void)
{
	struct vbat_ovp_device_info *di = g_di;
	hwlog_info("%s:vbat_ovp_handle_start_flag = %d,vbat_ovp_set_input_current_flag = %d\n",__func__,di->vbat_ovp_handle_start_flag,vbat_ovp_set_input_current_flag);
	return;
}
int get_vbat_ovp_status(void)
{
	struct vbat_ovp_device_info *di = g_di;
	if (NULL == di)
		return 0;
	return di->vbat_ovp_status;
}
static int vbat_ovp_cut_off_battery(void)
{
	int i;

	hwlog_info("%s\n",__func__);
	struct vbat_ovp_device_info *di = g_di;
	di->vbat_ovp_status = 1;
	vbat_ovp_send_uevent();
	hwlog_info("%s:send uevent\n",__func__);
	if (FCP_STAGE_SUCESS == fcp_get_stage_status())
	{
		if(vbat_ovp_fcp_adaptor_reset())
		{
			hwlog_err("%s:vbat_ovp_fcp_adaptor_reset fail\n",__func__);
		}
	}
	vbat_ovp_set_input_current_flag = 1;
	charge_set_input_current(2000);
	for (i = 0; i < 5; i++)
	{
		hwlog_info("%s:i = %d\n",__func__,i);
		msleep(2000);
	}
	if (NULL != di->vbat_ovp_ops && NULL != di->vbat_ovp_ops->cut_off_battery)
	{
		if (di->vbat_ovp_ops->cut_off_battery())
		{
			hwlog_err("%s:cut off battery err!\n",__func__);
			return -1;
		}
	}
	vbat_ovp_test_print_flag();
	hwlog_info("%s:done!\n",__func__);
	return 0;
}

static int vbat_ovp_handle(void)
{
	struct vbat_ovp_device_info *di = g_di;
	int ret;
	int timeout;

	hwlog_info("%s\n",__func__);
	if(0 == read_batovp_value_from_nv())
	{
		batovp_value++;
		hwlog_info("%s:batovp_value = %d\n",__func__,batovp_value);
		ret = write_batovp_value_to_nv((char *)&batovp_value);
		if (ret)
		{
			hwlog_err("%s:write nv error!\n",__func__);
		}
	} else
	{
		hwlog_err("%s:read nv error!\n",__func__);
	}
#ifdef CONFIG_DIRECT_CHARGER
	vbat_ovp_exit_direct_charge(0);
	reinit_completion(&usb_detach_completion);
	while (0 == is_scp_stop_charging_complete())
	{
		timeout = wait_for_completion_timeout(&usb_detach_completion, msecs_to_jiffies(500));
		if (timeout)
		{
			vbat_ovp_exit_direct_charge(1);
			di->vbat_ovp_handle_start_flag = 0;
			hwlog_info("%s:usb detach,ls protect exit!\n",__func__);
			vbat_ovp_test_print_flag();
			return 0;
		}
	}
	if (SCP_ADAPTOR_DETECT_SUCC == scp_adaptor_detect())
	{
		ret = vbat_ovp_scp_handle();
		if (0 == ret)
		{
			hwlog_info("%s:usb detach, exit!\n",__func__);
			di->vbat_ovp_handle_start_flag = 0;
			vbat_ovp_exit_direct_charge(1);
			vbat_ovp_test_print_flag();
			return 0;
		}
	}
#endif
	ret = vbat_ovp_cut_off_battery();
	if (ret)
	{
		hwlog_err("%s:vbat_ovp_cut_off_battery err!\n",__func__);
		di->vbat_ovp_handle_start_flag = 0;
		return -1;
	}
	di->vbat_ovp_handle_start_flag = 0;
	vbat_ovp_test_print_flag();
	hwlog_info("%s:done\n",__func__);
	return 0;
}
static int vbat_ovp_notifier_call(struct notifier_block *fault_nb, unsigned long event, void *data)
{
	struct vbat_ovp_device_info *di = g_di;

	hwlog_info("%s\n",__func__);
	if (CHARGER_REMOVED == charge_get_charger_type())
	{
#ifdef CONFIG_DIRECT_CHARGER
		if(SCP_STAGE_SWITCH_DETECT < scp_get_stage_status())
		{
			hwlog_info("%s:in direct charge progress!\n",__func__);
		} else
		{
#endif
			hwlog_info("%s:charger removed!\n",__func__);
			return -1;
#ifdef CONFIG_DIRECT_CHARGER
		}
#endif
	}
#ifdef  CONFIG_HUAWEI_USB_SHORT_CIRCUIT_PROTECT
	if (is_in_uscp_mode())
	{
		hwlog_info("%s:is_in_uscp_mode!\n",__func__);
		return -1;
	}
#endif
	if (0 == di->vbat_ovp_handle_start_flag)
	{
		di->vbat_ovp_handle_start_flag = 1;
		schedule_delayed_work(&g_di->vbat_ovp_work,msecs_to_jiffies(VBAT_OVP_WORK_DELAY));
		hwlog_info("%s:start work!\n",__func__);
	}else
	{
		hwlog_info("%s:work not stop!\n",__func__);
	}
	vbat_ovp_test_print_flag();
	hwlog_info("%s:done\n",__func__);
	return 0;
}
int vbat_ovp_ops_register(struct vbat_ovp_ops *ops)
{
	int ret = 0;

	hwlog_info("%s\n",__func__);
	if (ops != NULL) {
		g_vbat_ovp_ops = ops;
	} else {
		hwlog_err("%s:vbat ovp ops register fail!\n",__func__);
		ret = -EPERM;
	}
	hwlog_info("%s:done!\n",__func__);
	return ret;
}
static int vbat_ovp_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct vbat_ovp_device_info *di;

	hwlog_info("%s\n",__func__);
	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di) {
		hwlog_err("alloc di failed\n");
		return -ENOMEM;
	}

	INIT_DELAYED_WORK(&di->vbat_ovp_work, vbat_ovp_handle);
	di->vbat_ovp_ops = g_vbat_ovp_ops;
	di->vbat_ovp_nb.notifier_call = vbat_ovp_notifier_call;
	di->vbat_ovp_handle_start_flag = 0;
	di->vbat_ovp_status = 0;
	platform_set_drvdata(pdev, di);

	ret = atomic_notifier_chain_register(&vbat_ovp_notifier_list, &di->vbat_ovp_nb);
	if (ret < 0)
	{
		hwlog_err("vbat_ovp_notifier_list failed\n");
		goto  vbat_ovp_fail_0;
	}
	g_di = di;
	device_create_file(&pdev->dev, &dev_attr_vbatovp_notify_ctrl);
	vbat_ovp_test_print_flag();
	hwlog_info("vbat_ovp probe ok!\n");
	return 0;

vbat_ovp_fail_0:
	platform_set_drvdata(pdev, NULL);
	kfree(di);
	di = NULL;
	return ret;
}

static int vbat_ovp_remove(struct platform_device *pdev)
{
	struct vbat_ovp_device_info *di = platform_get_drvdata(pdev);
	atomic_notifier_chain_unregister(&vbat_ovp_notifier_list, &di->vbat_ovp_nb);
	cancel_delayed_work(&di->vbat_ovp_work);
	return 0;
}
static void vbat_ovp_shutdown(struct platform_device *pdev)
{
	struct vbat_ovp_device_info *di = platform_get_drvdata(pdev);
	hwlog_info("%s ++\n", __func__);
	atomic_notifier_chain_unregister(&vbat_ovp_notifier_list, &di->vbat_ovp_nb);
	cancel_delayed_work(&di->vbat_ovp_work);
	hwlog_info("%s --\n", __func__);
	return;
}
static struct of_device_id vbat_ovp_match_table[] = {
	{
	 .compatible = "vbat,ovp",
	 .data = NULL,
	 },
	{
	 },
};
static struct platform_driver vbat_ovp_driver = {
	.probe = vbat_ovp_probe,
	.remove = vbat_ovp_remove,
	.shutdown = vbat_ovp_shutdown,
	.driver = {
		   .name = "vbat,ovp",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(vbat_ovp_match_table),
		   },
};
static int __init vbat_ovp_init(void)
{
	hwlog_info("%s:vbat_ovp init\n",__func__);
	return platform_driver_register(&vbat_ovp_driver);
}
static void __exit vbat_ovp_exit(void)
{
	platform_driver_unregister(&vbat_ovp_driver);
}

late_initcall(vbat_ovp_init);
module_exit(vbat_ovp_exit);

static int __init vbat_ovp_complete_init(void)
{
	hwlog_info("%s:init succ!!!\n", __func__);
	init_completion(&usb_detach_completion);
}
static void __exit vbat_ovp_complete_exit(void)
{
	hwlog_info("%s:exit succ!!!\n", __func__);
}

fs_initcall(vbat_ovp_complete_init);
module_exit(vbat_ovp_complete_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("vbat ovp module driver");
MODULE_AUTHOR("HUAWEI Inc");
