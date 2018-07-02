/*
 * DUBAI common header file.
 *
 * Copyright (C) 2017 Huawei Device Co.,Ltd.
 */

#ifndef DUBAI_COMMON_H
#define DUBAI_COMMON_H

#include <linux/sched.h>

#define DUBAI_LOG_TAG	"DUBAI"

#define DUBAI_DEBUG		1
#define DUBAI_INFO		1

#define DUBAI_LOGD(fmt, ...) \
	do { \
		if (DUBAI_DEBUG) \
			pr_debug("["DUBAI_LOG_TAG"] %s: "fmt"\n", \
					__func__, ##__VA_ARGS__); \
	} while (0)

#define DUBAI_LOGI(fmt, ...) \
	do { \
		if (DUBAI_INFO) \
			pr_info("["DUBAI_LOG_TAG"] %s: "fmt"\n", \
					__func__, ##__VA_ARGS__); \
	} while (0)

#define DUBAI_LOGE(fmt, ...) \
	pr_err("["DUBAI_LOG_TAG"] %s: "fmt"\n", __func__, ##__VA_ARGS__)

#define NAME_LEN					(48)
enum {
	BUFFERED_LOG_MAGIC_PID = 0,
	BUFFERED_LOG_MAGIC_KWORKER,
	BUFFERED_LOG_MAGIC_UEVENT,
};

struct polling_event {
	long long timestamp;
	int event;
} __packed;

struct process_name {
	pid_t pid;
	char comm[TASK_COMM_LEN];
	char name[NAME_LEN];
} __packed;

struct dev_transmit_t {
	int length;
	char data[0];
} __packed;

struct buffered_log_entry {
	int length;
	int magic;
	unsigned char data[0];
} __packed;

void dubai_pid_cputime_init(void);
void dubai_pid_cputime_exit(void);
int dubai_get_pid_cputime(struct polling_event *polling);
int dubai_get_process_name(struct process_name *process);

void dubai_gpu_init(void);
void dubai_gpu_exit(void);
void dubai_set_gpu_enable(bool enable);
int dubai_get_gpu_info(unsigned long arg);

extern int (*send_buffered_log)(const struct buffered_log_entry *entry);
struct buffered_log_entry *create_buffered_log_entry(long long size, int magic);
void free_buffered_log_entry(struct buffered_log_entry *entry);
void buffered_log_release(void);

void dubai_stats_init(void);
void dubai_stats_exit(void);
void dubai_log_stats_enable(bool enable);
int dubai_get_kworker_info(long long timestamp);
int dubai_get_uevent_info(long long timestamp);

#endif
