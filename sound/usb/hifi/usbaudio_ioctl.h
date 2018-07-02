#ifndef __USBAUDIO_IOCTL_H
#define __USBAUDIO_IOCTL_H

enum usbaudio_stream_type
{
	DOWNLINK_STREAM = 0,
	UPLINK_STREAM,
	TYPE_MAX
};

enum usbaudio_stream_status
{
	START_STREAM = 0,
	STOP_STREAM,
	STATUS_MAX
};

enum usbaudio_controller_status
{
	ACPU_CONTROL = 0,
	DSP_CONTROL,
	CONTROL_MAX
};

enum usbaudio_dsp_reset_status
{
	USBAUDIO_DSP_NORMAL = 0,
	USBAUDIO_DSP_ABNORMAL,
};

struct usbaudio_info
{
	unsigned int usbid;
	unsigned int dnlink_rate_table[5];
	int controller_location;
	unsigned short uplink_channels;
	unsigned short dnlink_channels;
	unsigned char name[256];
	int sr_status;
};

void usbaudio_ctrl_query_info(struct usbaudio_info *usbinfo);
void usbaudio_ctrl_hifi_reset_inform(void);
int usbaudio_ctrl_usb_resume(void);
#endif /* __USBAUDIO_IOCTL_H*/
