/*
 * hi6403es_hifi_config.c -- adapt 64xx hifi misc to 6403
 *
 * Copyright (c) 2015 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/hisi/hi64xx_hifi_misc.h>
#include <linux/hisi/hi64xx/hi6403es_regs.h>
#include <linux/hisi/hi64xx/hi6403es_dsp_regs.h>
#include "hi64xx_hifi_debug.h"
#include "hi64xx_hifi_img_dl.h"
#include "hi64xx_hifi_om.h"
#include "hi6403es_hifi_config.h"
#include "../soundtrigger/soundtrigger_dma_drv.h"

static void hi6403es_hifi_runstall_cfg(bool pull_down)
{
	IN_FUNCTION;

	if (pull_down) {
		/* Pull down runstall of HIFI*/
		hi64xx_hifi_reg_clr_bit(HI64xx_DSP_SC_DSP_CTRL0, 2);
	} else {
		/* Pull up runstall of HIFI*/
		hi64xx_hifi_reg_set_bit(HI64xx_DSP_SC_DSP_CTRL0, 2);
	}
	OUT_FUNCTION;
}

static void hi6403es_hifi_watchdog_enable(bool enable)
{
	IN_FUNCTION;
	if (enable) {
		/*bit3:wd_pclk_en 0:disable 1:enable*/
		hi64xx_hifi_reg_set_bit(HI64xx_DSP_APB_CLK_CFG, 3);
	} else {
		/*bit3:wd_pclk_en 0:disable 1:enable*/
		hi64xx_hifi_reg_clr_bit(HI64xx_DSP_APB_CLK_CFG, 3);
	}
	OUT_FUNCTION;
}

static void hi6403es_hifi_notify_dsp(void)
{
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_DSP_NMI, 2);
}

/* when hifi is not running, ap access APB register with 0x20007020 bit[2]=1'b0
 * when hifi is running, ap & hifi access APB register with 0x20007020 bit[2]=0'b0*/
static void hi6403es_ram2axi_cfg_with_hifi(bool enable)
{
	IN_FUNCTION;

	hi64xx_hifi_write_reg(HI64xx_DSP_RAM2AXI_CTRL, 0x1);

	OUT_FUNCTION;
}

static void hi6403es_hifi_clk_enable(bool enable)
{
	IN_FUNCTION;

	if (enable) {

	} else {
	}

	OUT_FUNCTION;
}

static void hi6403es_dsp_power_ctrl(bool enable)
{
	IN_FUNCTION;

	if (enable) {
		hi64xx_hifi_reg_clr_bit(HI64xx_DSP_LP_CTRL, 0);
		hi64xx_hifi_reg_clr_bit(HI64xx_DSP_LP_CTRL, 1);
		hi64xx_hifi_write_reg(HI6403ES_EC_REG_DSP1, 0xFF);

		hi64xx_hifi_reg_set_bit(HI64xx_DSP_SC_DSP_CTRL0, 3);
		hi64xx_hifi_reg_set_bit(HI64xx_DSP_SC_DSP_CTRL0, 4);
		hi64xx_hifi_reg_set_bit(HI64xx_DSP_CLK_CFG, 6);
		hi64xx_hifi_reg_set_bit(HI6403ES_DSP_CTRL6_DMAC, 6);
	} else {
		hi64xx_hifi_reg_clr_bit(HI6403ES_DSP_CMD_STAT_VLD, 0);

		hi64xx_hifi_write_reg(HI6403ES_EC_REG_DSP1, 0x0);

		hi64xx_hifi_reg_set_bit(HI64xx_DSP_LP_CTRL, 1);
		hi64xx_hifi_reg_set_bit(HI64xx_DSP_LP_CTRL, 0);

		hi64xx_hifi_reg_clr_bit(HI64xx_DSP_SC_DSP_CTRL0, 3);

		hi64xx_hifi_reg_clr_bit(HI64xx_DSP_SC_DSP_CTRL0, 4);
		/*bit6:hifi_div_clk_en 0:disable 1:enable*/
		hi64xx_hifi_reg_clr_bit(HI64xx_DSP_CLK_CFG, 6);
		hi64xx_hifi_reg_clr_bit(HI6403ES_DSP_CTRL6_DMAC, 6);
	}

	OUT_FUNCTION;
}

static void hi6403es_hifi_init(void)
{
	IN_FUNCTION;

	/* 1.reset dsp_pd_srst_req */
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_SW_RST_REQ, 2);
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_SW_RST_REQ, 3);
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_SW_RST_REQ, 5);
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_SW_RST_REQ, 6);

	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_OCDHALTON_RST, 0);
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_DEBUG_RST, 0);

	/* 2.close dspif clocks, and soft reset dspif */
	hi64xx_hifi_write_reg(HI6403ES_DSP_I2S_DSPIF_CLK_EN, 0x0);

	/* 3.turn on dsp_top_mtcmos_ctrl*/
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_LP_CTRL, 0);
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_LP_CTRL, 3);
	hi64xx_hifi_reg_clr_bit(HI6403ES_DSP_LP_CTRL1,0);

	/* 4.enable dsp_top_iso_ctrl */
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_LP_CTRL, 1);
	hi64xx_hifi_reg_clr_bit(HI6403ES_DSP_LP_CTRL1,1);
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_LP_CTRL, 4);

	/* 5.sc_dsp_en dsp low power enable */
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_SC_DSP_CTRL0, 0);

	hi64xx_hifi_reg_set_bit(HI64xx_DSP_SC_DSP_CTRL0, 3);
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_SC_DSP_CTRL0, 4);
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_SC_DSP_CTRL0, 6);

	/* pull up runstall */
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_SC_DSP_CTRL0, 2);

	/* 6.apb_pd_pclk_en Open APB clock of power-off area */
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_APB_CLK_CFG, 6);

	//hi64xx_hifi_write_reg(HI6403ES_CFG_REG_CLK_CTRL, 0x4);
	/* 7.Release dsp_pd_srst_req */
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_SW_RST_REQ, 5);
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_SW_RST_REQ, 6);
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_SW_RST_REQ, 2);
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_SW_RST_REQ, 3);
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_OCDHALTON_RST, 0);
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_DEBUG_RST, 0);

	/* disable watchdog */
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_APB_CLK_CFG, 3);

	hi64xx_memset(HI6403ES_AP_TO_DSP_MSG_ADDR, (size_t)HI6403ES_AP_TO_DSP_MSG_SIZE);
	hi64xx_memset(HI6403ES_AP_DSP_CMD_ADDR, (size_t)HI6403ES_AP_DSP_CMD_SIZE);
	hi64xx_memset(HI6403ES_DSP_TO_AP_MSG_ADDR, (size_t)HI6403ES_DSP_TO_AP_MSG_SIZE);
	hi64xx_memset(HI6403ES_MLIBPARA_ADDR, (size_t)HI6403ES_MLIB_PARA_MAX_SIZE);

	OUT_FUNCTION;
}

static void hi6403es_hifi_deinit(void)
{
	IN_FUNCTION;

	/* Close watchdog clock */
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_APB_CLK_CFG, 3);

	/* Close HIFI clock */
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_SC_DSP_CTRL0, 0);
	hi6403es_hifi_clk_enable(false);

	/* Close APB clock */
	hi64xx_hifi_reg_clr_bit(HI64xx_DSP_APB_CLK_CFG, 6);

	/* Close DSPIF clocks, and soft reset DSPIF */
	hi64xx_hifi_write_reg(HI6403ES_DSP_I2S_DSPIF_CLK_EN, 0x0);

	/* Enable isolation cell */
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_LP_CTRL, 1);

	/* Soft reset HIFI */
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_SW_RST_REQ, 5);

	hi64xx_hifi_reg_set_bit(HI6403ES_DSP_LP_CTRL1,0);
	hi64xx_hifi_reg_set_bit(HI6403ES_DSP_LP_CTRL1,1);

	/* Turn off power of power-off area */
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_LP_CTRL, 0);

	/* Pull up runstall of HIFI */
	hi64xx_hifi_reg_set_bit(HI64xx_DSP_SC_DSP_CTRL0, 2);

	OUT_FUNCTION;
}

static void hi6403es_soundtrigger_fasttrans_ctrl(bool enable, bool fm)
{
	IN_FUNCTION;

	if (enable) {
		/* set S3 CTRL */
		hi64xx_hifi_write_reg(HI6403ES_DSP_S3_CTRL_L, 0x0E);/* 192K */
		/* DSP IF 5 */
		hi64xx_hifi_write_reg(HI6403ES_DSP_S3_CTRL_H, 0x66);/* 192K */
		/* use DSP IF5/DSP IF1 instead of PGA */
		hi64xx_hifi_write_reg(HI6403ES_SLIM_UP_EN2, 0xC7);/* use DSP IF5, DSP IF1 */
		/* Slimbus u5,u6,u1,u2 */
		hi64xx_hifi_write_reg(HI6403ES_SLIM_UP_EN1, 0xFF);/* Open u5,u6 */
		/* Slimbus u5,u6 sample rate */
		hi64xx_hifi_write_reg(HI6403ES_SLIM_CTRL_5, 0x66);/* 192K */

		/* set S1 CTRL */
		hi64xx_hifi_write_reg(HI6403ES_DSP_S1_CTRL_L, 0x04);/* 48K */
		/* DSP IF 1 */
		hi64xx_hifi_write_reg(HI6403ES_DSP_S1_CTRL_H, 0x14);/* 16K */
		/* Slimbus u1,u2 sample rate */
		hi64xx_hifi_write_reg(HI6403ES_SLIM_CTRL_3, 0x44);/* 48K */
	} else {
		/* set S3 CTRL */
		hi64xx_hifi_write_reg(HI6403ES_DSP_S3_CTRL_L, 0x00);/* 8K */
		/* DSP IF 5 */
		hi64xx_hifi_write_reg(HI6403ES_DSP_S3_CTRL_H, 0x00);/* 8K */
		/* use DSP IF5/DSP IF1 instead of PGA */
		hi64xx_hifi_write_reg(HI6403ES_SLIM_UP_EN2, 0x00);/* disuse DSP IF5, DSP IF1 */
		/* Slimbus u5,u6,u1,u2 */
		hi64xx_hifi_write_reg(HI6403ES_SLIM_UP_EN1, 0x00);/* close u5,u6,u1,u2 */
		/* Slimbus u5,u6 sample rate */
		hi64xx_hifi_write_reg(HI6403ES_SLIM_CTRL_5, 0x11);/* 16K */

		/* set S1 CTRL */
		hi64xx_hifi_write_reg(HI6403ES_DSP_S1_CTRL_L, 0x04);/* 48K */
		/* DSP IF 1 */
		hi64xx_hifi_write_reg(HI6403ES_DSP_S1_CTRL_H, 0x14);/* 16K */
		/* Slimbus u1,u2 sample rate */
		hi64xx_hifi_write_reg(HI6403ES_SLIM_CTRL_3, 0x44);/* 48K */
	}

	OUT_FUNCTION;
}

/* dsp_if bypass config bit 6,7 */
static unsigned int hi6403es_sc_src_lr_ctrls_m[] = {
	HI6403ES_SC_S1_SRC_LR_CTRL_M,
	HI6403ES_SC_S2_SRC_LR_CTRL_M,
	HI6403ES_SC_S3_SRC_LR_CTRL_M,
	HI6403ES_SC_S4_SRC_LR_CTRL_M,
	HI6403ES_SC_CODEC_MUX_SEL3_1,
};

static void hi6403es_dsp_if_set_bypass(unsigned int dsp_if_id, bool enable)
{
	unsigned int addr = 0;
	unsigned int bit = 0;

	unsigned int i2s_id = dsp_if_id / 2;
	unsigned int direct =
		(dsp_if_id & 0x1) ? HI6402_HIFI_PCM_OUT : HI6402_HIFI_PCM_IN;

	IN_FUNCTION;

	BUG_ON(i2s_id >= ARRAY_SIZE(hi6403es_sc_src_lr_ctrls_m));
	addr = hi6403es_sc_src_lr_ctrls_m[i2s_id];

	if(HI6402_HIFI_DSP_IF_PORT_8 > dsp_if_id) {
		bit = (direct == HI6402_HIFI_PCM_IN) ? 6 : 7;
		if (enable) {
			hi64xx_hifi_reg_set_bit(addr, bit);
		} else {
			hi64xx_hifi_reg_clr_bit(addr, bit);
		}
	} else if (HI6402_HIFI_DSP_IF_PORT_8 == dsp_if_id){
		if (enable) {
			hi64xx_hifi_reg_set_bit(addr, 7);
			hi64xx_hifi_reg_set_bit(addr, 3);
		} else {
			hi64xx_hifi_reg_clr_bit(addr, 7);
			hi64xx_hifi_reg_clr_bit(addr, 3);
		}
	}

	OUT_FUNCTION;
}

void hi6403es_mad_enable(void)
{
	/* mad_buffer ->en */
	hi64xx_hifi_reg_set_bit(HI6403ES_MAD_BUFFER_CTRL1, HI6403ES_MAD_BUF_EN_BIT);
	/* mad_vad_ao ->en */
	hi64xx_hifi_reg_set_bit(HI6403ES_MAD_CTRL, HI6403ES_MAD_VAD_AO_BIT);
	/* mad irq ->en */
	hi64xx_hifi_reg_set_bit(HI6403ES_MAD_CTRL, HI6403ES_MAD_INT_EN_BIT);
	/* mad ->en */
	hi64xx_hifi_reg_set_bit(HI6403ES_MAD_CTRL, HI6403ES_MAD_EN_BIT);
}

void hi6403es_mad_disable(void)
{
	/* mad_buffer ->dis */
	hi64xx_hifi_reg_clr_bit(HI6403ES_MAD_BUFFER_CTRL1, HI6403ES_MAD_BUF_EN_BIT);
	/* mad_vad_ao ->dis */
	hi64xx_hifi_reg_clr_bit(HI6403ES_MAD_CTRL, HI6403ES_MAD_VAD_AO_BIT);
	/* mad irq ->dis */
	hi64xx_hifi_reg_clr_bit(HI6403ES_MAD_CTRL, HI6403ES_MAD_INT_EN_BIT);
	/* mad ->dis */
	hi64xx_hifi_reg_clr_bit(HI6403ES_MAD_CTRL, HI6403ES_MAD_EN_BIT);
}

static void hi6403es_set_dsp_div(enum pll_state pll_state)
{
	switch(pll_state){
	case PLL_HIGH_FREQ:
		hi64xx_hifi_reg_clr_bit(HI6403ES_12M288_CLK_SEL_REG, HI6403ES_DSP_CLK_SW_BIT);
		hi64xx_hifi_reg_write_bits(HI6403ES_DSP_CLK_CFG,0x1,0xF);
		break;
	case PLL_LOW_FREQ:
		hi64xx_hifi_reg_set_bit(HI6403ES_12M288_CLK_SEL_REG, HI6403ES_DSP_CLK_SW_BIT);
		hi64xx_hifi_reg_write_bits(HI6403ES_DSP_CLK_CFG,0x3,0xF);
		break;
	default:
		break;
	}
	return;
}

int hi6403es_hifi_config_init(struct snd_soc_codec *codec,
				struct hi64xx_resmgr *resmgr,
				struct hi64xx_irq *irqmgr,
				enum bustype_select bus_sel)
{
	struct hi64xx_dsp_config dsp_config;
	struct hi64xx_hifi_img_dl_config dl_config;
	int ret = 0;

	if (!codec || !resmgr|| !irqmgr)
		return -EINVAL;

	HI64XX_DSP_INFO("%s++\n", __FUNCTION__);

	memset(&dsp_config, 0, sizeof(dsp_config));
	memset(&dl_config, 0, sizeof(dl_config));

	dsp_config.para_addr = HI6403ES_MLIBPARA_ADDR;
	dsp_config.msg_addr = HI6403ES_AP_TO_DSP_MSG_ADDR;
	dsp_config.rev_msg_addr = HI6403ES_DSP_TO_AP_MSG_ADDR;
	dsp_config.slimbus_load = false;
	dsp_config.codec_type = HI64XX_CODEC_TYPE_6403;
	dsp_config.cmd0_addr = HI6403ES_CMD0_ADDR;
	dsp_config.cmd1_addr = HI6403ES_CMD1_ADDR;
	dsp_config.cmd2_addr = HI6403ES_CMD2_ADDR;
	dsp_config.cmd3_addr = HI6403ES_CMD3_ADDR;
	dsp_config.wtd_irq_num = IRQ_WTD;
	dsp_config.vld_irq_num = IRQ_CMD_VALID;
	dsp_config.dump_ocram_addr = HI6403ES_DUMP_PANIC_STACK_ADDR;
	dsp_config.dump_ocram_size = HI6403ES_DUMP_PANIC_STACK_SIZE + HI6403ES_DUMP_CPUVIEW_SIZE;
	dsp_config.dump_log_addr = HI6403ES_SAVE_LOG_ADDR_ES;
	dsp_config.dump_log_size = HI6403ES_SAVE_LOG_SIZE_ES;
	dsp_config.ocram_start_addr = HI6403ES_OCRAM_BASE_ADDR;
	dsp_config.ocram_size = HI6403ES_OCRAM_SIZE;
	dsp_config.itcm_start_addr = HI6403ES_ITCM_BASE_ADDR;
	dsp_config.itcm_size = HI6403ES_ITCM_SIZE;
	dsp_config.dtcm_start_addr = HI6403ES_DTCM_BASE_ADDR;
	dsp_config.dtcm_size = HI6403ES_DTCM_SIZE;
	dsp_config.bus_sel = bus_sel;

	dsp_config.dsp_ops.init = hi6403es_hifi_init;
	dsp_config.dsp_ops.deinit = hi6403es_hifi_deinit;
	dsp_config.dsp_ops.clk_enable = hi6403es_hifi_clk_enable;
	dsp_config.dsp_ops.ram2axi = hi6403es_ram2axi_cfg_with_hifi;
	dsp_config.dsp_ops.runstall = hi6403es_hifi_runstall_cfg;
	dsp_config.dsp_ops.wtd_enable = hi6403es_hifi_watchdog_enable;
	dsp_config.dsp_ops.uart_enable = NULL;
	dsp_config.dsp_ops.notify_dsp = hi6403es_hifi_notify_dsp;
	dsp_config.dsp_ops.dsp_power_ctrl = hi6403es_dsp_power_ctrl;
	dsp_config.dsp_ops.soundtrigger_fasttrans_ctrl = hi6403es_soundtrigger_fasttrans_ctrl;
	dsp_config.dsp_ops.dsp_if_set_bypass = hi6403es_dsp_if_set_bypass;
	dsp_config.dsp_ops.mad_enable = hi6403es_mad_enable;
	dsp_config.dsp_ops.mad_disable = hi6403es_mad_disable;
	dsp_config.dsp_ops.set_dsp_div = hi6403es_set_dsp_div;
	dl_config.dspif_clk_en_addr = HI6403ES_DSP_I2S_DSPIF_CLK_EN;

	ret = hi64xx_hifi_misc_init(codec, resmgr, irqmgr, &dsp_config);

	ret += hi64xx_soundtrigger_init(CODEC_HI6403);

	ret += hi64xx_hifi_img_dl_init(irqmgr, &dl_config);

	ret += hi64xx_hifi_om_init(irqmgr);

	HI64XX_DSP_INFO("%s--\n", __FUNCTION__);

	return ret;
}
EXPORT_SYMBOL(hi6403es_hifi_config_init);

void hi6403es_hifi_config_deinit(void)
{
	hi64xx_hifi_misc_deinit();

	hi64xx_hifi_img_dl_deinit();

	hi64xx_hifi_om_deinit();
}
EXPORT_SYMBOL(hi6403es_hifi_config_deinit);

MODULE_DESCRIPTION("hi64xx hifi misc driver");
MODULE_LICENSE("GPL");
