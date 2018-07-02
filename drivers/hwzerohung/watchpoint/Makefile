ifeq ($(TARGET_VERSION_MODE),factory)
EXTRA_CFLAGS += -DHUNG_WP_FACTORY_MODE
endif
obj-$(CONFIG_HW_ZEROHUNG) += hung_wp_lowmemorykiller.o
obj-$(CONFIG_HW_ZEROHUNG) += hung_wp_screen.o
ifeq ($(CONFIG_ARCH_MSM),)
obj-$(CONFIG_HW_ZEROHUNG) += zrhung_wp_sochalt.o
obj-$(CONFIG_HW_ZEROHUNG) += zrhung_wp_io.o
endif
