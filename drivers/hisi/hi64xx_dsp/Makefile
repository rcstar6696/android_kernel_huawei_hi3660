# hi6402 dsp device driver
subdir-ccflags-y += -Wall -Werror

ccflags-y += -Idrivers/devkit/audiokit/
EXTRA_CFLAGS += -Idrivers/hisi/slimbus/
EXTRA_CFLAGS += -Idrivers/hisi/hifi_dsp/
EXTRA_CFLAGS += -Iinclude/linux/hisi/
EXTRA_CFLAGS += -Idrivers/hisi/mntn/blackbox/platform_hifi/
EXTRA_CFLAGS += -Idrivers/hisi/soundtrigger/

ifneq ($(TARGET_BUILD_VARIANT),user)
	EXTRA_CFLAGS += -DENABLE_HI64XX_HIFI_DEBUG
endif
ifeq ($(CONFIG_HIFI_IPC_3660),y)
	EXTRA_CFLAGS += -DPLATFORM_HI3660
endif

ifeq ($(CFG_CONFIG_HISI_FAMA),true)
	EXTRA_CFLAGS += -DCONFIG_HISI_FAMA
endif

obj-$(CONFIG_HI6402_HIFI_MISC) += hi64xx_hifi_misc.o
obj-$(CONFIG_HI6402_HIFI_MISC) += hi6402_hifi_config.o
obj-$(CONFIG_HI6402_HIFI_MISC) += hi6403_hifi_config.o
obj-$(CONFIG_HI6402_HIFI_MISC) += hi6403es_hifi_config.o
obj-$(CONFIG_HI6402_HIFI_MISC) += hi64xx_hifi_img_dl.o
obj-$(CONFIG_HI6402_HIFI_MISC) += hi64xx_hifi_om.o
obj-$(CONFIG_HI6402_HIFI_MISC) += hi64xx_hifi_anc_beta.o
