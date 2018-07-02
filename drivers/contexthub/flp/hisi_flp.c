/*
 * hisi flp driver.
 *
 * Copyright (C) 2015 huawei Ltd.
 * Author:lijiangxiong <lijingxiong@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/notifier.h>
#include <linux/syscalls.h>
#include <net/genetlink.h>
#include <linux/workqueue.h>
#include <linux/hisi/hisi_syscounter.h>
#include <soc_acpu_baseaddr_interface.h>
#include <soc_syscounter_interface.h>
#include <clocksource/arm_arch_timer.h>
#include <linux/wakelock.h>
#include "hisi_flp.h"
#include "../inputhub_api.h"
#include "hisi_softtimer.h"
#include <libhwsecurec/securec.h>

/*lint -e750 -esym(750,*) */
#define HISI_FLP_DEBUG              KERN_INFO
#define FLP_PDR_DATA	(0x1<<0)
#define FLP_BATCHING	(0x1<<2)
#define FLP_GEOFENCE	(0x1<<3)

#define PDR_INTERVAL_MIN	1000
#define PDR_INTERVAL_MXN	(3600*1000)
#define PDR_PRECISE_MIN	PDR_INTERVAL_MIN
#define PDR_PRECISE_MXN	60000
#define PDR_DATA_MAX_COUNT      300
#define PDR_DATA_MIX_COUNT      1
#define PDR_SINGLE_SEND_COUNT   48
#ifndef TRUE
#define  TRUE   1
#endif
/*lint +e750 +esym(750,*) */

typedef struct flp_device {
	struct list_head        list;
	pdr_start_config_t      pdr_config;
	unsigned int            pdr_start_count;
	unsigned int            pdr_flush_config ;
	unsigned int            service_type ;
	struct mutex            lock;
	unsigned int            pdr_cycle;
	compensate_data_t       pdr_compensate;
	unsigned int            denial_sevice;
	long                    pdr_last_utc;
} flp_device_t;

typedef struct flp_data_buf{
    char                  *data_buf;
    unsigned int          buf_size;
    unsigned int          read_index;
    unsigned int          write_index;
    unsigned int          data_count;
} flp_data_buf_t;

typedef struct flp_port {
	struct list_head        list;
	unsigned int            port_type ;
	unsigned int            channel_type;
	struct softtimer_list   sleep_timer ;
	flp_data_buf_t          pdr_buf;
	pdr_start_config_t      pdr_config;
	pdr_start_config_t      pdr_update_config;
	batching_config_t       gps_batching_config;
	unsigned int            rate ;          /*control get pdr data*/
	unsigned int            interval ;       /*control get pdr data*/
	unsigned int            nextid;         /*control get pdr data*/
	unsigned int            aquiredid;      /*control get pdr data*/
	unsigned int            pdr_flush_config  ;
	unsigned int            portid;
	unsigned long           total_count ;
	struct work_struct      work;
	unsigned int            work_para;
	unsigned int            need_awake;
	unsigned int            need_report;
	compensate_data_t       pdr_compensate;
	struct wake_lock        wlock;
	unsigned int            need_hold_wlock;
} flp_port_t;

#ifndef STUB_FOR_TEST
#define PDR_REGISTER_CALLBACK       register_mcu_event_notifier
#else
#define PDR_REGISTER_CALLBACK
#endif
#define FLP_DEBUG(message...) \
do { \
    if (g_flp_debug_level) { \
        printk(message); \
    } \
} while (0)

extern unsigned int g_flp_debug_level;
flp_device_t  g_flp_dev;
/*lint -e785 */
static struct genl_family flp_genl_family = {
    .id         = GENL_ID_GENERATE,
    .name       = FLP_GENL_NAME,
    .version    = TASKFLP_GENL_VERSION,
    .maxattr    = FLP_GENL_ATTR_MAX,
};
/*lint +e785 */

static int calc_GCD(unsigned int a, unsigned int b)
{
        unsigned int tmp;

        if (0 == a || 0 == b) {
                return 0;
        }

        /* Let a be the bigger one! */
        if (a < b) {
                tmp = a; a = b; b = tmp;
        }

        while ((a % b) != 0) {
                tmp = b;
                b = a % b;
                a = tmp;
        }
        return (int)b;
}

/*lint +e655*/
static int flp_genlink_checkin(flp_port_t *flp_port, unsigned int count, unsigned char cmd_type)
{
	if (!flp_port) {
		pr_err("[%s] flp_port NULL\n", __func__);
		return -EINVAL;
	}

	if(!flp_port->portid) {
		pr_err("[%s]no portid error\n", __func__);
		return -EBUSY;
	}

	if (((FLP_GENL_CMD_PDR_DATA == cmd_type) || (FLP_GENL_CMD_AR_DATA == cmd_type)
	||(FLP_GENL_CMD_ENV_DATA == cmd_type)) &&
	(!count)) {
		return -EFAULT;
	}

	return 0;

}
/*lint -e826 -e834 -e776*/
static int flp_generate_netlink_packet(flp_port_t *flp_port, char *buf,
        unsigned int count, unsigned char cmd_type)
{
	flp_data_buf_t *pdata = NULL ;
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	void *msg_header;
	char *data ;
	int result;
	static unsigned int flp_event_seqnum = 0;

	result = flp_genlink_checkin(flp_port, count, cmd_type);
	if (result) {
		pr_err("[%s]flp_genlink_checkin[%d]\n", __func__, result);
		return result;
	}

	if (FLP_GENL_CMD_PDR_DATA == cmd_type) {
		pdata = &flp_port->pdr_buf;
	}

	skb = genlmsg_new((size_t)count, GFP_ATOMIC);
	if (!skb)
		return -ENOMEM;

	/* add the genetlink message header */
	msg_header = genlmsg_put(skb, 0, flp_event_seqnum++,
	&flp_genl_family, 0, cmd_type);
	if (!msg_header) {
		nlmsg_free(skb);
		return -ENOMEM;
	}

	/* fill the data */
	data = nla_reserve_nohdr(skb, (int)count);
	if (!data) {
		nlmsg_free(skb);
		return -EINVAL;
	}
	switch (cmd_type) {
	case FLP_GENL_CMD_PDR_DATA:
	case FLP_GENL_CMD_AR_DATA:
	case FLP_GENL_CMD_ENV_DATA:
		if (!pdata) {
			nlmsg_free(skb);
			printk(KERN_ERR "%s error\n", __func__);
			return -EINVAL;
		}
		if (count  > pdata->data_count) {
			count = pdata->data_count;
		}

		/*copy data to user buffer*/
		if ((pdata->read_index + count) >  pdata->buf_size) {
			memcpy_s(data, (size_t)count, pdata->data_buf + pdata->read_index, (size_t)(pdata->buf_size - pdata->read_index));
			memcpy_s(data + pdata->buf_size - pdata->read_index,
			(size_t)count - (size_t)(pdata->buf_size - pdata->read_index),
			pdata->data_buf,
			(size_t)(count + pdata->read_index - pdata->buf_size));
		} else {
			memcpy_s(data, (size_t)count, pdata->data_buf + pdata->read_index, (size_t)count);
		}
		pdata->read_index = (pdata->read_index + count)%pdata->buf_size;
		pdata->data_count -= count;
	break ;
	default:
		if (buf && count) {
			memcpy_s(data, (size_t)count, buf, (size_t)count);
		}
	break ;
	};

	/*if aligned, just set real count*/
	nlh = (struct nlmsghdr *)((unsigned char *)msg_header - GENL_HDRLEN - NLMSG_HDRLEN);
	nlh->nlmsg_len = count + GENL_HDRLEN + NLMSG_HDRLEN;

	printk(HISI_FLP_DEBUG "%s 0x%x:%d:%d\n", __func__, flp_port->port_type, cmd_type, nlh->nlmsg_len);
	/* send unicast genetlink message */
	result = genlmsg_unicast(&init_net, skb, flp_port->portid);
	if (result) {
		printk(KERN_ERR "flp:Failed to send netlink event:%d", result);
	}

	return result;
}
/*lint -e845*/
static int  flp_pdr_stop_cmd(flp_port_t *flp_port, unsigned long arg)
{
	struct list_head    *pos;
	flp_port_t      *port;
	unsigned int delay = 0;

	printk(HISI_FLP_DEBUG "flp_stop_cmd pdr count[%d]--dalay[%d]\n", g_flp_dev.pdr_start_count, delay);
	if (!flp_port->pdr_buf.data_buf) {
		printk(KERN_ERR "Repeat stop is not permit \n");
		return -EPERM;
	}

	if (copy_from_user(&delay, (void *)arg, sizeof(unsigned int))) {
		printk(KERN_ERR "flp_ioctl copy_from_user error\n");
		return -EFAULT;
	}

	g_flp_dev.pdr_start_count-- ;

    flp_port->channel_type &= (~FLP_PDR_DATA);
    memset_s((void *)&flp_port->pdr_config, sizeof(flp_port->pdr_config), 0, sizeof(pdr_start_config_t));
    if (0 == g_flp_dev.pdr_start_count) {
        memset_s((void *)&g_flp_dev.pdr_config, sizeof(g_flp_dev.pdr_config), 0, sizeof(pdr_start_config_t));
        delay = 0;
#ifdef CONFIG_INPUTHUB_20
        send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_PDR_STOP_REQ, (char *)&delay, sizeof(int));
#else
        send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ,
                CMD_FLP_PDR_STOP_REQ, (char *)&delay, sizeof(int));
#endif
        send_cmd_from_kernel(TAG_PDR, CMD_CMN_CLOSE_REQ, 0, NULL, (size_t)0);
        g_flp_dev.pdr_cycle = 0;
        g_flp_dev.service_type &= ~FLP_PDR_DATA;
    } else if (g_flp_dev.pdr_start_count > 0) {
        list_for_each(pos, &g_flp_dev.list) {
            port = container_of(pos, flp_port_t, list);
            if ((port != flp_port) && (port->channel_type & FLP_PDR_DATA)) {
                memcpy_s((void *)&g_flp_dev.pdr_config, sizeof(g_flp_dev.pdr_config), &port->pdr_config, sizeof(pdr_start_config_t));
                break;
            }
        }
        g_flp_dev.pdr_flush_config = FLP_IOCTL_PDR_STOP(0);
#ifdef CONFIG_INPUTHUB_20
        send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_PDR_FLUSH_REQ, NULL, (size_t)0);
        send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_PDR_UPDATE_REQ,
             (char *)&g_flp_dev.pdr_config, sizeof(pdr_start_config_t));
#else
        send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ, CMD_FLP_PDR_FLUSH_REQ, NULL, (size_t)0);
        send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ, CMD_FLP_PDR_UPDATE_REQ,
             (char *)&g_flp_dev.pdr_config, sizeof(pdr_start_config_t));
#endif
    }

	kfree(flp_port->pdr_buf.data_buf);
	flp_port->pdr_buf.data_buf = NULL;
	return 0;
}
/*lint +e826 +e834 +e776 +e845*/
/*lint -e845*/

static int get_pdr_cfg(pdr_start_config_t *pdr_config, const char __user *buf, size_t len)
{
	if (len != sizeof(pdr_start_config_t)) {
		pr_err("[%s]len err [%lu]\n", __func__, len);
		return -EINVAL;
	}

	if (copy_from_user(pdr_config, buf, len)) {
		printk(KERN_ERR "flp_start_cmd copy_from_user error\n");
		return -EIO;
	}

	if (PDR_INTERVAL_MXN < pdr_config->report_interval ||PDR_INTERVAL_MIN > pdr_config->report_interval){
		pr_err("FLP[%s]interva err[%u]\n", __func__,pdr_config->report_interval);
		return -EINVAL;
	}

	if (PDR_DATA_MAX_COUNT < pdr_config->report_count || PDR_DATA_MIX_COUNT > pdr_config->report_count){
		pr_err("[%s]report_count err[%u]\n", __func__, pdr_config->report_count);
		return -EINVAL;
	}

	if (PDR_PRECISE_MXN < pdr_config->report_precise ||
		pdr_config->report_precise%PDR_PRECISE_MIN){
		pr_err("[%s]precise err[%u]\n", __func__, pdr_config->report_precise);
		return -EINVAL;
	}

	if (PDR_PRECISE_MIN > pdr_config->report_precise) {
		 pdr_config->report_precise = PDR_PRECISE_MIN;
	}

	if (pdr_config->report_interval <
		pdr_config->report_count * pdr_config->report_precise) {
		printk(KERN_ERR "flp_start_cmd error  line[%d]\n", __LINE__);
		return -EINVAL;
	}
	if (pdr_config->report_interval/pdr_config->report_precise >
		PDR_DATA_MAX_COUNT) {
		printk(KERN_ERR "flp_start_cmd error  line[%d]\n", __LINE__);
		return -EINVAL;
	}

	return 0;
}

static int flp_set_port_tag(flp_port_t *flp_port, unsigned int cmd)
{
    switch (cmd & FLP_IOCTL_TAG_MASK) {
        case FLP_IOCTL_TAG_FLP:
            flp_port->port_type = FLP_TAG_FLP ;
            break ;
        case FLP_IOCTL_TAG_GPS:
            flp_port->port_type = FLP_TAG_GPS ;
            break ;
        default:
            return -EFAULT;
    }
	return 0 ;
}

static int  flp_pdr_start_cmd(flp_port_t *flp_port, const char __user *buf, size_t len, unsigned int cmd)
{
            int ret = 0;
            flp_set_port_tag(flp_port, cmd);
            if (flp_port->pdr_buf.data_buf) {
		printk(KERN_ERR "Restart is not permit \n");
		return -EPERM;
            }
            flp_port->total_count = 0;
            flp_port->pdr_buf.buf_size = sizeof(flp_pdr_data_t) * PDR_DATA_MAX_COUNT * 2;
            flp_port->pdr_buf.read_index = 0 ;
            flp_port->pdr_buf.write_index = 0;
            flp_port->pdr_buf.data_count = 0;
            flp_port->pdr_buf.data_buf = (char *) kmalloc((size_t)flp_port->pdr_buf.buf_size, GFP_KERNEL);
            if (!flp_port->pdr_buf.data_buf) {
		printk(KERN_ERR "flp_open no mem\n");
		return -ENOMEM;
            }

            if (!g_flp_dev.pdr_start_count) {
                g_flp_dev.pdr_last_utc = 0;
            }

	ret = get_pdr_cfg(&flp_port->pdr_config, buf, len);
	if (ret)
		return ret;

	/*differ app multi start*/
	if (g_flp_dev.pdr_start_count) {
		g_flp_dev.pdr_config.report_precise = PDR_PRECISE_MIN;
		g_flp_dev.pdr_config.report_interval =
		(unsigned int)calc_GCD(g_flp_dev.pdr_config.report_interval, flp_port->pdr_config.report_interval);
		g_flp_dev.pdr_config.report_count = PDR_DATA_MIX_COUNT;
		g_flp_dev.pdr_config.report_times = 0;

		flp_port->pdr_flush_config = TRUE;
		g_flp_dev.pdr_flush_config = FLP_IOCTL_PDR_START(0);
		flp_port->rate = flp_port->pdr_config.report_precise / g_flp_dev.pdr_config.report_precise ;
		flp_port->interval = flp_port->pdr_config.report_interval / g_flp_dev.pdr_config.report_precise ;
	#ifdef CONFIG_INPUTHUB_20
		send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_PDR_FLUSH_REQ, NULL, (size_t)0);
		send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_PDR_UPDATE_REQ,
								(char *)&g_flp_dev.pdr_config, sizeof(pdr_start_config_t));
	#else
		send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ, CMD_FLP_PDR_FLUSH_REQ, NULL, (size_t)0);
		send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ, CMD_FLP_PDR_UPDATE_REQ,
		(char *)&g_flp_dev.pdr_config, sizeof(pdr_start_config_t));
	#endif
	} else {
		struct read_info rd;
		memset_s((void*)&rd, sizeof(rd), 0, sizeof(struct read_info));
		memcpy_s(&g_flp_dev.pdr_config, sizeof(g_flp_dev.pdr_config), &flp_port->pdr_config, sizeof(pdr_start_config_t));
		g_flp_dev.pdr_cycle = g_flp_dev.pdr_config.report_precise;
		g_flp_dev.pdr_config.report_times = 0;
		flp_port->rate = 1 ;
		flp_port->interval = flp_port->pdr_config.report_interval/g_flp_dev.pdr_config.report_precise ;
		ret = send_cmd_from_kernel_response(TAG_PDR, CMD_CMN_OPEN_REQ, 0, NULL, (size_t)0, &rd);
		if (ret) {
			pr_err("[%s]hub not support pdr[%d]\n", __func__, ret);
			goto PDR_START_ERR;
		}
		g_flp_dev.service_type |= FLP_PDR_DATA;
	#ifdef CONFIG_INPUTHUB_20
		send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_PDR_START_REQ,
		(char *)&g_flp_dev.pdr_config, sizeof(pdr_start_config_t));
	#else
		send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ, CMD_FLP_PDR_START_REQ,
		(char *)&g_flp_dev.pdr_config, sizeof(pdr_start_config_t));
	#endif
	}
	flp_port->nextid = 0;
	flp_port->aquiredid = 0;
	printk(KERN_ERR "flp[%u]:interval:%u,precise:%u,count:%u\n",
	flp_port->port_type, flp_port->pdr_config.report_interval,
	flp_port->pdr_config.report_precise, flp_port->pdr_config.report_count);
	g_flp_dev.pdr_start_count++ ;
	flp_port->channel_type |= FLP_PDR_DATA;
	return 0;
PDR_START_ERR:
	return ret;
}
/*lint +e845*/
/*lint +e838*/
/*lint -e845*/
static int  flp_pdr_update_cmd(flp_port_t *flp_port, const char __user *buf, size_t len)
{
	int ret;
	if (!(flp_port->channel_type & FLP_PDR_DATA)) {
		pr_err("FLP[%s] ERR: you must start first error\n", __func__);
		return -EINVAL;
	}

	ret = get_pdr_cfg(&flp_port->pdr_config, buf, len);
	if (ret)
		return ret;

    /*differ app multi start*/
    if (g_flp_dev.pdr_start_count > 1) {
        g_flp_dev.pdr_config.report_precise = PDR_PRECISE_MIN;
        g_flp_dev.pdr_config.report_interval =
            (unsigned int)calc_GCD(g_flp_dev.pdr_config.report_interval, flp_port->pdr_update_config.report_interval);
        g_flp_dev.pdr_config.report_count = PDR_DATA_MIX_COUNT;
        g_flp_dev.pdr_config.report_times = 0;
    }   else {
        memcpy_s(&g_flp_dev.pdr_config, sizeof(g_flp_dev.pdr_config), &flp_port->pdr_update_config, sizeof(pdr_start_config_t));
        g_flp_dev.pdr_config.report_times = 0;
    }
    flp_port->pdr_flush_config = TRUE ;
    g_flp_dev.pdr_flush_config = FLP_IOCTL_PDR_UPDATE(0);
#ifdef CONFIG_INPUTHUB_20
    send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_PDR_FLUSH_REQ, NULL, (size_t)0);
    send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_PDR_UPDATE_REQ, (char *)&g_flp_dev.pdr_config, sizeof(pdr_start_config_t));
#else
    send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ, CMD_FLP_PDR_FLUSH_REQ,
        NULL, (size_t)0);
    send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ, CMD_FLP_PDR_UPDATE_REQ,
        (char *)&g_flp_dev.pdr_config, sizeof(pdr_start_config_t));
#endif
    printk(KERN_ERR "flp[%u]:interval:%u,precise:%u,count:%u\n",
            flp_port->port_type, flp_port->pdr_update_config.report_interval,
            flp_port->pdr_update_config.report_precise, flp_port->pdr_update_config.report_count);
    return 0;
}
/*lint +e845*/

/*lint -e834 -e776*/
static void copy_data_to_buf(flp_data_buf_t *pdata, char *data,
        unsigned int len, unsigned int align)
{
    unsigned int deta;
    /*no enough space , just overwrite*/
    if ((pdata->data_count + len) > pdata->buf_size) {
        deta = pdata->data_count + len - pdata->buf_size;
        if (deta % align) {
            printk(KERN_ERR "copy_data_to_buf data not align\n");
            deta = (deta/align + 1)*align;
        }
        pdata->read_index = (pdata->read_index + deta)%pdata->buf_size;
    }
    /*copy data to flp pdr driver buffer*/
    if ((pdata->write_index + len) >  pdata->buf_size) {
        memcpy_s(pdata->data_buf + pdata->write_index ,
            (size_t)pdata->buf_size - (size_t)pdata->write_index,
            data, (size_t)(pdata->buf_size - pdata->write_index));
        memcpy_s(pdata->data_buf,
            (size_t)pdata->buf_size,
            data + pdata->buf_size - pdata->write_index,
            (size_t)(len + pdata->write_index - pdata->buf_size));
    } else {
        memcpy_s(pdata->data_buf + pdata->write_index,
            (size_t)pdata->buf_size - (size_t)pdata->write_index,
            data , (size_t)len);
    }
    pdata->write_index = (pdata->write_index + len)%pdata->buf_size;
    pdata->data_count = (pdata->write_index - pdata->read_index + pdata->buf_size)%pdata->buf_size;
    /*if buf is full*/
    if (!pdata->data_count) {
        pdata->data_count = pdata->buf_size;
    }
}

static int get_pdr_notify_from_mcu(const pkt_header_t *head)
{
    flp_port_t *flp_port ;
    struct list_head    *pos;
    int *data = (int *) (head + 1);
    int ret = 0;

#ifdef CONFIG_INPUTHUB_20
    if (SUB_CMD_FLP_PDR_UNRELIABLE_REQ == ((pkt_header_t *)head)->cmd) {
#else
    if (CMD_FLP_PDR_UNRELIABLE_REQ == head->cmd) {
#endif
        mutex_lock(&g_flp_dev.lock);
        list_for_each(pos, &g_flp_dev.list) {
            flp_port = container_of(pos, flp_port_t, list);
            if (!(flp_port->channel_type&FLP_PDR_DATA)) {
                continue ;
            }

            if (*data < 2) {
                ret |= flp_generate_netlink_packet(flp_port, (char *)data,
                        (unsigned int)sizeof(int), FLP_GENL_CMD_PDR_UNRELIABLE);
            } else if ((2 == *data) && (flp_port->need_report)) {
                ret |= flp_generate_netlink_packet(flp_port, (char *)data,
                        (unsigned int)sizeof(int), FLP_GENL_CMD_PDR_UNRELIABLE);
            }
        }
        mutex_unlock(&g_flp_dev.lock);
    }
    return ret;
}
/*lint -e845 -e826*/
static void __get_pdr_data_from_mcu(flp_pdr_data_t *data, unsigned int count)
{
    flp_data_buf_t *pdata;
    flp_port_t *flp_port ;
    struct list_head    *pos;
    flp_pdr_data_t *pevent;
    flp_pdr_data_t *pinsert;
    int ret;

    /*pick up data from inputhub buf*/
    list_for_each(pos, &g_flp_dev.list) {
	flp_port = container_of(pos, flp_port_t, list);
	if (!(flp_port->channel_type&FLP_PDR_DATA))
		continue;

	if (0 == flp_port->pdr_config.report_count ||0 == g_flp_dev.pdr_config.report_precise ||0 == flp_port->rate) {
		pr_err("%s count [%u] precise [%u]rate[%u]error\n", __func__,
			flp_port->pdr_config.report_count, g_flp_dev.pdr_config.report_precise,
			flp_port->rate);
		continue;
	}

        /* if start pdr ever,just discard history data*/
        if ((FLP_IOCTL_PDR_START(0) == g_flp_dev.pdr_flush_config) &&
            (flp_port->pdr_flush_config)) {
            if (count < PDR_SINGLE_SEND_COUNT) {
                flp_port->pdr_flush_config = 0;
                memcpy_s(&flp_port->pdr_compensate, sizeof(flp_port->pdr_compensate), &g_flp_dev.pdr_compensate,
                    sizeof(compensate_data_t));
            }
            continue ;
        }
        pdata = &flp_port->pdr_buf ;
        while (flp_port->nextid < count) {
            pevent = data + flp_port->nextid;
            pinsert = (flp_pdr_data_t *)(pdata->data_buf + pdata->write_index);
            copy_data_to_buf(pdata, (char *)pevent,
                (unsigned int)sizeof(flp_pdr_data_t), (unsigned int)sizeof(flp_pdr_data_t));
            /*as multi port scene, need subtract original point*/
            if (flp_port->pdr_compensate.compensate_position_x ||
                flp_port->pdr_compensate.compensate_position_y ||
                flp_port->pdr_compensate.compensate_step) {
                pinsert->step_count -= flp_port->pdr_compensate.compensate_step;
                pinsert->relative_position_x -= flp_port->pdr_compensate.compensate_position_x;
                pinsert->relative_position_y -= flp_port->pdr_compensate.compensate_position_y;
                pinsert->migration_distance -= flp_port->pdr_compensate.compensate_distance;
            }
            flp_port->total_count++ ;
            flp_port->aquiredid++ ;
            if ((flp_port->interval ==  flp_port->pdr_config.report_count) ||
                (flp_port->aquiredid%flp_port->pdr_config.report_count)) {
                flp_port->nextid += flp_port->rate;
            } else {
                flp_port->nextid += flp_port->interval - (flp_port->pdr_config.report_count - 1) * flp_port->rate;
            }
        }


	flp_port->aquiredid = flp_port->aquiredid%flp_port->pdr_config.report_count;
        flp_port->nextid -= count ;
        /*if up to report condition , send packet to hal layer*/
        if ((0 == flp_port->total_count%flp_port->pdr_config.report_count) ||
            (flp_port->pdr_buf.data_count >=
            flp_port->pdr_config.report_count*sizeof(flp_pdr_data_t))) {
	ret = flp_generate_netlink_packet(flp_port, pdata->data_buf, pdata->data_count, FLP_GENL_CMD_PDR_DATA);
	if (ret){
		pr_err("%s netlink_packet error[%d]\n", __func__, ret);
		return;
	}
        }
        /*check if need update pickup parameter or not*/
        if ((g_flp_dev.pdr_flush_config) && (count < PDR_SINGLE_SEND_COUNT)) {
            if ((FLP_IOCTL_PDR_UPDATE(0) == g_flp_dev.pdr_flush_config) && (flp_port->pdr_flush_config)) {
                memcpy_s(&flp_port->pdr_config, sizeof(flp_port->pdr_config), &flp_port->pdr_update_config,
                    sizeof(pdr_start_config_t));
                flp_port->nextid = 0;
                flp_port->aquiredid = 0;
                flp_port->total_count = 0 ;
            } else {
	flp_port->nextid *=  (flp_port->pdr_config.report_precise/g_flp_dev.pdr_config.report_precise)/flp_port->rate;
            }

	flp_port->rate = flp_port->pdr_config.report_precise/g_flp_dev.pdr_config.report_precise ;
	flp_port->interval = flp_port->pdr_config.report_interval/g_flp_dev.pdr_config.report_precise ;
            /*as send update or flush command to the port, send the received packets immediately*/
            if (((FLP_IOCTL_PDR_UPDATE(0) == g_flp_dev.pdr_flush_config) ||
                (FLP_IOCTL_PDR_FLUSH(0) == g_flp_dev.pdr_flush_config)) &&
                (flp_port->pdr_flush_config)) {
		flp_port->pdr_flush_config = 0;
		ret = flp_generate_netlink_packet(flp_port, pdata->data_buf, pdata->data_count, FLP_GENL_CMD_PDR_DATA);
		if (ret){
			pr_err("%s netlink_packet error[%d]\n", __func__, ret);
			return;
		}
            }
        }
        printk(HISI_FLP_DEBUG "flp:%s port_type:%d: len:%d,%d\n", __func__,
                                        flp_port->port_type, flp_port->pdr_config.report_count, flp_port->pdr_buf.data_count);
    }
}

static int get_pdr_data_from_mcu(const pkt_header_t *head)
{
    flp_pdr_data_t *data = (flp_pdr_data_t *) (head + 1);
    unsigned int len = head->length;
    flp_pdr_data_t *pevent;
    unsigned int count;
    unsigned int j;

    /*check data lenghth is valid*/
    if (len%(sizeof(flp_pdr_data_t))) {
        printk(KERN_ERR "pkt len[%d] error\n", head->length);
        return -EFAULT;
    }

	mutex_lock(&g_flp_dev.lock);
	/*no port be opened ,just discard data*/
	if (list_empty(&g_flp_dev.list)) {
		printk(KERN_ERR "flp pdr no port be opened\n");
		mutex_unlock(&g_flp_dev.lock);
		return -EFAULT;
	}

    count = len/(sizeof(flp_pdr_data_t));
    pevent = data;
    /*start first time, get utc of start cmd */
    if (!g_flp_dev.pdr_last_utc) {
        /*lint -e647 -esym(647,*) */
        g_flp_dev.pdr_last_utc = ktime_get_real_seconds() - g_flp_dev.pdr_config.report_count * (g_flp_dev.pdr_cycle/1000);
        /*lint +e647 +esym(647,*) */
        FLP_DEBUG("flputc--first [%ld]\n", g_flp_dev.pdr_last_utc);
    }
    /*for support multi port,transfer one times; timer continue in one fifo transfer*/
    for (j = 0; j < count; j++) {
        pevent = data + j;
        g_flp_dev.pdr_last_utc += g_flp_dev.pdr_cycle/1000;
        FLP_DEBUG("flputc[%ld]: %ld\n", g_flp_dev.pdr_last_utc, pevent->msec);
        pevent->msec = (unsigned long)g_flp_dev.pdr_last_utc;
    }
    if ((j == count) && (count < PDR_SINGLE_SEND_COUNT)) {
        g_flp_dev.pdr_last_utc = ktime_get_real_seconds();
    }
    /*record last packet*/
    if(count > 0) {
        g_flp_dev.pdr_compensate.compensate_step = pevent->step_count;
        g_flp_dev.pdr_compensate.compensate_position_x = pevent->relative_position_x;
        g_flp_dev.pdr_compensate.compensate_position_y = pevent->relative_position_y;
        g_flp_dev.pdr_compensate.compensate_distance = pevent->migration_distance;
    }

    printk(HISI_FLP_DEBUG "flp:recv pkt len[%d]\n", len);

    __get_pdr_data_from_mcu(data, count);

	/*short package indecate history date sending complete*/
	if ((g_flp_dev.pdr_flush_config) && (count < PDR_SINGLE_SEND_COUNT)) {
		g_flp_dev.pdr_flush_config = 0;
		g_flp_dev.pdr_cycle = g_flp_dev.pdr_config.report_precise;
	}

	mutex_unlock(&g_flp_dev.lock);
	return (int)len;
}
#ifdef GEOFENCE_BATCH_FEATURE
/*lint +e845 */
#ifdef CONFIG_INPUTHUB_20
static int get_common_data_from_mcu(const pkt_header_t *head)
{
    unsigned int len = head->length;
    char *data = (char  *) (head + 1);
    flp_port_t *flp_port ;
    struct list_head    *pos;

    printk(HISI_FLP_DEBUG "flp:%s cmd:%d: len:%d\n", __func__, head->cmd, len);
    mutex_lock(&g_flp_dev.lock);
    list_for_each(pos, &g_flp_dev.list) {
        flp_port = container_of(pos, flp_port_t, list);
        switch (((pkt_subcmd_req_t *)head)->subcmd)  {
            case SUB_CMD_FLP_LOCATION_UPDATE_REQ:
                if ((FLP_TAG_FLP == flp_port->port_type) && (flp_port->channel_type&FLP_BATCHING)) {
                    flp_generate_netlink_packet(flp_port, data,
                        len, FLP_GENL_CMD_GNSS_LOCATION);
                }
                break;
            case SUB_CMD_FLP_GEOF_TRANSITION_REQ:
                if ((FLP_TAG_FLP == flp_port->port_type) && (flp_port->channel_type&FLP_GEOFENCE)) {
                    flp_generate_netlink_packet(flp_port, data,
                        len, FLP_GENL_CMD_GEOFENCE_TRANSITION);
                }
                break;
            case SUB_CMD_FLP_GEOF_MONITOR_STATUS_REQ:
                if ((FLP_TAG_FLP == flp_port->port_type) && (flp_port->channel_type&FLP_GEOFENCE)) {
                    flp_generate_netlink_packet(flp_port, data,
                        len, FLP_GENL_CMD_GEOFENCE_MONITOR);
                }
                break;
            case SUB_CMD_FLP_RESET_RESP:
                if (FLP_TAG_FLP == flp_port->port_type) {
                    flp_generate_netlink_packet(flp_port, data, (unsigned int)sizeof(unsigned int), FLP_GENL_CMD_IOMCU_RESET);
                }
                break;
            default:
                printk(KERN_ERR "flp:%s cmd[0x%x] error\n", __func__, head->cmd);
                mutex_unlock(&g_flp_dev.lock);
                return -EFAULT;
        }
    }
    mutex_unlock(&g_flp_dev.lock);
    return (int)len;
}
#else
static int get_common_data_from_mcu(const pkt_header_t *head)
{
    unsigned int len = head->length;
    char *data = (char  *) (head + 1);
    flp_port_t *flp_port ;
    struct list_head    *pos;

    printk(HISI_FLP_DEBUG "flp:%s cmd:%d: len:%d\n", __func__, head->cmd, len);
    mutex_lock(&g_flp_dev.lock);
    list_for_each(pos, &g_flp_dev.list) {
        flp_port = container_of(pos, flp_port_t, list);
        switch (head->cmd)  {
            case CMD_FLP_LOCATION_UPDATE_REQ:
                if ((FLP_TAG_FLP == flp_port->port_type) && (flp_port->channel_type&FLP_BATCHING)) {
                    flp_generate_netlink_packet(flp_port, data,
                        len, FLP_GENL_CMD_GNSS_LOCATION);
                }
                break;
            case CMD_FLP_GEOF_TRANSITION_REQ:
                if ((FLP_TAG_FLP == flp_port->port_type) && (flp_port->channel_type&FLP_GEOFENCE)) {
                    flp_generate_netlink_packet(flp_port, data,
                        len, FLP_GENL_CMD_GEOFENCE_TRANSITION);
                }
                break;
            case CMD_FLP_GEOF_MONITOR_STATUS_REQ:
                if ((FLP_TAG_FLP == flp_port->port_type) && (flp_port->channel_type&FLP_GEOFENCE)) {
                    flp_generate_netlink_packet(flp_port, data,
                        len, FLP_GENL_CMD_GEOFENCE_MONITOR);
                }
                break;
            case CMD_FLP_RESET_RESP:
                if (FLP_TAG_FLP == flp_port->port_type) {
                    flp_generate_netlink_packet(flp_port, data, (unsigned int)sizeof(unsigned int), FLP_GENL_CMD_IOMCU_RESET);
                }
                break;
            default:
                printk(KERN_ERR "flp:%s cmd[0x%x] error\n", __func__, head->cmd);
                mutex_unlock(&g_flp_dev.lock);
                return -EFAULT;
        }
    }
    mutex_unlock(&g_flp_dev.lock);
    return (int)len;
}
#endif
#ifdef CONFIG_INPUTHUB_20
static int get_data_from_mcu(const pkt_header_t *head)
{
	switch(((pkt_subcmd_req_t *)head)->subcmd) {
	case SUB_CMD_FLP_LOCATION_UPDATE_REQ:
	case SUB_CMD_FLP_GEOF_TRANSITION_REQ:
	case SUB_CMD_FLP_GEOF_MONITOR_STATUS_REQ:
		return get_common_data_from_mcu(head);
	default:
		hwlog_err("uncorrect subcmd 0x%x.\n", ((pkt_subcmd_req_t *)head)->subcmd);
	}
	return -EFAULT;
}
#endif
#endif
static void  flp_service_recovery(void)
{
	flp_port_t *flp_port;
	unsigned int flag = 0;
#ifdef GEOFENCE_BATCH_FEATURE
	unsigned int response = FLP_IOMCU_RESET;
#endif
	struct list_head    *pos;
	list_for_each(pos, &g_flp_dev.list) {
		flp_port = container_of(pos, flp_port_t, list);
		if ((flp_port->channel_type&FLP_PDR_DATA) &&
		!(flag & FLP_PDR_DATA)) {
			send_cmd_from_kernel_nolock(TAG_PDR, CMD_CMN_OPEN_REQ, 0, NULL, (size_t)0);
		#ifdef CONFIG_INPUTHUB_20
			send_cmd_from_kernel_nolock(TAG_PDR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_PDR_START_REQ,
			(char *)&g_flp_dev.pdr_config, sizeof(pdr_start_config_t));
		#else
			send_cmd_from_kernel_nolock(TAG_PDR, CMD_CMN_CONFIG_REQ, CMD_FLP_PDR_START_REQ,
			(char *)&g_flp_dev.pdr_config, sizeof(pdr_start_config_t));
		#endif
			flag |= FLP_PDR_DATA;
		}
#ifdef GEOFENCE_BATCH_FEATURE
		if ((FLP_TAG_FLP == flp_port->port_type) && ((flp_port->channel_type & FLP_BATCHING) ||
		(flp_port->channel_type & FLP_GEOFENCE))) {
			flp_generate_netlink_packet(flp_port, (char *)&response, (unsigned int)sizeof(unsigned int), FLP_GENL_CMD_IOMCU_RESET);
		}
#endif
	}
}
/*lint -e715*/
static int flp_notifier(struct notifier_block *nb,
            unsigned long action, void *data)
{
    switch (action) {
        case IOM3_RECOVERY_3RD_DOING:
            flp_service_recovery();
            break;
        default:
            printk(KERN_ERR "register_iom3_recovery_notifier err\n");
            break;
    }
    return 0;
}
/*lint +e715*/
static struct notifier_block sensor_reboot_notify = {
    .notifier_call = flp_notifier,
    .priority = -1,
};

static void flp_timerout_work(struct work_struct *wk)
{
    flp_port_t *flp_port  = container_of(wk, flp_port_t, work);
    flp_generate_netlink_packet(flp_port, NULL, 0, (unsigned char)flp_port->work_para);
}
static void flp_sleep_timeout(unsigned long data)
{
    flp_port_t *flp_port = (flp_port_t *)data;
    printk(KERN_INFO "flp_sleep_timeout \n");
    if (flp_port) {
        flp_port->work_para = FLP_GENL_CMD_NOTIFY_TIMEROUT;
        queue_work(system_power_efficient_wq, &flp_port->work);
        if (flp_port->need_hold_wlock) {
            wake_lock_timeout(&flp_port->wlock, (long)(2 * HZ));
        }
    }
    return ;
}
void flp_port_resume(void)
{
    struct list_head    *pos;
    flp_port_t      *flp_port;

    mutex_lock(&g_flp_dev.lock);
    list_for_each(pos, &g_flp_dev.list) {
        flp_port = container_of(pos, flp_port_t, list);
        if (flp_port->need_awake) {
            flp_port->work_para = FLP_GENL_CMD_AWAKE_RET;
            queue_work(system_power_efficient_wq, &flp_port->work);
        }
    }
    mutex_unlock(&g_flp_dev.lock);
}

static bool flp_check_cmd(unsigned int cmd, int type)
{
	switch (type) {
	case FLP_PDR_DATA:
		if (((cmd & FLP_IOCTL_TAG_MASK) == FLP_IOCTL_TAG_FLP) ||
		((cmd & FLP_IOCTL_TAG_MASK) == FLP_IOCTL_TAG_GPS)) {
			return TRUE;
		}
	break ;
#ifdef GEOFENCE_BATCH_FEATURE
	case FLP_GEOFENCE:
	case FLP_BATCHING:
		if ((cmd & FLP_IOCTL_TAG_MASK) == FLP_IOCTL_TAG_FLP) {
			return TRUE;
		}
	break ;
#endif
	default :break ;
	}
	return 0;
}
/*lint -e845 */
static int  flp_pdr_flush(flp_port_t *flp_port)
{
	if (!(flp_port->channel_type & FLP_PDR_DATA)) {
		pr_err("FLP[%s] ERR: you must start first error\n", __func__);
		return -EINVAL;
	}
	g_flp_dev.pdr_flush_config = FLP_IOCTL_PDR_FLUSH(0);
	flp_port->pdr_flush_config = TRUE;
#ifdef CONFIG_INPUTHUB_20
	return send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_PDR_FLUSH_REQ, NULL, 0);
#else
	return send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ,
	CMD_FLP_PDR_FLUSH_REQ, NULL, 0);
#endif
}
/*lint +e845 */
static int flp_pdr_step(flp_port_t *flp_port, unsigned long arg)
 {
	step_report_t step_report;
	if (!(flp_port->channel_type & FLP_PDR_DATA)) {
		pr_err("FLP[%s] ERR: you must start first error\n", __func__);
		return -EINVAL;
	}
	memset_s((void*)&step_report, sizeof(step_report), 0, sizeof(step_report_t));
	flp_port->need_report = TRUE;
            if (copy_from_user(&step_report, (char __user *)arg, sizeof(step_report_t))) {
		pr_err("[%s]copy_from_user error\n", __func__);
		return -EIO;
            }
#ifdef CONFIG_INPUTHUB_20
	return send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ,
                SUB_CMD_FLP_PDR_STEPCFG_REQ, (char  *)&step_report, sizeof(step_report_t));
#else
            return send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ,
                CMD_FLP_PDR_STEPCFG_REQ, (char  *)&step_report, sizeof(step_report_t));
#endif
}

/*lint -e845 -e747 -e712*/
static int flp_pdr_ioctl(flp_port_t *flp_port, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	mutex_lock(&g_flp_dev.lock);
	switch (cmd & FLP_IOCTL_CMD_MASK) {
	case FLP_IOCTL_PDR_START(0):
		ret = flp_pdr_start_cmd(flp_port, (char __user *)arg, sizeof(pdr_start_config_t), cmd);
	break;
	case FLP_IOCTL_PDR_STOP(0):
		ret = flp_pdr_stop_cmd(flp_port, arg);
	break;
	case FLP_IOCTL_PDR_UPDATE(0):
		ret = flp_pdr_update_cmd(flp_port, (char __user *)arg, sizeof(pdr_start_config_t));
	break;
	case FLP_IOCTL_PDR_FLUSH(0):
		ret = flp_pdr_flush(flp_port);
	break;
	case FLP_IOCTL_PDR_STEP_CFG(0):
		ret = flp_pdr_step(flp_port, arg);
	break;
	default:
		printk(KERN_ERR "flp_pdr_ioctl input cmd[0x%x] error\n", cmd);
		mutex_unlock(&g_flp_dev.lock);
		return -EFAULT;
	}
	mutex_unlock(&g_flp_dev.lock);
	return ret;
}
/*lint +e845 +e747 +e712*/
/*lint -e715*/
static int flp_common_ioctl_open_service(flp_port_t *flp_port)
{
#ifdef GEOFENCE_BATCH_FEATURE
	unsigned int response = FLP_IOMCU_RESET;
#endif
	struct read_info rd;
	int ret;
	memset_s((void*)&rd, sizeof(rd), 0, sizeof(struct read_info));
	if (g_flp_dev.service_type & FLP_PDR_DATA && g_flp_dev.pdr_start_count) {
		ret = send_cmd_from_kernel_response(TAG_PDR, CMD_CMN_OPEN_REQ, 0, NULL, (size_t)0, &rd);
		if (0 == ret)
		#ifdef CONFIG_INPUTHUB_20
			send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_PDR_START_REQ,
			(char *)&g_flp_dev.pdr_config, (size_t)sizeof(pdr_start_config_t));
		#else
			send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ, CMD_FLP_PDR_START_REQ,
			(char *)&g_flp_dev.pdr_config, (size_t)sizeof(pdr_start_config_t));
		#endif

	}
#ifdef GEOFENCE_BATCH_FEATURE
	if (g_flp_dev.service_type & (FLP_BATCHING|FLP_GEOFENCE))
		flp_generate_netlink_packet(flp_port, (char *)&response, (unsigned int)sizeof(unsigned int), FLP_GENL_CMD_IOMCU_RESET);
#endif
	return 0;
}

static int flp_common_ioctl_close_service(flp_port_t *flp_port)
{
	unsigned int data = 0;
    /*if start pdr function ever*/
    if ((g_flp_dev.service_type & FLP_PDR_DATA) &&
            (g_flp_dev.pdr_start_count)) {
#ifdef CONFIG_INPUTHUB_20
        send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_PDR_STOP_REQ, (char *)&data, sizeof(int));
#else
        send_cmd_from_kernel(TAG_PDR, CMD_CMN_CONFIG_REQ,
            CMD_FLP_PDR_STOP_REQ, (char *)&data, sizeof(int));
#endif
        send_cmd_from_kernel(TAG_PDR, CMD_CMN_CLOSE_REQ, 0, NULL, (size_t)0);
    }
#ifdef GEOFENCE_BATCH_FEATURE
    if ((g_flp_dev.service_type & FLP_BATCHING) || (g_flp_dev.service_type & FLP_GEOFENCE)) {
        send_cmd_from_kernel(TAG_FLP, CMD_CMN_CLOSE_REQ, 0, NULL, (size_t)0);
    }
#endif
    return 0;
}

static int flp_common_ioctl(flp_port_t *flp_port, unsigned int cmd, unsigned long arg)
{
    unsigned int data = 0;
    int ret = 0;

    if (FLP_IOCTL_COMMON_RELEASE_WAKELOCK != cmd) {
        if (copy_from_user(&data, (void *)arg, sizeof(unsigned int))) {
            printk(KERN_ERR "flp_ioctl copy_from_user error[%d]\n", ret);
            return -EFAULT;
        }
    }

    switch (cmd) {
        case FLP_IOCTL_COMMON_SLEEP:
            printk(HISI_FLP_DEBUG "flp:start timer %d\n",  data);
            /*if timer is running just delete it ,then restart it*/
            hisi_softtimer_delete(&flp_port->sleep_timer);
            ret = hisi_softtimer_modify(&flp_port->sleep_timer, data);
            if (!ret)
                hisi_softtimer_add(&flp_port->sleep_timer);
            break ;
        case FLP_IOCTL_COMMON_AWAKE_RET:
            flp_port->need_awake = data;
            break ;
        case FLP_IOCTL_COMMON_SETPID:
            flp_port->portid = data;
            break ;
        case FLP_IOCTL_COMMON_CLOSE_SERVICE:
	mutex_lock(&g_flp_dev.lock);
	g_flp_dev.denial_sevice = data;
	printk(HISI_FLP_DEBUG "%s 0x%x\n", __func__, g_flp_dev.denial_sevice);
	if(g_flp_dev.denial_sevice)
		flp_common_ioctl_close_service(flp_port);
	else
		flp_common_ioctl_open_service(flp_port);
	mutex_unlock(&g_flp_dev.lock);
	break;
        case FLP_IOCTL_COMMON_HOLD_WAKELOCK:
            flp_port->need_hold_wlock = data;
            printk(HISI_FLP_DEBUG "%s 0x%x\n", __func__, flp_port->need_hold_wlock);
            break ;
        case FLP_IOCTL_COMMON_RELEASE_WAKELOCK:
            if (flp_port->need_hold_wlock) {
                wake_unlock(&flp_port->wlock);/*lint !e455*/
            }
            break;
        default:
            printk(KERN_ERR "flp_common_ioctl input cmd[0x%x] error\n", cmd);
            return -EFAULT;
    }
    return 0;
}
/*lint +e715*/
#ifdef GEOFENCE_BATCH_FEATURE
/*lint -e438*/
static int flp_ioctl_add_geofence(flp_port_t *flp_port, unsigned int cmd, unsigned long arg)
{
    geofencing_hal_config_t hal_config;
    char *cmd_data;
    geofencing_useful_data_t *pdata;
    unsigned int count;

    flp_set_port_tag(flp_port, cmd);
    send_cmd_from_kernel(TAG_FLP, CMD_CMN_OPEN_REQ, 0, NULL, (size_t)0);
    if (copy_from_user(&hal_config, (void *)arg, sizeof(geofencing_hal_config_t))) {
        printk(KERN_ERR "%s copy_from_user error\n", __func__);
        return -EFAULT;
    }
    if (hal_config.length> FLP_GEOFENCE_MAX_NUM || 0 == hal_config.length) {
        printk(KERN_ERR "flp geofence number overflow %d\n", hal_config.length);
        return -EFAULT;
    }
    cmd_data = (char *)kmalloc((size_t)hal_config.length, GFP_KERNEL);
    if (!cmd_data) {
        printk(KERN_ERR "%s kmalloc fail\n", __func__);
        return -ENOMEM;
    }
    if (copy_from_user(cmd_data, (const void __user *)hal_config.buf, (unsigned long)hal_config.length)) {
        printk(KERN_ERR "%s copy_from_user error\n", __func__);
        kfree(cmd_data);
        return -EFAULT;
    }
    /*packet size big than 128, so need split it*/
    pdata = (geofencing_useful_data_t *)cmd_data;
    count = hal_config.length/sizeof(geofencing_useful_data_t);
    do {
        if (count < 3) {
	#ifdef CONFIG_INPUTHUB_20
            send_cmd_from_kernel(TAG_FLP, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_ADD_GEOF_REQ,
                    (char *)pdata, count * sizeof(geofencing_useful_data_t));
	#else
            send_cmd_from_kernel(TAG_FLP, CMD_CMN_CONFIG_REQ, CMD_FLP_ADD_GEOF_REQ,
                    (char *)pdata, count * sizeof(geofencing_useful_data_t));
	#endif
            break;
        } else {
       #ifdef CONFIG_INPUTHUB_20
            send_cmd_from_kernel(TAG_FLP, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_ADD_GEOF_REQ,
                    (char *)pdata, 3 * sizeof(geofencing_useful_data_t));
       #else
            send_cmd_from_kernel(TAG_FLP, CMD_CMN_CONFIG_REQ, CMD_FLP_ADD_GEOF_REQ,
                    (char *)pdata, 3 * sizeof(geofencing_useful_data_t));
       #endif
            pdata += 3;
            count -= 3;
        }
    }while(count > 0);
    flp_port->channel_type |= FLP_GEOFENCE;
    g_flp_dev.service_type |= FLP_GEOFENCE;
    kfree(cmd_data);
    return 0;
}

static int flp_geofence_ioctl(flp_port_t *flp_port, unsigned int cmd, unsigned long arg)
{
    geofencing_hal_config_t hal_config;
    geofencing_option_info_t modify_config;
    char *cmd_data;

    switch (cmd) {
        case FLP_IOCTL_GEOFENCE_ADD :
            flp_ioctl_add_geofence(flp_port, cmd, arg);
            break;
        case FLP_IOCTL_GEOFENCE_REMOVE :
            if (!(flp_port->channel_type & FLP_GEOFENCE)) {
                printk(KERN_ERR "%s not start \n", __func__);
                return -EPERM;
            }
            if (copy_from_user(&hal_config, (const void __user *)arg, sizeof(geofencing_hal_config_t))) {
                printk(KERN_ERR "%s copy_from_user error\n", __func__);
                return -EFAULT;
            }
            if (hal_config.length> FLP_GEOFENCE_MAX_NUM || 0 == hal_config.length) {
                printk(KERN_ERR "flp geofence number overflow %d\n", hal_config.length);
                return -EFAULT;
            }
            cmd_data = (char *)kmalloc((size_t)hal_config.length, GFP_KERNEL);
            if (!cmd_data) {
                printk(KERN_ERR "%s kmalloc fail\n", __func__);
                return -ENOMEM;
            }
            if (copy_from_user(cmd_data, (const void __user *)hal_config.buf, (unsigned long)hal_config.length)) {
                printk(KERN_ERR "%s copy_from_user error\n", __func__);
                kfree(cmd_data);
                return -EFAULT;
            }
	#ifdef CONFIG_INPUTHUB_20
            send_cmd_from_kernel(TAG_FLP, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_REMOVE_GEOF_REQ,
                            (char *)cmd_data, (size_t)hal_config.length);
	#else
            send_cmd_from_kernel(TAG_FLP, CMD_CMN_CONFIG_REQ, CMD_FLP_REMOVE_GEOF_REQ,
                            (char *)cmd_data, (size_t)hal_config.length);
	#endif
            kfree(cmd_data);
            break;
        case FLP_IOCTL_GEOFENCE_MODIFY :
            if (!(flp_port->channel_type & FLP_GEOFENCE)) {
                printk(KERN_ERR "%s not start \n", __func__);
                return -EPERM;
            }
            if (copy_from_user(&modify_config, (const void __user *)arg, sizeof(geofencing_option_info_t))) {
                printk(KERN_ERR "%s copy_from_user error\n", __func__);
                return -EFAULT;
            }
	#ifdef CONFIG_INPUTHUB_20
            send_cmd_from_kernel(TAG_FLP, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_MODIFY_GEOF_REQ,
                (char *)&modify_config, sizeof(geofencing_option_info_t));
	#else
            send_cmd_from_kernel(TAG_FLP, CMD_CMN_CONFIG_REQ, CMD_FLP_MODIFY_GEOF_REQ,
                (char *)&modify_config, sizeof(geofencing_option_info_t));
	#endif
            break;
        default :
            printk(KERN_ERR "%s input cmd[0x%x] error\n", __func__, cmd);
            return -EFAULT;
    }
    return 0;
}
/*lint +e438*/
/*max complexiy must less than 15*/
static int __flp_location_ioctl(flp_port_t *flp_port, unsigned int cmd, unsigned long arg)
{
    int data;
    iomcu_location inject_data;
    if (!(flp_port->channel_type & FLP_BATCHING)) {
        printk(KERN_ERR "%s not start \n", __func__);
        return -EPERM;
    }
    switch (cmd) {
        case FLP_IOCTL_BATCHING_STOP :
            if (copy_from_user(&data, (void *)arg, sizeof(int))) {
                printk(KERN_ERR "%s copy_from_user error\n", __func__);
                return -EFAULT;
            }
	#ifdef CONFIG_INPUTHUB_20
            send_cmd_from_kernel(TAG_FLP, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_STOP_BATCHING_REQ, (char *)&data, sizeof(int));
	#else
            send_cmd_from_kernel(TAG_FLP, CMD_CMN_CONFIG_REQ, CMD_FLP_STOP_BATCHING_REQ, (char *)&data, sizeof(int));
	#endif
            flp_port->channel_type &= ~FLP_BATCHING;
            g_flp_dev.service_type &= ~FLP_BATCHING;
            break ;
        case FLP_IOCTL_BATCHING_UPDATE :
            if (copy_from_user(&flp_port->gps_batching_config, (void *)arg, sizeof(batching_config_t))) {
                printk(KERN_ERR "%s copy_from_user error\n", __func__);
                return -EFAULT;
            }
	#ifdef CONFIG_INPUTHUB_20
            send_cmd_from_kernel(TAG_FLP, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_UPDATE_BATCHING_REQ,
                (char *)&flp_port->gps_batching_config, sizeof(batching_config_t));
	#else
            send_cmd_from_kernel(TAG_FLP, CMD_CMN_CONFIG_REQ, CMD_FLP_UPDATE_BATCHING_REQ,
                (char *)&flp_port->gps_batching_config, sizeof(batching_config_t));
	#endif
            break;
        case FLP_IOCTL_BATCHING_CLEANUP :
            send_cmd_from_kernel(TAG_FLP, CMD_CMN_CLOSE_REQ, 0, NULL, (size_t)0);
            flp_port->channel_type &= ~FLP_BATCHING;
            g_flp_dev.service_type &= ~(FLP_BATCHING | FLP_GEOFENCE);
            break;
        case FLP_IOCTL_BATCHING_LASTLOCATION :
            if (copy_from_user(&data, (void *)arg, sizeof(int))) {
                printk(KERN_ERR "%s copy_from_user error\n", __func__);
                return -EFAULT;
            }
	#ifdef CONFIG_INPUTHUB_20
            send_cmd_from_kernel(TAG_FLP, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_GET_BATCHED_LOCATION_REQ, (char *)&data, sizeof(int));
	#else
            send_cmd_from_kernel(TAG_FLP, CMD_CMN_CONFIG_REQ, CMD_FLP_GET_BATCHED_LOCATION_REQ, (char *)&data, sizeof(int));
	#endif
            break ;
        case FLP_IOCTL_BATCHING_FLUSH :
	#ifdef CONFIG_INPUTHUB_20
            send_cmd_from_kernel(TAG_FLP, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_FLUSH_LOCATION_REQ, NULL, (size_t)0);
	#else
            send_cmd_from_kernel(TAG_FLP, CMD_CMN_CONFIG_REQ, CMD_FLP_FLUSH_LOCATION_REQ, NULL, (size_t)0);
	#endif
            break ;
        case FLP_IOCTL_BATCHING_INJECT :            /*inject data needn't recovery*/
            if (copy_from_user(&inject_data, (void *)arg, sizeof(iomcu_location))) {
                printk(KERN_ERR "%s copy_from_user error\n", __func__);
                return -EFAULT;
            }
	#ifdef CONFIG_INPUTHUB_20
            send_cmd_from_kernel(TAG_FLP, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_INJECT_LOCATION_REQ, (char *)&inject_data, sizeof(iomcu_location));
	#else
            send_cmd_from_kernel(TAG_FLP, CMD_CMN_CONFIG_REQ, CMD_FLP_INJECT_LOCATION_REQ, (char *)&inject_data, sizeof(iomcu_location));
	#endif
            break;
        case FLP_IOCTL_COMMON_HW_RESET :
            if (copy_from_user(&data, (void *)arg, sizeof(int))) {
                printk(KERN_ERR "%s copy_from_user error\n", __func__);
                return -EFAULT;
            }
	#ifdef CONFIG_INPUTHUB_20
            send_cmd_from_kernel(TAG_FLP, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_RESET_REQ, (char *)&data, sizeof(int));
	#else
            send_cmd_from_kernel(TAG_FLP, CMD_CMN_CONFIG_REQ, CMD_FLP_RESET_REQ, (char *)&data, sizeof(int));
	#endif
            break ;
	default:break;
    }
    return 0;
}

static int flp_location_ioctl(flp_port_t *flp_port, unsigned int cmd, unsigned long arg)
{
    struct read_info rd;

    switch (cmd) {
        case FLP_IOCTL_BATCHING_START :
            flp_set_port_tag(flp_port, cmd);
            if (!(flp_port->channel_type & FLP_BATCHING)) {
                send_cmd_from_kernel(TAG_FLP, CMD_CMN_OPEN_REQ, 0, NULL, (size_t)0);
            }
            if (copy_from_user(&flp_port->gps_batching_config, (void *)arg, sizeof(batching_config_t))) {
                printk(KERN_ERR "%s copy_from_user error\n", __func__);
                return -EFAULT;
            }
	#ifdef CONFIG_INPUTHUB_20
            send_cmd_from_kernel(TAG_FLP, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_START_BATCHING_REQ,
                (char *)&flp_port->gps_batching_config, sizeof(batching_config_t));
	#else
            send_cmd_from_kernel(TAG_FLP, CMD_CMN_CONFIG_REQ, CMD_FLP_START_BATCHING_REQ,
                (char *)&flp_port->gps_batching_config, sizeof(batching_config_t));
	#endif
            flp_port->channel_type |= FLP_BATCHING;
            g_flp_dev.service_type |= FLP_BATCHING;
            break ;
        case FLP_IOCTL_BATCHING_STOP :
        case FLP_IOCTL_BATCHING_UPDATE :
        case FLP_IOCTL_BATCHING_CLEANUP :
        case FLP_IOCTL_BATCHING_LASTLOCATION :
        case FLP_IOCTL_BATCHING_FLUSH :
        case FLP_IOCTL_BATCHING_INJECT :
        case FLP_IOCTL_COMMON_HW_RESET :
            __flp_location_ioctl(flp_port, cmd, arg);
            break ;
        case FLP_IOCTL_BATCHING_GET_SIZE:
	#ifdef CONFIG_INPUTHUB_20
            if (!send_cmd_from_kernel_response(TAG_FLP, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_GET_BATCH_SIZE_REQ, NULL, (size_t)0, &rd)) {
	#else
            if (!send_cmd_from_kernel_response(TAG_FLP, CMD_CMN_CONFIG_REQ, CMD_FLP_GET_BATCH_SIZE_REQ, NULL, (size_t)0, &rd)) {
	#endif
                if (copy_to_user((void *)arg, rd.data, sizeof(unsigned int))) {
                    printk(KERN_ERR "%s copy_to_user error\n", __func__);
                    return -EFAULT;
                }
            }
            break;
        default :
            printk(KERN_ERR "%s input cmd[0x%x] error\n", __func__, cmd);
            return -EFAULT;
    }
    return 0;
}
#endif
/*lint -e732*/
static long flp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	flp_port_t *flp_port  = (flp_port_t *)file->private_data;
	if (!flp_port) {
		printk(KERN_ERR "flp_ioctl parameter error\n");
		return -EINVAL;
	}
	printk(HISI_FLP_DEBUG "[%s]cmd[0x%x]\n\n", __func__, cmd&0x0FFFF);
	mutex_lock(&g_flp_dev.lock);
	if ((g_flp_dev.denial_sevice) && (cmd != FLP_IOCTL_COMMON_CLOSE_SERVICE)) {
		mutex_unlock(&g_flp_dev.lock);
		return 0;
	}
	mutex_unlock(&g_flp_dev.lock);

	switch (cmd & FLP_IOCTL_TYPE_MASK) {
	case FLP_IOCTL_TYPE_PDR:
		if (!flp_check_cmd(cmd, FLP_PDR_DATA))
			return -EPERM;
		return (long)flp_pdr_ioctl(flp_port, cmd, arg);
#ifdef GEOFENCE_BATCH_FEATURE
	case FLP_IOCTL_TYPE_GEOFENCE:
		if (!flp_check_cmd((int)cmd, FLP_GEOFENCE))
			return -EPERM;
		return (long)flp_geofence_ioctl(flp_port, cmd, arg);
	case FLP_IOCTL_TYPE_BATCHING:
		if (!flp_check_cmd((int)cmd, FLP_BATCHING))
			return -EPERM;
		return (long)flp_location_ioctl(flp_port, cmd, arg);
#endif
	case FLP_IOCTL_TYPE_COMMON:
		return (long)flp_common_ioctl(flp_port, cmd, arg);
	default:
		printk(KERN_ERR "flp_ioctl input cmd[0x%x] error\n", cmd);
		return -EFAULT;
	}
}
/*lint +e732*/
/*lint -e438*/
static int flp_open(struct inode *inode, struct file *filp)/*lint -e715*/
{
	int ret = 0;
	flp_port_t *flp_port;
	struct list_head *pos;
	int count = 0;

	mutex_lock(&g_flp_dev.lock);
	list_for_each(pos, &g_flp_dev.list) {
		count++;
	}

	if(count > 100) {
		pr_err("flp_open clinet limit\n");
		ret = -EACCES;
		goto FLP_OPEN_ERR;
	}

	flp_port  = (flp_port_t *) kmalloc(sizeof(flp_port_t), GFP_KERNEL|__GFP_ZERO);
	if (!flp_port) {
		printk(KERN_ERR "flp_open no mem\n");
		ret = -ENOMEM;
		goto FLP_OPEN_ERR;
	}
	INIT_LIST_HEAD(&flp_port->list);
	hisi_softtimer_create(&flp_port->sleep_timer,
	            flp_sleep_timeout, (unsigned long)flp_port, 0);
	INIT_WORK(&flp_port->work, flp_timerout_work);

	list_add_tail(&flp_port->list, &g_flp_dev.list);
	mutex_unlock(&g_flp_dev.lock);
	wake_lock_init(&flp_port->wlock, WAKE_LOCK_SUSPEND, "hisi_flp");
	filp->private_data = flp_port;
	printk(KERN_ERR "%s %d: enter\n", __func__, __LINE__);
	return 0;
FLP_OPEN_ERR:
	mutex_unlock(&g_flp_dev.lock);
	return ret;
}

static void __flp_release(flp_port_t *flp_port)
{
	hisi_softtimer_delete(&flp_port->sleep_timer);
	cancel_work_sync(&flp_port->work);

	if (flp_port->pdr_buf.data_buf) {
		kfree(flp_port->pdr_buf.data_buf);
		flp_port->pdr_buf.data_buf = NULL;
	}
	wake_lock_destroy(&flp_port->wlock);
	kfree(flp_port);
	flp_port = NULL;
}

static int flp_release(struct inode *inode, struct file *file)/*lint -e715*/
{
	flp_port_t *flp_port  = (flp_port_t *)file->private_data;
	struct list_head    *pos;
	flp_port_t      *port;
	printk(HISI_FLP_DEBUG "[%s]\n", __func__);
	if (!flp_port) {
		printk(KERN_ERR "flp_close parameter error\n");
		return -EINVAL;
	}

	mutex_lock(&g_flp_dev.lock);
	list_del(&flp_port->list);

/*if andriod vm restart, apk doesnot send stop cmd,just adjust it*/
	g_flp_dev.pdr_start_count = 0;
	list_for_each(pos, &g_flp_dev.list) {
		port = container_of(pos, flp_port_t, list);
		if (port->channel_type & FLP_PDR_DATA) {
			g_flp_dev.pdr_start_count++ ;
		}
	}

	/*if start pdr function ever*/
	if ((g_flp_dev.service_type & FLP_PDR_DATA) &&
	(!g_flp_dev.pdr_start_count)) {
		send_cmd_from_kernel(TAG_PDR, CMD_CMN_CLOSE_REQ, 0, NULL, (size_t)0);
		g_flp_dev.service_type &= ~FLP_PDR_DATA;
	}
	/*if start batching or Geofence function ever*/
	if ((g_flp_dev.service_type & FLP_BATCHING) || (g_flp_dev.service_type & FLP_GEOFENCE)) {
		send_cmd_from_kernel(TAG_FLP, CMD_CMN_CLOSE_REQ, 0, NULL, (size_t)0);
		g_flp_dev.service_type &= ~(FLP_BATCHING | FLP_GEOFENCE);
	}
	__flp_release(flp_port);
	file->private_data = NULL ;
	printk(KERN_ERR "%s pdr_count[%d]:service_type [%d] \n", __func__, g_flp_dev.pdr_start_count,
		g_flp_dev.service_type);
	mutex_unlock(&g_flp_dev.lock);
	return 0;
}
/*lint +e438*/
/*lint +e826*/
/*lint -e785 -e64*/
static const struct file_operations hisi_flp_fops = {
	.owner =          THIS_MODULE,
	.llseek =         no_llseek,
	.unlocked_ioctl = flp_ioctl,
	.open       =     flp_open,
	.release    =     flp_release,
};

/*******************************************************************************************
Description:   miscdevice to motion
*******************************************************************************************/
static struct miscdevice hisi_flp_miscdev =
{
    .minor =    MISC_DYNAMIC_MINOR,
    .name =     "flp",
    .fops =     &hisi_flp_fops,
};
/*lint +e785 +e64*/

/*******************************************************************************************
Function:       hisi_flp_register
Description:
Data Accessed:  no
Data Updated:   no
Input:          void
Output:        void
Return:        result of function, 0 successed, else false
*******************************************************************************************/
int  hisi_flp_register(void)
{
	int ret;
	memset_s((void*)&g_flp_dev, sizeof(g_flp_dev), 0 ,sizeof(g_flp_dev));
	ret = genl_register_family(&flp_genl_family);
	if (ret) {
		return ret ;
	}
	if (!getSensorMcuMode()) {
		printk(KERN_ERR "cannot register hisi flp %d\n", __LINE__);
		genl_unregister_family(&flp_genl_family);
		return -ENODEV;
	}
	INIT_LIST_HEAD(&g_flp_dev.list);
#ifdef CONFIG_INPUTHUB_20
	PDR_REGISTER_CALLBACK(TAG_PDR, CMD_DATA_REQ, get_pdr_data_from_mcu);
	PDR_REGISTER_CALLBACK(TAG_PDR, SUB_CMD_FLP_PDR_UNRELIABLE_REQ, get_pdr_notify_from_mcu);
#ifdef GEOFENCE_BATCH_FEATURE
	PDR_REGISTER_CALLBACK(TAG_FLP, CMD_CMN_CONFIG_REQ, get_data_from_mcu);
#endif
#else
	PDR_REGISTER_CALLBACK(TAG_PDR, CMD_FLP_PDR_DATA_REQ, get_pdr_data_from_mcu);
	PDR_REGISTER_CALLBACK(TAG_PDR, CMD_FLP_PDR_UNRELIABLE_REQ, get_pdr_notify_from_mcu);
#ifdef GEOFENCE_BATCH_FEATURE
	PDR_REGISTER_CALLBACK(TAG_FLP, CMD_FLP_LOCATION_UPDATE_REQ, get_common_data_from_mcu);
	PDR_REGISTER_CALLBACK(TAG_FLP, CMD_FLP_GEOF_TRANSITION_REQ, get_common_data_from_mcu);
	PDR_REGISTER_CALLBACK(TAG_FLP, CMD_FLP_GEOF_MONITOR_STATUS_REQ, get_common_data_from_mcu);
#endif
#endif
	register_iom3_recovery_notifier(&sensor_reboot_notify);
	mutex_init(&g_flp_dev.lock);

	ret = misc_register(&hisi_flp_miscdev);
	if (ret != 0)    {
		printk(KERN_ERR "cannot register hisi flp err=%d\n", ret);
		goto err;
	}
	printk(KERN_ERR "hisi_flp_register success\n");
	return 0;
err:
#ifdef CONFIG_INPUTHUB_20
	unregister_mcu_event_notifier(TAG_PDR, CMD_DATA_REQ, get_pdr_data_from_mcu);
	unregister_mcu_event_notifier(TAG_PDR, SUB_CMD_FLP_PDR_UNRELIABLE_REQ, get_pdr_notify_from_mcu);
#ifdef GEOFENCE_BATCH_FEATURE
	unregister_mcu_event_notifier(TAG_FLP, CMD_CMN_CONFIG_REQ, get_data_from_mcu);
#endif
#else
	unregister_mcu_event_notifier(TAG_PDR, CMD_FLP_PDR_DATA_REQ, get_pdr_data_from_mcu);
	unregister_mcu_event_notifier(TAG_PDR, CMD_FLP_PDR_UNRELIABLE_REQ, get_pdr_notify_from_mcu);
#ifdef GEOFENCE_BATCH_FEATURE
	unregister_mcu_event_notifier(TAG_FLP, CMD_FLP_LOCATION_UPDATE_REQ, get_common_data_from_mcu);
	unregister_mcu_event_notifier(TAG_FLP, CMD_FLP_GEOF_TRANSITION_REQ, get_common_data_from_mcu);
	unregister_mcu_event_notifier(TAG_FLP, CMD_FLP_GEOF_MONITOR_STATUS_REQ, get_common_data_from_mcu);
#endif
#endif
	genl_unregister_family(&flp_genl_family);
	return ret;
}

EXPORT_SYMBOL_GPL(hisi_flp_register);


/*******************************************************************************************
Function:       hisi_flp_unregister
Description:
Data Accessed:  no
Data Updated:   no
Input:          void
Output:        void
Return:        void
*******************************************************************************************/
 int hisi_flp_unregister(void)
{
#ifdef CONFIG_INPUTHUB_20
        unregister_mcu_event_notifier(TAG_PDR, CMD_DATA_REQ, get_pdr_data_from_mcu);
        unregister_mcu_event_notifier(TAG_PDR, SUB_CMD_FLP_PDR_UNRELIABLE_REQ, get_pdr_notify_from_mcu);
#ifdef GEOFENCE_BATCH_FEATURE
        unregister_mcu_event_notifier(TAG_FLP, CMD_CMN_CONFIG_REQ, get_data_from_mcu);
#endif
#else
        unregister_mcu_event_notifier(TAG_PDR, CMD_FLP_PDR_DATA_REQ, get_pdr_data_from_mcu);
        unregister_mcu_event_notifier(TAG_PDR, CMD_FLP_PDR_UNRELIABLE_REQ, get_pdr_notify_from_mcu);
#ifdef GEOFENCE_BATCH_FEATURE
        unregister_mcu_event_notifier(TAG_FLP, CMD_FLP_LOCATION_UPDATE_REQ, get_common_data_from_mcu);
        unregister_mcu_event_notifier(TAG_FLP, CMD_FLP_GEOF_TRANSITION_REQ, get_common_data_from_mcu);
        unregister_mcu_event_notifier(TAG_FLP, CMD_FLP_GEOF_MONITOR_STATUS_REQ, get_common_data_from_mcu);
#endif
#endif
	genl_unregister_family(&flp_genl_family);
	misc_deregister(&hisi_flp_miscdev);
	return 0;
}
EXPORT_SYMBOL_GPL(hisi_flp_unregister);
