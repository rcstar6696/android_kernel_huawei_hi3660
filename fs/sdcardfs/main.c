/* vim:set ts=4 sw=4 tw=0 noet ft=c:
 *
 * fs/sdcardfs/main.c
 *
 * Copyright (C) 2017 HUAWEI, Inc.
 * Author: gaoxiang <gaoxiang25@huawei.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of the Linux
 * distribution for more details.
 */
#include "sdcardfs.h"
#include <linux/module.h>
#include <linux/types.h>
#include <linux/parser.h>

#define CREATE_TRACE_POINTS
#include "trace-events.h"

enum {
	Opt_fsuid,
	Opt_fsgid,
	Opt_gid,
	Opt_debug,
	Opt_mask,
	Opt_multiuser,
	Opt_userid,
	Opt_fs_scontext,
	Opt_reserved_mb,
	Opt_quiet,
	Opt_err,
};

static const match_table_t sdcardfs_tokens = {
	{Opt_fsuid, "fsuid=%u"},
	{Opt_fsgid, "fsgid=%u"},
	{Opt_gid, "gid=%u"},
	{Opt_debug, "debug"},
	{Opt_mask, "mask=%u"},
	{Opt_userid, "userid=%d"},
	{Opt_multiuser, "multiuser"},
	{Opt_fs_scontext, "fs_scontext=%s"},
	{Opt_reserved_mb, "reserved_mb=%u"},
	{Opt_quiet, "quiet"},
	{Opt_err, NULL}
};

static void default_options(struct sdcardfs_mount_options *opts)
{
	/* by default, we use AID_MEDIA_RW as uid, gid */
	opts->fs_low_uid = AID_MEDIA_RW;
	opts->fs_low_gid = AID_MEDIA_RW;
	opts->mask = 0;
	opts->multiuser = false;
	opts->fs_user_id = 0;
	opts->gid = AID_SDCARD_RW;
	opts->has_fssecid = false;
	/* by default, 0MB is reserved */
	opts->reserved_mb = 0;
	opts->quiet = false;
}

int sdcardfs_parse_options(struct sdcardfs_mount_options *opts,
	char *options, int silent, int *debug)
{
	char *p, *string_option;
	substring_t args[MAX_OPT_ARGS];
	int option;

	if (options == NULL)
		return 0;

	while ((p = strsep(&options, ",")) != NULL) {
		int token;
		if (*p == '\0')
			continue;

		token = match_token(p, sdcardfs_tokens, args);

		switch (token) {
		case Opt_debug:
			break;
		case Opt_fsuid:
			if (match_int(&args[0], &option))
				return 0;
			opts->fs_low_uid = option;
			break;
		case Opt_fsgid:
			if (match_int(&args[0], &option))
				return 0;
			opts->fs_low_gid = option;
			break;
		case Opt_gid:
			if (match_int(&args[0], &option))
				return 0;
			opts->gid = option;
			break;
		case Opt_userid:
			if (match_int(&args[0], &option))
				return 0;
			opts->fs_user_id = option;
			break;
		case Opt_mask:
			if (match_int(&args[0], &option))
				return 0;
			opts->mask = option;
			break;
		case Opt_multiuser:
			opts->multiuser = true;
			break;
		case Opt_fs_scontext:
			string_option = match_strdup(&args[0]);
			if (string_option == NULL)
				return -ENOMEM;

			if (!strcmp(string_option, "current")) {	/*lint !e421*/
				security_task_getsecid(current,
					&opts->fs_low_secid);
				option = 0;
			} else
				option = security_secctx_to_secid(string_option,
					strlen(string_option), &opts->fs_low_secid);
			kfree(string_option);

			if (unlikely(option < 0)) {
				if (!silent)
					errln("Invalid fs_scontext: %d",
						opts->fs_low_secid);
				return -EINVAL;
			}
			opts->has_fssecid = true;
			break;
		case Opt_reserved_mb:
			if (match_int(&args[0], &option))
				return 0;
			opts->reserved_mb = option;
			break;
		case Opt_quiet:
			opts->quiet = true;
			break;
		default:	/* unknown option */
			if (!silent)
				errln("Unrecognized mount option \"%s\" "
					"or missing value", p);
			return -EINVAL;
		}
	}
	return 0;
}

static int __sdcardfs_setup_root(
	struct dentry *root,
	struct dentry *realdir,
	userid_t userid,
	int multiuser
) {
	struct sdcardfs_tree_entry *te;
	struct inode *inode;

	/* link the upper and lower dentries */
	te = sdcardfs_init_tree_entry(root, realdir);
	if (te == NULL)
		return -ENOMEM;

	te->revision = get_next_revision(root);

	/* setup permission policy */
	te->userid = userid;
	te->d_uid = AID_ROOT;
	te->under_android = false;
	te->perm = multiuser ? PERM_PRE_ROOT : PERM_ROOT;

	inode = d_inode(root);

	/* if inode->i_version < te->revision,
	   uid/gid/mode will be updated at the right time */
	inode->i_version = te->revision;

	/* used for revalidate in inode_permission */
	inode->i_private = te;

	__fix_derived_permission(te, inode);
	return 0;
}

static inline
struct dentry *__prepare_dir(
	const char *path_s,
	mode_t mode, uid_t uid, gid_t gid)
{
	struct path parent;
	struct dentry *dent = kern_path_create(AT_FDCWD,
		path_s, &parent, LOOKUP_DIRECTORY);
	if (IS_ERR(dent)) {
		if (dent == ERR_PTR(-EEXIST))
			dent = NULL;
	} else {
		int err = vfs_mkdir(d_inode(parent.dentry), dent, mode);
		if (err) {
			done_path_create(&parent, dent);
			dent = ERR_PTR(err);
		} else {
			struct iattr attrs = {.ia_valid = ATTR_UID | ATTR_GID};
			attrs.ia_uid = make_kuid(&init_user_ns, uid);
			attrs.ia_gid = make_kgid(&init_user_ns, gid);

			inode_lock(d_inode(dent));
			notify_change(dent, &attrs, NULL);
			inode_unlock(d_inode(dent));

			BUG_ON(dent != dget(dent));
			done_path_create(&parent, dent);
		}
	}
	return dent;
}

static
struct dentry *prepare_dir(
	const char *path_s,
	mode_t mode, uid_t uid, gid_t gid)
{
	struct dentry *dir =
		__prepare_dir(path_s, mode, uid, gid);
	if (dir == NULL) {
		struct path path;
		int err = kern_path(path_s, LOOKUP_DIRECTORY, &path);

		if (err)
			dir = ERR_PTR(-ESTALE);
		else if (d_is_negative(path.dentry) ||
			!S_ISDIR(d_inode(path.dentry)->i_mode)) {
			path_put(&path);
			dir = ERR_PTR(-ENOTDIR);
		} else {
			dir = dget(path.dentry);
			path_put(&path);
		}
	}
	return dir;
}

/*lint -save -e578*/
/* There is no need to lock the sdcardfs_super_info's rwsem as there is no
   way anyone can have a reference to the superblock at this point in time. */
static int sdcardfs_read_super(struct super_block *sb,
	const char *dev_name, void *raw_data, int silent)
{
	int err = 0;
	int debug;
	struct super_block *lower_sb;
	struct path lower_path;
	struct sdcardfs_sb_info *sbi;
	struct inode *inode, *lower_inode;

	/* avoid WARN_ON in sdcardfs_kill_sb */
	sb->s_magic = SDCARDFS_SUPER_MAGIC;

	if (dev_name == NULL) {
		errln("%s, missing dev_name argument", __FUNCTION__);
		err = -EINVAL;
		goto out;
	}

	infoln("read_super, device -> %s", dev_name);
	infoln("options -> %s", (char *)raw_data);

	/* parse lower path */
	err = kern_path(dev_name, LOOKUP_FOLLOW | LOOKUP_DIRECTORY,
			&lower_path);
	if (err) {
		errln("%s, error accessing device '%s'", __FUNCTION__, dev_name);
		goto out;
	}

	lower_inode = d_inode(lower_path.dentry);
	if (!S_ISDIR(lower_inode->i_mode)) {
		errln("%s, device '%s' should be a directory",
			__FUNCTION__, dev_name);
		err = -EINVAL;
		goto out_free;
	}

	/* allocate superblock private data */
	sbi = kzalloc(sizeof(struct sdcardfs_sb_info), GFP_KERNEL);
	if (sbi == NULL) {
		critln("%s, out of memory", __FUNCTION__);
		err = -ENOMEM;
		goto out_free;
	}

	sb->s_fs_info = sbi;
	default_options(&sbi->options);

	/* parse options */
	err = sdcardfs_parse_options(&sbi->options, raw_data, silent, &debug);
	if (err) {
		errln("%s, invalid options", __FUNCTION__);
		goto out_freesbi;
	}

#ifdef SDCARDFS_SUPPORT_RESERVED_SPACE
	sbi->basepath = lower_path;
	path_get(&sbi->basepath);
#endif

	/* set the lower superblock field of upper superblock */
	sbi->lower_mnt = lower_path.mnt;
	lower_sb = lower_path.dentry->d_sb;
	BUG_ON(lower_path.mnt->mnt_sb != lower_sb);

	sbi->next_revision = 1;

	/* increment lower_sb active references */
	atomic_inc(&lower_sb->s_active);

	/* inherit maxbytes from lower file system */
	sb->s_maxbytes = lower_sb->s_maxbytes;

	/* Our c/m/atime granularity is 1 ns because we may stack on file
	   systems whose granularity is as good. */
	sb->s_time_gran = 1;
	sb->s_op = &sdcardfs_sops;
	sb->s_d_op = &sdcardfs_ci_dops;

	/* get a inode and allocate our root dentry */
	inode = sdcardfs_ialloc(sb, lower_inode->i_mode);
	if (IS_ERR(inode)) {
		err = PTR_ERR(inode);
		goto out_sput;
	}

	/* make the sdcardfs root dentry */
	sb->s_root = d_make_root(inode);
	if (sb->s_root == NULL) {
		err = -ENOMEM;
		goto out_iput;
	}

	/* setup tree entry for root dentry */
	err = __sdcardfs_setup_root(sb->s_root, lower_path.dentry,
		sbi->options.fs_user_id, sbi->options.multiuser);
	if (err)
		goto out_freeroot;

	/* save obbpath(devpath) to sbi */
	sbi->devpath_s = __getname();
	if (sbi->devpath_s == NULL) {
		err = -ENOMEM;
		goto out_freetreeentry;
	}

	snprintf(sbi->devpath_s, PATH_MAX, "%s", dev_name);
	sbi->devpath_s[PATH_MAX - 1] = '\0';

#ifdef SDCARDFS_PLUGIN_PRIVACY_SPACE
	sbi->blocked_userid = sbi->appid_excluded = -1;
#endif

#ifdef SDCARDFS_SYSFS_FEATURE
	/* use kobject_unregister instread of kfree to free sbi after succeed */
	err = sdcardfs_sysfs_register_sb(sb);
	if (err)
		goto out_putname;
#endif

	/* prepare fs_struct */
	sbi->override_fs = prepare_fs_struct(&lower_path, 0);
	if (sbi->override_fs == NULL) {
		err = -ENOMEM;
		goto out_putname;
	}

	if (sbi->options.has_fssecid) {
		sbi->sdcardd_cred = prepare_creds();
		if (sbi->sdcardd_cred == NULL) {
			errln("%s, failed to prepare creds "
				"in order to override secid", __FUNCTION__);
			sbi->options.has_fssecid = false;
		}
	}

	if (!sbi->options.multiuser) {
		sbi->shared_obb = NULL;
	} else {
		struct dentry *dir;
		struct fs_struct *saved_fs = override_current_fs(sbi->override_fs);
		dir = prepare_dir("obb", 0775, sbi->options.fs_low_uid,
			sbi->options.fs_low_gid);
		revert_current_fs(saved_fs);

		if (IS_ERR(dir)) {
			err = PTR_ERR(dir);
			goto out_freefsstruct;
		}

		sbi->shared_obb = dir;
	}

	/* No need to call interpose because we already have
	   a positive dentry, which was instantiated by
	   d_make_root. Just need to d_rehash it. */
	d_rehash(sb->s_root);

	if (!silent)
		infoln("mounted on top of %s type %s", dev_name,
			lower_sb->s_type->name);
	goto out;

out_freefsstruct:
	free_fs_struct(sbi->override_fs);
out_putname:
	__putname(sbi->devpath_s);
out_freetreeentry:
	/* because dput_final will go into d_release, so it is no need
	   to call sdcardfs_free_tree_entry(sb->s_root) explicitly; */
	dget(lower_path.dentry);
out_freeroot:
	dput(sb->s_root);
out_iput:
	if (sb->s_root == NULL)
		iput(inode);
out_sput:
	/* drop refs we took earlier */
	atomic_dec(&lower_sb->s_active);
#ifdef SDCARDFS_SUPPORT_RESERVED_SPACE
	_path_put(&sbi->basepath);
#endif
out_freesbi:
	/* it ensures the right behavior in sdcardfs_put_super */
	sb->s_fs_info = NULL;
#ifdef SDCARDFS_SYSFS_FEATURE
	if (sbi->kobj.state_initialized)
		kobject_put(&sbi->kobj);
	else
#endif
		kfree(sbi);
out_free:
	_path_put(&lower_path);
out:
	return err;
}

struct _sdcardfs_mount_private {
	const char *dev_name;
	char *options;
};

/* A feature which supports mount_nodev() with options */
static int __sdcardfs_fill_super(struct super_block *sb,
	void *_priv, int silent)
{
	struct _sdcardfs_mount_private *priv = _priv;

	return sdcardfs_read_super(sb, priv->dev_name,
		priv->options, silent);
}

static struct dentry *
sdcardfs_mount(struct file_system_type *fs_type, int flags,
	const char *dev_name, void *raw_data)
{
	struct _sdcardfs_mount_private priv = {
		.dev_name = dev_name,
		.options = raw_data
	};

	return mount_nodev(fs_type, flags,
		(void *)&priv, __sdcardfs_fill_super);
}
/*lint -restore*/

static void sdcardfs_kill_sb(struct super_block *sb)
{
	WARN_ON(sb->s_magic != SDCARDFS_SUPER_MAGIC);

	kill_anon_super(sb);
}

static struct file_system_type sdcardfs_fs_type = {
	.owner		= THIS_MODULE,
	.name		= SDCARDFS_NAME,
	.mount		= sdcardfs_mount,
	.kill_sb	= sdcardfs_kill_sb,
	.fs_flags	= 0,
};

static int __init init_sdcardfs_fs(void)
{
	int err;

	infoln("initialize HUAWEI sdcardfs v" SDCARDFS_VERSION);

	err = sdcardfs_init_tree_cache();
	if (err)
		goto err;

	err = sdcardfs_configfs_init();
	if (err)
		goto err_tree;

#ifdef SDCARDFS_SYSFS_FEATURE
	err = sdcardfs_sysfs_init();
	if (err)
		goto err_configfs;
#endif

	err = register_filesystem(&sdcardfs_fs_type);
	if (!err)
		return 0;

#ifdef SDCARDFS_SYSFS_FEATURE
	sdcardfs_sysfs_exit();
err_configfs:
#endif
	sdcardfs_configfs_exit();
err_tree:
	sdcardfs_destroy_tree_cache();
err:
	return err;
}

static void __exit exit_sdcardfs_fs(void)
{
	unregister_filesystem(&sdcardfs_fs_type);
#ifdef SDCARDFS_SYSFS_FEATURE
	sdcardfs_sysfs_exit();
#endif
	sdcardfs_configfs_exit();
	sdcardfs_destroy_tree_cache();
	infoln("finalize sdcardfs module successfully\n");
}

MODULE_AUTHOR("Gao Xiang <gaoxiang25@huawei.com>, CUSTOMER BG, HUAWEI inc.");
MODULE_DESCRIPTION("HUAWEI Sdcardfs " SDCARDFS_VERSION);
MODULE_LICENSE("GPL");

module_init(init_sdcardfs_fs);
module_exit(exit_sdcardfs_fs);
