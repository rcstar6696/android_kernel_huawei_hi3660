#ifndef __DIRECT_CHARGER_H_
#define __DIRECT_CHARGER_H_

#include <linux/module.h>
#include <linux/device.h>
#include <linux/notifier.h>
#include <huawei_platform/power/huawei_charger.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/hrtimer.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#ifndef BIT
#define BIT(x) (1 << x)
#endif
/*scp adaptor register*/
#define ADAPTOR_SCP_DETECT_FAIL -1
#define SCP_ADP_TYPE 0x80
#define SCP_ADP_TYPE_B_MASK (BIT(5)|BIT(4))
#define SCP_ADP_TYPE_B 0x10

#define SCP_B_ADP_TYPE 0x81
#define SCP_B_DIRECT_ADP 0x10
#define DOUBLE_SIZE 2

#define SCP_VENDOR_ID_H 0x82
#define SCP_VENDOR_ID_L 0x83
#define SCP_MODULE_ID_H 0x84
#define SCP_MODULE_ID_L 0x85
#define SCP_SERRIAL_NO_H 0x86
#define SCP_SERRIAL_NO_L 0x87
#define SCP_PCHIP_ID 0x88
#define SCP_HWVER 0x89
#define SCP_FWVER_H 0x8a
#define SCP_FWVER_L 0x8b
#define SCP_SPID 0x8c
#define SCP_NEED_OPEN_OUTPUT2 0x31
#define SCP_MAX_POWER 0x90
#define SCP_CNT_POWER 0x91
#define SCP_MIN_VOUT 0x92
#define SCP_MAX_VOUT 0x93
#define SCP_MIN_IOUT 0x94
#define SCP_MAX_IOUT 0x95
#define SCP_MAX_IOUT_A_MASK (BIT(7) | BIT(6))
#define SCP_MAX_IOUT_A_SHIFT 6
#define SCP_MAX_IOUT_B_MASK (BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define SCP_VSTEP 0x96
#define SCP_ISTEP 0x97
#define SCP_MAX_VERR 0x98
#define SCP_MAX_IVRR 0x99
#define SCP_MAX_STTIME 0x9a
#define SCP_MAX_RSPTIME 0x9b
#define SCP_SCTRL 0x9e
#define SCP_OUTPUT2_ENABLE_MASK BIT(7)
#define SCP_OUTPUT2_ENABLE BIT(7)
#define SCP_CTRL_BYTE0 0xa0
#define SCP_OUTPUT_MODE_MASK BIT(6)
#define SCP_OUTPUT_MODE_ENABLE BIT(6)
#define SCP_OUTPUT_MODE_DISABLE 0
#define SCP_OUTPUT_MASK BIT(7)
#define SCP_OUTPUT_ENABLE BIT(7)
#define SCP_OUTPUT_DISABLE 0
#define SCP_ADAPTOR_RESET_MASK BIT(5)
#define SCP_ADAPTOR_RESET_ENABLE BIT(5)
#define SCP_ADAPTOR_RESET_DISABLE 0
#define SCP_CTRL_BYTE1 0xa1
#define SCP_WATCHDOG_BITS_PER_SECOND (2)
#define SCP_WATCHDOG_MASK (BIT(2) | BIT(1) | BIT(0))
#define SCP_STATUS_BYTE0 0xa2
#define SCP_CABLE_STS_MASK BIT(3)
#define SCP_PORT_LEAKAGE_INFO BIT(4)
#define SCP_PORT_LEAKAGE_SHIFT 4
#define SCP_STATUS_BYTE1 0xa3
#define SCP_SSTS 0xa5
#define SCP_SSTS_A_MASK (BIT(2) | BIT(1) | BIT(0))
#define SCP_SSTS_B_MASK (BIT(7))
#define SCP_SSTS_B_SHIFT 7
#define SCP_INSIDE_TMP 0xa6
#define SCP_PORT_TMP 0xa7
#define SCP_READ_VOLT_L 0xa8
#define SCP_READ_VOLT_H 0xa9
#define SCP_READ_IOLT_L 0xaa
#define SCP_READ_IOLT_H 0xab
#define SCP_SREAD_VOUT 0xc8
#define SCP_SREAD_VOUT_OFFSET 3000
#define SCP_SREAD_VOUT_STEP 10
#define SCP_SREAD_IOUT 0xc9
#define SCP_SREAD_IOUT_STEP 50
#define SCP_DAC_VSET_L 0xac
#define SCP_DAC_VSET_H 0xad
#define SCP_DAC_ISET_L 0xae
#define SCP_DAC_ISET_H 0xaf
#define SCP_VSET_BOUNDARY_L 0xb0
#define SCP_VSET_BOUNDARY_H 0xb1
#define SCP_ISET_BOUNDARY_L 0xb2
#define SCP_ISET_BOUNDARY_H 0xb3
#define SCP_MAX_VSET_OFFSET 0xb4
#define SCP_MAX_ISET_OFFSET 0xb5
#define SCP_VSET_L 0xb8
#define SCP_VSET_H 0xb9
#define SCP_ISET_L 0xba
#define SCP_ISET_H 0xbb
#define SCP_VSSET 0xca
#define VSSET_OFFSET 3000
#define VSSET_STEP 10
#define SCP_ISSET 0xcb
#define ISSET_STEP 50
#define SCP_VSET_OFFSET_L 0xbc
#define SCP_VSET_OFFSET_H 0xbd
#define SCP_ISET_OFFSET_L 0xbe
#define SCP_ISET_OFFSET_H 0xbf
#define SCP_STEP_VSET_OFFSET 0xcc
#define SCP_STEP_ISET_OFFSET 0xcd

#define SCP_ADAPTOR_DETECT_FAIL 1
#define SCP_ADAPTOR_DETECT_SUCC 0
#define SCP_ADAPTOR_DETECT_OTHER -1
#define SCP_CABLE_DETECT_FAIL 1
#define SCP_CABLE_DETECT_SUCC 0
#define SCP_CABLE_DETECT_ERROR -1

#define ERROR_RESISTANCE -99999

#define DC_ERR_CNT_MAX            (8)
#define DC_OPEN_RETRY_CNT_MAX      (3)
#define DC_VOLT_LEVEL             (5)
#define DC_TEMP_LEVEL             (5)

#define STRTOL_MAX_LEN     10

#define FAIL -1
#define SUCC 0

#define INVALID -1
#define VALID 0
#define WAIT_LS_DISCHARGE 200
#define MAX_TIMES_FOR_SET_ADAPTER_VOL_20 20
#define ADAPTER_VOL_DIFFRENCE_300_MV 300
#define MAX_ADAPTER_VOL_4400_MV 4400
#define MIN_ADAPTER_VOL_STEP_20_MV 20
#define MIN_CURRENT_FOR_RES_DETECT_800_MA 800
#define MAX_VOL_FOR_BATTERY_4360_MV 4360
#define CURRENT_SET_FOR_RES_DETECT_1000_MA 1000

#define VENDOR_ID_RICHTEK 0x01
#define VENDOR_ID_IWATT 0x03

#define MAX_IOUT_EXP_0 0
#define MAX_IOUT_EXP_1 1
#define MAX_IOUT_EXP_2 2
#define MAX_IOUT_EXP_3 3
#define TEN_EXP_0 1
#define TEN_EXP_1 10
#define TEN_EXP_2 100
#define TEN_EXP_3 1000
#define DROP_POWER_FLAG 1
#define DROP_POWER_FACTOR 8
#define SCP_NO_ERR 0
#define SCP_IS_ERR 1
#define ONE_BYTE_LEN 8
#define ONE_BYTE_MASK 0xff
#define ONE_BIT_EQUAL_TWO_SECONDS 2
#define WAIT_FOR_ADAPTOR_RESET 50
#define OUTPUT_MODE_ENABLE 1
#define OUTPUT_MODE_DISABLE 0
#define ADAPTOR_RESET 1
#define GPIO_ENABLE 1
#define GPIO_DISABLE 0
#define IN_SCP_CHARGING_STAGE 0
#define NOT_IN_SCP_CHARGING_STAGE -1
#define ENABLE 1
#define DISABLE 0
#define SET_HIZ_ENABLE 1
#define SET_HIZ_DISABLE 0

#define SCP_ADAPTOR_DETECT 1
#define SCP_ADAPTOR_NOT_DETECT 0

#define VBUS_ON_THRESHOLD 3000
#define VBAT_VBUS_DIFFERENCE 150
#define KICK_WATCHDOG_TIME 1000
#define BATTERY_CAPACITY_HIGH_TH 95

enum adapter_vendor{
	RICHTEK_ADAPTER,
	IWATT_ADAPTER,
};
enum direct_charge_error_code {
	DIRECT_CHARGE_SUCC,
	DIRECT_CHARGE_ERROR_CHARGE_DISABLED,
	DIRECT_CHARGE_ERROR_ADAPTOR_DETECT,
	DIRECT_CHARGE_ERROR_BAT_TEMP,
	DIRECT_CHARGE_ERROR_BAT_VOL,
	DIRECT_CHARGE_ERROR_SWITCH,
	DIRECT_CHARGE_ERROR_INIT,
	DIRECT_CHARGE_ERROR_USB_PORT_LEAKAGE_CURRENT,
	DIRECT_CHARGE_ERROR_ADAPTOR_VOLTAGE_ACCURACY,
	DIRECT_CHARGE_ERROR_OPEN_CHARGE_PATH,
	DIRECT_CHARGE_ERROR_FULL_REISISTANCE,
};
enum scp_retry_operate_type {
	SCP_RETRY_OPERATE_DEFAUTL,
	SCP_RETRY_OPERATE_RESET_ADAPTER,
	SCP_RETRY_OPERATE_RESET_CHIP,
	SCP_RETRY_OPERATE_UNVALID,
};

enum scp_stage_type {
	SCP_STAGE_DEFAULT,
	SCP_STAGE_SUPPORT_DETECT,
	SCP_STAGE_ADAPTER_DETECT,
	SCP_STAGE_SWITCH_DETECT,
	SCP_STAGE_CHARGE_INIT,
	SCP_STAGE_SECURITY_CHECK,
	SCP_STAGE_SUCCESS,
	SCP_STAGE_CHARGING,
	SCP_STAGE_CHARGE_DONE,
};

enum dc_temp_info {
	DC_TEMP_MIN =0,
	DC_TEMP_MAX,
	DC_CUR_MAX,
	DC_TEMP_TOTAL,
};

enum dc_volt_info {
	DC_PARA_VOL_TH = 0,
	DC_PARA_CUR_TH_HIGH,
	DC_PARA_CUR_TH_LOW,
	DC_PARA_TOTAL,
};

enum direct_charge_sysfs_type {
	DIRECT_CHARGE_SYSFS_ENABLE_CHARGER = 0,
	DIRECT_CHARGE_SYSFS_IIN_THERMAL,
	DIRECT_CHARGE_SYSFS_ADAPTOR_DETECT,
	DIRECT_CHARGE_SYSFS_LOADSWITCH_ID,
	DIRECT_CHARGE_SYSFS_LOADSWITCH_NAME,
	DIRECT_CHARGE_SYSFS_VBAT,
	DIRECT_CHARGE_SYSFS_IBAT,
	DIRECT_CHARGE_SYSFS_VADAPT,
	DIRECT_CHARGE_SYSFS_IADAPT,
	DIRECT_CHARGE_SYSFS_LS_VBUS,
	DIRECT_CHARGE_SYSFS_LS_IBUS,
	DIRECT_CHARGE_SYSFS_FULL_PATH_RESISTANCE,
	DIRECT_CHARGE_SYSFS_DIRECT_CHARGE_SUCC,
};

enum loadswitch_id {
	LOADSWITCH_RICHTEK,
	LOADSWITCH_TI,
	LOADSWITCH_FAIRCHILD,
	LOADSWITCH_NXP,
	LOADSWITCH_TOTAL,
};

enum direct_charge_fault_type {
	DIRECT_CHARGE_FAULT_NON = 0,
	DIRECT_CHARGE_FAULT_VBUS_OVP,
	DIRECT_CHARGE_FAULT_REVERSE_OCP,
	DIRECT_CHARGE_FAULT_OTP,
	DIRECT_CHARGE_FAULT_INPUT_OCP,
	DIRECT_CHARGE_FAULT_VDROP_OVP,
	DIRECT_CHARGE_FAULT_TOTAL,
};
static const char *const loadswitch_name[] = {
	[0] = "RICHTEK",
	[1] = "TI",
	[2] = "FAIRCHILD",
	[3] = "NXP",
	[4] = "ERROR",
};
static const char *const scp_check_stage[] = {
	[0] = "SCP_STAGE_DEFAULT",
	[1] = "SCP_STAGE_SUPPORT_DETECT",
	[2] = "SCP_STAGE_ADAPTER_DETECT",
	[3] = "SCP_STAGE_SWITCH_DETECT",
	[4] = "SCP_STAGE_CHARGE_INIT",
	[5] = "SCP_STAGE_SECURITY_CHECK",
	[6] = "SCP_STAGE_SUCCESS",
	[7] = "SCP_STAGE_CHARGING",
	[8] = "SCP_STAGE_CHARGE_DONE",
};

struct scp_init_data {
	int scp_mode_enable;
	int vset_boundary;
	int iset_boundary;
	int init_adaptor_voltage;
	int watchdog_timer;
};

struct nty_data {
	unsigned short addr;
	u8 event1;
	u8 event2;
};

struct smart_charge_ops {
	int (*is_support_scp)(void);
	int (*scp_init)(struct scp_init_data*);
	int (*scp_exit)(struct direct_charge_device*);
	int (*scp_adaptor_detect)(void);
	int (*scp_set_adaptor_voltage)(int);
	int (*scp_get_adaptor_voltage)(void);
	int (*scp_set_adaptor_current)(int);
	int (*scp_get_adaptor_current)(void);
	int (*scp_get_adaptor_current_set)(void);
	int (*scp_get_adaptor_max_current)(void);
	int (*scp_adaptor_reset)(void);
	int (*scp_adaptor_output_enable)(int);
	int (*scp_chip_reset)(void);
	int (*scp_stop_charge_config)(void);
	int (*is_scp_charger_type)(void);
	int (*scp_get_adaptor_status)(void);
	int (*scp_get_chip_status)(void);
	int (*scp_get_adaptor_info)(void*);
	int (*scp_cable_detect)(void);
	int (*scp_get_adaptor_temp)(int*);
	int (*scp_get_adapter_vendor_id)(void);
	int (*scp_get_usb_port_leakage_current_info)(void);
	int (*scp_power_enable)(int);
};
struct loadswitch_ops {
	int (*ls_init)(void);
	int (*ls_exit)(void);
	int (*ls_enable)(int);
	int (*ls_discharge)(int);
	int (*is_ls_close)(void);
	int (*get_ls_id)(void);
	int (*watchdog_config_ms)(int time);
};
struct batinfo_ops {
	int (*init)(void);
	int (*exit)(void);
	int (*get_bat_btb_voltage)(void);
	int (*get_bat_package_voltage)(void);
	int (*get_vbus_voltage)(int* vol);
	int (*get_bat_current)(int* cur);
	int (*get_ls_ibus)(int *ibus);
	int (*get_ls_temp)(int *temp);
};

/*currently we support 5 cc stage*/
struct dc_volt_para_info {
	int vol_th;
	int cur_th_high;
	int cur_th_low;
};
struct dc_temp_para_info {
	int temp_min;
	int temp_max;
	int cur_max;;
};
struct adaptor_info {
	int b_adp_type;
	int vendor_id_h;
	int vendor_id_l;
	int module_id_h;
	int module_id_l;
	int serrial_no_h;
	int serrial_no_l;
	int pchip_id;
	int hwver;
	int fwver_h;
	int fwver_l;
};
struct direct_charge_device {
	struct device *dev;
	struct smart_charge_ops* scp_ops;
	struct loadswitch_ops* ls_ops;
	struct batinfo_ops* bi_ops;
	struct direct_charge_cable_detect_ops* direct_charge_cable_detect;
	struct hrtimer threshold_caculation_timer;
	struct hrtimer charge_control_timer;
	struct hrtimer kick_watchdog_timer;
	struct workqueue_struct *direct_charge_wq;
	struct workqueue_struct *direct_charge_watchdog_wq;
	struct work_struct threshold_caculation_work;
	struct work_struct charge_control_work;
	struct work_struct fault_work;
	struct work_struct kick_watchdog_work;
	struct notifier_block fault_nb;
	struct dc_volt_para_info volt_para[DC_VOLT_LEVEL];
	struct dc_temp_para_info temp_para[DC_TEMP_LEVEL];
	struct adaptor_info adp_info;
	struct nty_data *fault_data;
	int stage_need_to_jump[2*DC_VOLT_LEVEL];
	int error_cnt;
	unsigned int use_5A;
	int sysfs_enable_charger;
	int vbat_ovp_enable_charger;
	int sysfs_iin_thermal;
	int threshold_caculation_interval;
	int charge_control_interval;
	int cur_stage;
	int cur_vbat_th;
	int cur_ibat_th_high;
	int cur_ibat_th_low;
	int vbat;
	int ibat;
	int ibat_abnormal_cnt;
	int ibat_abnormal_th;
	int vadapt;
	int iadapt;
	int tadapt;
	int ls_vbus;
	int ls_ibus;
	int tls;
	int full_path_resistance;
	int stage_size;
	int pre_stage;
	int adaptor_vset;
	int max_adaptor_vset;
	unsigned int max_tadapt;
	unsigned int max_tls;
	int adaptor_iset;
	int max_adaptor_iset;
	int delta_err;
	int vstep;
	int scp_stop_charging_flag_info;
	int scp_stop_charging_flag_error;
	int max_dc_bat_vol;
	int min_dc_bat_vol;
	int ovp_en;
	int bst_ctrl;
	int scp_power_en;
	int compensate_r;
	int compensate_v;
	int ls_id;
	char* ls_name;
	int vol_err_th;
	int full_path_res_max;
	int standard_cable_full_path_res_max;
	int full_path_res_threshold;
	int adaptor_leakage_current_th;
	int direct_charge_succ_flag;
	int first_cc_stage_timer_in_min;
	int max_adaptor_cur;
	int scp_cable_detect_enable;
	unsigned long first_cc_stage_timeout;
	int cc_cable_detect_enable;
	int cc_cable_detect_ok;
	int max_current_for_none_standard_cable;
	enum direct_charge_fault_type charge_fault;
	int adaptor_vendor_id;
	int is_need_bst_ctrl;
	int boost_5v_use_common_gpio;
	int scp_work_on_charger;
	int dc_err_report_flag;
	int use_gpio_for_ovp_en;
	int dc_open_retry_cnt;
};

extern struct atomic_notifier_head direct_charge_fault_notifier_list;

struct direct_charge_cable_detect_ops {
	int (*direct_charge_cable_detect)(void);
};
int cable_detect_ops_register(struct direct_charge_cable_detect_ops*);
int scp_ops_register(struct smart_charge_ops*);
int loadswitch_ops_register(struct loadswitch_ops*);
int batinfo_ops_register(struct batinfo_ops*);
void direct_charge_check(void);
void direct_charge_update_cutoff_flag(void);
void direct_charge_stop_charging(void);
enum scp_stage_type scp_get_stage_status(void);
void scp_set_stop_charging_flag(int flag);
int is_scp_stop_charging_complete(void);
int get_ls_vbus(void);
int scp_adaptor_detect(void);
int scp_adaptor_set_output_enable(int enable);
int direct_charge_get_cutoff_normal_flag(void);
int get_quick_charge_flag(void);
int get_super_charge_flag(void);
void vbat_ovp_exit_direct_charge(int enable_charge);
int vbat_ovp_scp_handle(void);
int is_direct_charge_failed(void);
int is_in_scp_charging_stage(void);
#ifdef CONFIG_SCHARGER_V300
#define HI6523_CV_CUT 150
extern bool is_hi6523_cv_limit(void);
#endif

#ifdef CONFIG_LLT_TEST
#endif
#endif
