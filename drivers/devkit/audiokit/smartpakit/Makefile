# Makefile for the smartpa devkit drivers.
#

# Each configuration option enables a list of files.
ifeq ($(TARGET_BUILD_VARIANT),eng)
	EXTRA_CFLAGS += -DSMARTPAKIT_MISC_RW_PERMISSION_ENABLE
endif
ifdef CONFIG_HUAWEI_SMARTPAKIT_AUDIO
	ccflags-y += -Idrivers/devkit/audiokit/
endif
obj-$(CONFIG_HUAWEI_SMARTPAKIT_AUDIO) += smartpakit.o
obj-$(CONFIG_HUAWEI_SMARTPAKIT_AUDIO) += smartpakit_i2c.o
ifeq ($(TARGET_BUILD_VARIANT),eng)
	obj-m += smartpakit_info.o
endif

