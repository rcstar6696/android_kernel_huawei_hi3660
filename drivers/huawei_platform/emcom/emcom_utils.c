#include <linux/module.h>
#include <linux/types.h>
#include "emcom_utils.h"




#undef HWLOG_TAG
#define HWLOG_TAG emcom_utils
HWLOG_REGIST();
MODULE_LICENSE("GPL");

Emcom_Support_Enum g_Modem_emcom_support = MODEM_NOT_SUPPORT_EMCOM;
/******************************************************************************
   6 函数实现
******************************************************************************/


/*****************************************************************************
 函 数 名  : Emcom_Ind_Modem_Support
 功能描述  : 判断modem是否支持emcom
 输入参数  : enSupport
 输出参数  : g_Modem_emcom_support
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :
        1.日    期   : 2017年06月18日
           作    者   : l00416134
           修改内容   : 新生成函数
*****************************************************************************/
void Emcom_Ind_Modem_Support(uint8_t enSupport)
{
    EMCOM_LOGD("Emcom_Ind_Modem_Support:%d\n",g_Modem_emcom_support);
    g_Modem_emcom_support = ( Emcom_Support_Enum )enSupport;
}


bool Emcom_Is_Modem_Support( void )
{
    if( MODEM_NOT_SUPPORT_EMCOM == g_Modem_emcom_support )
    {
        return false;
    }
    else
    {
        return true;
    }
}
