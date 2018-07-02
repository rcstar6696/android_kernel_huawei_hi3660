/*
 * hi6403es.c -- ALSA SoC HI6403ES codec driver
 *
 * Copyright (c) 2015 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/regulator/consumer.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/wakelock.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <asm/io.h>
#include <linux/hisi/hi64xx/hi64xx_compat.h>
#include <linux/debugfs.h>
#include <linux/hisi/hi64xx/hi64xx_utils.h>
#include <linux/hisi/hi64xx/hi64xx_resmgr.h>
#include <linux/hisi/hi64xx/hi64xx_vad.h>

#include <linux/hisi/hi64xx/hi6403es_regs.h>
#include "huawei_platform/audio/anc_hs_interface.h"
#include "../../../drivers/hisi/slimbus/slimbus.h"
#include "../../../drivers/hisi/hi64xx_dsp/hi6403es_hifi_config.h"
#include <linux/hisi/hilog.h>

/*lint -e750 -e730 -e835 -e715 -e785 -e838 -e64*/
#define HI6403ES_FORMATS	( SNDRV_PCM_FMTBIT_S16_LE | \
			SNDRV_PCM_FMTBIT_S16_BE | \
			SNDRV_PCM_FMTBIT_S24_LE | \
			SNDRV_PCM_FMTBIT_S24_BE )

#define HI6403ES_RATES SNDRV_PCM_RATE_8000_192000

/* res chain num */
#define HI6403ES_IMP_RES_NUM          8
/* supersonic wave range 100mV */
#define HI6403ES_IMP_RANGE_VALUE      100
/* fake res value 1000omh */
#define HI6403ES_FAKE_RES_VALUE       1000
/* feedback res value 625omh */
#define HI6403ES_FB_RES_VALUE         625
/* current ratio 32 */
#define HI6403ES_CURRENT_RATIO_VALUE  32

/* HS 0.5dB PGA GAIN */
#define HI6403ES_HS_LOW_0_GAIN        0x3D    //-29.5dB
#define HI6403ES_HS_LOW_1_GAIN        0x47    //-24.5dB
#define HI6403ES_HS_HIGH_0_GAIN       0x6E    //-5dB
#define HI6403ES_HS_HIGH_1_GAIN       0x78    //0dB

#define FS_8000		0x0
#define FS_16000	0x1
#define FS_32000	0x2
#define FS_48000	0x4
#define FS_96000	0x5

typedef enum {
	TRACK_FREE = 0,
	TRACK_STARTUP = 1,
}track_state;

typedef enum {
	hp_classh_state  = 0x1,  /* hp high mode(classAB) = 0 low mode(classh) = 1 */
	rcv_classh_state = 0x2,  /* classh_rcv_hp_switch true = 1 false =0 */
	hp_power_state   = 0x4,  /* hp power on = 1 power off = 0 */
	rcv_power_state  = 0x8,  /* rcv power on = 1 power off = 0 */
}classh_state;

struct hi6403es_board_cfg {
	/* board defination */
	int mic_num;
	bool use_stereo_smartpa;
	bool classh_rcv_hp_switch;
	bool hp_high_low_change_enable;
	bool hp_res_detect_enable;
	bool gpio_pd_enable;
};

/* codec private data */
struct hi6403es_platform_data {
	struct snd_soc_codec *codec;
	struct device_node *node;
	struct hi_cdc_ctrl *cdc_ctrl;
	struct hi64xx_resmgr *resmgr;
	struct hi64xx_irq *irqmgr;
	struct hi64xx_mbhc *mbhc;
	struct hi64xx_mbhc_config mbhc_config;
	struct hi6403es_board_cfg board_config;
	struct mutex rw_mutex;
	struct mutex dapm_mutex;
	struct mutex impdet_dapm_mutex;
	unsigned int mm_reg;
	unsigned int res_value;
	unsigned int hs_high_pga_gain;
	unsigned int hs_low_pga_gain;
	slimbus_track_param_t voice_params;
	slimbus_track_param_t capture_params;
	slimbus_track_param_t play_params;
	track_state voiceup_state;
	track_state audioup_4mic_state;
	int dp_clk_num;
	int cp1_num;
	int cp2_num;
	bool hsr_power_on;
	bool hsl_power_on;
	classh_state rcv_hp_classh_state;
};

struct snd_soc_codec *g_hi6403es_codec = NULL;
#define HI6403ES_IRQ_NUM (48)

static const struct of_device_id hi6403es_platform_match[] = {
	{ .compatible = "hisilicon,hi6403es-codec", },
	{},
};
MODULE_DEVICE_TABLE(of, hi6403es_codec_match);

/* VOLUME CONTROLS */
/*
* MAIN MIC GAIN volume control:
* from 0 to 36 dB in 2 dB steps
* MAX VALUE is 18
*/
static DECLARE_TLV_DB_SCALE(main_mic_tlv, 0, 200, 0);

/*
* AUX MIC GAIN volume control:
* from 0 to 36 dB in 2 dB steps
* MAX VALUE is 18
*/
static DECLARE_TLV_DB_SCALE(aux_mic_tlv, 0, 200, 0);

/*
* LINEINR MIC GAIN volume control:
* from -20 to 36 dB in 2 dB steps
* MAX VALUE is 18
*/
static DECLARE_TLV_DB_SCALE(lineinr_mic_tlv, -2000, 200, 0);

/*
* LINEINL MIC GAIN volume control:
* from -20 to 36 dB in 2 dB steps
* MAX VALUE is 18
*/
static DECLARE_TLV_DB_SCALE(lineinl_mic_tlv, -2000, 200, 0);

/*
* LOL PGA GAIN volume control:
* from -21 to 6 dB in 1.5 dB steps
* MAX VALUE is 18
*/
static DECLARE_TLV_DB_SCALE(lol_pga_tlv, -2100, 150, 0);

/*
* LOR PGA GAIN volume control:
* from -21 to 6 dB in 1.5 dB steps
* MAX VALUE is 18
*/
static DECLARE_TLV_DB_SCALE(lor_pga_tlv, -2100, 150, 0);

/*
* EP PGA GAIN volume control:
* from -21 to 6 dB in 1.5 dB steps
* MAX VALUE is 18
*/

static DECLARE_TLV_DB_SCALE(mad_tlv, 0, 200, 0);

int hi6403es_s2_rx_power_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		hi64xx_update_bits(codec, HI6403ES_S2_CFG_REG,
				1<<HI6403ES_S2_RX_EN_BIT, 1<<HI6403ES_S2_RX_EN_BIT);
		break;
	case SND_SOC_DAPM_POST_PMD:
		hi64xx_update_bits(codec, HI6403ES_S2_CFG_REG,
				1<<HI6403ES_S2_RX_EN_BIT, 0);
		break;
	default :
		pr_warn("%s : power mode event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_s4_rx_power_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		hi64xx_update_bits(codec, HI6403ES_S4_CLK_CFG_REG,
				1<<HI6403ES_S4_CLKEN_BIT, 1<<HI6403ES_S4_CLKEN_BIT);
		hi64xx_update_bits(codec, HI6403ES_S4_CFG_REG,
				1<<HI6403ES_S4_RX_EN_BIT, 1<<HI6403ES_S4_RX_EN_BIT);
		break;
	case SND_SOC_DAPM_POST_PMD:
		hi64xx_update_bits(codec, HI6403ES_S4_CLK_CFG_REG,
				1<<HI6403ES_S4_CLKEN_BIT, 0);
		hi64xx_update_bits(codec, HI6403ES_S4_CFG_REG,
				1<<HI6403ES_S4_RX_EN_BIT, 0);
		break;
	default :
		pr_warn("%s : power mode event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_mad_pll_power_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct hi6403es_platform_data *data = NULL;
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	BUG_ON(NULL == codec);

	data = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == data);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		hi64xx_resmgr_request_pll(data->resmgr, PLL_LOW);
		break;
	case SND_SOC_DAPM_POST_PMD:
		hi64xx_resmgr_release_pll(data->resmgr, PLL_LOW);
		break;
	default :
		pr_warn("%s : power mode event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

static void hi6403es_request_dp_clk(struct snd_soc_codec *codec, bool enable)
{
	struct hi6403es_platform_data *data = NULL;
	BUG_ON(NULL == codec);
	data = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == data);

	mutex_lock(&data->impdet_dapm_mutex);
	if (enable) {
		data->dp_clk_num++;
		if (1 == data->dp_clk_num) {
			hi64xx_update_bits(codec, HI6403ES_CODEC_DPCLK_REG,
						0x1<<HI6403ES_CODEC_DPCLK_BIT, 0x1<<HI6403ES_CODEC_DPCLK_BIT);
		}
	} else {
		if (0 == data->dp_clk_num) {
			pr_err("%s : dp clk num err!\n", __FUNCTION__);
			mutex_unlock(&data->impdet_dapm_mutex);
			return;
		}
		data->dp_clk_num--;
		if (0 == data->dp_clk_num) {
			hi64xx_update_bits(codec, HI6403ES_CODEC_DPCLK_REG,
						0x1<<HI6403ES_CODEC_DPCLK_BIT, 0x0<<HI6403ES_CODEC_DPCLK_BIT);
		}
	}
	mutex_unlock(&data->impdet_dapm_mutex);

	return;
}

static void hi6403es_request_cp1(struct snd_soc_codec *codec, bool enable)
{
	struct hi6403es_platform_data *data = NULL;
	BUG_ON(NULL == codec);
	data = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == data);

	mutex_lock(&data->impdet_dapm_mutex);
	if (enable) {
		data->cp1_num++;
		if (1 == data->cp1_num) {
			/* buck1 enable */
			hi64xx_update_bits(codec, HI6403ES_ANALOG_REG129,
					1<<HI6403ES_BUCK1_ENP_BIT, 1<<HI6403ES_BUCK1_ENP_BIT);
			/* classH enable */
			hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG05,
					1<<HI6403ES_CLASSH_CLKEN_BIT, 1<<HI6403ES_CLASSH_CLKEN_BIT);
		}
	} else {
		if (0 == data->cp1_num) {
			pr_err("%s : cp1 num err!\n", __FUNCTION__);
			mutex_unlock(&data->impdet_dapm_mutex);
			return;
		}
		data->cp1_num--;
		if (0 == data->cp1_num) {
			/* classH disable */
			hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG05,
					1<<HI6403ES_CLASSH_CLKEN_BIT, 0);
			/* buck1 disable */
			hi64xx_update_bits(codec, HI6403ES_ANALOG_REG129,
					1<<HI6403ES_BUCK1_ENP_BIT,0);
		}
	}
	mutex_unlock(&data->impdet_dapm_mutex);

	return;
}

static void hi6403es_request_cp2(struct snd_soc_codec *codec, bool enable)
{
	struct hi6403es_platform_data *data = NULL;
	BUG_ON(NULL == codec);
	data = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == data);

	mutex_lock(&data->impdet_dapm_mutex);
	if (enable) {
		data->cp2_num++;
		if (1 == data->cp2_num) {
			/* buck2 enable */
			hi64xx_update_bits(codec, HI6403ES_ANALOG_REG129,
					1<<HI6403ES_BUCK2_ENP_BIT, 1<<HI6403ES_BUCK2_ENP_BIT);
		}
	} else {
		if (0 == data->cp2_num) {
			pr_err("%s : cp2 num err!\n", __FUNCTION__);
			mutex_unlock(&data->impdet_dapm_mutex);
			return;
		}
		data->cp2_num--;
		if (0 == data->cp2_num) {
			/* buck2 disable */
			hi64xx_update_bits(codec, HI6403ES_ANALOG_REG129,
					1<<HI6403ES_BUCK2_ENP_BIT,0);
		}
	}
	mutex_unlock(&data->impdet_dapm_mutex);

	return;
}

int hi6403es_dp_clk_power_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	pr_info("%s : power mode event: 0x%x\n", __FUNCTION__, event);

	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		hi6403es_request_dp_clk(codec, true);
		break;
	case SND_SOC_DAPM_POST_PMD:
		hi6403es_request_dp_clk(codec, false);
		break;
	default :
		pr_warn("%s : power mode event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_pll_power_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct hi6403es_platform_data *priv = NULL;
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	BUG_ON(NULL == codec);

	priv = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == priv);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		hi64xx_resmgr_request_pll(priv->resmgr, PLL_HIGH);
		if (!priv->cdc_ctrl->pm_runtime_support) {
			/* slimbus master framer is hi6403 */
			mdelay(2);
			slimbus_switch_framer(SLIMBUS_DEVICE_HI6403, SLIMBUS_FRAMER_CODEC);
		}
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (!priv->cdc_ctrl->pm_runtime_support) {
			/* slimbus master framer is soc */
			slimbus_switch_framer(SLIMBUS_DEVICE_HI6403, SLIMBUS_FRAMER_SOC);
			mdelay(2);
		}
		hi64xx_resmgr_release_pll(priv->resmgr, PLL_HIGH);
		break;
	default :
		dev_warn(codec->dev, "%s : power mode event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}


int hi6403es_adc1r_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG09,
			1<<HI6403ES_ADC1R_CLKEN_BIT, 1<<HI6403ES_ADC1R_CLKEN_BIT);
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG07,
			1<<HI6403ES_ADC1R_PGA_CLKEN_BIT, 1<<HI6403ES_ADC1R_PGA_CLKEN_BIT);
		break;
	case SND_SOC_DAPM_POST_PMD:
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG07,
			1<<HI6403ES_ADC1R_PGA_CLKEN_BIT, 0);
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG09,
			1<<HI6403ES_ADC1R_CLKEN_BIT, 0);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_adc1l_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG09,
			1<<HI6403ES_ADC1L_CLKEN_BIT, 1<<HI6403ES_ADC1L_CLKEN_BIT);
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG07,
			1<<HI6403ES_ADC1L_PGA_CLKEN_BIT, 1<<HI6403ES_ADC1L_PGA_CLKEN_BIT);
		break;
	case SND_SOC_DAPM_POST_PMD:
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG07,
			1<<HI6403ES_ADC1L_PGA_CLKEN_BIT, 0);
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG09,
			1<<HI6403ES_ADC1L_CLKEN_BIT, 0);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_adc0r_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG09,
			1<<HI6403ES_ADC0R_CLKEN_BIT, 1<<HI6403ES_ADC0R_CLKEN_BIT);
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG06,
			1<<HI6403ES_ADC0R_PGA_CLKEN_BIT, 1<<HI6403ES_ADC0R_PGA_CLKEN_BIT);
		break;
	case SND_SOC_DAPM_POST_PMD:
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG06,
			1<<HI6403ES_ADC0R_PGA_CLKEN_BIT, 0);
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG09,
			1<<HI6403ES_ADC0R_CLKEN_BIT, 0);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_adc0l_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG09,
			1<<HI6403ES_ADC0L_CLKEN_BIT, 1<<HI6403ES_ADC0L_CLKEN_BIT);
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG06,
			1<<HI6403ES_ADC0L_PGA_CLKEN_BIT, 1<<HI6403ES_ADC0L_PGA_CLKEN_BIT);
		break;
	case SND_SOC_DAPM_POST_PMD:
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG06,
			1<<HI6403ES_ADC0L_PGA_CLKEN_BIT, 0);
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG09,
			1<<HI6403ES_ADC0L_CLKEN_BIT, 0);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_ec_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	int ret = 0;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		ret = slimbus_track_activate(SLIMBUS_DEVICE_HI6403, SLIMBUS_TRACK_ECREF, NULL);
		break;
	case SND_SOC_DAPM_POST_PMD:
		ret = slimbus_track_deactivate(SLIMBUS_DEVICE_HI6403, SLIMBUS_TRACK_ECREF, NULL);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return ret;
}

int hi6403es_cp1_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		hi6403es_request_cp1(codec, true);
		break;
	case SND_SOC_DAPM_POST_PMD:
		hi6403es_request_cp1(codec, false);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_cp2_power_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		hi6403es_request_cp2(codec, true);
		msleep(5);
		break;
	case SND_SOC_DAPM_POST_PMD:
		hi6403es_request_cp2(codec, false);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

static void hi6403es_classH_headphone_select(struct snd_soc_codec *codec)
{
	/* SRC up16 select dacls pga dout */
	hi64xx_update_bits(codec, HI6403ES_CLASSH_HP_EP_CTRL,
			0X1<<HI6403ES_DACL_S_UP16_DIN_SEL_BIT, 0<<HI6403ES_DACL_S_UP16_DIN_SEL_BIT);
	/* select classH path for headphone */
	hi64xx_update_bits(codec, HI6403ES_CLASSH_HP_EP_CTRL,
			0X3<<HI6403ES_CLASSH_DIN_L_SEL_BIT, 0X0<<HI6403ES_CLASSH_DIN_L_SEL_BIT);
	hi64xx_update_bits(codec, HI6403ES_CLASSH_HP_EP_CTRL,
			0X3<<HI6403ES_CLASSH_DIN_R_SEL_BIT, 0X0<<HI6403ES_CLASSH_DIN_R_SEL_BIT);
	/* select LUT1 */
	hi64xx_update_bits(codec, HI6403ES_CLASSH_CTRL12,
			0X1<<HI6403ES_CLASSH_HP_EP_SW_BIT, 0X1<<HI6403ES_CLASSH_HP_EP_SW_BIT);
}

static void hi6403es_classH_earphone_select(struct snd_soc_codec *codec)
{
	/* SRC up16 select classH dout L */
	hi64xx_update_bits(codec, HI6403ES_CLASSH_HP_EP_CTRL,
			0X1<<HI6403ES_DACL_S_UP16_DIN_SEL_BIT, 0X1<<HI6403ES_DACL_S_UP16_DIN_SEL_BIT);
	/* select classH path for earphone */
	hi64xx_update_bits(codec, HI6403ES_CLASSH_HP_EP_CTRL,
			0X3<<HI6403ES_CLASSH_DIN_L_SEL_BIT, 0X2<<HI6403ES_CLASSH_DIN_L_SEL_BIT);
	/* select LUT1B */
	hi64xx_update_bits(codec, HI6403ES_CLASSH_CTRL12,
			0X1<<HI6403ES_CLASSH_HP_EP_SW_BIT, 0X0<<HI6403ES_CLASSH_HP_EP_SW_BIT);
}

static void set_classh_config(struct snd_soc_codec *codec, classh_state classh_state_cfg)
{
	char state = classh_state_cfg & 0x0F;

	pr_info("%s ClassH state is %d\n", __FUNCTION__, state);

	switch(state) {
		/* headphone off & earphone off */
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
			break;

		/* headphone on(classH) & earphone off */
		case 0x5:
		case 0x7:
			/* classA/B -> classH */
			hi64xx_update_bits(codec, HI6403ES_CLASS_H_CFG_REG1,
					0x1<<HI6403ES_CLASS_H_SEL_BIT, 0x1<<HI6403ES_CLASS_H_SEL_BIT);
			/* select classH path for headphone */
			hi6403es_classH_headphone_select(codec);
			break;

		/* headphone on(classAB) & earphone off */
		case 0x4:
		case 0x6:
		/* headphone off & earphone on(classAB) */
		case 0x8:
		case 0x9:
			/* classH -> classA/B */
			hi64xx_update_bits(codec, HI6403ES_CLASS_H_CFG_REG1,
					0x1<<HI6403ES_CLASS_H_SEL_BIT, 0x0<<HI6403ES_CLASS_H_SEL_BIT);
			/* select classH path for headphone */
			hi6403es_classH_headphone_select(codec);
			break;

		/* headphone off & earphone on(classH) */
		case 0xa:
		case 0xb:
			/* classA/B -> classH */
			hi64xx_update_bits(codec, HI6403ES_CLASS_H_CFG_REG1,
					0x1<<HI6403ES_CLASS_H_SEL_BIT, 0x1<<HI6403ES_CLASS_H_SEL_BIT);
			/* select classH path for earphone */
			hi6403es_classH_earphone_select(codec);
			break;

		/* headphone on & earphone on */
		case 0xc:
		case 0xd:
		case 0xe:
		case 0xf:
			/* classH -> classA/B */
			hi64xx_update_bits(codec, HI6403ES_CLASS_H_CFG_REG1,
					0x1<<HI6403ES_CLASS_H_SEL_BIT, 0x0<<HI6403ES_CLASS_H_SEL_BIT);
			/* select classH path for headphone */
			hi6403es_classH_headphone_select(codec);
			break;

		default:
			pr_warn("%s : power mode event err : %d\n", __FUNCTION__, state);
			break;
	}
}

int hi6403es_classh_rcv_hp_switch(bool enable)
{
	struct hi6403es_platform_data *priv = NULL;

	priv = snd_soc_codec_get_drvdata(g_hi6403es_codec);

	BUG_ON(NULL == priv);

	if (enable) {
		priv->rcv_hp_classh_state |= rcv_classh_state;
	} else {
		priv->rcv_hp_classh_state &= ~rcv_classh_state;
	}
	return 0;
}
EXPORT_SYMBOL(hi6403es_classh_rcv_hp_switch);

static void hi6403es_headphone_pop_on(struct snd_soc_codec *codec)
{
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);

	BUG_ON(NULL == priv);
	/* headphone POP_ON cfg */
	snd_soc_write(codec, HI6403ES_ANALOG_REG52, 0x13);

	/* headphone dac mute disable */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG2,
			0x3<<HI6403ES_DAC_HPL_MUTE_BIT, 0);
	/* pop auto enable */
	//hi64xx_update_bits(codec, HI6403ES_ANALOG_REG47,
	//		1<<HI6403ES_HP_POP_AUTO_EN_BIT, 1<<HI6403ES_HP_POP_AUTO_EN_BIT);
	/* headphone dac vbias power on */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG2,
			1<<HI6403ES_DAC_HP_VB_PB_BIT, 0);
	/* headphone dac sec vbias power on */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG2,
			1<<HI6403ES_DAC_HP_VB_SEC_PB_BIT, 0);
	//msleep(20);
	/* headphone dac enable */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG2,
			0x3<<HI6403ES_DAC_HPL_PD_BIT, 0);
	/* headphone ibias on */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG82,
			1<<HI6403ES_HP_IBIAS_EN_BIT, 1<<HI6403ES_HP_IBIAS_EN_BIT);
	/* headphone gen power on */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG2,
			1<<HI6403ES_HP_GEN_PD_BIT, 0);
	/* pop clk power on */
	//hi64xx_update_bits(codec, HI6403ES_ANALOG_REG15,
	//		1<<HI6403ES_POP_CLK_PD_BIT, 0);
	/* pop clk rst */
	//hi64xx_update_bits(codec, HI6403ES_ANALOG_REG47,
	//		1<<HI6403ES_HP_POP_RST_BIT, 0);
	/* headphone feed back power on */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG3,
			0x3<<HI6403ES_HPR_FD_PD_BIT, 0);
	udelay(200);

	/* headphone power on */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG3,
			0x3<<HI6403ES_HPR_PD_BIT, 0);
	msleep(120);
	/* headphone pop pull set 0 */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG43,
			0x3F<<HI6403ES_HPL_POP_PULL_2_0_BIT, 0);
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG44,
			0x3F<<HI6403ES_HPR_POP_PULL_2_0_BIT, 0);
	snd_soc_write(codec, HI6403ES_DACL_PGA_GAIN_CFG_REG,priv->hs_high_pga_gain);
	snd_soc_write(codec, HI6403ES_DACR_PGA_GAIN_CFG_REG,priv->hs_high_pga_gain);
}

static void hi6403es_headphone_pop_off(struct snd_soc_codec *codec)
{
	snd_soc_write(codec, HI6403ES_DACL_PGA_GAIN_CFG_REG, 0xBA);
	snd_soc_write(codec, HI6403ES_DACR_PGA_GAIN_CFG_REG, 0xBA);
	msleep(85);

	/* headphone POP_OFF cfg */
	snd_soc_write(codec, HI6403ES_ANALOG_REG52, 0x1A);
	/* pop auto enable */
	//hi64xx_update_bits(codec, HI6403ES_ANALOG_REG47,
	//		1<<HI6403ES_HP_POP_AUTO_EN_BIT, 1<<HI6403ES_HP_POP_AUTO_EN_BIT);
	/* headphone pop pull set not 0 */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG43,
			0x3F<<HI6403ES_HPL_POP_PULL_2_0_BIT, 0x8<<HI6403ES_HPL_POP_PULL_2_0_BIT);
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG44,
			0x3F<<HI6403ES_HPR_POP_PULL_2_0_BIT, 0x8<<HI6403ES_HPR_POP_PULL_2_0_BIT);
	/* headphone power down */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG3,
			0x3<<HI6403ES_HPR_PD_BIT, 0x3<<HI6403ES_HPR_PD_BIT);
	msleep(50);
	/* headphone feed back power down */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG3,
			0x3<<HI6403ES_HPR_FD_PD_BIT, 0x3<<HI6403ES_HPR_FD_PD_BIT);
	udelay(100);
	/* pop clk rst */
	//hi64xx_update_bits(codec, HI6403ES_ANALOG_REG47,
	//		1<<HI6403ES_HP_POP_RST_BIT, 0);
	/* pop clk power down */
	//hi64xx_update_bits(codec, HI6403ES_ANALOG_REG15,
	//		1<<HI6403ES_POP_CLK_PD_BIT, 1<<HI6403ES_POP_CLK_PD_BIT);
	/* headphone ibias off */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG82,
			1<<HI6403ES_HP_IBIAS_EN_BIT, 0);
	/* headphone gen power down */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG2,
			1<<HI6403ES_HP_GEN_PD_BIT, 1<<HI6403ES_HP_GEN_PD_BIT);
	/* headphone dac disable */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG2,
			0x3<<HI6403ES_DAC_HPL_PD_BIT, 0x3<<HI6403ES_DAC_HPL_PD_BIT);

	/* headphone dac vbias power down */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG2,
			1<<HI6403ES_DAC_HP_VB_PB_BIT, 1<<HI6403ES_DAC_HP_VB_PB_BIT);
	/* headphone dac sec vbias power on */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG2,
			1<<HI6403ES_DAC_HP_VB_SEC_PB_BIT, 1<<HI6403ES_DAC_HP_VB_SEC_PB_BIT);
	/* headphone dac mute enable */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG2,
			0x3<<HI6403ES_DAC_HPL_MUTE_BIT, 0x3<<HI6403ES_DAC_HPL_MUTE_BIT);
}

static void hi6403es_headphone_on(struct snd_soc_codec *codec)
{
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);

	BUG_ON(NULL == priv);

	priv->rcv_hp_classh_state |= hp_power_state;
	set_classh_config(codec, priv->rcv_hp_classh_state);

	hi6403es_headphone_pop_on(codec);
}

static void hi6403es_headphone_off(struct snd_soc_codec *codec)
{
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);

	BUG_ON(NULL == priv);

	hi6403es_headphone_pop_off(codec);

	priv->rcv_hp_classh_state &= ~hp_power_state;
	set_classh_config(codec, priv->rcv_hp_classh_state);
}

void hi6403es_headphone_power_param_pass(struct hi6403es_platform_data *data,
					struct snd_soc_codec *codec, bool *hs_pwron_flag, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		*hs_pwron_flag = true;
		if (data->hsl_power_on && data->hsr_power_on)
			hi6403es_headphone_on(codec);
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (data->hsl_power_on && data->hsr_power_on)
			hi6403es_headphone_off(codec);
		*hs_pwron_flag = false;
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return;
}

int hi6403es_headphonel_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct hi6403es_platform_data *data = NULL;
	BUG_ON(NULL == codec);
	data = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == data);

	hi6403es_headphone_power_param_pass(data, codec, &data->hsl_power_on, event);

	return 0;
}

int hi6403es_headphoner_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct hi6403es_platform_data *data = NULL;
	BUG_ON(NULL == codec);
	data = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == data);

	hi6403es_headphone_power_param_pass(data, codec, &data->hsr_power_on, event);

	return 0;
}

int hi6403es_earphone_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	struct hi6403es_platform_data *priv = NULL;

	BUG_ON(NULL == codec);

	priv = snd_soc_codec_get_drvdata(codec);

	BUG_ON(NULL == priv);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		priv->rcv_hp_classh_state |= rcv_power_state;
		set_classh_config(codec, priv->rcv_hp_classh_state);

		/* lineoutl pga on bug in hi6403es */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG4, 1<<HI6403ES_LOL_PD_BIT, 0);
		/* earphone dac on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG4, 1<<HI6403ES_DAC_EAR_PD_BIT, 0);
		/* earphone pga on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG4, 1<<HI6403ES_EAR_PD_BIT, 0);
		/* earphone mute off */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG62, 1<<HI6403ES_EAR_MUTE_BIT, 0);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* earphone mute on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG62, 1<<HI6403ES_EAR_MUTE_BIT,1<<HI6403ES_EAR_MUTE_BIT);
		/* earphone pga on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG4, 1<<HI6403ES_EAR_PD_BIT, 1<<HI6403ES_EAR_PD_BIT);
		/* earphone dac pb */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG4, 1<<HI6403ES_DAC_EAR_PD_BIT, 1<<HI6403ES_DAC_EAR_PD_BIT);
		/* lineoutl pga off bug in hi6403es */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG4, 1<<HI6403ES_LOL_PD_BIT, 1<<HI6403ES_LOL_PD_BIT);

		priv->rcv_hp_classh_state &= ~rcv_power_state;
		set_classh_config(codec, priv->rcv_hp_classh_state);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_lineoutl_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* lineoutl dac on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG4, 1<<HI6403ES_DAC_LOL_PD_BIT, 0);
		/* lineoutl pga on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG4, 1<<HI6403ES_LOL_PD_BIT, 0);
		/* lineoutl mute off */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG63, 1<<HI6403ES_LOL_MUTE_BIT, 0);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* lineoutl mute on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG63, 1<<HI6403ES_LOL_MUTE_BIT, 1<<HI6403ES_LOL_MUTE_BIT);
		/* lineoutl pga off */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG4, 1<<HI6403ES_LOL_PD_BIT, 1<<HI6403ES_LOL_PD_BIT);
		/* lineoutl dac off */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG4, 1<<HI6403ES_DAC_LOL_PD_BIT, 1<<HI6403ES_DAC_LOL_PD_BIT);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_lineoutr_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* lineoutr dac on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG4, 1<<HI6403ES_DAC_LOR_PD_BIT, 0);
		/* lineoutr pga on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG4, 1<<HI6403ES_LOL_PD_BIT, 0);
		/* lineoutr mute off */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG64, 1<<HI6403ES_LOR_MUTE_BIT, 0);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* lineoutr mute off */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG64, 1<<HI6403ES_LOR_MUTE_BIT, 1<<HI6403ES_LOR_MUTE_BIT);
		/* lineoutr pga on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG4, 1<<HI6403ES_LOL_PD_BIT, 1<<HI6403ES_LOL_PD_BIT);
		/* lineoutr dac pb */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG4, 1<<HI6403ES_DAC_LOR_PD_BIT, 1<<HI6403ES_DAC_LOR_PD_BIT);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_mad_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* mad pga on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG83, 1<<HI6403ES_MAD_PGA_PD_BIT, 0);
		/* mad pga mute1 */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG84, 1<<HI6403ES_MAD_MUTE1_BIT, 0);
		/* mad pga mute2 */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG84, 1<<HI6403ES_MAD_MUTE2_BIT, 0);
		/* mad adc on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG83, 1<<HI6403ES_MAD_ADC_PD_BIT, 0);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* mad adc off */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG83, 1<<HI6403ES_MAD_ADC_PD_BIT, 1<<HI6403ES_MAD_ADC_PD_BIT);
		/* mad pga mute2 */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG84, 1<<HI6403ES_MAD_MUTE2_BIT, 1<<HI6403ES_MAD_MUTE2_BIT);
		/* mad pga mute1 */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG84, 1<<HI6403ES_MAD_MUTE1_BIT, 1<<HI6403ES_MAD_MUTE1_BIT);
		/* mad pga pd */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG83, 1<<HI6403ES_MAD_PGA_PD_BIT, 1<<HI6403ES_MAD_PGA_PD_BIT);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_main_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* main pga on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG1, 1<<HI6403ES_MAIN_PGA_PD_BIT, 0);
		/* main pga mute1 */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG24, 1<<HI6403ES_MAIN_MUTE1_BIT, 0);
		/* main pga mute2 */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG23, 1<<HI6403ES_MAIN_MUTE2_BIT, 0);
		/* adc0l on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG1, 1<<HI6403ES_ADC0L_PD_BIT, 0);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* adc0l off */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG1, 1<<HI6403ES_ADC0L_PD_BIT, 1<<HI6403ES_ADC0L_PD_BIT);
		/* main pga mute2 */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG23, 1<<HI6403ES_MAIN_MUTE2_BIT, 1<<HI6403ES_MAIN_MUTE2_BIT);
		/* main pga mute1 */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG24, 1<<HI6403ES_MAIN_MUTE1_BIT, 1<<HI6403ES_MAIN_MUTE1_BIT);
		/* main pga pd */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG1, 1<<HI6403ES_MAIN_PGA_PD_BIT, 1<<HI6403ES_MAIN_PGA_PD_BIT);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_aux_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* aux pga on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG1, 1<<HI6403ES_AUX_PGA_PD_BIT, 0);
		/* aux pga mute1 */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG22, 1<<HI6403ES_AUX_MUTE1_BIT, 0);
		/* aux pga mute2 */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG22, 1<<HI6403ES_AUX_MUTE2_BIT, 0);
		/* adc0r on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG1, 1<<HI6403ES_ADC0R_PD_BIT, 0);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* adc0r pd */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG1, 1<<HI6403ES_ADC0R_PD_BIT, 1<<HI6403ES_ADC0R_PD_BIT);
		/* aux pga mute2 */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG22, 1<<HI6403ES_AUX_MUTE2_BIT, 1<<HI6403ES_AUX_MUTE2_BIT);
		/* aux pga mute1 */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG22, 1<<HI6403ES_AUX_MUTE1_BIT, 1<<HI6403ES_AUX_MUTE1_BIT);
		/* aux pga pd */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG1, 1<<HI6403ES_AUX_PGA_PD_BIT, 1<<HI6403ES_AUX_PGA_PD_BIT);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_lineinl_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* lineinl pga on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG1, 1<<HI6403ES_LINEINL_PGA_PD_BIT, 0);
		/* lineinl mute1 */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG24, 1<<HI6403ES_LINEINL_MUTE1_BIT, 0);
		/* lineinl mute2 */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG20, 1<<HI6403ES_LINEINL_MUTE2_BIT, 0);
		/* adc1l on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG1, 1<<HI6403ES_ADC1L_PD_BIT, 0);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* adc1l pd */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG1, 1<<HI6403ES_ADC1L_PD_BIT, 1<<HI6403ES_ADC1L_PD_BIT);
		/* lineinl mute2 */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG20, 1<<HI6403ES_LINEINL_MUTE2_BIT, 1<<HI6403ES_LINEINL_MUTE2_BIT);
		/* lineinl mute1 */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG24, 1<<HI6403ES_LINEINL_MUTE1_BIT, 1<<HI6403ES_LINEINL_MUTE1_BIT);
		/* lineinl pga pd */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG1, 1<<HI6403ES_LINEINL_PGA_PD_BIT, 1<<HI6403ES_LINEINL_PGA_PD_BIT);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_lineinr_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* lineinr pga on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG1, 1<<HI6403ES_LINEINR_PGA_PD_BIT, 0);
		/* lineinr mute1 */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG24, 1<<HI6403ES_LINEINR_MUTE1_BIT, 0);
		/* lineinr mute2 */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG21, 1<<HI6403ES_LINEINR_MUTE2_BIT, 0);
		/* adc1r on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG1, 1<<HI6403ES_ADC1R_PD_BIT, 0);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* adc1r pd */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG1, 1<<HI6403ES_ADC1R_PD_BIT, 1<<HI6403ES_ADC1R_PD_BIT);
		/* lineinr mute2 */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG21, 1<<HI6403ES_LINEINR_MUTE2_BIT, 1<<HI6403ES_LINEINR_MUTE2_BIT);
		/* lineinr mute1 */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG24, 1<<HI6403ES_LINEINR_MUTE1_BIT, 1<<HI6403ES_LINEINR_MUTE1_BIT);
		/* lineinr pga pd */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG1, 1<<HI6403ES_LINEINR_PGA_PD_BIT, 1<<HI6403ES_LINEINR_PGA_PD_BIT);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_dacl_flt_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* dacl src clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG07, 1<<HI6403ES_DACL_SRC_CLKEN_BIT, 1<<HI6403ES_DACL_SRC_CLKEN_BIT);
		/* dacl clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG09, 1<<HI6403ES_DACL_CLKEN_BIT, 1<<HI6403ES_DACL_CLKEN_BIT);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* dacl clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG09, 1<<HI6403ES_DACL_CLKEN_BIT, 0);
		/* dacl src clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG07, 1<<HI6403ES_DACL_SRC_CLKEN_BIT, 0);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_dacr_flt_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* dacr src clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG07, 1<<HI6403ES_DACR_SRC_CLKEN_BIT, 1<<HI6403ES_DACR_SRC_CLKEN_BIT);
		/* dacr clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG09, 1<<HI6403ES_DACR_CLKEN_BIT, 1<<HI6403ES_DACR_CLKEN_BIT);

		break;
	case SND_SOC_DAPM_POST_PMD:
		/* dacr clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG09, 1<<HI6403ES_DACR_CLKEN_BIT, 0);
		/* dacrl src clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG07, 1<<HI6403ES_DACR_SRC_CLKEN_BIT, 0);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_dacl_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* dacl pga clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG10, 1<<HI6403ES_DACL_PGA_CLKEN_BIT, 1<<HI6403ES_DACL_PGA_CLKEN_BIT);
		/* dacl src up16 clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG06, 1<<HI6403ES_DACL_UP16_CLKEN_BIT, 1<<HI6403ES_DACL_UP16_CLKEN_BIT);
		/* dacl sdm clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG08, 1<<HI6403ES_DACL_SDM_CLKEN_BIT, 1<<HI6403ES_DACL_SDM_CLKEN_BIT);
		/* dsdl sdm clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG08, 1<<HI6403ES_DSDL_SDM_CLKEN_BIT, 1<<HI6403ES_DSDL_SDM_CLKEN_BIT);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* dacl sdm clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG08, 1<<HI6403ES_DACL_SDM_CLKEN_BIT, 0);
		/* dsdl sdm clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG08, 1<<HI6403ES_DSDL_SDM_CLKEN_BIT, 0);
		/* dacl src up16 clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG06, 1<<HI6403ES_DACL_UP16_CLKEN_BIT, 0);
		/* dacl pga clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG10, 1<<HI6403ES_DACL_PGA_CLKEN_BIT, 0);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_dacr_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* dacr pga clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG10, 1<<HI6403ES_DACR_PGA_CLKEN_BIT, 1<<HI6403ES_DACR_PGA_CLKEN_BIT);
		/* dacr src up16 clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG06, 1<<HI6403ES_DACR_UP16_CLKEN_BIT, 1<<HI6403ES_DACR_UP16_CLKEN_BIT);
		/* dacr sdm clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG08, 1<<HI6403ES_DACR_SDM_CLKEN_BIT, 1<<HI6403ES_DACR_SDM_CLKEN_BIT);
		/* dsdr sdm clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG08, 1<<HI6403ES_DSDR_SDM_CLKEN_BIT, 1<<HI6403ES_DSDR_SDM_CLKEN_BIT);

		break;
	case SND_SOC_DAPM_POST_PMD:
		/* dacr sdm clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG08, 1<<HI6403ES_DACR_SDM_CLKEN_BIT, 0);
		/* dsdr sdm clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG08, 1<<HI6403ES_DSDR_SDM_CLKEN_BIT, 0);
		/* dacr src up16 clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG06, 1<<HI6403ES_DACR_UP16_CLKEN_BIT, 0);
		/* dacrl pga clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG10, 1<<HI6403ES_DACR_PGA_CLKEN_BIT, 0);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_dacls_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* dacls pga enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG11, 1<<HI6403ES_DACLS_PGA_CLKEN_BIT, 1<<HI6403ES_DACLS_PGA_CLKEN_BIT);
		/* dacls clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG09, 1<<HI6403ES_DACLS_CLKEN_BIT, 1<<HI6403ES_DACLS_CLKEN_BIT);
		/* dacls src up16 clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG06, 1<<HI6403ES_DACLS_UP16_CLKEN_BIT, 1<<HI6403ES_DACLS_UP16_CLKEN_BIT);
		/* dacls sdm clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG08, 1<<HI6403ES_DACLS_SDM_CLKEN_BIT, 1<<HI6403ES_DACLS_SDM_CLKEN_BIT);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* dacls sdm clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG08, 1<<HI6403ES_DACLS_SDM_CLKEN_BIT, 0);
		/* dacls src up16 clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG06, 1<<HI6403ES_DACLS_UP16_CLKEN_BIT, 0);
		/* dacls clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG09, 1<<HI6403ES_DACLS_CLKEN_BIT, 0);
		/* dacls pga disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG11, 1<<HI6403ES_DACLS_PGA_CLKEN_BIT, 0);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_dacrs_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* dacrs clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG09, 1<<HI6403ES_DACRS_CLKEN_BIT, 1<<HI6403ES_DACRS_CLKEN_BIT);
		/* dacrs src up16 clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG06, 1<<HI6403ES_DACRS_UP16_CLKEN_BIT, 1<<HI6403ES_DACRS_UP16_CLKEN_BIT);
		/* dacrs sdm clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG08, 1<<HI6403ES_DACRS_SDM_CLKEN_BIT, 1<<HI6403ES_DACRS_SDM_CLKEN_BIT);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* dacrs sdm clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG08, 1<<HI6403ES_DACRS_SDM_CLKEN_BIT, 0);
		/* dacrs rsrc up16 clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG06, 1<<HI6403ES_DACRS_UP16_CLKEN_BIT, 0);
		/* dacrs clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG09, 1<<HI6403ES_DACRS_CLKEN_BIT, 0);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_sidetone_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* sidetone pga & srcclk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG10, 1<<HI6403ES_SIDE_PGA_CLKEN_BIT, 1<<HI6403ES_SIDE_PGA_CLKEN_BIT);
		//hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG10, 1<<HI6403ES_SIDE_SRC_CLKEN_BIT, 1<<HI6403ES_SIDE_SRC_CLKEN_BIT);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* sidetone pga & src clk disable */
		//hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG10, 1<<HI6403ES_SIDE_SRC_CLKEN_BIT, 0);
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG10, 1<<HI6403ES_SIDE_PGA_CLKEN_BIT, 0);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_madswitch_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* mad src enable */
		hi64xx_update_bits(codec, HI6403ES_S4_PORT_CLKEN_REG, 1<<HI6403ES_S1_MAD_SRC_CLKEN_BIT, 1<<HI6403ES_S1_MAD_SRC_CLKEN_BIT);
		/* s1_o_dsp_if_din_sel->mad_buffer out */
		hi64xx_update_bits(codec, HI6403ES_SC_CODEC_MUX_SEL1_3, 0x3, 0x2);
		/* keep for mad test */
		/* adc clk select pll48k */
		//hi64xx_update_bits(codec, HI6403ES_PATH_CLK_SEL_REG, 0xF, 0);
		/* end for mad test */
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* s1_o_dsp_if_din_sel->mad_buffer out */
		hi64xx_update_bits(codec, HI6403ES_SC_CODEC_MUX_SEL1_3, 0x3, 0x0);
		/* mad src disable */
		hi64xx_update_bits(codec, HI6403ES_S4_PORT_CLKEN_REG, 1<<HI6403ES_S1_MAD_SRC_CLKEN_BIT, 0);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_s2up_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct hi6403es_platform_data *priv = NULL;
	BUG_ON(NULL == codec);

	priv = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == priv);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		hi64xx_update_bits(codec, HI6403ES_S2_PORT_CLKEN_REG, 1<<HI6403ES_S2_OR_SRC_CLKEN_BIT, 1<<HI6403ES_S2_OR_SRC_CLKEN_BIT);
		hi64xx_update_bits(codec, HI6403ES_S2_PORT_CLKEN_REG, 1<<HI6403ES_S2_OL_SRC_CLKEN_BIT, 1<<HI6403ES_S2_OL_SRC_CLKEN_BIT);
		hi64xx_update_bits(codec, HI6403ES_S2_PORT_CLKEN_REG, 1<<HI6403ES_S2_IR_SRC_CLKEN_BIT, 1<<HI6403ES_S2_IR_SRC_CLKEN_BIT);
		hi64xx_update_bits(codec, HI6403ES_S2_PORT_CLKEN_REG, 1<<HI6403ES_S2_IL_SRC_CLKEN_BIT, 1<<HI6403ES_S2_IL_SRC_CLKEN_BIT);
		break;
	case SND_SOC_DAPM_POST_PMD:
		hi64xx_update_bits(codec, HI6403ES_S2_PORT_CLKEN_REG, 1<<HI6403ES_S2_OR_SRC_CLKEN_BIT, 0);
		hi64xx_update_bits(codec, HI6403ES_S2_PORT_CLKEN_REG, 1<<HI6403ES_S2_OL_SRC_CLKEN_BIT, 0);
		hi64xx_update_bits(codec, HI6403ES_S2_PORT_CLKEN_REG, 1<<HI6403ES_S2_IR_SRC_CLKEN_BIT, 0);
		hi64xx_update_bits(codec, HI6403ES_S2_PORT_CLKEN_REG, 1<<HI6403ES_S2_IL_SRC_CLKEN_BIT, 0);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_audioup_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct hi6403es_platform_data *priv = NULL;
	int ret = 0;

	BUG_ON(NULL == codec);

	priv = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == priv);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		priv->capture_params.channels = 2;
		ret = slimbus_track_activate(SLIMBUS_DEVICE_HI6403, SLIMBUS_TRACK_AUDIO_CAPTURE, &priv->capture_params);
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (priv->audioup_4mic_state == TRACK_FREE)
			ret = slimbus_track_deactivate(SLIMBUS_DEVICE_HI6403, SLIMBUS_TRACK_AUDIO_CAPTURE, NULL);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return ret;
}

int hi6403es_audioup_4mic_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct hi6403es_platform_data *priv = NULL;
	int ret = 0;

	BUG_ON(NULL == codec);

	priv = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == priv);

	if (priv->voiceup_state && (priv->voice_params.channels == 4)) {
		priv->capture_params.channels = 2;
	} else {
		priv->capture_params.channels = 4;
	}

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		if (priv->capture_params.channels == 4) {
			snd_soc_write(codec, HI6403ES_SLIM4_CLK_CFG_REG, 0x44);
			snd_soc_write(codec, HI6403ES_SLIM5_CLK_CFG_REG, 0x44);
		}
		hi64xx_update_bits(codec, HI6403ES_S3_CLK_CFG_REG1, 0x70, 0x40);

		ret = slimbus_track_activate(SLIMBUS_DEVICE_HI6403, SLIMBUS_TRACK_AUDIO_CAPTURE, &priv->capture_params);

		priv->audioup_4mic_state = TRACK_STARTUP;

		break;
	case SND_SOC_DAPM_POST_PMD:
		ret = slimbus_track_deactivate(SLIMBUS_DEVICE_HI6403, SLIMBUS_TRACK_AUDIO_CAPTURE, &priv->capture_params);
		priv->audioup_4mic_state = TRACK_FREE;
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return ret;
}

int hi6403es_voice8k_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct hi6403es_platform_data *priv = NULL;
	int ret = 0;
	slimbus_track_param_t voice_down_param;

	BUG_ON(NULL == codec);

	priv = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == priv);

	memset(&voice_down_param, 0, sizeof(voice_down_param));

	priv->voice_params.rate = SLIMBUS_SAMPLE_RATE_8K;
	voice_down_param.rate = priv->voice_params.rate;
	voice_down_param.channels = 2;
	voice_down_param.callback = priv->voice_params.callback;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* fs config & src power on */
		snd_soc_write(codec, HI6403ES_S3_CLK_CFG_REG1, 0x0);
		hi64xx_update_bits(codec, HI6403ES_S3_PORT_CLKEN_REG, 0xC4, 0);
		hi64xx_update_bits(codec, HI6403ES_S3_CONTROL_REG, 0xE, 0xA);
		hi64xx_update_bits(codec, HI6403ES_S3_DSPIF_CONTROL_REG, 0x3F, 0x12);
		hi64xx_update_bits(codec, HI6403ES_S3_PORT_CLKEN_REG, 0xC4, 0xC4);
		snd_soc_write(codec, HI6403ES_SLIM2_CLK_CFG_REG, 0x0);
		snd_soc_write(codec, HI6403ES_SLIM5_CLK_CFG_REG, 0x0);
		if (4 == priv->voice_params.channels) {
			hi64xx_update_bits(codec, HI6403ES_S4_PORT_CLKEN_REG, 0x3, 0x0);
			hi64xx_update_bits(codec, HI6403ES_U3_SRC_MODE_CFG_REG, 0x7, 0x2);
			hi64xx_update_bits(codec, HI6403ES_S4_PORT_CLKEN_REG, 0x3, 0x3);
			snd_soc_write(codec, HI6403ES_SLIM4_CLK_CFG_REG, 0x0);
			hi64xx_update_bits(codec, HI6403ES_S4_PORT_CLKEN_REG, 1 << 3, 1 << 3);
		}
		/* slimbus voice active */
		ret = slimbus_track_activate(SLIMBUS_DEVICE_HI6403, SLIMBUS_TRACK_VOICE_UP, &priv->voice_params);
		ret += slimbus_track_activate(SLIMBUS_DEVICE_HI6403, SLIMBUS_TRACK_VOICE_DOWN, &voice_down_param);

		priv->voiceup_state = TRACK_STARTUP;

		if (4 == priv->voice_params.channels)
			hi64xx_update_bits(codec, HI6403ES_S4_PORT_CLKEN_REG, 1 << 3, 0);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* slimbus voice deactive */
		ret = slimbus_track_deactivate(SLIMBUS_DEVICE_HI6403, SLIMBUS_TRACK_VOICE_UP, NULL);
		ret += slimbus_track_deactivate(SLIMBUS_DEVICE_HI6403, SLIMBUS_TRACK_VOICE_DOWN, NULL);

		priv->voiceup_state = TRACK_FREE;

		/* src power off */
		hi64xx_update_bits(codec, HI6403ES_S3_PORT_CLKEN_REG, 0xC4, 0x0);
		if (4 == priv->voice_params.channels) {
			hi64xx_update_bits(codec, HI6403ES_S4_PORT_CLKEN_REG, 0x3, 0x0);
		}
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return ret;
}

int hi6403es_voice16k_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct hi6403es_platform_data *priv = NULL;
	int ret = 0;
	slimbus_track_param_t voice_down_param;

	BUG_ON(NULL == codec);

	priv = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == priv);

	memset(&voice_down_param, 0, sizeof(voice_down_param));

	priv->voice_params.rate = SLIMBUS_SAMPLE_RATE_16K;
	voice_down_param.rate = priv->voice_params.rate;
	voice_down_param.channels = 2;
	voice_down_param.callback = priv->voice_params.callback;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* fs config & src power on */
		snd_soc_write(codec, HI6403ES_S3_CLK_CFG_REG1, 0x11);
		hi64xx_update_bits(codec, HI6403ES_S3_PORT_CLKEN_REG, 0xC4, 0);
		hi64xx_update_bits(codec, HI6403ES_S3_CONTROL_REG, 0xE, 0x2);
		hi64xx_update_bits(codec, HI6403ES_S3_DSPIF_CONTROL_REG, 0x3F, 0);
		hi64xx_update_bits(codec, HI6403ES_S3_PORT_CLKEN_REG, 0xC4, 0xC4);
		snd_soc_write(codec, HI6403ES_SLIM2_CLK_CFG_REG, 0x11);
		snd_soc_write(codec, HI6403ES_SLIM5_CLK_CFG_REG, 0x11);
		if (4 == priv->voice_params.channels) {
			hi64xx_update_bits(codec, HI6403ES_S4_PORT_CLKEN_REG, 0x3, 0x0);
			hi64xx_update_bits(codec, HI6403ES_U3_SRC_MODE_CFG_REG, 0x7, 0x0);
			hi64xx_update_bits(codec, HI6403ES_S4_PORT_CLKEN_REG, 0x3, 0x3);
			snd_soc_write(codec, HI6403ES_SLIM4_CLK_CFG_REG, 0x11);
			hi64xx_update_bits(codec, HI6403ES_S4_PORT_CLKEN_REG, 1 << 3, 1 << 3);
		}

		/* slimbus voice active */
		ret = slimbus_track_activate(SLIMBUS_DEVICE_HI6403, SLIMBUS_TRACK_VOICE_UP, &priv->voice_params);
		ret += slimbus_track_activate(SLIMBUS_DEVICE_HI6403, SLIMBUS_TRACK_VOICE_DOWN, &voice_down_param);

		priv->voiceup_state = TRACK_STARTUP;

		if (4 == priv->voice_params.channels)
			hi64xx_update_bits(codec, HI6403ES_S4_PORT_CLKEN_REG, 1 << 3, 0);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* slimbus voice deactive */
		ret = slimbus_track_deactivate(SLIMBUS_DEVICE_HI6403, SLIMBUS_TRACK_VOICE_UP, NULL);
		ret += slimbus_track_deactivate(SLIMBUS_DEVICE_HI6403, SLIMBUS_TRACK_VOICE_DOWN, NULL);

		priv->voiceup_state = TRACK_FREE;

		/* src power off */
		hi64xx_update_bits(codec, HI6403ES_S3_PORT_CLKEN_REG, 0xC4, 0x0);
		if (4 == priv->voice_params.channels) {
			hi64xx_update_bits(codec, HI6403ES_S4_PORT_CLKEN_REG, 0x3, 0x0);
		}
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return ret;
}

int hi6403es_voice32k_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct hi6403es_platform_data *priv = NULL;
	slimbus_track_param_t voice_down_param;
	int ret = 0;

	BUG_ON(NULL == codec);

	priv = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == priv);

	memset(&voice_down_param, 0, sizeof(voice_down_param));

	priv->voice_params.rate = SLIMBUS_SAMPLE_RATE_32K;
	voice_down_param.rate = priv->voice_params.rate;
	voice_down_param.channels = 2;
	voice_down_param.callback = priv->voice_params.callback;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* fs config & src power on */
		snd_soc_write(codec, HI6403ES_S3_CLK_CFG_REG1, 0x22);
		hi64xx_update_bits(codec, HI6403ES_S3_PORT_CLKEN_REG, 0xC4, 0);
		hi64xx_update_bits(codec, HI6403ES_S3_CONTROL_REG, 0xE, 0x4);
		hi64xx_update_bits(codec, HI6403ES_S3_DSPIF_CONTROL_REG, 0x3F, 0x24);
		hi64xx_update_bits(codec, HI6403ES_S3_PORT_CLKEN_REG, 0xC4, 0xC4);
		snd_soc_write(codec, HI6403ES_SLIM2_CLK_CFG_REG, 0x22);
		snd_soc_write(codec, HI6403ES_SLIM5_CLK_CFG_REG, 0x22);
		if (4 == priv->voice_params.channels) {
			hi64xx_update_bits(codec, HI6403ES_S4_PORT_CLKEN_REG, 0x3, 0x0);
			hi64xx_update_bits(codec, HI6403ES_U3_SRC_MODE_CFG_REG, 0x7, 0x4);
			hi64xx_update_bits(codec, HI6403ES_U4_SRC_MODE_CFG_REG, 0x7, 0x4);
			hi64xx_update_bits(codec, HI6403ES_S4_PORT_CLKEN_REG, 0x3, 0x3);
			snd_soc_write(codec, HI6403ES_SLIM4_CLK_CFG_REG, 0x22);
			hi64xx_update_bits(codec, HI6403ES_S4_PORT_CLKEN_REG, 1 << 3, 1 << 3);
		}

		/* slimbus voice active */
		ret = slimbus_track_activate(SLIMBUS_DEVICE_HI6403, SLIMBUS_TRACK_VOICE_UP, &priv->voice_params);
		ret += slimbus_track_activate(SLIMBUS_DEVICE_HI6403, SLIMBUS_TRACK_VOICE_DOWN, &voice_down_param);

		priv->voiceup_state = TRACK_STARTUP;

		if (4 == priv->voice_params.channels)
			hi64xx_update_bits(codec, HI6403ES_S4_PORT_CLKEN_REG, 1 << 3, 0);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* slimbus voice deactive */
		ret = slimbus_track_deactivate(SLIMBUS_DEVICE_HI6403, SLIMBUS_TRACK_VOICE_UP, NULL);
		ret += slimbus_track_deactivate(SLIMBUS_DEVICE_HI6403, SLIMBUS_TRACK_VOICE_DOWN, NULL);

		priv->voiceup_state = TRACK_FREE;

		/* src power off */
		hi64xx_update_bits(codec, HI6403ES_S3_PORT_CLKEN_REG, 0xC4, 0x0);
		if (4 == priv->voice_params.channels) {
			hi64xx_update_bits(codec, HI6403ES_S4_PORT_CLKEN_REG, 0x3, 0x0);
		}
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return ret;
}

int hi6403es_slimbus_param_pass(slimbus_track_type_t	track,
							slimbus_track_param_t	*params, int event)
{
	int ret = 0;

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		ret = slimbus_track_activate(SLIMBUS_DEVICE_HI6403, track, params);
		break;
	case SND_SOC_DAPM_POST_PMD:
		ret = slimbus_track_deactivate(SLIMBUS_DEVICE_HI6403, track, params);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		ret = -1;
		break;
	}

	return ret;
}

int hi6403es_play48k_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	int ret = 0;

	ret = hi6403es_slimbus_param_pass(SLIMBUS_TRACK_AUDIO_PLAY, NULL, event);

	return ret;
}

int hi6403es_play96k_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct hi6403es_platform_data *priv = NULL;
	int ret = 0;

	BUG_ON(NULL == codec);

	priv = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == priv);

	priv->play_params.rate = SLIMBUS_SAMPLE_RATE_96K ;

	ret = hi6403es_slimbus_param_pass(SLIMBUS_TRACK_DIRECT_PLAY, &priv->play_params, event);

	return ret;
}

int hi6403es_play192k_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	struct hi6403es_platform_data *priv = NULL;
	int ret = 0;

	BUG_ON(NULL == codec);

	priv = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == priv);

	priv->play_params.rate = SLIMBUS_SAMPLE_RATE_192K ;

	ret = hi6403es_slimbus_param_pass(SLIMBUS_TRACK_DIRECT_PLAY, &priv->play_params, event);

	return ret;
}

int hi6403es_lowlatency_play_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	int ret = 0;

	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		ret = slimbus_track_activate(SLIMBUS_DEVICE_HI6403, SLIMBUS_TRACK_FAST_PLAY, NULL);
		snd_soc_write(codec, HI6403ES_I2S_SLIM_SEL_REG, 0x3F);
		break;
	case SND_SOC_DAPM_POST_PMD:
		ret = slimbus_track_deactivate(SLIMBUS_DEVICE_HI6403, SLIMBUS_TRACK_FAST_PLAY, NULL);
		snd_soc_write(codec, HI6403ES_I2S_SLIM_SEL_REG, 0x33);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		ret = -1;
		break;
	}

	return ret;
}

void hi6403es_headphone_low_to_high_ib05_current_config(struct snd_soc_codec *codec)
{
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG73,
		0xF<<HI6403ES_IB05_HP_BIT, 0x5<<HI6403ES_IB05_HP_BIT);
	msleep(10);
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG73,
		0xF<<HI6403ES_IB05_HP_BIT, 0x6<<HI6403ES_IB05_HP_BIT);
	msleep(10);
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG73,
		0xF<<HI6403ES_IB05_HP_BIT, 0x7<<HI6403ES_IB05_HP_BIT);
	msleep(10);
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG73,
		0xF<<HI6403ES_IB05_HP_BIT, 0x8<<HI6403ES_IB05_HP_BIT);
	msleep(10);
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG73,
		0xF<<HI6403ES_IB05_HP_BIT, 0x9<<HI6403ES_IB05_HP_BIT);
}

void hi6403es_headphone_high_to_low_ib05_current_config(struct snd_soc_codec *codec)
{
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG73,
			0xF<<HI6403ES_IB05_HP_BIT, 0x8<<HI6403ES_IB05_HP_BIT);
	msleep(10);
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG73,
			0xF<<HI6403ES_IB05_HP_BIT, 0x7<<HI6403ES_IB05_HP_BIT);
	msleep(10);
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG73,
			0xF<<HI6403ES_IB05_HP_BIT, 0x6<<HI6403ES_IB05_HP_BIT);
	msleep(10);
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG73,
			0xF<<HI6403ES_IB05_HP_BIT, 0x5<<HI6403ES_IB05_HP_BIT);
	msleep(10);
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG73,
			0xF<<HI6403ES_IB05_HP_BIT, 0x4<<HI6403ES_IB05_HP_BIT);
}

void hi6403es_headphone_high_mode_shadow_config(struct snd_soc_codec *codec)
{
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);

	BUG_ON(NULL == priv);

	/* reg pga gain */
	snd_soc_write(codec, HI6403ES_CLASSH_CTRL4, 0x0);

	/* shadow dacl pga gain */
	snd_soc_write(codec, HI6403ES_PERF_SWITCH_CTRL6, priv->hs_high_pga_gain);
	/* shadow dacr pga gain */
	snd_soc_write(codec, HI6403ES_PERF_SWITCH_CTRL7, priv->hs_high_pga_gain);
	/* dacl pga gain */
	snd_soc_write(codec, HI6403ES_DACL_PGA_GAIN_CFG_REG, priv->hs_high_pga_gain);
	/* dacr pga gain */
	snd_soc_write(codec, HI6403ES_DACR_PGA_GAIN_CFG_REG, priv->hs_high_pga_gain);

	/* shadow headphone VBR plan sel */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG38_SHADOW,
			0x1<<HI6403ES_DAC_HP_PLAN_SEL_BIT_SHADOW, 1<<HI6403ES_DAC_HP_PLAN_SEL_BIT_SHADOW);
	/* shadow headphone dac VB current select */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG38_SHADOW,
			0x3<<HI6403ES_DAC_HP_VBR_SEL_BIT_SHADOW, 0<<HI6403ES_DAC_HP_VBR_SEL_BIT_SHADOW);
	/* shadow headphone ICELL VB disable */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG38_SHADOW,
			0x1<<HI6403ES_DAC_HP_VBSWEN_BIT_SHADOW, 1<<HI6403ES_DAC_HP_VBSWEN_BIT_SHADOW);
	/* shadow headphone select SGND */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG48_SHADOW,
			1<<HI6403ES_HP_GND_CTR_BIT_SHADOW, 1<<HI6403ES_HP_GND_CTR_BIT_SHADOW);
	/* shadow headphone feed back enable */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG48_SHADOW,
			0x1<<HI6403ES_HP_FB_EN_BIT_SHADOW, 0x1<<HI6403ES_HP_FB_EN_BIT_SHADOW);
	/* shadow hp ibct current 10uA */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG48_SHADOW,
			0x3<<HI6403ES_HP_IBCT_BIT_SHADOW, 0x2<<HI6403ES_HP_IBCT_BIT_SHADOW);
	/* shadow headphone mode select high mode */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG48_SHADOW,
			0x1<<HI6403ES_HP_SRB_CTRL_BIT_SHADOW, 1<<HI6403ES_HP_SRB_CTRL_BIT_SHADOW);
	/* shadow headphone res select */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG48_SHADOW,
			0x7<<HI6403ES_HP_RES_CT_BIT_SHADOW, 0x3<<HI6403ES_HP_RES_CT_BIT_SHADOW);
	/* shadow headphone feed back ibct config */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG49_SHADOW,
			0x3<<HI6403ES_HP_FB_IBCT_BIT_SHADOW, 0x2<<HI6403ES_HP_FB_IBCT_BIT_SHADOW);
	/* shadow headphone feed back mode config */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG49_SHADOW,
			0x1<<HI6403ES_HP_FB_STB_CTRL_BIT_SHADOW, 1<<HI6403ES_HP_FB_STB_CTRL_BIT_SHADOW);
	/* shadow headphone feed back res config */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG49_SHADOW,
			0x7<<HI6403ES_HP_FB_RES_CT_BIT_SHADOW, 0x3<<HI6403ES_HP_FB_RES_CT_BIT_SHADOW);

	/* headphone VBR plan sel */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG38,
			0x1<<HI6403ES_DAC_HP_PLAN_SEL_BIT, 1<<HI6403ES_DAC_HP_PLAN_SEL_BIT);
	/* headphone dac VB current select */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG38,
			0x3<<HI6403ES_DAC_HP_VBR_SEL_BIT, 0<<HI6403ES_DAC_HP_VBR_SEL_BIT);
	/* headphone ICELL VB disable */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG38,
			0x1<<HI6403ES_DAC_HP_VBSWEN_BIT, 1<<HI6403ES_DAC_HP_VBSWEN_BIT);
	/* headphone select SGND */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG48,
			1<<HI6403ES_HP_GND_CTR_BIT, 1<<HI6403ES_HP_GND_CTR_BIT);
	/* headphone feed back enable */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG48,
			0x1<<HI6403ES_HP_FB_EN_BIT, 0x1<<HI6403ES_HP_FB_EN_BIT);
	/* hp ibct current 10uA */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG48,
			0x3<<HI6403ES_HP_IBCT_BIT, 0x2<<HI6403ES_HP_IBCT_BIT);
	/* headphone mode select high mode */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG48,
			0x1<<HI6403ES_HP_SRB_CTRL_BIT, 1<<HI6403ES_HP_SRB_CTRL_BIT);
	/* headphone res select */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG48,
			0x7<<HI6403ES_HP_RES_CT_BIT, 0x3<<HI6403ES_HP_RES_CT_BIT);
	/* headphone feed back ibct config */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG49,
			0x3<<HI6403ES_HP_FB_IBCT_BIT, 0x2<<HI6403ES_HP_FB_IBCT_BIT);
	/* headphone feed back mode config */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG49,
			0x1<<HI6403ES_HP_FB_STB_CTRL_BIT, 1<<HI6403ES_HP_FB_STB_CTRL_BIT);
	/* headphone feed back res config */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG49,
			0x7<<HI6403ES_HP_FB_RES_CT_BIT, 0x3<<HI6403ES_HP_FB_RES_CT_BIT);

	/* headphone ib05 current config */
	//hi64xx_update_bits(codec, HI6403ES_ANALOG_REG73,
	//	0xF<<HI6403ES_IB05_HP_BIT, 0x9<<HI6403ES_IB05_HP_BIT);
}

void hi6403es_headphone_low_mode_shadow_config(struct snd_soc_codec *codec)
{
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);

	BUG_ON(NULL == priv);

	/* reg pga gain */
	snd_soc_write(codec, HI6403ES_CLASSH_CTRL4, 0x18);

	/* shadow dacl pga gain */
	snd_soc_write(codec, HI6403ES_PERF_SWITCH_CTRL6, priv->hs_low_pga_gain);
	/* shadow dacr pga gain */
	snd_soc_write(codec, HI6403ES_PERF_SWITCH_CTRL7, priv->hs_low_pga_gain);
	/* dacl pga gain */
	snd_soc_write(codec, HI6403ES_DACL_PGA_GAIN_CFG_REG, priv->hs_low_pga_gain);
	/* dacr pga gain */
	snd_soc_write(codec, HI6403ES_DACR_PGA_GAIN_CFG_REG, priv->hs_low_pga_gain);

	/* shadow headphone VBR plan sel */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG38_SHADOW,
			0x1<<HI6403ES_DAC_HP_PLAN_SEL_BIT_SHADOW, 1<<HI6403ES_DAC_HP_PLAN_SEL_BIT_SHADOW);
	/* shadow headphone dac VB current select */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG38_SHADOW,
			0x3<<HI6403ES_DAC_HP_VBR_SEL_BIT_SHADOW, 1<<HI6403ES_DAC_HP_VBR_SEL_BIT_SHADOW);
	/* shadow headphone ICELL VB disable */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG38_SHADOW,
			0x1<<HI6403ES_DAC_HP_VBSWEN_BIT_SHADOW, 1<<HI6403ES_DAC_HP_VBSWEN_BIT_SHADOW);
	/* shadow headphone select SGND */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG48_SHADOW,
			1<<HI6403ES_HP_GND_CTR_BIT_SHADOW, 1<<HI6403ES_HP_GND_CTR_BIT_SHADOW);
	/* shadow headphone feed back disable */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG48_SHADOW,
			0x1<<HI6403ES_HP_FB_EN_BIT_SHADOW, 1<<HI6403ES_HP_FB_EN_BIT_SHADOW);
	/* shadow hp ibct current 10uA */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG48_SHADOW,
			0x3<<HI6403ES_HP_IBCT_BIT_SHADOW, 0x2<<HI6403ES_HP_IBCT_BIT_SHADOW);
	/* shadow headphone mode select low mode */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG48_SHADOW,
			0x1<<HI6403ES_HP_SRB_CTRL_BIT_SHADOW, 0<<HI6403ES_HP_SRB_CTRL_BIT_SHADOW);
	/* shadow headphone res select */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG48_SHADOW,
			0x7<<HI6403ES_HP_RES_CT_BIT_SHADOW, 0x5<<HI6403ES_HP_RES_CT_BIT_SHADOW);

	/* shadow headphone feed back ibct config
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG49_SHADOW,
			0x3<<HI6403ES_HP_FB_IBCT_BIT_SHADOW, 0x2<<HI6403ES_HP_FB_IBCT_BIT_SHADOW);*/
	/* shadow headphone feed back mode config
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG49_SHADOW,
			0x1<<HI6403ES_HP_FB_STB_CTRL_BIT_SHADOW, 0<<HI6403ES_HP_FB_STB_CTRL_BIT_SHADOW);*/
	/* shadow headphone feed back res config
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG49_SHADOW,
			0x7<<HI6403ES_HP_FB_RES_CT_BIT_SHADOW, 0x5<<HI6403ES_HP_FB_RES_CT_BIT_SHADOW);*/


	/* headphone VBR plan sel */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG38,
			0x1<<HI6403ES_DAC_HP_PLAN_SEL_BIT, 1<<HI6403ES_DAC_HP_PLAN_SEL_BIT);
	/* headphone dac VB current select */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG38,
			0x3<<HI6403ES_DAC_HP_VBR_SEL_BIT, 1<<HI6403ES_DAC_HP_VBR_SEL_BIT);
	/* headphone ICELL VB enable */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG38,
			0x1<<HI6403ES_DAC_HP_VBSWEN_BIT, 1<<HI6403ES_DAC_HP_VBSWEN_BIT);
	/* headphone select SGND */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG48,
			1<<HI6403ES_HP_GND_CTR_BIT, 1<<HI6403ES_HP_GND_CTR_BIT);
	/* headphone feed back disable */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG48,
			0x1<<HI6403ES_HP_FB_EN_BIT, 1<<HI6403ES_HP_FB_EN_BIT);
	/* hp ibct current 10uA */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG48,
			0x3<<HI6403ES_HP_IBCT_BIT, 0x2<<HI6403ES_HP_IBCT_BIT);
	/* headphone mode select low mode */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG48,//dirrerent
			0x1<<HI6403ES_HP_SRB_CTRL_BIT, 0<<HI6403ES_HP_SRB_CTRL_BIT);
	/* headphone res select */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG48,
			0x7<<HI6403ES_HP_RES_CT_BIT, 0x5<<HI6403ES_HP_RES_CT_BIT);

	/* headphone feed back ibct config
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG49,
			0x3<<HI6403ES_HP_FB_IBCT_BIT, 0x2<<HI6403ES_HP_FB_IBCT_BIT);*/
	/* headphone feed back mode config
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG49,
			0x1<<HI6403ES_HP_FB_STB_CTRL_BIT, 0<<HI6403ES_HP_FB_STB_CTRL_BIT);*/
	/* headphone feed back res config
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG49,
			0x7<<HI6403ES_HP_FB_RES_CT_BIT, 0x5<<HI6403ES_HP_FB_RES_CT_BIT);*/

	/* headphone ib05 current init */
	//hi64xx_update_bits(codec, HI6403ES_ANALOG_REG73,
	//		0xF<<HI6403ES_IB05_HP_BIT, 0x4<<HI6403ES_IB05_HP_BIT);
}


int hi6403es_hp_high_level_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	struct hi6403es_platform_data *priv = NULL;

	BUG_ON(NULL == codec);

	priv = snd_soc_codec_get_drvdata(codec);

	BUG_ON(NULL == priv);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* hpdac high performance */
		priv->rcv_hp_classh_state &= ~hp_classh_state;
		set_classh_config(codec, priv->rcv_hp_classh_state);

		if (true == priv->board_config.hp_high_low_change_enable) {
			/* disable the trigger config */
			hi64xx_update_bits(codec, HI6403ES_PERF_SWITCH_CTRL8,
					0x1<<HI6403ES_PERF_TRIGGER_EN_BIT, 0x0<<HI6403ES_PERF_TRIGGER_EN_BIT);
			/* high mode shadow config  */
			hi6403es_headphone_high_mode_shadow_config(codec);
			/* headphone ib05 current config */
			hi6403es_headphone_low_to_high_ib05_current_config(codec);
			/* enable the trigger config */
			hi64xx_update_bits(codec, HI6403ES_PERF_SWITCH_CTRL8,
					0x1<<HI6403ES_PERF_TRIGGER_EN_BIT, 0x1<<HI6403ES_PERF_TRIGGER_EN_BIT);
		}
		break;

	case SND_SOC_DAPM_POST_PMD:
		/* hpdac lower power */
		priv->rcv_hp_classh_state |= hp_classh_state;
		set_classh_config(codec, priv->rcv_hp_classh_state);

		if (true == priv->board_config.hp_high_low_change_enable) {
			/* disable the trigger config */
			hi64xx_update_bits(codec, HI6403ES_PERF_SWITCH_CTRL8,
					0x1<<HI6403ES_PERF_TRIGGER_EN_BIT, 0x0<<HI6403ES_PERF_TRIGGER_EN_BIT);
			/* low mode shadow config  */
			hi6403es_headphone_low_mode_shadow_config(codec);
			/* headphone ib05 current config */
			hi6403es_headphone_high_to_low_ib05_current_config(codec);
			/* enable the trigger config */
			hi64xx_update_bits(codec, HI6403ES_PERF_SWITCH_CTRL8,
					0x1<<HI6403ES_PERF_TRIGGER_EN_BIT, 0x1<<HI6403ES_PERF_TRIGGER_EN_BIT);
		}
		break;
	default :
		dev_warn(codec->dev, "%s : power mode event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

void hi6403es_pll_param_pass(struct snd_soc_dapm_widget *w,
						enum hi64xx_pll_type pll_type, int event)
{
	struct hi6403es_platform_data *priv = NULL;
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	BUG_ON(NULL == codec);

	priv = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == priv);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		hi64xx_resmgr_request_pll(priv->resmgr, pll_type);
		break;
	case SND_SOC_DAPM_POST_PMD:
		hi64xx_resmgr_release_pll(priv->resmgr, pll_type);
		break;
	default :
		dev_warn(codec->dev, "%s : power mode event err : %d\n", __FUNCTION__, event);
		break;
	}

	return;
}

int hi6403es_pll48k_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	hi6403es_pll_param_pass(w, PLL_HIGH, event);

	return 0;
}

int hi6403es_pll44k1_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	hi6403es_pll_param_pass(w, PLL_44_1, event);

	return 0;
}

int hi6403es_pllmad_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	hi6403es_pll_param_pass(w, PLL_LOW, event);

	return 0;
}

static void anc_hs_control_charge(struct snd_soc_codec *codec, bool enable)
{
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == priv);

	dev_info(codec->dev, "%s + \n",__FUNCTION__);

#ifdef CONFIG_ANC_HS_INTERFACE
	if(!check_anc_hs_interface_support()) {
		return;
	}

	/* mask btn irqs */
	hi64xx_irq_mask_btn_irqs(priv->mbhc);
	if(enable) {
		pr_info("%s enable \n",__FUNCTION__);
			anc_hs_interface_force_charge(ANC_HS_ENABLE_CHARGE);
		} else {
			pr_info("%s disable \n",__FUNCTION__);
			anc_hs_interface_force_charge(ANC_HS_DISABLE_CHARGE);
		}
		/* unmask btn irqs */
		hi64xx_irq_unmask_btn_irqs(priv->mbhc);
#endif

}

int hi6403es_hsmic_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct hi6403es_platform_data *data = NULL;
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	BUG_ON(NULL == codec);

	data = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == data);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* stop anc charge */
		anc_hs_control_charge(codec, false);
		/* headset micbias on */
		hi64xx_resmgr_request_micbias(data->resmgr);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* headset micbias off */
		hi64xx_resmgr_release_micbias(data->resmgr);
		/* resume anc charge */
		anc_hs_control_charge(codec, true);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}

	return 0;
}

int hi6403es_mainmic_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
#ifdef CONFIG_ANC_HS_INTERFACE
		if(check_anc_hs_interface_support()) {
			anc_hs_interface_5v_control(ANC_HS_ENABLE_5VOLTATE);
		}
#endif
		/* mainmic discharge disable */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG093, 1<<HI6403ES_MAINMIC_DISCHG_EN_BIT, 0);
		/* mainmic power on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG089, 1<<HI6403ES_MAINMIC_PD_BIT, 0);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* mainmic power off */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG089, 1<<HI6403ES_MAINMIC_PD_BIT, 1<<HI6403ES_MAINMIC_PD_BIT);
		/* mainmic discharge enable */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG093, 1<<HI6403ES_MAINMIC_DISCHG_EN_BIT, 1<<HI6403ES_MAINMIC_DISCHG_EN_BIT);
		msleep(10);
		/* mainmic discharge disable */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG093, 1<<HI6403ES_MAINMIC_DISCHG_EN_BIT, 0);

#ifdef CONFIG_ANC_HS_INTERFACE
		if(check_anc_hs_interface_support()) {
			anc_hs_interface_5v_control(ANC_HS_DISABLE_5VOLTATE);
		}
#endif
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}
	return 0;
}

int hi6403es_auxmic_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* auxmic discharge disable */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG093, 1<<HI6403ES_AUXMIC_DISCHG_EN_BIT, 0);
		/* auxmic power on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG089, 1<<HI6403ES_AUXMIC_PD_BIT, 0);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* auxmic power off */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG089, 1<<HI6403ES_AUXMIC_PD_BIT, 1<<HI6403ES_AUXMIC_PD_BIT);
		/* auxmic discharge enable */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG093, 1<<HI6403ES_AUXMIC_DISCHG_EN_BIT, 1<<HI6403ES_AUXMIC_DISCHG_EN_BIT);
		msleep(10);
		/* auxmic discharge disable */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG093, 1<<HI6403ES_AUXMIC_DISCHG_EN_BIT, 0);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}
	return 0;
}

int hi6403es_mic3_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* auxmic discharge disable */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG093, 1<<HI6403ES_MIC3_DISCHG_EN_BIT, 0);
		/* auxmic power on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG089, 1<<HI6403ES_MIC3_PD_BIT, 0);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* auxmic power off */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG089, 1<<HI6403ES_MIC3_PD_BIT, 1<<HI6403ES_MIC3_PD_BIT);
		/* auxmic discharge enable */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG093, 1<<HI6403ES_MIC3_DISCHG_EN_BIT, 1<<HI6403ES_MIC3_DISCHG_EN_BIT);
		msleep(10);
		/* auxmic discharge disable */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG093, 1<<HI6403ES_MIC3_DISCHG_EN_BIT, 0);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}
	return 0;
}

int hi6403es_mic4_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);
	BUG_ON(NULL == codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* auxmic discharge disable */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG093, 1<<HI6403ES_MIC4_DISCHG_EN_BIT, 0);
		/* auxmic power on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG089, 1<<HI6403ES_MIC4_PD_BIT, 0);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* auxmic power off */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG089, 1<<HI6403ES_MIC4_PD_BIT, 1<<HI6403ES_MIC4_PD_BIT);
		/* auxmic discharge enable */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG093, 1<<HI6403ES_MIC4_DISCHG_EN_BIT, 1<<HI6403ES_MIC4_DISCHG_EN_BIT);
		msleep(10);
		/* auxmic discharge disable */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG093, 1<<HI6403ES_MIC4_DISCHG_EN_BIT, 0);
		break;
	default :
		pr_warn("%s : event err : %d\n", __FUNCTION__, event);
		break;
	}
	return 0;
}

int hi6403es_mad_src_power_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	return 0;
}

int hi6403es_s1_ol_switch_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	return 0;
}

int hi6403es_s1_or_switch_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	return 0;
}

int hi6403es_s3_ol_switch_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	return 0;
}

int hi6403es_s3_or_switch_power_event(struct snd_soc_dapm_widget *w,
			struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = snd_soc_dapm_to_codec(w->dapm);

	BUG_ON(NULL == codec);

	return 0;
}

/* SOUND KCONTROLS */
static const struct snd_kcontrol_new hi6403es_snd_controls[] = {
	/* s2 port cfg */
	SOC_SINGLE("S2 IF FS CFG",
		HI6403ES_S2_CLK_CFG_REG, HI6403ES_S2_CLK_SEL_BIT, 7, 0),
	SOC_SINGLE("S2 DSP IF IN FS",
		HI6403ES_S2_CLK_CFG_REG1, HI6403ES_DSP_IF_IN_FS_CFG, 7, 0),
	SOC_SINGLE("S2 DSP IF OUT FS",
		HI6403ES_S2_CLK_CFG_REG1, HI6403ES_DSP_IF_OUT_FS_CFG, 7, 0),
	SOC_SINGLE("S2 IF FUNC MODE CFG",
		HI6403ES_S2_CFG_REG, HI6403ES_S2_MODE_CFG_BIT, 5, 0),
	SOC_SINGLE("S2 IF FRAME MODE CFG",
		HI6403ES_S2_CFG_REG_H, HI6403ES_S2_FRAME_MODE_BIT, 1, 0),
	SOC_SINGLE("S2 RX CLK SEL",
		HI6403ES_S2_CFG_REG_H, HI6403ES_S2_RX_CLK_SEL_BIT, 1, 0),
	SOC_SINGLE("S2 TX CLK SEL",
		HI6403ES_S2_CFG_REG_H, HI6403ES_S2_TX_CLK_SEL_BIT, 1, 0),

	/* s1 i pga gain kcontrol */
	SOC_SINGLE("S1 IL PGA GAIN",
		HI6403ES_S1_PGA_IL_GAIN_CFG_REG, HI6403ES_S1_PGA_IL_GAIN_BIT, 255, 0),
	SOC_SINGLE("S1 IR PGA GAIN",
		HI6403ES_S1_PGA_IR_GAIN_CFG_REG, HI6403ES_S1_PGA_IR_GAIN_BIT, 255, 0),
	/* s2 i pga gain kcontrol */
	SOC_SINGLE("S2 IL PGA GAIN",
		HI6403ES_S2_PGA_IL_GAIN_CFG_REG, HI6403ES_S2_PGA_IL_GAIN_BIT, 255, 0),
	SOC_SINGLE("S2 IR PGA GAIN",
		HI6403ES_S2_PGA_IR_GAIN_CFG_REG, HI6403ES_S2_PGA_IR_GAIN_BIT, 255, 0),
	/* s3 i pga gain kcontrol */
	SOC_SINGLE("S3 IL PGA GAIN",
		HI6403ES_S3_PGA_IL_GAIN_CFG_REG, HI6403ES_S3_PGA_IL_GAIN_BIT, 255, 0),
	SOC_SINGLE("S3 IR PGA GAIN",
		HI6403ES_S3_PGA_IR_GAIN_CFG_REG, HI6403ES_S3_PGA_IR_GAIN_BIT, 255, 0),

	/* side pga gain kcontrol */
	SOC_SINGLE("SIDE PGA GAIN",
		HI6403ES_SIDE_PGA_GAIN_CFG_REG, HI6403ES_SIDE_PGA_GAIN_BIT, 255, 0),

	/* dacl mixer s1l gain kcontrol */
	SOC_SINGLE("DACL MIXER S1L GAIN",
		HI6403ES_DACL_MIXER_GAIN_CFG_REG, HI6403ES_DACL_MIXER_S1L_GAIN_BIT, 3, 0),

	/* dacl mixer s1r gain kcontrol */
	SOC_SINGLE("DACL MIXER S1R GAIN",
		HI6403ES_DACL_MIXER_GAIN_CFG_REG, HI6403ES_DACL_MIXER_S1R_GAIN_BIT, 3, 0),

	/*lint -e64*/
	/* dacr mixer s1l gain kcontrol */
	SOC_SINGLE("DACR MIXER S1L GAIN",
		HI6403ES_DACR_MIXER_GAIN_CFG_REG, HI6403ES_DACR_MIXER_S1L_GAIN_BIT, 3, 0),

	/* dacr mixer s1r gain kcontrol */
	SOC_SINGLE("DACR MIXER S1R GAIN",
		HI6403ES_DACR_MIXER_GAIN_CFG_REG, HI6403ES_DACR_MIXER_S1R_GAIN_BIT, 3, 0),
	/*lint +e64*/

	/* adc0l 05pga gain kcontrol */
	SOC_SINGLE("ADC0L 05PGA GAIN",
		HI6403ES_ADC0L_05PGA_GAIN_REG, HI6403ES_ADC0L_05PGA_GAIN_BIT, 0x7e, 0),
	/* adc0r 05pga gain kcontrol */
	SOC_SINGLE("ADC0R 05PGA GAIN",
		HI6403ES_ADC0R_05PGA_GAIN_REG, HI6403ES_ADC0R_05PGA_GAIN_BIT, 0x7e, 0),
	/* adc1l 05pga gain kcontrol */
	SOC_SINGLE("ADC1L 05PGA GAIN",
		HI6403ES_ADC1L_05PGA_GAIN_REG, HI6403ES_ADC1L_05PGA_GAIN_BIT, 0x7e, 0),
	/* adc1r 05pga gain kcontrol */
	SOC_SINGLE("ADC1R 05PGA GAIN",
		HI6403ES_ADC1R_05PGA_GAIN_REG, HI6403ES_ADC1R_05PGA_GAIN_BIT, 0x7e, 0),

	/* lineinl pga gain kcontrol */
	SOC_SINGLE_TLV("LINEL PGA GAIN",
		HI6403ES_ANALOG_REG20, HI6403ES_LINEINL_PGA_GAIN_BIT, 28, 0, lineinl_mic_tlv),
	/* lineinr pga gain kcontrol */
	SOC_SINGLE_TLV("LINER PGA GAIN",
		HI6403ES_ANALOG_REG21, HI6403ES_LINEINR_PGA_GAIN_BIT, 28, 0, lineinr_mic_tlv),
	/* aux pga gain kcontrol */
	SOC_SINGLE_TLV("AUX PGA GAIN",
		HI6403ES_ANALOG_REG22, HI6403ES_AUX_PGA_GAIN_BIT, 18, 0, aux_mic_tlv),
	/* main pga gain kcontrol */
	SOC_SINGLE_TLV("MAIN PGA GAIN",
		HI6403ES_ANALOG_REG23, HI6403ES_MAIN_PGA_GAIN_BIT, 18, 0, main_mic_tlv),

	/* lol pga gain kcontrol */
	SOC_SINGLE_TLV("LOL PGA GAIN",
		HI6403ES_ANALOG_REG63, HI6403ES_LOL_GAIN_BIT, 18, 0, lol_pga_tlv),

	/* lor pga gain kcontrol */
	SOC_SINGLE_TLV("LOR PGA GAIN",
		HI6403ES_ANALOG_REG64, HI6403ES_LOR_GAIN_BIT, 18, 0, lor_pga_tlv),

	/* ep pga gain kcontrol */
	SOC_SINGLE("EP PGA GAIN",
		HI6403ES_ANALOG_REG62, HI6403ES_EAR_GAIN_BIT, 22, 0),

	/* mad pga gain kcontrol */
	SOC_SINGLE_TLV("MAD PGA GAIN",
		HI6403ES_ANALOG_REG84, HI6403ES_MAD_PGA_GAIN_BIT, 18, 0, mad_tlv),

	/* s2 src kcontrol */
	SOC_SINGLE("S2 IL SRC MODE",
		HI6403ES_S2_CONTROL_REG, HI6403ES_S2IL_SRC_MODE_BIT, 7, 0),
	SOC_SINGLE("S2 IR SRC MODE",
		HI6403ES_S2_CONTROL_REG, HI6403ES_S2IR_SRC_MODE_BIT, 7, 0),
	SOC_SINGLE("S2 OL SRC MODE",
		HI6403ES_S2_DSPIF_CONTROL_REG, HI6403ES_S2OL_SRC_MODE_BIT, 7, 0),
	SOC_SINGLE("S2 OR SRC MODE",
		HI6403ES_S2_DSPIF_CONTROL_REG, HI6403ES_S2OR_SRC_MODE_BIT, 7, 0),

	/* BOOST */
	SOC_SINGLE("MAIN MIC BOOST0",
		HI6403ES_ANALOG_REG31, HI6403ES_ADC0L_BOOST_1_BIT, 1, 0),
	SOC_SINGLE("MAIN MIC BOOST1",
		HI6403ES_ANALOG_REG36, HI6403ES_ADC0L_BOOST_2_BIT, 1, 0),
	SOC_SINGLE("AUX MIC BOOST0",
		HI6403ES_ANALOG_REG31, HI6403ES_ADC0R_BOOST_1_BIT, 1, 0),
	SOC_SINGLE("AUX MIC BOOST1",
		HI6403ES_ANALOG_REG36, HI6403ES_ADC0R_BOOST_2_BIT, 1, 0),
	SOC_SINGLE("ADC MAD BOOST0",
		HI6403ES_ANALOG_REG088, HI6403ES_ADC_MAD_BOOST_1_BIT, 1, 0),
	SOC_SINGLE("ADC MAD BOOST1",
		HI6403ES_ANALOG_REG088, HI6403ES_ADC_MAD_BOOST_2_BIT, 1, 0),
	SOC_SINGLE("LINEINL BOOST0",
		HI6403ES_ANALOG_REG34, HI6403ES_ADC1L_BOOST_1_BIT, 1, 0),
	SOC_SINGLE("LINEINL BOOST1",
		HI6403ES_ANALOG_REG36, HI6403ES_ADC1L_BOOST_2_BIT, 1, 0),
	SOC_SINGLE("LINEINR BOOST0",
		HI6403ES_ANALOG_REG34, HI6403ES_ADC1R_BOOST_1_BIT, 1, 0),
	SOC_SINGLE("LINEINR BOOST1",
		HI6403ES_ANALOG_REG36, HI6403ES_ADC1R_BOOST_2_BIT, 1, 0),
};

/* SWITCH CONTROL */
/* MAD SWITCH */
static const struct snd_kcontrol_new hi6403es_dapm_mad_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_S1_PORT_CLKEN_REG, HI6403ES_S1_OL_SRC_CLKEN_BIT, 1, 0);
/* S1 OL SWITCH */
static const struct snd_kcontrol_new hi6403es_dapm_s1_ol_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_VIRTUAL_REG, HI6403ES_S1_OL_SRC_EN_MM_BIT, 1, 0);
/* S1 OR SWITCH */
static const struct snd_kcontrol_new hi6403es_dapm_s1_or_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_VIRTUAL_REG, HI6403ES_S1_OR_SRC_EN_MM_BIT, 1, 0);
/* S2 OL SWITCH */
static const struct snd_kcontrol_new hi6403es_dapm_s2_ol_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_S2_PORT_CLKEN_REG, HI6403ES_S2_OL_PGA_CLKEN_BIT, 1, 0);
/* S2 OR SWITCH */
static const struct snd_kcontrol_new hi6403es_dapm_s2_or_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_S2_PORT_CLKEN_REG, HI6403ES_S2_OR_PGA_CLKEN_BIT, 1, 0);
/* S3 OL SWITCH */
static const struct snd_kcontrol_new hi6403es_dapm_s3_ol_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_VIRTUAL_REG, HI6403ES_S3_OL_SRC_EN_MM_BIT, 1, 0);
/* S3 OR SWITCH */
static const struct snd_kcontrol_new hi6403es_dapm_s3_or_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_VIRTUAL_REG, HI6403ES_S3_OR_SRC_EN_MM_BIT, 1, 0);
/* S4 OL SWITCH */
static const struct snd_kcontrol_new hi6403es_dapm_s4_ol_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_VIRTUAL_REG, HI6403ES_S4_OL_SRC_EN_MM_BIT, 1, 0);
/* S4 OR SWITCH */
static const struct snd_kcontrol_new hi6403es_dapm_s4_or_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_VIRTUAL_REG, HI6403ES_S4_OR_SRC_EN_MM_BIT, 1, 0);
/* U3 SRC SWITCH */
static const struct snd_kcontrol_new hi6403es_dapm_u3_ol_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_VIRTUAL_REG, HI6403ES_U3_OL_SRC_EN_MM_BIT, 1, 0);
/* U4 SRC SWITCH */
static const struct snd_kcontrol_new hi6403es_dapm_u4_or_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_VIRTUAL_REG, HI6403ES_U4_OR_SRC_EN_MM_BIT, 1, 0);
/* AUDIOUP SWITCH */
static const struct snd_kcontrol_new hi6403es_dapm_audioup_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_VIRTUAL_REG, HI6403ES_AUDIOUP_BIT, 1, 0);
/* AUDIODOWN SWITCH */
static const struct snd_kcontrol_new hi6403es_dapm_play48k_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_VIRTUAL_REG, HI6403ES_PLAY48K_BIT, 1, 0);
static const struct snd_kcontrol_new hi6403es_dapm_play96k_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_VIRTUAL_REG, HI6403ES_PLAY96K_BIT, 1, 0);
static const struct snd_kcontrol_new hi6403es_dapm_play192k_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_VIRTUAL_REG, HI6403ES_PLAY192K_BIT, 1, 0);
/* Lowlatency SWITCH */
static const struct snd_kcontrol_new hi6403es_dapm_lowlatency_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_VIRTUAL_REG, HI6403ES_LOWLANTENCY_BIT, 1, 0);
/* AUDIOUP 4MIC SWITCH */
static const struct snd_kcontrol_new hi6403es_dapm_audioup_4mic_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_VIRTUAL_REG, HI6403ES_AUDIOUP_4MIC_BIT, 1, 0);
/* VOICEUP SWITCH */
static const struct snd_kcontrol_new hi6403es_dapm_voice8k_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_VIRTUAL_REG, HI6403ES_VOICE8K_BIT, 1, 0);
static const struct snd_kcontrol_new hi6403es_dapm_voice16k_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_VIRTUAL_REG, HI6403ES_VOICE16K_BIT, 1, 0);
static const struct snd_kcontrol_new hi6403es_dapm_voice32k_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_VIRTUAL_REG, HI6403ES_VOICE32K_BIT, 1, 0);
/* HPCLASSH SWITCH */
static const struct snd_kcontrol_new hi6403es_dapm_hp_high_level_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_VIRTUAL_REG, HI6403ES_HPCLASSH_BIT, 1, 0);
/* PLL48K SWITCH */
static const struct snd_kcontrol_new hi6403es_dapm_pll48k_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_VIRTUAL_REG, HI6403ES_PLL48K_BIT, 1, 0);
/* PLL44K1 SWITCH */
static const struct snd_kcontrol_new hi6403es_dapm_pll44k1_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_VIRTUAL_REG, HI6403ES_PLL44K1_BIT, 1, 0);
/* PLLMAD SWITCH */
static const struct snd_kcontrol_new hi6403es_dapm_pllmad_switch_controls =
	SOC_DAPM_SINGLE("SWITCH",
		HI6403ES_VIRTUAL_REG, HI6403ES_PLLMAD_BIT, 1, 0);

/* MUX CONTROL */
/* MUX OF DACL SRC IN 0x7215 */
static const char *hi6403es_dacl_src_mux_texts[] = {
	"DSP IF8 OL",
	"DACL",
};
static const struct soc_enum hi6403es_dacl_src_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_DAC_SRC_MUX_SEL, HI6403ES_DACL_SRC_MUX_BIT,
		ARRAY_SIZE(hi6403es_dacl_src_mux_texts),hi6403es_dacl_src_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_dacl_src_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_dacl_src_mux_enum);

/* MUX OF DACR SRC IN 0x7215 */
static const char *hi6403es_dacr_src_mux_texts[] = {
	"DSP IF8 OR",
	"DACR",
};
static const struct soc_enum hi6403es_dacr_src_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_DAC_SRC_MUX_SEL, HI6403ES_DACR_SRC_MUX_BIT,
		ARRAY_SIZE(hi6403es_dacr_src_mux_texts),hi6403es_dacr_src_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_dacr_src_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_dacr_src_mux_enum);

/* MUX OF DACL PGA IN 0x7289 */
static const char *hi6403es_dacl_pga_mux_texts[] = {
	"DACL_384",
	"ADC1L",
	"HP RES",
};
static const struct soc_enum hi6403es_dacl_pga_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_DACL_PGA_MUX_SEL, HI6403ES_DAC_PGA_MUX_BIT,
		ARRAY_SIZE(hi6403es_dacl_pga_mux_texts),hi6403es_dacl_pga_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_dacl_pga_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_dacl_pga_mux_enum);

/* MUX OF DACR PGA IN 0x728a */
static const char *hi6403es_dacr_pga_mux_texts[] = {
	"DACR_384",
	"ADC1R",
	"HP RES",
};
static const struct soc_enum hi6403es_dacr_pga_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_DACR_PGA_MUX_SEL, HI6403ES_DAC_PGA_MUX_BIT,
		ARRAY_SIZE(hi6403es_dacr_pga_mux_texts),hi6403es_dacr_pga_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_dacr_pga_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_dacr_pga_mux_enum);

/* MUX FOR DACL SDM45 0x733c */
static const char *hi6403es_dacl_sdm45_mux_texts[] = {
	"L UP",
	"DSDL",
	"IO TEST",
	"ANC",
};
static const struct soc_enum hi6403es_dacl_sdm45_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_DACL_SDM45_MUX_SEL, HI6403ES_DAC_SDM45_MUX_BIT,
		ARRAY_SIZE(hi6403es_dacl_sdm45_mux_texts),hi6403es_dacl_sdm45_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_dacl_sdm45_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_dacl_sdm45_mux_enum);

/* MUX FOR DACR SDM45 0x7340 */
static const char *hi6403es_dacr_sdm45_mux_texts[] = {
	"R UP",
	"DSDR",
	"IO TEST",
	"ANC",
};
static const struct soc_enum hi6403es_dacr_sdm45_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_DACR_SDM45_MUX_SEL, HI6403ES_DAC_SDM45_MUX_BIT,
		ARRAY_SIZE(hi6403es_dacr_sdm45_mux_texts),hi6403es_dacr_sdm45_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_dacr_sdm45_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_dacr_sdm45_mux_enum);

/* ADCL0 MUX 0x7294 */
static const char *hi6403es_adc_mux_texts[] = {
	"MAINMIC",
	"AUXMIC",
	"LINEINL",
	"LINEINR",
	"DMIC0L",
	"DMIC0R",
	"DMIC1L",
	"DMIC1R",
	"MAD",
};
static const struct soc_enum hi6403es_adcl0_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_ADC0_MUX_SEL, HI6403ES_ADCL0_MUX_BIT,
		ARRAY_SIZE(hi6403es_adc_mux_texts), hi6403es_adc_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_adcl0_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_adcl0_mux_enum);

/* ADCR0 MUX 0x7294 */
static const struct soc_enum hi6403es_adcr0_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_ADC0_MUX_SEL, HI6403ES_ADCR0_MUX_BIT,
		ARRAY_SIZE(hi6403es_adc_mux_texts), hi6403es_adc_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_adcr0_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_adcr0_mux_enum);

/* ADCL1 MUX 0x7295 */
static const struct soc_enum hi6403es_adcl1_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_ADC1_MUX_SEL, HI6403ES_ADCL1_MUX_BIT,
		ARRAY_SIZE(hi6403es_adc_mux_texts), hi6403es_adc_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_adcl1_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_adcl1_mux_enum);

/* ADCR0 MUX 0x7295 */
static const struct soc_enum hi6403es_adcr1_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_ADC1_MUX_SEL, HI6403ES_ADCR1_MUX_BIT,
		ARRAY_SIZE(hi6403es_adc_mux_texts), hi6403es_adc_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_adcr1_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_adcr1_mux_enum);

/* MIC1 MUX 0x7208 */
static const char *hi6403es_mic1_mux_texts[] = {
	"ADC0L",
	"ADC1L",
	"DACL_48",
	"S2L",
	"S3L",
	"S4L",
	"ADC0R",
	"ADC1R",
};
static const struct soc_enum hi6403es_mic1_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_MIC12_MUX_SEL, HI6403ES_MIC1_MUX_BIT,
		ARRAY_SIZE(hi6403es_mic1_mux_texts),hi6403es_mic1_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_mic1_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_mic1_mux_enum);

/* MIC2 MUX 0x7208 */
static const char *hi6403es_mic2_mux_texts[] = {
	"ADC0R",
	"ADC1R",
	"DACR_48",
	"S2R",
	"S3R",
	"S4R",
	"ADC0L",
	"ADC1L",
};
static const struct soc_enum hi6403es_mic2_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_MIC12_MUX_SEL, HI6403ES_MIC2_MUX_BIT,
		ARRAY_SIZE(hi6403es_mic2_mux_texts), hi6403es_mic2_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_mic2_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_mic2_mux_enum);

/* MIC3 MUX 0x7209 */
static const char *hi6403es_mic3_mux_texts[] = {
	"ADC0L",
	"ADC1L",
	"DACL_48",
	"S1L",
	"S2L",
	"S4L",
	"ADC0R",
	"ADC1R",
};
static const struct soc_enum hi6403es_mic3_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_MIC34_MUX_SEL, HI6403ES_MIC3_MUX_BIT,
		ARRAY_SIZE(hi6403es_mic3_mux_texts), hi6403es_mic3_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_mic3_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_mic3_mux_enum);

/* MIC4 MUX 0x7209 */
static const char *hi6403es_mic4_mux_texts[] = {
	"ADC0R",
	"ADC1R",
	"DACR_48",
	"S1R",
	"S2R",
	"S4R",
	"ADC0L",
	"ADC1L",
};
static const struct soc_enum hi6403es_mic4_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_MIC34_MUX_SEL, HI6403ES_MIC4_MUX_BIT,
		ARRAY_SIZE(hi6403es_mic4_mux_texts), hi6403es_mic4_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_mic4_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_mic4_mux_enum);

/* BTL MUX 0x720A */
static const char *hi6403es_btl_mux_texts[] = {
	"ADC0L",
	"ADC1L",
	"DACL_48",
	"S1L",
	"S3L",
	"S4L",
};
static const struct soc_enum hi6403es_btl_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_BT_MUX_SEL, HI6403ES_BTL_MUX_BIT,
		ARRAY_SIZE(hi6403es_btl_mux_texts),hi6403es_btl_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_btl_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_btl_mux_enum);

/* BTR MUX 0x720A */
static const char *hi6403es_btr_mux_texts[] = {
	"ADC0R",
	"ADC1R",
	"DACR_48",
	"S1R",
	"S3R",
	"S4R",
};
static const struct soc_enum hi6403es_btr_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_BT_MUX_SEL, HI6403ES_BTR_MUX_BIT,
		ARRAY_SIZE(hi6403es_btr_mux_texts), hi6403es_btr_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_btr_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_btr_mux_enum);

/* M1L MUX 0x720B */
static const char *hi6403es_m1l_mux_texts[] = {
	"ADC0L",
	"ADC1L",
	"DACSL",
	"DACL_48",
	"S1L",
	"S2L",
	"S3L",
};
static const struct soc_enum hi6403es_m1l_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_M1_MUX_SEL, HI6403ES_M1L_MUX_BIT,
		ARRAY_SIZE(hi6403es_m1l_mux_texts),hi6403es_m1l_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_m1l_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_m1l_mux_enum);

/* M1R MUX 0x720B */
static const char *hi6403es_m1r_mux_texts[] = {
	"ADC0R",
	"ADC1R",
	"DACSR",
	"DACR_48",
	"S1R",
	"S2R",
	"S3R",
};
static const struct soc_enum hi6403es_m1r_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_M1_MUX_SEL, HI6403ES_M1R_MUX_BIT,
		ARRAY_SIZE(hi6403es_m1r_mux_texts), hi6403es_m1r_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_m1r_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_m1r_mux_enum);

/* MDM MUX 0x7212 */
static const char *hi6403es_mdm_mux_texts[] = {
	"S3L",
	"S4L",
};
static const struct soc_enum hi6403es_mdm_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_MDM_MUX_SEL, HI6403ES_MDM_MUX_BIT,
		ARRAY_SIZE(hi6403es_mdm_mux_texts), hi6403es_mdm_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_mdm_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_mdm_mux_enum);

/* SIDETONE MUX 0x7212 */
static const char *hi6403es_sidetone_mux_texts[] = {
	"S1L",
	"S2L",
	"ADC0L",
	"ADC0R",
	"ADC1L",
	"ADC1R",
};
static const struct soc_enum hi6403es_sidetone_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_SIDETONE_MUX_SEL, HI6403ES_SIDETONE_MUX_BIT,
		ARRAY_SIZE(hi6403es_sidetone_mux_texts), hi6403es_sidetone_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_sidetone_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_sidetone_mux_enum);

/* NEARMIC MUX 0x7484 */
static const char *hi6403es_nearmic_mux_texts[] = {
	"ADC0L",
	"ADC1L",
	"ADC0R",
	"ADC1R",
};
static const struct soc_enum hi6403es_nearmic_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_ANC_MUX_SEL, HI6403ES_NEARMIC_MUX_BIT,
		ARRAY_SIZE(hi6403es_nearmic_mux_texts), hi6403es_nearmic_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_nearmic_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_nearmic_mux_enum);

/*PDM MUX 0x7297 */
static const char *hi6403es_pdm_mux_texts[] = {
	"DACL_48/DACR_48",
	"DACL_MIXER/DRCR_MIXER",
};
static const struct soc_enum hi6403es_pdm_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_PDM_MUX_SEL, HI6403ES_PDM_MUX_BIT,
		ARRAY_SIZE(hi6403es_pdm_mux_texts), hi6403es_pdm_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_pdm_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_pdm_mux_enum);

/*DACR SDM MUX 0x7342 */
static const char *hi6403es_dacr_sdm_mux_texts[] = {
	"R UP",
	"ANC",
	"IO TEST",
};
static const struct soc_enum hi6403es_dacr_sdm_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_DACR_SDM_MUX_SEL, HI6403ES_DACR_SDM_MUX_BIT,
		ARRAY_SIZE(hi6403es_dacr_sdm_mux_texts), hi6403es_dacr_sdm_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_dacr_sdm_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_dacr_sdm_mux_enum);

/*DACSR SDM MUX 0x7344 */
static const char *hi6403es_dacsr_sdm_mux_texts[] = {
	"SR UP",
	"ANC",
	"IO TEST",
};
static const struct soc_enum hi6403es_dacsr_sdm_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_DACSR_SDM_MUX_SEL, HI6403ES_DACSR_SDM_MUX_BIT,
		ARRAY_SIZE(hi6403es_dacsr_sdm_mux_texts), hi6403es_dacsr_sdm_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_dacsr_sdm_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_dacsr_sdm_mux_enum);

/*DAC HP MUX 0x7349 */
static const char *hi6403es_dac_hpl_mux_texts[] = {
	"DACL SDM45",
	"DACR SDM45",
	"HP_SDM_MUX",
	"Off",
};
static const struct soc_enum hi6403es_dac_hpl_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_DAC_HP_MUX_SEL, HI6403ES_DAC_HPL_MUX_BIT,
		ARRAY_SIZE(hi6403es_dac_hpl_mux_texts), hi6403es_dac_hpl_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_dac_hpl_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_dac_hpl_mux_enum);

static const char *hi6403es_dac_hpr_mux_texts[] = {
	"DACR SDM45",
	"DACL SDM45",
	"HP_SDM_MUX",
	"Off",
};
static const struct soc_enum hi6403es_dac_hpr_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_DAC_HP_MUX_SEL, HI6403ES_DAC_HPR_MUX_BIT,
		ARRAY_SIZE(hi6403es_dac_hpr_mux_texts), hi6403es_dac_hpr_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_dac_hpr_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_dac_hpr_mux_enum);

/*DACS MUX 0x734a */
static const char *hi6403es_dacs_mux_texts[] = {
	"DACL SDM",
	"DACSL",
	"DACR SDM",
	"DACSR",
	"EP_SDM_MUX",
	"Off",
};
static const struct soc_enum hi6403es_dac_ep_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_DACS_MUX_SEL, HI6403ES_DAC_EP_MUX_BIT,
		ARRAY_SIZE(hi6403es_dacs_mux_texts), hi6403es_dacs_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_dac_ep_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_dac_ep_mux_enum);

/*DAC LINEOUT L MUX */
static const struct soc_enum hi6403es_dac_ltl_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_DACS_MUX_SEL, HI6403ES_DAC_LTL_MUX_BIT,
		ARRAY_SIZE(hi6403es_dacs_mux_texts), hi6403es_dacs_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_dac_ltl_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_dac_ltl_mux_enum);

/*DAC LINEOUT R MUX 0x734b */
static const struct soc_enum hi6403es_dac_ltr_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_DAC_LTR_MUX_SEL, HI6403ES_DAC_LTR_MUX_BIT,
		ARRAY_SIZE(hi6403es_dacs_mux_texts), hi6403es_dacs_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_dac_ltr_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_dac_ltr_mux_enum);

/*ANC FAREND MUX 0x7484 */
static const char *hi6403es_anc_farend_mux_texts[] = {
	"DACL_384",
	"DACR_384",
	"DACSL_384",
	"DACSR_384",
};
static const struct soc_enum hi6403es_anc_farend_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_ANC_MUX_SEL, HI6403ES_ANC_FAREND_MUX_BIT,
		ARRAY_SIZE(hi6403es_anc_farend_mux_texts), hi6403es_anc_farend_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_anc_farend_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_anc_farend_mux_enum);

/*ERROR MIC MUX 0x7484 */
static const char *hi6403es_exp_mic_mux_texts[] = {
	"L0",
	"R0",
	"L1",
	"R1",
};
static const struct soc_enum hi6403es_error_mic_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_ANC_MUX_SEL, HI6403ES_ERROR_MIC_MUX_BIT,
		ARRAY_SIZE(hi6403es_exp_mic_mux_texts), hi6403es_exp_mic_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_error_mic_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_error_mic_mux_enum);

/*REF MIC MUX 0x7484 */
static const struct soc_enum hi6403es_ref_mic_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_ANC_MUX_SEL, HI6403ES_REF_MIC_MUX_BIT,
		ARRAY_SIZE(hi6403es_exp_mic_mux_texts), hi6403es_exp_mic_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_ref_mic_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_ref_mic_mux_enum);


/*DSP IF 8 TEST MUX 0x7211 */
static const char *hi6403es_if8_test_mux_texts[] = {
	"DAC MIXER",
	"ADC",
};
static const struct soc_enum hi6403es_if8_test_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_IF8_TEST_MUX_SEL, HI6403ES_IF8_TEST_MUX_BIT,
		ARRAY_SIZE(hi6403es_if8_test_mux_texts), hi6403es_if8_test_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_if8_test_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_if8_test_mux_enum);

/* LINEIN L MUX */
static const char *hi6403es_lineinl_mux_texts[] = {
	"LINEIN1",
	"LINEIN2",
	"LINEOUT",
	"MIC3",
};
static const struct soc_enum hi6403es_lineinl_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_ANALOG_REG20, HI6403ES_LINEINL_SEL_BIT,
		ARRAY_SIZE(hi6403es_lineinl_mux_texts), hi6403es_lineinl_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_lineinl_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_lineinl_mux_enum);

/* LINEIN R MUX */
static const char *hi6403es_lineinr_mux_texts[] = {
	"LINEIN1",
	"LINEIN2",
	"LINEOUT",
	"MIC4",
};
static const struct soc_enum hi6403es_lineinr_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_ANALOG_REG21, HI6403ES_LINEINR_SEL_BIT,
		ARRAY_SIZE(hi6403es_lineinr_mux_texts), hi6403es_lineinr_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_lineinr_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_lineinr_mux_enum);

/* MAINMIC MUX */
static const char *hi6403es_mainmic_mux_texts[] = {
	"HSMIC",
	"MAINMIC",
};
static const struct soc_enum hi6403es_mainmic_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_ANALOG_REG23, HI6403ES_MAINMIC_SEL_BIT,
		ARRAY_SIZE(hi6403es_mainmic_mux_texts), hi6403es_mainmic_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_mainmic_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_mainmic_mux_enum);

/* MADMIC MUX */
static const char *hi6403es_madmic_mux_texts[] = {
	"HSMIC",
	"MAINMIC",
};
static const struct soc_enum hi6403es_madmic_mux_enum =
	SOC_ENUM_SINGLE(HI6403ES_ANALOG_REG84, HI6403ES_MADMIC_SEL_BIT,
		ARRAY_SIZE(hi6403es_madmic_mux_texts), hi6403es_madmic_mux_texts);
static const struct snd_kcontrol_new hi6403es_dapm_madmic_mux_controls =
	SOC_DAPM_ENUM("Mux", hi6403es_madmic_mux_enum);

/* MIXER */

/* DACL MIXER */
static const struct snd_kcontrol_new hi6403es_dacl_mixer_controls[] = {
	SOC_DAPM_SINGLE("S1L SWITCH",
		HI6403ES_DACL_MIXER_SEL_0, HI6403ES_DACL_MIXER_S1L_BIT, 1, 1),
	SOC_DAPM_SINGLE("S2L SWITCH",
		HI6403ES_DACL_MIXER_SEL_0, HI6403ES_DACL_MIXER_S2L_BIT, 1, 1),
	SOC_DAPM_SINGLE("MDM SWITCH",
		HI6403ES_DACL_MIXER_SEL_0, HI6403ES_DACL_MIXER_MDM_BIT, 1, 1),/*fix me*/
	SOC_DAPM_SINGLE("S1R SWITCH",
		HI6403ES_DACL_MIXER_SEL_1, HI6403ES_DACL_MIXER_S1R_BIT, 1, 1),
};

/* DACR MIXER */
static const struct snd_kcontrol_new hi6403es_dacr_mixer_controls[] = {
	SOC_DAPM_SINGLE("S1R SWITCH",
		HI6403ES_DACR_MIXER_SEL_0, HI6403ES_DACR_MIXER_S1R_BIT, 1, 1),
	SOC_DAPM_SINGLE("S2R SWITCH",
		HI6403ES_DACR_MIXER_SEL_0, HI6403ES_DACR_MIXER_S2R_BIT, 1, 1),
	SOC_DAPM_SINGLE("MDM SWITCH",
		HI6403ES_DACR_MIXER_SEL_0, HI6403ES_DACR_MIXER_MDM_BIT, 1, 1),/*fix me*/
	SOC_DAPM_SINGLE("S1L SWITCH",
		HI6403ES_DACR_MIXER_SEL_1, HI6403ES_DACR_MIXER_S1L_BIT, 1, 1),
};

/* DACSL MIXER */
static const struct snd_kcontrol_new hi6403es_dacsl_mixer_controls[] = {
	SOC_DAPM_SINGLE("DACL SWITCH",
		HI6403ES_DACSL_MIXER_SEL_0, HI6403ES_DACSL_MIXER_DACL_BIT, 1, 1),
	SOC_DAPM_SINGLE("SIDETONE SWITCH",
		HI6403ES_DACSL_MIXER_SEL_1, HI6403ES_DACSL_MIXER_SIDETONE_BIT, 1, 1),
	SOC_DAPM_SINGLE("DSP IF8 OL SWITCH",
		HI6403ES_DACSL_MIXER_SEL_0, HI6403ES_DACSL_MIXER_OL_BIT, 1, 1),
	SOC_DAPM_SINGLE("MDM SWITCH",
		HI6403ES_DACSL_MIXER_SEL_0, HI6403ES_DACSL_MIXER_MDM_BIT, 1, 1),
};

/* DACSR MIXER */
static const struct snd_kcontrol_new hi6403es_dacsr_mixer_controls[] = {
	SOC_DAPM_SINGLE("DACR SWITCH",
		HI6403ES_DACSR_MIXER_SEL_0, HI6403ES_DACSR_MIXER_DACR_BIT, 1, 1),
	SOC_DAPM_SINGLE("SIDETONE SWITCH",
		HI6403ES_DACSR_MIXER_SEL_1, HI6403ES_DACSR_MIXER_SIDETONE_BIT, 1, 1),
	SOC_DAPM_SINGLE("DSP IF8 OR SWITCH",
		HI6403ES_DACSR_MIXER_SEL_0, HI6403ES_DACSR_MIXER_OR_BIT, 1, 1),
	SOC_DAPM_SINGLE("MDM SWITCH",
		HI6403ES_DACSR_MIXER_SEL_0, HI6403ES_DACSR_MIXER_MDM_BIT, 1, 1),
};

/* DACL SRC MIXER */
static const struct snd_kcontrol_new hi6403es_dacl_src_mixer_controls[] = {
	SOC_DAPM_SINGLE("DSP IF8 SWITCH",
		HI6403ES_DACSL_SRC_MIXER_SEL, HI6403ES_DACSL_MIXER_0_BIT, 1, 1),
	SOC_DAPM_SINGLE("SIDETONE SWITCH",
		HI6403ES_DACSL_SRC_MIXER_SEL, HI6403ES_DACSL_MIXER_1_BIT, 1, 1),
};

/* DACR SRC MIXER */
static const struct snd_kcontrol_new hi6403es_dacr_src_mixer_controls[] = {
	SOC_DAPM_SINGLE("SIDETONE SWITCH",
		HI6403ES_DACSR_SRC_MIXER_SEL, HI6403ES_DACSR_MIXER_1_BIT, 1, 1),
	SOC_DAPM_SINGLE("DSP IF8 SWITCH",
		HI6403ES_DACSR_SRC_MIXER_SEL, HI6403ES_DACSR_MIXER_0_BIT, 1, 1),
};

/* DACL FLT MIXER */
static const struct snd_kcontrol_new hi6403es_dacl_flt_mixer_controls[] = {
	SOC_DAPM_SINGLE("DACL SRC SWITCH",
		HI6403ES_DACL_FLT_MIXER_SEL, HI6403ES_DACL_FLT_MIXER_0_BIT, 1, 1),
	SOC_DAPM_SINGLE("S1L SWITCH",
		HI6403ES_DACL_FLT_MIXER_SEL, HI6403ES_DACL_FLT_MIXER_1_BIT, 1, 1),
};

/* DACR FLT MIXER */
static const struct snd_kcontrol_new hi6403es_dacr_flt_mixer_controls[] = {
	SOC_DAPM_SINGLE("S1R SWITCH",
		HI6403ES_DACR_FLT_MIXER_SEL, HI6403ES_DACR_FLT_MIXER_1_BIT, 1, 1),
	SOC_DAPM_SINGLE("DACR SRC SWITCH",
		HI6403ES_DACR_FLT_MIXER_SEL, HI6403ES_DACR_FLT_MIXER_0_BIT, 1, 1),
};

/* DSDL MIXER */
/* DSDR MIXER */

static const struct snd_soc_dapm_widget hi6403es_dapm_widgets[] = {
	/* INPUT */
	/* analog input */
	SND_SOC_DAPM_INPUT("LINEINL"),
	SND_SOC_DAPM_INPUT("LINEINR"),
	SND_SOC_DAPM_INPUT("LINEOUTL LOOP"),
	SND_SOC_DAPM_INPUT("LINEOUTR LOOP"),
	SND_SOC_DAPM_INPUT("HSMIC"),
	SND_SOC_DAPM_INPUT("MAINMIC"),
	SND_SOC_DAPM_INPUT("AUXMIC"),
	SND_SOC_DAPM_INPUT("MIC3"),
	SND_SOC_DAPM_INPUT("MIC4"),
	/* digital input */
	SND_SOC_DAPM_INPUT("S1L IN"),
	SND_SOC_DAPM_INPUT("S1R IN"),
	SND_SOC_DAPM_INPUT("S2L IN"),
	SND_SOC_DAPM_INPUT("S2R IN"),
	SND_SOC_DAPM_INPUT("S3L IN"),
	SND_SOC_DAPM_INPUT("S3R IN"),
	SND_SOC_DAPM_INPUT("S4L IN"),
	SND_SOC_DAPM_INPUT("S4R IN"),
	SND_SOC_DAPM_INPUT("DIGMIC0"),
	SND_SOC_DAPM_INPUT("DIGMIC1"),
	/* fake input for slimbus uplink config */
	SND_SOC_DAPM_INPUT("SLIM UP IN"),
	/* fake input for sending ec to AP */
	SND_SOC_DAPM_INPUT("ECL IN"),
	SND_SOC_DAPM_INPUT("ECR IN"),
	/* fake input for capture */
	SND_SOC_DAPM_INPUT("AUDIO IN"),
	/* fake input for voice */
	SND_SOC_DAPM_INPUT("VOICE IN"),
	/* fake input for HP HIGH */
	SND_SOC_DAPM_INPUT("HP HIGH IN"),

	/* OUTPUT */
	/* analog output */
	SND_SOC_DAPM_OUTPUT("EAR OUT"),
	SND_SOC_DAPM_OUTPUT("HPL OUT"),
	SND_SOC_DAPM_OUTPUT("HPR OUT"),
	SND_SOC_DAPM_OUTPUT("LINEOUT L"),
	SND_SOC_DAPM_OUTPUT("LINEOUT R"),
	/* digital output */
	SND_SOC_DAPM_OUTPUT("S1L OUT"),
	SND_SOC_DAPM_OUTPUT("S1R OUT"),
	SND_SOC_DAPM_OUTPUT("S2L OUT"),
	SND_SOC_DAPM_OUTPUT("S2R OUT"),
	SND_SOC_DAPM_OUTPUT("S3L OUT"),
	SND_SOC_DAPM_OUTPUT("S3R OUT"),
	SND_SOC_DAPM_OUTPUT("S4L OUT"),
	SND_SOC_DAPM_OUTPUT("S4R OUT"),
	SND_SOC_DAPM_OUTPUT("U3L OUT"),
	SND_SOC_DAPM_OUTPUT("U4R OUT"),
	/* fake output for sending ec to AP */
	SND_SOC_DAPM_OUTPUT("ECL OUT"),
	SND_SOC_DAPM_OUTPUT("ECR OUT"),
	/* fake output for slimbus uplink config */
	SND_SOC_DAPM_OUTPUT("SLIM UP OUT"),
	/* fake output for mad */
	SND_SOC_DAPM_OUTPUT("MAD OUT"),
	/* fake output for capture */
	SND_SOC_DAPM_OUTPUT("AUDIO OUT"),
	/* fake output for voice */
	SND_SOC_DAPM_OUTPUT("VOICE OUT"),
	/* fake output for HP HIGH */
	SND_SOC_DAPM_OUTPUT("HP HIGH OUT"),

	/* keep for pll test */
	SND_SOC_DAPM_INPUT("PLL IN"),
	SND_SOC_DAPM_OUTPUT("PLL OUT"),
	/* end for pll test */
	/* anc core */
	SND_SOC_DAPM_INPUT("ANC CORE IN"),
	SND_SOC_DAPM_OUTPUT("ANC CORE OUT"),

	/* MIC */
	SND_SOC_DAPM_MIC("HS MIC", hi6403es_hsmic_power_event),
	SND_SOC_DAPM_MIC("MAIN MIC", hi6403es_mainmic_power_event),
	SND_SOC_DAPM_MIC("AUX MIC", hi6403es_auxmic_power_event),
	SND_SOC_DAPM_MIC("MIC3 IN", hi6403es_mic3_power_event),
	SND_SOC_DAPM_MIC("MIC4 IN", hi6403es_mic4_power_event),

	/* SUPPLY */
	SND_SOC_DAPM_SUPPLY_S("PLL CLK",
		0, SND_SOC_NOPM, 0, 0, hi6403es_pll_power_event,
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_SUPPLY_S("MAD CLK",
		0, SND_SOC_NOPM, 0, 0, hi6403es_mad_pll_power_event,
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_SUPPLY_S("DP CLK",
		1, SND_SOC_NOPM, 0, 0, hi6403es_dp_clk_power_event,
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_SUPPLY_S("S2 IF CLK",
		1, HI6403ES_S2_CLK_CFG_REG, HI6403ES_S2_CLKEN_BIT, 0, NULL, 0),
	SND_SOC_DAPM_SUPPLY_S("S2 RX",
		2, SND_SOC_NOPM, 0, 0, hi6403es_s2_rx_power_event,
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_SUPPLY_S("S4 RX",
		2, SND_SOC_NOPM, 0, 0, hi6403es_s4_rx_power_event,
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_SUPPLY_S("CP1",
		3, SND_SOC_NOPM, 0, 0, hi6403es_cp1_power_event,
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_SUPPLY_S("CP2",
		4, SND_SOC_NOPM, 0, 0, hi6403es_cp2_power_event,
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),

	/* OUT DRIVER */
	SND_SOC_DAPM_OUT_DRV("S2 TX",
		HI6403ES_S2_CFG_REG, HI6403ES_S2_TX_EN_BIT, 0,
		NULL, 0),
	SND_SOC_DAPM_OUT_DRV("S4 TX",
		HI6403ES_S4_CFG_REG, HI6403ES_S4_TX_EN_BIT, 0,
		NULL, 0),
	SND_SOC_DAPM_OUT_DRV_E("HPL DRV",
		SND_SOC_NOPM, 0, 0, NULL, 0,
		hi6403es_headphonel_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_OUT_DRV_E("HPR DRV",
		SND_SOC_NOPM, 0, 0, NULL, 0,
		hi6403es_headphoner_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_OUT_DRV_E("EP DRV",
		SND_SOC_NOPM, 0, 0, NULL, 0,
		hi6403es_earphone_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_OUT_DRV_E("LOL DRV",
		SND_SOC_NOPM, 0, 0, NULL, 0,
		hi6403es_lineoutl_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_OUT_DRV_E("LOR DRV",
		SND_SOC_NOPM, 0, 0, NULL, 0,
		hi6403es_lineoutr_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),

	/* SLIMBUS UP */
	SND_SOC_DAPM_OUT_DRV("U1 DRV",
		HI6403ES_SLIM_UP_CLKEN_REG, HI6403ES_SLIM_UP_CLKEN_1_BIT, 0,
		NULL, 0),
	SND_SOC_DAPM_OUT_DRV("U2 DRV",
		HI6403ES_SLIM_UP_CLKEN_REG, HI6403ES_SLIM_UP_CLKEN_2_BIT, 0,
		NULL, 0),
	SND_SOC_DAPM_OUT_DRV("U3 DRV",
		HI6403ES_SLIM_UP_CLKEN_REG, HI6403ES_SLIM_UP_CLKEN_3_BIT, 0,
		NULL, 0),
	SND_SOC_DAPM_OUT_DRV("U4 DRV",
		HI6403ES_SLIM_UP_CLKEN_REG, HI6403ES_SLIM_UP_CLKEN_4_BIT, 0,
		NULL, 0),
	SND_SOC_DAPM_OUT_DRV("U5 DRV",
		HI6403ES_SLIM_UP_CLKEN_REG, HI6403ES_SLIM_UP_CLKEN_5_BIT, 0,
		NULL, 0),
	SND_SOC_DAPM_OUT_DRV("U6 DRV",
		HI6403ES_SLIM_UP_CLKEN_REG, HI6403ES_SLIM_UP_CLKEN_6_BIT, 0,
		NULL, 0),
	SND_SOC_DAPM_OUT_DRV_E("U7 DRV",
		HI6403ES_SLIM_UP_CLKEN_REG, HI6403ES_SLIM_UP_CLKEN_7_BIT, 0, NULL, 0,
		hi6403es_ec_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_OUT_DRV("U8 DRV",
		HI6403ES_SLIM_UP_CLKEN_REG, HI6403ES_SLIM_UP_CLKEN_8_BIT, 0,
		NULL, 0),

	/* ANALOG PGA */
	SND_SOC_DAPM_PGA_S("MAD PGA",
		0, SND_SOC_NOPM, 0, 0,
		hi6403es_mad_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_PGA_S("MAIN PGA",
		0, SND_SOC_NOPM, 0, 0,
		hi6403es_main_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_PGA_S("AUX PGA",
		0, SND_SOC_NOPM, 0, 0,
		hi6403es_aux_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_PGA_S("LINEINL PGA",
		0, SND_SOC_NOPM, 0, 0,
		hi6403es_lineinl_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_PGA_S("LINEINR PGA",
		0, SND_SOC_NOPM, 0, 0,
		hi6403es_lineinr_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),

	/* DIGTAL PGA */
	/* PGA OF S1-S4 */
	SND_SOC_DAPM_PGA_S("S1 IL PGA",
		0, HI6403ES_S1_PORT_CLKEN_REG, HI6403ES_S1_IL_PGA_CLKEN_BIT, 0,
		NULL, 0),
	SND_SOC_DAPM_PGA_S("S1 IR PGA",
		0, HI6403ES_S1_PORT_CLKEN_REG, HI6403ES_S1_IR_PGA_CLKEN_BIT, 0,
		NULL, 0),
	SND_SOC_DAPM_PGA_S("S2 IL PGA",
		0, HI6403ES_S2_PORT_CLKEN_REG, HI6403ES_S2_IL_PGA_CLKEN_BIT, 0,
		NULL, 0),
	SND_SOC_DAPM_PGA_S("S2 IR PGA",
		0, HI6403ES_S2_PORT_CLKEN_REG, HI6403ES_S2_IR_PGA_CLKEN_BIT, 0,
		NULL, 0),
	SND_SOC_DAPM_PGA_S("S3 IL PGA",
		0, HI6403ES_S3_PORT_CLKEN_REG, HI6403ES_S3_IL_PGA_CLKEN_BIT, 0,
		NULL, 0),
	SND_SOC_DAPM_PGA_S("S3 IR PGA",
		0, HI6403ES_S3_PORT_CLKEN_REG, HI6403ES_S3_IR_PGA_CLKEN_BIT, 0,
		NULL, 0),
	/* PGA FOR DAC */
	SND_SOC_DAPM_PGA_S("DACL FLT PGA",
		1, SND_SOC_NOPM, 0, 0, hi6403es_dacl_flt_power_event,
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_PGA_S("DACR FLT PGA",
		1, SND_SOC_NOPM, 0, 0, hi6403es_dacr_flt_power_event,
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_PGA_S("DACL PGA",
		1, SND_SOC_NOPM, 0, 0, hi6403es_dacl_power_event,
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_PGA_S("DACR PGA",
		1, SND_SOC_NOPM, 0, 0, hi6403es_dacr_power_event,
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_PGA_S("DACSL PGA",
		1, SND_SOC_NOPM, 0, 0, hi6403es_dacls_power_event,
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_PGA_S("DACSR PGA",
		1, SND_SOC_NOPM, 0, 0, hi6403es_dacrs_power_event,
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	/* PGA FOR ADC */
	SND_SOC_DAPM_PGA_S("ADC1R",
		1, SND_SOC_NOPM, 0, 0, hi6403es_adc1r_power_event,
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_PGA_S("ADC1L",
		1, SND_SOC_NOPM, 0, 0, hi6403es_adc1l_power_event,
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_PGA_S("ADC0R",
		1, SND_SOC_NOPM, 0, 0, hi6403es_adc0r_power_event,
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_PGA_S("ADC0L",
		1, SND_SOC_NOPM, 0, 0, hi6403es_adc0l_power_event,
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	/* PGA FOR SIDETONE */
	SND_SOC_DAPM_PGA_S("SIDETONE",
		1, SND_SOC_NOPM, 0, 0, hi6403es_sidetone_power_event,
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),

	/* SWITCH FOR S1-S4 OUT */
	SND_SOC_DAPM_SWITCH_E("MAD SWITCH",
		HI6403ES_MAD_CFG_REG, HI6403ES_MAD_EN_BIT, 0, &hi6403es_dapm_mad_switch_controls,
		hi6403es_madswitch_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_SWITCH("S1 OL SWITCH",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_s1_ol_switch_controls),
	SND_SOC_DAPM_SWITCH("S1 OR SWITCH",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_s1_or_switch_controls),
	SND_SOC_DAPM_SWITCH_E("S2 OL SWITCH",
		SND_SOC_NOPM, 0, 0, &hi6403es_dapm_s2_ol_switch_controls,
		hi6403es_s2up_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_SWITCH("S2 OR SWITCH",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_s2_or_switch_controls),
	SND_SOC_DAPM_SWITCH("S3 OL SWITCH",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_s3_ol_switch_controls),
	SND_SOC_DAPM_SWITCH("S3 OR SWITCH",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_s3_or_switch_controls),
	SND_SOC_DAPM_SWITCH("S4 OL SWITCH",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_s4_ol_switch_controls),
	SND_SOC_DAPM_SWITCH("S4 OR SWITCH",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_s4_or_switch_controls),
	/* SWITCH FOR U3 & U4 */
	SND_SOC_DAPM_SWITCH("U3 OL SWITCH",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_u3_ol_switch_controls),
	SND_SOC_DAPM_SWITCH("U4 OR SWITCH",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_u4_or_switch_controls),
	/* SWITCH FOR SLIMBUS UPLINK */
	SND_SOC_DAPM_SWITCH_E("AUDIOUP SWITCH",
		SND_SOC_NOPM, 0, 0, &hi6403es_dapm_audioup_switch_controls,
		hi6403es_audioup_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_SWITCH_E("AUDIOUP 4MIC SWITCH",
		SND_SOC_NOPM, 0, 0, &hi6403es_dapm_audioup_4mic_switch_controls,
		hi6403es_audioup_4mic_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_SWITCH_E("VOICE8K SWITCH",
		SND_SOC_NOPM, 0, 0, &hi6403es_dapm_voice8k_switch_controls,
		hi6403es_voice8k_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_SWITCH_E("VOICE16K SWITCH",
		SND_SOC_NOPM, 0, 0, &hi6403es_dapm_voice16k_switch_controls,
		hi6403es_voice16k_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_SWITCH_E("VOICE32K SWITCH",
		SND_SOC_NOPM, 0, 0, &hi6403es_dapm_voice32k_switch_controls,
		hi6403es_voice32k_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	/* SWITCH FOR SLIMBUS DOWNLINK */
	SND_SOC_DAPM_SWITCH_E("PLAY48K SWITCH",
		SND_SOC_NOPM, 0, 0, &hi6403es_dapm_play48k_switch_controls,
		hi6403es_play48k_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_SWITCH_E("PLAY96K SWITCH",
		SND_SOC_NOPM, 0, 0, &hi6403es_dapm_play96k_switch_controls,
		hi6403es_play96k_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_SWITCH_E("PLAY192K SWITCH",
		SND_SOC_NOPM, 0, 0, &hi6403es_dapm_play192k_switch_controls,
		hi6403es_play192k_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_SWITCH_E("LOWLATENCY SWITCH",
		SND_SOC_NOPM, 0, 0, &hi6403es_dapm_lowlatency_switch_controls,
		hi6403es_lowlatency_play_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	/* SWITCH FOR HPCLASSH CFG */
	SND_SOC_DAPM_SWITCH_E("HPHIGHLEVEL SWITCH",
		SND_SOC_NOPM, 0, 0, &hi6403es_dapm_hp_high_level_switch_controls,
		hi6403es_hp_high_level_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),

	/* keep for pll test */
	SND_SOC_DAPM_SWITCH_E("PLL48K SWITCH",
		SND_SOC_NOPM, 0, 0, &hi6403es_dapm_pll48k_switch_controls,
		hi6403es_pll48k_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_SWITCH_E("PLL44K1 SWITCH",
		SND_SOC_NOPM, 0, 0, &hi6403es_dapm_pll44k1_switch_controls,
		hi6403es_pll44k1_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	SND_SOC_DAPM_SWITCH_E("PLLMAD SWITCH",
		SND_SOC_NOPM, 0, 0, &hi6403es_dapm_pllmad_switch_controls,
		hi6403es_pllmad_power_event, (SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),
	/* end for pll test */

	/* MUX */
	/* MUX 9 */
	SND_SOC_DAPM_MUX("DACL SRC MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_dacl_src_mux_controls),
	/* MUX 10 */
	SND_SOC_DAPM_MUX("DACR SRC MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_dacr_src_mux_controls),
	/* MUX 11 */
	SND_SOC_DAPM_MUX("DACL PGA MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_dacl_pga_mux_controls),
	/* MUX 12 */
	SND_SOC_DAPM_MUX("DACR PGA MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_dacr_pga_mux_controls),
	/* MUX 13 */
	SND_SOC_DAPM_MUX("DACL SDM45 MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_dacl_sdm45_mux_controls),
	/* MUX 14 */
	SND_SOC_DAPM_MUX("DACR SDM45 MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_dacr_sdm45_mux_controls),
	/* MUX 15 */
	SND_SOC_DAPM_MUX("ADC0L MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_adcl0_mux_controls),
	/* MUX 16 */
	SND_SOC_DAPM_MUX("ADC0R MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_adcr0_mux_controls),
	/* MUX 17 */
	SND_SOC_DAPM_MUX("ADC1L MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_adcl1_mux_controls),
	/* MUX 18 */
	SND_SOC_DAPM_MUX("ADC1R MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_adcr1_mux_controls),
	/* MUX 19 */
	SND_SOC_DAPM_MUX("MIC1 MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_mic1_mux_controls),
	/* MUX 20 */
	SND_SOC_DAPM_MUX("MIC2 MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_mic2_mux_controls),
	/* MUX 21 */
	SND_SOC_DAPM_MUX("MIC3 MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_mic3_mux_controls),
	/* MUX 22 */
	SND_SOC_DAPM_MUX("MIC4 MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_mic4_mux_controls),
	/* MUX 23 */
	SND_SOC_DAPM_MUX("BTL MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_btl_mux_controls),
	/* MUX 24 */
	SND_SOC_DAPM_MUX("BTR MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_btr_mux_controls),
	/* MUX 25 */
	SND_SOC_DAPM_MUX("M1L MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_m1l_mux_controls),
	/* MUX 26 */
	SND_SOC_DAPM_MUX("M1R MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_m1r_mux_controls),
	/* MUX 27 */
	SND_SOC_DAPM_MUX("MDM MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_mdm_mux_controls),
	/* MUX 28 */
	SND_SOC_DAPM_MUX("SIDETONE MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_sidetone_mux_controls),
	/* MUX 30 */
	SND_SOC_DAPM_MUX("NEARMIC MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_nearmic_mux_controls),
	/* MUX 31 */
	SND_SOC_DAPM_MUX("PDM MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_pdm_mux_controls),
	/* MUX 41 */
	SND_SOC_DAPM_MUX("DACR SDM MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_dacr_sdm_mux_controls),
	/* MUX 43 */
	SND_SOC_DAPM_MUX("DACSR SDM MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_dacsr_sdm_mux_controls),
	/* MUX 51 */
	SND_SOC_DAPM_MUX("DAC HPL MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_dac_hpl_mux_controls),
	/* MUX 52 */
	SND_SOC_DAPM_MUX("DAC HPR MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_dac_hpr_mux_controls),
	/* MUX 53 */
	SND_SOC_DAPM_MUX("DAC EP MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_dac_ep_mux_controls),
	/* MUX 54 */
	SND_SOC_DAPM_MUX("DAC LINEOUTL MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_dac_ltl_mux_controls),
	/* MUX 55 */
	SND_SOC_DAPM_MUX("DAC LINEOUTR MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_dac_ltr_mux_controls),
	/* MUX 56 */
	SND_SOC_DAPM_MUX("ANC FAREND MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_anc_farend_mux_controls),
	/* MUX 57 */
	SND_SOC_DAPM_MUX("ERROR MIC MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_error_mic_mux_controls),
	/* MUX 58 */
	SND_SOC_DAPM_MUX("REF MIC MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_ref_mic_mux_controls),
	/* MUX 61 */
	SND_SOC_DAPM_MUX("IF8 TEST MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_if8_test_mux_controls),

	/* MIC MUX */
	/* MIC3 MUX */
	SND_SOC_DAPM_MUX("LINEIN L MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_lineinl_mux_controls),
	/* MIC4 MUX */
	SND_SOC_DAPM_MUX("LINEIN R MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_lineinr_mux_controls),
	/* MAINMIC MUX */
	SND_SOC_DAPM_MUX("MAINMIC MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_mainmic_mux_controls),
	/* MADMIC MUX */
	SND_SOC_DAPM_MUX("MADMIC MUX",
		SND_SOC_NOPM, 0, 0,
		&hi6403es_dapm_madmic_mux_controls),

	/* MIXER */
	SND_SOC_DAPM_MIXER("DACL MIXER",
		HI6403ES_DAC_MIXER_CLK_REG, HI6403ES_DACL_MIXER_EN_BIT, 0,
		hi6403es_dacl_mixer_controls,
		ARRAY_SIZE(hi6403es_dacl_mixer_controls)),
	SND_SOC_DAPM_MIXER("DACR MIXER",
		HI6403ES_DAC_MIXER_CLK_REG, HI6403ES_DACR_MIXER_EN_BIT, 0,
		hi6403es_dacr_mixer_controls,
		ARRAY_SIZE(hi6403es_dacr_mixer_controls)),
	SND_SOC_DAPM_MIXER("DACL SRC MIXER",
		HI6403ES_DAC_SRC_MIXER_CLK_REG, HI6403ES_DACL_SRC_MIXER_EN_BIT, 0,
		hi6403es_dacl_src_mixer_controls,
		ARRAY_SIZE(hi6403es_dacl_src_mixer_controls)),
	SND_SOC_DAPM_MIXER("DACR SRC MIXER",
		HI6403ES_DAC_SRC_MIXER_CLK_REG, HI6403ES_DACR_SRC_MIXER_EN_BIT, 0,
		hi6403es_dacr_src_mixer_controls,
		ARRAY_SIZE(hi6403es_dacr_src_mixer_controls)),
	SND_SOC_DAPM_MIXER("DACSL MIXER",
		HI6403ES_DAC_MIXER_CLK_REG, HI6403ES_DACSL_MIXER_EN_BIT, 0,
		hi6403es_dacsl_mixer_controls,
		ARRAY_SIZE(hi6403es_dacsl_mixer_controls)),
	SND_SOC_DAPM_MIXER("DACSR MIXER",
		HI6403ES_DAC_MIXER_CLK_REG, HI6403ES_DACSR_MIXER_EN_BIT, 0,
		hi6403es_dacsr_mixer_controls,
		ARRAY_SIZE(hi6403es_dacsr_mixer_controls)),
	SND_SOC_DAPM_MIXER("DACL FLT MIXER",
		HI6403ES_DAC_SRC_MIXER_CLK_REG, HI6403ES_DACL_FLT_MIXER_EN_BIT, 0,
		hi6403es_dacl_flt_mixer_controls,
		ARRAY_SIZE(hi6403es_dacl_flt_mixer_controls)),
	SND_SOC_DAPM_MIXER("DACR FLT MIXER",
		HI6403ES_DAC_SRC_MIXER_CLK_REG, HI6403ES_DACR_FLT_MIXER_EN_BIT, 0,
		hi6403es_dacr_flt_mixer_controls,
		ARRAY_SIZE(hi6403es_dacr_flt_mixer_controls)),
};

static const struct snd_soc_dapm_route route_map[] = {
	/* keep for pll test */
	{"PLL OUT",			NULL,			"PLL48K SWITCH"},
	{"PLL OUT",			NULL,			"PLL44K1 SWITCH"},
	{"PLL OUT",			NULL,			"PLLMAD SWITCH"},
	{"PLL48K SWITCH",	"SWITCH",		"PLL IN"},
	{"PLL44K1 SWITCH",	"SWITCH",		"PLL IN"},
	{"PLLMAD SWITCH",	"SWITCH",		"PLL IN"},
	/* end for pll test */

	{"S1 IL PGA",		NULL,			"S1L IN"},
	{"S1 IR PGA",		NULL,			"S1R IN"},
	{"S2 IL PGA",		NULL,			"S2L IN"},
	{"S2 IR PGA",		NULL,			"S2R IN"},
	{"S3 IL PGA",		NULL,			"S3L IN"},
	{"S3 IR PGA",		NULL,			"S3R IN"},


	{"AUDIO OUT",		NULL,			"PLAY48K SWITCH"},
	{"AUDIO OUT",		NULL,			"PLAY96K SWITCH"},
	{"AUDIO OUT",		NULL,			"PLAY192K SWITCH"},
	{"AUDIO OUT",		NULL,			"LOWLATENCY SWITCH"},
	{"PLAY48K SWITCH",		"SWITCH",		"AUDIO IN"},
	{"PLAY96K SWITCH",		"SWITCH",		"AUDIO IN"},
	{"PLAY192K SWITCH",		"SWITCH",		"AUDIO IN"},
	{"LOWLATENCY SWITCH",		"SWITCH",		"AUDIO IN"},


	{"MAD OUT",		NULL,			"MAD SWITCH"},


	{"S1L OUT",		NULL,			"U1 DRV"},
	{"S1R OUT",		NULL,			"U2 DRV"},
	{"U3L OUT",		NULL,			"U3 DRV"},
	{"U4R OUT",		NULL,			"U4 DRV"},
	{"S2L OUT",		NULL,			"S2 TX"},
	{"S2R OUT",		NULL,			"S2 TX"},
	{"S3L OUT",		NULL,			"U5 DRV"},
	{"S3R OUT",		NULL,			"U6 DRV"},
	{"S4L OUT",		NULL,			"S4 TX"},
	{"S4R OUT",		NULL,			"S4 TX"},

	{"ECL OUT",		NULL,			"U7 DRV"},
	{"ECR OUT",		NULL,			"U8 DRV"},

	{"AUDIO OUT",		NULL,			"AUDIOUP SWITCH"},
	{"AUDIO OUT",		NULL,			"AUDIOUP 4MIC SWITCH"},
	{"VOICE OUT",		NULL,			"VOICE8K SWITCH"},
	{"AUDIOUP SWITCH",		"SWITCH",		"AUDIO IN"},
	{"AUDIOUP 4MIC SWITCH",	"SWITCH",		"AUDIO IN"},
	{"VOICE8K SWITCH",		"SWITCH",		"VOICE IN"},

	{"VOICE OUT",		NULL,			"VOICE32K SWITCH"},
	{"VOICE OUT",		NULL,			"VOICE16K SWITCH"},
	{"VOICE32K SWITCH",	"SWITCH",		"VOICE IN"},
	{"VOICE16K SWITCH",	"SWITCH",		"VOICE IN"},

	{"HP HIGH OUT",		NULL,			"HPHIGHLEVEL SWITCH"},
	{"HPHIGHLEVEL SWITCH",	"SWITCH",		"HP HIGH IN"},

	{"VOICE8K SWITCH",      NULL,                   "PLL CLK"},
	{"VOICE16K SWITCH",     NULL,                   "PLL CLK"},
	{"VOICE32K SWITCH",     NULL,                   "PLL CLK"},
	{"AUDIOUP 4MIC SWITCH", NULL,                   "PLL CLK"},
	{"AUDIOUP SWITCH",      NULL,                   "PLL CLK"},
	{"PLAY48K SWITCH",     NULL,                   "PLL CLK"},
	{"PLAY96K SWITCH", NULL,               "PLL CLK"},
	{"PLAY192K SWITCH",      NULL,        "PLL CLK"},
	{"LOWLATENCY SWITCH",      NULL,        "PLL CLK"},

	{"U7 DRV",              NULL,                   "PLL CLK"},


	{"U1 DRV",		NULL,			"S1 OL SWITCH"},
	{"U2 DRV",		NULL,			"S1 OR SWITCH"},
	{"S2 TX",		NULL,			"S2 OL SWITCH"},
	{"S2 TX",		NULL,			"S2 OR SWITCH"},
	{"U3 DRV",		NULL,			"U3 OL SWITCH"},
	{"U4 DRV",		NULL,			"U4 OR SWITCH"},
	{"U5 DRV",		NULL,			"S3 OL SWITCH"},
	{"U6 DRV",		NULL,			"S3 OR SWITCH"},
	{"S4 TX",		NULL,			"S4 OL SWITCH"},
	{"S4 TX",		NULL,			"S4 OR SWITCH"},

	{"U7 DRV",		NULL,			"S4 OL SWITCH"},
	{"U8 DRV",		NULL,			"S4 OR SWITCH"},

	/* CLK SUPPLY */
	{"S1L IN",		NULL,			"DP CLK"},
	{"S1R IN",		NULL,			"DP CLK"},
	{"S1L IN",		NULL,			"PLL CLK"},
	{"S1R IN",		NULL,			"PLL CLK"},

	{"S2L IN",		NULL,			"S2 RX"},
	{"S2R IN",		NULL,			"S2 RX"},
	{"S2 RX",		NULL,			"DP CLK"},
	{"S2 RX",		NULL,			"S2 IF CLK"},
	{"S2 TX",		NULL,			"S2 IF CLK"},
	{"S2 RX",		NULL,			"PLL CLK"},

	{"S3L IN",		NULL,			"DP CLK"},
	{"S3R IN",		NULL,			"DP CLK"},
	{"S3L IN",		NULL,			"PLL CLK"},
	{"S3R IN",		NULL,			"PLL CLK"},

	{"S4L IN",		NULL,			"S4 RX"},
	{"S4R IN",		NULL,			"S4 RX"},
	{"S4L OUT",		NULL,			"S4 RX"},
	{"S4R OUT",		NULL,			"S4 RX"},
	{"S4 RX",		NULL,			"DP CLK"},
	{"S4 RX",		NULL,			"PLL CLK"},

	{"S1L OUT",		NULL,			"DP CLK"},
	{"S1R OUT",		NULL,			"DP CLK"},
	{"S2L OUT",		NULL,			"DP CLK"},
	{"S2R OUT",		NULL,			"DP CLK"},
	{"S3L OUT",		NULL,			"DP CLK"},
	{"S3R OUT",		NULL,			"DP CLK"},
	{"S4L OUT",		NULL,			"DP CLK"},
	{"S4R OUT",		NULL,			"DP CLK"},
	{"U3L OUT",		NULL,			"DP CLK"},
	{"U4R OUT",		NULL,			"DP CLK"},

	{"MAD PGA",		NULL,			"MAD CLK"},
	{"MAIN PGA",	NULL,			"PLL CLK"},
	{"AUX PGA",		NULL,			"PLL CLK"},
	{"LINEINL PGA",	NULL,			"PLL CLK"},
	{"LINEINR PGA",	NULL,			"PLL CLK"},

	{"MAD OUT",		NULL,			"MAD CLK"},
	{"MAD OUT",		NULL,			"DP CLK"},

	{"ECL OUT",		NULL,			"DP CLK"},
	{"ECR OUT",		NULL,			"DP CLK"},
	{"ECL OUT",		NULL,			"PLL CLK"},
	{"ECR OUT",		NULL,			"PLL CLK"},

	{"MAD SWITCH",		"SWITCH",		"MIC1 MUX"},
	{"S1 OL SWITCH",	"SWITCH",		"MIC1 MUX"},
	{"S1 OR SWITCH",	"SWITCH",		"MIC2 MUX"},
	{"S2 OL SWITCH",	"SWITCH",		"BTL MUX"},
	{"S2 OR SWITCH",	"SWITCH",		"BTR MUX"},
	{"S3 OL SWITCH",	"SWITCH",		"MIC3 MUX"},
	{"S3 OR SWITCH",	"SWITCH",		"MIC4 MUX"},
	{"S4 OL SWITCH",	"SWITCH",		"M1L MUX"},
	{"S4 OR SWITCH",	"SWITCH",		"M1R MUX"},

	{"U3 OL SWITCH",	"SWITCH",		"MIC1 MUX"},
	{"U4 OR SWITCH",	"SWITCH",		"MIC2 MUX"},


	/* DAC MIXER */
	{"DACL MIXER",		"S1L SWITCH",		"S1 IL PGA"},
	{"DACL MIXER",		"S2L SWITCH",		"S2 IL PGA"},
	{"DACL MIXER",		"MDM SWITCH",		"MDM MUX"},
	{"DACL MIXER",		"S1R SWITCH",		"S1 IR PGA"},

	{"DACR MIXER",		"S1R SWITCH",		"S1 IR PGA"},
	{"DACR MIXER",		"S2R SWITCH",		"S2 IR PGA"},
	{"DACR MIXER",		"MDM SWITCH",		"MDM MUX"},
	{"DACR MIXER",		"S1L SWITCH",		"S1 IL PGA"},

	/* DAC SRC MIXER MUX */
	{"DACL SRC MUX",		"DACL",			"DACL MIXER"},
	{"DACL SRC MUX",		"DSP IF8 OL",	"IF8 TEST MUX"},

	{"DACR SRC MUX",		"DSP IF8 OR",	"IF8 TEST MUX"},
	{"DACR SRC MUX",		"DACR",			"DACR MIXER"},

	/* DSP IF8 MUX */

	/* DAC SRC MIXER */
	{"DACL SRC MIXER",	"DSP IF8 SWITCH",	"DACL SRC MUX"},
	{"DACL SRC MIXER",	"SIDETONE SWITCH",	"SIDETONE"},

	{"DACR SRC MIXER",	"SIDETONE SWITCH",	"SIDETONE"},
	{"DACR SRC MIXER",	"DSP IF8 SWITCH",	"DACR SRC MUX"},

	/* DACS MIXER */
	{"DACSL MIXER",		"DACL SWITCH",		"DACL MIXER"},
	{"DACSL MIXER",		"SIDETONE SWITCH",	"SIDETONE"},
	{"DACSL MIXER",		"DSP IF8 OL SWITCH",	"DACL MIXER"},
	{"DACSL MIXER",		"MDM SWITCH",		"MDM MUX"},

	{"DACSR MIXER",		"DACR SWITCH",		"DACR MIXER"},
	{"DACSR MIXER",		"SIDETONE SWITCH",	"SIDETONE"},
	{"DACSR MIXER",		"DSP IF8 OR SWITCH",	"DACR MIXER"},
	{"DACSR MIXER",		"MDM SWITCH",		"MDM MUX"},

	/* DAC FLT MIXER */
	{"DACL FLT MIXER",	"DACL SRC SWITCH",	"DACL SRC MIXER"},
	{"DACL FLT MIXER",	"S1L SWITCH",		"S1 IL PGA"},

	{"DACR FLT MIXER",	"DACR SRC SWITCH",	"DACR SRC MIXER"},
	{"DACR FLT MIXER",	"S1R SWITCH",		"S1 IR PGA"},

	/* DAC PGA */
	{"DACL FLT PGA",	NULL,			"DACL FLT MIXER"},
	{"DACR FLT PGA",	NULL,			"DACR FLT MIXER"},

	{"DACSL PGA",		NULL,			"DACSL MIXER"},
	{"DACSR PGA",		NULL,			"DACSR MIXER"},

	/* DACR PGA MUX */
	{"DACL PGA MUX",	"DACL_384",		"DACL FLT PGA"},
	{"DACL PGA MUX",	"ADC1L",		"ADC1L"},

	/* DACR PGA MUX */
	{"DACR PGA MUX",	"DACR_384",		"DACR FLT PGA"},
	{"DACR PGA MUX",	"ADC1R",		"ADC1R"},

	{"DACL PGA",		NULL,			"DACL PGA MUX"},
	{"DACR PGA",		NULL,			"DACR PGA MUX"},

	{"DACL FLT PGA",	NULL,			"DP CLK"},
	{"DACR FLT PGA",	NULL,			"DP CLK"},
	{"DACL PGA",		NULL,			"DP CLK"},
	{"DACR PGA",		NULL,			"DP CLK"},
	{"DACSL PGA",		NULL,			"DP CLK"},
	{"DACSR PGA",		NULL,			"DP CLK"},
	{"DACL FLT PGA",	NULL,			"PLL CLK"},
	{"DACR FLT PGA",	NULL,			"PLL CLK"},
	{"DACL PGA",		NULL,			"PLL CLK"},
	{"DACR PGA",		NULL,			"PLL CLK"},
	{"DACSL PGA",		NULL,			"PLL CLK"},
	{"DACSR PGA",		NULL,			"PLL CLK"},

	/* HPL MUX */
	{"DAC HPL MUX",		"DACL SDM45",		"DACL PGA"},
	{"DAC HPL MUX",		"DACR SDM45",		"DACR PGA"},

	/* HPR MUX */
	{"DAC HPR MUX",		"DACL SDM45",		"DACL PGA"},
	{"DAC HPR MUX",		"DACR SDM45",		"DACR PGA"},

	/* EP MUX */
	{"DAC EP MUX",		"DACL SDM",		"DACL PGA"},
	{"DAC EP MUX",		"DACSL",		"DACSL PGA"},
	{"DAC EP MUX",		"DACR SDM",		"DACR PGA"},
	{"DAC EP MUX",		"DACSR",		"DACSR PGA"},

	/* LOL MUX */
	{"DAC LINEOUTL MUX",	"DACL SDM",		"DACL PGA"},
	{"DAC LINEOUTL MUX",	"DACSL",		"DACSL PGA"},
	{"DAC LINEOUTL MUX",	"DACR SDM",		"DACR PGA"},
	{"DAC LINEOUTL MUX",	"DACSR",		"DACSR PGA"},

	/* LOR MUX */
	{"DAC LINEOUTR MUX",	"DACL SDM",		"DACL PGA"},
	{"DAC LINEOUTR MUX",	"DACSL",		"DACSL PGA"},
	{"DAC LINEOUTR MUX",	"DACR SDM",		"DACR PGA"},
	{"DAC LINEOUTR MUX",	"DACSR",		"DACSR PGA"},

	/*PDM MUX */

	/* OUT DRIVER */
	{"HPL DRV",		NULL,			"DAC HPL MUX"},
	{"HPR DRV",		NULL,			"DAC HPR MUX"},

	{"EP DRV",		NULL,			"DAC EP MUX"},

	{"LOL DRV",		NULL,			"DAC LINEOUTL MUX"},
	{"LOR DRV",		NULL,			"DAC LINEOUTR MUX"},

	/* OUTPUT */
	{"EAR OUT",		NULL,			"EP DRV"},
	{"HPL OUT",		NULL,			"HPL DRV"},
	{"HPR OUT",		NULL,			"HPR DRV"},
	{"LINEOUT L",		NULL,			"LOL DRV"},
	{"LINEOUT R",		NULL,			"LOR DRV"},

	{"HPL DRV",		NULL,			"CP2"},
	{"HPR DRV",		NULL,			"CP2"},
	{"EP DRV",		NULL,			"CP1"},

	{"CP2",			NULL,			"CP1"},
	{"CP1",			NULL,			"PLL CLK"},

	/* SIDETONE MUX */
	{"SIDETONE MUX",	"S1L",			"S1 IL PGA"},
	{"SIDETONE MUX",	"S2L",			"S2 IL PGA"},
	{"SIDETONE MUX",	"ADC0L",		"ADC0L"},
	{"SIDETONE MUX",	"ADC0R",		"ADC0R"},
	{"SIDETONE MUX",	"ADC1L",		"ADC1L"},
	{"SIDETONE MUX",	"ADC1R",		"ADC1R"},

	/* MDM MUX */
	{"MDM MUX",		"S3L",			"S3 IL PGA"},

	/* SIDE PGA */
	{"SIDETONE",		NULL,			"SIDETONE MUX"},

	/* MIC1 MUX */
	{"MIC1 MUX",		"ADC0L",		"ADC0L"},
	{"MIC1 MUX",		"ADC1L",		"ADC1L"},
	{"MIC1 MUX",		"ADC0R",		"ADC0R"},
	{"MIC1 MUX",		"ADC1R",		"ADC1R"},
	{"MIC1 MUX",		"S2L",			"S2 IL PGA"},
	{"MIC1 MUX",		"S3L",			"S3 IL PGA"},
	{"MIC1 MUX",		"S4L",			"S4L IN"},
	{"MIC1 MUX",		"DACL_48",		"DACL SRC MIXER"},

	/* MIC2 MUX */
	{"MIC2 MUX",		"ADC0L",		"ADC0L"},
	{"MIC2 MUX",		"ADC1L",		"ADC1L"},
	{"MIC2 MUX",		"ADC0R",		"ADC0R"},
	{"MIC2 MUX",		"ADC1R",		"ADC1R"},
	{"MIC2 MUX",		"S2R",			"S2 IR PGA"},
	{"MIC2 MUX",		"S3R",			"S3 IR PGA"},
	{"MIC2 MUX",		"S4R",			"S4R IN"},
	{"MIC2 MUX",		"DACR_48",		"DACR SRC MIXER"},

	/* MIC3 MUX */
	{"MIC3 MUX",		"ADC0L",		"ADC0L"},
	{"MIC3 MUX",		"ADC1L",		"ADC1L"},
	{"MIC3 MUX",		"ADC0R",		"ADC0R"},
	{"MIC3 MUX",		"ADC1R",		"ADC1R"},
	{"MIC3 MUX",		"S1L",			"S1 IL PGA"},
	{"MIC3 MUX",		"S2L",			"S2 IL PGA"},
	{"MIC3 MUX",		"S4L",			"S4L IN"},
	{"MIC3 MUX",		"DACL_48",		"DACR SRC MIXER"},

	/* MIC4 MUX */
	{"MIC4 MUX",		"ADC0L",		"ADC0L"},
	{"MIC4 MUX",		"ADC1L",		"ADC1L"},
	{"MIC4 MUX",		"ADC0R",		"ADC0R"},
	{"MIC4 MUX",		"ADC1R",		"ADC1R"},
	{"MIC4 MUX",		"S1R",			"S1 IR PGA"},
	{"MIC4 MUX",		"S2R",			"S2 IR PGA"},
	{"MIC4 MUX",		"S4R",			"S4R IN"},
	{"MIC4 MUX",		"DACR_48",		"DACR SRC MIXER"},

	/* BTL MUX */
	{"BTL MUX",		"ADC0L",		"ADC0L"},
	{"BTL MUX",		"ADC1L",		"ADC1L"},
	{"BTL MUX",		"DACL_48",		"DACL SRC MIXER"},
	{"BTL MUX",		"S1L",			"S1 IL PGA"},
	{"BTL MUX",		"S3L",			"S3 IL PGA"},
	{"BTL MUX",		"S4L",			"S4L IN"},

	/* BTR MUX */
	{"BTR MUX",		"ADC0R",		"ADC0R"},
	{"BTR MUX",		"ADC1R",		"ADC1R"},
	{"BTR MUX",		"DACR_48",		"DACR SRC MIXER"},
	{"BTR MUX",		"S1R",			"S1 IR PGA"},
	{"BTR MUX",		"S3R",			"S3 IR PGA"},
	{"BTR MUX",		"S4R",			"S4L IN"},

	/* M1L MUX */
	{"M1L MUX",		"ADC0L",		"ADC0L"},
	{"M1L MUX",		"ADC1L",		"ADC1L"},
	{"M1L MUX",		"DACL_48",		"DACL SRC MIXER"},
	{"M1L MUX",		"DACL_48",		"ECL IN"},
	{"M1L MUX",		"DACSL",		"DACSL MIXER"},
	{"M1L MUX",		"S1L",			"S1 IL PGA"},
	{"M1L MUX",		"S2L",			"S2 IL PGA"},
	{"M1L MUX",		"S3L",			"S3 IL PGA"},

	/* M1R MUX */
	{"M1R MUX",		"ADC0R",		"ADC0R"},
	{"M1R MUX",		"ADC1R",		"ADC1R"},
	{"M1R MUX",		"DACR_48",		"DACR SRC MIXER"},
	{"M1R MUX",		"DACR_48",		"ECR IN"},
	{"M1R MUX",		"DACSR",		"DACSR MIXER"},
	{"M1R MUX",		"S1R",			"S1 IR PGA"},
	{"M1R MUX",		"S2R",			"S2 IR PGA"},
	{"M1R MUX",		"S3R",			"S3 IR PGA"},

	/* ADC */
	{"ADC0L",		NULL,			"ADC0L MUX"},
	{"ADC0R",		NULL,			"ADC0R MUX"},
	{"ADC1L",		NULL,			"ADC1L MUX"},
	{"ADC1R",		NULL,			"ADC1R MUX"},

	/* ADC MUX */
	{"ADC0L MUX",		"MAINMIC",		"MAIN PGA"},
	{"ADC0L MUX",		"AUXMIC",		"AUX PGA"},
	{"ADC0L MUX",		"LINEINL",		"LINEINL PGA"},
	{"ADC0L MUX",		"LINEINR",		"LINEINR PGA"},
	{"ADC0L MUX",		"DMIC0L",		"DIGMIC0"},
	{"ADC0L MUX",		"DMIC0R",		"DIGMIC0"},
	{"ADC0L MUX",		"DMIC1L",		"DIGMIC1"},
	{"ADC0L MUX",		"DMIC1R",		"DIGMIC1"},
	{"ADC0L MUX",		"MAD",			"MAD PGA"},

	{"ADC0R MUX",		"MAINMIC",		"MAIN PGA"},
	{"ADC0R MUX",		"AUXMIC",		"AUX PGA"},
	{"ADC0R MUX",		"LINEINL",		"LINEINL PGA"},
	{"ADC0R MUX",		"LINEINR",		"LINEINR PGA"},
	{"ADC0R MUX",		"DMIC0L",		"DIGMIC0"},
	{"ADC0R MUX",		"DMIC0R",		"DIGMIC0"},
	{"ADC0R MUX",		"DMIC1L",		"DIGMIC1"},
	{"ADC0R MUX",		"DMIC1R",		"DIGMIC1"},
	{"ADC0R MUX",		"MAD",			"MAD PGA"},

	{"ADC1L MUX",		"MAINMIC",		"MAIN PGA"},
	{"ADC1L MUX",		"AUXMIC",		"AUX PGA"},
	{"ADC1L MUX",		"LINEINL",		"LINEINL PGA"},
	{"ADC1L MUX",		"LINEINR",		"LINEINR PGA"},
	{"ADC1L MUX",		"DMIC0L",		"DIGMIC0"},
	{"ADC1L MUX",		"DMIC0R",		"DIGMIC0"},
	{"ADC1L MUX",		"DMIC1L",		"DIGMIC1"},
	{"ADC1L MUX",		"DMIC1R",		"DIGMIC1"},
	{"ADC1L MUX",		"MAD",			"MAD PGA"},

	{"ADC1R MUX",		"MAINMIC",		"MAIN PGA"},
	{"ADC1R MUX",		"AUXMIC",		"AUX PGA"},
	{"ADC1R MUX",		"LINEINL",		"LINEINL PGA"},
	{"ADC1R MUX",		"LINEINR",		"LINEINR PGA"},
	{"ADC1R MUX",		"DMIC0L",		"DIGMIC0"},
	{"ADC1R MUX",		"DMIC0R",		"DIGMIC0"},
	{"ADC1R MUX",		"DMIC1L",		"DIGMIC1"},
	{"ADC1R MUX",		"DMIC1R",		"DIGMIC1"},
	{"ADC1R MUX",		"MAD",			"MAD PGA"},

	{"MAD PGA",			NULL,			"MADMIC MUX"},
	{"MAIN PGA",		NULL,			"MAINMIC MUX"},
	{"AUX PGA",			NULL,			"AUXMIC"},
	{"LINEINL PGA",		NULL,			"LINEIN L MUX"},
	{"LINEINR PGA",		NULL,			"LINEIN R MUX"},

	{"MADMIC MUX",		"MAINMIC",		"MAINMIC"},
	{"MADMIC MUX",		"HSMIC",		"HSMIC"},

	{"MAINMIC MUX",		"MAINMIC",		"MAINMIC"},
	{"MAINMIC MUX",		"HSMIC",		"HSMIC"},

	{"LINEIN L MUX",	"LINEIN1",		"LINEINL"},
	{"LINEIN L MUX",	"LINEIN2",		"LINEINR"},
	{"LINEIN L MUX",	"LINEOUT",		"LINEOUTL LOOP"},
	{"LINEIN L MUX",	"MIC3",			"MIC3"},

	{"LINEIN R MUX",	"LINEIN1",		"LINEINL"},
	{"LINEIN R MUX",	"LINEIN2",		"LINEINR"},
	{"LINEIN R MUX",	"LINEOUT",		"LINEOUTR LOOP"},
	{"LINEIN R MUX",	"MIC4",			"MIC4"},

	/* ANA IN */
	{"MAINMIC",		NULL,			"MAIN MIC"},
	{"HSMIC",		NULL,			"HS MIC"},
	{"AUXMIC",		NULL,			"AUX MIC"},
	{"MIC3",		NULL,			"MIC3 IN"},
	{"MIC4",		NULL,			"MIC4 IN"},

	/* ANC INPUT DATA */
	{"ANC CORE OUT",	NULL,	"ANC FAREND MUX"},
	{"ANC CORE OUT",	NULL,	"ERROR MIC MUX"},
	{"ANC CORE OUT",	NULL,	"REF MIC MUX"},
	/* ANC FAREND MUX */
	{"ANC FAREND MUX",	"DACSL_384",		"DACSL PGA"},
	{"ANC FAREND MUX",	"DACSR_384",		"DACSR PGA"},

	/* ERROR MIC MUX */
	{"ERROR MIC MUX",	"L0",			"ANC CORE IN"},
	{"ERROR MIC MUX",	"R0",			"ANC CORE IN"},
	{"ERROR MIC MUX",	"L1",			"ANC CORE IN"},
	{"ERROR MIC MUX",	"R1",			"ANC CORE IN"},

	/* REF MIC MUX */
	{"REF MIC MUX",	"L0",			"ANC CORE IN"},
	{"REF MIC MUX",	"R0",			"ANC CORE IN"},
	{"REF MIC MUX",	"L1",			"ANC CORE IN"},
	{"REF MIC MUX",	"R1",			"ANC CORE IN"},
};

static void hi6403es_dump_reg(char *buf, unsigned int dump_size)
{
	int len = 0;
	unsigned int i = 0;

	if (NULL == buf) {
		pr_err("hi6403es_dump_reg buf NULL!\n");
		return;
	}

	len = strlen(buf);
	for (i = BASE_ADDR_PAGE_IO + 0x00; i <= BASE_ADDR_PAGE_IO + 0x07c; i += 4)
	{
		len += snprintf(buf + len, dump_size - len, "%#04x-%#04x\n", CODEC_BASE_ADDR | i, snd_soc_read(g_hi6403es_codec, i));
		if (((unsigned int)len) >= dump_size)
			return;
	}

	for (i = BASE_ADDR_PAGE_CFG + 0x00; i <= BASE_ADDR_PAGE_CFG + 0xff; i++)
	{
		if ((i >= BASE_ADDR_PAGE_CFG + 0x41) && (i <= BASE_ADDR_PAGE_CFG + 0x4c)) {
			i = BASE_ADDR_PAGE_CFG + 0x4c;
			continue;
		}
		len += snprintf(buf + len, dump_size - len, "%#04x-%#04x\n", CODEC_BASE_ADDR | i, snd_soc_read(g_hi6403es_codec, i));
		if (((unsigned int)len) >= dump_size)
			return;
	}

	for (i = BASE_ADDR_PAGE_CFG + 0x41; i <= BASE_ADDR_PAGE_CFG + 0x4c; i++)
	{
		len += snprintf(buf + len, dump_size - len, "%#04x-%#04x\n", CODEC_BASE_ADDR | i, snd_soc_read(g_hi6403es_codec, i));
		if (((unsigned int)len) >= dump_size)
			return;
	}

	for (i = BASE_ADDR_PAGE_DIG + 0x00; i <= BASE_ADDR_PAGE_DIG + 0x3FF; i++)
	{
		len += snprintf(buf + len, dump_size - len, "%#04x-%#04x\n", CODEC_BASE_ADDR | i, snd_soc_read(g_hi6403es_codec, i));
		if (((unsigned int)len) >= dump_size)
			return;
	}
	len = strlen(buf);
	snprintf(buf + len, dump_size - len, "\n");

}

static int hi6403es_audio_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct hi6403es_platform_data *priv = NULL;

	int rate = 0;
	int ret = 0;

	BUG_ON(NULL == codec);

	priv = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == priv);

	rate = params_rate(params);

	switch (rate) {
	case 8000:
	case 11250:
	case 16000:
	case 22500:
	case 32000:
	case 44100:
	case 48000:
		break;
	case 88200:
	case 96000:
	case 192000:
		break;
	default:
		pr_err("unknown rate : %d!\n", rate);
		ret = -1;
		break;
	}

	return ret;
}

static int hi6403es_audio_hw_free(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);
	int ret = 0;

	BUG_ON(NULL == priv);

	return ret;
}

struct snd_soc_dai_ops hi6403es_audio_dai_ops = {
	.hw_params = hi6403es_audio_hw_params,
	.hw_free = hi6403es_audio_hw_free,
};

static int hi6403es_voice_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct hi6403es_platform_data *priv = NULL;
	int ret = 0;
	int rate = 0;

	BUG_ON(NULL == codec);

	priv = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == priv);

	rate = params_rate(params);

	switch (rate) {
	case 8000:
		break;
	case 16000:
		break;
	case 32000:
		break;
	default:
		pr_err("unknown rate : %d!\n", rate);
		ret = -1;
		break;
	}

	return ret;
}

static int hi6403es_voice_hw_free(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);
	int ret = 0;

	BUG_ON(NULL == priv);

	hi64xx_update_bits(codec, HI6403ES_S3_CLK_CFG_REG1, 0xC4, 0);

	pr_info("free voice\n");
	return ret;
}

struct snd_soc_dai_ops hi6403es_voice_dai_ops = {
	.hw_params = hi6403es_voice_hw_params,
	.hw_free = hi6403es_voice_hw_free,
};

struct snd_soc_dai_driver hi6403es_dai[] = {
	{
		.name = "hi6403es-audio-dai",
		.playback = {
			.stream_name = "Playback",
			.channels_min = 2,
			.channels_max = 2,
			.rates = HI6403ES_RATES,
			.formats = HI6403ES_FORMATS},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 6,
			.rates = HI6403ES_RATES,
			.formats = HI6403ES_FORMATS},
		.ops = &hi6403es_audio_dai_ops,
	},
	{
		.name = "hi6403es-voice-dai",
		.playback = {
			.stream_name = "Down",
			.channels_min = 1,
			.channels_max = 2,
			.rates = HI6403ES_RATES,
			.formats = HI6403ES_FORMATS},
		.capture = {
			.stream_name = "Up",
			.channels_min = 1,
			.channels_max = 6,
			.rates = HI6403ES_RATES,
			.formats = HI6403ES_FORMATS},
		.ops = &hi6403es_voice_dai_ops,
	},
	{
		.name = "hi6403es-fm-dai",
		.playback = {
			.stream_name = "FM",
			.channels_min = 1,
			.channels_max = 2,
			.rates = HI6403ES_RATES,
			.formats = HI6403ES_FORMATS},
	},
};

int hi6403es_pll48k_config(struct snd_soc_codec *codec)
{
	snd_soc_write(codec, HI6403ES_MAIN1_PLL_CFG_REG2, 0x7E);
	snd_soc_write(codec, HI6403ES_MAIN1_PLL_DIV_REG, 0x14);
	snd_soc_write(codec, HI6403ES_MAIN1_PLL_CFG_REG3, 0x4F);
	snd_soc_write(codec, HI6403ES_MAIN1_PLL_WORD_REG1, 0x5C);
	snd_soc_write(codec, HI6403ES_MAIN1_PLL_WORF_REG2, 0x24);
	snd_soc_write(codec, HI6403ES_MAIN1_PLL_FILTER_REG, 0x56);
	snd_soc_write(codec, HI6403ES_MAIN1_PLL_CFG_REG4, 0x59);
	snd_soc_write(codec, HI6403ES_MAIN1_PLL_CFG_REG5, 0x0);
	snd_soc_write(codec, HI6403ES_MAIN1_PLL_CFG_REG6, 0x0);
	snd_soc_write(codec, HI6403ES_MAIN1_PLL_TRACK_REG1, 0x0);
	snd_soc_write(codec, HI6403ES_MAIN1_PLL_TRACK_REG2, 0x0);
	snd_soc_write(codec, HI6403ES_MAIN1_PLL_TRACK_REG3, 0x0);
	snd_soc_write(codec, HI6403ES_MAIN1_PLL_CFG_REG7, 0xA0);
	snd_soc_write(codec, HI6403ES_MAIN1_PLL_CFG_REG8, 0x40);
	snd_soc_write(codec, HI6403ES_MAIN1_PLL_CFG_REG9, 0x8);
	snd_soc_write(codec, HI6403ES_MAIN1_PLL_LOCK_CFG_REG, 0x40);
	snd_soc_write(codec, HI6403ES_MAIN1_PLL_PHASE_CFG_REG, 0x0);
	return 0;
}

bool hi6403es_pll48k_check(struct snd_soc_codec *codec)
{
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);
	int i = 0;
	unsigned int regval[6];

	BUG_ON(NULL == priv);

	mdelay(5);
	/* clear pll unlock state disable */
	hi64xx_update_bits(codec, HI6403ES_PLL_LOCK_CFG_REG,
					1<<HI6403ES_PLL_LOCK_CLR_BIT, 0);
	mdelay(5);
	/* check pll48k lock state */
	while ((snd_soc_read(codec, HI6403ES_PLL48K_STATE_REG) & 0x4)
		|| (snd_soc_read(codec, HI64xx_VERSION_REG) != HI6403ES_VERSION_VALUE)) {
		/* clear pll unlock state enable */
		hi64xx_update_bits(codec, HI6403ES_PLL_LOCK_CFG_REG,
						1<<HI6403ES_PLL_LOCK_CLR_BIT, 1<<HI6403ES_PLL_LOCK_CLR_BIT);
		mdelay(5);
		/* clear pll unlock state disable */
		hi64xx_update_bits(codec, HI6403ES_PLL_LOCK_CFG_REG,
						1<<HI6403ES_PLL_LOCK_CLR_BIT, 0);

		if (++i == 10) {
			pr_err("%s:check time is %d\n", __FUNCTION__, i);
			return false;
		}
	}
	regval[0] = snd_soc_read(codec, HI6403ES_PLL48K_STATE_REG);
	regval[1] = snd_soc_read(codec, HI64xx_VERSION_REG);
	regval[2] = snd_soc_read(codec, HI64XX_CHIP_ID_REG0);
	regval[3] = snd_soc_read(codec, HI64XX_CHIP_ID_REG1);
	regval[4] = snd_soc_read(codec, HI64XX_CHIP_ID_REG2);
	regval[5] = snd_soc_read(codec, HI64XX_CHIP_ID_REG3);
	pr_err("%s: check time is %d, pllstate:0x%x, version:0x%x, chipid0:0x%x, chipid1:0x%x, chipid2:0x%x, chipid3:0x%x\n",
		__FUNCTION__, i, regval[0], regval[1], regval[2], regval[3], regval[4], regval[5]);
	hi64xx_irq_enable_irq(priv->irqmgr, IRQ_PLL_UNLOCK);

	return true;
}

int hi6403es_pll48k_turn_on(struct snd_soc_codec *codec)
{
	hi64xx_update_bits(codec, HI6403ES_MAIN1_PLL_CFG_REG1,
					1<<HI6403ES_MAIN1_PLL_RUN_START_BIT, 1<<HI6403ES_MAIN1_PLL_RUN_START_BIT);
	hi64xx_update_bits(codec, HI6403ES_MAIN1_PLL_CFG_REG1,
					1<<HI6403ES_MAIN1_PLL_PD_BIT, 0);
	udelay(20);
	hi64xx_update_bits(codec, HI6403ES_MAIN1_PLL_CFG_REG1,
					1<<HI6403ES_MAIN1_PLL_RST_BIT, 1<<HI6403ES_MAIN1_PLL_RST_BIT);
	hi64xx_update_bits(codec, HI6403ES_MAIN1_PLL_CFG_REG1,
					1<<HI6403ES_MAIN1_ISO_18_BIT, 1<<HI6403ES_MAIN1_ISO_18_BIT);
	hi64xx_update_bits(codec, HI6403ES_MAIN1_PLL_CFG_REG1,
					1<<HI6403ES_MAIN1_ISO_12_BIT, 1<<HI6403ES_MAIN1_ISO_12_BIT);

	return 0;
}

int hi6403es_pll48k_turn_off(struct snd_soc_codec *codec)
{
	unsigned int regval[5];
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == priv);

	hi64xx_irq_disable_irq(priv->irqmgr, IRQ_PLL_UNLOCK);

	hi64xx_update_bits(codec, HI6403ES_MAIN1_PLL_CFG_REG1,
					1<<HI6403ES_MAIN1_PLL_PD_BIT, 1<<HI6403ES_MAIN1_PLL_PD_BIT);
	hi64xx_update_bits(codec, HI6403ES_MAIN1_PLL_CFG_REG1,
					1<<HI6403ES_MAIN1_ISO_18_BIT, 0);
	hi64xx_update_bits(codec, HI6403ES_MAIN1_PLL_CFG_REG1,
					1<<HI6403ES_MAIN1_ISO_12_BIT, 0);
	hi64xx_update_bits(codec, HI6403ES_MAIN1_PLL_CFG_REG1,
					1<<HI6403ES_MAIN1_PLL_RST_BIT, 0);
	hi64xx_update_bits(codec, HI6403ES_MAIN1_PLL_CFG_REG1,
					1<<HI6403ES_MAIN1_PLL_RUN_START_BIT, 0);
	regval[0] = snd_soc_read(codec, HI64xx_VERSION_REG);
	regval[1] = snd_soc_read(codec, HI64XX_CHIP_ID_REG0);
	regval[2] = snd_soc_read(codec, HI64XX_CHIP_ID_REG1);
	regval[3] = snd_soc_read(codec, HI64XX_CHIP_ID_REG2);
	regval[4] = snd_soc_read(codec, HI64XX_CHIP_ID_REG3);
	pr_info("48k off, version:0x%x, chipid0:0x%x, chipid1:0x%x, chipid2:0x%x, chipid3:0x%x\n",
		regval[0], regval[1], regval[2], regval[3], regval[4]);

	/* clear pll unlock state enable */
	hi64xx_update_bits(codec, HI6403ES_PLL_LOCK_CFG_REG,
					1<<HI6403ES_PLL_LOCK_CLR_BIT, 1<<HI6403ES_PLL_LOCK_CLR_BIT);
	return 0;
}

int hi6403es_pll44k1_config(struct snd_soc_codec *codec)
{
	snd_soc_write(codec, HI6403ES_MAIN2_PLL_CFG_REG2, 0x7E);
	snd_soc_write(codec, HI6403ES_MAIN2_PLL_DIV_REG, 0x10);
	snd_soc_write(codec, HI6403ES_MAIN2_PLL_CFG_REG3, 0x4E);
	snd_soc_write(codec, HI6403ES_MAIN2_PLL_WORD_REG1, 0xB3);
	snd_soc_write(codec, HI6403ES_MAIN2_PLL_WORF_REG2, 0x34);
	snd_soc_write(codec, HI6403ES_MAIN2_PLL_FILTER_REG, 0x56);
	snd_soc_write(codec, HI6403ES_MAIN2_PLL_CFG_REG4, 0x59);
	snd_soc_write(codec, HI6403ES_MAIN2_PLL_CFG_REG5, 0x0);
	snd_soc_write(codec, HI6403ES_MAIN2_PLL_CFG_REG6, 0x0);
	snd_soc_write(codec, HI6403ES_MAIN2_PLL_TRACK_REG1, 0x0);
	snd_soc_write(codec, HI6403ES_MAIN2_PLL_TRACK_REG2, 0x0);
	snd_soc_write(codec, HI6403ES_MAIN2_PLL_TRACK_REG3, 0x0);
	snd_soc_write(codec, HI6403ES_MAIN2_PLL_CFG_REG7, 0xA0);
	snd_soc_write(codec, HI6403ES_MAIN2_PLL_CFG_REG8, 0x40);
	snd_soc_write(codec, HI6403ES_MAIN2_PLL_CFG_REG9, 0x8);
	snd_soc_write(codec, HI6403ES_MAIN2_PLL_LOCK_CFG_REG, 0x40);
	snd_soc_write(codec, HI6403ES_MAIN2_PLL_PHASE_CFG_REG, 0x0);
	return 0;
}

bool hi6403es_pll44k1_check(struct snd_soc_codec *codec)
{
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);
	int i = 0;

	BUG_ON(NULL == priv);

	mdelay(5);
	/* clear pll unlock state disable */
	hi64xx_update_bits(codec, HI6403ES_PLL_LOCK_CFG_REG,
					1<<HI6403ES_PLL_LOCK_CLR_BIT, 0);
	mdelay(5);
	/* check pll44k1 lock state */
	while (snd_soc_read(codec, HI6403ES_PLL44K1_PLLMAD_STATE_REG) & 0x40) {
		/* clear pll unlock state enable */
		hi64xx_update_bits(codec, HI6403ES_PLL_LOCK_CFG_REG,
						1<<HI6403ES_PLL_LOCK_CLR_BIT, 1<<HI6403ES_PLL_LOCK_CLR_BIT);
		mdelay(5);
		/* clear pll unlock state disable */
		hi64xx_update_bits(codec, HI6403ES_PLL_LOCK_CFG_REG,
						1<<HI6403ES_PLL_LOCK_CLR_BIT, 0);

		if (++i == 10) {
			pr_err("%s: check time is %d\n", __FUNCTION__, i);
			return false;
		}
	}

	pr_err("%s: check time is %d\n", __FUNCTION__, i);
	hi64xx_irq_enable_irq(priv->irqmgr, IRQ_PLL44K1_UNLOCK);

	return true;
}

int hi6403es_pll44k1_turn_on(struct snd_soc_codec *codec)
{
	hi64xx_update_bits(codec, HI6403ES_MAIN2_PLL_CFG_REG1,
					1<<HI6403ES_MAIN2_PLL_RUN_START_BIT, 1<<HI6403ES_MAIN2_PLL_RUN_START_BIT);
	hi64xx_update_bits(codec, HI6403ES_MAIN2_PLL_CFG_REG1,
					1<<HI6403ES_MAIN2_PLL_PD_BIT, 0);
	udelay(20);
	hi64xx_update_bits(codec, HI6403ES_MAIN2_PLL_CFG_REG1,
					1<<HI6403ES_MAIN2_PLL_RST_BIT, 1<<HI6403ES_MAIN2_PLL_RST_BIT);
	hi64xx_update_bits(codec, HI6403ES_MAIN2_PLL_CFG_REG1,
					1<<HI6403ES_MAIN2_ISO_18_BIT, 1<<HI6403ES_MAIN2_ISO_18_BIT);
	hi64xx_update_bits(codec, HI6403ES_MAIN2_PLL_CFG_REG1,
					1<<HI6403ES_MAIN2_ISO_12_BIT, 1<<HI6403ES_MAIN2_ISO_12_BIT);

	return 0;
}

int hi6403es_pll44k1_turn_off(struct snd_soc_codec *codec)
{
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == priv);

	hi64xx_irq_disable_irq(priv->irqmgr, IRQ_PLL44K1_UNLOCK);

	hi64xx_update_bits(codec, HI6403ES_MAIN2_PLL_CFG_REG1,
					1<<HI6403ES_MAIN2_ISO_18_BIT, 0);
	hi64xx_update_bits(codec, HI6403ES_MAIN2_PLL_CFG_REG1,
					1<<HI6403ES_MAIN2_ISO_12_BIT, 0);
	hi64xx_update_bits(codec, HI6403ES_MAIN2_PLL_CFG_REG1,
					1<<HI6403ES_MAIN2_PLL_RST_BIT, 0);
	hi64xx_update_bits(codec, HI6403ES_MAIN2_PLL_CFG_REG1,
					1<<HI6403ES_MAIN2_PLL_PD_BIT, 1<<HI6403ES_MAIN2_PLL_PD_BIT);
	hi64xx_update_bits(codec, HI6403ES_MAIN2_PLL_CFG_REG1,
					1<<HI6403ES_MAIN2_PLL_RUN_START_BIT, 0);
	pr_err("44k1 off \n");
	/* clear pll unlock state enable */
	hi64xx_update_bits(codec, HI6403ES_PLL_LOCK_CFG_REG,
					1<<HI6403ES_PLL_LOCK_CLR_BIT, 1<<HI6403ES_PLL_LOCK_CLR_BIT);
	return 0;
}

int hi6403es_pllmad_config(struct snd_soc_codec *codec)
{
	snd_soc_write(codec, HI6403ES_MAD_PLL_CFG_REG10, 0x7);
	snd_soc_write(codec, HI6403ES_MAD_PLL_WORD_REG1, 0xBB);
	snd_soc_write(codec, HI6403ES_MAD_PLL_WORF_REG2, 0x80);
	snd_soc_write(codec, HI6403ES_MAD_PLL_CFG_REG1, 0xC7);
	snd_soc_write(codec, HI6403ES_MAD_PLL_CFG_REG2, 0x6);
	snd_soc_write(codec, HI6403ES_MAD_PLL_CFG_REG3, 0xC0);
	snd_soc_write(codec, HI6403ES_MAD_PLL_CFG_REG4, 0x18);
	snd_soc_write(codec, HI6403ES_MAD_PLL_CFG_REG5, 0x40);
	snd_soc_write(codec, HI6403ES_MAD_PLL_LOCK_CFG_REG, 0x40);
	return 0;
}

bool hi6403es_pllmad_check(struct snd_soc_codec *codec)
{
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);
	int i = 0;

	BUG_ON(NULL == priv);

	mdelay(5);
	/* clear pll unlock state disable */
	hi64xx_update_bits(codec, HI6403ES_PLL_LOCK_CFG_REG,
					1<<HI6403ES_PLL_LOCK_CLR_BIT, 0);
	mdelay(5);
	/* check pllmad lock state */
	while (snd_soc_read(codec, HI6403ES_PLL44K1_PLLMAD_STATE_REG) & 0x80) {
		/* clear pll unlock state enable */
		hi64xx_update_bits(codec, HI6403ES_PLL_LOCK_CFG_REG,
						1<<HI6403ES_PLL_LOCK_CLR_BIT, 1<<HI6403ES_PLL_LOCK_CLR_BIT);
		mdelay(5);
		/* clear pll unlock state disable */
		hi64xx_update_bits(codec, HI6403ES_PLL_LOCK_CFG_REG,
						1<<HI6403ES_PLL_LOCK_CLR_BIT, 0);
		if (++i == 10) {
			pr_err("%s: check time is %d\n", __FUNCTION__, i);
			return false;
		}
	}
	pr_err("%s: check time is %d\n", __FUNCTION__, i);
	hi64xx_irq_enable_irq(priv->irqmgr, IRQ_PLLMAD_UNLOCK);

	return true;
}

int hi6403es_pllmad_turn_on(struct snd_soc_codec *codec)
{
	hi64xx_update_bits(codec, HI6403ES_MAD_PLL_CFG_REG9,
					1<<HI6403ES_MAD_PLL_RUN_START_BIT, 1<<HI6403ES_MAD_PLL_RUN_START_BIT);
	hi64xx_update_bits(codec, HI6403ES_MAD_PLL_CFG_REG9,
					1<<HI6403ES_MAD_PLL_PD_BIT, 0);
	udelay(10);
	hi64xx_update_bits(codec, HI6403ES_MAD_PLL_CFG_REG9,
					1<<HI6403ES_MAD_PLL_RST_BIT, 1<<HI6403ES_MAD_PLL_RST_BIT);
	hi64xx_update_bits(codec, HI6403ES_MAD_PLL_CFG_REG6,
					1<<HI6403ES_MAD_PLL_ISO_18_BIT, 1<<HI6403ES_MAD_PLL_ISO_18_BIT);
	hi64xx_update_bits(codec, HI6403ES_12M288_CLK_SEL_REG, 0x2, 0x2);

	/* adc clk select madpll */
	hi64xx_update_bits(codec, HI6403ES_PATH_CLK_SEL_REG, 0xFF, 0xFF);

	return 0;
}
int hi6403es_pllmad_turn_off(struct snd_soc_codec *codec)
{
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == priv);

	hi64xx_irq_disable_irq(priv->irqmgr, IRQ_PLLMAD_UNLOCK);

	/* adc clk select pll48k */
	hi64xx_update_bits(codec, HI6403ES_PATH_CLK_SEL_REG, 0xFF, 0);
	hi64xx_update_bits(codec, HI6403ES_12M288_CLK_SEL_REG, 0x2, 0x0);
	hi64xx_update_bits(codec, HI6403ES_MAD_PLL_CFG_REG9,
					1<<HI6403ES_MAD_PLL_PD_BIT, 1<<HI6403ES_MAD_PLL_PD_BIT);
	hi64xx_update_bits(codec, HI6403ES_MAD_PLL_CFG_REG9,
					1<<HI6403ES_MAD_PLL_RST_BIT, 0);
	hi64xx_update_bits(codec, HI6403ES_MAD_PLL_CFG_REG6,
					1<<HI6403ES_MAD_PLL_ISO_18_BIT, 0);
	hi64xx_update_bits(codec, HI6403ES_MAD_PLL_CFG_REG9,
					1<<HI6403ES_MAD_PLL_RUN_START_BIT, 0);
	/* clear pll unlock state enable */
	hi64xx_update_bits(codec, HI6403ES_PLL_LOCK_CFG_REG,
					1<<HI6403ES_PLL_LOCK_CLR_BIT, 1<<HI6403ES_PLL_LOCK_CLR_BIT);
	pr_info("mad off \n");
	return 0;
}

/* functions to enable/disable micbias */
int hi6403es_enable_micbias(struct snd_soc_codec *codec)
{
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == priv);

	/* mask btn irqs */
	hi64xx_irq_mask_btn_irqs(priv->mbhc);
	/* eco off */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG093, 1<<HI6403ES_HS_ECO_EN_BIT, 0);
	pr_info("%s : eco disable \n", __FUNCTION__);
	/* mbhc cmp on */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG089, 1<<HI6403ES_MBHD_COMP_PD_BIT, 0);
	/* hsmic chg */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG093, 1<<HI6403ES_HSMIC_DISCHG_EN_BIT, 0);
	/* hsmic on */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG089, 1<<HI6403ES_HSMIC_PD_BIT, 0);
	pr_info("%s : hs micbias enable \n", __FUNCTION__);
	msleep(20);
	/* unmask btn irqs */
	hi64xx_irq_unmask_btn_irqs(priv->mbhc);

	return 0;
}

int hi6403es_disable_micbias(struct snd_soc_codec *codec)
{
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == priv);

	/* mask btn irqs */
	hi64xx_irq_mask_btn_irqs(priv->mbhc);
	/* hsmic pd */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG089, 1<<HI6403ES_HSMIC_PD_BIT, 1<<HI6403ES_HSMIC_PD_BIT);
	/* hsmic dischg */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG093, 1<<HI6403ES_HSMIC_DISCHG_EN_BIT, 1<<HI6403ES_HSMIC_DISCHG_EN_BIT);
	msleep(15);
	/* hsmic chg */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG093, 1<<HI6403ES_HSMIC_DISCHG_EN_BIT, 0);
	pr_info("%s : hs micbias disable \n", __FUNCTION__);
	/* eco on & eco auto saradc on */
	/* mbhc cmp off */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG089, 1<<HI6403ES_MBHD_COMP_PD_BIT, 1<<HI6403ES_MBHD_COMP_PD_BIT);
	/* eco on */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG093, 1<<HI6403ES_HS_ECO_EN_BIT, 1<<HI6403ES_HS_ECO_EN_BIT);
	pr_info("%s : eco enable \n", __FUNCTION__);
	msleep(20);

	/* unmask btn irqs */
	hi64xx_irq_unmask_btn_irqs(priv->mbhc);

	return 0;
}

/* functions to enable/disable ibias */
int hi6403es_enable_ibias(struct snd_soc_codec *codec)
{
	/* vref_fast on */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG0, 1<<HI6403ES_VREF_FAST_BIT, 1<<HI6403ES_VREF_FAST_BIT);
	/* vref select 1k */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG0, 0x03<<HI6403ES_VREFSEL_BIT, 0x03<<HI6403ES_VREFSEL_BIT);
	mdelay(30);
	/* vref_fast off */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG0, 1<<HI6403ES_VREF_FAST_BIT, 0);
	/* vref select 100k */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG0, 0x03<<HI6403ES_VREFSEL_BIT, 0x01<<HI6403ES_VREFSEL_BIT);\
	/* ibias on */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG0, 1<<HI6403ES_IBIAS_PD_BIT, 0);

	return 0;
}

int hi6403es_disable_ibias(struct snd_soc_codec *codec)
{
	/* ibias off */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG0, 1<<HI6403ES_IBIAS_PD_BIT, 1<<HI6403ES_IBIAS_PD_BIT);
	/* vref select pd */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG0, 0x03<<HI6403ES_VREFSEL_BIT, 0);
	return 0;
}

void hi6403es_headphone_resdet_config(struct snd_soc_codec *codec)
{
	/* ib05_hp 5uA */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG73,
			0xF<<HI6403ES_IB05_HP_BIT, 0x9<<HI6403ES_IB05_HP_BIT);
	/* ib05_hp_fb 5uA */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG77,
			0xF<<HI6403ES_IB05_HP_FB_BIT, 0x9<<HI6403ES_IB05_HP_FB_BIT);
	/* headphone 0x200070D1 config */
	snd_soc_write(codec, HI6403ES_ANALOG_REG48, 0xEA);
	/* headphone 0x200070D2 config */
	snd_soc_write(codec, HI6403ES_ANALOG_REG49, 0x2A);
	/* hpl_fb off */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG43,
			0x7<<HI6403ES_HPL_POP_PULL_5_3_BIT, 0x0<<HI6403ES_HPL_POP_PULL_5_3_BIT);
	/* hpr_fb off */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG44,
			0x7<<HI6403ES_HPR_POP_PULL_5_3_BIT, 0x0<<HI6403ES_HPR_POP_PULL_5_3_BIT);
	/* headphone dac VB current select -> 80uA */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG38,
			0x3<<HI6403ES_DAC_HP_VBR_SEL_BIT, 0x1<<HI6403ES_DAC_HP_VBR_SEL_BIT);
}

void hi6403es_imp_path_enable(struct snd_soc_codec *codec, bool enable)
{
	if (enable) {
		/* dacl src up16 clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG06, 1<<HI6403ES_DACL_UP16_CLKEN_BIT, 1<<HI6403ES_DACL_UP16_CLKEN_BIT);
		/* dacl sdm clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG08, 1<<HI6403ES_DACL_SDM_CLKEN_BIT, 1<<HI6403ES_DACL_SDM_CLKEN_BIT);
		/* dsdl sdm clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG08, 1<<HI6403ES_DSDL_SDM_CLKEN_BIT, 1<<HI6403ES_DSDL_SDM_CLKEN_BIT);
		/* dacr src up16 clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG06, 1<<HI6403ES_DACR_UP16_CLKEN_BIT, 1<<HI6403ES_DACR_UP16_CLKEN_BIT);
		/* dacr sdm clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG08, 1<<HI6403ES_DACR_SDM_CLKEN_BIT, 1<<HI6403ES_DACR_SDM_CLKEN_BIT);
		/* dsdr sdm clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG08, 1<<HI6403ES_DSDR_SDM_CLKEN_BIT, 1<<HI6403ES_DSDR_SDM_CLKEN_BIT);
		/* hp mux -> sdm */
		hi64xx_update_bits(codec, HI6403ES_DAC_HP_MUX_SEL, 0x3f, 0x0);
		/* dac select imp */
		hi64xx_update_bits(codec, HI6403ES_DACL_PGA_MUX_SEL,
				0x3<<HI6403ES_DAC_PGA_MUX_BIT, 0x2<<HI6403ES_DAC_PGA_MUX_BIT);
		hi64xx_update_bits(codec, HI6403ES_DACR_PGA_MUX_SEL,
				0x3<<HI6403ES_DAC_PGA_MUX_BIT, 0x2<<HI6403ES_DAC_PGA_MUX_BIT);
		/* up16 mux -> pga */
		hi64xx_update_bits(codec, HI6403ES_CLASSH_HP_EP_CTRL,
				0x1<<HI6403ES_DACL_U16_MUX_BIT, 0x0<<HI6403ES_DACL_U16_MUX_BIT);
		hi64xx_update_bits(codec, HI6403ES_CLASSH_HP_EP_CTRL,
				0x1<<HI6403ES_DACR_U16_MUX_BIT, 0x0<<HI6403ES_DACR_U16_MUX_BIT);

		/* dac pga enable -> -40dB */
		snd_soc_write(codec, HI6403ES_DACL_PGA_GAIN_CFG_REG, 0x28);
		snd_soc_write(codec, HI6403ES_DACR_PGA_GAIN_CFG_REG, 0x28);

		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG10,
				0x3<<HI6403ES_DACL_PGA_CLKEN_BIT, 0x3<<HI6403ES_DACL_PGA_CLKEN_BIT);

		/* imp_det clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG05,
				0x1<<HI6403ES_HP_DET_CLKEN_BIT, 0x1<<HI6403ES_HP_DET_CLKEN_BIT);
		/* imp_det enable */
		hi64xx_update_bits(codec, HI6403ES_IMP_DET_CTRL_REG,
				0x1<<HI6403ES_IMP_EN_BIT, 0x1<<HI6403ES_IMP_EN_BIT);
		msleep(1);
		/* imp_det disable */
		hi64xx_update_bits(codec, HI6403ES_IMP_DET_CTRL_REG,
				0x1<<HI6403ES_IMP_EN_BIT, 0x0<<HI6403ES_IMP_EN_BIT);
		msleep(1);
		/* imp_det clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG05,
				0x1<<HI6403ES_HP_DET_CLKEN_BIT, 0x0<<HI6403ES_HP_DET_CLKEN_BIT);

		/* headphone pop on */
		hi6403es_headphone_pop_on(codec);
	} else {
		/* headphone pop off */
		hi6403es_headphone_pop_off(codec);

		/* up16 mux -> classH */
		hi64xx_update_bits(codec, HI6403ES_CLASSH_HP_EP_CTRL,
				0x1<<HI6403ES_DACL_U16_MUX_BIT, 0x1<<HI6403ES_DACL_U16_MUX_BIT);
		hi64xx_update_bits(codec, HI6403ES_CLASSH_HP_EP_CTRL,
				0x1<<HI6403ES_DACR_U16_MUX_BIT, 0x1<<HI6403ES_DACR_U16_MUX_BIT);
		/* dac select imp */
		hi64xx_update_bits(codec, HI6403ES_DACL_PGA_MUX_SEL,
				0x3<<HI6403ES_DAC_PGA_MUX_BIT, 0x0<<HI6403ES_DAC_PGA_MUX_BIT);
		hi64xx_update_bits(codec, HI6403ES_DACR_PGA_MUX_SEL,
				0x3<<HI6403ES_DAC_PGA_MUX_BIT, 0x0<<HI6403ES_DAC_PGA_MUX_BIT);

		/* dac pga clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG10,
				0x3<<HI6403ES_DACL_PGA_CLKEN_BIT, 0x0<<HI6403ES_DACL_PGA_CLKEN_BIT);

		/* hp mux -> off */
		hi64xx_update_bits(codec, HI6403ES_DAC_HP_MUX_SEL, 0x3f, 0x1B);
		/* dacr sdm clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG08, 1<<HI6403ES_DACR_SDM_CLKEN_BIT, 0);
		/* dsdr sdm clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG08, 1<<HI6403ES_DSDR_SDM_CLKEN_BIT, 0);
		/* dacr src up16 clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG06, 1<<HI6403ES_DACR_UP16_CLKEN_BIT, 0);
		/* dacl sdm clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG08, 1<<HI6403ES_DACL_SDM_CLKEN_BIT, 0);
		/* dsdl sdm clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG08, 1<<HI6403ES_DSDL_SDM_CLKEN_BIT, 0);
		/* dacl src up16 clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG06, 1<<HI6403ES_DACL_UP16_CLKEN_BIT, 0);
	}
}

void hi6403es_headphone_path_enable(struct snd_soc_codec *codec)
{
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);

	BUG_ON(NULL == priv);

	/* disable the trigger for shadow gain */
	hi64xx_update_bits(codec, HI6403ES_PERF_SWITCH_CTRL8,
			0x1<<HI6403ES_PERF_TRIGGER_EN_BIT, 0x0<<HI6403ES_PERF_TRIGGER_EN_BIT);
	/* disable the config for shadow gain */
	hi64xx_update_bits(codec, HI6403ES_PERF_SWITCH_CTRL8,
			0x1<<HI6403ES_PERF_SWITCH_EN_BIT, 0x0<<HI6403ES_PERF_SWITCH_EN_BIT);

	/* pll request */
	hi64xx_resmgr_request_pll(priv->resmgr, PLL_HIGH);
	/* dp clk enable */
	hi6403es_request_dp_clk(codec, true);
	/* cp1 enable */
	hi6403es_request_cp1(codec, true);
	/* cp2 enable */
	hi6403es_request_cp2(codec, true);
	/* headphone config for imp_det */
	hi6403es_headphone_resdet_config(codec);

	/* dac pga fade enable
	hi64xx_update_bits(codec, HI6403ES_DACL_PGA_CFG_REG,
			0x1<<HI6403ES_DACL_PGA_FD_EN_BIT, 0x1<<HI6403ES_DACL_PGA_FD_EN_BIT);
	hi64xx_update_bits(codec, HI6403ES_DACR_PGA_CFG_REG,
			0x1<<HI6403ES_DACR_PGA_FD_EN_BIT, 0x1<<HI6403ES_DACR_PGA_FD_EN_BIT);*/

	/* path enable */
	hi6403es_imp_path_enable(codec, true);


}

void hi6403es_headphone_path_disable(struct snd_soc_codec *codec)
{
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);

	BUG_ON(NULL == priv);

	/* path enable */
	hi6403es_imp_path_enable(codec, false);
	/* cp2 enable */
	hi6403es_request_cp2(codec, false);
	/* cp1 enable */
	hi6403es_request_cp1(codec, false);
	/* dp clk enable */
	hi6403es_request_dp_clk(codec, false);
	/* pll request */
	hi64xx_resmgr_release_pll(priv->resmgr, PLL_HIGH);

	/* classA/B -> classH  */
	hi64xx_update_bits(codec, HI6403ES_CLASS_H_CFG_REG1,
			0x1<<HI6403ES_CLASS_H_SEL_BIT, 0x1<<HI6403ES_CLASS_H_SEL_BIT);

	if (true == priv->board_config.hp_high_low_change_enable) {
		hi6403es_headphone_low_mode_shadow_config(codec);
		/* headphone ib05 current init */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG73,
				0xF<<HI6403ES_IB05_HP_BIT, 0x4<<HI6403ES_IB05_HP_BIT);
		/* enable the config for shadow gain */
		hi64xx_update_bits(codec, HI6403ES_PERF_SWITCH_CTRL8,
				0x1<<HI6403ES_PERF_SWITCH_EN_BIT, 0x1<<HI6403ES_PERF_SWITCH_EN_BIT);
		/* enable the trigger for shadow gain */
		hi64xx_update_bits(codec, HI6403ES_PERF_SWITCH_CTRL8,
				0x1<<HI6403ES_PERF_TRIGGER_EN_BIT, 0x1<<HI6403ES_PERF_TRIGGER_EN_BIT);
	} else {
		hi6403es_headphone_high_mode_shadow_config(codec);
	}

	/* dac pga fade disable
	hi64xx_update_bits(codec, HI6403ES_DACL_PGA_CFG_REG,
			0x1<<HI6403ES_DACL_PGA_FD_EN_BIT, 0x0<<HI6403ES_DACL_PGA_FD_EN_BIT);
	hi64xx_update_bits(codec, HI6403ES_DACR_PGA_CFG_REG,
			0x1<<HI6403ES_DACR_PGA_FD_EN_BIT, 0x0<<HI6403ES_DACR_PGA_FD_EN_BIT);*/
}

void hi6403es_headphone_resdet_enable(struct snd_soc_codec *codec, bool enable)
{
	if (enable) {
		/* imp_det enable */
		hi64xx_update_bits(codec, HI6403ES_IMP_DET_CTRL_REG,
				0x1<<HI6403ES_IMP_EN_BIT, 0x1<<HI6403ES_IMP_EN_BIT);
		/* imp_det clk enable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG05,
				0x1<<HI6403ES_HP_DET_CLKEN_BIT, 0x1<<HI6403ES_HP_DET_CLKEN_BIT);

		/* dac pga : -40dB -> 0dB */
		snd_soc_write(codec, HI6403ES_DACL_PGA_GAIN_CFG_REG, 0x78);
		snd_soc_write(codec, HI6403ES_DACR_PGA_GAIN_CFG_REG, 0x78);
		msleep(32);

		/* sar clk -> 32k */
		hi64xx_update_bits(codec, HI6403ES_SAR_CONFIG_REG,
				0x7<<HI6403ES_SAR_CLK_SEL_BIT, 0x3<<HI6403ES_SAR_CLK_SEL_BIT);
		/* sar input -> hpl */
		hi64xx_update_bits(codec, HI6403ES_HP_DET_CFG_REG,
				0x3<<HI6403ES_SAR_INPUT_SEL_BIT, 0x2<<HI6403ES_SAR_INPUT_SEL_BIT);
		/* sar ref -> dvdd1.8 */
		hi64xx_update_bits(codec, HI6403ES_HP_DET_CFG_REG,
				0x7<<HI6403ES_SAR_REF_SEL_BIT, 0x4<<HI6403ES_SAR_REF_SEL_BIT);
		/* sar rst -> work */
		hi64xx_update_bits(codec, HI6403ES_SAR_CONFIG_REG,
				0x1<<HI6403ES_SAR_RST_BIT, 0x1<<HI6403ES_SAR_RST_BIT);
		/* sar power on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG089,
				0x1<<HI6403ES_MBHD_SAR_PD_BIT, 0x0<<HI6403ES_MBHD_SAR_PD_BIT);
		/* det_pd on */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG3,
				0x1<<HI6403ES_HP_RDET_PD_BIT, 0x0<<HI6403ES_HP_RDET_PD_BIT);
	} else {
		/* det_pd off */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG3,
				0x1<<HI6403ES_HP_RDET_PD_BIT, 0x1<<HI6403ES_HP_RDET_PD_BIT);
		/* sar power off */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG089,
				0x1<<HI6403ES_MBHD_SAR_PD_BIT, 0x1<<HI6403ES_MBHD_SAR_PD_BIT);

		/* sar ref -> micbias2.7 */
		hi64xx_update_bits(codec, HI6403ES_HP_DET_CFG_REG,
				0x7<<HI6403ES_SAR_REF_SEL_BIT, 0x6<<HI6403ES_SAR_REF_SEL_BIT);
		/* sar input -> mbhc */
		hi64xx_update_bits(codec, HI6403ES_HP_DET_CFG_REG,
				0x3<<HI6403ES_SAR_INPUT_SEL_BIT, 0x0<<HI6403ES_SAR_INPUT_SEL_BIT);
		/* sar clk -> 32k */
		hi64xx_update_bits(codec, HI6403ES_SAR_CONFIG_REG,
				0x7<<HI6403ES_SAR_CLK_SEL_BIT, 0x3<<HI6403ES_SAR_CLK_SEL_BIT);

		/* dac pga : 0dB -> -40dB */
		snd_soc_write(codec, HI6403ES_DACL_PGA_GAIN_CFG_REG, 0x28);
		snd_soc_write(codec, HI6403ES_DACR_PGA_GAIN_CFG_REG, 0x28);
		msleep(32);

		/* imp_det clk disable */
		hi64xx_update_bits(codec, HI6403ES_DIG_CLK_CFG_REG05,
				0x1<<HI6403ES_HP_DET_CLKEN_BIT, 0x0<<HI6403ES_HP_DET_CLKEN_BIT);
		/* imp_det disable */
		hi64xx_update_bits(codec, HI6403ES_IMP_DET_CTRL_REG,
				0x1<<HI6403ES_IMP_EN_BIT, 0x0<<HI6403ES_IMP_EN_BIT);
	}
}

bool hi6403es_check_sarready(struct snd_soc_codec *codec)
{
	/* read codec status */
	unsigned int value = snd_soc_read(codec, HI64xx_REG_IRQ_2) & (0x1<<HI64xx_SARADC_RD_BIT);

	/*clr irq*/
	snd_soc_write(codec, HI64xx_REG_IRQ_2, 0x1<<HI64xx_SARADC_RD_BIT);

	if (0 == value)
		return false;
	return true;
}

unsigned int hi6403es_get_voltagevlaue(struct snd_soc_codec *codec)
{
	int retry = 3;
	unsigned int sar_value = 0;
	unsigned int voltage_value = 0;

	/* start saradc */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG089,
			0x1<<HI6403ES_SARADC_START_BIT, 0x1<<HI6403ES_SARADC_START_BIT);

	while(retry--) {
		usleep_range(1000, 1100);
		if (hi6403es_check_sarready(codec)) {
			sar_value = snd_soc_read(codec, HI6403ES_SAR_VALUE_REG);
			pr_info("%s : saradc value for imp is %#x\n", __FUNCTION__, sar_value);

			break;
		}
	}
	if (0 > retry)
		pr_err("%s : get saradc err \n", __FUNCTION__);

	/* end saradc */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG089,
			0x1<<HI6403ES_SARADC_START_BIT, 0x0<<HI6403ES_SARADC_START_BIT);

	voltage_value = sar_value * (1800 / 0xFF);
	if (voltage_value < 100 || voltage_value > 300) {
		pr_info("%s : voltage value is %d, invalid value, res selected is %#x\n", __FUNCTION__, voltage_value, snd_soc_read(codec, HI6403ES_ANALOG_REG50));
		voltage_value = 0;
	}

	return voltage_value;
}

unsigned int hi6403es_calc_res(struct snd_soc_codec *codec, unsigned int voltage_value, int num)
{
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);
	unsigned int res_value = 0;
	unsigned int eq_res_value = 0;

	BUG_ON(NULL == priv);

	eq_res_value = (1<<num) * priv->res_value * HI6403ES_IMP_RANGE_VALUE / voltage_value / HI6403ES_CURRENT_RATIO_VALUE;

	if (eq_res_value >= 625) {
		pr_err("%s : calculate res value is %#x, invalid value\n", __FUNCTION__, eq_res_value);
		res_value = HI6403ES_FAKE_RES_VALUE;
	} else {
		res_value = HI6403ES_FB_RES_VALUE * eq_res_value / (HI6403ES_FB_RES_VALUE - eq_res_value);
	}
	return res_value;
}

unsigned int hi6403es_get_resvalue(struct snd_soc_codec *codec)
{
	unsigned int res_value = 0;
	unsigned int volume_value[HI6403ES_IMP_RES_NUM] = {0};
	unsigned int calc_res_value = 0;
	int i = 0;
	unsigned int data_num = 0;

	hi6403es_headphone_resdet_enable(codec, true);

	msleep(40);

	for (i = 0; i < HI6403ES_IMP_RES_NUM; i++) {
		/* select different res */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG50,
				0x7<<HI6403ES_DETRES_CHAIN_SEL_BIT, ((unsigned int)i)<<HI6403ES_DETRES_CHAIN_SEL_BIT);
		/* reset imp */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG50,
				0x1<<HI6403ES_DETRES_RST_BIT, 0x1<<HI6403ES_DETRES_RST_BIT);
		usleep_range(100,120);
		/* unreset imp */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG50,
				0x1<<HI6403ES_DETRES_RST_BIT, 0x0<<HI6403ES_DETRES_RST_BIT);
		usleep_range(100,120);

		volume_value[i] = hi6403es_get_voltagevlaue(codec);

		/* get sum of valid res value */
		if (0 != volume_value[i]) {
			calc_res_value = hi6403es_calc_res(codec, volume_value[i], i);
			pr_info("%s : afer calculated res[%d] is %d\n", __FUNCTION__, i, calc_res_value);
			if (HI6403ES_FAKE_RES_VALUE != calc_res_value) {
				data_num++;
				res_value += calc_res_value;
			}
		}
	}

	if (0 != data_num) {
		res_value = res_value / data_num;
		pr_info("%s : final res is %d\n", __FUNCTION__, res_value);
	} else {
		pr_err("%s : no valid res value! config as 32ohm!\n", __FUNCTION__);
		res_value = 32;
	}

	hi6403es_headphone_resdet_enable(codec, false);

	return res_value;
}

void hi6403es_hs_res_detect(struct snd_soc_codec *codec)
{
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);
	unsigned int res_value = 0;

	BUG_ON(NULL == priv);

	res_value = hi6403es_get_resvalue(codec);

	if (res_value < 100) {
		priv->hs_high_pga_gain = HI6403ES_HS_HIGH_0_GAIN;
		priv->hs_low_pga_gain = HI6403ES_HS_LOW_0_GAIN;
	} else {
		priv->hs_high_pga_gain = HI6403ES_HS_HIGH_1_GAIN;
		priv->hs_low_pga_gain = HI6403ES_HS_LOW_1_GAIN;
	}
}

static void hi6403es_mad_set_param(struct snd_soc_codec *codec)
{
	/* auto active time */
	snd_soc_write(codec, HI6403ES_MAD_AUTO_ACT_TIME_L, 0x0);

	/* pll time */
	snd_soc_write(codec, HI6403ES_MAD_PLL_TIME_L, 0x1);

	/* adc time */
	snd_soc_write(codec, HI6403ES_MAD_ADC_TIME_H, 0x0);
	snd_soc_write(codec, HI6403ES_MAD_ADC_TIME_L, 0x3);

	/* mad_ana_time */
	snd_soc_write(codec, HI6403ES_MAD_ANA_TIME_H, 0x0);
	snd_soc_write(codec, HI6403ES_MAD_ANA_TIME, 0x5);

	/* omt */
	snd_soc_write(codec, HI6403ES_MAD_OMIT_SAMP, 0x20);

	/* mad_vad_time */
	snd_soc_write(codec, HI6403ES_MAD_VAD_TIME_H, 0x0);
	snd_soc_write(codec, HI6403ES_MAD_VAD_TIME_L, 0xa0);

	/* mad_sleep_time */
	snd_soc_write(codec, HI6403ES_MAD_SLEEP_TIME_L, 0x0);

	/* mad_buffer_fifo_thre */
	snd_soc_write(codec, HI6403ES_MAD_BUFFER_CTRL0, 0xff);
	snd_soc_write(codec, HI6403ES_MAD_BUFFER_CTRL1, 0x1f);

	/* mad_cnt_thre,vad delay cnt */
	snd_soc_write(codec, HI6403ES_MAD_CNT_THRE, 0x4);

	/* mad_snr_thre */
	snd_soc_write(codec, HI6403ES_MAD_SNR_THRE_SUM, 0x55);
	snd_soc_write(codec, HI6403ES_MAD_SNR_THRE, 0x20);

	/* mad_min_chan_eng */
	snd_soc_write(codec, HI6403ES_MAD_MIN_CHAN_ENG, 0xa);
	/* mad_vad_num */
	snd_soc_write(codec, HI6403ES_MAD_VAD_NUM, 0x1);
	/* mad_alpha_en1 */
	snd_soc_write(codec, HI6403ES_MAD_ALPHA_EN1, 0xc);

	/* mad_vad_ao ->en, mad_irq_en->en, mad_en->en, mad_wind_sel */
	snd_soc_write(codec, HI6403ES_MAD_CTRL, 0x65);

	/* clear mad_src_fifo */
	snd_soc_write(codec, HI6403ES_MAD_SRCDN_CTRL, 0x8);
	snd_soc_write(codec, HI6403ES_MAD_SRCDN_CTRL, 0x0);

	/* clear mic1_src_fifo */
	snd_soc_write(codec, HI6403ES_MIC1_SRCDN_CTRL, 0x8);
	snd_soc_write(codec, HI6403ES_MIC1_SRCDN_CTRL, 0x0);
	/* mic2_src_fifo_clr */
	snd_soc_write(codec, HI6403ES_MIC2_SRCDN_CTRL, 0x8);
	snd_soc_write(codec, HI6403ES_MIC2_SRCDN_CTRL, 0x0);

	return;
}

enum hi64xx_pll_type hi6403es_pll_for_reg_access(struct snd_soc_codec *codec, unsigned int reg)
{
	if ((reg >= 0x20007200 && reg <= 0x20007fff) ||
		(reg >= 0x20007041 && reg <= 0x2000704c)) {
		return PLL_HIGH;
	} else {
		return PLL_NONE;
	}
}

static int hi6403es_resmgr_init(struct hi6403es_platform_data *pd)
{
	int ret = 0;
	struct resmgr_config cfg;

	cfg.pll_num = 3;
	cfg.pll_sw_mode = MODE_MULTIPLE;
	cfg.pfn_pll_ctrls[PLL_LOW].turn_on = hi6403es_pllmad_turn_on;
	cfg.pfn_pll_ctrls[PLL_LOW].turn_off = hi6403es_pllmad_turn_off;
	cfg.pfn_pll_ctrls[PLL_LOW].is_locked = hi6403es_pllmad_check;
	cfg.pfn_pll_ctrls[PLL_HIGH].turn_on = hi6403es_pll48k_turn_on;
	cfg.pfn_pll_ctrls[PLL_HIGH].turn_off = hi6403es_pll48k_turn_off;
	cfg.pfn_pll_ctrls[PLL_HIGH].is_locked = hi6403es_pll48k_check;
	cfg.pfn_pll_ctrls[PLL_44_1].turn_on = hi6403es_pll44k1_turn_on;
	cfg.pfn_pll_ctrls[PLL_44_1].turn_off = hi6403es_pll44k1_turn_off;
	cfg.pfn_pll_ctrls[PLL_44_1].is_locked = hi6403es_pll44k1_check;
	cfg.pll_for_reg_access = hi6403es_pll_for_reg_access;
	cfg.enable_ibias = hi6403es_enable_ibias;
	cfg.disable_ibias = hi6403es_disable_ibias;
	cfg.enable_micbias = hi6403es_enable_micbias;
	cfg.disable_micbias = hi6403es_disable_micbias;
	cfg.hi64xx_hs_high_resistance_enable = NULL;
	ret = hi64xx_resmgr_init(pd->codec, pd->cdc_ctrl, pd->irqmgr, &cfg, &pd->resmgr);

	return ret;
}


static void hi6403es_init_chip(struct snd_soc_codec *codec)
{
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);

	BUG_ON(NULL == priv);

	if (priv->cdc_ctrl->bus_sel == BUSTYPE_SELECT_SLIMBUS) {

		snd_soc_write(codec, HI6403ES_SSI_DATA_IOSHARE_REG, 0x1);
		snd_soc_write(codec, HI6403ES_I2S1_SYNC_IOSHARE_REG, 0x2);
		snd_soc_write(codec, HI6403ES_I2S1_CLK_IOSHARE_REG, 0x2);
		snd_soc_write(codec, HI6403ES_SLIM_DATA_IOCONFIG_REG, 0x114);
		snd_soc_write(codec, HI6403ES_SLIM_CLK_IOCONFIG_REG, 0x115);
		/* enable hi6403 slim framer */
		snd_soc_write(codec, HI6403ES_SLIM_CLK_DIV_REG, 0x53);
		/* slim&ssi mclk enable */
		snd_soc_write(codec, HI6403ES_CFG_CLK_CTRL, 0x3);
	}

	/* codec irq ioshare config */
	snd_soc_write(codec, HI6403ES_IRQ_IOSHARE_REG, 1<<HI6403ES_IRQ_EN_BIT);
	snd_soc_write(codec, HI6403ES_IRQN_IOCONFIG_REG, 0x84);
	/* slimbus ioshare config */
	snd_soc_write(codec, HI6403ES_I2S1_SYNC_IOSHARE_REG, 1<<HI6403ES_SLIM_DATA_EN_BIT);
	snd_soc_write(codec, HI6403ES_I2S1_CLK_IOSHARE_REG, 1<<HI6403ES_SLIM_CLK_EN_BIT);
	/* ssi ioshare config */
	snd_soc_write(codec, HI6403ES_SSI_DATA_IOSHARE_REG, 1<<HI6403ES_SSI_DATA_EN_BIT);
	/* I2S2 ioshare config */
	snd_soc_write(codec, HI6403ES_I2S2_SYNC_IOSHARE_REG, 1<<HI6403ES_I2S2_SYNC_EN_BIT);
	snd_soc_write(codec, HI6403ES_I2S2_CLK_IOSHARE_REG, 1<<HI6403ES_I2S2_CLK_EN_BIT);
	snd_soc_write(codec, HI6403ES_I2S2_SDI_IOSHARE_REG, 1<<HI6403ES_I2S2_SDI_EN_BIT);
	snd_soc_write(codec, HI6403ES_I2S2_SDO_IOSHARE_REG, 1<<HI6403ES_I2S2_SDO_EN_BIT);
	/* DSD ioshare config */
	//snd_soc_write(codec, HI6403ES_I2S3_SDI_IOSHARE_REG, 1<<HI6403ES_DSD_L_EN_BIT);
	//snd_soc_write(codec, HI6403ES_I2S3_SDO_IOSHARE_REG, 1<<HI6403ES_DSD_R_EN_BIT);
	/* I2S4 ioshare config */
	snd_soc_write(codec, HI6403ES_I2S4_SYNC_IOSHARE_REG, 1<<HI6403ES_I2S4_SYNC_EN_BIT);
	snd_soc_write(codec, HI6403ES_I2S4_CLK_IOSHARE_REG, 1<<HI6403ES_I2S4_CLK_EN_BIT);
	snd_soc_write(codec, HI6403ES_I2S4_SDI_IOSHARE_REG, 1<<HI6403ES_I2S4_SDI_EN_BIT);
	snd_soc_write(codec, HI6403ES_I2S4_SDO_IOSHARE_REG, 1<<HI6403ES_I2S4_SDO_EN_BIT);
	/* DMIC ioshare config */
	//snd_soc_write(codec, HI6403ES_DMIC0_CLK_IOSHARE_REG, 1<<HI6403ES_DMIC0_CLK_EN_BIT);
	//snd_soc_write(codec, HI6403ES_DMIC0_DATA_IOSHARE_REG, 1<<HI6403ES_DMIC0_DATA_EN_BIT);
	//snd_soc_write(codec, HI6403ES_DMIC1_CLK_IOSHARE_REG, 1<<HI6403ES_DMIC1_CLK_EN_BIT);
	//snd_soc_write(codec, HI6403ES_DMIC1_DATA_IOSHARE_REG, 1<<HI6403ES_DMIC1_DATA_EN_BIT);
	/* I2S2&I2S4 sdo config */
	hi64xx_update_bits(codec, HI6403ES_I2S2_SDO_IOCONFIG_REG,
			(1<<HI6403ES_I2S_SDO_INPUT_EN_BIT)|(1<<HI6403ES_I2S_SDO_OUTPUT_EN_BIT),
			(0<<HI6403ES_I2S_SDO_INPUT_EN_BIT)|(1<<HI6403ES_I2S_SDO_OUTPUT_EN_BIT));
	hi64xx_update_bits(codec, HI6403ES_I2S4_SDO_IOCONFIG_REG,
			(1<<HI6403ES_I2S_SDO_INPUT_EN_BIT)|(1<<HI6403ES_I2S_SDO_OUTPUT_EN_BIT),
			(0<<HI6403ES_I2S_SDO_INPUT_EN_BIT)|(1<<HI6403ES_I2S_SDO_OUTPUT_EN_BIT));
	/* slimbus clk schmitt config */
	hi64xx_update_bits(codec, HI6403ES_SLIM_CLK_IOCONFIG_REG,
			0x3<<HI6403ES_SLIM_SCHMITT_BIT, 0x1<<HI6403ES_SLIM_SCHMITT_BIT);
	/* slimbus pin disable pd */
	hi64xx_update_bits(codec, HI6403ES_SLIM_DATA_IOCONFIG_REG,
			0x1<<HI6403ES_IOSHARE_PIN_PD, 0x0<<HI6403ES_IOSHARE_PIN_PD);
	hi64xx_update_bits(codec, HI6403ES_SLIM_CLK_IOCONFIG_REG,
			0x1<<HI6403ES_IOSHARE_PIN_PD, 0x0<<HI6403ES_IOSHARE_PIN_PD);

	if ((priv->cdc_ctrl->platform_type == HI_CDCCTRL_PLATFORM_UDP) || (priv->cdc_ctrl->platform_type == HI_CDCCTRL_PLATFORM_FPGA)) {
		/* slimbus gpio set as 24mA for udp and FPGA */
		hi64xx_update_bits(codec, HI6403ES_SLIM_DATA_IOCONFIG_REG,
				0x7<<HI6403ES_SLIM_DATA_IO_CURRENT_BIT, 0x7<<HI6403ES_SLIM_DATA_IO_CURRENT_BIT);
		hi64xx_update_bits(codec, HI6403ES_SLIM_CLK_IOCONFIG_REG,
				0x7<<HI6403ES_SLIM_CLK_IO_CURRENT_BIT, 0x7<<HI6403ES_SLIM_CLK_IO_CURRENT_BIT);
	} else {
		/* slimbus gpio set as 16mA */
		hi64xx_update_bits(codec, HI6403ES_SLIM_DATA_IOCONFIG_REG,
				0x7<<HI6403ES_SLIM_DATA_IO_CURRENT_BIT, 0x5<<HI6403ES_SLIM_DATA_IO_CURRENT_BIT);
		hi64xx_update_bits(codec, HI6403ES_SLIM_CLK_IOCONFIG_REG,
				0x7<<HI6403ES_SLIM_CLK_IO_CURRENT_BIT, 0x5<<HI6403ES_SLIM_CLK_IO_CURRENT_BIT);
	}

	if (priv->board_config.gpio_pd_enable) {
		/* GPIO0 pd enable */
		hi64xx_update_bits(codec, HI6403ES_GPIO0_IOCONFIG_REG, 0x40, 0x40);
		/* 12.228M GPIO pd enable */
		hi64xx_update_bits(codec, HI6403ES_12M288_IOCONFIG_REG, 0x40, 0x40);
	}

	/* dsd clk disable */
	snd_soc_write(codec, HI6403ES_DSD_CLK_CFG_REG0, 0xC0);
	snd_soc_write(codec, HI6403ES_DSD_CLK_CFG_REG1, 0x80);
	/* SPK CLK GPIO pd disable */
	hi64xx_update_bits(codec, HI6403ES_SPK_CLK_IOCONFIG_REG, 0x40, 0x0);
	/* mic voltage config */
	snd_soc_write(codec, HI6403ES_MIC_3_4_CONFIG_REG, 0xBB);
	snd_soc_write(codec, HI6403ES_MIC_1_2_CONFIG_REG, 0xBB);
	snd_soc_write(codec, HI6403ES_HSMIC_CONFIG_REG, 0xB);
	/* main pll config */
	snd_soc_write(codec, HI6403ES_ANALOG_REG196, 0x24);
	/* mad pll config */
	snd_soc_write(codec, HI6403ES_ANALOG_REG197, 0xC0);
	/* main pll trk config */
	snd_soc_write(codec, HI6403ES_ANALOG_REG198, 0x88);
	/* pll ibias enable */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG195,
					1<<HI6403ES_PLL_IBIAS_GEN_BIT, 0);
	/* pll config */
	hi6403es_pll48k_config(codec);
	hi6403es_pllmad_config(codec);
	hi6403es_pll44k1_config(codec);
	/* pll lock state check enable */
	hi64xx_update_bits(codec, HI6403ES_PLL_LOCK_CFG_REG,
			1<<HI6403ES_PLL_LOCK_CHECK_BIT, 1<<HI6403ES_PLL_LOCK_CHECK_BIT);
	/* clear pll unlock state enable */
	hi64xx_update_bits(codec, HI6403ES_PLL_LOCK_CFG_REG,
					1<<HI6403ES_PLL_LOCK_CLR_BIT, 1<<HI6403ES_PLL_LOCK_CLR_BIT);
	/* 12M288&6M144 clk enable */
	hi64xx_update_bits(codec, HI6403ES_AUDIO_CLK_CFG_REG,
			0x3<<HI6403ES_ADUIO_12M288CLK_EN_BIT, 0x3<<HI6403ES_ADUIO_12M288CLK_EN_BIT);

	/* headphone irq ctrl -> reg */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG43,
			0x1<<HI6403ES_HSD_IRQ_LINK_DIS_BIT, 0x1<<HI6403ES_HSD_IRQ_LINK_DIS_BIT);
	/* headphone pop pull set not 0 */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG43,
			0x3F<<HI6403ES_HPL_POP_PULL_2_0_BIT, 0x8<<HI6403ES_HPL_POP_PULL_2_0_BIT);
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG44,
			0x3F<<HI6403ES_HPR_POP_PULL_2_0_BIT, 0x8<<HI6403ES_HPR_POP_PULL_2_0_BIT);

	/* headphone pop config */
	snd_soc_write(codec, HI6403ES_HPL_POP_CFG_REG, 0x7F);
	snd_soc_write(codec, HI6403ES_HPR_POP_CFG_REG, 0x7F);
	/* pop auto enable */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG47,
			1<<HI6403ES_HP_POP_AUTO_EN_BIT, 1<<HI6403ES_HP_POP_AUTO_EN_BIT);
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG47,
			0x3<<HI6403ES_HP_POP_RAMP_SPD_CFG_BIT, 0x2<<HI6403ES_HP_POP_RAMP_SPD_CFG_BIT);
	/* pop clk power on */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG15,
			1<<HI6403ES_POP_CLK_PD_BIT, 0);
	/* pop clk rst */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG47,
			1<<HI6403ES_HP_POP_RST_BIT, 0);

	/* ISO config */ /* bit1 change with 1do8 */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG97,
			0x3<<HI6403ES_ISO_A33_BIT, 0x3<<HI6403ES_ISO_A33_BIT);
	/* cp1 catch buck1 */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG108,
			1<<HI6403ES_CP1_EN_SEL_BIT, 0);
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG130,
			1<<HI6403ES_BUCK1_PD_SINGLE_BIT, 0);
	/* cp2 catch buck2 */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG121,
			1<<HI6403ES_CP2_EN_SEL_BIT, 0);
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG130,
			1<<HI6403ES_BUCK2_PD_SINGLE_BIT, 0);

	/* I2S4 config (fs init 48k) */
	snd_soc_write(codec, HI6403ES_S4_CFG_REG_H, 0x70);
	snd_soc_write(codec, HI6403ES_S4_CLK_CFG_REG, 0x4);
	snd_soc_write(codec, HI6403ES_S4_CLK_CFG_REG1, 0x44);
	/* S2 (uplink fs init 8k) */
	/* nothing */
	/* slimbus down1&2 port fs config */
	snd_soc_write(codec, HI6403ES_SLIM0_CLK_CFG_REG, 0x44);
	/* slimbus down3&4port fs config */
	snd_soc_write(codec, HI6403ES_SLIM1_CLK_CFG_REG, 0x44);
	/* slimbus up1&2 port fs config */
	snd_soc_write(codec, HI6403ES_SLIM3_CLK_CFG_REG, 0x44);
	/* slimbus up3&4 port fs config */
	snd_soc_write(codec, HI6403ES_SLIM4_CLK_CFG_REG, 0x44);
	/* slimbus up5&6 port fs config */
	snd_soc_write(codec, HI6403ES_SLIM5_CLK_CFG_REG, 0x11);
	/* slimbus up7&8 port fs config */
	snd_soc_write(codec, HI6403ES_SLIM6_CLK_CFG_REG, 0x44);
	/* I2S/SLIM SEL */
	snd_soc_write(codec, HI6403ES_I2S_SLIM_SEL_REG, 0x33);
	/* adc source select */
	snd_soc_write(codec, HI6403ES_ADC_SOURCE_SEL, 0x1f);
	/* mixer fs config */
	hi64xx_update_bits(codec, HI6403ES_DACL_FLT_MIXER_SEL,
			0x1<<HI6403ES_DACL_FLT_MIXER_FS_CFG_BIT, 0x1<<HI6403ES_DACL_FLT_MIXER_FS_CFG_BIT);
	hi64xx_update_bits(codec, HI6403ES_DACR_FLT_MIXER_SEL,
			0x1<<HI6403ES_DACR_FLT_MIXER_FS_CFG_BIT, 0x1<<HI6403ES_DACR_FLT_MIXER_FS_CFG_BIT);
	hi64xx_update_bits(codec, HI6403ES_DACSL_MIXER_SEL_1,
			0x1<<HI6403ES_DACSL_MIXER_FS_BIT, 0x1<<HI6403ES_DACSL_MIXER_FS_BIT);
	hi64xx_update_bits(codec, HI6403ES_DACSR_MIXER_SEL_1,
			0x1<<HI6403ES_DACSR_MIXER_FS_BIT, 0x1<<HI6403ES_DACSR_MIXER_FS_BIT);
	/* dsp if config */
	hi64xx_update_bits(codec, HI6403ES_S1_DSPIF_CONTROL_REG,
			0x3<<HI6403ES_S1_DSPIF_IN_BIT, 0x3<<HI6403ES_S1_DSPIF_IN_BIT);
	hi64xx_update_bits(codec, HI6403ES_S2_DSPIF_CONTROL_REG,
			0x3<<HI6403ES_S2_DSPIF_IN_BIT, 0x3<<HI6403ES_S2_DSPIF_IN_BIT);
	hi64xx_update_bits(codec, HI6403ES_S3_DSPIF_CONTROL_REG,
			0x3<<HI6403ES_S3_DSPIF_IN_BIT, 0x3<<HI6403ES_S3_DSPIF_IN_BIT);
	hi64xx_update_bits(codec, HI6403ES_S4_DSPIF_CONTROL_REG,
			0x3<<HI6403ES_S4_DSPIF_IN_BIT, 0x3<<HI6403ES_S4_DSPIF_IN_BIT);
	/* slimbus frame config */
	snd_soc_write(codec, HI6403ES_SLIM_CLK_DIV_REG, 0x53);
	/* sdm cfg *//* 139-144 */

	/* dac src mux init */
	snd_soc_write(codec, HI6403ES_DAC_SRC_MUX_SEL, 0x88);
	/* mux for sdm init */
	snd_soc_write(codec, HI6403ES_DAC_HP_MUX_SEL, 0x1B);
	snd_soc_write(codec, HI6403ES_DACS_MUX_SEL, 0x2D);
	snd_soc_write(codec, HI6403ES_DAC_LTR_MUX_SEL, 0x5);
	/* headphone detect config */
	snd_soc_write(codec, HI6403ES_HP_DET_CFG_REG, 0x9);
	/* mixer gian config */
	snd_soc_write(codec, HI6403ES_DACL_MIXER_GAIN_CFG_REG, 0xFF);
	snd_soc_write(codec, HI6403ES_DACR_MIXER_GAIN_CFG_REG, 0xFF);
	snd_soc_write(codec, HI6403ES_DACSL_MIXER_GAIN_CFG_REG, 0xFF);
	snd_soc_write(codec, HI6403ES_DACSR_MIXER_GAIN_CFG_REG, 0xFF);
	snd_soc_write(codec, HI6403ES_DACL_SRC_MIXER_GAIN_REG, 0xF);
	snd_soc_write(codec, HI6403ES_DACR_SRC_MIXER_GAIN_REG, 0xF);
	hi64xx_update_bits(codec, HI6403ES_DACL_FLT_MIXER_GAIN_CFG_REG, 0xF, 0xF);
	hi64xx_update_bits(codec, HI6403ES_DACR_FLT_MIXER_GAIN_CFG_REG, 0xF, 0xF);
	/* class A/B config */
	snd_soc_write(codec, HI6403ES_CLASS_H_CFG_REG0, 0xF);
	snd_soc_write(codec, HI6403ES_CLASS_H_CFG_REG1, 0x37);

	/* valid config */
	hi64xx_update_bits(codec, HI6403ES_HPL_SDM45_CONFIG_REG0, 0xC8, 0x88);
	hi64xx_update_bits(codec, HI6403ES_HPR_SDM45_CONFIG_REG0, 0xC8, 0x88);
	hi64xx_update_bits(codec, HI6403ES_DACL_SDM45_MUX_SEL, 0xFC, 0x9C);
	hi64xx_update_bits(codec, HI6403ES_DACR_SDM45_MUX_SEL, 0xFC, 0x9C);
	hi64xx_update_bits(codec, HI6403ES_DACL_SDM_MUX_SEL, 0x8, 0x8);
	hi64xx_update_bits(codec, HI6403ES_DACR_SDM_MUX_SEL, 0x8, 0x8);
	hi64xx_update_bits(codec, HI6403ES_DACSL_SDM_MUX_SEL, 0x8, 0x8);
	hi64xx_update_bits(codec, HI6403ES_DACSR_SDM_MUX_SEL, 0x8, 0x8);
	/* mic gain init */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG23,
			0x1F<<HI6403ES_MAIN_PGA_GAIN_BIT, 0x11<<HI6403ES_MAIN_PGA_GAIN_BIT);
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG22,
			0x1F<<HI6403ES_AUX_PGA_GAIN_BIT, 0x11<<HI6403ES_AUX_PGA_GAIN_BIT);
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG21,
			0x1F<<HI6403ES_LINEINR_PGA_GAIN_BIT, 0x11<<HI6403ES_LINEINR_PGA_GAIN_BIT);
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG20,
			0x1F<<HI6403ES_LINEINL_PGA_GAIN_BIT, 0x11<<HI6403ES_LINEINL_PGA_GAIN_BIT);
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG84,
			0x1F<<HI6403ES_MAD_PGA_GAIN_BIT, 0x11<<HI6403ES_MAD_PGA_GAIN_BIT);
	/* mad src mode config */
	hi64xx_update_bits(codec, HI6403ES_MAD_SRC_CFG_REG,
			0x1F<<HI6403ES_MAD_SRC_MODE_BIT, 0);
	/* 0.5dB step pga gain init -> 0dB */
	snd_soc_write(codec, HI6403ES_DACL_PGA_GAIN_CFG_REG, 0xBA);
	snd_soc_write(codec, HI6403ES_DACR_PGA_GAIN_CFG_REG, 0xBA);
	snd_soc_write(codec, HI6403ES_DACSL_PGA_GAIN_CFG_REG, 0x78);
	snd_soc_write(codec, HI6403ES_ADC0L_05PGA_GAIN_REG, 0x78);
	snd_soc_write(codec, HI6403ES_ADC0R_05PGA_GAIN_REG, 0x78);
	snd_soc_write(codec, HI6403ES_ADC1L_05PGA_GAIN_REG, 0x78);
	snd_soc_write(codec, HI6403ES_ADC1R_05PGA_GAIN_REG, 0x78);

	/* I2S1 config fs init 48k */
	hi64xx_update_bits(codec, HI6403ES_S1_CLK_CFG_REG0, 0x7, 0x4);
	/* S1 DSPIF */
	snd_soc_write(codec, HI6403ES_S1_CLK_CFG_REG1, 0x44);

	/* disable i2c master clk */
	hi64xx_update_bits(codec, HI6403ES_DSP_PERI_CLKEN1,
			0x1<<HI6403ES_I2C_MST_CLK_EN, 0);
	/* buck pll dig clk select 19M2 */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG200, 0x40, 0x0);
	/* buck1 ramp config */
	hi64xx_update_bits(codec, HI6403ES_CODEC_ANA_RWREG_107,
			0x3<<HI6403ES_BUCK1_REG_R, 0x0<<HI6403ES_BUCK1_REG_R);
	/* config buck1 dmd */
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_102, 0x61);
	/* buck1 bandwidth config */
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_99, 0x2F);
	/* buck1 config */
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_100, 0x18);
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_101, 0xDE);
	/* buck1 p triode config */
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_106, 0x32);
	/* buck2 p triode config */
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_119, 0x32);
	/* buck2 ramp config */
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_109, 0x22);
	/* config buck2 dmd */
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_116, 0xD5);
	/* buck2 bandwidth config */
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_113, 0xFC);
	/* buck2 config */
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_114, 0x9A);
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_115, 0xA2);
	/* buck1 Vout config */
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_137, 0x76);
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_138, 0x41);
	/* buck1 clk div select */
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_139, 0x86);
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_140, 0x14);
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_141, 0x12);
	/* buck1 mos config */
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_142, 0x66);
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_143, 0xBB);
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_144, 0x66);
	/* cp1 Vout config */
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_145, 0xC7);
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_146, 0x42);
	/* cp1 config */
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_147, 0xA9);
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_148, 0x3A);
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_132, 0x23);
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_133, 0x1C);
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_134, 0x0E);
	/* buck2 config */
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_135, 0x02);
	/* cp2 config */
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_136, 0x02);
	/* buck1 pull down disable */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG111, 0x80, 0x80);
	/* modify 0113 */
	//snd_soc_write(codec, HI6403ES_ANALOG_REG6, 0xFE); /* no need */
	snd_soc_write(codec, HI6403ES_HPL_SDM45_CONFIG_REG1, 0x08);
	snd_soc_write(codec, HI6403ES_HPL_SDM45_CONFIG_REG2, 0x82);
	snd_soc_write(codec, HI6403ES_HPL_SDM45_CONFIG_REG3, 0x9C);
	snd_soc_write(codec, HI6403ES_HPR_SDM45_CONFIG_REG1, 0x08);
	snd_soc_write(codec, HI6403ES_HPR_SDM45_CONFIG_REG2, 0x82);
	snd_soc_write(codec, HI6403ES_HPR_SDM45_CONFIG_REG3, 0x9C);
	snd_soc_write(codec, HI6403ES_CODEC_ANA_RWREG_80, 0x09);
	//hi64xx_update_bits(codec, HI6403ES_ANALOG_REG15, 0xFE, 0xFC); /* no need */

	/* classH config */
	snd_soc_write(codec, HI6403ES_CLASSH_HP_EP_CTRL, 0x30);
	if (priv->board_config.classh_rcv_hp_switch)
		priv->rcv_hp_classh_state |= rcv_classh_state;
	else
		priv->rcv_hp_classh_state &= ~rcv_classh_state;/*lint !e64*/
	set_classh_config(codec, priv->rcv_hp_classh_state);

	/* classA/B -> classH  */
	hi64xx_update_bits(codec, HI6403ES_CLASS_H_CFG_REG1,
			0x1<<HI6403ES_CLASS_H_SEL_BIT, 0x1<<HI6403ES_CLASS_H_SEL_BIT);
	priv->rcv_hp_classh_state |= hp_classh_state;

	if (true == priv->board_config.hp_high_low_change_enable) {
		hi6403es_headphone_low_mode_shadow_config(codec);
		/* headphone ib05 current init */
		hi64xx_update_bits(codec, HI6403ES_ANALOG_REG73,
				0xF<<HI6403ES_IB05_HP_BIT, 0x4<<HI6403ES_IB05_HP_BIT);
		/* enable the config for shadow gain */
		hi64xx_update_bits(codec, HI6403ES_PERF_SWITCH_CTRL8,
				0x1<<HI6403ES_PERF_SWITCH_EN_BIT, 0x1<<HI6403ES_PERF_SWITCH_EN_BIT);
		/* enable the trigger for shadow gain */
		hi64xx_update_bits(codec, HI6403ES_PERF_SWITCH_CTRL8,
				0x1<<HI6403ES_PERF_TRIGGER_EN_BIT, 0x1<<HI6403ES_PERF_TRIGGER_EN_BIT);
	} else {
		hi6403es_headphone_high_mode_shadow_config(codec);
	}

	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG14,
			0x3F<<HI6403ES_CHOPPER_BIT, 0x3F<<HI6403ES_CHOPPER_BIT);
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG15,
			0xF<<HI6403ES_MADPGA_CHOP_BPS_BIT, 0xF<<HI6403ES_MADPGA_CHOP_BPS_BIT);
	/* imp_det enable */
	hi64xx_update_bits(codec, HI6403ES_IMP_DET_CTRL_REG,
			0x1F<<HI6403ES_IMP_SAMP_CFG, 0x6<<HI6403ES_IMP_SAMP_CFG);
	/* detres sel -> 300 */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG50,
			0x1<<HI6403ES_DETRES_SEL_BIT, 0x1<<HI6403ES_DETRES_SEL_BIT);
	/* mic dischg disable */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG093, 0x1F<<HI6403ES_MIC4_DISCHG_EN_BIT, 0);
	/* dac pga fade enable */
	hi64xx_update_bits(codec, HI6403ES_DACL_PGA_CFG_REG,
			0x1<<HI6403ES_DACL_PGA_FD_EN_BIT, 0x1<<HI6403ES_DACL_PGA_FD_EN_BIT);
	hi64xx_update_bits(codec, HI6403ES_DACR_PGA_CFG_REG,
			0x1<<HI6403ES_DACR_PGA_FD_EN_BIT, 0x1<<HI6403ES_DACR_PGA_FD_EN_BIT);
	/* s3 IL pga fade enable */
	hi64xx_update_bits(codec, HI6403ES_S3_PGA_IL_CFG_REG,
			0x1<<HI6403ES_S3_PGA_IL_FD_EN_BIT, 0x1<<HI6403ES_S3_PGA_IL_FD_EN_BIT);

	/* lutlb for earphone for hi6403eses */
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG0, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG1, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG2, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG3, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG4, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG5, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG6, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG7, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG8, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG9, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG10, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG11, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG12, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG13, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG14, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG15, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG16, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG17, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG18, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG19, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG20, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG21, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG22, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG23, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG24, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG25, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG26, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG27, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG28, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG29, 0x37);
	snd_soc_write(codec, HI6403ES_LUT1B_CFG_REG30, 0x37);

	/* zero cross config for classH */
	snd_soc_write(codec, HI6403ES_PERF_SWITCH_CTRL3, 0x00);
	snd_soc_write(codec, HI6403ES_PERF_SWITCH_CTRL2, 0x00);
	snd_soc_write(codec, HI6403ES_PERF_SWITCH_CTRL1, 0x80);
	snd_soc_write(codec, HI6403ES_PERF_SWITCH_CTRL0, 0x00);
	snd_soc_write(codec, HI6403ES_PERF_SWITCH_CTRL5, 0x00);
	snd_soc_write(codec, HI6403ES_PERF_SWITCH_CTRL4, 0x80);

	/* set hsd_vth -> 1.65v */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG98,
			0x3<<HI6403ES_HSD_VTH_SEL_BIT, 0x2<<HI6403ES_HSD_VTH_SEL_BIT);
	/* set hsd -> cmp */
	hi64xx_update_bits(codec, HI6403ES_ANALOG_REG98,
			0x1<<HI6403ES_HSD_SEL_BIT, 0x1<<HI6403ES_HSD_SEL_BIT);
	/* init config for cmp */
	snd_soc_write(codec, HI6403ES_MBHD_COMP_CFG_REG, 0x1E);

	/* digital pga zc point -> 0 */
	hi64xx_update_bits(codec, HI6403ES_PGA_ZC_CFG_REG_0,
			0x1F<<HI6403ES_PGA_ZC_CFG_BIT, 0x0<<HI6403ES_PGA_ZC_CFG_BIT);
	hi64xx_update_bits(codec, HI6403ES_PGA_ZC_CFG_REG_1,
			0x1F<<HI6403ES_PGA_ZC_CFG_BIT, 0x0<<HI6403ES_PGA_ZC_CFG_BIT);
	hi64xx_update_bits(codec, HI6403ES_PGA_ZC_CFG_REG_2,
			0x1F<<HI6403ES_PGA_ZC_CFG_BIT, 0x0<<HI6403ES_PGA_ZC_CFG_BIT);
	hi64xx_update_bits(codec, HI6403ES_PGA_ZC_CFG_REG_3,
			0x1F<<HI6403ES_PGA_ZC_CFG_BIT, 0x0<<HI6403ES_PGA_ZC_CFG_BIT);
	hi64xx_update_bits(codec, HI6403ES_PGA_ZC_CFG_REG_4,
			0x1F<<HI6403ES_PGA_ZC_CFG_BIT, 0x0<<HI6403ES_PGA_ZC_CFG_BIT);
	hi64xx_update_bits(codec, HI6403ES_PGA_ZC_CFG_REG_5,
			0x1F<<HI6403ES_PGA_ZC_CFG_BIT, 0x0<<HI6403ES_PGA_ZC_CFG_BIT);
	hi64xx_update_bits(codec, HI6403ES_PGA_ZC_CFG_REG_6,
			0x1F<<HI6403ES_PGA_ZC_CFG_BIT, 0x0<<HI6403ES_PGA_ZC_CFG_BIT);
	hi64xx_update_bits(codec, HI6403ES_PGA_ZC_CFG_REG_7,
			0x1F<<HI6403ES_PGA_ZC_CFG_BIT, 0x0<<HI6403ES_PGA_ZC_CFG_BIT);
	hi64xx_update_bits(codec, HI6403ES_PGA_ZC_CFG_REG_8,
			0x1F<<HI6403ES_PGA_ZC_CFG_BIT, 0x0<<HI6403ES_PGA_ZC_CFG_BIT);
}

static int hi6403es_utils_init(struct hi6403es_platform_data *pd)
{
	int ret = 0;
	struct utils_config cfg;

	cfg.hi64xx_dump_reg = hi6403es_dump_reg;
	ret = hi64xx_utils_init(pd->codec, pd->cdc_ctrl, &cfg, pd->resmgr);

	return ret;
}

static struct mbhc_reg hi6403es_mbhc_reg = {
	.irq_source_reg = 0x2000701c,
	.irq_mbhc_2_reg = 0x20007016,
	.ana_60_reg = 0x200070fa,
	.saradc_value_reg = 0x2000716e,
	.mbhc_vref_reg = 0,
	.sar_cfg_reg = 0x200070fc,
	.micbias_eco_reg = 0x200070fe,
	.hsdet_ctrl_reg = 0x200070fb,
};

static struct hs_res_detect_func hi6403es_hs_res_detect_func = {
	.hs_res_detect = hi6403es_hs_res_detect,
	.hs_path_enable = hi6403es_headphone_path_enable,
	.hs_path_disable = hi6403es_headphone_path_disable,
};

static struct hs_res_detect_func hi6403es_hs_res_detect_func_null = {
	.hs_res_detect = NULL,
	.hs_path_enable = NULL,
	.hs_path_disable = NULL,
};

static irqreturn_t hi6403es_bunk1_ocp_handler(int irq, void *data)
{
	pr_warn("hi6403: bunk1_ocp irq received!!!\n");

	return IRQ_HANDLED;
}

static irqreturn_t hi6403es_bunk1_scp_handler(int irq, void *data)
{
	pr_warn("hi6403: bunk1_scp irq received!!!\n");

	return IRQ_HANDLED;
}

static irqreturn_t hi6403es_bunk2_ocp_handler(int irq, void *data)
{
	pr_warn("hi6403: bunk2_ocp irq received!!!\n");

	return IRQ_HANDLED;
}

static irqreturn_t hi6403es_bunk2_scp_handler(int irq, void *data)
{
	pr_warn("hi6403: bunk2_scp irq received!!!\n");

	return IRQ_HANDLED;
}

static irqreturn_t hi6403es_cp1_short_handler(int irq, void *data)
{
	pr_warn("hi6403: cp1_short irq received!!!\n");

	return IRQ_HANDLED;
}

static irqreturn_t hi6403es_cp2_short_handler(int irq, void *data)
{
	pr_warn("hi6403: cp2_short irq received!!!\n");

	return IRQ_HANDLED;
}

static irqreturn_t hi6403es_pll_unlock_handler(int irq, void *data)
{
	pr_warn("hi6403: pll_unlock irq received!!!\n");

	return IRQ_HANDLED;
}

static irqreturn_t hi6403es_pll44k1_unlock_handler(int irq, void *data)
{
	pr_warn("hi6403: pll44k1_unlock irq received!!!\n");

	return IRQ_HANDLED;
}

static irqreturn_t hi6403es_pllmad_unlock_handler(int irq, void *data)
{
	pr_warn("hi6403: pllmad_unlock irq received!!!\n");

	return IRQ_HANDLED;
}

static int hi6403es_codec_probe(struct snd_soc_codec *codec)
{
	int ret = 0;
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);

	if (!priv) {
		dev_err(codec->dev, "%s:get hi6403es platform data fail \n", __FUNCTION__);
		return -ENOENT;
	}

	snd_soc_codec_set_drvdata(codec, priv);

	priv->codec = codec;
	g_hi6403es_codec= codec;

	mutex_init(&priv->rw_mutex);
	mutex_init(&priv->dapm_mutex);
	mutex_init(&priv->impdet_dapm_mutex);

	priv->dp_clk_num = 0;
	priv->cp1_num = 0;
	priv->cp2_num = 0;

	ret = hi6403es_resmgr_init(priv);
	if (0 != ret) {
		dev_err(codec->dev, "%s: hi6403es_resmgr_init fail. err code is %x .\n", __FUNCTION__, ret);
		goto resmgr_init_err_exit;
	}

	pr_err("version register is %x\n", snd_soc_read(codec, HI64xx_VERSION));

	hi6403es_init_chip(codec);

	if (priv->board_config.hp_res_detect_enable) {
		ret = hi64xx_mbhc_init(codec, priv->node, &hi6403es_mbhc_reg, &hi6403es_hs_res_detect_func,
			priv->resmgr, priv->irqmgr, &priv->mbhc);
	} else {
		ret = hi64xx_mbhc_init(codec, priv->node, &hi6403es_mbhc_reg, &hi6403es_hs_res_detect_func_null,
			priv->resmgr, priv->irqmgr, &priv->mbhc);
	}

	if (0 != ret) {
		dev_err(codec->dev, "%s: hi6403es_mbhc_init fail. err code is %x .\n", __FUNCTION__, ret);
		goto mbhc_init_err_exit;
	}

	hi6403es_mad_set_param(codec);
	ret = hi6403es_hifi_config_init(codec, priv->resmgr, priv->irqmgr, priv->cdc_ctrl->bus_sel);
	if (0 != ret) {
		dev_err(codec->dev, "%s: hi6403es_hifi_config_init fail. err code is %x .\n", __FUNCTION__, ret);
		goto misc_init_err_exit;
	}

	ret = hi6403es_utils_init(priv);
	if (0 != ret) {
		dev_err(codec->dev, "%s: hi6403es_utils_init fail. err code is %x .\n", __FUNCTION__, ret);
		goto utils_init_err_exit;
	}

	ret = hi64xx_vad_init(codec, priv->irqmgr);
	if (0 != ret) {
		dev_err(codec->dev, "%s: hi6403es_vad_init fail. err code is %x .\n", __FUNCTION__, ret);
		goto vad_init_err_exit;
	}

	if (priv->cdc_ctrl->pm_runtime_support) {
		/* open codec pll and soc asp clk to make sure codec framer be enumerated */
		hi64xx_resmgr_request_pll(priv->resmgr, PLL_HIGH);
		slimbus_track_activate(SLIMBUS_DEVICE_HI6403, SLIMBUS_TRACK_AUDIO_PLAY, NULL);
		msleep(1);
		slimbus_track_deactivate(SLIMBUS_DEVICE_HI6403, SLIMBUS_TRACK_AUDIO_PLAY, NULL);
		hi64xx_resmgr_release_pll(priv->resmgr, PLL_HIGH);
	}

	dev_info(codec->dev, "%s: OK.\n", __FUNCTION__);
	return ret;

vad_init_err_exit:
utils_init_err_exit:
misc_init_err_exit:
mbhc_init_err_exit:
resmgr_init_err_exit:
	hi64xx_resmgr_deinit(priv->resmgr);

	dev_err(codec->dev, "%s: fail\n",__FUNCTION__);

	return ret;
}

static int hi6403es_codec_remove(struct snd_soc_codec *codec)
{
	hi6403es_hifi_config_deinit();
	hi64xx_utils_deinit();
	return 0;
}

static unsigned int hi6403es_reg_read(struct snd_soc_codec *codec,
		unsigned int reg)
{
	unsigned int ret;
	unsigned int reg_mask;
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == priv);

	reg_mask = reg & 0xFFFFF000;
	if (0x7000 == reg_mask || 0x1000 == reg_mask) {
		reg = reg | 0x20000000;
	} else if (0xc000 == reg_mask ) {
		mutex_lock(&priv->rw_mutex);
		ret = priv->mm_reg;
		mutex_unlock(&priv->rw_mutex);
		return ret;
	} else {

	}

	hi64xx_resmgr_request_reg_access(priv->resmgr, reg);
	ret = hi_cdcctrl_reg_read(priv->cdc_ctrl, reg);
	hi64xx_resmgr_release_reg_access(priv->resmgr, reg);

	return ret;
}

static int hi6403es_reg_write(struct snd_soc_codec *codec,
		unsigned int reg, unsigned int value)
{
	int ret = 0;
	unsigned int reg_mask;
	struct hi6403es_platform_data *priv = snd_soc_codec_get_drvdata(codec);
	BUG_ON(NULL == priv);

	reg_mask = reg & 0xFFFFF000;
	if (0x7000 == reg_mask || 0x1000 == reg_mask) {
		reg = reg | 0x20000000;
	} else if (0xc000 == reg_mask ) {
		mutex_lock(&priv->rw_mutex);
		priv->mm_reg = value;
		mutex_unlock(&priv->rw_mutex);
		return ret;
	} else {

	}

#ifdef CONFIG_HISI_DEBUG_FS
		/*record reg*/
		hi64xx_reg_wr_cache(reg, value);
#endif

	hi64xx_resmgr_request_reg_access(priv->resmgr, reg);
	ret = hi_cdcctrl_reg_write(priv->cdc_ctrl, reg, value);
	hi64xx_resmgr_release_reg_access(priv->resmgr, reg);
	return ret;
}


static void hi6403es_compat_deinit(void)
{
	hi64xx_compat_deinit();
}

static void hi6403es_get_board_micnum(struct device_node *node, struct hi6403es_board_cfg *board_cfg)
{
	unsigned int val = 0;

	if (!of_property_read_u32(node, "hisilicon,mic_num", &val)) {
		board_cfg->mic_num = val;
	} else {
		board_cfg->mic_num = 2;
	}
}

static void hi6403es_get_board_pa(struct device_node *node, struct hi6403es_board_cfg *board_cfg)
{
	unsigned int val = 0;

	if (!of_property_read_u32(node, "use_stereo_smartpa", &val)){
		if(val){
			board_cfg->use_stereo_smartpa = true;
		} else {
			board_cfg->use_stereo_smartpa = false;
		}
	} else {
		board_cfg->use_stereo_smartpa = false;
	}
}

static void hi6403es_get_board_hpswitch(struct device_node *node, struct hi6403es_board_cfg *board_cfg)
{
	unsigned int val = 0;

	if (!of_property_read_u32(node, "hisilicon,classh_rcv_hp_switch", &val)){
		if(val){
			board_cfg->classh_rcv_hp_switch = true;
		} else {
			board_cfg->classh_rcv_hp_switch = false;
		}
	} else {
		board_cfg->classh_rcv_hp_switch = false;
	}
}

static void hi6403es_get_board_high_low_change(struct device_node *node, struct hi6403es_board_cfg *board_cfg)
{
	unsigned int val = 0;

	if (!of_property_read_u32(node, "hisilicon,hp_high_low_change_enable", &val)){
		if(val){
			board_cfg->hp_high_low_change_enable = true;
		} else {
			board_cfg->hp_high_low_change_enable = false;
		}
	} else {
		board_cfg->hp_high_low_change_enable = false;
	}
}

static void hi6403es_get_board_hp_res_detect(struct device_node *node, struct hi6403es_board_cfg *board_cfg)
{
	unsigned int val = 0;

	if (!of_property_read_u32(node, "hisilicon,hp_res_detect_enable", &val)){
		if(val){
			board_cfg->hp_res_detect_enable = true;
		} else {
			board_cfg->hp_res_detect_enable = false;
		}
	} else {
		board_cfg->hp_res_detect_enable = false;
	}
}

static void hi6403es_get_board_gpio_pd_status(struct device_node *node, struct hi6403es_board_cfg *board_cfg)
{
	if (of_property_read_bool(node, "gpio_pd_enable")) {
		board_cfg->gpio_pd_enable = true;
	} else {
		board_cfg->gpio_pd_enable = false;
	}
}

static void hi6403es_get_board_cfg(struct device_node *node, struct hi6403es_board_cfg *board_cfg)
{
	hi6403es_get_board_micnum(node, board_cfg);
	hi6403es_get_board_pa(node, board_cfg);
	hi6403es_get_board_hpswitch(node, board_cfg);
	hi6403es_get_board_high_low_change(node, board_cfg);
	hi6403es_get_board_hp_res_detect(node, board_cfg);
	hi6403es_get_board_gpio_pd_status(node, board_cfg);
}

static int hi6403es_irq_init(struct hi64xx_irq *irq_data)
{
	struct hi64xx_irq_map irqmap;
	int ret = 0;

	if (HI64XX_MAX_IRQ_REGS_NUM >= 4) {
		irqmap.irq_regs[0] = HI64xx_REG_IRQ_0;
		irqmap.irq_regs[1] = HI64xx_REG_IRQ_1;
		irqmap.irq_regs[2] = HI64xx_REG_IRQ_2;
		irqmap.irq_regs[3] = HI64xx_REG_IRQ_3;
		irqmap.irq_regs[4] = HI6403ES_REG_IRQ_4;
		irqmap.irq_regs[5] = HI6403ES_REG_IRQ_5;
		irqmap.irq_mask_regs[0] = HI64xx_REG_IRQM_0;
		irqmap.irq_mask_regs[1] = HI64xx_REG_IRQM_1;
		irqmap.irq_mask_regs[2] = HI64xx_REG_IRQM_2;
		irqmap.irq_mask_regs[3] = HI64xx_REG_IRQM_3;
		irqmap.irq_mask_regs[4] = HI6403ES_REG_IRQM_4;
		irqmap.irq_mask_regs[5] = HI6403ES_REG_IRQM_5;
		irqmap.irq_num = HI6403ES_IRQ_NUM;

		ret = hi64xx_irq_init_irq(irq_data, &irqmap);
	} else {
		pr_err("%s: array size extend! \n", __FUNCTION__);
		ret = -ENOENT;
	}

	return ret;
}

static struct snd_soc_codec_driver hi6403es_codec_driver = {
	.probe = hi6403es_codec_probe,
	.remove = hi6403es_codec_remove,
	.read = hi6403es_reg_read,
	.write = hi6403es_reg_write,

	.controls = hi6403es_snd_controls,
	.num_controls = ARRAY_SIZE(hi6403es_snd_controls),
	.dapm_widgets = hi6403es_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(hi6403es_dapm_widgets),
	.dapm_routes = route_map,
	.num_dapm_routes = ARRAY_SIZE(route_map),
};
/*lint -e429*/
static int hi6403es_platform_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct hi6403es_platform_data *priv = NULL;
	const struct of_device_id *match = NULL;
	int ret = 0;

	priv = devm_kzalloc(dev, sizeof(struct hi6403es_platform_data), GFP_KERNEL);
	if (NULL == priv) {
		dev_err(dev, "%s : kzalloc failed\n", __FUNCTION__);
		return -ENOMEM;
	}
	priv->node = dev->of_node;
	match = of_match_device(hi6403es_platform_match, dev);
	if (!match) {
		dev_err(dev, "%s : get device info err\n", __FUNCTION__);
		ret = -ENOENT;
		goto free_platform_data;
	} else {
		hi6403es_get_board_cfg(priv->node, &priv->board_config);
		dev_info(dev, "%s : mic_num %d , use_stereo_smartpa %d, classh_rcv_hp_switch %d, hp_high_low_change_enable %d, hp_res_detect_enable %d, gpio_pd_enable %d\n",
			__FUNCTION__, priv->board_config.mic_num,
			priv->board_config.use_stereo_smartpa,
			priv->board_config.classh_rcv_hp_switch,
			priv->board_config.hp_high_low_change_enable,
			priv->board_config.hp_res_detect_enable,
			priv->board_config.gpio_pd_enable);
	}

	priv->irqmgr = (struct hi64xx_irq *)dev_get_drvdata(pdev->dev.parent);
	priv->cdc_ctrl = (struct hi_cdc_ctrl*)dev_get_drvdata(pdev->dev.parent->parent);
	priv->voice_params.channels = 2;
	priv->voice_params.rate = SLIMBUS_SAMPLE_RATE_16K;
	priv->capture_params.channels = 2;
	priv->capture_params.rate = SLIMBUS_SAMPLE_RATE_48K;
	priv->play_params.channels = 2;
	priv->play_params.rate = SLIMBUS_SAMPLE_RATE_48K;
	priv->res_value = 300;
	priv->hs_high_pga_gain = HI6403ES_HS_HIGH_0_GAIN;
	priv->hs_low_pga_gain = HI6403ES_HS_LOW_0_GAIN;
	priv->voiceup_state = TRACK_FREE;
	priv->audioup_4mic_state = TRACK_FREE;

	if (2 < priv->board_config.mic_num)
		priv->voice_params.channels = 4;

	ret = hi_cdcctrl_reg_read(priv->cdc_ctrl, HI64xx_VERSION_REG);
	if (ret != HI6403ES_VERSION_VALUE) {
		ret = -ENODEV;
		dev_err(dev, "[%s:%d]: read hi6403es version fail, ret=0x%x\n", __FUNCTION__, __LINE__, ret);
		goto free_platform_data;
	}

	platform_set_drvdata(pdev, priv);
	dev_set_name(dev, "hi6403es-codec");

	ret = hi6403es_irq_init(priv->irqmgr);
	if (0 != ret) {
		dev_err(dev, "%s: hi64xx_irq_init fail. err code is %x .\n", __FUNCTION__, ret);
		goto free_platform_data;
	}

	/* irq request : bunk1_ocp */
	ret = hi64xx_irq_request_irq(priv->irqmgr, IRQ_BUNK1_OCP, hi6403es_bunk1_ocp_handler, "bunk1_ocp", priv);
	if (0 != ret) {
		pr_err("%s %d: hi64xx_irq_request_irq fail. err code is %x .\n", __FUNCTION__, __LINE__, ret);
		goto irq_bunk1_ocp_exit;
	}

	/* irq request : bunk1_scp */
	ret = hi64xx_irq_request_irq(priv->irqmgr, IRQ_BUNK1_SCP, hi6403es_bunk1_scp_handler, "bunk1_scp", priv);
	if (0 != ret) {
		pr_err("%s %d: hi64xx_irq_request_irq fail. err code is %x .\n", __FUNCTION__, __LINE__, ret);
		goto irq_bunk1_scp_exit;
	}

	/* irq request : bunk2_ocp */
	ret = hi64xx_irq_request_irq(priv->irqmgr, IRQ_BUNK2_OCP, hi6403es_bunk2_ocp_handler, "bunk2_ocp", priv);
	if (0 != ret) {
		pr_err("%s %d: hi64xx_irq_request_irq fail. err code is %x .\n", __FUNCTION__, __LINE__, ret);
		goto irq_bunk2_ocp_exit;
	}

	/* irq request : bunk2_scp */
	ret = hi64xx_irq_request_irq(priv->irqmgr, IRQ_BUNK2_SCP, hi6403es_bunk2_scp_handler, "bunk2_scp", priv);
	if (0 != ret) {
		pr_err("%s %d: hi64xx_irq_request_irq fail. err code is %x .\n", __FUNCTION__, __LINE__, ret);
		goto irq_bunk2_scp_exit;
	}

	/* irq request : cp1_short */
	ret = hi64xx_irq_request_irq(priv->irqmgr, IRQ_CP1_SHORT, hi6403es_cp1_short_handler, "cp1_short", priv);
	if (0 != ret) {
		pr_err("%s %d: hi64xx_irq_request_irq fail. err code is %x .\n", __FUNCTION__, __LINE__, ret);
		goto irq_cp1_short_exit;
	}

	/* irq request : cp2_short */
	ret = hi64xx_irq_request_irq(priv->irqmgr, IRQ_CP2_SHORT, hi6403es_cp2_short_handler, "cp2_short", priv);
	if (0 != ret) {
		pr_err("%s %d: hi64xx_irq_request_irq fail. err code is %x .\n", __FUNCTION__, __LINE__, ret);
		goto irq_cp2_short_exit;
	}

	/* irq request : pll_unlock */
	ret = hi64xx_irq_request_irq(priv->irqmgr, IRQ_PLL_UNLOCK, hi6403es_pll_unlock_handler, "pll_unlock", priv);
	if (0 != ret) {
		pr_err("%s %d: hi64xx_irq_request_irq fail. err code is %x .\n", __FUNCTION__, __LINE__, ret);
		goto irq_pll_unlock_exit;
	}
	hi64xx_irq_disable_irq(priv->irqmgr, IRQ_PLL_UNLOCK);

	/* irq request : pll44k1_unlock */
	ret = hi64xx_irq_request_irq(priv->irqmgr, IRQ_PLL44K1_UNLOCK, hi6403es_pll44k1_unlock_handler, "pll44k1_unlock", priv);
	if (0 != ret) {
		pr_err("%s %d: hi64xx_irq_request_irq fail. err code is %x .\n", __FUNCTION__, __LINE__, ret);
		goto irq_pll44k1_unlock_exit;
	}
	hi64xx_irq_disable_irq(priv->irqmgr, IRQ_PLL44K1_UNLOCK);

	/* irq request : pllmad_unlock */
	ret = hi64xx_irq_request_irq(priv->irqmgr, IRQ_PLLMAD_UNLOCK, hi6403es_pllmad_unlock_handler, "pllmad_unlock", priv);
	if (0 != ret) {
		pr_err("%s %d: hi64xx_irq_request_irq fail. err code is %x .\n", __FUNCTION__, __LINE__, ret);
		goto irq_pllmad_unlock_exit;
	}
	hi64xx_irq_disable_irq(priv->irqmgr, IRQ_PLLMAD_UNLOCK);

	ret = hi64xx_compat_init(priv->cdc_ctrl, priv->irqmgr);
	if (0 != ret) {
		dev_err(dev, "%s: hi64xx_compat_init fail. err code is %x .\n", __FUNCTION__, ret);
		goto compat_init_err_exit;
	}

	ret = snd_soc_register_codec(dev, &hi6403es_codec_driver,
			hi6403es_dai, ARRAY_SIZE(hi6403es_dai));
	if (0 != ret) {
		dev_err(dev, "%s : snd_soc_register_codec create failed! err code 0x%x . \n", __FUNCTION__, ret);
		goto codec_register_err_exit;
	}

	return ret;

codec_register_err_exit:

compat_init_err_exit:
	hi6403es_compat_deinit();
irq_pllmad_unlock_exit:
	hi64xx_irq_free_irq(priv->irqmgr, IRQ_PLL44K1_UNLOCK, priv);
irq_pll44k1_unlock_exit:
	hi64xx_irq_free_irq(priv->irqmgr, IRQ_PLL_UNLOCK, priv);
irq_pll_unlock_exit:
	hi64xx_irq_free_irq(priv->irqmgr, IRQ_CP2_SHORT, priv);
irq_cp2_short_exit:
	hi64xx_irq_free_irq(priv->irqmgr, IRQ_CP1_SHORT, priv);
irq_cp1_short_exit:
	hi64xx_irq_free_irq(priv->irqmgr, IRQ_BUNK2_SCP, priv);
irq_bunk2_scp_exit:
	hi64xx_irq_free_irq(priv->irqmgr, IRQ_BUNK2_OCP, priv);
irq_bunk2_ocp_exit:
	hi64xx_irq_free_irq(priv->irqmgr, IRQ_BUNK1_SCP, priv);
irq_bunk1_scp_exit:
	hi64xx_irq_free_irq(priv->irqmgr, IRQ_BUNK1_OCP, priv);
irq_bunk1_ocp_exit:

free_platform_data:
	if (priv) {
		devm_kfree(dev, priv);
	}
	dev_err(dev, "%s: init failed\n", __FUNCTION__);

	return ret;
}
/*lint +e429*/
static int hi6403es_platform_remove(struct platform_device *pdev)
{
	struct hi6403es_platform_data *priv = platform_get_drvdata(pdev);

	pr_info("%s\n",__FUNCTION__);

	BUG_ON(NULL == priv);

	snd_soc_unregister_codec(&pdev->dev);

	if (!priv) {
		devm_kfree(&pdev->dev, priv);
	}
	return 0;
}

static void hi6403es_platform_shutdown(struct platform_device *pdev)
{
	struct hi6403es_platform_data *priv = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = NULL;

	BUG_ON(NULL == priv);
	codec = priv->codec;

	if (NULL != codec)
		hi6403es_headphone_off(codec);
}

static struct platform_driver hi6403es_platform_driver = {
	.probe	= hi6403es_platform_probe,
	.remove	= hi6403es_platform_remove,
	.shutdown = hi6403es_platform_shutdown,
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= "hi6403es-codec",
		.of_match_table = of_match_ptr(hi6403es_platform_match),
	},
};

static int __init hi6403es_platform_init(void)
{
	return platform_driver_register(&hi6403es_platform_driver);
}

static void __exit hi6403es_platform_exit(void)
{
	platform_driver_unregister(&hi6403es_platform_driver);
}

fs_initcall_sync(hi6403es_platform_init);
module_exit(hi6403es_platform_exit);

MODULE_DESCRIPTION("ASoC hi6403es codec driver");
MODULE_AUTHOR("liuyang <liuyang66@hisilicon.com>");
MODULE_LICENSE("GPL");

