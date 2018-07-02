/*
 * patch_mgr.c - patch manager
 *
 * Copyright (C) 2016 Baidu, Inc. All Rights Reserved.
 *
 * You should have received a copy of license along with this program;
 * if not, ask for it from Baidu, Inc.
 *
 */

#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>
#include <linux/mm_types.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/kobject.h>

#include "patch_mgr.h"
#include "util.h"
#include "patch_file.h"
#include "patch_info.h"
#include "patch_base.h"
#include "sysfs.h"
#include "plts.h"

#define FUNC_NAME_LEN_MAX 255
#define UEVENT_INFO_LEN_MAX 128
LIST_HEAD(patchinfo_list);

extern struct kobject *attack_kobj;
extern struct kset *attack_kset;
extern int attack_upload_init;

static struct uevent_info {
	const char *uidinfo;
	const char *patchinfo;
};

enum {
    UID = 0,
	PATCHID,
	UEVENT_NULL,
	UEVENT_LAST,
};

int oases_check_patch_func(const char *name)
{
	struct oases_patch_info *ctx, *c;
	struct oases_patch_entry *patch, *p;

	/* oases_mutex held */
	list_for_each_entry_safe(ctx, c, &patchinfo_list, list) {
		list_for_each_entry_safe(patch, p, &ctx->patches, list) {
			if (!strncmp(name, kp_vtab(patch)->get_name(patch), FUNC_NAME_LEN_MAX - 1)) {
				oases_error("duplicate name: %s\n", name);
				return -EEXIST;
			}
		}
	}
	return 0;
}

int oases_check_patch(const char *id)
{
	struct oases_patch_info *pos;
	struct oases_patch_info *n;

	/* oases_mutex held */
	list_for_each_entry_safe(pos, n, &patchinfo_list, list) {
		if (strncmp(pos->id, id, PATCH_ID_LEN - 1) == 0) {
			oases_error("patch exist: %s\n", id);
			return -EEXIST;
		}
	}
	return 0;
}

void oases_free_patch(struct oases_patch_info *info)
{
	struct oases_patch_entry *patch, *p;

	if (!info) {
		return;
	}
	list_for_each_entry_safe(patch, p, &info->patches, list) {
		kp_vtab(patch)->destroy(patch);
		list_del(&patch->list);
		kfree(patch->data);
		kfree(patch);
	}
	oases_patch_addr_free(&info->addresses);
	if (info->code_base) {
		vfree(info->code_base);
	}
	if (info->plog)
		kfree(info->plog);
	kfree(info);
}

int oases_op_patch(struct oases_patch_file *pfile)
{
	int ret;
	struct oases_attack_log *plog;
	struct oases_patch_info *info = NULL;
	int (*code_entry)(struct oases_patch_info *info) = NULL;

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		oases_error("kzalloc info fail\n");
		return -ENOMEM;
	}

	strncpy(info->id, pfile->pheader->id, PATCH_ID_LEN - 1);
	info->version = pfile->pheader->patch_version;
	info->status = STATUS_DISABLED;
	INIT_LIST_HEAD(&info->patches);

	plog = kzalloc(sizeof(struct oases_attack_log) * OASES_LOG_NODE_MAX, GFP_KERNEL);
	if (!plog) {
		oases_error("kzalloc log fail\n");
		ret = -ENOMEM;
		goto fail_alloc_log;;
	}
	plog->uid = INVALID_UID;
	info->log_index = 1;
	info->plog = plog;
	spin_lock_init(&info->log_lock);

	ret = oases_build_code(info, pfile);
	if (ret < 0) {
		oases_error("oases_build_code fail\n");
		goto fail_build_code;
	}

	code_entry = info->code_entry;
	ret = (*code_entry)(info);
	oases_debug("code_entry ret=%d\n", ret);
	if (ret != OASES_PATCH_SUCCESS) {
		if (ret > 0) {
			/* OASES_PATCH_FAILURE here */
			ret = -EINVAL;
		}
		goto fail_entry;
	}

	oases_sysfs_init_patch(info);
	ret = oases_sysfs_add_patch(info);
	if (ret < 0) {
		oases_error("oases_sysfs_add_patch fail\n");
		goto fail_sysfs;
	}

	info->attached = 1;
	list_add_tail(&info->list, &patchinfo_list);
	return 0;

fail_sysfs:
	oases_sysfs_del_patch(info);
	return ret;
fail_entry:
	vfree(info->code_base);
fail_build_code:
	kfree(plog);
fail_alloc_log:
	kfree(info);
	return ret;
}

static int op_unpatch(struct oases_patch_info *info)
{
	int ret;

	ret = oases_remove_patch(info);
	if (ret)
		return ret;
	if (info->attached) {
		list_del(&info->list);
		info->attached = 0;
	}
	oases_sysfs_del_patch(info);
	return 0;
}

int oases_op_unpatch(struct oases_unpatch *p)
{
	int ret;
	struct oases_patch_info *pos;
	struct oases_patch_info *n;
	struct oases_unpatch oup;

	memset(&oup, 0, sizeof(struct oases_unpatch));
	ret = copy_from_user(&oup, p, sizeof(*p));
	if (ret)
		return -EFAULT;

	oup.id[PATCH_ID_LEN -1] = '\0';
	list_for_each_entry_safe(pos, n, &patchinfo_list, list) {
		if (strncmp(oup.id, pos->id, PATCH_ID_LEN - 1) == 0) {
			if (pos->status == STATUS_ENABLED) {
				oases_error("error: patch enabled %s\n", pos->id);
				return -EINVAL;
			}
			return op_unpatch(pos);
		}
	}
	oases_error("warning: no patch: %s\n", oup.id);
	return -ENOENT;
}

static int attack_data_adapter(char **uevent_envp, kuid_t uid, char *id)
{
	int index = 0;
	int i;
	size_t attack_item_len = 0;

	struct uevent_info attack_info = {
		"uid=",
		"patch_id=",
	};

	if ((NULL == uevent_envp) || (NULL == id)) {
		return -EINVAL;
	}
	if ((ATKSYS_INIT != attack_upload_init) || (NULL == attack_kobj))
	{
	    return -EINVAL;
	}

	for (index = 0; index < UEVENT_NULL; index++) {
		uevent_envp[index] = vmalloc(UEVENT_INFO_LEN_MAX);
		if (NULL == uevent_envp[index]) {
			for (i = index; i >= 0; i--) {
				vfree(uevent_envp[i]);
				uevent_envp[i] = NULL;
			}
			return -EINVAL;
		}
		memset(uevent_envp[index], 0, UEVENT_INFO_LEN_MAX);
	}

	snprintf(uevent_envp[UID], UEVENT_INFO_LEN_MAX,"%s%d", attack_info.uidinfo, uid);
	strncpy(uevent_envp[PATCHID], attack_info.patchinfo, strlen(attack_info.patchinfo));
	strncat(uevent_envp[PATCHID], id, strlen(id));

	uevent_envp[UEVENT_NULL] = NULL;
	kobject_uevent_env(attack_kobj, KOBJ_CHANGE, uevent_envp);
	for (index = 0; index < UEVENT_NULL; index++) {
		vfree(uevent_envp[index]);
		uevent_envp[index] = NULL;
	}
	return 0;
}

static void increase_attack_count(struct oases_patch_info *ctx, kuid_t uid)
{
	int i, ret;
	struct oases_attack_log *entry;
	char *uevent_envp[UEVENT_LAST] = {NULL};

	/* node 0 belongs to INVALID_UID */
	if (uid_eq(uid, INVALID_UID)) {
		entry = ctx->plog;
		entry->count++;
		entry->end_time = get_seconds();
		if (!entry->start_time)
			entry->start_time = entry->end_time;
		return;
	}

	for (i = 1; i < ctx->log_index; i++) {
		entry = ctx->plog + i;
		if (uid_eq(entry->uid, uid)) {
			entry->count++;
			entry->end_time = get_seconds();
			return;
		}
	}

	/* new uid */
	entry = ctx->plog + ctx->log_index;
	entry->uid = uid;
	entry->count = 1;
	entry->start_time = get_seconds();
	entry->end_time = entry->start_time;
	ctx->log_index++;

	ret = attack_data_adapter(uevent_envp, uid, ctx->id);
	if (ret != 0)
	{
	    oases_error("uevent of attack failed\n");
	}
}

void oases_attack_logger(struct oases_patch_info *ctx)
{
	kuid_t uid = INVALID_UID;
	unsigned long flags;

	spin_lock_irqsave(&ctx->log_lock, flags);
	if (ctx->log_index >= OASES_LOG_NODE_MAX)
		goto index_zero;
	uid = current_uid();

index_zero:
	increase_attack_count(ctx, uid);
	spin_unlock_irqrestore(&ctx->log_lock, flags);
}
