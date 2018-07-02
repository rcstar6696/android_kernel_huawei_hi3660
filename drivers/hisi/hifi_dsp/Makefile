subdir-ccflags-y += -Wall -Werror

ifeq ($(CLT_IV),true)
EXTRA_CFLAGS += -DCLT_VOICE
endif

# hifi dsp device driver
ifeq ($(CONFIG_HIFI_IPC_3650),y)
EXTRA_CFLAGS += -Iinclude/modem/include/config/hi365x
EXTRA_CFLAGS += -Iinclude/modem/include/drv/hi365x
endif

ifeq ($(CFG_CONFIG_HISI_FAMA),true)
LOCAL_CFLAGS += -DCONFIG_HISI_FAMA
endif

ifeq ($(CONFIG_HIFI_IPC_3660),y)
EXTRA_CFLAGS += -Iinclude/modem/include/config/hi365x
EXTRA_CFLAGS += -Iinclude/modem/include/drv/hi365x
endif
ifeq ($(CONFIG_HIFI_IPC_6250),y)
EXTRA_CFLAGS += -DPLATFORM_HI6250
EXTRA_CFLAGS += -DEXPAND_HIFI_RUNNING_REGION
EXTRA_CFLAGS += -Iinclude/modem/include/config/hi62x0
EXTRA_CFLAGS += -Iinclude/modem/include/drv/hi62x0
endif

EXTRA_CFLAGS += -DMULTI_MIC
EXTRA_CFLAGS += -Idrivers/hisi/hifi_mailbox/mailbox
EXTRA_CFLAGS += -Idrivers/hisi/hifi_mailbox/ipcm
EXTRA_CFLAGS += -Isound/usb/hifi/
ifneq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DENABLE_HIFI_DEBUG
endif

obj-$(CONFIG_HIFI_DSP_ONE_TRACK)	+= hifi_lpp.o
obj-$(CONFIG_HIFI_DSP_ONE_TRACK)	+= hifi_om.o
ifneq ($(TARGET_BUILD_VARIANT),user)
obj-$(CONFIG_HIFI_DSP_ONE_TRACK)	+= memcpy_opt.o
endif

obj-y	+= voice_proxy/
ccflags-$(CONFIG_HIFI_DSP_ONE_TRACK) += -Idrivers/devkit/audiokit/
