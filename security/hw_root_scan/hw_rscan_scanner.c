/*
 * hw_rscan_scanner.c
 *
 * the hw_rscan_scanner.c for kernel space root scan
 *
 * Yongzheng Wu <Wu.Yongzheng@huawei.com>
 * likun <quentin.lee@huawei.com>
 * likan <likan82@huawei.com>
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 */

#include "./include/hw_rscan_scanner.h"

#define VAR_NOT_USED(variable)  do{(void)(variable);}while(0);
#define KCODE_OFFSET 0
#define SYSCALL_OFFSET 1
#define SEHOOKS_OFFSET 2
#define SESTATUS_OFFSET 3

static DEFINE_MUTEX(scanner_lock);	/* lint -save -e64 -e785 -e708 -e570 */
static DEFINE_MUTEX(whitelist_lock);	/* lint -save -e64 -e785 -e708 -e570 */
static const char *TAG = "hw_rscan_scanner";
static const char *DEFAULT_PROC = "/init";

struct rscan_skip_flags g_rscan_skip_flag = {
	.skip_kcode = NOT_SKIP,
	.skip_syscall = NOT_SKIP,
	.skip_se_hooks = NOT_SKIP,
	.skip_se_status = NOT_SKIP,
	.skip_rprocs = NOT_SKIP,
};

static struct rscan_result_dynamic g_rscan_clean_scan_result;

static int rscan_dynamic_raw_unlock(uint op_mask,
					struct rscan_result_dynamic *result)
{
	int ret = 0;
	int error_code = 0;

	if (op_mask & D_RSOPID_KCODE) {
		ret = kcode_scan(result->kcode);
		if (ret != 0) {
			error_code |= D_RSOPID_KCODE;
			RSLogError(TAG, "kcode_scan failed");
		}
	}

	if (op_mask & D_RSOPID_SYS_CALL) {
		ret = kcode_syscall_scan(result->syscalls);
		if (ret != 0) {
			error_code |= D_RSOPID_SYS_CALL;
			RSLogError(TAG, "kcode system call scan failed");
		}
	}

	if (op_mask & D_RSOPID_SE_HOOKS) {
		ret = sescan_hookhash(result->sehooks);
		if (ret != 0) {
			error_code |= D_RSOPID_SE_HOOKS;
			RSLogError(TAG, "sescan_hookhash scan failed");
		}
	}

	if (op_mask & D_RSOPID_SE_STATUS)
		result->seenforcing = get_selinux_enforcing();

	if (op_mask & D_RSOPID_RRPOCS) {
#ifdef CONFIG_RSCAN_SKIP_RPROCS
		strncpy(result->rprocs, "/init", sizeof(result->rprocs));
#else
		ret = get_root_procs(result->rprocs, sizeof(result->rprocs));
		if (0 == ret) {
			error_code |= D_RSOPID_RRPOCS;
			RSLogError(TAG, "root processes scan failed!");
		}
#endif
	}

	return error_code;
}

/* return: mask of bad scans items result */
int rscan_dynamic(uint op_mask, struct rscan_result_dynamic *result,
							int *error_code)
{
	int bad_mask = 0;

	if (result == NULL || error_code == NULL) {
		RSLogError(TAG, "input parameters error!");
		return -EINVAL;
	}

	*error_code = rscan_dynamic_raw_unlock(op_mask, result);
	if (*error_code != 0)
		RSLogWarning(TAG, "some item of root scan failed");

	if ((op_mask & D_RSOPID_KCODE)
		&& (g_rscan_skip_flag.skip_kcode == NOT_SKIP)
		&& (memcmp(result->kcode, g_rscan_clean_scan_result.kcode,
			sizeof(result->kcode)) != 0)) {
		bad_mask |= D_RSOPID_KCODE;
		RSLogDebug(TAG, "kernel code is abnormal");
	}

	if ((op_mask & D_RSOPID_SYS_CALL)
		&& (g_rscan_skip_flag.skip_syscall == NOT_SKIP)
		&& (memcmp(result->syscalls, g_rscan_clean_scan_result.syscalls,
			sizeof(result->syscalls)) != 0)) {
		bad_mask |= D_RSOPID_SYS_CALL;
		RSLogDebug(TAG, "kernel system call is abnormal");
	}

	if ((op_mask & D_RSOPID_SE_HOOKS)
		&& (g_rscan_skip_flag.skip_se_hooks == NOT_SKIP)
		&& (memcmp(result->sehooks, g_rscan_clean_scan_result.sehooks,
			sizeof(result->sehooks)) != 0)) {
		bad_mask |= D_RSOPID_SE_HOOKS;
		RSLogDebug(TAG, "SeLinux hooks is abnormal");
	}

	if ((op_mask & D_RSOPID_SE_STATUS)
		&& (g_rscan_skip_flag.skip_se_status == NOT_SKIP)
		&& (result->seenforcing !=
				g_rscan_clean_scan_result.seenforcing)) {
		bad_mask |= D_RSOPID_SE_STATUS;
		RSLogDebug(TAG, "SeLinux enforcing status is abnormal");
	}

	if ((op_mask & D_RSOPID_RRPOCS)
		&& (g_rscan_skip_flag.skip_rprocs == NOT_SKIP)) {
		rprocs_strip_whitelist(result->rprocs,
					(ssize_t)sizeof(result->rprocs));
		if (result->rprocs[0]) {
			bad_mask |= D_RSOPID_RRPOCS;
			RSLogDebug(TAG, "root processes are abnormal");
		}
	}

	RSLogTrace(TAG, "root scan finished.");
	return bad_mask;
}

int rscan_dynamic_raw(uint op_mask, struct rscan_result_dynamic *result)
{
	int error_code = 0;

	if (NULL == result) {
		RSLogError(TAG, "input parameter is invalid");
		return -EINVAL;
	}

	mutex_lock(&scanner_lock);
	error_code = rscan_dynamic_raw_unlock(op_mask, result);
	mutex_unlock(&scanner_lock);

	return error_code;
}

#ifdef CONFIG_ARCH_MSM
int get_battery_status(int *is_charging, int *percentage)
{
	union power_supply_propval status, capacity;
	struct power_supply *psy = power_supply_get_by_name(BATTERY_NAME);

	if (psy == NULL)
		return -EINVAL;
	if (is_charging && !psy->get_property(psy, POWER_SUPPLY_PROP_STATUS,
		&status)) {
		*is_charging = status.intval == POWER_SUPPLY_STATUS_CHARGING
			|| status.intval == POWER_SUPPLY_STATUS_FULL;
	}
	if (percentage && !psy->get_property(psy, POWER_SUPPLY_PROP_CAPACITY,
		&capacity))
		*percentage = capacity.intval;
	return 0;
}
#else
int get_battery_status(int *is_charging, int *percentage)
{
	union power_supply_propval status, capacity;
	struct power_supply *psy = NULL;

	if ((is_charging == NULL) || (percentage == NULL)) {
		RSLogError(TAG, "input parameters invalid");
		return -EINVAL;
	}

	psy = power_supply_get_by_name(BATTERY_NAME);
	if (psy == NULL)
		return -1;

	/* is_charging never be NULL because of input parameters check */
	if (!power_supply_get_property(psy, POWER_SUPPLY_PROP_STATUS, &status))
		*is_charging = (status.intval == POWER_SUPPLY_STATUS_CHARGING)
				|| (status.intval == POWER_SUPPLY_STATUS_FULL);

	/* percentage never be NULL because of input parameters check */
	if (!power_supply_get_property(psy, POWER_SUPPLY_PROP_CAPACITY,
								&capacity))
		*percentage = capacity.intval;

	return 0;
}
#endif

int rscan_get_status(struct rscan_status *status)
{
	int is_charging = 0, percentage = 0;
	struct timeval tv;
	int result = 0;

	if (!status) {
		RSLogError(TAG, "input parameter is invalid");
		return -EINVAL;
	}

	status->cpuload = 0;

	if (!get_battery_status(&is_charging, &percentage)) {
		status->battery = (uint32_t)percentage;
		status->charging = (uint32_t)is_charging;
	} else {
		RSLogWarning(TAG, "rootscan: get_battery_status failed");
		status->battery = 0;
		status->charging = 0;
		result = 1;
	}

	do_gettimeofday(&tv);
	status->time = (uint32_t)tv.tv_sec;
	status->timezone = (uint32_t)sys_tz.tz_minuteswest;

	return result;
}

int load_rproc_whitelist(char *whitelist, size_t len)
{
	struct file *filp;
	mm_segment_t oldfs;
	ssize_t size = 0;
	loff_t pos;

	if (NULL == whitelist) {
		RSLogError(TAG, "input parameter is invalid");
		return -EINVAL;
	}

	mutex_lock(&whitelist_lock);
	oldfs = get_fs();
	set_fs(get_ds());  /* lint -save -e501 */
	filp = filp_open(FILE_RPROC_WHITE_LIST, O_RDONLY, 0);
	set_fs(oldfs);
	if (IS_ERR(filp)) {
		RSLogError(TAG, "rootscan: opening %s failed.",
					FILE_RPROC_WHITE_LIST);
		mutex_unlock(&whitelist_lock);
		return -1;
	}

	oldfs = get_fs();
	set_fs(get_ds());  /* lint -save -e501 */
	pos = 0;
	size = vfs_read(filp, whitelist, len, &pos);
	set_fs(oldfs);

	filp_close(filp, NULL);

	if (size <= 0 || size >= len) {
		RSLogError(TAG, "rootscan: reading %s failed. size is %d",
					FILE_RPROC_WHITE_LIST, (int)size);
		mutex_unlock(&whitelist_lock);
		return -1;
	}

	RSLogDebug(TAG, "rootscan: reading %zd bytes from %s",
				size, FILE_RPROC_WHITE_LIST);
	whitelist[size] = '\0';
	mutex_unlock(&whitelist_lock);

	return 0;
}

int rscan_init_data(void)
{
	int ret = 0;

	/* initialize g_rscan_clean_scan_result */
	memset(&g_rscan_clean_scan_result, 0,
					sizeof(struct rscan_result_dynamic));

	g_rscan_clean_scan_result.seenforcing = 1;

	ret = load_rproc_whitelist(g_rscan_clean_scan_result.rprocs,
				sizeof(g_rscan_clean_scan_result.rprocs));
	if (ret != 0
		|| !init_rprocs_whitelist(g_rscan_clean_scan_result.rprocs)) {
		RSLogError(TAG, "load root whitelist failed, rproc will skip");
		strncpy(g_rscan_clean_scan_result.rprocs,
				DEFAULT_PROC, strlen(DEFAULT_PROC));
		g_rscan_skip_flag.skip_rprocs = SKIP;
	}

	ret = rscan_dynamic_raw(D_RSOPID_KCODE
					| D_RSOPID_SYS_CALL
					| D_RSOPID_SE_HOOKS,
					&g_rscan_clean_scan_result);
	if (ret != 0) {
		if (ret & D_RSOPID_KCODE) {
			RSLogError(TAG, "rscan D_RSOPID_KCODE init failed");
			g_rscan_skip_flag.skip_kcode = SKIP;
		}

		if (ret & D_RSOPID_SYS_CALL) {
			RSLogError(TAG, "rscan D_RSOPID_SYS_CALL init failed");
			g_rscan_skip_flag.skip_syscall = SKIP;
		}

		if (ret & D_RSOPID_SE_HOOKS) {
			RSLogError(TAG, "rscan D_RSOPID_SE_HOOKS init failed");
			g_rscan_skip_flag.skip_se_hooks = SKIP;
		}
	}

	return 0;
}

int rscan_trigger(void)
{
	int result = 0;
	int scan_err_code = 0;
	int root_masks = 0;
	int dynamic_ops = 0;
	struct rscan_result_dynamic *scan_result_buf = NULL;

#ifndef RS_DEBUG
	if (RO_NORMAL != get_ro_secure()) {
		RSLogTrace(TAG, "in engneering mode, root scan stopped");
		return RSCAN_ERR_SECURE_MODE;
	}
#endif

	mutex_lock(&scanner_lock);
	scan_result_buf = vmalloc(sizeof(struct rscan_result_dynamic));
	if (NULL == scan_result_buf) {
		RSLogError(TAG, "no enough space for scan_result_buf");
		mutex_unlock(&scanner_lock);
		return -ENOSPC;
	}
	memset(scan_result_buf, 0, sizeof(struct rscan_result_dynamic));

	dynamic_ops = RSOPID_ALL;
	root_masks = rscan_dynamic(dynamic_ops, scan_result_buf,
					&scan_err_code);
	if (0 != root_masks)
		RSLogTrace(TAG, "root scan err code is normal");

	result = rscan_data_upload(root_masks, scan_err_code, scan_result_buf);
	if (0 != result)
		RSLogError(TAG, "data upload failed, result is %d", result);

	/* scan_result_buf never evaluates to NULL */
	vfree(scan_result_buf);
	scan_result_buf = NULL;

	mutex_unlock(&scanner_lock);

	RSLogTrace(TAG, "scan and upload finished. result: %d", result);
	return result;
}

static int dynamic_call(unsigned int mask)
{
	int root_status = 0;

	RSLogDebug(TAG, "set %d scan resume", mask);
	root_status = rscan_dynamic_raw_unlock(mask,
		&g_rscan_clean_scan_result);
	if (root_status == 0 && (mask & D_RSOPID_KCODE))
		g_rscan_skip_flag.skip_kcode = NOT_SKIP;
	if (root_status == 0 && (mask & D_RSOPID_SYS_CALL))
		g_rscan_skip_flag.skip_syscall = NOT_SKIP;
	if (root_status == 0 && (mask & D_RSOPID_SE_HOOKS))
		g_rscan_skip_flag.skip_se_hooks = NOT_SKIP;
	if (root_status == 0 && (mask & D_RSOPID_SE_STATUS))
		g_rscan_skip_flag.skip_se_status = NOT_SKIP;
	return root_status;
}

static int __root_scan_pause(unsigned int op_mask, void *reserved)
{
	VAR_NOT_USED(reserved);

	g_rscan_skip_flag.skip_kcode = SKIP
		& ((op_mask & D_RSOPID_KCODE) >> KCODE_OFFSET);
	g_rscan_skip_flag.skip_syscall = SKIP
		& ((op_mask & D_RSOPID_SYS_CALL) >> SYSCALL_OFFSET);
	g_rscan_skip_flag.skip_se_hooks = SKIP
		& ((op_mask & D_RSOPID_SE_HOOKS) >> SEHOOKS_OFFSET);
	g_rscan_skip_flag.skip_se_status = SKIP
		& ((op_mask & D_RSOPID_SE_STATUS) >> SESTATUS_OFFSET);
	RSLogDebug(TAG, "set scan pause, pause mask %d", op_mask);
	return 0;
}

static int __root_scan_resume(unsigned int op_mask, void *reserved)
{
	VAR_NOT_USED(reserved);

	unsigned int resume_mask = 0;

	if ((op_mask & D_RSOPID_KCODE)
		&& SKIP == g_rscan_skip_flag.skip_kcode)
			resume_mask |= D_RSOPID_KCODE;
	if ((op_mask & D_RSOPID_SYS_CALL)
		&& SKIP == g_rscan_skip_flag.skip_syscall)
			resume_mask |= D_RSOPID_SYS_CALL;
	if ((op_mask & D_RSOPID_SE_HOOKS)
		&& SKIP == g_rscan_skip_flag.skip_se_hooks)
			resume_mask |= D_RSOPID_SE_HOOKS;
	if ((op_mask & D_RSOPID_SE_STATUS)
		&& SKIP == g_rscan_skip_flag.skip_se_status)
			resume_mask |= D_RSOPID_SE_STATUS;

	return dynamic_call(resume_mask);
}

int root_scan_pause(unsigned int op_mask, void *reserved)
{
	VAR_NOT_USED(reserved);

	int result = 0;
	int scan_err_code = 0;
	int root_status = 0;
	int dynamic_ops = 0;
	struct rscan_result_dynamic *scan_result_buf = NULL;

#ifndef RS_DEBUG
	if (RO_NORMAL != get_ro_secure()) {
		RSLogTrace(TAG, "in engneering mode, root scan stopped");
		return RSCAN_ERR_SECURE_MODE;
	}
#endif

	struct timeval tv;

	do_gettimeofday(&tv);
	RSLogTrace(TAG, "pause item:%d, time:%ld:%ld", op_mask,
			tv.tv_sec, tv.tv_usec);

	mutex_lock(&scanner_lock);
	scan_result_buf = vmalloc(sizeof(struct rscan_result_dynamic));
	if (NULL == scan_result_buf) {
		RSLogError(TAG, "no enough space for scan_result_buf");
		mutex_unlock(&scanner_lock);
		return -ENOSPC;
	}
	memset(scan_result_buf, 0, sizeof(struct rscan_result_dynamic));

	/*do scan before pause rootscan*/
	dynamic_ops = D_RSOPID_KCODE | D_RSOPID_SYS_CALL
			| D_RSOPID_SE_HOOKS | D_RSOPID_SE_STATUS;
	root_status = rscan_dynamic(dynamic_ops,
			scan_result_buf, &scan_err_code);
	if (0 == root_status) {
		RSLogTrace(TAG, "environment clean ,pause root scan go");
		result = __root_scan_pause(op_mask, reserved);
	} else {
		RSLogTrace(TAG, "already rooted, skip pause.");
		result = root_status;
	}
	mutex_unlock(&scanner_lock);

	/* scan_result_buf never evaluates to NULL */
	vfree(scan_result_buf);
	scan_result_buf = NULL;
	return result;
}

int root_scan_resume(unsigned int op_mask, void *reserved)
{
	VAR_NOT_USED(reserved);

	int result = 0;

#ifndef RS_DEBUG
	if (RO_NORMAL != get_ro_secure()) {
		RSLogTrace(TAG, "in engneering mode, root scan stopped");
		return RSCAN_ERR_SECURE_MODE;
	}
#endif

	struct timeval tv;

	do_gettimeofday(&tv);
	RSLogTrace(TAG, "resume item:%d, time:%ld:%ld",
			op_mask, tv.tv_sec, tv.tv_usec);

	mutex_lock(&scanner_lock);
	result = __root_scan_resume(op_mask, reserved);
	mutex_unlock(&scanner_lock);
	return result;
}

int rscan_dynamic_init(void)
{
	if (rscan_init_data()) {
		RSLogError(TAG, "rootscan: rscan init data failed");
		return RSCAN_ERR_SCANNER_INIT;
	}

	return 0;
}
