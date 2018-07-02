#
#	OASES Project to patch android kernel
#
#	Copyright (C) 2016 Baidu, Inc.
#

OASES_TOP := $(srctree)/$(src)
OASES_OUT := $(obj)

OASES_INLINEHOOK_AUTOGEN := \
	inlinehook_func_pre.bin \
	inlinehook_func_post.bin \
	inlinehook_subfunc_pre.bin \
	inlinehook_subfunc_post.bin

OASES_INLINEHOOK_AUTOGEN := $(addprefix $(OASES_OUT)/, $(OASES_INLINEHOOK_AUTOGEN))

$(obj)/inlinehook_data.o: $(OASES_INLINEHOOK_AUTOGEN)

$(OASES_OUT)/%.bin: $(OASES_OUT)/%.oas
	$(OBJCOPY) -O binary $^ $@

$(OASES_OUT)/%.oas: $(OASES_TOP)/%.S
	$(CC) -c -o $@ $^

$(OASES_OUT)/inlinehook_offset.h: $(OASES_INLINEHOOK_AUTOGEN)
	$(OASES_TOP)/inlinehook_gen_off $@ $(OASES_TOP)/inlinehook_const.h $^

$(obj)/hook_func.o: $(OASES_OUT)/inlinehook_offset.h
$(obj)/hook_subfunc.o: $(OASES_OUT)/inlinehook_offset.h

obj-$(CONFIG_OASES) += util.o vmarea.o plts.o kallsyms.o \
	inlinehook.o inlinehook_data.o \
	patch_addr.o \
	patch_base.o \
	hook_insn.o hook_func.o hook_subfunc.o \
	sysfs.o \
	patch_mgr.o \
	patch_api.o \
	patch_file.o \
	oases.o

obj-$(CONFIG_OASES_SIGNATURE_SUPPORT) += oases_sign_certificate.o \
	oases_signing.o \
	vendor_sign_certificate.o

$(obj)/oases_sign_certificate.o: $(OASES_TOP)/oases_key.x509
$(obj)/vendor_sign_certificate.o: $(OASES_TOP)/vendor_key.x509

obj-$(CONFIG_OASES_STATIC_PLTS) += plts_static.o


ccflags-y += -DOASES_ENABLE_REPLACEMENT_HANDLER=0
ccflags-y += -DDEBUG
ccflags-y += -I$(OASES_OUT)
asflags-y += -I$(OASES_OUT)

clean-files += $(OASES_INLINEHOOK_AUTOGEN) \
	$(OASES_OUT)/inlinehook_offset.h
