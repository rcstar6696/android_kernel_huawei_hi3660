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
#include "RnicCtx.h"
#include "RnicEntity.h"
#include "PsCommonDef.h"
#include "AtRnicInterface.h"
#include "RnicProcMsg.h"
#include "RnicDemandDialFileIO.h"
#include "RnicLog.h"
#include "RnicDebug.h"
#include "TafNvInterface.h"




/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID        PS_FILE_ID_RNIC_CTX_C


/*****************************************************************************
  2 全局变量定义
*****************************************************************************/

/* RNIC CTX,用于保存RNIC的全局变量 */
RNIC_CTX_STRU                           g_stRnicCtx;

extern RNIC_STATS_INFO_STRU                    g_astRnicStats[RNIC_NET_ID_MAX_NUM];


/*****************************************************************************
 函 数 名  : RNIC_RnicCtxInit
 功能描述  : 初始化RNIC的CTX
 输入参数  : pstRnicCtx:RNIC上下文首地址
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2011年12月06日
   作    者   : f00179208
   修改内容   : 新生成函数
 2.日    期   : 2012年6月05日
   作    者   : z60575
   修改内容   : DTS2012060508406，按需断开功耗问题修改
 3.日    期   : 2012年11月23日
   作    者   : f00179208
   修改内容   : DSDA Phase I: RNIC多实例
 3.日    期   : 2013年04月15日
   作    者   : f00179208
   修改内容   : C核单独复位项目:初始化复位信号量
 4.日    期   : 2013年07月222日
   作    者   : j00177245
   修改内容   : 清理编译warning
 5.日    期   : 2014年02月14日
   作    者   : m00217266
   修改内容   : 初始化L-C互操作项目新增加的全局变量
 6.日    期   : 2014年6月3日
   作    者   : m00217266
   修改内容   : DTS2014052902453
 7.日    期   : 2015年07月31日
   作    者   : w00316404
   修改内容   : DTS2015071401227
 8.日    期   : 2016年11月08日
   作    者   : l00373346
   修改内容   : DTS2016062107764
 9.日    期   : 2017年03月21日
   作    者   : l00373346
   修改内容   : RNIC新增NAP+GRO功能
*****************************************************************************/
VOS_VOID RNIC_InitCtx(
    RNIC_CTX_STRU                      *pstRnicCtx
)
{
    VOS_UINT8                           ucIndex;

    TAF_MEM_SET_S(g_astRnicStats, sizeof(g_astRnicStats), 0x00, sizeof(g_astRnicStats));

    /* Added by m00217266 for L-C互操作项目, 2014-2-19, begin */
    TAF_MEM_SET_S((VOS_VOID *)&g_stRnicRmnetConfigCheckInfo, sizeof(g_stRnicRmnetConfigCheckInfo), 0x00, sizeof(g_stRnicRmnetConfigCheckInfo));
    /* Added by m00217266 for L-C互操作项目, 2014-2-19, end */


    for (ucIndex = 0 ; ucIndex < RNIC_NET_ID_MAX_NUM ; ucIndex++)
    {
        /* 初始化RNIC流量统计 */
        RNIC_ClearNetDsFlowStats(ucIndex);

        /* 初始化RNIC PDP上下文 */
        RNIC_InitPdpCtx(&(pstRnicCtx->astSpecCtx[ucIndex].stPdpCtx), ucIndex);

        /* 初始化流控状态 */
        pstRnicCtx->astSpecCtx[ucIndex].enFlowCtrlStatus = RNIC_FLOW_CTRL_STATUS_STOP;

        /* 初始化模块ID */
        /* 初始化的时候就已经确定了哪张网卡对应哪个MODEM */
        pstRnicCtx->astSpecCtx[ucIndex].enModemId        = RNIC_GET_MODEM_ID_BY_NET_ID(ucIndex);

        /* Modified by l60609 for L-C互操作项目, 2014-1-14, begin */
        pstRnicCtx->astSpecCtx[ucIndex].enModemType      = RNIC_MODEM_TYPE_INSIDE;
        /* Modified by l60609 for L-C互操作项目, 2014-1-14, end */

        pstRnicCtx->astSpecCtx[ucIndex].enRmNetId        = ucIndex;
        pstRnicCtx->astSpecCtx[ucIndex].lSpePort         = RNIC_INVALID_SPE_PORT;
        pstRnicCtx->astSpecCtx[ucIndex].ulIpfPortFlg     = VOS_FALSE;

        pstRnicCtx->astSpecCtx[ucIndex].enRatType        = IMSA_RNIC_IMS_RAT_TYPE_BUTT;

    }

    /* 初始化RABID信息 */
    for (ucIndex = 0 ; ucIndex < RNIC_MODEM_ID_MAX_NUM ; ucIndex++)
    {
        RNIC_InitRabidInfo(&pstRnicCtx->astRabIdInfo[ucIndex]);
    }

    /* Added by m00217266 for L-C互操作项目, 2014-1-27, begin */
    /* 初始化PDNID信息 */
    RNIC_InitPdnIdInfo(&pstRnicCtx->stPdnIdInfo);
    /* Added by m00217266 for L-C互操作项目, 2014-1-27, end */

    /* 初始化RNIC定时器上下文 */
    RNIC_InitAllTimers(pstRnicCtx->astTimerCtx);

    /* 初始化拨号模式信息 */
    RNIC_InitDialMode(&(pstRnicCtx->stDialMode));

    /* 初始化拨号断开定时器超时次数参数统计 */
    RNIC_ClearTiDialDownExpCount();

    RNIC_SetTimer4WakeFlg(VOS_FALSE);

    /* 初始化复位信号量 */
    RNIC_InitResetSem();

    /* 初始化IPF模式信息 */
    RNIC_InitIpfMode(pstRnicCtx);

    return;
}

/*****************************************************************************
 函 数 名  : RNIC_InitRabidInfo
 功能描述  : 初始化Rabid对应的信息
 输入参数  : pstRabIdInfo:RabId信息地址
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2012年12月10日
   作    者   : f00179208
   修改内容   : 新生成函数

*****************************************************************************/
VOS_VOID RNIC_InitRabidInfo(
    RNIC_RABID_INFO_STAU               *pstRabIdInfo
)
{

    TAF_MEM_SET_S(pstRabIdInfo->aucRmNetId,
               sizeof(pstRabIdInfo->aucRmNetId),
               RNIC_RMNET_ID_BUTT,
               RNIC_RAB_ID_MAX_NUM);

    return;
}

/* Added by m00217266 for L-C互操作项目, 2014-2-8, begin */
/*****************************************************************************
 函 数 名  : RNIC_InitPdnIdInfo
 功能描述  : 初始化Pdnid对应的信息
 输入参数  : pstPdnIdInfo:PdnId信息地址
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2014年02月08日
   作    者   : m00217266
   修改内容   : 新生成函数

*****************************************************************************/
VOS_VOID RNIC_InitPdnIdInfo(
    RNIC_PDNID_INFO_STAU               *pstPdnIdInfo
)
{

    TAF_MEM_SET_S(pstPdnIdInfo->aucRmNetId,
               sizeof(pstPdnIdInfo->aucRmNetId),
               RNIC_RMNET_ID_BUTT,
               RNIC_PDN_ID_MAX);

    return;
}
/* Added by m00217266 for L-C互操作项目, 2014-2-8, end */


/*****************************************************************************
 函 数 名  : RNIC_InitPdpCtx
 功能描述  : 初始化指定网卡PDP上下文
 输入参数  : pstPdpCtx:PDP上下文首地址
             ucRmNetId:网卡ID
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2011年12月12日
   作    者   : f00179208
   修改内容   : 新生成函数
 2.日    期   : 2012年11月23日
   作    者   : f00179208
   修改内容   : DSDA Phase I: RNIC多实例
*****************************************************************************/
VOS_VOID RNIC_InitPdpCtx(
    RNIC_PDP_CTX_STRU                  *pstPdpCtx,
    VOS_UINT8                           ucRmNetId
)
{
    RNIC_InitIpv4PdpCtx(&(pstPdpCtx->stIpv4PdpInfo));
    RNIC_InitIpv6PdpCtx(&(pstPdpCtx->stIpv6PdpInfo));
    RNIC_InitIpv4v6PdpCtx(&(pstPdpCtx->stIpv4v6PdpInfo), ucRmNetId);

    IMM_ZcQueueHeadInit(&pstPdpCtx->stImsQue);

    return;
}

/*****************************************************************************
 函 数 名  : RNIC_ResetDialMode
 功能描述  : 重置拨号模式信息
 输入参数  : pstDialMode:拨号模式上下文首地址
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2013年04月15日
   作    者   : f00179208
   修改内容   : 新生成函数
*****************************************************************************/
VOS_VOID RNIC_ResetDialMode(
    RNIC_DIAL_MODE_STRU                *pstDialMode
)
{
    pstDialMode->enDialMode             = RNIC_DIAL_MODE_MANUAL;
    pstDialMode->ulIdleTime             = RNIC_DIAL_DEMA_IDLE_TIME;
    pstDialMode->enEventReportFlag      = RNIC_FORBID_EVENT_REPORT;

    return;
}

/*****************************************************************************
 函 数 名  : RNIC_InitDialMode
 功能描述  : 初始化拨号模式信息
 输入参数  : pstDialMode:拨号模式上下文首地址
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2011年12月12日
   作    者   : f00179208
   修改内容   : 新生成函数
 2.日    期   : 2012年1月31日
   作    者   : w00199382
   修改内容   : 初始化/proc/dial下的文件
*****************************************************************************/
VOS_VOID RNIC_InitDialMode(
    RNIC_DIAL_MODE_STRU                *pstDialMode
)
{
    VOS_UINT32                          ulRlst;

    pstDialMode->enDialMode             = RNIC_DIAL_MODE_MANUAL;
    pstDialMode->ulIdleTime             = RNIC_DIAL_DEMA_IDLE_TIME;
    pstDialMode->enEventReportFlag      = RNIC_FORBID_EVENT_REPORT;

    ulRlst                              = RNIC_InitDemandDialFile();

    if ( VOS_ERR == ulRlst)
    {
       RNIC_ERROR_LOG(ACPU_PID_RNIC, "RNIC_InitDialMode:RNIC_InitDemandDialFile ERR!");
    }

    return;
}

/*****************************************************************************
 函 数 名  : RNIC_InitIpv4PdpCtx
 功能描述  : 初始化IPV4 PDP上下文
 输入参数  : pstIpv4PdpCtx:IPV4 PDP上下文首地址
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2011年12月19日
   作    者   : f00179208
   修改内容   : 新生成函数

*****************************************************************************/
VOS_VOID RNIC_InitIpv4PdpCtx(
    RNIC_IPV4_PDP_INFO_STRU            *pstIpv4PdpCtx
)
{
    pstIpv4PdpCtx->enRegStatus = RNIC_PDP_REG_STATUS_DEACTIVE;
    /* Modified by l60609 for L-C互操作项目, 2014-1-14, begin */
    pstIpv4PdpCtx->ucRabId     = RNIC_RAB_ID_INVALID;
    pstIpv4PdpCtx->ucPdnId     = RNIC_PDN_ID_INVALID;
    /* Modified by l60609 for L-C互操作项目, 2014-1-14, end */
    pstIpv4PdpCtx->ulIpv4Addr  = 0;


    return;
}

/*****************************************************************************
 函 数 名  : RNIC_InitIpv6PdpCtx
 功能描述  : 初始化IPV6 PDP上下文
 输入参数  : pstIpv4PdpCtx:IPV6 PDP上下文首地址
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2011年12月19日
   作    者   : f00179208
   修改内容   : 新生成函数

*****************************************************************************/
VOS_VOID RNIC_InitIpv6PdpCtx(
    RNIC_IPV6_PDP_INFO_STRU            *pstIpv6PdpCtx
)
{
    pstIpv6PdpCtx->enRegStatus = RNIC_PDP_REG_STATUS_DEACTIVE;
    /* Modified by l60609 for L-C互操作项目, 2014-1-14, begin */
    pstIpv6PdpCtx->ucRabId     = RNIC_RAB_ID_INVALID;
    pstIpv6PdpCtx->ucPdnId     = RNIC_PDN_ID_INVALID;
    /* Modified by l60609 for L-C互操作项目, 2014-1-14, end */

    TAF_MEM_SET_S(pstIpv6PdpCtx->aucIpv6Addr,
               sizeof(pstIpv6PdpCtx->aucIpv6Addr),
               0x00,
               RNIC_MAX_IPV6_ADDR_LEN);


    return;
}

/*****************************************************************************
 函 数 名  : RNIC_InitIpv4v6PdpCtx
 功能描述  : 初始化指定网卡IPV4V6 PDP上下文
 输入参数  : pstIpv4v6PdpCtx:IPV4V6 PDP上下文首地址
             ucRmNetId:网卡ID
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2011年12月19日
   作    者   : f00179208
   修改内容   : 新生成函数
 2.日    期   : 2012年11月23日
   作    者   : f00179208
   修改内容   : DSDA Phase I: RNIC多实例
*****************************************************************************/
VOS_VOID RNIC_InitIpv4v6PdpCtx(
    RNIC_IPV4V6_PDP_INFO_STRU          *pstIpv4v6PdpCtx,
    VOS_UINT8                           ucRmNetId
)
{
    RNIC_PDP_CTX_STRU                  *pstPdpAddr;

    /* 获取PDP上下文地址 */
    pstPdpAddr                              = RNIC_GetPdpCtxAddr(ucRmNetId);

    /* Modified by l60609 for L-C互操作项目, 2014-1-14, begin */
    pstIpv4v6PdpCtx->enRegStatus            = RNIC_PDP_REG_STATUS_DEACTIVE;
    pstIpv4v6PdpCtx->ucRabId                = RNIC_RAB_ID_INVALID;
    pstIpv4v6PdpCtx->ucPdnId                = RNIC_PDN_ID_INVALID;
    /* Modified by l60609 for L-C互操作项目, 2014-1-14, end */

    pstPdpAddr->stIpv4PdpInfo.ucRabId       = RNIC_RAB_ID_INVALID;
    pstPdpAddr->stIpv6PdpInfo.ucRabId       = RNIC_RAB_ID_INVALID;

    pstIpv4v6PdpCtx->ulIpv4Addr             = 0;

    TAF_MEM_SET_S(pstIpv4v6PdpCtx->aucIpv6Addr,
               sizeof(pstIpv4v6PdpCtx->aucIpv6Addr),
               0x00,
               RNIC_MAX_IPV6_ADDR_LEN);


}

/*****************************************************************************
 函 数 名  : RNIC_InitResetSem
 功能描述  : 初始化RNIC复位的信号量
 输入参数  : 无
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2013年04月15日
   作    者   : f00179208
   修改内容   : 新生成函数
*****************************************************************************/
VOS_VOID RNIC_InitResetSem(VOS_VOID)
{
    g_stRnicCtx.hResetSem  = VOS_NULL_PTR;

    /* 分配二进制信号量 */
    if (VOS_OK != VOS_SmBCreate( "RNIC", 0, VOS_SEMA4_FIFO, &g_stRnicCtx.hResetSem))
    {
        PS_PRINTF("Create rnic acpu cnf sem failed!\r\n");
        RNIC_DBG_SET_SEM_INIT_FLAG(VOS_FALSE);
        RNIC_DBG_CREATE_BINARY_SEM_FAIL_NUM(1);

        return;
    }
    else
    {
        RNIC_DBG_SAVE_BINARY_SEM_ID(g_stRnicCtx.hResetSem);
    }

    RNIC_DBG_SET_SEM_INIT_FLAG(VOS_TRUE);

    return;
}

/*****************************************************************************
 函 数 名  : RNIC_InitIpfMode
 功能描述  : 初始化IPF模式
 输入参数  : *pstRnicCtx --- RNC上下文
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 201年11月08日
   作    者   : l00373346
   修改内容   : 新生成函数
*****************************************************************************/
VOS_VOID RNIC_InitIpfMode(
    RNIC_CTX_STRU                      *pstRnicCtx
)
{
    TAF_NV_ADS_IPF_MODE_CFG_STRU        stIpfMode;

    TAF_MEM_SET_S(&stIpfMode, (VOS_SIZE_T)sizeof(stIpfMode), 0x00, (VOS_SIZE_T)sizeof(TAF_NV_ADS_IPF_MODE_CFG_STRU));

    pstRnicCtx->ucIpfMode = 0;

    /* 读取NV项 */
    if (NV_OK == NV_ReadEx(MODEM_ID_0, en_NV_Item_ADS_IPF_MODE_CFG,
                      &stIpfMode, (VOS_UINT32)sizeof(TAF_NV_ADS_IPF_MODE_CFG_STRU)))
    {
        pstRnicCtx->ucIpfMode = stIpfMode.ucIpfMode;
    }

    return;
}



/*****************************************************************************
 函 数 名  : RNIC_ClearNetDsFlowStats
 功能描述  : 初始化流量统计信息
 输入参数  : enRmNetId --- RMNET ID
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月30日
    作    者   : A00165503
    修改内容   : 新生成函数
*****************************************************************************/
VOS_VOID RNIC_ClearNetDsFlowStats(RNIC_RMNET_ID_ENUM_UINT8 enRmNetId)
{
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt = VOS_NULL_PTR;

    pstNetCntxt = RNIC_GET_SPEC_NET_CTX(enRmNetId);

    pstNetCntxt->stDsFlowStats.ulCurrentRecvRate   = 0;
    pstNetCntxt->stDsFlowStats.ulPeriodRecvPktNum  = 0;
    pstNetCntxt->stDsFlowStats.ulTotalRecvFluxLow  = 0;
    pstNetCntxt->stDsFlowStats.ulTotalRecvFluxHigh = 0;

    pstNetCntxt->stDsFlowStats.ulCurrentSendRate   = 0;
    pstNetCntxt->stDsFlowStats.ulPeriodSendPktNum  = 0;
    pstNetCntxt->stDsFlowStats.ulTotalSendFluxLow  = 0;
    pstNetCntxt->stDsFlowStats.ulTotalSendFluxHigh = 0;

    return;
}

/*****************************************************************************
 函 数 名  : RNIC_GetRnicCtxAddr
 功能描述  : 获取当前RNIC的CTX
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 返回当前RNIC的CTX地址
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2011年12月06日
   作    者   : f00179208
   修改内容   : 新生成函数

*****************************************************************************/
RNIC_CTX_STRU* RNIC_GetRnicCtxAddr(VOS_VOID)
{
    return &(g_stRnicCtx);
}

/*****************************************************************************
 函 数 名  : RNIC_GetRnicTiDialDownExpCount
 功能描述  : 获取拨号模式统计拨号断开计数器
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 拨号模式统计拨号断开计数器
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2011年12月07日
   作    者   : f00179208
   修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32 RNIC_GetTiDialDownExpCount( VOS_VOID)
{
    return (g_stRnicCtx.ulTiDialDownExpCount);
}


/*****************************************************************************
 函 数 名  : RNIC_TiDialDownExpCountPlusOne
 功能描述  : 累加拨号模式统计拨号断开计数器
 输入参数  : 无
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2011年12月07日
   作    者   : f00179208
   修改内容   : 新生成函数

*****************************************************************************/
VOS_VOID RNIC_IncTiDialDownExpCount( VOS_VOID)
{
    (g_stRnicCtx.ulTiDialDownExpCount)++;

    return;
}

/*****************************************************************************
 函 数 名  : RNIC_ClearRnicTiDialDownExpCount
 功能描述  : 拨号模式统计拨号断开计数器清零
 输入参数  : 无
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2011年12月07日
   作    者   : f00179208
   修改内容   : 新生成函数

*****************************************************************************/
VOS_VOID RNIC_ClearTiDialDownExpCount( VOS_VOID)
{
    g_stRnicCtx.ulTiDialDownExpCount = 0;

    return;
}

/*****************************************************************************
 函 数 名  : RNIC_GetCurrentUlRate
 功能描述  : 获取指定网卡的上行速率
 输入参数  : ucRmNetId: 网卡ID
 输出参数  : 无
 返 回 值  : 当前RNIC速率的地址
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2011年12月07日
   作    者   : f00179208
   修改内容   : 新生成函数
 2.日    期   : 2012年11月23日
   作    者   : f00179208
   修改内容   : DSDA Phase I: RNIC多实例
*****************************************************************************/
VOS_UINT32 RNIC_GetCurrentUlRate(VOS_UINT8 ucRmNetId)
{
    return (g_stRnicCtx.astSpecCtx[ucRmNetId].stDsFlowStats.ulCurrentSendRate);
}

/*****************************************************************************
 函 数 名  : RNIC_SetCurrentUlRate
 功能描述  : 设置指定网卡的上行速率
 输入参数  : ulULDataRate:上行速率
             ucRmNetId: 网卡ID
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2011年12月13日
   作    者   : f00179208
   修改内容   : 新生成函数
 2.日    期   : 2012年11月23日
   作    者   : f00179208
   修改内容   : DSDA Phase I: RNIC多实例
*****************************************************************************/
VOS_VOID RNIC_SetCurrentUlRate(
    VOS_UINT32                          ulULDataRate,
    VOS_UINT8                           ucRmNetId
)
{
    g_stRnicCtx.astSpecCtx[ucRmNetId].stDsFlowStats.ulCurrentSendRate = ulULDataRate;

    return;
}


/*****************************************************************************
 函 数 名  : RNIC_GetCurrentDlRate
 功能描述  : 获取指定网卡的下行速率
 输入参数  : ucRmNetId: 网卡ID
 输出参数  : 无
 返 回 值  : 当前RNIC速率的地址
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2011年12月07日
   作    者   : f00179208
   修改内容   : 新生成函数
 2.日    期   : 2012年11月23日
   作    者   : f00179208
   修改内容   : DSDA Phase I: RNIC多实例
*****************************************************************************/
VOS_UINT32 RNIC_GetCurrentDlRate(VOS_UINT8 ucRmNetId)
{
    return (g_stRnicCtx.astSpecCtx[ucRmNetId].stDsFlowStats.ulCurrentRecvRate);
}

/*****************************************************************************
 函 数 名  : RNIC_SetCurrentDlRate
 功能描述  : 设置指定网卡的下行速率
 输入参数  : ulDLDataRate:下行速率
             ucRmNetId: 网卡ID
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2011年12月07日
   作    者   : f00179208
   修改内容   : 新生成函数
 2.日    期   : 2012年11月23日
   作    者   : f00179208
   修改内容   : DSDA Phase I: RNIC多实例
*****************************************************************************/
VOS_VOID RNIC_SetCurrentDlRate(
    VOS_UINT32                          ulDLDataRate,
    VOS_UINT8                           ucRmNetId
)
{
    g_stRnicCtx.astSpecCtx[ucRmNetId].stDsFlowStats.ulCurrentRecvRate = ulDLDataRate;

    return;
}

/*****************************************************************************
 函 数 名  : RNIC_GetDialModeAddr
 功能描述  : 获取按需拨号的模式以及时长的地址
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 按需拨号模式及时长的地址
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2011年12月07日
   作    者   : f00179208
   修改内容   : 新生成函数
*****************************************************************************/
RNIC_DIAL_MODE_STRU* RNIC_GetDialModeAddr(VOS_VOID)
{
    return &(g_stRnicCtx.stDialMode);
}

/*****************************************************************************
 函 数 名  : RNIC_GetPdpCtxAddr
 功能描述  : 获取RNIC模块指定网卡PDP上下文的地址
 输入参数  : ucRmNetId -- 网卡索引
 输出参数  : 无
 返 回 值  : RNIC模块指定网卡PDP上下文的地址
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2011年12月07日
   作    者   : f00179208
   修改内容   : 新生成函数
 2.日    期   : 2012年11月23日
   作    者   : f00179208
   修改内容   : DSDA Phase I: RNIC多实例
*****************************************************************************/
RNIC_PDP_CTX_STRU* RNIC_GetPdpCtxAddr(VOS_UINT8 ucRmNetId)
{
    return &(g_stRnicCtx.astSpecCtx[ucRmNetId].stPdpCtx);
}

/*****************************************************************************
 函 数 名  : RNIC_GetTimerAddr
 功能描述  : 获取RNIC CTX模块中定时器的上下文
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 定时器上下文地址
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2011年12月12日
   作    者   : f00179208
   修改内容   : 新生成函数

*****************************************************************************/
RNIC_TIMER_CTX_STRU*  RNIC_GetTimerAddr( VOS_VOID )
{
    return g_stRnicCtx.astTimerCtx;
}

/*****************************************************************************
 函 数 名  : RNIC_GetTimer4WakeFlg
 功能描述  : 获取RNIC CTX模块中是否设置Timer4唤醒标志
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 定时器上下文地址
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2012年6月5日
   作    者   : z60575
   修改内容   : DTS2012060508406, 新生成函数

*****************************************************************************/
VOS_UINT32 RNIC_GetTimer4WakeFlg(VOS_VOID)
{
    return g_stRnicCtx.ulSetTimer4WakeFlg;
}

/*****************************************************************************
 函 数 名  : RNIC_SetTimer4WakeFlg
 功能描述  : 设置RNIC CTX模块中是否设置Timer4唤醒标志
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 定时器上下文地址
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2012年6月5日
   作    者   : z60575
   修改内容   : DTS2012060508406, 新生成函数

*****************************************************************************/
VOS_VOID RNIC_SetTimer4WakeFlg(VOS_UINT32 ulFlg)
{
    g_stRnicCtx.ulSetTimer4WakeFlg = ulFlg;
}

/*****************************************************************************
 函 数 名  : RNIC_GetSpecNetCardCtxAddr
 功能描述  : 获取RNIC指定网卡的上下文
 输入参数  : ucIndex -- 网卡索引
 输出参数  : 无
 返 回 值  : RNIC指定网卡的上下文地址
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2012年11月24日
   作    者   : f00179208
   修改内容   : 新生成函数
*****************************************************************************/
RNIC_SPEC_CTX_STRU *RNIC_GetSpecNetCardCtxAddr(VOS_UINT8 ucRmNetId)
{
    return &(g_stRnicCtx.astSpecCtx[ucRmNetId]);
}

/*****************************************************************************
 函 数 名  : RNIC_GetNetCntxtByRmNetId
 功能描述  : 根据RMNETID获取网卡上下文指针
 输入参数  : enRmNetId --- RMNET ID
 输出参数  : 无
 返 回 值  : RNIC_SPEC_CTX_STRU*
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年3月30日
    作    者   : A00165503
    修改内容   : 新生成函数
*****************************************************************************/
RNIC_SPEC_CTX_STRU* RNIC_GetNetCntxtByRmNetId(RNIC_RMNET_ID_ENUM_UINT8 enRmNetId)
{
    if (enRmNetId >= RNIC_RMNET_ID_BUTT)
    {
        return VOS_NULL_PTR;
    }

    return RNIC_GET_SPEC_NET_CTX(enRmNetId);
}


/*****************************************************************************
 函 数 名  : RNIC_GetResetSem
 功能描述  : 获取RNIC复位信号量
 输入参数  : 无
 输出参数  : 无
 返 回 值  : RNIC复位信号量
 调用函数  :
 被调函数  :

 修改历史     :
 1.日    期   : 2013年04月15日
   作    者   : f00179208
   修改内容   : 新生成函数
*****************************************************************************/
VOS_SEM RNIC_GetResetSem(VOS_VOID)
{
    return g_stRnicCtx.hResetSem;
}



