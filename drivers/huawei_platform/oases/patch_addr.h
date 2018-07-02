#ifndef _OASES_PATCH_ADDR_H_
#define _OASES_PATCH_ADDR_H_

#include <linux/types.h>

struct oases_patch_addr {
	void *data;
    int i_key;
	void **addrs_key;
	u32 *old_insns_key;
	u32 *new_insns_key;
    int i_plt;
	void **addrs_plt;
	u32 *new_insns_plt;
};

static inline void oases_patch_addr_add_key(struct oases_patch_addr *addr,
    void *address, u32 insn)
{
    addr->addrs_key[addr->i_key] = address;
    addr->old_insns_key[addr->i_key] = *((u32 *)address);
    addr->new_insns_key[addr->i_key] = insn;
    addr->i_key++;
}

static inline void oases_patch_addr_add_plt(struct oases_patch_addr *addr,
    void *plt, void *trampoline)
{
    /*
     * ldr x16, trampoline_addr
     * br x16
     * trampline_addr:
     *   .quad trampoline
     */
    addr->addrs_plt[addr->i_plt] = plt;
    addr->new_insns_plt[addr->i_plt] = (u32) 0x58000050;
    addr->i_plt++;
    addr->addrs_plt[addr->i_plt] = plt + 4;
    addr->new_insns_plt[addr->i_plt] = (u32) 0xd61f0200;
    addr->i_plt++;
    addr->addrs_plt[addr->i_plt] = plt + 8;
    addr->new_insns_plt[addr->i_plt] = ((u64)trampoline);
    addr->i_plt++;
    addr->addrs_plt[addr->i_plt] = plt + 12;
    addr->new_insns_plt[addr->i_plt] = ((u64)trampoline) >> 32;
    addr->i_plt++;
}

int oases_patch_addr_init(struct oases_patch_addr *addr, int count);
void oases_patch_addr_free(struct oases_patch_addr *addr);

#endif
