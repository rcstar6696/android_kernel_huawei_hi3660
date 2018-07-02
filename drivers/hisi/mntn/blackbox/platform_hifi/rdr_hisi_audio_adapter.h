/*
 * audio rdr adpter.
 *
 * Copyright (c) 2015 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __RDR_HISI_AUDIO_ADAPTER_H__
#define __RDR_HISI_AUDIO_ADAPTER_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/hisi/rdr_pub.h>

#define RDR_FNAME_LEN					128UL
/* rdr modid for hifi from 0x84000000(HISI_BB_MOD_HIFI_START) to 0x84ffffff(HISI_BB_MOD_HIFI_END) */
#define RDR_AUDIO_MODID_START          HISI_BB_MOD_HIFI_START

#define RDR_AUDIO_SOC_MODID_START      RDR_AUDIO_MODID_START
#define RDR_AUDIO_SOC_WD_TIMEOUT_MODID 0x84000001U
#define RDR_AUDIO_SOC_MODID_END        0x8400000fU

#define RDR_AUDIO_CODEC_MODID_START      0x84000010U
#define RDR_AUDIO_CODEC_WD_TIMEOUT_MODID 0x84000011U
#define RDR_AUDIO_CODEC_MODID_END        0x8400001FU

#define RDR_AUDIO_REBOOT_MODID_START 0x84000020U
#define RDR_AUDIO_NOC_MODID          0x84000021U
#define RDR_AUDIO_REBOOT_MODID_END   0x8400002FU

#define RDR_AUDIO_MODID_END          HISI_BB_MOD_HIFI_END

int rdr_audio_write_file(char *name, char *data, u32 size);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif

