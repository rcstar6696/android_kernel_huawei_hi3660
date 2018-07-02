# define_trace.h needs to know how to find our header
CFLAGS_trace.o				:= -I$(src)

ccflags-$(CONFIG_USB_DWC3_HISI)		+= -DDWC3_ENABLE_CSP
ccflags-$(CONFIG_USB_DWC3_OTG)		+= -DDWC3_OTG_FORCE_MODE
ccflags-$(CONFIG_USB_DWC3_KIRIN970)	+= -DDWC3_OTG_FORCE_RESET_GCTL

ifneq ($(TARGET_BUILD_VARIANT),user)
EXTRA_CFLAGS += -DHISI_PLUGUSB_TEST
else
ifeq ($(TARGET_VERSION_MODE),factory)
EXTRA_CFLAGS += -DHISI_PLUGUSB_TEST
endif
endif

obj-$(CONFIG_USB_DWC3)			+= dwc3.o

dwc3-y					:= core.o debug.o trace.o

ifneq ($(filter y,$(CONFIG_USB_DWC3_HOST) $(CONFIG_USB_DWC3_DUAL_ROLE)),)
	dwc3-y				+= host.o
endif

ifneq ($(filter y,$(CONFIG_USB_DWC3_GADGET) $(CONFIG_USB_DWC3_DUAL_ROLE)),)
	dwc3-y				+= gadget.o ep0.o
endif

ifneq ($(CONFIG_USB_DWC3_ULPI),)
	dwc3-y				+= ulpi.o
endif

ifneq ($(CONFIG_DEBUG_FS),)
	dwc3-y				+= debugfs.o
endif

dwc3-$(CONFIG_USB_DWC3_OTG)		+= dwc3-otg.o

##
# Platform-specific glue layers go here
#
# NOTICE: Make sure your glue layer doesn't depend on anything
# which is arch-specific and that it compiles on all situations.
#
# We want to keep this requirement in order to be able to compile
# the entire driver (with all its glue layers) on several architectures
# and make sure it compiles fine. This will also help with allmodconfig
# and allyesconfig builds.
##

# obj-$(CONFIG_USB_DWC3_OMAP)		+= dwc3-omap.o
# obj-$(CONFIG_USB_DWC3_EXYNOS)		+= dwc3-exynos.o
# obj-$(CONFIG_USB_DWC3_PCI)		+= dwc3-pci.o
# obj-$(CONFIG_USB_DWC3_KEYSTONE)		+= dwc3-keystone.o
# obj-$(CONFIG_USB_DWC3_QCOM)		+= dwc3-qcom.o
# obj-$(CONFIG_USB_DWC3_ST)		+= dwc3-st.o

usb-vbus-y  := hisi_usb_vbus.o
ifeq ($(CONFIG_USB_DWC3_VBUS_PLT),y)
	usb-vbus-y    += hisi_usb_vbus_plt.o
endif

ifeq ($(CONFIG_USB_DWC3_VBUS_SPMI),y)
	usb-vbus-y    += hisi_usb_vbus_spmi.o
endif

obj-$(CONFIG_USB_DWC3_KIRIN970)		+= dwc3-kirin970.o hisi_usb3_bc.o hisi_usb3_misctrl.o
obj-$(CONFIG_USB_DWC3_KIRIN960)		+= dwc3-hi3660.o hisi_usb3_bc.o
obj-$(CONFIG_USB_DWC3_KIRIN950)		+= dwc3-hi3650.o hisi_usb3_bc.o
obj-$(CONFIG_USB_DWC3_HISI)		+= dwc3-hisi.o usb-vbus.o

ifneq ($(CFG_CONFIG_HISI_FAMA),true)
obj-$(CONFIG_USB_DWC3_NYET_ABNORMAL)		+= dwc3-hifi-usb.o
endif

ifeq ($(CONFIG_HIFI_MAILBOX), y)
EXTRA_CFLAGS += -I$(srctree)/drivers/hisi/hifi_mailbox/mailbox
EXTRA_CFLAGS += -I$(srctree)/drivers/hisi/hifi_mailbox/ipcm
EXTRA_CFLAGS += -I$(srctree)/drivers/hisi/hifi_dsp
endif
