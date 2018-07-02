# UFSHCD makefile
obj-$(CONFIG_SCSI_UFS_KIRIN) += ufs-kirin.o
obj-$(CONFIG_SCSI_UFS_QCOM) += ufs-qcom.o
obj-$(CONFIG_SCSI_UFSHCD) += ufshcd.o ufs_quirks.o
obj-$(CONFIG_HISI_BOOTDEVICE) += ufs-rpmb.o
obj-$(CONFIG_SCSI_UFSHCD_PCI) += ufshcd-pci.o
obj-$(CONFIG_SCSI_UFSHCD_PLATFORM) += ufshcd-pltfrm.o
obj-$(CONFIG_SCSI_UFS_FAULT_INJECT) += ufs_fault_inject.o
obj-$(CONFIG_SCSI_UFS_BLOCK_TEST) += ufs_block_test.o
obj-$(CONFIG_SCSI_UFS_TEST) += ufs_test.o
obj-$(CONFIG_HISI_DEBUG_FS) += ufs_debugfs.o
obj-$(CONFIG_HUAWEI_UFS_DSM) += dsm_ufs.o
obj-$(CONFIG_HUAWEI_UFS_VENDOR_MODE) += ufs_vendor_mode.o
obj-$(CONFIG_HISI_AB_PARTITION) += ufs-kirin-lib.o
obj-$(CONFIG_HISI_UFS_MANUAL_BKOPS) += hisi_ufs_bkops.o

ifeq ($(TARGET_VERSION_MODE),factory)
    EXTRA_CFLAGS += -DHEALTH_INFO_FACTORY_MODE
endif
