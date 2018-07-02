obj-$(CONFIG_HISI_COUL_HI6421V500)	+= hi6421v500/
obj-$(CONFIG_HISI_COUL_HI6555V100)	+= hi6555v100/
obj-$(CONFIG_HISI_COUL_HI6421V600)      += hi6421v600/
obj-$(CONFIG_HISI_COUL_HI6421V600_SPMI)      += hi6421v600_spmi/


ifeq ($(wildcard $(srctree)/drivers/huawei_platform/inputhub/$(TARGET_BOARD_PLATFORM)),)
ccflags-y  += -I$(srctree)/drivers/huawei_platform/inputhub/default/
else
ccflags-y  += -I$(srctree)/drivers/huawei_platform/inputhub/$(TARGET_BOARD_PLATFORM)/
endif

obj-$(CONFIG_HISI_COUL)	            += hisi_coul_core.o hisi_coul_drv.o
obj-$(CONFIG_HISI_COUL_TEST)        += hisi_coul_drv_test.o

