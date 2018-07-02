#ifndef __SDCARDFS_PACKAGELIST_H
#define __SDCARDFS_PACKAGELIST_H

#include <linux/configfs.h>
#include <linux/hashtable.h>
#include "multiuser.h"

struct sdcardfs_packagelist_entry {
	union {
		struct config_item item;
		struct rcu_head rcu;
	};
	struct hlist_node hlist;
	char *app_name;
	appid_t appid;
};

#endif
