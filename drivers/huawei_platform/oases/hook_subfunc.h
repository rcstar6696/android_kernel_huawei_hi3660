#ifndef _OASES_HOOK_SUBFUNC_H_
#define _OASES_HOOK_SUBFUNC_H_

#include "patch_base.h"

struct oases_subfunc {
    /* */
	struct oases_patch_base base;
	/* external */
	char *parent;
	char *pmod;
	char *child;
	char *cmod;
	void *filter;
	unsigned long mask;
    /* internal */
	void *start;
	void *end;
	void *sub;
	void *module; /* struct module * for pmod */
	struct oases_insn *insn;
	int count;
};
#define kp_subfunc(x) ((struct oases_subfunc *)((x)->data))

struct oases_subfunc_pre {
	struct oases_subfunc common;
};
#define kp_subfunc_pre(x) ((struct oases_subfunc_pre *)((x)->data))

struct oases_subfunc_post {
    struct oases_subfunc common;
};
#define kp_subfunc_post(x) ((struct oases_subfunc_post *)((x)->data))

#if OASES_ENABLE_REPLACEMENT_HANDLER
struct oases_subfunc_rep {
    struct oases_subfunc common;
};
#define kp_subfunc_rep(x) ((struct oases_subfunc_rep *)((x)->data))
#endif

extern const struct oases_patch_desc oases_subfunc_pre_ops;
extern const struct oases_patch_desc oases_subfunc_post_ops;
#if OASES_ENABLE_REPLACEMENT_HANDLER
extern const struct oases_patch_desc oases_subfunc_rep_ops;
#endif

#endif
