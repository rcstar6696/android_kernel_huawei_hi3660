#ifndef __LED_CONFIG_H__
#define __LED_CONFIG_H__

#define LED_CONFIG    "led_config"

#define GET_U8_FROM_NODE(dn, name, temp_val, received_val, flag) \
	if (of_property_read_u32(dn, name, &temp_val)) \
	{ \
		hwlog_err("%s:read %s fail, using default!!\n", __func__, name); \
		received_val = 0; \
		flag = 0; \
	} else { \
		received_val = (uint8_t)temp_val; \
		hwlog_debug("%s:read %s suss, value %d!!\n", __func__, name, received_val); \
		flag = 1; \
	}

struct led_current_config {
	uint8_t tp_color_id;
	uint8_t red_curr;
	uint8_t green_curr;
	uint8_t blue_curr;
	uint8_t got_flag;
};

extern void led_config_get_current_setting(struct hisi_led_platform_data* hisi_leds);
#endif
