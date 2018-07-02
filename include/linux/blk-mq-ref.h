#ifndef _BLK_MQ_REF_H_
#define _BLK_MQ_REF_H_

#include <linux/spinlock.h>

struct blk_mq_refcount {
	spinlock_t lock;
	int live;
	unsigned long c;
	void (*release)(struct blk_mq_refcount *ref);
};

static inline void blk_ref_put(struct blk_mq_refcount *ref)
{
	unsigned long flags;
	unsigned long count;

	spin_lock_irqsave(&ref->lock, flags);
	ref->c--;
	count = ref->c;
	spin_unlock_irqrestore(&ref->lock, flags);
	if (count == 0)
		ref->release(ref);
}

static inline bool blk_ref_tryget_live(struct blk_mq_refcount *ref)
{
	unsigned long flags;

	spin_lock_irqsave(&ref->lock, flags);
	if (ref->live >= 1) {
		ref->c++;
		spin_unlock_irqrestore(&ref->lock, flags);
		return true;
	}

	spin_unlock_irqrestore(&ref->lock, flags);
	return false;
}

static inline void blk_ref_kill(struct blk_mq_refcount *ref)
{
	unsigned long flags;

	spin_lock_irqsave(&ref->lock, flags);
	ref->live--;
	ref->c--;
	if (ref->c == 0) {
		spin_unlock_irqrestore(&ref->lock, flags);
		ref->release(ref);
		return;
	}
	spin_unlock_irqrestore(&ref->lock, flags);
}

static inline unsigned long blk_ref_read(struct blk_mq_refcount *ref)
{
	unsigned long flags;
	unsigned long count;

	spin_lock_irqsave(&ref->lock, flags);
	count = ref->c;
	spin_unlock_irqrestore(&ref->lock, flags);

	return count;
}

static inline bool blk_ref_is_zero(struct blk_mq_refcount *ref)
{
	return (blk_ref_read(ref) == 0);
}


static inline void blk_ref_reinit(struct blk_mq_refcount *ref)
{
	unsigned long flags;

	spin_lock_irqsave(&ref->lock, flags);
	ref->c = 1L;
	ref->live++;
	spin_unlock_irqrestore(&ref->lock, flags);
}

static inline void blk_ref_finish_init(struct blk_mq_refcount *ref)
{
	unsigned long flags;

	spin_lock_irqsave(&ref->lock, flags);
	spin_unlock_irqrestore(&ref->lock, flags);
}

static inline void blk_ref_init(struct blk_mq_refcount *ref,
		void (*release)(struct blk_mq_refcount *ref))
{
	spin_lock_init(&ref->lock);
	ref->c = 1L;
	ref->live = 1;
	ref->release = release;
}
static inline void blk_ref_exit(struct blk_mq_refcount *ref)
{
}

#endif /* _BLK_MQ_REF_H_ */
