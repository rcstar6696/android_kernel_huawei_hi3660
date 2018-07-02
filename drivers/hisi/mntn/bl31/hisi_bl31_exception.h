
#ifndef __BL31_EXCEPTION_H
#define __BL31_EXCEPTION_H

#define BL31_PANIC_MAGIC 0xdead
#define BL31_DEBUG_FN_VAL 0xc800aa02

#ifdef CONFIG_HISI_BL31_MNTN
extern void bl31_panic_ipi_handle(void);
extern u32 get_bl31_exception_flag(void);
#else
static inline void bl31_panic_ipi_handle(void)
{
}
static inline u32 get_bl31_exception_flag(void)
{
	return 0;
}

#endif

#endif
