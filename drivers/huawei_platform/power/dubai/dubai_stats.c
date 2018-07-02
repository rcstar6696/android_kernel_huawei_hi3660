/*
 * DUBAI kernel statistics.
 *
 * Copyright (C) 2017 Huawei Device Co.,Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/hashtable.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/suspend.h>
#include <linux/time.h>
#include <linux/kallsyms.h>
#include <linux/module.h>
#include <linux/kobject.h>

#include <huawei_platform/log/hwlog_kernel.h>
#include <huawei_platform/power/dubai/dubai.h>
#include <huawei_platform/power/dubai/dubai_common.h>

#define KWORKER_HASH_BITS			(10)
#define IRQ_NAME_SIZE				(128)
#define MAX_SYMBOL_LEN				(48)
#define MAX_DEVPATH_LEN				(128)
#define PRINT_MAX_LEN				(40)
#define LOG_ENTRY_SIZE(head, info, count) \
	sizeof(head) \
		+ (long long)(count) * sizeof(info)

static char irq_name[IRQ_NAME_SIZE];
static int irq_gpio;
static bool resume;
static int kworker_count;
static int uevent_count;
static atomic_t log_stats_enable;

static struct kworker_info {
	long long count;
	long long time;
	char symbol[MAX_SYMBOL_LEN];
} __packed;

struct kworker_transmit {
	long long timestamp;
	int count;
	char data[0];
} __packed;

static struct kworker_entry {
	unsigned long address;
	struct kworker_info info;
	struct hlist_node hash;
};

static struct uevent_info {
	char devpath[MAX_DEVPATH_LEN];
	int actions[KOBJ_MAX];
} __packed;

struct uevent_transmit {
	long long timestamp;
	int action_count;
	int count;
	char data[0];
} __packed;

static struct uevent_entry {
	struct list_head list;
	struct uevent_info uevent;
};

static DECLARE_HASHTABLE(kworker_hash_table, KWORKER_HASH_BITS);
static DEFINE_MUTEX(kworker_lock);
static LIST_HEAD(uevent_list);
static DEFINE_MUTEX(uevent_lock);

static struct kworker_entry *dubai_find_kworker_entry(unsigned long address)
{
	struct kworker_entry *entry = NULL;

	hash_for_each_possible(kworker_hash_table, entry, hash, address) {
		if (entry->address == address)
			return entry;
	}

	entry = kzalloc(sizeof(struct kworker_entry), GFP_ATOMIC);
	if (entry == NULL) {
		DUBAI_LOGE("Failed to allocate memory");
		return NULL;
	}
	entry->address = address;
	kworker_count++;
	hash_add(kworker_hash_table, &entry->hash, address);

	return entry;
}

void dubai_log_kworker(unsigned long address, unsigned long long enter_time)
{
	unsigned long long exit_time;
	struct kworker_entry *entry = NULL;

	if (!atomic_read(&log_stats_enable))
		return;

	exit_time = ktime_get_ns();

	if (!mutex_trylock(&kworker_lock))
		return;

	entry = dubai_find_kworker_entry(address);
	if (entry == NULL) {
		DUBAI_LOGE("Failed to find kworker entry");
		goto out;
	}

	entry->info.count++;
	entry->info.time += exit_time - enter_time;

out:
	mutex_unlock(&kworker_lock);
}

int dubai_get_kworker_info(long long timestamp)
{
	int ret = 0, count = 0;
	long long size = 0;
	unsigned char *data = NULL;
	unsigned long bkt;
	struct kworker_entry *entry;
	struct hlist_node *tmp;
	struct buffered_log_entry *log_entry = NULL;
	struct kworker_transmit *transmit = NULL;

	if (!atomic_read(&log_stats_enable))
		return -EPERM;

	count = kworker_count;
	size = LOG_ENTRY_SIZE(struct kworker_transmit, struct kworker_info, count);
	log_entry = create_buffered_log_entry(size, BUFFERED_LOG_MAGIC_KWORKER);
	if (log_entry == NULL) {
		DUBAI_LOGE("Failed to create buffered log entry");
		return -ENOMEM;
	}
	transmit = (struct kworker_transmit *)log_entry->data;
	transmit->timestamp = timestamp;
	transmit->count = 0;
	data = transmit->data;

	mutex_lock(&kworker_lock);
	hash_for_each_safe(kworker_hash_table, bkt, tmp, entry, hash) {
		if (entry->info.count == 0 || entry->info.time == 0) {
			hash_del(&entry->hash);
			kfree(entry);
			kworker_count--;
			continue;
		}

		if (strlen(entry->info.symbol) == 0) {
			char buffer[KSYM_SYMBOL_LEN] = {0};

			sprint_symbol_no_offset(buffer, entry->address);
			buffer[KSYM_SYMBOL_LEN - 1] = '\0';
			strncpy(entry->info.symbol, buffer, MAX_SYMBOL_LEN - 1);
		}

		if (transmit->count < count) {
			memcpy(data, &entry->info, sizeof(struct kworker_info));
			data += sizeof(struct kworker_info);
			transmit->count++;
		}
		entry->info.count = 0;
		entry->info.time = 0;
	}
	mutex_unlock(&kworker_lock);

	if (transmit->count > 0) {
		log_entry->length = LOG_ENTRY_SIZE(struct kworker_transmit,
								struct kworker_info, transmit->count);
		ret = send_buffered_log(log_entry);
		if (ret < 0)
			DUBAI_LOGE("Failed to send kworker log entry");
	}
	free_buffered_log_entry(log_entry);

	return ret;
}

static struct uevent_entry *dubai_find_uevent_entry(const char *devpath)
{
	struct uevent_entry *entry = NULL;

	list_for_each_entry(entry, &uevent_list, list) {
		if (!strncmp(devpath, entry->uevent.devpath, MAX_DEVPATH_LEN - 1))
			return entry;
	}

	entry = kzalloc(sizeof(struct uevent_entry), GFP_ATOMIC);
	if (entry == NULL) {
		DUBAI_LOGE("Failed to allocate memory");
		return NULL;
	}
	strncpy(entry->uevent.devpath, devpath, MAX_DEVPATH_LEN - 1);
	uevent_count++;
	list_add_tail(&entry->list, &uevent_list);

	return entry;
}

void dubai_log_uevent(const char *devpath, unsigned int action) {
	struct uevent_entry *entry = NULL;

	if (!atomic_read(&log_stats_enable)
		|| devpath == NULL
		|| action >= KOBJ_MAX)
		return;

	mutex_lock(&uevent_lock);
	entry = dubai_find_uevent_entry(devpath);
	if (entry == NULL) {
		DUBAI_LOGE("Failed to find uevent entry");
		goto out;
	}
	(entry->uevent.actions[action])++;

out:
	mutex_unlock(&uevent_lock);
}

int dubai_get_uevent_info(long long timestamp)
{
	int ret = 0, count = 0;
	long long size = 0;
	unsigned char *data = NULL;
	struct uevent_entry *entry, *tmp;
	struct buffered_log_entry *log_entry = NULL;
	struct uevent_transmit *transmit = NULL;

	if (!atomic_read(&log_stats_enable))
		return -EPERM;

	count = uevent_count;
	size = LOG_ENTRY_SIZE(struct uevent_transmit, struct uevent_info, count);
	log_entry = create_buffered_log_entry(size, BUFFERED_LOG_MAGIC_UEVENT);
	if (log_entry == NULL) {
		DUBAI_LOGE("Failed to create buffered log entry");
		return -ENOMEM;
	}
	transmit = (struct uevent_transmit *)log_entry->data;
	transmit->timestamp = timestamp;
	transmit->action_count = KOBJ_MAX;
	transmit->count = 0;
	data = transmit->data;

	mutex_lock(&uevent_lock);
	list_for_each_entry_safe(entry, tmp, &uevent_list, list) {
		int i, total;

		for (i = 0, total = 0; i < KOBJ_MAX; i++) {
			total += entry->uevent.actions[i];
		}

		if (total == 0) {
			list_del_init(&entry->list);
			kfree(entry);
			uevent_count--;
			continue;
		}
		if (transmit->count < count) {
			memcpy(data, &entry->uevent, sizeof(struct uevent_info));
			data += sizeof(struct uevent_info);
			transmit->count++;
		}
		memset(&(entry->uevent.actions), 0, KOBJ_MAX * sizeof(int));
	}
	mutex_unlock(&uevent_lock);

	if (transmit->count > 0) {
		log_entry->length = LOG_ENTRY_SIZE(struct uevent_transmit,
								struct uevent_info, transmit->count);
		ret = send_buffered_log(log_entry);
		if (ret < 0)
			DUBAI_LOGE("Failed to send uevent log entry");
	}
	free_buffered_log_entry(log_entry);

	return ret;
}

void dubai_log_stats_enable(bool enable)
{
	atomic_set(&log_stats_enable, enable ? 1 : 0);
}

void dubai_update_wakeup_info(const char *name, int gpio)
{
	if (name == NULL) {
		DUBAI_LOGE("Invalid parameter");
		return;
	}

	strncpy(irq_name, name, IRQ_NAME_SIZE - 1);
	irq_gpio = gpio;
	resume = true;
}

void ipf_get_waking_pkt(void* data, unsigned int len)
{
	if(data == NULL) {
		DUBAI_LOGE(" invalid param");
		return;
	}
	print_hex_dump(KERN_INFO, "", DUMP_PREFIX_ADDRESS, 16, 1, data, PRINT_MAX_LEN, 0);
	return;
}

static int dubai_pm_notify(struct notifier_block *nb,
			unsigned long mode, void *data)
{
	switch (mode) {
	case PM_SUSPEND_PREPARE:
		HWDUBAI_LOGE("DUBAI_TAG_KERNEL_SUSPEND", "");
		break;
	case PM_POST_SUSPEND:
		if (resume) {
			HWDUBAI_LOGE("DUBAI_TAG_KERNEL_WAKEUP",
				"irq=%s gpio=%d", irq_name, irq_gpio);
			resume = false;
		}
		HWDUBAI_LOGE("DUBAI_TAG_KERNEL_RESUME", "");
		break;
	default:
		break;
	}

	return 0;
}

static struct notifier_block dubai_pm_nb = {
	.notifier_call = dubai_pm_notify,
};

void dubai_stats_init(void)
{
	atomic_set(&log_stats_enable, 0);
	hash_init(kworker_hash_table);
	kworker_count= 0;
	uevent_count = 0;
	resume = false;
	register_pm_notifier(&dubai_pm_nb);
}

void dubai_stats_exit(void)
{
	struct kworker_entry *kworker = NULL;
	struct hlist_node *tmp;
	struct uevent_entry *uevent, *temp;
	unsigned long bkt;

	mutex_lock(&kworker_lock);
	hash_for_each_safe(kworker_hash_table, bkt, tmp, kworker, hash) {
		hash_del(&kworker->hash);
		kfree(kworker);
	}
	kworker_count = 0;
	mutex_unlock(&kworker_lock);

	mutex_lock(&uevent_lock);
	list_for_each_entry_safe(uevent, temp, &uevent_list, list) {
		list_del_init(&uevent->list);
		kfree(uevent);
	}
	uevent_count = 0;
	mutex_unlock(&uevent_lock);

	unregister_pm_notifier(&dubai_pm_nb);
}
