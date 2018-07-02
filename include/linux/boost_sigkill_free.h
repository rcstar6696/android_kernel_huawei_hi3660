/*
 * include/linux/boost_sigkill_free.h
 *
 * Boost memory free for SIGKILLed process
 *
 *  Copyright (C) 2016 Huawei Technologies Co., Ltd.
 */
#ifndef _BOOST_SIGKILL_FREE_H
#define _BOOST_SIGKILL_FREE_H

#include <linux/types.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/signal.h>

#define MMF_FAST_FREEING 21

#define sig_kernel_kill(sig) ((sig) == SIGKILL)

#ifdef CONFIG_HW_BOOST_SIGKILL_FREE
extern unsigned int sysctl_boost_sigkill_free;
#else
#define sysctl_boost_sigkill_free 0
#endif

#ifdef CONFIG_HW_BOOST_SIGKILL_FREE
extern void fast_free_user_mem(void);
#else
static inline void fast_free_user_mem(void) { }
#endif

#endif
