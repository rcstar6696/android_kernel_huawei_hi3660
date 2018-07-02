/******************************************************************************

                  版权所有 (C), 2001-2011, 华为技术有限公司

 ******************************************************************************
  文 件 名   : oal_mm.h
  版 本 号   : 初稿
  作    者   : mayuan
  生成日期   : 2012年12月10日
  最近修改   :
  功能描述   : oal_mm.h 的头文件
  函数列表   :
  修改历史   :
  1.日    期   : 2012年12月10日
    作    者   : mayuan
    修改内容   : 创建文件

******************************************************************************/

#ifndef __OAL_LINUX_MM_H__
#define __OAL_LINUX_MM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
/*lint -e322*/
#include <linux/slab.h>
#include <linux/hardirq.h>
#include <linux/vmalloc.h>
#ifdef _PRE_WLAN_CACHE_COHERENT_SUPPORT
#include <linux/dma-mapping.h>
#endif

/*lint +e322*/

/*****************************************************************************
  2 宏定义
*****************************************************************************/
typedef dma_addr_t  oal_dma_addr;


/*****************************************************************************
  3 枚举定义
*****************************************************************************/
typedef enum
{
    OAL_BIDIRECTIONAL = 0,
    OAL_TO_DEVICE = 1,
    OAL_FROM_DEVICE = 2,
    OAL_NONE = 3,
}oal_data_direction;
typedef oal_uint8 oal_direction_uint8;


/*****************************************************************************
  4 全局变量声明
*****************************************************************************/


/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/


/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
/*****************************************************************************
 函 数 名  : oal_memalloc
 功能描述  : 申请核心态的内存空间，并填充0。对于Linux操作系统而言，
             需要考虑中断上下文和内核上下文的不同情况(GFP_KERNEL和GFP_ATOMIC)。
 输入参数  : ul_size: alloc mem size
 输出参数  : 无
 返 回 值  : alloc mem addr
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年9月17日
    作    者   : t00231215
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void* oal_memalloc(oal_uint32 ul_size)
{
    oal_int32   l_flags = GFP_KERNEL;
    oal_void   *puc_mem_space;

    /* 不睡眠或在中断程序中标志置为GFP_ATOMIC */
    if (in_interrupt() || irqs_disabled())
    {
        l_flags = GFP_ATOMIC;
    }

    puc_mem_space = kmalloc(ul_size, l_flags);

    if (OAL_PTR_NULL == puc_mem_space)
    {
        return OAL_PTR_NULL;
    }

    return puc_mem_space;
}

#ifdef _PRE_WLAN_CACHE_COHERENT_SUPPORT
/*****************************************************************************
 函 数 名  : oal_mem_uncache_alloc
 功能描述  : 一致性内存(非cache)分配函数
 输入参数  : 待分配的mem的大小(size),返回的物理地址

 输出参数  : 返回的地址块物理地址的首地址(phy addr)
 返 回 值  : 分配后的虚拟地址
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年3月5日
    作    者   : lixiaochuan
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void* oal_mem_uncache_alloc(oal_uint32 ul_size, oal_uint32 *pul_phy_addr)
{
    oal_int32   l_flags = GFP_KERNEL;
    oal_void   *puc_mem_space;
    oal_uint32  ul_dma_real_addr;

    /* 不睡眠或在中断程序中标志置为GFP_ATOMIC */
    if (in_interrupt() || irqs_disabled())
    {
        l_flags = GFP_ATOMIC;
    }

    puc_mem_space = dma_alloc_coherent(NULL, ul_size, &ul_dma_real_addr, l_flags);

    /* 保存非cache内存的物理地址 */
    *pul_phy_addr = (oal_uint32)ul_dma_real_addr;

    if (OAL_PTR_NULL == puc_mem_space)
    {
        return OAL_PTR_NULL;
    }

    return puc_mem_space;

}

/*****************************************************************************
 函 数 名  : oal_mem_uncache_free
 功能描述  : 一致性内存(非cache)释放函数
 输入参数  : 待分配的mem的大小(size),分配内存的虚拟地址(p_buf),
             对应的物理地址(ul_dma_addr)
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年4月28日
    作    者   : lixiaochuan
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void oal_mem_uncache_free(oal_uint32 ul_size, oal_void *p_buf, oal_uint32 ul_dma_addr)
{
    dma_free_coherent(NULL, ul_size, p_buf, ul_dma_addr);
}

/*****************************************************************************
 函 数 名  : oal_dma_map_single
 功能描述  : 刷cache接口，将cache无效掉或者将cache中的内容同步到DDR
 输入参数  : 第一个参数:struct device,第二个参数:需要cache的指针，第三个参数:cache操作的size大小
             第四个参数:方向(0:双方向;1:将cache同步到DDR;2:将cache无效)
 输出参数  : 无
 返 回 值  : cache相关指针映射的物理地址
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年4月18日
    作    者   : l00347062
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_uint32 oal_dma_map_single(struct device *pst_dev, oal_void *p_buf, oal_uint32 ul_size, oal_direction_uint8 uc_dir)
{
    return dma_map_single(pst_dev, p_buf, ul_size, uc_dir);
}

/*****************************************************************************
 函 数 名  : oal_dma_unmap_single
 功能描述  : 该接口主要作为map之后映射的地址取消映射，同时对于包含L1、L2级cache，最好配之前map，配后在unmap
 输入参数  : 第一个参数:struct device,第二个参数:需要cache的指针映射的物理地址，第三个参数:cache操作的size大小
             第四个参数:方向(0:双方向;1:将cache同步到DDR;2:将cache无效)
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年4月18日
    作    者   : l00347062
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void oal_dma_unmap_single(struct device *pst_dev, oal_dma_addr ul_addr, oal_uint32 ul_size, oal_direction_uint8 uc_dir)
{
    dma_unmap_single(pst_dev, ul_addr, ul_size, uc_dir);
}
#endif

/*****************************************************************************
 函 数 名  : oal_free
 功能描述  : 释放核心态的内存空间。
 输入参数  : pbuf:需释放的内存地址
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年9月17日
    作    者   : t00231215
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void  oal_free(oal_void *p_buf)
{
    kfree(p_buf);
}

/*****************************************************************************
 函 数 名  : oal_memcopy
 功能描述  : 复制内存。不允许两块内存重叠。
 输入参数  : *p_dst: 内存复制目的地址
             *p_src: 内存复制源地址
             ul_size: 复制内存大小
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年9月18日
    作    者   : t00231215
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void oal_memcopy(oal_void *p_dst, const oal_void *p_src, oal_uint32 ul_size)
{
    memcpy(p_dst, p_src, ul_size);
}

/*****************************************************************************
 函 数 名  : oal_memmove
 功能描述  : 复制内存。允许两块内存重叠。
 输入参数  : *p_dst: mem move destination addr
             *p_src: mem move source addr
             ul_size: mem move size
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年9月18日
    作    者   : t00231215
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void  oal_memmove(oal_void *p_dst, const oal_void *p_src, oal_uint32 ul_size)
{
    memmove(p_dst, p_src, ul_size);
}

/*****************************************************************************
 函 数 名  : oal_memset
 功能描述  : 内存填充。
 输入参数  : *p_buf
             uc_char: char be set to mem
             ul_size: size of mem be set
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年9月18日
    作    者   : t00231215
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void  oal_memset(oal_void *p_buf, oal_int32 l_data, oal_uint32 ul_size)
{
    memset(p_buf, l_data, ul_size);
}


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of oal_mm.h */

