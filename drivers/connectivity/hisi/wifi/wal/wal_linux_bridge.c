/******************************************************************************

                  版权所有 (C), 2001-2011, 华为技术有限公司

 ******************************************************************************
  文 件 名   : wal_linux_bridge.c
  版 本 号   : 初稿
  作    者   : c00178899
  生成日期   : 2012年11月19日
  最近修改   :
  功能描述   : WAL linux桥接文件
  函数列表   :
  修改历史   :
  1.日    期   : 2012年11月19日
    作    者   : c00178899
    修改内容   : 创建文件

******************************************************************************/


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_profiling.h"
#include "oam_ext_if.h"
#include "frw_ext_if.h"
#include "wlan_spec.h"

#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
#include "hal_ext_if.h"
#endif

#include "hmac_vap.h"
#include "mac_vap.h"
#include "mac_resource.h"
#include "mac_data.h"
#include "hmac_ext_if.h"

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
#include "hmac_vap.h"
#endif

#include "wal_main.h"
#include "wal_linux_bridge.h"

#ifdef _PRE_WLAN_FEATURE_BTCOEX
#include "hmac_device.h"
#include "hmac_resource.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_LINUX_BRIDGE_C


/*****************************************************************************
  2 全局变量定义
*****************************************************************************/


/*****************************************************************************
  3 函数实现
*****************************************************************************/
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

/*****************************************************************************
 函 数 名  : wal_vap_start_xmit
 功能描述  : 挂接到VAP对应net_device结构体下的发送函数
 输入参数  : pst_buf: SKB结构体,其中data指针指向以太网头
             pst_dev: net_device结构体
 输出参数  : 无
 返 回 值  : OAL_SUCC或其它错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年11月6日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_net_dev_tx_enum  wal_vap_start_xmit(oal_netbuf_stru *pst_buf, oal_net_device_stru *pst_dev)
{
    return hmac_vap_start_xmit(pst_buf, pst_dev);
}

#endif

/*****************************************************************************
 函 数 名  : wal_bridge_vap_xmit
 功能描述  : 挂接到VAP对应net_device结构体下的发送函数
 输入参数  : pst_buf: SKB结构体,其中data指针指向以太网头
             pst_dev: net_device结构体
 输出参数  : 无
 返 回 值  : OAL_SUCC或其它错误码
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年11月6日
    作    者   : zhangheng
    修改内容   : 新生成函数

*****************************************************************************/
oal_net_dev_tx_enum  wal_bridge_vap_xmit(oal_netbuf_stru *pst_buf, oal_net_device_stru *pst_dev)
{
    return hmac_bridge_vap_xmit(pst_buf, pst_dev);
}

/*lint -e19*/
oal_module_symbol(wal_bridge_vap_xmit);
/*lint +e19*/

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

