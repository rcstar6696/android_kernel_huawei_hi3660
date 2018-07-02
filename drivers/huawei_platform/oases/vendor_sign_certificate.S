#include <linux/export.h>

#define GLOBAL(name)	\
	.globl VMLINUX_SYMBOL(name);	\
	VMLINUX_SYMBOL(name):

	.section ".init.data","aw"

GLOBAL(vendor_sign_certificate_list)
	.incbin "vendor_key.x509"
GLOBAL(vendor_sign_certificate_list_end)
