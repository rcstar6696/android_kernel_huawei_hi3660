/*
 * goodix_ts_test.c - TP test
 *
 * Copyright (C) 2015 - 2016 Goodix Technology Incorporated   
 * Copyright (C) 2015 - 2016 Yulong Cai <caiyulong@goodix.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be a reference
 * to you, when you are integrating the GOODiX's CTP IC into your system,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 */
#include <linux/slab.h>
#include <linux/delay.h>
#include "goodix_ts.h"

#define DRV_NUM_OFFSET	(0x807E - 0x8050)
#define SEN_NUM_OFFSET	(0x8080 - 0x8050)
#define KEY_NUM_OFFSET	(0x80A3 - 0x8050)
#define MODULE_SWITCH_DK_OFFSET	(0x8057 - 0x8050)
#define MODULE_SWITCH_SK_OFFSET	(0x805A - 0x8050)

#define MAX_DRV_NUM		32
#define MAX_SEN_NUM		32
#define FLOAT_AMPLIFIER 1000

/**
 * struct ts_test_params - test parameters
 * drv_num: touch panel tx(driver) number
 * sen_num: touch panel tx(sensor) number
 * sc_drv_num: screen tx number
 * sc_sen_num: screen rx number
 * key_num: key number
 * max_limits: max limits of rawdata
 * min_limits: min limits of rawdata
 * deviation_limits: channel deviation limits
 * short_threshold: short resistance threshold
 * r_drv_drv_threshold: resistance threshold between drv and drv
 * r_drv_sen_threshold: resistance threshold between drv and sen
 * r_sen_sen_threshold: resistance threshold between sen and sen
 * r_drv_gnd_threshold: resistance threshold between drv and gnd
 * r_sen_gnd_threshold: resistance threshold between sen and gnd
 * avdd_value: avdd voltage value
 */
struct ts_test_params {
	u32 drv_num;
	u32 sen_num;
	u32 sc_drv_num;
	u32 sc_sen_num;
	u32 key_num;

	u32 max_limits[MAX_DRV_NUM * MAX_SEN_NUM];
	u32 min_limits[MAX_DRV_NUM * MAX_SEN_NUM];
	u32 deviation_limits[MAX_DRV_NUM * MAX_SEN_NUM];

	u16 short_threshold;
	u16 r_drv_drv_threshold;
	u16 r_drv_sen_threshold;
	u16 r_sen_sen_threshold;
	u16 r_drv_gnd_threshold;
	u16 r_sen_gnd_threshold;
	u16 avdd_value;
};

/**
 * struct ts_test_rawdata - rawdata structure
 * addr: rawdata address in chip ram
 * data: rawdata buffer, u16, big-endian
 * size: rawdata size
 */
struct ts_test_rawdata {
	u16 addr;
	u16 data[MAX_SEN_NUM * MAX_DRV_NUM];
	u32 size;
};

/**
 * struct goodix_ts_test - main data structrue
 * ts: goodix touch screen data
 * test_config: test mode config data
 * orig_config: original config data
 * test_param: test parameters
 * rawdata: raw data structure
 * basedata: base data 
 * product_id: product id infomation
 * test_result: test result string
 * failed_reason: test failed reason
 */
struct goodix_ts_test {
	struct goodix_ts_data *ts;
	struct goodix_ts_config test_config;
	struct goodix_ts_config orig_config;
	struct ts_test_params test_params;
	struct ts_test_rawdata rawdata;
	struct ts_test_rawdata basedata;
	const char *product_id;

	char test_result[TS_RAWDATA_RESULT_MAX];
	char failed_reason[TS_RAWDATA_RESULT_MAX];
};

static struct goodix_ts_test *gts_test;

/**
 * goodix_hw_testmode - switch firmware to test mode
 */
static inline int goodix_hw_testmode(struct goodix_ts_test *ts_test)
{
	int ret = -ENODEV;

	if  (ts_test->ts->ops.send_cmd)
		ret = ts_test->ts->ops.send_cmd(0x01, 0x00);

	return ret;
}

/**
 * gt1151_passe_config - resolve tx/rx number and key number
 */
static int gt1151_passe_config(struct goodix_ts_config *test_config,
				struct ts_test_params *test_params)
{
	int i;

	test_params->drv_num = (test_config->data[DRV_NUM_OFFSET] & 0x1F)
					+ (test_config->data[DRV_NUM_OFFSET + 1] & 0x1F);
	test_params->sen_num = test_config->data[SEN_NUM_OFFSET] & 0xFF;

	if(test_config->data[MODULE_SWITCH_DK_OFFSET] & 0x01){  /* has key */
		if (test_config->data[MODULE_SWITCH_SK_OFFSET] & 0x80) {
			/* single chanel key */
			test_params->key_num = 0;
			for (i = 0; i < 3; i++) {
				if (test_config->data[KEY_NUM_OFFSET + i])
					test_params->key_num++;
			}

			test_params->sc_drv_num = test_params->drv_num - 1;
			test_params->sc_sen_num = test_params->sen_num
							- test_params->key_num;
		} else {
			test_params->sc_sen_num = test_params->sen_num - 1;
			test_params->sc_drv_num = test_params->drv_num;
			test_params->key_num = test_params->drv_num;
		}
	} else {
		test_params->sc_drv_num = test_params->drv_num;
		test_params->sc_sen_num = test_params->sen_num;
		test_params->key_num = 0;
	}

	GTP_INFO("tx_num:%d,rx_num:%d,sc_tx_num:%d,sc_rx_num:%d,key_num:%d",
		test_params->drv_num, test_params->sen_num,
		test_params->sc_drv_num, test_params->sc_sen_num,
		test_params->key_num);

	return 0;
}

/**
 * goodix_init_testlimits - get test limits(max/min/...) from dt
 */
static int goodix_init_testlimits(struct goodix_ts_test *ts_test)
{
	struct device_node *device;
	char project_id[20];
	int ret, i, size;
	u32 value[8];

	GTP_DEBUG("Get test limits from dt");


	sprintf(project_id, "goodix-sensorid-%u",
						ts_test->ts->hw_info.sensor_id);
	device = of_find_compatible_node(NULL, NULL, project_id);
	if (!device) {
		GTP_ERROR("No chip specific dts: %s, need to parse",
			    project_id);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(device,
					"raw_data_limit", &value[0], 3);
	if (ret < 0) {
		GTP_ERROR("Failed to read raw data limits from dt:%d", ret);
		return ret;
	} else {
		for (i = 0; i < MAX_SEN_NUM * MAX_DRV_NUM; i++) {
			ts_test->test_params.max_limits[i] = value[0];
			ts_test->test_params.min_limits[i] = value[1];
			ts_test->test_params.deviation_limits[i] = value[2];
		}
		GTP_INFO("raw_data_limit:%u %u %u", value[0],
			value[1], value[2]);
	}


	size = of_property_count_u32_elems(device, 
			"special_raw_data_limit");
	if (size > 0 && (size % 4) == 0) {
		int nodes = size / 4;
		u32 *array = kmalloc(size * sizeof(u32), GFP_KERNEL);
		if (!array) {
			GTP_ERROR("Failed to alloc memory");
			return -ENOMEM;
		}

		ret = of_property_read_u32_array(device,
				"special_raw_data_limit", array, size);
		if (ret < 0) {
			GTP_ERROR("Failed to parse special_raw_data_limit");
			kfree(array);
			return ret;
		}

		for (i = 0; i < nodes; i++) {
			if (array[i * 4] < MAX_SEN_NUM * MAX_DRV_NUM) {
				ts_test->test_params.max_limits[array[i * 4]] =
						array[i * 4 + 1];
				ts_test->test_params.min_limits[array[i * 4]] =
						array[i * 4 + 2];
				ts_test->test_params.deviation_limits[array[i * 4]] =
						array[i * 4 + 3];
			}
		}
	}

	ret = of_property_read_u32_array(device,
					"shortciurt_threshold", &value[0], 7);
	if (ret < 0) {
		GTP_ERROR("Failed to read shortcircut threshold from dt:%d", ret);
		return ret;
	} else {
		ts_test->test_params.short_threshold = value[0];
		ts_test->test_params.r_drv_drv_threshold = value[1];
		ts_test->test_params.r_drv_sen_threshold = value[2];
		ts_test->test_params.r_sen_sen_threshold = value[3];
		ts_test->test_params.r_drv_gnd_threshold = value[4];
		ts_test->test_params.r_sen_gnd_threshold = value[5];
		ts_test->test_params.avdd_value = value[6];
		GTP_INFO("shortciurt_threshold:%u %u %u %u %u %u %u",
			value[0], value[1], value[2], value[3], value[4],
			value[5], value[6]);
	}

	return 0;
}

/**
 * goodix_read_origconfig - read original config data
 */
static int goodix_read_origconfig(struct goodix_ts_test *ts_test)
{
	int ret = -ENODEV, cfg_size;

	GTP_DEBUG("Read original config data");

	cfg_size = GTP_CONFIG_ORG_LENGTH;
	if (ts_test->ts->ops.i2c_read) {
		ret = ts_test->ts->ops.i2c_read(GTP_REG_CONFIG_DATA,
				&ts_test->orig_config.data[0], cfg_size);
		if (ret < 0) {
			GTP_ERROR("Failed to read original config data");
			return ret;
		}

		if (ts_test->orig_config.data[EXTERN_CFG_OFFSET] & 0x40)  {
			ret = ts_test->ts->ops.i2c_read(GTP_REG_EXT_CONFIG,
				&ts_test->orig_config.data[cfg_size],
				GTP_CONFIG_EXT_LENGTH);
			if (ret < 0) {
				GTP_ERROR("Failed to read external config data");
				return ret;
			}
			cfg_size += GTP_CONFIG_EXT_LENGTH;
		}

		ts_test->orig_config.size = cfg_size;
		ts_test->orig_config.delay_ms = 200;
		ts_test->orig_config.name = "original_config";
		ts_test->orig_config.initialized = true;
	}

	return ret;
}

/**
 * goodix_tptest_prepare - preparation before tp test
 */
static int goodix_tptest_prepare(struct goodix_ts_test *ts_test)
{
	struct goodix_ts_config *test_config;
	int ret;

	GTP_INFO("TP test preparation");
	ret = of_property_read_string(ts_test->ts->pdev->dev.of_node,
					"product_id", &ts_test->product_id);
	if (ret) {
		GTP_ERROR("Invalid product id");
		return -EINVAL;
	}

	test_config = &ts_test->test_config;
	/* parse test config data form dts */
	ret = -ENODEV;
	if (ts_test->ts->ops.parse_cfg_data)
		ret = ts_test->ts->ops.parse_cfg_data(ts_test->ts,
			"tptest_config", &test_config->data[0], &test_config->size,
			ts_test->ts->hw_info.sensor_id);

	if (ret < 0) {
		GTP_ERROR("Failed to parse tptest-config:%d",ret);
		return ret;
	} else {
		test_config->initialized = true;
		test_config->name = "tptest_config";
		test_config->delay_ms = 40;
	}

	/* parse sensro & driver number */
	if (!strcmp(ts_test->product_id, "GT1151")) {
		gt1151_passe_config(&ts_test->test_config,
						&ts_test->test_params);
		ts_test->rawdata.addr = 0xB798;
		ts_test->basedata.addr = 0xB378;
	} else {
		return -ENODEV;
	}

	/* get test limits from dt */
	ret = goodix_init_testlimits(ts_test);
	if (ret < 0) {
		GTP_ERROR("Failed to init testlimits from dt:%d", ret);
		return ret;
	}

	ret = goodix_read_origconfig(ts_test);
	if (ret < 0)
		return ret;

	/* same config version, ensure firmware will accept
		this test config data */
	test_config->data[0] = ts_test->orig_config.data[0];
	ret = -ENODEV;
	if (ts_test->ts->ops.send_cfg)
		ret = ts_test->ts->ops.send_cfg(test_config);
	if (ret < 0) {
		GTP_ERROR("Failed to send test config:%d", ret);
		return ret;
	}

	return 0;
}

/**
 * goodix_tptest_finish - finish test
 */
static inline int goodix_tptest_finish(struct goodix_ts_test *ts_test)
{
	int ret = -ENODEV;

	GTP_INFO("TP test finish");
	goodix_chip_reset();
	if (ts_test->ts->ops.send_cfg)
		ret = ts_test->ts->ops.send_cfg(&ts_test->orig_config);
	if (ret < 0) {
		GTP_ERROR("Failed to send normal config:%d", ret);
		return ret;
	}

	return 0;
}

/**
 * goodix_cache_rawdata - cache rawdata
 */
static int goodix_cache_rawdata(struct goodix_ts_test *ts_test)
{
	int ret = -1, i, retry = 40;
	u8 buf[1] = {0x00};
	u32 rawdata_size;

	GTP_DEBUG("Cache rawdata");
	rawdata_size = ts_test->test_params.sc_sen_num *
			ts_test->test_params.sc_drv_num +
			ts_test->test_params.key_num;

	if (rawdata_size > MAX_DRV_NUM * MAX_SEN_NUM) {
		GTP_ERROR("Invalid rawdata size(%u)", rawdata_size);
		return -EINVAL;
	}

	while (retry--) {
		ret = ts_test->ts->ops.i2c_read(GTP_READ_COOR_ADDR, &buf[0], 1);
		if (ret < 0 || (buf[0] & 0x80) == 0x80)
			break;
		usleep_range(5000, 5010);
	}

	if (!ret && (buf[0] & 0x80) == 0x80) {
		/* cache rawdata */
		ret = ts_test->ts->ops.i2c_read(ts_test->rawdata.addr,
				(u8 *)&ts_test->rawdata.data[0], rawdata_size * 2);
		if (ret < 0) {
			GTP_ERROR("Failed to read rawdata:%d", ret);
			return ret;
		}

		for (i = 0; i < rawdata_size; i++)
			ts_test->rawdata.data[i] =
					be16_to_cpu(ts_test->rawdata.data[i]);
		ts_test->rawdata.size = rawdata_size;
		GTP_INFO("Rawdata ready");
	} else {
		GTP_ERROR("Read rawdata timeout");
		ret = -EFAULT;
	}

	buf[0] = 0x00;
	ts_test->ts->ops.i2c_write(GTP_READ_COOR_ADDR, &buf[0], 1);
	return ret;
}

static int goodix_store_nosiedata(struct goodix_ts_test *ts_test,
				struct ts_rawdata_info *info)
{
	int ret = -1, retry = 20, i;
	u8 buf[1] = {0x00};
	u32 data_size;

	data_size = ts_test->test_params.sc_sen_num *
			ts_test->test_params.sc_drv_num +
			ts_test->test_params.key_num;

	if (data_size > MAX_DRV_NUM * MAX_SEN_NUM) {
		GTP_ERROR("Invalid rawdata size(%u)", data_size);
		return -EINVAL;
	}

	while (retry--) {
		ret = ts_test->ts->ops.i2c_read(GTP_READ_COOR_ADDR, &buf[0], 1);
		if (ret < 0 || (buf[0] & 0x80) == 0x80)
			break;
		usleep_range(5000, 5010);
	}

	if (!ret && (buf[0] & 0x80) == 0x80) {
		ret = ts_test->ts->ops.i2c_read(ts_test->basedata.addr,
				(u8 *)&ts_test->basedata.data[0], data_size * 2);
		if (ret < 0) {
			GTP_ERROR("Failed to read basedata:%d", ret);
			return ret;
		}

		for (i = 0; i < data_size; i++)
			ts_test->basedata.data[i] =
					be16_to_cpu(ts_test->basedata.data[i]);
		ts_test->basedata.size = data_size;

		if (ts_test->basedata.size + info->used_size <=
						TS_RAWDATA_BUFF_MAX) {
			for (i = 0; i < ts_test->basedata.size; i++)
				info->buff[info->used_size + i] =
						ts_test->basedata.data[i] - 
						ts_test->rawdata.data[i];
			info->used_size += ts_test->basedata.size;
		} else {
			GTP_ERROR("Invalid basedata size[%u]",
						ts_test->basedata.size);
		}
	} else {
		GTP_ERROR("Read basedata timeout");
		ret = -EFAULT;
	}

	buf[0] = 0x00;
	ts_test->ts->ops.i2c_write(GTP_READ_COOR_ADDR, &buf[0], 1);

	return ret;
}

static int goodix_rawcapacitance_test(struct ts_test_rawdata *rawdata,
				struct ts_test_params *test_params)
{
	bool pass = true;
	int i;

	for (i = 0; i < rawdata->size; i++) {
		if (rawdata->data[i] > test_params->max_limits[i]) {
			GTP_ERROR("rawdata[%d][%d]:%u > max_limit:%u, NG", 
				i / test_params->sc_sen_num, i % test_params->sc_sen_num,
				rawdata->data[i], test_params->max_limits[i]);
			pass = false;
		}

		if (rawdata->data[i] < test_params->min_limits[i]) {
			GTP_ERROR("rawdata[%d][%d]:%u < mix_limit:%u, NG",
				i / test_params->sc_sen_num, i % test_params->sc_sen_num,
				rawdata->data[i], test_params->min_limits[i]);
			pass = false;
		}
	}

	return pass ? 0 : -1;
}

static int goodix_deltacapacitance_test(struct ts_test_rawdata *rawdata,
				struct ts_test_params *test_params)
{
	u32 sc_data_num, max_val = 0, rawdata_val;
	u32 up = 0, down = 0, left = 0, right = 0;
	int cols = test_params->sc_sen_num;
	bool pass = true;
	int i;

	sc_data_num = test_params->sc_drv_num * test_params->sc_sen_num;

	for (i = 0; i < sc_data_num; i++) {
		rawdata_val = rawdata->data[i];
		max_val = 0;
		if (i - cols >= 0) {
			up = rawdata->data[i - cols];
			up = abs(rawdata_val - up);
			if (up > max_val)
				max_val = up;
		}

		if (i + cols < sc_data_num) {
			down = rawdata->data[i + cols];
			down = abs(rawdata_val - down);
			if (down > max_val)
				max_val = down;
		}

		if (i % cols) {
			left = rawdata->data[i - 1];
			left = abs(rawdata_val - left);
			if (left > max_val)
				max_val = left;
		}

		if ((i + 1) % cols) {
			right = rawdata->data[i + 1];
			right = abs(rawdata_val - right);
			if (right > max_val)
				max_val = right;
		}

		/* float to integer */
		max_val *= FLOAT_AMPLIFIER;
		max_val /= rawdata_val;
		if (max_val > test_params->deviation_limits[i]) {
			GTP_ERROR("deviation[%d][%d]:%u > delta_limit:%u, NG",
			i / cols, i % cols, max_val,
			test_params->deviation_limits[i]);
			pass = false;
		}
	}

	return pass ? 0 : -1;
}

static int goodix_capacitance_test(struct goodix_ts_test *ts_test,
				struct ts_rawdata_info *info)
{
	int test_result0 = -1, test_result1 = -1;
	int test_cnt = 0;
	int ret, i;

begin_test:
	if (test_cnt > 1)
		goto end_test;

	for (i = 0; i < 4; i++) {
		GTP_INFO("RawCap Test:%d:%d", test_cnt, i);
		ret = goodix_cache_rawdata(ts_test);
		if (ret < 0) {
			/* test NG, re-test once */
			test_cnt++;
			test_result0 = -1;
			test_result1 = -1;
			goto begin_test;
		}

		test_result0 = goodix_rawcapacitance_test(&ts_test->rawdata,
						&ts_test->test_params);
		test_result1 = goodix_deltacapacitance_test(&ts_test->rawdata,
						&ts_test->test_params);
		if (test_result0 < 0 || test_result1 < 0) {
			/* test NG, re-test once */
			test_cnt++;
			goto begin_test;
		}
	}

end_test:
	if (!test_result0) {
		/* test OK*/
		strcat(ts_test->test_result, "-1P");
		
	} else {
		strcat(ts_test->test_result, "-1F");
		GTP_ERROR("RawCap test:NG");
	}

	if (!test_result1) {
		/* test OK*/
		strcat(ts_test->test_result, "-2P");
	} else {
		strcat(ts_test->test_result, "-2F");
		GTP_ERROR("DeltaCap test:NG");
	}

	/* save rawdata to info->buff */
	if (ts_test->rawdata.size <= TS_RAWDATA_BUFF_MAX) {
		info->buff[0] = ts_test->test_params.sen_num;
		info->buff[1] = ts_test->test_params.drv_num;
		for (i = 0; i < ts_test->rawdata.size; i++)
			info->buff[i + 2] = ts_test->rawdata.data[i];
		info->used_size = ts_test->rawdata.size + 2;
	} else {
		GTP_ERROR("Invalid rawdata size[%u]",
				ts_test->rawdata.size);
	}

	return 0;
}

static int goodix_shortcircut_test(struct goodix_ts_test *ts_test);

int goodix_get_rawdata(struct ts_rawdata_info *info,
				struct ts_cmd_node *out_cmd)
{
	int ret = 0;

	if (!goodix_ts)
		return -ENODEV;

	if (!gts_test) {
		gts_test = kzalloc(sizeof(struct goodix_ts_test),
						GFP_KERNEL);
		if (!gts_test) {
			GTP_ERROR("Failed to alloc mem");
			return -ENOMEM;
		}
		gts_test->ts = goodix_ts;
	}

	ret = goodix_tptest_prepare(gts_test);
	if (ret < 0) {
		GTP_ERROR("TP test init error:%d", ret);
		strcpy(gts_test->test_result, "0F-1F-2F-3F");
		goto exit_finish;
	}

	/* i2c test and switch hw working mode */
	ret = goodix_hw_testmode(gts_test);
	if (ret < 0) {
		GTP_ERROR("Unable to set hw testmode:%d", ret);
		strcpy(gts_test->test_result, "0F-1F-2F-3F");
		goto exit_finish;
	} else {
		strcpy(gts_test->test_result, "0P");
	}

	/* include rawcap test and deltacap test */
	goodix_capacitance_test(gts_test, info);
	goodix_store_nosiedata(gts_test, info);
	goodix_shortcircut_test(gts_test);

	strcpy(info->result, gts_test->test_result);

exit_finish:
	goodix_tptest_finish(gts_test);
	kfree(gts_test);
	gts_test = NULL;
	return 0;
}

/* short test */
static int goodix_short_test_prepare(struct goodix_ts_test *ts_test)
{
	u8 data[32 + 32]; /* max sen_num and drv_num */
	int ret, i, retry = 2;
	u16 drv_offest, sen_offest;
	u8 chksum = 0x00;

	GTP_INFO("Short test prepare+");
	while (retry--) {
		/* switch to shrot test system */
		ret = ts_test->ts->ops.send_cmd(0x42, 0x00);
		if (ret < 0) {
			GTP_ERROR("Can not switch to short test system");
			return ret;
		}

		/* check firmware running */
		for (i = 0; i < 50; i++) {
			ret = ts_test->ts->ops.i2c_read(0x5095, &data[0], 1);
			if (ret < 0)
				return ret;
			else if (data[0] == 0xaa)
				break;
			msleep(20);
		}

		if (i < 50)
			break;
		else
			ts_test->ts->ops.chip_reset();
	}

	if (retry <= 0) {
		GTP_ERROR("Switch to short test mode timeout");
		return -1;
	}

	GTP_INFO("Firmware in short test mode");
	data[0] = (ts_test->test_params.short_threshold >> 8) & 0xff;
	data[1] = ts_test->test_params.short_threshold & 0xff;
	ret = ts_test->ts->ops.i2c_write(0x8808, data, 2);
	if (ret < 0)
		return ret;

	/* ADC Read Delay */
	data[0] = (150 >> 8) & 0xff;
	data[1] = 150 & 0xff;
	ret = ts_test->ts->ops.i2c_write(0x880C, data, 2);
	if (ret < 0)
		return ret;

	/* DiffCode Short Threshold */
	data[0] = (20 >> 8) & 0xff;
	data[1] = 20 & 0xff;
	ret = ts_test->ts->ops.i2c_write(0x880A,data,2);
	if (ret < 0)
		return ret;

	memset(data, 0xFF, sizeof(data));
	drv_offest = 0x80fc - 0x8050;
	sen_offest = 0x80dc - 0x8050;

	for(i = 0; i < ts_test->test_params.sen_num; i++)
		data[i] = ts_test->test_config.data[sen_offest + i];

	for(i = 0; i < ts_test->test_params.drv_num; i++)
		data[32 + i] = ts_test->test_config.data[drv_offest + i];

	for (i = 0; i < sizeof(data); i++)
		chksum += data[i];

	chksum = 0 - chksum;
	ret = ts_test->ts->ops.i2c_write(0x880E,&data[32], 32);
	if (ret < 0)
		return ret;

	ret = ts_test->ts->ops.i2c_write(0x882E, &data[0], 32);
	if (ret < 0)
		return ret;

	ret = ts_test->ts->ops.i2c_write( 0x884E, &chksum, 1);
	if (ret < 0)
		return ret;

	data[0] = 0x01;
	ret = ts_test->ts->ops.i2c_write(0x813E, data, 1);
	if (ret < 0)
		return ret;

	/* clr 5095, runing dsp */
	data[0] = 0x00;
	ret = ts_test->ts->ops.i2c_write(0x5095, data, 1);
	if (ret < 0)
		return ret;

	GTP_INFO("Short test prepare-");
	return 0;
}

/**
 * goodix_calc_resisitance - calculate resistance
 * @self_capdata: self-capacitance value
 * @adc_signal: ADC signal value
 * return: resistance value
 */
static inline long goodix_calc_resisitance(u16 self_capdata, u16 adc_signal)
{
	return (long)self_capdata * 81 * FLOAT_AMPLIFIER /
				adc_signal - (81 * FLOAT_AMPLIFIER);
}

static int goodix_check_resistance_to_gnd(struct ts_test_params *test_params,
				u16 adc_signal, u8 pos)
{
	int max_drvs = 32;
	long r;
	u16 r_th, avdd_value;

	avdd_value = test_params->avdd_value;
	if (adc_signal == 0 || adc_signal == 0x8000)
		adc_signal |= 1;

	if ((adc_signal & 0x8000) == 0)	/* short to GND */
		r = (long)(5266285) / (adc_signal & (~0x8000)) -
			40 * 100;	//52662.85/code-40
	else	/* short to VDD */
		r = (long)36864 * (avdd_value - 9) *
			100 / ((adc_signal & (~0x8000)) * 7) -
			40 * 100;

	r *= 2;
	r = r / 100;
	if(r > 65535)
		r = 65535;
	else if (r < 0) {
		r = 0;
	}

	if (pos < max_drvs)
		r_th = test_params->r_drv_gnd_threshold;
	else
		r_th = test_params->r_sen_gnd_threshold;

	if (r < r_th) {
		if ((adc_signal & (0x8000)) == 0) {
			if (pos < max_drvs)
				GTP_ERROR("Tx%d shortcircut to GND,R=%ldK,R_Threshold=%dK",
					pos, r, r_th);
			else
				GTP_ERROR("Rx%d shortcircut to GND,R=%ldK,R_Threshold=%dK",
					pos - max_drvs, r, r_th);
		} else {
			if (pos < max_drvs)
				GTP_ERROR("Tx%d shortcircut to VDD,R=%ldK,R_Threshold=%dK",
					pos, r, r_th);
			else
				GTP_ERROR("Rx%d shortcircut to VDD,R=%ldK,R_Threshold=%dK",
					pos - max_drvs, r, r_th);
		}

		return -1;
	}

	return 0;
}

static int goodix_check_short_resistance(u16 self_capdata, u16 adc_signal,
				unsigned short short_r_th)
{
	unsigned short short_val;
	long r;
	int ret = 0;

	if (self_capdata == 0xffff || self_capdata == 0) 
		return 0;

	r = goodix_calc_resisitance(self_capdata, adc_signal);
	r = r / FLOAT_AMPLIFIER;
	if (r > 65535)
		r = 65535;
	short_val = (r >= 0 ? r : 0);

	if (short_val < short_r_th) {
		GTP_ERROR("Short circut:R=%dK,R_Threshold=%dK",
					short_val, short_r_th);
		ret = -1;
	}

	return ret;
}

static int goodix_shortcircut_analysis(struct goodix_ts_test *ts_test)
{
	u8 short_flag, *data_buf = NULL, short_status[3];
	int max_drvs = 32, max_sens  = 32;
	u16 self_capdata[max_drvs + max_sens], short_pin_num;
	u16 adc_signal, data_addr;
	int ret = 0, err = 0;
	u16 r_threshold;
	int size, i, j;

	ret = ts_test->ts->ops.i2c_read(0x8801,  &short_flag, 1);
	if (ret < 0) {
		return ret;
	} else if ((short_flag & 0x0F) == 0x00) {
		GTP_INFO("No shortcircut");
		return 0;
	}

	data_buf = kzalloc(256, GFP_KERNEL);
	if (!data_buf) {
		GTP_ERROR("Failed to alloc memory");
		return -ENOMEM;
	}

	/* shortcircut to gnd */
	if ((short_flag & 0x08) == 0x08) {
		/* read diff code, diff code will be used to calculate
			resistance between channel and GND */
		size = (max_drvs + max_sens) * 2;
		ret = ts_test->ts->ops.i2c_read(0xA322,  data_buf, size);
		if (ret < 0)
			goto exit_kfree;

		for (i = 0; i < size; i += 2) {
			adc_signal = be16_to_cpup((__be16*)&data_buf[i]);
			ret = goodix_check_resistance_to_gnd(&ts_test->test_params,
						adc_signal, i / 2);
			if (ret < 0)
				err |= ret;
		}
	}

	/* read self-capdata+ */
	size = (max_drvs + max_sens) * 2;
	ret = ts_test->ts->ops.i2c_read(0xA2A0, data_buf, size);
	if (ret < 0)
		goto exit_kfree;

	for (i = 0;i < max_drvs + max_sens; i++)
		self_capdata[i] = be16_to_cpup((__be16*)&data_buf[i * 2]) & 0x7fff;
	/* read self-capdata- */

	/* read tx tx short number */
	ret = ts_test->ts->ops.i2c_read(0x8802, &short_status[0], 3);
	if (ret < 0)
		goto exit_kfree;
	GTP_INFO("Tx&Tx:%d,Rx&Rx:%d,Tx&Rx:%d",short_status[0],
					short_status[1],short_status[2]);

	/* drv&drv shortcircut check */
	data_addr = 0x8800 + 0x60;
	for (i = 0; i < short_status[0]; i++) {
		size = 4 + max_drvs * 2 + 2;
		ret = ts_test->ts->ops.i2c_read(data_addr, data_buf, size);
		if (ret < 0)
			goto exit_kfree;

		r_threshold = ts_test->test_params.r_drv_drv_threshold;
		short_pin_num = be16_to_cpup((__be16 *)&data_buf[0]);
		if (short_pin_num > max_drvs + max_sens)
			continue;

		for (j = i + 1; j < max_drvs; j++) {
			adc_signal = be16_to_cpup((__be16 *)&data_buf[4 + j * 2]);
			if (adc_signal > ts_test->test_params.short_threshold) {
				ret = goodix_check_short_resistance(
						self_capdata[short_pin_num], adc_signal,
						r_threshold);
				if (ret < 0) {
					err |= ret;
					GTP_ERROR("Tx%d-Tx%d shortcircut", short_pin_num, j);
				}
			}
		}
		data_addr += size;
	}

	/* sen&sen shortcircut check */
	data_addr = 0x9120;
	for (i = 0; i < short_status[1]; i++) {
		size = 4 + max_sens * 2 + 2;
		ret = ts_test->ts->ops.i2c_read(data_addr, data_buf, size);
		if (ret < 0)
			goto exit_kfree;

		r_threshold = ts_test->test_params.r_sen_sen_threshold;
		short_pin_num = be16_to_cpup((__be16 *)&data_buf[0]) + max_drvs;
		if (short_pin_num > max_drvs + max_sens)
			continue;

		for (j = 0; j < max_sens; j++) {
			if(j == i || (j < i && (j & 0x01) == 0))
				continue;
			adc_signal = be16_to_cpup((__be16 *)&data_buf[4 + j * 2]);
			if (adc_signal > ts_test->test_params.short_threshold) {
				ret = goodix_check_short_resistance(
						self_capdata[short_pin_num], adc_signal,
						r_threshold);
				if (ret < 0) {
					err |= ret;
					GTP_ERROR("Rx%d-Rx%d shortcircut",
							short_pin_num - max_drvs, j);
				}
			}
		}
		data_addr += size;
	}

	/* sen&drv shortcircut check */
	data_addr = 0x99e0;
	for (i = 0; i < short_status[2]; i++) {
		size = 4 + max_sens * 2 + 2;
		ret = ts_test->ts->ops.i2c_read(data_addr, data_buf, size);
		if (ret < 0)
			goto exit_kfree;

		r_threshold = ts_test->test_params.r_drv_sen_threshold;
		short_pin_num = be16_to_cpup((__be16 *)&data_buf[0]) + max_drvs;
		if (short_pin_num > max_drvs + max_sens)
			continue;

		for (j = 0; j < max_drvs; j++) {
			adc_signal = be16_to_cpup((__be16 *)&data_buf[4 + j * 2]);
			if (adc_signal > ts_test->test_params.short_threshold) {
				ret = goodix_check_short_resistance(
						self_capdata[short_pin_num], adc_signal,
						r_threshold);
				if (ret < 0) {
					err |= ret;
					GTP_ERROR("Rx%d-Tx%d shortcircut",
							short_pin_num - max_drvs, j);
				}
			}
		}
		data_addr += size;
	}

exit_kfree:
	kfree(data_buf);
	return err | ret ? -EFAULT : 0;
}

static int goodix_shortcircut_test(struct goodix_ts_test *ts_test)
{
	u16 reg_sta = 0x8800;
	u8 data[2];
	int ret, retry = 150;

	ret = goodix_short_test_prepare(ts_test);
	if (ret < 0)
		goto end_test;

	while (retry--) {
		ret = ts_test->ts->ops.i2c_read(reg_sta, data, 1);
		if (ret < 0)
			goto end_test;
		else if (data[0] == 0x88)
			break;

		msleep(50);
		GTP_DEBUG("waitting...:%d", retry);
	}

	if (retry <= 0) {
		GTP_ERROR("Wait short test finish timeout");
		ret = -1;
		goto end_test;
	}

	ret = goodix_shortcircut_analysis(ts_test);
	if (ret < 0) {
		GTP_ERROR("Shortcircut test failed");
		goto end_test;
	}

end_test:
	if (ret < 0)
		strcat(ts_test->test_result, "-3F");
	else
		strcat(ts_test->test_result, "-3P");

	return ret;
}

