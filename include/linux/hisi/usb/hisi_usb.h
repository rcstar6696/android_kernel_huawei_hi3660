#ifndef _HISI_USB_H_
#define _HISI_USB_H_

#include <linux/usb.h>
#include <linux/hisi/usb/hisi_hifi_usb.h>

enum hisi_charger_type {
	CHARGER_TYPE_SDP = 0,		/* Standard Downstreame Port */
	CHARGER_TYPE_CDP,		/* Charging Downstreame Port */
	CHARGER_TYPE_DCP,		/* Dedicate Charging Port */
	CHARGER_TYPE_UNKNOWN,		/* non-standard */
	CHARGER_TYPE_NONE,		/* not connected */

	/* other messages */
	PLEASE_PROVIDE_POWER,		/* host mode, provide power */
	CHARGER_TYPE_ILLEGAL,		/* illegal type */
};

/**
 * event types passed to hisi_usb_otg_event().
 */
enum otg_dev_event_type {
	CHARGER_CONNECT_EVENT = 0,
	CHARGER_DISCONNECT_EVENT,
	ID_FALL_EVENT,
	ID_RISE_EVENT,
	START_HIFI_USB,
	START_HIFI_USB_RESET_VBUS,
	STOP_HIFI_USB,
	START_AP_USE_HIFIUSB,
	STOP_AP_USE_HIFIUSB,
	HIFI_USB_HIBERNATE,
	HIFI_USB_WAKEUP,
	NONE_EVENT,
	MAX_EVENT_TYPE = BITS_PER_LONG,
};

/**
 * event types notify user-space host abnormal event.
 */
enum usb_host_abnormal_event_type {
	USB_HOST_EVENT_NORMAL,
	USB_HOST_EVENT_POWER_INSUFFICIENT,
	USB_HOST_EVENT_HUB_TOO_DEEP,
	USB_HOST_EVENT_UNKNOW_DEVICE
};

#ifdef CONFIG_TCPC_CLASS
extern void usb_host_abnormal_event_notify(unsigned int event);
#else
static inline void usb_host_abnormal_event_notify(unsigned int event) {}
#endif


#if defined(CONFIG_USB_SUSB_HDRC) || defined(CONFIG_USB_DWC3)
int hisi_charger_type_notifier_register(struct notifier_block *nb);
int hisi_charger_type_notifier_unregister(struct notifier_block *nb);
enum hisi_charger_type hisi_get_charger_type(void);

/**
 * hisi_usb_otg_event() - Queue a event to be processed.
 * @evnet_type: the event to be processed.
 *
 * The event will be added to tail of a queue, and processed in a work.
 *
 * Return: 0 means the event added sucessfully. others means event was rejected.
 */
int hisi_usb_otg_event(enum otg_dev_event_type);
int hisi_usb_otg_event_sync(enum otg_dev_event_type);

void hisi_usb_otg_bc_again(void);
int hisi_usb_otg_irq_notifier_register(struct notifier_block *nb);
int hisi_usb_otg_irq_notifier_unregister(struct notifier_block *nb);
int hisi_usb_wakeup_hifi_usb(void);

#else
static inline int hisi_charger_type_notifier_register(
		struct notifier_block *nb){return 0;}
static inline int hisi_charger_type_notifier_unregister(
		struct notifier_block *nb){return 0;}
static inline enum hisi_charger_type hisi_get_charger_type(void)
{
	return CHARGER_TYPE_NONE;
}
static inline int hisi_usb_otg_event(enum otg_dev_event_type event_type)
{
	return 0;
}
static inline int hisi_usb_otg_event_sync(enum otg_dev_event_type event_type)
{
	return 0;
}
static inline void hisi_usb_otg_bc_again(void)
{
}
int hisi_usb_otg_irq_notifier_register(
	struct notifier_block *nb){return 0;}
int hisi_usb_otg_irq_notifier_unregister(
	struct notifier_block *nb){return 0;}
int hisi_usb_wakeup_hifi_usb(void){return 0;}
#endif /* CONFIG_USB_SUSB_HDRC || CONFIG_USB_DWC3 */

static inline int hisi_usb_id_change(enum otg_dev_event_type event)
{
	if ((event == ID_FALL_EVENT) || (event == ID_RISE_EVENT))
		return hisi_usb_otg_event(event);
	else
		return 0;
}

/* USB DMD */
#if defined(CONFIG_HUAWEI_DSM)
struct dsm_client;

struct dsm_client *get_usb_dsm_client(void);

#define usb_dsm_report(err_no, fmt, args...) \
do { \
	if (get_usb_dsm_client()) { \
		if(!dsm_client_ocuppy(get_usb_dsm_client())) { \
			dsm_client_record(get_usb_dsm_client(), fmt, ##args); \
			dsm_client_notify(get_usb_dsm_client(), err_no); \
			pr_info("[USB_DSM]usb dsm report err_no:%d\n", err_no); \
		} else \
			pr_err("[USB_DSM]usb dsm is busy! err_no:%d\n", err_no); \
	} else \
		pr_err("[USB_DSM]usb dsm clinet is NULL!\n"); \
} while (0)
#else
#define usb_dsm_report(err_no, fmt, args...)
#endif

#endif /* _HISI_USB_H_*/
