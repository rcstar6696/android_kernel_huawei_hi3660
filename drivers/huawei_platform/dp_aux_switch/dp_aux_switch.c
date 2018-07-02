/* Copyright (c) 2008-2019, Huawei Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/wakelock.h>
#include <linux/ion.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <huawei_platform/dp_aux_switch/dp_aux_switch.h>

static uint32_t g_dp_aux_gpio;
static uint32_t g_dp_aux_uart_gpio;

static struct regulator *g_dp_aux_ldo_supply = NULL;
static unsigned int g_dp_aux_ldo_status = 0;
static DEFINE_MUTEX(g_dp_aux_ldo_op_mutex);

#define SET_GPIO_HIGH 1
#define SET_GPIO_LOW  0
#define DTS_DP_AUX_SWITCH "huawei,dp_aux_switch"

void dp_aux_switch_op(uint32_t value)
{
	if (gpio_is_valid(g_dp_aux_gpio)) {
	        gpio_direction_output(g_dp_aux_gpio, value);
	} else {
	        printk(KERN_ERR "%s, Gpio is invalid:%d.\n", __func__, g_dp_aux_gpio);
	}
}

void dp_aux_uart_switch_enable(void)
{
	if (gpio_is_valid(g_dp_aux_uart_gpio)) {
	        gpio_direction_output(g_dp_aux_uart_gpio, SET_GPIO_HIGH);
	} else {
	        printk(KERN_ERR "%s, Gpio is invalid:%d.\n", __func__, g_dp_aux_uart_gpio);
	}
}

void dp_aux_uart_switch_disable(void)
{
	if (gpio_is_valid(g_dp_aux_uart_gpio)) {
	        gpio_direction_output(g_dp_aux_uart_gpio, SET_GPIO_LOW);
	} else {
	        printk(KERN_ERR "%s, Gpio is invalid:%d.\n", __func__, g_dp_aux_uart_gpio);
	}
}

int dp_aux_ldo_supply_enable(dp_aux_ldo_ctrl_type_t type)
{
	int ret = 0;

	printk(KERN_INFO "%s: count(%d), type(%d)\n", __func__, g_dp_aux_ldo_status, type);
	if (g_dp_aux_ldo_supply == NULL) {
		printk(KERN_ERR "%s: g_dp_aux_power_supply is NULL!\n", __func__);
		return -ENODEV;
	}

	if ((type < DP_AUX_LDO_CTRL_BEGIN) || (type >= DP_AUX_LDO_CTRL_MAX)) {
		printk(KERN_ERR "%s: type(%d) is invalid!\n", __func__, type);
		return -EINVAL;
	}

	mutex_lock(&g_dp_aux_ldo_op_mutex);
	if (g_dp_aux_ldo_status == 0) {
		ret = regulator_enable(g_dp_aux_ldo_supply);
		if (ret) {
			printk(KERN_ERR "%s: regulator enable failed(%d)!\n", __func__, ret);
			mutex_unlock(&g_dp_aux_ldo_op_mutex);
			return -EPERM;
		}

		g_dp_aux_ldo_status = 1 << type;
	} else {
		g_dp_aux_ldo_status =  g_dp_aux_ldo_status | (1 << type);
	}
	mutex_unlock(&g_dp_aux_ldo_op_mutex);

	printk(KERN_INFO "%s: regulator enable(%d) success!\n", __func__, type);
	return 0;
}
EXPORT_SYMBOL_GPL(dp_aux_ldo_supply_enable);

int dp_aux_ldo_supply_disable(dp_aux_ldo_ctrl_type_t type)
{
	int ret = 0;

	printk(KERN_INFO "%s: count(%d), type(%d)\n", __func__, g_dp_aux_ldo_status, type);
	if (g_dp_aux_ldo_supply == NULL) {
		printk(KERN_ERR "%s: g_dp_aux_power_supply is NULL!\n", __func__);
		return -ENODEV;
	}

	if ((type < DP_AUX_LDO_CTRL_BEGIN) || (type >= DP_AUX_LDO_CTRL_MAX)) {
		printk(KERN_ERR "%s: type(%d) is invalid!\n", __func__, type);
		return -EINVAL;
	}

	mutex_lock(&g_dp_aux_ldo_op_mutex);
	g_dp_aux_ldo_status = g_dp_aux_ldo_status & (~(1 << type));
	if (g_dp_aux_ldo_status == 0) {
		ret = regulator_disable(g_dp_aux_ldo_supply);
		if (ret) {
			printk(KERN_ERR "%s: regulator disable failed(%d)!\n", __func__, ret);
			mutex_unlock(&g_dp_aux_ldo_op_mutex);
			return -EPERM;
		}
	}
	mutex_unlock(&g_dp_aux_ldo_op_mutex);

	printk(KERN_INFO "%s: regulator disable(%d) success!\n", __func__, type);
	return 0;

}
EXPORT_SYMBOL_GPL(dp_aux_ldo_supply_disable);

static int dp_aux_ldo_supply_dts(struct device *dev)
{
	struct regulator *supply = NULL;

	if (dev == NULL) {
		printk(KERN_ERR "%s: dev is NULL!\n", __func__);
		return -1;
	}

	supply = regulator_get(dev, "auxldo");
	if (IS_ERR(supply)) {
		printk(KERN_ERR "%s: get regulator failed!\n", __func__);
		return -1;
	}

	int ret = regulator_get_voltage(supply);
	printk(KERN_INFO "%s: auxldo regulator_get_voltage=%d!\n", __func__, ret);

	g_dp_aux_ldo_supply = supply;
	printk(KERN_INFO "%s: get regulator from dts success.\n", __func__);
	return 0;
}

static int dp_aux_switch_probe(struct platform_device *pdev)
{
	int ret = dp_aux_ldo_supply_dts(&pdev->dev);
	if (ret < 0) {
		printk(KERN_ERR "%s: get aux ldo supply failed!\n", __func__);
	}

	return 0;
}

static int dp_aux_switch_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id dp_aux_switch_match[] = {
	{ .compatible = "huawei,dp_aux_switch", },
	{},
};

static struct platform_driver dp_aux_switch_driver = {
	.driver = {
		.name  = "dp_aux_switch",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(dp_aux_switch_match),
	},
	.probe  = dp_aux_switch_probe,
	.remove = dp_aux_switch_remove,
};

static int __init dp_aux_switch_init(void)
{
	int ret = 0;
	struct device_node *np;
	printk(KERN_INFO "%s: enter\n", __func__);

	ret = platform_driver_register(&dp_aux_switch_driver);
	if (ret < 0) {
		printk("%s: register dp_aux_switch_driver failed!\n", __func__);
		goto err_return;
	}

	np = of_find_compatible_node(NULL, NULL, DTS_DP_AUX_SWITCH);
	if (!np) {
		printk(KERN_ERR "NOT FOUND device node %s!\n", DTS_DP_AUX_SWITCH);
		goto err_return;
	}

	g_dp_aux_gpio = of_get_named_gpio(np, "cs-gpios", 0);
	g_dp_aux_uart_gpio = of_get_named_gpio(np, "cs-gpios", 1);

	if (!gpio_is_valid(g_dp_aux_gpio)) {
		printk(KERN_ERR "%s, Gpio is invalid:%d.\n", __func__, g_dp_aux_gpio);
		return 0;
	} else {
		/*request aux gpio*/
		ret = gpio_request(g_dp_aux_gpio, "dp_aux_gpio");
		if (ret < 0) {
			printk(KERN_ERR "%s, Fail to request gpio:%d. ret = %d\n", __func__, g_dp_aux_gpio, ret);
			goto err_return;
		}
		/*set aux gpio output low*/
		gpio_direction_output(g_dp_aux_gpio, SET_GPIO_LOW);
	}

	if (!gpio_is_valid(g_dp_aux_uart_gpio)) {
		printk(KERN_ERR "%s, Gpio is invalid:%d.\n", __func__, g_dp_aux_uart_gpio);
		return 0;
	} else {
		/*request aux uart gpio*/
		ret = gpio_request(g_dp_aux_uart_gpio, "dp_aux_uart_gpio");
		if (ret < 0) {
			printk(KERN_ERR "%s, Fail to request gpio:%d. ret = %d\n", __func__, g_dp_aux_uart_gpio, ret);
			goto err_return;
		}
		gpio_direction_output(g_dp_aux_uart_gpio, SET_GPIO_LOW);
	}

	/*set aux uart gpio output low*/
	printk(KERN_INFO "%s: sucess\n", __func__, ret);

err_return:
	return ret;
}

module_init(dp_aux_switch_init);
