#ifndef _INTERNAL_HISI_BLK_MQ_H_
#define _INTERNAL_HISI_BLK_MQ_H_

#include <linux/blkdev.h>

#ifdef CONFIG_HISI_BLK_MQ

static inline bool hisi_blk_is_sync(struct request_queue *q, unsigned long rw_flags)
{
#ifdef CONFIG_HISI_MQ_DISPATCH_DECISION
	return ( (!(rw_flags & REQ_WRITE)) || (rw_flags & (REQ_SYNC|REQ_META |REQ_FLUSH |REQ_FUA)));
#else
	return rw_is_sync(rw_flags);
#endif
}

static inline void hisi_blk_mq_init(struct request_queue *q)
{
}

#endif /* CONFIG_HISI_BLK_MQ */

#endif /* _INTERNAL_HISI_BLK_MQ_H_ */
