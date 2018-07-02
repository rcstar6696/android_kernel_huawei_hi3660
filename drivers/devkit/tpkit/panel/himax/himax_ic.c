/*< CPM2016032400222 shihuijun 20160324 begin */
/* Himax Android Driver Sample Code for Himax chipset
*
* Copyright (C) 2014 Himax Corporation.
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

#include "himax_ic.h"

#ifdef CONFIG_APP_INFO
#include <misc/app_info.h>
#endif

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <linux/hw_dev_dec.h>
#endif

static int	HX_TOUCH_INFO_POINT_CNT = 0;
#define RETRY_TIMES 200

#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)

#ifdef HX_TP_SYS_DIAG
	int  touch_monitor_stop_flag = 0;
	uint8_t diag_coor[HX_RECEIVE_BUF_MAX_SIZE] = {0};
#endif

#endif

struct himax_ts_data *g_himax_ts_data = NULL;

#ifdef CONFIG_HUAWEI_DSM
struct dsm_dev dsm_hmx_tp = {
	.name = "dsm_i2c_bus",	// dsm client name
	.fops = NULL,
	.buff_size = TP_RADAR_BUF_MAX,
};
struct hmx_dsm_info hmx_tp_dsm_info = {0};
struct dsm_client *hmx_tp_dclient = NULL;
#endif

uint8_t IC_STATUS_CHECK	  	= 0xAA;
uint8_t HW_RESET_ACTIVATE  	= 1;	//struct himax_ts_data *g_himax_ts_data;
static uint8_t 	EN_NoiseFilter	= 0x00;
static uint8_t	Last_EN_NoiseFilter = 0x00;
static int	hx_real_point_num = 0;		//real point report number	// for himax_ts_work_func use

#ifdef HX_ESD_WORKAROUND
uint8_t ESD_R36_FAIL = 0;
uint8_t g_check_r36h_flag = 0;
static uint8_t ESD_RESET_ACTIVATE = 1;
#endif

#ifdef HX_CHIP_STATUS_MONITOR
static int HX_POLLING_TIMER  = 5;//unit:sec
int HX_ON_HAND_SHAKING    = 0;
#endif

uint8_t *self_data = 0;
uint8_t *mutual_data = 0;

#define HIMAX_VENDER_NAME  "himax"
char himax_product_id[HX_PROJECT_ID_LEN+1]={"999999999"};
static u8 himax_tp_color=0xff;
extern u8 cypress_ts_kit_color[TP_COLOR_SIZE];

int himax_input_config(struct input_dev* input_dev); //himax_input_register(struct himax_ts_data *ts);
static void himax_shutdown(void);
static void himax_power_off_gpio_set(void);

static int himax_init_chip(void);
static int himax_power_off(void);
static int himax_core_suspend(void);
static int himax_core_resume(void);
static int himax_reset_device(void);

static int himax_parse_dts(struct device_node *device, struct ts_kit_device_data *chip_data);
static int himax_chip_detect(struct ts_kit_platform_data *platform_data);
static int himax_irq_top_half(struct ts_cmd_node *cmd);
static int himax_irq_bottom_half(struct ts_cmd_node *in_cmd,struct ts_cmd_node *out_cmd);
static int himax_chip_get_info(struct ts_chip_info_param *info);
static int himax_register_algo(struct ts_kit_device_data *dev_data);
static int himax_algo_cp(struct ts_kit_device_data *dev_data, struct ts_fingers *in_info, struct ts_fingers *out_info);
static int himax_get_capacitance_test_type(struct ts_test_type_info *info);
static int himax_get_rawdata(struct ts_rawdata_info *info, struct ts_cmd_node *out_cmd);

extern int get_boot_into_recovery_flag(void);
extern int himax_factory_start(struct himax_ts_data *ts,struct ts_rawdata_info *info_top);

struct ts_device_ops ts_kit_himax_ops = {
	.chip_parse_config =  himax_parse_dts,
	.chip_detect = himax_chip_detect,
	.chip_init =  himax_init_chip,
	.chip_register_algo = himax_register_algo,
	.chip_input_config = himax_input_config,
	.chip_irq_top_half =  himax_irq_top_half,
	.chip_irq_bottom_half =  himax_irq_bottom_half,
	.chip_suspend = himax_core_suspend,
	.chip_resume = himax_core_resume,
	.chip_reset= himax_reset_device,
	.chip_fw_update_boot = himax_fw_update_boot,
	.chip_fw_update_sd = himax_fw_update_sd,
	.chip_get_info = himax_chip_get_info,
	.chip_get_rawdata = himax_get_rawdata,
	.chip_get_capacitance_test_type = himax_get_capacitance_test_type,
	.chip_shutdown = himax_shutdown,/*NOT tested*/
 };

static int himax_get_rawdata(struct ts_rawdata_info *info, struct ts_cmd_node *out_cmd)
{
	int retval = NO_ERR;
	if((NULL == info)||(NULL == out_cmd)) {
		return HX_ERROR;
	}
	TS_LOG_INFO("%s: Entering\n",__func__);
	retval = himax_factory_start(g_himax_ts_data,info);
	TS_LOG_INFO("%s: End\n",__func__);

	return retval;
}

static int himax_get_capacitance_test_type(struct ts_test_type_info *info)
{
	TS_LOG_INFO("%s enter\n", __func__);
	if (!info){
		TS_LOG_ERR("%s\n", __func__);
		return INFO_FAIL;
	}
	strncpy(info->tp_test_type, g_himax_ts_data->tskit_himax_data
		->ts_platform_data->chip_data->tp_test_type, TS_CAP_TEST_TYPE_LEN);
	TS_LOG_INFO("%s:test_type=%s\n", __func__, info->tp_test_type);
	return NO_ERR;
}

int himax_hand_shaking(void)    //0:Running, 1:Stop, 2:I2C Fail
{
	int retval = 0;
	int result = 0;
	uint8_t buf0[HX_HAND_SHAKING_WRITE_MAX_SIZE] = {0};
	uint8_t hw_reset_check[HX_HAND_SHAKING_READ_MAX_SIZE] = {0};
	uint8_t hw_reset_check_2[HX_HAND_SHAKING_READ_MAX_SIZE] = {0};
	/*write 0xAA,back 0X55;write 0x55,back 0xAA,shaking success*/
	buf0[0] = HX_CMD_SETIDLE;
	if (IC_STATUS_CHECK == 0xAA) {
		buf0[1] = 0xAA;
		IC_STATUS_CHECK = 0x55;
	} else {
		buf0[1] = 0x55;
		IC_STATUS_CHECK = 0xAA;
	}

	retval = i2c_himax_master_write(buf0, HX_HAND_SHAKING_WRITE_MAX_SIZE, sizeof(buf0), DEFAULT_RETRY_CNT);
	if (retval < 0) {
		TS_LOG_ERR("[Himax]:write 0xF2 failed line: %d \n",__LINE__);
		goto work_func_send_i2c_msg_fail;
	}
	msleep(HX_SLEEP_50MS);

	buf0[0] = HX_CMD_SETIDLE;
	buf0[1] = 0x00;
	retval = i2c_himax_master_write( buf0, HX_HAND_SHAKING_WRITE_MAX_SIZE, sizeof(buf0), DEFAULT_RETRY_CNT);
	if (retval < 0) {
		TS_LOG_ERR("[Himax]:write 0xF2 failed line: %d \n",__LINE__);
		goto work_func_send_i2c_msg_fail;
	}
	msleep(HX_SLEEP_2MS);

	retval = i2c_himax_read(HX_REG_IC_VER, hw_reset_check, HX_HAND_SHAKING_READ_MAX_SIZE, sizeof(hw_reset_check), DEFAULT_RETRY_CNT);
	if (retval < 0) {
		TS_LOG_ERR("[Himax]:i2c_himax_read 0xD1 failed line: %d \n",__LINE__);
		goto work_func_send_i2c_msg_fail;
	}

	if ((IC_STATUS_CHECK != hw_reset_check[0])) {
		msleep(HX_SLEEP_2MS);
		retval = i2c_himax_read(HX_REG_IC_VER, hw_reset_check_2, HX_HAND_SHAKING_READ_MAX_SIZE, sizeof(hw_reset_check_2), DEFAULT_RETRY_CNT);
		if (retval < 0) {
			TS_LOG_ERR("[Himax]:i2c_himax_read 0xD1 failed line: %d \n",__LINE__);
			goto work_func_send_i2c_msg_fail;
		}

		if (hw_reset_check[0] == hw_reset_check_2[0]) {
			result = HX_HAND_SHAKING_STOP;
		} else {
			result = HX_HAND_SHAKING_RUNNING;
		}
	} else {
		result = HX_HAND_SHAKING_RUNNING;
	}

	return result;

work_func_send_i2c_msg_fail:
	return HX_HAND_SHAKING_I2C_FAIL;
}

static int himax_irq_top_half(struct ts_cmd_node *cmd)
{
	 cmd->command = TS_INT_PROCESS;
	 return NO_ERR;
}

int himax_input_config(struct input_dev* input_dev)//himax_input_register(struct himax_ts_data *ts)
{
	TS_LOG_INFO("%s: himax_input_config called\n", __func__);
	if(NULL == input_dev) {
		return HX_ERROR;
	}
	g_himax_ts_data->input_dev = input_dev;

	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

	TS_LOG_INFO("input_set_abs_params: min_x %d, max_x %d, min_y %d, max_y %d\n",
		g_himax_ts_data->pdata->abs_x_min, g_himax_ts_data->pdata->abs_x_max, g_himax_ts_data->pdata->abs_y_min, g_himax_ts_data->pdata->abs_y_max);

	input_set_abs_params(input_dev, ABS_MT_POSITION_X,g_himax_ts_data->pdata->abs_x_min, g_himax_ts_data->pdata->abs_x_max, g_himax_ts_data->pdata->abs_x_fuzz, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,g_himax_ts_data->pdata->abs_y_min, g_himax_ts_data->pdata->abs_y_max, g_himax_ts_data->pdata->abs_y_fuzz, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR,g_himax_ts_data->pdata->abs_pressure_min, g_himax_ts_data->pdata->abs_pressure_max, g_himax_ts_data->pdata->abs_pressure_fuzz, 0);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID,0,g_himax_ts_data->nFinger_support, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE,g_himax_ts_data->pdata->abs_pressure_min, g_himax_ts_data->pdata->abs_pressure_max, g_himax_ts_data->pdata->abs_pressure_fuzz, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR,g_himax_ts_data->pdata->abs_width_min, g_himax_ts_data->pdata->abs_width_max, g_himax_ts_data->pdata->abs_pressure_fuzz, 0);
	return NO_ERR;
}

static int himax_reset_device(void)
{
	int retval = NO_ERR;
	himax_HW_reset(HX_LOADCONFIG_EN,HX_INT_DISABLE);
	return retval;
}

static void calcDataSize(uint8_t finger_num)
{
	struct himax_ts_data *ts_data = g_himax_ts_data;
	ts_data->coord_data_size = HX_COORD_BYTE_NUM * finger_num;// 1 coord 4 bytes.
	ts_data->area_data_size = ((finger_num / HX_COORD_BYTE_NUM) + (finger_num % HX_COORD_BYTE_NUM ? 1 : 0)) * HX_COORD_BYTE_NUM;  // 1 area 4 finger ?
	ts_data->raw_data_frame_size = HX_RECEIVE_BUF_MAX_SIZE - ts_data->coord_data_size - ts_data->area_data_size - 4 - 4 - 1;
	if( ts_data->raw_data_frame_size == 0 )
	{
		TS_LOG_ERR("%s: could not calculate!\n", __func__);
		return;
	}
	ts_data->raw_data_nframes  = ((uint32_t)ts_data->x_channel * ts_data->y_channel +
									ts_data->x_channel + ts_data->y_channel) / ts_data->raw_data_frame_size +
									(((uint32_t)ts_data->x_channel * ts_data->y_channel +
									ts_data->x_channel + ts_data->y_channel) % ts_data->raw_data_frame_size)? 1 : 0;
	TS_LOG_INFO("%s: coord_data_size: %d, area_data_size:%d, raw_data_frame_size:%d, raw_data_nframes:%d", __func__,
				ts_data->coord_data_size, ts_data->area_data_size, ts_data->raw_data_frame_size, ts_data->raw_data_nframes);
}

static void calculate_point_number(void)
{
	HX_TOUCH_INFO_POINT_CNT = HX_MAX_PT * HX_COORD_BYTE_NUM;

	if ( (HX_MAX_PT % 4) == 0)
		HX_TOUCH_INFO_POINT_CNT += (HX_MAX_PT / HX_COORD_BYTE_NUM) * HX_COORD_BYTE_NUM;
	else
		HX_TOUCH_INFO_POINT_CNT += ((HX_MAX_PT /HX_COORD_BYTE_NUM) +1) * HX_COORD_BYTE_NUM;
}

static uint8_t himax_read_Sensor_ID(void)
{
	uint8_t val_high[HX_READ_SENSOR_ID_READ_MAX_SIZE]={0};
	uint8_t val_low[HX_READ_SENSOR_ID_READ_MAX_SIZE]={0};
	uint8_t ID0=0;
	uint8_t ID1=0;
	uint8_t sensor_id=0;
	int retval =0;
	uint8_t data[HX_READ_SENSOR_ID_WRITE_MAX_SIZE]={0};

	data[0] = HX_REG_ID_PIN_DEF; data[1] = 0x02; data[2] = 0x02;/*ID pin PULL High*/
	retval = i2c_himax_master_write(&data[0],HX_READ_SENSOR_ID_WRITE_MAX_SIZE, sizeof(data),DEFAULT_RETRY_CNT);
	if(retval <0) {
		return sensor_id;
	}
	msleep(HX_SLEEP_1MS);

	/*read id pin high*/
	retval = i2c_himax_read(HX_REG_ID_PIN_STATUS, val_high, HX_READ_SENSOR_ID_READ_MAX_SIZE, sizeof(val_high), DEFAULT_RETRY_CNT);
	if(retval <0) {
		return sensor_id;
	}
	data[0] = HX_REG_ID_PIN_DEF; data[1] = 0x01; data[2] = 0x01;/*ID pin PULL Low*/
	retval =i2c_himax_master_write(&data[0],HX_READ_SENSOR_ID_WRITE_MAX_SIZE, sizeof(data), DEFAULT_RETRY_CNT);
	if(retval <0) {
		return sensor_id;
	}
	msleep(HX_SLEEP_1MS);

	/*read id pin low*/
	retval = i2c_himax_read(HX_REG_ID_PIN_STATUS, val_low, HX_READ_SENSOR_ID_READ_MAX_SIZE, sizeof(val_low), DEFAULT_RETRY_CNT);
	if(retval <0) {
		return sensor_id;
	}
	if((val_high[0] & 0x01) ==0)
		ID0=0x02;/*GND*/
	else if((val_low[0] & 0x01) ==0)
		ID0=0x01;/*Floating*/
	else
		ID0=0x04;/*VCC*/

	if((val_high[0] & 0x02) ==0)
		ID1=0x02;/*GND*/
	else if((val_low[0] & 0x02) ==0)
		ID1=0x01;/*Floating*/
	else
		ID1=0x04;/*VCC*/
	if((ID0==0x04)&&(ID1!=0x04))
		{
			data[0] = HX_REG_ID_PIN_DEF; data[1] = 0x02; data[2] = 0x01;/*ID pin PULL High,Low*/
			retval = i2c_himax_master_write( &data[0],HX_READ_SENSOR_ID_WRITE_MAX_SIZE, sizeof(data), DEFAULT_RETRY_CNT);
			if(retval <0) {
				return sensor_id;
			}
			msleep(HX_SLEEP_1MS);
		}
	else if((ID0!=0x04)&&(ID1==0x04))
		{
			data[0] = HX_REG_ID_PIN_DEF; data[1] = 0x01; data[2] = 0x02;/*ID pin PULL Low,High*/
			retval =i2c_himax_master_write( &data[0],HX_READ_SENSOR_ID_WRITE_MAX_SIZE, sizeof(data), DEFAULT_RETRY_CNT);
			if(retval <0) {
				return sensor_id;
			}
			msleep(HX_SLEEP_1MS);

		}
	else if((ID0==0x04)&&(ID1==0x04))
		{
			data[0] = HX_REG_ID_PIN_DEF; data[1] = 0x02; data[2] = 0x02;/*ID pin PULL High,High*/
			retval = i2c_himax_master_write(&data[0],HX_READ_SENSOR_ID_WRITE_MAX_SIZE, sizeof(data), DEFAULT_RETRY_CNT);
			if(retval <0) {
				return sensor_id;
			}
			msleep(HX_SLEEP_1MS);

		}
	sensor_id=(ID1<<4)|ID0;

	data[0] = HX_CMD_SETIDLEDELAY; data[1] = sensor_id;
	retval = i2c_himax_master_write(&data[0],2, sizeof(data),DEFAULT_RETRY_CNT);/*Write to MCU*/
	if(retval <0) {
		return sensor_id;
	}
	msleep(HX_SLEEP_1MS);

	return sensor_id;

}

void himax_touch_information(void)
{
	uint8_t data[HX_TOUCH_INFORMATION_MAX_SIZE] = {0};
	int retval = 0;

	TS_LOG_INFO("%s:IC_TYPE =%d\n", __func__,IC_TYPE);

	if(IC_TYPE == HX_85XX_ES_SERIES_PWON)
		{
		data[0] = HX_REG_SRAM_SWITCH;
		/*sram E series register*/
		data[1] = 0x14;
		retval = i2c_himax_master_write(&data[0],2, sizeof(data),DEFAULT_RETRY_CNT);
		if(retval <0) {
			return;
		}
		msleep(HX_SLEEP_10MS);
		data[0] = HX_REG_SRAM_ADDR;
		/*sram start addr*/
		data[1] = 0x00;
		data[2] = 0x70;
		retval = i2c_himax_master_write(&data[0],3, sizeof(data),DEFAULT_RETRY_CNT);
		if(retval <0) {
			return;
		}
		msleep(HX_SLEEP_10MS);
		retval = i2c_himax_read(HX_REG_FLASH_RPLACE, data, 12, sizeof(data), DEFAULT_RETRY_CNT);
		if(retval <0) {
			return;
		}
			HX_RX_NUM = data[0];				 // FE(70)
			HX_TX_NUM = data[1];				 // FE(71)
			HX_MAX_PT = (data[2] & 0xF0) >> 4; // FE(72)
			if((data[4] & 0x04) == 0x04) {//FE(74)
				HX_XY_REVERSE = true;
			} else {
				HX_XY_REVERSE = false;
			}
		/*lint -save -e* */
		HX_X_RES = data[6]*256 + data[7]; //FE(76),FE(77)
		HX_Y_RES = data[8]*256 + data[9]; //FE(78),FE(79)
		/*lint -restore*/
		data[0] = HX_REG_SRAM_SWITCH;
		data[1] = 0x00;
		retval = i2c_himax_master_write(&data[0],2, sizeof(data),DEFAULT_RETRY_CNT);
		if(retval <0) {
			return;
		}
		msleep(HX_SLEEP_10MS);
		data[0] = HX_REG_SRAM_SWITCH;
		/*sram E series register*/
		data[1] = 0x14;
		retval = i2c_himax_master_write(&data[0],2, sizeof(data),DEFAULT_RETRY_CNT);
		if(retval <0) {
			return;
		}
		msleep(HX_SLEEP_10MS);
		data[0] = HX_REG_SRAM_ADDR;
		/*sram start addr*/
		data[1] = 0x00;
		data[2] = 0x02;
		retval = i2c_himax_master_write(&data[0],3, sizeof(data),DEFAULT_RETRY_CNT);
		if(retval <0) {
			return;
		}
		msleep(HX_SLEEP_10MS);
		data[0] = HX_REG_SRAM_SWITCH;
		/*disable sram test mode*/
		data[1] = 0x00;
		retval = i2c_himax_master_write(&data[0],2, sizeof(data),DEFAULT_RETRY_CNT);
		if(retval <0) {
			return;
		}
		msleep(HX_SLEEP_10MS);
			TS_LOG_INFO("%s:HX_RX_NUM =%d,HX_TX_NUM =%d,HX_MAX_PT=%d \n", __func__,HX_RX_NUM,HX_TX_NUM,HX_MAX_PT);
		}
		else if(IC_TYPE == HX_85XX_F_SERIES_PWON)
		{
			HX_RX_NUM				= HX8529_RX_NUM;
			HX_TX_NUM				= HX8529_TX_NUM;
			HX_BT_NUM				= HX8529_BT_NUM;
			HX_X_RES				= HX8529_X_RES;
			HX_Y_RES				= HX8529_Y_RES;
			HX_MAX_PT				= HX8529_MAX_PT;
			HX_XY_REVERSE			= true;
		}
		else
		{
			HX_RX_NUM				= 0;
			HX_TX_NUM				= 0;
			HX_BT_NUM				= 0;
			HX_X_RES				= 0;
			HX_Y_RES				= 0;
			HX_MAX_PT				= 0;
			HX_XY_REVERSE			= false;
		}
}

int  himax_power_on_initCMD(void)
{
	int retval = 0;
	TS_LOG_INFO("%s:enter\n", __func__);

	retval = i2c_himax_write_command(HX_CMD_TSSON, DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return retval;
	}
	msleep(HX_SLEEP_30MS);

	retval = i2c_himax_write_command(HX_CMD_TSSLPOUT, DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return retval;
	}
	/*don't modify this sleep time!it's for sram test!*/
	msleep(HX_SLEEP_50MS);

	TS_LOG_INFO("%s:exit\n", __func__);

	return retval;
}

static void himax_get_information(void)
{
	int retval = 0;
	TS_LOG_INFO("%s:enter\n", __func__);
	/*Sense on to update the information*/
	retval = i2c_himax_write_command(HX_CMD_TSSON, DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return;
	}
	msleep(HX_SLEEP_30MS);

	retval = i2c_himax_write_command(HX_CMD_TSSLPOUT, DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return;
	}
	msleep(HX_SLEEP_50MS);

	retval = i2c_himax_write_command(HX_CMD_TSSOFF, DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return;
	}
	msleep(HX_SLEEP_50MS);

	retval = i2c_himax_write_command(HX_CMD_TSSLPIN, DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return;
	}
	msleep(HX_SLEEP_50MS);

	himax_touch_information();

	retval = i2c_himax_write_command(HX_CMD_TSSON, DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return;
	}
	msleep(HX_SLEEP_30MS);

	retval = i2c_himax_write_command(HX_CMD_TSSLPOUT, DEFAULT_RETRY_CNT);
	if(retval < 0) {
		return;
	}
	msleep(HX_SLEEP_50MS);
	TS_LOG_INFO("%s:exit\n", __func__);
}

/*int_off:  false: before reset, need disable irq; true: before reset, don't need disable irq.*/
/*loadconfig:  after reset, load config or not.*/
void himax_HW_reset(bool loadconfig,bool int_off)
{
	struct himax_ts_data *ts = g_himax_ts_data;

	HW_RESET_ACTIVATE = 1;

	if(HX_UPDATE_FLAG == 1)
	{
		HX_RESET_COUNT++;
		TS_LOG_INFO("HX still in updateing no reset ");
		return ;
	}

		if(!int_off)
		{
			himax_int_enable(ts->tskit_himax_data->ts_platform_data->irq_id,0);
			msleep(HX_SLEEP_30MS);

			if(atomic_read(&g_himax_ts_data->irq_complete) == 0){
			msleep(HX_SLEEP_10MS);
			}
		}

		TS_LOG_INFO("%s: Now reset the Touch chip.\n", __func__);

		himax_rst_gpio_set(ts->rst_gpio, 0);
		msleep(RESET_LOW_TIME);
		himax_rst_gpio_set(ts->rst_gpio, 1);
		msleep(RESET_HIGH_TIME);

		TS_LOG_INFO("%s: reset ok.\n", __func__);

		if(loadconfig)
			himax_loadSensorConfig();

		if(!int_off)
		{
			himax_int_enable(ts->tskit_himax_data->ts_platform_data->irq_id,1);
		}
}

static bool himax_ic_package_check(void)
{
	uint8_t cmd[HX_IC_PACKAGE_CHECK_MAX_SIZE] = {0};
	memset(cmd, 0x00, sizeof(cmd));

	if (i2c_himax_read(HX_REG_IC_VER, &cmd[0], HX_IC_PACKAGE_CHECK_MAX_SIZE, sizeof(cmd), DEFAULT_RETRY_CNT) < 0)
		return IC_PACK_CHECK_FAIL;

	if(cmd[0] == 0x06 && cmd[1] == 0x85 &&
		(cmd[2] == 0x28 || cmd[2] == 0x29 || cmd[2] == 0x30 ))
	{
		IC_TYPE				= HX_85XX_F_SERIES_PWON;
		IC_CHECKSUM 		= HX_TP_BIN_CHECKSUM_CRC;
		//Himax: Set FW and CFG Flash Address
		FW_VER_MAJ_FLASH_ADDR	= HX8529_FW_VER_MAJ_FLASH_ADDR;
		FW_VER_MIN_FLASH_ADDR	= HX8529_FW_VER_MIN_FLASH_ADDR;
		CFG_VER_MAJ_FLASH_ADDR	= HX8529_CFG_VER_MAJ_FLASH_ADDR;
		CFG_VER_MIN_FLASH_ADDR 	= HX8529_CFG_VER_MIN_FLASH_ADDR;
		FW_CFG_VER_FLASH_ADDR	= HX8529_FW_CFG_VER_FLASH_ADDR;
		TS_LOG_INFO("Himax IC package 852x F\n");
	}
	else {
		TS_LOG_ERR("Himax IC package incorrect!!\n");
		return IC_PACK_CHECK_FAIL;
	}
	return IC_PACK_CHECK_SUCC;
}

static void himax_read_TP_info(void)
{
	char data[HX_READ_TP_INFO_MAX_SIZE] = {0};

	/*read fw version*/
	if (i2c_himax_read(HX_VER_FW_MAJ, data, ONEBYTE, sizeof(data), DEFAULT_RETRY_CNT) < 0) {
		TS_LOG_ERR("%s: i2c access fail!\n", __func__);
		return;
	}
	g_himax_ts_data->vendor_fw_ver_H = data[0];

	if (i2c_himax_read( HX_VER_FW_MIN, data,ONEBYTE, sizeof(data), DEFAULT_RETRY_CNT) < 0) {
		TS_LOG_ERR("%s: i2c access fail!\n", __func__);
		return;
	}
	g_himax_ts_data->vendor_fw_ver_L = data[0];
	/*read config version*/
	if (i2c_himax_read( HX_VER_FW_CFG, data, ONEBYTE, sizeof(data), DEFAULT_RETRY_CNT) < 0) {
		TS_LOG_ERR("%s: i2c access fail!\n", __func__);
		return;
	}
	g_himax_ts_data->vendor_config_ver = data[0];

	/*read sensor ID*/
	g_himax_ts_data->vendor_sensor_id = himax_read_Sensor_ID();

	TS_LOG_INFO("sensor_id=%x.\n",g_himax_ts_data->vendor_sensor_id);
	TS_LOG_INFO("fw_ver=%x,%x.\n",g_himax_ts_data->vendor_fw_ver_H,g_himax_ts_data->vendor_fw_ver_L);
	TS_LOG_INFO("config_ver=%x.\n",g_himax_ts_data->vendor_config_ver);
}

#ifdef HX_ESD_WORKAROUND
void ESD_HW_REST(void)
{
	if (self_test_inter_flag == 1 )
	{
		TS_LOG_INFO("In self test ,not  TP: ESD - Reset\n");
		return;
	}

	if(HX_UPDATE_FLAG==1)
	{
		HX_ESD_RESET_COUNT++;
		TS_LOG_INFO("HX still in updateing , no ESD reset");
		return;
	}
	ESD_RESET_ACTIVATE = 1;

	ESD_R36_FAIL = 0;
#ifdef HX_CHIP_STATUS_MONITOR
	HX_CHIP_POLLING_COUNT=0;
#endif
	TS_LOG_INFO("START_Himax TP: ESD - Reset\n");

	while(ESD_R36_FAIL <=3 )
	{
		himax_rst_gpio_set(g_himax_ts_data->rst_gpio, 0);
		msleep(RESET_LOW_TIME);
		himax_rst_gpio_set(g_himax_ts_data->rst_gpio, 1);
		msleep(RESET_HIGH_TIME);

		if(himax_loadSensorConfig()<0)
			ESD_R36_FAIL++;
		else
			break;

	}
	TS_LOG_INFO("END_Himax TP: ESD - Reset\n");
}
#endif

#ifdef HX_CHIP_STATUS_MONITOR
static void himax_chip_monitor_function(struct work_struct *work) //for ESD solution
{
	int retval = 0;
	if(NULL == work) {
		return;
	}
	TS_LOG_INFO(" %s: POLLING_COUNT=%x, STATUS=%x \n", __func__,HX_CHIP_POLLING_COUNT,retval);
	if(HX_CHIP_POLLING_COUNT >= (HX_POLLING_TIMES-1))//POLLING TIME
	{
		HX_ON_HAND_SHAKING=1;
		retval = himax_hand_shaking(); //0:Running, 1:Stop, 2:I2C Fail
		HX_ON_HAND_SHAKING=0;
		if(retval == HX_HAND_SHAKING_I2C_FAIL)
		{
			TS_LOG_INFO(" %s: I2C Fail \n", __func__);
			ESD_HW_REST();
		}
		else if(retval == HX_HAND_SHAKING_STOP)
		{
			TS_LOG_INFO(" %s: MCU Stop \n", __func__);
			ESD_HW_REST();
		}
		HX_CHIP_POLLING_COUNT=0;//clear polling counter
	}
	else
		HX_CHIP_POLLING_COUNT++;

	queue_delayed_work(g_himax_ts_data->himax_chip_monitor_wq, &g_himax_ts_data->himax_chip_monitor, HX_POLLING_TIMER*HZ);

	return;
}
#endif
void himax_get_rawdata_from_event(int RawDataLen,int hx_touch_info_size, int mul_num, int sel_num, int index, uint8_t *buf )
{
	int temp1 = 0;
	int temp2 = 0;
	int i = 0;
	int m=0;
	int n=0;
	if(NULL == buf) {
		return;
	}
	temp2 = mul_num + sel_num;
	for (i = 0; i < RawDataLen; i++)
	{
		temp1 = index + i;
		if (temp1 < mul_num)
		{ //mutual
	    	m=index + i;
			n=i + hx_touch_info_size+4;
			mutual_data[m] =buf[n];
			//(mutual_data[index + i] )=(buf[i + hx_touch_info_size+4]);	//4: RawData Header
		}
		else
		{//self
			if (temp1 >= temp2)
			{
				break;
			}
			 m=i+index-mul_num;
			 n=i + hx_touch_info_size+4;
			 self_data[m] = buf[n];
			//self_data[i+index-mul_num] = buf[i + hx_touch_info_size+4];	//4: RawData Header
		}
	}
}
int himax_start_get_rawdata_from_event(int hx_touch_info_size,int RawDataLen,uint8_t *buf)
{
	int check_sum_cal = 0;
	int mul_num = 0;
	int self_num = 0;
	int i = 0;
	int retval = NO_ERR;
	int index = 0;
	int m=0,m1=0,m2=0;
	if(NULL == buf) {
		return HX_ERROR;
	}
	for (i = hx_touch_info_size, check_sum_cal = 0; i < HX_RECEIVE_BUF_MAX_SIZE; i++)
	{
		check_sum_cal += buf[i];
	}
	if (check_sum_cal % 0x100 != 0)//designed to be devided by 0X100.
	{
		TS_LOG_ERR("fail,  check_sum_cal: %d\n", check_sum_cal);
#ifdef CONFIG_HUAWEI_DSM
			hmx_tp_dsm_info.WORK_status = TP_WOKR_CHECKSUM_ALL_ERROR;
			hmx_tp_report_dsm_err(DSM_TP_RAWDATA_ERROR_NO, 0);
#endif
			retval = HX_ERROR;
			return retval;//goto bypass_checksum_failed_packet;
		}
		mutual_data = getMutualBuffer();
		self_data 	= getSelfBuffer();
		mul_num = getXChannel() * getYChannel();
		self_num = getXChannel() + getYChannel();
		m=hx_touch_info_size+1;
		m1=hx_touch_info_size+2;
		m2=hx_touch_info_size+3;
		if ((buf[hx_touch_info_size] == buf[m] )&& (buf[m] == buf[m1])
		&& (buf[m1] == buf[m2]) && (buf[hx_touch_info_size] > 0))
		{
			index = (buf[hx_touch_info_size] - 1) * RawDataLen;
			TS_LOG_DEBUG("Header[%d]: %x, %x, %x, %x, mutual: %d, self: %d\n", index, buf[56], buf[57], buf[58], buf[59], mul_num, self_num);
			himax_get_rawdata_from_event(RawDataLen,hx_touch_info_size, mul_num, self_num, index, buf );
		}
		else
		{
			TS_LOG_INFO("[HIMAX TP MSG]%s: header format is wrong!\n", __func__);
#ifdef CONFIG_HUAWEI_DSM
		hmx_tp_dsm_info.WORK_status = TP_WOKR_HEAD_ERROR;
		hmx_tp_report_dsm_err(DSM_TP_RAWDATA_ERROR_NO, 0);
#endif
		retval = HX_ERROR;
		return retval;//goto bypass_checksum_failed_packet;
		}
		return retval;
}
#ifdef HX_CHIP_STATUS_MONITOR
int  himax_chip_monitor_hand_shaking()
{
	int j=0;
	HX_CHIP_POLLING_COUNT=0;
	if(HX_ON_HAND_SHAKING)//chip on hand shaking,wait hand shaking
	{
		for(j = 0; j < HX_HAND_SHAKING_MAX_TIME; j++)
		{
			if(HX_ON_HAND_SHAKING == 0)//chip on hand shaking end
			{
				TS_LOG_INFO("%s:HX_ON_HAND_SHAKING OK check %d times\n",__func__,j);
				break;
			}
			else
				msleep(HX_SLEEP_1MS);
		}
		if(j == HX_HAND_SHAKING_MAX_TIME)
		{
			TS_LOG_ERR("%s:HX_ON_HAND_SHAKING timeout reject interrupt\n",__func__);
			return HANDSHAKE_TIMEOUT;
		}
	}
	return NO_ERR;
}
#endif
#ifdef HX_ESD_WORKAROUND
int himax_check_report_data_for_esd(int hx_touch_info_size,uint8_t *buf)
{
	int i = 0;
	int retval = 0;
	if(NULL == buf) {
		TS_LOG_ERR("himax_check_report_data_for_esd buf pointer NULL!\n");
		return HX_ERROR;
	}
	for(i = 0; i < hx_touch_info_size; i++)
	{
		if (buf[i] == ESD_EVENT_ALL_ZERO) //case 2 ESD recovery flow-Disable
		{
			retval = ESD_ALL_AERO_BAK_VALUE;//if hand shanking fail,firmware error
		}
		else if(buf[i] == ESD_EVENT_ALL_ED)/*case 1 ESD recovery flow*/
		{
			retval = ESD_ALL_ED_BAK_VALUE;//ESD event,ESD reset
		}
		else
		{
			retval = 0;
			break;
		}
	}
	return retval;
}
#endif

void himax_debug_level_print(int type,int status,int hx_touch_info_size,struct himax_touching_data hx_touching,uint8_t *buf)
{
	uint32_t m = (uint32_t)hx_touch_info_size;
	if(NULL == buf) {
		return;
	}
	switch(type)
	{
		case 0:
			for (hx_touching.loop_i = 0; hx_touching.loop_i < m ; hx_touching.loop_i++) {
				printk("0x%2.2X ", buf[hx_touching.loop_i]);
				if (hx_touching.loop_i % 8 == 7)
					printk("\n");
			}
			break;
		case 1:
			TS_LOG_INFO("Finger %d=> X:%d, Y:%d W:%d, Z:%d, F:%d, N:%d\n",
			hx_touching.loop_i + 1, hx_touching.x, hx_touching.y, hx_touching.w, hx_touching.w, hx_touching.loop_i + 1, EN_NoiseFilter);
			break;
		case 2:
			break;
		case 3:
			if(status == 0) //reporting down
			{
				if ((((hx_touching.old_finger >> hx_touching.loop_i) ^ (hx_touching.finger_pressed >> hx_touching.loop_i)) & 1) == 1)
				{
					if (g_himax_ts_data->useScreenRes)
					{
							TS_LOG_INFO("status:%X, Screen:F:%02d Down, X:%d, Y:%d, W:%d, N:%d\n",
							hx_touching.finger_pressed, hx_touching.loop_i+1, (hx_touching.x * g_himax_ts_data->widthFactor) >> SHIFTBITS,
							(hx_touching.y * g_himax_ts_data->heightFactor )>> SHIFTBITS, hx_touching.w, EN_NoiseFilter);
					}
					else
					{
							TS_LOG_INFO("status:%X, Raw:F:%02d Down, X:%d, Y:%d, W:%d, N:%d\n",
							hx_touching.finger_pressed, hx_touching.loop_i+1, hx_touching.x, hx_touching.y, hx_touching.w, EN_NoiseFilter);
					}
				}
			}
			else if(status == 1)  //reporting up
			{
				if ((((hx_touching.old_finger >> hx_touching.loop_i) ^ (hx_touching.finger_pressed >> hx_touching.loop_i)) & 1) == 1)
				{
					if (g_himax_ts_data->useScreenRes)
					{
						TS_LOG_INFO("status:%X, Screen:F:%02d Up, X:%d, Y:%d, N:%d\n",
						hx_touching.finger_pressed,hx_touching.loop_i+1, (g_himax_ts_data->pre_finger_data[hx_touching.loop_i][0] * g_himax_ts_data->widthFactor )>> SHIFTBITS,
						(g_himax_ts_data->pre_finger_data[hx_touching.loop_i][1] * g_himax_ts_data->heightFactor) >> SHIFTBITS, Last_EN_NoiseFilter);
					}
					else{
						TS_LOG_INFO("status:%X, Raw:F:%02d Up, X:%d, Y:%d, N:%d\n",
						hx_touching.finger_pressed, hx_touching.loop_i + 1, g_himax_ts_data->pre_finger_data[hx_touching.loop_i][0],
						g_himax_ts_data->pre_finger_data[hx_touching.loop_i][1], Last_EN_NoiseFilter);
					}
				}
			}
			else if(status == 2) //all leave event
			{
				for (hx_touching.loop_i = 0; hx_touching.loop_i < g_himax_ts_data->nFinger_support && (g_himax_ts_data->debug_log_level & BIT(3)) > 0; hx_touching.loop_i++) {
					if (((g_himax_ts_data->pre_finger_mask >>hx_touching.loop_i) & 1) == 1)
					{
						if (g_himax_ts_data->useScreenRes) {
								TS_LOG_INFO("status:%X, Screen:F:%02d Up, X:%d, Y:%d, N:%d\n", 0,hx_touching.loop_i + 1, (g_himax_ts_data->pre_finger_data[hx_touching.loop_i][0] * g_himax_ts_data->widthFactor )>> SHIFTBITS,
								(g_himax_ts_data->pre_finger_data[hx_touching.loop_i][1] * g_himax_ts_data->heightFactor) >> SHIFTBITS, Last_EN_NoiseFilter);
						} else {
							TS_LOG_INFO("status:%X, Raw:F:%02d Up, X:%d, Y:%d, N:%d\n",0, hx_touching.loop_i + 1, g_himax_ts_data->pre_finger_data[hx_touching.loop_i][0],g_himax_ts_data->pre_finger_data[hx_touching.loop_i][1], Last_EN_NoiseFilter);
						}
					}
				}
				g_himax_ts_data->pre_finger_mask = 0;
			}
			break;
		default:
			break;
	}
}

int himax_checksum_cal(int hx_touch_info_size,struct himax_touching_data hx_touching,uint8_t *buf)
{
	int checksum = 0;
	uint32_t m=(uint32_t)hx_touch_info_size;
	if(NULL == buf) {
		return  checksum;
	}
	for (hx_touching.loop_i = 0; hx_touching.loop_i < m; hx_touching.loop_i++)
		checksum += buf[hx_touching.loop_i];

	return checksum;
}

void himax_parse_coords(int hx_touch_info_size,int hx_point_num,struct ts_fingers *info,struct himax_touching_data hx_touching,uint8_t *buf)
{
	uint8_t coordInfoSize = g_himax_ts_data->coord_data_size + g_himax_ts_data->area_data_size + 4;
	int base = 0;
	int m=0,m1=0,m2=0;
	if(NULL == buf||NULL == info) {
		return ;
	}
	if (hx_point_num != 0 ) {
		hx_touching.old_finger = g_himax_ts_data->pre_finger_mask;
		hx_touching.finger_pressed = buf[coordInfoSize - 2] << 8 | buf[coordInfoSize - 3];

		for (hx_touching.loop_i = 0; hx_touching.loop_i < g_himax_ts_data->nFinger_support; hx_touching.loop_i++)
		{
			if (((hx_touching.finger_pressed >> hx_touching.loop_i) & 1) == 1)
			{
				base = hx_touching.loop_i * 4;//every finger coordinate need 4 bytes.
				m=base + 1;
				m1=base + 2;
				m2=base + 3;
				hx_touching.x = ((buf[base]) << 8) |(buf[m]);
				hx_touching.y = ((buf[m1]) << 8 )| (buf[m2]);
				hx_touching.w = 10;
				if ((g_himax_ts_data->debug_log_level & BIT(3)) > 0)//debug 3: print finger coordinate information
				{
					himax_debug_level_print(3,0,hx_touch_info_size,hx_touching,buf); //status = report down
				}

				info->fingers[hx_touching.loop_i].status = TS_FINGER_PRESS;
				info->fingers[hx_touching.loop_i].x = hx_touching.x;
				info->fingers[hx_touching.loop_i].y = hx_touching.y;
				info->fingers[hx_touching.loop_i].major = 255;
				info->fingers[hx_touching.loop_i].minor = 255;
				info->fingers[hx_touching.loop_i].pressure = hx_touching.w;
				if (!g_himax_ts_data->first_pressed)
				{
					g_himax_ts_data->first_pressed = 1;//first report
					TS_LOG_INFO("S1@%d, %d\n", hx_touching.x, hx_touching.y);
				}

				g_himax_ts_data->pre_finger_data[hx_touching.loop_i][0] = hx_touching.x;
				g_himax_ts_data->pre_finger_data[hx_touching.loop_i][1] = hx_touching.y;

				if (g_himax_ts_data->debug_log_level & BIT(1))
					himax_debug_level_print(1,0,hx_touch_info_size,hx_touching, buf);  //status useless
			}
			else
			{
				if (hx_touching.loop_i == 0 && g_himax_ts_data->first_pressed == 1)
				{
					g_himax_ts_data->first_pressed = 2;
					TS_LOG_INFO("E1@%d, %d\n",
					g_himax_ts_data->pre_finger_data[0][0] , g_himax_ts_data->pre_finger_data[0][1]);
				}
				if ((g_himax_ts_data->debug_log_level & BIT(3)) > 0)
				{
					himax_debug_level_print(3,1,hx_touch_info_size,hx_touching,buf); //status= report up
				}
			}
		}
		g_himax_ts_data->pre_finger_mask = hx_touching.finger_pressed;

	}
	else
	{
		// leave event
		for (hx_touching.loop_i = 0; hx_touching.loop_i < g_himax_ts_data->nFinger_support; hx_touching.loop_i++) {
				if (((g_himax_ts_data->pre_finger_mask >> hx_touching.loop_i) & 1) == 1) {

					info->fingers[hx_touching.loop_i].status = TS_FINGER_RELEASE;
					info->fingers[hx_touching.loop_i].x = 0;
					info->fingers[hx_touching.loop_i].y = 0;
					info->fingers[hx_touching.loop_i].major = 0;
					info->fingers[hx_touching.loop_i].minor = 0;
					info->fingers[hx_touching.loop_i].pressure = 0;
				}
			}
		if (g_himax_ts_data->pre_finger_mask > 0) {
			himax_debug_level_print(3, 3, hx_touch_info_size,hx_touching,buf);  //all leave event
		}

		if (g_himax_ts_data->first_pressed == 1) {
			g_himax_ts_data->first_pressed = 2;
			TS_LOG_INFO("E1@%d, %d\n",g_himax_ts_data->pre_finger_data[0][0] , g_himax_ts_data->pre_finger_data[0][1]);
		}

		if (g_himax_ts_data->debug_log_level & BIT(1))
			TS_LOG_INFO("All Finger leave\n");
	}
}

static int himax_irq_bottom_half(struct ts_cmd_node *in_cmd,struct ts_cmd_node *out_cmd)
{
	int retval = 0;
	int RawDataLen = 0;
	int raw_cnt_max = 0;
	int raw_cnt_rmd = 0;
	int hx_touch_info_size = 0;

	uint8_t buf[HX_RECEIVE_BUF_MAX_SIZE] = {0};
	unsigned char check_sum_cal = 0;

	struct algo_param *algo_p = NULL;
	struct ts_fingers *info = NULL;

	static int iCount = 0;
#ifdef HX_TP_SYS_DIAG
	uint8_t diag_cmd = 0;
#endif

	struct himax_touching_data hx_touching;
#ifdef HX_CHIP_STATUS_MONITOR
	int j=0;
#endif
	int m=0;
	if(NULL == in_cmd||NULL == out_cmd) {
		return HX_ERROR;
	}
	algo_p = &out_cmd->cmd_param.pub_params.algo_param;
	info = &algo_p->info;

	out_cmd->command = TS_INPUT_ALGO;

	memset(buf, 0x00, sizeof(buf));
	atomic_set(&g_himax_ts_data->irq_complete, IRQ_DISABLE);
#ifdef HX_CHIP_STATUS_MONITOR
		if(himax_chip_monitor_hand_shaking()!=NO_ERR)
			return HANDSHAKE_TIMEOUT;
#endif
	hx_touching.x = 0;
	hx_touching.y = 0;
	hx_touching.w = 0;
	hx_touching.finger_pressed = 0;
	hx_touching.old_finger = 0;
	hx_touching.loop_i = 0;

	raw_cnt_max = HX_MAX_PT/4;//max point / 4
	raw_cnt_rmd = HX_MAX_PT%4;

	if (raw_cnt_rmd != 0x00) //more than 4 fingers
	{
		RawDataLen = HX_RECEIVE_BUF_MAX_SIZE - ((HX_MAX_PT+raw_cnt_max+3)*4) - 1;
		hx_touch_info_size = (HX_MAX_PT+raw_cnt_max+2)*4;
	}
	else //less than 4 fingers
	{
		RawDataLen = HX_RECEIVE_BUF_MAX_SIZE - ((HX_MAX_PT+raw_cnt_max+2)*4) - 1;
		hx_touch_info_size = (HX_MAX_PT+raw_cnt_max+1)*4;
	}

	if(hx_touch_info_size > HX_RECEIVE_BUF_MAX_SIZE)
	{
		TS_LOG_ERR("%s:hx_touch_info_size larger than HX_RECEIVE_BUF_MAX_SIZE\n",__func__);
		goto err_no_reset_out;
	}
#ifdef HX_TP_SYS_DIAG
	diag_cmd = getDiagCommand();
#ifdef HX_ESD_WORKAROUND
	if((diag_cmd) || (ESD_RESET_ACTIVATE) || (HW_RESET_ACTIVATE))
#else
	if((diag_cmd) || (HW_RESET_ACTIVATE))
#endif
	{
		retval = i2c_himax_read( HX_REG_EVENT_STACK, buf, HX_RECEIVE_BUF_MAX_SIZE, sizeof(buf), DEFAULT_RETRY_CNT);//diad cmd not 0, need to read 128.
	}
	else{
		if(touch_monitor_stop_flag != 0){
			retval = i2c_himax_read( HX_REG_EVENT_STACK, buf, HX_RECEIVE_BUF_MAX_SIZE, sizeof(buf), DEFAULT_RETRY_CNT);
			touch_monitor_stop_flag-- ;
		}
		else{
			retval = i2c_himax_read( HX_REG_EVENT_STACK, buf, hx_touch_info_size, sizeof(buf), DEFAULT_RETRY_CNT);
		}
	}
	if (retval < 0)
#else
	if (i2c_himax_read(HX_REG_EVENT_STACK, buf, hx_touch_info_size, sizeof(buf), DEFAULT_RETRY_CNT)<0)
#endif
	{
		TS_LOG_ERR("%s: can't read data from chip!\n", __func__);
#ifdef CONFIG_HUAWEI_DSM
			hmx_tp_dsm_info.WORK_status = TP_WOKR_READ_DATA_ERROR;
			hmx_tp_report_dsm_err(DSM_TP_DEV_STATUS_ERROR_NO, 0);
#endif
		iCount++;
		TS_LOG_ERR("%s: error count is %d !\n", __func__, iCount);
		if(iCount >= RETRY_TIMES)
		{
			iCount = 0;
			goto err_workqueue_out;
		}
		goto err_no_reset_out;
	}
	else
	{
#ifdef HX_ESD_WORKAROUND
	check_sum_cal = himax_check_report_data_for_esd(hx_touch_info_size,buf);
#ifdef HX_TP_SYS_DIAG
	diag_cmd = getDiagCommand();
	if (check_sum_cal != 0 && ESD_RESET_ACTIVATE == 0 && HW_RESET_ACTIVATE == 0
		&& diag_cmd == 0 && self_test_inter_flag == 0)  //ESD Check
#else
	if (check_sum_cal != 0 && ESD_RESET_ACTIVATE == 0 && HW_RESET_ACTIVATE == 0
		&& self_test_inter_flag == 0)  //ESD Check
#endif
	{
		retval = himax_hand_shaking(); //check mcu status  ---  0:Running, 1:Stop, 2:I2C Fail
		if (retval == HX_HAND_SHAKING_I2C_FAIL) {
			TS_LOG_ERR("HX_HAND_SHAKING_I2C_FAIL %d!\n",__LINE__);
			goto err_workqueue_out;
		}
		if ((retval == HX_HAND_SHAKING_STOP) && (check_sum_cal == ESD_ALL_AERO_BAK_VALUE))
		{
			TS_LOG_INFO("[HIMAX TP MSG]: ESD event checked - ALL Zero.\n");
			ESD_HW_REST();
			goto err_no_reset_out;
		}
		else if (check_sum_cal == ESD_ALL_ED_BAK_VALUE)
		{
			TS_LOG_INFO("[HIMAX TP MSG]: ESD event checked - ALL 0xED.\n");
			ESD_HW_REST();
			goto err_no_reset_out;
		}
	}
	else if (ESD_RESET_ACTIVATE)
	{
		ESD_RESET_ACTIVATE = 0;/*drop 1st interrupts after chip reset*/
		TS_LOG_INFO("[HIMAX TP MSG]:%s: Back from reset, ready to serve.\n", __func__);
		atomic_set(&g_himax_ts_data->irq_complete, IRQ_ENABLE);
		return retval;
	}
	else if (HW_RESET_ACTIVATE)
#else
	if (HW_RESET_ACTIVATE)
#endif
	{
		HW_RESET_ACTIVATE = 0;/*drop 1st interrupts after chip reset*/
		TS_LOG_INFO("[HIMAX TP MSG]:%s: HW_RST Back from reset, ready to serve.\n", __func__);
		atomic_set(&g_himax_ts_data->irq_complete, IRQ_ENABLE);
		return retval;
	}

	check_sum_cal = himax_checksum_cal(hx_touch_info_size,hx_touching,buf);

	if (check_sum_cal != 0x00 )  //self_test_inter_flag == 1
	{
		TS_LOG_INFO("[HIMAX TP MSG] checksum fail : check_sum_cal: 0x%02X\n", check_sum_cal);
#ifdef CONFIG_HUAWEI_DSM
		hmx_tp_dsm_info.WORK_status = TP_WOKR_CHECKSUM_INFO_ERROR;
		hmx_tp_report_dsm_err(DSM_TP_DEV_STATUS_ERROR_NO, 0);
#endif
		iCount++;
		TS_LOG_ERR("%s: error count is %d !\n", __func__, iCount);
		if(iCount >= RETRY_TIMES)
		{
			iCount = 0;
			goto err_workqueue_out;
		}
		goto err_no_reset_out;
	}
	}

	if (g_himax_ts_data->debug_log_level & BIT(0)) {
		TS_LOG_INFO("%s: raw data:\n", __func__);
		himax_debug_level_print(0,0,hx_touch_info_size,hx_touching,buf);  //status uselss
	}
	/*touch monitor raw data fetch*/
#ifdef HX_TP_SYS_DIAG
	diag_cmd = getDiagCommand();
	if (diag_cmd >= 1 && diag_cmd <= 6)
	{
		if(himax_start_get_rawdata_from_event(hx_touch_info_size,RawDataLen,buf) == HX_ERROR)
			goto bypass_checksum_failed_packet;
	}
	else if (diag_cmd == 7)
	{
		memcpy(&(diag_coor[0]), &buf[0], HX_RECEIVE_BUF_MAX_SIZE);
	}

#endif
bypass_checksum_failed_packet:
		m = HX_TOUCH_INFO_POINT_CNT + 2;
		EN_NoiseFilter = ((buf[m]) >> 3);//HX_TOUCH_INFO_POINT_CNT: 52 ;
		EN_NoiseFilter = EN_NoiseFilter & 0x01;

		if (buf[HX_TOUCH_INFO_POINT_CNT] == 0xff)
			hx_real_point_num = 0;
		else
			hx_real_point_num = buf[HX_TOUCH_INFO_POINT_CNT] & 0x0f;//only use low 4 bits.

		/*Touch Point information*/
		himax_parse_coords(hx_touch_info_size,hx_real_point_num,info,hx_touching,buf);

		Last_EN_NoiseFilter = EN_NoiseFilter;

	iCount = 0;//I2C communication ok, checksum ok;
	atomic_set(&g_himax_ts_data->irq_complete, 1);
	return retval;

err_workqueue_out:
	TS_LOG_ERR("%s: Now reset the Touch chip.\n", __func__);

	himax_HW_reset(HX_LOADCONFIG_EN,HX_INT_DISABLE);

err_no_reset_out:
	atomic_set(&g_himax_ts_data->irq_complete, IRQ_ENABLE);
	return NO_RESET_OUT;
}
static int himax_parse_specific_dts(struct himax_ts_data *ts,
				struct himax_i2c_platform_data *pdata)
{
	int retval = 0;
	int coords_size = 0;
	uint32_t coords[HX_COORDS_MAX_SIZE] = {0};
	struct property *prop = NULL;
	struct device_node *dt = ts->ts_dev->dev.of_node;
	if(NULL == ts || NULL == pdata) {
		return -1;
	}
	prop = of_find_property(dt, "himax,panel-coords", NULL);
	if (prop) {
		coords_size = prop->length /((int) sizeof(uint32_t));
		if (coords_size != HX_COORDS_MAX_SIZE)
			TS_LOG_DEBUG("%s:Invalid panel coords size %d", __func__, coords_size);
	}

	if (of_property_read_u32_array(dt, "himax,panel-coords", coords, coords_size) == NO_ERR) {
		pdata->abs_x_min = coords[0], pdata->abs_x_max = coords[1];
		pdata->abs_y_min = coords[2], pdata->abs_y_max = coords[3];
		TS_LOG_INFO("DT-%s:panel-coords = %d, %d, %d, %d\n", __func__, pdata->abs_x_min,
				pdata->abs_x_max, pdata->abs_y_min, pdata->abs_y_max);
	}
	else
	{
		pdata->abs_x_max = ABS_X_MAX_DEFAULT;
		pdata->abs_y_max = ABS_Y_MAX_DEFAULT;
	}

	prop = of_find_property(dt, "himax,display-coords", NULL);
	if (prop) {
		coords_size = prop->length /((int) sizeof(uint32_t));
		if (coords_size != HX_COORDS_MAX_SIZE)
			TS_LOG_DEBUG("%s:Invalid display coords size %d", __func__, coords_size);
	}

	retval = of_property_read_u32_array(dt, "himax,display-coords", coords, coords_size);
	if (retval) {
		TS_LOG_DEBUG("%s:Fail to read display-coords %d\n", __func__, retval);
		return retval;
	}
	pdata->screenWidth  = coords[1];
	pdata->screenHeight = coords[3];

	TS_LOG_INFO("DT-%s:display-coords = (%d, %d)", __func__, pdata->screenWidth,
		pdata->screenHeight);

	pdata->gpio_3v3_en = of_get_named_gpio(dt, "himax,vdd_ana-supply", 0);
	if (!gpio_is_valid(pdata->gpio_3v3_en)) {
		TS_LOG_INFO("DT:gpio_3v3_en value is not valid\n");
	}

	pdata->gpio_1v8_en = of_get_named_gpio(dt, "himax,vcc_i2c-supply", 0);
	if (!gpio_is_valid(pdata->gpio_1v8_en)) {
		TS_LOG_INFO("DT:pdata->gpio_1v8_en is not valid\n");
	}

	TS_LOG_INFO("DT:gpio_3v3_en=%d,gpio_1v8_en=%d\n",pdata->gpio_3v3_en,pdata->gpio_1v8_en);

	return NO_ERR;
	}

static int himax_parse_dts(struct device_node *device, struct ts_kit_device_data *chip_data)
{
	int retval = NO_ERR;
	const char *modulename = NULL;
	const char *projectid = NULL;
	const char *tptesttype = NULL;
	const char *chipname = NULL;
	int read_val = 0;
	TS_LOG_INFO("%s: parameter init begin\n", __func__);
	if(NULL == device||NULL == chip_data) {
		return -1;
	}
	retval = of_property_read_u32(device, "reg", &chip_data->ts_platform_data->client->addr);
	if (retval) {
		chip_data->ts_platform_data->client->addr = SLAVE_I2C_ADRR;
		TS_LOG_INFO("Not define reg in Dts, use default\n");
	}
	TS_LOG_INFO("get himax reg = 0x%02x\n", chip_data->ts_platform_data->client->addr);

	retval =of_property_read_u32(device, "ic_type",&chip_data->ic_type);
	if (retval) {
		chip_data->ic_type = ONCELL;
		TS_LOG_ERR("Not define device ic_type in Dts\n");
	} else {
		g_tskit_ic_type = chip_data->ic_type;
	}
	TS_LOG_INFO("get g_tskit_ic_type = %d.\n", g_tskit_ic_type);

	retval = of_property_read_u32(device, "himax,rawdata_timeout", &chip_data->rawdata_get_timeout);
	if (retval) {
		chip_data->rawdata_get_timeout = RAWDATA_GET_TIME_DEFAULT;
		TS_LOG_INFO("Not define chip rawdata limit time in Dts, use default\n");
	}
	TS_LOG_INFO("get chip rawdata limit time = %d\n", chip_data->rawdata_get_timeout);

	retval = of_property_read_u32(device, "himax,irq_config", &chip_data->irq_config);
	if (retval) {
		TS_LOG_ERR("Not define irq_config in Dts\n");
		return retval;
	}
	TS_LOG_INFO("get himax irq_config = %d\n", chip_data->irq_config);

	retval =of_property_read_string(device, "chip_name", &chipname);
	if (retval) {
		strncpy(chip_data->chip_name,STR_IC_NAME,strlen(STR_IC_NAME)+1);
		TS_LOG_ERR("Not define chipname in Dts,use default\n");
	}else{
		strncpy(chip_data->chip_name, chipname, CHIP_NAME_LEN);
	}
	TS_LOG_INFO("get himax_chipname = %s\n",chip_data->chip_name);

	retval =of_property_read_string(device, "project_id", &projectid);
	if (retval) {
		strncpy(himax_product_id, PRODUCE_ID, HX_PROJECT_ID_LEN);
		TS_LOG_ERR("Not define product id in Dts, use default\n");
	}
	else{
		strncpy(himax_product_id, projectid, HX_PROJECT_ID_LEN);
	}
	TS_LOG_INFO("get himax_project_id = %s\n",himax_product_id);

	retval =of_property_read_string(device, "tp_test_type", &tptesttype);
	if (retval) {
		TS_LOG_INFO("Not define device tp_test_type in Dts, use default\n");
		strncpy(chip_data->tp_test_type, "Normalize_type:judge_last_result", TS_CAP_TEST_TYPE_LEN);
	}
	else {
		snprintf(chip_data->tp_test_type, PAGE_SIZE, "%s", tptesttype);
	}
	TS_LOG_INFO("get tp test type = %s\n", chip_data->tp_test_type);

	retval = of_property_read_string(device, "module",&modulename);
	if (retval) {
		strncpy(chip_data->module_name, MODULE_NAME,strlen(MODULE_NAME)+1);
		TS_LOG_INFO("Not define module in Dts,use default\n");
	}
	else{
		strncpy(chip_data->module_name, modulename,strlen(MODULE_NAME)+1);
	}
	TS_LOG_INFO("module_name: %s\n", chip_data->module_name);
	/* get tp color flag */
	retval = of_property_read_u32(device,  "support_get_tp_color", &read_val);
	if (retval) {
		TS_LOG_INFO("%s, get device support_get_tp_color failed, will use default value: 0 \n ", __func__);
		read_val = 0; //default 0: no need know tp color
	}
	g_himax_ts_data->support_get_tp_color = (uint8_t)read_val;
	TS_LOG_INFO("%s, support_get_tp_color = %d \n", __func__, g_himax_ts_data->support_get_tp_color);
	TS_LOG_INFO("%s:sucess\n",__func__);

	return NO_ERR;

}

static int himax_chip_detect(struct ts_kit_platform_data *platform_data)
{
	int err = NO_ERR;
#ifdef HX_UPDATE_WITH_BIN_BUILDIN
	//int recovery_flag = 0;
#endif
	struct himax_ts_data *ts = NULL;
	struct himax_i2c_platform_data *pdata = NULL;
	TS_LOG_INFO("%s:called\n", __func__);
	if (!platform_data){
		TS_LOG_ERR("device, ts_kit_platform_data *platform_data or platform_data->ts_dev is NULL \n");
		err =  -EINVAL;
		goto out;
	}

	g_himax_ts_data->ts_dev = platform_data->ts_dev;
	g_himax_ts_data->ts_dev->dev.of_node = g_himax_ts_data->tskit_himax_data->cnode ;
	g_himax_ts_data->tskit_himax_data->ts_platform_data = platform_data;
	g_himax_ts_data->tskit_himax_data->is_in_cell = false;
	g_himax_ts_data->tskit_himax_data->is_i2c_one_byte = 0;
	g_himax_ts_data->tskit_himax_data->is_new_oem_structure= 0;
	g_himax_ts_data->dev = &(platform_data->client->dev);
	g_himax_ts_data->firmware_updating = false;

	ts = g_himax_ts_data;

	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (pdata == NULL) {
		err = -ENOMEM;
		goto err_alloc_platform_data_fail;
	}

	err = himax_parse_dts(ts->ts_dev->dev.of_node,ts->tskit_himax_data);
	if (err) {
		TS_LOG_INFO("himax_parse_dts for DT err\n");
		goto err_parse_pdata_failed;
	}

	if (ts->ts_dev->dev.of_node) { /*DeviceTree Init Platform_data*/
		err = himax_parse_specific_dts(ts, pdata);
		if (err < 0) {
			TS_LOG_INFO(" pdata is NULL for DT\n");
			goto err_parse_pdata_failed;
		}
	}
	pdata->gpio_reset = ts->tskit_himax_data->ts_platform_data->reset_gpio;
	TS_LOG_INFO("pdata->gpio_reset:%d\n",pdata->gpio_reset);
	pdata->gpio_irq = ts->tskit_himax_data->ts_platform_data->irq_gpio;
	TS_LOG_INFO("pdata->gpio_irq:%d\n",pdata->gpio_irq);
	ts->rst_gpio = pdata->gpio_reset;

	err = himax_gpio_power_config(pdata);

	if (err) {
		TS_LOG_ERR("%s: himax_gpio_power_config fail\n", __func__);
		goto err_himax_gpio_power_config;
	}

	err = himax_gpio_power_on(pdata);
	if (err) {
		TS_LOG_ERR("%s: himax_gpio_power_on fail\n", __func__);
		goto err_himax_gpio_power_on;
	}

#ifdef CONFIG_HUAWEI_DSM
	hmx_tp_dclient = dsm_register_client(&dsm_hmx_tp);
	if (!hmx_tp_dclient)
	{
		TS_LOG_ERR("%s: dsm register client failed\n", __func__);
		goto err_dsm_register_failed;
	}
	hmx_tp_dsm_info.irq_gpio = pdata->gpio_irq;
	hmx_tp_dsm_info.rst_gpio = pdata->gpio_reset;
#endif/*CONFIG_HUAWEI_DSM*/
	//Get Himax IC Type / FW information / Calculate the point number/HX8529  F series
	if (himax_ic_package_check() == IC_PACK_CHECK_FAIL) {
		TS_LOG_ERR("Himax chip does NOT EXIST");
		err = -ENOMEM;
		goto err_ic_package_failed;
	}

#ifdef  HX_TP_SYS_FLASH_DUMP
	ts->flash_wq = create_singlethread_workqueue("himax_flash_wq");
	if (!ts->flash_wq)
		{
			TS_LOG_ERR("%s: create flash workqueue failed\n", __func__);
			err = -ENOMEM;
			goto err_create_flash_wq_failed;
		}
		INIT_WORK(&ts->flash_work, himax_ts_flash_work_func);
		setSysOperation(0);
		setFlashBuffer();
#endif
	himax_read_TP_info();

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
	/* detect current device successful, set the flag as present */
	set_hw_dev_flag(DEV_I2C_TOUCH_PANEL);
#endif
	/*Himax Power On and Load Config*/
	if (himax_loadSensorConfig() < 0) {
		TS_LOG_ERR("%s: Load Sesnsor configuration failed, unload driver.\n", __func__);
		err = -ENOMEM;
		goto err_detect_failed;
	}

	himax_get_information();

	calculate_point_number();

	wake_lock_init(&ts->ts_flash_wake_lock, WAKE_LOCK_SUSPEND, HIMAX_VENDER_NAME);

#ifdef HX_TP_SYS_DIAG
	setXChannel(HX_RX_NUM); // X channel
	setYChannel(HX_TX_NUM); // Y channel

	setMutualBuffer();
	if (getMutualBuffer() == NULL) {
		TS_LOG_ERR("%s: mutual buffer allocate fail failed\n", __func__);
		goto err_setchannel_failed;
	}
#endif
	ts->pdata = pdata;

	ts->x_channel = HX_RX_NUM;
	ts->y_channel = HX_TX_NUM;
	ts->nFinger_support = HX_MAX_PT;
	/*calculate the i2c data size*/
	calcDataSize(ts->nFinger_support);
	TS_LOG_INFO("%s: calcDataSize complete\n", __func__);

	ts->pdata->abs_pressure_min	= 0;
	ts->pdata->abs_pressure_max	= 200;
	ts->pdata->abs_width_min	= 0;
	ts->pdata->abs_width_max	= 200;
	pdata->cable_config[0]		= 0x90;
	pdata->cable_config[1]		= 0x00;

	ts->suspended	= false;

#ifdef HX_CHIP_STATUS_MONITOR//for ESD solution
	ts->himax_chip_monitor_wq = create_singlethread_workqueue("himax_chip_monitor_wq");
	if (!ts->himax_chip_monitor_wq)
	{
		TS_LOG_ERR(" %s: create monitor workqueue failed\n", __func__);
		err = -ENOMEM;
		goto err_create_chip_monitor_wq_failed;
	}

	INIT_DELAYED_WORK(&ts->himax_chip_monitor, himax_chip_monitor_function);
	queue_delayed_work(ts->himax_chip_monitor_wq, &ts->himax_chip_monitor, HX_POLLING_TIMER*HZ);
#endif

	atomic_set(&ts->suspend_mode, 0);
	atomic_set(&ts->irq_complete, 1);

#ifdef HX_ESD_WORKAROUND
	ESD_RESET_ACTIVATE = 0;
#endif
	HW_RESET_ACTIVATE = 0;

	//himax_set_app_info(ts);//TO DO
#ifdef HX_ESD_WORKAROUND
	g_check_r36h_flag = R36_CHECK_ENABLE_FLAG;//ensure check R36 register after driver add succ
#endif

	TS_LOG_INFO("%s:sucess\n", __func__);
	return NO_ERR;

#ifdef HX_CHIP_STATUS_MONITOR
	cancel_delayed_work_sync(&ts->himax_chip_monitor);
err_create_chip_monitor_wq_failed:
	destroy_workqueue(ts->himax_chip_monitor_wq);
	freeMutualBuffer();
#endif

#ifdef HX_TP_SYS_DIAG
	//freeMutualBuffer();
err_setchannel_failed:
#endif
wake_lock_destroy(&ts->ts_flash_wake_lock);
err_detect_failed:

#ifdef  HX_TP_SYS_FLASH_DUMP
	freeFlashBuffer();
	destroy_workqueue(ts->flash_wq);
err_create_flash_wq_failed:
#endif

err_ic_package_failed:
#ifdef CONFIG_HUAWEI_DSM
	if (hmx_tp_dclient) {
		dsm_unregister_client(hmx_tp_dclient, &dsm_hmx_tp);
		hmx_tp_dclient = NULL;
	}
err_dsm_register_failed:
#endif/*CONFIG_HUAWEI_DSM*/
	himax_gpio_power_off(pdata);
err_himax_gpio_power_on:
	himax_gpio_power_deconfig(pdata);
err_himax_gpio_power_config:
err_parse_pdata_failed:
	kfree(pdata);
err_alloc_platform_data_fail:
out:
	if (g_himax_ts_data->tskit_himax_data){
		kfree(g_himax_ts_data->tskit_himax_data);
		g_himax_ts_data->tskit_himax_data = NULL;
	}

	if (g_himax_ts_data){
		kfree(g_himax_ts_data);
		g_himax_ts_data = NULL;
	}
	TS_LOG_ERR("detect himax error\n");
	return err;
}
static int __init early_parse_himax_panel_name_cmdline(char *p)
{
	if (p)
	{
		if (NULL != strstr(p, CMDLINE_PANEL_CHOPIN_BLACK_NAME))
		{
			himax_tp_color = BLACK;
		}
		else if (NULL != strstr(p, CMDLINE_PANEL_CHOPIN_WHITE_NAME))
		{
			himax_tp_color = WHITE;
		}
		TS_LOG_INFO("himax_tp_color :%d\n", himax_tp_color);
	}
	return 0;
}
early_param("mdss_mdp.panel", early_parse_himax_panel_name_cmdline);
static int himax_init_chip()
{
	if (g_himax_ts_data->support_get_tp_color == 1) {
		/* Get tp_color */
		cypress_ts_kit_color[0]=himax_tp_color;
	}
	#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)
	himax_touch_sysfs_init();
	#endif

	return NO_ERR;
}

int PowerOnSeq(struct himax_ts_data *ts)
{
	if(NULL == ts) {
		return HX_ERROR;
	}
	if (himax_loadSensorConfig() < 0) {
		TS_LOG_ERR("%s: Load Sesnsor configuration failed, unload driver.\n", __func__);
		return LOAD_SENSORCONFIG_RUN_FAIL;
	}
	himax_get_information();
	calculate_point_number();
#ifdef HX_TP_SYS_DIAG
	setXChannel(HX_RX_NUM); // X channel
	setYChannel(HX_TX_NUM); // Y channel
	setMutualBuffer();
	if (getMutualBuffer() == NULL) {
		TS_LOG_ERR("%s: mutual buffer allocate fail failed\n", __func__);
		return MUTUAL_ALLOC_FAIL;
	}
#endif
	ts->x_channel = HX_RX_NUM;
	ts->y_channel = HX_TX_NUM;
	ts->nFinger_support = HX_MAX_PT;
	calcDataSize(ts->nFinger_support);
	return NO_ERR;
}

static int himax_enter_sleep_mode(void)
{
	int retval = 0;
	uint8_t buf[SLEEP_OFF_BUF_LEN] = {0};
	memset(buf, 0x00, sizeof(buf));
	TS_LOG_INFO("%s: enter \n", __func__);

	//Sense off
	buf[0] = HX_CMD_TSSOFF;
	retval = i2c_himax_master_write( buf, ONEBYTE, sizeof(buf), DEFAULT_RETRY_CNT);
	if (retval < 0){
		TS_LOG_ERR("[himax] %s: HX_CMD_TSSOFF fail!\n", __func__);
		return retval;
	}
	msleep(HX_SLEEP_40MS);

	buf[0] = HX_CMD_TSSLPIN;
	retval = i2c_himax_master_write(buf, ONEBYTE, sizeof(buf), DEFAULT_RETRY_CNT);
	if (retval < 0){
		TS_LOG_ERR("[himax] %s: HX_CMD_TSSLPIN fail!\n", __func__);
		return retval;
	}

	TS_LOG_INFO("%s: exit \n", __func__);
	return NO_ERR;
}
static int himax_exit_sleep_mode(void)
{
	int retval = 0;
	TS_LOG_INFO("%s: enter \n", __func__);

	//Sense On
	retval = i2c_himax_write_command( HX_CMD_TSSON, DEFAULT_RETRY_CNT);
	if(retval <0) {
		TS_LOG_ERR("[himax] %s: HX_CMD_TSSON fail!\n", __func__);
		return retval;
	}
	mdelay(HX_SLEEP_1MS);
	retval = i2c_himax_write_command(HX_CMD_TSSLPOUT, DEFAULT_RETRY_CNT);
	if(retval <0) {
		TS_LOG_ERR("[himax] %s: HX_CMD_TSSLPOUT fail!\n", __func__);
		return retval;
	}
	msleep(HX_SLEEP_30MS);

	TS_LOG_INFO("%s: exit \n", __func__);
	return NO_ERR;
}
static int himax_power_rst_init(void)
{
	struct himax_ts_data *ts = NULL;
	uint8_t data[SLEEP_ON_BUF_LEN] = {0};
	memset(data, 0x00, sizeof(data));
	int retval = 0;

	TS_LOG_INFO("%s: enter \n", __func__);
	ts = g_himax_ts_data;

	//himax ic 852xf(auo module) resume need two reset and initial sensor on,this is first time
	gpio_direction_output(ts->rst_gpio, RST_DISABLE);
	mdelay(HX_SLEEP_5MS);
	gpio_direction_output(ts->rst_gpio, RST_ENABLE);
	msleep(HX_SLEEP_20MS);
	TS_LOG_INFO("%s first: pull reset gpio on. \n", __func__);

	data[0] = HX_REG_SET_CLK_ADDR;
	//adjust flash clock cmd
	data[1] = 0x06;
	data[2] = 0x03;
	retval = i2c_himax_master_write(&data[0],SET_CLK_DATA_LEN, sizeof(data),DEFAULT_RETRY_CNT);
	if(retval <0) {
		TS_LOG_ERR("[himax] %s: HX_REG_SET_CLK_ADDR fail!\n", __func__);
		return retval;
	}
	mdelay(HX_SLEEP_1MS);

	data[0] = HX_REG_SET_OSC_4_PUMP;
	//switch to mcu cmd
	data[1] = 0x11;
	data[2] = 0x00;
	retval = i2c_himax_master_write( &data[0],SET_CRYSTAL_DATA_LEN, sizeof(data),DEFAULT_RETRY_CNT);
	if(retval <0) {
		TS_LOG_ERR("[himax] %s: HX_REG_SET_OSC_4_PUMP fail!\n", __func__);
		return retval;
	}
	mdelay(HX_SLEEP_1MS);
	TS_LOG_INFO("%s first: flash clock ok. \n", __func__);

	//Sense On
	retval = i2c_himax_write_command( HX_CMD_TSSON, DEFAULT_RETRY_CNT);
	if(retval <0) {
		TS_LOG_ERR("[himax] %s: HX_CMD_TSSON fail!\n", __func__);
		return retval;
	}
	mdelay(HX_SLEEP_1MS);
	retval = i2c_himax_write_command(HX_CMD_TSSLPOUT, DEFAULT_RETRY_CNT);
	if(retval <0) {
		TS_LOG_ERR("[himax] %s: HX_CMD_TSSLPOUT fail!\n", __func__);
		return retval;
	}
	msleep(HX_SLEEP_30MS);

	//himax ic 852xf(auo module) resume need two reset and initial sensor on,this is second time
	gpio_direction_output(ts->rst_gpio, RST_DISABLE);
	mdelay(HX_SLEEP_5MS);
	gpio_direction_output(ts->rst_gpio, RST_ENABLE);
	msleep(HX_SLEEP_20MS);
	TS_LOG_INFO("%s second: pull reset gpio on. \n", __func__);
	data[0] = HX_REG_SET_CLK_ADDR;
	//adjust flash clock cmd
	data[1] = 0x06;
	data[2] = 0x03;
	retval = i2c_himax_master_write(&data[0],SET_CLK_DATA_LEN, sizeof(data),DEFAULT_RETRY_CNT);
	if(retval <0) {
		TS_LOG_ERR("[himax] %s: HX_REG_SET_CLK_ADDR fail!\n", __func__);
		return retval;
	}
	mdelay(HX_SLEEP_1MS);
	data[0] = HX_REG_SET_OSC_4_PUMP;
	//switch to mcu cmd
	data[1] = 0x11;
	data[2] = 0x00;
	retval = i2c_himax_master_write(&data[0],SET_CRYSTAL_DATA_LEN, sizeof(data),DEFAULT_RETRY_CNT);
	if(retval <0) {
		TS_LOG_ERR("[himax] %s: HX_REG_SET_OSC_4_PUMP fail!\n", __func__);
		return retval;
	}
	mdelay(HX_SLEEP_1MS);
	TS_LOG_INFO("%s: second: flash clock ok. \n", __func__);

	//Sense On
	retval = i2c_himax_write_command( HX_CMD_TSSON, DEFAULT_RETRY_CNT);
	if(retval <0) {
		TS_LOG_ERR("[himax] %s: HX_CMD_TSSON fail!\n", __func__);
		return retval;
	}
	mdelay(HX_SLEEP_1MS);
	retval = i2c_himax_write_command(HX_CMD_TSSLPOUT, DEFAULT_RETRY_CNT);
	if(retval <0) {
		TS_LOG_ERR("[himax] %s: HX_CMD_TSSLPOUT fail!\n", __func__);
		return retval;
	}

	TS_LOG_INFO("%s: exit \n", __func__);
	return NO_ERR;
}
static int himax_core_suspend(void)
{
#ifdef HX_CHIP_STATUS_MONITOR
	int t = 0;
#endif
	int retval = 0;
	struct himax_ts_data *ts = NULL;
//	int iCount = 0;
	TS_LOG_INFO("%s: Enter suspended. \n", __func__);

	ts = g_himax_ts_data;

	if(ts->firmware_updating) {
		TS_LOG_INFO("%s: tp fw is updating, return\n", __func__);
		return NO_ERR;
	}
	if(atomic_read(&hmx_mmi_test_status)) {
		TS_LOG_INFO("%s: tp fw is hmx_mmi_test_status, return\n", __func__);
		return NO_ERR;
	}
	if(ts->suspended)
	{
		TS_LOG_INFO("%s: Already suspended. Skipped. \n", __func__);
		return SUSPEND_IN;
	}
	else
	{
		ts->suspended = true;
		TS_LOG_INFO("%s: enter \n", __func__);
	}

#ifdef HX_TP_SYS_FLASH_DUMP
	if (getFlashDumpGoing())
	{
		TS_LOG_INFO("[himax] %s: Flash dump is going, reject suspend\n",__func__);
		return SUSPEND_REJECT;
	}
#endif

#ifdef HX_CHIP_STATUS_MONITOR
	if(HX_ON_HAND_SHAKING)//chip on hand shaking,wait hand shaking
	{
		for(t = 0; t < HX_HAND_SHAKING_MAX_TIME; t++)
			{
				if(HX_ON_HAND_SHAKING==0)//chip on hand shaking end
					{
						TS_LOG_INFO("%s:HX_ON_HAND_SHAKING OK check %d times\n",__func__,t);
						break;
					}
				else
				msleep(HX_SLEEP_1MS);
			}
		if(t == HX_HAND_SHAKING_MAX_TIME)
			{
				TS_LOG_ERR("%s:HX_ON_HAND_SHAKING timeout reject suspend\n",__func__);
				return HANDSHAKE_TIMEOUT_IN_SUSPEND;
			}
	}
#endif

#ifdef HX_CHIP_STATUS_MONITOR
	HX_CHIP_POLLING_COUNT = 0;
	cancel_delayed_work_sync(&ts->himax_chip_monitor);
#endif
	ts->first_pressed = 0;
	atomic_set(&ts->suspend_mode, 1);
	ts->pre_finger_mask = 0;

	msleep(HX_SLEEP_30MS);
//	if((atomic_read(&ts->irq_complete) == 0) && (iCount <10) ){
 	if(atomic_read(&ts->irq_complete) == 0){
	//	TS_LOG_INFO("%s irq function not complete. try time is %d\n",__func__, iCount);
		msleep(HX_SLEEP_10MS);
	//	iCount ++;
	}
	/*power down*/
	if (!g_tskit_pt_station_flag){
		gpio_direction_output(ts->pdata->gpio_3v3_en, GPIO_3V3_DISABLE);
		gpio_direction_output(ts->pdata->gpio_1v8_en, GPIO_1V8_DISABLE);
		gpio_direction_output(ts->rst_gpio, 0);
	}else{
		retval = himax_enter_sleep_mode();
		if(retval<0){
		   TS_LOG_ERR("[himax] %s: himax_enter_sleep_mode fail!\n", __func__);
		   return retval;
		}
	}

	TS_LOG_INFO("%s: exit \n", __func__);

	return NO_ERR;
}

static int himax_core_resume(void)
{
#ifdef HX_CHIP_STATUS_MONITOR
	int t=0;
#endif
	struct himax_ts_data *ts;
	uint8_t data[12] = {0};
	int retval=0;

	TS_LOG_INFO("%s: enter \n", __func__);

	ts = g_himax_ts_data;

	if(ts->firmware_updating) {
		TS_LOG_INFO("%s: tp fw is updating, return\n", __func__);
		return NO_ERR;
	}
	if(atomic_read(&hmx_mmi_test_status)) {
		TS_LOG_INFO("%s: tp fw is hmx_mmi_test_status, return\n", __func__);
		return NO_ERR;
	}

	if(ts->suspended)
	{
		TS_LOG_INFO("%s: will be resume \n", __func__);
	}
	else
	{
		TS_LOG_INFO("%s: Already resumed. Skipped. \n", __func__);
		return RESUME_IN;
	}
	/*power on*/
	if (!g_tskit_pt_station_flag){
		gpio_direction_output(ts->pdata->gpio_3v3_en, GPIO_3V3_ENABLE);
		udelay(VCCA_DELAY_TIME);
		gpio_direction_output(ts->pdata->gpio_1v8_en, GPIO_1V8_ENABLE);
		//himax 852xf(auo module) gpio power on also need two reset by call this function
		retval = himax_power_rst_init();
		if(retval <0){
			TS_LOG_ERR("[himax] %s: himax_power_rst_init fail!\n", __func__);
			return retval;
		}
	}else{
		retval = himax_exit_sleep_mode();
		if(retval <0){
			TS_LOG_ERR("[himax] %s: himax_exit_sleep_mode fail!\n", __func__);
			return retval;
		}
	}
	TS_LOG_INFO("%s: power on. \n", __func__);

	HW_RESET_ACTIVATE = HW_RST_FLAT_ENABLE;

#ifdef HX_CHIP_STATUS_MONITOR
	if(HX_ON_HAND_SHAKING)//chip on hand shaking,wait hand shaking
	{
		for(t = 0; t < HX_HAND_SHAKING_MAX_TIME; t++)
			{
				if(HX_ON_HAND_SHAKING==0)//chip on hand shaking end
					{
						TS_LOG_INFO("%s:HX_ON_HAND_SHAKING OK check %d times\n",__func__,t);
						break;
					}
				else
				msleep(HX_SLEEP_1MS);
			}
		if(t == HX_HAND_SHAKING_MAX_TIME)
			{
				TS_LOG_ERR("%s:HX_ON_HAND_SHAKING timeout reject resume\n",__func__);
				return HANDSHAKE_TIMEOUT_IN_RESUME;
			}
	}
#endif
	atomic_set(&ts->suspend_mode, 0);

#ifdef HX_CHIP_STATUS_MONITOR
	HX_CHIP_POLLING_COUNT = 0;
	queue_delayed_work(ts->himax_chip_monitor_wq, &ts->himax_chip_monitor, HX_POLLING_TIMER*HZ); //for ESD solution
#endif
	ts->suspended = false;

	TS_LOG_INFO("%s: exit \n", __func__);

	return NO_ERR;
}

static int __init himax_module_init(void)
{
    bool found = false;
    struct device_node* child = NULL;
    struct device_node* root = NULL;
    int err = NO_ERR;

    TS_LOG_INFO("[HXTP] himax_module_init called here\n");
    g_himax_ts_data = kzalloc(sizeof(struct himax_ts_data), GFP_KERNEL);
    if (!g_himax_ts_data) {
		TS_LOG_ERR("Failed to alloc mem for struct g_himax_ts_data\n");
       err =  -ENOMEM;
       goto himax_ts_data_alloc_fail;
    }
	g_himax_ts_data->tskit_himax_data = kzalloc(sizeof(struct ts_kit_device_data), GFP_KERNEL);
    if (!g_himax_ts_data->tskit_himax_data ) {
		TS_LOG_ERR("Failed to alloc mem for struct tskit_himax_data\n");
       err =  -ENOMEM;
       goto tskit_himax_data_alloc_fail;
    }
    root = of_find_compatible_node(NULL, NULL, "huawei,ts_kit");
    if (!root)
    {
	TS_LOG_ERR("[HXTP]huawei_ts, find_compatible_node huawei,ts_kit error\n");
        err = -EINVAL;
        goto out;
    }
	/*find the chip node*/
    for_each_child_of_node(root, child)
    {
        if (of_device_is_compatible(child, HIMAX_VENDER_NAME))
        {
            found = true;
            break;
        }
    }
    if (!found)
    {
	TS_LOG_ERR(" not found chip himax child node  !\n");
        err = -EINVAL;
        goto out;
    }

    g_himax_ts_data->tskit_himax_data->cnode = child;
    g_himax_ts_data->tskit_himax_data->ops = &ts_kit_himax_ops;

    err = huawei_ts_chip_register(g_himax_ts_data->tskit_himax_data);
    if(err)
    {
		TS_LOG_ERR(" himax chip register fail !\n");
		goto out;
    }
    TS_LOG_INFO("himax chip_register sucess! teturn value=%d\n", err);
    return err;
out:
	if (g_himax_ts_data->tskit_himax_data){
		kfree(g_himax_ts_data->tskit_himax_data);
		g_himax_ts_data->tskit_himax_data = NULL;
	}
tskit_himax_data_alloc_fail:
	if (g_himax_ts_data){
		kfree(g_himax_ts_data);
		g_himax_ts_data = NULL;
	}
himax_ts_data_alloc_fail:

	return err;

}

static void __exit himax_module_exit(void)
{

	TS_LOG_INFO("himax_module_exit called here\n");
	freeMutualBuffer();
	freeFlashBuffer();

	if (g_himax_ts_data->tskit_himax_data){
		kfree(g_himax_ts_data->tskit_himax_data);
		g_himax_ts_data->tskit_himax_data = NULL;
	}
	wake_lock_destroy(&g_himax_ts_data->ts_flash_wake_lock);

	if (g_himax_ts_data){
		kfree(g_himax_ts_data);
		g_himax_ts_data = NULL;
	}
    return;
}

static void himax_shutdown(void)
{
	TS_LOG_INFO("%s himax_shutdown call power off\n",__func__);
	himax_power_off();
	return;
}
static int himax_power_off(void)
{
	int err=0;
	TS_LOG_INFO("%s:enter\n", __func__);

	if (g_himax_ts_data->pdata->gpio_3v3_en >= 0) {
		err = gpio_direction_output(g_himax_ts_data->pdata->gpio_3v3_en, 0);
		if (err) {
			TS_LOG_ERR("unable to set direction for gpio [%d]\n",
				g_himax_ts_data->pdata->gpio_3v3_en);
			return err;
		}
	}
	if (g_himax_ts_data->pdata->gpio_1v8_en >= 0) {
		err = gpio_direction_output(g_himax_ts_data->pdata->gpio_1v8_en, 0);
		if (err) {
			TS_LOG_ERR("unable to set direction for gpio [%d]\n",
				g_himax_ts_data->pdata->gpio_1v8_en);
			return err;
		}
	}

	himax_power_off_gpio_set();
	return err;
}

static void himax_power_off_gpio_set(void)
{

	TS_LOG_INFO("%s:enter\n", __func__);

	if (g_himax_ts_data->pdata->gpio_reset >= 0) {
		gpio_free(g_himax_ts_data->pdata->gpio_reset);
	}
	if (g_himax_ts_data->pdata->gpio_3v3_en >= 0) {
		gpio_free(g_himax_ts_data->pdata->gpio_3v3_en);
	}
	if (g_himax_ts_data->pdata->gpio_1v8_en >= 0) {
		gpio_free(g_himax_ts_data->pdata->gpio_1v8_en);
	}
	if (gpio_is_valid(g_himax_ts_data->pdata->gpio_irq)) {
		gpio_free(g_himax_ts_data->pdata->gpio_irq);
	}
	TS_LOG_INFO("%s:exit\n", __func__);

}

static int himax_algo_cp(struct ts_kit_device_data *dev_data, struct ts_fingers *in_info, struct ts_fingers *out_info)
{
	int index = 0;
	int id = 0;
	TS_LOG_INFO("%s Enter",__func__);
	if((NULL == dev_data)||(NULL == in_info)||(NULL == out_info)) {
		return HX_ERROR;
	}
	for (index = 0, id = 0; index < TS_MAX_FINGER; index++, id++) {
		if (in_info->cur_finger_number == 0) {
			out_info->fingers[0].status = TS_FINGER_RELEASE;
			if (id >= 1)
				out_info->fingers[id].status = 0;
		} else {
			if (in_info->fingers[index].x != 0
			    || in_info->fingers[index].y != 0) {
				if (HIMAX_EV_TOUCHDOWN ==
				    in_info->fingers[index].event
				    || HIMAX_EV_MOVE ==
				    in_info->fingers[index].event
				    || HIMAX_EV_NO_EVENT ==
				    in_info->fingers[index].event) {
					out_info->fingers[id].x =
					    in_info->fingers[index].x;
					out_info->fingers[id].y =
					    in_info->fingers[index].y;
					out_info->fingers[id].pressure =
					    in_info->fingers[index].pressure;
					out_info->fingers[id].status =
					    TS_FINGER_PRESS;
				} else if (HIMAX_EV_LIFTOFF ==
					   in_info->fingers[index].event) {
					out_info->fingers[id].status =
					    TS_FINGER_RELEASE;
				}
			} else
				out_info->fingers[id].status = 0;
		}
	}
	return NO_ERR;
}

static struct ts_algo_func himax_algo_f1 = {
	.algo_name = "himax_algo_cp",
	.chip_algo_func = himax_algo_cp,
};

static int himax_register_algo(struct ts_kit_device_data *chip_data)
{
	int retval =  -EIO;
	TS_LOG_INFO("%s: himax_reg_algo called\n", __func__);
	if(NULL == chip_data) {
		return retval;
	}
	retval = register_ts_algo_func(chip_data, &himax_algo_f1);

	return retval;
}

static int himax_chip_get_info(struct ts_chip_info_param *info)
{
	int retval = NO_ERR;

	TS_LOG_INFO("%s Enter\n", __func__);
	if(NULL == info) {
		return HX_ERROR;
	}
#if 0
	if (himax_read_projectid() < 0) {
			TS_LOG_ERR("%s read project id error!\n", __func__);
		}
#endif
	snprintf(info->ic_vendor, PAGE_SIZE, "himax-%s", himax_product_id);
	snprintf(info->mod_vendor, PAGE_SIZE, g_himax_ts_data->tskit_himax_data->ts_platform_data->chip_data->module_name);

	snprintf(info->fw_vendor, PAGE_SIZE,
		"%x.%x.%x",
		g_himax_ts_data->vendor_fw_ver_H,
		g_himax_ts_data->vendor_fw_ver_L,
		g_himax_ts_data->vendor_config_ver);

	return retval;
}
/*lint -save -e* */
late_initcall(himax_module_init);
module_exit(himax_module_exit);
/*lint -restore*/
MODULE_AUTHOR("Huawei Device Company");
MODULE_DESCRIPTION("Huawei TouchScreen Driver");
MODULE_LICENSE("GPL");

/* CPM2016032400222 shihuijun 20160324 end > */

