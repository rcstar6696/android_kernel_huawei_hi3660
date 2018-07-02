/*
 * Synaptics DSX touchscreen driver
 *
 * Copyright (C) 2012 Synaptics Incorporated
 *
 * Copyright (C) 2012 Alexandra Chin <alexandra.chin@tw.synaptics.com>
 * Copyright (C) 2012 Scott Lin <scott.lin@tw.synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/ctype.h>
#include <linux/hrtimer.h>
#include "synaptics.h"
#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
#endif
#include "../../huawei_ts_kit.h"
#include "raw_data.h"

#define  CSV_TP_CAP_RAW_MIN "MutualRawMin"   /*1p/f*/
#define  CSV_TP_CAP_RAW_MAX "MutualRawMax"  /*1p/f*/
#define  CSV_TP_DELTA_ABS_TX_LIMIT "Tx_delta_abslimit"
#define  CSV_TP_DELTA_ABS_RX_LIMIT "Rx_delta_abslimit"
#define  CSV_TP_NOISE_DATA_LIMIT "noise_data_limit"
#define  CSV_TP_EE_SHORT_DATA_LIMIT "tddi_ee_short_data_limit"
#define  CSV_TP_SELFCAP_RAW_TX_RANGE "SelfCapRawTxRange"    /*6p/f*/
#define  CSV_TP_SELFCAP_RAW_RX_RANGE "SelfCapRawRxRange"
#define  CSV_TP_SELFCAP_NOIZE_TX_RANGE "SelfCapNoiseTxRange"   /*7p/f*/
#define  CSV_TP_SELFCAP_NOIZE_RX_RANGE "SelfCapNoiseRxRange"

#define WATCHDOG_TIMEOUT_S 2
#define FORCE_TIMEOUT_100MS 10
#define STATUS_WORK_INTERVAL 20	/* ms */
#define MAX_I2C_MSG_LENS 0x3F

#define RX_NUMBER 89		/*f01 control_base_addr + 57*/
#define TX_NUMBER 90		/*f01 control_base_addr + 58*/
#define RX_NUMBER_S3207 39	/*f11 control_base_addr + 39*/
#define TX_NUMBER_S3207 40	/*f11 control_base_addr + 40*/
#define FORCE_NUMER 9

#define BOOTLOADER_ID_OFFSET 0
#define BOOTLOADER_ID_OFFSET_V7 1

#define STATUS_IDLE 0
#define STATUS_BUSY 1

#define DATA_REPORT_INDEX_OFFSET 1
#define DATA_REPORT_DATA_OFFSET 3
#define CALIBRATION_INFO_OFFSET 0x0E

#define COMMAND_GET_REPORT 1
#define COMMAND_FORCE_CAL 2
#define COMMAND_FORCE_UPDATE 4
#define F54_READ_RATE_OFFSET 9

#define CONTROL_0_SIZE 1
#define CONTROL_1_SIZE 1
#define CONTROL_2_SIZE 2
#define CONTROL_3_SIZE 1
#define CONTROL_4_6_SIZE 3
#define CONTROL_7_SIZE 1
#define CONTROL_8_9_SIZE 3
#define CONTROL_10_SIZE 1
#define CONTROL_11_SIZE 2
#define CONTROL_12_13_SIZE 2
#define CONTROL_14_SIZE 1
#define CONTROL_15_SIZE 1
#define CONTROL_16_SIZE 1
#define CONTROL_17_SIZE 1
#define CONTROL_18_SIZE 1
#define CONTROL_19_SIZE 1
#define CONTROL_20_SIZE 1
#define CONTROL_21_SIZE 2
#define CONTROL_22_26_SIZE 7
#define CONTROL_27_SIZE 1
#define CONTROL_28_SIZE 2
#define CONTROL_29_SIZE 1
#define CONTROL_30_SIZE 1
#define CONTROL_31_SIZE 1
#define CONTROL_32_35_SIZE 8
#define CONTROL_36_SIZE 1
#define CONTROL_37_SIZE 1
#define CONTROL_38_SIZE 1
#define CONTROL_39_SIZE 1
#define CONTROL_40_SIZE 1
#define CONTROL_41_SIZE 1
#define CONTROL_42_SIZE 2
#define CONTROL_43_54_SIZE 13
#define CONTROL_55_56_SIZE 2
#define CONTROL_57_SIZE 1
#define CONTROL_58_SIZE 1
#define CONTROL_59_SIZE 2
#define CONTROL_60_62_SIZE 3
#define CONTROL_63_SIZE 1
#define CONTROL_64_67_SIZE 4
#define CONTROL_68_73_SIZE 8
#define CONTROL_74_SIZE 2
#define CONTROL_75_SIZE 1
#define CONTROL_76_SIZE 1
#define CONTROL_77_78_SIZE 2
#define CONTROL_79_83_SIZE 5
#define CONTROL_84_85_SIZE 2
#define CONTROL_86_SIZE 1
#define CONTROL_87_SIZE 1
#define CONTROL_88_SIZE 1

#define HIGH_RESISTANCE_DATA_SIZE 6
#define FULL_RAW_CAP_MIN_MAX_DATA_SIZE 4
#define TREX_DATA_SIZE 7

#define NO_AUTO_CAL_MASK 0x01
#define F54_BUF_LEN 80
#define TP_TEST_FAILED_REASON_LEN 20

#define TEST_PASS 1
#define TEST_FAIL 0
#define EE_SHORT_TEST_PASS 0
#define EE_SHORT_TEST_FAIL 1
#define SHORT_TEST_PASS "-5P"
#define SHORT_TEST_FAIL "-5F"
#define F54_TD43XX_EE_SHORT_SIZE 4
#define TD43XX_EE_SHORT_TEST_LEFT_SIZE 0x9
#define TD43XX_EE_SHORT_TEST_RIGHT_SIZE 0x9
#define SHIFT_ONE_BYTE 8
#define NORMAL_NUM_ONE 1
#define NORMAL_NUM_TWO 2
#define NORMAL_NUM_ONE_HUNDRED 100
#define HYBRID_BUF_SIZE 100
#define NOISE_DATA_LIMIT_SIZE 2
#define CSVFILE_NOISE_ROW 1
#define CSVFILE_NOISE_COLUMN 2
#define EE_SHORT_DATA_LIMIT_SIZE 2
#define CSVFILE_EE_SHORT_ROW 1
#define CSVFILE_EE_SHORT_COLUMN 2
#define LIMIT_ONE 0
#define LIMIT_TWO 1
#define CSV_PRODUCT_SYSTEM 1
#define CSV_ODM_SYSTEM 2
static char tp_test_failed_reason[TP_TEST_FAILED_REASON_LEN] = { "-software_reason" };
static unsigned short report_rate_offset = 38;
static char buf_f54test_result[F54_BUF_LEN] = { 0 };	/*store mmi test result*/
static int *tx_delta_buf = NULL;
static int *rx_delta_buf = NULL;
static signed int *g_td43xx_rt95_part_one = NULL;
static signed int *g_td43xx_rt95_part_two = NULL;
extern char synaptics_raw_data_limit_flag;
static int mmi_buf_size;
static int mmi_hybrid_abs_delta = 0;
static int rawdata_size;
extern struct dsm_client *ts_dclient;
extern struct synaptics_rmi4_data *rmi4_data;
static int synaptics_rmi4_f54_attention(void);
static int synaptics_get_threshold_from_csvfile(int columns, int rows, char* target_name, int32_t *data);

enum f54_ctr_work_rate {
	F54_AUTO_RATE = 0,
	F54_60LOW_RATE = 2,
	F54_120HI_RATE = 4,
};

enum f54_data_work_rate {
	F54_DATA120_RATE = 8,
	F54_DATA60_RATE = 4,
};

enum f54_report_types {
	F54_8BIT_IMAGE = 1,
	F54_16BIT_IMAGE = 2,
	F54_RAW_16BIT_IMAGE = 3,
	F54_HIGH_RESISTANCE = 4,
	F54_TX_TO_TX_SHORT = 5,
	F54_RX_TO_RX1 = 7,
	F54_TRUE_BASELINE = 9,
	F54_FULL_RAW_CAP_MIN_MAX = 13,
	F54_RX_OPENS1 = 14,
	F54_TX_OPEN = 15,
	F54_TX_TO_GROUND = 16,
	F54_RX_TO_RX2 = 17,
	F54_RX_OPENS2 = 18,
	F54_FULL_RAW_CAP = 19,
	F54_FULL_RAW_CAP_RX_COUPLING_COMP = 20,
	F54_SENSOR_SPEED = 22,
	F54_ADC_RANGE = 23,
	F54_TREX_OPENS = 24,
	F54_TREX_TO_GND = 25,
	F54_TREX_SHORTS = 26,
	F54_HYBRID_ABS_DELTA_CAP = 59,
	F54_HYBRID_SENSING_RAW_CAP = 63,
	F54_CALIBRATION = 84,
	F54_TD43XX_EE_SHORT = 95,
	INVALID_REPORT_TYPE = -1,
};

enum f54_rawdata_limit {
	RAW_DATA_UP = 0,
	RAW_DATA_LOW = 1,
	DELT_DATA_UP = 2,
	DELT_DATA_LOW = 3,
	RX_TO_RX_UP = 4,
	RX_TO_RX_LOW = 5,
	TX_TO_TX_UP = 6,
	TX_TO_TX_LOW = 7,
};

struct f54_query {
	union {
		struct {
			/* query 0 */
			unsigned char num_of_rx_electrodes;

			/* query 1 */
			unsigned char num_of_tx_electrodes;

			/* query 2 */
			unsigned char f54_query2_b0__1:2;
			unsigned char has_baseline:1;
			unsigned char has_image8:1;
			unsigned char f54_query2_b4__5:2;
			unsigned char has_image16:1;
			unsigned char f54_query2_b7:1;

			/* queries 3.0 and 3.1 */
			unsigned short clock_rate;

			/* query 4 */
			unsigned char touch_controller_family;

			/* query 5 */
			unsigned char has_pixel_touch_threshold_adjustment:1;
			unsigned char f54_query5_b1__7:7;

			/* query 6 */
			unsigned char has_sensor_assignment:1;
			unsigned char has_interference_metric:1;
			unsigned char has_sense_frequency_control:1;
			unsigned char has_firmware_noise_mitigation:1;
			unsigned char has_ctrl11:1;
			unsigned char has_two_byte_report_rate:1;
			unsigned char has_one_byte_report_rate:1;
			unsigned char has_relaxation_control:1;

			/* query 7 */
			unsigned char curve_compensation_mode:2;
			unsigned char f54_query7_b2__7:6;

			/* query 8 */
			unsigned char f54_query8_b0:1;
			unsigned char has_iir_filter:1;
			unsigned char has_cmn_removal:1;
			unsigned char has_cmn_maximum:1;
			unsigned char has_touch_hysteresis:1;
			unsigned char has_edge_compensation:1;
			unsigned char has_per_frequency_noise_control:1;
			unsigned char has_enhanced_stretch:1;

			/* query 9 */
			unsigned char has_force_fast_relaxation:1;
			unsigned char has_multi_metric_state_machine:1;
			unsigned char has_signal_clarity:1;
			unsigned char has_variance_metric:1;
			unsigned char has_0d_relaxation_control:1;
			unsigned char has_0d_acquisition_control:1;
			unsigned char has_status:1;
			unsigned char has_slew_metric:1;

			/* query 10 */
			unsigned char has_h_blank:1;
			unsigned char has_v_blank:1;
			unsigned char has_long_h_blank:1;
			unsigned char has_startup_fast_relaxation:1;
			unsigned char has_esd_control:1;
			unsigned char has_noise_mitigation2:1;
			unsigned char has_noise_state:1;
			unsigned char has_energy_ratio_relaxation:1;

			/* query 11 */
			unsigned char has_excessive_noise_reporting:1;
			unsigned char has_slew_option:1;
			unsigned char has_two_overhead_bursts:1;
			unsigned char has_query13:1;
			unsigned char has_one_overhead_burst:1;
			unsigned char f54_query11_b5:1;
			unsigned char has_ctrl88:1;
			unsigned char has_query15:1;

			/* query 12 */
			unsigned char number_of_sensing_frequencies:4;
			unsigned char f54_query12_b4__7:4;
		} __packed;
		unsigned char data[14];
	};
};

struct f54_control_0 {
	union {
		struct {
			unsigned char no_relax:1;
			unsigned char no_scan:1;
			unsigned char force_fast_relaxation:1;
			unsigned char startup_fast_relaxation:1;
			unsigned char gesture_cancels_sfr:1;
			unsigned char enable_energy_ratio_relaxation:1;
			unsigned char excessive_noise_attn_enable:1;
			unsigned char f54_control0_b7:1;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_1 {
	union {
		struct {
			unsigned char bursts_per_cluster:4;
			unsigned char f54_ctrl1_b4__7:4;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_2 {
	union {
		struct {
			unsigned short saturation_cap;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_3 {
	union {
		struct {
			unsigned char pixel_touch_threshold;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_4__6 {
	union {
		struct {
			/* control 4 */
			unsigned char rx_feedback_cap:2;
			unsigned char bias_current:2;
			unsigned char f54_ctrl4_b4__7:4;

			/* control 5 */
			unsigned char low_ref_cap:2;
			unsigned char low_ref_feedback_cap:2;
			unsigned char low_ref_polarity:1;
			unsigned char f54_ctrl5_b5__7:3;

			/* control 6 */
			unsigned char high_ref_cap:2;
			unsigned char high_ref_feedback_cap:2;
			unsigned char high_ref_polarity:1;
			unsigned char f54_ctrl6_b5__7:3;
		} __packed;
		struct {
			unsigned char data[3];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_7 {
	union {
		struct {
			unsigned char cbc_cap:3;
			unsigned char cbc_polarity:1;
			unsigned char cbc_tx_carrier_selection:1;
			unsigned char f54_ctrl7_b5__7:3;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_8__9 {
	union {
		struct {
			/* control 8 */
			unsigned short integration_duration:10;
			unsigned short f54_ctrl8_b10__15:6;

			/* control 9 */
			unsigned char reset_duration;
		} __packed;
		struct {
			unsigned char data[3];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_10 {
	union {
		struct {
			unsigned char noise_sensing_bursts_per_image:4;
			unsigned char f54_ctrl10_b4__7:4;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_11 {
	union {
		struct {
			unsigned short f54_ctrl11;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_12__13 {
	union {
		struct {
			/* control 12 */
			unsigned char slow_relaxation_rate;

			/* control 13 */
			unsigned char fast_relaxation_rate;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_14 {
	union {
		struct {
			unsigned char rxs_on_xaxis:1;
			unsigned char curve_comp_on_txs:1;
			unsigned char f54_ctrl14_b2__7:6;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_15n {
	unsigned char sensor_rx_assignment;
};

struct f54_control_15 {
	struct f54_control_15n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_16n {
	unsigned char sensor_tx_assignment;
};

struct f54_control_16 {
	struct f54_control_16n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_17n {
	unsigned char burst_count_b8__10:3;
	unsigned char disable:1;
	unsigned char f54_ctrl17_b4:1;
	unsigned char filter_bandwidth:3;
};

struct f54_control_17 {
	struct f54_control_17n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_18n {
	unsigned char burst_count_b0__7;
};

struct f54_control_18 {
	struct f54_control_18n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_19n {
	unsigned char stretch_duration;
};

struct f54_control_19 {
	struct f54_control_19n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_20 {
	union {
		struct {
			unsigned char disable_noise_mitigation:1;
			unsigned char f54_ctrl20_b1__7:7;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_21 {
	union {
		struct {
			unsigned short freq_shift_noise_threshold;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_22__26 {
	union {
		struct {
			/* control 22 */
			unsigned char f54_ctrl22;

			/* control 23 */
			unsigned short medium_noise_threshold;

			/* control 24 */
			unsigned short high_noise_threshold;

			/* control 25 */
			unsigned char noise_density;

			/* control 26 */
			unsigned char frame_count;
		} __packed;
		struct {
			unsigned char data[7];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_27 {
	union {
		struct {
			unsigned char iir_filter_coef;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_28 {
	union {
		struct {
			unsigned short quiet_threshold;
		} __packed;
		struct {
			unsigned char data[2];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_29 {
	union {
		struct {
			/* control 29 */
			unsigned char f54_ctrl29_b0__6:7;
			unsigned char cmn_filter_disable:1;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_30 {
	union {
		struct {
			unsigned char cmn_filter_max;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_31 {
	union {
		struct {
			unsigned char touch_hysteresis;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_32__35 {
	union {
		struct {
			/* control 32 */
			unsigned short rx_low_edge_comp;

			/* control 33 */
			unsigned short rx_high_edge_comp;

			/* control 34 */
			unsigned short tx_low_edge_comp;

			/* control 35 */
			unsigned short tx_high_edge_comp;
		} __packed;
		struct {
			unsigned char data[8];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_36n {
	unsigned char axis1_comp;
};

struct f54_control_36 {
	struct f54_control_36n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_37n {
	unsigned char axis2_comp;
};

struct f54_control_37 {
	struct f54_control_37n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_38n {
	unsigned char noise_control_1;
};

struct f54_control_38 {
	struct f54_control_38n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_39n {
	unsigned char noise_control_2;
};

struct f54_control_39 {
	struct f54_control_39n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_40n {
	unsigned char noise_control_3;
};

struct f54_control_40 {
	struct f54_control_40n *data;
	unsigned short address;
	unsigned char length;
};

struct f54_control_41 {
	union {
		struct {
			unsigned char no_signal_clarity:1;
			unsigned char f54_ctrl41_b1__7:7;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_57 {
	union {
		struct {
			unsigned char cbc_cap_0d:3;
			unsigned char cbc_polarity_0d:1;
			unsigned char cbc_tx_carrier_selection_0d:1;
			unsigned char f54_ctrl57_b5__7:3;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control_88 {
	union {
		struct {
			unsigned char tx_low_reference_polarity:1;
			unsigned char tx_high_reference_polarity:1;
			unsigned char abs_low_reference_polarity:1;
			unsigned char abs_polarity:1;
			unsigned char cbc_polarity:1;
			unsigned char cbc_tx_carrier_selection:1;
			unsigned char charge_pump_enable:1;
			unsigned char cbc_abs_auto_servo:1;
		} __packed;
		struct {
			unsigned char data[1];
			unsigned short address;
		} __packed;
	};
};

struct f54_control {
	struct f54_control_0 *reg_0;
	struct f54_control_1 *reg_1;
	struct f54_control_2 *reg_2;
	struct f54_control_3 *reg_3;
	struct f54_control_4__6 *reg_4__6;
	struct f54_control_7 *reg_7;
	struct f54_control_8__9 *reg_8__9;
	struct f54_control_10 *reg_10;
	struct f54_control_11 *reg_11;
	struct f54_control_12__13 *reg_12__13;
	struct f54_control_14 *reg_14;
	struct f54_control_15 *reg_15;
	struct f54_control_16 *reg_16;
	struct f54_control_17 *reg_17;
	struct f54_control_18 *reg_18;
	struct f54_control_19 *reg_19;
	struct f54_control_20 *reg_20;
	struct f54_control_21 *reg_21;
	struct f54_control_22__26 *reg_22__26;
	struct f54_control_27 *reg_27;
	struct f54_control_28 *reg_28;
	struct f54_control_29 *reg_29;
	struct f54_control_30 *reg_30;
	struct f54_control_31 *reg_31;
	struct f54_control_32__35 *reg_32__35;
	struct f54_control_36 *reg_36;
	struct f54_control_37 *reg_37;
	struct f54_control_38 *reg_38;
	struct f54_control_39 *reg_39;
	struct f54_control_40 *reg_40;
	struct f54_control_41 *reg_41;
	struct f54_control_57 *reg_57;
	struct f54_control_88 *reg_88;
};

struct f54_query_13 {
	union {
		struct {
			unsigned char has_ctrl86:1;
			unsigned char has_ctrl87:1;
			unsigned char has_ctrl87_sub0:1;
			unsigned char has_ctrl87_sub1:1;
			unsigned char has_ctrl87_sub2:1;
			unsigned char has_cidim:1;
			unsigned char has_noise_mitigation_enhancement:1;
			unsigned char has_rail_im:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_15 {
	union {
		struct {
			unsigned char has_ctrl90:1;
			unsigned char has_transmit_strength:1;
			unsigned char has_ctrl87_sub3:1;
			unsigned char has_query16:1;
			unsigned char has_query20:1;
			unsigned char has_query21:1;
			unsigned char has_query22:1;
			unsigned char has_query25:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_16 {
	union {
		struct {
			unsigned char has_query17:1;
			unsigned char has_data17:1;
			unsigned char has_ctrl92:1;
			unsigned char has_ctrl93:1;
			unsigned char has_ctrl94_query18:1;
			unsigned char has_ctrl95_query19:1;
			unsigned char has_ctrl99:1;
			unsigned char has_ctrl100:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f54_query_21 {
	union {
		struct {
			unsigned char has_abs_rx:1;
			unsigned char has_abs_tx:1;
			unsigned char has_ctrl91:1;
			unsigned char has_ctrl96:1;
			unsigned char has_ctrl97:1;
			unsigned char has_ctrl98:1;
			unsigned char has_data19:1;
			unsigned char has_query24_data18:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f55_query {
	union {
		struct {
			/* query 0 */
			unsigned char num_of_rx_electrodes;

			/* query 1 */
			unsigned char num_of_tx_electrodes;

			/* query 2 */
			unsigned char has_sensor_assignment:1;
			unsigned char has_edge_compensation:1;
			unsigned char curve_compensation_mode:2;
			unsigned char reserved:4;
		} __packed;
		unsigned char data[3];
	};
};

struct synaptics_rmi4_fn55_desc {
	unsigned short query_base_addr;
	unsigned short control_base_addr;
};
struct synaptics_fn54_statics_data {
	short RawimageAverage;
	short RawimageMaxNum;
	short RawimageMinNum;
	short RawimageNULL;
};
enum bl_version {
	V5 = 5,
	V6 = 6,
	V7 = 7,
};

struct synaptics_rmi4_f54_handle {
	bool no_auto_cal;
	unsigned char status;
	unsigned char intr_mask;
	unsigned char intr_reg_num;
	unsigned char *report_data;
	unsigned char bootloader_id[2];
	unsigned short query_base_addr;
	unsigned short control_base_addr;
	unsigned short data_base_addr;
	unsigned short command_base_addr;
	unsigned short fifoindex;
	unsigned int report_size;
	unsigned int data_buffer_size;

	enum bl_version bl_version;
	enum f54_report_types report_type;

	char *mmi_buf;
	int *rawdatabuf;
	u32 *hybridbuf;
	struct f54_query query;
	struct f54_query_13 query_13;
	struct f54_query_15 query_15;
	struct f54_query_16 query_16;
	struct f54_query_21 query_21;
	struct f54_control control;
	struct kobject *attr_dir;
	struct synaptics_fn54_statics_data raw_statics_data;
	struct synaptics_fn54_statics_data delta_statics_data;
	struct synaptics_rmi4_exp_fn_ptr *fn_ptr;
	struct synaptics_rmi4_data *rmi4_data;
	struct synaptics_rmi4_fn55_desc *fn55;
	struct synaptics_rmi4_fn_desc f34_fd;
};

static struct synaptics_rmi4_f54_handle *f54;

DECLARE_COMPLETION(synaptics_f54_remove_complete);

static void set_report_size(void)
{
	int rc = 0;
	switch (f54->report_type) {
	case F54_8BIT_IMAGE:
		f54->report_size =
		    f54->rmi4_data->num_of_rx * f54->rmi4_data->num_of_tx;
		break;
	case F54_16BIT_IMAGE:
	case F54_RAW_16BIT_IMAGE:
	case F54_TRUE_BASELINE:
	case F54_FULL_RAW_CAP:
	case F54_FULL_RAW_CAP_RX_COUPLING_COMP:
	case F54_SENSOR_SPEED:
		f54->report_size =
		    2 * f54->rmi4_data->num_of_rx * f54->rmi4_data->num_of_tx;
		break;
	case F54_HIGH_RESISTANCE:
		f54->report_size = HIGH_RESISTANCE_DATA_SIZE;
		break;
	case F54_TX_TO_TX_SHORT:
	case F54_TX_OPEN:
		f54->report_size = (f54->rmi4_data->num_of_tx + 7) / 8;
		break;
	case F54_TX_TO_GROUND:
		f54->report_size = 3;
		break;
	case F54_RX_TO_RX1:
		/*edw*/
		if (f54->rmi4_data->num_of_rx < f54->rmi4_data->num_of_tx)
			f54->report_size =
			    2 * f54->rmi4_data->num_of_rx *
			    f54->rmi4_data->num_of_rx;
		else
			f54->report_size =
			    2 * f54->rmi4_data->num_of_rx *
			    f54->rmi4_data->num_of_tx;
		break;
		/*edw*/
	case F54_RX_OPENS1:
		if (f54->rmi4_data->num_of_rx < f54->rmi4_data->num_of_tx)
			f54->report_size =
			    2 * f54->rmi4_data->num_of_rx *
			    f54->rmi4_data->num_of_rx;
		else
			f54->report_size =
			    2 * f54->rmi4_data->num_of_rx *
			    f54->rmi4_data->num_of_tx;
		break;
	case F54_FULL_RAW_CAP_MIN_MAX:
		f54->report_size = FULL_RAW_CAP_MIN_MAX_DATA_SIZE;
		break;
	case F54_RX_TO_RX2:
	case F54_RX_OPENS2:
		if (f54->rmi4_data->num_of_rx <= f54->rmi4_data->num_of_tx)
			f54->report_size = 0;
		else
			f54->report_size =
			    2 * f54->rmi4_data->num_of_rx *
			    (f54->rmi4_data->num_of_rx -
			     f54->rmi4_data->num_of_tx);
		break;
	case F54_ADC_RANGE:
		if (f54->query.has_signal_clarity) {
			rc = f54->fn_ptr->read(f54->rmi4_data,
					       f54->control.reg_41->address,
					       f54->control.reg_41->data,
					       sizeof(f54->control.reg_41->
						      data));
			if (rc < 0) {
				TS_LOG_INFO("Failed to read control reg_41\n");
				f54->report_size = 0;
				break;
			}
			if (!f54->control.reg_41->no_signal_clarity) {
				if (f54->rmi4_data->num_of_tx % 4)
					f54->rmi4_data->num_of_tx +=
					    4 - (f54->rmi4_data->num_of_tx % 4);
			}
		}
		f54->report_size =
		    2 * f54->rmi4_data->num_of_rx * f54->rmi4_data->num_of_tx;
		break;
	case F54_TREX_OPENS:
	case F54_TREX_TO_GND:
	case F54_TREX_SHORTS:
		f54->report_size = TREX_DATA_SIZE;
		break;
	case F54_CALIBRATION:
		f54->report_size = CALIBRATION_DATA_SIZE;
		break;
	case F54_HYBRID_ABS_DELTA_CAP:
	case F54_HYBRID_SENSING_RAW_CAP:
		f54->report_size = 4 * (f54->rmi4_data->num_of_rx + f54->rmi4_data->num_of_tx);
		break;
	case F54_TD43XX_EE_SHORT:
		f54->report_size = F54_TD43XX_EE_SHORT_SIZE
			* f54->rmi4_data->num_of_tx
			* f54->rmi4_data->num_of_rx;
		break;
	default:
		f54->report_size = 0;
	}

	return;
}

/* when the report type is  3 or 9, we call this function to  to find open
* transmitter electrodes, open receiver electrodes, transmitter-to-ground
* shorts, receiver-to-ground shorts, and transmitter-to-receiver shorts. */
static int f54_rawimage_report(void)
{
	short Rawimage;
	short Result = 0;
	int rows_size = 0;
	int columns_size = 0;
	char file_path[100] = {0};
	char file_name[64] = {0};
	int j;
	int i;
	int raw_cap_uplimit =
	    f54->rmi4_data->synaptics_chip_data->raw_limit_buf[RAW_DATA_UP];
	int raw_cap_lowlimit =
	    f54->rmi4_data->synaptics_chip_data->raw_limit_buf[RAW_DATA_LOW];
	int RawimageSum = 0;
	short RawimageAverage = 0;
	short RawimageMaxNum, RawimageMinNum;
	struct ts_rawdata_limit_tab limit_tab = {0};
	int *rawdata_from_chip = NULL;

	TS_LOG_INFO
	    ("f54_rawimage_report: rx = %d, tx = %d, mmibuf_size =%d, raw_cap_uplimit = %d,raw_cap_lowlimit = %d\n",
	     f54->rmi4_data->num_of_rx, f54->rmi4_data->num_of_tx, mmi_buf_size,
	     raw_cap_uplimit, raw_cap_lowlimit);

	RawimageMaxNum = (f54->mmi_buf[0]) | (f54->mmi_buf[1] << 8);
	RawimageMinNum = (f54->mmi_buf[0]) | (f54->mmi_buf[1] << 8);
	for (i = 0; i < mmi_buf_size; i += 2) {
		Rawimage = (f54->mmi_buf[i]) | (f54->mmi_buf[i + 1] << 8);
		RawimageSum += Rawimage;
		if (RawimageMaxNum < Rawimage)
			RawimageMaxNum = Rawimage;
		if (RawimageMinNum > Rawimage)
			RawimageMinNum = Rawimage;
	}
	RawimageAverage = RawimageSum / (mmi_buf_size / 2);
	f54->raw_statics_data.RawimageAverage = RawimageAverage;
	f54->raw_statics_data.RawimageMaxNum = RawimageMaxNum;
	f54->raw_statics_data.RawimageMinNum = RawimageMinNum;

	TS_LOG_INFO("raw_test_type:%d\n", f54->rmi4_data->synaptics_chip_data->raw_test_type);
	if (f54->rmi4_data->synaptics_chip_data->raw_test_type == RAW_DATA_TEST_TYPE_SINGLE_POINT) {

		rows_size = f54->rmi4_data->num_of_rx;
		columns_size = f54->rmi4_data->num_of_tx;

		TS_LOG_INFO("rows:%d, columns:%d\n", rows_size, columns_size);

		limit_tab.MutualRawMax =
			(int32_t*)kzalloc((rows_size+1)*(columns_size+1)*sizeof(int32_t), GFP_KERNEL);
		limit_tab.MutualRawMin =
			(int32_t*)kzalloc((rows_size+1)*(columns_size+1)*sizeof(int32_t), GFP_KERNEL);
		rawdata_from_chip = (int*)kzalloc((rows_size * columns_size)*sizeof(int), GFP_KERNEL);
		if (!limit_tab.MutualRawMax || !limit_tab.MutualRawMin || !rawdata_from_chip){
			TS_LOG_ERR("kzalloc error: MutualRawMax:%p, MutualRawMin:%p, rawdata_from_chip:%p\n",
				limit_tab.MutualRawMax, limit_tab.MutualRawMin, rawdata_from_chip);
			goto error_release_mem;
		}
		if (!strnlen(f54->rmi4_data->synaptics_chip_data->ts_platform_data->product_name, MAX_STR_LEN-1)
			|| !strnlen(f54->rmi4_data->synaptics_chip_data->chip_name, MAX_STR_LEN-1)
			|| !strnlen(f54->rmi4_data->rmi4_mod_info.project_id_string, SYNAPTICS_RMI4_PROJECT_ID_SIZE)
			|| f54->rmi4_data->module_name == NULL) {
			TS_LOG_INFO("Threshold file name is not detected\n");
			goto error_release_mem;
		}
		snprintf(file_name, sizeof(file_name), "%s_%s_%s_%s_raw.csv",
			f54->rmi4_data->synaptics_chip_data->ts_platform_data->product_name,
			f54->rmi4_data->synaptics_chip_data->chip_name,
			f54->rmi4_data->rmi4_mod_info.project_id_string,
			f54->rmi4_data->module_name);
#ifdef BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE
		snprintf(file_path, sizeof(file_path), "/product/etc/firmware/ts/%s", file_name);
#else
		snprintf(file_path, sizeof(file_path), "/odm/etc/firmware/ts/%s", file_name);
#endif
		TS_LOG_INFO("threshold file name:%s, rows_size=%d, columns_size=%d\n", file_path, rows_size, columns_size);

		Result   =ts_kit_parse_csvfile(file_path, CSV_TP_CAP_RAW_MAX, limit_tab.MutualRawMax, rows_size, columns_size);
		Result +=ts_kit_parse_csvfile(file_path, CSV_TP_CAP_RAW_MIN, limit_tab.MutualRawMin, rows_size, columns_size);
		if (0 != Result) {
			TS_LOG_ERR("csv file parse fail:%s\n", file_path);
			goto error_release_mem;
		} else {
			TS_LOG_INFO("rawdata compare start\n");
			for (i = 0, j = 0; i < mmi_buf_size; i += 2, j++) {
				rawdata_from_chip[j] = (f54->mmi_buf[i]) | (f54->mmi_buf[i + 1] << 8);
			}

			if (f54->rmi4_data->synaptics_chip_data->rawdata_arrange_type == TS_RAWDATA_TRANS_ABCD2CBAD
				|| f54->rmi4_data->synaptics_chip_data->rawdata_arrange_type == TS_RAWDATA_TRANS_ABCD2ADCB) {
				ts_kit_rotate_rawdata_abcd2cbad(rows_size, columns_size,
					 rawdata_from_chip, f54->rmi4_data->synaptics_chip_data->rawdata_arrange_type);
			}

			for (i = 0; i < rows_size; i++) {
				for (j = 0; j < columns_size; j++) {
					TS_LOG_DEBUG("\t%u", rawdata_from_chip[i*columns_size + j]);
				}
				TS_LOG_DEBUG("\n");
			}

			for (i = 0; i < (mmi_buf_size / 2); i++) {
				if (rawdata_from_chip[i] > limit_tab.MutualRawMin[i]
					&& rawdata_from_chip[i] < limit_tab.MutualRawMax[i]){
					Result++;
				}else{
					TS_LOG_ERR("error rawdata[%d]:%d out of range, min:%d, max:%d\n",
						i, rawdata_from_chip[i], limit_tab.MutualRawMin[i], limit_tab.MutualRawMax[i]);
				}
			}

			/* rawdata check done, release mem */
			if (limit_tab.MutualRawMax) {
				kfree(limit_tab.MutualRawMax);
				limit_tab.MutualRawMax = NULL;
			}
			if (limit_tab.MutualRawMin) {
				kfree(limit_tab.MutualRawMin);
				limit_tab.MutualRawMin = NULL;
			}
			if (rawdata_from_chip) {
				kfree(rawdata_from_chip);
				rawdata_from_chip = NULL;
			}
		}
	}else{
		for (i = 0; i < mmi_buf_size; i += 2) {
			Rawimage = (f54->mmi_buf[i]) | (f54->mmi_buf[i + 1] << 8);
			if (synaptics_raw_data_limit_flag) {
				raw_cap_uplimit = RawCapUpperLimit[i / 2];
				raw_cap_lowlimit = RawCapLowerLimit[i / 2];
			}
			if ((Rawimage >= raw_cap_lowlimit)
			    && (Rawimage <= raw_cap_uplimit)) {
				Result++;
			} else {
				TS_LOG_INFO("[%d,%d]\n", i / 2, Rawimage);
			}
		}
	}
	if (Result == (mmi_buf_size / 2)) {
		TS_LOG_DEBUG("rawdata is all right, Result = %d\n", Result);
		return 1;
	} else {
		TS_LOG_ERR("rawdata is out of range, Result = %d\n", Result);
		return 0;
	}
error_release_mem:
	if (limit_tab.MutualRawMax){
		kfree(limit_tab.MutualRawMax);
		limit_tab.MutualRawMax = NULL;
	}
	if (limit_tab.MutualRawMin){
		kfree(limit_tab.MutualRawMin);
		limit_tab.MutualRawMin = NULL;
	}
	if (rawdata_from_chip){
		kfree(rawdata_from_chip);
		rawdata_from_chip = NULL;
	}
	return 0;
}

static int write_to_f54_register(unsigned char report_type)
{
	unsigned char command;
	int result;
	unsigned char ready_report = 0xFF;
	int retry_times = 0;

	TS_LOG_INFO("report type = %d\n", report_type);

	command = report_type;
	/*set report type */
	if (F54_RX_TO_RX1 != command) {
		result =
		    f54->fn_ptr->write(f54->rmi4_data, f54->data_base_addr,
				       &command, 1);
		if (result < 0) {
			TS_LOG_ERR("Could not write report type to 0x%04x\n",
				   f54->data_base_addr);
			return result;
		}
	}
	mdelay(5);

	/*set get_report to 1 */
	command = (unsigned char)COMMAND_GET_REPORT;
	result =
	    f54->fn_ptr->write(f54->rmi4_data, f54->command_base_addr, &command,
			       1);

retry:
	msleep(100);
	result =
	    f54->fn_ptr->read(f54->rmi4_data, f54->command_base_addr,
			      &ready_report, 1);

	/*report is ready when COMMAND_GET_REPORT bit is clear */
	if ((ready_report & COMMAND_GET_REPORT) && retry_times < 20) {
		retry_times++;
	TS_LOG_INFO("report is not ready, retry times: %d, ready_report: %d\n",
		    retry_times + 1, ready_report);
		goto retry;
	}

	return result;
}

static int mmi_add_static_data(void)
{
	int i;

	i = strlen(buf_f54test_result);
	if (i >= F54_BUF_LEN) {
		return -EINVAL;
	}
	snprintf((buf_f54test_result + i), F54_BUF_LEN - i,
		 "[%4d,%4d,%4d]",
		 f54->raw_statics_data.RawimageAverage,
		 f54->raw_statics_data.RawimageMaxNum,
		 f54->raw_statics_data.RawimageMinNum);

	i = strlen(buf_f54test_result);
	if (i >= F54_BUF_LEN) {
		return -EINVAL;
	}
	snprintf((buf_f54test_result + i), F54_BUF_LEN - i,
		 "[%4d,%4d,%4d]",
		 f54->delta_statics_data.RawimageAverage,
		 f54->delta_statics_data.RawimageMaxNum,
		 f54->delta_statics_data.RawimageMinNum);

	return 0;
}

static int f54_deltarawimage_report(void)
{
	short Rawimage;
	int i;
	int delt_cap_uplimit =
	    f54->rmi4_data->synaptics_chip_data->raw_limit_buf[DELT_DATA_UP];
	int delt_cap_lowlimit =
	    f54->rmi4_data->synaptics_chip_data->raw_limit_buf[DELT_DATA_LOW];
	int DeltaRawimageSum = 0;
	short DeltaRawimageAverage = 0;
	short DeltaRawimageMaxNum, DeltaRawimageMinNum;
	short result = 0;
	struct ts_rawdata_limit_tab limit_tab = {0};
	int ret = 0;

	if (f54->rmi4_data->synaptics_chip_data->test_capacitance_via_csvfile) {
		limit_tab.unique_test = kzalloc(sizeof(struct ts_unique_capacitance_test), GFP_KERNEL);
		if (!limit_tab.unique_test) {
			TS_LOG_ERR("ts_unique_capacitance_test kzalloc error\n");
			ret = TEST_FAIL;
			goto error_release_mem;
		}
		limit_tab.unique_test->noise_data_limit =
				(int32_t*)kzalloc(NOISE_DATA_LIMIT_SIZE * sizeof(int32_t), GFP_KERNEL);
		if (!limit_tab.unique_test->noise_data_limit) {
			TS_LOG_ERR("noise_data_limit buffer kzalloc error\n");
			ret = TEST_FAIL;
			goto error_release_mem;
		}
		if (TEST_FAIL == synaptics_get_threshold_from_csvfile(CSVFILE_NOISE_COLUMN,
			CSVFILE_NOISE_ROW, CSV_TP_NOISE_DATA_LIMIT, limit_tab.unique_test->noise_data_limit)) {
			TS_LOG_ERR("csvfile get noise_data_limit err, use dts config.\n");
		} else {
			delt_cap_uplimit = limit_tab.unique_test->noise_data_limit[LIMIT_TWO];
			delt_cap_lowlimit = limit_tab.unique_test->noise_data_limit[LIMIT_ONE];
		}
	}

	TS_LOG_INFO
	    ("f54_deltarawimage_report enter, delt_cap_uplimit = %d, delt_cap_lowlimit = %d\n",
	     delt_cap_uplimit, delt_cap_lowlimit);

	DeltaRawimageMaxNum = (f54->mmi_buf[0]) | (f54->mmi_buf[1] << 8);
	DeltaRawimageMinNum = (f54->mmi_buf[0]) | (f54->mmi_buf[1] << 8);
	for (i = 0; i < mmi_buf_size; i += 2) {
		Rawimage = (f54->mmi_buf[i]) | (f54->mmi_buf[i + 1] << 8);
		DeltaRawimageSum += Rawimage;
		if (DeltaRawimageMaxNum < Rawimage)
			DeltaRawimageMaxNum = Rawimage;
		if (DeltaRawimageMinNum > Rawimage)
			DeltaRawimageMinNum = Rawimage;
	}
	DeltaRawimageAverage = DeltaRawimageSum / (mmi_buf_size / 2);
	f54->delta_statics_data.RawimageAverage = DeltaRawimageAverage;
	f54->delta_statics_data.RawimageMaxNum = DeltaRawimageMaxNum;
	f54->delta_statics_data.RawimageMinNum = DeltaRawimageMinNum;
	if (synaptics_raw_data_limit_flag) {
		delt_cap_lowlimit = NoiseCapLowerLimit;
		delt_cap_uplimit = NoiseCapUpperLimit;
	}
	for (i = 0; i < mmi_buf_size; i += 2) {
		Rawimage = (f54->mmi_buf[i]) | (f54->mmi_buf[i + 1] << 8);
		if ((Rawimage >= delt_cap_lowlimit)
		    && (Rawimage <= delt_cap_uplimit)) {
			result++;
		} else {
			TS_LOG_INFO("[%d,%d]\n", i / 2, Rawimage);
		}
	}

	if (result == (mmi_buf_size / 2)) {
		TS_LOG_DEBUG("deltadata is all right, Result = %d\n", result);
		ret = TEST_PASS;
	} else {
		TS_LOG_ERR("deltadata is out of range, Result = %d\n", result);
		ret = TEST_FAIL;
	}
error_release_mem:
	if (limit_tab.unique_test && limit_tab.unique_test->noise_data_limit){
		kfree(limit_tab.unique_test->noise_data_limit);
		limit_tab.unique_test->noise_data_limit = NULL;
	}
	if (limit_tab.unique_test) {
		kfree(limit_tab.unique_test);
		limit_tab.unique_test = NULL;
	}
	return ret;
}

static void mmi_deltacapacitance_test(void)
{
	unsigned char command;
	int result = 0;
	command = (unsigned char)F54_16BIT_IMAGE;
	TS_LOG_INFO("mmi_deltacapacitance_test called\n");
	write_to_f54_register(command);
	f54->report_type = command;
	result = synaptics_rmi4_f54_attention();
	if (result < 0) {
		TS_LOG_ERR("Failed to get data\n");
		strncat(buf_f54test_result, "-3F", MAX_STR_LEN);
		return;
	}
	result = f54_deltarawimage_report();
	if (1 == result) {
		strncat(buf_f54test_result, "-3P", MAX_STR_LEN);
	} else {
		TS_LOG_ERR
		    ("deltadata test is out of range, test result is 3F\n");
		strncat(buf_f54test_result, "-3F", MAX_STR_LEN);
		strncpy(tp_test_failed_reason, "-panel_reason",
			TP_TEST_FAILED_REASON_LEN);
	}
	return;
}

static void put_hybrid_data(int index)
{
	int i, j;
	u32 temp;

	for (i = 0, j = index; i < mmi_hybrid_abs_delta; i += 4, j++) {
		temp = (f54->mmi_buf[i]) | (f54->mmi_buf[i + 1] << 8) | (f54->mmi_buf[i + 2] << 16) | (f54->mmi_buf[i + 3] << 24);
		f54->hybridbuf[j] = temp;
	}
	TS_LOG_INFO("*******put_hybrid_data start****mmi_hybrid_abs_delta=%d****\n", mmi_hybrid_abs_delta);
	for (i = index; i < ((mmi_hybrid_abs_delta /4) + index); i++) {
		TS_LOG_INFO(" %d\n", f54->hybridbuf[i]);
	}
	TS_LOG_INFO ("*******put_hybrid_data end********\n");
}

static bool is_in_range(int inputdata, int uplimit, int downlimit)
{
     if ((inputdata >= downlimit)&& (inputdata <= uplimit))
         return true;
     else
         return false;
}

static void check_hybrid_raw_cap(void)
{
	int i;
	int index = mmi_hybrid_abs_delta / 4;
	int result = 0;
	char file_path[100] = {0};
	char file_name[64] = {0};
	int32_t  hybird_raw_rx_range[HYBRID_BUF_SIZE]={0};
	int32_t  hybird_raw_tx_range[HYBRID_BUF_SIZE]={0};
       int rx_num = f54->rmi4_data->num_of_rx;
       int tx_num = f54->rmi4_data->num_of_tx;

	snprintf(file_name, sizeof(file_name), "%s_%s_%s_%s_raw.csv",
			f54->rmi4_data->synaptics_chip_data->ts_platform_data->product_name,
			f54->rmi4_data->synaptics_chip_data->chip_name,
			f54->rmi4_data->rmi4_mod_info.project_id_string,
			f54->rmi4_data->module_name);
	#ifdef BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE
	snprintf(file_path, sizeof(file_path), "/product/etc/firmware/ts/%s", file_name);
	#else
	snprintf(file_path, sizeof(file_path), "/odm/etc/firmware/ts/%s", file_name);
	#endif

	TS_LOG_INFO("%s(%d), threshold file name:%s \n", __func__, __LINE__, file_path);

	result = ts_kit_parse_csvfile(file_path, CSV_TP_SELFCAP_RAW_TX_RANGE,
		hybird_raw_tx_range, 2, tx_num);
	if(0 != result){
	     TS_LOG_ERR("%s(%d), ts_kit_parse_csvfile  parse tx range fail result=%dis 6F\n", __func__, __LINE__, result);
	     goto test_fail;
	}

	for(i = 0; i < tx_num; i++){
		if( false == is_in_range(f54->hybridbuf[index+i+rx_num],
			hybird_raw_tx_range[i+tx_num], hybird_raw_tx_range[i]) ) {
		    TS_LOG_ERR("%s(%d), check_hybrid_raw_cap test is out of range, test result is 6F\n", __func__, __LINE__);
		    strncpy(tp_test_failed_reason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
		    goto test_fail;
		}
	}
	result = ts_kit_parse_csvfile(file_path, CSV_TP_SELFCAP_RAW_RX_RANGE,
		hybird_raw_rx_range, 2, rx_num);
	if(0 != result){
	     TS_LOG_ERR("%s(%d), ts_kit_parse_csvfile  parse rx fail result=%dis 6F\n", __func__, __LINE__, result);
	     goto test_fail;
	}

	for(i = 0; i < rx_num; i++){
		if(false == is_in_range(f54->hybridbuf[index+i],
			hybird_raw_rx_range[i+rx_num], hybird_raw_rx_range[i]) ){
		    TS_LOG_ERR("%s(%d), check_hybrid_raw_cap test is out of range, test result is 6F\n", __func__, __LINE__);
		    strncpy(tp_test_failed_reason, "-panel_reason",TP_TEST_FAILED_REASON_LEN);
		    goto test_fail;
		}
	}

	TS_LOG_INFO("%s(%d), hybrid_abs_delta_report_size = %d. test result is 6P \n", __func__, __LINE__, f54->report_size);
	strncat(buf_f54test_result, "-6P", MAX_STR_LEN);
       return;

test_fail:
	strncat(buf_f54test_result, "-6F", MAX_STR_LEN);
	return;
}

static void check_hybrid_abs_delta(void)
{
	int i;
	int index = 0;
	char file_path[100] = {0};
	char file_name[64] = {0};
	int result =0;
	int32_t  hybird_abs_rx_range[HYBRID_BUF_SIZE]={0};
	int32_t  hybird_abs_tx_range[HYBRID_BUF_SIZE]={0};
       int rx_num = f54->rmi4_data->num_of_rx;
       int tx_num = f54->rmi4_data->num_of_tx;

	snprintf(file_name, sizeof(file_name), "%s_%s_%s_%s_raw.csv",
			f54->rmi4_data->synaptics_chip_data->ts_platform_data->product_name,
			f54->rmi4_data->synaptics_chip_data->chip_name,
			f54->rmi4_data->rmi4_mod_info.project_id_string,
			f54->rmi4_data->module_name);
	#ifdef BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE
	snprintf(file_path, sizeof(file_path), "/product/etc/firmware/ts/%s", file_name);
	#else
	snprintf(file_path, sizeof(file_path), "/odm/etc/firmware/ts/%s", file_name);
	#endif

	TS_LOG_INFO("%s(%d), threshold file name:%s \n", __func__, __LINE__, file_path);

	result = ts_kit_parse_csvfile(file_path, CSV_TP_SELFCAP_NOIZE_TX_RANGE,
		hybird_abs_tx_range, 2, tx_num);
	if(0 != result){
	     TS_LOG_ERR("%s(%d), ts_kit_parse_csvfile  parse tx fail result=%dis 7F\n", __func__, __LINE__, result);
	     goto test_fail;
	}

	for(i = 0; i < tx_num; i++){
		if(false == is_in_range(f54->hybridbuf[index+i+rx_num],
			hybird_abs_tx_range[i+tx_num], hybird_abs_tx_range[i]) ){
		    TS_LOG_ERR("%s(%d), hybrid_abs_delta test is out of range, test result is 7F\n", __func__, __LINE__);
		    strncpy(tp_test_failed_reason, "-panel_reason",
			TP_TEST_FAILED_REASON_LEN);
		    goto test_fail;
		}
	}

	result = ts_kit_parse_csvfile(file_path, CSV_TP_SELFCAP_NOIZE_RX_RANGE,
		hybird_abs_rx_range, 2, rx_num);
	if(0 != result){
	     TS_LOG_ERR("%s(%d), ts_kit_parse_csvfile  parse rx fail result=%dis 7F\n", __func__, __LINE__, result);
	     goto test_fail;
	}

	for(i = 0; i < rx_num; i++){
		if(false == is_in_range(f54->hybridbuf[index+i],
			hybird_abs_rx_range[i+rx_num], hybird_abs_rx_range[i])){
		    TS_LOG_ERR("%s(%d), hybrid_abs_delta test is out of range, test result is 7F\n", __func__, __LINE__);
		    strncpy(tp_test_failed_reason, "-panel_reason",
			TP_TEST_FAILED_REASON_LEN);
		    goto test_fail;
		}
	}

	TS_LOG_INFO("%s(%d), hybrid_abs_delta_report_size = %d. test result is 7P \n", f54->report_size);
	strncat(buf_f54test_result, "-7P", MAX_STR_LEN);
       return;

test_fail:
	strncat(buf_f54test_result, "-7F", MAX_STR_LEN);
	return;
}

static int mmi_hybrid_abs_delta_test(void)
{
	unsigned char command;
	int result = 0;
	command = (unsigned char)F54_HYBRID_ABS_DELTA_CAP;
	TS_LOG_INFO("%s(%d), mmi_hybrid_abs_delta_test called\n", __func__, __LINE__);
	write_to_f54_register(command);
	f54->report_type = command;
	result = synaptics_rmi4_f54_attention();
	if (result < 0) {
		TS_LOG_ERR("%s(%d), Failed to get data\n", __func__, __LINE__);
		strncat(buf_f54test_result, "-7F", MAX_STR_LEN);
	}
	else{
		put_hybrid_data(0);
		check_hybrid_abs_delta();
	}
	return result;
}

static int mmi_hybrid_raw_cap_test(void)
{
	unsigned char command;
	int result = 0;
	command = (unsigned char)F54_HYBRID_SENSING_RAW_CAP;
	TS_LOG_INFO("%s(%d), mmi_hybrid_raw_cap_test called\n", __func__, __LINE__);
	write_to_f54_register(command);
	f54->report_type = command;
	result = synaptics_rmi4_f54_attention();
	if (result < 0) {
		TS_LOG_ERR("%s(%d), Failed to get data\n", __func__, __LINE__);
		strncat(buf_f54test_result, "-6F", MAX_STR_LEN);
	}
	else{
		mmi_hybrid_abs_delta = f54->report_size;
	       put_hybrid_data(mmi_hybrid_abs_delta / 4);
	       check_hybrid_raw_cap();
	}
	return result;
}

static short FindMedian(short* pdata, int num)
{
	int i,j;
	short temp;
	short *value;
	short median;

	value = (short *)kzalloc( num * sizeof(short), GFP_KERNEL);
	if (!value) {
		TS_LOG_ERR("%s(%d), failed to malloc\n", __func__, __LINE__);
		return -ENOMEM;
	}
	for(i = 0; i < num; i++) {
		*(value+i) = *(pdata+i);
	}

	/*sorting*/
	for (i = 1; i <= num-1; i++) {
		for (j = 1; j <= num-i; j++) {
			if (*(value+j-1) <= *(value+j)) {
				temp = *(value+j-1);
				*(value+j-1) = *(value+j);
				*(value+j) = temp;
			}
			else {
				continue;
			}
		}
	}
	/*calculation of median*/
	if (0 == num % NORMAL_NUM_TWO) {
		median = (*(value + (num/NORMAL_NUM_TWO - NORMAL_NUM_ONE)) +
			*(value + (num/NORMAL_NUM_TWO)))/NORMAL_NUM_TWO;
	}
	else {
		median = *(value+(num/NORMAL_NUM_TWO));
	}

	if(value) {
		kfree(value);
	}

	return median;
}

static int td43xx_ee_short_normalize_data(signed short * image)
{
	int retval = 0;
	int i = 0, j = 0;
	int tx_num = f54->rmi4_data->num_of_tx;
	int rx_num = f54->rmi4_data->num_of_rx;
	int part_two_limit = f54->rmi4_data->synaptics_chip_data->tddi_ee_short_test_parttwo_limit;
	unsigned char left_size = TD43XX_EE_SHORT_TEST_LEFT_SIZE; //0x09
	unsigned char right_size = TD43XX_EE_SHORT_TEST_RIGHT_SIZE;	//0x09
	signed short *report_data_16 = NULL;
	signed short *left_median = NULL;
	signed short *right_median = NULL;
	signed short *left_column_buf = NULL;
	signed short *right_column_buf = NULL;
	signed int temp = 0;
	char buf[MAX_CAP_DATA_SIZE_FOR_EESHORT] = {0};
	int tx_rx_delta_size = 0;
	int lens = 0;
	if (!image) {
		TS_LOG_ERR("%s(%d), td43xx_ee_short_data image is NULL\n", __func__, __LINE__);
		return -ENOMEM;
	}
	if(rx_num > 200){//rx's max number possibly
		TS_LOG_ERR("%s(%d), rx num is too large\n", __func__, __LINE__);
		return -EINVAL;
	}
	right_median = (unsigned short *) kzalloc(rx_num * sizeof(short), GFP_KERNEL);
	if (!right_median) {
		retval = -ENOMEM;
		TS_LOG_ERR("%s(%d), failed to malloc right_median\n", __func__, __LINE__);
		goto exit;
	}

	left_median = (unsigned short *) kzalloc(rx_num * sizeof(short), GFP_KERNEL);
	if (!left_median) {
		retval = -ENOMEM;
		TS_LOG_ERR("%s(%d), failed to malloc left_median\n", __func__, __LINE__);
		goto exit;
	}

	right_column_buf = (unsigned short *) kzalloc(right_size * rx_num * sizeof(short), GFP_KERNEL);
	if (!right_column_buf ) {
		retval = -ENOMEM;
		TS_LOG_ERR("%s(%d), failed to malloc right_column_buf\n", __func__, __LINE__);
		goto exit;
	}

	left_column_buf = (unsigned short *) kzalloc(left_size * rx_num * sizeof(short), GFP_KERNEL);
	if (!left_column_buf ) {
		retval = -ENOMEM;
		TS_LOG_ERR("%s(%d), failed to malloc left_column_buf\n", __func__, __LINE__);
		goto exit;
	}

	report_data_16 = image;
	for (i = 0; i < rx_num; i++) {
		for (j = 0; j < right_size; j++) {
			right_column_buf[i * right_size + j] =
					report_data_16[j * rx_num + i];
		}
	}

	report_data_16 = image + right_size * rx_num;
	for (i = 0; i < rx_num; i++) {
		for (j = 0; j < left_size; j++) {
			left_column_buf[i * left_size + j] =
					report_data_16[j * rx_num + i];
		}
	}

	for (i = 0; i < rx_num; i++) {
		right_median[i] = FindMedian(right_column_buf + i * right_size, right_size);
		left_median[i] = FindMedian(left_column_buf + i * left_size, left_size);
		if (-ENOMEM == right_median[i] || -ENOMEM == left_median[i]) {
			TS_LOG_ERR("failed to get midian[%d] value\n", i);
			retval = -ENOMEM;
			goto exit;
		}
		if (! right_median[i] || ! left_median[i]) {
			TS_LOG_ERR("the median value is 0.\n");
			retval = -EINVAL;
			goto exit;
		}
		TS_LOG_DEBUG("right_median[%d] = %d\n", i, right_median[i]);
		TS_LOG_DEBUG("left_median[%d] = %d\n", i, left_median[i]);
	}
	if (NULL != g_td43xx_rt95_part_two) {
		if (!tx_num || !rx_num) {
			retval = -ENOMEM;
			TS_LOG_ERR("%s(%d),tx_num=%d rx_num=%d\n", __func__, __LINE__,tx_num,rx_num);
			goto exit;
		} else {
			tx_rx_delta_size = (TX_RX_BUF_MAX<=(tx_num*rx_num*MAX_CAP_DATA_SIZE))?TX_RX_BUF_MAX:(tx_num*rx_num*MAX_CAP_DATA_SIZE);
		}
		snprintf(g_td43xx_rt95_part_two, PART_ONE_LIMIT_LENGTH, "%s\n", "part_two:");
		lens += PART_ONE_LIMIT_LENGTH;
	} else {
		retval = -ENOMEM;
		TS_LOG_ERR("%s(%d), g_td43xx_rt95_part_two is NULL\n", __func__, __LINE__);
		goto exit;
	}
	for (i = 0; i < tx_num; i++) {
		for (j = 0; j < rx_num; j++) {
			if (i < right_size) {
				temp = (unsigned int) image[i * rx_num + j];
				temp = temp * NORMAL_NUM_ONE_HUNDRED / right_median[j];	/*100 for avoid decimal number*/
				snprintf(buf, sizeof(buf),"%3d,", temp);
				lens += sizeof(buf);
				if (lens > tx_rx_delta_size) {
					retval = -ENOMEM;
					TS_LOG_ERR("%s(%d), lens is %d larger than buffer\n", __func__, __LINE__,lens);
					goto exit;
				}
				strncat(g_td43xx_rt95_part_two, buf ,sizeof(buf));
				if (!((j+EESHORT_PRINT_SHIFT_LENGTH)%rx_num)) {
					lens += EESHORT_PRINT_SHIFT_LENGTH;
					if (lens > tx_rx_delta_size) {
						retval = -ENOMEM;
						TS_LOG_ERR("%s(%d), lens is %d larger than buffer\n", __func__, __LINE__,lens);
						goto exit;
					}
					strncat(g_td43xx_rt95_part_two, "\n", EESHORT_PRINT_SHIFT_LENGTH);
				}
			}
			else {
				temp = (unsigned int) image[i * rx_num + j];
				temp = temp * NORMAL_NUM_ONE_HUNDRED / left_median[j]; /*100 for avoid decimal number*/
				snprintf(buf, sizeof(buf),"%3d,", temp);
				lens += sizeof(buf);
				if (lens > tx_rx_delta_size) {
					retval = -ENOMEM;
					TS_LOG_ERR("%s(%d), lens is %d larger than buffer\n", __func__, __LINE__,lens);
					goto exit;
				}
				strncat(g_td43xx_rt95_part_two, buf ,sizeof(buf));
				if (!((j+EESHORT_PRINT_SHIFT_LENGTH)%rx_num)) {
					lens += EESHORT_PRINT_SHIFT_LENGTH;
					if (lens > tx_rx_delta_size) {
						retval = -ENOMEM;
						TS_LOG_ERR("%s(%d), lens is %d larger than buffer\n", __func__, __LINE__,lens);
						goto exit;
					}
					strncat(g_td43xx_rt95_part_two, "\n", EESHORT_PRINT_SHIFT_LENGTH);
				}
			}
			if (temp < part_two_limit) {
				image[i * rx_num + j] = 1;
			}
			else {
				image[i * rx_num + j] = 0;
			}
		}
	}

exit:
	if (right_median)
		kfree(right_median);
	if (left_median)
		kfree(left_median);
	if (right_column_buf)
		kfree(right_column_buf);
	if (left_column_buf)
		kfree(left_column_buf);
	return retval;
}

static int td43xx_ee_short_report(void)
{
	int i = 0, j = 0, k = 0;
	int tx_num = f54->rmi4_data->num_of_tx;
	int rx_num = f54->rmi4_data->num_of_rx;
	signed short *td43xx_rt95_part_one = NULL;
	signed short *td43xx_rt95_part_two = NULL;
	char *td43xx_ee_short_data = NULL;
	unsigned int td43xx_rt95_report_size = tx_num * rx_num * 2;
	int TestResult = TEST_PASS;
	int retval = 0;
	int part_one_limit = f54->rmi4_data->synaptics_chip_data->tddi_ee_short_test_partone_limit;
	int part_two_limit = f54->rmi4_data->synaptics_chip_data->tddi_ee_short_test_parttwo_limit;
	unsigned char *buffer = f54->report_data;
	struct ts_rawdata_limit_tab limit_tab = {0};

	char buf[MAX_CAP_DATA_SIZE_FOR_EESHORT] = {0};
	int tx_rx_delta_size = 0;
	int lens = 0;
	if (!buffer) {
		TS_LOG_ERR("mmi_buf data is NULL\n");
		return TEST_FAIL;
	}

	if (f54->rmi4_data->synaptics_chip_data->test_capacitance_via_csvfile) {
		limit_tab.unique_test = kzalloc(sizeof(struct ts_unique_capacitance_test), GFP_KERNEL);
		if (!limit_tab.unique_test) {
			TS_LOG_ERR("ts_unique_capacitance_test kzalloc error\n");
			TestResult = TEST_FAIL;
			goto exit;
		}
		limit_tab.unique_test->ee_short_data_limit =
				(int32_t*)kzalloc(EE_SHORT_DATA_LIMIT_SIZE * sizeof(int32_t), GFP_KERNEL);
		if (!limit_tab.unique_test->ee_short_data_limit) {
			TS_LOG_ERR("ee_short_data_limit buffer kzalloc error\n");
			TestResult = TEST_FAIL;
			goto exit;
		}
		if (TEST_FAIL == synaptics_get_threshold_from_csvfile(CSVFILE_EE_SHORT_COLUMN,
			CSVFILE_EE_SHORT_ROW, CSV_TP_EE_SHORT_DATA_LIMIT, limit_tab.unique_test->ee_short_data_limit)) {
			TS_LOG_ERR("csvfile get ee_short_data_limit err, use dts config.\n");
		} else {
			part_one_limit = limit_tab.unique_test->ee_short_data_limit[LIMIT_ONE];
			part_two_limit = limit_tab.unique_test->ee_short_data_limit[LIMIT_TWO];
		}
	}

	TS_LOG_INFO("%s: report_type=%d,tx=%d,rx=%d,limit(%d, %d) #%d\n",
		__func__, f54->report_type, tx_num, rx_num, part_one_limit, part_two_limit,__LINE__);
	if (!part_one_limit || !part_two_limit) {
		TS_LOG_ERR("td43xx ee_short test limit is NULL\n");
		TestResult = TEST_FAIL;
		goto exit;
	}

	td43xx_ee_short_data = kzalloc(tx_num * rx_num, GFP_KERNEL);
	if (!td43xx_ee_short_data) {
		TS_LOG_ERR("%s(%d), failed to malloc td43xx_ee_short_data\n", __func__, __LINE__);
		TestResult = TEST_FAIL;
		goto exit;
	}

	td43xx_rt95_part_one = kzalloc(td43xx_rt95_report_size, GFP_KERNEL);
	if (!td43xx_rt95_part_one) {
		TS_LOG_ERR("%s(%d), failed to malloc td43xx_rt95_part_one\n", __func__, __LINE__);
		TestResult = TEST_FAIL;
		goto exit;
	}

	td43xx_rt95_part_two = kzalloc(td43xx_rt95_report_size, GFP_KERNEL);
	if (!td43xx_rt95_part_two) {
		TS_LOG_ERR("%s(%d), failed to malloc td43xx_rt95_part_two\n", __func__, __LINE__);
		TestResult = TEST_FAIL;
		goto exit;
	}
	if (NULL != g_td43xx_rt95_part_one) {
		if (!tx_num || !rx_num) {
			TS_LOG_ERR("%s(%d),tx_num=%d rx_num=%d\n", __func__, __LINE__,tx_num,rx_num);
			TestResult = TEST_FAIL;
			goto exit;
		} else {
			tx_rx_delta_size = (TX_RX_BUF_MAX<=(tx_num*rx_num*MAX_CAP_DATA_SIZE))?TX_RX_BUF_MAX:(tx_num*rx_num*MAX_CAP_DATA_SIZE);
		}
		snprintf(g_td43xx_rt95_part_one, PART_ONE_LIMIT_LENGTH, "%s\n", "part_one:");
		lens += PART_ONE_LIMIT_LENGTH;
	} else {
		TestResult = TEST_FAIL;
		TS_LOG_ERR("%s(%d), g_td43xx_rt95_part_one is NULL\n", __func__, __LINE__);
		goto exit;
	}
	for (i = 0, k = 0; i < tx_num * rx_num; i++) {
		td43xx_rt95_part_one[i] = buffer[k] |
								(buffer[k + 1]) << SHIFT_ONE_BYTE;
		snprintf(buf, sizeof(buf),"%3d,", td43xx_rt95_part_one[i]);
		lens += sizeof(buf);
		if (lens > tx_rx_delta_size) {
			TestResult = TEST_FAIL;
			TS_LOG_ERR("%s(%d), lens is %d larger than buffer\n", __func__, __LINE__,lens);
			goto exit;
		}
		strncat(g_td43xx_rt95_part_one, buf ,sizeof(buf));
		if (!((i+EESHORT_PRINT_SHIFT_LENGTH)%rx_num)) {
			lens += EESHORT_PRINT_SHIFT_LENGTH;
			if (lens > tx_rx_delta_size) {
				TestResult = TEST_FAIL;
				TS_LOG_ERR("%s(%d), lens is %d larger than buffer\n", __func__, __LINE__,lens);
				goto exit;
			}
			strncat(g_td43xx_rt95_part_one, "\n", EESHORT_PRINT_SHIFT_LENGTH);
		}
		td43xx_rt95_part_one[i] = (td43xx_rt95_part_one[i] > part_one_limit) ? EE_SHORT_TEST_FAIL : EE_SHORT_TEST_PASS;
		k += NORMAL_NUM_TWO;
	}

	for (i = 0, k = td43xx_rt95_report_size; i < tx_num * rx_num; i++) {
		td43xx_rt95_part_two[i] = buffer[k] |
								(buffer[k + 1]) << SHIFT_ONE_BYTE;
		k += NORMAL_NUM_TWO;
	}

	retval = td43xx_ee_short_normalize_data(td43xx_rt95_part_two);
	if (retval < 0) {
		TS_LOG_ERR("%s(%d), td43xx_ee_short_normalize_data failed\n", __func__, __LINE__);
		TestResult = TEST_FAIL;
		goto exit;
	}

	for (i = 0; i < tx_num; i++) {
		for (j = 0; j < rx_num; j++) {
			td43xx_ee_short_data[i*rx_num +j] =
			(unsigned char)(td43xx_rt95_part_one[i * rx_num + j]) ||
				td43xx_rt95_part_two[i * rx_num + j];
			if (0 != td43xx_ee_short_data[i * rx_num + j]) {
				TestResult = TEST_FAIL;
				TS_LOG_ERR("td43xx_ee_short_data:[%d, %d]%d\n",
					i, j, td43xx_ee_short_data[i * tx_num + j]);
			}
		}
	}

exit:
	if (limit_tab.unique_test && limit_tab.unique_test->ee_short_data_limit){
		kfree(limit_tab.unique_test->ee_short_data_limit);
		limit_tab.unique_test->ee_short_data_limit = NULL;
	}
	if (limit_tab.unique_test) {
		kfree(limit_tab.unique_test);
		limit_tab.unique_test = NULL;
	}
	if (td43xx_rt95_part_one)
		kfree(td43xx_rt95_part_one);
	if (td43xx_rt95_part_two)
		kfree(td43xx_rt95_part_two);
	if (td43xx_ee_short_data)
		kfree(td43xx_ee_short_data);

	return TestResult;
}

static void mmi_trex_shorts_test(void)
{
	unsigned char command = 0;
	unsigned char i = 0;
	int result = 0;
	char *buf_backup = NULL;

	if(NULL == f54) {
		TS_LOG_ERR("%s: f54 is NULL\n", __func__);
		goto param_nul;
	}
	if(NULL == f54->rmi4_data) {
		TS_LOG_ERR("%s: f54->rmi4_data is NULL\n", __func__);
		goto param_nul;
	}
	if(NULL == f54->rmi4_data->synaptics_chip_data) {
		TS_LOG_ERR("%s: f54->rmi4_data->synaptics_chip_data is NULL\n", __func__);
		goto param_nul;
	}
	command = (unsigned char)F54_TREX_SHORTS;
	TS_LOG_INFO("mmi_trex_shorts_test called\n");
	write_to_f54_register(command);
	f54->report_type = command;
	result = synaptics_rmi4_f54_attention();
	if (result < 0) {
		TS_LOG_ERR("%s: Failed to get data\n", __func__);
		goto err_get_result;
	}
	TS_LOG_INFO("%s-%d: trex_shorts_report_size = %d.\n", __func__, __LINE__, f54->report_size);

	if(NULL == f54->mmi_buf) {
		TS_LOG_ERR("%s: f54->mmi_buf is NULL\n", __func__);
		goto err_mmi_buf_null;
	}
	buf_backup = kzalloc(f54->report_size, GFP_KERNEL);
	if(!buf_backup) {
		TS_LOG_ERR("%s: buf_backup kzalloc failed\n", __func__);
		goto err_kzalloc_buf;
	}
	memcpy(buf_backup, f54->mmi_buf, f54->report_size);
	if(f54->report_size < TP_3320_SHORT_ARRAY_NUM) {
		TS_LOG_ERR("%s: report_size < trx_short_array_num, err\n", __func__);
		goto err_report_size;
	}
	for(i = 0; i < TP_3320_SHORT_ARRAY_NUM; i++) {
		TS_LOG_INFO("%s: buf_backup[%d] is %d\n", __func__, i, buf_backup[i]);
		if(buf_backup[i] != f54->rmi4_data->synaptics_chip_data->trx_short_circuit_array[i]) {
			TS_LOG_ERR("%s: test result is failed\n", __func__);
			goto err_match_defult;
		}
	}
	TS_LOG_INFO("%s: test result is succ\n", __func__);
	strncat(buf_f54test_result, SHORT_TEST_PASS, MAX_STR_LEN);
	kfree(buf_backup);
	return;

err_match_defult:
err_report_size:
	kfree(buf_backup);
err_kzalloc_buf:
err_mmi_buf_null:
err_get_result:
param_nul:
	strncat(buf_f54test_result, SHORT_TEST_FAIL, MAX_STR_LEN);
	return;
}

static void mmi_td43xx_ee_short_report(void)
{
	unsigned char command;
	int result = 0;
	command = (unsigned char)F54_TD43XX_EE_SHORT;

	TS_LOG_INFO("mmi_deltacapacitance_test called\n");
	write_to_f54_register(command);
	f54->report_type = command;
	result = synaptics_rmi4_f54_attention();
	if (result < 0) {
		TS_LOG_ERR("Failed to get data\n");
		strncat(buf_f54test_result, TD43XX_EE_SHORT_TEST_FAIL, MAX_STR_LEN);
		return;
	}
	result = td43xx_ee_short_report();
	if (TEST_PASS == result) {
		TS_LOG_INFO("tdxx_ee_short test is successed, result: 5P\n");
		strncat(buf_f54test_result, TD43XX_EE_SHORT_TEST_PASS, MAX_STR_LEN);
	} else {
		strncat(buf_f54test_result, TD43XX_EE_SHORT_TEST_FAIL, MAX_STR_LEN);
		strncpy(tp_test_failed_reason, "-panel_reason",
			TP_TEST_FAILED_REASON_LEN);
		TS_LOG_INFO("tdxx_ee_short test is failed, result: 5F\n");
	}

	return;
}
static int f54_delta_rx_report(void)
{
	short Rawimage_rx;
	short Rawimage_rx1;
	short Rawimage_rx2;
	short Result = 0;
	int i = 0;
	int j = 0;
	int step = 0;
	int delt_cap_uplimit =
	    f54->rmi4_data->synaptics_chip_data->raw_limit_buf[RX_TO_RX_UP];
	int delt_cap_lowlimit =
	    f54->rmi4_data->synaptics_chip_data->raw_limit_buf[RX_TO_RX_LOW];

	TS_LOG_INFO
	    ("rx = %d, tx = %d, delt_cap_uplimit = %d, delt_cap_lowlimit = %d\n",
	     f54->rmi4_data->num_of_rx, f54->rmi4_data->num_of_tx,
	     delt_cap_uplimit, delt_cap_lowlimit);

	for (i = 0; i < mmi_buf_size; i += 2) {
		Rawimage_rx1 = (f54->mmi_buf[i]) | (f54->mmi_buf[i + 1] << 8);
		Rawimage_rx2 =
		    (f54->mmi_buf[i + 2]) | (f54->mmi_buf[i + 3] << 8);
		Rawimage_rx = Rawimage_rx2 - Rawimage_rx1;
		if (synaptics_raw_data_limit_flag) {
			delt_cap_uplimit = RxDeltaCapUpperLimit[i / 2 - step];
			delt_cap_lowlimit = RxDeltaCapLowerLimit[i / 2 - step];
		}
		if ((Rawimage_rx <= delt_cap_uplimit)
		    && (Rawimage_rx >= delt_cap_lowlimit)) {
			Result++;
		} else {
			TS_LOG_INFO("[%d,%d]\n", i / 2 - step, Rawimage_rx);
		}
		j++;
		if (j == f54->rmi4_data->num_of_rx - 1) {
			i += 2;
			j = 0;
			step += 1;
		}
	}
	if (Result == (mmi_buf_size / 2 - f54->rmi4_data->num_of_tx)) {
		TS_LOG_DEBUG("rawdata rx diff is all right, Result = %d\n",
			     Result);
		return 1;
	} else {
		TS_LOG_ERR("rawdata rx diff is out of range, Result = %d\n",
			   Result);
		return 0;
	}

}

static int f54_delta_tx_report(void)
{
	short *Rawimage_tx = NULL;
	short Rawimage_delta_tx;
	int i, j, tx_n, rx_n;
	int Result = 0;
	int tx_to_tx_cap_uplimit =
	    f54->rmi4_data->synaptics_chip_data->raw_limit_buf[TX_TO_TX_UP];
	int tx_to_tx_cap_lowlimit =
	    f54->rmi4_data->synaptics_chip_data->raw_limit_buf[TX_TO_TX_LOW];

	TS_LOG_INFO
	    ("rx = %d, tx = %d, tx_to_tx_cap_uplimit = %d, tx_to_tx_cap_lowlimit = %d\n",
	     f54->rmi4_data->num_of_rx, f54->rmi4_data->num_of_tx,
	     tx_to_tx_cap_uplimit, tx_to_tx_cap_lowlimit);

	Rawimage_tx = (short *)kzalloc(mmi_buf_size, GFP_KERNEL);
	if (!Rawimage_tx) {
		TS_LOG_ERR("Rawimage_tx kzalloc error\n");
		return 0;
	}

	for (i = 0, j = 0; i < mmi_buf_size; i += 2, j++)
		Rawimage_tx[j] = (f54->mmi_buf[i]) | (f54->mmi_buf[i + 1] << 8);

	for (tx_n = 0; tx_n < f54->rmi4_data->num_of_tx - 1; tx_n++) {
		for (rx_n = 0; rx_n < f54->rmi4_data->num_of_rx; rx_n++) {
			if (synaptics_raw_data_limit_flag) {
				tx_to_tx_cap_uplimit =
				    TxDeltaCapUpperLimit[tx_n *
							 f54->rmi4_data->
							 num_of_rx + rx_n];
				tx_to_tx_cap_lowlimit =
				    TxDeltaCapLowerLimit[tx_n *
							 f54->rmi4_data->
							 num_of_rx + rx_n];
			}
			Rawimage_delta_tx =
			    Rawimage_tx[(tx_n + 1) * f54->rmi4_data->num_of_rx +
					rx_n] -
			    Rawimage_tx[tx_n * f54->rmi4_data->num_of_rx +
					rx_n];
			if ((Rawimage_delta_tx <= tx_to_tx_cap_uplimit)
			    && (Rawimage_delta_tx >= tx_to_tx_cap_lowlimit)) {
				Result++;
			} else {
				TS_LOG_INFO("[%d,%d]\n",
					    tx_n * f54->rmi4_data->num_of_rx +
					    rx_n, Rawimage_delta_tx);
			}
		}
	}
	kfree(Rawimage_tx);

	if (Result == (mmi_buf_size / 2 - f54->rmi4_data->num_of_rx)) {
		TS_LOG_DEBUG("rawdata tx diff is all right, Result = %d\n",
			     Result);
		return 1;
	} else {
		TS_LOG_ERR("rawdata tx diff is out of range, Result = %d\n",
			   Result);
		return 0;
	}
}
static int synaptics_get_threshold_from_csvfile(int columns, int rows, char* target_name, int32_t *data)
{
	char file_path[100] = {0};
	char file_name[64] = {0};
	int ret = 0;
	int result = 0;
	TS_LOG_INFO("%s called\n", __func__);

	if (!data || !target_name) {
		TS_LOG_ERR("parse csvfile failed, data or target_name is NULL\n");
		return TEST_FAIL;
	}

	if (!strnlen(f54->rmi4_data->synaptics_chip_data->ts_platform_data->product_name, MAX_STR_LEN-1)
		|| !strnlen(f54->rmi4_data->synaptics_chip_data->chip_name, MAX_STR_LEN-1)
		|| !strnlen(f54->rmi4_data->rmi4_mod_info.project_id_string, SYNAPTICS_RMI4_PROJECT_ID_SIZE)
		|| f54->rmi4_data->module_name == NULL) {
		TS_LOG_INFO("Threshold file name is not detected\n");
		return TEST_FAIL;
	}

	snprintf(file_name, sizeof(file_name), "%s_%s_%s_%s_raw.csv",
			f54->rmi4_data->synaptics_chip_data->ts_platform_data->product_name,
			f54->rmi4_data->synaptics_chip_data->chip_name,
			f54->rmi4_data->rmi4_mod_info.project_id_string,
			f54->rmi4_data->module_name);
	if (CSV_PRODUCT_SYSTEM == f54->rmi4_data->synaptics_chip_data->csvfile_use_product_system) {
		snprintf(file_path, sizeof(file_path), "/product/etc/firmware/ts/%s", file_name);
	}
	else if (CSV_ODM_SYSTEM == f54->rmi4_data->synaptics_chip_data->csvfile_use_product_system) {
		snprintf(file_path, sizeof(file_path), "/odm/etc/firmware/ts/%s", file_name);
	}
	else {
		TS_LOG_ERR("csvfile path is not supported, csvfile_use_product_system = %d\n",
				f54->rmi4_data->synaptics_chip_data->csvfile_use_product_system);
		return TEST_FAIL;
	}
	TS_LOG_INFO("threshold file name:%s, rows_size=%d, columns_size=%d\n", file_path, rows, columns);

	result =  ts_kit_parse_csvfile(file_path, target_name, data, rows, columns);
	if (0 == result){
		ret = TEST_PASS;
		TS_LOG_INFO(" Get threshold successed form csvfile\n");
	} else {
		TS_LOG_ERR("csv file parse fail:%s\n", file_path);
		ret = TEST_FAIL;
	}
	return ret;
}
static int f54_delta_rx2_report(void)
{
	short Rawimage_rx = 0;
	short Rawimage_rx1 = 0;
	short Rawimage_rx2 = 0;
	short Result = 0;
	int i = 0;
	int j = 0;
	int step = 0;
	int ret = 0;
	short rxdelt_cap_abslimit = 0;
	char buf[MAX_CAP_DATA_SIZE] = {0};
	struct ts_rawdata_limit_tab limit_tab = {0};
	int rows_size = f54->rmi4_data->num_of_tx;
	int columns_size = f54->rmi4_data->num_of_rx;
	int *abs_rxdelt_cap_limit = NULL;

	TS_LOG_INFO("%s called\n", __func__);

	limit_tab.unique_test = kzalloc(sizeof(struct ts_unique_capacitance_test), GFP_KERNEL);
	if (!limit_tab.unique_test) {
		TS_LOG_ERR("ts_unique_capacitance_test kzalloc error\n");
		ret = TEST_FAIL;
		goto error_release_mem;
	}
	limit_tab.unique_test->Read_only_support_unique = f54->rmi4_data->synaptics_chip_data->trx_delta_test_support;
	limit_tab.unique_test->Rx_delta_abslimit =
			(int32_t*)kzalloc(rows_size*columns_size*sizeof(int32_t), GFP_KERNEL);
	if (!limit_tab.unique_test->Rx_delta_abslimit) {
		TS_LOG_ERR("rx_delta_abslimit buffer kzalloc error\n");
		ret = TEST_FAIL;
		goto error_release_mem;
	}
	if (TEST_FAIL == synaptics_get_threshold_from_csvfile(columns_size-1, rows_size, CSV_TP_DELTA_ABS_RX_LIMIT, limit_tab.unique_test->Rx_delta_abslimit)) {
		TS_LOG_ERR("get abs_rxdelt_cap_limit err\n");
		ret = TEST_FAIL;
		memset(limit_tab.unique_test->Rx_delta_abslimit, 0, rows_size*columns_size*sizeof(int32_t));
	}
	abs_rxdelt_cap_limit = limit_tab.unique_test->Rx_delta_abslimit;

	snprintf(rx_delta_buf, MAX_CAP_DATA_SIZE, "%s\n", "RX:");
	for ( i = 0; i < mmi_buf_size; i+=2)
	{
		Rawimage_rx1 = (f54->mmi_buf[i]) | (f54->mmi_buf[i+1] << SHIFT_ONE_BYTE);
		Rawimage_rx2 = (f54->mmi_buf[i+2]) | (f54->mmi_buf[i+3] << SHIFT_ONE_BYTE);
		Rawimage_rx = abs(Rawimage_rx2 - Rawimage_rx1);
		snprintf(buf, sizeof(buf),"%3d,", Rawimage_rx);
		strncat(rx_delta_buf, buf ,sizeof(buf));
		rxdelt_cap_abslimit = abs_rxdelt_cap_limit[i/2 - step];
		if (Rawimage_rx <= rxdelt_cap_abslimit){
			Result++;
		}
		else{
			TS_LOG_ERR("[%d,%d] %d\n",i/2 - step,Rawimage_rx, rxdelt_cap_abslimit);
		}
		j++;
		if (j == columns_size - 1) {
			i += 2;
			j = 0;
			step += 1;
			strncat(rx_delta_buf, "\n", 1);
		}
	}

	if (Result == (mmi_buf_size/2 - rows_size)) {
		TS_LOG_INFO("rawdata rx diff is all right, Result = %d\n", Result);
		ret = TEST_PASS;
	}
	else {
		TS_LOG_ERR("rawdata rx diff is out of range, Result = %d\n", Result);
		ret = TEST_FAIL;
	}
error_release_mem:
	if (limit_tab.unique_test && limit_tab.unique_test->Rx_delta_abslimit){
		kfree(limit_tab.unique_test->Rx_delta_abslimit);
		limit_tab.unique_test->Rx_delta_abslimit = NULL;
	}
	if (limit_tab.unique_test) {
		kfree(limit_tab.unique_test);
		limit_tab.unique_test = NULL;
	}

	return ret;
}

static int f54_delta_tx2_report(void)
{
	int i = 0, j = 0;
	int tx_n = 0, rx_n = 0;
	int Result=0;
	int ret = 0;
	short txdelt_cap_abslimit = 0;
	short Rawimage_delta_tx = 0;
	char buf[MAX_CAP_DATA_SIZE] = {0};
	struct ts_rawdata_limit_tab limit_tab = {0};
	int rows_size = f54->rmi4_data->num_of_tx;
	int columns_size = f54->rmi4_data->num_of_rx;
	int *abs_txdelt_cap_limit =NULL;
	short *Rawimage_tx = NULL;

	TS_LOG_INFO("%s called\n", __func__);

	limit_tab.unique_test = kzalloc(sizeof(struct ts_unique_capacitance_test), GFP_KERNEL);
	if (!limit_tab.unique_test) {
		TS_LOG_ERR("ts_unique_capacitance_test kzalloc error\n");
		ret = TEST_FAIL;
		goto error_release_mem;
	}
	limit_tab.unique_test->Read_only_support_unique = f54->rmi4_data->synaptics_chip_data->trx_delta_test_support;
	limit_tab.unique_test->Tx_delta_abslimit =
			(int32_t*)kzalloc(rows_size*columns_size*sizeof(int32_t), GFP_KERNEL);
	if (!limit_tab.unique_test->Tx_delta_abslimit) {
		TS_LOG_ERR("Tx_delta_abslimit buffer kzalloc error\n");
		ret = TEST_FAIL;
		goto error_release_mem;
	}
	if (TEST_FAIL == synaptics_get_threshold_from_csvfile(columns_size, rows_size-1, CSV_TP_DELTA_ABS_TX_LIMIT, limit_tab.unique_test->Tx_delta_abslimit)) {
		TS_LOG_ERR("get abs_txdelt_cap_limit err\n");
		ret = TEST_FAIL;
		memset(limit_tab.unique_test->Tx_delta_abslimit, 0, rows_size*columns_size*sizeof(int32_t));
	}
	abs_txdelt_cap_limit = limit_tab.unique_test->Tx_delta_abslimit;

	Rawimage_tx = (short *)kzalloc(mmi_buf_size, GFP_KERNEL);
	if (!Rawimage_tx) {
		TS_LOG_ERR("Rawimage_tx kzalloc error\n");
		ret = TEST_FAIL;
		goto error_release_mem;
	}

	snprintf(tx_delta_buf, MAX_CAP_DATA_SIZE, "\n%s\n", "TX:");
	for ( i = 0, j = 0; i < mmi_buf_size; i+=2, j++)
		Rawimage_tx[j] = (f54->mmi_buf[i]) | (f54->mmi_buf[i+1] << SHIFT_ONE_BYTE);

	for( tx_n = 0; tx_n < rows_size - 1; tx_n++)
	{
		for(rx_n = 0; rx_n < columns_size; rx_n++)
		{
			txdelt_cap_abslimit = abs_txdelt_cap_limit[tx_n*columns_size+rx_n];

			Rawimage_delta_tx = abs(Rawimage_tx[(tx_n+1)*columns_size+rx_n]
				- Rawimage_tx[tx_n*columns_size+rx_n]);
			snprintf(buf, sizeof(buf),"%3d,", Rawimage_delta_tx);
			strncat(tx_delta_buf, buf ,sizeof(buf));
			if(Rawimage_delta_tx <= txdelt_cap_abslimit){
				Result++;
			}
			else{
				TS_LOG_ERR("[%d,%d]\n",tx_n*columns_size+rx_n,Rawimage_delta_tx);
			}
		}
		strncat(tx_delta_buf, "\n", 1);
	}

	if (Result == (mmi_buf_size/2 - columns_size)) {
		TS_LOG_INFO("rawdata tx diff is all right, Result = %d\n", Result);
		ret = TEST_PASS;
	}
	else {
		TS_LOG_ERR("rawdata tx diff is out of range, Result = %d\n", Result);
		ret = TEST_FAIL;
	}
error_release_mem:
	if (Rawimage_tx) {
		kfree(Rawimage_tx);
		Rawimage_tx = NULL;
	}
	if (limit_tab.unique_test && limit_tab.unique_test->Tx_delta_abslimit) {
		kfree(limit_tab.unique_test->Tx_delta_abslimit);
		limit_tab.unique_test->Tx_delta_abslimit = NULL;
	}
	if (limit_tab.unique_test) {
		kfree(limit_tab.unique_test);
		limit_tab.unique_test = NULL;
	}
	return ret;
}

static void mmi_rawcapacitance_test(void)
{
	unsigned char command;
	int result = 0;

	if (20 == f54->rmi4_data->synaptics_chip_data->rawdata_report_type)
		command = (unsigned char)F54_FULL_RAW_CAP_RX_COUPLING_COMP;
	else
		command = (unsigned char)F54_RAW_16BIT_IMAGE;

	TS_LOG_INFO("mmi_rawcapacitance_test called, command is %d\n", command);

	write_to_f54_register(command);
	f54->report_type = command;
	result = synaptics_rmi4_f54_attention();
	if (result < 0) {
		TS_LOG_ERR("Failed to get data\n");
		strncat(buf_f54test_result, "1F", MAX_STR_LEN);
		return;
	}
	result = f54_rawimage_report();
	if (1 == result) {
		strncat(buf_f54test_result, "1P", MAX_STR_LEN);
	} else {
		TS_LOG_ERR("raw data is out of range, , test result is 1F\n");
		strncat(buf_f54test_result, "1F", MAX_STR_LEN);
		strncpy(tp_test_failed_reason, "-panel_reason",
			TP_TEST_FAILED_REASON_LEN);
	}
	if (1 == (f54_delta_rx_report() && f54_delta_tx_report())) {
		strncat(buf_f54test_result, "-2P", MAX_STR_LEN);
	} else {
		TS_LOG_ERR
		    ("raw data diff is out of range, test result is 2F\n");
		strncat(buf_f54test_result, "-2F", MAX_STR_LEN);
		strncpy(tp_test_failed_reason, "-panel_reason",
			TP_TEST_FAILED_REASON_LEN);
	}
	return;
}

static void save_capacitance_data_to_rawdatabuf(void)
{
	int i , j;
	short temp;
	int index = 0;

	f54->rawdatabuf[0] = f54->rmi4_data->num_of_rx;
	f54->rawdatabuf[1] = f54->rmi4_data->num_of_tx;
	for(i = 0, j = index + 2; i < mmi_buf_size; i+=2, j++) {
		temp = (f54->mmi_buf[i]) | (f54->mmi_buf[i+1] << 8);
		f54->rawdatabuf[j] = temp;
	}
}

static int get_int_average(int *p, size_t len)
{
	int sum = 0;
	size_t i;

	for (i = 0; i < len; i++) {
		sum += p[i];
	}
	if (len != 0) {
		return (sum / len);
	} else {
		return 0;
	}
}

static int get_int_min(int *p, size_t len)
{
	int s_min = SHRT_MAX;
	size_t i;

	for (i = 0; i < len; i++) {
		s_min = s_min > p[i] ? p[i] : s_min;
	}

	return s_min;
}

static int get_int_max(int *p, size_t len)
{
	int s_max = SHRT_MIN;
	size_t i;

	for (i = 0; i < len; i++) {
		s_max = s_max < p[i] ? p[i] : s_max;
	}

	return s_max;
}

static void get_capacitance_stats(void)
{
	size_t num_elements = f54->rawdatabuf[0] * f54->rawdatabuf[1];

	f54->raw_statics_data.RawimageAverage =
		get_int_average(&f54->rawdatabuf[2], num_elements);
	f54->raw_statics_data.RawimageMaxNum =
		get_int_max(&f54->rawdatabuf[2], num_elements);
	f54->raw_statics_data.RawimageMinNum =
		get_int_min(&f54->rawdatabuf[2], num_elements);
}

static int check_enhance_raw_capacitance(void)
{
	int i;
	int count = f54->rmi4_data->num_of_rx * f54->rmi4_data->num_of_tx;

	save_capacitance_data_to_rawdatabuf();
	get_capacitance_stats();

	for (i = 0; i < count; i++) {
		TS_LOG_INFO("rawdata is upper: %d, lower: %d\n", f54->rmi4_data->synaptics_chip_data->upper[i], f54->rmi4_data->synaptics_chip_data->lower[i]);
		/* rawdatabuf[0] rawdatabuf[1] are num_of_rx and num_of_tx */
		if ((f54->rawdatabuf[i+2] > f54->rmi4_data->synaptics_chip_data->upper[i]) || (f54->rawdatabuf[i+2] < f54->rmi4_data->synaptics_chip_data->lower[i])) {
			TS_LOG_ERR("rawdata is out of range, failed at %d: upper: %d, lower: %d, raw: %d\n", i, f54->rmi4_data->synaptics_chip_data->upper[i], f54->rmi4_data->synaptics_chip_data->lower[i], f54->rawdatabuf[i+2]);
			return 0;
		}
	}

	return 1;
}

static int check_csvfile_raw_capacitance(void)
{
	int i = 0;
	int ret = TEST_PASS;
	int count = f54->rmi4_data->num_of_rx * f54->rmi4_data->num_of_tx;
	struct ts_rawdata_limit_tab limit_tab = {0};
	int rows_size = f54->rmi4_data->num_of_tx;
	int columns_size = f54->rmi4_data->num_of_rx;

	TS_LOG_INFO("%s called, rows:%d, columns:%d\n", __func__, rows_size, columns_size);
	save_capacitance_data_to_rawdatabuf();
	get_capacitance_stats();

	limit_tab.MutualRawMax =
		(int32_t*)kzalloc((rows_size)*(columns_size)*sizeof(int32_t), GFP_KERNEL);
	limit_tab.MutualRawMin =
		(int32_t*)kzalloc((rows_size)*(columns_size)*sizeof(int32_t), GFP_KERNEL);
	if (!limit_tab.MutualRawMax || !limit_tab.MutualRawMin ){
		TS_LOG_ERR("kzalloc rawdata buffer is NULL\n");
		ret = TEST_FAIL;
		goto error_release_mem;
	}
	if (TEST_FAIL == synaptics_get_threshold_from_csvfile(columns_size, rows_size, CSV_TP_CAP_RAW_MAX, limit_tab.MutualRawMax)) {
		TS_LOG_ERR("get rawdata_cap_max_limit err\n");
		ret = TEST_FAIL;
		goto error_release_mem;
	}
	if (TEST_FAIL == synaptics_get_threshold_from_csvfile(columns_size, rows_size, CSV_TP_CAP_RAW_MIN, limit_tab.MutualRawMin)) {
		TS_LOG_ERR("get rawdata_cap_min_limit err\n");
		ret = TEST_FAIL;
		goto error_release_mem;
	}

	for (i = 0; i < count; i++) {
		/* rawdatabuf[0] rawdatabuf[1] are num_of_rx and num_of_tx */
		if ((f54->rawdatabuf[i+2] > limit_tab.MutualRawMax[i]) || (f54->rawdatabuf[i+2] < limit_tab.MutualRawMin[i])) {
			TS_LOG_ERR("rawdata is out of range, failed at %d: upper: %d, lower: %d, raw: %d\n",
				i, limit_tab.MutualRawMax[i], limit_tab.MutualRawMin[i], f54->rawdatabuf[i+2]);
			ret = TEST_FAIL;
		}
	}

error_release_mem:
	if (limit_tab.MutualRawMax){
		kfree(limit_tab.MutualRawMax);
		limit_tab.MutualRawMax = NULL;
	}
	if (limit_tab.MutualRawMin){
		kfree(limit_tab.MutualRawMin);
		limit_tab.MutualRawMin = NULL;
	}

	return ret;
}

static void mmi_csvfile_rawdata_test(void)
{
	unsigned char command;
	int result = 0;

	command = (unsigned char)F54_RAW_16BIT_IMAGE;

	TS_LOG_INFO("%s called, command is %d\n", __func__, command);

	write_to_f54_register(command);
	f54->report_type = command;
	result = synaptics_rmi4_f54_attention();
	if (result < 0) {
		TS_LOG_ERR("Failed to get data\n");
		strncat(buf_f54test_result, RAWDATA_CAP_TEST_FAIL, MAX_STR_LEN);
		strncpy(tp_test_failed_reason, "-panel_reason",
			TP_TEST_FAILED_REASON_LEN);
		return;
	}
	result = check_csvfile_raw_capacitance();
	if (TEST_PASS == result) {
		strncat(buf_f54test_result, RAWDATA_CAP_TEST_PASS, MAX_STR_LEN);
		TS_LOG_INFO("raw data test successed, test result is 1P\n");
	} else {
		TS_LOG_ERR("raw data is out of range, test result is 1F\n");
		strncat(buf_f54test_result, RAWDATA_CAP_TEST_FAIL, MAX_STR_LEN);
		strncpy(tp_test_failed_reason, "-panel_reason",
			TP_TEST_FAILED_REASON_LEN);
	}
	if (f54->rmi4_data->synaptics_chip_data->trx_delta_test_support) {
		if (TEST_PASS == (f54_delta_rx2_report() & f54_delta_tx2_report())) {
			strncat(buf_f54test_result, TRX_DELTA_CAP_TEST_PASS, MAX_STR_LEN);
		} else {
			TS_LOG_ERR("trx_delta test is out of range, test result is 2F\n");
			strncat(buf_f54test_result, TRX_DELTA_CAP_TEST_FAIL, MAX_STR_LEN);
			strncpy(tp_test_failed_reason,"-panel_reason",TP_TEST_FAILED_REASON_LEN);
		}
	}
	return;
}

static void mmi_enhance_raw_capacitance_test(void)
{
	unsigned char command;
	int result = 0;

	if (20 == f54->rmi4_data->synaptics_chip_data->rawdata_report_type)
		command = (unsigned char)F54_FULL_RAW_CAP_RX_COUPLING_COMP;
	else
		command = (unsigned char)F54_RAW_16BIT_IMAGE;

	TS_LOG_INFO("mmi_rawcapacitance_test called, command is %d\n", command);

	write_to_f54_register(command);
	f54->report_type = command;
	result = synaptics_rmi4_f54_attention();
	if (result < 0) {
		TS_LOG_ERR("Failed to get data\n");
		strncat(buf_f54test_result, "1F", MAX_STR_LEN);
		return;
	}
	result = check_enhance_raw_capacitance();
	if (1 == result) {
		strncat(buf_f54test_result, "1P-2P", MAX_STR_LEN);
	} else {
		TS_LOG_ERR("raw data is out of range, , test result is 1F-2P\n");
		strncat(buf_f54test_result, "1F-2P", MAX_STR_LEN);
		strncpy(tp_test_failed_reason, "-panel_reason",
			TP_TEST_FAILED_REASON_LEN);
	}
}

static int synaptics_f54_malloc(void)
{
	f54 = kzalloc(sizeof(struct synaptics_rmi4_f54_handle), GFP_KERNEL);
	if (!f54) {
		TS_LOG_ERR("Failed to alloc mem for f54\n");
		return -ENOMEM;
	}

	f54->fn_ptr =
	    kzalloc(sizeof(struct synaptics_rmi4_exp_fn_ptr), GFP_KERNEL);
	if (!f54->fn_ptr) {
		TS_LOG_ERR("Failed to alloc mem for fn_ptr\n");
		return -ENOMEM;
	}

	f54->fn55 =
	    kzalloc(sizeof(struct synaptics_rmi4_fn55_desc), GFP_KERNEL);
	if (!f54->fn55) {
		TS_LOG_ERR("Failed to alloc mem for fn55\n");
		return -ENOMEM;
	}

	return NO_ERR;
}

static void synaptics_f54_free(void)
{
	TS_LOG_INFO("kfree f54 memory\n");
	if (f54 && f54->fn_ptr)
		kfree(f54->fn_ptr);
	if (f54 && f54->fn55)
		kfree(f54->fn55);
	if (f54 && f54->mmi_buf)
		kfree(f54->mmi_buf);
	if (f54 && f54->rawdatabuf)
		kfree(f54->rawdatabuf);
	if (f54 && f54->hybridbuf)
		kfree(f54->hybridbuf);
	if (f54) {
		kfree(f54);
		f54 = NULL;
	}
}

static void put_capacitance_data(int index)
{
	int i, j;
	short temp;
	f54->rawdatabuf[0] = f54->rmi4_data->num_of_rx;
	f54->rawdatabuf[1] = f54->rmi4_data->num_of_tx;
	for (i = 0, j = index + 2; i < mmi_buf_size; i += 2, j++) {
		temp = (f54->mmi_buf[i]) | (f54->mmi_buf[i + 1] << 8);
		f54->rawdatabuf[j] = temp;
	}
}

static void synaptics_change_report_rate(void)
{
	int rc = NO_ERR;
	unsigned char command = 0;
	unsigned char report_rate120 = 0;
	unsigned char report_rate60 = 0;

	if (0 == f54->rmi4_data->synaptics_chip_data->report_rate_test) {
		strncat(buf_f54test_result, "-4P", MAX_STR_LEN);
		TS_LOG_INFO("s3207 change_report_rate default pass\n");
		return;
	}
	TS_LOG_INFO("change report rate 120 first then to 60\n");
	command = (unsigned char)F54_120HI_RATE;
	rc = f54->fn_ptr->write(f54->rmi4_data,
				f54->control_base_addr + report_rate_offset,
				&command, 1);
	if (rc < 0) {
		TS_LOG_ERR("set ic to 120HZ error because of i2c error");
		strncat(buf_f54test_result, "-4F", MAX_STR_LEN);
		return;
	}
	msleep(200);
	rc = f54->fn_ptr->read(f54->rmi4_data,
			       f54->data_base_addr + F54_READ_RATE_OFFSET,
			       &report_rate120, 1);
	if (rc < 0) {
		TS_LOG_ERR("read 120HZ from ic error because of i2c error");
		strncat(buf_f54test_result, "-4F", MAX_STR_LEN);
		return;
	}
	TS_LOG_INFO("work report_rate 120 = %d\n", report_rate120);

	command = (unsigned char)F54_60LOW_RATE;
	rc = f54->fn_ptr->write(f54->rmi4_data,
				f54->control_base_addr + report_rate_offset,
				&command, 1);
	if (rc < 0) {
		TS_LOG_ERR("write ic to 60HZ error because of i2c error");
		strncat(buf_f54test_result, "-4F", MAX_STR_LEN);
		return;
	}
	msleep(200);
	rc = f54->fn_ptr->read(f54->rmi4_data,
			       f54->data_base_addr + F54_READ_RATE_OFFSET,
			       &report_rate60, 1);
	if (rc < 0) {
		TS_LOG_ERR("read 60HZ from ic error because of i2c error");
		strncat(buf_f54test_result, "-4F", MAX_STR_LEN);
		return;
	}
	TS_LOG_INFO("work report_rate 60 = %d\n", report_rate60);

	if ((F54_DATA120_RATE == report_rate120)
	    && (F54_DATA60_RATE == report_rate60)) {
		TS_LOG_DEBUG("change rate success\n");
		strncat(buf_f54test_result, "-4P", MAX_STR_LEN);
	} else {
		TS_LOG_ERR("change rate error");
		strncat(buf_f54test_result, "-4F", MAX_STR_LEN);
		strncpy(tp_test_failed_reason, "-panel_reason",
			TP_TEST_FAILED_REASON_LEN);
	}
	return;
}

int synap_get_cap_data(struct ts_rawdata_info *info)
{
	int rc = NO_ERR;
	unsigned char command;

	TS_LOG_INFO("synap_get_cap_data called\n");

	tx_delta_buf = info->tx_delta_buf;
	rx_delta_buf = info->rx_delta_buf;
	g_td43xx_rt95_part_one = info->td43xx_rt95_part_one;
	g_td43xx_rt95_part_two = info->td43xx_rt95_part_two;

	memset(buf_f54test_result, 0, sizeof(buf_f54test_result));
	memset(f54->rawdatabuf, 0, rawdata_size * sizeof(int));
	memset(f54->hybridbuf, 0 , HYBRID_BUF_SIZE * sizeof(u32));
	if (SYNAPTICS_TD4322 != f54->rmi4_data->synaptics_chip_data->ic_type
	&&SYNAPTICS_TD4310 != f54->rmi4_data->synaptics_chip_data->ic_type) {
		rc = f54->rmi4_data->status_save(f54->rmi4_data);
		if (rc < 0) {
			TS_LOG_ERR
				("failed to save glove/holster/palm or other status!\n");
		}
	}
	rc = f54->fn_ptr->read(f54->rmi4_data, f54->data_base_addr, &command,
			       1);
	if (rc < 0) {		/*i2c communication failed, mmi result is all failed */
		memcpy(buf_f54test_result, "0F-1F-2F",
		       (strlen("0F-1F-2F") + 1));
	} else {
		memcpy(buf_f54test_result, "0P-", (strlen("0P-") + 1));

		if (f54->rmi4_data->synaptics_chip_data->test_enhance_raw_data_capacitance) {
			mmi_enhance_raw_capacitance_test();	/*report type == 3 */
		} else if (f54->rmi4_data->synaptics_chip_data->test_capacitance_via_csvfile) {
			mmi_csvfile_rawdata_test();
		} else {
			mmi_rawcapacitance_test();	/*1-2p/f*/
		}

		TS_LOG_INFO ("Mutul rawdata test!!!\n");
		put_capacitance_data(0);
		mmi_deltacapacitance_test();	/*3p3f*/
		synaptics_change_report_rate(); /*4p4f*/
		if (f54->rmi4_data->synaptics_chip_data->td43xx_ee_short_test_support) {
			mmi_td43xx_ee_short_report();	/*report type == 95 */
		}
		put_capacitance_data(mmi_buf_size / 2);

		if (f54->rmi4_data->synaptics_chip_data->support_s3320_short_test) {
			mmi_trex_shorts_test();
		}

		if(f54->rmi4_data->synaptics_chip_data->self_cap_test)
		{
			TS_LOG_INFO ("Self rawdata test!!!%d \n", f54->rmi4_data->synaptics_chip_data->self_cap_test);
			TS_LOG_INFO ("%s: test_rawdata_normalizing = %x\n", __func__,
			     f54->rmi4_data->synaptics_chip_data->test_rawdata_normalizing);
			TS_LOG_INFO("seltest----start~!\n");
			mmi_hybrid_raw_cap_test();
			mmi_hybrid_abs_delta_test();
			TS_LOG_INFO("seltest----end~!\n");
		}
		mmi_add_static_data();
	}
	if ((SYNAPTICS_TD4322 != f54->rmi4_data->synaptics_chip_data->ic_type
		&& SYNAPTICS_TD4310 != f54->rmi4_data->synaptics_chip_data->ic_type)
		|| f54->rmi4_data->synaptics_chip_data->td43xx_ee_short_test_support) {
		rc = f54->rmi4_data->reset_device(f54->rmi4_data);
		if (rc < 0) {
			TS_LOG_ERR("failed to write command to f01 reset!\n");
			goto exit;
		}
		rc = f54->rmi4_data->status_resume(f54->rmi4_data);
		if (rc < 0) {
			TS_LOG_ERR
				("failed to resume glove/holster/palm or other status!\n");
		}
	}
	memcpy(info->buff, f54->rawdatabuf, rawdata_size * sizeof(int));

	info->hybrid_buff[0] = f54->rmi4_data->num_of_rx;
	info->hybrid_buff[1] = f54->rmi4_data->num_of_tx;
	memcpy(&info->hybrid_buff[2], f54->hybridbuf, (mmi_hybrid_abs_delta/2) * sizeof(int));

	strncat(buf_f54test_result, ";", strlen(";"));
	if (0 == strlen(buf_f54test_result) || strstr(buf_f54test_result, "F")) {
		strncat(buf_f54test_result, tp_test_failed_reason,
			strlen(tp_test_failed_reason));
	}

	switch (f54->rmi4_data->synaptics_chip_data->ic_type) {
	case SYNAPTICS_S3207:
		strncat(buf_f54test_result, "-synaptics_3207",
			strlen("-synaptics_3207"));
		break;
	case SYNAPTICS_S3350:
		strncat(buf_f54test_result, "-synaptics_3350",
			strlen("-synaptics_3350"));
		break;
	case SYNAPTICS_S3320:
		strncat(buf_f54test_result, "-synaptics_3320",
			strlen("-synaptics_3320"));
		break;
	case SYNAPTICS_S3718:
	case SYNAPTICS_TD4322:
	case SYNAPTICS_TD4310:
		strncat(buf_f54test_result, "-synaptics_",
			strlen("-synaptics_"));
		strncat(buf_f54test_result,
			f54->rmi4_data->rmi4_mod_info.project_id_string,
			strlen(f54->rmi4_data->rmi4_mod_info.
			       project_id_string));
		break;
	default:
		TS_LOG_ERR("failed to recognize ic_ver\n");
		break;
	}

	memcpy(info->result, buf_f54test_result, strlen(buf_f54test_result));
	info->used_size = rawdata_size;
	info->used_synaptics_self_cap_size = mmi_hybrid_abs_delta >> 1;
	TS_LOG_INFO("info->used_size = %d\n", info->used_size);
	rc = NO_ERR;
exit:
	synaptics_f54_free();
	return rc;
}

static int synaptics_rmi4_f54_attention_cust(void)
{
	int retval;
	int l;
	unsigned char report_index[2];
	int i = 0;
	unsigned int report_times_max = 0;
	unsigned int report_size_temp = MAX_I2C_MSG_LENS;
	unsigned char *report_data_temp = NULL;

	set_report_size();

	if (f54->report_size == 0) {
		TS_LOG_ERR("Report data size = 0\n");
		retval = -EINVAL;
		goto error_exit;
	}

	if (f54->data_buffer_size < f54->report_size){
		if ((f54->data_buffer_size) && (f54->report_data)) {
			kfree(f54->report_data);
			f54->report_data = NULL;
		}
		f54->report_data = kzalloc(f54->report_size, GFP_KERNEL);
		if (!f54->report_data) {
			TS_LOG_ERR("Failed to alloc mem for data buffer\n");
			f54->data_buffer_size = 0;
			retval = -ENOMEM;
			goto error_exit;
		}
		f54->data_buffer_size = f54->report_size;
	}

	report_times_max = f54->report_size/MAX_I2C_MSG_LENS;
	if(f54->report_size%MAX_I2C_MSG_LENS != 0)
	{
		report_times_max += 1;
	}

	report_index[0] = 0;
	report_index[1] = 0;

	retval = f54->fn_ptr->write(f54->rmi4_data,
			f54->data_base_addr + DATA_REPORT_INDEX_OFFSET,
			report_index,
			sizeof(report_index));
	if (retval < 0) {
		TS_LOG_ERR("Failed to write report data index\n");
		retval = -EINVAL;
		goto error_exit;
	}

	/* Point to the block data about to transfer */
	report_data_temp = f54->report_data;
	TS_LOG_INFO("report_size = %d.\n",f54->report_size);
	TS_LOG_INFO("report_times_max = %d.\n",report_times_max);

	for(i = 0;i < report_times_max;i ++){
		if(i == (report_times_max - 1))
		{
			/* The last time transfer the rest of the block data */
			report_size_temp = f54->report_size%MAX_I2C_MSG_LENS;
			/* Bug:if f54->report_size % MAX_I2C_MSG_LENS == 0,
			the last time transfer data len is MAX_I2C_MSG_LENS.
			*/
			report_size_temp = (report_size_temp != 0) ? report_size_temp : MAX_I2C_MSG_LENS;
		}
		TS_LOG_DEBUG("i = %d,report_size_temp = %d.\n",i,report_size_temp);
		retval = f54->fn_ptr->read(f54->rmi4_data,
			f54->data_base_addr + DATA_REPORT_DATA_OFFSET,
			report_data_temp,
			report_size_temp);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to read report data\n",__func__);
			retval = -EINVAL;
			//mutex_unlock(&f54->data_mutex);
			goto error_exit;
		}
		/* Point to the next 256bytes data */
		report_data_temp += MAX_I2C_MSG_LENS;
	}

	retval = NO_ERR;

error_exit:
	return retval;
}

int synap_get_calib_data(struct ts_calibration_data_info *info)
{
	int rc = NO_ERR;
	unsigned char command;

	TS_LOG_INFO("%s called\n", __FUNCTION__);

	command = (unsigned char) F54_CALIBRATION;

	write_to_f54_register(command);
	f54->report_type = command;
	rc = synaptics_rmi4_f54_attention_cust();
	if(rc < 0){
		TS_LOG_ERR("Failed to get data\n");
		goto exit;
	}

	memcpy(info->data, f54->report_data, f54->report_size);

	info->used_size = f54->report_size;
	TS_LOG_INFO("info->used_size = %d\n", info->used_size);
	info->tx_num = f54->rmi4_data->num_of_tx;
	info->rx_num = f54->rmi4_data->num_of_rx;
	TS_LOG_INFO("info->tx_num = %d\n", info->tx_num);
	TS_LOG_INFO("info->rx_num = %d\n", info->rx_num);
	rc = NO_ERR;
exit:
	synaptics_f54_free();
	return rc;
}

int synap_get_calib_info(struct ts_calibration_info_param *info)
{
	int rc = NO_ERR;
	char calibration_state = 0;

	TS_LOG_INFO("%s called\n", __FUNCTION__);
	TS_LOG_INFO("calibration info reg offset: 0x%x\n", CALIBRATION_INFO_OFFSET);

	rc = f54->fn_ptr->read(f54->rmi4_data,
		f54->data_base_addr + CALIBRATION_INFO_OFFSET,
		&calibration_state,
		sizeof (calibration_state));
	if(rc < 0){
		TS_LOG_ERR("Failed to get calibration info\n");
		goto exit;
	}
	TS_LOG_INFO("calibration_state = 0x%x\n", calibration_state);

	info->calibration_crc = ((calibration_state >> 1) & 1);
	TS_LOG_INFO("info->calibration_crc = %d\n", info->calibration_crc);
	rc = NO_ERR;
exit:
	synaptics_f54_free();
	return rc;
}

int synap_debug_data_test(struct ts_diff_data_info *info)
{
	int rc = NO_ERR;

	TS_LOG_INFO("synaptics_get_debug_cap_data called\n");

	memset(buf_f54test_result, 0, sizeof(buf_f54test_result));
	memset(f54->rawdatabuf, 0, rawdata_size * sizeof(int));
	memset(f54->mmi_buf, 0, mmi_buf_size);

	switch (info->debug_type) {
	case READ_DIFF_DATA:
		mmi_deltacapacitance_test();	/*report type == 2 */
		put_capacitance_data(0);
		break;
	case READ_RAW_DATA:
		mmi_rawcapacitance_test();	/*report type == 3 */
		put_capacitance_data(0);
		break;
	default:
		TS_LOG_ERR("failed to recognize ic_ver\n");
		break;
	}

	memcpy(info->buff, f54->rawdatabuf, rawdata_size * sizeof(int));
	memcpy(info->result, buf_f54test_result, strlen(buf_f54test_result));
	info->used_size = rawdata_size / 2;

	synaptics_f54_free();
	return rc;
}

static int synaptics_rmi4_f54_attention(void)
{
	int retval;
	int l;
	unsigned char report_index[2];
	int i = 0;
	unsigned int report_times_max = 0;
	unsigned int report_size_temp = MAX_I2C_MSG_LENS;
	unsigned char *report_data_temp = NULL;

	set_report_size();

	if (f54->report_size == 0) {
		TS_LOG_ERR("Report data size = 0\n");
		retval = -EINVAL;
		goto error_exit;
	}

	if (f54->data_buffer_size < f54->report_size) {
		if ((f54->data_buffer_size) && (f54->report_data)) {
			kfree(f54->report_data);
			f54->report_data = NULL;
		}
		f54->report_data = kzalloc(f54->report_size, GFP_KERNEL);
		if (!f54->report_data) {
			TS_LOG_ERR("Failed to alloc mem for data buffer\n");
			f54->data_buffer_size = 0;
			retval = -ENOMEM;
			goto error_exit;
		}
		f54->data_buffer_size = f54->report_size;
	}

	report_times_max = f54->report_size / MAX_I2C_MSG_LENS;
	if (f54->report_size % MAX_I2C_MSG_LENS != 0) {
		report_times_max += 1;
	}

	report_index[0] = 0;
	report_index[1] = 0;

	retval = f54->fn_ptr->write(f54->rmi4_data,
				    f54->data_base_addr +
				    DATA_REPORT_INDEX_OFFSET, report_index,
				    sizeof(report_index));
	if (retval < 0) {
		TS_LOG_ERR("Failed to write report data index\n");
		retval = -EINVAL;
		goto error_exit;
	}

	/* Point to the block data about to transfer */
	report_data_temp = f54->report_data;
	TS_LOG_INFO("report_size = %d.\n", f54->report_size);
	TS_LOG_INFO("report_times_max = %d.\n", report_times_max);

	for (i = 0; i < report_times_max; i++) {
		if (i == (report_times_max - 1)) {
			/* The last time transfer the rest of the block data */
			report_size_temp = f54->report_size % MAX_I2C_MSG_LENS;
			/* Bug:if f54->report_size % MAX_I2C_MSG_LENS == 0,
			   the last time transfer data len is MAX_I2C_MSG_LENS.
			 */
			report_size_temp =
			    (report_size_temp !=
			     0) ? report_size_temp : MAX_I2C_MSG_LENS;
		}
		TS_LOG_DEBUG("i = %d,report_size_temp = %d.\n", i,
			     report_size_temp);
		retval =
		    f54->fn_ptr->read(f54->rmi4_data,
				      f54->data_base_addr +
				      DATA_REPORT_DATA_OFFSET, report_data_temp,
				      report_size_temp);
		if (retval < 0) {
			TS_LOG_ERR("%s: Failed to read report data\n",
				   __func__);
			retval = -EINVAL;
			/*mutex_unlock(&f54->data_mutex);*/
			goto error_exit;
		}
		/* Point to the next 256bytes data */
		report_data_temp += MAX_I2C_MSG_LENS;
	}

	if (f54->report_size > mmi_buf_size)
		return NO_ERR;
	/*get report data, one data contains two bytes */
	for (l = 0; l < f54->report_size; l += 2) {
		f54->mmi_buf[l] = f54->report_data[l];
		f54->mmi_buf[l + 1] = f54->report_data[l + 1];
	}

	retval = NO_ERR;

error_exit:
	return retval;
}

static int synaptics_read_f34(void)
{
	int retval = NO_ERR;

	if (SYNAPTICS_S3718 != f54->rmi4_data->synaptics_chip_data->ic_type) {
		retval = f54->fn_ptr->read(f54->rmi4_data,
					   f54->f34_fd.query_base_addr +
					   BOOTLOADER_ID_OFFSET,
					   f54->bootloader_id,
					   sizeof(f54->bootloader_id));
		if (retval < 0) {
			TS_LOG_ERR("Failed to read bootloader ID\n");
			return retval;
		}
	} else {
		retval = f54->fn_ptr->read(f54->rmi4_data,
					   f54->f34_fd.query_base_addr +
					   BOOTLOADER_ID_OFFSET_V7,
					   f54->bootloader_id,
					   sizeof(f54->bootloader_id));
		if (retval < 0) {
			TS_LOG_ERR("Failed to read bootloader ID\n");
			return retval;
		}
	}
	/*V5 V6 version is char data, as '5' '6', V7 is int data, 7 */
	TS_LOG_INFO("bootloader_id[1] = %c, %d\n", f54->bootloader_id[1],
		    f54->bootloader_id[1]);

	switch (f54->bootloader_id[1]) {
	case '5':
		f54->bl_version = V5;
		break;
	case '6':
		f54->bl_version = V6;
		break;
	case 7:
		f54->bl_version = V7;
		break;
	default:
		TS_LOG_ERR("unknown %d %c\n", f54->bootloader_id[1],
			   f54->bootloader_id[1]);
		break;
	}

	if (SYNAPTICS_S3207 != f54->rmi4_data->synaptics_chip_data->ic_type) {
		if (V5 == f54->bl_version) {
			/*get tx and rx value by read register from F11_2D_CTRL77 and F11_2D_CTRL78 */
			retval =
			    f54->fn_ptr->read(f54->rmi4_data,
					      f54->rmi4_data->rmi4_feature.
					      f01_ctrl_base_addr + RX_NUMBER,
					      &f54->rmi4_data->num_of_rx, 1);
			if (retval < 0) {
				TS_LOG_ERR
				    ("Could not read RX value from 0x%04x\n",
				     f54->rmi4_data->rmi4_feature.
				     f01_ctrl_base_addr + RX_NUMBER);
				return -EINVAL;
			}

			retval =
			    f54->fn_ptr->read(f54->rmi4_data,
					      f54->rmi4_data->rmi4_feature.
					      f01_ctrl_base_addr + TX_NUMBER,
					      &f54->rmi4_data->num_of_tx, 1);
			if (retval < 0) {
				TS_LOG_ERR
				    ("Could not read TX value from 0x%04x\n",
				     f54->rmi4_data->rmi4_feature.
				     f01_ctrl_base_addr + TX_NUMBER);
				return -EINVAL;
			}
		}
	} else {
		retval =
		    f54->fn_ptr->read(f54->rmi4_data,
				      f54->rmi4_data->rmi4_feature.
				      f11_ctrl_base_addr + RX_NUMBER_S3207,
				      &f54->rmi4_data->num_of_rx, 1);
		if (retval < 0) {
			TS_LOG_ERR("Could not read RX value from 0x%04x\n",
				   f54->rmi4_data->rmi4_feature.
				   f11_ctrl_base_addr + RX_NUMBER_S3207);
			return -EINVAL;
		}

		retval =
		    f54->fn_ptr->read(f54->rmi4_data,
				      f54->rmi4_data->rmi4_feature.
				      f11_ctrl_base_addr + TX_NUMBER_S3207,
				      &f54->rmi4_data->num_of_tx, 1);
		if (retval < 0) {
			TS_LOG_ERR("Could not read TX value from 0x%04x\n",
				   f54->rmi4_data->rmi4_feature.
				   f11_ctrl_base_addr + TX_NUMBER_S3207);
			return -EINVAL;
		}
	}

	/*used for force touch data. */
	if (1 == f54->rmi4_data->synaptics_chip_data->support_3d_func) {
		TS_LOG_INFO("support 3d test\n");
		f54->rmi4_data->num_of_tx =
		    (f54->rmi4_data->num_of_tx) - FORCE_NUMER;
	}
	rawdata_size =
	    (f54->rmi4_data->num_of_tx) * (f54->rmi4_data->num_of_rx) * 2 + 2;
	mmi_buf_size =
	    (f54->rmi4_data->num_of_tx) * (f54->rmi4_data->num_of_rx) * 2;

	TS_LOG_INFO("rx = %d, tx = %d, rawdata_size = %d, mmi_buf_size = %d\n",
		    f54->rmi4_data->num_of_rx, f54->rmi4_data->num_of_tx,
		    rawdata_size, mmi_buf_size);
	return NO_ERR;
}

static void Synaptics_test_free_control_mem(void)
{
	struct f54_control control = f54->control;

	kfree(control.reg_7);
	kfree(control.reg_41);
	kfree(control.reg_57);
	kfree(control.reg_88);

	return;
}

static int Synaptics_test_set_controls(void)
{
	unsigned char length;
	unsigned char num_of_sensing_freqs;
	unsigned short reg_addr = f54->control_base_addr;
	struct f54_control *control = &f54->control;

	num_of_sensing_freqs = f54->query.number_of_sensing_frequencies;

	/* control 0 */
	reg_addr += CONTROL_0_SIZE;

	/* control 1 */
	if ((f54->query.touch_controller_family == 0) ||
	    (f54->query.touch_controller_family == 1))
		reg_addr += CONTROL_1_SIZE;

	/* control 2 */
	reg_addr += CONTROL_2_SIZE;

	/* control 3 */
	if (f54->query.has_pixel_touch_threshold_adjustment == 1)
		reg_addr += CONTROL_3_SIZE;

	/* controls 4 5 6 */
	if ((f54->query.touch_controller_family == 0) ||
	    (f54->query.touch_controller_family == 1))
		reg_addr += CONTROL_4_6_SIZE;

	/* control 7 */
	if (f54->query.touch_controller_family == 1) {
		control->reg_7 = kzalloc(sizeof(*(control->reg_7)), GFP_KERNEL);
		if (!control->reg_7)
			goto exit_no_mem;
		control->reg_7->address = reg_addr;
		reg_addr += CONTROL_7_SIZE;
	}

	/* controls 8 9 */
	if ((f54->query.touch_controller_family == 0) ||
	    (f54->query.touch_controller_family == 1))
		reg_addr += CONTROL_8_9_SIZE;

	/* control 10 */
	if (f54->query.has_interference_metric == 1)
		reg_addr += CONTROL_10_SIZE;

	/* control 11 */
	if (f54->query.has_ctrl11 == 1)
		reg_addr += CONTROL_11_SIZE;

	/* controls 12 13 */
	if (f54->query.has_relaxation_control == 1)
		reg_addr += CONTROL_12_13_SIZE;

	/* controls 14 15 16 */
	if (f54->query.has_sensor_assignment == 1) {
		reg_addr += CONTROL_14_SIZE;
		reg_addr += CONTROL_15_SIZE * f54->query.num_of_rx_electrodes;
		reg_addr += CONTROL_16_SIZE * f54->query.num_of_tx_electrodes;
	}

	/* controls 17 18 19 */
	if (f54->query.has_sense_frequency_control == 1) {
		reg_addr += CONTROL_17_SIZE * num_of_sensing_freqs;
		reg_addr += CONTROL_18_SIZE * num_of_sensing_freqs;
		reg_addr += CONTROL_19_SIZE * num_of_sensing_freqs;
	}

	/* control 20 */
	reg_addr += CONTROL_20_SIZE;

	/* control 21 */
	if (f54->query.has_sense_frequency_control == 1)
		reg_addr += CONTROL_21_SIZE;

	/* controls 22 23 24 25 26 */
	if (f54->query.has_firmware_noise_mitigation == 1)
		reg_addr += CONTROL_22_26_SIZE;

	/* control 27 */
	if (f54->query.has_iir_filter == 1)
		reg_addr += CONTROL_27_SIZE;

	/* control 28 */
	if (f54->query.has_firmware_noise_mitigation == 1)
		reg_addr += CONTROL_28_SIZE;

	/* control 29 */
	if (f54->query.has_cmn_removal == 1)
		reg_addr += CONTROL_29_SIZE;

	/* control 30 */
	if (f54->query.has_cmn_maximum == 1)
		reg_addr += CONTROL_30_SIZE;

	/* control 31 */
	if (f54->query.has_touch_hysteresis == 1)
		reg_addr += CONTROL_31_SIZE;

	/* controls 32 33 34 35 */
	if (f54->query.has_edge_compensation == 1)
		reg_addr += CONTROL_32_35_SIZE;

	/* control 36 */
	if ((f54->query.curve_compensation_mode == 1) ||
	    (f54->query.curve_compensation_mode == 2)) {
		if (f54->query.curve_compensation_mode == 1) {
			length = max(f54->query.num_of_rx_electrodes,
				     f54->query.num_of_tx_electrodes);
		} else if (f54->query.curve_compensation_mode == 2) {
			length = f54->query.num_of_rx_electrodes;
		}
		reg_addr += CONTROL_36_SIZE * length;
	}

	/* control 37 */
	if (f54->query.curve_compensation_mode == 2)
		reg_addr += CONTROL_37_SIZE * f54->query.num_of_tx_electrodes;

	/* controls 38 39 40 */
	if (f54->query.has_per_frequency_noise_control == 1) {
		reg_addr += CONTROL_38_SIZE * num_of_sensing_freqs;
		reg_addr += CONTROL_39_SIZE * num_of_sensing_freqs;
		reg_addr += CONTROL_40_SIZE * num_of_sensing_freqs;
	}

	/* control 41 */
	if (f54->query.has_signal_clarity == 1) {
		control->reg_41 = kzalloc(sizeof(*(control->reg_41)),
					  GFP_KERNEL);
		if (!control->reg_41)
			goto exit_no_mem;
		control->reg_41->address = reg_addr;
		reg_addr += CONTROL_41_SIZE;
	}

	/* control 42 */
	if (f54->query.has_variance_metric == 1)
		reg_addr += CONTROL_42_SIZE;

	/* controls 43 44 45 46 47 48 49 50 51 52 53 54 */
	if (f54->query.has_multi_metric_state_machine == 1)
		reg_addr += CONTROL_43_54_SIZE;

	/* controls 55 56 */
	if (f54->query.has_0d_relaxation_control == 1)
		reg_addr += CONTROL_55_56_SIZE;

	/* control 57 */
	if (f54->query.has_0d_acquisition_control == 1) {
		control->reg_57 = kzalloc(sizeof(*(control->reg_57)),
					  GFP_KERNEL);
		if (!control->reg_57)
			goto exit_no_mem;
		control->reg_57->address = reg_addr;
		reg_addr += CONTROL_57_SIZE;
	}

	/* control 58 */
	if (f54->query.has_0d_acquisition_control == 1)
		reg_addr += CONTROL_58_SIZE;

	/* control 59 */
	if (f54->query.has_h_blank == 1)
		reg_addr += CONTROL_59_SIZE;

	/* controls 60 61 62 */
	if ((f54->query.has_h_blank == 1) ||
	    (f54->query.has_v_blank == 1) || (f54->query.has_long_h_blank == 1))
		reg_addr += CONTROL_60_62_SIZE;

	/* control 63 */
	if ((f54->query.has_h_blank == 1) ||
	    (f54->query.has_v_blank == 1) ||
	    (f54->query.has_long_h_blank == 1) ||
	    (f54->query.has_slew_metric == 1) ||
	    (f54->query.has_slew_option == 1) ||
	    (f54->query.has_noise_mitigation2 == 1))
		reg_addr += CONTROL_63_SIZE;

	/* controls 64 65 66 67 */
	if (f54->query.has_h_blank == 1)
		reg_addr += CONTROL_64_67_SIZE * 7;
	else if ((f54->query.has_v_blank == 1) ||
		 (f54->query.has_long_h_blank == 1))
		reg_addr += CONTROL_64_67_SIZE;

	/* controls 68 69 70 71 72 73 */
	if ((f54->query.has_h_blank == 1) ||
	    (f54->query.has_v_blank == 1) || (f54->query.has_long_h_blank == 1))
		reg_addr += CONTROL_68_73_SIZE;

	/* control 74 */
	if (f54->query.has_slew_metric == 1)
		reg_addr += CONTROL_74_SIZE;

	/* control 75 */
	if (f54->query.has_enhanced_stretch == 1)
		reg_addr += CONTROL_75_SIZE * num_of_sensing_freqs;

	/* control 76 */
	if (f54->query.has_startup_fast_relaxation == 1)
		reg_addr += CONTROL_76_SIZE;

	/* controls 77 78 */
	if (f54->query.has_esd_control == 1)
		reg_addr += CONTROL_77_78_SIZE;

	/* controls 79 80 81 82 83 */
	if (f54->query.has_noise_mitigation2 == 1)
		reg_addr += CONTROL_79_83_SIZE;

	/* controls 84 85 */
	if (f54->query.has_energy_ratio_relaxation == 1)
		reg_addr += CONTROL_84_85_SIZE;

	/* control 86 */
	if ((f54->query.has_query13 == 1) && (f54->query_13.has_ctrl86 == 1)) {
		report_rate_offset = reg_addr - f54->control_base_addr;
		TS_LOG_INFO("%s, no 2 offset = %d, report_rate_offset = %d\n",
			    __func__, reg_addr, report_rate_offset);
		reg_addr += CONTROL_86_SIZE;
	}

	/* control 87 */
	if ((f54->query.has_query13 == 1) && (f54->query_13.has_ctrl87 == 1))
		reg_addr += CONTROL_87_SIZE;

	/* control 88 */
	if (f54->query.has_ctrl88 == 1) {
		control->reg_88 = kzalloc(sizeof(*(control->reg_88)),
					  GFP_KERNEL);
		if (!control->reg_88)
			goto exit_no_mem;
		control->reg_88->address = reg_addr;
		reg_addr += CONTROL_88_SIZE;
	}

	return 0;

exit_no_mem:
	TS_LOG_ERR("Failed to alloc mem for control registers\n");
	return -ENOMEM;
}

static int Synaptics_test_set_queries(void)
{
	int retval;
	unsigned char offset;
	struct synaptics_rmi4_data *rmi4_data = f54->rmi4_data;

	retval = f54->fn_ptr->read(rmi4_data,
				   f54->query_base_addr,
				   f54->query.data, sizeof(f54->query.data));
	if (retval < 0)
		return retval;
	offset = sizeof(f54->query.data);

	/* query 12 */
	if (f54->query.has_sense_frequency_control == 0)
		offset -= 1;

	/* query 13 */
	if (f54->query.has_query13) {
		retval = f54->fn_ptr->read(rmi4_data,
					   f54->query_base_addr + offset,
					   f54->query_13.data,
					   sizeof(f54->query_13.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 14 */
	if ((f54->query.has_query13) && (f54->query_13.has_ctrl87))
		offset += 1;

	/* query 15 */
	if (f54->query.has_query15) {
		retval = f54->fn_ptr->read(rmi4_data,
					   f54->query_base_addr + offset,
					   f54->query_15.data,
					   sizeof(f54->query_15.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 16 */
	retval = f54->fn_ptr->read(rmi4_data,
				   f54->query_base_addr + offset,
				   f54->query_16.data,
				   sizeof(f54->query_16.data));
	if (retval < 0)
		return retval;
	offset += 1;

	/* query 17 */
	if (f54->query_16.has_query17)
		offset += 1;

	/* query 18 */
	if (f54->query_16.has_ctrl94_query18)
		offset += 1;

	/* query 19 */
	if (f54->query_16.has_ctrl95_query19)
		offset += 1;

	/* query 20 */
	if ((f54->query.has_query15) && (f54->query_15.has_query20))
		offset += 1;

	/* query 21 */
	retval = f54->fn_ptr->read(rmi4_data,
				   f54->query_base_addr + offset,
				   f54->query_21.data,
				   sizeof(f54->query_21.data));
	if (retval < 0)
		return retval;

	return 0;
}

static int match_module_name(const char *module_name)
{
	TS_LOG_INFO("%s: module_name = %s,product_name=%s\n", __func__,
		    module_name, rmi4_data->synaptics_chip_data->ts_platform_data->product_name);
	if (strcmp(rmi4_data->synaptics_chip_data->ts_platform_data->product_name, "chm") == 0) {
		if (strcmp(module_name, "oflim") == 0) {
			RawCapUpperLimit = RawCapUpperLimit_oflim_chm;
			RawCapLowerLimit = RawCapLowerLimit_oflim_chm;
			RxDeltaCapUpperLimit = RxDeltaCapUpperLimit_oflim_chm;
			RxDeltaCapLowerLimit = RxDeltaCapLowerLimit_oflim_chm;
			TxDeltaCapUpperLimit = TxDeltaCapUpperLimit_oflim_chm;
			TxDeltaCapLowerLimit = TxDeltaCapLowerLimit_oflim_chm;
			return NO_ERR;
		} else if (strcmp(module_name, "lensone") == 0) {
			RawCapUpperLimit = RawCapUpperLimit_lensone_chm;
			RawCapLowerLimit = RawCapLowerLimit_lensone_chm;
			RxDeltaCapUpperLimit = RxDeltaCapUpperLimit_lensone_chm;
			RxDeltaCapLowerLimit = RxDeltaCapLowerLimit_lensone_chm;
			TxDeltaCapUpperLimit = TxDeltaCapUpperLimit_lensone_chm;
			TxDeltaCapLowerLimit = TxDeltaCapLowerLimit_lensone_chm;
			return NO_ERR;
		} else if (strcmp(module_name, "truly") == 0) {
			RawCapUpperLimit = RawCapUpperLimit_truly_chm;
			RawCapLowerLimit = RawCapLowerLimit_truly_chm;
			RxDeltaCapUpperLimit = RxDeltaCapUpperLimit_truly_chm;
			RxDeltaCapLowerLimit = RxDeltaCapLowerLimit_truly_chm;
			TxDeltaCapUpperLimit = TxDeltaCapUpperLimit_truly_chm;
			TxDeltaCapLowerLimit = TxDeltaCapLowerLimit_truly_chm;
			return NO_ERR;
		} else {
			TS_LOG_ERR("%s: Failed to match module_name \n",
				   __func__);
			return -EINVAL;
		}
	}
}

int synap_rmi4_f54_init(struct synaptics_rmi4_data *rmi4_data,
			    const char *module_name)
{
	int retval = -EINVAL;
	bool hasF54 = false;
	bool hasF55 = false;
	bool hasF34 = false;
	unsigned short ii;
	unsigned char page;
	unsigned char intr_count = 0;
	struct synaptics_rmi4_fn_desc rmi_fd;

	if (synaptics_raw_data_limit_flag) {
		retval = match_module_name(module_name);
		if (retval < 0) {
			retval = -ENOMEM;
			return retval;
		}
	}
	if (synaptics_f54_malloc() != NO_ERR)
		goto exit_free_mem;

	f54->rmi4_data = rmi4_data;
	f54->fn_ptr->read = rmi4_data->i2c_read;
	f54->fn_ptr->write = rmi4_data->i2c_write;

	for (page = 0; page < PAGES_TO_SERVICE; page++) {
		for (ii = PDT_START; ii > PDT_END; ii -= PDT_ENTRY_SIZE) {
			ii |= (page << 8);

			retval =
			    f54->fn_ptr->read(rmi4_data, ii,
					      (unsigned char *)&rmi_fd,
					      sizeof(rmi_fd));
			if (retval < 0) {
				TS_LOG_ERR
				    ("i2c read error, page = %d, ii = %d\n",
				     page, ii);
				goto exit_free_mem;
			}

			if (!rmi_fd.fn_number) {
				TS_LOG_INFO("!rmi_fd.fn_number,page=%d,ii=%d\n",
					    page, ii);
				retval = -EINVAL;
				break;
			}

			if (rmi_fd.fn_number == SYNAPTICS_RMI4_F54) {
				hasF54 = true;
				f54->query_base_addr =
				    rmi_fd.query_base_addr | (page << 8);
				f54->control_base_addr =
				    rmi_fd.ctrl_base_addr | (page << 8);
				f54->data_base_addr =
				    rmi_fd.data_base_addr | (page << 8);
				f54->command_base_addr =
				    rmi_fd.cmd_base_addr | (page << 8);
				TS_LOG_DEBUG
				    ("f54->control_base_addr = 0x%04x, f54->data_base_addr = 0x%04x, f54->query_base_addr = 0x%04x\n",
				     f54->control_base_addr,
				     f54->data_base_addr, f54->query_base_addr);
			} else if (rmi_fd.fn_number == SYNAPTICS_RMI4_F55) {
				hasF55 = true;
				f54->fn55->query_base_addr =
				    rmi_fd.query_base_addr | (page << 8);
				f54->fn55->control_base_addr =
				    rmi_fd.ctrl_base_addr | (page << 8);
			} else if (rmi_fd.fn_number == SYNAPTICS_RMI4_F34) {
				hasF34 = true;
				f54->f34_fd.query_base_addr =
				    rmi_fd.query_base_addr;
				f54->f34_fd.ctrl_base_addr =
				    rmi_fd.ctrl_base_addr;
				f54->f34_fd.data_base_addr =
				    rmi_fd.data_base_addr;
			}

			if (hasF54 && hasF55 && hasF34) {
				TS_LOG_INFO("%s: goto found\n", __func__);
				goto found;
			}

			if (!hasF54)
				intr_count +=
				    (rmi_fd.intr_src_count & MASK_3BIT);
		}
	}

	TS_LOG_INFO
	    ("f54->control_base_addr = 0x%04x, f54->data_base_addr = 0x%04x\n",
	     f54->control_base_addr, f54->data_base_addr);

	if (!hasF54 || !hasF34) {
		TS_LOG_ERR
		    ("Funtion  is not available, hasF54=%d, hasF34 = %d\n",
		     hasF54, hasF34);
		retval = -EINVAL;
		goto exit_free_mem;
	}

found:
	retval =
	    f54->fn_ptr->read(rmi4_data, f54->query_base_addr, f54->query.data,
			      sizeof(f54->query.data));
	if (retval < 0) {
		TS_LOG_ERR("Failed to read query registers\n");
		goto exit_free_mem;
	}

	retval = Synaptics_test_set_queries();
	if (retval < 0) {
		TS_LOG_ERR("Failed to set up f54 queries registers\n");
		goto exit_free_mem;
	}

	retval = Synaptics_test_set_controls();
	if (retval < 0) {
		TS_LOG_ERR("Failed to set up f54 control registers\n");
		goto exit_free_control;
	}

	retval = synaptics_read_f34();
	if (retval) {
		TS_LOG_ERR("Read F34 failed, retval = %d\n", retval);
		goto exit_free_mem;
	}

	f54->mmi_buf = (char *)kzalloc(mmi_buf_size, GFP_KERNEL);
	if (!f54->mmi_buf) {
		TS_LOG_ERR("Failed to alloc mmi_buf\n");
		retval = -ENOMEM;
		goto exit_free_mem;
	}

	f54->rawdatabuf =
	    (int *)kzalloc(rawdata_size * sizeof(int), GFP_KERNEL);
	if (!f54->rawdatabuf) {
		TS_LOG_ERR(" Failed to alloc rawdatabuf\n");
		retval = -ENOMEM;
		goto exit_free_mem;
	}
	f54->hybridbuf =
			(u32 *)kzalloc((HYBRID_BUF_SIZE) * sizeof(int), GFP_KERNEL);
	if (!f54->hybridbuf) {
		TS_LOG_ERR(" Failed to alloc hybridbuf\n");
		retval = -ENOMEM;
		goto exit_free_mem;
	}

	return NO_ERR;

exit_free_control:
	Synaptics_test_free_control_mem();

exit_free_mem:
	synaptics_f54_free();
	return retval;
}

unsigned short synap_f54_get_calibrate_addr(struct synaptics_rmi4_data
						*rmi4_data,
						const char *module_name)
{
	int rc = NO_ERR;
	u8 value = 0;
	unsigned short addr = 0;

	TS_LOG_INFO("%s called\n", __func__);

	rc = synap_rmi4_f54_init(rmi4_data, module_name);
	if (rc < 0) {
		TS_LOG_ERR("Failed to init f54\n");
		goto exit;
	}

	rc = f54->fn_ptr->read(rmi4_data, f54->query_base_addr + 0x0B, &value,
			       sizeof(value));
	if (rc) {
		TS_LOG_ERR("Read F54 ESD check failed, retval = %d\n", rc);
		goto exit;
	}

	addr = f54->control_base_addr + 0x1d;

	if (value & 0x10) {
		addr += 2;
	}

	TS_LOG_INFO("%s, esd value is 0x%02x, addr is 0x%02x\n", __func__,
		    value, addr);

exit:
	synaptics_f54_free();
	return addr;
}
