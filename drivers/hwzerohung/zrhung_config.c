/**********************************************************
 * Filename:    zrhung_config.c
 *
 * Discription: kernel configuration implementaion of zerohung
 *
 * Copyright: (C) 2017 huawei.
 *
 * Author: zhaochenxiao(00344580) zhangliang(00175161)
 *
**********************************************************/
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/security.h>
#include <asm/current.h>
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/syscalls.h>
#include <linux/mman.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <asm/cacheflush.h>
#include <linux/moduleloader.h>

#include "chipset_common/hwzrhung/zrhung.h"

#define SCONTEXT "u:r:logserver:s0"
#define HCFG_ENTRY_NUM 200
#define HCFG_VAL_LEN_MAX 512
#define WP_ID_MAX	99
#define HCFG_VAL_SIZE_MAX ((HCFG_VAL_LEN_MAX+1)*HCFG_ENTRY_NUM)

#define NOT_SUPPORT	-2
#define NO_CONFIG	-1
#define NOT_READY	1

struct hcfg_entry {
	uint32_t offset;
	uint32_t valid:1;
};

struct hcfg_table {
	uint64_t len;
	struct hcfg_entry entry[HCFG_ENTRY_NUM];
	char data[0];
};

struct hcfg_val {
	uint64_t wp;
	char data[HCFG_VAL_LEN_MAX];
};

struct hcfg_ctx {
	struct hcfg_table *table;
	unsigned long mem_size;
	int flag;
};

static DEFINE_SPINLOCK(lock);
static struct hcfg_ctx ctx;

int hcfgk_set_cfg(struct file *file, void __user*arg)
{
	int ret;
	uint64_t len;
	int i;
	uint64_t mem_size;
	struct hcfg_table *table = NULL;
	struct hcfg_table *tmp = NULL;

	if(!arg)
		return -EINVAL;

	ret = copy_from_user(&len, arg, sizeof(len));
	if(ret) {
		printk(KERN_ERR "copy hung config table from user failed.\n");
		return ret;
	}
	if(len > HCFG_VAL_SIZE_MAX)
		return -EINVAL;

	mem_size = PAGE_ALIGN(sizeof(*table)+len);

	table = module_alloc(mem_size);
	if(!table) {
		printk(KERN_ERR "Alloc hung config table failed.\n");
		return -ENOMEM;
	}
	set_memory_nx(table, mem_size>>PAGE_SHIFT);
	memset(table, 0, mem_size);

	ret = copy_from_user(table, arg, sizeof(*table)+len);
	if(ret) {
		printk(KERN_ERR "copy hung config table from user failed.\n");
		module_memfree(table);
		return ret;
	}
	/*
	 * make sure last byte in data is 0 terminated
	 */
	if(len > 0)
		table->data[len-1] = '\0';

	if((ret=set_memory_ro((unsigned long)table, mem_size>>PAGE_SHIFT))) {
		printk(KERN_ERR "set memory ro failed.\n");
	};

	spin_lock(&lock);

	tmp = ctx.table;
	ctx.table = table;
	table = tmp;
	ctx.mem_size = mem_size;

	spin_unlock(&lock);

	if(table != NULL) {
		module_memfree(table);
	}

	return ret;
}

int zrhung_get_config(zrhung_wp_id wp, char *data, uint32_t maxlen)
{
	int ret = NOT_READY;

	if(!data || wp >= HCFG_ENTRY_NUM || maxlen == 0)
		return -EINVAL;

	if (in_atomic() || in_interrupt()) {
		printk(KERN_ERR "can not get config in interrupt context");
		return -EINVAL;
	}

	spin_lock(&lock);

	if(!ctx.table || (wp <= WP_ID_MAX && ctx.flag == 0))
		goto out;

	if(!ctx.table->entry[wp].valid) {
		ret = NO_CONFIG;
		goto out;
	}

	if(ctx.table->entry[wp].offset >= ctx.table->len) {
		ret = -EINVAL;
		goto out;
	}

	strncpy(data, ctx.table->data+ctx.table->entry[wp].offset, maxlen-1);
	data[maxlen-1] = '\0';
	ret = 0;

out:
	spin_unlock(&lock);
	return ret;
}
EXPORT_SYMBOL(zrhung_get_config);

int hcfgk_ioctl_get_cfg(struct file *file, void __user*arg)
{
	struct hcfg_val val;
	int ret;

	if(!arg)
		return -EINVAL;

	memset(&val, 0, sizeof(val));
	if(copy_from_user(&val.wp, arg, sizeof(val.wp))) {
		printk(KERN_ERR "Get WP id from user failed.\n");
		return NOT_SUPPORT;
	}

	ret = zrhung_get_config(val.wp, val.data, sizeof(val.data));
	if(!ret && copy_to_user(arg, &val, sizeof(val))) {
		printk(KERN_ERR "Failed to copy hung config val to user.\n");
		ret = -EFAULT;
	}

	return ret;
}

int hcfgk_set_cfg_flag(struct file *file, void __user*arg)
{
	uint32_t flag;

	if(!arg)
		return -EINVAL;

	if(copy_from_user(&flag, arg, sizeof(flag))) {
		printk(KERN_ERR "Copy Hung config flag from user failed.\n");
		return -EFAULT;
	}

	printk(KERN_DEBUG "set hcfg flag: %d\n", ctx.flag);

	spin_lock(&lock);

	if(flag > 0)
		ctx.flag = flag;

	spin_unlock(&lock);

	return 0;
}

int hcfgk_get_cfg_flag(struct file *file, void __user*arg)
{
	if(!arg)
		return -EINVAL;

	printk(KERN_DEBUG "get hcfg flag: %d\n", ctx.flag);

	spin_lock(&lock);

	if(copy_to_user(arg, &ctx.flag, sizeof(ctx.flag))) {
		spin_unlock(&lock);
		printk(KERN_ERR "Failed to copy hung config flag to user.\n");
		return -EFAULT;
	}

	spin_unlock(&lock);

	return 0;
}
