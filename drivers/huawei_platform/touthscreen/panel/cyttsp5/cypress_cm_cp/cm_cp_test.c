/******************************************************************************
 * @file cm_cp_test.c
 *
 * cm_cp_test.c
 *
 * @version 0.0.1
 * @authors btok
 *
 ******************************************************************************
 * Copyright (2014), Cypress Semiconductor Corporation. All rights reserved.
 *
 * This software, associated documentation and materials ("Software") is owned
 * by Cypress Semiconductor Corporation ("Cypress") and is protected by and
 * subject to worldwide patent protection (United States and foreign), United
 * States copyright laws and international treaty provisions. Therefore, unless
 * otherwise specified in a separate license agreement between you and Cypress,
 * this Software must be treated like any other copyrighted material.
 * Reproduction, modification, translation, compilation, or representation of
 * this Software in any other form (e.g., paper, magnetic, optical, silicon) is
 * prohibited without Cypress's express written permission.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * Cypress reserves the right to make changes to the Software without notice.
 * Cypress does not assume any liability arising out of the application or use
 * of Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use as critical components in any products
 * where a malfunction or failure may reasonably be expected to result in
 * significant injury or death ("High Risk Product"). By including Cypress's
 * product in a High Risk Product, the manufacturer of such system or
 * application assumes all risk of such use and in doing so indemnifies Cypress
 * against all liability.
 *
 * Use of this Software may be limited by and subject to the applicable Cypress
 * software license agreement.
 *****************************************************************************/

#include <linux/device.h>
#include "pip.h"
#include "lookup.h"
#include "parameter.h"
#include "configuration.h"
#include "result.h"
#include "cm_cp_test.h"
#include "../cyttsp5_regs.h"
#include "../cyttsp5_core.h"

#define IDAC_LSB_DEFAULT	80
#define MTX_SUM_DEFAULT		1
#define CLK_DEFAULT		48
#define VREF_DEFAULT     2

#define CONVERSION_CONST	1000
#define MAX_READ_LENGTH     100

#define HW_BL_GIDAC_MULT (4 * 160)

/*#define ARRAY_SIZE(x)         sizeof(x)/sizeof(x[0])*/

#define ABS(x)			(((x) < 0) ? -(x) : (x))

//#define PRINT_CAP_DATA_DEBUG
static struct rx_attenuator_lookup rx_attenuator_lookup_table[] = {
     RX_ATTENUATOR_LOOKUP_TABLE };

static int8_t mtx_sum_lookup_table[] = { MTX_SUM_LOOKUP_TABLE };

static int selfcap_signal_swing_lookup_table[] = {
     SELFCAP_SIGNAL_SWING_LOOKUP_TABLE };

static uint8_t g_family_type = 0;

static int rx_attenuator_lookup(uint8_t index, int *value)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(rx_attenuator_lookup_table); i++)
		if (index == rx_attenuator_lookup_table[i].index) {
			*value = rx_attenuator_lookup_table[i].value;
			return 0;
		}

	return -EINVAL;
}

static int mtx_sum_lookup(uint8_t mtx_order, int *mtx_sum)
{
	if (IS_MTX_ORDER_VALID(mtx_order)) {
		*mtx_sum =
		    (int)mtx_sum_lookup_table[GET_MTX_SUM_OFFSET(mtx_order)];
		return 0;
	}

	return -EINVAL;
};

static int selfcap_signal_swing_lookup(uint8_t ref_scale, uint8_t rxdac,
				       int *vref_low, int *vref_mid,
				       int *vref_high)
{
	if (IS_REF_SCALE_VALID(ref_scale) && IS_RXDAC_VALID(rxdac)) {
		*vref_low =
		    selfcap_signal_swing_lookup_table[GET_VREF_LOW_OFFSET
						      (ref_scale, rxdac)];
		*vref_mid =
		    selfcap_signal_swing_lookup_table[GET_VREF_MID_OFFSET
						      (ref_scale, rxdac)];

		*vref_high =
		    selfcap_signal_swing_lookup_table[GET_VREF_HIGH_OFFSET
						      (ref_scale, rxdac)];

		return 0;
	}

	return -EINVAL;
}

static int get_configuration_parameter(enum parameter_id id,
				       enum parameter_type *parameter_type,
				       union parameter_value *parameter_value)
{
	uint16_t parameter_address;
	uint16_t parameter_size;
	uint32_t parameter_mask;
	const union parameter_value *enumerated_value;
	uint16_t row_number;
	uint16_t row_offset;
	uint16_t read_length;
	uint8_t data[ROW_SIZE];
	uint32_t value;
	int ret;
	int i;

	TS_LOG_INFO("%s,id:%d\n", __func__, id);
	ret = parameter_get_info(id, &parameter_address, &parameter_size,
				 &parameter_mask, parameter_type);
	if (ret) {
		TS_LOG_ERR("%s:Unable to get parameter info!\n", __func__);
		goto exit;
	}

	row_number = parameter_address / ROW_SIZE;
	row_offset = parameter_address % ROW_SIZE;

	ret = pip_read_data_block(row_number, row_offset + parameter_size,
				  CONFIG_BLOCK_ID, &read_length, data);
	if (ret) {
		TS_LOG_ERR("%s:Unable to read data block!\n", __func__);
		goto exit;
	}
#ifdef PRINT_CAP_DATA_DEBUG
	TS_LOG_INFO("%s: data[0..%d]:\n", __func__, read_length - 1);
	for (i = 0; i < read_length; i++) {
		TS_LOG_INFO("%02X ", data[i]);
	}
	TS_LOG_INFO("\n");
#endif

	value = data[row_offset];
	if (parameter_size == 2 || parameter_size == 4)
		value += data[row_offset + 1] << 8;
	if (parameter_size == 4) {
		value += data[row_offset + 2] << 16;
		value += data[row_offset + 3] << 24;
	}

	if (parameter_mask) {
		value &= parameter_mask;
		while ((parameter_mask & 0x01) == 0) {
			value >>= 1;
			parameter_mask >>= 1;
		}
	}

	ret = parameter_get_enumerated_value(id, (int)value, &enumerated_value);
	if (ret) {
		TS_LOG_ERR("%s:Unable to get parameter enumerated value!\n",
			   __func__);
		goto exit;
	}

	if (enumerated_value)
		memcpy(parameter_value, enumerated_value,
		       sizeof(union parameter_value));
	else if (!enumerated_value && *parameter_type == INTEGER)
		parameter_value->integer = (int32_t) value;
	else {
		TS_LOG_ERR("%s:Unable to get parameter value!\n", __func__);
		ret = -EINVAL;
	}
	i = 0;
exit:
	return ret;
}

/*
 * Wrapper function for pip_retrieve_scan to call it multiple times
 * to gather data if necessary
 */
static int retrieve_panel_scan(uint16_t read_offset, uint16_t read_length,
			       uint8_t data_id, uint16_t *actual_read_length,
			       uint8_t *data_format, uint8_t *data)
{
	uint16_t actual_read_len;
	uint16_t myread = 0;
	uint16_t total_read_len = 0;
	int ret = 0;

	while (read_length > 0) {
		if (read_length > MAX_READ_LENGTH) {
			myread = MAX_READ_LENGTH;
		} else {
			myread = read_length;
		}
		ret = pip_retrieve_panel_scan(read_offset, myread,
					      data_id, &actual_read_len,
					      data_format, data);
		if (ret) {
			TS_LOG_ERR("%s:Unable to retrieve panel scan!\n",
				   __func__);
			goto exit;
		}
		/*mdelay(5000);*/
		if (actual_read_len == 0)
			break;
		TS_LOG_DEBUG("%s: read_offset: %d\n", __func__, read_offset);
		TS_LOG_DEBUG("%s: actual_read_len: %d\n", __func__,
			     actual_read_len);
		TS_LOG_DEBUG("%s: read_length: %d\n", __func__, read_length);
		TS_LOG_DEBUG("%s: data_format: %02X\n", __func__, *data_format);

		read_offset += actual_read_len;
		total_read_len += actual_read_len;

		data += actual_read_len * GET_ELEMENT_SIZE(*data_format);

		read_length -= actual_read_len;
	}

	*actual_read_length = total_read_len;

exit:
	return ret;
}

static int32_t get_element(uint8_t element_size, uint8_t sign, uint8_t *data)
{
	if (element_size == 1) {
		if (sign == SIGN_UNSIGNED) {
			return *data;
		} else {
			return (int8_t) *data;
		}
	} else if (element_size == 2) {
		if (sign == SIGN_UNSIGNED) {
			return get_unaligned_le16(data);
		} else {
			return (int16_t) get_unaligned_le16(data);
		}
	} else if (element_size == 4) {
		if (sign == SIGN_UNSIGNED) {
			return get_unaligned_le32(data);
		} else {
			return (int)get_unaligned_le32(data);
		}
	}

	return 0;
}

static int retrieve_panel_raw_data(uint8_t data_id, uint16_t read_offset,
				   uint16_t read_length, uint8_t *data_format,
				   int32_t *raw_data)
{
	uint16_t actual_read_length;
	uint8_t element_size;
	uint8_t sign;
	uint8_t *data;
	int ret;
	int i;

	data = kzalloc(read_length * sizeof(uint32_t), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	ret = retrieve_panel_scan(read_offset, read_length, data_id,
				  &actual_read_length, data_format, data);
	if (ret) {
		TS_LOG_ERR("%s:Unable to retrieve panel raw data!\n", __func__);
		goto free;
	}

	if (read_length != actual_read_length) {
		TS_LOG_ERR("%s:Read length is not match!\n", __func__);
		ret = -EINVAL;
		goto free;
	}

	element_size = GET_ELEMENT_SIZE(*data_format);
	if (element_size != 1 && element_size != 2 && element_size != 4) {
		TS_LOG_ERR("%s:Element size error!\n", __func__);
		ret = -EINVAL;
		goto free;
	}

	sign = GET_SIGN(*data_format);

	for (i = 0; i < read_length; i++)
		raw_data[i] = get_element(element_size, sign,
					  &data[i << (element_size / 2)]);

free:
	kfree(data);

	return ret;
}

#if 1
static int retrieve_data_structure(uint16_t read_offset, uint16_t read_length,
				   uint8_t data_id,
				   uint16_t *actual_read_length,
				   uint8_t *data_format, uint8_t *data)
{
	uint16_t actual_read_len;
	uint16_t total_read_len = 0;
	uint16_t myread = 0;
	int ret = 0;
	TS_LOG_INFO("%s called\n", __func__);
	while (read_length > 0) {
		if (read_length > MAX_READ_LENGTH) {
			myread = MAX_READ_LENGTH;
		} else {
			myread = read_length;
		}
		ret = pip_retrieve_data_structure(read_offset, myread,
						  data_id, &actual_read_len,
						  data_format, data);
		if (ret) {
			TS_LOG_ERR("%s:Unable to retrieve panel scan!\n",
				   __func__);
			goto exit;
		}
		/*mdelay(5000);*/
		if (actual_read_len == 0)
			break;
		TS_LOG_DEBUG("%s: read_offset: %d\n", __func__, read_offset);
		TS_LOG_DEBUG("%s: actual_read_len: %d\n", __func__,
			     actual_read_len);
		TS_LOG_DEBUG("%s: read_length: %d\n", __func__, read_length);
		TS_LOG_DEBUG("%s: data_format: %02X\n", __func__, *data_format);

		read_offset += actual_read_len;
		total_read_len += actual_read_len;

		data += actual_read_len * GET_ELEMENT_SIZE(*data_format);

		read_length -= actual_read_len;
	}

	*actual_read_length = total_read_len;

exit:
	return ret;
}
#if 0
static int retrieve_panel_localpwc(uint8_t data_id, uint16_t read_offset,
				   uint16_t read_length, uint8_t *data_format,
				   int32_t *sensor_localpwc)
{
	uint16_t actual_read_length;
	uint8_t element_size;
	uint8_t sign;
	uint8_t *data;
	int ret;
	int i;

	data = kzalloc(read_length * sizeof(uint32_t), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	ret = retrieve_data_structure(read_offset, read_length, data_id,
				      &actual_read_length, data_format, data);
	if (ret) {
		TS_LOG_ERR("%s:Unable to retrieve panel pwc data!\n", __func__);
		goto free;
	}

	if (read_length != actual_read_length) {
		ret = -EINVAL;
		goto free;
	}

	element_size = GET_ELEMENT_SIZE(*data_format);
	if (element_size != 1 && element_size != 2 && element_size != 4) {
		ret = -EINVAL;
		goto free;
	}

	sign = GET_SIGN(*data_format);

	for (i = 0; i < read_length; i++)
		sensor_localpwc[i] = get_element(element_size, sign,
						 &data[i <<
						       (element_size / 2)]);

free:
	kfree(data);

	return ret;
}
#endif
#endif

static int calculate_cm_gen6(int raw_data, int tx_period_mutual,
			     int balancing_target_mutual,
			     int scaling_factor_mutual, int idac_lsb,
			     int idac_mutual, int rx_atten_mutual,
			     int gidac_mult, int clk, int vtx, int mtx_sum)
{
	long long i_bias = gidac_mult * idac_lsb * idac_mutual;
	long long adc =
	    (((long long)tx_period_mutual * balancing_target_mutual *
	      scaling_factor_mutual * idac_mutual * rx_atten_mutual) -
	     50000 * raw_data) / (scaling_factor_mutual * idac_mutual);

	return i_bias * adc / (clk * vtx * mtx_sum * 1000);
}
static int calculate_cm_gen6l(int raw_data, int tx_period_mutual,
			     int balancing_target_mutual,
			     int scaling_factor_mutual, int idac_lsb,
			     int idac_mutual, int rx_atten_mutual,
			     int gidac_mult, int clk, int vtx, int mtx_sum,
			     int hw_bl_gidac_lsb_config, int hw_bl_gidac_mutual, 
			     int hw_bl_pwc_mutual)
{
	long i_bias = gidac_mult * idac_lsb * idac_mutual;
	long  adc = tx_period_mutual * balancing_target_mutual -
			50l* CONVERSION_CONST  * raw_data / 
			(scaling_factor_mutual * idac_mutual * rx_atten_mutual);

	long cap = (i_bias * adc * rx_atten_mutual / (clk * vtx * mtx_sum)+
		   HW_BL_GIDAC_MULT* CONVERSION_CONST *(hw_bl_gidac_lsb_config+1l)/(clk * vtx * mtx_sum)*
		   hw_bl_gidac_mutual * hw_bl_pwc_mutual )/CONVERSION_CONST;

	return (int)cap;

}
#if 0
static void calculate_gradient_row(struct gd_sensor *gd_sensor_row_head,
				   uint16_t row_num, int exclude_row_edge,
				   int exclude_col_edge)
{
	int i = 0;
	int cm_min_cur = 0;
	int cm_max_cur = 0;
	int cm_ave_cur = 0;
	int cm_ave_next = 0;
	int cm_ave_prev = 0;
	struct gd_sensor *p = gd_sensor_row_head;
	if (exclude_row_edge) {
		for (i = 0; i < row_num; i++) {
			if (!exclude_col_edge) {
				cm_ave_cur = (p + i)->cm_ave;
				cm_min_cur = (p + i)->cm_min;
				cm_max_cur = (p + i)->cm_max;
				if (i < (row_num - 1)) {
					cm_ave_next = (p + i + 1)->cm_ave;
				}
				if (i > 0) {
					cm_ave_prev = (p + i - 1)->cm_ave;
				}
			} else {
				cm_ave_cur = (p + i)->cm_ave_exclude_edge;
				cm_min_cur = (p + i)->cm_min_exclude_edge;
				cm_max_cur = (p + i)->cm_max_exclude_edge;
				if (i < (row_num - 1)) {
					cm_ave_next =
					    (p + i + 1)->cm_ave_exclude_edge;
				}
				if (i > 0) {
					cm_ave_prev =
					    (p + i - 1)->cm_ave_exclude_edge;
				}
			}
			if (cm_ave_cur == 0) {
				cm_ave_cur = 1;
			}
			if ((i == 0) || (i == (row_num - 1))) {
				(p + i)->gradient_val =
				    (cm_max_cur -
				     cm_min_cur) * 100 / cm_ave_cur;
			} else if (i == 1) {
				(p + i)->gradient_val =
				    (cm_max_cur - cm_min_cur +
				     ABS(cm_ave_cur -
					 cm_ave_next)) * 100 / cm_ave_cur;
			} else {
				(p + i)->gradient_val =
				    (cm_max_cur - cm_min_cur +
				     ABS(cm_ave_cur -
					 cm_ave_prev)) * 100 / cm_ave_cur;
			}
		}
	} else if (!exclude_row_edge) {
		for (i = 0; i < row_num; i++) {
			if (!exclude_col_edge) {
				cm_ave_cur = (p + i)->cm_ave;
				cm_min_cur = (p + i)->cm_min;
				cm_max_cur = (p + i)->cm_max;
				if (i < (row_num - 1)) {
					cm_ave_next = (p + i + 1)->cm_ave;
				}
				if (i > 0) {
					cm_ave_prev = (p + i - 1)->cm_ave;
				}
			} else {
				cm_ave_cur = (p + i)->cm_ave_exclude_edge;
				cm_min_cur = (p + i)->cm_min_exclude_edge;
				cm_max_cur = (p + i)->cm_max_exclude_edge;
				if (i < (row_num - 1)) {
					cm_ave_next =
					    (p + i + 1)->cm_ave_exclude_edge;
				}
				if (i > 0) {
					cm_ave_prev =
					    (p + i - 1)->cm_ave_exclude_edge;
				}
			}
			if (cm_ave_cur == 0) {
				cm_ave_cur = 1;
			}
			if (i <= 1) {
				(p + i)->gradient_val =
				    (cm_max_cur - cm_min_cur +
				     ABS(cm_ave_cur -
					 cm_ave_next)) * 100 / cm_ave_cur;
			} else {
				(p + i)->gradient_val =
				    (cm_max_cur - cm_min_cur +
				     ABS(cm_ave_cur -
					 cm_ave_prev)) * 100 / cm_ave_cur;
			}
		}
	}
}
#endif
#if 0
static void calculate_gradient_col(struct gd_sensor *gd_sensor_row_head,
				   uint16_t col_num, int exclude_row_edge,
				   int exclude_col_edge)
{
	int i = 0;
	int cm_min_cur = 0;
	int cm_max_cur = 0;
	int cm_ave_cur = 0;
	int cm_ave_next = 0;
	int cm_ave_prev = 0;
	struct gd_sensor *p = gd_sensor_row_head;
	if (!exclude_col_edge) {
		for (i = 0; i < col_num; i++) {
			if (!exclude_row_edge) {
				cm_ave_cur = (p + i)->cm_ave;
				cm_min_cur = (p + i)->cm_min;
				cm_max_cur = (p + i)->cm_max;
				if (i < (col_num - 1)) {
					cm_ave_next = (p + i + 1)->cm_ave;
				}
				if (i > 0) {
					cm_ave_prev = (p + i - 1)->cm_ave;
				}
			} else {
				cm_ave_cur = (p + i)->cm_ave_exclude_edge;
				cm_min_cur = (p + i)->cm_min_exclude_edge;
				cm_max_cur = (p + i)->cm_max_exclude_edge;
				if (i < (col_num - 1)) {
					cm_ave_next =
					    (p + i + 1)->cm_ave_exclude_edge;
				}
				if (i > 0) {
					cm_ave_prev =
					    (p + i - 1)->cm_ave_exclude_edge;
				}
			}
			if (cm_ave_cur == 0) {
				cm_ave_cur = 1;
			}
			if (i <= 1) {
				(p + i)->gradient_val =
				    (cm_max_cur - cm_min_cur +
				     ABS(cm_ave_cur -
					 cm_ave_next)) * 100 / cm_ave_cur;
			} else {
				(p + i)->gradient_val =
				    (cm_max_cur - cm_min_cur +
				     ABS(cm_ave_cur -
					 cm_ave_prev)) * 100 / cm_ave_cur;
			}
		}
	} else if (exclude_col_edge) {
		for (i = 0; i < col_num; i++) {
			if (!exclude_row_edge) {
				cm_ave_cur = (p + i)->cm_ave;
				cm_min_cur = (p + i)->cm_min;
				cm_max_cur = (p + i)->cm_max;
				if (i < (col_num - 1)) {
					cm_ave_next = (p + i + 1)->cm_ave;
				}
				if (i > 0) {
					cm_ave_prev = (p + i - 1)->cm_ave;
				}
			} else {
				cm_ave_cur = (p + i)->cm_ave_exclude_edge;
				cm_min_cur = (p + i)->cm_min_exclude_edge;
				cm_max_cur = (p + i)->cm_max_exclude_edge;
				if (i < (col_num - 1)) {
					cm_ave_next =
					    (p + i + 1)->cm_ave_exclude_edge;
				}
				if (i > 0) {
					cm_ave_prev =
					    (p + i - 1)->cm_ave_exclude_edge;
				}
			}
			if (cm_ave_cur == 0) {
				cm_ave_cur = 1;
			}

			if ((i == 0) || (i == (col_num - 1))) {
				(p + i)->gradient_val =
				    (cm_max_cur -
				     cm_min_cur) * 100 / cm_ave_cur;
			} else if (i == 1) {
				(p + i)->gradient_val =
				    (cm_max_cur - cm_min_cur +
				     ABS(cm_ave_cur -
					 cm_ave_next)) * 100 / cm_ave_cur;
			} else {
				(p + i)->gradient_val =
				    (cm_max_cur - cm_min_cur +
				     ABS(cm_ave_cur -
					 cm_ave_prev)) * 100 / cm_ave_cur;
			}
		}
	}

}

#endif
#if 0
static void fill_gd_sensor_table(struct gd_sensor *head, int index,
				 uint16_t cm_max, uint16_t cm_min, int cm_ave,
				 uint16_t cm_max_exclude_edge,
				 uint16_t cm_min_exclude_edge,
				 int cm_ave_exclude_edge)
{
	(head + index)->cm_max = cm_max;
	(head + index)->cm_min = cm_min;
	(head + index)->cm_ave = cm_ave;
	(head + index)->cm_ave_exclude_edge = cm_ave_exclude_edge;
	(head + index)->cm_max_exclude_edge = cm_max_exclude_edge;
	(head + index)->cm_min_exclude_edge = cm_min_exclude_edge;
}
#endif
static int get_cm_uniformity_test_results_gen6(int vdda, uint16_t tx_num,
					       uint16_t rx_num,
					       uint16_t button_num,
					       bool skip_cm_button,
					       int32_t **sensor_raw_data,
					       int **cm_sensor_data,
					       int *cm_sensor_average)
{
	union parameter_value parameter_value;
	enum parameter_type parameter_type;
	uint16_t read_length;
	uint16_t sensor_element_num = 0;
	uint8_t data_format = 0;
	int8_t idac_mutual;
	int rx_atten_mutual;
	uint32_t tx_period_mutual;
	char *vdda_mode;
	uint32_t scaling_factor_mutual;
	uint32_t balancing_target_mutual;
	uint32_t gidac_mult;
	int vtx;
	int idac_lsb = IDAC_LSB_DEFAULT;
	int mtx_sum = MTX_SUM_DEFAULT;
	int clk = CLK_DEFAULT;
	uint8_t data[IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_LENGTH];
	int ret;
	int i;
	int j;

	sensor_element_num = rx_num * tx_num;
	TS_LOG_INFO("%s, get_cm_uniformity_test_results_gen6 called\n",
		    __func__);
	*sensor_raw_data =
	    kzalloc(sensor_element_num * sizeof(int32_t), GFP_KERNEL);
	*cm_sensor_data = kzalloc(sensor_element_num * sizeof(int), GFP_KERNEL);

	if (!*sensor_raw_data || !*cm_sensor_data) {
		ret = -ENOMEM;
		goto exit;
	}

	TS_LOG_INFO("%s: Set FORCE_SINGLE_TX to 1\n", __func__);

	/* Step 1: Set force single TX */
	ret = pip_set_parameter(FORCE_SINGLE_TX, 1, 0x01);
	if (ret) {
		TS_LOG_INFO("%s:Unable to set FORCE_SINGLE_TX parameter!\n",
			    __func__);
		goto exit;
	}

	/*workaround for CDT193384 */
	ret = pip_resume_scanning();
	if (ret) {
		TS_LOG_ERR("%s:Unable to resume panel scan!\n", __func__);
		goto restore_multi_tx;
	}

	ret = pip_suspend_scanning();
	if (ret) {
		TS_LOG_ERR("%s:Unable to suspend panel scan!\n", __func__);
		goto restore_multi_tx;
	}
	/*end workaround for CDT193384 */

	TS_LOG_INFO("%s: Perform calibrate IDACs\n", __func__);

	/* Step 2: Perform calibration */
	ret = pip_calibrate_idacs(0);
	if (ret) {
		TS_LOG_ERR("%s:Unable to calibrate IDACs!\n", __func__);
		goto restore_multi_tx;
	}
	if (button_num > 0) {
		ret = pip_calibrate_idacs(1);
		if (ret) {
			TS_LOG_ERR("%s Unable to calibrate button IDACs!\n",
				   __func__);
			goto restore_multi_tx;
		}
	}

	TS_LOG_INFO("%s: Get Mutual IDAC and RX Attenuator values\n", __func__);

	/* Step 3: Get Mutual IDAC and RX Attenuator values */
	ret = pip_retrieve_data_structure(0,
					  IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_LENGTH,
					  IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_ID,
					  &read_length, &data_format, data);
	if (ret) {
		TS_LOG_ERR("%s:Unable to retrieve data structure!\n", __func__);
		goto restore_multi_tx;
	}

	ret = rx_attenuator_lookup(data[RX_ATTENUATOR_MUTUAL_INDEX],
				   &rx_atten_mutual);
	if (ret) {
		TS_LOG_ERR("%s:Invalid RX Attenuator Index!\n", __func__);
		goto restore_multi_tx;
	}

	idac_mutual = (int8_t) data[IDAC_MUTUAL_INDEX];

	TS_LOG_INFO("%s: idac_mutual: %d\n", __func__, idac_mutual);
	TS_LOG_INFO("%s: rx_atten_mutual: %d\n", __func__, rx_atten_mutual);

	/* Get CDC:VDDA_MODE */
	ret = get_configuration_parameter(VDDA_MODE, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != STRING) {
		TS_LOG_ERR("%s:Unable to get vdda_mode!\n", __func__);
		goto restore_multi_tx;
	}

	vdda_mode = parameter_value.string;
	TS_LOG_INFO("%s: vdda_mode: %s\n", __func__, vdda_mode);

	if (!strcmp(vdda_mode, VDDA_MODE_BYPASS)) {
		if (vdda != 0)
			vtx = vdda;
		else {
			TS_LOG_ERR
			    ("%s:VDDA cannot be zero when VDDA_MODE is bypass!\n",
			     __func__);
			ret = -EINVAL;
			goto restore_multi_tx;
		}
	} else if (!strcmp(vdda_mode, VDDA_MODE_PUMP)) {
		/* Get CDC:TX_PUMP_VOLTAGE */
		ret = get_configuration_parameter(TX_PUMP_VOLTAGE,
						  &parameter_type,
						  &parameter_value);
		if (ret || parameter_type != FLOAT) {
			TS_LOG_ERR("%s:Unable to get tx_pump_voltage!\n",
				   __func__);
			goto restore_multi_tx;
		}
		vtx = parameter_value.flt;
	} else {
		TS_LOG_ERR("%s:Invalid VDDA_MODE: %s!\n", __func__, vdda_mode);
		ret = -EINVAL;
		goto restore_multi_tx;
	}

	TS_LOG_INFO("%s: vtx: %d\n", __func__, vtx);

	/* Get CDC:TX_PERIOD_MUTUAL */
	ret = get_configuration_parameter(TX_PERIOD_MUTUAL, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get tx_period_mutual!\n", __func__);
		goto restore_multi_tx;
	}

	tx_period_mutual = parameter_value.integer;
	TS_LOG_INFO("%s: tx_period_mutual: %d\n", __func__, tx_period_mutual);

	/* Get CDC:SCALING_FACTOR_MUTUAL */
	ret = get_configuration_parameter(SCALING_FACTOR_MUTUAL,
					  &parameter_type, &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get scaling_factor_mutual!\n",
			   __func__);
		goto restore_multi_tx;
	}

	scaling_factor_mutual = parameter_value.integer;
	TS_LOG_INFO("%s: scaling_factor_mutual: %d\n", __func__,
		    scaling_factor_mutual);

	/* Get Calibration:BALANCING_TARGET_MUTUAL */
	ret = get_configuration_parameter(BALANCING_TARGET_MUTUAL,
					  &parameter_type, &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get balancing_target_mutual!\n",
			   __func__);
		goto restore_multi_tx;
	}

	balancing_target_mutual = parameter_value.integer;
	TS_LOG_INFO("%s: balancing_target_mutual: %d\n", __func__,
		    balancing_target_mutual);

	/* Get CDC:INFRA_CTRL:GIDAC_MULT */
	ret = get_configuration_parameter(GIDAC_MULT,
					  &parameter_type, &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get gidac_mult!\n", __func__);
		goto restore_multi_tx;
	}

	gidac_mult = parameter_value.integer;

	TS_LOG_INFO("%s: gidac_mult:%d\n", __func__, gidac_mult);

	/* Step 4a: Execute Panel Scan */
	ret = pip_execute_panel_scan();
	if (ret) {
		TS_LOG_ERR("%s:Unable to execute panel scan!\n", __func__);
		goto restore_multi_tx;
	}

	/* Step 4b: Retrieve Panel Scan raw data */
	ret = retrieve_panel_raw_data(MUTUAL_CAP_RAW_DATA_ID, 0,
				      sensor_element_num, &data_format,
				      *sensor_raw_data);
	if (ret) {
		TS_LOG_ERR("%s:Unable to retrieve panel raw data!\n", __func__);
		goto restore_multi_tx;
	}
#ifdef PRINT_CAP_DATA_DEBUG
	TS_LOG_INFO("%s: sensor_raw_data[0..%d]:\n", __func__,
		    sensor_element_num - 1);
	for (i = 0; i < tx_num; i++) {
		for (j = 0; j < rx_num; j++) {
			TS_LOG_ERR("%5d ", (*sensor_raw_data)[i * rx_num + j]);
		}
		TS_LOG_INFO("\n");
	}
#endif

	/* Step 5 and 6: Calculate Cm_sensor and Cm_sensor_ave */
	*cm_sensor_average = 0;
	for (i = 0; i < sensor_element_num; i++) {
		(*cm_sensor_data)[i] = calculate_cm_gen6((*sensor_raw_data)[i],
							 tx_period_mutual,
							 balancing_target_mutual,
							 scaling_factor_mutual,
							 idac_lsb, idac_mutual,
							 rx_atten_mutual,
							 gidac_mult, clk, vtx,
							 mtx_sum);
		*cm_sensor_average += (*cm_sensor_data)[i];
	}
	*cm_sensor_average /= sensor_element_num;

#ifdef PRINT_CAP_DATA_DEBUG
	TS_LOG_ERR("%s: cm_sensor_data[0..%d]:\n", __func__,
		   sensor_element_num - 1);
	for (i = 0; i < tx_num; i++) {
		for (j = 0; j < rx_num; j++) {
			TS_LOG_ERR("%d ", (*cm_sensor_data)[i * rx_num + j]);
		}
		TS_LOG_INFO("\n");
	}
#endif

	TS_LOG_INFO("%s: cm_sensor_average: %d\n", __func__,
		    *cm_sensor_average);

restore_multi_tx:
	TS_LOG_INFO("%s: Set FORCE_SINGLE_TX to 0\n", __func__);

	/* Step 13: Set force single TX */
	ret = pip_set_parameter(FORCE_SINGLE_TX, 1, 0x00);
	if (ret) {
		TS_LOG_ERR("%s:Unable to set FORCE_SINGLE_TX parameter!\n",
			   __func__);
		goto exit;
	}

	TS_LOG_INFO("%s: Perform calibrate IDACs\n", __func__);

	/* Step 14: Perform calibration */
	ret = pip_calibrate_idacs(0);
	if (ret) {
		TS_LOG_ERR("%s:Unable to calibrate mutual IDACs!\n", __func__);
		goto exit;
	}
	if (button_num > 0) {
		ret = pip_calibrate_idacs(1);
		if (ret) {
			TS_LOG_ERR("%s:Unable to calibrate button IDACs!\n",
				   __func__);
			goto exit;
		}
	}

exit:
	if (ret) {
		kfree(*cm_sensor_data);
		kfree(*sensor_raw_data);
		*cm_sensor_data = NULL;
		*sensor_raw_data = NULL;
	}
	j = 0;
	return ret;
}

static int get_cm_uniformity_test_results_gen6l(int vdda, uint16_t tx_num,
					       uint16_t rx_num,uint16_t button_num,bool skip_cm_button,
					       int32_t **sensor_raw_data,int32_t **button_raw_data,
					       int **cm_sensor_data,int **cm_button_data,
					       int *cm_sensor_average,int *cm_button_average)
{
	union parameter_value parameter_value;
	enum parameter_type parameter_type;
	uint16_t read_length;
	uint16_t sensor_element_num = 0;
	uint8_t data_format = 0;
	int8_t idac_mutual;
	int8_t idac_mutual_btn =0;
	int rx_atten_mutual;
	int rx_atten_mutual_btn = 0;
	uint32_t tx_period_mutual;
	uint32_t tx_period_btn_mutual = 0;
	char *vdda_mode;
	uint32_t scaling_factor_mutual;
	uint32_t scaling_factor_btn_mutual = 0;
	uint32_t balancing_target_mutual;
	uint32_t balancing_target_btn_mutual = 0;
	uint32_t gidac_mult = 0;
	int vtx;
	int idac_lsb = IDAC_LSB_DEFAULT;
	int mtx_sum = MTX_SUM_DEFAULT;
	int clk = CLK_DEFAULT;
	uint8_t data[IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_LENGTH];
	uint8_t hw_bl_data[MUTUAL_HW_BASELINE_CALIBRATION_DATA_LENGTH] = {0};
	uint8_t hw_bl_gidac_mutual = 0;
	uint8_t hw_bl_pwc_mutual = 0;
	uint32_t hw_bl_gidac_lsb_config = 0;
	int ret;
	int i;
	int j;
	long long cm_sensor_average_bak =0;

	sensor_element_num = rx_num * tx_num;
	TS_LOG_INFO("%s, get_cm_uniformity_test_results_gen6 called\n",
		    __func__);
	*sensor_raw_data = kzalloc(sensor_element_num * sizeof(int32_t), GFP_KERNEL);
	*cm_sensor_data = kzalloc(sensor_element_num * sizeof(int), GFP_KERNEL);

	if (!*sensor_raw_data || !*cm_sensor_data) {
		ret = -ENOMEM;
		goto exit;
	}

	TS_LOG_INFO("%s: Set FORCE_SINGLE_TX to 1\n", __func__);

	/* Step 1: Set force single TX */
	ret = pip_set_parameter(FORCE_SINGLE_TX, 1, 0x01);
	if (ret) {
		TS_LOG_INFO("%s:Unable to set FORCE_SINGLE_TX parameter!\n",
			    __func__);
		goto exit;
	}

	/*workaround for CDT193384 */
	ret = pip_resume_scanning();
	if (ret) {
		TS_LOG_ERR("%s:Unable to resume panel scan!\n", __func__);
		goto restore_multi_tx;
	}

	ret = pip_suspend_scanning();
	if (ret) {
		TS_LOG_ERR("%s:Unable to suspend panel scan!\n", __func__);
		goto restore_multi_tx;
	}
	/*end workaround for CDT193384 */

	TS_LOG_INFO("%s: Perform calibrate IDACs\n", __func__);

	/* Step 2: Perform calibration */
	ret = pip_calibrate_idacs(0);
	if (ret) {
		TS_LOG_ERR("%s:Unable to calibrate IDACs!\n", __func__);
		goto restore_multi_tx;
	}
	if (button_num > 0) {
		ret = pip_calibrate_idacs(1);
		if (ret) {
			TS_LOG_ERR("%s Unable to calibrate button IDACs!\n",
				   __func__);
			goto restore_multi_tx;
		}
	}

	TS_LOG_INFO("%s: Get Mutual IDAC and RX Attenuator values\n", __func__);

	/* Step 3: Get Mutual IDAC and RX Attenuator values */
	ret = pip_retrieve_data_structure(0,
					  IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_LENGTH,
					  IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_ID,
					  &read_length, &data_format, data);
	if (ret) {
		TS_LOG_ERR("%s:Unable to retrieve data structure!\n", __func__);
		goto restore_multi_tx;
	}

	ret = rx_attenuator_lookup(data[RX_ATTENUATOR_MUTUAL_INDEX],
				   &rx_atten_mutual);
	if (ret) {
		TS_LOG_ERR("%s:Invalid RX Attenuator Index!\n", __func__);
		goto restore_multi_tx;
	}

	idac_mutual = (int8_t) data[IDAC_MUTUAL_INDEX];

	TS_LOG_INFO("%s: idac_mutual: %d\n", __func__, idac_mutual);
	TS_LOG_INFO("%s: rx_atten_mutual: %d\n", __func__, rx_atten_mutual);

	/* Get Mutual HW Baseline calibration data */
	ret = pip_retrieve_data_structure(0,
			MUTUAL_HW_BASELINE_CALIBRATION_DATA_LENGTH,
			MUTUAL_HW_BASELINE_CALIBRATION_DATA_ID,
			&read_length, &data_format, hw_bl_data);
	if (ret) {
		TS_LOG_ERR("%s:Unable to retrieve data structure!\n", __func__);
		goto restore_multi_tx;
	}

	hw_bl_gidac_mutual = hw_bl_data[0];
	hw_bl_pwc_mutual = hw_bl_data[1];

	TS_LOG_INFO("%s: hw_bl_gidac_mutual: %d\n", __func__, hw_bl_gidac_mutual);
	TS_LOG_INFO("%s: hw_bl_pwc_mutual: %d\n", __func__, hw_bl_pwc_mutual);

	/* Get CDC:VDDA_MODE */
	ret = get_configuration_parameter(VDDA_MODE, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != STRING) {
		TS_LOG_ERR("%s:Unable to get vdda_mode!\n", __func__);
		goto restore_multi_tx;
	}

	vdda_mode = parameter_value.string;
	TS_LOG_INFO("%s: vdda_mode: %s\n", __func__, vdda_mode);

	if (!strcmp(vdda_mode, VDDA_MODE_BYPASS)) {
		if (vdda != 0)
			vtx = vdda;
		else {
			TS_LOG_ERR
			    ("%s:VDDA cannot be zero when VDDA_MODE is bypass!\n",
			     __func__);
			ret = -EINVAL;
			goto restore_multi_tx;
		}
	} else if (!strcmp(vdda_mode, VDDA_MODE_PUMP)) {
		/* Get CDC:TX_PUMP_VOLTAGE */
		ret = get_configuration_parameter(TX_PUMP_VOLTAGE,
						  &parameter_type,
						  &parameter_value);
		if (ret || parameter_type != FLOAT) {
			TS_LOG_ERR("%s:Unable to get tx_pump_voltage!\n",
				   __func__);
			goto restore_multi_tx;
		}
		vtx = parameter_value.flt;
	} else {
		TS_LOG_ERR("%s:Invalid VDDA_MODE: %s!\n", __func__, vdda_mode);
		ret = -EINVAL;
		goto restore_multi_tx;
	}

	TS_LOG_INFO("%s: vtx: %d\n", __func__, vtx);

	/* Get CDC:TX_PERIOD_MUTUAL */
	ret = get_configuration_parameter(TX_PERIOD_MUTUAL, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get tx_period_mutual!\n", __func__);
		goto restore_multi_tx;
	}

	tx_period_mutual = parameter_value.integer;
	TS_LOG_INFO("%s: tx_period_mutual: %d\n", __func__, tx_period_mutual);

	/* Get CDC:SCALING_FACTOR_MUTUAL */
	ret = get_configuration_parameter(SCALING_FACTOR_MUTUAL,
					  &parameter_type, &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get scaling_factor_mutual!\n",
			   __func__);
		goto restore_multi_tx;
	}

	scaling_factor_mutual = parameter_value.integer;
	TS_LOG_INFO("%s: scaling_factor_mutual: %d\n", __func__,
		    scaling_factor_mutual);

	/* Get Calibration:BALANCING_TARGET_MUTUAL */
	ret = get_configuration_parameter(BALANCING_TARGET_MUTUAL,
					  &parameter_type, &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get balancing_target_mutual!\n",
			   __func__);
		goto restore_multi_tx;
	}

	balancing_target_mutual = parameter_value.integer;
	TS_LOG_INFO("%s: balancing_target_mutual: %d\n", __func__,
		    balancing_target_mutual);

	/* Get CDC:INFRA_CTRL:GIDAC_MULT */
	ret = get_configuration_parameter(GIDAC_MULT,
					  &parameter_type, &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get gidac_mult!\n", __func__);
		goto restore_multi_tx;
	}

	gidac_mult = parameter_value.integer;

	TS_LOG_INFO("%s: gidac_mult:%d\n", __func__, gidac_mult);
	
	/* Get CDC: HW_BL_GIDAC_LSB_CONFIG*/
	ret = get_configuration_parameter(HW_BL_GIDAC_LSB_CONFIG,
				&parameter_type, &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get hw_bl_gidac_lsb_config!\n", __func__);
		goto restore_multi_tx;
	}
	hw_bl_gidac_lsb_config = (uint32_t)parameter_value.integer;
	TS_LOG_INFO("%s: hw_bl_gidac_lsb_config:%d\n", __func__, hw_bl_gidac_lsb_config);

	/* Step 4a: Execute Panel Scan */
	ret = pip_execute_panel_scan();
	if (ret) {
		TS_LOG_ERR("%s:Unable to execute panel scan!\n", __func__);
		goto restore_multi_tx;
	}

	/* Step 4b: Retrieve Panel Scan raw data */
	ret = retrieve_panel_raw_data(MUTUAL_CAP_RAW_DATA_ID, 0,
				      sensor_element_num, &data_format,
				      *sensor_raw_data);
	if (ret) {
		TS_LOG_ERR("%s:Unable to retrieve panel raw data!\n", __func__);
		goto restore_multi_tx;
	}
#ifdef PRINT_CAP_DATA_DEBUG
	TS_LOG_INFO("%s: sensor_raw_data[0..%d]:\n", __func__,
		    sensor_element_num - 1);
	for (i = 0; i < tx_num; i++) {
		for (j = 0; j < rx_num; j++) {
			TS_LOG_INFO("sen[%d][%d] = %5d ", i, j, (*sensor_raw_data)[i * rx_num + j]);
		}
		TS_LOG_INFO("\n");
	}
#endif
	
	/* Step 5 and 6: Calculate Cm_sensor and Cm_sensor_ave */
	*cm_sensor_average = 0;
	TS_LOG_INFO("%s: calculate_cm_gen6l, sensor_element_num = %d\n", __func__, sensor_element_num);
	for (i = 0; i < sensor_element_num; i++) {
		(*cm_sensor_data)[i] = calculate_cm_gen6l((*sensor_raw_data)[i],
							 tx_period_mutual,
							 balancing_target_mutual,
							 scaling_factor_mutual,
							 idac_lsb, idac_mutual,
							 rx_atten_mutual,
							 gidac_mult, clk, vtx,
							 mtx_sum, 
							 hw_bl_gidac_lsb_config, 
							 hw_bl_gidac_mutual, hw_bl_pwc_mutual);
		cm_sensor_average_bak += (*cm_sensor_data)[i];
	}
	*cm_sensor_average = cm_sensor_average_bak / sensor_element_num;

#ifdef PRINT_CAP_DATA_DEBUG
	TS_LOG_INFO("%s: cm_sensor_data[0..%d]:\n", __func__,
		   sensor_element_num - 1);
	for (i = 0; i < tx_num; i++) {
		for (j = 0; j < rx_num; j++) {
			TS_LOG_INFO("%d ", (*cm_sensor_data)[i * rx_num + j]);
		}
		TS_LOG_INFO("\n");
	}
#endif
	TS_LOG_INFO("%s: cm_sensor_average: %d\n", __func__,
		    *cm_sensor_average);

restore_multi_tx:
	TS_LOG_INFO("%s: Set FORCE_SINGLE_TX to 0\n", __func__);

	/* Step 13: Set force single TX */
	ret = pip_set_parameter(FORCE_SINGLE_TX, 1, 0x00);
	if (ret) {
		TS_LOG_ERR("%s:Unable to set FORCE_SINGLE_TX parameter!\n",
			   __func__);
		goto exit;
	}

	TS_LOG_INFO("%s: Perform calibrate IDACs\n", __func__);

	/* Step 14: Perform calibration */
	ret = pip_calibrate_idacs(0);
	if (ret) {
		TS_LOG_ERR("%s:Unable to calibrate mutual IDACs!\n", __func__);
		goto exit;
	}
	if (button_num > 0) {
		ret = pip_calibrate_idacs(1);
		if (ret) {
			TS_LOG_ERR("%s:Unable to calibrate button IDACs!\n",
				   __func__);
			goto exit;
		}
	}

exit:
	if (ret) {
		kfree(*cm_sensor_data);
		kfree(*sensor_raw_data);
		*cm_sensor_data = NULL;
		*sensor_raw_data = NULL;
	}
	j = 0;
	return ret;
}

static int calculate_cm_calibration_gen6(int tx_period_mutual,
					 int balancing_target_mutual,
					 int idac_lsb, int idac_mutual,
					 int rx_atten_mutual, int gidac_mult,
					 int clk, int vtx, int mtx_sum)
{
	int i_bias = gidac_mult * idac_lsb * idac_mutual;
	int t_cal = tx_period_mutual * balancing_target_mutual / clk;

	return i_bias * t_cal * rx_atten_mutual / (vtx * mtx_sum * 1000);
}
static int calculate_cm_calibration_gen6l(int tx_period_mutual,
					 int balancing_target_mutual,
					 int idac_lsb, int idac_mutual,
					 int rx_atten_mutual, int gidac_mult,
					 int clk, int vtx, int mtx_sum,
					 int hw_bl_gidac_lsb_config, int hw_bl_gidac_mutual, int hw_bl_pwc_mutual)
{
	int i_bias = gidac_mult * idac_lsb * idac_mutual;
	long t_cal = tx_period_mutual * balancing_target_mutual * 100 /clk ;

	int cap =(int)( i_bias * t_cal * rx_atten_mutual / (vtx * mtx_sum) + CONVERSION_CONST *
	 	HW_BL_GIDAC_MULT*100l*(hw_bl_gidac_lsb_config+1)/(clk * vtx * mtx_sum)*
	 	hw_bl_gidac_mutual*hw_bl_pwc_mutual ) /(CONVERSION_CONST * 100l);

	return cap;
}

static int get_cm_calibration_check_test_results_gen6(int vdda,
						      bool skip_cm_button,
						      int
						      *cm_sensor_calibration)
{
	union parameter_value parameter_value;
	enum parameter_type parameter_type;
	uint16_t read_length;
	uint8_t data_format;
	int8_t idac_mutual;
	int rx_atten_mutual;
	int mtx_sum;
	char *vdda_mode;
	uint32_t tx_period_mutual;
	uint32_t mtx_order;
	uint32_t balancing_target_mutual;
	uint32_t gidac_mult;
	int vtx;
	int idac_lsb = IDAC_LSB_DEFAULT;
	int clk = CLK_DEFAULT;
	uint8_t data[IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_LENGTH];
	int ret;

	TS_LOG_INFO("%s: Get Mutual IDAC and RX Attenuator values\n", __func__);

	/* Step 1: Get Mutual IDAC and RX Attenuator values */
	ret = pip_retrieve_data_structure(0,
					  IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_LENGTH,
					  IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_ID,
					  &read_length, &data_format, data);
	if (ret) {
		TS_LOG_ERR("%s:Unable to retrieve data structure!\n", __func__);
		goto exit;
	}

	ret = rx_attenuator_lookup(data[RX_ATTENUATOR_MUTUAL_INDEX],
				   &rx_atten_mutual);
	if (ret) {
		TS_LOG_ERR("%s:Invalid RX Attenuator Index!\n", __func__);
		goto exit;
	}

	idac_mutual = (int8_t) data[IDAC_MUTUAL_INDEX];

	TS_LOG_INFO("%s: idac_mutual: %d\n", __func__, idac_mutual);
	TS_LOG_INFO("%s: rx_atten_mutual: %d\n", __func__, rx_atten_mutual);

	/* Get CDC:VDDA_MODE */
	ret = get_configuration_parameter(VDDA_MODE, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != STRING) {
		TS_LOG_ERR("%s:Unable to get vdda_mode!\n", __func__);
		goto exit;
	}

	vdda_mode = parameter_value.string;
	TS_LOG_INFO("%s: vdda_mode: %s\n", __func__, vdda_mode);

	if (!strcmp(vdda_mode, VDDA_MODE_BYPASS)) {
		if (vdda != 0)
			vtx = vdda;
		else {
			TS_LOG_ERR
			    ("%s:VDDA cannot be zero when VDDA_MODE is bypass!\n",
			     __func__);
			ret = -EINVAL;
			goto exit;
		}
	} else if (!strcmp(vdda_mode, VDDA_MODE_PUMP)) {
		/* Get CDC:TX_PUMP_VOLTAGE */
		ret = get_configuration_parameter(TX_PUMP_VOLTAGE,
						  &parameter_type,
						  &parameter_value);
		if (ret || parameter_type != FLOAT) {
			TS_LOG_ERR("%s:Unable to get tx_pump_voltage!\n",
				   __func__);
			goto exit;
		}
		vtx = parameter_value.flt;
	} else {
		TS_LOG_ERR("%s:Invalid VDDA_MODE: %s!\n", __func__, vdda_mode);
		ret = -EINVAL;
		goto exit;
	}

	TS_LOG_INFO("%s: vtx: %d\n", __func__, vtx);

	/* Get CDC:TX_PERIOD_MUTUAL */
	ret = get_configuration_parameter(TX_PERIOD_MUTUAL, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get tx_period_mutual!\n", __func__);
		goto exit;
	}

	tx_period_mutual = parameter_value.integer;
	TS_LOG_INFO("%s: tx_period_mutual: %d\n", __func__, tx_period_mutual);

	/* Get CDC:MTX_ORDER */
	ret = get_configuration_parameter(MTX_ORDER, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get mtx_order!\n", __func__);
		goto exit;
	}

	mtx_order = parameter_value.integer;
	TS_LOG_INFO("%s: mtx_order: %d\n", __func__, mtx_order);

	ret = mtx_sum_lookup(mtx_order, &mtx_sum);
	if (ret) {
		TS_LOG_ERR("%s:Invalid MTX Order Index!\n", __func__);
		goto exit;
	}

	TS_LOG_INFO("%s: mtx_sum: %d\n", __func__, mtx_sum);

	/* Get Calibration:BALANCING_TARGET_MUTUAL */
	ret = get_configuration_parameter(BALANCING_TARGET_MUTUAL,
					  &parameter_type, &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get balancing_target_mutual!\n",
			   __func__);
		goto exit;
	}

	balancing_target_mutual = parameter_value.integer;
	TS_LOG_INFO("%s: balancing_target_mutual: %d\n", __func__,
		    balancing_target_mutual);

	/* Get CDC:INFRA_CTRL:GIDAC_MULT */
	ret = get_configuration_parameter(GIDAC_MULT,
					  &parameter_type, &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get gidac_mult!\n", __func__);
		goto exit;
	}

	gidac_mult = parameter_value.integer;
	TS_LOG_INFO("%s: gidac_mult: %d\n", __func__, gidac_mult);

	*cm_sensor_calibration = calculate_cm_calibration_gen6(tx_period_mutual,
							       balancing_target_mutual,
							       idac_lsb,
							       idac_mutual,
							       rx_atten_mutual,
							       gidac_mult, clk,
							       vtx, mtx_sum);

	TS_LOG_INFO("%s: cm_sensor_calibration: %d\n", __func__,
		    *cm_sensor_calibration);

exit:

	return ret;
}

static int get_cm_calibration_check_test_results_gen6l(int vdda,
						      bool skip_cm_button,
						      int *cm_sensor_calibration,
						      int *cm_button_calibration)
{
	union parameter_value parameter_value;
	enum parameter_type parameter_type;
	uint16_t read_length;
	uint8_t data_format;
	int8_t idac_mutual;
	int8_t idac_mutual_btn;
	int rx_atten_mutual;
	int rx_atten_mutual_btn = 0;
	int mtx_sum;
	char *vdda_mode;
	uint32_t tx_period_mutual;
	uint32_t tx_period_btn_mutual;
	uint32_t mtx_order;
	uint32_t balancing_target_mutual;
	uint32_t balancing_target_btn_mutual;
	uint32_t gidac_mult;
	uint8_t hw_bl_gidac_lsb_config;
	uint8_t hw_bl_gidac_mutual;
	uint8_t hw_bl_pwc_mutual;
	int vtx;
	int idac_lsb = IDAC_LSB_DEFAULT;
	int clk = CLK_DEFAULT;
	uint8_t data[IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_LENGTH];
	uint8_t hw_bl_data[MUTUAL_HW_BASELINE_CALIBRATION_DATA_LENGTH] = {0};
	int ret;

	TS_LOG_INFO("%s: Get Mutual IDAC and RX Attenuator values\n", __func__);

	/* Step 1: Get Mutual IDAC and RX Attenuator values */
	ret = pip_retrieve_data_structure(0,
					  IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_LENGTH,
					  IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_ID,
					  &read_length, &data_format, data);
	if (ret) {
		TS_LOG_ERR("%s:Unable to retrieve data structure!\n", __func__);
		goto exit;
	}

	ret = rx_attenuator_lookup(data[RX_ATTENUATOR_MUTUAL_INDEX],
				   &rx_atten_mutual);
	if (ret) {
		TS_LOG_ERR("%s:Invalid RX Attenuator Index!\n", __func__);
		goto exit;
	}

	idac_mutual = (int8_t) data[IDAC_MUTUAL_INDEX];

	TS_LOG_INFO("%s: idac_mutual: %d\n", __func__, idac_mutual);
	TS_LOG_INFO("%s: rx_atten_mutual: %d\n", __func__, rx_atten_mutual);

	/* Get Mutual HW Baseline calibration data */
	ret = pip_retrieve_data_structure(0,
			MUTUAL_HW_BASELINE_CALIBRATION_DATA_LENGTH,
			MUTUAL_HW_BASELINE_CALIBRATION_DATA_ID,
			&read_length, &data_format, hw_bl_data);
	if (ret) {
		TS_LOG_ERR("%s:Unable to retrieve data structure!\n", __func__);
		goto exit;
	}

	hw_bl_gidac_mutual = hw_bl_data[0];
	hw_bl_pwc_mutual = hw_bl_data[1];

	TS_LOG_INFO("%s: hw_bl_gidac_mutual: %d\n", __func__, hw_bl_gidac_mutual);
	TS_LOG_INFO("%s: hw_bl_pwc_mutual: %d\n", __func__, hw_bl_pwc_mutual);

	/* Get CDC:VDDA_MODE */
	ret = get_configuration_parameter(VDDA_MODE, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != STRING) {
		TS_LOG_ERR("%s:Unable to get vdda_mode!\n", __func__);
		goto exit;
	}

	vdda_mode = parameter_value.string;
	TS_LOG_INFO("%s: vdda_mode: %s\n", __func__, vdda_mode);

	if (!strcmp(vdda_mode, VDDA_MODE_BYPASS)) {
		if (vdda != 0)
			vtx = vdda;
		else {
			TS_LOG_ERR
			    ("%s:VDDA cannot be zero when VDDA_MODE is bypass!\n",
			     __func__);
			ret = -EINVAL;
			goto exit;
		}
	} else if (!strcmp(vdda_mode, VDDA_MODE_PUMP)) {
		/* Get CDC:TX_PUMP_VOLTAGE */
		ret = get_configuration_parameter(TX_PUMP_VOLTAGE,
						  &parameter_type,
						  &parameter_value);
		if (ret || parameter_type != FLOAT) {
			TS_LOG_ERR("%s:Unable to get tx_pump_voltage!\n",
				   __func__);
			goto exit;
		}
		vtx = parameter_value.flt;
	} else {
		TS_LOG_ERR("%s:Invalid VDDA_MODE: %s!\n", __func__, vdda_mode);
		ret = -EINVAL;
		goto exit;
	}

	TS_LOG_INFO("%s: vtx: %d\n", __func__, vtx);

	/* Get CDC:TX_PERIOD_MUTUAL */
	ret = get_configuration_parameter(TX_PERIOD_MUTUAL, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get tx_period_mutual!\n", __func__);
		goto exit;
	}

	tx_period_mutual = parameter_value.integer;
	TS_LOG_INFO("%s: tx_period_mutual: %d\n", __func__, tx_period_mutual);

	/* Get CDC:MTX_ORDER */
	ret = get_configuration_parameter(MTX_ORDER, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get mtx_order!\n", __func__);
		goto exit;
	}

	mtx_order = parameter_value.integer;
	TS_LOG_INFO("%s: mtx_order: %d\n", __func__, mtx_order);

	ret = mtx_sum_lookup(mtx_order, &mtx_sum);
	if (ret) {
		TS_LOG_ERR("%s:Invalid MTX Order Index!\n", __func__);
		goto exit;
	}

	TS_LOG_INFO("%s: mtx_sum: %d\n", __func__, mtx_sum);

	/* Get Calibration:BALANCING_TARGET_MUTUAL */
	ret = get_configuration_parameter(BALANCING_TARGET_MUTUAL,
					  &parameter_type, &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get balancing_target_mutual!\n",
			   __func__);
		goto exit;
	}

	balancing_target_mutual = parameter_value.integer;
	TS_LOG_INFO("%s: balancing_target_mutual: %d\n", __func__,
		    balancing_target_mutual);

	/* Get CDC:INFRA_CTRL:GIDAC_MULT */
	ret = get_configuration_parameter(GIDAC_MULT,
					  &parameter_type, &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get gidac_mult!\n", __func__);
		goto exit;
	}

	gidac_mult = parameter_value.integer;
	TS_LOG_INFO("%s: gidac_mult: %d\n", __func__, gidac_mult);

	/* Get CDC: HW_BL_GIDAC_LSB_CONFIG*/
	ret = get_configuration_parameter(HW_BL_GIDAC_LSB_CONFIG,
				&parameter_type, &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get hw_bl_gidac_lsb_config!\n", __func__);
		goto exit;
	}
	hw_bl_gidac_lsb_config = parameter_value.integer;
	TS_LOG_INFO("%s: hw_bl_gidac_lsb_config:%d\n", __func__, hw_bl_gidac_lsb_config);

	*cm_sensor_calibration = calculate_cm_calibration_gen6l(tx_period_mutual,
							       balancing_target_mutual,
							       idac_lsb, idac_mutual,
							       rx_atten_mutual, gidac_mult, clk,
							       vtx, mtx_sum, hw_bl_gidac_lsb_config,
							       hw_bl_gidac_mutual, hw_bl_pwc_mutual);

	TS_LOG_INFO("%s: cm_sensor_calibration: %d\n", __func__,
		    *cm_sensor_calibration);
	
exit:
	return ret;
}

static int validate_cm_test_results_gen6(struct configuration *configuration,
					 struct result *result, uint32_t tx_num,
					 uint32_t rx_num, uint32_t button_num,
					 bool skip_cm_button,
					 int *cm_sensor_data,
					 int cm_sensor_calibration,
					 int cm_sensor_average,
					 int cm_sensor_delta,
					 int **cm_sensor_column_delta,
					 int **cm_sensor_row_delta, bool *pass)
{
	uint32_t sensor_num = tx_num * rx_num;
	uint32_t configuration_sensor_num;
	uint32_t configuration_button_num;
	int ret = 0;
	int i, j;

	*cm_sensor_column_delta =
	    kzalloc(rx_num * tx_num * sizeof(int), GFP_KERNEL);
	*cm_sensor_row_delta =
	    kzalloc(tx_num * rx_num * sizeof(int), GFP_KERNEL);
	if (!*cm_sensor_column_delta || !*cm_sensor_row_delta) {
		ret = -ENOMEM;
		goto exit;
	}

	configuration_sensor_num =
	    configuration->cm_min_max_table_sensor_size / 2;
	configuration_button_num =
	    configuration->cm_min_max_table_button_size / 2;

	if (configuration_sensor_num != sensor_num) {
		TS_LOG_ERR
		    ("%s: Configuration and Device number of sensors mismatch! (Configuration:%d, Device:%d)\n",
		     __func__, configuration_sensor_num, sensor_num);
		ret = -EINVAL;
		goto exit;
	}

	if (!skip_cm_button && (button_num != configuration_button_num)) {
		TS_LOG_ERR
		    ("%s: Configuration and Device number of buttons mismatch! (Configuration:%d, Device:%d)\n",
		     __func__, configuration_button_num, button_num);
		ret = -EINVAL;
		goto exit;
	}

	/* Check each sensor Cm data for min/max values */
	result->cm_sensor_validation_pass = true;
	for (i = 0; i < sensor_num; i++) {
		int row = i % rx_num;
		int col = i / rx_num;
		int cm_sensor_min =
		    configuration->
		    cm_min_max_table_sensor[(row * tx_num + col) * 2];
		int cm_sensor_max =
		    configuration->
		    cm_min_max_table_sensor[(row * tx_num + col) * 2 + 1];
		if ((cm_sensor_data[i] < cm_sensor_min)
		    || (cm_sensor_data[i] > cm_sensor_max)) {
			TS_LOG_ERR("%s: Sensor[%d,%d]:%d (%d,%d)\n",
				   "Cm sensor min/max test",
				   row, col,
				   (int)cm_sensor_data[i],
				   cm_sensor_min, cm_sensor_max);
			result->cm_sensor_validation_pass = false;
		}
	}

	/* Check each row Cm data with neighbor for difference */
	result->cm_sensor_row_delta_pass = true;
	for (i = 0; i < tx_num; i++) {
		for (j = 1; j < rx_num; j++) {
			int cm_sensor_row_diff =
			    ABS((int)cm_sensor_data[i * rx_num + j] -
				(int)cm_sensor_data[i * rx_num + j - 1]);
			(*cm_sensor_row_delta)[i * rx_num + j - 1] =
			    cm_sensor_row_diff;
			if (cm_sensor_row_diff
			    > configuration->cm_range_limit_row) {
				TS_LOG_ERR("%s: Sensor[%d,%d]:%d (%d)\n",
					   "Cm sensor row range limit test",
					   j, i,
					   cm_sensor_row_diff,
					   configuration->cm_range_limit_row);
				result->cm_sensor_row_delta_pass = false;
			}
		}
	}

	/* Check each column Cm data with neighbor for difference */
	result->cm_sensor_col_delta_pass = true;
	for (i = 1; i < tx_num; i++) {
		for (j = 0; j < rx_num; j++) {
			int cm_sensor_col_diff =
			    ABS((int)cm_sensor_data[i * rx_num + j] -
				(int)cm_sensor_data[(i - 1) * rx_num + j]);
			(*cm_sensor_column_delta)[(i - 1) * rx_num + j] =
			    cm_sensor_col_diff;
			if (cm_sensor_col_diff
			    > configuration->cm_range_limit_col) {
				TS_LOG_ERR("%s: Sensor[%d,%d]:%d (%d)\n",
					   "Cm sensor column range limit test",
					   j, i,
					   cm_sensor_col_diff,
					   configuration->cm_range_limit_col);
				result->cm_sensor_col_delta_pass = false;
			}
		}
	}

	/* Check sensor calculated Cm for min/max values */
	result->cm_sensor_calibration_pass = true;
	if (cm_sensor_calibration < configuration->cm_min_limit_cal
	    || cm_sensor_calibration > configuration->cm_max_limit_cal) {
		TS_LOG_ERR("%s: Cm_cal:%d (%d,%d)\n",
			   "Cm sensor Cm_cal min/max test",
			   cm_sensor_calibration,
			   configuration->cm_min_limit_cal,
			   configuration->cm_max_limit_cal);
		result->cm_sensor_calibration_pass = false;
	}

	/* Check sensor Cm delta for range limit */
	result->cm_sensor_delta_pass = true;
	if (cm_sensor_delta > configuration->cm_max_delta_sensor_percent) {
		TS_LOG_ERR("%s: Cm_sensor_delta:%d (%d)\n",
			   "Cm sensor delta range limit test",
			   cm_sensor_delta,
			   configuration->cm_max_delta_sensor_percent);
		result->cm_sensor_delta_pass = false;
	}

	result->cm_test_pass = result->cm_sensor_validation_pass
	    & result->cm_sensor_row_delta_pass
	    & result->cm_sensor_col_delta_pass
	    & result->cm_sensor_calibration_pass & result->cm_sensor_delta_pass;
	TS_LOG_INFO("%s: cm_test_pass=%d \n", __func__, result->cm_test_pass);
exit:
	if (ret) {
		kfree(*cm_sensor_row_delta);
		kfree(*cm_sensor_column_delta);
		*cm_sensor_row_delta = NULL;
		*cm_sensor_column_delta = NULL;
	} else if (pass)
		*pass = result->cm_test_pass;
	return ret;
}

static int validate_cm_test_results_gen6l(struct configuration *configuration,
					 struct result *result, uint32_t tx_num,
					 uint32_t rx_num, uint32_t button_num,
					 bool skip_cm_button,
					 int *cm_sensor_data, int *cm_button_data,
					 int cm_sensor_calibration, int cm_button_calibration,
					 int cm_sensor_average, int cm_button_average,
					 int cm_sensor_delta, int cm_button_delta,
					 int **cm_sensor_column_delta,
					 int **cm_sensor_row_delta, bool *pass)
{
	uint32_t sensor_num = tx_num * rx_num;
	uint32_t configuration_sensor_num;
	uint32_t configuration_button_num;
	int ret = 0;
	int i, j;

	*cm_sensor_column_delta =
	    kzalloc(rx_num * tx_num * sizeof(int), GFP_KERNEL);
	*cm_sensor_row_delta =
	    kzalloc(tx_num * rx_num * sizeof(int), GFP_KERNEL);
	if (!*cm_sensor_column_delta || !*cm_sensor_row_delta) {
		ret = -ENOMEM;
		goto exit;
	}

	configuration_sensor_num =
	    configuration->cm_min_max_table_sensor_size / 2;
	configuration_button_num =
	    configuration->cm_min_max_table_button_size / 2;

	if (configuration_sensor_num != sensor_num) {
		TS_LOG_ERR
		    ("%s: Configuration and Device number of sensors mismatch! (Configuration:%d, Device:%d)\n",
		     __func__, configuration_sensor_num, sensor_num);
		ret = -EINVAL;
		goto exit;
	}

	if (!skip_cm_button && (button_num != configuration_button_num)) {
		TS_LOG_ERR
		    ("%s: Configuration and Device number of buttons mismatch! (Configuration:%d, Device:%d)\n",
		     __func__, configuration_button_num, button_num);
		ret = -EINVAL;
		goto exit;
	}

	/* Check each sensor Cm data for min/max values */
	result->cm_sensor_validation_pass = true;
	for (i = 0; i < sensor_num; i++) {
		int row = i % rx_num;
		int col = i / rx_num;
		int cm_sensor_min =
		    configuration->cm_min_max_table_sensor[(row * tx_num + col) * 2];
		int cm_sensor_max =
		    configuration->cm_min_max_table_sensor[(row * tx_num + col) * 2 + 1];
		if ((cm_sensor_data[i] < cm_sensor_min)
		    || (cm_sensor_data[i] > cm_sensor_max)) {
			TS_LOG_ERR("%s: Sensor[%d,%d]:%d (%d,%d)\n",
				   "Cm sensor min/max test",
				   row, col, (int)cm_sensor_data[i],
				   cm_sensor_min, cm_sensor_max);
			result->cm_sensor_validation_pass = false;
		}
	}

	/* Check each row Cm data with neighbor for difference */
	result->cm_sensor_row_delta_pass = true;
	for (i = 0; i < tx_num; i++) {
		for (j = 1; j < rx_num; j++) {
			int cm_sensor_row_diff =
			    ABS((int)cm_sensor_data[i * rx_num + j] -
				(int)cm_sensor_data[i * rx_num + j - 1]);
			(*cm_sensor_row_delta)[i * rx_num + j - 1] =
			    cm_sensor_row_diff;
			if (cm_sensor_row_diff
			    > configuration->cm_range_limit_row) {
				TS_LOG_ERR("%s: Sensor[%d,%d]:%d (%d)\n",
					   "Cm sensor row range limit test",
					   j, i, cm_sensor_row_diff,
					   configuration->cm_range_limit_row);
				result->cm_sensor_row_delta_pass = false;
			}
		}
	}

	/* Check each column Cm data with neighbor for difference */
	result->cm_sensor_col_delta_pass = true;
	for (i = 1; i < tx_num; i++) {
		for (j = 0; j < rx_num; j++) {
			int cm_sensor_col_diff =
			    ABS((int)cm_sensor_data[i * rx_num + j] -
				(int)cm_sensor_data[(i - 1) * rx_num + j]);
			(*cm_sensor_column_delta)[(i - 1) * rx_num + j] =
			    cm_sensor_col_diff;
			if (cm_sensor_col_diff
			    > configuration->cm_range_limit_col) {
				TS_LOG_ERR("%s: Sensor[%d,%d]:%d (%d)\n",
					   "Cm sensor column range limit test",
					   j, i, cm_sensor_col_diff,
					   configuration->cm_range_limit_col);
				result->cm_sensor_col_delta_pass = false;
			}
		}
	}

	/* Check sensor calculated Cm for min/max values */
	result->cm_sensor_calibration_pass = true;
	if (cm_sensor_calibration < configuration->cm_min_limit_cal
	    || cm_sensor_calibration > configuration->cm_max_limit_cal) {
		TS_LOG_ERR("%s: Cm_sensor_calibration:%d (%d,%d)\n",
			   "Cm sensor Cm_cal min/max test",
			   cm_sensor_calibration,
			   configuration->cm_min_limit_cal,
			   configuration->cm_max_limit_cal);
		result->cm_sensor_calibration_pass = false;
	}

	/* Check sensor Cm delta for range limit */
	result->cm_sensor_delta_pass = true;
	if (cm_sensor_delta > configuration->cm_max_delta_sensor_percent) {
		TS_LOG_ERR("%s: Cm_sensor_delta:%d (%d)\n",
			   "Cm sensor delta range limit test",
			   cm_sensor_delta,
			   configuration->cm_max_delta_sensor_percent);
		result->cm_sensor_delta_pass = false;
	}

	result->cm_test_pass = result->cm_sensor_validation_pass
	    & result->cm_sensor_row_delta_pass
	    & result->cm_sensor_col_delta_pass
	    & result->cm_sensor_calibration_pass & result->cm_sensor_delta_pass;
	TS_LOG_INFO("cm_test_pass=%d \n", result->cm_test_pass);
exit:
	if (ret) {
		kfree(*cm_sensor_row_delta);
		kfree(*cm_sensor_column_delta);
		*cm_sensor_row_delta = NULL;
		*cm_sensor_column_delta = NULL;
	} else if (pass)
		*pass = result->cm_test_pass;
	return ret;
}

static int calculate_cp_calibration_gen6(int tx_period_self,
					 int balancing_target_self,
					 int idac_lsb, int idac_self,
					 int rx_atten_self, int gidac_mult,
					 int clk, int v_swing)
{
	int i_bias = gidac_mult * idac_lsb * idac_self;
	int t_cal =
	    tx_period_self * balancing_target_self * CONVERSION_CONST / (1000 *
									 clk);

	return i_bias * t_cal * rx_atten_self / v_swing;
}

static int calculate_cp_calibration_gen6l(int tx_period_self,
					 int balancing_target_self, int idac_lsb, int idac_self,
					 int rx_atten_self, int gidac_mult, int clk,
					 int v_swing, int hw_bl_gidac_lsb_config,
					 int hw_bl_gidac_self, int hw_bl_pwc_self)
{
	int i_bias = gidac_mult * idac_lsb * idac_self;
	long t_cal = tx_period_self * balancing_target_self * 100 / clk ;

	int cap = (int)(i_bias * t_cal * rx_atten_self / v_swing + HW_BL_GIDAC_MULT* 100l * 
		CONVERSION_CONST *(hw_bl_gidac_lsb_config+1)/(clk * v_swing)*hw_bl_gidac_self*
		hw_bl_pwc_self ) /100 ;
	
	return cap;
}

static int calculate_cp_gen6(int raw_data, int tx_period_self,
			     int balancing_target_self, int scaling_factor_self,
			     int idac_lsb, int idac_self, int rx_atten_self,
			     int gidac_mult, int clk, int v_swing)
{
	int i_bias = gidac_mult * idac_lsb * idac_self;
	int adc = tx_period_self * balancing_target_self / 100 +
	    500 * raw_data / (scaling_factor_self * idac_self * rx_atten_self);

	return i_bias * adc * rx_atten_self / (clk * v_swing / 100);
}

static int calculate_cp_gen6l(int raw_data, int tx_period_self,
			     int balancing_target_self, int scaling_factor_self,
			     int idac_lsb, int idac_self, int rx_atten_self,
			     int gidac_mult, int clk, int v_swing,
			     int hw_bl_gidac_lsb_config, int hw_bl_gidac_self, int hw_bl_gidac_pwc_self)
{
	int i_bias = gidac_mult * idac_lsb * idac_self;
	long adc = tx_period_self * balancing_target_self  +50* 
		CONVERSION_CONST * raw_data / (scaling_factor_self * idac_self *
						rx_atten_self);

	int cap = (int)(i_bias * adc * rx_atten_self * 100l/ (clk * v_swing) + CONVERSION_CONST*
			100*HW_BL_GIDAC_MULT* (hw_bl_gidac_lsb_config+1)/(clk * v_swing)*
			hw_bl_gidac_self*hw_bl_gidac_pwc_self ) / 100;

	return cap;
}

static int get_cp_calibration_check_test_results_gen6(uint16_t tx_num,
						      uint16_t rx_num,
						      uint16_t button_num,
						      bool skip_cp_sensor,
						      bool skip_cp_button,
						      int32_t **
						      sensor_tx_raw_data,
						      int32_t **
						      sensor_rx_raw_data,
						      int **cp_sensor_tx_data,
						      int **cp_sensor_rx_data,
						      int *cp_sensor_tx_average,
						      int *cp_sensor_rx_average,
						      int
						      *cp_sensor_rx_calibration,
						      int
						      *cp_sensor_tx_calibration)
{
	union parameter_value parameter_value;
	enum parameter_type parameter_type;
	uint16_t read_length;
	uint8_t data_format;
	int8_t idac_self_rx;
	int8_t idac_self_tx;
	uint8_t rxdac;
	uint8_t ref_scale;
	int rx_atten_self_rx;
	int rx_atten_self_tx;
	int vref_low;
	int vref_mid;
	int vref_high;
	int v_swing;
	uint32_t tx_period_self;
	uint32_t scaling_factor_self;
	uint32_t balancing_target_self;
	uint32_t gidac_mult;
	int idac_lsb = IDAC_LSB_DEFAULT;
	int clk = CLK_DEFAULT;
	uint8_t data[IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_LENGTH];
	int ret;
	int i;

	/* Get CDC:REFGEN_CTL:RXDAC */
	ret = get_configuration_parameter(RXDAC, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get rxdac!\n", __func__);
		goto exit;
	}

	rxdac = parameter_value.integer;
	TS_LOG_INFO("%s: rxdac: %d\n", __func__, rxdac);

	ret = get_configuration_parameter(REF_SCALE, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get rxdac!\n", __func__);
		goto exit;
	}

	ref_scale = parameter_value.integer;
	TS_LOG_INFO("%s: ref_scale: %d\n", __func__, ref_scale);

	ret = selfcap_signal_swing_lookup(ref_scale, rxdac,
					  &vref_low, &vref_mid, &vref_high);
	if (ret) {
		TS_LOG_ERR("%s:Invalid ref_scale or rxdac!\n", __func__);
		goto exit;
	}

	TS_LOG_INFO("%s: vref_low: %d\n", __func__, vref_low);
	TS_LOG_INFO("%s: vref_mid: %d\n", __func__, vref_mid);
	TS_LOG_INFO("%s: vref_high: %d\n", __func__, vref_high);

	v_swing = vref_high - vref_low;

	TS_LOG_INFO("%s: v_swing: %d\n", __func__, v_swing);

	/* Get CDC:INFRA_CTRL:GIDAC_MULT */
	ret = get_configuration_parameter(GIDAC_MULT, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get infra_ctrl!\n", __func__);
		goto exit;
	}

	gidac_mult = parameter_value.integer;
	TS_LOG_INFO("%s: gidac_mult: %d\n", __func__, gidac_mult);
	/* Step 1: Perform calibration */
	ret = pip_calibrate_idacs(0);
	if (ret) {
		TS_LOG_ERR("%s:Unable to calibrate self IDACs!\n", __func__);
		goto exit;
	}
	if (button_num > 0) {
		ret = pip_calibrate_idacs(1);
		if (ret) {
			TS_LOG_ERR("%s:Unable to calibrate button IDACs!\n",
				   __func__);
			goto exit;
		}
	}

	TS_LOG_INFO("%s: Get Self IDAC and RX Attenuator values\n", __func__);

	/* Step 2: Get Self IDAC and RX Attenuator Self RX values */
	ret = pip_retrieve_data_structure(0,
					  IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_LENGTH,
					  IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_ID,
					  &read_length, &data_format, data);
	if (ret) {
		TS_LOG_ERR("%s:Unable to retrieve data structure!\n", __func__);
		goto exit;
	}

	/* Step 4a: Execute Panel Scan */
	ret = pip_execute_panel_scan();
	if (ret) {
		TS_LOG_ERR("%s:Unable to execute panel scan!\n", __func__);
		goto exit;
	}

	if (skip_cp_sensor)
		goto process_cp_button;

	/* Allocate sensor rx and tx buffers */
	*sensor_tx_raw_data = kzalloc(tx_num * sizeof(int32_t), GFP_KERNEL);
	*sensor_rx_raw_data = kzalloc(rx_num * sizeof(int32_t), GFP_KERNEL);
	*cp_sensor_tx_data = kzalloc(tx_num * sizeof(int), GFP_KERNEL);
	*cp_sensor_rx_data = kzalloc(rx_num * sizeof(int), GFP_KERNEL);
	if (!*sensor_tx_raw_data || !*sensor_rx_raw_data
	    || !*cp_sensor_tx_data || !*cp_sensor_rx_data) {
		ret = -ENOMEM;
		goto free_buffers;
	}

	/* Step 4b: Retrieve Panel Scan raw data */
	ret = retrieve_panel_raw_data(SELF_CAP_RAW_DATA_ID, 0, rx_num,
				      &data_format, *sensor_rx_raw_data);
	if (ret) {
		TS_LOG_ERR("%s:Unable to retrieve panel raw data!\n", __func__);
		goto free_buffers;
	}

	ret = retrieve_panel_raw_data(SELF_CAP_RAW_DATA_ID, rx_num, tx_num,
				      &data_format, *sensor_tx_raw_data);
	if (ret) {
		TS_LOG_ERR("%s:Unable to retrieve panel raw data!\n", __func__);
		goto free_buffers;
	}

	ret = rx_attenuator_lookup(data[RX_ATTENUATOR_SELF_RX_INDEX],
				   &rx_atten_self_rx);
	if (ret) {
		TS_LOG_ERR("%s:Invalid RX Attenuator Index!\n", __func__);
		goto free_buffers;
	}

	idac_self_rx = (int8_t) data[IDAC_SELF_RX_INDEX];

	ret = rx_attenuator_lookup(data[RX_ATTENUATOR_SELF_TX_INDEX],
				   &rx_atten_self_tx);
	if (ret) {
		TS_LOG_ERR("%s:Invalid RX Attenuator Index!\n", __func__);
		goto free_buffers;
	}

	idac_self_tx = (int8_t) data[IDAC_SELF_TX_INDEX];

	TS_LOG_INFO("%s: idac_self_rx: %d\n", __func__, idac_self_rx);
	TS_LOG_INFO("%s: rx_atten_self_rx: %d\n", __func__, rx_atten_self_rx);
	TS_LOG_INFO("%s: idac_self_tx: %d\n", __func__, idac_self_tx);
	TS_LOG_INFO("%s: rx_atten_self_tx: %d\n", __func__, rx_atten_self_tx);

	/* Get CDC:TX_PERIOD_SELF */
	ret = get_configuration_parameter(TX_PERIOD_SELF, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get tx_period_self!\n", __func__);
		goto free_buffers;
	}

	tx_period_self = parameter_value.integer;
	TS_LOG_INFO("%s: tx_period_self: %d\n", __func__, tx_period_self);

	/* Get CDC:SCALING_FACTOR_SELF */
	ret = get_configuration_parameter(SCALING_FACTOR_SELF, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get scaling_factor_self!\n", __func__);
		goto free_buffers;
	}

	scaling_factor_self = parameter_value.integer;
	TS_LOG_INFO("%s: scaling_factor_self: %d\n", __func__,
		    scaling_factor_self);

	/* Get Calibration:BALANCING_TARGET_SELF */
	ret = get_configuration_parameter(BALANCING_TARGET_SELF,
					  &parameter_type, &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get balancing_target_self!\n",
			   __func__);
		goto free_buffers;
	}

	balancing_target_self = parameter_value.integer;
	TS_LOG_INFO("%s: balancing_target_self: %d\n", __func__,
		    balancing_target_self);

	*cp_sensor_rx_calibration =
	    calculate_cp_calibration_gen6(tx_period_self, balancing_target_self,
					  idac_lsb, idac_self_rx,
					  rx_atten_self_rx, gidac_mult, clk,
					  v_swing);

	TS_LOG_INFO("%s: cp_sensor_rx_calibration: %d\n", __func__,
		    *cp_sensor_rx_calibration);

	*cp_sensor_tx_calibration =
	    calculate_cp_calibration_gen6(tx_period_self, balancing_target_self,
					  idac_lsb, idac_self_tx,
					  rx_atten_self_tx, gidac_mult, clk,
					  v_swing);

	TS_LOG_INFO("%s: cp_sensor_tx_calibration: %d\n", __func__,
		    *cp_sensor_tx_calibration);

#ifdef PRINT_CAP_DATA_DEBUG
	TS_LOG_INFO("%s: sensor_rx_raw_data[0..%d]:\n", __func__, rx_num - 1);
	for (i = 0; i < rx_num; i++) {
		TS_LOG_INFO("%5d ", (*sensor_rx_raw_data)[i]);
	}
	TS_LOG_INFO("\n");
#endif
	*cp_sensor_rx_average = 0;
	for (i = 0; i < rx_num; i++) {
		(*cp_sensor_rx_data)[i] =
		    calculate_cp_gen6((*sensor_rx_raw_data)[i], tx_period_self,
				      balancing_target_self,
				      scaling_factor_self, idac_lsb,
				      idac_self_rx, rx_atten_self_rx,
				      gidac_mult, clk, v_swing);
		*cp_sensor_rx_average += (*cp_sensor_rx_data)[i];
	}
	*cp_sensor_rx_average /= rx_num;
#ifdef PRINT_CAP_DATA_DEBUG
	TS_LOG_INFO("%s: cp_sensor_rx_data[0..%d]:\n", __func__, rx_num - 1);
	for (i = 0; i < rx_num; i++) {
		TS_LOG_INFO("%d ", (*cp_sensor_rx_data)[i]);
	}
	TS_LOG_INFO("\n");
#endif
	TS_LOG_INFO("%s: cp_sensor_rx_average: %d\n", __func__,
		    *cp_sensor_rx_average);
#ifdef PRINT_CAP_DATA_DEBUG
	TS_LOG_INFO("%s: sensor_tx_raw_data[0..%d]:\n", __func__, tx_num - 1);
	for (i = 0; i < tx_num; i++) {
		TS_LOG_INFO("%5d ", (*sensor_tx_raw_data)[i]);
	}

#endif
	*cp_sensor_tx_average = 0;
	for (i = 0; i < tx_num; i++) {
		(*cp_sensor_tx_data)[i] =
		    calculate_cp_gen6((*sensor_tx_raw_data)[i], tx_period_self,
				      balancing_target_self,
				      scaling_factor_self, idac_lsb,
				      idac_self_tx, rx_atten_self_tx,
				      gidac_mult, clk, v_swing);
		*cp_sensor_tx_average += (*cp_sensor_tx_data)[i];
	}
	*cp_sensor_tx_average /= tx_num;
#ifdef PRINT_CAP_DATA_DEBUG
	TS_LOG_INFO("%s: cp_sensor_tx_data[0..%d]:\n", __func__, tx_num - 1);
	for (i = 0; i < tx_num; i++) {
		TS_LOG_INFO("%d ", (*cp_sensor_tx_data)[i]);
	}
	TS_LOG_INFO("\n");
#endif
	TS_LOG_INFO("%s: cp_sensor_tx_average: %d\n", __func__,
		    *cp_sensor_tx_average);

process_cp_button:
	if (skip_cp_button)
		goto exit;
free_buffers:
	if (ret) {
		if (!skip_cp_sensor) {
			kfree(*sensor_rx_raw_data);
			kfree(*sensor_tx_raw_data);
			kfree(*cp_sensor_rx_data);
			kfree(*cp_sensor_tx_data);
			*sensor_rx_raw_data = NULL;
			*sensor_tx_raw_data = NULL;
			*cp_sensor_rx_data = NULL;
			*cp_sensor_tx_data = NULL;
		}
	}

exit:
	return ret;
}

static int get_cp_calibration_check_test_results_gen6l(uint16_t tx_num,
						      uint16_t rx_num, uint16_t button_num, bool skip_cp_sensor,
						      bool skip_cp_button, int32_t **sensor_tx_raw_data,
						      int32_t **sensor_rx_raw_data,
						      int32_t **button_raw_data,
						      int **cp_sensor_tx_data,
						      int **cp_sensor_rx_data,
						      int **cp_button_data,
						      int *cp_sensor_tx_average,
						      int *cp_sensor_rx_average,
						      int *cp_button_average,
						      int **cp_sensor_rx_calibration,
						      int **cp_sensor_tx_calibration,
						      int *cp_button_calibration)
{
	union parameter_value parameter_value;
	enum parameter_type parameter_type;
	uint16_t read_length;
	uint8_t data_format;
	int8_t idac_self_rx;
	int8_t idac_self_tx;
	int8_t idac_self_btn;
	uint8_t rxdac;
	uint8_t ref_scale;
	int rx_atten_self_rx;
	int rx_atten_self_tx;
	int rx_atten_self_btn;
	int vref_low;
	int vref_mid;
	int vref_high;
	int v_swing;
	uint32_t tx_period_self;
	uint32_t tx_period_btn_self = 0;
	uint32_t scaling_factor_self;
	uint32_t scaling_factor_btn_self = 0;
	uint32_t balancing_target_btn_self = 0;
	uint32_t balancing_target_self;
	uint32_t gidac_mult;
	int idac_lsb = IDAC_LSB_DEFAULT;
	int clk = CLK_DEFAULT;
	uint8_t data[IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_LENGTH];
	int ret;
	int i;
	uint8_t *hw_bl_calibration_self_data=NULL;
	uint8_t *hw_bl_pwc_self_rx=NULL, *hw_bl_pwc_self_tx=NULL;
	int hw_bl_gidac_lsb_config =0, hw_bl_calibration_self_data_length =0;
	uint8_t hw_bl_gidac_self_rx =0, hw_bl_gidac_self_tx = 0;
	char *scanning_mode_button = NULL;
	uint16_t btn_read_offset = 0;
	long long int cp_sensor_average_bak =0;
	
	/* Get CDC:REFGEN_CTL:RXDAC */
	ret = get_configuration_parameter(RXDAC, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get rxdac!\n", __func__);
		goto exit;
	}

	rxdac = parameter_value.integer;
	TS_LOG_INFO("%s: rxdac: %d\n", __func__, rxdac);

	ret = get_configuration_parameter(REF_SCALE, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get rxdac!\n", __func__);
		goto exit;
	}

	ref_scale = parameter_value.integer;
	TS_LOG_INFO("%s: ref_scale: %d\n", __func__, ref_scale);

	ret = selfcap_signal_swing_lookup(ref_scale, rxdac,
					  &vref_low, &vref_mid, &vref_high);
	if (ret) {
		TS_LOG_ERR("%s:Invalid ref_scale or rxdac!\n", __func__);
		goto exit;
	}

	TS_LOG_INFO("%s: vref_low: %d\n", __func__, vref_low);
	TS_LOG_INFO("%s: vref_mid: %d\n", __func__, vref_mid);
	TS_LOG_INFO("%s: vref_high: %d\n", __func__, vref_high);

	v_swing = vref_high - vref_low;

	TS_LOG_INFO("%s: v_swing: %d\n", __func__, v_swing);

	/* Get CDC:INFRA_CTRL:GIDAC_MULT */
	ret = get_configuration_parameter(GIDAC_MULT, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get infra_ctrl!\n", __func__);
		goto exit;
	}

	gidac_mult = parameter_value.integer;
	TS_LOG_INFO("%s: gidac_mult: %d\n", __func__, gidac_mult);

	/* Get CDC: HW_BL_GIDAC_LSB_CONFIG*/
	ret = get_configuration_parameter(HW_BL_GIDAC_LSB_CONFIG,
				&parameter_type, &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get gidac_lsb_config!\n",__func__);
		goto exit;
	}
	hw_bl_gidac_lsb_config = parameter_value.integer;
	TS_LOG_INFO("%s: hw_bl_gidac_lsb_config:%d\n", __func__, hw_bl_gidac_lsb_config);
	
	/* Step 1: Perform calibration */
	ret = pip_calibrate_idacs(2);
	if (ret) {
		TS_LOG_ERR("%s:Unable to calibrate self IDACs!\n", __func__);
		goto exit;
	}
	if (button_num > 0) {
		ret = pip_calibrate_idacs(1);
		if (ret) {
			TS_LOG_ERR("%s:Unable to calibrate button IDACs!\n",
				   __func__);
			goto exit;
		}
	}

	TS_LOG_INFO("%s: Get Self IDAC and RX Attenuator values\n", __func__);

	/* Step 2: Get Self IDAC and RX Attenuator Self RX values */
	ret = pip_retrieve_data_structure(0,
					  IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_LENGTH,
					  IDAC_AND_RX_ATTENUATOR_CALIBRATION_DATA_ID,
					  &read_length, &data_format, data);
	if (ret) {
		TS_LOG_ERR("%s:Unable to retrieve data structure!\n", __func__);
		goto exit;
	}

	hw_bl_calibration_self_data_length= tx_num+rx_num+2;
	hw_bl_calibration_self_data =(uint8_t *) kzalloc(hw_bl_calibration_self_data_length * sizeof(uint8_t), GFP_KERNEL);
	hw_bl_pwc_self_rx = (uint8_t *) kzalloc((rx_num) * sizeof(uint8_t), GFP_KERNEL);
	hw_bl_pwc_self_tx = (uint8_t *) kzalloc((tx_num) * sizeof(uint8_t), GFP_KERNEL);

	if (!hw_bl_calibration_self_data || !hw_bl_pwc_self_rx || !hw_bl_pwc_self_tx) {
		ret = -ENOMEM;
		goto exit;
	}

	ret = pip_retrieve_data_structure(0,
			hw_bl_calibration_self_data_length,
			SELF_HW_BASELINE_CALIBRATION_DATA_ID,
			&read_length, &data_format, hw_bl_calibration_self_data);
	if (ret) {
		TS_LOG_ERR( "%s:Unable to retrieve data structure!\n", __func__);
		goto exit;
	}

	hw_bl_gidac_self_rx = hw_bl_calibration_self_data[0];
	hw_bl_gidac_self_tx = hw_bl_calibration_self_data[1];	
	TS_LOG_INFO("%s: hw_bl_gidac_self_rx=%d, hw_bl_gidac_self_tx = %d\n", __func__,hw_bl_gidac_self_rx,hw_bl_gidac_self_tx);
	
	memcpy(hw_bl_pwc_self_rx, hw_bl_calibration_self_data+2, rx_num);	
	memcpy(hw_bl_pwc_self_tx, hw_bl_calibration_self_data+2+rx_num, tx_num);
	
#ifdef PRINT_CAP_DATA_DEBUG
	for(i=0; i<rx_num; i++)
		TS_LOG_INFO("%s: hw_bl_pwc_self_rx[%d]: %d\n", __func__, i, hw_bl_pwc_self_rx[i]);

	for(i=0; i<tx_num; i++)
		TS_LOG_INFO("%s: hw_bl_pwc_self_tx[%d]: %d\n", __func__, i, hw_bl_pwc_self_tx[i]);
#endif	

	/* Step 4a: Execute Panel Scan */
	ret = pip_execute_panel_scan();
	if (ret) {
		TS_LOG_ERR("%s:Unable to execute panel scan!\n", __func__);
		goto exit;
	}

	if (skip_cp_sensor)
		goto exit;

	/* Allocate sensor rx and tx buffers */
	*sensor_tx_raw_data = kzalloc(tx_num * sizeof(int32_t), GFP_KERNEL);
	*sensor_rx_raw_data = kzalloc(rx_num * sizeof(int32_t), GFP_KERNEL);
	*cp_sensor_tx_data = kzalloc(tx_num * sizeof(int), GFP_KERNEL);
	*cp_sensor_rx_data = kzalloc(rx_num * sizeof(int), GFP_KERNEL);

	*cp_sensor_rx_calibration = kzalloc(rx_num * sizeof(int), GFP_KERNEL);
	*cp_sensor_tx_calibration = kzalloc(tx_num * sizeof(int), GFP_KERNEL);
	if (!*sensor_tx_raw_data || !*sensor_rx_raw_data
	    || !*cp_sensor_tx_data || !*cp_sensor_rx_data
	    ||!*cp_sensor_rx_calibration || !*cp_sensor_tx_calibration) {
		ret = -ENOMEM;
		goto free_buffers;
	}

	/* Step 4b: Retrieve Panel Scan raw data */
	ret = retrieve_panel_raw_data(SELF_CAP_RAW_DATA_ID, 0, rx_num,
				      &data_format, *sensor_rx_raw_data);
	if (ret) {
		TS_LOG_ERR("%s:Unable to retrieve panel raw data!\n", __func__);
		goto free_buffers;
	}
	
	ret = retrieve_panel_raw_data(SELF_CAP_RAW_DATA_ID, rx_num, tx_num,
				      &data_format, *sensor_tx_raw_data);
	if (ret) {
		TS_LOG_ERR("%s:Unable to retrieve panel raw data!\n", __func__);
		goto free_buffers;
	}

	ret = rx_attenuator_lookup(data[RX_ATTENUATOR_SELF_RX_INDEX],
				   &rx_atten_self_rx);
	if (ret) {
		TS_LOG_ERR("%s:Invalid RX Attenuator Index!\n", __func__);
		goto free_buffers;
	}

	idac_self_rx = (int8_t) data[IDAC_SELF_RX_INDEX];

	ret = rx_attenuator_lookup(data[RX_ATTENUATOR_SELF_TX_INDEX],
				   &rx_atten_self_tx);
	if (ret) {
		TS_LOG_ERR("%s:Invalid RX Attenuator Index!\n", __func__);
		goto free_buffers;
	}

	idac_self_tx = (int8_t) data[IDAC_SELF_TX_INDEX];

	TS_LOG_INFO("%s: idac_self_rx: %d\n", __func__, idac_self_rx);
	TS_LOG_INFO("%s: rx_atten_self_rx: %d\n", __func__, rx_atten_self_rx);
	TS_LOG_INFO("%s: idac_self_tx: %d\n", __func__, idac_self_tx);
	TS_LOG_INFO("%s: rx_atten_self_tx: %d\n", __func__, rx_atten_self_tx);

	/* Get CDC:TX_PERIOD_SELF */
	ret = get_configuration_parameter(TX_PERIOD_SELF, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get tx_period_self!\n", __func__);
		goto free_buffers;
	}

	tx_period_self = parameter_value.integer;
	TS_LOG_INFO("%s: tx_period_self: %d\n", __func__, tx_period_self);

	/* Get CDC:SCALING_FACTOR_SELF */
	ret = get_configuration_parameter(SCALING_FACTOR_SELF, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get scaling_factor_self!\n", __func__);
		goto free_buffers;
	}

	scaling_factor_self = parameter_value.integer;
	TS_LOG_INFO("%s: scaling_factor_self: %d\n", __func__,
		    scaling_factor_self);

	/* Get Calibration:BALANCING_TARGET_SELF */
	ret = get_configuration_parameter(BALANCING_TARGET_SELF,
					  &parameter_type, &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get balancing_target_self!\n",
			   __func__);
		goto free_buffers;
	}

	balancing_target_self = parameter_value.integer;
	TS_LOG_INFO("%s: balancing_target_self: %d\n", __func__,
		    balancing_target_self);

	for (i = 0; i < rx_num; i++) {
		(*cp_sensor_rx_calibration)[i] = calculate_cp_calibration_gen6l(tx_period_self,
				balancing_target_self, idac_lsb, idac_self_rx,
				rx_atten_self_rx, gidac_mult, clk, v_swing, 
				hw_bl_gidac_lsb_config, hw_bl_gidac_self_rx, hw_bl_pwc_self_rx[i]);
	}

	for (i = 0; i < tx_num; i++) {
		(*cp_sensor_tx_calibration)[i] = calculate_cp_calibration_gen6l(tx_period_self,
				balancing_target_self, idac_lsb, idac_self_tx,
				rx_atten_self_tx, gidac_mult, clk, v_swing, 
				hw_bl_gidac_lsb_config, hw_bl_gidac_self_tx, hw_bl_pwc_self_tx[i]);
	}
	
#ifdef PRINT_CAP_DATA_DEBUG
	TS_LOG_INFO("%s: cp_sensor_rx_calibration[0..%d]:\n", __func__, rx_num - 1);
	for (i = 0; i < rx_num; i++){
		TS_LOG_INFO("%5d  ", hw_bl_pwc_self_rx[i]);
		TS_LOG_INFO("%5d\n", (*cp_sensor_rx_calibration)[i]);
	}
	TS_LOG_INFO("\n");

	TS_LOG_INFO("%s: cp_sensor_tx_calibration[0..%d]:\n", __func__, tx_num - 1);
	for (i = 0; i < tx_num; i++){
		TS_LOG_INFO("%5d  ", hw_bl_pwc_self_tx[i]);
		TS_LOG_INFO("%5d\n", (*cp_sensor_tx_calibration)[i]);
	}
	TS_LOG_INFO("\n");
#endif

	*cp_sensor_rx_average = 0;
	for (i = 0; i < rx_num; i++) {
		(*cp_sensor_rx_data)[i] = calculate_cp_gen6l((*sensor_rx_raw_data)[i],
				tx_period_self, balancing_target_self,
				scaling_factor_self, idac_lsb, idac_self_rx,
				rx_atten_self_rx, gidac_mult, clk, v_swing, 
				hw_bl_gidac_lsb_config, hw_bl_gidac_self_rx, hw_bl_pwc_self_rx[i]);
	
		cp_sensor_average_bak += (*cp_sensor_rx_data)[i];
	}
	*cp_sensor_rx_average = cp_sensor_average_bak / rx_num;
	
	*cp_sensor_tx_average = 0;
	cp_sensor_average_bak = 0;
	for (i = 0; i < tx_num; i++) {
		(*cp_sensor_tx_data)[i] = calculate_cp_gen6l((*sensor_tx_raw_data)[i],
				tx_period_self, balancing_target_self,
				scaling_factor_self, idac_lsb, idac_self_tx,
				rx_atten_self_tx, gidac_mult, clk, v_swing, 
				hw_bl_gidac_lsb_config, hw_bl_gidac_self_tx, hw_bl_pwc_self_tx[i]);

		cp_sensor_average_bak += (*cp_sensor_tx_data)[i];
	}
	*cp_sensor_tx_average = cp_sensor_average_bak / tx_num;
#ifdef PRINT_CAP_DATA_DEBUG
	TS_LOG_INFO("%s: sensor_rx_raw_data[0..%d]:\n", __func__, rx_num - 1);
	for (i = 0; i < tx_num; i++){
		TS_LOG_INFO("%5d ", (*sensor_rx_raw_data)[i]);
	}
	TS_LOG_INFO("\n");
		TS_LOG_INFO("%s: line =%d\n", __func__, __LINE__);
	TS_LOG_INFO("%s: sensor_tx_raw_data[0..%d]:\n", __func__, tx_num - 1);
	for (i = 0; i < tx_num; i++)
		TS_LOG_INFO("%5d ", (*sensor_tx_raw_data)[i]);
	TS_LOG_INFO("\n");
	
	TS_LOG_INFO("%s: cp_sensor_tx_data[0..%d]:\n", __func__, tx_num - 1);
	for (i = 0; i < tx_num; i++){
		TS_LOG_INFO("%5d ", (*cp_sensor_tx_data)[i]);
	}
	TS_LOG_INFO("\n");
	
	TS_LOG_INFO("%s: cp_sensor_rx_data[0..%d]:\n", __func__, rx_num - 1);
	for (i = 0; i < rx_num; i++)
		TS_LOG_INFO("%5d ", (*cp_sensor_rx_data)[i]);
	TS_LOG_INFO("\n");
#endif
	TS_LOG_INFO("%s: cp_sensor_rx_average: %d\n", __func__,
			*cp_sensor_rx_average);
	TS_LOG_INFO("%s: cp_sensor_tx_average: %d\n", __func__,
			*cp_sensor_tx_average);

free_buffers:
	if (ret) {
		if (!skip_cp_sensor) {
			kfree(*sensor_rx_raw_data);
			kfree(*sensor_tx_raw_data);
			kfree(*cp_sensor_rx_data);
			kfree(*cp_sensor_tx_data);
			kfree(cp_sensor_rx_calibration);
			kfree(cp_sensor_tx_calibration);			
			*sensor_rx_raw_data = NULL;
			*sensor_tx_raw_data = NULL;
			*cp_sensor_rx_data = NULL;
			*cp_sensor_tx_data = NULL;
			cp_sensor_rx_calibration = NULL;
			cp_sensor_tx_calibration = NULL;
		}
	}

exit:	
	kfree(hw_bl_calibration_self_data);
	kfree(hw_bl_pwc_self_rx);
	kfree(hw_bl_pwc_self_tx);
	hw_bl_calibration_self_data = NULL;
	hw_bl_pwc_self_rx = NULL;
	hw_bl_pwc_self_tx = NULL;
	
	return ret;
}

static int validate_cp_test_results_gen6(struct configuration *configuration,
					 struct result *result, uint32_t tx_num,
					 uint32_t rx_num, bool skip_cp_sensor,
					 int *cp_sensor_rx_data,
					 int *cp_sensor_tx_data,
					 int cp_sensor_rx_calibration,
					 int cp_sensor_tx_calibration,
					 int cp_sensor_rx_average,
					 int cp_sensor_tx_average,
					 int cp_sensor_rx_delta,
					 int cp_sensor_tx_delta, bool *pass)
{
	uint32_t i = 0;
	uint32_t configuration_rx_num;
	uint32_t configuration_tx_num;
	result->cp_test_pass = true;
	configuration_rx_num = configuration->cp_min_max_table_rx_size / 2;
	configuration_tx_num = configuration->cp_min_max_table_tx_size / 2;

	/* Check Sensor Cp delta for range limit */
	result->cp_sensor_delta_pass = true;
	if ((cp_sensor_rx_delta > configuration->cp_max_delta_sensor_rx_percent)
	    || (cp_sensor_tx_delta >
		configuration->cp_max_delta_sensor_tx_percent)) {
		TS_LOG_ERR
		    ("%s: Cp_sensor_rx_delta:%d(%d) Cp_sensor_tx_delta:%d (%d)\n",
		     "Cp sensor delta range limit test", cp_sensor_rx_delta,
		     configuration->cp_max_delta_sensor_rx_percent,
		     cp_sensor_tx_delta,
		     configuration->cp_max_delta_sensor_tx_percent);
		result->cp_sensor_delta_pass = false;
	}

	/* Check sensor Cp rx for min/max values */
	result->cp_rx_validation_pass = true;
	for (i = 0; i < configuration_rx_num; i++) {
		int cp_rx_min = configuration->cp_min_max_table_rx[i * 2];
		int cp_rx_max = configuration->cp_min_max_table_rx[i * 2 + 1];
		if ((cp_sensor_rx_data[i] <= cp_rx_min)
		    || (cp_sensor_rx_data[i] >= cp_rx_max)) {
			TS_LOG_ERR("%s: Cp Rx[%d]:%d (%d,%d)\n",
				   "Cp Rx min/max test",
				   i,
				   (int)cp_sensor_rx_data[i],
				   cp_rx_min, cp_rx_max);
			result->cp_rx_validation_pass = false;
		}
	}
	/* Check sensor Cp tx for min/max values */
	result->cp_tx_validation_pass = true;
	for (i = 0; i < configuration_tx_num; i++) {
		int cp_tx_min = configuration->cp_min_max_table_tx[i * 2];
		int cp_tx_max = configuration->cp_min_max_table_tx[i * 2 + 1];
		if ((cp_sensor_tx_data[i] <= cp_tx_min)
		    || (cp_sensor_tx_data[i] >= cp_tx_max)) {
			TS_LOG_ERR("%s: Cp Tx[%d]:%d(%d,%d)\n",
				   "Cp Tx min/max test",
				   i,
				   (int)cp_sensor_tx_data[i],
				   cp_tx_min, cp_tx_max);
			result->cp_tx_validation_pass = false;
		}
	}

	result->cp_test_pass &= result->cp_sensor_delta_pass
	    & result->cp_rx_validation_pass & result->cp_tx_validation_pass;

	if (pass)
		*pass = result->cp_test_pass;

	return 0;
}

static int validate_cp_test_results_gen6l(struct configuration *configuration,
					 struct result *result, uint32_t tx_num,  uint32_t rx_num, 
					 bool skip_cp_sensor,  bool skip_cp_button,
					 int *cp_sensor_rx_data, int *cp_sensor_tx_data,
					 int *cp_button_data,  int cp_sensor_rx_calibration,
					 int cp_sensor_tx_calibration, int cp_button_calibration,
					 int cp_sensor_rx_average, int cp_sensor_tx_average,
					 int cp_button_average, int *cp_sensor_rx_delta,
					 int *cp_sensor_tx_delta, int cp_button_delta,
					 bool *pass)
{
	uint32_t i = 0;
	uint32_t configuration_rx_num;
	uint32_t configuration_tx_num;
	result->cp_test_pass = true;
	configuration_rx_num = configuration->cp_min_max_table_rx_size / 2;
	configuration_tx_num = configuration->cp_min_max_table_tx_size / 2;

	if (skip_cp_sensor)
		goto exit;
	
	/* Check Sensor Cp delta for range limit */
	result->cp_sensor_delta_pass = true;
	for(i=0; i<rx_num; i++) {
		if(cp_sensor_rx_delta[i] > configuration->cp_max_delta_sensor_rx_percent) {
			TS_LOG_ERR("%s: Cp_sensor_rx_delta:%2d(%d)\n",
				"Cp sensor delta range limit test",
				cp_sensor_rx_delta[i],
				configuration->cp_max_delta_sensor_rx_percent);
			result->cp_sensor_delta_pass = false;
		}
	}
	for(i=0; i<tx_num; i++) {
		if(cp_sensor_tx_delta[i] > configuration->cp_max_delta_sensor_tx_percent) {
			TS_LOG_ERR("%s: Cp_sensor_tx_delta:%2d(%d)\n",
				"Cp sensor delta range limit test",
				cp_sensor_tx_delta[i],
				configuration->cp_max_delta_sensor_tx_percent);
			result->cp_sensor_delta_pass = false;
		}
	}

	/* Check sensor Cp rx for min/max values */
	result->cp_rx_validation_pass = true;
	for (i = 0; i < configuration_rx_num; i++) {
		int cp_rx_min = configuration->cp_min_max_table_rx[i * 2];
		int cp_rx_max = configuration->cp_min_max_table_rx[i * 2 + 1];
		if ((cp_sensor_rx_data[i] <= cp_rx_min)
		    || (cp_sensor_rx_data[i] >= cp_rx_max)) {
			TS_LOG_ERR("%s: Cp Rx[%d]:%d (%d,%d)\n",
				   "Cp Rx min/max test",
				   i,
				   (int)cp_sensor_rx_data[i],
				   cp_rx_min, cp_rx_max);
			result->cp_rx_validation_pass = false;
		}
	}
	/* Check sensor Cp tx for min/max values */
	result->cp_tx_validation_pass = true;
	for (i = 0; i < configuration_tx_num; i++) {
		int cp_tx_min = configuration->cp_min_max_table_tx[i * 2];
		int cp_tx_max = configuration->cp_min_max_table_tx[i * 2 + 1];
		if ((cp_sensor_tx_data[i] <= cp_tx_min)
		    || (cp_sensor_tx_data[i] >= cp_tx_max)) {
			TS_LOG_ERR("%s: Cp Tx[%d]:%d(%d,%d)\n",
				   "Cp Tx min/max test",
				   i,
				   (int)cp_sensor_tx_data[i],
				   cp_tx_min, cp_tx_max);
			result->cp_tx_validation_pass = false;
		}
	}

	result->cp_test_pass &= result->cp_sensor_delta_pass
	    & result->cp_rx_validation_pass & result->cp_tx_validation_pass;
	
exit:
	if (pass)
		*pass = result->cp_test_pass;
	return 0;
}

static int get_extra_parameter(uint32_t *sensor_assignment,
			       uint32_t *config_ver, uint32_t *revision_ctrl,
			       uint32_t *device_id_high,
			       uint32_t *device_id_low)
{
	union parameter_value parameter_value;
	enum parameter_type parameter_type;
	struct pip_sysinfo sysinfo;
	int i;
	int ret;
	*device_id_high = 0;
	*device_id_low = 0;
	/* Get Device Setup:SENSOR_ASSIGNMENT */
	ret = get_configuration_parameter(SENSOR_ASSIGNMENT, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != STRING) {
		TS_LOG_ERR("%s:Unable to get sensor assignment!\n", __func__);
		goto exit;
	}
	*sensor_assignment = !strcmp(parameter_value.string, RX_IS_Y);
	TS_LOG_INFO("%s: sensor_assignment: %d\n", __func__,
		    *sensor_assignment);

	/* Get config version and revision control version */
	ret = pip_get_system_information(&sysinfo);
	if (ret) {
		TS_LOG_ERR("%s:Unable to get system information!\n", __func__);
		goto exit;
	}
	*revision_ctrl = sysinfo.cy_data.fw_revctrl;
	*config_ver = sysinfo.cy_data.fw_ver_conf;
	TS_LOG_INFO("%s: config_ver: %d\n", __func__, *config_ver);
	TS_LOG_INFO("%s: revision_ctrl: %d\n", __func__, *revision_ctrl);
	for (i = 0; i < 4; i++) {
		*device_id_low =
		    (*device_id_low << 8) + sysinfo.cy_data.mfg_id[i];
	}
	for (i = 4; i < 8; i++) {
		*device_id_high =
		    (*device_id_high << 8) + sysinfo.cy_data.mfg_id[i];
	}
	TS_LOG_INFO("%s: device_id_low: 0x%x\n", __func__, *device_id_low);
	TS_LOG_INFO("%s: device_id_high: 0x%x\n", __func__, *device_id_high);
	return 0;
exit:
	return ret;
}

static int check_tests(uint32_t *tx_num, uint32_t *rx_num,
		       uint32_t *button_num, bool *skip_cm_button,
		       bool *skip_cp_sensor, bool *skip_cp_button)
{
	union parameter_value parameter_value;
	enum parameter_type parameter_type;
	uint32_t act_lft_en_enabled;
	uint32_t bl_h2o_rjct_enabled;
	char *scanning_mode_button;
	int ret;

	/* Get CDC:TX_NUM */
	ret = get_configuration_parameter(TX_NUM, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get tx_num!\n", __func__);
		goto exit;
	}

	*tx_num = parameter_value.integer;
	TS_LOG_INFO("%s: tx_num: %d\n", __func__, *tx_num);

	/* Get CDC:RX_NUM */
	ret = get_configuration_parameter(RX_NUM, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get rx_num!\n", __func__);
		goto exit;
	}

	*rx_num = parameter_value.integer;
	TS_LOG_INFO("%s: rx_num: %d\n", __func__, *rx_num);

	/* Get CDC:BUTTON_NUM */
	ret = get_configuration_parameter(BUTTON_NUM, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != INTEGER) {
		TS_LOG_ERR("%s:Unable to get button_num!\n", __func__);
		goto exit;
	}

	*button_num = parameter_value.integer;
	TS_LOG_INFO("%s: button_num: %d\n", __func__, *button_num);

	/* Get FingerTracking:ACT_LFT_EN */
	ret = get_configuration_parameter(ACT_LFT_EN, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != STRING) {
		TS_LOG_ERR("%s:Unable to get act_lft_en!\n", __func__);
		goto exit;
	}

	act_lft_en_enabled = !strcmp(parameter_value.string,
				     ACT_LFT_EN_ENABLED);
	TS_LOG_INFO("%s: act_lft_en: %d\n", __func__, act_lft_en_enabled);

	/* Get ScanFiltering:BL_H20_RJCT */
	ret = get_configuration_parameter(BL_H20_RJCT, &parameter_type,
					  &parameter_value);
	if (ret || parameter_type != STRING) {
		TS_LOG_ERR("%s:Unable to get bl_h2o_rjct!\n", __func__);
		goto exit;
	}

	bl_h2o_rjct_enabled = !strcmp(parameter_value.string,
				      BL_H20_RJCT_ENABLED);
	TS_LOG_INFO("%s: bl_h2o_rjct: %d\n", __func__, bl_h2o_rjct_enabled);

	/* Get CDC:SCANNING_MODE_BUTTON */
	ret = get_configuration_parameter(SCANNING_MODE_BUTTON,
					  &parameter_type, &parameter_value);
	if (ret < 0 || parameter_type != STRING) {
		TS_LOG_ERR("%s:Unable to get scanning_mode_button! ret = %d\n",
			   __func__, ret);
		goto exit;
	}
	scanning_mode_button = parameter_value.string;
	TS_LOG_INFO("%s: scanning_mode_button: %s\n", __func__,
		    scanning_mode_button);

	*skip_cm_button = false;
	*skip_cp_button = false;
	*skip_cp_sensor = false;

	if (*button_num == 0) {
		*skip_cm_button = true;
		*skip_cp_button = true;
	} else if (!strcmp(scanning_mode_button, SCANNING_MODE_BUTTON_MUTUAL))
		*skip_cp_button = true;
	else if (!strcmp(scanning_mode_button, SCANNING_MODE_BUTTON_SELF))
		*skip_cm_button = true;

	if (!act_lft_en_enabled && !bl_h2o_rjct_enabled)
		*skip_cp_sensor = true;

exit:
	return ret;
}

#define CY_MEM_FREE(x) \
	do { \
		kfree(x); \
		x = NULL; \
	} while(0)

static int do_calibration_tdda(struct device *dev, bool set_single)
{
	u8 status = 0;
	int rc = 0;
	int res = 0;
	struct cyttsp5_core_commands *cyttsp_cmd = cyttsp5_get_commands();

	if (set_single) {
		/*force single tx*/
		rc = cyttsp_cmd->nonhid_cmd->set_param(dev, 0, 0x1F, 1);
		if (rc) {
			res = rc;
			TS_LOG_ERR("%s:force single tx failed", __func__);
		}
	} else {
		/*force multi tx*/
		rc = cyttsp_cmd->nonhid_cmd->set_param(dev, 0, 0x1F, 0);
		if (rc) {
			res = rc;
			TS_LOG_ERR("%s:force multi tx failed", __func__);
		}
	}

	/*suspend_scanning */
	rc = cyttsp_cmd->nonhid_cmd->suspend_scanning(dev, 0);
	if (rc) {
		res = rc;
		TS_LOG_ERR("%s:suspend_scanning failed", __func__);
	}

	/*do calibration*/
	TS_LOG_DEBUG("%s:do calibration in single tx mode", __func__);
	rc = cyttsp_cmd->nonhid_cmd->calibrate_idacs(dev,0, 0, &status);
	if (rc < 0) {
		res = rc;
		TS_LOG_ERR("%s: Error on calibrate idacs for mutual r=%d\n",
					__func__, rc);
	}

	rc = cyttsp_cmd->nonhid_cmd->calibrate_idacs(dev, 0, 2, &status);
	if (rc < 0) {
		res = rc;
		TS_LOG_ERR("%s: Error on calibrate idacs for self r=%d\n",
					__func__, rc);
	}
	/*resume_scanning */
	rc = cyttsp_cmd->nonhid_cmd->resume_scanning(dev, 0);
	if (rc) {
		res = rc;
		TS_LOG_ERR("%s:resume_scanning failed", __func__);
	}

	return res;
}

static int short_test_ttda(struct device *dev, struct result *result)
{
	int rc = 0;
	struct cyttsp5_device_access_data *dad;

	dad= cyttsp5_get_device_access_data(dev);

	rc = cyttsp5_run_and_get_selftest_result_noprint(
			dev, CY_ST_ID_AUTOSHORTS, PIP_CMD_MAX_LENGTH, false);
	if (rc) {
		TS_LOG_ERR("%s:execute short fail\n", __func__);
		return rc;
	}

	if (dad->ic_buf[1] != 0)
		result->short_test_pass = false;
	else
		result->short_test_pass = true;

	return rc;
}
static int validate_cp_test_result_ttda(
					uint16_t tx_num, uint16_t rx_num,
					struct configuration *configuration,
					int *cp_sensor_rx_data, int *cp_sensor_tx_data,
					int *cp_sensor_rx_delta, int *cp_sensor_tx_delta,
					struct result *result,bool *pass)
{
	uint32_t i = 0;
	uint32_t configuration_rx_num;
	uint32_t configuration_tx_num;
	result->cp_test_pass = true;
	configuration_rx_num = configuration->cp_min_max_table_rx_size / 2;
	configuration_tx_num = configuration->cp_min_max_table_tx_size / 2;
	if (configuration_rx_num != rx_num || configuration_tx_num != tx_num) {
		TS_LOG_ERR("%s:config file may mismatch, %s=%d, %s=%d, rx_num=%d, tx_num=%d\n",
					__func__,
					"configuration_rx_num", configuration_rx_num,
					"configuration_tx_num", configuration_tx_num,
					rx_num, tx_num);
		return -EINVAL;
	}
	/* Check Sensor Cp delta for range limit */
	result->cp_sensor_delta_pass = true;
	for(i=0; i<rx_num; i++) {
		if(cp_sensor_rx_delta[i] > configuration->cp_max_delta_sensor_rx_percent) {
			TS_LOG_ERR("%s: Cp_sensor_rx_delta:%2d(%d)\n",
							"Cp sensor delta range limit test",
							cp_sensor_rx_delta[i],
							configuration->cp_max_delta_sensor_rx_percent);
			result->cp_sensor_delta_pass = false;
		}
	}
	for(i=0; i<tx_num; i++) {
		if(cp_sensor_tx_delta[i] > configuration->cp_max_delta_sensor_tx_percent) {
			TS_LOG_ERR("%s: Cp_sensor_tx_delta:%2d(%d)\n",
							"Cp sensor delta range limit test",
							cp_sensor_tx_delta[i],
							configuration->cp_max_delta_sensor_tx_percent);
			result->cp_sensor_delta_pass = false;
		}
	}

	/* Check sensor Cp rx for min/max values */
	result->cp_rx_validation_pass = true;
	for (i = 0; i < rx_num; i++) {
		int cp_rx_min = configuration->cp_min_max_table_rx[i * 2];
		int cp_rx_max = configuration->cp_min_max_table_rx[i * 2 + 1];
		if ((cp_sensor_rx_data[i] <= cp_rx_min)
			|| (cp_sensor_rx_data[i] >= cp_rx_max)) {
			TS_LOG_ERR("%s: Cp Rx[%d]:%d (%d,%d)\n",
							"Cp Rx min/max test", i,
							(int)cp_sensor_rx_data[i],
							cp_rx_min, cp_rx_max);
			result->cp_rx_validation_pass = false;
		}
	}
	/* Check sensor Cp tx for min/max values */
	result->cp_tx_validation_pass = true;
	for (i = 0; i < tx_num; i++) {
		int cp_tx_min = configuration->cp_min_max_table_tx[i * 2];
		int cp_tx_max = configuration->cp_min_max_table_tx[i * 2 + 1];
		if ((cp_sensor_tx_data[i] <= cp_tx_min)
			|| (cp_sensor_tx_data[i] >= cp_tx_max)) {
			TS_LOG_ERR("%s: Cp Tx[%d]:%d(%d,%d)\n",
					"Cp Tx min/max test", i,
					(int)cp_sensor_tx_data[i],
					cp_tx_min, cp_tx_max);
			result->cp_tx_validation_pass = false;
		}
	}

	result->cp_test_pass &= result->cp_sensor_delta_pass
		& result->cp_rx_validation_pass & result->cp_tx_validation_pass;

	if (pass)
		*pass = result->cp_test_pass;
	return 0;
}

static int get_cp_test_data_ttda(struct device *dev,
					uint16_t tx_num, uint16_t rx_num,
					int **cp_sensor_tx_data,
					int **cp_sensor_rx_data,
					int *cp_sensor_tx_average,
					int *cp_sensor_rx_average,
					int **cp_sensor_rx_calibration,
					int **cp_sensor_tx_calibration,
					int **cp_sensor_rx_detla,
					int **cp_sensor_tx_detla)
{
	int ret = 0;
	long long cp_tx_ave_data_panel = 0;
	long long cp_rx_ave_data_panel = 0;
	struct cyttsp5_device_access_data *dad = cyttsp5_get_device_access_data(dev);
	int i = 0;
	*cp_sensor_tx_data = kzalloc(tx_num * sizeof(int), GFP_KERNEL);
	*cp_sensor_rx_data = kzalloc(rx_num * sizeof(int), GFP_KERNEL);
	*cp_sensor_tx_calibration = kzalloc(tx_num * sizeof(int), GFP_KERNEL);
	*cp_sensor_rx_calibration = kzalloc(rx_num * sizeof(int), GFP_KERNEL);
	*cp_sensor_tx_detla = kzalloc(tx_num * sizeof(int), GFP_KERNEL);
	*cp_sensor_rx_detla = kzalloc(rx_num * sizeof(int), GFP_KERNEL);

	if (!*cp_sensor_tx_data || !*cp_sensor_rx_data ||
		!*cp_sensor_tx_calibration || !*cp_sensor_rx_calibration) {
			TS_LOG_ERR("%s: memory alloc fail\n", __func__);
			ret = -ENOMEM;
			goto exit;
	}
	memset(dad->ic_buf, 0 ,CY_MAX_PRBUF_SIZE);
	ret = cyttsp5_run_and_get_selftest_result_noprint(dev,
					CY_ST_ID_CP_PANEL, (rx_num + tx_num) * 4, true);
	if (ret) {
		TS_LOG_ERR("%s:Get CP Panel not supported\n", __func__);
		goto exit;
	}

	/*Get cp_tx_data_panel*/
	for (i = 0; i < tx_num; i++) {
		(*cp_sensor_tx_data)[i] =
				10 * (dad->ic_buf[CP_PANEL_DATA_OFFSET+i*2]
				+ 256 * dad->ic_buf[CP_PANEL_DATA_OFFSET+i*2+1]);
		TS_LOG_DEBUG("cp_sensor_tx_data[%d]=%d\n", i, (*cp_sensor_tx_data)[i]);
		cp_tx_ave_data_panel += (*cp_sensor_tx_data)[i];
	}
	cp_tx_ave_data_panel /= tx_num;
	*cp_sensor_tx_average = cp_tx_ave_data_panel;

	/*Get cp_tx_cal_data_panel*/
	for (i = 0; i < tx_num; i++) {
		(*cp_sensor_tx_calibration)[i] =
			10*(dad->ic_buf[CP_PANEL_DATA_OFFSET+tx_num*2+i*2]
			+ 256 * dad->ic_buf[CP_PANEL_DATA_OFFSET+tx_num*2+i*2+1]);
		TS_LOG_DEBUG("cp_sensor_tx_calibration[%d]=%d\n",
						i, (*cp_sensor_tx_calibration)[i]);
	}

	/*get cp_sensor_tx_delta,using the first sensor cal value for temp */
	/*multiple 1000 to increase accuracy*/
	for (i = 0; i < tx_num;  i++) {
		(*cp_sensor_tx_detla)[i]= ABS(((*cp_sensor_tx_calibration)[i]
				- (*cp_sensor_tx_data)[i]) * 100 / (*cp_sensor_tx_data)[i]);
		TS_LOG_DEBUG("cp_sensor_tx_detla[%d]=%d\n",
						i, (*cp_sensor_tx_detla)[i]);
	}

	/*Get cp_rx_data_panel*/
	for (i = 0; i < rx_num;  i++) {
		(*cp_sensor_rx_data)[i] =
			10*(dad->ic_buf[CP_PANEL_DATA_OFFSET+tx_num*4+i*2] +
			256 * dad->ic_buf[CP_PANEL_DATA_OFFSET+tx_num*4+i*2+1]);
		TS_LOG_DEBUG("%s:cp_sensor_rx_data[%d]=%d\n", __func__ , i,
						(*cp_sensor_rx_data)[i]);
		cp_rx_ave_data_panel += (*cp_sensor_rx_data)[i];
	}
	cp_rx_ave_data_panel /= rx_num;
	*cp_sensor_rx_average = cp_rx_ave_data_panel;

	/*Get cp_rx_cal_data_panel*/
	for (i = 0; i < rx_num; i++) {
		(*cp_sensor_rx_calibration)[i] =
			10 * (dad->ic_buf[CP_PANEL_DATA_OFFSET+tx_num*4+rx_num*2+i*2] +
			256 * dad->ic_buf[CP_PANEL_DATA_OFFSET+tx_num*4+rx_num*2+i*2+1]);
		TS_LOG_DEBUG("cp_sensor_rx_calibration[%d]=%d\n",
						i, (*cp_sensor_rx_calibration)[i]);
	}

	/*get cp_sensor_rx_delta,using the first sensor cal value for temp */
	/*multiple 1000 to increase accuracy*/
	for (i = 0; i < rx_num; i++) {
		(*cp_sensor_rx_detla)[i] = ABS(((*cp_sensor_rx_calibration)[i]
					- (*cp_sensor_rx_data)[i]) * 100 / (*cp_sensor_rx_data)[i]);
		TS_LOG_DEBUG("cp_sensor_rx_detla[%d]=%d\n",
						i, (*cp_sensor_rx_detla)[i]);
	}

	return 0;

exit:
	CY_MEM_FREE(*cp_sensor_tx_data);
	CY_MEM_FREE(*cp_sensor_rx_data);
	CY_MEM_FREE(*cp_sensor_tx_calibration);
	CY_MEM_FREE(*cp_sensor_rx_calibration);
	CY_MEM_FREE(*cp_sensor_rx_detla);
	CY_MEM_FREE(*cp_sensor_tx_detla);
	return ret;
}

static int validate_cm_test_result_ttda(
					struct configuration *configuration,
					int tx_num, int rx_num,
					int *cm_sensor_data, int cm_sensor_average,
					int **cm_sensor_column_delta, int **cm_sensor_row_delta,
					int cm_sensor_calibration, int cm_sensor_delta,
					struct result *result, bool *pass)
{
	int ret = 0;
	int configuration_sensor_num;
	int sensor_num = tx_num * rx_num;
	int i, j = 0;

	*cm_sensor_column_delta = kzalloc((tx_num * rx_num) * sizeof(int), GFP_KERNEL);
	*cm_sensor_row_delta = kzalloc((tx_num * rx_num) * sizeof(int), GFP_KERNEL);
	if (!*cm_sensor_column_delta || !*cm_sensor_row_delta) {
		TS_LOG_ERR("%s:memory alloc fail\n", __func__);
		ret = -ENOMEM;
		goto exit;
	}

	configuration_sensor_num = configuration->cm_min_max_table_sensor_size / 2;

	if (configuration_sensor_num != sensor_num) {
		TS_LOG_ERR("%s: %s (Configuration:%d, Device:%d)\n", __func__,
					"Configuration and Device number of sensors mismatch!",
					configuration_sensor_num, sensor_num);
		ret = -EINVAL;
		goto exit;
	}


	/* Check each sensor Cm data for min/max values */
	result->cm_sensor_validation_pass = true;
	for (i = 0; i < sensor_num; i++) {
		int row = i % rx_num;
		int col = i / rx_num;
		int cm_sensor_min =
			configuration->cm_min_max_table_sensor[(row * tx_num + col) * 2];
		int cm_sensor_max =
			configuration->cm_min_max_table_sensor[(row * tx_num + col) * 2 + 1];
		if ((cm_sensor_data[i] < cm_sensor_min)
			|| (cm_sensor_data[i] > cm_sensor_max)) {
			TS_LOG_ERR("%s: Sensor[%d,%d]:%d (%d,%d)\n",
					"Cm sensor min/max test",
					row, col, (int)cm_sensor_data[i],
					cm_sensor_min, cm_sensor_max);
			result->cm_sensor_validation_pass = false;
		}
	}

	/* Check each row Cm data with neighbor for difference */
	result->cm_sensor_row_delta_pass = true;
	for (i = 0; i < tx_num; i++) {
		for (j = 1; j < rx_num; j++) {
			int cm_sensor_row_diff =
				ABS((int)cm_sensor_data[i * rx_num + j] -
				(int)cm_sensor_data[i * rx_num + j - 1]);
			(*cm_sensor_row_delta)[i * rx_num + j - 1] =
				cm_sensor_row_diff;
			if (cm_sensor_row_diff
				> configuration->cm_range_limit_row) {
				TS_LOG_ERR("%s: Sensor[%d,%d]:%d (%d)\n",
						"Cm sensor row range limit test",
						j, i, cm_sensor_row_diff,
						configuration->cm_range_limit_row);
				result->cm_sensor_row_delta_pass = false;
			}
		}
	}

	/* Check each column Cm data with neighbor for difference */
	result->cm_sensor_col_delta_pass = true;
	for (i = 1; i < tx_num; i++) {
		for (j = 0; j < rx_num; j++) {
			int cm_sensor_col_diff =
				ABS((int)cm_sensor_data[i * rx_num + j] -
				(int)cm_sensor_data[(i - 1) * rx_num + j]);
			(*cm_sensor_column_delta)[(i - 1) * rx_num + j] =
				cm_sensor_col_diff;
			if (cm_sensor_col_diff
				> configuration->cm_range_limit_col) {
				TS_LOG_ERR("%s: Sensor[%d,%d]:%d (%d)\n",
						"Cm sensor column range limit test",
						j, i, cm_sensor_col_diff,
						configuration->cm_range_limit_col);
				result->cm_sensor_col_delta_pass = false;
			}
		}
	}

	/* Check sensor calculated Cm for min/max values */
	result->cm_sensor_calibration_pass = true;
	if (cm_sensor_calibration < configuration->cm_min_limit_cal
	    || cm_sensor_calibration > configuration->cm_max_limit_cal) {
		TS_LOG_ERR("%s: Cm_sensor_calibration:%d (%d,%d)\n",
				"Cm sensor Cm_cal min/max test",
				cm_sensor_calibration,
				configuration->cm_min_limit_cal,
				configuration->cm_max_limit_cal);
		result->cm_sensor_calibration_pass = false;
	}

	/* Check sensor Cm delta for range limit */
	result->cm_sensor_delta_pass = true;
	if (cm_sensor_delta > configuration->cm_max_delta_sensor_percent) {
		TS_LOG_ERR("%s: Cm_sensor_delta:%d (%d)\n",
				"Cm sensor delta range limit test",
				cm_sensor_delta,
				configuration->cm_max_delta_sensor_percent);
		result->cm_sensor_delta_pass = false;
	}

	result->cm_test_pass = result->cm_sensor_validation_pass
		& result->cm_sensor_row_delta_pass
		& result->cm_sensor_col_delta_pass
		& result->cm_sensor_calibration_pass & result->cm_sensor_delta_pass;
	TS_LOG_INFO("cm_test_pass=%d \n", result->cm_test_pass);

exit:
	if (ret) {
		CY_MEM_FREE(*cm_sensor_column_delta);
		CY_MEM_FREE(*cm_sensor_row_delta);
	} else if (pass)
		*pass = result->cm_test_pass;
	return ret;
}

static int get_cm_test_data_ttda(struct device *dev,
					int tx_num, int rx_num,struct configuration *configuration,
					int **cm_sensor_data, int *cm_sensor_average,
					int *cm_sensor_calibration, int *cm_sensor_delta)
{
	int rc = 0;
	int i = 0;
	int cm_average_data = 0;
	int actual_read_length = 0;
	/*
	 * read data format is:
	 * cm_data[0]_low,cm_data[0]_high...
	 * cm_data[rx_num * tx_num]_low,cm_data[rx_num * tx_num]_low,
	 * cm_cal_low,cm_cal_high
	 * so need to read (rx_num * tx_num + 1) * 2 Bytes
	 */
	int read_length = (rx_num * tx_num + 1) * 2;
	struct cyttsp5_device_access_data *dad = cyttsp5_get_device_access_data(dev);
	if (!dad) {
		TS_LOG_ERR("%s:get access data fail\n", __func__);
		return -EINVAL;
	}

	(*cm_sensor_data) = kzalloc((tx_num * rx_num) * sizeof(int), GFP_KERNEL);
	if (!*cm_sensor_data) {
		TS_LOG_ERR("%s:cm_sensor_data alloc fail\n", __func__);
		rc = -ENOMEM;
		goto exit;
	}

	rc = cyttsp5_run_and_get_selftest_result_noprint(
							dev, CY_ST_ID_CM_PANEL, read_length, true);
	if (rc) {
		TS_LOG_ERR("%s:Get CM Panel fail\n", __func__);
		goto exit;
	}

	actual_read_length = get_unaligned_le16(&dad->ic_buf[4]);
	TS_LOG_INFO("%s:actual_read_length=%d, (tx_num * rx_num)=%d\n", __func__,
					actual_read_length, (tx_num * rx_num));

	for (i = 0; i < (tx_num * rx_num); i++) {
		(*cm_sensor_data)[i] =
			10*(dad->ic_buf[CM_PANEL_DATA_OFFSET+i*2] + 256
			* dad->ic_buf[CM_PANEL_DATA_OFFSET+i*2+1]);
		TS_LOG_DEBUG("cm_sensor_data[%d]=%d\n", i, (*cm_sensor_data)[i]);
		cm_average_data += (*cm_sensor_data)[i];
	}

	cm_average_data /= (tx_num * rx_num);
	*cm_sensor_average = cm_average_data;
	*cm_sensor_calibration =
				10*(dad->ic_buf[CM_PANEL_DATA_OFFSET+i*2]
				+ 256 * dad->ic_buf[CM_PANEL_DATA_OFFSET+i*2+1]);
	/*multiple 1000 to increate accuracy*/
	*cm_sensor_delta = ABS((cm_average_data - *cm_sensor_calibration) * 100 /
								cm_average_data);
	return 0;
exit:
	CY_MEM_FREE(*cm_sensor_data);
	return rc;

}

static int sysinfo_get_ttda(struct device *dev,
							uint32_t *tx_num, uint32_t *rx_num,
							struct result *result)
{
	struct cyttsp5_core_data *cd = dev_get_drvdata(dev);
	if (!cd) {
		TS_LOG_ERR("%s: cd is null \n", __func__);
		return -EINVAL;
	}

	*tx_num = cd->sysinfo.sensing_conf_data.tx_num;
	*rx_num = cd->sysinfo.sensing_conf_data.rx_num;

	result->config_ver = cd->sysinfo.cydata.fw_ver_conf;
	result->rx_num = *rx_num;
	result->tx_num = *tx_num;

	TS_LOG_INFO("%s:tx_num = %d, rx_num = %d, fw_ver_conf = 0x%02x\n", __func__,
					*tx_num, *rx_num, result->config_ver);
	return 0;
}

int cm_cp_test_run_ttda(struct device *dev,
			struct file *configuration_file,
			struct seq_file *result_file,  bool run_cm_test,
			bool run_cp_test, bool *cm_test_pass, bool *cp_test_pass)
{
	struct configuration *configuration = NULL;
	struct result *result = NULL;
	uint32_t tx_num, rx_num;
	int *cm_sensor_data = NULL;
	int *cm_sensor_column_delta = NULL, *cm_sensor_row_delta = NULL;
	int cm_sensor_average = 0;
	int cm_sensor_calibration = 0;
	int cm_sensor_delta = 0;
	int *cp_sensor_rx_data = NULL, *cp_sensor_tx_data = NULL;
	int *cp_sensor_rx_calibration = NULL, *cp_sensor_tx_calibration = NULL;
	int cp_sensor_rx_average = 0, cp_sensor_tx_average = 0;
	int *cp_sensor_rx_delta = NULL, *cp_sensor_tx_delta = NULL;
	int ret = 0;

	TS_LOG_INFO("%s: begin\n", __func__);
	if(!dev) {
		TS_LOG_ERR("%s:dev is null\n", __func__);
		seq_printf(result_file, "1F-");
		goto software_reason_exit;
	}

	configuration = kzalloc(sizeof(struct configuration), GFP_KERNEL);
	result = kzalloc(sizeof(struct result), GFP_KERNEL);
	if (!configuration || !result) {
		ret = -ENOMEM;
		TS_LOG_ERR("%s: kzalloc configuration result fail\n", __func__);
		seq_printf(result_file, "1F-");
		goto software_reason_exit;
	}
	ret  = sysinfo_get_ttda(dev, &tx_num, &rx_num, result);
	if (ret || !tx_num || !rx_num) {
		TS_LOG_ERR("%s:sysinfo get fail\n", __func__);
		seq_printf(result_file, "1F-");
		goto software_reason_exit;
	}

	ret = configuration_get(configuration_file, configuration);
	if (ret) {
		TS_LOG_ERR("%s: Fail getting configuration\n", __func__);
		seq_printf(result_file, "1F-");
		goto software_reason_exit;
	}

	g_family_type = (uint8_t)configuration->family_type;
	TS_LOG_INFO("%s: g_family_type:%d\n", __func__,g_family_type);

	ret = do_calibration_tdda(dev, true);
	if (ret) {
		TS_LOG_ERR("%s: calibration fail\n", __func__);
		seq_printf(result_file, "1F");
		goto software_reason_exit;
	}

	ret = short_test_ttda(dev, result);
	if (ret) {
		TS_LOG_ERR("%s: short test fail cause of software reason\n", __func__);
		seq_printf(result_file, "2F-");
		goto software_reason_exit;
	} else if (!result->short_test_pass) {
		TS_LOG_INFO("%s: short test fail cause of software reason\n", __func__);
		seq_printf(result_file, "2F-");
	} else {
		TS_LOG_ERR("%s: short test pass\n", __func__);
		seq_printf(result_file, "2P-");
	}

	ret = get_cm_test_data_ttda(dev, tx_num, rx_num, configuration,
					&cm_sensor_data, &cm_sensor_average,
					&cm_sensor_calibration, &cm_sensor_delta);
	if (ret) {
		TS_LOG_ERR("%s: get cm data fail, ret = %d\n", __func__, ret);
		seq_printf(result_file, "3F-");
		goto software_reason_exit;
	} else {
		TS_LOG_INFO("%s:get cm data succ\n", __func__);
	}

	ret = validate_cm_test_result_ttda(
					configuration,
					tx_num, rx_num,
					cm_sensor_data, cm_sensor_average,
					&cm_sensor_column_delta, &cm_sensor_row_delta,
					cm_sensor_calibration, cm_sensor_delta,
					result, cm_test_pass);
	if (ret) {
		TS_LOG_ERR("%s:validate cm test result fail, ret = %d\n", __func__, ret);
		seq_printf(result_file, "3F-");
		goto software_reason_exit;
	} else {
		TS_LOG_INFO("%s:validate cm test result succ\n", __func__);
	}

	result->cm_sensor_data = cm_sensor_data;
	result->cm_sensor_raw_data = NULL;
	result->cm_sensor_column_delta = cm_sensor_column_delta;
	result->cm_sensor_row_delta = cm_sensor_row_delta;
	result->cm_sensor_calibration = cm_sensor_calibration;
	result->cm_sensor_average = cm_sensor_average;
	result->cm_sensor_delta = cm_sensor_delta;

	ret = get_cp_test_data_ttda(dev,
					tx_num, rx_num,
					&cp_sensor_tx_data,	&cp_sensor_rx_data,
					&cp_sensor_tx_average, &cp_sensor_rx_average,
					&cp_sensor_rx_calibration, &cp_sensor_tx_calibration,
					&cp_sensor_rx_delta, &cp_sensor_tx_delta);
	if (ret) {
		TS_LOG_ERR("%s:get cp data fail, ret = %d\n", __func__, ret);
		seq_printf(result_file, "7F-");
		goto software_reason_exit;
	} else {
		TS_LOG_INFO("%s:get cp data succ\n", __func__);
	}

	ret = validate_cp_test_result_ttda(tx_num, rx_num,
					configuration, cp_sensor_rx_data, cp_sensor_tx_data,
					cp_sensor_rx_delta, cp_sensor_tx_delta,
					result,cp_test_pass);
	if (ret) {
		TS_LOG_ERR("%s:validate cp test result fail, ret = %d\n", __func__, ret);
		seq_printf(result_file, "7F-");
		goto software_reason_exit;
	} else {
		TS_LOG_INFO("%s:validate cp test result succ\n", __func__);
	}

	result->cp_sensor_rx_delta_gen6l = cp_sensor_rx_delta;
	result->cp_sensor_tx_delta_gen6l = cp_sensor_tx_delta;
	result->cp_sensor_rx_calibration_gen6l = cp_sensor_rx_calibration;
	result->cp_sensor_tx_calibration_gen6l = cp_sensor_tx_calibration;
	result->cp_sensor_rx_data = cp_sensor_rx_data;
	result->cp_sensor_tx_data = cp_sensor_tx_data;
	result->cp_sensor_rx_raw_data = NULL;
	result->cp_sensor_tx_raw_data = NULL;
	result->cp_sensor_rx_average = cp_sensor_rx_average;
	result->cp_sensor_tx_average = cp_sensor_tx_average;
	result->tx_num = tx_num;
	result->rx_num = rx_num;
	result->button_num = 0;
	result->cm_test_run = run_cm_test;
	result->cp_test_run = run_cp_test;
	result->test_summary = result->cm_test_pass && result->cp_test_pass;

	ret = result_save(result_file, configuration, result);
	if (ret)
		TS_LOG_ERR("%s:Fail saving result\n", __func__);

	ret = do_calibration_tdda(dev, false);
	if (ret) {
		TS_LOG_ERR("%s:calibration fail\n", __func__);
		seq_printf(result_file, "1F-");
		goto software_reason_exit;
	}
	goto exit;

software_reason_exit:
	seq_printf(result_file, ";software_reason\n");
exit:
	kfree(configuration);
	kfree(result);
	kfree(cm_sensor_data);
	kfree(cm_sensor_column_delta);
	kfree(cm_sensor_row_delta);
	kfree(cp_sensor_rx_delta);
	kfree(cp_sensor_tx_delta);
	kfree(cp_sensor_rx_calibration);
	kfree(cp_sensor_tx_calibration);
	kfree(cp_sensor_rx_data);
	kfree(cp_sensor_tx_data);

	return 0;
}


int cm_cp_test_run(char *device_path, struct file *parameter_file,
		   struct file *configuration_file,
		   struct seq_file *result_file, int vdda, bool run_cm_test,
		   bool run_cp_test, bool *cm_test_pass, bool *cp_test_pass)
{
	struct configuration *configuration;
	struct result *result;
	/*char tmp_buf[5];*/
	uint32_t tx_num, rx_num, button_num;
	uint32_t sensor_assignment = 0;	/*if tx is column, then sensor_assignment is 1;*/
	uint32_t config_ver, revision_ctrl, device_id_high, device_id_low;
	bool skip_cm_button, skip_cp_button, skip_cp_sensor;
	int32_t *cm_sensor_raw_data = NULL;
	int *cm_sensor_data = NULL, *cm_button_data = NULL;
	int *cm_sensor_column_delta = NULL, *cm_sensor_row_delta = NULL,
			*cm_button_raw_data = NULL;
	struct gd_sensor *cm_gradient_col = NULL, *cm_gradient_row = NULL;
	int cm_sensor_average, cm_button_average;
	int cm_sensor_calibration, cm_button_calibration;
	int cm_sensor_delta, cm_button_delta;
	int32_t *cp_sensor_rx_raw_data = NULL, *cp_sensor_tx_raw_data = NULL,
			*cp_button_raw_data = NULL;
	int *cp_sensor_rx_data = NULL, *cp_sensor_tx_data = NULL,
			*cp_button_data = NULL;
	int cp_sensor_rx_calibration, cp_sensor_tx_calibration, cp_button_calibration;
	int *cp_sensor_rx_calibration_gen6l = NULL, *cp_sensor_tx_calibration_gen6l=NULL;
	int cp_sensor_rx_average, cp_sensor_tx_average, cp_button_average;
	int cp_sensor_rx_delta, cp_sensor_tx_delta, cp_button_delta = 0;
	int *cp_sensor_rx_delta_gen6l = NULL, *cp_sensor_tx_delta_gen6l = NULL;
	int ret = 0, i = 0;
	TS_LOG_INFO("%s: begin\n", __func__);

	configuration = kzalloc(sizeof(struct configuration), GFP_KERNEL);
	result = kzalloc(sizeof(struct result), GFP_KERNEL);
	if (!configuration || !result) {
		ret = -ENOMEM;
		TS_LOG_ERR("%s: kzalloc configuration result fail\n", __func__);
		seq_printf(result_file, "2F;-software_reason");
		goto exit;
	}

	memset(configuration, 0, sizeof(struct configuration));
	memset(result, 0, sizeof(struct result));

	ret = parameter_init(parameter_file);
	if (ret) {
		TS_LOG_ERR("%s: Fail initing parameters!\n", __func__);
		seq_printf(result_file, "2F;-software_reason");
		goto exit;
	}

	ret = configuration_get(configuration_file, configuration);
	if (ret) {
		TS_LOG_ERR("%s: Fail getting configuration\n", __func__);
		seq_printf(result_file, "2F;-software_reason");
		goto parameter_exit;
	}
	g_family_type = (uint8_t)configuration->family_type;
	TS_LOG_INFO("%s: g_family_type:%d\n", __func__,g_family_type);

	ret = pip_init(device_path, HID_DESCRIPTOR_REGISTER);
	if (ret) {
		TS_LOG_ERR("%s: Unable to init pip!\n", __func__);
		seq_printf(result_file, "2F;-software_reason");
		goto parameter_exit;
	}

	/* Suspend scanning */
	ret = pip_suspend_scanning();
	if (ret) {
		TS_LOG_ERR("%s: Unable to suspend scanning!\n", __func__);
		seq_printf(result_file, "2F;-software_reason");
		goto pip_exit;
	}

	ret = check_tests(&tx_num, &rx_num, &button_num,
			  &skip_cm_button, &skip_cp_sensor, &skip_cp_button);
	if (ret) {
		TS_LOG_ERR("%s: Unable to check_tests !\n", __func__);
		seq_printf(result_file, "2F;-software_reason");
		goto resume_scanning;
	}

	/*add get_extra_parameter item */
	ret =
	    get_extra_parameter(&sensor_assignment, &config_ver, &revision_ctrl,
				&device_id_high, &device_id_low);
	if (ret) {
		TS_LOG_ERR("%s:Unable to get_extra_parameter\n", __func__);
		seq_printf(result_file, "2F;-software_reason");
	} else {
		TS_LOG_INFO("%s: get_extra_parameter pass\n", __func__);
		result->sensor_assignment = sensor_assignment;
		result->config_ver = config_ver;
		result->revision_ctrl = revision_ctrl;
		result->device_id_high = device_id_high;
		result->device_id_low = device_id_low;
	}

	/*add short test item */
	result->short_test_pass = true;
	ret = pip_short_test();
	if (ret) {
		result->short_test_pass = false;
		TS_LOG_ERR("%s:Unable to do short test\n", __func__);
		goto resume_scanning;
	} else {
		TS_LOG_INFO("%s: short tets pass\n", __func__);
	}

	seq_printf(result_file, "2P-");

	if (run_cp_test && skip_cp_sensor)
		TS_LOG_INFO("%s:Cp sensor test is skipped\n", __func__);
	if (run_cp_test && skip_cp_button)
		TS_LOG_INFO("%s:Cp button test is skipped\n", __func__);
	if (run_cm_test && skip_cm_button)
		TS_LOG_INFO("%s:Cm button test is skipped\n", __func__);

	if (run_cm_test) {
        if(g_family_type == 2){ 
            ret = get_cm_uniformity_test_results_gen6l(vdda, tx_num, rx_num,
                              button_num,skip_cm_button,&cm_sensor_raw_data,
                              &cm_button_raw_data,&cm_sensor_data,
                              &cm_button_data,&cm_sensor_average,
                              &cm_button_average);
            if (ret) {
                seq_printf(result_file, "3F;-software_reason");
                TS_LOG_ERR("%s:Unable to run Cm uniformity test gen6!\n",
                    __func__);
                goto err_get_cm_test_results;
            }

            ret = get_cm_calibration_check_test_results_gen6l(vdda,
                                 skip_cm_button, &cm_sensor_calibration, 
                                 &cm_button_calibration);
            if (ret) {
                seq_printf(result_file, "3F;-software_reason");
		   TS_LOG_ERR ("%s:Unable to run Cm calibration check test gen6!\n",
			     __func__);
			goto free_buffers;
		}
	   }else{		
	       ret = get_cm_uniformity_test_results_gen6(vdda, tx_num, rx_num,
							  button_num,
							  skip_cm_button,
							  &cm_sensor_raw_data,
							  &cm_sensor_data,
							  &cm_sensor_average);
		if (ret) {
			seq_printf(result_file, "3F;-software_reason");
			TS_LOG_ERR
			    ("%s:Unable to run Cm uniformity test gen6!\n",
			     __func__);
			goto err_get_cm_test_results;
		}

		ret = get_cm_calibration_check_test_results_gen6(vdda,
								 skip_cm_button,
								 &cm_sensor_calibration);
		if (ret) {
			seq_printf(result_file, "3F;-software_reason");
			TS_LOG_ERR
			    ("%s:Unable to run Cm calibration check test gen6!\n",
			     __func__);
			goto free_buffers;
		}
	}

		cm_sensor_delta = ABS((cm_sensor_average -
				       cm_sensor_calibration) * 100 /
				      cm_sensor_average);

		TS_LOG_INFO("%s: cm_sensor_delta: %d\n", __func__,
			    cm_sensor_delta);
		
	if(g_family_type == 2){
		ret =
		    validate_cm_test_results_gen6l(configuration, result, tx_num,
 						  rx_num, button_num, skip_cm_button,
						  cm_sensor_data, cm_button_data,
						  cm_sensor_calibration, cm_button_calibration,
						  cm_sensor_average, cm_button_average,
						  cm_sensor_delta, cm_button_delta,
						  &cm_sensor_column_delta,
						  &cm_sensor_row_delta,
						  cm_test_pass);
		if (ret) {
			seq_printf(result_file, "3F;-software_reason");
			TS_LOG_ERR("%s:Fail validating Cm test results!\n",
				   __func__);
			goto free_buffers;
		}		
	}else{
		ret =
		    validate_cm_test_results_gen6(configuration, result, tx_num,
						  rx_num, button_num,
						  skip_cm_button,
						  cm_sensor_data,
						  cm_sensor_calibration,
						  cm_sensor_average,
						  cm_sensor_delta,
						  &cm_sensor_column_delta,
						  &cm_sensor_row_delta,
						  cm_test_pass);
		if (ret) {
			seq_printf(result_file, "3F;-software_reason");
			TS_LOG_ERR("%s:Fail validating Cm test results!\n",
				   __func__);
			goto free_buffers;
		}
	}
		result->cm_sensor_data = cm_sensor_data;
		result->cm_sensor_raw_data = cm_sensor_raw_data;
		result->cm_sensor_column_delta = cm_sensor_column_delta;
		result->cm_sensor_row_delta = cm_sensor_row_delta;
		result->cm_sensor_calibration = cm_sensor_calibration;
		result->cm_sensor_average = cm_sensor_average;
		result->cm_sensor_delta = cm_sensor_delta;
	}

	if (run_cp_test) {
		if(g_family_type == 2){
			ret = get_cp_calibration_check_test_results_gen6l(tx_num, rx_num,
						button_num, skip_cp_sensor, skip_cp_button,
						&cp_sensor_tx_raw_data, &cp_sensor_rx_raw_data,
						&cp_button_raw_data, &cp_sensor_tx_data,
						&cp_sensor_rx_data, &cp_button_data,
						&cp_sensor_tx_average, &cp_sensor_rx_average,
						&cp_button_average, 
						&cp_sensor_rx_calibration_gen6l,
						&cp_sensor_tx_calibration_gen6l,
						&cp_button_calibration);
			if (ret) {
				seq_printf(result_file, "4F;-software_reason");
				TS_LOG_ERR("%s:Unable to run Cp calibration check test!\n",
			    		 __func__);
				goto free_buffers;
			}
		}else{
			ret = get_cp_calibration_check_test_results_gen6(tx_num, rx_num,
								 button_num,
								 skip_cp_sensor,
								 skip_cp_button,
								 &cp_sensor_tx_raw_data,
								 &cp_sensor_rx_raw_data,
								 &cp_sensor_tx_data,
								 &cp_sensor_rx_data,
								 &cp_sensor_tx_average,
								 &cp_sensor_rx_average,
								 &cp_sensor_rx_calibration,
								 &cp_sensor_tx_calibration);
			if (ret) {
				seq_printf(result_file, "4F;-software_reason");
				TS_LOG_ERR
			   		 ("%s:Unable to run Cp calibration check test!\n",
			    		 __func__);
				goto free_buffers;
			}
		}

		if (!skip_cp_sensor) {
			if(g_family_type == 2) {
		 		cp_sensor_rx_delta_gen6l = kzalloc(rx_num * sizeof(int), GFP_KERNEL);
				cp_sensor_tx_delta_gen6l = kzalloc(tx_num * sizeof(int), GFP_KERNEL);
				if(!cp_sensor_rx_delta_gen6l || !cp_sensor_rx_delta_gen6l)
					goto free_buffers;
			
		 		for(i=0; i<rx_num; i++){
					cp_sensor_rx_delta_gen6l[i] = ABS((cp_sensor_rx_calibration_gen6l[i] -
								cp_sensor_rx_data[i])* 100/
								cp_sensor_rx_data[i]);
		 		}

				for(i=0; i<tx_num; i++){
					cp_sensor_tx_delta_gen6l[i] = ABS((cp_sensor_tx_calibration_gen6l[i] -
								cp_sensor_tx_data[i]) * 100/
								cp_sensor_tx_data[i]);
				}
#ifdef PRINT_CAP_DATA_DEBUG
				TS_LOG_INFO("%s: cp_sensor_rx_delta_gen6l[0..%d]: ", __func__, rx_num-1);
				for (i = 0; i < rx_num; i++)
					TS_LOG_INFO("%5d\n", cp_sensor_rx_delta_gen6l[i]);
				TS_LOG_INFO("\n");
				TS_LOG_INFO("%s: cp_sensor_tx_delta_gen6l[0..%d]: ", __func__, tx_num-1);
				for (i = 0; i < tx_num; i++)
					TS_LOG_INFO("%5d\n", cp_sensor_tx_delta_gen6l[i]);
				TS_LOG_INFO("\n");
#endif
				result->cp_sensor_rx_delta_gen6l = cp_sensor_rx_delta_gen6l;
				result->cp_sensor_tx_delta_gen6l = cp_sensor_tx_delta_gen6l;
				result->cp_sensor_rx_calibration_gen6l = cp_sensor_rx_calibration_gen6l;
				result->cp_sensor_tx_calibration_gen6l = cp_sensor_tx_calibration_gen6l;
		 	}else{
				cp_sensor_rx_delta = ABS((cp_sensor_rx_calibration -
						  cp_sensor_rx_average) * 100 /
						 cp_sensor_rx_average);

				cp_sensor_tx_delta = ABS((cp_sensor_tx_calibration -
						  cp_sensor_tx_average) * 100 /
						 cp_sensor_tx_average);
				TS_LOG_INFO
			   		 ("cp_sensor_rx_calibration: %d,cp_sensor_rx_average:%d\n",
			     			cp_sensor_rx_calibration, cp_sensor_rx_average);
				TS_LOG_INFO
			   		 ("cp_sensor_tx_calibration: %d,cp_sensor_tx_average:%d\n",
			     			cp_sensor_tx_calibration, cp_sensor_tx_average);
				TS_LOG_INFO("%s: cp_sensor_rx_delta: %d\n", __func__,
				    		cp_sensor_rx_delta);
				TS_LOG_INFO("%s: cp_sensor_tx_delta: %d\n", __func__,
				    		cp_sensor_tx_delta);
				result->cp_sensor_rx_delta = cp_sensor_rx_delta;
				result->cp_sensor_tx_delta = cp_sensor_tx_delta;
				result->cp_sensor_rx_calibration = cp_sensor_rx_calibration;
				result->cp_sensor_tx_calibration = cp_sensor_tx_calibration;
			}	 

			result->cp_sensor_rx_data = cp_sensor_rx_data;
			result->cp_sensor_tx_data = cp_sensor_tx_data;
			result->cp_sensor_rx_raw_data = cp_sensor_rx_raw_data;
			result->cp_sensor_tx_raw_data = cp_sensor_tx_raw_data;
			result->cp_sensor_rx_average = cp_sensor_rx_average;
			result->cp_sensor_tx_average = cp_sensor_tx_average;
		}

		/*using tsg6l device*/
		if(g_family_type == 2){
			ret = validate_cp_test_results_gen6l(configuration, result, tx_num,
						  rx_num, skip_cp_sensor, skip_cp_button, 
						  cp_sensor_rx_data, cp_sensor_tx_data,
						  cp_button_data, cp_sensor_rx_calibration,
						  cp_sensor_tx_calibration,
						  cp_button_calibration,
						  cp_sensor_rx_average,
						  cp_sensor_tx_average,
						  cp_button_average,
						  cp_sensor_rx_delta_gen6l,
						  cp_sensor_tx_delta_gen6l,
						  cp_button_delta,
						  cp_test_pass);
		
		}else{
			ret = validate_cp_test_results_gen6(configuration, result, tx_num,
						  rx_num, skip_cp_sensor,
						  cp_sensor_rx_data,
						  cp_sensor_tx_data,
						  cp_sensor_rx_calibration,
						  cp_sensor_tx_calibration,
						  cp_sensor_rx_average,
						  cp_sensor_tx_average,
						  cp_sensor_rx_delta,
						  cp_sensor_tx_delta,
						  cp_test_pass);
		}
		
		if (ret) {
			seq_printf(result_file, "7F;-software_reason");
			TS_LOG_ERR("%s:Fail validating Cp test results!\n",
				   __func__);
			goto free_buffers;
		}

		result->tx_num = tx_num;
		result->rx_num = rx_num;
		result->button_num = button_num;

		result->cm_test_run = run_cm_test;
		result->cp_test_run = run_cp_test;
		result->test_summary = result->cm_test_pass
		    && result->cp_test_pass;

		TS_LOG_INFO("%s,get over\n", __func__);
	}
	ret = result_save(result_file, configuration, result);
	if (ret) {
		TS_LOG_ERR("%s:Fail saving result\n", __func__);
		goto free_buffers;
	}

free_buffers:
	if (run_cp_test) {
		if (!skip_cp_sensor) {
			kfree(cp_sensor_rx_raw_data);
			cp_sensor_rx_raw_data = NULL;

			kfree(cp_sensor_tx_raw_data);
			cp_sensor_tx_raw_data = NULL;

			kfree(cp_sensor_rx_data);
			cp_sensor_rx_data = NULL;

			kfree(cp_sensor_tx_data);
			cp_sensor_tx_data = NULL;
			kfree(cp_sensor_rx_delta_gen6l);
			cp_sensor_rx_delta_gen6l = NULL;
			kfree(cp_sensor_tx_delta_gen6l);
			cp_sensor_tx_delta_gen6l = NULL;
		}
	}
err_get_cm_test_results:
	if (run_cm_test) {
		kfree(cm_sensor_column_delta);
		cm_sensor_column_delta = NULL;
		kfree(cm_sensor_row_delta);
		cm_sensor_row_delta = NULL;
		kfree(cm_sensor_raw_data);
		cm_sensor_raw_data = NULL;

		kfree(cm_sensor_data);
		cm_sensor_data = NULL;

		kfree(cm_gradient_col);
		cm_gradient_col = NULL;

		kfree(cm_gradient_row);
		cm_gradient_row = NULL;

	}

resume_scanning:
	if (pip_resume_scanning()) {
		TS_LOG_ERR("%s:Unable to resume scanning!\n", __func__);
		goto pip_exit;
	}

pip_exit:
	pip_exit();

parameter_exit:
	parameter_exit();

exit:
	kfree(result);
	result = NULL;

	kfree(configuration);
	configuration = NULL;

	return ret;
}
