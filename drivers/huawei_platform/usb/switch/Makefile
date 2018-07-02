obj-$(CONFIG_SWITCH)        += switch_ap/
obj-$(CONFIG_SWITCH)        += switch_sensorhub/
ifeq ($(wildcard $(srctree)/drivers/huawei_platform/inputhub/$(TARGET_BOARD_PLATFORM)),)
ccflags-y  += -I$(srctree)/drivers/huawei_platform/inputhub/default/
else
ccflags-y  += -I$(srctree)/drivers/huawei_platform/inputhub/$(TARGET_BOARD_PLATFORM)/
endif
obj-y += switch_fsa9685_common.o
