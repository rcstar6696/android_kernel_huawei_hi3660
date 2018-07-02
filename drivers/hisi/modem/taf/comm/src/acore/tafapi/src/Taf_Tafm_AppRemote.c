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
#include "vos.h"
#include "Taf_Tafm_Remote.h"

#include "MnCommApi.h"

#include "TafApi.h"

#include "MnClient.h"
#include "TafAppMma.h"




/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID PS_FILE_ID_TAF_TAFM_APPREMOTE_C

/****************************************************/
/*    远端API使用的全局变量全部在定义时进行初始化   */
/****************************************************/
/*记录APP回调函数信息结果*/


/*记录TAF远端是否注册过MUX回调函数*/
VOS_UINT8 g_ucTafRetRegMuxCallbackFlag = TAF_REG_MUX_CALLBACK_NO;

/*****************************************************************************
 函 数 名  : Taf_GetAppMsgLen
 功能描述  :
 输入参数  : VOS_UINT32 ulParaLen
             VOS_UINT32 *pulMsgLen
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2008年7月8日
    作    者   : luojian id:107747
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32  Taf_GetAppMsgLen(VOS_UINT32 ulParaLen, VOS_UINT32 *pulMsgLen)
{
    VOS_UINT32  ulMsgLen;

    if(VOS_NULL_PTR == pulMsgLen)
    {
        return VOS_ERR;
    }

    if ( ulParaLen <= 4 )
    {
        ulMsgLen = sizeof(MN_APP_REQ_MSG_STRU);
    }
    else
    {
        ulMsgLen = (sizeof(MN_APP_REQ_MSG_STRU) + ulParaLen) -4;
    }

    *pulMsgLen = ulMsgLen;

    return VOS_OK;

}

/**********************************************************/
/*                  通信参数设置查询API                   */
/**********************************************************/

/**********************************************************
 Function:       Taf_ParaQuery
 Description:    通信参数查询函数
 Calls:          APP/AT
 Data Accessed:
 Data Updated:
 Input:          ClientId - APP/AT标识
                 OpId - 操作标识
                 ParaType - 通信参数类型
                 pPara - 辅助参数
 Output:
 Return:         TAF_SUCCESS - 成功
                 TAF_FAILURE - 失败
 Others:
  1.   L47619   2006.03.03   问题单:A32D02338
  2 日    期 : 2006-12-07
    作    者 : 韩鲁峰
    修改内容 : A32D07824 增加CGEQNEG的处理
  3.日    期 : 2013年07月08日
    作    者 : Y00213812
    修改内容 : VoLTE_PhaseI 项目，删除冗余代码
**********************************************************/
VOS_UINT32  Taf_ParaQuery  (
    VOS_UINT16                     ClientId,
    VOS_UINT8                      OpId,
    TAF_PARA_TYPE                  ParaType,
    VOS_VOID                      *pPara
)
{
    VOS_UINT8                           *pucPara;
    VOS_UINT16                           usMsgName;
    VOS_UINT32                           ulParaLen;
    VOS_UINT32                           ulRst;
    VOS_UINT32                           ulReceiverPid;

    if ((ParaType >= TAF_PH_MS_CLASS_PARA)
        && (ParaType < TAF_TELE_PARA_BUTT))
    {
        ulParaLen = sizeof(TAF_PARA_TYPE);
        ulReceiverPid = I0_WUEPS_PID_MMA;
        usMsgName = TAF_MSG_PARA_READ;
    }
    else
    {
        return TAF_FAILURE;
    }

    pucPara = (VOS_UINT8 *)PS_MEM_ALLOC( WUEPS_PID_AT, ulParaLen );

    if (VOS_NULL_PTR == pucPara)
    {
        return TAF_FAILURE;
    }

    TAF_MEM_SET_S( pucPara, ulParaLen, 0x00 , ulParaLen );

    TAF_MEM_CPY_S( pucPara, ulParaLen, &ParaType, sizeof(TAF_PARA_TYPE) );

    ulRst = MN_FillAndSndAppReqMsg( ClientId,
                                  OpId,
                                  usMsgName,
                                  pucPara,
                                  ulParaLen,
                                  ulReceiverPid);
    PS_MEM_FREE( WUEPS_PID_AT, pucPara );

    return ulRst;
}



