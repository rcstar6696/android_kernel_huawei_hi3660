/* vim:set ts=4 sw=4 tw=0 noet ft=c:
 *
 * fs/sdcardfs/lookup_ci.c
 *
 * Copyright (C) 2017 HUAWEI, Inc.
 * Author: gaoxiang <gaoxiang25@huawei.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of the Linux
 * distribution for more details.
 */
#include "sdcardfs.h"
#include <linux/version.h>

struct __lookup_ci_private {
	struct dir_context ctx;
	const struct qstr *to_find;
	char *name;
};

static
int __lookup_ci_match(
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0))
	void *_ctx, const char *name, int namelen,
#else
	struct dir_context *_ctx, const char *name, int namelen,
#endif
	loff_t offset, u64 ino, unsigned int d_type
) {
	struct __lookup_ci_private *buf = container_of(_ctx,
		struct __lookup_ci_private, ctx);

	if (namelen == buf->to_find->len &&
		!strncasecmp(name, buf->to_find->name, namelen)) {
		buf->name = __getname();
		if (buf->name != NULL) {
			memcpy(buf->name, name, namelen);
			buf->name[namelen] = '\0';
		}
		return -1;
	}
	return 0;
}

static inline int __iterate_dir_locked(struct file *file,
	struct dir_context *ctx)
{
	struct inode *inode;
	int res;
	if (file->f_op->iterate == NULL)
		return -ENOTDIR;

	inode = file_inode(file);
	if (IS_DEADDIR(inode))
		return -ENOENT;

	ctx->pos = file->f_pos;
	res = file->f_op->iterate(file, ctx);
	file->f_pos = ctx->pos;
	return res;
}

/* remember that the currect cred has been overrided */
char *sdcardfs_do_lookup_ci_begin(
	struct vfsmount *mnt, struct dentry *dir,
	struct qstr *name, bool locked
){
	int err;
	struct file *file;
	struct __lookup_ci_private buffer = {
			.ctx.actor = __lookup_ci_match,
			.to_find = name,
			.name = NULL,
	};

	/* no race, no need to get a reference */
	struct path path = {.dentry = dir, .mnt = mnt};

	/* any risk dentry_open within inode_lock(dir)? */
	file = dentry_open(&path,
		O_RDONLY | O_DIRECTORY | O_NOATIME, current_cred());	/*lint !e666*/

	if (IS_ERR(file))
		goto out;

	err = locked ? __iterate_dir_locked(file, &buffer.ctx) :
		iterate_dir(file, &buffer.ctx);
	fput(file);
	if (err)
		return ERR_PTR(err);
out:
	return buffer.name;
}

void sdcardfs_do_lookup_ci_end(char *ci) {
	BUG_ON(ci == NULL);
	__putname(ci);
}
