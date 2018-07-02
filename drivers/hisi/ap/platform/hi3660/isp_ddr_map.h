#ifndef __ISP_DDR_MAP_H__
#define __ISP_DDR_MAP_H__ 
#include <global_ddr_map.h>
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
#define MEM_SEC_BOOT_SIZE (0x00C00000)
#define MEM_SEC_HEAP_SIZE (0x01800000)
#define MEM_SMMU_ERRRD_ADDR_OFFSET (0x0000F000)
#define MEM_SMMU_ERRWR_ADDR_OFFSET (0x0000F800)
#define MEM_BOOT_ADDR_OFFSET (0x00010000)
#define MEM_OS_TEXT_ADDR_OFFSET (0x00018000)
#define MEM_PGD_ADDR_OFFSET (0x00000000)
#define MEM_PMD_ADDR_OFFSET (0x00002000)
#define MEM_PTE_ADDR_OFFSET (0x00810000)
#define MEM_RSCTABLE_ADDR_OFFSET (0x00014000)
#define MEM_RSCTABLE_SIZE (0x00004000)
#define MEM_ISPFW_SIZE (0x00800000)
#define MEM_ISPDTS_SIZE (0x02000000)
#define ISP_CORE_CFG_BASE_ADDR (0xE8400000)
#define ISP_PMC_BASE_ADDR (0xFFF31000)
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif
