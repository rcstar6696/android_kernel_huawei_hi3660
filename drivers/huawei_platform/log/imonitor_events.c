/**********************************************************
 * Filename:	imonitor_events.c
 *
 * Discription: Interfaces for generate imonitor event struct
				and convert it to regular string which can be
				analysed by imonitor engine
 *
 * Copyright: (C) 2016 huawei.
 *
 * Author: yaomanhai(00303692)
 *
**********************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/version.h>

#include "huawei_platform/log/hw_log.h"
#include "huawei_platform/log/log_exception.h"

#include "huawei_platform/log/imonitor.h"
#include "huawei_platform/log/imonitor_events.h"

#define BIT_TYPE (1)
#define TINYINT_TYPE (2)
#define SMALLINT_TYPE (3)
#define INT_TYPE (4)
#define FLOAT_TYPE (5)
#define DATETIME_TYPE (6)
#define BASE_TYPE_MAX DATETIME_TYPE
#define VARCHAR_TYPE (100)

#define BIT_TYPE_LEN (1)
#define TINYINT_TYPE_LEN (1)
#define SMALLINT_TYPE_LEN (2)
#define INT_TYPE_LEN (4)
#define FLOAT_TYPE_LEN (4)
#define DATETIME_TYPE_LEN (20)

#define MASK_OFFSET_GET_OFFSET(m) ((m) & (unsigned int)0x7FFFFFFF)
#define MASK_OFFSET_SET_MASK(m) ((m) | ((unsigned int)1 << 31))
#define MASK_OFFSET_GET_MASK(m) ((m) >> 31)

#define MAX_PATH_NUMBER (10)
#define MAX_PATH_LEN (256)

/*64K is max length of /dev/hwlog_exception*/
#define EVENT_INFO_BUF_LEN (64*1024)
#define EVENT_INFO_PACK_BUF_LEN (2*1024)

#define HWLOG_TAG	imonitor
HWLOG_REGIST();

#define BUF_POINTER_FORWARD \
	do { \
		if (tmplen < len) { \
			tmp += tmplen; \
			len -= tmplen; \
		} else { \
			hwlog_err("string over length"); \
			tmp += len; \
			len = 0; \
		} \
	} while (0)

/*event obj struct*/
struct imonitor_eventobj {
	unsigned int eventid;
	const unsigned short *desc;
	unsigned int params_count;
	/*
	 * the highest bit is used for mask, others bits used for offset of
	 * the param, its memory is allocated by  imonitor_create_eventobj
	 */
	unsigned int *mask_offset;
	/*
	 * memory used for store all param's values, it's allocated by
	 * imonitor_create_eventobj
	 */
	void *param;
	/*time*/
	long long time;
	/*dynamic file path should be packed and uploaded*/
	char *dynamic_path[MAX_PATH_NUMBER];
	/*
	 * dynamic file path should be packed and uploaded, deleted
	 * after packing and uploading
	 */
	char *dynamic_path_delete[MAX_PATH_NUMBER];
};

static int imonitor_convert_string(struct imonitor_eventobj *eventobj, char **pbuf);

struct imonitor_eventobj *imonitor_create_eventobj(unsigned int eventid)
{
	/*Look for module*/
	unsigned int i;

	const struct imonitor_module_index *module_index = NULL;

	const struct imonitor_event_index *event_index = NULL;

	struct imonitor_eventobj *eventobj = NULL;
	int len = 0;

	for (i = 0; i < imonitor_modules_count; i++) {
		if (eventid >= imonitor_modules_table[i].min_eventid
		&& eventid <= imonitor_modules_table[i].max_eventid) {
			module_index = &(imonitor_modules_table[i]);
			break;
		}
	}

	if (NULL == module_index) {
		hwlog_err("Invalid eventid: %d", eventid);
		return NULL;
	}

	/*Look for eventid description*/
	for (i = 0; i < module_index->events_count; i++) {
		if (eventid == module_index->events[i].eventid) {
			event_index = &(module_index->events[i]);
			break;
		}
	}
	if (NULL == event_index) {
		hwlog_err("Invalid eventid: %d", eventid);
		return NULL;
	}

	/*combined event obj struct*/
	eventobj = vmalloc(sizeof(struct imonitor_eventobj));
	if (NULL == eventobj)
		return NULL;
	eventobj->eventid = eventid;
	hwlog_info("imonitor_create_eventobj: %d", eventid);

	/*Initial path NULL*/
	for (i = 0; i < MAX_PATH_NUMBER; i++) {
		eventobj->dynamic_path[i] = NULL;
		eventobj->dynamic_path_delete[i] = NULL;
	}

	/*Time*/
	eventobj->time = 0;

	/*below are parameters related*/
	if (0 == event_index->params_count) {/*no parameters for this event*/
		eventobj->params_count = 0;
		eventobj->desc = NULL;
		eventobj->mask_offset = NULL;
		eventobj->param = NULL;
		return (void *)eventobj;
	}

	eventobj->params_count = event_index->params_count;
	eventobj->desc = event_index->desc;
	eventobj->mask_offset = vmalloc(eventobj->params_count*sizeof(int));
	if (NULL == eventobj->mask_offset) {
		hwlog_err("No memory for eventobj->mask_offset");
		vfree(eventobj);
		return NULL;
	}
	/*get length for malloc params memory*/
	len = 0;
	for (i = 0; i < eventobj->params_count; i++) {
		unsigned short type = eventobj->desc[i];

		eventobj->mask_offset[i] = len;
		switch (type) {
		case BIT_TYPE:
			len += BIT_TYPE_LEN;
			break;
		case TINYINT_TYPE:
			len += TINYINT_TYPE_LEN;
			break;
		case SMALLINT_TYPE:
			len += SMALLINT_TYPE_LEN;
			break;
		case INT_TYPE:
			len += INT_TYPE_LEN;
			break;
		case FLOAT_TYPE:
			len += FLOAT_TYPE_LEN;
			break;
		case DATETIME_TYPE:
			len += DATETIME_TYPE_LEN;
			break;
		default:
			if (type > BASE_TYPE_MAX && type <= VARCHAR_TYPE) {
				hwlog_err("type error:%d", type);
				vfree(eventobj->mask_offset);
				vfree(eventobj);
				return NULL;
			}
			len += type - VARCHAR_TYPE;
			break;
		}
	}
	eventobj->param = vmalloc(len);
	if (NULL == eventobj->param) {
		vfree(eventobj->mask_offset);
		vfree(eventobj);
		return NULL;
	}

	return (void *)eventobj;
}
EXPORT_SYMBOL(imonitor_create_eventobj);

int imonitor_set_param(struct imonitor_eventobj *eventobj,
		unsigned short paramid, long value)
{
	unsigned short type;
	unsigned int *mask_offset;
	char *param;
	int len;

	if (NULL == eventobj) {
		hwlog_err("Bad param for imonitor_set_param");
		return -EINVAL;
	}

	if (paramid >= eventobj->params_count) {
		hwlog_err("Bad param for imonitor_set_param, paramid: %d",
					paramid);
		return -EINVAL;
	}

	type = eventobj->desc[paramid];
	mask_offset = eventobj->mask_offset + paramid;
	param  = (char *)(eventobj->param)
				+ MASK_OFFSET_GET_OFFSET(*mask_offset);
	len = 0;
	switch (type) {
	case BIT_TYPE:
		if (((char)value) > 1) {
			hwlog_err("Invalid value for bit type, value: %d",
						(int)value);
			return -EINVAL;
		}
		/*go through down*/
	case TINYINT_TYPE:
		memcpy(param, &value, sizeof(char));
		break;
	case SMALLINT_TYPE:
		memcpy(param, &value, sizeof(short));
		break;
	case INT_TYPE:
		memcpy(param, &value, sizeof(int));
		break;
	case FLOAT_TYPE:
		/* kernel not support float */
		memcpy(param, &value, sizeof(int));
		break;
	case DATETIME_TYPE:
		if (0 == value)
			hwlog_err("Invalid value for datetime type");
		else {
			len = DATETIME_TYPE_LEN;
			strncpy((char *)param, (char *)value, len);
			param[len - 1] = '\0';
		}
		break;
	default:
		if (type > BASE_TYPE_MAX && type <= VARCHAR_TYPE) {
			hwlog_err("type error: %d", type);
			return -EINVAL;
		}
		if (0 == value)
			hwlog_err("Invalid value for varchar type");
		else {
			len = type - VARCHAR_TYPE;
			strncpy((char *)param, (char *)value, len);
			param[len - 1] = '\0';
		}
		break;
	}
	*mask_offset = MASK_OFFSET_SET_MASK(*mask_offset);
	return 0;
}
EXPORT_SYMBOL(imonitor_set_param);

int imonitor_unset_param(struct imonitor_eventobj *eventobj,
		unsigned short paramid)
{
	unsigned int *mask_offset;

	if (NULL == eventobj || paramid >= eventobj->params_count) {
		hwlog_err("Bad param for imonitor_unset_param");
		return -EINVAL;
	}

	mask_offset = eventobj->mask_offset + paramid;
	*mask_offset = MASK_OFFSET_GET_OFFSET(*mask_offset);
	return 0;
}
EXPORT_SYMBOL(imonitor_unset_param);

int imonitor_set_time(struct imonitor_eventobj *eventobj, long long seconds)
{
	if (NULL == eventobj || 0 == seconds) {
		hwlog_err("Bad param for imonitor_set_time");
		return -EINVAL;
	}
	eventobj->time = seconds;
	return 0;
}
EXPORT_SYMBOL(imonitor_set_time);

static int add_path(const char **pool, const char *path)
{
	int i;

	if (NULL == path || 0 == path[0]) {
		hwlog_err("Bad param add_path");
		return -EINVAL;
	}

	if (strlen(path) > MAX_PATH_LEN) {
		hwlog_err("file path over max: %d", MAX_PATH_LEN);
		return -EINVAL;
	}

	for (i = 0; i < MAX_PATH_NUMBER; i++) {
		if (NULL != pool[i])
			continue;
		pool[i] = kstrdup(path, GFP_ATOMIC);
		break;
	}

	if (MAX_PATH_NUMBER == i) {
		hwlog_err("Too many pathes");
		return -EINVAL;
	}
	return 0;
}

int imonitor_add_dynamic_path(struct imonitor_eventobj *eventobj,
		const char *path)
{
	if (NULL == eventobj) {
		hwlog_err("Bad param imonitor_add_dynamic_path");
		return -EINVAL;
	}

	return add_path(eventobj->dynamic_path, path);
}
EXPORT_SYMBOL(imonitor_add_dynamic_path);

int imonitor_add_and_del_dynamic_path(struct imonitor_eventobj *eventobj,
		const char *path)
{
	if (NULL == eventobj) {
		hwlog_err("Bad param imonitor_add_and_del_dynamic_path");
		return -EINVAL;
	}

	return add_path(eventobj->dynamic_path_delete, path);
}
EXPORT_SYMBOL(imonitor_add_and_del_dynamic_path);

/* make string ":" to "::", ";" to ";;", and remove newline character
   for example: "abc:def;ghi" transfer to "abc::def;;ghi" */
static char *make_regular(char *value)
{
	int count = 0;
	int len = 0;
	char *temp, *regular, *regular_temp;

	temp = value;
	while (*temp != '\0') {
		if (*temp == ':')
			count++;
		else if (*temp == ';')
			count++;
		else if (*temp == '\n' || *temp == '\r')
			*temp = ' ';
		temp++;
		len++;
	}
	if (count == 0) {
		/* no need to transfer, just return old value */
		return value;
	}
	regular = vmalloc(len + count*2 + 1);
	if (regular == NULL) {
		return NULL;
	}
	regular_temp = regular;
	temp = value;
	while (*temp != 0) {
		if (*temp == ':' || *temp == ';')
			*regular_temp++ = *temp;
		*regular_temp++ = *temp;
		temp++;
	}
	*regular_temp = '\0';
	return regular;
}

static int imonitor_convert_string(struct imonitor_eventobj *eventobj, char **pbuf)
{
	int len;
	char *buf = vmalloc(EVENT_INFO_BUF_LEN);
	if(NULL == buf)
	{
		goto ERRORDONE;
	}
	char *tmp;
	int tmplen;
	unsigned int i = 0;
	unsigned int keycount = 0;

	len = EVENT_INFO_BUF_LEN;
	tmp = buf;
	/*fill eventid*/
	tmplen = snprintf(tmp, len, "eventid %d", eventobj->eventid);
	BUF_POINTER_FORWARD;

	/*fill the path*/
	for (i = 0; i < MAX_PATH_NUMBER; i++) {
		if (NULL == eventobj->dynamic_path[i])
			break;
		tmplen = snprintf(tmp, len, " -i %s",
					eventobj->dynamic_path[i]);
		BUF_POINTER_FORWARD;
	}
	for (i = 0; i < MAX_PATH_NUMBER; i++) {
		if (NULL == eventobj->dynamic_path_delete[i])
			break;
		tmplen = snprintf(tmp, len, " -d %s",
					eventobj->dynamic_path_delete[i]);
		BUF_POINTER_FORWARD;
	}

	/*fill time*/
	if (eventobj->time) {
		tmplen = snprintf(tmp, len, " -t %lld",  eventobj->time);
		BUF_POINTER_FORWARD;
	}

	/*if no parameters for this event*/
	if (0 == eventobj->params_count)
		goto DONE;

	/*fill the param info*/
	keycount = 0;
	for (i = 0; i < eventobj->params_count; i++) {
		unsigned short type;
		unsigned int mask, offset, mask_offset;

		mask_offset = eventobj->mask_offset[i];
		mask = MASK_OFFSET_GET_MASK(mask_offset);
		if (0 == mask)
			continue;
		if (0 == keycount) {
			tmplen = snprintf(tmp, len, " --extra ");
			BUF_POINTER_FORWARD;
		}
		keycount++;
		type = eventobj->desc[i];
		offset = MASK_OFFSET_GET_OFFSET(mask_offset);
		/*fill key*/
		tmplen = snprintf(tmp, len, "%d:", i);
		BUF_POINTER_FORWARD;
		/*fill value*/
		tmplen = 0;
		switch (type) {
		case BIT_TYPE:
		case TINYINT_TYPE: {
			char t = 0;

			memcpy(&t, (unsigned char *)eventobj->param + offset,
					sizeof(char));
			tmplen = snprintf(tmp, len, "%d;", t);
			break;
		}
		case SMALLINT_TYPE: {
			short t = 0;

			memcpy(&t, (unsigned char *)eventobj->param + offset,
					sizeof(short));
			tmplen = snprintf(tmp, len, "%d;", t);
			break;
		}
		case INT_TYPE: {
			int t = 0;

			memcpy(&t, (unsigned char *)eventobj->param + offset,
					sizeof(int));
			tmplen = snprintf(tmp, len, "%d;", t);
			break;
		}
		case FLOAT_TYPE: {
			/* kernel not support float */
			int t = 0;

			memcpy(&t, (unsigned char *)eventobj->param + offset,
					sizeof(int));
			tmplen = snprintf(tmp, len, "%d;", t);
			break;
		}
		case DATETIME_TYPE:
		default: {
				char *value, *regular_value;
				int need_free = 1;

				if (type > BASE_TYPE_MAX &&
						type <= VARCHAR_TYPE) {
					hwlog_err("type error: %d", type);
					goto ERRORDONE;
				}
				value = (char *)eventobj->param + offset;
				regular_value = make_regular(value);
				if (regular_value == NULL) {
					regular_value = "NULL";
					need_free = 0;
				}
				tmplen = snprintf(tmp, len, "%s;",
							regular_value);
				if (value != regular_value && need_free)
					vfree(regular_value);
			break;
		}
		}
		BUF_POINTER_FORWARD;
	}

DONE:
	*pbuf = buf;
	return (EVENT_INFO_BUF_LEN - len);

ERRORDONE:
	if (NULL != buf)
		vfree(buf);
	*pbuf = NULL;
	return 0;
}

static int imonitor_write_log_exception(char *str, const int strlen)
{
	char        tempchr;
	int         nLeftBufLen = strlen+1;
	char       *strptr = str;
	int         sentcnt = 0;

	while (nLeftBufLen > 0) {
		if (nLeftBufLen <= EVENT_INFO_PACK_BUF_LEN) {
			logbuf_to_exception(0, 0, IDAP_LOGTYPE_CMD, 0, strptr, nLeftBufLen);
			sentcnt++;
			break;
		} else {
			tempchr = strptr[EVENT_INFO_PACK_BUF_LEN-1];
			strptr[EVENT_INFO_PACK_BUF_LEN-1] = '\0';
			logbuf_to_exception(0, 0, IDAP_LOGTYPE_CMD, 1, strptr, EVENT_INFO_PACK_BUF_LEN);
			nLeftBufLen -= (EVENT_INFO_PACK_BUF_LEN-1);
			strptr += (EVENT_INFO_PACK_BUF_LEN-1);
			strptr[0] = tempchr;
			sentcnt++;
		}
	}

	return sentcnt;
}

static void imonitor_file_lock(struct file *filp, int cmd)
{
	struct file_lock *fl;

	fl = locks_alloc_lock();
	if (NULL == fl) {
		hwlog_err("imonitor_file_lock alloc error");
		return;
	}
	fl->fl_file = filp;
	fl->fl_owner = filp;
	fl->fl_pid = 0;
	fl->fl_flags = FL_FLOCK;
	fl->fl_type = cmd;
	fl->fl_end = OFFSET_MAX;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(4, 4, 0))
	locks_lock_file_wait(filp, fl);
#else
	flock_lock_file_wait(filp, fl);
#endif
	locks_free_lock(fl);
}

int imonitor_send_event(struct imonitor_eventobj *obj)
{
	char   *str = NULL;
	int     buflen = 0;
	int     nSentPacket = 0;
	struct file *fp;

	if (NULL == obj) {
		hwlog_err("Bad param imonitor_send_event");
		return -EINVAL;
	}

	buflen = imonitor_convert_string(obj, &str);
	if (NULL == str)
		return -EINVAL;

	fp = filp_open("/dev/hwlog_exception", O_WRONLY, 0);
	if (IS_ERR(fp)) {
		hwlog_info("imonitor_send_event open fail");
		nSentPacket = imonitor_write_log_exception(str, buflen);
	} else {
		imonitor_file_lock(fp, F_WRLCK);
		nSentPacket = imonitor_write_log_exception(str, buflen);
		imonitor_file_lock(fp, F_UNLCK);
		filp_close(fp, 0);
	}
	hwlog_info("imonitor send event: %s", str);
	vfree(str);
	return nSentPacket;
}
EXPORT_SYMBOL(imonitor_send_event);

void imonitor_destroy_eventobj(struct imonitor_eventobj *eventobj)
{
	int i;
	if (NULL == eventobj)
		return;

	if (NULL != eventobj->mask_offset) {
		vfree(eventobj->mask_offset);
		eventobj->mask_offset = NULL;
	}

	if (NULL != eventobj->param) {
		vfree(eventobj->param);
		eventobj->param = NULL;
	}

	for (i = 0; i < MAX_PATH_NUMBER; i++) {
		if (eventobj->dynamic_path[i] != NULL) {
			kfree(eventobj->dynamic_path[i]);
			eventobj->dynamic_path[i] = NULL;
		}
		if (eventobj->dynamic_path_delete[i] != NULL) {
			kfree(eventobj->dynamic_path_delete[i]);
			eventobj->dynamic_path_delete[i] = NULL;
		}
	}

	vfree(eventobj);
}
EXPORT_SYMBOL(imonitor_destroy_eventobj);
