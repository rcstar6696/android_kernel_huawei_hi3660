#ifndef _PARTITION_H_
#define _PARTITION_H_

#include "hisi_partition.h"

typedef struct partition {
	char name[PART_NAMELEN];
	unsigned long long start;
	unsigned long long length;
	unsigned int flags;
}PARTITION;

#if defined(CONFIG_HISI_PARTITION_HI3650)
static const struct partition partition_table_emmc[] =
{
    {PART_XLOADER,           0,         256,      EMMC_BOOT_MAJOR_PART},
    {PART_PTABLE,            0,         512,      EMMC_USER_PART}, /* ptable           512K */
    {PART_VRL,               512,       256,      EMMC_USER_PART}, /* VRL              256K p1*/
    {PART_VRL_BACKUP,        768,       256,      EMMC_USER_PART}, /* VRL backup       256K p2*/
    {PART_FW_LPM3,           1024,      256,      EMMC_USER_PART}, /* mcuimage         256K p3*/
    {PART_FRP,               1280,      768,      EMMC_USER_PART}, /* frp              768K p4*/
    {PART_FASTBOOT,          2*1024,    4*1024,   EMMC_USER_PART}, /* fastboot         4M   p5*/
    {PART_MODEMNVM_FACTORY,  6*1024,    4*1024,   EMMC_USER_PART}, /* modemnvm factory 4M   p6*/
    {PART_NVME,              10*1024,   6*1024,   EMMC_USER_PART}, /* nvme             6M   p7*/
    {PART_OEMINFO,           16*1024,   64*1024,  EMMC_USER_PART}, /* oeminfo          64M  p8*/
    {PART_SPLASH,            80*1024,   8*1024,   EMMC_USER_PART}, /* splash           8M   p9*/
    {PART_MODEMNVM_BACKUP,   88*1024,   4*1024,   EMMC_USER_PART}, /* modemnvm backup  4M   p10*/
    {PART_MODEMNVM_IMG,      92*1024,   8*1024,   EMMC_USER_PART}, /* modemnvm img     8M   p11*/
    {PART_MODEMNVM_SYSTEM,   100*1024,  4*1024,   EMMC_USER_PART}, /* modemnvm system  4M   p12*/
    {PART_SECURE_STORAGE,    104*1024,  32*1024,  EMMC_USER_PART}, /* secure storage   32M  p13*/
    {PART_3RDMODEMNVM,       136*1024,  16*1024,  EMMC_USER_PART}, /* 3rdmodemnvm      16M  p14*/
    {PART_3RDMODEMNVMBKP,    152*1024,  16*1024,  EMMC_USER_PART}, /* 3rdmodemnvmback  16M  p15*/
    {PART_PERSIST,           168*1024,  2*1024,   EMMC_USER_PART}, /* persist          2M   p16*/
    {PART_RESERVED1,         170*1024,  14*1024,  EMMC_USER_PART}, /* reserved1        14M  p17*/
    {PART_MODEM_OM,          184*1024,  32*1024,  EMMC_USER_PART}, /* modem om         32M  p18*/
    {PART_SPLASH2,           216*1024,  64*1024,  EMMC_USER_PART}, /* splash2          64M  p19*/
    {PART_MISC,              280*1024,  2*1024,   EMMC_USER_PART}, /* misc             2M   p20*/
    {PART_MODEMNVM_UPDATE,   282*1024,  24*1024,  EMMC_USER_PART}, /* modemnvm update  24M  p21*/
    {PART_RECOVERY2,         306*1024,  64*1024,  EMMC_USER_PART}, /* recovery2        64M  p22*/
    {PART_RESERVED2,         370*1024,  64*1024,  EMMC_USER_PART}, /* reserved2        64M  p23*/
    {PART_TEEOS,             434*1024,  4*1024,   EMMC_USER_PART}, /* teeos            4M   p24*/
    {PART_TRUSTFIRMWARE,     438*1024,  2*1024,   EMMC_USER_PART}, /* trustfirmware    2M   p25*/
    {PART_SENSORHUB,         440*1024,  16*1024,  EMMC_USER_PART}, /* sensorhub        16M  p26*/
    {PART_FW_HIFI,           456*1024,  12*1024,  EMMC_USER_PART}, /* hifi             12M  p27*/
    {PART_BOOT,              468*1024,  32*1024,  EMMC_USER_PART}, /* boot             32M  p28*/
    {PART_RECOVERY,          500*1024,  64*1024,  EMMC_USER_PART}, /* recovery         64M  p29*/
    {PART_DTS,               564*1024,  64*1024,  EMMC_USER_PART}, /* dtimage          64M  p30*/
    {PART_MODEM,             628*1024,  64*1024,  EMMC_USER_PART}, /* modem            64M  p31*/
    {PART_MODEM_DSP,         692*1024,  16*1024,  EMMC_USER_PART}, /* modem_dsp        16M  p32*/
    {PART_MODEM_DTB,         708*1024,  8*1024,   EMMC_USER_PART}, /* modem_dtb        8M   p33*/
    {PART_DFX,               716*1024,  16*1024,  EMMC_USER_PART}, /* dfx              16M  p34*/
    {PART_3RDMODEM,          732*1024,  64*1024,  EMMC_USER_PART}, /* 3rdmodem         64M  p35*/
    {PART_CACHE,             796*1024,  256*1024, EMMC_USER_PART}, /* cache            256M p36*/
    {PART_HISITEST0,         1052*1024, 2*1024,   EMMC_USER_PART}, /* hisitest0        2M   p37*/
    {PART_HISITEST1,         1054*1024, 2*1024,   EMMC_USER_PART}, /* hisitest1        2M   p38*/
#ifdef CONFIG_VENDORIMAGE_FILE_SYSTEM_TYPE
    {PART_PATCH,             1056*1024,  32*1024,  EMMC_USER_PART}, /* patch            32M  p39*/
    {PART_BOOTFAIL_INFO,     1088*1024,  2*1024,   EMMC_USER_PART}, /* bootfail info    2M   p40*/
    {PART_RRECORD,           1090*1024,  16*1024,  EMMC_USER_PART}, /* rrecord          16M  p41*/
    {PART_RESERVED3,         1106*1024,  14*1024,  EMMC_USER_PART}, /* rrecord          14M  p42*/

#ifdef CONFIG_MARKET_OVERSEA
    {PART_SYSTEM,            1120*1024, 3008*1024,EMMC_USER_PART}, /* system           3008M p43*/
    {PART_CUST,              4128*1024, 192*1024, EMMC_USER_PART}, /* cust             192M  p44*/
    {PART_VERSION,           4320*1024, 32*1024,  EMMC_USER_PART}, /* version          32M    p45*/
    {PART_VENDOR,            4352*1024, 608*1024, EMMC_USER_PART}, /* vendor           608M  p46*/
    {PART_PRODUCT,           4960*1024, 192*1024, EMMC_USER_PART}, /* product          192M   p47*/
    {PART_HISITEST2,         5152*1024, 4*1024,   EMMC_USER_PART}, /* hisitest2        4M    p48*/
    {PART_USERDATA,          5156*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p49*/
#elif defined CONFIG_MARKET_INTERNAL
    {PART_SYSTEM,            1120*1024, 2688*1024,EMMC_USER_PART}, /* system           2688M p43*/
    {PART_CUST,              3808*1024, 192*1024, EMMC_USER_PART}, /* cust             192M  p44*/
    {PART_VERSION,           4000*1024, 32*1024,  EMMC_USER_PART}, /* version          32M    p45*/
    {PART_VENDOR,            4032*1024, 608*1024, EMMC_USER_PART}, /* vendor           608M  p46*/
    {PART_PRODUCT,           4640*1024, 192*1024, EMMC_USER_PART}, /* product          192M   p47*/
    {PART_HISITEST2,         4832*1024, 4*1024,   EMMC_USER_PART}, /* hisitest2        4M    p48*/
    {PART_USERDATA,          4836*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p49*/
#else
    {PART_SYSTEM,            1120*1024, 1984*1024,EMMC_USER_PART}, /* system           1984M p43*/
    {PART_CUST,              3104*1024, 192*1024, EMMC_USER_PART}, /* cust             192M  p44*/
    {PART_VERSION,           3296*1024, 32*1024,   EMMC_USER_PART}, /* version         32M    p45*/
    {PART_VENDOR,            3328*1024, 608*1024, EMMC_USER_PART}, /* vendor           608M  p46*/
    {PART_PRODUCT,           3936*1024, 192*1024,  EMMC_USER_PART}, /* product         192M   p47*/
    {PART_HISITEST2,         4128*1024, 4*1024,   EMMC_USER_PART}, /* hisitest2        4M    p48*/
    {PART_USERDATA,          4132*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p49*/
#endif
#else
#ifdef CONFIG_MARKET_OVERSEA
    {PART_SYSTEM,            1056*1024, 3584*1024,EMMC_USER_PART}, /* system           3584M p39*/
    {PART_CUST,              4640*1024, 512*1024, EMMC_USER_PART}, /* cust             512M  p40*/
    {PART_HISITEST2,         5152*1024, 4*1024,   EMMC_USER_PART}, /* hisitest2        4M    p41*/
    {PART_USERDATA,          5156*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p42*/
#elif defined CONFIG_MARKET_INTERNAL
    {PART_SYSTEM,            1056*1024, 3264*1024,EMMC_USER_PART}, /* system           3264M p39*/
    {PART_CUST,              4320*1024, 512*1024, EMMC_USER_PART}, /* cust             512M  p40*/
    {PART_HISITEST2,         4832*1024, 4*1024,   EMMC_USER_PART}, /* hisitest2        4M    p41*/
    {PART_USERDATA,          4836*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p42*/
#else
    {PART_SYSTEM,            1056*1024, 2560*1024,EMMC_USER_PART}, /* system           2560M p39*/
    {PART_CUST,              3616*1024, 512*1024, EMMC_USER_PART}, /* cust             512M  p40*/
    {PART_HISITEST2,         4128*1024, 4*1024,   EMMC_USER_PART}, /* hisitest2        4M    p41*/
    {PART_USERDATA,          4132*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p42*/
#endif
#endif
    {"0", 0, 0, 0},                                        /* total 8228M*/
};

#elif defined (CONFIG_HISI_PARTITION_HI6250)
static const struct partition partition_table_emmc[] =
{
    {PART_XLOADER,           0,         256,      EMMC_BOOT_MAJOR_PART},
    {PART_PTABLE,            0,         512,      EMMC_USER_PART}, /* ptable           512K */
    {PART_VRL,               512,       256,      EMMC_USER_PART}, /* VRL              256K p1*/
    {PART_VRL_BACKUP,        768,       256,      EMMC_USER_PART}, /* VRL backup       256K p2*/
    {PART_FW_LPM3,           1024,      256,      EMMC_USER_PART}, /* mcuimage         256K p3*/
    {PART_FRP,               1280,      768,      EMMC_USER_PART}, /* frp              768K p4*/
    {PART_FASTBOOT,          2*1024,    4*1024,   EMMC_USER_PART}, /* fastboot         4M   p5*/
    {PART_MODEMNVM_FACTORY,  6*1024,    4*1024,   EMMC_USER_PART}, /* modemnvm factory 4M   p6*/
    {PART_NVME,              10*1024,   6*1024,   EMMC_USER_PART}, /* nvme             6M   p7*/
    {PART_OEMINFO,           16*1024,   64*1024,  EMMC_USER_PART}, /* oeminfo          64M  p8*/
    {PART_RESERVED3,         80*1024,   4*1024,   EMMC_USER_PART}, /* reserved3        4M   p9*/
    {PART_MODEMNVM_BACKUP,   84*1024,   4*1024,   EMMC_USER_PART}, /* modemnvm backup  4M   p10*/
    {PART_MODEMNVM_IMG,      88*1024,   8*1024,   EMMC_USER_PART}, /* modemnvm img     8M   p11*/
    {PART_MODEMNVM_SYSTEM,   96*1024,   4*1024,   EMMC_USER_PART}, /* modemnvm system  4M   p12*/
    {PART_SECURE_STORAGE,    100*1024,  32*1024,  EMMC_USER_PART}, /* secure storage   32M  p13*/
    {PART_RESERVED4,         132*1024,  2*1024,   EMMC_USER_PART}, /* reserved4        2M   p14*/
    {PART_RESERVED5,         134*1024,  2*1024,   EMMC_USER_PART}, /* reserved5        2M   p15*/
    {PART_PERSIST,           136*1024,  2*1024,   EMMC_USER_PART}, /* persist          2M   p16*/
#ifdef SECDOG_SUPPORT_RSA_2048
    {PART_MODEM_SECURE,      138*1024,  14*1024,  EMMC_USER_PART}, /* modem_secure     14M  p17*/
#else
    {PART_RESERVED1,         138*1024,  14*1024,  EMMC_USER_PART}, /* reserved1        14M  p17*/
#endif
    {PART_MODEM_OM,          152*1024,  32*1024,  EMMC_USER_PART}, /* modem om         32M  p18*/
    {PART_SPLASH2,           184*1024,  64*1024,  EMMC_USER_PART}, /* splash2          64M  p19*/
    {PART_MISC,              248*1024,  2*1024,   EMMC_USER_PART}, /* misc             2M   p20*/
    {PART_MODEMNVM_UPDATE,   250*1024,  24*1024,  EMMC_USER_PART}, /* modemnvm update  24M  p21*/
    {PART_RESERVED2,         274*1024,  60*1024,  EMMC_USER_PART}, /* reserved2        60M  p22*/
    {PART_TEEOS,             334*1024,  4*1024,   EMMC_USER_PART}, /* teeos            4M   p23*/
    {PART_TRUSTFIRMWARE,     338*1024,  2*1024,   EMMC_USER_PART}, /* trustfirmware    2M   p24*/
    {PART_SENSORHUB,         340*1024,  16*1024,  EMMC_USER_PART}, /* sensorhub        16M  p25*/
    {PART_FW_HIFI,           356*1024,  12*1024,  EMMC_USER_PART}, /* hifi             12M  p26*/
    {PART_ERECOVERY_KERNEL,  368*1024,  24*1024,  EMMC_USER_PART},/* erecovery_kernel  24M  p27*/
    {PART_ERECOVERY_RAMDISK, 392*1024,  32*1024,  EMMC_USER_PART},/* erecovery_ramdisk 32M  p28*/
    {PART_ERECOVERY_VENDOR,  424*1024,  16*1024,  EMMC_USER_PART},/* erecovery_vendor 16M   p29*/
    {PART_KERNEL,            440*1024,  24*1024,  EMMC_USER_PART},/* kernel          24M    p30*/
    {PART_RAMDISK,           464*1024,  16*1024,  EMMC_USER_PART},/* ramdisk         16M    p31*/
    {PART_RECOVERY_RAMDISK,  480*1024,  32*1024,  EMMC_USER_PART},/* recovery_ramdisk 32M   p32*/
    {PART_RECOVERY_VENDOR,   512*1024,  16*1024,  EMMC_USER_PART},/* recovery_vendor 16M    p33*/
    {PART_DTS,               528*1024,  28*1024,  EMMC_USER_PART}, /* dtimage          28M  p34*/
    {PART_DTO,               556*1024,  4*1024,  EMMC_USER_PART}, /* dtoimage          4M  p35*/
    {PART_MODEM_FW,          560*1024,  96*1024,  EMMC_USER_PART}, /* modem_fw         96M  p36*/
    {PART_RECOVERY_VBMETA, 656*1024,  1*1024,   EMMC_USER_PART}, /* recovery_vbmeta   1M   p37*/
    {PART_ERECOVERY_VBMETA,657*1024,  1*1024,   EMMC_USER_PART}, /* erecovery_vbmeta  1M   p38*/
    {PART_RESERVED8,         658*1024,  2*1024,   EMMC_USER_PART}, /* reserved8        2M   p39*/
    {PART_DFX,               660*1024,  16*1024,  EMMC_USER_PART}, /* dfx              16M  p40*/
    {PART_VBMETA,         676*1024,  4*1024,   EMMC_USER_PART}, /* PART_VBMETA        4M   p41*/
    {PART_CACHE,             680*1024,  128*1024, EMMC_USER_PART}, /* cache            128M p42*/
    {PART_ODM,               808*1024,  128*1024, EMMC_USER_PART}, /* odm              128M p43*/
    {PART_HISITEST0,         936*1024,  2*1024,   EMMC_USER_PART}, /* hisitest0        2M   p44*/
    {PART_HISITEST1,         938*1024,  2*1024,   EMMC_USER_PART}, /* hisitest1        2M   p45*/
    {PART_HISITEST2,         940*1024,  4*1024,   EMMC_USER_PART}, /* hisitest2        4M   p46*/
#if (defined(CONFIG_MARKET_OVERSEA) || defined(CONFIG_MARKET_INTERNAL) || defined(CONFIG_MARKET_16G_OVERSEA) || defined(CONFIG_MARKET_16G_INTERNAL))
    {PART_PATCH,             944*1024,  32*1024,  EMMC_USER_PART}, /* patch            32M  p47*/
    {PART_BOOTFAIL_INFO,     976*1024,  2*1024,   EMMC_USER_PART}, /* bootfail_info    2M   p48*/
    {PART_RRECORD,           978*1024,  16*1024,  EMMC_USER_PART}, /* rrecord          16M  p49*/
    {PART_RESERVED9,         994*1024,  30*1024,  EMMC_USER_PART}, /* reserved9        30M  p50*/
#endif
#ifdef CONFIG_MARKET_OVERSEA
#ifdef CONFIG_PARTITION_ROMUPGRADE_HI6250
    {PART_SYSTEM,            1024*1024, 4688*1024,EMMC_USER_PART}, /* system           4688M p51*/
    {PART_CUST,              5712*1024, 192*1024, EMMC_USER_PART}, /* cust             192M  p52*/
    {PART_VERSION,           5904*1024, 32*1024,  EMMC_USER_PART}, /* version          32M   p53*/
    {PART_VENDOR,            5936*1024, 784*1024, EMMC_USER_PART}, /* vendor           784M  p54*/
    {PART_PRODUCT,           6720*1024, 192*1024, EMMC_USER_PART}, /* product          192M  p55*/
    {PART_USERDATA,          6912*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p56*/
#else
    {PART_SYSTEM,            1024*1024, 3536*1024,EMMC_USER_PART}, /* system           3536M p51*/
    {PART_CUST,              4560*1024, 192*1024, EMMC_USER_PART}, /* cust             192M  p52*/
    {PART_VERSION,           4752*1024, 32*1024,  EMMC_USER_PART}, /* version          32M   p53*/
    {PART_VENDOR,            4784*1024, 784*1024, EMMC_USER_PART}, /* vendor           784M  p54*/
    {PART_PRODUCT,           5568*1024, 192*1024, EMMC_USER_PART}, /* product          192M  p55*/
    {PART_USERDATA,          5760*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p56*/
#endif
#elif defined CONFIG_MARKET_INTERNAL
#ifdef CONFIG_PARTITION_ROMUPGRADE_HI6250
    {PART_SYSTEM,            1024*1024, 3552*1024,EMMC_USER_PART}, /* system           3552M p51*/
    {PART_CUST,              4576*1024, 192*1024, EMMC_USER_PART}, /* cust             192M  p52*/
    {PART_VERSION,           4768*1024, 32*1024,  EMMC_USER_PART}, /* version          32M   p53*/
    {PART_VENDOR,            4800*1024, 784*1024, EMMC_USER_PART}, /* vendor           784M  p54*/
    {PART_PRODUCT,           5584*1024, 192*1024, EMMC_USER_PART}, /* product          192M  p55*/
    {PART_USERDATA,          5776*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p56*/
#else
    {PART_SYSTEM,            1024*1024, 3088*1024,EMMC_USER_PART}, /* system           3088M p51*/
    {PART_CUST,              4112*1024, 192*1024, EMMC_USER_PART}, /* cust             192M  p52*/
    {PART_VERSION,           4304*1024, 32*1024,  EMMC_USER_PART}, /* version          32M   p53*/
    {PART_VENDOR,            4336*1024, 784*1024, EMMC_USER_PART}, /* vendor           784M  p54*/
    {PART_PRODUCT,           5120*1024, 192*1024, EMMC_USER_PART}, /* product          192M  p55*/
    {PART_USERDATA,          5312*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p56*/
#endif
#elif defined CONFIG_MARKET_16G_OVERSEA
    {PART_SYSTEM,            1024*1024, 3008*1024,EMMC_USER_PART}, /* system           3008M p51*/
    {PART_CUST,              4032*1024, 192*1024, EMMC_USER_PART}, /* cust             192M  p52*/
    {PART_VERSION,           4224*1024, 32*1024,  EMMC_USER_PART}, /* version          32M   p53*/
    {PART_VENDOR,            4256*1024, 784*1024, EMMC_USER_PART}, /* vendor           784M  p54*/
    {PART_PRODUCT,           5040*1024, 192*1024, EMMC_USER_PART}, /* product          192M  p55*/
    {PART_USERDATA,          5232*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p56*/
#elif defined CONFIG_MARKET_16G_INTERNAL
    {PART_SYSTEM,            1024*1024, 2720*1024,EMMC_USER_PART}, /* system           2720M p51*/
    {PART_CUST,              3744*1024, 192*1024, EMMC_USER_PART}, /* cust             192M  p52*/
    {PART_VERSION,           3936*1024, 32*1024,  EMMC_USER_PART}, /* version          32M   p53*/
    {PART_VENDOR,            3968*1024, 784*1024, EMMC_USER_PART}, /* vendor           784M  p54*/
    {PART_PRODUCT,           4752*1024, 192*1024, EMMC_USER_PART}, /* product          192M  p55*/
    {PART_USERDATA,          4944*1024, 4096*1024,EMMC_USER_PART}, /* userdata         4096M p56*/
#elif defined CONFIG_MARKET_BERLIN_OVERSEA
    {PART_PATCH,             944*1024,  32*1024,  EMMC_USER_PART}, /* patch             32M   p47*/
    {PART_BOOTFAIL_INFO,     976*1024,  2*1024,   EMMC_USER_PART}, /* bootfail_info     2M    p48*/
    {PART_RRECORD,           978*1024,  16*1024,  EMMC_USER_PART}, /* rrecord           16M   p49*/
    {PART_RESERVED9,         994*1024,  14*1024,  EMMC_USER_PART}, /* reserved9         14M   p50*/
    {PART_SYSTEM,            1008*1024, 3008*1024,EMMC_USER_PART}, /* system            3008M p51*/
    {PART_CUST,              4016*1024, 192*1024, EMMC_USER_PART}, /* cust              192M  p52*/
    {PART_VERSION,           4208*1024, 32*1024,  EMMC_USER_PART}, /* version           24M   p53*/
    {PART_VENDOR,            4240*1024, 608*1024, EMMC_USER_PART}, /* vendor            608M  p54*/
    {PART_PRODUCT,           4848*1024, 192*1024, EMMC_USER_PART}, /* product           200M  p55*/
    {PART_USERDATA,          5040*1024, 4096*1024,EMMC_USER_PART}, /* userdata          4096M p56*/
#elif defined CONFIG_MARKET_BERLIN_INTERNAL
    {PART_PATCH,             944*1024,  32*1024,  EMMC_USER_PART}, /* patch             32M   p47*/
    {PART_BOOTFAIL_INFO,     976*1024,  2*1024,   EMMC_USER_PART}, /* bootfail_info     2M    p48*/
    {PART_RRECORD,           978*1024,  16*1024,  EMMC_USER_PART}, /* rrecord           16M   p49*/
    {PART_RESERVED9,         994*1024,  14*1024,  EMMC_USER_PART}, /* reserved9         14M   p50*/
    {PART_SYSTEM,            1008*1024, 2688*1024,EMMC_USER_PART}, /* system            2688M p51*/
    {PART_CUST,              3696*1024, 192*1024, EMMC_USER_PART}, /* cust              192M  p52*/
    {PART_VERSION,           3888*1024, 32*1024,  EMMC_USER_PART}, /* version           32M   p53*/
    {PART_VENDOR,            3920*1024, 608*1024, EMMC_USER_PART}, /* vendor            608M  p54*/
    {PART_PRODUCT,           4528*1024, 192*1024, EMMC_USER_PART}, /* product           192M  p55*/
    {PART_USERDATA,          4720*1024, 4096*1024,EMMC_USER_PART}, /* userdata          4096M p56*/
#elif defined CONFIG_MARKET_FULL_OVERSEA
    {PART_PATCH,             944*1024,  32*1024,  EMMC_USER_PART}, /* patch             32M   p47*/
    {PART_BOOTFAIL_INFO,     976*1024,  2*1024,   EMMC_USER_PART}, /* bootfail_info     2M    p48*/
    {PART_RRECORD,           978*1024,  16*1024,  EMMC_USER_PART}, /* rrecord           16M   p49*/
    {PART_RESERVED9,         994*1024,  14*1024,  EMMC_USER_PART}, /* reserved9         14M   p50*/
    {PART_SYSTEM,            1008*1024, 5632*1024,EMMC_USER_PART}, /* system            5632M p51*/
    {PART_CUST,              6640*1024, 192*1024, EMMC_USER_PART}, /* cust              192M  p52*/
    {PART_VERSION,           6832*1024, 32*1024,  EMMC_USER_PART}, /* version           32M   p53*/
    {PART_VENDOR,            6864*1024, 784*1024, EMMC_USER_PART}, /* vendor            784M  p54*/
    {PART_PRODUCT,           7648*1024, 192*1024, EMMC_USER_PART}, /* product           192M  p55*/
    {PART_USERDATA,          7840*1024, 4096*1024,EMMC_USER_PART}, /* userdata          4096M p56*/
#elif defined CONFIG_MARKET_FULL_INTERNAL
    {PART_PATCH,             944*1024,  32*1024,  EMMC_USER_PART}, /* patch             32M   p47*/
    {PART_BOOTFAIL_INFO,     976*1024,  2*1024,   EMMC_USER_PART}, /* bootfail_info     2M    p48*/
    {PART_RRECORD,           978*1024,  16*1024,  EMMC_USER_PART}, /* rrecord           16M   p49*/
    {PART_RESERVED9,         994*1024,  14*1024,  EMMC_USER_PART}, /* reserved9         14M   p50*/
    {PART_SYSTEM,            1008*1024, 4752*1024,EMMC_USER_PART}, /* system            4752M p51*/
    {PART_CUST,              5760*1024, 192*1024, EMMC_USER_PART}, /* cust              192M  p52*/
    {PART_VERSION,           5952*1024, 32*1024,  EMMC_USER_PART}, /* version           32M   p53*/
    {PART_VENDOR,            5984*1024, 784*1024, EMMC_USER_PART}, /* vendor            784M  p54*/
    {PART_PRODUCT,           6768*1024, 192*1024, EMMC_USER_PART}, /* product           192M  p55*/
    {PART_USERDATA,          6960*1024, 4096*1024,EMMC_USER_PART}, /* userdata          4096M p56*/
#else
#ifdef CONFIG_VENDORIMAGE_FILE_SYSTEM_TYPE
    {PART_PATCH,             944*1024,  32*1024,  EMMC_USER_PART}, /* patch             32M   p47*/
    {PART_BOOTFAIL_INFO,     976*1024,  2*1024,   EMMC_USER_PART}, /* bootfail_info     2M    p48*/
    {PART_RRECORD,           978*1024,  16*1024,  EMMC_USER_PART}, /* rrecord           16M   p49*/
    {PART_RESERVED9,         994*1024,  14*1024,  EMMC_USER_PART}, /* reserved9         14M   p50*/
    {PART_SYSTEM,            1008*1024, 1984*1024,EMMC_USER_PART}, /* system            1984M p51*/
    {PART_CUST,              2992*1024, 192*1024, EMMC_USER_PART}, /* cust              192M  p52*/
    {PART_VERSION,           3184*1024, 32*1024,  EMMC_USER_PART}, /* version           32M   p53*/
    {PART_VENDOR,            3216*1024, 608*1024, EMMC_USER_PART}, /* vendor            608M  p54*/
    {PART_PRODUCT,           3824*1024, 192*1024, EMMC_USER_PART}, /* product           192M  p55*/
    {PART_USERDATA,          4016*1024, 4096*1024,EMMC_USER_PART}, /* userdata          4096M p56*/
#else
    {PART_PATCH,             944*1024,  32*1024,  EMMC_USER_PART}, /* patch             32M   p47*/
    {PART_BOOTFAIL_INFO,     976*1024,  2*1024,   EMMC_USER_PART}, /* bootfail_info     2M    p48*/
    {PART_RRECORD,           978*1024,  16*1024,  EMMC_USER_PART}, /* rrecord           16M   p49*/
    {PART_RESERVED9,         994*1024,  14*1024,  EMMC_USER_PART}, /* reserved9         14M   p50*/
    {PART_SYSTEM,            1008*1024, 1984*1024,EMMC_USER_PART}, /* system            1984M p51*/
    {PART_CUST,              2992*1024, 192*1024, EMMC_USER_PART}, /* cust              192M  p52*/
    {PART_VERSION,           3184*1024, 32*1024,  EMMC_USER_PART}, /* version           32M   p53*/
    {PART_VENDOR,            3216*1024, 608*1024, EMMC_USER_PART}, /* vendor            608M  p54*/
    {PART_PRODUCT,           3824*1024, 192*1024, EMMC_USER_PART}, /* product           192M  p55*/
    {PART_USERDATA,          4016*1024, 4096*1024,EMMC_USER_PART}, /* userdata          4096M p56*/
#endif
#endif
    {"0", 0, 0, 0},                                       /* */
};
#elif defined CONFIG_HISI_PARTITION_HI3660
static const struct partition partition_table_emmc[] =
{
  {PART_XLOADER_A,        0,         2*1024,        EMMC_BOOT_MAJOR_PART},
  {PART_XLOADER_B,        0,         2*1024,        EMMC_BOOT_BACKUP_PART},
  {PART_PTABLE,           0,         512,           EMMC_USER_PART},/* ptable          512K */
  {PART_FRP,              512,       512,           EMMC_USER_PART},/* frp             512K   p1*/
  {PART_PERSIST,          1024,      2048,          EMMC_USER_PART},/* persist         2048K  p2*/
  {PART_RESERVED1,        3072,      5120,          EMMC_USER_PART},/* reserved1       5120K  p3*/
  {PART_RESERVED6,        8*1024,    512,           EMMC_USER_PART},/* reserved6       512K   p4*/
  {PART_VRL,              8704,      512,           EMMC_USER_PART},/* vrl             512K   p5*/
  {PART_VRL_BACKUP,       9216,      512,           EMMC_USER_PART},/* vrl backup      512K   p6*/
  {PART_MODEM_SECURE,     9728,      8704,          EMMC_USER_PART},/* modem_secure    8704k  p7*/
  {PART_NVME,             18*1024,   6*1024,        EMMC_USER_PART},/* nvme            6M     p8*/
  {PART_OEMINFO,          24*1024,   64*1024,       EMMC_USER_PART},/* oeminfo         64M    p9*/
  {PART_SECURE_STORAGE,   88*1024,   32*1024,       EMMC_USER_PART},/* secure storage  32M    p10*/
  {PART_MODEM_OM,         120*1024,  32*1024,       EMMC_USER_PART},/* modem om        32M    p11*/
  {PART_MODEMNVM_FACTORY, 152*1024,  4*1024,        EMMC_USER_PART},/* modemnvm factory4M     p12*/
  {PART_MODEMNVM_BACKUP,  156*1024,  4*1024,        EMMC_USER_PART},/* modemnvm backup 4M     p13*/
  {PART_MODEMNVM_IMG,     160*1024,  12*1024,       EMMC_USER_PART},/* modemnvm img    12M    p14*/
  {PART_MODEMNVM_SYSTEM,  172*1024,  4*1024,        EMMC_USER_PART},/* modemnvm system 4M     p15*/
  {PART_SPLASH2,          176*1024,  80*1024,       EMMC_USER_PART},/* splash2         80M    p16*/
  {PART_CACHE,            256*1024,  128*1024,      EMMC_USER_PART},/* cache         128M     p17*/
  {PART_ODM_A,              384*1024,  128*1024,      EMMC_USER_PART},/* odm           128M     p18*/
  {PART_BOOTFAIL_INFO,    512*1024,  2*1024,        EMMC_USER_PART},/* bootfail info   2MB    p19*/
  {PART_MISC,             514*1024,  2*1024,        EMMC_USER_PART},/* misc            2M     p20*/
  {PART_RESERVED2,        516*1024,  32*1024,       EMMC_USER_PART},/* reserved2       32M    p21*/
  {PART_RESERVED10,        548*1024,  4*1024,       EMMC_USER_PART},/* PART_RESERVED10 4M     p22*/
  {PART_HISEE_FS,         552*1024,  8*1024,        EMMC_USER_PART},/* hisee_fs        8M     p23*/
  {PART_DFX,              560*1024,  16*1024,       EMMC_USER_PART},/* dfx             16M    p24*/
  {PART_RRECORD,          576*1024,  16*1024,       EMMC_USER_PART},/* rrecord         16M    p25*/
  {PART_FW_LPM3_A,        592*1024,  256,           EMMC_USER_PART},/* mcuimage        256K   p26*/
  {PART_RESERVED3_A,      606464,    3840,          EMMC_USER_PART},/* reserved3A      3840KB p27*/
  {PART_HISEE_IMG_A,      596*1024,    4*1024,      EMMC_USER_PART},/* part_hisee_img_a   4*1024KB p28*/
  {PART_FASTBOOT_A,       600*1024,  12*1024,       EMMC_USER_PART},/* fastboot        12M    p29*/
  {PART_VECTOR_A,         612*1024,  4*1024,        EMMC_USER_PART},/* avs vector      4M     p30*/
  {PART_ISP_BOOT_A,       616*1024,  2*1024,        EMMC_USER_PART},/* isp_boot        2M     p31*/
  {PART_ISP_FIRMWARE_A,   618*1024,  14*1024,       EMMC_USER_PART},/* isp_firmware    14M    p32*/
  {PART_FW_HIFI_A,        632*1024,  12*1024,       EMMC_USER_PART},/* hifi            12M    p33*/
  {PART_TEEOS_A,          644*1024,  8*1024,        EMMC_USER_PART},/* teeos           8M     p34*/
#ifdef CONFIG_SANITIZER_ENABLE
  {PART_ERECOVERY_KERNEL_A, 652*1024,  24*1024,     EMMC_USER_PART},/* erecovery_kernel  24M  p35*/
  {PART_ERECOVERY_RAMDISK_A, 676*1024,  32*1024,    EMMC_USER_PART},/* erecovery_ramdisk 32M  p36*/
  {PART_ERECOVERY_VENDOR_A, 708*1024,  8*1024,     EMMC_USER_PART},/* erecovery_vendor  8M    p37*/
  {PART_SENSORHUB_A,      716*1024,  16*1024,       EMMC_USER_PART},/* sensorhub       16M    p38*/
  {PART_KERNEL_A,         732*1024,  32*1024,       EMMC_USER_PART},/* kernel          32M    p39*/
#else
  {PART_ERECOVERY_KERNEL_A, 652*1024,  24*1024,     EMMC_USER_PART},/* erecovery_kernel  24M  p35*/
  {PART_ERECOVERY_RAMDISK_A, 676*1024,  32*1024,    EMMC_USER_PART},/* erecovery_ramdisk 32M  p36*/
  {PART_ERECOVERY_VENDOR_A, 708*1024,  16*1024,     EMMC_USER_PART},/* erecovery_vendor 16M   p37*/
  {PART_SENSORHUB_A,      724*1024,  16*1024,       EMMC_USER_PART},/* sensorhub       16M    p38*/
  {PART_KERNEL_A,         740*1024,  24*1024,       EMMC_USER_PART},/* kernel          24M    p39*/
#endif
  {PART_RAMDISK_A,        764*1024,  16*1024,       EMMC_USER_PART},/* ramdisk         16M    p40*/
  {PART_RECOVERY_RAMDISK_A, 780*1024,  32*1024,     EMMC_USER_PART},/* recovery_ramdisk 32M   p41*/
  {PART_RECOVERY_VENDOR_A, 812*1024,  16*1024,      EMMC_USER_PART},/* recovery_vendor 16M    p42*/
  {PART_DTS_A,            828*1024,  14*1024,       EMMC_USER_PART},/* dtimage         14M    p43*/
  {PART_DTO_A,            842*1024,   2*1024,       EMMC_USER_PART},/* dtoimage        2M     p44*/
  {PART_TRUSTFIRMWARE_A,  844*1024,  2*1024,        EMMC_USER_PART},/* trustfirmware   2M     p45*/
  {PART_MODEM_FW_A,       846*1024,  56*1024,       EMMC_USER_PART},/* modem_fw        56M    p46*/
  {PART_RESERVED4_A,      902*1024,  42*1024,       EMMC_USER_PART},/* reserved4A      42M    p47*/
  {PART_RECOVERY_VBMETA_A,  944*1024, 2*1024,       EMMC_USER_PART},/* recovery_vbmeta_a   2M    p48*/
  {PART_ERECOVERY_VBMETA_A, 946*1024, 2*1024,       EMMC_USER_PART},/* erecovery_vbmeta_a  2M    p49*/
  {PART_VBMETA_A,         948*1024,  4*1024,        EMMC_USER_PART},/* PART_VBMETA_A   4M     p50*/
  {PART_MODEMNVM_UPDATE_A,952*1024,  80*1024,       EMMC_USER_PART},/* modemnvm update 80M    p51*/
  {PART_PATCH_A,          1032 *1024,  32*1024,       EMMC_USER_PART},/* patch         32M    p52*/
  {PART_VERSION_A,        1064*1024,  32*1024,        EMMC_USER_PART},/* version       32M    p53*/
  {PART_VENDOR_A,         1096*1024,  784*1024,      EMMC_USER_PART},/* vendor         784M   p54*/
  {PART_PRODUCT_A,        1880*1024, 192*1024,      EMMC_USER_PART},/* product         192M   p55*/
  {PART_CUST_A,           2072*1024, 192*1024,      EMMC_USER_PART},/* cust            192M   p56*/
  #ifdef CONFIG_MARKET_INTERNAL
  {PART_SYSTEM_A,         2264*1024, 3552*1024,     EMMC_USER_PART},/* system          3552M  p57*/
  {PART_RESERVED5,        5816*1024, 128*1024,       EMMC_USER_PART},/* reserved5      128M   p58*/
  {PART_USERDATA,         5944*1024, (4UL)*1024*1024,EMMC_USER_PART},/* userdata       4G     p59*/
  #else
  {PART_SYSTEM_A,         2264*1024, 4688*1024,     EMMC_USER_PART},/* system          4688M  p57*/
  {PART_RESERVED5,        6952*1024, 128*1024,       EMMC_USER_PART},/* reserved5      128M   p58*/
  {PART_USERDATA,         7080*1024, (4UL)*1024*1024,EMMC_USER_PART},/* userdata       4G     p59*/
  #endif
  {"0", 0, 0, 0},                                        /* total 11848M*/
};
#ifdef CONFIG_HISI_STORAGE_UFS_PARTITION
static const struct partition partition_table_ufs[] =
{
  {PART_XLOADER_A,        0,         2*1024,        UFS_PART_0},
  {PART_XLOADER_B,        0,         2*1024,        UFS_PART_1},
  {PART_PTABLE,           0,         512,           UFS_PART_2},/* ptable          512K */
  {PART_FRP,              512,       512,           UFS_PART_2},/* frp             512K   p1*/
  {PART_PERSIST,          1024,      2048,          UFS_PART_2},/* persist         2048K  p2*/
  {PART_RESERVED1,        3072,      5120,          UFS_PART_2},/* reserved1       5120K  p3*/
  {PART_PTABLE_LU3,       0,         512,           UFS_PART_3},/* ptable_lu3      512K   p0*/
  {PART_VRL,              512,       512,           UFS_PART_3},/* vrl             512K   p1*/
  {PART_VRL_BACKUP,       1024,      512,           UFS_PART_3},/* vrl backup      512K   p2*/
  {PART_MODEM_SECURE,     1536,      8704,          UFS_PART_3},/* modem_secure    8704k  p3*/
  {PART_NVME,             10*1024,   6*1024,        UFS_PART_3},/* nvme            6M     p4*/
  {PART_OEMINFO,          16*1024,   64*1024,       UFS_PART_3},/* oeminfo         64M    p5*/
  {PART_SECURE_STORAGE,   80*1024,   32*1024,       UFS_PART_3},/* secure storage  32M    p6*/
  {PART_MODEM_OM,         112*1024,  32*1024,       UFS_PART_3},/* modem om        32M    p7*/
  {PART_MODEMNVM_FACTORY, 144*1024,  4*1024,        UFS_PART_3},/* modemnvm factory4M     p8*/
  {PART_MODEMNVM_BACKUP,  148*1024,  4*1024,        UFS_PART_3},/* modemnvm backup 4M     p9*/
  {PART_MODEMNVM_IMG,     152*1024,  12*1024,       UFS_PART_3},/* modemnvm img    12M    p10*/
  {PART_MODEMNVM_SYSTEM,  164*1024,  4*1024,        UFS_PART_3},/* modemnvm system 4M     p11*/
  {PART_SPLASH2,          168*1024,  80*1024,       UFS_PART_3},/* splash2         80M    p12*/
  {PART_CACHE,            248*1024,  128*1024,      UFS_PART_3},/* cache           128M     p13*/
  {PART_ODM_A,              376*1024,  128*1024,      UFS_PART_3},/* odm             128M     p14*/
  {PART_BOOTFAIL_INFO,    504*1024,  2*1024,        UFS_PART_3},/* bootfail info   2MB    p15*/
  {PART_MISC,             506*1024,  2*1024,        UFS_PART_3},/* misc            2M     p16*/
  {PART_RESERVED2,        508*1024,  32*1024,       UFS_PART_3},/* reserved2       32M    p17*/
  {PART_RESERVED10,        540*1024,  4*1024,        UFS_PART_3},/* PART_RESERVED10       4M     p18*/
  {PART_HISEE_FS,         544*1024,  8*1024,        UFS_PART_3},/* hisee_fs        8M     p19*/
  {PART_DFX,              552*1024,  16*1024,       UFS_PART_3},/* dfx             16M    p20*/
  {PART_RRECORD,          568*1024,  16*1024,       UFS_PART_3},/* rrecord         16M    p21*/
  {PART_FW_LPM3_A,        584*1024,  256,           UFS_PART_3},/* mcuimage        256K   p22*/
  {PART_RESERVED3_A,      598272,    3840,          UFS_PART_3},/*reserved3A      3840KB  p23*/
  {PART_HISEE_IMG_A,      588*1024,    4*1024,          UFS_PART_3},/*part_hisee_img_a   4*1024KB  p24*/
  {PART_FASTBOOT_A,       592*1024,  12*1024,       UFS_PART_3},/* fastboot        12M    p25*/
  {PART_VECTOR_A,         604*1024,  4*1024,        UFS_PART_3},/* avs vector      4M     p26*/
  {PART_ISP_BOOT_A,       608*1024,  2*1024,        UFS_PART_3},/* isp_boot        2M     p27*/
  {PART_ISP_FIRMWARE_A,   610*1024,  14*1024,       UFS_PART_3},/* isp_firmware    14M    p28*/
  {PART_FW_HIFI_A,        624*1024,  12*1024,       UFS_PART_3},/* hifi            12M    p29*/
  {PART_TEEOS_A,          636*1024,  8*1024,        UFS_PART_3},/* teeos           8M     p30*/
#ifdef CONFIG_SANITIZER_ENABLE
  {PART_ERECOVERY_KERNEL_A, 644*1024,  24*1024,     UFS_PART_3},/* erecovery_kernel 24M   p31*/
  {PART_ERECOVERY_RAMDISK_A, 668*1024,  32*1024,    UFS_PART_3},/* erecovery_ramdisk 32M  p32*/
  {PART_ERECOVERY_VENDOR_A, 700*1024,  8*1024,     UFS_PART_3},/* erecovery_vendor 8M     p33*/
  {PART_SENSORHUB_A,      708*1024,  16*1024,       UFS_PART_3},/* sensorhub       16M    p34*/
  {PART_KERNEL_A,           724*1024,  32*1024,     UFS_PART_3},/* kernel            32M  p35*/
#else
  {PART_ERECOVERY_KERNEL_A, 644*1024,  24*1024,     UFS_PART_3},/* erecovery_kernel 24M   p31*/
  {PART_ERECOVERY_RAMDISK_A, 668*1024,  32*1024,    UFS_PART_3},/* erecovery_ramdisk 32M  p32*/
  {PART_ERECOVERY_VENDOR_A, 700*1024,  16*1024,     UFS_PART_3},/* erecovery_vendor 16M   p33*/
  {PART_SENSORHUB_A,      716*1024,  16*1024,       UFS_PART_3},/* sensorhub       16M    p34*/
  {PART_KERNEL_A,           732*1024,  24*1024,     UFS_PART_3},/* kernel            24M    p35*/
#endif
  {PART_RAMDISK_A,        756*1024,  16*1024,       UFS_PART_3},/* ramdisk         16M    p36*/
  {PART_RECOVERY_RAMDISK_A, 772*1024,  32*1024,     UFS_PART_3},/* recovery_ramdisk 32M p37*/
  {PART_RECOVERY_VENDOR_A, 804*1024,  16*1024,      UFS_PART_3},/* recovery_vendor 16M   p38*/
  {PART_DTS_A,            820*1024,  14*1024,       UFS_PART_3},/* dtimage         14M    p39*/
  {PART_DTO_A,            834*1024,   2*1024,       UFS_PART_3},/* dtoimage         2M    p40*/
  {PART_TRUSTFIRMWARE_A,  836*1024,  2*1024,        UFS_PART_3},/* trustfirmware   2M     p41*/
  {PART_MODEM_FW_A,       838*1024,  56*1024,       UFS_PART_3},/* modem_fw        56M    p42*/
  {PART_RESERVED4_A,      894*1024,  42*1024,       UFS_PART_3},/* reserved4_A      42M    p43*/
  {PART_RECOVERY_VBMETA_A,  936*1024, 2*1024,       UFS_PART_3},/* recovery_vbmeta_a  2M    p44*/
  {PART_ERECOVERY_VBMETA_A, 938*1024, 2*1024,       UFS_PART_3},/* erecovery_vbmeta_a  2M    p45*/
  {PART_VBMETA_A,         940*1024,  4*1024,        UFS_PART_3},/* PART_VBMETA_A    4M    p46*/
  {PART_MODEMNVM_UPDATE_A,944*1024,  80*1024,       UFS_PART_3},/* modemnvm update 80M    p47*/
  {PART_PATCH_A,          1024*1024,  32*1024,      UFS_PART_3},/* patch           32M    p48*/
  {PART_VERSION_A,        1056*1024,  32*1024,      UFS_PART_3},/* version         24M    p49*/
  {PART_VENDOR_A,         1088*1024,  784*1024,     UFS_PART_3},/* vendor          784M   p50*/
  {PART_PRODUCT_A,        1872*1024, 192*1024,      UFS_PART_3},/* product         192M   p51*/
  {PART_CUST_A,           2064*1024, 192*1024,      UFS_PART_3},/* cust            192M   p52*/
  #ifdef CONFIG_MARKET_INTERNAL
  {PART_SYSTEM_A,         2256*1024, 3552*1024,     UFS_PART_3},/* system          3552M  p53*/
  {PART_RESERVED5,        5808*1024, 128*1024,      UFS_PART_3},/* reserved5       64M    p54*/
  {PART_USERDATA,         5936*1024, (4UL)*1024*1024,UFS_PART_3},/* userdata       4G     p55*/
  #else
  {PART_SYSTEM_A,         2256*1024, 4688*1024,     UFS_PART_3},/* system          4688M  p53*/
  {PART_RESERVED5,        6944*1024, 128*1024,      UFS_PART_3},/* reserved5       64M    p54*/
  {PART_USERDATA,         7072*1024, (4UL)*1024*1024,UFS_PART_3},/* userdata       4G     p55*/
  #endif
  {"0", 0, 0, 0},
};
#endif

#elif defined CONFIG_HISI_PARTITION_KIRIN970
static const struct partition partition_table_emmc[] =
{
  {PART_XLOADER_A,        0,         2*1024,        EMMC_BOOT_MAJOR_PART},
  {PART_XLOADER_B,        0,         2*1024,        EMMC_BOOT_BACKUP_PART},
  {PART_PTABLE,           0,         512,           EMMC_USER_PART},/* ptable          512K */
  {PART_FRP,              512,       512,           EMMC_USER_PART},/* frp             512K   p1*/
  {PART_PERSIST,          1024,      2048,          EMMC_USER_PART},/* persist         2048K  p2*/
  {PART_RESERVED1,        3072,      5120,          EMMC_USER_PART},/* reserved1       5120K  p3*/
  {PART_RESERVED6,        8*1024,    512,           EMMC_USER_PART},/* reserved6       512K   p4*/
  {PART_VRL,              8704,      512,           EMMC_USER_PART},/* vrl             512K   p5*/
  {PART_VRL_BACKUP,       9216,      512,           EMMC_USER_PART},/* vrl backup      512K   p6*/
  {PART_MODEM_SECURE,     9728,      8704,          EMMC_USER_PART},/* modem_secure    8704k  p7*/
  {PART_NVME,             18*1024,   5*1024,        EMMC_USER_PART},/* nvme            6M     p8*/
  {PART_CTF,              23*1024,  1*1024,       EMMC_USER_PART},/* PART_CTF       1M    p9*/
  {PART_OEMINFO,          24*1024,   64*1024,       EMMC_USER_PART},/* oeminfo         64M    p10*/
  {PART_SECURE_STORAGE,   88*1024,   32*1024,       EMMC_USER_PART},/* secure storage  32M    p11*/
  {PART_MODEM_OM,         120*1024,  32*1024,       EMMC_USER_PART},/* modem om        32M    p12*/
  {PART_MODEMNVM_FACTORY, 152*1024,  16*1024,        EMMC_USER_PART},/* modemnvm factory16M     p13*/
  {PART_MODEMNVM_BACKUP,  168*1024,  16*1024,        EMMC_USER_PART},/* modemnvm backup 16M     p14*/
  {PART_MODEMNVM_IMG,     184*1024,  20*1024,       EMMC_USER_PART},/* modemnvm img    20M    p15*/
  {PART_MODEMNVM_SYSTEM,  204*1024,  16*1024,        EMMC_USER_PART},/* modemnvm system 16M     p16*/
  {PART_HISEE_ENCOS,        220*1024,  4*1024,       EMMC_USER_PART},/* hisee_encos       4M    p17*/
  {PART_VERITYKEY,        224*1024,  1*1024,       EMMC_USER_PART},/* reserved2       32M    p18*/
  {PART_DDR_PARA,       225*1024,  1*1024,        EMMC_USER_PART},/* DDR_PARA        1M     p19*/
  {PART_RESERVED2,        226*1024,  27*1024,       EMMC_USER_PART},/* reserved2       32M    p20*/
  {PART_SPLASH2,          253*1024,  80*1024,       EMMC_USER_PART},/* splash2         80M    p21*/
  {PART_BOOTFAIL_INFO,    333*1024,  2*1024,        EMMC_USER_PART},/* bootfail info   2MB    p22*/
  {PART_MISC,             335*1024,  2*1024,        EMMC_USER_PART},/* misc            2M     p23*/
  {PART_DFX,              337*1024,  16*1024,       EMMC_USER_PART},/* dfx             16M    p24*/
  {PART_RRECORD,          353*1024,  16*1024,       EMMC_USER_PART},/* rrecord         16M    p25*/
  {PART_FW_LPM3_A,        369*1024,  256,           EMMC_USER_PART},/* mcuimage        256K   p26*/
  {PART_RESERVED3_A,      378112,    3840,          EMMC_USER_PART},/* reserved3A      3840KB p27*/
  {PART_HDCP_A,      373*1024,  1*1024,       EMMC_USER_PART},/* PART_HDCP_A      1M    p28*/
  {PART_HISEE_IMG_A,      374*1024,    4*1024,          EMMC_USER_PART},/* part_hisee_img_a   4*1024KB p29*/
  {PART_HHEE_A,        378*1024,  4*1024,        EMMC_USER_PART},/* PART_RESERVED10       4M     p30*/
  {PART_HISEE_FS_A,         382*1024,  8*1024,        EMMC_USER_PART},/* hisee_fs        8M     p31*/
  {PART_FASTBOOT_A,       390*1024,  12*1024,       EMMC_USER_PART},/* fastboot        12M    p32*/
  {PART_VECTOR_A,         402*1024,  4*1024,        EMMC_USER_PART},/* avs vector      4M     p33*/
  {PART_ISP_BOOT_A,       406*1024,  2*1024,        EMMC_USER_PART},/* isp_boot        2M     p34*/
  {PART_ISP_FIRMWARE_A,   408*1024,  14*1024,       EMMC_USER_PART},/* isp_firmware    14M    p35*/
  {PART_FW_HIFI_A,        422*1024,  12*1024,       EMMC_USER_PART},/* hifi            12M    p36*/
  {PART_TEEOS_A,          434*1024,  8*1024,        EMMC_USER_PART},/* teeos           8M     p37*/
  {PART_SENSORHUB_A,      442*1024,  16*1024,       EMMC_USER_PART},/* sensorhub       16M    p38*/
#ifdef CONFIG_SANITIZER_ENABLE
  {PART_ERECOVERY_KERNEL_A, 458*1024,  24*1024,     EMMC_USER_PART},/* erecovery_kernel  24M    p39*/
  {PART_ERECOVERY_RAMDISK_A, 482*1024,  32*1024,    EMMC_USER_PART},/* erecovery_ramdisk  32M  p40*/
  {PART_ERECOVERY_VENDOR_A, 514*1024,  8*1024,     EMMC_USER_PART},/* erecovery_vendor   8M  p41*/
  {PART_KERNEL_A,           522*1024,  32*1024,     EMMC_USER_PART},/* kernel           32M  p42*/
#else
  {PART_ERECOVERY_KERNEL_A, 458*1024,  24*1024,     EMMC_USER_PART},/* erecovery_kernel  24M    p39*/
  {PART_ERECOVERY_RAMDISK_A, 482*1024,  32*1024,    EMMC_USER_PART},/* erecovery_ramdisk 32M  p40*/
  {PART_ERECOVERY_VENDOR_A, 514*1024,  16*1024,     EMMC_USER_PART},/* erecovery_vendor 16M   p41*/
  {PART_KERNEL_A,           530*1024,  24*1024,     EMMC_USER_PART},/* kernel            24M    p42*/
#endif
  {PART_RAMDISK_A,        554*1024,  16*1024,       EMMC_USER_PART},/* ramdisk         16M    p43*/
  {PART_RECOVERY_RAMDISK_A, 570*1024,  32*1024,     EMMC_USER_PART},/* recovery_ramdisk 32M p44*/
  {PART_RECOVERY_VENDOR_A, 602*1024,  16*1024,      EMMC_USER_PART},/* recovery_vendor 16M   p45*/
  {PART_DTS_A,            618*1024,  12*1024,       EMMC_USER_PART},/* dtimage         12M    p46*/
  {PART_DTO_A,            630*1024,   4*1024,       EMMC_USER_PART},/* dtoimage        4M     p47*/
  {PART_TRUSTFIRMWARE_A,  634*1024,  2*1024,        EMMC_USER_PART},/* trustfirmware   2M     p48*/
  {PART_MODEM_FW_A,       636*1024,  56*1024,       EMMC_USER_PART},/* modem_fw        56M    p49*/
  {PART_RESERVED4_A,      692*1024,  20*1024,       EMMC_USER_PART},/* reserved4A      20M    p50*/
  {PART_RECOVERY_VBMETA_A,  712*1024, 2*1024,       EMMC_USER_PART},/* recovery_vbmeta_a   2M   p51*/
  {PART_ERECOVERY_VBMETA_A, 714*1024, 2*1024,       EMMC_USER_PART},/* erecovery_vbmeta_a  2M   p52*/
  {PART_VBMETA_A,       716*1024,  4*1024,       EMMC_USER_PART},/* PART_VBMETA_A       4M    p53*/
  {PART_MODEMNVM_UPDATE_A,720*1024,  16*1024,       EMMC_USER_PART},/* modemnvm update 16M    p54*/
  {PART_MODEMNVM_CUST_A,736*1024,  40*1024,      EMMC_USER_PART},/* modemnvm update cust 64M  p55*/
  {PART_PATCH_A,          776 *1024,  32*1024,      EMMC_USER_PART},/* patch           16M    p56*/
  {PART_VERSION_A,        808*1024,  32*1024,       EMMC_USER_PART},/* version         8M     p57*/
  {PART_VENDOR_A,         840*1024,  784*1024,      EMMC_USER_PART},/* vendor          704M   p58*/
  {PART_PRODUCT_A,        1624*1024, 192*1024,      EMMC_USER_PART},/* product         240M   p59*/
  {PART_CUST_A,           1816*1024, 192*1024,      EMMC_USER_PART},/* cust            256M   p60*/
  {PART_ODM_A,            2008*1024,  128*1024,     EMMC_USER_PART},/* odm             128M   p61*/
  {PART_CACHE,            2136*1024,  128*1024,     EMMC_USER_PART},/* cache           256M   p62*/
  #ifdef CONFIG_MARKET_INTERNAL
  {PART_SYSTEM_A,         2264*1024, 4752*1024,     EMMC_USER_PART},/* system          3697M  p63*/
  #ifdef CONFIG_FACTORY_MODE
  {PART_RESERVED5,        7016*1024, 128*1024,      EMMC_USER_PART},/* reserved5       128M   p64*/
  {PART_HIBENCH_DATA,   7144*1024, 512*1024,        EMMC_USER_PART},/* hibench_data    512M   p65*/
  {PART_USERDATA,         7656*1024, (4UL)*1024*1024,EMMC_USER_PART},/* userdata       4G     p66*/
  #else
  {PART_USERDATA,         7016*1024, (4UL)*1024*1024,EMMC_USER_PART},/* userdata       4G     p64*/
  #endif
  #else
  {PART_SYSTEM_A,         2264*1024, 5632*1024,     EMMC_USER_PART},/* system          4000M  p63*/
  #ifdef CONFIG_FACTORY_MODE
  {PART_RESERVED5,        7896*1024, 128*1024,       EMMC_USER_PART},/* reserved5      64M    p64*/
  {PART_HIBENCH_DATA,   8024*1024, 512*1024,       EMMC_USER_PART},/* hibench_data    512M    p65*/
  {PART_USERDATA,         8536*1024, (4UL)*1024*1024,EMMC_USER_PART},/* userdata        4G    p66*/
  #else
  {PART_USERDATA,         7896*1024, (4UL)*1024*1024,EMMC_USER_PART},/* userdata        4G    p64*/
  #endif
  #endif
  {"0", 0, 0, 0},                                        /* total 11848M*/
};
#ifdef CONFIG_HISI_STORAGE_UFS_PARTITION
static const struct partition partition_table_ufs[] =
{
  {PART_XLOADER_A,        0,         2*1024,        UFS_PART_0},
  {PART_XLOADER_B,        0,         2*1024,        UFS_PART_1},
  {PART_PTABLE,           0,         512,           UFS_PART_2},/* ptable          512K */
  {PART_FRP,              512,       512,           UFS_PART_2},/* frp             512K   p1*/
  {PART_PERSIST,          1024,      2048,          UFS_PART_2},/* persist         2048K  p2*/
  {PART_RESERVED1,        3072,      5120,          UFS_PART_2},/* reserved1       5120K  p3*/
  {PART_PTABLE_LU3,       0,         512,           UFS_PART_3},/* ptable_lu3      512K   p0*/
  {PART_VRL,              512,       512,           UFS_PART_3},/* vrl             512K   p1*/
  {PART_VRL_BACKUP,       1024,      512,           UFS_PART_3},/* vrl backup      512K   p2*/
  {PART_MODEM_SECURE,     1536,      8704,          UFS_PART_3},/* modem_secure    8704k  p3*/
  {PART_NVME,             10*1024,   5*1024,        UFS_PART_3},/* nvme            6M     p4*/
  {PART_CTF,              15*1024,  1*1024,       UFS_PART_3},/* PART_CTF       1M    p5*/
  {PART_OEMINFO,          16*1024,   64*1024,       UFS_PART_3},/* oeminfo         64M    p6*/
  {PART_SECURE_STORAGE,   80*1024,   32*1024,       UFS_PART_3},/* secure storage  32M    p7*/
  {PART_MODEM_OM,         112*1024,  32*1024,       UFS_PART_3},/* modem om        32M    p8*/
  {PART_MODEMNVM_FACTORY, 144*1024,  16*1024,        UFS_PART_3},/* modemnvm factory16M     p9*/
  {PART_MODEMNVM_BACKUP,  160*1024,  16*1024,        UFS_PART_3},/* modemnvm backup 16M     p10*/
  {PART_MODEMNVM_IMG,     176*1024,  20*1024,       UFS_PART_3},/* modemnvm img    20M    p11*/
  {PART_MODEMNVM_SYSTEM,  196*1024,  16*1024,        UFS_PART_3},/* modemnvm system 16M     p12*/
  {PART_HISEE_ENCOS,        212*1024,  4*1024,       UFS_PART_3},/* hisee_encos       4M    p13*/
  {PART_VERITYKEY,        216*1024,  1*1024,       UFS_PART_3},/* reserved2       32M    p14*/
  {PART_DDR_PARA,       217*1024,  1*1024,        UFS_PART_3},/* DDR_PARA        1M     p15*/
  {PART_RESERVED2,        218*1024,  27*1024,       UFS_PART_3},/* reserved2       32M    p16*/
  {PART_SPLASH2,          245*1024,  80*1024,       UFS_PART_3},/* splash2         80M    p17*/
  {PART_BOOTFAIL_INFO,    325*1024,  2*1024,        UFS_PART_3},/* bootfail info   2MB    p18*/
  {PART_MISC,             327*1024,  2*1024,        UFS_PART_3},/* misc            2M     p19*/
  {PART_DFX,              329*1024,  16*1024,       UFS_PART_3},/* dfx             16M    p20*/
  {PART_RRECORD,          345*1024,  16*1024,       UFS_PART_3},/* rrecord         16M    p21*/
  {PART_FW_LPM3_A,        361*1024,  256,           UFS_PART_3},/* mcuimage        256K   p22*/
  {PART_RESERVED3_A,      369920,    3840,          UFS_PART_3},/*reserved3A      3840KB  p23*/
  {PART_HDCP_A,      365*1024,  1*1024,       UFS_PART_3},/* PART_HDCP_A      1M    p24*/
  {PART_HISEE_IMG_A,      366*1024,    4*1024,          UFS_PART_3},/*part_hisee_img_a   4*1024KB  p25*/
  {PART_HHEE_A,        370*1024,  4*1024,        UFS_PART_3},/* PART_RESERVED10       4M     p26*/
  {PART_HISEE_FS_A,         374*1024,  8*1024,        UFS_PART_3},/* hisee_fs        8M     p27*/
  {PART_FASTBOOT_A,       382*1024,  12*1024,       UFS_PART_3},/* fastboot        12M    p28*/
  {PART_VECTOR_A,         394*1024,  4*1024,        UFS_PART_3},/* avs vector      4M     p29*/
  {PART_ISP_BOOT_A,       398*1024,  2*1024,        UFS_PART_3},/* isp_boot        2M     p30*/
  {PART_ISP_FIRMWARE_A,   400*1024,  14*1024,       UFS_PART_3},/* isp_firmware    14M    p31*/
  {PART_FW_HIFI_A,        414*1024,  12*1024,       UFS_PART_3},/* hifi            12M    p32*/
  {PART_TEEOS_A,          426*1024,  8*1024,        UFS_PART_3},/* teeos           8M     p33*/
  {PART_SENSORHUB_A,      434*1024,  16*1024,       UFS_PART_3},/* sensorhub       16M    p34*/
#ifdef CONFIG_SANITIZER_ENABLE
  {PART_ERECOVERY_KERNEL_A, 450*1024,  24*1024,     UFS_PART_3},/* erecovery_kernel 24M    p35*/
  {PART_ERECOVERY_RAMDISK_A, 474*1024,  32*1024,    UFS_PART_3},/* erecovery_ramdisk  32M  p36*/
  {PART_ERECOVERY_VENDOR_A, 506*1024,  8*1024,     UFS_PART_3},/* erecovery_vendor   8M    p37*/
  {PART_KERNEL_A,           514*1024,  32*1024,     UFS_PART_3},/* kernel           32M    p38*/
#else
  {PART_ERECOVERY_KERNEL_A, 450*1024,  24*1024,     UFS_PART_3},/* erecovery_kernel 24M    p35*/
  {PART_ERECOVERY_RAMDISK_A, 474*1024,  32*1024,    UFS_PART_3},/* erecovery_ramdisk 32M p36*/
  {PART_ERECOVERY_VENDOR_A, 506*1024,  16*1024,     UFS_PART_3},/* erecovery_vendor 16M p37*/
  {PART_KERNEL_A,           522*1024,  24*1024,     UFS_PART_3},/* kernel            24M    p38*/
#endif
  {PART_RAMDISK_A,        546*1024,  16*1024,       UFS_PART_3},/* ramdisk         16M    p39*/
  {PART_RECOVERY_RAMDISK_A, 562*1024,  32*1024,     UFS_PART_3},/* recovery_ramdisk 32M p40*/
  {PART_RECOVERY_VENDOR_A, 594*1024,  16*1024,      UFS_PART_3},/* recovery_vendor 16M   p41*/
  {PART_DTS_A,            610*1024,  12*1024,       UFS_PART_3},/* dtimage         12M    p42*/
  {PART_DTO_A,            622*1024,   4*1024,       UFS_PART_3},/* dtoimage         4M    p43*/
  {PART_TRUSTFIRMWARE_A,  626*1024,  2*1024,        UFS_PART_3},/* trustfirmware   2M     p44*/
  {PART_MODEM_FW_A,       628*1024,  56*1024,       UFS_PART_3},/* modem_fw        56M    p45*/
  {PART_RESERVED4_A,      684*1024,  20*1024,       UFS_PART_3},/* reserved4A       20M    p46*/
  {PART_RECOVERY_VBMETA_A,  704*1024,   2*1024,       UFS_PART_3},/* recovery_vbmeta_a   2M    p47*/
  {PART_ERECOVERY_VBMETA_A, 706*1024,   2*1024,       UFS_PART_3},/* erecovery_vbmeta_a  2M    p48*/
  {PART_VBMETA_A,        708*1024,  4*1024,       UFS_PART_3},/* PART_VBMETA_A      4M    p49*/
  {PART_MODEMNVM_UPDATE_A,712*1024,  16*1024,       UFS_PART_3},/* modemnvm update 16M    p50*/
  {PART_MODEMNVM_CUST_A,728*1024,  40*1024,       UFS_PART_3},/* modemnvm update cust 64M p51*/
  {PART_PATCH_A,          768*1024,  32*1024,       UFS_PART_3},/* patch           32M    p52*/
  {PART_VERSION_A,        800*1024,  32*1024,       UFS_PART_3},/* version        24M     p53*/
  {PART_VENDOR_A,         832*1024,  784*1024,      UFS_PART_3},/* vendor          784M   p54*/
  {PART_PRODUCT_A,        1616*1024, 192*1024,      UFS_PART_3},/* product         192M   p55*/
  {PART_CUST_A,           1808*1024, 192*1024,      UFS_PART_3},/* cust            192M   p56*/
  {PART_ODM_A,            2000*1024,  128*1024,     UFS_PART_3},/* odm            128M    p57*/
  {PART_CACHE,            2128*1024,  128*1024,     UFS_PART_3},/* cache          128M    p58*/
  #ifdef CONFIG_MARKET_INTERNAL
  {PART_SYSTEM_A,         2256*1024, 4752*1024,     UFS_PART_3},/* system          3552M  p59*/
  #ifdef CONFIG_FACTORY_MODE
  {PART_RESERVED5,        7008*1024, 128*1024,       UFS_PART_3},/* reserved5       128M  p60*/
  {PART_HIBENCH_DATA,   7136*1024, 512*1024,       UFS_PART_3},/* hibench_data      512M  p61*/
  {PART_USERDATA,         7648*1024, (4UL)*1024*1024,UFS_PART_3},/* userdata       4G     p62*/
  #else
  {PART_USERDATA,         7008*1024, (4UL)*1024*1024,UFS_PART_3},/* userdata       4G     p60*/
  #endif
  #else
  {PART_SYSTEM_A,         2256*1024, 5632*1024,     UFS_PART_3},/* system          4688M  p59*/
  #ifdef CONFIG_FACTORY_MODE
  {PART_RESERVED5,        7888*1024, 128*1024,       UFS_PART_3},/* reserved5     128M    p60*/
  {PART_HIBENCH_DATA,   8016*1024, 512*1024,       UFS_PART_3},/* hibench_data    512M    p61*/
  {PART_USERDATA,         8528*1024, (4UL)*1024*1024,UFS_PART_3},/* userdata       4G     p62*/
  #else
  {PART_USERDATA,         7888*1024, (4UL)*1024*1024,UFS_PART_3},/* userdata       4G     p60*/
  #endif
  #endif
  {"0", 0, 0, 0},
};
#endif
#endif
#endif