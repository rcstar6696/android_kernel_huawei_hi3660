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
#include "hisi_hisee_power.h"
#include "hisi_hisee_upgrade.h"
#include "hisi_hisee_chip_test.h"

/* hisee manufacture function begin */
extern void release_hisee_semphore(void);/*should be semaphore; whatever..*/
static int otp_image_upgrade_func(void *buf, int para)
{
    int ret;
    ret = write_hisee_otp_value(OTP_IMG_TYPE);
    check_and_print_result();
    set_errno_and_return(ret);/*lint !e1058*/
}/*lint !e715*/

static int hisee_write_rpmb_key(void *buf, int para)
{
    char *buff_virt = NULL;
    phys_addr_t buff_phy = 0;
    atf_message_header *p_message_header;
    int ret = HISEE_OK;
    int image_size = 0;

    buff_virt = (void *)dma_alloc_coherent(g_hisee_data.cma_device, SIZE_1K * 4,
                                            &buff_phy, GFP_KERNEL);
    if (buff_virt == NULL) {
        pr_err("%s(): dma_alloc_coherent failed\n", __func__);
        set_errno_and_return(HISEE_NO_RESOURCES);
    }
    memset(buff_virt, 0, SIZE_1K * 4);
    p_message_header = (atf_message_header *)buff_virt;
    set_message_header(p_message_header, CMD_WRITE_RPMB_KEY);
    image_size = HISEE_ATF_MESSAGE_HEADER_LEN;
    ret = send_smc_process(p_message_header, buff_phy, image_size,
                            HISEE_ATF_WRITE_RPMBKEY_TIMEOUT, CMD_WRITE_RPMB_KEY);
    dma_free_coherent(g_hisee_data.cma_device, (unsigned long)(SIZE_1K * 4), buff_virt, buff_phy);
    check_and_print_result();
    set_errno_and_return(ret);
}/*lint !e715*/

static int set_hisee_lcs_sm_otp(void *buf, int para)
{
    char *buff_virt = NULL;
    phys_addr_t buff_phy = 0;
    atf_message_header *p_message_header;
    int ret = HISEE_OK;
    int image_size;
    unsigned int result_offset;

    buff_virt = (void *)dma_alloc_coherent(g_hisee_data.cma_device, SIZE_1K * 4,
                                            &buff_phy, GFP_KERNEL);
    if (buff_virt == NULL) {
        pr_err("%s(): dma_alloc_coherent failed\n", __func__);
        set_errno_and_return(HISEE_NO_RESOURCES);
    }
    memset(buff_virt, 0, SIZE_1K * 4);
    p_message_header = (atf_message_header *)buff_virt;
    set_message_header(p_message_header, CMD_SET_LCS_SM);

    image_size = HISEE_ATF_MESSAGE_HEADER_LEN;
    result_offset = HISEE_ATF_MESSAGE_HEADER_LEN;
    p_message_header->test_result_phy = (unsigned int)buff_phy + result_offset;
    p_message_header->test_result_size = SIZE_1K * 4 - result_offset;
    ret = send_smc_process(p_message_header, buff_phy, (unsigned int)image_size,
                            HISEE_ATF_GENERAL_TIMEOUT, CMD_SET_LCS_SM);
    if (HISEE_OK != ret) {
        pr_err("%s(): hisee reported fail code=%d\n", __func__, *((int *)(void *)(buff_virt + result_offset)));
    }

    dma_free_coherent(g_hisee_data.cma_device, (unsigned long)(SIZE_1K * 4), buff_virt, buff_phy);
    check_and_print_result();
    set_errno_and_return(ret);
}/*lint !e715*/

static int upgrade_one_file_func(char *filename, se_smc_cmd cmd)
{
    char *buff_virt;
    phys_addr_t buff_phy = 0;
    atf_message_header *p_message_header;
    int ret = HISEE_OK;
    int image_size;
    unsigned int result_offset;

    /* alloc coherent buff with vir&phy addr (64K for upgrade file) */
    buff_virt = (void *)dma_alloc_coherent(g_hisee_data.cma_device, HISEE_SHARE_BUFF_SIZE,
                                            &buff_phy, GFP_KERNEL);
    if (buff_virt == NULL) {
        pr_err("%s(): dma_alloc_coherent failed\n", __func__);
        set_errno_and_return(HISEE_NO_RESOURCES);
    }
    memset(buff_virt, 0, HISEE_SHARE_BUFF_SIZE);

    /* read given file to buff */
    ret = hisee_read_file((const char *)filename, (buff_virt + HISEE_ATF_MESSAGE_HEADER_LEN), 0, 0);
    if (ret < HISEE_OK) {
        pr_err("%s(): hisee_read_file failed, filename=%s, ret=%d\n", __func__, filename, ret);
        dma_free_coherent(g_hisee_data.cma_device, (unsigned long)HISEE_SHARE_BUFF_SIZE, buff_virt, buff_phy);
        set_errno_and_return(ret);
    }
    image_size = (ret + HISEE_ATF_MESSAGE_HEADER_LEN);

    /* init and config the message */
    p_message_header = (atf_message_header *)buff_virt; /*lint !e826*/
    set_message_header(p_message_header, cmd);
    /* reserve 256B for test result(err code from hisee) */
    result_offset = ((u32)(image_size + SMC_TEST_RESULT_SIZE - 1)) & (~(u32)(SMC_TEST_RESULT_SIZE -1));
    if (result_offset + SMC_TEST_RESULT_SIZE <= HISEE_SHARE_BUFF_SIZE) {
        p_message_header->test_result_phy = (unsigned int)buff_phy + result_offset;
        p_message_header->test_result_size = HISEE_SHARE_BUFF_SIZE - result_offset;
    } else {
        /* this case, test_result_phy will be 0 and atf will not write test result;
         * and kernel side will record err code a meaningless val (but legal) */
        result_offset = 0;
    }

    /* smc call (synchronously) */
    ret = send_smc_process(p_message_header, buff_phy, (unsigned int)image_size,
                            HISEE_ATF_GENERAL_TIMEOUT, cmd);
    if (HISEE_OK != ret) {
        if (cmd != CMD_FACTORY_APDU_TEST) /* apdu test will not report err code */
            pr_err("%s(): hisee reported err code=%d\n", __func__, *((int *)(void *)(buff_virt + result_offset)));
    }

    dma_free_coherent(g_hisee_data.cma_device, (unsigned long)HISEE_SHARE_BUFF_SIZE, buff_virt, buff_phy);
    check_and_print_result();
    set_errno_and_return(ret);
}

static int hisee_apdu_test_func(void *buf, int para)
{
    int ret;
    ret = upgrade_one_file_func("/hisee_fs/test.apdu.bin", CMD_FACTORY_APDU_TEST);
    check_and_print_result();
    set_errno_and_return(ret);
}

int hisee_verify_isd_key(void)
{
    char *buff_virt;
    phys_addr_t buff_phy = 0;
    atf_message_header *p_message_header;
    int ret = HISEE_OK;
    unsigned int image_size;

    buff_virt = (void *)dma_alloc_coherent(g_hisee_data.cma_device, (unsigned long)SIZE_1K * 4,
                                            &buff_phy, GFP_KERNEL);
    if (buff_virt == NULL) {
        pr_err("%s(): dma_alloc_coherent failed\n", __func__);
        set_errno_and_return(HISEE_NO_RESOURCES);
    }
    memset(buff_virt, 0, (unsigned long)SIZE_1K * 4);
    p_message_header = (atf_message_header *)buff_virt;  /*lint !e826*/
    set_message_header(p_message_header, CMD_HISEE_VERIFY_KEY);
    image_size = HISEE_ATF_MESSAGE_HEADER_LEN;
    ret = send_smc_process(p_message_header, buff_phy, image_size,
                            HISEE_ATF_GENERAL_TIMEOUT, CMD_HISEE_VERIFY_KEY);
    dma_free_coherent(g_hisee_data.cma_device, (unsigned long)(SIZE_1K * 4), buff_virt, buff_phy);
    check_and_print_result();
    set_errno_and_return(ret);
}

static int hisee_write_casd_key(void)
{
    return HISEE_OK;
}

static int g_hisee_flag_protect_lcs = 0;
int hisee_debug(void)
{
    return g_hisee_flag_protect_lcs;
}

static int hisee_write_rpmb_key_process(void)
{
    int ret = HISEE_OK;
    int write_rpmbkey_try = 5;

    while (write_rpmbkey_try--) {
        ret = hisee_write_rpmb_key(NULL, 0);
        if (HISEE_OK == ret) {
            break;
        }

        ret = hisee_poweroff_func(NULL, HISEE_PWROFF_LOCK);
        CHECK_OK(ret);
        ret = hisee_poweron_upgrade_func(NULL, 0);
        CHECK_OK(ret);
        hisee_mdelay(DELAY_FOR_HISEE_POWERON_UPGRADE); /*lint !e744 !e747 !e748*/
    }

err_process:
    check_and_print_result();
    return ret;
}

static int hisee_apdu_test_process(void)
{
    int ret;
    ret = hisee_apdu_test_func(NULL, 0);
    CHECK_OK(ret);

    /* send command to delete test applet */
    ret = send_apdu_cmd(HISEE_DEL_TEST_APPLET);
    CHECK_OK(ret);

err_process:
    check_and_print_result();
    return ret;
}

static int hisee_manufacture_set_lcs_sm(unsigned int hisee_lcs_mode)
{
    int ret = HISEE_OK;
    if (HISEE_DM_MODE_MAGIC == hisee_lcs_mode && g_hisee_flag_protect_lcs == 0) {
        ret = set_hisee_lcs_sm_otp(NULL, 0);
        CHECK_OK(ret);

        ret = set_hisee_lcs_sm_efuse();
        CHECK_OK(ret);
    }

err_process:
    return ret;
}


/* poweron booting hisee in misc upgrade mode
 * and do write casd key and misc image upgrade in order
 * go through: hisee misc ready -> write casd key to nvm -> misc upgrade to nvm -> hisee cos ready. */
static int hisee_poweron_booting_misc_process(void)
{
    cosimage_version_info misc_version;
    int ret;

    /* poweron booting hisee and set the flag for the process */
    ret = hisee_poweron_booting_func(NULL, HISEE_POWER_ON_BOOTING_MISC);
    CHECK_OK(ret);

    /* wait hisee ready for receiving images */
    ret = wait_hisee_ready(HISEE_STATE_MISC_READY, 30000);
    CHECK_OK(ret);

    /* write casd key should be combined with misc image upgrade and be in right order */
    ret = hisee_write_casd_key();
    CHECK_OK(ret);


    /* misc image upgrade only supported in this function */
    ret = misc_image_upgrade_func(NULL, 0);
    CHECK_OK(ret);

    /* wait hisee cos ready for later process */
    ret = wait_hisee_ready(HISEE_STATE_COS_READY, 30000);
    CHECK_OK(ret);

    /* write current misc version into record area */
    if (g_misc_version) {
        misc_version.magic = HISEE_SW_VERSION_MAGIC_VALUE;
        misc_version.img_version_num[0] = g_misc_version;
        access_hisee_image_partition((char *)&misc_version, MISC_VERSION_WRITE_TYPE);
    }

err_process:
    check_and_print_result();
    return ret;
}

static int hisee_manufacture_image_upgrade_process(unsigned int hisee_lcs_mode)
{
    int ret;

    ret = hisee_poweroff_func(NULL, HISEE_PWROFF_LOCK);
    CHECK_OK(ret);

    /* wait hisee power down, if timeout or fail, return errno */
    ret = wait_hisee_ready(HISEE_STATE_POWER_DOWN, DELAY_FOR_HISEE_POWEROFF);
    CHECK_OK(ret);

    ret = hisee_poweron_upgrade_func(NULL, 0);
    CHECK_OK(ret);
    hisee_mdelay(DELAY_FOR_HISEE_POWERON_UPGRADE); /*lint !e744 !e747 !e748*/

    if (HISEE_DM_MODE_MAGIC == hisee_lcs_mode) {
        ret = hisee_write_rpmb_key_process();
        CHECK_OK(ret);
    }

    ret = cos_image_upgrade_func(NULL, HISEE_FACTORY_TEST_VERSION);
    CHECK_OK(ret);
    hisee_mdelay(DELAY_FOR_HISEE_POWEROFF); /*lint !e744 !e747 !e748*/

    ret = hisee_poweron_booting_misc_process();
    CHECK_OK(ret);

    if (HISEE_DM_MODE_MAGIC == hisee_lcs_mode) {
        ret = otp_image_upgrade_func(NULL, 0);
        CHECK_OK(ret);
    }

err_process:
    check_and_print_result();
    return ret;
}

static int hisee_total_manufacture_func(void *buf, int para)
{
    int ret, ret1;
    unsigned int hisee_lcs_mode = 0;

    ret = get_hisee_lcs_mode(&hisee_lcs_mode);
    CHECK_OK(ret);

    ret = hisee_manufacture_image_upgrade_process(hisee_lcs_mode);
    CHECK_OK(ret);

    ret = hisee_verify_isd_key();
    CHECK_OK(ret);

    ret = hisee_apdu_test_process();
    CHECK_OK(ret);

    ret = hisee_manufacture_set_lcs_sm(hisee_lcs_mode);
    CHECK_OK(ret);

    pr_err("%s() success!\n", __func__);
    ret = HISEE_OK;

err_process:
    ret1 = hisee_poweroff_func(NULL, HISEE_PWROFF_LOCK);
    if (HISEE_OK == ret) ret = ret1;
    hisee_mdelay(DELAY_FOR_HISEE_POWEROFF);

    if (HISEE_OK == ret)
        release_hisee_semphore();/*sync signal for flash_otp_task*/

    set_errno_and_return(ret);
}

static int factory_test_body(void *arg)
{
    int ret;

    if (g_hisee_data.factory_test_state != HISEE_FACTORY_TEST_RUNNING) {
        pr_err("%s hisee factory test state error: %x\n", __func__, g_hisee_data.factory_test_state);
	ret = HISEE_FACTORY_STATE_ERROR;
	goto exit;
    }
    ret = hisee_total_manufacture_func(NULL, 0);
    if (HISEE_OK != ret)
        g_hisee_data.factory_test_state = HISEE_FACTORY_TEST_FAIL;
    else
        g_hisee_data.factory_test_state = HISEE_FACTORY_TEST_SUCCESS;

exit:
    check_and_print_result();
    set_errno_and_return(ret);
} /*lint !e715*/

int hisee_parallel_manufacture_func(void *buf, int para)
{
    int ret = HISEE_OK;
    struct task_struct *factory_test_task = NULL;

    if (HISEE_FACTORY_TEST_RUNNING != g_hisee_data.factory_test_state) {
        g_hisee_data.factory_test_state = HISEE_FACTORY_TEST_RUNNING;
        factory_test_task = kthread_run(factory_test_body, NULL, "factory_test_body");
        if (!factory_test_task) {
            ret = HISEE_THREAD_CREATE_ERROR;
            g_hisee_data.factory_test_state = HISEE_FACTORY_TEST_FAIL;
            pr_err("hisee err create factory_test_task failed\n");
        }
    }
    set_errno_and_return(ret);
}/*lint !e715*/
/* hisee manufacture function end */
/* hisee slt test function begin */
/* hisee slt test function end */

