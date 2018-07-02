ifeq ($(wildcard $(srctree)/drivers/huawei_platform/inputhub/$(TARGET_BOARD_PLATFORM)),)
ccflags-y  += -I$(srctree)/drivers/huawei_platform/inputhub/default/
else
ccflags-y  += -I$(srctree)/drivers/huawei_platform/inputhub/$(TARGET_BOARD_PLATFORM)/
endif
EXTRA_CFLAGS += -Idrivers/hisi/tzdriver
ifeq ($(TARGET_BOARD_PLATFORM), kirin970)
ifeq ($(chip_type), es)
EXTRA_CFLAGS += -DCOMBOPHY_ES_BUGFIX
endif
endif

obj-$(CONFIG_CONTEXTHUB) += inputhub_api.o common.o
obj-$(CONFIG_CONTEXTHUB_SHMEM) += shmem.o
obj-$(CONFIG_CONTEXTHUB_SHELL) += shell_dbg.o
obj-$(CONFIG_CONTEXTHUB_LOADMONITOR) += loadmonitor.o
obj-$(CONFIG_CONTEXTHUB_PD) += pd.o
obj-$(CONFIG_CONTEXTHUB_PLAYBACK) += playback.o
obj-$(CONFIG_HISI_FLP)   += flp/


