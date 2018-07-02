subdir-ccflags-y += -Wall -Werror

EXTRA_CFLAGS += -Idrivers/hisi/hi64xx_dsp/
EXTRA_CFLAGS += -Idrivers/hisi/soundtrigger/

ifeq ($(CLT_IV), true)
EXTRA_CFLAGS += -DCLT_AUDIO
endif

ifeq ($(CONFIG_HI6402_MBHC_ANC),y)
EXTRA_CFLAGS += -DANC_HEADSET
EXTRA_CFLAGS += -DINVERT_HEADSET
endif

ifneq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DENABLE_HI64XX_CODEC_DEBUG
endif

ccflags-y += -Idrivers/devkit/audiokit/
obj-y += hi_cdc_ctrl.o
obj-y += hi_cdc_ssi.o
obj-y += hi64xx_resmgr.o
obj-y += hi64xx_mbhc.o
obj-y += hi64xx_irq.o
obj-y += hi64xx_compat.o
obj-y += hi64xx_utils.o
obj-y += hi64xx_vad.o
obj-y += asp_dma.o
