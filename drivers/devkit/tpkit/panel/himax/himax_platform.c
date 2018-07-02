/* Himax Android Driver Sample Code Ver for Himax chipset
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

#include "himax_platform.h"
#include "himax_ic.h"

int irq_enable_count = 1;
extern struct himax_ts_data *g_himax_ts_data;

//DEFINE_MUTEX(tp_wr_access);

int i2c_himax_read(uint8_t command, uint8_t *data, uint16_t length, uint16_t limit_len,uint8_t toRetry)
{
	int retry = 0;
	int retval = -1;
	uint16_t addr_length = 0;
	uint8_t cmd_input[1] = {0};
	struct ts_bus_info *bops = NULL;
	if(data == NULL) {
		return retval;
	}
	if(length > limit_len){
		TS_LOG_ERR("%s: i2c_read_block size error %d, over array size%d\n",
			__func__, length, limit_len);
		return retval;
	}

	bops = g_himax_ts_data->tskit_himax_data->ts_platform_data->bops;
	addr_length = 1;
	cmd_input[0] = command;
	for (retry = 0; retry < toRetry; retry++) {
		retval = bops->bus_read(cmd_input, addr_length, data, length);
		if (retval<0)
			mdelay(HX_SLEEP_5MS);
		else
			break;
	}
	if (retry == toRetry) {
		TS_LOG_ERR("%s: i2c_read_block error %d, retry over %d\n",
			__func__, retval, toRetry);
		#ifdef CONFIG_HUAWEI_DSM
			hmx_tp_report_dsm_err(DSM_TP_I2C_RW_ERROR_NO , retval);
		#endif
		return -EIO;
	}
	return retval;

}

int i2c_himax_write(uint8_t command,uint8_t *data, uint16_t length, uint16_t limit_len, uint8_t toRetry)
{
	int j = 0;
	int retval = -1;
	int retry = 0;
	uint8_t buf[HX_I2C_MAX_SIZE] = {0};
	struct ts_bus_info *bops = NULL;
	int m=0;
	if(data == NULL) {
		return retval;
	}
	if((length > limit_len)||(length > HX_I2C_MAX_SIZE)){
		TS_LOG_ERR("%s: i2c_read_block size error %d, over array size%d\n",
			__func__, length, limit_len);
		return retval;
	}

	bops = g_himax_ts_data->tskit_himax_data->ts_platform_data->bops;

	buf[0] = command;
	for(j=0;j<length;j++) {
		m=j+1;
		buf[m] = data[j];
	}
	for (retry = 0; retry < toRetry; retry++) {
		retval = bops->bus_write(buf, length+1);
		if (retval<0)
			mdelay(HX_SLEEP_5MS);
		else
			break;
	}

	if (retry == toRetry) {
		TS_LOG_ERR("%s: i2c_read_block error %d, retry over %d\n",
			__func__, retval, toRetry);
		#ifdef CONFIG_HUAWEI_DSM
			hmx_tp_report_dsm_err(DSM_TP_I2C_RW_ERROR_NO , retval);
		#endif
		return -EIO;
	}
	return retval;

}

int i2c_himax_write_command( uint8_t command, uint8_t toRetry)
{
	uint8_t data[1] = {0};
	data[0]=command;
	return i2c_himax_master_write(data, 1,sizeof(data), toRetry);
}

int i2c_himax_master_write(uint8_t *data, uint16_t length, uint16_t limit_len, uint8_t toRetry)
{
	int retry = 0/*, loop_i*/;
	int retval = -1;
	struct ts_bus_info *bops = NULL;

	if(length > limit_len){
		TS_LOG_ERR("%s: i2c_read_block size error %d, over array size%d\n",
			__func__, length, limit_len);
		return retval;
	}

	bops = g_himax_ts_data->tskit_himax_data->ts_platform_data->bops;

	for (retry = 0; retry < toRetry; retry++) {
		retval = bops->bus_write(data, length);
		if (retval<0)
			mdelay(HX_SLEEP_5MS);
		else
			break;
	}

	if (retry == toRetry) {
		TS_LOG_ERR("%s: i2c_read_block error %d, retry over %d\n",
			__func__, retval, toRetry);
		#ifdef CONFIG_HUAWEI_DSM
			hmx_tp_report_dsm_err(DSM_TP_I2C_RW_ERROR_NO , retval);
		#endif
		return -EIO;
	}
	return retval;
}
void himax_int_enable(int irqnum, int enable)
{
	TS_LOG_INFO("S_irqnum=%d, irq_enable_count = %d, enable =%d\n",irqnum, irq_enable_count, enable);

	if (enable == 1 && irq_enable_count == 0) {
		enable_irq(irqnum);
		irq_enable_count=1;
	} else if (enable == 0 && irq_enable_count == 1) {
		disable_irq_nosync(irqnum);
		irq_enable_count=0;
	}
	TS_LOG_INFO("E_irqnum=%d, irq_enable_count = %d, enable =%d\n",irqnum, irq_enable_count, enable);
}

void himax_rst_gpio_set(int pinnum, uint8_t value)
{
	gpio_direction_output(pinnum, value);
}

uint8_t himax_int_gpio_read(int pinnum)
{
	return gpio_get_value(pinnum);
}

int himax_gpio_power_config(struct himax_i2c_platform_data *pdata)
{
	int err = 0;
	TS_LOG_INFO("%s:enter\n", __func__);
	if (!pdata){
		TS_LOG_ERR("device, ts_kit_platform_data *data is NULL \n");
		err =  -EINVAL;
		goto out;
	}

	if (pdata->gpio_3v3_en > 0) {
		err = gpio_request(pdata->gpio_3v3_en, "himax-3v3_en");
		if (err < 0) {
			TS_LOG_ERR("%s: request 3v3_en pin failed\n", __func__);
			goto err_req_3v3;
		}
	}
	if (pdata->gpio_1v8_en > 0) {
		err = gpio_request(pdata->gpio_1v8_en, "himax-1V8_en");
		if (err < 0) {
			TS_LOG_ERR("%s: request gpio_1v8_en pin failed\n", __func__);
			goto err_req_1v8;
		}
	}
	if (gpio_is_valid(pdata->gpio_irq)) {
		err = gpio_direction_input(pdata->gpio_irq);
		if (err) {
			TS_LOG_ERR("unable to set direction for gpio [%d]\n",pdata->gpio_irq);
			goto err_req_1v8;
		}
	} else {
		TS_LOG_ERR("irq gpio not provided\n");
		goto err_req_1v8;
	}
	TS_LOG_INFO("%s:normal exit\n", __func__);

	return err;

err_req_1v8:
	gpio_free(pdata->gpio_1v8_en);
err_req_3v3:
	gpio_free(pdata->gpio_3v3_en);
out:
	TS_LOG_ERR("%s:error exit\n", __func__);

	return err;
}

int himax_gpio_power_on(struct himax_i2c_platform_data *pdata)
{
	int err = 0;
	TS_LOG_INFO("%s:enter\n", __func__);

	if (pdata->gpio_reset >= 0) {
		err = gpio_direction_output(pdata->gpio_reset, 0);
		if (err) {
			TS_LOG_ERR("unable to set direction for gpio [%d]\n",
				pdata->gpio_reset);
			return err;
		}
	}
	if (pdata->gpio_3v3_en >= 0) {
		err = gpio_direction_output(pdata->gpio_3v3_en, 1);
		if (err) {
			TS_LOG_ERR("unable to set direction for gpio [%d]\n",
				pdata->gpio_3v3_en);
			return err;
		}
		TS_LOG_INFO("3v3_en pin =%d\n", gpio_get_value(pdata->gpio_3v3_en));
	}
	usleep_range(VCCA_DELAY_TIME, VCCA_DELAY_TIME);
	if (pdata->gpio_1v8_en >= 0) {
		err = gpio_direction_output(pdata->gpio_1v8_en, 1);
		if (err) {
			TS_LOG_ERR("unable to set direction for gpio [%d]\n",
				pdata->gpio_1v8_en);
			return err;
		}
		TS_LOG_INFO("gpio_1v8_en pin =%d\n", gpio_get_value(pdata->gpio_1v8_en));
	}

	msleep(VCCD_DELAY_TIME);
	if (pdata->gpio_reset >= 0) {
		err = gpio_direction_output(pdata->gpio_reset, 1);
		if (err) {
			TS_LOG_ERR("unable to set direction for gpio [%d]\n",
				pdata->gpio_reset);
			return err;
		}
	}
	msleep(RESET_HIGH_TIME);
	TS_LOG_INFO("%s:exit\n", __func__);
	return err;
}

void himax_gpio_power_deconfig(struct himax_i2c_platform_data *pdata)
{
 	TS_LOG_INFO("%s:enter\n", __func__);
	if(NULL == pdata) {
		return;
	}
	if (pdata->gpio_reset >= 0) {
		gpio_free(pdata->gpio_reset);
	}
	if (pdata->gpio_3v3_en >= 0) {
		gpio_free(pdata->gpio_3v3_en);
	}
	if (pdata->gpio_1v8_en >= 0) {
		gpio_free(pdata->gpio_1v8_en);
	}
	if (gpio_is_valid(pdata->gpio_irq)) {
		gpio_free(pdata->gpio_irq);
	}

	TS_LOG_INFO("%s:exit\n", __func__);
}

void himax_gpio_power_off(struct himax_i2c_platform_data *pdata)
{
	int err = 0;
	TS_LOG_INFO("%s:enter\n", __func__);
	if(NULL == pdata) {
		return;
	}
	if (pdata->gpio_3v3_en >= 0) {
		err = gpio_direction_output(pdata->gpio_3v3_en, 0);
		if (err) {
			TS_LOG_ERR("unable to set direction for gpio [%d]\n",
				pdata->gpio_3v3_en);
			return;
		}
	}
	if (pdata->gpio_1v8_en >= 0) {
		err = gpio_direction_output(pdata->gpio_1v8_en, 0);
		if (err) {
			TS_LOG_ERR("unable to set direction for gpio [%d]\n",
				pdata->gpio_1v8_en);
			return;
		}
	}
	TS_LOG_INFO("%s:exit\n", __func__);
}
