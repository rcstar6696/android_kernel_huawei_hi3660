/*******************************************************************************
 * All rights reserved, Copyright (C) huawei LIMITED 2012
 *
 * This source code has been made available to you by HUAWEI on an
 * AS-IS basis. Anyone receiving this source code is licensed under HUAWEI
 * copyrights to use it in any way he or she deems fit, including copying it,
 * modifying it, compiling it, and redistributing it either with or without
 * modifications. Any person who transfers this source code or any derivative
 * work must include the HUAWEI copyright notice and this paragraph in
 * the transferred software.
*******************************************************************************/
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/freezer.h>
#include <linux/module.h>
#include <linux/mempool.h>
#include <linux/vmalloc.h>

#include "mem.h"
#include "smc.h"
#include "tc_ns_client.h"
#include "teek_ns_client.h"
#include "agent.h"
#include "securec.h"
#include "tc_ns_log.h"
#include "mailbox_mempool.h"

void tc_mem_free(TC_NS_Shared_MEM *shared_mem)
{
	if (NULL == shared_mem)
		return;

	if (shared_mem->from_mailbox)
		mailbox_free(shared_mem->kernel_addr);
	else
		vfree(shared_mem->kernel_addr);
	kfree(shared_mem);
}

TC_NS_Shared_MEM *tc_mem_allocate(size_t len, bool from_mailbox)
{
	TC_NS_Shared_MEM *shared_mem = NULL;
	void *addr = NULL;

	if (0 == len) {
		tloge("alloc length 0 share memory is not allowed\n");
		return ERR_PTR(-EFAULT);
	}

	shared_mem = kmalloc(sizeof(TC_NS_Shared_MEM), GFP_KERNEL|__GFP_ZERO);
	if (!shared_mem) {
		tloge("shared_mem kmalloc failed\n");
		return ERR_PTR(-ENOMEM);
	}

	if (from_mailbox)
		addr = mailbox_alloc(len, MB_FLAG_ZERO);
	else {
		len = ALIGN(len, SZ_4K);
		if (len > MAILBOX_POOL_SIZE) {
			tloge("alloc sharemem size(%zu) is too large\n", len);
			kfree(shared_mem);
			return ERR_PTR(-EINVAL);
		}
		addr = vmalloc_user(len);
	}
	if (!addr) {
		tloge("alloc maibox failed\n");
		kfree(shared_mem);
		return ERR_PTR(-ENOMEM);
	}

	shared_mem->from_mailbox = from_mailbox;
	shared_mem->kernel_addr = addr;
	shared_mem->len = len;

	return shared_mem;
}

int tc_mem_init(void)
{
	int ret;

	tlogi("tc_mem_init\n");

	ret = mailbox_mempool_init();
	if (ret) {
		tloge("tz mailbox init failed\n");
		return -ENOMEM;
	}

	return 0;
}

void tc_mem_destroy(void)
{
	tlogi("tc_client exit\n");

	mailbox_mempool_destroy();
}
