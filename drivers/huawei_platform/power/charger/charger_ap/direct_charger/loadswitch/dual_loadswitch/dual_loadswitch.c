#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <huawei_platform/log/hw_log.h>
#include <dual_loadswitch.h>

#define HWLOG_TAG dual_loadswitch
HWLOG_REGIST();

#define IF_OPS(ops, op)  if(ops && ops->op)

static struct dual_loadswitch_info *dli;
static struct loadswitch_ops *g_main_ops;
static struct loadswitch_ops *g_aux_ops;


/**********************************************************
*  Function:     loadswitch_main_ops_register
*  Discription:  register loadswitch operation, this function
*                should be called by main loadswitch driver
*  Parameters:   ops
*  return value:  0-sucess or others-fail
**********************************************************/
int loadswitch_main_ops_register(struct loadswitch_ops *ops)
{
	int ret = 0;

	if (ops != NULL)
		g_main_ops = ops;
	else
		ret = -EPERM;

	return ret;
}

/**********************************************************
*  Function:     loadswitch_aux_ops_register
*  Discription:  register loadswitch operation, this function
*                should be called by aux loadswitch driver
*  Parameters:   ops
*  return value:  0-sucess or others-fail
**********************************************************/
int loadswitch_aux_ops_register(struct loadswitch_ops *ops)
{
	int ret = 0;

	if (ops != NULL)
		g_aux_ops = ops;
	else
		ret = -EPERM;

	return ret;
}

/**********************************************************
*  Function:       dual_loadswitch_init
*  Discription:    dual loadswitch initialization
*  Parameters:   NULL
*  return value:  0-sucess or others-fail
**********************************************************/
static int dual_loadswitch_charge_init(void)
{
	int ret = 0;
	IF_OPS(g_main_ops, ls_init)
		ret |= g_main_ops->ls_init();
	IF_OPS(g_aux_ops, ls_init)
		ret |= g_aux_ops->ls_init();
	return ret;
}

/**********************************************************
*  Function:       dual_loadswitch_exit
*  Discription:    dual loadswitch exit
*  Parameters:   NULL
*  return value:  0-sucess or others-fail
**********************************************************/
static int dual_loadswitch_charge_exit(void)
{
	int ret = 0;
	IF_OPS(g_main_ops, ls_exit)
		ret |= g_main_ops->ls_exit();
	IF_OPS(g_aux_ops, ls_exit)
		ret |= g_aux_ops->ls_exit();
	return ret;
}
/**********************************************************
*  Function:       dual_loadswitch_enable
*  Discription:    dual loadswitch enable
*  Parameters:   enable
*  return value:  0-sucess or others-fail
**********************************************************/
static int dual_loadswitch_enable(int enable)
{
	int ret = 0;
	IF_OPS(g_main_ops, ls_enable)
		ret |= g_main_ops->ls_enable(enable);
	IF_OPS(g_aux_ops, ls_enable)
		ret |= g_aux_ops->ls_enable(enable);
	return ret;
}

/**********************************************************
*  Function:       dual_loadswitch_discharge
*  Discription:    dual loadswitch discharge
*  Parameters:   enable
*  return value:  0-sucess or others-fail
**********************************************************/
static int dual_loadswitch_discharge(int enable)
{
	int ret = 0;
	IF_OPS(g_main_ops, ls_discharge)
		ret |= g_main_ops->ls_discharge(enable);
	return ret;
}
/**********************************************************
*  Function:       dual_loadswitch_is_ls_close
*  Discription:    dual loadswitch is ls close
*  Parameters:   enable
*  return value:  0-open or others-close
**********************************************************/
static int dual_loadswitch_is_ls_close(void)
{
	int ret = 0;
	IF_OPS(g_main_ops, is_ls_close)
		ret |= g_main_ops->is_ls_close();
	IF_OPS(g_aux_ops, is_ls_close)
		ret |= g_aux_ops->is_ls_close();
	return ret;
}
/**********************************************************
*  Function:       dual_loadswitch_get_ls_id
*  Discription:    dual loadswitch get_ls_id
*  Parameters: NULL
*  return value:  device id
**********************************************************/
static int dual_loadswitch_get_ls_id(void)
{
	int ret = -1;
	IF_OPS(g_main_ops, get_ls_id)
		ret = g_main_ops->get_ls_id();
	return ret;
}

struct loadswitch_ops dual_loadswitch_ops = {
	.ls_init = dual_loadswitch_charge_init,
	.ls_exit = dual_loadswitch_charge_exit,
	.ls_enable = dual_loadswitch_enable,
	.ls_discharge = dual_loadswitch_discharge,
	.is_ls_close = dual_loadswitch_is_ls_close,
	.get_ls_id = dual_loadswitch_get_ls_id,
};

#ifdef CONFIG_SYSFS
#define DUAL_LOADSWITCH_SYSFS_FIELD(_name, n, m, store)                \
{                                                   \
    .attr = __ATTR(_name, m, dual_loadswitch_sysfs_show, store),    \
    .name = DUAL_LOADSWITCH_SYSFS_##n,          \
}

#define DUAL_LOADSWITCH_SYSFS_FIELD_RW(_name, n)               \
	DUAL_LOADSWITCH_SYSFS_FIELD(_name, n, S_IWUSR | S_IRUGO,       \
		dual_loadswitch_sysfs_store)

#define DUAL_LOADSWITCH_SYSFS_FIELD_RO(_name, n)               \
	DUAL_LOADSWITCH_SYSFS_FIELD(_name, n, S_IRUGO, NULL)

static ssize_t dual_loadswitch_sysfs_show(struct device *dev,
				       struct device_attribute *attr,
				       char *buf);
static ssize_t dual_loadswitch_sysfs_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count);

struct dual_loadswitch_sysfs_field_info {
	struct device_attribute attr;
	u8 name;
};

static struct dual_loadswitch_sysfs_field_info dual_loadswitch_sysfs_field_tbl[] = {
	DUAL_LOADSWITCH_SYSFS_FIELD_RW(enable_loadswitch_main, ENABLE_MAIN),
	DUAL_LOADSWITCH_SYSFS_FIELD_RW(enable_loadswitch_aux, ENABLE_AUX),
};

static struct attribute *dual_loadswitch_sysfs_attrs[ARRAY_SIZE(dual_loadswitch_sysfs_field_tbl) + 1];

static const struct attribute_group dual_loadswitch_sysfs_attr_group = {
	.attrs = dual_loadswitch_sysfs_attrs,
};

static void dual_loadswitch_sysfs_init_attrs(void)
{
	int i, limit = ARRAY_SIZE(dual_loadswitch_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		dual_loadswitch_sysfs_attrs[i] =
		    &dual_loadswitch_sysfs_field_tbl[i].attr.attr;
	}
	dual_loadswitch_sysfs_attrs[limit] = NULL;
}

static struct dual_loadswitch_sysfs_field_info
*dual_loadswitch_sysfs_field_lookup(const char *name)
{
	int i, limit = ARRAY_SIZE(dual_loadswitch_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		if (!strncmp(name, dual_loadswitch_sysfs_field_tbl[i].attr.attr.name, strlen(name)))
			break;
	}
	if (i >= limit)
		return NULL;

	return &dual_loadswitch_sysfs_field_tbl[i];
}

static ssize_t dual_loadswitch_sysfs_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct dual_loadswitch_sysfs_field_info *info = NULL;

	info = dual_loadswitch_sysfs_field_lookup(attr->attr.name);
	if (!info)
		return -EINVAL;

	switch (info->name) {
	case DUAL_LOADSWITCH_SYSFS_ENABLE_MAIN:
		return snprintf(buf, PAGE_SIZE, "%d\n",	dli->loadswitch_main_enable_sysfs);
	case DUAL_LOADSWITCH_SYSFS_ENABLE_AUX:
		return snprintf(buf, PAGE_SIZE, "%d\n",	dli->loadswitch_aux_enable_sysfs);
	}
	return 0;
}

static ssize_t dual_loadswitch_sysfs_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct dual_loadswitch_sysfs_field_info *info = NULL;
	long val = 0;

	info = dual_loadswitch_sysfs_field_lookup(attr->attr.name);
	if (!info)
		return -EINVAL;

	switch (info->name) {
	case DUAL_LOADSWITCH_SYSFS_ENABLE_MAIN:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
			return -EINVAL;
		dli->loadswitch_main_enable_sysfs = val;
		hwlog_info("set loadswitch main enable = %d\n", dli->loadswitch_main_enable_sysfs);

		g_main_ops->ls_enable(dli->loadswitch_main_enable_sysfs);
		break;
	case DUAL_LOADSWITCH_SYSFS_ENABLE_AUX:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
			return -EINVAL;
		dli->loadswitch_aux_enable_sysfs = val;
		hwlog_info("set loadswitch aux enable = %d\n", dli->loadswitch_aux_enable_sysfs);

		g_aux_ops->ls_enable(dli->loadswitch_aux_enable_sysfs);
		break;
	}

	return count;
}

static int dual_loadswitch_sysfs_create_group(void)
{
	dual_loadswitch_sysfs_init_attrs();
	return sysfs_create_group(&dli->dev->kobj, &dual_loadswitch_sysfs_attr_group);
}

static inline void dual_loadswitch_sysfs_remove_group(void)
{
	sysfs_remove_group(&dli->dev->kobj, &dual_loadswitch_sysfs_attr_group);
}
#else

/*lint -save -e* */
static int loadswitch_sysfs_create_group(void)
{
	return 0;
}

static inline void loadswitch_sysfs_remove_group(void)
{
}
/*lint -restore*/

#endif
/**********************************************************
*  Function:       dual_loadswitch_parse_dts
*  Discription:    parse the module dts config value
*  Parameters:   null
*  return value:  0-sucess or others-fail
**********************************************************/
static int dual_loadswitch_parse_dts(void)
{
	int ret = 0;

	return ret;
}

/**********************************************************
*  Function:       dual_loadswitch_probe
*  Discription:    dual_loadswitch module probe
*  Parameters:   device:platform_device
*  return value:  0-sucess or others-fail
**********************************************************/

/*lint -save -e* */
static int dual_loadswitch_probe(struct platform_device *pdev)
{
	int ret;

	dli = kzalloc(sizeof(*dli), GFP_KERNEL);
	if (NULL == dli)
	{
		hwlog_err("alloc dual_loadswitch_info failed\n");
		ret = -ENOMEM;
		return ret;
	}
	dli->dev = &pdev->dev;

	dual_loadswitch_parse_dts();

	ret = loadswitch_ops_register(&dual_loadswitch_ops);
	if (ret)
	{
		hwlog_err("register dual loadswitch ops failed!\n");
		goto dual_loadswitch_fail_0;
	}

	if ((NULL == g_main_ops) || (NULL == g_main_ops->ls_init)
	    || (NULL == g_main_ops->ls_enable) || (NULL == g_main_ops->ls_exit))
	{
		hwlog_err("main loadswitch ops is NULL!\n");
		ret = -EINVAL;
		goto dual_loadswitch_fail_1;
	}
	if ((NULL == g_aux_ops) || (NULL == g_aux_ops->ls_init)
	    || (NULL == g_aux_ops->ls_enable) || (NULL == g_main_ops->ls_exit))
	{
		hwlog_err("aux loadswitch ops is NULL!\n");
		ret = -EINVAL;
		goto dual_loadswitch_fail_2;
	}
	ret = dual_loadswitch_sysfs_create_group();
	if (ret)
		hwlog_err("can't create dual loadswitch sysfs entries\n");

	hwlog_info("dual loadswitch probe ok!\n");
	return ret;

dual_loadswitch_fail_2:
	g_aux_ops = NULL;
dual_loadswitch_fail_1:
	g_main_ops = NULL;
dual_loadswitch_fail_0:
	kfree(dli);
	dli = NULL;
	return ret;
}
/*lint -restore*/

static int dual_loadswitch_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_LLT_TEST
#endif

/*lint -save -e* */
static struct of_device_id dual_loadswitch_match_table[] = {
	{
	 .compatible = "dual_loadswitch",
	 .data = NULL,
	 },
	{
	 },
};
/*lint -restore*/

static struct platform_driver dual_loadswitch_driver = {
	.probe = dual_loadswitch_probe,
	.remove = dual_loadswitch_remove,
	.driver = {
		   .name = "dual_loadswitch",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(dual_loadswitch_match_table),
		   },
};

/**********************************************************
*  Function:       dual_loadswitch_init
*  Discription:    dual_loadswitch initialization
*  Parameters:   pdev:platform_device
*  return value:  0-sucess or others-fail
**********************************************************/
int __init dual_loadswitch_init(void)
{
	return platform_driver_register(&dual_loadswitch_driver);
}

/**********************************************************
*  Function:       dual_loadswitch_exit
*  Discription:    dual_loadswitch exit
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
void __exit dual_loadswitch_exit(void)
{
	platform_driver_unregister(&dual_loadswitch_driver);
}

/*lint -save -e* */
device_initcall_sync(dual_loadswitch_init);
module_exit(dual_loadswitch_exit);
/*lint -restore*/

MODULE_AUTHOR("HUAWEI");
MODULE_DESCRIPTION("dual_loadswitch module driver");
MODULE_LICENSE("GPL");
