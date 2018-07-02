

#include "hisi_coul_core.h"
#ifdef CONFIG_HUAWEI_CHARGER_SENSORHUB
#include "inputhub_bridge.h"
#endif

#ifdef CONFIG_HUAWEI_PLATFORM
#include <huawei_platform/log/hw_log.h>
#define HWLOG_TAG hisi_coul_core
HWLOG_REGIST();
#else
#define hwlog_debug(fmt, args...)do { printk(KERN_DEBUG   "[hisi_coul_core]" fmt, ## args); } while (0)
#define hwlog_info(fmt, args...) do { printk(KERN_INFO    "[hisi_coul_core]" fmt, ## args); } while (0)
#define hwlog_warn(fmt, args...) do { printk(KERN_WARNING"[hisi_coul_core]" fmt, ## args); } while (0)
#define hwlog_err(fmt, args...)  do { printk(KERN_ERR   "[hisi_coul_core]" fmt, ## args); } while (0)
#endif

#define LOW_INT_VOL_COUNT 3
#define LOW_INT_VOL_SLEEP_TIME 1000

char* p_charger = NULL;/*0x34A10000~0x34A11000 is reserved for pmu coulomb, we use these to transfer coul information from fastboot to kernel,we add charger info*/
extern struct blocking_notifier_head notifier_list;
extern unsigned int get_pd_charge_flag(void);
struct device *coul_dev = NULL;
static struct coul_device_ops *g_coul_core_ops = NULL;
static struct smartstar_coul_device *g_smartstar_coul_dev = NULL;
static struct ss_coul_nv_info my_nv_info;
static struct platform_device *g_pdev = NULL;
static unsigned long nv_info_addr = 0;
static int g_eco_leak_uah = 0;

static u32 is_board_type = 0;
static u32 battery_is_removable = 0;
static u32 adc_batt_id = DEFAULT_HKADC_BATT_ID;
static u32 adc_batt_temp = DEFAULT_HKADC_BATT_TEMP;
static s32 batt_temp_too_hot = TEMP_TOO_HOT;
static s32 batt_temp_too_cold = TEMP_TOO_COLD;
int pl_calibration_en = FALSE;
int v_offset_a = DEFAULT_V_OFF_A;
int v_offset_b = 0;
int c_offset_a = DEFAULT_C_OFF_A;
int c_offset_b = 0;

static int delta_sleep_time_offset = 30; //sleep time offset, in s
static int delta_sleep_time = 10*60; // sleep time bigger could update ocv, in s
static int delta_sleep_current = 50; // sleep current less could updat eocv, in mA

static unsigned int hand_chg_capacity_flag = 0;
static unsigned int input_capacity = 50;
static int disable_temperature_debounce = 1; /*lint !e551*/

static int sr_time_sleep[SR_ARR_LEN];
static int sr_time_wakeup[SR_ARR_LEN];
static int sr_index_sleep = 0;
static int sr_index_wakeup = 0;
static int sr_cur_state = 0;    // 1:wakeup  2:sleep

static int last_cali_time;
static int sr_suspend_temp;
static int last_cali_temp;
static int batt_init_level = 0;

/*low temp soc show optimize*/
static int low_temp_opt_flag = LOW_TEMP_OPT_CLOSE; /*1:open  0:close*/
static int uuc_current_min   = UUC_MIN_CURRENT_MA;
static int ratio_min         = 100;
static int g_basp_full_pc;
/*NTC Table*/
static int T_V_TABLE[][2] =
{
    {-273, 4095}, {-40, 3764}, {-36, 3689}, {-32, 3602}, {-28, 3500},
    {-24, 3387}, {-20, 3261}, {-16, 3122}, {-12, 2973}, {-8, 2814},
    {-4, 2650}, {0, 2480}, {4, 2308}, {8, 2136}, {12, 1967},
    {16, 1803}, {20, 1646}, {24, 1497}, {28, 1360}, {32, 1230},
    {36, 1111}, {40, 1001}, {44, 903}, {48, 812}, {52, 729},
    {56, 655}, {60, 590}, {64, 531}, {74, 406}, {84, 313},
    {125, 110}, {0, 0},
};

static struct OCV_LEVEL_PARAM ocv_level_para[OCV_LEVEL_MAX] =
{
	{0,  50, 570,9999,9999, -200,  0,     0,  1, 1, 1},
	{95, 50, 240,  30,9999, -200,  0,     0,  1, 1, 1},
	{95, 50, 120,  15, 500,    0,  0,     0,  0, 1, 1},
	{98, 50,  60,   4, 250,    0,  0,     0,  0, 1, 1},
	{95, 50,  60,   4, 250,    0,  3,  3600,  0, 0, 1},
	{95, 50,  30,   4, 250,    0,  6,  7200,  1, 0, 1},
	{95, 50,  30,   4, 500,  100,  8, 10800,  1, 0, 1},
	{90, 50,   3,   4, 250,  100, 10, 18000,  1, 0, 1}
};

static struct AVG_CURR2UPDATE_OCV curr2update_ocv[AVG_CURR_MAX] = {{0,0},{0,0}};
static u8 delta_soc_renew_ocv = 0;
static u8 g_test_delta_soc_renew_ocv = 0;
static u32 multi_ocv_open_flag = 0;
static u32 fcc_update_limit_flag = 0;
ATOMIC_NOTIFIER_HEAD(coul_fault_notifier_list);
static int basp_learned_fcc = 0;
static struct battery_aging_safe_policy basp_policy[BASP_PARA_LEVEL];
static enum BASP_FCC_LEARN_STATE {
    LS_UNKNOWN,
    LS_INIT,
    LS_GOOD,
    LS_BAD,
} basp_fcc_ls = LS_UNKNOWN;
static void coul_get_rm(struct smartstar_coul_device *di, int *rm);
static int hisi_coul_pm_notify(struct notifier_block * nb, unsigned long mode, void * unused);
BASP_LEVEL_TYPE get_basp_level(struct smartstar_coul_device *di);
struct coul_core_info_sh *g_di_coul_info_sh = NULL;
static struct wake_lock coul_lock;
static void iscd_clear_sampled_info(struct smartstar_coul_device *di);
static void check_batt_critical_electric_leakage(struct smartstar_coul_device *di);
static char dsm_buff[ISCD_DSM_LOG_SIZE_MAX] = { 0 };
#ifdef CONFIG_HISI_DEBUG_FS
int print_multi_ocv_threshold(void);
#endif

static struct coul_ocv_cali_info g_coul_ocv_cali_info[INDEX_MAX];
static int g_ocv_cali_index = 0;
static int g_ocv_cali_rbatt_valid_flag = 0;

static int get_timestamp(char *str, int len)
{
    struct timeval tv;
    struct rtc_time tm;

    if(NULL == str) {
        hwlog_err("%s input para is null.\n", __func__);
        return -EINVAL;
    }

    do_gettimeofday(&tv);
    tv.tv_sec -= (long)sys_tz.tz_minuteswest * 60;
    rtc_time_to_tm(tv.tv_sec, &tm);

    snprintf(str, len, "%04d-%02d-%02d %02d:%02d:%02d",
         tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    return strlen(str);
}

static void record_ocv_cali_info(struct smartstar_coul_device *di)
{
    static int index = 0;
    char timestamp[TIMESTAMP_STR_SIZE] = {0};

    if (!di) {
        hwlog_err("%s input para is null.\n", __func__);
        return;
    }

    /*clean buff*/
    memset(&g_coul_ocv_cali_info[index], 0, sizeof(g_coul_ocv_cali_info[index]));

    /*get timestamp*/
    get_timestamp(timestamp, TIMESTAMP_STR_SIZE);
    snprintf(g_coul_ocv_cali_info[index].cali_timestamp, TIMESTAMP_STR_SIZE, "%s", timestamp);

    /*save log:ocv,ocv_temp,cc,level*/
    g_coul_ocv_cali_info[index].cali_ocv_uv = di->batt_ocv;
    g_coul_ocv_cali_info[index].cali_ocv_temp = di->batt_ocv_temp;
    g_coul_ocv_cali_info[index].cali_cc_uah = di->coul_dev_ops->calculate_cc_uah();
    g_coul_ocv_cali_info[index].cali_ocv_level = di->last_ocv_level;

    /*Save the currently used buff number to the global variable*/
    g_ocv_cali_index = index;
    g_ocv_cali_rbatt_valid_flag = 0;

    /*switch buffer number*/
    index++;
    index = index % INDEX_MAX;
}

static int coul_dsm_report_ocv_cali_info(struct smartstar_coul_device *di, int err_num, const char *buff)
{
    char timestamp[TIMESTAMP_STR_SIZE] = {0};
    char dsm_buf[DSM_BUFF_SIZE_MAX] = {0};
    int i = 0, tmp_len = 0;

    if(!di || NULL == buff) {
        hwlog_err("null point in [%s].\n", __func__);
        return -1;
    }

    /*get timestamp*/
    get_timestamp(timestamp, TIMESTAMP_STR_SIZE);
    tmp_len += snprintf(dsm_buf, DSM_BUFF_SIZE_MAX, "%s\n", timestamp);

    /*common info:brand,cycles*/
    tmp_len += snprintf(dsm_buf + tmp_len, DSM_BUFF_SIZE_MAX - tmp_len, "batteryName:%s, chargeCycles:%d\n",
        di->batt_data->batt_brand, di->batt_chargecycles/PERCENT);

    /*key info*/
    tmp_len += snprintf(dsm_buf + tmp_len, DSM_BUFF_SIZE_MAX - tmp_len, "%s\n", buff);

    /*OCV history calibration information*/
    for (i = 0; i < INDEX_MAX; i++) {
        tmp_len += snprintf(dsm_buf + tmp_len, DSM_BUFF_SIZE_MAX - tmp_len, "[OCV calibration]%s ", g_coul_ocv_cali_info[i].cali_timestamp);
        tmp_len += snprintf(dsm_buf + tmp_len, DSM_BUFF_SIZE_MAX - tmp_len, "OCV:%duV ", g_coul_ocv_cali_info[i].cali_ocv_uv);
        tmp_len += snprintf(dsm_buf + tmp_len, DSM_BUFF_SIZE_MAX - tmp_len, "temp:%d ", g_coul_ocv_cali_info[i].cali_ocv_temp);
        tmp_len += snprintf(dsm_buf + tmp_len, DSM_BUFF_SIZE_MAX - tmp_len, "CC:%duAh ", (int)g_coul_ocv_cali_info[i].cali_cc_uah);
        tmp_len += snprintf(dsm_buf + tmp_len, DSM_BUFF_SIZE_MAX - tmp_len, "rbatt:%d ", g_coul_ocv_cali_info[i].cali_rbatt);
        tmp_len += snprintf(dsm_buf + tmp_len, DSM_BUFF_SIZE_MAX - tmp_len, "ocv_cali_level:%u\n", g_coul_ocv_cali_info[i].cali_ocv_level);
    }

    /*report*/
    return dsm_report(err_num, dsm_buf);
}

static void check_coul_abnormal_rollback(struct smartstar_coul_device *di)
{
    static int last_cc = 0;
    int new_cc = 0;
    static int abn_count = 0;
    static bool report_enable = true;

    if (!di) {
        hwlog_info("%s di is null.\n", __func__);
        return;
    }

    if (!report_enable) {
        return;
    }

    if ( di->charging_state != CHARGING_STATE_CHARGE_START && di->charging_state != CHARGING_STATE_CHARGE_RECHARGE
            && di->charging_state != CHARGING_STATE_CHARGE_DONE) {
        new_cc = di->coul_dev_ops->calculate_cc_uah()/PERMILLAGE;
        if (last_cc && new_cc < last_cc) {
            if (abn_count++ < COUL_ABN_COUNT) {
                hwlog_info("coul abnormal rollback, count is %d, last_cc=%d curr_cc=%d\n", abn_count, last_cc, new_cc);
            } else {
                abn_count = 0;
                hwlog_info("coul abnormal rollback, report dsm!\n");
                if(!dsm_report(ERROR_COUL_ROLLBACK, "coul abnormal rollback")) {
                    report_enable = false;
                }
            }
        } else {
            abn_count = 0;
        }
        last_cc = new_cc;
    } else {
        last_cc = 0;
    }
}

/**********************************************************
*  Function:       coul_wake_lock
*  Description:   apply coul wake_lock
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void coul_wake_lock(void)
{
    if (!wake_lock_active(&coul_lock)) {
        wake_lock(&coul_lock);
        hwlog_info("coul core wake lock\n");
    }
}/*lint !e454 !e456*/
/**********************************************************
*  Function:       coul_wake_unlock
*  Description:   release coul wake_lock
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void coul_wake_unlock(void)
{
    if (wake_lock_active(&coul_lock)) {
        wake_unlock(&coul_lock);/*lint !e455*/
        hwlog_info("coul core wake unlock\n");
    }
}

int coul_get_low_temp_opt(void)
{
    return low_temp_opt_flag;
}

static void basp_fcc_learn_evt_handler(struct smartstar_coul_device *di, LEARN_EVENT_TYPE evt)
{
    static enum BASP_FCC_LEARN_STATE prev_state = LS_UNKNOWN;
    switch (evt)
    {
        case EVT_START:
            basp_fcc_ls = LS_INIT;
            break;
        case EVT_PER_CHECK:
            if (basp_fcc_ls == LS_INIT || basp_fcc_ls == LS_GOOD)
            {
                if (di->batt_temp > BASP_FCC_LERAN_TEMP_MIN && di->batt_temp < BASP_FCC_LERAN_TEMP_MAX)
                    basp_fcc_ls = LS_GOOD;
                else
                    basp_fcc_ls = LS_BAD;
            }
            break;
        case EVT_DONE:
            basp_fcc_ls = LS_UNKNOWN;
            break;
    }

    if (basp_fcc_ls != prev_state)
    {
        hwlog_info(BASP_TAG"prev_state:%d, new_state:%d, batt_temp:%d\n", prev_state, basp_fcc_ls, di->batt_temp);
        prev_state = basp_fcc_ls;
    }
}
int coul_get_battery_aging_safe_policy(AGING_SAFE_POLICY_TYPE *asp);
static int save_nv_info(struct smartstar_coul_device *di);
#ifdef SMARTSTAR_DEBUG
static int basp_nv;
static int basp_nv_set(const char *buffer, const struct kernel_param *kp)
{
#define MAX_TMP_BUF_LEN (10)
    const char *begin = NULL, *end = NULL, *c = NULL;
    BASP_LEVEL_TYPE indx = BASP_LEVEL_0;
    char num[MAX_TMP_BUF_LEN] = {0};
    long val = 0;
    int need_save = 0;
    struct ss_coul_nv_info *pinfo = NULL;

    struct smartstar_coul_device *di = (struct smartstar_coul_device *)g_smartstar_coul_dev;
    if (NULL == di)
    {
        hwlog_err(BASP_TAG"[%s], input param NULL!\n", __func__);
        return -EINVAL;
    }
    pinfo = &di->nv_info;

    hwlog_info(BASP_TAG"buffer:%s\n", buffer);
    c = buffer;
    while (*c != '\n' && *c != '\0')
    {
        if (!((('0' <= *c) && (*c <= '9')) || *c == ' '))
        {
            hwlog_err(BASP_TAG"[%s], input invalid!\n", __func__);
            goto FUNC_END;
        }
        c++;
    }

    begin = buffer;
    if (*begin == '\0')
    {
        hwlog_err(BASP_TAG"[%s], input empty!\n", __func__);
        return 0;
    }
    while (*begin != '\0' && *begin != '\n')
    {
        while (*begin == ' ')
            begin++;
        end = begin;
        while (*end != ' ' && *end != '\0' && *end != '\n')
            end++;
        if (end - begin >= MAX_TMP_BUF_LEN)
        {
            hwlog_err(BASP_TAG"[%s], input too big!\n", __func__);
           goto  FUNC_END;
        }
        memcpy(num, begin, (end-begin));
        if (strict_strtol(num, 10, &val) < 0)
        {
                hwlog_err(BASP_TAG"[%s], num:%s, convert fail!\n", __func__, num);
                break;
        }

        need_save = 1;
        pinfo->real_fcc_record[indx++] = val;
        if (indx == MAX_RECORDS_CNT)
            indx = 0;/*lint !e64*/
        memset(num, 0, sizeof(num));
        begin = end;
    }

    if (need_save)
        save_nv_info(di);

FUNC_END:
    return strlen(buffer);

#undef MAX_TMP_BUF_LEN
}

static int basp_nv_get(char *buffer, const struct kernel_param *kp) {
    unsigned int len = strlen(buffer);
    int i = 0;

    struct smartstar_coul_device *di = (struct smartstar_coul_device *)g_smartstar_coul_dev;
    if (NULL == di) {
        hwlog_err(BASP_TAG"[%s], input param NULL!\n", __func__);
        return -EINVAL;
    }

    for (i = 0; i < MAX_RECORDS_CNT; i++) {
        snprintf(buffer + len, PAGE_SIZE - len, "learned_fcc[%d]:%d\n", i, di->nv_info.real_fcc_record[i]);
        len = strlen(buffer);
    }
    snprintf(buffer + len, PAGE_SIZE - len, "latest_record_index:%d\n", di->nv_info.latest_record_index);
    return strlen(buffer);
}

static struct kernel_param_ops basp_nv_ops = {
    .set = basp_nv_set,
    .get = basp_nv_get,
};
module_param_cb(basp_nv, &basp_nv_ops, &basp_nv, 0644);
#endif

int coul_get_battery_current_ma(void);
int coul_battery_temperature_tenth_degree(BATTERY_TEMP_USER_TYPE user);

/**********************************************************
*  Function:       coul_ops_register
*  Discription:    register the handler ops for chargerIC
*  Parameters:   ops:operations interface of charge device
*  return value:  0-sucess or others-fail
**********************************************************/
int coul_core_ops_register(struct coul_device_ops *ops)
{
    int ret = 0;

    if(ops != NULL)
    {
        g_coul_core_ops = ops;
    }
    else
    {
        hwlog_err("coul ops register fail!\n");
        ret = -EPERM;
    }
    return ret;
}

/*******************************************************
  Function:        coul_cali_adc
  Description:    cali coul adc
  Input:      di;
  Output:           NULL
  Return:           the battery temperaturer_cali_cnt

********************************************************/
static void coul_cali_adc(struct smartstar_coul_device *di)
{
    if (NULL == di || NULL == di->coul_dev_ops->cali_adc
		|| NULL == di->coul_dev_ops->get_coul_time) {
		hwlog_err("coul ops is NULL!\n");
		return;
	}
	di->coul_dev_ops->cali_adc();
	last_cali_time = (int)di->coul_dev_ops->get_coul_time();
	last_cali_temp = coul_battery_temperature_tenth_degree(USER_COUL);
}

/*******************************************************
  Function:      coul_clear_sr_time_array
  Description:  clear sleep and wakeup global variable
  Input:           NULL
  Output:         NULL
  Return:         NULL
********************************************************/
static void coul_clear_sr_time_array(void)
{
    memset(&sr_time_sleep, 0, sizeof(sr_time_sleep));
    memset(&sr_time_wakeup, 0, sizeof(sr_time_wakeup));
    sr_index_sleep = 0;
    sr_index_wakeup = 0;
}
/*******************************************************
  Function:      clear_cc_register
  Description:  clear cc register
  Input:           NULL
  Output:         NULL
  Return:         NULL
********************************************************/
static void coul_clear_cc_register(void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;

    if (di) {
        di->iscd->full_update_cc += di->coul_dev_ops->calculate_cc_uah();
        di->coul_dev_ops->clear_cc_register();
    }
}

 /*******************************************************
  Function:      coul_clear_coul_time
  Description:  clear sleep/wakeup/cl_in/cl_out time
  Input:           NULL
  Output:         NULL
  Return:         NULL
********************************************************/
static void coul_clear_coul_time(void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;
    unsigned int time_now = 0;

    if (di) {
        time_now = di->coul_dev_ops->get_coul_time();
        if (SR_DEVICE_WAKEUP == sr_cur_state) {
            di->sr_resume_time -= time_now;
            if (di->sr_resume_time > 0) {
                hwlog_err("[SR]%s(%d): di->sr_resume_time = %d\n", __func__, __LINE__, di->sr_resume_time);
                di->sr_resume_time = 0;
            }
            di->sr_suspend_time = 0;
        }
        else {
            di->sr_resume_time = 0;
            di->sr_suspend_time = 0;
            hwlog_err("[SR]%s(%d): sr_cur_state = %d\n", __func__, __LINE__, sr_cur_state);
        }
        di->charging_stop_time -= time_now;
        di->coul_dev_ops->clear_coul_time();
    }
    else {
        hwlog_err("[SR]%s(%d): di is NULL\n", __func__, __LINE__);
    }

    return;
}




 /*******************************************************
  Function:       is_between
  Description:   whether value locates in zone [left, right] or [right, left]
  Input:           NULL
  Output:         NULL
  Return:         1: yes
                      0: no
********************************************************/
static int is_between(int left, int right, int value)
{
	if ((left >= right) && (left >= value) && (value >= right))
		return 1;
	if ((left <= right) && (left <= value) && (value <= right))
		return 1;

	return 0;
}

 /*******************************************************
  Function:       linear_interpolate
  Description:   get y according to : y = ax +b
  Input:           (x0,y0) (x1,y1) x
  Output:         NULL
  Return:         y conresponding x
  Remark:       a = (y1 - y0) / (x1 - x0)
********************************************************/
static int linear_interpolate(int y0, int x0, int y1, int x1, int x)
{
	if ((y0 == y1) || (x == x0))
		return y0;
	if ((x1 == x0) || (x == x1))
		return y1;

	return y0 + ((y1 - y0) * (x - x0) / (x1 - x0));
}

 /*******************************************************
  Function:       interpolate_single_lut
  Description:   get y according to : y = ax +b
  Input:           struct single_row_lut *lut   lookup table
  Output:         NULL
  Return:         y conresponding x
  Remark:       a = (y1 - y0) / (x1 - x0)
                      x0 <x  and x1 > x
********************************************************/
static int interpolate_single_lut(struct single_row_lut *lut, int x)
{
	int i, result;

	if (x < lut->x[0]) {
		return lut->y[0];
	}
	if (x > lut->x[lut->cols - 1]) {
		return lut->y[lut->cols - 1];
	}

	for (i = 0; i < lut->cols; i++)
		if (x <= lut->x[i])
			break;
	if (x == lut->x[i]) {
		result = lut->y[i];
	} else {
		result = linear_interpolate(
			lut->y[i - 1],
			lut->x[i - 1],
			lut->y[i],
			lut->x[i],
			x);
	}
	return result;
}

static int interpolate_single_y_lut(struct single_row_lut *lut, int y)
{
	int i, result;

	if (y < lut->y[0]) {
		return lut->x[0];
	}
	if (y > lut->y[lut->cols - 1]) {
		return lut->x[lut->cols - 1];
	}

	for (i = 0; i < lut->cols; i++)
		if (y <= lut->y[i])
			break;
	if (y == lut->y[i]) {
		result = lut->x[i];
	} else {
		result = linear_interpolate(
			lut->x[i - 1],
			lut->y[i - 1],
			lut->x[i],
			lut->y[i],
			y);
	}
	return result;
}

 /*******************************************************
  Function:       interpolate_scalingfactor
  Description:    get y according to : y = ax +b
  Input:          struct sf_lut *sf_lut ---- lookup table
                  row_entry             ---- battery temperature
                  pc                    ---- percent of uah
  Output:         NULL
  Return:         scalefactor of pc
********************************************************/
static int interpolate_scalingfactor(struct sf_lut *sf_lut,
				int row_entry, int pc)
{
	int i, scalefactorrow1, scalefactorrow2, scalefactor;
	int rows, cols;
	int row1 = 0;
	int row2 = 0;

	/*
	 * sf table could be null when no battery aging data is available, in
	 * that case return 100%
	 */
	if (!sf_lut)
		return 100;

	if ((sf_lut->rows < 1) || (sf_lut->cols < 1))
		return 100;

	rows = sf_lut->rows;
	cols = sf_lut->cols;
	if (pc > sf_lut->percent[0]) {
		//hwlog_info("pc %d greater than known pc ranges for sfd\n", pc);
		row1 = 0;
		row2 = 0;
	}
	if (pc < sf_lut->percent[rows - 1]) {
		//hwlog_info("pc %d less than known pc ranges for sf\n", pc);
		row1 = rows - 1;
		row2 = rows - 1;
	}
	for (i = 0; i < rows; i++) {
		if (pc == sf_lut->percent[i]) {
			row1 = i;
			row2 = i;
			break;
		}
		if (pc > sf_lut->percent[i]) {
			if (0 == i)
				row1 = i;
			else
				row1 = i - 1;
			row2 = i;
			break;
		}
	}

	if (row_entry < sf_lut->row_entries[0])
		row_entry = sf_lut->row_entries[0];
	if (row_entry > sf_lut->row_entries[cols - 1])
		row_entry = sf_lut->row_entries[cols - 1];

	for (i = 0; i < cols; i++)
		if (row_entry <= sf_lut->row_entries[i])
			break;
	if (row_entry == sf_lut->row_entries[i]) {
		scalefactor = linear_interpolate(
				sf_lut->sf[row1][i],
				sf_lut->percent[row1],
				sf_lut->sf[row2][i],
				sf_lut->percent[row2],
				pc);
		return scalefactor;
	}

	scalefactorrow1 = linear_interpolate(
				sf_lut->sf[row1][i - 1],
				sf_lut->row_entries[i - 1],
				sf_lut->sf[row1][i],
				sf_lut->row_entries[i],
				row_entry);

	scalefactorrow2 = linear_interpolate(
				sf_lut->sf[row2][i - 1],
				sf_lut->row_entries[i - 1],
				sf_lut->sf[row2][i],
				sf_lut->row_entries[i],
				row_entry);

	scalefactor = linear_interpolate(
				scalefactorrow1,
				sf_lut->percent[row1],
				scalefactorrow2,
				sf_lut->percent[row2],
				pc);

	return scalefactor;
}

/*******************************************************
  Function:       coul_get_battery_voltage_and_current
  Description:    get battery current in mA and voltage in mV
  Input:          struct smartstar_coul_device *di   ---- coul device
  Output:         int *ibat_ua              ----  battery current
                  int *vbat_uv              ----  battery voltage
  Return:         NULL
********************************************************/
static void coul_get_battery_voltage_and_current(struct smartstar_coul_device *di, int *ibat_ua, int *vbat_uv)
{
    int vol[3], cur[3];
    int i;
    if( NULL == di || NULL==ibat_ua || NULL==vbat_uv)
    {
        hwlog_info("NULL point in [%s]\n", __func__);
	    return;
    }
    for (i=0; i<3; i++){
        vol[i] = di->coul_dev_ops->get_battery_voltage_uv();
        cur[i] = di->coul_dev_ops->get_battery_current_ua();
    }

    if (vol[0]==vol[1] && cur[0]==cur[1]){
        *ibat_ua = cur[0];
        *vbat_uv = vol[0];
    } else if (vol[1]==vol[2] && cur[1]==cur[2]){
        *ibat_ua = cur[1];
        *vbat_uv = vol[1];
    } else {
        *ibat_ua = cur[2];
        *vbat_uv = vol[2];
    }

    *vbat_uv += (*ibat_ua/1000)*(di->r_pcb/1000);

}


/*******************************************************
  Function:       interpolate_fcc
  Description:    look for fcc value by battery temperature
  Input:
                  struct smartstar_coul_device *di   ---- coul device
                  int batt_temp                      ---- battery temperature
  Output:         NULL
  Return:         battery fcc
  Remark:         this function is called  before fcc self_study
********************************************************/
static int interpolate_fcc(struct smartstar_coul_device *di, int batt_temp)
{
       if( NULL == di )
       {
           hwlog_info("NULL point in [%s]\n", __func__);
	    return -1;
       }
	/* batt_temp is in tenths of degC - convert it to degC for lookups */
	batt_temp = batt_temp/10;
	return interpolate_single_lut(di->batt_data->fcc_temp_lut, batt_temp);
}

/*
* look for fcc scaling factory value by battery charge cycles
* used to adjust fcc before fcc self_study
*/
static int interpolate_scalingfactor_fcc(struct smartstar_coul_device *di,
								int cycles)
{
	/*
	 * sf table could be null when no battery aging data is available, in
	 * that case return 100%
	 */
	if( NULL == di )
       {
           hwlog_info("NULL point in [%s]\n", __func__);
	    return -1;
       }
	if (di->batt_data->fcc_sf_lut)
		return interpolate_single_lut(di->batt_data->fcc_sf_lut, cycles);
	else
		return 100;
}

/*******************************************************
  Function:       interpolate_fcc_adjusted
  Description:    look for fcc value by battery temperature
  Input:
                  struct smartstar_coul_device *di   ---- coul device
                  int batt_temp                      ---- battery temperature
  Output:         NULL
  Return:         looked up battery fcc
  Remark:         this function is called  after fcc self_study
********************************************************/
static int interpolate_fcc_adjusted(struct smartstar_coul_device *di, int batt_temp)
{
    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
        return -1;
    }
    /* batt_temp is in tenths of degC - convert it to degC for lookups */
    batt_temp = batt_temp/10;
    return interpolate_single_lut(di->adjusted_fcc_temp_lut, batt_temp);
}

/*******************************************************
  Function:       calculate_fcc_uah
  Description:    calculate fcc value by battery temperature and chargecycles
  Input:
                  struct smartstar_coul_device *di   ---- coul device
                  int batt_temp                      ---- battery temperature
                  int chargecycles                   ---- charge chcycles for scalefactor
  Output:         NULL
  Return:         battery fcc
  Remark:         lookup table is different, seperated by self_study
********************************************************/
static int calculate_fcc_uah(struct smartstar_coul_device *di, int batt_temp,
							int chargecycles)
{
    int initfcc, result, scalefactor = 0;
    if( NULL == di ){
        hwlog_info("NULL point in [%s]\n", __func__);
	    return -1;
       }
    if (di->adjusted_fcc_temp_lut == NULL) {
        initfcc = interpolate_fcc(di, batt_temp);
        scalefactor = interpolate_scalingfactor_fcc(di, chargecycles);

        /* Multiply the initial FCC value by the scale factor. */
        result = (initfcc * scalefactor * 1000) / 100;

    } else {
		return 1000 * interpolate_fcc_adjusted(di, batt_temp);
    }

    return result;
}

/*******************************************************
  Function:       interpolate_pc
  Description:    look for pc
  Input:
                  struct pc_temp_ocv_lut *lut   ---- lookup table
                  int batt_temp                 ---- battery temperature
  Output:         NULL
  Return:         percent of uah, may exceed PERMILLAGE*SOC_FULL
********************************************************/
static int interpolate_pc_high_precision(struct pc_temp_ocv_lut *lut,
				int batt_temp, int ocv)
{
    int i, j, pcj, pcj_minus_one, pc;
    int rows = 0;
    int cols = 0;

    /* batt_temp is in tenths of degC - convert it to degC for lookups */
    batt_temp = batt_temp/10;
    if( NULL == lut )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
	    return PERMILLAGE*SOC_FULL;
    }
	if ((lut->rows < 1) || (lut->cols < 1)) {
		hwlog_info("rows:%d, cols:%d are small in [%s]\n", lut->rows, lut->cols, __func__);
		return PERMILLAGE*SOC_FULL;
	}

    rows = lut->rows;
    cols = lut->cols;

    if (batt_temp < lut->temp[0]) {
    	hwlog_err("batt_temp %d < known temp range for pc\n", batt_temp);
    	batt_temp = lut->temp[0];
    }
    if (batt_temp > lut->temp[cols - 1]) {
    	hwlog_err("batt_temp %d > known temp range for pc\n", batt_temp);
    	batt_temp = lut->temp[cols - 1];
    }

    for (j = 0; j < cols; j++)
    	if (batt_temp <= lut->temp[j])
    		break;
    if (batt_temp == lut->temp[j]) {
    	/* found an exact match for temp in the table */
    if (ocv >= lut->ocv[0][j]) {
		pc = linear_interpolate(
			lut->percent[1]*PERMILLAGE,
			lut->ocv[1][j],
			lut->percent[0]*PERMILLAGE,
			lut->ocv[0][j],
			ocv);
		return pc;
	}

    	if (ocv <= lut->ocv[rows - 1][j])
		return lut->percent[rows - 1]*PERMILLAGE;
    	for (i = 0; i < rows; i++) {
		if (ocv >= lut->ocv[i][j]) {
    			if (ocv == lut->ocv[i][j])
				return lut->percent[i]*PERMILLAGE;
    			pc = linear_interpolate(
				lut->percent[i]*PERMILLAGE,
    				lut->ocv[i][j],
				lut->percent[i - 1]*PERMILLAGE,
    				lut->ocv[i - 1][j],
    				ocv);/*lint !e676*/
    			return pc;
    		}
    	}
    }

    /*
     * batt_temp is within temperature for
     * column j-1 and j
     */
    pcj_minus_one = 0;
    pcj = 0;
    if (ocv >= lut->ocv[0][j]) {
	pcj = linear_interpolate(
		lut->percent[0]*PERMILLAGE,
		lut->ocv[0][j],
		lut->percent[1]*PERMILLAGE,
		lut->ocv[1][j],
		ocv);
	pcj_minus_one = linear_interpolate(
		lut->percent[0]*PERMILLAGE,
		lut->ocv[0][j-1],
		lut->percent[1]*PERMILLAGE,
		lut->ocv[1][j-1],
		ocv);
	if (pcj && pcj_minus_one) {
		pc = linear_interpolate(
			pcj_minus_one,
			lut->temp[j-1],
			pcj,
			lut->temp[j],
			batt_temp);
		return pc;
	}
	if (pcj)
		return pcj;

	if (pcj_minus_one)
		return pcj_minus_one;
    }

    if (ocv <= lut->ocv[rows - 1][j - 1])
	return lut->percent[rows - 1]*PERMILLAGE;

    pcj_minus_one = 0;
    pcj = 0;
    for (i = 0; i < rows-1; i++) {
    	if (pcj == 0
    		&& is_between(lut->ocv[i][j],
    			lut->ocv[i+1][j], ocv)) {/*lint !e679*/
    		pcj = linear_interpolate(
			lut->percent[i]*PERMILLAGE,
    			lut->ocv[i][j],
			lut->percent[(int)(i+1)]*PERMILLAGE,/*lint !e679*/
    			lut->ocv[i+1][j],/*lint !e679*/
    			ocv);
    	}

    	if (pcj_minus_one == 0
    		&& is_between(lut->ocv[i][j-1],
    			lut->ocv[i+1][j-1], ocv)) {/*lint !e679*/

    		pcj_minus_one = linear_interpolate(
			lut->percent[i]*PERMILLAGE,
    			lut->ocv[i][j-1],
			lut->percent[(int)(i+1)]*PERMILLAGE,/*lint !e679*/
    			lut->ocv[i+1][j-1],/*lint !e679*/
    			ocv);
    	}

    	if (pcj && pcj_minus_one) {
    		pc = linear_interpolate(
    			pcj_minus_one,
    			lut->temp[j-1],
    			pcj,
    			lut->temp[j],
    			batt_temp);
    		return pc;
    	}
    }

    if (pcj)
    	return pcj;

    if (pcj_minus_one)
    	return pcj_minus_one;

    hwlog_err("%d ocv wasn't found for temp %d in the LUT returning 100%%\n",
                                            ocv, batt_temp);
    return PERMILLAGE*SOC_FULL;
}
static int interpolate_pc(struct pc_temp_ocv_lut *lut, int batt_temp, int ocv)
{
    int pc;

    if (!lut) {
        hwlog_err("%s input para is null.\n", __func__);
        return TENTH*SOC_FULL;
    }

    pc = interpolate_pc_high_precision(lut, batt_temp, ocv)/PERCENT;
    return pc < TENTH*SOC_FULL ? pc : TENTH*SOC_FULL;
}

/*******************************************************
  Function:       calculate_pc
  Description:    calculate and adjust pc
  Input:
                  int ocv_uv                         ---- voltage
                  int batt_temp                      ----battery temperature
                  struct smartstar_coul_device *di   ---- coul device
                  int chargecycles                   ---- charge cycles
  Output:         NULL
  Return:         percent of uah
********************************************************/
static int calculate_pc(struct smartstar_coul_device *di, int ocv_uv, int batt_temp,
							int chargecycles)
{
    int pc, scalefactor;
    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
	 return -1;
    }
    pc = interpolate_pc(di->batt_data->pc_temp_ocv_lut, batt_temp, ocv_uv / 1000);

    scalefactor = interpolate_scalingfactor(di->batt_data->pc_sf_lut, chargecycles, pc/10);

    pc = (pc * scalefactor) / 100;
    return pc;
}

/*******************************************************
  Function:       calculate_remaining_charge_uah
  Description:    calculate remaining uah
  Input:
                  struct smartstar_coul_device *di   ---- coul device
                  int fcc_uah                        ---- full  charge uah
                  int chargecycles                   ---- charge cycles
  Output:         NULL
  Return:         remaining uah
********************************************************/
static int calculate_remaining_charge_uah(struct smartstar_coul_device *di,
						int fcc_uah, int chargecycles)
{
    int  ocv, pc;
    int fcc_mah = fcc_uah/1000;
    int temp;
    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
	 return -1;
    }
    temp = di->batt_ocv_temp;

    ocv = di->batt_ocv;
    pc = calculate_pc(di, ocv, temp, chargecycles);

    return fcc_mah * pc;
}

/*******************************************************
  Function:       get_rbatt
  Description:    calculate battery resistence
  Input:
                  struct smartstar_coul_device *di   ---- coul device
                  int soc_rbatt                      ---- soc
                  int batt_temp                      ---- battery tempture
  Output:         NULL
  Return:         battery resistence
********************************************************/
static int get_rbatt(struct smartstar_coul_device *di, int soc_rbatt, int batt_temp)
{
	int rbatt, scalefactor;
    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
        return -1;
    }
	rbatt = di->batt_data->default_rbatt_mohm;
	if (di->batt_data->rbatt_sf_lut == NULL)  {
		return rbatt;
	}
	/* Convert the batt_temp to DegC from deciDegC */
	batt_temp = batt_temp / 10;
	scalefactor = interpolate_scalingfactor(di->batt_data->rbatt_sf_lut,
							batt_temp, soc_rbatt);
	rbatt = (rbatt * scalefactor) / 100;

	if (is_between(20, 10, soc_rbatt))
		rbatt = rbatt
			+ ((20 - soc_rbatt) * di->batt_data->delta_rbatt_mohm) / 10;
	else
		if (is_between(10, 0, soc_rbatt))
			rbatt = rbatt + di->batt_data->delta_rbatt_mohm;

	return rbatt;
}

/*******************************************************
  Function:       interpolate_ocv
  Description:    look for ocv according to temp, lookup table and pc
  Input:
                  struct pc_temp_ocv_lut *lut      ---- lookup table
                  int batt_temp_degc               ---- battery temperature
                  int pc                           ---- percent of uah
  Output:         NULL
  Return:         percent of uah
********************************************************/
static int interpolate_ocv(struct pc_temp_ocv_lut *lut,
				int batt_temp_degc, int pc)
{
	int i, ocvrow1, ocvrow2, ocv;
	int rows, cols;
	int row1 = 0;
	int row2 = 0;
       if( NULL == lut )
       {
           hwlog_info("NULL point in [%s]\n", __func__);
	    return -1;
       }
       if (pc > 1000){
           pc = 1000;
       }
       else if (pc < 0){
           pc = 0;
       }

	rows = lut->rows;
	cols = lut->cols;
	if (pc > lut->percent[0]*10) {
		//hwlog_info("pc %d greater than known pc ranges for sfd\n", pc);
		row1 = 0;
		row2 = 0;
	}
	if (pc < lut->percent[rows - 1]*10) {
		//hwlog_info("pc %d less than known pc ranges for sf\n", pc);
		row1 = rows - 1;
		row2 = rows - 1;
	}
	for (i = 0; i < rows; i++) {
		if (pc == lut->percent[i]*10) {
			row1 = i;
			row2 = i;
			break;
		}
		if (pc > lut->percent[i]*10) {
			row1 = i - 1;
			row2 = i;
			break;
		}
	}

	if (batt_temp_degc < lut->temp[0])
		batt_temp_degc = lut->temp[0];
	if (batt_temp_degc > lut->temp[cols - 1])
		batt_temp_degc = lut->temp[cols - 1];

	for (i = 0; i < cols; i++)
		if (batt_temp_degc <= lut->temp[i])
			break;
	if (batt_temp_degc == lut->temp[i]) {
		ocv = linear_interpolate(
				lut->ocv[row1][i],
				lut->percent[row1]*10,
				lut->ocv[row2][i],
				lut->percent[row2]*10,
				pc);
		return ocv;
	}

	ocvrow1 = linear_interpolate(
				lut->ocv[row1][i - 1],
				lut->temp[i - 1],
				lut->ocv[row1][i],
				lut->temp[i],
				batt_temp_degc);

	ocvrow2 = linear_interpolate(
				lut->ocv[row2][i - 1],
				lut->temp[i - 1],
				lut->ocv[row2][i],
				lut->temp[i],
				batt_temp_degc);

	ocv = linear_interpolate(
				ocvrow1,
				lut->percent[row1]*10,
				ocvrow2,
				lut->percent[row2]*10,
				pc);

	return ocv;
}

/*******************************************************
  Function:       calculate_termination_uuc
  Description:    calculate unuse uah
  Return:         unuse uah without adjust
********************************************************/
static int calculate_termination_uuc(struct smartstar_coul_device *di,
				 int batt_temp, int chargecycles,
				int fcc_uah, int i_ma,
				int *ret_pc_unusable)
{
	int unusable_uv, pc_unusable, uuc;
	int i = 0;
	int ocv_mv;
	int batt_temp_degc = batt_temp / 10;
	int rbatt_mohm;
	int delta_uv;
	int prev_delta_uv = 0;
	int prev_rbatt_mohm = 0;
	int uuc_rbatt_uv;
    int fcc_mah = fcc_uah / 1000;
    int zero_voltage = 3200;
    int ratio = 100;
    int temp_uuc_setp = 5;

    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
        return -1;
    }

#if RBATT_ADJ
    if (di->rbatt_ratio){
        ratio = di->rbatt_ratio;
        i_ma = di->last_fifo_iavg_ma;
        if (LOW_TEMP_OPT_OPEN == low_temp_opt_flag){
            if ((ratio > 0) && (ratio < 100) && (batt_temp_degc < 15))
                ratio = ratio_min;
            if (1 != di->soc_limit_flag) {
                if (batt_temp_degc < 15)
                     i_ma  = (i_ma > (uuc_current_min + (batt_temp_degc + 20)* temp_uuc_setp) ) ? i_ma : (uuc_current_min + (batt_temp_degc + 20)* temp_uuc_setp);
                 else
                    i_ma  = (i_ma > uuc_current_min ) ? i_ma : uuc_current_min;
            }
            hwlog_info("low_temp_opt:low_temp_opt old_ratio =%d, low_ratio =%d,old_i_ma = %d,low_i_ma = %d\n",di->rbatt_ratio,ratio,di->last_fifo_iavg_ma, i_ma);
        }
    }
#endif

    if((batt_temp_degc < 5) && (batt_temp_degc > -10))
    {
        zero_voltage =(((5 - batt_temp_degc)*(ZERO_VOLTAGE_MINUS_10 - ZERO_VOLTAGE_PLUS_5)
                                /15)+ZERO_VOLTAGE_PLUS_5);
    }
    else if(batt_temp_degc <= -10)
    {
        zero_voltage = ZERO_VOLTAGE_MINUS_10;
    }
    else
    {
        zero_voltage =  ZERO_VOLTAGE_PLUS_5;
    }
    hwlog_info("%s,batt_temp_degc = %d,zero_voltage = %d\n",__func__,batt_temp_degc,zero_voltage);

    for (i = 0; i <= 100; i++)
    {
        ocv_mv = interpolate_ocv(di->batt_data->pc_temp_ocv_lut, batt_temp_degc, i*10);
        rbatt_mohm = get_rbatt(di, i, batt_temp);
        rbatt_mohm = rbatt_mohm*ratio/100;
        unusable_uv = (rbatt_mohm * i_ma) + (zero_voltage * 1000);
        delta_uv = ocv_mv * 1000 - unusable_uv;

		if (delta_uv > 0)
			break;

        prev_delta_uv = delta_uv;
        prev_rbatt_mohm = rbatt_mohm;
    }

	uuc_rbatt_uv = linear_interpolate(rbatt_mohm, delta_uv,
					prev_rbatt_mohm, prev_delta_uv,
					0);

    unusable_uv = (uuc_rbatt_uv * i_ma) + (zero_voltage * 1000);

	pc_unusable = calculate_pc(di, unusable_uv, batt_temp, chargecycles);
        uuc =  fcc_mah * pc_unusable;
	*ret_pc_unusable = pc_unusable;
	return uuc;
}

/*******************************************************
  Function:       adjust_uuc
  Description:    adjust unuse uah, changes no more than 2%
  Return:         adjusted uuc
********************************************************/

static int adjust_uuc(struct smartstar_coul_device *di, int fcc_uah,
			int new_pc_unusable,
			int new_uuc,
			int batt_temp,
			int rbatt,
			int *iavg_ma)
{
    int fcc_mah = fcc_uah / 1000;
    int uuc_pc_step_add    = 20;
    int uuc_pc_step_sub    = 20;
    int uuc_pc_max_diff    = 50;

    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
        return -1;
    }
    if ((LOW_TEMP_OPT_OPEN == low_temp_opt_flag) && (1 != di->soc_limit_flag)) /*start or wake allow jump*/
        uuc_pc_max_diff = 700;
	if (di->prev_pc_unusable == -EINVAL
		|| abs(di->prev_pc_unusable - new_pc_unusable) <= uuc_pc_max_diff) {
		di->prev_pc_unusable = new_pc_unusable;
		return new_uuc;
	}
    if (LOW_TEMP_OPT_OPEN == low_temp_opt_flag){
        uuc_pc_step_add = 40;
        uuc_pc_step_sub = 5;
    }

	/* the uuc is trying to change more than 2% restrict it */
	if (new_pc_unusable > di->prev_pc_unusable)
		di->prev_pc_unusable += uuc_pc_step_add;
	else
		di->prev_pc_unusable -= uuc_pc_step_sub;

	new_uuc = fcc_mah * di->prev_pc_unusable;

	return new_uuc;
}


/*******************************************************
  Function:       calculate_unusable_charge_uah
  Description:    calculate unuse uah
  Return:         unuse uah without adjust and changes less than 2%
********************************************************/
static int calculate_unusable_charge_uah(struct smartstar_coul_device *di,
				int rbatt, int fcc_uah, int cc_uah,
				 int batt_temp, int chargecycles,
				int iavg_ua)
{
	int uuc_uah_iavg;
	int i;
	int iavg_ma = iavg_ua / 1000;
	static int iavg_samples[IAVG_SAMPLES] = {0};
	static int iavg_index = 0;
	static int iavg_num_samples = 0;
	int pc_unusable;

	/*
	 * if we are charging use a nominal avg current so that we keep
	 * a reasonable UUC while charging
	 */
	if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
        return -1;
    }
	if (iavg_ma < 0)
		iavg_ma = CHARGING_IAVG_MA;
	iavg_samples[iavg_index] = iavg_ma;
	iavg_index = (iavg_index + 1) % IAVG_SAMPLES;
	iavg_num_samples++;
	if (iavg_num_samples >= IAVG_SAMPLES)
		iavg_num_samples = IAVG_SAMPLES;

	/* now that this sample is added calcualte the average */
	iavg_ma = 0;
	if (iavg_num_samples != 0) {
		for (i = 0; i < iavg_num_samples; i++) {
			iavg_ma += iavg_samples[i];
		}

		iavg_ma = DIV_ROUND_CLOSEST(iavg_ma, iavg_num_samples);
	}

	uuc_uah_iavg = calculate_termination_uuc(di,
					batt_temp, chargecycles,
					fcc_uah, iavg_ma,
					&pc_unusable);

    hwlog_info("RBATT_ADJ:UUC =%d uAh, pc=%d.%d\n",
        uuc_uah_iavg, pc_unusable/10, pc_unusable%10);

    di->rbatt_ratio = 0;

	/* restrict the uuc such that it can increase only by one percent */
	uuc_uah_iavg = adjust_uuc(di, fcc_uah, pc_unusable, uuc_uah_iavg,
					batt_temp, rbatt, &iavg_ma);

    uuc_uah_iavg += fcc_uah/1000*10;

	di->batt_uuc = uuc_uah_iavg;

	return uuc_uah_iavg;
}

 /*******************************************************
  Function:       recalc_chargecycles
  Description:    recalculate the chargecycle after charging done
  Input:
                  struct smartstar_coul_device *di   ---- coul device
  Output:         NULL
  Return:         new chargecycles
********************************************************/
static unsigned int recalc_chargecycles(struct smartstar_coul_device *di)
{
    int cc_end, real_fcc, fcc, temp, pc, new_chargecycles;
    unsigned int retval = 0;
    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
        return -1;/*lint !e570*/
    }
    if (di->batt_soc==100 && di->charging_begin_soc/10<MIN_BEGIN_PERCENT_FOR_LEARNING) {
        cc_end = di->coul_dev_ops->calculate_cc_uah();
        temp = di->batt_temp;
        real_fcc = (cc_end - di->charging_begin_cc)*1000/(1000 - di->charging_begin_soc);
        fcc = interpolate_fcc(di,temp);
        pc = real_fcc *100 / fcc;
        new_chargecycles = interpolate_single_y_lut(di->batt_data->fcc_sf_lut, pc);
        new_chargecycles -= 40*100;
        retval = (unsigned int)(new_chargecycles>0?new_chargecycles:0);

        hwlog_info("trigger battery charge cycle reclac, val = %d!\n", new_chargecycles);
    }

    return retval;
}

static int __init early_parse_pmu_nv_addr_cmdline(char *p)
{
    char buf[PMU_NV_ADDR_CMDLINE_MAX_LEN + 1] = {0};

    char *endptr = NULL;
    int len;
    if( NULL == p )
    {
        hwlog_err("NULL point in [%s]\n", __func__);
        return -1;
    }
    len = strlen(p);
    if(len > PMU_NV_ADDR_CMDLINE_MAX_LEN)
    {
        hwlog_err("pmu_nv_addr_cmdline length out of range\n");
        return -1;
    }
    memcpy(buf, p, len+1);

    nv_info_addr = simple_strtoul(buf, &endptr, 16);

    return 0;
}


early_param("pmu_nv_addr", early_parse_pmu_nv_addr_cmdline);

static int __init move_pmu_nv_info(void)
{
    struct ss_coul_nv_info *pmu_nv_addr;

    if (nv_info_addr == 0){
        return 0;
    }
   // #ifdef CONFIG_ARM64
    pmu_nv_addr = (struct ss_coul_nv_info*)ioremap_wc(nv_info_addr, sizeof (struct ss_coul_nv_info));
    //pmu_nv_addr = (struct ss_coul_nv_info*)phys_to_virt(nv_info_addr);

    hwlog_info("pmu_nv_addr=0x%pK\n", pmu_nv_addr);
	if (NULL == pmu_nv_addr)
	{
		hwlog_err("nv add err,pmu_nv_addr=0x%pK\n", pmu_nv_addr);
		return 0;
	}
    memcpy(&my_nv_info, pmu_nv_addr, sizeof(my_nv_info));

    //here transfer the old short value to new int value to avoid check_sum will overflow
    if(my_nv_info.fcc_check_sum!=0){
	hwlog_info("cp fcc_check_sum %d to fcc_check_sum_ext, and clear it\n",my_nv_info.fcc_check_sum);
	my_nv_info.fcc_check_sum_ext =my_nv_info.fcc_check_sum;
	my_nv_info.fcc_check_sum =0;
    }else{
	hwlog_info("fcc_check_sum =0,do nothing\n");
    }
    p_charger = (void*)pmu_nv_addr;
    return 0;
}



arch_initcall(move_pmu_nv_info);

char* get_charger_info_p(void)
{
   return p_charger;
}
EXPORT_SYMBOL(get_charger_info_p);
 /*******************************************************
  Function:       get_initial_params
  Description:    get NV info from fastboot send
  Input:
                  struct smartstar_coul_device *di   ---- coul device
  Output:         NULL
  Return:         0
********************************************************/
static int get_initial_params(struct smartstar_coul_device *di)
{
    int i;
    struct ss_coul_nv_info *pinfo = &di->nv_info;

    struct single_row_lut *preal_fcc_lut = &di->adjusted_fcc_temp_lut_tbl1;
    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
   	    return -1;
    }
    memcpy(&di->nv_info, &my_nv_info, sizeof(my_nv_info));

    di->batt_chargecycles = pinfo->charge_cycles;
    di->batt_limit_fcc    = pinfo->limit_fcc;
    v_offset_a = pinfo->v_offset_a==0?DEFAULT_V_OFF_A:pinfo->v_offset_a;
    v_offset_b = pinfo->v_offset_b==0?DEFAULT_V_OFF_B:pinfo->v_offset_b;

    hwlog_info("pl_v_a=%d,pl_v_b=%d,pl_c_a=%d,pl_c_b=%d,cycles=%d,limit_fcc=%d\n"
                "reg_c=%d, reg_v=%d,batt_id=%d\n",
        pinfo->v_offset_a, pinfo->v_offset_b, pinfo->c_offset_a, pinfo->c_offset_b,
        pinfo->charge_cycles,pinfo->limit_fcc, pinfo->calc_ocv_reg_c, pinfo->calc_ocv_reg_v,
        pinfo->hkadc_batt_id_voltage);

    hwlog_info("real use a/b value, v_offset_a=%d,v_offset_b=%d\n",v_offset_a,v_offset_b);
    for (i=0; i<MAX_TEMPS; i++)
    {
        if (pinfo->real_fcc[i] == 0){
            break;
        }

        if (pinfo->real_fcc[i] < 100)
        {
            hwlog_info("real fcc in nv is not currect!\n");
            return 0;
        }

        preal_fcc_lut->x[i] = pinfo->temp[i];
        preal_fcc_lut->y[i] = pinfo->real_fcc[i];
    }

    if (i == 0){
        hwlog_info("no real fcc data in nv\n");
        return 0;
    }

    preal_fcc_lut->cols = i;

    di->adjusted_fcc_temp_lut = preal_fcc_lut;

    hwlog_info("temp:real_fcc %d:%d %d:%d %d:%d %d:%d %d:%d %d:%d %d:%d\n"
        ,pinfo->temp[0], pinfo->real_fcc[0]
        ,pinfo->temp[1], pinfo->real_fcc[1]
        ,pinfo->temp[2], pinfo->real_fcc[2]
        ,pinfo->temp[3], pinfo->real_fcc[3]
        ,pinfo->temp[4], pinfo->real_fcc[4]
        ,pinfo->temp[5], pinfo->real_fcc[5]
        ,pinfo->temp[6], pinfo->real_fcc[6]
        );
    return 0;
}
 /*******************************************************
  Function:       save_nv_info
  Description:    save info to NV
  Input:
                  struct smartstar_coul_device *di   ---- coul device
  Output:         NULL
  Return:         -1: failed     0:success
********************************************************/
static int save_nv_info(struct smartstar_coul_device *di)
{
    int ret, i;
    int refresh_fcc_success = 1;
    struct hisi_nve_info_user nve;
    struct ss_coul_nv_info *pinfo = &di->nv_info;
    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
   	    return -1;
    }
    if (!di->is_nv_read){
		/* udp do not print err log */
		if (BAT_BOARD_ASIC == is_board_type)
			hwlog_err("save nv before read, error\n");
        di->coul_dev_ops->set_nv_save_flag(NV_SAVE_FAIL);
        return -1;
    }

    memset(&nve, 0, sizeof(nve));
    strncpy(nve.nv_name, HISI_COUL_NV_NAME, sizeof(HISI_COUL_NV_NAME));
    nve.nv_number = HISI_COUL_NV_NUM;
    nve.valid_size = sizeof(*pinfo);
    nve.nv_operation = NV_WRITE;

    pinfo->charge_cycles = di->batt_chargecycles;

    pinfo->limit_fcc = di->batt_limit_fcc;

    if (di->adjusted_fcc_temp_lut){
        for(i=0; i<di->adjusted_fcc_temp_lut->cols; i++)
        {
            if(di->adjusted_fcc_temp_lut->y[i] < 100)
            {
                refresh_fcc_success = 0;
                break;
            }
        }
        if(refresh_fcc_success){
            for(i=0; i<di->adjusted_fcc_temp_lut->cols; i++){
                pinfo->temp[i] = di->adjusted_fcc_temp_lut->x[i];
                pinfo->real_fcc[i] = di->adjusted_fcc_temp_lut->y[i];
            }
        }
    }
    else{
        for(i=0; i<MAX_TEMPS; i++){
            pinfo->temp[i] = 0;
            pinfo->real_fcc[i] = 0;
        }
    }

    memcpy(nve.nv_data, pinfo, sizeof(*pinfo));

    /* here save info in register */
    ret = hisi_nve_direct_access(&nve);
    if (ret) {
        hwlog_info("save nv partion failed, ret=%d\n", ret);
        di->coul_dev_ops->set_nv_save_flag(NV_SAVE_FAIL);
    }
    else
    {
        di->coul_dev_ops->set_nv_save_flag(NV_SAVE_SUCCESS);
    }

    return ret;
}
static int basp_full_pc_by_voltage(struct smartstar_coul_device *di);

static void battery_para_verification(struct smartstar_coul_device *di, struct pc_temp_ocv_lut *current_pc_temp_ocv_lut,int basp_vol)
{
	int ocv1 = 0, ocv2 = 0;
	ocv1 = interpolate_ocv(di->batt_data->pc_temp_ocv_lut0, DEFAULT_TEMP, PERMILLAGE);/*new battery ocv */
	ocv2 = interpolate_ocv(current_pc_temp_ocv_lut, DEFAULT_TEMP, PERMILLAGE);/*ocv-pc data verification*/
	if(ocv1 - ocv2 <= basp_vol)
	{
		di->batt_data->pc_temp_ocv_lut = current_pc_temp_ocv_lut;
	}
	hwlog_info("battery para changed ocv1 =%d,ocv2 =%d,ocv1-ocv2 =%d\n",ocv1, ocv2, ocv1-ocv2);
}

static int battery_para_changed(struct smartstar_coul_device *di, int flag)
{
	AGING_SAFE_POLICY_TYPE asp;
	int ret = 0, new_ocv = 0;
	if(NULL == di || NULL == di->batt_data)
	{
		hwlog_err("di or batt_data is NULL!\n");
		return -1;
	}

	if(flag || batt_init_level != di->basp_level)/*battery age level charged*/
	{
		ret = coul_get_battery_aging_safe_policy(&asp);
		if(ret < 0 || !asp.nondc_volt_dec)/*no limit vol can not changed*/
		{
			hwlog_err("get battery age fail ,vol dec= %d\n",asp.nondc_volt_dec);
			return -1;
		}
		if(di->batt_data->vol_dec1 > 0 && asp.nondc_volt_dec == di->batt_data->vol_dec1
			&& di->batt_data->pc_temp_ocv_lut1)
		{
			battery_para_verification(di, di->batt_data->pc_temp_ocv_lut1, asp.nondc_volt_dec);
		}else if(di->batt_data->vol_dec2 > 0 && asp.nondc_volt_dec == di->batt_data->vol_dec2
			&& di->batt_data->pc_temp_ocv_lut2)
		{
			battery_para_verification(di, di->batt_data->pc_temp_ocv_lut2, asp.nondc_volt_dec);
		}else if(di->batt_data->vol_dec3 > 0 && asp.nondc_volt_dec == di->batt_data->vol_dec3
			&& di->batt_data->pc_temp_ocv_lut3)
		{
			battery_para_verification(di, di->batt_data->pc_temp_ocv_lut3, asp.nondc_volt_dec);
		}else
		{
			hwlog_info("battery age para no changed\n");
		}
		new_ocv = interpolate_ocv(di->batt_data->pc_temp_ocv_lut, DEFAULT_TEMP, PERMILLAGE);
		hwlog_info("battery age level charged asp.nondc_volt_dec =%d,new table max vol=%d\n",asp.nondc_volt_dec,new_ocv);
	}
	return 0;
}
 /*******************************************************
  Function:       update_chargecycles
  Description:    update charge/discharge times
  Input:
                  struct smartstar_coul_device *di   ---- coul device
  Output:         NULL
  Return:         -1: failed     0:success
********************************************************/
static void update_chargecycles(struct smartstar_coul_device *di)
{
    if( 0 == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
   	 return;
    }
    if (di->batt_soc_real/10 - di->charging_begin_soc/10 > 0) {
        di->batt_chargecycles += di->batt_soc_real/10 - di->charging_begin_soc/10;
        hwlog_info("new chargecycle=%d, added=%d\n", di->batt_chargecycles, di->batt_soc_real/10 - di->charging_begin_soc/10);
        di->basp_level = get_basp_level(di);
    }
    else{
        hwlog_info("chargecycle not updated, soc_begin=%d, soc_current=%d, batt_soc=%d\n", di->charging_begin_soc/10, di->batt_soc_real/10,di->batt_soc);
    }
	if(battery_para_changed(di,CHANGE_LOAD) < 0)
	{
		hwlog_err("battery charge para fail!\n");
	}

    g_basp_full_pc = basp_full_pc_by_voltage(di);
	di->charging_begin_soc = 1000;
}

/*******************************************************
  Function:       coul_is_ready
  Description:    check wheather coul is ready
  Input:
                  NULL
  Output:         NULL
  Return:         1: OK     0:no ready
********************************************************/
int coul_is_ready(void)
{
    if (g_pdev)
        return 1;
    else
        return 0;
}

/**********************************************************
*  Function:       adc_to_temp
*  Discription:    convert adc sampling voltage to battery temperature
*  Parameters:
*                   voltage: adc sampling voltage
*  return value:
*                   battery temperature
**********************************************************/
static int adc_to_temp(int temp_volt)
{
    int temprature = 0;
    int i = 0;

    if(temp_volt >= T_V_TABLE[0][1])
    {
        return T_V_TABLE[0][0];
    }
    else if(temp_volt <= T_V_TABLE[T_V_ARRAY_LENGTH-1][1])
    {
        return T_V_TABLE[T_V_ARRAY_LENGTH-1][0];
    }
    else
    {
        for(i = 0; i < T_V_ARRAY_LENGTH; i++)
        {
            if(temp_volt == T_V_TABLE[i][1])
                return T_V_TABLE[i][0];
            if(temp_volt > T_V_TABLE[i][1])
            {
                break;
            }
        }
        if(0 == i)
        {
            return T_V_TABLE[0][0];
        }
        temprature = T_V_TABLE[i-1][0] + (temp_volt - T_V_TABLE[i-1][1])*(T_V_TABLE[i][0] - T_V_TABLE[i-1][0])/(T_V_TABLE[i][1] - T_V_TABLE[i-1][1]);
    }
    return temprature;
}
/*******************************************************
  Function:       coul_battery_temperature_tenth_degree
  Description:    get battery 10*temperature
  Input:		BATTERY_TEMP_USER:0-coul;1-charger
  Output:         NULL
  Return:         the battery temperature in centigrade.
********************************************************/
int coul_battery_temperature_tenth_degree(BATTERY_TEMP_USER_TYPE user)
{
    int retry_times = 3;
    int cnt = 0;
    int T_adc;
    int temperature;

    /*default is no battery in sft and udp, so here temp is fixed 25c to prevent low power reset*/
    if ( BAT_BOARD_ASIC != is_board_type) {
        return DEFAULT_TEMP*10;
    }
    while(cnt++ < retry_times)
    {
        T_adc = hisi_adc_get_adc(adc_batt_temp);

        if(T_adc < 0)
        {
            hwlog_err("Bat temp read fail!,retry_cnt = %d\n",cnt);
        }
        else
        {
            temperature = adc_to_temp(T_adc);
            return 10*temperature;
        }
    }

    hwlog_err("Bat temp read retry 3 times,error!\n");
	if (USER_CHARGER == user)
		return TEMP_IPC_GET_ABNORMAL*10;
	else
		return DEFAULT_TEMP*10;
}

/*******************************************************
  Function:        coul_ntc_temperature_compensation
  Description:    temperature compensation
  Input:      di;BATTERY_TEMP_USER:0-coul;1-charger
  Output:           NULL
  Return:           the battery temperature.
********************************************************/
int coul_ntc_temperature_compensation(struct smartstar_coul_device *di, BATTERY_TEMP_USER_TYPE user)
{
    int temp_without_compensation = 0;
    int temp_with_compensation = 0;
    int ichg = 0;
    int i = 0;

    /*modify the temperature obtained by sampling, according to the temperature compensation value
      corresponding to the different current */
    temp_without_compensation = coul_battery_temperature_tenth_degree(user);
    temp_with_compensation = temp_without_compensation;
    if (1 == di->ntc_compensation_is){
        ichg = coul_get_battery_current_ma();
        ichg = abs(ichg);
        for(i = 0;i < COMPENSATION_PARA_LEVEL;i++)
        {
            if (ichg >= di->ntc_temp_compensation_para[i].ntc_compensation_ichg)
            {
                temp_with_compensation = temp_without_compensation - di->ntc_temp_compensation_para[i].ntc_compensation_value;
                break;
            }
        }
    }
    hwlog_info("coul_ntc_temperature_compensation: current = %d, temp_without_compensation = %d, temp_with_compensation = %d\n",
                ichg, temp_without_compensation, temp_with_compensation );
    return temp_with_compensation;
}
/**********************************************************
*  Function:    get_temperature_stably
*  Discription:    the fun for adc get some err,we can avoid
*  Parameters:
*               di;BATTERY_TEMP_USER:0-coul;1-charger
*  return value:
*               battery temperature
**********************************************************/
static int get_temperature_stably(struct smartstar_coul_device *di, BATTERY_TEMP_USER_TYPE user)
{
	int retry_times = 5;
	int cnt = 0;
	int temperature;
	int delta = 0;

	if (NULL == di){
		hwlog_err("error, di is NULL, return default temp\n");
		return DEFAULT_TEMP*10;
	}

	while(cnt++ < retry_times)
	{
		temperature = coul_ntc_temperature_compensation(di, user);
		delta = abs(di->batt_temp - temperature);
		if(DELTA_TEMP < delta
			||ABNORMAL_BATT_TEMPERATURE_POWEROFF < temperature
			|| LOW_BATT_TEMP_CHECK_THRESHOLD >= temperature){
			continue;
		}
		hwlog_info("stably temp!,old_temp =%d,cnt =%d, temp = %d\n",di->batt_temp,cnt,temperature);
		return temperature;
	}
	return temperature;
}

static void update_battery_temperature(struct smartstar_coul_device *di, int status)
{
    int temp = get_temperature_stably(di, USER_COUL);
    if (TEMPERATURE_INIT_STATUS == status)
    {
        hwlog_info("init temp = %d\n", temp);
        di->batt_temp = temp;
    }
    else
    {
        if (temp - di->batt_temp > TEMPERATURE_CHANGE_LIMIT)
        {
            hwlog_err("temperature change too fast, pre = %d, current = %d\n", di->batt_temp, temp);
            di->batt_temp = di->batt_temp + TEMPERATURE_CHANGE_LIMIT;
        }
        else if (di->batt_temp - temp > TEMPERATURE_CHANGE_LIMIT)
        {
            hwlog_err("temperature change too fast, pre = %d, current = %d\n", di->batt_temp, temp);
            di->batt_temp = di->batt_temp - TEMPERATURE_CHANGE_LIMIT;
        }
        else if(di->batt_temp != temp)
        {
            hwlog_info("temperature changed, pre = %d, current = %d\n", di->batt_temp, temp);
            di->batt_temp = temp;
        }
    }
}

  /*******************************************************
  Function:        coul_get_battery_temperature
  Description:     return the battery temperature in centigrade.
  Input:           NULL
  Output:          NULL
  Return:          battery temperature in centigrade.
********************************************************/
int coul_get_battery_temperature(void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;
    if (NULL == di)
    {
        hwlog_err("error, di is NULL, return default temp\n");
        return DEFAULT_TEMP;
    }
    return (di->batt_temp / 10);
}

/*******************************************************
  Function:        coul_get_battery_temperature_for_charger
  Description:     return the battery temperature in centigrade.
  Input:           NULL
  Output:          NULL
  Return:          battery temperature in centigrade for charger.
********************************************************/
int coul_get_battery_temperature_for_charger(void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;
	int temp;

    if (NULL == di)
    {
        hwlog_err("error, di is NULL, return default temp for charger\n");
        return DEFAULT_TEMP;
    }
	temp = get_temperature_stably(di, USER_CHARGER);
	return (temp / 10);
}

/*******************************************************
  Function:        coul_is_battery_exist
  Description:     check whether battery exist
  Input:           NULL
  Output:          NULL
  Return:          0:battery isn't exist, 1: exist
********************************************************/
int coul_is_battery_exist(void)
{
    int temp;
    struct smartstar_coul_device *di = g_smartstar_coul_dev;

#ifdef CONFIG_HLTHERM_RUNTEST
    di->batt_exist = 0;
    return 0;
#endif
    if (NULL == di)
    {
        hwlog_err("error, di is NULL, return default exist\n");
        return 1;
    }
    temp = coul_get_battery_temperature();

    if ((temp <= ABNORMAL_BATT_TEMPERATURE_LOW)
        || (temp >= ABNORMAL_BATT_TEMPERATURE_HIGH)) {
        di->batt_exist = 0;
    } else {
        di->batt_exist = 1;
    }
    return di->batt_exist;
}

/*******************************************************
  Function:        coul_is_battery_reach_threshold
  Description:     check whether battery uah reach threshold
  Input:            NULL
  Output:          NULL
  Return:         0:not, 4: lower than warning_lev, 8: lower than Low_Lev
********************************************************/
int coul_is_battery_reach_threshold(void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;
    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
   	    return -1;
    }
    if (!coul_is_battery_exist()){
        return 0;
    }

    if (di->batt_soc > BATTERY_CC_WARNING_LEV)
        return 0;
    else if (di->batt_soc > BATTERY_CC_LOW_LEV)
        return BQ_FLAG_SOC1;
    else
        return BQ_FLAG_LOCK;
}

 /*******************************************************
  Function:        coul_get_battery_voltage_mv
  Description:     get battery voltage in mV
  Input:           NULL
  Output:          NULL
  Return:         battery voltage in mV
********************************************************/
int coul_get_battery_voltage_mv(void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;
    int ibat_ua = 0, vbat_uv = 0;
    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
        return -1;
    }

    coul_get_battery_voltage_and_current(di, &ibat_ua, &vbat_uv);
    return vbat_uv/1000;
}
 /*******************************************************
  Function:        coul_get_battery_voltage_uv
  Description:     get battery voltage in uV
  Input:           NULL
  Output:          NULL
  Return:         battery voltage in uV
********************************************************/
int coul_get_battery_voltage_uv(void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;
    int ibat_ua = 0, vbat_uv = 0;
    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
        return -1;
    }

    coul_get_battery_voltage_and_current(di, &ibat_ua, &vbat_uv);
    return vbat_uv;
}
 /*******************************************************
  Function:        coul_get_battery_current_ma
  Description:     get battery current in mA
  Input:           NULL
  Output:          NULL
  Return:          current current in mA
********************************************************/
int coul_get_battery_current_ma(void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;
    int cur =  di->coul_dev_ops->get_battery_current_ua();
    return (cur / 1000);
}

/*******************************************************
  Function:        coul_battery_brand
  Description:     get battery brand in string.
  Input:           NULL
  Output:          NULL
  Return:          battery band string
********************************************************/
char* coul_get_battery_brand(void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;
    return di->batt_data->batt_brand;
}

/*******************************************************
  Function:        coul_get_battery_current_avg_ma
  Description:     get battery avg current_ma.
  Input:           NULL
  Output:          NULL
  Return:          the battery avg_current in mA
********************************************************/
int coul_get_battery_current_avg_ma(void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;
    return di->last_iavg_ma;
}

 /*******************************************************
  Function:        coul_battery_unfiltered_capacity
  Description:     get the unfilter capacity of battery
  Input:           NULL
  Output:          NULL
  Return:          the unfilter capacity of battery
********************************************************/
int coul_battery_unfiltered_capacity(void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;

    if (!coul_is_battery_exist()){
        return 0;
    }

    return di->batt_soc_real/10;
}

 /*******************************************************
  Function:            coul_get_soc_vary_flag
  Description:        during wake-up, monitor the soc variety
  Input:                monitor flag: 0:monitoring in one period
                                              1:one period done
  Output:              deta_soc: variety of soc
  Return:              data valid:0: data is valid( soc err happened)
                                         others: data is invalid
********************************************************/
static int coul_get_soc_vary_flag(int monitor_flag, int *deta_soc)
{
    static int last_record_soc = 0;
    static int current_record_soc = 0;
    static int soc_changed = 0;
    static bool temp_stablity = TRUE;
    int soc_changed_abs = 0;
    int ret = -1;
    struct smartstar_coul_device *di = g_smartstar_coul_dev;

    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
        return -1;
    }

    if (!coul_is_battery_exist()){
        hwlog_info("battery not exist!\n");
        return -1;
    }

    /*Start up or resume, refresh record soc and return invalid data*/
    if(di->soc_monitor_flag != 1){
        last_record_soc = di->soc_unlimited;
        current_record_soc = di->soc_unlimited;
        di->soc_monitor_flag = 1;
        return -1;
    }

    if( di->batt_temp > DEFAULT_SOC_MONITOR_TEMP_MIN
        && di->batt_temp < DEFAULT_SOC_MONITOR_TEMP_MAX ){
        temp_stablity = TRUE;
    }else{
        temp_stablity = FALSE;
    }

    if(monitor_flag == 1){
        current_record_soc = di->soc_unlimited;
        soc_changed = current_record_soc - last_record_soc;
        if( soc_changed < 0 ){
            soc_changed_abs = -soc_changed;
        }else{
            soc_changed_abs = soc_changed;
        }
        last_record_soc = current_record_soc;
        /*if needed, report soc error*/
        if( (soc_changed_abs >= di->soc_monitor_limit)
            && (last_record_soc > 10 && last_record_soc < 90)
            && (TRUE == temp_stablity) ){
            *deta_soc = soc_changed;
            hwlog_err("soc vary fast! soc_changed is %d\n", soc_changed);
            ret = 0;
        }else{
            ret = -1;
        }
    }else{
        if( TRUE == temp_stablity ){
            ret = 0;
        }else{
            ret = -1;
        }
    }

    return ret;
}

/*******************************************************
  Function:        coul_get_battery_capacity
  Description:     get the capacity of battery
  Input:            NULL
  Output:          NULL
  Return:          the capacity of battery
********************************************************/
int coul_get_battery_capacity(void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;

    if (battery_is_removable) {
         if (!coul_is_battery_exist()){
             return 0;
         }
   }

   if(hand_chg_capacity_flag == 1)
        return input_capacity;

    return di->batt_soc;
}

 /*******************************************************
  Function:        coul_get_battery_rm
  Description:     battery remaining uah with uuh
  Input:           NULL
  Output:          NULL
  Return:         remaining uah
********************************************************/
int coul_get_battery_rm(void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;

    if (!coul_is_battery_exist()){
        return 0;
    }

    return di->batt_ruc/1000;
}

 /*******************************************************
  Function:        coul_get_battery_fcc
  Description:     battery full charge capacity
  Input:           NULL
  Output:          NULL
  Return:          fcc
********************************************************/
int coul_get_battery_fcc (void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;

    if (!coul_is_battery_exist()){
        return 0;
    }

    return di->batt_fcc/1000;
}
 /*******************************************************
  Function:        coul_get_battery_uuc
  Description:     battery unused capacity mah
  Input:           NULL
  Output:          NULL
  Return:          uuc
********************************************************/
int coul_get_battery_uuc (void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;

    if (!coul_is_battery_exist()){
        return 0;
    }

    return di->batt_uuc/1000;
}

 /*******************************************************
  Function:        coul_get_battery_cc
  Description:     capacity recorded by coulomb
  Input:            NULL
  Output:          NULL
  Return:          battery_cc
********************************************************/
int coul_get_battery_cc (void)
{
    int cc = 0;
    struct smartstar_coul_device *di = g_smartstar_coul_dev;

    if (!coul_is_battery_exist()){
        return 0;
    }
    cc = di->coul_dev_ops->calculate_cc_uah();
    return cc/1000;
}
 /*******************************************************
  Function:        coul_get_battery_delta_rc
  Description:     get battery delta cc
  Input:           NULL
  Output:          NULL
  Return:          delta rc(mah)
********************************************************/
int coul_get_battery_delta_rc (void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;

    if (!coul_is_battery_exist()){
        return 0;
    }

    return di->batt_delta_rc/1000;
}

 /*******************************************************
  Function:        coul_get_battery_ocv
  Description:     battery open circult voltage
  Input:           NULL
  Output:          NULL
  Return:          ocv in uV
********************************************************/
int coul_get_battery_ocv (void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;

    if (!coul_is_battery_exist()){
        return 0;
    }

    return di->batt_ocv;
}
 /*******************************************************
  Function:        coul_get_battery_resistance
  Description:     battery_resistance
  Input:           NULL
  Output:          NULL
  Return:          battery_resistance
********************************************************/
int coul_get_battery_resistance (void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;

    if (!coul_is_battery_exist()){
        return 0;
    }

    return di->rbatt;
}

/*******************************************************
  Function:        coul_get_battery_tte
  Description:     time to empty
  Input:           NULL
  Output:          NULL
  Return:          value in min or -1----charging
********************************************************/
int coul_get_battery_tte (void)
{
    int cc, cur;
    struct smartstar_coul_device *di = g_smartstar_coul_dev;

    if (!coul_is_battery_exist()){
        return 0;
    }

    cur =  di->coul_dev_ops->get_battery_current_ua();

    if (cur < 0){
        return -1; /* charging now */
    }

    cc = di->batt_ruc - di->batt_uuc;

	if (cc <= 0) {
        return -1;
    }

    return cc * 60 / cur;
}

 /*******************************************************
  Function:        coul_get_battery_ttf
  Description:     time to full
  Input:           NULL
  Output:          NULL
  Return:          value in min or -1----discharging
********************************************************/
int coul_get_battery_ttf (void)
{
    int cc, cur;
    struct smartstar_coul_device *di = g_smartstar_coul_dev;

    cur =  di->coul_dev_ops->get_battery_current_ua();

    if (!coul_is_battery_exist()){
        return 0;
    }

    if (cur >= 0){
        return -1; /* discharging now */
    }

    cc = di->batt_fcc - di->batt_ruc;

    return cc * 60 / (-cur); /* cur is < 0 */
}

/*******************************************************
  Function:        smartstar_battery_health
  Description:    battery health l
  Input:            NULL
  Output:          NULL
  Return:          0: Unknown, 1:Good, 2:Overheat, 3:Dead, 4:Over voltage,
        	         5:Unspecified failure, 6:Cold
********************************************************/
int coul_get_battery_health (void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;
    int status = POWER_SUPPLY_HEALTH_GOOD;
    int temp = di->batt_temp/10;

    if (!coul_is_battery_exist()){
        return 0;
    }

    if (temp < batt_temp_too_cold)
        status = POWER_SUPPLY_HEALTH_COLD;
    else if (temp > batt_temp_too_hot)
        status = POWER_SUPPLY_HEALTH_OVERHEAT;

    return status;
}

/*******************************************************
  Function:        coul_get_battery_capacity_level
  Description:     battery health level
  Input:           NULL
  Output:          NULL
  Return:          0: Unknown, 1:CRITICAL, 2:LOW, 3:NORMAL, 4:HIGH,
        	         5:FULL
********************************************************/
int coul_get_battery_capacity_level (void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;
    int data_capacity = di->batt_soc;
    int status;

    if (!coul_is_battery_exist()){
        return 0;
    }

    if(data_capacity > 100 || data_capacity < 0)
        return POWER_SUPPLY_CAPACITY_LEVEL_UNKNOWN;
    else if ((data_capacity >= 0) && (data_capacity <= 5))
        status = POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL;
    else if ((data_capacity > 5) && (data_capacity <= 15))
        status = POWER_SUPPLY_CAPACITY_LEVEL_LOW;
    else if ((data_capacity >= 95) && (data_capacity < 100))
        status = POWER_SUPPLY_CAPACITY_LEVEL_HIGH;
    else if (100 == data_capacity)
        status = POWER_SUPPLY_CAPACITY_LEVEL_FULL;
    else
        status = POWER_SUPPLY_CAPACITY_LEVEL_NORMAL;

    return status;
}

/*******************************************************
  Function:        coul_get_battery_technology
  Description:     get battery_technology
  Input:           NULL
  Output:          NULL
  Return:          "Li-poly"
********************************************************/
int coul_get_battery_technology (void)
{
    /*Default technology is "Li-poly"*/
    return POWER_SUPPLY_TECHNOLOGY_LIPO;
}

/*******************************************************
  Function:        coul_get_battery_charge_params
  Description:     battery data params
  Input:           NULL
  Output:          NULL
  Return:          0: invalid battery, 1: successed
********************************************************/
struct chrg_para_lut *coul_get_battery_charge_params(void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;
    return di->batt_data->chrg_para;
}

/*******************************************************
  Function:        coul_get_battery_vbat_max
  Description:     battery vbat max vol
  Input:           NULL
  Output:          NULL
  Return:          max vbatt vol
********************************************************/
int coul_get_battery_vbat_max(void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;
    return di->batt_data->vbatt_max;
}

int coul_battery_cycle_count(void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;
    return di->batt_chargecycles / 100;
}

int coul_battery_fcc_design(void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;
    return di->batt_data->fcc;
}
int coul_get_battery_aging_safe_policy(AGING_SAFE_POLICY_TYPE *asp)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;
    unsigned int i;

    if (!di || !asp ||!di->basp_total_level)
    {
	if(BAT_BOARD_UDP ==  is_board_type)
	{
		return 0;
	}
        hwlog_err(BASP_TAG"[%s] input param NULL!\n", __func__);
        return -1;
    }

    if (di->basp_level > (di->basp_total_level - 1))
    {
        hwlog_err(BASP_TAG"[%s] basp_level out of range:%d!\n", __func__, di->basp_level);
        return -1;
    }
    if ((0 == basp_learned_fcc) || (0 > basp_learned_fcc))
    {
        hwlog_err(BASP_TAG"[%s] basp_learned_fcc wrong:%d!\n", __func__, basp_learned_fcc);
        return -1;
    }

    for (i = 0; i < di->basp_total_level; i++) {
        if (di->basp_level == basp_policy[i].level) {
            asp->level = basp_policy[i].level;
            asp->dc_volt_dec = basp_policy[i].dc_volt_dec;
            asp->nondc_volt_dec = basp_policy[i].nondc_volt_dec;
            asp->cur_ratio = basp_policy[i].cur_ratio;
            asp->cur_ratio_policy = basp_policy[i].cur_ratio_policy;
            asp->learned_fcc = (unsigned int)basp_learned_fcc;
            break;
        }
    }

    if (i == di->basp_total_level) {
        hwlog_err(BASP_TAG"level %d is not exist\n", di->basp_level);
        return -1;
    }
    return 0;
}

/*******************************************************
  Function:        coul_get_battery_limit_fcc
  Description:     get the battery limit fcc
  Input:           NULL
  Output:          NULL
  Return:          limit fcc
********************************************************/
int coul_get_battery_limit_fcc(void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;
    return di->batt_limit_fcc / 1000;
}

/*******************************************************
  Function:        coul_is_fcc_debounce
  Description:     check whether fcc is debounce
  Input:           NULL
  Output:          NULL
  Return:          0: no  1: is debounce
********************************************************/
int coul_is_fcc_debounce(void)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;
    int ret = 0;
    int batt_fcc = 0;
    int fcc = 0;

    if (!di) {
        //hwlog_err("%s, di is NULL\n", __func__);
        return ret;
    }

    if (!coul_is_battery_exist()) {
        return ret;
    }

    batt_fcc = coul_get_battery_fcc();
    fcc = interpolate_fcc(di, di->batt_temp);
    if (batt_fcc < (fcc * 85 / 100)  || batt_fcc > (fcc * 115 / 100)) {
        ret = 1;
        hwlog_err("%s, fcc_from_temp=%d, batt_fcc=%d, ret=%d\n", __func__, fcc, batt_fcc, ret);
    }

    return ret;
}

/*******************************************************
*  Function:        coul_device_check
*  Description:     check coul is ok
*  Parameters:      void
*  return value:    0: success  1: fail
********************************************************/
static int coul_device_check(void)
{
     return COUL_IC_GOOD;
}

/*******************************************************
  Function:       get_ocv_by_fcc
  Description:    interpolate ocv value by full charge capacity when charging done
  Input:
                  struct smartstar_coul_device *di     ---- coul device
                  int batt_temp                        ---- battery temperature
  Output:         NULL
  Return:         NULL
********************************************************/
static void get_ocv_by_fcc(struct smartstar_coul_device *di,int batt_temp)
{
    unsigned int new_ocv;
    int batt_temp_degc = batt_temp/10;
    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
   	    return;
    }
    /*looking for ocv value in the OCV-FCC table*/
    new_ocv = interpolate_ocv(di->batt_data->pc_temp_ocv_lut, batt_temp_degc, 1000);
    new_ocv *=1000;

    if ((new_ocv - di->batt_ocv) > 0) {
        DBG_CNT_INC(dbg_ocv_cng_1);
        hwlog_info("full charged, and OCV change, "
                            "new_ocv = %d, old_ocv = %d \n",new_ocv,di->batt_ocv);
        di->batt_ocv = new_ocv;
        di->batt_ocv_temp = di->batt_temp;
        di->coul_dev_ops->save_ocv_temp((short)di->batt_ocv_temp);
        di->batt_ocv_valid_to_refresh_fcc = 1;
        di->coul_dev_ops->save_ocv(new_ocv, IS_UPDATE_FCC);
		di->last_ocv_level = OCV_LEVEL_0;
		di->coul_dev_ops->save_ocv_level(di->last_ocv_level);
        coul_clear_cc_register();
        coul_clear_coul_time();
    } else {
        DBG_CNT_INC(dbg_ocv_fc_failed);
        hwlog_err("full charged, but OCV don't change,\
                            new_ocv = %d, old_ocv = %d \n",new_ocv,di->batt_ocv);
    }
}
/*******************************************************
  Function:        is_in_capacity_dense_area
  Description:     judege if in capacity dense area
  Input:           ocv_uv
  Output:          NULL
  Return:          TRUE or FALSE
********************************************************/
static bool is_in_capacity_dense_area(int ocv_uv)
{
	if( ocv_uv < CAPACITY_DENSE_AREA_3200
		|| (ocv_uv > CAPACITY_DENSE_AREA_3670 && ocv_uv < CAPACITY_DENSE_AREA_3690)
		|| (ocv_uv > CAPACITY_DENSE_AREA_3730 && ocv_uv < CAPACITY_DENSE_AREA_3800))
		return TRUE;
	return FALSE;
}

static int get_ocv_vol_from_fifo(struct smartstar_coul_device *di)
{
    int i = 0;
    int used;
    int current_ua = 0;
    int voltage_uv = 0;
    int totalvol, totalcur;
    int fifo_depth = 0;

	if(NULL == di || NULL == di->coul_dev_ops)
	{
		hwlog_info("[%s]di is null\n",__FUNCTION__);
		return 0;
	}

    totalvol = 0;
    totalcur = 0;
    used = 0;
    fifo_depth = di->coul_dev_ops->get_fifo_depth();
	for (i = 0; i < fifo_depth; i++)
	{
        current_ua = di->coul_dev_ops->get_battery_cur_ua_from_fifo(i);
        voltage_uv = di->coul_dev_ops->get_battery_vol_uv_from_fifo(i);
		if (current_ua >= CURRENT_LIMIT
			|| current_ua < CHARGING_CURRENT_OFFSET)
		{
			DBG_CNT_INC(dbg_invalid_vol);
			hwlog_info("invalid current = %d ua\n", current_ua);
			continue;
		}
		if (voltage_uv >= CAPACITY_INVALID_AREA_4500 || voltage_uv <= CAPACITY_INVALID_AREA_2500)
		{
			DBG_CNT_INC(dbg_invalid_vol);
			hwlog_info("invalid voltage = %d uv\n", voltage_uv);
			continue;
		}
		DBG_CNT_INC(dbg_valid_vol);
		hwlog_info("valid current = %d ua\n", current_ua);
        totalvol += di->coul_dev_ops->get_battery_vol_uv_from_fifo(i);
        totalcur += current_ua;
		used++;
	}

	hwlog_info("used = %d\n", used);
	if (used > 0)
	{
		voltage_uv = totalvol / used;
		current_ua = totalcur / used;
		voltage_uv += current_ua/1000*(di->r_pcb/1000 + DEFAULT_BATTERY_OHMIC_RESISTANCE);
		return voltage_uv;
	}
	return 0;
}

/*******************************************************
  Function:        coul_get_fifo_avg_current_ma
  Description:    get the average current of the coul fifo
  Input:           NULL
  Output:          NULL
  Return:          average current in ma
********************************************************/
static int coul_get_fifo_avg_current_ma(void)
{
    int i = 0;
    int valid = 0;
    int current_ua = 0;
    int totalcur = 0;
    int fifo_depth = 0;
    struct smartstar_coul_device *di = g_smartstar_coul_dev;

    if(NULL == di || NULL == di->coul_dev_ops) {
        hwlog_info("[%s]di is null\n",__FUNCTION__);
        return 0;
    }

    fifo_depth = di->coul_dev_ops->get_fifo_depth();
    for (i = 0; i < fifo_depth; i++) {
        current_ua = di->coul_dev_ops->get_battery_cur_ua_from_fifo(i);

        if (current_ua == 0)
            continue;
        valid++;
        totalcur += current_ua;
    }

    if (valid) {
        current_ua = totalcur / valid;
    } else {
        current_ua =  di->coul_dev_ops->get_battery_current_ua();
    }

    return (current_ua/1000);
}


/*******************************************************
  Function:        get_ocv_by_vol
  Description:     calculate ocv by 10 history data when AP exist from deep sleep
  Input:           struct smartstar_coul_device *di      ---- coul device
  Output:          NULL
  Return:          OCV
********************************************************/
static void get_ocv_by_vol(struct smartstar_coul_device *di)
{
    int voltage_uv = 0;
    int rm = 0;

	voltage_uv = get_ocv_vol_from_fifo(di);

	if (0 == voltage_uv)
		return;
	if(is_in_capacity_dense_area(voltage_uv)){
		hwlog_info("do not update OCV(%d)\n", voltage_uv);
		return;
	}
	hwlog_info("awake from deep sleep, old OCV = %d \n",
					   di->batt_ocv);
	di->batt_ocv = voltage_uv;
	di->batt_ocv_temp = di->batt_temp;
	di->coul_dev_ops->save_ocv_temp((short)di->batt_ocv_temp);
	di->batt_ocv_valid_to_refresh_fcc = 1;
	record_ocv_cali_info(di);
	coul_clear_cc_register();
	coul_clear_coul_time();
	di->coul_dev_ops->save_ocv(voltage_uv, IS_UPDATE_FCC);
	hwlog_info("awake from deep sleep, new OCV = %d,fcc_flag=%d \n", di->batt_ocv, di->batt_ocv_valid_to_refresh_fcc);
	DBG_CNT_INC(dbg_ocv_cng_0);
	if(CHARGING_STATE_CHARGE_DONE == di->charging_state)
	{
	    coul_get_rm(di, &rm);
	    if(rm < di->batt_limit_fcc)
	    {
	        di->batt_limit_fcc = rm*100/101;
	        di->is_nv_need_save = 1;
	        di->coul_dev_ops->set_nv_save_flag(NV_SAVE_FAIL);
	    }
	}
}

/*******************************************************
  Function:        get_calc_ocv
  Description:     get ocv by soft way when shutdown time less 20min
  Input:           struct smartstar_coul_device *di      ---- coul device
  Output:          NULL
  Return:          OCV
********************************************************/
static int get_calc_ocv(struct smartstar_coul_device *di)
{
    int ocv = 0;
    int batt_temp = 0;
    int chargecycles = 0;
    int soc_rbatt = 0;
    int rbatt = 0;
    int vbatt_uv = 0;
    int ibatt_ua = 0;
    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
   	    return -1;
    }
    batt_temp = di->batt_temp;
    chargecycles = di->batt_chargecycles/100;

    vbatt_uv = di->coul_dev_ops->convert_regval2uv(di->nv_info.calc_ocv_reg_v);

    ibatt_ua = di->coul_dev_ops->convert_regval2ua(di->nv_info.calc_ocv_reg_c);

    soc_rbatt = calculate_pc(di, vbatt_uv, batt_temp, chargecycles);

    rbatt = get_rbatt(di, soc_rbatt/10, batt_temp);
    ocv =  vbatt_uv + ibatt_ua*rbatt/1000;

    hwlog_info("calc ocv, v_uv=%d, i_ua=%d, soc_rbatt=%d, rbatt=%d, ocv=%d\n",
        vbatt_uv, ibatt_ua, soc_rbatt/10, rbatt, ocv);

    return ocv;
}

/*******************************************************
  Function:        coul_get_initial_ocv
  Description:     get first ocv from register, hardware record it during system reset.
  Input:           struct smartstar_coul_device *di      ---- coul device
  Output:          NULL
  Return:          init OCV
********************************************************/
static void coul_get_initial_ocv(struct smartstar_coul_device *di)
{
    unsigned short ocvreg = 0;
    int ocv_uv = 0;

    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
   	    return;
    }

    ocvreg = di->coul_dev_ops->get_ocv();
    hwlog_info("[%s]ocvreg = 0x%x\n", __func__,ocvreg);
    di->batt_ocv_valid_to_refresh_fcc = 1;

    if (ocvreg == (unsigned short)FLAG_USE_CLAC_OCV){
	 	hwlog_info("using calc ocv.\n");
        ocv_uv = get_calc_ocv(di);
        di->batt_ocv_valid_to_refresh_fcc = 0;
        di->coul_dev_ops->save_ocv(ocv_uv, NOT_UPDATE_FCC);/*ocv temp saves in fastboot*/
        di->is_nv_need_save = 0;
		di->last_ocv_level = INVALID_SAVE_OCV_LEVEL;
		di->coul_dev_ops->save_ocv_level(di->last_ocv_level);
    }
    else if (di->coul_dev_ops->get_use_saved_ocv_flag()){
        if (di->coul_dev_ops->get_fcc_invalid_up_flag())
            di->batt_ocv_valid_to_refresh_fcc = 0;
        di->is_nv_need_save = 0;
        ocv_uv = di->coul_dev_ops->convert_regval2uv(ocvreg);
        hwlog_info("using save ocv.\n");
    } else {
        if (di->coul_dev_ops->get_fcc_invalid_up_flag()){
            di->batt_ocv_valid_to_refresh_fcc = 0;
        }
        ocv_uv = di->coul_dev_ops->convert_regval2uv(ocvreg);
        di->is_nv_need_save = 0;
        hwlog_info("using pmu ocv from fastboot.\n");
    }

    di->batt_ocv_temp = di->coul_dev_ops->get_ocv_temp();
    di->batt_ocv = ocv_uv;
	di->coul_dev_ops->get_ocv_level(&(di->last_ocv_level));
#ifdef CONFIG_HISI_DEBUG_FS
	print_multi_ocv_threshold();
#endif
    hwlog_info("initial OCV = %d , OCV_temp=%d, fcc_flag= %d, ocv_level:%d\n", di->batt_ocv,di->batt_ocv_temp,di->batt_ocv_valid_to_refresh_fcc, di->last_ocv_level);
}

/*******************************************************
  Function:        coul_set_low_vol_int
  Description:     set low voltage value according low_power status.
  Input:           struct smartstar_coul_device *di  ---- coul device
                   state -- normal or low power state
  Output:          NULL
  Return:          NULL
********************************************************/
static void coul_set_low_vol_int(struct smartstar_coul_device *di, int state)
{
    int vol = 0;

    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
   	 return;
    }
    di->v_low_int_value = state;

    if (state == LOW_INT_STATE_RUNNING){
        vol = di->v_cutoff;
    }
    else{
        vol = di->v_cutoff_sleep;
    }
    di->coul_dev_ops->set_low_low_int_val(vol);
}

/*******************************************************
  Function:        get_battery_id_voltage
  Description:     get voltage on ID pin from nv by writting in fastoot.
  Input:           struct smartstar_coul_device *di  ---- coul device
  Output:          NULL
  Return:          NULL
  Remark:         called in module initalization
********************************************************/
static void get_battery_id_voltage(struct smartstar_coul_device *di)
{
    /*change ID get from NTC resistance by HKADC path*/
    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
   	    return;
    }
    if((int)di->nv_info.hkadc_batt_id_voltage == INVALID_BATT_ID_VOL)
        di->batt_id_vol= 0;
    else
        di->batt_id_vol = (int)di->nv_info.hkadc_batt_id_voltage;
        hwlog_info("get battery id voltage is %d mv\n",di->batt_id_vol);
}


/*******************************************************
  Function:        get_battery_id_voltage_real
  Description:     get voltage on ID pin by HKADC.
  Input:           struct smartstar_coul_device *di  ---- coul device
  Output:          NULL
  Return:          NULL
********************************************************/
static void get_battery_id_voltage_real(struct smartstar_coul_device *di)
{
    int volt;

    volt = hisi_adc_get_adc(adc_batt_id);
    if(volt < 0){ //negative means get adc fail
        hwlog_err("HKADC get battery id fail\n");
        volt = 0;
    }
	di->batt_id_vol = (unsigned int)volt;
    hwlog_info("get battery id voltage is %d mv\n",di->batt_id_vol);
}

/*******************************************************
  Function:        bound_soc
  Description:     bound soc.
  Input:           soc
  Output:          NULL
  Return:          bound soc
********************************************************/
static int bound_soc(int soc)
{
	soc = max(0, soc);
	soc = min(100, soc);
	return soc;
}

/*******************************************************
  Function:        coul_get_battery_id_vol
  Description:     get voltage on ID pin by HKADC.
  Input:           NULL
  Output:          NULL
  Return:          batt id vol
********************************************************/
static int coul_get_battery_id_vol(void)
{
     struct smartstar_coul_device *di = g_smartstar_coul_dev;

     get_battery_id_voltage_real(di);
     return di->batt_id_vol;
}
/*******************************************************
  Function:        calculate_delta_rc
  Description:     calculate delta cc
  Input:           di,soc...
  Output:          NULL
  Return:          delta rc
********************************************************/
static int calculate_delta_rc(struct smartstar_coul_device *di, int soc,
		int batt_temp, int rbatt_tbl, int fcc_uah)
{
    int ibat_ua = 0, vbat_uv = 0;
    int pc_new = 0, delta_pc = 0, pc_new_100 = 0, delta_pc_100 = 0;
    int rc_new_uah = 0, delta_rc_uah = 0, delta_rc_uah_100 = 0, delta_rc_final = 0;
    int soc_new = -EINVAL;
    int ocv = 0, delta_ocv = 0, delta_ocv_100 = 0, ocv_new = 0;
    int rbatt_calc = 0, delta_rbatt = 0;
    int batt_temp_degc = batt_temp/10;
    int ratio = 0;
    struct vcdata vc = {0};
    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
   	    return -1;
    }
    coul_get_battery_voltage_and_current(di, &ibat_ua, &vbat_uv);

    di->coul_dev_ops->get_fifo_avg_data(&vc);

    vc.avg_v += (di->r_pcb/1000)*(vc.avg_c)/1000;

    if ((2 == di->soc_limit_flag) && (LOW_TEMP_OPT_OPEN == low_temp_opt_flag)){/*exit sleep , cal uuc*/
        if ((ibat_ua/1000) < -50)/*sensorhub charging or charge done*/
            goto out;
        hwlog_info("low_temp_opt: old_vc.avg_c=%d,old vc.avg_v=%d\n",vc.avg_c,vc.avg_v);
        vc.avg_c  = ((ibat_ua/1000 > UUC_MIN_CURRENT_MA) ? (ibat_ua/1000) : UUC_MIN_CURRENT_MA);
        vc.avg_v = vbat_uv/1000;
        hwlog_info("low_temp_opt: new_vc.avg_c=%d,od vc.avg_v=%d\n",vc.avg_c,vc.avg_v);
    } else {
         if (vc.avg_c < 50)
            goto out;
    }

    if (di->coul_dev_ops->get_delta_rc_ignore_flag()){
        if (!((batt_temp_degc < 5) && (LOW_TEMP_OPT_OPEN == low_temp_opt_flag))){
            hwlog_info("first ignore delta_rc !\n");
            goto out;
        }
    }

    di->last_fifo_iavg_ma = vc.avg_c;

    ocv = interpolate_ocv(di->batt_data->pc_temp_ocv_lut, batt_temp_degc, di->batt_soc_real);

    rbatt_calc = (ocv - vc.avg_v)*1000/vc.avg_c;

    if (!g_ocv_cali_rbatt_valid_flag && di->coul_dev_ops->calculate_cc_uah()/UA_PER_MA < CALI_RBATT_CC_MAX && vc.avg_c > CALI_RBATT_CURR_MIN) {
        g_coul_ocv_cali_info[g_ocv_cali_index].cali_rbatt = rbatt_calc;
        g_ocv_cali_rbatt_valid_flag = 1;
    }

    ratio = rbatt_calc*100/rbatt_tbl;

    if ((2 == di->soc_limit_flag) && (LOW_TEMP_OPT_OPEN == low_temp_opt_flag) && (batt_temp_degc < 15)){
        ratio = ratio > 100 ? ratio : 100;
        hwlog_info("low_temp_opt: old ratio =%d,new ratio =%d\n",rbatt_calc*100/rbatt_tbl,ratio);
    }

    di->rbatt_ratio = ratio;

    delta_rbatt = rbatt_calc - rbatt_tbl;

    delta_ocv = delta_rbatt*vc.avg_c/1000;

    ocv_new = ocv - delta_ocv;

    pc_new = interpolate_pc(di->batt_data->pc_temp_ocv_lut, batt_temp, ocv_new);

    rc_new_uah = di->batt_fcc/1000 * pc_new;

    delta_pc = pc_new - di->batt_soc_real;

    delta_rc_uah = di->batt_fcc/1000 * delta_pc;

    if (ratio <= 0){
        delta_ocv_100 = -rbatt_tbl*vc.avg_c/1000;
        pc_new_100 = interpolate_pc(di->batt_data->pc_temp_ocv_lut, batt_temp, ocv-delta_ocv_100);
        delta_pc_100 = pc_new_100 - di->batt_soc_real;
        delta_rc_uah_100 = di->batt_fcc/1000 * delta_pc_100;

        delta_rc_final = delta_rc_uah - delta_rc_uah_100;
    }

    soc_new = (rc_new_uah)*100 / (fcc_uah);

    soc_new = bound_soc(soc_new);

out:
    hwlog_info("RBATT_ADJ: soc_new=%d rbat_calc=%d rbat_btl=%d ratio=%d "
                       "c=%d u=%d last_ocv=%d ocv_temp=%d "
                       "soc=%d.%d, ocv=%d "
                       "cmin=%d cmax=%d cavg=%d vavg=%d "
                       "delta_ocv=%d delta_pc=%d.%d delta_rc_uah=%d "
                       "delta_ocv_100=%d delta_pc_100=%d.%d delta_rc_uah_100=%d "
                       "delta_rc_final=%d \n",
                       soc_new, rbatt_calc, rbatt_tbl, ratio,
                       ibat_ua, vbat_uv, di->batt_ocv, di->batt_ocv_temp,
                       di->batt_soc_real/10, di->batt_soc_real%10, ocv,
                       vc.min_c, vc.max_c, vc.avg_c, vc.avg_v,
                       delta_ocv, delta_pc/10, (int)abs(delta_pc%10), delta_rc_uah,
                       delta_ocv_100, delta_pc_100/10, (int)abs(delta_pc_100%10), delta_rc_uah_100,
                       delta_rc_final
                       );

    di->batt_soc_est = soc_new;
    return delta_rc_final;

}
/*******************************************************
  Function:        adjust_soc
  Description:     adjust soc
  Input:           soc
  Output:          NULL
  Return:          soc new
********************************************************/
static int adjust_soc(struct smartstar_coul_device *di, int soc)
{
    int ibat_ua = 0, vbat_uv = 0;
    int delta_soc = 0, n = 0;
    int soc_new = soc;
    int soc_est_avg = 0;
    static int soc_ests[3] = {100,100,100};
    static int i = 0;
    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
   	 return -1;
    }
    coul_get_battery_voltage_and_current(di, &ibat_ua, &vbat_uv);

    if (ibat_ua < -CHARGING_CURRENT_OFFSET) {
        goto out;
    }

    if (di->batt_soc_est<0){
        goto out;
    }

    soc_ests[i%3] = di->batt_soc_est;
    i++;

	soc_est_avg = DIV_ROUND_CLOSEST((soc_ests[0] + soc_ests[1] + soc_ests[2]), 3);

    if (soc_est_avg>2){
        goto out;
    }

    delta_soc = soc - soc_est_avg;

    if (delta_soc <= 0){
        goto out;
    }

    n = 3 - soc_est_avg;

    soc_new = soc - delta_soc*n/3;

    if ((get_temperature_stably(di, USER_COUL) > TEMP_OCV_ALLOW_CLEAR*10) && (delta_soc > ABNORMAL_DELTA_SOC)){
        hwlog_info("delta_soc=%d, mark save ocv is invalid\n", delta_soc);
        di->coul_dev_ops->clear_ocv();
		di->last_ocv_level = INVALID_SAVE_OCV_LEVEL;
		di->coul_dev_ops->save_ocv_level(di->last_ocv_level);
        di->batt_ocv_valid_to_refresh_fcc = 0;

    }


out:
    hwlog_info("soc_est_avg=%d delta_soc=%d n=%d\n",
                       soc_est_avg, delta_soc, n);
    soc_new = bound_soc(soc_new);
    return soc_new;
}
/* 电量平滑修正*/
/*******************************************************
  Function:        limit_soc
  Description:     limt soc
  Input:           input_soc
  Output:          NULL
  Return:          soc new
********************************************************/
static int limit_soc(struct smartstar_coul_device *di,int input_soc)
{
    int output_soc = input_soc;
    static int power_on_cnt = 0;

    int last_soc = 0;
    int current_ua = 0;
    int voltage_uv = 0;
    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
   	 return -1;
    }
    last_soc = di->batt_soc;
    coul_get_battery_voltage_and_current(di, &current_ua, &voltage_uv);
/*change <=1%*/
    if (di->soc_limit_flag == 1){
	/*soc can not increase during discharging*/
        if(current_ua >= CHARGING_CURRENT_OFFSET) {
        	if(last_soc - input_soc >= 1){
                if ((LOW_TEMP_OPT_OPEN == low_temp_opt_flag) && (last_soc - input_soc >= 2) && (di->batt_temp < -50))
                    output_soc = last_soc - 2;
                else
                    output_soc = last_soc - 1;
            }
        	else
        		output_soc = last_soc;
        }
        else {
        	if(input_soc - last_soc >= 1)
        		output_soc = last_soc + 1;
        	else
        		output_soc = last_soc;
        }
    }
    /*exist from sleep*/
    else if (di->soc_limit_flag == 2){
        hwlog_info("current_ua:%d,last_soc:%d,input_soc:%d",current_ua,last_soc,input_soc);
        if((current_ua >= CHARGING_CURRENT_OFFSET) || (di->charging_state == CHARGING_STATE_CHARGE_STOP)) {
        	if(last_soc < input_soc)
        		output_soc = last_soc;
        }
        else {
        	if(last_soc > input_soc)
        		output_soc = last_soc;
        }
    }
    /* charge_done, then soc 100% */
    if (di->charging_state == CHARGING_STATE_CHARGE_DONE){
        hwlog_info("pre_chargedone output_soc = %d\n", output_soc);
        output_soc = 100;
    }
    if (di->charging_state == CHARGING_STATE_CHARGE_START &&
        voltage_uv/1000>BATTERY_SWITCH_ON_VOLTAGE &&
        output_soc==0 &&
        (current_ua<-CHARGING_CURRENT_OFFSET || power_on_cnt < 3)
        ){
        output_soc = 1;
    }

    if ((LOW_TEMP_OPT_OPEN == low_temp_opt_flag) && (1 != di->soc_limit_flag)) {
        if ((output_soc < 2) && (coul_get_battery_voltage_mv() > 3500)){
            output_soc = 3;
            hwlog_err("low_temp_opt: soc < 2,vol>3500,soc=3!\n");
        }
    }

    power_on_cnt ++;

    return output_soc;
}
/*******************************************************
  Function:        calculate_iavg_ma
  Description:     cal iavg ma
  Input:           iavg_ua
  Output:          iavg_ma
  Return:          NULL
********************************************************/
static void calculate_iavg_ma(struct smartstar_coul_device *di, int iavg_ua)
{
    int iavg_ma = iavg_ua / 1000;
    int i;
	static int iavg_samples[IAVG_SAMPLES];
	static int iavg_index = 0;
	static int iavg_num_samples;
       if( NULL == di )
       {
           hwlog_info("NULL point in [%s]\n", __func__);
      	    return;
       }
	iavg_samples[iavg_index] = iavg_ma;
	iavg_index = (iavg_index + 1) % IAVG_SAMPLES;
	iavg_num_samples++;
	if (iavg_num_samples >= IAVG_SAMPLES)
		iavg_num_samples = IAVG_SAMPLES;

	iavg_ma = 0;
	for (i = 0; i < iavg_num_samples; i++) {
		iavg_ma += iavg_samples[i];
	}

	iavg_ma = DIV_ROUND_CLOSEST(iavg_ma, iavg_num_samples);

    if (iavg_num_samples > IAVG_TIME_2MIN)
        di->last_iavg_ma = -iavg_ma;
    return;
}
/*******************************************************
  Function:        adjust_delta_rc
  Description:     limit delta_rc 1% change
  Input:           delta_rc,fcc_uah
  Output:          NA
  Return:          adjust delta rc
********************************************************/
static int adjust_delta_rc(struct smartstar_coul_device *di, int delta_rc, int fcc_uah)
{
    int max_changeable_delta_rc = fcc_uah * MAX_DELTA_RC_PC /100;
    if (abs(di->batt_pre_delta_rc - delta_rc) <= max_changeable_delta_rc)
    {
        di->batt_pre_delta_rc = delta_rc;
        return delta_rc;
    }
    hwlog_info("delta_rc change exceed 1 percents, pre = %d, current = %d\n", di->batt_pre_delta_rc, delta_rc);
    if (di->batt_pre_delta_rc > delta_rc)
    {
        di->batt_pre_delta_rc -= max_changeable_delta_rc;
    }
    else
    {
        di->batt_pre_delta_rc += max_changeable_delta_rc;
    }
    return di->batt_pre_delta_rc;
}
/*******************************************************
  Function:        calculate_soc_params
  Description:     cal soc params
  Input:           smartstar_coul_device *di
  Output:          fcc_uah,unusable_charge_uah,remaining_charge_uah,cc_uah,delta_rc_uah,rbatt
  Return:          NULL
********************************************************/
static void calculate_soc_params(struct smartstar_coul_device *di,
						int *fcc_uah,
						int *unusable_charge_uah,
						int *remaining_charge_uah,
						int *cc_uah,
						int *delta_rc_uah,
						int *rbatt)
{
    int soc_rbatt=0, iavg_ua=0;
    int batt_temp = 0;
    int chargecycles = 0;
    int delt_rc = 0;
    static int first_in = 1;
    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
   	 return;
    }
    batt_temp = di->batt_temp;
    chargecycles = di->batt_chargecycles/100;
    *fcc_uah = calculate_fcc_uah(di, batt_temp, chargecycles); // calc fcc by cc and soc change

    di->batt_fcc = *fcc_uah;

    /* calculate remainging charge */
    *remaining_charge_uah = calculate_remaining_charge_uah(di,
    				*fcc_uah, chargecycles);

    di->batt_rm = *remaining_charge_uah;

    /* calculate cc micro_volt_hour */
    di->cc_end_value = di->coul_dev_ops->calculate_cc_uah();
    *cc_uah = di->cc_end_value;

    di->batt_ruc = *remaining_charge_uah - *cc_uah;

    di->get_cc_end_time = di->coul_dev_ops->get_coul_time();

	di->batt_soc_real = DIV_ROUND_CLOSEST((*remaining_charge_uah - *cc_uah), (*fcc_uah/1000));

    hwlog_info("SOC real = %d\n", di->batt_soc_real);

    soc_rbatt = di->batt_soc_real/10;
    if (soc_rbatt < 0)
    	soc_rbatt = 0;

    *rbatt = get_rbatt(di, soc_rbatt, batt_temp);

#if RBATT_ADJ
    delt_rc = calculate_delta_rc(di, di->batt_soc_real, di->batt_temp, *rbatt, *fcc_uah);
    *delta_rc_uah = adjust_delta_rc(di, delt_rc, di->batt_fcc);
    di->batt_delta_rc = *delta_rc_uah;
#endif

    if (first_in){
        di->last_cc = di->cc_end_value;
        di->last_time = di->get_cc_end_time;
        iavg_ua =  di->coul_dev_ops->get_battery_current_ua();
        first_in = 0;
    }
    else{
        int delta_cc = di->cc_end_value - di->last_cc;
        int delta_time = di->get_cc_end_time - di->last_time;
        di->last_cc = di->cc_end_value;
        di->last_time = di->get_cc_end_time;

        if(delta_time > 0)
            iavg_ua = div_s64((s64)delta_cc * 3600, delta_time);
        else
            iavg_ua =  di->coul_dev_ops->get_battery_current_ua();

        hwlog_info("delta_time=%ds, iavg_ua=%d\n", delta_time, iavg_ua);
    }

    calculate_iavg_ma(di,iavg_ua);

    *unusable_charge_uah = calculate_unusable_charge_uah(di, *rbatt,
    				*fcc_uah, *cc_uah,
    				batt_temp, chargecycles, iavg_ua);
}
/*******************************************************
  Function:        calculate_state_of_charge
  Description:     cal soc
  Input:           smartstar_coul_device *di
  Output:          NA
  Return:          soc
********************************************************/
static int calculate_state_of_charge(struct smartstar_coul_device *di)
{
    int remaining_usable_charge_uah, fcc_uah, unusable_charge_uah, delta_rc_uah;
    int remaining_charge_uah, soc;
    int cc_uah;
    int rbatt;
    int soc_no_uuc, soc_before_adjust, soc_before_limit;
    bool soc_at_term_flag = true ;
    //unsigned int eco_leak_uah = 0;

    if( NULL == di )
    {
        hwlog_err("NULL point in [%s]\n", __func__);
   	 return -1;
    }
    if (!di->batt_exist){
        return 0;
    }

    calculate_soc_params(di,
                                    &fcc_uah,
                                    &unusable_charge_uah,
                                    &remaining_charge_uah,
                                    &cc_uah,
                                    &delta_rc_uah,
                                    &rbatt);

    hwlog_info("FCC=%duAh, UUC=%duAh, RC=%duAh, CC=%duAh, delta_RC=%duAh, Rbatt=%dmOhm\n",
                       fcc_uah, unusable_charge_uah, remaining_charge_uah, cc_uah, delta_rc_uah, rbatt);

    di->rbatt = rbatt;

    if (di->batt_limit_fcc && di->batt_limit_fcc < fcc_uah*di->soc_at_term/100){
        soc_at_term_flag = false;
        hwlog_info("FCC =%duAh term flag= %d\n", fcc_uah,soc_at_term_flag);
    }

    if (di->batt_limit_fcc && di->batt_limit_fcc<fcc_uah){
        fcc_uah = di->batt_limit_fcc;
        hwlog_info("use limit_FCC! %duAh\n", fcc_uah);
    }

	soc = DIV_ROUND_CLOSEST((remaining_charge_uah - cc_uah), (fcc_uah/100));

	soc_no_uuc = soc;
	di->batt_soc_with_uuc = soc;

    /* calculate remaining usable charge */
    //eco_leak_uah = calculate_eco_leak_uah();

	/* 退出ECO模式后 */
    //remaining_charge_uah = remaining_charge_uah - eco_leak_uah;

    remaining_usable_charge_uah = remaining_charge_uah
                                                - cc_uah - unusable_charge_uah + delta_rc_uah;

    if (fcc_uah - unusable_charge_uah <= 0) {
    	soc = 0;
    } else {
        if((100 == di->soc_at_term) || !soc_at_term_flag){
		soc = DIV_ROUND_CLOSEST((remaining_usable_charge_uah),
								((fcc_uah - unusable_charge_uah)/100));/*lint !e1058*/
        }else{
		soc = DIV_ROUND_CLOSEST((remaining_usable_charge_uah),
								((fcc_uah - unusable_charge_uah)*(di->soc_at_term)/100/100));/*lint !e1058*/
        }
    }

    if (soc > 100)
    	soc = 100;
	soc_before_adjust = soc;
    soc= adjust_soc(di, soc);
	soc_before_limit = soc;
    di->soc_unlimited = soc_before_limit;
    hwlog_info("SOC without UUC = %d, SOC before adjust = %d, SOC before limit = %d\n",soc_no_uuc, soc_before_adjust, soc_before_limit);
    /*not exiting from ECO Mode capacity can not change more than 1%*/
    soc = limit_soc(di, soc);
    if( 0 == g_eco_leak_uah)
    {
	    hwlog_info("NOT EXIT FROM ECO,SOC_NEW = %d\n",soc);
    }
    else
    {
	 	hwlog_info("EXIT FROM ECO,SOC_NEW = %d\n",soc);
		g_eco_leak_uah = 0;
    }
    /*default is no battery in sft and udp, so here soc is fixed 20 to prevent low power reset*/
    if ( BAT_BOARD_ASIC != is_board_type) {
	if (soc < 50)
		soc = 50;
	hwlog_info("SFT and udp board: adjust Battery Capacity to %d Percents\n", soc);
    }
    di->batt_soc = soc;

    return soc;
}

/*******************************************************
  Function:        coul_get_rm
  Description:     get remain capacity
  Input:           struct smartstar_coul_device *di  ---- coul device
  Output:          NULL
  Return:          NULL
********************************************************/
static void coul_get_rm(struct smartstar_coul_device *di, int *rm)
{
    int fcc_uah = 0, unusable_charge_uah = 0, delta_rc_uah = 0;
    int remaining_charge_uah = 0;
    int cc_uah = 0;
    int rbatt = 0;

    if( NULL == di )
    {
        hwlog_err("NULL point in [%s]\n", __func__);
        return;
    }

    calculate_soc_params(di,
                                    &fcc_uah,
                                    &unusable_charge_uah,
                                    &remaining_charge_uah,
                                    &cc_uah,
                                    &delta_rc_uah,
                                    &rbatt);
    *rm = remaining_charge_uah - cc_uah;
}



static void calc_initial_ocv(struct smartstar_coul_device *di)
{
#if (defined(CONFIG_HISI_CHARGER_ARCH) || defined(CONFIG_HUAWEI_CHARGER))
    int old_charge_state;
    old_charge_state = charge_set_charge_state(0);
#endif
	coul_cali_adc(di);
    mdelay(2500); // 2.2s for calibration, 0.11s for sampling, and 0.19s for pad
    di->batt_ocv_temp = di->batt_temp;
    di->coul_dev_ops->save_ocv_temp((short)di->batt_ocv_temp);
    di->batt_ocv = coul_get_battery_voltage_mv()*1000;
    di->coul_dev_ops->save_ocv(di->batt_ocv, NOT_UPDATE_FCC);
#if (defined(CONFIG_HISI_CHARGER_ARCH) || defined(CONFIG_HUAWEI_CHARGER))
    charge_set_charge_state(old_charge_state);
#endif
    coul_clear_cc_register();
    coul_clear_coul_time();

    hwlog_info("OCV = %d\n", di->batt_ocv);

}

static void battery_plug_in(struct smartstar_coul_device *di)
{

    hwlog_info("%s: Enter\n",__FUNCTION__);

    if(NULL == di)
    {
        hwlog_info(KERN_ERR "[%s]di is null.\n",__FUNCTION__);
        return;
    }

    di->batt_exist = 1;

    /*set battery data*/
    get_battery_id_voltage_real(di);

    di->batt_data = get_battery_data(di->batt_id_vol);
    if(di->batt_data != NULL)
    {
        hwlog_info("%s: batt ID is %d, batt_brand is %s\n",__FUNCTION__,di->batt_id_vol, di->batt_data->batt_brand);
    }else{
        hwlog_err("%s: %d di->batt_data is NULL  \n", __func__, __LINE__);
        return;
    }
    update_battery_temperature(di, TEMPERATURE_INIT_STATUS);
    /*calculate first soc */
    calc_initial_ocv(di);

    di->charging_stop_time = di->coul_dev_ops->get_coul_time();

    di->last_iavg_ma = IMPOSSIBLE_IAVG;
    di->prev_pc_unusable = -EINVAL;

    di->sr_resume_time = di->coul_dev_ops->get_coul_time();
    sr_cur_state = SR_DEVICE_WAKEUP;

    di->batt_chargecycles = 0;
    di->batt_changed_flag = 1;
    di->batt_limit_fcc = 0;
    di->adjusted_fcc_temp_lut = NULL;
    di->is_nv_need_save = 1;
    di->coul_dev_ops->set_nv_save_flag(NV_SAVE_FAIL);
    hwlog_info("new battery plug in, reset chargecycles!\n");

    /*get the first soc value*/
    DI_LOCK();
    di->soc_limit_flag = 0;
    di->batt_soc = calculate_state_of_charge(di);
    di->soc_limit_flag = 1;
    DI_UNLOCK();

    coul_set_low_vol_int(di, LOW_INT_STATE_RUNNING);

    /*schedule calculate_soc_work*/
    queue_delayed_work(system_power_efficient_wq, &di->calculate_soc_delayed_work,
                        round_jiffies_relative(msecs_to_jiffies(di->soc_work_interval)));

    // save battery plug in magic number
    di->coul_dev_ops->set_battery_moved_magic_num(BATTERY_PLUG_IN);
    blocking_notifier_call_chain(&notifier_list, BATTERY_MOVE, NULL);//hisi_coul_charger_event_rcv(evt);

    hwlog_info("%s: Exit\n",__FUNCTION__);

}

static void battery_plug_out(struct smartstar_coul_device *di)
{
    hwlog_info("%s: Enter\n",__FUNCTION__);

    di->batt_exist = 0;

    blocking_notifier_call_chain(&notifier_list, BATTERY_MOVE, NULL);//hisi_coul_charger_event_rcv(evt);

    cancel_delayed_work(&di->calculate_soc_delayed_work);

    // save battery move magic number
    di->coul_dev_ops->set_battery_moved_magic_num(BATTERY_PLUG_OUT);

    // clear saved ocv
    di->coul_dev_ops->clear_ocv();
	di->last_ocv_level = INVALID_SAVE_OCV_LEVEL;
	di->coul_dev_ops->save_ocv_level(di->last_ocv_level);
    /*clear saved last soc*/
    di->coul_dev_ops->clear_last_soc_flag();

    hwlog_info("%s: Exit\n",__FUNCTION__);

}

static void battery_check_work(struct work_struct *work)
{
    struct smartstar_coul_device *di = container_of(work,
				struct smartstar_coul_device,
				battery_check_delayed_work.work);

    int batt_exist = coul_is_battery_exist();

    if (batt_exist != di->batt_exist){
        if (batt_exist){
            battery_plug_in(di);
        }
        else{
            battery_plug_out(di);
        }
    }

    queue_delayed_work(system_power_efficient_wq, &di->battery_check_delayed_work,
                round_jiffies_relative(msecs_to_jiffies(BATTERY_CHECK_TIME_MS)));
}
static int basp_full_pc_by_voltage(struct smartstar_coul_device *di)
{
    int delta_ocv = 0,pc =0;
    int ret = 0 , batt_fcc_ocv = 0;
    AGING_SAFE_POLICY_TYPE asp;
    if(NULL == di)
    {
        return 0;
    }
    if(di->batt_data->vol_dec1 > 0)/*select age battery para*/
    {
        return TENTH*SOC_FULL;
    }
    memset(&asp,0,sizeof(asp));
    ret = coul_get_battery_aging_safe_policy(&asp);

    if(ret < 0)
    {
        hwlog_err("get_vol_dec fail!\n");
    }
    delta_ocv = (int)asp.nondc_volt_dec;
    if (delta_ocv) {
        batt_fcc_ocv = interpolate_ocv(di->batt_data->pc_temp_ocv_lut, di->batt_temp/TENTH, TENTH*SOC_FULL);
        pc = interpolate_pc(di->batt_data->pc_temp_ocv_lut, di->batt_temp, batt_fcc_ocv - delta_ocv);
        hwlog_info(BASP_TAG "batt_fcc_ocv = %d mV, delta_ocv = %dmV, pc = %d\n",
                             batt_fcc_ocv, delta_ocv, pc);
    }

    return pc;
}
static int calculate_real_fcc_uah(struct smartstar_coul_device *di,int *ret_fcc_uah);
static int calculate_qmax_uah(struct smartstar_coul_device *di, int fcc_uah)
{
    int pc;
    int max_fcc_uah;
    int qmax = fcc_uah;

    if (!di) {
        hwlog_err("%s input param NULL!\n", __func__);
        return 0;
    }

    pc = basp_full_pc_by_voltage(di);
    if (pc) {
        qmax = (int)((s64)fcc_uah * PERMILLAGE /pc);
    }

    max_fcc_uah = ((int)di->batt_data->fcc*UA_PER_MA/PERCENT)*FCC_MAX_PERCENT;
    if (qmax >= max_fcc_uah) {
        hwlog_err(BASP_TAG "qmax(%d uAh) is above max_fcc(%d uAh), use max_fcc.\n",
                          qmax, max_fcc_uah);
        qmax = max_fcc_uah;
    }

    hwlog_info(BASP_TAG "fcc_real = %dmAh, qmax = %dmAh\n",
                         fcc_uah/UA_PER_MA, di->qmax/UA_PER_MA);
    return qmax;
}
static int coul_get_qmax(struct smartstar_coul_device *di)
 {
    int fcc_uah;
    int index;
    int qmax_basp,qmax_nonbasp;

    if (!di || !di->batt_exist) {
        return 0;
    }

    index = (di->nv_info.latest_record_index -1);
    if (!di->nv_info.fcc_check_sum_ext) {
        return calculate_fcc_uah(di, di->batt_temp, (int)di->batt_chargecycles/PERCENT);
    } else {
        fcc_uah = di->nv_info.real_fcc_record[(int)(index+MAX_RECORDS_CNT)% MAX_RECORDS_CNT] * UA_PER_MA;
        qmax_basp = calculate_qmax_uah(di, fcc_uah);  //basp
        fcc_uah = calculate_fcc_uah(di, di->batt_temp, (int)di->batt_chargecycles/PERCENT);
        qmax_nonbasp = calculate_qmax_uah(di, fcc_uah);  //non-basp
        if (qmax_basp * PERCENT > qmax_nonbasp * FCC_MAX_PERCENT) {
            return qmax_nonbasp;
        } else {
            return qmax_basp;
        }
    }
 }
/* new battery, clear record fcc */
void clear_record_fcc(struct smartstar_coul_device *di)
{
    int index = 0;
    struct ss_coul_nv_info *pinfo = NULL;

    if (NULL == di)
    {
        hwlog_err(BASP_TAG"[%s], input param NULL!\n", __func__);
        return;
    }
    pinfo = &di->nv_info;

    for (index = 0; index < MAX_RECORDS_CNT; index++)/*clear learn fcc index check su*/
    {
        pinfo->real_fcc_record[index] = 0;
    }
    pinfo->latest_record_index = 0;
    pinfo->fcc_check_sum_ext = 0;
    memcpy(&my_nv_info, pinfo, sizeof(*pinfo));
    di->is_nv_need_save = 1;/*set save nv flag,clear nv*/
    di->coul_dev_ops->set_nv_save_flag(NV_SAVE_FAIL);
}

void basp_record_fcc(struct smartstar_coul_device *di)
{
    int index = 0;
    int sum = 0;
    int i = 0;
    struct ss_coul_nv_info *pinfo = NULL;
    char buff[DSM_BUFF_SIZE_MAX] = {0};

    if (NULL == di)
    {
        hwlog_err(BASP_TAG"[%s], input param NULL!\n", __func__);
        return;
    }
    pinfo = &di->nv_info;

    index = pinfo->latest_record_index % MAX_RECORDS_CNT;
    pinfo->real_fcc_record[index] = di->fcc_real_mah;
    pinfo->latest_record_index = index + 1;
    hwlog_info(BASP_TAG"[%s], learn times =%d,index=%d!\n", __func__,pinfo->latest_record_index,index);
    for(i = 0;i < MAX_RECORDS_CNT;i++)
    {
        sum = sum + pinfo->real_fcc_record[i];
    }
    pinfo->fcc_check_sum_ext = sum;

    snprintf(buff, (size_t)DSM_BUFF_SIZE_MAX, "fcc_real_mah:%d, batt_brand:%s, batt_fcc:%d, charging_begin_soc:%d, "
        "batt_chargecycles:%d, batt_ocv:%d, basp_level:%d\n",
        di->fcc_real_mah, di->batt_data->batt_brand, di->batt_fcc/1000, di->charging_begin_soc, di->batt_chargecycles/100,
        di->batt_ocv, di->basp_level);
    coul_dsm_report_ocv_cali_info(di, ERROR_SAFE_PLOICY_LEARN, buff);

    memcpy(&my_nv_info, pinfo, sizeof(*pinfo));
}

void basp_refresh_fcc(struct smartstar_coul_device *di)
{
    if (NULL == di)
    {
        hwlog_err(BASP_TAG"[%s], input param NULL!\n", __func__);
        return;
    }

    if ((basp_fcc_ls == LS_GOOD)
        && (di->batt_temp > BASP_FCC_LERAN_TEMP_MIN && di->batt_temp < BASP_FCC_LERAN_TEMP_MAX)
        && di->charging_begin_soc/10 < MIN_BEGIN_PERCENT_FOR_SAFE
        && di->batt_ocv_valid_to_refresh_fcc
        && ((di->batt_ocv>3200000 && di->batt_ocv<3670000)
            || (di->batt_ocv>3690000 && di->batt_ocv <3730000)
            || (di->batt_ocv>3800000 && di->batt_ocv <3900000)
            )
        )
    {
        int fcc_uah, new_fcc_uah, delta_fcc_uah, max_delta_fcc_uah;
        new_fcc_uah = calculate_real_fcc_uah(di, &fcc_uah);
        max_delta_fcc_uah = interpolate_fcc(di, di->batt_temp)*DELTA_SAFE_FCC_PERCENT*10;/*max delta*/
        delta_fcc_uah = new_fcc_uah - fcc_uah;
        if (delta_fcc_uah < 0)
            delta_fcc_uah = -delta_fcc_uah;
        if (delta_fcc_uah > max_delta_fcc_uah)
        {
            /* new_fcc_uah is outside the scope limit it */
            if (new_fcc_uah > fcc_uah)
                new_fcc_uah = (fcc_uah + max_delta_fcc_uah);
            else
                new_fcc_uah = (fcc_uah - max_delta_fcc_uah);
            hwlog_info(BASP_TAG"delta_fcc=%d > %d percent of fcc=%d"
                               "restring it to %d\n",
                               delta_fcc_uah, DELTA_SAFE_FCC_PERCENT,
                               fcc_uah, new_fcc_uah);
        }
        di->fcc_real_mah = new_fcc_uah / 1000;
        hwlog_info(BASP_TAG"refresh_fcc, start soc=%d, new fcc=%d \n",
            di->charging_begin_soc, di->fcc_real_mah);
        /* record fcc */
        basp_record_fcc(di);
        di->basp_level = get_basp_level(di);
    }
    else
    {
        hwlog_err(BASP_TAG"[%s], basp_fcc_ls:%d, batt_temp:%d, charging_begin_soc:%d, ocv_valid:%d, batt_ocv:%d\n", \
            __func__, basp_fcc_ls, di->batt_temp, di->charging_begin_soc, di->batt_ocv_valid_to_refresh_fcc, di->batt_ocv);
    }
    basp_fcc_learn_evt_handler(di, EVT_DONE);
}

bool basp_check_sum(void)
{
    int i = 0;
    int records_sum = 0;
    for (i = 0; i < MAX_RECORDS_CNT; i++)
    {
        records_sum += my_nv_info.real_fcc_record[i];
        hwlog_info(BASP_TAG"check fcc records, [%d]:%dmAh\n", i, my_nv_info.real_fcc_record[i]);
    }
    if(records_sum != my_nv_info.fcc_check_sum_ext)
    {
        hwlog_info(BASP_TAG"check learn fcc valid , records_sum=[%d],check_sum=%d\n", records_sum, my_nv_info.fcc_check_sum_ext);
        return FALSE;
    }
        return TRUE;
}
BASP_LEVEL_TYPE get_basp_level(struct smartstar_coul_device *di)
{
    unsigned int i = 0;
    unsigned int records_cnt = 0, records_sum = 0;
    unsigned int  records_avg = BASP_DEFAULT_RECORD_FCC_AVG;
    BASP_LEVEL_TYPE basp_level_ret = BASP_LEVEL_0;
    if(!basp_check_sum()) { /*learn fcc is invaild,clear NV*/
        clear_record_fcc(di);
    }
    for (i = 0; i < MAX_RECORDS_CNT; i++) {
        if (0 == my_nv_info.real_fcc_record[i]) {
            continue;
        }
        records_sum += my_nv_info.real_fcc_record[i];
        records_cnt++;
        hwlog_info(BASP_TAG"valid fcc records, [%d]:%dmAh\n", i, my_nv_info.real_fcc_record[i]);
    }

    if (!records_cnt) {
        basp_learned_fcc = di->batt_data->fcc;
    } else {
        records_avg = records_sum/records_cnt;
        if( records_avg < di->batt_data->fcc*basp_policy[0].fcc_ratio/BASP_PARA_SCALE) {
            basp_learned_fcc = di->batt_data->fcc*basp_policy[0].fcc_ratio/BASP_PARA_SCALE;
        } else {
            basp_learned_fcc = records_avg;
        }
    }

    if (records_cnt >= MAX_RECORDS_CNT) {
        for (i = 0; i < di->basp_total_level; i++) {
            if (records_avg <  di->batt_data->fcc*basp_policy[i].fcc_ratio/BASP_PARA_SCALE) {
                basp_level_ret = basp_policy[i].level;
                goto FuncEnd;
            }
        }
    } else {
        for (i = 0; i < di->basp_total_level; i++) {
            if ((di->batt_chargecycles > basp_policy[i].chrg_cycles) &&
                 (records_avg <  di->batt_data->fcc*basp_policy[i].fcc_ratio/BASP_PARA_SCALE)) {
                basp_level_ret = basp_policy[i].level;
                goto FuncEnd;
            }
        }
    }
    for (i = di->basp_total_level; i != 0; i--) {
        if ((di->batt_chargecycles < basp_policy[i-1].chrg_cycles) ||
             (BASP_LEVEL_0 == basp_policy[i-1].level)) {
             basp_level_ret = basp_policy[i-1].level;
             goto FuncEnd;
        }
    }

FuncEnd:
    hwlog_info(BASP_TAG"level:%d, charge_cycles:%d, bat_fcc:%dmAh, records_cnt:%d, records_avg:%dmAh\n", \
        basp_level_ret, di->batt_chargecycles, di->batt_data->fcc, records_cnt, records_avg);
    return basp_level_ret;
}

static int get_batt_charge_info(struct smartstar_coul_device *di, char *buff, int size)
{
    int i;
    int len = 0;

    if ((NULL == di) || (NULL == buff) || (size <= 0))
        return -1;

    for (i = 0; i < MAX_RECORDS_CNT; i++) {
        snprintf(buff + len, size - len,
            "real_fcc_record[%d]:%d,\n", i, my_nv_info.real_fcc_record[i]);
        len = strlen(buff);
    }

    snprintf(buff + len, size - len,
        "fcc_real_mah:%d, batt_brand:%s, batt_chargecycles:%d, basp_level:%d\n",
        di->fcc_real_mah, di->batt_data->batt_brand, di->batt_chargecycles/100, di->basp_level);

    return 0;
}

 /*******************************************************
  Function:        calculate_soc_work
  Description:     calculate soc every(schedule workqueue) CALCULATE_SOC_MS
  Input:           struct work_struct *work
  Output:          NULL
  Return:          NULL
********************************************************/
 static void calculate_soc_work(struct work_struct *work)
{
    struct smartstar_coul_device *di = container_of(work,
				struct smartstar_coul_device,
				calculate_soc_delayed_work.work);

    static int cali_cnt = 0;
    static int charged_cnt = 0;
    static int last_cc=0;
    static int last_time=0;
    int evt;
    int ret = -1;
    short offset_cur_modify_val = 0;
    static int basp_dsm_flag = 1;
    unsigned int i;
    char buff[DSM_BUFF_SIZE_MAX] = {0};

    if( NULL == di || NULL== work)
    {
        hwlog_err("NULL point in [%s]\n", __func__);
   	 	return;
    }
    if(di->is_nv_need_save){
        ret = save_nv_info(di);
        if(!ret)
        {
            di->is_nv_need_save = 0;
        }
    }
    if (!di->batt_exist){
    	hwlog_info("battery not exist, do not calc soc any more\n");
        return;
    }
    if(basp_dsm_flag)
    {
        for (i = 0; i < di->basp_total_level; i++) {
            if (di->basp_level == basp_policy[i].level) {
                if (basp_policy[i].err_no) {
                    get_batt_charge_info(di, buff, sizeof(buff));
                    basp_dsm_flag = coul_dsm_report_ocv_cali_info(di, basp_policy[i].err_no, buff);
                }
                break;
            }
        }
    }
    basp_fcc_learn_evt_handler(di, EVT_PER_CHECK);

    offset_cur_modify_val = di->coul_dev_ops->get_offset_current_mod();
    hwlog_info("offset_cur_modify_val:0x%x\n", offset_cur_modify_val);
    if(0 != offset_cur_modify_val)
    {
        hwlog_err("curexception, offset_cur_modify_val:0x%x\n", offset_cur_modify_val);
    }
    offset_cur_modify_val = di->coul_dev_ops->get_offset_vol_mod();
    if(0 != offset_cur_modify_val)
    {
        di->coul_dev_ops->set_offset_vol_mod();
        hwlog_err("curexception, offset_vol_modify_val:0x%x\n", offset_cur_modify_val);
    }

    DI_LOCK();
    /* calc soc */
    di->batt_soc = calculate_state_of_charge(di);
    check_batt_critical_electric_leakage(di);
    check_coul_abnormal_rollback(di);

   if (cali_cnt % (CALIBRATE_INTERVAL / di->soc_work_interval) == 0)
   {
       if(pl_calibration_en == FALSE)
       {
			coul_cali_adc(di);
       }
       else
       {
           hwlog_info("pl_calibration_en == TRUE, do not calibrate coul!\n");
       }
   }
   else if (cali_cnt % (CALIBRATE_INTERVAL / di->soc_work_interval) == 1)
   {
           di->coul_dev_ops->show_key_reg();
   }
    cali_cnt++;/*Here coul must calibrate! when first*/
    if (di->charging_state == CHARGING_STATE_CHARGE_DONE){
        if(charged_cnt == 0){
            last_cc = di->coul_dev_ops->calculate_cc_uah();
            last_time = di->coul_dev_ops->get_coul_time();
        }
        charged_cnt++;
        if (charged_cnt%(CHARGED_OCV_UPDATE_INTERVAL/CALCULATE_SOC_MS) == 0){
        	int sleep_cc, sleep_current;
        	int sleep_time, time_now;

            sleep_cc = di->coul_dev_ops->calculate_cc_uah();
            sleep_cc = sleep_cc - last_cc;
            time_now = di->coul_dev_ops->get_coul_time();
            sleep_time = time_now - last_time;

        	hwlog_info("sleep_cc=%d, sleep_time=%d\n", sleep_cc, sleep_time);

            if (sleep_time <= 0){
                charged_cnt --;
            }
            else {
            	sleep_current = (sleep_cc * 18) / (sleep_time * 5);  /* 18 5 means what */

                if(sleep_current<0){
                    sleep_current = -sleep_current;
                }

            	hwlog_info("sleep_current=%d\n", sleep_current);

                if (sleep_current < 20){
					di->last_ocv_level = OCV_LEVEL_0;
					di->coul_dev_ops->save_ocv_level(di->last_ocv_level);
                    get_ocv_by_vol(di);
                }
            }
            last_cc = di->coul_dev_ops->calculate_cc_uah();
            last_time = di->coul_dev_ops->get_coul_time();
        }

    }
    else{
        charged_cnt = 0;
    }

    DI_UNLOCK();

    if ((LOW_TEMP_OPT_OPEN == low_temp_opt_flag) && (di->batt_temp < 50) && (di->charging_state != CHARGING_STATE_CHARGE_DONE))
        di->soc_work_interval = CALCULATE_SOC_MS/2;
    else {
        if (di->batt_soc > 30)
            di->soc_work_interval = CALCULATE_SOC_MS;
        else
            di->soc_work_interval = CALCULATE_SOC_MS/2;
    }

    /* work faster when capacity <3% */
    if(di->batt_soc <= BATTERY_CC_LOW_LEV)
    {
		evt = BATTERY_LOW_SHUTDOWN;
		hwlog_info("SMARTSTAR SHUTDOWN SOC LEVEL\n");
		blocking_notifier_call_chain(&notifier_list, evt, NULL);


	}
    queue_delayed_work(system_power_efficient_wq, &di->calculate_soc_delayed_work,
    		round_jiffies_relative(msecs_to_jiffies(di->soc_work_interval)) );

}

static void read_temperature_work(struct work_struct *work)
{
    struct smartstar_coul_device *di = container_of(work, struct smartstar_coul_device,
                read_temperature_delayed_work.work);
    update_battery_temperature(di, TEMPERATURE_UPDATE_STATUS);
    queue_delayed_work(system_power_efficient_wq, &di->read_temperature_delayed_work, round_jiffies_relative(msecs_to_jiffies(READ_TEMPERATURE_MS)) );
}

 /*******************************************************
  Function:        make_cc_no_overload
  Description:    update coulomb start value
  Input:            struct smartstar_coul_device *di    ----coul device
  Output:          NULL
  Return:          NULL
********************************************************/
static void make_cc_no_overload(struct smartstar_coul_device *di)
{
	int cc;
    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
        return;
    }
	cc = di->coul_dev_ops->calculate_cc_uah();
    di->coul_dev_ops->save_cc_uah(cc);
}

/*******************************************************
  Function:        coul_low_vol_int_handle
  Description:     handle coul low_voltage int
  Input:           smartstar_coul_device *di
  Output:          NULL
  Return:          NULL
  Remark:          NULL
********************************************************/
 static void coul_low_vol_int_handle(struct smartstar_coul_device *di)
{
	int ibat_ua = 0, vbat_uv = 0;
    int delta_soc = 0;
    int count = LOW_INT_VOL_COUNT;
    char buff[DSM_BUFF_SIZE_MAX] = {0};

    if( NULL == di)
    {
	  	hwlog_info("NULL point in [%s]\n", __func__);
     	return;
    }
    if ( BAT_BOARD_ASIC != is_board_type){
        return;
    }

	if (di->v_low_int_value == LOW_INT_STATE_SLEEP){
		coul_get_battery_voltage_and_current(di, &ibat_ua, &vbat_uv);
		if ((vbat_uv/1000 - LOW_INT_VOL_OFFSET) < (int)di->v_cutoff_sleep){
			hwlog_info("IRQ: low_vol_int current_vol is [%d] < [%d],use fifo vol!\n",vbat_uv/1000,di->v_cutoff_sleep);
		}
	}
	else{
		coul_get_battery_voltage_and_current(di, &ibat_ua, &vbat_uv);
		if ((vbat_uv/1000 - LOW_INT_VOL_OFFSET) < di->v_cutoff){/*lint !e574*/
			hwlog_info("IRQ: low_vol_int current_vol is [%d] < [%d],use fifo vol!\n",vbat_uv/1000,di->v_cutoff);
		}
	}

	hwlog_err("IRQ: low_vol_int, cur_batt_soc=%d%%, cur_voltage=%dmv, cur_current=%dma, cur_temperature=%d\n", \
		di->batt_soc, vbat_uv/1000, -(ibat_ua/1000), di->batt_temp/10);

	if (di->batt_exist){
		hwlog_info("IRQ: COUL_VBAT_LOW_INT, vbat=%duv, last vbat_int=%d\n",
									vbat_uv, di->v_low_int_value );
	}
	else{
        hwlog_err("IRQ: COUL_VBAT_LOW_INT, no battery, error\n");
        return;
   }

    if (strstr(saved_command_line, "androidboot.swtype=factory"))
    {
        hwlog_err("IRQ: COUL_VBAT_LOW_INT, factory_version, do nothing\n");
        return;
    }
    if(-1 != vbat_uv)
	{
       vbat_uv /= 1000;
	}
	/*if BATTERY vol it too low ,it's false ,return*/
	if ((vbat_uv < BATTERY_VOL_LOW_ERR) && (-1 !=vbat_uv)){
		hwlog_err("Battery vol too low,low_vol_irq is't ture!\n");
        /*false low vol int ,next suspend don't cali*/

		/*return;*/
	}
    /*Redefine low_vol_int count according to dts*/
    count = di->low_vol_filter_cnt;
    if (di->v_low_int_value == LOW_INT_STATE_SLEEP){
		if ((vbat_uv - LOW_INT_VOL_OFFSET) > (int)di->v_cutoff_sleep){
			hwlog_err("false low_int,in sleep!\n");
            /*false low vol int ,next suspend don't cali*/
			return;
		} else {
			while(count--)
			{
				msleep(LOW_INT_VOL_SLEEP_TIME);  /*sleep 1s*/
				coul_get_battery_voltage_and_current(di, &ibat_ua, &vbat_uv);
				hwlog_err("delay 1000ms get vbat_uv is %duv!\n",vbat_uv);
				di->coul_dev_ops->show_key_reg();
				if ((vbat_uv/1000 - LOW_INT_VOL_OFFSET) > (int)di->v_cutoff_sleep){
					hwlog_err("fifo0 is false,it's got in 32k clk period!\n");
					/*false low vol int ,next suspend don't cali*/
					return;
				}
			}
			delta_soc = di->batt_soc - 2;
            di->batt_soc = 1;
		}
    }
    else if (di->v_low_int_value == LOW_INT_STATE_RUNNING){
		if ((vbat_uv - LOW_INT_VOL_OFFSET) > di->v_cutoff){/*lint !e574*/
			hwlog_err("false low_int,in running!\n");
            /*false low vol int ,next suspend don't cali*/
			return;

		} else {
			while(count--)
			{
				msleep(LOW_INT_VOL_SLEEP_TIME);  /*sleep 1s*/
				coul_get_battery_voltage_and_current(di, &ibat_ua, &vbat_uv);
				hwlog_err("delay 1000ms get vbat_uv is %duv!\n",vbat_uv);
				di->coul_dev_ops->show_key_reg();
				if ((vbat_uv/1000 - LOW_INT_VOL_OFFSET) > di->v_cutoff){/*lint !e574*/
					hwlog_err("fifo0 is false,it's got in 32k clk period!\n");
					/*false low vol int ,next suspend don't cali*/
					return;
				}
			}
            delta_soc = di->batt_soc;
            di->batt_soc = 0;
		}
    }

    if ((delta_soc > ABNORMAL_DELTA_SOC) && (get_temperature_stably(di, USER_COUL) > TEMP_OCV_ALLOW_CLEAR*10)){
        hwlog_info("delta_soc=%d, mark save ocv is invalid\n", delta_soc);
        /*dmd report: current information -- fcc, CC, cur_vol, cur_current, cur_temp, cur_soc, delta_soc*/
        snprintf(buff, (size_t)DSM_BUFF_SIZE_MAX, "[LOW VOL] fcc:%dmAh, CC:%dmAh, cur_vol:%dmV, cur_current:%dmA, cur_temp:%d, cur_soc:%d, delta_soc:%d",
        di->batt_fcc/UA_PER_MA, di->coul_dev_ops->calculate_cc_uah()/UA_PER_MA, vbat_uv/UVOLT_PER_MVOLT, -(ibat_ua/UA_PER_MA), di->batt_temp, di->batt_soc, delta_soc);
        coul_dsm_report_ocv_cali_info(di, ERROR_LOW_VOL_INT, buff);
        di->coul_dev_ops->clear_ocv();
		di->last_ocv_level = INVALID_SAVE_OCV_LEVEL;
		di->coul_dev_ops->save_ocv_level(di->last_ocv_level);
        di->batt_ocv_valid_to_refresh_fcc = 0;
    }
	blocking_notifier_call_chain(&notifier_list, BATTERY_LOW_SHUTDOWN, NULL);
}

/*******************************************************
  Function:        fcc_update_limit_by_ocv
  Description:     be called when charge begin, update batt_ocv_valid_to_refresh_fcc flag by ocv update long,
                   because  cc err that affects fcc accuracy is larger in long interval of ocv update.
				    ocv time limit(T):
				    current [200ma,]:
				              NA
				    current [50ma, 100ma]:
				            T<4H
				    current [100ma, 200ma]:
				            T<8H
				    current [,50ma]:
				            NA;
  Input:           struct smartstar_coul_device *di                 ---- coul device
  Output:          NULL
  Return:          NULL
********************************************************/
void fcc_update_limit_by_ocv(struct smartstar_coul_device *di)
{
    int ocv_update_time;
    int delta_cc_uah;
    int iavg_ma;

	if(NULL == di || NULL == di->coul_dev_ops)
	{
		hwlog_err("[%s]di is null\n",__FUNCTION__);
		return;
	}
    if (!di->batt_ocv_valid_to_refresh_fcc)
        return;
    ocv_update_time   = di->coul_dev_ops->get_coul_time();
    delta_cc_uah      = di->coul_dev_ops->calculate_cc_uah();/*ocv update begin*/
    if(ocv_update_time > 0)
        iavg_ma = (div_s64((s64)delta_cc_uah * SEC_PER_HOUR, ocv_update_time))/ UA_PER_MA;
    else
        iavg_ma = 0;

    if (iavg_ma > 200){
        hwlog_info("[%s]:current [200ma,]=%d ma!\n",__func__,iavg_ma);
    } else if (iavg_ma > 100) {
        if (ocv_update_time > (4 * SEC_PER_HOUR))
            di->batt_ocv_valid_to_refresh_fcc = 0;
        hwlog_info("[%s]: current [100ma,200]= %d,t=%d!\n",__func__,iavg_ma,ocv_update_time);
    } else if(iavg_ma > 50){
        if (ocv_update_time > (8 * SEC_PER_HOUR))
            di->batt_ocv_valid_to_refresh_fcc = 0;
        hwlog_info("[%s]:current [50ma,100]= %d,t=%d!\n",__func__,iavg_ma,ocv_update_time);
    } else
        hwlog_info("[%s]:current [,50ma]=%d ma,NA!\n",__func__,iavg_ma);

    hwlog_info("[%s]:fcc_flag = %d!\n",__func__,di->batt_ocv_valid_to_refresh_fcc);
}
/*******************************************************
  Function:        coul_charging_begin
  Description:    be called when charge begin, update charge status,
                       calc soc, begin cc,  can't be called in atomic context
  Input:            struct smartstar_coul_device *di                 ---- coul device
  Output:          NULL
  Return:          NULL
********************************************************/
static void coul_charging_begin (struct smartstar_coul_device *di)
{
    hwlog_info("coul_charging_begin +\n");
    if( NULL == di )
    {
        hwlog_info("NULL point in [%s]\n", __func__);
        return;
    }
    hwlog_info("pre charging state is %d \n",di->charging_state);
    /* disable coul irq */
    //smartstar_irq_disable();
    if (di->charging_state == CHARGING_STATE_CHARGE_START)
        return;

    di->charging_state = CHARGING_STATE_CHARGE_START;

	if (fcc_update_limit_flag)
		fcc_update_limit_by_ocv(di);

    /*calculate soc again*/
    di->batt_soc = calculate_state_of_charge(di);

    /*record soc of charging begin*/
    di->charging_begin_soc = di->batt_soc_real;
    basp_fcc_learn_evt_handler(di, EVT_START);

    /*record cc value*/
    di->charging_begin_cc = di->coul_dev_ops->calculate_cc_uah();
    di->charging_begin_time = di->coul_dev_ops->get_coul_time();

    hwlog_info("coul_charging_begin -\n");
    hwlog_info("batt_soc=%d, charging_begin_soc=%d, charging_begin_cc=%d\n",
                       di->batt_soc, di->charging_begin_soc, di->charging_begin_cc);

}

/*******************************************************
  Function:        coul_charging_stop
  Description:     be called when charge stop, update charge status, update chargecycles
                   calc soc, cc, rm and set low vol reg. can't be called in atomic context
  Input:           struct smartstar_coul_device *di                 ---- coul device
  Output:          NULL
  Return:          NULL
********************************************************/
static void coul_charging_stop (struct smartstar_coul_device *di)
{
    int rm, cc;
    int fcc_101 = 0;
    if( NULL == di )
    {
        hwlog_err("NULL point in [%s]\n", __func__);
        return;
    }
    fcc_101 = di->batt_fcc*101/100;
    if (CHARGING_STATE_CHARGE_UNKNOW == di->charging_state){
        return;
    }
    /* enable coul irq */
    di->coul_dev_ops->irq_enable();
    di->batt_soc = calculate_state_of_charge(di);

    if (CHARGING_STATE_CHARGE_START == di->charging_state){
        update_chargecycles(di);
        di->is_nv_need_save = 1;
        di->coul_dev_ops->set_nv_save_flag(NV_SAVE_FAIL);
    }
	if (fcc_update_limit_flag) {
		di->batt_ocv_valid_to_refresh_fcc = 0;
    }
    cc = di->coul_dev_ops->calculate_cc_uah();

    rm = di->batt_rm - cc;
    /* adjust rm */
    if (rm > fcc_101){
        cc = cc + (rm-fcc_101);
        di->coul_dev_ops->save_cc_uah(cc);
        di->batt_limit_fcc = 0;
        di->is_nv_need_save = 1;
        di->coul_dev_ops->set_nv_save_flag(NV_SAVE_FAIL);
    }
    else if (di->batt_soc == 100 && di->batt_soc_real > 950){
        di->batt_limit_fcc = rm*100/101;
        di->is_nv_need_save = 1;
        di->coul_dev_ops->set_nv_save_flag(NV_SAVE_FAIL);
    }

    if (di->is_nv_need_save){
        di->coul_dev_ops->set_nv_save_flag(NV_SAVE_FAIL);
    }
    di->charging_state = CHARGING_STATE_CHARGE_STOP;
    di->charging_stop_time = di->coul_dev_ops->get_coul_time();
    coul_set_low_vol_int(di, LOW_INT_STATE_RUNNING);
    /* set shutdown level */
    if (ENABLED == di->iscd->enable && di->iscd->size) {
        coul_wake_lock();
        schedule_delayed_work(&di->iscd->delayed_work, msecs_to_jiffies(0));
    }
}

/*******************************************************
  Function:        calculate_real_fcc_uah
  Description:     calc fcc by cc_change /soc_change
  Input:           struct smartstar_coul_device *di    ---- coul device
                   int *ret_fcc_uah   ----------real fcc
  Output:          ret_fcc_uah    ----------real fcc
  Return:          real_fcc
********************************************************/
static int calculate_real_fcc_uah(struct smartstar_coul_device *di,
								  int *ret_fcc_uah)
{
    int fcc_uah, unusable_charge_uah, delta_rc;
    int remaining_charge_uah;
    int cc_uah;
    int real_fcc_uah;
    int rbatt;
    int terminate_soc_real;
    if( NULL == di )
    {
        hwlog_err("NULL point in [%s]\n", __func__);
     	 return -1;
    }
    terminate_soc_real = di->batt_soc_real;
    calculate_soc_params(di,
                         &fcc_uah,
                         &unusable_charge_uah,
                         &remaining_charge_uah, &cc_uah, &delta_rc, &rbatt);
    real_fcc_uah = (-(cc_uah - di->charging_begin_cc))/(terminate_soc_real - di->charging_begin_soc)*terminate_soc_real;
    //real_fcc_uah = remaining_charge_uah - cc_uah;
    //real_fcc_uah = real_fcc_uah*100/101;
    *ret_fcc_uah = fcc_uah;
    hwlog_info("real_fcc=%d, RC=%d CC=%d fcc=%d charging_begin_soc=%d.%d\n",
    				   real_fcc_uah, remaining_charge_uah, cc_uah, fcc_uah, di->charging_begin_soc/10, di->charging_begin_soc);
    return real_fcc_uah;
}

/*******************************************************
  Function:        readjust_fcc_table
  Description:     fcc self_study, establish a temp_fcc lookup table
  Input:           struct smartstar_coul_device *di                 ---- coul device
  Output:          a temp_fcc lookup table
  Return:          NULL
********************************************************/
static void readjust_fcc_table(struct smartstar_coul_device *di)
{
	struct single_row_lut *temp, *now;
	int i, fcc, ratio;
	int real_fcc_mah;
	if( NULL == di )
    {
        hwlog_err("NULL point in [%s]\n", __func__);
        return;
    }
	real_fcc_mah = di->fcc_real_mah;  /* calc by delt_cc/delt_soc */
	if (!di->batt_data->fcc_temp_lut)
	{
		hwlog_err("%s The static fcc lut table is NULL\n", __func__);
		return;
	}
    if (di->adjusted_fcc_temp_lut == NULL){
    	temp = &di->adjusted_fcc_temp_lut_tbl1;
        now = di->batt_data->fcc_temp_lut;
    }
    else if (di->adjusted_fcc_temp_lut == &di->adjusted_fcc_temp_lut_tbl1){
    	temp = &di->adjusted_fcc_temp_lut_tbl2;
        now = di->batt_data->fcc_temp_lut;
    }
    else{
    	temp = &di->adjusted_fcc_temp_lut_tbl1;
        now = di->batt_data->fcc_temp_lut;
    }

	fcc = interpolate_fcc(di, di->batt_temp);
	temp->cols = now->cols;
	for (i = 0; i < now->cols; i++)
	{
		temp->x[i] = now->x[i];
		ratio = div_u64(((u64)(now->y[i]) * 1000), fcc);/*lint !e574 !e571*/
		temp->y[i] = (ratio * real_fcc_mah);
		temp->y[i] /= 1000;
		hwlog_info("temp=%d, staticfcc=%d, adjfcc=%d, ratio=%d\n",
						   temp->x[i], now->y[i],
						   temp->y[i], ratio);
	}
	di->adjusted_fcc_temp_lut = temp;
}

static bool is_fcc_ready_to_refresh(struct smartstar_coul_device *di, int charging_iavg_ma)
{
    if( NULL == di )
    {
        hwlog_err("NULL point in [%s]\n", __func__);
        return FALSE;
    }
	if (di->charging_begin_soc/10 < MIN_BEGIN_PERCENT_FOR_LEARNING
        && (di->batt_temp > FCC_UPDATE_TEMP_MIN && di->batt_temp < FCC_UPDATE_TEMP_MAX)
        && (di->batt_ocv_temp > FCC_UPDATE_TEMP_MIN)
        && (charging_iavg_ma < -FCC_UPDATE_CHARGING_CURR_MIN_MA)
	    && ((di->batt_ocv>3200000 && di->batt_ocv<3670000)
            || (di->batt_ocv>3690000 && di->batt_ocv <3730000)
            || (di->batt_ocv>3800000 && di->batt_ocv <3900000)
            )
        )
            return TRUE;
    return FALSE;
}

/*******************************************************
  Function:        refresh_fcc
  Description:     fcc self_study, check learning condition when charge done and
                   call readjust_fcc_table to establish a temp_fcc lookup table
  Input:           struct smartstar_coul_device *di                 ---- coul device
  Output:          a temp_fcc lookup table
  Return:          NULL
********************************************************/
void refresh_fcc(struct smartstar_coul_device *di)
{
    int delta_cc_uah;
    int charging_time;
    int charging_iavg_ma;
    char buff[DSM_BUFF_SIZE_MAX] = {0};

    if( NULL == di )
    {
        hwlog_err("NULL point in [%s]\n", __func__);
        return;
    }

    charging_time    = di->coul_dev_ops->get_coul_time() - di->charging_begin_time;
    delta_cc_uah     = di->coul_dev_ops->calculate_cc_uah() - di->charging_begin_cc;
    if(charging_time > 0)
        charging_iavg_ma = (div_s64((s64)delta_cc_uah * SEC_PER_HOUR, charging_time))/ UA_PER_MA;
    else
        charging_iavg_ma = 0;
    hwlog_info("[%s]:charging_time = %ds,delta_cc_uah = %duah,charging_iavg_ma = %dma\n",__func__,charging_time, delta_cc_uah, charging_iavg_ma);

	if (di->batt_ocv_valid_to_refresh_fcc
		&& (TRUE == is_fcc_ready_to_refresh(di, charging_iavg_ma)))
	{
		int fcc_uah, new_fcc_uah, delta_fcc_uah, max_delta_fcc_uah, design_fcc_mah;
		new_fcc_uah = calculate_real_fcc_uah(di, &fcc_uah);
		design_fcc_mah = interpolate_fcc(di, di->batt_temp);
		delta_fcc_uah = new_fcc_uah - fcc_uah;
		if (delta_fcc_uah < 0){
            delta_fcc_uah = -delta_fcc_uah;
            max_delta_fcc_uah = design_fcc_mah*DELTA_MAX_DECR_FCC_PERCENT*10;
        } else
            max_delta_fcc_uah = design_fcc_mah*DELTA_MAX_INCR_FCC_PERCENT*10;

		if (delta_fcc_uah > max_delta_fcc_uah)
		{
			/* new_fcc_uah is outside the scope limit it */
			if (new_fcc_uah > fcc_uah)
				new_fcc_uah = (fcc_uah + max_delta_fcc_uah);
			else
				new_fcc_uah = (fcc_uah - max_delta_fcc_uah);
			hwlog_info("delta_fcc=%d > %d percent of fcc=%d"
							   "restring it to %d\n",
							   delta_fcc_uah, max_delta_fcc_uah,
							   fcc_uah, new_fcc_uah);
			/*dmd report: current information -- old_fcc,new_fcc, delta_fcc, charging_begin_soc, charging_begin_cc, charing_end_cc, temp, basplevel*/
			snprintf(buff, (size_t)DSM_BUFF_SIZE_MAX, " [refresh fcc warning]old_fcc:%dmAh, new_fcc:%dmAh, delta_fcc:%dmAh, "
				"charging_beging_soc:%d, charging_begin_cc:%dmAh, charging_end_cc:%dmAh, temp:%d, basplevel:%d",
				di->fcc_real_mah, new_fcc_uah/UA_PER_MA, delta_fcc_uah/UA_PER_MA, di->charging_begin_soc, di->charging_begin_cc/UA_PER_MA,
				di->coul_dev_ops->calculate_cc_uah()/UA_PER_MA, di->batt_temp, di->basp_level);
			coul_dsm_report_ocv_cali_info(di, ERROR_REFRESH_FCC_OUTSIDE, buff);
		}
        di->fcc_real_mah = new_fcc_uah / 1000;
        /*limit max fcc, consider boardd 1.5*fcc gain */
        if (di->fcc_real_mah > design_fcc_mah*3/2)
            di->fcc_real_mah = design_fcc_mah*3/2;
        hwlog_info("refresh_fcc, start soc=%d, new fcc=%d \n",
            di->charging_begin_soc, di->fcc_real_mah);
        /* update the temp_fcc lookup table */
    	readjust_fcc_table(di);
	}
}

/*******************************************************
  Function:        coul_charging_done
  Description:     be called when charge finish, update charge status, chargecycles
                   calc soc(100%), OCV. can't be called in atomic context
                   refresh_fcc if can be
  Input:           struct smartstar_coul_device *di                 ---- coul device
  Output:          NULL
  Return:          NULL
********************************************************/
static void coul_charging_done (struct smartstar_coul_device *di)
{
    int rm = 0;
    int ret;
    int ocv_update_hour;
    if( NULL == di )
    {
        hwlog_err("NULL point in [%s]\n", __func__);
        return;
    }
    if (CHARGING_STATE_CHARGE_START != di->charging_state) {
        hwlog_info("coul_charging_done, but pre charge state is %d \n",
                            di->charging_state);
        return;
    }
    /*limt fcc refresh by ocv update time*/
    ocv_update_hour = di->coul_dev_ops->get_coul_time()/SEC_PER_HOUR;
    if (ocv_update_hour >= FCC_UPDATE_MAX_OCV_INTERVAL)
        di->batt_ocv_valid_to_refresh_fcc = 0;
    hwlog_info("done fcc_flag = %d,ocv_time = %d hour!\n",di->batt_ocv_valid_to_refresh_fcc,ocv_update_hour);
    /* enable coul irq */
    di->coul_dev_ops->irq_enable();
    basp_refresh_fcc(di);
    refresh_fcc(di);
    coul_get_rm(di, &rm);
    di->batt_limit_fcc = rm*100/101;
    hwlog_info("coul_charging_done, adjust soc from %d to 100\n",di->batt_soc);

    di->batt_soc = 100;

    if (di->batt_changed_flag)
    {
        /*recalculate charge cycles*/
        recalc_chargecycles(di);
        di->batt_changed_flag = 0;
    }
    get_ocv_by_fcc(di, di->batt_temp);

    di->batt_fcc  = calculate_fcc_uah(di, di->batt_temp, di->batt_chargecycles/100);
    //di->cc_start_value = -di->batt_fcc/100;

    update_chargecycles(di);

    di->is_nv_need_save = 1;
    di->coul_dev_ops->set_nv_save_flag(NV_SAVE_FAIL);

    /*NV save in shutdown charging */
    if (1 == get_pd_charge_flag()){
        if(di->is_nv_need_save){
            ret = save_nv_info(di);
            if(!ret)
            {
                di->is_nv_need_save = 0;
            }
        }
    }

    di->charging_state = CHARGING_STATE_CHARGE_DONE;
    hwlog_info("new charging cycles = %d%%\n", di->batt_chargecycles);
    if (ENABLED == di->iscd->enable) {
        di->iscd->last_sample_cnt = 0;
        if (!di->iscd->rm_bcd || !di->iscd->fcc_bcd) {
            di->iscd->rm_bcd = rm;
            di->iscd->fcc_bcd = di->batt_fcc;
        }
        di->iscd->last_sample_time = current_kernel_time();
        hrtimer_start(&di->iscd->timer, ktime_set((s64)di->iscd->sample_time_interval, (unsigned long)0), HRTIMER_MODE_REL);
    }
}

/*******************************************************
  Function:        charger_event_process
  Description:     charge event distribution function
  Input:           struct smartstar_coul_device *di  ---- coul device
                   unsigned int event                ---- charge event
  Output:          NULL
  Return:          NULL
********************************************************/
static void charger_event_process(struct smartstar_coul_device *di,unsigned int event)
{
    if( NULL == di )
    {
        hwlog_err("NULL point in [%s]\n", __func__);
     	 return;
    }
    switch (event) {
    	case VCHRG_START_USB_CHARGING_EVENT:
    	case VCHRG_START_AC_CHARGING_EVENT:
    	case VCHRG_START_CHARGING_EVENT:
            hwlog_info("receive charge start event = 0x%x\n",(int)event);
            /*record soc and cc value*/
            DI_LOCK();
            coul_charging_begin(di);
            DI_UNLOCK();
    		break;

    	case VCHRG_STOP_CHARGING_EVENT:
            hwlog_info("receive charge stop event = 0x%x\n",(int)event);
            DI_LOCK();
    	    coul_charging_stop(di);
    	    DI_UNLOCK();
    	    break;

    	case VCHRG_CHARGE_DONE_EVENT:
            hwlog_info("receive charge done event = 0x%x\n",(int)event);
            DI_LOCK();
    	    coul_charging_done(di);
    	    DI_UNLOCK();
    		break;

    	case VCHRG_NOT_CHARGING_EVENT:
    	    di->charging_state = CHARGING_STATE_CHARGE_NOT_CHARGE;
            hwlog_err("charging is stop by fault\n");
    	    break;

    	case VCHRG_POWER_SUPPLY_OVERVOLTAGE:
    	    di->charging_state = CHARGING_STATE_CHARGE_NOT_CHARGE;
            hwlog_err("charging is stop by overvoltage\n");
    		break;

    	case VCHRG_POWER_SUPPLY_WEAKSOURCE:
    	    di->charging_state = CHARGING_STATE_CHARGE_NOT_CHARGE;
            hwlog_err("charging is stop by weaksource\n");
    		break;

    	default:
            di->charging_state = CHARGING_STATE_CHARGE_NOT_CHARGE;
    	    hwlog_err("unknow event %d\n",(int)event);
    		break;
    }
}

/*******************************************************
  Function:        coul_battery_charger_event_rcv
  Description:     package charger_event_process, and be registered in scharger Model
                   to get charge event
  Input:           unsigned int event         ---- charge event
  Output:          NULL
  Return:          NULL
********************************************************/
int coul_battery_charger_event_rcv (unsigned int evt)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;
    if( NULL == di )
    {
        hwlog_err("NULL point in [%s]\n", __func__);
        return -1;
    }
    if (!di || !di->batt_exist){
        return 0;
    }
    if (!coul_is_battery_exist()){
        return 0;
    }

    charger_event_process(di,evt);
    return 0;
}

/*******************************************************
  Function:        coul_smooth_startup_soc
  Description:     smooth first soc to avoid soc jump in startup step
  Input:           struct smartstar_coul_device *di   ---- coul device
  Output:          NULL
  Return:          NULL
********************************************************/
static void coul_smooth_startup_soc(struct smartstar_coul_device *di)
{
    bool  flag_soc_valid = FALSE;
    short soc_temp = 0;

    if(NULL == di) {
        hwlog_err("NULL point in [%s]\n", __func__);
        return;
    }

    if (di->last_soc_enable) {
        di->coul_dev_ops->get_last_soc_flag(&flag_soc_valid);
        di->coul_dev_ops->get_last_soc(&soc_temp);
        hwlog_info("[%s]:flag = %d,di->batt_soc = %d,soc_temp = %d\n",__func__,flag_soc_valid,di->batt_soc,soc_temp);
        if ((flag_soc_valid) && abs(di->batt_soc - soc_temp) < di->startup_delta_soc) {
            di->batt_soc = soc_temp;
            hwlog_info("battery last soc= %d,flag = %d\n", soc_temp , flag_soc_valid);
        }
    }
    di->coul_dev_ops->clear_last_soc_flag();
    return;
}
static int iscd_sample_ocv_soc_uAh(struct smartstar_coul_device *di, int ocv_uv, long * const ocv_soc_uAh)
{
    int pc;
    s64 qmax;

    if (!di ||!ocv_soc_uAh) {
        hwlog_err("ISCD %s input para is null\n", __func__);
        return ERROR;
    }

    qmax = coul_get_qmax(di);
    pc = interpolate_pc_high_precision(di->batt_data->pc_temp_ocv_lut, di->batt_temp, ocv_uv / UVOLT_PER_MVOLT);

    hwlog_info("ISCD qmax = %llduAh, pc = %d/100000, ocv_soc = %llduAh\n",
                          qmax, pc, qmax*pc/(SOC_FULL*PERMILLAGE));
    *ocv_soc_uAh = (int)(qmax*pc/(SOC_FULL*PERMILLAGE));

    return (*ocv_soc_uAh > 0 ? SUCCESS : ERROR);
}
static int iscd_check_ocv_variance(int *ocv, int avg_ocv, int n)
{
    s64 var = 0;
    int detal_ocv;
    s64 detal_ocv_square;
    int i;

    if (!ocv ||!n) {
        hwlog_err("ISCD %s input para is null\n", __func__);
        return FALSE;
    }

    for (i = 0; i < n && ocv[i] > 0; i++) {
        detal_ocv = (int)(avg_ocv - ocv[i]);
        detal_ocv_square = (s64)((s64)detal_ocv * (s64)detal_ocv);
        var  =  (s64)(var + detal_ocv_square);
    }
    var /= n;
    hwlog_info("ISCD ocv variance is %lld uV*uV\n", var);
    if (var >= ISCD_OCV_UV_VAR_THREHOLD) {
        return FALSE;
    }

    return TRUE;
}
static int iscd_sample_battery_ocv_uv(struct smartstar_coul_device *di, int *ocv_uv)
{
    int i;
    unsigned int j;
    unsigned int fifo_depth;
    int current_ua = 0;
    int voltage_uv = 0;
    int total_vol = 0;
    int total_cur = 0;
    int used = 0;
    int *fifo_volt_uv;
    int ret = ERROR;

    if (!di ||!ocv_uv) {
        hwlog_err("ISCD %s input para is null\n", __func__);
        return ERROR;
    }

    fifo_depth = (unsigned int)di->coul_dev_ops->get_fifo_depth();
    fifo_volt_uv = (int *)kzalloc((size_t)(sizeof(int) *fifo_depth), GFP_KERNEL);
    if (NULL == fifo_volt_uv) {
        hwlog_err("ISCD fifo_volt_uv alloc fail, try to next loop\n");
        return ERROR;
    }

    for (i = 0; i < ISCD_SAMPLE_RETYR_CNT; i++) {
        total_vol = 0;
        used = 0;
        for (j = 0; j < fifo_depth; j++) {
            fifo_volt_uv[j] = 0;
            current_ua = di->coul_dev_ops->get_battery_cur_ua_from_fifo(j);
            voltage_uv = di->coul_dev_ops->get_battery_vol_uv_from_fifo(j);
            if (current_ua >= CURRENT_LIMIT
                || current_ua < CHARGING_CURRENT_OFFSET) {
                hwlog_err("ISCD current invalid, value is %d uA\n", current_ua);
                continue;
            }
            if (voltage_uv >= ISCD_OCV_UV_MAX || voltage_uv <= ISCD_OCV_UV_MIN) {
                hwlog_err("ISCD invalid voltage = %d uV\n", voltage_uv);
                continue;
            }
            hwlog_info("ISCD valid current = %d uA, voltage = %duV\n", current_ua, voltage_uv);
            fifo_volt_uv[j] = voltage_uv;
            total_cur += current_ua;
            total_vol += voltage_uv;
            used++;
        }

        hwlog_info("ISCD used = %d, total_vol = %d\n", used, total_vol);
        if (used >= ISCD_OCV_FIFO_VALID_CNT)
        {
            *ocv_uv = total_vol / used;
            hwlog_info("ISCD avg_voltage_uv = %d\n", *ocv_uv);
            if (TRUE == iscd_check_ocv_variance(fifo_volt_uv, *ocv_uv, fifo_depth)) {
                current_ua = total_cur/used;
                *ocv_uv += (current_ua/MOHM_PER_OHM)*
                    (di->r_pcb/UOHM_PER_MOHM + DEFAULT_BATTERY_OHMIC_RESISTANCE);
                ret = SUCCESS;
                break;
            } else {
                hwlog_err("ISCD variance sample ocv is out of range(0, %d) \n", ISCD_OCV_UV_VAR_THREHOLD);
            }
        }
    }

    if (NULL != fifo_volt_uv) {
        kfree(fifo_volt_uv);
    }
    hwlog_info("ISCD sampled ocv is %duV\n", *ocv_uv);
    return ret;
}
static int iscd_sample_battery_info
                                    (struct smartstar_coul_device *di, struct iscd_sample_info *sample_info)
{
    int ret;
    int ocv_uv = 0;
    int tbatt;
    long ocv_soc_uAh = 0;
    s64 cc_value;
    s64 delta_cc = 0;
    struct timespec sample_time;
    time_t delta_time = 0;

    if (!di ||!sample_info) {
        hwlog_info("ISCD iscd buffer is null\n");
        return ERROR;
    }

    if (CHARGING_STATE_CHARGE_DONE != di->charging_state) {
        di->iscd->last_sample_cnt = 0;
        hwlog_err("ISCD  charge_state is %d, try to next loop\n", di->charging_state);
        return ERROR;
    }
    tbatt = di->batt_temp;
    if (tbatt > di->iscd->tbatt_max || tbatt < di->iscd->tbatt_min) {
        hwlog_err("ISCD battery temperature is %d, out of range [%d, %d]",
                              tbatt, di->iscd->tbatt_min, di->iscd->tbatt_max);
        return ERROR;
    }
    sample_time = current_kernel_time();
    cc_value = di->iscd->full_update_cc + di->coul_dev_ops->calculate_cc_uah();
    if (di->iscd->size > 0) {
        delta_cc = di->iscd->sample_info[di->iscd->size-1].cc_value - cc_value;
        delta_time = sample_time.tv_sec -di->iscd->sample_info[di->iscd->size-1].sample_time.tv_sec;
        if ((delta_time < (time_t)ISCD_CALC_INTERVAL_900S) &&
             (delta_cc >= ISCD_RECHARGE_CC)) {
            di->iscd->last_sample_cnt = 0;
            hwlog_err("ISCD delta_time(%ld) < %d, delta_cc(%lld) >= %d, try to next loop\n",
                              delta_time, ISCD_CALC_INTERVAL_900S,delta_cc, ISCD_RECHARGE_CC);
            return ERROR;
        }
    }
    ret = iscd_sample_battery_ocv_uv(di, &ocv_uv);
    if (SUCCESS != ret) {
        hwlog_err("ISCD sample ocv wrong(%dmV), try to next loop\n", ocv_uv);
        return ERROR;
    }
    ret = iscd_sample_ocv_soc_uAh(di, ocv_uv, &ocv_soc_uAh);
    if (SUCCESS != ret) {
        hwlog_err("ISCD sample ocv_capacity wrong,  try to next loop\n");
        return ERROR;
    }

    di->iscd->last_sample_cnt ++;
    sample_info->sample_cnt = di->iscd->last_sample_cnt;
    sample_info->sample_time = sample_time;
    sample_info->tbatt = tbatt;
    sample_info->ocv_volt_uv = ocv_uv;
    sample_info->cc_value = cc_value;
    sample_info->ocv_soc_uAh = ocv_soc_uAh;
    di->iscd->last_sample_time = sample_info->sample_time;
    hwlog_info("ISCD sampled info: sample_cnt = %d, time_s = %ld, tbatt = %d, ocv_uV = %d, cc_uAh = %lld, ocv_soc_uAh = %lld\n",
                          sample_info->sample_cnt, sample_info->sample_time.tv_sec, sample_info->tbatt/TENTH, sample_info->ocv_volt_uv,
                          sample_info->cc_value, sample_info->ocv_soc_uAh);

    return SUCCESS;
}
static void iscd_remove_sampled_info(struct smartstar_coul_device *di, int from, int to)
{
    int i, j;

    if (!di ||from < 0||to < 0) {
        hwlog_err("ISCD %s input para error\n", __func__);
        return;
    }
    if (from > to) {
        swap(from, to);
    }

    hwlog_info("iscd remove sampled info from index %d to index %d\n", from, to);
    for(i = to + 1, j = 0; i < di->iscd->size && i >= 0 && from + j >= 0; i++, j++) {
        di->iscd->sample_info[(int)(from+j)].sample_time = di->iscd->sample_info[i].sample_time; //pclint 679
        di->iscd->sample_info[(int)(from+j)].tbatt = di->iscd->sample_info[i].tbatt;
        di->iscd->sample_info[(int)(from+j)].ocv_volt_uv = di->iscd->sample_info[i].ocv_volt_uv;
        di->iscd->sample_info[(int)(from+j)].cc_value = di->iscd->sample_info[i].cc_value;
        di->iscd->sample_info[(int)(from+j)].ocv_soc_uAh = di->iscd->sample_info[i].ocv_soc_uAh;
    }
    di->iscd->size -= ((to -from) + 1);
}

static void iscd_reset_isc_buffer(struct smartstar_coul_device *di)
{
    int i;

    if (!di) {
        hwlog_err("ISCD %s di is null\n", __func__);
        return;
    }

    di->iscd->isc_buff[0] = 0;
    for (i = 1; i < ISCD_ISC_MAX_SIZE; i++) {
            di->iscd->isc_buff[i] = INVALID_ISC;
    }
}
static void iscd_clear_sampled_info(struct smartstar_coul_device *di)
{
    if (!di) {
        hwlog_err("ISCD %s di is null\n", __func__);
        return;
    }

    hwlog_info("ISCD clear sampled info, size = %d\n", di->iscd->size);
    if (!di->iscd->size) {
        hwlog_err("ISCD sampled info is already empty.\n");
        return;
    }
    iscd_remove_sampled_info(di, 0, di->iscd->size-1);
}
static void iscd_append_sampled_info(struct smartstar_coul_device *di, struct iscd_sample_info *sample_info)
{
    if (!di ||!di->iscd||!sample_info) {
        hwlog_err("ISCD %s input para is null\n", __func__);
        return;
    }

    if (di->iscd->size < ISCD_SMAPLE_LEN_MAX) {
        di->iscd->sample_info[di->iscd->size].sample_cnt = sample_info->sample_cnt;
        di->iscd->sample_info[di->iscd->size].sample_time = sample_info->sample_time;
        di->iscd->sample_info[di->iscd->size].tbatt = sample_info->tbatt;
        di->iscd->sample_info[di->iscd->size].ocv_volt_uv = sample_info->ocv_volt_uv;
        di->iscd->sample_info[di->iscd->size].cc_value = sample_info->cc_value;
        di->iscd->sample_info[di->iscd->size].ocv_soc_uAh = sample_info->ocv_soc_uAh;
        di->iscd->size += 1;
        return;
    }
}
static void iscd_insert_sampled_info(struct smartstar_coul_device *di, struct iscd_sample_info *sample_info)
{
    if (!di ||!di->iscd||!sample_info) {
        hwlog_err("ISCD %s input para is null\n", __func__);
        return;
    }

    if (di->iscd->size >= ISCD_SMAPLE_LEN_MAX) {
        hwlog_info("ISCD sample size is %d, remove one from list.\n", di->iscd->size);
        iscd_remove_sampled_info(di, 0, 0);
    }
    if (di->iscd->size && sample_info->sample_cnt > ISCD_INVALID_SAMPLE_CNT_FROM &&
        sample_info->sample_cnt <= ISCD_INVALID_SAMPLE_CNT_TO + 1) {
        hwlog_info("ISCD sample size is %d, remove one from list.\n", di->iscd->size);
        iscd_remove_sampled_info(di, di->iscd->size -1, di->iscd->size -1);
    }
    iscd_append_sampled_info(di, sample_info);
}
static int iscd_check_ocv_abrupt_change(struct smartstar_coul_device *di)
{
    int ret = ISCD_VALID;
    int size;
    time_t delta_time1,delta_time2;
    int delta_ocv1, delta_ocv2;
    s64 delta_cc1, delta_cc2;

    if (!di) {
        hwlog_err("ISCD %s di is null\n", __func__);
        return ISCD_INVALID;
    }

    size = di->iscd->size;
    if (size >= 3) { //linear compare needs at least 3 sample
        delta_time1 = di->iscd->sample_info[size-2].sample_time.tv_sec - di->iscd->sample_info[size-3].sample_time.tv_sec;
        delta_time2 = di->iscd->sample_info[size-1].sample_time.tv_sec - di->iscd->sample_info[size-2].sample_time.tv_sec;
        delta_cc1 = di->iscd->sample_info[size-2].cc_value- di->iscd->sample_info[size-3].cc_value;
        delta_cc2 = di->iscd->sample_info[size-1].cc_value- di->iscd->sample_info[size-2].cc_value;
        delta_ocv1 = di->iscd->sample_info[size-2].ocv_volt_uv- di->iscd->sample_info[size-3].ocv_volt_uv;
        delta_ocv2 = di->iscd->sample_info[size-1].ocv_volt_uv- di->iscd->sample_info[size-2].ocv_volt_uv;
        if (abs(delta_ocv2) >= ISCD_OCV_DELTA_MAX/HALF && abs(delta_ocv1) < ISCD_OCV_DELTA_MAX/HALF &&
             abs(delta_time2) < ISCD_CALC_INTERVAL_900S && abs(delta_time1) < ISCD_CALC_INTERVAL_900S &&
             abs(delta_cc2) < ISCD_RECHARGE_CC && abs(delta_cc1) < ISCD_RECHARGE_CC) {
             ret = ISCD_INVALID;
            hwlog_err("ISCD %s the last OCV invalid: ocv %d->%d->%d uV \n",
                                 __func__, di->iscd->sample_info[size-3].ocv_volt_uv, di->iscd->sample_info[size-2].ocv_volt_uv,
                                 di->iscd->sample_info[size-1].ocv_volt_uv);
        }
    }

    return ret;
}
static int iscd_remove_invalid_samples(struct smartstar_coul_device *di)
{
    int ret = FALSE;
    time_t delta_time = 0;
    int i, size;

    if (!di ||di->iscd->size <= 0) {
        hwlog_err("ISCD %s input para is null\n", __func__);
        return FALSE;
    }

    size = di->iscd->size;
    /*clear all samples when the lasted OCV is below designed ocv_min,typically 4V*/
    if (di->iscd->sample_info[size -1].ocv_volt_uv <= di->iscd->ocv_min) {
        hwlog_err("ISCD ocv(%duV) below %duV, clear sapmpled info.\n",
                             di->iscd->sample_info[size -1].ocv_volt_uv, di->iscd->ocv_min);
        iscd_clear_sampled_info(di);
        iscd_reset_isc_buffer(di);
        di->iscd->rm_bcd = 0;
        di->iscd->fcc_bcd = 0;
        return TRUE;
    }

    /*remove the samples whose delta time with the lasted sample is more than ISCD_SAMPLE_INTERVAL_MAX*/
    for (i = size -1; i >= 0 ; i--) {
        delta_time = di->iscd->sample_info[size -1].sample_time.tv_sec -di->iscd->sample_info[i].sample_time.tv_sec;
        if(delta_time >= ISCD_SAMPLE_INTERVAL_MAX) {
            hwlog_err("ISCD sample_time = %lds, sample[%d]_time = %lds, delta_time %ld >= %ds\n",
                                di->iscd->sample_info[size -1].sample_time.tv_sec, i, di->iscd->sample_info[i].sample_time.tv_sec,
                                delta_time, ISCD_SAMPLE_INTERVAL_MAX);
            iscd_remove_sampled_info(di, 0, i);
            iscd_reset_isc_buffer(di);
            ret = TRUE;
            break;
        }
    }

    if (ISCD_INVALID == iscd_check_ocv_abrupt_change(di)) {
        hwlog_err("ISCD the latest OCV is invalid, remove it from list.\n");
        iscd_remove_sampled_info(di, size -1, size -1);
        iscd_reset_isc_buffer(di);
        ret = TRUE;
    }

    return ret;
}
static int iscd_calc_isc_by_two_samples(struct smartstar_coul_device *di, int index0, int index1)
{
    int delta_tbatt = 0;
    int delta_tbatt_abs;
    int delta_ocv = 0;
    int delta_ocv_abs;
    s64 delta_cc = 0;
    s64 delta_cc_abs;
    s64 delta_ocv_soc_uAh = 0;
    time_t delta_time = 0;
    time_t delta_time_abs;
    int isc = 0;

    if (!di ||index0 < 0||index1 < 0) {
        hwlog_err("ISCD %s input para is null\n", __func__);
        return INVALID_ISC;
    }

    if (index0 > index1) {
        swap(index0, index1);
    }

    if (index1 >= 0 && index0 >= 0) { //pclint 676
        delta_time = di->iscd->sample_info[index1].sample_time.tv_sec - di->iscd->sample_info[index0].sample_time.tv_sec;
        delta_tbatt = di->iscd->sample_info[index1].tbatt - di->iscd->sample_info[index0].tbatt;
        delta_ocv = di->iscd->sample_info[index0].ocv_volt_uv- di->iscd->sample_info[index1].ocv_volt_uv;
        delta_ocv_soc_uAh = di->iscd->sample_info[index0].ocv_soc_uAh- di->iscd->sample_info[index1].ocv_soc_uAh;
        delta_cc = di->iscd->sample_info[index1].cc_value- di->iscd->sample_info[index0].cc_value;
        hwlog_info("ISCD calc isc by sample s%d s%d, delta_time(s2-s1) = %lds, delta_tbatt(s2-s1) = %d/10, "
                           "delta_ocv(s1-s2) = %duV, delta_ocv_soc_uAh(s1-s2) = %llduAh, delta_cc(s2-s1) = %llduAh \n",
                           index0, index1, delta_time, delta_tbatt, delta_ocv, delta_ocv_soc_uAh, delta_cc);
    }
    delta_time_abs = delta_time >= 0 ? delta_time : -delta_time;
    delta_tbatt_abs = delta_tbatt >= 0 ? delta_tbatt : -delta_tbatt;
    delta_ocv_abs = delta_ocv >= 0 ? delta_ocv : -delta_ocv;
    delta_cc_abs = delta_cc >= 0 ? delta_cc : -delta_cc;
    if ((delta_tbatt_abs <= di->iscd->tbatt_diff_max) &&
         ((delta_time_abs >= (time_t)di->iscd->calc_time_interval_min)
            ||((delta_ocv_abs >= ISCD_OCV_DELTA_MAX) &&
                  ((int)delta_cc_abs <= ISCD_CC_DELTA_MAX) &&
                  (delta_time_abs >= (time_t)ISCD_CALC_INTERVAL_900S)))
    ) {
        if (delta_time > 0) {
            isc = ((int)(delta_ocv_soc_uAh - delta_cc)) * SEC_PER_HOUR /(int)delta_time;
            hwlog_info("ISCD isc calc by sample %d %d is %d\n", index0, index1, isc);
        }
        if (isc < CHARGING_CURRENT_OFFSET) {
            hwlog_err("ISCD isc calc by sample s%d s%d is invalid(%d), discard it.\n", index0, index1, isc);
            isc = INVALID_ISC;
        }
        return isc;
    }
    return INVALID_ISC;
}
static void iscd_push_isc_to_isc_buf(struct smartstar_coul_device *di, int isc_tmp)
{
    static int index = 1;

    if (!di) {
        hwlog_err("ISCD %s di is null\n", __func__);
        return;
    }

    di->iscd->isc_buff[index++] = isc_tmp;
    di->iscd->isc_buff[0] += 1;
    if (di->iscd->isc_buff[0] >= ISCD_ISC_MAX_SIZE ) {
        di->iscd->isc_buff[0] = ISCD_ISC_MAX_SIZE - 1;
    }
    if (index >= ISCD_ISC_MAX_SIZE) {
        index = 1;
    }
}
static int avg_isc_by_threhold(struct smartstar_coul_device *di, int lower, int upper, int percent)
{
    s64 sum = 0;
    int cnt = 0;
    int isc_size = 0;
    int avg = INVALID_ISC;
    int i;

    if (!di) {
        hwlog_err("ISCD %s di is null\n", __func__);
        return INVALID_ISC;
    }

    isc_size = di->iscd->isc_buff[0];
    for (i = 1; i < ISCD_ISC_MAX_SIZE;  i++) {
        if (di->iscd->isc_buff[i] > lower && di->iscd->isc_buff[i] < upper) {
            sum += di->iscd->isc_buff[i];
            cnt++;
        }
    }
    hwlog_info("ISCD %s isc_size: %d, cnt: %d within threhold:(%d, %d)\n",
                          __func__, isc_size, cnt, lower, upper);
    if (cnt > 0 && cnt > isc_size* percent/PERCENT) {
        avg = (int)(sum/cnt);
    }

    return avg;
}

static int iscd_standard_deviation_of_isc(struct smartstar_coul_device *di,  int avg_isc)
{
    int i;
    s64 var = 0;
    int cnt = 0;
    int detal_isc;
    s64 detal_isc_square;

    if (!di) {
        hwlog_err("ISCD %s di is null\n", __func__);
        return INVALID_ISC;
    }

    for (i = 1; i < ISCD_ISC_MAX_SIZE; i++) {
        if (INVALID_ISC != di->iscd->isc_buff[i]) {
            detal_isc = (int)(avg_isc - di->iscd->isc_buff[i]);
            detal_isc_square = (s64)((s64)detal_isc * (s64)detal_isc);
            var += detal_isc_square;
            cnt ++;
        }
    }
    if (cnt > 0) {
        var /= cnt;
        hwlog_info("ISCD %s variance of isc: %lld, cnt: %d\n", __func__, var, cnt);
    }

    return (int)int_sqrt((unsigned long)var);
}

static int iscd_is_short_current_valid(struct smartstar_coul_device *di)
{
    int sample_size;
    int avg_isc = INVALID_ISC;
    int isc_buff_size;
    int sigma_isc;
    time_t sample_time;
    int chrg_cycle;
    int i;
    s64 sum_cnt = 0;
    int avg_cnt = 0;

    if (!di  || di->iscd->size <= 0) {
        hwlog_err("ISCD %s input para is error\n", __func__);
        return ISCD_INVALID;
    }

    chrg_cycle = coul_battery_cycle_count();
    if (chrg_cycle <= ISCD_CHARGE_CYCLE_MIN) {
        hwlog_err("ISCD %s charge_cycle(%d) is less than %d, try to next loop.\n",
                          __func__, chrg_cycle, ISCD_CHARGE_CYCLE_MIN);
       return ISCD_INVALID;
    }

    sample_size = di->iscd->size;
    sample_time = di->iscd->sample_info[sample_size-1].sample_time.tv_sec - di->iscd->sample_info[0].sample_time.tv_sec;

    if (sample_time < ISCD_SMAPLE_TIME_MIN) {
        hwlog_err("ISCD %s sample time(%lds) is less than %ds, try to next loop.\n",
                            __func__, sample_time, ISCD_SMAPLE_TIME_MIN);
        return ISCD_INVALID;
    }

    isc_buff_size = di->iscd->isc_buff[0];
    avg_isc = avg_isc_by_threhold(di, -INVALID_ISC, INVALID_ISC, 0);
    hwlog_info("ISCD %s isc_buff_size: %d, primary avg_isc: %duAh\n", __func__, isc_buff_size, avg_isc);
    if (avg_isc < ISCD_LARGE_ISC_THREHOLD) {
        for (i = 0; i < sample_size; i++) {
            sum_cnt += di->iscd->sample_info[i].sample_cnt;
        }
        avg_cnt = (int)(sum_cnt/sample_size);
        if (isc_buff_size >= ISCD_SMALL_ISC_VALID_SIZE_1 ||
            (isc_buff_size >= ISCD_SMALL_ISC_VALID_SIZE_2 &&
            avg_cnt < ISCD_INVALID_SAMPLE_CNT_TO +1)) {
            sigma_isc = iscd_standard_deviation_of_isc(di, avg_isc);
            avg_isc = avg_isc_by_threhold(di, avg_isc -sigma_isc, avg_isc + sigma_isc, ISCD_SMALL_ISC_VALID_PERCENT);
            hwlog_info("ISCD %s standard deviation of isc: %d, final avg_isc: %duAh\n", __func__, sigma_isc, avg_isc);
            di->iscd->isc = avg_isc;
            return (INVALID_ISC == avg_isc) ? ISCD_INVALID : ISCD_VALID;
        }
    } else if (avg_isc >= ISCD_LARGE_ISC_THREHOLD && isc_buff_size >= ISCD_LARGE_ISC_VALID_SIZE) {
        avg_isc = avg_isc_by_threhold(di, ISCD_LARGE_ISC_THREHOLD,
                                                INVALID_ISC, ISCD_LARGE_ISC_VALID_PERCENT);
        hwlog_info("ISCD %s final avg_isc: %duAh\n", __func__, avg_isc);
        di->iscd->isc = avg_isc;
        return (INVALID_ISC == avg_isc) ? ISCD_INVALID : ISCD_VALID;
    } else {
        /*do nothing*/
    }

    return ISCD_INVALID;
}
static void iscd_calc_isc_with_prev_samples(struct smartstar_coul_device *di, int index)
{
    int i, isc_tmp;

    if (!di ||index >= di->iscd->size) {
        hwlog_err("ISCD %s para is error, index = %d\n", __func__, index);
        return;
    }
    if (index == 0) {
        hwlog_err("ISCD %s samples is not enough, try to next loop \n", __func__);
        return;
    }

    for (i = 0; i < index; i++) {
        isc_tmp = iscd_calc_isc_by_two_samples(di, i, index);
        if (INVALID_ISC != isc_tmp) {
            iscd_push_isc_to_isc_buf(di, isc_tmp);
        }
    }
}
static void iscd_calc_isc_by_all_samples(struct smartstar_coul_device *di)
{
    int i;

    if (!di ||di->iscd->size <= 0) {
        hwlog_err("ISCD %s input para is null\n", __func__);
        return;
    }

    for (i = 1; i < di->iscd->size; i++) {
        iscd_calc_isc_with_prev_samples(di, i);
    }
}
void iscd_dump_dsm_info(struct smartstar_coul_device *di, char *buf)
{
    int i;
    char tmp_buf[ISCD_ERR_NO_STR_SIZE] = { 0 };

    if (!di || !buf) {
        hwlog_err("ISCD %s input para is null\n", __func__);
        return;
    }
    hwlog_info("ISCD %s ++\n", __func__);
    di->qmax = coul_get_qmax(di);
    snprintf(tmp_buf, (size_t)ISCD_ERR_NO_STR_SIZE, "battery is %s, charge_cycles is %d, "
        "rm is %dmAh %dmAh, fcc is %dmAh %dmAh, Qmax is %dmAh\n",
        di->batt_data->batt_brand, di->batt_chargecycles /PERCENT,
        di->iscd->rm_bcd/UA_PER_MA, di->batt_ruc/UA_PER_MA,
        di->iscd->fcc_bcd/UA_PER_MA, di->batt_fcc/UA_PER_MA,
        di->qmax/UA_PER_MA);
    strncat(buf, tmp_buf, strlen(tmp_buf));
    for (i = 0; i < MAX_RECORDS_CNT; i++) {
        snprintf(tmp_buf, (size_t)ISCD_ERR_NO_STR_SIZE, "BASP fcc[%d] is %d mAh\n", i, my_nv_info.real_fcc_record[i]);
        strncat(buf, tmp_buf, strlen(tmp_buf));
    }
    snprintf(tmp_buf, (size_t)ISCD_ERR_NO_STR_SIZE, "isc is %d uA\n", di->iscd->isc);
    strncat(buf, tmp_buf, strlen(tmp_buf));
    snprintf(tmp_buf, (size_t)ISCD_ERR_NO_STR_SIZE, "sample_info:\n");
    strncat(buf, tmp_buf, strlen(tmp_buf));
    snprintf(tmp_buf, (size_t)ISCD_ERR_NO_STR_SIZE, "id time  tbatt ocv     ocv_rm/uAh cc/uAh  cnt\n");
    strncat(buf, tmp_buf, strlen(tmp_buf));

    for (i = 0; i < di->iscd->size; i++) {
        snprintf(tmp_buf, (size_t)ISCD_ERR_NO_STR_SIZE, "%-2d %-5ld %-5d %-7d %-10lld %-7lld %-3d\n",
                    i,di->iscd->sample_info[i].sample_time.tv_sec -di->iscd->sample_info[0].sample_time.tv_sec,
                    di->iscd->sample_info[i].tbatt/TENTH, di->iscd->sample_info[i].ocv_volt_uv,
                    di->iscd->sample_info[i].ocv_soc_uAh, di->iscd->sample_info[i].cc_value,
                    di->iscd->sample_info[i].sample_cnt);
        strncat(buf, tmp_buf, strlen(tmp_buf));
    }
    hwlog_info("ISCD %s --\n", __func__);
}
static int iscd_dsm_report(struct smartstar_coul_device *di, int level)
{
    int ret = SUCCESS;
    struct timespec now = current_kernel_time();

    if (!di || level >= ISCD_MAX_LEVEL) {
        hwlog_err("ISCD %s input para error\n", __func__);
        return ERROR;
    }

    if (di->iscd->level_config[level].dsm_report_cnt < ISCD_DSM_REPORT_CNT_MAX) {
        if (!di->iscd->level_config[level].dsm_report_time ||
            (now.tv_sec - di->iscd->level_config[level].dsm_report_time >= ISCD_DSM_REPORT_INTERVAL)) {
            iscd_dump_dsm_info(di, dsm_buff);
            ret = dsm_report(di->iscd->level_config[level].dsm_err_no, dsm_buff);
            if (SUCCESS == ret) {
                di->iscd->level_config[level].dsm_report_cnt++;
                di->iscd->level_config[level].dsm_report_time = now.tv_sec;
            }
            memset(&dsm_buff, (unsigned)0, sizeof(dsm_buff));
        }
    }

    return ret;
}
static int iscd_protection(struct smartstar_coul_device *di, int level)
{
    if (!di || level >= ISCD_MAX_LEVEL) {
        hwlog_err("ISCD %s input para error\n", __func__);
        return ERROR;
    }
    return SUCCESS;
}
static int iscd_process_short_current(struct smartstar_coul_device *di)
{
    int ret = SUCCESS;
    int i;

    if (!di) {
        hwlog_err("ISCD %s di is null\n", __func__);
        return ERROR;
    }
    hwlog_info("%s ++\n", __func__);
    for (i = 0; i < di->iscd->total_level; i++) {
        if (di->iscd->isc >= di->iscd->level_config[i].isc_min &&
            di->iscd->isc < di->iscd->level_config[i].isc_max) {
            hwlog_info("ISCD isc: %duA,  level: %d, threhold: [%d, %d)uA\n", di->iscd->isc, i,
                     di->iscd->level_config[i].isc_min, di->iscd->level_config[i].isc_max);
            ret |= iscd_dsm_report(di, i);
            ret |= iscd_protection(di, i);
            break;
        }
    }
    hwlog_info("%s --\n", __func__);

    return ret;
}
static int iscd_calc_short_current(struct smartstar_coul_device *di, int rm_flag)
{
    int sample_size;

    if (!di) {
        hwlog_err("ISCD %s di is null\n", __func__);
        return FALSE;
    }

    sample_size = di->iscd->size;
    if (sample_size < 2) {  //isc must be calculated at least with 2 samples
        hwlog_err("ISCD %s sample size is %d\n, try to next calc.\n", __func__, sample_size);
        return FALSE;
    }

    if (TRUE == rm_flag) {
        if (di->iscd->sample_info[sample_size-1].sample_cnt > ISCD_INVALID_SAMPLE_CNT_TO) {
            iscd_calc_isc_by_all_samples(di);
            hwlog_info("some invalid samples has been removed, calc short current with all samples.\n");
            return TRUE;
        } else {
            return FALSE;
        }
    }

    if (di->iscd->sample_info[sample_size-1].sample_cnt > ISCD_INVALID_SAMPLE_CNT_TO ||
        di->iscd->sample_info[sample_size-1].sample_cnt == ISCD_STANDBY_SAMPLE_CNT) {
        //calc the last sample with its prev samples
        iscd_calc_isc_with_prev_samples(di, sample_size-1);
        return TRUE;
    } else if (di->iscd->sample_info[sample_size-1].sample_cnt > ISCD_INVALID_SAMPLE_CNT_FROM &&
                   di->iscd->sample_info[sample_size-1].sample_cnt <= ISCD_INVALID_SAMPLE_CNT_TO) {
        hwlog_info("ISCD %s cnt is %d, do nothing\n",
                              __func__, di->iscd->sample_info[sample_size-1].sample_cnt);
        return FALSE;
    } else if (di->iscd->sample_info[sample_size-1].sample_cnt == ISCD_INVALID_SAMPLE_CNT_FROM) {
        if (di->iscd->sample_info[sample_size-2].sample_cnt > ISCD_INVALID_SAMPLE_CNT_TO ||
            di->iscd->sample_info[sample_size-2].sample_cnt == ISCD_STANDBY_SAMPLE_CNT) {
            hwlog_info("ISCD %s this cnt is %d, but prev cnt is %d, do nothing\n",
                              __func__, di->iscd->sample_info[sample_size-1].sample_cnt,
                              di->iscd->sample_info[sample_size-2].sample_cnt);
            return FALSE;
        } else {
            //calc the penultimate sample with its prev samples
            iscd_calc_isc_with_prev_samples(di, sample_size-2);
            return TRUE;
        }
    } else {
        return FALSE;
    }
}
static void iscd_timer_start(struct smartstar_coul_device *di, time_t delta_secs)
{
    ktime_t kt;

    if (!di) {
        hwlog_info("ISCD %s di is null.\n", __func__);
        return;
    }

    kt = ktime_set((s64)delta_secs, (unsigned long)0);
    hrtimer_start(&di->iscd->timer, kt, HRTIMER_MODE_REL);
}
static void check_batt_critical_electric_leakage(struct smartstar_coul_device *di)
{
    if (!di) {
        hwlog_info("ISCD %s di is null.\n", __func__);
        return;
    }

    if (ENABLED == di->iscd->enable) {
        if ((di->batt_fcc/UA_PER_MA >= (int)di->batt_data->fcc/HALF) &&
            (di->batt_ruc * PERCENT >= di->batt_fcc * FCC_MAX_PERCENT ||
            di->batt_fcc/UA_PER_MA * PERCENT >= (int)di->batt_data->fcc*FCC_MAX_PERCENT)) {
            di->iscd->isc = ISCD_FITAL_LEVEL_THREHOLD;
            hwlog_err("ISCD rm = %d, fcc = %d, set internal short current to %dmA.\n",
                            di->batt_ruc, di->batt_fcc, di->iscd->isc/UA_PER_MA);
            iscd_process_short_current(di);
         }
    }
}
static void iscd_work(struct work_struct *work)
{
    int ret = 0;
    struct smartstar_coul_device *di = g_smartstar_coul_dev;
    struct iscd_sample_info *sample_info;
    hwlog_info("ISCD %s ++\n", __func__);

    if (!di) {
        hwlog_info("ISCD %s di is null.\n", __func__);
        goto FuncEnd;
    }
    (void)work;
    sample_info = kzalloc(sizeof(*sample_info), GFP_KERNEL);
    if (NULL == sample_info) {
        hwlog_err("ISCD sample_info alloc fail, try to next loop\n");
        goto FuncEnd;
    }

    if (CHARGING_STATE_CHARGE_STOP == di->charging_state) {
        hrtimer_cancel(&di->iscd->timer);
    } else {
        ret = iscd_sample_battery_info(di, sample_info);
        if (SUCCESS == ret) {
            iscd_insert_sampled_info(di, sample_info);
            ret = iscd_remove_invalid_samples(di);
            ret = iscd_calc_short_current(di, ret);
            if (TRUE == ret) {
                ret = iscd_is_short_current_valid(di);
                if (ISCD_VALID == ret)
                    iscd_process_short_current(di);
            }
            iscd_timer_start(di, (time_t)di->iscd->sample_time_interval);
        } else {
            iscd_timer_start(di, (time_t)di->iscd->sample_time_interval/QUARTER);
        }
    }
    kfree(sample_info);
FuncEnd:
    coul_wake_unlock();

    hwlog_info("ISCD %s --\n", __func__);
}
static enum hrtimer_restart iscd_timer_func(struct hrtimer *timer)
{
    struct smartstar_coul_device *di = g_smartstar_coul_dev;
    struct timespec now = current_kernel_time();
    time_t delta_time;

    if (!di) {
        hwlog_info("ISCD %s di is null.\n", __func__);
        return HRTIMER_NORESTART;
    }

    (void)timer;
    delta_time = now.tv_sec - di->iscd->last_sample_time.tv_sec;
    hwlog_info("ISCD delta time is %lds\n", delta_time);
    if (delta_time >= di->iscd->sample_time_interval - ISCD_SAMPLE_INTERVAL_DELTA) {
        hwlog_info("ISCD %s ++\n", __func__);
        coul_wake_lock();
        schedule_delayed_work(&di->iscd->delayed_work,
                msecs_to_jiffies((unsigned int)(di->iscd->sample_time_delay*MSEC_PER_SEC)));  //delay for battery stability
        hwlog_info("ISCD %s --\n", __func__);
    }

    return HRTIMER_NORESTART;
}

#ifdef CONFIG_SYSFS

static int do_save_offset_ret = 0;

enum coul_sysfs_type{
    COUL_SYSFS_PL_CALIBRATION_EN = 0,
    COUL_SYSFS_PL_V_OFFSET_A,
    COUL_SYSFS_PL_V_OFFSET_B,
    COUL_SYSFS_PL_C_OFFSET_A,
    COUL_SYSFS_PL_C_OFFSET_B,
    COUL_SYSFS_ATE_V_OFFSET_A,
    COUL_SYSFS_ATE_V_OFFSET_B,
    COUL_SYSFS_DO_SAVE_OFFSET,
    COUL_SYSFS_GAUGELOG_HEAD,
    COUL_SYSFS_GAUGELOG,
    COUL_SYSFS_HAND_CHG_CAPACITY_FLAG,
    COUL_SYSFS_INPUT_CAPACITY,
    COUL_SYSFS_ABS_CC,
    COUL_SYSFS_BATTERY_ID_VOLTAGE,
    COUL_SYSFS_BATTERY_BRAND_NAME,
    COUL_SYSFS_RBATT,
    COUL_SYSFS_REAL_SOC,
    COUL_SYSFS_CALI_ADC,
};

#define COUL_SYSFS_FIELD(_name, n, m, store)                \
{                                                   \
    .attr = __ATTR(_name, m, coul_sysfs_show, store),    \
    .name = COUL_SYSFS_##n,          \
}

#define COUL_SYSFS_FIELD_RW(_name, n)               \
        COUL_SYSFS_FIELD(_name, n, S_IWUSR | S_IRUGO,       \
                coul_sysfs_store)

#define COUL_SYSFS_FIELD_RO(_name, n)               \
        COUL_SYSFS_FIELD(_name, n, S_IRUGO, NULL)

static ssize_t coul_sysfs_show(struct device *dev,
        struct device_attribute *attr, char *buf);
static ssize_t coul_sysfs_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count);

struct coul_sysfs_field_info {
    struct device_attribute	attr;
    u8 name;
};
/*lint -e665*/
static struct coul_sysfs_field_info coul_sysfs_field_tbl[] = {
    COUL_SYSFS_FIELD_RW(pl_calibration_en,      PL_CALIBRATION_EN),
    COUL_SYSFS_FIELD_RW(pl_v_offset_a,          PL_V_OFFSET_A),
    COUL_SYSFS_FIELD_RW(pl_v_offset_b,          PL_V_OFFSET_B),
    COUL_SYSFS_FIELD_RW(pl_c_offset_a,          PL_C_OFFSET_A),
    COUL_SYSFS_FIELD_RW(pl_c_offset_b,          PL_C_OFFSET_B),
    COUL_SYSFS_FIELD_RO(ate_v_offset_a,         ATE_V_OFFSET_A),
    COUL_SYSFS_FIELD_RO(ate_v_offset_b,         ATE_V_OFFSET_B),
    COUL_SYSFS_FIELD_RW(do_save_offset,         DO_SAVE_OFFSET),
    COUL_SYSFS_FIELD_RO(gaugelog,               GAUGELOG),
    COUL_SYSFS_FIELD_RO(gaugelog_head,          GAUGELOG_HEAD),
    COUL_SYSFS_FIELD_RW(hand_chg_capacity_flag, HAND_CHG_CAPACITY_FLAG),
    COUL_SYSFS_FIELD_RW(input_capacity,         INPUT_CAPACITY),
    COUL_SYSFS_FIELD_RO(abs_cc,                 ABS_CC),
    COUL_SYSFS_FIELD_RO(battery_id_voltage,     BATTERY_ID_VOLTAGE),
    COUL_SYSFS_FIELD_RO(battery_brand_name,     BATTERY_BRAND_NAME),
    COUL_SYSFS_FIELD_RO(rbatt, RBATT),
    COUL_SYSFS_FIELD_RO(real_soc, REAL_SOC),
    COUL_SYSFS_FIELD_RW(cali_adc,         CALI_ADC),
};
/*lint +e665*/
static struct attribute *coul_sysfs_attrs[ARRAY_SIZE(coul_sysfs_field_tbl) + 1];

static const struct attribute_group coul_sysfs_attr_group = {
    .attrs = coul_sysfs_attrs,
};
/**********************************************************
*  Function:       coul_sysfs_init_attrs
*  Discription:    initialize coul_sysfs_attrs[] for coul attribute
*  Parameters:     NULL
*  return value:   NULL
**********************************************************/
static void coul_sysfs_init_attrs(void)
{
    int i, limit = ARRAY_SIZE(coul_sysfs_field_tbl);

    for (i = 0; i < limit; i++)
    {
        coul_sysfs_attrs[i] = &coul_sysfs_field_tbl[i].attr.attr;
    }
    coul_sysfs_attrs[limit] = NULL;
}
/**********************************************************
*  Function:       coul_sysfs_field_lookup
*  Discription:    get the current device_attribute from charge_sysfs_field_tbl by attr's name
*  Parameters:   name:device attribute name
*  return value:  coul_sysfs_field_tbl[]
**********************************************************/
static struct coul_sysfs_field_info *coul_sysfs_field_lookup(const char *name)
{
    int i, limit = ARRAY_SIZE(coul_sysfs_field_tbl);

    for (i = 0; i < limit; i++)
    {
        if (!strncmp(name, coul_sysfs_field_tbl[i].attr.attr.name,strlen(name)))
            break;
    }
    if (i >= limit)
        return NULL;

    return &coul_sysfs_field_tbl[i];
}
/**********************************************************
*  Function:       coul_sysfs_show
*  Discription:    show the value for all coul device's node
*  Parameters:     dev:device
*                      attr:device_attribute
*                      buf:string of node value
*  return value:  0-sucess or others-fail
**********************************************************/
static ssize_t coul_sysfs_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct coul_sysfs_field_info *info = NULL;
    struct smartstar_coul_device *di = dev_get_drvdata(dev);
    int val = 0;
    int temp = 0, voltage = 0, ufcapacity = 0, capacity = 100, afcapacity = 0, rm = 0, fcc = 0, delta_rc = 0;
    int cur = 0,uuc = 0,cc = 0, ocv=0, rbatt;
    unsigned int pd_charge = 0;

    info = coul_sysfs_field_lookup(attr->attr.name);
    if (!info)
        return -EINVAL;
    switch(info->name){
    case COUL_SYSFS_PL_CALIBRATION_EN:
        return snprintf(buf, PAGE_SIZE, "%d\n", pl_calibration_en);
    case COUL_SYSFS_PL_V_OFFSET_A:
        return snprintf(buf, PAGE_SIZE, "%d\n", v_offset_a);
    case COUL_SYSFS_PL_V_OFFSET_B:
        return snprintf(buf, PAGE_SIZE, "%d\n", v_offset_b);
    case COUL_SYSFS_PL_C_OFFSET_A:
        return snprintf(buf, PAGE_SIZE, "%d\n", c_offset_a);
    case COUL_SYSFS_PL_C_OFFSET_B:
        return snprintf(buf, PAGE_SIZE, "%d\n", c_offset_b);
    case COUL_SYSFS_ATE_V_OFFSET_A:
        return snprintf(buf, PAGE_SIZE, "%d\n", di->coul_dev_ops->get_ate_a());
    case COUL_SYSFS_ATE_V_OFFSET_B:
        return snprintf(buf, PAGE_SIZE, "%d\n", di->coul_dev_ops->get_ate_b());
    case COUL_SYSFS_DO_SAVE_OFFSET:
        return snprintf(buf, PAGE_SIZE, "%d\n", do_save_offset_ret);
    case COUL_SYSFS_GAUGELOG_HEAD:
        return snprintf(buf, PAGE_SIZE, "ss_VOL  ss_CUR  ss_ufSOC  ss_SOC  SOC  ss_RM  ss_FCC  ss_UUC  ss_CC  ss_dRC  pdFlag Temp  ss_OCV   rbatt  fcc    ocv_level  fcc_flag   ");
    case COUL_SYSFS_GAUGELOG:
        temp       = coul_get_battery_temperature();
        voltage    = coul_get_battery_voltage_mv();
        cur        = -(coul_get_battery_current_ma());
        ufcapacity = coul_battery_unfiltered_capacity();
        capacity   = coul_get_battery_capacity();
        afcapacity = hisi_bci_show_capacity();
        rm         = coul_get_battery_rm();
        fcc        = coul_get_battery_fcc();
        uuc        = coul_get_battery_uuc();
        cc         = coul_get_battery_cc();
        delta_rc   = coul_get_battery_delta_rc();
        ocv        = coul_get_battery_ocv();
        rbatt      = coul_get_battery_resistance();
        pd_charge  = get_pd_charge_flag();

        snprintf(buf, PAGE_SIZE, "%-6d  %-6d  %-8d  %-6d  %-3d  %-5d  %-6d  %-6d  %-5d  %-6d  %-5d  %-4d  %-7d  %-5d  %-5d  %-9d   %-8d   ",
                    voltage,  (signed short)cur, ufcapacity, capacity, afcapacity, rm, fcc, uuc, cc, delta_rc, pd_charge, temp, ocv, rbatt, di->batt_limit_fcc/1000, di->last_ocv_level, di->batt_ocv_valid_to_refresh_fcc);

        return strlen(buf);
    case COUL_SYSFS_HAND_CHG_CAPACITY_FLAG:
        return snprintf(buf, PAGE_SIZE, "%d\n", hand_chg_capacity_flag);
    case COUL_SYSFS_INPUT_CAPACITY:
        return snprintf(buf, PAGE_SIZE, "%d\n", input_capacity);
    case COUL_SYSFS_ABS_CC:
        val = di->coul_dev_ops->get_abs_cc() + (di->coul_dev_ops->calculate_cc_uah() / 1000);
        return snprintf(buf, PAGE_SIZE, "%d\n", val);
    case COUL_SYSFS_BATTERY_ID_VOLTAGE:
        return snprintf(buf, PAGE_SIZE, "%d\n",di->batt_id_vol);
    case COUL_SYSFS_BATTERY_BRAND_NAME:
        return snprintf(buf, PAGE_SIZE, "%s\n",di->batt_data->batt_brand);
    case COUL_SYSFS_RBATT:
        rbatt = coul_get_battery_resistance();
        return snprintf(buf, PAGE_SIZE, "%d\n", rbatt);
    case COUL_SYSFS_REAL_SOC:
        ufcapacity = coul_battery_unfiltered_capacity();
        return snprintf(buf, PAGE_SIZE, "%d\n", ufcapacity);
    case COUL_SYSFS_CALI_ADC:
        return snprintf(buf, PAGE_SIZE, "%d\n", 0);
    default:
        hwlog_err("(%s)NODE ERR!!HAVE NO THIS NODE:(%d)\n",__func__,info->name);
        break;
    }
    return 0;
}

static int save_cali_param(void)
{
    int ret;
    struct hisi_nve_info_user nve;
    struct coul_cali_nv_info *pinfo = (struct coul_cali_nv_info* ) (&(nve.nv_data[0]));

    memset(&nve, 0, sizeof(nve));
    strncpy(nve.nv_name, HISI_BAT_CALIBRATION_NV_NAME, sizeof(HISI_BAT_CALIBRATION_NV_NAME));
    nve.nv_number = HISI_BAT_CALIBRATION_NV_NUM;
    nve.valid_size = sizeof(*pinfo);
    nve.nv_operation = NV_WRITE;
    pinfo->v_offset_a = v_offset_a;
    pinfo->v_offset_b = v_offset_b;

    ret = hisi_nve_direct_access(&nve);
    if (ret)
    {
        hwlog_err("save cali param failed, ret=%d\n", ret);
    }
    else
    {
        hwlog_info("save cali param success\n");
    }
    return ret;
}

/**********************************************************
*  Function:      coulsysfs_store
*  Discription:   set the value for coul_data's node which is can be written
*  Parameters:    dev:device
*                 attr:device_attribute
*                 buf:string of node value
*                 count:unused
*  return value:  0-sucess or others-fail
**********************************************************/
static ssize_t coul_sysfs_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    struct coul_sysfs_field_info *info = NULL;
    struct smartstar_coul_device *di = dev_get_drvdata(dev);
    long val = 0;

    info = coul_sysfs_field_lookup(attr->attr.name);
    if (!info)
        return -EINVAL;

    switch(info->name){
    case COUL_SYSFS_PL_CALIBRATION_EN:
        if((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
            return -EINVAL;
        pl_calibration_en = val;
	if(pl_calibration_en == TRUE)
	{
		di->coul_dev_ops->cali_auto_off();
	}
        coul_cali_adc(di);
        break;
    case COUL_SYSFS_PL_V_OFFSET_A:
        if(strict_strtol(buf, 10, &val) < 0)
            return -EINVAL;
        v_offset_a = val;
        break;
    case COUL_SYSFS_PL_V_OFFSET_B:
        if(strict_strtol(buf, 10, &val) < 0)
            return -EINVAL;
        v_offset_b = val;
        break;
    case COUL_SYSFS_PL_C_OFFSET_A:
        if(strict_strtol(buf, 10, &val) < 0)
            return -EINVAL;
        c_offset_a = val;
        break;
    case COUL_SYSFS_PL_C_OFFSET_B:
        if(strict_strtol(buf, 10, &val) < 0)
            return -EINVAL;
        c_offset_b = val;
        break;
    case COUL_SYSFS_DO_SAVE_OFFSET:
        do_save_offset_ret = save_cali_param();
        hwlog_info("do_save_offset_ret:%d, v_offset_a:%d, c_offset_a:%d\n", do_save_offset_ret,v_offset_a, c_offset_a);
        break;
    case COUL_SYSFS_HAND_CHG_CAPACITY_FLAG:
        if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
            return -EINVAL;
        hand_chg_capacity_flag = val;
        hwlog_info("hand_chg_capacity_flag is set to %d\n", hand_chg_capacity_flag);
        break;
    case COUL_SYSFS_INPUT_CAPACITY:
        if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 100))
            return -EINVAL;
        input_capacity = val;
        break;
    case COUL_SYSFS_CALI_ADC:
        if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 100))
            return -EINVAL;
        hwlog_info("cali_adc =  %ld\n", val);
        if (1 == val) {
			coul_cali_adc(di);
        }
        break;
    default:
        hwlog_err("(%s)NODE ERR!!HAVE NO THIS NODE:(%d)\n",__func__,info->name);
        break;
    }
    return count;
}

/**********************************************************
*  Function:       coul_sysfs_create_group
*  Discription:    create the coul device sysfs group
*  Parameters:     di:smartstar_coul_device
*  return value:   0-sucess or others-fail
**********************************************************/
static int coul_sysfs_create_group(struct smartstar_coul_device *di)
{
    coul_sysfs_init_attrs();
    return sysfs_create_group(&di->dev->kobj, &coul_sysfs_attr_group);
}
/**********************************************************
*  Function:       charge_sysfs_remove_group
*  Discription:    remove the charge device sysfs group
*  Parameters:   di:charge_device_info
*  return value:  NULL
**********************************************************/
static inline void coul_sysfs_remove_group(struct smartstar_coul_device *di)
{
    sysfs_remove_group(&di->dev->kobj, &coul_sysfs_attr_group);
}
#else
static int coul_sysfs_create_group(struct smartstar_coul_device *di)
{
    return 0;
}
static inline void coul_sysfs_remove_group(struct smartstar_coul_device *di) {}
#endif

static int coul_create_sysfs(struct smartstar_coul_device *di)
{
    int retval;
    struct class *power_class;

    if (!di) {
        hwlog_err("%s input di is null.", __func__);
        return -1;
    }

    retval = coul_sysfs_create_group(di);
    if (retval) {
        hwlog_err("%s failed to create sysfs group!!!\n", __func__);
        return -1;
    }
    power_class = hw_power_get_class();
    if (power_class) {
        if (NULL == coul_dev) {
            coul_dev = device_create(power_class, NULL, 0, "%s", "coul");
            if (IS_ERR(coul_dev)) {
                coul_dev = NULL;
            }
        }
        if (coul_dev) {
            retval = sysfs_create_link(&coul_dev->kobj, &di->dev->kobj, "coul_data");
            if (0 != retval)
                hwlog_err("%s failed to create sysfs link!!!\n", __FUNCTION__);
        } else {
            hwlog_err("%s failed to create new_dev!!!\n", __FUNCTION__);
        }
    }
    return retval;
}
/**********************************************************
*  Function:       coul_fault_notifier_call
*  Discription:    respond the fault events from coul driver
*  Parameters:   fault_nb:fault notifier_block
*                      event:fault event name
*                      data:unused
*  return value:  NOTIFY_OK-success or others
**********************************************************/
static int coul_fault_notifier_call(struct notifier_block *fault_nb,unsigned long event, void *data)
{
    struct smartstar_coul_device *di = container_of(fault_nb, struct smartstar_coul_device, fault_nb);

    di->coul_fault = (enum coul_fault_type)event;
    queue_work(system_power_efficient_wq, &di->fault_work);
    return NOTIFY_OK;
}

/**********************************************************
*  Function:       coul_fault_work
*  Discription:    handler the fault events from coul driver
*  Parameters:     work:fault workqueue
*  return value:   NULL
**********************************************************/
static void coul_fault_work(struct work_struct *work)
{
    struct smartstar_coul_device *di = container_of(work, struct smartstar_coul_device, fault_work);

    switch(di->coul_fault)
    {
        case COUL_FAULT_LOW_VOL:
            hwlog_err("[%s]low vol int!\n",__func__);
            di->coul_dev_ops->irq_disable(); /*disable coul irq*/
            coul_wake_lock();
            coul_low_vol_int_handle(di);
            coul_wake_unlock();
            di->coul_dev_ops->irq_enable(); /*enable coul irq*/
            di->coul_fault = COUL_FAULT_NON;
            break;
        case COUL_FAULT_CL_INT:
            hwlog_err("[%s]cc capability compare int!\n",__func__);
            di->coul_fault = COUL_FAULT_NON;
            break;
        case COUL_FAULT_CL_IN:
            hwlog_err("[%s]cc in over 81 int!\n",__func__);
            di->coul_fault = COUL_FAULT_NON;
            make_cc_no_overload(di);
            break;
        case COUL_FAULT_CL_OUT:
            hwlog_err("[%s]cc out over 81 int!\n",__func__);
            di->coul_fault = COUL_FAULT_NON;
            make_cc_no_overload(di);
            break;
        default:
            hwlog_err("[%s] default deal with!\n",__func__);
            break;
    }
}

static void contexthub_thermal_init(void)
{
#ifdef CONFIG_HISI_THERMAL_CONTEXTHUB
	int i, v_t_table[T_V_ARRAY_LENGTH+1][2];
	struct hw_chan_table* p_ddr_header;
	char* p_chub_ddr;

	void __iomem *g_share_addr = ioremap_wc(CONTEXTHUB_THERMAL_DDR_HEADER_ADDR,
		CONTEXTHUB_THERMAL_DDR_TOTAL_SIZE);
	if (NULL == g_share_addr) {
		pr_err("[%s]share_addr ioremap_wc failed.\n", __func__);
		return;
	}
	memset((void*)g_share_addr, 0xFF, CONTEXTHUB_THERMAL_DDR_TOTAL_SIZE);
	p_chub_ddr = g_share_addr + CONTEXTHUB_THERMAL_DDR_MEMBERS_SIZE;
	p_ddr_header =  (struct hw_chan_table*)g_share_addr;
	p_ddr_header++;
	p_ddr_header->usr_id = 0xFFFF;
	p_ddr_header->hw_channel = (unsigned short int)adc_batt_temp;
	p_ddr_header->table_id = (unsigned short int)HKADC_BAT_TABLEID;
	p_ddr_header->table_size = sizeof(v_t_table);
	if(p_ddr_header->table_size > CONTEXTHUB_THERMAL_DDR_MEMBERS_SIZE) {
		pr_err("[%s]tableSIZE[%d]MAX[%d]\n", __func__, p_ddr_header->table_size,
			CONTEXTHUB_THERMAL_DDR_MEMBERS_SIZE);
		return;
	}

	for(i = 0; i <= T_V_ARRAY_LENGTH; i++) {
		v_t_table[i][0] = adc_to_volt(T_V_TABLE[T_V_ARRAY_LENGTH - i][1]);/*lint !e679*/
		v_t_table[i][1] = T_V_TABLE[T_V_ARRAY_LENGTH - i][0];/*lint !e679*/
	}

	memcpy((void*)(p_chub_ddr + CONTEXTHUB_THERMAL_DDR_MEMBERS_SIZE * HKADC_BAT_TABLEID),
		(void*)v_t_table, p_ddr_header->table_size);
#endif
}

/**********************************************************
*  Function:       get_ntc_table
*  Discription:    get ntc table from dts
*  Parameters:     np:device_node
*  return value:   0: SUCCESS; -1: FAIL
**********************************************************/
static int get_ntc_table(struct device_node* np)
{
    u32 ntc_table[T_V_ARRAY_LENGTH] = {0};
    int i = 0;
    int len = -1;

    if(!np)
    {
        hwlog_err("%s, np is null!\n", __func__);
        return -1;
    }

    len = of_property_count_u32_elems(np,"ntc_table");
    hwlog_debug("Load ntc length is %d\n", len);
    if( len != T_V_ARRAY_LENGTH+1 ){
        if(len == -1){
            hwlog_err("%s, ntc_table not exist, use default array!\n", __func__);
        }else{
            hwlog_err("%s, ntc_table length is %d, use default array!\n", __func__, len);
        }
        return -1;
    }

    if(of_property_read_u32_array(np, "ntc_table", ntc_table, T_V_ARRAY_LENGTH))
    {
        hwlog_err("%s, ntc_table not exist, use default array!\n", __func__);
        return -1;
    }

    for(i = 0; i < T_V_ARRAY_LENGTH; i++)
    {
        T_V_TABLE[i][1] = ntc_table[i];
        hwlog_debug("T_V_TABLE[%d][1] = %d\t",i,T_V_TABLE[i][1]);
    }
    hwlog_debug("\n");

    return 0;
}
#ifdef CONFIG_HUAWEI_CHARGER_SENSORHUB
static void coul_sensorhub_init(struct platform_device *pdev, struct smartstar_coul_device *di)
{

	if (NULL == pdev || NULL == di)
		return;
	g_di_coul_info_sh = (struct coul_core_info_sh *)devm_kzalloc(&pdev->dev,sizeof(struct coul_core_info_sh), GFP_KERNEL);
	if (NULL == g_di_coul_info_sh) {
		hwlog_err("g_di_coul_info_sh allocate fail!\n");
		return;
	}
	g_di_coul_info_sh->ntc_compensation_is = di->ntc_compensation_is;
	g_di_coul_info_sh->basp_learned_fcc = basp_learned_fcc;
	g_di_coul_info_sh->basp_level = di->basp_level;
	g_di_coul_info_sh->basp_total_level = di->basp_total_level;
	g_di_coul_info_sh->v_offset_a = v_offset_a;
	g_di_coul_info_sh->v_offset_b = v_offset_b;
	g_di_coul_info_sh->c_offset_a = c_offset_a;
	g_di_coul_info_sh->c_offset_b = c_offset_b;
	hwlog_info("vbat calibration parameter: v_offset_a=%d,v_offset_b=%d,c_offset_a=%d,c_offset_b=%d\n", g_di_coul_info_sh->v_offset_a, g_di_coul_info_sh->v_offset_b, g_di_coul_info_sh->c_offset_a, g_di_coul_info_sh->c_offset_b);
	memcpy((void*)g_di_coul_info_sh->basp_policy, (void*)basp_policy,
		sizeof(struct battery_aging_safe_policy)*BASP_LEVEL_CNT);
	memcpy((void*)g_di_coul_info_sh->ntc_temp_compensation_para, (void*)di->ntc_temp_compensation_para,
		sizeof(struct ntc_temp_compensation_para_data)*COMPENSATION_PARA_LEVEL);
	hwlog_info("coul_sensorhub:get ntc_compensation_is=%d,basp_learned_fcc=%d,basp_level=%d\n",
        				g_di_coul_info_sh->ntc_compensation_is, g_di_coul_info_sh->basp_learned_fcc, g_di_coul_info_sh->basp_level);
}

extern struct CONFIG_ON_DDR* pConfigOnDDr;
int update_coul_sensorhub_info(void)
{
	pConfigOnDDr->g_di_coul_info_sh.basp_level = g_smartstar_coul_dev->basp_level;
	pConfigOnDDr->g_di_coul_info_sh.basp_learned_fcc = basp_learned_fcc;
	return 0;
}
#endif
static void coul_core_get_basp_policy(struct device_node* np, struct smartstar_coul_device* di)
{
	int ret = 0;
	unsigned int i = 0;
	int array_len = 0;
	u32 basp_tmp[BASP_MEM_CNT*BASP_PARA_LEVEL];

	/* basp_policy para */
	array_len = of_property_count_u32_elems(np, "basp_policy");
	if ((array_len <= 0) ||(array_len % BASP_MEM_CNT != 0)) {
		di->basp_total_level = 0;
		hwlog_err(BASP_TAG"basp_policy is invaild, please check basp_policy number!!\n");
	} else if (array_len > BASP_PARA_LEVEL * BASP_MEM_CNT) {
		di->basp_total_level = 0;
		hwlog_err(BASP_TAG"basp_policy is too long, use only front %d paras!!\n" , array_len);
	} else {
		ret = of_property_read_u32_array(np, "basp_policy", basp_tmp, array_len);
		if (ret) {
		di->basp_total_level = 0;
		hwlog_err(BASP_TAG"dts:get basp_policy fail!\n");
		} else {
			di->basp_total_level = array_len / BASP_MEM_CNT;
			for (i = 0; i < di->basp_total_level; i++) {
				basp_policy[i].level = basp_tmp[(int)(BASP_MEM_LEVEL+BASP_MEM_CNT*i)]; /*lint !e64*//*(int) for pclint and can never be out of bounds*/
				basp_policy[i].chrg_cycles = basp_tmp[(int)(BASP_MEM_CHRG_CYCLES+BASP_MEM_CNT*i)];
				basp_policy[i].fcc_ratio = basp_tmp[(int)(BASP_MEM_FCC_RATIO+BASP_MEM_CNT*i)];
				basp_policy[i].dc_volt_dec = basp_tmp[(int)(BASP_MEM_DC_VOLT_DEC+BASP_MEM_CNT*i)];
				basp_policy[i].nondc_volt_dec = basp_tmp[(int)(BASP_MEM_NONDC_VOLT_DEC+BASP_MEM_CNT*i)];
				basp_policy[i].cur_ratio = basp_tmp[(int)(BASP_MEM_CUR_RATIO+BASP_MEM_CNT*i)];
				basp_policy[i].cur_ratio_policy = basp_tmp[(int)(BASP_MEM_CUR_RATIO_POLICY+BASP_MEM_CNT*i)];
				basp_policy[i].err_no = basp_tmp[(int)(BASP_MEM_ERR_NO+BASP_MEM_CNT*i)];
				hwlog_info(BASP_TAG"[%d], level: %d chrg_cycles: %-5d "
							"fcc_ratio: %-3d dc_volt_dec: %-3d nondc_volt_dec: %-3d cur_ratio: %-3d cur_ratio_policy: %d err_no: %d\n",
							i, basp_policy[i].level, basp_policy[i].chrg_cycles, basp_policy[i].fcc_ratio,
							basp_policy[i].dc_volt_dec, basp_policy[i].nondc_volt_dec,
							basp_policy[i].cur_ratio, basp_policy[i].cur_ratio_policy, basp_policy[i].err_no);
			}
		}
	}
}
static void coul_core_get_iscd_dsm_config(struct device_node* np, struct smartstar_coul_device* di)
{
    int ret = 0;
    int i = 0;
    int array_len;
    u32 config_tmp[ISCD_LEVEL_CONFIG_CNT*ISCD_MAX_LEVEL];

    if (!di ||!np) {
        hwlog_info("ISCD %s di is null.\n", __func__);
        return;
    }

    /* iscd dsm config para */
    array_len = of_property_count_u32_elems(np, "iscd_level_info");
    if ((array_len <= 0) ||(array_len % ISCD_LEVEL_CONFIG_CNT != 0)) {
        di->iscd->total_level = 0;
        hwlog_err("ISCD iscd_level_info is invaild, please check iscd_level_info number!!\n");
    } else if (array_len > (int)ISCD_MAX_LEVEL * ISCD_LEVEL_CONFIG_CNT) {
        di->iscd->total_level  = 0;
        hwlog_err("ISCD iscd_level_info is too long, use only front %d paras!!\n" , array_len);
    } else {
        ret = of_property_read_u32_array(np, "iscd_level_info", config_tmp, (unsigned long)(long)array_len);
        if (ret) {
            di->iscd->total_level  = 0;
            hwlog_err("ISCD dts:get iscd_level_info fail!\n");
        } else {
            di->iscd->total_level  = array_len / ISCD_LEVEL_CONFIG_CNT;
            for (i = 0; i < di->iscd->total_level; i++) {
                di->iscd->level_config[i].isc_min = (int)config_tmp[(int)(ISCD_ISC_MIN+ISCD_LEVEL_CONFIG_CNT*i)]; /*(int) for pclint and can never be out of bounds*/
                di->iscd->level_config[i].isc_max = (int)config_tmp[(int)(ISCD_ISC_MAX+ISCD_LEVEL_CONFIG_CNT*i)];
                di->iscd->level_config[i].dsm_err_no = (int)config_tmp[(int)(ISCD_DSM_ERR_NO+ISCD_LEVEL_CONFIG_CNT*i)];
                di->iscd->level_config[i].dsm_report_cnt = (int)config_tmp[(int)(ISCD_DSM_REPORT_CNT+ISCD_LEVEL_CONFIG_CNT*i)];
                di->iscd->level_config[i].dsm_report_time = config_tmp[(int)(ISCD_DSM_REPORT_TIME+ISCD_LEVEL_CONFIG_CNT*i)];
                di->iscd->level_config[i].protection_type = (int)config_tmp[(int)(ISCD_PROTECTION_TYPE+ISCD_LEVEL_CONFIG_CNT*i)];
                hwlog_info("ISCD level[%d], isc_min: %-6d isc_max: %-7d dsm_err_no: %-9d dsm_report_cnt: %d dsm_report_time:%ld dsm_protection_type:%d\n",
                     i, di->iscd->level_config[i].isc_min, di->iscd->level_config[i].isc_max,
                     di->iscd->level_config[i].dsm_err_no, di->iscd->level_config[i].dsm_report_cnt,
                     di->iscd->level_config[i].dsm_report_time, di->iscd->level_config[i].protection_type);
            }
        }
    }
}
static void coul_core_get_iscd_info(struct device_node* np, struct smartstar_coul_device* di)
{
    int ret;

    if (!di ||!np) {
        hwlog_info("ISCD %s di is null.\n", __func__);
        return;
    }

    ret = of_property_read_s32(np, "iscd_enable", &di->iscd->enable);
    if (ret) {
        hwlog_err("get iscd_enable fail, use default one !!\n");
        di->iscd->enable = DISABLED;
    }
    hwlog_info("ISCD iscd_enable = %d\n", di->iscd->enable);
    ret = of_property_read_s32(np, "iscd_ocv_min", &di->iscd->ocv_min);
    if (ret) {
        hwlog_err("get iscd_ocv_min fail, use default one !!\n");
        di->iscd->ocv_min = ISCD_DEFAULT_OCV_MIN;
    }
    hwlog_info("ISCD ocv_min = %d\n", di->iscd->ocv_min);
    ret = of_property_read_s32(np, "iscd_batt_temp_min", &di->iscd->tbatt_min);
    if (ret) {
        hwlog_err("get iscd_batt_temp_min fail, use default one !!\n");
        di->iscd->tbatt_min = ISCD_DEFAULT_TBATT_MIN;
    }
    hwlog_info("ISCD tbatt_min = %d\n", di->iscd->tbatt_min);
    ret = of_property_read_s32(np, "iscd_batt_temp_max", &di->iscd->tbatt_max);
    if (ret) {
        hwlog_err("get iscd_batt_temp_max fail, use default one !!\n");
        di->iscd->tbatt_max = ISCD_DEFAULT_TBATT_MAX;
    }
    hwlog_info("ISCD tbatt_max = %d\n", di->iscd->tbatt_max);
    ret = of_property_read_s32(np, "iscd_batt_temp_diff_max", &di->iscd->tbatt_diff_max);
    if (ret) {
        hwlog_err("get iscd_batt_temp_diff_max fail, use default one !!\n");
        di->iscd->tbatt_diff_max = ISCD_DEFAULT_TBATT_DIFF;
    }
    hwlog_info("ISCD tbatt_diff_max = %d\n", di->iscd->tbatt_diff_max);
    ret = of_property_read_s32(np, "iscd_sample_time_interval", &di->iscd->sample_time_interval);
    if (ret) {
        hwlog_err("get iscd_sample_time_interval fail, use default one !!\n");
        di->iscd->sample_time_interval = ISCD_DEFAULT_SAMPLE_INTERVAL;
    }
    hwlog_info("ISCD sample_time_interval = %d\n", di->iscd->sample_time_interval);
    ret = of_property_read_s32(np, "iscd_sample_time_delay", &di->iscd->sample_time_delay);
    if (ret) {
        hwlog_err("get iscd_sample_time_delay fail, use default one !!\n");
        di->iscd->sample_time_delay = ISCD_DEFAULT_SAMPLE_DELAY;
    }
    hwlog_info("ISCD sample_time_delay = %d\n", di->iscd->sample_time_delay);
    ret = of_property_read_s32(np, "iscd_calc_time_interval_min", &di->iscd->calc_time_interval_min);
    if (ret) {
        hwlog_err("get iscd_calc_time_interval_min fail, use default one !!\n");
        di->iscd->calc_time_interval_min = ISCD_DEFAULT_CALC_INTERVAL_MIN;
    }
    hwlog_info("ISCD calc_time_interval_min = %d\n", di->iscd->calc_time_interval_min);
    ret = of_property_read_s32(np, "iscd_level_warning_threhold", &di->iscd->isc_warning_threhold);
    if (ret) {
        hwlog_err("get iscd_level_warning_threhold fail, use default one !!\n");
        di->iscd->isc_warning_threhold = ISCD_WARNING_LEVEL_THREHOLD;
    }
    hwlog_info("ISCD isc_warning_threhold = %d\n", di->iscd->isc_warning_threhold);
    ret = of_property_read_s32(np, "iscd_level_error_threhold", &di->iscd->isc_error_threhold);
    if (ret) {
        hwlog_err("get iscd_level_error_threhold fail, use default one !!\n");
        di->iscd->isc_error_threhold = ISCD_ERROR_LEVEL_THREHOLD;
    }
    hwlog_info("ISCD isc_error_threhold = %d\n", di->iscd->isc_error_threhold);
    ret = of_property_read_s32(np, "iscd_level_critical_threhold", &di->iscd->isc_critical_threhold);
    if (ret) {
        hwlog_err("get iscd_level_critical_threhold fail, use default one !!\n");
        di->iscd->isc_critical_threhold = ISCD_CRITICAL_LEVEL_THREHOLD;
    }
    hwlog_info("ISCD isc_critical_threhold = %d\n", di->iscd->isc_critical_threhold);
    coul_core_get_iscd_dsm_config(np, di);
}
static void check_low_temp_opt(struct device_node* np)
{
    int ret = 0;
    ret = of_property_read_u32(np, "low_temp_opt_enable", (u32 *)&low_temp_opt_flag);
    if (ret || LOW_TEMP_OPT_OPEN != low_temp_opt_flag) {
        low_temp_opt_flag = LOW_TEMP_OPT_CLOSE;
    }
    hwlog_info("low_temp_opt: low temp soc show optimize is %d\n", low_temp_opt_flag);
}

static void get_multi_ocv_open_flag(struct device_node* np)
{
    int ret = 0;
	if (NULL == np)
		return;
    ret = of_property_read_u32(np, "multi_ocv_open", &multi_ocv_open_flag);
    if (ret) {
        multi_ocv_open_flag = 0;
    }
    hwlog_info("multi_ocv_open: flag is %d\n", multi_ocv_open_flag);
}

static void get_fcc_update_limit_flag(struct device_node* np)
{
    int ret = 0;
	if (NULL == np)
		return;
    ret = of_property_read_u32(np, "fcc_update_limit", &fcc_update_limit_flag);
    if (ret) {
        fcc_update_limit_flag = 0;
    }
    hwlog_info("fcc_update_limit: flag is %d\n", fcc_update_limit_flag);
}


static void get_cutoff_vol_mv(struct smartstar_coul_device* di,struct device_node* np)
{
    if(!di || !np)
        return;
    if (of_property_read_u32(np, "normal_cutoff_vol_mv", &di->v_cutoff))
        di->v_cutoff = BATTERY_NORMAL_CUTOFF_VOL;
    if (of_property_read_u32(np, "sleep_cutoff_vol_mv", &di->v_cutoff_sleep))
        di->v_cutoff_sleep = BATTERY_VOL_2_PERCENT;

    hwlog_err("get_cutoff_vol_mv: cutoff = %d,cutoff_sleep = %d\n", di->v_cutoff,di->v_cutoff_sleep);
}


static void coul_core_get_dts(struct smartstar_coul_device *di)
{
	struct device_node *batt_node;
    struct device_node* np;
    unsigned int r_pcb = DEFAULT_RPCB;
    unsigned int last_soc_enable = 0;
    unsigned int startup_delta_soc = 0;
    unsigned int soc_at_term = 100;
    unsigned int soc_monitor_limit = DEFAULT_SOC_MONITOR_LIMIT;
    unsigned int low_vol_filter_cnt = LOW_INT_VOL_COUNT;
    const char *compensation_data_string = NULL;
    unsigned int ntc_compensation_is =0;
    int ret = 0;
    int i = 0;
    int idata = 0;
    int array_len = 0;
    const char *batt_temp_too_hot_string = NULL;
    const char *batt_temp_too_cold_string = NULL;
    np = di->dev->of_node;
    if(NULL == np){
        hwlog_err("%s np is null!\n",__FUNCTION__);
        return;
    }
    /*check if open low temp soc show optimize function*/
    check_low_temp_opt(np);
	get_multi_ocv_open_flag(np);
	get_fcc_update_limit_flag(np);
    get_cutoff_vol_mv(di,np);
    if (of_property_read_u32(np, "current_offset_a",(u32 *)&c_offset_a)){
	    c_offset_a = DEFAULT_C_OFF_A;
		hwlog_err("error:get current_offset_a value failed!\n");
    }
	hwlog_info("dts:get v_a=%d,v_b=%d,c_a=%d,c_b=%d\n",
        				v_offset_a, v_offset_b, c_offset_a, c_offset_b);

    batt_node = of_find_compatible_node(NULL, NULL, "huawei,hisi_bci_battery");
    if (batt_node) {
        of_property_read_u32(batt_node, "battery_board_type",&is_board_type);
		of_property_read_u32(batt_node, "battery_is_removable",&battery_is_removable);
    } else {
		is_board_type = BAT_BOARD_SFT;
		battery_is_removable = 0;
	}
	hwlog_err( "dts:get board type is %d ,battery removable flag is %d !\n",is_board_type ,battery_is_removable);

    if (of_property_read_u32(np, "r_pcb",&r_pcb)){
		hwlog_err("error:get r_pcb value failed!\n");
	}
	di->r_pcb = r_pcb;
    hwlog_info("dts:get r_pcb = %d! \n",r_pcb);

    if (of_property_read_u32(np, "adc_batt_id",&adc_batt_id)){
		hwlog_err("dts:can not get batt id adc channel,use default channel: %d!\n",adc_batt_id);
    }
    hwlog_info("dts:get batt id adc channel = %d! \n",adc_batt_id);

    if (of_property_read_u32(np, "adc_batt_temp",&adc_batt_temp)){
		hwlog_err("dts:can not get batt temp adc channel,use default channel: %d!\n",adc_batt_temp);
    }
    hwlog_info("dts:get batt temperature adc channel = %d! \n",adc_batt_temp);

    if (get_ntc_table(np) == -1){
        hwlog_err("Use default ntc_table!\n");
    } else{
        hwlog_err("Use ntc_table from dts!\n");
    }

    if (of_property_read_u32(np, "last_soc_enable",&last_soc_enable)){
        hwlog_err("dts:can not get last_soc_enable,use default : %d!\n",last_soc_enable);
    }
    di->last_soc_enable = last_soc_enable;
    hwlog_info("dts:get last_soc_enable = %d! \n",last_soc_enable);

    if (of_property_read_u32(np, "startup_delta_soc",&startup_delta_soc)){
        hwlog_err("dts:can not get delta_soc,use default : %d!\n",startup_delta_soc);
    }
    di->startup_delta_soc = startup_delta_soc;
    hwlog_info("dts:get delta_soc = %d! \n",startup_delta_soc);

    if (of_property_read_u32(np, "soc_at_term",&soc_at_term)){
        hwlog_err("dts:can not get soc_at_term,use default : %d!\n",soc_at_term);
    }
    di->soc_at_term = soc_at_term;
    hwlog_info("dts:get soc_at_term = %d! \n",soc_at_term);

    /* ntc_temp_compensation_para */
    if(of_property_read_u32(np, "ntc_compensation_is", &(ntc_compensation_is)))
    {
        hwlog_info("get ntc_compensation_is failed\n");
    }else
    {
        di->ntc_compensation_is = ntc_compensation_is;
        hwlog_info("ntc_compensation_is = %d\n",di->ntc_compensation_is);

        memset(di->ntc_temp_compensation_para, 0, COMPENSATION_PARA_LEVEL * sizeof(struct ntc_temp_compensation_para_data));/* reset to 0*/
        array_len = of_property_count_strings(np, "ntc_temp_compensation_para");
        if ((array_len <= 0) ||(array_len % NTC_COMPENSATION_PARA_TOTAL != 0))
        {
            hwlog_err("ntc_temp_compensation_para is invaild,please check ntc_temp_compensation_para number!!\n");
            return ;
        }
        if (array_len > COMPENSATION_PARA_LEVEL * NTC_COMPENSATION_PARA_TOTAL)
        {
            array_len = COMPENSATION_PARA_LEVEL * NTC_COMPENSATION_PARA_TOTAL;
            hwlog_err("ntc_temp_compensation_para is too long,use only front %d paras!!\n" , array_len);
            return;
        }

        for (i = 0; i < array_len; i++)
        {
            ret = of_property_read_string_index(np, "ntc_temp_compensation_para", i, &compensation_data_string);
            if (ret)
            {
                hwlog_err("get ntc_temp_compensation_para failed\n");
                return ;
            }
            idata = simple_strtol(compensation_data_string, NULL, 10);
            switch(i % NTC_COMPENSATION_PARA_TOTAL)
            {
                case  NTC_COMPENSATION_PARA_ICHG :
                    di->ntc_temp_compensation_para[i / NTC_COMPENSATION_PARA_TOTAL].ntc_compensation_ichg = idata;
                    break;
                case  NTC_COMPENSATION_PARA_VALUE :
                    di->ntc_temp_compensation_para[i / NTC_COMPENSATION_PARA_TOTAL].ntc_compensation_value = idata;
                    break;
                default:
                    hwlog_err("ntc_temp_compensation_para get failed \n");
            }
            hwlog_info("di->ntc_temp_compensation_para[%d][%d] = %d\n",
                        i / (NTC_COMPENSATION_PARA_TOTAL), i % (NTC_COMPENSATION_PARA_TOTAL), idata);
        }
    }

    coul_core_get_basp_policy(np, di);
    coul_core_get_iscd_info(np, di);

    if (of_property_read_u32(np, "soc_monitor_limit", &soc_monitor_limit)) {
        hwlog_err("dts:get soc_monitor_limit fail, use default limit value!\n");
    }
    di->soc_monitor_limit = soc_monitor_limit;
    hwlog_info("soc_monitor_limit = %d\n",di->soc_monitor_limit);

    if (of_property_read_u32(np, "low_vol_filter_cnt", &low_vol_filter_cnt)) {
        hwlog_err("dts:get low_vol_filter_cnt fail, use default limit value!\n");
    }
    di->low_vol_filter_cnt = low_vol_filter_cnt;
    hwlog_info("low_vol_filter_cnt = %d\n",di->low_vol_filter_cnt);

    if (of_property_read_string(np, "batt_temp_too_hot", &batt_temp_too_hot_string))
	    hwlog_err("error:get batt_temp_too_hot value failed!\n");
    else
	    batt_temp_too_hot = simple_strtol(batt_temp_too_hot_string, NULL, 10);
    hwlog_info("dts:get batt_temp_too_hot = %d! \n",batt_temp_too_hot);

    if (of_property_read_string(np, "batt_temp_too_cold", &batt_temp_too_cold_string))
	    hwlog_err("error:get batt_temp_too_cold value failed!\n");
    else
	    batt_temp_too_cold = simple_strtol(batt_temp_too_cold_string, NULL, 10);
    hwlog_info("dts:get temp_too_cold = %d! \n",batt_temp_too_cold);

}

static int coul_shutdown_prepare(struct notifier_block *nb, unsigned long event, void *_data)
{
    struct smartstar_coul_device *di = container_of(nb, struct smartstar_coul_device, reboot_nb);

    switch (event)
    {
    case SYS_DOWN:
    case SYS_HALT:
    case SYS_POWER_OFF:
        hwlog_info("coul prepare to shutdown, event = %ld\n",event);
        cancel_delayed_work_sync(&di->calculate_soc_delayed_work);
        break;
    default:
        hwlog_err("error event, coul ignore, event = %ld\n",event);
        break;
    }
    return 0;
}
static void battery_para_check(struct smartstar_coul_device *di)
{
    if(NULL == di)
    {
        return ;
    }
    di->basp_level = get_basp_level(di);
    batt_init_level = di->basp_level;
    if( BASP_LEVEL_0 != di->basp_level)/*battery is not new*/
    {
        if(battery_para_changed(di,BOOT_LOAD) < 0)
        {
             hwlog_err("battery para charged no config!\n");
        }
    }
}
/*******************************************************
  Function:        hisi_coul_probe
  Description:     probe function
  Input:           struct platform_device *pdev   ---- platform device
  Output:          NULL
  Return:          NULL
********************************************************/
static int  hisi_coul_probe(struct platform_device *pdev)
{
    struct hisi_coul_ops *coul_ops = NULL;
    struct smartstar_coul_device *di = NULL;
    int retval = 0;
    struct iscd_info *iscd;


    di = (struct smartstar_coul_device *)devm_kzalloc(&pdev->dev,sizeof(*di), GFP_KERNEL);
    if (!di) {
		hwlog_err("%s failed to alloc di struct\n",__FUNCTION__);
		return -1;
    }
    iscd = kzalloc(sizeof(*iscd), GFP_KERNEL);
    if (NULL == iscd) {
        hwlog_err("%s failed to alloc iscd struct\n",__FUNCTION__);
        return -1;/*lint !e429*/
    }
    di->iscd = iscd;
    di->low_vol_filter_cnt = LOW_INT_VOL_COUNT;
    iscd_reset_isc_buffer(di);

    di->dev =&pdev->dev;
    if (!g_coul_core_ops) {
        hwlog_err("%s g_coul_core_ops is NULL!\n",__FUNCTION__);
        return -1;/*lint !e429*/
     }
    di->coul_dev_ops = g_coul_core_ops;
    if((di->coul_dev_ops->calculate_cc_uah     == NULL) ||(di->coul_dev_ops->convert_regval2ua            == NULL)
        ||(di->coul_dev_ops->convert_regval2uv == NULL) ||(di->coul_dev_ops->is_battery_moved             == NULL)
        ||(di->coul_dev_ops->convert_uv2regval == NULL) ||(di->coul_dev_ops->get_fifo_avg_data            == NULL)
        ||(di->coul_dev_ops->get_nv_read_flag  == NULL) ||(di->coul_dev_ops->get_delta_rc_ignore_flag     == NULL)
        ||(di->coul_dev_ops->set_nv_save_flag  == NULL) ||(di->coul_dev_ops->get_battery_current_ua       == NULL)
        ||(di->coul_dev_ops->get_coul_time     == NULL) ||(di->coul_dev_ops->clear_cc_register            == NULL)
        ||(di->coul_dev_ops->cali_adc          == NULL) ||(di->coul_dev_ops->get_battery_voltage_uv       == NULL)
        ||(di->coul_dev_ops->get_abs_cc        == NULL) ||(di->coul_dev_ops->get_battery_vol_uv_from_fifo == NULL)
        ||(di->coul_dev_ops->exit_eco          == NULL) ||(di->coul_dev_ops->show_key_reg                 == NULL)
        ||(di->coul_dev_ops->enter_eco         == NULL) ||(di->coul_dev_ops->get_offset_current_mod       == NULL)
        ||(di->coul_dev_ops->save_ocv          == NULL) ||(di->coul_dev_ops->get_ocv                      == NULL)
        ||(di->coul_dev_ops->clear_coul_time   == NULL) ||(di->coul_dev_ops->set_low_low_int_val          == NULL)
        ||(di->coul_dev_ops->clear_ocv         == NULL) ||(di->coul_dev_ops->get_fcc_invalid_up_flag      == NULL)
        ||(di->coul_dev_ops->get_ate_a         == NULL) ||(di->coul_dev_ops->calculate_eco_leak_uah       == NULL)
        ||(di->coul_dev_ops->get_ate_b         == NULL) ||(di->coul_dev_ops->save_cc_uah                  == NULL)
        ||(di->coul_dev_ops->get_fifo_depth    == NULL) ||(di->coul_dev_ops->get_battery_cur_ua_from_fifo == NULL)
        ||(di->coul_dev_ops->save_ocv_temp     == NULL) ||(di->coul_dev_ops->get_ocv_temp                 == NULL)
        ||(di->coul_dev_ops->clear_ocv_temp    == NULL) ||(di->coul_dev_ops->get_use_saved_ocv_flag       == NULL)
        ||(di->coul_dev_ops->irq_enable        == NULL) ||(di->coul_dev_ops->set_battery_moved_magic_num  == NULL)
        ||(di->coul_dev_ops->get_last_soc      == NULL) ||(di->coul_dev_ops->save_last_soc                == NULL)
        ||(di->coul_dev_ops->get_last_soc_flag == NULL) ||(di->coul_dev_ops->clear_last_soc_flag          == NULL)
        ||(di->coul_dev_ops->get_offset_vol_mod == NULL)||(di->coul_dev_ops->set_offset_vol_mod           == NULL)
	||(di->coul_dev_ops->irq_disable       == NULL) ||(di->coul_dev_ops->cali_auto_off              == NULL))
    {
        hwlog_err("coul device ops is NULL!\n");
        retval = -EINVAL;
        goto coul_failed;
    }
    g_smartstar_coul_dev = di;
    platform_set_drvdata(pdev, di);
    /*get dts data*/
    coul_core_get_dts(di);
    contexthub_thermal_init();
    mutex_init(&di->soc_mutex);
    coul_set_low_vol_int(di, LOW_INT_STATE_RUNNING);
    /*set di element with default data*/
    di->prev_pc_unusable   = -EINVAL;
    di->batt_pre_delta_rc  = 0;
    di->last_cali_temp     = -990; /* invalid temperature */
    di->soc_work_interval  = CALCULATE_SOC_MS;
    di->last_iavg_ma       = IMPOSSIBLE_IAVG;
    /* read nv info */
    get_initial_params(di);
    di->is_nv_read         = di->coul_dev_ops->get_nv_read_flag();
    di->is_nv_need_save    = 0;
    di->coul_dev_ops->set_nv_save_flag(NV_SAVE_SUCCESS);
    di->sr_resume_time     = di->coul_dev_ops->get_coul_time();
	di->resume_cc = di->coul_dev_ops->calculate_cc_uah();
    sr_cur_state           = SR_DEVICE_WAKEUP;
    di->batt_temp = coul_battery_temperature_tenth_degree(USER_COUL);
    get_battery_id_voltage(di);
    /*check battery is exist*/
    if (!coul_is_battery_exist()) {
        hwlog_err("%s: no battery, just register callback\n",__FUNCTION__);
        di->batt_data = get_battery_data(di->batt_id_vol);
        di->batt_exist = 0;
        goto coul_no_battery;
    }
    di->batt_exist = 1;

    /*set battery data*/
    di->batt_data = get_battery_data(di->batt_id_vol);
    if (NULL == di->batt_data) {
        hwlog_err("%s: batt ID(0x%x) is invalid\n",__FUNCTION__,di->batt_id_vol);
        retval = -1;
        goto coul_failed_1;
    }
    hwlog_info("%s: batt ID is %d, batt_brand is %s\n",__FUNCTION__,di->batt_id_vol, di->batt_data->batt_brand);

    /*init battery remove check work*/
	if (battery_is_removable) {
        INIT_DELAYED_WORK(&di->battery_check_delayed_work, battery_check_work);
	}
    wake_lock_init(&coul_lock, WAKE_LOCK_SUSPEND, "coul_wakelock");
    /* Init soc calc work */
    INIT_DELAYED_WORK(&di->calculate_soc_delayed_work, calculate_soc_work);
    INIT_WORK(&di->fault_work, coul_fault_work);
    INIT_DELAYED_WORK(&di->read_temperature_delayed_work, read_temperature_work);
    INIT_DELAYED_WORK(&di->iscd->delayed_work, iscd_work);
    hrtimer_init(&di->iscd->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    di->iscd->timer.function = iscd_timer_func;

    di->fault_nb.notifier_call = coul_fault_notifier_call;
    retval = atomic_notifier_chain_register(&coul_fault_notifier_list, &di->fault_nb);
    di->reboot_nb.notifier_call = coul_shutdown_prepare;
    register_reboot_notifier(&(di->reboot_nb));
    if (retval < 0)
    {
       hwlog_err("coul_fault_register_notifier failed\n");
       goto coul_failed_2;
    }

#ifdef CONFIG_PM
	di->pm_notify.notifier_call = hisi_coul_pm_notify;
	register_pm_notifier(&di->pm_notify);
#endif
    hwlog_info("battery temperature is %d.%d\n", di->batt_temp/10, di->batt_temp%10);

    /*calculate init soc */
    coul_get_initial_ocv(di);
    di->charging_stop_time = di->coul_dev_ops->get_coul_time();

    /* battery moved, clear battery data  */
    if(di->coul_dev_ops->is_battery_moved()){
        di->batt_chargecycles = 0;
        di->batt_changed_flag = 1;
        di->batt_limit_fcc = 0;
        di->adjusted_fcc_temp_lut = NULL; /* enable it when test ok */
        di->is_nv_need_save = 1;
        di->coul_dev_ops->set_nv_save_flag(NV_SAVE_FAIL);
        di->coul_dev_ops->clear_last_soc_flag();
        /*clear safe record fcc*/
        di->nv_info.latest_record_index = 0;
        my_nv_info.latest_record_index = 0;
        memset(di->nv_info.real_fcc_record, 0, sizeof(di->nv_info.real_fcc_record));
        memset(my_nv_info.real_fcc_record, 0, sizeof(my_nv_info.real_fcc_record));
        hwlog_info("battery changed, reset chargecycles!\n");
    } else {
        hwlog_info("battery not changed, chargecycles = %d%%\n", di->batt_chargecycles);
    }
    battery_para_check(di);

    g_basp_full_pc = basp_full_pc_by_voltage(di);
    di->qmax = coul_get_qmax(di);
    hwlog_info("%s qmax is %dmAh\n", __func__, di->qmax/UA_PER_MA);

    /*get the first soc value*/
    DI_LOCK();
    di->soc_limit_flag = 0;
    di->soc_monitor_flag = 0;
    di->batt_soc = calculate_state_of_charge(di);
    coul_smooth_startup_soc(di);
    di->soc_limit_flag = 1;
    DI_UNLOCK();

    /*schedule calculate_soc_work*/
    queue_delayed_work(system_power_efficient_wq, &di->calculate_soc_delayed_work, round_jiffies_relative(msecs_to_jiffies(di->soc_work_interval)));
    queue_delayed_work(system_power_efficient_wq, &di->read_temperature_delayed_work, round_jiffies_relative(msecs_to_jiffies(READ_TEMPERATURE_MS)) );

coul_no_battery:
    coul_ops = (struct hisi_coul_ops*) kzalloc(sizeof (*coul_ops), GFP_KERNEL);
    if (!coul_ops) {
		hwlog_err("failed to alloc coul_ops struct\n");
		retval = -1;
        goto coul_failed_3;
    }
    /* config coul ops */
    coul_ops->battery_id_voltage          = coul_get_battery_id_vol;
    coul_ops->is_coul_ready               = coul_is_ready;
    coul_ops->is_battery_exist            = coul_is_battery_exist;
    coul_ops->is_battery_reach_threshold  = coul_is_battery_reach_threshold;
    coul_ops->battery_brand               = coul_get_battery_brand;
    coul_ops->battery_voltage             = coul_get_battery_voltage_mv;
    coul_ops->battery_voltage_uv          = coul_get_battery_voltage_uv;
    coul_ops->battery_current             = coul_get_battery_current_ma;
    coul_ops->fifo_avg_current             = coul_get_fifo_avg_current_ma;
    coul_ops->battery_current_avg         = coul_get_battery_current_avg_ma;
    coul_ops->battery_unfiltered_capacity = coul_battery_unfiltered_capacity;
    coul_ops->battery_capacity            = coul_get_battery_capacity;
    coul_ops->battery_temperature         = coul_get_battery_temperature;
    coul_ops->battery_rm                  = coul_get_battery_rm;
    coul_ops->battery_fcc                 = coul_get_battery_fcc;
    coul_ops->battery_tte                 = coul_get_battery_tte;
    coul_ops->battery_ttf                 = coul_get_battery_ttf;
    coul_ops->battery_health              = coul_get_battery_health;
    coul_ops->battery_capacity_level      = coul_get_battery_capacity_level;
    coul_ops->battery_technology          = coul_get_battery_technology;
    coul_ops->battery_charge_params       = coul_get_battery_charge_params;
    coul_ops->battery_vbat_max            = coul_get_battery_vbat_max;
    coul_ops->get_battery_limit_fcc       = coul_get_battery_limit_fcc;
    coul_ops->charger_event_rcv           = coul_battery_charger_event_rcv;
    coul_ops->coul_is_fcc_debounce        = coul_is_fcc_debounce;
    coul_ops->battery_cycle_count         = coul_battery_cycle_count;
    coul_ops->battery_fcc_design          = coul_battery_fcc_design;
    coul_ops->aging_safe_policy           = coul_get_battery_aging_safe_policy;
    coul_ops->dev_check                   = coul_device_check;
    coul_ops->get_soc_vary_flag           = coul_get_soc_vary_flag;
    coul_ops->battery_temperature_for_charger = coul_get_battery_temperature_for_charger;
    coul_ops->coul_low_temp_opt           = coul_get_low_temp_opt;
    coul_ops->battery_cc           = coul_get_battery_cc;
    di->ops = coul_ops;
    retval = hisi_coul_ops_register(coul_ops,COUL_HISI);
    if (retval) {
        hwlog_err("failed to register coul ops\n");
        goto coul_failed_4;
    }
#ifdef CONFIG_HUAWEI_CHARGER_SENSORHUB
	coul_sensorhub_init(pdev, di);
#endif
    /*create sysfs*/
    //retval = sysfs_create_group(&di->dev->kobj, &coul_attr_group);
    retval = coul_create_sysfs(di);
    if (retval) {
        hwlog_err("%s failed to create sysfs!!!\n", __FUNCTION__);
        goto coul_failed_4;
    }
    coul_cali_adc(di);
    hwlog_info("coul core probe ok!\n");
	g_pdev = pdev;
    return 0;

coul_failed_4:
    kfree(coul_ops);
    di->ops = NULL;
coul_failed_3:
    cancel_delayed_work(&di->calculate_soc_delayed_work);
    cancel_delayed_work(&di->read_temperature_delayed_work);
coul_failed_2:
    atomic_notifier_chain_unregister(&coul_fault_notifier_list, &di->fault_nb);
	if (battery_is_removable)
		cancel_delayed_work(&di->battery_check_delayed_work);
coul_failed_1:
    platform_set_drvdata(pdev, NULL);
    g_smartstar_coul_dev = 0;
coul_failed:
    kfree(iscd);
    hwlog_err("coul core probe failed!\n");
    return retval;/*lint !e593*/
}

/*******************************************************
  Function:        hisi_coul_remove
  Description:    remove function
  Input:            struct platform_device *pdev        ---- platform device
  Output:          NULL
  Return:          NULL
********************************************************/
static int  hisi_coul_remove(struct platform_device *pdev)
{
    struct smartstar_coul_device *di = platform_get_drvdata(pdev);

    if(NULL == di)
    {
        hwlog_info("[%s]di is null\n",__FUNCTION__);
        return 0;
    }
    return 0;

}

#ifdef CONFIG_PM

static int hisi_coul_pm_notify(struct notifier_block * nb, unsigned long mode, void * unused)
{
    struct smartstar_coul_device *di = container_of(nb, struct smartstar_coul_device, pm_notify);

    switch (mode) {
    case PM_SUSPEND_PREPARE:
	hwlog_info("%s:-n",__func__);
        sr_suspend_temp = coul_battery_temperature_tenth_degree(USER_COUL);
       	cancel_delayed_work_sync(&di->calculate_soc_delayed_work);
	break;
    case PM_POST_SUSPEND:
	hwlog_info("%s:+n",__func__);
	di->batt_soc = calculate_state_of_charge(di);
	queue_delayed_work(system_power_efficient_wq, &di->calculate_soc_delayed_work,
			round_jiffies_relative(msecs_to_jiffies((unsigned int)di->soc_work_interval)));
	break;
    case PM_HIBERNATION_PREPARE:
    case PM_POST_HIBERNATION:
    default:
	break;
    }
    return 0;
}/*lint !e715 */

static int calc_wakeup_avg_current(struct smartstar_coul_device *di, int last_cc, int last_time, int curr_cc, int curr_time)
{
	static int first_in = 1;
	int iavg_ma = 0;
	int delta_cc = 0;
	int delta_time = 0;

	if(NULL == di || NULL == di->coul_dev_ops)
	{
		hwlog_info("[%s]di is null\n",__FUNCTION__);
		return 0;
	}

    if (first_in){
        iavg_ma =  di->coul_dev_ops->get_battery_current_ua() / UA_PER_MA;
        first_in = 0;
    }
    else{
        delta_cc = curr_cc - last_cc;
        delta_time = curr_time - last_time;
        if(delta_time > 0)
            iavg_ma = (div_s64((s64)delta_cc * 3600, delta_time))/ UA_PER_MA;
        else
            iavg_ma =  di->coul_dev_ops->get_battery_current_ua() / UA_PER_MA;

        hwlog_info("wake_up delta_time=%ds, iavg_ma=%d\n", delta_time, iavg_ma);
    }

	if (abs(iavg_ma) >= 500) {
		curr2update_ocv[AVG_CURR_500MA].current_ma = abs(iavg_ma);
		curr2update_ocv[AVG_CURR_500MA].time = curr_time;
	}
	else if (abs(iavg_ma) >= 250) {
		curr2update_ocv[AVG_CURR_250MA].current_ma = abs(iavg_ma);
		curr2update_ocv[AVG_CURR_250MA].time = curr_time;
	}
	return iavg_ma;
}

/*******************************************************
  Function:        hisi_coul_suspend
  Description:     suspend function, called when coul enter sleep, v9 no sleep
  Input:           struct platform_device *pdev                     ---- platform device
  Output:          NULL
  Return:          NULL
********************************************************/
static int hisi_coul_suspend(struct platform_device *pdev,
	pm_message_t state)
{
    struct smartstar_coul_device *di = platform_get_drvdata(pdev);

    int current_sec = 0;
    int wakeup_time = 0;
    int sr_delta_temp;
	int wakeup_avg_current_ma = 0;

    if(NULL == di)
    {
        hwlog_info("[%s]di is null\n",__FUNCTION__);
        return 0;
    }

    hwlog_info("%s:+\n",__func__);
    current_sec = di->coul_dev_ops->get_coul_time();
    DI_LOCK();
    di->suspend_cc = di->coul_dev_ops->calculate_cc_uah();
    di->suspend_time = (unsigned int)current_sec;
    di->sr_suspend_time = current_sec;

    wakeup_time = current_sec - di->sr_resume_time;
    if (wakeup_time > SR_MAX_RESUME_TIME) {
        coul_clear_sr_time_array();
        hwlog_info("[SR]%s(%d): wakeup_time(%d) > SR_MAX_RESUME_TIME(%d)\n", __func__, __LINE__, wakeup_time, SR_MAX_RESUME_TIME);
		wakeup_avg_current_ma = calc_wakeup_avg_current(di, di->resume_cc, di->sr_resume_time, di->suspend_cc, di->sr_suspend_time);
	}
    else if (wakeup_time >= 0) {
        sr_time_wakeup[sr_index_wakeup] = wakeup_time;
        sr_index_wakeup++;
        sr_index_wakeup = sr_index_wakeup % SR_ARR_LEN;
		wakeup_avg_current_ma = calc_wakeup_avg_current(di, di->resume_cc, di->sr_resume_time, di->suspend_cc, di->sr_suspend_time);
    }
    else {
        hwlog_err("[SR]%s(%d): wakeup_time=%d, di->sr_suspend_time=%d, di->sr_resume_time=%d\n",
            __func__, __LINE__, wakeup_time, di->sr_suspend_time, di->sr_resume_time);
    }

    sr_cur_state = SR_DEVICE_SLEEP;
    DI_UNLOCK();
    hwlog_info("SUSPEND! cc=%d, time=%d, wakeup_avg_current:%d\n", di->suspend_cc,
                       di->suspend_time, wakeup_avg_current_ma);
    cancel_delayed_work(&di->read_temperature_delayed_work);
    if (di->batt_exist){
        cancel_delayed_work(&di->calculate_soc_delayed_work);
    }
    sr_delta_temp = sr_suspend_temp - last_cali_temp;
    if (sr_delta_temp < 0)
            sr_delta_temp = -sr_delta_temp;
    if (((TEMP_THRESHOLD_CALI*10) <= sr_delta_temp)
        || ((CALIBRATE_INTERVAL/1000) < (current_sec - last_cali_time))
		|| (0 > (current_sec - last_cali_time))) {
		/*this is to reduce calibrate times in frequently SR*/
		di->coul_dev_ops->cali_adc();
		last_cali_time = current_sec;
		last_cali_temp = sr_suspend_temp;
	}

	if (battery_is_removable) {
    	cancel_delayed_work(&di->battery_check_delayed_work);
	}
    coul_set_low_vol_int(di, LOW_INT_STATE_SLEEP);
	di->coul_dev_ops->enter_eco();
    hwlog_info("%s:-\n",__func__);
    return 0;
}

/* calculate last SR_TOTAL_TIME seconds duty ratio */
static int sr_get_duty_ratio(void) {
    int total_sleep_time = 0;
    int total_wakeup_time = 0;
    int last_sleep_idx  = (sr_index_sleep - 1 < 0) ? SR_ARR_LEN - 1 : sr_index_sleep - 1;
    int last_wakeup_idx = (sr_index_wakeup - 1 < 0) ? SR_ARR_LEN - 1 : sr_index_wakeup - 1;
    int cnt = 0;
    int duty_ratio = 0;

    do {
        total_sleep_time += sr_time_sleep[last_sleep_idx];
        total_wakeup_time += sr_time_wakeup[last_wakeup_idx];

        last_sleep_idx = (last_sleep_idx - 1 < 0) ? SR_ARR_LEN - 1 : last_sleep_idx - 1;
        last_wakeup_idx = (last_wakeup_idx - 1 < 0) ? SR_ARR_LEN - 1 : last_wakeup_idx - 1;

        cnt++;
        if (cnt >= SR_ARR_LEN) {
            break;
        }
    } while (total_sleep_time + total_wakeup_time < SR_TOTAL_TIME);

    /* calculate duty ratio */
    if (total_sleep_time + total_wakeup_time >= SR_TOTAL_TIME) {
        duty_ratio = total_sleep_time * 100 / (total_sleep_time + total_wakeup_time);
        hwlog_info("[SR]%s(%d): total_wakeup=%ds, total_sleep=%ds, duty_ratio=%d\n",
            __func__, __LINE__, total_wakeup_time, total_sleep_time, duty_ratio);
    }
    return duty_ratio;
}

static int get_big_current_10min(int time_now)
{
	u8 i;
	for (i = 0; i < AVG_CURR_MAX; i++) {
		if ((time_now - curr2update_ocv[i].time) < CURR2UPDATE_OCV_TIME)
			return curr2update_ocv[i].current_ma;
	}
	return -1;
}

static void clear_big_current_10min(void)
{
	u8 i;
	for (i = 0; i < AVG_CURR_MAX; i++) {
		 curr2update_ocv[i].current_ma = 0;
		 curr2update_ocv[i].time = 0;
	}
}
#ifdef CONFIG_HISI_DEBUG_FS
int control_ocv_level(int level, int val)
{
	if (level >=  OCV_LEVEL_MAX || level < OCV_LEVEL_0)
		return -1;
	if (val == 1)
		ocv_level_para[level].is_enabled = 1;
	else if (val == 0)
		ocv_level_para[level].is_enabled = 0;
	else
		return -1;
	hwlog_info("ocv level[%d]is set[%d]\n", level, ocv_level_para[level].is_enabled);
	return 0;
}

int print_multi_ocv_threshold(void)
{
 int i = 0;
 hwlog_info("%s++++",__func__);
 hwlog_info("duty_ratio_limit|sleep_time_limit|wake_time_limit|max_avg_curr_limit|temp_limit|ocv_gap_time_limit|delta_soc_limit|ocv_update_anyway|allow_fcc_update|is_enabled|\n");
 for (i = 0; i < OCV_LEVEL_MAX; i++) {
	hwlog_info(	"%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|\n",ocv_level_para[i].duty_ratio_limit,\
	ocv_level_para[i].sleep_time_limit, ocv_level_para[i].wake_time_limit,\
	ocv_level_para[i].max_avg_curr_limit,ocv_level_para[i].temp_limit,\
	ocv_level_para[i].ocv_gap_time_limit,ocv_level_para[i].delta_soc_limit,\
	ocv_level_para[i].ocv_update_anyway,ocv_level_para[i].allow_fcc_update,ocv_level_para[i].is_enabled);
 }
 hwlog_info("%s----",__func__);
 return 0;
}

u8 get_delta_soc(void)
{
	if (!g_test_delta_soc_renew_ocv)
		return delta_soc_renew_ocv;
	else
		return g_test_delta_soc_renew_ocv;
}

u8 set_delta_soc(u8 delta_soc)
{
    g_test_delta_soc_renew_ocv = delta_soc;
	hwlog_info("delta_soc is set[%d]\n", delta_soc);
    return g_test_delta_soc_renew_ocv;
}
#endif
static int save_multi_ocv_and_level(struct smartstar_coul_device *di)
{
	int cc = 0;

	if(NULL == di || NULL == di->coul_dev_ops)
	{
		hwlog_info("[%s]di is null\n",__FUNCTION__);
		return 0;
	}
    g_eco_leak_uah = di->coul_dev_ops->calculate_eco_leak_uah();
    di->coul_dev_ops->save_ocv_temp((short)di->batt_ocv_temp);
	if (ocv_level_para[di->last_ocv_level].allow_fcc_update) {
        di->batt_ocv_valid_to_refresh_fcc = 1;
        di->coul_dev_ops->save_ocv(di->batt_ocv, IS_UPDATE_FCC);
	} else {
		di->batt_ocv_valid_to_refresh_fcc = 0;
        di->coul_dev_ops->save_ocv(di->batt_ocv, NOT_UPDATE_FCC);
	}
	clear_big_current_10min();
	coul_clear_cc_register();
	coul_clear_coul_time();
	cc = di->coul_dev_ops->calculate_cc_uah();
	cc = cc + g_eco_leak_uah;
	di->coul_dev_ops->save_cc_uah(cc);
	di->coul_dev_ops->save_ocv_level(di->last_ocv_level);
	hwlog_info("awake from deep sleep, new OCV = %d, ocv level = %u, fcc_flag = %d\n", di->batt_ocv, di->last_ocv_level, di->batt_ocv_valid_to_refresh_fcc);
	return 0;
}

static int is_ocv_reach_renew_threshold(struct smartstar_coul_device *di, u8 last_ocv_level, u8 current_ocv_level, int ocv_now, int time_now, int batt_temp)
{
	int pc_now = 0;

	if(NULL == di)
	{
		hwlog_info("[%s]di is null\n",__FUNCTION__);
		return 0;
	}

	if (current_ocv_level <= last_ocv_level)
		return 1;
	pc_now = calculate_pc(di, ocv_now, batt_temp, di->batt_chargecycles/PERCENT);
    if (g_test_delta_soc_renew_ocv == 0)
        delta_soc_renew_ocv = abs(pc_now - di->batt_soc_real)/TENTH;
    else
        delta_soc_renew_ocv = g_test_delta_soc_renew_ocv;
	hwlog_info("[SR]%s(delta_soc:%d, time_now:%d)\n", __func__, delta_soc_renew_ocv, time_now);
	if ((time_now >= (ocv_level_para[current_ocv_level].ocv_gap_time_limit - ocv_level_para[last_ocv_level].ocv_gap_time_limit))
		&& (delta_soc_renew_ocv >= (ocv_level_para[current_ocv_level].delta_soc_limit - ocv_level_para[last_ocv_level].delta_soc_limit)))
		return 1;
	else
		return 0;
}

static u8  judge_ocv_threshold(int sleep_time, int wake_time, int sleep_current, int big_current_10min, int temp, int duty_ratio)
{
	u8 i;
	for (i = 0; i < OCV_LEVEL_MAX; i++) {
		if (TRUE == ocv_level_para[i].is_enabled
                && duty_ratio >= ocv_level_para[i].duty_ratio_limit
				&& sleep_time >= ocv_level_para[i].sleep_time_limit
				&& wake_time <= ocv_level_para[i].wake_time_limit
				&& sleep_current <= ocv_level_para[i].sleep_current_limit
				&& temp >= ocv_level_para[i].temp_limit
				&& big_current_10min <= ocv_level_para[i].max_avg_curr_limit) {
	        hwlog_info("[SR]%s(LEVEL:%d): sleep_time=%ds, wake_time=%ds, sleep_current=%dma, duty_ratio=%d, temp=%d, big_current=%d\n",
	            __func__, i, sleep_time, wake_time, sleep_current, duty_ratio, temp, big_current_10min);
			break;
			}
	}
	return i;
}

static int  calc_ocv_level(struct smartstar_coul_device *di, int time_now, int sleep_time, int wake_time, int sleep_current, int big_current_10min, int temp, int duty_ratio)
{
    /* judge if need update ocv */
	u8 cur_level;
	int ocv_now = 0;

	if(NULL == di)
	{
		hwlog_info("[%s]di is null\n",__FUNCTION__);
		return 0;
	}

	cur_level = judge_ocv_threshold(sleep_time, wake_time, sleep_current, big_current_10min, temp, duty_ratio);
	ocv_now = get_ocv_vol_from_fifo(di);
	if (0 == ocv_now)
		return 0;
	if (cur_level < LOW_PRECISION_OCV_LEVEL) {
		if( !ocv_level_para[cur_level].ocv_update_anyway
			&& TRUE == is_in_capacity_dense_area(ocv_now)){
    		hwlog_info("%s:do not update OCV(%d)\n", __func__, ocv_now);
            return 0;
        }
		di->last_ocv_level = cur_level;
		di->batt_ocv = ocv_now;
    	di->batt_ocv_temp = temp;
		return 1;
	}
	else if ((cur_level < OCV_LEVEL_MAX)
		&& is_ocv_reach_renew_threshold(di, di->last_ocv_level, cur_level, ocv_now, time_now, temp)){
		/*calculate low precision ocv*/
		if( !ocv_level_para[cur_level].ocv_update_anyway
			&& TRUE == is_in_capacity_dense_area(ocv_now)){
    		hwlog_info("%s:do not update OCV(%d)\n", __func__, ocv_now);
            return 0;
        }
		di->last_ocv_level = cur_level;
		di->batt_ocv = ocv_now;
    	di->batt_ocv_temp = temp;
		return 1;
	}
	return 0;
}
/*******************************************************
  Function:        ocv_could_update
  Description:     check whether MULTI OCV can update
  Input:           struct platform_device *pdev    ---- platform device
  Output:          NULL
  Return:          1: can update, 0: can not
********************************************************/
static int multi_ocv_could_update(struct smartstar_coul_device *di)
{
    int sleep_cc, sleep_current = 0;
    int sleep_time, time_now;
    int last_wakeup_time = 0;
    int last_sleep_time = 0;
    int duty_ratio = 0;
	int big_current = 0;

	if(NULL == di || NULL == di->coul_dev_ops)
	{
		hwlog_info("[%s]di is null\n",__FUNCTION__);
		return 0;
	}

    sleep_cc = di->coul_dev_ops->calculate_cc_uah();
    sleep_cc = sleep_cc - di->suspend_cc;  /* sleep uah */
    time_now = di->coul_dev_ops->get_coul_time();
    sleep_time = time_now - di->suspend_time; /* sleep time */
	/*get big wakeup current in 10 min*/
	big_current = get_big_current_10min(time_now);
	/*get sleep current*/
    /* ma = ua/1000 = uas/s/1000 = uah*3600/s/1000 = uah*18/(s*5) */
    if (sleep_time > 0)
        sleep_current = (sleep_cc * 18) / (sleep_time * 5);
    else {
        hwlog_err("[SR]%s(%d): sleep_time = %d\n",  __func__, __LINE__, sleep_time);
        return 0;
    }
    /* get last wakeup time */
    if (sr_index_wakeup >= 0 && sr_index_wakeup < SR_ARR_LEN)
        last_wakeup_time = (sr_index_wakeup - 1 < 0) ? sr_time_wakeup[SR_ARR_LEN - 1]: sr_time_wakeup[sr_index_wakeup - 1];
    /* get last sleep time */
    if (sr_index_sleep >= 0 && sr_index_sleep < SR_ARR_LEN)
        last_sleep_time = (sr_index_sleep - 1 < 0) ? sr_time_sleep[SR_ARR_LEN - 1]: sr_time_sleep[sr_index_sleep - 1];
    /* get last SR_TOTAL_TIME seconds duty ratio */
    duty_ratio = sr_get_duty_ratio();
    hwlog_info("[SR]going to update ocv, sleep_time=%ds, sleep_current=%d ma\n", sleep_time, sleep_current);
    return calc_ocv_level(di,time_now, last_sleep_time, last_wakeup_time, sleep_current, big_current, di->batt_temp, duty_ratio);
}

static int sr_need_update_ocv(struct smartstar_coul_device *di)
{
    int last_wakeup_time = 0;
    int last_sleep_time = 0;
    int duty_ratio = 0;

    /* get last wakeup time */
    if (sr_index_wakeup >= 0 && sr_index_wakeup < SR_ARR_LEN) {
        last_wakeup_time = (sr_index_wakeup - 1 < 0) ? sr_time_wakeup[SR_ARR_LEN - 1]: sr_time_wakeup[sr_index_wakeup - 1];
    }

    /* get last sleep time */
    if (sr_index_sleep >= 0 && sr_index_sleep < SR_ARR_LEN) {
        last_sleep_time = (sr_index_sleep - 1 < 0) ? sr_time_sleep[SR_ARR_LEN - 1]: sr_time_sleep[sr_index_sleep - 1];
    }

    /* get last SR_TOTAL_TIME seconds duty ratio */
    duty_ratio = sr_get_duty_ratio();

    /* judge if need update ocv */
    if (last_sleep_time > SR_DELTA_SLEEP_TIME &&
        last_wakeup_time < SR_DELTA_WAKEUP_TIME &&
        duty_ratio > SR_DUTY_RATIO ) {
        hwlog_info("[SR]%s(%d): need_update, last_sleep=%ds, last_wakeup=%ds, duty_ratio=%d\n",
            __func__, __LINE__, last_sleep_time, last_wakeup_time, duty_ratio);
        return 1;
    }
    else {
        hwlog_info("[SR]%s(%d): no_need_update, last_sleep=%ds, last_wakeup=%ds, duty_ratio=%d\n",
            __func__, __LINE__, last_sleep_time, last_wakeup_time, duty_ratio);
        return 0;
    }
}

/*******************************************************
  Function:        ocv_could_update
  Description:     check whether OCV can update
  Input:           struct platform_device *pdev    ---- platform device
  Output:          NULL
  Return:          1: can update, 0: can not
  Remark:         update condition----sleep_time > 10min && sleep_current < 50mA
********************************************************/
static int ocv_could_update(struct smartstar_coul_device *di)
{
    int sleep_cc, sleep_current = 0;
    int sleep_time, time_now;

    sleep_cc = di->coul_dev_ops->calculate_cc_uah();
    sleep_cc = sleep_cc - di->suspend_cc;  /* sleep uah */
    time_now = di->coul_dev_ops->get_coul_time();
    sleep_time = time_now - di->suspend_time; /* sleep time */

    if ((sleep_time < (delta_sleep_time - delta_sleep_time_offset)) && !sr_need_update_ocv(di))
    {
        hwlog_info("[SR]Can't update ocv, sleep_time=%d s\n", sleep_time);
        return 0;
    }

    /* ma = ua/1000 = uas/s/1000 = uah*3600/s/1000 = uah*18/(s*5) */
    if (sleep_time > 0) {
        sleep_current = (sleep_cc * 18) / (sleep_time * 5);

        if (sleep_current > delta_sleep_current)
        {
            hwlog_info("[SR]Can't update ocv, sleep_current=%d ma, sleep_time=%d s\n", sleep_current, sleep_time);
            return 0;
        }
    }
    else {
        hwlog_err("[SR]%s(%d): sleep_time = %d\n",  __func__, __LINE__, sleep_time);
        return 0;
    }

    hwlog_info("[SR]going to update ocv, sleep_time=%ds, sleep_current=%d ma\n", sleep_time, sleep_current);
    return 1;
}

/*******************************************************
  Function:        get_ocv_resume
  Description:     get ocv after resuming
  Input:            struct platform_device *pdev    ---- platform device
  Output:          NULL
  Return:          1: can update, 0: can not
  Remark:         update condition----sleep_time > 10min && sleep_current < 50mA
********************************************************/
static void get_ocv_resume(struct smartstar_coul_device *di)
{
    int cc;

    g_eco_leak_uah = di->coul_dev_ops->calculate_eco_leak_uah();
    get_ocv_by_vol(di);
    cc = di->coul_dev_ops->calculate_cc_uah();
	cc = cc + g_eco_leak_uah;
	di->coul_dev_ops->save_cc_uah(cc);
	return;
}
/*******************************************************
  Function:        hisi_coul_resume
  Description:     suspend function, called when coul wakeup from deep sleep
  Input:           struct platform_device *pdev                     ---- platform device
  Output:          NULL
  Return:          NULL
********************************************************/
static int hisi_coul_resume(struct platform_device *pdev)
{
    struct smartstar_coul_device *di = platform_get_drvdata(pdev);

    int current_sec = 0;
    int sr_sleep_time = 0;
    int old_soc = 0;

    if(NULL == di)
    {
        hwlog_info("[%s]di is null\n",__FUNCTION__);
        return 0;
    }
    hwlog_info("%s:+\n",__func__);
    current_sec = di->coul_dev_ops->get_coul_time();
    update_battery_temperature(di, TEMPERATURE_INIT_STATUS);
    sr_sleep_time = current_sec - di->sr_suspend_time;

    disable_temperature_debounce = 0;
    coul_set_low_vol_int(di, LOW_INT_STATE_RUNNING);
    DI_LOCK();
    sr_cur_state = SR_DEVICE_WAKEUP;
    di->sr_resume_time = current_sec;
    /* record sleep time */
    if (sr_sleep_time >= 0) {
        sr_time_sleep[sr_index_sleep] = sr_sleep_time;
        sr_index_sleep++;
        sr_index_sleep = sr_index_sleep % SR_ARR_LEN;
    }
    else {
        hwlog_err("[SR]%s(%d): sr_sleep_time = %d\n", __func__, __LINE__, sr_sleep_time);
    }

    if ((current_sec - di->charging_stop_time > 30*60)
		&& multi_ocv_open_flag && multi_ocv_could_update(di)){
		record_ocv_cali_info(di);
        save_multi_ocv_and_level(di);
    } else if((current_sec - di->charging_stop_time > 30*60)
    	&& !multi_ocv_open_flag && ocv_could_update(di)) {
		get_ocv_resume(di);
		di->last_ocv_level = OCV_LEVEL_0;
		di->coul_dev_ops->save_ocv_level(di->last_ocv_level);
		g_coul_ocv_cali_info[g_ocv_cali_index].cali_ocv_level = di->last_ocv_level;
	}
    else if (di->batt_delta_rc > di->batt_data->fcc*ABNORMAL_DELTA_SOC*10
        && di->charging_state != CHARGING_STATE_CHARGE_START /*lint !e574*/
        && (current_sec - di->charging_stop_time > 30*60)){
        int old_ocv = di->batt_ocv;
    	hwlog_info("Update ocv for delta_rc(%d)!\n", di->batt_delta_rc);
        //get_ocv_by_vol(di);
        get_ocv_resume(di);
		di->last_ocv_level =  INVALID_SAVE_OCV_LEVEL;
		di->coul_dev_ops->save_ocv_level(di->last_ocv_level);
		g_coul_ocv_cali_info[g_ocv_cali_index].cali_ocv_level = di->last_ocv_level;
        if (old_ocv != di->batt_ocv){
            di->coul_dev_ops->save_ocv(di->batt_ocv, NOT_UPDATE_FCC);/*for set NOT_UPDATE_fCC Flag*/
            di->batt_ocv_valid_to_refresh_fcc = 0;
        }
    }
	di->coul_dev_ops->exit_eco();
    di->soc_limit_flag = 2;
    di->soc_monitor_flag = 2;
    old_soc = di->batt_soc;
    di->batt_soc = calculate_state_of_charge(di);
    /*dmd report*/
    if (abs(old_soc - di->batt_soc) >= SOC_JUMP_MAX) {
        char buff[DSM_BUFF_SIZE_MAX] = {0};
        /* current information --- fcc, temp, old_soc, new_soc */
        snprintf(buff, (size_t)DSM_BUFF_SIZE_MAX, "[resume jump]fcc:%d, temp:%d, old_soc:%d, new_soc:%d",
        di->batt_fcc, di->batt_temp, old_soc, di->batt_soc);
        coul_dsm_report_ocv_cali_info(di, ERROR_RESUME_SOC_JUMP, buff);
    }

    di->soc_limit_flag = 1;
	di->resume_cc = di->coul_dev_ops->calculate_cc_uah();
    if ((LOW_TEMP_OPT_OPEN == low_temp_opt_flag) && (di->batt_temp < 50))
        di->soc_work_interval = CALCULATE_SOC_MS/4;
    else
        di->soc_work_interval = CALCULATE_SOC_MS/2;

    DI_UNLOCK();
    if (di->batt_exist){
        queue_delayed_work(system_power_efficient_wq, &di->read_temperature_delayed_work, round_jiffies_relative(msecs_to_jiffies(READ_TEMPERATURE_MS)));
        queue_delayed_work(system_power_efficient_wq, &di->calculate_soc_delayed_work, round_jiffies_relative(msecs_to_jiffies((unsigned int)di->soc_work_interval)));
    }
	if (battery_is_removable) {
	    queue_delayed_work(system_power_efficient_wq, &di->battery_check_delayed_work,
                round_jiffies_relative(msecs_to_jiffies(BATTERY_CHECK_TIME_MS)));
	}
    if (ENABLED == di->iscd->enable &&
         CHARGING_STATE_CHARGE_DONE == di->charging_state) {
        hrtimer_start(&di->iscd->timer, ktime_set((unsigned long)0, (unsigned long)0), HRTIMER_MODE_REL);
    }

    hwlog_info("%s:-\n",__func__);
        return 0;
}
#endif
static struct of_device_id hisi_coul_core_match_table[] =
{
    {
          .compatible = "hisi,coul_core",
    },
    { /*end*/},
};

static void hisi_coul_shutdown(struct platform_device *pdev)
{
    struct smartstar_coul_device *di = platform_get_drvdata(pdev);
    int last_soc = hisi_bci_show_capacity();

    hwlog_err("hisi_coul_shutdown start!\n");
    if (NULL == di)
    {
        hwlog_err("[coul_shutdown]:di is NULL\n");
        return;
    }
    if (last_soc >= 0){
        di->coul_dev_ops->save_last_soc(last_soc);
    }
    cancel_delayed_work(&di->calculate_soc_delayed_work);
    cancel_delayed_work(&di->read_temperature_delayed_work);
    if (battery_is_removable)
        cancel_delayed_work(&di->battery_check_delayed_work);
    hwlog_err("hisi_coul_shutdown end!\n");
}


static struct platform_driver hisi_coul_core_driver = {
	.probe		= hisi_coul_probe,
	.remove		= hisi_coul_remove,
	.shutdown   = hisi_coul_shutdown,
#ifdef CONFIG_PM
	.suspend	= hisi_coul_suspend,
	.resume		= hisi_coul_resume,
#endif
	.driver		= {
	.name		= "hisi_coul_core",
       .owner          = THIS_MODULE,
       .of_match_table = hisi_coul_core_match_table,
	},
};

int __init hisi_coul_core_init(void)
{
    return platform_driver_register(&hisi_coul_core_driver);
}

void __exit hisi_coul_core_exit(void)
{
    platform_driver_unregister(&hisi_coul_core_driver);
}

fs_initcall(hisi_coul_core_init);
module_exit(hisi_coul_core_exit);


#ifdef SMARTSTAR_DEBUG
void ss_di_show(void)
{
    struct smartstar_coul_device *di = (struct smartstar_coul_device *)g_smartstar_coul_dev;

    hwlog_err("batt_exist = 0x%x\n", di->batt_exist);
    hwlog_err("prev_pc_unusable = %d\n", di->prev_pc_unusable);
    //hwlog_err("irqs = %d\n", di->irqs);
    hwlog_err("batt_ocv = %d\n", di->batt_ocv);
    hwlog_err("batt_changed_flag = %d\n", di->batt_changed_flag);
    hwlog_err("soc_limit_flag = %d\n", di->soc_limit_flag);
    hwlog_err("batt_temp = %d\n", di->batt_temp);
    hwlog_err("batt_fcc = %d\n", di->batt_fcc);
    hwlog_err("batt_limit_fcc = %d\n", di->batt_limit_fcc);
    hwlog_err("batt_rm = %d\n", di->batt_rm);
    hwlog_err("batt_ruc = %d\n", di->batt_ruc);
    hwlog_err("batt_uuc = %d\n", di->batt_uuc);
    hwlog_err("rbatt = %d\n", di->rbatt);
    hwlog_err("r_pcb = %d\n", di->r_pcb);
    hwlog_err("soc_work_interval = %d\n", di->soc_work_interval);
    hwlog_err("charging_begin_soc = %d\n", di->charging_begin_soc);
    hwlog_err("charging_state = %d\n", di->charging_state);
    hwlog_err("batt_soc = %d\n", di->batt_soc);
    hwlog_err("batt_soc_real = %d\n", di->batt_soc_real);
    hwlog_err("batt_soc_with_uuc = %d\n", di->batt_soc_with_uuc);
    hwlog_err("batt_soc_est = %d\n", di->batt_soc_est);
    hwlog_err("batt_id_vol = %d\n", di->batt_id_vol);
    hwlog_err("batt_chargecycles = %d\n", di->batt_chargecycles);
    hwlog_err("last_cali_temp = %d\n", di->last_cali_temp);
    hwlog_err("cc_end_value = %d\n", di->cc_end_value);
    //hwlog_err("cc_start_value = %d\n", di->cc_start_value);
    hwlog_err("v_cutoff = %d\n", di->v_cutoff);
    hwlog_err("v_low_int_value = %d\n", di->v_low_int_value);
    hwlog_err("get_cc_end_time = %d\n", di->get_cc_end_time);
    hwlog_err("suspend_time = %d\n", di->suspend_time);
    hwlog_err("charging_begin_cc = %d\n", di->charging_begin_cc);
    hwlog_err("suspend_cc = %d\n", di->suspend_cc);
	hwlog_err("resume_cc = %d\n", di->resume_cc);
    hwlog_err("last_time = %d\n", di->last_time);
    hwlog_err("last_cc = %d\n", di->last_cc);
    hwlog_err("last_iavg_ma = %d\n", di->last_iavg_ma);
    hwlog_err("fcc_real_mah = %d\n", di->fcc_real_mah);
    hwlog_err("is_nv_read = %d\n", di->is_nv_read);
    hwlog_err("is_nv_need_save = %d\n", di->is_nv_need_save);
    hwlog_err("dbg_ocv_cng_0 = %d\n",di->dbg_ocv_cng_0);
    hwlog_err("dbg_ocv_cng_1 = %d\n",di->dbg_ocv_cng_1);
    hwlog_err("dbg_valid_vol = %d\n",di->dbg_valid_vol);
    hwlog_err("dbg_invalid_vol = %d\n",di->dbg_invalid_vol);
    hwlog_err("dbg_ocv_fc_failed = %d\n",di->dbg_ocv_fc_failed);
}

static int coul_running = 1;	// 1 is running, 0 is suspend
static int coul_state_ops_set(const char *buffer,
							  const struct kernel_param *kp)
{
	int run = 0;
	run = buffer[0] - '0';
	if (run && !coul_running)
	{
		coul_running = 1;
		hisi_coul_resume(g_pdev);
	}
	else if (!run && coul_running)
	{
		pm_message_t pm = { 0 };
		coul_running = 0;
		hisi_coul_suspend(g_pdev, pm);
	}
	return 0;
}

static int coul_state_ops_get(char *buffer, const struct kernel_param *kp)
{
	snprintf(buffer, PAGE_SIZE, "%d", coul_running);
	return strlen(buffer);
}

static struct kernel_param_ops coul_state_ops = {
	.set = coul_state_ops_set,
	.get = coul_state_ops_get,
};

module_param_cb(coul_running, &coul_state_ops, &coul_running, 0644);

#endif

MODULE_AUTHOR("HISILICON");
MODULE_DESCRIPTION("hisi coul core driver");
MODULE_LICENSE("GPL");
