/* vim:set ts=4 sw=4 tw=0 noet ft=c:
 *
 * fs/sdcardfs/configfs.c
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
#include <linux/configfs.h>

static ssize_t
sdcardfs_configfs_pkgdir_appid_show(
	struct config_item *item, char *page
) {
	struct sdcardfs_packagelist_entry *pkg = container_of(
		item, struct sdcardfs_packagelist_entry, item);

	return scnprintf(page, PAGE_SIZE, "%d\n", (int)pkg->appid);
}

static ssize_t sdcardfs_configfs_pkgdir_appid_store(
	struct config_item *item,
	const char *page, size_t count
) {
	unsigned int tmp;
	int ret;

	struct sdcardfs_packagelist_entry *pkg = container_of(
		item, struct sdcardfs_packagelist_entry, item);

	ret = kstrtouint(page, 10, &tmp);
	if (ret)
		return ret;

	pkg->appid = tmp;
	return count;
}

/* all users should have R & W permission */
#define SDCARDFS_CONFIGFS_ATTR(_pfx, _name)	\
static struct configfs_attribute _pfx##attr_##_name = {	\
	.ca_name    = __stringify(_name),	\
	.ca_mode    = S_IRUGO | S_IWUGO,	\
	.ca_owner   = THIS_MODULE,	\
	.show       = _pfx##_name##_show,	\
	.store      = _pfx##_name##_store,	\
}

SDCARDFS_CONFIGFS_ATTR(sdcardfs_configfs_pkgdir_, appid);

/* there exists a pkgdir and some attributes for
 * each Android package dir under rootdir */
static struct configfs_attribute *sdcardfs_configfs_pkgdir_attrs[] = {
	&sdcardfs_configfs_pkgdir_attr_appid,
	NULL,
};

/* remove a pkgdir */
static
void sdcardfs_configfs_pkgdir_item_release(
	struct config_item *item
) {
	struct sdcardfs_packagelist_entry *pkg = container_of(
		item, struct sdcardfs_packagelist_entry, item);

	sdcardfs_packagelist_entry_release(pkg);
}

static struct configfs_item_operations sdcardfs_configfs_pkgdir_item_ops = {
	.release	= sdcardfs_configfs_pkgdir_item_release,
};

static struct config_item_type sdcardfs_configfs_pkgdir_type = {
	.ct_attrs	= sdcardfs_configfs_pkgdir_attrs,
	.ct_item_ops	= &sdcardfs_configfs_pkgdir_item_ops,
	.ct_owner	= THIS_MODULE,
};

/* rootdir consists of all pkgdirs + an extersion dir */
static
struct config_item *
sdcardfs_configfs_rootdir_group_make_item(
	struct config_group *group,
	const char *name
) {
	struct sdcardfs_packagelist_entry *pkg;

	pkg = sdcardfs_packagelist_entry_alloc();
	if (pkg == NULL)
		return NULL;
	config_item_init_type_name(&pkg->item,
		name, &sdcardfs_configfs_pkgdir_type);
	sdcardfs_packagelist_entry_register(pkg, name, 0);
	return &pkg->item;
}

static struct configfs_group_operations sdcardfs_configfs_rootdir_group_ops = {
	.make_item	= sdcardfs_configfs_rootdir_group_make_item,
};

static struct config_item_type sdcardfs_configfs_rootdir_type = {
	.ct_owner	= THIS_MODULE,
	.ct_group_ops	= &sdcardfs_configfs_rootdir_group_ops,
};

static struct configfs_subsystem sdcardfs_configfs_subsys = {
	.su_group = {
		.cg_item = {
			.ci_namebuf = "sdcardfs",
			.ci_type = &sdcardfs_configfs_rootdir_type,
		},
	},
};

int sdcardfs_configfs_init(void)
{
	int ret;
	struct configfs_subsystem *subsys = &sdcardfs_configfs_subsys;

	config_group_init(&subsys->su_group);
	mutex_init(&subsys->su_mutex);
	ret = configfs_register_subsystem(subsys);
	if (ret) {
		errln("failed to register configfs subsystem(%s): %d",
			subsys->su_group.cg_item.ci_namebuf, ret);
	}
	return ret;
}

void sdcardfs_configfs_exit(void) {
	configfs_unregister_subsystem(&sdcardfs_configfs_subsys);
}
