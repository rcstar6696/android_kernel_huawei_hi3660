#include <linux/export.h>

#define GLOBAL(name)	\
	.globl VMLINUX_SYMBOL(name);	\
	VMLINUX_SYMBOL(name):

	.section ".init.data","aw"

GLOBAL(oases_sign_certificate_list)
	.incbin "oases_key.x509"
GLOBAL(oases_sign_certificate_list_end)
