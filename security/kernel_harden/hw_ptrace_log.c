/*
 * Huawei Kernel Harden, ptrace log upload
 *
 * Copyright (c) 2016 Huawei.
 *
 * Authors:
 * yinyouzhan <yinyouzhan@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/capability.h>
#include <linux/export.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/pagemap.h>
#include <linux/ptrace.h>
#include <linux/security.h>
#include <linux/signal.h>
#include <linux/uio.h>
#include <linux/audit.h>
#include <linux/pid_namespace.h>
#include <huawei_platform/log/imonitor.h>

#define PTRACE_POKE_STATE_IMONITOR_ID     (940000005)
static int ptrace_do_upload_log(long type, const char * child_cmdline, const char * tracer_cmdline)
{
	struct imonitor_eventobj *obj = NULL;
	int ret = 0;

	obj = imonitor_create_eventobj(PTRACE_POKE_STATE_IMONITOR_ID);
	if (!obj)
		return -1;

	ret += imonitor_set_param(obj, E940000005_ERRTYPE_INT, type);
	ret += imonitor_set_param(obj, E940000005_CHILD_CMDLINE_VARCHAR, (long)child_cmdline);
	ret += imonitor_set_param(obj, E940000005_TRACER_CMDLINE_VARCHAR, (long)tracer_cmdline);
	if (ret) {
		imonitor_destroy_eventobj(obj);
		return ret;
	}
	ret = imonitor_send_event(obj);
	imonitor_destroy_eventobj(obj);
	return ret;
}
int record_ptrace_info_before_return_EIO(long request, struct task_struct *child)
{
	struct task_struct *tracer;
	char tcomm_child[sizeof(child->comm) + 8] = {0}; /*8 is reserved for unknown string*/
	char tcomm_tracer[sizeof(child->comm) + 8] = {0};/*comm size is same within any task*/
	static unsigned int  g_ptrace_log_counter = 0;

	if (child == NULL)
		return -EIO;

	if (g_ptrace_log_counter >= 100) /* only 100 log upload since power on */
		return -EIO;
	g_ptrace_log_counter++;

	(void)get_task_comm(tcomm_child, child);
	rcu_read_lock();
	tracer = ptrace_parent(child);
	if (tracer) {
		(void)get_task_comm(tcomm_tracer, tracer);
	} else {
		(void)strncpy(tcomm_tracer, "unknown", sizeof("unknown"));
	}
	rcu_read_unlock();
	pr_err("child_cmdline=%s, tracer_cmdline=%s\n",tcomm_child,tcomm_tracer);
	(void)ptrace_do_upload_log(request, tcomm_child, tcomm_tracer);
	return -EIO;
}