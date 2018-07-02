#ifndef _OASES_PLTS_H_
#define _OASES_PLTS_H_

#include <linux/types.h>

/*
 * LDR X16, <lable>
 * BR  X16
 * <label>
 * u64 target
 */
struct oases_plt_entry {
	u32 ldr;
	u32 br;
	u64 addr;
};

void plts_lock(void *mod);
void plts_unlock(void *mod);
/* plts_*() must be called with plt lock held */
int plts_empty(void *mod);
void *plts_reserve(void *mod);
void *plts_free(void *mod, void *plt);
int plts_purge(void *mod);

int oases_plts_init(void);
void oases_plts_free(void);

#endif
