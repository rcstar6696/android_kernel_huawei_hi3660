obj-$(CONFIG_HUAWEI_CHARGER_AP) += huawei_charger.o charging_core.o vbat_ovp.o
obj-$(CONFIG_BQ2419X_CHARGER) += bq2419x/
obj-$(CONFIG_BQ2429X_CHARGER) += bq2429x/
obj-$(CONFIG_BQ25892_CHARGER) += bq25892/
obj-$(CONFIG_DUAL_CHARGER) += dual_charger/
obj-$(CONFIG_DIRECT_CHARGER) += direct_charger/

ifeq ($(HLTHERM_RUNTEST), true)
    CFLAGS_KERNEL += -DCONFIG_HLTHERM_RUNTEST
endif
