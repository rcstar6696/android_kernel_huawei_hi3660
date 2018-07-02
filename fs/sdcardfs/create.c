/* this file is as part of other files used for including source (eg. inode.c) */

#ifdef SDCARDFS_CASE_INSENSITIVE
struct __create_lookup_ci_private {
	struct dir_context ctx;
	const struct qstr *to_find;
	bool found;
};

static int __sdcardfs_do_create_lookup_ci_match(
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0))
	void *_ctx, const char *name, int namelen,
#else
	struct dir_context *_ctx, const char *name, int namelen,
#endif
	loff_t offset, u64 ino, unsigned int d_type)
{
	struct __create_lookup_ci_private *buf = container_of(_ctx,
		struct __create_lookup_ci_private, ctx);

	if (namelen == buf->to_find->len &&
		!strncasecmp(name, buf->to_find->name, namelen)) {
		buf->found = true;
		return -1;		/* found */
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

static inline int __do_create_lookup_ci(
	struct super_block *sb,
	struct dentry *dir,
	struct qstr *name)
{
	int err;
	struct file *file;
	struct __create_lookup_ci_private buffer = {
			.ctx.actor = __sdcardfs_do_create_lookup_ci_match,
			.to_find = name,
			.found = false
	};
	/* no race, no need to get a reference */
	struct path path = {.dentry = dir,
		.mnt = SDCARDFS_SB(sb)->lower_mnt};
	/* any risk dentry_open within inode_lock(dir)? */
	file = dentry_open(&path,
		O_RDONLY | O_DIRECTORY | O_NOATIME, current_cred());
	if (IS_ERR(file))
		return PTR_ERR(file);
	err = __iterate_dir_locked(file, &buffer.ctx);
	fput(file);
	if (err)
		return err;
	if (buffer.found)
		return -ESTALE;
	return 0;
}
#endif

/* context for sdcardfs_do_create_xxxx */
typedef struct sdcardfs_do_create_struct {
	struct dentry *parent, *real_dentry;
	struct dentry *real_dir_dentry;
	const struct cred *saved_cred;
	struct fs_struct *saved_fs;
} _sdcardfs_do_create_struct;

#define this(x)		__->x

static int __sdcardfs_do_create_begin(
	_sdcardfs_do_create_struct *__,
	struct inode *dir,
	struct dentry *dentry
) {
	int err;
	struct sdcardfs_sb_info *sbi;

	if (SDCARDFS_D(dentry) != NULL) {
		warnln("%s, negative dentry(%s) should not have tree entry",
			__FUNCTION__, dentry->d_name.name);
		return -ESTALE;
	}

	this(parent) = dget_parent(dentry);
	BUG_ON(d_inode(this(parent)) != dir);

	this(real_dir_dentry) = sdcardfs_get_lower_dentry(this(parent));
	BUG_ON(this(real_dir_dentry) == NULL);

	inode_lock_nested(d_inode(this(real_dir_dentry)), I_MUTEX_PARENT);

	/* save current_cred and override it */
	sbi = SDCARDFS_SB(dir->i_sb);
	OVERRIDE_CRED(sbi, this(saved_cred));
	if (IS_ERR(this(saved_cred))) {
		err = PTR_ERR(this(saved_cred));
		goto unlock_err;
	}

#ifdef SDCARDFS_CASE_INSENSITIVE
	err = __do_create_lookup_ci(dentry->d_sb,
		this(real_dir_dentry), &dentry->d_name);
	if (err) {
		goto revert_cred_err;
	}
#endif

	this(real_dentry) = lookup_one_len(dentry->d_name.name,
		this(real_dir_dentry), dentry->d_name.len);

	if (IS_ERR(this(real_dentry))) {
		err = PTR_ERR(this(real_dentry));
		goto revert_cred_err;
	}

	if (d_inode(this(real_dentry)) != NULL) {
		err = -ESTALE;
		goto dput_err;
	}

	BUG_ON(sbi->override_fs == NULL);
	this(saved_fs) = override_current_fs(sbi->override_fs);
	return 0;

dput_err:
	dput(this(real_dentry));
revert_cred_err:
	REVERT_CRED(this(saved_cred));
unlock_err:
	inode_unlock(d_inode(this(real_dir_dentry)));
	dput(this(real_dir_dentry));
	dput(this(parent));
	return err;
}

static int __sdcardfs_do_create_end(
	_sdcardfs_do_create_struct *__,
	const char *__caller_FUNCTION__,
	struct inode *dir,
	struct dentry *dentry,
	int err
) {
	revert_current_fs(this(saved_fs));
	REVERT_CRED(this(saved_cred));

	if (err) {
		dput(this(real_dentry));
		goto out;
	}

	err = PTR_ERR(sdcardfs_interpose(this(parent),
		dentry, this(real_dentry)));
	if (err)
		errln("%s, unexpected error when interposing: %d",
			__caller_FUNCTION__, err);

out:
	fsstack_copy_inode_size(dir, d_inode(this(real_dir_dentry)));

	inode_unlock(d_inode(this(real_dir_dentry)));
	dput(this(real_dir_dentry));
	dput(this(parent));
	return err;
}
