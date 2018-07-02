/* vim:set ts=4 sw=4 tw=0 noet ft=c:
 *
 * fs/sdcardfs/packagelist.c
 *
 * Copyright (C) 2017 HUAWEI, Inc.
 * Author: gaoxiang <gaoxiang25@huawei.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of the Linux
 * distribution for more details.
 */
#include "sdcardfs.h"
#include "packagelist.h"
#include <linux/ctype.h>

DEFINE_HASHTABLE(pkgl_hashtable, 8);

/* BKDR Hash Function */
static u32 str_hash(const char *str)
{
	const u32 seed = 131;	/* 31 131 1313 13131 131313 etc.. */
	u32 hash = 0;

	while (*str != '\0')
		hash = hash * seed + tolower(*str++);
	return hash;
}

appid_t get_appid(const char *app_name)
{
#ifdef SDCARDFS_DEBUG_APPID
	return SDCARDFS_DEBUG_APPID;
#else
	appid_t retid = 0;
	struct sdcardfs_packagelist_entry *iter;
	u32 hashval = str_hash(app_name);

	rcu_read_lock();
	hash_for_each_possible_rcu(pkgl_hashtable, iter, hlist, hashval) {	/*lint !e666*/
		if (!strcasecmp(iter->app_name, app_name)) {
			retid = iter->appid;
			break;
		}
	}
	rcu_read_unlock();
	return retid;
#endif
}

struct sdcardfs_packagelist_entry *
sdcardfs_packagelist_entry_alloc(void)
{
	struct sdcardfs_packagelist_entry *pkg;

	pkg = kzalloc(
		sizeof(struct sdcardfs_packagelist_entry),
		GFP_KERNEL);
	return pkg;
}

void sdcardfs_packagelist_entry_register(
	struct sdcardfs_packagelist_entry *pkg,
	const char *app_name, appid_t appid
) {
	pkg->app_name = kstrdup(app_name, GFP_KERNEL);
	pkg->appid = appid;
	hash_add_rcu(pkgl_hashtable, &pkg->hlist,
		str_hash(app_name));	/*lint !e666*/
}

static void __rcu_free(struct rcu_head *rcu)
{
	struct sdcardfs_packagelist_entry *pkg =
		container_of(rcu, struct sdcardfs_packagelist_entry, rcu);
	BUG_ON(pkg->app_name == NULL);
	kfree(pkg->app_name);
	kfree(pkg);
}

void sdcardfs_packagelist_entry_release(
	struct sdcardfs_packagelist_entry *pkg
) {
	hash_del_rcu(&pkg->hlist);
	call_rcu(&pkg->rcu, __rcu_free);
}
