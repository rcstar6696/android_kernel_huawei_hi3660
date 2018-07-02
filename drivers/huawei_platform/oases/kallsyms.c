/*
 * search symbol in kernel
 *
 * Copyright (C) 2016 Baidu, Inc. All Rights Reserved.
 *
 * You should have received a copy of license along with this program;
 * if not, ask for it from Baidu, Inc.
 *
 */

#include <linux/slab.h>
#include <linux/kallsyms.h>
#include <linux/module.h>
#include <asm/sections.h>
#include "kallsyms.h"
#include "util.h"

extern const unsigned long kallsyms_addresses[] __attribute__((weak));
extern const u8 kallsyms_names[] __attribute__((weak));
extern const unsigned long kallsyms_num_syms __attribute__((weak, section(".rodata")));
extern const u8 kallsyms_token_table[] __attribute__((weak));
extern const u16 kallsyms_token_index[] __attribute__((weak));
extern const unsigned long kallsyms_markers[] __attribute__((weak));

/*
 * This is taken from kallsyms_expand_symbol mostly
 * return 0 means match
 */
static int kallsyms_compare_symbol(unsigned int *off, const char *name, int nlen)
{
    int diff = -1;
    int len, skipped_first = 0;
	const u8 *tptr, *data;

	/* Get the compressed symbol length from the first symbol byte. */
	data = &kallsyms_names[*off];
	len = *data;
	data++;

    /*
	 * Update the offset to return the offset for the next symbol on
	 * the compressed stream.
	 */
	*off += len + 1;

	/*
	 * For every byte on the compressed symbol data, copy the table
	 * entry for that byte.
	 */
	while (len) {
		tptr = &kallsyms_token_table[kallsyms_token_index[*data]];
		data++;
		len--;

		while (*tptr) {
			if (skipped_first) {
				diff = *tptr - *name;
                if (diff)
                    return diff;
                name++;
				nlen--;
                if (!nlen)
                    break;
			} else
				skipped_first = 1;
			tptr++;
		}
        if (!nlen)
            break;
	}

	return diff;
}

static unsigned int kallsyms_expand_symbol(unsigned int off,
					   char *result, size_t maxlen)
{
	int len, skipped_first = 0;
	const u8 *tptr, *data;

	/* Get the compressed symbol length from the first symbol byte. */
	data = &kallsyms_names[off];
	len = *data;
	data++;

	/*
	 * Update the offset to return the offset for the next symbol on
	 * the compressed stream.
	 */
	off += len + 1;

	/*
	 * For every byte on the compressed symbol data, copy the table
	 * entry for that byte.
	 */
	while (len) {
		tptr = &kallsyms_token_table[kallsyms_token_index[*data]];
		data++;
		len--;

		while (*tptr) {
			if (skipped_first) {
				if (maxlen <= 1)
					goto tail;
				*result = *tptr;
				result++;
				maxlen--;
			} else
				skipped_first = 1;
			tptr++;
		}
	}

tail:
	if (maxlen)
		*result = '\0';

	/* Return to offset to the next symbol. */
	return off;
}


static int oases_kallsyms_on_each_symbol(
    int (*fn)(void *, const char *, struct module *, unsigned long),
	struct oases_find_symbol *data)
{
    int ret, i;
	char namebuf[KSYM_NAME_LEN];
    int nlen;
    unsigned int off = 0, tmp;

    if (!data->name)
        return 0;
    nlen = strlen(data->name);
    if (!nlen)
        return 0;
	for (i = 0; i < kallsyms_num_syms; i++) {
        tmp = off;
        ret = kallsyms_compare_symbol(&off, data->name, nlen);
        /* only expand when prefix is matched */
        if (ret == 0) {
            kallsyms_expand_symbol(tmp, namebuf, ARRAY_SIZE(namebuf));
    		ret = fn(data, namebuf, NULL, kallsyms_addresses[i]);
    		if (ret != 0)
    			return ret;
        }
	}
	return module_kallsyms_on_each_symbol(fn, data);
}

/*
 * Return 1 if found a match symbol, otherwise 0 to continue search
 * Note we search until all symbols checked, trying to solve duplicate
 * symbol problem.
 */
static int lookup_name_callback(void *data, const char *name,
			struct module *mod, unsigned long addr)
{
	struct oases_find_symbol *s = data;

	if (strcmp(s->name, name))
		return 0;

	if (s->callback && !(*s->callback)((void *)addr)) {
		return 0;
	}

	if (s->mod && !mod)
		return 0;

#ifdef CONFIG_MODULES
	/* call module_kallsyms_on_each_symbol when s->mod is true */
	if (s->mod && strcmp(s->mod, mod->name))
		return 0;
#endif

	s->addr = (void *)addr;
	s->count++;
	s->module = mod;

	if (s->count > 1) {
		return 1;
	}

	return 0;
}

/*
 * Return 0 if success otherwise error code.
 *
 * further check on symbol size, attributes, T/t/W/w/D/d/A...
 * e.g. 000000000000a2a0 A mc_buffer
 */
int oases_lookup_name_internal(struct oases_find_symbol *args)
{
	if (args->mod)
		module_kallsyms_on_each_symbol(lookup_name_callback, args);
	else
		oases_kallsyms_on_each_symbol(lookup_name_callback, args);

	if (!args->addr || args->count != 1)
		return -EINVAL;

	return 0;
}
