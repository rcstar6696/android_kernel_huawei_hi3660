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
#include "AdsTimerMgmt.h"
#include "AdsCtx.h"
#include "AdsDownLink.h"
#include "AdsUpLink.h"
#include "AdsDebug.h"



/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID                 PS_FILE_ID_ADS_TIMERMGMT_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/

ADS_TIMER_OPERATE_STRU                  g_astAdsTmrOperateTbl[] =
{
    { ACPU_PID_ADS_UL,  TI_ADS_UL_SEND,         VOS_TIMER_PRECISION_5,  {0},    ADS_UL_RcvTiSendExpired     },
    { ACPU_PID_ADS_UL,  TI_ADS_DSFLOW_STATS,    VOS_TIMER_NO_PRECISION, {0},    VOS_NULL_PTR                },
    { ACPU_PID_ADS_DL,  TI_ADS_DL_ADQ_EMPTY,    VOS_TIMER_PRECISION_0,  {0},    ADS_DL_RcvTiAdqEmptyExpired },
    { ACPU_PID_ADS_UL,  TI_ADS_UL_DATA_STAT,    VOS_TIMER_NO_PRECISION, {0},    VOS_NULL_PTR                }

};

/*****************************************************************************
  3 函数实现
*****************************************************************************/

/*****************************************************************************
 函 数 名  : ADS_MNTN_TraceTimerOperation
 功能描述  : 发送消息给OM模块，ADS定时器运行状态
 输入参数  : ulPid         - PID
             enTimerId     - 定时器ID
             ulTimerLen    - 定时器时长
             enTimerStatus - 定时器状态
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年8月22日
    作    者   : A00165503
    修改内容   : 新生成函数
*****************************************************************************/
VOS_VOID  ADS_MNTN_TraceTimerOperation(
    VOS_UINT32                          ulPid,
    ADS_TIMER_ID_ENUM_UINT32            enTimerId,
    VOS_UINT32                          ulTimerLen,
    ADS_TIMER_OPERATION_TYPE_ENUM_UINT8 enTimerAction,
    ADS_TIMER_STOP_CAUSE_ENUM_UINT8     enStopCause
)
{
    ADS_TIMER_INFO_STRU                 stMsg = {0};

    stMsg.ulSenderCpuId     = VOS_LOCAL_CPUID;
    stMsg.ulReceiverCpuId   = VOS_LOCAL_CPUID;
    stMsg.ulSenderPid       = ulPid;
    stMsg.ulReceiverPid     = VOS_PID_TIMER;
    stMsg.ulLength          = sizeof(ADS_TIMER_INFO_STRU) - VOS_MSG_HEAD_LENGTH;
    stMsg.enTimerId         = enTimerId;
    stMsg.ulTimerLen        = ulTimerLen;
    stMsg.enTimerAction     = enTimerAction;
    stMsg.enTimerStopCause  = enStopCause;
    stMsg.aucReserved[0]    = 0;
    stMsg.aucReserved[1]    = 0;

    DIAG_TraceReport(&stMsg);

    return;
}

/*****************************************************************************
 函 数 名  : ADS_StartTimer
 功能描述  : ADS启动定时器
 输入参数  : VOS_UINT32                          ulPid
             ADS_TIMER_ID_ENUM_UINT32            enTimerId
             VOS_UINT32                          ulLen
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年12月14日
    作    者   : 鲁琳/l60609
    修改内容   : 新生成函数

  2.日    期   : 2013年9月22日
    作    者   : A00165503
    修改内容   : DTS2013092200927: 定时器使用优化

  3.日    期   : 2016年01月25日
    作    者   : w00316404
    修改内容   : DTS2016010709647: 定时器使用句柄判断并优化

  4.日    期   : 2016年2月22日
    作    者   : A00165503
    修改内容   : DTS2016022200697: 去除句柄赋空操作, 防止定时器资源维护异常

*****************************************************************************/
VOS_VOID  ADS_StartTimer(
    ADS_TIMER_ID_ENUM_UINT32            enTimerId,
    VOS_UINT32                          ulLen
)
{
    ADS_TIMER_CTX_STRU                 *pstTiCtx      = VOS_NULL_PTR;
    ADS_TIMER_OPERATE_STRU             *pstTmrOperate = VOS_NULL_PTR;
    VOS_UINT32                          ulRet;

    /* 不在使用的定时器范围内 */
    if (enTimerId >= ADS_MAX_TIMER_NUM)
    {
        ADS_ERROR_LOG1(ACPU_PID_ADS_UL,
            "ADS_StartTimer: timer id is invalid. <enTimerId>", enTimerId);
        return;
    }


    /* 获取定时器上下文 */
    pstTmrOperate = &(g_astAdsTmrOperateTbl[enTimerId]);
    pstTiCtx      = &(g_stAdsCtx.astAdsTiCtx[enTimerId]);

    /* 定时器长度检查 */
    if (0 == ulLen)
    {
        ADS_ERROR_LOG(pstTmrOperate->ulPid,
            "ADS_StartTimer: timer len is 0,");
        return;
    }

    /* 定时器已运行 */
    if (VOS_NULL_PTR != pstTiCtx->hTimer)
    {
        return;
    }

    /* 启动定时器 */
    if (VOS_NULL_PTR == pstTmrOperate->pfnTimerStartCallBack)
    {
        ulRet = VOS_StartRelTimer(&(pstTiCtx->hTimer),
                                  pstTmrOperate->ulPid,
                                  ulLen,
                                  enTimerId,
                                  0,
                                  VOS_RELTIMER_NOLOOP,
                                  pstTmrOperate->enPrecision);
    }
    else
    {
        ulRet = VOS_StartCallBackRelTimer(&(pstTiCtx->hTimer),
                                          pstTmrOperate->ulPid,
                                          ulLen,
                                          enTimerId,
                                          0,
                                          VOS_RELTIMER_NOLOOP,
                                          pstTmrOperate->pfnTimerStartCallBack,
                                          pstTmrOperate->enPrecision);
    }

    if (VOS_OK != ulRet)
    {
        ADS_ERROR_LOG1(pstTmrOperate->ulPid,
            "ADS_StartTimer: timer start failed! <ret>", ulRet);
        return;
    }

    /*勾包ADS_TIMER_INFO_STRU*/
    ADS_MNTN_TraceTimerOperation(pstTmrOperate->ulPid, enTimerId, ulLen, ADS_TIMER_OPERATION_START, ADS_TIMER_STOP_CAUSE_ENUM_BUTT);

    return;
}

/*****************************************************************************
 函 数 名  : ADS_StopTimer
 功能描述  : ADS停止定时器
 输入参数  : VOS_UINT32                          ulPid
             ADS_TIMER_ID_ENUM_UINT32            enTimerId
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年12月7日
    作    者   : 鲁琳/l60609
    修改内容   : 新生成函数

  2.日    期   : 2016年2月22日
    作    者   : A00165503
    修改内容   : DTS2016022200697: 去除句柄赋空操作, 防止定时器资源维护异常

*****************************************************************************/
VOS_VOID ADS_StopTimer(
    VOS_UINT32                          ulPid,
    ADS_TIMER_ID_ENUM_UINT32            enTimerId,
    ADS_TIMER_STOP_CAUSE_ENUM_UINT8     enStopCause
)
{
    ADS_TIMER_CTX_STRU                 *pstTiCtx = VOS_NULL_PTR;

    /* 不在使用的定时器范围内 */
    if (enTimerId >= ADS_MAX_TIMER_NUM)
    {
        ADS_ERROR_LOG1(ACPU_PID_ADS_UL,
            "ADS_StopTimer: timer id is invalid. <enTimerId>", enTimerId);
        return;
    }

    /* 获取定时器上下文 */
    pstTiCtx = &(g_stAdsCtx.astAdsTiCtx[enTimerId]);

    /* 停止定时器 */
    if (VOS_NULL_PTR != pstTiCtx->hTimer)
    {
        (VOS_VOID)VOS_StopRelTimer(&(pstTiCtx->hTimer));
    }

    /*勾包ADS_TIMER_INFO_STRU*/
    ADS_MNTN_TraceTimerOperation(ulPid, enTimerId, 0, ADS_TIMER_OPERATION_STOP, enStopCause);

    return;
}

/*****************************************************************************
 函 数 名  : ADS_GetTimerStatus
 功能描述  : 获取ADS定时器的状态
 输入参数  : VOS_UINT32                          ulPid
             ADS_TIMER_ID_ENUM_UINT32            enTimerId
 输出参数  : 无
 返 回 值  : ADS_TIMER_STATUS_ENUM_UINT8
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2011年12月7日
    作    者   : 鲁琳/l60609
    修改内容   : 新生成函数

  2.日    期   : 2013年9月22日
    作    者   : A00165503
    修改内容   : DTS2013092200927: 定时器使用优化

  3.日    期   : 2016年2月22日
    作    者   : A00165503
    修改内容   : DTS2016022200697: 去除句柄赋空操作, 防止定时器资源维护异常

*****************************************************************************/
ADS_TIMER_STATUS_ENUM_UINT8 ADS_GetTimerStatus(
    VOS_UINT32                          ulPid,
    ADS_TIMER_ID_ENUM_UINT32            enTimerId
)
{
    ADS_TIMER_CTX_STRU                 *pstTiCtx = VOS_NULL_PTR;

    /* 不在使用的定时器范围内 */
    if (enTimerId >= ADS_MAX_TIMER_NUM)
    {
        return ASD_TIMER_STATUS_BUTT;
    }

    /* 获取定时器上下文 */
    pstTiCtx = &(g_stAdsCtx.astAdsTiCtx[enTimerId]);

    /* 检查定时器句柄 */
    if (VOS_NULL_PTR != pstTiCtx->hTimer)
    {
        return ADS_TIMER_STATUS_RUNNING;
    }

    return ADS_TIMER_STATUS_STOP;
}


