/*
 * Huawei Touchscreen Driver
 *
 * Copyright (C) 2017 Huawei Device Co.Ltd
 * License terms: GNU General Public License (GPL) version 2
 *
 */

#ifndef _INPUT_MT_WRAPPER_H_
#define _INPUT_MT_WRAPPER_H_

#define INPUT_MT_WRAPPER_MAX_FINGERS (10)
#define THP_MT_WRAPPER_GAIN (100)
#define THP_MT_WRAPPER_MAX_FINGERS (10)
#define THP_MT_WRAPPER_MAX_X (1079)
#define THP_MT_WRAPPER_MAX_Y (1919)
#define THP_MT_WRAPPER_MAX_Z (THP_MT_WRAPPER_GAIN)
#define THP_MT_WRAPPER_MAX_MAJOR (1)
#define THP_MT_WRAPPER_MAX_MINOR (1)
#define THP_MT_WRAPPER_MIN_ORIENTATION (-90*THP_MT_WRAPPER_GAIN)
#define THP_MT_WRAPPER_MAX_ORIENTATION  (90*THP_MT_WRAPPER_GAIN)
#define THP_MT_WRAPPER_TOOL_TYPE_MAX (1)
#define THP_MT_WRAPPER_TOOL_TYPE_STYLUS (1)

#define THP_INPUT_DEV_COMPATIBLE "huawei,thp_input"

enum input_mt_wrapper_state {
	INPUT_MT_WRAPPER_STATE_DEFAULT,
	INPUT_MT_WRAPPER_STATE_FIRST_TOUCH = 1,
	INPUT_MT_WRAPPER_STATE_LAST_TOUCH = 2,
	INPUT_MT_WRAPPER_STATE_SAME_REPORT,
	INPUT_MT_WRAPPER_STATE_SAME_ZERO_REPORT,
};

#define THP_INPUT_DEVICE_NAME	"input_mt_wrapper"
/*#define TYPE_B_PROTOCOL*/


struct thp_input_dev_config {
	int abs_max_x;
	int abs_max_y;
	int abs_max_z;
	int tracking_id_max;
	int major_max;
	int minor_max;
	int orientation_min;
	int orientation_max;
	int tool_type_max;
};

struct thp_mt_wrapper_data {
	struct input_dev *input_dev;
	struct thp_input_dev_config input_dev_config;
};

struct input_mt_wrapper_touch_data {
	unsigned char down;
	unsigned char valid; /* 0:invalid !=0:valid */
	int x;
	int y;
	int pressure;
	int tracking_id;
	int shape;
	int major;
	int minor;
	int orientation;
	unsigned int tool_type;
};

struct thp_mt_wrapper_ioctl_touch_data {
	struct input_mt_wrapper_touch_data touch[INPUT_MT_WRAPPER_MAX_FINGERS];
	enum input_mt_wrapper_state state;
	int t_num;
	int down_num;
};

/* commands */
#define INPUT_MT_WRAPPER_IO_TYPE  (0xB9)
#define INPUT_MT_WRAPPER_IOCTL_CMD_SET_COORDINATES \
	_IOWR(INPUT_MT_WRAPPER_IO_TYPE, 0x01, \
		struct thp_mt_wrapper_ioctl_touch_data)

int thp_mt_wrapper_init(void);
void thp_mt_wrapper_exit(void);

#endif /* _INPUT_MT_WRAPPER_H_ */
