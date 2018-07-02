ccflags-y += -I$(srctree)/drivers/hisi/secure_boot/inc -Wall -Werror

obj-$(CONFIG_HISILICON_PLATFORM_MAINTAIN)	+= hisilicon_platform_mntn.o
obj-$(CONFIG_HISILICON_PLATFORM_HISI_EASYSHELL)	+= hisi-easy-shell.o
obj-$(CONFIG_HISI_BOOT_TIME) += boottime.o
obj-$(CONFIG_HISI_BB) += blackbox/
ifeq ($(TARGET_VERSION_MODE),factory)
obj-$(CONFIG_HISI_VIRT_TO_PHYS) += virt_to_phys.o
endif
obj-$(CONFIG_HISI_POWER_OFF)	+= hisi_poweroff.o
obj-$(CONFIG_HISI_FIQ)	+= hisi_fiq.o
hisilicon_platform_mntn-y := util.o dump.o fastbootlog.o mntn_filesys.o hisi_mntn_test.o hisi_bootup_keypoint.o
subdir-ccflags-$(CONFIG_HISI_BB)               += -Idrivers/hisi/mntn/blackbox/
obj-$(CONFIG_HISI_MNTNDUMP)       += mntn_dump.o
obj-$(CONFIG_HISI_RECORD_SP)       += mntn_record_sp.o
obj-$(CONFIG_HISI_HISEE_MNTN) += hisee/
obj-$(CONFIG_HISI_BL31_MNTN) += bl31/
