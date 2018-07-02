/* vim:set ts=4 sw=4 tw=0 noet ft=c:
 *
 * fs/sdcardfs/super.c
 *
 * Copyright (C) 2017 HUAWEI, Inc.
 * Author: gaoxiang <gaoxiang25@huawei.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of the Linux
 * distribution for more details.
 */
#include "sdcardfs.h"

/* could be triggered after deactivate_locked_super()
   is called, thus including umount and failed to initialize. */
static void sdcardfs_put_super(struct super_block *sb)
{
	struct vfsmount *lower_mnt;
	struct sdcardfs_sb_info *sbi = SDCARDFS_SB(sb);

	/* failed to read_super */
	if (sbi == NULL)
		return;

	/* if exists, dput(shared_obb) */
	dput(sbi->shared_obb);

	if (sbi->sdcardd_cred != NULL)
		put_cred(sbi->sdcardd_cred);

	free_fs_struct(sbi->override_fs);

	if (sbi->devpath_s == NULL)
		errln("%s, unexpected sbi->devpath_s == NULL",
			__FUNCTION__);
	else {
		infoln("unmounting on top of %s\n", sbi->devpath_s);
		__putname(sbi->devpath_s);
	}

	/* deal with lower_sb & lower_mnt */
	lower_mnt = sbi->lower_mnt;
	BUG_ON(lower_mnt == NULL);
	atomic_dec(&lower_mnt->mnt_sb->s_active);
	mntput(lower_mnt);

#ifdef SDCARDFS_SUPPORT_RESERVED_SPACE
	_path_put(&sbi->basepath);
#endif

#ifdef SDCARDFS_SYSFS_FEATURE
	kobject_put(&sbi->kobj);
#else
	kfree(sbi);
#endif
	sb->s_fs_info = NULL;
}

static int sdcardfs_statfs(struct dentry *dentry, struct kstatfs *buf)
{
	int err;

#ifdef SDCARDFS_SUPPORT_RESERVED_SPACE
	struct sdcardfs_sb_info *sbi = SDCARDFS_SB(dentry->d_sb);
	err = vfs_statfs(&sbi->basepath, buf);

	if (sbi->options.reserved_mb) {
		u64 min_blocks;

		/* Invalid statfs informations. */
		if (!buf->f_bsize) {
			errln("f_bsize == 0 returned by underlay_statfs.");
			return -EINVAL;
		}

		min_blocks = ((u64)sbi->options.reserved_mb << 20) / (u64)buf->f_bsize;
		buf->f_blocks -= min_blocks;

		if (buf->f_bavail > min_blocks)
			buf->f_bavail -= min_blocks;
		else
			buf->f_bavail = 0;

		/* Make reserved blocks invisiable to media storage */
		buf->f_bfree = buf->f_bavail;
	}
#else
	struct path lower_path;

	sdcardfs_get_lower_path(dentry, &lower_path);
	err = vfs_statfs(&lower_path, buf);
	_path_put(&lower_path);
#endif

	/* set return buf to our f/s to avoid confusing user-level utils */
	buf->f_type = SDCARDFS_SUPER_MAGIC;

	return err;
}

/* @flags: numeric mount options
   @options: mount options string */
static int sdcardfs_remount_fs(struct super_block *sb, int *flags, char *options)
{
	int err = 0;

	/* The VFS will take care of "ro" and "rw" flags among others.  We
	   can safely accept a few flags (RDONLY, MANDLOCK), and honor
	   SILENT, but anything else left over is an error. */
	if ((*flags & ~(MS_RDONLY | MS_MANDLOCK | MS_SILENT)) != 0) {
		errln("remount flags 0x%x unsupported", *flags);
		err = -EINVAL;
	}

	return err;
}

/*
 * Called by iput() when the inode reference count reached zero
 * and the inode is not hashed anywhere.  Used to clear anything
 * that needs to be, before the inode is completely destroyed and put
 * on the inode free list.
 */
static void sdcardfs_evict_inode(struct inode *inode)
{
	truncate_inode_pages(&inode->i_data, 0);
	clear_inode(inode);
}

/*
 * Used only in nfs, to kill any pending RPC tasks, so that subsequent
 * code can actually succeed and won't leave tasks that need handling.
 */
static void sdcardfs_umount_begin(struct super_block *sb)
{
	struct super_block *lower_sb;

	lower_sb = sdcardfs_lower_super(sb);
	if (lower_sb && lower_sb->s_op && lower_sb->s_op->umount_begin)
		lower_sb->s_op->umount_begin(lower_sb);
}

static int sdcardfs_show_options(struct seq_file *m,
	struct dentry *root)
{
	struct sdcardfs_mount_options *opts
		= &SDCARDFS_SB(root->d_sb)->options;

#define xx(...)  seq_printf(m, __VA_ARGS__)
#define __SDCARDFS_MISC__SHOW_OPTIONS
#include "misc.h"
	return 0;
};

const struct super_operations sdcardfs_sops = {
	.put_super      = sdcardfs_put_super,
	.statfs         = sdcardfs_statfs,
	.remount_fs     = sdcardfs_remount_fs,
	.evict_inode    = sdcardfs_evict_inode,
	.umount_begin   = sdcardfs_umount_begin,
	.show_options   = sdcardfs_show_options,
	.drop_inode     = generic_delete_inode,
};
