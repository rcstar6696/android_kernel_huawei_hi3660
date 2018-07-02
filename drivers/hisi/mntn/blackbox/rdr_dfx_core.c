/*
 * blackbox. (kernel run data recorder.)
 *
 * Copyright (c) 2013 Huawei Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/of.h>

#include <linux/hisi/hisi_bootup_keypoint.h>
#include <linux/hisi/rdr_pub.h>
#include <linux/hisi/util.h>
#include <linux/hisi/rdr_hisi_platform.h>
#include <linux/hisi/rdr_dfx_core.h>
#include <linux/hisi/kirin_partition.h>

#include <hisi_partition.h>
#include "rdr_print.h"
#include "rdr_inner.h"

static u32 dfx_size_tbl[DFX_MAX_MODULE] = {0};
static u32 dfx_addr_tbl[DFX_MAX_MODULE] = {0};

int dfx_open(void)
{
	void *buf;
	char p_name[BDEVNAME_SIZE + 12];
	int ret, fd_dfx;

	BB_PRINT_START();

	buf = kzalloc(SZ_4K, GFP_KERNEL);
	if (!buf) {
		pr_err("%s():%d:kzalloc buf1 fail\n", __func__, __LINE__);
		return -ENOMEM;
	}
	ret = flash_find_ptn(PART_DFX, buf);
	if (0 != ret) {
		pr_err("%s():%d:flash_find_ptn fail\n", __func__, __LINE__);
		kfree(buf);
		return ret;
	}

	memset(p_name, 0, sizeof(p_name));
	strncpy(p_name, buf, sizeof(p_name));
	p_name[BDEVNAME_SIZE + 11] = '\0';
	kfree(buf);

	fd_dfx = sys_open(p_name, O_RDWR, FILE_LIMIT);

	return fd_dfx;
}

int dfx_read(u32 module, void *buffer, u32 size)
{
	int ret, fd_dfx, cnt=0;
	mm_segment_t old_fs = get_fs();

	if (dfx_size_tbl[module] < size || !dfx_size_tbl[module])
		return cnt;
	if (!buffer)
		return cnt;
	/*lint -e501 -esym(501,*)*/
	set_fs(KERNEL_DS);
	/*lint -e501 +esym(501,*)*/

	fd_dfx = dfx_open();
	ret = sys_lseek(fd_dfx, dfx_addr_tbl[module], SEEK_SET);
	if (ret < 0) {
		pr_err("%s():%d:lseek fail[%d]\n", __func__, __LINE__, ret);
		goto close;
	}
	cnt = sys_read(fd_dfx, buffer, size);
	if (cnt < 0) {
		pr_err("%s():%d:read fail[%d]\n", __func__, __LINE__, cnt);
		goto close;
	}
close:
	sys_close(fd_dfx);
	set_fs(old_fs);
	return cnt;
}

int dfx_write(u32 module,void *buffer, u32 size)
{
	int ret, fd_dfx, cnt=0;
	mm_segment_t old_fs = get_fs();

	if (dfx_size_tbl[module] < size || !dfx_size_tbl[module])
		return cnt;
	if (!buffer)
		return cnt;
	/*lint -e501 -esym(501,*)*/
	set_fs(KERNEL_DS);
	/*lint -e501 +esym(501,*)*/

	fd_dfx = dfx_open();
	ret = sys_lseek(fd_dfx, dfx_addr_tbl[module], SEEK_SET);
	if (ret < 0) {
		pr_err("%s():%d:lseek fail[%d]\n", __func__, __LINE__, ret);
		goto close;
	}
	cnt = sys_write(fd_dfx, buffer, size);
	if (cnt < 0) {
		pr_err("%s():%d:read fail[%d]\n", __func__, __LINE__, cnt);
		goto close;
	}
close:
	sys_close(fd_dfx);
	set_fs(old_fs);
	return cnt;
}

static int get_dfx_core_size(void)
{
	int ret;
	struct device_node *np;

	np = of_find_compatible_node(NULL, NULL,
				     "hisilicon,dfx_partition");
	if (!np) {
		pr_err("[%s], cannot find rdr_ap_adapter node in dts!\n",
		       __func__);
		return -ENODEV;
	}

	ret = of_property_read_u32(np, "dfx_noreboot_size",
				   &dfx_size_tbl[DFX_NOREBOOT]);
	if (ret) {
		pr_err("[%s], cannot find dfx_noreboot_size in dts!\n",
		       __func__);
		return ret;
	}
	dfx_addr_tbl[DFX_ZEROHUNG] += dfx_size_tbl[DFX_NOREBOOT];
	ret = of_property_read_u32(np, "dfx_zerohung_size",
				   &dfx_size_tbl[DFX_ZEROHUNG]);
	if (ret) {
		pr_err("[%s], cannot find dfx_zerohung_size in dts!\n",
		       __func__);
		return ret;
	}

	return 0;
}

int dfx_partition_init(void)
{
	int ret;

	ret = get_dfx_core_size();
	if(ret < 0)
		goto err;

	pr_err("%s success\n", __func__);
	return 0;
err:
	return ret;
}

early_initcall(dfx_partition_init);
