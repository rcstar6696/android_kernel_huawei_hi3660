# hifi dsp device driver
subdir-ccflags-y += -Wall -Werror

EXTRA_CFLAGS += -Idrivers/hisi/slimbus
EXTRA_CFLAGS += -Idrivers/hisi/hifi_dsp
EXTRA_CFLAGS +=	-Iinclude/linux/hisi/hi64xx/

obj-$(CONFIG_SOUND_TRIGGER)	+= mlib_ringbuffer.o
obj-$(CONFIG_SOUND_TRIGGER)	+= mlib_static_ringbuffer.o
obj-$(CONFIG_SOUND_TRIGGER)	+= soundtrigger_dma_drv.o
obj-$(CONFIG_SOUND_TRIGGER)	+= soundtrigger_event.o
