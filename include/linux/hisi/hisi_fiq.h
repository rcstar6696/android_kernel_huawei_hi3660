#ifndef __HISI_FIQ_H
#define __HISI_FIQ_H

/*
* Call a function on IPI_MNTN_INFORM
*/
#ifdef CONFIG_HISI_FIQ
extern void hisi_mntn_inform(void);
#else
static inline void hisi_mntn_inform(void){}
#endif

#endif
