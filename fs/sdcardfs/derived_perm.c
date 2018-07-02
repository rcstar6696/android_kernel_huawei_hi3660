/* vim:set ts=4 sw=4 tw=0 noet ft=c:
 *
 * fs/sdcardfs/derived_perm.c
 *
 * Copyright (C) 2017 HUAWEI, Inc.
 * Author: gaoxiang <gaoxiang25@huawei.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of the Linux
 * distribution for more details.
 */
#include "sdcardfs.h"

const struct cred *prepare_fsids(struct sdcardfs_sb_info *sbi)
{
	struct cred *cred = prepare_creds();

	if (cred == NULL)
		return NULL;
	cred->fsuid = make_kuid(&init_user_ns, sbi->options.fs_low_uid);
	cred->fsgid = make_kgid(&init_user_ns, sbi->options.fs_low_gid);
#ifdef CONFIG_SECURITY
	/* if necessary, override security field */
	if (unlikely(sbi->options.has_fssecid)) {
		const struct cred *saved_cred = override_creds(sbi->sdcardd_cred);
		if (likely(saved_cred != NULL)) {
			int ret = set_security_override(cred, sbi->options.fs_low_secid);
			if (unlikely(ret < 0)) {
				critln("Security denies permission to nominate"
					" security context: %d", ret);
				sbi->options.has_fssecid = false;
			}
			revert_creds(saved_cred);
		}
	}
#endif
	return cred;
}

/* Do not directly use this function. Use OVERRIDE_CRED() instead. */
const struct cred *override_fsids(struct sdcardfs_sb_info *sbi) {
	const struct cred *cred = prepare_fsids(sbi);

	if (cred == NULL)
		return ERR_PTR(-ENOMEM);
	return override_creds(cred);
}

/* Do not directly use this function, use REVERT_CRED() instead. */
void revert_fsids(const struct cred *old_cred)
{
	const struct cred *cur_cred = current->cred;

	revert_creds(old_cred);
	put_cred(cur_cred);
}

struct fs_struct *prepare_fs_struct(
	struct path *root,
	int umask
) {
	struct fs_struct *fs;

	fs = kzalloc(sizeof(struct fs_struct), GFP_KERNEL);
	if (fs == NULL)
		return NULL;

	/* no need to lock fs before committing to task_struct */
	fs->users = 1;
	fs->in_exec = 0;
	spin_lock_init(&fs->lock);
	seqcount_init(&fs->seq);
	fs->umask = umask;
	fs->root = *root;
	path_get(&fs->root);
	fs->pwd = *root;
	path_get(&fs->pwd);
	return fs;
}

/* Kernel has already enforced everything we returned through
 * derive_permissions_locked(), so this is used to lock down access
 * even further, such as enforcing that apps hold sdcard_rw. */
int check_caller_access_to_name(
	struct dentry *parent,
	const char *name
) {
	int err = 1;
	struct sdcardfs_tree_entry *pi = SDCARDFS_DI_R(parent);
	/* Always block security-sensitive files at root */
	if (pi->perm == PERM_ROOT) {
		if (!strcasecmp(name, "autorun.inf")
			|| !strcasecmp(name, ".android_secure")
			|| !strcasecmp(name, "android_secure")) {
			err = 0;
		}
	}
	read_unlock(&pi->lock);
	return err;
}

/* copy derived state from parent inode */
static inline void
__inherit_derived_state(
	struct sdcardfs_tree_entry *pi,	/* parent tree entry */
	struct sdcardfs_tree_entry *ci		/* tree entry that we want to inherit */
) {
	ci->revision = pi->revision;

	ci->perm = PERM_INHERIT;
	ci->userid = pi->userid;
	ci->d_uid = pi->d_uid;
	ci->under_android = pi->under_android;
}

void __get_derived_permission(struct super_block *sb, const char *name,
	struct sdcardfs_tree_entry *pi, struct sdcardfs_tree_entry *ci)
{
	unsigned userid;
	appid_t appid;

	/* By default, each node inherits from its parent */
	__inherit_derived_state(pi, ci);

	/* Derive custom permissions based on parent and current node */
	switch (pi->perm) {
		case PERM_INHERIT:
			/* Already inherited above */
			break;

		case PERM_PRE_ROOT:
			/* Legacy internal layout places users at top level */
			ci->perm = PERM_ROOT;
			if (!kstrtouint(name, 10, &userid))
				ci->userid = userid;
			break;

		case PERM_ROOT:
			/* Assume masked off by default. */
			if (!strcasecmp(name, "Android")) {
				/* App-specific directories inside; let anyone traverse */
				ci->perm = PERM_ANDROID;
				ci->under_android = true;
			}
			break;

		case PERM_ANDROID:
			if (!strcasecmp(name, "data")) {
				/* App-specific directories inside; let anyone traverse */
				ci->perm = PERM_ANDROID_DATA;
			} else if (!strcasecmp(name, "obb")) {
				/* App-specific directories inside; let anyone traverse */
				ci->perm = PERM_ANDROID_OBB;

				/* Single OBB directory is always shared */
				/* if shared_obb != NULL, Single OBB directory is available */
				ci->ovl = SDCARDFS_SB(sb)->shared_obb;
			} else if (!strcasecmp(name, "media")) {
				/* App-specific directories inside; let anyone traverse */
				ci->perm = PERM_ANDROID_MEDIA;
			}
			break;

		case PERM_ANDROID_DATA:
		case PERM_ANDROID_OBB:
		case PERM_ANDROID_MEDIA:
			appid = get_appid(name);
			if (appid < AID_SYSTEM)
				ci->revision = 0;
			else
				ci->d_uid = multiuser_get_uid(pi->userid, appid);
			break;
	}
}

void get_derived_permission4(struct dentry *parent,
	struct dentry *dentry,
	const char *rename, bool lazy_recursive)
{
	struct sdcardfs_tree_entry *pi = SDCARDFS_DI_R(parent);
	struct sdcardfs_tree_entry *ci = SDCARDFS_DI_W(dentry);

	__get_derived_permission(dentry->d_sb, rename, pi, ci);
	if (lazy_recursive)
		ci->revision = get_next_revision(dentry);
	write_unlock(&ci->lock);
	read_unlock(&pi->lock);
}

void get_derived_permission(struct dentry *parent, struct dentry *dentry)
{
	get_derived_permission4(parent, dentry,
		dentry->d_name.name, false);
}
