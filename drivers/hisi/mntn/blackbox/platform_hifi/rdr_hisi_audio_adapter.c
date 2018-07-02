/*
 * audio rdr adpter.
 *
 * Copyright (c) 2015 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/uaccess.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/kthread.h>
#include <linux/thread_info.h>
#include <linux/notifier.h>
#include <linux/syscalls.h>
#include <linux/errno.h>

#include <linux/hisi/util.h>
#include <linux/hisi/rdr_hisi_ap_hook.h>

#include "rdr_print.h"
#include "rdr_inner.h"
#include "rdr_hisi_audio_adapter.h"
#include "rdr_hisi_audio_codec.h"
#include "rdr_hisi_audio_soc.h"

/*lint -e730*/


struct rdr_audio_des_s {
	struct rdr_register_module_result audio_info;

	char soc_pathname[RDR_FNAME_LEN];
	char codec_pathname[RDR_FNAME_LEN];
};
static struct rdr_audio_des_s s_rdr_audio_des;

static int create_dir(char *path)
{
	long fd;

	BUG_ON(NULL == path);

	fd = sys_access(path, 0);
	if (fd) {
		BB_PRINT_PN("need create dir %s\n", path);
		fd = sys_mkdir(path, 0770);
		if (fd < 0) {
			BB_PRINT_ERR("create dir %s failed, fd:%ld\n", path, fd);
			return -EBADF;
		}
		BB_PRINT_PN("create dir %s successed, fd: %ld\n", path, fd);
	}

	/* change dir limit root-system */
	if (bbox_chown((const char *)path, ROOT_UID, SYSTEM_GID, false)) {/*lint !e747*/
		BB_PRINT_ERR("[%s], chown %s dir failed\n", __func__, path);
	}

	return 0;
}

static int rdr_audio_create_dir(const char *path)
{
	char cur_path[RDR_FNAME_LEN];
	int index = 0;

	memset(cur_path, 0, RDR_FNAME_LEN);
	if (*path != '/')
		return -ENOTDIR;

	cur_path[index++] = *path++;

	while (*path != '\0') {
		if (*path == '/') {
			if (create_dir(cur_path))
				return -ENOENT;
		}
		cur_path[index] = *path;
		path++;
		index++;
	}

	return 0;
}

static int rdr_audio_loopwrite_open(char *name, unsigned int *pfd)
{
	int ret;
	int fd;

	BUG_ON(NULL == pfd);
	BUG_ON(NULL == name);

	ret = rdr_audio_create_dir(name);
	if (ret) {
		BB_PRINT_ERR("create dir fail, name: %s, ret: %d\n", name, ret);
		return ret;
	}

	/* sys_access() return 0:success, return ~0:error */
	if (!(sys_access(name, 0))) {
		ret = (int)sys_unlink(name);
		if (ret) {
			BB_PRINT_ERR("remove file fail, name: %s, ret: %d\n", name, ret);
			return ret;
		}
	}

	fd = (int)sys_open(name, O_CREAT | O_RDWR, 0660);
	if (fd < 0) {
		BB_PRINT_ERR("create and open file fail, name: %s, fd: %d\n", name, fd);
		return -EBADF;
	}

	*pfd = (unsigned int)fd;

	return ret;
}

static int rdr_audio_loopwrite_append(unsigned int fd, void *address, u32 length)
{
	long bytes;

	BUG_ON(NULL == address);
	BUG_ON(0 == length);

	bytes = sys_write(fd, address, (unsigned long)length);
	if (bytes != length) {
		BB_PRINT_ERR("write data failed, ret: %ld\n", bytes);
		return -EINVAL;
	}

	return 0;
}

static void rdr_audio_loopwrite_close(unsigned int fd)
{
	long ret;

	ret = sys_fsync(fd);
	if (ret < 0)
		pr_err("[%s]sys_fsync failed, ret is %ld\n", __func__, ret);

	ret = sys_close(fd);
	if (ret)
		BB_PRINT_ERR("close file failed, ret: %ld\n", ret);

	return;
}

int rdr_audio_write_file(char *name, char *data, u32 size)
{
	int ret;
	unsigned int fd = 0;
	mm_segment_t oldfs;

	BUG_ON(NULL == name);
	BUG_ON(NULL == data);
	BUG_ON(0 == size);

	oldfs = get_fs();
	set_fs(KERNEL_DS);/*lint !e501*/

	ret = rdr_audio_loopwrite_open(name, &fd);
	if (ret) {
		BB_PRINT_ERR("open %s fail\n", name);
		set_fs(oldfs);
		return ret;
	}

	ret = rdr_audio_loopwrite_append(fd, data, size);
	if (ret)
		BB_PRINT_ERR("write %s fail\n", name);

	/* change file limit root-system */
	if (bbox_chown((const char *)name, ROOT_UID, SYSTEM_GID, false)) {/*lint !e747*/
		BB_PRINT_ERR("[%s], chown %s file failed\n", __func__, name);
	}

	rdr_audio_loopwrite_close(fd);

	set_fs(oldfs);

	return ret;
}

void rdr_audio_dump_log(u32 modid, u32 etype, u64 coreid,
				char *pathname, pfn_cb_dump_done pfn_cb)
{
	if (!pathname) {
		BB_PRINT_ERR("input path name is NULL\n");
		return;
	}

	if (!pfn_cb) {
		BB_PRINT_ERR("input dump done cb is NULL\n");
		return;
	}

	BB_PRINT_DBG(" ====================================\n");
	BB_PRINT_DBG(" modid:          [0x%x]\n",   modid);
	BB_PRINT_DBG(" coreid:         [0x%llx]\n", coreid);
	BB_PRINT_DBG(" exce tpye:      [0x%x]\n",   etype);
	BB_PRINT_DBG(" path name:      [%s]\n",     pathname);
	BB_PRINT_DBG(" dump start:     [0x%pK]\n", (void *)s_rdr_audio_des.audio_info.log_addr);
	BB_PRINT_DBG(" dump len:       [%d]\n",     s_rdr_audio_des.audio_info.log_len);
	BB_PRINT_DBG(" nve:            [0x%llx]\n", s_rdr_audio_des.audio_info.nve);
	BB_PRINT_DBG(" callback:       [0x%pK]\n",   pfn_cb);
	BB_PRINT_DBG(" ====================================\n");

	if (((modid >= (unsigned int)RDR_AUDIO_SOC_MODID_START) && (modid <= RDR_AUDIO_SOC_MODID_END))
		|| ((modid >= RDR_AUDIO_REBOOT_MODID_START) && (modid <= RDR_AUDIO_REBOOT_MODID_END))) {
		snprintf(s_rdr_audio_des.soc_pathname, RDR_FNAME_LEN, "%s", pathname);
		rdr_audio_soc_dump(modid, s_rdr_audio_des.soc_pathname, pfn_cb);
	} else if ((modid >= RDR_AUDIO_CODEC_MODID_START) && (modid <= RDR_AUDIO_CODEC_MODID_END)) {
		snprintf(s_rdr_audio_des.codec_pathname, RDR_FNAME_LEN, "%s", pathname);
		rdr_audio_codec_dump(modid, s_rdr_audio_des.codec_pathname, pfn_cb);
	} else if (modid >= (unsigned int)HISI_BB_MOD_CP_START && modid <= (unsigned int)HISI_BB_MOD_CP_END) {
		snprintf(s_rdr_audio_des.soc_pathname, RDR_FNAME_LEN, "%s", pathname);
		BB_PRINT_PN("modem reset soc hifi dump = %s, begin\n", s_rdr_audio_des.soc_pathname);
		rdr_audio_soc_dump(modid, s_rdr_audio_des.soc_pathname, pfn_cb);
		BB_PRINT_PN("modem reset soc hifi dump = %s, end\n", s_rdr_audio_des.soc_pathname);
	} else {
		BB_PRINT_ERR("mod id is invalide: 0x%x[soc:0x%x - 0x%x, codec: 0x%x - 0x%x]\n"
						, modid
						, RDR_AUDIO_SOC_MODID_START, RDR_AUDIO_SOC_MODID_END
						, RDR_AUDIO_CODEC_MODID_START, RDR_AUDIO_CODEC_MODID_END);
	}

	return;
}

void rdr_audio_reset(u32 modid, u32 etype, u64 coreid)
{
	BB_PRINT_DBG(" ====================================\n");
	BB_PRINT_DBG(" modid:		   [0x%x]\n",	modid);
	BB_PRINT_DBG(" coreid:		   [0x%llx]\n", coreid);
	BB_PRINT_DBG(" exce tpye:	   [0x%x]\n",	etype);
	BB_PRINT_DBG(" ====================================\n");

	if (modid >= (unsigned int)RDR_AUDIO_SOC_MODID_START && modid <= RDR_AUDIO_SOC_MODID_END) {
		rdr_audio_soc_reset(modid, etype, coreid);
	} else if (modid >= RDR_AUDIO_CODEC_MODID_START && modid <= RDR_AUDIO_CODEC_MODID_END) {
		rdr_audio_codec_reset(modid, etype, coreid);
	} else if (modid >= RDR_AUDIO_REBOOT_MODID_START && modid <= RDR_AUDIO_REBOOT_MODID_END){
		/* system will be reboot, do nothing here */
		BB_PRINT_ERR("audio NOC exception, system will be reboot\n");
	} else {
		BB_PRINT_ERR("mod id is invalide: 0x%x[soc:0x%x - 0x%x, codec: 0x%x - 0x%x]\n"
						, modid
						, RDR_AUDIO_SOC_MODID_START, RDR_AUDIO_SOC_MODID_END
						, RDR_AUDIO_CODEC_MODID_START, RDR_AUDIO_CODEC_MODID_END);
	}

	return;
}

static int rdr_audio_register_core(void)
{
	int ret;
	struct rdr_module_ops_pub module_ops;

	BB_PRINT_START();

	module_ops.ops_dump = rdr_audio_dump_log;
	module_ops.ops_reset = rdr_audio_reset;

	/* <0 error, >=0 success */
	ret = rdr_register_module_ops((u64)RDR_HIFI, &module_ops, &s_rdr_audio_des.audio_info);
	if (ret < 0) {
		BB_PRINT_ERR("rdr register hifi module ops error\n");
		ret = -EBUSY;
	} else {
		ret = 0;
	}

	BB_PRINT_END();

	return ret;
}

static int rdr_audio_register_exception(void)
{
	int ret = 0;
	struct rdr_exception_info_s einfo;

	BB_PRINT_START();

	/* ===sochifi exception register begin=== */
	memset(&einfo, 0, sizeof(struct rdr_exception_info_s));
	einfo.e_modid = (unsigned int)RDR_AUDIO_SOC_MODID_START;
	einfo.e_modid_end = RDR_AUDIO_SOC_MODID_END;
	einfo.e_process_priority = RDR_WARN;
	einfo.e_reboot_priority = RDR_REBOOT_NO;

	/* 标志位e_notify_core_mask中填0, RDR框架中会判断是否为0, 是0时不启动RDR框架中的导出LOG操作, 确认人:刘海龙 */
	einfo.e_notify_core_mask = RDR_HIFI;
	einfo.e_reset_core_mask = RDR_HIFI;
	einfo.e_reentrant = (unsigned int)RDR_REENTRANT_DISALLOW;
	einfo.e_exce_type = SOCHIFI_S_EXCEPTION;
	einfo.e_upload_flag = (unsigned int)RDR_UPLOAD_YES;
	einfo.e_from_core = RDR_HIFI;
	memcpy(einfo.e_from_module, "RDR_SOCHIFI_WATCHDOG", sizeof("RDR_SOCHIFI_WATCHDOG"));
	memcpy(einfo.e_desc, "RDR_SOCHIFI watchdog timeout.",
			sizeof("RDR_SOCHIFI watchdog timeout."));

	/* error return 0, ok return modid */
	if (!rdr_register_exception(&einfo)) {
		BB_PRINT_ERR("regist audio soc exception fail\n");
		ret = -EBUSY;
	} else {
		ret = 0;
	}
	/* ===sochifi exception register end=== */

	/* ===codechifi exception register begin=== */
	memset(&einfo, 0, sizeof(struct rdr_exception_info_s));
	einfo.e_modid = RDR_AUDIO_CODEC_MODID_START;
	einfo.e_modid_end = RDR_AUDIO_CODEC_MODID_END;
	einfo.e_process_priority = RDR_WARN;
	einfo.e_reboot_priority = RDR_REBOOT_NO;
	einfo.e_notify_core_mask = RDR_HIFI;
	einfo.e_reset_core_mask = RDR_HIFI;
	einfo.e_reentrant = (unsigned int)RDR_REENTRANT_DISALLOW;
	einfo.e_exce_type = CODECHIFI_S_EXCEPTION;
	einfo.e_upload_flag = (unsigned int)RDR_UPLOAD_YES;
	einfo.e_from_core = RDR_HIFI;
	memcpy(einfo.e_from_module, "RDR_CODECHIFI_WATCHDOG", sizeof("RDR_CODECHIFI_WATCHDOG"));
	memcpy(einfo.e_desc, "RDR_CODECHIFI watchdog timeout.",
			sizeof("RDR_CODECHIFI watchdog timeout."));

	/* error return 0, ok return modid */
	if (!rdr_register_exception(&einfo)) {
		BB_PRINT_ERR("register audio codec exception fail\n");
		ret = -EBUSY;
	} else {
		ret = 0;
	}
	/* ===codecshifi exception register end=== */

	/* ===shoule reboot exception register begin=== */
	memset(&einfo, 0, sizeof(struct rdr_exception_info_s));
	einfo.e_modid = RDR_AUDIO_REBOOT_MODID_START;
	einfo.e_modid_end = RDR_AUDIO_REBOOT_MODID_END;
	einfo.e_process_priority = RDR_WARN;
	einfo.e_reboot_priority = RDR_REBOOT_NOW;
	einfo.e_notify_core_mask = RDR_HIFI;
	einfo.e_reset_core_mask = RDR_AP;
	einfo.e_reentrant = (unsigned int)RDR_REENTRANT_DISALLOW;
	einfo.e_exce_type = SOCHIFI_S_EXCEPTION;
	einfo.e_upload_flag = (unsigned int)RDR_UPLOAD_YES;
	einfo.e_from_core = RDR_HIFI;
	memcpy(einfo.e_from_module, "AUDIO_NOC_EXCEPTION", sizeof("AUDIO_NOC_EXCEPTION"));
	memcpy(einfo.e_desc, "NOC error due to audio", sizeof("NOC error due to audio"));

	/* error return 0, ok return modid */
	if (!rdr_register_exception(&einfo)) {
		BB_PRINT_ERR("register audio codec exception fail\n");
		ret = -EBUSY;
	} else {
		ret = 0;
	}
	/* ===shoule reboot exception register end=== */

	BB_PRINT_END();

	return ret;
}

static int rdr_audio_init_early(void)
{
	int ret;

	ret = rdr_audio_register_exception();
	if (ret) {
		BB_PRINT_ERR("rdr_hifi_register_exception fail\n");
		return ret;
	}

	ret = rdr_audio_register_core();
	if (ret) {
		BB_PRINT_ERR("rdr_hifi_register_core fail\n");
		return ret;
	}

	return ret;
}

static int __init rdr_audio_init(void)
{
	int ret;

	BB_PRINT_START();

	ret = rdr_audio_init_early();
	if (ret) {
		BB_PRINT_ERR("%s():init early fail\n", __func__);
		BB_PRINT_END();
		return ret;
	}

	memset(&s_rdr_audio_des.audio_info, 0, sizeof(struct rdr_register_module_result));
	memset(s_rdr_audio_des.soc_pathname, 0, RDR_FNAME_LEN);
	memset(s_rdr_audio_des.codec_pathname, 0, RDR_FNAME_LEN);

	ret = rdr_audio_soc_init();
	if (ret)
		BB_PRINT_ERR("init rdr soc hifi fail\n");

	ret = rdr_audio_codec_init();
	if (ret)
		BB_PRINT_ERR("init rdr codec hifi fail\n");

	BB_PRINT_END();

	return ret;
}

static void __exit rdr_audio_exit(void)
{
	rdr_audio_soc_exit();
	rdr_audio_codec_exit();
	return;
}

/*lint -e528 -e753*/
module_init(rdr_audio_init);
module_exit(rdr_audio_exit);

MODULE_LICENSE("GPL");


