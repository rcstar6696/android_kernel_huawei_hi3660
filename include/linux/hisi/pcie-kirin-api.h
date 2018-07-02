/*
 * PCIe host controller driver for Kirin 960 SoCs
 *
 * Copyright (C) 2015 Huawei Electronics Co., Ltd.
 *		http://www.huawei.com
 *
 * Author: Xiaowei Song <songxiaowei@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _PCIE_KIRIN_API_H
#define _PCIE_KIRIN_API_H


enum kirin_pcie_event {
	KIRIN_PCIE_EVENT_MIN_INVALID = 0x0,		/*min invalid value*/
	KIRIN_PCIE_EVENT_LINKUP = 0x1,		/* linkup event  */
	KIRIN_PCIE_EVENT_LINKDOWN = 0x2,		/* linkdown event */
	KIRIN_PCIE_EVENT_WAKE = 0x4,	/* wake event*/
	KIRIN_PCIE_EVENT_L1SS = 0x8,	/* l1ss event*/
	KIRIN_PCIE_EVENT_MAX_INVALID = 0xF,	/* max invalid value*/
};

enum kirin_pcie_trigger {
	KIRIN_PCIE_TRIGGER_CALLBACK,
	KIRIN_PCIE_TRIGGER_COMPLETION,
};

struct kirin_pcie_notify {
	enum kirin_pcie_event event;
	void *user;
	void *data;
	u32 options;
};


struct kirin_pcie_register_event {
	u32 events;
	void *user;
	enum kirin_pcie_trigger mode;
	void (*callback)(struct kirin_pcie_notify *notify);
	struct kirin_pcie_notify notify;
	struct completion *completion;
	u32 options;
};

int kirin_pcie_register_event(struct kirin_pcie_register_event *reg);
int kirin_pcie_deregister_event(struct kirin_pcie_register_event *reg);


#endif

