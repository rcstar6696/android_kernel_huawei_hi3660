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



#include <product_config.h>
#include "osl_types.h"
#include "osl_bio.h"
#include "osl_io.h"
#include "bsp_rfile.h"
#include "bsp_dspload.h"
#include "bsp_dsp.h"
#include "bsp_ddr.h"
#include "dump_cphy_tcm.h"
#include "dump_file.h"
#include "dump_print.h"
#include "dump_config.h"


/*****************************************************************************
* 函 数 名  : om_save_cphy_tcm
* 功能描述  : 保存xdsp的镜像
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_save_cphy_tcm(char *  dst_path)
{
    u8 * data;
    bool flag = false;
    char  file_name[128] = {0,};
    struct bbe_dump_proc_flag *c_flag;
    DUMP_FILE_CFG_STRU* cfg = dump_get_file_cfg();

    if(cfg->file_list.file_bits.cphy_tcm != 1)
    {
        return;
    }

    c_flag = (struct bbe_dump_proc_flag *)((unsigned long)SHM_BASE_ADDR + SHM_OFFSET_CDSP_FLAG);
    if (SAVE_TCM_BEGIN == c_flag->dsp_dump_flag)
    {
        dump_fetal("carry xdsp tcm to ddr not finished!\n");
        flag = true;
    }
    else if(SAVE_TCM_END == c_flag->dsp_dump_flag)
    {
        flag = true;
    }
    else
    {
        flag = false;
    }

    data = (u8 *)ioremap_wc(MDDR_FAMA(DDR_CBBE_IMAGE_ADDR),DDR_CBBE_IMAGE_SIZE);
    if (NULL == data)
    {
        dump_fetal("ioremap DDR_CBBE_IMAGE_ADDR fail\n");
        return;
    }

    /*满足DSP处于上电条件的情况下cphy_dtcm.bin的在所有版本中均需要保存*/
    if (flag ==true)
    {
        /* coverity[HUAWEI DEFECT] */
        memset(file_name, 0, sizeof(file_name));
        /* coverity[HUAWEI DEFECT] */
        snprintf(file_name, sizeof(file_name), "%scphy_dtcm.bin", dst_path);
        dump_save_file(file_name, data, CPHY_PUB_DTCM_SIZE);
        dump_fetal("[dump]: save %s finished\n", file_name);
    }

    if(EDITION_INTERNAL_BETA != dump_get_edition_type())
    {
        iounmap(data);
        return;
    }

    if (flag == true)
    {
        /* coverity[HUAWEI DEFECT] */
        memset(file_name, 0, sizeof(file_name));
        /* coverity[HUAWEI DEFECT] */
        snprintf(file_name, sizeof(file_name), "%scphy_itcm.bin", dst_path);
        dump_save_file(file_name, data+CPHY_PUB_DTCM_SIZE, CPHY_PUB_ITCM_SIZE);
        dump_fetal("[dump]: save %s finished\n", file_name);
    }
    else
    {
        /* coverity[HUAWEI DEFECT] */
        memset(file_name, 0, sizeof(file_name));
        /* coverity[HUAWEI DEFECT] */
        snprintf(file_name, sizeof(file_name), "%scphy_ddr.bin", dst_path);
        dump_save_file(file_name, data, DDR_CBBE_IMAGE_SIZE);
        dump_fetal("[dump]: save %s finished\n", file_name);
    }
    c_flag->dsp_dump_flag = 0;

    iounmap(data);
    return;
}


