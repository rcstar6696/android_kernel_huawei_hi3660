/* vim:set ts=4 sw=4 tw=0 noet ft=c:
 *
 * fs/sdcardfs/misc.h
 * Reusable code-snippets / Utilities for the sdcardfs implementation
 *
 * Copyright (C) 2017 HUAWEI, Inc.
 * Author: gaoxiang <gaoxiang25@huawei.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of the Linux
 * distribution for more details.
 */
#if defined(__SDCARDFS_MISC__SHOW_OPTIONS)
#undef __SDCARDFS_MISC__SHOW_OPTIONS
	if (opts->fs_low_uid)
		xx(",fsuid=%u", opts->fs_low_uid);
	if (opts->fs_low_gid)
		xx(",fsgid=%u", opts->fs_low_gid);
	if (opts->gid)
		xx(",gid=%u", opts->gid);
	if (opts->multiuser)
		xx(",multiuser");
	if (opts->mask)
		xx(",mask=%u", opts->mask);
	if (opts->fs_user_id)
		xx(",userid=%u", opts->fs_user_id);
	if (opts->reserved_mb)
		xx(",reserved_mb=%u", opts->reserved_mb);
	if (opts->quiet)
		xx(",quiet");
#else
#error precompiled macro is not defined
#endif

