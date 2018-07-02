/*
 * Hisilicon hisi ION Driver
 *
 * Copyright (c) 2015 Hisilicon Limited.
 *
 * Author: Chen Feng <puck.chen@hisilicon.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) "Ion: " fmt

#include <linux/export.h>
#include <linux/err.h>
#include <linux/hisi/hisi_ion.h>
#include <linux/platform_device.h>
#include <linux/hisi/hisi-iommu.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/sched.h>
#include <linux/rwsem.h>
#include <linux/uaccess.h>
#include <linux/dma-mapping.h>
#include <linux/dma-contiguous.h>
#include <asm/cacheflush.h>
#include <asm/cpu.h>
#include <linux/compat.h>
#include <linux/sizes.h>
#include <ion_priv.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_fdt.h>
#include <asm/cputype.h>
#include <asm/topology.h>

#ifdef CONFIG_HISI_SMARTPOOL_OPT
#include "hisi/hisi_ion_smart_pool.h"
#endif
#ifdef CONFIG_HISI_SPECIAL_SCENE_POOL
#include "hisi/hisi_ion_scene_pool.h"
#endif

#define MAX_HISI_ION_DYNAMIC_AREA_NAME_LEN  64
struct hisi_ion_dynamic_area {
	phys_addr_t    base;
	unsigned long  size;
	char           name[MAX_HISI_ION_DYNAMIC_AREA_NAME_LEN];
};

struct hisi_ion_type_table {
	const char *name;
	enum ion_heap_type type;
};

static const struct hisi_ion_type_table ion_type_table[] = {
	{"ion_system", ION_HEAP_TYPE_SYSTEM},
	{"ion_system_contig", ION_HEAP_TYPE_SYSTEM_CONTIG},
	{"ion_carveout", ION_HEAP_TYPE_CARVEOUT},
	{"ion_chunk", ION_HEAP_TYPE_CHUNK},
	{"ion_dma", ION_HEAP_TYPE_DMA},
	{"ion_custom", ION_HEAP_TYPE_CUSTOM},
#ifdef CONFIG_ION_HISI_SECCM
	{"ion_sec", ION_HEAP_TYPE_SECCM},
#endif
#ifdef CONFIG_ION_HISI_SECSG
	{"ion_sec", ION_HEAP_TYPE_SECSG},
#endif
	{"ion_dma_pool", ION_HEAP_TYPE_DMA_POOL},
#ifdef CONFIG_ION_HISI_FAMA_MISC
	{"ion_fama_misc", ION_HEAP_TYPE_FAMA_MISC},
#endif
};

static struct ion_device *idev;
static int num_heaps;
static struct ion_heap **heaps;
static struct ion_platform_heap **heaps_data;

static void __iomem *cluster0_resume_bit;
static void __iomem *cluster1_resume_bit;

#define MAX_HISI_ION_DYNAMIC_AREA_NUM  5
static struct hisi_ion_dynamic_area  ion_dynamic_area_table[MAX_HISI_ION_DYNAMIC_AREA_NUM];
static int ion_dynamic_area_count = 0;

static int ion_cpu_map[CONFIG_NR_CPUS] = {0};

static int add_dynamic_area(phys_addr_t base, unsigned long  len, const char* name)
{
	int ret = 0;
	int i = ion_dynamic_area_count;

	if (i < MAX_HISI_ION_DYNAMIC_AREA_NUM) {
		ion_dynamic_area_table[i].base = base;
		ion_dynamic_area_table[i].size  = len;
		strncpy(ion_dynamic_area_table[i].name, name, /* unsafe_function_ignore: strncpy  */
				MAX_HISI_ION_DYNAMIC_AREA_NAME_LEN-1);
		ion_dynamic_area_table[i].name[MAX_HISI_ION_DYNAMIC_AREA_NAME_LEN-1] = '\0';
		pr_err("insert heap-name %s \n", ion_dynamic_area_table[i].name);

		ion_dynamic_area_count ++;

		return ret;

	}

	return -EFAULT;
}

static struct hisi_ion_dynamic_area* find_dynamic_area_by_name(const char* name)
{
	int i = 0;

	if (!name) {
		return NULL;
	}

	for (; i < MAX_HISI_ION_DYNAMIC_AREA_NUM; i++) {
		pr_err("name = %s, table name %s \n", name, ion_dynamic_area_table[i].name);
		if (!strcmp(name, ion_dynamic_area_table[i].name)) { /*lint !e421 */
			return &ion_dynamic_area_table[i];
		}
	}

	return NULL;
}

static int  hisi_ion_reserve_area(struct reserved_mem *rmem)
{
	char *status   = NULL;
	int  namesize = 0;
	const char* heapname;

	status = (char *)of_get_flat_dt_prop(rmem->fdt_node, "status", NULL);
	if (status && (strncmp(status, "ok", strlen("ok")) != 0))
		return 0;

	heapname = of_get_flat_dt_prop(rmem->fdt_node, "heap-name", &namesize);
	if (!heapname || (namesize <= 0)) {
		pr_err("no 'heap-name' property namesize=%d\n", namesize);
		return -EFAULT;
	}

	pr_info("base 0x%llx, size is 0x%llx, node name %s, heap-name %s namesize %d,"
			"[%d][%d][%d][%d]\n",
			rmem->base, rmem->size, rmem->name, heapname, namesize,
			heapname[0],heapname[1],heapname[2],heapname[3] );

	if (add_dynamic_area(rmem->base, rmem->size, heapname)) {
		pr_err("fail to add to dynamic area \n");
		return -EFAULT;
	}

	return 0;
}
RESERVEDMEM_OF_DECLARE(hisi_ion, "hisi_ion", hisi_ion_reserve_area); /*lint !e611 */
struct ion_device *get_ion_device(void) {
	return idev;
}

#ifdef CONFIG_HISI_SPECIAL_SCENE_POOL
struct ion_heap *ion_get_system_heap(void)
{
	int i;
	struct ion_heap *ptr_heap;

	for (i = 0; i < num_heaps; i++) {
		ptr_heap = heaps[i];

		if (!ptr_heap)
			continue;
		if (ptr_heap->type == ION_HEAP_TYPE_SYSTEM)
			break;
	}
	if (i >= num_heaps)
		ptr_heap = NULL;

	return ptr_heap;
}
#endif

static void ion_pm_init(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	u32 cluster0_resume_addr = 0, cluster1_resume_addr = 0;
	int ret;

	ret = of_property_read_u32(np, "cluster-0-resume", &cluster0_resume_addr);
	if (ret < 0)
		pr_err("Failed to get cluster-0-resume form dts!\n");

	ret = of_property_read_u32(np, "cluster-1-resume", &cluster1_resume_addr);
	if (ret < 0)
		pr_err("Failed to get cluster-1-resume form dts!\n");

	cluster0_resume_bit = ioremap(cluster0_resume_addr, SZ_4K);
	cluster1_resume_bit = ioremap(cluster1_resume_addr, SZ_4K);
}

static inline void artemis_flush_cache(unsigned int level)
{
	asm volatile("msr s1_1_c15_c14_0, %0" : : "r" (level));
	asm volatile("dsb sy");
	asm volatile("isb");
}

static inline void artemis_flush_cache_all(void)
{
	artemis_flush_cache(0); /*flush L1 cache*/

	artemis_flush_cache(2); /*flush l2 cache*/
}

static void hisi_ion_flush_cache_all(void *dummy)
{
	unsigned int midr = read_cpuid_id();

	if (MIDR_PARTNUM(midr) == ARM_CPU_PART_CORTEX_ARTEMIS)
		artemis_flush_cache_all();
	else
		flush_cache_all();
}

void ion_flush_all_cpus_caches(void)
{
	int cpu;
	unsigned int cluster0_stat;
	unsigned int cluster1_stat;
	unsigned int stat;
	cpumask_t mask;

	preempt_disable();
	cluster0_stat = readl(cluster0_resume_bit) & 0x0f;
	cluster1_stat = readl(cluster1_resume_bit) & 0x0f;

	stat = ~(cluster0_stat | cluster1_stat << 4) & 0xff;
	if (cluster1_stat == 0x0f)
		stat |= (unsigned int)BIT(ion_cpu_map[4]);
	if (cluster0_stat == 0x0f)
		stat |= (unsigned int)BIT(ion_cpu_map[0]);

	cpumask_clear(&mask);
	for_each_online_cpu(cpu) {
		int pos = ion_cpu_map[cpu];
		if (stat & (1 << pos))
			cpumask_set_cpu(cpu, &mask);
	}

	on_each_cpu_mask(&mask, hisi_ion_flush_cache_all, NULL, 1);
	preempt_enable();
}

struct ion_client *hisi_ion_client_create(const char *name)
{
	return ion_client_create(idev, name);
}
EXPORT_SYMBOL(hisi_ion_client_create);

static long hisi_ion_custom_ioctl(struct ion_client *client,
				unsigned int cmd,
				unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case ION_HISI_CUSTOM_PHYS:
	{
		struct ion_phys_data data;
		struct ion_handle *handle;
		ion_phys_addr_t phys_addr = 0;
		size_t size = 0;

		if (copy_from_user(&data, (void __user *)arg,
				sizeof(data))) {
			return -EFAULT;
		}

		handle = ion_import_dma_buf(client, data.fd_buffer);
		if (IS_ERR(handle))
			return PTR_ERR(handle);

		ret = ion_phys(client, handle, &phys_addr, &size);
		if (ret) {
			ion_free(client, handle);
			return ret;
		}

		data.size = size & 0xffffffff;
		data.phys_l = phys_addr & 0xffffffff;
		data.phys_h = (phys_addr >> 32) & 0xffffffff;

		if (copy_to_user((void __user *)arg, &data, sizeof(data))) {
			ion_free(client, handle);
			return -EFAULT;
		}
		ion_free(client, handle);

		break;
	}
	case ION_HISI_CUSTOM_SET_FLAG:
	{
		struct ion_flag_data data;
		struct ion_handle *handle;

		if (copy_from_user(&data, (void __user *)arg,
				sizeof(data))) {
			return -EFAULT;
		}
		if ((data.flags != 0) &&
		    (data.flags != (ION_FLAG_CACHED |
				    ION_FLAG_CACHED_NEEDS_SYNC))) {
			return -EINVAL;
		}
		handle = ion_import_dma_buf(client, data.shared_fd);

		if (IS_ERR(handle)) {
			pr_err("ION_HISI_CUSTOM_SET_FLAG error handle\n");
			return -EINVAL;
		}

		ret = ion_change_flags(client, handle, data.flags);
		ion_free(client, handle);/*lint !e668 */

		break;
	}

#ifdef CONFIG_HISI_SMARTPOOL_OPT
	case ION_HISI_CUSTOM_SET_SMART_POOL_INFO:
	{
		struct ion_smart_pool_info_data smart_pool_info;

		if (copy_from_user(&smart_pool_info, (void __user *)arg,
				sizeof(smart_pool_info))) {
			return -EFAULT;
		}
		if (smart_pool_info.water_mark < MAX_POOL_SIZE)
			ion_smart_set_water_mark(smart_pool_info.water_mark);
		break;
	}
#endif

#ifdef CONFIG_HISI_SPECIAL_SCENE_POOL
	case ION_HISI_CUSTOM_SPECIAL_SCENE_ENTER:
	{
		struct ion_special_scene_pool_info_data scene_pool_info;
		void *pool;

		if (copy_from_user(&scene_pool_info, (void __user *)arg,
				   sizeof(scene_pool_info))) {
			return -EFAULT;
		}
		/*Translate KB to number of pages.*/
		scene_pool_info.water_mark >>= 2;
		if (scene_pool_info.water_mark > MAX_SPECIAL_SCENE_POOL_SIZE)
			scene_pool_info.water_mark =
				MAX_SPECIAL_SCENE_POOL_SIZE;
		pool = ion_get_scene_pool(ion_get_system_heap());
		ion_scene_pool_wakeup_process(pool, F_WAKEUP_AUTOFREE,
					      &scene_pool_info);
		break;
	}

	case ION_HISI_CUSTOM_SPECIAL_SCENE_EXIT:
	{
		struct ion_special_scene_pool_info_data scene_pool_info
				= {0, SPECIAL_SCENE_ALL_WORKERS_MASK, 0};
		void *pool = ion_get_scene_pool(ion_get_system_heap());

		ion_scene_pool_wakeup_process(pool, F_FORCE_STOP,
					      &scene_pool_info);
		break;
	}
#endif

	case ION_HISI_CLEAN_CACHES:
	case ION_HISI_CLEAN_INV_CACHES:
	{
		struct ion_flush_data data;

		if (copy_from_user(&data, (void __user *)arg,
				sizeof(data))) {
			return -EFAULT;
		}
		ret = ion_sync_for_device(client, data.fd);
		break;
	}
	case ION_HISI_INV_CACHES:
	{
		struct ion_flush_data data;

		if (copy_from_user(&data, (void __user *)arg,
				sizeof(data))) {
			return -EFAULT;
		}
		ret = ion_sync_for_cpu(client, data.fd);
		break;
	}
	default:
		return -ENOTTY;
	}

	return ret;
}

#ifdef CONFIG_COMPAT

struct compat_ion_flush_data {
	compat_int_t fd;
	compat_ulong_t vaddr;
	compat_int_t offset;
	compat_int_t length;
};

static int compat_get_ion_flush_data(struct compat_ion_flush_data __user
		*data32, struct ion_flush_data __user *data)
{
	compat_int_t fd;
	compat_ulong_t vaddr;
	compat_int_t offset;
	compat_int_t length;
	int err;

	err = get_user(fd, &data32->fd);
	err |= put_user(fd, &data->fd);
	err |= get_user(vaddr, &data32->vaddr);
	err |= put_user(vaddr, &data->vaddr);
	err |= get_user(offset, &data32->offset);
	err |= put_user(offset, &data->offset);
	err |= get_user(length, &data32->length);
	err |= put_user(length, &data->length);

	return err;
}

static int compat_put_ion_flush_data(struct compat_ion_flush_data __user
				     *data32,
				     struct ion_flush_data __user *data)
{
	compat_int_t fd;
	compat_ulong_t vaddr;
	compat_int_t offset;
	compat_int_t length;
	int err;

	err = get_user(fd, &data->fd);
	err |= put_user(fd, &data32->fd);
	err |= get_user(vaddr, &data->vaddr);
	err |= put_user(vaddr, &data32->vaddr);
	err |= get_user(offset, &data->offset);
	err |= put_user(offset, &data32->offset);
	err |= get_user(length, &data->length);
	err |= put_user(length, &data32->length);

	return err;
}

static long __attribute__((unused)) compat_hisi_ion_custom_ioctl(
				struct ion_client *client,
				unsigned int cmd,
				unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case ION_HISI_CUSTOM_PHYS:
		return hisi_ion_custom_ioctl(client, cmd, arg);
	case ION_HISI_CLEAN_CACHES:
	case ION_HISI_INV_CACHES:
	case ION_HISI_CLEAN_INV_CACHES:
	{
		struct compat_ion_flush_data __user *data32;
		struct ion_flush_data __user *data;
		int err;

		data32 = compat_ptr(arg);
		data = compat_alloc_user_space(sizeof(*data));
		if (data == NULL) {
			pr_err("compat_alloc_user_space failed!\n");
			return -EFAULT;
		}

		err = compat_get_ion_flush_data(data32, data);
		if (err) {
			pr_err("compat_get_ion_flush_data failed!\n");
			return err;
		}

		ret = hisi_ion_custom_ioctl(client, cmd, (unsigned long)data);

		err = compat_put_ion_flush_data(data32, data);

		return ret ? ret : err;

	}
	default:
		return -ENOTTY;
	}
}

#endif

static int get_type_by_name(const char *name, enum ion_heap_type *type)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(ion_type_table); i++) { /*lint !e527 */
		if (strcmp(name, ion_type_table[i].name)) /*lint !e421 */
			continue;

		*type = ion_type_table[i].type;
		return 0;
	}

	return -EINVAL;
}

static int hisi_set_platform_data(struct platform_device *pdev)
{
	unsigned int base = 0;
	unsigned int size = 0;
	unsigned int id = 0;
	const char *heap_name;
	const char *type_name;
	const char *status;
	enum ion_heap_type type = 0; /*lint !e64 */
	int ret = 0;
	struct device_node *np;
	struct device_node *phandle_node;
	struct property *prop;
	struct ion_platform_heap *p_data;
	const struct device_node *dt_node = pdev->dev.of_node;
	int index = 0;

	for_each_child_of_node(dt_node, np)
		num_heaps++;

	heaps_data = devm_kzalloc(&pdev->dev,
				  sizeof(struct ion_platform_heap *) *
				  num_heaps,
				  GFP_KERNEL);
	if (!heaps_data)
		return -ENOMEM;

	for_each_child_of_node(dt_node, np) {
		ret = of_property_read_string(np, "status", &status);
		if (!ret) {
			if (strncmp("ok", status, strlen("ok")))
				continue;
		}

		phandle_node = of_parse_phandle(np, "heap-name", 0);
		if (phandle_node) {
			int len;

			ret = of_property_read_string(phandle_node, "status", &status);
			if (!ret) {
				if (strncmp("ok", status, strlen("ok")))
					continue;
			}

			prop = of_find_property(phandle_node, "heap-name", &len);
			if (!prop) {
				pr_err("no heap-name in phandle of node %s\n", np->name);
				continue;
			}

			if (!prop->value || !prop->length) {
				pr_err("%s %s %d, node %s, invalid phandle, value=%pK,length=%d\n",
						__FILE__,__FUNCTION__,__LINE__,
						np->name, prop->value, prop->length );
				continue;
			} else {
				heap_name = prop->value;
			}
		} else {
			ret = of_property_read_string(np, "heap-name", &heap_name);
			if (ret < 0) {
				pr_err("invalid heap-name in node %s, please check the name \n", np->name);
				continue;
			}

		}

		pr_err("node name [%s], heap-name [%s]\n", np->name, heap_name);

		ret = of_property_read_u32(np, "heap-id", &id);
		if (ret < 0) {
			pr_err("check the id %s\n", np->name);
			continue;
		}

		ret = of_property_read_u32(np, "heap-base", &base);
		if (ret < 0) {
			pr_err("check the base of node %s\n", np->name);
			continue;
		}

		ret = of_property_read_u32(np, "heap-size", &size);
		if (ret < 0) {
			pr_err("check the size of node %s\n", np->name);
			continue;
		}

		ret = of_property_read_string(np, "heap-type", &type_name);
		if (ret < 0) {
			pr_err("check the type of node %s\n", np->name);
			continue;
		}

		ret = get_type_by_name(type_name, &type);
		if (ret < 0) {
			pr_err("type name error %s!\n", type_name);
			continue;
		}
		pr_err("heap index %d : name %s base 0x%x size 0x%x id %d type %d\n",
			index, heap_name, base, size, id, type);

		p_data = devm_kzalloc(&pdev->dev,
				      sizeof(struct ion_platform_heap),
				      GFP_KERNEL);
		if (!p_data)
			return -ENOMEM;

		p_data->name = heap_name;
		p_data->base = base;
		p_data->size = size;
		p_data->id = id;
		p_data->type = type;
		p_data->priv = (void *)&pdev->dev;

		if (!p_data->base && !p_data->size) {
			struct hisi_ion_dynamic_area* area = NULL;
			pr_err("heap %s base =0, try to find dynamic area \n", p_data->name);
			area = find_dynamic_area_by_name(p_data->name);
			if (area) {
				p_data->base = area->base;
				p_data->size = area->size;
				pr_err("have found heap name %s base = 0x%lx, size %zu\n",
						p_data->name,
						p_data->base, p_data->size);
			}
		}

		heaps_data[index] = p_data;
		index++;
	}
	num_heaps = index;
	return 0;
}

static int hisi_ion_probe(struct platform_device *pdev)
{
	int i;
	int err;
	static struct ion_platform_heap *p_heap;

	idev = ion_device_create(hisi_ion_custom_ioctl);
	err = hisi_set_platform_data(pdev);
	if (err) {
		pr_err("ion set platform data error!\n");
		goto err_free_idev;
	}
	heaps = devm_kzalloc(&pdev->dev,
			     sizeof(struct ion_heap *) * num_heaps,
			     GFP_KERNEL);
	if (!heaps) {
		err = -ENOMEM;
		goto err_free_idev;
	}

	/* FIXME will move to iommu driver*/
	if (!hisi_ion_enable_iommu(pdev)) {
		dev_info(&pdev->dev, "enable iommu fail \n");
		err = -EINVAL;
		goto err_free_idev;
	}
	ion_pm_init(pdev);

	/*
	 * create the heaps as specified in the dts file
	 */
	for (i = 0; i < num_heaps; i++) {
		p_heap = heaps_data[i];

		pr_info("id %d  name %s base %lu size %lu\n",
				i, p_heap->name, p_heap->base, p_heap->size);

		heaps[i] = ion_heap_create(p_heap);
		if (IS_ERR_OR_NULL(heaps[i])) {
			pr_err("error add %s of type %d with %lx@%lx\n",
			       p_heap->name, p_heap->type,
			       p_heap->base, (unsigned long)p_heap->size);
			continue;
		}

		ion_device_add_heap(idev, heaps[i]);

		pr_info("adding heap %s of type %d with %lx@%lx\n",
			p_heap->name, p_heap->type,
			p_heap->base, (unsigned long)p_heap->size);
	}
	return 0;

err_free_idev:
	ion_device_destroy(idev);

	return err;
}

static int hisi_ion_remove(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < num_heaps; i++) {
		ion_heap_destroy(heaps[i]);
		heaps[i] = NULL;
	}
	ion_device_destroy(idev);

	return 0;
}

static const struct of_device_id hisi_ion_match_table[] = {
	{.compatible = "hisilicon,hisi-ion"},
	{},
};

static struct platform_driver hisi_ion_driver = {
	.probe = hisi_ion_probe,
	.remove = hisi_ion_remove,
	.driver = {
		.name = "ion-hisi",
		.of_match_table = hisi_ion_match_table,
	},
};

/*lint -e701 -e713*/
static int __init hisi_ion_init(void)
{
	int ret;
	int cpu;

	ret = platform_driver_register(&hisi_ion_driver);

	for_each_present_cpu(cpu){
		int core_id = topology_core_id(cpu);
		int cluster_id = topology_physical_package_id(cpu);
		ion_cpu_map[cpu] = (cluster_id << 2) + core_id;
	}

	return ret;
}
/*lint +e701 +e713*/

subsys_initcall(hisi_ion_init);
