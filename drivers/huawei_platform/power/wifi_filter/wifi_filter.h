#ifndef __WIFI_FILTER_H__
#define __WIFI_FILTER_H__

/******************************************************************************

				  版权所有 (C), 2017, 华为技术有限公司

 ******************************************************************************
  文 件 名   : wifi_filter.h
  版 本 号   : 初稿
  作	者   : z00220931
  生成日期   : 2017年04月26日
  最近修改   :
  功能描述   : doze模式下wifi芯片过滤冗余包
  函数列表   :
  修改历史   :
  1.日	期   : 2017年04月26日
	作	者   : z00220931
	修改内容   : 创建文件
******************************************************************************/
#include <net/ip.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter_ipv4/ip_tables.h>
typedef struct {
    unsigned short protocol;    //协议类型
    unsigned short port;  //目的端口号
    unsigned int filter_cnt;    //过滤报文数
//    unsigned int uid;
} hw_wifi_filter_item;

struct hw_wlan_filter_ops{
    int (*set_filter_enable)(int);
    int (*add_filter_items)(hw_wifi_filter_item*, int);
    int (*clear_filters)(void);
    int (*get_filter_pkg_stat)(hw_wifi_filter_item*, int, int*);
};


/******************************************************************************
   2 函数声明
******************************************************************************/
/*---------driver----------*/
int hw_register_wlan_filter(struct hw_wlan_filter_ops *ops);

int hw_unregister_wlan_filter(void);

/*---------wl_android----------*/

int hw_set_net_filter_enable(int enable);
/*-----kernel----*/
void get_filter_info(
    struct sk_buff *skb,
    const struct nf_hook_state *state,
    unsigned int hook,
    const struct xt_table_info *private,
    const struct ipt_entry *e);

#endif

