# Add your debugging flag (or not) to CFLAGS
include drivers/vcodec/hi_vcodec/venc_hivna_v210/drv/venc/drv_venc_make.cfg

EXTRA_CFLAGS += $(VENC_CFLAGS) -fno-pic
EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/venc_hivna_v210/drv/include
EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/venc_hivna_v210/include
EXTRA_CFLAGS += -Idrivers/vcodec/hi_vcodec/venc_hivna_v210/drv/venc

ifneq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DUSER_DISABLE_VENC_PROC
endif

ifeq ($(TARGET_BOARD_PLATFORM), kirin970)
EXTRA_CFLAGS += -DPLATFORM_KIRIN970
endif

obj-$(CONFIG_HI_VCODEC_VENC) += hi_omxvenc.o
hi_omxvenc-objs :=  venc_regulator.o     \
                    drv_venc_intf.o      \
                    drv_venc_efl.o       \
                    drv_venc_osal.o      \
                    drv_venc.o           \
                    drv_venc_proc.o      \
                    hal_venc.o           \
                    hi_drv_mem.o


#obj-m += venc.o
#venc-y :=           venc_regulator.o     \
                    drv_venc_intf.o      \
                    drv_venc_efl.o       \
                    drv_venc_osal.o      \
                    drv_venc.o           \
                    drv_venc_proc.o      \
                    hal_venc.o           \
                    hi_drv_mem.o

