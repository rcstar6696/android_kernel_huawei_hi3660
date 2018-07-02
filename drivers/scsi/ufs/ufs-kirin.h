/* Copyright (c) 2013-2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef UFS_KIRIN_H_
#define UFS_KIRIN_H_

#define HBRN8_POLL_TOUT_MS      1000

/* Device ID */
#define DEV_ID        0x00
#define PEER_DEV_ID   0x01
#define PEER_CPORT_ID 0x00
#define TRAFFIC_CLASS 0x00

/*
 * sctrl specific define
 */
#define SCDEEPSLEEPED_OFFSET		(0x008)
#define MASK_UFS_MPHY_RHOLD             (0x1 << (11 + 16))
#define BIT_UFS_MPHY_RHOLD              (0x1 << 11)
#define EFUSE_RHOLD_BIT                 (0x1 << 22)

#define BIT_SYS_UFS_CLK_TYPE_OFF	(0x6)
#define BIT_SYS_UFS_CLK_TYPE_MASK	(0x3 << BIT_SYS_UFS_CLK_TYPE_OFF)

/*
 * pmctrl specific define
 */
#define PPLL3CTRL0_OFFSET	(0x048)
#define PPLL3CTRL1_OFFSET	(0x04C)

#define PPLL3_LOCK		UFS_BIT(26)
/*
 * pctrl specific define
 */
#define PCTRL_PERI_CTRL3_OFFSET	(0x010)
#define UFS_TCXO_EN_WITH_MASK	(UFS_BIT(0) | UFS_BIT(16))
/*
 * pericrg specific define
 */
#define PEREN5_OFFSET		(0x050)
#define PERDIS5_OFFSET		(0x054)
#define PERRSTEN3_OFFSET	(0x084)
#define PERRSTDIS3_OFFSET	(0x088)
#define PERRSTSTAT3_OFFSET	(0x08C)
#define CLKDIV16_OFFSET		(0x0E8)
#define CLKDIV17_OFFSET		(0x0EC)
#define CLKDIV21_OFFSET		(0x0FC)
#define PEREN7_OFFSET		(0x420)
#define PERDIS7_OFFSET		(0x424)
#define PERCLKEN7_OFFSET	(0x428)
#define UFS_ARESET		UFS_BIT(7)
#define RST_UFS			UFS_BIT(12)
//#define RST_UFSIO		UFS_BIT(30)
#define CLK_UFSPHY		UFS_BIT(12)
#define CLK_UFSIO		UFS_BIT(14)
#define CLK_UFS_SUBSYS		UFS_BIT(21)
#define CLK_ABB_BACKUP		UFS_BIT(22)
/* USE_HISI_MPHY_TC */
/* LPMCU: 0xB4000000, A53: 0xF4000000, I2C: 0x0 */
#define BASE_BUS_ADDR         ( 0x0)
#define REG_SC_APB_IF         ( BASE_BUS_ADDR + 0x00023000)
#define SCNOCSTA              ( REG_SC_APB_IF + 0xa0)
#define NOCSTA                ( BASE_BUS_ADDR + 0x00026000)
#define SCPPLLCTRL0           ( REG_SC_APB_IF + 0x8)
#define SCPPLLCTRL1           ( REG_SC_APB_IF + 0xc)
#define SCPPLLSTAT            ( REG_SC_APB_IF + 0x14)
#define SC_CLKEN              ( REG_SC_APB_IF + 0x18)
#define SC_RSTEN              ( REG_SC_APB_IF + 0x30)
#define SC_RSTDIS             ( REG_SC_APB_IF + 0x34)
#define SC_RSTSTAT            ( REG_SC_APB_IF + 0x38)
#define SC_CLK_DIV            ( REG_SC_APB_IF + 0x84)
#define SC_INT_MASK_EN        ( REG_SC_APB_IF + 0x70)
#define SC_UFS_REFCLK_RST_PAD ( REG_SC_APB_IF + 0x90)

/*
 * ufs sysctrl specific define
 */
#define PSW_POWER_CTRL			(0x04)
#define PHY_ISO_EN			(0x08)
#define HC_LP_CTRL			(0x0C)
#define PHY_CLK_CTRL			(0x10)
#define PSW_CLK_CTRL			(0x14)
#define CLOCK_GATE_BYPASS		(0x18)
#define RESET_CTRL_EN			(0x1C)
#define PHY_RESET_STATUS		(0x28)
#define PHY_MPX_TEST_CTRL		(0x30)
#define PHY_MPX_TEST_OBSV		(0x34)
#define UFS_SYSCTRL			(0x5C)
#define UFS_DEVICE_RESET_CTRL           (0x60)
#define UFS_APB_ADDR_MASK		(0x64)
#define UFS_CRG_UFS_CFG			(0x7C)

#define BIT_UFS_PSW_ISO_CTRL		(1 << 16)
#define BIT_UFS_PSW_MTCMOS_EN		(1 << 0)
#define BIT_UFS_REFCLK_ISO_EN		(1 << 16)
#define BIT_UFS_PHY_ISO_CTRL		(1 << 0)
#define BIT_SYSCTRL_LP_ISOL_EN		(1 << 16)
#define BIT_SYSCTRL_LP_PWR_GATE		(1 << 0)
#define BIT_SYSCTRL_PWR_READY		(1 << 8)
#define BIT_SYSCTRL_REF_CLOCK_EN	(1 << 24)
#define MASK_SYSCTRL_REF_CLOCK_SEL	(0x3 << 8)
#define MASK_SYSCTRL_CFG_CLOCK_FREQ	(0xFF)
#define UFS_FREQ_CFG_CLK                (0x39)
#define BIT_SYSCTRL_PSW_CLK_EN		(1 << 4)
#define MASK_UFS_CLK_GATE_BYPASS	(0x3F)
#define BIT_STATUS_LP_RESETCOMPLETE	(1 << 0)
#define BIT_SYSCTRL_LP_RESET_N		(1 << 0)
#define BIT_UFS_REFCLK_SRC_SEl		(1 << 0)
#define MASK_UFS_SYSCRTL_BYPASS		(0x3F << 16)
#define MASK_UFS_DEVICE_RESET		(0x1 << 16)
#define BIT_UFS_DEVICE_RESET		(0x1)

/*
 * KIRIN UFS HC specific Registers
 */
enum {
	UFS_REG_OCPTHRTL = 0xc0,
	UFS_REG_OOCPR    = 0xc4,

	UFS_REG_CDACFG   = 0xd0,
	UFS_REG_CDATX1   = 0xd4,
	UFS_REG_CDATX2   = 0xd8,
	UFS_REG_CDARX1   = 0xdc,
	UFS_REG_CDARX2   = 0xe0,
	UFS_REG_CDASTA   = 0xe4,

	UFS_REG_LBMCFG   = 0xf0,
	UFS_REG_LBMSTA   = 0xf4,
	UFS_REG_UFSMODE  = 0xf8,

	UFS_REG_HCLKDIV  = 0xfc,
};

/*
 * KIRIN UFS SYSCTRL Register
 */
enum {
	UFSSYS_MEMORY_CTRL			= 0x0000,
	UFSSYS_PSW_POWER_CTRL			= 0x0004,
	UFSSYS_PHY_ISO_EN			= 0x0008,
	UFSSYS_HC_LP_CTRL			= 0x000C,
	UFSSYS_PHY_CLK_CTRL			= 0x0010,
	UFSSYS_PSW_CLK_CTRL			= 0x0014,
	UFSSYS_CLOCK_GATE_BYPASS		= 0x0018,
	UFSSYS_RESET_CTRL_EN			= 0x001C,
	UFSSYS_PHY_RESET_STATUS			= 0x0028,
	UFSSYS_HC_DEBUG				= 0x002C,
	UFSSYS_PHY_MPX_TEST_CTRL		= 0x0030,
	UFSSYS_PHY_MPX_TEST_OBSV		= 0x0034,
	UFSSYS_PHY_DTB_OUT			= 0x0038,
	UFSSYS_DEBUG_MONITOR_HH			= 0x003C,
	UFSSYS_DEBUG_MONITOR_H			= 0x0040,
	UFSSYS_DEBUG_MONITOR_L			= 0x0044,
	UFSSYS_DEBUG_MONITORUP_H		= 0x0048,
	UFSSYS_DEBUG_MONITORUP_L		= 0x004C,
	UFSSYS_MK2_CTRL				= 0x0050,
	UFSSYS_UFS_SYSCTRL			= 0x005C,
	UFSSYS_UFS_RESET_CTRL			= 0x0060,
	UFSSYS_UFS_UMECTRL			= 0x0064,
	UFSSYS_DEBUG_MONITOR_UME_HH		= 0x0068,
	UFSSYS_DEBUG_MONITOR_UME_H		= 0x006C,
	UFSSYS_DEBUG_MONITOR_UME_L		= 0x0070,
	UFSSYS_UFS_MEM_CLK_GATE_BYPASS		= 0x0074,
	UFSSYS_CRG_UFS_CFG			= 0x007C,
	UFSSYS_CRG_UFS_CFG1			= 0x0080,
	UFSSYS_UFSAXI_W_QOS_LMTR		= 0x0084,
	UFSSYS_UFSAXI_R_QOS_LMTR		= 0x0088,
	UFSSYS_CRG_UFS_STAT			= 0x00A8,
};

/*
 * KIRIN UFS Crypto Registers
 */
enum {
	UFS_REG_CCAP = 0x100,

	UFS_REG_CRYPTOCAP_0 = 0x104,
	UFS_REG_CRYPTOCAP_1 = 0x108,

	UFS_REG_CRYPTOCFG_0_0 = 0x500,
	UFS_REG_CRYPTOCFG_0_1 = 0x504,
	UFS_REG_CRYPTOCFG_0_2 = 0x508,
	UFS_REG_CRYPTOCFG_0_3 = 0x50c,
	UFS_REG_CRYPTOCFG_0_4 = 0x510,
	UFS_REG_CRYPTOCFG_0_5 = 0x514,
	UFS_REG_CRYPTOCFG_0_6 = 0x518,
	UFS_REG_CRYPTOCFG_0_7 = 0x51c,
	UFS_REG_CRYPTOCFG_0_8 = 0x520,
	UFS_REG_CRYPTOCFG_0_9 = 0x524,
	UFS_REG_CRYPTOCFG_0_10 = 0x528,
	UFS_REG_CRYPTOCFG_0_11 = 0x52c,
	UFS_REG_CRYPTOCFG_0_12 = 0x530,
	UFS_REG_CRYPTOCFG_0_13 = 0x534,
	UFS_REG_CRYPTOCFG_0_14 = 0x538,
	UFS_REG_CRYPTOCFG_0_15 = 0x53c,
	UFS_REG_CRYPTOCFG_0_16 = 0x540,
};

/* Here external BL31 function declaration for UFS inline encrypt */
/* Now it is a test magic number */
//#define RPMB_SVC_UFS_TEST		(0xc500bbb0)
#define RPMB_SVC_UFS_TEST             (0xc600FFF5)

#define UFS_AHIT_AUTOH8_TIMER           (0x1001)

/*use #0~29 key index*/
#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO
#define MAX_CRYPTO_KEY_INDEX        (30)
#endif

/* REG UFS_REG_OCPTHRTL definition */
#define LP_PGE UFS_BIT(16)
#define LP_AH8_PGE UFS_BIT(17)

#define UFS_HCLKDIV_NORMAL_VALUE	0xE4
#define UFS_HCLKDIV_FPGA_VALUE		0x28

/* KIRIN UFS Unipro specific Registers */
#define VS_ULPH8_Cntrl 0xd0af
#define Ulp_Ulp_CtrlMode UFS_BIT(3)

#define UFS_KIRIN_LIMIT_NUM_LANES_RX	2
#define UFS_KIRIN_LIMIT_NUM_LANES_TX	2
#define UFS_KIRIN_LIMIT_HSGEAR_RX	UFS_HS_G1
#define UFS_KIRIN_LIMIT_HSGEAR_TX	UFS_HS_G1
#define UFS_KIRIN_LIMIT_PWMGEAR_RX	UFS_PWM_G4
#define UFS_KIRIN_LIMIT_PWMGEAR_TX	UFS_PWM_G4
#define UFS_KIRIN_LIMIT_RX_PWR_PWM	SLOW_MODE
#define UFS_KIRIN_LIMIT_TX_PWR_PWM	SLOW_MODE
#define UFS_KIRIN_LIMIT_RX_PWR_HS	FASTAUTO_MODE
#define UFS_KIRIN_LIMIT_TX_PWR_HS	FASTAUTO_MODE
#define UFS_KIRIN_LIMIT_HS_RATE		PA_HS_MODE_A
#define UFS_KIRIN_LIMIT_DESIRED_MODE	FAST

#define KIRIN_CAP_RESERVED	UFS_BIT(0)
#define USE_SNPS_MPHY_TC	UFS_BIT(1)
#define USE_FPGA_BOARD_CLK	UFS_BIT(2)
#define USE_RATE_B		UFS_BIT(3)
#define BROKEN_FASTAUTO		UFS_BIT(4)
#define USE_ONE_LANE		UFS_BIT(5)
#define USE_HS_GEAR3		UFS_BIT(6)
#define USE_HS_GEAR2		UFS_BIT(7)
#define USE_HS_GEAR1		UFS_BIT(8)
#define USE_AUTO_H8		UFS_BIT(9)
#define BROKEN_CLK_GATE_BYPASS	UFS_BIT(10)
#define USE_HISI_MPHY_TC	UFS_BIT(11)
#define USE_SINCE_BOSTON_CS	UFS_BIT(12)
#define USE_SYSBUS_207M		UFS_BIT(13)
#define PREPARE_MPHY_CALIB_AUTO UFS_BIT(14)

struct ufs_kirin_host {
	struct ufs_hba *hba;

	void __iomem *ufs_sys_ctrl;
	void __iomem *pctrl;
	void __iomem *pericrg;
	void __iomem *pmctrl;
	void __iomem *sysctrl;

	struct clk *clk_ufsio_ref;

	uint64_t caps;

	u32 avail_ln_rx;
	u32 avail_ln_tx;

	u32 busthrtl_backup;
	u32 reset_gpio;

	u32 tx_equalizer;

	struct ufs_pa_layer_attr dev_req_params;

	int chipsel_gpio;
	struct i2c_client *i2c_client;

	bool mr_workround_enable;
	bool need_memrepair;

	int in_suspend;
	u32 dly_ms_after_rst;
};

struct st_register_dump {
	uint32_t addr;
	char *name;
};

#define ufs_kirin_is_link_off(hba) ufshcd_is_link_off(hba)
#define ufs_kirin_is_link_active(hba) ufshcd_is_link_active(hba)
#define ufs_kirin_is_link_hibern8(hba) ufshcd_is_link_hibern8(hba)

#define ufs_sys_ctrl_writel(host, val, reg)                                    \
	writel((val), (host)->ufs_sys_ctrl + (reg))
#define ufs_sys_ctrl_readl(host, reg) readl((host)->ufs_sys_ctrl + (reg))
#define ufs_sys_ctrl_set_bits(host, mask, reg)                                 \
	ufs_sys_ctrl_writel(                                                   \
		(host), ((mask) | (ufs_sys_ctrl_readl((host), (reg)))), (reg))
#define ufs_sys_ctrl_clr_bits(host, mask, reg)                                 \
	ufs_sys_ctrl_writel((host),                                            \
			    ((~(mask)) & (ufs_sys_ctrl_readl((host), (reg)))), \
			    (reg))

#define ufs_pctrl_writel(host, val, reg) writel((val), (host)->pctrl + (reg))
#define ufs_pctrl_readl(host, reg) readl((host)->pctrl + (reg))

#define ufs_pericrg_writel(host, val, reg)                                     \
	writel((val), (host)->pericrg + (reg))
#define ufs_pericrg_readl(host, reg) readl((host)->pericrg + (reg))

#define ufs_pmctrl_writel(host, val, reg) writel((val), (host)->pmctrl + (reg))
#define ufs_pmctrl_readl(host, reg) readl((host)->pmctrl + (reg))

#define ufs_sctrl_writel(host, val, reg) writel((val), (host)->sysctrl + (reg))
#define ufs_sctrl_readl(host, reg) readl((host)->sysctrl + (reg))

#endif /* UFS_KIRIN_H_ */
