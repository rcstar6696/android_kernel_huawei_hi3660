#ifndef _LINUX_HISI_CMD_DEBUG_H
#define _LINUX_HISI_CMD_DEBUG_H

static inline unsigned int cma_bitmap_used(struct cma *cma)
{
	unsigned int used;

	mutex_lock(&cma->lock);
	used = (unsigned int)bitmap_weight(cma->bitmap, (unsigned int)cma->count);
	mutex_unlock(&cma->lock);

	return used << cma->order_per_bit;
}

static inline unsigned long int cma_bitmap_maxchunk(struct cma *cma)
{
	unsigned long maxchunk = 0;
	unsigned long start, end = 0;

	mutex_lock(&cma->lock);
	for (;;) {
		start = find_next_zero_bit(cma->bitmap, cma->count, end);
		if (start >= cma->count)
			break;
		end = find_next_bit(cma->bitmap, cma->count, start);
		maxchunk = max(end - start, maxchunk);
	}
	mutex_unlock(&cma->lock);

	return (maxchunk << cma->order_per_bit);
}

#endif