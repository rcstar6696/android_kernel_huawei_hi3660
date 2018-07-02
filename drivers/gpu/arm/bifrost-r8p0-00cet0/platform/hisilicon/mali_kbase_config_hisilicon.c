/*
 *
 * (C) COPYRIGHT ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */





#include <linux/ioport.h>
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_config.h>
#ifdef CONFIG_DEVFREQ_THERMAL
#include <linux/devfreq_cooling.h>
#endif

#include <trace/events/power.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
#include <linux/pm_opp.h>
#else
#include <linux/opp.h>
#endif

#include "mali_kbase_hisi_callback.h"

#ifdef CONFIG_PM_DEVFREQ
#include <linux/hisi/hisi_devfreq.h>
#endif

#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_REPORT_VSYNC
#include <linux/export.h>
#endif
#include <linux/delay.h>
#include <linux/of_address.h>
#include <backend/gpu/mali_kbase_pm_internal.h>
#include "mali_kbase_config_platform.h"
#include "mali_kbase_config_hifeatures.h"
#ifdef CONFIG_HISI_IPA_THERMAL
#include <linux/thermal.h>
#endif

#define MALI_TRUE ((uint32_t)1)
#define MALI_FALSE ((uint32_t)0)
typedef uint32_t     mali_bool;

typedef enum {
        MALI_ERROR_NONE = 0,
        MALI_ERROR_OUT_OF_GPU_MEMORY,
        MALI_ERROR_OUT_OF_MEMORY,
        MALI_ERROR_FUNCTION_FAILED,
}mali_error;

#define HARD_RESET_AT_POWER_OFF 0

#ifndef CONFIG_OF
static struct kbase_io_resources io_resources = {
	.job_irq_number = 68,
	.mmu_irq_number = 69,
	.gpu_irq_number = 70,
	.io_memory_region = {
	.start = 0xFC010000,
	.end = 0xFC010000 + (4096 * 4) - 1
	}
};
#endif /* CONFIG_OF */


#define DEFAULT_POLLING_MS        20


#define RUNTIME_PM_DELAY_1MS      1
#define RUNTIME_PM_DELAY_30MS    30

#ifdef CONFIG_REPORT_VSYNC
static struct kbase_device *kbase_dev = NULL;
#endif


#ifdef CONFIG_HISI_HW_VOTE_GPU_FREQ

#define FREQ_VOTE_RATIO		(16U)
#define MHz		(1000000)


enum hw_vote_ret_type {
	REG_PMCTRL_TYPE,
};

struct hw_vote_gpu_reg_info {
	unsigned int reg_type;
	unsigned int vote_reg_offset;
	unsigned int vote_bits_mask;
	unsigned int vote_wr_mask;
	unsigned int result_reg_offset;
	unsigned int result_rd_mask;
};

static struct hw_vote_gpu_reg_info gpu_vote_reg;
static void __iomem *pmctrl_base = NULL;


static int
hisi_hw_vote_gpu_regs_remap(void)
{
	struct device_node *np;
	int elem_num;
	int ret;

	np = of_find_compatible_node(NULL, NULL, "hisilicon,pmctrl");
	if (!np) {
		pr_err("[%s] doesn't have pmctrl node!\n", __func__);
		return -ENODEV;
	}

	pmctrl_base = of_iomap(np, 0);
	if (NULL == pmctrl_base) {
		pr_err("[%s]: ioremap fail!\n", __func__);
		return -EINVAL;
	}

	np = of_find_compatible_node(NULL, NULL, "hisi,gpu-hw-vote-freq");
	if (!np) {
		pr_err("[%s] doesn't have hw-vote-freq node!\n", __func__);
		return -ENODEV;
	}

	elem_num = of_property_count_elems_of_size(np, "vote_reg_info", sizeof(unsigned int));
	if (elem_num != (sizeof(struct hw_vote_gpu_reg_info) / 4)) {
		pr_err("[%s]vote_reg_info & struct un-match!\n", __func__);
		return -ENODEV;
	}

	ret = of_property_read_u32_array(np, "vote_reg_info", (unsigned int*)(&gpu_vote_reg), sizeof(struct hw_vote_gpu_reg_info) / 4);
	if (ret) {
		pr_err("[%s]parse vote_reg_info fail!\n", __func__);
		return -ENODEV;
	}

	return 0;
}

static int
hisi_hw_vote_gpu_target(unsigned long freq)
{
	unsigned int freq_cfg;
	void __iomem *reg;
	int shift, width;
	unsigned int mask;


	if (0 == gpu_vote_reg.vote_reg_offset) {
		pr_err("%s: not remap!\n", __func__);
		return -EINVAL;
	}

	freq_cfg = (freq / MHz) / FREQ_VOTE_RATIO;

	if (REG_PMCTRL_TYPE == gpu_vote_reg.reg_type) {
		reg   = pmctrl_base + gpu_vote_reg.vote_reg_offset;
		shift = ffs(gpu_vote_reg.vote_bits_mask);
		if (shift <= 0) {
			pr_err("%s: gpu result reg mask error\n", __func__);
			return -EINVAL;
		}
		shift -= 1;
		width = fls(gpu_vote_reg.vote_bits_mask) - shift;
		mask  = (1 << width) - 1;
		writel(gpu_vote_reg.vote_wr_mask | ((freq_cfg & mask) << shift), reg);
	}

	return 0;
}

static unsigned long
hisi_hw_vote_gpu_result_get(struct device *dev)
{
	unsigned long target_freq = 0;
	struct dev_pm_opp *opp;
	void __iomem *reg;
	unsigned int shift;

	if (NULL == dev) {
		pr_err("%s: dev is NULL!\n", __func__);
		return target_freq;
	}

	if (0 == gpu_vote_reg.result_reg_offset) {
		pr_err("%s: not remap!\n", __func__);
		return target_freq;
	}

	if (REG_PMCTRL_TYPE == gpu_vote_reg.reg_type) {
		reg   = pmctrl_base + gpu_vote_reg.result_reg_offset;
		shift = ffs(gpu_vote_reg.result_rd_mask);
		if (shift <= 0) {
			pr_err("%s: gpu result reg mask error\n", __func__);
			return 0;
		}
		shift -= 1;
		target_freq = (readl(reg) & gpu_vote_reg.result_rd_mask) >> shift;
	}

	target_freq = target_freq * MHz * FREQ_VOTE_RATIO;

	opp = dev_pm_opp_find_freq_ceil(dev, &target_freq);
	if (IS_ERR(opp)) {
		target_freq = 0;
	}

	return target_freq;
}

#endif

static int kbase_set_hi_features_mask(struct kbase_device *kbdev)
{
	const enum kbase_hi_feature *hi_features;
	u32 gpu_vid;
	u32 product_id;

	gpu_vid = kbdev->gpu_vid;
	product_id = gpu_vid & GPU_ID_VERSION_PRODUCT_ID;
	product_id >>= GPU_ID_VERSION_PRODUCT_ID_SHIFT;

	if (GPU_ID_IS_NEW_FORMAT(product_id)) {
		switch (gpu_vid) {
		case GPU_ID2_MAKE(6, 0, 10, 0, 0, 0, 2):
			hi_features = kbase_hi_feature_tMIx_r0p0;
			break;
		case GPU_ID2_MAKE(6, 2, 2, 1, 0, 0, 0):
			hi_features = kbase_hi_feature_tHEx_r0p0;
			break;
		case GPU_ID2_MAKE(6, 2, 2, 1, 0, 0, 1):
			hi_features = kbase_hi_feature_tHEx_r0p0;
			break;
		case GPU_ID2_MAKE(7, 0, 9, 0, 1, 0, 0):
			hi_features = kbase_hi_feature_tSIx_r1p0;
			break;
		default:
			dev_err(kbdev->dev,
				"[hi-feature]Unknown GPU ID %x", gpu_vid);
			return -EINVAL;
		}
	} else {
		switch (gpu_vid) {
		case GPU_ID_MAKE(GPU_ID_PI_TFRX, 0, 2, 0):
			hi_features = kbase_hi_feature_t880_r0p2;
			break;
		case GPU_ID_MAKE(GPU_ID_PI_T83X, 1, 0, 0):
			hi_features = kbase_hi_feature_t830_r2p0;
			break;
		case GPU_ID_MAKE(GPU_ID_PI_TFRX, 2, 0, 0):
			hi_features = kbase_hi_feature_t880_r2p0;
			break;
		default:
			dev_err(kbdev->dev,
				"[hi-feature]Unknown GPU ID %x", gpu_vid);
			return -EINVAL;
		}
	}

	dev_info(kbdev->dev, "[hi-feature]GPU identified as 0x%04x r%dp%d status %d",
		(gpu_vid & GPU_ID_VERSION_PRODUCT_ID) >> GPU_ID_VERSION_PRODUCT_ID_SHIFT,
		(gpu_vid & GPU_ID_VERSION_MAJOR) >> GPU_ID_VERSION_MAJOR_SHIFT,
		(gpu_vid & GPU_ID_VERSION_MINOR) >> GPU_ID_VERSION_MINOR_SHIFT,
		(gpu_vid & GPU_ID_VERSION_STATUS) >> GPU_ID_VERSION_STATUS_SHIFT);

	for (; *hi_features != KBASE_HI_FEATURE_END; hi_features++)
		set_bit(*hi_features, &kbdev->hi_features_mask[0]);

	return 0;
}

static inline void kbase_platform_on(struct kbase_device *kbdev)
{
	if (kbdev->regulator) {
		if (unlikely(regulator_enable(kbdev->regulator))) {
			dev_err(kbdev->dev, "Failed to enable regulator\n");
			BUG_ON(1);
		}

		if (kbdev->gpu_vid == 0) {
			kbdev->gpu_vid = kbase_os_reg_read(kbdev, GPU_CONTROL_REG(GPU_ID));

			/*just for Sigurd r1p1*/
			if(kbdev->gpu_vid == 0x70901010)
			{
				dev_err(kbdev->dev, "the Real gpu id is:0x%x\n", kbdev->gpu_vid);
				kbdev->gpu_vid = 0x70901000;
			}
			if (unlikely(kbase_set_hi_features_mask(kbdev))) {
				dev_err(kbdev->dev, "Failed to set hi features\n");
			}
		}

		if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_HI0004)) {
			kbase_os_reg_write(kbdev, GPU_CONTROL_REG(PWR_KEY), KBASE_PWR_KEY_VALUE);
			kbase_os_reg_write(kbdev, GPU_CONTROL_REG(PWR_OVERRIDE1), KBASE_PWR_OVERRIDE_VALUE);
		}

		if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_HI0003)) {
			int value = 0;
			value = readl(kbdev->pctrlreg + PERI_CTRL19) & GPU_X2P_GATOR_BYPASS;
			writel(value, kbdev->pctrlreg + PERI_CTRL19);
		}
	}
}

static inline void kbase_platform_off(struct kbase_device *kbdev)
{

	if (kbdev->regulator) {
		if (unlikely(regulator_disable(kbdev->regulator))) {
			dev_err(kbdev->dev, "MALI-MIDGARD: Failed to disable regulator\n");
		}
	}

}

#ifdef CONFIG_PM_DEVFREQ
static struct hisi_devfreq_data hisi_devfreq_priv_data = {
	.vsync_hit = 0,
	.cl_boost = 0,
};

static int mali_kbase_devfreq_target(struct device *dev, unsigned long *_freq,
			      u32 flags)
{
	struct kbase_device *kbdev = (struct kbase_device *)dev->platform_data;
	unsigned long old_freq = kbdev->devfreq->previous_freq;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
	struct dev_pm_opp *opp = NULL;
#else
	struct opp *opp = NULL;
#endif
	unsigned long freq;

	rcu_read_lock();
	opp = devfreq_recommended_opp(dev, _freq, flags);
	if (IS_ERR(opp)) {
		pr_err("[mali]  Failed to get Operating Performance Point\n");
		rcu_read_unlock();
		return PTR_ERR(opp);
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
	freq = dev_pm_opp_get_freq(opp);
#else
	freq = opp_get_freq(opp);
#endif
	rcu_read_unlock();

#ifdef CONFIG_HISI_IPA_THERMAL
	freq = ipa_freq_limit(IPA_GPU,freq);
#endif

	if (old_freq == freq)
		goto update_target;

	trace_clock_set_rate("clk-g3d",freq,raw_smp_processor_id());

#ifdef CONFIG_HISI_HW_VOTE_GPU_FREQ
	if (hisi_hw_vote_gpu_target(freq)) {
#else
	if (clk_set_rate((kbdev->clk), freq)) {
#endif
		pr_err("[mali]  Failed to set gpu freqency, [%lu->%lu]\n", old_freq, freq);
		return -ENODEV;
	}

update_target:
	*_freq = freq;

	return 0;
}

#ifdef CONFIG_MALI_BOUND_REPORT
static bool mali_kbase_bound_report(struct kbase_device *kbdev,bool bound_event)
{
	bool out_data, bound_it = false;
	if(NULL == kbdev)
		return false;

	if(bound_event)
		kbdev->bound_report_info.bound_times_in_fifo ++;
	if(kfifo_is_full(&kbdev->bound_report_info.bound_fifo)){
		kfifo_out(&kbdev->bound_report_info.bound_fifo,&out_data,1);
		if(out_data)
			kbdev->bound_report_info.bound_times_in_fifo --;
	}
	kfifo_in(&kbdev->bound_report_info.bound_fifo,&bound_event,1);

	if(kbdev->bound_report_info.bound_times_in_fifo > 3)
		bound_it = true;	//bound detected!!!

	-- kbdev->bound_report_info.duration_times;
	if(bound_it != kbdev->bound_report_info.report_bound_flag){		//new bound state is changed
		if(kbdev->bound_report_info.duration_times <= 0){			//check the period, make sure the previous flag state last at least for a while(BOUND_DURATION_TIMES*20 ms)
			kbdev->bound_report_info.report_bound_flag = bound_it;
			kbdev->bound_report_info.duration_times = BOUND_DURATION_TIMES;
		}
	}

	return kbdev->bound_report_info.report_bound_flag;
}
#endif

#ifdef CONFIG_DEVFREQ_THERMAL
void mali_kbase_devfreq_detect_bound_worker(struct work_struct *work)
{
	int err;
	struct kbase_device *kbdev = container_of(work,
					struct kbase_device, bound_detect_work);

	bool bound_event = false;
	struct thermal_cooling_device *cdev = kbdev->devfreq_cooling;

#if defined(CONFIG_MALI_MIDGARD_DVFS)
	bound_event = kbase_ipa_dynamic_bound_detect(kbdev->ipa_ctx, &err, kbdev->bound_detect_freq, kbdev->bound_detect_btime, cdev->ipa_enabled);
#endif

	cdev->ipa_enabled = false;

	cdev->bound_event = bound_event;

#ifdef CONFIG_MALI_BOUND_REPORT
	kbdev->bound_report_info.report_bound_flag = mali_kbase_bound_report(kbdev,bound_event);
#endif

}

void mali_kbase_devfreq_detect_bound(struct kbase_device *kbdev,
		unsigned long cur_freq,
		unsigned long btime)
{
	kbdev->bound_detect_freq = cur_freq;
	kbdev->bound_detect_btime = btime;
	queue_work(system_unbound_wq, &kbdev->bound_detect_work);
}
#endif

static int mali_kbase_get_dev_status(struct device *dev,
				      struct devfreq_dev_status *stat)
{
	struct hisi_devfreq_data *priv_data = &hisi_devfreq_priv_data;
	struct kbase_device *kbdev = (struct kbase_device *)dev->platform_data;

	if (kbdev->pm.backend.metrics.kbdev != kbdev) {
		pr_err("%s pm backend metrics not initialized\n", __func__);
		return -EINVAL;
	}

	(void)kbase_pm_get_dvfs_action(kbdev);
	stat->busy_time = kbdev->pm.backend.metrics.utilisation;
	stat->total_time = 100;
#ifdef CONFIG_HISI_HW_VOTE_GPU_FREQ
	stat->current_frequency = hisi_hw_vote_gpu_result_get(dev);
#else
	stat->current_frequency = clk_get_rate(kbdev->clk);
#endif
	priv_data->vsync_hit = kbdev->pm.backend.metrics.vsync_hit;
	priv_data->cl_boost = kbdev->pm.backend.metrics.cl_boost;
	stat->private_data = (void *)priv_data;

#ifdef CONFIG_DEVFREQ_THERMAL
	/*Avoid sending HWC dump cmd to GPU when GPU is power-off*/
	if (kbdev->pm.backend.gpu_powered)
		(void)mali_kbase_devfreq_detect_bound(kbdev, stat->current_frequency, stat->busy_time);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,1,15)
	memcpy(&kbdev->devfreq->last_status, stat, sizeof(*stat));
#else
	memcpy(&kbdev->devfreq_cooling->last_status, stat, sizeof(*stat));
#endif
#endif

	return 0;
}

static struct devfreq_dev_profile mali_kbase_devfreq_profile = {
	/* it would be abnormal to enable devfreq monitor during initialization. */
	.polling_ms	= DEFAULT_POLLING_MS, //STOP_POLLING,
	.target		= mali_kbase_devfreq_target,
	.get_dev_status	= mali_kbase_get_dev_status,
};
#endif

#ifdef CONFIG_REPORT_VSYNC
void mali_kbase_pm_report_vsync(int buffer_updated)
{
	unsigned long flags;

	if (kbase_dev){
		spin_lock_irqsave(&kbase_dev->pm.backend.metrics.lock, flags);
		kbase_dev->pm.backend.metrics.vsync_hit = buffer_updated;
		spin_unlock_irqrestore(&kbase_dev->pm.backend.metrics.lock, flags);
	}
}
EXPORT_SYMBOL(mali_kbase_pm_report_vsync);
#endif

#ifdef CONFIG_MALI_MIDGARD_DVFS
int kbase_platform_dvfs_event(struct kbase_device *kbdev, u32 utilisation, u32 util_gl_share, u32 util_cl_share[2])
{
	return 1;
}

int kbase_platform_dvfs_enable(struct kbase_device *kbdev, bool enable, int freq)
{
	unsigned long flags;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	if (enable != kbdev->pm.backend.metrics.timer_active) {
		if (enable) {
			spin_lock_irqsave(&kbdev->pm.backend.metrics.lock, flags);
			kbdev->pm.backend.metrics.timer_active = MALI_TRUE;
			spin_unlock_irqrestore(&kbdev->pm.backend.metrics.lock, flags);
			hrtimer_start(&kbdev->pm.backend.metrics.timer,
					HR_TIMER_DELAY_MSEC(kbdev->pm.dvfs_period),
					HRTIMER_MODE_REL);
		} else {
			spin_lock_irqsave(&kbdev->pm.backend.metrics.lock, flags);
			kbdev->pm.backend.metrics.timer_active = MALI_FALSE;
			spin_unlock_irqrestore(&kbdev->pm.backend.metrics.lock, flags);
			hrtimer_cancel(&kbdev->pm.backend.metrics.timer);
		}
	}

	return 1;
}
#endif

#ifdef CONFIG_DEVFREQ_THERMAL
static unsigned long hisi_model_static_power(unsigned long voltage)
{
	int temperature;
	const unsigned long voltage_cubed = (voltage * voltage * voltage) >> 10;
	unsigned long temp, temp_squared, temp_cubed;
	unsigned long temp_scaling_factor = 0;

	struct device_node *dev_node = NULL;
	int ret = -EINVAL, i;
	const char *temperature_scale_capacitance[5];
	int capacitance[5] = {0};

	dev_node = of_find_node_by_name(NULL, "capacitances");
	if (dev_node) {
		for (i = 0; i < 5; i++) {
			ret = of_property_read_string_index(dev_node, "hisilicon,gpu_temp_scale_capacitance", i, &temperature_scale_capacitance[i]);
			if (ret) {
				pr_err("%s temperature_scale_capacitance [%d] read err\n",__func__,i);
				continue;
			}

			ret = kstrtoint(temperature_scale_capacitance[i], 10, &capacitance[i]);
			if (ret)
				continue;
		}
	}

	temperature = get_soc_temp();
	temp =(unsigned long)((long)temperature) / 1000;
	temp_squared = temp * temp;
	temp_cubed = temp_squared * temp;
	temp_scaling_factor = capacitance[3] * temp_cubed +
				capacitance[2] * temp_squared +
				capacitance[1] * temp +
				capacitance[0];

	return (((capacitance[4] * voltage_cubed) >> 20) * temp_scaling_factor) / 1000000;/* [false alarm]: no problem - fortify check */
}

#ifdef CONFIG_HISI_THERMAL_SPM
unsigned long hisi_calc_gpu_static_power(unsigned long voltage, int temperature)
{
	const long voltage_cubed = (voltage * voltage * voltage) >> 10;
	long temp, temp_squared, temp_cubed;
	long temp_scaling_factor;

	struct device_node *dev_node = NULL;
	int ret = -EINVAL, i;
	const char *temperature_scale_capacitance[5];
	int capacitance[5] = {0};

	dev_node = of_find_node_by_name(NULL, "capacitances");
	if (dev_node) {
		for (i = 0; i < 5; i++) {
			ret = of_property_read_string_index(dev_node, "hisilicon,gpu_temp_scale_capacitance", i, &temperature_scale_capacitance[i]);
			if (ret) {
				pr_err("%s temperature_scale_capacitance [%d] read err\n",__func__,i);
				continue;
			}

			ret = kstrtoint(temperature_scale_capacitance[i], 10, &capacitance[i]);
			if (ret)
				continue;
		}
	}

	temp = (long)temperature / 1000;
	temp_squared = temp * temp;
	temp_cubed = temp_squared * temp;
	temp_scaling_factor = capacitance[3] * temp_cubed +
				capacitance[2] * temp_squared +
				capacitance[1] * temp +
				capacitance[0];

	return (unsigned long)(((((long)capacitance[4] * voltage_cubed) / (1024 * 1024)) * temp_scaling_factor) / 1000000);/* [false alarm]: no problem - fortify check */
}
EXPORT_SYMBOL(hisi_calc_gpu_static_power);
#endif

static unsigned long hisi_model_dynamic_power(unsigned long freq,
		unsigned long voltage)
{
	/* The inputs: freq (f) is in Hz, and voltage (v) in mV.
	 * The coefficient (c) is in mW/(MHz mV mV).
	 *
	 * This function calculates the dynamic power after this formula:
	 * Pdyn (mW) = c (mW/(MHz*mV*mV)) * v (mV) * v (mV) * f (MHz)
	 */
	const unsigned long v2 = (voltage * voltage) / 1000; /* m*(V*V) */
	const unsigned long f_mhz = freq / 1000000; /* MHz */
	unsigned long coefficient = 3600; /* mW/(MHz*mV*mV) */
    struct device_node * dev_node = NULL;
    u32 prop = 0;

	dev_node = of_find_node_by_name(NULL, "capacitances");
    if(dev_node)
    {
        int ret = of_property_read_u32(dev_node,"hisilicon,gpu_dyn_capacitance",&prop);
        if(ret == 0)
        {
            coefficient = prop;
        }
    }

	return (coefficient * v2 * f_mhz) / 1000000; /* mW */
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
static struct devfreq_cooling_power hisi_model_ops = {
#else
static struct devfreq_cooling_ops hisi_model_ops = {
#endif
	.get_static_power = hisi_model_static_power,
	.get_dynamic_power = hisi_model_dynamic_power,
};
#endif

static int kbase_platform_init(struct kbase_device *kbdev)
{
      int err;
	struct device *dev = kbdev->dev;
	dev->platform_data = kbdev;

#ifdef CONFIG_REPORT_VSYNC
	kbase_dev = kbdev;
#endif


	kbdev->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(kbdev->clk)) {
		printk("[mali]  Failed to get clk\n");
		return 0;
	}


	kbdev->regulator = devm_regulator_get(dev, "gpu");
	if (IS_ERR(kbdev->regulator)) {
		printk("[mali]  Failed to get regulator\n");
		return 0;
	}

	kbdev->hisi_callbacks = (struct kbase_hisi_callbacks *)gpu_get_callbacks();

#ifdef CONFIG_PM_DEVFREQ
#ifdef CONFIG_HISI_HW_VOTE_GPU_FREQ
	err = hisi_hw_vote_gpu_regs_remap();
	if (err) {
		pr_err("%s: remap fail%d!\n", __func__, err);
		return 0;
	}
#endif

	if (/*dev_pm_opp_of_add_table(dev) ||*/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
		hisi_devfreq_init_freq_table(dev,
			&mali_kbase_devfreq_profile.freq_table)){
#else
		opp_init_devfreq_table(dev,
			&mali_kbase_devfreq_profile.freq_table)) {
#endif
		printk("[mali]  Failed to init devfreq_table\n");
		kbdev->devfreq = NULL;
	} else {
#ifdef CONFIG_HISI_HW_VOTE_GPU_FREQ
		mali_kbase_devfreq_profile.initial_freq = hisi_hw_vote_gpu_result_get(dev);
#else
		mali_kbase_devfreq_profile.initial_freq = clk_get_rate(kbdev->clk);
#endif
		rcu_read_lock();
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0))
		mali_kbase_devfreq_profile.max_state = dev_pm_opp_get_opp_count(dev);
#else
		mali_kbase_devfreq_profile.max_state = opp_get_opp_count(dev);
#endif
		rcu_read_unlock();
		dev_set_name(dev, "gpufreq");
		kbdev->devfreq = devfreq_add_device(dev,
						&mali_kbase_devfreq_profile,
						GPU_DEFAULT_GOVERNOR,
						NULL);
	}

	if (NULL == kbdev->devfreq) {
		printk("[mali]  NULL pointer [kbdev->devFreq]\n");
		goto JUMP_DEVFREQ_THERMAL;
	}

#ifdef CONFIG_DEVFREQ_THERMAL
	{
		struct devfreq_cooling_ops *callbacks;

		callbacks = (struct devfreq_cooling_ops *)POWER_MODEL_CALLBACKS;

		kbdev->devfreq_cooling = of_devfreq_cooling_register_power(
				kbdev->dev->of_node,
				kbdev->devfreq,
				callbacks);
		if (IS_ERR_OR_NULL(kbdev->devfreq_cooling)) {
			err = PTR_ERR(kbdev->devfreq_cooling);
			dev_err(kbdev->dev,
				"Failed to register cooling device (%d)\n",
				err);
			goto JUMP_DEVFREQ_THERMAL;
		}
	}
#endif

	/* make devfreq function */
	//mali_kbase_devfreq_profile.polling_ms = DEFAULT_POLLING_MS;
#endif/*CONFIG_PM_DEVFREQ*/
JUMP_DEVFREQ_THERMAL:
	return 1;
}

static void kbase_platform_term(struct kbase_device *kbdev)
{
#ifdef CONFIG_PM_DEVFREQ
	devfreq_remove_device(kbdev->devfreq);
#endif
}

kbase_platform_funcs_conf platform_funcs = {
	.platform_init_func = &kbase_platform_init,
	.platform_term_func = &kbase_platform_term,
};

static int pm_callback_power_on(struct kbase_device *kbdev)
{
#ifdef CONFIG_MALI_MIDGARD_RT_PM
	int result;
	int ret_val;
	struct device *dev = kbdev->dev;

#if (HARD_RESET_AT_POWER_OFF != 1)
	if (!pm_runtime_status_suspended(dev))
		ret_val = 0;
	else
#endif
		ret_val = 1;

	if (unlikely(dev->power.disable_depth > 0)) {
		kbase_platform_on(kbdev);
	} else {
		result = pm_runtime_resume(dev);
		if (result < 0 && result == -EAGAIN)
			kbase_platform_on(kbdev);
		else if (result < 0)
			printk("[mali]  pm_runtime_resume failed (%d)\n", result);
	}

	return ret_val;
#else
	kbase_platform_on(kbdev);

	return 1;
#endif
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
#ifdef CONFIG_MALI_MIDGARD_RT_PM
	struct device *dev = kbdev->dev;
	int ret = 0, retry = 0;

	if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_HI0008)) {
		/* when GPU in idle state, auto decrease the clock rate.
		 */
		unsigned int tiler_lo = kbdev->tiler_available_bitmap & 0xFFFFFFFF;
		unsigned int tiler_hi = (kbdev->tiler_available_bitmap >> 32) & 0xFFFFFFFF;
		unsigned int l2_lo = kbdev->l2_available_bitmap & 0xFFFFFFFF;
		unsigned int l2_hi = (kbdev->l2_available_bitmap >> 32) & 0xFFFFFFFF;

		kbase_os_reg_write(kbdev, GPU_CONTROL_REG(TILER_PWROFF_LO), tiler_lo);
		kbase_os_reg_write(kbdev, GPU_CONTROL_REG(TILER_PWROFF_HI), tiler_hi);
		kbase_os_reg_write(kbdev, GPU_CONTROL_REG(L2_PWROFF_LO), l2_lo);
		kbase_os_reg_write(kbdev, GPU_CONTROL_REG(L2_PWROFF_HI), l2_hi);
	}

#if HARD_RESET_AT_POWER_OFF
	/* Cause a GPU hard reset to test whether we have actually idled the GPU
	 * and that we properly reconfigure the GPU on power up.
	 * Usually this would be dangerous, but if the GPU is working correctly it should
	 * be completely safe as the GPU should not be active at this point.
	 * However this is disabled normally because it will most likely interfere with
	 * bus logging etc.
	 */
	KBASE_TRACE_ADD(kbdev, CORE_GPU_HARD_RESET, NULL, NULL, 0u, 0);
	kbase_os_reg_write(kbdev, GPU_CONTROL_REG(GPU_COMMAND), GPU_COMMAND_HARD_RESET);
#endif

	if (unlikely(dev->power.disable_depth > 0)) {
		kbase_platform_off(kbdev);
	} else {
		do {
			if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_HI0007))
				ret = pm_schedule_suspend(dev, RUNTIME_PM_DELAY_1MS);
			else
				ret = pm_schedule_suspend(dev, RUNTIME_PM_DELAY_30MS);
			if (ret != -EAGAIN) {
				if (unlikely(ret < 0)) {
					pr_err("[mali]  pm_schedule_suspend failed (%d)\n\n", ret);
					WARN_ON(1);
				}

				/* correct status */
				break;
			}

			/* -EAGAIN, repeated attempts for 1s totally */
			msleep(50);
		} while (++retry < 20);
	}
#else
	kbase_platform_off(kbdev);
#endif
}

static int pm_callback_runtime_init(struct kbase_device *kbdev)
{
	pm_suspend_ignore_children(kbdev->dev, true);
	pm_runtime_enable(kbdev->dev);
	return 0;
}

static void pm_callback_runtime_term(struct kbase_device *kbdev)
{
	pm_runtime_disable(kbdev->dev);
}

static void pm_callback_runtime_off(struct kbase_device *kbdev)
{
#ifdef CONFIG_PM_DEVFREQ
	devfreq_suspend_device(kbdev->devfreq);
#elif defined(CONFIG_MALI_MIDGARD_DVFS)
	kbase_platform_dvfs_enable(kbdev, false, 0);
#endif

	kbase_platform_off(kbdev);
}

static int pm_callback_runtime_on(struct kbase_device *kbdev)
{
	kbase_platform_on(kbdev);

#ifdef CONFIG_PM_DEVFREQ
	devfreq_resume_device(kbdev->devfreq);
#elif defined(CONFIG_MALI_MIDGARD_DVFS)
	if (kbase_platform_dvfs_enable(kbdev, true, 0) != MALI_TRUE)
		return -EPERM;
#endif

	return 0;
}

static inline void pm_callback_suspend(struct kbase_device *kbdev)
{
#ifdef CONFIG_MALI_MIDGARD_RT_PM
	if (!pm_runtime_status_suspended(kbdev->dev))
		pm_callback_runtime_off(kbdev);
#else
	pm_callback_power_off(kbdev);
#endif
}

static inline void pm_callback_resume(struct kbase_device *kbdev)
{
#ifdef CONFIG_MALI_MIDGARD_RT_PM
	if (!pm_runtime_status_suspended(kbdev->dev))
		pm_callback_runtime_on(kbdev);
	else
		pm_callback_power_on(kbdev);
#else
	pm_callback_power_on(kbdev);
#endif
}

static inline int pm_callback_runtime_idle(struct kbase_device *kbdev)
{
	return 1;
}

struct kbase_pm_callback_conf pm_callbacks = {
	.power_on_callback = pm_callback_power_on,
	.power_off_callback = pm_callback_power_off,
	.power_suspend_callback = pm_callback_suspend,
	.power_resume_callback = pm_callback_resume,
#ifdef CONFIG_MALI_MIDGARD_RT_PM
	.power_runtime_init_callback = pm_callback_runtime_init,
	.power_runtime_term_callback = pm_callback_runtime_term,
	.power_runtime_off_callback = pm_callback_runtime_off,
	.power_runtime_on_callback = pm_callback_runtime_on,
	.power_runtime_idle_callback = pm_callback_runtime_idle
#else
	.power_runtime_init_callback = NULL,
	.power_runtime_term_callback = NULL,
	.power_runtime_off_callback = NULL,
	.power_runtime_on_callback = NULL,
	.power_runtime_idle_callback = NULL
#endif
};



static struct kbase_platform_config hi_platform_config = {
#ifndef CONFIG_OF
	.io_resources = &io_resources
#endif
};

struct kbase_platform_config *kbase_get_platform_config(void)
{
	return &hi_platform_config;
}

int kbase_platform_early_init(void)
{
	/* Nothing needed at this stage */
	return 0;
}
