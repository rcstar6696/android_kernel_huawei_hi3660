#ifndef __HISI_SP805_WDT_H
#define __HISI_SP805_WDT_H

#ifdef CONFIG_HISI_SP805_WATCHDOG
extern unsigned int get_wdt_kick_time(void);
extern unsigned long get_wdt_expires_time(void);
#else
static inline unsigned int get_wdt_kick_time(void) { return 0; }
static inline unsigned long get_wdt_expires_time(void) { return 0; }
#endif

#endif /*__HISI_SP805_WDT_H*/
