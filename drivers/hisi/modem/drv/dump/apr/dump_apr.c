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


#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/thread_info.h>
#include <linux/syslog.h>
#include <linux/errno.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/notifier.h>
#include <linux/kdebug.h>
#include <linux/reboot.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/wakelock.h>
#include <asm/string.h>
#include <asm/traps.h>
#include "product_config.h"
#include <linux/syscalls.h>
#include "osl_types.h"
#include "osl_io.h"
#include "osl_bio.h"
#include "osl_malloc.h"
#include "bsp_dump.h"
#include "bsp_nvim.h"
#include "bsp_dump_mem.h"
#include "drv_nv_def.h"
#include "mdrv_om.h"
#include "drv_comm.h"
#include <gunas_errno.h>
#include "dump_modem_area.h"
#include "dump_apr.h"
#include "dump_print.h"
#include "dump_file.h"
#include "dump_config.h"
#include "dump_exc_ctrl.h"



/*存储cp中断信息 中断号 责任组件*/
interupt_table_t *g_cp_interupt_table = NULL;

/*存储cp任务 责任组件信息*/
task_table_t *g_cp_task_table = NULL;
/*存储modid范围 责任组件信息*/
modid_info_t *g_p_modid_table = NULL;

/*****************************************************************************
* 函 数 名  : bsp_dump_parse_apr_dts_info
*
* 功能描述  : 读取dts初始化任务信息和中断信息 组件信息
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
s32 dump_apr_init(void)
{
    struct device_node *dev_node;
    struct device_node *child ;
    struct property *prop = NULL;
    const __be32 *p = NULL;
    char * module_name = NULL;
    u32 sum = 0;
    unsigned long alloc_size;
    u32 index = 0;
    u32 module_index = 0;
    void * temp = NULL;
    u32 modid_sum = 0;
    u32 task_sum = 0;
    u32 int_sum = 0;
    u32 u = 0;
    u32 i = 0;

    if(dump_get_product_type() == DUMP_MBB)
    {
        return BSP_OK;
    }
    /*读取责任组件信息*/
    dev_node = of_find_compatible_node(NULL,NULL,"hisilicon,apr_config");
    if(!dev_node)
    {
        dump_fetal("get apr_config node failed!\n");
        return BSP_ERROR;
    }

    for_each_child_of_node(dev_node,child)
    {
        if(of_property_read_u32(child, "modid_sum", &sum))
        {
            sum = 0;
            dump_fetal("get modid_sum failed!\n");
        }
        modid_sum+=sum;
        if(of_property_read_u32(child, "task_sum", &sum))
        {
            sum = 0;
            dump_fetal("get task_sum failed!\n");
        }
        task_sum+=sum;
        if(of_property_read_u32(child, "int_sum", &sum))
        {
            sum = 0;
            dump_fetal("get int_sum failed!\n");
        }
        int_sum+=sum;

    }

    alloc_size = (modid_sum*(sizeof(modid_info_t) - sizeof(u32)) + sizeof(u32));
    g_p_modid_table = (modid_info_t *)(uintptr_t)osl_malloc((u32)alloc_size);
    if(NULL == g_p_modid_table)
    {
        dump_fetal("p_module_info malloc fail, size:0x%x\n", alloc_size);
    }
    memset(g_p_modid_table, 0, alloc_size);
    alloc_size = int_sum*(sizeof(interupt_table_t) - sizeof(u32)) + sizeof(u32);
    g_cp_interupt_table = (interupt_table_t *)(uintptr_t)osl_malloc((u32)alloc_size);
    if(NULL == g_cp_interupt_table)
    {
        dump_fetal("g_cp_interupt_table malloc fail, size:0x%x\n", alloc_size);
    }
    memset(g_cp_interupt_table, 0, alloc_size);

    alloc_size = task_sum*(sizeof(task_table_t) - sizeof(u32)) + sizeof(u32);
    g_cp_task_table = (task_table_t *)(uintptr_t)osl_malloc((u32)alloc_size);
    if(NULL == g_cp_task_table)
    {
        dump_fetal("g_cp_task_table malloc fail, size:0x%x\n", alloc_size);
    }
    memset(g_cp_task_table, 0, alloc_size);

    for_each_child_of_node(dev_node,child)
    {
        /*如果本次读取组件名称失败则不再读取其他信息*/
        if(of_property_read_string(child, (const char*)"module_name", (const char**)&module_name))
        {
            dump_fetal("get module_name fail\n");
            continue;
        }
        /*读取modid信息*/
        if(of_property_read_u32(child, "modid_sum", &sum))
        {
            sum = 0;
            dump_fetal("get modid_sum failed!\n");
        }

        of_property_for_each_u32(child, "modid_range", prop , p, u)
        {
            module_index = g_p_modid_table->moudid_sum;
            if(i%2 == 0)
            {
                g_p_modid_table->modid_info[module_index].modid_start = u;
            }
            if(i%2 == 1)
            {
                g_p_modid_table->modid_info[module_index].modid_end = u;
                /*coverity[secure_coding]*/
                snprintf(g_p_modid_table->modid_info[module_index].module,(unsigned long)DUMP_MODULE_NAME_LEN, "%s", module_name);
                g_p_modid_table->moudid_sum++;
            }
            i++;
        }
        /*读取task信息*/
        if(of_property_read_u32(child, "task_sum", &sum))
        {
            sum = 0;
            dump_fetal("get task_sum failed!\n");
        }
        for(index = 0; (index < sum)&&(g_cp_task_table); index++)
        {
            module_index = g_cp_task_table->task_sum;
            if(of_property_read_string_index(child, (const char*)"task_name", (int)index,(const char**)&temp))
            {
                dump_fetal("get task_name fail\n");
            }
            else
            {
                /*coverity[secure_coding]*/
                snprintf(g_cp_task_table->task_info[module_index].task_name, (unsigned long)16, "%s",(char *)temp);
                /*coverity[secure_coding]*/
                snprintf(g_cp_task_table->task_info[module_index].module,(unsigned long)DUMP_MODULE_NAME_LEN, "%s", module_name);
                g_cp_task_table->task_sum++;
            }
        }
        /*读取中断信息*/
        if(of_property_read_u32(child, "int_sum", &sum))
        {
            sum = 0;
            dump_fetal("get int_sum failed!\n");
        }
        for(index = 0; (index < sum)&&(g_cp_interupt_table); index++)
        {
            module_index = g_cp_interupt_table->intrupt_sum;
            if(of_property_read_u32_index(child, "int_id" ,index, &(g_cp_interupt_table->interupt_info[module_index].interrpt_id)))
            {
                dump_fetal("get int_id fail\n");
            }
            else
            {
                /*coverity[secure_coding]*/
                snprintf(g_cp_interupt_table->interupt_info[module_index].module,(unsigned long)DUMP_MODULE_NAME_LEN, "%s", module_name);
                g_cp_interupt_table->intrupt_sum++;
            }
        }
    }

    dump_fetal("init apr dts success\n");
    return BSP_OK;
}
/*****************************************************************************
* 函 数 名  : bsp_dump_parse_reset_info
*
* 功能描述  : 将异常信息解析
*
* 输入参数  :  reset_info:存储解析后的异常信息的数据流
               size
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
void dump_apr_parse_reset_info( dump_reset_log_t *dump_reset_info, dump_except_info_t dump_except_info)
{
    dump_apr_get_reset_ccore(dump_except_info.except_core, (char*)(dump_reset_info->reboot_core));
    dump_apr_get_reset_reason(dump_except_info.except_reason, dump_reset_info->reboot_reson);
    dump_apr_get_reset_modid(dump_except_info.except_reason, dump_except_info.modId, &dump_reset_info->modid);
    dump_apr_get_reset_voice(dump_except_info.voice, dump_reset_info->voice);
    dump_apr_get_reset_task(dump_except_info, dump_reset_info->task_name,&dump_reset_info->reboot_int);
    dump_apr_get_reset_module(dump_except_info, dump_reset_info->task_name,dump_reset_info->module_name);
    if(DUMP_OUT_VOICE == dump_except_info.voice)
    {
        /*coverity[secure_coding]*/
        snprintf((char*)(dump_reset_info->brieftype), sizeof(dump_reset_info->brieftype), "%s", dump_reset_info->module_name);
    }
    else
    {
        /*coverity[secure_coding]*/
        snprintf((char*)(dump_reset_info->brieftype), sizeof(dump_reset_info->brieftype),"%s_VOICE", dump_reset_info->module_name);
    }
}
/*****************************************************************************
* 函 数 名  : bsp_dump_get_reset_module
*
* 功能描述  : 解析复位的责任组件
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
void dump_apr_get_reset_module(dump_except_info_t dump_except_info, u8* task_name,u8 * module)
{
    u32 index = 0;
    unsigned long size = 0;
    u32 reboot_reason = 0;

    dump_get_reboot_contex(NULL, &reboot_reason);
    /*主动复位根据modid确定责任组件*/
    if(DUMP_REASON_NORMAL == reboot_reason)
    {
        if(!g_p_modid_table)
        {
            return;
        }
        for(index = 0; index < g_p_modid_table->moudid_sum; index++)
        {
            if((dump_except_info.modId >=g_p_modid_table->modid_info[index].modid_start)
                &&(dump_except_info.modId< g_p_modid_table->modid_info[index].modid_end) )
            {
                /*coverity[secure_coding]*/
                snprintf((char*)module, (unsigned long)12, "%s", g_p_modid_table->modid_info[index].module);
                return;
            }
        }
        /*如果查找不到复位的责任组件则用复位任务和中断来判断责任组件*/
        if(index == g_p_modid_table->moudid_sum)
        {
            dump_fetal("can't find the modid in dts config\n");
            dump_fetal("reboot modid:0x%x\n", dump_except_info.modId);
        }
    }

    /*wdg abort根据临终任务名或中断号确定责任组件*/
    /*如果为中断复位*/
    if(DUMP_CTX_INT == dump_except_info.reboot_context)
    {
        if(!g_cp_interupt_table)
        {
            return;
        }
        for(index = 0; index < g_cp_interupt_table->intrupt_sum; index++ )
        {
            if(dump_except_info.reboot_int == g_cp_interupt_table->interupt_info[index].interrpt_id)
            {
                /*coverity[secure_coding]*/
                snprintf((char*)module, (unsigned long)12, "%s",  g_cp_interupt_table->interupt_info[index].module);
                break;
            }
        }
        if(index == g_cp_interupt_table->intrupt_sum)
        {
            dump_fetal("can't find the int in dts config\n");
            dump_fetal("reboot int num:0x%x\n", dump_except_info.reboot_int);
        }
        return;
    }
    if(DUMP_CTX_TASK == dump_except_info.reboot_context)
    {
        /*否则为任务复位*/
        if(!g_cp_task_table)
        {
            return;
        }
        /*查找责任组件*/
        for(index = 0; index < g_cp_task_table->task_sum; index++ )
        {
            size = (strlen((const char*)(g_cp_task_table->task_info[index].task_name)) > strlen((const char*)task_name)? \
                        strlen((const char*)task_name):strlen((const char*)(g_cp_task_table->task_info[index].task_name)));
            if(!strncmp((const char*)task_name, g_cp_task_table->task_info[index].task_name, size))
            {
                /*coverity[secure_coding]*/
                snprintf((char*)module, (unsigned long)12, "%s",  g_cp_task_table->task_info[index].module);
                break;
            }
        }
        /*如果在已有任务中查找不到，则责任组件为空*/
        if(index == g_cp_task_table->task_sum)
        {
            dump_fetal("can't find the task in dts config\n");
            dump_fetal("reboot task name:%s\n", task_name);
            return;
        }
    }
}
/*****************************************************************************
* 函 数 名  : bsp_dump_get_cp_reset_reason
*
* 功能描述  : 获取CP复位的原因
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
void dump_apr_get_reset_task(dump_except_info_t dump_except_info,  u8 * task_name, u32 * reboot_int)
{
    u8* task_name_table = NULL;
    u32 task_cnt = 0;
    dump_task_info_s * temp_task_name = NULL;
    u8 *dump_base_info = NULL;
    u32 task_index = 0;
    u32 task_info_filed = 0;
    unsigned long task_name_offset = 0;
    u32 reboot_reason = 0;
    u32 reboot_core = 0;

    dump_get_reboot_contex(&reboot_core, &reboot_reason);


    /*中断处理*/
    if(DUMP_CTX_INT == dump_except_info.reboot_context)
    {
        dump_fetal("reboot reason int\n");
        *reboot_int = dump_except_info.reboot_int;
        return;
    }
    /*任务处理*/
    if((DUMP_REASON_NORMAL == reboot_reason)||(DUMP_REASON_ARM == reboot_reason))
    {
        if(DUMP_CPU_COMM == reboot_core)
        {
            task_info_filed = DUMP_CP_BASE_INFO_SMP;
   
            task_name_offset = DUMP_TASK_NAME_OFFSET;
  

        }
        else if(DUMP_CPU_APP == reboot_core)
        {
            task_info_filed = DUMP_MODEMAP_BASE_INFO_SMP;
         
            task_name_offset = DUMP_TASK_NAME_OFFSET;
        

        }
        else
        {
            dump_fetal("except core error value:0x%x\n", dump_except_info.except_core);
            return;
        }
        /*获取基本信息和任务切换记录地址信息*/
        dump_base_info = bsp_dump_get_field_addr(task_info_filed);
        if(NULL == dump_base_info)
        {
            dump_fetal("get ddr info fail\n");
            dump_fetal("base info   field:0x%x ptr:0x%p\n", task_info_filed, dump_base_info);
            return;
        }
        /*lint -save -e845 */
        /*coverity[secure_coding]*/
        snprintf((char*)task_name, (unsigned long)16, "%s",(u8*)((uintptr_t)dump_base_info + task_name_offset));
        /*lint -restore +e845 */
    }
    else if(DUMP_REASON_WDT == reboot_reason || DUMP_REASON_DLOCK== reboot_reason)
    {
        /*如果为wdg复位*/
        if(DUMP_CPU_COMM == reboot_core)
        {
            task_info_filed = DUMP_CP_ALLTASK_NAME;
        }
        else if(DUMP_CPU_APP == reboot_core)
        {
            task_info_filed = DUMP_MODEMAP_ALLTASK;
        }
        /*CP存储任务名的区域*/
        task_name_table = bsp_dump_get_field_addr(task_info_filed);
        if(NULL == task_name_table)
        {
            dump_fetal("get cp tasck name ddr fail, field:0x%x\n", task_info_filed);
            return;
        }

        task_cnt = *((u32 *)(uintptr_t)task_name_table + 3)/4;
        /* 偏移10字节，去掉队列头 */
        task_name_table += 0x10;
        temp_task_name = (dump_task_info_s *)((uintptr_t)task_name_table);
        dump_fetal("task_cnt:0x%x\n", task_cnt);
        /*查找任务名*/
        for(task_index = 0;task_index < task_cnt; task_index++)
        {
            if(temp_task_name->task_id == dump_except_info.reboot_task)
            {
                dump_fetal("reboot task:%s\n", temp_task_name->task_name);
                /*coverity[secure_coding]*/
                snprintf((char*)task_name, (unsigned long)12 , "%s", temp_task_name->task_name);
                break;
            }
            temp_task_name++;
        }

        /*没有找到该任务*/
        if(task_index == task_cnt)
        {
            dump_fetal("can't find the task, task id:0x%x\n", dump_except_info.reboot_task);
            return;
        }
    }
    else
    {
        dump_base_info = bsp_dump_get_field_addr(DUMP_MODEMAP_BASE_INFO_SMP);
        if(NULL == dump_base_info)
        {
            dump_fetal("get ddr info fail\n");
            dump_fetal("base info   field:0x%x ptr:0x%p\n", task_info_filed, dump_base_info);
            return;
        }

        /*coverity[secure_coding]*/
        snprintf((char*)task_name, (unsigned long)16, "%s",(dump_base_info + DUMP_TASK_NAME_OFFSET));


    }
}

/*****************************************************************************
* 函 数 名  : bsp_dump_search_taskid
*
* 功能描述  : 查找复位的task id
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
s32 dump_apr_get_reset_taskid(dump_queue_t *Q)
{
    u32 element = 0;
    u32 index ;
    u32 queue_index = 0;
    u32 element_num = 0;

    if(Q->num < Q->maxNum)
    {
        element_num = Q->num;
    }
    else
    {
        element_num = Q->maxNum;
    }

    for(index = 0; index < element_num/2; index++)
    {
        queue_index = ((Q->rear+Q->maxNum)-2*(index + 1)) %Q->maxNum;
        element = Q->data[queue_index];
        if((DUMP_INT_IN_FLAG != ((element>>16)&0xFFFF))&&(DUMP_INT_EXIT_FLAG != ((element>>16)&0xFFFF)))
        {
            return (s32)element;
        }
    }
    return BSP_ERROR;
}

/*****************************************************************************
* 函 数 名  : bsp_dump_get_reset_context_and_taskid
*
* 功能描述  :获取复位为中断复位还是任务复位,如果为任务复位,解析复位任务id，如果复位为中断，解析复位中断号
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
void dump_apr_get_reset_context_and_id(u32 *reboot_context, u32 *reboot_task,u32 *reboot_int, struct dump_global_struct_s* dump_head)
{
    dump_queue_t* task_switch = NULL;
    dump_base_info_t *dump_base_info ;
    u32 task_switch_field = 0;
    u32 base_info_field = 0;
    u32 element = 0;
    u32 reboot_core = 0;

    *reboot_task = 0xFFFFFFFF;
    (void)dump_head;
    dump_get_reboot_contex(&reboot_core,NULL);
    if(DUMP_CPU_COMM== reboot_core)
    {
        task_switch_field = DUMP_CP_TASK_SWITCH;
        base_info_field = DUMP_CP_BASE_INFO_SMP;
        task_switch = (dump_queue_t*)((uintptr_t)bsp_dump_get_field_addr(task_switch_field));
        if(NULL == task_switch)
        {
            dump_fetal("get cp task switch ddr fail, field:0x%x\n", task_switch_field);
            return;
        }
    }
    else if(DUMP_CPU_APP == reboot_core)
    {
        //task_switch_field = DUMP_MODEMAP_TASK_SWITCH;
        base_info_field = DUMP_MODEMAP_BASE_INFO_SMP;
    }
    else
    {
        return;
    }

    dump_base_info = (dump_base_info_t*)((uintptr_t)bsp_dump_get_field_addr(base_info_field));


    if(NULL != dump_base_info)
    {
        if((dump_base_info->modId != DRV_ERRNO_DUMP_ARM_EXC
            && dump_base_info->modId != DRV_ERRNO_DUMP_CP_WDT
            && dump_base_info->modId != DRV_ERRNO_DLOCK))
        {
            *reboot_context = dump_base_info->reboot_context;
            if(DUMP_CTX_INT == dump_base_info->reboot_context)
            {
                *reboot_int = dump_base_info->reboot_int;
                *reboot_task = 0xFFFFFFFF;
                return;
            }
            else
            {
                *reboot_int = 0xFFFFFFFF;
                if(task_switch != NULL)
                {
                    *reboot_task = (u32)dump_apr_get_reset_taskid(task_switch);
                }
                else
                {
                    *reboot_task =  dump_base_info->reboot_task;
                }
                return;
            }
        }
        else
        {
            if(task_switch == NULL)
            {
                return;
            }
            /*如果为wdg和abort复位*/
            if(task_switch->rear+task_switch->maxNum < 2)
            {
                return;
            }
            /*lint -save -e679 */
            element = task_switch->data[((task_switch->rear+task_switch->maxNum)-2) % task_switch->maxNum];/* [false alarm]:fortify */
            /*lint -restore +e679 */
            if(DUMP_INT_IN_FLAG == ((element>>16)&0xFFFF))
            {
                *reboot_context = DUMP_CTX_INT;
                *reboot_int = element&0xFFFF;
                *reboot_task = 0xFFFFFFFF;
            }
            else
            {
                *reboot_context = DUMP_CTX_TASK;
                *reboot_int = 0xFFFFFFFF;
                *reboot_task = (u32)dump_apr_get_reset_taskid(task_switch);
            }

        }
    }
    else
    {
        dump_fetal("get ddr info fail\n");
        dump_fetal("base info field:0x%x ptr:0x%p\n", base_info_field, dump_base_info);
        return;
    }


}
/*****************************************************************************
* 函 数 名  : bsp_dump_get_reset_voice
*
* 功能描述  : 解析复位时是否在语音下
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
void dump_apr_get_reset_voice(u32 voice, u8 * reboot_voice)
{
    if(DUMP_OUT_VOICE == voice)
    {
        /*coverity[secure_coding]*/
        snprintf((char*)reboot_voice, (unsigned long)4, "no");
    }
    else
    {
        /*coverity[secure_coding]*/
        snprintf((char*)reboot_voice, (unsigned long)4, "yes");
    }
    return;
}
/*****************************************************************************
* 函 数 名  : bsp_dump_get_reset_modid
*
* 功能描述  : 获取复位的modid
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
void dump_apr_get_reset_modid(u32 reason, u32 reboot_modid, u32 * modId)
{
    if(DUMP_REASON_WDT == reason)
    {
       *modId =  DRV_ERRNO_DUMP_CP_WDT;
    }
    else if(DUMP_REASON_DLOCK == reason)
    {
       *modId =  DRV_ERRNO_DLOCK;
    }
    else
    {
        *modId = reboot_modid;
    }
}
/*****************************************************************************
* 函 数 名  : bsp_dump_get_cp_reset_reason
*
* 功能描述  : 获取复位的原因
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
void dump_apr_get_reset_reason(u32 reason, u8 * reboot_reason)
{
    if(DUMP_REASON_NORMAL == reason)
    {
        /*coverity[secure_coding]*/
        snprintf((char*)reboot_reason, (unsigned long)16, "%s", NOARMAL);
    }
    else if(DUMP_REASON_WDT == reason)
    {
        /*coverity[secure_coding]*/
        snprintf((char*)reboot_reason, (unsigned long)16, "%s", WDT);
    }
    else if(DUMP_REASON_ARM == reason)
    {
        /*coverity[secure_coding]*/
        snprintf((char*)reboot_reason, (unsigned long)16, "%s", DATA_ABORT);
    }
    else if(DUMP_REASON_NOC == reason)
    {
        /*coverity[secure_coding]*/
        snprintf((char*)reboot_reason, (unsigned long)16, "%s", NOC);
    }
    else if(DUMP_REASON_DMSS == reason)
    {
        /*coverity[secure_coding]*/
        snprintf((char*)reboot_reason, (unsigned long)16, "%s", DMSS);
    }
    else if(DUMP_REASON_RST_FAIL== reason)
    {
        /*coverity[secure_coding]*/
        snprintf((char*)reboot_reason, (unsigned long)16, "%s", RST_FAIL);
    }
    else if(DUMP_REASON_RST_NOT_SUPPORT== reason)
    {
        /*coverity[secure_coding]*/
        snprintf((char*)reboot_reason, (unsigned long)16, "%s", RST_NOT_SUPPORT);
    }
    else if(DUMP_REASON_UNDEF == reason)
    {
        /*coverity[secure_coding]*/
        snprintf((char*)reboot_reason, (unsigned long)16, "%s", UNDEFFINE);
    }
    else if(DUMP_REASON_DLOCK == reason)
    {
        /*coverity[secure_coding]*/
        snprintf((char*)reboot_reason, (unsigned long)16, "%s", DLOCK);
    }
    else
    {
        /*coverity[secure_coding]*/
        snprintf((char*)reboot_reason, (unsigned long)16, "%s", UNKNOW);
    }

}
/*****************************************************************************
* 函 数 名  : bsp_dump_get_reset_ccore
*
* 功能描述  : 获取CP复位的原因
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
void dump_apr_get_reset_ccore(u32 core, char *reboot_core)
{
    if(DUMP_CPU_COMM == core)
    {
        /*coverity[secure_coding]*/
        snprintf(reboot_core, (unsigned long)8, "%s", CCORE);
    }
    else if(DUMP_CPU_APP == core)
    {
        /*coverity[secure_coding]*/
        snprintf(reboot_core, (unsigned long)8, "%s", ACORE);
    }
    else
    {
        /*coverity[secure_coding]*/
        snprintf(reboot_core,(unsigned long)8, "%s", UNKNOW_CORE);
    }
    return;
}


/*****************************************************************************
* 函 数 名  : bsp_dump_show_apr_cfg
*
* 功能描述  : 显示apr的配置
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
s32  dump_apr_show_cfg(u32 type)
{
    u32 index = 0;

    if((2 == type)||(0 == type))
    {
        if(!g_cp_interupt_table)
        {
            dump_fetal("g_cp_interupt_table = NULL\n");
            return BSP_ERROR;
        }
        dump_fetal("interupt sum :0x%x\n", g_cp_interupt_table->intrupt_sum);
        for(index = 0;index < g_cp_interupt_table->intrupt_sum; index++)
        {
            dump_fetal("index:0x%-2x interrpt_id:0x%-8x module_name:%s\n", \
                index, g_cp_interupt_table->interupt_info[index].interrpt_id, \
                g_cp_interupt_table->interupt_info[index].module);
        }
    }
    if((3 == type)||(0 == type))
    {
        if(!g_cp_task_table)
        {
            dump_fetal("g_cp_task_table = NULL\n");
            return BSP_ERROR;
        }
        dump_fetal("task sum :0x%x\n", g_cp_task_table->task_sum);
        for(index = 0;index < g_cp_task_table->task_sum; index++)
        {
            dump_fetal("index:0x%-2x task:%-16s  module_name:%s\n", \
                index, g_cp_task_table->task_info[index].task_name, \
                g_cp_task_table->task_info[index].module);
        }
    }
    if((4 == type)||(0 == type))
    {
        if(!g_p_modid_table)
        {
            dump_fetal("g_p_modid_table = NULL\n");
            return BSP_ERROR;
        }
        dump_fetal("modid sum :0x%x\n", g_p_modid_table->moudid_sum);
        for(index = 0;index < g_p_modid_table->moudid_sum; index++)
        {
            dump_fetal("index:0x%-2x modid start:0x%-8x end:0x%-8x module_name:%s\n", \
                index, g_p_modid_table->modid_info[index].modid_start, g_p_modid_table->modid_info[index].modid_end, \
                g_p_modid_table->modid_info[index].module);
        }
    }
    return BSP_OK;
}
/*****************************************************************************
* 函 数 名  : om_get_reset_info
*
* 功能描述  : 获取复位信息
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
void dump_apr_get_reset_info(char * reset_info, u32 size)
{
    struct dump_global_area_ctrl_s global_area = {0,};
    struct dump_global_base_info_s *dump_head ;
    dump_except_info_t dump_except_info;
    dump_reset_log_t dump_reset_info = {{0,},};
    dump_base_info_t *dump_base_info ;
    u32 base_info_field = 0;
    u32 reboot_reason = 0;
    u32 reboot_core = 0;
    struct dump_area_mntn_addr_info_s area_info = {NULL,NULL,0};
    u32 flag = DUMP_VOICE_BUTT;
    dump_area_head_t* area_head = NULL;
    s32 ret;

    ret = dump_get_area_info(DUMP_AREA_CP,&area_info);

    if(ret == BSP_OK && area_info.vaddr)
    {
        area_head = (dump_area_head_t*)(area_info.vaddr);
        memcpy((u8*)&flag,(u8*)(area_head->version + 12),sizeof(u32));
    }

    dump_get_reboot_contex(&reboot_core,&reboot_reason);

    if(dump_get_global_info(&global_area) == BSP_ERROR)
    {
        return ;
    }

    dump_head = &(global_area.virt_addr->base_info);
    /*获取复位相关信息*/
    if(DUMP_CPU_COMM == reboot_core)
    {
        base_info_field = DUMP_CP_BASE_INFO_SMP;

    }
    else if(DUMP_CPU_APP == reboot_core)
    {
        base_info_field = DUMP_MODEMAP_BASE_INFO_SMP;
    }
    else
    {
        dump_fetal("except core error value:0x%x\n", dump_head->e_core);
        return;
    }

    dump_base_info = (dump_base_info_t*)((uintptr_t)bsp_dump_get_field_addr(base_info_field));
    if(NULL == dump_base_info)
    {
        dump_fetal("get ddr info fail\n");
        dump_fetal("base info   field:0x%x ptr:0x%p\n", base_info_field, dump_base_info);
        return;
    }
    dump_except_info.except_core = reboot_core;
    dump_except_info.voice = flag;
    dump_except_info.modId= dump_base_info->modId;
    dump_except_info.except_reason = reboot_reason;
    dump_apr_get_reset_context_and_id(&(dump_except_info.reboot_context),\
                &(dump_except_info.reboot_task),&(dump_except_info.reboot_int),(struct dump_global_struct_s*)((uintptr_t)dump_head));


    dump_fetal("except_core = 0x%x\n", dump_except_info.except_core);
    dump_fetal("voice = 0x%x\n", dump_except_info.voice);
    dump_fetal("modId = 0x%x\n", dump_except_info.modId);
    dump_fetal("except_reason = 0x%x\n", dump_except_info.except_reason);
    dump_fetal("reboot_int = 0x%x\n", dump_except_info.reboot_int);
    dump_fetal("reboot_context = 0x%x\n", dump_except_info.reboot_context);
    dump_fetal("reboot_task = 0x%x\n", dump_except_info.reboot_task);
    /*解析复位信息*/
    dump_apr_parse_reset_info(&dump_reset_info, dump_except_info);
    /*复位信息写入数据流*/
    /*coverity[secure_coding]*/
    snprintf(reset_info, (unsigned long)size,"brief:%s\ncmpnt:%s\nreset_reason:%s\ntask_name:%s\nmodid:0x%x\nreboot_int:0x%x\nvoice:%s\nreset_core:%s\n",
                dump_reset_info.brieftype,
                dump_reset_info.module_name,
                dump_reset_info.reboot_reson,
                dump_reset_info.task_name,
                dump_reset_info.modid,
                dump_reset_info.reboot_int,
                dump_reset_info.voice,
                dump_reset_info.reboot_core
            );


}
/*****************************************************************************
* 函 数 名  : bsp_apr_save_data
*
* 功能描述  : 保存apr数据
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
void dump_save_apr_data(char* dir_name)
{
    char temp_reset_info[256] = {0,};
    char file_name[MODEM_DUMP_FILE_NAME_LENGTH] = {0,};
    DUMP_FILE_CFG_STRU* cfg = dump_get_file_cfg();

    if(cfg->file_list.file_bits.reset_log == 1)
    {
        dump_apr_get_reset_info(temp_reset_info, 256);
        /*coverity[secure_coding]*/
        memset(file_name, 0, sizeof(file_name));
        /*coverity[secure_coding]*/
        snprintf(file_name, sizeof(file_name), "%sreset.log", dir_name);
        dump_save_file(file_name, (u8 *)temp_reset_info, (u32)strlen(temp_reset_info));
        dump_fetal("[dump]: save %s finished\n", file_name);
    }

}

