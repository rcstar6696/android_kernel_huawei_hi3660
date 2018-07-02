#ifndef _HIFI_USB_H_
#define _HIFI_USB_H_

#include <linux/mutex.h>
#include <linux/completion.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/wakelock.h>
#include <linux/notifier.h>
#include <linux/usb/hifi-usb-mailbox.h>

#include "proxy-hcd.h"
#include "hifi-usb-urb-buf.h"
#include "hifi-usb-stat.h"

#define HIFI_USB_CONFIRM_UDEV_TIME (5 * HZ)

struct hifi_usb_msg_wrap {
	struct list_head node;
	struct hifi_usb_op_msg msg;
};

struct hifi_usb_proxy {
	struct proxy_hcd_client 	*client;
	struct dentry 			*debugfs_root;

	/* for message process */
	struct mutex 			msg_lock;
	struct completion 		msg_completion;
	struct work_struct		msg_work;
	struct list_head		msg_queue;
	struct hifi_usb_op_msg 		op_msg;
	struct hifi_usb_runstop_msg 	runstop_msg;
	unsigned int			runstop;

	struct urb_buffers 		urb_bufs;
	struct list_head		complete_urb_list;
	struct timer_list 		confirm_udev_timer;
	struct wake_lock 		hifi_usb_wake_lock;

	spinlock_t			lock; /* for complete_urb_list */

	struct hifi_usb_stats 		stat;

	/* for hibernation */
	unsigned int			hibernation_policy;
	unsigned int			hibernation_state:1;
	unsigned int			hibernation_support:1;
	unsigned int 			ignore_port_status_change_once:1;

	struct notifier_block		fb_notify;
	unsigned int			hibernation_ctrl; /* hibernation allowed when all bits cleared */

	unsigned int 			port_status;
	unsigned int			hibernation_count;
	unsigned int			revive_time;
	unsigned int 			max_revive_time;

	bool				hifiusb_suspended;
	bool				hifiusb_hibernating;
	bool				hid_key_pressed;
	bool				hifi_reset_flag;
};

void hifi_usb_msg_receiver(struct hifi_usb_op_msg *__msg);
int always_use_hifi_usb(int val);
int never_use_hifi_usb(int val);

#endif /* _HIFI_USB_H_ */
