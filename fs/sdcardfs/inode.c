/* vim:set ts=4 sw=4 tw=0 noet ft=c:
 *
 * fs/sdcardfs/inode.c
 *
 * Copyright (C) 2017 HUAWEI, Inc.
 * Author: gaoxiang <gaoxiang25@huawei.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of the Linux
 * distribution for more details.
 */
#include "sdcardfs.h"
#include <linux/fs_struct.h>
#include <linux/version.h>

#include "trace-events.h"

#include "create.c"

static int sdcardfs_create(struct inode *dir, struct dentry *dentry,
	umode_t mode, bool want_excl)
{
	_sdcardfs_do_create_struct this;
	int err;

	trace_sdcardfs_create_enter(dir, dentry, mode, want_excl);

	err = __sdcardfs_do_create_begin(&this, dir, dentry);
	if (err)
		goto out;

	/* for regular files, the last 16bit of mode is 0664 */
	mode = (mode & S_IFMT) | 0664;

	err = vfs_create(d_inode(this.real_dir_dentry),
		this.real_dentry, mode, want_excl);

	err = __sdcardfs_do_create_end(&this, __FUNCTION__, dir, dentry, err);
out:
	trace_sdcardfs_create_exit(dir, dentry, mode, want_excl, err);
	return err;
}

static struct dentry *dentry_creat(
	struct dentry *parent,
	char *name,
	int len
) {
	struct dentry *dentry;

	inode_lock(d_inode(parent));
	dentry = lookup_one_len(name, parent, len);
	if (unlikely(dentry == NULL))
		dentry = ERR_PTR(-ENOENT);
	else if (!IS_ERR(dentry)) {
		int err = (d_is_positive(dentry) ?
			 -EEXIST : vfs_create(d_inode(parent),
			 dentry, S_IFREG | 0664, 0));
		if (err) {
			dput(dentry);
			dentry = ERR_PTR(err);
		}
	}
	inode_unlock(d_inode(parent));
	return dentry;
}

int touch_nomedia(struct dentry *parent)
{
	int err;
	struct dentry *d_nomedia;

	d_nomedia = dentry_creat(parent, ".nomedia", sizeof(".nomedia")-1);
	if (IS_ERR(d_nomedia))
		err = PTR_ERR(d_nomedia);
	else {
		dput(d_nomedia);
		err = 0;
	}
	return err;
}

static int sdcardfs_mkdir(struct inode *dir,
	struct dentry *dentry, umode_t mode
) {
	_sdcardfs_do_create_struct this;
	int err;

	trace_sdcardfs_mkdir_enter(dir, dentry, mode);

#ifdef SDCARDFS_SUPPORT_RESERVED_SPACE
	if (!check_min_free_space(dir->i_sb, 0, 1)) {
		errln("%s, No minimum free space.", __FUNCTION__);
		err = -ENOSPC;
		goto out;
	}
#endif
	err = __sdcardfs_do_create_begin(&this, dir, dentry);
	if (err)
		goto out;

	/* for directories, the last 16bit of mode is 0775 */
	mode = (mode & S_IFMT) | 0775;

	err = vfs_mkdir(d_inode(this.real_dir_dentry),
		this.real_dentry, mode);

	/* When creating /Android/data and /Android/obb, mark them as .nomedia */
	if (!err && d_is_positive(this.real_dentry)) {
		struct sdcardfs_tree_entry *pte = SDCARDFS_D(this.parent);
		if (unlikely(pte->perm == PERM_ANDROID)) {
			if (unlikely(!strcasecmp(dentry->d_name.name, "data"))) {
touch_real:
				err = touch_nomedia(this.real_dentry);
				WARN_ON(err == -EEXIST);
			} else if (unlikely(!strcasecmp(dentry->d_name.name, "obb"))) {
				/* not multiuser obb */
				if (SDCARDFS_SB(dir->i_sb)->shared_obb == NULL)
					goto touch_real;
				err = touch_nomedia(SDCARDFS_SB(dir->i_sb)->shared_obb);
				if (err == -EEXIST)
					err = 0;
			}

			if (unlikely(err))
				errln("sdcardfs: failed to create .nomedia in %s: %d",
					dentry->d_name.name, err);
		}
	}

	err = __sdcardfs_do_create_end(&this, __FUNCTION__, dir, dentry, err);
out:
	trace_sdcardfs_mkdir_exit(dir, dentry, mode, err);
	return err;
}

#include "remove.c"

static int sdcardfs_unlink(struct inode *dir, struct dentry *dentry)
{
	_sdcardfs_do_remove_struct this;
	int ret;
	struct inode *real_inode;

	trace_sdcardfs_unlink_enter(dir, dentry);

	ret = __sdcardfs_do_remove_begin(&this, dir, dentry);
	if (ret)
		goto out;

	real_inode = d_inode(this.real_dentry);
	ihold(real_inode);

	ret = S_ISDIR(real_inode->i_mode) ?
		-EISDIR : vfs_unlink(d_inode(this.real_dir_dentry), this.real_dentry, NULL);

	__sdcardfs_do_remove_end(&this, dir);
	iput(real_inode);	/* truncate real_inode here */
out:
	trace_sdcardfs_unlink_exit(dir, dentry, ret);
	return ret;
}

static int sdcardfs_rmdir(struct inode *dir, struct dentry *dentry)
{
	_sdcardfs_do_remove_struct this;
	int ret;

	trace_sdcardfs_rmdir_enter(dir, dentry);

	ret = __sdcardfs_do_remove_begin(&this, dir, dentry);
	if (ret)
		goto out;

	ret = vfs_rmdir(d_inode(this.real_dir_dentry), this.real_dentry);
	__sdcardfs_do_remove_end(&this, dir);
out:
	trace_sdcardfs_rmdir_exit(dir, dentry, ret);
	return ret;
}

static inline
struct dentry *__lookup_real_rename_ci(
	struct dentry *realdir,
	const char *name,
	struct dentry *dentry
) {
	struct dentry *real;
	real = lookup_one_len(name, realdir, dentry->d_name.len);

	if (IS_ERR(real)) {
		if (real == ERR_PTR(-ENOENT))
			real = NULL;
	/* hashed (see d_delete) and unhashed(by d_alloc) but negative */
	} else if (d_is_negative(real)) {
		dput(real);
		real = ERR_PTR(-ESTALE);
	}
	return real;
}

static inline
struct dentry *__lookup_rename_ci(
	struct dentry *realdir,
	struct dentry *dentry
) {
	struct dentry *ret;
	char *name;

	name = sdcardfs_do_lookup_ci_begin(
		SDCARDFS_SB(dentry->d_sb)->lower_mnt,
		realdir, &dentry->d_name, true
	);
	if (IS_ERR(name) || name == NULL)
		return (struct dentry *)name;

	ret = __lookup_real_rename_ci(realdir, name, dentry);

	sdcardfs_do_lookup_ci_end(name);
	return ret;
}

static int sdcardfs_rename(
	struct inode *old_dir,
	struct dentry *old_dentry,
	struct inode *new_dir, struct dentry *new_dentry
) {
	int err;
	struct dentry *trap, *dentry;
	struct dentry *real_old_parent, *real_new_parent;
	struct dentry *real_old_dentry, *real_new_dentry;
	const struct cred *saved_cred;
	bool overlapped = true;

	trace_sdcardfs_rename_enter(old_dir, old_dentry, new_dir, new_dentry);

	/* since old_dir, new_old both have inode_locked, so
	   it is no need to use dget_parent */
	real_old_dentry = sdcardfs_get_real_dentry(old_dentry);
	real_old_parent = dget_parent(real_old_dentry);

	/* note that real_dir_dentry ?(!=) lower_dentry(dget_parent(dentry)).
	   and it's unsafe to check by use IS_ROOT since inode_lock isnt taken */
	if (unlikely(real_old_parent == real_old_dentry)) {
		err = -EBUSY;
		goto dput_err;
	}

	real_new_parent = sdcardfs_get_lower_dentry(new_dentry->d_parent);

	trap = lock_rename(real_old_parent, real_new_parent);

	/* source should not be ancestor of target */
	if (real_old_dentry == trap) {
		err = -EINVAL;
		goto unlock_err;
	}

	err = -ESTALE;
	/* avoid race between dget_parent and lock_rename */
	if (unlikely(real_old_parent != real_old_dentry->d_parent))
		goto unlock_err;

	/* save current_cred and override it */
	OVERRIDE_CRED(SDCARDFS_SB(old_dir->i_sb), saved_cred);
	if (IS_ERR(saved_cred)) {
		err = PTR_ERR(saved_cred);
		goto unlock_err;
	}

	/* real_old_dentry must be hashed and in the real_old_dir */
	dentry = lookup_one_len(real_old_dentry->d_name.name,
		real_old_parent, real_old_dentry->d_name.len);
	if (IS_ERR(dentry)) {
		/* maybe some err or real_old_parent DEADDIR */
		err = PTR_ERR(dentry);
		goto revert_cred_err;
	}

	dput(dentry);

	if (real_old_dentry != dentry)
		goto revert_cred_err;

	/* real_target may be a negative unhashed dentry */
	dentry = __lookup_rename_ci(real_new_parent, new_dentry);
	if (IS_ERR(dentry)) {
		err = PTR_ERR(dentry);
		goto revert_cred_err;
	} else if (dentry != NULL && dentry != real_old_dentry) {
		real_new_dentry = dentry;
		/* target should not be ancestor of source */
		if (dentry == trap) {
			err = -ENOTEMPTY;
			goto dput2_err;
		}
	} else {
		if (dentry == real_old_dentry)
			dput(dentry);

		/* and if dentry == real_old_dentry, new_dentry
		   could be positive */
		real_new_dentry = lookup_one_len(new_dentry->d_name.name,
			real_new_parent, new_dentry->d_name.len);
		if (IS_ERR(real_new_dentry)) {
			err = PTR_ERR(real_new_dentry);
			goto revert_cred_err;
		}
		overlapped = false;
	}

	err = vfs_rename(d_inode(real_old_parent), real_old_dentry,
		d_inode(real_new_parent), real_new_dentry, NULL, 0);

	dentry = new_dentry->d_parent;
	get_derived_permission4(dentry,
		old_dentry, new_dentry->d_name.name, true);

	fsstack_copy_inode_size(old_dir, d_inode(real_new_parent));
	if (new_dir != old_dir)
		fsstack_copy_inode_size(new_dir, d_inode(real_new_parent));

dput2_err:
	dput(real_new_dentry);
revert_cred_err:
	REVERT_CRED(saved_cred);
unlock_err:
	unlock_rename(real_old_parent, real_new_parent);
	dput(real_new_parent);
dput_err:
	dput(real_old_parent);
	dput(real_old_dentry);

	trace_sdcardfs_rename_exit(old_dir, old_dentry, new_dir, new_dentry, err);
	return err;
}

static int sdcardfs_setattr(struct dentry *dentry, struct iattr *ia)
{
	int err;
	struct iattr copied_ia;
	struct inode *inode = d_inode(dentry);

	/* since sdcardfs uses its own uid/gid derived policy,
	   so uid/gid modification is unsupported */
	if (unlikely(ia->ia_valid & ATTR_FORCE)) {
		copied_ia = *ia;
		copied_ia.ia_valid &= ~(ATTR_UID | ATTR_GID | ATTR_MODE);
		ia = &copied_ia;
	} else
	/* We strictly follow the fat/exfat file system behavior */
	if (((ia->ia_valid & ATTR_UID) &&
		!uid_eq(ia->ia_uid, inode->i_uid)) ||
		((ia->ia_valid & ATTR_GID) &&
		!gid_eq(ia->ia_gid, inode->i_gid)) ||
		((ia->ia_valid & ATTR_MODE) &&
		(ia->ia_mode & ~SDCARDFS_VALID_MODE))) {
		err = SDCARDFS_SB(dentry->d_sb)->options.quiet ? 0 : -EPERM;
		goto out;
	} else
	/* We don't return -EPERM here. Yes, strange, but this is too
	 * old behavior. */
	if (ia->ia_valid & ATTR_MODE)
		ia->ia_valid &= ~ATTR_MODE;

	err = inode_change_ok(inode, ia);
	if (!err) {
		struct dentry *lower_dentry;

		if (ia->ia_valid & ATTR_SIZE) {
			err = inode_newsize_ok(inode, ia->ia_size);
			if (err)
				goto out;
			truncate_setsize(inode, ia->ia_size);
		}

		if (ia->ia_valid & ATTR_FILE) {
			struct file *lower_file = sdcardfs_lower_file(ia->ia_file);
			WARN_ON(lower_file == NULL);
			ia->ia_file = lower_file;
		}

		lower_dentry = sdcardfs_get_lower_dentry(dentry);

		if (lower_dentry != NULL) {
			const struct cred *saved_cred;

			/* Allow touch updating timestamps. */
			ia->ia_valid |= ATTR_FORCE;

			/* save current_cred and override it */
			OVERRIDE_CRED(SDCARDFS_SB(dentry->d_sb), saved_cred);
			if (unlikely(IS_ERR(saved_cred)))
				err = PTR_ERR(saved_cred);
			else {
				inode_lock(d_inode(lower_dentry));
				err = notify_change(lower_dentry, ia, NULL);
				inode_unlock(d_inode(lower_dentry));

				REVERT_CRED(saved_cred);
			}
			dput(lower_dentry);
		}
	}
out:
	return err;
}

static int sdcardfs_getattr(struct vfsmount *mnt, struct dentry *dentry,
		 struct kstat *stat)
{
	int err;
	struct path lower_path;
	const struct cred *saved_cred;

	debugln("%s, dentry=%p, name=%s", __FUNCTION__,
		dentry, dentry->d_name.name);

	if (sdcardfs_get_lower_path(dentry, &lower_path)) {
		WARN_ON(1);
		err = -ESTALE;
		goto out;
	}

	/* save current_cred and override it */
	OVERRIDE_CRED(SDCARDFS_SB(mnt->mnt_sb), saved_cred);
	if (IS_ERR(saved_cred)) {
		err = PTR_ERR(saved_cred);
		goto out_pathput;
	}

	err = vfs_getattr(&lower_path, stat);
	REVERT_CRED(saved_cred);

	if (!err) {
		struct inode *inode = d_inode(dentry);
		struct sdcardfs_tree_entry *te = inode->i_private;

		/* note that generic_fillattr dont take any lock */
		stat->nlink = S_ISDIR(inode->i_mode) ? 1 : inode->i_nlink;

		if (te->revision > inode->i_version) {
			inode_lock(inode);
			__fix_derived_permission(te, inode);
			inode_unlock(inode);
		}
		stat->uid = inode->i_uid;
		stat->gid = inode->i_gid;
		stat->mode = inode->i_mode;
		stat->dev = inode->i_sb->s_dev;		/* fix df */
	}

out_pathput:
	_path_put(&lower_path);
out:
	return err;
}

static int sdcardfs_permission(struct inode *inode, int mask)
{
	bool need_reval;
#ifdef SDCARDFS_PLUGIN_PRIVACY_SPACE
	struct sdcardfs_sb_info *sbi;
#endif
	struct sdcardfs_tree_entry *te = inode->i_private;

	need_reval = te->revision > inode->i_version;

	if (need_reval) {
		if (mask & MAY_NOT_BLOCK)
			return -ECHILD;

		/* FIXME: find a more suitable location */
		__fix_derived_permission(te, inode);
	}

#ifdef SDCARDFS_PLUGIN_PRIVACY_SPACE
	sbi = SDCARDFS_SB(inode->i_sb);
	if (unlikely(sbi->blocked_userid >= 0)) {
		uid_t uid = from_kuid(&init_user_ns, current_fsuid()); /*lint !e666*/

		if (multiuser_get_user_id(uid) == sbi->blocked_userid &&
			multiuser_get_app_id(uid) != sbi->appid_excluded)
			return -EACCES;
	}
#endif

	return generic_permission(inode, mask);
}

const struct inode_operations sdcardfs_dir_iops = {
	.create     = sdcardfs_create,
	.lookup     = sdcardfs_lookup,
	.permission = sdcardfs_permission,
	.unlink     = sdcardfs_unlink,
	.mkdir      = sdcardfs_mkdir,
	.rmdir      = sdcardfs_rmdir,
	.rename     = sdcardfs_rename,
	.setattr    = sdcardfs_setattr,
	.getattr    = sdcardfs_getattr,

	.setxattr = sdcardfs_setxattr,
	.getxattr = sdcardfs_getxattr,
	.listxattr = sdcardfs_listxattr,
	.removexattr = sdcardfs_removexattr,
};

const struct inode_operations sdcardfs_main_iops = {
	.permission = sdcardfs_permission,
	.setattr    = sdcardfs_setattr,
	.getattr    = sdcardfs_getattr,

	.setxattr = sdcardfs_setxattr,
	.getxattr = sdcardfs_getxattr,
	.listxattr = sdcardfs_listxattr,
	.removexattr = sdcardfs_removexattr,
};
