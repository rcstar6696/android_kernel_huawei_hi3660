#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/wakelock.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
#include <linux/clk-private.h>
#endif
#include <linux/clk-provider.h>
#include <linux/bitops.h>
#include "ivp.h"
#include "ivp_log.h"
#include "ivp_core.h"
#include "ivp_smmu.h"
#include "ivp_reg.h"
//lint -save -e750 -e753 -specific(-e753) -e750 -e528 -specific(-e528) -e144 -e82 -e64 -e785 -e715 -e712 -e40
//lint -save -e63 -e732 -e42 -e550 -e438 -e834 -e648 -e747 -e778 -e50
//lint -save -e749 -e84 -e866 -e514 -e846 -e737 -e574


#define IVP_WDG_REG_BASE_OFFSET          (0x1000)
#define IVP_SMMU_REG_BASE_OFFSET         (0x40000)
#define REMAP_ADD                        (0xe8d00000)
#define DEAD_FLAG                        (0xdeadbeef)
#define SIZE_16K                         (16 * 1024)
#define GIC_IRQ_CLEAR_REG                (0xe82b11a4)

static struct ivp_device ivp_dev;
extern struct dsm_client *client_ivp;
static struct wake_lock ivp_power_wakelock;
static struct mutex ivp_wake_lock_mutex;
static void *iram = NULL;

enum {
    IVP_BOOT_FROM_IRAM = 0,
    IVP_BOOT_FROM_DDR = 1,
};

enum {
    IVP_RUNSTALL_RUN = 0,
    IVP_RUNSTALL_STALL = 1,
};
enum {
    IVP_MEM_SLEEPMODE_NORMAL = 0,
    IVP_MEM_SLEEPMODE = 1,
    IVP_MEM_SLEEPMODE_DEEP = 2,
    IVP_MEM_SLEEPMODE_SHUTDOWN = 4,
    IVP_MEM_SLEEPMODE_MAX,
};

enum {
    IVP_DISABLE = 0,
    IVP_ENABLE  = 1,
};

/*lint -save -e30 -e31 -e64 -e142 -e438 -e529
-e530 -e550 -e695 -e712 -e713 -e715 -e732 -e744
-e747 -e774 -e778 -e785 -e838 -e1058 -e1564*/

inline void ivp_reg_write(unsigned int off, u32 val)
{
    char __iomem * reg = ivp_dev.io_res.cfg_base_addr + off;
    writel(val, reg);
}

//read ivp cfg reg
inline u32 ivp_reg_read(unsigned int off)
{
    char __iomem * reg = ivp_dev.io_res.cfg_base_addr + off;
    u32 val = readl(reg);
    return val;
}

inline void ivp_pericrg_reg_write(unsigned int off, u32 val)
{
    char __iomem * reg = ivp_dev.io_res.pericrg_base_addr + off;
    writel(val, reg);
}

//read ivp cfg reg
inline u32 ivp_pericrg_reg_read(unsigned int off)
{
    char __iomem * reg = ivp_dev.io_res.pericrg_base_addr + off;
    u32 val = readl(reg);
    return val;
}
/******************************************************
 *     read/write watch dog reg need unlock first     *
 *****************************************************/
inline void ivp_wdg_reg_write(unsigned int off, u32 val)
{
    char __iomem * reg = ivp_dev.io_res.cfg_base_addr + IVP_WDG_REG_BASE_OFFSET + off;
    writel(val, reg);
}

inline u32 ivp_wdg_reg_read(unsigned int off)
{
    char __iomem * reg = ivp_dev.io_res.cfg_base_addr + IVP_WDG_REG_BASE_OFFSET + off;
    u32 val = readl(reg);
    return val;
}

inline void ivp_smmu_reg_write(unsigned int off, u32 val)
{
    char __iomem * reg = ivp_dev.io_res.cfg_base_addr + IVP_SMMU_REG_BASE_OFFSET + off;
    writel(val, reg);
}

inline u32 ivp_smmu_reg_read(unsigned int off)
{
    char __iomem * reg = ivp_dev.io_res.cfg_base_addr + IVP_SMMU_REG_BASE_OFFSET + off;
    u32 val = readl(reg);
    return val;
}

inline u32 ivp_pctrl_reg_read(unsigned int off)
{
    char __iomem * reg = ivp_dev.io_res.pctrl_base_addr + off;
    u32 val = readl(reg);
    return val;
}

inline void ivp_gic_reg_write(unsigned int off, u32 val)
{
    /* char __iomem * reg = ivp_dev.io_res.gic_base_addr + off; */
    char __iomem * reg = ivp_dev.io_res.gic_base_addr;
    writel(val, reg);
}

inline u32 ivp_gic_reg_read(unsigned int off)
{
    /* char __iomem * reg = ivp_dev.io_res.gic_base_addr + off; */
    char __iomem * reg = ivp_dev.io_res.gic_base_addr;
    u32 val = readl(reg);
    return val;
}

inline void ivp_hw_clr_wdg_irq(void)
{
    //unlock reg
    ivp_wdg_reg_write(WDG_REG_OFF_WDLOCK, WDG_REG_UNLOCK_KEY);

    //clear irq
    ivp_wdg_reg_write(WDG_REG_OFF_WDINTCLR, 1);

    //disable irq
    ivp_wdg_reg_write(WDG_REG_OFF_WDCONTROL, 0);

    //lock reg
    ivp_wdg_reg_write(WDG_REG_OFF_WDLOCK, WDG_REG_LOCK_KEY);
}

inline void ivp_hw_set_ocdhalt_on_reset(struct ivp_device *devp, int mode)
{
    ivp_reg_write(IVP_REG_OFF_OCDHALTONRESET, (unsigned int)mode);
}

inline void ivp_hw_set_bootmode(struct ivp_device *devp, int mode)
{
    u32 val = (u32)mode;
    ivp_reg_write(IVP_REG_OFF_STATVECTORSEL, val & 0x01);
}

inline void ivp_hw_clockgate(struct ivp_device *devp, int state)
{
    u32 val = (u32)state;
    ivp_reg_write(IVP_REG_OFF_DSPCORE_GATE, val & 0x01);
}

inline void ivp_hw_enable_reset(struct ivp_device *devp)
{
    ivp_reg_write(IVP_REG_OFF_DSP_CORE_RESET_EN, 0x02);
    ivp_reg_write(IVP_REG_OFF_DSP_CORE_RESET_EN, 0x01);
    ivp_reg_write(IVP_REG_OFF_DSP_CORE_RESET_EN, 0x04);
}

inline void ivp_hw_disable_reset(struct ivp_device *devp)
{
    ivp_reg_write(IVP_REG_OFF_DSP_CORE_RESET_DIS, 0x04);
    ivp_reg_write(IVP_REG_OFF_DSP_CORE_RESET_DIS, 0x01);
    ivp_reg_write(IVP_REG_OFF_DSP_CORE_RESET_DIS, 0x02);
}

static void ivp_hw_runstall(struct ivp_device *devp, int mode)
{
    u32 val = (u32)mode;
    ivp_reg_write(IVP_REG_OFF_RUNSTALL, val & 0x01);
}

static int ivp_hw_query_runstall(struct ivp_device *devp)
{
    return (int)ivp_reg_read(IVP_REG_OFF_RUNSTALL);
}

static void ivp_hw_trigger_NMI(struct ivp_device *devp)
{
    //risedge triger.triger 0->1->0
    ivp_reg_write(IVP_REG_OFF_NMI, 0);
    ivp_reg_write(IVP_REG_OFF_NMI, 1);
    ivp_reg_write(IVP_REG_OFF_NMI, 0);
}

inline void ivp_hw_set_mem_sleepmode(int off, int mode, int mask)
{
    unsigned int old_mode = ivp_reg_read((u32)off);
    u32 val = (u32)mask;

    unsigned int new_mode = (old_mode & (~val)) | (u32)mode ;
    if (new_mode != old_mode) {
        ivp_dbg("mode changed.old:%#x, new:%#x", old_mode, new_mode);
        ivp_reg_write((u32)off, new_mode);
    }
}

static void ivp_hw_set_all_bus_gate_clock(int mode)
{
    if (mode > 2) {
        ivp_err("Invalid bus clk auto gate mode.");
        return;
    }

    if (IVP_ENABLE == mode) {
        ivp_reg_write(IVP_REG_OFF_BUS_GATE_CLOCK, 0xfff);
    } else {
        ivp_reg_write(IVP_REG_OFF_BUS_GATE_CLOCK, 0x0);
    }
}

static void ivp_hw_set_all_smmu_awake_bypass(int mode)
{
    if (mode > 2) {
        ivp_err("Invalid smmu awakebypass mode.");
        return;
    }

    if (mode) {
        ivp_reg_write(IVP_REG_OFF_SMMU_AWAKEBYPASS, 0xf);
    } else {
        ivp_reg_write(IVP_REG_OFF_SMMU_AWAKEBYPASS, 0x5);
    }
}

static void ivp_hw_set_peri_autodiv(int mode)
{
    unsigned int enable = 0;

    ivp_info("set mode to:%d", mode);
    enable = ivp_pericrg_reg_read(PERICRG_REG_OFF_PERI_AUTODIV0);
    if (IVP_ENABLE == mode) {
        ivp_pericrg_reg_write(PERICRG_REG_OFF_PEREN6, GATE_CLK_AUTODIV_IVP);
        ivp_pericrg_reg_write(PERICRG_REG_OFF_PERI_AUTODIV0,
                enable &
                ~(IVP_DW_AXI_M2_ST_BYPASS | IVP_DW_AXI_M1_ST_BYPASS |
                  IVP_PWAITMODE_BYPASS | IVP_DIV_AUTO_REDUCE_BYPASS)
                );

    } else {
        ivp_pericrg_reg_write(PERICRG_REG_OFF_PERDIS6, GATE_CLK_AUTODIV_IVP);
    }
}

inline int ivp_hw_query_waitmode(struct ivp_device *devp)
{
    return (int)ivp_reg_read(IVP_REG_OFF_PWAITMODE);
}

static void ivp_dev_set_dynamic_clk(int mode)
{
    if (mode) {
        //bus gate clock enable.
        ivp_hw_set_all_bus_gate_clock(IVP_ENABLE);
        //pericrg.
        ivp_hw_set_peri_autodiv(IVP_ENABLE);
        //smmu bypass enable.
        ivp_hw_set_all_smmu_awake_bypass(IVP_DISABLE);

    } else {
        //smmu bypass disable.
        ivp_hw_set_all_smmu_awake_bypass(IVP_ENABLE);
        //pericrg.
        ivp_hw_set_peri_autodiv(IVP_DISABLE);
        //bus gate clock disable.
        ivp_hw_set_all_bus_gate_clock(IVP_DISABLE);
    }
}

static int ivp_dev_poweron(struct ivp_device *devp)
{
    int ret = 0;

    mutex_lock(&ivp_wake_lock_mutex);
    if (!wake_lock_active(&ivp_power_wakelock)) {
        wake_lock(&ivp_power_wakelock);
        ivp_info("%s ivp power on enter, wake lock\n", __func__);
    }
    mutex_unlock(&ivp_wake_lock_mutex);/*lint !e456*/

    ret = ivp_poweron_pri(devp);
    if (ret) {
        ivp_err("power on pri setting failed [%d]!", ret);
        goto ERR_IVP_POWERON_PRI;
    }

    //set auto gate clk etc.
    ivp_dev_set_dynamic_clk(IVP_ENABLE);

    ret = ivp_poweron_remap(devp);
    if (ret) {
        ivp_err("power on remap setting failed [%d]!", ret);
        goto ERR_IVP_POWERON_REMAP;
    }

    //After reset, enter running mode
    ivp_hw_set_ocdhalt_on_reset(devp, 0);

    //Put ivp in stall mode
    ivp_hw_runstall(devp, IVP_RUNSTALL_STALL);
    //Reset ivp core
    ivp_hw_enable_reset(devp);

    //Boot from IRAM.
    ivp_hw_set_bootmode(devp, IVP_BOOT_FROM_IRAM);

    //Disable system reset, let ivp core leave from reset
    ivp_hw_disable_reset(devp);

    return ret;/*lint !e454*/

ERR_IVP_POWERON_REMAP:
    ivp_poweroff_pri(devp);

ERR_IVP_POWERON_PRI:
    mutex_lock(&ivp_wake_lock_mutex);
    if (wake_lock_active(&ivp_power_wakelock)) {
        wake_unlock(&ivp_power_wakelock);
        ivp_err("%s ivp power on failed, wake unlock\n", __func__);
    }
    mutex_unlock(&ivp_wake_lock_mutex);/*lint !e456*/

    return ret;/*lint !e454*/
}

static void ivp_dev_poweroff(struct ivp_device *devp)
{
    int ret = 0;

    ivp_hw_runstall(devp, IVP_RUNSTALL_STALL);

    ivp_hw_enable_reset(devp);

    ret = ivp_poweroff_pri(devp);
    if (ret) {
        ivp_err("power on private setting failed [%d]!", ret);
    }

    mutex_lock(&ivp_wake_lock_mutex);
    if (wake_lock_active(&ivp_power_wakelock)) {
        wake_unlock(&ivp_power_wakelock);
        ivp_info("%s ivp power off, wake unlock\n", __func__);
    }
    mutex_unlock(&ivp_wake_lock_mutex);/*lint !e456*/
}

static void ivp_dev_run(struct ivp_device *devp)
{
    ivp_dbg("enter");
    if (ivp_hw_query_runstall(devp) == IVP_RUNSTALL_RUN)
        return;
    ivp_hw_runstall(devp, IVP_RUNSTALL_RUN);
}

static void ivp_dev_stop(struct ivp_device *devp)
{
    ivp_dbg("enter");
    ivp_hw_runstall(devp, IVP_RUNSTALL_STALL);
}

static void ivp_dev_suspend(struct ivp_device *devp)
{
    unsigned long irq_status = 0;
    uint32_t wfi = 0;
    uint32_t binterrupt = 0;
    uint32_t wdg_enable = 0;

    ivp_dbg("enter %s", __func__);

    local_irq_save(irq_status);
    wfi = ivp_reg_read(IVP_REG_OFF_PWAITMODE);
    binterrupt = ivp_reg_read(IVP_REG_OFF_BINTERRUPT);
    wdg_enable = ivp_wdg_reg_read(WDG_REG_OFF_WDCONTROL);
    if ((wfi == 1) && (binterrupt == 0) && (wdg_enable == 0)) {
         ivp_hw_runstall(devp, IVP_RUNSTALL_STALL);
         local_irq_restore(irq_status);
         return;
    }
    local_irq_restore(irq_status);

    if (wfi == 1 && ((binterrupt != 0) || (wdg_enable != 0)))
        ivp_warn("Suspend on wrong status, binterrupt=%u wdgenable=%u",
                  binterrupt, wdg_enable);

}

static void ivp_dev_resume(struct ivp_device *devp)
{
    ivp_dbg("enter");

    if (ivp_hw_query_runstall(devp) == IVP_RUNSTALL_RUN)
        return;

    ivp_hw_runstall(devp, IVP_RUNSTALL_RUN);
}

static int ivp_dev_keep_on_wdg(struct ivp_device *devp)
{
    if (down_interruptible(&ivp_dev.wdg_sem)) {
        ivp_info("interrupt");
        return -EINTR;
    }

    if (atomic_read(&devp->wdg_sleep)) {
        ivp_info("watchdog sleeped");
        return -EAGAIN;
    }

    return 0;
}

static void ivp_dev_sleep_wdg(struct ivp_device *devp)
{
    atomic_set(&devp->wdg_sleep, 1);
    up(&devp->wdg_sem);
}

static int ivp_dev_smmu_reset(void) {
    unsigned int status = 0;
    int ret = 0;
    int count = 1000;

    ivp_info("enter");
    ivp_reg_write(IVP_REG_OFF_SMMU_RST_EN, BIT_SMMU_CRST_EN | BIT_SMMU_BRST_EN);
    udelay(10);
    ivp_reg_write(IVP_REG_OFF_SMMU_RST_DIS, BIT_SMMU_CRST_DIS | BIT_SMMU_BRST_DIS);

    while (count--) {
        status = ivp_reg_read(IVP_REG_OFF_SMMU_RST_ST);
        if ((status & BIT_SMMU_CRST_DIS) && (status & BIT_SMMU_BRST_DIS)) {
            break;
        }
        udelay(1);
    }

    if (count <= 0) {
        ret = -1;
        ivp_err("Reset smmu fail.");
    }

    return ret;
}

static int ivp_dev_smmu_init(struct ivp_device *pdev)
{
    struct ivp_smmu_dev *smmu_dev = pdev->smmu_dev;
    int ret = 0;

    if (NULL == smmu_dev) {
        ivp_err("Ivp smmu dev is NULL.");
        return -ENODEV;
    }

    //reset smmu
    ret = ivp_dev_smmu_reset();
    if (ret) {
        ivp_warn("Reset ret [%d]", ret);
    }

    //enable smmu
    ret = ivp_smmu_trans_enable(smmu_dev);
    if (ret) {
        ivp_warn("Enable trans ret [%d]", ret);
    }

    return ret;
}

static int ivp_dev_smmu_deinit(struct ivp_device *pdev)
{
    struct ivp_smmu_dev *smmu_dev = pdev->smmu_dev;
    int ret = 0;

    ivp_info("enter");
    if (NULL == smmu_dev) {
        ivp_err("Ivp smmu dev is NULL.");
        return -ENODEV;
    }

    ret = ivp_smmu_trans_disable(smmu_dev);
    if (ret) {
        ivp_err("Enable trans failed.[%d]", ret);
    }

    return ret;
}

static int ivp_dev_smmu_invalid_tlb(void)
{
    struct ivp_smmu_dev *smmu_dev = ivp_dev.smmu_dev;
    int ret = 0;

    ivp_info("enter");
    if (NULL == smmu_dev) {
        ivp_err("Ivp smmu dev is NULL.");
        return -ENODEV;
    }

    ret = ivp_smmu_invalid_tlb(smmu_dev, IVP_SMMU_CB_VMID_NS);
    if (ret) {
        ivp_err("invalid tbl fail.[%d]", ret);
    }

    return ret;
}

static irqreturn_t ivp_wdg_irq_handler(int irq, void *dev_id)
{
    struct ivp_device *pdev = (struct ivp_device *) dev_id;;
    ivp_warn("=========WDG IRQ Trigger=============");
    //Clear IRQ
    ivp_hw_clr_wdg_irq();

    up(&pdev->wdg_sem);
    ivp_warn("=========WDG IRQ LEAVE===============");

    if (!dsm_client_ocuppy(client_ivp)) {
        dsm_client_record(client_ivp, "ivp\n");
        dsm_client_notify(client_ivp, DSM_IVP_WATCH_ERROR_NO);
        ivp_info("[I/DSM] %s dsm_client_ivp_watach dog", client_ivp->client_name);
    }

    return IRQ_HANDLED;
}

static void ivp_parse_dwaxi_info(void)
{
    u32 val, bits_val;
    u32 offset;
    offset = PCTRL_REG_OFF_PERI_STAT4;
    val = ivp_pctrl_reg_read(offset);

    ivp_dbg("pctrl reg:%#x = %#x", offset, val);

    bits_val = (val & BIT(18)) >> 18;
    switch(bits_val) {
    case 0:
        ivp_warn("BIT[18] : %#x", bits_val);
        break;

    case 1:
        ivp_warn("BIT[18] : %#x", bits_val);
        break;
    }

    bits_val = (val & BIT(17)) >> 17;
    switch(bits_val) {
    case 0:
        ivp_warn("BIT[17] : %#x, dlock write", bits_val);
        break;

    case 1:
        ivp_warn("BIT[17] : %#x, dlock read", bits_val);
        break;
    }

    bits_val = (val & 0x1e000) >> 13;
    ivp_warn("ivp32 dlock id[%#x]", bits_val);

    bits_val = (val & 0x1c00) >> 10;
    ivp_warn("ivp32 dlock slv[%#x]", bits_val);

    offset = IVP_REG_OFF_SMMU_RST_ST;
    val = ivp_reg_read(offset);
    ivp_warn("ivp reg:%#x = %#x", offset, val);

    offset = IVP_REG_OFF_TIMER_WDG_RST_STATUS;
    val = ivp_reg_read(offset);
    ivp_warn("ivp:%#x = %#x", offset, val);

    offset = IVP_REG_OFF_DSP_CORE_RESET_STATUS;
    val = ivp_reg_read(offset);
    ivp_warn("ivp:%#x = %#x", offset, val);
}

static irqreturn_t ivp_dwaxi_irq_handler(int irq, void *dev_id)
{
    ivp_warn("==========DWAXI IRQ Trigger===============");
    ivp_warn("dwaxi triggled!SMMU maybe in reset status");
    //clear dwaxi irq
    ivp_gic_reg_write(GIC_REG_OFF_GICD_ICENABLERn, 0x80000);
    ivp_parse_dwaxi_info();
    ivp_warn("==========DWAXI IRQ LEAVE=================");

    if (!dsm_client_ocuppy(client_ivp)) {
         dsm_client_record(client_ivp, "ivp\n");
         dsm_client_notify(client_ivp, DSM_IVP_DWAXI_ERROR_NO);
         ivp_info("[I/DSM] %s dsm_client_ivp_dwaxi", client_ivp->client_name);
    }

    return IRQ_HANDLED;
}

static int ivp_init_resethandler(struct ivp_device *pdev)
{
    /* init code to remap address */
    iram = ioremap(REMAP_ADD, SIZE_16K);
    if (!iram) {
        ivp_err("Can't map ivp base address: 0x%x", REMAP_ADD);
        return -1;
    }

    iowrite32(DEAD_FLAG, iram);

    return 0;
}

static int ivp_check_resethandler(void)
{
    /* check init code in remap address */
    int inited = 0;
    uint32_t flag = ioread32(iram);
    if (flag != DEAD_FLAG)
        inited = 1;

    return inited;
}

static void ivp_deinit_resethandler(void)
{
    /* deinit remap address */
    if(iram) {
        iounmap(iram);
    }
}

static int ivp_open(struct inode *inode, struct file *fd)
{
    struct ivp_device *pdev = &ivp_dev;
    int ret = 0;

    if (atomic_read(&pdev->accessible) == 0) {
        ivp_err("maybe ivp dev has been opened!");
        return -EBUSY;
    }

    atomic_dec(&pdev->accessible);

    atomic_set(&pdev->wdg_sleep, 0);
    sema_init(&pdev->wdg_sem, 0);

    ret = ivp_dev_poweron(pdev);
    if (ret < 0) {
        ivp_err("Failed to power on ivp.");
        goto err_ivp_dev_poweron;
    }

    ret = ivp_dev_smmu_init(pdev);
    if (ret) {
        ivp_err("Failed to init smmu.");
        goto err_ivp_dev_smmu_init;
    }

    ret = request_irq(pdev->wdg_irq, ivp_wdg_irq_handler, 0, "ivp_wdg_irq", (void *)pdev);
    if (ret) {
        ivp_err("Failed to request wdg irq.%d", ret);
        goto err_request_irq_wdg;
    }

    ret = request_irq(pdev->dwaxi_dlock_irq, ivp_dwaxi_irq_handler, 0, "ivp_dwaxi_irq", (void *)pdev);
    if (ret) {
        ivp_err("Failed to request dwaxi irq.%d", ret);
        goto err_request_irq_dwaxi;
    }

    ret = ivp_init_resethandler(pdev);
    if (ret) {
        ivp_err("Failed to init reset handler.");
        goto err_ivp_init_resethandler;
    }

    fd->private_data = (void *)pdev;

    ivp_info("open ivp device sucess!");

    return ret;

err_ivp_init_resethandler:
    free_irq(pdev->dwaxi_dlock_irq, pdev);
err_request_irq_dwaxi:
    free_irq(pdev->wdg_irq, pdev);
err_request_irq_wdg:
    ivp_dev_smmu_deinit(pdev);
err_ivp_dev_smmu_init:
    ivp_dev_poweroff(pdev);
err_ivp_dev_poweron:
    atomic_inc(&pdev->accessible);

    if (!dsm_client_ocuppy(client_ivp)) {
         dsm_client_record(client_ivp, "ivp\n");
         dsm_client_notify(client_ivp, DSM_IVP_OPEN_ERROR_NO);
         ivp_info("[I/DSM] %s dsm_client_ivp_open", client_ivp->client_name);
    }

    ivp_info("open ivp device fail!");
    return ret;
}

static int ivp_release(struct inode *inode, struct file *fd)
{
    struct ivp_device *pdev = (struct ivp_device *)fd->private_data;
    if (NULL == pdev) {
        ivp_err("dev is NULL.");
        return -ENODEV;
    }

    if (atomic_read(&pdev->accessible) != 0) {
        ivp_err("maybe ivp dev not opened!");
        return -1;
    }

    ivp_deinit_resethandler();

    ivp_hw_runstall(pdev, IVP_RUNSTALL_STALL);
    if (ivp_hw_query_runstall(pdev) != IVP_RUNSTALL_STALL) {
        ivp_err("Failed to stall ivp.");
    }

    ivp_hw_clr_wdg_irq();
    free_irq(pdev->dwaxi_dlock_irq, pdev);
    free_irq(pdev->wdg_irq, pdev);
    ivp_dev_smmu_deinit(pdev);
    ivp_dev_poweroff(pdev);

    ivp_info("ivp device closed.");

    atomic_inc(&pdev->accessible);

    return 0;
}

static void ivp_dev_hwa_enable(void)
{
    ivp_info("ivp will enable hwa.");
    ivp_reg_write(IVP_REG_OFF_APB_GATE_CLOCK, 0x00003FFF);
    ivp_reg_write(IVP_REG_OFF_TIMER_WDG_RST_DIS, 0x0000007F);

    return;
}

static long ivp_ioctl(struct file *fd, unsigned int cmd, unsigned long args)
{
    long ret = 0;
    struct ivp_device *pdev = (struct ivp_device *)fd->private_data;

    switch (cmd) {
    case IVP_IOCTL_SECTCOUNT: {
        unsigned int sect_count = ivp_dev.sect_count;
        ivp_info("IOCTL:get sect count:%#x.", sect_count);
        ret = copy_to_user((void *)args, &sect_count, sizeof(sect_count));
        }
        break;

    case IVP_IOCTL_SECTINFO: {
        struct ivp_sect_info info;
        if (0 != copy_from_user(&info, (void *)args, sizeof(info))) {
            ivp_err("Invalid input param size.");
            return -EINVAL;
        }

        if (info.index >= ivp_dev.sect_count) {
            ivp_err("index is out of range.index:%u, sec_count:%u",
                     info.index, ivp_dev.sect_count);
            return -EINVAL;
        }

        ret = copy_to_user((void *)args, &ivp_dev.sects[info.index], sizeof(struct ivp_sect_info));
        }
        break;

    case IVP_IOCTL_DSP_RUN:
        if (ivp_check_resethandler() == 1) {
            ivp_dev_run(pdev);
        } else {
            ivp_err("ivp image not upload.");
        }

        break;

    case IVP_IOCTL_DSP_SUSPEND:
        ivp_dev_suspend(pdev);
        break;

    case IVP_IOCTL_DSP_RESUME:
        if (ivp_check_resethandler() == 1) {
            ivp_dev_resume(pdev);
        } else {
            ivp_err("ivp image not upload.");
        }
        break;

    case IVP_IOCTL_DSP_STOP:
        ivp_dev_stop(pdev);
        break;

    case IVP_IOCTL_QUERY_RUNSTALL: {
        unsigned int runstall = (u32)ivp_hw_query_runstall(pdev);
        put_user(runstall, (unsigned int *)args);
        }
        break;

    case IVP_IOCTL_QUERY_WAITI: {
        unsigned int waiti = (u32)ivp_hw_query_waitmode(pdev);
        put_user(waiti, (unsigned int *)args);
        }
        break;

    case IVP_IOCTL_TRIGGER_NMI:
        ivp_hw_trigger_NMI(pdev);
        break;

    case IVP_IOCTL_WATCHDOG:
        ret = ivp_dev_keep_on_wdg(pdev);
        break;

    case IVP_IOCTL_WATCHDOG_SLEEP:
        ivp_dev_sleep_wdg(pdev);
        break;

    case IVP_IOCTL_SMMU_INVALIDATE_TLB:
        ret = ivp_dev_smmu_invalid_tlb();
        break;

    case IVP_IOCTL_BM_INIT:
        ivp_dev_hwa_enable();
        break;

    default:
        ivp_err("Invalid ioctl command(0x%08x) received!", cmd);
        ret = -EINVAL;
        break;
    }

    return ret;
}

static long ivp_ioctl32(struct file *fd, unsigned int cmd, unsigned long arg)
{
    long ret = 0;
    void *ptr_user = compat_ptr(arg);
    ret = ivp_ioctl(fd, cmd, (unsigned long)ptr_user);
    return ret;
}

static int ivp_mmap(struct file *fd, struct vm_area_struct *vma)
{
    int i = 0;
    int ret = 0;
    unsigned int size = 0;
    unsigned long mm_pgoff = (vma->vm_pgoff << IVP_MMAP_SHIFT);
    unsigned long phy_addr = vma->vm_pgoff << (PAGE_SHIFT + IVP_MMAP_SHIFT);

    size = vma->vm_end - vma->vm_start;
    if (size > LISTENTRY_SIZE) {
        ivp_err("Invalid size = 0x%08x.", size);
        return -EINVAL;
    }

    for (; i < ivp_dev.sect_count; i++) {
        if (phy_addr >= (ivp_dev.sects[i].acpu_addr << IVP_MMAP_SHIFT) &&
             phy_addr <= ((ivp_dev.sects[i].acpu_addr << IVP_MMAP_SHIFT) + ivp_dev.sects[i].len) &&
            (phy_addr + size) <= ((ivp_dev.sects[i].acpu_addr << IVP_MMAP_SHIFT) + ivp_dev.sects[i].len)) {
            ivp_info("Valid section %d for target.", i);
            break;
        }
    }

    if (i == ivp_dev.sect_count) {
        ivp_err("Invalid mapping address or size.");
        return -EINVAL;
    }

    vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
    ret = remap_pfn_range(vma, vma->vm_start, mm_pgoff, size, vma->vm_page_prot);
    if (ret < 0) {
        ivp_err("Failed to map address space. Error code is %d.", ret);
        return ret;
    }
    ivp_dbg("nocached success, ret:%#x", ret);

    return 0;
}

static struct file_operations ivp_fops = {
    .owner = THIS_MODULE,
    .open = ivp_open,
    .release = ivp_release,
    .unlocked_ioctl = ivp_ioctl,
    .compat_ioctl = ivp_ioctl32,
    .mmap = ivp_mmap,
};

static struct ivp_device ivp_dev = {
    .device = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = IVP_MODULE_NAME,
        .fops = &ivp_fops,
    }
};

static inline int ivp_setup_one_onchipmem_section(struct ivp_sect_info *sect, struct device_node *np)
{
    const char *name = of_node_full_name(np);
    unsigned int settings[3] = {0};
    unsigned int len = strlen(name);

    if (of_property_read_u32_array(np, OF_IVP_SECTION_NAME, settings, ARRAY_SIZE(settings))) {/*lint -save -e30 -e84 -e514 -e846 -e866*/
        ivp_err("read reg fail.");
        return -EINVAL;
    }

    len = (len >= sizeof(sect->name)) ? (sizeof(sect->name)-1) : len;
    strncpy(sect->name, name, len); // unsafe_function_ignore: strncpy
    sect->name[len] = '\0';
    sect->ivp_addr  = settings[0];
    sect->acpu_addr = settings[1];
    sect->len       = settings[2];

    return 0;
}

static inline int ivp_setup_onchipmem_sections(struct platform_device *plat_devp, struct ivp_device *ivp_devp)
{
    struct device_node *parent = NULL, *child = NULL;
    size_t i = 0;

    if (plat_devp == NULL || ivp_devp == NULL) {
        ivp_err("pointer is NULL.");
        return -EINVAL;
    }

    parent = of_get_child_by_name(plat_devp->dev.of_node, OF_IVP_SECTION_NODE_NAME);
    if (NULL == parent) {
        ivp_err("Failed to get mem parent node.");
        return -ENODEV;
    }

    /*lint -save -e737 -e838*/
    ivp_devp->sect_count = of_get_child_count(parent);
    ivp_devp->sects = (struct ivp_sect_info *)
                      kzalloc(sizeof(struct ivp_sect_info) * ivp_devp->sect_count, GFP_KERNEL);

    if (NULL == ivp_devp->sects) {
        ivp_err("kmalloc fail.");
        return -ENOMEM;
    }

    ivp_info("section count:%d.", ivp_devp->sect_count);

    for_each_child_of_node(parent, child) {
        if (ivp_setup_one_onchipmem_section(&ivp_devp->sects[i], child)) {
            ivp_err("setup %lu section fail", i);
            goto err_out;
        }

        if (!strncmp(ivp_devp->sects[i].name, OF_IVP_DDR_MEM_NAME, sizeof(OF_IVP_DDR_MEM_NAME))
            || !strncmp(ivp_devp->sects[i].name, OF_IVP_SHARE_MEM_NAME, sizeof(OF_IVP_SHARE_MEM_NAME))
            || !strncmp(ivp_devp->sects[i].name, OF_IVP_LOG_MEM_NAME, sizeof(OF_IVP_LOG_MEM_NAME))) {
            ivp_devp->ivp_meminddr_len += ivp_devp->sects[i].len;
        }

        i++;
    }
    /*lint -restore*/
    return 0;

err_out:
    if(ivp_devp->sects)
        kfree(ivp_devp->sects);
    ivp_devp->sects = NULL;
    ivp_devp->sect_count = 0;
    return -EFAULT;
}

static int ivp_setup_wdg_irq(struct platform_device *plat_devp,
                                       struct ivp_device *ivp_devp)
{
    int irq = platform_get_irq(plat_devp, 0);
    if (irq < 0) {
        ivp_err("Get irq fail!");
        return -EINVAL;
    }

    ivp_devp->wdg_irq = (unsigned int)irq;
    ivp_info("Get irq: %d.", irq);

    return 0;
}

static int ivp_setup_dwaxi_irq(struct platform_device *plat_devp,
                                          struct ivp_device *ivp_devp)
{
    int irq = platform_get_irq(plat_devp, 1);

    if (irq < 0) {
        ivp_err("Get irq fail!");
        return -EINVAL;
    }

    ivp_devp->dwaxi_dlock_irq = (unsigned int)irq;
    ivp_info("Get irq: %d.", irq);

    return 0;
}

static int ivp_setup_smmu_dev(struct ivp_device *ivp_devp)
{
    struct ivp_smmu_dev *smmu_dev = ivp_smmu_get_device((unsigned long)0);
    if (NULL == smmu_dev) {
        ivp_err("Failed to get ivp smmu dev!");
        return -ENODEV;
    }
    ivp_devp->smmu_dev = smmu_dev;
    ivp_devp->smmu_pgd_base = smmu_dev->pgd_base;
    return 0;
}

static void ivp_release_iores(struct platform_device *plat_devp)
{
    struct ivp_device *pdev =
         (struct ivp_device *) platform_get_drvdata(plat_devp);

    ivp_info("enter");
    if (NULL == pdev) {
        ivp_err("%s: pdev is null", __func__);
        return;
    }

    if (NULL != pdev->io_res.gic_base_addr) {
        devm_iounmap(&plat_devp->dev, pdev->io_res.gic_base_addr);
        pdev->io_res.gic_base_addr = NULL;
    }

    if (NULL != pdev->io_res.pericrg_base_addr) {
        devm_iounmap(&plat_devp->dev, pdev->io_res.pericrg_base_addr);
        pdev->io_res.pericrg_base_addr = NULL;
    }

    if (NULL != pdev->io_res.pctrl_base_addr) {
        devm_iounmap(&plat_devp->dev, pdev->io_res.pctrl_base_addr);
        pdev->io_res.pctrl_base_addr = NULL;
    }

    if (NULL != pdev->io_res.cfg_base_addr) {
        devm_iounmap(&plat_devp->dev, pdev->io_res.cfg_base_addr);
        pdev->io_res.cfg_base_addr = NULL;
    }
}

static int ivp_init_reg_res(struct platform_device *pdev, struct ivp_device *ivp_devp)
{
    struct resource *mem_res = NULL;
    int ret = 0;

    mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0); /*lint -save -e838*/
    if (NULL == mem_res) {
        ivp_err("Get cfg res fail!");
        ret = -ENOENT;
        goto ERR_EXIT;
    }

    ivp_devp->io_res.cfg_base_addr = devm_ioremap(&pdev->dev, mem_res->start, resource_size(mem_res));
    if (NULL == ivp_devp->io_res.cfg_base_addr) {
        ivp_err("Map cfg reg failed!");
        ret =  -ENOMEM;
        goto ERR_EXIT;
    }

    mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    if (NULL == mem_res) {
        ivp_err("Get pctrl res failed!");
        ret = -ENOENT;
        goto ERR_EXIT;
    }

    ivp_devp->io_res.pctrl_base_addr = devm_ioremap(&pdev->dev, mem_res->start, resource_size(mem_res));
    if (NULL == ivp_devp->io_res.pctrl_base_addr) {
        ivp_err("Map pctrl reg failed!");
        ret =  -ENOMEM;
        goto ERR_EXIT;
    }

    mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
    if (NULL == mem_res) {
        ivp_err("Get preicrg res failed!");
        ret = -ENOENT;
        goto ERR_EXIT;
    }
    ivp_devp->io_res.pericrg_base_addr = devm_ioremap(&pdev->dev, mem_res->start, resource_size(mem_res));
    if (NULL == ivp_devp->io_res.pericrg_base_addr) {
        ivp_err("Map pericrg res failed!");
        ret = -ENOMEM;
        goto ERR_EXIT;
    }

    mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 3);
    if (NULL == mem_res) {
        ivp_err("Get gic res failed");
        ret = -ENOENT;
        goto ERR_EXIT;
    }

    ivp_devp->io_res.gic_base_addr = devm_ioremap(&pdev->dev, GIC_IRQ_CLEAR_REG, 4); /*lint -save -e747*/
    if (NULL == ivp_devp->io_res.gic_base_addr) {
        ivp_err("Map gic res failed!");
        ret = -ENOMEM;
        goto ERR_EXIT;
    }

    return ret;

ERR_EXIT:
    ivp_release_iores(pdev);
    return ret;
}

static int ivp_remove(struct platform_device *plat_devp)
{
    struct ivp_device *pdev =
        (struct ivp_device *) platform_get_drvdata(plat_devp);

    if (NULL == pdev || NULL == pdev->sects) {
        ivp_err("ivp dev is NULL.This should not happen");
        return -ENODEV;
    }

    misc_deregister(&pdev->device);

    ivp_deinit_pri(pdev);

    kfree(pdev->sects);
    pdev->sects = NULL;
    pdev->sect_count = 0;

    if (NULL != pdev->smmu_dev) {
        pdev->smmu_dev = NULL;
    }
    wake_lock_destroy(&ivp_power_wakelock);
    mutex_destroy(&ivp_wake_lock_mutex);

    return 0;
}

static int ivp_probe(struct platform_device *pdev)
{
    int ret = 0;

    platform_set_drvdata(pdev, &ivp_dev);
    atomic_set(&ivp_dev.accessible, 1);

    wake_lock_init(&ivp_power_wakelock, WAKE_LOCK_SUSPEND, "ivp_power_wakelock");
    mutex_init(&ivp_wake_lock_mutex);

    /*lint -save -e838*/
    ret = misc_register(&ivp_dev.device);
    if (ret) {
        goto err_out_misc;
    }

    ret = ivp_setup_smmu_dev(&ivp_dev);
    if (ret) {
        goto err_out_smmu;
    }

    ret = ivp_setup_onchipmem_sections(pdev, &ivp_dev);
    if (ret) {
        goto err_out_onchipmem;
    }

    ret = ivp_setup_wdg_irq(pdev, &ivp_dev);
    if (ret) {
        goto err_out_wdg;
    }

    ret = ivp_setup_dwaxi_irq(pdev, &ivp_dev);
    if (ret) {
        goto err_out_dwaxi_irq;
    }

    ret = ivp_init_reg_res(pdev, &ivp_dev);
    if (ret) {
        goto err_out_reg_res;
    }

    ret = ivp_init_pri(pdev, &ivp_dev);
    if (ret) {
        goto err_out_init_pri;
    }

    ivp_info("Success!");
    return ret;
    /*lint -restore*/

err_out_init_pri:
    ivp_release_iores(pdev);
err_out_reg_res:
err_out_dwaxi_irq:
err_out_wdg:
    if(ivp_dev.sects)
        kfree(ivp_dev.sects);
    ivp_dev.sects = NULL;
    ivp_dev.sect_count = 0;
err_out_onchipmem:
    ivp_dev.smmu_dev = NULL;
err_out_smmu:
    misc_deregister(&ivp_dev.device);
err_out_misc:
    mutex_destroy(&ivp_wake_lock_mutex);
    wake_lock_destroy(&ivp_power_wakelock);

    return ret;
}
/*lint -restore*/

/*lint -save -e785 -e64*/
static struct of_device_id ivp_of_id[] = {
    { .compatible = "hisilicon,hisi-ivp", },
    {}
};

static struct platform_driver ivp_platform_driver = {
    .probe = ivp_probe,
    .remove = ivp_remove,
    .driver = {
        .name = IVP_MODULE_NAME,
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(ivp_of_id),
    },
};

module_platform_driver(ivp_platform_driver);
/*lint -restore*/

MODULE_DESCRIPTION("hisilicon_ivp driver");
MODULE_LICENSE("GPL");
//lint -restore
