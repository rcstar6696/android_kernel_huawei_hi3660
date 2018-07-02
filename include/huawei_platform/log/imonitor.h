/**********************************************************
 * Filename:    imonitor.h
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
#ifndef _IMONITOR_H
#define _IMONITOR_H
#include "imonitor_keys.h"

#ifdef __cplusplus
extern "C" {
#endif

struct imonitor_eventobj;

struct imonitor_eventobj *imonitor_create_eventobj(unsigned int eventid);

int imonitor_set_param(struct imonitor_eventobj *eventobj,
		unsigned short paramid, long value);

int imonitor_unset_param(struct imonitor_eventobj *eventobj,
		unsigned short paramid);

int imonitor_set_time(struct imonitor_eventobj *eventobj,
		long long seconds);

int imonitor_add_dynamic_path(struct imonitor_eventobj *eventobj,
		const char *path);

int imonitor_add_and_del_dynamic_path(struct imonitor_eventobj *eventobj,
		const char *path);

int imonitor_send_event(struct imonitor_eventobj *eventobj);

void imonitor_destroy_eventobj(struct imonitor_eventobj *eventobj);

#ifdef __cplusplus
}
#endif
#endif
