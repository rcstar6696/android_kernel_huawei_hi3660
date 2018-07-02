/*
 * hw_rscan_data_uploader.c
 *
 * the hw_rscan_data_uploader.c for kernel data uploading through uevent.
 *
 * likun <quentin.lee@huawei.com>
 * likan <likan82@huawei.com>
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 */

#include "./include/hw_rscan_data_uploader.h"

static const char *TAG = "hw_rscan_data_uploader";

static struct kobject *rscan_kobj;
static struct kset *rscan_kset;
static int rscan_upload_init = RSCAN_UNINIT;
static DEFINE_MUTEX(upload_mutex);

enum {
	RSCAN_TYPE_INT,
	RSCAN_TYPE_STRING,
	RSCAN_TYPE_NULL,
};

enum {
	RSCAN_ROOT_STATUS = 0,
	RSCAN_ERR_CODE,
	RSCAN_KCODE,
	RSCAN_SYSCALL,
	RSCAN_SE_HOOKS,
	RSCAN_SE_STATUS,
	RSCAN_RPROCS,
	RSCAN_NULL,
	RSCAN_LAST,
};

struct rscan_key_info {
	uint key_id;
	const char *keyname;
	uint type;
	uint mask;
};

struct rscan_key_info key_info[] = {
	{
		RSCAN_ROOT_STATUS,
		"ROOT_STATUS=",
		RSCAN_TYPE_INT,
		D_RSOPID_MASK_NONE,
	},
	{
		RSCAN_ERR_CODE,
		"ERR_CODE=",
		RSCAN_TYPE_INT,
		D_RSOPID_MASK_NONE,
	},
	{
		RSCAN_KCODE,
		"KCODE=",
		RSCAN_TYPE_INT,
		D_RSOPID_KCODE,
	},
	{
		RSCAN_SYSCALL,
		"SYS_CALL=",
		RSCAN_TYPE_INT,
		D_RSOPID_SYS_CALL,
	},
	{
		RSCAN_SE_HOOKS,
		"SE_HOOKS=",
		RSCAN_TYPE_INT,
		D_RSOPID_SE_HOOKS,
	},
	{
		RSCAN_SE_STATUS,
		"SE_STATUS=",
		RSCAN_TYPE_INT,
		D_RSOPID_SE_STATUS,
	},
	{
		RSCAN_RPROCS,
		"RPROCS=",
		RSCAN_TYPE_STRING,
		0,
	},
	{
		RSCAN_NULL,
		NULL,
		RSCAN_TYPE_NULL,
		0,
	},
};

int rscan_uploader_init(void)
{
	const char *rscan_kobj_name = "hw_root_scanner";
	const char *rscan_kset_name = "hw_rscan_kset";
	int error = RSCAN_SUCC;

	if (rscan_upload_init == RSCAN_INIT) {
		RSLogTrace(TAG, "rscan_kobj_init already init!");
		return RSCAN_SUCC;
	}

	do {
		rscan_kobj = kobject_create_and_add(rscan_kobj_name,
								kernel_kobj);
		if (!rscan_kobj) {
			RSLogError(TAG, "creat kobject failed");
			error = RSCAN_ERR_KOBJ_ADD;
			break;
		}

		rscan_kset =  kset_create_and_add(rscan_kset_name, NULL,
								kernel_kobj);
		if (!rscan_kset) {
			RSLogError(TAG, "creat kset failed");
			error = RSCAN_ERR_KSET_ADD;
			break;
		}
		rscan_kobj->kset = rscan_kset;

		error = kobject_uevent(rscan_kobj, KOBJ_ADD);
		if (error) {
			RSLogError(TAG, "kobj_uevent add failed, result is %d",
						error);
			error = RSCAN_ERR_KOBJECT_EVENT;
			break;
		}

		/* init ok */
		RSLogTrace(TAG, "rscan_kobj_init ok!");
		rscan_upload_init = RSCAN_INIT;
		return RSCAN_SUCC;
	} while (0);

	RSLogError(TAG, "rscan_kobj_init failed, error = %d!", error);
	rscan_uploader_deinit();
	return error;
}

void rscan_uploader_deinit(void)
{
	if (rscan_kobj) {
		kobject_put(rscan_kobj);
		rscan_kobj = NULL;
	}

	if (rscan_kset) {
		kset_unregister(rscan_kset);
		rscan_kset = NULL;
	}

	RSLogDebug(TAG, "rscan_kobj_deinit ok!");
	rscan_upload_init = RSCAN_UNINIT;
}

/* concatenate rscan data of type int and type char array */
static int rscan_data_adapter(char **uevent_envp, uint rstatus, int err_code,
					struct rscan_result_dynamic *result)
{
	int index = 0;
	size_t rscan_item_len = 0;
	size_t rscan_value_len = 0;

	if ((NULL == uevent_envp)
		||  (NULL == result)) {
		RSLogError(TAG, "input arguments invalid");
		return -EINVAL;
	}

	for (index = 0; index < RSCAN_LAST; index++) {
		if (key_info[index].type == RSCAN_TYPE_NULL)
			continue;

		if (key_info[index].type == RSCAN_TYPE_INT) {
			rscan_value_len = VALUE_LENGHT_MAX;
		} else {
			rscan_value_len = strnlen(result->rprocs,
					sizeof(result->rprocs) * sizeof(char));
			if (rscan_value_len > RPROC_VALUE_LEN_MAX) {
				rscan_value_len = RPROC_VALUE_LEN_MAX;
				result->rprocs[rscan_value_len] = '\0';
			}
		}
		rscan_item_len = strlen(key_info[index].keyname)
					+ rscan_value_len;

		uevent_envp[index] = vmalloc((rscan_item_len + 1)
							* (sizeof(char)));
		if (NULL == uevent_envp[index]) {
			RSLogError(TAG, "no space for uevent_envp[%d]", index);
			return -ENOSPC;
		}
		memset(uevent_envp[index], 0, rscan_item_len + 1);

		if (key_info[index].type == RSCAN_TYPE_INT) {
			if (key_info[index].key_id == RSCAN_ROOT_STATUS) {
				snprintf(uevent_envp[index], rscan_item_len,
					"%s%d",
					key_info[index].keyname, rstatus);
			} else if (key_info[index].key_id ==
							(uint)RSCAN_ERR_CODE) {
				snprintf(uevent_envp[index], rscan_item_len,
					"%s%d",
					key_info[index].keyname, err_code);
			} else {
				snprintf(uevent_envp[index], rscan_item_len,
					"%s%d",
					key_info[index].keyname,
					!!(rstatus & key_info[index].mask));
			}
		} else {
			/*
			 * never use snprintf(uevent_envp[index],
			 *			rscan_item_len, "%s%s",
			 * ... or it'll make kernel crash
			 */
			strncpy(uevent_envp[index], key_info[index].keyname,
					strlen(key_info[index].keyname));
			strncat(uevent_envp[index], result->rprocs,
					rscan_value_len);
		}
	}

	return 0;
}

int rscan_data_upload(uint rstatus, int err_code,
					struct rscan_result_dynamic *result)
{
	int ret = 0;
	char *uevent_envp[RSCAN_LAST] = {NULL};
	int index = 0;

	if (RSCAN_UNINIT == rscan_upload_init) {
		RSLogError(TAG, "rscan_data_upload failed, not init!");
		return -EINVAL;
	}

	if ((NULL == rscan_kobj) || (NULL == result)) {
		RSLogError(TAG, "input arguments invalid");
		return -EINVAL;
	}

	do {
		ret = rscan_data_adapter(uevent_envp, rstatus,
						err_code, result);
		if (ret != 0) {
			RSLogError(TAG, "data adpter failed, ret is %d", ret);
			break;
		}

#ifdef RS_DEBUG
		for (index = 0; index <	 RSCAN_LAST; index++) {
			if (NULL != uevent_envp[index]) {
				RSLogDebug(TAG, "uevent_envp[%d] is %s", index,
						uevent_envp[index]);
			}
		}
#endif

		ret = kobject_uevent_env(rscan_kobj, KOBJ_CHANGE, uevent_envp);
		if (0 != ret)
			RSLogError(TAG, "kobj upload failed, ret is %d", ret);
	} while (0);

	for (index = 0; index < RSCAN_LAST; index++) {
		vfree(uevent_envp[index]);
		uevent_envp[index] = NULL;
	}

	RSLogTrace(TAG, "event upload finished. result: %d", ret);
	return ret;
}
