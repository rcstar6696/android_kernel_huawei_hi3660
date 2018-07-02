/*
 * hw_rscan_scanner.h
 *
 * the hw_rscan_scanner.h for kernel space root scan
 *
 * Yongzheng Wu <Wu.Yongzheng@huawei.com>
 * likun <quentin.lee@huawei.com>
 * likan <likan82@huawei.com>
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 */

#ifndef _HW_RSCAN_DYNAMIC_H_
#define _HW_RSCAN_DYNAMIC_H_

#include <chipset_common/security/root_scan.h>
#include <linux/fs.h>	/* for reading "/rootscan.conf" */
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/power_supply.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/uaccess.h>	/* for reading "/rootscan.conf" */
#include "kcode.h"
#include "rproc.h"
#include "sescan.h"
#include "hw_rscan_interface.h"
#include "hw_rscan_utils.h"
#include "hw_rscan_data_uploader.h"

#define BATTERY_NAME "Battery"
#define FILE_RPROC_WHITE_LIST "/rootscan.conf"

int rscan_dynamic_init(void);

/*
 * rscan_dynamic - for kernel space root scan
 * Description: root scan by dynamic features, with result processed
 * @ops_mask, bit masks for caller to set different items
 *         for scanner. The lowest bit is for kernel code check, then the
 *         30th bit for system call verify, and so on.
 * @result, used for return results for each scanner item
 *          that correspond to ops_mask. In this function, result have been
 *          processed. For example, rproc had filtered whitelist.
 * @error_code, scan process result.
 * @return:
 *     Overall root status, return 0 if the device keep unrooted.
 *     Otherwise return an interger that each of its bit corresponds
 *     to its bitmasks
 */
int rscan_dynamic(uint op_mask, struct rscan_result_dynamic *result,
							int *error_code);

#endif
