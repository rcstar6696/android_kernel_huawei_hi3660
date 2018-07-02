#ifndef __DUAL_LOADSWITCH_H_
#define __DUAL_LOADSWITCH_H_

#include <huawei_platform/power/direct_charger.h>
enum dual_loadswitch_sysfs_type {
	DUAL_LOADSWITCH_SYSFS_ENABLE_MAIN = 0,
	DUAL_LOADSWITCH_SYSFS_ENABLE_AUX,
};

struct dual_loadswitch_info {
	struct device *dev;
	int loadswitch_main_enable_sysfs;
	int loadswitch_aux_enable_sysfs;
};

#ifdef CONFIG_LLT_TEST
#endif
int loadswitch_main_ops_register(struct loadswitch_ops *);
int loadswitch_aux_ops_register(struct loadswitch_ops *);

#endif
