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

#include <linux/timer.h>
#include <linux/thread_info.h>
#include <linux/syslog.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <asm/string.h>
#include "osl_types.h"
#include "osl_bio.h"
#include "osl_io.h"
#include "bsp_rfile.h"
#include "bsp_dspload.h"
#include "bsp_dsp.h"
#include "bsp_dump.h"
#include "bsp_ddr.h"
#include "dump_file.h"
#include "dump_print.h"
#include "dump_config.h"


char * lphy_image_ddr_addr = NULL;
/*****************************************************************************
* 函 数 名  : dump_save_all_tcm
* 功能描述  : 保存全部的tcm文件
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_save_all_tcm(char* data,char* dst_path)
{
    char file_name[128] = {0}; 
    /* DTCM PUB */
    /* coverity[HUAWEI DEFECT] */
    memset(file_name, 0, sizeof(file_name));
    /* coverity[HUAWEI DEFECT] */
    snprintf(file_name, sizeof(file_name), "%slphy_pub_dtcm.bin", dst_path);
    dump_save_file(file_name,data,LPHY_PUB_DTCM_SIZE);
    dump_fetal("[dump]: save %s finished!\n",file_name);

    /*DTCM MOD*/
    /* coverity[HUAWEI DEFECT] */
    memset(file_name, 0, sizeof(file_name));
    /* coverity[HUAWEI DEFECT] */
    snprintf(file_name, sizeof(file_name), "%slphy_mode_dtcm.bin", dst_path);
    dump_save_file(file_name,data+LPHY_PUB_DTCM_SIZE,LPHY_PRV_DTCM_SIZE);
    dump_fetal("[dump]: save %s finished!\n",file_name);

    if(dump_get_edition_type() == EDITION_INTERNAL_BETA)
    {
        /* ITCM PUB */
        /* coverity[HUAWEI DEFECT] */
        memset(file_name, 0, sizeof(file_name));
        /* coverity[HUAWEI DEFECT] */
        snprintf(file_name, sizeof(file_name), "%slphy_pub_itcm.bin", dst_path);
        dump_save_file(file_name,data+LPHY_PUB_DTCM_SIZE+LPHY_PRV_DTCM_SIZE,LPHY_PUB_ITCM_SIZE);
        dump_fetal("[dump]: save %s finished!\n",file_name);

        /*ITCM MOD*/
        /* coverity[HUAWEI DEFECT] */
        memset(file_name, 0, sizeof(file_name));
        /* coverity[HUAWEI DEFECT] */
        snprintf(file_name, sizeof(file_name), "%slphy_mode_itcm.bin", dst_path);
        dump_save_file(file_name,data+LPHY_PUB_DTCM_SIZE+LPHY_PUB_ITCM_SIZE+LPHY_PRV_DTCM_SIZE,LPHY_PRV_ITCM_SIZE);
        dump_fetal("[dump]: save %s finished!\n",file_name);
    }
}

/*****************************************************************************
* 函 数 名  : dump_save_some_tcm
* 功能描述  : 保存全部的dtcm和itcm文件
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_save_some_tcm(char* data,char* dst_path)
{
    int fd;
    int ret;
    char file_name[128] = {0};

    /*MBB与PHONE均保存DTCM*/
    /* coverity[HUAWEI DEFECT] */
    memset(file_name, 0, sizeof(file_name));
    /* coverity[HUAWEI DEFECT] */
    snprintf(file_name, sizeof(file_name), "%slphy_dtcm.bin", dst_path);
    fd = bsp_open(file_name,RFILE_RDWR|RFILE_CREAT,0660);
    if(fd<0){
        dump_fetal("[dump]:open %s failed ,save lphy_dtcm failed!\n",file_name);
        return;
    }
    ret = bsp_write(fd,data,LPHY_PUB_DTCM_SIZE);
    if(ret != LPHY_PUB_DTCM_SIZE)
        goto err0;
    ret = bsp_write(fd,data+LPHY_PUB_DTCM_SIZE + LPHY_PUB_ITCM_SIZE,LPHY_PRV_DTCM_SIZE);
    if(ret != LPHY_PRV_DTCM_SIZE)
        goto err0;
    ret = bsp_write(fd,data+LPHY_PUB_DTCM_SIZE + LPHY_PUB_ITCM_SIZE + LPHY_PRV_DTCM_SIZE + LPHY_PRV_ITCM_SIZE,LPHY_PRV_DTCM_SIZE);
    if(ret != LPHY_PRV_DTCM_SIZE)
        goto err0;
    dump_fetal("[dump]: save %s finished!\n",file_name);
err0:
    bsp_close(fd);

    /*如果是PHONE产品则同时保存LPHY ITCM，MBB受空间限制则只保存DTCM部分*/
    if(DUMP_PHONE == dump_get_product_type())
    {
        /* coverity[HUAWEI DEFECT] */
        memset(file_name, 0, sizeof(file_name));
        /* coverity[HUAWEI DEFECT] */
        snprintf(file_name, sizeof(file_name), "%slphy_itcm.bin", dst_path);
        fd = bsp_open(file_name,RFILE_RDWR|RFILE_CREAT,0660);
        if(fd<0){
            dump_fetal("[dump]:open %s failed ,save lphy_itcm failed!\n",file_name);
            return;
        }
        ret = bsp_write(fd,data+LPHY_PUB_DTCM_SIZE,LPHY_PUB_ITCM_SIZE);
        if(ret != LPHY_PUB_ITCM_SIZE)
            goto err1;
        ret = bsp_write(fd,data+LPHY_PUB_DTCM_SIZE+LPHY_PUB_ITCM_SIZE+LPHY_PRV_DTCM_SIZE,LPHY_PRV_ITCM_SIZE);
        if(ret != LPHY_PRV_ITCM_SIZE)
            goto err1;
        ret = bsp_write(fd,data+LPHY_PUB_DTCM_SIZE+LPHY_PUB_ITCM_SIZE+LPHY_PRV_DTCM_SIZE+LPHY_PRV_ITCM_SIZE+LPHY_PRV_DTCM_SIZE,LPHY_PRV_ITCM_SIZE);
        if(ret != LPHY_PRV_ITCM_SIZE)
            goto err1;
        dump_fetal("[dump]: save %s finished!\n",file_name);
    err1:
        bsp_close(fd);
    }

}
/*****************************************************************************
* 函 数 名  : om_save_lphy_tcm
* 功能描述  : 保存tldsp的镜像
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/

void dump_save_lphy_tcm(char * dst_path)
{
    bool flag = false;
    struct dsp_dump_proc_flag *tl_flag = NULL;
    DUMP_FILE_CFG_STRU* cfg = dump_get_file_cfg();

    if(cfg->file_list.file_bits.lphy_tcm == 0)
    {
        return;
    }

    lphy_image_ddr_addr = (char *)ioremap_wc(MDDR_FAMA(DDR_TLPHY_IMAGE_ADDR), DDR_TLPHY_IMAGE_SIZE);
    if(NULL == lphy_image_ddr_addr)
    {
        dump_fetal("ioremap DDR_TLPHY_IMAGE_ADDR fail\n");
        return;
    }

    tl_flag = (struct dsp_dump_proc_flag *)((unsigned long)SHM_BASE_ADDR + SHM_OFFSET_DSP_FLAG);
    if(SAVE_TCM_BEGIN == tl_flag->dsp_dump_flag)
    {
        dump_fetal("carry tldsp tcm to ddr not finished!\n");
        flag = true;
    }
    else if(SAVE_TCM_END == tl_flag->dsp_dump_flag)
    {
        flag = true;
    }
    else
    {
        flag = false;
    }

    /*DSP DDR内存分布参考hi_dsp.h*/
    if(flag == true)
    {
        dump_save_all_tcm(lphy_image_ddr_addr,dst_path);
    }
    else if(dump_get_edition_type() == EDITION_INTERNAL_BETA)
    {
        dump_save_some_tcm(lphy_image_ddr_addr,dst_path);
    }
    tl_flag->dsp_dump_flag = 0;

    iounmap(lphy_image_ddr_addr);
    return;
}

