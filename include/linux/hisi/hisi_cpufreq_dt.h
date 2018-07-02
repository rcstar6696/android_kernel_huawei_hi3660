#ifndef _HISI_CPUFREQ_DT_H
#define _HISI_CPUFREQ_DT_H


#ifdef CONFIG_HISI_CPUFREQ_DT
int hisi_cpufreq_set_supported_hw(struct cpufreq_policy *policy);
void hisi_cpufreq_put_supported_hw(struct cpufreq_policy *policy);
void hisi_cpufreq_get_suspend_freq(struct cpufreq_policy *policy);

#ifdef CONFIG_HISI_HW_VOTE_CPU_FREQ
int hisi_hw_vote_target(struct cpufreq_policy *policy, unsigned int index);
unsigned int hisi_cpufreq_get(unsigned int cpu);
int hisi_cpufreq_policy_cur_init(struct cpufreq_policy *policy);
#endif
#endif

#endif /* _HISI_CPUFREQ_DT_H */
