/*
 * hw_mmc_maintenance.c
 *
 * add maintenance function for mmc
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 *
 * Authors:
 * Wangtao <wangtao126@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/highmem.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/scatterlist.h>
#include <linux/regulator/consumer.h>
#include <linux/pm_runtime.h>
#include <linux/bootmem.h>
#include <linux/blkdev.h>
#include <linux/bio.h>
#include <linux/blk-mq.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/slot-gpio.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdhci.h>
#include <linux/capability.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include <linux/mmc/hw_mmc_maintenance.h>

/*
 * mmc command sequence record
 */

#define CMDQ_READ_CMD_NO (46)
#define CMDQ_WRITE_CMD_NO (47)

static DEFINE_SPINLOCK(cmd_spin_lock);
static u32 mmc_cmd_max;
static u32 mmc_cmd_no;
char *mmc_cmd_buf;

/* command */
struct mmc_cmd_content {
		u32 no;
		u32 opcode;
		u32 arg;
		u32 blk_addr;
		u32 blocks;
		u32 flags;
		u32 ticks;
		char reserve[4];
} __packed;

static ssize_t read_mmc_cmd(struct file *file, char __user *buf,
			 size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;

	if (p + count > MMC_CMD_BUF_SIZE) {
		pr_err("%s: mmc_cmd_buf=%p\n", __func__, mmc_cmd_buf);
		count = MMC_CMD_BUF_SIZE - p;
	}

	if (!mmc_cmd_buf || copy_to_user(buf, mmc_cmd_buf + p, count))
		return -EFAULT;

	p += count;
	*ppos = p;
	return count;
}
static int open_mmc(struct inode *inode, struct file *filp)
{
	return 0;
}
static ssize_t write_null(struct file *file, const char __user *buf,
			  size_t count, loff_t *ppos)
{
	return count;
}
static loff_t null_lseek(struct file *file, loff_t offset, int orig)
{
	return file->f_pos = 0;
}
const struct file_operations mmc_cmd_fops = {
	.llseek		= null_lseek,
	.read		= read_mmc_cmd,
	.open		= open_mmc,
	.write		= write_null,
};

void mmc_cmd_sequence_rec_init(void)
{
	if (!mmc_cmd_buf) {
		mmc_cmd_buf =  kmalloc(MMC_CMD_BUF_SIZE, GFP_KERNEL);
		if (!mmc_cmd_buf) {
			pr_err("%s: mmc_cmd_buf is NULL\n", __func__);
			return;
		}
	}
	pr_err("%s: mmc_cmd_buf=%p\n", __func__, mmc_cmd_buf);
	memset(mmc_cmd_buf, 0, MMC_CMD_BUF_SIZE);
	mmc_cmd_max = MMC_CMD_BUF_SIZE / sizeof(struct mmc_cmd_content);
}
EXPORT_SYMBOL(mmc_cmd_sequence_rec_init);

void mmc_cmd_sequence_rec_exit(void)
{
	if (unlikely(mmc_cmd_buf)) {
		kfree(mmc_cmd_buf);
		mmc_cmd_buf = NULL;
	}
}
EXPORT_SYMBOL(mmc_cmd_sequence_rec_exit);

/* record command to memory */
static void record_cmd(struct mmc_cmd_content *r)
{
	struct mmc_cmd_content *ptr = NULL;

	if (!mmc_cmd_buf)
		return;

	spin_lock(&cmd_spin_lock);
	ptr = (struct mmc_cmd_content *)mmc_cmd_buf + mmc_cmd_no % mmc_cmd_max;
	r->no = mmc_cmd_no;
	memcpy(ptr, r, sizeof(struct mmc_cmd_content));
	mmc_cmd_no++;
	spin_unlock(&cmd_spin_lock);
}

/* make record of cmdq read and write command */
void record_mmc_cmdq_cmd(struct mmc_request *mrq)
{
	struct mmc_cmd_content record;

	if (!mrq)
		return;

	memset(&record, 0, sizeof(struct mmc_cmd_content));

	/* CMD6/CMD35/CMD36/CMD37 */
	if (mrq->cmdq_req && mrq->cmdq_req->cmdq_req_flags & DCMD && mrq->cmd) {
		record.opcode = mrq->cmd->opcode;
		record.arg = mrq->cmd->arg;
		record.flags = mrq->cmd->flags;
		record.ticks = jiffies;
		record.blk_addr = 1;
		goto record;
	}

	/* CMD46/CMD47 */
	if (mrq->data && mrq->cmdq_req) {
		if (mrq->data->flags & MMC_DATA_READ ||
			mrq->data->flags & MMC_DATA_WRITE) {
			record.opcode = (mrq->data->flags & MMC_DATA_READ) ?
							CMDQ_READ_CMD_NO:CMDQ_WRITE_CMD_NO;
			record.blk_addr = mrq->cmdq_req->blk_addr;
			record.blocks = mrq->data->blocks;
			record.flags = mrq->data->flags;
			record.ticks = jiffies;
			record.arg = mrq->cmdq_req->tag;
			goto record;
		}
	}
	return;
record:
	record_cmd(&record);
}
EXPORT_SYMBOL(record_mmc_cmdq_cmd);

/* make record of non cmdq command */
void record_mmc_cmd(struct mmc_command *cmd)
{
	struct mmc_cmd_content record;

	if (!cmd)
		return;

	if (cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK ||
			cmd->opcode == MMC_READ_MULTIPLE_BLOCK ||
			cmd->opcode == MMC_WRITE_BLOCK ||
			cmd->opcode == MMC_READ_SINGLE_BLOCK) {
		if (cmd->data) {
			memset(&record, 0, sizeof(struct mmc_cmd_content));
			record.opcode = cmd->opcode;
			record.blk_addr = cmd->arg;
			record.blocks = cmd->data->blocks;
			record.flags = cmd->data->flags;
			record.ticks = jiffies;
			record_cmd(&record);
		}
	} else {
		memset(&record, 0, sizeof(struct mmc_cmd_content));
		record.opcode = cmd->opcode;
		record.arg = cmd->arg;
		record.flags = cmd->flags;
		record.ticks = jiffies;
		record_cmd(&record);
	}
}
EXPORT_SYMBOL(record_mmc_cmd);

/*
 * mmc read and write data record
 */
#ifdef CONFIG_HW_MMC_MAINTENANCE_DATA

#define MMC_BOOTMEM_POS_SIZE  (16)
#define MMC_BOOTMEM_DATA_SIZE (MMC_BOOTMEM_SIZE - MMC_BOOTMEM_POS_SIZE)
#define MMC_BOOTMEM_DATA_POS_START (MMC_BOOTMEM_DATA_SIZE + 12)

/* data header */
struct mmc_data_content {
		u32 no;
		u32 blk_addr;
		u32 ticks;
		u32 rw_flag;
} __packed;

char *mmc_bootmem;
static DEFINE_SPINLOCK(mmc_bootmem_spin_lock);

static u32 data_cur_pos;
static u32 bootmem_data_seq;

static ssize_t read_mmc_data(struct file *file, char __user *buf,
			 size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;

	if (p + count > MMC_BOOTMEM_SIZE) {
		pr_err("%s: mmc_bootmem=%p\n", __func__, mmc_bootmem);
		count = MMC_BOOTMEM_SIZE - p;
	}

	if (!mmc_bootmem || copy_to_user(buf, mmc_bootmem + p, count))
		return -EFAULT;

	p += count;
	*ppos = p;
	return count;
}

const struct file_operations mmc_data_fops = {
	.llseek		= null_lseek,
	.read		= read_mmc_data,
	.open		= open_mmc,
	.write		= write_null,
};

/* initial memory for mmc maintenance */
void mmc_bootmem_init(char *mmc_bootmem)
{
	if (!mmc_bootmem) {
		pr_err("%s: mmc_bootmem = %p\n", __func__, mmc_bootmem);
		return;
	}
	memset(mmc_bootmem, 0, MMC_BOOTMEM_SIZE);
	pr_err("%s: mmc_bootmem = %p\n", __func__, mmc_bootmem);
}
EXPORT_SYMBOL(mmc_bootmem_init);

/* record data to memory */
static void record_data(char *data, u32 length)
{
	u32 len1 = 0, len2 = 0;
	char *start1 = NULL;
	char *start2 = NULL;

	if (!length || !mmc_bootmem)
		return;

	start1 = (char *)(mmc_bootmem + data_cur_pos);
	start2 = (char *)mmc_bootmem;
	if (data_cur_pos + length > MMC_BOOTMEM_DATA_SIZE) {
		len1 = MMC_BOOTMEM_DATA_SIZE - data_cur_pos;
		len2 = data_cur_pos + length - MMC_BOOTMEM_DATA_SIZE;
	} else {
		len1 = length;
		len2 = 0;
	}
	if (len1) {
		memcpy(start1, data, len1);
		data_cur_pos += len1;
	}
	if (len2) {
		memcpy(start2, data + len1, len2);
		data_cur_pos = len2;
	}
}

/* record final position of data in memory */
static void record_data_pos(void)
{
	if (!mmc_bootmem)
		return;
	memcpy((char *)(mmc_bootmem + MMC_BOOTMEM_DATA_POS_START),
				(char *)(&data_cur_pos), sizeof(data_cur_pos));
	bootmem_data_seq++;
}

/* record cmdq read and write data */
void record_cmdq_rw_data(struct mmc_request *mrq)
{
	struct mmc_data_content record;
	struct request *req = NULL;
	int direction = 0;
	int i = 0;
	struct page *vec_page = NULL;
	char *data = NULL;
	u32 data_length = 0;
	struct bio *bio = NULL;

	if (!mrq)
		return;

	req = mrq->req;
	if (req != NULL && req->bio != NULL) {
		if (mrq->data != NULL && !mrq->data->error) {
			direction = (mrq->data->flags & MMC_DATA_WRITE) ?
							DMA_TO_DEVICE : DMA_FROM_DEVICE;
			spin_lock(&mmc_bootmem_spin_lock);
			/* record data head */
			memset(&record, 0, sizeof(record));
			record.no = bootmem_data_seq;
			record.blk_addr = req->bio->bi_iter.bi_sector;
			record.ticks = jiffies;
			record.rw_flag = direction;
			record_data((char *)(&record), sizeof(struct mmc_data_content));
			/* record data */
			dma_sync_sg_for_cpu(mmc_dev(mrq->host), mrq->data->sg,
								mrq->data->sg_len, direction);
			bio = req->bio;
			for_each_bio(bio) {
				for (i = 0; i < bio->bi_vcnt; i++) {
					vec_page = bio->bi_io_vec[i].bv_page;
					data = kmap_atomic(vec_page);
					record_data(data + bio->bi_io_vec[i].bv_offset,
								bio->bi_io_vec[i].bv_len);
					kunmap_atomic(data);
					data_length += bio->bi_io_vec[i].bv_len;
				}
			}
			/* record data length */
			record_data((char *)(&data_length), sizeof(data_length));
			/* record pos */
			record_data_pos();
			spin_unlock(&mmc_bootmem_spin_lock);
		}
	}
}
#endif
