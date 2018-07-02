#ifndef _OASES_INLINEHOOK_H
#define _OASES_INLINEHOOK_H

#include <linux/types.h>

/* fixed values for now */
#define TRAMPOLINE_THUNK_OFF 	80 * 4 /* must enough for trampoline */
#define TRAMPOLINE_DATA_OFF 	100 * 4
#define TRAMPOLINE_LABEL_OFF	120 * 4
#define TRAMPOLINE_BUF_SIZE		160 * 4

struct oases_insn;

static inline int is_insn_b(u32 addr)
{
	if ((addr & (u32) 0xFC000000UL) == (u32) 0x14000000UL)
		return 1;
	return 0;
}

static inline int is_insn_bl(u32 insn)
{
	if ((insn & (u32) 0xFC000000UL) == (u32) 0x94000000UL)
		return 1;
	return 0;
}

static inline int is_bl_to(void *loc, void *target)
{
	int ret = 0;
	u64 offset;
	u32 insn;

	insn = *((u32 *)loc);
	offset = (insn & (u32) 0x3FFFFFFUL) << 2; /* 28 bits */
	if (offset & 0x8000000UL) {
		offset += 0xFFFFFFFFF0000000UL;
	}

	offset += (u64)loc;
	if (offset == (u64)target)
		ret = 1;

	return ret;
}

int oases_make_jump_insn(u32 *addr, u32 *dst, u32 *insn);
int oases_relocate_insn(struct oases_insn *oases_insn, int off);

#endif /* _OASES_INLINEHOOK_H */
