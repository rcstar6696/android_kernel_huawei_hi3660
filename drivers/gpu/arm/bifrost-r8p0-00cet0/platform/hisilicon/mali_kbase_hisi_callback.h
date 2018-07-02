#ifndef _HISI_CALLBACK_H_
#define _HISI_CALLBACK_H_

#include <mali_kbase.h>

#define KBASE_PM_TIME_SHIFT			8

struct kbase_hisi_callbacks {
	void (* cl_boost_init)(void *dev);
	void (* cl_boost_update_utilization)(void *dev, void *atom, u64 microseconds_spent);
};

uintptr_t gpu_get_callbacks(void);

#endif
