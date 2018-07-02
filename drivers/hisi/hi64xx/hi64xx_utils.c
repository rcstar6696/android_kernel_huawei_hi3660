#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/hisi/hi64xx/hi64xx_utils.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>

/*lint -e750 -e679*/

#define SIZE_MAX_HI64xxDUMP            0x6000
#define AUDIO_DEBUG_DIR                "audio"
#define AUDIO_DEBUG_HI64XXdump         "hi64xxdump"


static struct proc_dir_entry *audio_debug_dir = NULL;
static struct utils_config *s_utils_config = NULL;

extern void hi64xx_resmgr_dump(struct hi64xx_resmgr *resmgr);

int hi64xx_update_bits(struct snd_soc_codec *codec, unsigned int reg,
                unsigned int mask, unsigned int value)
{
	int change;
	unsigned int old, new;

	old = snd_soc_read(codec, reg);
	new = (old & ~mask) | (value & mask);
	change = old != new;
	if (change)
		snd_soc_write(codec, reg, new);

	return change;
}
EXPORT_SYMBOL_GPL(hi64xx_update_bits);


static ssize_t hi64xx_dump_read(struct file *file, char __user *user_buf,
			size_t count, loff_t *ppos)
{
	char * buf = NULL;
	ssize_t value_ret= 0;

	if (user_buf == NULL) {
		pr_err("%s user_buf is null.\n", __FUNCTION__);
		return -EINVAL;
	}


	buf = vmalloc(SIZE_MAX_HI64xxDUMP);
	if (NULL == buf) {
		pr_err("buf NULL!\n");
		return  -ENOMEM;
	}
	memset(buf, 0, SIZE_MAX_HI64xxDUMP);

	if (s_utils_config->hi64xx_dump_reg != NULL) {
		s_utils_config->hi64xx_dump_reg(buf, SIZE_MAX_HI64xxDUMP);
	} else {
		snprintf(buf, SIZE_MAX_HI64xxDUMP, "hi64xx_dump_reg function is null. \n");
	}

	value_ret = simple_read_from_buffer(user_buf, count, ppos, buf, strlen(buf));

	vfree(buf);
	buf = NULL;

	return value_ret;
}

static void hi64xx_remove_audio_debug_procfs(void)
{
	remove_proc_entry(AUDIO_DEBUG_HI64XXdump, audio_debug_dir);
	remove_proc_entry(AUDIO_DEBUG_DIR, 0);
}


static const struct file_operations hi64xx_dump_proc_ops = {
	.owner = THIS_MODULE,
	.read  = hi64xx_dump_read,
};

int hi64xx_utils_init(struct snd_soc_codec *codec, struct hi_cdc_ctrl *cdc_ctrl, const struct utils_config *config,
	struct hi64xx_resmgr* resmgr)
{
	struct proc_dir_entry * hi64xx_dump = NULL;

	s_utils_config = kzalloc(sizeof(struct utils_config), GFP_KERNEL);
	if (!s_utils_config) {
		pr_err("hi64xx_utils_init: Failed to kzalloc s_utils_config\n");
		goto error_exit;
	}
	memcpy(s_utils_config, config, sizeof(struct utils_config));

	audio_debug_dir = proc_mkdir(AUDIO_DEBUG_DIR, NULL);
	if (!audio_debug_dir) {
		pr_err("hi64xx_utils_init: Failed to create audio debug proc dir\n");
		goto error_exit;
	}
	hi64xx_dump = proc_create(AUDIO_DEBUG_HI64XXdump, S_IRUSR|S_IRGRP, audio_debug_dir, &hi64xx_dump_proc_ops);
	if (!hi64xx_dump) {
		pr_err("hi64xx_utils_init: Failed to create hi64xxdump proc\n");
		remove_proc_entry(AUDIO_DEBUG_DIR, 0);
		goto error_exit;
	}



	return 0;

error_exit:
	if (!s_utils_config) {
		kfree(s_utils_config);
		s_utils_config = NULL;
	}

	return 0;
}
EXPORT_SYMBOL(hi64xx_utils_init);

void hi64xx_utils_deinit(void)
{
	if (!s_utils_config) {
		kfree(s_utils_config);
		s_utils_config = NULL;
	}
	hi64xx_remove_audio_debug_procfs();
}
EXPORT_SYMBOL(hi64xx_utils_deinit);

MODULE_DESCRIPTION("hi64xx util");
MODULE_AUTHOR("liujinhong <liujinhong@hisilicon.com>");
MODULE_LICENSE("GPL");
