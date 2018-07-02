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
#include "vos.h"
#include "Taf_Tafm_Remote.h"
#include "PsCommonDef.h"
#include "NVIM_Interface.h"
#include "TafApi.h"

/* #include "MnComm.h" */
#include "MnCommApi.h"
#include "TafAppMma.h"




/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/

#define    THIS_FILE_ID PS_FILE_ID_TAF_MMA_API_C

/*****************************************************************************
   2 全局变量定义
*****************************************************************************/
    extern VOS_UINT32 AT_GetDestPid(
        MN_CLIENT_ID_T                      usClientId,
        VOS_UINT32                          ulRcvPid
    );

/*****************************************************************************
   3 函数实现
*****************************************************************************/



/*****************************************************************************
 函 数 名  : Taf_PhonePinHandle
 功能描述  : +CPIN USIM提供的PIN操做的API
             void Api_PIN_Handle(VOS_UINT8 ucClientId,VOS_UINT8 ucCmdType, VOS_UINT8
             ucPINType,\
             VOS_UINT8 *pucOldPIN, VOS_UINT8 *pucNewPIN)；
 输入参数  : ClientId - APP/AT标识
                 OpId - 操作标识
*pPinData - 手机PIN码操作数据结构
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2005年8月25日
    作    者   : liuyang
    修改内容   : V200R001版本生成函数
  2.日    期   : 2007年10月16日
    作    者   : F62575
    修改内容   : 问题单A32D13062(文档校对发现问题)
*****************************************************************************/
TAF_UINT32 Taf_PhonePinHandle ( MN_CLIENT_ID_T          ClientId,
                                MN_OPERATION_ID_T       OpId,
                                TAF_PH_PIN_DATA_STRU   *pPinData)
{
    return MN_FillAndSndAppReqMsg( ClientId,
                                   OpId,
                                   TAF_MSG_MMA_OP_PIN_REQ,
                                   pPinData,
                                   sizeof(TAF_PH_PIN_DATA_STRU),
                                   I0_WUEPS_PID_MMA);
}


/*****************************************************************************
 函 数 名  : Taf_MePersonalisationHandle
 功能描述  : 锁卡状态查询
 输入参数  : pMsg  消息块
 输出参数  : ClientId - APP/AT标识
             OpId - 操作标识
 返 回 值  : 成功，失败
 修改历史      :
  1.日    期   : 2007年9月28日
    作    者   : h44270
    修改内容   : 新生成函数
*****************************************************************************/
VOS_UINT32 Taf_MePersonalisationHandle(MN_CLIENT_ID_T          ClientId,
                                       MN_OPERATION_ID_T                 OpId,
                                       TAF_ME_PERSONALISATION_DATA_STRU *pMePersonalData)
{
    return MN_FillAndSndAppReqMsg( ClientId,
                                   OpId,
                                   TAF_MSG_MMA_ME_PERSONAL_REQ,
                                   pMePersonalData,
                                   sizeof(TAF_ME_PERSONALISATION_DATA_STRU),
                                   I0_WUEPS_PID_MMA);
}


/* Taf_PhoneAttach */

/**********************************************************
 Function:       Taf_PhonePlmnList
 Description:    可用PLMN搜索
 Calls:          APP/AT
 Data Accessed:  2005-09-16
 Data Updated:
 Input:          ClientId - APP/AT标识
                 OpId     - 操作标识
                 ListType - 搜索类型
 Output:
 Return:         TAF_SUCCESS - 成功
                 TAF_FAILURE - 失败
 Others:
    1.Date        : 2007-10-16
    Author      : F62575
    Modification: 问题单A32D13062(文档校对发现问题)
    2.Date        : 2015-3-4
    Author      : b00269685
    Modification: 使用新接口改为分段上报
**********************************************************/
TAF_UINT32 Taf_PhonePlmnList (
    VOS_UINT32                          ulModuleId,
    MN_CLIENT_ID_T                      usClientId,
    MN_OPERATION_ID_T                   ucOpId,
    TAF_MMA_PLMN_LIST_PARA_STRU            *pstPlmnListPara
)
{
    TAF_MMA_PLMN_LIST_REQ_STRU             *pstMsg  = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 参数检查 */
    if (VOS_NULL_PTR == pstPlmnListPara)
    {
        return VOS_FALSE;
    }

    /* 申请消息包TAF_MMA_DETACH_REQ_STRU */
    pstMsg = (TAF_MMA_PLMN_LIST_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                           ulSenderPid,
                                           sizeof(TAF_MMA_PLMN_LIST_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_PLMN_LIST_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_PLMN_LIST_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 根据输入参数填充TAF_PLMN_LIST_REQ_STRU */
    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MMA_PLMN_LIST_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;
    TAF_MEM_CPY_S(&(pstMsg->stPlmnListPara), sizeof(pstMsg->stPlmnListPara), pstPlmnListPara, sizeof(TAF_MMA_PLMN_LIST_PARA_STRU));

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_PlmnAutoReselReq
 功能描述  : 自动搜网
 输入参数  : ulModuleId             ---  外部模块PID
             usCliendId             ---  外部模块CliendId
             ucOpId                 ---  外部模块OpId
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月23日
    作    者   : f00279542
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_PlmnAutoReselReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_PLMN_AUTO_RESEL_REQ_STRU   *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_PLMN_AUTO_RESEL_REQ_STRU */
    pstMsg = (TAF_MMA_PLMN_AUTO_RESEL_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                            ulSenderPid,
                                            sizeof(TAF_MMA_PLMN_AUTO_RESEL_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_PLMN_AUTO_RESEL_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_PLMN_AUTO_RESEL_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_PLMN_AUTO_RESEL_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_PlmnSpecialSelReq
 功能描述  : 自动搜网
 输入参数  : ulModuleId             ---  外部模块PID
             usCliendId             ---  外部模块CliendId
             ucOpId                 ---  外部模块OpId
             ucPlmnReselModeType    ---  fplmn resel设置参数
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月23日
    作    者   : f00279542
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_PlmnSpecialSelReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_PLMN_USER_SEL_STRU             *pstPlmnUserSel
)
{
    TAF_MMA_PLMN_SPECIAL_SEL_REQ_STRU  *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_PLMN_SPECIAL_SEL_REQ_STRU */
    pstMsg = (TAF_MMA_PLMN_SPECIAL_SEL_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                            ulSenderPid,
                                            sizeof(TAF_MMA_PLMN_SPECIAL_SEL_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_PLMN_SPECIAL_SEL_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_PLMN_SPECIAL_SEL_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_PLMN_SPECIAL_SEL_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;
    TAF_MEM_CPY_S(&(pstMsg->stPlmnUserSel), sizeof(pstMsg->stPlmnUserSel), pstPlmnUserSel, sizeof(TAF_PLMN_USER_SEL_STRU));

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}
/*****************************************************************************
 函 数 名  : TAF_MMA_AbortPlmnListReq
 功能描述  : AbortReq
 输入参数  : ulModuleId             ---  外部模块PID
             usCliendId             ---  外部模块CliendId
             ucOpId                 ---  外部模块OpId
             ucPlmnReselModeType    ---  fplmn resel设置参数
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月23日
    作    者   : f00279542
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_AbortPlmnListReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_PLMN_LIST_ABORT_REQ_STRU   *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_PLMN_LIST_ABORT_REQ_STRU */
    pstMsg = (TAF_MMA_PLMN_LIST_ABORT_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                            ulSenderPid,
                                            sizeof(TAF_MMA_PLMN_LIST_ABORT_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_PLMN_LIST_ABORT_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_PLMN_LIST_ABORT_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_PLMN_LIST_ABORT_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;
    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_QryLocInfoReq
 功能描述  : query loc info
 输入参数  : ulModuleId             ---  外部模块PID
             usCliendId             ---  外部模块CliendId
             ucOpId                 ---  外部模块OpId
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月23日
    作    者   : f00279542
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryLocInfoReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_LOCATION_INFO_QRY_REQ_STRU *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包ID_TAF_MMA_LOCINFO_QRY_REQ */
    pstMsg = (TAF_MMA_LOCATION_INFO_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                                   ulSenderPid,
                                                   sizeof(TAF_MMA_LOCATION_INFO_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_LOCATION_INFO_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_LOCATION_INFO_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_LOCATION_INFO_QRY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;
    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}
/*****************************************************************************
 函 数 名  : TAF_MMA_QryCipherReq
 功能描述  : query cipher info
 输入参数  : ulModuleId             ---  外部模块PID
             usCliendId             ---  外部模块CliendId
             ucOpId                 ---  外部模块OpId
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月23日
    作    者   : f00279542
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryCipherReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_CIPHER_QRY_REQ_STRU        *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包ID_TAF_MMA_LOCINFO_QRY_REQ */
    pstMsg = (TAF_MMA_CIPHER_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                            ulSenderPid,
                                            sizeof(TAF_MMA_CIPHER_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_CIPHER_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_CIPHER_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_CIPHER_QRY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;
    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}
/*****************************************************************************
 函 数 名  : TAF_MMA_SetPrefPlmnTypeReq
 功能描述  : set prefer plmn type
 输入参数  : ulModuleId             ---  外部模块PID
             usCliendId             ---  外部模块CliendId
             ucOpId                 ---  外部模块OpId
             ucPlmnReselModeType    ---  fplmn resel设置参数
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月23日
    作    者   : f00279542
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_SetPrefPlmnTypeReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    MN_PH_PREF_PLMN_TYPE_ENUM_U8       *penPrefPlmnType
)
{
    TAF_MMA_PREF_PLMN_TYPE_SET_REQ_STRU    *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                              ulReceiverPid;
    VOS_UINT32                              ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_PREF_PLMN_TYPE_SET_REQ_STRU */
    pstMsg = (TAF_MMA_PREF_PLMN_TYPE_SET_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                                    ulSenderPid,
                                                    sizeof(TAF_MMA_PREF_PLMN_TYPE_SET_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_PREF_PLMN_TYPE_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_PREF_PLMN_TYPE_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_PREF_PLMN_TYPE_SET_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;
    pstMsg->enPrefPlmnType              = *penPrefPlmnType;
    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}
/*****************************************************************************
 函 数 名  : TAF_MMA_MtPowerDownReq
 功能描述  : 自动搜网
 输入参数  : ulModuleId             ---  外部模块PID
             usCliendId             ---  外部模块CliendId
             ucOpId                 ---  外部模块OpId
             ucPlmnReselModeType    ---  fplmn resel设置参数
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月23日
    作    者   : f00279542
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_MtPowerDownReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_MT_POWER_DOWN_REQ_STRU     *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_MT_POWER_DOWN_REQ_STRU */
    pstMsg = (TAF_MMA_MT_POWER_DOWN_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                            ulSenderPid,
                                            sizeof(TAF_MMA_MT_POWER_DOWN_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_MT_POWER_DOWN_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_MT_POWER_DOWN_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_MT_POWER_DOWN_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;
    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}


/*****************************************************************************
 函 数 名  : TAF_MMA_SetQuickStartReq
 功能描述  : AT^CQST=功能
 输入参数  : 无
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月24日
    作    者   : y00322978
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_SetQuickStartReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    VOS_UINT32                          ulSetValue
)
{
    TAF_MMA_QUICKSTART_SET_REQ_STRU    *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_QUICKSTART_SET_REQ_STRU */
    pstMsg = (TAF_MMA_QUICKSTART_SET_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                              ulSenderPid,
                                              sizeof(TAF_MMA_QUICKSTART_SET_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_QUICKSTART_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_QUICKSTART_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_QUICKSTART_SET_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;
    pstMsg->ulQuickStartMode            = ulSetValue;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_QryQuickStartReq
 功能描述  : CQST QRY命令
 输入参数  : 无
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月27日
    作    者   : y00322978
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryQuickStartReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_QUICKSTART_QRY_REQ_STRU    *pstMsg;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_QUICKSTART_QRY_REQ_STRU */
    pstMsg = (TAF_MMA_QUICKSTART_QRY_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                       ulSenderPid,
                                       sizeof(TAF_MMA_QUICKSTART_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_QUICKSTART_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_QUICKSTART_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_QUICKSTART_QRY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;

}


/*****************************************************************************
 函 数 名  : Taf_UsimRestrictedAccessCommand
 功能描述  : 用于支持受限制的卡操作命令
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2006年10月25日
    作    者   : h59254
    修改内容   : 新生成函数
*****************************************************************************/
TAF_UINT32 Taf_UsimRestrictedAccessCommand(MN_CLIENT_ID_T               ClientId,
                                           MN_OPERATION_ID_T            OpId,
                                           USIMM_RACCESS_REQ_STRU      *pPara)
{
    return MN_FillAndSndAppReqMsg( ClientId,
                                   OpId,
                                   TAF_MSG_MMA_USIM_RESTRICTED_ACCESS,
                                   pPara,
                                   sizeof(USIMM_RACCESS_REQ_STRU),
                                   I0_WUEPS_PID_MMA);
}



/*****************************************************************************
 函 数 名  : Taf_IndPhFreq
 功能描述  : 指定频点搜网
 输入参数  : MN_CLIENT_ID_T ClientId
             MN_OPERATION_ID_T   OpId
             TAF_IND_FREQ_STRU Freq
 输出参数  : TAF_SUCCESS or TAF_FAILURE
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2006年10月24日
    作    者   : x51137
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 Taf_IndPhFreq(MN_CLIENT_ID_T     ClientId,
                         MN_OPERATION_ID_T        OpId,
                         TAF_IND_FREQ_STRU        Freq)
{
    VOS_UINT16 temp_Freq;
    VOS_UINT16 temp_value;

    switch (Freq.RatType)
    {
    case TAF_SYS_SUBMODE_GSM:
        temp_value   = Freq.GsmBand;
        temp_value <<= 12;
        temp_Freq   = temp_value;
        temp_value  = Freq.IndFreq;
        temp_value &= 0x0fff;
        temp_Freq |= temp_value;

        /* write temp_GSM_Freq to NVIM */
        if (NV_OK != NV_Write( en_NV_Item_Gsm_Ind_Freq, &temp_Freq, sizeof(VOS_UINT16)))
        {
            return TAF_FAILURE;
        }

        break;

    case TAF_SYS_SUBMODE_WCDMA:

        /*write Freq to NVIM */
        temp_Freq = Freq.IndFreq;
        if (NV_OK != NV_Write( en_NV_Item_Wcdma_Ind_Freq, &temp_Freq, sizeof(VOS_UINT16)))
        {
            return TAF_FAILURE;
        }

        break;

    default:
        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}


/*****************************************************************************
 函 数 名  : TAF_MMA_QrySyscfgReq
 功能描述  : AT+SYSCFG QRY逻辑
 输入参数  : VOS_UINT32                          ulModuleId,
             VOS_UINT16                          usClientId,
             VOS_UINT8                           ucOpId
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月26日
    作    者   : y00322978
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QrySyscfgReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_SYSCFG_QRY_REQ_STRU        *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_SYSCFG_QRY_REQ_STRU */
    pstMsg = (TAF_MMA_SYSCFG_QRY_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                           ulSenderPid,
                                           sizeof(TAF_MMA_SYSCFG_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
       return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                (VOS_SIZE_T)(sizeof(TAF_MMA_SYSCFG_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_SYSCFG_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_SYSCFG_QRY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}


/*****************************************************************************
 函 数 名  : Taf_SetCopsFormatTypeReq
 功能描述  : 处理cops设置显示类型的请求
 输入参数  : TAF_CLIENT_ID        ClientId
             TAF_ID OpId
             TAF_MMA_NET_SCAN_REQ_STRU    pstNetScan
 输出参数  : 无
 返 回 值  : TAF_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月9日
    作    者   : z00161729
    修改内容   : AT&T 支持EONS特性修改
*****************************************************************************/
VOS_UINT32 Taf_SetCopsFormatTypeReq(
    MN_CLIENT_ID_T                                          usClientId,
    MN_OPERATION_ID_T                                       ucOpId,
    TAF_MMA_COPS_FORMAT_TYPE_SET_REQ_STRU                  *pstCopsFormatType
)
{
    return MN_FillAndSndAppReqMsg(usClientId,
                                  ucOpId,
                                  TAF_MSG_MMA_COPS_FORMAT_TYPE_SET_REQ,
                                  pstCopsFormatType,
                                  sizeof(TAF_MMA_COPS_FORMAT_TYPE_SET_REQ_STRU),
                                  I0_WUEPS_PID_MMA);
}


/*****************************************************************************
 函 数 名  : TAF_SetUsimStub
 功能描述  : 处理模拟usim回复读文件回复给mma
 输入参数  : TAF_CLIENT_ID        ClientId
             TAF_ID OpId
             TAF_MMA_USIM_STUB_SET_REQ_STRU     *pstUsimStub
 输出参数  : 无
 返 回 值  : TAF_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月31日
    作    者   : z00161729
    修改内容   : AT&T 支持EONS特性修改
*****************************************************************************/
VOS_UINT32 TAF_SetUsimStub(
    MN_CLIENT_ID_T                      usClientId,
    MN_OPERATION_ID_T                   ucOpId,
    TAF_MMA_USIM_STUB_SET_REQ_STRU     *pstUsimStub
)
{
    return MN_FillAndSndAppReqMsg(usClientId,
                                  ucOpId,
                                  TAF_MSG_MMA_USIM_STUB_SET_REQ,
                                  pstUsimStub,
                                  sizeof(TAF_MMA_USIM_STUB_SET_REQ_STRU),
                                  I0_WUEPS_PID_MMA);
}


/*****************************************************************************
 函 数 名  : TAF_SetRefreshStub
 功能描述  : 模拟pih给指定模块发送refresh消息
 输入参数  : TAF_CLIENT_ID        ClientId
             TAF_ID OpId
             TAF_MMA_REFRESH_STUB_SET_REQ_STRU  *pstRefreshStub
 输出参数  : 无
 返 回 值  : TAF_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月31日
    作    者   : z00161729
    修改内容   : AT&T 支持EONS特性修改
*****************************************************************************/
VOS_UINT32 TAF_SetRefreshStub(
    MN_CLIENT_ID_T                      usClientId,
    MN_OPERATION_ID_T                   ucOpId,
    TAF_MMA_REFRESH_STUB_SET_REQ_STRU  *pstRefreshStub
)
{
    return MN_FillAndSndAppReqMsg(usClientId,
                                  ucOpId,
                                  TAF_MSG_MMA_REFRESH_STUB_SET_REQ,
                                  pstRefreshStub,
                                  sizeof(TAF_MMA_REFRESH_STUB_SET_REQ_STRU),
                                  I0_WUEPS_PID_MMA);
}


/*****************************************************************************
 函 数 名  : TAF_SetAutoReselStub
 功能描述  : auto resel
 输入参数  : TAF_CLIENT_ID        ClientId
             TAF_ID OpId
             TAF_MMA_AUTO_RESEL_STUB_SET_REQ_STRU       *pstAutoReselStub
 输出参数  : 无
 返 回 值  : TAF_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年5月29日
    作    者   : b00269685
    修改内容   : 新增
*****************************************************************************/
VOS_UINT32 TAF_SetAutoReselStub(
    MN_CLIENT_ID_T                                          usClientId,
    MN_OPERATION_ID_T                                       ucOpId,
    TAF_MMA_AUTO_RESEL_STUB_SET_REQ_STRU                    *pstAutoReselStub
)
{
    return MN_FillAndSndAppReqMsg(usClientId,
                                  ucOpId,
                                  TAF_MSG_MMA_AUTO_RESEL_STUB_SET_REQ,
                                  pstAutoReselStub,
                                  sizeof(TAF_MMA_AUTO_RESEL_STUB_SET_REQ_STRU),
                                  I0_WUEPS_PID_MMA);
}

/* Deleted by k902809 for Iteration 11, 2015-3-25, begin */

/* Deleted by k902809 for Iteration 11, Iteration 11 2015-3-25, end */

/*****************************************************************************
 函 数 名  : TAF_QryUsimInfo
 功能描述  :
 输入参数  :
 输出参数  : Icctype
 返 回 值  : TAF_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2009年9月19日
    作    者   : z40661
    修改内容   : 新增函数,查询USIM卡的相关信息

*****************************************************************************/
TAF_UINT32 TAF_QryUsimInfo(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId,
    TAF_PH_QRY_USIM_INFO_STRU           *pstInfo
)
{
    if ( (TAF_PH_ICC_UNKNOW == pstInfo->Icctype)
      || (pstInfo->Icctype > TAF_PH_ICC_USIM))
    {
        return TAF_FAILURE;
    }

    if (pstInfo->enEfId > TAF_PH_OPL_FILE)
    {
        return TAF_FAILURE;
    }
    return MN_FillAndSndAppReqMsg( ClientId,
                             OpId,
                             TAF_MSG_MMA_USIM_INFO,
                             pstInfo,
                             sizeof(TAF_PH_QRY_USIM_INFO_STRU),
                             I0_WUEPS_PID_MMA);

}

/*****************************************************************************
 函 数 名  : TAF_QryCpnnInfo
 功能描述  :
 输入参数  :
 输出参数  :
 返 回 值  : TAF_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2009年9月19日
    作    者   : z40661
    修改内容   : 新增函数,查询USIM卡的相关信息

*****************************************************************************/
TAF_UINT32 TAF_QryCpnnInfo(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId,
    TAF_PH_ICC_TYPE                     IccType
)
{
    if ( (TAF_PH_ICC_UNKNOW == IccType)
      || (IccType > TAF_PH_ICC_USIM))
    {
        return TAF_FAILURE;
    }

    return MN_FillAndSndAppReqMsg( ClientId,
                             OpId,
                             TAF_MSG_MMA_CPNN_INFO,
                             &IccType,
                             sizeof(IccType),
                             I0_WUEPS_PID_MMA);
}

/* Deleted by k902809 for Iteration 11, 2015-3-24, begin */

/* Deleted by k902809 for Iteration 11, Iteration 11 2015-3-24, end */


/* Added by s00246516 for L-C互操作项目, 2014-01-29, Begin */
/*****************************************************************************
 函 数 名  : TAF_MMA_PhoneModeSetReq
 功能描述  : 手机模式设置接口
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
             pstPhoneModePara ---  Phone mode设置参数
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月27日
    作    者   : s00246516
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_PhoneModeSetReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_MMA_PHONE_MODE_PARA_STRU       *pstPhoneModePara
)
{
    TAF_MMA_PHONE_MODE_SET_REQ_STRU    *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 参数检查 */
    if (VOS_NULL_PTR == pstPhoneModePara)
    {
        return VOS_FALSE;
    }

    /* 申请消息包TAF_MMA_PHONE_MODE_SET_REQ_STRU */
    pstMsg = (TAF_MMA_PHONE_MODE_SET_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                       ulSenderPid,
                                       sizeof(TAF_MMA_PHONE_MODE_SET_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_PHONE_MODE_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_PHONE_MODE_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_PHONE_MODE_SET_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;
    TAF_MEM_CPY_S(&(pstMsg->stPhoneModePara), sizeof(pstMsg->stPhoneModePara), pstPhoneModePara, sizeof(TAF_MMA_PHONE_MODE_PARA_STRU));

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}


/*****************************************************************************
 函 数 名  : TAF_MMA_QryPhoneModeReq
 功能描述  : 手机模式查询接口
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月23日
    作    者   : y00322978
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryPhoneModeReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_PHONE_MODE_QRY_REQ_STRU    *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_PHONE_MODE_SET_REQ_STRU */
    pstMsg = (TAF_MMA_PHONE_MODE_QRY_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                       ulSenderPid,
                                       sizeof(TAF_MMA_PHONE_MODE_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_PHONE_MODE_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_PHONE_MODE_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_PHONE_MODE_QRY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_CsgListSearchReq
 功能描述  : CSG列表查询接口
 输入参数  : ulModuleId      -  外部模块PID
             usCliendId      -  外部模块CliendId
             ucOpId          -  外部模块OpId
             pstPlmnListPara -  csg list分段查询参数
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月28日
    作    者   : z00161729
    修改内容   : 支持LTE CSG功能新增

*****************************************************************************/
VOS_UINT32 TAF_MMA_CsgListSearchReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_MMA_PLMN_LIST_PARA_STRU        *pstPlmnListPara
)
{
    TAF_MMA_CSG_LIST_SEARCH_REQ_STRU   *pstMsg  = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 参数检查 */
    if (VOS_NULL_PTR == pstPlmnListPara)
    {
        return VOS_FALSE;
    }

    /* 申请消息包 */
    pstMsg = (TAF_MMA_CSG_LIST_SEARCH_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                             ulSenderPid,
                                             sizeof(TAF_MMA_CSG_LIST_SEARCH_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_CSG_LIST_SEARCH_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_CSG_LIST_SEARCH_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->enMsgName         = ID_TAF_MMA_CSG_LIST_SEARCH_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;

    TAF_MEM_CPY_S(&pstMsg->stPlmnListPara, sizeof(pstMsg->stPlmnListPara), pstPlmnListPara, sizeof(TAF_MMA_PLMN_LIST_PARA_STRU));

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_AbortCsgListSearchReq
 功能描述  : 终止csg list搜网接口
 输入参数  : ulModuleId             ---  外部模块PID
             usCliendId             ---  外部模块CliendId
             ucOpId                 ---  外部模块OpId
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年9月28日
    作    者   : z00161729
    修改内容   : 支持LTE CSG功能新增

*****************************************************************************/
VOS_UINT32 TAF_MMA_AbortCsgListSearchReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_CSG_LIST_ABORT_REQ_STRU    *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包 */
    pstMsg = (TAF_MMA_CSG_LIST_ABORT_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                            ulSenderPid,
                                            sizeof(TAF_MMA_CSG_LIST_ABORT_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_CSG_LIST_ABORT_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_CSG_LIST_ABORT_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->enMsgName                   = ID_TAF_MMA_CSG_LIST_ABORT_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_SetCsgIdSearch
 功能描述  : 设置CSG ID接口
 输入参数  : ulModuleId       ---  外部模块PID
           : usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
             pstUserSelCsgId  ---  USER设置参数
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年11月23日
    作    者   : s00193151
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_SetCsgIdSearch(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_MMA_CSG_SPEC_SEARCH_INFO_STRU  *pstUserSelCsgId
)
{
    TAF_MMA_CSG_SPEC_SEARCH_REQ_STRU   *pstMsg  = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 参数检查 */
    if (VOS_NULL_PTR == pstUserSelCsgId)
    {
        return VOS_FALSE;
    }

    /* 申请消息包TAF_MMA_CSG_SPEC_SEARCH_REQ_STRU */
    pstMsg = (TAF_MMA_CSG_SPEC_SEARCH_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                             ulSenderPid,
                                             sizeof(TAF_MMA_CSG_SPEC_SEARCH_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_CSG_SPEC_SEARCH_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_CSG_SPEC_SEARCH_REQ_STRU) - VOS_MSG_HEAD_LENGTH));

    /* 根据输入参数填充TAF_MMA_CSG_SPEC_SEARCH_REQ_STRU, 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MMA_CSG_SPEC_SEARCH_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;

    TAF_MEM_CPY_S(&pstMsg->stCsgSpecSearchInfo, sizeof(pstMsg->stCsgSpecSearchInfo), pstUserSelCsgId, sizeof(pstMsg->stCsgSpecSearchInfo));

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_QryCampCsgIdInfoReq
 功能描述  : Csg Id Info qry req
 输入参数  : VOS_UINT32                          ulModuleId
             VOS_UINT16                          usClientId
             VOS_UINT8                           ucOpId
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年11月24日
    作    者   : s00193151
    修改内容   : 支持LTE CSG新增
*****************************************************************************/
VOS_UINT32 TAF_MMA_QryCampCsgIdInfoReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_QRY_CAMP_CSG_ID_INFO_REQ_STRU        *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                                    ulReceiverPid;
    VOS_UINT32                                    ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_QRY_CAMP_CSG_ID_INFO_REQ_STRU */
    pstMsg = (TAF_MMA_QRY_CAMP_CSG_ID_INFO_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                        ulSenderPid,
                                        sizeof(TAF_MMA_QRY_CAMP_CSG_ID_INFO_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
               (VOS_SIZE_T)(sizeof(TAF_MMA_QRY_CAMP_CSG_ID_INFO_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
               0x00,
               (VOS_SIZE_T)(sizeof(TAF_MMA_QRY_CAMP_CSG_ID_INFO_REQ_STRU) - VOS_MSG_HEAD_LENGTH));

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_QRY_CAMP_CSG_ID_INFO_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}


/*****************************************************************************
 函 数 名  : TAF_MMA_SetSysCfgReq
 功能描述  : 手机系统配置接口
 输入参数  : ulModuleId      ---  外部模块PID
            : usCliendId     ---  外部模块CliendId
            ucOpId           ---  外部模块OpId
            pstSysCfgPara    ---  Sys Cfg设置参数
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月27日
    作    者   : s00246516
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_SetSysCfgReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_MMA_SYS_CFG_PARA_STRU          *pstSysCfgPara
)
{
    TAF_MMA_SYS_CFG_REQ_STRU           *pstMsg  = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 参数检查 */
    if (VOS_NULL_PTR == pstSysCfgPara)
    {
        return VOS_FALSE;
    }

    /* 申请消息包TAF_MMA_SYS_CFG_REQ_STRU */
    pstMsg = (TAF_MMA_SYS_CFG_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                             ulSenderPid,
                                             sizeof(TAF_MMA_SYS_CFG_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_SYS_CFG_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_SYS_CFG_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 根据输入参数填充TAF_MMA_SYS_CFG_REQ_STRU */
    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MMA_SYS_CFG_SET_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;

    TAF_MEM_CPY_S(&(pstMsg->stSysCfgPara), sizeof(pstMsg->stSysCfgPara), pstSysCfgPara, sizeof(TAF_MMA_SYS_CFG_PARA_STRU));

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_QryEonsUcs2Req
 功能描述  : 手机系统配置接口
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年03月12日
    作    者   : c00318887
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryEonsUcs2Req(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_EONS_UCS2_REQ_STRU         *pstMsg  = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_EONS_UCS2_REQ_STRU */
    pstMsg = (TAF_MMA_EONS_UCS2_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                        ulSenderPid,
                                        sizeof(TAF_MMA_EONS_UCS2_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_EONS_UCS2_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_EONS_UCS2_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 根据输入参数填充TAF_MMA_EONS_UCS2_REQ_STRU */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MSG_MMA_EONS_UCS2_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_AcqBestNetworkReq
 功能描述  : 获取网络接口
 输入参数  : ulModuleId      ---  外部模块PID
            : usCliendId     ---  外部模块CliendId
            ucOpId           ---  外部模块OpId
            pstAcqPara       ---  Acq设置参数
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月27日
    作    者   : s00246516
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_AcqBestNetworkReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_MMA_ACQ_PARA_STRU              *pstAcqPara
)
{
    TAF_MMA_ACQ_REQ_STRU               *pstMsg  = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 参数检查 */
    if (VOS_NULL_PTR == pstAcqPara)
    {
        return VOS_FALSE;
    }

    /* 申请消息包TAF_MMA_ACQ_REQ_STRU */
    pstMsg = (TAF_MMA_ACQ_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                             ulSenderPid,
                                             sizeof(TAF_MMA_ACQ_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_ACQ_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_ACQ_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MMA_ACQ_BEST_NETWORK_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;
    TAF_MEM_CPY_S(&(pstMsg->stAcqPara), sizeof(pstMsg->stAcqPara), pstAcqPara, sizeof(TAF_MMA_ACQ_PARA_STRU));

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_RegReq
 功能描述  : 注册网络接口
 输入参数  : ulModuleId      ---  外部模块PID
            : usCliendId     ---  外部模块CliendId
            ucOpId           ---  外部模块OpId
            pstRegPara       ---  REG设置参数
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月27日
    作    者   : s00246516
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_RegReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_MMA_REG_PARA_STRU              *pstRegPara
)
{
    TAF_MMA_REG_REQ_STRU               *pstMsg  = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 参数检查 */
    if (VOS_NULL_PTR == pstRegPara)
    {
        return VOS_FALSE;
    }

    /* 申请消息包TAF_MMA_REG_REQ_STRU */
    pstMsg = (TAF_MMA_REG_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                             ulSenderPid,
                                             sizeof(TAF_MMA_REG_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_REG_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_REG_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 根据输入参数填充TAF_MMA_REG_REQ_STRU */
    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MMA_REG_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;
    TAF_MEM_CPY_S(&(pstMsg->stRegPara), sizeof(pstMsg->stRegPara), pstRegPara, sizeof(TAF_MMA_REG_PARA_STRU));

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_PowerSaveReq
 功能描述  : Power Save接口
 输入参数  : ulModuleId      ---  外部模块PID
            : usCliendId     ---  外部模块CliendId
            ucOpId           ---  外部模块OpId
            pstPowerSavePara ---  Power Save设置参数
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月27日
    作    者   : s00246516
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_PowerSaveReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_MMA_POWER_SAVE_PARA_STRU       *pstPowerSavePara
)
{

    TAF_MMA_POWER_SAVE_REQ_STRU        *pstMsg  = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 参数检查 */
    if (VOS_NULL_PTR == pstPowerSavePara)
    {
        return VOS_FALSE;
    }

    /* 申请消息包TAF_MMA_POWER_SAVE_REQ_STRU */
    pstMsg = (TAF_MMA_POWER_SAVE_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                           ulSenderPid,
                                           sizeof(TAF_MMA_POWER_SAVE_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_POWER_SAVE_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_POWER_SAVE_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MMA_POWER_SAVE_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;
    TAF_MEM_CPY_S(&(pstMsg->stPowerSavePara), sizeof(pstMsg->stPowerSavePara), pstPowerSavePara, sizeof(TAF_MMA_POWER_SAVE_PARA_STRU));

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_DetachReq
 功能描述  : Detach接口
 输入参数  : ulModuleId      ---  外部模块PID
            : usCliendId     ---  外部模块CliendId
            ucOpId           ---  外部模块OpId
            pstDetachPara    ---  Detach设置参数
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年1月27日
    作    者   : s00246516
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_DetachReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_MMA_DETACH_PARA_STRU           *pstDetachPara
)
{
    TAF_MMA_DETACH_REQ_STRU            *pstMsg  = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 参数检查 */
    if (VOS_NULL_PTR == pstDetachPara)
    {
        return VOS_FALSE;
    }

    /* 申请消息包TAF_MMA_DETACH_REQ_STRU */
    pstMsg = (TAF_MMA_DETACH_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                           ulSenderPid,
                                           sizeof(TAF_MMA_DETACH_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_DETACH_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_DETACH_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 根据输入参数填充TAF_MMA_DETACH_REQ_STRU */
    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MMA_DETACH_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;
    TAF_MEM_CPY_S(&(pstMsg->stDetachPara), sizeof(pstMsg->stDetachPara), pstDetachPara, sizeof(TAF_MMA_DETACH_PARA_STRU));

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}
/* Added by s00246516 for L-C互操作项目, 2014-01-29, End */

/*****************************************************************************
 函 数 名  : TAF_MMA_SetImsSwitchReq
 功能描述  : IMSSwitch接口
 输入参数  : ulModuleId     ---  外部模块PID
             usCliendId     ---  外部模块CliendId
             ucOpId         ---  外部模块OpId
             ucImsSwitch    ---  IMS 状态设置参数
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年02月02日
    作    者   : zwx247453
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_SetImsSwitchReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_MMA_IMS_SWITCH_SET_ENUM_UINT8   enLteImsSwitch
)
{
    TAF_MMA_IMS_SWITCH_SET_REQ_STRU    *pstMsg  = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 参数检查 */
    if ((TAF_MMA_IMS_SWITCH_SET_OFF != enLteImsSwitch)
     && (TAF_MMA_IMS_SWITCH_SET_ON  != enLteImsSwitch))
    {
        return VOS_FALSE;
    }

    /* 申请消息包TAF_MMA_IMS_SWITCH_SET_REQ_STRU */
    pstMsg = (TAF_MMA_IMS_SWITCH_SET_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                           ulSenderPid,
                                           sizeof(TAF_MMA_IMS_SWITCH_SET_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_IMS_SWITCH_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_IMS_SWITCH_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 根据输入参数填充TAF_MMA_IMS_SWITCH_SET_REQ_STRU */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MMA_IMS_SWITCH_SET_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;
    pstMsg->enLteImsSwitch    = enLteImsSwitch;

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_QryImsSwitchReq
 功能描述  : IMSSwitch接口
 输入参数  : ulModuleId     ---  外部模块PID
             usCliendId     ---  外部模块CliendId
             ucOpId         ---  外部模块OpId
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年02月02日
    作    者   : zwx247453
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryImsSwitchReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_IMS_SWITCH_QRY_REQ_STRU    *pstMsg  = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);


    /* 申请消息包TAF_MMA_IMS_SWITCH_QRY_REQ_STRU */
    pstMsg = (TAF_MMA_IMS_SWITCH_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                           ulSenderPid,
                                           sizeof(TAF_MMA_IMS_SWITCH_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_IMS_SWITCH_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_IMS_SWITCH_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 根据输入参数填充TAF_MMA_IMS_SWITCH_QRY_REQ_STRU */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MMA_IMS_SWITCH_QRY_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_SetVoiceDomainReq
 功能描述  : VoiceDomain接口
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
             enVoiceDomain    ---  优先域设置参数
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年02月02日
    作    者   : zwx247453
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_SetVoiceDomainReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_MMA_VOICE_DOMAIN_ENUM_UINT32    enVoiceDomain
)
{
    TAF_MMA_VOICE_DOMAIN_SET_REQ_STRU  *pstMsg  = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 参数检查 */
    if (enVoiceDomain >= TAF_MMA_VOICE_DOMAIN_BUTT)
    {
        return VOS_FALSE;
    }

    /* 申请消息包TAF_MMA_VOICE_DOMAIN_SET_REQ_STRU */
    pstMsg = (TAF_MMA_VOICE_DOMAIN_SET_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                           ulSenderPid,
                                           sizeof(TAF_MMA_VOICE_DOMAIN_SET_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_VOICE_DOMAIN_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_VOICE_DOMAIN_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 根据输入参数填充TAF_MMA_VOICE_DOMAIN_SET_REQ_STRU */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MMA_VOICE_DOMAIN_SET_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;
    pstMsg->enVoiceDomain     = enVoiceDomain;

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_QryVoiceDomainReq
 功能描述  : VoiceDomain接口
 输入参数  : ulModuleId      ---  外部模块PID
             usCliendId      ---  外部模块CliendId
             ucOpId          ---  外部模块OpId
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年02月02日
    作    者   : zwx247453
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryVoiceDomainReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_VOICE_DOMAIN_QRY_REQ_STRU  *pstMsg  = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_VOICE_DOMAIN_QRY_REQ_STRU */
    pstMsg = (TAF_MMA_VOICE_DOMAIN_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                           ulSenderPid,
                                           sizeof(TAF_MMA_VOICE_DOMAIN_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_VOICE_DOMAIN_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_VOICE_DOMAIN_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 根据输入参数填充TAF_MMA_VOICE_DOMAIN_QRY_REQ_STRU */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MMA_VOICE_DOMAIN_QRY_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_SetRoamImsSupportReq
 功能描述  : RoamImsSupport接口
 输入参数  : ulModuleId      ---  外部模块PID
             usCliendId      ---  外部模块CliendId
             ucOpId          ---  外部模块OpId
             pstRoamImsSupport    ---  RoamImsSupport设置参数
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年3月9日
    作    者   : w00316404
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_SetRoamImsSupportReq(
    VOS_UINT32                           ulModuleId,
    VOS_UINT16                           usClientId,
    VOS_UINT8                            ucOpId,
    TAF_MMA_ROAM_IMS_SUPPORT_ENUM_UINT32 enRoamImsSupport
)
{
    TAF_MMA_ROAM_IMS_SUPPORT_SET_REQ_STRU  *pstMsg  = VOS_NULL_PTR;
    VOS_UINT32                              ulReceiverPid;
    VOS_UINT32                              ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 参数检查 */
    if (TAF_MMA_ROAM_IMS_BUTT <= enRoamImsSupport)
    {
        return VOS_FALSE;
    }

    /* 申请消息包TAF_MMA_ROAM_IMS_SUPPORT_SET_REQ_STRU */
    pstMsg = (TAF_MMA_ROAM_IMS_SUPPORT_SET_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                           ulSenderPid,
                                           sizeof(TAF_MMA_ROAM_IMS_SUPPORT_SET_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_ROAM_IMS_SUPPORT_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_ROAM_IMS_SUPPORT_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 根据输入参数填充TAF_MMA_ROAM_IMS_SUPPORT_SET_REQ_STRU */
    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MMA_ROAM_IMS_SUPPORT_SET_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;
    TAF_MEM_CPY_S(&(pstMsg->enRoamingImsSupportFlag), sizeof(pstMsg->enRoamingImsSupportFlag), &enRoamImsSupport, sizeof(TAF_MMA_ROAM_IMS_SUPPORT_ENUM_UINT32));

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_SetImsDomainCfgReq
 功能描述  : AT向MMA发送Ims域选择偏好设置请求消息函数
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
             enImsDomainCfg   ---  IMS域选择偏好参数

 输出参数  : VOS_TRUE:成功,VOS_FALSE:失败
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月26日
    作    者   : h00360002
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_SetImsDomainCfgReq(
    VOS_UINT32                                   ulModuleId,
    VOS_UINT16                                   usClientId,
    VOS_UINT8                                    ucOpId,
    TAF_MMA_IMS_DOMAIN_CFG_TYPE_ENUM_UINT32      enImsDomainCfg
)
{
    TAF_MMA_IMS_DOMAIN_CFG_SET_REQ_STRU     *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                               ulReceiverPid;
    VOS_UINT32                               ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    if (enImsDomainCfg >= TAF_MMA_IMS_DOMAIN_CFG_TYPE_BUTT)
    {
        return VOS_FALSE;
    }

    /* 内存申请 */
    pstMsg = (TAF_MMA_IMS_DOMAIN_CFG_SET_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                           ulSenderPid,
                                           sizeof(TAF_MMA_IMS_DOMAIN_CFG_SET_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
          (VOS_SIZE_T)(sizeof(TAF_MMA_IMS_DOMAIN_CFG_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
          0x00,
          (VOS_SIZE_T)(sizeof(TAF_MMA_IMS_DOMAIN_CFG_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH));

    /* 根据输入参数填充TAF_MMA_IMS_DOMAIN_CFG_SET_REQ_STRU */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MMA_IMS_DOMAIN_CFG_SET_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;
    pstMsg->enImsDoaminCfg    = enImsDomainCfg;

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_QryImsDomainCfgReq
 功能描述  : AT向MMA发送Ims域选择偏好查询请求函数
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId

 输出参数  : VOS_TRUE:成功,VOS_FALSE:失败
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月26日
    作    者   : h00360002
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryImsDomainCfgReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_IMS_DOMAIN_CFG_QRY_REQ_STRU      *pstMsg  = VOS_NULL_PTR;
    VOS_UINT32                                ulReceiverPid;
    VOS_UINT32                                ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 内存申请*/
    pstMsg = (TAF_MMA_IMS_DOMAIN_CFG_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                           ulSenderPid,
                                           sizeof(TAF_MMA_IMS_DOMAIN_CFG_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
          (VOS_SIZE_T)(sizeof(TAF_MMA_IMS_DOMAIN_CFG_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
          0x00,
          (VOS_SIZE_T)(sizeof(TAF_MMA_IMS_DOMAIN_CFG_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH));

    /* 根据输入参数填充TAF_MMA_IMS_DOMAIN_CFG_QRY_REQ_STRU */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MMA_IMS_DOMAIN_CFG_QRY_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;

     /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_SetImsVtCapCfgReq
 功能描述  : AT向MMA发送ImsVT能力设置请求消息函数
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
             pstImsVtCap      ---  IMS VT能力信息

 输出参数  : VOS_TRUE:成功,VOS_FALSE:失败
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年05月10日
    作    者   : n00269697
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_SetImsVtCapCfgReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_MMA_IMS_VIDEO_CALL_CAP_STRU    *pstImsVtCap
)
{
    TAF_MMA_IMS_VIDEO_CALL_CAP_REQ_STRU                    *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                                              ulReceiverPid;
    VOS_UINT32                                              ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 内存申请 */
    pstMsg = (TAF_MMA_IMS_VIDEO_CALL_CAP_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                        ulSenderPid,
                                        sizeof(TAF_MMA_IMS_VIDEO_CALL_CAP_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
          (VOS_SIZE_T)(sizeof(TAF_MMA_IMS_VIDEO_CALL_CAP_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
          0x00,
          (VOS_SIZE_T)(sizeof(TAF_MMA_IMS_VIDEO_CALL_CAP_REQ_STRU) - VOS_MSG_HEAD_LENGTH));

    /* 根据输入参数填充TAF_MMA_IMS_SMS_CFG_SET_REQ_STRU */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MMA_IMS_VIDEO_CALL_CAP_SET_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;
    TAF_MEM_CPY_S(&pstMsg->stImsVtCap, sizeof(pstMsg->stImsVtCap), pstImsVtCap, sizeof(TAF_MMA_IMS_VIDEO_CALL_CAP_STRU));

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_SetImsSmsCfgReq
 功能描述  : AT向MMA发送Ims短信能力设置请求消息函数
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
             ucEnableFlg      ---  IMS 短信能力标志

 输出参数  : VOS_TRUE:成功,VOS_FALSE:失败
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月31日
    作    者   : m00217266
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_SetImsSmsCfgReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    VOS_UINT8                           ucEnableFlg
)
{
    TAF_MMA_IMS_SMS_CFG_SET_REQ_STRU   *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 内存申请 */
    pstMsg = (TAF_MMA_IMS_SMS_CFG_SET_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                        ulSenderPid,
                                        sizeof(TAF_MMA_IMS_SMS_CFG_SET_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
          (VOS_SIZE_T)(sizeof(TAF_MMA_IMS_SMS_CFG_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
          0x00,
          (VOS_SIZE_T)(sizeof(TAF_MMA_IMS_SMS_CFG_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH));

    /* 根据输入参数填充TAF_MMA_IMS_SMS_CFG_SET_REQ_STRU */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MMA_IMS_SMS_CFG_SET_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;
    pstMsg->ucEnableFlg       = ucEnableFlg;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_QryImsSmsCfgReq
 功能描述  : AT向MMA发送Ims短信能力查询请求函数
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId

 输出参数  : VOS_TRUE:成功,VOS_FALSE:失败
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月31日
    作    者   : m00217266
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryImsSmsCfgReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_IMS_SMS_CFG_QRY_REQ_STRU   *pstMsg  = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 内存申请*/
    pstMsg = (TAF_MMA_IMS_SMS_CFG_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                        ulSenderPid,
                                        sizeof(TAF_MMA_IMS_SMS_CFG_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
          (VOS_SIZE_T)(sizeof(TAF_MMA_IMS_SMS_CFG_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
          0x00,
          (VOS_SIZE_T)(sizeof(TAF_MMA_IMS_SMS_CFG_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH));

    /* 根据输入参数填充TAF_MMA_IMS_DOMAIN_CFG_QRY_REQ_STRU */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MMA_IMS_SMS_CFG_QRY_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;

     /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}


/*****************************************************************************
 函 数 名  : TAF_MMA_AttachReq
 功能描述  : AT向MMA发送ATTACH请求消息函数
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
             ucAttachType     ---  Attach设置参数

 输出参数  : VOS_TRUE:成功,VOS_FALSE:失败
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年4月9日
    作    者   : h00313353
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_AttachReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_MMA_ATTACH_TYPE_ENUM_UINT8      enAttachType
)
{
    TAF_MMA_ATTACH_REQ_STRU            *pstMsg  = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请内存消息并初始化 */
    pstMsg = (TAF_MMA_ATTACH_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                           ulSenderPid,
                                           sizeof(TAF_MMA_ATTACH_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
               (VOS_SIZE_T)(sizeof(TAF_MMA_ATTACH_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
               0x00,
               (VOS_SIZE_T)(sizeof(TAF_MMA_ATTACH_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 根据输入参数填充TAF_MMA_ATTACH_REQ_STRU */
    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->enMsgName         = ID_TAF_MMA_ATTACH_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;
    pstMsg->enAttachType      = enAttachType;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_AttachStatusQryReq
 功能描述  : AT向MMA发送域状态查询消息的消息发送函数
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
             enDomainType     ---  查询的服务域的类型
 输出参数  : VOS_TRUE:成功,VOS_FALSE:失败
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年4月9日
    作    者   : h00313353
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_AttachStatusQryReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_MMA_SERVICE_DOMAIN_ENUM_UINT8   enDomainType
)
{
    TAF_MMA_ATTACH_STATUS_QRY_REQ_STRU *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请内存消息并初始化 */
    pstMsg = (TAF_MMA_ATTACH_STATUS_QRY_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                           ulSenderPid,
                                           sizeof(TAF_MMA_ATTACH_STATUS_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_ATTACH_STATUS_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_ATTACH_STATUS_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 根据输入参数填充TAF_MMA_ATTACH_REQ_STRU */
    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->enMsgName         = ID_TAF_MMA_ATTACH_STATUS_QRY_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;
    pstMsg->enDomainType      = enDomainType;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;

}


/*****************************************************************************
 函 数 名  : TAF_MMA_SrvAcqReq
 功能描述  : SPM模块给MMA模块发送搜网指示
 输入参数  : enSrvType:服务类型
            pstRatList:需要搜网的RAT列表
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年6月17日
    作    者   : w00242748
    修改内容   : 新生成函数

*****************************************************************************/
VOS_VOID TAF_MMA_SrvAcqReq(
    TAF_MMA_SRV_TYPE_ENUM_UINT8         enSrvType,
    TAF_MMA_SRV_ACQ_RAT_LIST_STRU      *pstRatList,
    VOS_UINT32                          ulModuleId
)
{
    TAF_MMA_SRV_ACQ_REQ_STRU           *pstMsg  = VOS_NULL_PTR;

    /* 申请消息包TAF_MMA_SRV_ACQ_REQ_STRU */
    pstMsg = (TAF_MMA_SRV_ACQ_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                             WUEPS_PID_TAF,
                                             sizeof(TAF_MMA_SRV_ACQ_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {

        return;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_SRV_ACQ_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_SRV_ACQ_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid       = WUEPS_PID_TAF;
    pstMsg->ulReceiverPid     = WUEPS_PID_MMA;
    pstMsg->ulMsgName         = ID_TAF_MMA_SRV_ACQ_REQ;

    pstMsg->stCtrl.ulModuleId = ulModuleId;

    pstMsg->enSrvType         = enSrvType;
    pstMsg->stRatList         = *pstRatList;

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(WUEPS_PID_TAF, pstMsg))
    {
        return;
    }

    return;
}



/*****************************************************************************
 函 数 名  : TAF_MMA_ProcCFreqLockSetReq
 功能描述  : 发送TAF_MMA_CFREQ_LOCK_SET_REQ
 输入参数  : ulModuleId        -  外部模块PID
             usClientId        - Client ID
             ucOpId            - 本次操作的标识
             stCFreqLockPara   - 发送锁频操作需要的参数
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年12月29日
    作    者   : y00307564
    修改内容   : 新生成函数
*****************************************************************************/
VOS_UINT32  TAF_MMA_ProcCFreqLockSetReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_MMA_CFREQ_LOCK_SET_PARA_STRU   *pstCFreqLockPara
)
{
    TAF_MMA_CFREQ_LOCK_SET_REQ_STRU    *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 参数检查 */
    if (VOS_NULL_PTR == pstCFreqLockPara)
    {
        return VOS_FALSE;
    }

    /* 申请消息包TAF_MMA_CFREQ_LOCK_SET_REQ_STRU */
    pstMsg = (TAF_MMA_CFREQ_LOCK_SET_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                       ulSenderPid,
                                       sizeof(TAF_MMA_CFREQ_LOCK_SET_REQ_STRU));
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_CFREQ_LOCK_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_CFREQ_LOCK_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MMA_CDMA_FREQ_LOCK_SET_REQ;
    pstMsg->ulModuleId        = ulModuleId;
    pstMsg->usClientId        = usClientId;
    pstMsg->ucOpId            = ucOpId;
    TAF_MEM_CPY_S(&pstMsg->stCFreqLockPara, sizeof(pstMsg->stCFreqLockPara), pstCFreqLockPara, sizeof(TAF_MMA_CFREQ_LOCK_SET_PARA_STRU));

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_ProcCFreqLockQryReq
 功能描述  : 手机模式设置接口
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年12月25日
    作    者   : m00312079
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_ProcCFreqLockQryReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_CFREQ_LOCK_QUERY_REQ_STRU *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                         ulReceiverPid;
    VOS_UINT32                         ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_CFREQ_LOCK_QRY_REQ_STRU */
    pstMsg = (TAF_MMA_CFREQ_LOCK_QUERY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                                   ulSenderPid,
                                                   sizeof(TAF_MMA_CFREQ_LOCK_QUERY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_CFREQ_LOCK_QUERY_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_CFREQ_LOCK_QUERY_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_CDMA_FREQ_LOCK_QRY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_ProcCdmaCsqSetReq
 功能描述  : 手机模式设置接口
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
             pstCdmaCsqPara   ---  CdmaCsq mode设置参数
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年12月25日
    作    者   : m00312079
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_ProcCdmaCsqSetReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_MMA_CDMACSQ_PARA_STRU          *pstCdmaCsqPara
)
{
    TAF_MMA_CDMACSQ_SET_REQ_STRU       *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 参数检查 */
    if (VOS_NULL_PTR == pstCdmaCsqPara)
    {
        return VOS_FALSE;
    }

    /* 申请消息包TAF_MMA_CDMA_CSQ_SET_REQ_STRU */
    pstMsg = (TAF_MMA_CDMACSQ_SET_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                       ulSenderPid,
                                       sizeof(TAF_MMA_CDMACSQ_SET_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_CDMACSQ_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_CDMACSQ_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_CDMACSQ_SET_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;
    TAF_MEM_CPY_S(&(pstMsg->stCdmaCsqPara), sizeof(pstMsg->stCdmaCsqPara), pstCdmaCsqPara, sizeof(TAF_MMA_CDMACSQ_PARA_STRU));

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_ProcCdmaCsqQryReq
 功能描述  : 手机模式设置接口
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年12月25日
    作    者   : m00312079
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_ProcCdmaCsqQryReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_CDMACSQ_QRY_REQ_STRU       *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_CDMACSQ_QRY_REQ_STRU */
    pstMsg = (TAF_MMA_CDMACSQ_QRY_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                       ulSenderPid,
                                       sizeof(TAF_MMA_CDMACSQ_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_CDMACSQ_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_CDMACSQ_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_CDMACSQ_QRY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;

}

/*****************************************************************************
 函 数 名  : TAF_MMA_Proc1xChanSetReq
 功能描述  : 发送TAF_MMA_1XCHAN_SET_REQ
 输入参数  : ulModuleId        -  外部模块PID
             usClientId        - Client ID
             ucOpId            - 本次操作的标识
             stCFreqLockPara   - 发送锁频操作需要的参数
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年05月18日
    作    者   : z00316370
    修改内容   : 新生成函数
*****************************************************************************/
VOS_UINT32  TAF_MMA_Proc1xChanSetReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_MMA_CFREQ_LOCK_SET_PARA_STRU   *pstCFreqLockPara
)
{
    TAF_MMA_CFREQ_LOCK_SET_REQ_STRU    *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 参数检查 */
    if (VOS_NULL_PTR == pstCFreqLockPara)
    {
        return VOS_FALSE;
    }

    /* 申请消息包TAF_MMA_CFREQ_LOCK_SET_REQ_STRU */
    pstMsg = (TAF_MMA_CFREQ_LOCK_SET_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                       ulSenderPid,
                                       sizeof(TAF_MMA_CFREQ_LOCK_SET_REQ_STRU));
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_CFREQ_LOCK_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_CFREQ_LOCK_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MMA_1XCHAN_SET_REQ;
    pstMsg->ulModuleId        = ulModuleId;
    pstMsg->usClientId        = usClientId;
    pstMsg->ucOpId            = ucOpId;
    TAF_MEM_CPY_S(&pstMsg->stCFreqLockPara, sizeof(pstMsg->stCFreqLockPara), pstCFreqLockPara, sizeof(TAF_MMA_CFREQ_LOCK_SET_PARA_STRU));

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_Proc1xChanQryReq
 功能描述  : 获取当前驻留channel的消息接口
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年05月18日
    作    者   : z00316370
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_Proc1xChanQryReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_1XCHAN_QUERY_REQ_STRU       *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                         ulReceiverPid;
    VOS_UINT32                         ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包ID_TAF_MSG_MMA_CVER_QUERY_REQ */
    pstMsg = (TAF_MMA_1XCHAN_QUERY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                sizeof(TAF_MMA_1XCHAN_QUERY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_1XCHAN_QUERY_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_1XCHAN_QUERY_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_1XCHAN_QUERY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_ProcProRevInUseQryReq
 功能描述  : 获取当前使用协议版本的消息接口
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年05月18日
    作    者   : z00316370
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_ProcProRevInUseQryReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_CVER_QUERY_REQ_STRU       *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                         ulReceiverPid;
    VOS_UINT32                         ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包ID_TAF_MSG_MMA_CVER_QUERY_REQ */
    pstMsg = (TAF_MMA_CVER_QUERY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                sizeof(TAF_MMA_CVER_QUERY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_CVER_QUERY_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_CVER_QUERY_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_CVER_QUERY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_ProcStateQryReq
 功能描述  : 获取当前使用协议版本的消息接口
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年05月18日
    作    者   : z00316370
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_ProcStateQryReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_STATE_QUERY_REQ_STRU       *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                         ulReceiverPid;
    VOS_UINT32                         ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包ID_TAF_MSG_MMA_CVER_QUERY_REQ */
    pstMsg = (TAF_MMA_STATE_QUERY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                sizeof(TAF_MMA_STATE_QUERY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_STATE_QUERY_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_STATE_QUERY_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_GETSTA_QUERY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_ProcHPVerQryReq
 功能描述  : 获取当前使用协议版本的消息接口
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年05月18日
    作    者   : z00316370
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_ProcCHVerQryReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_CHIGHVER_QUERY_REQ_STRU   *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                         ulReceiverPid;
    VOS_UINT32                         ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包ID_TAF_MSG_MMA_CVER_QUERY_REQ */
    pstMsg = (TAF_MMA_CHIGHVER_QUERY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                sizeof(TAF_MMA_CHIGHVER_QUERY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_CHIGHVER_QUERY_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_CHIGHVER_QUERY_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_CHIGHVER_QUERY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}




/*****************************************************************************
 函 数 名  : TAF_MMA_SetQuitCallBack
 功能描述  : at QCCB命令处理
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年7月9日
    作    者   : y00322978
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_SetQuitCallBack(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_QUIT_CALLBACK_SET_REQ_STRU *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包ID_TAF_MMA_QUIT_CALLBACK_SET_REQ */
    pstMsg = (TAF_MMA_QUIT_CALLBACK_SET_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                sizeof(TAF_MMA_QUIT_CALLBACK_SET_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_QUIT_CALLBACK_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_QUIT_CALLBACK_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_QUIT_CALLBACK_SET_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_SetCSidList
 功能描述  : AT CMD CSIDLIST
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
             pstWhiteSidList  ---  sid白名单信息
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年7月13日
    作    者   : y00322978
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_SetCSidList(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_MMA_OPER_LOCK_WHITE_SID_STRU   *pstWhiteSidList
)
{
    TAF_MMA_CSIDLIST_SET_REQ_STRU      *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包ID_TAF_MSG_MMA_CVER_QUERY_REQ */
    pstMsg = (TAF_MMA_CSIDLIST_SET_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                sizeof(TAF_MMA_CSIDLIST_SET_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_CSIDLIST_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_CSIDLIST_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_CSIDLIST_SET_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;
    TAF_MEM_CPY_S(&pstMsg->stWhiteSidInfo, sizeof(pstMsg->stWhiteSidInfo), pstWhiteSidList, sizeof(TAF_MMA_OPER_LOCK_WHITE_SID_STRU));

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}
/*****************************************************************************
 函 数 名  : TAF_MMA_QryCurrEmcCallBackMode
 功能描述  : 查询当前是否在紧急呼callback模式
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年7月14日
    作    者   : y00322978
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryCurrEmcCallBackMode(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_1X_EMC_CALL_BACK_QRY_REQ_STRU                  *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                                              ulReceiverPid;
    VOS_UINT32                                              ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* Allocating memory for message */
    pstMsg = (TAF_MMA_1X_EMC_CALL_BACK_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                                               sizeof(TAF_MMA_1X_EMC_CALL_BACK_QRY_REQ_STRU));
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( ((VOS_UINT8 *)pstMsg + VOS_MSG_HEAD_LENGTH),
                (VOS_SIZE_T)(sizeof(TAF_MMA_1X_EMC_CALL_BACK_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_1X_EMC_CALL_BACK_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    pstMsg->ulReceiverPid       = ulReceiverPid;
    pstMsg->ulSenderPid         = ulSenderPid;
    pstMsg->ulMsgName           = ID_TAF_MMA_1X_EMC_CALL_BACK_QRY_REQ;
    pstMsg->stCtrl.ulModuleId   = ulModuleId;
    pstMsg->stCtrl.usClientId   = usClientId;
    pstMsg->stCtrl.ucOpId       = ucOpId;

    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;

}


/*****************************************************************************
 函 数 名  : TAF_MMA_ProcHdrCsqSetReq
 功能描述  : 手机模式设置接口
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
             pstHDRCsqPara   ---  HDRCsq mode设置参数
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :2015年10月21日
    作    者   : C00299064
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_ProcHdrCsqSetReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_MMA_HDR_CSQ_PARA_STRU          *pstHdrCsqPara
)
{
    TAF_MMA_HDR_CSQ_SET_REQ_STRU       *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_CDMA_CSQ_SET_REQ_STRU */
    pstMsg = (TAF_MMA_HDR_CSQ_SET_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                       ulSenderPid,
                                       sizeof(TAF_MMA_HDR_CSQ_SET_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_HDR_CSQ_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_HDR_CSQ_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_HDR_CSQ_SET_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;
    TAF_MEM_CPY_S(&(pstMsg->stHdrCsqSetting), sizeof(pstMsg->stHdrCsqSetting), pstHdrCsqPara, sizeof(TAF_MMA_HDR_CSQ_PARA_STRU));

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_ProcHdrCsqQryReq
 功能描述  : 手机模式设置接口
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月20日
    作    者   : C00299064
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_ProcHdrCsqQryReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_HDR_CSQ_QRY_SETTING_REQ_STRU *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_CDMACSQ_QRY_REQ_STRU */
    pstMsg = (TAF_MMA_HDR_CSQ_QRY_SETTING_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                       ulSenderPid,
                                       sizeof(TAF_MMA_HDR_CSQ_QRY_SETTING_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_HDR_CSQ_QRY_SETTING_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_HDR_CSQ_QRY_SETTING_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_HDR_CSQ_QRY_SETTING_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;

}


/*****************************************************************************
 函 数 名  : TAF_MMA_QryCurrSidNid
 功能描述  : 查询当前系统驻留的SID NID
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月22日
    作    者   : f00279542
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryCurrSidNid(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_CURR_SID_NID_QRY_REQ_STRU  *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* Allocating memory for message */
    pstMsg = (TAF_MMA_CURR_SID_NID_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                                               sizeof(TAF_MMA_CURR_SID_NID_QRY_REQ_STRU));
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( ((VOS_UINT8 *)pstMsg + VOS_MSG_HEAD_LENGTH),
                (VOS_SIZE_T)(sizeof(TAF_MMA_CURR_SID_NID_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_CURR_SID_NID_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    pstMsg->ulReceiverPid       = ulReceiverPid;
    pstMsg->ulSenderPid         = ulSenderPid;
    pstMsg->ulMsgName           = ID_TAF_MMA_CURR_SID_NID_QRY_REQ;
    pstMsg->stCtrl.ulModuleId   = ulModuleId;
    pstMsg->stCtrl.usClientId   = usClientId;
    pstMsg->stCtrl.ucOpId       = ucOpId;

    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;

}

/*****************************************************************************
 函 数 名  : TAF_MMA_QryMultiModeSidMcc
 功能描述  : 查询当前模式的SID/MCC
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年1月20日
    作    者   : w00242748
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryCtRoamInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_CTCC_ROAMING_NW_INFO_QRY_REQ_STRU              *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                                              ulReceiverPid;
    VOS_UINT32                                              ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* Allocating memory for message */
    pstMsg = (TAF_MMA_CTCC_ROAMING_NW_INFO_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                                               sizeof(TAF_MMA_CTCC_ROAMING_NW_INFO_QRY_REQ_STRU));
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( ((VOS_UINT8 *)pstMsg + VOS_MSG_HEAD_LENGTH),
                (VOS_SIZE_T)(sizeof(TAF_MMA_CTCC_ROAMING_NW_INFO_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_CTCC_ROAMING_NW_INFO_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    pstMsg->ulReceiverPid       = ulReceiverPid;
    pstMsg->ulSenderPid         = ulSenderPid;
    pstMsg->enMsgName           = ID_TAF_MMA_CTCC_ROAMING_NW_INFO_QRY_REQ;
    pstMsg->stCtrl.ulModuleId   = ulModuleId;
    pstMsg->stCtrl.usClientId   = usClientId;
    pstMsg->stCtrl.ucOpId       = ucOpId;

    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;

}

/*****************************************************************************
 函 数 名  : TAF_MMA_SetCtOosCount
 功能描述  : 设置中国电信OOS次数
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年7月9日
    作    者   : y00322978
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_SetCtOosCount(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    VOS_UINT32                          ulClOosCount,
    VOS_UINT32                          ulGulOosCount
)
{
    TAF_MMA_CTCC_OOS_COUNT_SET_REQ_STRU                    *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                                              ulReceiverPid;
    VOS_UINT32                                              ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包ID_TAF_MMA_QUIT_CALLBACK_SET_REQ */
    pstMsg = (TAF_MMA_CTCC_OOS_COUNT_SET_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                sizeof(TAF_MMA_CTCC_OOS_COUNT_SET_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_CTCC_OOS_COUNT_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_CTCC_OOS_COUNT_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->enMsgName                   = ID_TAF_MMA_CTCC_OOS_COUNT_SET_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;
    pstMsg->usClOosCount                = (VOS_UINT16)ulClOosCount;
    pstMsg->usGulOosCount               = (VOS_UINT16)ulGulOosCount;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_SetCtRoamInfo
 功能描述  : 设置CTROAMINFO命令是否主动上报
 输入参数  : VOS_UINT32                          ulModuleId
             VOS_UINT16                          usClientId
             VOS_UINT8                           ucOpId
             VOS_UINT8                           ucCtRoamRtpFlag
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年3月11日
    作    者   : g00261581
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_SetCtRoamInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    VOS_UINT8                           ucCtRoamRtpFlag
)
{
    TAF_MMA_CTCC_ROAMING_NW_INFO_RTP_CFG_SET_REQ_STRU      *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                                              ulReceiverPid;
    VOS_UINT32                                              ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包ID_TAF_MMA_CTCC_ROAMING_NW_INFO_RTP_CFG_SET_REQ */
    pstMsg = (TAF_MMA_CTCC_ROAMING_NW_INFO_RTP_CFG_SET_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                                  sizeof(TAF_MMA_CTCC_ROAMING_NW_INFO_RTP_CFG_SET_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_CTCC_ROAMING_NW_INFO_RTP_CFG_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_CTCC_ROAMING_NW_INFO_RTP_CFG_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->enMsgName         = ID_TAF_MMA_CTCC_ROAMING_NW_INFO_RTP_CFG_SET_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;
    pstMsg->ucCtRoamRtpFlag   = ucCtRoamRtpFlag;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}
/*****************************************************************************
 函 数 名  : TAF_MMA_QryPrlIdInfo
 功能描述  : 查询PRLID ,MLPL version id, MSPL version id
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016-3-24
    作    者   : l00359089
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryPrlIdInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_PRLID_QRY_REQ_STRU         *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* Allocating memory for message */
    pstMsg = (TAF_MMA_PRLID_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                                        sizeof(TAF_MMA_PRLID_QRY_REQ_STRU));
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( ((VOS_UINT8 *)pstMsg + VOS_MSG_HEAD_LENGTH),
                sizeof(TAF_MMA_PRLID_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_PRLID_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    pstMsg->ulReceiverPid       = ulReceiverPid;
    pstMsg->ulSenderPid         = ulSenderPid;
    pstMsg->enMsgName           = ID_TAF_MMA_PRLID_QRY_REQ;
    pstMsg->stCtrl.ulModuleId   = ulModuleId;
    pstMsg->stCtrl.usClientId   = usClientId;
    pstMsg->stCtrl.ucOpId       = ucOpId;

    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_QryRatCombinedMode
 功能描述  : 查询多模组合的当前模式
 输入参数  : VOS_UINT32                          ulModuleId
             VOS_UINT16                          usClientId
             VOS_UINT8                           ucOpId
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年7月18日
    作    者   : l00359089
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryRatCombinedMode(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_RAT_COMBINED_MODE_QRY_REQ_STRU                 *pstMsg;
    VOS_UINT32                                              ulReceiverPid;
    VOS_UINT32                                              ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* Allocating memory for message */
    pstMsg = (TAF_MMA_RAT_COMBINED_MODE_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                     sizeof(TAF_MMA_RAT_COMBINED_MODE_QRY_REQ_STRU));
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( ((VOS_UINT8 *)pstMsg + VOS_MSG_HEAD_LENGTH),
                   sizeof(TAF_MMA_RAT_COMBINED_MODE_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                   0x00,
                   (VOS_SIZE_T)(sizeof(TAF_MMA_RAT_COMBINED_MODE_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    pstMsg->ulReceiverPid       = ulReceiverPid;
    pstMsg->ulSenderPid         = ulSenderPid;
    pstMsg->enMsgName           = ID_TAF_MMA_RAT_COMBINED_MODE_QRY_REQ;
    pstMsg->stCtrl.ulModuleId   = ulModuleId;
    pstMsg->stCtrl.usClientId   = usClientId;
    pstMsg->stCtrl.ucOpId       = ucOpId;

    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}



/*****************************************************************************
 函 数 名  : TAF_MMA_ProcResetNtf
 功能描述  : 发送复位步骤
 输入参数  : ulModuleId        -  外部模块PID
             usClientId        - Client ID
             ucOpId            - 本次操作的标识
             ucResetStep       - 发送锁频操作需要的参数
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年8月13日
    作    者   : w00242748
    修改内容   : 新生成函数
*****************************************************************************/
VOS_UINT32  TAF_MMA_ProcResetNtf(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    VOS_UINT8                           ucResetStep
)
{
    TAF_MMA_RESET_NTF_STRU             *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_CFREQ_LOCK_SET_REQ_STRU */
    pstMsg = (TAF_MMA_RESET_NTF_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                       ulSenderPid,
                                       sizeof(TAF_MMA_RESET_NTF_STRU));
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_RESET_NTF_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_RESET_NTF_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MMA_RESET_NTF;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;
    pstMsg->ucResetStep       = ucResetStep;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}


/*****************************************************************************
 函 数 名  : TAF_MMA_SetFPlmnInfo
 功能描述  : 下发CFPLMN设置请求
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
             pstCFPlmnPara    ---  fplmn mode设置参数
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年2月9日
    作    者   : f00279542
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_SetFPlmnInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_PH_FPLMN_OPERATE_STRU          *pstCFPlmnPara
)
{
    TAF_MMA_CFPLMN_SET_REQ_STRU        *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 参数检查 */
    if (VOS_NULL_PTR == pstCFPlmnPara)
    {
        return VOS_FALSE;
    }

    /* 申请消息包TAF_MMA_CDMA_CSQ_SET_REQ_STRU */
    pstMsg = (TAF_MMA_CFPLMN_SET_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                            ulSenderPid,
                                            sizeof(TAF_MMA_CFPLMN_SET_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_CFPLMN_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_CFPLMN_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_CFPLMN_SET_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;
    TAF_MEM_CPY_S(&(pstMsg->stCFPlmnPara), sizeof(pstMsg->stCFPlmnPara), pstCFPlmnPara, sizeof(TAF_PH_FPLMN_OPERATE_STRU));

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_QryFPlmnInfo
 功能描述  : fplmn 查询功能
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年2月9日
    作    者   : f00279542
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryFPlmnInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_CFPLMN_QUERY_REQ_STRU      *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_CDMACSQ_QRY_REQ_STRU */
    pstMsg = (TAF_MMA_CFPLMN_QUERY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                              ulSenderPid,
                                              sizeof(TAF_MMA_CFPLMN_QUERY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_CFPLMN_QUERY_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_CFPLMN_QUERY_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_CFPLMN_QUERY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);
    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_SetCpolReq
 功能描述  : prefplmn设置命令请求接口
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
             pstPrefPlmn      ---  prefplmn 设置参数
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年2月3日
    作    者   : y00307564
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_SetCpolReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_PH_SET_PREFPLMN_STRU           *pstPrefPlmn
)
{
    TAF_MMA_PREF_PLMN_SET_REQ_STRU     *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 参数检查 */
    if (VOS_NULL_PTR == pstPrefPlmn)
    {
        return VOS_FALSE;
    }

    /* 申请消息包TAF_MMA_PREF_PLMN_SET_REQ_STRU */
    pstMsg = (TAF_MMA_PREF_PLMN_SET_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                       ulSenderPid,
                                       sizeof(TAF_MMA_PREF_PLMN_SET_REQ_STRU));
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_PREF_PLMN_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_PREF_PLMN_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH));

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MMA_PREF_PLMN_SET_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;
    TAF_MEM_CPY_S(&pstMsg->stPrefPlmn, sizeof(pstMsg->stPrefPlmn), pstPrefPlmn, sizeof(TAF_PH_SET_PREFPLMN_STRU));

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_QueryCpolReq
 功能描述  : prefplmn查询命令请求接口
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
             pstCpolInfo      ---  pstCpolInfo查询cpol消息参数
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年2月3日
    作    者   : y00307564
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QueryCpolReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_MMA_CPOL_INFO_QUERY_REQ_STRU   *pstCpolInfo
)
{
    TAF_MMA_PREF_PLMN_QUERY_REQ_STRU   *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 参数检查 */
    if (VOS_NULL_PTR == pstCpolInfo)
    {
        return VOS_FALSE;
    }

    /* 申请消息包TAF_MMA_PREF_PLMN_QUERY_REQ_STRU */
    pstMsg = (TAF_MMA_PREF_PLMN_QUERY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                       ulSenderPid,
                                       sizeof(TAF_MMA_PREF_PLMN_QUERY_REQ_STRU));
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_PREF_PLMN_QUERY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_PREF_PLMN_QUERY_REQ_STRU) - VOS_MSG_HEAD_LENGTH));

    /* 填写消息头 */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MMA_PREF_PLMN_QUERY_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;

    TAF_MEM_CPY_S(&pstMsg->stCpolInfo, sizeof(pstMsg->stCpolInfo), pstCpolInfo, sizeof(TAF_MMA_CPOL_INFO_QUERY_REQ_STRU));

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_TestCpolReq
 功能描述  : prefplmn测试命令请求接口
 输入参数  : ulModuleId       ---  外部模块PID
             usCliendId       ---  外部模块CliendId
             ucOpId           ---  外部模块OpId
             enPrefPlmnType   ---  测试prefplmn类型
 输出参数  : 无
 返 回 值  : VOS_TRUE:成功,VOS_FALSE:失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年2月3日
    作    者   : y00307564
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_TestCpolReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    MN_PH_PREF_PLMN_TYPE_ENUM_U8        enPrefPlmnType
)
{
    TAF_MMA_PREF_PLMN_TEST_REQ_STRU    *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 参数检查 */
    if (MN_PH_PREF_PLMN_HPLMN < enPrefPlmnType)
    {
        return VOS_FALSE;
    }

    /* 申请消息包TAF_MMA_PREF_PLMN_TEST_REQ_STRU */
    pstMsg = (TAF_MMA_PREF_PLMN_TEST_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                       ulSenderPid,
                                       sizeof(TAF_MMA_PREF_PLMN_TEST_REQ_STRU));
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_PREF_PLMN_TEST_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_PREF_PLMN_TEST_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->ulMsgName         = ID_TAF_MMA_PREF_PLMN_TEST_REQ;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;
    pstMsg->stCtrl.ucOpId     = ucOpId;
    pstMsg->enPrefPlmnType    = enPrefPlmnType;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_SetCerssiReq
 功能描述  : Set Cerssi Cfg Req
 输入参数  : VOS_UINT32                          ulModuleId
             VOS_UINT16                          usClientId
             VOS_UINT8                           ucOpId
             TAF_START_INFO_IND_STRU            *pstStartInfoInd
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月23日
    作    者   : g00261581
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_SetCerssiReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_START_INFO_IND_STRU            *pstStartInfoInd
)
{
    TAF_MMA_CERSSI_SET_REQ_STRU        *pstCerssiCfg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 参数检查 */
    if (VOS_NULL_PTR == pstStartInfoInd)
    {
        return VOS_FALSE;
    }

    /* 申请消息包TAF_MMA_CDMA_CSQ_SET_REQ_STRU */
    pstCerssiCfg = (TAF_MMA_CERSSI_SET_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                            ulSenderPid,
                                            sizeof(TAF_MMA_CERSSI_SET_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstCerssiCfg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstCerssiCfg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_CERSSI_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_CERSSI_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstCerssiCfg->ulSenderPid                 = ulSenderPid;
    pstCerssiCfg->ulReceiverPid               = ulReceiverPid;
    pstCerssiCfg->ulMsgName                   = ID_TAF_MMA_CERSSI_SET_REQ;

    pstCerssiCfg->stCtrl.ulModuleId           = ulModuleId;
    pstCerssiCfg->stCtrl.usClientId           = usClientId;
    pstCerssiCfg->stCtrl.ucOpId               = ucOpId;
    pstCerssiCfg->ucActionType                = pstStartInfoInd->ucActionType;
    pstCerssiCfg->ucRrcMsgType                = pstStartInfoInd->ucRrcMsgType;
    pstCerssiCfg->ucMinRptTimerInterval       = pstStartInfoInd->ucMinRptTimerInterval;
    pstCerssiCfg->ucSignThreshold             = pstStartInfoInd->ucSignThreshold;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstCerssiCfg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_QryCerssiReq
 功能描述  : Mma Proc Cerssi Qry Req
 输入参数  : VOS_UINT32                          ulModuleId
             VOS_UINT16                          usClientId
             VOS_UINT8                           ucOpId
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月24日
    作    者   : g00261581
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryCerssiReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_CERSSI_INFO_QRY_REQ_STRU   *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_PHONE_MODE_SET_REQ_STRU */
    pstMsg = (TAF_MMA_CERSSI_INFO_QRY_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                                ulSenderPid,
                                                sizeof(TAF_MMA_CERSSI_INFO_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                sizeof(TAF_MMA_CERSSI_INFO_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
                sizeof(TAF_MMA_CERSSI_INFO_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_CERSSI_QRY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}


/*****************************************************************************
 函 数 名  : TAF_MMA_QryCrpnReq
 功能描述  : AT^CRPN
 输入参数  : 无
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月24日
    作    者   : y00322978
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryCrpnReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_MMA_CRPN_QRY_PARA_STRU         *pstCrpnQryReq
)
{
    TAF_MMA_CRPN_QRY_REQ_STRU          *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_CRPN_QRY_REQ_STRU */
    pstMsg = (TAF_MMA_CRPN_QRY_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                       ulSenderPid,
                                       sizeof(TAF_MMA_CRPN_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
       return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_CRPN_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_CRPN_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_CRPN_QRY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    if (VOS_NULL_PTR != pstCrpnQryReq)
    {
        TAF_MEM_CPY_S(&pstMsg->stCrpnQryPara, sizeof(pstMsg->stCrpnQryPara), pstCrpnQryReq, sizeof(TAF_MMA_CRPN_QRY_PARA_STRU));
    }

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_SetCmmReq
 功能描述  : AT^CMM 设置
 输入参数  : ulModuleId usClientId ucOpId *pstCrpnQryReq
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月24日
    作    者   : y00322978
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_SetCmmReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    MM_TEST_AT_CMD_STRU                *pstTestAtCmd
)
{
    TAF_MMA_CMM_SET_REQ_STRU           *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_CMM_SET_REQ_STRU */
    pstMsg = (TAF_MMA_CMM_SET_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                       ulSenderPid,
                                       sizeof(TAF_MMA_CMM_SET_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
       return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_CMM_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_CMM_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_CMM_SET_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    if (VOS_NULL_PTR != pstTestAtCmd )
    {
        TAF_MEM_CPY_S(&pstMsg->stCmmSetReq, sizeof(pstMsg->stCmmSetReq), pstTestAtCmd, sizeof(MM_TEST_AT_CMD_STRU));
    }

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/* Added by k902809 for Iteration 11, 2015-3-23, begin */
/*****************************************************************************
  Function Name     :   TAF_MMA_QryAcInfoReq
  Description       :   Allocate the memory for message.
                        Sends the message to MMA.
                        Returns VOS_FALSE on failure of senfing message.

  Input parameters  :   UINT32                              ulModuleId,
                        MN_CLIENT_ID_T                      ClientId,
                        MN_OPERATION_ID_T                   OpId
  Outout parameters :
  Return Value      :   VOS_FALSE or VOS_TRUE

  Modify History:
      1)  Date      :   2015-03-21
          Author    :   k902809
          Modify content :    Create
*****************************************************************************/
VOS_UINT32 TAF_MMA_QryAcInfoReq(
    VOS_UINT32                          ulModuleId,
    MN_CLIENT_ID_T                      usClientId,
    MN_OPERATION_ID_T                   ucOpId
)
{
    TAF_MMA_AC_INFO_QRY_REQ_STRU       *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* Allocating memory for message */
    pstMsg = (TAF_MMA_AC_INFO_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                                          sizeof(TAF_MMA_AC_INFO_QRY_REQ_STRU));
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( ((VOS_UINT8 *)pstMsg + VOS_MSG_HEAD_LENGTH),
                (VOS_SIZE_T)(sizeof(TAF_MMA_AC_INFO_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_AC_INFO_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    pstMsg->ulReceiverPid       = ulReceiverPid;
    pstMsg->ulSenderPid         = ulSenderPid;
    pstMsg->ulMsgName           = ID_TAF_MMA_AC_INFO_QRY_REQ;
    pstMsg->stCtrl.ulModuleId   = ulModuleId;
    pstMsg->stCtrl.usClientId   = usClientId;
    pstMsg->stCtrl.ucOpId       = ucOpId;

    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
  Function Name     :   TAF_MMA_QryCopnInfoReq
  Description       :   Allocate the memory for message.
                        Sends the message to MMA
                        Returns AT_FAILURE on failure of senfing message.

  Input parameters  :   UINT32                              ulModuleId,
                        MN_CLIENT_ID_T                      ClientId,
                        VOS_UINT16                          usFromIndex,
                        MN_OPERATION_ID_T                   OpId
  Outout parameters :
  Return Value      :   VOS_FALSE or VOS_TRUE

  Modify History:
      1)  Date      :   2015-03-24
          Author    :   k902809
          Modify content :    Create
*****************************************************************************/
VOS_UINT32 TAF_MMA_QryCopnInfoReq(
    VOS_UINT32                          ulModuleId,
    MN_CLIENT_ID_T                      usClientId,
    VOS_UINT16                          usFromIndex,
    MN_OPERATION_ID_T                   ucOpId
)
{
    TAF_MMA_COPN_INFO_QRY_REQ_STRU     *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /*
    AT向MMA请求运营商信息:
    因为核间消息限制，不能一次获取所有运营商信息，这里定义为一次获取50条运营商信息
    第一条请求消息，从索引0开始要求连续的50条运营商信息
    */

    /* Allocating memory for message */
    pstMsg = (TAF_MMA_COPN_INFO_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                                            sizeof(TAF_MMA_COPN_INFO_QRY_REQ_STRU));
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( ((VOS_UINT8 *)pstMsg + VOS_MSG_HEAD_LENGTH),
                (VOS_SIZE_T)(sizeof(TAF_MMA_COPN_INFO_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_COPN_INFO_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    pstMsg->ulReceiverPid       = ulReceiverPid;
    pstMsg->ulSenderPid         = ulSenderPid;
    pstMsg->ulMsgName           = ID_TAF_MMA_COPN_INFO_QRY_REQ;
    pstMsg->stCtrl.ulModuleId   = ulModuleId;
    pstMsg->stCtrl.usClientId   = usClientId;
    pstMsg->stCtrl.ucOpId       = ucOpId;
    pstMsg->usFromIndex         = usFromIndex;
    pstMsg->usPlmnNum           = TAF_MMA_COPN_PLMN_MAX_NUM;

    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
  Function Name     :   TAF_MMA_SimInsertReq
  Description       :   Allocate the memory for sim insert message.
                        Sends the message to MMA
                        Returns AT_FAILURE on failure of senfing message.

  Input parameters  :   UINT32                              ulModuleId
                        MN_CLIENT_ID_T                      ClientId
                        MN_OPERATION_ID_T                   OpId
                        TAF_SIM_INSERT_STATE_ENUM_UINT32    enSimInsertState
  Outout parameters :
  Return Value      :   VOS_FALSE or VOS_TRUE

  Modify History:
      1)  Date      :   2015-03-24
          Author    :   k902809
          Modify content :    Create
*****************************************************************************/
VOS_UINT32 TAF_MMA_SimInsertReq(
    VOS_UINT32                          ulModuleId,
    MN_CLIENT_ID_T                      usClientId,
    MN_OPERATION_ID_T                   ucOpId,
    TAF_SIM_INSERT_STATE_ENUM_UINT32    enSimInsertState)
{

    TAF_MMA_SIM_INSERT_REQ_STRU        *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* Allocating memory for message */
    pstMsg = (TAF_MMA_SIM_INSERT_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                                         sizeof(TAF_MMA_SIM_INSERT_REQ_STRU));
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( ((VOS_UINT8 *)pstMsg + VOS_MSG_HEAD_LENGTH),
                (VOS_SIZE_T)(sizeof(TAF_MMA_SIM_INSERT_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_SIM_INSERT_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    pstMsg->ulReceiverPid       = ulReceiverPid;
    pstMsg->ulSenderPid         = ulSenderPid;
    pstMsg->ulMsgName           = ID_TAF_MMA_SIM_INSERT_REQ;
    pstMsg->stCtrl.ulModuleId   = ulModuleId;
    pstMsg->stCtrl.usClientId   = usClientId;
    pstMsg->stCtrl.ucOpId       = ucOpId;
    pstMsg->enSimInsertState    = enSimInsertState;

    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}


/*****************************************************************************
  Function Name     :   TAF_MMA_SetEOPlmnReq
  Description       :   Allocate the memory for EOPlmn.
                        Sends the message to MMA
                        Returns AT_FAILURE on failure of senfing message.

  Input parameters  :   UINT32                              ulModuleId;
                        MN_CLIENT_ID_T                      ClientId,
                        MN_OPERATION_ID_T                   OpId
                        TAF_MMA_SET_EOPLMN_LIST_STRU       *pstEOPlmnCfg
  Outout parameters :
  Return Value      :   VOS_FALSE or VOS_TRUE

  Modify History:
      1)  Date      :   2015-03-24
          Author    :   k902809
          Modify content :    Create
*****************************************************************************/
VOS_UINT32 TAF_MMA_SetEOPlmnReq(
    VOS_UINT32                          ulModuleId,
    MN_CLIENT_ID_T                      usClientId,
    MN_OPERATION_ID_T                   ucOpId,
    TAF_MMA_SET_EOPLMN_LIST_STRU       *pstEOPlmnCfg
)
{
    TAF_MMA_EOPLMN_SET_REQ_STRU        *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    if (VOS_NULL_PTR == pstEOPlmnCfg)
    {
        return VOS_FALSE;
    }

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* Allocating memory for message */
    pstMsg = (TAF_MMA_EOPLMN_SET_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                                         sizeof(TAF_MMA_EOPLMN_SET_REQ_STRU));
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( ((VOS_UINT8 *)pstMsg + VOS_MSG_HEAD_LENGTH),
                (VOS_SIZE_T)(sizeof(TAF_MMA_EOPLMN_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_EOPLMN_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    pstMsg->ulReceiverPid       = ulReceiverPid;
    pstMsg->ulSenderPid         = ulSenderPid;
    pstMsg->ulMsgName           = ID_TAF_MMA_EOPLMN_SET_REQ;
    pstMsg->stCtrl.ulModuleId   = ulModuleId;
    pstMsg->stCtrl.usClientId   = usClientId;
    pstMsg->stCtrl.ucOpId       = ucOpId;

    TAF_MEM_CPY_S(&(pstMsg->stEOPlmnSetPara), sizeof(pstMsg->stEOPlmnSetPara), pstEOPlmnCfg, sizeof(TAF_MMA_SET_EOPLMN_LIST_STRU));

    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}


/*****************************************************************************
  Function Name     :   TAF_MMA_EOPlmnQryReq
  Description       :   Allocate the memory for EOPlmn.
                        Sends the message to MMA
                        Returns AT_FAILURE on failure of senfing message.

  Input parameters  :   UINT32                              ulModuleId;
                        MN_CLIENT_ID_T                      ClientId,
                        MN_OPERATION_ID_T                   OpId
                        TAF_MMA_SET_EOPLMN_LIST_STRU       *pstEOPlmnCfg
  Outout parameters :
  Return Value      :   VOS_FALSE or VOS_TRUE

  Modify History:
      1)  Date      :   2015-03-30
          Author    :   k902809
          Modify content :    Create
*****************************************************************************/
VOS_UINT32 TAF_MMA_QryEOPlmnReq(
    VOS_UINT32                          ulModuleId,
    MN_CLIENT_ID_T                      usClientId,
    MN_OPERATION_ID_T                   ucOpId
)
{
    TAF_MMA_EOPLMN_QRY_REQ_STRU        *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* Allocating memory for message */
    pstMsg = (TAF_MMA_EOPLMN_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                                         sizeof(TAF_MMA_EOPLMN_QRY_REQ_STRU));
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( ((VOS_UINT8 *)pstMsg + VOS_MSG_HEAD_LENGTH),
                (VOS_SIZE_T)(sizeof(TAF_MMA_EOPLMN_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_EOPLMN_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    pstMsg->ulReceiverPid       = ulReceiverPid;
    pstMsg->ulSenderPid         = ulSenderPid;
    pstMsg->ulMsgName           = ID_TAF_MMA_EOPLMN_QRY_REQ;
    pstMsg->stCtrl.ulModuleId   = ulModuleId;
    pstMsg->stCtrl.usClientId   = usClientId;
    pstMsg->stCtrl.ucOpId       = ucOpId;

    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}


/*****************************************************************************
  Function Name     :   TAF_MMA_QryCLocInfoReq
  Description       :   Allocate the memory for message.
                        Sends the message to MMA.
                        Returns VOS_FALSE on failure of senfing message.

  Input parameters  :   UINT32                              ulModuleId,
                        MN_CLIENT_ID_T                      ClientId,
                        MN_OPERATION_ID_T                   OpId
  Outout parameters :
  Return Value      :   VOS_FALSE or VOS_TRUE

  Modify History:
      1)  Date      :   2015-03-25
          Author    :   k902809
          Modify content :    Create
*****************************************************************************/
VOS_UINT32 TAF_MMA_QryCLocInfoReq(
    VOS_UINT32                          ulModuleId,
    MN_CLIENT_ID_T                      usClientId,
    MN_OPERATION_ID_T                   ucOpId
)
{
    TAF_MMA_CDMA_LOCINFO_QRY_REQ_STRU  *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* Allocating memory for message */
    pstMsg = (TAF_MMA_CDMA_LOCINFO_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                                               sizeof(TAF_MMA_CDMA_LOCINFO_QRY_REQ_STRU));
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( ((VOS_UINT8 *)pstMsg + VOS_MSG_HEAD_LENGTH),
                (VOS_SIZE_T)(sizeof(TAF_MMA_CDMA_LOCINFO_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_CDMA_LOCINFO_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    pstMsg->ulReceiverPid       = ulReceiverPid;
    pstMsg->ulSenderPid         = ulSenderPid;
    pstMsg->ulMsgName           = ID_TAF_MMA_CDMA_LOCINFO_QRY_REQ;
    pstMsg->stCtrl.ulModuleId   = ulModuleId;
    pstMsg->stCtrl.usClientId   = usClientId;
    pstMsg->stCtrl.ucOpId       = ucOpId;

    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
  Function Name     :   TAF_MMA_NetScanReq
  Description       :   Allocate the memory for message.
                        Sends the message to MMA.
                        Returns VOS_FALSE on failure of senfing message.

  Input parameters  :   UINT32                              ulModuleId,
                        MN_CLIENT_ID_T                      ClientId,
                        MN_OPERATION_ID_T                   OpId,
                        TAF_MMA_NET_SCAN_REQ_STRU          *pstRecvNetScanSetPara
  Outout parameters :
  Return Value      :   VOS_FALSE or VOS_TRUE

  Modify History:
      1)  Date      :   2015-03-21
          Author    :   k902809
          Modify content :    Create
*****************************************************************************/
VOS_UINT32 TAF_MMA_NetScanReq(
    VOS_UINT32                          ulModuleId,
    MN_CLIENT_ID_T                      usClientId,
    MN_OPERATION_ID_T                   ucOpId,
    TAF_MMA_NET_SCAN_REQ_STRU          *pstRecvNetScanSetPara
)
{
    TAF_MMA_NET_SCAN_REQ_STRU          *pstNetScanSetPara = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    if (VOS_NULL_PTR == pstRecvNetScanSetPara)
    {
        return VOS_FALSE;
    }

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* Allocating memory for message */
    pstNetScanSetPara = (TAF_MMA_NET_SCAN_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                                                  sizeof(TAF_MMA_NET_SCAN_REQ_STRU));
    if (VOS_NULL_PTR == pstNetScanSetPara)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( ((VOS_UINT8 *)pstNetScanSetPara + VOS_MSG_HEAD_LENGTH),
                (VOS_SIZE_T)(sizeof(TAF_MMA_NET_SCAN_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_NET_SCAN_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    pstNetScanSetPara->ulReceiverPid       = ulReceiverPid;
    pstNetScanSetPara->ulSenderPid         = ulSenderPid;
    pstNetScanSetPara->ulMsgName           = ID_TAF_MMA_NET_SCAN_REQ;
    pstNetScanSetPara->stCtrl.ulModuleId   = ulModuleId;
    pstNetScanSetPara->stCtrl.usClientId   = usClientId;
    pstNetScanSetPara->stCtrl.ucOpId       = ucOpId;
    pstNetScanSetPara->usCellNum           = pstRecvNetScanSetPara->usCellNum;
    pstNetScanSetPara->enRat               = pstRecvNetScanSetPara->enRat;
    pstNetScanSetPara->sCellPow            = pstRecvNetScanSetPara->sCellPow;
    TAF_MEM_CPY_S(&pstNetScanSetPara->stBand, sizeof(pstNetScanSetPara->stBand), &pstRecvNetScanSetPara->stBand, sizeof(TAF_USER_SET_PREF_BAND64));

    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstNetScanSetPara);

    return VOS_TRUE;
}

/*****************************************************************************
  Function Name     :   TAF_MMA_NetScanAbortReq
  Description       :   Allocate the memory for message.
                        Sends the message to MMA.
                        Returns VOS_FALSE on failure of senfing message.

  Input parameters  :   UINT32                              ulModuleId,
                        MN_CLIENT_ID_T                      ClientId,
                        MN_OPERATION_ID_T                   OpId
  Outout parameters :
  Return Value      :   VOS_FALSE or VOS_TRUE

  Modify History:
      1)  Date      :   2015-03-21
          Author    :   k902809
          Modify content :    Create
*****************************************************************************/
VOS_UINT32 TAF_MMA_NetScanAbortReq(
    VOS_UINT32                          ulModuleId,
    MN_CLIENT_ID_T                      usClientId,
    MN_OPERATION_ID_T                   ucOpId
)
{
    TAF_MMA_NET_SCAN_ABORT_REQ_STRU    *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* Allocating memory for message */
    pstMsg = (TAF_MMA_NET_SCAN_ABORT_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                                             sizeof(TAF_MMA_NET_SCAN_ABORT_REQ_STRU));
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( ((VOS_UINT8 *)pstMsg + VOS_MSG_HEAD_LENGTH),
                (VOS_SIZE_T)(sizeof(TAF_MMA_NET_SCAN_ABORT_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_NET_SCAN_ABORT_REQ_STRU) - VOS_MSG_HEAD_LENGTH));

    pstMsg->ulReceiverPid       = ulReceiverPid;
    pstMsg->ulSenderPid         = ulSenderPid;
    pstMsg->ulMsgName           = ID_TAF_MMA_NET_SCAN_ABORT_REQ;
    pstMsg->stCtrl.ulModuleId   = ulModuleId;
    pstMsg->stCtrl.usClientId   = usClientId;
    pstMsg->stCtrl.ucOpId       = ucOpId;

    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}


/*****************************************************************************
  Function Name     :   TAF_MMA_QrySpnReq
  Description       :   Allocate the memory for message.
                        Sends the message to MMA.
                        Returns VOS_FALSE on failure of senfing message.

  Input parameters  :   UINT32                              ulModuleId,
                        MN_CLIENT_ID_T                      ClientId,
                        MN_OPERATION_ID_T                   OpId
  Outout parameters :
  Return Value      :   VOS_FALSE or VOS_TRUE

  Modify History:
      1)  Date      :   2015-03-30
          Author    :   k902809
          Modify content :    Create
*****************************************************************************/
VOS_UINT32 TAF_MMA_QrySpnReq(
    VOS_UINT32                          ulModuleId,
    MN_CLIENT_ID_T                      usClientId,
    MN_OPERATION_ID_T                   ucOpId
)
{
    TAF_MMA_SPN_QRY_REQ_STRU           *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* Allocating memory for message */
    pstMsg = (TAF_MMA_SPN_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                                      sizeof(TAF_MMA_SPN_QRY_REQ_STRU));
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( ((VOS_UINT8 *)pstMsg + VOS_MSG_HEAD_LENGTH),
                (VOS_SIZE_T)(sizeof(TAF_MMA_SPN_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_SPN_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    pstMsg->ulReceiverPid       = ulReceiverPid;
    pstMsg->ulSenderPid         = ulSenderPid;
    pstMsg->ulMsgName           = ID_TAF_MMA_SPN_QRY_REQ;
    pstMsg->stCtrl.ulModuleId   = ulModuleId;
    pstMsg->stCtrl.usClientId   = usClientId;
    pstMsg->stCtrl.ucOpId       = ucOpId;

    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}



/*****************************************************************************
  Function Name     :   TAF_MMA_QryMMPlmnInfoReq
  Description       :   Allocate the memory for message.
                        Sends the message to MMA.
                        Returns VOS_FALSE on failure of senfing message.

  Input parameters  :   UINT32                              ulModuleId,
                        MN_CLIENT_ID_T                      ClientId,
                        MN_OPERATION_ID_T                   OpId
  Outout parameters :
  Return Value      :   VOS_FALSE or VOS_TRUE

  Modify History:
      1)  Date      :   2015-03-30
          Author    :   k902809
          Modify content :    Create
*****************************************************************************/
VOS_UINT32 TAF_MMA_QryMMPlmnInfoReq(
    VOS_UINT32                          ulModuleId,
    MN_CLIENT_ID_T                      usClientId,
    MN_OPERATION_ID_T                   ucOpId
)
{
    TAF_MMA_MMPLMNINFO_QRY_REQ_STRU    *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* Allocating memory for message */
    pstMsg = (TAF_MMA_MMPLMNINFO_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                                             sizeof(TAF_MMA_MMPLMNINFO_QRY_REQ_STRU));
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( ((VOS_UINT8 *)pstMsg + VOS_MSG_HEAD_LENGTH),
                (VOS_SIZE_T)(sizeof(TAF_MMA_MMPLMNINFO_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_MMPLMNINFO_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    pstMsg->ulReceiverPid       = ulReceiverPid;
    pstMsg->ulSenderPid         = ulSenderPid;
    pstMsg->ulMsgName           = ID_TAF_MMA_MMPLMNINFO_QRY_REQ;
    pstMsg->stCtrl.ulModuleId   = ulModuleId;
    pstMsg->stCtrl.usClientId   = usClientId;
    pstMsg->stCtrl.ucOpId       = ucOpId;

    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
  Function Name     :   TAF_MMA_QryPlmnReq
  Description       :   Allocate the memory for message.
                        Sends the message to MMA.
                        Returns VOS_FALSE on failure of senfing message.

  Input parameters  :   UINT32                              ulModuleId,
                        MN_CLIENT_ID_T                      ClientId,
                        MN_OPERATION_ID_T                   OpId
  Outout parameters :
  Return Value      :   VOS_FALSE or VOS_TRUE

  Modify History:
      1)  Date      :   2015-03-30
          Author    :   k902809
          Modify content :    Create
*****************************************************************************/
VOS_UINT32 TAF_MMA_QryPlmnReq(
    VOS_UINT32                          ulModuleId,
    MN_CLIENT_ID_T                      usClientId,
    MN_OPERATION_ID_T                   ucOpId
)
{
    TAF_MMA_LAST_CAMP_PLMN_QRY_REQ_STRU    *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                              ulReceiverPid;
    VOS_UINT32                              ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* Allocating memory for message */
    pstMsg = (TAF_MMA_LAST_CAMP_PLMN_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                    sizeof(TAF_MMA_LAST_CAMP_PLMN_QRY_REQ_STRU));
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( ((VOS_UINT8 *)pstMsg + VOS_MSG_HEAD_LENGTH),
                (VOS_SIZE_T)(sizeof(TAF_MMA_LAST_CAMP_PLMN_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_LAST_CAMP_PLMN_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH));

    pstMsg->ulReceiverPid       = ulReceiverPid;
    pstMsg->ulSenderPid         = ulSenderPid;
    pstMsg->ulMsgName           = ID_TAF_MMA_LAST_CAMP_PLMN_QRY_REQ;
    pstMsg->stCtrl.ulModuleId   = ulModuleId;
    pstMsg->stCtrl.usClientId   = usClientId;
    pstMsg->stCtrl.ucOpId       = ucOpId;

    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
  Function Name     :   TAF_MMA_QryUserSrvStateReq
  Description       :   Allocate the memory for message.
                        Sends the message to MMA.
                        Returns VOS_FALSE on failure of senfing message.

  Input parameters  :   UINT32                              ulModuleId,
                        MN_CLIENT_ID_T                      ClientId,
                        MN_OPERATION_ID_T                   OpId
  Outout parameters :
  Return Value      :   VOS_FALSE or VOS_TRUE

  Modify History:
      1)  Date      :   2015-03-27
          Author    :   k902809
          Modify content :    Create
*****************************************************************************/
VOS_UINT32 TAF_MMA_QryUserSrvStateReq(
    VOS_UINT32                          ulModuleId,
    MN_CLIENT_ID_T                      usClientId,
    MN_OPERATION_ID_T                   ucOpId
)
{
    TAF_MMA_USER_SRV_STATE_QRY_REQ_STRU    *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                              ulReceiverPid;
    VOS_UINT32                              ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* Allocating memory for message */
    pstMsg = (TAF_MMA_USER_SRV_STATE_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                    sizeof(TAF_MMA_USER_SRV_STATE_QRY_REQ_STRU));
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( ((VOS_UINT8 *)pstMsg + VOS_MSG_HEAD_LENGTH),
                (VOS_SIZE_T)(sizeof(TAF_MMA_USER_SRV_STATE_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_USER_SRV_STATE_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    pstMsg->ulReceiverPid       = ulReceiverPid;
    pstMsg->ulSenderPid         = ulSenderPid;
    pstMsg->ulMsgName           = ID_TAF_MMA_USER_SRV_STATE_QRY_REQ;
    pstMsg->stCtrl.ulModuleId   = ulModuleId;
    pstMsg->stCtrl.usClientId   = usClientId;
    pstMsg->stCtrl.ucOpId       = ucOpId;

    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}


/*****************************************************************************
  Function Name     :   TAF_MMA_QryApPwrOnAndRegTimeReq
  Description       :   Allocate the memory for message.
                        Sends the message to MMA.
                        Returns VOS_FALSE on failure of senfing message.

  Input parameters  :   UINT32                              ulModuleId,
                        MN_CLIENT_ID_T                      ClientId,
                        MN_OPERATION_ID_T                   OpId
  Outout parameters :
  Return Value      :   VOS_FALSE or VOS_TRUE

  Modify History:
      1)  Date      :   2015-03-27
          Author    :   k902809
          Modify content :    Create
*****************************************************************************/
VOS_UINT32 TAF_MMA_QryApPwrOnAndRegTimeReq(
    VOS_UINT32                          ulModuleId,
    MN_CLIENT_ID_T                      usClientId,
    MN_OPERATION_ID_T                   ucOpId
)
{
    TAF_MMA_POWER_ON_AND_REG_TIME_QRY_REQ_STRU *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                                  ulReceiverPid;
    VOS_UINT32                                  ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* Allocating memory for message */
    pstMsg = (TAF_MMA_POWER_ON_AND_REG_TIME_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid,
                                                           sizeof(TAF_MMA_POWER_ON_AND_REG_TIME_QRY_REQ_STRU));
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( ((VOS_UINT8 *)pstMsg + VOS_MSG_HEAD_LENGTH),
                (VOS_SIZE_T)(sizeof(TAF_MMA_POWER_ON_AND_REG_TIME_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_POWER_ON_AND_REG_TIME_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    pstMsg->ulReceiverPid       = ulReceiverPid;
    pstMsg->ulSenderPid         = ulSenderPid;
    pstMsg->ulMsgName           = ID_TAF_MMA_POWER_ON_AND_REG_TIME_QRY_REQ;
    pstMsg->stCtrl.ulModuleId   = ulModuleId;
    pstMsg->stCtrl.usClientId   = usClientId;
    pstMsg->stCtrl.ucOpId       = ucOpId;

    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/* Added by k902809 for Iteration 11, Iteration 11 2015-3-23, end */

/*****************************************************************************
 函 数 名  : TAF_MMA_SetAutoAttachReq
 功能描述  : AT^CAATT命令处理
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月25日
    作    者   : y00322978
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32  TAF_MMA_SetAutoAttachReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    VOS_UINT32                          ulSetValue
)
{
    TAF_MMA_AUTO_ATTACH_SET_REQ_STRU   *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_AUTO_ATTACH_SET_REQ_STRU */
    pstMsg = (TAF_MMA_AUTO_ATTACH_SET_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                       ulSenderPid,
                                       sizeof(TAF_MMA_AUTO_ATTACH_SET_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_AUTO_ATTACH_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_AUTO_ATTACH_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_AUTO_ATTACH_SET_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;
    pstMsg->ulAutoAttachEnable          = ulSetValue;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_TestSysCfgReq
 功能描述  : TEST SYSCFG
 输入参数  : VOS_UINT32                          ulModuleId,
             VOS_UINT16                          usClientId,
             VOS_UINT8                           ucOpId
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月26日
    作    者   : y00322978
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_TestSysCfgReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_SYSCFG_TEST_REQ_STRU       *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_SYSCFG_TEST_REQ_STRU */
    pstMsg = (TAF_MMA_SYSCFG_TEST_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                       ulSenderPid,
                                       sizeof(TAF_MMA_SYSCFG_TEST_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_SYSCFG_TEST_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_SYSCFG_TEST_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_SYSCFG_TEST_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);
    return VOS_TRUE;
}


/*****************************************************************************
 函 数 名  : TAF_MMA_QryAccessModeReq
 功能描述  : Phone mode qry req
 输入参数  : VOS_UINT32                          ulModuleId
             VOS_UINT16                          usClientId
             VOS_UINT8                           ucOpId
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月27日
    作    者   : g00261581
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryAccessModeReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_ACCESS_MODE_QRY_REQ_STRU   *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_ACCESS_MODE_QRY_REQ_STRU */
    pstMsg = (TAF_MMA_ACCESS_MODE_QRY_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                       ulSenderPid,
                                       sizeof(TAF_MMA_ACCESS_MODE_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                (VOS_SIZE_T)(sizeof(TAF_MMA_ACCESS_MODE_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_ACCESS_MODE_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_ACCESS_MODE_QRY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_QryCopsInfoReq
 功能描述  : Cops Info qry req
 输入参数  : VOS_UINT32                          ulModuleId
             VOS_UINT16                          usClientId
             VOS_UINT8                           ucOpId
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月27日
    作    者   : g00261581
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryCopsInfoReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_COPS_QRY_REQ_STRU          *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_COPS_QRY_REQ_STRU */
    pstMsg = (TAF_MMA_COPS_QRY_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                        ulSenderPid,
                                        sizeof(TAF_MMA_COPS_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                (VOS_SIZE_T)(sizeof(TAF_MMA_COPS_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_COPS_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_COPS_QRY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_SetEflociInfo
 功能描述  : TAF给MMA发送EflociInfo设置请求
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年07月18日
    作    者   : h00360002
    修改内容   : 新增函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_SetEflociInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_MMA_EFLOCIINFO_STRU            *pstEfLociInfo
)
{
    TAF_MMA_EFLOCIINFO_SET_REQ_STRU    *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包 */
    pstMsg = (TAF_MMA_EFLOCIINFO_SET_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid, sizeof(TAF_MMA_EFLOCIINFO_SET_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
               (VOS_SIZE_T)(sizeof(TAF_MMA_EFLOCIINFO_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
               0x00,
               (VOS_SIZE_T)(sizeof(TAF_MMA_EFLOCIINFO_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH));

    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->enMsgName         = ID_TAF_MMA_EFLOCIINFO_SET_REQ;
    pstMsg->stCtrl.ucOpId     = ucOpId;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;

    TAF_MEM_CPY_S( &(pstMsg->stEflociInfo),
                   sizeof(TAF_MMA_EFLOCIINFO_STRU),
                   pstEfLociInfo,
                   sizeof(TAF_MMA_EFLOCIINFO_STRU));

    /* 消息发送*/
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_QryEflociInfo
 功能描述  : TAF给MMA发送EflociInfo查询请求
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年07月18日
    作    者   : h00360002
    修改内容   : 新增函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryEflociInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_EFLOCIINFO_QRY_REQ_STRU    *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包 */
    pstMsg = (TAF_MMA_EFLOCIINFO_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid, sizeof(TAF_MMA_EFLOCIINFO_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
               (VOS_SIZE_T)(sizeof(TAF_MMA_EFLOCIINFO_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
               0x00,
               (VOS_SIZE_T)(sizeof(TAF_MMA_EFLOCIINFO_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH));

    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->enMsgName         = ID_TAF_MMA_EFLOCIINFO_QRY_REQ;
    pstMsg->stCtrl.ucOpId     = ucOpId;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;

    /* 消息发送*/
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_SetPsEflociInfo
 功能描述  : TAF给MMA发送PsEflociInfo设置请求
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年07月18日
    作    者   : h00360002
    修改内容   : 新增函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_SetPsEflociInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_MMA_EFPSLOCIINFO_STRU          *pstPsefLociInfo
)
{
    TAF_MMA_EFPSLOCIINFO_SET_REQ_STRU  *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包 */
    pstMsg = (TAF_MMA_EFPSLOCIINFO_SET_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid, sizeof(TAF_MMA_EFPSLOCIINFO_SET_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
               (VOS_SIZE_T)(sizeof(TAF_MMA_EFPSLOCIINFO_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
               0x00,
               (VOS_SIZE_T)(sizeof(TAF_MMA_EFPSLOCIINFO_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH));

    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->enMsgName         = ID_TAF_MMA_EFPSLOCIINFO_SET_REQ;
    pstMsg->stCtrl.ucOpId     = ucOpId;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;

    TAF_MEM_CPY_S( &(pstMsg->stPsEflociInfo),
                   sizeof(TAF_MMA_EFPSLOCIINFO_STRU),
                   pstPsefLociInfo,
                   sizeof(TAF_MMA_EFPSLOCIINFO_STRU));

    /* 消息发送*/
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_QryPsEflociInfo
 功能描述  : TAF给MMA发送PsEflociInfo设置请求
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年07月18日
    作    者   : h00360002
    修改内容   : 新增函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryPsEflociInfo(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_EFPSLOCIINFO_QRY_REQ_STRU  *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包 */
    pstMsg = (TAF_MMA_EFPSLOCIINFO_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid, sizeof(TAF_MMA_EFPSLOCIINFO_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
               (VOS_SIZE_T)(sizeof(TAF_MMA_EFPSLOCIINFO_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
               0x00,
               (VOS_SIZE_T)(sizeof(TAF_MMA_EFPSLOCIINFO_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH));

    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->enMsgName         = ID_TAF_MMA_EFPSLOCIINFO_QRY_REQ;
    pstMsg->stCtrl.ucOpId     = ucOpId;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;

    /* 消息发送*/
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_QryDplmnListReq
 功能描述  : TAF给MMA发送 消息
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月16日
    作    者   : n00355355
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryDplmnListReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_DPLMN_QRY_REQ_STRU         *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包 */
    pstMsg = (TAF_MMA_DPLMN_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid, sizeof(TAF_MMA_DPLMN_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                (VOS_SIZE_T)(sizeof(TAF_MMA_DPLMN_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_DPLMN_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 填写消息头 */
    pstMsg->ulSenderCpuId               = VOS_LOCAL_CPUID;
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverCpuId             = VOS_LOCAL_CPUID;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_DPLMN_QRY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 消息发送*/
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_SetDplmnListReq
 功能描述  : AT给MMA发送SET DPLMN LIST REQ 消息
 输入参数  : VOS_UINT32                          ulModuleId,
             VOS_UINT16                          usClientId,
             VOS_UINT8                           ucSeq,
             VOS_UINT8                          *pucVersionId,
             VOS_UINT16                          usParaLen
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年10月13日
    作    者   : n00355355
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_SetDplmnListReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucSeq,
    VOS_UINT8                          *pucVersion,
    TAF_MMA_DPLMN_INFO_SET_STRU        *pstDplmnInfo
)
{
    TAF_MMA_DPLMN_SET_REQ_STRU         *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包 */
    pstMsg = (TAF_MMA_DPLMN_SET_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                        WUEPS_PID_TAF,
                                        sizeof(TAF_MMA_DPLMN_SET_REQ_STRU));

    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    /* 清空消息内容 */
    TAF_MEM_SET_S((VOS_UINT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
               (VOS_SIZE_T)sizeof(TAF_MMA_DPLMN_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                0x00,
               (VOS_SIZE_T)sizeof(TAF_MMA_DPLMN_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 填充消息头 */
    pstMsg->ulSenderCpuId               = VOS_LOCAL_CPUID;
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverCpuId             = VOS_LOCAL_CPUID;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_DPLMN_SET_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = 0;

    /* 填充流水号、版本号*/
    pstMsg->ucSeq                       = ucSeq;
    TAF_MEM_CPY_S( pstMsg->aucVersionId,
                sizeof(pstMsg->aucVersionId),
                pucVersion,
                TAF_MMA_VERSION_INFO_LEN);

    if ( pstDplmnInfo->ucEhPlmnNum > TAF_MMA_MAX_EHPLMN_NUM )
    {
        pstDplmnInfo->ucEhPlmnNum = TAF_MMA_MAX_EHPLMN_NUM;
    }

    if ( pstDplmnInfo->usDplmnNum > TAF_MMA_MAX_DPLMN_NUM )
    {
        pstDplmnInfo->usDplmnNum = TAF_MMA_MAX_DPLMN_NUM;
    }

    /* 填充HPLMN个数、HPLMN列表、预制DPLMN个数、DPLMN列表 */
    pstMsg->stDplmnInfo.ucEhPlmnNum     = pstDplmnInfo->ucEhPlmnNum;
    pstMsg->stDplmnInfo.usDplmnNum      = pstDplmnInfo->usDplmnNum;
    TAF_MEM_CPY_S(pstMsg->stDplmnInfo.astEhPlmnInfo,
               sizeof(pstMsg->stDplmnInfo.astEhPlmnInfo),
               pstDplmnInfo->astEhPlmnInfo,
               sizeof(TAF_PLMN_ID_STRU) * pstDplmnInfo->ucEhPlmnNum);
    TAF_MEM_CPY_S(pstMsg->stDplmnInfo.astDplmnList,
               sizeof(pstMsg->stDplmnInfo.astDplmnList),
               pstDplmnInfo->astDplmnList,
               sizeof(TAF_MMA_PLMN_WITH_SIM_RAT_STRU) * pstDplmnInfo->usDplmnNum );

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ulSenderPid, pstMsg))
    {
        return VOS_FALSE;
    }

    return VOS_TRUE;
}

/* Added by s00217060 for 边境搜网优化PhaseI, 2016-8-20, begin */
/*****************************************************************************
 函 数 名  : TAF_MMA_SetBorderInfoReq
 功能描述  : AT给MMA发送SET_BORDER_INFO_REQ 消息
 输入参数  : VOS_UINT32                          ulModuleId
             VOS_UINT16                          usClientId
             VOS_UINT8                           ucSeq
             VOS_UINT8                          *pucVersion
             TAF_MMA_DPLMN_INFO_SET_STRU        *pstDplmnInfo
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年8月20日
    作    者   : s00217060
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_SetBorderInfoReq(
    VOS_UINT32                                              ulModuleId,
    VOS_UINT16                                              usClientId,
    TAF_MMA_SET_BORDER_INFO_OPERATE_TYPE_ENUM_UINT8         enOperateType,
    TAF_MMA_BORDER_INFO_STRU                               *pstBorderInfo
)
{
    TAF_MMA_BORDER_INFO_SET_REQ_STRU   *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包 */
    pstMsg = (TAF_MMA_BORDER_INFO_SET_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                        WUEPS_PID_TAF,
                                        sizeof(TAF_MMA_BORDER_INFO_SET_REQ_STRU) + pstBorderInfo->ulBorderInfoLen - 4);

    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    /* 清空消息内容 */
    TAF_MEM_SET_S((VOS_UINT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
               (VOS_SIZE_T)sizeof(TAF_MMA_BORDER_INFO_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH + pstBorderInfo->ulBorderInfoLen - 4,
                0x00,
               (VOS_SIZE_T)sizeof(TAF_MMA_BORDER_INFO_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH + pstBorderInfo->ulBorderInfoLen - 4);

    /* 填充消息头 */
    pstMsg->ulSenderCpuId               = VOS_LOCAL_CPUID;
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverCpuId             = VOS_LOCAL_CPUID;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_BORDER_INFO_SET_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = 0;

    /* 填充消息内容 */
    pstMsg->enOperateType               = enOperateType;

    TAF_MEM_CPY_S(&(pstMsg->stBorderInfo),
                  sizeof(TAF_MMA_BORDER_INFO_STRU) + pstBorderInfo->ulBorderInfoLen - 4,
                  pstBorderInfo,
                  sizeof(TAF_MMA_BORDER_INFO_STRU) + pstBorderInfo->ulBorderInfoLen - 4);

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_QryBorderInfoReq
 功能描述  : 发送查询消息ID_TAF_MMA_BORDER_INFO_QRY_REQ
 输入参数  : VOS_UINT32                          ulModuleId
             VOS_UINT16                          usClientId
             VOS_UINT8                           ucOpId
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年8月24日
    作    者   : s00217060
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryBorderInfoReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_BORDER_INFO_QRY_REQ_STRU   *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包 */
    pstMsg = (TAF_MMA_BORDER_INFO_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid, sizeof(TAF_MMA_BORDER_INFO_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                (VOS_SIZE_T)(sizeof(TAF_MMA_BORDER_INFO_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_BORDER_INFO_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 填写消息头 */
    pstMsg->ulSenderCpuId               = VOS_LOCAL_CPUID;
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverCpuId             = VOS_LOCAL_CPUID;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_BORDER_INFO_QRY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 消息发送*/
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}
/* Added by s00217060 for 边境搜网优化PhaseI, 2016-8-20, end */

/*****************************************************************************
 函 数 名  : TAF_MMA_QryRegStateReq
 功能描述  : Qry reg state info
 输入参数  : VOS_UINT32                          ulModuleId
             VOS_UINT16                          usClientId
             VOS_UINT8                           ucOpId
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月27日
    作    者   : g00261581
    修改内容   : 新生成函数
  2.日    期   : 2015年8月21日
    作    者   : w00176964
    修改内容   : DTS2015081907463
*****************************************************************************/
VOS_UINT32 TAF_MMA_QryRegStateReq(
    VOS_UINT32                                              ulModuleId,
    VOS_UINT16                                              usClientId,
    VOS_UINT8                                               ucOpId,
    TAF_MMA_QRY_REG_STATUS_TYPE_ENUM_UINT32                 enRegStaType
)
{
    TAF_MMA_REG_STATE_QRY_REQ_STRU     *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_REG_STATE_QRY_REQ_STRU */
    pstMsg = (TAF_MMA_REG_STATE_QRY_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                              ulSenderPid,
                                              sizeof(TAF_MMA_REG_STATE_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                (VOS_SIZE_T)(sizeof(TAF_MMA_REG_STATE_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_REG_STATE_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_REG_STATE_QRY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    pstMsg->enQryRegStaType = enRegStaType;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_QryAutoAttachInfoReq
 功能描述  : Mma Qry Auto Attach Info
 输入参数  : VOS_UINT32                          ulModuleId
             VOS_UINT16                          usClientId
             VOS_UINT8                           ucOpId
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月28日
    作    者   : g00261581
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryAutoAttachInfoReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_AUTO_ATTACH_QRY_REQ_STRU    *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_AUTOATTACH_QRY_REQ_STRU */
    pstMsg = (TAF_MMA_AUTO_ATTACH_QRY_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                               ulSenderPid,
                                               sizeof(TAF_MMA_AUTO_ATTACH_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                (VOS_SIZE_T)(sizeof(TAF_MMA_AUTO_ATTACH_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_AUTO_ATTACH_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_AUTO_ATTACH_QRY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_QrySystemInfoReq
 功能描述  : Mma Qry System Info
 输入参数  : VOS_UINT32                          ulModuleId
             VOS_UINT16                          usClientId
             VOS_UINT8                           ucOpId
             VOS_UINT32                          ulSysInfoExFlag
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月28日
    作    者   : g00261581
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QrySystemInfoReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    VOS_UINT32                          ulSysInfoExFlag
)
{
    TAF_MMA_SYSINFO_QRY_REQ_STRU       *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_SYSINFO_QRY_REQ_STRU */
    pstMsg = (TAF_MMA_SYSINFO_QRY_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                            ulSenderPid,
                                            sizeof(TAF_MMA_SYSINFO_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                (VOS_SIZE_T)(sizeof(TAF_MMA_SYSINFO_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_SYSINFO_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_SYSINFO_QRY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;
    pstMsg->ulSysInfoExFlag             = ulSysInfoExFlag;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}
/* QryAntennaInfoReq 移至MTA处理 */

/*****************************************************************************
 函 数 名  : TAF_MMA_QryApHplmnInfoReq
 功能描述  : Mma Qry ApHplmn Info
 输入参数  : VOS_UINT32                          ulModuleId
             VOS_UINT16                          usClientId
             VOS_UINT8                           ucOpId
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月30日
    作    者   : g00261581
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryApHplmnInfoReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_HOME_PLMN_QRY_REQ_STRU     *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_APHPLMN_QRY_REQ_STRU */
    pstMsg = (TAF_MMA_HOME_PLMN_QRY_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                              ulSenderPid,
                                              sizeof(TAF_MMA_HOME_PLMN_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                (VOS_SIZE_T)(sizeof(TAF_MMA_HOME_PLMN_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_HOME_PLMN_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderCpuId               = VOS_LOCAL_CPUID;
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverCpuId             = VOS_LOCAL_CPUID;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_HOME_PLMN_QRY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/* QryCsnrReq移至MTA处理 */

/*****************************************************************************
 函 数 名  : TAF_MMA_QryCsqReq
 功能描述  : CSQ QRY查询消息下发接口
 输入参数  : VOS_UINT32                          ulModuleId,
             VOS_UINT16                          usClientId,
             VOS_UINT8                           ucOpId
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月28日
    作    者   : y00322978
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryCsqReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_CSQ_QRY_REQ_STRU           *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_CSQ_QRY_REQ_STRU */
    pstMsg = (TAF_MMA_CSQ_QRY_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                       ulSenderPid,
                                       sizeof(TAF_MMA_CSQ_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
            (VOS_SIZE_T)(sizeof(TAF_MMA_CSQ_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
            0x00,
            (VOS_SIZE_T)(sizeof(TAF_MMA_CSQ_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_CSQ_QRY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;

}

/* QryCsqlvlReq移至MTA处理 */



VOS_UINT32 TAF_MMA_QryBatteryCapacityReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_BATTERY_CAPACITY_QRY_REQ_STRU                  *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_CBC_QRY_REQ_STRU */
    pstMsg = (TAF_MMA_BATTERY_CAPACITY_QRY_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                        ulSenderPid,
                                        sizeof(TAF_MMA_BATTERY_CAPACITY_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                (VOS_SIZE_T)(sizeof(TAF_MMA_BATTERY_CAPACITY_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_BATTERY_CAPACITY_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_BATTERY_CAPACITY_QRY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}


VOS_UINT32 TAF_MMA_QryHandShakeReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_HAND_SHAKE_QRY_REQ_STRU    *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包TAF_MMA_HS_QRY_REQ_STRU */
    pstMsg = (TAF_MMA_HAND_SHAKE_QRY_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                       ulSenderPid,
                                       sizeof(TAF_MMA_HAND_SHAKE_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S( (VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                (VOS_SIZE_T)(sizeof(TAF_MMA_HAND_SHAKE_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
                0x00,
                (VOS_SIZE_T)(sizeof(TAF_MMA_HAND_SHAKE_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH) );

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid                 = ulSenderPid;
    pstMsg->ulReceiverPid               = ulReceiverPid;
    pstMsg->ulMsgName                   = ID_TAF_MMA_HAND_SHAKE_QRY_REQ;
    pstMsg->stCtrl.ulModuleId           = ulModuleId;
    pstMsg->stCtrl.usClientId           = usClientId;
    pstMsg->stCtrl.ucOpId               = ucOpId;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}



/*****************************************************************************
 函 数 名  : TAF_MMA_ImsRegDomainNotify
 功能描述  : SPM模块给MMA模块发送IMS注册域通知
 输入参数  : TAF_SDC_IMS_REG_DOMAIN_ENUM_UINT8           enImsRegDomain
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月4日
    作    者   : n00269697
    修改内容   : 新生成函数

  2.日    期: 2017年7月6日
    作    者: l00359089
    修改内容: 新增入参TAF_SDC_IMS_REG_STATUS_ENUM_UINT8 enImsRegStatus
*****************************************************************************/
VOS_VOID TAF_MMA_ImsRegDomainNotify(
    TAF_MMA_IMS_REG_DOMAIN_ENUM_UINT8           enImsRegDomain,
    TAF_MMA_IMS_REG_STATUS_ENUM_UINT8           enImsRegStatus
)
{
    TAF_MMA_IMS_REG_DOMAIN_NOTIFY_STRU           *pstMsg  = VOS_NULL_PTR;

    /* 申请消息包TAF_MMA_SRV_ACQ_REQ_STRU */
    pstMsg = (TAF_MMA_IMS_REG_DOMAIN_NOTIFY_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                             WUEPS_PID_TAF,
                                             sizeof(TAF_MMA_IMS_REG_DOMAIN_NOTIFY_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
               sizeof(TAF_MMA_IMS_REG_DOMAIN_NOTIFY_STRU) - VOS_MSG_HEAD_LENGTH,
               0x00,
               (VOS_SIZE_T)(sizeof(TAF_MMA_IMS_REG_DOMAIN_NOTIFY_STRU) - VOS_MSG_HEAD_LENGTH));

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid       = WUEPS_PID_TAF;
    pstMsg->ulReceiverPid     = WUEPS_PID_MMA;
    pstMsg->enMsgName         = ID_TAF_MMA_IMS_REG_DOMAIN_NOTIFY;
    pstMsg->enImsRegDomain    = enImsRegDomain;

    pstMsg->enImsRegStatus    = enImsRegStatus;

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(WUEPS_PID_TAF, pstMsg))
    {
        return;
    }

    return;
}
/*****************************************************************************
 函 数 名  : TAF_MMA_QryPacspReq
 功能描述  : 查询网络选择菜单可用性，即查询ENS是否支持
 输入参数  : VOS_UINT32                          ulModuleId
             VOS_UINT16                          usClientId
             VOS_UINT8                           ucOpId
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月9日
    作    者   : n00269697
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 TAF_MMA_QryPacspReq(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId
)
{
    TAF_MMA_PACSP_QRY_REQ_STRU         *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包 */
    pstMsg = (TAF_MMA_PACSP_QRY_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid, sizeof(TAF_MMA_PACSP_QRY_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
               (VOS_SIZE_T)(sizeof(TAF_MMA_PACSP_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH),
               0x00,
               (VOS_SIZE_T)(sizeof(TAF_MMA_PACSP_QRY_REQ_STRU) - VOS_MSG_HEAD_LENGTH));

    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->enMsgName         = ID_TAF_MMA_PACSP_QRY_REQ;
    pstMsg->stCtrl.ucOpId     = ucOpId;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;

    /* 消息发送*/
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}

/*****************************************************************************
 函 数 名  : TAF_MMA_SendSmcNoEntityNtf
 功能描述  : MSG模块给MMA通知SMC当前没有实体
 输入参数  : VOS_VOID
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年10月15日
    作    者   : n00269697
    修改内容   : 新生成函数

*****************************************************************************/
VOS_VOID TAF_MMA_SendSmcNoEntityNtf(VOS_VOID)
{
    TAF_MMA_SMC_NO_ENTITY_NOTIFY_STRU           *pstMsg  = VOS_NULL_PTR;

    pstMsg = (TAF_MMA_SMC_NO_ENTITY_NOTIFY_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                             WUEPS_PID_TAF,
                                             sizeof(TAF_MMA_SMC_NO_ENTITY_NOTIFY_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
               sizeof(TAF_MMA_SMC_NO_ENTITY_NOTIFY_STRU) - VOS_MSG_HEAD_LENGTH,
               0x00,
               (VOS_SIZE_T)(sizeof(TAF_MMA_SMC_NO_ENTITY_NOTIFY_STRU) - VOS_MSG_HEAD_LENGTH));

    /* 发送PID统一填写为WUEPS_PID_TAF */
    pstMsg->ulSenderPid       = WUEPS_PID_TAF;
    pstMsg->ulReceiverPid     = WUEPS_PID_MMA;
    pstMsg->enMsgName         = ID_TAF_MMA_SMC_NO_ENTITY_NOTIFY;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(WUEPS_PID_TAF, pstMsg);

    return;
}

/* Added by wx270776 for 适配LNAS R13协议升级迭代开发, 2017-2-13, begin */
/*****************************************************************************
 函 数 名  : TAF_MMA_AcdcAppNotify
 功能描述  : AT给MMA发送ID_TAF_MMA_ACDC_APP_NOTIFY消息
 输入参数  : VOS_UINT32                          ulModuleId,
             VOS_UINT16                          usClientId,
             VOS_UINT8                           ucOpId,
             TAF_MMA_ACDC_APP_INFO_STRU         *pstAcdcAppInfo
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年02月13日
    作    者   : wx270776
    修改内容   : 新生成函数

*****************************************************************************/
/*lint -save -e838 -specific(-e838)*/
VOS_UINT32 TAF_MMA_AcdcAppNotify(
    VOS_UINT32                          ulModuleId,
    VOS_UINT16                          usClientId,
    VOS_UINT8                           ucOpId,
    TAF_MMA_ACDC_APP_INFO_STRU         *pstAcdcAppInfo
)
{
    TAF_MMA_ACDC_APP_NOTIFY_STRU       *pstMsg = VOS_NULL_PTR;
    VOS_UINT32                          ulReceiverPid;
    VOS_UINT32                          ulSenderPid;

    ulReceiverPid = AT_GetDestPid(usClientId, WUEPS_PID_MMA);
    ulSenderPid   = AT_GetDestPid(usClientId, WUEPS_PID_TAF);

    /* 申请消息包 */
    pstMsg = (TAF_MMA_ACDC_APP_NOTIFY_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid, sizeof(TAF_MMA_ACDC_APP_NOTIFY_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_FALSE;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
               (VOS_SIZE_T)(sizeof(TAF_MMA_ACDC_APP_NOTIFY_STRU) - VOS_MSG_HEAD_LENGTH),
               0x00,
               (VOS_SIZE_T)(sizeof(TAF_MMA_ACDC_APP_NOTIFY_STRU) - VOS_MSG_HEAD_LENGTH));

    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->enMsgName         = ID_TAF_MMA_ACDC_APP_NOTIFY;
    pstMsg->stCtrl.ucOpId     = ucOpId;
    pstMsg->stCtrl.ulModuleId = ulModuleId;
    pstMsg->stCtrl.usClientId = usClientId;

    TAF_MEM_CPY_S(&(pstMsg->stAcdcAppInfo),
                  sizeof(TAF_MMA_ACDC_APP_INFO_STRU),
                  pstAcdcAppInfo,
                  sizeof(TAF_MMA_ACDC_APP_INFO_STRU));

    /* 消息发送*/
    (VOS_VOID)PS_SEND_MSG(ulSenderPid, pstMsg);

    return VOS_TRUE;
}
/*lint -restore*/
/* Added by wx270776 for 适配LNAS R13协议升级迭代开发, 2017-2-13, end */

/*****************************************************************************
 函 数 名  : TAF_MMA_SndRestartReq
 功能描述  : 发送modem重置消息
 输入参数  : 无
 输出参数  : 无
 返 回 值  : VOS_UOID
 调用函数  : 
 被调函数  : 
 
 修改历史      :
  1.日    期   : 2017年2月10日
    作    者   : c00299063
    修改内容   : 新生成函数
*****************************************************************************/
VOS_VOID TAF_MMA_SndRestartReq(
    VOS_UINT32                          ulModuleId,
    MODEM_ID_ENUM_UINT16                enModemId
)
{
    TAF_MMA_RESTART_REQ_STRU           *pstMsg;
    VOS_UINT32                          ulSenderPid;
    VOS_UINT32                          ulReceiverPid;

    if (MODEM_ID_0 == enModemId)   
    {
        ulSenderPid      = I0_WUEPS_PID_TAF;
        ulReceiverPid    = I0_WUEPS_PID_MMA;
    }
    else if (MODEM_ID_1 == enModemId)
    {
        ulSenderPid      = I1_WUEPS_PID_TAF;
        ulReceiverPid    = I1_WUEPS_PID_MMA;
    }
    else if (MODEM_ID_2 == enModemId)
    {
        ulSenderPid      = I2_WUEPS_PID_TAF;
        ulReceiverPid    = I2_WUEPS_PID_MMA;
    }
    else
    {
        return;
    }

    pstMsg = (TAF_MMA_RESTART_REQ_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(ulSenderPid, sizeof(TAF_MMA_RESTART_REQ_STRU));

    /* 内存申请失败，返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
                  sizeof(TAF_MMA_RESTART_REQ_STRU) - VOS_MSG_HEAD_LENGTH,
                  0x00,
                  (VOS_SIZE_T)(sizeof(TAF_MMA_RESTART_REQ_STRU) - VOS_MSG_HEAD_LENGTH));

    pstMsg->ulSenderPid       = ulSenderPid;
    pstMsg->ulReceiverPid     = ulReceiverPid;
    pstMsg->enMsgName         = ID_TAF_MMA_RESTART_REQ;
    pstMsg->ulModuleId        = ulModuleId;

    /* 发送消息 */
    (VOS_VOID)PS_SEND_MSG(WUEPS_PID_TAF, pstMsg);

    return;
}


