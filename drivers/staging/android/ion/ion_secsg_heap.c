/*
 * /ion/ion_secsg_heap.c
 *
 * Copyright (C) 2011 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt) "secsg: " fmt

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/dma-mapping.h>
#include <linux/dma-contiguous.h>
#include <linux/genalloc.h>
#include <linux/mutex.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/cma.h>
#include <linux/sizes.h>
#include <linux/hisi/hisi_cma.h>
#include <linux/hisi/hisi_ion.h>
#include <linux/hisi/hisi_drmdriver.h>
#include <linux/hisi/hisi_mm.h>
#include <linux/workqueue.h>
#include <teek_client_api.h>
#include <teek_client_id.h>
#include <teek_client_constants.h>
#include <linux/memblock.h>

#include <linux/sched.h>
#include <asm/tlbflush.h>
#include <linux/list.h>

#include "ion.h"
#include "ion_priv.h"

/*uuid to TA: f8028dca-aba0-11e6-80f5-76304dec7eb7*/
#define UUID_TEEOS_TZMP2_IonMemoryManagement \
{ \
	0xf8028dca,\
	0xaba0,\
	0x11e6,\
	{ \
		0x80, 0xf5, 0x76, 0x30, 0x4d, 0xec, 0x7e, 0xb7 \
	} \
}

enum SECSG_HEAP_TYPE {
	HEAP_NORMAL = 0,
	HEAP_PROTECT = 1,
	HEAP_SECURE = 2,
	HEAP_MAX,
};

#define ION_PBL_SHIFT 12
#define ION_NORMAL_SHIFT 16
#define DEVICE_MEMORY PROT_DEVICE_nGnRE

#define SECBOOT_CMD_ID_MEM_ALLOCATE 0x1

#ifdef CONFIG_HISI_ION_SECSG_DEBUG
#define secsg_debug(fmt, ...) \
	pr_info(fmt, ##__VA_ARGS__)
#else
#define secsg_debug(fmt, ...)
#endif

struct ion_secsg_heap {
	/* heap attr: secure, protect, non_sec */
	u32 heap_attr;
	u32 pool_shift;
	/* heap total size*/
	size_t heap_size;
	/* heap allocated size*/
	unsigned long alloc_size;
	struct ion_heap heap;
	struct device_node *nd;
	struct device *dev;
	struct cma *cma;
	struct gen_pool *pool;
	/* heap mutex */
	struct mutex mutex;
	/* heap flag */
	u64 flag;
	u64 per_alloc_sz;
	/* align size = 64K*/
	u64 per_bit_sz;
	u64 water_mark;
	struct list_head allocate_head;
	TEEC_Context *context;
	TEEC_Session *session;
	uint32_t origin;
	int TA_init;
};

struct cma *hisi_secsg_cma;

enum SEC_Task{
	SEC_TASK_DRM = 0x0,
	SEC_TASK_SEC,
	SEC_TASK_MAX,
};
struct mem_chunk_list {
	u32 protect_id;
	u32 nents;
	void * phys_addr;  /*Must be the start addr of struct mem_phys */
};

struct mem_phys {
	u32 addr;
	u32 size;
};

struct alloc_list {
	u64 addr;
	u32 size;
	struct list_head list;
};

int hisi_sec_cma_set_up(struct reserved_mem *rmem)
{
	phys_addr_t align = PAGE_SIZE << max(MAX_ORDER - 1, pageblock_order);
	phys_addr_t mask = align - 1;
	unsigned long node = rmem->fdt_node;
	struct cma *cma;
	int err;

	if (!of_get_flat_dt_prop(node, "reusable", NULL) ||
	    of_get_flat_dt_prop(node, "no-map", NULL))
		return -EINVAL;

	if ((rmem->base & mask) || (rmem->size & mask)) {
		pr_err("Reserved memory: incorrect alignment of CMA region\n");
		return -EINVAL;
	}

	if(!memblock_is_memory(rmem->base)){
		memblock_free(rmem->base, rmem->size);
		pr_err("memory is invalid(0x%llx), size(0x%llx)\n",
			rmem->base, rmem->size);
		return -EINVAL;
	}
	err = cma_init_reserved_mem(rmem->base, rmem->size, 0, &cma);
	if (err) {
		pr_err("Reserved memory: unable to setup CMA region\n");
		return err;
	}

	hisi_secsg_cma = cma;
	ion_register_dma_camera_cma((void *)cma);

	return 0;
}
/*lint -e528 -esym(528,RESERVEDMEM_OF_DECLARE)*/
RESERVEDMEM_OF_DECLARE(hisi_secsg_cma, "hisi-cma-pool", hisi_sec_cma_set_up);//lint !e611
/*lint -e528 +esym(528,RESERVEDMEM_OF_DECLARE)*/

static int hisi_ion_TA_init(struct ion_secsg_heap *secsg_heap)
{
	u32 root_id = 2000;
	char package_name[] = "sec_mem";
	TEEC_UUID svc_id = UUID_TEEOS_TZMP2_IonMemoryManagement;
	TEEC_Operation operation = {0};
	TEEC_Result result;

	if(secsg_heap->TA_init){
		secsg_heap->TA_init += 1;
		pr_err("TA had been opened before(%d).\n", secsg_heap->TA_init);
		return 0;
	}
	/* initialize TEE environment */
	result = TEEK_InitializeContext(NULL, secsg_heap->context);
	if(result != TEEC_SUCCESS) {
		pr_err("InitializeContext failed, ReturnCode=0x%x\n", result);
		goto cleanup_1;
	} else {
		secsg_debug("InitializeContext success\n");
	}
	/* operation params create  */
	operation.started = 1;
	operation.cancel_flag = 0;
	/*open session*/
	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE,
			    TEEC_NONE,
			    TEEC_MEMREF_TEMP_INPUT,
			    TEEC_MEMREF_TEMP_INPUT);/*lint !e845*/

	operation.params[2].tmpref.buffer = (void *)(&root_id);/*lint !e789*/
	operation.params[2].tmpref.size = sizeof(root_id);
	operation.params[3].tmpref.buffer = (void *)(package_name);/*lint !e789*/
	operation.params[3].tmpref.size = (size_t)(strlen(package_name) + 1);

	result = TEEK_OpenSession(secsg_heap->context, secsg_heap->session, &svc_id,
				TEEC_LOGIN_IDENTIFY, NULL, &operation, &secsg_heap->origin);
	if(result != TEEC_SUCCESS) {
		pr_err("OpenSession failed, ReturnCode=0x%x, ReturnOrigin=0x%x\n", result, secsg_heap->origin);
		goto cleanup_2;
	} else {
		secsg_debug("OpenSession success\n");
	}
	secsg_heap->TA_init = 1;
	return 0;
cleanup_2:
	TEEK_FinalizeContext(secsg_heap->context);
cleanup_1:
	return -1;
}
static int hisi_ion_TA_exec(struct ion_secsg_heap *secsg_heap,
		struct mem_chunk_list *chunk_list,
		u32 cmd_to_ta)
{
	int ret = 0;
	TEEC_Result result;
	TEEC_Operation operation = {0};
	u32 protect_id = chunk_list->protect_id;

	if(!secsg_heap->TA_init){
		pr_err("[%s] TA inited here, please notice your status.\n", __func__);
		return -1;
	}

	operation.started = 1;
	operation.cancel_flag = 0;
	operation.params[0].value.a = cmd_to_ta;
	operation.params[0].value.b = protect_id;

	operation.paramTypes = TEEC_PARAM_TYPES(
		TEEC_VALUE_INPUT,
		TEEC_VALUE_INPUT,
		TEEC_MEMREF_TEMP_INPUT,
		TEEC_NONE);
	operation.params[1].value.a = chunk_list->nents;
	/* operation.params[1].value.b = ret; receive the return value*/
	/* number of list in CMD buffer alloc/table set/table clean*/
	operation.params[2].tmpref.buffer = chunk_list->phys_addr;
	operation.params[2].tmpref.size = (chunk_list->nents) * sizeof(struct mem_phys);
	result = TEEK_InvokeCommand(secsg_heap->session, SECBOOT_CMD_ID_MEM_ALLOCATE, &operation, &secsg_heap->origin);
	if (result != TEEC_SUCCESS) {
		pr_err("InvokeCommand failed, ReturnCode=0x%x, ReturnOrigin=0x%x\n", result, secsg_heap->origin);
		ret = -1;
	} else {
		ret = 0;
		secsg_debug("Config protect table successfully .\n");
	}
	return ret;
}

static void hisi_ion_TA_finish(struct ion_secsg_heap *secsg_heap)
{
	if(!secsg_heap->TA_init)
		return;
	if(secsg_heap->TA_init && (--secsg_heap->TA_init) > 0){
		pr_err("do not close it for (%d)task is using it.\n",
			secsg_heap->TA_init);
		return;
	}
	TEEK_CloseSession(secsg_heap->session);
	TEEK_FinalizeContext(secsg_heap->context);
	secsg_heap->TA_init = 0;
	secsg_debug("TA closed !\n");
}

static inline void free_alloc_list(struct list_head *head)
{
	struct alloc_list *pos = NULL;
	while(!list_empty(head)){
		pos = list_first_entry(head, struct alloc_list, list);
		list_del(&(pos->list));
		kfree(pos);
	}
}

static u32 count_list_nr(struct ion_secsg_heap *secsg_heap)
{
	u32 nr = 0;
	struct list_head *head = &secsg_heap->allocate_head;
	struct list_head *pos;

	list_for_each(pos, head)
		nr++;
	return nr;
}

static int cons_phys_struct(struct ion_secsg_heap *secsg_heap,
		u32 nents,
		struct list_head *head,
		u32 cmd_to_ta)
{
	u32 i = 0;
	int ret = 0;
	u32 protect_id = SEC_TASK_MAX;
	struct mem_phys *mem_phys;
	struct mem_chunk_list chunk_list;
	struct list_head *pos = head->prev;
	struct alloc_list *tmp_list = NULL;
	unsigned long size = nents * sizeof(*mem_phys);

	mem_phys = kzalloc( size, GFP_KERNEL);
	if(!mem_phys){
		pr_err("[%s], mem_phys failed(nents = %d)\n", __func__, nents);
		ret = -ENOMEM;
		return ret;
	}
	for(i = 0; (i < nents) && (pos != (head)); i++){
		tmp_list = list_entry(pos, struct alloc_list, list);
		mem_phys[i].addr = (u32)tmp_list->addr;
		mem_phys[i].size = tmp_list->size;
		pos = pos->prev;
	}
	if(i < nents){
		pr_err("[%s], invalid nents(%d) or head!\n", __func__, nents);
		ret = -EINVAL;
		goto out;
	}
	if (secsg_heap->heap_attr == HEAP_SECURE) {
		protect_id = SEC_TASK_SEC;
	} else if (secsg_heap->heap_attr == HEAP_PROTECT) {
		protect_id = SEC_TASK_DRM;
	} else {
		pr_err("not sec heap, return.!\n");
		ret = -EINVAL;
		goto out;
	}
	/*Call TZ Driver here*/
	chunk_list.nents = nents;
	chunk_list.phys_addr = (void *)mem_phys;
	chunk_list.protect_id = protect_id;
	ret = hisi_ion_TA_exec(secsg_heap, &chunk_list, cmd_to_ta);/*lint !e732*/
out:
	kfree(mem_phys);
	return ret;
}

static int __secsg_cma_alloc(struct ion_secsg_heap *secsg_heap,
				unsigned long user_alloc_size)
{
	int ret = 0;
	unsigned long size_remain;
	unsigned long allocated_size;
	unsigned long cma_remain;
	u32 size = secsg_heap->per_alloc_sz;/*lint !e712*/
	u32 per_bit_sz = secsg_heap->per_bit_sz;/*lint !e712*/
	u64 cma_size;
	/* Add for TEEOS ++, per_alloc_size = 64M */
	unsigned long virt;
	struct page *pg;
	struct alloc_list *alloc = NULL;/*lint !e429*/
	/* Add for TEEOS -- */
#ifdef CONFIG_HISI_KERNELDUMP
	int k;
	struct page *tmp_page = NULL;
#endif

	secsg_debug("into %s \n", __func__);
	/* add 64M for every times
	 * per_alloc_sz = 64M, per_bit_sz = 16M(the original min_size)
	 */
	allocated_size = secsg_heap->alloc_size;
	size_remain = gen_pool_avail(secsg_heap->pool);
	cma_size = cma_get_size(secsg_heap->cma);
	cma_remain = cma_size - (allocated_size + size_remain );
	if(secsg_heap->heap_size <= (allocated_size + size_remain )){
		pr_err("heap is full!(allocated(0x%lx), remain(0x%lx), heap_size(0x%lx))\n",
			allocated_size, size_remain, secsg_heap->heap_size);
		return -ENOMEM;
	}

	/* we allocated more than 1M for SMMU page table before.
	 * then, for the last cma alloc , there is no 64M in
	 * cma pool. So, we allocate as much contiguous memory
	 * as we can.
	 */
retry:
	pg = cma_alloc(secsg_heap->cma,
			   (size_t)(1UL << get_order(size)),/*lint !e516 !e866 !e834 !e747*/
			   get_order(per_bit_sz));/*lint !e516 !e866 !e834 !e732 !e747*/
	if (!pg){
		size = size >> 1;
		if((size >= user_alloc_size) && (size >= per_bit_sz)){/*lint !e516 !e866 !e834 !e747*/
			pr_err("retry 0x%x\n", size);
			goto retry;
		}else {
			pr_err("out of memory\n");
			return -ENOMEM;
		}
	}

#ifdef CONFIG_HISI_KERNELDUMP
	tmp_page = pg;
	for (k = 0; k < (int)(1U << get_order(size)); k++) {/*lint !e516 !e866 !e834 !e747*/
		SetPageMemDump(tmp_page);
		tmp_page++;
	}
#endif

	alloc = kzalloc(sizeof(struct alloc_list), GFP_KERNEL);
	if(!alloc){
		pr_err("alloc list failed.\n");
		ret = -ENOMEM;
		goto err_out1;
	}
	alloc->addr = page_to_phys(pg);
	alloc->size = size;
	list_add_tail(&alloc->list, &secsg_heap->allocate_head);

	if (secsg_heap->flag & ION_FLAG_SECURE_BUFFER) {
		ion_flush_all_cpus_caches();
		virt = (unsigned long)__va(alloc->addr);/*lint !e834 !e648*/
		/*lint -save -e747*/
		create_mapping_late(alloc->addr,
				virt,
				size,
				__pgprot(DEVICE_MEMORY));
		/*lint -restore*/
		flush_tlb_all();
		if(cons_phys_struct(secsg_heap, 1,
			&(secsg_heap->allocate_head), ION_SEC_CMD_TABLE_SET)){
			pr_err("cons_phys_struct failed \n");
			ret = -EINVAL;
			goto err_out2;
		}
	}else {
		memset(page_address(pg), 0x0, size);/*lint !e747*/ /* unsafe_function_ignore: memset  */
	}
	gen_pool_free(secsg_heap->pool, page_to_phys(pg), size);/*lint !e747*/
	secsg_debug("out %s %u MB memory(ret = %d). \n",
			__func__, (size) / SZ_1M, ret);
	return 0;/*lint !e429*/
err_out2:
	/*lint -save -e747 */
	create_mapping_late(alloc->addr,
			virt,
			size,
			__pgprot(PAGE_KERNEL));
	/*lint -restore*/
	list_del(&alloc->list);
	kfree(alloc);
err_out1:
	cma_release(secsg_heap->cma, pg,
		(1U << get_order(size)));/*lint !e516 !e866 !e834 !e747*/
	return ret;
}

static int __secsg_heap_input_check(struct ion_secsg_heap *secsg_heap,
				    unsigned long size, unsigned long flag)
{
	if ((secsg_heap->alloc_size + size) > secsg_heap->heap_size) {
		pr_err("alloc size = 0x%lx, size = 0x%lx, heap size = 0x%lx\n",
			secsg_heap->alloc_size, size, secsg_heap->heap_size);
		return -EINVAL;
	}
	if (((secsg_heap->heap_attr == HEAP_SECURE) ||
	     (secsg_heap->heap_attr == HEAP_PROTECT)) &&
	    !(flag & ION_FLAG_SECURE_BUFFER)) {
		pr_err("allocating memory w/o sec flag in sec heap(%u)\n",
		       secsg_heap->heap_attr);
		return -EINVAL;
	}
	if ((secsg_heap->heap_attr == HEAP_NORMAL) &&
	    (flag & ION_FLAG_SECURE_BUFFER)) {
		pr_err("invalid allocate sec memory in normal heap\n");
		return -EINVAL;
	}
	secsg_heap->flag = flag;
	return 0;
}
static int __secsg_create_pool(struct ion_secsg_heap *secsg_heap)
{
	u64 cma_base;
	u64 cma_size;
	int ret = 0;

	secsg_debug("into %s\n", __func__);
	/* Allocate on 4KB boundaries (1 << ION_PBL_SHIFT)*/
	secsg_heap->pool = gen_pool_create(secsg_heap->pool_shift, -1);

	if (!secsg_heap->pool) {
		pr_err("in __secsg_create_pool create failed\n");
		return -ENOMEM;
	}
	/* Add all memory to genpool first，one chunk only*/
	cma_base = cma_get_base(secsg_heap->cma);
	cma_size = cma_get_size(secsg_heap->cma);
	if (gen_pool_add(secsg_heap->pool, cma_base, cma_size, -1)) {
		pr_err("cma_base 0x%llx cma_size 0x%llx\n", cma_base, cma_size);
		ret = -ENOMEM;
		goto err_add;
	}

	/* Alloc the 512M memory first*/
	if (!gen_pool_alloc(secsg_heap->pool, cma_size)) {
		pr_err("in __secsg_create_pool alloc failed\n");
		ret = -ENOMEM;
		goto err_alloc;
	}
	return 0;
err_alloc:
	gen_pool_destroy(secsg_heap->pool);
err_add:
	secsg_heap->pool = NULL;
	return ret;
}

static void __secsg_pool_release(struct ion_secsg_heap *secsg_heap)
{
	u32 nents;
	u64 addr;
	u32 size;
	unsigned long virt;
	unsigned long size_remain = 0;
	unsigned long offset = 0;/*lint !e438 !e529 */
	struct alloc_list *pos;

	if (secsg_heap->flag & ION_FLAG_SECURE_BUFFER) {
		nents = count_list_nr(secsg_heap);
		if (nents &&
		    cons_phys_struct(secsg_heap, nents,
				     &secsg_heap->allocate_head,
				     ION_SEC_CMD_TABLE_CLEAN)) {
			pr_err("heap_type:(%u)unconfig failed!!!\n",
			       secsg_heap->heap_attr);
			goto out;
		}
	}

	if (!list_empty(&secsg_heap->allocate_head)) {
		list_for_each_entry(pos, &secsg_heap->allocate_head, list) {
			addr = pos->addr;
			size = pos->size;
			virt = (unsigned long)__va(addr);/*lint !e834 !e648*/
			/*lint -save -e747 */
			if (secsg_heap->flag & ION_FLAG_SECURE_BUFFER) {
				create_mapping_late(addr,
						    virt,
						    size,
						    __pgprot(PAGE_KERNEL));
			}
			/*lint -restore*/
			cma_release(secsg_heap->cma, phys_to_page(addr),
				    (1U << get_order(size)));/*lint !e516 !e866 !e834 !e747*/
			offset = gen_pool_alloc(secsg_heap->pool, size);/*lint !e747*/
			if (!offset)
				pr_err("%s:%d:gen_pool_alloc failed!\n",
				       __func__, __LINE__);
		}
		free_alloc_list(&secsg_heap->allocate_head);
	}
out:
	size_remain = gen_pool_avail(secsg_heap->pool);

	secsg_debug("out %s, size_remain = 0x%lx(0x%lx)\n",
		__func__, size_remain, offset);
	return;/*lint !e438*/
}/*lint !e550*/

static int __secsg_alloc(struct ion_secsg_heap *secsg_heap,
				struct ion_buffer *buffer,
				unsigned long size)
{
	int ret = 0;
	unsigned long offset = 0;
	struct sg_table *table;

	table = kzalloc(sizeof(*table), GFP_KERNEL);
	if (!table){
		pr_err("[%s] kzalloc failed .\n", __func__);
		return -ENOMEM;
	}

	if (sg_alloc_table(table, 1, GFP_KERNEL)){
		pr_err("[%s] sg_alloc_table failed .\n", __func__);
		ret = -ENOMEM;
		goto err_out1;
	}
	/*align size*/
	offset = gen_pool_alloc(secsg_heap->pool, size);
	if(!offset){
		/**
		 * For the drm memory is also used by Camera,
		 * So when drm create memory pool, need clean
		 * the camera drm heap.
		 */
		ion_clean_dma_camera_cma();

		ret = __secsg_cma_alloc(secsg_heap, size);
		if (ret)
			goto err_out2;
		offset = gen_pool_alloc(secsg_heap->pool, size);
		if (!offset) {
			ret = -ENOMEM;
			pr_err("line = %d, in __secsg_alloc, gen_pool_alloc failed!\n", __LINE__);
			goto err_out2;
		}
	}
	sg_set_page(table->sgl, pfn_to_page(PFN_DOWN(offset)), size, 0);/*lint !e712 !e747*/
	buffer->priv_virt = table;

	secsg_debug(" out [%s] .\n", __func__);
	return ret;
err_out2:
	sg_free_table(table);
err_out1:
	kfree(table);
	return ret;
}

static void __secsg_free(
	struct ion_secsg_heap *secsg_heap,
	struct sg_table *table,
	struct ion_buffer *buffer)
{
	struct page *page = sg_page(table->sgl);
	ion_phys_addr_t paddr = PFN_PHYS(page_to_pfn(page));

	if (!(buffer->flags & ION_FLAG_SECURE_BUFFER))
		(void)ion_heap_buffer_zero(buffer);
	gen_pool_free(secsg_heap->pool, paddr, buffer->size);

	sg_free_table(table);
	kfree(table);

	secsg_debug("out %s\n", __func__);
}

static int __secsg_fill_watermark(struct ion_secsg_heap *secsg_heap)
{
	struct page *pg;
	u64 size = secsg_heap->water_mark;
	u64 per_bit_sz = secsg_heap->per_bit_sz;
	struct alloc_list *alloc;/*lint !e429*/
#ifdef CONFIG_HISI_KERNELDUMP
	int k;
	struct page *tmp_page = NULL;
#endif

	if (!size || size > secsg_heap->per_alloc_sz)
		return -EINVAL;

	pg = cma_alloc(secsg_heap->cma,
		       (size_t)(1UL << get_order(size)),/*lint !e516 !e866 !e834 !e747*/
		       get_order(per_bit_sz));/*lint !e516 !e866 !e834 !e732 !e747*/
	if (!pg) {
		pr_err("%s:alloc cma fail\n", __func__);
		return -ENOMEM;
	}

#ifdef CONFIG_HISI_KERNELDUMP
	tmp_page = pg;
	for (k = 0; k < (int)(1U << get_order(size)); k++) {/*lint !e516 !e866 !e834 !e747*/
		SetPageMemDump(tmp_page);
		tmp_page++;
	}
#endif

	alloc = kzalloc(sizeof(*alloc), GFP_KERNEL);
	if (!alloc)
		goto err;

	alloc->addr = page_to_phys(pg);
	alloc->size = size;
	list_add_tail(&alloc->list, &secsg_heap->allocate_head);

	memset(page_address(pg), 0x0, size);/*lint !e747*//* unsafe_function_ignore: memset */
	gen_pool_free(secsg_heap->pool, page_to_phys(pg), size);/*lint !e747*/
	secsg_debug("out %s %llu MB memory.\n",
		    __func__, (size) / SZ_1M);
	return 0;/*lint !e429*/
err:
	cma_release(secsg_heap->cma, pg,
		    (1U << get_order(size)));/*lint !e516 !e866 !e834 !e747*/
	return -ENOMEM;
}

static void ion_secsg_heap_free(struct ion_buffer *buffer)
{
	struct ion_heap *heap = buffer->heap;
	struct sg_table *table = buffer->priv_virt;
	struct ion_secsg_heap *secsg_heap =
		container_of(heap, struct ion_secsg_heap, heap);/*lint !e826*/

	mutex_lock(&secsg_heap->mutex);
	__secsg_free(secsg_heap, table, buffer);
	WARN_ON(secsg_heap->alloc_size < buffer->size);
	secsg_heap->alloc_size -= buffer->size;
	if (!secsg_heap->alloc_size) {
		__secsg_pool_release(secsg_heap);
		if (secsg_heap->water_mark &&
		    __secsg_fill_watermark(secsg_heap))
			pr_err("__secsg_fill_watermark failed!\n");
	}
	if(!secsg_heap->alloc_size &&
		(secsg_heap->flag & ION_FLAG_SECURE_BUFFER)){
		hisi_ion_TA_finish(secsg_heap);
	}
	mutex_unlock(&secsg_heap->mutex);
	secsg_debug("out %s size 0x%lx heap id %u, heap allocate %lx\n", __func__,
			buffer->size, heap->id, secsg_heap->alloc_size);
}

static int ion_secsg_heap_allocate(struct ion_heap *heap,
		struct ion_buffer *buffer,
		unsigned long size, unsigned long align,
		unsigned long flags)
{
	struct ion_secsg_heap *secsg_heap =
		container_of(heap, struct ion_secsg_heap, heap);/*lint !e826*/
	int ret = 0;

	mutex_lock(&secsg_heap->mutex);

	if (__secsg_heap_input_check(secsg_heap, size, flags)){
		pr_err("input params failed\n");
		ret = -EINVAL;
		goto err_mutex;
	}
	/*init the TA conversion here*/
	if(!secsg_heap->alloc_size &&
		(flags & ION_FLAG_SECURE_BUFFER) &&
		hisi_ion_TA_init(secsg_heap)){
		pr_err("[%s] TA init failed\n", __func__);
		ret = -1;
		goto err_mutex;
	}
	if (__secsg_alloc(secsg_heap, buffer, size)) {/*lint !e838*/
		pr_err("[%s] secsg_alloc failed, size = 0x%lx.\n",
			__func__, secsg_heap->alloc_size);
		ret = -ENOMEM;
		goto err_alloc_out;
	}
	secsg_heap->alloc_size += size;
	mutex_unlock(&secsg_heap->mutex);
	secsg_debug("out %s  size 0x%lx heap id %u\n",
			__func__, size, heap->id);
	return 0;
err_alloc_out:
	if (!secsg_heap->alloc_size)
		__secsg_pool_release(secsg_heap);
	if(!secsg_heap->alloc_size &&
		(flags & ION_FLAG_SECURE_BUFFER)){
		hisi_ion_TA_finish(secsg_heap);
	}
err_mutex:
	mutex_unlock(&secsg_heap->mutex);
	return ret;
}/*lint !e715*/

static int ion_secsg_heap_phys(struct ion_heap *heap,
				struct ion_buffer *buffer,
				ion_phys_addr_t *addr, size_t *len)
{
	/* keep the input parames for compatible with other heaps*/
	/* TZ driver can call "ion_phys" with ion_handle input*/
	struct sg_table *table = buffer->priv_virt;
	struct page *page = sg_page(table->sgl);

	ion_phys_addr_t paddr = PFN_PHYS(page_to_pfn(page));
	*addr = paddr;
	*len = buffer->size;
	return 0;
}/*lint !e715*/

static int ion_secsg_heap_map_user(struct ion_heap *heap,
		struct ion_buffer *buffer,
		struct vm_area_struct *vma)
{
	struct ion_secsg_heap *secsg_heap =
		container_of(heap, struct ion_secsg_heap, heap);/*lint !e826*/
	if ((secsg_heap->heap_attr == HEAP_SECURE) ||
	    (secsg_heap->heap_attr == HEAP_PROTECT)) {
		pr_err("secure buffer, can not call %s\n", __func__);
		return -EINVAL;
	}
	return ion_heap_map_user(heap, buffer, vma);
}

static void *ion_secsg_heap_map_kernel(struct ion_heap *heap,
		struct ion_buffer *buffer)
{
	struct ion_secsg_heap *secsg_heap =
		container_of(heap, struct ion_secsg_heap, heap);/*lint !e826*/
	if ((secsg_heap->heap_attr == HEAP_SECURE) ||
	    (secsg_heap->heap_attr == HEAP_PROTECT)) {
		pr_err("secure buffer, can not call %s\n", __func__);
		return NULL;
	}
	return ion_heap_map_kernel(heap, buffer);
}

static void ion_secsg_heap_unmap_kernel(struct ion_heap *heap,
		struct ion_buffer *buffer)
{
	struct ion_secsg_heap *secsg_heap =
		container_of(heap, struct ion_secsg_heap, heap);/*lint !e826*/
	if ((secsg_heap->heap_attr == HEAP_SECURE) ||
	    (secsg_heap->heap_attr == HEAP_PROTECT)) {
		pr_err("secure buffer, can not call %s\n", __func__);
		return;
	}
	ion_heap_unmap_kernel(heap, buffer);
}

static int ion_secsg_heap_map_iommu(
		struct ion_buffer *buffer,
		struct ion_iommu_map *map_data)
{
	struct ion_secsg_heap *secsg_heap =
		container_of(buffer->heap, struct ion_secsg_heap, heap);/*lint !e826*/
	if ((secsg_heap->heap_attr == HEAP_SECURE) ||
	    (secsg_heap->heap_attr == HEAP_PROTECT)) {
		pr_err("%s:sec or protect buffer can't map iommu\n", __func__);
		return -EINVAL;
	} else {
		return ion_heap_map_iommu(buffer, map_data);
	}
}

static void ion_secsg_heap_unmap_iommu(struct ion_iommu_map *map_data)
{
	struct ion_heap *heap = map_data->buffer->heap;
	struct ion_secsg_heap *secsg_heap =
		container_of(heap, struct ion_secsg_heap, heap);/*lint !e826*/
	if ((secsg_heap->heap_attr == HEAP_SECURE) ||
	    (secsg_heap->heap_attr == HEAP_PROTECT)) {
		pr_err("[%s]secure buffer, do nothing.\n", __func__);
		return;
	}
	ion_heap_unmap_iommu(map_data);
	return;
}

/*lint -save -e715 */
static struct sg_table *ion_secsg_heap_map_dma(struct ion_heap *heap,
						struct ion_buffer *buffer)
{
	return buffer->priv_virt;
}

static void ion_secsg_heap_unmap_dma(struct ion_heap *heap,
						struct ion_buffer *buffer)
{
}
/*lint -restore*/

static struct ion_heap_ops secsg_heap_ops = {
	.allocate = ion_secsg_heap_allocate,
	.free = ion_secsg_heap_free,
	.phys = ion_secsg_heap_phys,
	.map_dma = ion_secsg_heap_map_dma,
	.unmap_dma = ion_secsg_heap_unmap_dma,
	.map_user = ion_secsg_heap_map_user,
	.map_kernel = ion_secsg_heap_map_kernel,
	.unmap_kernel = ion_secsg_heap_unmap_kernel,
	.map_iommu = ion_secsg_heap_map_iommu,
	.unmap_iommu = ion_secsg_heap_unmap_iommu,
};/*lint !e785*/

static int __secsg_parse_dt(struct device *dev,
			    struct ion_platform_heap *heap_data,
			    struct ion_secsg_heap *secsg_heap)
{
	struct device_node *nd;
	u64 per_bit_sz = 0;
	u64 per_alloc_sz = 0;
	u64 water_mark = 0;
	u32 heap_attr = 0;
	u32 pool_shift = ION_PBL_SHIFT;
	int ret;

	nd = of_get_child_by_name(dev->of_node, heap_data->name);
	if (!nd) {
		pr_err("can't of_get_child_by_name %s\n", heap_data->name);
		ret = -EINVAL;
		goto out;
	}
	secsg_heap->nd = nd;

	ret = of_property_read_u64(nd, "per-alloc-size", &per_alloc_sz);
	if (ret < 0) {
		pr_err("can't find prop:%s\n", "per-alloc-size");
		goto out;
	}
	secsg_heap->per_alloc_sz = per_alloc_sz;

	ret = of_property_read_u64(nd, "per-bit-size", &per_bit_sz);
	if (ret < 0) {
		pr_err("can't find prop:%s\n", "per-bit-size");
		goto out;
	}
	secsg_heap->per_bit_sz = per_bit_sz;

	ret = of_property_read_u64(nd, "water-mark", &water_mark);
	if (ret < 0) {
		pr_err("can't find prop:water-mark\n");
		water_mark = 0;
	}
	secsg_heap->water_mark = water_mark;

	ret = of_property_read_u32(nd, "pool-shift", &pool_shift);
	if (ret < 0) {
		pr_err("can not find pool-shift.\n");
		pool_shift = ION_PBL_SHIFT;
	}
	secsg_heap->pool_shift = pool_shift;
	ret = 0;
	ret = of_property_read_u32(nd, "heap-attr", &heap_attr);
	if (ret < 0) {
		pr_err("can not find heap-arrt.\n");
		heap_attr = HEAP_NORMAL;
	}
	if (heap_attr >= HEAP_MAX)
		heap_attr = HEAP_NORMAL;
	secsg_heap->heap_attr = heap_attr;



out:
	return ret;
}

struct ion_heap *ion_secsg_heap_create(struct ion_platform_heap *heap_data)
{
	int ret;
	struct device *dev;
	struct ion_secsg_heap *secsg_heap;

	secsg_heap = kzalloc(sizeof(*secsg_heap), GFP_KERNEL);
	if (!secsg_heap)
		return ERR_PTR(-ENOMEM);/*lint !e747*/

	mutex_init(&secsg_heap->mutex);

	secsg_heap->pool = NULL;
	secsg_heap->heap.ops = &secsg_heap_ops;
	secsg_heap->heap.type = ION_HEAP_TYPE_SECSG;
	secsg_heap->heap_size = heap_data->size;
	secsg_heap->alloc_size = 0;
	dev = heap_data->priv;
	secsg_heap->dev = dev;
	INIT_LIST_HEAD(&secsg_heap->allocate_head);

	if (!hisi_secsg_cma) {
		pr_err("hisi_secsg_cma failed.\n");
		goto mutex_err;
	}
	secsg_heap->cma = hisi_secsg_cma;

	ret = __secsg_parse_dt(dev, heap_data, secsg_heap);
	if (ret)
		goto mutex_err;

	if ((secsg_heap->heap_attr == HEAP_SECURE) ||
	    (secsg_heap->heap_attr == HEAP_PROTECT)) {
		secsg_heap->context = kzalloc(sizeof(TEEC_Context), GFP_KERNEL);
		if (!secsg_heap->context)
			goto mutex_err;
		secsg_heap->session = kzalloc(sizeof(TEEC_Session), GFP_KERNEL);
		if (!secsg_heap->session)
			goto free_context;
	} else {
		secsg_heap->context = NULL;
		secsg_heap->session = NULL;
	}
	secsg_heap->origin = 0;
	secsg_heap->TA_init = 0;

	ret = __secsg_create_pool(secsg_heap);
	if (ret) {
		pr_err("[%s] pool create failed.\n", __func__);
		goto free_session;
	}

	if (secsg_heap->water_mark &&
	    __secsg_fill_watermark(secsg_heap))
		pr_err("__secsg_fill_watermark failed!\n");

	pr_err("secsg heap info %s:\n"
		  "\t\t\t\t heap attr : %u\n"
		  "\t\t\t\t pool shift : %u\n"
		  "\t\t\t\t heap size : %lu MB\n"
		  "\t\t\t\t per alloc size :  %llu MB\n"
		  "\t\t\t\t per bit size : %llu KB\n"
		  "\t\t\t\t water_mark size : %llu MB\n"
		  "\t\t\t\t cma base : 0x%llx\n"
		  "\t\t\t\t cma size : 0x%lx\n",
		  heap_data->name,
		  secsg_heap->heap_attr,
		  secsg_heap->pool_shift,
		  secsg_heap->heap_size / SZ_1M,
		  secsg_heap->per_alloc_sz / SZ_1M,
		  secsg_heap->per_bit_sz / SZ_1K,
		  secsg_heap->water_mark / SZ_1M,
		  cma_get_base(secsg_heap->cma),
		  cma_get_size(secsg_heap->cma));

	return &secsg_heap->heap;/*lint !e429*/
free_session:
	kfree(secsg_heap->session);
free_context:
	kfree(secsg_heap->context);
mutex_err:
	kfree(secsg_heap);
	return ERR_PTR(-ENOMEM);/*lint !e747*/
}

void ion_secsg_heap_destroy(struct ion_heap *heap)
{
	struct ion_secsg_heap *secsg_heap =
		container_of(heap, struct ion_secsg_heap, heap);/*lint !e826*/

	if (secsg_heap->context)
		kfree(secsg_heap->context);
	if (secsg_heap->session)
		kfree(secsg_heap->session);
	kfree(secsg_heap);
}
