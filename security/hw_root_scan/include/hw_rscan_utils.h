/*
 * hw_rscan_utils.h
 *
 * the hw_rscan_utils.h - get current run mode, eng or user
 *
 * likun <quentin.lee@huawei.com>
 * likan <likan82@huawei.com>
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 */

#ifndef _HW_RSCAN_UTILS_H_
#define _HW_RSCAN_UTILS_H_

#include <asm/segment.h>
#include <linux/buffer_head.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kobject.h>
#include <linux/ratelimit.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include "hw_rscan_interface.h"

/* #define RS_DEBUG */

#define FILE_SIZE_MAX (4096)
#define EOF (-1)
#define LINE_LEN_SHORT (200)

#define DEFAULT_PROP_FILE   "/default.prop"

#define RSCAN_UNINIT    0
#define RSCAN_INIT      1

#define RO_SECURE       0
#define RO_NORMAL       1

/*
 * A convenient interface for error log print, Root Scan Log Error.
 * and the print rate is limited
 * used like below,
 * RSLogError("hw_rscan_utils", "xxx %s xxx", yyy);
 */
#define RSCAN_ERROR "[Error]"
#define RSLogError(tag, fmt, args...) \
	pr_err_ratelimited("%s[%s][%s] " fmt "\n", RSCAN_ERROR,\
						tag, __func__, ##args)

/*
 * A convenient interface for warning log print, Root Scan Log warning.
 * and the print rate is limited
 * used like below,
 * RSLogWarning("hw_rscan_utils", "xxx %s xxx", yyy);
 */
#define RSCAN_WARNING "[WARNING]"
#define RSLogWarning(tag, fmt, args...) \
	pr_warn_ratelimited("%s[%s][%s] " fmt "\n", RSCAN_WARNING,\
						tag, __func__, ##args)

/*
 * A convenient interface for trace log print, Root Scan Log trace.
 * and the print rate is limited
 * used like below,
 * RSLogTrace("hw_rscan_utils", "xxx %s xxx", yyy);
 */
#define RSCAN_TRACE "[TRACE]"
#define RSLogTrace(tag, fmt, args...) \
	pr_info_ratelimited("%s[%s][%s] " fmt "\n", RSCAN_TRACE,\
						tag, __func__, ##args)

#ifdef RS_DEBUG
/*
 * A convenient interface for debug log print, Root Scan Log debug.
 * and the print rate is limited
 * used like below,
 * RSLogDebug("hw_rscan_utils", "xxx %s xxx", yyy);
 */
#define RSCAN_DEBUG "[DEBUG]"
#define RSLogDebug(tag, fmt, args...) \
	printk_ratelimited(KERN_DEBUG "%s[%s][%s] " fmt "\n", RSCAN_DEBUG,\
							tag, __func__, ##args)
#else
#define RSLogDebug(tag, fmt, args...) no_printk(fmt, ##args)
#endif

/*
 * file_open - a helper to open file in kernel space
 * Description: a wrapper interface of filp_open,
 *  open file and return file pointer
 * @path, path to open
 * @flag, open flags, as O_RDONLY, O_WRONLY, O_RDWR, etc.
 * @mode, mode for the new file if O_CREAT is set, else ignored
 * @return:
 *      file pointer.
 */
struct file *file_open(const char *path, int flag, umode_t mode);

/*
 * file_close - a helper to close file in kernel space
 * Description: a wrapper interface of filp_close to close file
 * @filp, file pointer
 */
void file_close(struct file *filp);

/*
 * file_read - a helper to read file in kernel space
 * Description: a wrapper interface of vfs_read,
 *  read contents from file to buffer
 * @filp, file pointer
 * @buf, buffer for user to carry the readed contents
 * @len, size of buf
 * @return:
 *     return actual size of the file, or return failed codes
 */
ssize_t file_read(struct file *filp, char *buf, ssize_t len);

#endif

