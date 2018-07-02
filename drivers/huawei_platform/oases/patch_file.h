#ifndef _OASES_PATCH_FILE_H_
#define _OASES_PATCH_FILE_H_

#include <linux/types.h>

#define PATCH_FILE_MAGIC "OASES16\0"
#define PATCH_MAGIC_SIZE 8

#define PATCH_ID_LEN 64
#define PATCH_NAME_LEN 20

#define OASES_SIG_STRING "~OASES signature appended~\n"

#define RELOC_FUNC_NAME_SIZE	64
#define PATCH_MAX_SIZE PAGE_SIZE * 4

struct oases_patch_info;

struct reloc_function_info {
	unsigned int offset;
	char   name[RELOC_FUNC_NAME_SIZE];
};

enum oases_section_type {
	SECTION_RX = 0,
	SECTION_RW,
	SECTION_RO
};

struct oases_section_info {
	unsigned int type;
	unsigned int offset;
	unsigned int size;
	unsigned int page;
};

struct oases_patch_header {
	char magic[PATCH_MAGIC_SIZE];
	unsigned int checksum;

	/* uniq vendor id */
	char id[PATCH_ID_LEN];
	/* name of the vuln */
	char vulnname[PATCH_NAME_LEN];

	unsigned int oases_version;
	unsigned int patch_version;

	unsigned int header_size;
	unsigned int patch_size;
	unsigned int reset_data_count; /* the count of reset datas with (base addres + &offset) */
	unsigned int reset_data_offset;
	unsigned int reloc_func_count; /* relocation api function count */
	unsigned int reloc_func_offset;
	unsigned int section_info_offset;
	unsigned int code_size;
	unsigned int code_offset;
	unsigned int code_entry_offset; /* code entry offset, base is code_body*/

	/* padding */
	unsigned int padding1[1];
	unsigned int padding2[1];
	unsigned int padding3[1];
	unsigned int padding4[1];
};

struct oases_patch_file {
	struct oases_patch_header *pheader;
	unsigned int *redatas; /* unsinged int[reset_data_count] */
	struct reloc_function_info *relfuncs;
	struct oases_section_info *sections;
	char *codes;
	unsigned long len; /* patch length */
};

int oases_init_patch_file(struct oases_patch_file *pfile, void *data);

int oases_layout_patch_file(struct oases_patch_file *pfile, char *data);

int oases_build_code(struct oases_patch_info *info, struct oases_patch_file *pfile);

#endif/* _OASES_PATCH_H */
