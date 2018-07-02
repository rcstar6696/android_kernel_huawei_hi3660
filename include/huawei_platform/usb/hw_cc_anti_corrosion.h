#ifndef HISI_USB_CC_CORROSION_H
#define HISI_USB_CC_CORROSION_H

#include <linux/mtd/hisi_nve_interface.h>
#define HISI_CORROSION_NV_NAME         "CGNF"
#define HISI_CORROSION_NV_NUM          380
#define HISI_CORROSION_NV_LEN          1

struct cc_corrosion_ops{
	void (*set_cc_mode)(int mode);
	int (*get_cc_mode)(void);
};

struct cc_anti_corrosion_dev {
	struct platform_device *pdev;
	struct notifier_block nb;
	int irq_gpio;
	int irq;
	struct delayed_work  update_nv_work;
	struct delayed_work  intb_work;
	bool is_first_work;
	struct mutex int_lock;
	struct mutex nv_lock;
	struct pinctrl* pctrl;
	struct pinctrl_state *pins_default;
	int pre_gpio_val;
};

void cc_corrosion_register_ops(struct cc_corrosion_ops *ops);
int hw_cc_corrosion_write_nv(unsigned char info);
int hw_cc_corrosion_read_nv(unsigned char *info);

#define hw_usb_dbg(format, arg...)    \
        do {                 \
                printk(KERN_INFO "[USB_DEBUG][%s]"format, __func__, ##arg); \
        } while (0)
#define hw_usb_err(format, arg...)    \
        do {                 \
                printk(KERN_ERR "[USB_DEBUG]"format, ##arg); \
        } while (0)

#endif

