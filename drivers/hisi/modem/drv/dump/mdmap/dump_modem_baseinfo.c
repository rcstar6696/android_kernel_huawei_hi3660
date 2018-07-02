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
#include "bsp_dump.h"
#include "bsp_slice.h"
#include "dump_print.h"

dump_base_info_t*    g_mdm_dump_base_info = NULL;


/*****************************************************************************
* 函 数 名  : dump_save_base_info
* 功能描述  : 保存modem ap的基本信息
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_save_base_info(u32 mod_id, u32 arg1, u32 arg2, char *data, u32 length)
{
    struct task_struct *task = NULL;
    if(g_mdm_dump_base_info == NULL)
    {
        return;
    }
    g_mdm_dump_base_info->modId = mod_id;
    g_mdm_dump_base_info->arg1 = arg1;
    g_mdm_dump_base_info->arg2 = arg2;
    g_mdm_dump_base_info->arg3 = (u32)(uintptr_t)data;
    g_mdm_dump_base_info->arg3_length = length;

    if(BSP_MODU_OTHER_CORE == g_mdm_dump_base_info->modId)
    {
        g_mdm_dump_base_info->reboot_task = (u32)(-1);
        g_mdm_dump_base_info->reboot_int = (u32)(-1);
    }
    else
    {
        g_mdm_dump_base_info->reboot_time = bsp_get_slice_value();

        if(in_interrupt())
        {
            g_mdm_dump_base_info->reboot_task = (u32)(-1);
            memset(g_mdm_dump_base_info->taskName, 0, sizeof(g_mdm_dump_base_info->taskName));

            if(DUMP_MBB == dump_get_product_type())
            {
                //g_mdm_dump_base_info->reboot_int = g_mdm_dump_base_info->current_int;
            }
            else
            {
                g_mdm_dump_base_info->reboot_int = 0xAAAABBBB;
            }
            g_mdm_dump_base_info->reboot_context = DUMP_CTX_INT;
        }
        else
        {
            g_mdm_dump_base_info->reboot_task_tcb= (u32)(uintptr_t)current;

            if(g_mdm_dump_base_info->modId == 0x11000025 || g_mdm_dump_base_info->modId == 0x1100002A)
            {
                /* A核VOS只记录的任务的pid*/
                g_mdm_dump_base_info->reboot_task_tcb = g_mdm_dump_base_info->arg1;
                task = find_task_by_vpid(g_mdm_dump_base_info->arg1);
            }
            else
            {
                g_mdm_dump_base_info->reboot_task =  (u32)(((struct task_struct *)(current))->pid);
                task = (struct task_struct *)(current);
            }
            if(task != NULL)
            {
                /*coverity[secure_coding]*/
                memset(g_mdm_dump_base_info->taskName,0,16);
                /*coverity[secure_coding]*/
                memcpy(g_mdm_dump_base_info->taskName,((struct task_struct *)(task))->comm, 16);
                dump_fetal("g_mdm_dump_base_info->taskName = %s\n",g_mdm_dump_base_info->taskName);
            }
            g_mdm_dump_base_info->reboot_int = (u32)(-1);
            g_mdm_dump_base_info->reboot_context = DUMP_CTX_TASK;

        }
    }

    dump_fetal("save base info finish\n");
    return;
}

/*****************************************************************************
* 函 数 名  : dump_save_momdem_reset_baseinfo
* 功能描述  : 更新modem单独复位失败的基本信息
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_save_momdem_reset_baseinfo(u32 modid,char* name)
{
    u32 len = 0;
    g_mdm_dump_base_info->modId = modid;
    g_mdm_dump_base_info->arg1 = 0;
    g_mdm_dump_base_info->arg2 = 0;
    g_mdm_dump_base_info->arg3 = 0;
    g_mdm_dump_base_info->arg3_length = 0;
    g_mdm_dump_base_info->reboot_time = bsp_get_slice_value();

    if(name != NULL)
    {
        len = strlen(name);
        /*coverity[secure_coding]*/
        memcpy(g_mdm_dump_base_info->taskName,name,len > 16 ? 16 : len);
    }
    g_mdm_dump_base_info->reboot_context = DUMP_CTX_TASK;
    g_mdm_dump_base_info->reboot_int= 0xFFFFFFFF;

    dump_fetal("modem reset fail update base info finish modid= 0x%x\n",modid);
}

/*****************************************************************************
* 函 数 名  : dump_base_info_init
* 功能描述  : modem ap基本信息初始化
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
s32 dump_base_info_init(void)
{
    g_mdm_dump_base_info = (dump_base_info_t*)bsp_dump_get_field_addr(DUMP_MODEMAP_BASE_INFO_SMP);
    
    if(g_mdm_dump_base_info != NULL)
    {
        /*coverity[secure_coding]*/
        memset(g_mdm_dump_base_info, 0, sizeof(dump_base_info_t));
        g_mdm_dump_base_info->vec = 0xff;
    }
    else
    {
        return BSP_ERROR;
    }
    
    dump_fetal("dump_base_info_init finish\n");
    return BSP_OK;
}
