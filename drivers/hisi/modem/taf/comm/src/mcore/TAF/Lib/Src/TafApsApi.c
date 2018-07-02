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


/*****************************************************************************
   1 头文件包含
*****************************************************************************/


#include "PsCommonDef.h"
#include "TafApsApi.h"





/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID        PS_FILE_ID_TAF_APS_API_C


/*****************************************************************************
   2 全局变量定义
*****************************************************************************/


/*****************************************************************************
   3 外部函数声明
*****************************************************************************/

/*****************************************************************************
   4 函数实现
*****************************************************************************/

/*****************************************************************************
 函 数 名  : TAF_PS_SndMsg
 功能描述  : 发送PS域消息
 输入参数  : VOS_UINT32                          ulTaskId
             VOS_UINT32                          ulMsgId
             VOS_VOID                           *pData
             VOS_UINT32                          ulLen
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月8日
    作    者   : A00165503
    修改内容   : 新生成函数
  2.日    期   : 2012年12月21日
    作    者   : l00227485
    修改内容   : DSDA PhaseII
  3.日    期   : 2016年02月19日
    作    者   : w00316404
    修改内容   : Modify for DTS2016021808321
*****************************************************************************/
VOS_UINT32 TAF_PS_SndMsg(
    VOS_UINT16                          usModemId,
    VOS_UINT32                          ulMsgId,
    VOS_VOID                           *pData,
    VOS_UINT32                          ulLength
)
{
    VOS_UINT32                          ulResult = 0;
    TAF_PS_MSG_STRU                    *pstMsg   = VOS_NULL_PTR;
    VOS_UINT32                          ulPid;

    switch (usModemId)
    {
        case MODEM_ID_0:
            ulPid = I0_WUEPS_PID_TAF;
            break;

        case MODEM_ID_1:
            ulPid = I1_WUEPS_PID_TAF;
            break;

        case MODEM_ID_2:
            ulPid = I2_WUEPS_PID_TAF;
            break;

        default:
            PS_PRINTF("TAF_PS_SndMsg: ModemId is Error!");
            return VOS_ERR;
    }

    /* 构造消息 */
    pstMsg = (TAF_PS_MSG_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                ulPid,
                                sizeof(MSG_HEADER_STRU) + ulLength);
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_ERR;
    }

    pstMsg->stHeader.ulReceiverPid      = ulPid;
    pstMsg->stHeader.ulMsgName          = ulMsgId;

    /* 填写消息内容 */
    TAF_MEM_CPY_S(pstMsg->aucContent, ulLength, pData, ulLength);

    /* 发送消息 */
    ulResult = PS_SEND_MSG(ulPid, pstMsg);
    if (VOS_OK != ulResult)
    {
        return VOS_ERR;
    }

    return VOS_OK;
}

/*****************************************************************************
 函 数 名  : TAF_PS_SetPrimPdpContextInfo
 功能描述  : 设置Primary PDP上下文信息
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             pstPdpContextInfo          - Primary PDP上下文信息
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数
  2.日    期   : 2013年07月08日
    作    者   : Y00213812
    修改内容   : VoLTE_PhaseI 项目，结构名称修改，增加PID

*****************************************************************************/
VOS_UINT32 TAF_PS_SetPrimPdpContextInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    TAF_PDP_PRIM_CONTEXT_EXT_STRU      *pstPdpContextInfo
)
{
    VOS_UINT32                                  ulResult;
    TAF_PS_SET_PRIM_PDP_CONTEXT_INFO_REQ_STRU   stSetPdpCtxInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stSetPdpCtxInfoReq, sizeof(stSetPdpCtxInfoReq), 0x00, sizeof(TAF_PS_SET_PRIM_PDP_CONTEXT_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_SET_PRIM_PDP_CONTEXT_INFO_REQ消息 */
    stSetPdpCtxInfoReq.stCtrl.ulModuleId = ulModuleId;
    stSetPdpCtxInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stSetPdpCtxInfoReq.stCtrl.ucOpId     = ucOpId;
    stSetPdpCtxInfoReq.stPdpContextInfo  = *pstPdpContextInfo;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_SET_PRIM_PDP_CONTEXT_INFO_REQ,
                             &stSetPdpCtxInfoReq,
                             sizeof(TAF_PS_SET_PRIM_PDP_CONTEXT_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetPrimPdpContextInfo
 功能描述  : 获取Primary PDP上下文信息
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数
  2.日    期   : 2013年07月08日
    作    者   : Y00213812
    修改内容   : VoLTE_PhaseI 项目，结构名称修改，增加PID

*****************************************************************************/
VOS_UINT32 TAF_PS_GetPrimPdpContextInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId
)
{
    VOS_UINT32                                  ulResult;
    TAF_PS_GET_PRIM_PDP_CONTEXT_INFO_REQ_STRU   stGetPdpCtxInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stGetPdpCtxInfoReq, sizeof(stGetPdpCtxInfoReq), 0x00, sizeof(TAF_PS_GET_PRIM_PDP_CONTEXT_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_GET_PRIM_PDP_CONTEXT_INFO_REQ消息 */
    stGetPdpCtxInfoReq.stCtrl.ulModuleId = ulModuleId;
    stGetPdpCtxInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetPdpCtxInfoReq.stCtrl.ucOpId     = ucOpId;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_PRIM_PDP_CONTEXT_INFO_REQ,
                             &stGetPdpCtxInfoReq,
                             sizeof(TAF_PS_GET_PRIM_PDP_CONTEXT_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetPdpContextInfo
 功能描述  : 获取主和从 PDP上下文信息
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年3月5日
    作    者   : c00173809
    修改内容   : DTS2012010604900
*****************************************************************************/
VOS_UINT32 TAF_PS_GetPdpContextInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId
)
{
    VOS_UINT32                                  ulResult;
    TAF_PS_GET_PDP_CONTEXT_INFO_REQ_STRU        stGetPdpCtxInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stGetPdpCtxInfoReq, sizeof(stGetPdpCtxInfoReq), 0x00, sizeof(TAF_PS_GET_PDP_CONTEXT_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_GET_PRIM_PDP_CONTEXT_INFO_REQ消息 */
    stGetPdpCtxInfoReq.stCtrl.ulModuleId = ulModuleId;
    stGetPdpCtxInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetPdpCtxInfoReq.stCtrl.ucOpId     = ucOpId;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_PDPCONT_INFO_REQ,
                             &stGetPdpCtxInfoReq,
                             sizeof(TAF_PS_GET_PDP_CONTEXT_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_SetSecPdpContextInfo
 功能描述  : 设置Secondary PDP上下文信息
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             pstPdpContextInfo          - Secondary PDP上下文信息
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_SetSecPdpContextInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    TAF_PDP_SEC_CONTEXT_EXT_STRU       *pstPdpContextInfo
)
{
    VOS_UINT32                                  ulResult;
    TAF_PS_SET_SEC_PDP_CONTEXT_INFO_REQ_STRU    stSetPdpCtxInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stSetPdpCtxInfoReq, sizeof(stSetPdpCtxInfoReq), 0x00, sizeof(TAF_PS_SET_SEC_PDP_CONTEXT_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_SET_SEC_PDP_CONTEXT_INFO_REQ消息 */
    stSetPdpCtxInfoReq.stCtrl.ulModuleId = ulModuleId;
    stSetPdpCtxInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stSetPdpCtxInfoReq.stCtrl.ucOpId     = ucOpId;
    stSetPdpCtxInfoReq.stPdpContextInfo  = *pstPdpContextInfo;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_SET_SEC_PDP_CONTEXT_INFO_REQ,
                             &stSetPdpCtxInfoReq,
                             sizeof(TAF_PS_SET_SEC_PDP_CONTEXT_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetSecPdpContextInfo
 功能描述  : 获取Secondary PDP上下文信息
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_GetSecPdpContextInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId
)
{
    VOS_UINT32                                  ulResult;
    TAF_PS_GET_SEC_PDP_CONTEXT_INFO_REQ_STRU    stGetPdpCtxInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stGetPdpCtxInfoReq, sizeof(stGetPdpCtxInfoReq), 0x00, sizeof(TAF_PS_GET_SEC_PDP_CONTEXT_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_GET_SEC_PDP_CONTEXT_INFO_REQ消息 */
    stGetPdpCtxInfoReq.stCtrl.ulModuleId = ulModuleId;
    stGetPdpCtxInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetPdpCtxInfoReq.stCtrl.ucOpId     = ucOpId;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_SEC_PDP_CONTEXT_INFO_REQ,
                             &stGetPdpCtxInfoReq,
                             sizeof(TAF_PS_GET_SEC_PDP_CONTEXT_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_SetTftInfo
 功能描述  : 设置PDP TFT参数
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             pstTftInfo                 - TFT参数
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_SetTftInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    TAF_TFT_EXT_STRU                   *pstTftInfo
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_SET_TFT_INFO_REQ_STRU        stSetTftInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stSetTftInfoReq, sizeof(stSetTftInfoReq), 0x00, sizeof(TAF_PS_SET_TFT_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_SET_TFT_INFO_REQ消息 */
    stSetTftInfoReq.stCtrl.ulModuleId = ulModuleId;
    stSetTftInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stSetTftInfoReq.stCtrl.ucOpId     = ucOpId;
    stSetTftInfoReq.stTftInfo         = *pstTftInfo;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_SET_TFT_INFO_REQ,
                             &stSetTftInfoReq,
                             sizeof(TAF_PS_SET_TFT_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetTftInfo
 功能描述  : 获取PDP TFT参数
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_GetTftInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_GET_TFT_INFO_REQ_STRU        stGetTftInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stGetTftInfoReq, sizeof(stGetTftInfoReq), 0x00, sizeof(TAF_PS_GET_TFT_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_GET_SEC_PDP_CONTEXT_INFO_REQ消息 */
    stGetTftInfoReq.stCtrl.ulModuleId = ulModuleId;
    stGetTftInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetTftInfoReq.stCtrl.ucOpId     = ucOpId;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_TFT_INFO_REQ,
                             &stGetTftInfoReq,
                             sizeof(TAF_PS_GET_TFT_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_SetUmtsQosInfo
 功能描述  : 设置UMTS QOS参数信息
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             pstUmtsQosInfo             - UMTS QOS参数
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_SetUmtsQosInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    TAF_UMTS_QOS_EXT_STRU              *pstUmtsQosInfo
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_SET_UMTS_QOS_INFO_REQ_STRU   stSetUmtsQosInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stSetUmtsQosInfoReq, sizeof(stSetUmtsQosInfoReq), 0x00, sizeof(TAF_PS_SET_UMTS_QOS_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_SET_UMTS_QOS_INFO_REQ消息 */
    stSetUmtsQosInfoReq.stCtrl.ulModuleId = ulModuleId;
    stSetUmtsQosInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stSetUmtsQosInfoReq.stCtrl.ucOpId     = ucOpId;
    stSetUmtsQosInfoReq.stUmtsQosInfo     = *pstUmtsQosInfo;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_SET_UMTS_QOS_INFO_REQ,
                             &stSetUmtsQosInfoReq,
                             sizeof(TAF_PS_SET_UMTS_QOS_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetUmtsQosInfo
 功能描述  : 获取UMTS QOS参数信息
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_GetUmtsQosInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_GET_UMTS_QOS_INFO_REQ_STRU   stGetUmtsQosInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stGetUmtsQosInfoReq, sizeof(stGetUmtsQosInfoReq), 0x00, sizeof(TAF_PS_GET_UMTS_QOS_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_GET_UMTS_QOS_INFO_REQ消息 */
    stGetUmtsQosInfoReq.stCtrl.ulModuleId = ulModuleId;
    stGetUmtsQosInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetUmtsQosInfoReq.stCtrl.ucOpId     = ucOpId;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_UMTS_QOS_INFO_REQ,
                             &stGetUmtsQosInfoReq,
                             sizeof(TAF_PS_GET_UMTS_QOS_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_SetUmtsQosMinInfo
 功能描述  : 设置UMTS MIN QOS参数信息
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             pstUmtsQosMinInfo          - UMTS MIN QOS参数
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_SetUmtsQosMinInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    TAF_UMTS_QOS_EXT_STRU              *pstUmtsQosMinInfo
)
{
    VOS_UINT32                              ulResult;
    TAF_PS_SET_UMTS_QOS_MIN_INFO_REQ_STRU   stSetUmtsQosMinInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stSetUmtsQosMinInfoReq, sizeof(stSetUmtsQosMinInfoReq), 0x00, sizeof(TAF_PS_SET_UMTS_QOS_MIN_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_SET_UMTS_QOS_MIN_INFO_REQ消息 */
    stSetUmtsQosMinInfoReq.stCtrl.ulModuleId = ulModuleId;
    stSetUmtsQosMinInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stSetUmtsQosMinInfoReq.stCtrl.ucOpId     = ucOpId;
    stSetUmtsQosMinInfoReq.stUmtsQosMinInfo  = *pstUmtsQosMinInfo;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_SET_UMTS_QOS_MIN_INFO_REQ,
                             &stSetUmtsQosMinInfoReq,
                             sizeof(TAF_PS_SET_UMTS_QOS_MIN_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetUmtsQosMinInfo
 功能描述  : 获取UMTS MIN QOS参数信息
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_GetUmtsQosMinInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId
)
{
    VOS_UINT32                              ulResult;
    TAF_PS_GET_UMTS_QOS_MIN_INFO_REQ_STRU   stGetUmtsQosMinInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stGetUmtsQosMinInfoReq, sizeof(stGetUmtsQosMinInfoReq), 0x00, sizeof(TAF_PS_GET_UMTS_QOS_MIN_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_GET_UMTS_QOS_MIN_INFO_REQ消息 */
    stGetUmtsQosMinInfoReq.stCtrl.ulModuleId = ulModuleId;
    stGetUmtsQosMinInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetUmtsQosMinInfoReq.stCtrl.ucOpId     = ucOpId;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_UMTS_QOS_MIN_INFO_REQ,
                             &stGetUmtsQosMinInfoReq,
                             sizeof(TAF_PS_GET_UMTS_QOS_MIN_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetDynamicUmtsQosInfo
 功能描述  : 获取动态UMTS QOS参数信息
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             pstCidListInfo             - CID列表
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_GetDynamicUmtsQosInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    TAF_CID_LIST_STRU                  *pstCidListInfo
)
{
    VOS_UINT32                                  ulResult;
    TAF_PS_GET_DYNAMIC_UMTS_QOS_INFO_REQ_STRU   stGetDynamicUmtsQosInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stGetDynamicUmtsQosInfoReq, sizeof(stGetDynamicUmtsQosInfoReq), 0x00, sizeof(TAF_PS_GET_DYNAMIC_UMTS_QOS_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_GET_DYNAMIC_UMTS_QOS_INFO_REQ消息 */
    stGetDynamicUmtsQosInfoReq.stCtrl.ulModuleId = ulModuleId;
    stGetDynamicUmtsQosInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetDynamicUmtsQosInfoReq.stCtrl.ucOpId     = ucOpId;
    stGetDynamicUmtsQosInfoReq.stCidListInfo     = *pstCidListInfo;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_DYNAMIC_UMTS_QOS_INFO_REQ,
                             &stGetDynamicUmtsQosInfoReq,
                             sizeof(TAF_PS_GET_DYNAMIC_UMTS_QOS_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_SetPdpContextState
 功能描述  : 设置指定CID(表)对应的PDP的状态
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             pstCidListStateInfo        - CID列表状态
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_SetPdpContextState(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    TAF_CID_LIST_STATE_STRU            *pstCidListStateInfo
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_SET_PDP_STATE_REQ_STRU       stSetPdpStateReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stSetPdpStateReq, sizeof(stSetPdpStateReq), 0x00, sizeof(TAF_PS_SET_PDP_STATE_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_SET_PDP_STATE_REQ消息 */
    stSetPdpStateReq.stCtrl.ulModuleId = ulModuleId;
    stSetPdpStateReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stSetPdpStateReq.stCtrl.ucOpId     = ucOpId;
    stSetPdpStateReq.stCidListStateInfo   = *pstCidListStateInfo;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_SET_PDP_CONTEXT_STATE_REQ,
                             &stSetPdpStateReq,
                             sizeof(TAF_PS_SET_PDP_STATE_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetPdpContextState
 功能描述  : 获取所有已定义CID对应的PDP的状态
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_GetPdpContextState(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_GET_PDP_STATE_REQ_STRU       stGetPdpStateReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stGetPdpStateReq, sizeof(stGetPdpStateReq), 0x00, sizeof(TAF_PS_GET_PDP_STATE_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_GET_PDP_STATE_REQ消息 */
    stGetPdpStateReq.stCtrl.ulModuleId = ulModuleId;
    stGetPdpStateReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetPdpStateReq.stCtrl.ucOpId     = ucOpId;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_PDP_CONTEXT_STATE_REQ,
                             &stGetPdpStateReq,
                             sizeof(TAF_PS_GET_PDP_STATE_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_CallModify
 功能描述  : 修改PS CALL参数
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             TAF_CID_LIST_STRU          - CID列表
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_CallModify(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    TAF_CID_LIST_STRU                  *pstCidListInfo
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_CALL_MODIFY_REQ_STRU         stCallModifyReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stCallModifyReq, sizeof(stCallModifyReq), 0x00, sizeof(TAF_PS_CALL_MODIFY_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_CALL_MODIFY_REQ消息 */
    stCallModifyReq.stCtrl.ulModuleId = ulModuleId;
    stCallModifyReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stCallModifyReq.stCtrl.ucOpId     = ucOpId;
    stCallModifyReq.stCidListInfo     = *pstCidListInfo;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_CALL_MODIFY_REQ,
                             &stCallModifyReq,
                             sizeof(TAF_PS_CALL_MODIFY_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_CallAnswer
 功能描述  : 应答PS CALL
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             pstAnsInfo                 - PS CALL应答信息
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_CallAnswer(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    TAF_PS_ANSWER_STRU                 *pstAnsInfo
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_CALL_ANSWER_REQ_STRU         stCallAnswerReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stCallAnswerReq, sizeof(stCallAnswerReq), 0x00, sizeof(TAF_PS_CALL_ANSWER_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_CALL_ANSWER_REQ消息 */
    stCallAnswerReq.stCtrl.ulModuleId = ulModuleId;
    stCallAnswerReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stCallAnswerReq.stCtrl.ucOpId     = ucOpId;
    stCallAnswerReq.stAnsInfo         = *pstAnsInfo;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_CALL_ANSWER_REQ,
                             &stCallAnswerReq,
                             sizeof(TAF_PS_CALL_ANSWER_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_CallHangup
 功能描述  : 挂断PS CALL
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_CallHangup(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_CALL_HANGUP_REQ_STRU         stCallHangupReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stCallHangupReq, sizeof(stCallHangupReq), 0x00, sizeof(TAF_PS_CALL_HANGUP_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_CALL_HANGUP_REQ消息 */
    stCallHangupReq.stCtrl.ulModuleId = ulModuleId;
    stCallHangupReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stCallHangupReq.stCtrl.ucOpId     = ucOpId;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_CALL_HANGUP_REQ,
                             &stCallHangupReq,
                             sizeof(TAF_PS_CALL_HANGUP_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_CallOrig
 功能描述  : 发起PS CALL
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             pstDialParaInfo            - PS CALL参数
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

  2.日    期   : 2016年02月19日
    作    者   : w00316404
    修改内容   : Modify for DTS2016021808321

*****************************************************************************/
VOS_UINT32 TAF_PS_CallOrig(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    TAF_PS_DIAL_PARA_STRU              *pstDialParaInfo
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_CALL_ORIG_REQ_STRU           stCallOrigReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stCallOrigReq, sizeof(stCallOrigReq), 0x00, sizeof(TAF_PS_CALL_ORIG_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_CALL_ORIG_REQ消息 */
    stCallOrigReq.stCtrl.ulModuleId = ulModuleId;
    stCallOrigReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stCallOrigReq.stCtrl.ucOpId     = ucOpId;

    TAF_MEM_CPY_S(&stCallOrigReq.stDialParaInfo,
               sizeof(stCallOrigReq.stDialParaInfo),
               pstDialParaInfo,
               sizeof(TAF_PS_DIAL_PARA_STRU));

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_CALL_ORIG_REQ,
                             &stCallOrigReq,
                             sizeof(TAF_PS_CALL_ORIG_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_CallEnd
 功能描述  : 结束PS CALL
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             ucCid                      - CID
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数
  2.日    期   : 2016年02月19日
    作    者   : w00316404
    修改内容   : Modify for DTS2016021808321
  3.日    期   : 2017年02月05日
    作    者   : w00316404
    修改内容   : do recovery requirement
*****************************************************************************/
VOS_UINT32 TAF_PS_CallEnd(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    VOS_UINT8                           ucCid
)
{
    return TAF_PS_CallEndEx(ulModuleId, usExClientId, ucOpId, ucCid, TAF_PS_CALL_END_CAUSE_NORMAL);
}

/*****************************************************************************
 函 数 名  : TAF_PS_CallEndEx
 功能描述  : 结束PS CALL
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             ucCid                      - CID
             enCause                    - 去激活的原因
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年02月06日
    作    者   : w00316404
    修改内容   : 新生成函数
*****************************************************************************/
VOS_UINT32 TAF_PS_CallEndEx(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    VOS_UINT8                           ucCid,
    TAF_PS_CALL_END_CAUSE_ENUM_UINT8    enCause
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_CALL_END_REQ_STRU            stCallEndReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stCallEndReq, sizeof(stCallEndReq), 0x00, sizeof(TAF_PS_CALL_END_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_CALL_END_REQ消息 */
    stCallEndReq.stCtrl.ulModuleId = ulModuleId;
    stCallEndReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stCallEndReq.stCtrl.ucOpId     = ucOpId;
    stCallEndReq.ucCid             = ucCid;
    stCallEndReq.enCause           = enCause;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_CALL_END_REQ,
                             &stCallEndReq,
                             sizeof(TAF_PS_CALL_END_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetPdpIpAddrInfo
 功能描述  : 获取指定CID(表)的PDP IP地址信息
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             pstCidListInfo             - CID列表
输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_GetPdpIpAddrInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    TAF_CID_LIST_STRU                  *pstCidListInfo
)
{
    VOS_UINT32                              ulResult;
    TAF_PS_GET_PDP_IP_ADDR_INFO_REQ_STRU    stGetPdpIpAddrReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stGetPdpIpAddrReq, sizeof(stGetPdpIpAddrReq), 0x00, sizeof(TAF_PS_GET_PDP_IP_ADDR_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_GET_PDP_IP_ADDR_INFO_REQ消息 */
    stGetPdpIpAddrReq.stCtrl.ulModuleId = ulModuleId;
    stGetPdpIpAddrReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetPdpIpAddrReq.stCtrl.ucOpId     = ucOpId;
    stGetPdpIpAddrReq.stCidListInfo     = *pstCidListInfo;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_PDP_IP_ADDR_INFO_REQ,
                             &stGetPdpIpAddrReq,
                             sizeof(TAF_PS_GET_PDP_IP_ADDR_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_SetAnsModeInfo
 功能描述  : 设置PS域呼叫应答模式信息
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             ulAnsMode                  - 应答模式信息
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_SetAnsModeInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    VOS_UINT32                          ulAnsMode
)
{
    VOS_UINT32                              ulResult;
    TAF_PS_SET_ANSWER_MODE_INFO_REQ_STRU    stSetAnsModeReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stSetAnsModeReq, sizeof(stSetAnsModeReq), 0x00, sizeof(TAF_PS_SET_ANSWER_MODE_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_SET_ANSWER_MODE_INFO_REQ消息 */
    stSetAnsModeReq.stCtrl.ulModuleId = ulModuleId;
    stSetAnsModeReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stSetAnsModeReq.stCtrl.ucOpId     = ucOpId;
    stSetAnsModeReq.ulAnsMode         = ulAnsMode;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_SET_ANSWER_MODE_INFO_REQ,
                             &stSetAnsModeReq,
                             sizeof(TAF_PS_SET_ANSWER_MODE_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetAnsModeInfo
 功能描述  : 获取PS域呼叫应答模式信息
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_GetAnsModeInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId
)
{
    VOS_UINT32                              ulResult;
    TAF_PS_GET_ANSWER_MODE_INFO_REQ_STRU    stGetAnsModeReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stGetAnsModeReq, sizeof(stGetAnsModeReq), 0x00, sizeof(TAF_PS_GET_ANSWER_MODE_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_SET_ANSWER_MODE_INFO_REQ消息 */
    stGetAnsModeReq.stCtrl.ulModuleId   = ulModuleId;
    stGetAnsModeReq.stCtrl.usClientId   = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetAnsModeReq.stCtrl.ucOpId       = ucOpId;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_ANSWER_MODE_INFO_REQ,
                             &stGetAnsModeReq,
                             sizeof(TAF_PS_GET_ANSWER_MODE_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetDynamicPrimPdpContextInfo
 功能描述  : 获取指定的已定义CID的动态Primary PDP上下文信息
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             ucCid                      - CID
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_GetDynamicPrimPdpContextInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    VOS_UINT8                           ucCid
)
{
    VOS_UINT32                                          ulResult;
    TAF_PS_GET_DYNAMIC_PRIM_PDP_CONTEXT_INFO_REQ_STRU   stGetDynamicPrimPdpCtxInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stGetDynamicPrimPdpCtxInfoReq, sizeof(stGetDynamicPrimPdpCtxInfoReq), 0x00, sizeof(TAF_PS_GET_DYNAMIC_PRIM_PDP_CONTEXT_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_GET_DYNAMIC_PRIM_PDP_CONTEXT_INFO_REQ消息 */
    stGetDynamicPrimPdpCtxInfoReq.stCtrl.ulModuleId = ulModuleId;
    stGetDynamicPrimPdpCtxInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetDynamicPrimPdpCtxInfoReq.stCtrl.ucOpId     = ucOpId;
    stGetDynamicPrimPdpCtxInfoReq.ucCid             = ucCid;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_DYNAMIC_PRIM_PDP_CONTEXT_INFO_REQ,
                             &stGetDynamicPrimPdpCtxInfoReq,
                             sizeof(TAF_PS_GET_DYNAMIC_PRIM_PDP_CONTEXT_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetDynamicSecPdpContextInfo
 功能描述  : 获取指定的已定义CID的动态Secondary PDP上下文信息
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             ucCid                      - CID
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_GetDynamicSecPdpContextInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    VOS_UINT8                           ucCid
)
{
    VOS_UINT32                                          ulResult;
    TAF_PS_GET_DYNAMIC_SEC_PDP_CONTEXT_INFO_REQ_STRU    stGetDynamicSecPdpCtxInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stGetDynamicSecPdpCtxInfoReq, sizeof(stGetDynamicSecPdpCtxInfoReq), 0x00, sizeof(TAF_PS_GET_DYNAMIC_SEC_PDP_CONTEXT_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_GET_DYNAMIC_SEC_PDP_CONTEXT_INFO_REQ消息 */
    stGetDynamicSecPdpCtxInfoReq.stCtrl.ulModuleId = ulModuleId;
    stGetDynamicSecPdpCtxInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetDynamicSecPdpCtxInfoReq.stCtrl.ucOpId     = ucOpId;
    stGetDynamicSecPdpCtxInfoReq.ucCid             = ucCid;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_DYNAMIC_SEC_PDP_CONTEXT_INFO_REQ,
                             &stGetDynamicSecPdpCtxInfoReq,
                             sizeof(TAF_PS_GET_DYNAMIC_SEC_PDP_CONTEXT_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetDynamicTftInfo
 功能描述  : 获取指定的已定义CID的动态TFT信息
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             ucCid                      - CID
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_GetDynamicTftInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    VOS_UINT8                           ucCid
)
{
    VOS_UINT32                              ulResult;
    TAF_PS_GET_DYNAMIC_TFT_INFO_REQ_STRU    stGetDynamicTftInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stGetDynamicTftInfoReq, sizeof(stGetDynamicTftInfoReq), 0x00, sizeof(TAF_PS_GET_DYNAMIC_TFT_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_GET_DYNAMIC_TFT_INFO_REQ消息 */
    stGetDynamicTftInfoReq.stCtrl.ulModuleId = ulModuleId;
    stGetDynamicTftInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetDynamicTftInfoReq.stCtrl.ucOpId     = ucOpId;
    stGetDynamicTftInfoReq.ucCid             = ucCid;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_DYNAMIC_TFT_INFO_REQ,
                             &stGetDynamicTftInfoReq,
                             sizeof(TAF_PS_GET_DYNAMIC_TFT_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_SetEpsQosInfo
 功能描述  : 设置指定CID的EPS QOS参数信息
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             pstEpsQosInfo              - EPS QOS参数信息
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_SetEpsQosInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    TAF_EPS_QOS_EXT_STRU               *pstEpsQosInfo
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_SET_EPS_QOS_INFO_REQ_STRU    stSetEpsQosInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stSetEpsQosInfoReq, sizeof(stSetEpsQosInfoReq), 0x00, sizeof(TAF_PS_SET_EPS_QOS_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_SET_EPS_QOS_INFO_REQ消息 */
    stSetEpsQosInfoReq.stCtrl.ulModuleId = ulModuleId;
    stSetEpsQosInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stSetEpsQosInfoReq.stCtrl.ucOpId     = ucOpId;
    stSetEpsQosInfoReq.stEpsQosInfo      = *pstEpsQosInfo;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_SET_EPS_QOS_INFO_REQ,
                             &stSetEpsQosInfoReq,
                             sizeof(TAF_PS_SET_EPS_QOS_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetEpsQosInfo
 功能描述  : 获取EPS QOS参数信息
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_GetEpsQosInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_GET_EPS_QOS_INFO_REQ_STRU    stGetEpsQosInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stGetEpsQosInfoReq, sizeof(stGetEpsQosInfoReq), 0x00, sizeof(TAF_PS_GET_EPS_QOS_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_GET_EPS_QOS_INFO_REQ消息 */
    stGetEpsQosInfoReq.stCtrl.ulModuleId = ulModuleId;
    stGetEpsQosInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetEpsQosInfoReq.stCtrl.ucOpId     = ucOpId;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_EPS_QOS_INFO_REQ,
                             &stGetEpsQosInfoReq,
                             sizeof(TAF_PS_GET_EPS_QOS_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetDynamicEpsQosInfo
 功能描述  : 获取指定的已定义CID的动态EPS QOS参数
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             ucCid                      - CID
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_GetDynamicEpsQosInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    VOS_UINT8                           ucCid
)
{
    VOS_UINT32                                  ulResult;
    TAF_PS_GET_DYNAMIC_EPS_QOS_INFO_REQ_STRU    stGetDynamicEpsQosInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stGetDynamicEpsQosInfoReq, sizeof(stGetDynamicEpsQosInfoReq), 0x00, sizeof(TAF_PS_GET_DYNAMIC_EPS_QOS_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_GET_DYNAMIC_EPS_QOS_INFO_REQ消息 */
    stGetDynamicEpsQosInfoReq.stCtrl.ulModuleId = ulModuleId;
    stGetDynamicEpsQosInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetDynamicEpsQosInfoReq.stCtrl.ucOpId     = ucOpId;
    stGetDynamicEpsQosInfoReq.ucCid             = ucCid;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_DYNAMIC_EPS_QOS_INFO_REQ,
                             &stGetDynamicEpsQosInfoReq,
                             sizeof(TAF_PS_GET_DYNAMIC_EPS_QOS_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetDsFlowInfo
 功能描述  : 获取数据流量信息
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             pstQueryConfigInfo         - DSFLOW查询配置参数
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_GetDsFlowInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_GET_DSFLOW_INFO_REQ_STRU     stGetDsFlowInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stGetDsFlowInfoReq, sizeof(stGetDsFlowInfoReq), 0x00, sizeof(TAF_PS_GET_DSFLOW_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_GET_DSFLOW_INFO_REQ消息 */
    stGetDsFlowInfoReq.stCtrl.ulModuleId = ulModuleId;
    stGetDsFlowInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetDsFlowInfoReq.stCtrl.ucOpId     = ucOpId;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_DSFLOW_INFO_REQ,
                             &stGetDsFlowInfoReq,
                             sizeof(TAF_PS_GET_DSFLOW_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_ClearDsFlowInfo
 功能描述  : 清除数据流量信息
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             pstClearConfigInfo         - 流量清除配置参数信息
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_ClearDsFlowInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    TAF_DSFLOW_CLEAR_CONFIG_STRU       *pstClearConfigInfo
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_CLEAR_DSFLOW_REQ_STRU        stClearDsFlowReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stClearDsFlowReq, sizeof(stClearDsFlowReq), 0x00, sizeof(TAF_PS_CLEAR_DSFLOW_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_CLEAR_DSFLOW_REQ消息 */
    stClearDsFlowReq.stCtrl.ulModuleId = ulModuleId;
    stClearDsFlowReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stClearDsFlowReq.stCtrl.ucOpId     = ucOpId;
    stClearDsFlowReq.stClearConfigInfo = *pstClearConfigInfo;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_CLEAR_DSFLOW_REQ,
                             &stClearDsFlowReq,
                             sizeof(TAF_PS_CLEAR_DSFLOW_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_ConfigDsFlowRpt
 功能描述  : 配置流量上报模式
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             pstReportConfigInfo        - 流量上报配置参数信息
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_ConfigDsFlowRpt(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    TAF_DSFLOW_REPORT_CONFIG_STRU      *pstReportConfigInfo
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_CONFIG_DSFLOW_RPT_REQ_STRU   stConfigDsFlowRptReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stConfigDsFlowRptReq, sizeof(stConfigDsFlowRptReq), 0x00, sizeof(TAF_PS_CONFIG_DSFLOW_RPT_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_CONFIG_DSFLOW_RPT_REQ消息 */
    stConfigDsFlowRptReq.stCtrl.ulModuleId = ulModuleId;
    stConfigDsFlowRptReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stConfigDsFlowRptReq.stCtrl.ucOpId     = ucOpId;
    stConfigDsFlowRptReq.stReportConfigInfo   = *pstReportConfigInfo;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_CONFIG_DSFLOW_RPT_REQ,
                             &stConfigDsFlowRptReq,
                             sizeof(TAF_PS_CONFIG_DSFLOW_RPT_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_SetPdpDnsInfo
 功能描述  : 设置PDP DNS信息
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             pstPdpDnsInfo              - PDP DNS信息
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_SetPdpDnsInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    TAF_PDP_DNS_EXT_STRU               *pstPdpDnsInfo
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_SET_PDP_DNS_INFO_REQ_STRU    stSetPdpDnsInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stSetPdpDnsInfoReq, sizeof(stSetPdpDnsInfoReq), 0x00, sizeof(TAF_PS_SET_PDP_DNS_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_SET_PDP_DNS_INFO_REQ消息 */
    stSetPdpDnsInfoReq.stCtrl.ulModuleId = ulModuleId;
    stSetPdpDnsInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stSetPdpDnsInfoReq.stCtrl.ucOpId     = ucOpId;
    stSetPdpDnsInfoReq.stPdpDnsInfo      = *pstPdpDnsInfo;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_SET_PDP_DNS_INFO_REQ,
                             &stSetPdpDnsInfoReq,
                             sizeof(TAF_PS_SET_PDP_DNS_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetPdpDnsInfo
 功能描述  : 获取PDP DNS信息
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_GetPdpDnsInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_GET_PDP_DNS_INFO_REQ_STRU    stGetPdpDnsInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stGetPdpDnsInfoReq, sizeof(stGetPdpDnsInfoReq), 0x00, sizeof(TAF_PS_GET_PDP_DNS_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_GET_PDP_DNS_INFO_REQ消息 */
    stGetPdpDnsInfoReq.stCtrl.ulModuleId = ulModuleId;
    stGetPdpDnsInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetPdpDnsInfoReq.stCtrl.ucOpId     = ucOpId;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_PDP_DNS_INFO_REQ,
                             &stGetPdpDnsInfoReq,
                             sizeof(TAF_PS_GET_PDP_DNS_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_TrigGprsData
 功能描述  : 发送上行GPRS数据
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             pstGprsDataInfo            - GPRS数据信息
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_TrigGprsData(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    TAF_GPRS_DATA_STRU                 *pstGprsDataInfo
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_TRIG_GPRS_DATA_REQ_STRU      stTrigGprsDataReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stTrigGprsDataReq, sizeof(stTrigGprsDataReq), 0x00, sizeof(TAF_PS_TRIG_GPRS_DATA_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_TRIG_GPRS_DATA_REQ消息 */
    stTrigGprsDataReq.stCtrl.ulModuleId = ulModuleId;
    stTrigGprsDataReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stTrigGprsDataReq.stCtrl.ucOpId     = ucOpId;
    stTrigGprsDataReq.stGprsDataInfo    = *pstGprsDataInfo;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_TRIG_GPRS_DATA_REQ,
                             &stTrigGprsDataReq,
                             sizeof(TAF_PS_TRIG_GPRS_DATA_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_ConfigNbnsFunction
 功能描述  : 配置NBNS功能
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             ulEnabled                  - NBNS功能使能标记
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_ConfigNbnsFunction(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    VOS_UINT32                          ulEnabled
)
{
    VOS_UINT32                              ulResult;
    TAF_PS_CONFIG_NBNS_FUNCTION_REQ_STRU    stConfigNbnsFunReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stConfigNbnsFunReq, sizeof(stConfigNbnsFunReq), 0x00, sizeof(TAF_PS_CONFIG_NBNS_FUNCTION_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_CONFIG_NBNS_FUNCTION_REQ消息 */
    stConfigNbnsFunReq.stCtrl.ulModuleId = ulModuleId;
    stConfigNbnsFunReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stConfigNbnsFunReq.stCtrl.ucOpId     = ucOpId;
    stConfigNbnsFunReq.ulEnabled         = ulEnabled;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_CONFIG_NBNS_FUNCTION_REQ,
                             &stConfigNbnsFunReq,
                             sizeof(TAF_PS_CONFIG_NBNS_FUNCTION_REQ_STRU));

    return ulResult;
}

/* Deleted by Y00213812 for VoLTE_PhaseI 项目, 2013-07-08, begin */
/* Deleted by Y00213812 for VoLTE_PhaseI 项目, 2013-07-08, end */

/*****************************************************************************
 函 数 名  : TAF_PS_SetAuthDataInfo
 功能描述  : 设置鉴权参数信息(NDIS)
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             pstAuthDataInfo            - 鉴权参数
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_SetAuthDataInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    TAF_AUTHDATA_EXT_STRU              *pstAuthDataInfo
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_SET_AUTHDATA_INFO_REQ_STRU   stSetAuthDataInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stSetAuthDataInfoReq, sizeof(stSetAuthDataInfoReq), 0x00, sizeof(TAF_PS_SET_AUTHDATA_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_SET_AUTHDATA_INFO_REQ消息 */
    stSetAuthDataInfoReq.stCtrl.ulModuleId = ulModuleId;
    stSetAuthDataInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stSetAuthDataInfoReq.stCtrl.ucOpId     = ucOpId;
    stSetAuthDataInfoReq.stAuthDataInfo    = *pstAuthDataInfo;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_SET_AUTHDATA_INFO_REQ,
                             &stSetAuthDataInfoReq,
                             sizeof(TAF_PS_SET_AUTHDATA_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetAuthDataInfo
 功能描述  : 获取鉴权参数信息(NDIS)
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_GetAuthDataInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_GET_AUTHDATA_INFO_REQ_STRU   stGetAuthDataInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stGetAuthDataInfoReq, sizeof(stGetAuthDataInfoReq), 0x00, sizeof(TAF_PS_GET_AUTHDATA_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_GET_AUTHDATA_INFO_REQ消息 */
    stGetAuthDataInfoReq.stCtrl.ulModuleId = ulModuleId;
    stGetAuthDataInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetAuthDataInfoReq.stCtrl.ucOpId     = ucOpId;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_AUTHDATA_INFO_REQ,
                             &stGetAuthDataInfoReq,
                             sizeof(TAF_PS_GET_AUTHDATA_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetGprsActiveType
 功能描述  : 获取D命令GPRS类型(PPP拨号)
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             pstAtdPara                 - D命令拨号参数
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_GetGprsActiveType(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    TAF_ATD_PARA_STRU                  *pstAtdPara
)
{
    VOS_UINT32                              ulResult;
    TAF_PS_GET_D_GPRS_ACTIVE_TYPE_REQ_STRU  stGetGprsActiveTypeReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stGetGprsActiveTypeReq, sizeof(stGetGprsActiveTypeReq), 0x00, sizeof(TAF_PS_GET_D_GPRS_ACTIVE_TYPE_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_GET_AUTHDATA_INFO_REQ消息 */
    stGetGprsActiveTypeReq.stCtrl.ulModuleId = ulModuleId;
    stGetGprsActiveTypeReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetGprsActiveTypeReq.stCtrl.ucOpId     = ucOpId;
    stGetGprsActiveTypeReq.stAtdPara         = *pstAtdPara;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_D_GPRS_ACTIVE_TYPE_REQ,
                             &stGetGprsActiveTypeReq,
                             sizeof(TAF_PS_GET_D_GPRS_ACTIVE_TYPE_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_PppDialOrig
 功能描述  : 发起PPP拨号
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             ucCid                      - CID
             pstPppReqConfigInfo        - PPP配置参数
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年10月5日
    作    者   : A00165503
    修改内容   : 新生成函数

  2.日    期   : 2014年5月16日
    作    者   : A00165503
    修改内容   : DTS2014050703206: PPP拨号接口调整
*****************************************************************************/
VOS_UINT32 TAF_PS_PppDialOrig(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    VOS_UINT8                           ucCid,
    TAF_PPP_REQ_CONFIG_INFO_STRU       *pstPppReqConfigInfo
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_PPP_DIAL_ORIG_REQ_STRU       stPppDialOrigReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stPppDialOrigReq, sizeof(stPppDialOrigReq), 0x00, sizeof(TAF_PS_PPP_DIAL_ORIG_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_PPP_DIAL_ORIG_REQ消息 */
    stPppDialOrigReq.stCtrl.ulModuleId          = ulModuleId;
    stPppDialOrigReq.stCtrl.usClientId          = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stPppDialOrigReq.stCtrl.ucOpId              = ucOpId;

    stPppDialOrigReq.stPppDialParaInfo.ucCid    = ucCid;

    stPppDialOrigReq.stPppDialParaInfo.bitOpPppConfigInfo = VOS_TRUE;
    TAF_MEM_CPY_S(&(stPppDialOrigReq.stPppDialParaInfo.stPppReqConfigInfo),
               sizeof(stPppDialOrigReq.stPppDialParaInfo.stPppReqConfigInfo),
               pstPppReqConfigInfo,
               sizeof(TAF_PPP_REQ_CONFIG_INFO_STRU));

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_PPP_DIAL_ORIG_REQ,
                             &stPppDialOrigReq,
                             sizeof(TAF_PS_PPP_DIAL_ORIG_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
函 数 名  : TAF_PS_GetLteCsInfo
功能描述  : 获取LTE CS Info
输入参数  : 无
输出参数  : 无
返 回 值  : VOS_OK                     - 发送消息成功
            VOS_ERR                    - 发送消息失败
调用函数  :
被调函数  :

修改历史      :
 1.日    期   : 2012年02月23日
   作    者   : h00135900
   修改内容   : 新生成函数
*****************************************************************************/
VOS_UINT32 TAF_PS_GetLteCsInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId

)
{
    VOS_UINT32                                  ulResult;
    TAF_PS_LTECS_REQ_STRU   stGetLteCsInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stGetLteCsInfoReq, sizeof(stGetLteCsInfoReq), 0x00, sizeof(TAF_PS_LTECS_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_SET_PRIM_PDP_CONTEXT_INFO_REQ消息 */
    stGetLteCsInfoReq.stCtrl.ulModuleId = ulModuleId;
    stGetLteCsInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetLteCsInfoReq.stCtrl.ucOpId     = ucOpId;


    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_LTE_CS_REQ,
                             &stGetLteCsInfoReq,
                             sizeof(TAF_PS_LTECS_REQ_STRU));


    return ulResult;
}

/*****************************************************************************
函 数 名  : TAF_PS_GetCemodeInfo
功能描述  : 获取CEMODE Info
输入参数  : 无
输出参数  : 无
返 回 值  : VOS_OK                     - 发送消息成功
            VOS_ERR                    - 发送消息失败
调用函数  :
被调函数  :

修改历史      :
 1.日    期   : 2012年03月20日
   作    者   : w00182550
   修改内容   : 新生成函数
*****************************************************************************/
VOS_UINT32 TAF_PS_GetCemodeInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId
)
{
    VOS_UINT32                                  ulResult;
    TAF_PS_CEMODE_REQ_STRU   stGetCemodeInfoReq;



    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stGetCemodeInfoReq, sizeof(stGetCemodeInfoReq), 0x00, sizeof(TAF_PS_CEMODE_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_SET_PRIM_PDP_CONTEXT_INFO_REQ消息 */
    stGetCemodeInfoReq.stCtrl.ulModuleId = ulModuleId;
    stGetCemodeInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetCemodeInfoReq.stCtrl.ucOpId     = ucOpId;


    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_CEMODE_REQ,
                             &stGetCemodeInfoReq,
                             sizeof(TAF_PS_CEMODE_REQ_STRU));


    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_SetPdpProfInfo
 功能描述  : 设置PDP_Profile 参数
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             stPdpProfInfo            - PDP_Profile 参数
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
 日    期   : 2012年3月12日
 作    者   : x00126983
 修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_SetPdpProfInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    TAF_PDP_PROFILE_EXT_STRU           *stPdpProfInfo
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_SET_PROFILE_INFO_REQ_STRU    stSetPdpProfInfoReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stSetPdpProfInfoReq, sizeof(stSetPdpProfInfoReq), 0x00, sizeof(TAF_PS_SET_PROFILE_INFO_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_SET_AUTHDATA_INFO_REQ消息 */
    stSetPdpProfInfoReq.stCtrl.ulModuleId = ulModuleId;
    stSetPdpProfInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stSetPdpProfInfoReq.stCtrl.ucOpId     = ucOpId;
    stSetPdpProfInfoReq.stPdpProfInfo     = *stPdpProfInfo;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_SET_PDPROFMOD_INFO_REQ,
                             &stSetPdpProfInfoReq,
                             sizeof(TAF_PS_SET_PROFILE_INFO_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetCidSdfParaInfo
 功能描述  : 获取所有NV项中的SDF配置信息, 支持同步和异步两种方式
             输出参数非空 --- 同步方式(目前仅支持C核)
             输出参数为空 --- 异步方式
 输入参数  : ulModuleId                 - 填写PID
             usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
 输出参数  : pstSdfQueryInfo            - SDF配置信息
 返 回 值  : VOS_OK                     - 成功
             VOS_ERR                    - 失败

修改历史  :

1.日    期   : 2013年07月08日
  作    者   : Y00213812
  修改内容   : VoLTE_PhaseI 项目，新增API

*****************************************************************************/
VOS_UINT32 TAF_PS_GetCidSdfParaInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    TAF_SDF_PARA_QUERY_INFO_STRU       *pstSdfQueryInfo
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_SDF_INFO_REQ_STRU            stSdfInfoReq;

    ulResult = VOS_ERR;
    TAF_MEM_SET_S(&stSdfInfoReq, sizeof(stSdfInfoReq), 0x00, sizeof(TAF_PS_SDF_INFO_REQ_STRU));

    /* 如果出参为空，则采用异步方式获取SDF信息 */
    if (VOS_NULL_PTR == pstSdfQueryInfo)
    {
        /* 构造ID_MSG_TAF_GET_CID_SDF_REQ消息 */
        stSdfInfoReq.stCtrl.ulModuleId = ulModuleId;
        stSdfInfoReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
        stSdfInfoReq.stCtrl.ucOpId     = ucOpId;

        /* 发送消息 */
        ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                                 ID_MSG_TAF_PS_GET_CID_SDF_REQ,
                                 &stSdfInfoReq,
                                 sizeof(TAF_PS_SDF_INFO_REQ_STRU));

        return ulResult;
    }

    TAF_MEM_SET_S(pstSdfQueryInfo, sizeof(TAF_SDF_PARA_QUERY_INFO_STRU), 0x00, sizeof(TAF_SDF_PARA_QUERY_INFO_STRU));
/* 同步方式目前仅支持C核 */

    return ulResult;
}


/*****************************************************************************
 函 数 名  : TAF_PS_GetUnusedCid
 功能描述  : 获取空闲未激活的CID
             输出参数非空 --- 同步方式(目前仅支持C核)
             输出参数为空 --- 异步方式
 输入参数  : ulModuleId                 - 填写PID
             usClientId                 - 客户端ID
             ucOpId                     - 操作码ID
 输出参数  : puCid                      - 空闲未激活CID
 返 回 值  : VOS_OK                     - 成功
             VOS_ERR                    - 失败

 修改历史  :

 1.日    期   : 2013年07月08日
   作    者   : Y00213812
   修改内容   : VoLTE_PhaseI 项目，新增API
*****************************************************************************/
VOS_UINT32 TAF_PS_GetUnusedCid(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    VOS_UINT8                          *puCid
)
{
    VOS_UINT32                          ulResult;

    ulResult = VOS_OK;

    if (VOS_NULL_PTR == puCid)
    {
        /* 目前暂不支持异步方式 */
        return VOS_ERR;
    }

/* 同步方式目前仅支持C核 */

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetDynamicDnsInfo
 功能描述  : 获取指定CID的DNS信息
 输入参数  : ulModuleId                 - 填写PID
             usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             ucCid                      - CID
 输出参数  : 无

 返 回 值  : VOS_OK                     - 成功
             VOS_ERR                    - 失败
 修改历史  :

 1.日    期   : 2013年07月08日
   作    者   : Y00213812
   修改内容   : VoLTE_PhaseI 项目，新增API

*****************************************************************************/
VOS_UINT32 TAF_PS_GetDynamicDnsInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    VOS_UINT8                           ucCid
)
{
    VOS_UINT32                          ulResult;

    TAF_PS_GET_NEGOTIATION_DNS_REQ_STRU stNegoDns;

    /* 初始化 */
    TAF_MEM_SET_S(&stNegoDns, sizeof(stNegoDns), 0x00, sizeof(TAF_PS_GET_NEGOTIATION_DNS_REQ_STRU));

    /* 填充消息结构 */
    stNegoDns.stCtrl.ulModuleId = ulModuleId;
    stNegoDns.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stNegoDns.stCtrl.ucOpId     = ucOpId;
    stNegoDns.ucCid             = ucCid;

    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_NEGOTIATION_DNS_REQ,
                             &stNegoDns,
                             sizeof(TAF_PS_GET_NEGOTIATION_DNS_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_SetCqosPriInfo
 功能描述  : 配置CDMA QOS 参数
 输入参数  : VOS_UINT32                                      ulModuleId
             usExClientId                                  - 扩展客户端ID
                                                             A核 : ModemID(高四bit) + 客户端ID
                                                             C核 : 客户端ID
             TAF_PS_CDATA_1X_QOS_NON_ASSURED_PRI_ENUM_UINT8  enPri
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年10月16日
    作    者   : y00218312
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_SetCqosPriInfo(
    VOS_UINT32                                      ulModuleId,
    VOS_UINT16                                      usExClientId,
    TAF_PS_CDATA_1X_QOS_NON_ASSURED_PRI_ENUM_UINT8  enPri
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_SET_1X_CQOS_PRI_REQ_STRU     stSetCqosPriReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stSetCqosPriReq, sizeof(stSetCqosPriReq), 0x00, sizeof(TAF_PS_SET_1X_CQOS_PRI_REQ_STRU));

    /* 构造TAF_PS_SET_CQOS_PRI_REQ_STRU消息 */
    stSetCqosPriReq.stCtrl.ulModuleId = ulModuleId;
    stSetCqosPriReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stSetCqosPriReq.stCtrl.ucOpId     = 0;
    stSetCqosPriReq.enPri             = enPri;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_SET_CQOS_PRI_REQ,
                             &stSetCqosPriReq,
                             sizeof(TAF_PS_SET_1X_CQOS_PRI_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_SetApDsFlowRptCfg
 功能描述  : 设置AP流量上报参数
 输入参数  : ulModuleId                 - PID
             usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             pstRptCfg                  - 配置参数
 输出参数  : 无
 返 回 值  : VOS_OK                     - 成功
             VOS_ERR                    - 失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年2月2日
    作    者   : A00165503
    修改内容   : 新生成函数
*****************************************************************************/
VOS_UINT32 TAF_PS_SetApDsFlowRptCfg(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    TAF_APDSFLOW_RPT_CFG_STRU          *pstRptCfg
)
{
    TAF_PS_SET_APDSFLOW_RPT_CFG_REQ_STRU    stSetRptCfgReq;
    VOS_UINT32                              ulResult;

    TAF_MEM_SET_S(&stSetRptCfgReq, sizeof(stSetRptCfgReq), 0x00, sizeof(TAF_PS_SET_APDSFLOW_RPT_CFG_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_SET_APDSFLOW_RPT_CFG_REQ消息 */
    stSetRptCfgReq.stCtrl.ulModuleId = ulModuleId;
    stSetRptCfgReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stSetRptCfgReq.stCtrl.ucOpId     = ucOpId;

    TAF_MEM_CPY_S(&(stSetRptCfgReq.stRptCfg),
               sizeof(stSetRptCfgReq.stRptCfg),
               pstRptCfg,
               sizeof(TAF_APDSFLOW_RPT_CFG_STRU));

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_SET_APDSFLOW_RPT_CFG_REQ,
                             &stSetRptCfgReq,
                             sizeof(TAF_PS_SET_APDSFLOW_RPT_CFG_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetApDsFlowRptCfg
 功能描述  : 获取AP流量上报参数
 输入参数  : ulModuleId                 - PID
             usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
 输出参数  : 无
 返 回 值  : VOS_OK                     - 成功
             VOS_ERR                    - 失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年2月2日
    作    者   : A00165503
    修改内容   : 新生成函数
*****************************************************************************/
VOS_UINT32 TAF_PS_GetApDsFlowRptCfg(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_PS_GET_APDSFLOW_RPT_CFG_REQ_STRU    stGetRptCfgReq;
    VOS_UINT32                              ulResult;

    TAF_MEM_SET_S(&stGetRptCfgReq, sizeof(stGetRptCfgReq), 0x00, sizeof(TAF_PS_GET_APDSFLOW_RPT_CFG_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_GET_APDSFLOW_RPT_CFG_REQ消息 */
    stGetRptCfgReq.stCtrl.ulModuleId = ulModuleId;
    stGetRptCfgReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetRptCfgReq.stCtrl.ucOpId     = ucOpId;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_APDSFLOW_RPT_CFG_REQ,
                             &stGetRptCfgReq,
                             sizeof(TAF_PS_GET_APDSFLOW_RPT_CFG_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_SetDsFlowNvWriteCfg
 功能描述  : 设置流量写NV配置
 输入参数  : ulModuleId                 - PID
             usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             pstNvWriteCfg              - 流量写NV配置
 输出参数  : 无
 返 回 值  : VOS_OK                     - 成功
             VOS_ERR                    - 失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年2月12日
    作    者   : A00165503
    修改内容   : 新生成函数
*****************************************************************************/
VOS_UINT32 TAF_PS_SetDsFlowNvWriteCfg(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    TAF_DSFLOW_NV_WRITE_CFG_STRU       *pstNvWriteCfg
)
{
    TAF_PS_SET_DSFLOW_NV_WRITE_CFG_REQ_STRU stSetWriteNvCfgReq;
    VOS_UINT32                              ulResult;

    TAF_MEM_SET_S(&stSetWriteNvCfgReq, sizeof(stSetWriteNvCfgReq), 0x00, sizeof(TAF_PS_SET_DSFLOW_NV_WRITE_CFG_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_SET_DSFLOW_NV_WRITE_CFG_REQ消息 */
    stSetWriteNvCfgReq.stCtrl.ulModuleId = ulModuleId;
    stSetWriteNvCfgReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stSetWriteNvCfgReq.stCtrl.ucOpId     = ucOpId;

    TAF_MEM_CPY_S(&(stSetWriteNvCfgReq.stNvWriteCfg),
               sizeof(stSetWriteNvCfgReq.stNvWriteCfg),
               pstNvWriteCfg,
               sizeof(TAF_DSFLOW_NV_WRITE_CFG_STRU));

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_SET_DSFLOW_NV_WRITE_CFG_REQ,
                             &stSetWriteNvCfgReq,
                             sizeof(TAF_PS_SET_DSFLOW_NV_WRITE_CFG_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetDsFlowNvWriteCfg
 功能描述  : 获取流量写NV配置
 输入参数  : ulModuleId                 - PID
             usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
 输出参数  : 无
 返 回 值  : VOS_OK                     - 成功
             VOS_ERR                    - 失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年2月12日
    作    者   : A00165503
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_GetDsFlowNvWriteCfg(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_PS_GET_DSFLOW_NV_WRITE_CFG_REQ_STRU stGetNvWriteCfgReq;
    VOS_UINT32                              ulResult;

    TAF_MEM_SET_S(&stGetNvWriteCfgReq, sizeof(stGetNvWriteCfgReq), 0x00, sizeof(TAF_PS_GET_DSFLOW_NV_WRITE_CFG_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_GET_DSFLOW_WRITE_NV_CFG_REQ消息 */
    stGetNvWriteCfgReq.stCtrl.ulModuleId = ulModuleId;
    stGetNvWriteCfgReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetNvWriteCfgReq.stCtrl.ucOpId     = ucOpId;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_DSFLOW_NV_WRITE_CFG_REQ,
                             &stGetNvWriteCfgReq,
                             sizeof(TAF_PS_GET_DSFLOW_NV_WRITE_CFG_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_SetCtaInfo
 功能描述  : 设置aps no data time len
 输入参数  : VOS_UINT32                   ulModuleId,
             usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             VOS_UINT8                    ucOpId,
             VOS_UINT8                    ucTimeLen
 输出参数  : 无
 返 回 值  : VOS_OK                     - 成功
             VOS_ERR                    - 失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年4月20日
    作    者   : c00299063
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_SetCtaInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    VOS_UINT8                           ucTimeLen
)
{
    TAF_PS_SET_CTA_INFO_REQ_STRU        stSetPktCdataInactivityTmrlenReq;
    VOS_UINT32                          ulResult;

    TAF_MEM_SET_S(&stSetPktCdataInactivityTmrlenReq, sizeof(stSetPktCdataInactivityTmrlenReq), 0x00, sizeof(stSetPktCdataInactivityTmrlenReq));

    /* 构造ID_MSG_TAF_PS_GET_DSFLOW_WRITE_NV_CFG_REQ消息 */
    stSetPktCdataInactivityTmrlenReq.stCtrl.ulModuleId  = ulModuleId;
    stSetPktCdataInactivityTmrlenReq.stCtrl.usClientId  = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stSetPktCdataInactivityTmrlenReq.stCtrl.ucOpId      = ucOpId;


    stSetPktCdataInactivityTmrlenReq.ucPktCdataInactivityTmrLen = ucTimeLen;


    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_SET_CTA_INFO_REQ,
                             &stSetPktCdataInactivityTmrlenReq,
                             sizeof(stSetPktCdataInactivityTmrlenReq));

    return ulResult;
}
/*****************************************************************************
 函 数 名  : TAF_PS_GetCtaInfo
 功能描述  : 获取aps no data time len
 输入参数  : VOS_UINT32                   ulModuleId,
             usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             VOS_UINT8                    ucOpId,
 输出参数  : 无
 返 回 值  : VOS_OK                     - 成功
             VOS_ERR                    - 失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年4月20日
    作    者   : c00299063
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_GetCtaInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_PS_GET_CTA_INFO_REQ_STRU        stGetPktCdataInactivityTmrlenReq;
    VOS_UINT32                                              ulResult;

    TAF_MEM_SET_S(&stGetPktCdataInactivityTmrlenReq, sizeof(stGetPktCdataInactivityTmrlenReq), 0x00, sizeof(stGetPktCdataInactivityTmrlenReq));

    stGetPktCdataInactivityTmrlenReq.stCtrl.ulModuleId  = ulModuleId;
    stGetPktCdataInactivityTmrlenReq.stCtrl.usClientId  = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetPktCdataInactivityTmrlenReq.stCtrl.ucOpId      = ucOpId;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_CTA_INFO_REQ,
                             &stGetPktCdataInactivityTmrlenReq,
                             sizeof(stGetPktCdataInactivityTmrlenReq));

    return ulResult;
}


/*****************************************************************************
 函 数 名  : TAF_PS_GetCgmtuInfo
 功能描述  : TAF PS Proc CGMTU AT Qry Command
 输入参数  : VOS_UINT32                   ulModuleId
             usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             VOS_UINT8                    ucOpId
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年5月29日
    作    者   : g00261581
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_GetCgmtuInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_PS_GET_CGMTU_VALUE_REQ_STRU     stGetCgmtuValueReq;
    VOS_UINT32                          ulResult;

    TAF_MEM_SET_S(&stGetCgmtuValueReq, sizeof(stGetCgmtuValueReq), 0x00, sizeof(stGetCgmtuValueReq));

    stGetCgmtuValueReq.stCtrl.ulModuleId = ulModuleId;
    stGetCgmtuValueReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGetCgmtuValueReq.stCtrl.ucOpId     = ucOpId;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_CGMTU_VALUE_REQ,
                             &stGetCgmtuValueReq,
                             sizeof(stGetCgmtuValueReq));

    return ulResult;
}


/*****************************************************************************
 函 数 名  : TAF_PS_SetCdataDialModeInfo
 功能描述  : 配置CDMA  PPP数据传输模式参数
 输入参数  : VOS_UINT32                   ulModuleId
             usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             TAF_PS_CDATA_DIAL_MODE_ENUM_UINT32 enDialMode
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年04月02日
    作    者   : y00314741
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_SetCdataDialModeInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    TAF_PS_CDATA_DIAL_MODE_ENUM_UINT32  enDialMode
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_CDATA_DIAL_MODE_REQ_STRU     stDialModeReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stDialModeReq, sizeof(stDialModeReq), 0x00, sizeof(TAF_PS_CDATA_DIAL_MODE_REQ_STRU));

    /* 构造TAF_PS_CDATA_DIAL_MODE_REQ_STRU消息 */
    stDialModeReq.stCtrl.ulModuleId     = ulModuleId;
    stDialModeReq.stCtrl.usClientId     = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stDialModeReq.stCtrl.ucOpId         = 0;
    stDialModeReq.enDialMode            = enDialMode;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_SET_CDMA_DIAL_MODE_REQ,
                             &stDialModeReq,
                             sizeof(TAF_PS_CDATA_DIAL_MODE_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_SetImsPdpCfg
 功能描述  : 设置IMS PDP
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             pstImsPdpCfg               - IMS PDP配置
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年7月30日
    作    者   : z00301431
    修改内容   : 新生成函数
*****************************************************************************/
VOS_UINT32 TAF_PS_SetImsPdpCfg(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    TAF_IMS_PDP_CFG_STRU               *pstImsPdpCfg
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_SET_IMS_PDP_CFG_REQ_STRU     stSetImsPdpCfgReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stSetImsPdpCfgReq, sizeof(stSetImsPdpCfgReq), 0x00, sizeof(TAF_PS_SET_IMS_PDP_CFG_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_SET_IMS_PDP_CFG_REQ消息 */
    stSetImsPdpCfgReq.stCtrl.ulModuleId = ulModuleId;
    stSetImsPdpCfgReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stSetImsPdpCfgReq.stCtrl.ucOpId     = ucOpId;
    stSetImsPdpCfgReq.stImsPdpCfg       = *pstImsPdpCfg;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_SET_IMS_PDP_CFG_REQ,
                             &stSetImsPdpCfgReq,
                             sizeof(TAF_PS_SET_IMS_PDP_CFG_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_SetCdmaDormantTimer
 功能描述  : 设置DORMANT TIMER
 输入参数  : usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             ucOpId                     - 操作码ID
             ucDormantTimer             - 时长
 输出参数  : 无
 返 回 值  : VOS_OK                     - 发送消息成功
             VOS_ERR                    - 发送消息失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年08月13日
    作    者   : y00314741
    修改内容   : 新生成函数
*****************************************************************************/
VOS_UINT32 TAF_PS_SetCdmaDormantTimer(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId,
    VOS_UINT8                           ucDormantTimer
)
{
    VOS_UINT32                          ulResult;
    TAF_PS_SET_1X_DORM_TIMER_REQ_STRU   st1xDormTimerReq;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&st1xDormTimerReq, sizeof(st1xDormTimerReq), 0x00, sizeof(TAF_PS_SET_1X_DORM_TIMER_REQ_STRU));

    /* 构造ID_MSG_TAF_PS_SET_1X_DORMANT_TIMER_REQ消息 */
    st1xDormTimerReq.stCtrl.ulModuleId = ulModuleId;
    st1xDormTimerReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    st1xDormTimerReq.stCtrl.ucOpId     = ucOpId;
    st1xDormTimerReq.ucDormantTimer    = ucDormantTimer;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_SET_1X_DORM_TIMER_REQ,
                             &st1xDormTimerReq,
                             sizeof(TAF_PS_SET_1X_DORM_TIMER_REQ_STRU));

    return ulResult;
}

/*****************************************************************************
 函 数 名  : TAF_PS_ProcCdmaDormTimerQryReq
 功能描述  : TAF PS Proc CDORMTIMER AT Qry Command
 输入参数  : VOS_UINT32                   ulModuleId
             usExClientId               - 扩展客户端ID
                                          A核 : ModemID(高四bit) + 客户端ID
                                          C核 : 客户端ID
             VOS_UINT8                    ucOpId
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年08月14日
    作    者   : y00314741
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_PS_ProcCdmaDormTimerQryReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usExClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_PS_GET_1X_DORM_TIMER_REQ_STRU   stGet1xDormTimerReq;
    VOS_UINT32                          ulResult;

    TAF_MEM_SET_S(&stGet1xDormTimerReq, sizeof(stGet1xDormTimerReq), 0x00, sizeof(stGet1xDormTimerReq));

    stGet1xDormTimerReq.stCtrl.ulModuleId = ulModuleId;
    stGet1xDormTimerReq.stCtrl.usClientId = TAF_PS_GET_CLIENTID_FROM_EXCLIENTID(usExClientId);
    stGet1xDormTimerReq.stCtrl.ucOpId     = ucOpId;

    /* 发送消息 */
    ulResult = TAF_PS_SndMsg(TAF_PS_GET_MODEMID_FROM_EXCLIENTID(usExClientId),
                             ID_MSG_TAF_PS_GET_1X_DORM_TIEMR_REQ,
                             &stGet1xDormTimerReq,
                             sizeof(stGet1xDormTimerReq));

    return ulResult;
}




/*****************************************************************************
 函 数 名  : TAF_PS_GetCdataBearStatus
 功能描述  : 获取PPP的状态
 输入参数  : 无
 输出参数  : 无
 返 回 值  : APS_CDS_CDATA_BEAR_STATUS_ENUM_UINT8
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月20日
    作    者   : y00322978
    修改内容   : 新生成函数
*****************************************************************************/
TAF_PS_CDATA_BEAR_STATUS_ENUM_UINT8 TAF_PS_GetCdataBearStatus(
    VOS_UINT8                                               ucPdpId
)
{
    TAF_PS_CDATA_BEAR_STATUS_ENUM_UINT8 ucCdataBearStatus;

    ucCdataBearStatus = TAF_PS_CDATA_BEAR_STATUS_INACTIVE;


    return ucCdataBearStatus;
}

/*****************************************************************************
 函 数 名  : TAF_PS_GetPppStatus
 功能描述  : 可谓可测查询当前PPP状态
 输入参数  : 无
 输出参数  : 无
 返 回 值  : APS_CDS_CDATA_BEAR_STATUS_ENUM_UINT8
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月20日
    作    者   : y00322978
    修改内容   : 新生成函数

*****************************************************************************/
TAF_PS_CDATA_BEAR_STATUS_ENUM_UINT8  TAF_PS_GetPppStatus( VOS_VOID )
{
    VOS_UINT8                           ucPdpId;
    TAF_PS_CDATA_BEAR_STATUS_ENUM_UINT8 enCurrCdataStatus;
    TAF_PS_CDATA_BEAR_STATUS_ENUM_UINT8 enPreCdataStatus;

    enCurrCdataStatus = TAF_PS_CDATA_BEAR_STATUS_INACTIVE;
    enPreCdataStatus = TAF_PS_CDATA_BEAR_STATUS_INACTIVE;

    for (ucPdpId = 0; ucPdpId < TAF_PS_MAX_PDPID; ucPdpId++)
    {
        enCurrCdataStatus = TAF_PS_GetCdataBearStatus(ucPdpId);
        if ((enPreCdataStatus < enCurrCdataStatus)
        && (TAF_PS_CDATA_BEAR_STATUS_BUTT != enCurrCdataStatus))
        {
            enPreCdataStatus = enCurrCdataStatus;
        }

    }
    return enPreCdataStatus;
}




