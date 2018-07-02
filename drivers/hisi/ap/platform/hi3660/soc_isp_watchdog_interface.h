#ifndef __SOC_ISP_WATCHDOG_INTERFACE_H__
#define __SOC_ISP_WATCHDOG_INTERFACE_H__ 
#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif
#define SOC_ISP_WatchDog_WDG_LOAD_ADDR(base) ((base) + (0x0000))
#define SOC_ISP_WatchDog_WDG_VALUE_ADDR(base) ((base) + (0x0004))
#define SOC_ISP_WatchDog_WDG_CONTROL_ADDR(base) ((base) + (0x0008))
#define SOC_ISP_WatchDog_WDG_INTCLR_ADDR(base) ((base) + (0x000C))
#define SOC_ISP_WatchDog_WDG_RIS_ADDR(base) ((base) + (0x0010))
#define SOC_ISP_WatchDog_WDG_MIS_ADDR(base) ((base) + (0x0014))
#define SOC_ISP_WatchDog_WDG_LOCK_ADDR(base) ((base) + (0x0C00))
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int wdg_load : 32;
    } reg;
} SOC_ISP_WatchDog_WDG_LOAD_UNION;
#endif
#define SOC_ISP_WatchDog_WDG_LOAD_wdg_load_START (0)
#define SOC_ISP_WatchDog_WDG_LOAD_wdg_load_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int wdogvalue : 32;
    } reg;
} SOC_ISP_WatchDog_WDG_VALUE_UNION;
#endif
#define SOC_ISP_WatchDog_WDG_VALUE_wdogvalue_START (0)
#define SOC_ISP_WatchDog_WDG_VALUE_wdogvalue_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int inten : 1;
        unsigned int resen : 1;
        unsigned int reserved : 30;
    } reg;
} SOC_ISP_WatchDog_WDG_CONTROL_UNION;
#endif
#define SOC_ISP_WatchDog_WDG_CONTROL_inten_START (0)
#define SOC_ISP_WatchDog_WDG_CONTROL_inten_END (0)
#define SOC_ISP_WatchDog_WDG_CONTROL_resen_START (1)
#define SOC_ISP_WatchDog_WDG_CONTROL_resen_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int wdg_intclr : 32;
    } reg;
} SOC_ISP_WatchDog_WDG_INTCLR_UNION;
#endif
#define SOC_ISP_WatchDog_WDG_INTCLR_wdg_intclr_START (0)
#define SOC_ISP_WatchDog_WDG_INTCLR_wdg_intclr_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int wdogris : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_ISP_WatchDog_WDG_RIS_UNION;
#endif
#define SOC_ISP_WatchDog_WDG_RIS_wdogris_START (0)
#define SOC_ISP_WatchDog_WDG_RIS_wdogris_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int wdogmis : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_ISP_WatchDog_WDG_MIS_UNION;
#endif
#define SOC_ISP_WatchDog_WDG_MIS_wdogmis_START (0)
#define SOC_ISP_WatchDog_WDG_MIS_wdogmis_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int wdg_lock : 32;
    } reg;
} SOC_ISP_WatchDog_WDG_LOCK_UNION;
#endif
#define SOC_ISP_WatchDog_WDG_LOCK_wdg_lock_START (0)
#define SOC_ISP_WatchDog_WDG_LOCK_wdg_lock_END (31)
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
#endif
