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

/************************************************************************
Copyright   : 2005-2008, Huawei Tech. Co., Ltd.
File name   : STKApi.c
Author      : m00128685
Version     : V200R001
Date        : 2008-12-28
Description : 该文件定义了STK模块的对外接口

Description :
History     :

1.日    期  : 2008年12月28日
  作    者  : m00128685
  修改内容  : Create
************************************************************************/

#include "siappstk.h"
#include "si_stk.h"
#include "product_config.h"




/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID PS_FILE_ID_STK_API_C

/*****************************************************************************
函 数 名  : GetMainMenu
功能描述  : 获取STK功能的主菜单
输入参数  : 无
输出参数  : 无
返 回 值  : VOS_UINT32函数执行结果
调用函数  : 无
被调函数  : 外部接口
History     :
1.日    期  : 2009年7月6日
  作    者  : H9254
  修改内容  : Create
*****************************************************************************/
VOS_UINT32 SI_STK_SendReqMsg(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId,
    VOS_UINT32                          MsgName,
    VOS_UINT32                          CmdType,
    VOS_UINT32                          DataLen,
    VOS_UINT8                           *pData)
{
    SI_STK_REQ_STRU         *pstSTKReq;
    VOS_UINT32              ulSendPid;
    VOS_UINT32              ulReceiverPid;
    MODEM_ID_ENUM_UINT16    enModemID;

    ulSendPid = WUEPS_PID_AT;

    /* 调用接口获取Modem ID */
    if(VOS_OK != AT_GetModemIdFromClient(ClientId,&enModemID))
    {
      STK_ERROR_LOG("SI_STK_SendReqMsg: AT_GetModemIdFromClient Return Error");
      return VOS_ERR;
    }

    if(MODEM_ID_1 == enModemID)
    {
        ulReceiverPid = I1_MAPS_STK_PID;
    }
    else if(MODEM_ID_2 == enModemID)
    {
        ulReceiverPid = I2_MAPS_STK_PID;
    }
    else
    {
        ulReceiverPid = I0_MAPS_STK_PID;
    }


    pstSTKReq = (SI_STK_REQ_STRU *)VOS_AllocMsg(ulSendPid, sizeof(SI_STK_REQ_STRU)-VOS_MSG_HEAD_LENGTH+DataLen);

    if(VOS_NULL_PTR == pstSTKReq)
    {
        STK_ERROR_LOG("SI_STK_SendReqMsg: VOS_AllocMsg Return Error");
        return VOS_ERR;
    }

    pstSTKReq->MsgName       = MsgName;
    pstSTKReq->ulReceiverPid = ulReceiverPid;
    pstSTKReq->OpId          = OpId;
    pstSTKReq->ClientId      = ClientId;
    pstSTKReq->SatType       = CmdType;
    pstSTKReq->Datalen       = DataLen;

    if(DataLen != 0)
    {
        PAM_MEM_CPY_S(pstSTKReq->Data, DataLen, pData, DataLen);
    }

    if(VOS_OK != VOS_SendMsg(ulSendPid, pstSTKReq))
    {
        return VOS_ERR;
    }

    return VOS_OK;
}

/*****************************************************************************
函 数 名  : GetMainMenu
功能描述  : 获取STK功能的主菜单
输入参数  : 无
输出参数  : 无
返 回 值  : VOS_UINT32函数执行结果
调用函数  : 无
被调函数  : 外部接口
History     :
1.日    期  : 2009年7月6日
  作    者  : H9254
  修改内容  : Create
*****************************************************************************/
VOS_UINT32 SI_STK_GetMainMenu(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId)
{
    return SI_STK_SendReqMsg(ClientId,OpId, SI_STK_GETMAINMNUE, SI_STK_NOCMDDATA,0,VOS_NULL_PTR);
}

/*****************************************************************************
函 数 名  : SI_STK_GetSTKCommand
功能描述  : 获取缓存的STK主动命令
输入参数  : 无
输出参数  : 无
返 回 值  : VOS_UINT32函数执行结果
调用函数  : 无
被调函数  : 外部接口
History     :
1.日    期  : 2008年10月18日
  作    者  : z00100318
  修改内容  : Create
*****************************************************************************/
VOS_UINT32 SI_STK_GetSTKCommand(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId,
    SI_STK_CMD_TYPE                     CmdType)
{
    return SI_STK_SendReqMsg(ClientId,OpId, SI_STK_GETCOMMAND, CmdType,0,VOS_NULL_PTR);
}

/*****************************************************************************
函 数 名  : SI_STK_QuerySTKCommand
功能描述  : 查询最后一次SIM卡上报的主动命令
输入参数  : 无
输出参数  : 无
返 回 值  : VOS_UINT32函数执行结果
调用函数  : 无
被调函数  : 外部接口
History     :
1.日    期  : 2008年10月18日
  作    者  : z00100318
  修改内容  : Create
*****************************************************************************/
VOS_UINT32 SI_STK_QuerySTKCommand(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId)
{
    return VOS_ERR;
}

/*****************************************************************************
函 数 名  : SI_STK_DataSendSimple
功能描述  : 终端给予下发数据函数
输入参数  : 无
输出参数  : 无
返 回 值  : VOS_UINT32函数执行结果
调用函数  : 无
被调函数  : 外部接口
History     :
1.日    期  : 2008年10月18日
  作    者  : z00100318
  修改内容  : Create
*****************************************************************************/
VOS_UINT32 SI_STK_DataSendSimple(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId,
    SI_SEND_DATA_TYPE                   SendType,
    VOS_UINT32                          DataLen,
    VOS_UINT8                           *pData)
{
    return SI_STK_SendReqMsg(ClientId,OpId, SI_STK_SIMPLEDOWN,SendType,DataLen,pData);
}

/*****************************************************************************
函 数 名  : SI_STK_TerminalResponse
功能描述  : 终端给予响应函数
输入参数  : 无
输出参数  : 无
返 回 值  : VOS_UINT32函数执行结果
调用函数  : 无
被调函数  : 外部接口
History     :
1.日    期  : 2008年10月18日
  作    者  : z00100318
  修改内容  : Create
*****************************************************************************/
VOS_UINT32 SI_STK_TerminalResponse(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId,
    SI_STK_TERMINAL_RSP_STRU            *pstTRStru)
{
    return VOS_ERR;
}


/*****************************************************************************
函 数 名  : SI_STKDualIMSIChangeReq
功能描述  : 触发IMSI切换流程，沃达丰定制流程
输入参数  : 无
输出参数  : 无
返 回 值  : VOS_UINT32函数执行结果
调用函数  : 无
被调函数  : 外部接口
History     :
1.日    期  : 2010年02月10日
  作    者  : m00128685
  修改内容  : Create
*****************************************************************************/
VOS_UINT32 SI_STKDualIMSIChangeReq(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId)
{
    return TAF_FAILURE;
}

/*****************************************************************************
函 数 名  : SI_STKIsDualImsiSupport
功能描述  : 判断是否支持Dual IMSI切换
输入参数  : 无
输出参数  : 无
返 回 值  : 执行结果
调用函数  : 无
被调函数  : 外部接口
History     :
1.日    期  : 2010年02月11日
  作    者  : m00128685
  修改内容  : Create
*****************************************************************************/
VOS_UINT32 SI_STKIsDualImsiSupport(VOS_VOID)
{
    return TAF_FAILURE;
}

/*****************************************************************************
函 数 名  : SI_STK_MenuSelection
功能描述  : STK功能ENVELOP下载函数
输入参数  : 无
输出参数  : 无
返 回 值  : VOS_UINT32函数执行结果
调用函数  : 无
被调函数  : 外部接口
History     :
1.日    期  : 2008年10月18日
  作    者  : z00100318
  修改内容  : Create
*****************************************************************************/

VOS_UINT32 SI_STK_MenuSelection(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId,
    SI_STK_ENVELOPE_STRU                *pstENStru)
{
    return TAF_FAILURE;
}

/*****************************************************************************
函 数 名  : SI_STK_SetUpCallConfirm
功能描述  : 用户回复是否允许发起呼叫
输入参数  : ulAction -- 用户回复结果
输出参数  : 无
返 回 值  : 执行结果
调用函数  : 无
被调函数  : 外部接口
History     :
1.日    期  : 2012年9月14日
  作    者  : j00168360
  修改内容  : Create
*****************************************************************************/
VOS_UINT32 SI_STK_SetUpCallConfirm(
    MN_CLIENT_ID_T                      ClientId,
    SI_STK_SETUPCALLCONFIRM_ENUM_UINT32 enAction)
{
    /* 参数检查 */
    if (SI_STK_SETUPCALL_BUTT <= enAction)
    {
        STK_ERROR_LOG("SI_STK_SetUpCallConfirm: The Input Para is Error");

        return VOS_ERR;
    }

    return SI_STK_SendReqMsg(ClientId, 0, SI_STK_SETUPCALL_CONFIRM, SI_STK_SETUPCALL, sizeof(VOS_UINT32), (VOS_UINT8*)&enAction);
}





