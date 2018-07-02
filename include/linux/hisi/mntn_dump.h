#ifndef __MNTN_DUMP_H
#define __MNTN_DUMP_H
#include <mntn_public_interface.h>

#ifdef CONFIG_HISI_MNTNDUMP
extern int register_mntn_dump(int mod_id, unsigned int size, void **vaddr);
#else
static int register_mntn_dump(int mod_id, unsigned int size, void **vaddr){return -1;}
#endif

#endif
