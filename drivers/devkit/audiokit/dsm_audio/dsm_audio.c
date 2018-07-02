#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/string.h>
#include <dsm_audio.h>

static struct dsm_dev dsm_audio = {
	.name = DSM_AUDIO_NAME,
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = NULL,
	.buff_size = DSM_AUDIO_BUF_SIZE,
};

static struct dsm_dev dsm_smartpa = {
	.name = DSM_SMARTPA_NAME,
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = NULL,
	.buff_size = DSM_SMARTPA_BUF_SIZE,
};

static struct dsm_dev dsm_anc_hs = {
	.name = DSM_ANC_HS_NAME,
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = NULL,
	.buff_size = DSM_ANC_HS_BUF_SIZE,
};

//lint -save -e578 -e605  -e715 -e528 -e753
static struct dsm_client *dsm_client_table[AUDIO_DEVICE_MAX] = {NULL};
static struct dsm_dev *dsm_dev_table[AUDIO_DEVICE_MAX] = {&dsm_audio, &dsm_smartpa, &dsm_anc_hs};

static int audio_dsm_register(struct dsm_client **dsm_audio_client, struct dsm_dev *dsm_audio)
{
	if(NULL == dsm_audio || NULL == dsm_audio_client )
		return -EINVAL;

	*dsm_audio_client = dsm_register_client(dsm_audio);
	if(NULL == *dsm_audio_client) {
		*dsm_audio_client = dsm_find_client(dsm_audio->name);
		if(NULL == *dsm_audio_client) {
			dsm_loge("dsm_audio_client register failed!\n");
			return -ENOMEM;
		} else {
			dsm_loge("dsm_audio_client find in dsm_server\n");
		}
	}
	return 0;
}
//lint -restore
static int audio_dsm_init(void)
{
#ifdef CONFIG_HUAWEI_DSM
	int i = 0;
	int ret = 0;

	for(i = 0; i < AUDIO_DEVICE_MAX; i++) {
		if (NULL != dsm_client_table[i] || NULL == dsm_dev_table[i]) {
			continue;
		}
		ret = audio_dsm_register(&dsm_client_table[i], dsm_dev_table[i]);
		if(ret) {
			dsm_loge("dsm dev %s register failed %d\n",dsm_dev_table[i]->name, ret);
		}
	}
#endif
	return 0;
}

//lint -save -e578 -e605  -e715 -e528 -e753
int audio_dsm_report_num(enum audio_device_type dev_type, int error_no, unsigned int mesg_no)
{
#ifdef CONFIG_HUAWEI_DSM
	int err = 0;

	if(NULL == dsm_client_table[dev_type]) {
		dsm_loge("dsm_audio_client did not register!\n");
		return -EINVAL;
	}

	err = dsm_client_ocuppy(dsm_client_table[dev_type]);
	if(0 != err) {
		dsm_loge("user buffer is busy!\n");
		return -EBUSY;
	}

	dsm_logi("report error_no=0x%x, mesg_no=0x%x!\n",
			error_no, mesg_no);
	err = dsm_client_record(dsm_client_table[dev_type], "Message code = 0x%x.\n", mesg_no);
	dsm_client_notify(dsm_client_table[dev_type], error_no);
#endif
	return 0;
}

int audio_dsm_report_info(enum audio_device_type dev_type, int error_no, char *fmt, ...)
{
	int ret = 0;
#ifdef CONFIG_HUAWEI_DSM
	int err = 0;
	char *dsm_report_buffer = NULL;
	va_list args;

    dsm_logi("begin,errorno %d,dev_type %d ",error_no, dev_type);
	if(NULL == dsm_client_table[dev_type]) {
		dsm_loge("dsm_audio_client did not register!\n");
		ret = -EINVAL;
		goto out;
	}

	//if dsm_client_table[dev_type] is ok,then dsm_dev_table is also ok.
	dsm_report_buffer = kzalloc(dsm_dev_table[dev_type]->buff_size, GFP_KERNEL);
	if (NULL == dsm_report_buffer) {
		dsm_loge("dsm_report_buffer malloc failed\n");
		ret = -ENOMEM;
		goto out;
	}

    dsm_logi("begin,errorno %d,dev_type %d ",error_no, dev_type);
	va_start(args, fmt);
	ret = vsnprintf(dsm_report_buffer, dsm_dev_table[dev_type]->buff_size, fmt, args);
	va_end(args);
    dsm_logi("begin,errorno %d,dev_type %d ",error_no, dev_type);

	err = dsm_client_ocuppy(dsm_client_table[dev_type]);
	if(0 != err) {
		dsm_loge("user buffer is busy!\n");
		ret = -EBUSY;
		goto out;
	}

	dsm_logi("report dsm_error_no = %d, %s\n",
			error_no, dsm_report_buffer);
	dsm_client_record(dsm_client_table[dev_type], "%s\n", dsm_report_buffer);
	dsm_client_notify(dsm_client_table[dev_type], error_no);
out:
	if(dsm_report_buffer) {
		kfree(dsm_report_buffer);
		dsm_report_buffer = NULL;
	}
#endif
	return ret;
}

EXPORT_SYMBOL(audio_dsm_report_num);
EXPORT_SYMBOL(audio_dsm_report_info);

subsys_initcall_sync(audio_dsm_init);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Huawei dsm audio");
MODULE_AUTHOR("<penghongxing@huawei.com>");
//lint -restore
