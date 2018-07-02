/*
 * hw_rscan_utils.c
 *
 * the hw_rscan_utils.c - get current run mode, eng or user
 *
 * likun <quentin.lee@huawei.com>
 * likan <likan82@huawei.com>
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 */

#include "./include/hw_rscan_utils.h"
#include <linux/slab.h>
#define INITED 1
#define UNINIT 0

static const char *TAG = "hw_rscan_utils";
static int init_flag = UNINIT;
static int ro_secure_value = RO_SECURE;

struct file *file_open(const char *path, int flag, umode_t mode)
{
	struct file *filp = NULL;
	mm_segment_t oldfs;
	long err = 0;

	if (!path) {
		RSLogError(TAG, "file path is empty");
		return NULL;
	}

	oldfs = get_fs();
	set_fs(get_ds());

	filp = filp_open(path, flag, mode);
	set_fs(oldfs);
	if (IS_ERR(filp)) {
		err = PTR_ERR(filp);
		RSLogError(TAG, "open file %s failed with error code %ld",
					path, err);
		return NULL;
	}

	return filp;
}

void file_close(struct file *filp)
{
	if (filp)
		filp_close(filp, NULL);
	else
		RSLogWarning(TAG, "file pointer is invalid");
}

ssize_t file_read(struct file *filp, char *buf, ssize_t len)
{
	mm_segment_t oldfs;
	ssize_t ret = 0;
	loff_t pos = 0;

	if (NULL == filp || NULL == buf) {
		RSLogError(TAG, "pointer is invalid");
		return 0;
	}

	oldfs = get_fs();
	set_fs(get_ds());
	ret = vfs_read(filp, buf, len, &pos);
	set_fs(oldfs);
	if (ret <= 0) {
		RSLogError(TAG, "read file failed, ret is %d", (int)ret);
		return 0;
	}

	return ret;
}

int _get_next_line(char *buf, ssize_t buf_len, char *line_buf,
				ssize_t line_size)
{
	ssize_t pos = 0;
	ssize_t has_new_line_char = 0;

	while (pos < buf_len) {
		if (buf[pos] == '\n') {
			has_new_line_char = 1;
			break;
		}
		pos++;
	}

	line_size = pos > (line_size - 1) ? (line_size - 1) : pos;
	memcpy(line_buf, buf, line_size);
	line_buf[line_size] = '\0';
	return pos + has_new_line_char;
}

int get_next_line(char *buf, ssize_t buf_len, ssize_t *pos, char *line_buf,
				ssize_t line_size)
{

	if (NULL == buf || NULL == line_buf || NULL == pos || line_size <= 0) {
		RSLogError(TAG, "buf is invalid");
		return -1;
	}

	if ((*pos) >= buf_len) {
		RSLogDebug(TAG, "no more line!");
		return -1;
	}

	int new_line_len = _get_next_line(buf + (*pos),
					buf_len - (*pos), line_buf, line_size);
	if (new_line_len <= 0)
		return -1;

	(*pos) += new_line_len;
	return new_line_len;
}

/* change error return to RO_NORMAL */
int _get_ro_secure(char *file_buf, char *line_buf)
{
	struct file *stream = NULL;
	ssize_t file_size = 0;
	ssize_t cur_pos = 0;

	stream = file_open(DEFAULT_PROP_FILE, O_RDONLY, 0);
	if (NULL == stream) {
		RSLogError(TAG, "open file %s failed", DEFAULT_PROP_FILE);
		return RO_NORMAL;
	}

	file_size = file_read(stream, file_buf, (ulong)FILE_SIZE_MAX);
	file_close(stream);

	if (file_size <= 0) {
		RSLogError(TAG, "read file %s failed, file size incorrect",
					DEFAULT_PROP_FILE);
		return RO_NORMAL;
	}

	while (1) {
		int line_len = get_next_line(file_buf, file_size,
					&cur_pos, line_buf, LINE_LEN_SHORT);

		if (line_len < 0)	/* no more line */
			return RO_NORMAL;

		if (NULL != strstr(line_buf, "ro.secure=0"))
			return RO_SECURE;
		else if (NULL != strstr(line_buf, "ro.secure=1"))
			return RO_NORMAL;
	}

	return RO_NORMAL;
}

int get_ro_secure(void)
{
	char *file_buf = NULL;
	char *line_buf = NULL;
	int ret = RO_SECURE;

	if(INITED == init_flag)
		return ro_secure_value;

	do {
		file_buf = vmalloc((ulong)FILE_SIZE_MAX);
		if (NULL == file_buf) {
			RSLogError(TAG, "no enough space for file_buf");
			ret = -ENOSPC;
			break;
		}
		memset(file_buf, 0, (ulong)FILE_SIZE_MAX);

		line_buf = vmalloc((ulong)LINE_LEN_SHORT);
		if (NULL == line_buf) {
			RSLogError(TAG, "no enough space for line_buf");
			ret = -ENOSPC;
			break;
		}
		memset(line_buf, 0, (ulong)LINE_LEN_SHORT);

		ret = _get_ro_secure(file_buf, line_buf);
		ro_secure_value = ret;
		init_flag = INITED;
	} while (0);

	vfree(file_buf);
	file_buf = NULL;

	vfree(line_buf);
	line_buf = NULL;

	return ret;
}
