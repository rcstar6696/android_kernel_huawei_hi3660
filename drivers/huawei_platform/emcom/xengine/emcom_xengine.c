#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/time.h>
#include <net/sock.h>
#include <net/tcp.h>
#include <net/ip.h>
#include <net/netlink.h>
#include <linux/skbuff.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <uapi/linux/netlink.h>
#include <linux/kthread.h>
#include <linux/netdevice.h>
#include <linux/spinlock.h>
#include <net/pkt_sched.h>
#include <net/sch_generic.h>
#include "../emcom_netlink.h"
#include "../emcom_utils.h"

#ifdef CONFIG_HUAWEI_BASTET
#include <huawei_platform/power/bastet/bastet_utils.h>
#endif
#include <huawei_platform/emcom/emcom_xengine.h>
#include <linux/version.h>


#undef HWLOG_TAG
#define HWLOG_TAG emcom_xengine
HWLOG_REGIST();
MODULE_LICENSE("GPL");


#define     EMCOM_MAX_ACC_APP  (5)
#define     EMCOM_UID_ACC_AGE_MAX  (1000)

#define     EMCOM_SPEED_CTRL_BASE_WIN_SIZE   (10000)


#ifdef CONFIG_HUAWEI_BASTET_COMM
	extern int bastet_comm_keypsInfo_write(uint32_t ulState);
#endif

struct Emcom_Xengine_acc_app_info     g_CurrentUids[EMCOM_MAX_ACC_APP];
struct Emcom_Xengine_speed_ctrl_info  g_SpeedCtrlInfo;

struct sk_buff_head g_UdpSkbList;
struct timer_list   g_UdpSkb_timer;
uid_t  g_UdpRetranUid;
bool   g_Emcom_udptimerOn = false;

#define EMCOM_UDPRETRAN_NODELAY
#define UDPTIMER_DELAY  (4)
#define EMCOM_MAX_UDP_SKB  (20)
#define MIN_JIFFIE         1
struct Emcom_Xengine_netem_skb_cb {
	psched_time_t    time_to_send;
	ktime_t          tstamp_save;
};

struct mutex g_Mpip_mutex;
uid_t   g_MpipUids[EMCOM_MAX_MPIP_APP];/* The uid of bind to Mpip Application */
bool    g_MpipStart               = false;/* The uid of bind to Mpip Application */
char    g_Ifacename[IFNAMSIZ]     = {0};/* The uid of bind to Mpip Application */


void Emcom_Xengine_Mpip_Init(void);
/******************************************************************************
   6 函数实现
******************************************************************************/
static inline bool invalid_uid(uid_t uid)
{
	/* if uid less than 10000, it is not an Android apk */
	return (uid < UID_APP);
}

static inline bool invalid_SpeedCtrlSize(uint32_t grade)
{
	/* the speed control grade bigger than 10000 */
	return (grade < EMCOM_SPEED_CTRL_BASE_WIN_SIZE);
}


/*****************************************************************************
 函 数 名  : Emcom_Xengine_netem_skb_cb
 功能描述  :获取SKB的时间控制块
 输入参数  : struct sk_buff *skb
 输出参数  : struct Emcom_Xengine_netem_skb_cb *
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
        1.日    期   : 2017年09月9日
           作    者   : l00301519
           修改内容   : 新生成函数
*****************************************************************************/
static inline struct Emcom_Xengine_netem_skb_cb *Emcom_Xengine_netem_skb_cb(struct sk_buff *skb)
{
	/* we assume we can use skb next/prev/tstamp as storage for rb_node */
	qdisc_cb_private_validate(skb, sizeof(struct Emcom_Xengine_netem_skb_cb));
	return (struct Emcom_Xengine_netem_skb_cb *)qdisc_skb_cb(skb)->data;
}

/*****************************************************************************
 函 数 名  : Emcom_Xengine_setUdpTimerCb
 功能描述  :设置skb的用于定时控制的控制块
 输入参数  : struct sk_buff *skb
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
        1.日    期   : 2017年09月9日
           作    者   : l00301519
           修改内容   : 新生成函数
*****************************************************************************/
static void Emcom_Xengine_setUdpTimerCb(struct sk_buff *skb)
{
	struct Emcom_Xengine_netem_skb_cb *cb;
	unsigned long now;
	now = jiffies;
	cb = Emcom_Xengine_netem_skb_cb(skb);
	/* translate to jiffies */
	cb->time_to_send = now + UDPTIMER_DELAY*HZ/MSEC_PER_SEC;
}

/*****************************************************************************
 函 数 名  : Emcom_Xengine_udpretran_clear
 功能描述  :清理UDP重传队列和定时器
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
        1.日    期   : 2017年09月9日
           作    者   : l00301519
           修改内容   : 新生成函数
*****************************************************************************/
int Emcom_Xengine_udpretran_clear(void)
{
	g_UdpRetranUid = UID_INVALID_APP;
	skb_queue_purge(&g_UdpSkbList);
	if(g_Emcom_udptimerOn)
	{
		del_timer(&g_UdpSkb_timer);
		g_Emcom_udptimerOn = false;
	}
	return 0;
}

/*****************************************************************************
 函 数 名  : Emcom_Xengine_UdpTimer_handler
 功能描述  :UDP重传定时器处理函数
 输入参数  : unsigned long pac
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
        1.日    期   : 2017年09月7日
           作    者   : l00301519
           修改内容   : 新生成函数
*****************************************************************************/
static void Emcom_Xengine_UdpTimer_handler(unsigned long pac)
{
	struct sk_buff *skb;
	unsigned long now;
	struct Emcom_Xengine_netem_skb_cb *cb;
	int jiffie_n;

	/* anyway, send out the first skb */
    if(!skb_queue_empty(&g_UdpSkbList))
	{
		skb = skb_dequeue(&g_UdpSkbList);
		dev_queue_xmit(skb);
		EMCOM_LOGD("Emcom_Xengine_UdpTimer_handler send skb\n");
	}

	skb = skb_peek(&g_UdpSkbList);
	if(!skb)
	{
		goto timer_off;
		return;
	}
	cb = Emcom_Xengine_netem_skb_cb(skb);
	now = jiffies;
	/* if remaining time is little than 1 jiffie, send out */
	while(cb->time_to_send <= now + MIN_JIFFIE)
	{
		EMCOM_LOGD("Emcom_Xengine_UdpTimer_handler send another skb\n");
		skb = skb_dequeue(&g_UdpSkbList);
		dev_queue_xmit(skb);
		skb = skb_peek(&g_UdpSkbList);
		if(!skb)
		{
			goto timer_off;
			return;
		}
		cb = Emcom_Xengine_netem_skb_cb(skb);
		now = jiffies;
	}
	/* set timer based on next skb cb */
	now = jiffies;
	jiffie_n = cb->time_to_send - now;

	if(jiffie_n < MIN_JIFFIE)
	{
		jiffie_n = MIN_JIFFIE;
	}
	EMCOM_LOGD("Emcom_Xengine_UdpTimer_handler modify timer hz %d\n", jiffie_n);
	mod_timer(&g_UdpSkb_timer, jiffies + jiffie_n);
	g_Emcom_udptimerOn = true;
	return;

timer_off:
	g_Emcom_udptimerOn = false;
}

/*****************************************************************************
 函 数 名  : Emcom_Xengine_Init
 功能描述  : 初始化快抢技术相关结构体
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
	1.日	期   : 2017年03月13日
	  作	者   : z00371705
	  修改内容   : 新生成函数
*****************************************************************************/
void Emcom_Xengine_Init(void)
{
	uint8_t  index;
	for( index = 0; index < EMCOM_MAX_ACC_APP; index ++)
	{
		g_CurrentUids[index].lUid = UID_INVALID_APP;
		g_CurrentUids[index].ulAge = 0;
	}
	g_SpeedCtrlInfo.lUid = UID_INVALID_APP;
	g_SpeedCtrlInfo.ulSize = 0;
	spin_lock_init(&g_SpeedCtrlInfo.stLocker);
	g_UdpRetranUid = UID_INVALID_APP;
	g_Emcom_udptimerOn = false;
	skb_queue_head_init(&g_UdpSkbList);
	init_timer(&g_UdpSkb_timer);
	g_UdpSkb_timer.function = Emcom_Xengine_UdpTimer_handler;
	mutex_init(&g_Mpip_mutex);
	Emcom_Xengine_Mpip_Init();
}

/*****************************************************************************
 函 数 名  : Emcom_Xengine_Mpip_Init
 功能描述  : 初始化使用Mpip的APP列表
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
	1.日	期   : 2017年09月05日
	  作	者   : z00196795
	  修改内容   : 新生成函数
*****************************************************************************/
void Emcom_Xengine_Mpip_Init(void)
{
	uint8_t  uIndex;
	mutex_lock(&g_Mpip_mutex);
	for( uIndex = 0; uIndex < EMCOM_MAX_MPIP_APP; uIndex ++)
	{
		g_MpipUids[uIndex] = UID_INVALID_APP;
	}
	mutex_unlock(&g_Mpip_mutex);
}

/*****************************************************************************
 函 数 名  : emcom_process_clear
 功能描述  : 处理上层下发的清除消息
 输入参数  :无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
    1.日    期   : 2016年12月07日
       作    者   : z00371705
       修改内容   : 新生成函数

*****************************************************************************/
bool Emcom_Xengine_IsAccUid(uid_t lUid)
{
	uint8_t  index;
	for( index = 0; index < EMCOM_MAX_ACC_APP; index ++)
	{
		if( lUid == g_CurrentUids[index].lUid )
		{
			return true;
		}
	}

	return false;
}

#if defined(CONFIG_PPPOLAC) || defined(CONFIG_PPPOPNS)
/*****************************************************************************
 函 数 名  : BST_FG_Hook_Ul_Stub
 功能描述  : 勾取上行数据包
 输入参数  : struct sock *pstSock  socket对象
                           struct msghdr *msg   发送的消息结构体
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
    1.日    期   : 2015年12月13日
      作    者   : z00371705
      修改内容   : 新生成函数
*****************************************************************************/

bool Emcom_Xengine_Hook_Ul_Stub(struct sock *pstSock)
{
	uid_t lSockUid = 0;
	bool  bFound   = false;

	if(( NULL == pstSock ) )
	{
		EMCOM_LOGD("Emcom_Xengine_Hook_Ul_Stub param invalid\n");
		return false;
	}

	/**
	 * if uid equals current acc uid, accelerate it,else stop it
	 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 10)
	lSockUid = sock_i_uid(pstSock).val;
#else
	lSockUid = sock_i_uid(pstSock);
#endif

	if( invalid_uid ( lSockUid ))
	{
		return false;
	}

	bFound = Emcom_Xengine_IsAccUid ( lSockUid );

	return bFound;
}
#endif


/*****************************************************************************
 函 数 名  : emcom_process_clear
 功能描述  : 处理上层下发的清除消息
 输入参数  :无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
    1.日    期   : 2016年12月07日
       作    者   : z00371705
       修改内容   : 新生成函数

*****************************************************************************/
int Emcom_Xengine_clear(void)
{
	uint8_t  index;
	for( index = 0; index < EMCOM_MAX_ACC_APP; index ++)
	{
		g_CurrentUids[index].lUid = UID_INVALID_APP;
		g_CurrentUids[index].ulAge = 0;
	}
	mutex_lock(&g_Mpip_mutex);
	for( index = 0; index < EMCOM_MAX_MPIP_APP; index ++)
	{
		g_MpipUids[index] = UID_INVALID_APP;
	}
	memset(g_Ifacename, 0, sizeof(char)*IFNAMSIZ);
	g_MpipStart = false;
	mutex_unlock(&g_Mpip_mutex);
	Emcom_Xengine_udpretran_clear();
	return 0;
}

/*****************************************************************************
 函 数 名  : BST_FG_StartAccUid
 功能描述  : 启动app加速
 输入参数  : uint8_t *pdata  加速的app信息
                           uint16_t len 数据长度
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
        1.日    期   : 2016年11月18日
           作    者   : z00371705
           修改内容   : 新生成函数
*****************************************************************************/

int Emcom_Xengine_StartAccUid(uint8_t *pdata, uint16_t len)
{
	uid_t              uid;
	uint8_t            index;
	uint8_t            ucIdleIndex;
	uint8_t            ucOldIndex;
	uint8_t            ucOldAge;
	bool               bFound;
	/*input param check*/
	if( NULL == pdata )
	{
		EMCOM_LOGE("Emcom_Xengine_StartAccUid:data is null");
		return -EINVAL;
	}

	/*check len is invalid*/
	if(len != sizeof(uid_t))
	{
		EMCOM_LOGI("Emcom_Xengine_StartAccUid: len:%d is illegal", len);
		return -EINVAL;
	}

	uid =*(uid_t *)pdata;

	/*check uid*/
	if (invalid_uid(uid))
		return -EINVAL;

	EMCOM_LOGD("Emcom_Xengine_StartAccUid: uid:%d ready to added", uid);
	ucIdleIndex = EMCOM_MAX_ACC_APP;
	ucOldIndex  = EMCOM_MAX_ACC_APP;
	ucOldAge    = 0;
	bFound  = false;

	/*check whether has the same uid, and  record the first idle position and the oldest position*/
	for( index = 0; index < EMCOM_MAX_ACC_APP; index ++)
	{
		if( UID_INVALID_APP == g_CurrentUids[index].lUid )
		{
			if( EMCOM_MAX_ACC_APP == ucIdleIndex )
			{
				ucIdleIndex  = index;
			}
		}
		else if( uid == g_CurrentUids[index].lUid )
		{
			g_CurrentUids[index].ulAge = 0;
			bFound = true;
		}
		else
		{
			g_CurrentUids[index].ulAge ++;
			if( g_CurrentUids[index].ulAge > ucOldAge )
			{
				ucOldAge    = g_CurrentUids[index].ulAge;
				ucOldIndex  = index ;
			}

		}
	}

	/*remove the too old acc uid*/
	if(ucOldAge  > EMCOM_UID_ACC_AGE_MAX )
	{
		EMCOM_LOGD("Emcom_Xengine_StartAccUid: uid:%d added too long, remove it", g_CurrentUids[ucOldIndex].lUid );
		g_CurrentUids[ucOldIndex].ulAge = 0;
		g_CurrentUids[ucOldIndex].lUid  = UID_INVALID_APP;
	}

	EMCOM_LOGD("Emcom_Xengine_StartAccUid: ucIdleIndex=%d,ucOldIndex=%d,ucOldAge=%d",ucIdleIndex, ucOldIndex,ucOldAge);

	/*if has already added, return*/
	if(bFound)
	{
		EMCOM_LOGD("Emcom_Xengine_StartAccUid: uid:%d already added", uid);
		return 0;
	}

	/*if it is new uid, and has idle position , add it*/
	if( ucIdleIndex < EMCOM_MAX_ACC_APP )
	{
		EMCOM_LOGD("Emcom_Xengine_StartAccUid: uid:%d added", uid);
		g_CurrentUids[ucIdleIndex].ulAge = 0;
		g_CurrentUids[ucIdleIndex].lUid = uid;
		return 0;
	}


	/*if it is new uid, and acc list if full , replace the oldest*/
	if( ucOldIndex < EMCOM_MAX_ACC_APP )
	{
		EMCOM_LOGD("Emcom_Xengine_StartAccUid: uid:%d replace the oldest uid:%d", uid,g_CurrentUids[ucOldIndex].lUid);
		g_CurrentUids[ucOldIndex].ulAge = 0;
		g_CurrentUids[ucOldIndex].lUid = uid;
		return 0;
	}

	return 0;
}


/*****************************************************************************
 函 数 名  : Emcom_Xengine_StopAccUid
 功能描述  : 停止APP加速
 输入参数  : uint8_t *pdata  停止加速的app信息
                           uint16_t len 数据长度
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
        1.日    期   : 2016年11月18日
           作    者   : z00371705
           修改内容   : 新生成函数
*****************************************************************************/

int Emcom_Xengine_StopAccUid(uint8_t *pdata, uint16_t len)
{
	uid_t              uid;
	uint8_t            index;

	/*input param check*/
	if( NULL == pdata )
	{
		EMCOM_LOGE("Emcom_Xengine_StopAccUid:data is null");
		return -EINVAL;
	}

	/*check len is invalid*/
	if(len != sizeof(uid_t))
	{
		EMCOM_LOGI("Emcom_Xengine_StopAccUid: len：%d is illegal", len);
		return -EINVAL;
	}

	uid =*(uid_t *)pdata;

	/*check uid*/
	if (invalid_uid(uid))
		return -EINVAL;

	/*remove specify uid*/
	for( index = 0; index < EMCOM_MAX_ACC_APP; index ++)
	{
		if( uid == g_CurrentUids[index].lUid )
		{
			g_CurrentUids[index].ulAge = 0;
			g_CurrentUids[index].lUid  = UID_INVALID_APP;
			EMCOM_LOGD("Emcom_Xengine_StopAccUid:lUid:%d",uid);
			break;
		}
	}

	return 0;
}

/*****************************************************************************
 函 数 名  : Emcom_Xengine_SetSpeedCtrlInfo
 功能描述  : 设置不控速应用的uid
 输入参数  : uint8_t *pdata  前台不控速应用的uid
             uint16_t len    数据长度
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
        1.日    期   : 2017年09月07日
           作    者   : y00368747
           修改内容   : 新生成函数
*****************************************************************************/
int Emcom_Xengine_SetSpeedCtrlInfo(uint8_t *pdata, uint16_t len)
{
	struct Emcom_Xengine_speed_ctrl_data* pSpeedCtrlInfo;
	uid_t              lUid;
	uint32_t           ulSize;

	/*input param check*/
	if( NULL == pdata )
	{
		EMCOM_LOGE("Emcom_Xengine_SetSpeedCtrlInfo:data is null");
		return -EINVAL;
	}

	/*check len is invalid*/
	if(len != sizeof(struct Emcom_Xengine_speed_ctrl_data))
	{
		EMCOM_LOGI("Emcom_Xengine_SetSpeedCtrlInfo: len:%d is illegal", len);
		return -EINVAL;
	}

	pSpeedCtrlInfo = (struct Emcom_Xengine_speed_ctrl_data *)pdata;
	lUid = pSpeedCtrlInfo->lUid;
	ulSize = pSpeedCtrlInfo->ulSize;

	/*check uid*/
	if (invalid_uid(lUid))
	{
		EMCOM_LOGI("Emcom_Xengine_SetSpeedCtrlInfo: uid:%d is illegal", lUid);
		return -EINVAL;
	}
	/*if size is zero, clear the speed control info */
	if(!ulSize)
	{
		EMCOM_LOGD("Emcom_Xengine_SetSpeedCtrlInfo: clear speed ctrl state");
		lUid = UID_INVALID_APP;
		ulSize = 0;
		EMCOM_XENGINE_SetSpeedCtrl(g_SpeedCtrlInfo, lUid, ulSize);
		return 0;
	}
	/*check size*/
	if (invalid_SpeedCtrlSize(ulSize))
	{
		EMCOM_LOGI("Emcom_Xengine_SetSpeedCtrlInfo: size:%d is illegal", ulSize);
		return -EINVAL;
	}

	EMCOM_LOGD("Emcom_Xengine_SetSpeedCtrlInfo: uid:%d size:%d", lUid, ulSize);
	EMCOM_XENGINE_SetSpeedCtrl(g_SpeedCtrlInfo, lUid, ulSize);
	return 0;
}


/*****************************************************************************
 函 数 名  : Emcom_Xengine_SpeedCtrl_WinSize
 功能描述  : 返回TCP接收窗口大小
 输入参数  : sock *pstSock  sk指针
            uint32_t*       win 原始窗口大小
 输出参数  : 无
 返 回 值  : 窗口大小
 调用函数  :
 被调函数  :
 修改历史  :
        1.日    期   : 2017年09月07日
           作    者   : y00368747
           修改内容   : 新生成函数
*****************************************************************************/
void Emcom_Xengine_SpeedCtrl_WinSize(struct sock *pstSock, uint32_t *pstSize)
{
	uid_t lSockUid = 0;
	uid_t lUid = 0;
	uint32_t ulSize = 0;

	if( NULL == pstSock )
	{
		EMCOM_LOGD("Emcom_Xengine_Hook_Ul_Stub param invalid\n");
		return;
	}

	if( NULL == pstSize )
	{
		EMCOM_LOGD(" Emcom_Xengine_SpeedCtrl_WinSize window size invalid\n");
		return;
	}

	EMCOM_XENGINE_GetSpeedCtrlUid(g_SpeedCtrlInfo, lUid);
	if( invalid_uid ( lUid ))
	{
		return;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 10)
	lSockUid = sock_i_uid(pstSock).val;
#else
	lSockUid = sock_i_uid(pstSock);
#endif

	if( invalid_uid ( lSockUid ))
	{
		return;
	}

	EMCOM_XENGINE_GetSpeedCtrlInfo(g_SpeedCtrlInfo, lUid, ulSize);
	/* check uid */
	if( lSockUid == lUid)
	{
		return;
	}

	if (ulSize)
	{
		*pstSize = g_SpeedCtrlInfo.ulSize < *pstSize ? g_SpeedCtrlInfo.ulSize : *pstSize;
	}

}


/*****************************************************************************
 函 数 名  : Emcom_Xengine_Config_MPIP
 功能描述  : 启动app加速
 输入参数  : uint8_t *pdata  绑定的app UID列表信息
             uint16_t len 数据长度
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
        1.日    期   : 2017年09月05日
           作    者   : z00196795
           修改内容   : 新生成函数
*****************************************************************************/

int Emcom_Xengine_Config_MPIP(uint8_t *pdata, uint16_t len)
{
	uint8_t            uIndex;
	uint8_t            *ptemp;
	uint8_t            ulength;
	/*The empty updated list means clear the Mpip App Uid list*/

	EMCOM_LOGD("The Mpip list will be update to empty.");

	/*Clear the Mpip App Uid list*/
	mutex_lock(&g_Mpip_mutex);
	for( uIndex = 0; uIndex < EMCOM_MAX_MPIP_APP; uIndex ++)
	{
		g_MpipUids[uIndex] = UID_INVALID_APP;
	}
	mutex_unlock(&g_Mpip_mutex);

	if((NULL == pdata) || (0 == len))
	{
		return 0;
	}
	ptemp = pdata;
	ulength = len/sizeof(uid_t);
	if(EMCOM_MAX_MPIP_APP < ulength )
	{
		EMCOM_LOGE("The length of received MPIP APP uid list is error.");
		return -EINVAL;
	}
	mutex_lock(&g_Mpip_mutex);
	for(uIndex = 0; uIndex < ulength; uIndex++)
	{
		g_MpipUids[uIndex] = *(uid_t *)ptemp;
		ptemp += sizeof(uid_t);
	}
	mutex_unlock(&g_Mpip_mutex);

	return 0;
}


/*****************************************************************************
 函 数 名  : Emcom_Xengine_StartMPIP
 功能描述  : 启动APP绑定第二个PDN
 输入参数  : char *pdata 第二个PDN的网卡名称
             uint16_t len 数据长度
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
        1.日    期   : 2017年09月05日
           作    者   : z00196795
           修改内容   : 新生成函数
*****************************************************************************/

int Emcom_Xengine_StartMPIP(char *pdata, uint16_t len)
{
	/*input param check*/
	if( (NULL == pdata) || (0 == len) || (IFNAMSIZ < (len/sizeof(uint16_t))) )
	{
	    EMCOM_LOGE("MPIP interface name or length %d is error", len);
		return -EINVAL;
	}
	mutex_lock(&g_Mpip_mutex);
	memcpy (g_Ifacename, pdata, len);
	g_MpipStart = true;
	mutex_unlock(&g_Mpip_mutex);
	EMCOM_LOGD("Mpip is :%d to start.", g_MpipStart);
	return 0;
}


/*****************************************************************************
 函 数 名  : Emcom_Xengine_StopMPIP
 功能描述  : 停止APP加速
 输入参数  : uint8_t *pdata  保持格式，为空
            uint16_t len 数据长度，为空
 输出参数  : 无
 返 回 值  : 整形
 调用函数  :
 被调函数  :
 修改历史  :
        1.日    期   : 2017年09月05日
           作    者   : z00196795
           修改内容   : 新生成函数
*****************************************************************************/

int Emcom_Xengine_StopMPIP(uint8_t *pdata, uint16_t len)
{
	mutex_lock(&g_Mpip_mutex);
	g_MpipStart = false;
	mutex_unlock(&g_Mpip_mutex);
	EMCOM_LOGD("MPIP function is :%d, ready to stop", g_MpipStart);

	return 0;
}

/*****************************************************************************
 函 数 名  : Emcom_Xengine_IsMpipBindUid
 功能描述  : 判断是否为允许绑定的Uid
 输入参数  : uid_t lUid
 输出参数  : 无
 返 回 值  : true/false
 调用函数  :
 被调函数  :
 修改历史  :
    1.日    期   : 2017年09月07日
       作    者   : y00369524
       修改内容   : 新生成函数

*****************************************************************************/
bool Emcom_Xengine_IsMpipBindUid(uid_t lUid)
{
	uint8_t  uIndex;
	mutex_lock(&g_Mpip_mutex);
	for( uIndex = 0; uIndex < EMCOM_MAX_MPIP_APP; uIndex ++)
	{
		if( lUid == g_MpipUids[uIndex] )
		{
			mutex_unlock(&g_Mpip_mutex);
			return true;
		}
	}
	mutex_unlock(&g_Mpip_mutex);

	return false;
}
/*****************************************************************************
 函 数 名  : Emcom_Xengine_Mpip_Bind2Device
 功能描述  : 勾取上行数据包
 输入参数  : struct sock *pstSock  socket对象
             struct msghdr *msg   发送的消息结构体
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
    1.日    期   : 2017年09月07日
       作    者   : y00369524
       修改内容   : 新生成函数
*****************************************************************************/

void Emcom_Xengine_Mpip_Bind2Device(struct sock *pstSock)
{
	bool  bFound           = false;
	uint8_t  uIndex        = 0;
	uid_t lSockUid         = 0;
	struct net *net        = NULL;
	struct net_device *dev = NULL;

	if(NULL == pstSock)
	{
		EMCOM_LOGE(" param invalid.\n");
		return;
	}

	if(!g_MpipStart)
	{
		return;
	}
	/**
	 * if uid equals current bind uid, bind 2 device
	 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 10)
	lSockUid = sock_i_uid(pstSock).val;
#else
	lSockUid = sock_i_uid(pstSock);
#endif

	if( invalid_uid ( lSockUid ))
	{
		return;
	}

	net = sock_net(pstSock);
	bFound = Emcom_Xengine_IsMpipBindUid( lSockUid );
	if(bFound)
	{
		rcu_read_lock();
		dev = dev_get_by_name_rcu(net, g_Ifacename);
		if(dev)
		{
            uIndex = dev->ifindex;
		}
		rcu_read_unlock();
		if ((!dev) || (!test_bit(__LINK_STATE_START, &dev->state)))
		{
			g_MpipStart = false;
			emcom_send_msg2daemon(NETLINK_EMCOM_KD_XENIGE_DEV_FAIL, NULL, 0);
			EMCOM_LOGE(" get dev fail or dev is not up.\n");
			return;
		}

		lock_sock(pstSock);
		pstSock->sk_bound_dev_if = uIndex;
		sk_dst_reset(pstSock);
		release_sock(pstSock);
		}
}

/*****************************************************************************
 函 数 名  : Emcom_Xengine_RrcKeep
 功能描述  : 通知bastet做 RRC加速
 输入参数  : uint8_t *pdata  停止加速的app信息
                           uint16_t len 数据长度
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
        1.日    期   : 2016年11月18日
           作    者   : z00371705
           修改内容   : 新生成函数
*****************************************************************************/

int Emcom_Xengine_RrcKeep( void )
{
#ifdef CONFIG_HUAWEI_BASTET
	post_indicate_packet(BST_IND_RRC_KEEP,NULL,0);
#endif
	return 0;
}


/*****************************************************************************
 函 数 名  : Emcom_Send_KeyPsInfo
 功能描述  : emcom xengine 收到damone消息的参数传递给bastet
 输入参数  : uint8_t *pdata  消息的参数
             uint16_t len 数据长度
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
        1.日    期   : 2017年06月30日
           作    者   : l00416134
           修改内容   : 新生成函数
*****************************************************************************/

int Emcom_Send_KeyPsInfo(uint8_t *pdata, uint16_t len)
{
	uint32_t            ulState;

	/*input param check*/
	if( NULL == pdata )
	{
		EMCOM_LOGE("Emcom_Send_KeyPsInfo:data is null");
		return -EINVAL;
	}

	/*check len is invalid*/
	if( len < sizeof( uint32_t ) )
	{
		EMCOM_LOGE("Emcom_Send_KeyPsInfo: len：%d is illegal", len);
		return -EINVAL;
	}

	ulState =*(uint32_t *)pdata;

	if( true != Emcom_Is_Modem_Support() )
	{
		EMCOM_LOGI( "Emcom_Send_KeyPsInfo: modem not support" );
		return -EINVAL;
	}

#ifdef CONFIG_HUAWEI_BASTET_COMM
	bastet_comm_keypsInfo_write( ulState );
#endif
	return 0;
}


/*****************************************************************************
 函 数 名  : Emcom_Xengine_isWlan
 功能描述  :判断是否是wlan传输
 输入参数  : struct sk_buff *skb
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
        1.日    期   : 2017年09月7日
           作    者   : l00301519
           修改内容   : 新生成函数
*****************************************************************************/
static inline bool Emcom_Xengine_isWlan(struct sk_buff *skb)
{
	const char *delim = "wlan0";
	int len = strlen(delim);
	if(!skb->dev)
	{
		return false;
	}
	if(!skb->dev->name)
	{
		return false;
	}

	if (strncmp(skb->dev->name, delim, len))
	{
		return false;
	}

	return true;
}


/*****************************************************************************
 函 数 名  : Emcom_Xengine_UdpEnqueue
 功能描述  :将skb插入队列
 输入参数  : struct sk_buff *skb
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
        1.日    期   : 2017年09月7日
           作    者   : l00301519
           修改内容   : 新生成函数
*****************************************************************************/
void Emcom_Xengine_UdpEnqueue(struct sk_buff *skb)
{
	struct sock *sk;
	struct sk_buff *skb2;
	uid_t lSockUid = UID_INVALID_APP;
	/* invalid g_UdpRetranUid means UDP retran is closed */

	if(invalid_uid(g_UdpRetranUid))
	{
		return;
	}

	if((!skb))
	{
		EMCOM_LOGE("Emcom_Xengine_UdpEnqueue skb null");
		return;
	}
	if(g_UdpSkbList.qlen >= EMCOM_MAX_UDP_SKB)
	{
		EMCOM_LOGE("Emcom_Xengine_UdpEnqueue max skb");
		return;
	}

	sk = skb_to_full_sk(skb);
	if (unlikely(!sk))
	{
		EMCOM_LOGE("Emcom_Xengine_UdpEnqueue sk null");
		return;
	}

	if (unlikely(!sk->sk_socket))
	{
		EMCOM_LOGE("Emcom_Xengine_UdpEnqueue sk_socket null");
		return;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 10)
	lSockUid = sock_i_uid(sk).val;
#else
	lSockUid = sock_i_uid(sk);
#endif
	if(lSockUid == g_UdpRetranUid)
	{
		if(!Emcom_Xengine_isWlan(skb))
		{
			EMCOM_LOGD("Emcom_Xengine_UdpEnqueue not wlan");
			Emcom_Xengine_udpretran_clear();
			return;
		}
		if(sk->sk_socket->type == SOCK_DGRAM)
		{
			skb2 = skb_copy(skb, GFP_ATOMIC);
			if(unlikely(!skb2))
			{
				EMCOM_LOGE("Emcom_Xengine_UdpEnqueue skb2 null");
				return;
			}
#ifdef EMCOM_UDPRETRAN_NODELAY
			dev_queue_xmit(skb2);
			return;
#endif
			skb_queue_tail(&g_UdpSkbList,skb2);
			Emcom_Xengine_setUdpTimerCb(skb2);
			if(!g_Emcom_udptimerOn)
			{
				skb2 = skb_peek(&g_UdpSkbList);
				if(!skb2)
				{
					EMCOM_LOGE("Emcom_Xengine_UdpEnqueue peek skb2 null");
					return;
				}
				g_Emcom_udptimerOn = true;
				g_UdpSkb_timer.expires = jiffies + UDPTIMER_DELAY*HZ/MSEC_PER_SEC;
				EMCOM_LOGD("Emcom_Xengine_UdpEnqueue: jiffie %d",UDPTIMER_DELAY*HZ/MSEC_PER_SEC);
				add_timer(&g_UdpSkb_timer);
			}
		}
	}
}


/*****************************************************************************
 函 数 名  : Emcom_Xengine_StartUdpReTran
 功能描述  :开始UDP重传
 输入参数  : uint8_t *pdata  消息的参数
             uint16_t len 数据长度
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
        1.日    期   : 2017年09月7日
           作    者   : l00301519
           修改内容   : 新生成函数
*****************************************************************************/
int Emcom_Xengine_StartUdpReTran(uint8_t *pdata, uint16_t len)
{
	uid_t              uid;

	/*input param check*/
	if( NULL == pdata )
	{
		EMCOM_LOGE("Emcom_Xengine_StartUdpReTran:data is null");
		return -EINVAL;
	}

	/*check len is invalid*/
	if(len != sizeof(uid_t))
	{
		EMCOM_LOGI("Emcom_Xengine_StartUdpReTran: len：%d is illegal", len);
		return -EINVAL;
	}

	uid =*(uid_t *)pdata;
	/*check uid*/
	if (invalid_uid(uid))
	{
		EMCOM_LOGE("Emcom_Xengine_StartUdpReTran: uid is invalid %d", uid);
		return -EINVAL;
	}
	EMCOM_LOGI("Emcom_Xengine_StartUdpReTran: uid：%d ", uid);
	g_UdpRetranUid = uid;
	return 0;
}

/*****************************************************************************
 函 数 名  : Emcom_Xengine_StopUdpReTran
 功能描述  :停止UDP重传
 输入参数  : uint8_t *pdata  消息的参数
             uint16_t len 数据长度
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
        1.日    期   : 2017年09月7日
           作    者   : l00301519
           修改内容   : 新生成函数
*****************************************************************************/
int Emcom_Xengine_StopUdpReTran(uint8_t *pdata, uint16_t len)
{
	Emcom_Xengine_udpretran_clear();
	return 0;
}


/*****************************************************************************
 函 数 名  : Emcom_Xengine_EvtProc
 功能描述  :emcom xengine 收到deamon的消息处理入口
 输入参数  : int32_t event   处理事件
                           uint8_t *pdata  事件的参数
                           uint16_t len 数据长度
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
        1.日    期   : 2017年03月18日
           作    者   : z00371705
           修改内容   : 新生成函数
*****************************************************************************/

void Emcom_Xengine_EvtProc(int32_t event, uint8_t *pdata, uint16_t len)
{
	switch(event)
	{
		case NETLINK_EMCOM_DK_START_ACC:
			EMCOM_LOGD("emcom netlink receive acc start\n");
			Emcom_Xengine_StartAccUid(pdata,len);
			break;
		case NETLINK_EMCOM_DK_STOP_ACC:
			EMCOM_LOGD("emcom netlink receive acc stop\n");
			Emcom_Xengine_StopAccUid(pdata,len);
			break;
		case NETLINK_EMCOM_DK_CLEAR:
			EMCOM_LOGD("emcom netlink receive clear info\n");
			Emcom_Xengine_clear();
			break;
		case NETLINK_EMCOM_DK_RRC_KEEP:
			EMCOM_LOGD("emcom netlink receive rrc keep\n");
			Emcom_Xengine_RrcKeep();
			break;
		case NETLINK_EMCOM_DK_KEY_PSINFO:
			EMCOM_LOGD("emcom netlink receive psinfo\n");
			Emcom_Send_KeyPsInfo(pdata,len);
			break;
		case NETLINK_EMCOM_DK_SPEED_CTRL:
			EMCOM_LOGD("emcom netlink receive speed control uid\n");
			Emcom_Xengine_SetSpeedCtrlInfo(pdata,len);
			break;
		case NETLINK_EMCOM_DK_START_UDP_RETRAN:
			EMCOM_LOGD("emcom netlink receive wifi udp start\n");
			Emcom_Xengine_StartUdpReTran(pdata,len);
			break;
		case NETLINK_EMCOM_DK_STOP_UDP_RETRAN:
			EMCOM_LOGD("emcom netlink receive wifi udp stop\n");
			Emcom_Xengine_StopUdpReTran(pdata,len);
			break;
		case NETLINK_EMCOM_DK_CONFIG_MPIP:
			EMCOM_LOGD("emcom netlink receive btm config start\n");
			Emcom_Xengine_Config_MPIP(pdata,len);
			break;
		case NETLINK_EMCOM_DK_START_MPIP:
			EMCOM_LOGD("emcom netlink receive btm start\n");
			Emcom_Xengine_StartMPIP(pdata,len);
			break;
		case NETLINK_EMCOM_DK_STOP_MPIP:
			EMCOM_LOGD("emcom netlink receive btm stop\n");
			Emcom_Xengine_StopMPIP(pdata,len);
			break;
		default:
			EMCOM_LOGI("emcom Xengine unsupport packet, the type is %d.\n", event);
			break;
	}
}



