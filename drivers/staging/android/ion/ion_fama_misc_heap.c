/*
 * drivers/staging/android/ion/ion_fama_misc_heap.c
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
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/genalloc.h>
#include <linux/io.h>
#include <linux/ion.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/sizes.h>

#include "ion.h"
#include "ion_priv.h"

struct ion_fama_misc_heap {
	struct ion_heap heap;
	struct gen_pool *pool;
	ion_phys_addr_t base;
	size_t size;
};

#define DDR3_BS 0x45D400000
#define DDR4_BS 0x40000000
#define DDR4_DDR3_OFFSET (DDR3_BS - DDR4_BS)

/*lint -e715 -esym(715,*) */
ion_phys_addr_t ion_fama_misc_allocate(struct ion_heap *heap,
				      unsigned long size,
				      unsigned long align)
{
	/*lint -e826 -esym(826,*) */
	struct ion_fama_misc_heap *fama_misc_heap =
		container_of(heap, struct ion_fama_misc_heap, heap);
	unsigned long offset = gen_pool_alloc(fama_misc_heap->pool, size);
	/*lint -e826 +esym(826,*) */
	if (!offset)
		return (ion_phys_addr_t)ION_CARVEOUT_ALLOCATE_FAIL;

	return offset;
}
/*lint -e715 +esym(715,*) */

void ion_fama_misc_free(struct ion_heap *heap, ion_phys_addr_t addr,
		       unsigned long size)
{
	struct ion_fama_misc_heap *fama_misc_heap =
		container_of(heap, struct ion_fama_misc_heap, heap);/*lint !e826 */

	if (addr == (ion_phys_addr_t)ION_CARVEOUT_ALLOCATE_FAIL)
		return;
	gen_pool_free(fama_misc_heap->pool, addr, size/2);
}

/*lint -e715 -esym(715,*) */
static int ion_fama_misc_heap_phys(struct ion_heap *heap,
				  struct ion_buffer *buffer,
				  ion_phys_addr_t *addr, size_t *len)
{
	struct sg_table *table = buffer->priv_virt;
	struct page *page = sg_page(table->sgl);
	ion_phys_addr_t paddr = PFN_PHYS(page_to_pfn(page));

	*addr = paddr;
	*len = buffer->size;
	return 0;
}
/*lint -e715 +esym(715,*) */

/*lint -e715 -esym(715,*) */
static int ion_fama_misc_heap_allocate(struct ion_heap *heap,
				      struct ion_buffer *buffer,
				      unsigned long size, unsigned long align,
				      unsigned long flags)
{
	struct sg_table *table;
	ion_phys_addr_t paddr;
	int ret;
	struct scatterlist *sg;

	/*lint -e50 -esym(50,*)*/
	size = ALIGN(size, SZ_8K);
	/*lint -e50 +esym(50,*)*/
	if (align > PAGE_SIZE)
		return -EINVAL;

	table = kmalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!table)
		return -ENOMEM;
	ret = sg_alloc_table(table, 2, GFP_KERNEL);
	if (ret)
		goto err_free;

	paddr = ion_fama_misc_allocate(heap, size / 2, align);
	if (paddr == (ion_phys_addr_t)ION_CARVEOUT_ALLOCATE_FAIL) {
		ret = -ENOMEM;
		goto err_free_table;
	}

	sg = table->sgl;
	sg_set_page(sg, pfn_to_page(PFN_DOWN(paddr)), (unsigned int)size/2, 0);
	sg = sg_next(sg);
	sg_set_page(sg, pfn_to_page(PFN_DOWN(paddr + DDR4_DDR3_OFFSET)),
			(unsigned int)size / 2, 0);

	buffer->priv_virt = table;

	return 0;

err_free_table:
	sg_free_table(table);
err_free:
	kfree(table);
	return ret;
}
/*lint -e715 +esym(715,*) */

static void ion_fama_misc_heap_free(struct ion_buffer *buffer)
{
	struct ion_heap *heap = buffer->heap;
	struct sg_table *table = buffer->priv_virt;
	struct page *page = sg_page(table->sgl);
	ion_phys_addr_t paddr = PFN_PHYS(page_to_pfn(page));

	ion_fama_misc_free(heap, paddr, buffer->size);

	sg_free_table(table);
	kfree(table);
}

/*lint -e715 -esym(715,*) */
static struct sg_table *ion_fama_misc_heap_map_dma(struct ion_heap *heap,
						  struct ion_buffer *buffer)
{
	return buffer->priv_virt;
}
/*lint -e715 +esym(715,*) */

/*lint -e715 -esym(715,*) */
static void ion_fama_misc_heap_unmap_dma(struct ion_heap *heap,
					struct ion_buffer *buffer)
{
}
/*lint -e715 +esym(715,*) */

static void ion_fama_misc_heap_buffer_zero(struct ion_buffer *buffer)
{
	ion_heap_buffer_zero(buffer);
}

/*lint -e785 -esym(785,*) */
static struct ion_heap_ops fama_misc_heap_ops = {
	.allocate = ion_fama_misc_heap_allocate,
	.free = ion_fama_misc_heap_free,
	.phys = ion_fama_misc_heap_phys,
	.map_dma = ion_fama_misc_heap_map_dma,
	.unmap_dma = ion_fama_misc_heap_unmap_dma,
	.map_user = ion_heap_map_user,
	.map_kernel = ion_heap_map_kernel,
	.unmap_kernel = ion_heap_unmap_kernel,
	.map_iommu = ion_heap_map_iommu,
	.unmap_iommu = ion_heap_unmap_iommu,
	.buffer_zero = ion_fama_misc_heap_buffer_zero,
};
/*lint -e785 +esym(785,*) */

struct ion_heap *ion_fama_misc_heap_create(struct ion_platform_heap *heap_data)
{
	struct ion_fama_misc_heap *fama_misc_heap;
	int ret;

	struct page *page;
	size_t size;

	page = pfn_to_page(PFN_DOWN(heap_data->base));
	size = heap_data->size;

	ion_pages_sync_for_device(NULL, page, size, DMA_BIDIRECTIONAL);

	ret = ion_heap_pages_zero(page, size, pgprot_writecombine(PAGE_KERNEL));
	if (ret)
		return ERR_PTR((long)ret);

	fama_misc_heap = kzalloc(sizeof(struct ion_fama_misc_heap), GFP_KERNEL);
	if (!fama_misc_heap)
		return ERR_PTR((long)-ENOMEM);

	fama_misc_heap->pool = gen_pool_create(PAGE_SHIFT, -1);
	if (!fama_misc_heap->pool) {
		kfree(fama_misc_heap);
		return ERR_PTR((long)-ENOMEM);
	}
	fama_misc_heap->base = heap_data->base;
	fama_misc_heap->size = size;
	gen_pool_add(fama_misc_heap->pool, fama_misc_heap->base, heap_data->size,
		     -1);
	fama_misc_heap->heap.ops = &fama_misc_heap_ops;
	fama_misc_heap->heap.type = ION_HEAP_TYPE_FAMA_MISC;
	fama_misc_heap->heap.flags = ION_HEAP_FLAG_DEFER_FREE;

	return &fama_misc_heap->heap;
}

void ion_fama_misc_heap_destroy(struct ion_heap *heap)
{
	struct ion_fama_misc_heap *fama_misc_heap =
	     container_of(heap, struct  ion_fama_misc_heap, heap);/*lint !e826 */

	gen_pool_destroy(fama_misc_heap->pool);
	kfree(fama_misc_heap);
}
