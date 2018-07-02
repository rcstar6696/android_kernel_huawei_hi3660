/*
	*drivers/power/huawei_charger.c
	*
	*huawei	charger	driver
	*
	*Copyright(C)2012-2015 HUAWEI, Inc.
	*Author: HUAWEI, Inc.
	*
	*This package is free software; you can	redistribute it and/or modify
	*it under the terms of the GNU General Public License version 2 as
	*published by the Free Software Foundation.
*/
/**********************************************************
*	Function: scp_retry_pre_operate
*	Discription: pre operate before retry scp enable
*	Parameters: di:charge_device_info,type : enum scp_retry_operate_type
*	return value: 0: scp pre operate success
*	              -1:scp pre operate fail
**********************************************************/
#include <huawei_platform/log/hw_log.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/wakelock.h>
#include <linux/power/hisi/coul/hisi_coul_drv.h>
#include <linux/power/hisi/hisi_bci_battery.h>
#include <huawei_platform/power/direct_charger.h>
#include <huawei_platform/power/huawei_charger.h>
#include <huawei_platform/power/vbat_ovp.h>
#ifdef CONFIG_TCPC_CLASS
#include <huawei_platform/usb/hw_pd_dev.h>
#endif
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif
#if defined CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif
#ifdef CONFIG_HUAWEI_USB_SHORT_CIRCUIT_PROTECT
#include <huawei_platform/power/usb_short_circuit_protect.h>
#endif
#ifdef CONFIG_SUPERSWITCH_FSC
#include <huawei_platform/usb/superswitch/fsc/core/hw_scp.h>
#endif
#define HWLOG_TAG direct_charge
#include "huawei_platform/dp_aux_switch/dp_aux_switch.h"

/*PT set adator test result*/
extern void chg_set_adaptor_test_result(enum adaptor_name charger_type, enum test_state result);
/*lint -save -e* */
HWLOG_REGIST();

ATOMIC_NOTIFIER_HEAD(direct_charge_fault_notifier_list);
static struct wake_lock direct_charge_lock;
static struct direct_charge_device *g_di;
static enum scp_stage_type scp_stage = SCP_STAGE_DEFAULT;
struct smart_charge_ops* g_scp_ops;
struct loadswitch_ops* g_ls_ops;
struct batinfo_ops* g_bi_ops;
struct direct_charge_cable_detect_ops* g_cable_detect_ops;
static int scp_stop_charging_complete_flag = 1;
static char dc_err_dsm_buff[CHARGE_DMDLOG_SIZE] = { 0 };

static int battery_temp_handler(int temp);
static int cutoff_normal_flag = 0;
static int quick_charge_flag = 0;
static int super_charge_flag = 0;
static int scp_adaptor_detect_flag = 0;
struct dc_volt_para_info orig_volt_para[DC_VOLT_LEVEL];
extern int pd_dpm_notify_direct_charge_status(bool dc);
/*lint -restore*/

int get_quick_charge_flag(void)
{
	return quick_charge_flag;
}
int get_super_charge_flag(void)
{
	return super_charge_flag;
}
static void direct_charge_send_quick_charge_uevent(void)
{
	quick_charge_flag = 1;
	super_charge_flag = 0;
	direct_charger_connect_send_uevent();
}
static void direct_charge_send_super_charge_uevent(void)
{
	quick_charge_flag = 0;
	super_charge_flag = 1;
	direct_charger_connect_send_uevent();
}
static void direct_charge_send_normal_charge_uevent(void)
{
	quick_charge_flag = 0;
	super_charge_flag = 0;
	direct_charger_connect_send_uevent();
}

int direct_charge_get_cutoff_normal_flag(void)
{
	return cutoff_normal_flag;
}

int cable_detect_ops_register(struct direct_charge_cable_detect_ops* ops)
{
	int ret = 0;

	if (ops != NULL)
	{
		g_cable_detect_ops = ops;
	}
	else
	{
		hwlog_err("cable detect ops register fail!\n");
		ret = -EPERM;
	}
	return ret;
}
int scp_ops_register(struct smart_charge_ops* ops)
{
	int ret = 0;

	if (ops != NULL)
	{
		g_scp_ops = ops;
	}
	else
	{
		hwlog_err("scp ops register fail!\n");
		ret = -EPERM;
	}
	return ret;
}

int loadswitch_ops_register(struct loadswitch_ops* ops)
{
	int ret = 0;

	if (ops != NULL)
	{
		g_ls_ops = ops;
	}
	else
	{
		hwlog_err("ls ops register fail!\n");
		ret = -EPERM;
	}
	return ret;
}

int batinfo_ops_register(struct batinfo_ops* ops)
{
	int ret = 0;

	if (ops != NULL)
	{
		g_bi_ops = ops;
	}
	else {
		hwlog_err("batinfo ops register fail!\n");
		ret = -EPERM;
	}
	return ret;
}

/**********************************************************
*  Function:       direct_charge_wake_lock
*  Description:   apply direct_charge wake_lock
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void direct_charge_wake_lock(void)
{
	if (!wake_lock_active(&direct_charge_lock)) {
		wake_lock(&direct_charge_lock);
		hwlog_info("direct_charge wake lock\n");
	}
}

/**********************************************************
*  Function:       direct_charge_wake_unlock
*  Description:   release direct_charge wake_lock
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void direct_charge_wake_unlock(void)
{
	if (wake_lock_active(&direct_charge_lock)) {
		wake_unlock(&direct_charge_lock);
		hwlog_info("direct_charge wake unlock\n");
	}
}
int is_in_scp_charging_stage(void)
{
	int ret = 0;

	if (SCP_STAGE_CHARGING == scp_stage)
	{
		hwlog_info("in direct charge progress!\n");
		return IN_SCP_CHARGING_STAGE;
	}
	return NOT_IN_SCP_CHARGING_STAGE;
}
/**********************************************************
*  Function:       scp_get_stage_status
*  Description:    get the stage of scp charge
*  Parameters:
*  return value:   stage
**********************************************************/
enum scp_stage_type scp_get_stage_status(void)
{
	return scp_stage;
}

/**********************************************************
*  Function:       scp_set_stage_status
*  Description:    set the stage of scp charge
*  Parameters:     stage type
*  return value:   NULL
**********************************************************/
static void scp_set_stage_status(enum scp_stage_type stage_type)
{
	scp_stage = stage_type;
}
static int is_direct_charge_ops_valid(struct direct_charge_device *di)
{
	if (NULL == di)
	{
		hwlog_err("[%s]:di is NULL!\n", __func__);
		return INVALID;
	}
	if ((NULL == di->scp_ops) || (NULL == di->scp_ops->is_support_scp)
		||(NULL	== di->scp_ops->scp_init) || (NULL == di->scp_ops->scp_adaptor_detect)
		||(NULL	== di->scp_ops->scp_set_adaptor_voltage) || (NULL == di->scp_ops->scp_get_adaptor_voltage)
		||(NULL	== di->scp_ops->scp_get_adaptor_current) || (NULL == di->scp_ops->scp_set_adaptor_current)
		||(NULL	== di->scp_ops->scp_adaptor_reset) || (NULL == di->scp_ops->scp_chip_reset)
		||(NULL	== di->scp_ops->scp_stop_charge_config)	|| (NULL == di->scp_ops->scp_get_adaptor_status)
		||(NULL	== di->scp_ops->scp_get_chip_status) || (NULL == di->scp_ops->scp_exit)
		||(NULL == di->scp_ops->scp_get_adaptor_max_current) || (NULL == di->scp_ops->scp_cable_detect)
		||(NULL == di->scp_ops->scp_get_adapter_vendor_id )|| (NULL == di->scp_ops->scp_power_enable))
	{
		hwlog_err("scp ops is null!\n");
		return	INVALID;
	}

	if ((NULL == di->ls_ops) || (NULL == di->ls_ops->ls_init) || (NULL == di->ls_ops->ls_exit)
		|| (NULL == di->ls_ops->is_ls_close) || (NULL == di->ls_ops->get_ls_id) || (NULL == di->ls_ops->ls_enable)
		||(NULL == di->ls_ops->watchdog_config_ms))
	{
		hwlog_err("ls ops is null!\n");
		return	INVALID;
	}

	if ((NULL == di->bi_ops) || (NULL == di->bi_ops->get_bat_current) || (NULL == di->bi_ops->exit)
		|| (NULL == di->bi_ops->get_bat_btb_voltage) || (NULL == di->bi_ops->get_vbus_voltage))
	{
		hwlog_err("bi ops is null!\n");
		return	INVALID;
	}
	return	VALID;
}
static int get_bat_voltage(struct direct_charge_device* di)
{
	int btb_vol = 0;
	int package_vol = 0;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };

	btb_vol = di->bi_ops->get_bat_btb_voltage();
	package_vol = di->bi_ops->get_bat_package_voltage();
	if (btb_vol < 0 && package_vol < 0)
	{
		snprintf(tmp_buf, sizeof(tmp_buf), "[%s]:error\n", __func__);
		hwlog_err("%s", tmp_buf);
		strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
		di->scp_stop_charging_flag_error = 1;
		return 0;
	}

	return btb_vol > package_vol ? btb_vol : package_vol;
}

static int get_bat_current(struct direct_charge_device* di)
{
	int bat_curr = 0;
	int ret;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };

	ret = di->bi_ops->get_bat_current(&bat_curr);
	if (ret < 0)
	{
		snprintf(tmp_buf, sizeof(tmp_buf), "[%s]:error\n", __func__);
		hwlog_err("%s", tmp_buf);
		strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
		di->scp_stop_charging_flag_error = 1;
	}
	return bat_curr;
}

int get_ls_vbus(void)
{
	struct direct_charge_device *di = g_di;
	int vbus = 0;
	int ret;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };

	ret = di->bi_ops->get_vbus_voltage(&vbus);
	if (ret < 0)
	{
		snprintf(tmp_buf, sizeof(tmp_buf), "[%s]:error\n", __func__);
		hwlog_err("%s", tmp_buf);
		strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
		di->scp_stop_charging_flag_error = 1;
	}
	return vbus;
}

static int get_ls_ibus(struct direct_charge_device *di)
{
	int ibus = 0;
	int ret;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };

	ret = di->bi_ops->get_ls_ibus(&ibus);
	if (ret < 0)
	{
		snprintf(tmp_buf, sizeof(tmp_buf), "[%s]:error\n", __func__);
		hwlog_err("%s", tmp_buf);
		strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
		di->scp_stop_charging_flag_error = 1;
	}
	return ibus;
}

static int get_ls_temp(struct direct_charge_device *di)
{
	int temp = 0;
	int ret;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };

	ret = di->bi_ops->get_ls_temp(&temp);
	if (ret < 0)
	{
		snprintf(tmp_buf, sizeof(tmp_buf), "[%s]:error\n", __func__);
		hwlog_err("%s", tmp_buf);
		strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
		di->scp_stop_charging_flag_error = 1;
	}
	return temp;
}

static int is_ls_close(struct direct_charge_device *di)
{
	int ret;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };

	ret = di->ls_ops->is_ls_close();
	if (ret)
	{
		snprintf(tmp_buf, sizeof(tmp_buf), "[%s]:ls is close\n", __func__);
		hwlog_err("%s", tmp_buf);
		strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
		di->scp_stop_charging_flag_error = 1;
		return 1;
	}
	return 0;
}
void scp_set_stop_charging_flag(int flag)
{
	struct direct_charge_device *di = g_di;
	if (!di) {
		hwlog_err("[%s]:di is null, return\n", __func__);
		return;
	}
	di->scp_stop_charging_flag_error = flag;
}

int is_scp_stop_charging_complete(void)
{
	return (1 == scp_stop_charging_complete_flag);
}

static int get_adaptor_voltage(struct direct_charge_device* di)
{
	int adaptor_vol = -1;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };

	if(di->scp_stop_charging_flag_error)
		return -1;
	if (di->scp_ops->scp_get_adaptor_voltage)
	{
 		adaptor_vol = di->scp_ops->scp_get_adaptor_voltage();
		if (adaptor_vol < 0)
		{
			snprintf(tmp_buf, sizeof(tmp_buf), "[%s]:error\n", __func__);
			hwlog_err("%s", tmp_buf);
			strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
			di->scp_stop_charging_flag_error = 1;
		}
	}
	return adaptor_vol;
}

static int get_adaptor_current(struct direct_charge_device* di)
{
	int adaptor_cur = -1;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };

	if(di->scp_stop_charging_flag_error)
		return -1;
	switch(di->adaptor_vendor_id)
	{
		case IWATT_ADAPTER:
			adaptor_cur = get_ls_ibus(di);
			return adaptor_cur;
		default:
			if (di->scp_ops->scp_get_adaptor_current)
			{
				adaptor_cur = di->scp_ops->scp_get_adaptor_current();
				if (adaptor_cur < 0)
				{
					snprintf(tmp_buf, sizeof(tmp_buf), "[%s]:error\n", __func__);
					hwlog_err("%s", tmp_buf);
					strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
					di->scp_stop_charging_flag_error = 1;
				}
			}
			return adaptor_cur;
	}
}
static int get_adaptor_current_for_vbat_ovp(struct direct_charge_device* di)
{
	int adaptor_cur = -1;

	switch(di->adaptor_vendor_id)
	{
		case IWATT_ADAPTER:
			adaptor_cur = get_ls_ibus(di);
			return adaptor_cur;
		default:
			if (di->scp_ops->scp_get_adaptor_current)
			{
				adaptor_cur = di->scp_ops->scp_get_adaptor_current();
			}
			return adaptor_cur;
	}
}


static int get_adaptor_current_set(struct direct_charge_device* di)
{
	int adaptor_cur_set = -1;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };

	if(di->scp_stop_charging_flag_error)
		return -1;
	if (di->scp_ops->scp_get_adaptor_current_set)
	{
 		adaptor_cur_set = di->scp_ops->scp_get_adaptor_current_set();
		if (adaptor_cur_set < 0)
		{
			snprintf(tmp_buf, sizeof(tmp_buf), "[%s]:error\n", __func__);
			hwlog_err("%s", tmp_buf);
			strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
			di->scp_stop_charging_flag_error = 1;
		}
	}
	return adaptor_cur_set;
}

static int get_adaptor_max_current(struct direct_charge_device* di)
{
	int adaptor_max_cur = -1;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };

	if(di->scp_stop_charging_flag_error)
		return -1;
	if (di->scp_ops->scp_get_adaptor_max_current)
	{
		adaptor_max_cur = di->scp_ops->scp_get_adaptor_max_current();
		if (adaptor_max_cur < 0)
		{
			snprintf(tmp_buf, sizeof(tmp_buf), "[%s]:error\n", __func__);
			hwlog_err("%s", tmp_buf);
			strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
			di->scp_stop_charging_flag_error = 1;
		}
	}
	return adaptor_max_cur;
}

static void set_adaptor_voltage(struct direct_charge_device* di)
{
	int ret = -1;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };

	if(di->scp_stop_charging_flag_error)
		return;
	if (di->scp_ops->scp_set_adaptor_voltage)
	{
		hwlog_info("set_adaptor_vol = %d!\n", di->adaptor_vset);
		if (di->adaptor_vset >= di->max_adaptor_vset)
		{
			di->adaptor_vset = di->max_adaptor_vset;
		}
		ret = di->scp_ops->scp_set_adaptor_voltage(di->adaptor_vset);
		if (ret)
		{
			snprintf(tmp_buf, sizeof(tmp_buf), "[%s]:error\n", __func__);
			hwlog_err("%s", tmp_buf);
			strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
			di->scp_stop_charging_flag_error = 1;
		}
	}
}

static void set_adaptor_current(struct direct_charge_device* di)
{
	int ret = -1;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };

	if(di->scp_stop_charging_flag_error)
		return;
	if (di->scp_ops->scp_set_adaptor_current)
	{
		hwlog_info("set_adaptor_cur = %d!\n", di->adaptor_iset);
		if (di->adaptor_iset >= di->max_adaptor_iset)
		{
			di->adaptor_iset = di->max_adaptor_iset;
		}
		ret = di->scp_ops->scp_set_adaptor_current(di->adaptor_iset);
		if (ret)
		{
			snprintf(tmp_buf, sizeof(tmp_buf), "[%s]:error\n", __func__);
			hwlog_err("%s", tmp_buf);
			strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
			di->scp_stop_charging_flag_error = 1;
		}
	}
}

static int get_adaptor_temp(struct direct_charge_device* di)
{
	int ret = -1;
	int temp = 0;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };

	if(di->scp_stop_charging_flag_error)
		return 0;
	if (di->scp_ops->scp_get_adaptor_temp)
	{
		ret = di->scp_ops->scp_get_adaptor_temp(&temp);
		if (ret)
		{
			snprintf(tmp_buf, sizeof(tmp_buf), "[%s]:error\n", __func__);
			hwlog_err("%s", tmp_buf);
			strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
			di->scp_stop_charging_flag_error = 1;
		}
	}
	return temp;
}
static int can_battery_temp_do_direct_charge(struct direct_charge_device *di)
{
	int bat_temp = hisi_battery_temperature();
	int bat_temp_cur_max = battery_temp_handler(bat_temp);

	if (0 == bat_temp_cur_max)
	{
		hwlog_info("%s : temp = %d, can not do direct charging \n", __func__, bat_temp);
		return 0;
	}
	return	1;
}
static int can_battery_vol_do_direct_charge(struct direct_charge_device *di)
{
	int bat_vol = hisi_battery_voltage();

	if (bat_vol < di->min_dc_bat_vol || bat_vol > di->max_dc_bat_vol)
	{
		hwlog_info("%s : vol = %d, can not do direct charging \n", __func__, bat_vol);
		return 0;
	}
	return	1;
}

static int scp_retry_pre_operate(enum scp_retry_operate_type type, struct direct_charge_device *di)
{
	int ret	= -1;

	switch (type) {
	case SCP_RETRY_OPERATE_RESET_ADAPTER:
		if (NULL != di->scp_ops->scp_adaptor_reset)
		{
			hwlog_info("send scp adapter reset cmd \n");
			ret = di->scp_ops->scp_adaptor_reset();
		}
		else
		{
			ret = -1;
		}
		break;
	case SCP_RETRY_OPERATE_RESET_CHIP:
		if (NULL != di->scp_ops->scp_chip_reset)
		{
			hwlog_info("scp_chip_reset \n");
			ret = di->scp_ops->scp_chip_reset();
			msleep(2000);
		}
		else
		{
			ret = -1;
		}
		break;
	default:
		break;
	}
	return ret;
}

static void scp_power_control(struct direct_charge_device* di, int enable)
{
	int ret;
	/* power control for SCP communication */
	ret = di->scp_ops->scp_power_enable(enable);
	if(ret)
	{
		hwlog_err("[%s]:fail, status = %d!\n", __func__,enable);
		return;
	}
	hwlog_err("[%s]: success, status = %d!\n", __func__,enable);
	return;
}
static int restore_normal_charge(struct direct_charge_device* di)
{
	int ret;

	msleep(WAIT_LS_DISCHARGE); /*need to wait loadswitch discharge*/
	/*no need to check the return val, here when ovp_en set fail ,we do note return*/
	dp_aux_ldo_supply_disable(DP_AUX_LDO_CTRL_DIRECT_CHARGE);
	ret = gpio_direction_input(di->ovp_en);
	if (ret)
		ret = FAIL;
	else
		ret = SUCC;
	restore_pluggin_pluggout_interrupt();
	return ret;
}
static int cutoff_normal_charge(struct direct_charge_device* di)
{
	int ret = SUCC;

	cutoff_normal_flag = 1;
	ignore_pluggin_and_pluggout_interrupt();
	dp_aux_ldo_supply_enable(DP_AUX_LDO_CTRL_DIRECT_CHARGE);
	if (di->use_gpio_for_ovp_en) {
		if (di->scp_work_on_charger) {
			scp_power_control(di, ENABLE);
			charge_set_hiz_enable(SET_HIZ_ENABLE);
		}
		msleep(100);
		/*no need to check the return val, here when ovp_en set fail ,we still return succ*/
		gpio_direction_output(di->ovp_en, 1);
	} else {
#ifdef CONFIG_SUPERSWITCH_FSC
		ret = FUSB3601_vout_enable(0);
#endif
	}
	return ret;
}
int scp_adaptor_set_output_enable(int enable)
{
	int ret, i;
	struct direct_charge_device *di = g_di;

    	for (i = 0; i < 3; i++) {
		ret = di->scp_ops->scp_adaptor_output_enable(enable);
		if (!ret)
			break;
	}
	if (ret) {
		hwlog_err("[%s]: direct charge set output voltage fail !!\n", __func__);
	}
	return ret;
}
int scp_adaptor_detect(void)
{
	int ret;
	int i;
	struct direct_charge_device *di = g_di;

	if (NULL == di || NULL == di->scp_ops || NULL == di->scp_ops->scp_adaptor_detect)
	{
		hwlog_err("[%s]bad scp adaptor detect ops!\n", __func__);
		return -1;
	}
	ret = di->scp_ops->scp_adaptor_detect();
	if (SCP_ADAPTOR_DETECT_FAIL == ret)
	{
		for (i = 0; i < 3 && SCP_ADAPTOR_DETECT_FAIL == ret; ++i)
		{
			if ((scp_retry_pre_operate(SCP_RETRY_OPERATE_RESET_ADAPTER, di)) < 0)
			{
				hwlog_err("reset adapter failed	\n");
				break;
			}
			ret = di->scp_ops->scp_adaptor_detect();
		}
		if (SCP_ADAPTOR_DETECT_FAIL == ret)
		{
			/* reset scp chip and try again	*/
			if ((scp_retry_pre_operate(SCP_RETRY_OPERATE_RESET_CHIP, di)) == 0)
			{
				ret = di->scp_ops->scp_adaptor_detect();
			}
			else
			{
				hwlog_err("%s : scp_retry_pre_operate failed \n", __func__);
			}
		}
	}
	hwlog_info("%s : scp adaptor detect ret = %d \n", __func__, ret);
	return ret;
}
static void scp_adaptor_and_cable_detect(struct direct_charge_device *di)
{
	int ret;

	ret = scp_adaptor_detect();
	/*try again in the next loop*/
	if (SCP_ADAPTOR_DETECT_SUCC != ret) {
		scp_set_stage_status(SCP_STAGE_DEFAULT);
		return;
	}
	scp_adaptor_detect_flag = SCP_ADAPTOR_DETECT;
	chg_set_adaptor_test_result(TYPE_SCP,DETECT_SUCC);
	/*if detect result is other charger, in the next loop, we will not try again*/

	if (di->scp_cable_detect_enable)
	{
		ret = di->scp_ops->scp_cable_detect();
		if (ret)
			hwlog_err("%s : cable detect fail = %d\n", __func__, ret);
	}
	if (SUCC == ret)
	{
		if (NULL == di || NULL == di->direct_charge_cable_detect || NULL == di->direct_charge_cable_detect->direct_charge_cable_detect)
		{
			hwlog_err("[%s]NULL direct charge cable detect ops!\n", __func__);
			scp_set_stage_status(SCP_STAGE_DEFAULT);
			return;
		}
		ret = di->direct_charge_cable_detect->direct_charge_cable_detect();
		if(ret)
		{
			di->cc_cable_detect_ok = 0;
			di->full_path_res_threshold = di->full_path_res_max;
			direct_charge_send_normal_charge_uevent();
			hwlog_info("%s:cable detect fail!\n",__func__);
		}else
		{
			di->cc_cable_detect_ok = 1;
			di->full_path_res_threshold = di->standard_cable_full_path_res_max;
			direct_charge_send_super_charge_uevent();
			hwlog_info("%s:cable detect ok!\n",__func__);
		}
		ret = di->scp_ops->scp_get_adaptor_info(&(di->adp_info));
		if (ret)
		{
			hwlog_err("get adaptor info failed\n");
		}
		di->adaptor_vendor_id = di->scp_ops->scp_get_adapter_vendor_id();
		if (di->adaptor_vendor_id < 0)
		{
			hwlog_err("get adaptor vendor id failed\n");
			scp_set_stage_status(SCP_STAGE_DEFAULT);
			return;
		}
		hwlog_info("b_adp_type= 0x%x\n", di->adp_info.b_adp_type);
		hwlog_info("vendor_id_h= 0x%x\n", di->adp_info.vendor_id_h);
		hwlog_info("vendor_id_l= 0x%x\n", di->adp_info.vendor_id_l);
		hwlog_info("module_id_h= 0x%x\n", di->adp_info.module_id_h);
		hwlog_info("module_id_l= 0x%x\n", di->adp_info.module_id_l);
		hwlog_info("serrial_no_h= 0x%x\n", di->adp_info.serrial_no_h);
		hwlog_info("serrial_no_l= 0x%x\n", di->adp_info.serrial_no_l);
		hwlog_info("pchip_id= 0x%x\n", di->adp_info.pchip_id);
		hwlog_info("hwver= 0x%x\n", di->adp_info.hwver);
		hwlog_info("fwver_h= 0x%x\n", di->adp_info.fwver_h);
		hwlog_info("fwver_l= 0x%x\n", di->adp_info.fwver_l);
		hwlog_info("adaptor_vendor_id= 0x%x\n", di->adaptor_vendor_id);
		scp_set_stage_status(SCP_STAGE_SWITCH_DETECT);
	}
}


static int do_adpator_voltage_accuracy_check(struct direct_charge_device *di)
{
	int adp_vol;
	int vol_err;
	int i;
	int ret;
	int bat_curr;
	char buf[1024] = { 0 };
	char dsm_buf[CHARGE_DMDLOG_SIZE] = { 0 };
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };

	di->adaptor_vset = 4400;
	set_adaptor_voltage(di);
	usleep_range(500000,501000);
	ret = di->bi_ops->get_bat_current(&bat_curr);/*keep communication with the adaptor within 1 second*/
	if (ret)
	{
		hwlog_err("get_bat_current fail!\n");
	}
	hwlog_info("To keep communication with the adaptor:bat_curr = %d!\n",bat_curr);
	for (i = 0; i < 3; ++i)
	{
		adp_vol = get_adaptor_voltage(di);
		if (adp_vol < 0)
		{
			snprintf(tmp_buf, sizeof(tmp_buf), "%s: get adptor voltage(%dmV) fail!\n", __func__, adp_vol);
			strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
			hwlog_err("%s", tmp_buf);
			return -1;
		}
		vol_err = adp_vol - 4400;
		if (vol_err < 0)
			vol_err = -vol_err;
		snprintf(tmp_buf, sizeof(tmp_buf),
			"Verr = %d Verr_th = %d, Vset = %d, Vread = %d, Vbus = %d!\n",
			 vol_err, di->vol_err_th, di->adaptor_vset, adp_vol, get_charger_vbus_vol());
		strncat(dsm_buf, tmp_buf, strlen(tmp_buf));
		hwlog_info("%s", tmp_buf);
		if (vol_err > di->vol_err_th)
		{
			snprintf(tmp_buf, sizeof(tmp_buf), "Verr(%d) > Verr_th(%d)\n",
				vol_err, di->vol_err_th);
			strncat(dsm_buf, tmp_buf, strlen(tmp_buf));
			strncat(dc_err_dsm_buff, dsm_buf, strlen(dsm_buf));
			dsm_report(DSM_DIRECT_CHARGE_VOL_ACCURACY, dsm_buf);
			return -1;
		}
	}
	return 0;
}
static int do_full_path_resistance_check(struct direct_charge_device *di)
{
	int adp_vol;
	int iadapt;
	int vbus_vol;
	int ibus;
	int delta_vol;
	int r;
	int ret;
	int i;
	int sum = 0;
	char dsm_buf[CHARGE_DMDLOG_SIZE] = { 0 };
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };

	usleep_range(500000,501000);
	adp_vol = get_adaptor_voltage(di); /*keep communication with the adaptor within 1 second*/
	if (adp_vol < 0)
	{
		snprintf(tmp_buf, sizeof(tmp_buf), "%s: get adptor voltage fail, adp_vol = %d\n", __func__, adp_vol);
		hwlog_err("%s", tmp_buf);
		strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
		return -1;
	}
	ret = is_ls_close(di);/*keep communication with loadswitchwithin 1 second*/
	if (ret)
	{
		snprintf(tmp_buf, sizeof(tmp_buf), "%s: ls is close!\n", __func__);
		hwlog_err("%s", tmp_buf);
		strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
		return -1;
	}
	usleep_range(400000,401000);
	for (i = 0; i < 3; ++i)
	{
		ret = di->bi_ops->get_ls_ibus(&ibus);
		if (ret)
		{
			snprintf(tmp_buf, sizeof(tmp_buf), "%s:get ibus fail, ls_ibus = %d\n", __func__, ibus);
			hwlog_err("%s", tmp_buf);
			strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
			return -1;
		}
		ret = di->bi_ops->get_vbus_voltage(&vbus_vol);
		if (ret)
		{
			snprintf(tmp_buf, sizeof(tmp_buf), "%s:get vbus vol fail, vbus = %d\n", __func__, vbus_vol);
			hwlog_err("%s", tmp_buf);
			strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
			return -1;
		}
		adp_vol = get_adaptor_voltage(di);
		if (adp_vol < 0)
		{
			snprintf(tmp_buf, sizeof(tmp_buf), "%s: get adptor voltage fail, adp_vol = %d\n", __func__, adp_vol);
			hwlog_err("%s", tmp_buf);
			strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
			return -1;
		}
		iadapt = get_adaptor_current(di);
		if (iadapt < 0)
		{
			snprintf(tmp_buf, sizeof(tmp_buf), "%s: get adptor current fail, iadapt = %d\n", __func__, iadapt);
			hwlog_err("%s", tmp_buf);
			strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
			return -1;
		}
		delta_vol = adp_vol - vbus_vol;
		r = delta_vol* 1000 / ibus;
		snprintf(tmp_buf, sizeof(tmp_buf),
			"full_res[%d] = %d, vadapt =%d, iadapt = %d, vbus = %d, ibus = %d\n",
			i, r, adp_vol, iadapt, vbus_vol, ibus);
		strncat(dsm_buf, tmp_buf, strlen(tmp_buf));
		hwlog_info("%s", tmp_buf);
		sum += r;
	}
	r = sum / 3;
	di->full_path_resistance = r;
	hwlog_info("di->full_path_res_threshold = %d\n", di->full_path_res_threshold);
	if (r >= -di->full_path_res_threshold && r <= di->full_path_res_threshold)
	{
		if (0 == di->cc_cable_detect_ok)
		{
			direct_charge_send_quick_charge_uevent();
		}
		return 0;
	}
	hwlog_err("full path resistance = %d is out of[%d, %d]\n", r, -di->full_path_res_threshold, di->full_path_res_threshold);
	if (1 == di->cc_cable_detect_ok)
	{
		snprintf(tmp_buf, sizeof(tmp_buf), "full_res(%d) is out of[%d, %d]\n",
			r, -di->full_path_res_threshold, di->full_path_res_threshold);
		hwlog_err("%s", tmp_buf);
		strncat(dsm_buf, tmp_buf, strlen(tmp_buf));
		dsm_report(DSM_DIRECT_CHARGE_FULL_PATH_RESISTANCE, dsm_buf);
		strncat(dc_err_dsm_buff, dsm_buf, strlen(dsm_buf));
	}
	return -1;
}
static int do_usb_port_leakage_current_check(struct direct_charge_device *di)
{
	int iadapt;
	int  leak_current;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };

	switch(di->adaptor_vendor_id)
	{
		case IWATT_ADAPTER:
			di->adaptor_iset = 400;
			set_adaptor_current(di);
			msleep(100);
			leak_current = di->scp_ops->scp_get_usb_port_leakage_current_info();
			if (leak_current)
			{
				snprintf(tmp_buf, sizeof(tmp_buf),"iwatt_adaptor usb port current leak, charger_vbus_vol = %d\n",
					get_charger_vbus_vol());
				goto FuncEnd;
			}
			return 0;
		default:
			iadapt = get_adaptor_current(di);
			hwlog_info("[%s]:iadapt = %d, charger_vbus_vol = %d, ls_ibus = %d\n", __func__, iadapt, get_charger_vbus_vol(), get_ls_ibus(di));
			if (iadapt < 0)
			{
				hwlog_err("get adptor current fail!\n");
				return -1;
			}
			if (iadapt > di->adaptor_leakage_current_th)
			{
				snprintf(tmp_buf, sizeof(tmp_buf), "%s: idapt(%d) > adaptor_leakage_current_th(%d),  charger_vbus_vol = %d\n",
					__func__, iadapt, di->adaptor_leakage_current_th, get_charger_vbus_vol());
				goto FuncEnd;
			}
			return 0;
	}

FuncEnd:
	hwlog_err("%s", tmp_buf);
	strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
	dsm_report(DSM_DIRECT_CHARGE_USB_PORT_LEAKAGE_CURRENT, tmp_buf);
	return -1;
}
static int open_direct_charge_path(struct direct_charge_device *di)
{
	int bat_vol;
	int ls_ibus;
	int ret;
	int adjust_times = MAX_TIMES_FOR_SET_ADAPTER_VOL_20;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };
	int bat_capacity;

	bat_capacity = hisi_battery_capacity();
	bat_vol = get_bat_voltage(di);
	di->adaptor_vset = bat_vol + ADAPTER_VOL_DIFFRENCE_300_MV;
	if (MAX_ADAPTER_VOL_4400_MV < di->adaptor_vset)
	{
		di->adaptor_vset = MAX_ADAPTER_VOL_4400_MV;
	}
	di->adaptor_iset = CURRENT_SET_FOR_RES_DETECT_1000_MA;
	set_adaptor_voltage(di);
	set_adaptor_current(di);
	msleep(50);
	ret = di->ls_ops->ls_enable(1);
	if (ret)
	{
		snprintf(tmp_buf, sizeof(tmp_buf), "%s: ls enable fail!\n", __func__);
		goto FuncEnd;
	}
	msleep(10);
	ls_ibus = get_ls_ibus(di);
	hwlog_info("%s:ls_ibus = %d!\n",__func__,ls_ibus);
	while (MIN_CURRENT_FOR_RES_DETECT_800_MA > ls_ibus)
	{
		bat_vol = get_bat_voltage(di);
		if (MAX_VOL_FOR_BATTERY_4360_MV < bat_vol)
		{
			snprintf(tmp_buf, sizeof(tmp_buf), "%s: adaptor_vset(%d) too high, ls_ibus = %d\n",
				__func__, bat_vol, ls_ibus);
			goto FuncEnd;
		}
		adjust_times--;
		hwlog_info("%s: adjust_times = %d!\n", __func__,adjust_times);
		if(0 == adjust_times)
		{
			snprintf(tmp_buf, sizeof(tmp_buf), "%s: try too many times, ls_ibus = %d\n",
				__func__, ls_ibus);
			goto FuncEnd;
		}
		di->adaptor_vset += MIN_ADAPTER_VOL_STEP_20_MV;
		hwlog_info("%s:adaptor_vset = %d!\n",__func__,di->adaptor_vset);
		set_adaptor_voltage(di);
		msleep(5);
		ls_ibus = get_ls_ibus(di);
		hwlog_info("%s:ls_ibus = %d!\n",__func__,ls_ibus);
	}
	return 0;

FuncEnd:
	hwlog_err("%s", tmp_buf);
	strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
	if(bat_capacity >= BATTERY_CAPACITY_HIGH_TH)
	{
		di->dc_open_retry_cnt += 1;
		hwlog_info("%s:open direct charge path fail, battery capacity is %d, over threshold! \n",__func__,bat_capacity);
	}
	return -1;
}
static int scp_security_check(struct direct_charge_device *di)
{
	int ret;
	di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_ADAPTOR_VOLTAGE_ACCURACY;
	ret = do_adpator_voltage_accuracy_check(di);
	if (ret)
	{
		hwlog_err("adaptor voltage accuracy check fail!\n");
		return -1;
	}
	di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_USB_PORT_LEAKAGE_CURRENT;
	ret = do_usb_port_leakage_current_check(di);
	if (ret)
	{
		hwlog_err("usb port leakage current check fail!\n");
		return -1;
	}
	di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_OPEN_CHARGE_PATH;
	ret = open_direct_charge_path(di);
	if (ret)
	{
		hwlog_err("open direct charge path fail!\n");
		return -1;
	}
	di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_FULL_REISISTANCE;
	ret = do_full_path_resistance_check(di);
	if (ret)
	{
		hwlog_err("full path resiststance check fail!\n");
		return -1;
	}
	return	ret;
}


static int is_support_scp(struct direct_charge_device* di)
{
	/*check	whether	support	scp detect*/
	if (di->scp_ops && di->scp_ops->is_support_scp)
	{
		/*return 0 means support scp*/
		if (di->scp_ops->is_support_scp())
		{
			hwlog_err("not support scp!\n");
			return 1;
		}
		scp_set_stage_status(SCP_STAGE_ADAPTER_DETECT);
		return 0;
	}
	return 1;
}
static int scp_direct_charge_init(struct direct_charge_device* di)
{
	int ret;
	struct scp_init_data sid;
	sid.scp_mode_enable = 1;
	sid.vset_boundary = di->max_adaptor_vset;
	sid.iset_boundary = di->max_adaptor_iset;
	sid.init_adaptor_voltage = 4400;
	sid.watchdog_timer = 3;
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };

	pd_dpm_notify_direct_charge_status(true);
	ret = di->scp_ops->scp_init(&sid);
	if (ret)
	{
		snprintf(tmp_buf, sizeof(tmp_buf), "%s: scp init fail!\n", __func__);
		goto FuncEnd;
	}
	ret = di->ls_ops->ls_init();
	if (ret)
	{
		snprintf(tmp_buf, sizeof(tmp_buf), "%s: ls init fail!\n", __func__);
		goto FuncEnd;
	}
	ret = di->bi_ops->init();
	if (ret)
	{
		snprintf(tmp_buf, sizeof(tmp_buf), "%s: bi init fail!\n", __func__);
		goto FuncEnd;
	}
	hwlog_info("direct charge init succ!\n");
	return SUCC;
FuncEnd:
	pd_dpm_notify_direct_charge_status(false);
	hwlog_err("%s", tmp_buf);
	strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
	return FAIL;
}
static void scp_stop_charging(void)
{
	int ret;
	struct direct_charge_device *di = g_di;
	int vbus_vol = 0;
	int vbat;

	scp_adaptor_detect_flag = SCP_ADAPTOR_NOT_DETECT;
	if (di->scp_stop_charging_flag_error)
	{
		di->error_cnt += 1;
	}
	if (di->scp_stop_charging_flag_error || di->scp_stop_charging_flag_info || (0 == di->sysfs_enable_charger) ||(0 == di->vbat_ovp_enable_charger))
	{
		scp_set_stage_status(SCP_STAGE_DEFAULT);
	}
	else
	{
		scp_set_stage_status(SCP_STAGE_CHARGE_DONE);
	}
	ret = di->ls_ops->ls_enable(0);
	if (ret)
	{
		hwlog_err("[%s]: ls enable fail!\n", __func__);
	}
	pd_dpm_notify_direct_charge_status(false);
	ret = di->scp_ops->scp_exit(di);
	if (ret)
	{
		hwlog_err("[%s]: scp exit fail!\n", __func__);
	}
	if (di->scp_work_on_charger)
	{
		scp_power_control(di, DISABLE);
		charge_set_hiz_enable(SET_HIZ_DISABLE);
	}
	if (di->ls_ops->ls_discharge)
	{
		ret = di->ls_ops->ls_discharge(1);
		if (ret)
		{
			hwlog_err("[%s]: ls discharge fail!\n", __func__);
		}
		else
		{
			hwlog_info("[%s]: ls discharge succ!\n", __func__);
		}
	}
	direct_charger_disconnect_update_charger_type();
	if (FAIL == restore_normal_charge(di))
	{
		hwlog_info("%s: restore normal charge fail!\n", __func__);
	}
	ret = di->ls_ops->ls_discharge(0);
	msleep(20);
	ret = di->bi_ops->get_vbus_voltage(&vbus_vol);
	if (ret)
	{
		hwlog_err("[%s]: get vbus vol fail!\n", __func__);
	}
	hwlog_info("%s: vbus_vol = %d!\n", __func__, vbus_vol);
	vbat = get_bat_voltage(di);
	hwlog_info("%s: vbat = %d!\n", __func__, vbat);

	if ((vbus_vol < VBUS_ON_THRESHOLD) || (vbat - vbus_vol) > VBAT_VBUS_DIFFERENCE)
	{
		hwlog_info("%s: vbat - vbus_vol = %d!\n", __func__, vbat - vbus_vol);
		if (!strstr(saved_command_line, "androidboot.swtype=factory")) {
			di->sysfs_enable_charger = 1;
		}
		di->error_cnt = 0;
		di->dc_open_retry_cnt = 0;
		hwlog_info("%s:direct charger disconnect!\n", __func__);
		di->full_path_resistance = ERROR_RESISTANCE;
		di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_ADAPTOR_DETECT;
		quick_charge_flag = 0;
		super_charge_flag = 0;
		direct_charger_disconnect_send_uevent();
		di->dc_err_report_flag = FALSE;
		memset(dc_err_dsm_buff, 0, sizeof(dc_err_dsm_buff));
	}
	else
	{
		hwlog_info("%s:switch to normal charge!\n", __func__);
	}
	ret = di->ls_ops->ls_exit();
	if (ret)
	{
		hwlog_err("[%s]: ls exit fail!\n", __func__);
	}
	ret = di->bi_ops->exit();
	if (ret)
	{
		hwlog_err("[%s]: bi exit fail!\n", __func__);
	}
	ret = di->scp_ops->scp_chip_reset();
	if (ret)
	{
		hwlog_err("[%s]: scp_chip_reset fail!\n", __func__);
	}
	hrtimer_cancel(&di->threshold_caculation_timer);
	cancel_work_sync(&di->threshold_caculation_work);

	hrtimer_cancel(&di->kick_watchdog_timer);
	cancel_work_sync(&di->kick_watchdog_work);

	di->scp_stop_charging_flag_error = 0;
	di->scp_stop_charging_flag_info = 0;
	di->cur_stage = 0;
	di->pre_stage = 0;
	di->vbat = 0;
	di->ibat = 0;
	di->vadapt = 0;
	di->iadapt = 0;
	di->ls_vbus = 0;
	di->ls_ibus = 0;
	di->compensate_v = 0;
	di->ibat_abnormal_cnt = 0;
	di->max_adaptor_cur = 0;
	direct_charge_wake_unlock();
	scp_stop_charging_complete_flag = 1;
}

/*lint -save -e* */
static void direct_charge_parse_volt_para(struct device_node* np, struct direct_charge_device* di)
{
	int ret, i, array_len, idata;
	const char *volt_para_string = NULL;

	array_len = of_property_count_strings(np, "volt_para");
	di->stage_size = array_len / DC_PARA_TOTAL;
	hwlog_info("stage_size = %d\n", di->stage_size);
	if ((array_len <= 0) || (array_len % DC_PARA_TOTAL != 0)) {
		hwlog_err("volt_para is invaild,please check volt_para number!!\n");
		return;
	}
	if (array_len > DC_VOLT_LEVEL * DC_PARA_TOTAL) {
		array_len = DC_VOLT_LEVEL * DC_PARA_TOTAL;
		hwlog_err("volt_para is too long,use only front %d paras!!\n", array_len);
		return;
	}
	for (i = 0; i < array_len; i++) {
		ret = of_property_read_string_index(np, "volt_para", i, &volt_para_string);
		if (ret) {
			hwlog_err("get volt_para failed\n");
			return;
		}

		idata = simple_strtol(volt_para_string, NULL, STRTOL_MAX_LEN);
		switch (i % DC_PARA_TOTAL) {
		case DC_PARA_VOL_TH:
			if (idata < 0) {
				hwlog_err ("the volt_para vol_th is out of range!!\n");
				return;
			}
#ifdef CONFIG_SCHARGER_V300
			if (TRUE == is_hi6523_cv_limit())
				idata -= HI6523_CV_CUT;
#endif
			orig_volt_para[i / (DC_PARA_TOTAL)].vol_th = idata;
			break;
		case DC_PARA_CUR_TH_HIGH:
			if (idata < 0) {
				hwlog_err ("the volt_para cur_th_high is out of range!!\n");
				return;
			}
			orig_volt_para[i / (DC_PARA_TOTAL)].cur_th_high = idata;
			break;
		case DC_PARA_CUR_TH_LOW:
			if (idata < 0) {
				hwlog_err ("the volt_para cur_th_low is out of range!!\n");
				return;
			}
			orig_volt_para[i / (DC_PARA_TOTAL)].cur_th_low = idata;
			break;
		default:
			hwlog_err("get volt_para failed\n");
			return;
		}
	}

	for (i = 0; i < di->stage_size; i++) {
		di->volt_para[i].vol_th = orig_volt_para[i].vol_th;
		di->volt_para[i].cur_th_high = orig_volt_para[i].cur_th_high;
		di->volt_para[i].cur_th_low = orig_volt_para[i].cur_th_low;
		hwlog_info("orig_volt_para[%d], vol_th:%d, cur_th_high:%d, cur_th_low:%d\n", \
				i, di->volt_para[i].vol_th, di->volt_para[i].cur_th_high, di->volt_para[i].cur_th_low);
	}
}

static int direct_charge_parse_dts(struct device_node* np, struct direct_charge_device* di)
{
	int ret = 0;
	int i = 0;
	int array_len = 0;
	int idata = 0;
	const char *chrg_data_string = NULL;


	ret = of_property_read_u32(np, "scp_work_on_charger", &(di->scp_work_on_charger));
	if (ret)
	{
		hwlog_err("scp_work_on_charger failed\n");
		return -EINVAL;
	}
	hwlog_info("scp_work_on_charger = %d\n", di->scp_work_on_charger);
	ret = of_property_read_u32(np, "use_gpio_for_ovp_en", &(di->use_gpio_for_ovp_en));
	if (ret)
	{
		hwlog_err("use_gpio_for_ovp_en failed\n");
		return -EINVAL;
	}
	hwlog_info("use_gpio_for_ovp_en = %d\n", di->use_gpio_for_ovp_en);
	if (di->use_gpio_for_ovp_en) {
		di->ovp_en = of_get_named_gpio(np, "ovp_en", 0);
		hwlog_info("ovp_en = %d\n", di->ovp_en);
		if (!gpio_is_valid(di->ovp_en)) {
			hwlog_err("ovp_en is not valid\n");
			return -EINVAL;
		}
	}
	ret = of_property_read_u32(np, "standard_cable_full_path_res_max", &(di->standard_cable_full_path_res_max));
	if (ret)
	{
		hwlog_err("get standard_cable_full_path_res_max failed\n");
		return -EINVAL;
	}
	hwlog_info("standard_cable_full_path_res_max = %d\n", di->standard_cable_full_path_res_max);
	ret = of_property_read_u32(np, "max_current_for_none_standard_cable", &(di->max_current_for_none_standard_cable));
	if (ret)
	{
		hwlog_err("get max_current_for_none_standard_cable failed\n");
		return -EINVAL;
	}
	hwlog_info("max_current_for_none_standard_cable = %d\n", di->max_current_for_none_standard_cable);
	ret = of_property_read_u32(np, "use_5A", &(di->use_5A));
	if (ret)
	{
		hwlog_err("get use_5A failed\n");
		return -EINVAL;
	}
	hwlog_info("use_5A = %d\n", di->use_5A);
	ret = of_property_read_u32(np, "max_tadapt", &(di->max_tadapt));
	if (ret)
	{
		hwlog_err("get max_tadapt failed\n");
		return -EINVAL;
	}
	hwlog_info("max_tadapt = %d\n", di->max_tadapt);
	ret = of_property_read_u32(np, "max_tls", &(di->max_tls));
	if (ret)
	{
		hwlog_err("get max_tls failed\n");
		return -EINVAL;
	}
	hwlog_info("max_tls = %d\n", di->max_tls);
	ret = of_property_read_u32(np, "scp_cable_detect_enable", &(di->scp_cable_detect_enable));
	if (ret)
	{
		hwlog_err("get scp_cable_detect_enable failed\n");
		return -EINVAL;
	}
	hwlog_info("scp_cable_detect_enable = %d\n", di->scp_cable_detect_enable);
	ret = of_property_read_u32(np, "ibat_abnormal_th", &(di->ibat_abnormal_th));
	if (ret)
	{
		hwlog_err("get ibat_abnormal_th failed\n");
		return -EINVAL;
	}
	hwlog_info("ibat_abnormal_th = %d\n", di->ibat_abnormal_th);
	ret = of_property_read_u32(np, "first_cc_stage_timer_in_min", &(di->first_cc_stage_timer_in_min));
	if (ret)
	{
		hwlog_err("get first_cc_stage_timer_in_min failed\n");
		return -EINVAL;
	}
	hwlog_info("first_cc_stage_timer_in_min = %d\n", di->first_cc_stage_timer_in_min);
	ret = of_property_read_u32(np, "vol_err_th", &(di->vol_err_th));
	if (ret)
	{
		hwlog_err("get vol_err_th failed\n");
		return -EINVAL;
	}
	hwlog_info("vol_err_th = %d\n", di->vol_err_th);
	ret = of_property_read_u32(np, "full_path_res_max", &(di->full_path_res_max));
	if (ret)
	{
		hwlog_err("get full_path_res_max failed\n");
		return -EINVAL;
	}
	hwlog_info("full_path_res_max = %d\n", di->full_path_res_max);
	ret = of_property_read_u32(np, "adaptor_leakage_current_th", &(di->adaptor_leakage_current_th));
	if (ret)
	{
		hwlog_err("get adaptor_leakage_current_th failed\n");
		return -EINVAL;
	}
	hwlog_info("adaptor_leakage_current_th = %d\n", di->adaptor_leakage_current_th);
	ret = of_property_read_u32(np, "compensate_r", &(di->compensate_r));
	if (ret)
	{
		hwlog_err("get compensate_r failed\n");
		return -EINVAL;
	}
	hwlog_info("compensate_r = %d\n", di->compensate_r);
	ret = of_property_read_u32(np, "max_dc_bat_vol", &(di->max_dc_bat_vol));
	if (ret)
	{
		hwlog_err("get max_dc_bat_vol failed\n");
		return -EINVAL;
	}
	#ifdef CONFIG_SCHARGER_V300
	if (TRUE == is_hi6523_cv_limit())
		di->max_dc_bat_vol -= HI6523_CV_CUT;
	#endif
	hwlog_info("max_dc_bat_vol = %d\n", di->max_dc_bat_vol);
	ret = of_property_read_u32(np, "min_dc_bat_vol", &(di->min_dc_bat_vol));
	if (ret)
	{
		hwlog_err("get min_dc_bat_vol failed\n");
		return -EINVAL;
	}
	hwlog_info("min_dc_bat_vol = %d\n", di->min_dc_bat_vol);
	ret = of_property_read_u32(np, "max_adaptor_vset", &(di->max_adaptor_vset));
	if (ret)
	{
		hwlog_err("get max_adaptor_vset failed\n");
		return -EINVAL;
	}
	hwlog_info("max_adaptor_vset = %d\n", di->max_adaptor_vset);
	ret = of_property_read_u32(np, "charge_control_interval", &(di->charge_control_interval));
	if (ret)
	{
		hwlog_err("get charge_control_interval failed\n");
		return -EINVAL;
	}
	hwlog_info("charge_control_interval = %d\n", di->charge_control_interval);
	ret = of_property_read_u32(np, "threshold_caculation_interval", &(di->threshold_caculation_interval));
	if (ret)
	{
		hwlog_err("get threshold_caculation_interval failed\n");
		return -EINVAL;
	}
	hwlog_info("threshold_caculation_interval = %d\n", di->threshold_caculation_interval);
	ret = of_property_read_u32(np, "vstep", &(di->vstep));
	if (ret)
	{
		hwlog_err("get vstep failed\n");
		return -EINVAL;
	}
	hwlog_info("vstep = %d\n", di->vstep);
	ret = of_property_read_u32(np, "delta_err", &(di->delta_err));
	if (ret)
	{
		hwlog_err("get delta_err failed\n");
		return -EINVAL;
	}
	hwlog_info("delta_err = %d\n", di->delta_err);
	ret = of_property_read_u32(np, "cc_cable_detect_enable", &(di->cc_cable_detect_enable));
	if (ret)
	{
		hwlog_err("get cc_cable_detect_enable failed\n");
	}
	hwlog_info("cc_cable_detect_enable = %d\n", di->cc_cable_detect_enable);
	direct_charge_parse_volt_para(np, di);
	array_len = of_property_count_strings(np, "temp_para");
	if ((array_len <= 0) || (array_len % DC_TEMP_TOTAL != 0))
	{
		hwlog_err("temp_para is invaild,please check temp_para number!!\n");
		return -EINVAL;
	}
	if (array_len > DC_TEMP_LEVEL * DC_TEMP_TOTAL)
	{
		array_len = DC_TEMP_LEVEL * DC_TEMP_TOTAL;
		hwlog_err("temp_para is too long,use only front %d paras!!\n", array_len);
		return -EINVAL;
	}
	for (i = 0; i < array_len; i++)
	{
		ret = of_property_read_string_index(np, "temp_para", i, &chrg_data_string);
		if (ret)
		{
			hwlog_err("get temp_para failed\n");
			return -EINVAL;
		}
		idata = simple_strtol(chrg_data_string, NULL, 10);
		switch (i % DC_TEMP_TOTAL) {
		case DC_TEMP_MIN:
			di->temp_para[i / (DC_TEMP_TOTAL)].temp_min = idata;
			break;
		case DC_TEMP_MAX:
			di->temp_para[i / (DC_TEMP_TOTAL)].temp_max = idata;
			break;
		case DC_CUR_MAX:
			di->temp_para[i / (DC_TEMP_TOTAL)].cur_max = idata;
			break;
		default:
			hwlog_err("get temp_para failed\n");
			return -EINVAL;
		}
		hwlog_info("di->temp_para[%d][%d] = %d\n", i / (DC_TEMP_TOTAL), i % (DC_TEMP_TOTAL), idata);
	}
	for (i = 0; i < 2*DC_VOLT_LEVEL; ++i)
	{
		di->stage_need_to_jump[i] = -1;
	}
	array_len = of_property_count_strings(np, "stage_need_to_jump");
	if ((array_len <= 0) || (array_len > 2*di->stage_size))
	{
		hwlog_err("invalid stage need to jump!!\n");
		return -EINVAL;
	}
	for (i = 0; i < array_len; i++)
	{
		ret = of_property_read_string_index(np, "stage_need_to_jump", i,&chrg_data_string);
		if (ret)
		{
			hwlog_err("get stage_need_to_jump error\n");
			return -EINVAL;
		}
		idata = simple_strtol(chrg_data_string, NULL, 10);
		if (idata < -1 || idata > 2*di->stage_size)
		{
			hwlog_err("stage_need_to_jump:[%d] is out of range!!\n", idata);
			return -EINVAL;
		}
		hwlog_info("stage_need_to_jump[%d] = %d\n", i, idata);
		di->stage_need_to_jump[i] = idata;
	}
	return ret;
}
/*lint -restore*/

static int jump_stage_if_need(int cur_stage)
{
	int i;
	struct direct_charge_device *di = g_di;

	for (i = 0; i < 2*di->stage_size; ++i)
	{
		if (-1 == di->stage_need_to_jump[i])
		{
			return cur_stage;
		}
		else
		{
			if (cur_stage == di->stage_need_to_jump[i])
			{
				hwlog_info("jump stage %d\n", cur_stage);
				return jump_stage_if_need(cur_stage + 1);
			}
		}
	}
	return cur_stage;
}

/*lint -save -e* */
static void select_direct_charge_stage(struct direct_charge_device* di)
{
	int i;
	int vbat_th;
	int cur_stage = 0;
	int stage_size = di->stage_size;
	int vbat = get_bat_voltage(di);
	int ibat = get_bat_current(di);
	int iadaptor = get_adaptor_current(di);
	char tmp_buf[ERR_NO_STRING_SIZE] = { 0 };

	di->vbat = vbat;
	di->ibat = ibat;
	if (iadaptor < di->ibat_abnormal_th)
	{
		di->ibat_abnormal_cnt++;
	}
	else
	{
		di->ibat_abnormal_cnt = 0;
	}
	if (di->ibat_abnormal_cnt > 10)
	{
		di->scp_stop_charging_flag_error = 1;
		snprintf(tmp_buf, sizeof(tmp_buf),"%s: ibat abnormal, stop direct charge\n", __func__);
		hwlog_err("%s", tmp_buf);
		strncat(dc_err_dsm_buff, tmp_buf, strlen(tmp_buf));
		return;
	}
	di->pre_stage = di->cur_stage;
	for (i = stage_size - 1; i >=0; --i)
	{
		vbat_th = di->volt_para[i].vol_th + di->compensate_v;
		vbat_th = vbat_th > di->volt_para[stage_size - 1].vol_th ? di->volt_para[stage_size - 1].vol_th : vbat_th;
		if (vbat >= vbat_th && ibat <= di->volt_para[i].cur_th_low)
		{
			cur_stage = 2*i +2;
			break;
		}
		else if (vbat >= vbat_th)
		{
			cur_stage = 2*i +1;
			break;
		}
	}
	if (i < 0)
	{
		cur_stage = 0;
	}
	if (cur_stage < di->pre_stage)
	{
		cur_stage = di->pre_stage;
	}
	if (di->first_cc_stage_timer_in_min)
	{
		if (0 == cur_stage)
		{
			if (time_after(jiffies, di->first_cc_stage_timeout))
			{
				hwlog_info("first_cc_stage_timeout in %d min, stage++\n",di->first_cc_stage_timer_in_min);
				cur_stage += 1;
			}
		}
	}
	if (cur_stage != di->cur_stage)
	{
		di->cur_stage = jump_stage_if_need(cur_stage);
	}
}
/*lint -restore*/

static int battery_temp_handler(int temp)
{
	int i;
	struct direct_charge_device *di = g_di;

	for (i = 0; i < DC_TEMP_LEVEL; ++i)
	{
		if  (temp >= di->temp_para[i].temp_min && temp < di->temp_para[i].temp_max)
		{
			return di->temp_para[i].cur_max;
		}
	}
	hwlog_err("error temp = %d\n",temp);
	return 0;
}
static void select_direct_charge_param(struct direct_charge_device* di)
{
	int cur_th_high;
	int max_adaptor_cur;
	int vbat_th;
	int bat_temp_cur_max;
	int bat_temp = hisi_battery_temperature();

	bat_temp_cur_max = battery_temp_handler(bat_temp);
	if (0 == bat_temp_cur_max)
	{
		hwlog_info("%s : temp = %d, can not do direct charging \n", __func__, bat_temp);
		di->scp_stop_charging_flag_info = 1;
	}
	max_adaptor_cur = get_adaptor_max_current(di);
	if (max_adaptor_cur < 0)
		return;
	if(di->use_5A)
	{
		if (max_adaptor_cur == 4500)
		{
			max_adaptor_cur = di->volt_para[0].cur_th_high;
		}
	}
	if (max_adaptor_cur != di->max_adaptor_cur)
	{
		hwlog_info("%s : max_adaptor_cur = %d\n", __func__, max_adaptor_cur);
		di->max_adaptor_cur = max_adaptor_cur;
	}
	di->compensate_v = di->ibat*di->compensate_r/1000;
	vbat_th = di->volt_para[di->cur_stage/2].vol_th + di->compensate_v;
	di->cur_vbat_th = vbat_th < di->volt_para[di->stage_size - 1].vol_th ? vbat_th: di->volt_para[di->stage_size - 1].vol_th;

	cur_th_high = di->volt_para[di->cur_stage/2].cur_th_high;
	if (di->cc_cable_detect_enable)
	{
		if (0 == di->cc_cable_detect_ok)
		{
			if((di->full_path_resistance >= (-1) * di->full_path_res_threshold) && (di->full_path_resistance <= di->full_path_res_threshold))
			{
				hwlog_info("%s:do_full_path_resistance_check succ!\n",__func__);
				cur_th_high = di->volt_para[di->cur_stage/2].cur_th_high > di->max_current_for_none_standard_cable ? di->max_current_for_none_standard_cable : di->volt_para[di->cur_stage/2].cur_th_high;
			}
		}else
		{
			cur_th_high = di->volt_para[di->cur_stage/2].cur_th_high;
		}
		hwlog_info("[%s]:cc_check_result = %d ,after_cc_check_cur_th_high = %d\n", __func__,di->cc_cable_detect_ok,cur_th_high);
	}
	cur_th_high = cur_th_high >  bat_temp_cur_max ? bat_temp_cur_max : cur_th_high;
	cur_th_high = cur_th_high >  max_adaptor_cur ? max_adaptor_cur : cur_th_high;
	di->cur_ibat_th_high = cur_th_high > di->sysfs_iin_thermal ? di->sysfs_iin_thermal: cur_th_high;
	di->cur_ibat_th_low = di->volt_para[di->cur_stage/2].cur_th_low;
}
static void battery_aging_safe_policy(struct direct_charge_device* di)
{
	int ret, i, cur_level;
	static int last_level = BASP_PARA_LEVEL;
	AGING_SAFE_POLICY_TYPE basp = {0};

	ret = hisi_battery_aging_safe_policy(&basp);
	if (ret) {
		hwlog_err(BASP_TAG"[%s] get basp policy fail, ret:%d!\n", __func__, ret);
		return;
	}

	cur_level = basp.level;

	if (cur_level != last_level) {
		di->volt_para[di->stage_size -1].vol_th =
			orig_volt_para[di->stage_size -1].vol_th - basp.dc_volt_dec;
		for (i = 0; i < di->stage_size -1; i++) {
			di->volt_para[i].vol_th = orig_volt_para[i].vol_th < di->volt_para[di->stage_size -1].vol_th\
								? orig_volt_para[i].vol_th : di->volt_para[di->stage_size -1].vol_th;
		}
		switch (basp.cur_ratio_policy) {
		case BASP_RATIO_POLICY_ALL:
			di->volt_para[di->stage_size -1].cur_th_high =
						orig_volt_para[di->stage_size -1].cur_th_high * basp.cur_ratio/BASP_PARA_SCALE;
			di->volt_para[di->stage_size -1].cur_th_low =
						orig_volt_para[di->stage_size -1].cur_th_low;
			for (i = 0; i < di->stage_size -1; i++) {
				di->volt_para[i].cur_th_high = orig_volt_para[i].cur_th_high * basp.cur_ratio /BASP_PARA_SCALE;
				di->volt_para[i].cur_th_low = orig_volt_para[i].cur_th_low * basp.cur_ratio /BASP_PARA_SCALE;
			}
			break;
		case BASP_RATIO_POLICY_MAX:
			di->volt_para[0].cur_th_high = orig_volt_para[0].cur_th_high * basp.cur_ratio /BASP_PARA_SCALE;
			di->volt_para[0].cur_th_low = orig_volt_para[0].cur_th_low;
			for (i = 1; i < di->stage_size;i++) {
				di->volt_para[i].cur_th_high = orig_volt_para[i].cur_th_high <= di->volt_para[0].cur_th_high ?
							orig_volt_para[i].cur_th_high : di->volt_para[0].cur_th_high;
				di->volt_para[i].cur_th_low = orig_volt_para[i].cur_th_low;
			}
			break;
		default:
			break;
		}
		last_level = cur_level;
		hwlog_info(BASP_TAG"cur_level = %d\n", cur_level);
		for (i = 0; i < di->stage_size; i++) {
			hwlog_info(BASP_TAG"volt_para[%d], vol_th:%d, cur_th_high:%d, cur_th_low:%d\n",
				i, di->volt_para[i].vol_th, di->volt_para[i].cur_th_high, di->volt_para[i].cur_th_low);
		}
	}

	return;
}
static void direct_charge_regulation(struct direct_charge_device* di)
{
	char buf[ERR_NO_STRING_SIZE] = { 0 };
	int ret;
	int vbat = di->vbat;
	int ibat = di->ibat;
	int vbat_sh = di->cur_vbat_th;
	int ibat_sh_high = di->cur_ibat_th_high;
	int ibat_sh_low = di->cur_ibat_th_low;
	int iadapt = get_adaptor_current(di);
	int iadapt_set = get_adaptor_current_set(di);

	di->vadapt = get_adaptor_voltage(di);
	di->tadapt = get_adaptor_temp(di);
	di->iadapt = iadapt;
	di->ls_ibus = get_ls_ibus(di);
	di->ls_vbus = get_ls_vbus();
	di->tls = get_ls_temp(di);

	hwlog_info("cur_stage = %d vbat = %d ibat = %d vbat_sh = %d ibat_sh_high = %d ibat_sh_low = %d vadp = %d iadap = %d ls_vbus = %d ls_ibus = %d iadapt_set = %d tadapt = %d tls = %d!\n",
			di->cur_stage, vbat, ibat, vbat_sh, ibat_sh_high, ibat_sh_low, di->vadapt, iadapt, di->ls_vbus, di->ls_ibus, iadapt_set, di->tadapt, di->tls);
	if (di->tls > (int)(di->max_tls))
	{
		snprintf(buf, sizeof(buf), "%s: tls = %d > %d, stop direct_charge!\n", __func__, di->tls, di->max_tls);
		hwlog_err("%s", buf);
		strncat(dc_err_dsm_buff, buf, strlen(buf));
		di->scp_stop_charging_flag_error = 1;
		return;
	}
	if (di->tadapt > (int)(di->max_tadapt))
	{
		snprintf(buf, sizeof(buf), "%s: tadapt(%d) > [%d], stop direct_charge!\n", __func__, di->tadapt, di->max_tadapt);
		hwlog_err("%s", buf);
		strncat(dc_err_dsm_buff, buf, strlen(buf));
		dsm_report(DSM_DIRECT_CHARGE_ADAPTER_OTP, buf);
		di->scp_stop_charging_flag_error = 1;
		return;
	}
	ret = is_ls_close(di);/*keep communication with loadswitchwithin 1 second*/
	if (ret)
	{
		hwlog_err("ls is close!\n");
		return;
	}
	if (di->cur_stage % 2)
	{
		if (vbat > vbat_sh)
		{
			di->adaptor_vset += vbat_sh - vbat;
			set_adaptor_voltage(di);
			return;
		}
		if (iadapt > ibat_sh_high)
		{
			di->adaptor_vset -= di->vstep;
			set_adaptor_voltage(di);
			return;
		}
		if (ibat > ibat_sh_high)
		{
			di->adaptor_vset -= di->vstep;
			set_adaptor_voltage(di);
			return;
		}
		if (ibat > ibat_sh_high - di->delta_err)
		{
			hwlog_info("do nothing!\n");
			return;
		}
		if (di->adaptor_iset < ibat_sh_high - 1000)
		{
			di->adaptor_iset += 1000;
			set_adaptor_current(di);
			return;
		}
		else if (di->adaptor_iset < ibat_sh_high)
		{
			di->adaptor_iset = ibat_sh_high;
			set_adaptor_current(di);
			return;
		}
		else
		{
			if (iadapt < ibat_sh_high - di->delta_err)
			{
				di->adaptor_vset += di->vstep;
				set_adaptor_voltage(di);
				return;
			}
		}
	}
	else
	{
		if (iadapt > ibat_sh_high)
		{
			di->adaptor_vset -= di->vstep;
			set_adaptor_voltage(di);
			return;
		}
		if (ibat > ibat_sh_high)
		{
			di->adaptor_vset -= di->vstep;
			set_adaptor_voltage(di);
			return;
		}
		if (ibat > ibat_sh_high - di->delta_err)
		{
			hwlog_info("do nothing!\n");
			return;
		}
		if (di->adaptor_iset < ibat_sh_high - 1000)
		{
			di->adaptor_iset += 1000;
			set_adaptor_current(di);
			return;
		}
		else if (di->adaptor_iset < ibat_sh_high)
		{
			di->adaptor_iset = ibat_sh_high;
			set_adaptor_current(di);
			return;
		}
		else
		{
			if (iadapt < ibat_sh_high - di->delta_err)
			{
				di->adaptor_vset += di->vstep;
				set_adaptor_voltage(di);
				return;
			}
		}

	}
}

/*lint -save -e* */
static void charge_control_work(struct work_struct *work)
{
	struct direct_charge_device *di = container_of(work,struct direct_charge_device, charge_control_work);
	int interval = di->charge_control_interval;

	if (di->scp_stop_charging_flag_error || di->scp_stop_charging_flag_info || (0 == di->sysfs_enable_charger) ||(0 == di->vbat_ovp_enable_charger))
	{
		hwlog_info("direct charge stop!\n");
		scp_stop_charging();
		return;
	}
	if (2*di->stage_size == di->cur_stage)
	{
		hwlog_info("cur_stage = %d vbat = %d ibat = %d\n", di->cur_stage, di->vbat, di->ibat);
		hwlog_info("direct charge done!\n");
		scp_stop_charging();
		return;
	}
	direct_charge_regulation(di);

	hrtimer_start(&di->charge_control_timer, ktime_set(interval/MSEC_PER_SEC, (interval % MSEC_PER_SEC) * USEC_PER_SEC), HRTIMER_MODE_REL);
}
/*lint -restore*/

/*lint -save -e* */
static void threshold_caculation_work(struct work_struct *work)
{
	struct direct_charge_device *di = container_of(work,struct direct_charge_device, threshold_caculation_work);
	int interval = di->threshold_caculation_interval;

	if (di->scp_stop_charging_flag_error || di->scp_stop_charging_flag_info || (0 == di->sysfs_enable_charger) ||(0 == di->vbat_ovp_enable_charger))
	{
		hwlog_info("direct charge stop, stop threshold_caculation!\n");
		return;
	}
	battery_aging_safe_policy(di);
	select_direct_charge_stage(di);
	select_direct_charge_param(di);
	if (2*di->stage_size == di->cur_stage)
	{
		hwlog_info("direct charge done, stop threshold_caculation!\n");
		return;
	}
	hrtimer_start(&di->threshold_caculation_timer, ktime_set(interval/MSEC_PER_SEC, (interval % MSEC_PER_SEC) * USEC_PER_SEC), HRTIMER_MODE_REL);
}
/*lint -restore*/

static void kick_watchdog_work(struct work_struct *work)
{
	struct direct_charge_device *di = container_of(work,struct direct_charge_device, kick_watchdog_work);
	int interval = KICK_WATCHDOG_TIME;//kich watchdog timer;
	int ret;
	int bat_curr;

	if (di->scp_stop_charging_flag_error || di->scp_stop_charging_flag_info || (0 == di->sysfs_enable_charger) ||(0 == di->vbat_ovp_enable_charger))
	{
		hwlog_info("direct charge stop, stop kick_watchdog!\n");
		return;
	}
	if (DOUBLE_SIZE * di->stage_size == di->cur_stage)
	{
		hwlog_info("direct charge done, stop kick_watchdog!\n");
		return;
	}

	ret = di->bi_ops->get_bat_current(&bat_curr);/*keep communication with the adaptor within 1 second*/
	if (ret)
	{
		hwlog_err("get_bat_current fail!\n");
	}
	hwlog_info("To keep communication with ls : bat_curr = %d!\n", bat_curr);
	hrtimer_start(&di->kick_watchdog_timer, ktime_set(interval/MSEC_PER_SEC, (interval % MSEC_PER_SEC) * USEC_PER_SEC), HRTIMER_MODE_REL);
}

/*lint -save -e* */
static enum hrtimer_restart threshold_caculation_timer_func(struct hrtimer *timer)
{
	struct direct_charge_device *di;

	di = container_of(timer, struct direct_charge_device, threshold_caculation_timer);
	queue_work(di->direct_charge_wq, &di->threshold_caculation_work);
	return HRTIMER_NORESTART;
}

static enum hrtimer_restart charge_control_timer_func(struct hrtimer *timer)
{
	struct direct_charge_device *di;

	di = container_of(timer, struct direct_charge_device, charge_control_timer);
	queue_work(di->direct_charge_wq, &di->charge_control_work);
	return HRTIMER_NORESTART;
}
/*lint -restore*/

static enum hrtimer_restart kick_watchdog_timer_func(struct hrtimer *timer)
{
	struct direct_charge_device *di;

	di = container_of(timer, struct direct_charge_device, kick_watchdog_timer);
	queue_work(di->direct_charge_watchdog_wq, &di->kick_watchdog_work);
	return HRTIMER_NORESTART;
}

static void scp_start_charging(struct direct_charge_device *di)
{
	int interval;

	hwlog_info("%s \n",__func__);
	direct_charge_wake_lock();
	if (di->first_cc_stage_timer_in_min)
	{
		/*8A maximum 5 min, ap will not suspend in direct charge mode, so use jiffies */
		hwlog_info("%s start timing\n",__func__);
		di->first_cc_stage_timeout = jiffies + msecs_to_jiffies(di->first_cc_stage_timer_in_min*60*MSEC_PER_SEC);
	}
	select_direct_charge_stage(di);
	chg_set_adaptor_test_result(TYPE_SCP,PROTOCOL_FINISH_SUCC);
	scp_set_stage_status(SCP_STAGE_CHARGING);
	select_direct_charge_param(di);
	interval = di->charge_control_interval;
	hrtimer_start(&di->charge_control_timer, ktime_set(interval/MSEC_PER_SEC, (interval % MSEC_PER_SEC) * USEC_PER_SEC), HRTIMER_MODE_REL);
	interval = di->threshold_caculation_interval;
	hrtimer_start(&di->threshold_caculation_timer, ktime_set(interval/MSEC_PER_SEC, (interval % MSEC_PER_SEC) * USEC_PER_SEC), HRTIMER_MODE_REL);
	interval = KICK_WATCHDOG_TIME;
	hrtimer_start(&di->kick_watchdog_timer, ktime_set(interval/MSEC_PER_SEC, (interval % MSEC_PER_SEC) * USEC_PER_SEC), HRTIMER_MODE_REL);
}
/*lint -save -e* */
void vbat_ovp_exit_direct_charge(int enable_charge)
{
	int i = 0;
	int ibus = 0;
	struct direct_charge_device *di = g_di;

	di->vbat_ovp_enable_charger = enable_charge;
	hwlog_info("%s: vbat_ovp_enable_charger = %d \n",__func__,di->vbat_ovp_enable_charger);
}

static int vbat_ovp_scp_exit(void)
{
	int ret;
	int val;
	struct direct_charge_device *di = g_di;
	int vbus_vol = 0;
	val = di->ls_ops->ls_enable(0);
	if (val)
	{
		hwlog_err("[%s]: ls enable fail!\n", __func__);
	}
	if (di->ls_ops->ls_discharge)
	{
		val = di->ls_ops->ls_discharge(1);
		if (val)
		{
			hwlog_err("[%s]: ls discharge fail!\n", __func__);
		}
		else
		{
			hwlog_info("[%s]: ls discharge succ!\n", __func__);
		}
	}
	msleep(200);
	val = di->ls_ops->ls_discharge(0);
	msleep(20);
	val = di->bi_ops->get_vbus_voltage(&vbus_vol);
	if (val)
	{
		hwlog_err("[%s]: get vbus vol fail!\n", __func__);
	}
	hwlog_info("%s: vbus_vol = %d!\n", __func__, vbus_vol);
	if (vbus_vol < 3000)
	{
		hwlog_info("%s:direct charger disconnect!\n", __func__);
		ret = 0;
	}
	else
	{
		hwlog_info("%s:comunication fail!\n", __func__);
		ret = -1;
	}
	val = di->scp_ops->scp_exit(di);
	if (val)
	{
		hwlog_err("[%s]: scp exit fail!\n", __func__);
	}
	if (di->scp_work_on_charger)
	{
		scp_power_control(di, DISABLE);
		charge_set_hiz_enable(SET_HIZ_ENABLE);
	}
	val = di->ls_ops->ls_exit();
	if (val)
	{
		hwlog_err("[%s]: ls exit fail!\n", __func__);
	}
	val = di->bi_ops->exit();
	if (val)
	{
		hwlog_err("[%s]: bi exit fail!\n", __func__);
	}
	return ret;
}
int vbat_ovp_scp_handle(void)
{
	struct direct_charge_device *di = g_di;
	int timeout;
	int ret;
	int adaptor_vol;
	int iadaptor;

	if (0 != is_direct_charge_ops_valid(di))
	{
		hwlog_err("%s:bad ops \n", __func__);
		return -1;
	}
	di->adaptor_vendor_id = di->scp_ops->scp_get_adapter_vendor_id();
	di->adaptor_vset = 4300;
	di->adaptor_iset = 2000;
	ret = scp_direct_charge_init(di);
	if (ret)
	{
		hwlog_err("%s:scp_direct_charge_init fail\n", __func__);
		ret = vbat_ovp_scp_exit();
		return ret;
	}
	ret = di->ls_ops->watchdog_config_ms(0);
	if (ret)
	{
		hwlog_err("%s:watchdog_config_ms fail\n", __func__);
		ret = vbat_ovp_scp_exit();
		return ret;
	}

	adaptor_vol = di->scp_ops->scp_get_adaptor_voltage();
	if (adaptor_vol < 0)
	{
		hwlog_err("%s:scp_get_adaptor_voltage fail\n", __func__);
		ret = vbat_ovp_scp_exit();
		return ret;
	}
	iadaptor= get_adaptor_current_for_vbat_ovp(di);
	if (iadaptor < 0)
	{
		hwlog_err("%s:scp_get_iadaptor fail\n", __func__);
		ret = vbat_ovp_scp_exit();
		return ret;
	}
	hwlog_info("%s:adaptor_vol1 = %d, adaptor_current1 = %d\n",__func__, adaptor_vol,iadaptor);
	ret = di->scp_ops->scp_set_adaptor_voltage(di->adaptor_vset);
	if (ret)
	{
		hwlog_err("%s:scp_set_adaptor_voltage fail\n", __func__);
		ret = vbat_ovp_scp_exit();
		return ret;
	}
	ret = di->scp_ops->scp_set_adaptor_current(di->adaptor_iset);
	if (ret)
	{
		hwlog_err("%s:scp_set_adaptor_current\n", __func__);
		ret = vbat_ovp_scp_exit();
		return ret;
	}
	ret = di->ls_ops->ls_enable(1);
	if (ret)
	{
		hwlog_err("%s:ls_enable fail\n", __func__);
		ret = vbat_ovp_scp_exit();
		return ret;
	}
	hwlog_info("%s:set_adaptor_vol = %d, set_adaptor_current = %d\n",__func__, di->adaptor_vset,di->adaptor_iset);
	while(1)
	{
		di->adaptor_vset = 4300;
		di->adaptor_iset = 2000;
		ret = di->scp_ops->scp_set_adaptor_voltage(di->adaptor_vset);
		if (ret)
		{
			hwlog_err("%s:scp_set_adaptor_voltage fail\n", __func__);
			ret = vbat_ovp_scp_exit();
			return ret;
		}
		ret = di->scp_ops->scp_set_adaptor_current(di->adaptor_iset);
		if (ret)
		{
			hwlog_err("%s:scp_set_adaptor_current fail\n", __func__);
			ret = vbat_ovp_scp_exit();
			return ret;
		}
		adaptor_vol = di->scp_ops->scp_get_adaptor_voltage();
		if (adaptor_vol < 0)
		{
			hwlog_err("%s:scp_get_adaptor_voltage fail\n", __func__);
			ret = vbat_ovp_scp_exit();
			return ret;
		}
		iadaptor= get_adaptor_current_for_vbat_ovp(di);
		if (iadaptor < 0)
		{
			hwlog_err("%s:scp_get_iadaptor fail\n", __func__);
			ret = vbat_ovp_scp_exit();
			return ret;
		}
		hwlog_info("%s:adaptor_vol = %d, adaptor_current = %d\n",__func__, adaptor_vol,iadaptor);
		msleep(500);
	}
}
int is_direct_charge_failed(void)
{
	struct direct_charge_device *di = g_di;

	if (NULL == di)
	{
		return 0;
	}
	if (di->error_cnt >= DC_ERR_CNT_MAX)
	{
		return 1;
	}
	return 0;
}

void direct_charge_check(void)
{
	struct direct_charge_device *di = g_di;
	enum charge_done_type charge_done_status = get_charge_done_type();

	if (INVALID == is_direct_charge_ops_valid(di))
	{
		hwlog_err("%s : NULL pointer \n", __func__);
		return;
	}
	if ((SCP_STAGE_CHARGE_DONE == scp_stage) || (CHARGE_DONE == charge_done_status))
	{
		hwlog_err("%s : scp_stage = %d,charge_done_status = %d\n", __func__, scp_stage, charge_done_status);
		return;
	}
	if (di->error_cnt >= DC_ERR_CNT_MAX)
	{
		hwlog_info("%s error exceed %d times, direct charge is disabled\n", __func__, DC_ERR_CNT_MAX);
		di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_CHARGE_DISABLED;
		direct_charge_send_normal_charge_uevent();
		if (FALSE == di->dc_err_report_flag && di->dc_open_retry_cnt <= DC_OPEN_RETRY_CNT_MAX) {
			hwlog_err("%s", dc_err_dsm_buff);
			dsm_report(DSM_DIRECT_CHARGE_ERR, dc_err_dsm_buff);
			memset(dc_err_dsm_buff, 0, sizeof(dc_err_dsm_buff));
			di->dc_err_report_flag = TRUE;
		}
		return;
	}
	if (0 == di->sysfs_enable_charger ||0 == di->vbat_ovp_enable_charger)
	{
		hwlog_info("%s direct_charge is disabled\n",__func__);
		di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_CHARGE_DISABLED;
		direct_charge_send_normal_charge_uevent();
		return;
	}
	if (SCP_STAGE_DEFAULT == scp_get_stage_status())
	{
		if (is_support_scp(di))
		{
			hwlog_err("%s : not support scp \n", __func__);
			return;
		}
	}
#ifdef  CONFIG_HUAWEI_USB_SHORT_CIRCUIT_PROTECT
	if (is_in_uscp_mode()) {
		scp_set_stage_status(SCP_STAGE_DEFAULT);
		hwlog_err("%s direct_charge is disabled by uscp\n",__func__);
		di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_CHARGE_DISABLED;
		return;
	}
#endif
	if (SCP_STAGE_ADAPTER_DETECT == scp_get_stage_status())
	{
		di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_ADAPTOR_DETECT;
		scp_adaptor_and_cable_detect(di);
	}
	if (!can_battery_temp_do_direct_charge(di))
	{
		di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_BAT_TEMP;
		scp_set_stage_status(SCP_STAGE_DEFAULT);
		hwlog_err("temp out of range, try next loop!\n");
		return;
	}
	if (!can_battery_vol_do_direct_charge(di))
	{
		di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_BAT_VOL;
		scp_set_stage_status(SCP_STAGE_DEFAULT);
		hwlog_err("volt out of range, try next loop!\n");
		return;
	}
	if(SCP_STAGE_SWITCH_DETECT == scp_get_stage_status())
	{
		scp_stop_charging_complete_flag = 0;
		di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_SWITCH;
		if (SUCC == cutoff_normal_charge(di))
		{
			scp_set_stage_status(SCP_STAGE_CHARGE_INIT);
		}
		else
		{
			hwlog_err("%s : switch to direct charge failed \n", __func__);
			di->scp_stop_charging_flag_error =1;
			scp_stop_charging();
		}
	}
	if (SCP_STAGE_CHARGE_INIT == scp_get_stage_status())
	{
		di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_INIT;
		if (SUCC == scp_direct_charge_init(di))
		{
			scp_set_stage_status(SCP_STAGE_SECURITY_CHECK);
		}
		else
		{
			hwlog_err("%s : direct_charge init failed \n", __func__);
			di->scp_stop_charging_flag_error =1;
			scp_stop_charging();
		}
	}
	if (SCP_STAGE_SECURITY_CHECK == scp_get_stage_status())
	{
		if (0 == scp_security_check(di))
		{
			scp_set_stage_status(SCP_STAGE_SUCCESS);
		}
		else
		{
			hwlog_err("%s : scp security check fail	\n", __func__);
			di->scp_stop_charging_flag_error =1;
			scp_stop_charging();
		}
	}
	if (SCP_STAGE_SUCCESS == scp_get_stage_status())
	{
		di->direct_charge_succ_flag = DIRECT_CHARGE_SUCC;
		scp_start_charging(di);
	}
	if (SCP_STAGE_CHARGING == scp_get_stage_status())
	{
		hwlog_info("%s : in direct charge process\n", __func__);
	}
	hwlog_info("[%s]direct charge stage  %s !!! \n", __func__, scp_check_stage[scp_get_stage_status()]);
}
/*lint -restore*/

/*lint -save -e* */
void direct_charge_update_cutoff_flag(void)
{
	struct direct_charge_device *di = g_di;
	if(NULL == di)
		return;
	hwlog_info("[%s]cutoff_normal_flag = %d ! \n", __func__, cutoff_normal_flag);
	if (cutoff_normal_flag)
	{
		cutoff_normal_flag = 0;
	}
	else
	{
		di->error_cnt = 0;
		quick_charge_flag = 0;
		super_charge_flag = 0;
	}
}
void direct_charge_stop_charging(void)
{
	struct direct_charge_device *di = g_di;
	if(NULL == di)
		return;

	scp_set_stage_status(SCP_STAGE_DEFAULT);
	di->full_path_resistance = ERROR_RESISTANCE;
	di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_ADAPTOR_DETECT;
	di->scp_stop_charging_flag_error = 0;
	di->scp_stop_charging_flag_info = 0;
	di->cur_stage = 0;
	di->pre_stage = 0;
	di->vbat = 0;
	di->ibat = 0;
	di->vadapt = 0;
	di->iadapt = 0;
	di->ls_vbus = 0;
	di->ls_ibus = 0;
	di->compensate_v = 0;
	if (!strstr(saved_command_line, "androidboot.swtype=factory")) {
		di->sysfs_enable_charger = 1;
	}
	di->ibat_abnormal_cnt = 0;
	di->max_adaptor_cur = 0;
	di->dc_open_retry_cnt = 0;
	di->dc_err_report_flag = FALSE;
	memset(dc_err_dsm_buff, 0, sizeof(dc_err_dsm_buff));
}
/*lint -restore*/

#ifdef CONFIG_SYSFS
#define DIRECT_CHARGE_SYSFS_FIELD(_name, n, m, store)                \
{                                                   \
    .attr = __ATTR(_name, m, direct_charge_sysfs_show, store),    \
    .name = DIRECT_CHARGE_SYSFS_##n,          \
}

#define DIRECT_CHARGE_SYSFS_FIELD_RW(_name, n)               \
	DIRECT_CHARGE_SYSFS_FIELD(_name, n, S_IWUSR | S_IRUGO, direct_charge_sysfs_store)

#define DIRECT_CHARGE_SYSFS_FIELD_RO(_name, n)               \
	DIRECT_CHARGE_SYSFS_FIELD(_name, n, S_IRUGO, NULL)

static ssize_t direct_charge_sysfs_show(struct device *dev,
				 struct device_attribute *attr, char *buf);
static ssize_t direct_charge_sysfs_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count);

struct direct_charge_sysfs_field_info {
	struct device_attribute attr;
	u8 name;
};

static struct direct_charge_sysfs_field_info direct_charge_sysfs_field_tbl[] = {
	DIRECT_CHARGE_SYSFS_FIELD_RW(enable_charger, ENABLE_CHARGER),
	DIRECT_CHARGE_SYSFS_FIELD_RW(iin_thermal, IIN_THERMAL),
	DIRECT_CHARGE_SYSFS_FIELD_RO(adaptor_detect, ADAPTOR_DETECT),
	DIRECT_CHARGE_SYSFS_FIELD_RO(loadswitch_id, LOADSWITCH_ID),
	DIRECT_CHARGE_SYSFS_FIELD_RO(loadswitch_name, LOADSWITCH_NAME),
	DIRECT_CHARGE_SYSFS_FIELD_RO(vbat, VBAT),
	DIRECT_CHARGE_SYSFS_FIELD_RO(ibat, IBAT),
	DIRECT_CHARGE_SYSFS_FIELD_RO(vadapt, VADAPT),
	DIRECT_CHARGE_SYSFS_FIELD_RO(iadapt, IADAPT),
	DIRECT_CHARGE_SYSFS_FIELD_RO(ls_vbus, LS_VBUS),
	DIRECT_CHARGE_SYSFS_FIELD_RO(ls_ibus, LS_IBUS),
	DIRECT_CHARGE_SYSFS_FIELD_RO(full_path_resistance, FULL_PATH_RESISTANCE),
	DIRECT_CHARGE_SYSFS_FIELD_RO(direct_charge_succ, DIRECT_CHARGE_SUCC),
};

static struct attribute *direct_charge_sysfs_attrs[ARRAY_SIZE(direct_charge_sysfs_field_tbl) + 1];

static const struct attribute_group direct_charge_sysfs_attr_group = {
	.attrs = direct_charge_sysfs_attrs,
};

/**********************************************************
*  Function:       direct_charge_sysfs_init_attrs
*  Description:    initialize direct_charge_sysfs_attrs[] for direct_charge attribute
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void direct_charge_sysfs_init_attrs(void)
{
	int i, limit = ARRAY_SIZE(direct_charge_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		direct_charge_sysfs_attrs[i] = &direct_charge_sysfs_field_tbl[i].attr.attr;
	}
	direct_charge_sysfs_attrs[limit] = NULL;	/* Has additional entry for this */
}

/**********************************************************
*  Function:       direct_charge_sysfs_field_lookup
*  Description:    get the current device_attribute from direct_charge_sysfs_field_tbl by attr's name
*  Parameters:   name:device attribute name
*  return value:  direct_charge_sysfs_field_tbl[]
**********************************************************/
static struct direct_charge_sysfs_field_info *direct_charge_sysfs_field_lookup(const char *name)
{
	int i, limit = ARRAY_SIZE(direct_charge_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		if (!strncmp
		    (name, direct_charge_sysfs_field_tbl[i].attr.attr.name,
		     strlen(name)))
			break;
	}
	if (i >= limit)
		return NULL;

	return &direct_charge_sysfs_field_tbl[i];
}
/**********************************************************
*  Function:       direct_charge_sysfs_show
*  Description:    show the value for all direct charge device's node
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*  return value:  0-sucess or others-fail
**********************************************************/
static ssize_t direct_charge_sysfs_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct direct_charge_sysfs_field_info *info = NULL;
	struct direct_charge_device *di = dev_get_drvdata(dev);
	enum usb_charger_type type = charge_get_charger_type();
	int ret;

	info = direct_charge_sysfs_field_lookup(attr->attr.name);
	if (!info)
		return -EINVAL;

	switch (info->name) {
	case DIRECT_CHARGE_SYSFS_ENABLE_CHARGER:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->sysfs_enable_charger);
	case DIRECT_CHARGE_SYSFS_IIN_THERMAL:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->sysfs_iin_thermal);
	case DIRECT_CHARGE_SYSFS_ADAPTOR_DETECT:
		ret = SCP_ADAPTOR_DETECT_FAIL;
		if (INVALID == is_direct_charge_ops_valid(di))
		{
			hwlog_err("(%s)invalid ops\n", __func__);
			return snprintf(buf, PAGE_SIZE, "%d\n", ret);
		}
		if(di->scp_ops->is_support_scp())
		{
			hwlog_err("(%s)not support scp\n", __func__);
			return snprintf(buf, PAGE_SIZE, "%d\n", ret);
		}
		if ((CHARGER_TYPE_STANDARD == type) ||
			((CHARGER_REMOVED == type) && get_direct_charge_flag()))
		{
			if (SCP_ADAPTOR_DETECT == scp_adaptor_detect_flag)
			{
				ret = 0;
			}else{
				ret = ADAPTOR_SCP_DETECT_FAIL;
			}
			hwlog_info("(%s)adaptor_detect = %d\n", __func__, ret);
		}
		return snprintf(buf, PAGE_SIZE, "%d\n", ret);
	case DIRECT_CHARGE_SYSFS_LOADSWITCH_ID:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->ls_id);
	case DIRECT_CHARGE_SYSFS_LOADSWITCH_NAME:
		return snprintf(buf, PAGE_SIZE, "%s\n", NULL == di->ls_name ? "ERROR" : di->ls_name);
	case DIRECT_CHARGE_SYSFS_VBAT:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->vbat);
	case DIRECT_CHARGE_SYSFS_IBAT:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->ibat);
	case DIRECT_CHARGE_SYSFS_VADAPT:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->vadapt);
	case DIRECT_CHARGE_SYSFS_IADAPT:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->iadapt);
	case DIRECT_CHARGE_SYSFS_LS_VBUS:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->ls_vbus);
	case DIRECT_CHARGE_SYSFS_LS_IBUS:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->ls_ibus);
	case DIRECT_CHARGE_SYSFS_FULL_PATH_RESISTANCE:
		return snprintf(buf, PAGE_SIZE, "%d\n", di->full_path_resistance);
	case DIRECT_CHARGE_SYSFS_DIRECT_CHARGE_SUCC:
		hwlog_info("(%s)direct_charge_succ_flag = %d\n", __func__, di->direct_charge_succ_flag);
		return snprintf(buf, PAGE_SIZE, "%d\n", di->direct_charge_succ_flag);
	default:
		hwlog_err("(%s)NODE ERR!!HAVE NO THIS NODE:(%d)\n", __func__, info->name);
		break;
	}
	return 0;
}
/**********************************************************
*  Function:       direct_charge_sysfs_store
*  Description:    set the value for charge_data's node which is can be written
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*                      count:unused
*  return value:  0-sucess or others-fail
**********************************************************/
static ssize_t direct_charge_sysfs_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct direct_charge_sysfs_field_info *info = NULL;
	struct direct_charge_device *di = dev_get_drvdata(dev);
	long val = 0;

	info = direct_charge_sysfs_field_lookup(attr->attr.name);
	if (!info)
		return -EINVAL;

	switch (info->name) {
	case DIRECT_CHARGE_SYSFS_ENABLE_CHARGER:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
			return -EINVAL;
		di->sysfs_enable_charger = val;
		hwlog_info("set enable_charger = %d\n", di->sysfs_enable_charger);
		break;
	case DIRECT_CHARGE_SYSFS_IIN_THERMAL:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 8000))
			return -EINVAL;
		hwlog_info("set iin_thermal = %ld\n", val);
		if (0 == val)
		{
			di->sysfs_iin_thermal = di->volt_para[0].cur_th_high;
		}
		else if (val < di->volt_para[di->stage_size - 1].cur_th_low)
		{
			hwlog_info("iin_thermal = %ld < %d, ignored\n", val, di->volt_para[di->stage_size - 1].cur_th_low);
			return -EINVAL;
		}
		else
		{
			di->sysfs_iin_thermal = val;
		}
		break;
	default:
		hwlog_err("(%s)NODE ERR!!HAVE NO THIS NODE:(%d)\n", __func__, info->name);
		break;
	}
	return count;
}

/**********************************************************
*  Function:       direct_charge_sysfs_create_group
*  Description:    create the direct charge device sysfs group
*  Parameters:   di:direct_charge_device
*  return value:  0-sucess or others-fail
**********************************************************/
static int direct_charge_sysfs_create_group(struct direct_charge_device *di)
{
	direct_charge_sysfs_init_attrs();
	return sysfs_create_group(&di->dev->kobj, &direct_charge_sysfs_attr_group);
}

/**********************************************************
*  Function:       direct_charge_sysfs_remove_group
*  Description:    remove the direct_charge device sysfs group
*  Parameters:   di:direct_charge_device
*  return value:  NULL
**********************************************************/

/*lint -save -e* */
static inline void direct_charge_sysfs_remove_group(struct direct_charge_device *di)
{
	sysfs_remove_group(&di->dev->kobj, &direct_charge_sysfs_attr_group);
}
#else
static int direct_charge_sysfs_create_group(struct direct_charge_device *di)
{
	return 0;
}

static inline void direct_charge_sysfs_remove_group(struct direct_charge_device *di)
{
}
#endif
/*lint -restore*/

/*lint -save -e* */
static void direct_charge_fault_work(struct work_struct *work)
{
	char buf[512] = { 0 };
	char reg_info[512] = { 0 };
	int bat_capacity = 0;
	struct direct_charge_device *di = container_of(work, struct direct_charge_device, fault_work);
	struct nty_data* data = di->fault_data;

	di->scp_stop_charging_flag_error = 1;
	snprintf(reg_info, sizeof(reg_info), "charge_fault = %d, addr = 0x%x, event1 = 0x%x, event2 = 0x%x\n",
		di->charge_fault, data->addr, data->event1, data->event2);
	strncat(dc_err_dsm_buff, reg_info, strlen(reg_info));
	switch (di->charge_fault) {
	case DIRECT_CHARGE_FAULT_VBUS_OVP:
		hwlog_err("vbus ovp happened!\n");
		snprintf(buf, sizeof(buf), "vbus ovp happened\n");
		strncat(buf, reg_info, strlen(reg_info));
		dsm_report(DSM_DIRECT_CHARGE_VBUS_OVP, buf);
		break;
	case DIRECT_CHARGE_FAULT_REVERSE_OCP:
		bat_capacity = hisi_battery_capacity();
		hwlog_err("reverse ocp happened! battery capacity is %d \n", bat_capacity);
		snprintf(buf, sizeof(buf), "reverse ocp happened\n");
		strncat(buf, reg_info, strlen(reg_info));
		if(bat_capacity < BATTERY_CAPACITY_HIGH_TH){
			dsm_report(DSM_DIRECT_CHARGE_REVERSE_OCP, buf);
		}else{
			hwlog_err("battery capacity is over threshold, dsm not report!\n");
		}
		break;
	case DIRECT_CHARGE_FAULT_OTP:
		hwlog_err("otp happened!\n");
		snprintf(buf, sizeof(buf), "otp happened\n");
		strncat(buf, reg_info, strlen(reg_info));
		dsm_report(DSM_DIRECT_CHARGE_OTP, buf);
		break;
	case DIRECT_CHARGE_FAULT_INPUT_OCP:
		hwlog_err("input ocp happened!\n");
		snprintf(buf, sizeof(buf), "input ocp happened\n");
		strncat(buf, reg_info, strlen(reg_info));
		dsm_report(DSM_DIRECT_CHARGE_INPUT_OCP, buf);
	case DIRECT_CHARGE_FAULT_VDROP_OVP:
		hwlog_err("vdrop ovp happened!\n");
		snprintf(buf, sizeof(buf), "vdrop ovp happened\n");
		strncat(buf, reg_info, strlen(reg_info));
		//dsm_report(DSM_DIRECT_CHARGE_VDROP_OVP, buf);
		break;
	default:
		hwlog_err("unknow fault: %d happened!\n", di->charge_fault);
		break;
	}
}

static int direct_charge_fault_notifier_call(struct notifier_block *fault_nb, unsigned long event, void *data)
{
	struct direct_charge_device *di = container_of(fault_nb, struct direct_charge_device, fault_nb);
	enum scp_stage_type stage = scp_get_stage_status();

	if (stage < SCP_STAGE_SECURITY_CHECK || stage == SCP_STAGE_CHARGE_DONE)
	{
		hwlog_err("notify event:%d when direct charge not initialized, so ignored!\n", di->charge_fault);
		return NOTIFY_OK;
	}
	di->charge_fault = (enum charge_fault_type)event;
	di->fault_data = (struct nty_data*)data;
	schedule_work(&di->fault_work);
	return NOTIFY_OK;
}
/*lint -restore*/

/*lint -save -e* */
static int direct_charge_probe(struct platform_device	*pdev)
{
	int ret = 0;
	struct direct_charge_device *di;
	struct class *power_class = NULL;
	struct device_node *np = NULL;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di)
	{
		hwlog_err("alloc di failed\n");
		return	-ENOMEM;
	}
	di->dev = &pdev->dev;
	np = di->dev->of_node;
	ret = direct_charge_parse_dts(np, di);
	if (ret)
	{
		hwlog_err("parse dts fail\n");
		goto fail_0;
	}
	battery_aging_safe_policy(di);
	di->scp_ops = g_scp_ops;
	di->vbat_ovp_enable_charger = 1;
	di->ls_ops = g_ls_ops;
	di->bi_ops = g_bi_ops;
	di->direct_charge_cable_detect = g_cable_detect_ops;
	di->sysfs_enable_charger = 1;
	di->sysfs_iin_thermal = di->volt_para[0].cur_th_high;
	di->max_adaptor_iset = di->volt_para[0].cur_th_high;
	di->direct_charge_succ_flag = DIRECT_CHARGE_ERROR_ADAPTOR_DETECT;
	if ((NULL == di->scp_ops) || (NULL == di->ls_ops) || (NULL == di->bi_ops))
	if (INVALID == is_direct_charge_ops_valid(di))
	{
		hwlog_err("direct charge ops is	NULL!\n");
		ret = -EINVAL;
		goto fail_0;
	}
	if (di->use_gpio_for_ovp_en) {
		ret = gpio_request(di->ovp_en, "ovp_en");
		if (ret) {
			hwlog_err("could not request ovp_en,ret = %d\n",ret);
			//ret = -ENOMEM;
			//goto fail_0;
		}
		ret = gpio_direction_input(di->ovp_en);/*avoid ovp_en to hiz mode*/
		if (ret) {
			hwlog_err("could not set ovp_en input\n");
			//ret = -ENOMEM;
			//goto free_scp_power_en;
		}
	}

	platform_set_drvdata(pdev, di);


	di->direct_charge_wq = create_singlethread_workqueue("direct_charge_wq");
	di->direct_charge_watchdog_wq = create_singlethread_workqueue("direct_charge_watchdog_wq");

	wake_lock_init(&direct_charge_lock, WAKE_LOCK_SUSPEND, "direct_charge_wakelock");
	INIT_WORK(&di->threshold_caculation_work, threshold_caculation_work);
	INIT_WORK(&di->charge_control_work, charge_control_work);
	INIT_WORK(&di->fault_work, direct_charge_fault_work);
	INIT_WORK(&di->kick_watchdog_work, kick_watchdog_work);


	hrtimer_init(&di->threshold_caculation_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	di->threshold_caculation_timer.function	= threshold_caculation_timer_func;

	hrtimer_init(&di->charge_control_timer,	CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	di->charge_control_timer.function = charge_control_timer_func;

	hrtimer_init(&di->kick_watchdog_timer,	CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	di->kick_watchdog_timer.function = kick_watchdog_timer_func;

	ret = direct_charge_sysfs_create_group(di);
	if (ret)
		hwlog_err("can't create	charge sysfs entries\n");
	power_class = hw_power_get_class();
	if (power_class)
	{
		if (charge_dev == NULL)
			charge_dev = device_create(power_class,	NULL, 0, NULL, "charger");
		ret = sysfs_create_link(&charge_dev->kobj, &di->dev->kobj, "direct_charger");
		if (ret)
		{
			hwlog_err("create link to direct_charger fail.\n");
			goto free_ovp_en;
		}
	}
	g_di = di;

	di->fault_nb.notifier_call = direct_charge_fault_notifier_call;
	ret = atomic_notifier_chain_register(&direct_charge_fault_notifier_list, &di->fault_nb);
	if (ret < 0)
	{
		hwlog_err("direct_charge_fault_register_notifier failed\n");
		goto free_sysfs_group;
	}
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
	ret = di->ls_ops->ls_init();
	di->ls_id = di->ls_ops->get_ls_id();
	if (di->ls_id < 0 || di->ls_id >= LOADSWITCH_TOTAL)
	{
		hwlog_err("error loadswitch id info\n");
		di->ls_id = LOADSWITCH_TOTAL;
	}
	di->ls_name = loadswitch_name[di->ls_id];
	hwlog_info("loadswitch id = %d(%s)\n", di->ls_id, di->ls_name);
	ret |= di->ls_ops->ls_exit();
	if (ret)
	{
		hwlog_err("dev_check for loadswitch fail.\n");
	}
	else
	{
		set_hw_dev_flag(DEV_I2C_LOADSWITCH);
		hwlog_info("dev_check for loadswitch succ.\n");
	}
#endif
	hwlog_info("direct charger probe ok!\n");
	return	0;

free_sysfs_group:
	direct_charge_sysfs_remove_group(di);
free_ovp_en:
	gpio_free(di->ovp_en);
fail_0:
	kfree(di);
	di = NULL;
	return	ret;
}

/**********************************************************
*	Function: direct_charge_remove
*	Description: direct_charge module remove
*	Parameters: pdev:platform_device
*	return value: NULL
**********************************************************/
static int direct_charge_remove(struct platform_device *pdev)
{
	struct direct_charge_device *di = platform_get_drvdata(pdev);

	hwlog_info("%s ++\n", __func__);
	if (NULL == di)
	{
		hwlog_err("[%s]di is NULL!\n", __func__);
		return -ENODEV;
	}

	hwlog_info("%s	--\n", __func__);

	return 0;
}
/**********************************************************
*	Function: direct_charge_shutdown
*	Description: direct_charge module shutdown
*	Parameters: pdev:platform_device
*	return value: NULL
**********************************************************/
static void direct_charge_shutdown(struct platform_device *pdev)
{
	struct direct_charge_device *di = platform_get_drvdata(pdev);

	hwlog_info("%s ++\n", __func__);
	if (NULL == di)
	{
		hwlog_err("[%s]di is NULL!\n", __func__);
		return;
	}

	hwlog_info("%s	--\n", __func__);

	return;
}

#ifdef	CONFIG_PM
/**********************************************************
*	Function: direct_charge_suspend
*	Description: direct charge module suspend
*	Parameters: pdev:platform_device
*		    state:unused
*	return	value: 0-sucess	or others-fail
**********************************************************/
static int direct_charge_suspend(struct	platform_device	*pdev, pm_message_t state)
{
	hwlog_info("%s ++\n", __func__);

	hwlog_info("%s --\n", __func__);

	return	0;
}

/**********************************************************
*	Function: direct charge_resume
*	Description: direct charge module resume
*	Parameters: pdev:platform_device
*	return	value: 0-sucess	or others-fail
**********************************************************/
static int direct_charge_resume(struct	platform_device	*pdev)
{
	hwlog_info("%s	++\n", __func__);

	hwlog_info("%s --\n", __func__);

	return	0;
}
#endif	/* CONFIG_PM */

static struct of_device_id direct_charge_match_table[] = {
	{
		.compatible = "direct_charger",
		.data = NULL,
	},
	{
	},
};

static struct platform_driver direct_charge_driver = {
	.probe = direct_charge_probe,
	.remove	= direct_charge_remove,
#ifdef	CONFIG_PM
	.suspend = direct_charge_suspend,
	.resume	= direct_charge_resume,
#endif
	.shutdown = direct_charge_shutdown,
	.driver	= {
		.name = "direct_charger",
		.owner = THIS_MODULE,
		.of_match_table	= of_match_ptr(direct_charge_match_table),
	},
};

/**********************************************************
*	Function: direct_charge_init
*	Description: direct charge module initialization
*	Parameters: NULL
*	return value: 0-sucess or others-fail
**********************************************************/
static int __init direct_charge_init(void)
{
	return platform_driver_register(&direct_charge_driver);
}

/**********************************************************
*	Function: direct_charge_exit
*	Description: direct charge module exit
*	Parameters: NULL
*	return	value:	NULL
**********************************************************/
static void __exit direct_charge_exit(void)
{
	platform_driver_unregister(&direct_charge_driver);
}
/*lint -restore*/

late_initcall(direct_charge_init);
module_exit(direct_charge_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("direct charger module driver");
MODULE_AUTHOR("HUAWEI Inc");
