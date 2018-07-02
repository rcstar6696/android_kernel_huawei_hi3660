#ifndef __SOC_GIC_NS_INTERFACE_H__
#define __SOC_GIC_NS_INTERFACE_H__ 
#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif
#define SOC_GIC_NS_GICD_CTLR_NS_ADDR(base) ((base) + (0x1000))
#define SOC_GIC_NS_GICD_TYPER_ADDR(base) ((base) + (0x1004))
#define SOC_GIC_NS_GICD_IIDR_ADDR(base) ((base) + (0x1008))
#define SOC_GIC_NS_GICD_ISENABLER0_ADDR(base) ((base) + (0x1100))
#define SOC_GIC_NS_GICD_ISENABLERN_ADDR(base,N) ((base) + (0x1104+(4*(N))))
#define SOC_GIC_NS_GICD_ICENABLER0_ADDR(base) ((base) + (0x1180))
#define SOC_GIC_NS_GICD_ICENABLERn_ADDR(base,N) ((base) + (0x1184+(4*(N))))
#define SOC_GIC_NS_GICD_ISPENDRN_ADDR(base,N) ((base) + (0x1200+(4*(N))))
#define SOC_GIC_NS_GICD_ICPENDRn_ADDR(base,N) ((base) + (0x1280+(4*(N))))
#define SOC_GIC_NS_GICD_ISACTIVERn_ADDR(base,N) ((base) + (0x1300+(4*(N))))
#define SOC_GIC_NS_GICD_ICACTIVERn_ADDR(base,N) ((base) + (0x1380+(4*(N))))
#define SOC_GIC_NS_GICD_IPRIORITYRn_ADDR(base,N) ((base) + (0x1400+(4*(N))))
#define SOC_GIC_NS_GICD_ITARGETSR0_7_ADDR(base,N) ((base) + (0x1800+(4*(N))))
#define SOC_GIC_NS_GICD_ITARGETSRn_ADDR(base,N) ((base) + (0x1820+(4*(N))))
#define SOC_GIC_NS_GICD_ICFGR0_ADDR(base) ((base) + (0x1C00))
#define SOC_GIC_NS_GICD_ICFGR1_ADDR(base) ((base) + (0x1C04))
#define SOC_GIC_NS_GICD_ICFGRn_ADDR(base,N) ((base) + (0x1C08+(4*(N))))
#define SOC_GIC_NS_GICD_PPISR_ADDR(base) ((base) + (0x1D00))
#define SOC_GIC_NS_GICD_SPISRn_ADDR(base,N) ((base) + (0x1D04+(4*(N))))
#define SOC_GIC_NS_GICD_SGIR_ADDR(base) ((base) + (0x1F00))
#define SOC_GIC_NS_GICD_CPENDSGIRn_ADDR(base,N) ((base) + (0x1F10+(4*(N))))
#define SOC_GIC_NS_GICD_SPENDSGIRn_ADDR(base,N) ((base) + (0x1F20+(4*(N))))
#define SOC_GIC_NS_GICD_PIDR4_ADDR(base) ((base) + (0x1FD0))
#define SOC_GIC_NS_GICD_PIDR5_ADDR(base) ((base) + (0x1FD4))
#define SOC_GIC_NS_GICD_PIDR6_ADDR(base) ((base) + (0x1FD8))
#define SOC_GIC_NS_GICD_PIDR7_ADDR(base) ((base) + (0x1FDC))
#define SOC_GIC_NS_GICD_PIDR0_ADDR(base) ((base) + (0x1FE0))
#define SOC_GIC_NS_GICD_PIDR1_ADDR(base) ((base) + (0x1FE4))
#define SOC_GIC_NS_GICD_PIDR2_ADDR(base) ((base) + (0x1FE8))
#define SOC_GIC_NS_GICD_PIDR3_ADDR(base) ((base) + (0x1FEC))
#define SOC_GIC_NS_GICD_CIDR0_ADDR(base) ((base) + (0x1FF0))
#define SOC_GIC_NS_GICD_CIDR1_ADDR(base) ((base) + (0x1FF4))
#define SOC_GIC_NS_GICD_CIDR2_ADDR(base) ((base) + (0x1FF8))
#define SOC_GIC_NS_GICD_CIDR3_ADDR(base) ((base) + (0x1FFC))
#define SOC_GIC_NS_GICC_CTLR_NS_ADDR(base) ((base) + (0x2000))
#define SOC_GIC_NS_GICC_PMR_ADDR(base) ((base) + (0x2004))
#define SOC_GIC_NS_GICC_BPR_NS_ADDR(base) ((base) + (0x2008))
#define SOC_GIC_NS_GICC_IAR_ADDR(base) ((base) + (0x200C))
#define SOC_GIC_NS_GICC_EOIR_ADDR(base) ((base) + (0x2010))
#define SOC_GIC_NS_GICC_RPR_ADDR(base) ((base) + (0x2014))
#define SOC_GIC_NS_GICC_HPPIR_ADDR(base) ((base) + (0x2018))
#define SOC_GIC_NS_GICC_IIDR_ADDR(base) ((base) + (0x20FC))
#define SOC_GIC_NS_GICC_DIR_ADDR(base) ((base) + (0x3000))
#define SOC_GIC_NS_GICH_HCR_ADDR(base) ((base) + (0x4000))
#define SOC_GIC_NS_GICH_VTR_ADDR(base) ((base) + (0x4004))
#define SOC_GIC_NS_GICH_VMCR_ADDR(base) ((base) + (0x4008))
#define SOC_GIC_NS_GICH_MISR_ADDR(base) ((base) + (0x4010))
#define SOC_GIC_NS_GICH_EISR0_ADDR(base) ((base) + (0x4020))
#define SOC_GIC_NS_GICH_ELSR0_ADDR(base) ((base) + (0x4030))
#define SOC_GIC_NS_GICH_APR0_ADDR(base) ((base) + (0x40F0))
#define SOC_GIC_NS_GICH_LR0_ADDR(base) ((base) + (0x4100))
#define SOC_GIC_NS_GICH_LR1_ADDR(base) ((base) + (0x4104))
#define SOC_GIC_NS_GICH_LR2_ADDR(base) ((base) + (0x4108))
#define SOC_GIC_NS_GICH_LR3_ADDR(base) ((base) + (0x410C))
#define SOC_GIC_NS_GICV_CTLR_ADDR(base) ((base) + (0x6000))
#define SOC_GIC_NS_GICV_PMR_ADDR(base) ((base) + (0x6004))
#define SOC_GIC_NS_GICV_BPR_ADDR(base) ((base) + (0x6008))
#define SOC_GIC_NS_GICV_IAR_ADDR(base) ((base) + (0x600C))
#define SOC_GIC_NS_GICV_EOIR_ADDR(base) ((base) + (0x6010))
#define SOC_GIC_NS_GICV_RPR_ADDR(base) ((base) + (0x6014))
#define SOC_GIC_NS_GICV_HPPIR_ADDR(base) ((base) + (0x6018))
#define SOC_GIC_NS_GICV_ABPR_ADDR(base) ((base) + (0x601C))
#define SOC_GIC_NS_GICV_AIAR_ADDR(base) ((base) + (0x6020))
#define SOC_GIC_NS_GICV_AEOIR_ADDR(base) ((base) + (0x6024))
#define SOC_GIC_NS_GICV_AHPPIR_ADDR(base) ((base) + (0x6028))
#define SOC_GIC_NS_GICV_APR0_ADDR(base) ((base) + (0x60D0))
#define SOC_GIC_NS_GICV_IIDR_ADDR(base) ((base) + (0x60FC))
#define SOC_GIC_NS_GICV_DIR_ADDR(base) ((base) + (0x7000))
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int EnableGrp0 : 1;
        unsigned int EnableGrp1 : 1;
        unsigned int reserved : 30;
    } reg;
} SOC_GIC_NS_GICD_CTLR_NS_UNION;
#endif
#define SOC_GIC_NS_GICD_CTLR_NS_EnableGrp0_START (0)
#define SOC_GIC_NS_GICD_CTLR_NS_EnableGrp0_END (0)
#define SOC_GIC_NS_GICD_CTLR_NS_EnableGrp1_START (1)
#define SOC_GIC_NS_GICD_CTLR_NS_EnableGrp1_END (1)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int ITLinesNumber : 5;
        unsigned int CPUNumber : 3;
        unsigned int reserved_0 : 2;
        unsigned int SecurityExtn : 1;
        unsigned int LSPI : 5;
        unsigned int reserved_1 : 16;
    } reg;
} SOC_GIC_NS_GICD_TYPER_UNION;
#endif
#define SOC_GIC_NS_GICD_TYPER_ITLinesNumber_START (0)
#define SOC_GIC_NS_GICD_TYPER_ITLinesNumber_END (4)
#define SOC_GIC_NS_GICD_TYPER_CPUNumber_START (5)
#define SOC_GIC_NS_GICD_TYPER_CPUNumber_END (7)
#define SOC_GIC_NS_GICD_TYPER_SecurityExtn_START (10)
#define SOC_GIC_NS_GICD_TYPER_SecurityExtn_END (10)
#define SOC_GIC_NS_GICD_TYPER_LSPI_START (11)
#define SOC_GIC_NS_GICD_TYPER_LSPI_END (15)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int Implementer : 12;
        unsigned int Revision : 4;
        unsigned int Variant : 4;
        unsigned int reserved : 4;
        unsigned int ProductID : 8;
    } reg;
} SOC_GIC_NS_GICD_IIDR_UNION;
#endif
#define SOC_GIC_NS_GICD_IIDR_Implementer_START (0)
#define SOC_GIC_NS_GICD_IIDR_Implementer_END (11)
#define SOC_GIC_NS_GICD_IIDR_Revision_START (12)
#define SOC_GIC_NS_GICD_IIDR_Revision_END (15)
#define SOC_GIC_NS_GICD_IIDR_Variant_START (16)
#define SOC_GIC_NS_GICD_IIDR_Variant_END (19)
#define SOC_GIC_NS_GICD_IIDR_ProductID_START (24)
#define SOC_GIC_NS_GICD_IIDR_ProductID_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int setenable : 32;
    } reg;
} SOC_GIC_NS_GICD_ISENABLER0_UNION;
#endif
#define SOC_GIC_NS_GICD_ISENABLER0_setenable_START (0)
#define SOC_GIC_NS_GICD_ISENABLER0_setenable_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int setenable : 32;
    } reg;
} SOC_GIC_NS_GICD_ISENABLERN_UNION;
#endif
#define SOC_GIC_NS_GICD_ISENABLERN_setenable_START (0)
#define SOC_GIC_NS_GICD_ISENABLERN_setenable_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int ClearEnable : 32;
    } reg;
} SOC_GIC_NS_GICD_ICENABLER0_UNION;
#endif
#define SOC_GIC_NS_GICD_ICENABLER0_ClearEnable_START (0)
#define SOC_GIC_NS_GICD_ICENABLER0_ClearEnable_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int ClearEnable : 32;
    } reg;
} SOC_GIC_NS_GICD_ICENABLERn_UNION;
#endif
#define SOC_GIC_NS_GICD_ICENABLERn_ClearEnable_START (0)
#define SOC_GIC_NS_GICD_ICENABLERn_ClearEnable_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int SetPending : 32;
    } reg;
} SOC_GIC_NS_GICD_ISPENDRN_UNION;
#endif
#define SOC_GIC_NS_GICD_ISPENDRN_SetPending_START (0)
#define SOC_GIC_NS_GICD_ISPENDRN_SetPending_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int ClearPending : 32;
    } reg;
} SOC_GIC_NS_GICD_ICPENDRn_UNION;
#endif
#define SOC_GIC_NS_GICD_ICPENDRn_ClearPending_START (0)
#define SOC_GIC_NS_GICD_ICPENDRn_ClearPending_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int SetActive : 32;
    } reg;
} SOC_GIC_NS_GICD_ISACTIVERn_UNION;
#endif
#define SOC_GIC_NS_GICD_ISACTIVERn_SetActive_START (0)
#define SOC_GIC_NS_GICD_ISACTIVERn_SetActive_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int ClearActive : 32;
    } reg;
} SOC_GIC_NS_GICD_ICACTIVERn_UNION;
#endif
#define SOC_GIC_NS_GICD_ICACTIVERn_ClearActive_START (0)
#define SOC_GIC_NS_GICD_ICACTIVERn_ClearActive_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved_0 : 4;
        unsigned int Priority_byte_offset0 : 4;
        unsigned int reserved_1 : 4;
        unsigned int Priority_byte_offset1 : 4;
        unsigned int reserved_2 : 4;
        unsigned int Priority_byte_offset2 : 4;
        unsigned int reserved_3 : 4;
        unsigned int Priority_byte_offset3 : 4;
    } reg;
} SOC_GIC_NS_GICD_IPRIORITYRn_UNION;
#endif
#define SOC_GIC_NS_GICD_IPRIORITYRn_Priority_byte_offset0_START (4)
#define SOC_GIC_NS_GICD_IPRIORITYRn_Priority_byte_offset0_END (7)
#define SOC_GIC_NS_GICD_IPRIORITYRn_Priority_byte_offset1_START (12)
#define SOC_GIC_NS_GICD_IPRIORITYRn_Priority_byte_offset1_END (15)
#define SOC_GIC_NS_GICD_IPRIORITYRn_Priority_byte_offset2_START (20)
#define SOC_GIC_NS_GICD_IPRIORITYRn_Priority_byte_offset2_END (23)
#define SOC_GIC_NS_GICD_IPRIORITYRn_Priority_byte_offset3_START (28)
#define SOC_GIC_NS_GICD_IPRIORITYRn_Priority_byte_offset3_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int CPUtargets_byte_offset0 : 8;
        unsigned int CPUtargets_byte_offset1 : 8;
        unsigned int CPUtargets_byte_offset2 : 8;
        unsigned int CPUtargets_byte_offset3 : 8;
    } reg;
} SOC_GIC_NS_GICD_ITARGETSR0_7_UNION;
#endif
#define SOC_GIC_NS_GICD_ITARGETSR0_7_CPUtargets_byte_offset0_START (0)
#define SOC_GIC_NS_GICD_ITARGETSR0_7_CPUtargets_byte_offset0_END (7)
#define SOC_GIC_NS_GICD_ITARGETSR0_7_CPUtargets_byte_offset1_START (8)
#define SOC_GIC_NS_GICD_ITARGETSR0_7_CPUtargets_byte_offset1_END (15)
#define SOC_GIC_NS_GICD_ITARGETSR0_7_CPUtargets_byte_offset2_START (16)
#define SOC_GIC_NS_GICD_ITARGETSR0_7_CPUtargets_byte_offset2_END (23)
#define SOC_GIC_NS_GICD_ITARGETSR0_7_CPUtargets_byte_offset3_START (24)
#define SOC_GIC_NS_GICD_ITARGETSR0_7_CPUtargets_byte_offset3_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int CPUtargets_byte_offset0 : 8;
        unsigned int CPUtargets_byte_offset1 : 8;
        unsigned int CPUtargets_byte_offset2 : 8;
        unsigned int CPUtargets_byte_offset3 : 8;
    } reg;
} SOC_GIC_NS_GICD_ITARGETSRn_UNION;
#endif
#define SOC_GIC_NS_GICD_ITARGETSRn_CPUtargets_byte_offset0_START (0)
#define SOC_GIC_NS_GICD_ITARGETSRn_CPUtargets_byte_offset0_END (7)
#define SOC_GIC_NS_GICD_ITARGETSRn_CPUtargets_byte_offset1_START (8)
#define SOC_GIC_NS_GICD_ITARGETSRn_CPUtargets_byte_offset1_END (15)
#define SOC_GIC_NS_GICD_ITARGETSRn_CPUtargets_byte_offset2_START (16)
#define SOC_GIC_NS_GICD_ITARGETSRn_CPUtargets_byte_offset2_END (23)
#define SOC_GIC_NS_GICD_ITARGETSRn_CPUtargets_byte_offset3_START (24)
#define SOC_GIC_NS_GICD_ITARGETSRn_CPUtargets_byte_offset3_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int SGIConfigF0 : 2;
        unsigned int SGIConfigF1 : 2;
        unsigned int SGIConfigF2 : 2;
        unsigned int SGIConfigF3 : 2;
        unsigned int SGIConfigF4 : 2;
        unsigned int SGIConfigF5 : 2;
        unsigned int SGIConfigF6 : 2;
        unsigned int SGIConfigF7 : 2;
        unsigned int SGIConfigF8 : 2;
        unsigned int SGIConfigF9 : 2;
        unsigned int SGIConfigF10 : 2;
        unsigned int SGIConfigF11 : 2;
        unsigned int SGIConfigF12 : 2;
        unsigned int SGIConfigF13 : 2;
        unsigned int SGIConfigF14 : 2;
        unsigned int SGIConfigF15 : 2;
    } reg;
} SOC_GIC_NS_GICD_ICFGR0_UNION;
#endif
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF0_START (0)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF0_END (1)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF1_START (2)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF1_END (3)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF2_START (4)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF2_END (5)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF3_START (6)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF3_END (7)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF4_START (8)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF4_END (9)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF5_START (10)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF5_END (11)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF6_START (12)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF6_END (13)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF7_START (14)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF7_END (15)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF8_START (16)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF8_END (17)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF9_START (18)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF9_END (19)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF10_START (20)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF10_END (21)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF11_START (22)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF11_END (23)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF12_START (24)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF12_END (25)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF13_START (26)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF13_END (27)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF14_START (28)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF14_END (29)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF15_START (30)
#define SOC_GIC_NS_GICD_ICFGR0_SGIConfigF15_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int PPIConfig0 : 2;
        unsigned int PPIConfig1 : 2;
        unsigned int PPIConfig2 : 2;
        unsigned int PPIConfig3 : 2;
        unsigned int PPIConfig4 : 2;
        unsigned int PPIConfig5 : 2;
        unsigned int PPIConfig6 : 2;
        unsigned int PPIConfig7 : 2;
        unsigned int PPIConfig8 : 2;
        unsigned int PPIConfig9 : 2;
        unsigned int PPIConfig10 : 2;
        unsigned int PPIConfig11 : 2;
        unsigned int PPIConfig12 : 2;
        unsigned int PPIConfig13 : 2;
        unsigned int PPIConfig14 : 2;
        unsigned int PPIConfig15 : 2;
    } reg;
} SOC_GIC_NS_GICD_ICFGR1_UNION;
#endif
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig0_START (0)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig0_END (1)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig1_START (2)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig1_END (3)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig2_START (4)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig2_END (5)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig3_START (6)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig3_END (7)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig4_START (8)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig4_END (9)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig5_START (10)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig5_END (11)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig6_START (12)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig6_END (13)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig7_START (14)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig7_END (15)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig8_START (16)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig8_END (17)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig9_START (18)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig9_END (19)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig10_START (20)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig10_END (21)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig11_START (22)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig11_END (23)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig12_START (24)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig12_END (25)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig13_START (26)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig13_END (27)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig14_START (28)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig14_END (29)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig15_START (30)
#define SOC_GIC_NS_GICD_ICFGR1_PPIConfig15_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int PPIConfig0 : 2;
        unsigned int PPIConfig1 : 2;
        unsigned int PPIConfig2 : 2;
        unsigned int PPIConfig3 : 2;
        unsigned int PPIConfig4 : 2;
        unsigned int PPIConfig5 : 2;
        unsigned int PPIConfig6 : 2;
        unsigned int PPIConfig7 : 2;
        unsigned int PPIConfig8 : 2;
        unsigned int PPIConfig9 : 2;
        unsigned int PPIConfig10 : 2;
        unsigned int PPIConfig11 : 2;
        unsigned int PPIConfig12 : 2;
        unsigned int PPIConfig13 : 2;
        unsigned int PPIConfig14 : 2;
        unsigned int PPIConfig15 : 2;
    } reg;
} SOC_GIC_NS_GICD_ICFGRn_UNION;
#endif
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig0_START (0)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig0_END (1)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig1_START (2)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig1_END (3)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig2_START (4)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig2_END (5)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig3_START (6)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig3_END (7)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig4_START (8)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig4_END (9)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig5_START (10)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig5_END (11)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig6_START (12)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig6_END (13)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig7_START (14)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig7_END (15)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig8_START (16)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig8_END (17)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig9_START (18)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig9_END (19)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig10_START (20)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig10_END (21)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig11_START (22)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig11_END (23)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig12_START (24)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig12_END (25)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig13_START (26)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig13_END (27)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig14_START (28)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig14_END (29)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig15_START (30)
#define SOC_GIC_NS_GICD_ICFGRn_PPIConfig15_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved_0: 9;
        unsigned int PPIStatus : 7;
        unsigned int reserved_1: 16;
    } reg;
} SOC_GIC_NS_GICD_PPISR_UNION;
#endif
#define SOC_GIC_NS_GICD_PPISR_PPIStatus_START (9)
#define SOC_GIC_NS_GICD_PPISR_PPIStatus_END (15)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int IRQS : 32;
    } reg;
} SOC_GIC_NS_GICD_SPISRn_UNION;
#endif
#define SOC_GIC_NS_GICD_SPISRn_IRQS_START (0)
#define SOC_GIC_NS_GICD_SPISRn_IRQS_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int SGIINTID : 4;
        unsigned int reserved_0 : 11;
        unsigned int NSATT : 1;
        unsigned int CPUTargetList : 8;
        unsigned int TargetListFilter : 2;
        unsigned int reserved_1 : 6;
    } reg;
} SOC_GIC_NS_GICD_SGIR_UNION;
#endif
#define SOC_GIC_NS_GICD_SGIR_SGIINTID_START (0)
#define SOC_GIC_NS_GICD_SGIR_SGIINTID_END (3)
#define SOC_GIC_NS_GICD_SGIR_NSATT_START (15)
#define SOC_GIC_NS_GICD_SGIR_NSATT_END (15)
#define SOC_GIC_NS_GICD_SGIR_CPUTargetList_START (16)
#define SOC_GIC_NS_GICD_SGIR_CPUTargetList_END (23)
#define SOC_GIC_NS_GICD_SGIR_TargetListFilter_START (24)
#define SOC_GIC_NS_GICD_SGIR_TargetListFilter_END (25)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int SGI_clear_pending_bits : 32;
    } reg;
} SOC_GIC_NS_GICD_CPENDSGIRn_UNION;
#endif
#define SOC_GIC_NS_GICD_CPENDSGIRn_SGI_clear_pending_bits_START (0)
#define SOC_GIC_NS_GICD_CPENDSGIRn_SGI_clear_pending_bits_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int SGI_set_pending_bits : 32;
    } reg;
} SOC_GIC_NS_GICD_SPENDSGIRn_UNION;
#endif
#define SOC_GIC_NS_GICD_SPENDSGIRn_SGI_set_pending_bits_START (0)
#define SOC_GIC_NS_GICD_SPENDSGIRn_SGI_set_pending_bits_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int continuationcode : 4;
        unsigned int reserved : 4;
        unsigned int implementation_defined : 24;
    } reg;
} SOC_GIC_NS_GICD_PIDR4_UNION;
#endif
#define SOC_GIC_NS_GICD_PIDR4_continuationcode_START (0)
#define SOC_GIC_NS_GICD_PIDR4_continuationcode_END (3)
#define SOC_GIC_NS_GICD_PIDR4_implementation_defined_START (8)
#define SOC_GIC_NS_GICD_PIDR4_implementation_defined_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved : 8;
        unsigned int implementation_defined : 24;
    } reg;
} SOC_GIC_NS_GICD_PIDR5_UNION;
#endif
#define SOC_GIC_NS_GICD_PIDR5_implementation_defined_START (8)
#define SOC_GIC_NS_GICD_PIDR5_implementation_defined_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved : 8;
        unsigned int implementation_defined : 24;
    } reg;
} SOC_GIC_NS_GICD_PIDR6_UNION;
#endif
#define SOC_GIC_NS_GICD_PIDR6_implementation_defined_START (8)
#define SOC_GIC_NS_GICD_PIDR6_implementation_defined_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved : 8;
        unsigned int implementation_defined : 24;
    } reg;
} SOC_GIC_NS_GICD_PIDR7_UNION;
#endif
#define SOC_GIC_NS_GICD_PIDR7_implementation_defined_START (8)
#define SOC_GIC_NS_GICD_PIDR7_implementation_defined_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int devid_7_0 : 8;
        unsigned int implementation_defined : 24;
    } reg;
} SOC_GIC_NS_GICD_PIDR0_UNION;
#endif
#define SOC_GIC_NS_GICD_PIDR0_devid_7_0_START (0)
#define SOC_GIC_NS_GICD_PIDR0_devid_7_0_END (7)
#define SOC_GIC_NS_GICD_PIDR0_implementation_defined_START (8)
#define SOC_GIC_NS_GICD_PIDR0_implementation_defined_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int devid_11_8 : 4;
        unsigned int archid_3_0 : 4;
        unsigned int implementation_defined : 24;
    } reg;
} SOC_GIC_NS_GICD_PIDR1_UNION;
#endif
#define SOC_GIC_NS_GICD_PIDR1_devid_11_8_START (0)
#define SOC_GIC_NS_GICD_PIDR1_devid_11_8_END (3)
#define SOC_GIC_NS_GICD_PIDR1_archid_3_0_START (4)
#define SOC_GIC_NS_GICD_PIDR1_archid_3_0_END (7)
#define SOC_GIC_NS_GICD_PIDR1_implementation_defined_START (8)
#define SOC_GIC_NS_GICD_PIDR1_implementation_defined_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int archid_6_4 : 3;
        unsigned int UsesJEPcode : 1;
        unsigned int ArchRev : 4;
        unsigned int implementation_defined : 24;
    } reg;
} SOC_GIC_NS_GICD_PIDR2_UNION;
#endif
#define SOC_GIC_NS_GICD_PIDR2_archid_6_4_START (0)
#define SOC_GIC_NS_GICD_PIDR2_archid_6_4_END (2)
#define SOC_GIC_NS_GICD_PIDR2_UsesJEPcode_START (3)
#define SOC_GIC_NS_GICD_PIDR2_UsesJEPcode_END (3)
#define SOC_GIC_NS_GICD_PIDR2_ArchRev_START (4)
#define SOC_GIC_NS_GICD_PIDR2_ArchRev_END (7)
#define SOC_GIC_NS_GICD_PIDR2_implementation_defined_START (8)
#define SOC_GIC_NS_GICD_PIDR2_implementation_defined_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved : 4;
        unsigned int Revision : 4;
        unsigned int implementation_defined : 24;
    } reg;
} SOC_GIC_NS_GICD_PIDR3_UNION;
#endif
#define SOC_GIC_NS_GICD_PIDR3_Revision_START (4)
#define SOC_GIC_NS_GICD_PIDR3_Revision_END (7)
#define SOC_GIC_NS_GICD_PIDR3_implementation_defined_START (8)
#define SOC_GIC_NS_GICD_PIDR3_implementation_defined_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int Component : 32;
    } reg;
} SOC_GIC_NS_GICD_CIDR0_UNION;
#endif
#define SOC_GIC_NS_GICD_CIDR0_Component_START (0)
#define SOC_GIC_NS_GICD_CIDR0_Component_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int Component : 32;
    } reg;
} SOC_GIC_NS_GICD_CIDR1_UNION;
#endif
#define SOC_GIC_NS_GICD_CIDR1_Component_START (0)
#define SOC_GIC_NS_GICD_CIDR1_Component_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int Component : 32;
    } reg;
} SOC_GIC_NS_GICD_CIDR2_UNION;
#endif
#define SOC_GIC_NS_GICD_CIDR2_Component_START (0)
#define SOC_GIC_NS_GICD_CIDR2_Component_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int Component : 32;
    } reg;
} SOC_GIC_NS_GICD_CIDR3_UNION;
#endif
#define SOC_GIC_NS_GICD_CIDR3_Component_START (0)
#define SOC_GIC_NS_GICD_CIDR3_Component_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int enablegrp1 : 1;
        unsigned int reserved_0 : 4;
        unsigned int fiqbypdisgrp1 : 1;
        unsigned int irqbypdisgrp1 : 1;
        unsigned int reserved_1 : 2;
        unsigned int eoimodens : 1;
        unsigned int reserved_2 : 22;
    } reg;
} SOC_GIC_NS_GICC_CTLR_NS_UNION;
#endif
#define SOC_GIC_NS_GICC_CTLR_NS_enablegrp1_START (0)
#define SOC_GIC_NS_GICC_CTLR_NS_enablegrp1_END (0)
#define SOC_GIC_NS_GICC_CTLR_NS_fiqbypdisgrp1_START (5)
#define SOC_GIC_NS_GICC_CTLR_NS_fiqbypdisgrp1_END (5)
#define SOC_GIC_NS_GICC_CTLR_NS_irqbypdisgrp1_START (6)
#define SOC_GIC_NS_GICC_CTLR_NS_irqbypdisgrp1_END (6)
#define SOC_GIC_NS_GICC_CTLR_NS_eoimodens_START (9)
#define SOC_GIC_NS_GICC_CTLR_NS_eoimodens_END (9)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int priority_low : 3;
        unsigned int priority : 5;
        unsigned int reserved : 24;
    } reg;
} SOC_GIC_NS_GICC_PMR_UNION;
#endif
#define SOC_GIC_NS_GICC_PMR_priority_low_START (0)
#define SOC_GIC_NS_GICC_PMR_priority_low_END (2)
#define SOC_GIC_NS_GICC_PMR_priority_START (3)
#define SOC_GIC_NS_GICC_PMR_priority_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int binarypoint : 3;
        unsigned int reserved : 29;
    } reg;
} SOC_GIC_NS_GICC_BPR_NS_UNION;
#endif
#define SOC_GIC_NS_GICC_BPR_NS_binarypoint_START (0)
#define SOC_GIC_NS_GICC_BPR_NS_binarypoint_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int InterruptID : 10;
        unsigned int cpuid : 3;
        unsigned int reserved : 19;
    } reg;
} SOC_GIC_NS_GICC_IAR_UNION;
#endif
#define SOC_GIC_NS_GICC_IAR_InterruptID_START (0)
#define SOC_GIC_NS_GICC_IAR_InterruptID_END (9)
#define SOC_GIC_NS_GICC_IAR_cpuid_START (10)
#define SOC_GIC_NS_GICC_IAR_cpuid_END (12)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int EOIINTID : 10;
        unsigned int CPUID : 3;
        unsigned int reserved : 19;
    } reg;
} SOC_GIC_NS_GICC_EOIR_UNION;
#endif
#define SOC_GIC_NS_GICC_EOIR_EOIINTID_START (0)
#define SOC_GIC_NS_GICC_EOIR_EOIINTID_END (9)
#define SOC_GIC_NS_GICC_EOIR_CPUID_START (10)
#define SOC_GIC_NS_GICC_EOIR_CPUID_END (12)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int Priority : 8;
        unsigned int reserved : 24;
    } reg;
} SOC_GIC_NS_GICC_RPR_UNION;
#endif
#define SOC_GIC_NS_GICC_RPR_Priority_START (0)
#define SOC_GIC_NS_GICC_RPR_Priority_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int PENDINTID : 10;
        unsigned int CPUID : 3;
        unsigned int reserved : 19;
    } reg;
} SOC_GIC_NS_GICC_HPPIR_UNION;
#endif
#define SOC_GIC_NS_GICC_HPPIR_PENDINTID_START (0)
#define SOC_GIC_NS_GICC_HPPIR_PENDINTID_END (9)
#define SOC_GIC_NS_GICC_HPPIR_CPUID_START (10)
#define SOC_GIC_NS_GICC_HPPIR_CPUID_END (12)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int implementer : 12;
        unsigned int revision : 4;
        unsigned int archversion : 4;
        unsigned int productid : 12;
    } reg;
} SOC_GIC_NS_GICC_IIDR_UNION;
#endif
#define SOC_GIC_NS_GICC_IIDR_implementer_START (0)
#define SOC_GIC_NS_GICC_IIDR_implementer_END (11)
#define SOC_GIC_NS_GICC_IIDR_revision_START (12)
#define SOC_GIC_NS_GICC_IIDR_revision_END (15)
#define SOC_GIC_NS_GICC_IIDR_archversion_START (16)
#define SOC_GIC_NS_GICC_IIDR_archversion_END (19)
#define SOC_GIC_NS_GICC_IIDR_productid_START (20)
#define SOC_GIC_NS_GICC_IIDR_productid_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int InterruptID : 10;
        unsigned int CPUID : 3;
        unsigned int reserved : 19;
    } reg;
} SOC_GIC_NS_GICC_DIR_UNION;
#endif
#define SOC_GIC_NS_GICC_DIR_InterruptID_START (0)
#define SOC_GIC_NS_GICC_DIR_InterruptID_END (9)
#define SOC_GIC_NS_GICC_DIR_CPUID_START (10)
#define SOC_GIC_NS_GICC_DIR_CPUID_END (12)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int En : 1;
        unsigned int UIE : 1;
        unsigned int LRENPIE : 1;
        unsigned int NPIE : 1;
        unsigned int VGrp0EIE : 1;
        unsigned int VGrp0DIE : 1;
        unsigned int VGrp1EIE : 1;
        unsigned int VGrp1DIE : 1;
        unsigned int reserved : 19;
        unsigned int EOICount : 5;
    } reg;
} SOC_GIC_NS_GICH_HCR_UNION;
#endif
#define SOC_GIC_NS_GICH_HCR_En_START (0)
#define SOC_GIC_NS_GICH_HCR_En_END (0)
#define SOC_GIC_NS_GICH_HCR_UIE_START (1)
#define SOC_GIC_NS_GICH_HCR_UIE_END (1)
#define SOC_GIC_NS_GICH_HCR_LRENPIE_START (2)
#define SOC_GIC_NS_GICH_HCR_LRENPIE_END (2)
#define SOC_GIC_NS_GICH_HCR_NPIE_START (3)
#define SOC_GIC_NS_GICH_HCR_NPIE_END (3)
#define SOC_GIC_NS_GICH_HCR_VGrp0EIE_START (4)
#define SOC_GIC_NS_GICH_HCR_VGrp0EIE_END (4)
#define SOC_GIC_NS_GICH_HCR_VGrp0DIE_START (5)
#define SOC_GIC_NS_GICH_HCR_VGrp0DIE_END (5)
#define SOC_GIC_NS_GICH_HCR_VGrp1EIE_START (6)
#define SOC_GIC_NS_GICH_HCR_VGrp1EIE_END (6)
#define SOC_GIC_NS_GICH_HCR_VGrp1DIE_START (7)
#define SOC_GIC_NS_GICH_HCR_VGrp1DIE_END (7)
#define SOC_GIC_NS_GICH_HCR_EOICount_START (27)
#define SOC_GIC_NS_GICH_HCR_EOICount_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int ListRegs : 6;
        unsigned int reserved : 20;
        unsigned int PREbits : 3;
        unsigned int PRIbits : 3;
    } reg;
} SOC_GIC_NS_GICH_VTR_UNION;
#endif
#define SOC_GIC_NS_GICH_VTR_ListRegs_START (0)
#define SOC_GIC_NS_GICH_VTR_ListRegs_END (5)
#define SOC_GIC_NS_GICH_VTR_PREbits_START (26)
#define SOC_GIC_NS_GICH_VTR_PREbits_END (28)
#define SOC_GIC_NS_GICH_VTR_PRIbits_START (29)
#define SOC_GIC_NS_GICH_VTR_PRIbits_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int VMGrp0En : 1;
        unsigned int VMGrp1En : 1;
        unsigned int VMAckCtl : 1;
        unsigned int VMFIQEn : 1;
        unsigned int VMCBPR : 1;
        unsigned int reserved_0: 4;
        unsigned int VEM : 1;
        unsigned int reserved_1: 8;
        unsigned int VMABP : 3;
        unsigned int VMBP : 3;
        unsigned int reserved_2: 3;
        unsigned int VMPriMask : 5;
    } reg;
} SOC_GIC_NS_GICH_VMCR_UNION;
#endif
#define SOC_GIC_NS_GICH_VMCR_VMGrp0En_START (0)
#define SOC_GIC_NS_GICH_VMCR_VMGrp0En_END (0)
#define SOC_GIC_NS_GICH_VMCR_VMGrp1En_START (1)
#define SOC_GIC_NS_GICH_VMCR_VMGrp1En_END (1)
#define SOC_GIC_NS_GICH_VMCR_VMAckCtl_START (2)
#define SOC_GIC_NS_GICH_VMCR_VMAckCtl_END (2)
#define SOC_GIC_NS_GICH_VMCR_VMFIQEn_START (3)
#define SOC_GIC_NS_GICH_VMCR_VMFIQEn_END (3)
#define SOC_GIC_NS_GICH_VMCR_VMCBPR_START (4)
#define SOC_GIC_NS_GICH_VMCR_VMCBPR_END (4)
#define SOC_GIC_NS_GICH_VMCR_VEM_START (9)
#define SOC_GIC_NS_GICH_VMCR_VEM_END (9)
#define SOC_GIC_NS_GICH_VMCR_VMABP_START (18)
#define SOC_GIC_NS_GICH_VMCR_VMABP_END (20)
#define SOC_GIC_NS_GICH_VMCR_VMBP_START (21)
#define SOC_GIC_NS_GICH_VMCR_VMBP_END (23)
#define SOC_GIC_NS_GICH_VMCR_VMPriMask_START (27)
#define SOC_GIC_NS_GICH_VMCR_VMPriMask_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int EOI : 1;
        unsigned int U : 1;
        unsigned int LRENP : 1;
        unsigned int NP : 1;
        unsigned int VGrp0E : 1;
        unsigned int VGrp0D : 1;
        unsigned int VGrp1E : 1;
        unsigned int VGrp1D : 1;
        unsigned int reserved : 24;
    } reg;
} SOC_GIC_NS_GICH_MISR_UNION;
#endif
#define SOC_GIC_NS_GICH_MISR_EOI_START (0)
#define SOC_GIC_NS_GICH_MISR_EOI_END (0)
#define SOC_GIC_NS_GICH_MISR_U_START (1)
#define SOC_GIC_NS_GICH_MISR_U_END (1)
#define SOC_GIC_NS_GICH_MISR_LRENP_START (2)
#define SOC_GIC_NS_GICH_MISR_LRENP_END (2)
#define SOC_GIC_NS_GICH_MISR_NP_START (3)
#define SOC_GIC_NS_GICH_MISR_NP_END (3)
#define SOC_GIC_NS_GICH_MISR_VGrp0E_START (4)
#define SOC_GIC_NS_GICH_MISR_VGrp0E_END (4)
#define SOC_GIC_NS_GICH_MISR_VGrp0D_START (5)
#define SOC_GIC_NS_GICH_MISR_VGrp0D_END (5)
#define SOC_GIC_NS_GICH_MISR_VGrp1E_START (6)
#define SOC_GIC_NS_GICH_MISR_VGrp1E_END (6)
#define SOC_GIC_NS_GICH_MISR_VGrp1D_START (7)
#define SOC_GIC_NS_GICH_MISR_VGrp1D_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int List_register_ECO_status : 32;
    } reg;
} SOC_GIC_NS_GICH_EISR0_UNION;
#endif
#define SOC_GIC_NS_GICH_EISR0_List_register_ECO_status_START (0)
#define SOC_GIC_NS_GICH_EISR0_List_register_ECO_status_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int List_register_status : 32;
    } reg;
} SOC_GIC_NS_GICH_ELSR0_UNION;
#endif
#define SOC_GIC_NS_GICH_ELSR0_List_register_status_START (0)
#define SOC_GIC_NS_GICH_ELSR0_List_register_status_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int Active_priority_bits : 32;
    } reg;
} SOC_GIC_NS_GICH_APR0_UNION;
#endif
#define SOC_GIC_NS_GICH_APR0_Active_priority_bits_START (0)
#define SOC_GIC_NS_GICH_APR0_Active_priority_bits_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int VirtualID : 10;
        unsigned int PhysicalID : 10;
        unsigned int reserved : 3;
        unsigned int Priority : 5;
        unsigned int State : 2;
        unsigned int Grp1 : 1;
        unsigned int HW : 1;
    } reg;
} SOC_GIC_NS_GICH_LR0_UNION;
#endif
#define SOC_GIC_NS_GICH_LR0_VirtualID_START (0)
#define SOC_GIC_NS_GICH_LR0_VirtualID_END (9)
#define SOC_GIC_NS_GICH_LR0_PhysicalID_START (10)
#define SOC_GIC_NS_GICH_LR0_PhysicalID_END (19)
#define SOC_GIC_NS_GICH_LR0_Priority_START (23)
#define SOC_GIC_NS_GICH_LR0_Priority_END (27)
#define SOC_GIC_NS_GICH_LR0_State_START (28)
#define SOC_GIC_NS_GICH_LR0_State_END (29)
#define SOC_GIC_NS_GICH_LR0_Grp1_START (30)
#define SOC_GIC_NS_GICH_LR0_Grp1_END (30)
#define SOC_GIC_NS_GICH_LR0_HW_START (31)
#define SOC_GIC_NS_GICH_LR0_HW_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int VirtualID : 10;
        unsigned int PhysicalID : 10;
        unsigned int reserved : 3;
        unsigned int Priority : 5;
        unsigned int State : 2;
        unsigned int Grp1 : 1;
        unsigned int HW : 1;
    } reg;
} SOC_GIC_NS_GICH_LR1_UNION;
#endif
#define SOC_GIC_NS_GICH_LR1_VirtualID_START (0)
#define SOC_GIC_NS_GICH_LR1_VirtualID_END (9)
#define SOC_GIC_NS_GICH_LR1_PhysicalID_START (10)
#define SOC_GIC_NS_GICH_LR1_PhysicalID_END (19)
#define SOC_GIC_NS_GICH_LR1_Priority_START (23)
#define SOC_GIC_NS_GICH_LR1_Priority_END (27)
#define SOC_GIC_NS_GICH_LR1_State_START (28)
#define SOC_GIC_NS_GICH_LR1_State_END (29)
#define SOC_GIC_NS_GICH_LR1_Grp1_START (30)
#define SOC_GIC_NS_GICH_LR1_Grp1_END (30)
#define SOC_GIC_NS_GICH_LR1_HW_START (31)
#define SOC_GIC_NS_GICH_LR1_HW_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int VirtualID : 10;
        unsigned int PhysicalID : 10;
        unsigned int reserved : 3;
        unsigned int Priority : 5;
        unsigned int State : 2;
        unsigned int Grp1 : 1;
        unsigned int HW : 1;
    } reg;
} SOC_GIC_NS_GICH_LR2_UNION;
#endif
#define SOC_GIC_NS_GICH_LR2_VirtualID_START (0)
#define SOC_GIC_NS_GICH_LR2_VirtualID_END (9)
#define SOC_GIC_NS_GICH_LR2_PhysicalID_START (10)
#define SOC_GIC_NS_GICH_LR2_PhysicalID_END (19)
#define SOC_GIC_NS_GICH_LR2_Priority_START (23)
#define SOC_GIC_NS_GICH_LR2_Priority_END (27)
#define SOC_GIC_NS_GICH_LR2_State_START (28)
#define SOC_GIC_NS_GICH_LR2_State_END (29)
#define SOC_GIC_NS_GICH_LR2_Grp1_START (30)
#define SOC_GIC_NS_GICH_LR2_Grp1_END (30)
#define SOC_GIC_NS_GICH_LR2_HW_START (31)
#define SOC_GIC_NS_GICH_LR2_HW_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int VirtualID : 10;
        unsigned int PhysicalID : 10;
        unsigned int reserved : 3;
        unsigned int Priority : 5;
        unsigned int State : 2;
        unsigned int Grp1 : 1;
        unsigned int HW : 1;
    } reg;
} SOC_GIC_NS_GICH_LR3_UNION;
#endif
#define SOC_GIC_NS_GICH_LR3_VirtualID_START (0)
#define SOC_GIC_NS_GICH_LR3_VirtualID_END (9)
#define SOC_GIC_NS_GICH_LR3_PhysicalID_START (10)
#define SOC_GIC_NS_GICH_LR3_PhysicalID_END (19)
#define SOC_GIC_NS_GICH_LR3_Priority_START (23)
#define SOC_GIC_NS_GICH_LR3_Priority_END (27)
#define SOC_GIC_NS_GICH_LR3_State_START (28)
#define SOC_GIC_NS_GICH_LR3_State_END (29)
#define SOC_GIC_NS_GICH_LR3_Grp1_START (30)
#define SOC_GIC_NS_GICH_LR3_Grp1_END (30)
#define SOC_GIC_NS_GICH_LR3_HW_START (31)
#define SOC_GIC_NS_GICH_LR3_HW_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int EnableGrp0 : 1;
        unsigned int EnableGrp1 : 1;
        unsigned int AckCtl : 1;
        unsigned int FIQEn : 1;
        unsigned int CBPR : 1;
        unsigned int reserved : 4;
        unsigned int EOImode : 1;
        unsigned int HW : 22;
    } reg;
} SOC_GIC_NS_GICV_CTLR_UNION;
#endif
#define SOC_GIC_NS_GICV_CTLR_EnableGrp0_START (0)
#define SOC_GIC_NS_GICV_CTLR_EnableGrp0_END (0)
#define SOC_GIC_NS_GICV_CTLR_EnableGrp1_START (1)
#define SOC_GIC_NS_GICV_CTLR_EnableGrp1_END (1)
#define SOC_GIC_NS_GICV_CTLR_AckCtl_START (2)
#define SOC_GIC_NS_GICV_CTLR_AckCtl_END (2)
#define SOC_GIC_NS_GICV_CTLR_FIQEn_START (3)
#define SOC_GIC_NS_GICV_CTLR_FIQEn_END (3)
#define SOC_GIC_NS_GICV_CTLR_CBPR_START (4)
#define SOC_GIC_NS_GICV_CTLR_CBPR_END (4)
#define SOC_GIC_NS_GICV_CTLR_EOImode_START (9)
#define SOC_GIC_NS_GICV_CTLR_EOImode_END (9)
#define SOC_GIC_NS_GICV_CTLR_HW_START (10)
#define SOC_GIC_NS_GICV_CTLR_HW_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved_0: 3;
        unsigned int Priority : 5;
        unsigned int reserved_1: 24;
    } reg;
} SOC_GIC_NS_GICV_PMR_UNION;
#endif
#define SOC_GIC_NS_GICV_PMR_Priority_START (3)
#define SOC_GIC_NS_GICV_PMR_Priority_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int BinaryPoint : 3;
        unsigned int reserved : 29;
    } reg;
} SOC_GIC_NS_GICV_BPR_UNION;
#endif
#define SOC_GIC_NS_GICV_BPR_BinaryPoint_START (0)
#define SOC_GIC_NS_GICV_BPR_BinaryPoint_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int InterruptID : 10;
        unsigned int CPUID : 3;
        unsigned int reserved : 19;
    } reg;
} SOC_GIC_NS_GICV_IAR_UNION;
#endif
#define SOC_GIC_NS_GICV_IAR_InterruptID_START (0)
#define SOC_GIC_NS_GICV_IAR_InterruptID_END (9)
#define SOC_GIC_NS_GICV_IAR_CPUID_START (10)
#define SOC_GIC_NS_GICV_IAR_CPUID_END (12)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int EOIINTID : 10;
        unsigned int CPUID : 3;
        unsigned int reserved : 19;
    } reg;
} SOC_GIC_NS_GICV_EOIR_UNION;
#endif
#define SOC_GIC_NS_GICV_EOIR_EOIINTID_START (0)
#define SOC_GIC_NS_GICV_EOIR_EOIINTID_END (9)
#define SOC_GIC_NS_GICV_EOIR_CPUID_START (10)
#define SOC_GIC_NS_GICV_EOIR_CPUID_END (12)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int CPUID : 8;
        unsigned int reserved : 24;
    } reg;
} SOC_GIC_NS_GICV_RPR_UNION;
#endif
#define SOC_GIC_NS_GICV_RPR_CPUID_START (0)
#define SOC_GIC_NS_GICV_RPR_CPUID_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int PENDINTID : 10;
        unsigned int CPUID : 3;
        unsigned int reserved : 19;
    } reg;
} SOC_GIC_NS_GICV_HPPIR_UNION;
#endif
#define SOC_GIC_NS_GICV_HPPIR_PENDINTID_START (0)
#define SOC_GIC_NS_GICV_HPPIR_PENDINTID_END (9)
#define SOC_GIC_NS_GICV_HPPIR_CPUID_START (10)
#define SOC_GIC_NS_GICV_HPPIR_CPUID_END (12)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int BinaryPoint : 3;
        unsigned int reserved : 29;
    } reg;
} SOC_GIC_NS_GICV_ABPR_UNION;
#endif
#define SOC_GIC_NS_GICV_ABPR_BinaryPoint_START (0)
#define SOC_GIC_NS_GICV_ABPR_BinaryPoint_END (2)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int InterruptID : 10;
        unsigned int CPUID : 3;
        unsigned int reserved : 19;
    } reg;
} SOC_GIC_NS_GICV_AIAR_UNION;
#endif
#define SOC_GIC_NS_GICV_AIAR_InterruptID_START (0)
#define SOC_GIC_NS_GICV_AIAR_InterruptID_END (9)
#define SOC_GIC_NS_GICV_AIAR_CPUID_START (10)
#define SOC_GIC_NS_GICV_AIAR_CPUID_END (12)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int InterruptID : 10;
        unsigned int CPUID : 3;
        unsigned int reserved : 19;
    } reg;
} SOC_GIC_NS_GICV_AEOIR_UNION;
#endif
#define SOC_GIC_NS_GICV_AEOIR_InterruptID_START (0)
#define SOC_GIC_NS_GICV_AEOIR_InterruptID_END (9)
#define SOC_GIC_NS_GICV_AEOIR_CPUID_START (10)
#define SOC_GIC_NS_GICV_AEOIR_CPUID_END (12)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int PENDINTID : 10;
        unsigned int CPUID : 3;
        unsigned int reserved : 19;
    } reg;
} SOC_GIC_NS_GICV_AHPPIR_UNION;
#endif
#define SOC_GIC_NS_GICV_AHPPIR_PENDINTID_START (0)
#define SOC_GIC_NS_GICV_AHPPIR_PENDINTID_END (9)
#define SOC_GIC_NS_GICV_AHPPIR_CPUID_START (10)
#define SOC_GIC_NS_GICV_AHPPIR_CPUID_END (12)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int Active_priority_bits : 32;
    } reg;
} SOC_GIC_NS_GICV_APR0_UNION;
#endif
#define SOC_GIC_NS_GICV_APR0_Active_priority_bits_START (0)
#define SOC_GIC_NS_GICV_APR0_Active_priority_bits_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int Implementer : 12;
        unsigned int Revision : 4;
        unsigned int ArchVersion : 4;
        unsigned int ProductID : 12;
    } reg;
} SOC_GIC_NS_GICV_IIDR_UNION;
#endif
#define SOC_GIC_NS_GICV_IIDR_Implementer_START (0)
#define SOC_GIC_NS_GICV_IIDR_Implementer_END (11)
#define SOC_GIC_NS_GICV_IIDR_Revision_START (12)
#define SOC_GIC_NS_GICV_IIDR_Revision_END (15)
#define SOC_GIC_NS_GICV_IIDR_ArchVersion_START (16)
#define SOC_GIC_NS_GICV_IIDR_ArchVersion_END (19)
#define SOC_GIC_NS_GICV_IIDR_ProductID_START (20)
#define SOC_GIC_NS_GICV_IIDR_ProductID_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int InterruptID : 10;
        unsigned int CPUID : 3;
        unsigned int reserved : 19;
    } reg;
} SOC_GIC_NS_GICV_DIR_UNION;
#endif
#define SOC_GIC_NS_GICV_DIR_InterruptID_START (0)
#define SOC_GIC_NS_GICV_DIR_InterruptID_END (9)
#define SOC_GIC_NS_GICV_DIR_CPUID_START (10)
#define SOC_GIC_NS_GICV_DIR_CPUID_END (12)
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
#endif
