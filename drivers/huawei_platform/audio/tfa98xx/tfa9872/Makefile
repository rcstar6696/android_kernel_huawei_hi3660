# tfa9872 smartPA driver

ifeq ($(TARGET_VERSION_MODE),factory)
EXTRA_CFLAGS += -DNXP_FACTORY_MODE
endif
ccflags-$(CONFIG_TFA9872) += -Idrivers/devkit/audiokit/
obj-$(CONFIG_TFA9872)	+= tfa9872.o
