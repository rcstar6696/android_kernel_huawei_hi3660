

#include "switch_fsa9685_common.h"
#include <huawei_platform/log/hw_log.h>
#include <protocol.h>

#define HWLOG_TAG sensorhub
HWLOG_REGIST();

struct switch_extra_ops *g_switch_ops;
#ifdef CONFIG_INPUTHUB
extern sys_status_t iom3_sr_status;
#endif

int switch_extra_ops_register(struct switch_extra_ops *ops)
{
	int ret = 0;

	if (ops != NULL) {
		g_switch_ops = ops;
	} else {
		hwlog_err("charge extra ops register fail!\n");
		ret = -EPERM;
	}
	return ret;
}

int fsa9685_manual_sw(int input_select)
{
	if (NULL == g_switch_ops || NULL == g_switch_ops->manual_switch
#ifdef CONFIG_INPUTHUB
	|| iom3_sr_status == ST_SLEEP
#endif
	) {
		hwlog_err("g_switch_ops is NULL or sensorhub is sleep.\n");
		return -1;
	}
	return g_switch_ops->manual_switch(input_select);
}

int fsa9685_manual_detach(void)
{
	if (NULL == g_switch_ops || NULL == g_switch_ops->manual_detach
#ifdef CONFIG_INPUTHUB
	|| iom3_sr_status == ST_SLEEP
#endif
	) {
		hwlog_err("g_switch_ops is NULL or sensorhub is sleep.\n");
		return 0;
	}
	return g_switch_ops->manual_detach();
}

int fsa9685_dcd_timeout_enable(bool enable_flag)
{
	if (NULL == g_switch_ops || NULL == g_switch_ops->dcd_timeout_enable
#ifdef CONFIG_INPUTHUB
	|| iom3_sr_status == ST_SLEEP
#endif
	) {
		hwlog_err("g_switch_ops is NULL or sensorhub is sleep.\n");
		return -1;
	}
	return g_switch_ops->dcd_timeout_enable(enable_flag);
}
