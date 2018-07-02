/*copyright (c) Huawei Technologies Co., Ltd. 1998-2014. All rights reserved.
 *
 * File name: pid_socket.c
 * Description: This file use to record pid and socket
 * Author: xishiying@huawei.com
 * Version: 0.1
 * Date:  2014/11/27
 */

#include <net/tcp.h>

#include <linux/compiler.h>
#include <linux/gfp.h>
#include <linux/module.h>


#include <linux/fs.h>
#ifdef CONFIG_HW_CROSSLAYER_OPT
#ifdef CONFIG_HW_CROSSLAYER_OPT_DBG
#include <linux/time.h>
#include <linux/timex.h>
#include <linux/rtc.h>
#endif 
#include <net/tcp_crosslayer.h>
#endif 

#ifdef CONFIG_HW_WIFIPRO
#include <huawei_platform/net/ipv4/wifipro_tcp_monitor.h>
#endif
#ifdef CONFIG_HW_WIFI
#include <huawei_platform/net/ipv4/wifi_tcp_statistics.h>
#endif

#include "pid_socket.h"


/*20150114 add,get pid and process name of the app who used connect function.*/
int get_proc_name(int pid, char *buf, int len)
{
	char filename[32];
	struct file *filep;
	int result = 0;
	mm_segment_t oldfs;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	snprintf(filename, sizeof(filename), "/proc/%d/cmdline", pid);
	filep = filp_open(filename, O_RDONLY, 0);
	if (IS_ERR(filep)) {
		pr_err("get_proc_name:open file error\n");
		result = -EINVAL;
		set_fs(oldfs);
		return -1;
	}

	filep->f_pos = 0;
	result = filep->f_op->read(filep, buf, len, &filep->f_pos);
	filp_close(filep, NULL);
	set_fs(oldfs);
	return 0;
}


void print_process_pid_name(struct inet_sock *inet)
{
	char process_name[100] = {0};
	int pid = task_tgid_vnr(current);
	unsigned short source_port = inet->inet_sport;

	get_proc_name(pid, process_name, 99);
	source_port = htons(source_port);
	pr_crit("%s: pid:(%d),name:(%s),port:(%d)\n", __func__, pid, process_name, source_port);
}

/*20150114 add,get pid and process name of the app who used connect function.*/