/*
 * patch_file.c - transform patch to code
 *
 * Copyright (C) 2016 Baidu, Inc. All Rights Reserved.
 *
 * You should have received a copy of license along with this program;
 * if not, ask for it from Baidu, Inc.
 *
 */

#if IS_ENABLED(CONFIG_MODULES)
#include <linux/moduleloader.h>
#endif

#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/crc32.h>
#include <asm/cacheflush.h>

#include "util.h"
#include "patch_api.h"
#include "patch_file.h"
#include "patch_info.h"
#include "patch_mgr.h"
#include "oases_signing.h"
#include "plts.h"
#include "vmarea.h"

struct func_relocation_item {
	char* name;		 /* relocation function name */
	unsigned long addr; /* relocation function addr */
};

#define funcptr_2_ul(func) ((unsigned long)(&(func)))
#define FUNC_RELOCATION_ITEM(s) {#s, funcptr_2_ul(s)}

/* api function from patch_api.h */
static const struct func_relocation_item api_fri_list[] = {
	FUNC_RELOCATION_ITEM(oases_lookup_name),
	FUNC_RELOCATION_ITEM(oases_register_patch),
	FUNC_RELOCATION_ITEM(oases_printk),
	FUNC_RELOCATION_ITEM(oases_memset),
	FUNC_RELOCATION_ITEM(oases_memcpy),
	FUNC_RELOCATION_ITEM(oases_memcmp),
	FUNC_RELOCATION_ITEM(oases_copy_from_user),
	FUNC_RELOCATION_ITEM(oases_copy_to_user),
	FUNC_RELOCATION_ITEM(oases_get_user_u8),
	FUNC_RELOCATION_ITEM(oases_put_user_u8),
	FUNC_RELOCATION_ITEM(oases_get_user_u16),
	FUNC_RELOCATION_ITEM(oases_put_user_u16),
	FUNC_RELOCATION_ITEM(oases_get_user_u32),
	FUNC_RELOCATION_ITEM(oases_put_user_u32),
	FUNC_RELOCATION_ITEM(oases_get_user_u64),
	FUNC_RELOCATION_ITEM(oases_put_user_u64),
	FUNC_RELOCATION_ITEM(oases_get_current),
	FUNC_RELOCATION_ITEM(oases_linux_version_code),
};

static void fix_oases_plt(unsigned long symbol, void* plt_entry)
{
	struct oases_plt_entry entry = {
		.ldr = 0x58000050UL, /* LDR X16, loc_addr */
		.br = 0xd61f0200UL, /* BR X16 */
		.addr = symbol /* lable: loc_addr */
	};
	memcpy(plt_entry, &entry, sizeof(entry));
}

static unsigned long get_symbol_addr(const char *name)
{
	unsigned int i;
	size_t listsize = ARRAY_SIZE(api_fri_list);
	for (i = 0; i < listsize; i++) {
		if (!strncmp(name, api_fri_list[i].name, RELOC_FUNC_NAME_SIZE)) {
			return api_fri_list[i].addr;
		}
	}

	return 0;
}

static int reloc_api_function(void *code_base, const char *name, unsigned int offset)
{
	unsigned long symbol;

	symbol = get_symbol_addr(name);
	if (!symbol) {
		oases_debug("symbol:%s, offset: %x find addr fail\n", name, offset);
		return -EINVAL;
	}

	fix_oases_plt(symbol, code_base + offset);
	return 0;
}

static void reset_data(void *data, unsigned long value)
{
	unsigned long tmp;
	tmp = *((unsigned long *)data);
	tmp += value;
	memcpy(data, &tmp, sizeof(unsigned long));
}

static int oases_patch_sig_check(struct oases_patch_file *patch,  char *data)
{
	const unsigned long markerlen = sizeof(OASES_SIG_STRING) - 1;
	int ret;
	if (patch->len < markerlen
		|| memcmp(data + patch->len - markerlen, OASES_SIG_STRING, markerlen) != 0) {
		oases_error("system sig marker check fail\n");
		return -ENOKEY;
	}
	patch->len -= markerlen;
	oases_error("verifying system sig\n");
	ret = oases_verify_sig(data, &patch->len, SIG_TYPE_SYSTEM);
	if (ret < 0) {
		oases_error("system sig verify fail, ret=%d\n", ret);
		return ret;
	}

	if (patch->len > markerlen
		&& !memcmp(data + patch->len - markerlen, OASES_SIG_STRING, markerlen)) {
		patch->len -= markerlen;
		oases_error("verifying oases sig\n");
		ret = oases_verify_sig(data, &patch->len, SIG_TYPE_OASES);
		if (ret < 0) {
			oases_error("oases sig verify fail, ret=%d\n", ret);
			return ret;
		}
	}

	return ret;
}

static int verify_patch_checksum(struct oases_patch_header *pheader, char *data, unsigned long size)
{
	unsigned int checksum;
	unsigned int offset = PATCH_MAGIC_SIZE + sizeof(pheader->checksum);

	checksum = crc32(0, data + offset, size - offset);
	if (checksum != pheader->checksum)
		return -ENOEXEC;

	return 0;
}

static int valid_section_info(struct oases_patch_header *pheader, char *data)
{
	int i;
	struct oases_section_info *info = (struct oases_section_info *)(data + pheader->section_info_offset);
	for (i = 0; i <= SECTION_RO; i++) {
		if (info[i].type != i || info[i].offset > pheader->code_size
			|| info[i].size > info[i].page * PAGE_SIZE)
			return -EINVAL;
	}
	return 0;
}

static int verify_patch_header(struct oases_patch_header *pheader, unsigned long size)
{
	int ssize;

	if (pheader->header_size != sizeof(struct oases_patch_header))
		return -ENOEXEC;

	if (pheader->patch_size == 0 || pheader->code_size == 0 || pheader->patch_size != size
		|| pheader->patch_size < pheader->header_size + pheader->code_size)
		return -ENOEXEC;

	/* reset_data_offset and reset_data_count */
	if (pheader->reset_data_offset != pheader->header_size
		|| pheader->reset_data_offset >= size) {
		oases_debug("reset_data_offset error\n");
		return -ENOEXEC;
	}

	ssize = pheader->reloc_func_offset - pheader->reset_data_offset;
	if (ssize < 0 || pheader->reloc_func_offset >= size
		|| (pheader->reset_data_count != (ssize / sizeof(unsigned int)))) {
		oases_debug("reset_data_count error\n");
		return -ENOEXEC;
	}

	/* reloc_func_offset and reloc_func_count */
	ssize = pheader->section_info_offset - pheader->reloc_func_offset;
	if (ssize < 0
		|| (pheader->reloc_func_count != (ssize / sizeof(struct reloc_function_info)))) {
		oases_debug("reloc_func_offset or reloc_func_count error\n");
		return -ENOEXEC;
	}

	ssize = pheader->code_offset - pheader->section_info_offset;
	if (ssize < 0 ||
		((ssize / sizeof(struct oases_section_info)) != SECTION_RO + 1)) {
		oases_debug("oases_section_info or reloc_func_count error\n");
		return -ENOEXEC;
	}

	/* code_offset and code_entry_offset */
	if (pheader->code_offset >= size
		|| pheader->code_offset + pheader->code_size != pheader->patch_size
		|| pheader->code_entry_offset > (pheader->code_size - sizeof(unsigned int))) {
		oases_debug("code_offset error\n");
		return -ENOEXEC;
	}

	return 0;
}

static int oases_layout_patch(struct oases_patch_file *pfile, char *data)
{
	int ret = 0, i;
	struct oases_patch_header *pheader = (struct oases_patch_header *)data;
	unsigned long code_size = 0;
	unsigned int *redatas;
	struct reloc_function_info *relfuncs;
	struct oases_section_info *sections;

	if (strncmp(pheader->magic, PATCH_FILE_MAGIC, PATCH_MAGIC_SIZE))
		return -EINVAL;

	ret = verify_patch_checksum(pheader, data, pfile->len);
	if (ret < 0)
		return ret;

	ret = verify_patch_header(pheader, pfile->len);
	if (ret < 0)
		return ret;

	ret = oases_valid_name(pheader->id, PATCH_ID_LEN - 1);
	if (ret < 0) {
		oases_debug("invalid id: %s\n", pheader->id);
		return ret;
	}

	/* check section info values */
	if (valid_section_info(pheader, data) < 0) {
		oases_debug("valid_section_info error\n");
		return -EINVAL;
	}

	sections = (struct oases_section_info *)(data + pheader->section_info_offset);
	for (i= 0; i <= SECTION_RO; i++)
		code_size += sections[i].page * PAGE_SIZE;
	code_size -= sizeof(unsigned long);

	/* check reset data offset */
	redatas = (unsigned int *)(data + pheader->reset_data_offset);
	for (i = 0; i < pheader->reset_data_count; i++)
		if (redatas[i] > code_size)
			return -EINVAL;
	/* check reloc function info */
	relfuncs = (struct reloc_function_info *)(data + pheader->reloc_func_offset);
	for (i = 0; i < pheader->reloc_func_count; i++)
		if (relfuncs[i].offset > code_size || strlen(relfuncs[i].name) == 0)
			return -EINVAL;

	pfile->pheader = pheader;
	pfile->redatas = redatas;
	pfile->relfuncs = relfuncs;
	pfile->sections = sections;
	pfile->codes = data + pheader->code_offset;
	return 0;
}

int oases_build_code(struct oases_patch_info *info, struct oases_patch_file *pfile)
{
	int i, ret;
	unsigned int cnt, code_size = 0;
	struct oases_patch_header *pheader = pfile->pheader;
	struct oases_section_info *sections = pfile->sections;
	struct oases_section_info section;
	struct reloc_function_info *relocinfo;
	void *code_base, *tmp;

	for (i= 0; i <= SECTION_RO; i++)
		code_size += sections[i].page * PAGE_SIZE;

	code_base = vmalloc_exec(code_size);
	if (!code_base) {
		return -ENOMEM;
	}
	memset(code_base, 0, code_size);
	/* copy sections */
	tmp = code_base;
	for (i= 0; i <= SECTION_RO; i++) {
		section = sections[i];
		if (section.size > 0) {
			memcpy(tmp, pfile->codes + section.offset, section.size);
			tmp += section.page * PAGE_SIZE;
		}
	}

	cnt = pheader->reset_data_count;
	for ( i = 0; i < cnt; i++) {
		reset_data(code_base + pfile->redatas[i], (unsigned long)code_base);
	}

	cnt = pheader->reloc_func_count;
	for ( i = 0; i < cnt; i++) {
		relocinfo = &pfile->relfuncs[i];
		ret = reloc_api_function(code_base, relocinfo->name, relocinfo->offset);
		if (ret < 0)
			goto fail;
	}

	/* set page attr , section layout
		+++++++++++++++
		+  RX  | RW  | RO +
		+++++++++++++++
	*/
	tmp = code_base;
	for (i = 0; i <= SECTION_RO; i++) {
		section = sections[i];
		if (section.size > 0) {
			/* already RWX */
			switch(section.type) {
			case SECTION_RX:
#ifdef CONFIG_HISI_HHEE
				/* kill W for RX */
				ret = oases_set_vmarea_ro((unsigned long)tmp, section.page);
				oases_debug("setting rx, ret=%d\n", ret);

				oases_debug("is_hkip_enabled=%d\n", is_hkip_enabled());
				if (is_hkip_enabled()) {
					oases_debug("tmp=%pK, section.page=%lu\n", tmp, section.page);
					aarch64_insn_disable_pxn_hkip(tmp, section.page * PAGE_SIZE);
				}
#else
				/* kill W for RX */
				oases_set_vmarea_ro((unsigned long)tmp, section.page);
#endif

				tmp += section.page * PAGE_SIZE;
				break;
			case SECTION_RW:
				/* kill X for RW */
				oases_set_vmarea_nx((unsigned long)tmp, section.page);
				tmp += section.page * PAGE_SIZE;
				break;
			case SECTION_RO:
				/* kill WX for RO */
				oases_set_vmarea_ro((unsigned long)tmp, section.page);
				oases_set_vmarea_nx((unsigned long)tmp, section.page);
				tmp += section.page * PAGE_SIZE;
				break;
			default:
				continue;
			}
		}
	}

	flush_icache_range((unsigned long) code_base, (unsigned long)(code_base + code_size));
	info->code_base = code_base;
	info->code_entry = code_base + pheader->code_entry_offset;
	info->code_size = code_size;
	return 0;
fail:
	vfree(code_base);
	return ret;
}

int oases_init_patch_file(struct oases_patch_file *pfile, void *data)
{
	int ret;
	struct oases_patch_header *pheader;

	ret = oases_patch_sig_check(pfile, data);
	if (ret < 0)
		return ret;

	ret = oases_layout_patch(pfile, data);
	if (ret < 0)
		return ret;

	pheader = pfile->pheader;
	if (pheader->oases_version > OASES_VERSION) {
		oases_error("pheader->oases_version(%d) > OASES_VERSION(%d)\n",
				pheader->oases_version, OASES_VERSION);
		return -EINVAL;
	}

	ret = oases_check_patch(pheader->id);
	if (ret < 0)
		return ret;

	return 0;
}
