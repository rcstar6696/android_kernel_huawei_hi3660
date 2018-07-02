#include "core.h"
#include "typec.h"
#include "bc12.h"
#include "scp.h"
#include "accp.h"
#include "moisture_detection.h"
#include "../Platform_Linux/platform_helpers.h"
#include "../Platform_Linux/fusb3601_global.h"
#include <huawei_platform/log/hw_log.h>
#include <linux/delay.h>
#define HWLOG_TAG core_dot_c
HWLOG_REGIST();
#ifdef PLATFORM_ARM
#include "stdlib.h"
#endif

#define LATENCY_MAX 6000

void FUSB3601_core_initialize(struct Port *port)
{
  FUSB3601_InitializeVars(port, 1, 0x4A);
  FUSB3601_InitializePort(port);
}

void FUSB3601_core_state_machine(struct Port *port)
{
	FSC_U8 data;
	int ret;
  /* Read Type-C/PD interrupts - AlertL, AlertH */
	if((port->registers_.AlertMskL.byte || port->registers_.AlertMskH.byte)
			&& (FUSB3601_platform_get_device_irq_state(port->port_id_))){
		FUSB3601_ReadRegisters(port, regALERTL, 2);

		  /* Read Type-C/PD statuses - CCStat, PwrStat, FaultStat */
		  if(port->registers_.AlertL.I_CCSTAT ||
				  port->registers_.AlertL.I_PORT_PWR ||
				  port->registers_.AlertH.I_FAULT) {
			  FUSB3601_ReadRegisters(port, regCCSTAT, 3);
		  }

		  /* Check and handle a chip reset */
		  if (port->registers_.FaultStat.ALL_REGS_RESET) {
			  FUSB3601_WriteTCState(&port->log_, 555, port->tc_state_);
			FUSB3601_InitializeVars(port, 1, 0x4A);
			FUSB3601_InitializePort(port);
			return;
		  }

		  /* Check for factory mode */
		  if(port->registers_.AlertH.byte == 0x79){
			  port->factory_mode_ = TRUE;
			  FUSB3601_WriteTCState(&port->log_, 498, port->tc_state_);
			  FUSB3601_platform_delay(10 * 1000);
		  }
	}

	//TODO: Stick this in with the above interrupts, but also needs a timer interrupt flag
	  /* Start with TypeC/PD state machines */
	if(!port->factory_mode_) {
		FUSB3601_StateMachineTypeC(port);
	}
	else{
			FUSB3601_WriteTCState(&port->log_, 500, port->tc_state_);
			FUSB3601_platform_i2c_read(port->port_id_, 0x01, 1, &data);
			if(data != 0x79) {
				FUSB3601_WriteTCState(&port->log_, 500+data, port->tc_state_);
				FUSB3601_InitializeVars(port, 1, 0x4A);
				FUSB3601_InitializePort(port);
			}
			FUSB3601_TimerStart(&port->tc_state_timer_, ktPDDebounce);
			return;
		}

	/* Read SCP interrupts - Event1,2,3 */
//	if(port->registers_.SCPInt1Msk.byte || port->registers_.SCPInt2Msk.byte) {
//		ReadRegisters(port, regEVENT_1, 3);
//
//		/* Clear unneeded interrupts */
//		if (port->registers_.Event1.CC_PLGIN) {
//		 ClearInterrupt(port, regEVENT_1, MSK_CC_PLGIN);
//		}
//
//		/* Continue with SCP/ACCP handling, if applicable */
//		if (port->activemode_ == Mode_None && port->registers_.Event1.byte) {
//		  if (port->registers_.Event1.ACCP_PLGIN) {
//			ConnectACCP(port);
//		  }
//		  else if (port->registers_.Event1.SCP_PLGIN) {
//			ConnectSCP(port);
//		  }
//		}
//		else if (port->activemode_ == Mode_ACCP) {
//		  ProcessACCP(port);
//		}
//		else if (port->activemode_ == Mode_SCP_A ||
//				 port->activemode_ == Mode_SCP_B) {
//		  ProcessSCP(port);
//		}
//	}

	/* Read MUS interrupts */
	if(port->registers_.MUSIntMask.byte) {
		FUSB3601_ReadRegister(port, regMUS_INTERRUPT);

		  /* Check BC1.2 Attach status */
		  if (port->registers_.MUSInterrupt.I_BC_ATTACH) {
		    ConnectBC12(port);
		  }
		  else if (port->bc12_active_) {
		    ProcessBC12(port);
		  } else if(port->registers_.MUSInterrupt.I_H2O_DET) {
		    moisture_detection_complete();
		  }
                
		
		//ret = fusb_I2C_ReadData(regMUS_INTERRUPT,&data);
		//hwlog_info("FUSB%s:regMUS_INTERRUPT before = [0x%x]\n",__func__,data);
		  /* Process MUS */
		if (port->registers_.MUSInterrupt.byte) {
		  FUSB3601_ClearInterrupt(port, regMUS_INTERRUPT, MSK_MUS_ALL);
		}
		//ret = fusb_I2C_ReadData(regMUS_INTERRUPT,&data);
		//hwlog_info("FUSB%s:regMUS_INTERRUPT after = [0x%x]\n",__func__,data);
	}

	/* Read OVP interrupts */
	if(port->registers_.DataOVPMsk.byte) {
		FUSB3601_ReadRegister(port, regDATA_OVP_INT);

	  /* Process Vendor Defined */
	  if (port->registers_.DataOVPInt.byte) {
		FUSB3601_ClearInterrupt(port, regDATA_OVP_INT, MSK_OVP_ALL);
	  }
	}
}

void FUSB3601_core_enable_typec(struct Port *port, FSC_BOOL enable)
{
  port->tc_enabled_ = enable;
}

FSC_U32 FUSB3601_core_get_next_timeout(struct Port *port)
{
  FSC_U32 time = 0;
  FSC_U32 nexttime = 0xFFFFFFFF;

  time = FUSB3601_TimerRemaining(&port->tc_state_timer_);
  if (time > 0 && time < nexttime) nexttime = time;

  time = FUSB3601_TimerRemaining(&port->policy_state_timer_);
  if (time > 0 && time < nexttime) nexttime = time;

  time = FUSB3601_TimerRemaining(&port->pd_debounce_timer_);
  if (time > 0 && time < nexttime) nexttime = time;

  time = FUSB3601_TimerRemaining(&port->cc_debounce_timer_);
  if (time > 0 && time < nexttime) nexttime = time;

  time = FUSB3601_TimerRemaining(&port->vdm_timer_);
    if (time > 0 && time < nexttime) nexttime = time;

  if (nexttime == 0xFFFFFFFF) nexttime = 0;

  return nexttime;
}

/*
 * Call this function to get the lower 8-bits of the core revision number.
 */
FSC_U8 FUSB3601_core_get_rev_lower(void)
{
    return FSC_TYPEC_CORE_FW_REV_LOWER;
}

/*
 * Call this function to get the middle 8-bits of the core revision number.
 */
FSC_U8 FUSB3601_core_get_rev_middle(void)
{
    return FSC_TYPEC_CORE_FW_REV_MIDDLE;
}

/*
 * Call this function to get the upper 8-bits of the core revision number.
 */
FSC_U8 FUSB3601_core_get_rev_upper(void)
{
    return FSC_TYPEC_CORE_FW_REV_UPPER;
}

FSC_U8 FUSB3601_core_get_cc_orientation(void)
{
    /* TODO */
    return 0;
}

void FUSB3601_core_configure_port_type(FSC_U8 config) 
{
}

void FUSB3601_core_enable_pd(FSC_BOOL enable)
{
    //if (enable == TRUE) EnableUSBPD();
    //else                DisableUSBPD();
}

void FUSB3601_core_send_hard_reset(void)
{
    //FUSB3601_SendUSBPDHardReset();
}

void FUSB3601_core_set_state_unattached(struct Port *port)
{
    FUSB3601_SetStateUnattached(port);
}

void FUSB3601_core_reset_pd(void)
{
    //FUSB3601_EnableUSBPD();
    //FUSB3601_USBPDEnable(TRUE, sourceOrSink);
}

void FUSB3601_core_redo_bc12(struct Port *port)
{
	struct fusb3601_chip* chip = fusb3601_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
        return;
    }
	
	down(&chip->suspend_lock);
	
	if((port->tc_state_ == AttachedSink)
		|| (port->tc_state_ == DebugAccessorySink)) {
	
		port->registers_.PwrCtrl.AUTO_DISCH = 0;
		FUSB3601_WriteRegister(port, regPWRCTRL);

		FUSB3601_SendCommand(port, DisableSinkVbus);
		
		FUSB3601_SendCommand(port, DisableVbusDetect);
		
		FUSB3601_SendCommand(port, EnableVbusDetect);
		
		port->registers_.PwrCtrl.AUTO_DISCH = 1;
		FUSB3601_WriteRegister(port, regPWRCTRL);
		
		FUSB3601_SendCommand(port, SinkVbus);
	}
	usleep_range(800000,801000);	
	up(&chip->suspend_lock);
}
