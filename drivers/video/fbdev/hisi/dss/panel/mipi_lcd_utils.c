/* Copyright (c) 2017-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/
#include "hisi_fb.h"
#include "mipi_lcd_utils.h"
uint32_t g_mipi_lcd_name = 0;


/*lint -save -e527 -e569*/
static int get_jdi_project_id_nt36860c(char *out)
{
	int i = 0;
	int ret = 0;
	struct hisi_fb_data_type *hisifd_primary = NULL;
	uint32_t read_value[12] = {0};
	uint32_t tmp_value = 0;
	char project_id_reg[] = {0x29};

	struct dsi_cmd_desc project_id_cmd[] = {
		{DTYPE_GEN_READ1, 0, 50, WAIT_TYPE_US,
			sizeof(project_id_reg), project_id_reg},
	};

	char page22_reg[] = {0xff, 0x22};
	char mtp_load[] = {0xfb, 0x01};

	struct dsi_cmd_desc page22_cmd[] = {
		{DTYPE_DCS_LWRITE, 0, 50, WAIT_TYPE_US,
			sizeof(page22_reg), page22_reg},
		{DTYPE_DCS_LWRITE, 0, 50, WAIT_TYPE_US,
			sizeof(mtp_load), mtp_load},
	};

	char page10_reg[] = {0xff, 0x10};
	struct dsi_cmd_desc page10_cmd[] = {
		{DTYPE_DCS_LWRITE, 0, 50, WAIT_TYPE_US,
			sizeof(page10_reg), page10_reg},
		{DTYPE_DCS_LWRITE, 0, 50, WAIT_TYPE_US,
			sizeof(mtp_load), mtp_load},
	};

	hisifd_primary = hisifd_list[PRIMARY_PANEL_IDX];

	if ((NULL == hisifd_primary) || (NULL == out)) {
		HISI_FB_ERR("NULL pointer\n");
		return -1;
	}

	mipi_dsi_cmds_tx(page22_cmd, ARRAY_SIZE(page22_cmd), hisifd_primary->mipi_dsi0_base);
	mipi_dsi_lread_reg(read_value, project_id_cmd, 10, hisifd_primary->mipi_dsi0_base);
	mipi_dsi_cmds_tx(page10_cmd, ARRAY_SIZE(page10_cmd), hisifd_primary->mipi_dsi0_base);

	for (i = 0; i < (10 + 3) / 4; i++) {
		HISI_FB_INFO("0x%x\n", read_value[i]);
	}

	for (i = 0; i < 10; i++) {
		switch (i % 4) {
			case 0:
				tmp_value = read_value[i / 4] & 0xFF;
				break;
			case 1:
				tmp_value = (read_value[i / 4] >> 8) & 0xFF;
				break;
			case 2:
				tmp_value = (read_value[i / 4] >> 16) & 0xFF;
				break;
			case 3:
				tmp_value = (read_value[i / 4] >> 24) & 0xFF;
				break;
			default:
				break;
		}

		if (tmp_value == 0) {
			out[i] = tmp_value + '0';
		} else {
			out[i] = tmp_value;
		}
		HISI_FB_INFO("0x%x\n", out[i]);
	}
	return ret;
}

static int get_project_id_nt36870(char *out)
{
	int i = 0;
	int ret = 0;
	struct hisi_fb_data_type *hisifd_primary = NULL;
	uint32_t read_value = 0;
	char project_id_reg[] = {0x58};

	struct dsi_cmd_desc project_id_cmd[] = {
		{DTYPE_DCS_READ, 0, 50, WAIT_TYPE_US,
			sizeof(project_id_reg), project_id_reg},
	};

	char page21_reg[] = {0xff, 0x21};
	char mtp_load[] = {0xfb, 0x01};

	struct dsi_cmd_desc page21_cmd[] = {
		{DTYPE_DCS_LWRITE, 0, 50, WAIT_TYPE_US,
			sizeof(page21_reg), page21_reg},
		{DTYPE_DCS_LWRITE, 0, 50, WAIT_TYPE_US,
			sizeof(mtp_load), mtp_load},
	};

	char page10_reg[] = {0xff, 0x10};
	struct dsi_cmd_desc page10_cmd[] = {
		{DTYPE_DCS_LWRITE, 0, 50, WAIT_TYPE_US,
			sizeof(page10_reg), page10_reg},
		{DTYPE_DCS_LWRITE, 0, 50, WAIT_TYPE_US,
			sizeof(mtp_load), mtp_load},
	};

	hisifd_primary = hisifd_list[PRIMARY_PANEL_IDX];
	if ((NULL == hisifd_primary) || (NULL == out)) {
		HISI_FB_ERR("NULL pointer\n");
		return -1;
	}
	mipi_dsi_cmds_tx(page21_cmd, ARRAY_SIZE(page21_cmd), hisifd_primary->mipi_dsi0_base);
	for (i = 0; i < 10; i++) {
		char *data = project_id_cmd[0].payload;
		*data = 0x58 + i;
		ret = mipi_dsi_cmds_rx(&read_value, project_id_cmd, ARRAY_SIZE(project_id_cmd), hisifd_primary->mipi_dsi0_base);
		if (ret) {
			HISI_FB_ERR("read error\n");
			return -1;
		}
		if (read_value == 0) {
			out[i] = read_value + '0';
		} else {
			out[i] = read_value;
		}
		HISI_FB_INFO("project_id_cmd[0].payload[0] = 0x%x\n", project_id_cmd[0].payload[0]);
		HISI_FB_INFO("read_value = 0x%x\n", read_value);
		HISI_FB_INFO("+++++++out[%d] = 0x%x\n", i, out[i]);
	}
	mipi_dsi_cmds_tx(page10_cmd, ARRAY_SIZE(page10_cmd), hisifd_primary->mipi_dsi0_base);
	return ret;
}

static int get_project_id_on_udp(char *out)
{
	int ret = -1;

	if (NULL == out) {
		return -1;
	}

	if (g_mipi_lcd_name == JDI_2LANE_NT36860C) {
		ret = get_jdi_project_id_nt36860c(out);
	} else if (g_mipi_lcd_name == LG_2LANE_NT36870) {
		ret = get_project_id_nt36870(out);
	} else {
		HISI_FB_ERR("panel is not supported .\n");
	}
	return ret;
}

int hostprocessing_get_project_id_for_udp(char *out)
{
	static bool is_first_access = true;
	int ret = -1;
	struct hisi_fb_data_type *hisifd = hisifd_list[PRIMARY_PANEL_IDX];

	if ((NULL == hisifd) || (NULL == out)) {
		HISI_FB_ERR("NULL pointer\n");
		return -1;
	}

	HISI_FB_DEBUG("platform is not supported .\n");
	return -1;

	if (!is_first_access) {
		HISI_FB_DEBUG("you have accessed before .\n");
		return -1;
	}

	if (g_dss_version_tag == FB_ACCEL_KIRIN970) {
		if (g_fastboot_enable_flag == 1) {
			ret = get_project_id_on_udp(out);
		} else {
			down(&hisifd->blank_sem);
			if (hisifd->panel_power_on) {
				hisifb_activate_vsync(hisifd);
				get_project_id_on_udp(out);
				hisifb_deactivate_vsync(hisifd);
			} else {
				HISI_FB_ERR("panel is off .\n");
			}
			up(&hisifd->blank_sem);
		}
	}
	is_first_access = false;
	return ret;
}
EXPORT_SYMBOL(hostprocessing_get_project_id_for_udp);
/*lint -restore*/

