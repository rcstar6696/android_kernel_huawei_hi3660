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
  Copyright    : 2005-2009, Huawei Tech. Co., Ltd.
  File name    : PBApi.c
  Author       : z00100318
  Version      : V200R001
  Date         : 2008-10-28
  Description  : 该C文件给出了---接口模块实现
  Function List:
  History      :
 ************************************************************************/


#include "TafTypeDef.h"
#include "si_pb.h"
#include "product_config.h"

/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define      THIS_FILE_ID     PS_FILE_ID_PBAPI_C


/*****************************************************************************
函 数 名  : SI_PB_GetReceiverPid
功能描述  : FDN激活
输入参数  : 无
输出参数  : 无
返 回 值  : SI_UINT32 函数执行结果
调用函数  : 无
被调函数  : 外部接口
History     :
1.日    期  : 2008年10月18日
  作    者  : H59254
  修改内容  : Create
*****************************************************************************/
VOS_UINT32 SI_PB_GetReceiverPid(MN_CLIENT_ID_T  ClientId, VOS_UINT32 *pulReceiverPid)
{
    MODEM_ID_ENUM_UINT16    enModemID;

    /* 调用接口获取Modem ID */
    if(VOS_OK != AT_GetModemIdFromClient(ClientId,&enModemID))
    {
        return VOS_ERR;
    }

    if(MODEM_ID_1 == enModemID)
    {
        *pulReceiverPid = I1_MAPS_PB_PID;
    }
    else if (MODEM_ID_2 == enModemID)
    {
        *pulReceiverPid = I2_MAPS_PB_PID;
    }
    else
    {
        *pulReceiverPid = I0_MAPS_PB_PID;
    }

    return VOS_OK;
}

/*****************************************************************************
函 数 名  : SI_PB_Read
功能描述  : 读取电话本记录
输入参数  : 无
输出参数  : 无
返 回 值  : SI_UINT32 函数执行结果
调用函数  : 无
被调函数  : 外部接口
History     :
1.日    期  : 2008年10月14日
  作    者  : H59254
  修改内容  : Create
*****************************************************************************/
SI_UINT32 SI_PB_Read(  MN_CLIENT_ID_T           ClientId,
                            MN_OPERATION_ID_T        OpId,
                            SI_PB_STORATE_TYPE       Storage,
                            SI_UINT16                Index1,
                            SI_UINT16                Index2)
{
    return TAF_FAILURE;
}

/*****************************************************************************
函 数 名  : SI_PB_SRead
功能描述  : 读取复合电话本记录
输入参数  : 无
输出参数  : 无
返 回 值  : SI_UINT32 函数执行结果
调用函数  : 无
被调函数  : 外部接口
History     :
1.日    期  : 2009年06月05日
  作    者  : m00128685
  修改内容  : Create
*****************************************************************************/
SI_UINT32 SI_PB_SRead(  MN_CLIENT_ID_T           ClientId,
                            MN_OPERATION_ID_T        OpId,
                            SI_PB_STORATE_TYPE       Storage,
                            SI_UINT16                Index1,
                            SI_UINT16                Index2)
{
    return TAF_FAILURE;
}

/*****************************************************************************
函 数 名  : SI_PB_Query
功能描述  : 电话本号码查询
输入参数  : 无
输出参数  : 无
返 回 值  : SI_UINT32 函数执行结果
调用函数  : 无
被调函数  : 外部接口
History     :
1.日    期  : 2008年10月14日
  作    者  : H59254
  修改内容  : Create
*****************************************************************************/
SI_UINT32 SI_PB_Query(     MN_CLIENT_ID_T           ClientId,
                                MN_OPERATION_ID_T        OpId)
{
    return TAF_FAILURE;
}

/*****************************************************************************
函 数 名  : SI_PB_Set
功能描述  : 设置当前号码本使用的存储器类型
输入参数  : 无
输出参数  : 无
返 回 值  : SI_UINT32 函数执行结果
调用函数  : 无
被调函数  : 外部接口
History     :
1.日    期  : 2008年10月14日
  作    者  : H59254
  修改内容  : Create
*****************************************************************************/
SI_UINT32 SI_PB_Set(    MN_CLIENT_ID_T           ClientId,
                            MN_OPERATION_ID_T        OpId,
                            SI_PB_STORATE_TYPE      Storage)
{
    SI_PB_SET_REQ_STRU  *pMsg;
    VOS_UINT32          ulReceiverPid;

    if(SI_PB_STORAGE_FD != Storage)
    {
        PB_ERROR_LOG("SI_PB_Set:Double Modem only support the FDN");

        return TAF_FAILURE;
    }

    if (VOS_OK != SI_PB_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PB_ERROR_LOG("SI_PB_Set:Get ulReceiverPid Error.");

        return TAF_FAILURE;
    }

    pMsg = (SI_PB_SET_REQ_STRU *)VOS_AllocMsg(WUEPS_PID_AT, sizeof(SI_PB_SET_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pMsg)
    {
        PB_ERROR_LOG("SI_PB_Set:VOS_AllocMsg Failed");

        return TAF_FAILURE;
    }

    pMsg->ulReceiverPid = ulReceiverPid;
    pMsg->ulMsgName     = SI_PB_SET_REQ;
    pMsg->usClient      = ClientId;
    pMsg->ucOpID        = OpId;
    pMsg->ulStorage     = Storage;

    if(VOS_OK != VOS_SendMsg(WUEPS_PID_AT, pMsg))
    {
        PB_ERROR_LOG("SI_PB_Set:VOS_SendMsg Failed");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}

/*****************************************************************************
函 数 名  : SI_PB_Add
功能描述  : 电话本中追加一条记录
输入参数  : 无
输出参数  : 无
返 回 值  : SI_UINT32 函数执行结果
调用函数  : 无
被调函数  : 外部接口
History     :
1.日    期  : 2008年10月14日
  作    者  : H59254
  修改内容  : Create
*****************************************************************************/
SI_UINT32 SI_PB_Add(    MN_CLIENT_ID_T          ClientId,
                            MN_OPERATION_ID_T        OpId,
                             SI_PB_STORATE_TYPE      Storage,
                             SI_PB_RECORD_STRU       *pRecord)
{
    SI_PB_ADD_REP_STRU  *pMsg;
    VOS_UINT32          ulReceiverPid;

    if (VOS_OK != SI_PB_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PB_ERROR_LOG("SI_PB_Add:Get ulReceiverPid Error.");

        return TAF_FAILURE;
    }

    if(VOS_NULL_PTR == pRecord)
    {
        PB_ERROR_LOG("SI_PB_Add:pRecord is a NULL pointer");

        return TAF_FAILURE;
    }

    /*lint -e433  修改人: j00174725; 检视人: xucheng */
    pMsg = (SI_PB_ADD_REP_STRU *)VOS_AllocMsg(WUEPS_PID_AT, sizeof(SI_PB_ADD_REP_STRU) - VOS_MSG_HEAD_LENGTH);
    /*lint +e433  修改人: j00174725; 检视人: xucheng */

    if (VOS_NULL_PTR == pMsg)
    {
        PB_ERROR_LOG("SI_PB_Add:VOS_AllocMsg Failed");

        return TAF_FAILURE;
    }

    pMsg->ulReceiverPid = ulReceiverPid;
    pMsg->ulMsgName     = SI_PB_ADD_REQ;
    pMsg->usClient      = ClientId;
    pMsg->ucOpID        = OpId;

    pMsg->ulStorage = SI_PB_STORAGE_FD;     /*只能够操作FDN号码*/

    pRecord->Index = 1;

    PAM_MEM_CPY_S(&pMsg->stRecord, sizeof(SI_PB_RECORD_STRU), pRecord, sizeof(SI_PB_RECORD_STRU));

    if(VOS_OK !=  VOS_SendMsg(WUEPS_PID_AT, pMsg))
    {
        PB_ERROR_LOG("SI_PB_Add:VOS_SendMsg Failed");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}
/*****************************************************************************
函 数 名  : SI_PB_SAdd
功能描述  : 电话本中追加一条记录
输入参数  : 无
输出参数  : 无
返 回 值  : SI_UINT32 函数执行结果
调用函数  : 无
被调函数  : 外部接口
History     :
1.日    期  : 2009年06月05日
  作    者  : m00128685
  修改内容  : Create
*****************************************************************************/
SI_UINT32 SI_PB_SAdd(    MN_CLIENT_ID_T          ClientId,
                            MN_OPERATION_ID_T        OpId,
                             SI_PB_STORATE_TYPE      Storage,
                             SI_PB_RECORD_STRU       *pRecord)
{
    return TAF_FAILURE;
}

/*****************************************************************************
函 数 名  : SI_PB_Modify
功能描述  : 电话本中追加一条记录
输入参数  : 无
输出参数  : 无
返 回 值  : SI_UINT32 函数执行结果
调用函数  : 无
被调函数  : 外部接口
History     :
1.日    期  : 2008年10月14日
  作    者  : H59254
  修改内容  : Create
*****************************************************************************/
SI_UINT32 SI_PB_Modify(    MN_CLIENT_ID_T          ClientId,
                                MN_OPERATION_ID_T       OpId,
                                SI_PB_STORATE_TYPE      Storage,
                                SI_PB_RECORD_STRU       *pRecord )
{
    SI_PB_MODIFY_REP_STRU  *pMsg;
    VOS_UINT32              ulReceiverPid;

    if (VOS_OK != SI_PB_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PB_ERROR_LOG("SI_PB_Modify:Get ulReceiverPid Error.");

        return TAF_FAILURE;
    }

    if(VOS_NULL_PTR == pRecord)
    {
        PB_ERROR_LOG("SI_PB_Modify:pRecord is a NULL pointer");

        return TAF_FAILURE;
    }

    /*lint -e433  修改人: j00174725; 检视人: xucheng */
    pMsg = (SI_PB_MODIFY_REP_STRU *)VOS_AllocMsg(WUEPS_PID_AT, sizeof(SI_PB_MODIFY_REP_STRU) - VOS_MSG_HEAD_LENGTH);
    /*lint +e433  修改人: j00174725; 检视人: xucheng */

    if (VOS_NULL_PTR == pMsg)
    {
        PB_ERROR_LOG("SI_PB_Modify:VOS_AllocMsg Failed");

        return TAF_FAILURE;
    }

    pMsg->ulReceiverPid = ulReceiverPid;
    pMsg->ulMsgName     = SI_PB_MODIFY_REQ;
    pMsg->usClient      = ClientId;
    pMsg->ucOpID        = OpId;

    pMsg->ulStorage = SI_PB_STORAGE_FD;     /*只能够操作FDN号码*/

    PAM_MEM_CPY_S(&pMsg->Record, sizeof(SI_PB_RECORD_STRU), pRecord, sizeof(SI_PB_RECORD_STRU));

    if(VOS_OK !=  VOS_SendMsg(WUEPS_PID_AT, pMsg))
    {
        PB_ERROR_LOG("SI_PB_Modify:VOS_SendMsg Failed");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}

/*****************************************************************************
函 数 名  : SI_PB_SModify
功能描述  : 电话本中追加一条记录
输入参数  : 无
输出参数  : 无
返 回 值  : SI_UINT32 函数执行结果
调用函数  : 无
被调函数  : 外部接口
History     :
1.日    期  : 2009年06月05日
  作    者  : m00128685
  修改内容  : Create
*****************************************************************************/
SI_UINT32 SI_PB_SModify(    MN_CLIENT_ID_T          ClientId,
                                MN_OPERATION_ID_T       OpId,
                                SI_PB_STORATE_TYPE      Storage,
                                SI_PB_RECORD_STRU       *pRecord )
{
    return TAF_FAILURE;
}

/*****************************************************************************
函 数 名  : SI_PB_Delete
功能描述  : 电话本中删除一条记录
输入参数  : 无
输出参数  : 无
返 回 值  : SI_UINT32 函数执行结果
调用函数  : 无
被调函数  : 外部接口
History     :
1.日    期  : 2008年10月14日
  作    者  : H59254
  修改内容  : Create
*****************************************************************************/
SI_UINT32 SI_PB_Delete(     MN_CLIENT_ID_T             ClientId,
                                MN_OPERATION_ID_T           OpId,
                                SI_PB_STORATE_TYPE          Storage,
                                SI_UINT16                   Index)
{
    SI_PB_DELETE_REQ_STRU  *pMsg;
    VOS_UINT32              ulReceiverPid;

    if (VOS_OK != SI_PB_GetReceiverPid(ClientId, &ulReceiverPid))
    {
        PB_ERROR_LOG("SI_PB_Modify:Get ulReceiverPid Error.");

        return TAF_FAILURE;
    }

    pMsg = (SI_PB_DELETE_REQ_STRU *)VOS_AllocMsg(WUEPS_PID_AT, sizeof(SI_PB_DELETE_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pMsg)
    {
        PB_ERROR_LOG("SI_PB_Delete:VOS_AllocMsg Failed");

        return TAF_FAILURE;
    }

    pMsg->ulReceiverPid = ulReceiverPid;
    pMsg->ulMsgName     = SI_PB_DELETE_REQ;
    pMsg->usClient      = ClientId;
    pMsg->ucOpID        = OpId;
    pMsg->usIndex       = Index;

    pMsg->ulStorage = SI_PB_STORAGE_FD;     /*只能够操作FDN号码*/

    if(VOS_OK !=  VOS_SendMsg(WUEPS_PID_AT, pMsg))
    {
        PB_ERROR_LOG("SI_PB_Delete:VOS_SendMsg Failed");

        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}

/*****************************************************************************
函 数 名  : SI_PB_Search
功能描述  : 电话本中查找一个记录
输入参数  : 无
输出参数  : 无
返 回 值  : SI_UINT32 函数执行结果
调用函数  : 无
被调函数  : 外部接口
History     :
1.日    期  : 2009年3月12日
  作    者  : H59254
  修改内容  : Create
*****************************************************************************/
SI_UINT32 SI_PB_Search(    MN_CLIENT_ID_T             ClientId,
                                MN_OPERATION_ID_T           OpId,
                                SI_PB_STORATE_TYPE          Storage,
                                SI_UINT8                    ucLength,
                                SI_UINT8                    *pucContent)
{
    return TAF_FAILURE;
}

/*****************************************************************************
函 数 名  :SI_PB_GetStorateType
功能描述  :获取电话本当前存储介质
输入参数  :无
输出参数  :无
返 回 值  :电话本当前存储介质

修订记录  :
1. 日    期   : 2011年05月17日
   作    者   : j00168360
   修改内容   : Creat [DTS2011042105653]，at+cpbf混合查找功能
*****************************************************************************/
VOS_UINT32 SI_PB_GetStorateType(VOS_VOID)
{
    return SI_PB_STORAGE_UNSPECIFIED;   /*返回当前未指定*/
}

/*****************************************************************************
函 数 名  :SI_PB_GetSPBFlag
功能描述  :获取复合电话本标志，由此可判断出当前是否支持复合电话本
输入参数  :无
输出参数  :无
返 回 值  :复合电话本flag

修订记录  :
1. 日    期   : 2011年05月17日
   作    者   : j00168360
   修改内容   : Creat [DTS2011042105653]，at+cpbf混合查找功能
*****************************************************************************/
VOS_UINT32 SI_PB_GetSPBFlag(VOS_VOID)
{
    return VOS_FALSE;   /*返回状态关闭*/
}







