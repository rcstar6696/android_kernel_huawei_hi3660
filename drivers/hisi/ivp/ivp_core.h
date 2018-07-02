#ifndef _IVP_CORE_H_
#define _IVP_CORE_H_

#include "ivp_platform.h"

extern u32 ivp_reg_read(unsigned int off);
extern void ivp_reg_write(unsigned int off, u32 val);
extern u32 ivp_wdg_reg_read(unsigned int off);
extern void ivp_wdg_reg_write(unsigned int off, u32 val);
extern u32 ivp_smmu_reg_read(unsigned int off);
extern u32 ivp_pctrl_reg_read(unsigned int off);

#endif /* _IVP_CORE_H_ */
