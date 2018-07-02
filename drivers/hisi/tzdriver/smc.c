#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/semaphore.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/list.h>
#include <linux/cpu.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/spinlock.h>
#include <asm/compiler.h>
#include <linux/timer.h>
#include <linux/rtc.h>
#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/string.h>
#include <linux/hisi/rdr_pub.h>

#include <huawei_platform/log/imonitor.h>
#define IMONITOR_TEEOS_EVENT_ID		(901002003)
#include "securec.h"
#include "tc_ns_log.h"

/*#define TC_DEBUG*/
/*#define TC_VERBOSE*/
#include "smc.h"
#include "teek_client_constants.h"
#include "tc_ns_client.h"
#include "agent.h"
#include "teek_ns_client.h"
#include "tui.h"

#include "tlogger.h"
#include <linux/crc32.h>
#include "security_auth_enhance.h"

struct session_crypto_info *g_session_root_key = NULL;

/*lint -save -e750 -e529*/
#include <linux/hisi/hisi_drmdriver.h>
#include <linux/cpumask.h>

#define SECS_POWER_UP	(0xAA55A5A5)
#define SECS_POWER_DOWN	(0x55AA5A5A)
#define SECS_SECCLK_EN  (0x5A5A55AA)
#define SECS_SECCLK_DIS (0xA5A5AA55)

#define TRIGGER_TIMER (jiffies + HZ) /*trigger time 1s*/
#define CPU0 (0)
#define ONLY_CPU0_ONLINE (0xE5A5)
#define OTHER_CPU_ONLINE (0xA5E5)

static DEFINE_MUTEX(secs_timer_lock);
static DEFINE_MUTEX(secs_count_lock);
static void secs_timer_func(unsigned long unused_value);
static DEFINE_TIMER(secs_timer, secs_timer_func, 0, 0);/*lint -e785 */
static unsigned long g_secs_power_ctrl_count = 0;
#define UNUSED(x)                   ((void)x)

#define MAX_EMPTY_RUNS		100

static atomic_t event_nr;
/* Number of outstanding commands */
static atomic_t outstading_cmds;
/* Current state of the system */
static uint8_t sys_crash;

enum SPI_CLK_MODE {
	SPI_CLK_OFF = 0,
	SPI_CLK_ON,
};

struct regulator_bulk_data regu_burning;
struct clk *secs_clk;

struct wait_entry {
	struct list_head list;
	uint32_t event_nr;
	TC_NS_SMC_CMD *cmd;
	struct completion done;
};

struct smc_work {
	struct kthread_work kthwork;
	struct work_struct work;
	TC_NS_SMC_CMD *cmd;
	struct wait_entry *we;
	uint32_t cmd_type;
	unsigned int cmd_ret;
};

static struct task_struct *cmd_work_thread;
static DEFINE_KTHREAD_WORKER(cmd_worker);

static struct task_struct *smc_thread;
static struct task_struct *siq_thread;

#define MAX_SMC_CMD 20

typedef struct __attribute__ ((__packed__)) TC_NS_SMC_QUEUE {
	uint32_t last_in;
	TC_NS_SMC_CMD in[MAX_SMC_CMD];
	uint32_t last_out;
	TC_NS_SMC_CMD out[MAX_SMC_CMD];
} TC_NS_SMC_QUEUE;

/* tzdriver's own queue pointer */
static uint32_t last_in, last_out;

TC_NS_SMC_QUEUE *cmd_data;
phys_addr_t cmd_phys;

/*spinlock_t wait_th_lock = SPIN_LOCK_UNLOCKED;*/
static DEFINE_MUTEX(wait_th_lock);

static DECLARE_WAIT_QUEUE_HEAD(smc_th_wait);
static DECLARE_WAIT_QUEUE_HEAD(siq_th_wait);

static atomic_t smc_th_run;
static atomic_t siq_th_run;

static LIST_HEAD(wait_list);

extern int spi_init_secos(unsigned int spi_bus_id);
extern int spi_exit_secos(unsigned int spi_bus_id);

static int sec_s_power_on(void);
static int sec_s_power_down(void);

static noinline int smc_send(uint32_t cmd, phys_addr_t cmd_addr,
			     uint32_t cmd_type, uint8_t wait)
{
	/*tlogd("start to send smc to secure\n");*/
	register u64 x0 asm("x0") = cmd;
	register u64 x1 asm("x1") = cmd_addr;
	register u64 x2 asm("x2") = cmd_type;
	register u64 x3 asm("x3") = cmd_addr >> 32;

	do {
		asm volatile(
				__asmeq("%0", "x0")
				__asmeq("%1", "x0")
				__asmeq("%2", "x1")
				__asmeq("%3", "x2")
				__asmeq("%4", "x3")
				"smc	#0\n"
				: "+r" (x0)
				: "r" (x0), "r" (x1), "r" (x2), "r" (x3));
	} while (x0 == TSP_REQUEST && wait);

	return x0;
}

static int siq_thread_fn(void *arg)
{
	int ret;

	while(1) {
		ret = wait_event_interruptible(siq_th_wait,/*lint !e666*/
				atomic_read(&siq_th_run));
		if (ret) {
			tloge("wait_event_interruptible failed!\n");
			return -EINTR;
		}

		atomic_set(&siq_th_run, 0);/*lint !e1058*/
		smc_send(TSP_REE_SIQ, (phys_addr_t)1, 0, false);
	}
}

static int smc_thread_fn(void *arg)
{
	uint32_t ret, i = 0;
	uint8_t cmd_processed = 0, cmd_count = 0;
	struct wait_entry *we = NULL;
	struct wait_entry *tmp = NULL;
	bool slept = false;

	atomic_set(&smc_th_run, 0);/*lint !e1058*/

	while (!kthread_should_stop()) {
		cmd_processed = 0;

		/* Start running TrustedCore becasue
		 * we have a incoming command */
		ret = TSP_REQUEST;
		if (i % 10 == 0 && atomic_read(&outstading_cmds) > 1)
			smc_send(TSP_REE_SIQ, 0, 0, false);
		else
			ret = smc_send(TSP_REQUEST, 0, 0, false);
		i++;
		if (ret == TSP_CRASH) {
			tlogd("System has crashed!\n");
			sys_crash = 1;
			tlogger_store_lastmsg();
			(void)teeos_log_exception_archive();
			rdr_system_error(0x83000001, 0, 0);
			break;
		}

		while (last_out != cmd_data->last_out) {
			TC_NS_SMC_CMD cmd;
			uint8_t found = 0;
			/* Command was processed in this run */
			cmd_processed = 1;
			/* Reset consecutive empty runs */
			cmd_count = 0;
			tlogd("processing answer %d\n", last_out);

			/* Get copy of answer so there is no
			 * risk of changing in the foreach */
			if (EOK != memcpy_s(&cmd,
						sizeof(TC_NS_SMC_CMD),
						&cmd_data->out[last_out],
						sizeof(TC_NS_SMC_CMD))) {
				tloge("memcpy_s failed,%s line:%d",
						__func__, __LINE__);
				goto next_cmd;
			}

			if ((TEEC_SUCCESS == cmd.ret_val) && (TEEC_SUCCESS
				!= verify_chksum(&cmd))) {
				cmd.ret_val = (uint32_t)TEEC_ERROR_GENERIC;
				tloge("verify_chksum failed.\n");
			}
			if (cmd.ret_val == TEEC_PENDING
			    || cmd.ret_val == TEEC_PENDING2) { /*lint !e650 */
				tlogd("wakeup command %d\n", cmd.event_nr);
			}
			if (TEE_ERROR_TAGET_DEAD == cmd.ret_val) { /*lint !e650 !e737 */
				tloge("error smc call: ret = %x and cmd.err_origin=%x\n",
						cmd.ret_val, cmd.err_origin);
				if(TEEC_ORIGIN_TRUSTED_APP_TUI == cmd.err_origin){
					do_ns_tui_release();
				}
				tlogger_store_lastmsg();
				if (0 > teeos_log_exception_archive())
					tloge("log_exception_archive failed\n");
			}
			if (cmd.ret_val == TEEC_PENDING2) { /*lint !e650 */
				unsigned int agent_id = cmd.agent_id;

				/* If the agent does not exist post
				 * the answer right back to the TEE */
				if (agent_process_work(&cmd, agent_id) !=
				    TEEC_SUCCESS) {
					TC_NS_POST_SMC(&cmd);
					atomic_dec(&outstading_cmds);
					goto next_cmd;
				} else {
				/* Since the TA will be blocked at this point,
				* the number of outstanding cmds also
				* decreases in order not to force scheduling */
					atomic_dec(&outstading_cmds);
					tlogd("outstading commands %u\n",
						atomic_read(&outstading_cmds));
					/* Since we are out of band:
					* TEE requests tzdriver to do something,
					* we don't need to wakeup&remove
					* any party from the list */
					goto next_cmd;
				}
			}

			tlogd("processing answer %d\n", cmd.event_nr);
			mutex_lock(&wait_th_lock);
			list_for_each_entry_safe(we, tmp, &wait_list, list) {
				if (we->event_nr == cmd.event_nr) {
					found = 1;
					list_del(&we->list);
					break;
				}
			}
			mutex_unlock(&wait_th_lock);

			if (found) {
				tlogd("complete answer %d\n", cmd.event_nr);
				if (EOK != memcpy_s(we->cmd,
							sizeof(TC_NS_SMC_CMD),
							&cmd,
							sizeof(TC_NS_SMC_CMD)))
					tloge("memcpy_s failed,%s line:%d",
						__func__, __LINE__);
				/* If the client gave up on
				 * the completion we just ignore it ! */
				if (completion_done(&we->done)) {
					tlogd("LOST EVENT!\n");
					kfree(we);
					atomic_dec(&outstading_cmds);
				} else
					complete(&we->done);

			} else {
				tlogd("LOST EVENT id =%u!!!\n", cmd.event_nr);
			}

next_cmd:
			if (++last_out >= MAX_SMC_CMD)
				last_out = 0;

			/* Give the other processes time to run */
			cond_resched();
		}

		/* Every time we have an empty run, TrustedCore return from reet
		 * and no command was processed
		 * it means we preempted the agent thread,
		 * so back off and give it time */
		if (ret != TSP_REQUEST && cmd_processed == 0) {
			/* Give others time to run,
			 * eg agents blocking TAs blocking other TAs */
			TCVERBOSE("empty run resched %u!!!\n",
				  atomic_read(&outstading_cmds));
			cond_resched();
			cmd_count++;
		}
		/* We should freeze if no waiters or number of outstanding
		 * commands is 0:first means no CAs are active
		 * send means that CAs might be active
		 * but we are processing a agent request */

		if (ret != TSP_REQUEST
		    && (!atomic_read(&outstading_cmds))
		    && !kthread_should_stop()) {
			tlogd("going to sleep %u!!!\n",
				atomic_read(&outstading_cmds));
			if (slept)
				(void)sec_s_power_down();
			slept = true;
			(void)wait_event_interruptible(smc_th_wait,/*lint !e666*/
						 atomic_read(&smc_th_run));
			(void)sec_s_power_on();
			tlogd("wakeup %u!!!\n",
				atomic_read(&outstading_cmds));
			atomic_set(&smc_th_run, 0);/*lint !e1058*/
			i = 0;
			cmd_count = 0;
		}

		/* If too many empty runs happened consecutively
		 * then just sleep as we are in a bad state */
		if (ret != TSP_REQUEST && cmd_processed == 0
		    && cmd_count > MAX_EMPTY_RUNS) {
			tlogd("empty run sleep %u!!!\n",
				atomic_read(&outstading_cmds));
			if (slept)
				(void)sec_s_power_down();
			slept = true;
			(void)wait_event_interruptible(smc_th_wait,/*lint !e666*/
						 atomic_read(&smc_th_run));
			(void)sec_s_power_on();
			tlogd("wakeup from sleep %u!!!\n",
				atomic_read(&outstading_cmds));

			atomic_set(&smc_th_run, 0);/*lint !e1058*/
			cmd_count = 0;
		}

	}
	tlogd("smc thread stopped\n");
	if (slept)
		(void)sec_s_power_down();
	/* Wake up all the waiting threads */
	mutex_lock(&wait_th_lock);
	list_for_each_entry(we, &wait_list, list) {
		we->cmd->ret_val = TEEC_ERROR_GENERIC; /*lint !e570 */
		complete(&we->done);
	}
	mutex_unlock(&wait_th_lock);

	while (1) {
		set_current_state(TASK_INTERRUPTIBLE);/*lint !e446*/ /*lint !e666*/ /*lint !e666*/
		schedule();
	}
	return 0; /*lint !e527 */
}

static void smc_work_no_wait(struct work_struct *work)
{
	struct smc_work *s_work = container_of(work, struct smc_work, work);

	smc_send(TSP_REQUEST, cmd_phys, s_work->cmd_type, true);
}

static void smc_work_func(struct kthread_work *work)
{
	struct smc_work *s_work = container_of(work, struct smc_work, kthwork);

	if (update_timestamp(s_work->cmd)) {
		tlogd("update_timestamp failed !\n");
	}

	if (update_chksum(s_work->cmd))
		tloge("update_chksum failed.\n");
	if (EOK != memcpy_s(&cmd_data->in[last_in],
				sizeof(TC_NS_SMC_CMD),
				s_work->cmd,
				sizeof(TC_NS_SMC_CMD)))
		tloge("memcpy_s failed,%s line:%d", __func__, __LINE__);
	isb();
	wmb();
	if (++last_in >= MAX_SMC_CMD)
		last_in = 0;
	cmd_data->last_in = last_in;
	isb();
	wmb();

	tlogd("***smc_work_func in %d %d ***\n", last_in, cmd_data->last_in);
	tc_smc_wakeup();

	tlogd("***smc_work_func out %d %d ***\n", last_out,
		cmd_data->last_out);
}

static unsigned int smc_send_func(TC_NS_SMC_CMD *cmd, uint32_t cmd_type,
				  uint8_t flags)
{

	struct wait_entry *we = kmalloc(sizeof(struct wait_entry), GFP_KERNEL);
	struct smc_work work = {
		KTHREAD_WORK_INIT(work.kthwork, smc_work_func),
		.cmd = cmd,
		.cmd_type = cmd_type,
		.we = we,
	};

	if (we == NULL) {
		tloge("failed to malloc memory!\n");
		return (unsigned int)-ENOMEM;
	}
	if (!cmd) {
		TCERR("invalid cmd\n");
		kfree(we);
		return (unsigned int)-ENOMEM;
	}

	we->event_nr = cmd->event_nr;
	we->cmd = cmd;
	init_completion(&we->done);
	INIT_LIST_HEAD(&we->list);

	mutex_lock(&wait_th_lock);
	list_add_tail(&we->list, &wait_list);
	atomic_inc(&outstading_cmds);
	mutex_unlock(&wait_th_lock);

	if (!queue_kthread_work(&cmd_worker, &work.kthwork)) {
		tloge("cmd work did't queue successfully, was already pending.\n");
		kfree(we);
		atomic_dec(&outstading_cmds);
		return (unsigned int)TEEC_ERROR_GENERIC;

	}

	tlogd("***wait_thr_add waiting for completion %u ***\n",
		cmd->event_nr);
	/* We need to make sure the flush the work so
	 * it has made it all the way to the smc thread! */
	flush_kthread_work(&work.kthwork);
	/* In sync mode we don't return till we get an answer */
	wait_for_completion(&we->done);
	tlogd("***smc_send_func wait for complete done %d***\n",
		cmd->event_nr);
	atomic_dec(&outstading_cmds);

	/* We only free it if we actually finished it! */
	kfree(we);

	tlogd("***wait_thr_add waiting completion done*** %u %x\n",
		cmd->event_nr, cmd->ret_val);

	return cmd->ret_val;
}

#define HUNGTASK_LIST_LEN	11
static const char* g_hungtask_monitor_list[HUNGTASK_LIST_LEN] = {
	"system_server","fingerprintd", "atcmdserver", "keystore", "gatekeeperd",
	"volisnotd", "secure_storage", "secure_storage_s", "mediaserver",
	"vold", "tee_test_ut"};

bool is_tee_hungtask(struct task_struct *t)
{
	uint32_t i;
	if (!t)
		return false;

	for (i=0; i < HUNGTASK_LIST_LEN; i++) {
		if (!strcmp(t->comm, g_hungtask_monitor_list[i])) { /*lint !e421 */
			tloge("tee_hungtask detected:the hungtask is %s\n",t->comm);
			return true;
		}
	}

	return false;
}

void tc_smc_wakeup(void)
{
	atomic_set(&smc_th_run, 1);/*lint !e1058*/
	/* If anybody waiting then wake the smc process */
	wake_up_interruptible(&smc_th_wait);
}

void wakeup_tc_siq(void)
{
	atomic_set(&siq_th_run, 1);/*lint !e1058*/
	wake_up_interruptible(&siq_th_wait);
}

static void secs_timer_init(void)
{
	int cpu;
	unsigned int flag = ONLY_CPU0_ONLINE;

	mutex_lock(&secs_timer_lock);
	for_each_online_cpu(cpu) {/*lint -e713 */
		if (!timer_pending(&secs_timer)) {
			if (CPU0 == cpu) {
				flag = ONLY_CPU0_ONLINE;
				continue;
			} else {
				secs_timer.expires = jiffies;
				add_timer_on(&secs_timer, cpu);
				flag = OTHER_CPU_ONLINE;
				break;
			}
		}
	}
	if (ONLY_CPU0_ONLINE == flag) {
		pr_err("only cpu0 online secure clk feature disable\n");
	}
	mutex_unlock(&secs_timer_lock);
	return;
}

static void secs_timer_func(unsigned long unused_value)
{
	int ret;

	UNUSED(unused_value);
	ret = atfd_hisi_service_access_register_smc(ACCESS_REGISTER_FN_MAIN_ID, (u64)SECS_SECCLK_EN, (u64)0, ACCESS_REGISTER_FN_SUB_ID_SECS_POWER_CTRL);
	if (ret) {
		pr_err("failed to enable sec_s secclk %d\n", ret);
		return;
	} else {
		mod_timer(&secs_timer, TRIGGER_TIMER);
		return;
	}
}

static int sec_s_power_on(void)
{
	int ret = 0;

	if (of_get_property(np, "sec-s-regulator-enable", NULL)) { /*lint !e456*/
		/*power on ccs */
		if (of_get_property(np, "sec-s-power-ctrl-en", NULL)) {
			ret = clk_prepare_enable(secs_clk);
			if (ret < 0) {
				pr_err("ck_prepare_enable is failed\n");
				return ret; /*lint !e454*/
			}
		}

		if (of_get_property(np, "sec-s-power-atfd-proc", NULL)) {
			ret = atfd_hisi_service_access_register_smc(ACCESS_REGISTER_FN_MAIN_ID, (u64)SECS_POWER_UP, (u64)0, ACCESS_REGISTER_FN_SUB_ID_SECS_POWER_CTRL);
			if (ret)
				pr_err("failed to powerup sec_s %d\n", ret);
		} else {
			ret = regulator_bulk_enable(1, &regu_burning);
			if (ret)
				pr_err("failed to enable sec_s regulators %d\n", ret);
		}
	} else if (of_get_property(np, "sec-s-power-proc-en", NULL)) {
		mutex_lock(&secs_count_lock);
		if (0 == g_secs_power_ctrl_count) {
			g_secs_power_ctrl_count++;
			mutex_unlock(&secs_count_lock);
			secs_timer_init();
		} else {
			g_secs_power_ctrl_count++;
			mutex_unlock(&secs_count_lock);
		}
		ret = atfd_hisi_service_access_register_smc(ACCESS_REGISTER_FN_MAIN_ID, (u64)SECS_POWER_UP, (u64)0, ACCESS_REGISTER_FN_SUB_ID_SECS_POWER_CTRL);
		if (ret) {
			pr_err("failed to powerup sec_s %d\n", ret);
		}
	}

	return ret; /*lint !e454*/
}

static int sec_s_power_down(void)
{
	int ret = 0;

	if (of_get_property(np, "sec-s-regulator-enable", NULL)) {
		/*power down ccs */
		if (of_get_property(np, "sec-s-power-ctrl-en", NULL))
			clk_disable_unprepare(secs_clk);

			if (of_get_property(np, "sec-s-power-atfd-proc", NULL)) {
				ret = atfd_hisi_service_access_register_smc(ACCESS_REGISTER_FN_MAIN_ID, (u64)SECS_POWER_DOWN, (u64)0, ACCESS_REGISTER_FN_SUB_ID_SECS_POWER_CTRL);
				if (ret)
				pr_err("failed to powerdown sec_s %d\n", ret);
			} else {
				ret = regulator_bulk_disable(1, &regu_burning);
				if (ret)
					pr_err("failed to disable sec_s regulators %d\n", ret);
			}
	} else if (of_get_property(np, "sec-s-power-proc-en", NULL)) {
		mutex_lock(&secs_count_lock);
		g_secs_power_ctrl_count--;
		if (0 == g_secs_power_ctrl_count) {
			mutex_unlock(&secs_count_lock);
			del_timer_sync(&secs_timer);
		} else {
			mutex_unlock(&secs_count_lock);
		}
		ret = atfd_hisi_service_access_register_smc(ACCESS_REGISTER_FN_MAIN_ID, (u64)SECS_SECCLK_DIS, (u64)0, ACCESS_REGISTER_FN_SUB_ID_SECS_POWER_CTRL);
		if (ret) {
			pr_err("failed to disable sec_s secclk %d\n", ret);
			return ret;
		}

		ret = atfd_hisi_service_access_register_smc(ACCESS_REGISTER_FN_MAIN_ID, (u64)SECS_POWER_DOWN, (u64)0, ACCESS_REGISTER_FN_SUB_ID_SECS_POWER_CTRL);
		if (ret)
			pr_err("failed to powerdown sec_s %d\n", ret);
	}
	return ret;
}

/*
 * Function:     TC_NS_SMC
 * Description:   This function first power on crypto cell,
 *					then send smc cmd to trustedcore.
 *					After finished, power off crypto cell.
 * Parameters:   cmd_addr
 * Return:      0:smc cmd handled succefully
 *              0xFFFFFFFF:power on or power off failed.
 *              0xFFFFxxxx:smc cmd handler failed or cmd pending.
 */
unsigned int TC_NS_SMC(TC_NS_SMC_CMD *cmd, uint8_t flags)
{
	unsigned int ret = 0;
	int spi_ret;

	if (sys_crash) {
		tloge("ERROR: sys crash happened!!!\n");
		return (unsigned int)TEEC_ERROR_GENERIC;
	}

	if (!cmd) {
		tloge("invalid cmd\n");
		return (unsigned int)TEEC_ERROR_GENERIC;
	}

	spi_ret = spi_init_secos(0);
	if (spi_ret) {
		pr_err("spi0 for spi_init_secos error : %d\n", spi_ret);
		goto spi_err;
	}
	spi_ret = spi_init_secos(1);
	if (spi_ret) {
		pr_err("spi1 for spi_init_secos error : %d\n", spi_ret);
		goto spi_err;
	}

	tlogd(KERN_INFO "***TC_NS_SMC call start on cpu %d ***\n",
		raw_smp_processor_id());
	/* TODO
	 * directily call smc_send on cpu0 will call SMP PREEMPT error,
	 * so just use thread to send smc
	 */
	cmd->event_nr = atomic_add_return(1, &event_nr);
    /*Here should add comment*/
	mb();

	ret = smc_send_func(cmd, TC_NS_CMD_TYPE_NS_TO_SECURE, flags);

	spi_ret = spi_exit_secos(0);
	if (spi_ret) {
		pr_err("spi0 for spi_exit_secos error : %d\n", spi_ret);
		goto spi_err;
	}
	spi_ret = spi_exit_secos(1);
	if (spi_ret) {
		pr_err("spi1 for spi_exit_secos error : %d\n", spi_ret);
		goto spi_err;
	}

spi_err:
	return ret;
}

unsigned int TC_NS_SMC_WITH_NO_NR(TC_NS_SMC_CMD *cmd, uint8_t flags)
{
	unsigned int ret = 0;
	int spi_ret;

	if (sys_crash)
		return TEEC_ERROR_GENERIC; /*lint !e570 */

	if (!cmd) {
		tloge("invalid cmd\n");
		return TEEC_ERROR_GENERIC; /*lint !e570 */
	}

	spi_ret = spi_init_secos(0);
	if (spi_ret) {
		pr_err("spi0 for spi_init_secos error : %d\n", spi_ret);
		goto spi_err;
	}
	spi_ret = spi_init_secos(1);
	if (spi_ret) {
		pr_err("spi1 for spi_init_secos error : %d\n", spi_ret);
		goto spi_err;
	}

	tlogd(KERN_INFO "***TC_NS_SMC call start on cpu %d ***\n",
		raw_smp_processor_id());
	/* TODO
	 * directily call smc_send on cpu0 will call SMP PREEMPT error,
	 * so just use thread to send smc
	 */
	mb();

	ret = smc_send_func(cmd, TC_NS_CMD_TYPE_NS_TO_SECURE, flags);

	spi_ret = spi_exit_secos(0);
	if (spi_ret) {
		pr_err("spi0 for spi_exit_secos error : %d\n", spi_ret);
		goto spi_err;
	}
	spi_ret = spi_exit_secos(1);
	if (spi_ret) {
		pr_err("spi1 for spi_exit_secos error : %d\n", spi_ret);
		goto spi_err;
	}

spi_err:
	return ret;
}

unsigned int TC_NS_POST_SMC(TC_NS_SMC_CMD *cmd)
{
	struct smc_work work = {
		KTHREAD_WORK_INIT(work.kthwork, smc_work_func),
		.cmd = cmd,
		.cmd_type = TC_NS_CMD_TYPE_NS_TO_SECURE,
		.we = NULL
	};
	if (sys_crash)
		return TEEC_ERROR_GENERIC; /*lint !e570 */

	if (!cmd) {
		tloge("invalid cmd\n");
		return TEEC_ERROR_GENERIC; /*lint !e570 */
	}

	atomic_inc(&outstading_cmds);
	if (!queue_kthread_work(&cmd_worker, &work.kthwork))
		return TEEC_ERROR_GENERIC; /*lint !e570 */

	/* We need to make sure the flush the work
	 * so it has made it all the way to the smc thread! */
	flush_kthread_work(&work.kthwork);
	tlogd("Command posted %u\n", cmd->event_nr);

	return TEEC_SUCCESS;
}

static int smc_set_cmd_buffer(void)
{
	struct smc_work work = {
		.cmd = NULL,
		.cmd_type = TC_NS_CMD_TYPE_SECURE_CONFIG
	};
	INIT_WORK_ONSTACK(&work.work, smc_work_no_wait);
	/* Run work on CPU 0 */
	schedule_work_on(0, &work.work);
	flush_work(&work.work);
	return 0;
}

static int get_session_root_key(void)
{
	int ret;
	uint32_t *buffer = (uint32_t *)(cmd_data->in);
	uint32_t tee_crc = 0;
	uint32_t crc = 0;
	if (!buffer || ((uint64_t)buffer & 0x3)) {
		tloge("Session root key must be 4bytes aligned.\n");
		return -EFAULT;
	}

	g_session_root_key = kzalloc(sizeof(struct session_crypto_info),
				     GFP_KERNEL);
	if (!g_session_root_key) {
		tloge("No memory to store session root key.\n");
		return -ENOMEM;
	}

	tee_crc = *buffer;
	if (memcpy_s(g_session_root_key,
		     sizeof(struct session_crypto_info),
		     (void *)(buffer + 1),
		     sizeof(struct session_crypto_info))) {
		tloge("Copy session root key from TEE failed.\n");
		ret = -EFAULT;
		goto free_mem;
	}

	crc = crc32(0, (void *)g_session_root_key,
		    sizeof(struct session_crypto_info));
	if (crc != tee_crc) {
		tloge("Session root key has been modified.\n");
		ret = -EFAULT;
		goto free_mem;
	}

	if (memset_s((void *)(cmd_data->in), sizeof(cmd_data->in),
		     0, sizeof(cmd_data->in))) {
		tloge("Clean the command buffer failed.\n");
		ret = -EFAULT;
		goto free_mem;
	}

	return 0;

free_mem:
	kfree(g_session_root_key);
	g_session_root_key = NULL;
	return ret;
}
extern char __cfc_rules_start[];
extern char __cfc_rules_stop[];
extern char __cfc_area_start[];
extern char __cfc_area_stop[];
unsigned int *cfc_seqlock;

static void smc_set_cfc_info(void)
{
	struct cfc_rules_pos {
		u64 magic;
		u64 address;
		u64 size;
		u64 cfc_area_start;
		u64 cfc_area_stop;
	} __attribute__((packed)) *buffer = (void *)(cmd_data->out);

	buffer->magic = 0xCFC00CFC00CFCCFC;
	buffer->address = virt_to_phys(__cfc_rules_start);
	buffer->size = (void *)__cfc_rules_stop - (void *)__cfc_rules_start;
	buffer->cfc_area_start = virt_to_phys(__cfc_area_start);
	buffer->cfc_area_stop = virt_to_phys(__cfc_area_stop);
	cfc_seqlock = (u32 *)__cfc_rules_start;
}

/* Sync with trustedcore_src TEE/cfc.h */
enum cfc_info_to_linux {
       CFC_TO_LINUX_IS_ENABLED = 0xCFC0CFC1,
       CFC_TO_LINUX_IS_DISABLED = 0xCFC0CFC2,
};

/* default is false */
bool cfc_is_enabled;

static void smc_get_cfc_info(void)
{
	uint32_t cfc_flag = ((uint32_t *)(cmd_data->out))[0];

	if (cfc_flag == (uint32_t) CFC_TO_LINUX_IS_ENABLED)
		cfc_is_enabled = true;
}


#define compile_time_assert(cond, msg) \
    typedef char ASSERT_##msg[(cond) ? 1 : -1]
compile_time_assert(sizeof(TC_NS_SMC_QUEUE) <= PAGE_SIZE,
		size_of_TC_NS_SMC_QUEUE_too_large);
int smc_init_data(struct device *class_dev)
{
	int ret = 0;

	atomic_set(&event_nr, 0);/*lint !e1058*/
	atomic_set(&outstading_cmds, 0);/*lint !e1058*/

	if (NULL == class_dev || IS_ERR(class_dev))
		return -ENOMEM;

	cmd_data = (TC_NS_SMC_QUEUE *) __get_free_page(GFP_KERNEL | __GFP_ZERO);
	if (!cmd_data)
		return -ENOMEM;

	smc_set_cfc_info();

	cmd_phys = virt_to_phys(cmd_data);

	/* Send the allocated buffer to TrustedCore for init */
	if (smc_set_cmd_buffer()) {
		ret = -EINVAL;
		goto free_mem;
	}
	if (get_session_root_key()) {
		ret = -EFAULT;
		goto free_mem;
	}
	smc_get_cfc_info();
	if (of_get_property(np, "sec-s-regulator-enable", NULL)) {
		regu_burning.supply = "sec-s-buring";
		ret = devm_regulator_bulk_get(class_dev, 1, &regu_burning);
		if (ret) {
			dev_err(class_dev,
				"couldn't get sec_burning regulator %d\n", ret);
			goto free_mem;
		} else {
			pr_info("get sec_s_buring regulator success!\n");
		}
		if (of_get_property(np, "sec-s-power-ctrl-en", NULL)) {
			secs_clk = devm_clk_get(class_dev, "clk_secs");
			if (IS_ERR(secs_clk)) {
			    dev_err(class_dev,
				    "couldn't get sec_s clk!\n");
			    ret = -ENODEV;
			    goto free_mem;
			} else
			    pr_info("get secs_clk success\n");
		}
	}

	smc_thread = kthread_create_on_node(smc_thread_fn,
					    NULL, cpu_to_node(0),
					    "smcthread/%d", 0);

	if (unlikely(IS_ERR(smc_thread))) {
		dev_err(class_dev, "couldn't create smcthread %ld\n",
			PTR_ERR(smc_thread));
		ret = PTR_ERR(smc_thread);
		goto free_mem;
	}

	kthread_bind(smc_thread, 0);

	siq_thread = kthread_create_on_node(siq_thread_fn,
					    NULL, cpu_to_node(0),
					    "siqthread/%d", 0);

	if (unlikely(IS_ERR(siq_thread))) {
		dev_err(class_dev, "couldn't create siqthread %ld\n",
			PTR_ERR(siq_thread));
		ret = (int)PTR_ERR(siq_thread);
		goto free_smc_worker;
	}

	kthread_bind(siq_thread, 0);

	cmd_work_thread =
	    kthread_create(kthread_worker_fn, &cmd_worker, "tcwork");
	if (IS_ERR(cmd_work_thread)) {
		dev_err(class_dev, "couldn't create cmd_work_threads %ld\n",
			PTR_ERR(cmd_work_thread));
		ret = (int)PTR_ERR(cmd_work_thread);
		goto free_siq_worker;
	}

	wake_up_process(cmd_work_thread);
	wake_up_process(smc_thread);
	wake_up_process(siq_thread);

	return 0;

free_siq_worker:
	kthread_stop(siq_thread);
	siq_thread = NULL;

free_smc_worker:
	kthread_stop(smc_thread);
	smc_thread = NULL;

free_mem:
	free_page((unsigned long)cmd_data);
	cmd_data = NULL;
	if (!IS_ERR_OR_NULL(g_session_root_key)) {
		kfree(g_session_root_key);
		g_session_root_key = NULL;
	}
	return ret;
}

/*lint -e838*/
int teeos_log_exception_archive(void)
{
	int ret;
	struct imonitor_eventobj *teeos_obj;
	teeos_obj = imonitor_create_eventobj(IMONITOR_TEEOS_EVENT_ID);

	ret = imonitor_set_param(teeos_obj, E901002003_PNAME_VARCHAR, (long)"teeos");
	if (0 != ret)
		tloge("imonitor_set_param failed\n");

	ret = imonitor_add_dynamic_path(teeos_obj, "/data/hisi_logs/running_trace/teeos_logs");
	if (0 != ret) {
		tloge("add path  failed\n");
		imonitor_destroy_eventobj(teeos_obj);
		return ret;
	}
	ret = imonitor_add_dynamic_path(teeos_obj, "/data/sec_storage_data/log");
	if (0 != ret) {
		tloge("add path  failed\n");
		imonitor_destroy_eventobj(teeos_obj);
		return ret;
	}
	ret = imonitor_send_event(teeos_obj);
	imonitor_destroy_eventobj(teeos_obj);
	return ret;
}

void smc_free_data(void)
{
	free_page((unsigned long)cmd_data);

	if (!IS_ERR_OR_NULL(cmd_work_thread)) {
		kthread_stop(cmd_work_thread);
		cmd_work_thread = NULL;
	}
	if (!IS_ERR_OR_NULL(smc_thread)) {
		kthread_stop(smc_thread);
		smc_thread = NULL;
	}
	if (!IS_ERR_OR_NULL(g_session_root_key)) {
		kfree(g_session_root_key);
		g_session_root_key = NULL;
	}
}
/*lint -restore*/
