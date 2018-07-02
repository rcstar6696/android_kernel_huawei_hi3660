/* vim:set ts=4 sw=4 tw=0 noet ft=c:
 *
 * fs/sdcardfs/sdcardd.c
 * The file to support AOSP sdcard daemon mount
 *
 * Copyright (C) 2017 HUAWEI, Inc.
 * Author: gaoxiang <gaoxiang25@huawei.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of the Linux
 * distribution for more details.
 */
#include "sdcardfs.h"

static int bind_remount_len;
static char bind_remount_dir_name[PATH_MAX];

int sdcardfs_ignore_bind_mount(char *kernel_dev, char __user *dir_name)
{
	int err;
	struct path devpath;

	if (likely(strcmp(current->comm, "sdcard"))) {
		err = -EINVAL;
		goto out;
	}

	debugln("%s, sdcardd is mounting with MS_BIND", __FUNCTION__);

	/* parse lower path */
	err = kern_path(kernel_dev, LOOKUP_FOLLOW | LOOKUP_DIRECTORY, &devpath);
	if (err) {
		errln("%s, error accessing device '%s'", __FUNCTION__, kernel_dev);
		goto out;
	}

	/* source dev must be a sdcardfs file system */
	if (devpath.mnt->mnt_sb->s_magic != SDCARDFS_SUPER_MAGIC) {
		err = -ENODEV;
		goto out_path_put;
	}

	/* target dir must be start with /mnt/runtime/ */
	bind_remount_len = strncpy_from_user(bind_remount_dir_name, dir_name, PATH_MAX);

	if (unlikely(bind_remount_len < 0) || likely(strncmp(bind_remount_dir_name,
		"/mnt/runtime/", sizeof("/mnt/runtime/")-1))) {
		err = -EINVAL;
		goto out_path_put;
	}

	err = 0;

out_path_put:
	path_put(&devpath);
out:
	return err;
}

int sdcardfs_do_sdcard_remount(char **kernel_dev,
	char __user *dir_name, char *data_page)
{
	int err, len, debug;
	struct path devpath;
	struct sdcardfs_sb_info *sbi;
	struct sdcardfs_mount_options _opts;
	char *kernel_dir;

	if (likely(strcmp(current->comm, "sdcard")
		|| *bind_remount_dir_name == '\0')) {
		err = -EINVAL;
		goto out;
	}

	debugln("%s, sdcardd is mounting with MS_REMOUNT", __FUNCTION__);

	/* parse lower path */
	err = kern_path(*kernel_dev, LOOKUP_FOLLOW | LOOKUP_DIRECTORY, &devpath);
	if (err) {
		errln("%s, error accessing device '%s'", __FUNCTION__, *kernel_dev);
		goto out_sdcard;
	}

	/* source dev must be a sdcardfs file system */
	if (devpath.mnt->mnt_sb->s_magic != SDCARDFS_SUPER_MAGIC) {
		err = -ENODEV;
		goto out_path_put;
	}

	kernel_dir = __getname();
	if (kernel_dir == NULL) {
		err = -ENOMEM;
		goto out_path_put;
	}

	/* target dir must be the same as bind_remount_dir_name */
	len = strncpy_from_user(kernel_dir, dir_name, PATH_MAX);
	err = (len != bind_remount_len ||
		memcmp(kernel_dir, bind_remount_dir_name, len + 1));
	__putname(kernel_dir);

	if (err) {
		err = -EINVAL;
		goto out_path_put;
	}

	sbi = SDCARDFS_SB(devpath.mnt->mnt_sb);
	memcpy(&_opts, &sbi->options,
		sizeof(struct sdcardfs_mount_options));

	/* parse options & fill data_page */
	err = sdcardfs_parse_options(&_opts, data_page, 1, &debug);
	if (err) {
		goto out_path_put;
	}

	*kernel_dev = kstrdup(sbi->devpath_s, GFP_KERNEL);
	if (*kernel_dev == NULL) {
		err = -ENOMEM;
		goto out_path_put;
	}

	/* will always success */
	len = 0;
#define opts     (&_opts)
#define xx(...)  len += sprintf(data_page + len, __VA_ARGS__)
#define __SDCARDFS_MISC__SHOW_OPTIONS
#include "misc.h"

	/* skip the first comma ',' */
	if (data_page[0] == ',') {
		int i;

		for(i = 0; i < len; ++i)
			data_page[i] = data_page[i + 1];	/*lint !e679*/
		data_page[i] = '\0';
	}

	err = 0;
out_path_put:
	path_put(&devpath);
out_sdcard:
	bind_remount_dir_name[0] = '\0';
	bind_remount_len = 0;
out:
	return err;
}

