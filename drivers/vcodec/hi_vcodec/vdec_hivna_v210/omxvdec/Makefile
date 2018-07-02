
TOP := drivers/../..

PLATFORM_DIR := platform

EXTRA_CFLAGS += -DENV_ARMLINUX_KERNEL
EXTRA_CFLAGS += -DHIVDEC_SMMU_SUPPORT

EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/vdec_hivna_v210/include
EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/vdec_hivna_v210/omxvdec
EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/vdec_hivna_v210/omxvdec/$(PLATFORM_DIR)
EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/vdec_hivna_v210/vfmw_v4.0
EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/vdec_hivna_v210/vfmw_v4.0/firmware/osal/linux_kernel
EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/vdec_hivna_v210/vfmw_v4.0/firmware/osal
EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/vdec_hivna_v210/vfmw_v4.0/firmware/common
EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/vdec_hivna_v210/vfmw_v4.0/scene/kirin
EXTRA_CFLAGS += -fno-pic
EXTRA_CFLAGS += -DOMXVDEC_TVP_CONFLICT

ifneq ($(TARGET_BUILD_VARIANT), user)
EXTRA_CFLAGS += -DUSER_DISABLE_VDEC_PROC
endif

ifeq ($(TARGET_BOARD_PLATFORM), kirin970)
EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/vdec_hivna_v210/vfmw_v4.0/firmware/hal/HiSCDV300
EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/vdec_hivna_v210/vfmw_v4.0/firmware/product/HiVCodecV200
EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/vdec_hivna_v210/vfmw_v4.0/firmware/hal/HiVDHV500R002
EXTRA_CFLAGS +=-DPLATFORM_KIRIN970
endif

#build in
obj-$(CONFIG_HI_VCODEC_VDEC) += hi_omxvdec.o
hi_omxvdec-objs += omxvdec.o $(PLATFORM_DIR)/regulator.o $(PLATFORM_DIR)/memory.o

