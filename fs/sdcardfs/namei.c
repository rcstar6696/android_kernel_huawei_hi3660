/* vim:set ts=4 sw=4 tw=0 noet ft=c:
 *
 * fs/sdcardfs/namei.c
 *
 * Copyright (C) 2017 HUAWEI, Inc.
 * Author: gaoxiang <gaoxiang25@huawei.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of the Linux
 * distribution for more details.
 */
#include "sdcardfs.h"
#include "linux/delay.h"
#include <linux/version.h>

#include "trace-events.h"

static int __is_weird_inode(mode_t mode) {
	return S_ISBLK(mode) || S_ISCHR(mode) ||
		S_ISFIFO(mode) || S_ISSOCK(mode) || S_ISLNK(mode);
}

struct inode *sdcardfs_ialloc(
	struct super_block *sb,
	mode_t mode
){
	struct inode *inode = new_inode(sb);

	if (unlikely(inode == NULL))
		return ERR_PTR(-ENOMEM);

	inode->i_ino = get_next_ino();
	inode->i_version = 1;
	inode->i_generation = get_seconds();

	BUG_ON(__is_weird_inode(mode));

	/* use different set of inode ops for symlinks & directories */
	if (S_ISDIR(mode)) {
		inode->i_op = &sdcardfs_dir_iops;
		inode->i_fop = &sdcardfs_dir_fops;
	} else {
		inode->i_op = &sdcardfs_main_iops;
		inode->i_fop = &sdcardfs_main_fops;
	}

	inode->i_flags |= S_NOATIME | S_NOCMTIME;
	inode->i_mode = mode;
	inode->i_mapping->a_ops = &sdcardfs_aops;

	trace_sdcardfs_ialloc(inode);
	return inode;
}

/* 1) it's optional to lock the parent by the caller.
      so we cannot assume real_dentry is still hashed now
   2) sdcardfs_interpose use a real_dentry refcount
      increased by the caller */
struct dentry *sdcardfs_interpose(
	struct dentry *parent,
	struct dentry *dentry,
	struct dentry *real_dentry
) {
	/* d_sb has been assigned by d_alloc */
	struct sdcardfs_tree_entry *te;
	struct dentry *lower_dentry;
	struct inode *inode, *lower_inode;
	struct super_block *sb = dentry->d_sb;

	/* dentry cannot be seen by others, so it's no need
	   taking dentry d_lock */
	BUG_ON(!d_unhashed(dentry));
	BUG_ON(d_really_is_positive(dentry));

	/* since the real_dentry is referenced, it cannot turn
	   into negative state. therefore it is no need taking d_lock.
	   there are some races with parent, d_name? */
	BUG_ON(d_is_negative(real_dentry));

	te = sdcardfs_init_tree_entry(dentry, real_dentry);
	if (unlikely(te == NULL)) {
		dput(real_dentry);
		return ERR_PTR(-ENOMEM);
	}
	get_derived_permission(parent, dentry);

	/* before d_add, we can access lower_dentry without locking */
	lower_dentry = (te->ovl != NULL ? te->ovl : te->real.dentry);

	lower_inode = d_inode(lower_dentry);
	inode = sdcardfs_ialloc(sb, lower_inode->i_mode);
	if (IS_ERR(inode)) {
		errln("%s, failed to alloc inode, err=%d",
			__FUNCTION__, PTR_ERR(inode));

		sdcardfs_free_tree_entry(dentry);
		return (struct dentry *)inode;
	}

	/* used for revalidate in inode_permission */
	inode->i_private = te;
	fsstack_copy_inode_size(inode, lower_inode);

	__fix_derived_permission(te, inode);
	d_add(dentry, inode);
	return NULL;
}

static int __is_weird_dentry(struct dentry *dentry)
{
	int weird = dentry->d_flags & (DCACHE_NEED_AUTOMOUNT |
		DCACHE_MANAGE_TRANSIT);
	if (likely(!weird)) {
		/* since whether d_inode == NULL has
		   been checked by the caller */
		weird = __is_weird_inode(d_inode(dentry)->i_mode);
	}
	return weird;
}

static inline
struct dentry *__lookup_real(struct dentry *dir,
	const char *name, int len
) {
	struct dentry *real;

	inode_lock_nested(d_inode(dir), I_MUTEX_PARENT);
	real = lookup_one_len(name, dir, len);
	inode_unlock(d_inode(dir));

	if (IS_ERR(real)) {
		if (real == ERR_PTR(-ENOENT))
			real = NULL;
	} else if (d_really_is_negative(real)) {
		dput(real);
		real = NULL;
	} else if (__is_weird_dentry(real)) {
		dput(real);
		/* Don't support traversing automounts and other weirdness */
		real = ERR_PTR(-EREMOTE);
	}
	return real;
}

static inline
struct dentry *__lookup_real_ci(
	struct vfsmount *mnt,
	struct dentry *dir,
	struct qstr *qname)
{
	struct dentry *ret;
	char *name;

	name = sdcardfs_do_lookup_ci_begin(mnt, dir,
		qname, false);

	if (IS_ERR(name) || name == NULL)
		return (struct dentry *)name;

	ret = __lookup_real(dir, name, qname->len);
	sdcardfs_do_lookup_ci_end(name);
	if (ret == NULL)
		ret = ERR_PTR(-ESTALE);
	return ret;
}

/* parent dir lock should be locked */
struct dentry *
sdcardfs_lookup(struct inode *dir,
	struct dentry *dentry, unsigned int flags)
{
	struct dentry *parent;
	struct dentry *lower_dir_dentry, *ret;
	struct inode *lower_dir_inode;
	struct sdcardfs_sb_info *sbi;
	const struct cred *saved_cred;

	BUG_ON(IS_ROOT(dentry));

	parent = dget_parent(dentry);
	BUG_ON(d_inode(parent) != dir);

	/* d_revalidate should have been triggered. so
	   the lower_dir_entry must be hashed */
	lower_dir_dentry = sdcardfs_get_lower_dentry(parent);
	BUG_ON(lower_dir_dentry == NULL);

	lower_dir_inode = d_inode(lower_dir_dentry);
	BUG_ON(lower_dir_inode == NULL);

	sbi = SDCARDFS_SB(dentry->d_sb);
	OVERRIDE_CRED(sbi, saved_cred);
	if (IS_ERR(saved_cred)) {
		ret = (struct dentry *)saved_cred;
		goto out;
	}

	ret = __lookup_real(lower_dir_dentry,
		dentry->d_name.name, dentry->d_name.len);
	if (ret == NULL) {
#ifdef SDCARDFS_CASE_INSENSITIVE
		ret = __lookup_real_ci(sbi->lower_mnt, lower_dir_dentry, &dentry->d_name);
		if (ret == NULL) {
#endif
			if (!(flags & (LOOKUP_CREATE | LOOKUP_RENAME_TARGET)))
				ret = ERR_PTR(-ENOENT);
			else {
				/* in this case, the dentry is still negative.
				   fsdata will be initialized in create/rename */

				/* and, we dont need to d_instantiate the dentry */
				/* since DCACHE_MISS_TYPE == 0x00000000 */
				/* d_instantiate(dentry, NULL); */
			}
			REVERT_CRED(saved_cred);
			goto out;
#ifdef SDCARDFS_CASE_INSENSITIVE
		}
#endif
	}
	REVERT_CRED(saved_cred);

	if (!IS_ERR(ret))
		ret = sdcardfs_interpose(parent, dentry, ret);
out:
	/* Only in __sdcardfs_interpose, sdcardfs_init_tree_entry would
	   be called. So we can d_release a dentry without tree_entry */
	dput(lower_dir_dentry);
	dput(parent);

	trace_sdcardfs_lookup(dir, dentry, flags);
	return ret;
}

