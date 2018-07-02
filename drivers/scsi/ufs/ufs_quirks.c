/*
 * Copyright (c) 2013-2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ufshcd.h"
#include "ufs_quirks.h"
#include "dsm_ufs.h"
#ifdef CONFIG_HISI_BOOTDEVICE
#include <linux/bootdevice.h>
#endif
#include <linux/of.h>

#define SERIAL_NUM_SIZE 12
#define BOARDID_SIZE 4

static struct ufs_card_fix ufs_fixups[] = {
	/* UFS cards deviations table */
	END_FIX};

static void get_boardid(unsigned int *boardid)
{
	struct device_node *root = NULL;
	int ret = 0;

	if ((root = of_find_node_by_path("/")) == NULL) {/*lint !e838 */
		printk("Failed : of_find_node_by_path.%pK\n", root);
		goto err_get_bid;
	}

	if ((ret = of_property_read_u32_array(root, "hisi,boardid", boardid, BOARDID_SIZE)) < 0 ) {
		printk("Failed : of_property_read_u32.%d\n", ret);
		goto err_get_bid;
	}
	printk("Board ID.(%x %x %x %x)\n", boardid[0], boardid[1], boardid[2], boardid[3]);
	return;

err_get_bid:
	boardid[0] = 0;
	boardid[1] = 0;
	boardid[2] = 0;
	boardid[3] = 0;
	return;
}

static void ufs_set_sec_unique_number(struct ufs_hba *hba,
				      uint8_t *str_desc_buf,
				      uint8_t *desc_buf,
                                      char *product_name)
{
	int i;
	unsigned int boardid[BOARDID_SIZE] = {0};
	uint8_t snum_buf[SERIAL_NUM_SIZE + 1];

	memset(&hba->unique_number, 0, sizeof(hba->unique_number));
	memset(snum_buf, 0, sizeof(snum_buf));
	get_boardid(boardid);

	switch (hba->manufacturer_id) {
	case UFS_VENDOR_SAMSUNG:
		if (product_name[9] == 'A' && !((boardid[0]==6 && boardid[1]==4 && boardid[2]==5 && boardid[3]==3) /*for board_id == 6453, keep old uid for workaround*/
			|| (boardid[0]==6 && boardid[1]==4 && boardid[2]==5 && boardid[3]==5) /*for board_id == 6455, keep old uid for workaround*/
			|| (boardid[0]==6 && boardid[1]==9 && boardid[2]==0 && boardid[3]==8) /*for board_id == 6908, keep old uid for workaround*/
			|| (boardid[0]==6 && boardid[1]==9 && boardid[2]==1 && boardid[3]==3))) {/*for board_id == 6913, keep old uid for workaround*/
			/* Samsung V4 UFS need 24 Bytes for serial number, transfer unicode to 12 bytes
			 * the magic number 12 here was following original below HYNIX/TOSHIBA decoding method
			*/
			for (i = 0; i < 12; i++) {
				snum_buf[i] =
					str_desc_buf[QUERY_DESC_HDR_SIZE + i * 2 + 1];/*lint !e679*/
			}
		} else {
			/* samsung has 12Byte long of serail number, just copy it */
			memcpy(snum_buf, str_desc_buf + QUERY_DESC_HDR_SIZE, 12);
		}
		break;
	case UFS_VENDOR_HYNIX:
		/* hynix only have 6Byte, add a 0x00 before every byte */
		for (i = 0; i < 6; i++) {
			snum_buf[i * 2] = 0x0;/*lint !e679*/
			snum_buf[i * 2 + 1] = str_desc_buf[QUERY_DESC_HDR_SIZE + i];/*lint !e679*/
		}
		break;
	case UFS_VENDOR_TOSHIBA:
		/*
		 * toshiba: 20Byte, every two byte has a prefix of 0x00, skip
		 * and add two 0x00 to the end
		 */
		for (i = 0; i < 10; i++) {
			snum_buf[i] =
				str_desc_buf[QUERY_DESC_HDR_SIZE + i * 2 + 1];/*lint !e679*/
		}
		snum_buf[10] = 0;
		snum_buf[11] = 0;
		break;
	case UFS_VENDOR_HIVV:
		memcpy(snum_buf, str_desc_buf + QUERY_DESC_HDR_SIZE, 12);
		break;
	case UFS_VENDOR_MICRON:
		memcpy(snum_buf, str_desc_buf + QUERY_DESC_HDR_SIZE, 4);
		for(i = 4; i < 12; i++) {
			snum_buf[i] = 0;
		}
		break;
	default:
		dev_err(hba->dev, "unknown ufs manufacturer id\n");
		break;
	}

	hba->unique_number.manufacturer_id = hba->manufacturer_id;
	hba->unique_number.manufacturer_date = hba->manufacturer_date;
	memcpy(hba->unique_number.serial_number, snum_buf, SERIAL_NUM_SIZE);
}

static int ufs_get_device_info(struct ufs_hba *hba,
			       struct ufs_card_info *card_data)
{
	int err;
	uint8_t model_index;
	uint8_t serial_num_index;
	uint8_t str_desc_buf[QUERY_DESC_STRING_MAX_SIZE + 1];
	uint8_t desc_buf[QUERY_DESC_DEVICE_MAX_SIZE];
	bool ascii_type;

	err = ufshcd_read_device_desc(hba, desc_buf,
				      QUERY_DESC_DEVICE_MAX_SIZE);
	if (err)
		goto out;

	/*
	 * getting vendor (manufacturerID) and Bank Index in big endian
	 * format
	 */
	card_data->wmanufacturerid = desc_buf[DEVICE_DESC_PARAM_MANF_ID] << 8 |
				     desc_buf[DEVICE_DESC_PARAM_MANF_ID + 1];
	card_data->spec_version = desc_buf[DEVICE_DESC_PARAM_SPEC_VER] << 8 |
				  desc_buf[DEVICE_DESC_PARAM_SPEC_VER + 1];
	hba->manufacturer_id = card_data->wmanufacturerid;

	/*
	 * getting (manufacturer DATE) and Bank Index in big endian
	 * format
	 */
	card_data->wmanufacturer_date =
	    desc_buf[DEVICE_DESC_PARAM_MANF_DATE] << 8 |
	    desc_buf[DEVICE_DESC_PARAM_MANF_DATE + 1];

	hba->manufacturer_date = card_data->wmanufacturer_date;
	hba->ufs_device_spec_version = card_data->spec_version;
	/*product name*/
	model_index = desc_buf[DEVICE_DESC_PARAM_PRDCT_NAME];

	memset(str_desc_buf, 0, QUERY_DESC_STRING_MAX_SIZE);
	err = ufshcd_read_string_desc(hba, model_index, str_desc_buf,
				      QUERY_DESC_STRING_MAX_SIZE, ASCII_STD);
	if (err)
		goto out;

	str_desc_buf[QUERY_DESC_STRING_MAX_SIZE] = '\0';
	strlcpy(card_data->model, (char *)(str_desc_buf + QUERY_DESC_HDR_SIZE),
		min_t(uint8_t, str_desc_buf[QUERY_DESC_LENGTH_OFFSET],
		      MAX_MODEL_LEN));
	/* Null terminate the model string */
	card_data->model[MAX_MODEL_LEN] = '\0';

	serial_num_index = desc_buf[DEVICE_DESC_PARAM_SN];
	memset(str_desc_buf, 0, QUERY_DESC_STRING_MAX_SIZE);

	/*spec is unicode but sec use hex data*/
	ascii_type = UTF16_STD;

	err = ufshcd_read_string_desc(hba, serial_num_index, str_desc_buf,
				      QUERY_DESC_STRING_MAX_SIZE, ascii_type);

	if (err)
		goto out;
	str_desc_buf[QUERY_DESC_STRING_MAX_SIZE] = '\0';

	ufs_set_sec_unique_number(hba, str_desc_buf, desc_buf, card_data->model);

#ifdef CONFIG_HISI_BOOTDEVICE
	if (get_bootdevice_type() == BOOT_DEVICE_UFS) {
		u32 cid[4];
		int i;
		memcpy(cid, (u32 *)&hba->unique_number, sizeof(cid));
		for (i = 0; i < 3; i++)
			cid[i] = be32_to_cpu(cid[i]);

		cid[3] = (((cid[3]) & 0xffff) << 16) | (((cid[3]) >> 16) & 0xffff);
		set_bootdevice_cid((u32 *)cid);
		set_bootdevice_product_name(card_data->model, MAX_MODEL_LEN);
		set_bootdevice_manfid(hba->manufacturer_id);
	}
#endif

out:
	return err;
}

void ufs_advertise_fixup_device(struct ufs_hba *hba)
{
	int err;
	struct ufs_card_fix *f;
	struct ufs_card_info card_data;

	card_data.wmanufacturerid = 0;
	card_data.model = kzalloc(MAX_MODEL_LEN + 1, GFP_KERNEL);
	if (!card_data.model)
		goto out;

	/* get device data*/
	err = ufs_get_device_info(hba, &card_data);
	if (err) {
		dev_err(hba->dev, "%s: Failed getting device info\n", __func__);
		goto out;
	}

	for (f = ufs_fixups; f->quirk; f++) {/*lint !e661*/
		/* if same wmanufacturerid */
		if (((f->card.wmanufacturerid == card_data.wmanufacturerid) ||
		     (f->card.wmanufacturerid == UFS_ANY_VENDOR)) &&
		    /* and same model */
		    (STR_PRFX_EQUAL(f->card.model, card_data.model) ||
		     !strncmp(f->card.model, UFS_ANY_MODEL, sizeof(UFS_ANY_MODEL))))
			/* update quirks */
			hba->dev_quirks |= f->quirk;
	}
out:
	kfree(card_data.model);
}

void ufs_get_geometry_info(struct ufs_hba *hba)
{
	int err;
	uint8_t desc_buf[QUERY_DESC_GEOMETRY_MAZ_SIZE];
	u64 total_raw_device_capacity;
#ifdef CONFIG_HISI_BOOTDEVICE
	u8 rpmb_read_write_size = 0;
	u64 rpmb_read_frame_support = 0;
	u64 rpmb_write_frame_support = 0;
#endif
	err =
	    ufshcd_read_geometry_desc(hba, desc_buf, QUERY_DESC_GEOMETRY_MAZ_SIZE);
	if (err) {
		dev_err(hba->dev, "%s: Failed getting geometry info\n", __func__);
		goto out;
	}
	total_raw_device_capacity =
		(u64)desc_buf[GEOMETRY_DESC_TOTAL_DEVICE_CAPACITY + 0] << 56 |
		(u64)desc_buf[GEOMETRY_DESC_TOTAL_DEVICE_CAPACITY + 1] << 48 |
		(u64)desc_buf[GEOMETRY_DESC_TOTAL_DEVICE_CAPACITY + 2] << 40 |
		(u64)desc_buf[GEOMETRY_DESC_TOTAL_DEVICE_CAPACITY + 3] << 32 |
		(u64)desc_buf[GEOMETRY_DESC_TOTAL_DEVICE_CAPACITY + 4] << 24 |
		(u64)desc_buf[GEOMETRY_DESC_TOTAL_DEVICE_CAPACITY + 5] << 16 |
		(u64)desc_buf[GEOMETRY_DESC_TOTAL_DEVICE_CAPACITY + 6] << 8 |
		desc_buf[GEOMETRY_DESC_TOTAL_DEVICE_CAPACITY + 7] << 0;

#ifdef CONFIG_HISI_BOOTDEVICE
	set_bootdevice_size(total_raw_device_capacity);
	rpmb_read_write_size = (u8)desc_buf[GEOMETRY_DESC_RPMB_READ_WRITE_SIZE];
	/*we set rpmb support frame, now max is 64*/
	if(rpmb_read_write_size > MAX_FRAME_BIT){
		dev_err(hba->dev, "%s: rpmb_read_write_size is 0x%x, and large than 64, we set default value is 64\n", __func__,rpmb_read_write_size);
		rpmb_read_write_size = MAX_FRAME_BIT;
	}
	rpmb_read_frame_support = (((uint64_t)1 << (rpmb_read_write_size - 1)) -1) | (((uint64_t)1 << (rpmb_read_write_size - 1)));
	rpmb_write_frame_support = (((uint64_t)1 << (rpmb_read_write_size - 1)) -1) | (((uint64_t)1 << (rpmb_read_write_size - 1)));
	set_rpmb_read_frame_support(rpmb_read_frame_support);
	set_rpmb_write_frame_support(rpmb_write_frame_support);
#endif

out:
	return;
}

void ufs_get_device_health_info(struct ufs_hba *hba)
{
	int err;
	uint8_t desc_buf[QUERY_DESC_HEALTH_MAX_SIZE];
	u8 pre_eol_info;
	u8 life_time_est_typ_a;
	u8 life_time_est_typ_b;

	err =
	    ufshcd_read_device_health_desc(hba, desc_buf, QUERY_DESC_HEALTH_MAX_SIZE);
	if (err) {
		dev_err(hba->dev, "%s: Failed getting device health info\n", __func__);
		goto out;
	}

	pre_eol_info = desc_buf[HEALTH_DEVICE_DESC_PARAM_PREEOL];
	life_time_est_typ_a = desc_buf[HEALTH_DEVICE_DESC_PARAM_LIFETIMEA];
	life_time_est_typ_b = desc_buf[HEALTH_DEVICE_DESC_PARAM_LIFETIMEB];

	if(strstr(saved_command_line, "androidboot.swtype=factory") &&
		(life_time_est_typ_a > 1 || life_time_est_typ_b > 1)){
		dsm_ufs_update_error_info(hba, DSM_UFS_LIFETIME_EXCCED_ERR);
		dev_err(hba->dev, "%s: life_time_est_typ_a = %d, life_time_est_typ_b = %d\n",
		        __func__,
		        life_time_est_typ_a,
		        life_time_est_typ_b);
		dsm_ufs_update_lifetime_info(life_time_est_typ_a, life_time_est_typ_b);
		if (dsm_ufs_enabled())
			schedule_work(&hba->dsm_work);
	}

#ifdef CONFIG_HISI_BOOTDEVICE
	set_bootdevice_pre_eol_info(pre_eol_info);
	set_bootdevice_life_time_est_typ_a(life_time_est_typ_a);
	set_bootdevice_life_time_est_typ_b(life_time_est_typ_b);
#endif

out:
	return;
}
