/*
 * hook ops for sub function
 *
 * Copyright (C) 2016 Baidu, Inc. All Rights Reserved.
 *
 * You should have received a copy of license along with this program;
 * if not, ask for it from Baidu, Inc.
 *
 */

#include <linux/types.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/kallsyms.h>

#if IS_ENABLED(CONFIG_MODULES)
#include <linux/module.h>
#endif

#include "hook_subfunc.h"
#include "util.h"
#include "hook_insn.h"
#include "patch_api.h"
#include "inlinehook.h"
#include "inlinehook_offset.h"
#include "patch_info.h"
#include "patch_mgr.h"
#include "kallsyms.h"

static const char *subfunc_get_name(struct oases_patch_entry *patch)
{
	return kp_subfunc(patch)->parent;
}

static int subfunc_get_count(struct oases_patch_entry *patch)
{
    return kp_subfunc(patch)->count;
}

static int subfunc_show_info(struct oases_patch_entry *patch,
	struct kobject *kobj, struct kobj_attribute *attr, char *buf, int off)
{
	return scnprintf(buf + off, PAGE_SIZE - off - 1,
		"%s %s\n", kp_subfunc(patch)->pmod ?: "vmlinux", kp_subfunc(patch)->parent);
}

static int subfunc_with_each_insn(struct oases_patch_entry *patch,
    int (*callback)(struct oases_patch_entry *patch, struct oases_insn *insn, void *data),
	void *data)
{
    struct oases_subfunc *kp = kp_subfunc(patch);
    int ret, i;
    for (i = 0; i < kp->count; i++) {
        ret = callback(patch, &kp->insn[i], data);
        if (ret)
            return ret;
    }
    return 0;
}

static int subfunc_check_user(struct oases_subfunc *kp,
	struct patch_subfunc *up)
{
	if (!valid_patch_pointer(pb_owner(kp), up))
		return -EINVAL;
	if (!valid_patch_pointer(pb_owner(kp), up->parent))
		return -EINVAL;
	if (up->pmod && !valid_patch_pointer(pb_owner(kp), up->pmod))
		return -EINVAL;
	if (!valid_patch_pointer(pb_owner(kp), up->child))
		return -EINVAL;
	if (up->cmod && !valid_patch_pointer(pb_owner(kp), up->cmod))
		return -EINVAL;
	if (!valid_patch_pointer(pb_owner(kp), up->filter))
		return -EINVAL;
	return oases_check_patch_func(up->parent);
}

static int subfunc_copy_from_user(struct oases_subfunc *kp,
			struct patch_subfunc *up, int flags)
{
	int ret, i;
	struct oases_find_symbol args;
	unsigned long off, len, mask;
	u32 *ps, *pe, *pi;

#ifdef CONFIG_MODULES
	if (up->pmod) {
		kp->module = oases_ref_module(up->pmod);
		if (kp->module == NULL) {
			oases_error("could not find module %s\n", up->pmod);
			return -EINVAL;
		}
	}
#endif

	kp->parent = up->parent;
	kp->pmod = up->pmod;
	kp->mask = up->mask;

	memset(&args, 0, sizeof(struct oases_find_symbol));
	args.mod = up->pmod;
	args.name = up->parent;
	args.callback = NULL;
	if (oases_lookup_name_internal(&args) < 0) {
		oases_error("could not find %s, %lu\n", args.name, args.count);
		return -EINVAL;
	}

	kp->start = args.addr;
	kp->module = args.module;
	ret = kallsyms_lookup_size_offset((unsigned long) kp->start, &len, &off);
	if (!ret || off || !len) {
		oases_error("could not size %s, %d, %lu, %lu\n", kp->parent, ret, off, len);
		return -EINVAL;
	}

	kp->end = kp->start + len;
	kp->child = up->child;
	kp->cmod = up->cmod;

	memset(&args, 0, sizeof(struct oases_find_symbol));
	args.mod = up->cmod;
	args.name = up->child;
	args.callback = NULL;
	if (oases_lookup_name_internal(&args) < 0) {
		oases_error("could not find %s, %lu\n", args.name, args.count);
		return -EINVAL;
	}

	kp->sub = args.addr;
	ps = (u32 *) kp->start;
	pe = (u32 *) kp->end;

	mask = up->mask;
	oases_debug("up->mask: %ld\n", mask);

	i = 0;
	for (pi = ps; pi < pe; pi++) {
		if (is_insn_bl(*pi) && is_bl_to(pi, kp->sub)) {
			if (i++ > OASES_SUBFUNC_MASK_MAX - 1) {
				oases_error("OASES_SUBFUNC_MASK_MAX=%d reached\n", OASES_SUBFUNC_MASK_MAX);
				return -ENOMEM;
			}

			if (!up->mask) {
				kp->count += 1;
			} else {
				if (mask & 1)
					kp->count += 1;
				if (!(mask >>= 1))
					break;
			}
		}
	}

	oases_debug("kp->count == %d\n", kp->count);
	if (kp->count == 0)
		return -EINVAL;

	oases_debug("mask == %ld\n", mask);
	if (up->mask && mask)
		return -EINVAL;

	ret = oases_insn_alloc_nr(&kp->insn, kp->module, kp->count, flags);
	if (ret)
		return ret;

	i = 0;
	mask = up->mask;
	for (pi = ps; pi < pe; pi++) {
		if (is_insn_bl(*pi) && is_bl_to(pi, kp->sub)) {
			if (!up->mask) {
				kp->insn[i].address = pi;
				kp->insn[i].origin_to = kp->sub;
				kp->insn[i].handler = up->filter;
				i++;
			} else {
				if (mask & 1) {
					kp->insn[i].address = pi;
					kp->insn[i].origin_to = kp->sub;
					kp->insn[i].handler = up->filter;
					i++;
				}
				if (!(mask >>= 1))
					break;
			}
		}
	}

	return 0;
}

static int subfunc_create(struct oases_patch_entry *patch, void *user, int flags)
{
    int ret;
	struct oases_subfunc *kc = kp_subfunc(patch);
    struct patch_subfunc *uc = user;

    /* check and copy user params */
	ret = subfunc_check_user(kc, uc);
	if (ret)
		return ret;
	/* validate modules and function names */
	ret = subfunc_copy_from_user(kc, uc, flags);
	if (ret)
		return ret;

    return 0;
}

static void subfunc_destroy(struct oases_patch_entry *patch)
{
	struct oases_subfunc *kc = kp_subfunc(patch);
	int i;

	for (i = 0; i < kc->count; i++)
    	oases_insn_free(&kc->insn[i], kc->module);
	if (kc->insn)
		kfree(kc->insn);
	kc->insn = NULL;
	kc->count = 0;
#if IS_ENABLED(CONFIG_MODULES)
    if (kc->module) {
        oases_unref_module(kc->module);
		kc->module = NULL;
	}
#endif
}

static u32 subfunc_setup_jump(struct oases_patch_entry *patch, struct oases_insn *insn)
{
	int ret;
	u32 x;
	ret = oases_make_jump_insn(insn->address, insn->plt, &x);
	return ret ? 0 : (x | 0x80000000);
}

/* patch_subfunc_pre functions */
static int subfunc_pre_create(struct oases_patch_entry *patch, void *user)
{
    return subfunc_create(patch, user, 0);
}

extern char oases_handler_subfunc_pre_start, oases_handler_subfunc_pre_end;

static int subfunc_pre_setup_trampoline(struct oases_patch_entry *patch, struct oases_insn *insn)
{
	void *trampoline = insn->trampoline;

	memcpy(trampoline, &oases_handler_subfunc_pre_start,
		&oases_handler_subfunc_pre_end - &oases_handler_subfunc_pre_start);
	/* filter_addr */
	*((u64 *)(trampoline + SUBFUNC_PRE_FILTER_ADDR_OFFSET)) = (u64)(insn->handler);
	/* origin_addr */
	*((u64 *)(trampoline + SUBFUNC_PRE_ORIGIN_ADDR_OFFSET)) = (u64)(insn->origin_to);
	/* retn_addr */
	*((u64 *)(trampoline + SUBFUNC_PRE_RETURN_ADDR_OFFSET)) = (u64)(insn->address + 4);
	/* attack logger */
	*((u64 *)(trampoline + SUBFUNC_PRE_PATCH_INFO_CTX_OFFSET)) = (u64)(kp_owner(patch));
	*((u64 *)(trampoline + SUBFUNC_PRE_ATTACK_LOGGER_OFFSET)) = (u64)(oases_attack_logger);
	return 0;
}

const struct oases_patch_desc oases_subfunc_pre_ops = {
	.type = OASES_SUBFUNC_PRE,
	.size = sizeof(struct oases_subfunc_pre),
	.usize = sizeof(struct patch_subfunc_pre),
	.get_name = subfunc_get_name,
	.get_count = subfunc_get_count,
	.show_info = subfunc_show_info,
    .with_each_insn = subfunc_with_each_insn,
    .create = subfunc_pre_create,
    .destroy = subfunc_destroy,
    .setup_jump = subfunc_setup_jump,
	.setup_trampoline = subfunc_pre_setup_trampoline,
};

/* subfunc_post functions */
static int subfunc_post_create(struct oases_patch_entry *patch, void *user)
{
	return subfunc_create(patch, user, 0);
}

extern char oases_handler_subfunc_post_start, oases_handler_subfunc_post_end;

static int subfunc_post_setup_trampoline(struct oases_patch_entry *patch, struct oases_insn *insn)
{
	struct oases_subfunc *kp = kp_subfunc(patch);
	void *trampoline = insn->trampoline;

	memcpy(trampoline, &oases_handler_subfunc_post_start,
		&oases_handler_subfunc_post_end - &oases_handler_subfunc_post_start);
	/* filter_addr */
	*((u64 *)(trampoline + SUBFUNC_POST_FILTER_ADDR_OFFSET)) = (u64)(insn->handler);
	/* orig_addr */
	*((u64 *)(trampoline + SUBFUNC_POST_ORIGIN_ADDR_OFFSET)) = (u64)(kp->sub);
	/* attack logger */
	*((u64 *)(trampoline + SUBFUNC_POST_PATCH_INFO_CTX_OFFSET)) = (u64)(kp_owner(patch));
	*((u64 *)(trampoline + SUBFUNC_POST_ATTACK_LOGGER_OFFSET)) = (u64)(oases_attack_logger);
	return 0;
}

const struct oases_patch_desc oases_subfunc_post_ops = {
	.type = OASES_SUBFUNC_POST,
	.size = sizeof(struct oases_subfunc_post),
	.usize = sizeof(struct patch_subfunc_post),
	.get_name = subfunc_get_name,
	.get_count = subfunc_get_count,
	.show_info = subfunc_show_info,
    .with_each_insn = subfunc_with_each_insn,
    .create = subfunc_post_create,
    .destroy = subfunc_destroy,
	.setup_jump = subfunc_setup_jump,
    .setup_trampoline = subfunc_post_setup_trampoline,
};

#if OASES_ENABLE_REPLACEMENT_HANDLER
static int subfunc_rep_create(struct oases_patch_entry *patch, void *user)
{
	return subfunc_create(patch, user, OASES_INSN_FLAG_NO_IC);
}

static int subfunc_rep_setup_trampoline(struct oases_patch_entry *patch, struct oases_insn *insn)
{
	return 0;
}

const struct oases_patch_desc oases_subfunc_rep_ops = {
	.type = OASES_SUBFUNC_REP,
	.size = sizeof(struct oases_subfunc_rep),
    .usize = sizeof(struct patch_subfunc_rep),
	.get_name = subfunc_get_name,
	.get_count = subfunc_get_count,
	.show_info = subfunc_show_info,
    .with_each_insn = subfunc_with_each_insn,
    .create = subfunc_rep_create,
    .destroy = subfunc_destroy,
	.setup_jump = subfunc_setup_jump,
    .setup_trampoline = subfunc_rep_setup_trampoline,
};
#endif
