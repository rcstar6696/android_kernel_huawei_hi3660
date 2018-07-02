#include <linux/module.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/device.h>
#include "hisi_peripheral_tm.h"
#include <linux/power/hisi/coul/hisi_coul_drv.h>

#define CREATE_TRACE_POINTS

#include <trace/events/shell_temp.h>

#define	MUTIPLY_FACTOR		(100000)
#define	DEFAULT_SHELL_TEMP	(25000)
#define	NORMAL_TEMP_DIFF		(10000)

struct hisi_temp_tracing_t {
	int temp;
	int coef;
};

struct hisi_shell_sensor_t {
	u32 id;
	struct hisi_temp_tracing_t temp_tracing[0];
};

struct hisi_shell_t {
	int sensor_count;
	int sample_count;
	u32 interval;
	int bias;
	int temp;
	int index;
	int valid_flag;
	struct thermal_zone_device	*tz_dev;
	struct delayed_work work;
	struct hisi_shell_sensor_t hisi_shell_sensor[0];
};

int ipa_get_periph_id(const char *name);
int ipa_get_periph_value(u32 sensor, int *val);

int hisi_get_shell_temp(struct thermal_zone_device *thermal,
				      int *temp)
{
	struct hisi_shell_t *hisi_shell = thermal->devdata;

	if (!hisi_shell || !temp)
		return -EINVAL;

	*temp = hisi_shell->temp;

	return 0;
}

/*lint -e785*/
struct thermal_zone_device_ops shell_thermal_zone_ops = {
	.get_temp = hisi_get_shell_temp,
};
/*lint +e785*/

static int calc_shell_temp(struct hisi_shell_t *hisi_shell)
{
	int i, j, k;
	struct hisi_shell_sensor_t *shell_sensor;
	long sum = 0;

	for (i = 0; i < hisi_shell->sensor_count; i++) {
		shell_sensor = (struct hisi_shell_sensor_t *)((u64)(hisi_shell->hisi_shell_sensor) + (u64)((long)i) * (sizeof(struct hisi_shell_sensor_t)
						+ sizeof(struct hisi_temp_tracing_t) * (u64)((long)hisi_shell->sample_count)));
		for (j = 0; j < hisi_shell->sample_count; j++) {
			k = (hisi_shell->index - j) <  0 ? ((hisi_shell->index - j) + hisi_shell->sample_count) : hisi_shell->index - j;
			sum += (long)shell_sensor->temp_tracing[j].coef * (long)shell_sensor->temp_tracing[k].temp;
			trace_calc_shell_temp(i, j, shell_sensor->temp_tracing[j].coef, shell_sensor->temp_tracing[k].temp, sum);
		}
	}

	sum += ((long)hisi_shell->bias * 1000L);

	return (int)(sum / MUTIPLY_FACTOR);
}

static void hkadc_handle_temp_data(struct hisi_shell_t *hisi_shell, struct hisi_shell_sensor_t *shell_sensor, int temp)
{
	int old_index, old_temp, diff;

	old_index = (hisi_shell->index - 1) <  0 ? ((hisi_shell->index - 1) + hisi_shell->sample_count) : hisi_shell->index - 1;
	old_temp = shell_sensor->temp_tracing[old_index].temp;
	diff = temp - old_temp;

	if (!hisi_shell->valid_flag && !hisi_shell->index)
		shell_sensor->temp_tracing[hisi_shell->index].temp = temp;
	else {
		if (diff > NORMAL_TEMP_DIFF)
			shell_sensor->temp_tracing[hisi_shell->index].temp = old_temp + NORMAL_TEMP_DIFF;
		else if (diff < -NORMAL_TEMP_DIFF)
			shell_sensor->temp_tracing[hisi_shell->index].temp = old_temp - NORMAL_TEMP_DIFF;
		else
			shell_sensor->temp_tracing[hisi_shell->index].temp = temp;
	}
}

static void hkadc_sample_temp(struct work_struct *work)
{
	int i, index;
	struct hisi_shell_t *hisi_shell;
	struct hisi_shell_sensor_t *shell_sensor;
	int temp;

	hisi_shell = container_of((struct delayed_work *)work, struct hisi_shell_t, work); /*lint !e826*/
	index = hisi_shell->index;
	mod_delayed_work(system_freezable_power_efficient_wq, (struct delayed_work *)work, round_jiffies(msecs_to_jiffies(hisi_shell->interval)));

	for (i = 0; i < hisi_shell->sensor_count ; i++) {
		shell_sensor = (struct hisi_shell_sensor_t *)((u64)(hisi_shell->hisi_shell_sensor) + (u64)((long)i) * (sizeof(struct hisi_shell_sensor_t)
						+ sizeof(struct hisi_temp_tracing_t) * (u64)((long)hisi_shell->sample_count)));

		if (ipa_get_periph_value(shell_sensor->id, &temp))
			shell_sensor->temp_tracing[index].temp = DEFAULT_SHELL_TEMP;
		else
			hkadc_handle_temp_data(hisi_shell, shell_sensor, (int)temp);
	}

	if (!hisi_shell->valid_flag && index >= hisi_shell->sample_count - 1)
		hisi_shell->valid_flag = 1;

	if (hisi_shell->valid_flag)
		hisi_shell->temp = calc_shell_temp(hisi_shell);
	else
		hisi_shell->temp = hisi_battery_temperature() * 1000;

	trace_shell_temp(hisi_shell->temp);

	index++;
	hisi_shell->index = index >= hisi_shell->sample_count ? 0 : index;
}

static int hisi_shell_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *dev_node = dev->of_node;
	int ret;
	int sample_count, sensor_count;
	struct device_node *np;
	struct device_node *child;
	int i = 0, j, coef;
	const char *ptr_coef, *ptr_type;
	struct hisi_shell_sensor_t *shell_sensor;
	struct hisi_shell_t *hisi_shell;

	if (!of_device_is_available(dev_node)) {
		dev_err(dev, "HISI shell dev not found\n");
		return -ENODEV;
	}

	ret = of_property_read_s32(dev_node, "count", &sample_count);
	if (ret) {
		pr_err("%s count read err\n", __func__);
		goto exit;
	}

	np = of_find_node_by_name(dev_node, "sensors");
	if (!np) {
		pr_err("sensors node not found\n");
		ret = -ENODEV;
		goto exit;
	}

	sensor_count = of_get_child_count(np);
	if (sensor_count <= 0) {
		ret = -EINVAL;
		pr_err("%s sensor count read err\n", __func__);
		goto node_put;
	}

	hisi_shell = kzalloc(sizeof(struct hisi_shell_t) + (u64)((long)sensor_count) * (sizeof(struct hisi_shell_sensor_t)
					+ sizeof(struct hisi_temp_tracing_t) * (u64)((long)sample_count)), GFP_KERNEL);
	if (!hisi_shell) {
		ret = -ENOMEM;
		pr_err("no enough memory\n");
		goto node_put;
	}
	hisi_shell->sensor_count = sensor_count;
	hisi_shell->sample_count = sample_count;

	ret = of_property_read_u32(dev_node, "interval", &hisi_shell->interval);
	if (ret) {
		pr_err("%s interval read err\n", __func__);
		goto free_mem;
	}

	ret = of_property_read_s32(dev_node, "bias", &hisi_shell->bias);
	if (ret) {
		pr_err("%s bias read err\n", __func__);
		goto free_mem;
	}

	for_each_child_of_node(np, child) {
		shell_sensor = (struct hisi_shell_sensor_t *)((u64)hisi_shell + sizeof(struct hisi_shell_t) + (u64)((long)i) * (sizeof(struct hisi_shell_sensor_t)
						+ sizeof(struct hisi_temp_tracing_t) * (u64)((long)sample_count)));

		ret = of_property_read_string(child, "type", &ptr_type);
		if (ret) {
			pr_err("%s type read err\n", __func__);
			goto free_mem;
		}

		ret = ipa_get_periph_id(ptr_type);
		if (ret < 0) {
			pr_err("%s sensor id get err\n", __func__);
			goto free_mem;
		}
		shell_sensor->id = (u32)ret;

		for (j = 0; j < hisi_shell->sample_count; j++) {
			ret =  of_property_read_string_index(child, "coef", j, &ptr_coef);
			if (ret) {
				pr_err("%s coef [%d] read err\n", __func__, j);
				goto free_mem;
			}

			ret = kstrtoint(ptr_coef, 10, &coef);
			if (ret) {
				pr_err("%s kstortoint is failed\n", __func__);
				goto free_mem;
			}
			shell_sensor->temp_tracing[j].coef = coef;
		}

		i++;
	}

	hisi_shell->tz_dev = thermal_zone_device_register(dev_node->name,
			0, 0, hisi_shell, &shell_thermal_zone_ops, NULL, 0, 0);
	if (IS_ERR(hisi_shell->tz_dev)) {
		dev_err(dev, "register thermal zone for shell failed.\n");
		ret = -ENODEV;
		goto unregister;
	}

	hisi_shell->temp = hisi_battery_temperature() * 1000;
	pr_info("SHELL: temp %d\n", hisi_shell->temp);
	of_node_put(np);

	platform_set_drvdata(pdev, hisi_shell);

	INIT_DELAYED_WORK(&hisi_shell->work, hkadc_sample_temp); /*lint !e747*/
	mod_delayed_work(system_freezable_power_efficient_wq, &hisi_shell->work, round_jiffies(msecs_to_jiffies(hisi_shell->interval)));

	return 0; /*lint !e429*/

unregister:
	thermal_zone_device_unregister(hisi_shell->tz_dev);
free_mem:
	kfree(hisi_shell);
node_put:
	of_node_put(np);
exit:

	return ret;
}

static int hisi_shell_remove(struct platform_device *pdev)
{
	struct hisi_shell_t *hisi_shell = platform_get_drvdata(pdev);

	if (hisi_shell) {
		platform_set_drvdata(pdev, NULL);
		thermal_zone_device_unregister(hisi_shell->tz_dev);
		kfree(hisi_shell);
	}

	return 0;
}
/*lint -e785*/
static struct of_device_id hisi_shell_of_match[] = {
	{ .compatible = "hisi,shell-temp" },
	{},
};
/*lint +e785*/
MODULE_DEVICE_TABLE(of, hisi_shell_of_match);

int shell_temp_pm_resume(struct platform_device *pdev)
{
	struct hisi_shell_t *hisi_shell;

	pr_info("%s+\n", __func__);
	hisi_shell = platform_get_drvdata(pdev);

	if (hisi_shell) {
		hisi_shell->temp = hisi_battery_temperature() * 1000;
		pr_info("SHELL: temp %d\n", hisi_shell->temp);
		hisi_shell->index = 0;
		hisi_shell->valid_flag = 0;
	}
	pr_info("%s-\n", __func__);

	return 0;
}

/*lint -e64 -e785 -esym(64,785,*)*/
static struct platform_driver hisi_shell_platdrv = {
	.driver = {
		.name		= "hisi-shell-temp",
		.owner		= THIS_MODULE,
		.of_match_table = hisi_shell_of_match,
	},
	.probe	= hisi_shell_probe,
	.remove	= hisi_shell_remove,
	.resume = shell_temp_pm_resume,
};
/*lint -e64 -e785 +esym(64,785,*)*/

static int __init hisi_shell_init(void)
{
	return platform_driver_register(&hisi_shell_platdrv); /*lint !e64*/
}

static void __exit hisi_shell_exit(void)
{
	platform_driver_unregister(&hisi_shell_platdrv);
}
/*lint -e528 -esym(528,*)*/
module_init(hisi_shell_init);
module_exit(hisi_shell_exit);
/*lint -e528 +esym(528,*)*/

/*lint -e753 -esym(753,*)*/
MODULE_LICENSE("GPL v2");
/*lint -e753 +esym(753,*)*/
