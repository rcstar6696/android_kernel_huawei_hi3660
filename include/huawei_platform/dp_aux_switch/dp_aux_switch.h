/* Copyright (c) 2008-2019, Huawei Tech. Co., Ltd. All rights reserved.
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
#ifndef __DP_AUX_SWITCH_H__
#define __DP_AUX_SWITCH_H__

typedef enum dp_aux_ldo_ctrl_type {
	DP_AUX_LDO_CTRL_BEGIN = 0,
	DP_AUX_LDO_CTRL_USB = DP_AUX_LDO_CTRL_BEGIN,
	DP_AUX_LDO_CTRL_COMBOPHY,
	DP_AUX_LDO_CTRL_DIRECT_CHARGE,
	DP_AUX_LDO_CTRL_HIFIUSB,
	DP_AUX_LDO_CTRL_TYPECPD,

	DP_AUX_LDO_CTRL_MAX,
} dp_aux_ldo_ctrl_type_t;

#ifdef CONFIG_DP_AUX_SWITCH
void dp_aux_switch_op(uint32_t value);
void dp_aux_uart_switch_enable(void);
void dp_aux_uart_switch_disable(void);
int dp_aux_ldo_supply_enable(dp_aux_ldo_ctrl_type_t type);
int dp_aux_ldo_supply_disable(dp_aux_ldo_ctrl_type_t type);
#else
static inline void dp_aux_switch_op(uint32_t value) {}
static inline void dp_aux_uart_switch_enable(void) {}
static inline void dp_aux_uart_switch_disable(void) {}
static inline int dp_aux_ldo_supply_enable(dp_aux_ldo_ctrl_type_t type) { return 0; }
static inline int dp_aux_ldo_supply_disable(dp_aux_ldo_ctrl_type_t type) { return 0; }
#endif

#endif // __DP_AUX_SWITCH_H__

