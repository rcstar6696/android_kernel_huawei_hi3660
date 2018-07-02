#include <asm/compiler.h>
#include <linux/compiler.h>
#include <linux/fd.h>
#include <linux/tty.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/semaphore.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/atomic.h>
#include <linux/notifier.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/hisi/ipc_msg.h>
#include <linux/hisi/hisi_rproc.h>
#include <linux/hisi/kirin_partition.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include "soc_acpu_baseaddr_interface.h"
#include "soc_sctrl_interface.h"
#include "hisi_hisee.h"
#include "hisi_hisee_fs.h"


int write_hisee_otp_value(hisee_img_file_type otp_img_index)
{
	char *buff_virt;
	phys_addr_t buff_phy = 0;
	atf_message_header *p_message_header;
	int ret = HISEE_OK;
	int image_size;
	unsigned int result_offset;

	if (otp_img_index < OTP_IMG_TYPE || otp_img_index > OTP1_IMG_TYPE) {
		pr_err("%s(): otp_img_index=%d invalid\n", __func__, (int)otp_img_index);
		set_errno_and_return(HISEE_INVALID_PARAMS);
	}
	buff_virt = (void *)dma_alloc_coherent(g_hisee_data.cma_device, HISEE_SHARE_BUFF_SIZE,
											&buff_phy, GFP_KERNEL);
	if (buff_virt == NULL) {
		pr_err("%s(): dma_alloc_coherent failed\n", __func__);
		set_errno_and_return(HISEE_NO_RESOURCES);
	}

	pr_err("%s(): entering, otp_img_index=%d\n", __func__, (int)otp_img_index);
	memset(buff_virt, 0, HISEE_SHARE_BUFF_SIZE);
	p_message_header = (atf_message_header *)buff_virt;
	set_message_header(p_message_header, CMD_UPGRADE_OTP);
	ret = filesys_hisee_read_image(otp_img_index, (buff_virt + HISEE_ATF_MESSAGE_HEADER_LEN));
	if (ret < HISEE_OK) {
		pr_err("%s(): filesys_hisee_read_image failed, ret=%d\n", __func__, ret);
		dma_free_coherent(g_hisee_data.cma_device, (unsigned long)HISEE_SHARE_BUFF_SIZE, buff_virt, buff_phy);
		set_errno_and_return(ret);
	}
	image_size = (ret + HISEE_ATF_MESSAGE_HEADER_LEN);

	result_offset = (u32)(image_size + SMC_TEST_RESULT_SIZE - 1) & (~(u32)(SMC_TEST_RESULT_SIZE -1));
	if (result_offset + SMC_TEST_RESULT_SIZE <= HISEE_SHARE_BUFF_SIZE) {
		p_message_header->test_result_phy = (unsigned int)buff_phy + result_offset;
		p_message_header->test_result_size = (unsigned int)SMC_TEST_RESULT_SIZE;
	}
	ret = send_smc_process(p_message_header, buff_phy, image_size,
							HISEE_ATF_OTP_TIMEOUT, CMD_UPGRADE_OTP);
	if (HISEE_OK != ret) {
		if (result_offset + SMC_TEST_RESULT_SIZE <= HISEE_SHARE_BUFF_SIZE) {
			pr_err("%s(): hisee reported fail code=%d\n", __func__, *((int *)(void *)(buff_virt + result_offset)));
		}
	}

	dma_free_coherent(g_hisee_data.cma_device, (unsigned long)HISEE_SHARE_BUFF_SIZE, buff_virt, buff_phy);
	return ret;
}

/** read full path file interface
 * @fullname: input, the full path name should be read
 * @buffer: output, save the data
 * @offset: the offset in this file started to read
 * @size: the count bytes should be read.if zero means read total file
 * On success, the number of bytes read is returned (zero indicates end of file),
 * On error, the return value is less than zero, please check the errcode in hisee module.
 */
int hisee_read_file(const char *fullname, char *buffer, size_t offset, size_t size)
{
	struct file *fp;
	int ret = HISEE_OK;
	ssize_t cnt;
	ssize_t read_bytes = 0;
	loff_t pos = 0;
	mm_segment_t old_fs;

	if (NULL == fullname || NULL == buffer) {
		pr_err("%s(): passed ptr is NULL\n", __func__);
		set_errno_and_return(HISEE_INVALID_PARAMS);
	}
	if (/*offset >= HISEE_SHARE_BUFF_SIZE || */size > HISEE_SHARE_BUFF_SIZE) {
		pr_err("%s(): passed size is invalid\n", __func__);
		set_errno_and_return(HISEE_INVALID_PARAMS);
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);/*lint !e501*/
	fp = filp_open(fullname, O_RDONLY, 0600);
	if (IS_ERR(fp)) {
		pr_err("%s():open %s failed\n", __func__, fullname);
		ret = HISEE_OPEN_FILE_ERROR;
		goto out;
	}
	ret = vfs_llseek(fp, 0L, SEEK_END);/*lint !e712*/
	if (ret < 0) {
		pr_err("%s():lseek %s failed from end.\n", __func__, fullname);
		ret = HISEE_LSEEK_FILE_ERROR;
		goto close;
	}
	pos = fp->f_pos;/*lint !e613*/
	if ((offset + size) > (size_t)pos) {
		pr_err("%s(): offset(%lx), size(%lx) both invalid.\n", __func__, offset, size);
		ret = HISEE_OUTOF_RANGE_FILE_ERROR;
		goto close;
	}
	ret = vfs_llseek(fp, offset, SEEK_SET);/*lint !e712*/
	if (ret < 0) {
		pr_err("%s():lseek %s failed from begin.\n", __func__, fullname);
		ret = HISEE_LSEEK_FILE_ERROR;
		goto close;
	}
	read_bytes = (ssize_t)(pos - fp->f_pos);/*lint !e613*/
	pos = fp->f_pos;/*lint !e613*/
	if (0 != size)
		read_bytes = size;
	cnt = vfs_read(fp, (char __user *)buffer, read_bytes, &pos);
	if (cnt < read_bytes) {
		pr_err("%s():read %s failed, return [%ld]\n", __func__, fullname, cnt);
		ret = HISEE_READ_FILE_ERROR;
		goto close;
	}
	read_bytes = cnt;
close:
	filp_close(fp, NULL);/*lint !e668*/
out:
	set_fs(old_fs);
	if (ret >= HISEE_OK) {
		atomic_set(&g_hisee_errno, HISEE_OK);/*lint !e1058*/
		ret = read_bytes;
	} else {
		atomic_set(&g_hisee_errno, ret);/*lint !e1058*/
	}
	return ret;
}

static int check_img_header_is_valid(hisee_img_header *p_img_header)
{
	unsigned int i;
	int ret = HISEE_OK;
	unsigned int misc_cnt = 0;
	unsigned int emmc_cos_cnt = 0;
	unsigned int rpmb_cos_cnt = 0;

	for (i = 0; i <  p_img_header->file_cnt; i++) {
		if (!strncmp(p_img_header->file[i].name, HISEE_IMG_MISC_NAME, strlen(HISEE_IMG_MISC_NAME)))
			misc_cnt++;
		if (!strncmp(p_img_header->file[i].name, HISEE_IMG_COS_NAME, strlen(HISEE_IMG_COS_NAME))) {
			if ((p_img_header->file[i].name[strlen(HISEE_IMG_COS_NAME)] - '0') < COS_IMG_ID_3)
				rpmb_cos_cnt++;
			else
				emmc_cos_cnt++;
		}
		if (p_img_header->file[i].size >= HISEE_SHARE_BUFF_SIZE) {
			pr_err("%s():size check %s failed\n", __func__, p_img_header->file[i].name);
			ret = HISEE_SUB_FILE_SIZE_CHECK_ERROR;
			return ret;
		}
		if (0 == i && (p_img_header->file[i].offset != (HISEE_IMG_HEADER_LEN + HISEE_IMG_SUB_FILES_LEN * p_img_header->file_cnt))) {
			pr_err("%s():offset check %s failed\n", __func__, p_img_header->file[i].name);
			ret = HISEE_SUB_FILE_OFFSET_CHECK_ERROR;
			return ret;
		}
		if (i > 0 && (p_img_header->file[i].offset < p_img_header->file[i - 1].offset)) {
			pr_err("%s():offset check %s failed\n", __func__, p_img_header->file[i].name);
			ret = HISEE_SUB_FILE_OFFSET_CHECK_ERROR;
			return ret;
		}

		/* get misc version */
		if (misc_cnt == HISEE_MAX_MISC_IMAGE_NUMBER) {
			misc_cnt--;
			g_misc_version = p_img_header->file[i].size;
			pr_err("%s():misc version =%d\n", __func__, g_misc_version);
		}
	}
	if (/*cnt < HISEE_MIN_MISC_IMAGE_NUMBER ||*/ misc_cnt > HISEE_MAX_MISC_IMAGE_NUMBER) {
		pr_err("%s():misc cnt =%d is invalid\n", __func__, misc_cnt);
		ret = HISEE_SUB_FILE_OFFSET_CHECK_ERROR;
		return ret;
	}
	if ( (rpmb_cos_cnt + emmc_cos_cnt) > HISEE_MAX_COS_IMAGE_NUMBER) {
		pr_err("%s():cos cnt =%d is invalid\n", __func__, rpmb_cos_cnt + emmc_cos_cnt);
		ret = HISEE_SUB_FILE_OFFSET_CHECK_ERROR;
		return ret;
	}
	p_img_header->emmc_cos_cnt = emmc_cos_cnt;
	p_img_header->rpmb_cos_cnt = rpmb_cos_cnt;
	pr_err("%s():rpmb_cos cnt =%d emmc_cos cnt =%d\n", __func__, rpmb_cos_cnt , emmc_cos_cnt);
	p_img_header->misc_image_cnt = misc_cnt;
	pr_err("%s():misc_cnt =%d\n", __func__, misc_cnt);
	return ret;
}


/*static int hisee_erase_hisee_img_head(void)
{
	struct file *fp;
	char fullname[MAX_PATH_NAME_LEN + 1] = { 0 };
	char erase_head_data[HISEE_IMG_HEADER_LEN] = {0};
	int ret;
	loff_t pos;
	ssize_t cnt;
	mm_segment_t old_fs;

	ret = flash_find_ptn(HISEE_IMAGE_PARTITION_NAME, fullname);
	if (0 != ret) {
	    pr_err("%s():flash_find_ptn fail\n", __func__);
		ret = HISEE_OPEN_FILE_ERROR;
	    set_errno_and_return(ret);
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(fullname, O_WRONLY, 0600);
	if (IS_ERR(fp)) {
		pr_err("%s():open %s failed\n", __func__, fullname);
		ret = HISEE_OPEN_FILE_ERROR;
		set_fs(old_fs);
		set_errno_and_return(ret);
	}

	pos = fp->f_pos;
	cnt = vfs_write(fp, (char __user *)erase_head_data, (unsigned long)HISEE_IMG_HEADER_LEN, &pos);
	if (cnt < HISEE_IMG_HEADER_LEN) {
		pr_err("%s():write failed, return [%ld]\n", __func__, cnt);
		ret = HISEE_WRITE_FILE_ERROR;
	}

	filp_close(fp, NULL);
	set_fs(old_fs);
	check_and_print_result();
	set_errno_and_return(ret);
}*/

/** parse the hisee_img partition header interface
 * @buffer: output, save the data
 * On success, zero is return,
 * On error, the return value is less than zero, please check the errcode in hisee module.
 */
int hisee_parse_img_header(char *buffer)
{
	struct file *fp;
	hisee_img_header *p_img_header;
	unsigned int sw_version_num;
	char fullname[MAX_PATH_NAME_LEN + 1] = { 0 };
	char cos_img_rawdata[HISEE_IMG_HEADER_LEN] = {0};
	int ret;
	loff_t pos;
	ssize_t cnt;
	unsigned int i, cos_cnt;
	mm_segment_t old_fs;

	ret = flash_find_ptn(HISEE_IMAGE_PARTITION_NAME, fullname);
	if (0 != ret) {
	    pr_err("%s():flash_find_ptn fail\n", __func__);
		ret = HISEE_OPEN_FILE_ERROR;
	    set_errno_and_return(ret);
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);/*lint !e501*/
	fp = filp_open(fullname, O_RDONLY, 0600);
	if (IS_ERR(fp)) {
		pr_err("%s():open %s failed\n", __func__, fullname);
		ret = HISEE_OPEN_FILE_ERROR;
		set_fs(old_fs);
		set_errno_and_return(ret);
	}

	pos = fp->f_pos;/*lint !e613*/
	cnt = vfs_read(fp, (char __user *)buffer, HISEE_IMG_HEADER_LEN, &pos);
	if (cnt < (ssize_t)HISEE_IMG_HEADER_LEN) {
		pr_err("%s():read %s failed, return [%ld]\n", __func__, fullname, cnt);
		ret = HISEE_READ_FILE_ERROR;
		goto error;
	}
	fp->f_pos = pos;/*lint !e613*/
	p_img_header = (hisee_img_header *)buffer;
	if (strncmp(p_img_header->magic, HISEE_IMG_MAGIC_VALUE, HISEE_IMG_MAGIC_LEN)) {
		pr_err("%s() hisee_img magic value is wrong.\n", __func__);
		ret = HISEE_IMG_PARTITION_MAGIC_ERROR;
		goto error;
	}
	if (p_img_header->file_cnt > HISEE_IMG_SUB_FILE_MAX) {
		pr_err("%s() hisee_img file numbers is invalid.\n", __func__);
		ret = HISEE_IMG_PARTITION_FILES_ERROR;
		goto error;
	}
	cnt = vfs_read(fp, (char __user *)(buffer + HISEE_IMG_HEADER_LEN), (unsigned long)(unsigned int)(HISEE_IMG_SUB_FILES_LEN * p_img_header->file_cnt), &pos);
	if (cnt < (ssize_t)(unsigned)(HISEE_IMG_SUB_FILES_LEN * p_img_header->file_cnt)) {
		pr_err("%s():read %s failed, return [%ld]\n", __func__, fullname, cnt);
		ret = HISEE_READ_FILE_ERROR;
		goto error;
	}

	ret = check_img_header_is_valid(p_img_header);
	if (HISEE_OK != ret) {
		pr_err("%s(): check_img_header_is_valid fail,ret=%d.\n", __func__, ret);
		goto error;
	}
	cos_cnt = g_hisee_data.hisee_img_head.rpmb_cos_cnt + g_hisee_data.hisee_img_head.emmc_cos_cnt;
	/*there is a assumption: the first file in hisee.img is always the cos image*/
	for (i = 0; i < cos_cnt; i++) {
		fp->f_pos = p_img_header->file[i].offset;/*lint !e613*/
		cnt = vfs_read(fp, (char __user *)cos_img_rawdata, (unsigned long)HISEE_IMG_HEADER_LEN, &pos);
		if (cnt < (ssize_t)HISEE_IMG_HEADER_LEN) {
			pr_err("%s():read %s failed, return [%ld]\n", __func__, fullname, cnt);
			ret = HISEE_READ_FILE_ERROR;
			goto error;
		}
		sw_version_num = *((unsigned int *)(&cos_img_rawdata[HISEE_COS_VERSION_OFFSET]));/*lint !e826*/
		p_img_header->sw_version_cnt[i] = sw_version_num;
	}

error:
	filp_close(fp, NULL);/*lint !e668*/
	set_fs(old_fs);
	set_errno_and_return(ret);
}

int get_sub_file_name(hisee_img_file_type type, char *sub_file_name)
{
	switch (type) {
	case SLOADER_IMG_TYPE:
		strncat(sub_file_name, HISEE_IMG_SLOADER_NAME, HISEE_IMG_SUB_FILE_NAME_LEN);
		break;
	case COS_IMG_TYPE:
		strncat(sub_file_name, HISEE_IMG_COS_NAME, HISEE_IMG_SUB_FILE_NAME_LEN);
		break;
	case COS1_IMG_TYPE:
	case COS2_IMG_TYPE:
	case COS3_IMG_TYPE:
		strncat(sub_file_name, HISEE_IMG_COS_NAME, (unsigned long)HISEE_IMG_SUB_FILE_NAME_LEN);
		sub_file_name[3] = (char)'0' + (char)(type - COS_IMG_TYPE);/*lint !e734*/
		break;
	case OTP_IMG_TYPE:
	case OTP1_IMG_TYPE:
		strncat(sub_file_name, HISEE_IMG_OTP0_NAME, HISEE_IMG_SUB_FILE_NAME_LEN);
		break;
	case MISC0_IMG_TYPE:
	case MISC1_IMG_TYPE:
	case MISC2_IMG_TYPE:
	case MISC3_IMG_TYPE:
	case MISC4_IMG_TYPE:
		strncat(sub_file_name, HISEE_IMG_MISC_NAME, HISEE_IMG_SUB_FILE_NAME_LEN);
		sub_file_name[4] = (char)'0' + (char)(type - MISC0_IMG_TYPE);
		break;
	default:
		return HISEE_IMG_SUB_FILE_NAME_ERROR;
	}
	return HISEE_OK;
}


static int hisee_image_type_chk(hisee_img_file_type type, char *sub_file_name, unsigned int *index)
{
	unsigned int i;
	int ret = HISEE_OK;

	for (i = 0; i < HISEE_IMG_SUB_FILE_MAX; i++) {
		if (!g_hisee_data.hisee_img_head.file[i].name[0])
			continue;
		if (OTP_IMG_TYPE == type) {
			if (!strncmp(sub_file_name, g_hisee_data.hisee_img_head.file[i].name, (unsigned long)HISEE_IMG_SUB_FILE_NAME_LEN) ||
				!strncmp(HISEE_IMG_OTP_NAME, g_hisee_data.hisee_img_head.file[i].name, (unsigned long)HISEE_IMG_SUB_FILE_NAME_LEN))
				break;
		} else if (OTP1_IMG_TYPE == type) {
			if (!strncmp(HISEE_IMG_OTP1_NAME, g_hisee_data.hisee_img_head.file[i].name, (unsigned long)HISEE_IMG_SUB_FILE_NAME_LEN))
				break;
		}
		else {
			if (!strncmp(sub_file_name, g_hisee_data.hisee_img_head.file[i].name, (unsigned long)HISEE_IMG_SUB_FILE_NAME_LEN))
				break;
		}
	}
	if (i == HISEE_IMG_SUB_FILE_MAX) {
		pr_err("%s():hisee_read_img_header failed, ret=%d\n", __func__, ret);
		ret = HISEE_IMG_SUB_FILE_ABSENT_ERROR;
	}

	*index = i;

	return ret;
}

/* read hisee_fs partition file interface
* @type: input, the file type need to read in hisee_img partiton
* @buffer: output, save the data
* On success, the number of bytes read is returned (zero indicates end of file),
* On error, the return value is less than zero, please check the errcode in hisee module.
*/
int filesys_hisee_read_image(hisee_img_file_type type, char *buffer)
{
	int ret = HISEE_OK;
	char sub_file_name[HISEE_IMG_SUB_FILE_NAME_LEN] = {0};
	char fullname[MAX_PATH_NAME_LEN + 1] = { 0 };
	unsigned int index;

	if (NULL == buffer) {
		set_errno_and_return(HISEE_INVALID_PARAMS);
	}

	ret = get_sub_file_name(type, sub_file_name);
	if (HISEE_OK != ret) {
		set_errno_and_return(ret);
	}

	mutex_lock(&g_hisee_data.hisee_mutex);
	ret = hisee_parse_img_header((char *)&(g_hisee_data.hisee_img_head));
	if (HISEE_OK != ret) {
		pr_err("%s():hisee_read_img_header failed, ret=%d\n", __func__, ret);
		mutex_unlock(&g_hisee_data.hisee_mutex);
		set_errno_and_return(ret);
	}
	mutex_unlock(&g_hisee_data.hisee_mutex);

	ret = hisee_image_type_chk(type, sub_file_name, &index);
	if (HISEE_OK != ret) {
		set_errno_and_return(ret);
	}

	ret = flash_find_ptn(HISEE_IMAGE_PARTITION_NAME, fullname);
	if (0 != ret) {
		pr_err("%s():flash_find_ptn fail\n", __func__);
		ret = HISEE_OPEN_FILE_ERROR;
		atomic_set(&g_hisee_errno, ret);/*lint !e1058*/
		return ret;
	}

	ret = hisee_read_file((const char *)fullname, buffer,
							g_hisee_data.hisee_img_head.file[index].offset,
							g_hisee_data.hisee_img_head.file[index].size);
	if (ret < HISEE_OK) {
		pr_err("%s():hisee_read_file failed, ret=%d\n", __func__, ret);
		atomic_set(&g_hisee_errno, ret);/*lint !e1058*/
		return ret;
	}
	ret = g_hisee_data.hisee_img_head.file[index].size;
	atomic_set(&g_hisee_errno, HISEE_OK);/*lint !e1058*/
	return ret;
}




static void access_hisee_file_prepare(hisee_image_a_access_type access_type, int *flags, long *file_offset, unsigned long *size)
{
	if ((SW_VERSION_WRITE_TYPE == access_type)
		|| (COS_UPGRADE_RUN_WRITE_TYPE == access_type)
		|| (MISC_VERSION_WRITE_TYPE == access_type)
		|| (SW_COSID_RUN_WRITE_TYPE == access_type))
		*flags = O_WRONLY;
	else
		*flags = O_RDONLY;

	if ((SW_VERSION_WRITE_TYPE == access_type)
		|| (SW_VERSION_READ_TYPE == access_type)) {
		*file_offset = HISEE_IMG_PARTITION_SIZE - SIZE_1K;
		*size = sizeof(cosimage_version_info);
	} else if ((COS_UPGRADE_RUN_WRITE_TYPE == access_type)
		|| (COS_UPGRADE_RUN_READ_TYPE == access_type)){
		*file_offset = (long)((HISEE_IMG_PARTITION_SIZE - SIZE_1K) + HISEE_COS_VERSION_STORE_SIZE);
		*size = sizeof(unsigned int);
	} else if ((MISC_VERSION_READ_TYPE == access_type)
					|| (MISC_VERSION_WRITE_TYPE == access_type)){
		*file_offset = (long)((HISEE_IMG_PARTITION_SIZE - SIZE_1K) + HISEE_COS_VERSION_STORE_SIZE
						+ HISEE_UPGRADE_STORE_SIZE);
		*size = sizeof(cosimage_version_info);
	} else {
		*file_offset = (long)((HISEE_IMG_PARTITION_SIZE - SIZE_1K) + HISEE_COS_VERSION_STORE_SIZE
													+ HISEE_UPGRADE_STORE_SIZE
													+ HISEE_MISC_VERSION_STORE_SIZE);
		*size = sizeof(unsigned int);
	}

	return;
}

int access_hisee_image_partition(char *data_buf, hisee_image_a_access_type access_type)
{
	int fd;
	ssize_t cnt;
	mm_segment_t old_fs;
	char fullpath[128] = {0};
	long file_offset;
	unsigned long size;
	int ret;
	int flags;

	if (1 == g_hisee_partition_byname_find) {
		ret = flash_find_ptn(HISEE_IMAGE_PARTITION_NAME, fullpath);
		if (0 != ret) {
			pr_err("%s():flash_find_ptn fail\n", __func__);
			ret = HISEE_OPEN_FILE_ERROR;
			set_errno_and_return(ret);
		}
	} else {
		flash_find_hisee_ptn(HISEE_IMAGE_A_PARTION_NAME, fullpath);
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);/*lint !e501*/

	access_hisee_file_prepare(access_type, &flags, &file_offset, &size);

	fd = (int)sys_open(fullpath, flags, HISEE_FILESYS_DEFAULT_MODE);
	if (fd < 0) {
		pr_err("%s():open %s failed %d\n", __func__, fullpath, fd);
		ret = HISEE_OPEN_FILE_ERROR;
		set_fs(old_fs);
		set_errno_and_return(ret);
	}

	ret = (int)sys_lseek((unsigned int)fd, file_offset, SEEK_SET);
	if (ret < 0) {
		pr_err("%s(): sys_lseek failed,ret=%d.\n", __func__, ret);
		ret = HISEE_LSEEK_FILE_ERROR;
		sys_close((unsigned int)fd);
		set_fs(old_fs);
		set_errno_and_return(ret);
	}
	ret = HISEE_OK;
	if ((SW_VERSION_WRITE_TYPE == access_type)
		|| (COS_UPGRADE_RUN_WRITE_TYPE == access_type)
		|| (MISC_VERSION_WRITE_TYPE == access_type)
		|| (SW_COSID_RUN_WRITE_TYPE == access_type)) {
		cnt = sys_write((unsigned int)fd, (char __user *)data_buf, size);
		ret = sys_fsync((unsigned int)fd);
		if (ret < 0) {
			pr_err("%s():fail to sync %s.\n", __func__, fullpath);
			ret = HISEE_ENCOS_SYNC_FILE_ERROR;
		}
	} else
		cnt = sys_read((unsigned int)fd, (char __user *)data_buf, size);

	if (cnt < (ssize_t)(size)) {
		pr_err("%s(): access %s failed, return [%ld]\n", __func__, fullpath, cnt);
		ret = HISEE_ACCESS_FILE_ERROR;
	}

	sys_close((unsigned int)fd);
	set_fs(old_fs);
	set_errno_and_return(ret);
}
