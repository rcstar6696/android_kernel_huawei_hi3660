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

#include "msp_errno.h"
#include <dms.h>
#include "dms_core.h"
#include "vos.h"




/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/

#define THIS_FILE_ID                    PS_FILE_ID_DMS_NDIS_C


/*****************************************************************************
  2 全局变量定义
*****************************************************************************/

VOS_UINT32              g_ulNdisCfgFlag     = 0xffffffffU;
USBNdisStusChgFunc      g_atConnectBrk      = NULL;
USBNdisAtRecvFunc       g_atCmdRcv          = NULL;


/*****************************************************************************
  3 函数实现
*****************************************************************************/

/*****************************************************************************
 函 数 名  : DMS_NcmOpen
 功能描述  : 打开NDIS CTRL通道
 输入参数  :

 输出参数  :
 返 回 值  : ERR_MSP_FAILURE/ERR_MSP_SUCCESS
 调用函数  :
 被调函数  :
 修改历史  :
   1.日    期  : 2012年8月27日
     作    者  : heliping
     修改内容  : Creat Function
*****************************************************************************/
VOS_UINT32 DMS_NcmOpen(VOS_VOID)
{
    VOS_UINT32                          ulRet = ERR_MSP_SUCCESS;
    UDI_HANDLE                          lHandle = UDI_INVALID_HANDLE;
    DMS_PHY_BEAR_PROPERTY_STRU         *pstPhyBearProp =NULL;
    UDI_OPEN_PARAM_S                    stParam ;

    pstPhyBearProp = DMS_GetPhyBearProperty(DMS_PHY_BEAR_USB_NCM);

    if (UDI_INVALID_HANDLE != pstPhyBearProp->lPortHandle)
    {
        return ERR_MSP_SUCCESS;
    }

    stParam.devid = UDI_NCM_CTRL_ID;

    lHandle = mdrv_udi_open(&stParam);
    if (UDI_INVALID_HANDLE == lHandle)
    {
        ulRet =  ERR_MSP_FAILURE;
    }
    else
    {
        pstPhyBearProp->lPortHandle = lHandle;

        if (VOS_OK != mdrv_udi_ioctl(lHandle , NCM_IOCTL_REG_NDIS_RESP_STATUS_FUNC, DMS_NcmWrtCB))
        {
            DMS_LOG_WARNING("DMS_NcmOpen: NCM_IOCTL_REG_NDIS_RESP_STATUS_FUNC fail.\n");
        }
    }

    /* 配置 NDIS CTRL 通道 */
    DMS_NcmProcCbReg(g_atConnectBrk,(USB_NAS_AT_CMD_RECV)g_atCmdRcv);

    return ulRet;
}

/*****************************************************************************
 函 数 名  : DMS_NcmClose
 功能描述  : NDIS CTRL通道关闭
 输入参数  :

 输出参数  :
 返 回 值  : -1/ERR_MSP_SUCCESS
 调用函数  :
 被调函数  :
 修改历史  :
   1.日    期  : 2012年8月27日
     作    者  : heliping
     修改内容  : Creat Function
*****************************************************************************/
VOS_UINT32 DMS_NcmClose(VOS_VOID)
{
    VOS_INT32                           lRet = ERR_MSP_SUCCESS;
    DMS_PHY_BEAR_PROPERTY_STRU         *pstPhyBearProp = NULL;

    pstPhyBearProp = DMS_GetPhyBearProperty(DMS_PHY_BEAR_USB_NCM);

    if (UDI_INVALID_HANDLE == pstPhyBearProp->lPortHandle)
    {
        DMS_LOG_WARNING("DMS_UsbPortClose[%d]: Already close.\n", DMS_PHY_BEAR_USB_NCM);
        return ERR_MSP_SUCCESS;
    }

    lRet = mdrv_udi_close(pstPhyBearProp->lPortHandle);

    pstPhyBearProp->lPortHandle = UDI_INVALID_HANDLE;

    return (VOS_UINT32)lRet;
}

/*****************************************************************************
 函 数 名  : DMS_NcmSendData
 功能描述  : NDIS CTRL通道数据发送接口
 输入参数  : pData: 发送buf
             ulLen: 发送长度
 输出参数  :
 返 回 值  : ERR_MSP_INVALID_PARAMETER/ERR_MSP_FAILURE/ERR_MSP_SUCCESS
 调用函数  :
 被调函数  :
 修改历史  :
   1.日    期  : 2012年8月27日
     作    者  : heliping
     修改内容  : Creat Function
*****************************************************************************/
VOS_UINT32 DMS_NcmSendData(VOS_UINT8 *pData, VOS_UINT32 ulLen)
{
    VOS_INT32                           lRet = ERR_MSP_SUCCESS;
    NCM_AT_RSP_S                        stATResponse = {0};
    DMS_PHY_BEAR_PROPERTY_STRU         *pstPhyBearProp = DMS_GetPhyBearProperty(DMS_PHY_BEAR_USB_NCM);
    VOS_UINT_PTR                        ptrAddr;

    ptrAddr = (VOS_UINT_PTR)pData;

    /*检查NDIS通道状态*/
    if (ACM_EVT_DEV_SUSPEND == pstPhyBearProp->ucChanStat)
    {
        return ERR_MSP_FAILURE;
    }

    if ((pData == NULL) || (ulLen == 0) || (ulLen > 2048))
    {
        return ERR_MSP_INVALID_PARAMETER;
    }

    stATResponse.pu8AtAnswer = pData;
    stATResponse.u32Length = ulLen;

    if (UDI_INVALID_HANDLE ==pstPhyBearProp->lPortHandle)
    {
        return ERR_MSP_FAILURE;
    }

    DMS_DBG_SDM_FUN(DMS_SDM_VCOM_WRT_NCM, (VOS_UINT32)(ptrAddr & (~0U)), ulLen, 0);

    lRet = mdrv_udi_ioctl(pstPhyBearProp->lPortHandle, NCM_IOCTL_AT_RESPONSE, &stATResponse);
    if (ERR_MSP_SUCCESS != lRet)
    {
        return ERR_MSP_FAILURE;
    }

    DMS_DBG_SDM_FUN(DMS_SDM_VCOM_WRT_SUSS_NCM, 0, 0, 0);

    return ERR_MSP_SUCCESS;
}

/*****************************************************************************
 函 数 名  : DMS_NcmStatusChangeReg
 功能描述  : ndis通道速率配置函数
 输入参数  : pPdpStru: 配置数据

 输出参数  :
 返 回 值  : ERR_MSP_INVALID_PARAMETER/-1/0
 调用函数  :
 被调函数  :
 修改历史  :
   1.日    期  : 2012年8月27日
     作    者  : heliping
     修改内容  : Creat Function
*****************************************************************************/
VOS_INT32 DMS_NcmStatusChangeReg(NAS_PRO_STRU * pPdpStru)
{
    DMS_PHY_BEAR_PROPERTY_STRU         *pstPhyBearProp = NULL;
    VOS_INT32                           lRet = -1;

    pstPhyBearProp = DMS_GetPhyBearProperty(DMS_PHY_BEAR_USB_NCM);

    if (NULL == pPdpStru)
    {
        return ERR_MSP_INVALID_PARAMETER;
    }

    if (UDI_INVALID_HANDLE == pstPhyBearProp->lPortHandle)
    {
        return lRet;
    }

    lRet = mdrv_udi_ioctl(pstPhyBearProp->lPortHandle, NCM_IOCTL_NETWORK_CONNECTION_NOTIF, (VOS_VOID*)(&pPdpStru->enActiveSatus));
    if (ERR_MSP_SUCCESS != lRet)
    {
        return lRet;
    }

    return lRet;
}

/*****************************************************************************
 函 数 名  : DMS_NcmExtFuncReg
 功能描述  : ndis通道回调函数注册函数封装
 输入参数  : connectBrk: 连接状态处理函数
             atCmdRcv: 数据接收回调函数
 输出参数  :
 返 回 值  : ERR_MSP_SUCCESS
 调用函数  :
 被调函数  :
 修改历史  :
   1.日    期  : 2012年8月27日
     作    者  : heliping
     修改内容  : Creat Function
*****************************************************************************/
int DMS_NcmExtFuncReg(USBNdisStusChgFunc connectBrk,USB_NAS_AT_CMD_RECV atCmdRcv)
{

    g_atConnectBrk = (USBNdisStusChgFunc )connectBrk;
    g_atCmdRcv     = (USBNdisAtRecvFunc )atCmdRcv;

    DMS_NcmProcCbReg((USBNdisStusChgFunc )connectBrk,(USB_NAS_AT_CMD_RECV )atCmdRcv);

    return ERR_MSP_SUCCESS;
}


/*****************************************************************************
 函 数 名  : DMS_NcmExtFuncReg
 功能描述  : ndis通道回调函数注册函数
 输入参数  : connectBrk: 连接状态处理函数
             atCmdRcv: 数据接收回调函数
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :
 修改历史  :
   1.日    期  : 2012年8月27日
     作    者  : heliping
     修改内容  : Creat Function
*****************************************************************************/
VOS_VOID DMS_NcmProcCbReg(USBNdisStusChgFunc connectBrk,USB_NAS_AT_CMD_RECV atCmdRcv)
{
    DMS_PHY_BEAR_PROPERTY_STRU         *pstPhyBearProp = NULL;
    VOS_INT32                           lRet = -1;

    if ((connectBrk == NULL) || (atCmdRcv == NULL))
    {
        return ;
    }

    /*NDIS 通道没有打开或者 通道已经被配置，则直接返回*/
    if ((g_ulNdisCfgFlag == 0xffffffffU)||(g_ulNdisCfgFlag == 0))
    {
        return ;
    }

    pstPhyBearProp = DMS_GetPhyBearProperty(DMS_PHY_BEAR_USB_NCM);

    if (UDI_INVALID_HANDLE == pstPhyBearProp->lPortHandle)
    {
        return ;
    }

    lRet = mdrv_udi_ioctl(pstPhyBearProp->lPortHandle, NCM_IOCTL_REG_CONNECT_STUS_CHG_FUNC, (USBNdisStusChgFunc)(connectBrk));
    if (ERR_MSP_SUCCESS != lRet)
    {
        return ;
    }

    lRet = mdrv_udi_ioctl (pstPhyBearProp->lPortHandle , NCM_IOCTL_REG_AT_PROCESS_FUNC, (USBNdisAtRecvFunc)(atCmdRcv));
    if (ERR_MSP_SUCCESS != lRet)
    {
        return ;
    }

    g_ulNdisCfgFlag = 0;
    return ;
}

/*****************************************************************************
 函 数 名  : DMS_SetNdisChanStatus
 功能描述  : 设置NDIS通道是否允许上报主动上报
 输入参数  : VOS_UINT8 *ucStatus

 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年10月09日
    作    者   : h00135900
    修改内容   : 新生成函数

*****************************************************************************/
VOS_VOID DMS_SetNdisChanStatus(ACM_EVT_E enStatus)
{
    DMS_PHY_BEAR_PROPERTY_STRU         *pstPhyBearProp = DMS_GetPhyBearProperty(DMS_PHY_BEAR_USB_NCM);

    pstPhyBearProp->ucChanStat = (VOS_UINT8)enStatus;
    return;
}

VOS_VOID DMS_NcmWrtCB (char* pDoneBuff, int status)
{
    if (Dms_IsStaticBuf ((VOS_UINT8*)pDoneBuff))
    {
        Dms_FreeStaticBuf((VOS_UINT8*)pDoneBuff);
    }
    else
    {
        if(pDoneBuff != NULL)
        {
            kfree(pDoneBuff);
        }
    }

    return;
}



