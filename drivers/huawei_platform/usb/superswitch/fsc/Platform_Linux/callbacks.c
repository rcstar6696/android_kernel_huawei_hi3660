#include "../core/callbacks.h"
#include <linux/printk.h>           /* pr_err, printk, etc */
#include <linux/kthread.h>
#include "fusb3601_global.h"        /* Chip structure */
#include "platform_helpers.h"       /* Implementation details */

#include <huawei_platform/usb/hw_pd_dev.h>

#define TCP_VBUS_CTRL_PD_DETECT (1 << 7)

typedef enum
{
    TCPC_CTRL_PLUG_ORIENTATION = (1 << 0),
    TCPC_CTRL_BIST_TEST_MODE   = (1 << 1)
} tcpc_ctrl_t;

struct pd_dpm_vbus_state get_vbus_state;

/*******************************************************************************
* Function:        platform_notify_cc_orientation
* Input:           orientation - Orientation of CC (NONE, CC1, CC2)
* Return:          None
* Description:     A callback used by the core to report to the platform the
*                  current CC orientation. Called in SetStateAttached... and
*                  SetStateUnattached functions.
******************************************************************************/
void FUSB3601_platform_notify_cc_orientation(CCOrientation orientation)
{

	// Optional: Notify platform of CC orientation
	FSC_U8 cc1State = 0, cc2State = 0;
	struct pd_dpm_typec_state tc_state;

    struct fusb3601_chip* chip = fusb3601_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
        return;
    }

	/* For Force States */
	/* Not Implemented *
	if((orientation != NONE) && (chip->port.port_type_ != USBTypeC_DRP))
	{
		fusb_StopTimers(&chip->timer_force_timeout);
		chip->port.port_type_ = USBTypeC_DRP;
	}
	*/

	tc_state.polarity = orientation;
	pr_info("FUSB  %s - orientation = %d, sourceOrSink = %s\n", __func__, orientation, (chip->port.source_or_sink_ == Source) ? "SOURCE" : "SINK");
	if (orientation != NONE)
	{
		if (chip->port.source_or_sink_ == Source)
		{
			tc_state.new_state = PD_DPM_TYPEC_ATTACHED_SRC;
		}
		else
		{
			tc_state.new_state = PD_DPM_TYPEC_ATTACHED_SNK;
		}
	}
	else if(orientation == NONE)
	{
		tc_state.new_state = PD_DPM_TYPEC_UNATTACHED;
	}

	// cc1State & cc2State = pull-up or pull-down status of each CC line:
	// CCTypeOpen		= 0
	// CCTypeRa		= 1
	// CCTypeRdUSB		= 2
	// CCTypeRd1p5		= 3
	// CCTypeRd3p0		= 4
	// CCTypeUndefined	= 5
	tc_state.cc1_status = chip->port.registers_.CCStat.CC1_STAT;
	tc_state.cc2_status = chip->port.registers_.CCStat.CC2_STAT;

//	pr_info("FUSB %s - Entering Orientation Function\n", __func__);
	pd_dpm_handle_pe_event(PD_DPM_PE_EVT_TYPEC_STATE, (void*)&tc_state);
//	pr_info("FUSB %s - Exiting Orientation Function\n", __func__);
}
void FUSB3601_platform_notify_audio_accessory(void)
{
    struct pd_dpm_typec_state tc_state;

    tc_state.new_state = PD_DPM_TYPEC_ATTACHED_AUDIO;

    pd_dpm_handle_pe_event(PD_DPM_PE_EVT_TYPEC_STATE, (void*)&tc_state);
    pr_info("FUSB %s - platform_notify_audio_accessory\n", __func__);
}


/*******************************************************************************
* Function:        platform_notify_pd_contract
* Input:           contract - TRUE: Contract, FALSE: No Contract
* Return:          None
* Description:     A callback used by the core to report to the platform the
*                  current PD contract status. Called in PDPolicy.
*******************************************************************************/
int FUSB3601_notify_thread_sink_vbus(void* vbus_state)
{
	pd_dpm_handle_pe_event(PD_DPM_PE_EVT_SINK_VBUS, (void *)vbus_state);
	return 0;
}
void FUSB3601_platform_notify_pd_contract(FSC_BOOL contract,FSC_U32 pd_voltage, FSC_U32 pd_current, FSC_BOOL externally_powered)
{
    // Optional: Notify platform of PD contract

	struct fusb3601_chip* chip = fusb3601_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
        return;
    }

	//pr_info("FUSB  %s, contract=%d - PD Contract: %dmV/%dmA; g_vbus=%d,sourceOrSink: %s, Externally Powered: %d\n", __func__, contract, PDvoltage * 50, PDcurrent *10, g_vbus, (sourceOrSink == SOURCE) ? "SOURCE" : "SINK", externally_powered_bit);

	if (contract)
	{
		get_vbus_state.vbus_type = TCP_VBUS_CTRL_PD_DETECT;
		get_vbus_state.mv = pd_voltage * 50;
		get_vbus_state.ma = pd_current * 10;
		printk("Enter function: %s, voltage: %d, current: %d\n", __func__, get_vbus_state.mv, (pd_current * 10));   //PengYalong 20170517
		get_vbus_state.ext_power = externally_powered;
		if (chip->port.source_or_sink_ == Sink)
		{
			//pd_dpm_handle_pe_event(PD_DPM_PE_EVT_SINK_VBUS, (void *)&g_vbus_state);
			kthread_run(FUSB3601_notify_thread_sink_vbus, (void*)&get_vbus_state, "notitfy_vbus");
		}
	}
	else {
		if (chip->port.source_or_sink_ == Sink)
		{
			pd_dpm_handle_pe_event(PD_DPM_PE_EVT_DIS_VBUS_CTRL, NULL);
		}
	}
//	pr_info("FUSB  %s - PD Contract: %dmV/%dmA; g_vbus=%d,sourceOrSink: %s\n", __func__, PDvoltage * 50, PDcurrent *10, g_vbus, (chip->port.source_or_sink_ == Source) ? "SOURCE" : "SINK");

}

/*******************************************************************************
* Function:        platform_notify_pd_state
* Input:           state - SOURCE or SINK
* Return:          None
* Description:     A callback used by the core to report to the platform the
*                  PD state status when entering SOURCE or SINK. Called in PDPolicy.
*******************************************************************************/
void FUSB3601_platform_notify_pd_state(SourceOrSink state)
{
    // Optional: Notify platform of PD SRC or SNK state
    //struct pd_dpm_pd_state pd_state;
	//pd_state.connected = (state == Source) ? PD_CONNECT_PE_READY_SRC : PD_CONNECT_PE_READY_SNK;
	//pd_dpm_handle_pe_event(PD_DPM_PE_EVT_PD_STATE, (void *)&pd_state);
	//pr_info("FUSB %s, %s\n", __func__, (state == Source)?"SourceReady":"Sink Ready");
}

/*******************************************************************************
* Function:        platform_notify_unsupported_accessory
* Input:           None
* Return:          None
* Description:     A callback used by the core to report entry to the
*                  Unsupported Accessory state. The platform may implement
*                  USB Billboard.
*******************************************************************************/
void FUSB3601_platform_notify_unsupported_accessory(void)
{
    /* Optional: Implement USB Billboard */
}

/*******************************************************************************
* Function:        platform_set_data_role
* Input:           PolicyIsDFP - Current data role
* Return:          None
* Description:     A callback used by the core to report the new data role after
*                  a data role swap.
*******************************************************************************/
void FUSB3601_platform_notify_data_role(FSC_BOOL PolicyIsDFP)
{
    struct pd_dpm_swap_state swap_state;
	struct fusb3601_chip* chip = fusb3601_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
        return;
    }

	//swap_state.new_role = (chip->port.source_or_sink_ == Source) ? PD_ROLE_DFP : PD_ROLE_UFP;
    //pd_dpm_handle_pe_event(PD_DPM_PE_EVT_DR_SWAP, (void *)&swap_state);
}

/*******************************************************************************
* Function:        platform_notify_bist
* Input:           bistEnabled - TRUE when BIST enabled, FALSE when disabled
* Return:          None
* Description:     A callback that may be used to limit current sinking during
*                  BIST
*******************************************************************************/
void FUSB3601_platform_notify_bist(FSC_BOOL bistEnabled)
{
    /* Do something */
}
