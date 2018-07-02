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

#ifndef __PSLOGFILTERCOMM_H__
#define __PSLOGFILTERCOM_H__


/******************************************************************************
  1 其他头文件包含
******************************************************************************/
#include "vos.h"
#include "PsLogFilterInterface.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)


/******************************************************************************
  2 宏定义
******************************************************************************/

/******************************************************************************
  3 枚举定义
******************************************************************************/


/******************************************************************************
  4 全局变量声明
******************************************************************************/


/******************************************************************************
  5 消息头定义
******************************************************************************/


/******************************************************************************
  6 消息定义
******************************************************************************/


/******************************************************************************
  7 STRUCT定义
******************************************************************************/

/******************************************************************************
  8 UNION定义
******************************************************************************/


/******************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
 函 数 名  : PS_OM_LayerMsgMatchFuncCommReg
 功能描述  : A C核注册MsgMatch回调接口
 输入参数  : 需要注册的MsgMatchFunc函数:
                PS_OM_LAYER_MSG_MATCH_PFUNC         pFunc,
             记录注册的MsgMatchFunc的个数，每注册一个+1:
                VOS_UINT32                         *pulRegCnt,
             存储MsgMatchFunc函数的数组:
                PS_OM_LAYER_MSG_MATCH_PFUNC         apfuncMatchEntry[],
             存储MsgMatchFunc函数的数组的最大数量:
                VOS_UINT32                          ulMsgMatchItemMaxCnt
输入参数回调接口规则:
             1.如果注册回调内部没有对消息进行处理，则需要将入参指针返回，否则
               本模块不知道是否需要将此消息传递给下一个注册回调进行处理
             2.如果注册回调内部对消息进行了处理，则返回值能够实现两个功能:
               ①返回VOS_NULL，则将此消息进行完全过滤，不会再勾取出来
               ②返回与入参指针不同的另一个指针，则勾取的消息将会使用返回的指
                 针内容替换原消息的内容。另本模块不负责对替换的内存进行释放，
                 替换原消息使用的内存请各模块自行管理。
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月3日
    作    者   : z00383822
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 PS_OM_LayerMsgMatchFuncCommReg
(
    PS_OM_LAYER_MSG_MATCH_PFUNC         pFunc,
    VOS_UINT32                         *pulRegCnt,
    PS_OM_LAYER_MSG_MATCH_PFUNC        *apfuncMatchEntry,
    VOS_UINT32                          ulMsgMatchItemMaxCnt
);


/*****************************************************************************
 函 数 名  : PS_OM_LayerMsgCommMatch
 功能描述  : A C核层间消息替换接口
 输入参数  : 需要处理的消息:
                VOS_VOID                           *pMsg,
             记录已经注册的MsgMatchFunc的个数:
                VOS_UINT32                          ulRegCnt,
             记录已经注册的MsgMatchFunc的数组:
                PS_OM_LAYER_MSG_MATCH_PFUNC         apfuncMatchEntry[],
             存储MsgMatchFunc函数的数组的最大数量:
                VOS_UINT32                          ulMsgMatchItemMaxCnt
 输出参数  : 无
 返 回 值  : VOS_VOID*
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月14日
    作    者   : z00383822
    修改内容   : 新生成函数

*****************************************************************************/
VOS_VOID* PS_OM_LayerMsgCommMatch
(
    VOS_VOID                           *pMsg,
    VOS_UINT32                          ulRegCnt,
    PS_OM_LAYER_MSG_MATCH_PFUNC        *apfuncMatchEntry,
    VOS_UINT32                          ulMsgMatchItemMaxCnt
);


/*****************************************************************************
 函 数 名  : PS_OM_LayerMsgFilterFuncReg
 功能描述  : A、C 核注册MsgMatch回调接口
 输入参数  :需要注册的MsgFilterFunc函数:
                PS_OM_LAYER_MSG_FILTER_PFUNC         pFunc,
            记录已经注册的MsgFilterFunc的个数:
                VOS_UINT32                          *pulRegCnt,
            记录已经注册的MsgFilterFunc的数组:
                PS_OM_LAYER_MSG_FILTER_PFUNC         apfuncFilterEntry[],
            存储MsgFilterFunc函数的数组的最大数量:
                VOS_UINT32                           ulMsgFilterItemMaxCnt
            输入参数回调接口规则:
             1.如果注册回调内部没有对消息进行处理，返回VOS_FALSE，否则
               本模块不知道是否需要将此消息传递给下一个注册回调进行处理
             2.如果注册回调内部对消息进行了处理，返回VOS_TRUE表示该消息
               需要进行过滤。
 输出参数  : 无
 返 回 值  : VOS_UINT32
 调用函数  :
 被调函数  :
 修改历史      :
  1.日    期   : 2016年11月3日
    作    者   : z00383822
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 PS_OM_LayerMsgFilterFuncCommReg
(
    PS_OM_LAYER_MSG_FILTER_PFUNC         pFunc,
    VOS_UINT32                          *pulRegCnt,
    PS_OM_LAYER_MSG_FILTER_PFUNC        *apfuncFilterEntry,
    VOS_UINT32                           ulMsgFilterItemMaxCnt
);


/*****************************************************************************
 函 数 名  : PS_OM_LayerMsgCommFilter
 功能描述  : A、C 核层间消息过滤接口
 输入参数  : 需要处理的消息:
                const VOS_VOID                     *pMsg,
             记录已经注册的MsgFilterFunc的个数:
                VOS_UINT32                          ulRegCnt,
             记录已经注册的MsgFilterFunc的数组:
                PS_OM_LAYER_MSG_MATCH_PFUNC         apfuncFilterEntry[],
             存储MsgFilterFunc函数的数组的最大数量:
                VOS_UINT32                          ulCount
 输出参数  : 无
 返 回 值  : VOS_UINT32:
                返回VOS_TRUE:  表示该消息需要进行过滤
                返回VOS_FALSE: 表示该消息无需进行过滤
 调用函数  :
 被调函数  :
 修改历史      :
  1.日    期   : 2016年11月3日
    作    者   : z00383822
    修改内容   : 新生成函数
*****************************************************************************/
VOS_UINT32 PS_OM_LayerMsgCommFilter
(
    const VOS_VOID                     *pMsg,
    VOS_UINT32                          ulRegCnt,
    PS_OM_LAYER_MSG_FILTER_PFUNC       *apfuncFilterEntry,
    VOS_UINT32                          ulMsgFilterItemMaxCnt
);


#pragma pack()


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* PsLogFilterComm.h */


