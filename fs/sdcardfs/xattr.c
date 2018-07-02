/* vim:set ts=4 sw=4 tw=0 noet ft=c:
 *
 * fs/sdcardfs/xattr.c
 *
 * Copyright (c) 2013 Samsung Electronics Co. Ltd
 *   Authors: Daeho Jeong, Woojoong Lee, Seunghwan Hyun,
 *               Sunghwan Yun, Sungjong Seo
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of the Linux
 * distribution for more details.
 */
#include <linux/xattr.h>
#include "sdcardfs.h"

int sdcardfs_setxattr(struct dentry *dentry, const char *name,
	const void *value, size_t size, int flags)
{
	int rc;
	struct dentry *lower_dentry = sdcardfs_get_lower_dentry(dentry);
	struct inode *lower_inode = d_inode(lower_dentry);

	if (unlikely(lower_inode->i_op->setxattr == NULL)) {
		rc = -EOPNOTSUPP;
		goto out;
	}

	rc = vfs_setxattr(lower_dentry, name, value, size, flags);
out:
	dput(lower_dentry);
	return rc;
}

ssize_t sdcardfs_getxattr(struct dentry *dentry,
	const char *name, void *value, size_t size)
{
	ssize_t rc;
	struct dentry *lower_dentry = sdcardfs_get_lower_dentry(dentry);
	struct inode *lower_inode = d_inode(lower_dentry);

	if (unlikely(lower_inode->i_op->getxattr == NULL)) {
		rc = -EOPNOTSUPP;
		goto out;
	}

	rc = lower_inode->i_op->getxattr(lower_dentry, name, value, size);
out:
	dput(lower_dentry);
	return rc;
}

ssize_t sdcardfs_listxattr(struct dentry *dentry, char *list, size_t size)
{
	ssize_t rc;
	struct dentry *lower_dentry = sdcardfs_get_lower_dentry(dentry);
	struct inode *lower_inode = d_inode(lower_dentry);

	if (unlikely(lower_inode->i_op->listxattr == NULL)) {
		rc = -EOPNOTSUPP;
		goto out;
	}

	rc = lower_inode->i_op->listxattr(lower_dentry, list, size);
out:
	dput(lower_dentry);
	return rc;
}

int sdcardfs_removexattr(struct dentry *dentry, const char *name)
{
	ssize_t rc;
	struct dentry *lower_dentry = sdcardfs_get_lower_dentry(dentry);
	struct inode *lower_inode = d_inode(lower_dentry);

	if (unlikely(lower_inode->i_op->removexattr == NULL)) {
		rc = -EOPNOTSUPP;
		goto out;
	}

	inode_lock(lower_inode);
	rc = lower_inode->i_op->removexattr(lower_dentry, name);
	inode_unlock(lower_inode);
out:
	dput(lower_dentry);
	return rc;
}
