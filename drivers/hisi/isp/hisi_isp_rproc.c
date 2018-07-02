/*
 * hisilicon driver, hisi_isp_rproc.c
 *
 * Copyright (c) 2013 Hisilicon Technologies CO., Ltd.
 *
 */

/*lint -e747 -e715
 -esym(747,*) -esym(715,*)*/
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
#include <linux/hisi/hisi_mailbox_dev.h>
#include <linux/delay.h>
#include <linux/kfifo.h>
#include <linux/mutex.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/rproc_share.h>
#include <linux/sched/rt.h>
#include <linux/kthread.h>
#include <global_ddr_map.h>
#include <linux/iommu.h>
#include <linux/hisi/hisi-iommu.h>
#include <linux/miscdevice.h>
#include "./hisi_isp_rproc.h"
#include "soc_vivo_bus_interface.h"
#include "soc_acpu_baseaddr_interface.h"
#include "isp_ddr_map.h"

#define ISP_MEM_SIZE    0x10000
#define TMP_SIZE        0x1000
#define MAX_SIZE        64


struct hisp_pwr_ops {
    struct mutex lock;
    unsigned int refs_a7;
    unsigned int refs_isp;
    unsigned int refs_ispinit;
    int (*ispup)(struct hisp_pwr_ops *);
    int (*ispdn)(struct hisp_pwr_ops *);
    int (*ispinit)(struct hisp_pwr_ops *);
    int (*ispexit)(struct hisp_pwr_ops *);
    int (*a7up)(struct hisp_pwr_ops *);
    int (*a7dn)(struct hisp_pwr_ops *);
};

struct hisi_isp_nsec {
    struct device *device;
    struct platform_device *isp_pdev;
    struct hisp_pwr_ops *isp_ops;
    void __iomem *crgperi_base;
    void __iomem *isp_regs_base;
    struct regulator *isp_supply;
    struct regulator *ispsrt_supply;
    unsigned int clock_num;
    struct clk *ispclk[ISP_CLK_MAX];
    unsigned int ispclk_value[ISP_CLK_MAX];
    unsigned int clkdis_dvfs[ISP_CLK_MAX];
    unsigned int clkdn[HISP_CLKDOWN_MAX][ISP_CLK_MAX];
    const char *clk_name[ISP_CLK_MAX];
    void *isp_dma_va;
    dma_addr_t isp_dma;
    u64 pgd_base;
    u64 remap_addr;
    int clk_powerby_media;
    int clk_dvfs;
    int ispsmmu_init_byap;
    int isp_mdc_flag;
};

struct hisi_ispmmu_regs_s {
    unsigned int offset;
    unsigned int data;
    int num;
};

/* Define the union U_VP_WR_PREFETCH */
typedef union
{
    /* Define the struct bits */
    struct
    {
        unsigned int    vpwr_index_id0        : 7   ; /* [6..0]  */
        unsigned int    reserved_0            : 1   ; /* [7]  */
        unsigned int    vpwr_index_id1        : 7   ; /* [14..8]  */
        unsigned int    reserved_1            : 16  ; /* [30..15]  */
        unsigned int    vpwr_prefetch_bypass  : 1   ; /* [31]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_VP_WR_PREFETCH;

/* Define the union U_VP_RD_PREFETCH */
typedef union
{
    /* Define the struct bits */
    struct
    {
        unsigned int    vprd_index_id0        : 7   ; /* [6..0]  */
        unsigned int    reserved_0            : 1   ; /* [7]  */
        unsigned int    vprd_index_id1        : 7   ; /* [14..8]  */
        unsigned int    reserved_1            : 16  ; /* [30..15]  */
        unsigned int    vprd_prefetch_bypass  : 1   ; /* [31]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_VP_RD_PREFETCH;

/* Define the union U_CVDR_CFG */
typedef union
{
    /* Define the struct bits */
    struct
    {
        unsigned int    du_write_threshold    : 6   ; /* [5..0]  */
        unsigned int    reserved_0            : 2   ; /* [7..6]  */
        unsigned int    stall_srt_du_threshold : 8  ; /* [15..8]  */
        unsigned int    max_axiread_id        : 5   ; /* [20..16]  */
        unsigned int    reserved_1            : 3   ; /* [23..21]  */
        unsigned int    max_axiwrite_tx       : 5   ; /* [28..24]  */
        unsigned int    reserved_2            : 1   ; /* [29]  */
        unsigned int    force_rd_clk_on       : 1   ; /* [30]  */
        unsigned int    force_wr_clk_on       : 1   ; /* [31]  */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_CVDR_CFG;

/* Define the union U_CVDR_WR_QOS_CFG */
typedef union
{
    /* Define the struct bits */
    struct
    {
        unsigned int    wr_qos_threshold_01_stop  : 4  ; /* [3..0] */
        unsigned int    wr_qos_threshold_01_start : 4  ; /* [7..4] */
        unsigned int    wr_qos_threshold_10_stop  : 4  ; /* [11..8] */
        unsigned int    wr_qos_threshold_10_start : 4  ; /* [15..12] */
        unsigned int    wr_qos_threshold_11_stop  : 4  ; /* [19..16] */
        unsigned int    wr_qos_threshold_11_start : 4  ; /* [23..20] */
        unsigned int    reserved_0                : 2  ; /* [25..24] */
        unsigned int    wr_qos_rt_min             : 2  ; /* [27..26] */
        unsigned int    wr_qos_srt_max            : 2  ; /* [29..28] */
        unsigned int    wr_qos_sr                 : 2  ; /* [31..30] */
    } bits;

    /* Define an unsigned member */
    unsigned int    u32;

} U_CVDR_WR_QOS_CFG;

static struct hisi_isp_nsec nsec_rproc_dev;

struct hisi_ispmmu_regs_s ispmmu_up_regs[] = {
    {SMMU_APB_REG_SMMU_SCR_REG, SCR_DEFAULT, 1},
    {SMMU_APB_REG_SMMU_INTMASK_NS_REG, INTMASK_NS, 1},
    {SMMU_APB_REG_SMMU_INTMAS_S_REG, INTMASK_S, 1},
    {SMMU_APB_REG_SMMU_CB_TTBR0_REG, PGD_BASE, 1},
    {SMMU_APB_REG_SMMU_RLD_EN0_NS_REG, RLD_EN0_NS_DEFAULT, 1},
    {SMMU_APB_REG_SMMU_RLD_EN1_NS_REG, RLD_EN1_NS_DEFAULT, 1},
    {SMMU_APB_REG_SMMU_RLD_EN2_NS_REG, RLD_EN2_NS_DEFAULT, 1},
    {SMMU_APB_REG_SMMU_SMRX_NS_0_REG, SMRX_NS_DEFAULT, 80},
    {SMMU_APB_REG_SMMU_SCR_S_REG, SCR_S_DEFAULT, 1},
    {SMMU_APB_REG_SMMU_RLD_EN0_S_REG, RLD_EN0_S_DEFAULT, 1},
    {SMMU_APB_REG_SMMU_RLD_EN1_S_REG, RLD_EN1_S_DEFAULT, 1},
    {SMMU_APB_REG_SMMU_RLD_EN2_S_REG, RLD_EN2_S_DEFAULT, 1},
    {SMMU_APB_REG_SMMU_SCB_TTBCR_REG, SCB_TTBCR_DEFAULT, 1},
    {SMMU_APB_REG_SMMU_SCB_TTBR_REG, PGD_BASE, 1},
    {SMMU_APB_REG_SMMU_SMRX_S_0_REG, SMRX_NS_DEFAULT, 80},
};

static inline int is_ispup(void)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    return ((readl(dev->crgperi_base + CRG_C88_PERIPHISP_SEC_RSTSTAT) & BIT_RST_ISP) ?
        1 : 0);
}

static int need_powerup(unsigned int refs)
{
    if (refs == 0xffffffff)
        pr_err("[%s] need_powerup exc, refs == 0xffffffff\n", __func__);

    return ((refs == 0) ? 1 : 0);
}

static int need_powerdn(unsigned int refs)
{
    if (refs == 0xffffffff)
        pr_err("[%s] need_powerdn exc, refs == 0xffffffff\n", __func__);

    return ((refs == 1) ? 1 : 0);
}

//lint -save -e529 -e438
int get_isp_mdc_flag(void)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;

    if(dev != NULL)
        return dev->isp_mdc_flag;

    return 0;
}

//lint -save -e529 -e438
static void ispmmu_init(void)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    int i = 0, num = 0;
    void __iomem *smmu_base = dev->isp_regs_base + ISP_BASE_ADDR_SSMMU;
    u64 pgt_addr = dev->pgd_base;

    pr_info("[%s] +\n", __func__);

    writel((readl(ISP_020010_MODULE_CGR_TOP + dev->isp_regs_base) | SMMU_CLK_ENABLE),
            ISP_020010_MODULE_CGR_TOP + dev->isp_regs_base);
    if(dev->ispsmmu_init_byap){
        for (i = 0; i < sizeof(ispmmu_up_regs) / sizeof(struct hisi_ispmmu_regs_s); i++) {/*lint !e574 */
            for (num = 0; num < ispmmu_up_regs[i].num; num++) {
                writel(ispmmu_up_regs[i].data, smmu_base + ispmmu_up_regs[i].offset + num * 4);
            }
        }
        writel(pgt_addr, SMMU_APB_REG_SMMU_CB_TTBR0_REG + smmu_base);
        writel(pgt_addr, SMMU_APB_REG_SMMU_SCB_TTBR_REG + smmu_base);
        writel(0x1, SMMU_APB_REG_SMMU_SCB_TTBCR_REG + smmu_base);
        writel(0x1, SMMU_APB_REG_SMMU_CB_TTBCR_REG + smmu_base);
    }
    /*set jpeg for nosec*/
    //writel(0x6,ISP_BASE_ADDR_SUB_CTRL + ISP_CORE_CTRL_S);
    //writel(0x8,ISP_SUB_CTRL_S + ISP_BASE_ADDR_SUB_CTRL);

    /*set mid*/
    writel(MID_FOR_JPGEN, dev->isp_regs_base + ISP_BASE_ADDR_CVDR_SRT + CVDR_SRT_AXI_CFG_VP_WR_25);
    writel(MID_FOR_JPGEN, dev->isp_regs_base + ISP_BASE_ADDR_CVDR_SRT + CVDR_SRT_AXI_CFG_NR_RD_1);

    pr_info("[%s] -\n", __func__);
}
//lint -restore
static int ispmmu_exit(void)
{
    pr_info("[%s] +\n", __func__);

    return 0;
}

static void ispcvdr_init(void)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    void __iomem *isp_base = dev->isp_regs_base;
    void __iomem *isprt_base = dev->isp_regs_base + ISP_BASE_ADDR_CVDR_RT;
    void __iomem *ispsrt_base = dev->isp_regs_base + ISP_BASE_ADDR_CVDR_SRT;
    void __iomem *ispsctrl_base = dev->isp_regs_base + ISP_SUB_CTRL_BASE_ADDR;

    pr_info("[%s] +\n", __func__);
    writel((readl(ISP_020010_MODULE_CGR_TOP + isp_base) | CVDR_CLK_ENABLE),
            ISP_020010_MODULE_CGR_TOP + isp_base);

    /* CVDR RT*/
    writel(0x0B0B4001, isprt_base + CVDR_RT_CVDR_CFG_REG);
    /* CVDR SRT*/
    writel(0x0B132201, ispsrt_base + CVDR_SRT_CVDR_CFG_REG);
    //CVDR_RT QOS
    writel(0xF8765432, isprt_base + CVDR_RT_CVDR_WR_QOS_CFG_REG);
    writel(0xF8122334, isprt_base + CVDR_RT_CVDR_RD_QOS_CFG_REG);

    //CVDR_SRT QOS
    writel(0xD0765432, ispsrt_base + CVDR_SRT_CVDR_WR_QOS_CFG_REG);
    writel(0xD0122334, ispsrt_base + CVDR_SRT_CVDR_RD_QOS_CFG_REG);

    //CVDR SRT PORT STOP
    writel(0x00020000, ispsrt_base + CVDR_SRT_NR_RD_CFG_3_REG);
    writel(0x00020000, ispsrt_base + CVDR_SRT_VP_WR_IF_CFG_10_REG);
    writel(0x00020000, ispsrt_base + CVDR_SRT_VP_WR_IF_CFG_11_REG);

    //JPGENC limiter DU=8
    writel(0x00000000, ispsrt_base + CVDR_SRT_VP_WR_IF_CFG_25_REG);
    writel(0x80060100, ispsrt_base + CVDR_SRT_NR_RD_CFG_1_REG);
    writel(0x0F001111, ispsrt_base + CVDR_SRT_LIMITER_NR_RD_1_REG);

    //SRT READ
    writel(0x00026E10, ispsctrl_base + SUB_CTRL_ISP_FLUX_CTRL2_0_REG);
    writel(0x0000021F, ispsctrl_base + SUB_CTRL_ISP_FLUX_CTRL2_1_REG);
    //SRT WRITE
    writel(0x00027210, ispsctrl_base + SUB_CTRL_ISP_FLUX_CTRL3_0_REG);
    writel(0x0000024E, ispsctrl_base + SUB_CTRL_ISP_FLUX_CTRL3_1_REG);
    pr_info("[%s] -\n", __func__);
}

static void set_isp_nonsec(void)
{
    pr_alert("[%s] +\n", __func__);
    atfisp_set_nonsec();
    pr_alert("[%s] -\n", __func__);
}
static void disreset_a7(u64 remap_addr)
{
    pr_info("[%s] +\n", __func__);
    atfisp_disreset_a7(remap_addr);
    pr_info("[%s] -\n", __func__);
}

int isp_a7_qos_cfg(void)
{
    void __iomem* vivobus_base;

    pr_info("[%s] +\n", __func__);

    vivobus_base = (void __iomem *)ioremap(SOC_ACPU_NOC_ISP_Service_Target_BASE_ADDR, SZ_4K);
    if (vivobus_base == NULL) {
        pr_err("[%s] vivobus_base remap fail\n", __func__);
        return -ENOMEM;
    }

    pr_info("[%s]  vivobus_base.%pK, ", __func__, vivobus_base);

    __raw_writel(QOS_PRIO_3,      (volatile void __iomem*)(SOC_VIVO_BUS_ISP_RD_QOS_PRIORITY_ADDR(vivobus_base)));
    __raw_writel(QOS_BYPASS_MODE, (volatile void __iomem*)(SOC_VIVO_BUS_ISP_RD_QOS_MODE_ADDR(vivobus_base)));
    __raw_writel(QOS_PRIO_3,      (volatile void __iomem*)(SOC_VIVO_BUS_ISP_WR_QOS_PRIORITY_ADDR(vivobus_base)));
    __raw_writel(QOS_BYPASS_MODE, (volatile void __iomem*)(SOC_VIVO_BUS_ISP_WR_QOS_MODE_ADDR(vivobus_base)));
    __raw_writel(QOS_PRIO_4,      (volatile void __iomem*)(SOC_VIVO_BUS_A7T0VIVOBUS_RD_QOS_PRIORITY_ADDR(vivobus_base)));
    __raw_writel(QOS_FIX_MODE,    (volatile void __iomem*)(SOC_VIVO_BUS_A7T0VIVOBUS_RD_QOS_MODE_ADDR(vivobus_base)));
    __raw_writel(QOS_PRIO_4,      (volatile void __iomem*)(SOC_VIVO_BUS_A7T0VIVOBUS_WR_QOS_PRIORITY_ADDR(vivobus_base)));
    __raw_writel(QOS_FIX_MODE,    (volatile void __iomem*)(SOC_VIVO_BUS_A7T0VIVOBUS_WR_QOS_MODE_ADDR(vivobus_base)));
    __raw_writel(QOS_PRIO_3,      (volatile void __iomem*)(SOC_VIVO_BUS_ISP1_RD_QOS_PRIORITY_ADDR(vivobus_base)));
    __raw_writel(QOS_BYPASS_MODE, (volatile void __iomem*)(SOC_VIVO_BUS_ISP1_RD_QOS_MODE_ADDR(vivobus_base)));
    __raw_writel(QOS_PRIO_3,      (volatile void __iomem*)(SOC_VIVO_BUS_ISP1_WR_QOS_PRIORITY_ADDR(vivobus_base)));
    __raw_writel(QOS_BYPASS_MODE, (volatile void __iomem*)(SOC_VIVO_BUS_ISP1_WR_QOS_MODE_ADDR(vivobus_base)));

    pr_info("QOS : ISP.rd.(prio.0x%x, mode.0x%x), ISP.wr.(prio.0x%x, mode.0x%x), A7.rd.(prio.0x%x, mode.0x%x), A7.wr.(prio.0x%x, mode.0x%x)\n",
        __raw_readl((volatile void __iomem*)(SOC_VIVO_BUS_ISP_RD_QOS_PRIORITY_ADDR(vivobus_base))),
        __raw_readl((volatile void __iomem*)(SOC_VIVO_BUS_ISP_RD_QOS_MODE_ADDR(vivobus_base))),
        __raw_readl((volatile void __iomem*)(SOC_VIVO_BUS_ISP_WR_QOS_PRIORITY_ADDR(vivobus_base))),
        __raw_readl((volatile void __iomem*)(SOC_VIVO_BUS_ISP_WR_QOS_MODE_ADDR(vivobus_base))),
        __raw_readl((volatile void __iomem*)(SOC_VIVO_BUS_A7T0VIVOBUS_RD_QOS_PRIORITY_ADDR(vivobus_base))),
        __raw_readl((volatile void __iomem*)(SOC_VIVO_BUS_A7T0VIVOBUS_RD_QOS_MODE_ADDR(vivobus_base))),
        __raw_readl((volatile void __iomem*)(SOC_VIVO_BUS_A7T0VIVOBUS_WR_QOS_PRIORITY_ADDR(vivobus_base))),
        __raw_readl((volatile void __iomem*)(SOC_VIVO_BUS_A7T0VIVOBUS_WR_QOS_MODE_ADDR(vivobus_base))));

    pr_info("QOS : ISP1.rd.(prio.0x%x, mode.0x%x), ISP1.wr.(prio.0x%x, mode.0x%x)\n",
        __raw_readl((volatile void __iomem*)(SOC_VIVO_BUS_ISP1_RD_QOS_PRIORITY_ADDR(vivobus_base))),
        __raw_readl((volatile void __iomem*)(SOC_VIVO_BUS_ISP1_RD_QOS_MODE_ADDR(vivobus_base))),
        __raw_readl((volatile void __iomem*)(SOC_VIVO_BUS_ISP1_WR_QOS_PRIORITY_ADDR(vivobus_base))),
        __raw_readl((volatile void __iomem*)(SOC_VIVO_BUS_ISP1_WR_QOS_MODE_ADDR(vivobus_base))));

    iounmap(vivobus_base);
    return 0;
}

static int nsec_a7_powerup(struct hisp_pwr_ops *ops)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;

    pr_info("[%s] + refs_a7.0x%x\n", __func__, ops->refs_a7);

    if (!need_powerup(ops->refs_a7)) {
        ops->refs_a7++;
        pr_info("[%s] + refs_isp.0x%x\n", __func__, ops->refs_a7);
        return 0;
    }

    /* need config by secure core */
    disreset_a7(dev->remap_addr);

    ops->refs_a7++;
    pr_info("[%s] - refs_a7.0x%x\n", __func__, ops->refs_a7);
    return 0;
}

static int nsec_a7_powerdn(struct hisp_pwr_ops *ops)
{
    pr_info("[%s] + refs_a7.0x%x\n", __func__, ops->refs_a7);

    if (!need_powerdn(ops->refs_a7)) {
        ops->refs_a7--;
        pr_info("[%s] + refs_a7.0x%x\n", __func__, ops->refs_a7);
        return 0;
    }

    //writel(0x00000030, dev->crgperi_base + CRG_C80_PERIPHISP_SEC_RSTEN);
    ops->refs_a7--;
    pr_info("[%s] - refs_a7.0x%x\n", __func__, ops->refs_a7);
    return 0;
}

static int check_clock_valid(int clk)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;

    if (clk >= (int)dev->clock_num) {
        pr_err("[%s] Failed : clk %d >= %d\n", __func__, clk, dev->clock_num);
        return -EINVAL;
    }

    return 0;
}

int check_dvfs_valid(void)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    return dev->clk_dvfs;
}

static int hisp_clock_down(struct hisi_isp_nsec *dev, int clk, int clkdown)
{
    int ret = 0, stat_machine = 0, type = 0;
    unsigned long ispclk = 0;

    if (!check_dvfs_valid()) {
        pr_err("[%s] Failed : Do not Support DVFS\n", __func__);
        return -EINVAL;
    }

    if (clk >= (int)dev->clock_num) {
        pr_err("[%s] Failed : clk.(%d >= %d)\n", __func__, clk, dev->clock_num);
        return -EINVAL;
    }

    stat_machine = clkdown;
    do {
        type = stat_machine;
        switch (type) {
            case HISP_CLK_TURBO:
            case HISP_CLK_NORMINAL:
            case HISP_CLK_SVS:
            case HISP_CLK_DISDVFS:
                stat_machine ++;
                break;
            default:
                pr_err("[%s] Failed: type.(%d > %d)\n", __func__, type, HISP_CLKDOWN_MAX);
                return -EINVAL;
        }

        ispclk = (unsigned long)dev->clkdn[type][clk];
        pr_info("[%s] Clock Down %lu.%lu MHz\n", __func__, ispclk/1000000, ispclk%1000000);
        if ((ret = clk_set_rate(dev->ispclk[clk], (unsigned long)ispclk)) < 0) {
            pr_err("[%s] Failed: clk_set_rate.%d, %d > %d try clock down ...\n", __func__, ret, type, stat_machine );
            goto try_clock_down;
        }

        if ((ret = clk_prepare_enable(dev->ispclk[clk])) < 0) {
            pr_err("[%s] Failed: clk_prepare_enable.%d, %d > %d try clock down ...\n", __func__, ret, type, stat_machine);
            goto try_clock_down;
        }
try_clock_down:
        if (ret != 0 && stat_machine < HISP_CLKDOWN_MAX && stat_machine >= 0)
            pr_info("[%s] Try Clock Down %lu.%lu MHz > %u.%u MHz\n", __func__, ispclk/1000000, ispclk%1000000, dev->clkdn[stat_machine][clk]/1000000, dev->clkdn[stat_machine][clk]%1000000);
    } while (ret != 0);

    return ret;
}

static int hisp_clock_enable(struct hisi_isp_nsec *dev, int clk)
{
    unsigned long ispclock = 0;
    int ret;

    if (check_clock_valid(clk))
        return -EINVAL;

    ispclock = (unsigned long)dev->ispclk_value[clk];
    if ((ret = clk_set_rate(dev->ispclk[clk], ispclock)) < 0) {
        pr_err("[%s] Failed: %d.%d M, %d.%s.clk_set_rate.%d\n", __func__, (int)ispclock/1000000, (int)ispclock%1000000, clk, dev->clk_name[clk], ret);
        goto try_clock_down;
    }

    if ((ret = clk_prepare_enable(dev->ispclk[clk])) < 0) {
        pr_err("[%s] Failed: %d.%d M, %d.%s.clk_prepare_enable.%d\n", __func__, (int)ispclock/1000000, (int)ispclock%1000000, clk, dev->clk_name[clk], ret);
        goto try_clock_down;
    }

    pr_info("[%s] %d.%s.clk_set_rate.%d.%d M\n", __func__, clk, dev->clk_name[clk], (int)ispclock/1000000, (int)ispclock%1000000);

    return 0;

try_clock_down:
    return hisp_clock_down(dev, clk, HISP_CLK_SVS);
}

static void hisp_clock_disable(struct hisi_isp_nsec *dev, int clk)
{
    unsigned long ispclock = 0;
    int ret = 0;

    if (check_clock_valid(clk))
        return;

    clk_disable_unprepare(dev->ispclk[clk]);

    if (dev->clk_powerby_media) {
        switch (clk) {
            case ISPCPU_CLK:
            case ISPFUNC_CLK:
            case VIVOBUS_CLK:
                ispclock = (unsigned long)dev->clkdis_dvfs[clk];
                if ((ret = clk_set_rate(dev->ispclk[clk], ispclock)) < 0) {
                    pr_err("[%s] Failed: %d.%d M, %d.%s.clk_set_rate.%d\n", __func__, (int)ispclock/1000000, (int)ispclock%1000000, clk, dev->clk_name[clk], ret);
                    return;
                }
                pr_info("[%s] %d.%s.clk_set_rate.%d.%d M\n", __func__, clk, dev->clk_name[clk], (int)ispclock/1000000, (int)ispclock%1000000);
                break;
            default:
                break;
        }
    }
}

int secnsec_setclkrate(unsigned int type, unsigned int rate)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    int ret = -EINVAL;

    switch(type) {
        case ISPCPU_CLK:
        case ISPFUNC_CLK:
        case VIVOBUS_CLK:
            if((rate > dev->ispclk_value[type])
                || (rate == 0)) {
                pr_err("[%s] Failed : DVFS type.0x%x.%s, %d.(%d.%08d M) > %d.(%d.%08d M)\n", __func__,
                    type, dev->clk_name[type], rate, rate/1000000, rate%1000000, dev->ispclk_value[type], dev->ispclk_value[type]/1000000, dev->ispclk_value[type]%1000000);
                return -EINVAL;
            }

            if ((ret = clk_set_rate(dev->ispclk[type], (unsigned long)rate)) < 0) {
                pr_err("[%s] Failed : DVFS Set.0x%x.%s Rate.%d.(%d.%08d M)\n", __func__, type, dev->clk_name[type], rate, rate/1000000, rate%1000000);
                return ret;
            }

            pr_info("[%s] DVFS Set.0x%x.%s Rate.%d.(%d.%08d M)\n", __func__, type, dev->clk_name[type], rate, rate/1000000, rate%1000000);
            break;
        default:
            pr_err("[%s] Failed : DVFS Invalid type.0x%x, rate.%d\n", __func__, type, rate);
            return -EINVAL;
    }

    return 0;
}

int nsec_setclkrate(unsigned int type, unsigned int rate)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    struct hisp_pwr_ops *ops = dev->isp_ops;
    int ret = 0;

    if (!ops) {
        pr_info("[%s] Failed : ops.%pK\n", __func__, ops);
        return -ENXIO;
    }

    mutex_lock(&ops->lock);
    if (ops->refs_a7 == 0) {
        pr_info("[%s] Failed : refs_a7.%d, check ISPCPU PowerDown\n", __func__, ops->refs_a7);
        mutex_unlock(&ops->lock);
        return -ENODEV;
    }

    if ((ret = secnsec_setclkrate(type, rate)) < 0)
        pr_info("[%s] Failed : secnsec_setclkrate.%d\n", __func__, ret);
    mutex_unlock(&ops->lock);

    return ret;
}

int hisp_powerup(void)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    struct hisp_pwr_ops *ops = dev->isp_ops;
    int ret, err, index, err_index;

    if (!ops) {
        pr_info("[%s] Failed : ops.%pK\n", __func__, ops);
        return -ENXIO;
    }

    pr_info("[%s] + clk_powerby_media.%d, refs_isp.0x%x\n", __func__, dev->clk_powerby_media, ops->refs_isp);
    if (!need_powerup(ops->refs_isp)) {
        ops->refs_isp++;
        pr_info("[%s] + refs_isp.0x%x\n", __func__, ops->refs_isp);
        return 0;
    }

    if (dev->clk_powerby_media) {
        if ((ret = regulator_enable(dev->isp_supply)) != 0) {
            pr_err("[%s] Failed: isp regulator_enable.%d\n", __func__, ret);
            return ret;
        }
    }

    for (index = 0; index < (int)dev->clock_num; index ++) {
        if ((ret = hisp_clock_enable(dev, index)) < 0) {
            pr_err("[%s] Failed: hisp_clock_enable.%d, index.%d\n", __func__, ret, index);
            goto err_ispclk;
        }
    }

    if ((ret = regulator_enable(dev->ispsrt_supply)) != 0) {
        pr_err("[%s] Failed: ispsrt regulator_enable.%d\n", __func__, ret);
        goto err_ispclk;
    }

    ops->refs_isp++;
    pr_err("[%s] - clk_powerby_media.%d, refs_isp.0x%x\n", __func__, dev->clk_powerby_media, ops->refs_isp);

    return 0;

err_ispclk:
    for (err_index = 0; err_index < index; err_index ++)
        hisp_clock_disable(dev, err_index);

    if (dev->clk_powerby_media) {
        if ((err = regulator_disable(dev->isp_supply)) < 0)
            pr_err("[%s] Failed: isp regulator_disable.%d, up ret.%d\n", __func__, err, ret);
    }

    return ret;
}

int hisp_powerdn(void)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    struct hisp_pwr_ops *ops = dev->isp_ops;
    int ret = 0, index;

    if (!ops) {
        pr_info("[%s] Failed : ops.%pK\n", __func__, ops);
        return -ENXIO;
    }

    pr_info("[%s] + clk_powerby_media.%d, refs_isp.0x%x\n", __func__, dev->clk_powerby_media, ops->refs_isp);
    if (!need_powerdn(ops->refs_isp)) {
        ops->refs_isp--;
        pr_info("[%s] + refs_isp.0x%x\n", __func__, ops->refs_isp);
        return 0;
    }

    if ((ret = regulator_disable(dev->ispsrt_supply)) != 0)
        pr_err("[%s] Failed: ispsrt regulator_disable.%d\n", __func__, ret);

    for (index = 0; index < (int)dev->clock_num; index ++)
        hisp_clock_disable(dev, index);

    if (dev->clk_powerby_media) {
        if ((ret = regulator_disable(dev->isp_supply)) != 0)
            pr_err("[%s] Failed: isp regulator_disable.%d\n", __func__, ret);
    }

    ops->refs_isp--;
    pr_info("[%s] - clk_powerby_media.%d, refs_isp.0x%x\n", __func__, dev->clk_powerby_media, ops->refs_isp);

    return 0;
}

static int nsec_isp_powerup(struct hisp_pwr_ops *ops)
{
    return hisp_powerup();
}

static int nsec_isp_powerdn(struct hisp_pwr_ops *ops)
{
    return hisp_powerdn();
}

static int nsec_isp_init(struct hisp_pwr_ops *ops)
{
    pr_info("[%s] + refs_ispinit.0x%x\n", __func__, ops->refs_ispinit);
    if (!need_powerup(ops->refs_ispinit)) {
        ops->refs_ispinit++;
        pr_info("[%s] + refs_ispinit.0x%x\n", __func__, ops->refs_ispinit);
        return 0;
    }

    ispmmu_init();
    ispcvdr_init();

    ops->refs_ispinit++;
    pr_err("[%s] - refs_ispinit.0x%x\n", __func__, ops->refs_ispinit);

    return 0;
}

static int nsec_isp_exit(struct hisp_pwr_ops *ops)
{
    int ret;

    pr_info("[%s] + refs_ispinit.%x\n", __func__, ops->refs_ispinit);
    if (!need_powerdn(ops->refs_ispinit)) {
        ops->refs_ispinit--;
        pr_info("[%s] + refs_ispinit.%x\n", __func__, ops->refs_ispinit);
        return 0;
    }

    if ((ret = ispmmu_exit()))
        pr_err("[%s] Failed : ispmmu_exit.%d\n", __func__, ret);

    ops->refs_ispinit--;
    pr_err("[%s] - refs_ispinit.%x\n", __func__, ops->refs_ispinit);

    return 0;
}

static struct hisp_pwr_ops isp_pwr_ops = {
    .lock       = __MUTEX_INITIALIZER(isp_pwr_ops.lock),
    .a7up       = nsec_a7_powerup,
    .a7dn       = nsec_a7_powerdn,
    .ispup      = nsec_isp_powerup,
    .ispdn      = nsec_isp_powerdn,
    .ispinit    = nsec_isp_init,
    .ispexit    = nsec_isp_exit,
};

int hisp_nsec_jpeg_powerup(void)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    struct hisp_pwr_ops *ops = dev->isp_ops;
    int ret = 0;

    pr_info("[%s] +\n", __func__);
    if (!ops) {
        pr_err("[%s] Failed : ops.%pK\n", __func__, ops);
        return -EINVAL;
    }

    mutex_lock(&ops->lock);
    ret = ops->ispup(ops);
    if (0 != ret) {
        pr_err("[%s] Failed : ispup.%d\n", __func__, ret);
        mutex_unlock(&ops->lock);
        return ret;
    }

    if (need_powerup(ops->refs_ispinit)) {
        ret = isp_a7_qos_cfg();
        if (0 != ret) {
            pr_err("[%s] Failed : isp_a7_qos_cfg.%d\n", __func__, ret);
            mutex_unlock(&ops->lock);
            return ret;
        }
        set_isp_nonsec();
    }

    ret = ops->ispinit(ops);
    if (0 != ret) {
        pr_err("[%s] ispinit.%d\n", __func__, ret);
        goto isp_down;
    }

    pr_info("[%s] refs_a7.0x%x, refs_isp.0x%x, refs_ispinit.0x%x\n", __func__,
            ops->refs_a7, ops->refs_isp, ops->refs_ispinit);
    mutex_unlock(&ops->lock);

    return 0;

isp_down:
    if ((ops->ispdn(ops)) != 0)
        pr_err("[%s] Failed : ispdn\n", __func__);

    pr_info("[%s] refs_a7.0x%x, refs_isp.0x%x, refs_ispinit.0x%x\n", __func__,
            ops->refs_a7, ops->refs_isp, ops->refs_ispinit);
    mutex_unlock(&ops->lock);
    pr_info("[%s] -\n", __func__);

    return ret;
}
EXPORT_SYMBOL(hisp_nsec_jpeg_powerup);
/*lint -save -e631 -e613*/
int hisp_nsec_jpeg_powerdn(void)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    struct hisp_pwr_ops *ops = dev->isp_ops;
    int ret = 0;

    pr_info("%s: +\n", __func__);
    if (!ops) {
        pr_err("%s: failed, isp_ops is null.\n", __func__);
    }

    mutex_lock(&ops->lock);

    if ((ret = ops->ispexit(ops)))
        pr_err("%s: jpegdn faled, ret.%d\n", __func__, ret);

    if ((ret = ops->ispdn(ops)) != 0)
        pr_err("%s: ispdn faled, ret.%d\n", __func__, ret);

    pr_info("%s:refs_a7.0x%x, refs_isp.0x%x, refs_ispinit.0x%x\n", __func__,
            ops->refs_a7, ops->refs_isp, ops->refs_ispinit);

    mutex_unlock(&ops->lock);
    pr_info("%s: -\n", __func__);
    return 0;
}
/*lint -restore */
EXPORT_SYMBOL(hisp_nsec_jpeg_powerdn);

int nonsec_isp_device_enable(void)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    struct hisp_pwr_ops *ops = dev->isp_ops;
    int ret = 0;

    if (!ops) {
        pr_err("[%s] Failed : ops.%pK\n", __func__, ops);
        return -1;
    }

    mutex_lock(&ops->lock);
    ret = ops->ispup(ops);
    if (0 != ret) {
        pr_err("[%s] Failed : ispup.%d\n", __func__, ret);
        mutex_unlock(&ops->lock);
        return ret;
    }

    if (need_powerup(ops->refs_ispinit)) {
        ret = isp_a7_qos_cfg();
        if (0 != ret) {
            pr_err("[%s] Failed : isp_a7_qos_cfg.%d\n", __func__, ret);
            mutex_unlock(&ops->lock);
            return ret;
        }
        set_isp_nonsec();
    }

    ret = ops->ispinit(ops);
    if (0 != ret) {
        pr_err("[%s] Failed : ispinit.%d\n", __func__, ret);
        goto isp_down;
    }

    ret = ops->a7up(ops);
    if (0 != ret) {
        pr_err("[%s] Failed : a7up.%d\n", __func__, ret);
        goto isp_exit;
    }

    pr_info("[%s] refs_a7.0x%x, refs_isp.0x%x, refs_ispinit.0x%x\n", __func__,
            ops->refs_a7, ops->refs_isp, ops->refs_ispinit);
    mutex_unlock(&ops->lock);

    return ret;

isp_exit:
    if ((ops->ispexit(ops)) != 0)
        pr_err("[%s] Failed : ispexit\n", __func__);
isp_down:
    if ((ops->ispdn(ops)) != 0)
        pr_err("[%s] Failed : ispdn\n", __func__);

    pr_info("[%s] refs_a7.0x%x, refs_isp.0x%x, refs_ispinit.0x%x\n", __func__,
            ops->refs_a7, ops->refs_isp, ops->refs_ispinit);
    mutex_unlock(&ops->lock);

    return ret;
}
EXPORT_SYMBOL(nonsec_isp_device_enable);

int nonsec_isp_device_disable(void)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    struct hisp_pwr_ops *ops = dev->isp_ops;
    int ret = 0;

    if (!ops) {
        pr_err("[%s] Failed : ops.%pK\n", __func__, ops);
        return -1;
    }

    mutex_lock(&ops->lock);
    if ((ret = ops->a7dn(ops)) != 0)
        pr_err("[%s] a7dn faled, ret.%d\n", __func__, ret);

    if ((ret = ops->ispexit(ops)))
        pr_err("[%s] jpegdn faled, ret.%d\n", __func__, ret);

    if ((ret = ops->ispdn(ops)) != 0)
        pr_err("[%s] ispdn faled, ret.%d\n", __func__, ret);

    pr_info("[%s] refs_a7.0x%x, refs_isp.0x%x, refs_ispinit.0x%x\n", __func__,
            ops->refs_a7, ops->refs_isp, ops->refs_ispinit);
    mutex_unlock(&ops->lock);

    return 0;
}
EXPORT_SYMBOL(nonsec_isp_device_disable);

static int set_nonsec_pgd(struct rproc *rproc)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    struct iommu_domain_data *info = NULL;
    struct iommu_domain *domain = rproc->domain;
    struct rproc_shared_para *param = NULL;

    if (NULL == domain) {
        pr_info("[%s] Failed : domain.%pK\n", __func__, domain);
        return -EINVAL;
    }

    info = (struct iommu_domain_data *)domain->priv;
    if (NULL == info) {
        pr_info("[%s] Failed : info.%pK\n", __func__, info);
        return -EINVAL;
    }

    dev->pgd_base = info->phy_pgd_base;

    hisp_lock_sharedbuf();
    param = rproc_get_share_para();
    if (!param) {
        pr_err("[%s] Failed : param.%pK\n", __func__, param);
        hisp_unlock_sharedbuf();
        return -EINVAL;
    }
    param->dynamic_pgtable_base = dev->pgd_base;
    hisp_unlock_sharedbuf();

    pr_info("[%s] dev->pgd_base.0x%llx == info->phy_pgd_base.0x%llx\n", __func__,
            dev->pgd_base, info->phy_pgd_base);

    return 0;
}

int hisi_isp_rproc_pgd_set(struct rproc *rproc)
{
    int err = 0;

    if (use_nonsec_isp()) {
        pr_info("[%s] +\n", __func__);
        err = set_nonsec_pgd(rproc);
        if (0 != err) {
            pr_err("[%s] Failed : set_nonsec_pgd.%d\n", __func__, err);
            return err;
        }
    }
    else{
        struct rproc_shared_para *param = NULL;
        hisp_lock_sharedbuf();
        param = rproc_get_share_para();
        if(param)
            param->dynamic_pgtable_base = get_nonsec_pgd();
        hisp_unlock_sharedbuf();
    }

    return 0;
}

static int hisi_isp_nsec_getdts(struct platform_device *pdev, struct hisi_isp_nsec *dev)
{
    struct device *device = &pdev->dev;
    struct device_node *np = device->of_node;
    int ret, i;

    if (!np) {
        pr_err("[%s] Failed : np.%pK\n", __func__, np);
        return -ENODEV;
    }

    pr_info("[%s] +\n", __func__);
    dev->device = device;
    dev->isp_supply = devm_regulator_get(device, "isp-subsys-rproc");
    if (IS_ERR(dev->isp_supply)) {
        pr_err("[%s] Failed : ISP devm_regulator_get.%pK\n", __func__, dev->isp_supply);
        return -EINVAL;
    }

    dev->ispsrt_supply = devm_regulator_get(device, "isp-srt");
    if (IS_ERR(dev->ispsrt_supply)) {
        pr_err("[%s] Failed : ISPSRT devm_regulator_get.%pK\n", __func__, dev->ispsrt_supply);
        return -EINVAL;
    }

    dev->remap_addr = dev->isp_dma;
    set_a7mem_pa(dev->remap_addr);
    set_a7mem_va(dev->isp_dma_va);
    pr_info("[%s] remap_addr.0x%llx, dma_va.%pK\n", __func__, dev->remap_addr, dev->isp_dma_va);

    if ((ret = of_property_read_u32(np, "clock-num", (unsigned int *)(&dev->clock_num))) < 0 ) {
        pr_err("[%s] Failed: clock-num of_property_read_u32.%d\n", __func__, ret);
        return -EINVAL;
    }

    if ((ret = of_property_read_string_array(np, "clock-names", dev->clk_name, dev->clock_num)) < 0) {
        pr_err("[%s] Failed : clock-names of_property_read_string_array.%d\n", __func__, ret);
        return -EINVAL;
    }

    if ((ret = of_property_read_u32_array(np, "clock-value", dev->ispclk_value, dev->clock_num)) < 0) {
        pr_err("[%s] Failed: clock-value of_property_read_u32_array.%d\n", __func__, ret);
        return -EINVAL;
    }

    if ((ret = of_property_read_u32(np, "clk_dvfs", (unsigned int *)(&dev->clk_dvfs))) < 0 ) {
        pr_err("[%s] Failed: clk_dvfs of_property_read_u32.%d\n", __func__, ret);
        return -EINVAL;
    }
    pr_info("[%s] clk_dvfs.0x%x\n", __func__, dev->clk_dvfs);

    if ((ret = of_property_read_u32(np, "clk_flag", (unsigned int *)(&dev->clk_powerby_media))) < 0 ) {
        pr_err("[%s] Failed: clk_flag of_property_read_u32.%d\n", __func__, ret);
        return -EINVAL;
    }
    pr_info("[%s] clk_powerby_media.0x%x\n", __func__, dev->clk_powerby_media);

    if ((ret = of_property_read_u32(np, "ispsmmu-init-byap", (unsigned int *)(&dev->ispsmmu_init_byap))) < 0 ) {
        pr_err("[%s] Failed: ispsmmu-init-byap of_property_read_u32.%d\n", __func__, ret);
        return -EINVAL;
    }
    pr_info("[%s] isp-smmu-flag.0x%x\n", __func__, dev->ispsmmu_init_byap);

    if ((ret = of_property_read_u32(np, "isp-mdc-flag", (unsigned int *)(&dev->isp_mdc_flag))) < 0 ) {
        pr_err("[%s] Failed: isp-mdc-flag of_property_read_u32.%d\n", __func__, ret);
        return -EINVAL;
    }
    pr_info("[%s] isp_mdc_flag.0x%x\n", __func__, dev->isp_mdc_flag);

    if (dev->clk_powerby_media) {
        if ((ret = of_property_read_u32_array(np, "clkdis-dvfs", dev->clkdis_dvfs, dev->clock_num)) < 0) {
            pr_err("[%s] Failed: clkdis-dvfs of_property_read_u32_array.%d\n", __func__, ret);
            return -EINVAL;
        }
    }

    if (dev->clk_dvfs) {
        if ((ret = of_property_read_u32_array(np, "clock-value", dev->clkdn[HISP_CLK_TURBO], dev->clock_num)) < 0) {
            pr_err("[%s] Failed: TURBO of_property_read_u32_array.%d\n", __func__, ret);
            return -EINVAL;
        }

        if ((ret = of_property_read_u32_array(np, "clkdn-normal", dev->clkdn[HISP_CLK_NORMINAL], dev->clock_num)) < 0) {
            pr_err("[%s] Failed: NORMINAL of_property_read_u32_array.%d\n", __func__, ret);
            return -EINVAL;
        }

        if ((ret = of_property_read_u32_array(np, "clkdn-svs", dev->clkdn[HISP_CLK_SVS], dev->clock_num)) < 0) {
            pr_err("[%s] Failed: SVS of_property_read_u32_array.%d\n", __func__, ret);
            return -EINVAL;
        }

        if ((ret = of_property_read_u32_array(np, "clkdis-dvfs", dev->clkdn[HISP_CLK_DISDVFS], dev->clock_num)) < 0) {
            pr_err("[%s] Failed: SVS of_property_read_u32_array.%d\n", __func__, ret);
            return -EINVAL;
        }
    }

    for (i = 0; i < (int)dev->clock_num; i++) {
        dev->ispclk[i] = devm_clk_get(device, dev->clk_name[i]);
        if (IS_ERR_OR_NULL(dev->ispclk[i])) {
            pr_err("[%s] Failed : ispclk.%s.%d.%li\n", __func__, dev->clk_name[i], i, PTR_ERR(dev->ispclk[i]));
            return -EINVAL;
        }
        pr_info("[%s] ISP clock.%d.%s: %d.%d M\n", __func__, i, dev->clk_name[i], dev->ispclk_value[i]/1000000, dev->ispclk_value[i]%1000000);
        if (dev->clk_powerby_media)
            pr_info("[%s] clkdis.%d.%s: %d.%d M\n", __func__, i, dev->clk_name[i], dev->clkdis_dvfs[i]/1000000, dev->clkdis_dvfs[i]%1000000);
    }

    pr_info("[%s] -\n", __func__);

    return 0;
}

static int isp_remap_rsc(struct hisi_isp_nsec *dev)
{
    dev->crgperi_base = (void __iomem *)ioremap(SOC_ACPU_PERI_CRG_BASE_ADDR, SZ_4K);
    if (dev->crgperi_base == NULL) {
        pr_err("[%s] crgperi_base err 0x%x(0x%x)\n", __func__, SOC_ACPU_PERI_CRG_BASE_ADDR, SZ_4K);
        return -ENOMEM;
    }

    dev->isp_regs_base = (void __iomem *)ioremap(ISP_CORE_CFG_BASE_ADDR, REG_BASE_ISP_SIZE);
    if (unlikely(!dev->isp_regs_base)) {
        pr_err("[%s] isp_regs_base 0x%x.0x%x\n", __func__, ISP_CORE_CFG_BASE_ADDR, REG_BASE_ISP_SIZE);
        goto free_crgperi_base;
    }

    dev->isp_dma_va = dma_alloc_coherent(dev->device, ISP_MEM_SIZE, &dev->isp_dma, GFP_KERNEL);
    if (unlikely(!dev->isp_dma_va)) {
        pr_err("[%s] isp_dma_va failed\n", __func__);
        goto free_regs_base;
    }

    pr_info("[%s] crgperi_base.%pK, isp_regs_base.%pK, isp_dma_va.%pK, dma.0x%llx\n", __func__,
            dev->crgperi_base, dev->isp_regs_base, dev->isp_dma_va, dev->isp_dma);

    return 0;

free_regs_base:
    iounmap(dev->isp_regs_base);
free_crgperi_base:
    iounmap(dev->crgperi_base);

    dev->crgperi_base = NULL;
    dev->isp_regs_base = NULL;

    return -ENOMEM;
}

static void isp_unmap_rsc(struct hisi_isp_nsec *dev)
{
    if (dev->crgperi_base)
        iounmap(dev->crgperi_base);
    if (dev->isp_regs_base)
        iounmap(dev->isp_regs_base);
    if (dev->isp_dma_va)
        dma_free_coherent(dev->device, ISP_MEM_SIZE, dev->isp_dma_va, dev->isp_dma);

    dev->crgperi_base = NULL;
    dev->isp_regs_base = NULL;
    dev->isp_dma_va = NULL;
}

int hisi_isp_nsec_probe(struct platform_device *pdev)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    struct hisp_pwr_ops *ops = &isp_pwr_ops;
    int ret = 0;

    pr_alert("[%s] +\n", __func__);
    ops->refs_a7 = 0;
    ops->refs_isp = 0;
    ops->refs_ispinit = 0;

    dev->device = &pdev->dev;
    dev->isp_pdev = pdev;
    dev->isp_ops = ops;

    if ((ret = isp_remap_rsc(dev)) != 0) {
        pr_err("[%s] failed, isp_remap_src.%d\n", __func__, ret);
        return ret;
    }

    if ((ret = hisi_isp_nsec_getdts(pdev, dev)) != 0) {
        pr_err("[%s] Failed : hisi_isp_nsec_getdts.%d.\n", __func__, ret);
        goto out;
    }

    pr_alert("[%s] -\n", __func__);

    return 0;
out:
    isp_unmap_rsc(dev);

    return ret;
}

int hisi_isp_nsec_remove(struct platform_device *pdev)
{
    struct hisi_isp_nsec *dev = &nsec_rproc_dev;
    isp_unmap_rsc(dev);
    return 0;
}

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("HiStar V150 rproc driver");
