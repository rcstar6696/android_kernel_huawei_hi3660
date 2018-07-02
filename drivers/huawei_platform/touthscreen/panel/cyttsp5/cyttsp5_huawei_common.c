/*
 * cyttsp5_power_ctrl.c
 * Cypress TrueTouch(TM) Standard Product V5 Platform Module.
 * For use with Cypress Txx5xx parts.
 * Supported parts include:
 * TMA5XX
 *
 * Copyright (C) 2013-2014 Cypress Semiconductor
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, and only version 2, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Contact Cypress Semiconductor at www.cypress.com <ttdrivers@cypress.com>
 *
 */

#include "cyttsp5_regs.h"

u8 g_ts_log_level;

struct kobject *virtual_key_kobject_ts;

#define CY_MAX_BUF_SIZE 128

enum cyttsp5_holster_command {
	SET_HOLSTER_ON = 0,
	SET_HOLSTER_OFF,
	GET_HOLSTER_MODE,
};

static char *holster_command[] = {
	"0x04 0x00 0x08 0x00 0x2f 0x00 0x06 0x25 0x01 0x01",	/*holster on*/
	"0x04 0x00 0x08 0x00 0x2f 0x00 0x06 0x25 0x01 0x00",	/*holster off*/
	"0x04 0x00 0x06 0x00 0x2f 0x00 0x05 0x25"	/*read holster mode*/
};

enum cyttsp5_holster_command_status {
	SET_HOLSTER_SUCCESSFUL_ONE = 0,
	SET_HOLSTER_SUCCESSFUL_TWO,
	GET_HOLSTER_ON_RESPONSE_ONE,
	GET_HOLSTER_ON_RESPONSE_TWO,
	GET_HOLSTER_OFF_RESPONSE_ONE,
	GET_HOLSTER_OFF_RESPONSE_TWO,
	GET_HOLSTER_DEFAULT_RESPONSE_ONE,
	GET_HOLSTER_DEFAULT_RESPONSE_TWO,
};

static char *holster_response_state[] = {
	"07 00 1F 00 06 25 01",	/*set holster on/off success*/
	"07 00 1F 00 86 25 01",	/*set holster on/off success*/
	"08 00 1F 00 05 25 01 01",	/*holster mode on response*/
	"08 00 1F 00 85 25 01 01",	/*holster mode on response*/
	"08 00 1F 00 05 25 01 02",	/*holster mode off response*/
	"08 00 1F 00 85 25 01 02",	/*holster mode off response*/
	"08 00 1F 00 05 25 01 00",	/*holster mode default status response*/
	"08 00 1F 00 85 25 01 00"	/*holster mode default status response*/
};

enum cyttsp5_window_command_status {
	SET_WINDOW_CMD_LENGTH = 0,
	SET_WINDOW_X_START,
	SET_WINDOW_X_END,
	SET_WINDOW_Y_START,
	SET_WINDOW_Y_END,
};

static char *window_zone_command[] = {
	"04 00 09 00 2f 00 06 26 02 00 00",	/* full command temple */
	"04 00 09 00 2f 00 06 26 02",	/* x start */
	"04 00 09 00 2f 00 06 27 02",	/* x end */
	"04 00 09 00 2f 00 06 28 02",	/* y start */
	"04 00 09 00 2f 00 06 29 02",	/* y end */
};

struct cyttsp5_mt_data *g_mt_data;

#define COVER_RESPONSE_MAX_LEN	30
#define COVERT_STRING_MAX_LEN	10

/*
 * 0 : holster mode is off.
   1 : holster mode is on.
  -1 : get holster mode is error.
*/
int cyttsp5_get_holster_mode(struct device *dev)
{
	int rc;
	int i = 0;
	int index = 0;
	int num_read = 0;
	int length = 0;
	u8 response_buf[CY_MAX_BUF_SIZE] = { 0 };
	char response_str[COVER_RESPONSE_MAX_LEN] = { 0 };

	TS_LOG_DEBUG("Enter %s.\n", __func__);

	memset(response_buf, 0, sizeof(response_buf));

	rc = cyttsp5_send_command(dev, holster_command[GET_HOLSTER_MODE]);
	if (rc || !cyttsp5_command_status(dev)) {
		TS_LOG_ERR("%s: Send read TP holster mode command failed.\n",
			   __func__);
		return -1;
	}

	msleep(10);

	num_read = cyttsp5_command_response(dev, response_buf);
	if (!cyttsp5_command_status(dev) || num_read < 0) {
		TS_LOG_ERR("%s: Get TP holster mode command response failed.\n",
			   __func__);
		return -1;
	}

	for (i = 0; i < num_read; i++) {
		TS_LOG_DEBUG("%s,response_buf[%d]=0x%02X.\n", __func__, i,
			     response_buf[i]);
		index +=
		    scnprintf(response_str + index, COVER_RESPONSE_MAX_LEN,
			      "%02X ", response_buf[i]);
	}

	TS_LOG_DEBUG("%s,response_str=%s.\n", __func__, response_str);

	/*response string len.*/
	length = strlen(holster_response_state[GET_HOLSTER_ON_RESPONSE_ONE]);
	TS_LOG_DEBUG("%s,length = %d.\n", __func__, length);

	/*holster mode on.*/
	for (i = GET_HOLSTER_ON_RESPONSE_ONE; i <= GET_HOLSTER_ON_RESPONSE_TWO;
	     i++) {
		if (strncmp(holster_response_state[i], response_str, length) ==
		    0) {
			return 1;
		}
	}

	/*holster mode off or default status.*/
	for (i = GET_HOLSTER_OFF_RESPONSE_ONE;
	     i <= GET_HOLSTER_DEFAULT_RESPONSE_TWO; i++) {
		if (strncmp(holster_response_state[i], response_str, length) ==
		    0) {
			return 0;
		}
	}

	TS_LOG_ERR("%s: get touch holster mode failed.\n", __func__);
	return -1;
}

static int convert_format(int num, char *str)
{
	char buf[COVERT_STRING_MAX_LEN] = {0};
	int ret = 0, len = 0, tmp = num;

	if(NULL == str){
		TS_LOG_ERR("%s : str is null .\n", __func__);
		return -1;
	}

	while(tmp > 0){
		ret = snprintf(buf, COVERT_STRING_MAX_LEN, "%02x", tmp & 0xff);
		if(ret < 0){
			TS_LOG_ERR("%s : format convert error!\n", __func__);
			return -1;
		}

		strncat(str, " ", strlen(" "));
		strncat(str + strlen(str), buf, strlen(buf));

		memset(buf, 0 , sizeof(buf));

		tmp = tmp >> 8;
	}

	TS_LOG_DEBUG("%s : Convert_str = %s\n", __func__, str);

	return 0;
}

static int set_effective_window(struct device *dev)
{
	char *cmd_buf = NULL;
	int i = 0, j =0;
	int rc = -1;
	struct cyttsp5_core_data *cd = dev_get_drvdata(dev);
	char conv_str[COVERT_STRING_MAX_LEN] = {0};
	int window_edge[4];

	cmd_buf = kzalloc(strlen(window_zone_command[SET_WINDOW_CMD_LENGTH]) + 1,
		GFP_KERNEL);
	if(NULL == cmd_buf){
		TS_LOG_ERR("%s : kzalloc cmd buf fail .\n", __func__);
		goto free_buf;
	}

	window_edge[0] = cd->window_info.x0;
	window_edge[1] = cd->window_info.x1;
	window_edge[2] = cd->window_info.y0;
	window_edge[3] = cd->window_info.y1;

	for(i = SET_WINDOW_X_START; i <= SET_WINDOW_Y_END ; i++){
		strncpy(cmd_buf, window_zone_command[i],
			strlen(window_zone_command[i]));

		if(window_edge[i -1] == 0){
			strncpy(conv_str, " 00 00", strlen( " 00 00"));
		}else{
			rc = convert_format(window_edge[i -1], conv_str);
			if(rc < 0){
				TS_LOG_ERR ("%s: Format convert failed!\n", __func__);
				goto free_buf;
			}

			if(strlen(conv_str) < 4){  /*additional bit*/
				strncat(conv_str, " 00", strlen( " 00"));
			}
		}

		/* merge  command*/
		strncpy(cmd_buf + strlen(cmd_buf), conv_str, strlen(conv_str));
		TS_LOG_DEBUG("%s : cmd_buf = %s \n", __func__, cmd_buf);

		rc = cyttsp5_send_command(dev, cmd_buf);
		if (rc || !cyttsp5_command_status(dev)) {
			TS_LOG_ERR
			    ("%s: Send read TP holster mode command failed!\n",
			     __func__);
			goto free_buf;
		}
		memset(cmd_buf, 0 , strlen(cmd_buf));
		memset(conv_str, 0 , strlen(conv_str));
	}

free_buf:
	if(cmd_buf != NULL)
		kfree(cmd_buf);
	return rc;
}

int cyttsp5_set_holster_mode(struct device *dev, int holster_switch)
{
	u8 response_buf[CY_MAX_BUF_SIZE] = { 0 };
	char response_str[COVER_RESPONSE_MAX_LEN] = { 0 };
	int i = 0;
	int num_read = 0;
	int index = 0;
	int rc = -1;
	int flag = 0;
	int length = 0;
	struct cyttsp5_core_data *cd = dev_get_drvdata(dev);

	TS_LOG_DEBUG("Enter %s,holster_switch = %d.\n", __func__,
		     holster_switch);

	if(cd->window_info.enable){
		rc = set_effective_window(dev);
		if (rc < 0) {
			TS_LOG_ERR("%s: set holster window zone fail !\n", __func__);
			return -1;
		}
	}

	memset(response_buf, 0, sizeof(response_buf));

	flag = cyttsp5_get_holster_mode(dev);
	if (flag < 0) {
		TS_LOG_ERR("%s,get Touch holster mode failed.\n", __func__);
		return -1;
	}

	if (holster_switch == flag) {
		TS_LOG_INFO("%s,Touch is already in %s mode.\n", __func__,
			    (flag == 1) ? "holster ON" : "holster OFF");
		return 0;
	}

	TS_LOG_DEBUG("%s,Begin to switch holster mode.\n", __func__);

	if (holster_switch) {
		rc = cyttsp5_send_command(dev, holster_command[SET_HOLSTER_ON]);
		if (rc || !cyttsp5_command_status(dev)) {
			TS_LOG_ERR
			    ("%s: Send read TP holster mode command failed!\n",
			     __func__);
			goto exit;
		}
	} else {
		rc = cyttsp5_send_command(dev,
					  holster_command[SET_HOLSTER_OFF]);
		if (rc || !cyttsp5_command_status(dev)) {
			TS_LOG_ERR
			    ("%s: Send read TP holster mode command failed!\n",
			     __func__);
			goto exit;
		}

	}

	msleep(50);

	num_read = cyttsp5_command_response(dev, response_buf);
	if (!cyttsp5_command_status(dev) || num_read < 0) {
		TS_LOG_ERR("%s: Get command response failed\n", __func__);
		goto exit;
	}

	for (i = 0; i < num_read; i++) {
		index += scnprintf(response_str + index, COVER_RESPONSE_MAX_LEN,
				   "%02X ", response_buf[i]);
	}

	TS_LOG_DEBUG("%s,response_str=%s.\n", __func__, response_str);

	length = strlen(holster_response_state[SET_HOLSTER_SUCCESSFUL_ONE]);
	for (i = SET_HOLSTER_SUCCESSFUL_ONE; i <= SET_HOLSTER_SUCCESSFUL_TWO;
	     i++) {
		if (strncmp(holster_response_state[i], response_str, length) ==
		    0) {
			TS_LOG_INFO("%s,Set TP holster mode successful!\n",
				    __func__);
			return 0;
		}
	}

	rc = -1;
exit:
	TS_LOG_ERR("%s,Set TP holster mode failed.\n", __func__);
	return rc;
}

void cyttsp5_holster_mode_work_function(struct work_struct *work)
{
	struct cyttsp5_core_data *cd =
	    container_of(work, struct cyttsp5_core_data, holster_work);
	int rc = -1;
	int flag_mode = -1;
	struct device *dev = cd->dev;

	TS_LOG_DEBUG("Enter %s.\n", __func__);

	atomic_set(&cd->holster_work_status, 1);

	flag_mode = cd->window_info.holster_switch;
	rc = cyttsp5_set_holster_mode(dev, flag_mode);
	if (rc < 0) {
		TS_LOG_INFO("%s:set tp holster mode %s failed!\n",
			    __func__, (flag_mode == 0 ? "OFF" : "ON"));

		if (cd->check_holster > 0) {
			cd->check_holster -= 1;
			mod_timer(&cd->holster_timer, jiffies +
				  msecs_to_jiffies(CY_COVER_MODE_TIMEOUT));
		}
	} else {
		cd->check_holster = 0;
	}

	atomic_set(&cd->holster_work_status, 0);

	TS_LOG_INFO("Exit %s.\n", __func__);
}

void cyttsp5_holster_mode_timer(unsigned long handle)
{
	struct cyttsp5_core_data *cd = (struct cyttsp5_core_data *)handle;

	TS_LOG_DEBUG("Enter %s.\n", __func__);

	if (atomic_read(&cd->holster_work_status)) {
		TS_LOG_INFO("%s: work_is_busy = %d.\n", __func__,
			    atomic_read(&cd->holster_work_status));
		mod_timer(&cd->holster_timer,
			  jiffies + msecs_to_jiffies(CY_COVER_MODE_TIMEOUT));
	} else {
		TS_LOG_INFO("%s: work_is_busy = %d.\n", __func__,
			    atomic_read(&cd->holster_work_status));
		schedule_work(&cd->holster_work);
	}

	TS_LOG_INFO("Exit %s.\n", __func__);
}

int cyttsp5_in_available_area(struct device *dev, int x_axis, int y_axis)
{
	struct cyttsp5_core_data *cd = dev_get_drvdata(dev);

	TS_LOG_DEBUG("%s:holster value is %d, (%d,%d), (%d,%d)\n",
		     __func__, cd->window_info.holster_switch,
		     cd->window_info.x0, cd->window_info.y0, cd->window_info.x1,
		     cd->window_info.y1);

	if ((x_axis > cd->window_info.x0 && x_axis < cd->window_info.x1)
	    && (y_axis > cd->window_info.y0 && y_axis < cd->window_info.y1)) {
		return 1;
	}

	return 0;
}

struct kobject *tp_get_virtual_key_obj(char *name)
{
	if (NULL == virtual_key_kobject_ts) {
		virtual_key_kobject_ts = kobject_create_and_add(name, NULL);
		if (!virtual_key_kobject_ts) {
			TS_LOG_ERR("%s: create virtual_key kobjetct error!\n",
				   __func__);
			return NULL;
		} else {
			TS_LOG_DEBUG("%s: create virtual_key successful!\n",
				     __func__);
		}
	} else {
		TS_LOG_DEBUG("%s: virtual_key already exist!\n", __func__);
	}

	return virtual_key_kobject_ts;
}
