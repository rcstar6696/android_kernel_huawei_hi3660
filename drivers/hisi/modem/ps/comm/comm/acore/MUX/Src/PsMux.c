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


/*lint -e767*/
#define    THIS_FILE_ID        PS_FILE_ID_PSMUX_C
/*lint +e767*/

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "product_config.h"


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "AtMuxInterface.h"

/*****************************************************************************
  3 函数实现
*****************************************************************************/
/*****************************************************************************
 函 数 名  : MUX_DlciDlDataSend
 功能描述  : MUX的下行数据接收函数API(STUB!!!)
 输入参数  : AT_MUX_DLCI_TYPE_ENUM_UINT8   enDlci      链路号
             VOS_UINT8                     *pData      输入数据指针
             VOS_UINT32                     ulDataLen  输入数据长度
 输出参数  : 无
 返 回 值  : 无

 调用函数  :

 被调函数  :

 修改历史      :
  1.日    期   : 2012年8月10日
    作    者   : h00163499
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 MUX_DlciDlDataSend (AT_MUX_DLCI_TYPE_ENUM_UINT8 enDlci, VOS_UINT8* pData, VOS_UINT16 usDataLen)
{
    return VOS_OK;
}

/*****************************************************************************
 函 数 名  : MUX_AtRgstUlPortCallBack
 功能描述  : AT模块向MUX模块注册的上行回调函数 (STUB)
 输入参数  : AT_MUX_DLCI_TYPE_ENUM_UINT8   enDlci      链路号
             RCV_UL_DLCI_DATA_FUNC         pFunc       上行数据处理函数指针
 输出参数  : 无
 返 回 值  : 无

 调用函数  :

 被调函数  :

 修改历史      :
  1.日    期   : 2012年8月10日
    作    者   : h00163499
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 MUX_AtRgstUlPortCallBack (AT_MUX_DLCI_TYPE_ENUM_UINT8 enDlci, RCV_UL_DLCI_DATA_FUNC pFunc)
{
    return VOS_OK;
}

/*****************************************************************************
 函 数 名  : MUX_Pid_InitFunc
 功能描述  : MUX模块 PID初始化
 输入参数  : VOS_INIT_PHASE_DEFINE ip   初始化类型
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  : 模块初始化函数

 修改历史      :
  1.日    期   : 2012年8月10日
    作    者   : h00163499
    修改内容   : 新生成函数
*****************************************************************************/
VOS_UINT32 MUX_Pid_InitFunc( enum VOS_INIT_PHASE_DEFINE ip )
{
    return VOS_OK;
}

/*****************************************************************************
 函 数 名  : MUX_AtMsgProc
 功能描述  : MUX接收来自AT模块的消息处理函数
 输入参数  : MsgBlock *pMsgBlock    从AT发来的消息

 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  : 模块初始化函数，在NDIS任务中
 被调函数  :

 修改历史      :
  1.日    期   : 2012年8月10日
    作    者   : h00163499
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 MUX_AtMsgProc( const MsgBlock *pMsgBlock )
{
    return VOS_OK;
}



