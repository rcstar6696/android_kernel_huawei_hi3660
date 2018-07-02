# slimbus driver
subdir-ccflags-y += -Wall -Werror
ccflags-y += -Idrivers/devkit/audiokit/

EXTRA_CFLAGS += -Idrivers/hisi/slimbus/vendor/candance/include
EXTRA_CFLAGS +=	-Idrivers/hisi/slimbus/vendor/candance
EXTRA_CFLAGS +=	-Isound/soc/hisilicon



hisilicon-slimbus-platform-objs		:= vendor/candance/cps_bm.o vendor/candance/src/csmi.o vendor/candance/slimbus_drv.o
hisilicon-slimbus-platform-objs		+= slimbus_6402.o slimbus.o slimbus_utils.o slimbus_6403.o
ifneq ($(TARGET_BUILD_VARIANT),user)
hisilicon-slimbus-platform-objs += slimbus_debug.o
endif
obj-$(CONFIG_HISILICON_PLATFORM_SLIMBUS)		+= hisilicon-slimbus-platform.o
