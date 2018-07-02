/*
 * hw_rscan_module.c
 *
 * the hw_rscan_module.c for root scanner kernel space init and deinit
 *
 * likun <quentin.lee@huawei.com>
 * likan <likan82@huawei.com>
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/types.h>
#include "./include/hw_rscan_utils.h"
#include "./include/hw_rscan_data_uploader.h"
#include "./include/hw_rscan_scanner.h"
#include "./include/hw_rscan_proc.h"

static int __init rscan_module_init(void);
static void __exit rscan_module_exit(void);
static const char *TAG = "hw_rscan_module";

struct rscan_module_work {
	struct workqueue_struct *rscan_wq;
	struct work_struct rscan_work;
};

static struct rscan_module_work rscan_work_data;

static void rscan_work_init(struct work_struct *data)
{
	int result = 0;

#ifndef RS_DEBUG
	if (RO_NORMAL != get_ro_secure()) {
		RSLogTrace(TAG, "in engneering mode, root scan stopped");
		return;
	}
#endif

	RSLogDebug(TAG, "rscan work init.");

	do {
		/* init uploader */
		result = rscan_uploader_init();
		if (result != 0)
			break;

		/* init dynamic scanner */
		result = rscan_dynamic_init();
		if (result != 0) {
			RSLogError(TAG, "dynamic scanner init failed: %d",
						result);
			break;
		}

		/* init proc file */
		result = rscan_proc_init();
		if (result != 0) {
			RSLogError(TAG, "rscan_proc_init init failed.");
			break;
		}
	} while (0);

	if (0 != result) {
		/* The function __init should not references __exit*/
		/*rscan_module_exit();*/
		rscan_proc_deinit();
		rscan_uploader_deinit();
	}

	RSLogTrace(TAG, "+++root scan init end, result:%d", result);
}

static int __init rscan_module_init(void)
{
	rscan_work_data.rscan_wq =
				create_singlethread_workqueue("HW_ROOT_SCAN");

	if (rscan_work_data.rscan_wq == NULL) {
		RSLogError(TAG, "rscan module wq error, no mem");
		return -ENOMEM;
	}

	INIT_WORK(&(rscan_work_data.rscan_work), rscan_work_init);
	queue_work(rscan_work_data.rscan_wq, &(rscan_work_data.rscan_work));

	return 0;
}

static void __exit rscan_module_exit(void)
{
	rscan_proc_deinit();
	rscan_uploader_deinit();
	destroy_workqueue(rscan_work_data.rscan_wq);
}

late_initcall(rscan_module_init);   /* lint -save -e528 */
module_exit(rscan_module_exit);   /* lint -save -e528 */

MODULE_AUTHOR("likun <quentin.lee@huawei.com>");
MODULE_DESCRIPTION("Huawei root scanner");
