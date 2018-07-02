#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/suspend.h>
#include <linux/list.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/clk-provider.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
#include <linux/clk-private.h>
#endif
#include <linux/clkdev.h>

static HLIST_HEAD(clocks_pm);
static DEFINE_MUTEX(clocks_pm_lock);/*lint !e651 !e708 !e570 !e64 !e785 */
void pmclk_monitor_enable(void)
{
	struct clk *pmclk;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
	struct clk *clk;
#else
	struct clk_core *clk;
#endif
	unsigned int ret = 0;
	hlist_for_each_entry(clk, &clocks_pm, child_node) {/*lint !e826 !e62 !e64 */
		pmclk = __clk_lookup(clk->name);
		if (IS_ERR_OR_NULL(pmclk)) {
			pr_err("%s get failed!\n", __clk_get_name(pmclk));
			return;
		}
		if (__clk_get_enable_count(pmclk) > 0) {
			pr_err("The Driver Who use the %s is Exception! cnt=%d Please Check the Drv\n",
					__clk_get_name(pmclk), __clk_get_enable_count(pmclk));
			ret++;
		}
	}
	if (0 < ret) {
		WARN_ON(1);/*lint !e730 */
	}
}

/*static int
clk_pm_callback(struct notifier_block *nb,
			unsigned long action, void *ptr)
{
	switch (action) {
	case PM_SUSPEND_PREPARE:
		break;
	case PM_POST_SUSPEND:
		pmclk_monitor_enable();
		break;
	default:
		return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}*/

static void pmclk_add(struct clk *clk)
{
	mutex_lock(&clocks_pm_lock);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0))
	hlist_add_head(&clk->child_node, &clocks_pm);
#else
	hlist_add_head(&clk->core->child_node, &clocks_pm);
#endif
	mutex_unlock(&clocks_pm_lock);
}

static int hisi_pmclk_monitor_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct clk *clk;
	int ret;
	unsigned int num, i;

	if (!np) {
		pr_err("could not find pmclk node!\n");
		return -EINVAL;
	}
	ret = of_property_read_u32(np, "hisi-pmclk-num", &num);/*lint !e732 */
	if (ret) {
		pr_err("cound not find hisi-pmclk-num property!\n");
		return -EINVAL;
	}
	for (i = 0; i < num; i++) {
		clk = of_clk_get(np, i);/*lint !e713 */
		if (IS_ERR_OR_NULL(clk)) {
			pr_err("%s:Failed to get pmclk\n", __func__);
		}
		pmclk_add(clk);
	}
	/* no longer supported notifier*/
	/* pm_notifier(clk_pm_callback, 0);*/
	pr_err("pm clk monitor setup!\n");

	return ret;
}

static int hisi_pmclk_monitor_remove(struct platform_device *pdev)
{
	return 0;
}/*lint !e715 */

static struct of_device_id hisi_pmclk_of_match[] = {
	{ .compatible = "hisilicon,pm-clk-monitor" },/*lint !e785 */
	{ },/*lint !e785 */
};
MODULE_DEVICE_TABLE(of, hisi_pmclk_of_match);

static struct platform_driver hisi_pmclk_monitor_driver = {
	.probe          = hisi_pmclk_monitor_probe,
	.remove         = hisi_pmclk_monitor_remove,
	.driver         = {
		.name   = "hisi_pmclk",
		.owner  = THIS_MODULE,/*lint !e64 */
		.of_match_table = of_match_ptr(hisi_pmclk_of_match),
	},/*lint !e785 */
};/*lint !e785 */

static int __init hisi_pmclk_monitor_init(void)
{
	return platform_driver_register(&hisi_pmclk_monitor_driver);/*lint !e64 */
}
/*lint -e528 -esym(528,*) */
fs_initcall(hisi_pmclk_monitor_init);
/*lint -e528 +esym(528,*) */