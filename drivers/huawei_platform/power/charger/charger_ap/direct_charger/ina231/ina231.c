/*
 * Driver for ina230 or ina231 power monitor chips
 *
 * Copyright (c) 2013- Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/power/direct_charger.h>

struct ina231_data *g_idata;
/* common register definitions */
#define INA231_CONFIG			0x00
#define INA231_SHUNT_VOLTAGE	0x01 /* readonly */
#define INA231_BUS_VOLTAGE	0x02 /* readonly */
#define INA231_POWER			0x03 /* readonly */
#define INA231_CURRENT			0x04 /* readonly */
#define INA231_CALIBRATION		0x05
#define INA231_MASK_ENABLE	0x06
#define INA231_ALERT_LIMIT		0x07

/* register count */
#define INA231_MAX_REGS		8

#define HWLOG_TAG ina231_for_charge
HWLOG_REGIST();
struct ina231_config {
	u16 config_sleep_in;
	u16 config_reset;
	u16 config_work;
	u16 calibrate_content;
	u16 mask_enable_content;
	u16 alert_limit_content;

	int bus_voltage_lsb;	/* uV */

	int current_lsb;	/* uA */
};

struct ina231_data {
	struct device *dev;
	struct i2c_client *client;
	const struct ina231_config *config;
	struct regulator *vs_regu;
	int vs_on;
	int need_to_power_vs;
};

static  struct ina231_config ina231_config = {
	.config_sleep_in = 0x4124, /*config ina231 to powerdown mode, number of averages:1, Vbus conversion time is 1.1ms, Vsh conversion time is 1.1ms*/
	.config_reset = 0x8000, /*reset the whole chip*/
	.config_work = 0x4377, /*config ina231 to shunt and bus continous mode, nuber of averages:4, Vbus conversion time is 2.116ms, Vsh conversion time is 4.156ms*/
	.calibrate_content = 0x1000, /*current_LSB: 0.5ma, */
	.mask_enable_content = 0x0,
	.alert_limit_content = 0x0,

	.bus_voltage_lsb = 1250, /* 1250uV/bit */
	.current_lsb = 500, /* 500uA/bit */
};
static int enable_vs_supply(void)
{
	struct ina231_data *idata = g_idata;
	int ret;
	if (idata->vs_regu)
	{
		ret = regulator_enable(idata->vs_regu);
		if (ret)
		{
			hwlog_err("[%s]fail!\n", __func__);
			return -1;
		}
	}
	hwlog_info("[%s]succ!\n", __func__);
	return 0;
}

static int ina231_init(void)
{
	int ret = 0;
	struct ina231_data *idata = g_idata;
	struct i2c_client *client = NULL;

	if (!idata->vs_on)
	{
		hwlog_err("[%s]vs_on is 0, fail!\n", __func__);
		return -1;
	}
	client = idata->client;
	ret = i2c_smbus_write_word_swapped(client, INA231_CONFIG, idata->config->config_work);
	ret |= i2c_smbus_write_word_swapped(client, INA231_CALIBRATION, idata->config->calibrate_content);
	ret |= i2c_smbus_write_word_swapped(client, INA231_MASK_ENABLE, idata->config->mask_enable_content);
	ret |= i2c_smbus_write_word_swapped(client, INA231_ALERT_LIMIT, idata->config->alert_limit_content);
	if (ret)
	{
		hwlog_err("[%s]fail!\n", __func__);
		return -1;
	}
	msleep(100);

	return 0;
}

static int ina231_exit(void)
{
	return 0;
}
static int ina231_get_bus_voltage_mv(void)
{
	s16 reg;
	int ret = 0;
	int bus_voltage_lsb; /* 0mV/bit */
	struct ina231_data *idata = g_idata;
	struct i2c_client *client = NULL;

	client = idata->client;
	bus_voltage_lsb = idata->config->bus_voltage_lsb;
	ret = i2c_smbus_read_word_swapped(client, INA231_BUS_VOLTAGE);
	if (ret < 0)
	{
		hwlog_err("[%s]fail, ret = 0x%x!\n", __func__, ret);
		return -1;
	}
	reg = (s16) ret;

	return (int) (reg* bus_voltage_lsb)/1000;
}

static int ina231_get_current_ma(int* val)
{
	s16 reg;
	int ret = 0;
	int current_lsb; /* 0mV/bit */
	struct ina231_data *idata = g_idata;
	struct i2c_client *client = NULL;

	client = idata->client;
	current_lsb = idata->config->current_lsb;
	ret= i2c_smbus_read_word_swapped(client, INA231_CURRENT);
	if (ret < 0)
	{
		hwlog_err("[%s]fail, reg = 0x%x!\n", __func__, ret);
		return -1;
	}

	reg = (s16) ret;
	*val = (int) (reg* current_lsb)/1000;
	return 0;
}
static int ina231_get_bat_temp(void)
{
	return 25;
}

struct batinfo_ops ina231_ops = {
	.init = ina231_init,
	.exit = ina231_exit,
	.get_bat_btb_voltage = ina231_get_bus_voltage_mv,
	.get_bat_package_voltage = ina231_get_bus_voltage_mv,
	.get_bat_current = ina231_get_current_ma,
	.get_bat_temperature = ina231_get_bat_temp,
};

static int ina231_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = client->adapter;
	struct ina231_data *idata;
	struct device_node *np = NULL;
	int ret = -ENODEV;
	int ibat = 0;
	int i;
	unsigned int calibrate_content;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_WORD_DATA))
		return -ENODEV;

	idata = devm_kzalloc(&client->dev, sizeof(*idata), GFP_KERNEL);
	if (!idata)
		return -ENOMEM;

	g_idata = idata;
	idata->config = &ina231_config;
	idata->client = client;
	idata->dev = &client->dev;

	np = idata->dev->of_node;
	ret = of_property_read_u32(np, "calibrate_content", &calibrate_content);
	if (ret)
	{
		hwlog_err("get calibrate_content failed\n");
		goto fail0;
	}
	ina231_config.calibrate_content = (u16)calibrate_content;
	hwlog_info("calibrate_content = 0x%x\n", ina231_config.calibrate_content);
	ret = of_property_read_u32(np, "need_to_power_vs", &(idata->need_to_power_vs));
	if (ret)
	{
		hwlog_err("get need_to_power_vs failed\n");
		goto fail0;
	}
	hwlog_info("need_to_power_vs = %d\n", idata->need_to_power_vs);
	if (idata->need_to_power_vs)
	{
		idata->vs_regu = devm_regulator_get(&client->dev, "vs");
		if (IS_ERR(idata->vs_regu))
		{
			hwlog_err("[%s]can not get vs-supply!\n", __func__);
			ret = -ENODEV;
			goto fail0;
		}
		if (enable_vs_supply())
		{
			ret = -ENODEV;
			goto fail0;
		}
	}
	idata->vs_on = 1;

	/* communication check and reset device*/
	ret = i2c_smbus_write_word_swapped(client, INA231_CONFIG, idata->config->config_reset);
	if (ret < 0)
	{
		hwlog_err("%s reset failed\n", client->name);
		goto fail0;
	}
	mdelay(20);

	/* goto sleep */
	i2c_smbus_write_word_swapped(client, INA231_CONFIG, idata->config->config_sleep_in);

	i2c_set_clientdata(client, idata);
	ret = batinfo_ops_register(&ina231_ops);
	if (ret)
	{
		hwlog_err("register ina231 ops failed!\n");
		goto fail0;
	}

	ina231_init();
	for (i = 0; i < 8; ++i)
	{
		hwlog_info("reg[%d] = 0x%x\n", i, i2c_smbus_read_word_swapped(client, i));
	}
	ret = ina231_get_current_ma(&ibat);
	if (ret)
	{
		hwlog_err("get current failed!\n");
	}
	hwlog_info("bat_vol = %d, bat_cur = %d\n", ina231_get_bus_voltage_mv(), ibat);
	hwlog_info("name:%s(address:0x%x) probe successfully\n", client->name, client->addr);
	return 0;
fail0:
	kfree(idata);
	idata = NULL;
	return ret;
}

static int ina231_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id ina231_id[] = {
	{"ina231_for_charge", 0},
	{ },
};
MODULE_DEVICE_TABLE(i2c, ina231_id);

static struct of_device_id ina231_match_table[] = {
	{
		.compatible = "huawei,ina231_for_charge",
		.data = NULL,
	},
	{ },
};

MODULE_DEVICE_TABLE(of, ina231_of_match);

static struct i2c_driver ina231_driver = {
	.probe		= ina231_probe,
	.remove		= ina231_remove,
	.shutdown = NULL,
	.driver = {
		.name = "huawei_ina231_for_charge",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ina231_match_table),
	},
	.id_table = ina231_id,
};

static int __init ina231_module_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&ina231_driver);
	if (ret)
		hwlog_err("Unable to register ina231 driver\n");

	return ret;
}

module_init(ina231_module_init);

MODULE_DESCRIPTION("ina231 driver");
MODULE_LICENSE("GPL");
