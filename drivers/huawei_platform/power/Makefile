obj-$(CONFIG_HUAWEI_CHARGER)	+= charger/
obj-$(CONFIG_HUAWEI_COUL)	+= coul/
obj-$(CONFIG_HUAWEI_KSTATE)	+= hw_kstate/
obj-$(CONFIG_HW_PTM) += hw_ptm/
obj-$(CONFIG_HUAWEI_FEATURE_PRINT_PID_NAME) += pid_socket/
obj-$(CONFIG_INPUT_HW_ATE)	+= ate/
obj-$(CONFIG_HUAWEI_BASTET)	+= bastet/
obj-$(CONFIG_HUAWEI_SMPL)	+= smpl/
obj-$(CONFIG_BUCKBOOST)	+= buckboost/
obj-$(CONFIG_HUAWEI_USB_SHORT_CIRCUIT_PROTECT)	+= usb_short_circuit_protect/
obj-$(CONFIG_HUAWEI_CPU_BUCK)	+= cpu_buck/


ifeq ($(HLTHERM_RUNTEST),true)
CFLAGS_KERNEL += -DCONFIG_HLTHERM_RUNTEST
endif

obj-$(CONFIG_HUAWEI_WATER_CHECK) += water_check/
obj-$(CONFIG_DOZE_FILTER)	+= wifi_filter/
obj-$(CONFIG_HUAWEI_DUBAI) += dubai/
