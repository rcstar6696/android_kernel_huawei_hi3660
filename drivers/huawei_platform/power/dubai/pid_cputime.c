/*
 * DUBAI pid cputime.
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
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/net.h>
#include <linux/profile.h>
#include <linux/slab.h>

#include <huawei_platform/power/dubai/dubai_common.h>

#define PID_HASH_BITS			(10)
#define MAX_CMDLINE_LEN			(128)
#define BASE_COUNT				(500)
#define LOG_ENTRY_SIZE(count) \
	sizeof(struct pid_cputime_transmit) \
		+ (long long)(count) * sizeof(struct pid_cputime)

enum {
	PROCESS_STATE_DEAD = 0,
	PROCESS_STATE_ACTIVE,
	PROCESS_STATE_INVALID,
};

enum {
	TASK_STATE_RUNNING = 0,
	TASK_STATE_SLEEPING,
	TASK_STATE_DISK_SLEEP,
	TASK_STATE_STOPPED,
	TASK_STATE_TRACING_STOP,
	TASK_STATE_DEAD,
	TASK_STATE_ZOMBIE,
};

static const int task_state_array[] = {
	TASK_STATE_RUNNING,
	TASK_STATE_SLEEPING,
	TASK_STATE_DISK_SLEEP,
	TASK_STATE_STOPPED,
	TASK_STATE_TRACING_STOP,
	TASK_STATE_DEAD,
	TASK_STATE_ZOMBIE,
};

struct pid_cputime {
	uid_t uid;
	pid_t pid;
	unsigned long long time;
	bool cmdline;
	char name[NAME_LEN];
} __packed;

static struct pid_entry {
	pid_t pid;
	uid_t uid;
	cputime_t utime;
	cputime_t stime;
	cputime_t active_utime;
	cputime_t active_stime;
	bool alive;
	char name[NAME_LEN];
	char comm[TASK_COMM_LEN];
	struct hlist_node hash;
};

static struct pid_cputime_transmit {
	long long timestamp;
	int event;
	int type;
	int count;
	unsigned char value[0];
} __packed;

static DECLARE_HASHTABLE(pid_hash_table, PID_HASH_BITS);
static DEFINE_MUTEX(pid_lock);

static long long polling_timestamp;
static int polling_event;
static int dead_count;
static int active_count;

/*
 * Create log entry to store pid cputime structures
 */
static struct buffered_log_entry *dubai_create_log_entry(int count, int type)
{
	long long size = 0;
	struct buffered_log_entry *entry = NULL;
	struct pid_cputime_transmit *transmit = NULL;

	if ((count < 0) || (type >= PROCESS_STATE_INVALID)) {
		DUBAI_LOGE("Invalid parameter");
		return NULL;
	}

	/*
	 * allocate more space(BASE_COUNT)
	 * size = pid_cputime_transmit size + pid_cputime size * count
	 */
	count += BASE_COUNT;
	size = LOG_ENTRY_SIZE(count);
	entry = create_buffered_log_entry(size, BUFFERED_LOG_MAGIC_PID);
	if (entry == NULL) {
		DUBAI_LOGE("Failed to create buffered log entry");
		return NULL;
	}

	transmit = (struct pid_cputime_transmit *)entry->data;
	transmit->timestamp = polling_timestamp;
	transmit->event = polling_event;
	transmit->type = type;
	transmit->count = count;

	return entry;
}

static unsigned long long dubai_cputime_to_usecs(cputime_t time)
{
	return ((unsigned long long)
		jiffies_to_msecs(cputime_to_jiffies(time)) * USEC_PER_MSEC);
}

static void dubai_copy_name(char *to, char *from, int len)
{
	char *p;

	if (strlen(from) <= len) {
		strncpy(to, from, len);
		return;
	}

	p = strrchr(from, '/');
	if (p != NULL && (*(p + 1) != '\0'))
		strncpy(to, p + 1, len);
	else
		strncpy(to, from, len);
}

static int dubai_pid_entry_copy(const struct pid_entry *pid_entry,
		unsigned char *value)
{
	struct pid_cputime stat;
	cputime_t total_time;

	total_time = pid_entry->active_utime + pid_entry->active_stime;
	total_time += pid_entry->utime + pid_entry->stime;
	if (total_time == 0)
		return -1;

	memset(&stat, 0, sizeof(struct pid_cputime));
	stat.uid = pid_entry->uid;
	stat.pid = pid_entry->pid;
	stat.time = dubai_cputime_to_usecs(total_time);
	if (strlen(pid_entry->name) > 0) {
		stat.cmdline = true;
		dubai_copy_name(stat.name, pid_entry->name, NAME_LEN - 1);
	} else {
		stat.cmdline = false;
		dubai_copy_name(stat.name, pid_entry->comm, NAME_LEN - 1);
	}

	memcpy(value, &stat, sizeof(struct pid_cputime));

	return 0;
}

static inline int dubai_get_task_state(const struct task_struct *task)
{
	unsigned int state = (task->state | task->exit_state) & TASK_REPORT;

	BUILD_BUG_ON(1 + ilog2(TASK_REPORT)
			!= ARRAY_SIZE(task_state_array) - 1);

	return task_state_array[fls(state)];
}

static bool dubai_group_leader_alive(const struct task_struct *task)
{
	struct task_struct *leader = NULL;

	if (task == NULL) {
		DUBAI_LOGE("Invalid parameter");
		return false;
	}

	leader = task->group_leader;
	if ((leader == NULL)
		|| (leader->flags & PF_EXITING)
		|| (leader->flags & PF_EXITPIDONE)
		|| (leader->flags & PF_SIGNALED)
		|| (dubai_get_task_state(leader) >= TASK_STATE_DEAD)) {
		return false;
	}

	return true;
}

static struct pid_entry *dubai_find_pid_entry(pid_t pid)
{
	struct pid_entry *pid_entry;

	hash_for_each_possible(pid_hash_table, pid_entry, hash, pid) {
		if (pid_entry->pid == pid)
			return pid_entry;
	}

	return NULL;
}

static struct pid_entry *dubai_find_or_register_pid(const struct task_struct *task)
{
	struct pid_entry *pid_entry = NULL;
	pid_t pid = task->tgid;
	char *p = NULL;
	char comm[TASK_COMM_LEN] = {0};
	int len = TASK_COMM_LEN - 1;
	bool kthread = false;
	struct task_struct *leader = task->group_leader;

	if (leader != NULL)
		strncpy(comm, leader->comm, len);
	else
		strncpy(comm, task->comm, len);

	if (task->flags & PF_KTHREAD) {
		kthread = true;
		len = NAME_LEN - 1;
	}

	pid_entry = dubai_find_pid_entry(pid);
	if (pid_entry != NULL) {
		p = kthread ? pid_entry->name : pid_entry->comm;
		if (strncmp(p, comm, len))
			strncpy(p, comm, len);

		return pid_entry;
	}

	pid_entry = kzalloc(sizeof(struct pid_entry), GFP_ATOMIC);
	if (pid_entry == NULL) {
		DUBAI_LOGE("Failed to allocate memory");
		return NULL;
	}
	pid_entry->pid = pid;
	pid_entry->uid = from_kuid_munged(current_user_ns(), task_uid(task));
	pid_entry->alive = true;
	p = kthread ? pid_entry->name : pid_entry->comm;
	strncpy(p, comm, len);

	hash_add(pid_hash_table, &pid_entry->hash, pid);
	active_count++;

	return pid_entry;
}

int dubai_update_pid_cputime(void)
{
	bool alive;
	cputime_t utime;
	cputime_t stime;
	struct pid_entry *pid_entry;
	struct task_struct *task, *temp;

	rcu_read_lock();
	/* update active time from alive task */
	do_each_thread(temp, task) {
		/*
		 * check group leader whether it is alive or not
		 * if not, do not record this task's cpu time
		 */
		alive = dubai_group_leader_alive(task);
		if (!alive)
			continue;

		pid_entry = dubai_find_or_register_pid(task);
		if (pid_entry == NULL) {
			rcu_read_unlock();
			DUBAI_LOGE("Failed to find the pid_entry for pid %d",
				task->tgid);
			return -ENOMEM;
		}
		task_cputime_adjusted(task, &utime, &stime);
		pid_entry->active_utime += utime;
		pid_entry->active_stime += stime;
	} while_each_thread(temp, task);
	rcu_read_unlock();

	return 0;
}

static int dubai_clear_and_update(void)
{
	int ret = 0, count = 0;
	unsigned char *value;
	unsigned long bkt;
	struct pid_entry *pid_entry;
	struct hlist_node *tmp;
	struct pid_cputime_transmit *transmit;
	struct buffered_log_entry *entry;

	entry = dubai_create_log_entry(dead_count, PROCESS_STATE_DEAD);
	if (entry == NULL) {
		DUBAI_LOGE("Failed to create log entry for dead processes");
		return -ENOMEM;
	}
	transmit = (struct pid_cputime_transmit *)(entry->data);
	value = transmit->value;
	count = transmit->count;
	transmit->count = 0;

	mutex_lock(&pid_lock);
	/*
	 * initialize hash list
	 * report dead process and delete hash node
	 */
	hash_for_each_safe(pid_hash_table, bkt, tmp, pid_entry, hash) {
		pid_entry->active_stime = 0;
		pid_entry->active_utime = 0;

		if (!pid_entry->alive) {
			if (transmit->count < count) {
				if (dubai_pid_entry_copy(pid_entry, value) == 0) {
					value += sizeof(struct pid_cputime);
					transmit->count++;
				}
			}
			hash_del(&pid_entry->hash);
			kfree(pid_entry);
		}
	}
	dead_count = 0;

	ret = dubai_update_pid_cputime();
	if (ret < 0) {
		DUBAI_LOGE("Failed to update process cpu time");
		mutex_unlock(&pid_lock);
		goto out;
	}
	mutex_unlock(&pid_lock);

	if (transmit->count > 0) {
		entry->length = LOG_ENTRY_SIZE(transmit->count);
		ret = send_buffered_log(entry);
		if (ret < 0)
			DUBAI_LOGE("Failed to send dead process log entry");
	}

out:
	free_buffered_log_entry(entry);

	return ret;
}

static int dubai_send_active_process(void)
{
	int ret = 0, count= 0;
	unsigned char *value;
	unsigned long bkt;
	struct pid_entry *pid_entry;
	struct pid_cputime_transmit *transmit;
	struct buffered_log_entry *entry;

	entry = dubai_create_log_entry(active_count, PROCESS_STATE_ACTIVE);
	if (entry == NULL) {
		DUBAI_LOGE("Failed to create log entry for active processes");
		return -ENOMEM;
	}
	transmit = (struct pid_cputime_transmit *)(entry->data);
	value = transmit->value;
	count = transmit->count;
	transmit->count = 0;

	mutex_lock(&pid_lock);
	hash_for_each(pid_hash_table, bkt, pid_entry, hash) {
		cputime_t active_time;

		active_time = pid_entry->active_utime + pid_entry->active_stime;
		if (active_time == 0 || !pid_entry->alive)
			continue;

		if (transmit->count >= count)
			break;

		if (dubai_pid_entry_copy(pid_entry, value) == 0) {
			value += sizeof(struct pid_cputime);
			transmit->count++;
		}
	}
	mutex_unlock(&pid_lock);

	if (transmit->count > 0) {
		entry->length = LOG_ENTRY_SIZE(transmit->count);
		ret = send_buffered_log(entry);
		if (ret < 0)
			DUBAI_LOGE("Failed to send active process log entry");
	}
	free_buffered_log_entry(entry);

	return ret;
}

/*
 * if there are dead processes in the list,
 * we should clear these dead processes
 * in case of pid reused
 */
int dubai_get_pid_cputime(struct polling_event *event)
{
	int ret = 0;

	if (event == NULL) {
		DUBAI_LOGE("Invalid parameter");
		return -EINVAL;
	}

	polling_timestamp = event->timestamp;
	polling_event = event->event;

	ret = dubai_clear_and_update();
	if (ret < 0) {
		DUBAI_LOGE("Failed to clear dead process and update list");
		goto out;
	}

	ret = dubai_send_active_process();
	if (ret < 0) {
		DUBAI_LOGE("Failed to send active process cpu time");
	}

out:
	return ret;
}

int dubai_get_process_name(struct process_name *process)
{
	struct task_struct *task, *leader;
	char cmdline[MAX_CMDLINE_LEN] = {0};
	int ret = 0, compare = 0;

	if (process == NULL || process->pid <= 0) {
		DUBAI_LOGE("Invalid parameter");
		return -EINVAL;
	}

	rcu_read_lock();
	task = find_task_by_vpid(process->pid);
	if (task)
		get_task_struct(task);
	rcu_read_unlock();
	if (!task)
		return -EFAULT;

	leader = task->group_leader;
	compare = strncmp(process->comm, task->comm, TASK_COMM_LEN - 1);
	if (compare != 0 && leader != NULL)
		compare = strncmp(process->comm, leader->comm, TASK_COMM_LEN - 1);

	ret = get_cmdline(task, cmdline, MAX_CMDLINE_LEN - 1);
	cmdline[MAX_CMDLINE_LEN - 1] = '\0';
	if (ret > 0 && (compare == 0 || strstr(cmdline, process->comm) != NULL))
		dubai_copy_name(process->name, cmdline, NAME_LEN - 1);

out:
	put_task_struct(task);
	return ret;
}

static int dubai_process_notifier(struct notifier_block *self,
		unsigned long cmd, void *v)
{
	int ret;
	bool got_cmdline = false;
	cputime_t utime, stime;
	struct task_struct *task = v;
	pid_t pid = task->tgid;
	struct pid_entry *pid_entry;
	struct task_struct *leader = task->group_leader;
	char cmdline[MAX_CMDLINE_LEN] = {0};

	if (task == NULL)
		return NOTIFY_OK;

	if (!(task->flags & PF_KTHREAD) && pid == task->pid) {
		ret = get_cmdline(task, cmdline, MAX_CMDLINE_LEN - 1);
		cmdline[MAX_CMDLINE_LEN - 1] = '\0';
		if (ret > 0)
			got_cmdline = true;
	}

	mutex_lock(&pid_lock);

	if (pid == task->pid || dubai_group_leader_alive(task)) {
		pid_entry = dubai_find_or_register_pid(task);
		if (pid_entry != NULL && got_cmdline)
			dubai_copy_name(pid_entry->name, cmdline, NAME_LEN - 1);
	} else {
		pid_entry = dubai_find_pid_entry(pid);
	}

	if (pid_entry == NULL)
		goto exit;

	if (!pid_entry->alive) {
		ret = strncmp(task->comm, pid_entry->comm, TASK_COMM_LEN - 1);
		if (leader != NULL)
			ret &= strncmp(leader->comm, pid_entry->comm, TASK_COMM_LEN - 1);
		if (ret != 0)
			goto exit;
	}

	task_cputime_adjusted(task, &utime, &stime);
	pid_entry->utime += utime;
	pid_entry->stime += stime;

	/* pid has died */
	if (pid_entry->pid == task->pid) {
		pid_entry->alive = false;
		active_count--;
		dead_count++;
	}

exit:
	mutex_unlock(&pid_lock);
	return NOTIFY_OK;
}

static struct notifier_block process_notifier_block = {
	.notifier_call	= dubai_process_notifier,
};

void dubai_pid_cputime_init(void)
{
	hash_init(pid_hash_table);
	dead_count = 0;
	active_count = 0;
	profile_event_register(PROFILE_TASK_EXIT, &process_notifier_block);
}

void dubai_pid_cputime_exit(void)
{
	struct pid_entry *pid_entry = NULL;
	struct hlist_node *tmp;
	unsigned long bkt;

	mutex_lock(&pid_lock);
	hash_for_each_safe(pid_hash_table, bkt, tmp, pid_entry, hash) {
		hash_del(&pid_entry->hash);
		kfree(pid_entry);
	}
	dead_count = 0;
	active_count = 0;
	profile_event_unregister(PROFILE_TASK_EXIT, &process_notifier_block);
	mutex_unlock(&pid_lock);
}
