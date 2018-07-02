/******************************************************************************

                  版权所有 (C), 2001-2011, 华为技术有限公司

 ******************************************************************************
  文 件 名   : oal_hardware.h
  版 本 号   : 初稿
  作    者   : t00231215
  生成日期   : 2012年11月29日
  最近修改   :
  功能描述   : 操作系统硬件相关接口封装
  函数列表   :
  修改历史   :
  1.日    期   : 2012年11月29日
    作    者   : t00231215
    修改内容   : 创建文件

******************************************************************************/

#ifndef __OAL_LINUX_HARDWARE_H__
#define __OAL_LINUX_HARDWARE_H__

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
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/pci.h>
#include <linux/ioport.h>
#include <linux/pm.h>
#include <linux/version.h>
#if (_PRE_CONFIG_TARGET_PRODUCT == _PRE_TARGET_PRODUCT_TYPE_E5)
#include    <linux/module.h>
#include    <linux/timer.h>
#include    <linux/init.h>
#include    <linux/version.h>
#include    <linux/ktime.h>
#include    <linux/hrtimer.h>
#include    "mdrv_timer.h"
#endif
#include <linux/gpio.h>
/*lint +e322*/

#include "oal_util.h"

/*****************************************************************************
  2 宏定义
*****************************************************************************/
typedef irq_handler_t oal_irq_handler_t;

/*BEGIN:Added by zhouqingsong/2012/2/15 for SD5115V100*/
#define OAL_HI_TIMER_REG_BASE               (0x10105000)

#define OAL_HI_TIMER_NUM                    2
#define OAL_HI_TIMER_ENABLE                 1
#define OAL_HI_TIMER_DISABLE                0
#define OAL_HI_TIMER_INT_DISABLE            1
#define OAL_HI_TIMER_INT_CLEAR              0
#define OAL_HI_TIMER_DEFAULT_PERIOD         1

#define OAL_HI_TIMER_IRQ_NO                 80            /*5113 : 5   5115:80*/

#define OAL_HI_TIMER_FREE_MODE              0         /* 1101测试新增 */
#define OAL_HI_TIMER_CYCLE_MODE             1
#define OAL_HI_TIMER_SIZE_32_BIT            1
#define OAL_HI_TIMER_WRAPPING               0
#define OAL_HI_TIMER_INT_OCCUR              1
#define OAL_HI_TIMER_INT_VALID              0x01
#define OAL_HI_TIMER_NO_DIV_FREQ            0x0

#define OAL_HI_SC_REG_BASE                  (0x10100000)
#define OAL_HI_SC_CTRL                      (OAL_HI_SC_REG_BASE + 0x0000)

#define OAL_IRQ_ENABLE                      1  /* 可以中断 */
#define OAL_IRQ_FORBIDDEN                   0  /* 禁止中断 */

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)

#if (_PRE_CONFIG_TARGET_PRODUCT == _PRE_TARGET_PRODUCT_TYPE_E5)
#ifdef _PRE_E5_750_PLATFORM
#define WL_WAKE_HOST   GPIO_0_0
#define WL_SLEEP_GPIO  GPIO_4_7
#define WL_PCIE_RESET  GPIO_4_6
#else
#define WL_WAKE_HOST   GPIO_0_7
#define WL_SLEEP_GPIO  GPIO_1_2
#define WL_PCIE_RESET  GPIO_1_3
#endif

#define OAL_IRQF_TRIGGER_NONE               IRQF_TRIGGER_NONE
#define OAL_IRQF_NO_SUSPEND                 IRQF_NO_SUSPEND
#define OAL_IRQF_SHARED                     IRQF_SHARED
#define OAL_IRQF_TRIGGER_RISING             IRQF_TRIGGER_RISING
#define OAL_IRQF_TRIGGER_FALLING            IRQF_TRIGGER_FALLING

#define OAL_IRQF_TRIGGER_HIGH               IRQF_TRIGGER_HIGH
#define OAL_IRQF_TRIGGER_LOW                IRQF_TRIGGER_LOW

#define OAL_IRQ_HANDLED                     IRQ_HANDLED

#else
#define WL_WAKE_HOST   0
#define WL_SLEEP_GPIO  0
#define WL_PCIE_RESET  0


#define OAL_IRQF_TRIGGER_NONE               0
#define OAL_IRQF_NO_SUSPEND                 0
#define OAL_IRQF_SHARED                     0
#define OAL_IRQF_TRIGGER_RISING             0
#define OAL_IRQF_TRIGGER_FALLING            0

#define OAL_IRQF_TRIGGER_HIGH               0
#define OAL_IRQF_TRIGGER_LOW                0

#define OAL_IRQ_HANDLED                     0
#endif

#endif

typedef irqreturn_t                         oal_irqreturn_t;

/*****************************************************************************
  3 枚举定义
*****************************************************************************/
typedef enum
{
    OAL_5115TIMER_ONE,
    OAL_5115TIMER_SEC,

    OAL_5115TIMER_BUTT
}oal_5115timer_enum;
typedef oal_uint8 oal_5115timer_enum_uint8;

typedef enum
{
    OAL_5115IRQ_ADRDSH = 0,
    OAL_5115IRQ_ADUDSH = 1,
    OAL_5115IRQ_ADRT = 2,
    OAL_5115IRQ_DCSWR = 3,
    OAL_5115IRQ_DTSPl0 = 4,
    OAL_5115IRQ_DTSPl1 = 5,
    OAL_5115IRQ_DDERTF = 6,
    OAL_5115IRQ_DMSC = 7,
    OAL_5115IRQ_DRMP = 8,
    OAL_5115IRQ_DTOWT = 9,
    OAL_5115IRQ_FEPE = 10,
    OAL_5115IRQ_FES = 11,
    OAL_5115IRQ_FEA = 12,
    OAL_5115IRQ_FEF = 13,
    OAL_5115IRQ_HIIA = 14,
    OAL_5115IRQ_HRI = 15,
    OAL_5115IRQ_MFSC = 16,
    OAL_5115IRQ_OIS = 17,
    OAL_5115IRQ_OMSNA = 18,
    OAL_5115IRQ_OMSNF = 19,
    OAL_5115IRQ_OSLIS = 20,
    OAL_5115IRQ_WH5RW = 21,
    OAL_5115IRQ_WH5RR = 22,
    OAL_5115IRQ_MPP   = 23,
    OAL_5115IRQ_OPCCEN  = 24,

    OAL_5115IRQ_BUTT
}oal_5115irq_enum;
typedef oal_uint8 oal_5115irq_enum_uint8;

/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/
typedef struct cpumask     *oal_cpumask;
typedef oal_uint32          oal_irq_num;

#define OAL_SA_SHIRQ        IRQF_SHARED       /* 中断类型 */

typedef oal_uint32        (*oal_irq_intr_func)(void *);
typedef oal_int32         (*oal_dbac_isr_func)(int);

typedef struct
{
    volatile oal_uint32 ul_timerx_load;
    volatile oal_uint32 ul_timerx_value;
    volatile oal_uint32 ul_timerx_control;
    volatile oal_uint32 ul_timerx_intclr;
    volatile oal_uint32 ul_timerx_ris;
    volatile oal_uint32 ul_timerx_mis;
    volatile oal_uint32 ul_timerx_bgload;
    volatile oal_uint32 ul_reserve;
} oal_hi_timerx_reg_stru;
/*timer控制寄存器*/
typedef union
{
    volatile oal_uint32 ul_value;
    struct
    {
        volatile oal_uint32 ul_oneshot: 1;                 /*选择计数模式 0：回卷计数 1：一次性计数*/
        volatile oal_uint32 ul_timersize: 1;               /*16bit|32bit计数操作模式 0：16bit 1：32bit*/
        volatile oal_uint32 ul_timerpre: 2;                /*预分频因子 00：不分频 01：4级分频 10：8级分频 11：未定义，设置相当于分频因子10*/
        volatile oal_uint32 ul_reserved0: 1;               /*保留位*/
        volatile oal_uint32 ul_intenable: 1;               /*中断屏蔽位 0：屏蔽 1：不屏蔽*/
        volatile oal_uint32 ul_timermode: 1;               /*计数模式 0：自由模式 1：周期模式*/
        volatile oal_uint32 ul_timeren: 1;                 /*定时器使能位 0：禁止 1：使能*/
        volatile oal_uint32 ul_reserved1: 24;              /*保留位*/
    } bits_stru;
} oal_hi_timer_control_union;

/*timer2_3寄存器*/
typedef struct
{
    oal_hi_timerx_reg_stru ast_timer[2];
} oal_hi_timer_reg_stru;
typedef struct
{
    oal_hi_timer_control_union  u_timerx_config;
}oal_hi_timerx_config_stru;

/* PCI驱动相关定义 */
typedef struct pci_driver       oal_pci_driver_stru;
typedef struct pci_device_id    oal_pci_device_id_stru;
typedef struct pci_dev          oal_pci_dev_stru;
typedef pm_message_t            oal_pm_message_t;

/* 中断设备结构体 */
typedef struct
{
    oal_uint32              ul_irq;                  /* 中断号 */
    oal_int32               l_irq_type;             /* 中断类型标志 */
    oal_void               *p_drv_arg;              /* 中断处理函数参数 */
    oal_int8               *pc_name;                /* 中断设备名字 只为界面友好 */
    oal_irq_intr_func       p_irq_intr_func;        /* 中断处理函数地址 */
}oal_irq_dev_stru;

typedef oal_uint8   oal_hi_timerx_index_enum_uint8;

typedef enum
{
    HI5115_TIMER_INDEX_0 = 0,
    HI5115_TIMER_INDEX_1,
    HI5115_TIMER_INDEX_2,
    HI5115_TIMER_INDEX_3,
    HI5115_TIMER_INDEX_BUTT
}oal_hi_timerx_index_enum;

/*****************************************************************************
  8 UNION定义
*****************************************************************************/

/*****************************************************************************
  4 全局变量声明
*****************************************************************************/
extern oal_hi_timer_reg_stru *g_pst_reg_timer;
extern oal_uint32 g_aul_irq_save_time[][255];

/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
/*****************************************************************************
 函 数 名  : oal_irq_free
 功能描述  : 释放中断处理程序
 输入参数  : st_osdev: 中断设备结构体
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年10月9日
    作    者   : t00231215
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void  oal_irq_free(oal_irq_dev_stru *st_osdev)
{
    free_irq(st_osdev->ul_irq, st_osdev);
}

/*****************************************************************************
 函 数 名  : oal_irq_enable
 功能描述  : 开启中断
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年10月10日
    作    者   : t00231215
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void  oal_irq_enable(oal_void)
{
    local_irq_enable();
}

/*****************************************************************************
 函 数 名  : oal_irq_disable
 功能描述  : 禁止本地中断
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年10月10日
    作    者   : t00231215
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void  oal_irq_disable(oal_void)
{
    local_irq_disable();
}

/*****************************************************************************
 函 数 名  : oal_irq_interrupt
 功能描述  : 中断服务程序
 输入参数  : l_irq: 中断号
             p_dev: 中断设备
 输出参数  : 无
 返 回 值  : IRQ_HANDLED中断程序处理完毕
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年10月9日
    作    者   : t00231215
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE irqreturn_t  oal_irq_interrupt(oal_int32 l_irq, oal_void *p_dev)
{
    oal_irq_dev_stru *st_osdev = (oal_irq_dev_stru *)p_dev;

    st_osdev->p_irq_intr_func((oal_void *)st_osdev);

    return IRQ_HANDLED;
}

/*****************************************************************************
 函 数 名  : oal_irq_setup
 功能描述  : 注册中断。
 输入参数  : st_osdev: 中断设备结构体
             fn: 中断后想执行的函数地址
 输出参数  : 无
 返 回 值  : 成功返回0； 失败返回非零；
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年10月9日
    作    者   : t00231215
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_int32  oal_irq_setup(oal_irq_dev_stru *st_osdev)
{
    oal_int32 l_err = 0;

    l_err = request_irq(st_osdev->ul_irq, oal_irq_interrupt,
                        st_osdev->l_irq_type, st_osdev->pc_name, (oal_void*)st_osdev);

    return l_err;
}

/*****************************************************************************
 函 数 名  : oal_irq_trigger
 功能描述  : 软件启动目标核的硬件中断
 输入参数  : mask: 目标核核掩码
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年10月17日
    作    者   : t00231215
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void  oal_irq_trigger(oal_uint8 uc_cpuid)
{

}
/*****************************************************************************
 函 数 名  : oal_gpio_is_valid
 功能描述  :
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年1月6日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_int32  oal_gpio_is_valid(oal_int32 i_number)
{
    return gpio_is_valid(i_number);
}

/*****************************************************************************
 函 数 名  : oal_gpio_request
 功能描述  :
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年1月6日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_int32  oal_gpio_request(oal_uint32 ul_gpio, OAL_CONST oal_int8 *pc_label)
{
    return gpio_request(ul_gpio, pc_label);
}
/*****************************************************************************
 函 数 名  : oal_gpio_free
 功能描述  :
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年1月6日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void  oal_gpio_free(oal_uint32 ul_gpio)
{
    gpio_free(ul_gpio);
}

/*****************************************************************************
 函 数 名  : oal_gpio_direction_input
 功能描述  :
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年1月6日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_int32  oal_gpio_direction_input(oal_uint32 ul_gpio)
{
    return gpio_direction_input(ul_gpio);

}
/*****************************************************************************
 函 数 名  : oal_gpio_direction_output
 功能描述  :
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年1月6日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_int32  oal_gpio_direction_output(oal_uint32 ul_gpio, oal_int l_level)
{
    return gpio_direction_output(ul_gpio, l_level);
}

/*****************************************************************************
 函 数 名  : oal_gpio_to_irq
 功能描述  :
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年1月6日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_int32  oal_gpio_to_irq(oal_uint32 ul_gpio)
{
    return gpio_to_irq(ul_gpio);
}

/*****************************************************************************
 函 数 名  : oal_request_irq
 功能描述  :
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年1月6日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_int32  oal_request_irq(oal_uint32             ul_irq,
                                                     oal_irq_handler_t      p_handler,
                                                     oal_uint32             ul_flags,
                                                     OAL_CONST oal_int8    *p_name,
                                                     oal_void              *p_dev)
{
    /* TBD: 参数3待讨论 */

    return request_irq(ul_irq, p_handler, ul_flags, p_name, p_dev);
}

/*****************************************************************************
 函 数 名  : oal_gpio_get_value
 功能描述  :
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年1月6日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_int32  oal_gpio_get_value(oal_uint32 ul_gpio)
{
    return gpio_get_value(ul_gpio);
}

/*****************************************************************************
 函 数 名  : oal_gpio_set_value
 功能描述  :
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年1月6日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void  oal_gpio_set_value(oal_uint32 ul_gpio,oal_int32 value)
{
     gpio_set_value(ul_gpio,value);

}


/*****************************************************************************
 函 数 名  : oal_wifi_reg_on_pull_up
 功能描述  :
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年1月6日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void  oal_wifi_reg_on_pull_up(oal_int32 wifi_gpio_addr)
{
    if (!oal_gpio_is_valid(wifi_gpio_addr))
    {
        OAL_IO_PRINT("wifi_reg_on_pull_up:fail to get wifi gpio!\n");
        return;
    }
    /*如果已经上过电，则直接返回*/
    if (1 == oal_gpio_get_value(wifi_gpio_addr))
    {
        OAL_IO_PRINT("wifi_reg_on_pull_up:WL_REG_ON has been pulled up in wifi_reg_on_pull_up!!!\n");
        return;
    }
    oal_mdelay(500);
    oal_gpio_direction_output(wifi_gpio_addr,1);
    oal_mdelay(500);
}

/*****************************************************************************
 函 数 名  : oal_wifi_reg_on_pull_down
 功能描述  :
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年1月6日
    作    者   : huxiaotong
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void  oal_wifi_reg_on_pull_down(oal_int32 wifi_gpio_addr)
{
    if (!oal_gpio_is_valid(wifi_gpio_addr))
    {
        OAL_IO_PRINT("wifi_reg_on_pull_down:fail to get wifi gpio!\n");
        return;
    }
    /*如果已经下过电，则直接返回*/
    if (0 == oal_gpio_get_value(wifi_gpio_addr))
    {
        OAL_IO_PRINT("wifi_reg_on_pull_down:WL_REG_ON has been pulled down in wifi_reg_on_pull_down!!!\n");
        return;
    }

    oal_gpio_direction_output(wifi_gpio_addr,0);
    oal_mdelay(500);
}
#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
/*****************************************************************************
 函 数 名  : oal_5115timer_get_10ns
 功能描述  : 获取5115硬件定时器10ns精度时间戳
 输入参数  : en_timer: 有两个定时器选一个
 输出参数  : 无
 返 回 值  : 10ns精度的时间戳
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年1月26日
    作    者   : t00231215
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_uint32  oal_5115timer_get_10ns(oal_void)
{
    /* 02 待后续实现内容 TBD */
    return 1;
}

#else
/*****************************************************************************
 函 数 名  : oal_5115timer_get_10ns
 功能描述  : 获取5115硬件定时器10ns精度时间戳
 输入参数  : en_timer: 有两个定时器选一个
 输出参数  : 无
 返 回 值  : 10ns精度的时间戳
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年1月26日
    作    者   : t00231215
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_uint32  oal_5115timer_get_10ns(oal_void)
{
#if(_PRE_TARGET_PRODUCT_TYPE_WS835DMB == _PRE_CONFIG_TARGET_PRODUCT) //产品采用了该中断的第二个定时器
    return g_pst_reg_timer->ast_timer[OAL_5115TIMER_ONE].ul_timerx_value;
#elif(_PRE_TARGET_PRODUCT_TYPE_E5 == _PRE_CONFIG_TARGET_PRODUCT)
    /* E5 产品暂无硬件定时器资源 */
    return 1;
#else
    return g_pst_reg_timer->ast_timer[OAL_5115TIMER_SEC].ul_timerx_value;
#endif
}
#endif
/*****************************************************************************
 函 数 名  : oal_irq_save
 功能描述  : 禁止中断,获取中断状态
 输入参数  : pui_flags: 中断状态寄存器
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年9月21日
    作    者   : t00231215
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void  oal_irq_save(oal_uint *pui_flags, oal_uint32 ul_type)
{
#ifdef _PRE_DEBUG_MODE
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

    oal_uint32 ul_core_id = OAL_GET_CORE_ID();
#endif
#endif
    local_irq_save(*pui_flags);
#ifdef _PRE_DEBUG_MODE
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

    /* 数组最后一个用来保存save时间 */
    /* 数组最后第二个用来保存save的类型，其他用来保存各类型的最大save - restore的时间*/
    /* 每次restore的时候需要清空save时间，用来判断有无重复save */
        if (g_aul_irq_save_time[ul_core_id][254] == 0)
        {
            g_aul_irq_save_time[ul_core_id][254] = oal_5115timer_get_10ns();
            g_aul_irq_save_time[ul_core_id][253] = ul_type;
        }
        else
        {
            /* 重复save */
            OAL_IO_PRINT("\n core %d oal_irq_save[%d] failed, already saved by [%d] \n",ul_core_id, ul_type, g_aul_irq_save_time[ul_core_id][253]);
            oal_dump_stack();
        }
#endif

#endif
}

/*****************************************************************************
 函 数 名  : oal_irq_restore
 功能描述  : 恢复中断,恢复中断状态
 输入参数  : *pui_flags: 中断状态地址
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年9月21日
    作    者   : t00231215
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void  oal_irq_restore(oal_uint *pui_flags, oal_uint32 ul_type)
{
#ifdef _PRE_DEBUG_MODE
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    oal_uint32 ul_restore_time;
    oal_uint32 ul_core_id = OAL_GET_CORE_ID();

    if (g_aul_irq_save_time[ul_core_id][254] != 0)
    {
        /* restore时，需要判断上次save的type是否相同，不相同为非法 */
        if ((ul_type < 253) && (g_aul_irq_save_time[ul_core_id][253] == ul_type))
        {
            ul_restore_time = g_aul_irq_save_time[ul_core_id][254] - oal_5115timer_get_10ns();

            /* 记录这个类型的save - restore 最大值 */
            if (g_aul_irq_save_time[ul_core_id][ul_type] < ul_restore_time)
            {
                g_aul_irq_save_time[ul_core_id][ul_type] = ul_restore_time;
            }
        }
        else
        {
            /* restore出错 */
            OAL_IO_PRINT("\n core %d oal_irq_restore[%d] failed, should be [%d] \n",ul_core_id, ul_type, g_aul_irq_save_time[ul_core_id][253]);
            oal_dump_stack();
        }
        g_aul_irq_save_time[ul_core_id][254] = 0;
        g_aul_irq_save_time[ul_core_id][253] = -1;
    }
    else
    {
        /* 重复restore */
        printk("\n core %d oal_irq_restore[%d] failed, already restored \n",ul_core_id, ul_type);
        oal_dump_stack();
    }
#endif
#endif
    local_irq_restore(*pui_flags);
}

/*****************************************************************************
 函 数 名  : oal_irq_set_affinity
 功能描述  : 将指定的中断号绑定到指定的cpu上执行
 输入参数  : ul_irq: 中断号
             mask: cpu掩码
 输出参数  : 无
 返 回 值  : 成功返回OAL_SUCC,失败返回OAL_FAIL
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2012年10月10日
    作    者   : t00231215
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_int32  oal_irq_set_affinity(oal_irq_num irq, oal_uint32 ul_cpu)
{
    oal_int32 l_ret;
    struct cpumask mask;

    cpumask_clear(&mask);
    cpumask_set_cpu(ul_cpu, &mask);

    l_ret = irq_set_affinity(irq, &mask);

    if (l_ret != 0)
    {
        return OAL_FAIL;
    }

    return OAL_SUCC;
}
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)

/*****************************************************************************
 函 数 名  : oal_5115timer_init
 功能描述  : 初始化5115硬件定时器
 输入参数  : en_timer:要用的定时器
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年1月25日
    作    者   : t00231215
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void  oal_5115timer_init(oal_void)
{
#if (_PRE_TARGET_PRODUCT_TYPE_E5 != _PRE_CONFIG_TARGET_PRODUCT)
    oal_hi_timer_control_union u_reg_control;

    g_pst_reg_timer = (oal_hi_timer_reg_stru *)ioremap(OAL_HI_TIMER_REG_BASE, sizeof(oal_hi_timer_reg_stru));

    /*读timer控制器*/
#if(_PRE_TARGET_PRODUCT_TYPE_WS835DMB == _PRE_CONFIG_TARGET_PRODUCT) //产品采用了该中断的第二个定时器
    u_reg_control.ul_value = g_pst_reg_timer->ast_timer[OAL_5115TIMER_ONE].ul_timerx_control;
#else
    u_reg_control.ul_value = g_pst_reg_timer->ast_timer[OAL_5115TIMER_SEC].ul_timerx_control;
#endif
    /* 计数模式为自由模式 */
    u_reg_control.bits_stru.ul_timermode = OAL_HI_TIMER_FREE_MODE;

    /*不分频*/
    u_reg_control.bits_stru.ul_timerpre = OAL_HI_TIMER_NO_DIV_FREQ;

    /* 屏蔽中断 */
    u_reg_control.bits_stru.ul_intenable = OAL_HI_TIMER_INT_CLEAR;

    /*配置为32bit计数操作模式*/
    u_reg_control.bits_stru.ul_timersize = OAL_HI_TIMER_SIZE_32_BIT;

    /*配置为回卷计数*/
    u_reg_control.bits_stru.ul_oneshot = OAL_HI_TIMER_WRAPPING;

    /*使能寄存器*/
    u_reg_control.bits_stru.ul_timeren = OAL_TRUE;       /* HI_TRUE_E */

    /*写回timer控制器*/
#if(_PRE_TARGET_PRODUCT_TYPE_WS835DMB == _PRE_CONFIG_TARGET_PRODUCT) //产品采用了该中断的第二个定时器
    g_pst_reg_timer->ast_timer[OAL_5115TIMER_ONE].ul_timerx_control = u_reg_control.ul_value;
#else
    g_pst_reg_timer->ast_timer[OAL_5115TIMER_SEC].ul_timerx_control = u_reg_control.ul_value;
#endif
#endif
}

/*****************************************************************************
 函 数 名  : oal_5115timer_exit
 功能描述  : 释放硬件定时器虚拟内存
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年1月26日
    作    者   : t00231215
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void  oal_5115timer_exit(oal_void)
{
#if (_PRE_TARGET_PRODUCT_TYPE_E5 != _PRE_CONFIG_TARGET_PRODUCT)
    iounmap(g_pst_reg_timer);
#endif
}
#else

/*****************************************************************************
 函 数 名  : oal_5115timer_init
 功能描述  : 初始化5115硬件定时器
 输入参数  : en_timer:要用的定时器
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年1月25日
    作    者   : t00231215
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void  oal_5115timer_init(oal_void)
{

}

/*****************************************************************************
 函 数 名  : oal_5115timer_exit
 功能描述  : 释放硬件定时器虚拟内存
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2013年1月26日
    作    者   : t00231215
    修改内容   : 新生成函数

*****************************************************************************/
OAL_STATIC OAL_INLINE oal_void  oal_5115timer_exit(oal_void)
{
}

#endif

/*****************************************************************************
 函 数 名  : oal_mdrv_timer_start
 功能描述  : E5平台dbac用硬件定时器相关,定义在:drv_timer.h
 输入参数  : ul_id:定时器ID；p_func:回调函数;l_arg:无;ul_timer_value:定时值;
             ul_mode:无;ul_type:单位;(0:表示ms；1:表示us;)
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016 年4 月26 日
    作    者   : l00347062
    修改内容   : 新生成函数

*****************************************************************************/
#if (_PRE_CONFIG_TARGET_PRODUCT == _PRE_TARGET_PRODUCT_TYPE_E5)
OAL_STATIC OAL_INLINE oal_int32 oal_mdrv_timer_start(oal_uint32 ul_id, oal_dbac_isr_func p_func, oal_int32 l_arg, oal_uint32 ul_timer_value, oal_uint32 ul_mode, oal_uint32 ul_type)
{
    return mdrv_timer_start(ul_id, p_func, l_arg, ul_timer_value, ul_mode, ul_type);
}

OAL_STATIC OAL_INLINE oal_int32 oal_mdrv_timer_stop(oal_uint32 ul_id)
{
    return mdrv_timer_stop(ul_id);
}
#endif

/* 创建一个新的被使用资源区 */
#define oal_request_mem_region(start, n, name)  request_mem_region(start, n, name)
#define oal_release_mem_region(start, n)        release_mem_region(start, n)

#define oal_ioremap(cookie, size)               ioremap(cookie, size)
#define oal_ioremap_nocache(_cookie, _size)     ioremap_nocache(_cookie, _size)
#define oal_iounmap(cookie)                     iounmap(cookie)

#define oal_writel(_ul_val, _ul_addr)           writel(_ul_val, _ul_addr)

#define oal_readl(_ul_addr)                     readl(_ul_addr)

OAL_STATIC  OAL_INLINE oal_hi_timerx_reg_stru  *hi_timerx_request(oal_hi_timerx_index_enum_uint8  en_timer_idx)
{
    oal_hi_timerx_reg_stru      *pst_timer  = NULL;

    if (en_timer_idx < HI5115_TIMER_INDEX_BUTT)
    {
        pst_timer = (oal_hi_timerx_reg_stru *)oal_ioremap(OAL_HI_TIMER_REG_BASE + sizeof(oal_hi_timerx_reg_stru) * en_timer_idx,
                                                            sizeof(oal_hi_timerx_reg_stru));
    }

    return pst_timer;
}

OAL_STATIC  OAL_INLINE  oal_void    hi_timerx_load(oal_hi_timerx_reg_stru  *pst_timer, oal_uint32 ul_load)
{
    if(NULL != pst_timer)
    {
        pst_timer->ul_timerx_load = ul_load;
    }
}

OAL_STATIC  OAL_INLINE  oal_void    hi_timerx_start(oal_hi_timerx_reg_stru *pst_timer)
{
    if(NULL != pst_timer)
    {
        ((oal_hi_timer_control_union *)&pst_timer->ul_timerx_control)->bits_stru.ul_timeren = OAL_TRUE;
    }
}

OAL_STATIC  OAL_INLINE  oal_void    hi_timerx_stop(oal_hi_timerx_reg_stru *pst_timer)
{
    if(NULL != pst_timer)
    {
        ((oal_hi_timer_control_union *)&pst_timer->ul_timerx_control)->bits_stru.ul_timeren = OAL_FALSE;
    }
}

OAL_STATIC  OAL_INLINE  oal_void    hi_timerx_release(oal_hi_timerx_reg_stru  *pst_timer)
{
    if(NULL != pst_timer)
    {
        hi_timerx_stop(pst_timer);
        oal_iounmap(pst_timer);
    }
}

OAL_STATIC  OAL_INLINE  oal_uint32    hi_timerx_read(oal_hi_timerx_reg_stru *pst_timer)
{
    if(NULL != pst_timer)
    {
        return  pst_timer->ul_timerx_value;
    }

    return  0xDEAD; // TODO
}

OAL_STATIC  OAL_INLINE  oal_void    hi_timerx_enable_intr(oal_hi_timerx_reg_stru *pst_timer)
{
    if(NULL != pst_timer)
    {
        ((oal_hi_timer_control_union *)&pst_timer->ul_timerx_control)->bits_stru.ul_intenable = OAL_TRUE;
    }
}

OAL_STATIC  OAL_INLINE  oal_void    hi_timerx_disable_intr(oal_hi_timerx_reg_stru *pst_timer)
{
    if(NULL != pst_timer)
    {
        ((oal_hi_timer_control_union *)&pst_timer->ul_timerx_control)->bits_stru.ul_intenable = OAL_FALSE;
    }
}

OAL_STATIC  OAL_INLINE  oal_void    hi_timerx_clear_intr(oal_hi_timerx_reg_stru *pst_timer)
{
    if(NULL != pst_timer)
    {
        pst_timer->ul_timerx_intclr = 0x0001; // write any value will clear intr
    }
}
OAL_STATIC  OAL_INLINE  oal_bool_enum_uint8  hi_timerx_intr_hit(oal_hi_timerx_reg_stru *pst_timer)
{
    if(NULL != pst_timer)
    {
        return pst_timer->ul_timerx_ris & 0x0001 ? OAL_TRUE : OAL_FALSE;
    }

    return OAL_FALSE;
}
OAL_STATIC  OAL_INLINE oal_void    hi_timerx_control(oal_hi_timerx_reg_stru        *pst_timer,
                                                     oal_hi_timerx_config_stru     *pst_config)
{
    if ((NULL != pst_timer) && (NULL != pst_config))
    {
        pst_timer->ul_timerx_control = pst_config->u_timerx_config.ul_value;
    }
}
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of oal_hardware.h */
