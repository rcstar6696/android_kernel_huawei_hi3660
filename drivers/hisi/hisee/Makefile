EXTRA_CFLAGS 	+= -Idrivers/hisi/mntn/blackbox/platform_lpm3
#LOCAL_C_INCLUDES += vendor/huawei/chipset_common/modules/libc_sec/include
#LOCAL_CFLAGS += -Ivendor/huawei/chipset_common/modules/libc_sec/include

subdir-ccflags-y              += -Wall -Werror

ifeq ($(TARGET_BOARD_PLATFORM), hi3660)
EXTRA_CFLAGS += -DHISEE_POWER_MANAGE_COUNT
endif

ifeq ($(TARGET_BOARD_PLATFORM), kirin970)
EXTRA_CFLAGS += -DHISEE_POWER_MANAGE_PRO
EXTRA_CFLAGS += -DCONFIG_HISEE_CHIPTEST_SUPPORT_APPLET_OPTIMIZATION
ifneq ($(chip_type), es)
EXTRA_CFLAGS += -DCFG_HICOS_MISCIMG_PATCH
endif
endif

#if support casd key, open the macro
#EXTRA_CFLAGS += -DHISEE_CHIPTEST_SUPPORT_CASDKEY


obj-$(CONFIG_HISI_HISEE)  += hisi_hisee.o
obj-$(CONFIG_HISI_HISEE)  += hisi_hisee_power.o
obj-$(CONFIG_HISI_HISEE)  += hisi_hisee_upgrade.o
obj-$(CONFIG_HISI_HISEE)  += hisi_hisee_chip_test.o
obj-$(CONFIG_HISI_HISEE)  += hisi_hisee_fs.o
obj-$(CONFIG_HISI_HISEE)  += hisi_hisee_autotest.o
