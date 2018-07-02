/*
 * hisilicon efuse driver, hisi_atfisp.c
 *
 * Copyright (c) 2013 Hisilicon Technologies CO., Ltd.
 *
 */

//ISP_LINT
/*lint -e750
 -esym(750,*)*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/scatterlist.h>
#include <linux/printk.h>
#include <linux/file.h>
#include <linux/kthread.h>
#include <linux/rproc_share.h>
#include <linux/remoteproc.h>
#include <linux/ion.h>
#include <linux/hisi/hisi_ion.h>
#include <linux/genalloc.h>
#include <linux/hisi/hisi-iommu.h>
#include <linux/hisi/ion-iommu.h>
#include <linux/mutex.h>
#include <linux/iommu.h>
#include <linux/compiler.h>
#include <linux/cpumask.h>
#include <asm/uaccess.h>
#include <asm/compiler.h>
#include <global_ddr_map.h>
#include "teek_client_id.h"
#include <linux/hisi/hisi_load_image.h>
#include <linux/hisi/kirin_partition.h>
#include <linux/platform_data/remoteproc-hisi.h>
#include "hisi_partition.h"
#include "partition.h"
#include <isp_ddr_map.h>
#include <linux/list.h>
#include "libhwsecurec/securec.h"
#include <linux/syscalls.h>
#include <linux/delay.h>

#define MAX_SIZE    64
#define ISP_ATF_CPU 1
#define DEVICE_PATH  "/dev/block/bootdevice/by-name/"

#define A7_REQUEST_ADDR         0xD0000000
#define A7_REQUEST_BUF_SIZE     0x96000
#define A7_REQUEST_BUF_NUM      16
#define A7_ISPDTS_ADDR          0xC8000000
#define A7_ISPDTS_SIZE          0x02000000
#define A7_DTS_BUF_SIZE         0x200000
#define A7_DTS_BUF_NUM          16
#define A7_GET_OTP_ADDR         0xD0A00000
#define A7_CAPABILITY_ADDR      0xD0B00000
#define A7_ACQUIRE_ADDR         0xD0C00000
#define A7_CALIBRATION_ADDR     0xD0D00000
#define A7_TONEMAP_ADDR         0xD0E00000
#define A7_MAP_BUFFER_ADDR      0xD1000000
#define IRIS_MAP_BUFFER_ADDR    0xC2A00000
#define A7_MAP_DMAP_BUFFER_ADDR 0xCA000000
#define A7_MAP_BUFFER_SIZE      0xF000000
#define IRIS_MAP_BUFFER_SIZE    0X1600000

#define ISP_FN_SET_PARAMS   (0xC500AB00)
#define ISP_FN_A7_INIT      (0xC500AB22)
#define ISP_FN_A7_EXIT      (0xC500AB23)
#define ISP_FN_A7_MAP       (0xC500AB24)
#define ISP_FN_A7_UNMAP     (0xC500AB25)
#define ISP_FN_SET_NONSEC   (0xC500AB26)
#define ISP_FN_DISRESET_A7  (0xC500AB27)
#define ISP_FN_RESET_A7     (0xC500AB29)
#define ISP_FN_ISPFE_PWU    (0xC500AB30)

/* VivoBus Regs Base & Offset */
#define AP_A7_SHARE_OFFSET      (0x1000)
#define AP_A7_CPUCFG_OFFSET     (0x800)

/* pclint coverity--not used */
#define ISP_FN_ISP_INIT         (0xC500AB11)
#define ISP_FN_ISP_EXIT         (0xC500AB12)

/* hisp mem_pool  */
#define ISP_MEM_POOL_NUM     (3)
#define ISP_MEM_POOL_ALIGN   (0x8000)
#define ISP_MEM_POOL_1M_ALIGN  (0x100000)
#define ISP_MEM_POOL_1M_MASK_L (0x000FFFFF)
#define ISP_MEM_POOL_1M_MASK_H (0xFFF00000)

enum mapType
{
    MAP_TYPE_DYNAMIC = 0,
    MAP_TYPE_STATIC,
    MAP_TYPE_STATIC_SEC,
    MAP_TYPE_DYNAMIC_CARVEOUT,
    MAP_TYPE_MAX,
};

struct hisi_isp_ops {
#define UNINITIAL   0
    unsigned int refs_ispsrt_subsys;
    unsigned int refs_isp_module;
    unsigned int refs_a7_module;
    unsigned int refs_dts;
    unsigned int refs_rsc;
    unsigned int refs_fw;
    unsigned int refs_setparams;
    int (*ispsrtup)(void);
    int (*ispsrtdn)(void);
    int (*ispinit)(void);
    int (*ispexit)(void);
    int (*a7init)(void);
    int (*a7exit)(void);
    int (*loaddts)(void);
    int (*loadrsc)(void);
    int (*loadfw)(void);
    int (*setparams)(unsigned int, unsigned int, unsigned int);
};

struct hisi_a7mapping_s {
    unsigned int a7va;
    unsigned int size;
    unsigned int prot;
    unsigned int offset;
    unsigned int reserve;
    unsigned long a7pa;
    void *apva;
};

struct hisi_atfshrdmem_s {
    struct hisi_a7mapping_s a7mapping[MAXA7MAPPING];
};

struct hisi_sec_ion_s {
    u64 sec_boot_phymem_addr;
    u64 sec_fw_phymem_addr;
    struct ion_handle *boot_ion_handle;
    struct ion_handle *fw_ion_handle;
    struct ion_client *ion_client;
};

struct hisi_atfisp_s {
    struct device *device;
    unsigned int boardid;
    void *atfshrd_vaddr;
    unsigned long long atfshrd_paddr;
    void *rsctable_vaddr;
    unsigned long long rsctable_paddr;
    struct hisi_atfshrdmem_s *shrdmem;
    struct hisi_isp_ops *ispops;
    struct task_struct *secisp_kthread;
    struct task_struct *secisp_stop_kthread;
    wait_queue_head_t secisp_wait;
    bool secisp_wake;
    struct mutex pwrlock;
    struct mutex isp_iova_pool_mutex;
    unsigned long long phy_pgd_base;
    struct iommu_domain *domain;
    struct gen_pool *isp_iova_pool;
    unsigned long isp_iova_start;
    unsigned long isp_iova_size;
    struct ion_handle *handle;
    struct ion_client *client;
    unsigned int a7va_a7ispmem;
    unsigned int ispva_a7ispmem;
    void *ap_dyna_array;
    struct hisi_a7mapping_s *ap_dyna;
    int map_req_flag;
    int map_dts_flag;
    int isp_wdt_flag;
    struct rproc_shared_para *sec_isp_share_para;
    int clk_powerby_media;
    struct hisi_sec_ion_s *sec_mem_info;
    struct hisi_isp_ion_s *sec_smmuerr_ion;
} atfisp_dev;

struct mem_pool_info_list_s {
    unsigned int addr;
    unsigned int size;
    struct list_head list;
};

struct mem_pool_info_s {
    unsigned int addr;
    unsigned int size;
    unsigned int prot;
    unsigned int enable;
    struct mem_pool_info_list_s *node;
    struct gen_pool *isp_mem_pool;
};

struct hisi_mem_pool_s {
    unsigned int count;
    struct mutex mem_pool_mutex;
    struct mem_pool_info_s isp_pool_info[ISP_MEM_POOL_NUM];
}hisi_mem_pool_info;

struct map_type_info {
    unsigned int va;
    unsigned int size;
};

struct map_sglist_s {
    unsigned long long addr;
    unsigned int size;
};

unsigned int map_type_info[MAP_TYPE_MAX];
extern u64 get_isprdr_addr(void);
extern int use_sec_isp(void);
static int free_secmem_ion(void *data);
static int smmu_err_addr_free(void);

noinline int atfd_hisi_service_isp_smc(u64 funcid, u64 arg0, u64 arg1, u64 arg2)
{
       asm volatile (
            __asmeq("%0", "x0")
            __asmeq("%1", "x1")
            __asmeq("%2", "x2")
            __asmeq("%3", "x3")
            "smc    #0\n"
        : "+r" (funcid)
        : "r" (arg0), "r" (arg1), "r" (arg2));

    return (int)funcid;
}

static void isp_iova_pool_destroy(struct gen_pool *pool)
{
    gen_pool_destroy(pool);
}

/*lint -save -e838 */
static unsigned long hisi_isp_alloc_iova(struct gen_pool *pool,
        unsigned long size, unsigned long align)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    unsigned long iova = 0;

    mutex_lock(&dev->isp_iova_pool_mutex);

    iova = gen_pool_alloc(pool, size);
    if(!iova){
        pr_err("hisi isp iommu gen_pool_alloc failed!\n");
        mutex_unlock(&dev->isp_iova_pool_mutex);
        return 0;
    }

    if(align > ((unsigned long)1 << (unsigned long)(pool->min_alloc_order)))/*lint !e571 */
        pr_info("hisi iommu domain cant align to 0x%lx\n", align);
    mutex_unlock(&dev->isp_iova_pool_mutex);
    return iova;
}

static void hisi_isp_free_iova(struct gen_pool *pool,
        unsigned long iova, size_t size)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;

    mutex_lock(&dev->isp_iova_pool_mutex);
    gen_pool_free(pool, iova, size);
    mutex_unlock(&dev->isp_iova_pool_mutex);
}
static struct gen_pool *isp_iova_pool_setup(unsigned long start,
        unsigned long size, unsigned int align)
{
    struct gen_pool *pool = NULL;
    int ret = 0;

    pool = gen_pool_create((int)(order_base_2(align)), -1);/*lint !e666 !e835 !e747 !e516 !e866 !e712 */
    if(!pool){
        pr_err("Create isp gen pool failed!\n");
        return NULL;
    }
    /* iova start should not be 0, because return
       0 when alloc iova is considered as error */
    if(!start)
        pr_err("iova start should not be 0!\n");
    ret = gen_pool_add(pool, start, size, -1);
    if(ret){
        pr_err("Gen pool add failed!\n");
        isp_iova_pool_destroy(pool);
        return NULL;
    }

    return pool;
}

/*lint -restore */
int hisp_getsglist(void *listmem, void **vaddr, int size)
{
    struct scatterlist *sg;
    struct sg_table *table;
    dma_addr_t dma_addr = 0;
    struct page **pages, **tmppages;
    int npages = 0, ret, i;
    unsigned long long *plist = listmem;
    pgprot_t pageprot = PAGE_KERNEL;

    if ((npages = PAGE_ALIGN(size) / PAGE_SIZE) <= 0) {/*lint !e573 */
        pr_err("[%s] Failed : npages.0x%x, size.0x%x\n", __func__, npages, size);
        return -EINVAL;
    }

    if ((table = kzalloc(sizeof(struct sg_table), GFP_KERNEL)) == NULL) {
        pr_err("[%s] Failed : kzalloc.%pK\n", __func__, table);
        return -ENOMEM;
    }

    if ((ret = sg_alloc_table(table, npages, GFP_KERNEL)) != 0) {
        pr_err("[%s] Failed : sg_alloc_table.%d, table.%pK, npages.%d\n", __func__, ret, table, npages);
        kfree(table);
        return -ENOMEM;
    }

    if ((pages = vmalloc(npages * sizeof(struct page *))) == NULL) {
        pr_err("[%s] Failed : vmalloc.%pK\n", __func__, pages);
        sg_free_table(table);
        kfree(table);
        return -ENOMEM;
    }

    for (i = 0; i < npages; i ++)
        pages[i] = alloc_page(GFP_KERNEL | GFP_DMA | __GFP_ZERO);

    pageprot = pgprot_noncached(pageprot);
    if ((*vaddr = vmap(pages, npages, VM_MAP, pageprot)) == NULL) {
        pr_err("[%s] Failed : vmap.%pK\n", __func__, *vaddr);
        sg_free_table(table);
        kfree(table);
        for (i = 0; i < npages; i ++){
            free_page((unsigned long)pages[i]);
        }
        vfree(pages);
        return -ENOMEM;
    }

    tmppages = pages;
    sg = table->sgl;
    while (sg) {
        struct page *page = *tmppages;
        sg_set_page(sg, page, PAGE_SIZE, 0);
        if ((dma_addr = sg_dma_address(sg)) == 0)
            dma_addr = sg_phys(sg);

        *plist = (unsigned long long)dma_addr;
        //pr_err("[%s] size.(0x%x, 0x%x), vmap.%pK, dma_addr.0x%llx, tmppages.%pK, *plist.(%pK, 0x%llx, %pK)\n", __func__, size, npages, *vaddr, dma_addr, tmppages, *plist, *plist, plist);
        plist ++;
        tmppages ++;
        sg = sg_next(sg);
    }
    sg_free_table(table);
    kfree(table);
    vfree(pages);

    return npages;
}

int hisp_unmap_a7ispmem(void *listmem, void **vaddr, int size)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;

    ion_unmap_iommu(dev->client, dev->handle);
    ion_free(dev->client, dev->handle);

    return 0;
}

void dump_mem(void *addr, int size)
{
    int i;
    pr_err("[%s] addr.%pK, size.0x%x\n", __func__, addr, size);
    for (i = 0; i < size; i += 16)
        printk("0x%08x 0x%08x 0x%08x 0x%08x\n", __raw_readl(addr + i + 0), __raw_readl(addr + i + 4), __raw_readl(addr + i + 8), __raw_readl(addr + i + 12));
}

void *getsec_a7sharedmem_addr(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    return dev->shrdmem->a7mapping[A7SHARED].apva;
}

unsigned long long get_nonsec_pgd(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    return dev->phy_pgd_base;
}

int get_ispops(struct hisi_isp_ops **ops)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;

    *ops = dev->ispops;

    return 0;
}

int hisp_sec_jpeg_powerdn(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    int ret = 0;

    if (!dev->ispops) {
        pr_err("[%s] Failed : ispops.%pK\n", __func__, dev->ispops);
        return -EINVAL;
    }

    if (dev->ispops->ispexit)
        if ((ret = dev->ispops->ispexit()) < 0)
            pr_err("[%s] Failed : ispexit.%d\n", __func__, ret);

    if (dev->ispops->ispsrtdn)
        if ((ret = dev->ispops->ispsrtdn()) < 0)
            pr_err("[%s] Failed : ispsrtdn.%d\n", __func__, ret);

    return 0;
}

int hisp_a7isp_powerdn(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    int ret = 0;

    if (!dev->ispops) {
        pr_err("[%s] Failed : ispops.%pK\n", __func__, dev->ispops);
        return -EINVAL;
    }

    if (dev->ispops->a7exit)
        if ((ret = dev->ispops->a7exit()) < 0)
            pr_err("[%s] Failed : a7exit.%d\n", __func__, ret);

    return (ret | hisp_jpeg_powerdn());
}

int hisp_sec_jpeg_powerup(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    int ret = 0;

    if (!dev->ispops) {
        pr_err("[%s] Failed : ispops.%pK\n", __func__, dev->ispops);
        return -EINVAL;
    }

    if (dev->ispops->ispsrtup) {
        if ((ret = dev->ispops->ispsrtup()) < 0) {
            pr_err("[%s] Failed : ispsrtup.%d\n", __func__, ret);
            return ret;
        }
    }

    if (dev->ispops->ispinit) {
        if ((ret = dev->ispops->ispinit()) < 0) {
            pr_err("[%s] Failed : ispinit.%d\n", __func__, ret);
            goto err_jpegup;
        }
    }

    return 0;

err_jpegup:
    if (dev->ispops->ispsrtdn)
        if (dev->ispops->ispsrtdn() < 0)
            pr_err("[%s] Failed : err_jpegup ispsrtdn\n", __func__);

    return ret;
}

int hisp_a7isp_powerup(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    int ret = 0;

    if (!dev->ispops) {
        pr_err("[%s] Failed : ispops.%pK\n", __func__, dev->ispops);
        return -EINVAL;
    }

    if ((ret = hisp_jpeg_powerup()) < 0) {
        pr_err("[%s] Failed : hisp_jpeg_powerup.%d\n", __func__, ret);
        return ret;
    }

    if (dev->ispops->a7init) {
        if ((ret = dev->ispops->a7init()) < 0) {
            pr_err("[%s] Failed : a7init.%d\n", __func__, ret);
            goto err_a7up;
        }
    }

    if (dev->ispops->loadfw) {
        if ((ret = dev->ispops->loadfw()) < 0) {
            pr_err("[%s] Failed : loadfw.%d\n", __func__, ret);
            goto err_loadfw;
        }
    }

    return 0;

err_loadfw:
    if (dev->ispops->a7exit)
        if (dev->ispops->a7exit() < 0)
            pr_err("[%s] Failed : err_loadfw a7exit\n", __func__);
err_a7up:
    if (hisp_jpeg_powerdn() < 0)
        pr_err("[%s] Failed : err_a7up hisp_jpeg_powerdn\n", __func__);

    return ret;
}

static int bsp_read_bin(const char *partion_name, unsigned int offset,
                unsigned int length, char *buffer)
{
    int ret          = -1;
    char *pathname   = NULL;
    unsigned long pathlen;
    struct file *fp;
    mm_segment_t fs;
    loff_t pos = 0;

    if ((NULL == partion_name) || (NULL == buffer)) {
        pr_err("partion_name(%pK) or buffer(%pK) is null", partion_name, buffer);
        return -1;
    }

    /*get resource*/
    pathlen = sizeof(DEVICE_PATH) + strnlen(partion_name, (unsigned long)PART_NAMELEN);
    pathname = kmalloc(pathlen, GFP_KERNEL);
    if (!pathname) {
        pr_err("pathname malloc failed\n");
        return -1;
    }

    ret = flash_find_ptn((const char *)partion_name, pathname);
    if (ret < 0) {
        pr_err("partion_name(%s) is not in partion table!\n", partion_name);
        goto error;
    }

    fp = filp_open(pathname, O_RDONLY, 0600);
    if (IS_ERR(fp)) {
        pr_err("filp_open(%s) failed", pathname);
        goto error;
    }

    ret = vfs_llseek(fp, offset, SEEK_SET);
    if (ret < 0) {
        pr_err("seek ops failed, ret %d", ret);
        goto error2;
    }

    fs = get_fs();
    set_fs(KERNEL_DS);/*lint !e501 */

    pos = fp->f_pos;/*lint !e613 */
    ret = vfs_read(fp, (char __user *)buffer, length, &pos);/*lint !e613 */
    if (ret != length) {
        pr_err("read ops failed, ret=%d(len=%d)", ret, length);
        set_fs(fs);
        goto error2;
    }
    set_fs(fs);

    filp_close(fp, NULL);/*lint !e668 */

    /*free resource*/
    if(NULL != pathname) {
       kfree(pathname);
       pathname = NULL;
    }

    return 0;

error2:
    filp_close(fp, NULL);/*lint !e668 */

error:
    if(NULL != pathname) {
       kfree(pathname);
       pathname = NULL;
    }

    pr_err("failed");
    return -1;
}

typedef enum {
    ISPIMG_DTS = 0,
    ISPIMG_RSC,
    ISPIMG_FW,
    ISPIMG_MAX
} ISPIMG_TYPE_E;

typedef enum {
    ISPPWR_JPEGUP = 0,
    ISPPWR_JPEGDN,
    ISPPWR_A7UP,
    ISPPWR_A7DN,
    ISPPWR_MAX
} ISPPWR_TYPE_E;

struct hisi_isp_loadimg_s {
    unsigned int addr;
    unsigned int size;
    char *partition;
} loadimg[] = {
    {0x0,   MEM_ISPDTS_SIZE,    "isp_dts"},
    {MEM_RSCTABLE_ADDR_OFFSET,   MEM_RSCTABLE_SIZE,    "isp_firmware"},
    {0x0,   MEM_ISPFW_SIZE,     "isp_firmware"}
};

static int atfisp_loadfw(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    struct load_image_info loadinfo;
    int ret = 0;

    loadinfo.ecoretype      = ISP;
    loadinfo.image_addr     = loadimg[ISPIMG_FW].addr;
    loadinfo.image_size     = loadimg[ISPIMG_FW].size;
    loadinfo.partion_name   = loadimg[ISPIMG_FW].partition;

    pr_info("[%s] + %s.(0x%lx, 0x%x), init.%x\n", __func__, loadinfo.partion_name, loadinfo.image_addr, loadinfo.image_size, dev->ispops->refs_fw);
    if ((ret = bsp_load_and_verify_image(&loadinfo)) < 0) {
        pr_err("[%s] Failed : bsp_load_and_verify_image.%d, %s.(0x%lx, 0x%x)\n", __func__, ret, loadinfo.partion_name, loadinfo.image_addr, loadinfo.image_size);
        return ret;
    }

    dev->ispops->refs_fw++;

    pr_info("[%s] - bsp_load_and_verify_image.%d, %s.(0x%lx, 0x%x), init.%x\n", __func__, ret, loadinfo.partion_name, loadinfo.image_addr, loadinfo.image_size, dev->ispops->refs_fw);
	return 0;
}

#define HISP_PARAMS_SHRDLIST    (0xCCCE0000)
#define HISP_PARAMS_DUMP        (0xCCCE0006)
#define HISP_PARAMS_NSECDUMP    (0xCCCE0007)

#define RSC_OFFSET              (0x00003000)
#define RSC_SIZE                (0x00001000)
#define LISTENTRY_SIZE          (0x00140000)

int sec_setclkrate(unsigned int type, unsigned int rate)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    int ret = 0;

    if (!dev->ispops) {
        pr_info("[%s] Failed : dev->ispops.%pK\n", __func__, dev->ispops);
        return -ENXIO;
    }

    mutex_lock(&dev->pwrlock);
    if (dev->ispops->refs_fw == 0) {
        pr_info("[%s] Failed : refs_fw.%d, check ISPCPU PowerDown\n", __func__, dev->ispops->refs_fw);
        mutex_unlock(&dev->pwrlock);
        return -ENODEV;
    }

    if ((ret = secnsec_setclkrate(type, rate)) < 0)
        pr_info("[%s] Failed : secnsec_setclkrate.%d\n", __func__, ret);
    mutex_unlock(&dev->pwrlock);

    return ret;
}

void atfisp_set_nonsec(void)
{
    atfd_hisi_service_isp_smc(ISP_FN_SET_NONSEC, 0, 0, 0);
}

static int atfisp_reset_ispcpu(void)
{
    int ret =0;
    ret = atfd_hisi_service_isp_smc(ISP_FN_RESET_A7, 0, 0, 0);
    if(ret < 0)
        pr_err("[%s] Failed : ISPCPU RESET ERROR!\n", __func__);
    else
        pr_info("[%s] ISPCPU RESET SUUCESS!\n", __func__);
    return ret;
}

void atfisp_ispfe_powerup(void)
{
    atfd_hisi_service_isp_smc(ISP_FN_ISPFE_PWU, 0, 0, 0);
}
void atfisp_disreset_a7(u64 remap_addr)
{
    atfd_hisi_service_isp_smc(ISP_FN_DISRESET_A7, remap_addr, 0, 0);
}

static int atfisp_setparams(unsigned int type, unsigned int param1, unsigned int param2)
{
    atfd_hisi_service_isp_smc(ISP_FN_SET_PARAMS, type, param1, param2);
    return 0;
}

static int atfisp_module_init(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;

    pr_info("[%s] + refs.%d\n", __func__, dev->ispops->refs_isp_module);
    atfd_hisi_service_isp_smc(ISP_FN_ISP_INIT, dev->phy_pgd_base, 0, 0);
    dev->ispops->refs_isp_module++;
    pr_info("[%s] - refs.%d\n", __func__, dev->ispops->refs_isp_module);

    return 0;
}

static int atfisp_module_exit(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;

    pr_info("[%s] + refs.%d\n", __func__, dev->ispops->refs_isp_module);
    atfd_hisi_service_isp_smc(ISP_FN_ISP_EXIT, 0, 0, 0);
    dev->ispops->refs_isp_module--;
    pr_info("[%s] - refs.%d\n", __func__, dev->ispops->refs_isp_module);

    return 0;
}

static int atfa7_module_init(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    u64 remap = 0;
    pr_info("[%s] + refs.%d\n", __func__, dev->ispops->refs_a7_module);

    remap = get_a7remap_addr();
    atfd_hisi_service_isp_smc(ISP_FN_A7_INIT, remap, 0, 0);

    dev->ispops->refs_a7_module++;
    pr_info("[%s] - refs.%d\n", __func__, dev->ispops->refs_a7_module);

    return 0;
}

static int atfa7_module_exit(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    unsigned int type;

    pr_info("[%s] + refs.%d\n", __func__, dev->ispops->refs_a7_module);
    type = 1;
    atfd_hisi_service_isp_smc(ISP_FN_A7_EXIT, type, 0, 0);
    dev->ispops->refs_a7_module--;
    pr_info("[%s] - refs.%d\n", __func__, dev->ispops->refs_a7_module);

    return 0;
}

struct hisi_isp_ops atfisp_ops = {
    .ispsrtup   = hisp_powerup,
    .ispsrtdn   = hisp_powerdn,
    .ispinit    = atfisp_module_init,
    .ispexit    = atfisp_module_exit,
    .a7init     = atfa7_module_init,
    .a7exit     = atfa7_module_exit,
    .loadfw     = atfisp_loadfw,
    .setparams  = atfisp_setparams,
};

int secisp_device_enable(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;

    if (!dev->secisp_kthread) {
        pr_err("[%s] Failed : secisp_kthread.%pK\n", __func__, dev->secisp_kthread);
        return -ENXIO;
    }

    dev->secisp_wake = 1;
    wake_up(&dev->secisp_wait);

    return 0;
}

static int do_secisp_device_enable(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    int ret, err_ret;

    if (!dev->ispops) {
        pr_err("[%s] Failed : ispops.%pK\n", __func__, dev->ispops);
        return -ENXIO;
    }

    if (dev->ispops->ispsrtup) {
        if ((ret = dev->ispops->ispsrtup()) < 0) {
            pr_err("[%s] Failed : ispsrtup.%d\n", __func__, ret);
            return ret;
        }
    }

    if (dev->ispops->ispinit) {
        if ((ret = dev->ispops->ispinit()) < 0) {
            pr_err("[%s] Failed : atfisp_module_init.%d\n", __func__, ret);
            goto err_ispinit;
        }
    }

    if ((ret = isp_a7_qos_cfg()) < 0) {
        pr_err("[%s] Failed : isp_a7_qos_cfg.%d\n", __func__, ret);
        goto err_ispinit;
    }

    if (dev->ispops->a7init) {
        if ((ret = dev->ispops->a7init()) < 0) {
            pr_err("[%s] Failed : atfa7_module_init.%d\n", __func__, ret);
            goto err_a7init;
        }
    }

    if (dev->ispops->loadfw) {
        if ((ret = dev->ispops->loadfw()) < 0) {
            pr_err("[%s] Failed : atfisp_loadfw.%d\n", __func__, ret);
            goto err_loadfw;
        }
    }

    return 0;

err_loadfw:
    if (dev->ispops->a7exit) {
        if ((err_ret = dev->ispops->a7exit()) < 0)
            pr_err("[%s] Failed : a7exit.%d\n", __func__, err_ret);
    }
err_a7init:
    if (dev->ispops->ispexit) {
        if ((err_ret = dev->ispops->ispexit()) < 0)
            pr_err("[%s] Failed : ispexit.%d\n", __func__, err_ret);
    }
err_ispinit:
    if (dev->ispops->ispsrtdn) {
        if ((err_ret = dev->ispops->ispsrtdn()) < 0)
            pr_err("[%s] Failed : ispsrtdn.%d\n", __func__, err_ret);
    }

    return ret;
}

int secisp_device_disable(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    struct cpumask cpu_mask;
    int ret, cpu_no;

    if (!dev->ispops) {
        pr_err("[%s] Failed : ispops.%pK\n", __func__, dev->ispops);
        return -ENXIO;
    }

    cpumask_clear(&cpu_mask);
    for (cpu_no = 1; cpu_no < 4; cpu_no++) {
        cpumask_set_cpu(cpu_no, &cpu_mask);
    }

    if(sched_setaffinity(current->pid, &cpu_mask) < 0)
        pr_err("%s: Couldn't set affinity to cpu\n", __func__);

    mutex_lock(&dev->pwrlock);
    if (dev->ispops->a7exit) {
        if ((ret = dev->ispops->a7exit()) < 0)
            pr_err("[%s] Failed : a7exit.%d\n", __func__, ret);
    }

    if (dev->ispops->ispexit) {
        if ((ret = dev->ispops->ispexit()) < 0)
            pr_err("[%s] Failed : ispexit.%d\n", __func__, ret);
    }

    if (dev->ispops->ispsrtdn) {
        if ((ret = dev->ispops->ispsrtdn()) < 0)
            pr_err("[%s] Failed : ispsrtdn.%d\n", __func__, ret);
    }

    dev->ispops->refs_fw--;
    mutex_unlock(&dev->pwrlock);

    dev->secisp_stop_kthread = kthread_create(free_secmem_ion, NULL, "secispmemfree");
    if (IS_ERR(dev->secisp_stop_kthread)) {
        pr_err("[%s] Failed : kthread_create.%ld\n", __func__, PTR_ERR(dev->secisp_stop_kthread));
        goto out;
    }
    wake_up_process(dev->secisp_stop_kthread);

out:

    if (sync_isplogcat() < 0)
        pr_err("[%s] Failed: sync_isplogcat\n", __func__);

    return 0;
}

static unsigned long get_a7shared_pa(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;

    return dev->atfshrd_paddr + AP_A7_SHARE_OFFSET;
}

static void *get_a7shared_va(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;

    return dev->atfshrd_vaddr + AP_A7_SHARE_OFFSET;
}

static unsigned long hisp_getreservemem(unsigned int etype, unsigned long paddr)
{
    unsigned long addr = 0;

    switch(etype) {
        case A7VQ:
        case A7VRING0:
        case A7VRING1:
            addr = paddr;
            break;
        case A7RDR:
            addr = get_isprdr_addr();
            pr_info("[%s] A7RDR.0x%lx\n", __func__, addr);
            break;
        case A7SHARED:
            addr = get_a7shared_pa();
            pr_info("[%s] A7SHARED.0x%lx\n", __func__, addr);
            break;
        default:
            pr_debug("[%s] default : etype.0x%x addr.0x%lx\n", __func__, etype, addr);
            return 0;
    }

    return addr;
}

int hisp_meminit(unsigned int etype, unsigned long paddr)
{
    const char *propname[] = {
        "a7-vaddr-boot",
        "a7-vaddr-text",
        "a7-vaddr-data",
        "a7-vaddr-pgm",
        "a7-vaddr-pte",
        "a7-vaddr-rdr",
        "a7-vaddr-shrd",
        "a7-vaddr-vq",
        "a7-vaddr-vr0",
        "a7-vaddr-vr1",
        "a7-vaddr-heap",
        "a7-vaddr-a7dyna",
        "a7-vaddr-regisp",
        "a7-vaddr-regipcs",
        "a7-vaddr-regipcns",
        "a7-vaddr-regpctrl",
        "a7-vaddr-regsctrl",
        "a7-vaddr-regpcfg",
        "a7-vaddr-reggic",
        "a7-vaddr-regsysc",
        "a7-vaddr-reguart",
        "a7-vaddr-reggpio",
        "a7-vaddr-reggpio25",
        "a7-vaddr-regioc",
        "a7-vaddr-mdc",
    };
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    struct device_node *np = dev->device->of_node;
    unsigned int offset = 0, size = 0;
    void *addr = NULL, *vaddr = NULL;
    int ret, index;

    if (!np) {
        pr_err("[%s] Failed : np.%pK\n", __func__, np);
        return -EINVAL;
    }

    if (etype >= MAXA7MAPPING) {
        pr_err("[%s] Failed : etype.(0x%x >= 0x%x)\n", __func__, etype, MAXA7MAPPING);
        return -EINVAL;
    }

    if (dev->shrdmem == NULL) {
        pr_err("[%s] Failed : shrdmem.(%pK = %pK)\n", __func__, dev->shrdmem, dev->atfshrd_vaddr);
        return -ENOMEM;
    }

    if((ret = of_property_read_u32_array(np, propname[etype], (unsigned int *)(&dev->shrdmem->a7mapping[etype]), 5)) < 0) {
        pr_info("[%s] propname.%s, of_property_read_u32_array.%d\n",
            __func__, propname[etype], ret);
        return -EINVAL;
    }
    pr_info("[%s] propname.%s, of_property_read_u32_array.%d.(0x%x, 0x%x, 0x%x, 0x%x, 0x%x)\n", __func__, propname[etype], ret,
    dev->shrdmem->a7mapping[etype].a7va, dev->shrdmem->a7mapping[etype].size, dev->shrdmem->a7mapping[etype].prot, dev->shrdmem->a7mapping[etype].offset, dev->shrdmem->a7mapping[etype].reserve);

    if (dev->shrdmem->a7mapping[etype].reserve) {
        dev->shrdmem->a7mapping[etype].a7pa = hisp_getreservemem(etype, paddr);
        if (etype ==  A7SHARED) {
            dev->shrdmem->a7mapping[etype].apva = get_a7shared_va();
        }
    } else {
        index = 5;
        if((ret = of_property_read_u32_index(np, propname[etype], index, &offset)) < 0) {
            pr_err("[%s] Failed : propname.%s, %d.of_property_read_u32_index.%d\n", __func__, propname[etype], index, ret);
            return -EINVAL;
        }

        index = 6;
        if((ret = of_property_read_u32_index(np, propname[etype], index, &size)) < 0) {
            pr_err("[%s] Failed : propname.%s, %d.of_property_read_u32_index.%d\n", __func__, propname[etype], index, ret);
            return -EINVAL;
        }

        dev->shrdmem->a7mapping[etype].a7pa = dev->atfshrd_paddr + offset;
        addr = offset + dev->atfshrd_vaddr;
        pr_err("[%s] atfshrd_vaddr.0x%llx, addr.%pK, vaddr.%pK, offset.0x%x, size.0x%x\n", __func__, (long long unsigned int)dev->atfshrd_vaddr, addr, vaddr, offset, size);
        switch (etype) {
            case A7DYNAMIC:
                dev->ap_dyna_array = addr;
                dev->ap_dyna = (struct hisi_a7mapping_s *)&dev->shrdmem->a7mapping[etype];
                pr_err("[%s] dyna_array.%pK\n", __func__, dev->ap_dyna_array);
                break;
            default :
                if ((ret = hisp_getsglist(addr, &vaddr, size)) < 0) {
                    pr_err("[%s] hisp_getsglist.%pK\n", __func__, vaddr);
                    return -ENOMEM;
                }
                dev->shrdmem->a7mapping[etype].apva = vaddr;
                pr_err("[%s] etype.0x%x, addr.%pK, vaddr.%pK, size.0x%x\n", __func__, etype, addr, vaddr, size);
                break;
        }
    }

    return 0;
}

int hisp_apisp_map(unsigned int *a7addr, unsigned int *ispaddr, unsigned int size)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;

    if ((dev->a7va_a7ispmem == 0) || (dev->ispva_a7ispmem == 0))
        return -EINVAL;

    *a7addr = dev->a7va_a7ispmem;
    *ispaddr = dev->ispva_a7ispmem;
    pr_info("[%s] X a7va.(0x%x, 0x%x), ispva.(0x%x, 0x%x)\n", __func__, *a7addr, dev->a7va_a7ispmem, *ispaddr, dev->ispva_a7ispmem);

    return 0;
}
EXPORT_SYMBOL(hisp_apisp_map);

int hisp_apisp_unmap(void)
{
    return 0;
}
EXPORT_SYMBOL(hisp_apisp_unmap);

static int hisp_rsctablemem_init(struct hisi_atfisp_s *dev)
{
    dma_addr_t dma_addr = 0;

    if ((dev->rsctable_vaddr = dma_alloc_coherent(dev->device, RSC_SIZE, &dma_addr, GFP_KERNEL)) == NULL) {
        pr_err("[%s] rsctable_vaddr.%pK\n", __func__, dev->rsctable_vaddr);
        return -ENOMEM;
    }
    dev->rsctable_paddr = (unsigned long long)dma_addr;

    pr_info("[%s] [A7RSC].paddr.0x%llx\n", __func__, dev->rsctable_paddr);

    return 0;
}

int set_ispcpu_reset(void)
{
    int ret = 0;
    ret = atfisp_reset_ispcpu();
    if(ret < 0)
        return -ENODEV;
    else
        return 0;
}

int get_ispcpu_cfg_info(void)
{
    return 0;
}
struct hisi_isp_fstcma_mdc_s *get_fstcma_mdc(unsigned int size)
{
    struct hisi_isp_fstcma_mdc_s *fstcma_mdc = NULL;

    fstcma_mdc = kzalloc(sizeof(struct hisi_isp_fstcma_mdc_s), GFP_KERNEL);/*lint !e838 */
    if (!fstcma_mdc) {
        pr_err("in %s fstcma_mdc kzalloc is failed\n", __func__);
        return NULL;
    }

    if ((fstcma_mdc->mdc_virt_addr = hisi_fstcma_alloc(&fstcma_mdc->mdc_dma_addr, size, GFP_KERNEL)) == NULL) {
        pr_err("[%s] mdc_virt_addr.%pK\n", __func__, fstcma_mdc->mdc_virt_addr);
        kfree(fstcma_mdc);
        return NULL;
    }

    return fstcma_mdc;
}
static int hisp_sharedmem_init(struct hisi_atfisp_s *dev)
{
    dma_addr_t dma_addr = 0;

    if ((dev->atfshrd_vaddr = hisi_fstcma_alloc(&dma_addr, LISTENTRY_SIZE, GFP_KERNEL)) == NULL) {
        pr_err("[%s] atfshrd_vaddr.%pK\n", __func__, dev->atfshrd_vaddr);
        return -ENOMEM;
    }
    dev->atfshrd_paddr = (unsigned long long)dma_addr;
    dev->shrdmem = (struct hisi_atfshrdmem_s *)dev->atfshrd_vaddr;

    return 0;
}


static int hisi_atf_getdts(struct platform_device *pdev)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    struct device *device = &pdev->dev;
    struct device_node *np = device->of_node;
    int ret;

    if (!np) {
        pr_err("[%s] Failed : np.%pK\n", __func__, np);
        return -ENODEV;
    }

    if ((ret = of_property_read_u32(np, "isp-iova-start", (unsigned int *)(&dev->isp_iova_start))) < 0 ) {
        pr_err("Failed : isp-iova_addr.0x%lx of_property_read_u32.%d\n", dev->isp_iova_start, ret);
        return -EINVAL;
    }

    pr_info("isp-iova_addr.0x%lx of_property_read_u32.%d\n", dev->isp_iova_start, ret);

    if ((ret = of_property_read_u32(np, "isp-iova-size", (unsigned int *)(&dev->isp_iova_size))) < 0 ) {
        pr_err("Failed : isp-iova_size.0x%lx of_property_read_u32.%d\n", dev->isp_iova_size, ret);
        return -EINVAL;
    }

    pr_info("isp-iova_size.0x%lx of_property_read_u32.%d\n", dev->isp_iova_size, ret);

    if ((ret = of_property_read_u32(np, "isp-wdt-flag", (unsigned int *)(&dev->isp_wdt_flag))) < 0 ) {
        pr_err("[%s] Failed: isp-wdt-flag of_property_read_u32.%d\n", __func__, ret);
        return -EINVAL;
    }

    pr_info("isp-wdt-flag.0x%x of_property_read_u32.%d\n", dev->isp_wdt_flag, ret);

    return 0;
}

void *get_rsctable(int *tablesz)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;

    *tablesz = RSC_SIZE;
    return dev->rsctable_vaddr;
}

void a7_map_set_pa_list(void *listmem, struct scatterlist *sg, unsigned int size)
{
    dma_addr_t dma_addr = 0;
    unsigned int len, set_size = 0;
    struct map_sglist_s *maplist = listmem;
    unsigned int last_counts = 0, last_len = 0;

    while (sg) {
        if ((dma_addr = sg_dma_address(sg)) == 0)
            dma_addr = sg_phys(sg);

        len = sg->length;
        if (len == 0) {
            pr_err("[%s] break len.0x%x\n", __func__, len);
            break;
        }
        set_size += len;
        if (set_size > size) {
            pr_err("[%s] break size.(0x%x > 0x%x), len.0x%x\n", __func__, set_size, size, len);
            maplist->addr = (unsigned long long)dma_addr;
            maplist->size = len - (set_size - size);
            break;
        }

        maplist->addr = (unsigned long long)dma_addr;
        maplist->size = len;
        if (last_len != len) {
            if (last_len != 0)
                pr_info("[%s] list.(%pK + %pK), maplist.(0x%llx, 0x%x X 0x%x)\n", __func__, listmem, maplist, (unsigned long long)dma_addr, last_counts, last_len);
            last_counts = 1;
            last_len = len;
        } else {
            last_counts ++;
        }
        maplist++;
        sg = sg_next(sg);
    }

    pr_info("[%s] list.(%pK + %pK), maplist.(0x%llx, 0x%x X 0x%x)\n", __func__, listmem, maplist, (unsigned long long)dma_addr, last_counts, last_len);
    pr_info("%s: size.0x%x == set_size.0x%x\n", __func__, size, set_size);
}
/*lint -save -e838 */
unsigned int a7_mmu_map(struct scatterlist *sgl, unsigned int size, unsigned int prot, unsigned int flag)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    unsigned int map_size = 0, map_prot;
    struct hisi_a7mapping_s *map_info = dev->ap_dyna;
    void *addr = dev->ap_dyna_array;
    unsigned long iova;

    if (size == 0) {
        pr_err("%s: invalid para, size == 0x%x.\n", __func__, size);
        return 0;
    }

    map_size = PAGE_ALIGN(size);

    if (prot == 0) {
        map_prot = map_info->prot;
    } else {
        map_prot = prot;
        map_info->prot = map_prot;
    }

    switch(flag) {
        case MAP_TYPE_DYNAMIC:
            iova = hisi_isp_alloc_iova(dev->isp_iova_pool, (unsigned long)map_size, (unsigned long)0);
            if(iova == 0){
                pr_err("[%s] Failed : hisi_isp_alloc_iova iova.0x%lx\n", __func__,iova);
                return 0;
            }
            map_info->a7va = (unsigned int)iova;
            map_info->size = map_size;

            break;
        case MAP_TYPE_STATIC:
            map_info->a7va = A7_MAP_BUFFER_ADDR;
            map_info->size = size;

            if (size > A7_MAP_BUFFER_SIZE) {
                pr_err("%s: invalid para, size == 0x%x.\n", __func__, size);
                return 0;
            }

            break;
        case MAP_TYPE_STATIC_SEC:
            map_info->a7va = IRIS_MAP_BUFFER_ADDR;
            map_info->size = size;

            if(size > IRIS_MAP_BUFFER_SIZE){
                pr_err("%s: invalid para, size == 0x%x.\n", __func__, size);
                return 0;
            }

            break;

        default:
            pr_err("[%s] Failed : type.0x%x\n", __func__, flag);
            return 0;
    }
    pr_info("[%s] type.0x%x, a7va.0x%x, size.0x%x, map_size.0x%x, map_prot.0x%x\n", __func__,
                            flag, map_info->a7va, size, map_size, map_prot);
    a7_map_set_pa_list(addr, sgl, map_size);

    atfd_hisi_service_isp_smc((unsigned long long)ISP_FN_A7_MAP, (unsigned long long)0, (unsigned long long)0, (unsigned long long)0);
    return map_info->a7va;
}

void a7_mmu_unmap(unsigned int va, unsigned int size)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    struct hisi_a7mapping_s *map_info = dev->ap_dyna;
    unsigned int map_size;

    map_size = PAGE_ALIGN(size);/*lint !e50 */
    pr_info("%s: va = 0x%x, size = 0x%x, map_size = 0x%x\n", __func__, va, size, map_size);
    map_info->a7va = va;
    map_info->size = map_size;

    atfd_hisi_service_isp_smc(ISP_FN_A7_UNMAP, 0, 0, 0);

    if((dev->isp_iova_start <= map_info->a7va) && (map_info->a7va < (dev->isp_iova_start + dev->isp_iova_size)))
    {
        pr_info("[%s] map_info->a7va.0x%x map_info->size.0x%x\n", __func__, map_info->a7va, map_info->size);
        hisi_isp_free_iova(dev->isp_iova_pool, (unsigned long)map_info->a7va, (unsigned long)map_info->size);
    }
}

static unsigned int get_size_align_mask(unsigned int align)
{
    unsigned int mask = 0xFFFFFFFF;
    unsigned int mask_num = 0;

    for(mask_num = 0; mask_num < 32; mask_num++)
    {
        if((0x1 & (align >> mask_num )) == 1)
        {
            return mask;
        }
        mask &= ~(1 << mask_num);/*lint !e747 !e701 !e502 */
    }
    return 0;
}

static unsigned long mem_pool_alloc_iova(struct gen_pool *pool,
        unsigned int size, unsigned long align)
{
    struct hisi_mem_pool_s *dev = (struct hisi_mem_pool_s *)&hisi_mem_pool_info;
    unsigned long iova = 0;

    if(pool == NULL)
        return 0;

    mutex_lock(&dev->mem_pool_mutex);

    iova = gen_pool_alloc(pool, (unsigned long)size);
    if(!iova){
        pr_err("hisi isp iommu gen_pool_alloc failed!\n");
        mutex_unlock(&dev->mem_pool_mutex);
        return 0;
    }

    if(align > ((unsigned long)1 << (unsigned long)(pool->min_alloc_order)))/*lint !e571 */
        pr_info("hisi iommu domain can't align to 0x%lx\n", align);
    mutex_unlock(&dev->mem_pool_mutex);
    return iova;
}

static void mem_pool_free_iova(struct gen_pool *pool,
        unsigned long iova, size_t size)
{
    struct hisi_mem_pool_s *dev = (struct hisi_mem_pool_s *)&hisi_mem_pool_info;

    mutex_lock(&dev->mem_pool_mutex);
    gen_pool_free(pool, iova, size);
    mutex_unlock(&dev->mem_pool_mutex);
}

static void mem_pool_destroy(unsigned int pool_num)
{
    struct hisi_mem_pool_s *dev = (struct hisi_mem_pool_s *)&hisi_mem_pool_info;
    struct mem_pool_info_s *pool_info = &dev->isp_pool_info[pool_num];
    struct mem_pool_info_list_s *listnode = pool_info->node;
    struct mem_pool_info_list_s *node = NULL;
    struct mem_pool_info_list_s *node_temp = NULL;
    struct hisi_atfisp_s *atf_dev  = NULL;
    struct hisi_a7mapping_s *map_info = NULL;

    if(pool_info->enable == 0){
        pr_err("%s: the pool_num %d not creat!\n",__func__, pool_num);
        return;
    }

    if(use_sec_isp()){
        atf_dev = (struct hisi_atfisp_s *)&atfisp_dev;
        map_info = atf_dev->ap_dyna;

        map_info->a7va = pool_info->addr;
        map_info->size = pool_info->size;

        atfd_hisi_service_isp_smc(ISP_FN_A7_UNMAP, 0, 0, 0);/*lint !e747 */
    }

    list_for_each_entry_safe(node, node_temp, &listnode->list, list){/*lint !e64 !e826 */
        mem_pool_free_iova(pool_info->isp_mem_pool, (unsigned long)node->addr, (unsigned long)node->size);
        list_del(&node->list);
        pr_info("%s: va = 0x%x, size = 0x%x\n", __func__, node->addr, node->size);
        kfree(node);
   }
    kfree(listnode);
    dev->count--;
    gen_pool_destroy(pool_info->isp_mem_pool);
    pool_info->enable = 0;
}

static unsigned int mem_pool_setup(struct scatterlist *sgl, unsigned int iova, \
    unsigned int size, \
    unsigned int prot, \
    unsigned int pool_num, \
    unsigned int align )
{
    struct hisi_mem_pool_s *dev = (struct hisi_mem_pool_s *)&hisi_mem_pool_info;
    struct mem_pool_info_s *pool_info = &dev->isp_pool_info[pool_num];
    struct mem_pool_info_list_s *listnode = NULL;
    struct gen_pool *pool = NULL;
    struct hisi_atfisp_s *atf_dev = NULL;
    struct hisi_a7mapping_s *map_info = NULL;
    unsigned int pool_addr = 0;
    unsigned int pool_size = 0;
    unsigned int map_prot = 0;
    unsigned int map_size = 0;
    unsigned int align_mask = 0;

    void *addr = NULL;
    int ret = 0;

    if(++(dev->count) > ISP_MEM_POOL_NUM){
        pr_err("Can't support so many pools!\n");
        return 0;
    }

    memset(pool_info,0,sizeof(struct mem_pool_info_s));/*lint !e838 */
    map_size = PAGE_ALIGN(size);/*lint !e50 */

    pool = gen_pool_create((int)(order_base_2(ISP_MEM_POOL_ALIGN)), -1);/*lint !e666 !e835 !e747 !e516 !e866 !e712 !e778*/
    if(!pool){
        pr_err("Create isp gen pool failed!\n");
        return 0;
    }

   if(use_sec_isp()){
        atf_dev = (struct hisi_atfisp_s *)&atfisp_dev;
        map_info = atf_dev->ap_dyna;

        map_prot = prot;
        pool_addr = (unsigned int)atf_dev->isp_iova_start;
        pool_size = map_size;
        switch(pool_num){
            case 0:
                pool_addr = (unsigned int)atf_dev->isp_iova_start;
                break;
            case 1:
                pool_addr = 0xC8000000;
                break;
            case 2:
                pool_addr = 0xD4000000;
                break;
            default:
                break;
        }

        addr = atf_dev->ap_dyna_array;
        map_info->a7va = pool_addr;
        map_info->size = pool_size;

        pr_debug("[%s] pool_num.%d, iova.0x%x, size.0x%x, map_size.0x%x, map_prot.0x%x\n", __func__,
                            pool_num, map_info->a7va, size, map_size, map_prot);

        a7_map_set_pa_list(addr, sgl, map_size);
        atfd_hisi_service_isp_smc((unsigned long long)ISP_FN_A7_MAP, (unsigned long long)0, (unsigned long long)0, (unsigned long long)0);

    }
   else {
        pool_addr = iova;

        align_mask = get_size_align_mask(align);
        if(pool_addr > (pool_addr & align_mask)){
            pool_addr &= align_mask;
            pool_addr += align;
        }
        pool_size = map_size;
   }

    ret = gen_pool_add(pool, (unsigned long)pool_addr, (unsigned long)pool_size, -1);
    if(ret){
        pr_err("Gen pool add failed!\n");
        mem_pool_destroy(pool_num);
        return 0;
    }

    listnode = kzalloc(sizeof(struct mem_pool_info_list_s), GFP_KERNEL);
    if (!listnode) {
        pr_err("%s: alloc listnode fail\n", __func__);
        mem_pool_destroy(pool_num);
        return 0;
    }
    listnode->addr = 0x0;
    listnode->size= 0x0;
    INIT_LIST_HEAD(&listnode->list);
    pool_info->node = listnode;
    pool_info->addr = pool_addr;
    pool_info->size = pool_size;
    pool_info->prot = prot;
    pool_info->isp_mem_pool = pool;
    pool_info->enable = 1;
    pr_debug("[%s] pool_num.%d, isp_mem_pool.0x%pK, pool.addr.0x%x, size.0x%x\n",__func__,pool_num,pool,pool_addr,pool_size);
    return pool_addr;
}
/*lint -save -e530 -e429*/
unsigned long hisp_mem_pool_alloc_iova(
    unsigned int size, \
    unsigned int pool_num
)
{
    struct hisi_mem_pool_s *dev = (struct hisi_mem_pool_s *)&hisi_mem_pool_info;
    struct mem_pool_info_list_s *node = NULL;
    struct mem_pool_info_s *pool_info = NULL;
    struct mem_pool_info_list_s *listnode = NULL;

    unsigned int map_size = 0;
    unsigned long iova;

    if (size == 0) {
        pr_err("%s: invalid para, size == 0x%x.\n", __func__, size);
        return 0;
    }

    if(pool_num >= ISP_MEM_POOL_NUM){
        pr_err("%s: wrong num, num : 0x%d\n",__func__, pool_num);
        return 0;
    }
    pool_info = &dev->isp_pool_info[pool_num];
    if(pool_info->enable == 0){
        pr_err("%s: the pool_num %d not creat!\n",__func__, pool_num);
        return 0;
    }
    listnode = pool_info->node;

    map_size = PAGE_ALIGN(size);/*lint !e50 */
    iova = mem_pool_alloc_iova(pool_info->isp_mem_pool, map_size, (unsigned long)0);
    if(iova == 0){
        pr_err("[%s] Failed : hisi_isp_alloc_iova iova.0x%lx\n", __func__,iova);
        return 0;
    }

    node = kzalloc(sizeof(struct mem_pool_info_list_s), GFP_KERNEL);
    if (!node) {
        pr_err("%s: alloc mem_pool_info_list_s fail\n", __func__);
        mem_pool_free_iova(pool_info->isp_mem_pool,iova,(unsigned long)map_size);
        return 0;
    }
    node->addr = (unsigned int)iova;
    node->size = map_size;
    list_add_tail(&node->list, &listnode->list);
    pr_info("[%s] pool.num.%d, iova.0x%lx, size.0x%x, map_size.0x%x\n", __func__,\
                            pool_num, iova, size, map_size);
    return iova;
}
/*lint -restore */
unsigned int hisp_mem_pool_free_iova(unsigned int pool_num, unsigned int va, unsigned int size)
{
    struct hisi_mem_pool_s *dev = (struct hisi_mem_pool_s *)&hisi_mem_pool_info;
    struct mem_pool_info_list_s *node = NULL;
    struct mem_pool_info_list_s *node_temp = NULL;
    struct mem_pool_info_s *pool_info = NULL;
    struct mem_pool_info_list_s *listnode = NULL;
    unsigned int map_size;

    if (size == 0) {
        pr_err("%s: invalid para, size == 0x%x.\n", __func__, size);
        return EINVAL;
    }

    if(pool_num >= ISP_MEM_POOL_NUM){
        pr_err("Mem pool num wrong!\n");
        return EBADF;
    }
    pool_info = &dev->isp_pool_info[pool_num];
    if(pool_info->enable == 0){
        pr_err("%s: the pool_num %d not creat!\n",__func__, pool_num);
        return EPERM;
    }
    listnode = pool_info->node;

    map_size = PAGE_ALIGN(size);/*lint !e50 */
    if((pool_info->addr <= va) && (va < (pool_info->addr + pool_info->size)))
    {
        list_for_each_entry_safe(node, node_temp, &listnode->list, list){/*lint !e64 !e826 */
            if (node->addr == va){
                mem_pool_free_iova(pool_info->isp_mem_pool, (unsigned long)node->addr, (unsigned long)node->size);
                list_del(&node->list);
                pr_info("%s: va = 0x%x, size = 0x%x, map_size = 0x%x\n", __func__, node->addr, node->size, map_size);
                kfree(node);
                return 0;
            }
        }
    }
    return ENXIO;
}

unsigned int hisp_mem_map_steup(struct scatterlist *sgl, unsigned int iova, \
    unsigned int size, \
    unsigned int prot, \
    unsigned int pool_num,  \
    unsigned int flag, \
    unsigned int align )
{
    unsigned int va;

    switch(flag) {
        case MAP_TYPE_DYNAMIC:
            if(pool_num >= ISP_MEM_POOL_NUM){
                pr_err("%s: wrong num, num : 0x%x\n",__func__, pool_num);
                return 0;
            }
            va = mem_pool_setup(sgl,iova,size,prot,pool_num,align);
            if(va == 0){
               pr_err("[%s] Failed : hisp_mem_pool_setup failed!\n",__func__);
               return 0;
            }
            break;
        case MAP_TYPE_STATIC_SEC:
            va = a7_mmu_map(sgl,size,prot,flag);
            if(va == 0){
               pr_err("[%s] Failed : mmu_map failed!\n",__func__);
               return 0;
            }
            break;
        default:
            pr_err("[%s] Failed : type.0x%x\n", __func__, flag);
            return 0;
    }
    return va;
}

void hisp_mem_pool_destroy(unsigned int pool_num)
{
    pr_info("%s: pool num = %d\n", __func__, pool_num);

    if(pool_num >= ISP_MEM_POOL_NUM){
        pr_err("%s: wrong num, num : 0x%x\n",__func__, pool_num);
        return;
    }
    mem_pool_destroy(pool_num);
}

void hisp_mem_pool_init(void)
{
    struct hisi_mem_pool_s *dev = (struct hisi_mem_pool_s *)&hisi_mem_pool_info;

    memset(dev,0x0,sizeof(hisi_mem_pool_info));/*lint !e838 */
    mutex_init(&dev->mem_pool_mutex);
}
/* MDC reserved memory, iova: 0xc3000000 */
unsigned int hisp_mem_pool_alloc_carveout(size_t size, unsigned int type)
{
    if((type == MAP_TYPE_DYNAMIC_CARVEOUT) && (size == MEM_MDC_SIZE))
        return MEM_MDC_DA;
    return 0;
}

int hisp_mem_pool_free_carveout(unsigned int  iova, size_t size)
{
    return 0;/* the free ops in the powerdn */
}
/*lint -restore */


int hisp_rsctable_init(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    int ret;

    pr_info("[%s] +\n", __func__);
    if (!dev->rsctable_vaddr) {
        pr_err("[%s] rsctable_vaddr.%pK\n", __func__, dev->rsctable_vaddr);
        return -ENOMEM;
    }

    if ((ret = bsp_read_bin("isp_firmware", RSC_OFFSET, RSC_SIZE, dev->rsctable_vaddr)) < 0) {
        pr_err("[%s] bsp_read_bin.%d\n", __func__, ret);
        return ret;
    }

    pr_info("[%s] -\n", __func__);

    return 0;
}

static u64 get_boot_sec_phyaddr(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    struct hisi_sec_ion_s *hisi_secmem_ion = NULL;

    if(dev->sec_mem_info){
        hisi_secmem_ion = dev->sec_mem_info;
        if(hisi_secmem_ion->sec_boot_phymem_addr)
            return hisi_secmem_ion->sec_boot_phymem_addr;
    }
    return 0;
}

static u64 get_fw_sec_phyaddr(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    struct hisi_sec_ion_s *hisi_secmem_ion = NULL;

    if(dev->sec_mem_info){
        hisi_secmem_ion = dev->sec_mem_info;
        if(hisi_secmem_ion->sec_fw_phymem_addr)
            return hisi_secmem_ion->sec_fw_phymem_addr;
    }
    return 0;
}

static unsigned long hisp_set_secmem_addr_pa(unsigned int etype, unsigned int vaddr, unsigned int offset)
{
    unsigned long addr = 0;
    unsigned long remap = 0;

    remap = (unsigned long)get_a7remap_addr();
    switch(etype) {
        case A7TEXT:
        case A7PGM:
        case A7PTE:
        case A7DATA:
            addr = (unsigned long)(remap + offset);
            break;
        case A7BOOT:
            addr = (unsigned long)(remap + offset);
            loadimg[ISPIMG_FW].addr = (unsigned int)addr;
            break;
        case A7HEAP:
            remap = (unsigned long)get_fw_sec_phyaddr();
            addr = (unsigned long)(remap + offset);
            break;
        case A7MDC:
            addr =  get_mdc_addr_pa();/*lint !e747 */
            if (!addr)
            {
                pr_err("[%s] get_mdc_addr_pa is NULL!\n", __func__);
                return 0;
            }
            set_shared_mdc_pa_addr(addr);
            break;
        case A7REGISP:
        case A7REGIPCS:
        case A7REGIPCNS:
        case A7REGPCTRL:
        case A7REGSCTRL:
        case A7REGPCFG:
        case A7REGGIC:
        case A7REGSYSC:
        case A7REGUART:
        case A7REGGPIO:
        case A7REGGPIO25:
        case A7REGIOC:
            addr = vaddr;
            break;
        default:
            pr_debug("[%s] Failed : etype.0x%x\n", __func__, etype);
            return 0;
    }

    return addr;
}

int hisp_secmem_pa_init(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    int index = 0;
    unsigned long addr = 0;

    for (index = 0; index < MAXA7MAPPING; index ++)
        if ((addr = hisp_set_secmem_addr_pa(index, dev->shrdmem->a7mapping[index].a7va, dev->shrdmem->a7mapping[index].offset)) != 0)
            dev->shrdmem->a7mapping[index].a7pa = addr;
    return 0;
}

static int smmu_err_addr_free(void){
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    struct rproc_shared_para *secisp_share_para = NULL;
    struct hisi_isp_ion_s *smmu_err_ion = dev->sec_smmuerr_ion;

    if(smmu_err_ion == NULL){
        return -ENOMEM;
    }

    free_nesc_addr_ion(smmu_err_ion);
    kfree(smmu_err_ion);
    dev->sec_smmuerr_ion = NULL;

    secisp_share_para = dev->sec_isp_share_para;
    if(secisp_share_para == NULL){
        pr_err("Faild: secisp_share_para\n");
        return -ENOMEM;
    }
    secisp_share_para->sec_smmuerr_addr = 0;
    return 0;
}

int smmu_err_addr_init(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    struct rproc_shared_para *secisp_share_para = NULL;

    size_t align = 12;/*lint !e838 */
    unsigned int heap_id_mask = (0x1 << 14);/*lint !e838 */
    unsigned int flags = (0x1 << 4);/*lint !e838 */
    /* alloc 16k */
    dev->sec_smmuerr_ion = get_nesc_addr_ion(0x4000, align, heap_id_mask, flags);/*lint !e747 */
    if(dev->sec_smmuerr_ion == NULL){
        pr_err("sec_smmuerr_ion failed!\n");
        return 0;
    }

    if(!dev->sec_smmuerr_ion->paddr){
        pr_err("dev->sec_smmuerr_ion.paddr failed!\n");
        return -ENOMEM;
    }
    secisp_share_para = dev->sec_isp_share_para;
    if(secisp_share_para == NULL){
        pr_err("Faild: secisp_share_para\n");
        return -ENOMEM;
        }
    secisp_share_para->sec_smmuerr_addr = dev->sec_smmuerr_ion->paddr;
    pr_info("[%s] sec_smmuerr_addr.0x%llx\n", __func__, dev->sec_smmuerr_ion->paddr);
    return 0;
}

int set_share_pararms(void)
{
    int ret = 0;
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    u64 remap = 0;

    remap = get_a7remap_addr();
    if(!remap){
        pr_err("[%s]ERR: remap addr.0x%llx err!\n", __func__, remap);
        return -ENODEV;
    }
    if ((ret = atfisp_setparams(HISP_PARAMS_SHRDLIST, dev->atfshrd_paddr, remap)) < 0) {
        pr_err("[%s] atfisp_setparams.%d, (type.0x%x, atfshrd_paddr.(0x%llx >> 0x%llx))\n", __func__, ret, HISP_PARAMS_SHRDLIST, dev->atfshrd_paddr, remap);
        kfree(dev->atfshrd_vaddr);
        return -ENODEV;
    }

    pr_info("[%s] atfisp_setparams.%d, (type.0x%x, atfshrd_paddr.(0x%llx >> 0x%llx))\n", __func__, ret, HISP_PARAMS_SHRDLIST, dev->atfshrd_paddr, remap) ;
    return 0;
}

static int secisp_work_fn(void *data)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    struct cpumask cpu_mask;
    int ret, cpu_no;

    pr_info("[%s] +\n", __func__);

    cpumask_clear(&cpu_mask);

    for (cpu_no = 1; cpu_no < 4; cpu_no++)
    {
        cpumask_set_cpu(cpu_no, &cpu_mask);
    }

    if(sched_setaffinity(current->pid, &cpu_mask) < 0)
    {
        pr_err("%s: Couldn't set affinity to cpu\n", __func__);
    }

    while (1) {
        if (kthread_should_stop())
            break;

        wait_event(dev->secisp_wait, dev->secisp_wake);

        if ((ret = hisp_secmem_init()) < 0) {
            pr_err("[%s] Failed : hisp_secmem_init.%d\n", __func__, ret);
        }

        if ((ret = set_isp_remap_addr(get_boot_sec_phyaddr())) < 0) {
            pr_err("[%s] Failed : set_isp_remap_addr.%d\n", __func__, ret);
        }

        if ((ret = hisp_secmem_pa_init()) < 0) {
            pr_err("[%s] Failed : hisp_secmem_init.%d\n", __func__, ret);
        }

        if ((ret = set_share_pararms()) < 0) {
            pr_err("[%s] Failed : set_share_pararms.%d\n", __func__, ret);
        }

        if ((ret = smmu_err_addr_init()) < 0) {
            pr_err("[%s] Failed : set_share_pararms.%d\n", __func__, ret);
        }

        mutex_lock(&dev->pwrlock);
        if ((ret = do_secisp_device_enable()) < 0) {
            pr_err("[%s] Failed : do_secisp_device_enable.%d.\n", __func__, ret);
        }
        mutex_unlock(&dev->pwrlock);
        dev->secisp_wake = 0;
    }
    pr_info("[%s] -\n", __func__);

    return 0;
}

/*lint -save -e429*/
static void sec_freemem_stop(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    if(dev->secisp_stop_kthread){
        kthread_stop(dev->secisp_stop_kthread);
    }
    dev->secisp_stop_kthread = NULL;
}
static int free_secmem_ion(void *data)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    struct hisi_sec_ion_s *hisi_secmem_ion = dev->sec_mem_info;
    int Ret = 0;

    pr_info("[%s] +\n", __func__);
    if(hisi_secmem_ion == NULL){
        pr_err("[%s] hisi_secmem_ion is NULL\n", __func__);
        return -ENODEV;
    }
    ion_free(hisi_secmem_ion->ion_client, hisi_secmem_ion->boot_ion_handle);
    ion_free(hisi_secmem_ion->ion_client, hisi_secmem_ion->fw_ion_handle);
    ion_client_destroy(hisi_secmem_ion->ion_client);
    hisi_secmem_ion->ion_client = NULL;
    hisi_secmem_ion->boot_ion_handle= NULL;
    hisi_secmem_ion->fw_ion_handle= NULL;
    kfree(hisi_secmem_ion);
    dev->sec_mem_info = NULL;

    if((Ret = smmu_err_addr_free()) < 0){
        pr_err("[%s] smmu_err_addr_free ERR\n", __func__);
    }
    sec_freemem_stop();

    while(1){
        msleep_interruptible(10);
        if (kthread_should_stop()){
            pr_info("[%s] -\n", __func__);
            return 0;
        }
    }
}

int hisp_secmem_init(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    struct hisi_sec_ion_s *hisi_secmem_ion = NULL;
    ion_phys_addr_t secmem_ion_phys = 0x0 ;
    size_t secmem_ion_size = 0;
    int ret = 0;

    pr_info("[%s] +\n", __func__);
    hisi_secmem_ion = kzalloc(sizeof(struct hisi_sec_ion_s), GFP_KERNEL);/*lint !e838 */
    if (!hisi_secmem_ion) {
        pr_err("in %s hisi_secmem_ion kzalloc is failed\n", __func__);
        return -ENODEV;
    }

    hisi_secmem_ion->ion_client = hisi_ion_client_create("secmem_ion_client");
    if (IS_ERR(hisi_secmem_ion->ion_client)) {
        pr_err("hisi hisi_secmem_ion_client create failed \n");
        kfree(hisi_secmem_ion);
        return -ENODEV;
    }
/* alloc sec boot mem */
    hisi_secmem_ion->boot_ion_handle= ion_alloc(hisi_secmem_ion->ion_client, MEM_SEC_BOOT_SIZE, PAGE_SIZE, (1 << ION_IRIS_HEAP_ID), ION_FLAG_SECURE_BUFFER);
    if (IS_ERR(hisi_secmem_ion->boot_ion_handle)){
        pr_err("hisi_secmem_ion_alloc failed \n");
        ion_client_destroy(hisi_secmem_ion->ion_client);
        kfree(hisi_secmem_ion);
        return -ENODEV;
    }
    ret = ion_phys(hisi_secmem_ion->ion_client, hisi_secmem_ion->boot_ion_handle,&secmem_ion_phys, &secmem_ion_size);/*lint !e838 */
    if (ret < 0)
    {
        pr_err("%s, failed to get phy addr,ret:%d!\n", __func__, ret);
        ion_free(hisi_secmem_ion->ion_client, hisi_secmem_ion->boot_ion_handle);
        ion_client_destroy(hisi_secmem_ion->ion_client);
        kfree(hisi_secmem_ion);
        return -ENODEV;
    }
    hisi_secmem_ion->sec_boot_phymem_addr = (u64)secmem_ion_phys;
/* alloc sec boot mem */
    hisi_secmem_ion->fw_ion_handle= ion_alloc(hisi_secmem_ion->ion_client, MEM_SEC_HEAP_SIZE, PAGE_SIZE, (1 << ION_IRIS_HEAP_ID), ION_FLAG_SECURE_BUFFER);
    if (IS_ERR(hisi_secmem_ion->fw_ion_handle)){
        pr_err("hisi_secmem_ion_alloc failed \n");
        ion_free(hisi_secmem_ion->ion_client, hisi_secmem_ion->boot_ion_handle);
        ion_client_destroy(hisi_secmem_ion->ion_client);
        kfree(hisi_secmem_ion);
        return -ENODEV;
    }

    ret = ion_phys(hisi_secmem_ion->ion_client, hisi_secmem_ion->fw_ion_handle,&secmem_ion_phys, &secmem_ion_size);/*lint !e838 */
    if (ret < 0)
    {
        pr_err("%s, failed to get phy addr,ret:%d!\n", __func__, ret);
        ion_free(hisi_secmem_ion->ion_client, hisi_secmem_ion->fw_ion_handle);
        ion_free(hisi_secmem_ion->ion_client, hisi_secmem_ion->boot_ion_handle);
        ion_client_destroy(hisi_secmem_ion->ion_client);
        kfree(hisi_secmem_ion);
        return -ENODEV;
    }
    hisi_secmem_ion->sec_fw_phymem_addr = (u64)secmem_ion_phys;
    pr_info("[%s] atf_boot_phymem.0x%llx,atf_fw_phymem.0x%llx\n", __func__,hisi_secmem_ion->sec_boot_phymem_addr,hisi_secmem_ion->sec_fw_phymem_addr);
    dev->sec_mem_info = hisi_secmem_ion;
    pr_info("[%s] -\n", __func__);

    return 0;
}
/*lint -restore */

void hisi_ispsec_share_para_set(void)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    isp_share_para = dev->sec_isp_share_para;
    pr_info("%s.%d: isp_share_para.%pK, dev->sec_isp_share_para.%pK, case.%u\n",
            __func__, __LINE__, isp_share_para, dev->sec_isp_share_para,
            hisi_isp_rproc_case_get());
}

int hisi_atfisp_probe(struct platform_device *pdev)
{
    unsigned long iova_start = 0;
    unsigned long iova_size = 0;
    unsigned long iova_align;
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    struct iommu_domain_data *info;
    int ret = 0, index = 0;

    dev->map_req_flag               = 0;
    dev->map_dts_flag               = 0;
    dev->device                     = &pdev->dev;
    atfisp_ops.refs_ispsrt_subsys   = UNINITIAL;
    atfisp_ops.refs_isp_module      = UNINITIAL;
    atfisp_ops.refs_a7_module       = UNINITIAL;
    atfisp_ops.refs_dts             = UNINITIAL;
    atfisp_ops.refs_fw              = UNINITIAL;
    dev->ispops                     = &atfisp_ops;

    if ((ret = hisi_atf_getdts(pdev)) < 0)
        pr_err("[%s] Failed : hisi_atf_getdts.%d.\n", __func__, ret);

    if ((ret = hisp_sharedmem_init(dev)) < 0)
        pr_err("[%s] Failed : hisp_sharedmem_init.%d.\n", __func__, ret);

    if ((ret = hisp_rsctablemem_init(dev)) < 0)
        pr_err("[%s] Failed : hisp_rsctablemem_init.%d.\n", __func__, ret);

    for (index = 0; index < MAXA7MAPPING; index ++)
        if ((ret = hisp_meminit(index, 0)) < 0)
            pr_err("Failed : index.%d, hisp_meminit.%d\n", index, ret);

    isp_share_para = (struct rproc_shared_para *)dev->shrdmem->a7mapping[A7SHARED].apva;
    dev->sec_isp_share_para = isp_share_para;

    if ((dev->domain = hisi_ion_enable_iommu(NULL)) == NULL) {
        pr_err("[%s] Failed : iommu_domain_alloc.%pK\n", __func__, dev->domain);
        return -ENODEV;
    }
    if(of_get_iova_info(pdev->dev.of_node, &iova_start, &iova_size, &iova_align)) {
        pr_err("[%s] Failed : dev get iommu info error.\n", __func__);
        return -ENODEV;
    }

    if ((info = (struct iommu_domain_data *)dev->domain->priv) == NULL) {
        pr_err("[%s] Failed : info.%pK\n",__func__, info);
        ret = -ENODEV;
        goto out;
    }
    dev->phy_pgd_base = info->phy_pgd_base;
    pr_info("[%s] info.iova.(0x%lx, 0x%lx) phy_pgd_base.0x%llx\n", __func__,
            iova_start, iova_size, dev->phy_pgd_base);
    mutex_init(&dev->isp_iova_pool_mutex);

    dev->isp_iova_pool = isp_iova_pool_setup((unsigned long)dev->isp_iova_start,
                (unsigned long)dev->isp_iova_size, 0x8000);

    if(dev->isp_iova_pool == NULL)
    {
        pr_err("[%s] Failed : isp_iova_pool.%pK\n",__func__,dev->isp_iova_pool);
        ret = -ENOMEM;
        goto out;
    }
    pr_info("[%s] sucessfully : isp_iova_pool.%pK\n",__func__,dev->isp_iova_pool);

    mutex_init(&dev->pwrlock);
    init_waitqueue_head(&dev->secisp_wait);
    dev->secisp_kthread = kthread_create(secisp_work_fn, NULL, "secispwork");
    if (IS_ERR(dev->secisp_kthread)) {
        pr_err("[%s] Failed : kthread_create.%ld\n", __func__, PTR_ERR(dev->secisp_kthread));
        ret = -1;
        isp_iova_pool_destroy(dev->isp_iova_pool);
        dev->isp_iova_pool = NULL;
        goto out;
    }

    wake_up_process(dev->secisp_kthread);

    return 0;

out:

    return ret;
}

int hisi_atfisp_remove(struct platform_device *pdev)
{
    struct hisi_atfisp_s *dev = (struct hisi_atfisp_s *)&atfisp_dev;
    dev->domain = NULL;

    return 0;
}

MODULE_DESCRIPTION("Hisilicon atfisp module");
MODULE_AUTHOR("chentao20@huawei.com");
MODULE_LICENSE("GPL");

void hisi_secisp_dump(void)
{
    atfisp_setparams(HISP_PARAMS_DUMP, 1, get_boot_sec_phyaddr());
}
