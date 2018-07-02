ccflags-y += -Idrivers/media/huawei/camera

ifeq ($(CONFIG_HISI_CAMERA_HISTAR_ISPV110), y)
ccflags-y += -Idrivers/media/huawei/camera/platform/hi6250
obj-y += hi6250/
endif

ifeq ($(CONFIG_HISI_CAMERA_HISTAR_ISPV150), y)
ccflags-y += -Idrivers/media/huawei/camera/platform/hi3660
obj-y += hi3660/
endif

ifeq ($(CONFIG_HISI_CAMERA_HISTAR_ISPV160), y)
ccflags-y += -Idrivers/media/huawei/camera/platform/kirin970
obj-y += kirin970/
endif

