/*
 * kcode.c
 *
 * the kcode.c for kernel code integrity checking
 *
 * Yongzheng Wu <Wu.Yongzheng@huawei.com>
 * likun <quentin.lee@huawei.com>
 * likan <likan82@huawei.com>
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 */

#include "./include/kcode.h"

static const char *TAG = "kcode";

static const struct memrange {
	char *start;
	char *end;
} ranges[] = {
	{_stext, _etext},
	{NULL, NULL}
};

static const int memrange_num = ARRAY_SIZE(ranges) - 1;
static size_t memrange_size;

/* Do sanity check on _stext, __v7_setup_stack, _etext.
 * The size of android-msm-hammerhead-3.4-lollipop-release is 14583504.
 * We think it's good as long as the size is less than 30M.
 */
static int kcode_verify_ranges(void)
{
	const struct memrange *range;
	char *ptr = NULL;

	memrange_size = 0;
	for (range = ranges; range->start != NULL; range++) {
		if (range->end <= range->start) {
			RSLogError(TAG, "range error 1, start=%p, end=%p",
						range->start, range->end);
			return 1;
		}

		if ((unsigned)((range->end - range->start) - 1) > 30000000 ||
		    (uintptr_t)range->start % 4 ||
		    (uintptr_t)range->end % 4) {
			RSLogError(TAG, "range error 2, start=%p, end=%p",
						range->start, range->end);
			return 1;
		}

		if ((NULL != ptr) && range->start <= ptr) {
			RSLogError(TAG, "range error 3, prev=%p, start=%p",
						ptr, range->start);
			return 1;
		}
		ptr = range->end;
		memrange_size += (unsigned)(range->end - range->start);
	}

	if (memrange_size > 30000000) {
		RSLogError(TAG, "range error 4, memrange_size=%zu",
							memrange_size);
		return 1;
	}
	return 0;
}

int kcode_scan(uint8_t *hash)
{
	struct scatterlist sg[memrange_num];
	struct hash_desc desc;
	int i;
	int err;

	if (memrange_size == 0)
		if (kcode_verify_ranges())
			return -ENOMEM;

	sg_init_table(sg, memrange_num);
	for (i = 0; ranges[i].start != NULL; i++)
		sg_set_buf(&sg[i], ranges[i].start,
			(unsigned int)(ranges[i].end - ranges[i].start));

	desc.flags = 0;

	desc.tfm = crypto_alloc_hash("sha256", 0, (u32)CRYPTO_ALG_ASYNC);
	if (IS_ERR(desc.tfm)) {
		RSLogError(TAG, "crypto_alloc_hash(sha256) error %ld",
							PTR_ERR(desc.tfm));
		return -ENOMEM;
	}
	crypto_hash_init(&desc);
	crypto_hash_update(&desc, sg, memrange_size);
	err = crypto_hash_final(&desc, (u8 *)hash);
	RSLogDebug(TAG, "kscan result %d", err);

	crypto_free_hash(desc.tfm);
	return err;
}

size_t kcode_get_size(void)
{
	return (size_t)(_etext - _stext);
}

void kcode_copy(char *buffer)
{
	memcpy(buffer, _stext, (ulong)(_etext - _stext));
}

int kcode_syscall_scan(uint8_t *hash)
{
	struct scatterlist sg;
	struct hash_desc desc;
	void *ptr = (void *)sys_call_table;
	int err;

	/* define NR_syscalls as 326 */
	size_t size = NR_syscalls * sizeof(void *);

	sg_init_one(&sg, ptr, size);
	desc.flags = 0;
	desc.tfm = crypto_alloc_hash("sha256", 0, CRYPTO_ALG_ASYNC);
	if (desc.tfm == NULL) {
		RSLogError(TAG, "crypto_alloc_hash(sha256) error");
		return -ENOMEM;
	}
	crypto_hash_init(&desc);
	crypto_hash_update(&desc, &sg, size);
	err = crypto_hash_final(&desc, (u8 *)hash);
	RSLogDebug(TAG, "syscallscan result %d", err);

	crypto_free_hash(desc.tfm);
	return err;
}
