TOP      := drivers/../..
VFMW_DIR := drivers/vcodec/hi_vcodec/vdec_hivna_v210

ifeq ($(TARGET_BOARD_PLATFORM), hi3660)
include $(VFMW_DIR)/vfmw_v4.0/firmware/product/HiVCodecV100/NOR_CFG/vfmw_make.cfg
endif

ifeq ($(TARGET_BOARD_PLATFORM), kirin970)
include $(VFMW_DIR)/vfmw_v4.0/firmware/product/HiVCodecV200/NOR_CFG/vfmw_make.cfg
endif

EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/vdec_hivna_v210/include
EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/vdec_hivna_v210/omxvdec/platform
EXTRA_CFLAGS += $(VFMW_CFLAGS) -fno-pic

#build in
obj-$(CONFIG_HI_VCODEC_VDEC) += hi_vfmw.o
hi_vfmw-objs := $(VFMW_CFILES)

#module
#obj-m += hi_vfmw.o
#hi_vfmw-y := $(VFMW_CFILES)
