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
**************************************************************************** */
#include "OmCommonPpm.h"
#include "OmHsicPpm.h"
#include "cpm.h"
#include "TafOamInterface.h"


#define    THIS_FILE_ID        PS_FILE_ID_OM_HSIC_PPM_C

/* ****************************************************************************
  2 全局变量定义
**************************************************************************** */
/* HSIC和OM关联状态，默认为不关联 */
OM_HSIC_PORT_STATUS_ENUM_UINT32         g_ulOmHsicConnectStatus = OM_HSIC_PORT_STATUS_OFF;

/*互斥信号量，用来保护OM HSIC关联 */
VOS_SEM                                 g_ulOmAcpuHsicSem;

/*****************************************************************************
  3 外部引用声明
*****************************************************************************/


/*****************************************************************************
  4 函数实现
*****************************************************************************/

/*****************************************************************************
 函 数 名  :
 功能描述  :
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2014年5月31日
    作    者   : XXXXXXXX
    修改内容   : V8R1 OM_Optimize项目新增

*****************************************************************************/


/*****************************************************************************
 函 数 名  : PPM_GetHsicPortStatus
 功能描述  : OM口和HSIC关联状态
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 状态状态
 修改历史      :
  1.日    期   : 2014年5月31日
    作    者   : h59254
    修改内容   : V8R1 OM_Optimize项目新增
*****************************************************************************/
OM_HSIC_PORT_STATUS_ENUM_UINT32 PPM_GetHsicPortStatus(VOS_VOID)
{
    /* 返回OM口和HSIC关联状态 */
    return g_ulOmHsicConnectStatus;
}

/*****************************************************************************
 函 数 名  : PPM_HsicIndWriteDataCB
 功能描述  : 用于处理Hsic 口的异步发送数据的回调
 输入参数  : pucData:   需要发送的数据内容
             ulDataLen: 数据长度
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
   1.日    期  : 2012年2月2日
     作    者  : zhuli
     修改内容  : Creat Function
*****************************************************************************/
VOS_VOID PPM_HsicIndWriteDataCB(VOS_UINT8* pucVirData, VOS_UINT8* pucPhyData, VOS_INT lLen)
{
    /*当前只承载OM数据*/
    PPM_PortWriteAsyCB(OM_HSIC_IND_PORT_HANDLE, pucVirData, lLen);

    return;
}


/*****************************************************************************
 函 数 名  : PPM_HsicCfgReadDataCB
 功能描述  : 用于ACPU上面底软把HSIC CFG口数据通过ICC发送给OM模块
 输入参数  : 无
 输出参数  : 无
 返 回 值  : VOS_ERR/VOS_OK
 修改历史  :
  1.日    期   : 2014年5月31日
    作    者   : h59254
    修改内容   : V8R1 OM_Optimize项目新增
*****************************************************************************/
VOS_INT32 PPM_HsicCfgReadDataCB(VOS_VOID)
{
    if (OM_HSIC_PORT_STATUS_OFF == PPM_GetHsicPortStatus())
    {
        return VOS_OK;
    }

    return PPM_ReadPortData(CPM_HSIC_CFG_PORT, g_astOMPortUDIHandle[OM_HSIC_CFG_PORT_HANDLE], OM_HSIC_CFG_PORT_HANDLE);
}

/*****************************************************************************
 函 数 名  : PPM_HsicCfgWriteDataCB
 功能描述  : 用于处理Hsic CFG口的异步发送数据的回调
 输入参数  : pucData:   需要发送的数据内容
             ulDataLen: 数据长度
 输出参数  : 无
 返 回 值  : 无
 修改历史  :
  1.日    期   : 2014年5月31日
    作    者   : h59254
    修改内容   : V8R1 OM_Optimize项目新增
*****************************************************************************/
VOS_VOID PPM_HsicCfgWriteDataCB(VOS_UINT8* pucVirData, VOS_UINT8* pucPhyData, VOS_INT lLen)
{
    /*当前只承载OM数据*/
    PPM_PortWriteAsyCB(OM_HSIC_CFG_PORT_HANDLE, pucVirData, lLen);

    return;
}

/*****************************************************************************
 函 数 名  : PPM_HsicCfgPortOpen
 功能描述  : 用于初始化OM使用的Hsic CFG端口
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
  1.日    期   : 2014年5月31日
    作    者   : h59254
    修改内容   : V8R1 OM_Optimize项目新增
*****************************************************************************/
VOS_VOID PPM_HsicCfgPortOpen(VOS_VOID)
{

    PPM_ReadPortDataInit(CPM_HSIC_CFG_PORT,
                           OM_HSIC_CFG_PORT_HANDLE,
                           PPM_HsicCfgReadDataCB,
                           PPM_HsicCfgWriteDataCB,
                           VOS_NULL_PTR);

    return;
}

/*****************************************************************************
 函 数 名  : PPM_HsicIndPortOpen
 功能描述  : 用于初始化OM使用的Hsic IND端口
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 修改历史      :
  1.日    期   : 2014年5月31日
    作    者   : h59254
    修改内容   : V8R1 OM_Optimize项目新增
*****************************************************************************/
VOS_VOID PPM_HsicIndPortOpen(VOS_VOID)
{
    /* HSIC IND 端口不会收数据，没有断开处理 */
    PPM_ReadPortDataInit(CPM_HSIC_IND_PORT,
                           OM_HSIC_IND_PORT_HANDLE,
                           VOS_NULL_PTR,
                           PPM_HsicIndWriteDataCB,
                           VOS_NULL_PTR);

    return;
}

/*****************************************************************************
 函 数 名  : PPM_HsicIndPortClose
 功能描述  : Hsic IND端口已经消失，需要关闭USB端口
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
  1.日    期   : 2014年5月31日
    作    者   : h59254
    修改内容   : V8R1 OM_Optimize项目新增
*****************************************************************************/
VOS_VOID PPM_HsicIndPortClose(VOS_VOID)
{
    PPM_PortCloseProc(OM_HSIC_IND_PORT_HANDLE, CPM_HSIC_IND_PORT);

    return;
}

/*****************************************************************************
 函 数 名  : PPM_HsicCfgPortClose
 功能描述  : Hsic IND端口已经消失，需要关闭USB端口
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 修改历史  :
  1.日    期   : 2014年5月31日
    作    者   : h59254
    修改内容   : V8R1 OM_Optimize项目新增
*****************************************************************************/
VOS_VOID PPM_HsicCfgPortClose(VOS_VOID)
{
    PPM_PortCloseProc(OM_HSIC_CFG_PORT_HANDLE, CPM_HSIC_CFG_PORT);

    return;
}

/*****************************************************************************
 函 数 名  : GU_OamHsicIndSendData
 功能描述  : 将输入的数据通过Hsic IND发送给PC侧
 输入参数  : pucVirAddr:   数据虚地址
             pucPhyAddr:   数据实地址
             ulDataLen: 数据长度
 输出参数  : 无
 返 回 值  : VOS_ERROR/VOS_OK
 修改历史  :
  1.日    期   : 2014年5月31日
    作    者   : h59254
    修改内容   : V8R1 OM_Optimize项目新增
*****************************************************************************/
VOS_UINT32 PPM_HsicIndSendData(VOS_UINT8 *pucVirAddr, VOS_UINT8 *pucPhyAddr, VOS_UINT32 ulDataLen)
{
    return PPM_PortSend(OM_HSIC_IND_PORT_HANDLE, pucVirAddr, pucPhyAddr, ulDataLen);
}

/*****************************************************************************
 函 数 名  : PPM_HsicCfgSendData
 功能描述  : 将输入的数据通过Hsic CFG发送给PC侧
 输入参数  : pucVirAddr:   数据虚地址
             pucPhyAddr:   数据实地址
             ulDataLen: 数据长度
 输出参数  : 无
 返 回 值  : VOS_ERROR/VOS_OK
 修改历史  :
  1.日    期   : 2014年5月31日
    作    者   : h59254
    修改内容   : V8R1 OM_Optimize项目新增
*****************************************************************************/
VOS_UINT32 PPM_HsicCfgSendData(VOS_UINT8 *pucVirAddr, VOS_UINT8 *pucPhyAddr, VOS_UINT32 ulDataLen)
{
    return PPM_PortSend(OM_HSIC_CFG_PORT_HANDLE, pucVirAddr, pucPhyAddr, ulDataLen);
}

/*****************************************************************************
 函 数 名  : PPM_HsicIndStatusCB
 功能描述  : 用于ACPU上面处理HSIC IND端口断开后的OM链接断开
 输入参数  : enPortState:   端口状态
 输出参数  : 无
 返 回 值  : 无
 修改历史  :
  1.日    期   : 2014年5月31日
    作    者   : h59254
    修改内容   : V8R1 OM_Optimize项目新增
*****************************************************************************/
VOS_VOID PPM_HsicIndStatusCB(ACM_EVT_E enPortState)
{
    PPM_PortStatus(OM_HSIC_IND_PORT_HANDLE, CPM_HSIC_IND_PORT, enPortState);

    return;
}

/*****************************************************************************
 函 数 名  : PPM_HsicIndStatusCB
 功能描述  : 用于ACPU上面处理HSIC CFG端口断开后的OM链接断开
 输入参数  : enPortState:   端口状态
 输出参数  : 无
 返 回 值  : 无
 修改历史  :
  1.日    期   : 2014年5月31日
    作    者   : h59254
    修改内容   : V8R1 OM_Optimize项目新增
*****************************************************************************/
VOS_VOID PPM_HsicCfgStatusCB(ACM_EVT_E enPortState)
{
    PPM_PortStatus(OM_HSIC_CFG_PORT_HANDLE, CPM_HSIC_CFG_PORT, enPortState);

    return;
}

/*****************************************************************************
 函 数 名  : PPM_HsicPortInit
 功能描述  : 用于HSIC口通道的初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 修改历史  :
  1.日    期   : 2014年5月31日
    作    者   : h59254
    修改内容   : V8R1 OM_Optimize项目新增
*****************************************************************************/
VOS_VOID PPM_HsicPortInit(VOS_VOID)
{
    /* 产品不支持HSIC特性，直接返回 */
    if (BSP_MODULE_SUPPORT != mdrv_misc_support_check(BSP_MODULE_TYPE_HSIC))
    {
        return;
    }

    /* 如果HSIC通道已经枚举成功，则由协议栈执行初始化操作；否则将初始化函数注册至底软，
        由底软在HSIC枚举成功后调用以进行初始化*/
    if (VOS_TRUE == DRV_GET_HSIC_ENUM_STATUS())
    {
        PPM_HsicIndPortOpen();
        PPM_HsicCfgPortOpen();
    }
    else
    {
        DRV_HSIC_REGUDI_ENABLECB((HSIC_UDI_ENABLE_CB_T)PPM_HsicIndPortOpen);
        DRV_HSIC_REGUDI_ENABLECB((HSIC_UDI_ENABLE_CB_T)PPM_HsicCfgPortOpen);
    }

    DRV_HSIC_REGUDI_DISABLECB((HSIC_UDI_DISABLE_CB_T)PPM_HsicIndPortClose);
    DRV_HSIC_REGUDI_DISABLECB((HSIC_UDI_DISABLE_CB_T)PPM_HsicCfgPortClose);

    CPM_PhySendReg(CPM_HSIC_IND_PORT,  PPM_HsicIndSendData);
    CPM_PhySendReg(CPM_HSIC_CFG_PORT,  PPM_HsicCfgSendData);

    return;
}

/*****************************************************************************
 Prototype       : PPM_HsicConnectProc
 Description     : OM口和HSIC关联
 Input           : None
 Output          : None
 Return Value    : VOS_UINT32

 History         : ---
    Date         : 2012-04-09
    Author       : h59254
    Modification : AP-Modem锁网锁卡项目新增函数
 *****************************************************************************/
VOS_VOID PPM_HsicConnectProc(VOS_VOID)
{
    if (VOS_OK != VOS_SmP(g_ulOmAcpuHsicSem, 0))
    {
        return;
    }

    /* 产品不支持HSIC特性，直接初始化成功 */
    if (BSP_MODULE_SUPPORT != mdrv_misc_support_check(BSP_MODULE_TYPE_HSIC))
    {
        (VOS_VOID)VOS_SmV(g_ulOmAcpuHsicSem);

        return;
    }

    /* 如果已经关联上不做关联 */
    if (OM_HSIC_PORT_STATUS_ON == g_ulOmHsicConnectStatus)
    {
        (VOS_VOID)VOS_SmV(g_ulOmAcpuHsicSem);

        return;
    }

    /* 将全局变量设置为已关联上 */
    g_ulOmHsicConnectStatus = OM_HSIC_PORT_STATUS_ON;

    (VOS_VOID)VOS_SmV(g_ulOmAcpuHsicSem);

    return;
}

/*****************************************************************************
 Prototype       : PPM_HsicDisconnectProc
 Description     : OM口和HSIC解除关联
 Input           : None
 Output          : None
 Return Value    : VOS_UINT32

 History         : ---
    Date         : 2012-04-09
    Author       : h59254
    Modification : AP-Modem锁网锁卡项目新增函数
 *****************************************************************************/
VOS_VOID PPM_HsicDisconnectProc(VOS_VOID)
{
    /* 产品不支持HSIC特性，直接初始化成功 */
    if (BSP_MODULE_SUPPORT != mdrv_misc_support_check(BSP_MODULE_TYPE_HSIC))
    {
        return;
    }

    /* 如果已经是未关联状态不做解除关联操作 */
    if (OM_HSIC_PORT_STATUS_OFF == g_ulOmHsicConnectStatus)
    {
        return;
    }

    /* 将全局变量设置为已解除关联上 */
    g_ulOmHsicConnectStatus = OM_HSIC_PORT_STATUS_OFF;

    return;
}





