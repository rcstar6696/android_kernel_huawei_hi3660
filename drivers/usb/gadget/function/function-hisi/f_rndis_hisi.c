
#include "u_ether.h"

atomic_t hisi_uether_enable_flag = ATOMIC_INIT(0);
extern struct device *create_function_device(char *name);
static inline struct f_rndis_opts *
f_rndis_opts_from_func_inst(const struct usb_function_instance *fi)
{
	return container_of(fi, struct f_rndis_opts, func_inst);
}

static u8 host_ethaddr_record[ETH_ALEN];
static char manufacturer[256];
static bool wceis;

static ssize_t rndis_manufacturer_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", manufacturer);
}

static ssize_t rndis_manufacturer_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int len;

	len = min(size, sizeof(manufacturer) - 1);

	strncpy(manufacturer, buf, len);

	manufacturer[len] = 0;

	return size;
}

static DEVICE_ATTR(manufacturer, (S_IRUGO | S_IWUSR), rndis_manufacturer_show,
						    rndis_manufacturer_store);

static ssize_t rndis_wceis_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", wceis);
}

static ssize_t rndis_wceis_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	u32 value;
	int ret;

	ret = kstrtou32(buf, 0, &value);
	if (ret)
		return ret;

	wceis = !!value;
	return size;
}

static DEVICE_ATTR(wceis, (S_IRUGO | S_IWUSR), rndis_wceis_show,
					     rndis_wceis_store);

static ssize_t rndis_ethaddr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%02x:%02x:%02x:%02x:%02x:%02x\n",
		host_ethaddr_record[0], host_ethaddr_record[1],
		host_ethaddr_record[2], host_ethaddr_record[3],
		host_ethaddr_record[4], host_ethaddr_record[5]);
}

static ssize_t rndis_ethaddr_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int set_etheraddr[ETH_ALEN];
	int i;

	if (sscanf(buf, "%02x:%02x:%02x:%02x:%02x:%02x\n",
		    &set_etheraddr[0], &set_etheraddr[1],
		    &set_etheraddr[2], &set_etheraddr[3],
		    &set_etheraddr[4], &set_etheraddr[5]) == 6) {
		for (i = 0; i < ETH_ALEN; i++)
			host_ethaddr_record[i] = (u8)set_etheraddr[i];

		return size;
	}

	return -EINVAL;
}

static DEVICE_ATTR(ethaddr, (S_IRUGO | S_IWUSR), rndis_ethaddr_show,
					       rndis_ethaddr_store);

static ssize_t rndis_vendorID_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct usb_function_instance *fi = dev_get_drvdata(dev);
	struct f_rndis_opts *opts = f_rndis_opts_from_func_inst(fi);

	return snprintf(buf, PAGE_SIZE, "%04x\n", opts->vendor_id);
}

static ssize_t rndis_vendorID_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct usb_function_instance *fi = dev_get_drvdata(dev);
	struct f_rndis_opts *opts = f_rndis_opts_from_func_inst(fi);
	unsigned int value;

	if (kstrtou32(buf, 0, &value)) {
		pr_err("[%s]read vendorID from input failed\n", __func__);
		return -EINVAL;
	}

	opts->vendor_id = value;
	return size;
}

static DEVICE_ATTR(vendorID, (S_IRUGO | S_IWUSR), rndis_vendorID_show,
						rndis_vendorID_store);

static struct device_attribute *rndis_function_attributes[] = {
	&dev_attr_manufacturer,
	&dev_attr_wceis,
	&dev_attr_ethaddr,
	&dev_attr_vendorID,
	NULL
};

static int create_rndis_device(struct usb_function_instance *fi)
{
	struct device *dev;
	struct device_attribute **attrs;
	struct device_attribute *attr;
	int err = 0;

	dev = create_function_device("f_rndis");
	if (IS_ERR(dev))
		return PTR_ERR(dev);

	attrs = rndis_function_attributes;
	if (attrs) {
		while ((attr = *attrs++) && !err)
			err = device_create_file(dev, attr);
		if (err) {
			device_destroy(dev->class, dev->devt);
			return -EINVAL;
		}
	}
	dev_set_drvdata(dev, fi);
	return 0;
}

static void rndis_free_inst(struct usb_function_instance *f)
{
	struct f_rndis_opts *opts;

	opts = container_of(f, struct f_rndis_opts, func_inst);

	kfree(opts->rndis_os_desc.group.default_groups); /* single VLA chunk */
	kfree(opts);
}

static struct usb_function_instance *rndis_alloc_inst(void)
{
	struct f_rndis_opts *opts;
	struct usb_os_desc *descs[1];
	char *names[1];
	int rc;

	opts = kzalloc(sizeof(*opts), GFP_KERNEL);
	if (!opts)
		return ERR_PTR(-ENOMEM);
	opts->rndis_os_desc.ext_compat_id = opts->rndis_ext_compat_id;

	mutex_init(&opts->lock);
	opts->func_inst.free_func_inst = rndis_free_inst;
	opts->manufacturer = manufacturer;

	rc = create_rndis_device(&opts->func_inst);
	if (rc) {
		pr_err("%s: create rndis device failed\n", __func__);
		goto err_gether_setup;
	}

	INIT_LIST_HEAD(&opts->rndis_os_desc.ext_prop);

	descs[0] = &opts->rndis_os_desc;
	names[0] = "rndis";
	usb_os_desc_prepare_interf_dir(&opts->func_inst.group, 1, descs,
				       names, THIS_MODULE);
	config_group_init_type_name(&opts->func_inst.group, "",
				    &rndis_func_type);

	return &opts->func_inst;

err_gether_setup:
	kfree(opts);

	return ERR_PTR(rc);
}

static int hisi_rndis_create_net(struct f_rndis_opts *opts,
		struct usb_gadget *g)
{
#define HOST_ADD_LEN (18)
	char addr[HOST_ADD_LEN];
	char *host_addr = NULL;
	if (!opts)
		return -EINVAL;
	if (get_ether_addr_str(host_ethaddr_record, addr, HOST_ADD_LEN) > 0) {
		pr_info("use host_ethaddr_record %s\n", addr);
		host_addr = addr;
	}

	opts->net = gether_setup_name(g, NULL, host_addr,
			host_ethaddr_record, 10, "rndis");
	if (IS_ERR(opts->net)) {
		pr_err("[%s]: gether setup failed\n", __func__);
		return PTR_ERR(opts->net);
	}

	opts->bound = true;

	if (wceis) {
		/* "Wireless" RNDIS; auto-detected by Windows */
		rndis_iad_descriptor.bFunctionClass =
						USB_CLASS_WIRELESS_CONTROLLER;
		rndis_iad_descriptor.bFunctionSubClass = 0x01;
		rndis_iad_descriptor.bFunctionProtocol = 0x03;
		rndis_control_intf.bInterfaceClass =
						USB_CLASS_WIRELESS_CONTROLLER;
		rndis_control_intf.bInterfaceSubClass =	 0x01;
		rndis_control_intf.bInterfaceProtocol =	 0x03;
	}

	return 0;
}

static void hisi_rndis_free_net(struct f_rndis_opts *opts)
{
	if (!opts)
		return;

	if (opts->bound)
		gether_cleanup(netdev_priv(opts->net));
	else
		free_netdev(opts->net);

	opts->bound = false;
}

void hisi_uether_enable_set(int n)
{
	atomic_set(&hisi_uether_enable_flag, n);
}

int hisi_uether_enable_get(void)
{
	return atomic_read(&hisi_uether_enable_flag);
}
