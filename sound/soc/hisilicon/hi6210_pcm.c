/******************************************************************************

                  版权所有 (C), 2001-2011, 华为技术有限公司

 ******************************************************************************
  文 件 名   : hi6210_pcm.c
  版 本 号   : 初稿
  作    者   : 石旺来 s00212991
  生成日期   : 2012年7月31日
  最近修改   :
  功能描述   : xxxx
  函数列表   :
              hi6210_exit
              hi6210_init
              hi6210_pcm_close
              hi6210_pcm_free
              hi6210_pcm_hw_free
              hi6210_pcm_hw_params
              hi6210_pcm_new
              hi6210_pcm_open
              hi6210_pcm_pointer
              hi6210_pcm_prepare
              hi6210_pcm_trigger
              hi6210_platform_probe
              hi6210_platform_remove
              status_read_proc_hstatus
              status_write_proc_hstatus
  修改历史   :
  1.日    期   : 2012年7月31日
    作    者   : 石旺来 s00212991
    修改内容   : 创建文件

******************************************************************************/

/*
the 2 MACRO should be used seperately
CONFIG_SND_TEST_AUDIO_PCM_LOOP : for ST, simu data send of mailbox
__DRV_AUDIO_MAILBOX_WORK__   : leave mailbox's work to workqueue
*/

#define __DRV_AUDIO_MAILBOX_WORK__


/*****************************************************************************
  1 头文件包含
 *****************************************************************************/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/sched/rt.h>


#include <linux/hisi/hilog.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#ifdef CONFIG_HIFI_MAILBOX
#include "drv_mailbox_cfg.h"
//#include "../../../drivers/hisi/hifi_mailbox/mailbox/drv_mailbox_cfg.h"
#endif

#ifdef CONFIG_HI6XXX_MAILBOX_MULTICORE
#include <../../../drivers/hisi/mailbox/hi6xxx_mailbox/drv_mailbox.h>
#endif

#include "hifi_lpp.h"
#include "hi6210_pcm.h"
#include "hi6210_log.h"

/*lint -e750 -e785 -e838 -e749 -e747 -e611 -e570 -e647 -e574*/

/*****************************************************************************
  2 macro define
 *****************************************************************************/

#define UNUSED_PARAMETER(x) (void)(x)

#define HI6210_PCM "hi6210-hifi"
/*
 * PLAYBACK SUPPORT FORMATS
 * BITS : 8/16/24  18/20
 * LITTLE_ENDIAN / BIG_ENDIAN
 * MONO / STEREO
 * UNSIGNED / SIGNED
 */
#define HI6210_PB_FORMATS  (SNDRV_PCM_FMTBIT_S8 | \
		SNDRV_PCM_FMTBIT_U8 | \
		SNDRV_PCM_FMTBIT_S16_LE | \
		SNDRV_PCM_FMTBIT_S16_BE | \
		SNDRV_PCM_FMTBIT_U16_LE | \
		SNDRV_PCM_FMTBIT_U16_BE | \
		SNDRV_PCM_FMTBIT_S24_LE | \
		SNDRV_PCM_FMTBIT_S24_BE | \
		SNDRV_PCM_FMTBIT_U24_LE | \
		SNDRV_PCM_FMTBIT_U24_BE)

/*
 * PLAYBACK SUPPORT RATES
 * 8/11.025/16/22.05/32/44.1/48/88.2/96kHz
 */
#define HI6210_PB_RATES    (SNDRV_PCM_RATE_8000_48000 | \
		SNDRV_PCM_RATE_44100 | \
		SNDRV_PCM_RATE_88200 | \
		SNDRV_PCM_RATE_96000 | \
		SNDRV_PCM_RATE_192000 | \
		SNDRV_PCM_RATE_384000)

#define HI6210_PB_MIN_CHANNELS  ( 1 )
#define HI6210_PB_MAX_CHANNELS  ( 2 )
/* Assume the FIFO size */
#define HI6210_PB_FIFO_SIZE     ( 16 )

/* CAPTURE SUPPORT FORMATS : SIGNED 16/24bit */
#define HI6210_CP_FORMATS  ( SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE)

/* CAPTURE SUPPORT RATES : 48/96kHz */
#define HI6210_CP_RATES    ( SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000 )

#define HI6210_CP_MIN_CHANNELS  ( 1 )
#define HI6210_CP_MAX_CHANNELS  ( 6 )
/* Assume the FIFO size */
#define HI6210_CP_FIFO_SIZE     ( 32 )
#define HI6210_MODEM_RATES      ( SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_48000)
#define HI6210_BT_RATES         ( SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 )
#define HI6210_FM_RATES         ( SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 )

#define HI6210_MAX_BUFFER_SIZE  ( 192 * 1024 )    /* 0x30000 */
#define HI6210_BUFFER_SIZE_MM   ( 32 * 1024 )
#define HI6210_MIN_BUFFER_SIZE  ( 32 )
#define HI6210_MAX_PERIODS      ( 32 )
#define HI6210_MIN_PERIODS      ( 2 )
#define HI6210_WORK_DELAY_1MS   ( 33 )    /* 33 equals 1ms */
#define HI6210_CYC_SUB(Cur, Pre, CycLen)                    \
	(((Cur) < (Pre)) ? (((CycLen) - (Pre)) + (Cur)) : ((Cur) - (Pre)))

#ifndef OK
#define OK              0
#endif
#ifndef ERROR
#define ERROR           -1
#endif

#undef NULL
#define NULL ((void *)0)

#define ALSA_TIMEOUT_MILLISEC 30

PCM_DMA_BUF_CONFIG  g_pcm_dma_buf_config[PCM_DEVICE_MAX][PCM_STREAM_MAX] =
{
	{{PCM_DMA_BUF_0_PLAYBACK_BASE,PCM_DMA_BUF_0_PLAYBACK_LEN},{PCM_DMA_BUF_0_CAPTURE_BASE,PCM_DMA_BUF_0_CAPTURE_LEN}},/*normal*/
	{{0,0},{0,0}},/*modem*/
	{{0,0},{0,0}},/*fm*/
	{{0,0},{0,0}},/*bt*/
	{{0,0},{0,0}},/*offload*/
	{{PCM_DMA_BUF_1_PLAYBACK_BASE,PCM_DMA_BUF_1_PLAYBACK_LEN},{PCM_DMA_BUF_1_CAPTURE_BASE,PCM_DMA_BUF_1_CAPTURE_LEN}},/*direct*/
	{{PCM_DMA_BUF_2_PLAYBACK_BASE,PCM_DMA_BUF_2_PLAYBACK_LEN},{PCM_DMA_BUF_2_CAPTURE_BASE,PCM_DMA_BUF_2_CAPTURE_LEN}} /*lowlatency*/
};

/*****************************************************************************

 *****************************************************************************/
static const unsigned int freq[] = {
	8000,   11025,  12000,  16000,
	22050,  24000,  32000,  44100,
	48000,  88200,  96000,  176400,
	192000, 384000,
};

static const struct snd_soc_component_driver hi6210_pcm_component = {
	.name   = HI6210_PCM,
};

static u64 hi6210_pcm_dmamask           = (u64)(0xffffffff);

/* according to definition in pcm driver  */
enum pcm_device {
	PCM_DEVICE_NORMAL = 0,
	PCM_DEVICE_MODEM,
	PCM_DEVICE_FM,
	PCM_DEVICE_BT,
	PCM_DEVICE_OFFLOAD,
	PCM_DEVICE_DIRECT,
	PCM_DEVICE_FAST,
	PCM_DEVICE_TOTAL
};

static struct snd_soc_dai_driver hi6210_dai[] =
{
	{
		.name = "hi6210-mm",
		.playback = {
			.stream_name  = "hi6210-mm Playback",
			.channels_min = HI6210_PB_MIN_CHANNELS,
			.channels_max = HI6210_PB_MAX_CHANNELS,
			.rates        = HI6210_PB_RATES,
			.formats      = HI6210_PB_FORMATS
		},
		.capture = {
			.stream_name  = "hi6210-mm Capture",
			.channels_min = HI6210_CP_MIN_CHANNELS,
			.channels_max = HI6210_CP_MAX_CHANNELS,
			.rates        = HI6210_CP_RATES,
			.formats      = HI6210_CP_FORMATS
		},
	},
	{
		.name = "hi6210-modem",
		.playback = {
			.stream_name  = "hi6210-modem Playback",
			.channels_min = HI6210_PB_MIN_CHANNELS,
			.channels_max = HI6210_PB_MAX_CHANNELS,
			.rates        = HI6210_MODEM_RATES,
			.formats      = HI6210_PB_FORMATS
		},
	},
	{
		.name = "hi6210-fm",
		.playback = {
			.stream_name  = "hi6210-fm Playback",
			.channels_min = HI6210_PB_MIN_CHANNELS,
			.channels_max = HI6210_PB_MAX_CHANNELS,
			.rates        = HI6210_FM_RATES,
			.formats      = HI6210_PB_FORMATS
		},
	},
	{
		.name = "hi6210-bt",
		.playback = {
			.stream_name  = "hi6210-bt Playback",
			.channels_min = HI6210_PB_MIN_CHANNELS,
			.channels_max = HI6210_PB_MAX_CHANNELS,
			.rates        = HI6210_BT_RATES,
			.formats      = HI6210_PB_FORMATS
		},
	},
	{
		.name = "hi6210-lpp",
		.playback = {
			.stream_name  = "hi6210-lpp Playback",
			.channels_min = HI6210_PB_MIN_CHANNELS,
			.channels_max = HI6210_PB_MAX_CHANNELS,
			.rates        = HI6210_PB_RATES,
			.formats      = HI6210_PB_FORMATS
		},
	},
	{
		.name = "hi6210-direct",
		.playback = {
			.stream_name  = "hi6210-direct Playback",
			.channels_min = HI6210_PB_MIN_CHANNELS,
			.channels_max = HI6210_PB_MAX_CHANNELS,
			.rates        = HI6210_PB_RATES,
			.formats      = HI6210_PB_FORMATS
		},
	},
	{
		.name = "hi6210-fast",
		.playback = {
			.stream_name  = "hi6210-fast Playback",
			.channels_min = HI6210_PB_MIN_CHANNELS,
			.channels_max = HI6210_PB_MAX_CHANNELS,
			.rates        = HI6210_PB_RATES,
			.formats      = HI6210_PB_FORMATS
		},
		.capture = {
			.stream_name  = "hi6210-fast Capture",
			.channels_min = HI6210_CP_MIN_CHANNELS,
			.channels_max = HI6210_CP_MAX_CHANNELS,
			.rates        = HI6210_CP_RATES,
			.formats      = HI6210_CP_FORMATS
		},
	},
};

/* define the capability of playback channel */
static const struct snd_pcm_hardware hi6210_hardware_playback =
{
	.info             = SNDRV_PCM_INFO_INTERLEAVED
		| SNDRV_PCM_INFO_NONINTERLEAVED
		| SNDRV_PCM_INFO_MMAP
		| SNDRV_PCM_INFO_MMAP_VALID
		| SNDRV_PCM_INFO_PAUSE,
	.formats          = SNDRV_PCM_FMTBIT_S16_LE,
	.channels_min     = HI6210_PB_MIN_CHANNELS,
	.channels_max     = HI6210_PB_MAX_CHANNELS,
	.buffer_bytes_max = PCM_DMA_BUF_PLAYBACK_PRIMARY_LEN,
	.period_bytes_min = HI6210_MIN_BUFFER_SIZE,
	.period_bytes_max = PCM_DMA_BUF_PLAYBACK_PRIMARY_LEN,
	.periods_min      = HI6210_MIN_PERIODS,
	.periods_max      = HI6210_MAX_PERIODS,
	.fifo_size        = HI6210_PB_FIFO_SIZE,
};

/* define the capability of capture channel */
static const struct snd_pcm_hardware hi6210_hardware_capture =
{
	.info             = SNDRV_PCM_INFO_INTERLEAVED,
	.formats          = SNDRV_PCM_FMTBIT_S16_LE,
	.rates            = SNDRV_PCM_RATE_48000,
	.channels_min     = HI6210_CP_MIN_CHANNELS,
	.channels_max     = HI6210_CP_MAX_CHANNELS,
	.buffer_bytes_max = HI6210_MAX_BUFFER_SIZE,
	.period_bytes_min = HI6210_MIN_BUFFER_SIZE,
	.period_bytes_max = HI6210_MAX_BUFFER_SIZE,
	.periods_min      = HI6210_MIN_PERIODS,
	.periods_max      = HI6210_MAX_PERIODS,
	.fifo_size        = HI6210_CP_FIFO_SIZE,
};

/* define the capability of playback channel for direct*/
static const struct snd_pcm_hardware hi6210_hardware_direct_playback =
{
	.info             = SNDRV_PCM_INFO_INTERLEAVED
		| SNDRV_PCM_INFO_NONINTERLEAVED
		| SNDRV_PCM_INFO_MMAP
		| SNDRV_PCM_INFO_MMAP_VALID
		| SNDRV_PCM_INFO_PAUSE,
	.formats          = SNDRV_PCM_FMTBIT_S24_LE,
	.channels_min     = HI6210_PB_MIN_CHANNELS,
	.channels_max     = HI6210_PB_MAX_CHANNELS,
	.buffer_bytes_max = PCM_DMA_BUF_PLAYBACK_DIRECT_LEN,
	.period_bytes_min = HI6210_MIN_BUFFER_SIZE,
	.period_bytes_max = PCM_DMA_BUF_PLAYBACK_DIRECT_LEN,
	.periods_min      = HI6210_MIN_PERIODS,
	.periods_max      = HI6210_MAX_PERIODS,
	.fifo_size        = HI6210_PB_FIFO_SIZE,
};

/* define the capability of playback channel for Modem */
static const struct snd_pcm_hardware hi6210_hardware_modem_playback =
{
	.info             = SNDRV_PCM_INFO_INTERLEAVED
		| SNDRV_PCM_INFO_NONINTERLEAVED
		| SNDRV_PCM_INFO_BLOCK_TRANSFER
		| SNDRV_PCM_INFO_PAUSE,
	.formats          = SNDRV_PCM_FMTBIT_S16_LE,
	.channels_min     = HI6210_PB_MIN_CHANNELS,
	.channels_max     = HI6210_PB_MAX_CHANNELS,
	.buffer_bytes_max = HI6210_MAX_BUFFER_SIZE,
	.period_bytes_min = HI6210_MIN_BUFFER_SIZE,
	.period_bytes_max = HI6210_MAX_BUFFER_SIZE,
	.periods_min      = HI6210_MIN_PERIODS,
	.periods_max      = HI6210_MAX_PERIODS,
	.fifo_size        = HI6210_PB_FIFO_SIZE,
};

struct hi6210_pcm_private_data
{
	u32 pcm_cp_status_open;
	u32 pcm_pb_status_open;
	u32 pcm_direct_pb_status_open;
	u32 pcm_fast_pb_status_open;
	struct hi6210_runtime_data pcm_rtd_playback;
	struct hi6210_runtime_data pcm_rtd_capture;
	struct hi6210_runtime_data pcm_rtd_direct_playback;
	struct hi6210_runtime_data pcm_rtd_fast_playback;
};

static struct hi6210_pcm_private_data pdata;

spinlock_t g_pcm_cp_open_lock;
spinlock_t g_pcm_pb_open_lock;

/*****************************************************************************
  3 function declare
 *****************************************************************************/
extern int mailbox_get_timestamp(void);
static int hi6210_pcm_notify_set_buf(struct snd_pcm_substream *substream);
static irq_rt_t hi6210_notify_recv_isr(void *usr_para, void *mail_handle, unsigned int mail_len);

#ifdef __DRV_AUDIO_MAILBOX_WORK__
static irq_rt_t hi6210_mb_intr_handle(unsigned short pcm_mode, struct snd_pcm_substream *substream);
#endif

/*****************************************************************************
  3 Functions
 *****************************************************************************/


/*****************************************************************************
	function name  : hi6210_intr_handle
	Description  : after data transfered
	inputs : struct snd_pcm_substream *substream
	output : no
	return  : STATIC irq_rt_t
	called function : hi6210_mb_intr_handle()
	history    :
	Date  :
	Author  :
 *****************************************************************************/
static irq_rt_t hi6210_intr_handle(struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct hi6210_runtime_data *prtd = NULL;
	snd_pcm_uframes_t rt_period_size = 0;
	unsigned int num_period = 0;
	snd_pcm_uframes_t avail = 0;
	unsigned short pcm_mode = 0;

	IN_FUNCTION;

	if (NULL == substream) {
		loge("[%s:%d] ISR substream is null, error\n", __FUNCTION__, __LINE__);
		return IRQ_HDD_PTR;
	}

	if (NULL == substream->runtime) {
		loge("[%s:%d] substream->runtime == NULL\n", __FUNCTION__, __LINE__);
		return IRQ_HDD_PTR;
	}

	pcm_mode = (unsigned short)substream->stream;
	prtd    = (struct hi6210_runtime_data *)substream->runtime->private_data;
	rt_period_size  = substream->runtime->period_size;
	num_period      = substream->runtime->periods;

	if (NULL == prtd) {
		loge("[%s:%d] prtd == NULL\n", __FUNCTION__, __LINE__);
		return IRQ_HDD_PTR;
	}

	if (STATUS_RUNNING != prtd->status) {
		logd("[%s:%d] dma status %d error\n" ,__FUNCTION__, __LINE__, prtd->status);
		return IRQ_HDD_STATUS;
	}

	if (SNDRV_PCM_STREAM_CAPTURE == pcm_mode) {
		avail = (snd_pcm_uframes_t)snd_pcm_capture_hw_avail(substream->runtime);
	} else {
		avail = (snd_pcm_uframes_t)snd_pcm_playback_hw_avail(substream->runtime);
	}
	if (avail < rt_period_size) {
		logw("avail(%d)< rt_period_size(%d)\n", (int)avail, (int)rt_period_size);
		return IRQ_HDD_SIZE;
	} else {
		++prtd->period_cur;
		prtd->period_cur = (prtd->period_cur) % num_period;

		snd_pcm_period_elapsed(substream);

		ret = hi6210_pcm_notify_set_buf(substream);
		if(ret < 0) {
			loge("hi6210_notify_pcm_set_buf(%d)\n", ret);
			return IRQ_HDD_ERROR;
		}

		prtd->period_next = (prtd->period_next + 1) % num_period;
	}

	OUT_FUNCTION;

	return IRQ_HDD;
}

/*****************************************************************************
	function name  : hi6210_mb_intr_handle
	Description  : data transfering over mailbox
	inputs : struct snd_pcm_substream *substream
	output : no
	return  : STATIC irq_rt_t
	called function : hi6210_notify_recv_isr()
	history    :
	Date  :
	Author  :
 *****************************************************************************/
static irq_rt_t hi6210_mb_intr_handle(unsigned short pcm_mode,
		struct snd_pcm_substream *substream)
{
	irq_rt_t ret = IRQ_NH;

	if (NULL == substream) {
		loge("substream is NULL\n");
		return IRQ_HDD_PTRS;
	}

	switch (pcm_mode)
	{
	case SNDRV_PCM_STREAM_PLAYBACK:
		/* lock used to protect close while doing _intr_handle_pb */
		spin_lock_bh(&g_pcm_pb_open_lock);
		if ((0 == pdata.pcm_pb_status_open) && (0 == pdata.pcm_direct_pb_status_open) && (0 == pdata.pcm_fast_pb_status_open)) {
			logd("pcm playback closed\n");
			spin_unlock_bh(&g_pcm_pb_open_lock);
			return IRQ_HDD;
		}

		ret = hi6210_intr_handle(substream);
		spin_unlock_bh(&g_pcm_pb_open_lock);
		break;
	case SNDRV_PCM_STREAM_CAPTURE:
		/* lock used to protect close while doing _intr_handle_cp */
		spin_lock_bh(&g_pcm_cp_open_lock);

		if (0 == pdata.pcm_cp_status_open) {
			logd("pcm capture closed\n");
			spin_unlock_bh(&g_pcm_cp_open_lock);;
			return IRQ_HDD;
		}
		ret = hi6210_intr_handle(substream);
		spin_unlock_bh(&g_pcm_cp_open_lock);
		break;
	default:
		ret = IRQ_NH_MODE;
		loge("PCM Mode error(%d)\n", pcm_mode);
		break;
	}

	return ret;
}

/*****************************************************************************
	function name  : hi6210_mailbox_send_data
	Description  : data transfering over mailbox
	inputs : void *pmsg_body :the transfering data
	output : no
	return  : STATIC int
	called function : hi6210_pcm_notify_()
	history    :
	Date  :
	Author  :
 *****************************************************************************/
static int hi6210_mailbox_send_data(void *pmsg_body, unsigned int msg_len,
		unsigned int msg_priority)
{
	unsigned int ret = 0;
	static unsigned int err_count = 0;

	ret = DRV_MAILBOX_SENDMAIL(MAILBOX_MAILCODE_ACPU_TO_HIFI_AUDIO, pmsg_body, msg_len);
	if (MAILBOX_OK != ret) {
		if (err_count % 50 == 0)
			HiLOGE("audio", "Hi6210_pcm","mailbox ap to hifi fail,ret=%d, maybe ap is abnormal\n", ret);
		err_count++;
	} else {
		err_count = 0;
	}

	return (int)ret;
}

/*****************************************************************************
	function name  : hi6210_pcm_notify_set_buf
	Description  : notify the data buffer is ready to tranfer
	inputs : hifi_channel_set_buffer *psrc_data
	output : no
	return  : STATIC int
	called function : hi6210_intr_handle()
	history    :
	Date  :
	Author  :

 *****************************************************************************/
static int hi6210_pcm_notify_set_buf(struct snd_pcm_substream *substream)
{
	int ret = 0;
	unsigned int period_size;
	struct hifi_channel_set_buffer msg_body = {0};
	unsigned short pcm_mode = (unsigned short)substream->stream;
	struct hi6210_runtime_data *prtd = (struct hi6210_runtime_data *)substream->runtime->private_data;

	IN_FUNCTION;

	if (NULL == prtd) {
		loge("[%s:%d] prtd is null, error\n", __FUNCTION__, __LINE__);
		return -EINVAL;
	}
	period_size = prtd->period_size;

	if ((SNDRV_PCM_STREAM_PLAYBACK != pcm_mode) && (SNDRV_PCM_STREAM_CAPTURE != pcm_mode)) {
		loge("[%s:%d] pcm mode %d invalid\n", __FUNCTION__, __LINE__, pcm_mode);
		return -EINVAL;
	}

	msg_body.msg_type   = (unsigned short)HI_CHN_MSG_PCM_SET_BUF;
	msg_body.pcm_mode   = pcm_mode;
	msg_body.pcm_device = (unsigned short)substream->pcm->device;
	msg_body.data_addr  = (substream->runtime->dma_addr + prtd->period_next * period_size) - PCM_DMA_BUF_0_PLAYBACK_BASE;
	msg_body.data_len   = period_size;

	if (STATUS_RUNNING != prtd->status) {
		logd("[%s:%d] pcm status %d error \n", __FUNCTION__, __LINE__, prtd->status);
		return -EINVAL;
	}

	/* mail-box send */
	/* trace_dot(APCM,"4",0); */
	ret = hi6210_mailbox_send_data( &msg_body, sizeof(struct hifi_channel_set_buffer), 0 );
	if (OK != ret)
		ret = -EBUSY;

	/* trace_dot(APCM,"5",0); */
	OUT_FUNCTION;

	return ret;
}

static void print_pcm_timeout(unsigned int pre_time, const char *print_type, unsigned int time_delay)
{
	unsigned int  delay_time;
	unsigned int  curr_time;

	if (hifi_misc_get_platform_type() != HIFI_DSP_PLATFORM_ASIC) {
		return;
	}

	curr_time = (unsigned int)mailbox_get_timestamp();
	delay_time = curr_time - pre_time;

	if (delay_time > (HI6210_WORK_DELAY_1MS * time_delay)) {
		logw("[%d]:%s, delaytime %u.\n", mailbox_get_timestamp(), print_type, delay_time);
	}
}

static long get_snd_current_millisec(void)
{
	struct timeval last_update;
	long curr_time;

	do_gettimeofday(&last_update);
	curr_time = last_update.tv_sec * 1000 + last_update.tv_usec / 1000;
	return curr_time;
}

void snd_pcm_print_timeout(struct snd_pcm_substream *substream, unsigned int timeout_type)
{
	long delay_time;
	long curr_time;
	const char *timeout_str[SND_TIMEOUT_TYPE_MAX] = {
		"pcm write interval timeout",
		"pcm write proc timeout",
		"pcm read interval timeout"
		"pcm read proc timeout"};

	if (hifi_misc_get_platform_type() != HIFI_DSP_PLATFORM_ASIC) {
		return;
	}

	if (substream == NULL) {
		loge("[%s:%d] substream is null\n",  __FUNCTION__, __LINE__);
		return;
	}

	if (timeout_type >= SND_TIMEOUT_TYPE_MAX) {
		return;
	}

	curr_time = get_snd_current_millisec();
	delay_time = curr_time - substream->runtime->pre_time;

	if (delay_time > ALSA_TIMEOUT_MILLISEC && (substream->runtime->pre_time != 0)) {
		logw("%s, delay time %ld ms.\n", timeout_str[timeout_type], delay_time);
	}

	if (timeout_type == SND_TIMEOUT_TYPE_WRITE_INTERVAL
		|| timeout_type == SND_TIMEOUT_TYPE_READ_INTERVAL) {
		substream->runtime->pre_time = curr_time;
    }
}
EXPORT_SYMBOL(snd_pcm_print_timeout);

void snd_pcm_reset_pre_time(struct snd_pcm_substream *substream)
{
	if (substream == NULL) {
		loge("[%s:%d] substream is null\n",  __FUNCTION__, __LINE__);
		return;
	}

	substream->runtime->pre_time = 0;
}
EXPORT_SYMBOL(snd_pcm_reset_pre_time);

/*****************************************************************************
	function name  : hi6210_notify_recv_isr
	Description  : recv data and process
	inputs : void *usr_para
	output : no
	return  : STATIC irq_rt_t
	called function : hi6210_pcm_new()
	history    :
	Date  :
	Author  :
 *****************************************************************************/
static irq_rt_t hi6210_notify_recv_isr(void *usr_para, void *mail_handle, unsigned int mail_len)
{
	struct snd_pcm_substream * substream    = NULL;
	struct hi6210_runtime_data *prtd        = NULL;
	struct hifi_chn_pcm_period_elapsed mail_buf;
	unsigned int mail_size          = mail_len;
	unsigned int ret_mail           = MAILBOX_OK;
	irq_rt_t ret                    = IRQ_NH;
	unsigned int start_time = 0;
	const char *print_type[2] = {"recv pcm msg timeout", "process pcm msg timeout"};

	UNUSED_PARAMETER(usr_para);

	start_time = (unsigned int)mailbox_get_timestamp();
	memset(&mail_buf, 0, sizeof(struct hifi_chn_pcm_period_elapsed));/* unsafe_function_ignore: memset */

	/*get the data from mailbox*/

	ret_mail = DRV_MAILBOX_READMAILDATA(mail_handle, (unsigned char*)&mail_buf, &mail_size);
	if ((ret_mail != MAILBOX_OK)
		|| (mail_size == 0)
			|| (mail_size > sizeof(struct hifi_chn_pcm_period_elapsed)))
	{
		loge("Empty point or data length error! size: %d  ret_mail:%d sizeof(struct hifi_chn_pcm_period_elapsed):%lu\n", mail_size, ret_mail, sizeof(struct hifi_chn_pcm_period_elapsed));
		HiLOGE("audio", "Hi6210_pcm", "Empty point or data length error! size: %d\n", mail_size);
		return IRQ_NH_MB;
	}

#ifdef __DRV_AUDIO_MAILBOX_WORK__
	substream = INT_TO_ADDR(mail_buf.substream_l32,mail_buf.substream_h32);
	if (NULL == substream) {
		loge("substream from hifi is NULL\n");
		return IRQ_NH_OTHERS;
	}
	if (NULL == substream->runtime) {
		loge("substream runtime is NULL\n");
		return IRQ_NH_OTHERS;
	}

	prtd = (struct hi6210_runtime_data *)substream->runtime->private_data;
	if (NULL == prtd) {
		loge("prtd is NULL\n");
		return IRQ_NH_OTHERS;
	}
	if (STATUS_STOP == prtd->status) {
		logi("process has stopped\n");
		return IRQ_NH_OTHERS;
	}

	switch(mail_buf.msg_type) {
		case HI_CHN_MSG_PCM_PERIOD_ELAPSED:
			/* check if elapsed msg is timeout */
			print_pcm_timeout(mail_buf.msg_timestamp, print_type[0], 10);
			ret = hi6210_mb_intr_handle(mail_buf.pcm_mode, substream);
			if (ret == IRQ_NH)
				loge("mb msg handle err, ret : %d\n", ret);
			break;
		case HI_CHN_MSG_PCM_PERIOD_STOP:
			if (STATUS_STOPPING == prtd->status) {
				prtd->status = STATUS_STOP;
				logi("device %d mode %d stop now !\n", substream->pcm->device, mail_buf.pcm_mode);
			}
			break;
		default:
			loge("msg_type 0x%x\n", mail_buf.msg_type);
			break;
	}
	/* check if isr proc is timeout */
	print_pcm_timeout(start_time, print_type[1], 20);

	return ret;
#else
	substream = INT_TO_ADDR(mail_buf.substream_l32,mail_buf.substream_h32);
	switch(mail_buf.msg_type)
	{
	case HI_CHN_MSG_PCM_PERIOD_ELAPSED:
		ret = hi6210_mb_intr_handle(mail_buf.pcm_mode, substream);
		if (ret == IRQ_NH)
			loge("ret : %d\n", ret);
		break;
	default:
		ret = IRQ_NH_TYPE;
		loge("msg_type 0x%x\n", mail_buf.msg_type);
		break;
	}

	return ret;
#endif
}

/*****************************************************************************
	function name  : hi6210_notify_isr_register
	Description  : register the callback fun to rec data
	inputs : void *pisr
	output : no
	return  : STATIC int
	called function : hi6210_pcm_new()
	history    :
	Date  :
	Author  :
 *****************************************************************************/
static int hi6210_notify_isr_register(irq_hdl_t pisr)
{
	int ret                     = 0;
	unsigned int mailbox_ret    = MAILBOX_OK;

	if (NULL == pisr) {
		loge("pisr==NULL!\n");
		ret = ERROR;
	} else {
		mailbox_ret = DRV_MAILBOX_REGISTERRECVFUNC(MAILBOX_MAILCODE_HIFI_TO_ACPU_AUDIO, (void *)pisr, NULL);
		if (MAILBOX_OK != mailbox_ret) {
			ret = ERROR;
			loge("ret : %d,0x%x\n", ret, MAILBOX_MAILCODE_HIFI_TO_ACPU_AUDIO);
		}
	}

	return ret;
}


/*****************************************************************************
	function name  : hi6210_pcm_notify_hw_params
	Description  : set hw params
	inputs : struct snd_pcm_substream *substream,params
	output : no
	return  : STATIC int
	called function :
	history    :
	Date  :
	Author  :

 *****************************************************************************/
static int hi6210_pcm_notify_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	int ret = 0;
	struct hifi_chn_pcm_open open_msg = {0};
	struct hifi_chn_pcm_hw_params hw_params_msg  = {0};
	unsigned int params_value = 0;
	unsigned int infreq_index = 0;

	IN_FUNCTION;

	open_msg.msg_type = (unsigned short)HI_CHN_MSG_PCM_OPEN;
	hw_params_msg.msg_type = (unsigned short)HI_CHN_MSG_PCM_HW_PARAMS;
	hw_params_msg.pcm_mode = open_msg.pcm_mode = (unsigned short)substream->stream;
	hw_params_msg.pcm_device = open_msg.pcm_device = (unsigned short)substream->pcm->device;

	/* check channels  : mono or stereo */
	params_value = params_channels(params);
	if ((HI6210_CP_MIN_CHANNELS <= params_value) && (HI6210_CP_MAX_CHANNELS >= params_value)) {
		open_msg.config.channels = params_value;
		hw_params_msg.channel_num = params_value;
	} else {
		loge("DAC not support %d channels\n", params_value);
		return -EINVAL;
	}

	/* check samplerate */
	params_value = params_rate(params);
	logi("rate is %d\n", params_value);

	for (infreq_index = 0; infreq_index < ARRAY_SIZE(freq); infreq_index++) {
		if(params_value == freq[infreq_index])
			break;
	}

	if (ARRAY_SIZE(freq) <= infreq_index) {
		loge("[%s:%d] rate %d not support\n", __FUNCTION__, __LINE__, params_value);
		return -EINVAL;
	}

	open_msg.config.rate = params_value;

	hw_params_msg.sample_rate = params_value;

	/* check format */
	params_value = (unsigned int)params_format(params);
	if (params_value == SNDRV_PCM_FORMAT_S24_LE) {
		params_value = PCM_FORMAT_S24_LE_LA;
	} else {
		params_value = PCM_FORMAT_S16_LE;
	}

	hw_params_msg.format = params_value;

	open_msg.config.format = params_value;
	open_msg.config.period_size = params_period_size(params);
	open_msg.config.period_count = params_periods(params);

	ret = hi6210_mailbox_send_data(&open_msg, sizeof(struct hifi_chn_pcm_open), 0);

	/* send hw_params*/
	ret += hi6210_mailbox_send_data(&hw_params_msg, sizeof(struct hifi_chn_pcm_hw_params), 0);

	OUT_FUNCTION;

	return ret;
}

static int hi6210_pcm_notify_hw_free(struct snd_pcm_substream *substream)
{
	int ret = 0;

	UNUSED_PARAMETER(substream);

	return ret;
}

/*****************************************************************************
	function name  : hi6210_pcm_notify_prepare
	Description  : private
	inputs : struct snd_pcm_substream *substream
	output : no
	return  : STATIC int
	called function :
	history    :
	Date  :

 *****************************************************************************/
static int hi6210_pcm_notify_prepare(struct snd_pcm_substream *substream)
{
	int ret = OK;

	UNUSED_PARAMETER(substream);

	return ret;
}

/*****************************************************************************
	function name  : hi6210_pcm_notify_trigger
	Description  : stop or start stream
	inputs : struct snd_pcm_substream *substream
	output : no
	return  : STATIC int
	called function :
	history    :
	Date  :

 *****************************************************************************/
static int hi6210_pcm_notify_trigger(int cmd, struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct hifi_chn_pcm_trigger msg_body = {0};
	unsigned int period_size = 0;
	struct hi6210_runtime_data *prtd = (struct hi6210_runtime_data *)substream->runtime->private_data;

	IN_FUNCTION;

	if (NULL == prtd) {
		loge("[%s:%d] prtd is null\n",  __FUNCTION__, __LINE__);
		return -EINVAL;
	}

	period_size = prtd->period_size;

	msg_body.msg_type   	= (unsigned short)HI_CHN_MSG_PCM_TRIGGER;
	msg_body.pcm_mode   	= (unsigned short)substream->stream;
	msg_body.pcm_device   	= (unsigned short)substream->pcm->device;
	msg_body.tg_cmd     	= (unsigned short)cmd;
	msg_body.substream_l32  = GET_LOW32(substream);
	msg_body.substream_h32  = GET_HIG32(substream);

	if ((SNDRV_PCM_TRIGGER_START == cmd)
		|| (SNDRV_PCM_TRIGGER_RESUME == cmd)
		|| (SNDRV_PCM_TRIGGER_PAUSE_RELEASE == cmd)) {
		msg_body.data_addr = (substream->runtime->dma_addr + prtd->period_next * period_size) - PCM_DMA_BUF_0_PLAYBACK_BASE;
		msg_body.data_len  = period_size;
	}

	ret = hi6210_mailbox_send_data(&msg_body, sizeof(struct hifi_chn_pcm_trigger), 0);

	OUT_FUNCTION;

	return ret;
}


/*****************************************************************************
	function name  : hi6210_pcm_notify_open
	Description  : open and init stream
	inputs : struct snd_pcm_substream *substream
	output : no
	return  : STATIC int
	called function :
	history    :
	Date  :

 *****************************************************************************/
static int hi6210_pcm_notify_open(struct snd_pcm_substream *substream)
{
	int ret = 0;

	UNUSED_PARAMETER(substream);

	return ret;
}

static int hi6210_pcm_notify_close(struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct hifi_chn_pcm_close msg_body  = {0};

	IN_FUNCTION;

	msg_body.msg_type = (unsigned short)HI_CHN_MSG_PCM_CLOSE;
	msg_body.pcm_mode = (unsigned short)substream->stream;
	msg_body.pcm_device = (unsigned short)substream->pcm->device;
	ret = hi6210_mailbox_send_data(&msg_body, sizeof(struct hifi_chn_pcm_close), 0);
	if (ret)
		ret = -EBUSY;

	OUT_FUNCTION;

	return ret;
}


static int hi6210_pcm_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	int ret = 0;
	struct hi6210_runtime_data *prtd = (struct hi6210_runtime_data *)substream->runtime->private_data;
	size_t bytes = params_buffer_bytes(params);
	int device = substream->pcm->device;

	if ((device != PCM_DEVICE_NORMAL) && (device != PCM_DEVICE_DIRECT) && (device != PCM_DEVICE_FAST)) {
		return ret;
	}

	if (NULL == prtd) {
		loge("[%s:%d] prtd is null\n",  __FUNCTION__, __LINE__);
		return -EINVAL;
	}

	ret = snd_pcm_lib_malloc_pages(substream, bytes);
	if (ret < 0) {
		loge("snd_pcm_lib_malloc_pages ret : %d\n", ret);
		return ret;
	}

	spin_lock(&prtd->lock);
	prtd->period_size = params_period_bytes(params);
	prtd->period_next = 0;
	spin_unlock(&prtd->lock);

	ret = hi6210_pcm_notify_hw_params(substream, params);
	if (ret < 0) {
		loge("[%s:%d] pcm mode %d error\n", __FUNCTION__, __LINE__, substream->stream);
		snd_pcm_lib_free_pages(substream);
	}

	return ret;
}

static int hi6210_pcm_hw_free(struct snd_pcm_substream *substream)
{
	int ret = 0;
	int i   = 0;
	struct hi6210_runtime_data *prtd = NULL;
	int device = substream->pcm->device;

	prtd = (struct hi6210_runtime_data *)substream->runtime->private_data;

	if ((device != PCM_DEVICE_NORMAL) && (device != PCM_DEVICE_DIRECT) && (device != PCM_DEVICE_FAST)) {
		return ret;
	}

	if (NULL == prtd) {
		loge("[%s:%d] prtd is null\n",  __FUNCTION__, __LINE__);
		return -EINVAL;
	}

	for(i = 0; i < 30 ; i++) {  /* wait for dma ok */
		if (STATUS_STOP == prtd->status) {
			break;
		} else {
			msleep(10);
		}
	}
	if (30 == i) {
		logi("timeout for waiting for stop info from other\n");
	}

	ret = hi6210_pcm_notify_hw_free(substream);
	if (ret < 0) {
		loge("[%s:%d] free fail device %d\n", __FUNCTION__, __LINE__, substream->pcm->device);
	}

	ret = snd_pcm_lib_free_pages(substream);

	return ret;
}

static int hi6210_pcm_prepare(struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct hi6210_runtime_data *prtd = (struct hi6210_runtime_data *)substream->runtime->private_data;
	int device = substream->pcm->device;

	if ((device != PCM_DEVICE_NORMAL) && (device != PCM_DEVICE_DIRECT) && (device != PCM_DEVICE_FAST)) {
		return ret;
	}

	if (NULL == prtd) {
		loge("[%s:%d] prtd is null\n",  __FUNCTION__, __LINE__);
		return -EINVAL;
	}

	/* init prtd */
	spin_lock(&prtd->lock);
	prtd->status        = STATUS_STOP;
	prtd->period_next   = 0;
	prtd->period_cur    = 0;
	spin_unlock(&prtd->lock);

	ret = hi6210_pcm_notify_prepare(substream);

	return ret;
}

static int hi6210_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	int ret = 0;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct hi6210_runtime_data *prtd = (struct hi6210_runtime_data *)substream->runtime->private_data;
	unsigned int num_periods = runtime->periods;
	int device = substream->pcm->device;

	if ((device != PCM_DEVICE_NORMAL) && (device != PCM_DEVICE_DIRECT) && (device != PCM_DEVICE_FAST)) {
		return ret;
	}

	if (NULL == prtd) {
		loge("[%s:%d] prtd is null\n",  __FUNCTION__, __LINE__);
		return -EINVAL;
	}

	logi("device %d mode %d trigger %d \n", substream->pcm->device, substream->stream, cmd);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		ret = hi6210_pcm_notify_trigger(cmd, substream);
		if (ret < 0) {
			loge("[%s:%d trigger %d failed, ret : %d\n", __FUNCTION__, __LINE__, cmd, ret);
		} else {
			spin_lock(&prtd->lock);
			prtd->status = STATUS_RUNNING;
			prtd->period_next = (prtd->period_next + 1) % num_periods;
			spin_unlock(&prtd->lock);
		}
		break;

	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		spin_lock(&prtd->lock);
		prtd->status = STATUS_STOPPING;
		spin_unlock(&prtd->lock);

		ret = hi6210_pcm_notify_trigger(cmd, substream);
		if (ret < 0) {
			loge("hi6210_notify_pcm_trigger ret : %d\n", ret);
		}

		break;

	default:
		loge("trigger cmd error : %d\n", cmd);
		ret = -EINVAL;
		break;

	}

	return ret;
}

static snd_pcm_uframes_t hi6210_pcm_pointer(struct snd_pcm_substream *substream)
{
	long frame = 0L;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct hi6210_runtime_data *prtd = (struct hi6210_runtime_data *)substream->runtime->private_data;
	int device = substream->pcm->device;

	if ((device != PCM_DEVICE_NORMAL) && (device != PCM_DEVICE_DIRECT) && (device != PCM_DEVICE_FAST)) {
		return (snd_pcm_uframes_t)frame;
	}

	if (NULL == prtd) {
		loge("[%s:%d] prtd is null\n",  __FUNCTION__, __LINE__);
		return -EINVAL;
	}

	frame = bytes_to_frames(runtime, prtd->period_cur * prtd->period_size);
	if (frame >= runtime->buffer_size)
		frame = 0;

	return (snd_pcm_uframes_t)frame;
}

static int hi6210_pcm_open(struct snd_pcm_substream *substream)
{
	int ret = 0;
	struct hi6210_runtime_data *prtd = NULL;

	if ((substream->stream != SNDRV_PCM_STREAM_PLAYBACK) && (substream->stream != SNDRV_PCM_STREAM_CAPTURE)) {
		loge("[%s:%d] pcm mode %d error\n", __FUNCTION__, __LINE__, substream->stream);
		return -1;
	}

	switch (substream->pcm->device) {
	case PCM_DEVICE_NORMAL:
		if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream) {
			spin_lock_bh(&g_pcm_pb_open_lock);
			pdata.pcm_pb_status_open = (u32)1;
			spin_unlock_bh(&g_pcm_pb_open_lock);
			snd_soc_set_runtime_hwparams(substream, &hi6210_hardware_playback);
			prtd = &pdata.pcm_rtd_playback;
			prtd->substream = substream;
		} else {
			spin_lock_bh(&g_pcm_cp_open_lock);
			pdata.pcm_cp_status_open = (u32)1;
			spin_unlock_bh(&g_pcm_cp_open_lock);
			snd_soc_set_runtime_hwparams(substream, &hi6210_hardware_capture);
			prtd = &pdata.pcm_rtd_capture;
			prtd->substream = substream;
		}
		break;
	case PCM_DEVICE_DIRECT:
		if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream) {
			spin_lock_bh(&g_pcm_pb_open_lock);
			pdata.pcm_direct_pb_status_open = (u32)1;
			spin_unlock_bh(&g_pcm_pb_open_lock);
			snd_soc_set_runtime_hwparams(substream, &hi6210_hardware_direct_playback);
			prtd = &pdata.pcm_rtd_direct_playback;
			prtd->substream = substream;
		}
		break;
	case PCM_DEVICE_FAST:
		if (SNDRV_PCM_STREAM_PLAYBACK == substream->stream) {
			spin_lock_bh(&g_pcm_pb_open_lock);
			pdata.pcm_fast_pb_status_open = (u32)1;
			spin_unlock_bh(&g_pcm_pb_open_lock);
			snd_soc_set_runtime_hwparams(substream, &hi6210_hardware_playback);
			prtd = &pdata.pcm_rtd_fast_playback;
			prtd->substream = substream;
		} else {
			spin_lock_bh(&g_pcm_cp_open_lock);
			pdata.pcm_cp_status_open = (u32)1;
			spin_unlock_bh(&g_pcm_cp_open_lock);
			snd_soc_set_runtime_hwparams(substream, &hi6210_hardware_capture);
			prtd = &pdata.pcm_rtd_capture;
			prtd->substream = substream;
		}
		break;
	default:
		ret = snd_soc_set_runtime_hwparams(substream, &hi6210_hardware_modem_playback);
		break;
	}

	if (prtd) {
		spin_lock(&prtd->lock);
		prtd->period_cur  = 0;
		prtd->period_next = 0;
		prtd->period_size = 0;
		prtd->status      = STATUS_STOP;
		substream->runtime->private_data = prtd;
		spin_unlock(&prtd->lock);

		ret = hi6210_pcm_notify_open(substream);
	}

	return ret;
}

static int hi6210_pcm_close(struct snd_pcm_substream *substream)
{
	int ret = 0;
	int device = substream->pcm->device;

	if ((device != PCM_DEVICE_NORMAL) && (device != PCM_DEVICE_DIRECT) && (device != PCM_DEVICE_FAST)) {
		return ret;
	}

	if (NULL == substream->runtime->private_data) {
		logi("[%s:%d] prtd is null\n", __FUNCTION__, __LINE__);
	}

	logi("device %d, mode %d close\n", substream->pcm->device, substream->stream);
	switch (substream->pcm->device) {
	case PCM_DEVICE_NORMAL:
		if(SNDRV_PCM_STREAM_PLAYBACK == substream->stream) {
			spin_lock_bh(&g_pcm_pb_open_lock);
			pdata.pcm_pb_status_open = (u32)0;
			ret = hi6210_pcm_notify_close(substream);
			spin_unlock_bh(&g_pcm_pb_open_lock);
		} else {
			spin_lock_bh(&g_pcm_cp_open_lock);
			pdata.pcm_cp_status_open = (u32)0;
			ret = hi6210_pcm_notify_close(substream);
			spin_unlock_bh(&g_pcm_cp_open_lock);
		}
		break;
	case PCM_DEVICE_DIRECT:
		if(SNDRV_PCM_STREAM_PLAYBACK == substream->stream) {
			spin_lock_bh(&g_pcm_pb_open_lock);
			pdata.pcm_direct_pb_status_open = (u32)0;
			ret = hi6210_pcm_notify_close(substream);
			spin_unlock_bh(&g_pcm_pb_open_lock);
		}
		break;
	case PCM_DEVICE_FAST:
		if(SNDRV_PCM_STREAM_PLAYBACK == substream->stream) {
			spin_lock_bh(&g_pcm_pb_open_lock);
			pdata.pcm_fast_pb_status_open = (u32)0;
			ret = hi6210_pcm_notify_close(substream);
			spin_unlock_bh(&g_pcm_pb_open_lock);
		} else {
			spin_lock_bh(&g_pcm_cp_open_lock);
			pdata.pcm_cp_status_open = (u32)0;
			ret = hi6210_pcm_notify_close(substream);
			spin_unlock_bh(&g_pcm_cp_open_lock);
		}
		break;
	default:
		break;
	}

	substream->runtime->private_data = NULL;

	return ret;
}

/* define all pcm ops of hi6210 pcm */
static struct snd_pcm_ops hi6210_pcm_ops = {
	.open       = hi6210_pcm_open,
	.close      = hi6210_pcm_close,
	.ioctl      = snd_pcm_lib_ioctl,
	.hw_params  = hi6210_pcm_hw_params,
	.hw_free    = hi6210_pcm_hw_free,
	.prepare    = hi6210_pcm_prepare,
	.trigger    = hi6210_pcm_trigger,
	.pointer    = hi6210_pcm_pointer,
};


static int preallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream 	*substream = pcm->streams[stream].substream;
	struct snd_dma_buffer 		*buf = &substream->dma_buffer;

	if ((pcm->device >= PCM_DEVICE_MAX) ||(stream >= PCM_STREAM_MAX)) {
		loge("Invalid argument  : device %d stream %d \n", pcm->device, stream);
		return -EINVAL;
	}

	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;
	buf->addr = g_pcm_dma_buf_config[pcm->device][stream].pcm_dma_buf_base;
	buf->bytes = g_pcm_dma_buf_config[pcm->device][stream].pcm_dma_buf_len;
	buf->area = ioremap(buf->addr, buf->bytes);

	if (!buf->area) {
		loge("[%s:%d] error\n", __FUNCTION__, __LINE__);
		return -ENOMEM;
	}

	return 0;
}

static void free_dma_buffers(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;

	logd("Entered %s\n", __func__);

	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;

		iounmap(buf->area);

		buf->area = NULL;
		buf->addr = 0;
	}
}

static int hi6210_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	int ret = 0;
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm  *pcm  = rtd->pcm;
	bool need_process;

	IN_FUNCTION;

	if (!card->dev->dma_mask) {
		logi("dev->dma_mask not set\n");
		card->dev->dma_mask = &hi6210_pcm_dmamask;
	}

	if (!card->dev->coherent_dma_mask) {
		logi("dev->coherent_dma_mask not set\n");
		card->dev->coherent_dma_mask = hi6210_pcm_dmamask;
	}

	logi("PLATFORM machine set\n");

	need_process = ((pcm->device == PCM_DEVICE_NORMAL)
			|| (pcm->device == PCM_DEVICE_DIRECT)
			|| (pcm->device == PCM_DEVICE_FAST));

	if (need_process) {
		/* register callback */
		ret = hi6210_notify_isr_register((void*)hi6210_notify_recv_isr);
		if (ret) {
			loge("notify Isr register error : %d\n", ret);
			goto out;
		}
	} else {
		logi("We just alloc space for the the four device \n");
		goto out;
	}
	logi("pcm-device = %d\n", pcm->device);

	if (pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream) {
		ret = preallocate_dma_buffer(pcm, SNDRV_PCM_STREAM_PLAYBACK);
		if (ret)
			goto out;
	}

	if (pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream) {
		ret = preallocate_dma_buffer(pcm, SNDRV_PCM_STREAM_CAPTURE);
		if (ret)
			goto out;
	}
out:
	OUT_FUNCTION;

	return ret;
}

static void hi6210_pcm_free(struct snd_pcm *pcm)
{
	IN_FUNCTION;
	free_dma_buffers(pcm);
	OUT_FUNCTION;
}

struct snd_soc_platform_driver hi6210_pcm_platform = {
	.ops      = &hi6210_pcm_ops,
	.pcm_new  =  hi6210_pcm_new,
	.pcm_free =  hi6210_pcm_free,
};

static int  hi6210_platform_probe(struct platform_device *pdev)
{
	int ret = -ENODEV;

	IN_FUNCTION;

	logi("hi6210_platform_probe beg\n");

	memset(&pdata, 0, sizeof(struct hi6210_pcm_private_data));/* unsafe_function_ignore: memset */

	ret = snd_soc_register_component(&pdev->dev, &hi6210_pcm_component,
			hi6210_dai, ARRAY_SIZE(hi6210_dai));
	if (ret) {
		loge("snd_soc_register_dai return %d\n" ,ret);
		goto probe_failed;
	}

	/* register platform (name : hi6210-hifi) */
	dev_set_name(&pdev->dev, HI6210_PCM);
	ret = snd_soc_register_platform(&pdev->dev, &hi6210_pcm_platform);
	if (ret) {
		loge("snd_soc_register_platform return %d\n", ret);
		snd_soc_unregister_component(&pdev->dev);
		goto probe_failed;
	}

#ifdef __DRV_AUDIO_MAILBOX_WORK__
	spin_lock_init(&pdata.pcm_rtd_playback.lock);
	spin_lock_init(&pdata.pcm_rtd_capture.lock);
	spin_lock_init(&pdata.pcm_rtd_direct_playback.lock);
	spin_lock_init(&pdata.pcm_rtd_fast_playback.lock);
	spin_lock_init(&g_pcm_cp_open_lock);
	spin_lock_init(&g_pcm_pb_open_lock);
#endif

	OUT_FUNCTION;
	logi("hi6210_platform_probe end\n");

	return ret;

probe_failed:
	OUT_FUNCTION;
	return ret;
}

static int hi6210_platform_remove(struct platform_device *pdev)
{
	memset(&pdata, 0, sizeof(struct hi6210_pcm_private_data));/* unsafe_function_ignore: memset */

	snd_soc_unregister_platform(&pdev->dev);
	snd_soc_unregister_component(&pdev->dev);
	return 0;
}


static const struct of_device_id hi6210_hifi_match_table[] =
{
	{.compatible = HI6210_PCM, },
	{ },
};
static struct platform_driver hi6210_platform_driver = {
	.driver = {
		.name  = HI6210_PCM,
		.owner = THIS_MODULE,
		.of_match_table = hi6210_hifi_match_table,
	},
	.probe  = hi6210_platform_probe,
	.remove = hi6210_platform_remove,
};

static int __init hi6210_init(void)
{
	logi("%s\n",__FUNCTION__);
	return platform_driver_register(&hi6210_platform_driver);
}
module_init(hi6210_init);

static void __exit hi6210_exit(void)
{
	platform_driver_unregister(&hi6210_platform_driver);
}
module_exit(hi6210_exit);

MODULE_AUTHOR("S00212991");
MODULE_DESCRIPTION("Hi6210 HIFI platform driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:hifi");

