/*
 * HiStar Remote Processor driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>
#include <linux/amba/bus.h>
#include <linux/dma-mapping.h>
#include <linux/remoteproc.h>
#include <linux/platform_data/remoteproc-hisi.h>
#include <linux/hisi/hisi_mailbox.h>
#include <linux/hisi/hisi_rproc.h>
#include "remoteproc_internal.h"
#include <linux/hisi/hisi_mailbox_dev.h>
#include <linux/delay.h>
#include <linux/kfifo.h>
#include <linux/wakelock.h>
#include <linux/mutex.h>
#include <linux/regulator/consumer.h>
#include <linux/scatterlist.h>
#include <linux/clk.h>
#include <linux/rproc_share.h>
#include <linux/sched/rt.h>
#include <linux/kthread.h>
#include <global_ddr_map.h>
#include <asm/cacheflush.h>
#include <linux/firmware.h>
#include <linux/iommu.h>
#include <linux/hisi/hisi-iommu.h>
#include <linux/crc32.h>
#include <soc_acpu_baseaddr_interface.h>
#include <soc_pctrl_interface.h>
#include <linux/ion.h>
#include <linux/hisi/hisi_ion.h>
#include <linux/spinlock.h>
#include "soc_acpu_baseaddr_interface.h"
#include "soc_vivo_bus_interface.h"
#include "soc_crgperiph_interface.h"
#include "isp_ddr_map.h"

#define DTS_COMP_NAME   "hisilicon,isp"

typedef	unsigned int mbox_msg_t;
static unsigned int communicat_msg[8] = {0};
static int perf_para = 0x0;
struct rproc_shared_para *isp_share_para = NULL;

static int debug_mask = 0x05;
#define RPROC_INFO(fmt, args...) \
	do { \
		if (debug_mask & 0x01) \
			printk("Hisi rprocDrv Info: [%s] " fmt, __func__, ##args); \
	} while (0)
#define RPROC_DEBUG(fmt, args...) \
	do { \
		if (debug_mask & 0x02) \
			printk("Hisi rprocDrv Debug: [%s] " fmt, __func__, ##args); \
	} while (0)
#define RPROC_ERR(fmt, args...) \
	do { \
		if (debug_mask & 0x04) \
			printk("Hisi rprocDrv Dump: [%s] " fmt, __func__, ##args); \
	} while (0)

#define RPMSG_RX_FIFO_DEEP  257
#define MBOX_REG_COUNT      8

#define TIMEOUT             200
#define DIS_RSTSATA_OFFSET  0x4
#define EN_RSTSATA_OFFSET   0x8
#define EN_SATA_OFFSET      0xC

#define REG_BASE_CSSYS_PHY  0xEC050000
#define A7PC_OFFSET         0xA0
#define PMEVCNTR0_EL0       0x1000
#define PMCCNTR_EL0         0x107C
#define DBGLAR              0x0FB0
#define DBGOSLAR            0x0300
#define PMLAR               0x1FB0
#define PMCR                0x1E04
#define PMXEVTYPER0         0x1400
#define PMCNTENSET          0x1C00

/* CRG Regs Offset */
#define CRG_C84_PERIPHISP_SEC_RSTDIS        (0xC84)
#define CRG_C88_PERIPHISP_SEC_RSTSTAT       (0xC88)
#define IP_RST_ISPA7                        (1 << 4)
#define IP_RST_ISP                          (1 << 0)

#define CRG_C90_PERIPHISP_ISPA7_CTRL_BASE   (0xC90)
#define ISPA7_DBGPWRDUP                     (1 << 2)
#define ISPA7_VINITHI_HIGH                  (1 << 1)
#define ISPA7_REMAP_ENABLE                  (1 << 11)

/* PMCtrl Regs Offset */
#define PMCTRL_384_NOC_POWER_IDLEACK        (0x384)
#define PMCTRL_388_NOC_POWER_IDLE           (0x388)

/* ISP Regs Offset */
#define REVISION_ID_OFFSET                  (0x20008)

enum hisi_boardid {
    HI3650_BOARDID  = 0x00000003,
    HI6250_BOARDID  = 0x00000004,
    INVALID_BOARDID = 0xFFFFFFFF,
};

#define IS_HI3650(id) (id == HI3650_BOARDID)
#define IS_HI6250(id) (id == HI6250_BOARDID)

enum hisi_rp_mbox_messages {
    RP_MBOX_READY           = 0xFFFFFF00,
    RP_MBOX_PENDING_MSG     = 0xFFFFFF01,
    RP_MBOX_CRASH           = 0xFFFFFF02,
    RP_MBOX_ECHO_REQUEST    = 0xFFFFFF03,
    RP_MBOX_ECHO_REPLY      = 0xFFFFFF04,
    RP_MBOX_ABORT_REQUEST   = 0xFFFFFF05,
};

typedef struct rproc_camera {
    struct clk *aclk;
    struct clk *aclk_dss;
    struct clk *pclk_dss;
	/* pinctrl */
	struct pinctrl *isp_pinctrl;
	struct pinctrl_state *pinctrl_def;
	struct pinctrl_state *pinctrl_idle;
} rproc_camera_t;

struct rproc_boot_device {
    unsigned int boardid;
    struct platform_device *isp_pdev;
    void __iomem *crgperi_base;
    void __iomem *pctrl_base;
    u64 remap_addr;
    u64 nsec_remap_addr;
    void *remap_va;
    struct regulator *isp_subsys_ip;
    struct clk *ispa7_clk;
    struct clk *isp_timer;
    unsigned int ispa7_clk_value;
    struct hisi_isp_rproc *isp_rproc;
    struct isp_plat_cfg tmp_plat_cfg;
    rproc_camera_t isp_data;
    int isp_subsys_power_flag;
    unsigned int a7_ap_mbox;
    unsigned int ap_a7_mbox;
    enum hisi_isp_rproc_case_attr case_type;
	int secisp;
	int rpmsg_status;
    int ispcpu_status;
    struct wake_lock ispcpu_wakelock;
    struct mutex ispcpu_mutex;
    struct wake_lock jpeg_wakelock;
    struct mutex jpeg_mutex;
    struct mutex sharedbuf_mutex;
    struct mutex rpmsg_ready_mutex;
    spinlock_t rpmsg_ready_spin_mutex;
    unsigned int rpmsg_ready_state;
    int bypass_pwr_updn;
#ifdef CONFIG_HISI_REMOTEPROC_FASTCMA_MDC_DEBUG
    struct hisi_isp_fstcma_mdc_s *mdc_fstcma;
#else
    struct hisi_isp_ion_s *mdc_mem_info;
#endif
#ifdef CONFIG_HISI_REMOTEPROC_DMAALLOC_DEBUG
    struct hisi_isp_vring_s hisi_isp_vring[HISI_ISP_VRING_NUM];
#endif
} rproc_boot_dev;

struct isp_rx_mbox {
    struct kfifo rpmsg_rx_fifo;
    spinlock_t rpmsg_rx_lock;
    wait_queue_head_t wait;
    struct task_struct *rpmsg_rx_tsk;
    int can_be_wakeup;
} *isp_rx_mbox;

struct rx_buf_data {
    bool is_have_data;
    unsigned int rpmsg_rx_len;
    mbox_msg_t rpmsg_rx_buf[MBOX_REG_COUNT];
};

struct hisi_isp_rproc {
    struct hisi_mbox *mbox;
    struct notifier_block nb;
    struct rproc *rproc;
};

struct rproc *hisi_rproc;

void __weak ispperf_stop_record(void)
{
    return;
}

void __weak ispperfctrl_update(void)
{
    return;
}

void __weak ispmonitor_update(void)
{
    return;
}

void __weak ispcoresight_update(void)
{
    return;
}
/*lint -save -e454*/
void hisp_lock_sharedbuf(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    mutex_lock(&dev->sharedbuf_mutex);
}

/*lint -save -e455*/
void hisp_unlock_sharedbuf(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    mutex_unlock(&dev->sharedbuf_mutex);
}

int is_ispcpu_powerup(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    return dev->ispcpu_status;
}

int hisp_rproc_enable_iommu(struct rproc *rproc, iommu_fault_handler_t handler)
{
	struct iommu_domain *domain;

	domain = hisi_ion_enable_iommu(NULL);
	if (!domain)
		return -EINVAL;

	rproc->domain = domain;
	//iommu_set_fault_handler(domain, handler, rproc);

	return 0; /*lint !e715 !e438*/
}

void hisp_rproc_disable_iommu(struct rproc *rproc)
{
    rproc->domain = NULL;
}

void hisp_rproc_init(struct rproc *rproc)
{
	if (use_sec_isp())
		rproc->fw_ops = &rproc_bin_fw_ops;
	else
		rproc->fw_ops = &rproc_elf_fw_ops;

	pr_info("[%s] elf.%pK, bin.%pK, use.%pK\n", __func__, (void *)&rproc_elf_fw_ops, (void *)&rproc_bin_fw_ops, (void *)rproc->fw_ops);

	INIT_LIST_HEAD(&rproc->dynamic_mems);
	INIT_LIST_HEAD(&rproc->reserved_mems);
	INIT_LIST_HEAD(&rproc->caches);
	INIT_LIST_HEAD(&rproc->cdas);
	INIT_LIST_HEAD(&rproc->pages);
}
static void hisi_isp_data_text_vunmap(struct rproc *rproc)
{
	struct rproc_mem_entry *entry, *tmp;
    if (use_sec_isp())
        return;
	/* clean up dynamic allocations */
	list_for_each_entry_safe(entry, tmp, &rproc->dynamic_mems, node) {
		if ((!strncmp(entry->priv, "ISP_MEM_TEXT", strlen(entry->priv)) || (!strncmp(entry->priv, "ISP_MEM_DATA", strlen(entry->priv)))))/*lint !e64 */
		{
			vunmap(entry->va);
			list_del(&entry->node);
			kfree(entry);
		}
	}
}
void hisp_rproc_resource_cleanup(struct rproc *rproc)
{
	struct rproc_mem_entry *entry, *tmp;
	struct rproc_page *page_entry, *page_tmp;
	struct rproc_cache_entry *cache_entry, *cache_tmp;
	struct device *dev = &rproc->dev;
	struct page **tmp_page;
	unsigned int i;

	/* clean up debugfs cda entries */
	list_for_each_entry_safe(entry, tmp, &rproc->cdas, node) {
		rproc_remove_trace_file(entry->priv);
		rproc->num_cdas--;
		list_del(&entry->node);
		kfree(entry);
	}

	/* clean up carveout allocations */
	list_for_each_entry_safe(entry, tmp, &rproc->carveouts, node) {
		dma_free_coherent(dev->parent, entry->len, entry->va, entry->dma);
		list_del(&entry->node);
		kfree(entry);
	}

	/* clean up reserved allocations */
	list_for_each_entry_safe(entry, tmp, &rproc->reserved_mems, node) {
            hisp_lock_sharedbuf();
            if (use_nonsec_isp())
                ;
            else {
                iounmap(entry->va);
            }
            /* reset the share parameter pointer */
            if (!use_sec_isp()) {
                if(!strncmp(entry->priv, "ISP_SHARED_MEM", strlen(entry->priv))) {
                    isp_share_para = NULL;
                    pr_info("reserved_mems : %s, isp_share_para: %pK\n",
                        (char *)entry->priv, isp_share_para);/*lint !e559 */
                }
            }
            hisp_unlock_sharedbuf();
	    list_del(&entry->node);
	    kfree(entry);
	}

	/* clean up dynamic allocations */
	list_for_each_entry_safe(entry, tmp, &rproc->dynamic_mems, node) {
		vunmap(entry->va);
		list_del(&entry->node);
		kfree(entry);
	}

	/* free page */
	list_for_each_entry_safe(page_entry, page_tmp, &rproc->pages, node) {
		i = 0;
		tmp_page = (struct page **)page_entry->va;
		while (NULL != tmp_page && i < page_entry->num) {
			if (NULL != *tmp_page)
				__free_page(*tmp_page);
			else
				pr_info("tmp_page: %pK, i: %d, page_num: %u", tmp_page, i, page_entry->num);
			tmp_page++;
			i++;
		}
		vfree(page_entry->va);
		list_del(&page_entry->node);
		kfree(page_entry);
	}

	/* clean up cache entry */
	list_for_each_entry_safe(cache_entry, cache_tmp, &rproc->caches, node) {
		list_del(&cache_entry->node);
		kfree(cache_entry);
	}
}

void hisp_virtio_boot_complete(struct rproc *rproc, int flag)
{
	if (flag) {
		rproc->rproc_enable_flag = false;
		pr_err("[%s] Failed : handle resources flag.%d\n", __func__, flag);
	} else {
		rproc->rproc_enable_flag = true;
	}

	pr_info("[%s] flag.%d\n", __func__, flag);
	complete_all(&rproc->boot_comp);
	pr_info("[%s] -\n", __func__);
}

/*
* take a bootware
*/
static int rproc_bw_load(struct rproc *rproc, const struct firmware *fw)
{
	struct device *dev = &rproc->dev;
	const char *name = rproc->bootware;
	int ret;

	ret = rproc_fw_sanity_check(rproc, fw);
	if (ret) {
		dev_err(dev, "%s:rproc_fw_sanity_check failed \n", __func__);
		return ret;
	}

	dev_info(dev, "Booting fw image %s, size %zd\n", name, fw->size);

	/* load the ELF segments to memory */
	ret = rproc_load_segments(rproc, fw);
	if (ret) {
		dev_err(dev, "Failed to load program segments.%s: %d\n", name, ret);
		return ret;
	}

	return 0;
}

int hisp_rproc_boot(struct rproc *rproc)
{
	const struct firmware *firmware_p = NULL;
	struct device *dev = NULL;
	int ret;

	if (!rproc) {
		pr_err("invalid rproc handle\n");
		return -EINVAL;
	}

	dev = &rproc->dev;

	ret = mutex_lock_interruptible(&rproc->lock);
	if (ret) {
		dev_err(dev, "can't lock rproc %s: %d\n", rproc->name, ret);
		return ret;
	}

	/* prevent underlying implementation from being removed */
	if (!try_module_get(dev->parent->driver->owner)) {
		dev_err(dev, "%s: can't get owner\n", __func__);
		ret = -EINVAL;
		goto unlock_mutex;
	}

	/* skip the boot process if rproc is already powered up */
	if (atomic_inc_return(&rproc->power) > 1) {
		ret = 0;
		goto unlock_mutex;
	}

	dev_info(dev, "powering up %s\n", rproc->name);

	if (use_sec_isp()) {
		ret= sec_rproc_boot(rproc);
		goto unlock_mutex;
	}

	/* loading a firmware is required */
	if (!rproc->firmware) {
		dev_err(dev, "%s: no firmware to load\n", __func__);
		ret = -EINVAL;
		goto unlock_mutex;
	}

	/* loading a bootware is required */
	if (!rproc->bootware) {
		dev_err(dev, "%s: no bootware to load\n", __func__);
		ret = -EINVAL;
		goto unlock_mutex;
	}

	/* load firmware */
	ret = request_firmware(&firmware_p, rproc->firmware, dev);
	if (ret < 0) {
		dev_err(dev, "request_firmware failed: %d\n", ret);
		goto downref_rproc;
	}

	ret = rproc_fw_boot(rproc, firmware_p);
	release_firmware(firmware_p);
	if (0 != ret) {
		pr_err("%s: rproc_fw_boot failed.\n", __func__);
		goto downref_rproc;
	}

	/* load bootware */
	ret = request_firmware(&firmware_p, rproc->bootware, dev);
	if (ret < 0) {
		dev_err(dev, "Failed: bootware request_firmware.%d\n", ret);
		goto downref_rproc;
	}

	ret = rproc_bw_load(rproc, firmware_p);
	release_firmware(firmware_p);
	if (0 != ret) {
		pr_err("%s: rproc_bw_load failed.\n", __func__);
		goto downref_rproc;
	}

	/* flush memory cache */
	rproc_memory_cache_flush(rproc);

downref_rproc:
	if (ret) {
		module_put(dev->parent->driver->owner);
		atomic_dec(&rproc->power);
	}
unlock_mutex:
	mutex_unlock(&rproc->lock);/*lint !e455 */

	return ret;
}

unsigned long hisp_sg2virtio(struct virtqueue *vq, struct scatterlist *sg)
{
	struct virtio_device *vdev = vq->vdev;
	struct rproc_vdev *rvdev = container_of(vdev, struct rproc_vdev, vdev);
	struct rproc *rproc = rvdev->rproc;
	dma_addr_t dma;
    dma = sg_dma_address(sg);
	return ((dma - vq->dma_base) + rproc->ipc_addr);
}

/* rpmsg init msg dma and device addr */
void virtqueue_sg_init(struct scatterlist *sg, void *va, dma_addr_t dma, int size)
{
	sg_init_one(sg, va, size);
	sg_dma_address(sg) = dma;
	sg_dma_len(sg) = size;
}

/*lint -save -e429*/
int rpmsg_vdev_map_resource(struct virtio_device *vdev, dma_addr_t dma, int total_space)
{
    struct rproc_vdev *rvdev = container_of(vdev, struct rproc_vdev, vdev);
    struct rproc *rproc = rvdev->rproc;
    struct rproc_vring *rvring;
    struct rproc_mem_entry *vringmapping[RVDEV_NUM_VRINGS], *vqmapping = NULL;
    int i, j, index, ret = 0, size, prot = 0, unmaped;

    pr_info("[%s] +\n", __func__);
	if (use_sec_isp()) {
		/* map vring */
		for (i = 0; i < RVDEV_NUM_VRINGS; i++) {
			rvring = &rvdev->vring[i];
			index = A7VRING0 + i;
			size = PAGE_ALIGN(vring_size(rvring->len, rvring->align));/*lint !e666 */
			if ((ret = hisp_meminit(index, rvring->dma)) < 0) {
				pr_err("[%s] Failed : hisp_meminit.%d.(0x%x, 0x%llx)\n", __func__, ret, index, (unsigned long long)rvring->dma);
				return ret;
			}
		}

		/* map virtqueue */
		index = A7VQ;
		if ((ret = hisp_meminit(index, dma)) < 0) {
			pr_err("[%s] Failed : hisp_meminit.%d.(0x%x, 0x%llx)\n", __func__, ret, index, (unsigned long long)dma);
			return ret;
		}
		return 0;
	}

	if (!rproc->domain) {
		pr_err("[%s] Failed : domain don't exist!\n", __func__);
		return -EINVAL;
	}

	/* map vring */
    for (i = 0; i < RVDEV_NUM_VRINGS; i++)
        vringmapping[i] = NULL;
	for (i = 0; i < RVDEV_NUM_VRINGS; i++) {
		prot = IOMMU_READ | IOMMU_WRITE;
		rvring = &rvdev->vring[i];
		size = PAGE_ALIGN(vring_size(rvring->len, rvring->align));/*lint !e666 */

		ret = iommu_map(rproc->domain, rvring->da, rvring->dma, size, prot);
		if (ret) {
			pr_err("[%s] Failed : iommu_map.%d\n", __func__, ret);
			return ret;
		}

		vringmapping[i] = kzalloc(sizeof(struct rproc_mem_entry), GFP_KERNEL);
		if (!vringmapping[i]) {
			pr_err("[%s] Failed : vringmapping kzalloc\n", __func__);
			unmaped = iommu_unmap(rproc->domain, rvring->da, size);
			if (unmaped != size)
				pr_err("[%s] Failed : iommu_unmap size.%u, unmaped.%u\n",
						__func__, size, unmaped);
            ret = -ENOMEM;
            goto exit_vdevmap;
		}

		vringmapping[i]->da = rvring->da;
		vringmapping[i]->len = size;
		list_add_tail(&vringmapping[i]->node, &rproc->mappings);
        pr_info("[%s] Vring.%d : 0x%lx -> 0x%x\n", __func__, i, (unsigned long)rvring->dma, (unsigned int)rvring->da);
	}

	/* map virtqueue*/
	prot = IOMMU_READ | IOMMU_WRITE;
	ret = iommu_map(rproc->domain, rproc->ipc_addr, (phys_addr_t)(dma),
                total_space, prot);
	if (ret) {
		pr_err("[%s] Failed : iommu_map.%d, 0x%lx -> 0x%x\n", __func__, ret, (unsigned long)dma, (unsigned int)rproc->ipc_addr);
        goto exit_vdevmap;
	}

	vqmapping = kzalloc(sizeof(struct rproc_mem_entry), GFP_KERNEL);
	if (!vqmapping) {
		pr_err("[%s] Failed : kzalloc\n", __func__);
		unmaped = iommu_unmap(rproc->domain, rproc->ipc_addr, total_space);
		if (unmaped != total_space)
			pr_err("[%s] Failed : iommu_unmap size.%u, unmaped.%u\n",
						__func__, total_space, unmaped);
		ret = -ENOMEM;
        goto exit_vdevmap;
	}
	vqmapping->da = rproc->ipc_addr;
	vqmapping->len = total_space;
	list_add_tail(&vqmapping->node, &rproc->mappings);
    pr_info("[%s] VQ : 0x%lx -> 0x%x\n", __func__, (unsigned long)dma, (unsigned int)rproc->ipc_addr);

	return 0;
exit_vdevmap:
    for (j = 0; j < i; j ++)
        if (vringmapping[j])
            kfree(vringmapping[j]);
    return ret;
}

void set_rpmsg_status(int status)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    dev->rpmsg_status = status;
}

int sec_rproc_boot(struct rproc *rproc)
{
	struct device *dev = &rproc->dev;
	struct resource_table *table, *loaded_table;
	int ret, tablesz;

	if (!rproc->table_ptr)
		return -ENOMEM;

	/* look for the resource table */
	table = rproc_find_rsc_table(rproc, NULL, &tablesz);
	if (!table) {
	    pr_err("[%s] Failed : rproc_find_rsc_table.%pK\n", __func__, table);
		return -EINVAL;
	}

	/* Verify that resource table in loaded fw is unchanged */
	if (rproc->table_csum != crc32(0, table, tablesz)) {
		dev_err(dev, "Failed : resource checksum 0x%x = 0x%x\n", rproc->table_csum, crc32(0, table, tablesz));
		return -EINVAL;
	}
#if 0
	/* handle fw resources which are required to boot rproc */
	ret = rproc_handle_resources(rproc, tablesz, rproc_loading_handlers);
	if (ret) {
		dev_err(dev, "Failed to process resources: %d\n", ret);
		return ret;
    }
#endif
	/* set shared parameters for rproc*/
	ret = rproc_set_shared_para();
	if (ret) {
		dev_err(dev, "Failed : rproc_set_shared_para.%d\n", ret);
		goto clean_up;
	}

	/*
	 * The starting device has been given the rproc->cached_table as the
	 * resource table. The address of the vring along with the other
	 * allocated resources (carveouts etc) is stored in cached_table.
	 * In order to pass this information to the remote device we must
	 * copy this information to device memory.
	 */
	loaded_table = rproc_find_loaded_rsc_table(rproc, NULL);
	if (!loaded_table) {
	    pr_err("[%s] Failed : rproc_find_loaded_rsc_table.%pK\n", __func__, loaded_table);
	    ret = -EINVAL;
		goto clean_up;
	}

	memcpy(loaded_table, rproc->cached_table, tablesz);

	/* power up the remote processor */
	ret = rproc->ops->start(rproc);
	if (ret) {
		dev_err(dev, "can't start rproc %s: %d\n", rproc->name, ret);
		goto clean_up;
	}

	/*
	 * Update table_ptr so that all subsequent vring allocations and
	 * virtio fields manipulation update the actual loaded resource table
	 * in device memory.
	 */
	rproc->table_ptr = loaded_table;

	rproc->state = RPROC_RUNNING;

    return 0;
clean_up:
    rproc_resource_cleanup(rproc);
    return ret;
}

void sec_rscwork_func(struct work_struct *work)
{
    struct rproc *rproc;
    int ret;

    if ((ret = hisp_rsctable_init()) < 0) {
        pr_err("[%s] Failed : hisp_rsctable_init.%d\n", __func__, ret);
        return;
    }

    rproc = container_of(work, struct rproc, sec_rscwork);
    rproc_fw_config_virtio(NULL, rproc);
}

struct rproc_shared_para *rproc_get_share_para(void)
{
    pr_info("%s: enter.\n", __func__);
    if (isp_share_para)
        return isp_share_para;

    pr_info("%s: failed.\n", __func__);
    return NULL;
}

static void free_mdc_ion(unsigned int size)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
#ifdef CONFIG_HISI_REMOTEPROC_FASTCMA_MDC_DEBUG
    struct hisi_isp_fstcma_mdc_s *hisi_mdc_fstcma = dev->mdc_fstcma;
    if(hisi_mdc_fstcma == NULL){
        return;
    }
    hisi_fstcma_free(hisi_mdc_fstcma->mdc_virt_addr,hisi_mdc_fstcma->mdc_dma_addr,MEM_MDC_SIZE);
    kfree(hisi_mdc_fstcma);
    dev->mdc_fstcma = NULL;
#else
    struct hisi_isp_ion_s *hisi_mdc_ion = dev->mdc_mem_info;

    if(hisi_mdc_ion == NULL || size == 0x0){
        return;
    }
    free_nesc_addr_ion(hisi_mdc_ion);
    kfree(hisi_mdc_ion);
    dev->mdc_mem_info = NULL;
#endif
}

static void *get_mdc_addr_va(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
#ifdef CONFIG_HISI_REMOTEPROC_FASTCMA_MDC_DEBUG
    struct hisi_isp_fstcma_mdc_s *hisi_mdc_fstcma = NULL;
    if(dev->mdc_fstcma == NULL)
    {
        pr_err("[%s], NONE mdc_mem_info!\n", __func__);
        return 0;
    }
    pr_debug("get_mdc_addr_va FASTCMA_MDC_DEBUG!\n");
    hisi_mdc_fstcma = dev->mdc_fstcma;/*lint !e838 */
    if(hisi_mdc_fstcma->mdc_virt_addr == NULL)
    {
        pr_err("[%s], NONE hisi_mdc_fstcma mdc_virt_addr!\n", __func__);
        return 0;
    }
    return hisi_mdc_fstcma->mdc_virt_addr;
#else
    struct hisi_isp_ion_s *hisi_mdc_ion = NULL;

    if(dev->mdc_mem_info == NULL)
    {
        pr_err("[%s], NONE mdc_mem_info!\n", __func__);
        return NULL;
    }

    hisi_mdc_ion = dev->mdc_mem_info;/*lint !e838 */
    if(hisi_mdc_ion->virt_addr == NULL)
    {
        pr_err("[%s], NONE hisi_mdc_ion virt_addr!\n", __func__);
        return NULL;
    }
    return hisi_mdc_ion->virt_addr;
#endif
}

u64 get_mdc_addr_pa(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
#ifdef CONFIG_HISI_REMOTEPROC_FASTCMA_MDC_DEBUG
    struct hisi_isp_fstcma_mdc_s *hisi_mdc_fstcma = NULL;
    if(dev->mdc_fstcma == NULL)
    {
        pr_err("[%s], NONE mdc_mem_info!\n", __func__);
        return 0;
    }
    pr_debug("get_mdc_addr_pa FASTCMA_MDC_DEBUG!\n");
    hisi_mdc_fstcma = dev->mdc_fstcma;/*lint !e838 */
    if(hisi_mdc_fstcma->mdc_dma_addr == 0x0)
    {
        pr_err("[%s], NONE hisi_mdc_fstcma mdc_dma_addr!\n", __func__);
        return 0;
    }
    return hisi_mdc_fstcma->mdc_dma_addr;
#else
    struct hisi_isp_ion_s *hisi_mdc_ion = NULL;

    if(dev->mdc_mem_info == NULL)
    {
        pr_err("[%s], NONE mdc_mem_info!\n", __func__);
        return 0;
    }

    hisi_mdc_ion = dev->mdc_mem_info;/*lint !e838 */
    if(hisi_mdc_ion->paddr == 0x0)
    {
        pr_err("[%s], NONE hisi_mdc_ion virt_addr!\n", __func__);
        return 0;
    }
    return hisi_mdc_ion->paddr;
#endif
}

void set_shared_mdc_pa_addr(u64 mdc_pa_addr)
{
    struct rproc_shared_para *share_para = NULL;

    hisp_lock_sharedbuf();
    share_para = rproc_get_share_para();
    if (!share_para) {
        pr_err("[%s] Failed : share_para.%pK\n", __func__, share_para);
        hisp_unlock_sharedbuf();
        return;
    }

    share_para->mdc_pa_addr = mdc_pa_addr;
    pr_debug("[%s] share_para->mdc_pa_addr.0x%llx\n", __func__,share_para->mdc_pa_addr);
    hisp_unlock_sharedbuf();
}
/*lint -save -e429*/

static int mdc_addr_pa_init(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
#ifdef CONFIG_HISI_REMOTEPROC_FASTCMA_MDC_DEBUG
    pr_debug("mdc_addr_pa_init FASTCMA_MDC_DEBUG!\n");
    dev->mdc_fstcma = get_fstcma_mdc(MEM_MDC_SIZE);
    if(dev->mdc_fstcma == NULL){
        pr_err("get_fstcma_mdc failed!\n");
        return -EINVAL;
    }
#else
    size_t align = 12;/*lint !e838 */
    unsigned int heap_id_mask = (0x1 << 14);/*lint !e838 */
    unsigned int flags = (0x1 << 4);/*lint !e838 */
    size_t size = MEM_MDC_SIZE;

    dev->mdc_mem_info = get_nesc_addr_ion(size,align,heap_id_mask,flags);
    if(dev->mdc_mem_info == NULL){
        pr_err("get_mdc_addr_pa failed!\n");
        return -EINVAL;
    }
#endif
    return 0;
}


void free_nesc_addr_ion(struct hisi_isp_ion_s *hisi_nescaddr_ion)
{
    if(hisi_nescaddr_ion == NULL){
        return;
    }
    if(hisi_nescaddr_ion->ion_client == NULL || hisi_nescaddr_ion->ion_handle == NULL){
        pr_err("in %s ion_client & ion_handle is NULL!\n", __func__);
        return;
    }
    ion_unmap_kernel(hisi_nescaddr_ion->ion_client, hisi_nescaddr_ion->ion_handle);
    ion_free(hisi_nescaddr_ion->ion_client, hisi_nescaddr_ion->ion_handle);
    ion_client_destroy(hisi_nescaddr_ion->ion_client);
    hisi_nescaddr_ion->ion_client = NULL;
    hisi_nescaddr_ion->ion_handle = NULL;
    hisi_nescaddr_ion->virt_addr  = NULL;
}

struct hisi_isp_ion_s *get_nesc_addr_ion(size_t size, size_t align, unsigned int heap_id_mask, unsigned int flags)
{
    struct hisi_isp_ion_s *hisi_nescaddr_ion = NULL;
    ion_phys_addr_t nesc_addr_ion_phys = 0x0 ;
    size_t nesc_addr_ion_size = 0;
    int ret = 0;

    hisi_nescaddr_ion = kzalloc(sizeof(struct hisi_isp_ion_s), GFP_KERNEL);/*lint !e838 */
    if (!hisi_nescaddr_ion) {
        pr_err("in %s hisi_nescaddr_ion kzalloc is failed\n", __func__);
        return NULL;
    }

    hisi_nescaddr_ion->ion_client = hisi_ion_client_create("nescaddr_ion_client");
    if (IS_ERR(hisi_nescaddr_ion->ion_client)) {
        pr_err("hisi hisi_nescaddr_ion create failed \n");
        kfree(hisi_nescaddr_ion);
        return NULL;
    }

    hisi_nescaddr_ion->ion_handle = ion_alloc(hisi_nescaddr_ion->ion_client, size, align, heap_id_mask, flags);
    if (IS_ERR(hisi_nescaddr_ion->ion_handle)){
        pr_err("hisi ion_alloc failed \n");
        ion_client_destroy(hisi_nescaddr_ion->ion_client);
        kfree(hisi_nescaddr_ion);
        return NULL;
    }
    hisi_nescaddr_ion->virt_addr = ion_map_kernel(hisi_nescaddr_ion->ion_client, hisi_nescaddr_ion->ion_handle);
    if(hisi_nescaddr_ion->virt_addr == NULL)
    {
        pr_err("hisi ion_map_kernel failed \n");
        ion_free(hisi_nescaddr_ion->ion_client, hisi_nescaddr_ion->ion_handle);
        ion_client_destroy(hisi_nescaddr_ion->ion_client);
        kfree(hisi_nescaddr_ion);
        return NULL;
    }

    ret = ion_phys(hisi_nescaddr_ion->ion_client, hisi_nescaddr_ion->ion_handle,&nesc_addr_ion_phys, &nesc_addr_ion_size);/*lint !e838 */
    if (ret < 0)
    {
        pr_err("%s, failed to get phy addr,ret:%d!\n", __func__, ret);
        if((hisi_nescaddr_ion->ion_client != NULL) && (hisi_nescaddr_ion->ion_handle != NULL)){
        ion_unmap_kernel(hisi_nescaddr_ion->ion_client, hisi_nescaddr_ion->ion_handle);
        ion_free(hisi_nescaddr_ion->ion_client, hisi_nescaddr_ion->ion_handle);
        ion_client_destroy(hisi_nescaddr_ion->ion_client);
        kfree(hisi_nescaddr_ion);
        return NULL;
        }
        else{
            pr_err("%s, ion_client & ion_handle is NULL!\n", __func__);
        }
    }
    if(nesc_addr_ion_phys)
        hisi_nescaddr_ion->paddr = nesc_addr_ion_phys;

    return hisi_nescaddr_ion;
}
/*lint -restore */

/* rproc private da to va */
void *hisp_rproc_da_to_va(struct rproc *rproc, u64 da, int len)
{
	struct rproc_mem_entry *dynamic_mem, *reserved_mem;
	void *ptr = NULL;

	list_for_each_entry(reserved_mem, &rproc->reserved_mems, node) {
		int offset = da - reserved_mem->da;

		/* try next if da is too small */
		if (offset < 0)
			continue;

		/* try next if da is too large */
		if (offset + len > reserved_mem->len)
			continue;

		ptr = reserved_mem->va + offset;

		return ptr;
	}

	list_for_each_entry(dynamic_mem, &rproc->dynamic_mems, node) {
		int offset = da - dynamic_mem->da;

		/* try next if da is too small */
		if (offset < 0)
			continue;

		/* try next if da is too large */
		if (offset + len > dynamic_mem->len)
			continue;

		ptr = dynamic_mem->va + offset;

		return ptr;
	}

    return ptr;
}

/**
 * rproc_handle_version() - handle the verison information
 * @rproc: the remote processor
 * @rsc: the trace resource descriptor
 * @avail: size of available data (for sanity checking the image)
 */
int rproc_handle_version(struct rproc *rproc, struct fw_rsc_version *rsc,
                                int offset, int avail)
{
	printk("[%s] Firmware_version: magic = %x, module = %s, version = %s, build_time = %s, reserved = %s.\n", __func__,
							rsc->magic, rsc->module, rsc->version, rsc->build_time,
							rsc->reserved != 0?rsc->reserved:"NULL");

	return 0;
}

/**
 * rproc_handle_cda() - handle a shared trace buffer resource
 * @rproc: the remote processor
 * @rsc: the cda resource descriptor
 * @avail: size of available data (for sanity checking the image)
 *
 * In case the remote processor dumps cda bin into memory,
 * export it via debugfs.
 *
 * Currently, the 'da' member of @rsc should contain the device address
 * where the remote processor is dumping the cdas. Later we could also
 * support dynamically allocating this address using the generic
 * DMA API (but currently there isn't a use case for that).
 *
 * Returns 0 on success, or an appropriate error code otherwise
 */
 /*lint -save -e429 */
int rproc_handle_cda(struct rproc *rproc, struct fw_rsc_cda *rsc,
							int offset, int avail)
{
	struct rproc_mem_entry *cda;
	struct device *dev = &rproc->dev;
	void *ptr;
	char name[15];

	if (sizeof(*rsc) > avail) {/*lint !e574 */
		dev_err(dev, "cda rsc is truncated\n");
		return -EINVAL;
	}

	/* make sure reserved bytes are zeroes */
	if (rsc->reserved) {
		dev_err(dev, "trace rsc has non zero reserved bytes\n");
		return -EINVAL;
	}

	/* what's the kernel address of this resource ? */
	ptr = rproc_da_to_va(rproc, rsc->da, rsc->len);
	if (!ptr) {
		dev_err(dev, "erroneous cda resource entry\n");
		return -EINVAL;
	}

	cda = kzalloc(sizeof(*cda), GFP_KERNEL);
	if (!cda) {
		dev_err(dev, "kzalloc trace failed\n");
		return -ENOMEM;
	}

	/* set the trace buffer dma properties */
	cda->len = rsc->len;
	cda->va = ptr;

	/* make sure snprintf always null terminates, even if truncating */
	snprintf(name, sizeof(name), "cda%d", rproc->num_cdas);

	/* create the debugfs entry */
	cda->priv = rproc_create_cda_file(name, rproc, cda);
	if (!cda->priv) {
		cda->va = NULL;
		kfree(cda);
		return -EINVAL;
	}

	list_add_tail(&cda->node, &rproc->cdas);

	rproc->num_cdas++;

	dev_dbg(dev, "%s added: va %pK, da 0x%x, len 0x%x\n", name, ptr,
						rsc->da, rsc->len);

	return 0;
}
/*lint -restore */
void rproc_memory_cache_flush(struct rproc *rproc)
{
	struct rproc_cache_entry *tmp;

	list_for_each_entry(tmp, &rproc->caches, node)
		__flush_dcache_area(tmp->va, tmp->len);
}

/* package vaddr to physical addr(sgl) */
/*lint -save -e429*/
int vaddr_to_sgl(struct rproc *rproc, void **vaddr,
                        unsigned int length, struct sg_table **table)
{
	int ret;
	char *vaddr_tmp = (char *)vaddr;
	unsigned int len = 0;
	struct scatterlist *sg;
	struct page **pages, **tmp;
	struct rproc_page *r_page;
	int npages = 0, alloced = 0;

	npages = PAGE_ALIGN(length) / PAGE_SIZE;
	if (0 == npages)
		return -EINVAL;

	//pr_info("%s: length = 0x%x, npages = %d.\n", __func__, length, npages);

	r_page = kzalloc(sizeof(struct rproc_page), GFP_KERNEL);
	if (!r_page) {
		pr_err("%s: kzalloc failed. \n", __func__);
		return -ENOMEM;
	}

	pages = vmalloc(sizeof(struct page *) * npages);
	if (!pages) {
		pr_err("%s: vmalloc failed. \n", __func__);
		goto vmalloc_fail;
	}

	tmp = pages;

	*table = kzalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!(*table)) {
		pr_err("%s: vmalloc failed. \n", __func__);
		goto kmalloc_fail;
	}

	ret = sg_alloc_table(*table, npages, GFP_KERNEL);
	if (ret) {
		pr_err("%s:sg_alloc_table failed. \n", __func__);
		goto sg_fail;
	}

	while (alloced < npages) {
		*(tmp++) = alloc_page(GFP_KERNEL | GFP_DMA | __GFP_ZERO);
		alloced++;
	}

	*vaddr = vmap(pages, npages, VM_MAP, PAGE_KERNEL);
	if (!(*vaddr)) {
		pr_err("%s: vmap failed.\n", __func__);
		goto vmap_fail;
	}

	sg = (*table)->sgl;
	tmp = pages;
	alloced = 0;
	while (len < length && alloced < npages) {
		struct page *page = *(tmp++);
		sg_set_page(sg, page, PAGE_SIZE, 0);
		sg = sg_next(sg);

		len += PAGE_SIZE;
		vaddr_tmp += PAGE_SIZE;
	}

	r_page->va = (void *)pages;
	r_page->num = npages;
	list_add_tail(&r_page->node, &rproc->pages);
	pr_info("%s:len = 0x%x, length = 0x%x \n", __func__, len, length);
	return 0;

vmap_fail:
	alloced = 0;
	tmp = pages;
	while (alloced < npages) {
		__free_page(*(tmp++));
		alloced++;
	}
sg_fail:
	kfree(*table);
kmalloc_fail:
	vfree(pages);
vmalloc_fail:
	kfree(r_page);

	return -ENOMEM;
}
/*lint -restore */
/**
* rproc_handle_dynamic_memory() - handle phys non-contiguous memory allocation requests
* @rproc: rproc handle
* @rsc: the resource entry
* @avail: size of available data (for image validation)
*
* This function will handle firmware requests for allocation of physically
* contiguous memory regions.
*
* These request entries should come first in the firmware's resource table,
* as other firmware entries might request placing other data objects inside
* these memory regions (e.g. data/code segments, trace resource entries, ...).
*
* Allocating memory this way helps utilizing the reserved physical memory
* (e.g. CMA) more efficiently, and also minimizes the number of TLB entries
* needed to map it (in case @rproc is using an IOMMU). Reducing the TLB
* pressure is important; it may have a substantial impact on performance.
*/
/*lint -save -e429*/
int rproc_handle_dynamic_memory(struct rproc *rproc,
										struct fw_rsc_dynamic_memory *rsc,
										int offset, int avail)
{
	struct rproc_mem_entry *dynamic_mem, *mapping;
	struct rproc_cache_entry *cache_entry;
	struct device *dev = &rproc->dev;
	struct sg_table *table;
	void *va;
	int ret = -1;

	if (sizeof(*rsc) > avail) {/*lint !e574 */
		dev_err(dev, "dynamic_mem rsc is truncated\n");
		return -EINVAL;
	}

	/* make sure reserved bytes are zeroes */
	if (rsc->reserved) {
		dev_err(dev, "dynamic_mem rsc has non zero reserved bytes\n");
		return -EINVAL;
	}

	dev_dbg(dev, "dynamic_mem rsc: da %x, pa %x, len %x, flags %x\n",
	rsc->da, rsc->pa, rsc->len, rsc->flags);

	cache_entry = kzalloc(sizeof(*cache_entry), GFP_KERNEL);
	if (!cache_entry) {
		dev_err(dev, "kzalloc cache_entry failed\n");
		return -ENOMEM;
	}

	dynamic_mem = kzalloc(sizeof(*dynamic_mem), GFP_KERNEL);
	if (!dynamic_mem) {
		dev_err(dev, "kzalloc dynamic_mem failed\n");
		ret = -ENOMEM;
		goto free_cache;
	}
#if 0
	va = vmalloc(rsc->len);
	if (!va) {
		dev_err(dev, "%s:vmalloc failed\n", __func__);
		goto free_memory;
	}
#endif
	ret = vaddr_to_sgl(rproc, &va, rsc->len, &table);
	if (0 != ret) {
		dev_err(dev, "%s:vaddr_to_sgl failed\n", __func__);
		goto free_memory;
	}

	/* mapping */
	if (rproc->domain) {
		mapping = kzalloc(sizeof(*mapping), GFP_KERNEL);
		if (!mapping) {
			dev_err(dev, "kzalloc mapping failed\n");
			ret = -ENOMEM;
			goto free_sgl;
		}
		ret = iommu_map_sg(rproc->domain, rsc->da, table->sgl, sg_nents(table->sgl), rsc->flags);
		if (ret != sg_nents(table->sgl)*PAGE_SIZE) {
			dev_err(dev, "hisi_iommu_map_range failed: ret %d len %d\n", ret, rsc->len);
			goto free_mapping;
		}

		/*
		* We'll need this info later when we'll want to unmap
		* everything (e.g. on shutdown).
		*
		* We can't trust the remote processor not to change the
		* resource table, so we must maintain this info independently.
		*/
		mapping->da = rsc->da;
		mapping->len = rsc->len;
		list_add_tail(&mapping->node, &rproc->mappings);
	}

    /* free table */
    sg_free_table(table);
    kfree(table);

	/*
	* Some remote processors might need to know the pa
	* even though they are behind an IOMMU. E.g., OMAP4's
	* remote M3 processor needs this so it can control
	* on-chip hardware accelerators that are not behind
	* the IOMMU, and therefor must know the pa.
	*
	* Generally we don't want to expose physical addresses
	* if we don't have to (remote processors are generally
	* _not_ trusted), so we might want to do this only for
	* remote processor that _must_ have this (e.g. OMAP4's
	* dual M3 subsystem).
	*
	* Non-IOMMU processors might also want to have this info.
	* In this case, the device address and the physical address
	* are the same.
	*/

	dynamic_mem->va = va;
	dynamic_mem->len = rsc->len;
	dynamic_mem->dma = rsc->da;
	dynamic_mem->da = rsc->da;
	dynamic_mem->priv = rsc->name;

	list_add_tail(&dynamic_mem->node, &rproc->dynamic_mems);

	/* save cache entry */
	cache_entry->va = va;
	cache_entry->len = rsc->len;
	list_add_tail(&cache_entry->node, &rproc->caches);

	return 0;

free_mapping:
	kfree(mapping);
free_sgl:
	sg_free_table(table);
	kfree(table);
free_memory:
	kfree(dynamic_mem);
free_cache:
	kfree(cache_entry);
	return ret;
}
/*lint -restore */
/* rproc_handle_reserved_memory() - handle phys reserved memory allocation requests */
/*lint -save -e429*/
int rproc_handle_reserved_memory(struct rproc *rproc,
											struct fw_rsc_reserved_memory *rsc,
											int offset, int avail)
{
	struct rproc_mem_entry *reserved_mem, *mapping;
	struct rproc_cache_entry *cache_entry;
	struct device *dev = &rproc->dev;
	u64 addr;
	void *va = NULL;
	int ret = -1;

    if (sizeof(*rsc) > avail) {/*lint !e574 */
        dev_err(dev, "memory rsc is truncated\n");
        return -EINVAL;
    }

    /* make sure reserved bytes are zeroes */
    if (rsc->reserved) {
        dev_err(dev, "memory rsc has non zero reserved bytes\n");
        return -EINVAL;
    }

    if (!strncmp(rsc->name, "ISP_MEM_BOOTWARE", strlen(rsc->name)))/*lint !e64 */
        addr = get_a7remap_addr();
    else if (!strncmp(rsc->name, "ISP_MDH_MEM", strlen(rsc->name))){/*lint !e64 */
        addr = get_mdc_addr_pa();/*lint !e747 */
        if (!addr)
        {
            dev_err(dev, "get_mdc_addr_pa err! \n");
            return -EINVAL;
        }
        set_shared_mdc_pa_addr(addr);
    }
    else
        addr = rsc->pa;
    dev_err(dev, "%s : da.0x%x, pa.0x%llx, len.0x%x, flags.0x%x\n",
                rsc->name, rsc->da, addr, rsc->len, rsc->flags);

	cache_entry = kzalloc(sizeof(*cache_entry), GFP_KERNEL);
	if (!cache_entry) {
		dev_err(dev, "kzalloc cache_entry failed\n");
		return -ENOMEM;
	}

	reserved_mem = kzalloc(sizeof(*reserved_mem), GFP_KERNEL);
	if (!reserved_mem) {
		dev_err(dev, "kzalloc reserved_mem failed\n");
		ret = -ENOMEM;
		goto free_cache;
	}

    pr_info("%s: ioremap before.\n", __func__);
    if (use_nonsec_isp()) {
        if (!strncmp(rsc->name, "ISP_MEM_BOOTWARE", strlen(rsc->name))) {/*lint !e64 */
            va = get_a7remap_va();
            pr_info("%s: nsec boot\n", __func__);
        }
        else if(!strncmp(rsc->name, "ISP_MDH_MEM", strlen(rsc->name))){/*lint !e64 */
            va = get_mdc_addr_va();
            if(!va)
            {
                dev_err(dev, "get_mdc_addr_va NULL! \n");
                goto free_memory;
            }
            pr_debug("%s: ISP_MDH_MEM va.0x%pK\n", __func__,va);
        }
    } else {
        va = ioremap(addr, rsc->len);/*lint !e747 */
        if (!va) {
            dev_err(dev, "ioremap failed addr.0x%llx\n", addr);
            ret = -ENOMEM;
            goto free_memory;
        }
    }

    pr_info("%s: addr.0x%llx, len.0x%x, kva.%pK, name.%s\n",
            __func__, addr, rsc->len, va, rsc->name);

	if (rproc->domain) {
		mapping = kzalloc(sizeof(*mapping), GFP_KERNEL);
		if (!mapping) {
			dev_err(dev, "kzalloc mapping failed\n");
			ret = -ENOMEM;
			goto free_iomem;
		}

		ret = iommu_map(rproc->domain, rsc->da, addr, rsc->len, rsc->flags);
		if (ret) {
			dev_err(dev, "iommu_map failed: %d\n", ret);
			goto free_mapping;
		}

		mapping->da = rsc->da;
		mapping->len = rsc->len;
		list_add_tail(&mapping->node, &rproc->mappings);
	}

	reserved_mem->va = va;
	reserved_mem->len = rsc->len;
	reserved_mem->dma = rsc->da;
	reserved_mem->da = rsc->da;
	reserved_mem->priv = rsc->name;

	list_add_tail(&reserved_mem->node, &rproc->reserved_mems);

	/* save cache entry */
	cache_entry->va = va;
	cache_entry->len = rsc->len;
	list_add_tail(&cache_entry->node, &rproc->caches);

	return 0;

free_mapping:
	kfree(mapping);
free_iomem:
if (use_nonsec_isp()) {
        if (!strncmp(rsc->name, "ISP_MEM_BOOTWARE", strlen(rsc->name)))/*lint !e64 */
            ;
    } else /*lint !e548 */
        iounmap(va);
free_memory:
	kfree(reserved_mem);
free_cache:
	kfree(cache_entry);
	return ret;
}
/*lint -restore */
/* rproc_handle_rdr_memory() - handle phys rdr memory allocation requests */
/*lint -save -e429*/
int rproc_handle_rdr_memory(struct rproc *rproc,
									struct fw_rsc_carveout *rsc,
									int offset, int avail)
{
	struct rproc_mem_entry *mapping;
	struct device *dev = &rproc->dev;
	int ret = -1;
	u64 isprdr_addr;

	isprdr_addr = get_isprdr_addr();
	pr_info("ispRDR Paddr.0x%llx ==>> (DA.0x%x, PA.0x%x, Len.0x%x, Flag.0x%x).%s\n",
					isprdr_addr, rsc->da, rsc->pa, rsc->len, rsc->flags, rsc->name);

	if (0 == isprdr_addr) {
		pr_info("%s: rdr func is off.\n", __func__);
		return 0;
	}

	if (sizeof(*rsc) > avail) {/*lint !e574 */
		dev_err(dev, "memory rsc is truncated\n");
		return -EINVAL;
	}

	/* make sure reserved bytes are zeroes */
	if (rsc->reserved) {
		dev_err(dev, "memory rsc has non zero reserved bytes\n");
		return -EINVAL;
	}

	if (rproc->domain) {
		mapping = kzalloc(sizeof(*mapping), GFP_KERNEL);
		if (!mapping) {
			dev_err(dev, "kzalloc mapping failed\n");
			return -ENOMEM;
		}

		ret = iommu_map(rproc->domain, rsc->da, isprdr_addr, rsc->len, rsc->flags);
		if (ret) {
			dev_err(dev, "iommu_map failed: %d\n", ret);
			goto free_mapping;
		}

		mapping->da = rsc->da;
		mapping->len = rsc->len;
		list_add_tail(&mapping->node, &rproc->mappings);
	}

	return 0;

free_mapping:
	kfree(mapping);

	return ret;
}
/*lint -restore */
/* rproc_handle_shared_memory() - handle phys shared parameters memory allocation requests */
/*lint -save -e429*/
int rproc_handle_shared_memory(struct rproc *rproc,
										struct fw_rsc_carveout *rsc,
										int offset, int avail)
{
	struct rproc_mem_entry *reserved_mem, *mapping;
	struct device *dev = &rproc->dev;
	u64 a7sharedmem_addr;
	void *va;
	int ret = -1;

	pr_info("%s: entern.\n", __func__);
	if (sizeof(*rsc) > avail) {/*lint !e574 */
		dev_err(dev, "memory rsc is truncated\n");
		return -EINVAL;
	}

	/* make sure reserved bytes are zeroes */
	if (rsc->reserved) {
		dev_err(dev, "memory rsc has non zero reserved bytes\n");
		return -EINVAL;
	}

	a7sharedmem_addr = get_a7sharedmem_addr();
	dev_err(dev, "%s: da %x, pa %llx, len %x, flags %x\n",
					rsc->name, rsc->da, a7sharedmem_addr, rsc->len, rsc->flags);

	reserved_mem = kzalloc(sizeof(*reserved_mem), GFP_KERNEL);
	if (!reserved_mem) {
		dev_err(dev, "kzalloc reserved_mem failed\n");
		return -ENOMEM;
	}

    if (use_nonsec_isp())
        va = get_a7sharedmem_va();
    else {
        va = ioremap_wc(a7sharedmem_addr, rsc->len);
        if (!va) {
            dev_err(dev, "ioremap failed a7sharedmem_addr.0x%llx\n", a7sharedmem_addr);
            ret = -ENOMEM;
            goto free_memory;
        }
    }
    pr_info("[%s] va.%pK\n", __func__, va);

	if (rproc->domain) {
		mapping = kzalloc(sizeof(*mapping), GFP_KERNEL);
		if (!mapping) {
			dev_err(dev, "kzalloc mapping failed\n");
			ret = -ENOMEM;
			goto free_iomem;
		}

		ret = iommu_map(rproc->domain, rsc->da, a7sharedmem_addr, rsc->len, rsc->flags);
		if (ret) {
			dev_err(dev, "iommu_map failed: %d\n", ret);
			goto free_mapping;
		}

		mapping->da = rsc->da;
		mapping->len = rsc->len;
		list_add_tail(&mapping->node, &rproc->mappings);
	}

	reserved_mem->va = va;
	reserved_mem->len = rsc->len;
	reserved_mem->dma = rsc->da;
	reserved_mem->da = rsc->da;
	reserved_mem->priv = rsc->name;

	list_add_tail(&reserved_mem->node, &rproc->reserved_mems);
    pr_info("[%s] name.%s, (0x%x, 0x%x) -> %pK\n", __func__, rsc->name, rsc->da, rsc->len, va);

    hisp_lock_sharedbuf();
	isp_share_para = (struct rproc_shared_para *)va;
	memset(isp_share_para, 0, rsc->len);
    pr_info("[%s] isp_share_para.%pK, va.%pK\n", __func__, isp_share_para, va);
	hisp_unlock_sharedbuf();

    isploglevel_update();
    ispmonitor_update();
    ispcoresight_update();

    pr_info("[%s] -\n", __func__);

	return 0;

free_mapping:
	kfree(mapping);
free_iomem:
    if (use_nonsec_isp())
        ;
    else
        iounmap(va);
free_memory:
	kfree(reserved_mem);

	return ret;
}
/*lint -restore */

static int get_a7log_mode(void)
{
    struct device_node *np = NULL;
    int ret = 0, a7log_mode = 0;

    np = of_find_compatible_node(NULL, NULL, "hisilicon,prktimer");
    if (!np) {
        printk("NOT FOUND device node 'hisilicon,prktimer'!\n");
        return -ENXIO;
    }

    ret = of_property_read_u32(np, "fpga_flag", &a7log_mode);/*lint !e64*/
    if (ret) {
        printk("failed to get fpga_flag resource.\n");
        return -ENXIO;
    }

    return a7log_mode;
}
u32 get_share_exc_flag(void)
{
    struct rproc_shared_para *share_para = NULL;
    u32 exc_flag = 0x0;

    hisp_lock_sharedbuf();
    share_para = rproc_get_share_para();
    if (!share_para) {
        RPROC_ERR("Failed : rproc_get_share_para.%pK\n", share_para);
        hisp_unlock_sharedbuf();
        return 0xFFFFFFFF;
    }
    exc_flag = share_para->exc_flag;
    hisp_unlock_sharedbuf();
    return exc_flag;
}

int set_plat_parameters(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    struct rproc_shared_para *param = NULL;

    hisp_lock_sharedbuf();
    param = rproc_get_share_para();
    if (!param) {
        RPROC_ERR("Failed : rproc_get_share_para.%pK\n", param);
        hisp_unlock_sharedbuf();
        return -EINVAL;
    }

    param->plat_cfg.perf_power      = perf_para;
    param->plat_cfg.platform_id     = hw_is_fpga_board();
    param->plat_cfg.isp_local_timer = dev->tmp_plat_cfg.isp_local_timer;
    param->rdr_enable_type         |= ISPCPU_RDR_USE_APCTRL; /*lint !e648*/
    param->logx_switch             |= ISPCPU_LOG_USE_APCTRL;
    param->exc_flag                 = 0x0;
    if (get_a7log_mode())
        param->logx_switch         |= ISPCPU_LOG_TIMESTAMP_FPGAMOD;

    param->bootware_paddr           = get_a7remap_addr();
    RPROC_INFO("platform_id = %d, isp_local_timer = %d, perf_power = %d, logx_switch.0x%x, bootware_addr.0x%llx\n",
        param->plat_cfg.platform_id, param->plat_cfg.isp_local_timer,
        param->plat_cfg.perf_power, param->logx_switch, param->bootware_paddr);
    hisp_unlock_sharedbuf();

    isploglevel_update();
    ispmonitor_update();
    ispcoresight_update();

    return 0;
}

int rproc_set_shared_para(void)
{
	struct rproc_shared_para *share_para = NULL;
	int ret, i;

	ret = set_plat_parameters();
	if (ret) {
		pr_err("%s: set_plat_parameters failed.\n", __func__);
		return ret;
	}

    hisp_lock_sharedbuf();
    share_para = rproc_get_share_para();
	if (!share_para) {
		pr_err("%s:rproc_get_share_para failed.\n", __func__);
		hisp_unlock_sharedbuf();
		return -EINVAL;
	}

    share_para->bootware_paddr = get_a7remap_addr();
    pr_info("%s: bootware_addr.0x%llx.\n", __func__, share_para->bootware_paddr);

	if (get_isprdr_addr())
		share_para->rdr_enable = 1;

	share_para->rdr_enable_type |= RDR_CHOOSE_TYPE;
	for (i = 0; i < IRQ_NUM; i++)
		share_para->irq[i] = 0;

	pr_info("%s: platform_id = 0x%x, timer = 0x%x, rdr_enable = %d, rdr_enable_type = %d\n",
		__func__, share_para->plat_cfg.platform_id,
		share_para->plat_cfg.isp_local_timer, share_para->rdr_enable,
		share_para->rdr_enable_type);
    hisp_unlock_sharedbuf();

	return ret;
}

int rproc_bootware_attach(struct rproc *rproc, const char *bootware)
{
	rproc->bootware = bootware;
	return 0;
}

int use_nonsec_isp(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    return ((dev->case_type == NONSEC_CASE) ? 1 : 0);
}

int use_sec_isp(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    return ((dev->case_type == SEC_CASE) ? 1 : 0);
}

int hisi_isp_rproc_case_set(enum hisi_isp_rproc_case_attr type)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;

    if (type >= INVAL_CASE) {
		pr_err("%s: invalid case, type = %u\n", __func__, type);
        return -EINVAL;
    }

	if (sync_isplogcat() < 0)
	    pr_err("[%s] Failed: sync_isplogcat\n", __func__);

    dev->case_type = type;
	if (type == SEC_CASE) {
		hisi_ispsec_share_para_set();
		if (likely(hisi_rproc)) /*lint !e730 */
			hisi_rproc->fw_ops = &rproc_bin_fw_ops;
	} else {
		if (likely(hisi_rproc)) /*lint !e730 */
			hisi_rproc->fw_ops = &rproc_elf_fw_ops;
	}

	pr_info("%s.%d: type.%u, rporc.%pK\n", __func__, __LINE__, type, hisi_rproc);

	pr_info("[%s] elf_ops.%pK, bin_ops.%pK, use_ops.%pK\n",
			__func__, (void *)&rproc_elf_fw_ops,
			(void *)&rproc_bin_fw_ops,
			hisi_rproc ? (void *)hisi_rproc->fw_ops : NULL);

    return 0;
}

enum hisi_isp_rproc_case_attr hisi_isp_rproc_case_get(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;

    return dev->case_type;
}

char *hisp_get_comp_name(void)
{
	return DTS_COMP_NAME;
}

int hisp_set_clk_rate(unsigned int type, unsigned int rate)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    int ret = -EINVAL;

    if (!check_dvfs_valid()) {
        pr_err("[%s] Failed : check_dvfs_valid\n", __func__);
        return -EINVAL;
    }

    switch (dev->case_type) {
        case NONSEC_CASE:
            if ((ret = nsec_setclkrate(type, rate)) < 0)
                pr_err("[%s] Failed : nsec_setclkrate.%d\n", __func__, ret);
            break;
        case SEC_CASE:
            if ((ret = sec_setclkrate(type, rate)) < 0)
                pr_err("[%s] Failed : sec_setclkrate.%d\n", __func__, ret);
            break;
        default:
            pr_err("[%s] Unsupported case_type.%d\n", __func__, dev->case_type);
            return -EINVAL;
    }

    return ret;
}

int hisi_isp_rproc_setpinctl(struct pinctrl *isp_pinctrl, struct pinctrl_state *pinctrl_def, struct pinctrl_state *pinctrl_idle)
{
	struct rproc_boot_device *rproc_dev = &rproc_boot_dev;

    if (hw_is_fpga_board()) {
        RPROC_ERR("this board is fpga board, don't need to init pinctrl.\n");
        return 0;
    }

	rproc_dev->isp_data.isp_pinctrl = isp_pinctrl;
	rproc_dev->isp_data.pinctrl_def = pinctrl_def;
	rproc_dev->isp_data.pinctrl_idle = pinctrl_idle;

	return 0;
}

int hisi_isp_rproc_setclkdepend(struct clk *aclk_dss, struct clk *pclk_dss)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;

	if (hw_is_fpga_board()) {
		RPROC_ERR("this board is fpga board, don't need to init clkdepend.\n");
		return 0;
	}

	rproc_dev->isp_data.aclk_dss = aclk_dss;
	rproc_dev->isp_data.pclk_dss = pclk_dss;

	return 0;
}

int hisi_isp_dependent_clock_enable(void)
{
	struct rproc_boot_device *dev = (struct rproc_boot_device *)&rproc_boot_dev;
    int rc = 0;
    if(dev->boardid > HI6250_BOARDID)
        return rc;
    RPROC_INFO("Enable dss aclk, pclk\n");

    rc = clk_prepare(dev->isp_data.aclk_dss);
    if (rc)
        RPROC_ERR("Failed : aclk_dss clk_prepare.%d\n", rc);

    rc = clk_enable(dev->isp_data.aclk_dss);
    if (rc)
        RPROC_ERR("Failed : aclk_dss clk_enable.%d\n", rc);

    rc = clk_prepare(dev->isp_data.pclk_dss);
    if (rc)
        RPROC_ERR("Failed : pclk_dss clk_prepare.%d", rc);

    rc = clk_enable(dev->isp_data.pclk_dss);
    if (rc)
        RPROC_ERR("Failed : pclk_dss clk_enable.%d\n", rc);

    return rc;
}

int hisi_isp_dependent_clock_disable(void)
{
    struct rproc_boot_device *dev = (struct rproc_boot_device *)&rproc_boot_dev;
    if(dev->boardid > HI6250_BOARDID)
        return 0;
    RPROC_INFO("Disable dss aclk, pclk.");

    clk_disable(dev->isp_data.pclk_dss);
    clk_unprepare(dev->isp_data.pclk_dss);

    clk_disable(dev->isp_data.aclk_dss);
    clk_unprepare(dev->isp_data.aclk_dss);

    return 0;
}

u64 get_a7remap_addr(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    if (use_nonsec_isp())
        return dev->nsec_remap_addr;
    else
        return dev->remap_addr;
}

u64 get_a7sharedmem_addr(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    if (use_nonsec_isp())
        return (dev->nsec_remap_addr + 0xF000);
    else
        return (dev->remap_addr + 0xF000);
}

void set_a7mem_pa(u64 addr)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    dev->nsec_remap_addr = addr;
}

void *get_a7remap_va(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    pr_info("%s: va = %pK\n", __func__, dev->remap_va);
    return dev->remap_va;
}

void *get_a7sharedmem_va(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    //pr_info("%s: va = %pK\n", __func__, dev->remap_va + 0xf000);
    return (dev->remap_va + 0xF000);
}

void set_a7mem_va(void *addr)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    dev->remap_va = addr;
    pr_info("%s: remap_va = %pK\n", __func__, dev->remap_va);
}

int _stat_poll(void __iomem *addr, unsigned int value, void __iomem *stat_addr, unsigned int cond)
{
    unsigned int stat;
    int          timeout;

    __raw_writel(value, (volatile void __iomem *)addr);

    /* polling timeout */
    timeout = TIMEOUT;
    stat = __raw_readl((volatile void __iomem *)stat_addr);
    while (((stat & value) != cond) && timeout >= 0) {
        usleep_range(100, 110);
        stat = __raw_readl((volatile void __iomem *)stat_addr);
        --timeout;
    }

    return ((timeout < 0) ? (stat) : (0));
}

int en_stat_poll(void __iomem *addr, unsigned int value)
{
	return _stat_poll(addr, value, addr + EN_SATA_OFFSET, value);
}

int en_rststat_poll(void __iomem *addr, unsigned int value)
{
	return _stat_poll(addr, value, addr + EN_RSTSATA_OFFSET, value);
}

int dis_rststat_poll(void __iomem *addr, unsigned int value)
{
	return _stat_poll(addr, value, addr + DIS_RSTSATA_OFFSET, 0);
}

int noc_rststat_poll(void __iomem *addr, unsigned int value)
{
	return _stat_poll(addr, value, addr + EN_RSTSATA_OFFSET, 0);
}

static void dump_bootware(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    void __iomem *bw_base;
    int i = 0;

    bw_base = (void __iomem *)ioremap(dev->remap_addr, SZ_8K);
    for(i = 0; i < 1024; i += 16)
        pr_alert("0x%08x 0x%08x 0x%08x 0x%08x\n", __raw_readl(bw_base + i), __raw_readl(bw_base + i + 4), __raw_readl(bw_base + i + 8), __raw_readl(bw_base + i + 12));
    iounmap(bw_base);
}

static void dump_ispa7_regs(void)
{
#define DUMP_A7PC_TIMES (3)
    struct rproc_boot_device *dev = &rproc_boot_dev;
    void __iomem *crg_base = dev->crgperi_base;
    void __iomem *cssys_base;
    void __iomem *isp_base;
    unsigned int sec_rststat;
    int i = 0;

    if ((sec_rststat = __raw_readl(crg_base + CRG_C88_PERIPHISP_SEC_RSTSTAT)) != 0)
        pr_alert("sec_rststat.0x%x = 0x%x\n", CRG_C88_PERIPHISP_SEC_RSTSTAT, sec_rststat);

    if ((sec_rststat & IP_RST_ISPA7) == 0) {
        if ((cssys_base = (void __iomem *)ioremap(REG_BASE_CSSYS_PHY, SZ_8K)) == NULL) {
            RPROC_ERR("Failed : ioremap cssys_base.%pK\n", cssys_base);
            return;
        }
        for (i = 0; i < DUMP_A7PC_TIMES; i ++)
        pr_alert("A7PC_OFFSET.0x%x, PMEVCNTR0_EL0.0x%x, PMCCNTR_EL0.0x%x\n",
            __raw_readl(cssys_base + A7PC_OFFSET), __raw_readl(cssys_base + PMEVCNTR0_EL0), __raw_readl(cssys_base + PMCCNTR_EL0));
        iounmap(cssys_base);
    }
    if ((sec_rststat & IP_RST_ISP) == 0) {
        if ((isp_base = (void __iomem *)ioremap(ISP_CORE_CFG_BASE_ADDR, SZ_2M)) == NULL) {
            RPROC_ERR("Failed : ioremap isp_base.%pK\n", isp_base);
            return ;
        }
        pr_alert("isp.dump.version.0x%x\n", __raw_readl(isp_base + REVISION_ID_OFFSET));
        iounmap(isp_base);
    }
}

static void dump_crg_regs(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    void __iomem *crg_base = dev->crgperi_base;

    pr_alert("DIV10.0x%x, DIV18.0x%x, DIV20.0x%x\n",
        __raw_readl(SOC_CRGPERIPH_CLKDIV10_ADDR(crg_base)),
        __raw_readl(SOC_CRGPERIPH_CLKDIV18_ADDR(crg_base)),
        __raw_readl(SOC_CRGPERIPH_CLKDIV20_ADDR(crg_base)));
    pr_alert("CLKEN0.0x%x, STAT0.0x%x, CLKEN3.0x%x, STAT3.0x%x, CLKEN5.0x%x, STAT5.0x%x\n",
        __raw_readl(SOC_CRGPERIPH_PERCLKEN0_ADDR(crg_base)),
        __raw_readl(SOC_CRGPERIPH_PERSTAT0_ADDR(crg_base)),
        __raw_readl(SOC_CRGPERIPH_PERCLKEN3_ADDR(crg_base)),
        __raw_readl(SOC_CRGPERIPH_PERSTAT3_ADDR(crg_base)),
        __raw_readl(SOC_CRGPERIPH_PERCLKEN5_ADDR(crg_base)),
        __raw_readl(SOC_CRGPERIPH_PERSTAT5_ADDR(crg_base)));
    pr_alert("RSTSTAT0.0x%x, RSTSTAT3.0x%x, RSTSTAT.0x%x, ISOSTAT.0x%x, PWRSTAT.0x%x, PWRACK.0x%x\n",
        __raw_readl(SOC_CRGPERIPH_PERRSTSTAT0_ADDR(crg_base)),
        __raw_readl(SOC_CRGPERIPH_PERRSTSTAT3_ADDR(crg_base)),
        __raw_readl((crg_base) + CRG_C88_PERIPHISP_SEC_RSTSTAT),
        __raw_readl(SOC_CRGPERIPH_ISOSTAT_ADDR(crg_base)),
        __raw_readl(SOC_CRGPERIPH_PERPWRSTAT_ADDR(crg_base)),
        __raw_readl(SOC_CRGPERIPH_PERPWRACK_ADDR(crg_base)));
}

static void dump_noc_regs(void)
{
    void __iomem *pmc_base;

    if ((pmc_base = (void __iomem *)ioremap(ISP_PMC_BASE_ADDR, SZ_4K)) == NULL) {
        RPROC_ERR("Failed : ioremap pmc_base.%pK\n", pmc_base);
        return;
    }

    pr_alert("NOC_POWER: IDLEACK.0x%x, IDLE.0x%x\n",
        __raw_readl(pmc_base + PMCTRL_384_NOC_POWER_IDLEACK), __raw_readl(pmc_base + PMCTRL_388_NOC_POWER_IDLE));
    iounmap(pmc_base);
}

static int dump_a7boot_stone(void)
{
    struct rproc_shared_para *param = NULL;
    int entry = 0;

    hisp_lock_sharedbuf();
    if ((param = rproc_get_share_para()) == NULL) {
        RPROC_ERR("Failed : rproc_get_share_para.%pK\n", param);
        hisp_unlock_sharedbuf();
        return -EINVAL;
    }

    RPROC_ERR("BOOTWARE::entry.%d, invalid_tlb.%d, enable_mmu.%d\n",
        param->bw_stat.entry, param->bw_stat.invalid_tlb, param->bw_stat.enable_mmu);
    RPROC_ERR("FIRMWARE::entry.%d, hard_boot_init.%d, hard_drv_init.%d, app_init.%d\n",
        param->fw_stat.entry, param->fw_stat.hard_boot_init,
        param->fw_stat.hard_drv_init, param->fw_stat.app_init);

    entry = param->bw_stat.entry;
    hisp_unlock_sharedbuf();

    return entry;
}

void hisi_isp_boot_stat_dump(void)
{
    pr_alert("[%s] +\n", __func__);
	dump_a7boot_stone();
    if (use_sec_isp()) {
        hisi_secisp_dump();
        return;
    }

    if (use_nonsec_isp())
        return;

    dump_ispa7_regs();
    dump_crg_regs();
   	dump_noc_regs();
	dump_bootware();

    pr_alert("[%s] -\n", __func__);
}

static int cpu_debug_init(void)
{
    void __iomem *cssys_base;

    if ((cssys_base = (void __iomem *)ioremap(REG_BASE_CSSYS_PHY, SZ_8K)) == NULL) {
        RPROC_ERR("Failed : ioremap cssys_base.%pK\n", cssys_base);
        return -1;
    }
    __raw_writel((unsigned int)0xC5ACCE55, (volatile void __iomem *)(cssys_base + DBGLAR));
    __raw_writel((unsigned int)0x00000000, (volatile void __iomem *)(cssys_base + DBGOSLAR));
    __raw_writel((unsigned int)0xC5ACCE55, (volatile void __iomem *)(cssys_base + PMLAR));
    __raw_writel((unsigned int)0x00000007, (volatile void __iomem *)(cssys_base + PMCR));
    __raw_writel((unsigned int)0x00000008, (volatile void __iomem *)(cssys_base + PMXEVTYPER0));
    __raw_writel((unsigned int)0x80000001, (volatile void __iomem *)(cssys_base + PMCNTENSET));
    iounmap(cssys_base);

    return 0;
}

static int dis_reset_a7(struct rproc_boot_device *dev)
{
    volatile unsigned int value, ret = 0;
    void __iomem * addr;

    /*
     * CRGPERI_A7_SEC_RSTDIS: ip_rst_isp = 1, ip_arst_isp = 1,
     * ip_hrst_isp = 1, ip_rst_ispa7cfg = 1, ip_rst_ispa7 = 1
     */
    value = 0xFF;
    addr = CRG_C84_PERIPHISP_SEC_RSTDIS + dev->crgperi_base;
    RPROC_INFO("CRGPERI_A7_SEC_RSTDIS : %pK = 0x%x\n", addr, value);
    if ((ret = dis_rststat_poll(addr, value)) != 0)
        RPROC_ERR("Failed : ISP_SEC_RSTDIS : %pK = 0x%x, ret.0x%x\n", addr, value, ret);

    if (cpu_debug_init() < 0)
        RPROC_ERR("Failed: cpu_debug_init");

    if (!dump_a7boot_stone())
        hisi_isp_boot_stat_dump();

    return ret;
}

static int power_up_isp_subsys(struct rproc_boot_device *dev)
{
    int ret;

    RPROC_INFO("Start Dis Reset ISP A7 clk.%d\n", dev->ispa7_clk_value);
    ret = clk_set_rate(dev->ispa7_clk, dev->ispa7_clk_value);
    if(ret < 0) {
        RPROC_ERR("Failed: clk_set_rate.%d\n", ret);
        return ret;
    }

    ret = clk_prepare_enable(dev->ispa7_clk);
    if(ret < 0) {
        RPROC_ERR("Failed: clk_prepare_enable.%d\n", ret);
        return ret;
    }

    if (IS_HI6250(dev->boardid)) {
        ret = clk_prepare(dev->isp_timer);
        if (ret) {
            clk_disable_unprepare(dev->ispa7_clk);
            RPROC_ERR("Failed : isp_timer clk_prepare boardid.0x%x, ret.%d\n", dev->boardid, ret);
            return ret;
        }

        ret = clk_enable(dev->isp_timer);
        if (ret) {
            clk_unprepare(dev->isp_timer);
            clk_disable_unprepare(dev->ispa7_clk);
            RPROC_ERR("Failed : isp_timer clk_enable boardid.0x%x, ret.%d\n", dev->boardid, ret);
            return ret;
        }
    }

    ret = regulator_enable(dev->isp_subsys_ip);
    if (0 != ret) {
        RPROC_ERR("Failed: regulator_enable.%d \n", ret);
        goto err;
    }

    dev->isp_subsys_power_flag = 1;
    RPROC_INFO("X...\n");
    return 0;

err:
    if (IS_HI6250(dev->boardid)) {
        clk_disable(dev->isp_timer);
        clk_unprepare(dev->isp_timer);
    }
    clk_disable_unprepare(dev->ispa7_clk);

    dev->isp_subsys_power_flag = 0;
    return ret;
}

static void remap_a7_entry(struct rproc_boot_device *dev)
{
    u64 addr = dev->remap_addr;

    RPROC_INFO("remap start.0x%llx\n", addr);

    __raw_writel((unsigned int)(addr >> 4) | ISPA7_VINITHI_HIGH | ISPA7_REMAP_ENABLE | ISPA7_DBGPWRDUP
            , CRG_C90_PERIPHISP_ISPA7_CTRL_BASE + dev->crgperi_base);
}

static void power_down_isp_subsys(struct rproc_boot_device *dev)
{
    int ret = 0;

    RPROC_INFO("Enter \n");
    ret = regulator_disable(dev->isp_subsys_ip);
    if (0 != ret) {
        RPROC_ERR("Failed: regulator_disable.%d\n", ret);
    }

    if (IS_HI6250(dev->boardid)) {
        clk_disable(dev->isp_timer);
        clk_unprepare(dev->isp_timer);
    }
    clk_disable_unprepare(dev->ispa7_clk);

    dev->isp_subsys_power_flag = 0;
}

static int default_isp_device_disable(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;

    RPROC_INFO("+\n");
    if (dev->isp_subsys_power_flag) {
        RPROC_INFO("isp_subsys_power_flag = %d \n", dev->isp_subsys_power_flag);
        power_down_isp_subsys(dev);
    }

    if (dev->crgperi_base != NULL) {
        iounmap(dev->crgperi_base);
        dev->crgperi_base = NULL;
    }

    RPROC_INFO("-\n");

    return 0;
}
static int isp_device_disable(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    int ret = 0;

    RPROC_INFO("+\n");
    switch (dev->case_type) {
        case SEC_CASE:
            if ((ret = get_ispcpu_cfg_info()) < 0)
                RPROC_ERR("Failed : get_ispcpu_cfg_info.%d\n", ret);
            wait_firmware_coredump();
            if ((ret = secisp_device_disable()) < 0)
                RPROC_ERR("Failed : secisp_device_disable.%d\n", ret);
            break;
        case NONSEC_CASE:
            if ((ret = get_ispcpu_cfg_info()) < 0)
                RPROC_ERR("Failed : get_ispcpu_cfg_info.%d\n", ret);
            wait_firmware_coredump();
            if ((ret = set_ispcpu_reset()) < 0)
                RPROC_ERR("Failed : set_ispcpu_reset.%d\n", ret);
            if ((ret = nonsec_isp_device_disable()) < 0)
                RPROC_ERR("Failed : nonsec_isp_device_disable.%d\n", ret);
            break;
        default:
            if ((ret = default_isp_device_disable()) < 0)
                RPROC_ERR("Failed : default_isp_device_disable.%d\n", ret);
            break;
    }
    dev->ispcpu_status = 0;
    stop_isplogcat();

    if (ret != 0)
        RPROC_ERR("Failed : ispcpu power down fail.%d, dev->case_type.%d\n", ret, dev->case_type);
    mutex_lock(&dev->ispcpu_mutex);
    if (wake_lock_active(&dev->ispcpu_wakelock)) {
        wake_unlock(&dev->ispcpu_wakelock);
        RPROC_INFO("ispcpu power up wake unlock.\n");
    }
    mutex_unlock(&dev->ispcpu_mutex);/*lint !e456 */
    RPROC_INFO("-\n");

    return ret;
}
static int default_isp_device_enable(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    int ret = -ENOMEM;

    RPROC_INFO("+\n");
    if (dev->crgperi_base == NULL) {
        dev->crgperi_base = (void __iomem *)ioremap(SOC_ACPU_PERI_CRG_BASE_ADDR, SZ_4K);
        if (dev->crgperi_base == NULL) {
            RPROC_ERR("crgperi_base err 0x%x(0x%x)\n", SOC_ACPU_PERI_CRG_BASE_ADDR, SZ_4K);
            goto err;
        }
    }

    remap_a7_entry(dev);
    ret = power_up_isp_subsys(dev);
    if (0 != ret) {
        RPROC_ERR("power_up_isp_subsys failed.%d\n", ret);
        goto err;
    }

    if ( (ret = isp_a7_qos_cfg()) < 0) {
        RPROC_ERR("isp_a7_qos_cfg failed.%d\n", ret);
        goto err;
    }

    ret = dis_reset_a7(dev);
    if (0 != ret) {
        RPROC_ERR("disreset_a7 failed. \n");
        goto err1;
    }
    RPROC_INFO("-\n");

    return 0;

err1:
    power_down_isp_subsys(dev);
err:
    if (dev->crgperi_base != NULL) {
        iounmap(dev->crgperi_base);
        dev->crgperi_base = NULL;
    }

    return (ret < 0 ? ret : -1);
}

/* enable rproc a7 and isp core*/
/*lint -save -e838  -e454*/
static int isp_device_enable(void)
{
#define TIMEOUT_ISPLOG_START (10)
    struct rproc_boot_device *dev = &rproc_boot_dev;
    int ret = -ENOMEM, timeout = 0;

    RPROC_INFO("+\n");
    mutex_lock(&dev->ispcpu_mutex);
    if (!wake_lock_active(&dev->ispcpu_wakelock)) {
        wake_lock(&dev->ispcpu_wakelock);
        RPROC_INFO("ispcpu power up wake lock.\n");
    }
    mutex_unlock(&dev->ispcpu_mutex);/*lint !e456 */
    timeout = TIMEOUT_ISPLOG_START;
    do {
        if ((ret = start_isplogcat()) < 0)
            RPROC_ERR("Failed : secisp_device_enable.%d, timeout.%d\n", ret, timeout);
    } while (ret < 0 && timeout-- > 0);

    switch (dev->case_type) {
        case SEC_CASE:
            if ((ret = secisp_device_enable()) < 0)
                RPROC_ERR("Failed : secisp_device_enable.%d\n", ret);
            break;
        case NONSEC_CASE:
            if ((ret = nonsec_isp_device_enable()) < 0)
                RPROC_ERR("Failed : nonsec_isp_device_enable.%d\n", ret);
            break;
        default:
            if ((ret = default_isp_device_enable()) < 0)
                RPROC_ERR("Failed : default_isp_device_enable.%d\n", ret);
            break;
    }
    dev->ispcpu_status = 1;
    RPROC_INFO("-\n");

    if (ret != 0){
        RPROC_ERR("Failed : ispcpu power up fail.%d, dev->case_type.%d\n", ret, dev->case_type);
        mutex_lock(&dev->ispcpu_mutex);
        if (wake_lock_active(&dev->ispcpu_wakelock)) {
            wake_unlock(&dev->ispcpu_wakelock);
            RPROC_INFO("ispcpu power up wake unlock.\n");
        }
        mutex_unlock(&dev->ispcpu_mutex);/*lint !e456 */
    }
    return ret;
}
/*lint -restore */
static void isp_mbox_rx_work(void)
{
    struct rx_buf_data mbox_reg;
    int ret;

    while (kfifo_len(&isp_rx_mbox->rpmsg_rx_fifo) >= sizeof(struct rx_buf_data)) {/*lint !e84 */
        if ((ret = kfifo_out_locked(&isp_rx_mbox->rpmsg_rx_fifo, (unsigned char *)&mbox_reg, \
                                    sizeof(struct rx_buf_data), &isp_rx_mbox->rpmsg_rx_lock)) < 0) {/*lint !e84 */
            RPROC_ERR("Failed : kfifo_out_locked.%d\n", ret);
            return ;
        }

        /* maybe here need the flag of is_have_data */
        if (rproc_vq_interrupt(hisi_rproc, mbox_reg.rpmsg_rx_buf[0]) == IRQ_NONE)
            RPROC_DEBUG("no message was found in vqid\n");
    }
}

static int isp_mbox_rx_thread(void *context)
{
    int ret = 0;

    RPROC_INFO("+\n");
    while (!kthread_should_stop()) {
		ret = wait_event_interruptible(isp_rx_mbox->wait, isp_rx_mbox->can_be_wakeup == 1);
		isp_rx_mbox->can_be_wakeup = 0;
		if (ret) {
			RPROC_ERR("isp_mbox_rx_thread wait event failed\n");
		    continue;
		}

        isp_mbox_rx_work();
    }
    RPROC_INFO("-\n");

    return 0 ;
}

int hisi_rproc_mbox_callback(struct notifier_block *this, unsigned long len, void *data)
{
    mbox_msg_t *msg = (mbox_msg_t *)data;
    struct rx_buf_data mbox_reg;
    struct rproc_boot_device *dev = &rproc_boot_dev;
    int ret;
    unsigned int i;
    spin_lock_bh(&dev->rpmsg_ready_spin_mutex);
    if(!dev->rpmsg_ready_state){
        RPROC_INFO("isp is powered off state\n");
        spin_unlock_bh(&dev->rpmsg_ready_spin_mutex);/*lint !e456 */
        return NOTIFY_DONE;
    }

    hisp_recvtask();
	switch (msg[0]) {
		case RP_MBOX_CRASH:/*lint !e650 */
			/*  just log this for now. later, we'll also do recovery */
			RPROC_INFO("hisi rproc crashed\n");

			break;
		case RP_MBOX_ECHO_REPLY:/*lint !e650 */
			RPROC_INFO("received echo reply \n");
			break;
		default:
			/*  msg contains the index of the triggered vring */
			RPROC_DEBUG("default.%d\n", msg[0]);

            mbox_reg.rpmsg_rx_len = MBOX_REG_COUNT;
            for (i = 0; i < mbox_reg.rpmsg_rx_len; i++) {
                mbox_reg.rpmsg_rx_buf[i] = msg[i];
            }

            if (kfifo_avail(&isp_rx_mbox->rpmsg_rx_fifo) < sizeof(struct rx_buf_data)) {/*lint !e84 */
                RPROC_ERR("rpmsg_rx_fifo is full \n");
                spin_unlock_bh(&dev->rpmsg_ready_spin_mutex);/*lint !e456 */
                return -1;
            }

            ret = kfifo_in_locked(&isp_rx_mbox->rpmsg_rx_fifo, (unsigned char *)&mbox_reg, \
                                  sizeof(struct rx_buf_data), &isp_rx_mbox->rpmsg_rx_lock);/*lint !e84 */
            if (ret <= 0) {
                RPROC_ERR("kfifo_in_locked failed \n");
                spin_unlock_bh(&dev->rpmsg_ready_spin_mutex);/*lint !e456 */
                return ret;
            }
            RPROC_DEBUG("kfifo_in_locked success !\n");

			isp_rx_mbox->can_be_wakeup = 1;
			wake_up_interruptible(&isp_rx_mbox->wait);
    }
    spin_unlock_bh(&dev->rpmsg_ready_spin_mutex);/*lint !e456 */
    RPROC_DEBUG("----hisi_rproc_mbox_callback rx msg X----\n");

    return NOTIFY_DONE;
}
EXPORT_SYMBOL(hisi_rproc_mbox_callback);

static int init_hisi_ipc_resource(void)
{
    int ret;

    RPROC_INFO("+\n");
    isp_rx_mbox = kzalloc(sizeof(struct isp_rx_mbox), GFP_KERNEL);
    if (!isp_rx_mbox) {
        RPROC_ERR("Failed : kzalloc isp_rx_mbox\n");
        return -ENOMEM;
    }

    init_waitqueue_head(&isp_rx_mbox->wait);
    isp_rx_mbox->can_be_wakeup = 0;
    isp_rx_mbox->rpmsg_rx_tsk = kthread_create(isp_mbox_rx_thread, NULL, "rpmsg_tx_tsk");

	if (unlikely(IS_ERR(isp_rx_mbox->rpmsg_rx_tsk))) {
		RPROC_ERR("Failed : create kthread tx_kthread\n");
		ret = -EINVAL;
		goto kthread_failure;
	} else {
		struct sched_param param;
		/*set the thread's priority to 75, the bigger sched_priority, the higher priority*/
		param.sched_priority = (MAX_RT_PRIO - 25);
		ret = sched_setscheduler(isp_rx_mbox->rpmsg_rx_tsk, SCHED_RR, &param);
		if (ret < 0) {
		    RPROC_ERR("Failed : sched_setscheduler\n");
		    goto kthread_failure;
		}
		wake_up_process(isp_rx_mbox->rpmsg_rx_tsk);
	}

    spin_lock_init(&isp_rx_mbox->rpmsg_rx_lock);

    if (kfifo_alloc(&isp_rx_mbox->rpmsg_rx_fifo, sizeof(struct rx_buf_data) * RPMSG_RX_FIFO_DEEP, GFP_KERNEL)) {
        RPROC_ERR("Failed : kfifo_alloc\n");
        ret = -ENOMEM;
        goto kfifo_failure;
    }

    RPROC_INFO("-\n");
    return 0;
kfifo_failure:
kthread_failure:
    kfree(isp_rx_mbox);

    return ret;
}

static int hisi_rproc_start(struct rproc *rproc)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    spin_lock_bh(&rproc_dev->rpmsg_ready_spin_mutex);
    rproc_dev->rpmsg_ready_state = 1;
    spin_unlock_bh(&rproc_dev->rpmsg_ready_spin_mutex);/*lint !e456 */
    return 0;
}

static int hisi_rproc_stop(struct rproc *rproc)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;

    RPROC_INFO("+\n");
    spin_lock_bh(&rproc_dev->rpmsg_ready_spin_mutex);
    rproc_dev->rpmsg_ready_state = 0;
    spin_unlock_bh(&rproc_dev->rpmsg_ready_spin_mutex);/*lint !e456 */
    RPROC_FLUSH_TX(rproc_dev->ap_a7_mbox);/*lint !e64 */

    isp_device_disable();
    RPROC_INFO("-\n");
	return 0;
}

static void hisi_rproc_kick(struct rproc *rproc, int vqid)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
	int ret;

	/* Send the index of the triggered virtqueue in the mailbox payload */
	communicat_msg[0] = vqid;

    RPROC_DEBUG("+.0x%x \n", communicat_msg[0]);
	ret = RPROC_ASYNC_SEND(rproc_dev->ap_a7_mbox, communicat_msg, sizeof(communicat_msg[0]));/*lint !e64 */
	if(ret)
        RPROC_ERR("Failed: RPROC_ASYNC_SEND.%d\n", ret);
    hisp_sendx();
	communicat_msg[0] = 0;
    RPROC_DEBUG("-\n");
}

bool rproc_get_sync_flag(void)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    struct rproc *rproc = rproc_dev->isp_rproc->rproc;

    return rproc->sync_flag;
}

void rproc_set_sync_flag(bool flag)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    struct rproc *rproc = rproc_dev->isp_rproc->rproc;

    rproc->sync_flag = flag;
}
int set_isp_remap_addr(u64 remap_addr)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    struct rproc_shared_para *param = NULL;

    if(!remap_addr){
        RPROC_ERR("Failed : remap_addr.0x%llx\n", remap_addr);
        return -ENOMEM;
    }
    dev->remap_addr = remap_addr;

    param = rproc_get_share_para();
    if(param == NULL){
        RPROC_ERR("Failed : rproc_get_share_para\n");
        return -ENOMEM;
    }
    param->bootware_paddr           = remap_addr;

    return 0;
}

int hisi_isp_rproc_disable(void)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    struct rproc_vdev *rvdev, *rvtmp;
    struct rproc *rproc;
    int err = 0;

    RPROC_INFO("+\n");
    if ((err = bypass_power_updn()) != 0 ) {/*lint !e838 */
        pr_err("[%s] bypass_power_updn.0x%x\n", __func__, err);
        return -ENODEV;
    }

    if (!rproc_dev->isp_rproc) {
        RPROC_INFO("Failed : isp_rproc.%pK\n", rproc_dev->isp_rproc);
        return -ENOMEM;
    }
    rproc = rproc_dev->isp_rproc->rproc;

    init_completion(&rproc->crash_comp);

    /* clean up remote vdev entries */
    list_for_each_entry_safe(rvdev, rvtmp, &rproc->rvdevs, node)
    {
        if(rvdev == NULL)
        {
            pr_err("[%s] list_for_each_entry_safe return rvdev null\n", __func__);
            return -ENODEV;
        }
        rproc_remove_virtio_dev(rvdev);
    }

    /* Free the copy of the resource table */
    kfree(rproc->cached_table);

	rproc->domain = NULL;
    rproc_set_sync_flag(true);
    RPROC_INFO("-\n");

    return 0;
}
EXPORT_SYMBOL(hisi_isp_rproc_disable);

int hisi_isp_rproc_enable(void)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    struct rproc *rproc;
    int err = 0;

    RPROC_INFO("+\n");
    if ((err = bypass_power_updn()) != 0 ) {/*lint !e838 */
        pr_err("[%s] bypass_power_updn.0x%x\n", __func__, err);
        return -ENODEV;
    }

	pr_info("%s.%d: case type.%u\n", __func__, __LINE__,
			hisi_isp_rproc_case_get());

    if (!rproc_dev->isp_rproc) {
        RPROC_INFO("Failed : isp_rproc.%pK\n", rproc_dev->isp_rproc);
        return -ENOMEM;
    }
    rproc = rproc_dev->isp_rproc->rproc;

    if (!rproc_get_sync_flag()) {
        RPROC_INFO("sync_flag exception.\n");
        return -EAGAIN;
    }

    rproc_set_sync_flag(false);
    set_rpmsg_status(0);

    /* clean up the invalid exception entries */
    if (!list_empty(&rproc->rvdevs)) {
        RPROC_ERR("Failed : enable exception will disable...\n");
        hisi_isp_rproc_disable();
    }

    init_completion(&rproc->boot_comp);
    rproc_dev->isp_subsys_power_flag = 0;

    RPROC_INFO("rproc_enable...\n");
    err = rproc_add_virtio_devices(rproc);
    if (err) {
        RPROC_ERR("Failed : rproc_enable.%d\n", err);
        rproc_set_sync_flag(true);/*lint !e747 */
        return err;
    }

    RPROC_INFO("waiting boot_comp...\n");
    wait_for_completion(&rproc->boot_comp);
    RPROC_INFO("wait boot_comp X\n");

    if (!rproc->rproc_enable_flag || !rproc_dev->rpmsg_status) {
        RPROC_ERR("Failed : rproc_enable rproc_enable_flag.%d, rpmsg_status.%d", rproc->rproc_enable_flag, rproc_dev->rpmsg_status);
        rproc_set_sync_flag(true);
        return -EAGAIN;
    }

    err = hisi_isp_rproc_pgd_set(rproc);
    if (0 != err) {
        RPROC_ERR("Failed : hisi_isp_rproc_pgd_set.%d\n", err);
        goto enable_err;
    }
    hisi_isp_data_text_vunmap(rproc);
    err = isp_device_enable();
    if (0 != err) {
        RPROC_ERR("Failed : isp_device_enable.%d\n", err);
        goto enable_err;
    }

    RPROC_INFO("-\n");

    return 0;
enable_err:
    hisi_isp_rproc_disable();

    return err;
}
EXPORT_SYMBOL(hisi_isp_rproc_enable);
/*lint -save -e454*/
int hisp_jpeg_powerup(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    int ret = 0;

    mutex_lock(&dev->jpeg_mutex);
    if (!wake_lock_active(&dev->jpeg_wakelock)) {
        wake_lock(&dev->jpeg_wakelock);
        RPROC_INFO("jpeg power up wake lock.\n");
    }
    mutex_unlock(&dev->jpeg_mutex);/*lint !e456 */
    switch (dev->case_type) {
        case NONSEC_CASE:
            ret = hisp_nsec_jpeg_powerup();
            break;
        case SEC_CASE:
            ret = hisp_sec_jpeg_powerup();
            break;
        default:
            ret = -EINVAL;
            break;
    }
    if (ret != 0) {
    RPROC_ERR("Failed : jpeg power up fail.%d, case_type.%d\n", ret, dev->case_type);
    mutex_lock(&dev->jpeg_mutex);
    if (wake_lock_active(&dev->jpeg_wakelock)) {
        wake_unlock(&dev->jpeg_wakelock);
        RPROC_INFO("jpeg power up wake unlock.\n");
    }
    mutex_unlock(&dev->jpeg_mutex);/*lint !e456 */
    }
    return ret;
}
/*lint -restore */
EXPORT_SYMBOL(hisp_jpeg_powerup);

int hisp_jpeg_powerdn(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    int ret = 0;

    switch (dev->case_type) {
        case NONSEC_CASE:
            ret = hisp_nsec_jpeg_powerdn();
            break;
        case SEC_CASE:
            ret = hisp_sec_jpeg_powerdn();
            break;
        default:
            ret = -EINVAL;
            break;
    }
    mutex_lock(&dev->jpeg_mutex);
    if (wake_lock_active(&dev->jpeg_wakelock)) {
        wake_unlock(&dev->jpeg_wakelock);
        RPROC_INFO("jpeg power up wake unlock.\n");
    }
    mutex_unlock(&dev->jpeg_mutex);/*lint !e456 */
    if(ret != 0)
        RPROC_ERR("Failed : jpeg power down fail.%d, case_type.%d\n", ret, dev->case_type);
    return ret;
}
EXPORT_SYMBOL(hisp_jpeg_powerdn);

static struct rproc_ops hisi_rproc_ops = {
    .start      = hisi_rproc_start,
    .stop       = hisi_rproc_stop,
    .kick       = hisi_rproc_kick,
//    .da_to_va   = hisp_rproc_da_to_va,// Diff between 4.1 and 4.4, use after 4.1 removed.
};

int hisi_rproc_select_def(void)
{
	struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    int ret;

    RPROC_INFO("+\n");
    if (!hw_is_fpga_board()) {
        ret = pinctrl_select_state(rproc_dev->isp_data.isp_pinctrl, rproc_dev->isp_data.pinctrl_def);
        if (0 != ret) {
            RPROC_ERR("Failed : could not set pins to default state.\n");
            return ret;
        }
    }
    RPROC_INFO("-\n");

    return 0;
}

int hisi_rproc_select_idle(void)
{
	struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    int ret;

    RPROC_INFO("+\n");
    if (!hw_is_fpga_board()) {
        ret = pinctrl_select_state(rproc_dev->isp_data.isp_pinctrl, rproc_dev->isp_data.pinctrl_idle);
        if (0 != ret) {
            RPROC_ERR("Failed : could not set pins to ilde state.\n");
            return ret;
        }
    }
	RPROC_INFO("-\n");

    return 0;
}

int bypass_power_updn(void)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    return dev->bypass_pwr_updn;
}

int set_power_updn(int bypass)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;

    if (bypass != 0 || bypass != 1) {/*lint !e774 */
        RPROC_ERR("Failed : bypass.%x\n", bypass);
        return -EINVAL;
    }

    dev->bypass_pwr_updn = bypass;

    return 0;
}

static unsigned int get_boardid(void)
{
#define BOARDID_SIZE (4)
#define ISP_1_PIPE_FLAG (1 << 21)
#define ISP_2_PIPE_FLAG (1 << 20)
    struct rproc_boot_device *dev = &rproc_boot_dev;
    unsigned int boardid[BOARDID_SIZE] = {0};
    struct device_node *root = NULL;
    unsigned int pctrl_stat10;
    int ret = 0;

    dev->bypass_pwr_updn = 0;
    if ((root = of_find_node_by_path("/")) == NULL) {/*lint !e838 */
        RPROC_ERR("Failed : of_find_node_by_path.%pK\n", root);
        goto err_get_bid;
    }

    if ((ret = of_property_read_u32_array(root, "hisi,boardid", boardid, BOARDID_SIZE)) < 0 ) {
        RPROC_ERR("Failed : of_property_read_u32.%d\n", ret);
        goto err_get_bid;
    }
    RPROC_INFO("Board ID.(%x %x %x %x)\n", boardid[0], boardid[1], boardid[2], boardid[3]);

    if (boardid[0] == 7 && boardid[1] == 0 && boardid[2] == 9 ) {
        if (boardid[3] == 8) {
            if ((dev->pctrl_base = (void __iomem *)ioremap(SOC_ACPU_PCTRL_BASE_ADDR, SZ_4K)) == NULL) {/*lint !e747  */
                RPROC_ERR("Failed : pctrl_base ioremap.%pK\n", dev->pctrl_base);
                goto err_get_bid;
            }
            pctrl_stat10 = readl(SOC_PCTRL_PERI_STAT10_ADDR(dev->pctrl_base));/*lint !e732 */
            if ((pctrl_stat10 & (ISP_1_PIPE_FLAG | ISP_2_PIPE_FLAG)) == 0) {
                RPROC_ERR("ISP/Camera Power Up and Down May be Bypassed, pctrl_stat10.0x%x\n", pctrl_stat10);
                dev->bypass_pwr_updn = 1;
            }
            iounmap(dev->pctrl_base);
            dev->pctrl_base = NULL;
        }
    }

    if(boardid[0] == 8 && boardid[1] == 0 && boardid[2] == 9) {
        if (boardid[3] == 7) {
            if ((dev->pctrl_base = (void __iomem *)ioremap(SOC_ACPU_PCTRL_BASE_ADDR, SZ_4K)) == NULL) {/*lint !e747  */
                RPROC_ERR("Failed : pctrl_base ioremap.%pK\n", dev->pctrl_base);
                goto err_get_bid;
            }
            pctrl_stat10 = readl(SOC_PCTRL_PERI_STAT10_ADDR(dev->pctrl_base));/*lint !e732 */
            if ((pctrl_stat10 & (ISP_1_PIPE_FLAG | ISP_2_PIPE_FLAG)) == 0) {
                RPROC_ERR("ISP/Camera Power Up and Down May be Bypassed, pctrl_stat10.0x%x\n", pctrl_stat10);
                dev->bypass_pwr_updn = 1;
            }
            iounmap(dev->pctrl_base);
            dev->pctrl_base = NULL;
        }
    }
    return boardid[0];
err_get_bid:
    return INVALID_BOARDID;/*lint !e570 */
}

/*lint -save -e429*/
static struct hisi_rproc_data *hisi_rproc_data_dtget(struct device *pdev)
{
    struct rproc_boot_device *dev = &rproc_boot_dev;
    struct device_node *np = pdev->of_node;
    struct hisi_rproc_data *data;
    const char *name = NULL;
    unsigned int addr, platform_info, ispa7_clk;
    int ret;

    if (!np) {
        RPROC_ERR("Failed : No dt node\n");
        return NULL;
    }

    if ((dev->boardid = get_boardid()) == INVALID_BOARDID)/*lint !e650 */
        return NULL;

    if (!use_sec_isp() && !use_nonsec_isp()) {
        dev->isp_subsys_ip = devm_regulator_get(pdev, "isp-subsys-rproc");
        if (IS_ERR(dev->isp_subsys_ip)) {
            RPROC_ERR("Failed : Couldn't get regulator ip.%pK\n", dev->isp_subsys_ip);
            return NULL;
        }
        RPROC_INFO("isp_subsys_ip.%pK\n", dev->isp_subsys_ip);
    }

    if(use_sec_isp() || use_nonsec_isp()){
        dev->ap_a7_mbox = HISI_RPROC_ISP_MBX23;
        dev->a7_ap_mbox = HISI_RPROC_ISP_MBX8;
    }else{
        dev->ap_a7_mbox = HISI_RPROC_ISP_MBX2;
        dev->a7_ap_mbox = HISI_RPROC_ISP_MBX0;
    }
    data = devm_kzalloc(pdev, sizeof(struct hisi_rproc_data), GFP_KERNEL);
    if (!data) {
        RPROC_ERR("Failed : cannot allocate platform data memory.%pK\n", data);
        return NULL;
    }

	if ((ret = of_property_read_string(np, "isp-names", &name)) < 0 ) {
		RPROC_ERR("Failed : isp-names.%s of_property_read_string.%d\n", name, ret);
		return NULL;
	}
	data->name = name;

	if ((ret = of_property_read_string(np, "firmware-names", &name)) < 0 ) {
		RPROC_ERR("Failed : firmware-names.%s of_property_read_string.%d\n", name, ret);
		return NULL;
	}
	data->firmware = name;

	if ((ret = of_property_read_string(np, "bootware-names", &name)) < 0 ) {
		RPROC_ERR("Failed : bootware-names.%s of_property_read_string.%d\n", name, ret);
		return NULL;
	}
	data->bootware = name;

	if ((ret = of_property_read_string(np, "mailbox-names", &name)) < 0 ) {
		RPROC_ERR("Failed : mailbox-names.%s of_property_read_32.%d\n", name, ret);
		return NULL;
	}
	data->mbox_name = name;

	if ((ret = of_property_read_u32(np, "isp-ipc-addr", &addr)) < 0 ) {
		RPROC_ERR("Failed : isp-ipc_addr.0x%x of_property_read_u32.%d\n", addr, ret);
		return NULL;
	}
    data->ipc_addr = addr;
    RPROC_DEBUG("isp-ipc-addr.%u \n", data->ipc_addr);

	if ((ret = of_property_read_u32(np, "isp-remap-addr", &addr)) < 0 ) {
		RPROC_ERR("Failed : isp-remap-addr.0x%x of_property_read_u32.%d\n", addr, ret);
		return NULL;
	}
    dev->remap_addr = addr;

	if ((ret = of_property_read_u32(np, "isp_local_timer", &platform_info)) < 0 ) {
		RPROC_ERR("Failed: isp_local_timer.0x%x of_property_read_u32.%d\n", platform_info, ret);
		return NULL;
	}
    dev->tmp_plat_cfg.isp_local_timer = platform_info;
	RPROC_INFO("isp_local_timer = %d\n", dev->tmp_plat_cfg.isp_local_timer);

    dev->ispa7_clk = devm_clk_get(pdev, NULL);
    if(IS_ERR_OR_NULL(dev->ispa7_clk)) {
        RPROC_ERR("Failed : ispa7_clk.%ld\n", PTR_ERR(dev->ispa7_clk));
        return NULL;
    }

	if ((ret = of_property_read_u32(np, "ispa7-default-clk", &ispa7_clk)) < 0 ) {
		RPROC_ERR("Failed: ispa7_clk.0x%x of_property_read_u32.%d\n", ispa7_clk, ret);
		return NULL;
	}
	dev->ispa7_clk_value = ispa7_clk;
	RPROC_INFO("ispa7_clk.%d\n", dev->ispa7_clk_value);

    if (IS_HI6250(dev->boardid)) {
        if ((ret = of_property_read_string_index(np, "clock-names", 1, &name)) < 0) {
            RPROC_ERR("Failed : isp_timer clock-names boardid.0x%x, ret.%d\n", dev->boardid, ret);
            return NULL;
        }
        RPROC_INFO("[ID.0x%x] isp_timer : clock-names.%s, \n", dev->boardid, name);

    dev->isp_timer = devm_clk_get(pdev, name);
    if (IS_ERR_OR_NULL(dev->isp_timer)) {
            RPROC_ERR("Failed : isp_timer devm_clk_get boardid.0x%x, ret.%d\n", dev->boardid, ret);
            return NULL;
        }
        RPROC_INFO("[ID.0x%x] isp_timer : %pK\n", dev->boardid, dev->isp_timer);
    }

    return data;
}

struct coresight_user_para {
    unsigned long long coresight_addr_da;   /*R8 addr*/
    unsigned long long coresight_addr_vir;  /*AP addr*/
};
struct coresight_user_para g_cs_para;

void coresight_mem_init(struct device *dev)
{
    void *addr_vir;
    dma_addr_t dma_addr = 0;

#if 0
    if ((addr_vir = hisi_fstcma_alloc(&dma_addr, 0x1000, GFP_KERNEL)) == NULL) {
        pr_err("[coresight] [%s] coresight_mem_init.%pK\n", __func__, addr_vir);
        return -ENOMEM;
    }
    pr_err("[coresight] [%s] coresight_mem_init  addr_vir:%pK addr_ph:%pK \n", __func__, addr_vir, dma_addr);

#else
    if ((addr_vir = dma_alloc_coherent(dev, 0x1000, &dma_addr, GFP_KERNEL)) == NULL) {
        pr_err("[coresight] [%s] coresight_mem_init.%pK\n", __func__, addr_vir);
        return;
    }

    pr_err("[coresight] [%s] coresight_mem_init  addr_vir:%pK addr_ph:%pK \n", __func__, (void *)addr_vir, (void *)dma_addr);
#endif
    /*lint !e110 !e82 !e533*/
    g_cs_para.coresight_addr_da = (unsigned long long)dma_addr;  /*lint !e110 !e82 !e533 !e626 */
    g_cs_para.coresight_addr_vir = (unsigned long long)addr_vir; /*lint !e110 !e82 !e533 !e626 */

    pr_err("[coresight] coresight_mem_init  addr_vir:%llx addr_ph:%llx \n", g_cs_para.coresight_addr_vir, g_cs_para.coresight_addr_da);
    return;
}

/*lint -restore */
#ifdef CONFIG_HISI_REMOTEPROC_DMAALLOC_DEBUG
void *get_vring_dma_addr(u64 *dma_handle, size_t size, unsigned int index)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    struct hisi_isp_vring_s *hisi_isp_vring = NULL;
    if(rproc_dev == NULL){
        pr_err("%s: rproc_boot_device in NULL\n", __func__);
        return NULL;
    }
    RPROC_INFO("+\n");
    hisi_isp_vring = &rproc_dev->hisi_isp_vring[index];
    if(hisi_isp_vring == NULL){
        pr_err("%s: hisi_isp_vring is NULL\n", __func__);
        return NULL;
    }
    if(hisi_isp_vring->size != size){
        pr_err("%s: hisi_isp_vring size not same: 0x%lx --> 0x%lx\n", __func__,hisi_isp_vring->size,size);
        return NULL;
    }
    *dma_handle = hisi_isp_vring->paddr;
    pr_debug("%s: dma_handle.0x%llx-->%pK,virt_addr.0x%pK\n", __func__,hisi_isp_vring->paddr,dma_handle,hisi_isp_vring->virt_addr);
    RPROC_INFO("-\n");
    return hisi_isp_vring->virt_addr;
}

static int get_vring_dma_addr_probe(struct platform_device *pdev)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    struct hisi_isp_vring_s *hisi_isp_vring = NULL;

    unsigned int i = 0;

    if((rproc_dev == NULL) || (pdev == NULL)){
        pr_err("%s: rproc_boot_device in NULL\n", __func__);
        return -ENOMEM;
    }
    for(i = 0;i < HISI_ISP_VRING_NUM;i++)
    {
        hisi_isp_vring = &rproc_dev->hisi_isp_vring[i];
        if(hisi_isp_vring == NULL){
            pr_err("%s: hisi_isp_vring is NULL\n", __func__);
            return -ENOMEM;
        }
        if(i < (HISI_ISP_VRING_NUM - 1))
           hisi_isp_vring->size =  HISI_ISP_VRING_SIEZ;
        else
           hisi_isp_vring->size =  HISI_ISP_VQUEUE_SIEZ;

        hisi_isp_vring->virt_addr = dma_alloc_coherent(&pdev->dev,
                         hisi_isp_vring->size, &hisi_isp_vring->paddr,
                         GFP_KERNEL);
        if (!hisi_isp_vring->virt_addr) {
            return -ENOMEM;
        }
        pr_debug("%s: hisi_isp_vring %d :paddr.0x%llx,size.0x%lx\n", __func__,i,hisi_isp_vring->paddr,hisi_isp_vring->size);
    }
    return 0;
}
static int get_vring_dma_addr_remove(struct platform_device *pdev)
{
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
    struct hisi_isp_vring_s *hisi_isp_vring = NULL;
    unsigned int i = 0;
    if(rproc_dev == NULL){
        pr_err("%s: rproc_boot_device in NULL\n", __func__);
        return -ENOMEM;
    }
    for(i = 0;i < HISI_ISP_VRING_NUM;i++)
    {
        hisi_isp_vring = &rproc_dev->hisi_isp_vring[i];
        if(hisi_isp_vring == NULL){
            pr_err("%s: hisi_isp_vring is NULL\n", __func__);
            return -ENOMEM;
        }
        dma_free_coherent(&pdev->dev, hisi_isp_vring->size,\
                          hisi_isp_vring->virt_addr, hisi_isp_vring->paddr);
    }
    return 0;
}
#endif
static int hisi_rproc_probe(struct platform_device *pdev)
{
	struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
	struct device *dev = &pdev->dev;
    struct device_node *np = dev->of_node;
    struct hisi_rproc_data *data;
    struct hisi_isp_rproc *hproc;
    int ret;

	RPROC_INFO("+\n");

    rproc_dev->isp_pdev = pdev;

    ret = hisi_isp_rproc_case_set(DEFAULT_CASE);
    if (ret) {
        pr_err("%s: case set falied, case.%x\n", __func__, DEFAULT_CASE);
        return -EINVAL;
    }

	if ((ret = of_property_read_u32(np, "hisi,use_secisp", &rproc_dev->secisp)) < 0 ) {/*lint !e64 */
        RPROC_ERR("Failed: secisp.0x%x of_property_read_u32.%d\n", rproc_dev->secisp, ret);
        return ret;
    }

    if (rproc_dev->secisp) {
        ret = hisi_isp_rproc_case_set(SEC_CASE);
        if (ret) {
            pr_err("%s: case set falied, case.%x\n", __func__, SEC_CASE);
            return -EINVAL;
        }

        pr_alert("%s: sec isp probe.\n", __func__);
        if ((ret = hisi_atfisp_probe(pdev)) < 0) {
            RPROC_ERR("Failed : hisi_atfisp_probe.%d\n", ret);
            return ret;
        }

        pr_alert("%s: no sec isp probe.\n", __func__);
        if ((ret = hisi_isp_nsec_probe(pdev)) < 0) {
            RPROC_ERR("Failed : hisi_isp_nsec_probe.%d\n", ret);
            return ret;
        }
        hisp_mem_pool_init();
#ifdef CONFIG_HISI_REMOTEPROC_FASTCMA_MDC_DEBUG
        rproc_dev->mdc_fstcma = NULL;
#else
        rproc_dev->mdc_mem_info = NULL;
#endif
        if(get_isp_mdc_flag()){
            if ((ret = mdc_addr_pa_init()) < 0) {
                RPROC_ERR("Failed : mdc_addr_pa_init.%d\n", ret);
                return ret;
            }
        }
    }

	if ((data = hisi_rproc_data_dtget(dev)) == NULL ) {
		dev_err(&pdev->dev, "hisi_rproc_data_dtget: %pK\n", data);
		return -EINVAL;
	}

    ret = dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(64));/*lint !e598 !e648 */
    if (ret) {
        dev_err(&pdev->dev, "dma_set_coherent_mask: %d\n", ret);
        return ret;
    }

	RPROC_INFO("rproc_alloc\n");
    hisi_rproc = rproc_alloc(&pdev->dev, data->name, &hisi_rproc_ops,
                data->firmware, sizeof(*hproc));
    if (!hisi_rproc)
        return -ENOMEM;

    wake_lock_init(&rproc_dev->jpeg_wakelock, WAKE_LOCK_SUSPEND, "jpeg_wakelock");
    wake_lock_init(&rproc_dev->ispcpu_wakelock, WAKE_LOCK_SUSPEND, "ispcpu_wakelock");
    mutex_init(&rproc_dev->jpeg_mutex);
    mutex_init(&rproc_dev->ispcpu_mutex);
    mutex_init(&rproc_dev->sharedbuf_mutex);
    mutex_init(&rproc_dev->rpmsg_ready_mutex);
    spin_lock_init(&rproc_dev->rpmsg_ready_spin_mutex);

	if ((ret = rproc_bootware_attach(hisi_rproc, data->bootware)) < 0 ) {
		dev_err(&pdev->dev, "rproc_bootware_attach: %d\n", ret);
		goto free_hisi_rproc;
	}

    hisi_rproc->ipc_addr = data->ipc_addr;
	hisi_rproc->has_iommu = true;
	RPROC_INFO("hisi_rproc.%pK, priv.%pK, ipc_addr = 0x%x\n",
                        hisi_rproc, hisi_rproc->priv, hisi_rproc->ipc_addr);

    hproc = hisi_rproc->priv;
    hproc->rproc = hisi_rproc;
    rproc_dev->isp_rproc = hproc;

	RPROC_INFO("hproc->rproc.%pK, hproc.%pK\n", hproc->rproc, hproc);

    platform_set_drvdata(pdev, hisi_rproc);

	RPROC_INFO("rproc_add\n");

    ret = rproc_add(hisi_rproc);

    if (ret)
        goto free_hisi_rproc;
#ifdef CONFIG_HISI_REMOTEPROC_DMAALLOC_DEBUG
    ret = get_vring_dma_addr_probe(pdev);
    if (ret < 0)
        goto free_hisi_rproc;
#endif
	ret = init_hisi_ipc_resource();
	if (0 != ret) {
		RPROC_ERR("Failed : init_hisi_ipc_resource.%d\n", ret);
		goto free_hisi_rproc;
	}

	hproc->nb.notifier_call = hisi_rproc_mbox_callback;

	ret = RPROC_MONITOR_REGISTER(rproc_dev->a7_ap_mbox, &hproc->nb);/*lint !e64 */
	if (0 != ret) {
		RPROC_ERR("Failed : RPROC_MONITOR_REGISTER.%d\n", ret);
		goto free_hisi_rproc;
	}
    rproc_dev->rpmsg_ready_state = 0;
    hisi_rproc->rproc_enable_flag = false;
    rproc_set_sync_flag(true);

#ifdef ISP_CORESIGHT
    coresight_mem_init(dev);
#endif

    RPROC_INFO("-\n");
    return 0;

free_hisi_rproc:
    rproc_put(hisi_rproc);
    wake_lock_destroy(&rproc_dev->jpeg_wakelock);
    wake_lock_destroy(&rproc_dev->ispcpu_wakelock);
    mutex_destroy(&rproc_dev->jpeg_mutex);
    mutex_destroy(&rproc_dev->ispcpu_mutex);
    mutex_destroy(&rproc_dev->rpmsg_ready_mutex);
    return ret;
}

static int hisi_rproc_remove(struct platform_device *pdev)
{
    struct rproc *rproc = platform_get_drvdata(pdev);
    struct rproc_boot_device *rproc_dev = &rproc_boot_dev;
	struct hisi_isp_rproc *hproc = NULL;
	int ret;

	RPROC_INFO("+\n");
    if (NULL == rproc) {
        RPROC_ERR("Failed : rproc.%pK\n", rproc);
        return -ENOMEM;
    }

    hproc = rproc->priv;
    rproc_dev->rpmsg_ready_state = 0;
    if (NULL != hproc) {
        ret = RPROC_MONITOR_UNREGISTER(rproc_dev->a7_ap_mbox, &hproc->nb);/*lint !e64 */
        if (0 != ret) {
            RPROC_ERR("Failed : RPROC_MONITOR_UNREGISTER.%d\n", ret);
        }
    } else {
		RPROC_ERR("rproc->priv.%pK\n", rproc->priv);
    }
	RPROC_FLUSH_TX(rproc_dev->ap_a7_mbox);/*lint !e64 */
	kthread_stop(isp_rx_mbox->rpmsg_rx_tsk);

	kfifo_free(&isp_rx_mbox->rpmsg_rx_fifo);
	kfree(isp_rx_mbox);
    wake_lock_destroy(&rproc_dev->jpeg_wakelock);
    wake_lock_destroy(&rproc_dev->ispcpu_wakelock);
    mutex_destroy(&rproc_dev->jpeg_mutex);
    mutex_destroy(&rproc_dev->ispcpu_mutex);
    mutex_destroy(&rproc_dev->sharedbuf_mutex);
    mutex_destroy(&rproc_dev->rpmsg_ready_mutex);
#ifdef CONFIG_HISI_REMOTEPROC_DMAALLOC_DEBUG
    ret = get_vring_dma_addr_remove(pdev);
    if (ret < 0)
        RPROC_ERR("Failed : get_vring_dma_addr_remove.%d\n", ret);
#endif
    rproc_del(rproc);
    rproc_put(rproc);

    if (rproc_dev->secisp) {
        if ((ret = hisi_atfisp_remove(pdev)) < 0)
            RPROC_ERR("Failed : hisi_atfisp_remove.%d\n", ret);

        if ((ret = hisi_isp_nsec_remove(pdev)) != 0)
            RPROC_ERR("Failed : hisi_isp_nsec_remove.%d\n", ret);
        free_mdc_ion(MEM_MDC_SIZE);
    }

    RPROC_INFO("-\n");
    return 0;
}

static struct of_device_id hisi_rproc_of_match[] = {
    { .compatible = DTS_COMP_NAME},
    { },
};
MODULE_DEVICE_TABLE(of, hisi_rproc_of_match);

static struct platform_driver hisi_rproc_driver = {
    .driver = {
        .owner      = THIS_MODULE,
        .name       = "isp",
        .of_match_table = of_match_ptr(hisi_rproc_of_match),
    },
    .probe  = hisi_rproc_probe,
    .remove = hisi_rproc_remove,
};
module_platform_driver(hisi_rproc_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("HiStar V100 Remote Processor control driver");
