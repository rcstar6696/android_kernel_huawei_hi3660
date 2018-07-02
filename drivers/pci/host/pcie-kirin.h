/*
 * PCIe host controller driver for Kirin 960 SoCs
 *
 * Copyright (C) 2015 Huawei Electronics Co., Ltd.
 *		http://www.huawei.com
 *
 * Author: Xiaowei Song <songxiaowei@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _PCIE_KIRIN_H
#define _PCIE_KIRIN_H

#include <asm/compiler.h>
#include <linux/compiler.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/atomic.h>
#include <linux/interrupt.h>
#include <linux/irqdomain.h>
#include <linux/kernel.h>
#include <linux/of_gpio.h>
#include <linux/pci.h>
#include <linux/of_pci.h>
#include <linux/platform_device.h>
#include <linux/resource.h>
#include <linux/signal.h>
#include <linux/types.h>
#include <linux/irq.h>
#include <linux/msi.h>
#include <linux/of_address.h>
#include <linux/pci_regs.h>
#include <linux/regulator/consumer.h>
#include <linux/version.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/hisi/pcie-kirin-api.h>
#include <soc_sctrl_interface.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/wait.h>
#include <linux/freezer.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0))
#include "kirin-pcie-designware.h"
#else
#include "pcie-designware.h"
#endif

#define to_kirin_pcie(x)	container_of(x, struct kirin_pcie, pp)

#define MAX_IRQ_NUM 5
#define IRQ_INTA 0
#define IRQ_INTB 1
#define IRQ_MSI 1
#define IRQ_INTC 2
#define IRQ_INTD 3
#define IRQ_LINKDOWN 4

#define REF_CLK_FREQ 100000000

#define TEST_BUS0_OFFSET 0x0
#define TEST_BUS1_OFFSET 0x1000000

/* PCIe ELBI registers */
#define SOC_PCIECTRL_CTRL0_ADDR 0x000
#define SOC_PCIECTRL_CTRL1_ADDR 0x004
#define SOC_PCIECTRL_CTRL7_ADDR 0x01c
#define SOC_PCIECTRL_CTRL8_ADDR 0x020
#define SOC_PCIECTRL_CTRL11_ADDR 0x02c
#define SOC_PCIECTRL_CTRL12_ADDR 0x030
#define SOC_PCIECTRL_CTRL20_ADDR 0x050
#define SOC_PCIECTRL_CTRL21_ADDR 0x054
#define SOC_PCIECTRL_STATE1_ADDR 0x404
#define SOC_PCIECTRL_STATE4_ADDR 0x410
#define SOC_PCIECTRL_STATE5_ADDR 0x414

#define SOC_PCIEPHY_CTRL0_ADDR 0x000
#define SOC_PCIEPHY_CTRL1_ADDR 0x004
#define SOC_PCIEPHY_CTRL2_ADDR 0x008
#define SOC_PCIEPHY_CTRL3_ADDR 0x00c
#define SOC_PCIEPHY_CTRL33_ADDR 0x0084
#define SOC_PCIEPHY_CTRL38_ADDR 0x0098
#define SOC_PCIEPHY_CTRL40_ADDR 0x00A0
#define SOC_PCIEPHY_STATE0_ADDR 0x400
#define SOC_PCIEPHY_STATE39_ADDR 0x049c


#define PCIE_APB_CLK_REQ	(0x1 << 23)
#define PERST_FUN_SEC 0x2006
#define PERST_ASSERT_EN 0x1

#define ENTRY_L23_BIT (0x1 << 2)
#define PCIE_ELBI_SLV_DBI_ENABLE	(0x1 << 21)
#define PME_TURN_OFF_BIT (0x1 << 8)
#define PME_ACK_BIT (0x1<<16)
#define PCI_ANY_ID (~0)

#define RD_FLAG 0
#define WR_FLAG 1
#define IOMG_GPIO 0
#define IOMG_CLKREQ 1
#define BOARD_FPGA 0
#define BOARD_EMU  1
#define BOARD_ASIC 2
#define ENABLE 1
#define DISABLE 0

#define PCI_EXT_LTR_CAP_ID	0x18
#define LTR_MAX_SNOOP_LATENCY	0x04
#define PCI_EXT_L1SS_CAP_ID 0x1E
#define PCI_EXT_L1SS_CTRL1	0x08
#define PCI_EXT_L1SS_CTRL2 	0x0C

#define PCIE_APP_LTSSM_ENABLE		0x01c
#define PCIE_ELBI_RDLH_LINKUP		0x400
#define PCIE_LINKUP_ENABLE		(0x8020)
#define PCIE_LTSSM_ENABLE_BIT	  (0x1 << 11)

/*designware register*/
/*lint -e648 -e750 -esym(750,*) -esym(648,*)*/
#define PCIE_ACK_FREQ_ASPM_CTRL		0x70c
#define PCIE_ACK_FREQ_ASPM_MASK		0xFFFF00
#define PCIE_GEN2_CTRL_OFF		0x80c
#define PCIE_GEN2_CTRL_MASK		0xFF
#define PCIE_ATU_VIEWPORT		0x900
#define PCIE_ATU_REGION_INBOUND		(0x1 << 31)
#define PCIE_ATU_REGION_OUTBOUND		(0x0 << 31)
#define PCIE_ATU_REGION_INDEX1		(0x1 << 0)
#define PCIE_ATU_REGION_INDEX0		(0x0 << 0)
#define PCIE_ATU_CR1			0x4
#define PCIE_ATU_TYPE_MEM		(0x0 << 0)
#define PCIE_ATU_TYPE_CFG0		(0x4 << 0)
#define PCIE_ATU_TYPE_MSG		0x14
#define PCIE_ATU_CR2			0x8
#define PCIE_ATU_ENABLE			(0x1 << 31)
#define PCIE_ATU_LOWER_BASE		0xC
#define PCIE_ATU_UPPER_BASE		0x10
#define PCIE_ATU_LIMIT			0x14
#define PCIE_ATU_LOWER_TARGET		0x18
#define PCIE_ATU_UPPER_TARGET		0x1C

/* port logic register */
#define PROT_FORCE_LINK_REG			0x708
#define PORT_LINK_CTRL_REG 			0x710
#define PORT_MSI_CTRL_ADDR			0x820
#define PORT_MSI_CTRL_UPPER_ADDR		0x824
#define PORT_MSI_CTRL_INT0_ENABLE		0x828
#define PORT_GEN3_CTRL_REG			0x890
#define PORT_PIPE_LOOPBACK_REG 		0x8B8

#define MSG_CODE_PME_TURN_OFF 25
#define MSG_TYPE_ROUTE_BROADCAST 0x13
#define MSG_CPU_ADDR 0xf6000000
#define MSG_CPU_ADDR_SIZE 0x100

#define LTSSM_LINK_DOWN		0xFFFFFFFF
#define ADDR_OFFSET_4BYTE	0x4

/*PCIe capability register*/
#define ENTER_COMPLIANCE	(0x1 << 4)
/*lint -e648 -e750 +esym(648,*) +esym(750,*)*/
struct kirin_pcie_irq_info {
	char *name;
	int num;
};

struct kirin_pcie_dtsinfo {
	u32		board_type;
	u32		chip_type;
	u32		eco;
	int 		ep_flag;
	u32		ep_ltr_latency;
	u32		ep_l1ss_ctrl2;
	u32		l1ss_ctrl1;
	u32		aspm_state;
	u32 		pcie_eye_param_ctrl2;	/* this param will be set to pcie phy ctrl2 */
	u32 		pcie_eye_param_ctrl3;/* this param will be set to pcie phy ctrl3 */
	u32 		iso_info[2];
	u32 		assert_info[2];
	u32		iatu_base_offset;
	u32 		t_ref2perst[2];
	u32 		t_perst2access[2];
	u32 		t_perst2rst[2];
	u32		eye_param_vboost;
	u32 		eye_param_iboost;
	u32		eye_param_pre;
	u32		eye_param_post;
	u32		eye_param_main;
};

struct kirin_pcie {
	void __iomem		*apb_base;
	void __iomem		*phy_base;
	void __iomem		*crg_base;
	void __iomem        *sctrl_base;
	void __iomem		*pmctrl_base;
	void __iomem		*pme_base;
	u32					natural_phy_offset;
	u32					apb_phy_offset;
	u32					sram_phy_offset;
	struct clk			*apb_sys_clk;
	struct clk			*apb_phy_clk;
	struct clk			*phy_ref_clk;
	struct clk			*pcie_aclk;
	struct clk			*pcie_aux_clk;
	int     gpio_id_reset;
	struct  pcie_port	pp;
	struct  pci_dev		*rc_dev;
	struct  pci_dev		*ep_dev;
	unsigned int		ep_devid;
	unsigned int		ep_venid;
	atomic_t		usr_suspend;
	atomic_t		is_ready;  //driver is ready
	atomic_t		is_power_on;
	atomic_t		is_enumerated;
	struct mutex		pm_lock;
	spinlock_t		ep_ltssm_lock;
	struct pci_saved_state *rc_saved_state;
	struct work_struct	handle_work;
	struct kirin_pcie_register_event *event_reg;
	struct kirin_pcie_irq_info irq[5];
	u32	msi_controller_config[3];
	u32 aer_config;
	u32 rc_id;
	struct kirin_pcie_dtsinfo dtsinfo;
	u32			ep_link_status;
};

enum rc_power_status {
	RC_POWER_OFF = 0,
	RC_POWER_ON = 1,
	RC_POWER_SUSPEND = 2,
	RC_POWER_RESUME = 3,
	RC_POWER_INVALID = 4,
};


enum link_aspm_state {
	ASPM_CLOSE = 0,		/*disable aspm L0s L1*/
	ASPM_L0S = 1,		/* enable l0s  */
	ASPM_L1 = 2,		/* enable l1 */
	ASPM_L0S_L1 = 3,	/* enable l0s & l1*/
};

enum link_speed {
	GEN1 = 0,
	GEN2 = 1,
	GEN3 = 2,
};

enum l1ss_ctrl_state {
	L1SS_CLOSE = 0x0,		/*disable l1ss*/
	L1SS_PM_1_2 = 0x1,		/* pci-pm L1.2*/
	L1SS_PM_1_1 = 0x2,		/* pci-pm L1.1*/
	L1SS_PM_ALL = 0x3,		/* pci-pm L1.2 & L1.1*/
	L1SS_ASPM_1_2 = 0x4,	/* aspm L1.2 */
	L1SS_ASPM_1_1 = 0x8,	/* aspm L1.1 */
	L1SS_ASPM_ALL = 0xC,	/* aspm L1.2 & L1.1 */
	L1SS_PM_ASPM_ALL = 0xF,	/* aspm l1ss & pci-pm l1ss*/
};

enum dsm_err_id {
	DSM_ERR_POWER_ON = 1,
	DSM_ERR_ESTABLISH_LINK,
	DSM_ERR_ENUMERATE,
	DSM_ERR_LINK_DOWN,
};

enum RST_TYPE{
	RST_ENABLE = 1,
	RST_DISABLE = 2,
};

enum {
	DEVICE_LINK_MIN		= 0,
	DEVICE_LINK_UP		= 1,
	DEVICE_LINK_ABNORMAL	= 2,
	DEVICE_LINK_MAX		= 3,
};

#ifdef __SLT_FEATURE__
enum pcie_test_result {
	RESULT_OK,
	ERR_DATA_TRANS,
	ERR_L0S,
	ERR_L1,
	ERR_L0S_L1,
	ERR_L1_1,
	ERR_L1_2,
	ERR_OTHER
};
enum pcie_voltage {
      NORMAL_VOL,
      LOW_VOL
};
#endif

enum {
	POWEROFF_BUSON		= 0x0,
	POWERON			= 0x1,
	POWEROFF_BUSDOWN	= 0x2,
	POWER_MAX		= 0x3,
};

#define PCIE_PR_ERR(fmt, args ...) \
	do { \
		printk(KERN_ERR "[PCIE]%s:" fmt "\n", \
		__FUNCTION__, ##args); \
	} while (0)

#define PCIE_PR_INFO(fmt, args ...)	\
	do { \
		printk(KERN_INFO "[PCIE]%s:" fmt "\n", \
		__FUNCTION__, ##args); \
	} while (0)

#define PCIE_PR_DEBUG(fmt, args ...) \
	do { \
		printk(KERN_DEBUG "[PCIE]%s:" fmt "\n", \
		__FUNCTION__, ##args); \
	} while (0)

extern struct kirin_pcie g_kirin_pcie[];
extern unsigned int g_rc_num;

void kirin_elb_writel(struct kirin_pcie *pcie, u32 val, u32 reg);

u32 kirin_elb_readl(struct kirin_pcie *pcie, u32 reg);

/*Registers in PCIePHY*/

void kirin_apb_phy_writel(struct kirin_pcie *pcie, u32 val, u32 reg);

u32 kirin_apb_phy_readl(struct kirin_pcie *pcie, u32 reg);

void kirin_natural_phy_writel(struct kirin_pcie *pcie, u32 val, u32 reg);

u32 kirin_natural_phy_readl(struct kirin_pcie *pcie, u32 reg);

void kirin_sram_phy_writel(struct kirin_pcie *pcie, u32 val, u32 reg);

u32 kirin_sram_phy_readl(struct kirin_pcie *pcie, u32 reg);

void kirin_pcie_readl_rc(struct pcie_port *pp,
					void __iomem *dbi_base, u32 *val);
void kirin_pcie_writel_rc(struct pcie_port *pp,
					u32 val, void __iomem *dbi_base);
int kirin_pcie_rd_own_conf(struct pcie_port *pp, int where, int size,
				u32 *val);
int kirin_pcie_wr_own_conf(struct pcie_port *pp, int where, int size,
				u32 val);
void kirin_pcie_get_eyeparam(struct kirin_pcie *pcie, struct platform_device *pdev);
int kirin_pcie_power_on(struct pcie_port *pp, enum rc_power_status on_flag);
int kirin_pcie_enumerate(u32 rc_idx);
void kirin_pcie_iso_ctrl(struct kirin_pcie *pcie, int en_flag);
int kirin_pcie_turn_off(struct pcie_port *pp, enum rc_power_status on_flag);
int kirin_pcie_turn_on(struct pcie_port *pp, enum rc_power_status on_flag);
int kirin_pcie_lp_ctrl(u32 rc_idx, u32 enable);
void dsm_pcie_dump_info(struct kirin_pcie *pcie, enum dsm_err_id id);
void dsm_pcie_clear_info(void);
void kirin_pcie_reset_ctrl(struct kirin_pcie *pcie, enum RST_TYPE rst);
int kirin_pcie_perst_cfg(struct kirin_pcie *pcie, int pull_up);
int32_t kirin_pcie_get_dtsinfo(u32 *rc_id, struct platform_device *pdev);

#if defined(CONFIG_KIRIN_PCIE_EYEPARAM)
void pcie_debug_init(void *dev_data);
#endif

#if defined(CONFIG_PCIE_KIRIN_SLT)
int pcie_slt_wlan_on(struct kirin_pcie *pcie, int on);
#endif
#endif

