#include <linux/version.h>
#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/workqueue.h>
#include "shmem.h"
#include "inputhub_api.h"
#include "common.h"
#include <libhwsecurec/securec.h>

#define SHMEM_AP_RECV_PHY_ADDR            DDR_SHMEM_CH_SEND_ADDR_AP
#define SHMEM_AP_RECV_PHY_SIZE            DDR_SHMEM_CH_SEND_SIZE
#define SHMEM_AP_SEND_PHY_ADDR            DDR_SHMEM_AP_SEND_ADDR_AP
#define SHMEM_AP_SEND_PHY_SIZE            DDR_SHMEM_AP_SEND_SIZE
#define SHMEM_INIT_OK                     (0x0aaaa5555)
#define MODULE_NAME                       "sharemem"

static LIST_HEAD(shmem_client_list);
static DEFINE_MUTEX(shmem_recv_lock); /*lint !e651 !e708 !e570 !e64 !e785*/

struct shmem_ipc_data {
	unsigned int module_id;	/*enum is different between M7 & A53, so use "unsigned int" */
	unsigned int buf_size;
};

struct shmem_ipc {
	pkt_header_t hd;
	struct shmem_ipc_data data;
};

struct shmem {
	unsigned int init_flag;
	void __iomem *recv_addr;
	void __iomem *send_addr;
	struct semaphore send_sem;
};

static struct shmem shmem_gov;

static int shmem_ipc_send(unsigned char cmd, obj_tag_t module_id,
			  unsigned int size)
{
	struct shmem_ipc pkt;
#ifdef CONFIG_INPUTHUB_20
	write_info_t winfo;
#endif

	pkt.data.module_id = module_id;
	pkt.data.buf_size = size;
#ifdef CONFIG_INPUTHUB_20
	winfo.tag = TAG_SHAREMEM;
	winfo.cmd = cmd;
	winfo.wr_buf = &pkt.data;
	winfo.wr_len = sizeof(struct shmem_ipc_data);
	return write_customize_cmd(&winfo, NULL, true);
#else
	pkt.hd.tag = TAG_SHAREMEM;
	pkt.hd.cmd = cmd;
	pkt.hd.resp = 0;
	pkt.hd.length = sizeof(struct shmem_ipc_data);
	return inputhub_mcu_write_cmd_adapter(&pkt, (unsigned int)sizeof(pkt), NULL);
#endif
}

struct workqueue_struct *receive_response_wq = NULL;
struct receive_response_work_t {
	struct shmem_ipc_data data;
	struct work_struct worker;
};

struct receive_response_work_t receive_response_work;

static void receive_response_work_handler(struct work_struct *work)
{
	struct receive_response_work_t *p =
	    container_of(work, struct receive_response_work_t, worker); /*lint !e826*/
	if (!p) {
		pr_err("%s NULL pointer\n", __func__);
		return;
	}
	shmem_ipc_send(CMD_SHMEM_AP_RECV_RESP, (obj_tag_t)p->data.module_id,
		       p->data.buf_size);
}

const pkt_header_t *shmempack(const char *buf, unsigned int length)
{
	struct shmem_ipc *msg;
	static char recv_buf[SHMEM_AP_RECV_PHY_SIZE] = { 0, };
	const pkt_header_t *head = (const pkt_header_t *)recv_buf;

	if (NULL == buf)
		return NULL;

	msg = (struct shmem_ipc *)buf;

	memcpy_s(recv_buf, sizeof(recv_buf), shmem_gov.recv_addr, (size_t)msg->data.buf_size);
	memcpy_s(&receive_response_work.data, sizeof(receive_response_work.data), &msg->data, sizeof(receive_response_work.data));
	queue_work(receive_response_wq, &receive_response_work.worker);

	return head;
} /*lint !e715*/

static int shmem_recv_init(void)
{
	receive_response_wq =
	    create_freezable_workqueue("sharemem_receive_response");
	if (!receive_response_wq) {
		pr_err("failed to create sharemem_receive_response workqueue\n");
		return -1;
	}

	shmem_gov.recv_addr =
	    ioremap_wc((ssize_t)SHMEM_AP_RECV_PHY_ADDR, (unsigned long)SHMEM_AP_RECV_PHY_SIZE);
	if (!shmem_gov.recv_addr) {
		pr_err("[%s] ioremap err\n", __func__);
		return -ENOMEM;
	}

	INIT_WORK(&receive_response_work.worker, receive_response_work_handler);

	return 0;
}

int shmem_send(obj_tag_t module_id, const void *usr_buf,
	       unsigned int usr_buf_size)
{
	int ret;
	if ((NULL == usr_buf) || (usr_buf_size > SHMEM_AP_SEND_PHY_SIZE))
		return -EINVAL;
	if (SHMEM_INIT_OK != shmem_gov.init_flag)
		return -EPERM;
	ret = down_timeout(&shmem_gov.send_sem, (long)msecs_to_jiffies(500));
	if (ret)
		pr_warning("[%s]down_timeout 500\n", __func__);
	memcpy_s((void *)shmem_gov.send_addr, (size_t)SHMEM_AP_SEND_PHY_SIZE, usr_buf, (unsigned long)usr_buf_size);
	return shmem_ipc_send(CMD_SHMEM_AP_SEND_REQ, module_id, usr_buf_size);
}

static int shmem_send_resp(const pkt_header_t * head)
{
	up(&shmem_gov.send_sem);
	return 0;
} /*lint !e715*/

static int shmem_send_init(void)
{
	int ret = register_mcu_event_notifier(TAG_SHAREMEM,
					      CMD_SHMEM_AP_SEND_RESP,
					      shmem_send_resp);
	if (ret) {
		pr_err("[%s] register_mcu_event_notifier err\n", __func__);
		return ret;
	}

	shmem_gov.send_addr =
	    ioremap_wc((ssize_t)SHMEM_AP_SEND_PHY_ADDR, (unsigned long)SHMEM_AP_SEND_PHY_SIZE);
	if (!shmem_gov.send_addr) {
		unregister_mcu_event_notifier(TAG_SHAREMEM,
					      CMD_SHMEM_AP_SEND_RESP,
					      shmem_send_resp);
		pr_err("[%s] ioremap err\n", __func__);
		return -ENOMEM;
	}

	sema_init(&shmem_gov.send_sem, 1);

	return 0;
}

int contexthub_shmem_init(void)
{
	int ret;
	ret = get_contexthub_dts_status();
	if(ret)
		return ret;

	ret = shmem_recv_init();
	if (ret)
		return ret;
	ret = shmem_send_init();
	if (ret)
		return ret;
	shmem_gov.init_flag = SHMEM_INIT_OK;
	return ret;
}

/*lint -e753*/
MODULE_ALIAS("platform:contexthub" MODULE_NAME);
MODULE_LICENSE("GPL v2");

