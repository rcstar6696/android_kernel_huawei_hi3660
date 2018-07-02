/*
 * fs/sdcardfs/file.c
 *
 * Copyright (c) 2013 Samsung Electronics Co. Ltd
 *   Authors: Daeho Jeong, Woojoong Lee, Seunghwan Hyun,
 *               Sunghwan Yun, Sungjong Seo
 *
 * This program has been developed as a stackable file system based on
 * the WrapFS which written by
 *
 * Copyright (c) 1998-2011 Erez Zadok
 * Copyright (c) 2009     Shrikar Archak
 * Copyright (c) 2003-2011 Stony Brook University
 * Copyright (c) 2003-2011 The Research Foundation of SUNY
 *
 * This file is dual licensed.  It may be redistributed and/or modified
 * under the terms of the Apache 2.0 License OR version 2 of the GNU
 * General Public License.
 */

#include "sdcardfs.h"
#ifdef CONFIG_SDCARD_FS_FADV_NOACTIVE
#include <linux/backing-dev.h>
#endif
#include <linux/version.h>
#include <linux/uio.h>

static ssize_t sdcardfs_read(struct file *file, char __user *buf,
			   size_t count, loff_t *ppos)
{
	int err;
	struct file *lower_file;
	struct dentry *dentry = file->f_path.dentry;
#ifdef CONFIG_SDCARD_FS_FADV_NOACTIVE
	struct backing_dev_info *bdi;
#endif

	lower_file = sdcardfs_lower_file(file);

#ifdef CONFIG_SDCARD_FS_FADV_NOACTIVE
	if (file->f_mode & FMODE_NOACTIVE) {
		if (!(lower_file->f_mode & FMODE_NOACTIVE)) {
			bdi = lower_file->f_mapping->backing_dev_info;
			lower_file->f_ra.ra_pages = bdi->ra_pages * 2;
			spin_lock(&lower_file->f_lock);
			lower_file->f_mode |= FMODE_NOACTIVE;
			spin_unlock(&lower_file->f_lock);
		}
	}
#endif

	err = vfs_read(lower_file, buf, count, ppos);
	/* update our inode atime upon a successful lower read */
	if (err >= 0) {
		fsstack_copy_attr_atime(d_inode(dentry),
			file_inode(lower_file));
	}

	return err;
}

static ssize_t sdcardfs_write(struct file *file, const char __user *buf,
			    size_t count, loff_t *ppos)
{
	int err;
	struct file *lower_file;
	struct dentry *dentry = file->f_path.dentry;

#ifdef SDCARDFS_SUPPORT_RESERVED_SPACE
	/* check disk space */
	if (!check_min_free_space(dentry->d_sb, count, 0)) {
		infoln("%s, No minimum free space.", __FUNCTION__);
		return -ENOSPC;
	}
#endif

	lower_file = sdcardfs_lower_file(file);
	err = vfs_write(lower_file, buf, count, ppos);
	/* update our inode times+sizes upon a successful lower write */
	if (err >= 0) {
		fsstack_copy_inode_size(d_inode(dentry),
					file_inode(lower_file));
		fsstack_copy_attr_times(d_inode(dentry),
					file_inode(lower_file));
	}

	return err;
}

static int sdcardfs_readdir(struct file *file, struct dir_context *ctx)
{
	int err;
	struct file *lower_file = NULL;
	struct dentry *dentry = file->f_path.dentry;

	lower_file = sdcardfs_lower_file(file);

	lower_file->f_pos = file->f_pos;
	err = iterate_dir(lower_file, ctx);
	file->f_pos = lower_file->f_pos;
	if (err >= 0)		/* copy the atime */
		fsstack_copy_attr_atime(d_inode(dentry),
					file_inode(lower_file));
	return err;
}

static long sdcardfs_unlocked_ioctl(struct file *file, unsigned int cmd,
				  unsigned long arg)
{
	long err = -ENOTTY;
	struct file *lower_file;

	lower_file = sdcardfs_lower_file(file);

	/* XXX: use vfs_ioctl if/when VFS exports it */
	if (!lower_file || !lower_file->f_op)
		goto out;
	if (lower_file->f_op->unlocked_ioctl)
		err = lower_file->f_op->unlocked_ioctl(lower_file, cmd, arg);

out:
	return err;
}

#ifdef CONFIG_COMPAT
static long sdcardfs_compat_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	long err = -ENOTTY;
	struct file *lower_file;

	lower_file = sdcardfs_lower_file(file);

	/* XXX: use vfs_ioctl if/when VFS exports it */
	if (!lower_file || !lower_file->f_op)
		goto out;
	if (lower_file->f_op->compat_ioctl)
		err = lower_file->f_op->compat_ioctl(lower_file, cmd, arg);

out:
	return err;
}
#endif

static int sdcardfs_mmap(struct file *file, struct vm_area_struct *vma)
{
	int err = 0;
	bool willwrite;
	struct file *lower_file;
	const struct vm_operations_struct *saved_vm_ops = NULL;

	/* this might be deferred to mmap's writepage */
	willwrite = ((vma->vm_flags | VM_SHARED | VM_WRITE) == vma->vm_flags);

	/*
	 * File systems which do not implement ->writepage may use
	 * generic_file_readonly_mmap as their ->mmap op.  If you call
	 * generic_file_readonly_mmap with VM_WRITE, you'd get an -EINVAL.
	 * But we cannot call the lower ->mmap op, so we can't tell that
	 * writeable mappings won't work.  Therefore, our only choice is to
	 * check if the lower file system supports the ->writepage, and if
	 * not, return EINVAL (the same error that
	 * generic_file_readonly_mmap returns in that case).
	 */
	lower_file = sdcardfs_lower_file(file);
	if (willwrite && !lower_file->f_mapping->a_ops->writepage) {
		err = -EINVAL;
		errln("lower file system does not support writeable mmap");
		goto out;
	}

	/*
	 * find and save lower vm_ops.
	 *
	 * XXX: the VFS should have a cleaner way of finding the lower vm_ops
	 */
	if (!SDCARDFS_F(file)->lower_vm_ops) {
		err = lower_file->f_op->mmap(lower_file, vma);
		if (err) {
			errln("lower mmap failed %d", err);
			goto out;
		}
		saved_vm_ops = vma->vm_ops; /* save: came from lower ->mmap */
	}

	/*
	 * Next 3 lines are all I need from generic_file_mmap.  I definitely
	 * don't want its test for ->readpage which returns -ENOEXEC.
	 */
	file_accessed(file);
	vma->vm_ops = &sdcardfs_vm_ops;

	file->f_mapping->a_ops = &sdcardfs_aops; /* set our aops */
	if (!SDCARDFS_F(file)->lower_vm_ops) /* save for our ->fault */
		SDCARDFS_F(file)->lower_vm_ops = saved_vm_ops;
	vma->vm_private_data = file;
	get_file(lower_file);
	vma->vm_file = lower_file;

out:
	return err;
}

static int sdcardfs_open(struct inode *inode, struct file *file)
{
	int err;
	struct file *lower_file;
	struct path lower_path;

	struct dentry *dentry = file->f_path.dentry;
	struct dentry *parent = dget_parent(dentry);
	struct sdcardfs_sb_info *sbi = SDCARDFS_SB(dentry->d_sb);
	const struct cred *saved_cred = NULL;

	/* don't open unhashed/deleted files */
	if (d_unhashed(dentry)) {
		err = -ENOENT;
		goto out_err;
	}

	if (!check_caller_access_to_name(parent, dentry->d_name.name)) {
		err = -EACCES;
		goto out_err;
	}

	/* save current_cred and override it */
	OVERRIDE_CRED(sbi, saved_cred);
	if (IS_ERR(saved_cred)) {
		err = PTR_ERR(saved_cred);
		goto out_err;
	}

	file->private_data =
		kzalloc(sizeof(struct sdcardfs_file_info), GFP_KERNEL);
	if (!SDCARDFS_F(file)) {
		err = -ENOMEM;
		goto out_revert_cred;
	}

	/* open lower object and link sdcardfs's file struct to lower's */
	sdcardfs_get_lower_path(dentry, &lower_path);
	lower_file = dentry_open(&lower_path, file->f_flags, current_cred());	/*lint !e666*/
	/* TODO: add file opened statistics */
	if (IS_ERR(lower_file)) {
		err = PTR_ERR(lower_file);
		kfree(SDCARDFS_F(file));
	} else {
		SDCARDFS_F(file)->lower_file = lower_file;
		sdcardfs_copy_and_fix_attrs(inode, d_inode(lower_path.dentry));
		err = 0;	/* open success */
	}
	_path_put(&lower_path);

out_revert_cred:
	REVERT_CRED(saved_cred);
out_err:
	dput(parent);
	return err;
}

static int sdcardfs_flush(struct file *file, fl_owner_t id)
{
	int err = 0;
	struct file *lower_file = NULL;

	lower_file = sdcardfs_lower_file(file);
	if (lower_file && lower_file->f_op && lower_file->f_op->flush) {
		filemap_write_and_wait(file->f_mapping);
		err = lower_file->f_op->flush(lower_file, id);
	}

	return err;
}

/* release all lower object references & free the file info structure */
static int sdcardfs_file_release(struct inode *inode, struct file *file)
{
	struct file *lower_file;

	lower_file = sdcardfs_lower_file(file);
	if (lower_file) {
		SDCARDFS_F(file)->lower_file = NULL;
		fput(lower_file);
	}

	kfree(SDCARDFS_F(file));
	return 0;
}

static int sdcardfs_fsync(struct file *file, loff_t start, loff_t end,
			int datasync)
{
	int err;
	struct file *lower_file;
	struct path lower_path;
	struct dentry *dentry = file->f_path.dentry;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0))
	err = __generic_file_fsync(file, start, end, datasync);
#else
	err = generic_file_fsync(file, start, end, datasync);
#endif
	if (err)
		goto out;

	lower_file = sdcardfs_lower_file(file);
	sdcardfs_get_lower_path(dentry, &lower_path);
	err = vfs_fsync_range(lower_file, start, end, datasync);
	_path_put(&lower_path);
out:
	return err;
}

static int sdcardfs_fasync(int fd, struct file *file, int flag)
{
	int err = 0;
	struct file *lower_file = NULL;

	lower_file = sdcardfs_lower_file(file);
	if (lower_file->f_op && lower_file->f_op->fasync)
		err = lower_file->f_op->fasync(fd, lower_file, flag);

	return err;
}

static ssize_t sdcardfs_read_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	int err;
	struct file *file = iocb->ki_filp, *lower_file;

	lower_file = sdcardfs_lower_file(file);
	if (!lower_file->f_op->read_iter) {
		err = -EINVAL;
		goto out;
	}

	get_file(lower_file); /* prevent lower_file from being released */
	iocb->ki_filp = lower_file;
	err = lower_file->f_op->read_iter(iocb, iter);
	iocb->ki_filp = file;
	fput(lower_file);
out:
	return err;
}

ssize_t sdcardfs_write_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	int err;
	struct file *file = iocb->ki_filp,
		*lower_file =  sdcardfs_lower_file(file);

	if (lower_file->f_op->write_iter == NULL) {
		err = -EINVAL;
		goto out;
	}

#ifdef SDCARDFS_SUPPORT_RESERVED_SPACE
	/* check disk space */
	if (!check_min_free_space(file->f_path.dentry->d_sb,
		iov_iter_count(iter), 0)) {
		infoln("%s, No minimum free space.", __FUNCTION__);
		err = -ENOSPC;
		goto out;
	}
#endif
	get_file(lower_file); /* prevent lower_file from being released */
	iocb->ki_filp = lower_file;
	err = lower_file->f_op->write_iter(iocb, iter);
	iocb->ki_filp = file;
	fput(lower_file);

	/* update overlay inode sizes upon a successful lower write */
	if (err >= 0 || err == -EIOCBQUEUED)
		fsstack_copy_inode_size(d_inode(file->f_path.dentry),
					file_inode(lower_file));
out:
	return err;
}

/*
 * Sdcardfs cannot use generic_file_llseek as ->llseek, because it would
 * only set the offset of the upper file.  So we have to implement our
 * own method to set both the upper and lower file offsets
 * consistently.
 */
static loff_t sdcardfs_file_llseek(struct file *file, loff_t offset, int whence)
{
	int err;
	struct file *lower_file;

	err = generic_file_llseek(file, offset, whence);
	if (err < 0)
		goto out;

	lower_file = sdcardfs_lower_file(file);
	err = generic_file_llseek(lower_file, offset, whence);

out:
	return err;
}


const struct file_operations sdcardfs_main_fops = {
	.llseek		= generic_file_llseek,
	.read		= sdcardfs_read,
	.write		= sdcardfs_write,
	.unlocked_ioctl	= sdcardfs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= sdcardfs_compat_ioctl,
#endif
	.mmap		= sdcardfs_mmap,
	.open		= sdcardfs_open,
	.flush		= sdcardfs_flush,
	.release	= sdcardfs_file_release,
	.fsync		= sdcardfs_fsync,
	.fasync		= sdcardfs_fasync,
	.read_iter	= sdcardfs_read_iter,
	.write_iter	= sdcardfs_write_iter,
};

/* trimmed directory options */
const struct file_operations sdcardfs_dir_fops = {
	.llseek		= sdcardfs_file_llseek,
	.read		= generic_read_dir,
	.iterate	= sdcardfs_readdir,
	.unlocked_ioctl	= sdcardfs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= sdcardfs_compat_ioctl,
#endif
	.open		= sdcardfs_open,
	.release	= sdcardfs_file_release,
	.flush		= sdcardfs_flush,
	.fsync		= sdcardfs_fsync,
	.fasync		= sdcardfs_fasync,
};
