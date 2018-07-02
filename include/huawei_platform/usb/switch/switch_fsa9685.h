/************************************************************
*
* Copyright (C), 1988-1999, Huawei Tech. Co., Ltd.
* FileName: switch_fsa9685.h
* Author: huxiaoqiang(00272253)       Version : 0.1      Date:  2013-11-07
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*  Description:    .h file for switch fsa9685
*  Version:
*  Function List:
*  History:
*  <author>  <time>   <version >   <desc>
***********************************************************/
#include <linux/hisi/usb/hisi_usb.h>
#include <huawei_platform/usb/switch/switch_fsa9685_common.h>

#define ADAPTOR_BC12_TYPE_MAX_CHECK_TIME 100
#define WAIT_FOR_BC12_DELAY 5
#define ACCP_NOT_PREPARE_OK -1
#define ACCP_PREPARE_OK 0
#define BOOST_5V_CLOSE_FAIL -1
#define SET_DCDTOUT_FAIL -1
#define SET_DCDTOUT_SUCC 0


int fcp_read_switch_status (void);
int fcp_read_adapter_status(void);
void switch_dump_register(void);
int is_fcp_charger_type(void);
