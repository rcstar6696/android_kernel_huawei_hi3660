/******************************************************************************

                  版权所有 (C), 2001-2011, 华为技术有限公司

 ******************************************************************************
  文 件 名   : Hi6210-pcm.h
  版 本 号   : 初稿
  作    者   : 石旺来 s00212991
  生成日期   : 2012年7月31日
  最近修改   :
  功能描述   : Hi6210-pcm.h 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2012年7月31日
    作    者   : 石旺来 s00212991
    修改内容   : 创建文件

******************************************************************************/

#ifndef HI6210_PCM_H
#define HI6210_PCM_H


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
#include "hifi_lpp.h"


/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define HI_CHN_COMMON   \
	unsigned short  msg_type;   \
	unsigned short  pcm_device;   \
	unsigned short  pcm_mode;   \
	unsigned short  reserved;


#define INT_TO_ADDR(low,high) (void*) (unsigned long)((unsigned long long)(low) | ((unsigned long long)(high)<<32))
#define GET_LOW32(x) (unsigned int)(((unsigned long long)(unsigned long)(x))&0xffffffffULL)
#define GET_HIG32(x) (unsigned int)((((unsigned long long)(unsigned long)(x))>>32)&0xffffffffULL)
/*****************************************************************************
  3 枚举定义
*****************************************************************************/

/* HI6210 PCM运行状态 */
enum HI6210_STATUS
{
	STATUS_STOP = 0,
	STATUS_RUNNING = 1,
	STATUS_STOPPING = 2
};

enum HI6210_PCM_FORMATS
{
	PCM_FORMAT_S16_LE = 0,
	PCM_FORMAT_S24_LE_LA,       /* left alignment */
	PCM_FORMAT_S24_LE_RA,       /* right alignment */
	PCM_FORMAT_S32_LE,
	PCM_FORMAT_MAX,
};

/* AP与HIFI音频数据传输消息类型 */
enum AUDIO_MSG_ENUM
{
	/* AP DSP驱动与HIFI播放器通道模块交互消息ID */
	ID_AP_AUDIO_PLAY_DECODE_REQ         = 0xDD33,
	ID_AUDIO_AP_PLAY_DECODE_RSP         = 0xDD34,

	ID_AP_AUDIO_RECORD_START_REQ        = 0xDD40,
	ID_AP_AUDIO_RECORD_STOP_REQ         = 0xDD41,

	/* AP音频驱动与HIFI音频通道模块交互消息ID */
	ID_AP_AUDIO_PCM_OPEN_REQ            = 0xDD25,
	ID_AP_AUDIO_PCM_CLOSE_REQ           = 0xDD26,
	ID_AP_AUDIO_PCM_HW_PARA_REQ         = 0xDD27,
	ID_AP_AUDIO_PCM_HW_FREE_REQ         = 0xDD28,   /* 暂未使用，保留 */
	ID_AP_AUDIO_PCM_PREPARE_REQ         = 0xDD29,   /* 暂未使用，保留 */
	ID_AP_AUDIO_PCM_TRIGGER_REQ         = 0xDD2A,
	ID_AP_AUDIO_PCM_POINTER_REQ         = 0xDD2B,   /* 暂未使用，保留 */
	ID_AP_AUDIO_PCM_SET_BUF_CMD         = 0xDD2C,
	ID_AUDIO_AP_PCM_PERIOD_ELAPSED_CMD  = 0xDD2D,

	/* HIFI音频通道模块内部交互消息ID */
	ID_AUDIO_UPDATE_PLAY_BUFF_CMD       = 0xDD2E,
	ID_AUDIO_UPDATE_CAPTURE_BUFF_CMD    = 0xDD2F,
	ID_AUDIO_AP_PCM_TRIGGER_CNF         = 0xDDA0,
	ID_AUDIO_AP_PCM_XRUN = 0xDD2E,
};
typedef unsigned short  AUDIO_MSG_ENUM_UINT16;

enum HIFI_CHN_MSG_TYPE
{
	HI_CHN_MSG_PCM_OPEN             = ID_AP_AUDIO_PCM_OPEN_REQ,
	HI_CHN_MSG_PCM_CLOSE            = ID_AP_AUDIO_PCM_CLOSE_REQ,
	HI_CHN_MSG_PCM_HW_PARAMS        = ID_AP_AUDIO_PCM_HW_PARA_REQ,
	HI_CHN_MSG_PCM_HW_FREE          = ID_AP_AUDIO_PCM_HW_FREE_REQ,
	HI_CHN_MSG_PCM_PREPARE          = ID_AP_AUDIO_PCM_PREPARE_REQ,
	HI_CHN_MSG_PCM_TRIGGER          = ID_AP_AUDIO_PCM_TRIGGER_REQ,
	HI_CHN_MSG_PCM_POINTER          = ID_AP_AUDIO_PCM_POINTER_REQ,
	/* 数据搬运 */
	HI_CHN_MSG_PCM_SET_BUF          = ID_AP_AUDIO_PCM_SET_BUF_CMD,
	/* HIFI数据搬运完成*/
	HI_CHN_MSG_PCM_PERIOD_ELAPSED   = ID_AUDIO_AP_PCM_PERIOD_ELAPSED_CMD,
	HI_CHN_MSG_PCM_PERIOD_STOP      = ID_AUDIO_AP_PCM_TRIGGER_CNF,
	HI_CHN_MSG_PCM_XRUN      = ID_AUDIO_AP_PCM_XRUN,
};

enum IRQ_RT
{
	/* IRQ Not Handled as Other problem */
	IRQ_NH_OTHERS    = -5,
	/* IRQ Not Handled as Mailbox problem */
	IRQ_NH_MB     = -4,
	/* IRQ Not Handled as pcm MODE problem */
	IRQ_NH_MODE     = -3,
	/* IRQ Not Handled as TYPE problem */
	IRQ_NH_TYPE     = -2,
	/* IRQ Not Handled */
	IRQ_NH          = -1,
	/* IRQ HanDleD */
	IRQ_HDD         = 0,
	/* IRQ HanDleD related to PoinTeR */
	IRQ_HDD_PTR     = 1,
	/* IRQ HanDleD related to STATUS */
	IRQ_HDD_STATUS,
	/* IRQ HanDleD related to SIZE */
	IRQ_HDD_SIZE,
	/* IRQ HanDleD related to PoinTeR of Substream */
	IRQ_HDD_PTRS,
	/* IRQ HanDleD Error */
	IRQ_HDD_ERROR,
};
typedef enum IRQ_RT irq_rt_t;

typedef irq_rt_t (*irq_hdl_t)(void *, unsigned int);

/*****************************************************************************
  4 消息头定义
*****************************************************************************/

/*****************************************************************************
  5 消息定义
*****************************************************************************/

/*****************************************************************************s
  6 STRUCT定义
*****************************************************************************/

#ifdef __DRV_AUDIO_MAILBOX_WORK__
struct hi6210_pcm_mailbox_data
{
	struct list_head node;
	unsigned short   msg_type;   /* 消息类型，HIFI_CHN_MSG_TYPE */
	unsigned short   pcm_mode;   /* PLAYBACK 或 CAPTURE */
	void *           substream;  /* 通道SubStream对象的地址 */
};
#endif

/* runtime data，用于PLAYBACK或CAPTURE */
struct hi6210_runtime_data
{
	spinlock_t          lock;         /* protect hi6210_runtime_data */
	unsigned int        period_next;  /* record which period to fix dma next time */
	unsigned int        period_cur;   /* record which period using now */
	unsigned int        period_size;  /* DMA SIZE */
	enum HI6210_STATUS  status;       /* pcm status running or stop */
	struct snd_pcm_substream * substream;
#ifdef __DRV_AUDIO_MAILBOX_WORK__
	struct hi6210_pcm_mailbox_data hi6210_pcm_mailbox;
#endif
	long snd_pre_time;
};

struct hifi_pcm_config
{
	unsigned int channels;
	unsigned int rate;
	unsigned int period_size;
	unsigned int period_count;
	unsigned int format;
};

/* AP核间通信传递给HIFI的内容，在PCM OPEN时使用的结构体信息 */
struct hifi_chn_pcm_open
{
	HI_CHN_COMMON
	struct hifi_pcm_config config;
};

/* AP核间通信传递给HIFI的内容，在PCM OPEN时使用的结构体信息 */
struct hifi_chn_pcm_close
{
	HI_CHN_COMMON
};

/* AP核间通信传递给HIFI的内容，在HW PARAMS时使用的结构体信息 */
struct hifi_chn_pcm_hw_params
{
	HI_CHN_COMMON
	unsigned int    channel_num;    /* 通道的声道数 */
	unsigned int    sample_rate;    /* 通道采样率 */
	unsigned int    format;         /* 音频格式 */
};

/* AP核间通信传递给HIFI的内容，在HW FREE时使用的结构体信息
struct hifi_chn_pcm_hw_free
{
    HI_CHN_COMMON
};*/

/* AP核间通信传递给HIFI的内容，在PCM PREPARE时使用的结构体信息
struct hifi_chn_pcm_prepare
{
    HI_CHN_COMMON
};*/

/* AP核间通信传递给HIFI的内容，在PCM TRIGGER时使用的结构体信息 */
struct hifi_chn_pcm_trigger
{
	HI_CHN_COMMON
	unsigned short  tg_cmd;         /* 触发类型,例如SNDRV_PCM_TRIGGER_START */
	unsigned short  reserved1;      /* reserved for aligned */
	unsigned int    substream_l32;  /* 通道SubStream对象的地址 */
	unsigned int    substream_h32;  /* 通道SubStream对象的地址 */
	unsigned int    data_addr;      /* DMA数据搬运的数据指针，物理地址 */
	unsigned int    data_len;       /* 数据长度，单位Byte */
};

/* AP核间通信传递给HIFI的内容，DMA Buffer数据传递时使用 */
struct hifi_channel_set_buffer
{
	HI_CHN_COMMON
	unsigned int    data_addr;  /* DMA数据搬运的数据指针，物理地址
									   PLAYBACK时作为DMA数据搬运源
									   CAPTURE 时作为DMA数据搬运目的地 */
	unsigned int    data_len;   /* 数据长度，单位Byte */
};

/* HIFI核间通信传递给AP的内容，在DMA数据搬运完成后传递 */
struct hifi_chn_pcm_period_elapsed
{
	HI_CHN_COMMON
	unsigned int    substream_l32;  /* 通道SubStream对象的地址 */
	unsigned int    substream_h32;  /* 通道SubStream对象的地址 */
	unsigned int    msg_timestamp;
};

#define PCM_DMA_BUF_PLAYBACK_PRIMARY_LEN (48*20*2*2*4)

#define PCM_DMA_BUF_PLAYBACK_DIRECT_LEN (384*20*4*2*4)

#define PCM_DMA_BUF_PLAYBACK_LEN    (0x00030000)
#define PCM_DMA_BUF_0_PLAYBACK_BASE (PCM_PLAY_BUFF_LOCATION)
#define PCM_DMA_BUF_0_PLAYBACK_LEN  (PCM_DMA_BUF_PLAYBACK_PRIMARY_LEN)
#define PCM_DMA_BUF_0_CAPTURE_BASE  (PCM_DMA_BUF_0_PLAYBACK_BASE+PCM_DMA_BUF_0_PLAYBACK_LEN)
#define PCM_DMA_BUF_0_CAPTURE_LEN   (PCM_DMA_BUF_PLAYBACK_LEN)

#define PCM_DMA_BUF_1_PLAYBACK_BASE (PCM_DMA_BUF_0_CAPTURE_BASE+PCM_DMA_BUF_0_CAPTURE_LEN)
#define PCM_DMA_BUF_1_PLAYBACK_LEN  (PCM_DMA_BUF_PLAYBACK_DIRECT_LEN)
#define PCM_DMA_BUF_1_CAPTURE_BASE  (PCM_DMA_BUF_1_PLAYBACK_BASE+PCM_DMA_BUF_1_PLAYBACK_LEN)
#define PCM_DMA_BUF_1_CAPTURE_LEN   (193*1024)

#define PCM_DMA_BUF_2_PLAYBACK_BASE (PCM_DMA_BUF_1_CAPTURE_BASE+PCM_DMA_BUF_1_CAPTURE_LEN)
#define PCM_DMA_BUF_2_PLAYBACK_LEN  (PCM_DMA_BUF_PLAYBACK_LEN)
#define PCM_DMA_BUF_2_CAPTURE_BASE  (PCM_DMA_BUF_2_PLAYBACK_BASE+PCM_DMA_BUF_2_PLAYBACK_LEN)
#define PCM_DMA_BUF_2_CAPTURE_LEN   (PCM_DMA_BUF_PLAYBACK_LEN)

#define PCM_DMA_BUF_3_PLAYBACK_BASE (PCM_DMA_BUF_2_CAPTURE_BASE+PCM_DMA_BUF_2_CAPTURE_LEN)
#define PCM_DMA_BUF_3_PLAYBACK_LEN  (0)
#define PCM_DMA_BUF_3_CAPTURE_BASE  (PCM_DMA_BUF_3_PLAYBACK_BASE+PCM_DMA_BUF_3_PLAYBACK_LEN)
#define PCM_DMA_BUF_3_CAPTURE_LEN   (0)

#define PCM_STREAM_MAX              (2)
#define PCM_DEVICE_MAX              (7)

typedef struct pcm_dma_buf_config
{
	u64       pcm_dma_buf_base;
	u64       pcm_dma_buf_len;
}PCM_DMA_BUF_CONFIG;

/*****************************************************************************
  7 UNION定义
*****************************************************************************/


/*****************************************************************************
  8 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  9 全局变量声明
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
void snd_pcm_print_timeout(struct snd_pcm_substream *substream, unsigned int timeout_type);
void snd_pcm_reset_pre_time(struct snd_pcm_substream *substream);

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /*HI6210_PCM_H*/

