/* Copyright (c) 2013-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/
#ifndef __HI6403ES_DSP_REGS_H__
#define __HI6403ES_DSP_REGS_H__

#include "hi64xx_dsp_regs.h"

/* |~0x10000000~~|~~0x10010000~~~|~0x10010080~|~~0x100100c0~~~|~0x10010140~|~0x10011140~~|~~0x10011340~~| */
/* |~ring buffer~|~ap to dsp msg~|~ap dsp cmd~|~dsp to ap msg~|~mlib para~~|~panic stack~|~dump cpuview~| */
/* |~~~~~64k~~~~~|~~~128 byte~~~~|~~~64 byte~~|~~~128 byte~~~~|~~~~~4k~~~~~|~~~512 byte~~|~~~512 byte~~~| */
#define HI6403ES_OCRAM_BASE_ADDR             (0x10000000)
#define HI6403ES_ITCM_BASE_ADDR              (0x08000000)
#define HI6403ES_DTCM_BASE_ADDR              (0x08010000)

#define HI6403ES_OCRAM_SIZE                  (0x58000)
#define HI6403ES_ITCM_SIZE                   (0x5000)
#define HI6403ES_DTCM_SIZE                   (0x20000)

#define HI6403ES_RINGBUFFER_SIZE             (0x10000)    /* sample rate:16K resolution:16bit 2s*/
#define HI6403ES_AP_TO_DSP_MSG_SIZE          (0x80)
#define HI6403ES_AP_DSP_CMD_SIZE             (0x40)       /* CMD size */
#define HI6403ES_DSP_TO_AP_MSG_SIZE          (0x80)
#define HI6403ES_MLIB_PARA_MAX_SIZE          (0x1000)
#define HI6403ES_DUMP_PANIC_STACK_SIZE       (0x200)      /* store panic static info */
#define HI6403ES_DUMP_CPUVIEW_SIZE           (0x200)      /* store cpuview info */
#define HI6403ES_SAVE_LOG_SIZE               (0x800)
#define HI6403ES_SAVE_LOG_SIZE_ES            (0x4000)

#define HI6403ES_RINGBUFFER_ADDR             (HI6403ES_OCRAM_BASE_ADDR)
#define HI6403ES_AP_TO_DSP_MSG_ADDR          (HI6403ES_RINGBUFFER_ADDR + HI6403ES_RINGBUFFER_SIZE)
#define HI6403ES_AP_DSP_CMD_ADDR             (HI6403ES_AP_TO_DSP_MSG_ADDR + HI6403ES_AP_TO_DSP_MSG_SIZE)
#define HI6403ES_DSP_TO_AP_MSG_ADDR          (HI6403ES_AP_DSP_CMD_ADDR + HI6403ES_AP_DSP_CMD_SIZE)
#define HI6403ES_MLIBPARA_ADDR               (HI6403ES_DSP_TO_AP_MSG_ADDR + HI6403ES_DSP_TO_AP_MSG_SIZE)
#define HI6403ES_DUMP_PANIC_STACK_ADDR       (HI6403ES_MLIBPARA_ADDR + HI6403ES_MLIB_PARA_MAX_SIZE)
#define HI6403ES_DUMP_CPUVIEW_ADDR           (HI6403ES_DUMP_PANIC_STACK_ADDR + HI6403ES_DUMP_PANIC_STACK_SIZE)
#define HI6403ES_SAVE_LOG_ADDR               (0x0802f800)
#define HI6403ES_SAVE_LOG_ADDR_ES            (0x10058000)

#define HI6403ES_CMD0_ADDR                   (HI6403ES_AP_DSP_CMD_ADDR + 0x004 * 0)
#define HI6403ES_CMD1_ADDR                   (HI6403ES_AP_DSP_CMD_ADDR + 0x004 * 1)
#define HI6403ES_CMD2_ADDR                   (HI6403ES_AP_DSP_CMD_ADDR + 0x004 * 2)
#define HI6403ES_CMD3_ADDR                   (HI6403ES_AP_DSP_CMD_ADDR + 0x004 * 3)

/* regs */
#define HI6403ES_DSP_I2S_DSPIF_CLK_EN        (HI64xx_DSP_SCTRL_BASE + 0x0F0)
#define HI6403ES_DSP_CMD_STAT_VLD            (HI64xx_DSP_SCTRL_BASE + 0xc)

#define HI6403ES_CFG_REG_CLK_CTRL            (BASE_ADDR_PAGE_CFG + 0x038)
#define HI6403ES_DSP_LP_CTRL                 (BASE_ADDR_PAGE_CFG + 0x050)
#define HI6403ES_DSP_CTRL6_DMAC              (BASE_ADDR_PAGE_CFG + 0x054)
#define HI6403ES_DSP_LP_CTRL1                (BASE_ADDR_PAGE_CFG + 0x082)
#define HI6403ES_EC_REG_DSP1                 (BASE_ADDR_PAGE_CFG + 0x09D)

#define HI6403ES_DSP_S1_CTRL_L               (HI64xx_AUDIO_SUB_BASE + 0xAE)
#define HI6403ES_DSP_S1_CTRL_H               (HI64xx_AUDIO_SUB_BASE + 0xAF)

#define HI6403ES_DSP_S3_CTRL_L               (HI64xx_AUDIO_SUB_BASE + 0xB2)
#define HI6403ES_DSP_S3_CTRL_H               (HI64xx_AUDIO_SUB_BASE + 0xB3)

#define HI6403ES_SLIM_CTRL_3                 (HI64xx_AUDIO_SUB_BASE + 0xC6)
#define HI6403ES_SLIM_CTRL_5                 (HI64xx_AUDIO_SUB_BASE + 0xC8)

#define HI6403ES_SLIM_UP_EN1                 (HI64xx_AUDIO_SUB_BASE + 0x1FE)
#define HI6403ES_SLIM_UP_EN2                 (HI64xx_AUDIO_SUB_BASE + 0x1FF)

#define HI6403ES_SC_CODEC_MUX_SEL3_1		 (HI64xx_AUDIO_SUB_BASE + 0x15)
#define HI6403ES_SC_S1_SRC_LR_CTRL_M		 (HI64xx_AUDIO_SUB_BASE + 0x37)
#define HI6403ES_SC_S2_SRC_LR_CTRL_M		 (HI64xx_AUDIO_SUB_BASE + 0x49)
#define HI6403ES_SC_S3_SRC_LR_CTRL_M		 (HI64xx_AUDIO_SUB_BASE + 0x58)
#define HI6403ES_SC_S4_SRC_LR_CTRL_M		 (HI64xx_AUDIO_SUB_BASE + 0x67)

#endif
