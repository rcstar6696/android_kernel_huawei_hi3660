/*
 * Copyright (c) 2013-2015, Linux Foundation. All rights reserved.
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

#include <linux/bootdevice.h>
#include <linux/time.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <soc_crgperiph_interface.h>
#include <linux/gpio.h>
#include <soc_sctrl_interface.h>
#include <soc_ufs_sysctrl_interface.h>
#include <linux/hisi/hisi_idle_sleep.h>
#include <linux/vmalloc.h>
#include <linux/kthread.h>
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include "ufshcd.h"
#include "unipro.h"
#include "ufs-kirin.h"
#include "ufshci.h"
#include "../scsi_logging.h"
#include "dsm_ufs.h"
#ifdef CONFIG_HISI_UFS_MANUAL_BKOPS
#include "hisi_ufs_bkops.h"
#endif

struct st_caps_map {
	char *caps_name;
	uint64_t cap_bit;
};
extern unsigned int scsi_logging_level;
#ifdef CONFIG_SCSI_UFS_KIRIN_V21
static void ufs_kirin_v21_soc_init(struct ufs_hba *hba);
#endif

#define LOGGING_LEVEL(name, level)                                             \
	((level & ((1 << (SCSI_LOG_##name##_BITS)) - 1))                       \
	 << (SCSI_LOG_##name##_SHIFT))

#define UFS_SCSI_LOGGING_LEVEL                                                 \
	(LOGGING_LEVEL(ERROR, 5) | LOGGING_LEVEL(TIMEOUT, 1) |                 \
	 LOGGING_LEVEL(SCAN, 1) | LOGGING_LEVEL(MLQUEUE, 1) |                  \
	 LOGGING_LEVEL(MLCOMPLETE, 1) | LOGGING_LEVEL(LLQUEUE, 1) |            \
	 LOGGING_LEVEL(LLCOMPLETE, 1) | LOGGING_LEVEL(HLQUEUE, 1) |            \
	 LOGGING_LEVEL(HLCOMPLETE, 1) | LOGGING_LEVEL(IOCTL, 1))
static char ufs_product_name[32] = {0};
static int __init early_parse_ufs_product_name_cmdline(char *arg)
{
	if (arg) {
		strncpy(ufs_product_name, arg, strnlen(arg, sizeof(ufs_product_name)));
		pr_info("cmdline ufs_product_name=%s\n", ufs_product_name);
	} else {
		pr_info("no ufs_product_name cmdline\n");
	}
	return 0;
}
/*lint -e528 -esym(528,*)*/
early_param("ufs_product_name", early_parse_ufs_product_name_cmdline);
/*lint -e528 +esym(528,*)*/
/* Here external BL31 function declaration for UFS inline encrypt*/
#ifdef CONFIG_SCSI_UFS_INLINE_CRYPTO
noinline int atfd_hisi_uie_smc(u64 function_id, u64 arg0, u64 arg1, u64 arg2)
{
	asm volatile(
		__asmeq("%0", "x0")
		__asmeq("%1", "x1")
		__asmeq("%2", "x2")
		__asmeq("%3", "x3")
		"smc    #0\n"
	: "+r" (function_id)
	: "r" (arg0), "r" (arg1), "r" (arg2));

	return (int)function_id;
}
#endif

static u64 kirin_ufs_dma_mask = DMA_BIT_MASK(64);/*lint !e598 !e648*/
#ifdef CONFIG_SCSI_UFS_KIRIN_V21
#define UFS_I2C_SLAVE (0x5A)
#define UFS_I2C_BUS 3

/*lint -e785*/
static struct i2c_board_info ufs_i2c_board_info = {
    /* FIXME*/
    .type = "i2c_ufs",
    .addr = UFS_I2C_SLAVE,
};
/*lint +e785*/
static int ufs_i2c_writel(struct ufs_hba *hba, u32 val, u32 addr)
{
	struct ufs_kirin_host *host = hba->priv;
	int ret = -1;
	union i2c_fmt {
		unsigned char chars[8];
		u32 addr_val[2];
	} data;
	data.addr_val[0] = cpu_to_be32(addr);
	data.addr_val[1] = val;
	if (host->i2c_client) {
		ret = i2c_master_send(host->i2c_client, (char *)data.chars,
				      (int)sizeof(union i2c_fmt));
		if (ret < 0)
			pr_err("%s ufs_i2c_write fail\n", __func__);
	} else
		pr_err("%s ufs_i2c_write fail client empty\n", __func__);
	return ret;
}

static int ufs_i2c_readl(struct ufs_hba *hba, u32 *value, u32 addr)
{
	int ret = -1;
	struct ufs_kirin_host *host = hba->priv;

	u32 temp = cpu_to_be32(addr);
	if (host->i2c_client) {
		ret = i2c_master_recv(host->i2c_client, (char *)(&temp),
				      (int)sizeof(u32));
		if (ret < 0)
			pr_err("%s ufs_i2c_readl fail\n", __func__);
	} else
		pr_err("%s ufs_i2c_readl fail client empty\n", __func__);
	*value = temp;
	return ret;
}
#endif

static void kirin_ufs_uic_log(struct ufs_hba *hba)
{
	unsigned int reg, reg_lane1, index;

	struct st_register_dump unipro_reg[] = {
		{0x15A70000, "PA_Hibern8Time"},
		{0x15AA0000, "PA_Granularity"},
		{0x15c00000, "PA_PACPFrameCount"},
		{0x15c10000, "PA_PACPErrorCount"},

		{0xD0300000, "DME_HibernateEnter"},
		{0xD0310000, "DME_HibernateEnterInd"},
		{0xD0320000, "DME_HibernateExit"},
		{0xD0330000, "DME_HibernateExitInd"},
		{0xD0600000, "DME_ErrorPHYInd"},
		{0xD0610000, "DME_ErrorPAInd"},
		{0xD0620000, "DME_ErrorDInd"},
		{0xD0630000, "DME_ErrorNInd"},
		{0xD0640000, "DME_ErrorTInd"},
		{0xD0820000, "VS_L2Status"},
		{0xD0830000, "VS_PowerState"},
		{0xd0920000, "VS_DebugTxByteCount"},
		{0xd0930000, "VS_DebugRxByteCount"},
		{0xd0940000, "VS_DebugInvalidByteEnable"},
		{0xd0950000, "VS_DebugLinkStartup"},
		{0xd0960000, "VS_DebugPwrChg"},
		{0xd0970000, "VS_DebugStates"},
		{0xd0980000, "VS_DebugCounter0"},
		{0xd0990000, "VS_DebugCounter1"},
		{0xd09a0000, "VS_DebugCounter0Mask"},
		{0xd09b0000, "VS_DebugCounter1Mask"},
		{0xd09d0000, "VS_DebugCounterOverflow"},
		{0xd09f0000, "VS_DebugCounterBMask"},
		{0xd0a00000, "VS_DebugSaveConfigTime"},
		{0xd0a10000, "VS_DebugLoopback"},
	};

	struct st_register_dump tx_phy[] = {
		{0x00210000, "TX_MODE"},
		{0x00220000, "TX_HSRATE_SERIES"},
		{0x00230000, "TX_HSGEAR"},
		{0x00240000, "TX_PWMGEAR"},
		{0x00410000, "TX_FSM_STATE"},
	};

	struct st_register_dump rx_phy[] = {
		{0x00A10000, "RX_MODE"},
		{0x00A20000, "RX_HSRATE_SERIES"},
		{0x00A30000, "RX_HSGEAR"},
		{0x00A40000, "RX_PWMGEAR"},
		{0x00C10000, "RX_FSM_STATE"},
	};

	for (index = 0;
		index < (sizeof(unipro_reg) / sizeof(struct st_register_dump));
		index++) {
		/* dont print more info if one uic cmd failed */
		if (ufshcd_dme_get(hba, unipro_reg[index].addr, &reg))
			goto out;

		dev_err(hba->dev, ": %s: 0x%08x\n", unipro_reg[index].name,
			reg);
	}

	for (index = 0;
		index < (sizeof(tx_phy) / sizeof(struct st_register_dump));
		index++) {
		/* dont print more info if one uic cmd failed */
		if (ufshcd_dme_get(hba, tx_phy[index].addr, &reg))
			goto out;

		if (ufshcd_dme_get(hba, tx_phy[index].addr | 0x1, &reg_lane1))
			goto out;

		dev_err(hba->dev, ": %s: LANE0: 0x%08x, LANE1: 0x%08x\n",
			tx_phy[index].name, reg, reg_lane1);
	}

	for (index = 0;
		index < (sizeof(rx_phy) / sizeof(struct st_register_dump));
		index++) {
		/* dont print more info if one uic cmd failed */
		if (ufshcd_dme_get(hba, rx_phy[index].addr | 0x4, &reg))
			goto out;

		if (ufshcd_dme_get(hba, rx_phy[index].addr | 0x5, &reg_lane1))
			goto out;

		dev_err(hba->dev, ": %s: LANE0: 0x%08x, LANE1: 0x%08x\n",
			rx_phy[index].name, reg, reg_lane1);
	}

out:
	return;
}

static void kirin_ufs_hci_log(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = hba->priv;

	dev_err(hba->dev, ": --------------------------------------------------- \n");
	dev_err(hba->dev, ": \t\tHCI STANDARD REGISTER DUMP\n");
	dev_err(hba->dev, ": --------------------------------------------------- \n");
	dev_err(hba->dev, ": CAPABILITIES:                 0x%08x\n", ufshcd_readl(hba, REG_CONTROLLER_CAPABILITIES));
	dev_err(hba->dev, ": UFS VERSION:                  0x%08x\n", ufshcd_readl(hba, REG_UFS_VERSION));
	dev_err(hba->dev, ": PRODUCT ID:                   0x%08x\n", ufshcd_readl(hba, REG_CONTROLLER_DEV_ID));
	dev_err(hba->dev, ": MANUFACTURE ID:               0x%08x\n", ufshcd_readl(hba, REG_CONTROLLER_PROD_ID));
	dev_err(hba->dev, ": INTERRUPT STATUS:             0x%08x\n", ufshcd_readl(hba, REG_INTERRUPT_STATUS));
	dev_err(hba->dev, ": INTERRUPT ENABLE:             0x%08x\n", ufshcd_readl(hba, REG_INTERRUPT_ENABLE));
	dev_err(hba->dev, ": CONTROLLER STATUS:            0x%08x\n", ufshcd_readl(hba, REG_CONTROLLER_STATUS));
	dev_err(hba->dev, ": CONTROLLER ENABLE:            0x%08x\n", ufshcd_readl(hba, REG_CONTROLLER_ENABLE));
	dev_err(hba->dev, ": UIC ERR PHY ADAPTER LAYER:    0x%08x\n", ufshcd_readl(hba, REG_UIC_ERROR_CODE_PHY_ADAPTER_LAYER));
	dev_err(hba->dev, ": UIC ERR DATA LINK LAYER:      0x%08x\n", ufshcd_readl(hba, REG_UIC_ERROR_CODE_DATA_LINK_LAYER));
	dev_err(hba->dev, ": UIC ERR NETWORK LATER:        0x%08x\n", ufshcd_readl(hba, REG_UIC_ERROR_CODE_NETWORK_LAYER));
	dev_err(hba->dev, ": UIC ERR TRANSPORT LAYER:      0x%08x\n", ufshcd_readl(hba, REG_UIC_ERROR_CODE_TRANSPORT_LAYER));
	dev_err(hba->dev, ": UIC ERR DME:                  0x%08x\n", ufshcd_readl(hba, REG_UIC_ERROR_CODE_DME));
	dev_err(hba->dev, ": UTP TRANSF REQ INT AGG CNTRL: 0x%08x\n", ufshcd_readl(hba, REG_UTP_TRANSFER_REQ_INT_AGG_CONTROL));
	dev_err(hba->dev, ": UTP TRANSF REQ LIST BASE L:   0x%08x\n", ufshcd_readl(hba, REG_UTP_TRANSFER_REQ_LIST_BASE_L));
	dev_err(hba->dev, ": UTP TRANSF REQ LIST BASE H:   0x%08x\n", ufshcd_readl(hba, REG_UTP_TRANSFER_REQ_LIST_BASE_H));
	dev_err(hba->dev, ": UTP TRANSF REQ DOOR BELL:     0x%08x\n", ufshcd_readl(hba, REG_UTP_TRANSFER_REQ_DOOR_BELL));
	dev_err(hba->dev, ": UTP TRANSF REQ LIST CLEAR:    0x%08x\n", ufshcd_readl(hba, REG_UTP_TRANSFER_REQ_LIST_CLEAR));
	dev_err(hba->dev, ": UTP TRANSF REQ LIST RUN STOP: 0x%08x\n", ufshcd_readl(hba, REG_UTP_TRANSFER_REQ_LIST_RUN_STOP));
	dev_err(hba->dev, ": UTP TASK REQ LIST BASE L:     0x%08x\n", ufshcd_readl(hba, REG_UTP_TASK_REQ_LIST_BASE_L));
	dev_err(hba->dev, ": UTP TASK REQ LIST BASE H:     0x%08x\n", ufshcd_readl(hba, REG_UTP_TASK_REQ_LIST_BASE_H));
	dev_err(hba->dev, ": UTP TASK REQ DOOR BELL:       0x%08x\n", ufshcd_readl(hba, REG_UTP_TASK_REQ_DOOR_BELL));
	dev_err(hba->dev, ": UTP TASK REQ LIST CLEAR:      0x%08x\n", ufshcd_readl(hba, REG_UTP_TASK_REQ_LIST_CLEAR));
	dev_err(hba->dev, ": UTP TASK REQ LIST RUN STOP:   0x%08x\n", ufshcd_readl(hba, REG_UTP_TASK_REQ_LIST_RUN_STOP));
	dev_err(hba->dev, ": UIC COMMAND:                  0x%08x\n", ufshcd_readl(hba, REG_UIC_COMMAND));
	dev_err(hba->dev, ": UIC COMMAND ARG1:             0x%08x\n", ufshcd_readl(hba, REG_UIC_COMMAND_ARG_1));
	dev_err(hba->dev, ": UIC COMMAND ARG2:             0x%08x\n", ufshcd_readl(hba, REG_UIC_COMMAND_ARG_2));
	dev_err(hba->dev, ": UIC COMMAND ARG3:             0x%08x\n", ufshcd_readl(hba, REG_UIC_COMMAND_ARG_3));
	dev_err(hba->dev, ": DWC BUSTHRTL:                 0x%08x\n", ufshcd_readl(hba, UFS_REG_OCPTHRTL));
	dev_err(hba->dev, ": DWC HCLKDIV:                  0x%08x\n", ufshcd_readl(hba, UFS_REG_HCLKDIV));

	dev_err(hba->dev, ": --------------------------------------------------- \n");
	dev_err(hba->dev, ": \t\tUFS SYSCTRL REGISTER DUMP\n");
	dev_err(hba->dev, ": --------------------------------------------------- \n");
	dev_err(hba->dev, ": UFSSYS_MEMORY_CTRL:             0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_MEMORY_CTRL));
	dev_err(hba->dev, ": UFSSYS_PSW_POWER_CTRL:          0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_PSW_POWER_CTRL));
	dev_err(hba->dev, ": UFSSYS_PHY_ISO_EN:              0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_PHY_ISO_EN));
	dev_err(hba->dev, ": UFSSYS_HC_LP_CTRL:              0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_HC_LP_CTRL));
	dev_err(hba->dev, ": UFSSYS_PHY_CLK_CTRL:            0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_PHY_CLK_CTRL));
	dev_err(hba->dev, ": UFSSYS_PSW_CLK_CTRL:            0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_PSW_CLK_CTRL));
	dev_err(hba->dev, ": UFSSYS_CLOCK_GATE_BYPASS:       0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_CLOCK_GATE_BYPASS));
	dev_err(hba->dev, ": UFSSYS_RESET_CTRL_EN:           0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_RESET_CTRL_EN));
	dev_err(hba->dev, ": UFSSYS_PHY_RESET_STATUS:        0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_PHY_RESET_STATUS));
	dev_err(hba->dev, ": UFSSYS_HC_DEBUG:                0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_HC_DEBUG));
	dev_err(hba->dev, ": UFSSYS_PHY_MPX_TEST_CTRL:       0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_PHY_MPX_TEST_CTRL));
	dev_err(hba->dev, ": UFSSYS_PHY_MPX_TEST_OBSV:       0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_PHY_MPX_TEST_OBSV));
	dev_err(hba->dev, ": UFSSYS_PHY_DTB_OUT:             0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_PHY_DTB_OUT));
	dev_err(hba->dev, ": UFSSYS_DEBUG_MONITOR_HH:        0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_DEBUG_MONITOR_HH));
	dev_err(hba->dev, ": UFSSYS_DEBUG_MONITOR_H:         0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_DEBUG_MONITOR_H));
	dev_err(hba->dev, ": UFSSYS_DEBUG_MONITOR_L:         0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_DEBUG_MONITOR_L));
	dev_err(hba->dev, ": UFSSYS_DEBUG_MONITORUP_H:       0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_DEBUG_MONITORUP_H));
	dev_err(hba->dev, ": UFSSYS_DEBUG_MONITORUP_L:       0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_DEBUG_MONITORUP_L));
	dev_err(hba->dev, ": UFSSYS_MK2_CTRL:                0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_MK2_CTRL));
	dev_err(hba->dev, ": UFSSYS_UFS_SYSCTRL:             0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_UFS_SYSCTRL));
#ifdef CONFIG_SCSI_UFS_KIRIN_V21
	dev_err(hba->dev, ": UFSSYS_UFS_RESET_CTRL:          0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_UFS_RESET_CTRL));
	dev_err(hba->dev, ": UFSSYS_UFS_UMECTRL:             0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_UFS_UMECTRL));
	dev_err(hba->dev, ": UFSSYS_DEBUG_MONITOR_UME_HH:    0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_DEBUG_MONITOR_UME_HH));
	dev_err(hba->dev, ": UFSSYS_DEBUG_MONITOR_UME_H:     0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_DEBUG_MONITOR_UME_H));
	dev_err(hba->dev, ": UFSSYS_DEBUG_MONITOR_UME_L:     0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_DEBUG_MONITOR_UME_L));
	dev_err(hba->dev, ": UFSSYS_UFS_MEM_CLK_GATE_BYPASS: 0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_UFS_MEM_CLK_GATE_BYPASS));
	dev_err(hba->dev, ": UFSSYS_CRG_UFS_CFG:             0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_CRG_UFS_CFG));
	dev_err(hba->dev, ": UFSSYS_CRG_UFS_CFG1:            0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_CRG_UFS_CFG1));
	dev_err(hba->dev, ": UFSSYS_UFSAXI_W_QOS_LMTR:       0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_UFSAXI_W_QOS_LMTR));
	dev_err(hba->dev, ": UFSSYS_UFSAXI_R_QOS_LMTR:       0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_UFSAXI_R_QOS_LMTR));
	dev_err(hba->dev, ": UFSSYS_CRG_UFS_STAT:            0x%08x\n", ufs_sys_ctrl_readl(host, UFSSYS_CRG_UFS_STAT));
#endif
}

static int ufs_kirin_check_hibern8(struct ufs_hba *hba)
{
	int err = 0;
	u32 tx_fsm_val_0 = 0;
	u32 tx_fsm_val_1 = 0;
	unsigned long timeout = jiffies + msecs_to_jiffies(HBRN8_POLL_TOUT_MS);

	do {
		err = ufshcd_dme_get(hba, UIC_ARG_MIB_SEL(MPHY_TX_FSM_STATE, 0),
				     &tx_fsm_val_0);
		err |= ufshcd_dme_get(hba, UIC_ARG_MIB_SEL(MPHY_TX_FSM_STATE, 1),
				     &tx_fsm_val_1);
		if (err || (tx_fsm_val_0 == TX_FSM_HIBERN8 && tx_fsm_val_1 == TX_FSM_HIBERN8))
			break;

		/* sleep for max. 200us */
		usleep_range(100, 200);
	} while (time_before(jiffies, timeout));

	/*
	 * we might have scheduled out for long during polling so
	 * check the state again.
	 */
	if (time_after(jiffies, timeout)) {
		err = ufshcd_dme_get(hba, UIC_ARG_MIB_SEL(MPHY_TX_FSM_STATE, 0),
				     &tx_fsm_val_0);
		err |= ufshcd_dme_get(hba, UIC_ARG_MIB_SEL(MPHY_TX_FSM_STATE, 1),
				     &tx_fsm_val_1);
	}

	if (err) {
		dev_err(hba->dev, "%s: unable to get TX_FSM_STATE, err %d\n",
			__func__, err);
	} else if (tx_fsm_val_0 != TX_FSM_HIBERN8 || tx_fsm_val_1 != TX_FSM_HIBERN8) {
		err = -1;
		dev_err(hba->dev, "%s: invalid TX_FSM_STATE, lane0 = %d, lane1 = %d\n",
			__func__, tx_fsm_val_0, tx_fsm_val_1);
	}

	return err;
}

#ifdef CONFIG_SCSI_UFS_INLINE_CRYPTO
static int ufs_kirin_uie_config_init(struct ufs_hba *hba)
{
	int reg_value = 0;
	int err = 0;

	/* enable UFS cryptographic operations on transactions */
	reg_value = ufshcd_readl(hba, REG_CONTROLLER_ENABLE);
	reg_value |= CRYPTO_GENERAL_ENABLE;
	ufshcd_writel(hba, reg_value, REG_CONTROLLER_ENABLE);

	/* Here UFS driver, which set SECURITY reg 0x1 in BL31,
	 * has the permission to write scurity key registers.
	 */
	err = atfd_hisi_uie_smc(RPMB_SVC_UFS_TEST, 0x0, 0x0, 0x0);
#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO
	if(err) {
		dev_err(hba->dev, "%s: first set ufs inline key failed,try again.\n", __func__);
		err = atfd_hisi_uie_smc(RPMB_SVC_UFS_TEST, 0x0, 0x0, 0x0);
		if(err)
			BUG_ON(1);
	}
#endif

	return err;
}

#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO
/*generate the key index use bkdrhash alg,we limit
 *the result in the range of 0~29 */
static u32 bkdrhash_alg(u8 *str, int len)
{
	u32 seed = 131;
	u32 hash = 0;
	int i;

	for (i = 0; i < len; i++) {
		hash = hash * seed + str[i];
	}

	return (hash & 0xFFFFFFFF);
}

#ifdef CONFIG_HISI_DEBUG_FS
static void test_generate_cci_dun_use_bkdrhash(u8 *key, int key_len)
{
	u32 crypto_cci;
	u64 dun;
	u32 hash_res;

	hash_res = bkdrhash_alg(key, key_len);
	crypto_cci = hash_res % MAX_CRYPTO_KEY_INDEX;
	dun = (u64)hash_res;
	pr_err("%s: ufs crypto key index is %d, dun is 0x%llx\n", __func__, crypto_cci, dun);
}
#endif
#else
/* the func to config key */
static void ufs_kirin_uie_key_prepare(struct ufs_hba *hba, int key_index, void *key)
{
#ifndef CONFIG_SCSI_UFS_KIRIN_V21
	struct ufs_kirin_host *host = hba->priv;
#endif
	int reg_value = 0;
	int key_cfg = 0;
	u32 key_reg_offset = 0;

#ifndef CONFIG_SCSI_UFS_KIRIN_V21
	/*
	 * when writing key reg of the number 22 ~ 31,
	 * we must set reg apb_addr of ufs_sys_ctrl
	 */
	if (key_index > 21) {
		ufs_sys_ctrl_writel(host, 0x10001, UFS_APB_ADDR_MASK);
		key_cfg = key_index - 22;
	} else
		key_cfg = key_index;
#else
	key_cfg = key_index;
#endif

	/* key operation start */
	reg_value = ufshcd_readl(hba, UFS_REG_CRYPTOCFG_0_16 + (key_cfg * 0x80));
	if ((reg_value >> 31) & 0x1) {
		/* TODO step 1st
		 * Verify that no pending transactions reference x-CRYPTOCFG
		 * in their CCI field, i.e. UTRD.CCI != x for all pending transactions
		 */

		/*step 2nd writing 0x0 to clear x-CRYPTOCFG reg*/
		ufshcd_writel(hba, 0, UFS_REG_CRYPTOCFG_0_16 + (key_cfg * 0x80));
	}

	/* step 3rd write the cryptographic key to x-CRYPTOKEY field
	 * The key is organized according to the algorithm-specific layout.
	 * Unused regions of CRYPTOKEY should be written with zeros.
	 * The key is written in little-endian format, sequentially
	 * and in one atomic set of operations.
	 */
	/* use the following way to  write key to improve efficiency */
	key_reg_offset = key_cfg * 0x80;
	memcpy(hba->key_reg_base + key_reg_offset, key, 64);
	mb();

	/* step 4th set x-CRYPTOCFG with CAPIDX, DUSIZE, and CFGE=1 */
	ufshcd_writel(hba, 0x80000108, UFS_REG_CRYPTOCFG_0_16 + (key_cfg * 0x80));
	/* key operation end */

#ifndef CONFIG_SCSI_UFS_KIRIN_V21
	/* clear reg apb_addr of ufs_sys_ctrl */
	if (key_index > 21)
		ufs_sys_ctrl_writel(host, 0x10000, UFS_APB_ADDR_MASK);
#endif
}
#endif

/* configure UTRD to enable cryptographic operations for this transaction. */
static void ufs_kirin_uie_utrd_prepare(struct ufs_hba *hba,
		struct ufshcd_lrb *lrbp)
{
	struct utp_transfer_req_desc *req_desc = lrbp->utr_descriptor_ptr;
	u32 dword_0, dword_1, dword_3;
	u64 dun;
	u32 crypto_enable;
	u32 crypto_cci;
#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO
	u32 hash_res;
#else
	unsigned long flags;
#endif
	/*
	 * According to UFS 2.1 SPEC
	 * decrypte incoming payload if the command is SCSI READ operation
	 * encrypte outgoing payload if the command is SCSI WRITE operation
	 * And Kirin UFS controller only support SCSI cmd as below:
	 * READ_6/READ_10/WRITE_6/WRITE_10
	 */
	switch (lrbp->cmd->cmnd[0]) {
	case READ_10:
	case WRITE_10:
		crypto_enable = UTP_REQ_DESC_CRYPTO_ENABLE;
		break;
	default:
		return;
	}

	if (lrbp->cmd->request && lrbp->cmd->request->ci_key) {
#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO
		hash_res = bkdrhash_alg((u8 *)lrbp->cmd->request->ci_key,
				lrbp->cmd->request->ci_key_len);
		crypto_cci = hash_res % MAX_CRYPTO_KEY_INDEX;

#ifdef CONFIG_HISI_DEBUG_FS
		if(hba->inline_debug_flag == DEBUG_LOG_ON)
			dev_err(hba->dev, "%s: key index is %d\n", __func__, crypto_cci);
#endif
#else
		crypto_cci = lrbp->task_tag;
		spin_lock_irqsave(hba->host->host_lock, flags);
		ufs_kirin_uie_key_prepare(hba, crypto_cci, lrbp->cmd->request->ci_key);
		spin_unlock_irqrestore(hba->host->host_lock, flags);
#endif
	} else {
		return;
	}

	dun = (u64)lrbp->cmd->request->bio->index;

#if defined(CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO) && defined(CONFIG_HISI_DEBUG_FS)
	if(hba->inline_debug_flag == DEBUG_LOG_ON) {
		dev_err(hba->dev, "%s: dun is 0x%llx\n", __func__, ((u64)hash_res) << 32 | dun);
	}
	if(hba->inline_debug_flag == DEBUG_CRYPTO_ON) {
		crypto_enable = UTP_REQ_DESC_CRYPTO_ENABLE;
	} else if(hba->inline_debug_flag == DEBUG_CRYPTO_OFF) {
		crypto_enable = 0x0;
	}
#endif

	dword_0 = crypto_enable | crypto_cci;
	dword_1 = (u32)(dun & 0xffffffff);
#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO
	dword_3 = (u32)((dun >> 32) | hash_res);
#else
	dword_3 = (u32)((dun >> 32) & 0xffffffff);
#endif

	req_desc->header.dword_0 |= cpu_to_le32(dword_0);
	req_desc->header.dword_1 = cpu_to_le32(dword_1);
	req_desc->header.dword_3 = cpu_to_le32(dword_3);
}
#endif

static void ufs_kirin_regulator_init(struct ufs_hba *hba)
{
	struct device *dev = hba->dev;

	hba->vreg_info.vcc =
		devm_kzalloc(dev, sizeof(struct ufs_vreg), GFP_KERNEL);
	if (!hba->vreg_info.vcc) {
		dev_err(dev, "vcc alloc error\n");
		goto error;
	}

	hba->vreg_info.vcc->reg = devm_regulator_get(dev, "vcc");
	if (!hba->vreg_info.vcc->reg) {
		dev_err(dev, "get regulator vcc failed\n");
		goto error;
	}

	if (regulator_set_voltage(hba->vreg_info.vcc->reg, 2950000, 2950000)) {
		dev_err(dev, "set vcc voltage failed\n");
		goto error;
	}

	if (regulator_enable(hba->vreg_info.vcc->reg))
		dev_err(dev, "regulator vcc enable failed\n");

error:
	return;
}

#if 0
void ufs_kirin_clk_init(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = hba->priv;
	u32 ufs_clk_type;
	u32 count = 0;

	ufs_pericrg_writel(host, CLK_UFSIO, PERDIS7_OFFSET);
	ufs_sys_ctrl_clr_bits(host, BIT_SYSCTRL_REF_CLOCK_EN, PHY_CLK_CTRL);
	if (ufs_sys_ctrl_readl(host, PHY_CLK_CTRL) & BIT_SYSCTRL_REF_CLOCK_EN)
		mdelay(1);
	ufs_clk_type = ufs_sctrl_readl(host, SCDEEPSLEEPED_OFFSET);
	ufs_clk_type = (ufs_clk_type & BIT_SYS_UFS_CLK_TYPE_MASK) >> BIT_SYS_UFS_CLK_TYPE_OFF;

	if (1 == ufs_clk_type) {
		/* use pll clk */
		pr_info("UFS use pll clk\n");
		ufs_sys_ctrl_set_bits(host, BIT_UFS_REFCLK_SRC_SEl, UFS_SYSCTRL);
		ufs_pericrg_writel(host, 0x01ff01ff, CLKDIV21_OFFSET);
		ufs_pmctrl_writel(host, 0x03000000, PPLL3CTRL1_OFFSET);
		if (0x03000000 != ufs_pmctrl_readl(host, PPLL3CTRL1_OFFSET))
			mdelay(1);
		ufs_pmctrl_writel(host, 0x00904005, PPLL3CTRL0_OFFSET);
		for (count = 0; count < 10; count++) {
			mdelay(1);
			if (PPLL3_LOCK & ufs_pmctrl_readl(host, PPLL3CTRL0_OFFSET))
				break;
		}
		ufs_pmctrl_writel(host, 0x07000000, PPLL3CTRL1_OFFSET);
		ufs_pericrg_writel(host, 0x01ff01bf, CLKDIV21_OFFSET);
		ufs_pericrg_writel(host, CLK_ABB_BACKUP, PEREN5_OFFSET);
	} else if (2 == ufs_clk_type) {
		/* use 19.2 clk */
		pr_info("UFS use 19.2 digtal clk\n");
		ufs_sys_ctrl_set_bits(host, BIT_UFS_REFCLK_SRC_SEl, UFS_SYSCTRL);
		ufs_pericrg_writel(host, 0x1c00040, CLKDIV21_OFFSET);
		ufs_pericrg_writel(host, CLK_ABB_BACKUP, PEREN5_OFFSET);
	} else {
		/* use abb clk */
		pr_info("UFS use abb clk\n");
		ufs_sys_ctrl_clr_bits(host, BIT_UFS_REFCLK_SRC_SEl, UFS_SYSCTRL);
		ufs_sys_ctrl_clr_bits(host, BIT_UFS_REFCLK_ISO_EN, PHY_ISO_EN);
		ufs_pctrl_writel(host, UFS_TCXO_EN_WITH_MASK, PCTRL_PERI_CTRL3_OFFSET);
		mdelay(1);
	}

	ufs_pericrg_writel(host, CLK_UFSIO, PEREN7_OFFSET); /* open device ref clock */
	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_REF_CLOCK_EN, PHY_CLK_CTRL); /* open mphy ref clk */
	return;
}
#else
static void ufs_kirin_clk_init(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = hba->priv;
	int ret;

	pr_info("%s ++\n", __func__);
	pr_info("UFS use abb clk\n");
#if 0
	ufs_pericrg_writel(host, CLK_UFSIO, PERDIS7_OFFSET);
#else
	ret = clk_prepare_enable(host->clk_ufsio_ref);
	if (ret) {
		pr_err("%s ,clk_prepare_enable failed\n", __func__);
		return;
	}
	clk_disable_unprepare(host->clk_ufsio_ref);
	#if 0
	if (CLK_UFSIO & ufs_pericrg_readl(host, PERCLKEN7_OFFSET)) {
		pr_err("%s:disable clk ref err. PERDIS7 = 0x%x\n", __func__,
			ufs_pericrg_readl(host, PERCLKEN7_OFFSET));
	}
	#endif
#endif
	ufs_sys_ctrl_clr_bits(host, BIT_SYSCTRL_REF_CLOCK_EN, PHY_CLK_CTRL);
	if (ufs_sys_ctrl_readl(host, PHY_CLK_CTRL) & BIT_SYSCTRL_REF_CLOCK_EN)
		mdelay(1);
	/* use abb clk */
	ufs_sys_ctrl_clr_bits(host, BIT_UFS_REFCLK_SRC_SEl, UFS_SYSCTRL);
	ufs_sys_ctrl_clr_bits(host, BIT_UFS_REFCLK_ISO_EN, PHY_ISO_EN);
#if 0
	ufs_pctrl_writel(host, UFS_TCXO_EN_WITH_MASK, PCTRL_PERI_CTRL3_OFFSET);
	mdelay(1);
	ufs_pericrg_writel(host, CLK_UFSIO, PEREN7_OFFSET); /* open device ref clock */
#else
	ret = clk_prepare_enable(host->clk_ufsio_ref);
	if (ret) {
		pr_err("%s ,clk_prepare_enable failed\n", __func__);
		return;
	}
	#if 0
	if (!(0x1 & ufs_pctrl_readl(host, PCTRL_PERI_CTRL3_OFFSET))) {
		pr_err("%s:enable clk ref err. PERI_CTRL3 = 0x%x\n", __func__,
			ufs_pctrl_readl(host, PCTRL_PERI_CTRL3_OFFSET));
	}
	if (!(CLK_UFSIO & ufs_pericrg_readl(host, PERCLKEN7_OFFSET))) {
		pr_err("%s:enable clk ref err. PERDIS7 = 0x%x\n", __func__,
			ufs_pericrg_readl(host, PERCLKEN7_OFFSET));
	}
	#endif
#endif
	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_REF_CLOCK_EN, PHY_CLK_CTRL); /* open mphy ref clk */
	pr_info("%s --\n", __func__);
	return;
}
#endif

static inline bool ufs_kirin_need_memrepair(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = hba->priv;

	if (!host->mr_workround_enable) {
		pr_err("%s mr_workround_enable is false\n", __func__);
		return false;
	}

	if ((BIT(0) & ufs_sctrl_readl(host, 0x3A0)) &&
	    (BIT(19) & ufs_sctrl_readl(host, 0x010))) {
		pr_err("%s need memrepair\n", __func__);
		return true;
	} else {
		pr_err("%s no need memrepair\n", __func__);
		return false;
	}
}

static void ufs_kirin_soc_init(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = (struct ufs_kirin_host *)hba->priv;
	u32 reg;

	pr_info("%s ++\n", __func__);

	ufs_pericrg_writel(host, RST_UFS, PERRSTEN3_OFFSET);

	ufs_sys_ctrl_set_bits(host, BIT_UFS_PSW_MTCMOS_EN, PSW_POWER_CTRL); /* HC_PSW powerup */
	udelay(10);
	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_PWR_READY, HC_LP_CTRL); /* notify PWR ready */
	ufs_sys_ctrl_writel(host, MASK_UFS_DEVICE_RESET | 0,
		UFS_DEVICE_RESET_CTRL);

	if (host->reset_gpio != 0xFFFFFFFF) {
		if (0 > gpio_direction_output(host->reset_gpio, 0))
			pr_err("%s: could not set gpio %d output push down\n", __func__, host->reset_gpio);
	}

	if (!(host->caps & USE_FPGA_BOARD_CLK)) {
		ufs_pericrg_writel(host, 0 | BIT(14 + 16), CLKDIV17_OFFSET); /* set hc hclk div */
		ufs_pericrg_writel(host, (0x3 << 9) | (0x3 << (9 + 16)), CLKDIV16_OFFSET); /* set mphy cfg clk div */
	}

	reg = ufs_sys_ctrl_readl(host, PHY_CLK_CTRL);
	reg = (reg & ~MASK_SYSCTRL_CFG_CLOCK_FREQ) | UFS_FREQ_CFG_CLK;
	ufs_sys_ctrl_writel(host, reg, PHY_CLK_CTRL); /* set cfg clk freq */
	ufs_sys_ctrl_clr_bits(host, MASK_SYSCTRL_REF_CLOCK_SEL, PHY_CLK_CTRL); /* set ref clk freq */
	/* bypass ufs clk gate */
	ufs_sys_ctrl_set_bits(host, MASK_UFS_CLK_GATE_BYPASS, CLOCK_GATE_BYPASS);
	ufs_sys_ctrl_set_bits(host, MASK_UFS_SYSCRTL_BYPASS, UFS_SYSCTRL);

	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_PSW_CLK_EN, PSW_CLK_CTRL); /* open psw clk */
	ufs_sys_ctrl_clr_bits(host, BIT_UFS_PSW_ISO_CTRL, PSW_POWER_CTRL); /* disable ufshc iso */
	ufs_sys_ctrl_clr_bits(host, BIT_UFS_PHY_ISO_CTRL, PHY_ISO_EN); /* disable phy iso */
	ufs_sys_ctrl_clr_bits(host, BIT_SYSCTRL_LP_ISOL_EN, HC_LP_CTRL); /* notice iso disable */
	ufs_pericrg_writel(host, UFS_ARESET, PERRSTDIS3_OFFSET); /* disable aresetn */
	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_LP_RESET_N, RESET_CTRL_EN); /* disable lp_reset_n */
	mdelay(1);

	if (host->reset_gpio != 0xFFFFFFFF) {
		if (0 > gpio_direction_output(host->reset_gpio, 1))
			pr_err("%s: could not set gpio %d output pull up\n", __func__, host->reset_gpio);
	}
	ufs_sys_ctrl_writel(host, MASK_UFS_DEVICE_RESET | BIT_UFS_DEVICE_RESET,
		UFS_DEVICE_RESET_CTRL);

	mdelay(20);

	/* enable the fix of linereset recovery, and enable rx_reset/tx_rest beat */
	/* enable ref_clk_en override(bit5) & override value = 1(bit4), with mask */
	ufs_sys_ctrl_writel(host, 0x03300330, UFS_DEVICE_RESET_CTRL);

	ufs_pericrg_writel(host, RST_UFS, PERRSTDIS3_OFFSET);
	if (ufs_pericrg_readl(host, PERRSTSTAT3_OFFSET) & RST_UFS)
		mdelay(1);

	host->need_memrepair = false;

	/*set SOC_SCTRL_SCBAKDATA11_ADDR ufs bit to 1 when init*/
	hisi_idle_sleep_vote(ID_UFS, 1);

	pr_info("%s --\n", __func__);
	return;
}

static void ufs_kirin_device_hw_reset(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = hba->priv;

	ufs_sys_ctrl_writel(host, MASK_UFS_DEVICE_RESET | 0,
			    UFS_DEVICE_RESET_CTRL);
	if (host->reset_gpio != 0xFFFFFFFF) {
		if (0 > gpio_direction_output(host->reset_gpio, 0))
			pr_err("%s: could not set gpio %d output push down\n", __func__, host->reset_gpio);
	}

	mdelay(1);

	if (host->reset_gpio != 0xFFFFFFFF) {
		if (0 > gpio_direction_output(host->reset_gpio, 1))
			pr_err("%s: could not set gpio %d output pull up\n", __func__, host->reset_gpio);
	}
	ufs_sys_ctrl_writel(host, MASK_UFS_DEVICE_RESET | BIT_UFS_DEVICE_RESET,
			    UFS_DEVICE_RESET_CTRL);
	/* some device need at least 40ms */
	mdelay(40);
}

/**
 * Soc init will reset host controller, all register value will lost
 * including memory address, doorbell and AH8 AGGR
 */
static void ufs_kirin_full_reset(struct ufs_hba *hba)
{
#ifndef CONFIG_SCSI_UFS_KIRIN_V21
	struct ufs_kirin_host *host = hba->priv;

	/*
	 * disable ref clock, clk_init will re-enable
	 */
	clk_disable_unprepare(host->clk_ufsio_ref);

	ufs_kirin_clk_init(hba);
#endif

#ifdef CONFIG_HUAWEI_UFS_DSM
	dsm_ufs_disable_volt_irq(hba);
#endif
	disable_irq(hba->irq);

	/* wait for 1s to be sure axi entered to idle state */
	msleep(1000);

#ifdef CONFIG_SCSI_UFS_KIRIN_V21
	ufs_kirin_v21_soc_init(hba);
#else
	ufs_kirin_soc_init(hba);
#endif
	enable_irq(hba->irq);
#ifdef CONFIG_HUAWEI_UFS_DSM
	dsm_ufs_enable_volt_irq(hba);
#endif
}

static void ufs_kirin_pre_hce_notify(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = (struct ufs_kirin_host *)hba->priv;

	BUG_ON(!host->pericrg || !host->ufs_sys_ctrl ||
	    !host->pctrl || !host->sysctrl || !host->pmctrl);

	return;
}


static int ufs_kirin_hce_enable_notify(struct ufs_hba *hba, bool status)
{
	int err = 0;

	switch (status) {/*lint !e483*/
	case PRE_CHANGE:
		ufs_kirin_pre_hce_notify(hba);
		break;
	case POST_CHANGE:
		break;
	default:
		dev_err(hba->dev, "%s: invalid status %d\n", __func__, status);
		err = -EINVAL;
		break;
	}
	return err;
}

#if 1
static void ufs_kirin_phy_init(struct ufs_kirin_host *host)
{
	struct ufs_hba *hba = host->hba;

	if (host->avail_ln_rx == 0 || host->avail_ln_tx == 0) {
		ufshcd_dme_get(hba, UIC_ARG_MIB(PA_AVAILRXDATALANES),
			       &host->avail_ln_rx);
		ufshcd_dme_get(hba, UIC_ARG_MIB(PA_AVAILTXDATALANES),
			       &host->avail_ln_tx);
		if (host->avail_ln_rx != host->avail_ln_tx)
			dev_warn(hba->dev, "available data lane is not "
					   "equal(rx:%d, tx:%d)\n",
				 host->avail_ln_rx, host->avail_ln_tx);
	}
}
#endif

/*lint -e648 -e845*/
uint16_t ufs_kirin_mphy_read(struct ufs_hba *hba, uint16_t addr)
{
	uint16_t result;
	uint32_t value;
	/*DME_SET(16'h8117, cr_para_addr.MSB );*/
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x8117), (addr & 0xFF00) >> 8);
	/*DME_SET(16'h8116, cr_para_addr.LSB);*/
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x8116), (addr & 0xFF));
	/*DME_SET(16'h811c, 0x0);*//*trigger read*/
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x811C), 0);
	/*DME_GET(16'h811b, read_data.MSB);*/
	ufshcd_dme_get(hba, UIC_ARG_MIB(0x811B), &value); /* Unipro VS_mphy_disable */
	result = (uint16_t)(value & 0xFF);
	result <<= 8;
	/*DME_GET(16'h811a, read_data.LSB);*/
	ufshcd_dme_get(hba, UIC_ARG_MIB(0x811A), &value); /* Unipro VS_mphy_disable */
	result |= (uint16_t)(value & 0xFF);
	return result;
}

void ufs_kirin_mphy_write(struct ufs_hba *hba, uint16_t addr, uint16_t value)
{
	/*DME_SET(16'h8117, cr_para_addr.MSB );*/
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x8117), (addr & 0xFF00) >> 8);
	/*DME_SET(16'h8116, cr_para_addr.LSB);*/
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x8116), (addr & 0xFF));
	/*DME_SET(16'h8119, cr_para_wr_data.MSB);*/
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x8119), (value & 0xFF00) >> 8);
	/*DME_SET(16'h8118, cr_para_wr_data.LSB );*/
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x8118), (value & 0xFF));
	/*DME_SET(16'h811c, 0x0);*//*trigger write*/
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x811C), 1);
}

#ifdef CONFIG_SCSI_UFS_HS_ERROR_RECOVER
void ufs_kirin_set_vol(struct ufs_hba *hba, int v_tx, int v_rx)
{
	pr_err("ufs v_tx:%d v_rx:%d\n", v_tx, v_rx);
	if ((v_rx > 0) && (v_rx < 4)) {
		ufs_kirin_mphy_write(hba, 0x004A, 0x0090);
		ufs_kirin_mphy_write(hba, 0x90cb, 0x0080);
		ufs_kirin_mphy_write(hba, 0x90ce, 0x0010);
		if (v_rx == 3) {
			ufs_kirin_mphy_write(hba, 0x0060, 0x36E4);
			ufs_kirin_mphy_write(hba, 0x0060, 0x36E0);
		}
		if (v_rx == 2) {
			ufs_kirin_mphy_write(hba, 0x0060, 0x3884);
			ufs_kirin_mphy_write(hba, 0x0060, 0x3880);
		}
		if (v_rx == 1) {
			ufs_kirin_mphy_write(hba, 0x0060, 0x3A24);
			ufs_kirin_mphy_write(hba, 0x0060, 0x3A20);
		}
		ufs_kirin_mphy_write(hba, 0x9005, 0x4000);
		ufs_kirin_mphy_write(hba, 0x9005, 0x0000);
	}
	if ((v_tx > 0) && (v_tx < 4)) {
		ufs_kirin_mphy_write(hba, 0x004A, 0x0090);
		ufs_kirin_mphy_write(hba, 0x90C3, 0x0010);
		ufs_kirin_mphy_write(hba, 0x90C9, 0x0001);
		ufs_kirin_mphy_write(hba, 0x90C5, 0x0002);

		if (v_tx == 3) {
			ufs_kirin_mphy_write(hba, 0x0060, 0x36E4);
			ufs_kirin_mphy_write(hba, 0x0060, 0x36E0);
		}
		if (v_tx == 2) {
			ufs_kirin_mphy_write(hba, 0x0060, 0x3884);
			ufs_kirin_mphy_write(hba, 0x0060, 0x3880);
		}
		if (v_tx == 1) {
			ufs_kirin_mphy_write(hba, 0x0060, 0x3A24);
			ufs_kirin_mphy_write(hba, 0x0060, 0x3A20);
		}
	}
}
#endif

/* hisi mphy testchip specific configuration */
/*lint -e648 -e845*/
static int ufs_kirin_dme_setup_hisi_mphy(struct ufs_hba *hba)
{
	uint32_t val = 0;
	u32 retry = 1000;

	pr_err("%s ++\n", __func__);
	while (retry--) {
		ufshcd_dme_get(hba, UIC_ARG_MIB_SEL(0xD010, 0x0), &val);
		if (val == 1)
			break;
		if (!retry)
			return -EIO;
	}
	/*DME layer enable*/
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xd000, 0x0), 0x1);
	pr_err("%s --\n", __func__);
	return 0;
}

/* snps mphy testchip specific configuration */
static int ufs_kirin_dme_setup_snps_mphy(struct ufs_hba *hba)
{
	uint32_t val;
	uint64_t retry = 1000;
	struct ufs_kirin_host *host = hba->priv;

	pr_info("%s ++\n", __func__);
	while (retry--) {
		ufshcd_dme_get(hba, UIC_ARG_MIB_SEL(0xD010, 0x0), &val);
		if (val == 1)
			break;
		if (!retry)
			return -EIO;
	}

	/* PLS put test UIC command here */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c0, 0x0), 0x24);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xd0a0, 0x0), 0x18);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xd09e, 0x0), 0x1);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f2, 0x4), 0x3);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f2, 0x5), 0x3);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00bd, 0x4), 0x16);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00bd, 0x5), 0x16);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c7, 0x4), 0x42);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c7, 0x5), 0x42);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x80f8, 0x0), 0x00);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f3, 0x4), 0xa4);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f3, 0x4), 0xa4);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00b4, 0x4), 0x1);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00b4, 0x5), 0x1);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f4, 0x4), 0x1);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f4, 0x5), 0x1);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00e9, 0x4), 0x28);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00e9, 0x5), 0x28);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00b5, 0x4), 0x1e);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00b5, 0x5), 0x1e);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00bf, 0x4), 0x2f);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00bf, 0x5), 0x2f);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x80e6, 0x0), 0x4);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x80cd, 0x0), 0x2);

	/******************************************************/
	/*                  Common Block                      */
	/******************************************************/

	/* Common block Tx Global Hibernate Exit */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x802b, 0x0), 0x00);
	/* Common block Reference Clokc Mode 26MHzt */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x80bf, 0x0), 0x01);
	/* Common block DCO Target Frequency MAX PWM G1:9Mpbs */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x80ea, 0x0), 0x80);
	/* Common block TX and RX Div Factor is 4 7Mbps/20 = 350KHz */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x80f1, 0x0), 0x04);
	/* Common Block  */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x80f3, 0x0), 0x64);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x80fb, 0x0), 0x09);
	/* Common Block Real Time Observe Select; For debugging */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x80f0, 0x0), 0x00);

	/******************************************************/
	/*                       Lane 0                       */
	/******************************************************/

	/* Tx0 Reference Clock 26MHz */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00eb, 0x0), 0x0d);
	/* TX0 Configuration Clock Frequency Val; Divider setting */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00ec, 0x0), 0x12);
	/* TX0 20bit RMMI Interface */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f0, 0x0), 0x12);
	/* TX0  */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f1, 0x0), 0xd6);
	/* Rx0 Reference Clock 26MHz */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00eb, 0x4), 0x01);
	/* RX0 Configuration Clock Frequency Val; Divider setting */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00ec, 0x4), 0x12);
	/* RX0 20bit RMMI Interface */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f0, 0x4), 0x02);
	/* RX0 Squelch Detector output is routed to RX hibernate Exit Type1
	 * signal */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00ba, 0x4), 0x80);

	/******************************************************/
	/*                       Lane 1                       */
	/******************************************************/

	if (host->avail_ln_tx == 2) {
		/* Tx1 Reference Clock 26MHz */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00eb, 0x1), 0x0d);
		/* TX1 Configuration Clock Frequency Val; Divider setting */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00ec, 0x1), 0x12);
		/* TX1 20bit RMMI Interface */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f0, 0x1), 0x12);
		/* TX1  */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f1, 0x1), 0xd6);
	}

	if (host->avail_ln_rx == 2) {
		/* Rx1 Reference Clock 26MHz */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00eb, 0x5), 0x01);
		/* RX1 Configuration Clock Frequency Val; Divider setting */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00ec, 0x5), 0x12);
		/* RX1 20bit RMMI Interface */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f0, 0x5), 0x02);
		/* RX1 Squelch Detector output is routed to RX hibernate Exit
		 * Type1 signal */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00ba, 0x5), 0x80);
	}

	/*TX_Controlled_ActTimer set to 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8001, 0x4), 0x00);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8002, 0x4), 0x00);

	/*disable termination resister switch. if delete, can not work in
	 * fastauto. need to check in ASIC...*/
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00bb, 0x0), 0x8);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00bb, 0x1), 0x8);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xd0b6, 0x0), 0x1);

	/* To write Shadow register bank to effective configuration block */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xd085, 0x0), 0x01);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xd000, 0x0), 0x01);

	pr_info("%s --\n", __func__);
	return 0;
}

/* snps asic mphy specific configuration */
static int ufs_kirin_dme_setup_snps_asic_mphy(struct ufs_hba *hba)
{
	uint32_t value = 0;
	int err = 0;
	struct ufs_kirin_host *host = hba->priv;

	pr_info("%s ++\n", __func__);

	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD0C1, 0x0), 0x1); /* Unipro VS_mphy_disable */
	if (host->caps & USE_RATE_B) {
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x156A, 0x0), 0x2); /* PA_HSSeries */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8114, 0x0), 0x1); /* MPHY CBRATESEL */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8121, 0x0), 0x2D); /* MPHY CBOVRCTRL2 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8122, 0x0), 0x1); /* MPHY CBOVRCTRL3 */

#ifdef CONFIG_SCSI_UFS_KIRIN_V21
		/*for some plateform,hw will set these auto to prepare phy calibration*/
		if (!(host->caps & PREPARE_MPHY_CALIB_AUTO)) {
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8127, 0x0), 0x98); /* MPHY CBOVRCTRL4 */
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8128, 0x0), 0x1); /* MPHY CBOVRCTRL5 */
		}
#endif

		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD085, 0x0), 0x1); /* Unipro VS_MphyCfgUpdt */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x800D, 0x4), 0x58); /* MPHY RXOVRCTRL4 rx0 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x800D, 0x5), 0x58); /* MPHY RXOVRCTRL4 rx1 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x800E, 0x4), 0xB); /* MPHY RXOVRCTRL5 rx0 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x800E, 0x5), 0xB); /* MPHY RXOVRCTRL5 rx1 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8009, 0x4), 0x1); /* MPHY RXSQCONTROL rx0 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8009, 0x5), 0x1); /* MPHY RXSQCONTROL rx1 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD085, 0x0), 0x1); /* Unipro VS_MphyCfgUpdt */
	} else {
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x156A, 0x0), 0x1); /* PA_HSSeries */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8114, 0x0), 0x0); /* MPHY CBRATESEL */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8121, 0x0), 0x4C); /* MPHY CBOVRCTRL2 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8122, 0x0), 0x1); /* MPHY CBOVRCTRL3 */

#ifdef CONFIG_SCSI_UFS_KIRIN_V21
		/*for some plateform,hw will set these auto to prepare phy calibration*/
		if (!(host->caps & PREPARE_MPHY_CALIB_AUTO)) {
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8127, 0x0), 0x82); /* MPHY CBOVRCTRL4 */
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8128, 0x0), 0x1); /* MPHY CBOVRCTRL5 */
		}
#endif
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD085, 0x0), 0x1); /* Unipro VS_MphyCfgUpdt */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x800D, 0x4), 0x18); /* MPHY RXOVRCTRL4 rx0 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x800D, 0x5), 0x18); /* MPHY RXOVRCTRL4 rx1 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x800E, 0x4), 0xD); /* MPHY RXOVRCTRL5 rx0 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x800E, 0x5), 0xD); /* MPHY RXOVRCTRL5 rx1 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8009, 0x4), 0x1); /* MPHY RXSQCONTROL rx0 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8009, 0x5), 0x1); /* MPHY RXSQCONTROL rx1 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD085, 0x0), 0x1); /* Unipro VS_MphyCfgUpdt */
	}

	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x8113, 0x0), 0x1);
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD085, 0x0), 0x1);

#ifdef CONFIG_SCSI_UFS_KIRIN_V21
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0092, 0x4), 0xA);/* RX_Hibern8Time_Capability*/
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0092, 0x5), 0xA);/* RX_Hibern8Time_Capability*/
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x008f, 0x4), 0xA);/* RX_Min_ActivateTime */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x008f, 0x5), 0xA);/* RX_Min_ActivateTime*/
#else
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x008F, 0x4), 0x7); /* Tactive RX */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x008F, 0x5), 0x7); /* Tactive RX */

#endif

	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0095, 0x4), 0x4F); /* Gear3 Synclength */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0095, 0x5), 0x4F); /* Gear3 Synclength */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0094, 0x4), 0x4F); /* Gear2 Synclength */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0094, 0x5), 0x4F); /* Gear2 Synclength */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x008B, 0x4), 0x4F); /* Gear1 Synclength */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x008B, 0x5), 0x4F); /* Gear1 Synclength */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x000F, 0x0), 0x5); /* Thibernate Tx */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x000F, 0x1), 0x5); /* Thibernate Tx */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD085, 0x0), 0x1);

	ufshcd_dme_get(hba, UIC_ARG_MIB_SEL(0xD0C1, 0x0), &value); /* Unipro VS_mphy_disable */
	if (value != 0x1)
		pr_warn("Warring!!! Unipro VS_mphy_disable is 0x%x\n", value);

	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD0C1, 0x0), 0x0); /* Unipro VS_mphy_disable */
	if (likely(!hba->host->is_emulator)) {
		err = ufs_kirin_check_hibern8(hba);
		if (err)
			pr_err("ufs_kirin_check_hibern8 error\n");
	}

#ifdef CONFIG_SCSI_UFS_HS_ERROR_RECOVER
#ifndef CONFIG_SCSI_UFS_KIRIN_V21
	ufs_kirin_set_vol(hba, hba->v_tx, hba->v_rx);
#endif
#endif
	pr_info("%s --\n", __func__);
	return err;
}

/*lint +e648 +e845*/
#define UFS_TX_EQUALIZER_35DB
/*#define UFS_TX_EQUALIZER_60DB*/
/*lint -save -e845*/
static void hisi_mphy_updata_temp_sqvref(struct ufs_hba *hba,
				struct ufs_kirin_host *host)
{
	if (host->caps & USE_HISI_MPHY_TC) {
		/*in low temperature to solve the PLL'S oscill */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0x00c1, 0x0),
			       0x1); /*RG_PLL_CP*/
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0x00d4, 0x0),
			       0x51); /*RG_PLL_DMY0*/
		/*rate A->B's VC0 stable time*/
		/* ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00db, 0x0),
			       0x5);*/
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f0, 0x4),
			       0x1); /*RX enable lane 0*/
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f0, 0x5),
			       0x1); /*RX enable lane 1*/
		/* H8's workaround*/
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f1, 0x4),
			       0x7); /*RX_SQ_VERF, lane 0*/
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00f1, 0x5),
			       0x7); /*RX_SQ_VERF, lane 1*/
	}
}

static void hisi_mphy_updata_vswing_fsm_ocs5(struct ufs_hba *hba,
				struct ufs_kirin_host *host)
{
	uint32_t value = 0;
	if (host->caps & USE_HISI_MPHY_TC) {
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c2, 0x4),
			       0x1); /*RX_MC_PRESENT*/
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c2, 0x5),
			       0x1); /*RX_MC_PRESENT*/
		/*disable vSwing change*/
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0x00c7, 0x0),
			       0x3); /*meaure the power, can close it*/
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0x00c8, 0x0),
			       0x3); /*meaure the power, can close it*/
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0x007a, 0x0), 0x1c);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x007a, 0x1), 0x1c);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0x007c, 0x0), 0xd4);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x007c, 0x1), 0xd4);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cf, 0x4),
			       0x2); /*RX_STALL*/
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cf, 0x5),
			       0x2); /*RX_STALL*/
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00d0, 0x4),
			       0x2); /*RX_SLEEP*/
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00d0, 0x5),
			       0x2); /*RX_SLEEP*/

		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cc, 0x4),
			       0x3); /*RX_HS_CLK_EN*/
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cc, 0x5),
			       0x3); /*RX_HS_CLK_EN*/
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cd, 0x4),
			       0x3); /*RX_LS_CLK_EN*/
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cd, 0x5),
			       0x3); /*RX_LS_CLK_EN*/
		/*enhance the accuracy of squelch detection*/
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00ce, 0x4),
			       0x3); /*RX_H8_EXIT*/
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00ce, 0x5),
			       0x3); /*RX_H8_EXIT*/

		/* try to solve the OCS=5 */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00e9, 0x4),
			       0x20); /*RX_HS_DATA_VALID_TIMER_VAL0*/
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00e9, 0x5),
			       0x20); /*RX_HS_DATA_VALID_TIMER_VAL0*/
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00ea, 0x4),
			       0x1); /*RX_HS_DATA_VALID_TIMER_VAL1*/
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00ea, 0x5),
			       0x1); /*RX_HS_DATA_VALID_TIMER_VAL1*/

		/* set the HS-prepare length and sync length to MAX value, try
		* to solve the data check error problem,
		* the device seems not receive the write cmd. */
		/* PA_TxHsG1SyncLength , can not set MPHY's register directly */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0x1552, 0x0), 0x4F);
		/* PA_TxHsG2SyncLength , can not set MPHY's register directly */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0x1554, 0x0), 0x4F);
		/* PA_TxHsG3SyncLength , can not set MPHY's register directly */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0x1556, 0x0), 0x4F);

		/*enlarge TX_LS_PREPARE_LENGTH*/
		/*enable override*/
		ufshcd_dme_get(hba, UIC_ARG_MIB_SEL((u32)0xd0f0, 0x0),
			       &value); /* Unipro VS_mphy_disable */
		value |= (1 << 3);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0xd0f0, 0x0), value);
		/*Set to max value 0xf*/
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0xd0f4, 0x0), 0xf);


		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0xd085, 0x0), 0x1); /* update */
		/* FPGA should enlarge the VS_AdjustTrailingClocks,
		 * to give more time for Host's RX to prepare receive burst data */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0xd086, 0x0), 0xF0); /* Unipro VS_AdjustTrailingClocks */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL((u32)0xd0a0, 0x0), 0x3); /* Unipro VS_DebugSaveConfigTime */
	}
}
/*lint -restore*/
static void hisi_mphy_busdly_config(struct ufs_hba *hba,
			struct ufs_kirin_host *host)
{
	uint32_t reg = 0;
	if ((host->caps & USE_HISI_MPHY_TC)
		&& (host->caps & USE_FPGA_BOARD_CLK)) {
		/*UFS_BUSTHRTL_OFF*/
		reg = ufshcd_readl(hba, UFS_REG_OCPTHRTL);
		reg &= (~((0x3f << 18) | (0xff << 0)));
		reg |= ((0x10 << 18) | (0xff << 0));
		ufshcd_writel(hba, reg, UFS_REG_OCPTHRTL);
		reg = ufshcd_readl(hba, UFS_REG_OCPTHRTL);
	}
}

/*lint -e648 -e845*/
static int ufs_kirin_link_startup_pre_change(struct ufs_hba *hba)
{
	int err = 0;
	uint32_t value = 0;
	uint32_t reg = 0;
	struct ufs_kirin_host *host = hba->priv;

	pr_info("%s ++\n", __func__);

	/*for hisi MPHY*/
	hisi_mphy_updata_temp_sqvref(hba, host);

	/*FIXME is it good for FPGA condition*/
	if (!(host->caps & USE_SNPS_MPHY_TC || host->caps & USE_HISI_MPHY_TC)) {
		err = ufs_kirin_dme_setup_snps_asic_mphy(hba);
		if (err)
			return err;
	}

	if (host->caps & USE_FPGA_BOARD_CLK) {
		ufshcd_writel(hba, UFS_HCLKDIV_FPGA_VALUE, UFS_REG_HCLKDIV);
	} else {
#ifndef CONFIG_SCSI_UFS_KIRIN_V21
		ufshcd_writel(hba, UFS_HCLKDIV_NORMAL_VALUE, UFS_REG_HCLKDIV);
#endif
	}

	/* disable auto H8 */
	reg = ufshcd_readl(hba, REG_CONTROLLER_AHIT);
	reg = reg & (~UFS_AHIT_AH8ITV_MASK);
	ufshcd_writel(hba, reg, REG_CONTROLLER_AHIT);

	if (host->caps & USE_SNPS_MPHY_TC) {
		ufs_kirin_phy_init(host);
		/* SNPS mphy test chip specific configration */
		err = ufs_kirin_dme_setup_snps_mphy(hba);
		if (err)
			return err;
	} else if (host->caps & USE_HISI_MPHY_TC) {
		err = ufs_kirin_dme_setup_hisi_mphy(hba);
		if (err)
			return err;
	}

	ufs_sys_ctrl_writel(host, MASK_UFS_DEVICE_RESET | BIT_UFS_DEVICE_RESET,\
		UFS_DEVICE_RESET_CTRL); /* disable Device Reset */
	/*for hisi MPHY*/
	hisi_mphy_updata_vswing_fsm_ocs5(hba, host);

	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x155E, 0x0), 0x0); /* Unipro PA_Local_TX_LCC_Enable */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xD0AB, 0x0), 0x0); /* close Unipro VS_Mk2ExtnSupport */
	ufshcd_dme_get(hba, UIC_ARG_MIB_SEL(0xD0AB, 0x0), &value);
	if (0 != value) {
		/* Ensure close success */
		pr_warn("Warring!!! close VS_Mk2ExtnSupport failed\n");
	}
	if (!((host->caps & USE_HISI_MPHY_TC) &&
	      (host->caps & USE_FPGA_BOARD_CLK))) {
		/*FPGA with HISI PHY not configure equalizer*/
		if (35 == host->tx_equalizer) {
			ufs_kirin_mphy_write(hba, 0x1002, 0xAC78);
			ufs_kirin_mphy_write(hba, 0x1102, 0xAC78);
			ufs_kirin_mphy_write(hba, 0x1003, 0x2440);
			ufs_kirin_mphy_write(hba, 0x1103, 0x2440);
		} else if (60 == host->tx_equalizer) {
			ufs_kirin_mphy_write(hba, 0x1002, 0xAA78);
			ufs_kirin_mphy_write(hba, 0x1102, 0xAA78);
			ufs_kirin_mphy_write(hba, 0x1003, 0x2640);
			ufs_kirin_mphy_write(hba, 0x1103, 0x2640);
		}
	}
	/*for hisi MPHY*/
	hisi_mphy_busdly_config(hba, host);

	pr_info("%s --\n", __func__);

	return err;
}

static void hisi_mphy_link_post_config(struct ufs_hba *hba,
			struct ufs_kirin_host *host)
{
	uint32_t tx_lane_num = 1;
	uint32_t rx_lane_num = 1;

	if (host->caps & USE_HISI_MPHY_TC) {
		/*set the PA_TActivate to 128. need to check in ASIC...*/
		/* H8's workaround */
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x15a8, 0x0), 5);
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x80da, 0x0), 0x2d);

		ufshcd_dme_get(hba, UIC_ARG_MIB(0x1561), &tx_lane_num);
		ufshcd_dme_get(hba, UIC_ARG_MIB(0x1581), &rx_lane_num);

		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c2, 0x4),
			       0x0); /*RX_MC_PRESENT*/
		if (tx_lane_num > 1 && rx_lane_num > 1) {
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c2, 0x5),
				       0x0); /*RX_MC_PRESENT*/
		}
	}
}
/*lint -e648 -e845*/
static int ufs_kirin_link_startup_post_change(struct ufs_hba *hba)
{
	uint32_t value;
	struct ufs_kirin_host *host = hba->priv;

	pr_info("%s ++\n", __func__);
	if (host->caps & USE_SNPS_MPHY_TC) {
		/*set the granularity to 2. need to check in ASIC...*/
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x15aa, 0x0), 0x2);
		/*set the PA_TActivate to 128. need to check in ASIC...*/
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x15a8, 0x0), 128);

		/* disable power-gating in auto hibernate 8 */
		ufshcd_rmwl(hba, LP_AH8_PGE, 0, UFS_REG_OCPTHRTL);

		/* disable Ultra Low Power Hibenate feature of Synopsys MPHY */
		ufshcd_dme_get(hba, UIC_ARG_MIB(VS_ULPH8_Cntrl), &value);
		value &= (uint32_t)(~Ulp_Ulp_CtrlMode);
		ufshcd_dme_set(hba, UIC_ARG_MIB(VS_ULPH8_Cntrl), value);
	} else {
		ufshcd_dme_set(hba, UIC_ARG_MIB(0x2044), 0x0); /* Unipro DL_AFC0CreditThreshold */
		ufshcd_dme_set(hba, UIC_ARG_MIB(0x2045), 0x0); /* Unipro DL_TC0OutAckThreshold */
		ufshcd_dme_set(hba, UIC_ARG_MIB(0x2040), 0x9); /* Unipro DL_TC0TXFCThreshold */
	}
	/*for hisi MPHY*/
	hisi_mphy_link_post_config(hba, host);

	if (host->caps & BROKEN_CLK_GATE_BYPASS) {
		/* not bypass ufs clk gate */
		ufs_sys_ctrl_clr_bits(host, MASK_UFS_CLK_GATE_BYPASS, CLOCK_GATE_BYPASS);
		ufs_sys_ctrl_clr_bits(host, MASK_UFS_SYSCRTL_BYPASS, UFS_SYSCTRL);
	}

	if (host->caps & USE_AUTO_H8) {
		/* disable power-gating in auto hibernate 8 */
		ufshcd_rmwl(hba, LP_AH8_PGE, 0, UFS_REG_OCPTHRTL);

		/* enable auto H8 */
		ufshcd_writel(hba, UFS_AHIT_AUTOH8_TIMER, REG_CONTROLLER_AHIT);
	}

	ufshcd_dme_set(hba, UIC_ARG_MIB(0xd09a), 0x80000000); /* select received symbol cnt */
	ufshcd_dme_set(hba, UIC_ARG_MIB(0xd09c), 0x00000005); /* reset counter0 and enable */

	pr_info("%s --\n", __func__);
	return 0;
}
/*lint +e648 +e845*/
/*lint -save -e483*/
static int ufs_kirin_link_startup_notify(struct ufs_hba *hba, bool status)
{
	int err = 0;
	switch (status) {
	case PRE_CHANGE:
		err = ufs_kirin_link_startup_pre_change(hba);
		break;
	case POST_CHANGE:
		err = ufs_kirin_link_startup_post_change(hba);
		break;
	default:
		break;
	}

	return err;
}
/*lint -restore*/
/* TODO: get limit information from dts */
struct ufs_kirin_dev_params {
	u32 pwm_rx_gear; /* pwm rx gear to work in */
	u32 pwm_tx_gear; /* pwm tx gear to work in */
	u32 hs_rx_gear;  /* hs rx gear to work in */
	u32 hs_tx_gear;  /* hs tx gear to work in */
	u32 rx_lanes;    /* number of rx lanes */
	u32 tx_lanes;    /* number of tx lanes */
	u32 rx_pwr_pwm;  /* rx pwm working pwr */
	u32 tx_pwr_pwm;  /* tx pwm working pwr */
	u32 rx_pwr_hs;   /* rx hs working pwr */
	u32 tx_pwr_hs;   /* tx hs working pwr */
	u32 hs_rate;     /* rate A/B to work in HS */
	u32 desired_working_mode;
};

static int ufs_kirin_get_pwr_dev_param(struct ufs_kirin_dev_params *kirin_param,
				       struct ufs_pa_layer_attr *dev_max,
				       struct ufs_pa_layer_attr *agreed_pwr)
{
	int min_kirin_gear;
	int min_dev_gear;
	bool is_dev_sup_hs = false;
	bool is_kirin_max_hs = false;

	if (dev_max->pwr_rx == FASTAUTO_MODE || dev_max->pwr_rx == FAST_MODE)
		is_dev_sup_hs = true;

	if (kirin_param->desired_working_mode == FAST) {
		is_kirin_max_hs = true;
		min_kirin_gear = min_t(u32, kirin_param->hs_rx_gear,
				       kirin_param->hs_tx_gear);
	} else {
		min_kirin_gear = min_t(u32, kirin_param->pwm_rx_gear,
				       kirin_param->pwm_tx_gear);
	}

	/*
	 * device doesn't support HS but kirin_param->desired_working_mode is
	 * HS, thus device and kirin_param don't agree
	 */
	if (!is_dev_sup_hs && is_kirin_max_hs) {
		pr_err("%s: failed to agree on power mode (device doesn't "
		       "support HS but requested power is HS)\n",
		       __func__);
		return -ENOTSUPP;
	} else if (is_dev_sup_hs && is_kirin_max_hs) {
		/*
		 * since device supports HS, it supports FAST_MODE.
		 * since kirin_param->desired_working_mode is also HS
		 * then final decision (FAST/FASTAUTO) is done according
		 * to kirin_params as it is the restricting factor
		 */
		agreed_pwr->pwr_rx = agreed_pwr->pwr_tx =
			kirin_param->rx_pwr_hs;
	} else {
		/*
		 * here kirin_param->desired_working_mode is PWM.
		 * it doesn't matter whether device supports HS or PWM,
		 * in both cases kirin_param->desired_working_mode will
		 * determine the mode
		 */
		agreed_pwr->pwr_rx = agreed_pwr->pwr_tx =
			kirin_param->rx_pwr_pwm;
	}

	/*
	 * we would like tx to work in the minimum number of lanes
	 * between device capability and vendor preferences.
	 * the same decision will be made for rx
	 */
	agreed_pwr->lane_tx =
		min_t(u32, dev_max->lane_tx, kirin_param->tx_lanes);
	agreed_pwr->lane_rx =
		min_t(u32, dev_max->lane_rx, kirin_param->rx_lanes);

	/* device maximum gear is the minimum between device rx and tx gears */
	min_dev_gear = min_t(u32, dev_max->gear_rx, dev_max->gear_tx);

	/*
	 * if both device capabilities and vendor pre-defined preferences are
	 * both HS or both PWM then set the minimum gear to be the chosen
	 * working gear.
	 * if one is PWM and one is HS then the one that is PWM get to decide
	 * what is the gear, as it is the one that also decided previously what
	 * pwr the device will be configured to.
	 */
	if ((is_dev_sup_hs && is_kirin_max_hs) ||
	    (!is_dev_sup_hs && !is_kirin_max_hs))
		agreed_pwr->gear_rx = agreed_pwr->gear_tx =
			min_t(u32, min_dev_gear, min_kirin_gear);
	else
		agreed_pwr->gear_rx = agreed_pwr->gear_tx = min_kirin_gear;

	agreed_pwr->hs_rate = kirin_param->hs_rate;

	pr_err("ufs final power mode: gear = %d, lane = %d, pwr = %d, "
		"rate = %d\n",
		agreed_pwr->gear_rx, agreed_pwr->lane_rx, agreed_pwr->pwr_rx,
		agreed_pwr->hs_rate);
	return 0;
}

/*lint -e845 -e648*/
static void ufs_kirin_pwr_change_pre_change(struct ufs_hba *hba)
{
	uint32_t value;
	pr_info("%s ++\n", __func__);

	pr_info("device manufacturer_id is 0x%x\n", hba->manufacturer_id);
#ifdef CONFIG_SCSI_UFS_KIRIN_V21
	/*Boston platform need to set SaveConfigTime to 0x13, and change sync length to maximum value */
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0xD0A0), 0x13); /* VS_DebugSaveConfigTime */
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0x1552), 0x4f); /* g1 sync length */
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0x1554), 0x4f); /* g2 sync length */
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0x1556), 0x4f); /* g3 sync length */
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0x15a7), 0xA); /* PA_Hibern8Time */
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0x15a8), 0xA); /* PA_Tactivate */
	ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0xd085, 0x0), 0x01);
#else
	if (UFS_VENDOR_HYNIX == hba->manufacturer_id) {
		pr_info("H**** device must set VS_DebugSaveConfigTime 0x10\n");
		ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0xD0A0), 0x10); /* VS_DebugSaveConfigTime */
		ufshcd_dme_set(hba, UIC_ARG_MIB(0x1556), 0x48); /* sync length */
		/* no need to update in unipro register */
		/* ufshcd_dme_set(hba, UIC_ARG_MIB(0xD085), 0x01); */
	}
#endif
	ufshcd_dme_get(hba, UIC_ARG_MIB(0x15A8), &value);
	if (value < 0x7)
		ufshcd_dme_set(hba, UIC_ARG_MIB(0x15A8), 0x7); /* update PaTactive */

	ufshcd_dme_set(hba, UIC_ARG_MIB(0x155c), 0x0); /* PA_TxSkip */

	/*PA_PWRModeUserData0 = 8191, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x15b0), 8191);
	/*PA_PWRModeUserData1 = 65535, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x15b1), 65535);
	/*PA_PWRModeUserData2 = 32767, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x15b2), 32767);
	/*DME_FC0ProtectionTimeOutVal = 8191, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0xd041), 8191);
	/*DME_TC0ReplayTimeOutVal = 65535, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0xd042), 65535);
	/*DME_AFC0ReqTimeOutVal = 32767, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0xd043), 32767);
	/*PA_PWRModeUserData3 = 8191, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x15b3), 8191);
	/*PA_PWRModeUserData4 = 65535, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x15b4), 65535);
	/*PA_PWRModeUserData5 = 32767, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB(0x15b5), 32767);
	/*DME_FC1ProtectionTimeOutVal = 8191, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0xd044), 8191);
	/*DME_TC1ReplayTimeOutVal = 65535, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0xd045), 65535);
	/*DME_AFC1ReqTimeOutVal = 32767, default is 0*/
	ufshcd_dme_set(hba, UIC_ARG_MIB((u32)0xd046), 32767);


	pr_info("%s --\n", __func__);
	return;
}


void adapt_pll_to_power_mode(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = hba->priv;

	/*FIXME is need to distinguish FASTMODE FAST SLOW SLOWAUTO */
	if (host->caps & USE_HS_GEAR1) {
		if (host->caps & USE_RATE_B) {
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c2, 0x0),
				       0x0); /*RG_PLL_PRE_DIV*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c9, 0x0),
				       0x0); /*RG_PLL_SWC_EN*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c4, 0x0),
				       0x1); /*RG_PLL_FBK_S*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c3, 0x0),
				       0x4c); /*RG_PLL_FBK_P*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cf, 0x0),
				       0x2); /*RG_PLL_TXHSGR*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cd, 0x0),
				       0x2); /*RG_PLL_RXHSGR*/

		} else {
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c2, 0x0),
				       0x0); /*RG_PLL_PRE_DIV*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c9, 0x0),
				       0x0); /*RG_PLL_SWC_EN*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c4, 0x0),
				       0x1); /*RG_PLL_FBK_S*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c3, 0x0),
				       0x41); /*RG_PLL_FBK_P*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cf, 0x0),
				       0x2); /*RG_PLL_TXHSGR*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cd, 0x0),
				       0x2); /*RG_PLL_RXHSGR*/
		}
	} else if (host->caps & USE_HS_GEAR2) {
		if (host->caps & USE_RATE_B) {
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c2, 0x0),
				       0x0); /*RG_PLL_PRE_DIV*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c9, 0x0),
				       0x0); /*RG_PLL_SWC_EN*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c4, 0x1),
				       0x1); /*RG_PLL_FBK_S*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c3, 0x4c),
				       0x4c); /*RG_PLL_FBK_P*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cf, 0x1),
				       0x2); /*RG_PLL_TXHSGR*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cd, 0x1),
				       0x2); /*RG_PLL_RXHSGR*/

		} else {
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c2, 0x0),
				       0x0); /*RG_PLL_PRE_DIV*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c9, 0x0),
				       0x0); /*RG_PLL_SWC_EN*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c4, 0x1),
				       0x1); /*RG_PLL_FBK_S*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c3, 0x41),
				       0x41); /*RG_PLL_FBK_P*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cf, 0x1),
				       0x2); /*RG_PLL_TXHSGR*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cd, 0x1),
				       0x2); /*RG_PLL_RXHSGR*/
		}
	} else if (host->caps & USE_HS_GEAR3) {
		if (host->caps & USE_RATE_B) {
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c2, 0x0),
				       0x0); /*RG_PLL_PRE_DIV*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c9, 0x0),
				       0x0); /*RG_PLL_SWC_EN*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c4, 0x1),
				       0x1); /*RG_PLL_FBK_S*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c3, 0x4c),
				       0x4c); /*RG_PLL_FBK_P*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cf, 0x1),
				       0x0); /*RG_PLL_TXHSGR*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cd, 0x1),
				       0x0); /*RG_PLL_RXHSGR*/

		} else {
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c2, 0x0),
				       0x0); /*RG_PLL_PRE_DIV*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c9, 0x0),
				       0x0); /*RG_PLL_SWC_EN*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c4, 0x1),
				       0x1); /*RG_PLL_FBK_S*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00c3, 0x41),
				       0x41); /*RG_PLL_FBK_P*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cf, 0x1),
				       0x0); /*RG_PLL_TXHSGR*/
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x00cd, 0x1),
				       0x0); /*RG_PLL_RXHSGR*/
		}
	}
}
/*lint -save -e845*/

void deemphasis_config(struct ufs_kirin_host *host, struct ufs_hba *hba,
				struct ufs_pa_layer_attr *dev_req_params)
{
	uint32_t value = 0;
	if (host->caps & USE_HISI_MPHY_TC) {
		/*FIXME is it good to use for FPGA condition*/
		if (host->caps & USE_FPGA_BOARD_CLK) {
			/*  de-emphasis level map
			5????bx0000: 0 dB
			5????bx0001: 0.72 dB
			5????bx0010: 1.45 dB
			5????bx0011: 2.18 dB
			5????bx0100: 2.92 dB
			5????bx0101: 3.67 dB
			5????bx0110: 4.44 dB
			5????bx0111: 5.22 dB
			5????bx1110: 6.02 dB
			5????bx1111: 6.85 dB
			*/
			/* the de-emphasis level you want to select, for
			* example ,
			* value = 0x5, it's 3.67 dB */

			if (host->caps & USE_HS_GEAR3) {
				value = 0x26; /*4.44 db*/
				ufshcd_dme_set(hba,
					UIC_ARG_MIB_SEL(
					0x007e, 0x0),
					0x5);
				ufshcd_dme_set(hba,
					UIC_ARG_MIB_SEL(
					0x0025, 0x0),
					0x22);
				ufshcd_dme_set(hba,
					UIC_ARG_MIB_SEL(
					0x007d, 0x0),
					0x22);
				if ((dev_req_params->lane_tx > 1) &&
					(dev_req_params->lane_rx > 1)) {
					ufshcd_dme_set(hba,
						UIC_ARG_MIB_SEL(
						0x007e, 0x1),
						0x5);
					ufshcd_dme_set(hba,
						UIC_ARG_MIB_SEL(
						0x0025, 0x1),
						0x22);
					ufshcd_dme_set(hba,
						UIC_ARG_MIB_SEL(
						0x007d, 0x1),
						0x22);
				}
			} else {
				value = 0x6f; /*6.85 db*/
				ufshcd_dme_set(hba,
					UIC_ARG_MIB_SEL(
					0x007e, 0x0),
					0x5);
				ufshcd_dme_set(hba,
					UIC_ARG_MIB_SEL(
					0x0025, 0x0),
					0x15);
				ufshcd_dme_set(hba,
					UIC_ARG_MIB_SEL(
					0x007d, 0x0),
					0x15);
				if ((dev_req_params->lane_tx > 1) &&
					(dev_req_params->lane_rx > 1)) {
					ufshcd_dme_set(hba,
						UIC_ARG_MIB_SEL(
						0x007e, 0x1),
						0x5);
					ufshcd_dme_set(hba,
						UIC_ARG_MIB_SEL(
						0x0025, 0x1),
						0x15);
					ufshcd_dme_set(hba,
						UIC_ARG_MIB_SEL(
						0x007d, 0x1),
						0x15);
				}
			}
			ufshcd_dme_set(hba,
					UIC_ARG_MIB_SEL(
					0x0037, 0x0),
					value);
			ufshcd_dme_set(hba,
					UIC_ARG_MIB_SEL(
					0x007b, 0x0),
					value);
			if ((dev_req_params->lane_tx > 1) &&
				(dev_req_params->lane_rx > 1)) {
				ufshcd_dme_set(hba,
						UIC_ARG_MIB_SEL(
						0x0037, 0x1),
						value);
				ufshcd_dme_set(hba,
						UIC_ARG_MIB_SEL(
						0x007b, 0x1),
						value);
			}
		}
	}
}

/*lint -restore*/
static int ufs_kirin_pwr_change_notify(struct ufs_hba *hba, bool status,
				       struct ufs_pa_layer_attr *dev_max_params,
				       struct ufs_pa_layer_attr *dev_req_params)
{
	struct ufs_kirin_dev_params ufs_kirin_cap;
	struct ufs_kirin_host *host = hba->priv;
	int ret = 0;
#if 0
	uint32_t value;
#endif

	if (!dev_req_params) {
		pr_err("%s: incoming dev_req_params is NULL\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	switch (status) {/*lint !e483*/
	case PRE_CHANGE:
		if (host->caps & USE_ONE_LANE) {
			ufs_kirin_cap.tx_lanes = 1;
			ufs_kirin_cap.rx_lanes = 1;
		} else {
			ufs_kirin_cap.tx_lanes = 2;
			ufs_kirin_cap.rx_lanes = 2;
		}

#ifdef CONFIG_SCSI_UFS_HS_ERROR_RECOVER
		if (hba->hs_single_lane) {
			ufs_kirin_cap.tx_lanes = 1;
			ufs_kirin_cap.rx_lanes = 1;
		}
#endif

		if (host->caps & USE_HS_GEAR3) {
			ufs_kirin_cap.hs_rx_gear = UFS_HS_G3;
			ufs_kirin_cap.hs_tx_gear = UFS_HS_G3;
			ufs_kirin_cap.desired_working_mode = FAST;
		} else if (host->caps & USE_HS_GEAR2) {
			ufs_kirin_cap.hs_rx_gear = UFS_HS_G2;
			ufs_kirin_cap.hs_tx_gear = UFS_HS_G2;
			ufs_kirin_cap.desired_working_mode = FAST;
		} else if (host->caps & USE_HS_GEAR1) {
			ufs_kirin_cap.hs_rx_gear = UFS_HS_G1;
			ufs_kirin_cap.hs_tx_gear = UFS_HS_G1;
			ufs_kirin_cap.desired_working_mode = FAST;
		} else {
			ufs_kirin_cap.hs_rx_gear = 0;
			ufs_kirin_cap.hs_tx_gear = 0;
			ufs_kirin_cap.desired_working_mode = SLOW;
		}

#ifdef CONFIG_SCSI_UFS_HS_ERROR_RECOVER
		if (hba->use_pwm_mode)
			ufs_kirin_cap.desired_working_mode = SLOW;
#endif

		ufs_kirin_cap.pwm_rx_gear = UFS_KIRIN_LIMIT_PWMGEAR_RX;
		ufs_kirin_cap.pwm_tx_gear = UFS_KIRIN_LIMIT_PWMGEAR_TX;
		ufs_kirin_cap.rx_pwr_pwm = UFS_KIRIN_LIMIT_RX_PWR_PWM;
		ufs_kirin_cap.tx_pwr_pwm = UFS_KIRIN_LIMIT_TX_PWR_PWM;
		/*hynix not support fastauto now*/
		if ((host->caps & BROKEN_FASTAUTO) ||
		    (UFS_VENDOR_HYNIX == host->hba->manufacturer_id) ||
		    (UFS_VENDOR_SAMSUNG == host->hba->manufacturer_id)) {
			ufs_kirin_cap.rx_pwr_hs = FAST_MODE;
			ufs_kirin_cap.tx_pwr_hs = FAST_MODE;
		} else {
			ufs_kirin_cap.rx_pwr_hs = FASTAUTO_MODE;
			ufs_kirin_cap.tx_pwr_hs = FASTAUTO_MODE;
		}

		if (host->caps & USE_RATE_B)
			ufs_kirin_cap.hs_rate = PA_HS_MODE_B;
		else
			ufs_kirin_cap.hs_rate = PA_HS_MODE_A;

		ret = ufs_kirin_get_pwr_dev_param(
			&ufs_kirin_cap, dev_max_params, dev_req_params);
		if (ret) {
			pr_err("%s: failed to determine capabilities\n",
			       __func__);
			goto out;
		}
		/*for hisi MPHY*/
		deemphasis_config(host, hba, dev_req_params);

#if 0
#ifdef UFS_TX_EQUALIZER_35DB
		pr_info("set TX_EQUALIZER 3.5db\n");
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0037, 0x0), 0x1);
		if ((dev_req_params->lane_tx > 1) && (dev_req_params->lane_rx > 1))
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0037, 0x1), 0x1);
#endif
#ifdef UFS_TX_EQUALIZER_60DB
		pr_info("set TX_EQUALIZER 6db\n");
		ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0037, 0x0), 0x2);
		if ((dev_req_params->lane_tx > 1) && (dev_req_params->lane_rx > 1))
			ufshcd_dme_set(hba, UIC_ARG_MIB_SEL(0x0037, 0x1), 0x2);
#endif
#endif
		if (host->caps & USE_HISI_MPHY_TC) {
			/*FIXME is it good to use for FPGA condition*/
			if (host->caps & USE_FPGA_BOARD_CLK) {
				adapt_pll_to_power_mode(hba);
			}
		}

		ufs_kirin_pwr_change_pre_change(hba);
		break;
	case POST_CHANGE:
#if 0
		ufshcd_dme_get(hba, UIC_ARG_MIB_SEL(0x0037, 0x0), &value);
		pr_info("check TX_EQUALIZER DB value lane0 = 0x%x\n", value);
		if ((dev_req_params->lane_tx > 1) && (dev_req_params->lane_rx > 1)) {
			ufshcd_dme_get(hba, UIC_ARG_MIB_SEL(0x0037, 0x1), &value);
			pr_info("check TX_EQUALIZER DB value lane1 = 0x%x\n", value);
		}
#endif
		break;
	default:
		ret = -EINVAL;
		break;
	}
out:
	return ret;
}

#ifdef FEATURE_KIRIN_UFS_PSW
static int ufs_kirin_wait_ufssys_bitset_timeout(struct ufs_kirin_host *host,
		uint32_t mask, uint32_t off, int timeout_us)
{
	while (--timeout_us > 0) {
		if (mask & ufs_sys_ctrl_readl(host, off))
			break;
		udelay(1);
	}
	if (timeout_us <= 0) {
		pr_err("%s: wait ufs sys bit time out\n", __func__);
		return -1;
	}
	return 0;
}
#endif

static int ufs_kirin_suspend_before_set_link_state(struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
#ifdef FEATURE_KIRIN_UFS_PSW
	struct ufs_kirin_host *host = hba->priv;

	if (ufshcd_is_runtime_pm(pm_op))
		return 0;

	/*step1:store BUSTHRTL register*/
	host->busthrtl_backup = ufshcd_readl(hba, UFS_REG_OCPTHRTL);
	/*enable PowerGating*/
	ufshcd_rmwl(hba, LP_PGE, LP_PGE, UFS_REG_OCPTHRTL);
#endif
	return 0;
}

static inline void ufs_kirin_memrepair_suspend(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = hba->priv;

	pr_err("%s ufs need memory repair\n", __func__);
	ufs_sys_ctrl_writel(host, 0x00E00000, UFS_CRG_UFS_CFG);
	udelay(1);
	ufs_sys_ctrl_set_bits(host, BIT_UFS_PSW_ISO_CTRL, PSW_POWER_CTRL);

	return;
}

static inline void ufs_kirin_memrepair_resume(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = hba->priv;
	int i;

	pr_err("%s ufs need memory repair\n", __func__);
	ufs_sys_ctrl_clr_bits(host, BIT_UFS_PSW_ISO_CTRL, PSW_POWER_CTRL);
	for (i = 0; i < 400; i++) {
		if (BIT(19) & ufs_pctrl_readl(host, 0x18C))
			break;
		udelay(1);
	}
	if (i > 399)
		pr_err("%s ufs memory repair timeout\n", __func__);
	ufs_sys_ctrl_writel(host, 0x00E000E0, UFS_CRG_UFS_CFG);

	return;
}

static int ufs_kirin_suspend(struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
	struct ufs_kirin_host *host = hba->priv;

	/*set SOC_SCTRL_SCBAKDATA11_ADDR ufs bit to 0 when idle*/
	hisi_idle_sleep_vote(ID_UFS, 0);

	if (ufshcd_is_runtime_pm(pm_op))
		return 0;

	pr_info("ufs_sys_ctrl 0x3C: 0x%x\n", ufs_sys_ctrl_readl(host, 0x3C));
	pr_info("ufs_sys_ctrl 0x40: 0x%x\n", ufs_sys_ctrl_readl(host, 0x40));

	if (host->in_suspend) {
		WARN_ON(1);/*lint !e730*/
		return 0;
	}

	ufs_sys_ctrl_clr_bits(host, BIT_SYSCTRL_REF_CLOCK_EN, PHY_CLK_CTRL);
	udelay(10);
	/* set ref_dig_clk override of PHY PCS to 0 */
	ufs_sys_ctrl_writel(host, 0x00100000, UFS_DEVICE_RESET_CTRL);

#ifdef FEATURE_KIRIN_UFS_PSW
	/*step 10: poll HC_LP_CTRL(0x0C)bit0*/
	if (ufs_kirin_wait_ufssys_bitset_timeout(host,
			BIT_SYSCTRL_LP_PWR_GATE, HC_LP_CTRL, 10000)) {
		pr_err("%s: LP_PWR_GATE time out\n", __func__);
		return -1;
	}

	/*step 11:set PSW_CLK_CTRL(0x14) bit[4] to 0,close psw clk*/
	ufs_sys_ctrl_clr_bits(host, BIT_SYSCTRL_PSW_CLK_EN, PSW_CLK_CTRL);

	/*step 12:set HC_LP_CTRL(0x0C) bit[16] to 1,set PSW_POWER_CTRL(0x04) bit[16] to 1*/
	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_LP_ISOL_EN, HC_LP_CTRL);
	ufs_sys_ctrl_set_bits(host, BIT_UFS_PSW_ISO_CTRL, PSW_POWER_CTRL);

	/*step 13:set UFS_SC RESET_CTRL_EN(0x1C) bit[0] to 0,reset UFSHCPSW area*/
	ufs_sys_ctrl_clr_bits(host, BIT_SYSCTRL_LP_RESET_N, RESET_CTRL_EN);

	/*step 14:set UFS_SC PSW_POWER_CTRL(0x04) bit[0] to 0,power off psw area*/
	ufs_sys_ctrl_clr_bits(host, BIT_UFS_PSW_MTCMOS_EN, PSW_POWER_CTRL);
	ufs_sys_ctrl_clr_bits(host, BIT_SYSCTRL_PWR_READY, HC_LP_CTRL);
#endif

	udelay(100);
	clk_disable_unprepare(host->clk_ufsio_ref);
	if (CLK_UFSIO & ufs_pericrg_readl(host, PERCLKEN7_OFFSET)) {
		pr_err("%s:disable clk ref err. PERDIS7 = 0x%x\n", __func__,
			ufs_pericrg_readl(host, PERCLKEN7_OFFSET));
	}

	/* close ufsphy cfg clk */
	ufs_pericrg_writel(host, CLK_UFSPHY, PERDIS7_OFFSET);
	if (CLK_UFSPHY & ufs_pericrg_readl(host, PERCLKEN7_OFFSET)) {
		pr_err("%s:disable phy cfg clk err. PERCLKEN7 = 0x%x\n", __func__,
			ufs_pericrg_readl(host, PERCLKEN7_OFFSET));
	}

	/* ufs_pericrg_writel(host, CLK_UFS_SUBSYS, PERDIS5_OFFSET); */
	host->in_suspend = true;

	return 0;
}

static int ufs_kirin_resume(struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
	struct ufs_kirin_host *host = hba->priv;
	int ret;

	/*set SOC_SCTRL_SCBAKDATA11_ADDR ufs bit to 1 when busy*/
	hisi_idle_sleep_vote(ID_UFS, 1);

	if (!host->in_suspend)
		return 0;

	/* ufs_pericrg_writel(host, CLK_UFS_SUBSYS, PEREN5_OFFSET); */

	/* open ufsphy cfg clk */
	ufs_pericrg_writel(host, CLK_UFSPHY, PEREN7_OFFSET);
	if (!(CLK_UFSPHY & ufs_pericrg_readl(host, PERCLKEN7_OFFSET))) {
		pr_err("%s:enable phy cfg clk err. PERCLKEN7 = 0x%x\n", __func__,
			ufs_pericrg_readl(host, PERCLKEN7_OFFSET));
	}

	ret = clk_prepare_enable(host->clk_ufsio_ref);
	if (ret) {
		pr_err("%s ,clk_prepare_enable failed\n", __func__);
		return ret;
	}
	if (!(0x1 & ufs_pctrl_readl(host, PCTRL_PERI_CTRL3_OFFSET))) {
		pr_err("%s:enable clk ref err. PERI_CTRL3 = 0x%x\n", __func__,
			ufs_pctrl_readl(host, PCTRL_PERI_CTRL3_OFFSET));
	}
	if (!(CLK_UFSIO & ufs_pericrg_readl(host, PERCLKEN7_OFFSET))) {
		pr_err("%s:enable clk ref err. PERDIS7 = 0x%x\n", __func__,
			ufs_pericrg_readl(host, PERCLKEN7_OFFSET));
	}

	udelay(1);
	/* set ref_dig_clk override of PHY PCS to 1 */
	ufs_sys_ctrl_writel(host, 0x00100010, UFS_DEVICE_RESET_CTRL);
	udelay(10);
	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_REF_CLOCK_EN, PHY_CLK_CTRL);

#ifdef FEATURE_KIRIN_UFS_PSW
	/*step5: set UFS_SC PSW_POWER_CTRL(0x04) bit[0] to 1,power up psw area*/
	ufs_sys_ctrl_set_bits(host, BIT_UFS_PSW_MTCMOS_EN, PSW_POWER_CTRL);
	udelay(10);

	/*step6:set UFS_SC HC_LP_CTRL(0x0C) bit[8] to 1,make LP_pwr_ready effective*/
	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_PWR_READY, HC_LP_CTRL);

	/*step7:set UFS_SC PSW_CLK_CTRL(0x14) bit[4] to 1,enable clk of PSW*/
	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_PSW_CLK_EN, PSW_CLK_CTRL);

	/*step8:set UFS_SC RESET_CTRL_EN(0x1C) bit[0] to 1,reset PSW area*/
	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_LP_RESET_N, RESET_CTRL_EN);

	if (ufs_kirin_wait_ufssys_bitset_timeout(host,
			BIT_STATUS_LP_RESETCOMPLETE, PHY_RESET_STATUS, 10000)) {
		pr_err("%s: wait lp_resetcomplete time out\n", __func__);
		return -1;
	}

	/*step 9 set PSW_POWER_CTRL(0x04) bit[16] to 0,disable Isolation*/

	ufs_sys_ctrl_clr_bits(host, BIT_UFS_PSW_ISO_CTRL, PSW_POWER_CTRL);
	if (BIT_UFS_PSW_ISO_CTRL & ufs_sys_ctrl_readl(host, PSW_POWER_CTRL)) {
		pr_err("%s: set psw_iso_ctrl fail\n", __func__);
		mdelay(1);
	}
	ufs_sys_ctrl_clr_bits(host, BIT_SYSCTRL_LP_ISOL_EN, HC_LP_CTRL);
#endif

	host->in_suspend = false;
	return 0;
}

static int ufs_kirin_resume_after_set_link_state(struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
#ifdef FEATURE_KIRIN_UFS_PSW
	struct ufs_kirin_host *host = hba->priv;

	if (ufshcd_is_runtime_pm(pm_op))
		return 0;

	ufshcd_writel(hba, host->busthrtl_backup, UFS_REG_OCPTHRTL);
#endif
	return 0;
}
/* platform_get_resource will require resource exclusively, ufs_sys_ctrl used
 * for ufs only, but pctrl and pericrg are common resource */
static int ufs_kirin_get_resource(struct ufs_kirin_host *host)
{
	struct resource *mem_res;
	struct device_node *np = NULL;
	struct device *dev = host->hba->dev;
	struct platform_device *pdev = to_platform_device(dev);

	/* get resource of ufs sys ctrl */
	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	host->ufs_sys_ctrl = devm_ioremap_resource(dev, mem_res);
	if (!host->ufs_sys_ctrl) {
		dev_err(dev, "cannot ioremap for ufs sys ctrl register\n");
		return -ENOMEM;
	}

	np = of_find_compatible_node(NULL, NULL, "hisilicon,crgctrl");
	if (!np) {
		dev_err(host->hba->dev,
			"can't find device node \"hisilicon,crgctrl\"\n");
		return -ENXIO;
	}

	host->pericrg = of_iomap(np, 0);
	if (!host->pericrg) {
		dev_err(host->hba->dev, "crgctrl iomap error!\n");
		return -ENOMEM;
	}

	np = of_find_compatible_node(NULL, NULL, "hisilicon,pctrl");
	if (!np) {
		dev_err(host->hba->dev,
			"can't find device node \"hisilicon,pctrl\"\n");
		return -ENXIO;
	}

	host->pctrl = of_iomap(np, 0);
	if (!host->pctrl) {
		dev_err(host->hba->dev, "pctrl iomap error!\n");
		return -ENOMEM;
	}

	np = of_find_compatible_node(NULL, NULL, "hisilicon,pmctrl");
	if (!np) {
		dev_err(host->hba->dev,
			"can't find device node \"hisilicon,pmctrl\"\n");
		return -ENXIO;
	}

	host->pmctrl = of_iomap(np, 0);
	if (!host->pmctrl) {
		dev_err(host->hba->dev, "pmctrl iomap error!\n");
		return -ENOMEM;
	}

	np = of_find_compatible_node(NULL, NULL, "hisilicon,sysctrl");
	if (!np) {
		dev_err(host->hba->dev,
			"can't find device node \"hisilicon,sysctrl\"\n");
		return -ENXIO;
	}

	host->sysctrl = of_iomap(np, 0);
	if (!host->sysctrl) {
		dev_err(host->hba->dev, "sysctrl iomap error!\n");
		return -ENOMEM;
	}

	/* we only use 64 bit dma */
	dev->dma_mask = &kirin_ufs_dma_mask;

	return 0;
}

/*lint -save -e715*/
static ssize_t ufs_kirin_inline_stat_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
#ifdef CONFIG_SCSI_UFS_INLINE_CRYPTO
	struct ufs_hba *hba = dev_get_drvdata(dev);
#endif
	int ret_show = 0;

#ifdef CONFIG_SCSI_UFS_INLINE_CRYPTO
	if (ufshcd_readl(hba, REG_CONTROLLER_CAPABILITIES)
					& MASK_INLINE_ENCRYPTO_SUPPORT)
		ret_show = 1;
#endif

	return snprintf(buf, PAGE_SIZE, "%d\n", ret_show);
}
/*lint -restore*/

static void ufs_kirin_inline_crypto_attr(struct ufs_hba *hba)
{
	hba->inline_state.inline_attr.show = ufs_kirin_inline_stat_show;

	sysfs_attr_init(&hba->inline_state.inline_attr.attr);
	hba->inline_state.inline_attr.attr.name = "ufs_inline_stat";
	hba->inline_state.inline_attr.attr.mode = S_IRUGO;
	if (device_create_file(hba->dev, &hba->inline_state.inline_attr))
		dev_err(hba->dev, "Failed to create sysfs for ufs_inline_state\n");
}

#if defined(CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO) && defined(CONFIG_HISI_DEBUG_FS)
static ssize_t ufs_kirin_inline_debug_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct ufs_hba *hba = dev_get_drvdata(dev);

	if (hba->inline_debug_flag == DEBUG_LOG_ON || hba->inline_debug_flag == DEBUG_CRYPTO_ON) {
		return snprintf(buf, PAGE_SIZE, "%s\n", "on");
	} else {
		return snprintf(buf, PAGE_SIZE, "%s\n", "off");
	}
}

static ssize_t ufs_kirin_inline_debug_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t count)
{
	struct ufs_hba *hba = dev_get_drvdata(dev);

	if (sysfs_streq(buf, "off")) {
		hba->inline_debug_flag = DEBUG_LOG_OFF;
	} else if (sysfs_streq(buf, "on")) {
		hba->inline_debug_flag = DEBUG_LOG_ON;
	} else if(sysfs_streq(buf, "crypto_on")) {
		hba->inline_debug_flag = DEBUG_CRYPTO_ON;
	} else if(sysfs_streq(buf, "crypto_off")) {
		hba->inline_debug_flag = DEBUG_CRYPTO_OFF;
	} else {
		pr_err("%s: invalid input debug parameter.\n", __func__);
		return -EINVAL;
	}

	return count;
}

static ssize_t ufs_kirin_inline_dun_cci_test(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t count)
{
	int i;
	char buf_temp[65] = {0};

	if(count != 65) {
		pr_err("%s: the input key len is not 64.\n", __func__);
		return count;
	}

	for(i = 0; i < 64; i++) {
		buf_temp[i] = buf[i];
	}
	buf_temp[64] = '\0';
	pr_err("%s: input key is %s\n", __func__, buf_temp);
	test_generate_cci_dun_use_bkdrhash((u8 *)buf_temp, 64);
	return count;
}

static void ufs_kirin_inline_crypto_debug_init(struct ufs_hba *hba)
{
	hba->inline_debug_flag = DEBUG_LOG_OFF;

	hba->inline_debug_state.inline_attr.show = ufs_kirin_inline_debug_show;
	hba->inline_debug_state.inline_attr.store = ufs_kirin_inline_debug_store;
	sysfs_attr_init(&hba->inline_debug_state.inline_attr.attr);
	hba->inline_debug_state.inline_attr.attr.name = "ufs_inline_debug";
	hba->inline_debug_state.inline_attr.attr.mode = S_IRUGO | S_IWUSR;
	if (device_create_file(hba->dev, &hba->inline_debug_state.inline_attr))
		dev_err(hba->dev, "Failed to create sysfs for inline_debug_state\n");

	hba->inline_dun_cci_test.inline_attr.store = ufs_kirin_inline_dun_cci_test;
	sysfs_attr_init(&hba->inline_dun_cci_test.inline_attr.attr);
	hba->inline_dun_cci_test.inline_attr.attr.name = "ufs_inline_dun_cci_test";
	hba->inline_dun_cci_test.inline_attr.attr.mode = S_IWUSR;
	if (device_create_file(hba->dev, &hba->inline_dun_cci_test.inline_attr))
		dev_err(hba->dev, "Failed to create sysfs for inline_dun_cci_test\n");
}
#endif

static void ufs_kirin_set_pm_lvl(struct ufs_hba *hba)
{
	hba->rpm_lvl = UFS_PM_LVL_1;
	hba->spm_lvl = UFS_PM_LVL_3;
}

/**
 * ufs_kirin_advertise_quirks - advertise the known KIRIN UFS controller quirks
 * @hba: host controller instance
 *
 * KIRIN UFS host controller might have some non standard behaviours (quirks)
 * than what is specified by UFSHCI specification. Advertise all such
 * quirks to standard UFS host controller driver so standard takes them into
 * account.
 */
static void ufs_kirin_advertise_quirks(struct ufs_hba *hba)
{
	/* put all our quirks here */
	/*hba->quirks |= UFSHCD_QUIRK_BROKEN_LCC;*/
}

static inline void ufs_kirin_populate_caps_dt(struct device_node *np,
					      struct ufs_kirin_host *host)
{
	unsigned int idx;
	struct st_caps_map caps_arry[] = {
		{"ufs-kirin-use-rate-B", USE_RATE_B},
		{"ufs-kirin-broken-fastauto", BROKEN_FASTAUTO},
		{"ufs-kirin-use-one-line", USE_ONE_LANE},
		{"ufs-kirin-use-HS-GEAR3", USE_HS_GEAR3},
		{"ufs-kirin-use-HS-GEAR2", USE_HS_GEAR2},
		{"ufs-kirin-use-HS-GEAR1", USE_HS_GEAR1},
		{"ufs-kirin-use-snps-mphy-tc", USE_SNPS_MPHY_TC},
		{"ufs-kirin-use-hisi-mphy-tc", USE_HISI_MPHY_TC},
		{"ufs-kirin-since-boston-cs", USE_SINCE_BOSTON_CS},
		{"ufs-kirin-sysbus-207m", USE_SYSBUS_207M},
		{"ufs-kirin-use-fpga-board-clk", USE_FPGA_BOARD_CLK},
		{"ufs-kirin-broken-clk-gate-pypass", BROKEN_CLK_GATE_BYPASS},
		{"ufs-kirin-pre-mphy-calib-auto", PREPARE_MPHY_CALIB_AUTO},
	};

	for (idx = 0; idx < sizeof(caps_arry) / sizeof(struct st_caps_map);
	     idx++) {
		if (of_find_property(np, caps_arry[idx].caps_name, NULL))
			host->caps |= caps_arry[idx].cap_bit;
	}
}

static void ufs_kirin_populate_quirks_dt(struct device_node *np,
					struct ufs_kirin_host *host)
{
	if (of_find_property(np, "ufs-kirin-unipro-termination", NULL))
		host->hba->quirks |= UFSHCD_QUIRK_UNIPRO_TERMINATION;

	if (of_find_property(np, "ufs-kirin-unipro-scrambing", NULL))
		host->hba->quirks |= UFSHCD_QUIRK_UNIPRO_SCRAMBLING;

}

#ifdef CONFIG_HISI_UFS_MANUAL_BKOPS
static void ufs_kirin_populate_mgc_dt(struct device_node *parent_np,
					struct ufs_kirin_host *host)
{
	struct device_node *child_np;
	char *compatible;
	char *model;
	char *rev;
	unsigned int man_id = 0;
	int ret = 0;
	int is_white;
	struct hisi_ufs_bkops_id *bkops_id;
	struct ufs_hba *hba = host->hba;
	struct device *dev = hba->dev;

	INIT_LIST_HEAD(&host->hba->bkops_whitelist);
	INIT_LIST_HEAD(&host->hba->bkops_blacklist);

	for_each_child_of_node(parent_np, child_np) {
		ret = of_property_read_string(child_np, "compatible", (const char **)(&compatible));
		if (ret) {
			pr_err("check the compatible %s\n", child_np->name);
			continue;
		} else {
			if (!strcmp("white", compatible))/*lint !e421*/
				is_white = 1;
			else if (!strcmp("black", compatible))/*lint !e421*/
				is_white = 0;
			else {
				pr_err("check the compatible %s\n", child_np->name);
				continue;
			}
		}

		ret = of_property_read_u32(child_np, "manufacturer_id", &man_id);
		if (ret) {
			pr_err("check the manufacturer_id %s\n", child_np->name);
			continue;
		}

		ret = of_property_read_string(child_np, "model", (const char **)(&model));
		if (ret) {
			pr_err("check the model %s\n", child_np->name);
			continue;
		}

		ret = of_property_read_string(child_np, "rev", (const char **)(&rev));
		if (ret) {
			pr_err("check the rev %s\n", child_np->name);
			continue;
		}

		bkops_id = devm_kzalloc(dev, sizeof(*bkops_id), GFP_KERNEL);
		if (!bkops_id) {
			pr_err("%s %d Failed to alloc bkops_id\n", __func__, __LINE__);
			return;
		}

		bkops_id->manufacturer_id = man_id;
		bkops_id->ufs_model = model;
		bkops_id->ufs_rev = rev;
		INIT_LIST_HEAD(&bkops_id->p);
		if (is_white)
			list_add(&bkops_id->p, &hba->bkops_whitelist);
		else
			list_add(&bkops_id->p, &hba->bkops_blacklist);
	}
} /*lint !e429*/
#endif /* CONFIG_HISI_UFS_MANUAL_BKOPS */

void ufs_kirin_populate_dt(struct device *dev,
				  struct ufs_kirin_host *host)
{
	struct device_node *np = dev->of_node;
	int ret;

	if (!np) {
		dev_err(dev, "can not find device node\n");
		return;
	}

	ufs_kirin_populate_caps_dt(np, host);
#ifdef CONFIG_HISI_UFS_MANUAL_BKOPS
	ufs_kirin_populate_mgc_dt(np, host);
#endif
#ifdef CONFIG_SCSI_UFS_KIRIN_LINERESET_CHECK
	if (of_find_property(np, "ufs-kirin-linereset-check-disable", NULL))
		host->hba->bg_task_enable = false;
	else
		host->hba->bg_task_enable = true;
#endif

	if (of_find_property(np, "ufs-kirin-use-auto-H8", NULL)) {
		host->caps |= USE_AUTO_H8;
		host->hba->caps |= UFSHCD_CAP_AUTO_HIBERN8;	/*lint !e648 */
		host->hba->ahit_ts = 4;	/* 4 -> 10ms */
		host->hba->ahit_ah8itv = 1;
	}

	ufs_kirin_populate_quirks_dt(np, host);
	if (!of_property_read_u32(np, "reset-gpio", &(host->reset_gpio))) {
		if (0 > gpio_request(host->reset_gpio, "ufs_device_reset")) {
			pr_err("%s: could not request gpio %d\n", __func__,
			       host->reset_gpio);
			host->reset_gpio = 0xFFFFFFFF;
		}
	} else {
		host->reset_gpio = 0xFFFFFFFF;
	}

	if (of_find_property(np, "ufs-kirin-ssu-by-self", NULL))
		host->hba->caps |= UFSHCD_CAP_SSU_BY_SELF;

	if (of_find_property(np, "ufs-on-emulator", NULL))
		host->hba->host->is_emulator = 1;
	else
		host->hba->host->is_emulator = 0;

	ret =
	    of_property_match_string(np, "ufs-0db-equalizer-product-names",
				     ufs_product_name);
	if (ret >= 0) {
		dev_info(dev, "find %s in dts\n", ufs_product_name);
		host->tx_equalizer = 0;
	} else {
#ifdef UFS_TX_EQUALIZER_35DB
		host->tx_equalizer = 35;
#endif
#ifdef UFS_TX_EQUALIZER_60DB
		host->tx_equalizer = 60;
#endif
	}

	if (of_property_read_u32(np, "dly-ms-after-dev-rst",
		&(host->dly_ms_after_rst))) {
		dev_warn(dev, "dly-ms-after-dev-rst property not found,use default val\n");
		host->dly_ms_after_rst = 40;
	}

	if (of_find_property(np, "kirin970-ufs-memory-repair-workround", NULL)) {
		pr_err("%s with kirin970-ufs-memory-repair-workround\n", __func__);
		host->mr_workround_enable = true;
	} else {
		pr_err("%s no kirin970-ufs-memory-repair-workround\n", __func__);
		host->mr_workround_enable = false;
	}
}

/**
 * ufs_kirin_init
 * @hba: host controller instance
 */
static int ufs_kirin_init(struct ufs_hba *hba)
{
	int err = 0;
	struct device *dev = hba->dev;
	struct ufs_kirin_host *host;

#ifdef CONFIG_HISI_BOOTDEVICE
	if (get_bootdevice_type() == BOOT_DEVICE_UFS)
		set_bootdevice_name(dev);
#endif

	host = devm_kzalloc(dev, sizeof(*host), GFP_KERNEL);
	if (!host) {
		err = -ENOMEM;
		dev_err(dev, "%s: no memory for kirin ufs host\n", __func__);
		goto out;
	}

	host->hba = hba;
	hba->priv = (void *)host;

	host->clk_ufsio_ref = devm_clk_get(dev, "clk_ufsio_ref");
	if (IS_ERR(host->clk_ufsio_ref)) {
		err = PTR_ERR(host->clk_ufsio_ref);
		dev_err(dev, "clk_ref clock not found.\n");
		goto out;
	}

	ufs_kirin_advertise_quirks(hba);

	ufs_kirin_set_pm_lvl(hba);

	ufs_kirin_populate_dt(dev, host);

	err = ufs_kirin_get_resource(host);
	if (err)
		goto host_free;

	ufs_kirin_regulator_init(hba);

	ufs_kirin_clk_init(hba);

	ufs_kirin_soc_init(hba);

#ifdef CONFIG_HISI_BOOTDEVICE
	if (get_bootdevice_type() == BOOT_DEVICE_UFS)
#endif
		ufs_kirin_inline_crypto_attr(hba);

	scsi_logging_level = UFS_SCSI_LOGGING_LEVEL;

	goto out;

host_free:
	devm_kfree(dev, host);
	hba->priv = NULL;
out:
	return err;
}

static void ufs_kirin_exit(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = hba->priv;

	if ((host->caps & USE_HISI_MPHY_TC) && host->i2c_client) {
		i2c_unregister_device(host->i2c_client);
		host->i2c_client = NULL;
	}

	return;
}
#ifdef CONFIG_SCSI_UFS_KIRIN_LINERESET_CHECK
/*lint -e550 -e732 -e527 -e529*/
static bool ufs_kirin_can_checking(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = hba->priv;
	if (hba->ufshcd_state != UFSHCD_STATE_OPERATIONAL) {
		return false;
	}
	if (hba->is_hibernate) {
		return false;
	}
	if (hba->pm_op_in_progress) {
		return false;
	}
	if (host->in_suspend) {
		return false;
	}

	return true;
}

static int ufs_kirin_daemon_thread(void *d)
{
	struct ufs_hba *hba = d;
	struct ufs_kirin_host *host = hba->priv;
	unsigned long flags;
	u32 link_state;
	u32 lane0_tx_state, lane0_rx_state, lane1_rx_state;
	u32 linereset_ind;
	u32 lane0_tx_state_p;
	u32 check_times;

	do {
		msleep(1000);
		if (!ufs_kirin_can_checking(hba))
			continue;
		ufs_sys_ctrl_writel(host, 0x08081010, PHY_MPX_TEST_CTRL);
		wmb();
		link_state = ufs_sys_ctrl_readl(host, PHY_MPX_TEST_OBSV);

		lane0_tx_state = (link_state & (0xF << 24)) >> 24;
		lane0_rx_state = (link_state & (0x7 << 8)) >> 8;
		lane1_rx_state = (link_state & (0x7 << 16)) >> 16;

		if (((lane0_tx_state == 0x6) || (lane0_tx_state == 0x7)) && ((lane0_rx_state == 0x2) || (lane1_rx_state == 0x2))) {
			msleep(5);
			if (!ufs_kirin_can_checking(hba))
				continue;
			hba->reg_uecpa = ufshcd_readl(hba, REG_UIC_ERROR_CODE_PHY_ADAPTER_LAYER);
			check_times = 0;
re_check:
			lane0_tx_state_p = lane0_tx_state;
			msleep(50);
			if (!ufs_kirin_can_checking(hba))
				continue;
			ufs_sys_ctrl_writel(host, 0x08081010, PHY_MPX_TEST_CTRL);
			wmb();
			link_state = ufs_sys_ctrl_readl(host, PHY_MPX_TEST_OBSV);

			lane0_tx_state = (link_state & (0xF << 24)) >> 24;
			lane0_rx_state = (link_state & (0x7 << 8)) >> 8;
			lane1_rx_state = (link_state & (0x7 << 16)) >> 16;

			if ((lane0_tx_state == lane0_tx_state_p) && ((lane0_rx_state == 0x2) || (lane1_rx_state == 0x2))) {
				msleep(5);
				if (!ufs_kirin_can_checking(hba))
					continue;
				hba->reg_uecpa = ufshcd_readl(hba, REG_UIC_ERROR_CODE_PHY_ADAPTER_LAYER);
				linereset_ind = (hba->reg_uecpa & (0x1 << 4)) >> 4;
				if (linereset_ind) {
					check_times = 0;
					goto re_check;
				} else if (check_times < 3) {
					check_times++;
					goto re_check;
				} else {
					dev_err(hba->dev, "unipro register error happy, reset hba\n");
					spin_lock_irqsave(hba->host->host_lock, flags);
					/* block commands from scsi mid-layer */
					scsi_block_requests(hba->host);

					/* transfer error masks to sticky bits */
					hba->ufshcd_state = UFSHCD_STATE_EH_SCHEDULED;
					hba->force_host_reset = 1;
					queue_kthread_work(&hba->eh_worker, &hba->eh_work);
					spin_unlock_irqrestore(hba->host->host_lock, flags);
					msleep(4000);
				}
			}

		}
	} while (1);

	return 0;
}
/*lint +e550 +e732 +e527 +e529*/
#endif

/*lint -save -e529 -e438 -e732*/
#ifdef CONFIG_SCSI_UFS_HS_ERROR_RECOVER
int ufs_kirin_get_pwr_by_sysctrl(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = (struct ufs_kirin_host *)hba->priv;
	u32 link_state;
	u32 lane0_rx_state;

	ufs_sys_ctrl_writel(host, 0x08081010, PHY_MPX_TEST_CTRL);
	wmb();
	link_state = ufs_sys_ctrl_readl(host, PHY_MPX_TEST_OBSV);
	lane0_rx_state = (link_state & (0x7 << 8)) >> 8;

	if (lane0_rx_state == 2)
		return SLOW;
	else if (lane0_rx_state == 3)
		return FAST;
	else
		return -1;
}
#endif
/*lint -restore*/

/**
 * struct ufs_hba_kirin_vops - UFS KIRIN specific variant operations
 *
 * The variant operations configure the necessary controller and PHY
 * handshake during initialization.
 */
/*lint -save -e785*/
const struct ufs_hba_variant_ops ufs_hba_kirin_vops = {
	.name = "kirin",
	.init = ufs_kirin_init,
	.exit = ufs_kirin_exit,
	.setup_clocks = NULL,
	.hce_enable_notify = ufs_kirin_hce_enable_notify,
	.link_startup_notify = ufs_kirin_link_startup_notify,
	.pwr_change_notify = ufs_kirin_pwr_change_notify,
	.full_reset = ufs_kirin_full_reset,
	.device_reset = ufs_kirin_device_hw_reset,
	.suspend_before_set_link_state = ufs_kirin_suspend_before_set_link_state,
	.suspend = ufs_kirin_suspend,
	.resume = ufs_kirin_resume,
	.resume_after_set_link_state = ufs_kirin_resume_after_set_link_state,
#ifdef CONFIG_SCSI_UFS_INLINE_CRYPTO
	.uie_config_init = ufs_kirin_uie_config_init,
	.uie_utrd_pre = ufs_kirin_uie_utrd_prepare,
#endif
	.dbg_hci_dump = kirin_ufs_hci_log,
	.dbg_uic_dump = kirin_ufs_uic_log,
#ifdef CONFIG_SCSI_UFS_KIRIN_LINERESET_CHECK
	.background_thread = ufs_kirin_daemon_thread,
#endif
#ifdef CONFIG_SCSI_UFS_HS_ERROR_RECOVER
	.get_pwr_by_debug_register = ufs_kirin_get_pwr_by_sysctrl,
#endif
};
/*lint -restore*/
#ifdef CONFIG_SCSI_UFS_KIRIN_V21
static void set_rhold(struct ufs_kirin_host *host)
{
	if (ufs_sctrl_readl(host, SCDEEPSLEEPED_OFFSET) & EFUSE_RHOLD_BIT)
		ufs_sctrl_writel(host,
			(MASK_UFS_MPHY_RHOLD | BIT_UFS_MPHY_RHOLD),
			UFS_DEVICE_RESET_CTRL);
}

static void ufs_kirin_v21_soc_init(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = (struct ufs_kirin_host *)hba->priv;
	u32 reg;
	int ret;

	pr_info("%s ++\n", __func__);

	if (host->caps & USE_SYSBUS_207M) {
		/*CS LOW TEMP 207M*/
		writel(BIT(SOC_SCTRL_SCPERDIS4_gt_clk_ufs_subsys_START),
			SOC_SCTRL_SCPERDIS4_ADDR(host->sysctrl));
		writel(0x003F0007, SOC_SCTRL_SCCLKDIV9_ADDR(host->sysctrl));
		writel(BIT(SOC_SCTRL_SCPEREN4_gt_clk_ufs_subsys_START),
			SOC_SCTRL_SCPEREN4_ADDR(host->sysctrl));
	}

	writel(1<<SOC_UFS_Sysctrl_UFS_UMECTRL_ufs_ies_en_mask_START,\
		SOC_UFS_Sysctrl_UFS_UMECTRL_ADDR(host->ufs_sys_ctrl));

	writel(1<<(SOC_UFS_Sysctrl_CRG_UFS_CFG_ip_rst_ufs_START+16) | 0, \
		SOC_UFS_Sysctrl_CRG_UFS_CFG_ADDR(host->ufs_sys_ctrl));

	/* efuse indicates enable rhold or not */
	set_rhold(host);
	ufs_sys_ctrl_set_bits(host, BIT_UFS_PSW_MTCMOS_EN, PSW_POWER_CTRL); /* HC_PSW powerup */
	udelay(10);
	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_PWR_READY, HC_LP_CTRL); /* notify PWR ready */
#if 0
	ufs_sys_ctrl_writel(host, MASK_UFS_DEVICE_RESET | 0,
		UFS_DEVICE_RESET_CTRL);
#endif
	if (host->caps & USE_SINCE_BOSTON_CS) {
		/*STEP 4 CLOSE MPHY REF CLOCK*/
		ufs_sys_ctrl_clr_bits(host, BIT_SYSCTRL_REF_CLOCK_EN, PHY_CLK_CTRL);
		reg = ((0x3 << 2) | (0x7 << (2 + 16))); /*Bit[4:2], div =4*/
		writel(reg,
		       SOC_UFS_Sysctrl_CRG_UFS_CFG_ADDR(host->ufs_sys_ctrl));
	}
	reg = ufs_sys_ctrl_readl(host, PHY_CLK_CTRL);
	if (host->caps & USE_SINCE_BOSTON_CS) {
		if (host->caps & USE_FPGA_BOARD_CLK) {
			reg = (reg & ~MASK_SYSCTRL_CFG_CLOCK_FREQ) | 0x14;
		} else {
			if (host->caps & USE_SYSBUS_207M)
				reg = (reg & ~MASK_SYSCTRL_CFG_CLOCK_FREQ) | 0x34;
			else
				reg = (reg & ~MASK_SYSCTRL_CFG_CLOCK_FREQ) | 0x3C;
		}
	} else {
		reg = (reg & ~MASK_SYSCTRL_CFG_CLOCK_FREQ) | 0x14;
	}
	ufs_sys_ctrl_writel(host, reg, PHY_CLK_CTRL); /* set cfg clk freq */
	ufs_sys_ctrl_clr_bits(host, MASK_SYSCTRL_REF_CLOCK_SEL, PHY_CLK_CTRL); /* set ref clk freq */

	/* enable ref_clk_en override(bit5) & override value = 1(bit4), with mask */
	ufs_sys_ctrl_writel(host, 0x00300030, UFS_DEVICE_RESET_CTRL);

	/* bypass ufs clk gate */
	ufs_sys_ctrl_set_bits(host, MASK_UFS_CLK_GATE_BYPASS, CLOCK_GATE_BYPASS);
	ufs_sys_ctrl_set_bits(host, MASK_UFS_SYSCRTL_BYPASS, UFS_SYSCTRL);

	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_PSW_CLK_EN, PSW_CLK_CTRL); /* open psw clk */
	ufs_sys_ctrl_clr_bits(host, BIT_UFS_PSW_ISO_CTRL, PSW_POWER_CTRL); /* disable ufshc iso */
	ufs_sys_ctrl_clr_bits(host, BIT_UFS_PHY_ISO_CTRL, PHY_ISO_EN); /* disable phy iso */
	ufs_sys_ctrl_clr_bits(host, BIT_SYSCTRL_LP_ISOL_EN, HC_LP_CTRL); /* notice iso disable */

	writel(1<<(SOC_UFS_Sysctrl_CRG_UFS_CFG_ip_arst_ufs_START+16) |\
		1<<SOC_UFS_Sysctrl_CRG_UFS_CFG_ip_arst_ufs_START,\
		SOC_UFS_Sysctrl_CRG_UFS_CFG_ADDR(host->ufs_sys_ctrl));

	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_LP_RESET_N, RESET_CTRL_EN); /* disable lp_reset_n */
	mdelay(1);

	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_REF_CLOCK_EN,
			      PHY_CLK_CTRL); /* open clock of M-PHY */
	if (host->caps & USE_HISI_MPHY_TC) {
		ufs_i2c_writel(hba, (unsigned int) BIT(6),
			       SC_RSTDIS); /*enable Device Reset*/
		ufs_i2c_readl(hba, &reg, SC_UFS_REFCLK_RST_PAD);
		reg = reg & (~(BIT(2) | BIT(10)));
		/*output enable, For EMMC to high dependence, open
		 * DA_UFS_OEN_RST
		 * and DA_UFS_OEN_REFCLK*/
		ufs_i2c_writel(hba, reg, SC_UFS_REFCLK_RST_PAD);
		/* FPGA is not usable, only for open the clk, to match
		 * clk_unprepare_enble later in suspend&fullreset func */
		ret = clk_prepare_enable(host->clk_ufsio_ref);
		if (ret) {
			pr_err("%s ,clk_prepare_enable failed\n", __func__);
			return;
		}
		mdelay(2);
		ufs_i2c_writel(hba, (unsigned int)BIT(6),
				SC_RSTEN); /*disable Device Reset*/
	} else {
		ufs_sys_ctrl_writel(host, MASK_UFS_DEVICE_RESET | 0,
				    UFS_DEVICE_RESET_CTRL); /* reset device */
		#if 1
		ret = clk_prepare_enable(host->clk_ufsio_ref);
		if (ret) {
			pr_err("%s ,clk_prepare_enable failed\n", __func__);
			return;
		}
		#else
		writel((1 << SOC_SCTRL_SCPEREN4_gt_clk_ufsio_ref_START),
		       SOC_SCTRL_SCPEREN4_ADDR(
			   host->sysctrl)); /* open clock of device */
		#endif

		mdelay(1);

		reg = readl(SOC_SCTRL_SCDEEPSLEEPED_ADDR(host->sysctrl));
		if (0 == (reg & (1 << 21))) {
			mdelay(1);
			ufs_sys_ctrl_writel(
			    host, MASK_UFS_DEVICE_RESET | BIT_UFS_DEVICE_RESET,
			    UFS_DEVICE_RESET_CTRL); /* disable Device Reset */
		}
	}
	mdelay((unsigned long)host->dly_ms_after_rst);

#if 0
	/* enable the fix of linereset recovery, and enable rx_reset/tx_rest beat */
	/* enable ref_clk_en override(bit5) & override value = 1(bit4), with mask */
	ufs_sys_ctrl_writel(host, 0x03300330, UFS_DEVICE_RESET_CTRL);
#endif

	writel(1<<(SOC_UFS_Sysctrl_CRG_UFS_CFG_ip_rst_ufs_START+16) |\
		1<<SOC_UFS_Sysctrl_CRG_UFS_CFG_ip_rst_ufs_START,\
		SOC_UFS_Sysctrl_CRG_UFS_CFG_ADDR(host->ufs_sys_ctrl));

	reg = readl(SOC_UFS_Sysctrl_CRG_UFS_CFG_ADDR(host->ufs_sys_ctrl));
	if (reg & (1<<SOC_UFS_Sysctrl_CRG_UFS_CFG_ip_rst_ufs_START))
		mdelay(1);

	host->need_memrepair = ufs_kirin_need_memrepair(hba);

	/*set SOC_SCTRL_SCBAKDATA11_ADDR ufs bit to 1 when init*/
	hisi_idle_sleep_vote(ID_UFS, 1);

	pr_info("%s --\n", __func__);
	return;
}

static int create_i2c_client(struct ufs_hba *hba)
{
	struct ufs_kirin_host *host = hba->priv;
	struct i2c_adapter *adapter;

	adapter = i2c_get_adapter(UFS_I2C_BUS);
	if (!adapter) {
		pr_err("%s i2c_get_adapter error\n", __func__);
		return -EIO;
	}
	host->i2c_client = i2c_new_device(adapter, &ufs_i2c_board_info);
	if (!host->i2c_client) {
		pr_err("%s i2c_new_device error\n", __func__);
		return -EIO;
	}
	return 0;
}

void cs_chipsel_gpio_config(struct ufs_kirin_host *host, struct device *dev)
{
	int err = 0;

	host->chipsel_gpio = of_get_named_gpio(dev->of_node, "cs-gpios", 0);
	if (!gpio_is_valid(host->chipsel_gpio)) {
		pr_err("%s: gpio of host->chipsel_gpio is not valid,check DTS\n", __func__);
	}
	err = gpio_request((unsigned int)host->chipsel_gpio, "cs-gpio");
	if (err < 0) {
		pr_err("Can`t request cs chipsel gpio %d\n", host->chipsel_gpio);
	}
	err = gpio_direction_output((unsigned int)host->chipsel_gpio, 0);
	if (err < 0) {
		pr_err("%s: could not set gpio %d output push down\n", __func__, host->chipsel_gpio);
	}
}

/**
 * ufs_kirin_v21_init
 * @hba: host controller instance
 */
static int ufs_kirin_v21_init(struct ufs_hba *hba)
{
	int err = 0;
	struct device *dev = hba->dev;
	struct ufs_kirin_host *host;

#ifdef CONFIG_HISI_BOOTDEVICE
	if (get_bootdevice_type() == BOOT_DEVICE_UFS)
		set_bootdevice_name(dev);
#endif

	host = devm_kzalloc(dev, sizeof(*host), GFP_KERNEL);
	if (!host) {
		err = -ENOMEM;
		dev_err(dev, "%s: no memory for kirin ufs host\n", __func__);
		goto out;
	}

	host->hba = hba;
	hba->priv = (void *)host;

	host->clk_ufsio_ref = devm_clk_get(dev, "clk_ufsio_ref");
	if (IS_ERR(host->clk_ufsio_ref)) {
		err = PTR_ERR(host->clk_ufsio_ref);
		dev_err(dev, "clk_ufsio_ref not found.\n");
		goto out;
	}

	ufs_kirin_advertise_quirks(hba);

	ufs_kirin_set_pm_lvl(hba);

	ufs_kirin_populate_dt(dev, host);
	if ((host->caps & USE_HISI_MPHY_TC) && !host->i2c_client) {
		cs_chipsel_gpio_config(host, dev);
		err = create_i2c_client(hba);
		if (err) {
			dev_err(dev, "create i2c client error\n");
			goto host_free;
		}
	}
	err = ufs_kirin_get_resource(host);
	if (err)
		goto host_free;

	ufs_kirin_regulator_init(hba);

	ufs_kirin_v21_soc_init(hba);

#ifdef CONFIG_HISI_BOOTDEVICE
	if (get_bootdevice_type() == BOOT_DEVICE_UFS) {
#endif
		ufs_kirin_inline_crypto_attr(hba);

#if defined(CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO) && defined(CONFIG_HISI_DEBUG_FS)
		ufs_kirin_inline_crypto_debug_init(hba);
#endif

#ifdef CONFIG_HISI_BOOTDEVICE
	}
#endif

	goto out;

host_free:
	devm_kfree(dev, host);
	hba->priv = NULL;
out:
	return err;
}

static int ufs_kirin_v21_suspend(struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
	struct ufs_kirin_host *host = hba->priv;

	/*set SOC_SCTRL_SCBAKDATA11_ADDR ufs bit to 0 when idle*/
	hisi_idle_sleep_vote(ID_UFS, 0);

	if (ufshcd_is_runtime_pm(pm_op))
		return 0;

	if (host->in_suspend) {
		WARN_ON(1);/*lint !e730*/
		return 0;
	}

	ufs_sys_ctrl_clr_bits(host, BIT_SYSCTRL_REF_CLOCK_EN, PHY_CLK_CTRL);
	udelay(10);
	/* set ref_dig_clk override of PHY PCS to 0 */
	ufs_sys_ctrl_writel(host, 0x00100000, UFS_DEVICE_RESET_CTRL);
#if 1
	clk_disable_unprepare(host->clk_ufsio_ref);
#else
	writel((1<<SOC_SCTRL_SCPERDIS4_gt_clk_ufsio_ref_START),\
		SOC_SCTRL_SCPERDIS4_ADDR(host->sysctrl)); /* 关闭Device 的参考时钟 */
#endif
	if (host->need_memrepair) {
		ufs_kirin_memrepair_suspend(hba);
	}

	host->in_suspend = true;

	return 0;
}


static int ufs_kirin_v21_resume(struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
	struct ufs_kirin_host *host = hba->priv;
	int ret;

	/*set SOC_SCTRL_SCBAKDATA11_ADDR ufs bit to 1 when busy*/
	hisi_idle_sleep_vote(ID_UFS, 1);

	if (!host->in_suspend)
		return 0;

	if (host->need_memrepair) {
		ufs_kirin_memrepair_resume(hba);
	}
#if 1
	ret = clk_prepare_enable(host->clk_ufsio_ref);
	if (ret) {
		pr_err("%s ,clk_prepare_enable failed\n", __func__);
		return ret;
	}
#else
	writel((1<<SOC_SCTRL_SCPEREN4_gt_clk_ufsio_ref_START),\
		SOC_SCTRL_SCPEREN4_ADDR(host->sysctrl)); /* 打开Device 的参考时钟 */
#endif
	udelay(1);
	/* set ref_dig_clk override of PHY PCS to 1 */
	ufs_sys_ctrl_writel(host, 0x00100010, UFS_DEVICE_RESET_CTRL);
	udelay(10);
	ufs_sys_ctrl_set_bits(host, BIT_SYSCTRL_REF_CLOCK_EN, PHY_CLK_CTRL);


	host->in_suspend = false;
	return 0;
}


/**
 * struct ufs_hba_kirin_vops - UFS KIRIN specific variant operations
 *
 * The variant operations configure the necessary controller and PHY
 * handshake during initialization.
 */
 /*lint -save -e785*/
const struct ufs_hba_variant_ops ufs_hba_kirin_v21_vops = {
	.name = "kirin",
	.init = ufs_kirin_v21_init,
	.exit = ufs_kirin_exit,
	.setup_clocks = NULL,
	.hce_enable_notify = ufs_kirin_hce_enable_notify,
	.link_startup_notify = ufs_kirin_link_startup_notify,
	.pwr_change_notify = ufs_kirin_pwr_change_notify,
	.full_reset = ufs_kirin_full_reset,
	.device_reset = ufs_kirin_device_hw_reset,
	.suspend_before_set_link_state = ufs_kirin_suspend_before_set_link_state,
	.suspend = ufs_kirin_v21_suspend,
	.resume = ufs_kirin_v21_resume,
	.resume_after_set_link_state = ufs_kirin_resume_after_set_link_state,
	.dbg_hci_dump = kirin_ufs_hci_log,
	.dbg_uic_dump = kirin_ufs_uic_log,
#ifdef CONFIG_SCSI_UFS_INLINE_CRYPTO
	.uie_config_init = ufs_kirin_uie_config_init,
	.uie_utrd_pre = ufs_kirin_uie_utrd_prepare,
#endif
#ifdef CONFIG_SCSI_UFS_HS_ERROR_RECOVER
	.get_pwr_by_debug_register = ufs_kirin_get_pwr_by_sysctrl,
#endif
};
/*lint -restore*/
#endif

EXPORT_SYMBOL(ufs_hba_kirin_vops);
