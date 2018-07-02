/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
 *
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/kernel.h> 
#include <asm/string.h>
#include "bsp_sram.h"
#include "bsp_shared_ddr.h"
#include "bsp_ddr.h"
#include "dump_file.h"
#include "dump_config.h"
#include "dump_print.h"
#include "dump_modem_area.h"
#include "dump_modem_rdr.h"


/*****************************************************************************
* 函 数 名  : dump_save_modem_bin
* 功能描述  : 保存modem_dump.bin
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_save_mntn_bin(char* dir_name)
{
    struct dump_global_area_ctrl_s global_area = {0,};
    char file_name[MODEM_DUMP_FILE_NAME_LENGTH] = {0,};
    s32 ret;
    DUMP_FILE_CFG_STRU* cfg = dump_get_file_cfg();

    if(cfg->file_list.file_bits.mdm_dump == 1  && (dump_get_product_type()== DUMP_PHONE))
    {
        memset(file_name, 0, sizeof(file_name));
        /*coverity[secure_coding]*/
        snprintf(file_name, sizeof(file_name), "%smodem_dump.bin", dir_name);
        ret = dump_get_global_info(&global_area);
        if(ret == BSP_OK && global_area.virt_addr != NULL)
        {
            dump_save_file(file_name, (u8 *)global_area.virt_addr, global_area.length);
            dump_fetal("[dump]: save %s finished\n", file_name);
        }

    }
}

/*****************************************************************************
* 函 数 名  : dump_save_mdm_sram_file
* 功能描述  : 保存modem的sram
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_save_mdm_sram_file(char* dir_name)
{
    char file_name[MODEM_DUMP_FILE_NAME_LENGTH] = {0,};
    DUMP_FILE_CFG_STRU* cfg = dump_get_file_cfg();

    if((cfg->file_list.file_bits.mdm_sram == 1)&&(EDITION_INTERNAL_BETA == dump_get_edition_type()))
    {
        memset(file_name, 0, sizeof(file_name));
        /*coverity[secure_coding]*/
        snprintf(file_name, sizeof(file_name), "%smodem_sram.bin", dir_name);
        dump_save_file(file_name, (u8 *)g_mem_ctrl.sram_virt_addr, g_mem_ctrl.sram_mem_size);
        dump_fetal("[dump]: save %s finished\n", file_name);
    }
}


/*****************************************************************************
* 函 数 名  : dump_save_mdm_share_file
* 功能描述  : 保存modem的共享内存
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_save_mdm_secshare_file(char* dir_name)
{
    char file_name[MODEM_DUMP_FILE_NAME_LENGTH] = {0,};
    DUMP_FILE_CFG_STRU* cfg = dump_get_file_cfg();
    void* addr = NULL;
    /*lint -save -e835*/
    if(cfg->file_list.file_bits.mdm_secshare == 1  
        && (dump_get_product_type()== DUMP_PHONE) 
        && (EDITION_INTERNAL_BETA == dump_get_edition_type()))
    {
        addr = ioremap_wc((phys_addr_t)(MDDR_FAMA(DDR_SEC_SHARED_ADDR)),(size_t)(DDR_SEC_SHARED_SIZE));
        if(addr == NULL)
        {
            return;
        }
        memset(file_name, 0, sizeof(file_name));
        /*coverity[secure_coding]*/
        snprintf(file_name, sizeof(file_name), "%smodem_secshared.bin", dir_name);
        dump_save_file(file_name, addr, DDR_SEC_SHARED_SIZE);
        dump_fetal("[dump]: save %s finished\n", file_name);
    }
    /*lint -restore +e835*/

}

/*****************************************************************************
* 函 数 名  : dump_save_mdm_ddr_file
* 功能描述  : 保存modem的共享内存
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_save_mdm_share_file(char* dir_name)
{
    char file_name[MODEM_DUMP_FILE_NAME_LENGTH] = {0,};
    DUMP_FILE_CFG_STRU* cfg = dump_get_file_cfg();

    if(cfg->file_list.file_bits.mdm_share == 1  
        && (dump_get_product_type()== DUMP_PHONE) 
        && (EDITION_INTERNAL_BETA == dump_get_edition_type()))
    {
        memset(file_name, 0, sizeof(file_name));
        /*coverity[secure_coding]*/
        snprintf(file_name, sizeof(file_name), "%smodem_share.bin", dir_name);
        dump_save_file(file_name, (u8 *)g_mem_ctrl.sddr_virt_addr, g_mem_ctrl.sddr_mem_size);
        dump_fetal("[dump]: save %s finished\n", file_name);
    }
}


/*****************************************************************************
* 函 数 名  : dump_save_all_mem
* 功能描述  : 保存modem相关的mem
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_save_mem_bin(char* dir_name)
{
    dump_save_mntn_bin(dir_name);
    dump_save_mdm_share_file(dir_name);
    dump_save_mdm_sram_file(dir_name);
    dump_save_mdm_secshare_file(dir_name);

}
