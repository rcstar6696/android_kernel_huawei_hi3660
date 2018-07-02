/*
 * record the data to rdr. (RDR: kernel run data recorder.)
 * This file wraps the ring buffer.
 *
 * Copyright (c) 2013 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/string.h>

#include <linux/hisi/rdr_hisi_ap_ringbuffer.h>

int hisiap_ringbuffer_init(struct hisiap_ringbuffer_s *q, u32 bytes,
			   u32 fieldcnt, const char *keys)
{
	if (NULL == q) {
		pr_err("[%s], buffer head is null!\n", __func__);
		return -EINVAL;
	}

	if (bytes <
	    (sizeof(struct hisiap_ringbuffer_s) + sizeof(u8) * fieldcnt)) {
		pr_err("[%s], ringbuffer size [0x%x] is too short!\n",
		       __func__, bytes);
		return -EINVAL;
	}

	/* max_num: records count. */
	q->max_num = (bytes - sizeof(struct hisiap_ringbuffer_s)) /
	    (sizeof(u8) * fieldcnt);
	q->rear = 0;		/* point to the last NULL record. UNIT is record. */
	q->is_full = 0;
	q->field_count = fieldcnt;	/* How many u8 in ONE record. */

	memset(q->keys, 0, HISIAP_KEYS_MAX + 1);
	if (keys)
		strncpy(q->keys, keys, HISIAP_KEYS_MAX);/*lint !e747 */
	return 0;
}

/*lint -e818 */
void hisiap_ringbuffer_write(struct hisiap_ringbuffer_s *q, u8 *element)
{
	int rear;

	if (q->rear >= q->max_num) {
		q->is_full = 1;
		q->rear = 0;
	}

/*lint -e713 */
	rear = q->rear++;
/*lint +e713 */

/*lint -e737 */
	memcpy((void *)&q->data[(long)rear * q->field_count],
	       (void *)element, q->field_count * sizeof(u8));
/*lint +e737 */
	return;
}

/*lint +e818 */

/*************************************************************************
Function:		hisiap_is_ringbuffer_full
Description:	判断ringbuffer是否已经写满；
Input:		buffer_addr: buffer首地址；
Return:		0：buffer未满；1：buffer已满；-1: 查询无效；
*************************************************************************/
int  hisiap_is_ringbuffer_full(const void *buffer_addr)
{
	if (NULL == buffer_addr)
		return -1;

	return (int)(((struct hisiap_ringbuffer_s *)buffer_addr)->is_full);
}/*lint !e818*/

