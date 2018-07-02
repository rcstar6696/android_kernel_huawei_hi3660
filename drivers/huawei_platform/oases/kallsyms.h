#ifndef _OASES_KALLSYMS_H_
#define _OASES_KALLSYMS_H_

struct module;

struct oases_find_symbol {
	/* args */
	const char *mod;
	const char *name;
	int (*callback)(void *addr);

	/* ret */
	void *addr;
	unsigned long count;
	void *module;
};

int oases_lookup_name_internal(struct oases_find_symbol *args);

#endif
