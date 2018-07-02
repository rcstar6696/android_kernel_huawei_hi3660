/*
 * Copyright (C) 2010 - 2016 Novatek, Inc.
 *
 * $Revision: 4017 $
 * $Date: 2016-04-01 09:41:08 +0800 (星期五, 01 四月 2016) $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */
#ifndef 	_LINUX_NVT_TOUCH_H
#define		_LINUX_NVT_TOUCH_H

#include <linux/i2c.h>
#include <linux/input.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include "../../huawei_ts_kit.h"
//---GPIO number---
#define NVTTOUCH_RST_PIN 980
#define NVTTOUCH_INT_PIN 943
#define NVTTOUCH_DISP_RST_PIN 956


//---INT trigger mode---
//#define IRQ_TYPE_EDGE_RISING 1
//#define IRQ_TYPE_EDGE_FALLING 2
//#define IRQ_TYPE_LEVEL_HIGH 4
//#define IRQ_TYPE_LEVEL_LOW 8
#define INT_TRIGGER_TYPE IRQ_TYPE_EDGE_FALLING


//---I2C driver info.---
#define NVT_I2C_NAME "NVT-ts"
#define I2C_BLDR_Address 0x01
#define I2C_FW_Address 0x01
#define I2C_HW_Address 0x62


//---Input device info.---
#define NVT_TS_NAME "NVTCapacitiveTouchScreen"


//---Touch info.---
#define TOUCH_MAX_WIDTH 1080
#define TOUCH_MAX_HEIGHT 1920
#define TOUCH_MAX_FINGER_NUM 10
#define TOUCH_KEY_NUM 0
#define ONE_SIZE 1
#if TOUCH_KEY_NUM > 0
extern const uint16_t touch_key_array[TOUCH_KEY_NUM];
#endif
#define TOUCH_FORCE_NUM 1000

//---Customerized func.---
#define NVT_TOUCH_PROC 1		// for novatek cmdline tools and apk
#define NVT_TOUCH_EXT_PROC 1	// for novatek debug

#define BOOT_UPDATE_FIRMWARE 0
#define BOOT_UPDATE_FIRMWARE_NAME "novatek_ts_fw.bin"

#define PROJECT_ID_LEN 9
#define SUSPEND_CMD_BUF_SIZE	2
#define POWER_SLEEP_MODE	1
#define REAL_PROJECT_ID_LEN 10

struct nvt_ts_data {
	struct ts_kit_device_data *chip_data;
	struct platform_device *ts_dev;
	struct regulator *tp_vci;
	struct regulator *tp_vddio;
	
#ifndef CONFIG_OF
	struct iomux_block *tp_gpio_block;
	struct block_config *tp_gpio_block_config;
#else
	struct pinctrl *pctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_idle;
#endif
	struct mutex i2c_mutex;
	struct mutex mp_mutex;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct nvt_work;
	struct delayed_work nvt_fwu_work;
	uint16_t addr;
	int8_t phys[32];
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	uint16_t abs_x_max;
	uint16_t abs_y_max;
	uint8_t x_num;
	uint8_t y_num;
	uint8_t max_touch_num;
	uint8_t max_button_num;
	uint32_t int_trigger_type;
	int32_t irq_gpio;
	uint32_t irq_flags;
	int32_t reset_gpio;
	uint32_t support_aft;
	uint32_t reset_flags;
	int32_t disp_rst_gpio;
	uint32_t disp_rst_flags;
	bool print_criteria;
	uint32_t noise_test_frame;
	uint32_t open_test_by_fw;
	uint32_t project_id_flash_address;
	uint32_t csvfile_use_system;
	uint32_t test_capacitance_via_csvfile;
	uint32_t i2c_retry_time;
	bool criteria_threshold_flag;
	bool nvttddi_channel_flag;
	uint32_t *NvtTddi_X_Channel;
	uint32_t *NvtTddi_Y_Channel;
	uint32_t *PS_Config_Lmt_FW_CC_P;
	uint32_t *PS_Config_Lmt_FW_CC_N;
	uint32_t *PS_Config_Lmt_FW_Diff_P;
	int32_t *PS_Config_Lmt_FW_Diff_N;
	uint32_t *PS_Config_Lmt_Short_Rawdata_P;
	uint32_t *PS_Config_Lmt_Short_Rawdata_N;
	uint32_t *mADCOper_Cnt;
	uint32_t *PS_Config_Lmt_FW_Rawdata_P;
	uint32_t *PS_Config_Lmt_FW_Rawdata_N;
	uint32_t *PS_Config_Lmt_FW_Rawdata_X_Delta;
	uint32_t *PS_Config_Lmt_FW_Rawdata_Y_Delta;
	uint32_t *PS_Config_Lmt_Open_Rawdata_P_A;
	uint32_t *PS_Config_Lmt_Open_Rawdata_N_A;
	uint32_t *NVT_TDDI_AIN_X;
	uint32_t *NVT_TDDI_AIN_Y;
	struct nvt_ts_mem_map *mmap;
	uint8_t carrier_system;
	uint8_t power_sleep_mode;
	uint32_t nvt_chip_id_partone;
	uint32_t nvt_chip_id_parttwo;
	uint32_t nvt_chip_id_partthree;
	bool gesture_module;
};

#if NVT_TOUCH_PROC
struct nvt_flash_data{
	rwlock_t lock;
	struct i2c_client *client;
};
#endif

struct nvt_ts_mem_map {
	uint32_t EVENT_BUF_ADDR;
	uint32_t RAW_PIPE0_ADDR;
	uint32_t RAW_PIPE0_Q_ADDR;
	uint32_t RAW_PIPE1_ADDR;
	uint32_t RAW_PIPE1_Q_ADDR;
	uint32_t BASELINE_ADDR;
	uint32_t BASELINE_Q_ADDR;
	uint32_t BASELINE_BTN_ADDR;
	uint32_t BASELINE_BTN_Q_ADDR;
	uint32_t DIFF_PIPE0_ADDR;
	uint32_t DIFF_PIPE0_Q_ADDR;
	uint32_t DIFF_PIPE1_ADDR;
	uint32_t DIFF_PIPE1_Q_ADDR;
	uint32_t RAW_BTN_PIPE0_ADDR;
	uint32_t RAW_BTN_PIPE0_Q_ADDR;
	uint32_t RAW_BTN_PIPE1_ADDR;
	uint32_t RAW_BTN_PIPE1_Q_ADDR;
	uint32_t DIFF_BTN_PIPE0_ADDR;
	uint32_t DIFF_BTN_PIPE0_Q_ADDR;
	uint32_t DIFF_BTN_PIPE1_ADDR;
	uint32_t DIFF_BTN_PIPE1_Q_ADDR;
	uint32_t READ_FLASH_CHECKSUM_ADDR;
	uint32_t RW_FLASH_DATA_ADDR;
};


typedef enum {
	RESET_STATE_INIT = 0xA0,// IC reset
	RESET_STATE_REK,		// ReK baseline
	RESET_STATE_REK_FINISH,	// baseline is ready
	RESET_STATE_NORMAL_RUN	// normal run
} RST_COMPLETE_STATE;
#define LOW_EIGHT_BITS(x)			((x)&0xFF)
#define MIDDLE_EIGHT_BITS(x)		(((x)&0xFF00)>>8)
#define HIGHT_EIGHT_BITS(x)			(((x)&0xFF0000)>>16)
#define NVTTDDI_ERR	-1
#define  IC_X_CFG_SIZE	18
#define  IC_Y_CFG_SIZE	32
#define NVT_TDDI_IC_ARRAY_SIZE	40
#define NVTTDDI_X_CHANNEL_NUM	18
#define NVTTDDI_Y_CHANNEL_NUM	30
#define NVTTDDI_TWO_BYTES_LENGTH	2
#define NVTTDDI_THREE_BYTES_LENGTH	3
#define NVTTDDI_FIVE_BYTES_LENGTH	5
#define NVTTDDI_DOUBLE_ZERO_CMD	0x00
#define NVTTDDI_ZERO_ONE_CMD		0x01
#define NVTTDDI_ZERO_TWO_CMD		0x02
#define NVTTDDI_ZERO_FIVE_CMD		0x05
#define NVTTDDI_ZERO_SIX_CMD			0x06
#define NVTTDDI_ONE_E_CMD			0x1E
#define NVTTDDI_TWO_ZERO_CMD		0x20
#define NVTTDDI_FOUR_FIVE_CMD		0x45
#define NVTTDDI_FOUR_SEVEN_CMD		0x47
#define NVTTDDI_FIVE_ZERO_CMD		0x50
#define NVTTDDI_DOUBLE_A_CMD		0xAA
#define NVTTDDI_DOUBLE_F_CMD		0xFF
#define NVTTDDI_DELAY_10_MS			10
#define NVTTDDI_DELAY_20_MS			20
#define NVTTDDI_DELAY_30_MS			30
#define NVTTDDI_RETRY_5_TIMES		5
#define NVTTDDI_FRAME_NUMBER		1
#define NVTTDDI_TEST_FRAME_DIVIDE_NUM	10
#define NVTTDDI_MULTIPLY_7_NUM		7
#define NVTTDDI_MULTIPLY_2_NUM		2
#define NVTTDDI_MULTIPLY_256_NUM		256
#define NVTTDDI_PLUS_ONE				1

/*gesture mode*/

#define DOUBLE_CLICK_WAKEUP	15
//const uint16_t gesture_key_array = KEY_POWER;
//static struct wake_lock gestrue_wakelock;
#define EVENT_MAP_HOST_CMD                    0x50
#define IS_APP_ENABLE_GESTURE(x)  ((u32)(1<<x))

#define FLAG_EXIST	1
#define U8_MIN		0
#define U8_MAX		0xFF
#endif /* _LINUX_NVT_TOUCH_H */
