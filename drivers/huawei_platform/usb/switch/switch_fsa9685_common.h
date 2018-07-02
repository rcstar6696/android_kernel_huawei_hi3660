


#ifndef _SWITCH_FSA9685_COMMON
#define _SWITCH_FSA9685_COMMON

#include <linux/types.h>

struct switch_extra_ops {
	int (*manual_switch)(int input_select);
	int (*dcd_timeout_enable)(bool enable_flag);
	int (*manual_detach)(void);
};

int switch_extra_ops_register(struct switch_extra_ops *ops);
int fsa9685_manual_sw(int input_select);
int fsa9685_manual_detach(void);
int fsa9685_dcd_timeout_enable(bool enable_flag);
#endif
