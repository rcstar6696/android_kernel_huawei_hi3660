/*
 * fs/f2fs/recovery.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *             http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/fs.h>
#include <linux/f2fs_fs.h>
#include "f2fs.h"
#include "node.h"
#include "segment.h"

/*
 * Roll forward recovery scenarios.
 *
 * [Term] F: fsync_mark, D: dentry_mark
 *
 * 1. inode(x) | CP | inode(x) | dnode(F)
 * -> Update the latest inode(x).
 *
 * 2. inode(x) | CP | inode(F) | dnode(F)
 * -> No problem.
 *
 * 3. inode(x) | CP | dnode(F) | inode(x)
 * -> Recover to the latest dnode(F), and drop the last inode(x)
 *
 * 4. inode(x) | CP | dnode(F) | inode(F)
 * -> No problem.
 *
 * 5. CP | inode(x) | dnode(F)
 * -> The inode(DF) was missing. Should drop this dnode(F).
 *
 * 6. CP | inode(DF) | dnode(F)
 * -> No problem.
 *
 * 7. CP | dnode(F) | inode(DF)
 * -> If f2fs_iget fails, then goto next to find inode(DF).
 *
 * 8. CP | dnode(F) | inode(x)
 * -> If f2fs_iget fails, then goto next to find inode(DF).
 *    But it will fail due to no inode(DF).
 */

static struct kmem_cache *fsync_entry_slab;

bool space_for_roll_forward(struct f2fs_sb_info *sbi)
{
	s64 nalloc = percpu_counter_sum_positive(&sbi->alloc_valid_block_count);

	if (sbi->last_valid_block_count + nalloc > sbi->user_block_count)
		return false;
	return true;
}

static struct fsync_inode_entry *get_fsync_inode(struct list_head *head,
								nid_t ino)
{
	struct fsync_inode_entry *entry;

	list_for_each_entry(entry, head, list)
		if (entry->inode->i_ino == ino)
			return entry;

	return NULL;
}

static struct fsync_inode_entry *add_fsync_inode(struct f2fs_sb_info *sbi,
					struct list_head *head, nid_t ino)
{
	struct inode *inode;
	struct fsync_inode_entry *entry;

	inode = f2fs_iget_retry(sbi->sb, ino);
	if (IS_ERR(inode))
		return ERR_CAST(inode);

	entry = f2fs_kmem_cache_alloc(fsync_entry_slab, GFP_F2FS_ZERO);
	entry->inode = inode;
	entry->pino = F2FS_I(inode)->i_pino;
	list_add_tail(&entry->list, head);

	return entry;
}

static void del_fsync_inode(struct fsync_inode_entry *entry,
			struct radix_tree_root *root)
{
	nid_t ino = entry->inode->i_ino;

	iput(entry->inode);
	list_del(&entry->list);
	if (root) {
		struct fsync_inode_entry *e;

		e = radix_tree_lookup(root, ino);
		if (e)
			radix_tree_delete(root, ino);
	}
	kmem_cache_free(fsync_entry_slab, entry);
}

static void destroy_fsync_dnodes(struct list_head *head,
			struct radix_tree_root *root)
{
	struct fsync_inode_entry *entry, *tmp;

	list_for_each_entry_safe(entry, tmp, head, list)
		del_fsync_inode(entry, root);
}

static int recover_dentry(struct inode *inode, struct page *ipage,
						struct list_head *dir_list)
{
	struct f2fs_inode *raw_inode = F2FS_INODE(ipage);
	nid_t pino = le32_to_cpu(raw_inode->i_pino);
	struct f2fs_dir_entry *de;
	struct fscrypt_name fname;
	struct page *page;
	struct inode *dir, *einode;
	struct fsync_inode_entry *entry;
	int err = 0;
	char *name;

	entry = get_fsync_inode(dir_list, pino);
	if (!entry) {
		entry = add_fsync_inode(F2FS_I_SB(inode), dir_list, pino);
		if (IS_ERR(entry)) {
			dir = ERR_CAST(entry);
			err = PTR_ERR(entry);
			goto out;
		}
	}

	dir = entry->inode;

	memset(&fname, 0, sizeof(struct fscrypt_name));
	fname.disk_name.len = le32_to_cpu(raw_inode->i_namelen);
	fname.disk_name.name = raw_inode->i_name;

	if (unlikely(fname.disk_name.len > F2FS_NAME_LEN)) {
		WARN_ON(1);
		err = -ENAMETOOLONG;
		goto out;
	}
retry:
	de = __f2fs_find_entry(dir, &fname, &page);
	if (de && inode->i_ino == le32_to_cpu(de->ino))
		goto out_unmap_put;

	if (de) {
		einode = f2fs_iget_retry(inode->i_sb, le32_to_cpu(de->ino));
		if (IS_ERR(einode)) {
			WARN_ON(1);
			err = PTR_ERR(einode);
			if (err == -ENOENT)
				err = -EEXIST;
			goto out_unmap_put;
		}
		err = acquire_orphan_inode(F2FS_I_SB(inode));
		if (err) {
			iput(einode);
			goto out_unmap_put;
		}
		f2fs_delete_entry(de, page, dir, einode);
		iput(einode);
		goto retry;
	} else if (IS_ERR(page)) {
		err = PTR_ERR(page);
	} else {
		err = __f2fs_do_add_link(dir, &fname, inode,
					inode->i_ino, inode->i_mode);
	}
	if (err == -ENOMEM)
		goto retry;
	goto out;

out_unmap_put:
	f2fs_dentry_kunmap(dir, page);
	f2fs_put_page(page, 0);
out:
	if (file_enc_name(inode))
		name = "<encrypted>";
	else
		name = raw_inode->i_name;
	f2fs_msg(inode->i_sb, KERN_NOTICE,
			"%s: ino = %x, name = %s, dir = %lx, err = %d",
			__func__, ino_of_node(ipage), name,
			IS_ERR(dir) ? 0 : dir->i_ino, err);
	return err;
}

static void recover_inode(struct inode *inode, struct page *page)
{
	struct f2fs_inode *raw = F2FS_INODE(page);
	char *name;

	inode->i_mode = le16_to_cpu(raw->i_mode);
	f2fs_i_size_write(inode, le64_to_cpu(raw->i_size));
	inode->i_atime.tv_sec = le64_to_cpu(raw->i_atime);
	inode->i_ctime.tv_sec = le64_to_cpu(raw->i_ctime);
	inode->i_mtime.tv_sec = le64_to_cpu(raw->i_mtime);
	inode->i_atime.tv_nsec = le32_to_cpu(raw->i_atime_nsec);
	inode->i_ctime.tv_nsec = le32_to_cpu(raw->i_ctime_nsec);
	inode->i_mtime.tv_nsec = le32_to_cpu(raw->i_mtime_nsec);

	F2FS_I(inode)->i_advise = raw->i_advise;

	/* For directory inode only */
	F2FS_I(inode)->i_current_depth = le32_to_cpu(raw->i_current_depth);
	F2FS_I(inode)->i_dir_level = raw->i_dir_level;

	if (file_enc_name(inode))
		name = "<encrypted>";
	else
		name = F2FS_INODE(page)->i_name;

	f2fs_msg(inode->i_sb, KERN_NOTICE, "recover_inode: ino = %x, name = %s",
			ino_of_node(page), name);
}

static void __add_dir_entry(struct radix_tree_root *root, nid_t ino,
				void *entry)
{
	struct fsync_inode_entry *e;

retry:
	radix_tree_preload(GFP_NOFS | __GFP_NOFAIL);

	e = radix_tree_lookup(root, ino);
	if (!e) {
		e = entry;
		if (radix_tree_insert(root, ino, e)) {
			radix_tree_preload_end();
			goto retry;
		}
	}
	radix_tree_preload_end();
}

static int find_fsync_dnodes(struct f2fs_sb_info *sbi, int type,
		struct list_head *head, struct radix_tree_root *root)
{
	struct curseg_info *curseg;
	struct page *page = NULL;
	block_t blkaddr;
	int err = 0;

	/* get node pages in the current segment */
	curseg = CURSEG_I(sbi, type);
	blkaddr = NEXT_FREE_BLKADDR(sbi, curseg);

	while (1) {
		struct fsync_inode_entry *entry;

		if (!is_valid_blkaddr(sbi, blkaddr, META_POR))
			return 0;

		page = get_tmp_page(sbi, blkaddr);
		if (PageChecked(page)) {
			f2fs_msg(sbi->sb, KERN_ERR, "Abandon looped node block list");
			destroy_fsync_dnodes(head, NULL);
			break;
		}

		/*
		 * it's not needed to clear PG_checked flag in temp page since we
		 * will truncate all those pages in the end of recovery.
		 */
		SetPageChecked(page);

		if (!is_recoverable_dnode(page))
			break;

		if (!is_fsync_dnode(page))
			goto next;

		entry = get_fsync_inode(head, ino_of_node(page));
		if (!entry) {
			if (IS_INODE(page) && is_dent_dnode(page)) {
				err = recover_inode_page(sbi, page);
				if (err)
					break;
			}

			/*
			 * CP | dnode(F) | inode(DF)
			 * For this case, we should not give up now.
			 */
			entry = add_fsync_inode(sbi, head, ino_of_node(page));
			if (IS_ERR(entry)) {
				err = PTR_ERR(entry);
				if (err == -ENOENT) {
					err = 0;
					goto next;
				}
				break;
			}

			/* add this fsync inode to the radix tree */
			if (root)
				__add_dir_entry(root, ino_of_node(page), entry);
		}
		entry->blkaddr = blkaddr;
		if (__is_set_ckpt_flags(F2FS_CKPT(sbi), CP_CRC_RECOVERY_FLAG_XOR))
			entry->node_ver =
				cpver_of_node(page) ^ (sbi->ckpt_crc << 32);
		else
			entry->node_ver = cur_cp_version(F2FS_CKPT(sbi));

		if (IS_INODE(page) && is_dent_dnode(page))
			entry->last_dentry = blkaddr;
next:
		/* check next segment */
		blkaddr = next_blkaddr_of_node(page);
		f2fs_put_page(page, 1);

		ra_meta_pages_cond(sbi, blkaddr);
	}
	f2fs_put_page(page, 1);
	return err;
}

static int check_index_in_prev_nodes(struct f2fs_sb_info *sbi,
			block_t blkaddr, struct dnode_of_data *dn)
{
	struct seg_entry *sentry;
	unsigned int segno = GET_SEGNO(sbi, blkaddr);
	unsigned short blkoff = GET_BLKOFF_FROM_SEG0(sbi, blkaddr);
	struct f2fs_summary_block *sum_node;
	struct f2fs_summary sum;
	struct page *sum_page, *node_page;
	struct dnode_of_data tdn = *dn;
	nid_t ino, nid;
	struct inode *inode;
	unsigned int offset;
	block_t bidx;
	int i;

	sentry = get_seg_entry(sbi, segno);
	if (!f2fs_test_bit(blkoff, sentry->cur_valid_map))
		return 0;

	/* Get the previous summary */
	for (i = CURSEG_HOT_DATA; i <= CURSEG_COLD_DATA; i++) {
		struct curseg_info *curseg = CURSEG_I(sbi, i);
		if (curseg->segno == segno) {
			sum = curseg->sum_blk->entries[blkoff];
			goto got_it;
		}
	}

	sum_page = get_sum_page(sbi, segno);
	sum_node = (struct f2fs_summary_block *)page_address(sum_page);
	sum = sum_node->entries[blkoff];
	f2fs_put_page(sum_page, 1);
got_it:
	/* Use the locked dnode page and inode */
	nid = le32_to_cpu(sum.nid);
	if (dn->inode->i_ino == nid) {
		tdn.nid = nid;
		if (!dn->inode_page_locked)
			lock_page(dn->inode_page);
		tdn.node_page = dn->inode_page;
		tdn.ofs_in_node = le16_to_cpu(sum.ofs_in_node);
		goto truncate_out;
	} else if (dn->nid == nid) {
		tdn.ofs_in_node = le16_to_cpu(sum.ofs_in_node);
		goto truncate_out;
	}

	/* Get the node page */
	node_page = get_node_page(sbi, nid);
	if (IS_ERR(node_page))
		return PTR_ERR(node_page);

	offset = ofs_of_node(node_page);
	ino = ino_of_node(node_page);
	f2fs_put_page(node_page, 1);

	if (ino != dn->inode->i_ino) {
		/* Deallocate previous index in the node page */
		inode = f2fs_iget_retry(sbi->sb, ino);
		if (IS_ERR(inode))
			return PTR_ERR(inode);
	} else {
		inode = dn->inode;
	}

	bidx = start_bidx_of_node(offset, inode) + le16_to_cpu(sum.ofs_in_node);

	/*
	 * if inode page is locked, unlock temporarily, but its reference
	 * count keeps alive.
	 */
	if (ino == dn->inode->i_ino && dn->inode_page_locked)
		unlock_page(dn->inode_page);

	set_new_dnode(&tdn, inode, NULL, NULL, 0);
	if (get_dnode_of_data(&tdn, bidx, LOOKUP_NODE))
		goto out;

	if (tdn.data_blkaddr == blkaddr)
		truncate_data_blocks_range(&tdn, 1);

	f2fs_put_dnode(&tdn);
out:
	if (ino != dn->inode->i_ino)
		iput(inode);
	else if (dn->inode_page_locked)
		lock_page(dn->inode_page);
	return 0;

truncate_out:
	if (datablock_addr(tdn.node_page, tdn.ofs_in_node) == blkaddr)
		truncate_data_blocks_range(&tdn, 1);
	if (dn->inode->i_ino == nid && !dn->inode_page_locked)
		unlock_page(dn->inode_page);
	return 0;
}

static int do_recover_data(struct f2fs_sb_info *sbi, struct inode *inode,
					struct page *page, block_t blkaddr)
{
	struct dnode_of_data dn;
	struct node_info ni;
	unsigned int start, end;
	int err = 0, recovered = 0;

	/* step 1: recover xattr */
	if (IS_INODE(page)) {
		recover_inline_xattr(inode, page);
	} else if (f2fs_has_xattr_block(ofs_of_node(page))) {
		err = recover_xattr_data(inode, page, blkaddr);
		if (!err)
			recovered++;
		goto out;
	}

	/* step 2: recover inline data */
	if (recover_inline_data(inode, page))
		goto out;

	if (recover_inline_dentry(inode, page))
		goto out;

	/* step 3: recover data indices */
	start = start_bidx_of_node(ofs_of_node(page), inode);
	end = start + ADDRS_PER_PAGE(page, inode);

	set_new_dnode(&dn, inode, NULL, NULL, 0);
retry_dn:
	err = get_dnode_of_data(&dn, start, ALLOC_NODE);
	if (err) {
		if (err == -ENOMEM) {
			congestion_wait(BLK_RW_ASYNC, HZ/50);
			goto retry_dn;
		}
		goto out;
	}

	f2fs_wait_on_page_writeback(dn.node_page, NODE, true);

	get_node_info(sbi, dn.nid, &ni);
	f2fs_bug_on(sbi, ni.ino != ino_of_node(page));
	f2fs_bug_on(sbi, ofs_of_node(dn.node_page) != ofs_of_node(page));

	for (; start < end; start++, dn.ofs_in_node++) {
		block_t src, dest;

		src = datablock_addr(dn.node_page, dn.ofs_in_node);
		dest = datablock_addr(page, dn.ofs_in_node);

		/* skip recovering if dest is the same as src */
		if (src == dest)
			continue;

		/* dest is invalid, just invalidate src block */
		if (dest == NULL_ADDR) {
			truncate_data_blocks_range(&dn, 1);
			continue;
		}

		if (!file_keep_isize(inode) &&
			(i_size_read(inode) <= ((loff_t)start << PAGE_SHIFT)))
			f2fs_i_size_write(inode,
				(loff_t)(start + 1) << PAGE_SHIFT);

		/*
		 * dest is reserved block, invalidate src block
		 * and then reserve one new block in dnode page.
		 */
		if (dest == NEW_ADDR) {
			truncate_data_blocks_range(&dn, 1);
			reserve_new_block(&dn);
			continue;
		}

		/* dest is valid block, try to recover from src to dest */
		if (is_valid_blkaddr(sbi, dest, META_POR)) {

			if (src == NULL_ADDR) {
				err = reserve_new_block(&dn);
#ifdef CONFIG_F2FS_FAULT_INJECTION
				while (err)
					err = reserve_new_block(&dn);
#endif
				/* We should not get -ENOSPC */
				f2fs_bug_on(sbi, err);
				if (err)
					goto err;
			}
retry_prev:
			/* Check the previous node page having this index */
			err = check_index_in_prev_nodes(sbi, dest, &dn);
			if (err) {
				if (err == -ENOMEM) {
					congestion_wait(BLK_RW_ASYNC, HZ/50);
					goto retry_prev;
				}
				goto err;
			}

			/* write dummy data page */
			f2fs_replace_block(sbi, &dn, src, dest,
						ni.version, false, false);
			recovered++;
		}
	}

	copy_node_footer(dn.node_page, page);
	fill_node_footer(dn.node_page, dn.nid, ni.ino,
					ofs_of_node(page), false);
	set_page_dirty(dn.node_page);
err:
	f2fs_put_dnode(&dn);
out:
	f2fs_msg(sbi->sb, KERN_NOTICE,
		"recover_data: ino = %lx (i_size: %s) recovered = %d, err = %d",
		inode->i_ino,
		file_keep_isize(inode) ? "keep" : "recover",
		recovered, err);
	return err;
}

static int __exist_dentry_in_parent(struct f2fs_sb_info *sbi,
				struct fsync_inode_entry *entry,
				struct fsync_inode_entry *parent_entry)
{
	struct page *res_page;
	struct page *ipage;
	struct f2fs_inode *raw_inode;
	struct fscrypt_name fname;
	struct f2fs_dir_entry *de;
	int ret = 0;

	if (parent_entry->node_ver < entry->node_ver) {
		ret = 1;
		goto out;
	}

	ipage = get_node_page(sbi, entry->inode->i_ino);
	if (IS_ERR(ipage)) {
		if (PTR_ERR(ipage) == -ENOMEM)
			ret = -ENOMEM;
		goto out;
	}

	raw_inode = F2FS_INODE(ipage);

	memset(&fname, 0, sizeof(struct fscrypt_name));
	fname.disk_name.len = le32_to_cpu(raw_inode->i_namelen);
	fname.disk_name.name = raw_inode->i_name;

	if (unlikely(fname.disk_name.len > F2FS_NAME_LEN))
		goto out_put;

	de = __f2fs_find_entry(parent_entry->inode, &fname,
					&res_page);
	if (!de)
		goto out_put;

	if (IS_ERR(de))
		ret = PTR_ERR(de);
	else if (entry->inode->i_ino == le32_to_cpu(de->ino))
		ret = 1;

	f2fs_dentry_kunmap(parent_entry->inode, res_page);
	f2fs_put_page(res_page, 0);
out_put:
	f2fs_put_page(ipage, 1);
out:
	return ret;
}

static int recover_data(struct f2fs_sb_info *sbi, int type,
		struct list_head *head, struct radix_tree_root *root,
		struct list_head *parent_list)
{
	struct curseg_info *curseg;
	struct page *page = NULL;
	int err = 0;
	block_t blkaddr;

	/* get node pages in the current segment */
	curseg = CURSEG_I(sbi, type);
	blkaddr = NEXT_FREE_BLKADDR(sbi, curseg);

	while (1) {
		struct fsync_inode_entry *entry;

		if (!is_valid_blkaddr(sbi, blkaddr, META_POR))
			break;

		ra_meta_pages_cond(sbi, blkaddr);

		page = get_tmp_page(sbi, blkaddr);

		if (!is_recoverable_dnode(page)) {
			f2fs_put_page(page, 1);
			break;
		}

		entry = get_fsync_inode(head, ino_of_node(page));
		if (!entry || entry->node_ver < cur_cp_version(F2FS_CKPT(sbi)))
			goto next;
		/*
		 * inode(x) | CP | inode(x) | dnode(F)
		 * In this case, we can lose the latest inode(x).
		 * So, call recover_inode for the inode update.
		 */
		if (IS_INODE(page))
			recover_inode(entry->inode, page);
		if (entry->last_dentry == blkaddr) {
			err = recover_dentry(entry->inode, page, parent_list);
			if (err) {
				f2fs_put_page(page, 1);
				break;
			}
		}
		err = do_recover_data(sbi, entry->inode, page, blkaddr);
		if (err) {
			f2fs_put_page(page, 1);
			break;
		}

		if (entry->blkaddr == blkaddr) {
			if (type == CURSEG_WARM_NODE)
				del_fsync_inode(entry, NULL);
		}
next:
		/* check next segment */
		blkaddr = next_blkaddr_of_node(page);
		f2fs_put_page(page, 1);
	}
	if (!err)
		allocate_new_segments(sbi);
	return err;
}

static int __find_child_inodes(struct f2fs_dentry_ptr *d,
					struct list_head *head)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(d->inode);
	struct f2fs_dir_entry *de;
	unsigned int bit_pos = 0;
	int err = 0;

	/*lint -save -e574*/
	while (bit_pos < d->max) {
	/*lint -restore*/
		struct fsync_inode_entry *entry;
		struct node_info ni;
		nid_t ino;
		int slots;

		bit_pos = find_next_bit_le(d->bitmap, d->max, bit_pos);
		/*lint -save -e574*/
		if (bit_pos >= d->max)
		/*lint -restore*/
			break;

		de = &d->dentry[bit_pos];

		if (unlikely(!de->name_len)) {
			bit_pos++;
			continue;
		}

		slots = GET_DENTRY_SLOTS(le16_to_cpu(de->name_len));
		ino = le32_to_cpu(de->ino);

		get_node_info(sbi, ino, &ni);
		if (!is_valid_blkaddr(sbi, ni.blk_addr, META_POR))
			goto next;

		entry = get_fsync_inode(head, ino);
		if (!entry) {
			struct inode *inode;

			inode = f2fs_iget(sbi->sb, ino);
			if (IS_ERR(inode)) {
				err = PTR_ERR(inode);
				if (err == -ENOENT) {
					err = 0;
					goto next;
				}
				break;
			}

			if (S_ISDIR(inode->i_mode)) {
				iput(inode);
				goto next;
			}

			entry = add_fsync_inode(sbi, head, ino);
			if (IS_ERR(entry)) {
				err = PTR_ERR(entry);
				iput(inode);
				break;
			}

			entry->pino = d->inode->i_ino;
		}

		entry->blkaddr = ni.blk_addr;
		entry->node_ver = cur_cp_version(F2FS_CKPT(sbi)) - 1;
next:
		bit_pos += slots;
	}

	return err;
}

static int find_child_inodes(struct f2fs_sb_info *sbi, struct list_head *dir_list,
			struct list_head *child_list)
{
	struct fsync_inode_entry *entry;
	int err = 0;

	list_for_each_entry(entry, dir_list, list) {
		struct f2fs_dentry_ptr d;
		struct inode *inode = entry->inode;

		if ((f2fs_has_inline_dentry(inode))) {
			struct page *ipage;

			ipage = get_node_page(sbi, inode->i_ino);
			if (IS_ERR(ipage)) {
				err = PTR_ERR(ipage);
				return err;
			}
			make_dentry_ptr(inode, &d,
					(void *)inline_data_addr(ipage), 0);
			__find_child_inodes(&d, child_list);
			f2fs_put_page(ipage, 1);
		} else {
			unsigned long npages = dir_blocks(inode);
			unsigned long bidx;

			for (bidx = 0; bidx < npages; bidx++) {
				struct page *dentry_page;
				struct f2fs_dentry_block *dentry_blk;

				dentry_page = get_lock_data_page(inode, bidx,
									true);
				if (IS_ERR(dentry_page)) {
					err = PTR_ERR(dentry_page);
					if (err == -ENOENT)
						continue;
					else
						return err;
				}

				dentry_blk = kmap(dentry_page);
				make_dentry_ptr(inode, &d,
						(void *)dentry_blk, 1);
				__find_child_inodes(&d, child_list);
				kunmap(dentry_page);

				f2fs_put_page(dentry_page, 1);
			}
			truncate_inode_pages(inode->i_mapping, 0);
		}
	}
	return 0;
}

static int remove_invalid_inodes(struct f2fs_sb_info *sbi,
			struct list_head *head,
			struct radix_tree_root *root)
{
	struct fsync_inode_entry *entry, *tmp;
	int err = 0;

	list_for_each_entry_safe(entry, tmp, head, list) {
		struct fsync_inode_entry *parent_entry;

		parent_entry = radix_tree_lookup(root, entry->pino);
		if (parent_entry) {
			err = __exist_dentry_in_parent(sbi, entry, parent_entry);
			if (err > 0) {
				err = 0;
				continue;
			} else if (err < 0) {
				break;
			}

			i_size_write(entry->inode, 0);
			if (F2FS_HAS_BLOCKS(entry->inode)) {
				err = f2fs_truncate(entry->inode);
				if (err)
					break;
			}

			err = remove_inode_page(entry->inode);
			if (err)
				break;

			del_fsync_inode(entry, NULL);
		}

	}

	return err;
}

int recover_fsync_data(struct f2fs_sb_info *sbi, bool check_only)
{
	struct list_head dir_list, regular_list, parent_list;
	struct radix_tree_root dir_root;
	int err;
	int ret = 0;
	bool need_writecp = false;

	fsync_entry_slab = f2fs_kmem_cache_create("f2fs_fsync_inode_entry",
			sizeof(struct fsync_inode_entry));
	if (!fsync_entry_slab)
		return -ENOMEM;

	INIT_LIST_HEAD(&dir_list);
	INIT_LIST_HEAD(&regular_list);
	INIT_LIST_HEAD(&parent_list);
	INIT_RADIX_TREE(&dir_root, GFP_ATOMIC);

	/* prevent checkpoint */
	mutex_lock(&sbi->cp_mutex);

	/* step #1: find fsynced dir inode numbers */
	err = find_fsync_dnodes(sbi, CURSEG_HOT_NODE, &dir_list, &dir_root);
	if (err)
		goto out;

	/* step #2: find child inode numbers in fsynced dir */
	err = find_child_inodes(sbi, &dir_list, &regular_list);
	if (err)
		goto out;

	/* step #3: find fsynced file inode numbers */
	err = find_fsync_dnodes(sbi, CURSEG_WARM_NODE, &regular_list, NULL);
	if (err)
		goto out;

	if (list_empty(&dir_list) && list_empty(&regular_list))
		goto out;

	if (check_only) {
		ret = 1;
		goto out;
	}

	need_writecp = true;

	/* step #4: recover dir data */
	err = recover_data(sbi, CURSEG_HOT_NODE, &dir_list, &dir_root, &parent_list);
	if (err)
		goto out;

	/* step #5: remove invalid regular file */
	err = remove_invalid_inodes(sbi, &regular_list, &dir_root);
	if (err)
		goto out;

	/* step #5: recover regular file data */
	err = recover_data(sbi, CURSEG_WARM_NODE, &regular_list, &dir_root, &parent_list);

out:
	destroy_fsync_dnodes(&regular_list, NULL);

	/* truncate meta pages to be used by the recovery */
	truncate_inode_pages_range(META_MAPPING(sbi),
			(loff_t)MAIN_BLKADDR(sbi) << PAGE_SHIFT, -1);

	if (err) {
		truncate_inode_pages_final(NODE_MAPPING(sbi));
		truncate_inode_pages_final(META_MAPPING(sbi));
	}

	clear_sbi_flag(sbi, SBI_POR_DOING);
	if (err)
		set_ckpt_flags(sbi, CP_ERROR_FLAG);
	mutex_unlock(&sbi->cp_mutex);

	/* let's drop all the directory inodes for clean checkpoint */
	destroy_fsync_dnodes(&dir_list, &dir_root);
	destroy_fsync_dnodes(&parent_list, NULL);

	if (!err && need_writecp) {
		struct cp_control cpc = {
			.reason = CP_RECOVERY,
		};
		err = write_checkpoint(sbi, &cpc);
	}

	kmem_cache_destroy(fsync_entry_slab);
	return ret ? ret: err;
}
