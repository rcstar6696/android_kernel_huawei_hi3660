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

/******************************************************************************

                  版权所有 (C), 2001-2011, 华为技术有限公司

 ******************************************************************************
  文 件 名   : CbtCpm.c
  版 本 号   : 初稿
  作    者   :
  生成日期   :
  最近修改   :
  功能描述   : CBT的物理端口和逻辑通道映射管理模块
  函数列表   :
  修改历史   :
  1.日    期   : 2014年5月31日
    作    者   : L00256032
    修改内容   : V8R1 OM_Optimize项目新增

***************************************************************************** */

/*****************************************************************************
  1 头文件包含
**************************************************************************** */
#include "CbtCpm.h"



#define    THIS_FILE_ID        PS_FILE_ID_CBT_CPM_C

/* ****************************************************************************
  2 全局变量定义
**************************************************************************** */
CBTCPM_RCV_FUNC                g_pCbtRcvFunc = VOS_NULL_PTR;
CBTCPM_SEND_FUNC               g_pCbtSndFunc = VOS_NULL_PTR;

/*****************************************************************************
  3 外部引用声明
*****************************************************************************/
extern VOS_UINT32 CBTSCM_SoftDecodeDataRcv(VOS_UINT8 *pucBuffer, VOS_UINT32 ulLen);

/*AT提供给OM注册端口接收函数的接口*/
extern VOS_VOID AT_RcvFuncReg(AT_PHY_PORT_ENUM_UINT32 ulPhyPort, CBTCPM_RCV_FUNC pRcvFunc);

/*AT提供给OM查询发送数据的函数接口*/
extern CBTCPM_SEND_FUNC AT_QuerySndFunc(AT_PHY_PORT_ENUM_UINT32 ulPhyPort);

/*****************************************************************************
  4 函数实现
*****************************************************************************/

/*****************************************************************************
 函 数 名  : CBTCPM_PortRcvReg
 功能描述  : 给CBT通道注册接收函数
 输入参数  : pRecvFunc：   数据接收函数地址
 输出参数  : 无
 返 回 值  : 无
 修改历史      :
  1.日    期   : 2014年5月31日
    作    者   : h59254
    修改内容   : V8R1 OM_Optimize项目新增
*****************************************************************************/
VOS_VOID CBTCPM_PortRcvReg(CBTCPM_RCV_FUNC pRcvFunc)
{
    g_pCbtRcvFunc = pRcvFunc;

    return;
}

/*****************************************************************************
 函 数 名  : CBTCPM_PortSndReg
 功能描述  : 给CBT通道注册发送函数
 输入参数  : pSndFunc：   数据发送函数地址
 输出参数  : 无
 返 回 值  : 无
 修改历史      :
  1.日    期   : 2014年5月31日
    作    者   : h59254
    修改内容   : V8R1 OM_Optimize项目新增
*****************************************************************************/
VOS_VOID CBTCPM_PortSndReg(CBTCPM_SEND_FUNC pSndFunc)
{
    g_pCbtSndFunc = pSndFunc;

    return;
}

/*****************************************************************************
 函 数 名  : CBTCPM_GetRcvFunc
 功能描述  : 获取CBT通道接收函数地址
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 数据接收函数地址
 修改历史      :
  1.日    期   : 2014年5月31日
    作    者   : h59254
    修改内容   : V8R1 OM_Optimize项目新增
*****************************************************************************/
CBTCPM_RCV_FUNC CBTCPM_GetRcvFunc(VOS_VOID)
{
    return g_pCbtRcvFunc;
}

/*****************************************************************************
 函 数 名  : CBTCPM_GetSndFunc
 功能描述  : 获取CBT通道发送函数地址
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 数据发送函数地址
 修改历史      :
  1.日    期   : 2014年5月31日
    作    者   : h59254
    修改内容   : V8R1 OM_Optimize项目新增
*****************************************************************************/
CBTCPM_SEND_FUNC CBTCPM_GetSndFunc(VOS_VOID)
{
    return g_pCbtSndFunc;
}

/*****************************************************************************
 函 数 名  : CBTCPM_NotifyChangePort
 功能描述  : AT变更CBT的数据接收和发送函数
 输入参数  : enPhyPort:AT的物理端口号
 输出参数  : 无
 返 回 值  : VOS_OK/VOS_ERR
 修改历史      :
  1.日    期   : 2014年5月31日
    作    者   : h59254
    修改内容   : V8R1 OM_Optimize项目新增
*****************************************************************************/
VOS_UINT32 CBTCPM_NotifyChangePort(AT_PHY_PORT_ENUM_UINT32 enPhyPort)
{
    /* NAS走UART口做校准 */
    if (CPM_IND_PORT > enPhyPort)
    {
        /* 从AT获得发送数据的函数指针 */
        CBTCPM_PortSndReg(AT_QuerySndFunc(enPhyPort));

        /* 用AT的端口做校准，不从USB或VCOM上收数据 */
        CBTCPM_PortRcvReg(VOS_NULL_PTR);

        /* 将校准通道的接收函数给AT模块 */
        AT_RcvFuncReg(enPhyPort, CBTSCM_SoftDecodeDataRcv);

        return VOS_OK;
    }

    return VOS_ERR;
}





