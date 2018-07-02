/*
 * hw_rscan_data_uploader.h
 *
 * the hw_rscan_data_uploader.h for kernel data uploading through uevent.
 *
 * likun <quentin.lee@huawei.com>
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 */

#ifndef _HW_RSCAN_DATA_UPLOADER_H_
#define _HW_RSCAN_DATA_UPLOADER_H_

#include <linux/kobject.h>
#include <linux/slab.h>
#include "hw_rscan_utils.h"

/* initial length of each result data is no more than 20 */
#define RSCAN_RESULT_DATA_LENGTH 20

/* the maximum length of each rscan result is uids,whose max value is 65534 */
#define VALUE_LENGHT_MAX   20

/*
 * the length of list of rproc is limited to 840 byte,
 * otherwise, upload contents will be empty
 */
#define RPROC_VALUE_LEN_MAX  840

/*
 * rscan_uploader_init - root scan uploader initialization.
 *
 * Description: This function is to init uploader, including
 * kobject, kset, and kobj_uvent registering, and set rscan_upload_init
 * to 1
 */
int rscan_uploader_init(void);

/*
 * rscan_uploader_deinit - root scan uploader deinitialization.
 *
 * Description: This function is to deinit uploader, including
 * putting kobject, unregistering kset, and and set rscan_upload_init
 * to 0
 */
void rscan_uploader_deinit(void);

/*
 * rscan_data_upload - an API for root scanner to upload its result.
 * @rstatus, current root status of the device,
 * @err_code, return code of root scan process,
 * @result, details result correspond to rstatus
 *
 * Description: After root scan, need to call this interface
 * to upload the current root status and its details.
 */
int rscan_data_upload(uint rstatus, int err_code,
				struct rscan_result_dynamic *result);

#endif
