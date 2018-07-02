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
#include <linux/kthread.h>
#include <linux/timer.h>
#include <linux/timex.h>
#include <linux/rtc.h>
#include <linux/sched.h>
#include "dump_file.h"
#include "dump_print.h"
#include "dump_modem_save.h"
#include "dump_config.h"
#include "dump_cp_agent.h"
#include "dump_apr.h"
#include "dump_modem_area.h"
#include "dump_modem_rdr.h"
#include "dump_modem_mem.h"

modem_dump_ctrl_s            g_dump_ctrl;


/*****************************************************************************
* 函 数 名  : dump_save_log_files
* 功能描述  : 保存全部的modem log文件
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_save_log_files(char * dir_name)
{
    s32 ret = BSP_OK;
    
    if(dir_name == NULL)
    {
        dump_fetal("dir_name is null\n");
        return;
    }

    ret = dump_create_dir(dir_name);
    if(ret != BSP_OK )
    {
        dump_fetal("creat dir fail exit\n");
        return;
    }

    dump_fetal("[dump]: enter save log file\n");

    dump_save_cp_logs(dir_name);

    dump_save_mem_bin(dir_name);
    
    dump_save_apr_data(dir_name);

    dump_fetal("dump all files save finished\n");

}
/*****************************************************************************
* 函 数 名  : dump_save_and_reboot
* 功能描述  : 触发保存任务，并且复位
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_save_and_reboot(void)
{
    g_dump_ctrl.dump_task_job = DUMP_TASK_JOB_SAVE_REBOOT;

    up(&g_dump_ctrl.sem_dump_task);

    dump_fetal("up g_dump_ctrl.sem_dump_task \n");

    return;
}


/*****************************************************************************
* 函 数 名  : dump_save_task
* 功能描述  : 保存modem log的入口函数
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
int dump_save_task(void *data)
{
    rdr_exc_info_s* rdr_exc_info = dump_get_rdr_exc_info();
    s32 ret = BSP_OK;
    
    /* coverity[no_escape] */
    while(1)
    {
        down(&g_dump_ctrl.sem_dump_task);

        dump_fetal("down g_dump_ctrl.sem_dump_task\n");

        if(DUMP_TASK_JOB_SAVE_REBOOT == (g_dump_ctrl.dump_task_job & DUMP_TASK_JOB_SAVE_REBOOT))
        {
            dump_fetal("begin to save modem log \n");

            ret = dump_wait_cp_save_done(5000, (bool)true);
            if(ret == BSP_ERROR)
            {
                dump_cp_timeout_proc();
            }

            dump_save_log_files(rdr_exc_info->log_path);

            if(rdr_exc_info->dump_done != BSP_NULL)
            {
                rdr_exc_info->dump_done(rdr_exc_info->modid, rdr_exc_info->coreid);

                dump_fetal("notify rdr dump handle finished\n");
            }
        }

        g_dump_ctrl.dump_task_job = 0;
    }

    /*lint -e527 -esym(527,*)*/
    return BSP_OK;
    /*lint -e527 +esym(527,*)*/    
}

/*****************************************************************************
* 函 数 名  : dump_save_task_init
* 功能描述  : 创建modem ap 保存log 的任务函数
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
s32 dump_save_task_init(void)
{
    struct task_struct * pid = NULL;
    struct sched_param   param = {0,};

    sema_init(&g_dump_ctrl.sem_dump_task, 0);
    g_dump_ctrl.dump_task_job = 0;

    pid = (struct task_struct *)kthread_run(dump_save_task, 0, "dump_save");
    if (IS_ERR((void*)pid))
    {
        dump_error("dump_save_task_init[%d]: create kthread task failed! ret=%p\n", __LINE__, pid);
        return BSP_ERROR;
    }
    g_dump_ctrl.dump_task_id = (uintptr_t)pid;

    param.sched_priority = 97;
    if (BSP_OK != sched_setscheduler(pid, SCHED_FIFO, &param))
    {
        dump_error("dump_save_task_init[%d]: sched_setscheduler failed!\n", __LINE__);
        return BSP_ERROR;
    }

    dump_fetal("dump_save_task_init finish\n");

    return BSP_OK;
}

/****************************************************************************
* 函 数 名  : bsp_om_save_reboot_log
* 功能描述  : 对外接口用于开关机接口保存开关机信息
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void bsp_om_save_reboot_log(const char * func_name, const void* caller)
{

    struct timex txc = {0,};
    struct rtc_time tm = {0,};
    char log_buff[200] = {0};
    char temp[30] = {0};

    do_gettimeofday(&(txc.time));
    rtc_time_to_tm(txc.time.tv_sec, &tm);
    
    /*coverity[secure_coding]*/
    /* coverity[overrun-buffer-val] */
    snprintf(temp, sizeof(temp), "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    /*coverity[secure_coding]*/
    snprintf((char*)log_buff,sizeof(log_buff) ,"system reboot reason: NORMAL_RESET A CORE, FUNC:%s, caller:%p, TIME:%s\n", func_name, caller, temp);
    dump_append_file(OM_DUMP_PATH, OM_RESET_LOG, (void*)log_buff, (u32)(strlen(log_buff)), OM_RESET_LOG_MAX);
    dump_fetal("bsp_om_save_reboot_log finish\n");

}

