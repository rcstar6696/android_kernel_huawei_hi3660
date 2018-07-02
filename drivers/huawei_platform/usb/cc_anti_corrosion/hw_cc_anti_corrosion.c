#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/wakelock.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/delay.h>
#include <linux/of_address.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/of_gpio.h>
#include <linux/hisi/usb/hisi_usb.h>

#include <huawei_platform/usb/hw_cc_anti_corrosion.h>
#include <huawei_platform/usb/hw_pd_dev.h>

#include <linux/mfd/hisi_pmic.h>
#include <pmic_interface.h>
#include <linux/hisi/hisi_adc.h>

extern int hisi_usb_vbus_value(void);
struct cc_anti_corrosion_dev *cc_corrosion_dev_p = NULL;
#define CC_MODE_DRP 1
#define CC_MODE_UFP 0
#define DEBOUNCE_TIME 10
#define FIRST_DELAY_COUNT 20
#define NV_DELAY_COUNT 30
#define INT_TRIGGER_RISING (IRQF_NO_SUSPEND | IRQF_TRIGGER_RISING)
#define INT_TRIGGER_FALLING (IRQF_NO_SUSPEND | IRQF_TRIGGER_FALLING)
#define NV_CGNF_OK 0
#define NV_CGNF_NOT_OK 1
#define NV_DEFAULT 2
#define VBUS_ON 1
static unsigned char prev_nv_value = NV_DEFAULT;
static struct cc_corrosion_ops *cc_corrosion_ops = NULL;

int hw_cc_corrosion_write_nv(unsigned char info)
{
	struct hisi_nve_info_user nve;
	int ret = 0;
	if (prev_nv_value == info) {
		hw_usb_err("%s nv have no change %d\n", __func__, info);
		return ret;
	}

	memset(&nve,0,sizeof(nve));
	strncpy(nve.nv_name, HISI_CORROSION_NV_NAME, sizeof(HISI_CORROSION_NV_NAME));
	nve.nv_number = HISI_CORROSION_NV_NUM;
	nve.valid_size = HISI_CORROSION_NV_LEN;
	nve.nv_operation = NV_WRITE;

	memcpy(nve.nv_data, &info, sizeof(info));
	ret = hisi_nve_direct_access(&nve);
	if (ret) {
		hw_usb_err("%s save nv partion failed, ret=%d\n", __func__, ret);
	} else {
		hw_usb_err("%s save nv partion success\n", __func__);
		prev_nv_value = info;
	}
	return ret;
}

int hw_cc_corrosion_read_nv(unsigned char *info)
{
	struct hisi_nve_info_user nve;
	int ret = -1;
	if (!info) {
		return ret;
	}
	memset(&nve,0,sizeof(nve));
	strncpy(nve.nv_name, HISI_CORROSION_NV_NAME, sizeof(HISI_CORROSION_NV_NAME));
	nve.nv_number = HISI_CORROSION_NV_NUM;
	nve.valid_size = HISI_CORROSION_NV_LEN;
	nve.nv_operation = NV_READ;

	ret = hisi_nve_direct_access(&nve);
	if (ret) {
		hw_usb_err("%s save nv partion failed, ret= %d\n", __func__, ret);
	} else {
		*info = (unsigned char)nve.nv_data[0];
		prev_nv_value = (unsigned char)nve.nv_data[0];
		hw_usb_err("%s save nv partion success info %d\n", __func__, *info);
	}
	return ret;
}

void cc_corrosion_register_ops(struct cc_corrosion_ops *ops)
{
	if (!ops) {
		hw_usb_err("%s ops or cc_corrosion_dev_p is NULL\n", __func__);
		return;
	}
	cc_corrosion_ops = ops;
}
static void hw_cc_corrosion_first_work(void)
{
	int ret = -1;
	int gpio_value =0;
	int count = FIRST_DELAY_COUNT;
	unsigned char nv_value = 1;

	if (!cc_corrosion_dev_p) {
		hw_usb_err("%s cc_corrosion_dev_p is NULL\n", __func__);
		return;
	}

	hw_usb_err("Enter %s\n", __func__);
	do {
		if(cc_corrosion_ops)
			break;
		msleep(100);
		count--;
	} while(count);

	if (!cc_corrosion_ops) {
		hw_usb_err("%s get cc_corrosion_ops fail\n", __func__);
		return;
	}

	count = NV_DELAY_COUNT;
	do{
		ret = hw_cc_corrosion_read_nv(&nv_value);
		if (!ret) {
			hw_usb_err("%s get corrosion nv success\n", __func__);
			break;
		}
		msleep(100);
		count--;
	} while(count);

	if (ret) {
		hw_usb_err("%s read nv ret %d\n", __func__, ret);
		nv_value = NV_CGNF_OK;
	}

	gpio_value = gpio_get_value(cc_corrosion_dev_p->irq_gpio);

	if ( (NV_CGNF_OK == nv_value) && gpio_value) {
		hw_usb_err("%s set cc mode to UFP\n", __func__);
		cc_corrosion_ops->set_cc_mode(CC_MODE_UFP);
	}else {
		hw_usb_err("%s do noting nv_vlaue %d gpio_value %d\n", __func__, nv_value, gpio_value);
	}
}
static void hw_cc_corrosion_update_nv_work(struct work_struct *work)
{
	int ret;
	int gpio_value = -1;
	unsigned char nv_value = 1;

	if (!cc_corrosion_dev_p || !cc_corrosion_ops) {
		hw_usb_err("%s cc_corrosion_dev_p or ops is NULL\n", __func__);
		return;
	}

	if (VBUS_ON != hisi_usb_vbus_value())
	{
		hw_usb_err("%s vbus is absent\n", __func__);
		return;
	}

	mutex_lock(&cc_corrosion_dev_p->nv_lock);
	gpio_value = gpio_get_value(cc_corrosion_dev_p->irq_gpio);

	nv_value = gpio_value ? NV_CGNF_NOT_OK : NV_CGNF_OK;
	ret = hw_cc_corrosion_write_nv(nv_value);
	if (ret)
	{
		hw_usb_err("%s write nv fail %d ,nv_value %d\n", __func__, ret, nv_value);
	}

	if (gpio_value) {
		hw_usb_err("%s set CC_MODE_DRP gpio_value %d nv_value %d\n", __func__, gpio_value, nv_value);
		cc_corrosion_ops->set_cc_mode(CC_MODE_DRP);
	}
	hw_usb_err("%s gpio_value %d, nv_value %d\n", __func__, gpio_value, nv_value);

	mutex_unlock(&cc_corrosion_dev_p->nv_lock);
}
static void hw_cc_corrosion_intb_work(struct work_struct *work)
{
	int ret;
	int gpio_value;
	unsigned char nv_value = 1;

	if (!cc_corrosion_dev_p) {
		hw_usb_err("%s cc_corrosion_dev_p is NULL\n", __func__);
		return;
	}

	mutex_lock(&cc_corrosion_dev_p->int_lock);
	if (cc_corrosion_dev_p->is_first_work){
		hw_cc_corrosion_first_work();

		cc_corrosion_dev_p->is_first_work = 0;
		mutex_unlock(&cc_corrosion_dev_p->int_lock);
		return;
	}

	if (!cc_corrosion_ops) {
		hw_usb_err("%s cc_corrosion_dev_p ops is NULL\n", __func__);
		mutex_unlock(&cc_corrosion_dev_p->int_lock);
		return;
	}

	gpio_value = gpio_get_value(cc_corrosion_dev_p->irq_gpio);

	ret = hw_cc_corrosion_read_nv(&nv_value);
	if (ret)
	{
		hw_usb_err("%s read nv fail %d\n", __func__, ret);
		nv_value = NV_CGNF_OK;
	}

	if (NV_CGNF_NOT_OK == nv_value)
	{
		hw_usb_err("%s gpio is not ok\n", __func__);
		goto err_bounce;
	}

	if (!gpio_value)
	{
		hw_usb_err("%s set cc mode to DRP when gpio %d\n", __func__, gpio_value);
		cc_corrosion_ops->set_cc_mode(CC_MODE_DRP);
	} else {
		hw_usb_err("%s set cc mode to UFP when gpio %d\n", __func__, gpio_value);
		cc_corrosion_ops->set_cc_mode(CC_MODE_UFP);
	}

err_bounce:
	mutex_unlock(&cc_corrosion_dev_p->int_lock);
}

static irqreturn_t hw_cc_corrosion_irq_handle(int irq, void *dev_id)
{
	int gpio_value = -1;

	disable_irq_nosync(cc_corrosion_dev_p->irq);
	gpio_value = gpio_get_value(cc_corrosion_dev_p->irq_gpio);

	//for IRQ debounce
	if (cc_corrosion_dev_p->pre_gpio_val == gpio_value)
	{
		hw_usb_err("%s gpio do not change %d\n", __func__, gpio_value);
		goto err_gpio;
	} else {
		hw_usb_err("%s gpio change to %d\n", __func__, gpio_value);
		cc_corrosion_dev_p->pre_gpio_val = gpio_value;
		cancel_delayed_work(&cc_corrosion_dev_p->intb_work);
	}

	if (0 == gpio_value)
	{
		irq_set_irq_type(cc_corrosion_dev_p->irq, INT_TRIGGER_RISING);
	} else {
		irq_set_irq_type(cc_corrosion_dev_p->irq, INT_TRIGGER_FALLING);
	}

	enable_irq(cc_corrosion_dev_p->irq);
	schedule_delayed_work(&cc_corrosion_dev_p->intb_work, msecs_to_jiffies(DEBOUNCE_TIME));
	return IRQ_HANDLED;

err_gpio:
	enable_irq(cc_corrosion_dev_p->irq);
	return IRQ_HANDLED;
}

static int hw_cc_anti_corrosion_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device_node *dnp = NULL;
	struct device* dev = NULL;

	hw_usb_err("Enter %s funtion\n", __func__);
	if (NULL == pdev)
	{
		hw_usb_err("%s The pdev is NULL !\n", __func__);
		return ret;
	}

	cc_corrosion_dev_p = (struct cc_anti_corrosion_dev *)devm_kzalloc(&pdev->dev, sizeof(*cc_corrosion_dev_p), GFP_KERNEL);
	if (NULL == cc_corrosion_dev_p)
	{
		hw_usb_err("%s alloc otg_dev failed! not enough memory\n", __func__);
		return ret;
	}

	cc_corrosion_dev_p->pre_gpio_val = -1;
	platform_set_drvdata(pdev, cc_corrosion_dev_p);
	cc_corrosion_dev_p->pdev = pdev;
	dev = &pdev->dev;
	dnp = dev->of_node;

	cc_corrosion_dev_p->irq_gpio = of_get_named_gpio(dnp, "cc_corrosion_gpio", 0);
	if (cc_corrosion_dev_p->irq_gpio < 0)
	{
		hw_usb_err("%s of_get_named_gpio error!!! irq_gpio=%d.\n", __func__, cc_corrosion_dev_p->irq_gpio);
		goto err_of_get_named_gpio;
	}

	mutex_init(&cc_corrosion_dev_p->int_lock);
	mutex_init(&cc_corrosion_dev_p->nv_lock);
	INIT_DELAYED_WORK(&cc_corrosion_dev_p->update_nv_work, hw_cc_corrosion_update_nv_work);
	INIT_DELAYED_WORK(&cc_corrosion_dev_p->intb_work, hw_cc_corrosion_intb_work);
	/*
	* init otg gpio process
	*/
	ret = gpio_request(cc_corrosion_dev_p->irq_gpio, "cc_corrosion_gpio_irq");
	if (ret < 0)
	{
		hw_usb_err("%s gpio_request error!!! ret=%d. irq_gpio=%d.\n", __func__, ret, cc_corrosion_dev_p->irq_gpio);
		goto err_of_get_named_gpio;
	}
	cc_corrosion_dev_p->pctrl = devm_pinctrl_get(dev);

	if (IS_ERR(cc_corrosion_dev_p->pctrl))
	{
		hw_usb_err("%s devm_pinctrl_get fail\n", __func__);
		goto err_devm_pinctrl_get;
	}

	cc_corrosion_dev_p->pins_default = pinctrl_lookup_state(cc_corrosion_dev_p->pctrl, "default");

	if (IS_ERR(cc_corrosion_dev_p->pins_default))
	{
		hw_usb_err("%s pinctrl_lookup_state fail\n", __func__);
		goto err_devm_pinctrl_get;
	}

	ret = pinctrl_select_state(cc_corrosion_dev_p->pctrl, cc_corrosion_dev_p->pins_default);
	if (ret < 0)
	{
		hw_usb_err("%s pinctrl_select_state failed error=%d\n", __func__, ret);
		goto err_devm_pinctrl_get;
	}

	cc_corrosion_dev_p->irq = gpio_to_irq(cc_corrosion_dev_p->irq_gpio);
	if (cc_corrosion_dev_p->irq < 0) {
		hw_usb_err("%s gpio_to_irq error!!! dev_p->gpio=%d, dev_p->irq=%d.\n", __func__, cc_corrosion_dev_p->irq_gpio, cc_corrosion_dev_p->irq);
		goto err_devm_pinctrl_get;
	}

	ret = gpio_direction_input(cc_corrosion_dev_p->irq_gpio);
	if (ret < 0) {
		hw_usb_err("%s gpio_direction_input error!!! ret=%d. gpio=%d.\n", __func__, ret, cc_corrosion_dev_p->irq_gpio);
		goto err_devm_pinctrl_get;
	}

	cc_corrosion_dev_p->is_first_work = 1;

	ret = request_irq(cc_corrosion_dev_p->irq, hw_cc_corrosion_irq_handle,
				IRQF_NO_SUSPEND | IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "cc_corrosion_gpio_irq", NULL);
	if (ret < 0) {
		hw_usb_err("%s request otg irq handle funtion fail!! ret:%d\n", __func__, ret);
		goto err_devm_pinctrl_get;
	}

	schedule_delayed_work(&cc_corrosion_dev_p->intb_work, msecs_to_jiffies(0));

	hw_usb_err("Exit %s funtion\n", __func__);
	return 0;

err_devm_pinctrl_get:
	gpio_free(cc_corrosion_dev_p->irq_gpio);
err_of_get_named_gpio:
	devm_kfree(&pdev->dev, cc_corrosion_dev_p);
	cc_corrosion_dev_p = NULL;
	return 0;
}

static int hw_cc_anti_corrosion_remove(struct platform_device *pdev)
{
	if (!pdev || !cc_corrosion_dev_p)
	{
		hw_usb_err("%s pdev or cc_corrosion_dev_p is NULL\n", __func__);
		return 0;
	}

	free_irq(cc_corrosion_dev_p->irq, pdev);
	gpio_free(cc_corrosion_dev_p->irq_gpio);
	devm_kfree(&pdev->dev, cc_corrosion_dev_p);
	cc_corrosion_dev_p = NULL;
	return 0;
}

static struct of_device_id hw_cc_anti_corrosion_of_match[] = {
    { .compatible = "huawei,cc-anti-corrosion", },
    { },
};

static struct platform_driver hw_cc_anti_corrosion_drv = {
	.probe		= hw_cc_anti_corrosion_probe,
	.remove     = hw_cc_anti_corrosion_remove,
	.driver		= {
		.owner		= THIS_MODULE,
		.name		= "hw_cc_anti_corrosion",
		.of_match_table	= hw_cc_anti_corrosion_of_match,
	},
};

static int __init hw_cc_anti_corrosion_init(void)
{
    int ret = 0;

    ret = platform_driver_register(&hw_cc_anti_corrosion_drv);
    hw_usb_err("Enter [%s] function, ret is %d\n", __func__, ret);

    return ret;
}

static void __exit hw_cc_anti_corrosion_exit(void)
{
    platform_driver_unregister(&hw_cc_anti_corrosion_drv);
    return ;
}

device_initcall_sync(hw_cc_anti_corrosion_init);
module_exit(hw_cc_anti_corrosion_exit);

MODULE_AUTHOR("huawei");
MODULE_DESCRIPTION("This module anti cc corrosion");
MODULE_LICENSE("GPL v2");

