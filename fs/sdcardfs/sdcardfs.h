/* vim:set ts=4 sw=4 tw=0 noet ft=c:
 *
 * fs/sdcardfs/sdcardfs.h
 *
 * Copyright (C) 2017 HUAWEI, Inc.
 * Author: gaoxiang <gaoxiang25@huawei.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of the Linux
 * distribution for more details.
 */
#ifndef __SDCARDFS_H
#define __SDCARDFS_H

#include <linux/dcache.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/aio.h>
#include <linux/mm.h>
#include <linux/mount.h>
#include <linux/namei.h>
#include <linux/seq_file.h>
#include <linux/statfs.h>
#include <linux/fs_stack.h>
#include <linux/magic.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/security.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/fs_struct.h>
#include <linux/ratelimit.h>
#ifdef SDCARDFS_SYSFS_FEATURE
#include <linux/kobject.h>
#endif

#include "multiuser.h"
#include "macros.h"

/* the file system magic number */
#define SDCARDFS_SUPER_MAGIC	0x5dca2df5

/* the file system name */
#define SDCARDFS_NAME "sdcardfs"

#undef pr_fmt
#define pr_fmt(fmt)	SDCARDFS_NAME ": " fmt

#define debugln(fmt, ...)   pr_debug(fmt "\n", ##__VA_ARGS__)
#define infoln(fmt, ...)    pr_info(fmt "\n", ##__VA_ARGS__)
#define warnln(fmt, ...)    pr_warn(fmt "\n", ##__VA_ARGS__)
#define errln(fmt, ...)     pr_err(fmt "\n", ##__VA_ARGS__)
#define critln(fmt, ...)    pr_crit(fmt "\n", ##__VA_ARGS__)

#define infoln_ratelimited(fmt, ...)	pr_info_ratelimited(fmt "\n", ##__VA_ARGS__)

#include "android_filesystem_config.h"

static inline
struct fs_struct *override_current_fs(
	struct fs_struct *fs
) {
	struct fs_struct *saved_fs;

	task_lock(current);
	saved_fs = current->fs;
	current->fs = fs;
	task_unlock(current);
	return saved_fs;
}

static inline
void __free_fs_struct(struct fs_struct *fs)
{
	path_put(&fs->root);
	path_put(&fs->pwd);

	kfree(fs);
}

#define revert_current_fs	override_current_fs
#define free_fs_struct	__free_fs_struct

/* OVERRIDE_CRED() and REVERT_CRED()
 *  - OVERRIDE_CRED()
 *      backup original task->cred
 *      and modifies task->cred->fsuid/fsgid to specified value.
 *  - REVERT_CRED()
 *      restore original task->cred->fsuid/fsgid.
 * Remember that these two macros should be used in pair */

#define OVERRIDE_CRED(sdcardfs_sbi, saved_cred) (saved_cred = override_fsids(sdcardfs_sbi))
#define REVERT_CRED(saved_cred)	revert_fsids(saved_cred)

/* Android 6.0 runtime permissions model */

/* Permission mode for a specific node. Controls how file permissions
 * are derived for children nodes. */
typedef enum {
    /* Nothing special; this node should just inherit from its parent. */
    PERM_INHERIT,
    /* This node is one level above a normal root; used for legacy layouts
     * which use the first level to represent user_id. */
    PERM_PRE_ROOT,
    /* This node is "/" */
    PERM_ROOT,
    /* This node is "/Android" */
    PERM_ANDROID,
    /* This node is "/Android/data" */
    PERM_ANDROID_DATA,
    /* This node is "/Android/obb" */
    PERM_ANDROID_OBB,
    /* This node is "/Android/media" */
    PERM_ANDROID_MEDIA,
} perm_t;

struct sdcardfs_sb_info;
struct sdcardfs_mount_options;

/* derived_perm.c */
const struct cred *override_fsids(struct sdcardfs_sb_info *sbi);
void revert_fsids(const struct cred *old_cred);

extern struct fs_struct *prepare_fs_struct(struct path *, int);

/* operations defined in specific files */
extern const struct file_operations sdcardfs_main_fops, sdcardfs_dir_fops;
extern const struct inode_operations sdcardfs_main_iops, sdcardfs_dir_iops;
extern const struct super_operations sdcardfs_sops;
extern const struct dentry_operations sdcardfs_ci_dops;
extern const struct address_space_operations sdcardfs_aops;
extern const struct vm_operations_struct sdcardfs_vm_ops;

/* lookup_ci.c */
extern char *sdcardfs_do_lookup_ci_begin(struct vfsmount *,
	struct dentry *, struct qstr *, bool);
extern void sdcardfs_do_lookup_ci_end(char *);

/* namei.c */
extern struct dentry *sdcardfs_lookup(struct inode *dir,
	struct dentry *dentry, unsigned int flags);

/* used for lookup/create/mkdir */
extern struct dentry *sdcardfs_interpose(struct dentry *,
	struct dentry *, struct dentry *);

/* inode.c */
extern struct inode *sdcardfs_ialloc(struct super_block *, mode_t);
extern int touch_nomedia(struct dentry *parent);

/* tree.c */
extern int sdcardfs_init_tree_cache(void);
extern void sdcardfs_destroy_tree_cache(void);

/* xattr.c */
extern int sdcardfs_setxattr(struct dentry *dentry,
	const char *name, const void *value, size_t size, int flags);
extern ssize_t sdcardfs_getxattr(struct dentry *dentry,
	const char *name, void *value, size_t size);
extern ssize_t sdcardfs_listxattr(struct dentry *dentry,
	char *list, size_t size);
extern int sdcardfs_removexattr(struct dentry *dentry, const char *name);

/* valid file mode bits */
#define SDCARDFS_VALID_MODE	(S_IFREG | S_IFDIR | S_IRWXUGO)

/* file private data */
struct sdcardfs_file_info {
	struct file *lower_file;
	const struct vm_operations_struct *lower_vm_ops;
};

struct sdcardfs_mount_options {
	uid_t fs_low_uid;
	gid_t fs_low_gid;
	userid_t fs_user_id;
	gid_t gid;
	mode_t mask;
	bool multiuser;
	/* used for overiding secid */
	bool has_fssecid;
	/* set = fake successful chmods and chowns */
	bool quiet;
	u32 fs_low_secid;
	unsigned int reserved_mb;
};

/* sdcardfs super-block data in memory */
struct sdcardfs_sb_info {
	struct vfsmount *lower_mnt;
	/* derived perm policy : some of options have been added
	 * to sdcardfs_mount_options (Android 4.4 support) */
	struct sdcardfs_mount_options options;
	char *devpath_s;
#ifdef SDCARDFS_SUPPORT_RESERVED_SPACE
	struct path basepath;
#endif
	struct dentry *shared_obb;

	struct fs_struct *override_fs;
	struct cred *sdcardd_cred;

#ifdef SDCARDFS_SYSFS_FEATURE
	struct kobject kobj;
#endif

#ifdef SDCARDFS_PLUGIN_PRIVACY_SPACE
	int blocked_userid, appid_excluded;
#endif

	unsigned next_revision;
};

/* superblock to private data */
#define SDCARDFS_SB(super) ((struct sdcardfs_sb_info *)(super)->s_fs_info)

#define get_next_revision(d) (SDCARDFS_SB((d)->d_sb)->next_revision++)

/* superblock to lower superblock */
static inline struct super_block *sdcardfs_lower_super(
	const struct super_block *sb)
{
	struct sdcardfs_sb_info *sbi = SDCARDFS_SB(sb);

	BUG_ON(sbi->lower_mnt == NULL);
	return sbi->lower_mnt->mnt_sb;
}

struct sdcardfs_tree_entry {
	rwlock_t lock;

	struct dentry *ovl;
	struct {
		struct dentry *dentry;

		/* if sdcardfs_disconnected, use ino/genertion pair
		   to connect real again. */
		unsigned long	ino;
		__u32 generation;

		bool dentry_invalid;
		unsigned d_seq;
	} real;

	/* a number used to decide whether permissions
	   should be updated from the parent when revalidated */
	unsigned revision;

	/* state derived based on current position in hierachy */
	perm_t perm;
	userid_t userid;
	uid_t d_uid;
	bool under_android;
};

#define SDCARDFS_DI_LOCKED		((void *)0x19921118)

/* keep in mind SDCARDFS_D is unsafe for RCU lookup */
static inline
struct sdcardfs_tree_entry *SDCARDFS_D(
	const struct dentry *d
) {
	struct sdcardfs_tree_entry *te;

	while ((te = (struct sdcardfs_tree_entry *)
		lockless_dereference(d->d_fsdata)) == SDCARDFS_DI_LOCKED)
		cpu_relax();
	return te;
}

static inline
struct sdcardfs_tree_entry *SDCARDFS_DI_X(
	const struct dentry *_d
) {
	struct dentry *d = (struct dentry *)_d;
	struct sdcardfs_tree_entry *te;

	while ((te = (struct sdcardfs_tree_entry *)xchg(
		&d->d_fsdata, SDCARDFS_DI_LOCKED)) == SDCARDFS_DI_LOCKED)
		cpu_relax();

	smp_mb();
	ACCESS_ONCE(d->d_fsdata) = NULL;
	if (te != NULL)
		write_lock(&te->lock);
	return te;
}

/* acquire tree entry with te read locked */
static inline
struct sdcardfs_tree_entry *SDCARDFS_DI_R(
	const struct dentry *_d
) {
	struct dentry *d = (struct dentry *)_d;
	struct sdcardfs_tree_entry *te;

	while ((te = (struct sdcardfs_tree_entry *)xchg(
		&d->d_fsdata, SDCARDFS_DI_LOCKED)) == SDCARDFS_DI_LOCKED)
		cpu_relax();
	BUG_ON(te == NULL);
	read_lock(&te->lock);
	return ACCESS_ONCE(d->d_fsdata) = te;
}

/* acquire tree entry with te write locked */
static inline
struct sdcardfs_tree_entry *SDCARDFS_DI_W(
	const struct dentry *_d
) {
	struct dentry *d = (struct dentry *)_d;
	struct sdcardfs_tree_entry *te;

	while ((te = (struct sdcardfs_tree_entry *)xchg(
		&d->d_fsdata, SDCARDFS_DI_LOCKED)) == SDCARDFS_DI_LOCKED)
		cpu_relax();
	BUG_ON(te == NULL);
	write_lock(&te->lock);
	return ACCESS_ONCE(d->d_fsdata) = te;
}

extern struct dentry *
_sdcardfs_reactivate_real_locked(const struct dentry *dentry,
	struct sdcardfs_tree_entry *te);

static inline
struct sdcardfs_tree_entry *
sdcardfs_tree_entry_real_locked(
	const struct dentry *dentry,
	struct dentry **real_dentry
) {
	struct sdcardfs_tree_entry *te = SDCARDFS_DI_R(dentry);
	*real_dentry = (!te->real.dentry_invalid ? te->real.dentry :
		_sdcardfs_reactivate_real_locked(dentry, te));
	return te;
}

static inline
struct sdcardfs_tree_entry *
sdcardfs_tree_entry_lower_locked(
	const struct dentry *dentry,
	struct dentry **lower_dentry
) {
	struct sdcardfs_tree_entry *te = SDCARDFS_DI_R(dentry);
	*lower_dentry = (te->ovl != NULL ? te->ovl :
		(!te->real.dentry_invalid ? te->real.dentry :
			_sdcardfs_reactivate_real_locked(dentry, te)));
	return te;
}

static inline
struct sdcardfs_tree_entry *
sdcardfs_real_dentry_rcu_locked(
	struct dentry *d,
	struct dentry **real_dentry
) {
	/* it is needed to treat differently for RCU approach.
	 * because countref isnt taken and free_tree_entry
	 * could be triggered at the same time */
	struct sdcardfs_tree_entry *te;

	while ((te = (struct sdcardfs_tree_entry *)xchg(
		&d->d_fsdata, SDCARDFS_DI_LOCKED)) == SDCARDFS_DI_LOCKED)
		cpu_relax();

	if (te != NULL) {
		read_lock(&te->lock);
		*real_dentry = (te->real.dentry_invalid ? NULL :
			te->real.dentry);
	}
	ACCESS_ONCE(d->d_fsdata) = te;
	return te;
}

static inline
struct dentry *sdcardfs_get_real_dentry_with_seq(
	const struct dentry *dentry,
	unsigned *d_seq
) {
	struct sdcardfs_tree_entry *te;
	struct dentry *real_dentry;

	te = sdcardfs_tree_entry_real_locked(dentry, &real_dentry);
	real_dentry = dget(real_dentry);
	*d_seq = te->real.d_seq;
	read_unlock(&te->lock);

	return real_dentry;
}

static inline
struct dentry *sdcardfs_get_real_dentry(
	const struct dentry *dentry
) {
	unsigned d_seq;

	return sdcardfs_get_real_dentry_with_seq(
		dentry, &d_seq);
}

static inline
struct dentry *sdcardfs_get_lower_dentry(
	const struct dentry *dentry
) {
	struct sdcardfs_tree_entry *te;
	struct dentry *lower_dentry;

	te = sdcardfs_tree_entry_lower_locked(dentry, &lower_dentry);
	lower_dentry = dget(lower_dentry);
	read_unlock(&te->lock);

	return lower_dentry;
}

extern struct sdcardfs_tree_entry *
	sdcardfs_init_tree_entry(struct dentry *, struct dentry *);
extern void sdcardfs_free_tree_entry(struct dentry *);

/* need fixup its permission from its parent */
static inline int need_fixup_permission(
	struct inode *dir,
	struct sdcardfs_tree_entry *te
) {
	return te->revision < dir->i_version;
}

static inline void __fix_derived_permission(
	struct sdcardfs_tree_entry *te,
	struct inode *inode
) {
#if defined(SDCARDFS_DEBUG_UID) && defined(SDCARDFS_DEBUG_GID)
	inode->i_uid = make_kuid(&init_user_ns, SDCARDFS_DEBUG_UID);
	inode->i_gid = make_kgid(&init_user_ns, SDCARDFS_DEBUG_GID);
#else
	struct sdcardfs_mount_options *opts =
		&SDCARDFS_SB(inode->i_sb)->options;
	int visible_mode, owner_mode, filtered_mode;

	inode->i_uid = make_kuid(&init_user_ns, te->d_uid);

	if (opts->gid == AID_SDCARD_RW) {
		/* As an optimization, certain trusted system components only run
		 * as owner but operate across all users. Since we're now handing
		 * out the sdcard_rw GID only to trusted apps, we're okay relaxing
		 * the user boundary enforcement for the default view. The UIDs
		 * assigned to app directories are still multiuser aware. */
		inode->i_gid = make_kgid(&init_user_ns, AID_SDCARD_RW);
	} else {
		inode->i_gid = make_kgid(&init_user_ns,
			multiuser_get_uid(te->userid, opts->gid));
	}

	/* sdcard storage has its "i_mode" derived algorithm */
	visible_mode = 0775 & ~opts->mask;

	if (te->perm == PERM_PRE_ROOT) {
		/* Top of multi-user view should always be visible to ensure
		 * secondary users can traverse inside. */
		visible_mode = 0711;
	} else if (te->under_android) {
		/* Block "other" access to Android directories, since only apps
		 * belonging to a specific user should be in there; we still
		 * leave +x open for the default view. */
		if (opts->gid == AID_SDCARD_RW)
			visible_mode = visible_mode & ~0006;
		else
			visible_mode = visible_mode & ~0007;
	}
	owner_mode = inode->i_mode & 0700;
	filtered_mode = visible_mode & (owner_mode | (owner_mode >> 3) | (owner_mode >> 6));
	inode->i_mode = (inode->i_mode & S_IFMT) | filtered_mode;

	inode->i_version = te->revision;
#endif
}

static inline void fix_derived_permission(
	struct dentry *dentry
) {
	struct sdcardfs_tree_entry *te = SDCARDFS_DI_R(dentry);

	__fix_derived_permission(te, d_inode(dentry));
	read_unlock(&te->lock);
}

/* file to private Data */
#define SDCARDFS_F(file) ((struct sdcardfs_file_info *)((file)->private_data))

/* file to lower file */
#define sdcardfs_lower_file(f)	(SDCARDFS_F(f)->lower_file)

static inline int sdcardfs_get_lower_path(
	const struct dentry *dentry, struct path *path
) {
	path->dentry = sdcardfs_get_lower_dentry(dentry);
	if (path->dentry != NULL) {
		path->mnt = mntget(SDCARDFS_SB(dentry->d_sb)->lower_mnt);
		debugln("%s, dentry=%p, lower_path(dentry=%p, mnt=%p)",
			__FUNCTION__, dentry, path->dentry, path->mnt);
		return 0;
	}
	debugln("%s, dentry=%p, lower_path(null)", __FUNCTION__, dentry);
	return -1;
}

static inline void _path_put(const struct path *path)
{
	debugln("%s, lower_path(dentry=%p, mnt=%p)",
		__FUNCTION__, path->dentry, path->mnt);

	path_put(path);
}

/* main.c */
int sdcardfs_parse_options(struct sdcardfs_mount_options *, char *, int, int *);

/* packagelist.c */
extern appid_t get_appid(const char *app_name);

extern struct sdcardfs_packagelist_entry *sdcardfs_packagelist_entry_alloc(void);

extern void sdcardfs_packagelist_entry_register(struct sdcardfs_packagelist_entry *pkg,
	const char *app_name, appid_t appid);
extern void sdcardfs_packagelist_entry_release(struct sdcardfs_packagelist_entry *);

/* configfs.c */
extern int sdcardfs_configfs_init(void);
extern void sdcardfs_configfs_exit(void);

/* sysfs.c */
#ifdef SDCARDFS_SYSFS_FEATURE
extern int sdcardfs_sysfs_init(void);
extern void sdcardfs_sysfs_exit(void);
extern int sdcardfs_sysfs_register_sb(struct super_block *);
#endif

/* derived_perm.c */
extern int check_caller_access_to_name(struct dentry *, const char *);

extern void get_derived_permission4(struct dentry *,
	struct dentry *, const char *, bool);
extern void get_derived_permission(struct dentry *, struct dentry *);

#ifdef SDCARDFS_SUPPORT_RESERVED_SPACE
/* Return 1, if a disk has enough free space, otherwise 0.
 * We assume that any files can not be overwritten. */
static inline int check_min_free_space(struct super_block *sb,
	size_t size, int isdir)
{
	struct sdcardfs_sb_info *sbi = SDCARDFS_SB(sb);

	if (sbi->options.reserved_mb) {
		int err;
		struct kstatfs statfs;
		u64 avail;

		err = vfs_statfs(&sbi->basepath, &statfs);
		if (unlikely(err)) {
out_invalid:
			infoln("statfs               : invalid return");
			infoln("vfs_statfs error#    : %d", err);
			infoln("statfs.f_type        : 0x%X", (u32)statfs.f_type);
			infoln("statfs.f_blocks      : %llu blocks", statfs.f_blocks);
			infoln("statfs.f_bfree       : %llu blocks", statfs.f_bfree);
			infoln("statfs.f_files       : %llu", statfs.f_files);
			infoln("statfs.f_ffree       : %llu", statfs.f_ffree);
			infoln("statfs.f_fsid.val[1] : 0x%X", (u32)statfs.f_fsid.val[1]);
			infoln("statfs.f_fsid.val[0] : 0x%X", (u32)statfs.f_fsid.val[0]);
			infoln("statfs.f_namelen     : %ld", statfs.f_namelen);
			infoln("statfs.f_frsize      : %ld", statfs.f_frsize);
			infoln("statfs.f_flags       : %ld", statfs.f_flags);
			infoln("sdcardfs reserved_mb : %u", sbi->options.reserved_mb);
			if (sbi->devpath_s != NULL)
				infoln("sdcardfs dev_name    : %s", sbi->devpath_s);
out_nospc:
			infoln_ratelimited("statfs.f_bavail : %llu blocks / "
				"statfs.f_bsize : %ld bytes / "
				"required size : %llu byte",
				statfs.f_bavail, statfs.f_bsize, (u64)size);
			return 0;
		}

		/* Invalid statfs informations. */
		if (unlikely(!statfs.f_bsize))
			goto out_invalid;

		/* if you are checking directory, set size to f_bsize. */
		if (isdir)
			size = statfs.f_bsize;

		/* available size */
		avail = statfs.f_bavail * statfs.f_bsize;

		/* not enough space */
		if ((u64)size > avail ||
			(avail - size) < (u64)sbi->options.reserved_mb << 20)
			goto out_nospc;
	}
	/* enough space */
	return 1;
}
#endif

/* Copies attrs and maintains sdcardfs managed attrs */
static inline void sdcardfs_copy_and_fix_attrs(struct inode *dest, const struct inode *src)
{
	dest->i_rdev = src->i_rdev;
	dest->i_atime = src->i_atime;
	dest->i_mtime = src->i_mtime;
	dest->i_ctime = src->i_ctime;
	dest->i_blkbits = src->i_blkbits;
	dest->i_flags = src->i_flags;
	set_nlink(dest, src->i_nlink);
}
#endif	/* not _SDCARDFS_H_ */
