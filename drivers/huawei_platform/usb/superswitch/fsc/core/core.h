
#ifndef _FSC_CORE_H_
#define _FSC_CORE_H_

#include "port.h"
#include "typec.h"
#include "log.h"
#include "version.h"

void FUSB3601_core_initialize(struct Port *port);
void FUSB3601_core_state_machine(struct Port *port);

FSC_U32 FUSB3601_core_get_next_timeout(struct Port *port);

void FUSB3601_core_enable_typec(struct Port *port, FSC_BOOL enable);

#ifdef FSC_DEBUG

void FUSB3601_core_set_state_unattached(struct Port *port);
void FUSB3601_core_reset_pd(void);

FSC_U16 FUSB3601_core_get_advertised_current(void);

FSC_U8 FUSB3601_core_get_cc_orientation(void);
#endif // FSC_DEBUG

void FUSB3601_core_redo_bc12(struct Port *port);

#endif /* _FSC_CORE_H_ */

