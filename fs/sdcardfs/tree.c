/* vim:set ts=4 sw=4 tw=0 noet ft=c:
 *
 * fs/sdcardfs/tree.c
 *
 * Copyright (C) 2017 HUAWEI, Inc.
 * Author: gaoxiang <gaoxiang25@huawei.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of the Linux
 * distribution for more details.
 */
#include "sdcardfs.h"

/* The tree cache is just so we have properly sized dentries */
static struct kmem_cache *sdcardfs_tree_entry_cachep;

int sdcardfs_init_tree_cache(void)
{
	sdcardfs_tree_entry_cachep = kmem_cache_create("sdcardfs_tree_entry",
		sizeof(struct sdcardfs_tree_entry),
		0, SLAB_RECLAIM_ACCOUNT, NULL);
	return sdcardfs_tree_entry_cachep != NULL ? 0 : -ENOMEM;
}

void sdcardfs_destroy_tree_cache(void)
{
	BUG_ON(sdcardfs_tree_entry_cachep == NULL);
	kmem_cache_destroy(sdcardfs_tree_entry_cachep);
}

struct sdcardfs_tree_entry *
sdcardfs_init_tree_entry(struct dentry *dentry,
	struct dentry *real)
{
	struct sdcardfs_tree_entry *te =
		kmem_cache_zalloc(sdcardfs_tree_entry_cachep, GFP_KERNEL);

	if (te == NULL)
		return NULL;
	te->real.d_seq = __read_seqcount_begin(&real->d_seq);
	te->real.dentry = real;
	rwlock_init(&te->lock);

	smp_wmb();
	ACCESS_ONCE(dentry->d_fsdata) = te;
	return te;
}

/* no lock for callers plz */
void sdcardfs_free_tree_entry(struct dentry *dentry)
{
	struct sdcardfs_tree_entry *te = SDCARDFS_DI_X(dentry);

	if (te != NULL) {
		struct dentry *real = te->real.dentry_invalid ?
			NULL : te->real.dentry;

		debugln("%s, dentry(%p, %s) free %p",
			__FUNCTION__, dentry, dentry->d_name.name, te);
		write_unlock(&te->lock);

		/* dput could lead to reclaim lower_dentry/inode.
		   so, it is not suitable to put dput in a rwlock */
		dput(real);
		kmem_cache_free(sdcardfs_tree_entry_cachep, te);
	}
}

struct __sdcardfs_ilookup5_priv_data {
	unsigned long ino;
	__u32 generation;
};

static int
__sdcardfs_ilookup5_test(struct inode *inode, void *_priv)
{
	struct __sdcardfs_ilookup5_priv_data *p = _priv;

	return p->generation == inode->i_generation &&
		p->ino == inode->i_ino;
}

/* find the exact alias */
static struct dentry *__sdcardfs_d_reclaim_alias(
	struct inode *inode,
	struct dentry *reclaim_dentry,
	unsigned d_seq
) {
	struct dentry *found = NULL;
	if (likely(!hlist_empty(&inode->i_dentry))) {
		spin_lock(&inode->i_lock);
		hlist_for_each_entry(found, &inode->i_dentry, d_u.d_alias) {
			spin_lock(&found->d_lock);
			if (found == reclaim_dentry &&
				!__read_seqcount_retry(&found->d_seq, d_seq)) {
				dget_dlock(found);
				spin_unlock(&found->d_lock);
				break;
			}
			spin_unlock(&found->d_lock);
		}
		spin_unlock(&inode->i_lock);
	}
	return found;
}

#ifdef SDCARDFS_UNDERLAY_MULTI_ALIASES
static int __sdcardfs_evaluate_real_locked(
	const struct dentry *dentry,
	struct sdcardfs_tree_entry *te,
	struct dentry *candidate
) {
	struct sdcardfs_tree_entry *pte;
	struct dentry *parent;
	int valid = 1;

	/* avoid deadlock -- will check again*/
	write_unlock(&te->lock);

	/* make sure that the parent cannot be released */
	rcu_read_lock();
	parent = ACCESS_ONCE(dentry->d_parent);
	BUG_ON(parent == dentry);
	pte = SDCARDFS_DI_R(parent);
	rcu_read_unlock();
	if (candidate->d_parent != pte->real.dentry)
		valid = 0;
	read_unlock(&pte->lock);

	if (valid) {
		if (dentry->d_name.len !=
			candidate->d_name.len)
			valid = 0;
		else {
			spin_lock(&candidate->d_lock);
			valid = !strcasecmp(
				dentry->d_name.name,
				candidate->d_name.name
			);
			spin_unlock(&candidate->d_lock);
		}
	}
	write_lock(&te->lock);
	/* check d_seq again at last :) */
	if (valid)
		valid = !__read_seqcount_retry(&candidate->d_seq,
			te->real.d_seq);
	return valid;
}
#endif

struct dentry *
_sdcardfs_reactivate_real_locked(
	const struct dentry *dentry,
	struct sdcardfs_tree_entry *te
) {
	struct dentry *pivot, *victim = NULL;
	struct inode *real_inode;
	struct __sdcardfs_ilookup5_priv_data priv;

	BUG_ON(!te->real.dentry_invalid);

	priv.ino = te->real.ino;
	priv.generation = te->real.generation;
	read_unlock(&te->lock);

	real_inode = ilookup5_nowait(
		sdcardfs_lower_super(dentry->d_sb),
		priv.ino, __sdcardfs_ilookup5_test,
		&priv);

	write_lock(&te->lock);

	/* safe accessed without te lock */
	if (!te->real.dentry_invalid) {
		goto out_unlock;
	}

	if (real_inode == NULL ||
	/* if the real_inode is in I_NEW state,
	   it shouldn't be the original real one */
		test_bit(__I_NEW, &real_inode->i_state)) {
		pivot = NULL;
		goto out_pivot;
	}

	/* if the real is still not updated */
	pivot = __sdcardfs_d_reclaim_alias(real_inode,
		te->real.dentry, te->real.d_seq);

#ifdef SDCARDFS_UNDERLAY_MULTI_ALIASES
	if (pivot != NULL && !S_ISDIR(real_inode->i_mode)) {
		int valid;

		valid = __sdcardfs_evaluate_real_locked(dentry, te, pivot);
		/* someone updates it in _sdcardfs_evaluate_real_locked */
		if (!te->real.dentry_invalid) {
			victim = pivot;
			goto out_unlock;
		}
		if (!valid) {
			victim = pivot;
			pivot = NULL;
		}
	}
#endif

out_pivot:
	te->real.dentry = pivot;
	te->real.dentry_invalid = false;
out_unlock:
	write_unlock(&te->lock);
	iput(real_inode);
	dput(victim);
	read_lock(&te->lock);
	return te->real.dentry;
}
