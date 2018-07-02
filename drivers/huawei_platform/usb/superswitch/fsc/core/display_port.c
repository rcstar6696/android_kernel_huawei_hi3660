/*
 *  display_port.cpp
 *
 *  The display port implementation.
 */

#ifdef FSC_HAVE_DP

#include "display_port_types.h"
#include "display_port.h"
#include "PDTypes.h"
#include "port.h"
#include "vdm.h"
#include "vdm_types.h"
#include "protocol.h"

//TODO: Update DP to use data objects, not U32
void FUSB3601_requestDpStatus(struct Port *port)
{
  doDataObject_t svdmh = {0};
  FSC_U32 length = 0;
  FSC_U32 arr[2] = {0};
  doDataObject_t temp[2] = {{0}};
  svdmh.SVDM.SVID = DP_SID;
  svdmh.SVDM.VDMType = STRUCTURED_VDM;
  svdmh.SVDM.Version = STRUCTURED_VDM_VERSION;
  /* saved mode position */
  svdmh.SVDM.ObjPos = port->display_port_data_.DpModeEntered & 0x7;
  svdmh.SVDM.CommandType = INITIATOR;
  svdmh.SVDM.Command = DP_COMMAND_STATUS;
  arr[0] = svdmh.object;
  length++;
  arr[1] = port->display_port_data_.DpStatus.word;
  length++;

  temp[0].object = arr[0];
    temp[1].object = arr[1];
  //SendVdmMessageWithTimeout(port, SOP_TYPE_SOP, arr, length,
//                            port->policy_state_);
  FUSB3601_ProtocolSendData(port, DMTVendorDefined, length, temp, 0, peSinkSendSoftReset, FALSE, SOP_TYPE_SOP);
}

void FUSB3601_requestDpConfig(struct Port *port, DisplayPortConfig_t in)
{
  doDataObject_t svdmh = {0};
  FSC_U32 length = 0;
  FSC_U32 arr[2] = {0};
  doDataObject_t temp[2] = {{0}};
  port->display_port_data_.DpPpRequestedConfig.word = in.word;
  svdmh.SVDM.SVID = DP_SID;
  svdmh.SVDM.VDMType = STRUCTURED_VDM;
  svdmh.SVDM.Version = STRUCTURED_VDM_VERSION;
  svdmh.SVDM.ObjPos = port->display_port_data_.DpModeEntered & 0x7;
  svdmh.SVDM.CommandType = INITIATOR;
  svdmh.SVDM.Command = DP_COMMAND_CONFIG;
  arr[0] = svdmh.object;
  length++;
  arr[1] = in.word;
  length++;

  temp[0].object = arr[0];
    temp[1].object = arr[1];
  //SendVdmMessage(port, SOP_TYPE_SOP, arr, length, port->policy_state_);
  FUSB3601_ProtocolSendData(port, DMTVendorDefined, length, temp, 0, peSinkSendSoftReset, FALSE, SOP_TYPE_SOP);
}

void FUSB3601_WriteDpControls(struct Port *port, FSC_U8* data)
{
  FSC_BOOL en = FALSE;
  FSC_BOOL ame_en = FALSE;
  FSC_U32 m = 0;
  FSC_U32 v = 0;
  FSC_U32 stat = 0;
  en = *data++ ? TRUE : FALSE;
  stat = (FSC_U32)(*data++ );
  stat |= ((FSC_U32)(*data++ ) << 8);
  stat |= ((FSC_U32)(*data++ ) << 16);
  stat |= ((FSC_U32)(*data++ ) << 24);
  ame_en = *data++ ? TRUE : FALSE;
  m = (FSC_U32)(*data++ );
  m |= ((FSC_U32)(*data++ ) << 8);
  m |= ((FSC_U32)(*data++ ) << 16);
  m |= ((FSC_U32)(*data++ ) << 24);
  v = (FSC_U32)(*data++ );
  v |= ((FSC_U32)(*data++ ) << 8);
  v |= ((FSC_U32)(*data++ ) << 16);
  v |= ((FSC_U32)(*data++ ) << 24);
  FUSB3601_configDp(port, en, stat);
  FUSB3601_configAutoDpModeEntry(port, ame_en, m, v);
}

void FUSB3601_ReadDpControls(struct Port *port, FSC_U8* data)
{
  *data++ = (FSC_U8)(port->display_port_data_.DpEnabled);
  *data++ = (FSC_U8)(port->display_port_data_.DpStatus.word);
  *data++ = (FSC_U8)((port->display_port_data_.DpStatus.word >> 8) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpStatus.word >> 16) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpStatus.word >> 24) & 0xFF);
  *data++ = (FSC_U8)(port->display_port_data_.DpAutoModeEntryEnabled);
  *data++ = (FSC_U8)(port->display_port_data_.DpModeEntryMask.word);
  *data++ = (FSC_U8)((port->display_port_data_.DpModeEntryMask.word >> 8) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpModeEntryMask.word >> 16) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpModeEntryMask.word >> 24) & 0xFF);
  *data++ = (FSC_U8)(port->display_port_data_.DpModeEntryValue.word);
  *data++ = (FSC_U8)((port->display_port_data_.DpModeEntryValue.word >> 8) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpModeEntryValue.word >> 16) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpModeEntryValue.word >> 24) & 0xFF);
}

void FUSB3601_ReadDpStatus(struct Port *port, FSC_U8* data)
{
  *data++ = (FSC_U8)(port->display_port_data_.DpConfig.word);
  *data++ = (FSC_U8)((port->display_port_data_.DpConfig.word >> 8) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpConfig.word >> 16) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpConfig.word >> 24) & 0xFF);
  *data++ = (FSC_U8)(port->display_port_data_.DpPpStatus.word);
  *data++ = (FSC_U8)((port->display_port_data_.DpPpStatus.word >> 8) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpPpStatus.word >> 16) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpPpStatus.word >> 24) & 0xFF);
  *data++ = (FSC_U8)(port->display_port_data_.DpPpConfig.word);
  *data++ = (FSC_U8)((port->display_port_data_.DpPpConfig.word >> 8) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpPpConfig.word >> 16) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpPpConfig.word >> 24) & 0xFF);
}

void FUSB3601_resetDp(struct Port *port)
{
  port->display_port_data_.DpStatus.word = 0x0;
  port->display_port_data_.DpConfig.word = 0x0;
  port->display_port_data_.DpPpStatus.word = 0x0;
  port->display_port_data_.DpPpRequestedConfig.word = 0x0;
  port->display_port_data_.DpPpConfig.word = 0x0;
  port->display_port_data_.DpModeEntered = 0x0;
}

FSC_BOOL FUSB3601_processDpCommand(struct Port *port, FSC_U32* arr_in)
{
  doDataObject_t svdmh_in = {0};
  DisplayPortStatus_t stat;
  DisplayPortConfig_t config;

  if (port->display_port_data_.DpEnabled == FALSE) return TRUE;

  svdmh_in.object = arr_in[0];
  switch (svdmh_in.SVDM.Command) {
    case DP_COMMAND_STATUS:
      if (svdmh_in.SVDM.CommandType == INITIATOR) {
        if (port->display_port_data_.DpModeEntered == FALSE) return TRUE;
        stat.word = arr_in[1];
        FUSB3601_informStatus(port, stat);
        FUSB3601_updateStatusData(port); /*  get updated info from system */
        FUSB3601_sendStatusData(port, svdmh_in); /*  send it out */
      }
      else {
        stat.word = arr_in[1];
        FUSB3601_informStatus(port, stat);
      }
      break;
    case DP_COMMAND_CONFIG:
      if (svdmh_in.SVDM.CommandType == INITIATOR) {
        if (port->display_port_data_.DpModeEntered == FALSE) return TRUE;
        config.word = arr_in[1];
        if (FUSB3601_DpReconfigure(port, config) == TRUE) {
          /* if pin reconfig is successful */
          FUSB3601_replyToConfig(port, svdmh_in, TRUE);
        }
        else {
          /* if pin reconfig is NOT successful */
          FUSB3601_replyToConfig(port, svdmh_in, FALSE);
        }
      }
      else {
        if (svdmh_in.SVDM.CommandType == RESPONDER_ACK) {
          FUSB3601_informConfigResult(port, TRUE);
        }
        else {
          FUSB3601_informConfigResult(port, FALSE);
        }
      }
      break;
    default:
      /* command not recognized */
      return TRUE;
  }
  return FALSE;
}

FSC_BOOL FUSB3601_dpEvaluateModeEntry(struct Port *port, FSC_U32 mode_in)
{
  DisplayPortCaps_t field_mask = {0};
  DisplayPortCaps_t temp = {0};
  if (port->display_port_data_.DpEnabled == FALSE) return FALSE;
  if (port->display_port_data_.DpAutoModeEntryEnabled == FALSE) return FALSE;

  /*  Mask works on fields at a time, so fix that here for incomplete values */
  /*  Field must be set to all 0s in order to be unmasked */
  /*  TODO - Magic numbers! */
  field_mask.word = port->display_port_data_.DpModeEntryMask.word;
  if (field_mask.field0) field_mask.field0 = 0x3;
  if (field_mask.field1) field_mask.field1 = 0x3;
  if (field_mask.field2) field_mask.field2 = 0x1;
  if (field_mask.field3) field_mask.field3 = 0x1;
  if (field_mask.field4) field_mask.field4 = 0x3F;
  if (field_mask.field5) field_mask.field5 = 0x1F;
  field_mask.fieldrsvd0 = 0x3;
  field_mask.fieldrsvd1 = 0x3;
  field_mask.fieldrsvd2 = 0x7FF;

  /*  For unmasked fields, at least one bit must match */
  temp.word = mode_in & port->display_port_data_.DpModeEntryValue.word;

  /*  Then, forget about the masked fields */
  temp.word = temp.word | field_mask.word;

  /*  At this point, if every field is non-zero, enter the mode */
  if ((temp.field0 != 0) && (temp.field1 != 0) && (temp.field2 != 0)
      && (temp.field3 != 0) && (temp.field4 != 0) && (temp.field5 != 0)) {
    return TRUE;
  }
  else {
    return FALSE;
  }
}

/*  Internal helper functions */
void FUSB3601_configDp(struct Port *port, FSC_BOOL enabled, FSC_U32 status)
{
  port->display_port_data_.DpEnabled = enabled;
  port->display_port_data_.DpStatus.word = status;
}

void FUSB3601_configAutoDpModeEntry(struct Port *port, FSC_BOOL enabled,
                                        FSC_U32 mask, FSC_U32 value)
{
  port->display_port_data_.DpAutoModeEntryEnabled = enabled;
  port->display_port_data_.DpModeEntryMask.word = mask;
  port->display_port_data_.DpModeEntryValue.word = value;
}

void FUSB3601_sendStatusData(struct Port *port, doDataObject_t svdmh_in)
{
  doDataObject_t svdmh_out = {0};
  FSC_U32 length_out = 0;
  FSC_U32 arr_out[2] = {0};
  doDataObject_t temp[2] = {{0}};

  port->display_port_data_.DpStatus.Enabled = 1;

  /*  Reflect most fields */
  svdmh_out.object = svdmh_in.object;
  svdmh_out.SVDM.Version = STRUCTURED_VDM_VERSION;
  svdmh_out.SVDM.CommandType = RESPONDER_ACK;
  arr_out[0] = svdmh_out.object;
  length_out++;
  arr_out[1] = port->display_port_data_.DpStatus.word;
  length_out++;

  temp[0].object = arr_out[0];
  temp[1].object = arr_out[1];

  //SendVdmMessage(port, SOP_TYPE_SOP, arr_out, length_out,
//                 port->policy_state_);
  FUSB3601_ProtocolSendData(port, DMTVendorDefined, length_out, temp, 0, peSinkSendSoftReset, FALSE, SOP_TYPE_SOP);
}

void FUSB3601_replyToConfig(struct Port *port, doDataObject_t svdmh_in,
                                FSC_BOOL success)
{
  doDataObject_t svdmh_out = {0};
  FSC_U32 length_out = 0;
  FSC_U32 arr_out[2] = {0};
  doDataObject_t temp[2] = {{0}};

  /*  Reflect most fields */
  svdmh_out.object = svdmh_in.object;
  svdmh_out.SVDM.Version = STRUCTURED_VDM_VERSION;
  svdmh_out.SVDM.CommandType = success == TRUE ? RESPONDER_ACK : RESPONDER_NAK;
  arr_out[0] = svdmh_out.object;
  length_out++;

  temp[0].object = arr_out[0];
//  SendVdmMessage(port, SOP_TYPE_SOP, arr_out, length_out,
//                 port->policy_state_);
  FUSB3601_ProtocolSendData(port, DMTVendorDefined, length_out, temp, 0, peSinkSendSoftReset, FALSE, SOP_TYPE_SOP);
}

void FUSB3601_informStatus(struct Port *port, DisplayPortStatus_t stat)
{
  /*
   * TODO: 'system' should implement this
   * This function should be called to inform the 'system' of the DP status of
   * the port partner.
   */
  port->display_port_data_.DpPpStatus.word = stat.word;
}

void FUSB3601_updateStatusData(struct Port *port)
{
  /*
   * TODO: 'system' should implement this
   * Called to get an update of our status - to be sent to the port partner
   */
}

void FUSB3601_informConfigResult(struct Port *port, FSC_BOOL success)
{
  /*
   * TODO: 'system' should implement this
   * Called when a config message is either ACKd or NAKd by the other side
   */
  if (success == TRUE) {
    port->display_port_data_.DpPpConfig.word =
        port->display_port_data_.DpPpRequestedConfig.word;
  }
}

FSC_BOOL FUSB3601_DpReconfigure(struct Port *port, DisplayPortConfig_t config) {
  /*
   *  TODO: 'system' should implement this
   *  Called with a DisplayPort configuration to do!
   *  Return TRUE if/when successful, FALSE otherwise
   */
  port->display_port_data_.DpConfig.word = config.word;
  /*  Must actually change configurations here before returning TRUE */
  return TRUE;
}

#endif /*  FSC_HAVE_DP */
