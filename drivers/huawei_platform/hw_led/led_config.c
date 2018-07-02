#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/hisi/hisi_leds.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/touthscreen/huawei_tp_color.h>
#include "led_config.h"

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif
#define HWLOG_TAG  led_config
HWLOG_REGIST();

extern int tp_color_provider(void);

static uint8_t obtain_tp_color_retry_times = 5; //retry 5 times
static uint8_t led_current_using_tp_color_setting = 0;
static uint8_t led_got_tp_color_current_flag = 0;

struct led_current_config leds_current_setting[TP_COLOR_MAX] = {
	[TP_COLOR_WHITE]    = {.tp_color_id = WHITE, .got_flag = 0,},
	[TP_COLOR_BLACK]    = {.tp_color_id = BLACK, .got_flag = 0,},
	[TP_COLOR_BLACK2]   = {.tp_color_id = BLACK2, .got_flag = 0,},
	[TP_COLOR_PINK]     = {.tp_color_id = PINK, .got_flag = 0,},
	[TP_COLOR_RED]      = {.tp_color_id = RED, .got_flag = 0,},
	[TP_COLOR_YELLOW]   = {.tp_color_id = YELLOW, .got_flag = 0,},
	[TP_COLOR_BLUE]     = {.tp_color_id = BLUE, .got_flag = 0,},
	[TP_COLOR_GOLD]     = {.tp_color_id = GOLD, .got_flag = 0,},
	[TP_COLOR_PINKGOLD] = {.tp_color_id = PINKGOLD, .got_flag = 0,},
	[TP_COLOR_SILVER]   = {.tp_color_id = SILVER, .got_flag = 0,},
	[TP_COLOR_GRAY]     = {.tp_color_id = GRAY, .got_flag = 0,},
	[TP_COLOR_CAFE]     = {.tp_color_id = CAFE, .got_flag = 0,},
	[TP_COLOR_CAFE2]    = {.tp_color_id = CAFE2, .got_flag = 0,},
	[TP_COLOR_GREEN]    = {.tp_color_id = GREEN, .got_flag = 0,},
};

void led_config_get_current_setting(struct hisi_led_platform_data* hisi_leds)
{
	uint32_t i = 0;
	int tp_color = 0;

	if (NULL == hisi_leds) {
		hwlog_err("%s null pointer error!\n", __func__);
		return;
	}

	if (!led_current_using_tp_color_setting) {
		hwlog_err("%s do not use the feature, return!\n", __func__);
		return;
	}
	if (!!led_got_tp_color_current_flag) {
		hwlog_debug("%s has alread set!\n", __func__);
		return;
	}

	tp_color = tp_color_provider();
	if (-1 == tp_color) { // fail return -1
		if (obtain_tp_color_retry_times > 0) {
			obtain_tp_color_retry_times--;
		} else {
			hwlog_err("%s can not obtain tp_color, do not using the feature!\n", __func__);
			led_current_using_tp_color_setting = 0;
		}
		hwlog_err("%s got tp_color fail!\n", __func__);
		return;
	}
	hwlog_info("%s tp_color:0x%x\n", __func__, tp_color);

	for (i = 0; i < TP_COLOR_MAX; i++) {
		if ((leds_current_setting[i].tp_color_id == tp_color) && !!leds_current_setting[i].got_flag) {
			hwlog_info("%s tp_color_id 0x%x got config current setting\n", __func__, tp_color);
			hisi_leds->leds[0].each_maxdr_iset = leds_current_setting[i].red_curr;
			hisi_leds->leds[1].each_maxdr_iset = leds_current_setting[i].green_curr;
			hisi_leds->leds[2].each_maxdr_iset = leds_current_setting[i].blue_curr;
			led_got_tp_color_current_flag = 1;
			break;
		}
	}
}
EXPORT_SYMBOL(led_config_get_current_setting);

static uint8_t led_config_parse_single_current_conf(struct device_node *led_node,
		uint8_t tp_color_id, const char* led_name, uint8_t* ret_val)
{
	uint8_t flag = 0;
	uint8_t tmp_ret_val = 0;
	char tmp_name[128] = {0}; //the max length of str
	char* color_name = NULL;
	uint32_t tmp = 0;

	if (NULL == led_node || NULL == led_name || NULL == ret_val) {
		hwlog_err("%s null pointer error!\n", __func__);
		return flag;
	}

	switch (tp_color_id) {
		case WHITE:
			color_name = "white";
			break;
		case BLACK:
			color_name = "black";
			break;
		case BLACK2:
			color_name = "black2";
			break;
		case PINK:
			color_name = "pink";
			break;
		case RED:
			color_name = "red";
			break;
		case YELLOW:
			color_name = "yellow";
			break;
		case BLUE:
			color_name = "blue";
			break;
		case GOLD:
			color_name = "gold";
			break;
		case PINKGOLD:
			color_name = "pinkgold";
			break;
		case SILVER:
			color_name = "silver";
			break;
		case GRAY:
			color_name = "gray";
			break;
		case CAFE:
			color_name = "cafe";
			break;
		case CAFE2:
			color_name = "cafe2";
			break;
		case GREEN:
			color_name = "green";
			break;
		defult:
			hwlog_err("%s tp_color_id error!\n", __func__);
			return flag;
	}
	if (sizeof(tmp_name) <= (strlen(color_name) + strlen(led_name) + strlen("_tp_"))) {
		hwlog_err("%s tmp_name buffer overflow error!\n", __func__);
		return flag;
	}
	sprintf(tmp_name, "%s_tp_%s", color_name, led_name);
	hwlog_debug("%s %s!\n", __func__, tmp_name);
	GET_U8_FROM_NODE(led_node, tmp_name, tmp, tmp_ret_val, flag);
	*ret_val = tmp_ret_val;
	return flag;
}

static void led_config_parse_current_setting(struct device_node *led_node)
{
	uint8_t flag = 1;
	uint8_t tmp = 0;
	uint32_t i = 0;
	uint32_t tmp_u32 = 0;

	if (NULL == led_node) {
		hwlog_err("%s null pointer error!\n", __func__);
		return;
	}

	GET_U8_FROM_NODE(led_node, "led_current_using_tp_color_setting", tmp_u32, led_current_using_tp_color_setting, flag);
	if (!led_current_using_tp_color_setting) {
		hwlog_info("%s do not using tp_color current feature!\n", __func__);
		return;
	}

	for (i = 0; i < TP_COLOR_MAX; i++) {
		flag = 1;
		tmp = led_config_parse_single_current_conf(led_node, leds_current_setting[i].tp_color_id,
				"led_red_maxdr_iset", &leds_current_setting[i].red_curr);
		flag = flag && tmp;
		tmp = led_config_parse_single_current_conf(led_node, leds_current_setting[i].tp_color_id,
				"led_green_maxdr_iset", &leds_current_setting[i].green_curr);
		flag = flag && tmp;
		tmp = led_config_parse_single_current_conf(led_node, leds_current_setting[i].tp_color_id,
				"led_blue_maxdr_iset", &leds_current_setting[i].blue_curr);
		flag = flag && tmp;

		leds_current_setting[i].got_flag = flag;
		hwlog_info("%s tp_color_id 0x%x, got_flag %d\n", __func__, leds_current_setting[i].tp_color_id, leds_current_setting[i].got_flag);
	}
}

static const struct of_device_id led_config_match_table[] = {
	{.compatible = "huawei,led_config",},
	{},
};
MODULE_DEVICE_TABLE(of, led_config_match_table);

static int led_config_probe(struct platform_device *pdev)
{
	struct device_node *led_node = NULL;

	if (NULL == pdev) {
		hwlog_err("%s null pointer error!\n", __func__);
		return -ENOMEM;
	}

	led_node = pdev->dev.of_node;
	if (!led_node) {
		hwlog_err("%s failed to find dts node led_alwayson\n", __func__);
		return -ENODEV;;
	}

	led_config_parse_current_setting(led_node);

	hwlog_info("%s succ.\n", __func__);
	return 0;
}

static int led_config_remove(struct platform_device *pdev)
{
	hwlog_info("%s\n", __func__);
	return 0;
}

struct platform_driver led_config_driver = {
	.probe = led_config_probe,
	.remove = led_config_remove,
	.driver = {
		.name = LED_CONFIG,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(led_config_match_table),
	},
};

static int __init led_config_init(void)
{
	hwlog_info("init!\n");
	return platform_driver_register(&led_config_driver);
}

static void __exit led_config_exit(void)
{
	platform_driver_unregister(&led_config_driver);
}

module_init(led_config_init);
module_exit(led_config_exit);

MODULE_AUTHOR("HUAWEI");
MODULE_DESCRIPTION("Led config driver");
MODULE_LICENSE("GPL");
