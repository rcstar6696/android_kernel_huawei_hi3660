#ifndef _IVP_PLATFORM_H_
#define _IVP_PLATFORM_H_

#include <linux/fs.h>
#include <linux/atomic.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

#define LISTENTRY_SIZE         (0x00600000)


struct ivp_iomem_res {
    char __iomem *cfg_base_addr;
    char __iomem *pctrl_base_addr;
    char __iomem *pericrg_base_addr;
    char __iomem *gic_base_addr;
};

struct ivp_sect_info {
    char name[64];
    unsigned int index;
    unsigned int len;
    unsigned int ivp_addr;
    unsigned int reserved;
    union {
        unsigned long acpu_addr;
        char compat32[8];
    };
};

struct ivp_device {
    struct ivp_iomem_res io_res;
    struct clk * clk;
    unsigned int clk_rate;
    unsigned int wdg_irq;
    atomic_t wdg_sleep;
    unsigned int dwaxi_dlock_irq;
    struct semaphore wdg_sem;
    int sect_count;
    struct ivp_sect_info *sects;
    struct dentry *debugfs;
    struct miscdevice device;
    struct regulator *regulator;
    unsigned long       smmu_pgd_base;
    struct ivp_smmu_dev *smmu_dev;
    atomic_t accessible;
    void *vaddr_memory;

    int ivp_meminddr_len;
    unsigned int dynamic_mem_size;
    unsigned int dynamic_mem_section_size;
};

extern int ivp_poweron_pri(struct ivp_device *ivp_devp);
extern int ivp_poweron_remap(struct ivp_device *ivp_devp);
extern int ivp_poweroff_pri(struct ivp_device *ivp_devp);
extern int ivp_init_pri(struct platform_device *pdev, struct ivp_device *ivp_devp);
extern void ivp_deinit_pri(struct ivp_device *ivp_devp);

#endif /* _IVP_PLATFORM_H_ */
