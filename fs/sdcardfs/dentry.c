/* vim:set ts=4 sw=4 tw=0 noet ft=c:
 *
 * fs/sdcardfs/dentry.c
 *
 * Copyright (C) 2017 HUAWEI, Inc.
 * Author: gaoxiang <gaoxiang25@huawei.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of the Linux
 * distribution for more details.
 */
#include "sdcardfs.h"
#include "linux/ctype.h"
#include <linux/version.h>

#include "trace-events.h"

static inline void dentry_rcuwalk_barrier(struct dentry *dentry)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0))
	assert_spin_locked(&dentry->d_lock);
	/* Go through a barrier */
	write_seqcount_barrier(&dentry->d_seq);
#else
	lockdep_assert_held(&dentry->d_lock);
	/* Go through am invalidation barrier */
	write_seqcount_invalidate(&dentry->d_seq);
#endif
}

/* locking order:
 *     dentry->d_lock
 *         SDCARDFS_DI_LOCK
 *             te->lock */

/*lint -save -e454 -e455 -e456*/
static int sdcardfs_d_delete(const struct dentry *dentry)
{
	struct sdcardfs_tree_entry *te;
	struct dentry *real_dentry;
	struct inode *real_inode;
	int ret = 1;	/* kill it by default */

	trace_sdcardfs_d_delete_enter(dentry);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 12, 0))
	/* only for Linux versions which "vfs: reorganize dput()
	   memory accesses" from Linus is not applied */
	if (unlikely(d_unhashed(dentry)))
		goto out;
#endif

	te = SDCARDFS_DI_W(dentry);
	/* since dentry is hashed, there is no way that te == NULL */
	BUG_ON(te == NULL);

	/* Since d_delete can be "deleted" for many times,
	 * it may break in just after d_revalidate
	 *
	 * lookup_fast
	 *                                 dput
	 *                                      d_delete   <--- invalid
	 *     d_revalidate_fast   <--- -ECHILD
	 *     unlazy_walk
	 *           dget
	 *           dput
	 *                d_delete <---- already invalid */
	if (te->real.dentry_invalid == true) {
		ret = 0;	/* maybe better than kill it */
		goto out_unlock;
	}

	real_dentry = te->real.dentry;
	BUG_ON(real_dentry == NULL);

	/* some dentry interposed with unhashed dentry, we should kill it */
	spin_lock(&real_dentry->d_lock);
	if (d_unhashed(real_dentry)) {
		spin_unlock(&real_dentry->d_lock);
		goto out_unlock;
	}
	real_inode = d_inode(real_dentry);
	spin_unlock(&real_dentry->d_lock);

	/* including unlink/rmdir(d_delete), rename(d_move) ... */
	if (!__read_seqcount_retry(&real_dentry->d_seq,
		te->real.d_seq)) {
		struct dentry *cast = (struct dentry *)dentry;
		/* hashed and positive, deactivate the dentry! */
		te->real.ino = real_inode->i_ino;
		te->real.generation = real_inode->i_generation;

		debugln("%s, dentry=%p (ino=%lu, gen=%u)", __FUNCTION__, dentry,
			te->real.ino, te->real.generation);

		te->real.dentry_invalid = true;

		/* since real_dentry is invalid, we should prevent
		   the dentry revalidated in the lookup_fast path */
		dentry_rcuwalk_barrier(cast);
		write_unlock(&te->lock);

		/* it's ok...safe to unlock & lock d_lock again */
		spin_unlock(&cast->d_lock);

		/* dput may be blocked, so take it out of the locks */
		dput(real_dentry);

		spin_lock(&cast->d_lock);

		ret = likely(d_count(dentry) == 1) ?
			/* we need to check again whether it is unreachable now. */
			d_unhashed(dentry) : 0;
		goto out;
	}

out_unlock:
	write_unlock(&te->lock);
out:
	trace_sdcardfs_d_delete_exit(dentry, ret);
	return ret;
}
/*lint -restore*/

/* d_revalidate only focus on revalidating the real dentry.
   because we assume that the ovldentry cannot be d_drop. */
static int __sdcardfs_d_revalidate_fast(
	struct dentry *dentry,
	unsigned int flags
) {
	struct sdcardfs_tree_entry *te;
	struct dentry *real_dentry;
	int err = 1;

	trace_sdcardfs_d_revalidate_fast_enter(dentry, flags);

	/* if dentry_unlink_inode() before, should invalidate it (differ
	   from VFS). think why we needn't considering after :) */
	if (unlikely(!d_inode_rcu(dentry))) {
		err = 0;
		goto out;
	}

	te = sdcardfs_real_dentry_rcu_locked(dentry, &real_dentry);

	/* hashed but without te...hmm, reclaiming */
	if (te == NULL) {
		err = 0;
		goto out;
	}

	if (real_dentry == NULL) {
		/* fall back to ref-walk mode, then reactivate it */
out_refwalk:
		trace_sdcardfs_d_revalidate_fast_refwalk(dentry, flags);
		err = -ECHILD;
		goto out_unlock;
	}

	/* if real_dentry was hashed,
	   it will remain hashed iff d_seq isnt changed. */
	if (__read_seqcount_retry(&real_dentry->d_seq,
		te->real.d_seq)) {
		/* we cannot confirm the following case, add a WARN_ON to notice that */
		WARN_ON(IS_ROOT(dentry));

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 0))
		err = 0;		/* dentry invalid */
		goto out_unlock;
#else
		err = -ENOENT;
		read_unlock(&te->lock);
		d_invalidate(dentry);
		goto out;
#endif
	}

	if (need_fixup_permission(
		d_inode_rcu(ACCESS_ONCE(dentry->d_parent)),	te)) {
		/* XXX: it's not suitble for us
		   to get_derived_permission in RCU lookup now :-( */
		goto out_refwalk;
	}

	if ((real_dentry->d_flags & DCACHE_OP_REVALIDATE)) {
		err = real_dentry->d_op->d_revalidate(real_dentry, flags);
		if (err < 0)
			goto out_unlock;

		/* follow overlayfs?
		   give a chance and fall back to ref-walk */
		else if (err == 0)
			goto out_refwalk;
	}

out_unlock:
	read_unlock(&te->lock);
out:
	trace_sdcardfs_d_revalidate_fast_exit(dentry, flags, err);
	return err;
}

static int __sdcardfs_d_revalidate_slow(
	struct dentry *dentry,
	unsigned int flags
) {
	struct dentry *parent, *real_dentry;
	unsigned seq;
	int ret = 0;

	trace_sdcardfs_d_revalidate_slow_enter(dentry, flags);

	/* root should always walk into __sdcardfs_d_revalidate_fast */
	BUG_ON(IS_ROOT(dentry));

	/* ref-walk takes a refcount so nothing to worry about */
	if (d_really_is_negative(dentry))
		goto out;

	real_dentry = sdcardfs_get_real_dentry_with_seq(dentry, &seq);
	if (real_dentry == NULL) {
		trace_sdcardfs_d_revalidate_slow_miss(dentry, flags);
		goto out;
	}

	ret = 1;
	if (real_dentry->d_flags & DCACHE_OP_REVALIDATE) {
		ret = real_dentry->d_op->d_revalidate(real_dentry, flags);
		if (ret <= 0)
			goto out_dput;
	}

retry:
	parent = dget_parent(dentry);
	if (need_fixup_permission(d_inode(parent), SDCARDFS_D(dentry)))
		get_derived_permission(parent, dentry);
	dput(parent);

	if (parent != ACCESS_ONCE(dentry->d_parent))
		goto retry;

	/* check if the hierarchy of this dentry was changed */
	if (unlikely(__read_seqcount_retry(&real_dentry->d_seq, seq))) {
		/* we cannot confirm the following case, add a WARN_ON to notice that */
		WARN_ON(IS_ROOT(dentry));
		ret = 0;
	}

out_dput:
	dput(real_dentry);
out:
	trace_sdcardfs_d_revalidate_slow_exit(dentry, flags, ret);
	return ret;
}

/* return value: -ERRNO if error (returned to user)
 * 0: tell VFS to invalidate dentry
 * 1: dentry is valid */
static int sdcardfs_d_revalidate(
	struct dentry *dentry,
	unsigned int flags
) {
	if (flags & LOOKUP_RCU)
		return __sdcardfs_d_revalidate_fast(dentry, flags);
	return __sdcardfs_d_revalidate_slow(dentry, flags);
}

/* __sdcardfs_d_release is an alias of sdcardfs_free_tree_entry */
#define sdcardfs_d_release	sdcardfs_free_tree_entry

const struct dentry_operations sdcardfs_ci_dops = {
	.d_delete	= sdcardfs_d_delete,
	.d_revalidate	= sdcardfs_d_revalidate,
	.d_release	= sdcardfs_d_release
};

