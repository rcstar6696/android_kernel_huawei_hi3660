#ifndef __SOC_WDTV100_INTERFACE_H__
#define __SOC_WDTV100_INTERFACE_H__ 
#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif
#define SOC_WDTV100_WDLOAD_ADDR(base) ((base) + (0x0000))
#define SOC_WDTV100_WDVALUE_ADDR(base) ((base) + (0x0004))
#define SOC_WDTV100_WDCONTROL_ADDR(base) ((base) + (0x0008))
#define SOC_WDTV100_WDINTCLR_ADDR(base) ((base) + (0x000C))
#define SOC_WDTV100_WDRIS_ADDR(base) ((base) + (0x0010))
#define SOC_WDTV100_WDMIS_ADDR(base) ((base) + (0x0014))
#define SOC_WDTV100_WDLOCK_ADDR(base) ((base) + (0x0C00))
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int wdogload : 32;
    } reg;
} SOC_WDTV100_WDLOAD_UNION;
#endif
#define SOC_WDTV100_WDLOAD_wdogload_START (0)
#define SOC_WDTV100_WDLOAD_wdogload_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int wdogvalue : 32;
    } reg;
} SOC_WDTV100_WDVALUE_UNION;
#endif
#define SOC_WDTV100_WDVALUE_wdogvalue_START (0)
#define SOC_WDTV100_WDVALUE_wdogvalue_END (31)
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
} SOC_WDTV100_WDCONTROL_UNION;
#endif
#define SOC_WDTV100_WDCONTROL_inten_START (0)
#define SOC_WDTV100_WDCONTROL_inten_END (0)
#define SOC_WDTV100_WDCONTROL_resen_START (1)
#define SOC_WDTV100_WDCONTROL_resen_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int wdogxintclr : 32;
    } reg;
} SOC_WDTV100_WDINTCLR_UNION;
#endif
#define SOC_WDTV100_WDINTCLR_wdogxintclr_START (0)
#define SOC_WDTV100_WDINTCLR_wdogxintclr_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int wdogris : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_WDTV100_WDRIS_UNION;
#endif
#define SOC_WDTV100_WDRIS_wdogris_START (0)
#define SOC_WDTV100_WDRIS_wdogris_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int wdogmis : 1;
        unsigned int reserved : 31;
    } reg;
} SOC_WDTV100_WDMIS_UNION;
#endif
#define SOC_WDTV100_WDMIS_wdogmis_START (0)
#define SOC_WDTV100_WDMIS_wdogmis_END (0)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int wdoglock : 32;
    } reg;
} SOC_WDTV100_WDLOCK_UNION;
#endif
#define SOC_WDTV100_WDLOCK_wdoglock_START (0)
#define SOC_WDTV100_WDLOCK_wdoglock_END (31)
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
#endif
