#ifndef __SOC_PCTRL_INTERFACE_H__
#define __SOC_PCTRL_INTERFACE_H__ 
#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif
#define SOC_PCTRL_G3D_RASTER_ADDR(base) ((base) + (0x000))
#define SOC_PCTRL_PERI_CTRL0_ADDR(base) ((base) + (0x004))
#define SOC_PCTRL_PERI_CTRL1_ADDR(base) ((base) + (0x008))
#define SOC_PCTRL_PERI_CTRL2_ADDR(base) ((base) + (0x00C))
#define SOC_PCTRL_PERI_CTRL3_ADDR(base) ((base) + (0x010))
#define SOC_PCTRL_PERI_CTRL4_ADDR(base) ((base) + (0x014))
#define SOC_PCTRL_PERI_CTRL12_ADDR(base) ((base) + (0x034))
#define SOC_PCTRL_PERI_CTRL13_ADDR(base) ((base) + (0x038))
#define SOC_PCTRL_PERI_CTRL14_ADDR(base) ((base) + (0x03C))
#define SOC_PCTRL_PERI_CTRL15_ADDR(base) ((base) + (0x040))
#define SOC_PCTRL_PERI_CTRL16_ADDR(base) ((base) + (0x044))
#define SOC_PCTRL_PERI_CTRL17_ADDR(base) ((base) + (0x048))
#define SOC_PCTRL_PERI_CTRL18_ADDR(base) ((base) + (0x04C))
#define SOC_PCTRL_PERI_CTRL19_ADDR(base) ((base) + (0x050))
#define SOC_PCTRL_PERI_CTRL20_ADDR(base) ((base) + (0x054))
#define SOC_PCTRL_PERI_CTRL21_ADDR(base) ((base) + (0x058))
#define SOC_PCTRL_PERI_CTRL22_ADDR(base) ((base) + (0x05C))
#define SOC_PCTRL_PERI_CTRL23_ADDR(base) ((base) + (0x060))
#define SOC_PCTRL_PERI_CTRL24_ADDR(base) ((base) + (0x064))
#define SOC_PCTRL_PERI_CTRL25_ADDR(base) ((base) + (0x068))
#define SOC_PCTRL_PERI_CTRL26_ADDR(base) ((base) + (0x06C))
#define SOC_PCTRL_PERI_CTRL27_ADDR(base) ((base) + (0x070))
#define SOC_PCTRL_PERI_CTRL28_ADDR(base) ((base) + (0x074))
#define SOC_PCTRL_PERI_CTRL29_ADDR(base) ((base) + (0x078))
#define SOC_PCTRL_PERI_CTRL30_ADDR(base) ((base) + (0x07C))
#define SOC_PCTRL_PERI_CTRL31_ADDR(base) ((base) + (0x080))
#define SOC_PCTRL_PERI_CTRL32_ADDR(base) ((base) + (0x084))
#define SOC_PCTRL_PERI_CTRL33_ADDR(base) ((base) + (0x088))
#define SOC_PCTRL_PERI_STAT0_ADDR(base) ((base) + (0x094))
#define SOC_PCTRL_PERI_STAT1_ADDR(base) ((base) + (0x098))
#define SOC_PCTRL_PERI_STAT2_ADDR(base) ((base) + (0x09C))
#define SOC_PCTRL_PERI_STAT3_ADDR(base) ((base) + (0x0A0))
#define SOC_PCTRL_PERI_STAT4_ADDR(base) ((base) + (0x0A4))
#define SOC_PCTRL_PERI_STAT5_ADDR(base) ((base) + (0x0A8))
#define SOC_PCTRL_PERI_STAT6_ADDR(base) ((base) + (0x0AC))
#define SOC_PCTRL_PERI_STAT7_ADDR(base) ((base) + (0x0B0))
#define SOC_PCTRL_PERI_STAT8_ADDR(base) ((base) + (0x0B4))
#define SOC_PCTRL_PERI_STAT9_ADDR(base) ((base) + (0x0B8))
#define SOC_PCTRL_PERI_STAT10_ADDR(base) ((base) + (0x0BC))
#define SOC_PCTRL_PERI_STAT11_ADDR(base) ((base) + (0x0C0))
#define SOC_PCTRL_PERI_STAT12_ADDR(base) ((base) + (0x0C4))
#define SOC_PCTRL_PERI_STAT13_ADDR(base) ((base) + (0x0C8))
#define SOC_PCTRL_PERI_STAT14_ADDR(base) ((base) + (0x0CC))
#define SOC_PCTRL_PERI_STAT15_ADDR(base) ((base) + (0x0D0))
#define SOC_PCTRL_PERI_STAT16_ADDR(base) ((base) + (0x0D4))
#define SOC_PCTRL_PERI_STAT17_ADDR(base) ((base) + (0x0D8))
#define SOC_PCTRL_PERI_STAT18_ADDR(base) ((base) + (0x0DC))
#define SOC_PCTRL_PERI_STAT19_ADDR(base) ((base) + (0x0E0))
#define SOC_PCTRL_USB2_HOST_CTRL0_ADDR(base) ((base) + (0x0F0))
#define SOC_PCTRL_USB2_HOST_CTRL1_ADDR(base) ((base) + (0x0F4))
#define SOC_PCTRL_USB2_HOST_CTRL2_ADDR(base) ((base) + (0x0F8))
#define SOC_PCTRL_RESOURCE0_LOCK_ADDR(base) ((base) + (0x400))
#define SOC_PCTRL_RESOURCE0_UNLOCK_ADDR(base) ((base) + (0x404))
#define SOC_PCTRL_RESOURCE0_LOCK_ST_ADDR(base) ((base) + (0x408))
#define SOC_PCTRL_RESOURCE1_LOCK_ADDR(base) ((base) + (0x40C))
#define SOC_PCTRL_RESOURCE1_UNLOCK_ADDR(base) ((base) + (0x410))
#define SOC_PCTRL_RESOURCE1_LOCK_ST_ADDR(base) ((base) + (0x414))
#define SOC_PCTRL_RESOURCE2_LOCK_ADDR(base) ((base) + (0x418))
#define SOC_PCTRL_RESOURCE2_UNLOCK_ADDR(base) ((base) + (0x41C))
#define SOC_PCTRL_RESOURCE2_LOCK_ST_ADDR(base) ((base) + (0x420))
#define SOC_PCTRL_RESOURCE3_LOCK_ADDR(base) ((base) + (0x424))
#define SOC_PCTRL_RESOURCE3_UNLOCK_ADDR(base) ((base) + (0x428))
#define SOC_PCTRL_RESOURCE3_LOCK_ST_ADDR(base) ((base) + (0x42C))
#define SOC_PCTRL_RESOURCE4_LOCK_ADDR(base) ((base) + (0x800))
#define SOC_PCTRL_RESOURCE4_UNLOCK_ADDR(base) ((base) + (0x804))
#define SOC_PCTRL_RESOURCE4_LOCK_ST_ADDR(base) ((base) + (0x808))
#define SOC_PCTRL_RESOURCE5_LOCK_ADDR(base) ((base) + (0x80C))
#define SOC_PCTRL_RESOURCE5_UNLOCK_ADDR(base) ((base) + (0x810))
#define SOC_PCTRL_RESOURCE5_LOCK_ST_ADDR(base) ((base) + (0x814))
#define SOC_PCTRL_RESOURCE6_LOCK_ADDR(base) ((base) + (0x818))
#define SOC_PCTRL_RESOURCE6_UNLOCK_ADDR(base) ((base) + (0x81C))
#define SOC_PCTRL_RESOURCE6_LOCK_ST_ADDR(base) ((base) + (0x820))
#define SOC_PCTRL_RESOURCE7_LOCK_ADDR(base) ((base) + (0x824))
#define SOC_PCTRL_RESOURCE7_UNLOCK_ADDR(base) ((base) + (0x828))
#define SOC_PCTRL_RESOURCE7_LOCK_ST_ADDR(base) ((base) + (0x82C))
#define SOC_PCTRL_PERI_CTRL5_ADDR(base) ((base) + (0xC00))
#define SOC_PCTRL_PERI_CTRL6_ADDR(base) ((base) + (0xC04))
#define SOC_PCTRL_PERI_CTRL7_ADDR(base) ((base) + (0xC08))
#define SOC_PCTRL_PERI_CTRL8_ADDR(base) ((base) + (0xC0C))
#define SOC_PCTRL_PERI_CTRL9_ADDR(base) ((base) + (0xC10))
#define SOC_PCTRL_PERI_CTRL10_ADDR(base) ((base) + (0xC14))
#define SOC_PCTRL_PERI_CTRL11_ADDR(base) ((base) + (0xC18))
#define SOC_PCTRL_PERI_CTRL34_ADDR(base) ((base) + (0xC1C))
#define SOC_PCTRL_PERI_CTRL35_ADDR(base) ((base) + (0xC20))
#define SOC_PCTRL_PERI_CTRL38_ADDR(base) ((base) + (0xC2C))
#define SOC_PCTRL_PERI_CTRL39_ADDR(base) ((base) + (0xC30))
#define SOC_PCTRL_PERI_CTRL40_ADDR(base) ((base) + (0xC34))
#define SOC_PCTRL_PERI_CTRL41_ADDR(base) ((base) + (0xC38))
#define SOC_PCTRL_PERI_CTRL42_ADDR(base) ((base) + (0xC3C))
#define SOC_PCTRL_PERI_CTRL43_ADDR(base) ((base) + (0xC40))
#define SOC_PCTRL_PERI_CTRL44_ADDR(base) ((base) + (0xC44))
#define SOC_PCTRL_PERI_CTRL48_ADDR(base) ((base) + (0xC54))
#define SOC_PCTRL_PERI_CTRL49_ADDR(base) ((base) + (0xC58))
#define SOC_PCTRL_PERI_CTRL50_ADDR(base) ((base) + (0xC5C))
#define SOC_PCTRL_PERI_CTRL51_ADDR(base) ((base) + (0xC60))
#define SOC_PCTRL_PERI_CTRL52_ADDR(base) ((base) + (0xC64))
#define SOC_PCTRL_PERI_CTRL53_ADDR(base) ((base) + (0xC68))
#define SOC_PCTRL_PERI_CTRL54_ADDR(base) ((base) + (0xC6C))
#define SOC_PCTRL_PERI_CTRL55_ADDR(base) ((base) + (0xC70))
#define SOC_PCTRL_PERI_CTRL56_ADDR(base) ((base) + (0xC74))
#define SOC_PCTRL_PERI_CTRL57_ADDR(base) ((base) + (0xC78))
#define SOC_PCTRL_PERI_CTRL58_ADDR(base) ((base) + (0xC7C))
#define SOC_PCTRL_PERI_CTRL59_ADDR(base) ((base) + (0xC80))
#define SOC_PCTRL_PERI_CTRL60_ADDR(base) ((base) + (0xC84))
#define SOC_PCTRL_PERI_CTRL61_ADDR(base) ((base) + (0xC88))
#define SOC_PCTRL_PERI_CTRL62_ADDR(base) ((base) + (0xC8C))
#define SOC_PCTRL_PERI_CTRL63_ADDR(base) ((base) + (0xC90))
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int g3d_div : 10;
        unsigned int portrait_landscape : 1;
        unsigned int lcd_3d_2d : 1;
        unsigned int g3d_raster_en : 1;
        unsigned int lcd_3d_sw_inv : 4;
        unsigned int reserved : 15;
    } reg;
} SOC_PCTRL_G3D_RASTER_UNION;
#endif
#define SOC_PCTRL_G3D_RASTER_g3d_div_START (0)
#define SOC_PCTRL_G3D_RASTER_g3d_div_END (9)
#define SOC_PCTRL_G3D_RASTER_portrait_landscape_START (10)
#define SOC_PCTRL_G3D_RASTER_portrait_landscape_END (10)
#define SOC_PCTRL_G3D_RASTER_lcd_3d_2d_START (11)
#define SOC_PCTRL_G3D_RASTER_lcd_3d_2d_END (11)
#define SOC_PCTRL_G3D_RASTER_g3d_raster_en_START (12)
#define SOC_PCTRL_G3D_RASTER_g3d_raster_en_END (12)
#define SOC_PCTRL_G3D_RASTER_lcd_3d_sw_inv_START (13)
#define SOC_PCTRL_G3D_RASTER_lcd_3d_sw_inv_END (16)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sc_1wire_e : 1;
        unsigned int peri_ctrl0_cmd : 15;
        unsigned int peri_ctrl0_msk : 16;
    } reg;
} SOC_PCTRL_PERI_CTRL0_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL0_sc_1wire_e_START (0)
#define SOC_PCTRL_PERI_CTRL0_sc_1wire_e_END (0)
#define SOC_PCTRL_PERI_CTRL0_peri_ctrl0_cmd_START (1)
#define SOC_PCTRL_PERI_CTRL0_peri_ctrl0_cmd_END (15)
#define SOC_PCTRL_PERI_CTRL0_peri_ctrl0_msk_START (16)
#define SOC_PCTRL_PERI_CTRL0_peri_ctrl0_msk_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int test_point_sel : 7;
        unsigned int peri_ctrl0_cmd : 9;
        unsigned int peri_ctrl1_msk : 16;
    } reg;
} SOC_PCTRL_PERI_CTRL1_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL1_test_point_sel_START (0)
#define SOC_PCTRL_PERI_CTRL1_test_point_sel_END (6)
#define SOC_PCTRL_PERI_CTRL1_peri_ctrl0_cmd_START (7)
#define SOC_PCTRL_PERI_CTRL1_peri_ctrl0_cmd_END (15)
#define SOC_PCTRL_PERI_CTRL1_peri_ctrl1_msk_START (16)
#define SOC_PCTRL_PERI_CTRL1_peri_ctrl1_msk_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int peri_ctrl2_cmd : 16;
        unsigned int peri_ctrl2_msk : 16;
    } reg;
} SOC_PCTRL_PERI_CTRL2_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL2_peri_ctrl2_cmd_START (0)
#define SOC_PCTRL_PERI_CTRL2_peri_ctrl2_cmd_END (15)
#define SOC_PCTRL_PERI_CTRL2_peri_ctrl2_msk_START (16)
#define SOC_PCTRL_PERI_CTRL2_peri_ctrl2_msk_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int ufs_tcxo_en : 1;
        unsigned int usb_tcxo_en : 1;
        unsigned int reserved : 14;
        unsigned int peri_ctrl3_msk : 16;
    } reg;
} SOC_PCTRL_PERI_CTRL3_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL3_ufs_tcxo_en_START (0)
#define SOC_PCTRL_PERI_CTRL3_ufs_tcxo_en_END (0)
#define SOC_PCTRL_PERI_CTRL3_usb_tcxo_en_START (1)
#define SOC_PCTRL_PERI_CTRL3_usb_tcxo_en_END (1)
#define SOC_PCTRL_PERI_CTRL3_peri_ctrl3_msk_START (16)
#define SOC_PCTRL_PERI_CTRL3_peri_ctrl3_msk_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved : 32;
    } reg;
} SOC_PCTRL_PERI_CTRL4_UNION;
#endif
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved : 32;
    } reg;
} SOC_PCTRL_PERI_CTRL12_UNION;
#endif
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved : 32;
    } reg;
} SOC_PCTRL_PERI_CTRL13_UNION;
#endif
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sc_pctrl_cohe1 : 32;
    } reg;
} SOC_PCTRL_PERI_CTRL14_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL14_sc_pctrl_cohe1_START (0)
#define SOC_PCTRL_PERI_CTRL14_sc_pctrl_cohe1_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sc_pctrl_cohe2 : 32;
    } reg;
} SOC_PCTRL_PERI_CTRL15_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL15_sc_pctrl_cohe2_START (0)
#define SOC_PCTRL_PERI_CTRL15_sc_pctrl_cohe2_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int spi4_cs_sel : 4;
        unsigned int spi3_cs_sel : 4;
        unsigned int reserved : 24;
    } reg;
} SOC_PCTRL_PERI_CTRL16_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL16_spi4_cs_sel_START (0)
#define SOC_PCTRL_PERI_CTRL16_spi4_cs_sel_END (3)
#define SOC_PCTRL_PERI_CTRL16_spi3_cs_sel_START (4)
#define SOC_PCTRL_PERI_CTRL16_spi3_cs_sel_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int modem_debug_sel : 8;
        unsigned int reserved : 24;
    } reg;
} SOC_PCTRL_PERI_CTRL17_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL17_modem_debug_sel_START (0)
#define SOC_PCTRL_PERI_CTRL17_modem_debug_sel_END (7)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved : 32;
    } reg;
} SOC_PCTRL_PERI_CTRL18_UNION;
#endif
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int g3d_drm_mode_cfg : 1;
        unsigned int reserved_0 : 1;
        unsigned int reserved_1 : 1;
        unsigned int reserved_2 : 1;
        unsigned int g3d_pw_sel : 1;
        unsigned int g3d_slp_sel : 1;
        unsigned int reserved_3 : 3;
        unsigned int gpu_striping_granule : 3;
        unsigned int reserved_4 : 3;
        unsigned int reserved_5 : 3;
        unsigned int reserved_6 : 1;
        unsigned int reserved_7 : 1;
        unsigned int reserved_8 : 1;
        unsigned int reserved_9 : 1;
        unsigned int reserved_10 : 1;
        unsigned int reserved_11 : 1;
        unsigned int gpu_x2p_gatedclock_en : 1;
        unsigned int reserved_12 : 1;
        unsigned int reserved_13 : 1;
        unsigned int reserved_14 : 1;
        unsigned int reserved_15 : 1;
        unsigned int reserved_16 : 1;
        unsigned int reserved_17 : 1;
        unsigned int reserved_18 : 1;
    } reg;
} SOC_PCTRL_PERI_CTRL19_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL19_g3d_drm_mode_cfg_START (0)
#define SOC_PCTRL_PERI_CTRL19_g3d_drm_mode_cfg_END (0)
#define SOC_PCTRL_PERI_CTRL19_g3d_pw_sel_START (4)
#define SOC_PCTRL_PERI_CTRL19_g3d_pw_sel_END (4)
#define SOC_PCTRL_PERI_CTRL19_g3d_slp_sel_START (5)
#define SOC_PCTRL_PERI_CTRL19_g3d_slp_sel_END (5)
#define SOC_PCTRL_PERI_CTRL19_gpu_striping_granule_START (9)
#define SOC_PCTRL_PERI_CTRL19_gpu_striping_granule_END (11)
#define SOC_PCTRL_PERI_CTRL19_gpu_x2p_gatedclock_en_START (24)
#define SOC_PCTRL_PERI_CTRL19_gpu_x2p_gatedclock_en_END (24)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved : 32;
    } reg;
} SOC_PCTRL_PERI_CTRL20_UNION;
#endif
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int g3d_tprf_uhd_rm : 4;
        unsigned int g3d_tprf_uhd_rme : 1;
        unsigned int g3d_tprf_uhd_wa : 3;
        unsigned int g3d_tprf_uhd_ra : 2;
        unsigned int g3d_tprf_uhd_wpulse : 3;
        unsigned int reserved : 19;
    } reg;
} SOC_PCTRL_PERI_CTRL21_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL21_g3d_tprf_uhd_rm_START (0)
#define SOC_PCTRL_PERI_CTRL21_g3d_tprf_uhd_rm_END (3)
#define SOC_PCTRL_PERI_CTRL21_g3d_tprf_uhd_rme_START (4)
#define SOC_PCTRL_PERI_CTRL21_g3d_tprf_uhd_rme_END (4)
#define SOC_PCTRL_PERI_CTRL21_g3d_tprf_uhd_wa_START (5)
#define SOC_PCTRL_PERI_CTRL21_g3d_tprf_uhd_wa_END (7)
#define SOC_PCTRL_PERI_CTRL21_g3d_tprf_uhd_ra_START (8)
#define SOC_PCTRL_PERI_CTRL21_g3d_tprf_uhd_ra_END (9)
#define SOC_PCTRL_PERI_CTRL21_g3d_tprf_uhd_wpulse_START (10)
#define SOC_PCTRL_PERI_CTRL21_g3d_tprf_uhd_wpulse_END (12)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved : 32;
    } reg;
} SOC_PCTRL_PERI_CTRL22_UNION;
#endif
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved_0 : 1;
        unsigned int reserved_1 : 1;
        unsigned int dmac_ckgt_dis : 1;
        unsigned int pctrl_dphytx_ulpsexit0 : 1;
        unsigned int pctrl_dphytx_ulpsexit1 : 1;
        unsigned int reserved_2 : 27;
    } reg;
} SOC_PCTRL_PERI_CTRL23_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL23_dmac_ckgt_dis_START (2)
#define SOC_PCTRL_PERI_CTRL23_dmac_ckgt_dis_END (2)
#define SOC_PCTRL_PERI_CTRL23_pctrl_dphytx_ulpsexit0_START (3)
#define SOC_PCTRL_PERI_CTRL23_pctrl_dphytx_ulpsexit0_END (3)
#define SOC_PCTRL_PERI_CTRL23_pctrl_dphytx_ulpsexit1_START (4)
#define SOC_PCTRL_PERI_CTRL23_pctrl_dphytx_ulpsexit1_END (4)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int pmussi0_mst_cnt : 3;
        unsigned int djtag_mst_pstrb : 4;
        unsigned int i2c_freq_sel : 1;
        unsigned int reserved_0 : 1;
        unsigned int sdio0_resp_ctrl : 1;
        unsigned int sd3_resp_ctrl : 1;
        unsigned int reserved_1 : 1;
        unsigned int lpmcu_resp_ctrl : 1;
        unsigned int iomcu_cfgbus_resp_ctrl : 1;
        unsigned int iomcu_sysbus_resp_ctrl : 1;
        unsigned int isp_axi_xdcdr_sel : 1;
        unsigned int pmussi1_mst_cnt : 3;
        unsigned int reserved_2 : 1;
        unsigned int codecssi_mst_cnt : 3;
        unsigned int reserved_3 : 1;
        unsigned int reserved_4 : 1;
        unsigned int sc_clk_usb3phy_3mux1_sel : 2;
        unsigned int reserved_5 : 1;
        unsigned int reserved_6 : 1;
        unsigned int pmussi2_mst_cnt : 3;
    } reg;
} SOC_PCTRL_PERI_CTRL24_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL24_pmussi0_mst_cnt_START (0)
#define SOC_PCTRL_PERI_CTRL24_pmussi0_mst_cnt_END (2)
#define SOC_PCTRL_PERI_CTRL24_djtag_mst_pstrb_START (3)
#define SOC_PCTRL_PERI_CTRL24_djtag_mst_pstrb_END (6)
#define SOC_PCTRL_PERI_CTRL24_i2c_freq_sel_START (7)
#define SOC_PCTRL_PERI_CTRL24_i2c_freq_sel_END (7)
#define SOC_PCTRL_PERI_CTRL24_sdio0_resp_ctrl_START (9)
#define SOC_PCTRL_PERI_CTRL24_sdio0_resp_ctrl_END (9)
#define SOC_PCTRL_PERI_CTRL24_sd3_resp_ctrl_START (10)
#define SOC_PCTRL_PERI_CTRL24_sd3_resp_ctrl_END (10)
#define SOC_PCTRL_PERI_CTRL24_lpmcu_resp_ctrl_START (12)
#define SOC_PCTRL_PERI_CTRL24_lpmcu_resp_ctrl_END (12)
#define SOC_PCTRL_PERI_CTRL24_iomcu_cfgbus_resp_ctrl_START (13)
#define SOC_PCTRL_PERI_CTRL24_iomcu_cfgbus_resp_ctrl_END (13)
#define SOC_PCTRL_PERI_CTRL24_iomcu_sysbus_resp_ctrl_START (14)
#define SOC_PCTRL_PERI_CTRL24_iomcu_sysbus_resp_ctrl_END (14)
#define SOC_PCTRL_PERI_CTRL24_isp_axi_xdcdr_sel_START (15)
#define SOC_PCTRL_PERI_CTRL24_isp_axi_xdcdr_sel_END (15)
#define SOC_PCTRL_PERI_CTRL24_pmussi1_mst_cnt_START (16)
#define SOC_PCTRL_PERI_CTRL24_pmussi1_mst_cnt_END (18)
#define SOC_PCTRL_PERI_CTRL24_codecssi_mst_cnt_START (20)
#define SOC_PCTRL_PERI_CTRL24_codecssi_mst_cnt_END (22)
#define SOC_PCTRL_PERI_CTRL24_sc_clk_usb3phy_3mux1_sel_START (25)
#define SOC_PCTRL_PERI_CTRL24_sc_clk_usb3phy_3mux1_sel_END (26)
#define SOC_PCTRL_PERI_CTRL24_pmussi2_mst_cnt_START (29)
#define SOC_PCTRL_PERI_CTRL24_pmussi2_mst_cnt_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved_0 : 8;
        unsigned int secp_mem_ctrl_sd : 1;
        unsigned int secs_mem_ctrl_sd : 1;
        unsigned int mmbuf_sram_sd : 1;
        unsigned int smmu_integ_sec_override : 1;
        unsigned int reserved_1 : 1;
        unsigned int reserved_2 : 1;
        unsigned int reserved_3 : 1;
        unsigned int reserved_4 : 1;
        unsigned int reserved_5 : 1;
        unsigned int sc_g3d_dw_axi_m1_cg_en : 1;
        unsigned int sc_g3d_dw_axi_s0_cg_en : 1;
        unsigned int sc_g3d_dw_axi_s1_cg_en : 1;
        unsigned int sc_g3d_dw_axi_s2_cg_en : 1;
        unsigned int reserved_6 : 6;
        unsigned int reserved_7 : 1;
        unsigned int reserved_8 : 1;
        unsigned int sc_modem_ipc_auto_clk_gate_en : 1;
        unsigned int sc_modem_ipc_soft_gate_clk_en : 1;
        unsigned int reserved_9 : 1;
    } reg;
} SOC_PCTRL_PERI_CTRL25_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL25_secp_mem_ctrl_sd_START (8)
#define SOC_PCTRL_PERI_CTRL25_secp_mem_ctrl_sd_END (8)
#define SOC_PCTRL_PERI_CTRL25_secs_mem_ctrl_sd_START (9)
#define SOC_PCTRL_PERI_CTRL25_secs_mem_ctrl_sd_END (9)
#define SOC_PCTRL_PERI_CTRL25_mmbuf_sram_sd_START (10)
#define SOC_PCTRL_PERI_CTRL25_mmbuf_sram_sd_END (10)
#define SOC_PCTRL_PERI_CTRL25_smmu_integ_sec_override_START (11)
#define SOC_PCTRL_PERI_CTRL25_smmu_integ_sec_override_END (11)
#define SOC_PCTRL_PERI_CTRL25_sc_g3d_dw_axi_m1_cg_en_START (17)
#define SOC_PCTRL_PERI_CTRL25_sc_g3d_dw_axi_m1_cg_en_END (17)
#define SOC_PCTRL_PERI_CTRL25_sc_g3d_dw_axi_s0_cg_en_START (18)
#define SOC_PCTRL_PERI_CTRL25_sc_g3d_dw_axi_s0_cg_en_END (18)
#define SOC_PCTRL_PERI_CTRL25_sc_g3d_dw_axi_s1_cg_en_START (19)
#define SOC_PCTRL_PERI_CTRL25_sc_g3d_dw_axi_s1_cg_en_END (19)
#define SOC_PCTRL_PERI_CTRL25_sc_g3d_dw_axi_s2_cg_en_START (20)
#define SOC_PCTRL_PERI_CTRL25_sc_g3d_dw_axi_s2_cg_en_END (20)
#define SOC_PCTRL_PERI_CTRL25_sc_modem_ipc_auto_clk_gate_en_START (29)
#define SOC_PCTRL_PERI_CTRL25_sc_modem_ipc_auto_clk_gate_en_END (29)
#define SOC_PCTRL_PERI_CTRL25_sc_modem_ipc_soft_gate_clk_en_START (30)
#define SOC_PCTRL_PERI_CTRL25_sc_modem_ipc_soft_gate_clk_en_END (30)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int pack_dbg_mode_en : 4;
        unsigned int pack_dbg_mode_sel : 5;
        unsigned int reserved_0 : 5;
        unsigned int isp_dw_axi_gatedclock_en : 1;
        unsigned int ivp32_dw_axi_gatedclock_en : 1;
        unsigned int spi_tprf_slp : 1;
        unsigned int spi_tprf_dslp : 1;
        unsigned int spi_tprf_sd : 1;
        unsigned int g3d_tprf_slp : 1;
        unsigned int g3d_tprf_dslp : 1;
        unsigned int g3d_tprf_sd : 1;
        unsigned int g3d_spsram_slp : 1;
        unsigned int g3d_spsram_dslp : 1;
        unsigned int g3d_spsram_sd : 1;
        unsigned int reserved_1 : 7;
    } reg;
} SOC_PCTRL_PERI_CTRL26_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL26_pack_dbg_mode_en_START (0)
#define SOC_PCTRL_PERI_CTRL26_pack_dbg_mode_en_END (3)
#define SOC_PCTRL_PERI_CTRL26_pack_dbg_mode_sel_START (4)
#define SOC_PCTRL_PERI_CTRL26_pack_dbg_mode_sel_END (8)
#define SOC_PCTRL_PERI_CTRL26_isp_dw_axi_gatedclock_en_START (14)
#define SOC_PCTRL_PERI_CTRL26_isp_dw_axi_gatedclock_en_END (14)
#define SOC_PCTRL_PERI_CTRL26_ivp32_dw_axi_gatedclock_en_START (15)
#define SOC_PCTRL_PERI_CTRL26_ivp32_dw_axi_gatedclock_en_END (15)
#define SOC_PCTRL_PERI_CTRL26_spi_tprf_slp_START (16)
#define SOC_PCTRL_PERI_CTRL26_spi_tprf_slp_END (16)
#define SOC_PCTRL_PERI_CTRL26_spi_tprf_dslp_START (17)
#define SOC_PCTRL_PERI_CTRL26_spi_tprf_dslp_END (17)
#define SOC_PCTRL_PERI_CTRL26_spi_tprf_sd_START (18)
#define SOC_PCTRL_PERI_CTRL26_spi_tprf_sd_END (18)
#define SOC_PCTRL_PERI_CTRL26_g3d_tprf_slp_START (19)
#define SOC_PCTRL_PERI_CTRL26_g3d_tprf_slp_END (19)
#define SOC_PCTRL_PERI_CTRL26_g3d_tprf_dslp_START (20)
#define SOC_PCTRL_PERI_CTRL26_g3d_tprf_dslp_END (20)
#define SOC_PCTRL_PERI_CTRL26_g3d_tprf_sd_START (21)
#define SOC_PCTRL_PERI_CTRL26_g3d_tprf_sd_END (21)
#define SOC_PCTRL_PERI_CTRL26_g3d_spsram_slp_START (22)
#define SOC_PCTRL_PERI_CTRL26_g3d_spsram_slp_END (22)
#define SOC_PCTRL_PERI_CTRL26_g3d_spsram_dslp_START (23)
#define SOC_PCTRL_PERI_CTRL26_g3d_spsram_dslp_END (23)
#define SOC_PCTRL_PERI_CTRL26_g3d_spsram_sd_START (24)
#define SOC_PCTRL_PERI_CTRL26_g3d_spsram_sd_END (24)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int isp_sft_fiq : 1;
        unsigned int isp_sys_ctrl_0 : 31;
    } reg;
} SOC_PCTRL_PERI_CTRL27_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL27_isp_sft_fiq_START (0)
#define SOC_PCTRL_PERI_CTRL27_isp_sft_fiq_END (0)
#define SOC_PCTRL_PERI_CTRL27_isp_sys_ctrl_0_START (1)
#define SOC_PCTRL_PERI_CTRL27_isp_sys_ctrl_0_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int isp_sys_ctrl_1 : 32;
    } reg;
} SOC_PCTRL_PERI_CTRL28_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL28_isp_sys_ctrl_1_START (0)
#define SOC_PCTRL_PERI_CTRL28_isp_sys_ctrl_1_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int pctrl_dphytx_stopcnt0 : 32;
    } reg;
} SOC_PCTRL_PERI_CTRL29_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL29_pctrl_dphytx_stopcnt0_START (0)
#define SOC_PCTRL_PERI_CTRL29_pctrl_dphytx_stopcnt0_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int pctrl_dphytx_ctrl0 : 16;
        unsigned int pctrl_dphytx_ctrl1 : 16;
    } reg;
} SOC_PCTRL_PERI_CTRL30_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL30_pctrl_dphytx_ctrl0_START (0)
#define SOC_PCTRL_PERI_CTRL30_pctrl_dphytx_ctrl0_END (15)
#define SOC_PCTRL_PERI_CTRL30_pctrl_dphytx_ctrl1_START (16)
#define SOC_PCTRL_PERI_CTRL30_pctrl_dphytx_ctrl1_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int peri_spram_rtsel : 2;
        unsigned int peri_spram_wtsel : 2;
        unsigned int peri_spmbsram_rtsel : 2;
        unsigned int peri_spmbsram_wtsel : 2;
        unsigned int peri_rom_rtsel : 2;
        unsigned int peri_rom_ptsel : 2;
        unsigned int peri_rom_trb : 2;
        unsigned int peri_rom_tm : 1;
        unsigned int reserved_0 : 1;
        unsigned int peri_tprf_rct : 2;
        unsigned int peri_tprf_wct : 2;
        unsigned int peri_tprf_kp : 3;
        unsigned int reserved_1 : 1;
        unsigned int peri_dpsram_rtsel : 2;
        unsigned int peri_dpsram_wtsel : 2;
        unsigned int reserved_2 : 4;
    } reg;
} SOC_PCTRL_PERI_CTRL31_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL31_peri_spram_rtsel_START (0)
#define SOC_PCTRL_PERI_CTRL31_peri_spram_rtsel_END (1)
#define SOC_PCTRL_PERI_CTRL31_peri_spram_wtsel_START (2)
#define SOC_PCTRL_PERI_CTRL31_peri_spram_wtsel_END (3)
#define SOC_PCTRL_PERI_CTRL31_peri_spmbsram_rtsel_START (4)
#define SOC_PCTRL_PERI_CTRL31_peri_spmbsram_rtsel_END (5)
#define SOC_PCTRL_PERI_CTRL31_peri_spmbsram_wtsel_START (6)
#define SOC_PCTRL_PERI_CTRL31_peri_spmbsram_wtsel_END (7)
#define SOC_PCTRL_PERI_CTRL31_peri_rom_rtsel_START (8)
#define SOC_PCTRL_PERI_CTRL31_peri_rom_rtsel_END (9)
#define SOC_PCTRL_PERI_CTRL31_peri_rom_ptsel_START (10)
#define SOC_PCTRL_PERI_CTRL31_peri_rom_ptsel_END (11)
#define SOC_PCTRL_PERI_CTRL31_peri_rom_trb_START (12)
#define SOC_PCTRL_PERI_CTRL31_peri_rom_trb_END (13)
#define SOC_PCTRL_PERI_CTRL31_peri_rom_tm_START (14)
#define SOC_PCTRL_PERI_CTRL31_peri_rom_tm_END (14)
#define SOC_PCTRL_PERI_CTRL31_peri_tprf_rct_START (16)
#define SOC_PCTRL_PERI_CTRL31_peri_tprf_rct_END (17)
#define SOC_PCTRL_PERI_CTRL31_peri_tprf_wct_START (18)
#define SOC_PCTRL_PERI_CTRL31_peri_tprf_wct_END (19)
#define SOC_PCTRL_PERI_CTRL31_peri_tprf_kp_START (20)
#define SOC_PCTRL_PERI_CTRL31_peri_tprf_kp_END (22)
#define SOC_PCTRL_PERI_CTRL31_peri_dpsram_rtsel_START (24)
#define SOC_PCTRL_PERI_CTRL31_peri_dpsram_rtsel_END (25)
#define SOC_PCTRL_PERI_CTRL31_peri_dpsram_wtsel_START (26)
#define SOC_PCTRL_PERI_CTRL31_peri_dpsram_wtsel_END (27)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int pctrl_dphytx_stopcnt1 : 32;
    } reg;
} SOC_PCTRL_PERI_CTRL32_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL32_pctrl_dphytx_stopcnt1_START (0)
#define SOC_PCTRL_PERI_CTRL32_pctrl_dphytx_stopcnt1_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved : 32;
    } reg;
} SOC_PCTRL_PERI_CTRL33_UNION;
#endif
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int int_dmabus_error_probe_observer_mainfault0 : 1;
        unsigned int int_sysbus_error_probe_observer_mainfault0 : 1;
        unsigned int int_aobus_error_probe_observer_mainfault0 : 1;
        unsigned int int_modemcfg_error_probe_observer_mainfault0 : 1;
        unsigned int int_ufsbus_error_probe_observer_mainfault0 : 1;
        unsigned int int_vcodec_error_probe_observer_mainfault0 : 1;
        unsigned int int_vivobus_error_probe_observer_mainfault0 : 1;
        unsigned int int_cfgbus_error_probe_observer_mainfault0 : 1;
        unsigned int int_mmc0bus_error_probe_observer_mainfault0 : 1;
        unsigned int int_mmc1bus_error_probe_observer_mainfault0 : 1;
        unsigned int intr_ivp32_peri_bus_error_probe_observer_mainfault0 : 1;
        unsigned int reserved_0 : 2;
        unsigned int int_modem_transaction_probe_mainstatalarm : 1;
        unsigned int reserved_1 : 1;
        unsigned int intr_dss0_rd_transaction_probe_mainstatalarm : 1;
        unsigned int intr_dss0_wr_transaction_probe_mainstatalarm : 1;
        unsigned int intr_dss1_rd_transaction_probe_mainstatalarm : 1;
        unsigned int intr_dss1_wr_transaction_probe_mainstatalarm : 1;
        unsigned int intr_isp_wr_transaction_probe_mainstatalarm : 1;
        unsigned int intr_isp_rd_transaction_probe_mainstatalarm : 1;
        unsigned int reserved_2 : 2;
        unsigned int intr_a7_wr_transaction_probe_mainstatalarm : 1;
        unsigned int intr_a7_rd_transaction_probe_mainstatalarm : 1;
        unsigned int reserved_3 : 2;
        unsigned int int_sysbusddrc_packet_probe_mainstatalarm : 1;
        unsigned int reserved_4 : 2;
        unsigned int dphytx_pctrl_trstop_flag1 : 1;
        unsigned int dphytx_pctrl_trstop_flag0 : 1;
    } reg;
} SOC_PCTRL_PERI_STAT0_UNION;
#endif
#define SOC_PCTRL_PERI_STAT0_int_dmabus_error_probe_observer_mainfault0_START (0)
#define SOC_PCTRL_PERI_STAT0_int_dmabus_error_probe_observer_mainfault0_END (0)
#define SOC_PCTRL_PERI_STAT0_int_sysbus_error_probe_observer_mainfault0_START (1)
#define SOC_PCTRL_PERI_STAT0_int_sysbus_error_probe_observer_mainfault0_END (1)
#define SOC_PCTRL_PERI_STAT0_int_aobus_error_probe_observer_mainfault0_START (2)
#define SOC_PCTRL_PERI_STAT0_int_aobus_error_probe_observer_mainfault0_END (2)
#define SOC_PCTRL_PERI_STAT0_int_modemcfg_error_probe_observer_mainfault0_START (3)
#define SOC_PCTRL_PERI_STAT0_int_modemcfg_error_probe_observer_mainfault0_END (3)
#define SOC_PCTRL_PERI_STAT0_int_ufsbus_error_probe_observer_mainfault0_START (4)
#define SOC_PCTRL_PERI_STAT0_int_ufsbus_error_probe_observer_mainfault0_END (4)
#define SOC_PCTRL_PERI_STAT0_int_vcodec_error_probe_observer_mainfault0_START (5)
#define SOC_PCTRL_PERI_STAT0_int_vcodec_error_probe_observer_mainfault0_END (5)
#define SOC_PCTRL_PERI_STAT0_int_vivobus_error_probe_observer_mainfault0_START (6)
#define SOC_PCTRL_PERI_STAT0_int_vivobus_error_probe_observer_mainfault0_END (6)
#define SOC_PCTRL_PERI_STAT0_int_cfgbus_error_probe_observer_mainfault0_START (7)
#define SOC_PCTRL_PERI_STAT0_int_cfgbus_error_probe_observer_mainfault0_END (7)
#define SOC_PCTRL_PERI_STAT0_int_mmc0bus_error_probe_observer_mainfault0_START (8)
#define SOC_PCTRL_PERI_STAT0_int_mmc0bus_error_probe_observer_mainfault0_END (8)
#define SOC_PCTRL_PERI_STAT0_int_mmc1bus_error_probe_observer_mainfault0_START (9)
#define SOC_PCTRL_PERI_STAT0_int_mmc1bus_error_probe_observer_mainfault0_END (9)
#define SOC_PCTRL_PERI_STAT0_intr_ivp32_peri_bus_error_probe_observer_mainfault0_START (10)
#define SOC_PCTRL_PERI_STAT0_intr_ivp32_peri_bus_error_probe_observer_mainfault0_END (10)
#define SOC_PCTRL_PERI_STAT0_int_modem_transaction_probe_mainstatalarm_START (13)
#define SOC_PCTRL_PERI_STAT0_int_modem_transaction_probe_mainstatalarm_END (13)
#define SOC_PCTRL_PERI_STAT0_intr_dss0_rd_transaction_probe_mainstatalarm_START (15)
#define SOC_PCTRL_PERI_STAT0_intr_dss0_rd_transaction_probe_mainstatalarm_END (15)
#define SOC_PCTRL_PERI_STAT0_intr_dss0_wr_transaction_probe_mainstatalarm_START (16)
#define SOC_PCTRL_PERI_STAT0_intr_dss0_wr_transaction_probe_mainstatalarm_END (16)
#define SOC_PCTRL_PERI_STAT0_intr_dss1_rd_transaction_probe_mainstatalarm_START (17)
#define SOC_PCTRL_PERI_STAT0_intr_dss1_rd_transaction_probe_mainstatalarm_END (17)
#define SOC_PCTRL_PERI_STAT0_intr_dss1_wr_transaction_probe_mainstatalarm_START (18)
#define SOC_PCTRL_PERI_STAT0_intr_dss1_wr_transaction_probe_mainstatalarm_END (18)
#define SOC_PCTRL_PERI_STAT0_intr_isp_wr_transaction_probe_mainstatalarm_START (19)
#define SOC_PCTRL_PERI_STAT0_intr_isp_wr_transaction_probe_mainstatalarm_END (19)
#define SOC_PCTRL_PERI_STAT0_intr_isp_rd_transaction_probe_mainstatalarm_START (20)
#define SOC_PCTRL_PERI_STAT0_intr_isp_rd_transaction_probe_mainstatalarm_END (20)
#define SOC_PCTRL_PERI_STAT0_intr_a7_wr_transaction_probe_mainstatalarm_START (23)
#define SOC_PCTRL_PERI_STAT0_intr_a7_wr_transaction_probe_mainstatalarm_END (23)
#define SOC_PCTRL_PERI_STAT0_intr_a7_rd_transaction_probe_mainstatalarm_START (24)
#define SOC_PCTRL_PERI_STAT0_intr_a7_rd_transaction_probe_mainstatalarm_END (24)
#define SOC_PCTRL_PERI_STAT0_int_sysbusddrc_packet_probe_mainstatalarm_START (27)
#define SOC_PCTRL_PERI_STAT0_int_sysbusddrc_packet_probe_mainstatalarm_END (27)
#define SOC_PCTRL_PERI_STAT0_dphytx_pctrl_trstop_flag1_START (30)
#define SOC_PCTRL_PERI_STAT0_dphytx_pctrl_trstop_flag1_END (30)
#define SOC_PCTRL_PERI_STAT0_dphytx_pctrl_trstop_flag0_START (31)
#define SOC_PCTRL_PERI_STAT0_dphytx_pctrl_trstop_flag0_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int dphytx_pctrl_status0 : 32;
    } reg;
} SOC_PCTRL_PERI_STAT1_UNION;
#endif
#define SOC_PCTRL_PERI_STAT1_dphytx_pctrl_status0_START (0)
#define SOC_PCTRL_PERI_STAT1_dphytx_pctrl_status0_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int cfgbus_servicetarget_mainnopendingtrans : 1;
        unsigned int nocdjtag_mst_i_mainnopendingtrans : 1;
        unsigned int nocperfstat_i_mainnopendingtrans : 1;
        unsigned int noclpm3mst_i_mainnopendingtrans : 1;
        unsigned int nocsd3stat_i_mainnopendingtrans : 1;
        unsigned int nocusb3_i_mainnopendingtrans : 1;
        unsigned int nocsecp_i_mainnopendingtrans : 1;
        unsigned int nocsecs_i_mainnopendingtrans : 1;
        unsigned int nocsocp_i_mainnopendingtrans : 1;
        unsigned int noctopcssys_i_mainnopendingtrans : 1;
        unsigned int dss_servicetarget_mainnopendingtrans : 1;
        unsigned int nocdss0_rd_i_mainnopendingtrans : 1;
        unsigned int nocdss0_wr_i_mainnopendingtrans : 1;
        unsigned int nocdss1_rd_i_mainnopendingtrans : 1;
        unsigned int nocipf_i_mainnopendingtrans : 1;
        unsigned int nocdss1_wr_i_mainnopendingtrans : 1;
        unsigned int nocvivocfg_i_mainnopendingtrans : 1;
        unsigned int dmabus_servicetarget_mainnopendingtrans : 1;
        unsigned int nocdmacmst_i_mainnopendingtrans : 1;
        unsigned int isp_servicetarget_mainnopendingtrans : 1;
        unsigned int nocisp0_rd_i_mainnopendingtrans : 1;
        unsigned int nocisp0_wr_i_mainnopendingtrans : 1;
        unsigned int noca7tovivobus_rd_i_mainnopendingtrans : 1;
        unsigned int noca7tovivobus_wr_i_mainnopendingtrans : 1;
        unsigned int nocisp1_rd_i_mainnopendingtrans : 1;
        unsigned int nocisp1_wr_i_mainnopendingtrans : 1;
        unsigned int reserved : 1;
        unsigned int nocvcodeccfg_i_mainnopendingtrans : 1;
        unsigned int nocsdio0_i_mainnopendingtrans : 1;
        unsigned int nocpcie_i_mainnopendingtrans : 1;
        unsigned int vdec_servicetarget_mainnopendingtrans : 1;
        unsigned int nocvenc_i_mainnopendingtrans : 1;
    } reg;
} SOC_PCTRL_PERI_STAT2_UNION;
#endif
#define SOC_PCTRL_PERI_STAT2_cfgbus_servicetarget_mainnopendingtrans_START (0)
#define SOC_PCTRL_PERI_STAT2_cfgbus_servicetarget_mainnopendingtrans_END (0)
#define SOC_PCTRL_PERI_STAT2_nocdjtag_mst_i_mainnopendingtrans_START (1)
#define SOC_PCTRL_PERI_STAT2_nocdjtag_mst_i_mainnopendingtrans_END (1)
#define SOC_PCTRL_PERI_STAT2_nocperfstat_i_mainnopendingtrans_START (2)
#define SOC_PCTRL_PERI_STAT2_nocperfstat_i_mainnopendingtrans_END (2)
#define SOC_PCTRL_PERI_STAT2_noclpm3mst_i_mainnopendingtrans_START (3)
#define SOC_PCTRL_PERI_STAT2_noclpm3mst_i_mainnopendingtrans_END (3)
#define SOC_PCTRL_PERI_STAT2_nocsd3stat_i_mainnopendingtrans_START (4)
#define SOC_PCTRL_PERI_STAT2_nocsd3stat_i_mainnopendingtrans_END (4)
#define SOC_PCTRL_PERI_STAT2_nocusb3_i_mainnopendingtrans_START (5)
#define SOC_PCTRL_PERI_STAT2_nocusb3_i_mainnopendingtrans_END (5)
#define SOC_PCTRL_PERI_STAT2_nocsecp_i_mainnopendingtrans_START (6)
#define SOC_PCTRL_PERI_STAT2_nocsecp_i_mainnopendingtrans_END (6)
#define SOC_PCTRL_PERI_STAT2_nocsecs_i_mainnopendingtrans_START (7)
#define SOC_PCTRL_PERI_STAT2_nocsecs_i_mainnopendingtrans_END (7)
#define SOC_PCTRL_PERI_STAT2_nocsocp_i_mainnopendingtrans_START (8)
#define SOC_PCTRL_PERI_STAT2_nocsocp_i_mainnopendingtrans_END (8)
#define SOC_PCTRL_PERI_STAT2_noctopcssys_i_mainnopendingtrans_START (9)
#define SOC_PCTRL_PERI_STAT2_noctopcssys_i_mainnopendingtrans_END (9)
#define SOC_PCTRL_PERI_STAT2_dss_servicetarget_mainnopendingtrans_START (10)
#define SOC_PCTRL_PERI_STAT2_dss_servicetarget_mainnopendingtrans_END (10)
#define SOC_PCTRL_PERI_STAT2_nocdss0_rd_i_mainnopendingtrans_START (11)
#define SOC_PCTRL_PERI_STAT2_nocdss0_rd_i_mainnopendingtrans_END (11)
#define SOC_PCTRL_PERI_STAT2_nocdss0_wr_i_mainnopendingtrans_START (12)
#define SOC_PCTRL_PERI_STAT2_nocdss0_wr_i_mainnopendingtrans_END (12)
#define SOC_PCTRL_PERI_STAT2_nocdss1_rd_i_mainnopendingtrans_START (13)
#define SOC_PCTRL_PERI_STAT2_nocdss1_rd_i_mainnopendingtrans_END (13)
#define SOC_PCTRL_PERI_STAT2_nocipf_i_mainnopendingtrans_START (14)
#define SOC_PCTRL_PERI_STAT2_nocipf_i_mainnopendingtrans_END (14)
#define SOC_PCTRL_PERI_STAT2_nocdss1_wr_i_mainnopendingtrans_START (15)
#define SOC_PCTRL_PERI_STAT2_nocdss1_wr_i_mainnopendingtrans_END (15)
#define SOC_PCTRL_PERI_STAT2_nocvivocfg_i_mainnopendingtrans_START (16)
#define SOC_PCTRL_PERI_STAT2_nocvivocfg_i_mainnopendingtrans_END (16)
#define SOC_PCTRL_PERI_STAT2_dmabus_servicetarget_mainnopendingtrans_START (17)
#define SOC_PCTRL_PERI_STAT2_dmabus_servicetarget_mainnopendingtrans_END (17)
#define SOC_PCTRL_PERI_STAT2_nocdmacmst_i_mainnopendingtrans_START (18)
#define SOC_PCTRL_PERI_STAT2_nocdmacmst_i_mainnopendingtrans_END (18)
#define SOC_PCTRL_PERI_STAT2_isp_servicetarget_mainnopendingtrans_START (19)
#define SOC_PCTRL_PERI_STAT2_isp_servicetarget_mainnopendingtrans_END (19)
#define SOC_PCTRL_PERI_STAT2_nocisp0_rd_i_mainnopendingtrans_START (20)
#define SOC_PCTRL_PERI_STAT2_nocisp0_rd_i_mainnopendingtrans_END (20)
#define SOC_PCTRL_PERI_STAT2_nocisp0_wr_i_mainnopendingtrans_START (21)
#define SOC_PCTRL_PERI_STAT2_nocisp0_wr_i_mainnopendingtrans_END (21)
#define SOC_PCTRL_PERI_STAT2_noca7tovivobus_rd_i_mainnopendingtrans_START (22)
#define SOC_PCTRL_PERI_STAT2_noca7tovivobus_rd_i_mainnopendingtrans_END (22)
#define SOC_PCTRL_PERI_STAT2_noca7tovivobus_wr_i_mainnopendingtrans_START (23)
#define SOC_PCTRL_PERI_STAT2_noca7tovivobus_wr_i_mainnopendingtrans_END (23)
#define SOC_PCTRL_PERI_STAT2_nocisp1_rd_i_mainnopendingtrans_START (24)
#define SOC_PCTRL_PERI_STAT2_nocisp1_rd_i_mainnopendingtrans_END (24)
#define SOC_PCTRL_PERI_STAT2_nocisp1_wr_i_mainnopendingtrans_START (25)
#define SOC_PCTRL_PERI_STAT2_nocisp1_wr_i_mainnopendingtrans_END (25)
#define SOC_PCTRL_PERI_STAT2_nocvcodeccfg_i_mainnopendingtrans_START (27)
#define SOC_PCTRL_PERI_STAT2_nocvcodeccfg_i_mainnopendingtrans_END (27)
#define SOC_PCTRL_PERI_STAT2_nocsdio0_i_mainnopendingtrans_START (28)
#define SOC_PCTRL_PERI_STAT2_nocsdio0_i_mainnopendingtrans_END (28)
#define SOC_PCTRL_PERI_STAT2_nocpcie_i_mainnopendingtrans_START (29)
#define SOC_PCTRL_PERI_STAT2_nocpcie_i_mainnopendingtrans_END (29)
#define SOC_PCTRL_PERI_STAT2_vdec_servicetarget_mainnopendingtrans_START (30)
#define SOC_PCTRL_PERI_STAT2_vdec_servicetarget_mainnopendingtrans_END (30)
#define SOC_PCTRL_PERI_STAT2_nocvenc_i_mainnopendingtrans_START (31)
#define SOC_PCTRL_PERI_STAT2_nocvenc_i_mainnopendingtrans_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int peri_stat3_0 : 1;
        unsigned int peri_stat3_1 : 1;
        unsigned int peri_stat3_2 : 1;
        unsigned int peri_stat3_3 : 1;
        unsigned int peri_stat3_4 : 1;
        unsigned int peri_stat3_5 : 1;
        unsigned int peri_stat3_6 : 1;
        unsigned int peri_stat3_7 : 1;
        unsigned int peri_stat3_8 : 1;
        unsigned int peri_stat3_9 : 1;
        unsigned int peri_stat3_10 : 1;
        unsigned int peri_stat3_11 : 1;
        unsigned int peri_stat3_12 : 1;
        unsigned int peri_stat3_13 : 1;
        unsigned int peri_stat3_14 : 1;
        unsigned int peri_stat3_15 : 1;
        unsigned int peri_stat3_16 : 1;
        unsigned int peri_stat3_17 : 1;
        unsigned int peri_stat3_18 : 1;
        unsigned int peri_stat3_19 : 1;
        unsigned int peri_stat3_20 : 1;
        unsigned int peri_stat3 : 11;
    } reg;
} SOC_PCTRL_PERI_STAT3_UNION;
#endif
#define SOC_PCTRL_PERI_STAT3_peri_stat3_0_START (0)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_0_END (0)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_1_START (1)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_1_END (1)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_2_START (2)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_2_END (2)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_3_START (3)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_3_END (3)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_4_START (4)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_4_END (4)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_5_START (5)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_5_END (5)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_6_START (6)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_6_END (6)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_7_START (7)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_7_END (7)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_8_START (8)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_8_END (8)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_9_START (9)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_9_END (9)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_10_START (10)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_10_END (10)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_11_START (11)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_11_END (11)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_12_START (12)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_12_END (12)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_13_START (13)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_13_END (13)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_14_START (14)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_14_END (14)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_15_START (15)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_15_END (15)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_16_START (16)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_16_END (16)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_17_START (17)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_17_END (17)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_18_START (18)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_18_END (18)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_19_START (19)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_19_END (19)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_20_START (20)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_20_END (20)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_START (21)
#define SOC_PCTRL_PERI_STAT3_peri_stat3_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int psam_idle : 1;
        unsigned int ipf_idle : 1;
        unsigned int vdec_idle : 1;
        unsigned int reserved_0 : 1;
        unsigned int venc_idle : 1;
        unsigned int reserved_1 : 1;
        unsigned int socp_idle : 1;
        unsigned int ivp32_dwaxi_dlock_mst : 3;
        unsigned int ivp32_dwaxi_dlock_slv : 3;
        unsigned int ivp32_dwaxi_dlock_id : 6;
        unsigned int ivp32_dwaxi_dlock_wr : 1;
        unsigned int ivp32_dwaxi_dlock_irq : 1;
        unsigned int reserved_2 : 1;
        unsigned int intr_cci_err : 1;
        unsigned int mdm_ipc_s_clk_state : 3;
        unsigned int mdm_ipc_ns_clk_state : 3;
        unsigned int reserved_3 : 3;
    } reg;
} SOC_PCTRL_PERI_STAT4_UNION;
#endif
#define SOC_PCTRL_PERI_STAT4_psam_idle_START (0)
#define SOC_PCTRL_PERI_STAT4_psam_idle_END (0)
#define SOC_PCTRL_PERI_STAT4_ipf_idle_START (1)
#define SOC_PCTRL_PERI_STAT4_ipf_idle_END (1)
#define SOC_PCTRL_PERI_STAT4_vdec_idle_START (2)
#define SOC_PCTRL_PERI_STAT4_vdec_idle_END (2)
#define SOC_PCTRL_PERI_STAT4_venc_idle_START (4)
#define SOC_PCTRL_PERI_STAT4_venc_idle_END (4)
#define SOC_PCTRL_PERI_STAT4_socp_idle_START (6)
#define SOC_PCTRL_PERI_STAT4_socp_idle_END (6)
#define SOC_PCTRL_PERI_STAT4_ivp32_dwaxi_dlock_mst_START (7)
#define SOC_PCTRL_PERI_STAT4_ivp32_dwaxi_dlock_mst_END (9)
#define SOC_PCTRL_PERI_STAT4_ivp32_dwaxi_dlock_slv_START (10)
#define SOC_PCTRL_PERI_STAT4_ivp32_dwaxi_dlock_slv_END (12)
#define SOC_PCTRL_PERI_STAT4_ivp32_dwaxi_dlock_id_START (13)
#define SOC_PCTRL_PERI_STAT4_ivp32_dwaxi_dlock_id_END (18)
#define SOC_PCTRL_PERI_STAT4_ivp32_dwaxi_dlock_wr_START (19)
#define SOC_PCTRL_PERI_STAT4_ivp32_dwaxi_dlock_wr_END (19)
#define SOC_PCTRL_PERI_STAT4_ivp32_dwaxi_dlock_irq_START (20)
#define SOC_PCTRL_PERI_STAT4_ivp32_dwaxi_dlock_irq_END (20)
#define SOC_PCTRL_PERI_STAT4_intr_cci_err_START (22)
#define SOC_PCTRL_PERI_STAT4_intr_cci_err_END (22)
#define SOC_PCTRL_PERI_STAT4_mdm_ipc_s_clk_state_START (23)
#define SOC_PCTRL_PERI_STAT4_mdm_ipc_s_clk_state_END (25)
#define SOC_PCTRL_PERI_STAT4_mdm_ipc_ns_clk_state_START (26)
#define SOC_PCTRL_PERI_STAT4_mdm_ipc_ns_clk_state_END (28)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sc_mdm2ap_states : 32;
    } reg;
} SOC_PCTRL_PERI_STAT5_UNION;
#endif
#define SOC_PCTRL_PERI_STAT5_sc_mdm2ap_states_START (0)
#define SOC_PCTRL_PERI_STAT5_sc_mdm2ap_states_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sc_mdm2ap_states : 32;
    } reg;
} SOC_PCTRL_PERI_STAT6_UNION;
#endif
#define SOC_PCTRL_PERI_STAT6_sc_mdm2ap_states_START (0)
#define SOC_PCTRL_PERI_STAT6_sc_mdm2ap_states_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sc_mdm2ap_states : 32;
    } reg;
} SOC_PCTRL_PERI_STAT7_UNION;
#endif
#define SOC_PCTRL_PERI_STAT7_sc_mdm2ap_states_START (0)
#define SOC_PCTRL_PERI_STAT7_sc_mdm2ap_states_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int sc_mdm2ap_states : 32;
    } reg;
} SOC_PCTRL_PERI_STAT8_UNION;
#endif
#define SOC_PCTRL_PERI_STAT8_sc_mdm2ap_states_START (0)
#define SOC_PCTRL_PERI_STAT8_sc_mdm2ap_states_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int tp_s0_grp : 16;
        unsigned int tp_s1_grp : 16;
    } reg;
} SOC_PCTRL_PERI_STAT9_UNION;
#endif
#define SOC_PCTRL_PERI_STAT9_tp_s0_grp_START (0)
#define SOC_PCTRL_PERI_STAT9_tp_s0_grp_END (15)
#define SOC_PCTRL_PERI_STAT9_tp_s1_grp_START (16)
#define SOC_PCTRL_PERI_STAT9_tp_s1_grp_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int tp_s2_grp : 16;
        unsigned int tp_s7_grp : 16;
    } reg;
} SOC_PCTRL_PERI_STAT10_UNION;
#endif
#define SOC_PCTRL_PERI_STAT10_tp_s2_grp_START (0)
#define SOC_PCTRL_PERI_STAT10_tp_s2_grp_END (15)
#define SOC_PCTRL_PERI_STAT10_tp_s7_grp_START (16)
#define SOC_PCTRL_PERI_STAT10_tp_s7_grp_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int tp_s8_grp : 16;
        unsigned int tp_s9_grp : 16;
    } reg;
} SOC_PCTRL_PERI_STAT11_UNION;
#endif
#define SOC_PCTRL_PERI_STAT11_tp_s8_grp_START (0)
#define SOC_PCTRL_PERI_STAT11_tp_s8_grp_END (15)
#define SOC_PCTRL_PERI_STAT11_tp_s9_grp_START (16)
#define SOC_PCTRL_PERI_STAT11_tp_s9_grp_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int tp_s10_grp : 16;
        unsigned int tp_s25_grp : 16;
    } reg;
} SOC_PCTRL_PERI_STAT12_UNION;
#endif
#define SOC_PCTRL_PERI_STAT12_tp_s10_grp_START (0)
#define SOC_PCTRL_PERI_STAT12_tp_s10_grp_END (15)
#define SOC_PCTRL_PERI_STAT12_tp_s25_grp_START (16)
#define SOC_PCTRL_PERI_STAT12_tp_s25_grp_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int tp_s12_grp : 16;
        unsigned int tp_s13_grp : 16;
    } reg;
} SOC_PCTRL_PERI_STAT13_UNION;
#endif
#define SOC_PCTRL_PERI_STAT13_tp_s12_grp_START (0)
#define SOC_PCTRL_PERI_STAT13_tp_s12_grp_END (15)
#define SOC_PCTRL_PERI_STAT13_tp_s13_grp_START (16)
#define SOC_PCTRL_PERI_STAT13_tp_s13_grp_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int tp_s14_grp : 16;
        unsigned int tp_s15_grp : 16;
    } reg;
} SOC_PCTRL_PERI_STAT14_UNION;
#endif
#define SOC_PCTRL_PERI_STAT14_tp_s14_grp_START (0)
#define SOC_PCTRL_PERI_STAT14_tp_s14_grp_END (15)
#define SOC_PCTRL_PERI_STAT14_tp_s15_grp_START (16)
#define SOC_PCTRL_PERI_STAT14_tp_s15_grp_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int axi_isp_dlock : 11;
        unsigned int vdm_clk_en_0 : 1;
        unsigned int vdm_clk_en_1 : 1;
        unsigned int vdm_clk_en_2 : 1;
        unsigned int reserved : 18;
    } reg;
} SOC_PCTRL_PERI_STAT15_UNION;
#endif
#define SOC_PCTRL_PERI_STAT15_axi_isp_dlock_START (0)
#define SOC_PCTRL_PERI_STAT15_axi_isp_dlock_END (10)
#define SOC_PCTRL_PERI_STAT15_vdm_clk_en_0_START (11)
#define SOC_PCTRL_PERI_STAT15_vdm_clk_en_0_END (11)
#define SOC_PCTRL_PERI_STAT15_vdm_clk_en_1_START (12)
#define SOC_PCTRL_PERI_STAT15_vdm_clk_en_1_END (12)
#define SOC_PCTRL_PERI_STAT15_vdm_clk_en_2_START (13)
#define SOC_PCTRL_PERI_STAT15_vdm_clk_en_2_END (13)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int dphytx_pctrl_status1 : 32;
    } reg;
} SOC_PCTRL_PERI_STAT16_UNION;
#endif
#define SOC_PCTRL_PERI_STAT16_dphytx_pctrl_status1_START (0)
#define SOC_PCTRL_PERI_STAT16_dphytx_pctrl_status1_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int tp_s3_grp : 16;
        unsigned int tp_s4_grp : 16;
    } reg;
} SOC_PCTRL_PERI_STAT17_UNION;
#endif
#define SOC_PCTRL_PERI_STAT17_tp_s3_grp_START (0)
#define SOC_PCTRL_PERI_STAT17_tp_s3_grp_END (15)
#define SOC_PCTRL_PERI_STAT17_tp_s4_grp_START (16)
#define SOC_PCTRL_PERI_STAT17_tp_s4_grp_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int tp_s5_grp : 16;
        unsigned int tp_s6_grp : 16;
    } reg;
} SOC_PCTRL_PERI_STAT18_UNION;
#endif
#define SOC_PCTRL_PERI_STAT18_tp_s5_grp_START (0)
#define SOC_PCTRL_PERI_STAT18_tp_s5_grp_END (15)
#define SOC_PCTRL_PERI_STAT18_tp_s6_grp_START (16)
#define SOC_PCTRL_PERI_STAT18_tp_s6_grp_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int tp_s11_grp : 16;
        unsigned int reserved : 16;
    } reg;
} SOC_PCTRL_PERI_STAT19_UNION;
#endif
#define SOC_PCTRL_PERI_STAT19_tp_s11_grp_START (0)
#define SOC_PCTRL_PERI_STAT19_tp_s11_grp_END (15)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int usb2_host_ctrl0 : 32;
    } reg;
} SOC_PCTRL_USB2_HOST_CTRL0_UNION;
#endif
#define SOC_PCTRL_USB2_HOST_CTRL0_usb2_host_ctrl0_START (0)
#define SOC_PCTRL_USB2_HOST_CTRL0_usb2_host_ctrl0_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int usb2_host_ctrl1 : 32;
    } reg;
} SOC_PCTRL_USB2_HOST_CTRL1_UNION;
#endif
#define SOC_PCTRL_USB2_HOST_CTRL1_usb2_host_ctrl1_START (0)
#define SOC_PCTRL_USB2_HOST_CTRL1_usb2_host_ctrl1_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int usb2_host_ctrl2 : 32;
    } reg;
} SOC_PCTRL_USB2_HOST_CTRL2_UNION;
#endif
#define SOC_PCTRL_USB2_HOST_CTRL2_usb2_host_ctrl2_START (0)
#define SOC_PCTRL_USB2_HOST_CTRL2_usb2_host_ctrl2_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource0lock_cmd0 : 1;
        unsigned int resource0lock_id0 : 3;
        unsigned int resource0lock_cmd1 : 1;
        unsigned int resource0lock_id1 : 3;
        unsigned int resource0lock_cmd2 : 1;
        unsigned int resource0lock_id2 : 3;
        unsigned int resource0lock_cmd3 : 1;
        unsigned int resource0lock_id3 : 3;
        unsigned int resource0lock_cmd4 : 1;
        unsigned int resource0lock_id4 : 3;
        unsigned int resource0lock_cmd5 : 1;
        unsigned int resource0lock_id5 : 3;
        unsigned int resource0lock_cmd6 : 1;
        unsigned int resource0lock_id6 : 3;
        unsigned int resource0lock_cmd7 : 1;
        unsigned int resource0lock_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE0_LOCK_UNION;
#endif
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_cmd0_START (0)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_cmd0_END (0)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_id0_START (1)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_id0_END (3)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_cmd1_START (4)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_cmd1_END (4)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_id1_START (5)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_id1_END (7)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_cmd2_START (8)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_cmd2_END (8)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_id2_START (9)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_id2_END (11)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_cmd3_START (12)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_cmd3_END (12)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_id3_START (13)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_id3_END (15)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_cmd4_START (16)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_cmd4_END (16)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_id4_START (17)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_id4_END (19)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_cmd5_START (20)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_cmd5_END (20)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_id5_START (21)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_id5_END (23)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_cmd6_START (24)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_cmd6_END (24)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_id6_START (25)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_id6_END (27)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_cmd7_START (28)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_cmd7_END (28)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_id7_START (29)
#define SOC_PCTRL_RESOURCE0_LOCK_resource0lock_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource0unlock_cmd0 : 1;
        unsigned int resource0unlock_id0 : 3;
        unsigned int resource0unlock_cmd1 : 1;
        unsigned int resource0unlock_id1 : 3;
        unsigned int resource0unlock_cmd2 : 1;
        unsigned int resource0unlock_id2 : 3;
        unsigned int resource0unlock_cmd3 : 1;
        unsigned int resource0unlock_id3 : 3;
        unsigned int resource0unlock_cmd4 : 1;
        unsigned int resource0unlock_id4 : 3;
        unsigned int resource0unlock_cmd5 : 1;
        unsigned int resource0unlock_id5 : 3;
        unsigned int resource0unlock_cmd6 : 1;
        unsigned int resource0unlock_id6 : 3;
        unsigned int resource0unlock_cmd7 : 1;
        unsigned int resource0unlock_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE0_UNLOCK_UNION;
#endif
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_cmd0_START (0)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_cmd0_END (0)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_id0_START (1)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_id0_END (3)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_cmd1_START (4)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_cmd1_END (4)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_id1_START (5)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_id1_END (7)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_cmd2_START (8)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_cmd2_END (8)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_id2_START (9)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_id2_END (11)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_cmd3_START (12)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_cmd3_END (12)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_id3_START (13)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_id3_END (15)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_cmd4_START (16)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_cmd4_END (16)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_id4_START (17)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_id4_END (19)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_cmd5_START (20)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_cmd5_END (20)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_id5_START (21)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_id5_END (23)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_cmd6_START (24)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_cmd6_END (24)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_id6_START (25)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_id6_END (27)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_cmd7_START (28)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_cmd7_END (28)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_id7_START (29)
#define SOC_PCTRL_RESOURCE0_UNLOCK_resource0unlock_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource0lock_st0 : 1;
        unsigned int resource0lock_st_id0 : 3;
        unsigned int resource0lock_st1 : 1;
        unsigned int resource0lock_st_id1 : 3;
        unsigned int resource0lock_st2 : 1;
        unsigned int resource0lock_st_id2 : 3;
        unsigned int resource0lock_st3 : 1;
        unsigned int resource0lock_st_id3 : 3;
        unsigned int resource0lock_st4 : 1;
        unsigned int resource0lock_st_id4 : 3;
        unsigned int resource0lock_st5 : 1;
        unsigned int resource0lock_st_id5 : 3;
        unsigned int resource0lock_st6 : 1;
        unsigned int resource0lock_st_id6 : 3;
        unsigned int resource0lock_st7 : 1;
        unsigned int resource0lock_st_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE0_LOCK_ST_UNION;
#endif
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st0_START (0)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st0_END (0)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st_id0_START (1)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st_id0_END (3)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st1_START (4)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st1_END (4)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st_id1_START (5)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st_id1_END (7)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st2_START (8)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st2_END (8)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st_id2_START (9)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st_id2_END (11)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st3_START (12)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st3_END (12)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st_id3_START (13)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st_id3_END (15)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st4_START (16)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st4_END (16)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st_id4_START (17)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st_id4_END (19)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st5_START (20)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st5_END (20)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st_id5_START (21)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st_id5_END (23)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st6_START (24)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st6_END (24)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st_id6_START (25)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st_id6_END (27)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st7_START (28)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st7_END (28)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st_id7_START (29)
#define SOC_PCTRL_RESOURCE0_LOCK_ST_resource0lock_st_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource1lock_cmd0 : 1;
        unsigned int resource1lock_id0 : 3;
        unsigned int resource1lock_cmd1 : 1;
        unsigned int resource1lock_id1 : 3;
        unsigned int resource1lock_cmd2 : 1;
        unsigned int resource1lock_id2 : 3;
        unsigned int resource1lock_cmd3 : 1;
        unsigned int resource1lock_id3 : 3;
        unsigned int resource1lock_cmd4 : 1;
        unsigned int resource1lock_id4 : 3;
        unsigned int resource1lock_cmd5 : 1;
        unsigned int resource1lock_id5 : 3;
        unsigned int resource1lock_cmd6 : 1;
        unsigned int resource1lock_id6 : 3;
        unsigned int resource1lock_cmd7 : 1;
        unsigned int resource1lock_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE1_LOCK_UNION;
#endif
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_cmd0_START (0)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_cmd0_END (0)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_id0_START (1)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_id0_END (3)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_cmd1_START (4)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_cmd1_END (4)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_id1_START (5)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_id1_END (7)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_cmd2_START (8)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_cmd2_END (8)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_id2_START (9)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_id2_END (11)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_cmd3_START (12)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_cmd3_END (12)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_id3_START (13)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_id3_END (15)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_cmd4_START (16)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_cmd4_END (16)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_id4_START (17)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_id4_END (19)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_cmd5_START (20)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_cmd5_END (20)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_id5_START (21)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_id5_END (23)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_cmd6_START (24)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_cmd6_END (24)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_id6_START (25)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_id6_END (27)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_cmd7_START (28)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_cmd7_END (28)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_id7_START (29)
#define SOC_PCTRL_RESOURCE1_LOCK_resource1lock_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource1unlock_cmd0 : 1;
        unsigned int resource1unlock_id0 : 3;
        unsigned int resource1unlock_cmd1 : 1;
        unsigned int resource1unlock_id1 : 3;
        unsigned int resource1unlock_cmd2 : 1;
        unsigned int resource1unlock_id2 : 3;
        unsigned int resource1unlock_cmd3 : 1;
        unsigned int resource1unlock_id3 : 3;
        unsigned int resource1unlock_cmd4 : 1;
        unsigned int resource1unlock_id4 : 3;
        unsigned int resource1unlock_cmd5 : 1;
        unsigned int resource1unlock_id5 : 3;
        unsigned int resource1unlock_cmd6 : 1;
        unsigned int resource1unlock_id6 : 3;
        unsigned int resource1unlock_cmd7 : 1;
        unsigned int resource1unlock_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE1_UNLOCK_UNION;
#endif
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_cmd0_START (0)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_cmd0_END (0)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_id0_START (1)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_id0_END (3)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_cmd1_START (4)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_cmd1_END (4)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_id1_START (5)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_id1_END (7)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_cmd2_START (8)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_cmd2_END (8)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_id2_START (9)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_id2_END (11)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_cmd3_START (12)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_cmd3_END (12)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_id3_START (13)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_id3_END (15)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_cmd4_START (16)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_cmd4_END (16)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_id4_START (17)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_id4_END (19)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_cmd5_START (20)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_cmd5_END (20)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_id5_START (21)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_id5_END (23)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_cmd6_START (24)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_cmd6_END (24)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_id6_START (25)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_id6_END (27)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_cmd7_START (28)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_cmd7_END (28)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_id7_START (29)
#define SOC_PCTRL_RESOURCE1_UNLOCK_resource1unlock_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource1lock_st0 : 1;
        unsigned int resource1lock_st_id0 : 3;
        unsigned int resource1lock_st1 : 1;
        unsigned int resource1lock_st_id1 : 3;
        unsigned int resource1lock_st2 : 1;
        unsigned int resource1lock_st_id2 : 3;
        unsigned int resource1lock_st3 : 1;
        unsigned int resource1lock_st_id3 : 3;
        unsigned int resource1lock_st4 : 1;
        unsigned int resource1lock_st_id4 : 3;
        unsigned int resource1lock_st5 : 1;
        unsigned int resource1lock_st_id5 : 3;
        unsigned int resource1lock_st6 : 1;
        unsigned int resource1lock_st_id6 : 3;
        unsigned int resource1lock_st7 : 1;
        unsigned int resource1lock_st_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE1_LOCK_ST_UNION;
#endif
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st0_START (0)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st0_END (0)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st_id0_START (1)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st_id0_END (3)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st1_START (4)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st1_END (4)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st_id1_START (5)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st_id1_END (7)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st2_START (8)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st2_END (8)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st_id2_START (9)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st_id2_END (11)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st3_START (12)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st3_END (12)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st_id3_START (13)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st_id3_END (15)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st4_START (16)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st4_END (16)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st_id4_START (17)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st_id4_END (19)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st5_START (20)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st5_END (20)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st_id5_START (21)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st_id5_END (23)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st6_START (24)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st6_END (24)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st_id6_START (25)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st_id6_END (27)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st7_START (28)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st7_END (28)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st_id7_START (29)
#define SOC_PCTRL_RESOURCE1_LOCK_ST_resource1lock_st_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource2lock_cmd0 : 1;
        unsigned int resource2lock_id0 : 3;
        unsigned int resource2lock_cmd1 : 1;
        unsigned int resource2lock_id1 : 3;
        unsigned int resource2lock_cmd2 : 1;
        unsigned int resource2lock_id2 : 3;
        unsigned int resource2lock_cmd3 : 1;
        unsigned int resource2lock_id3 : 3;
        unsigned int resource2lock_cmd4 : 1;
        unsigned int resource2lock_id4 : 3;
        unsigned int resource2lock_cmd5 : 1;
        unsigned int resource2lock_id5 : 3;
        unsigned int resource2lock_cmd6 : 1;
        unsigned int resource2lock_id6 : 3;
        unsigned int resource2lock_cmd7 : 1;
        unsigned int resource2lock_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE2_LOCK_UNION;
#endif
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_cmd0_START (0)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_cmd0_END (0)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_id0_START (1)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_id0_END (3)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_cmd1_START (4)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_cmd1_END (4)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_id1_START (5)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_id1_END (7)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_cmd2_START (8)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_cmd2_END (8)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_id2_START (9)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_id2_END (11)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_cmd3_START (12)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_cmd3_END (12)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_id3_START (13)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_id3_END (15)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_cmd4_START (16)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_cmd4_END (16)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_id4_START (17)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_id4_END (19)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_cmd5_START (20)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_cmd5_END (20)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_id5_START (21)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_id5_END (23)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_cmd6_START (24)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_cmd6_END (24)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_id6_START (25)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_id6_END (27)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_cmd7_START (28)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_cmd7_END (28)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_id7_START (29)
#define SOC_PCTRL_RESOURCE2_LOCK_resource2lock_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource2unlock_cmd0 : 1;
        unsigned int resource2unlock_id0 : 3;
        unsigned int resource2unlock_cmd1 : 1;
        unsigned int resource2unlock_id1 : 3;
        unsigned int resource2unlock_cmd2 : 1;
        unsigned int resource2unlock_id2 : 3;
        unsigned int resource2unlock_cmd3 : 1;
        unsigned int resource2unlock_id3 : 3;
        unsigned int resource2unlock_cmd4 : 1;
        unsigned int resource2unlock_id4 : 3;
        unsigned int resource2unlock_cmd5 : 1;
        unsigned int resource2unlock_id5 : 3;
        unsigned int resource2unlock_cmd6 : 1;
        unsigned int resource2unlock_id6 : 3;
        unsigned int resource2unlock_cmd7 : 1;
        unsigned int resource2unlock_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE2_UNLOCK_UNION;
#endif
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_cmd0_START (0)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_cmd0_END (0)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_id0_START (1)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_id0_END (3)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_cmd1_START (4)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_cmd1_END (4)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_id1_START (5)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_id1_END (7)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_cmd2_START (8)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_cmd2_END (8)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_id2_START (9)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_id2_END (11)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_cmd3_START (12)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_cmd3_END (12)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_id3_START (13)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_id3_END (15)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_cmd4_START (16)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_cmd4_END (16)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_id4_START (17)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_id4_END (19)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_cmd5_START (20)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_cmd5_END (20)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_id5_START (21)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_id5_END (23)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_cmd6_START (24)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_cmd6_END (24)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_id6_START (25)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_id6_END (27)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_cmd7_START (28)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_cmd7_END (28)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_id7_START (29)
#define SOC_PCTRL_RESOURCE2_UNLOCK_resource2unlock_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource2lock_st0 : 1;
        unsigned int resource2lock_st_id0 : 3;
        unsigned int resource2lock_st1 : 1;
        unsigned int resource2lock_st_id1 : 3;
        unsigned int resource2lock_st2 : 1;
        unsigned int resource2lock_st_id2 : 3;
        unsigned int resource2lock_st3 : 1;
        unsigned int resource2lock_st_id3 : 3;
        unsigned int resource2lock_st4 : 1;
        unsigned int resource2lock_st_id4 : 3;
        unsigned int resource2lock_st5 : 1;
        unsigned int resource2lock_st_id5 : 3;
        unsigned int resource2lock_st6 : 1;
        unsigned int resource2lock_st_id6 : 3;
        unsigned int resource2lock_st7 : 1;
        unsigned int resource2lock_st_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE2_LOCK_ST_UNION;
#endif
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st0_START (0)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st0_END (0)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st_id0_START (1)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st_id0_END (3)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st1_START (4)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st1_END (4)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st_id1_START (5)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st_id1_END (7)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st2_START (8)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st2_END (8)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st_id2_START (9)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st_id2_END (11)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st3_START (12)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st3_END (12)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st_id3_START (13)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st_id3_END (15)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st4_START (16)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st4_END (16)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st_id4_START (17)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st_id4_END (19)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st5_START (20)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st5_END (20)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st_id5_START (21)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st_id5_END (23)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st6_START (24)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st6_END (24)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st_id6_START (25)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st_id6_END (27)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st7_START (28)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st7_END (28)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st_id7_START (29)
#define SOC_PCTRL_RESOURCE2_LOCK_ST_resource2lock_st_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource3lock_cmd0 : 1;
        unsigned int resource3lock_id0 : 3;
        unsigned int resource3lock_cmd1 : 1;
        unsigned int resource3lock_id1 : 3;
        unsigned int resource3lock_cmd2 : 1;
        unsigned int resource3lock_id2 : 3;
        unsigned int resource3lock_cmd3 : 1;
        unsigned int resource3lock_id3 : 3;
        unsigned int resource3lock_cmd4 : 1;
        unsigned int resource3lock_id4 : 3;
        unsigned int resource3lock_cmd5 : 1;
        unsigned int resource3lock_id5 : 3;
        unsigned int resource3lock_cmd6 : 1;
        unsigned int resource3lock_id6 : 3;
        unsigned int resource3lock_cmd7 : 1;
        unsigned int resource3lock_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE3_LOCK_UNION;
#endif
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_cmd0_START (0)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_cmd0_END (0)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_id0_START (1)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_id0_END (3)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_cmd1_START (4)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_cmd1_END (4)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_id1_START (5)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_id1_END (7)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_cmd2_START (8)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_cmd2_END (8)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_id2_START (9)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_id2_END (11)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_cmd3_START (12)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_cmd3_END (12)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_id3_START (13)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_id3_END (15)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_cmd4_START (16)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_cmd4_END (16)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_id4_START (17)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_id4_END (19)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_cmd5_START (20)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_cmd5_END (20)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_id5_START (21)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_id5_END (23)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_cmd6_START (24)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_cmd6_END (24)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_id6_START (25)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_id6_END (27)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_cmd7_START (28)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_cmd7_END (28)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_id7_START (29)
#define SOC_PCTRL_RESOURCE3_LOCK_resource3lock_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource3unlock_cmd0 : 1;
        unsigned int resource3unlock_id0 : 3;
        unsigned int resource3unlock_cmd1 : 1;
        unsigned int resource3unlock_id1 : 3;
        unsigned int resource3unlock_cmd2 : 1;
        unsigned int resource3unlock_id2 : 3;
        unsigned int resource3unlock_cmd3 : 1;
        unsigned int resource3unlock_id3 : 3;
        unsigned int resource3unlock_cmd4 : 1;
        unsigned int resource3unlock_id4 : 3;
        unsigned int resource3unlock_cmd5 : 1;
        unsigned int resource3unlock_id5 : 3;
        unsigned int resource3unlock_cmd6 : 1;
        unsigned int resource3unlock_id6 : 3;
        unsigned int resource3unlock_cmd7 : 1;
        unsigned int resource3unlock_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE3_UNLOCK_UNION;
#endif
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_cmd0_START (0)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_cmd0_END (0)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_id0_START (1)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_id0_END (3)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_cmd1_START (4)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_cmd1_END (4)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_id1_START (5)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_id1_END (7)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_cmd2_START (8)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_cmd2_END (8)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_id2_START (9)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_id2_END (11)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_cmd3_START (12)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_cmd3_END (12)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_id3_START (13)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_id3_END (15)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_cmd4_START (16)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_cmd4_END (16)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_id4_START (17)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_id4_END (19)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_cmd5_START (20)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_cmd5_END (20)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_id5_START (21)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_id5_END (23)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_cmd6_START (24)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_cmd6_END (24)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_id6_START (25)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_id6_END (27)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_cmd7_START (28)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_cmd7_END (28)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_id7_START (29)
#define SOC_PCTRL_RESOURCE3_UNLOCK_resource3unlock_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource3lock_st0 : 1;
        unsigned int resource3lock_st_id0 : 3;
        unsigned int resource3lock_st1 : 1;
        unsigned int resource3lock_st_id1 : 3;
        unsigned int resource3lock_st2 : 1;
        unsigned int resource3lock_st_id2 : 3;
        unsigned int resource3lock_st3 : 1;
        unsigned int resource3lock_st_id3 : 3;
        unsigned int resource3lock_st4 : 1;
        unsigned int resource3lock_st_id4 : 3;
        unsigned int resource3lock_st5 : 1;
        unsigned int resource3lock_st_id5 : 3;
        unsigned int resource3lock_st6 : 1;
        unsigned int resource3lock_st_id6 : 3;
        unsigned int resource3lock_st7 : 1;
        unsigned int resource3lock_st_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE3_LOCK_ST_UNION;
#endif
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st0_START (0)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st0_END (0)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st_id0_START (1)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st_id0_END (3)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st1_START (4)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st1_END (4)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st_id1_START (5)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st_id1_END (7)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st2_START (8)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st2_END (8)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st_id2_START (9)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st_id2_END (11)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st3_START (12)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st3_END (12)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st_id3_START (13)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st_id3_END (15)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st4_START (16)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st4_END (16)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st_id4_START (17)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st_id4_END (19)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st5_START (20)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st5_END (20)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st_id5_START (21)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st_id5_END (23)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st6_START (24)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st6_END (24)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st_id6_START (25)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st_id6_END (27)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st7_START (28)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st7_END (28)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st_id7_START (29)
#define SOC_PCTRL_RESOURCE3_LOCK_ST_resource3lock_st_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource4lock_cmd0 : 1;
        unsigned int resource4lock_id0 : 3;
        unsigned int resource4lock_cmd1 : 1;
        unsigned int resource4lock_id1 : 3;
        unsigned int resource4lock_cmd2 : 1;
        unsigned int resource4lock_id2 : 3;
        unsigned int resource4lock_cmd3 : 1;
        unsigned int resource4lock_id3 : 3;
        unsigned int resource4lock_cmd4 : 1;
        unsigned int resource4lock_id4 : 3;
        unsigned int resource4lock_cmd5 : 1;
        unsigned int resource4lock_id5 : 3;
        unsigned int resource4lock_cmd6 : 1;
        unsigned int resource4lock_id6 : 3;
        unsigned int resource4lock_cmd7 : 1;
        unsigned int resource4lock_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE4_LOCK_UNION;
#endif
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_cmd0_START (0)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_cmd0_END (0)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_id0_START (1)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_id0_END (3)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_cmd1_START (4)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_cmd1_END (4)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_id1_START (5)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_id1_END (7)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_cmd2_START (8)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_cmd2_END (8)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_id2_START (9)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_id2_END (11)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_cmd3_START (12)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_cmd3_END (12)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_id3_START (13)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_id3_END (15)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_cmd4_START (16)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_cmd4_END (16)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_id4_START (17)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_id4_END (19)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_cmd5_START (20)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_cmd5_END (20)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_id5_START (21)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_id5_END (23)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_cmd6_START (24)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_cmd6_END (24)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_id6_START (25)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_id6_END (27)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_cmd7_START (28)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_cmd7_END (28)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_id7_START (29)
#define SOC_PCTRL_RESOURCE4_LOCK_resource4lock_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource4unlock_cmd0 : 1;
        unsigned int resource4unlock_id0 : 3;
        unsigned int resource4unlock_cmd1 : 1;
        unsigned int resource4unlock_id1 : 3;
        unsigned int resource4unlock_cmd2 : 1;
        unsigned int resource4unlock_id2 : 3;
        unsigned int resource4unlock_cmd3 : 1;
        unsigned int resource4unlock_id3 : 3;
        unsigned int resource4unlock_cmd4 : 1;
        unsigned int resource4unlock_id4 : 3;
        unsigned int resource4unlock_cmd5 : 1;
        unsigned int resource4unlock_id5 : 3;
        unsigned int resource4unlock_cmd6 : 1;
        unsigned int resource4unlock_id6 : 3;
        unsigned int resource4unlock_cmd7 : 1;
        unsigned int resource4unlock_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE4_UNLOCK_UNION;
#endif
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_cmd0_START (0)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_cmd0_END (0)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_id0_START (1)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_id0_END (3)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_cmd1_START (4)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_cmd1_END (4)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_id1_START (5)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_id1_END (7)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_cmd2_START (8)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_cmd2_END (8)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_id2_START (9)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_id2_END (11)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_cmd3_START (12)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_cmd3_END (12)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_id3_START (13)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_id3_END (15)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_cmd4_START (16)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_cmd4_END (16)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_id4_START (17)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_id4_END (19)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_cmd5_START (20)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_cmd5_END (20)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_id5_START (21)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_id5_END (23)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_cmd6_START (24)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_cmd6_END (24)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_id6_START (25)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_id6_END (27)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_cmd7_START (28)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_cmd7_END (28)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_id7_START (29)
#define SOC_PCTRL_RESOURCE4_UNLOCK_resource4unlock_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource4lock_st0 : 1;
        unsigned int resource4lock_st_id0 : 3;
        unsigned int resource4lock_st1 : 1;
        unsigned int resource4lock_st_id1 : 3;
        unsigned int resource4lock_st2 : 1;
        unsigned int resource4lock_st_id2 : 3;
        unsigned int resource4lock_st3 : 1;
        unsigned int resource4lock_st_id3 : 3;
        unsigned int resource4lock_st4 : 1;
        unsigned int resource4lock_st_id4 : 3;
        unsigned int resource4lock_st5 : 1;
        unsigned int resource4lock_st_id5 : 3;
        unsigned int resource4lock_st6 : 1;
        unsigned int resource4lock_st_id6 : 3;
        unsigned int resource4lock_st7 : 1;
        unsigned int resource4lock_st_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE4_LOCK_ST_UNION;
#endif
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st0_START (0)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st0_END (0)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st_id0_START (1)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st_id0_END (3)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st1_START (4)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st1_END (4)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st_id1_START (5)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st_id1_END (7)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st2_START (8)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st2_END (8)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st_id2_START (9)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st_id2_END (11)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st3_START (12)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st3_END (12)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st_id3_START (13)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st_id3_END (15)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st4_START (16)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st4_END (16)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st_id4_START (17)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st_id4_END (19)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st5_START (20)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st5_END (20)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st_id5_START (21)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st_id5_END (23)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st6_START (24)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st6_END (24)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st_id6_START (25)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st_id6_END (27)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st7_START (28)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st7_END (28)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st_id7_START (29)
#define SOC_PCTRL_RESOURCE4_LOCK_ST_resource4lock_st_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource5lock_cmd0 : 1;
        unsigned int resource5lock_id0 : 3;
        unsigned int resource5lock_cmd1 : 1;
        unsigned int resource5lock_id1 : 3;
        unsigned int resource5lock_cmd2 : 1;
        unsigned int resource5lock_id2 : 3;
        unsigned int resource5lock_cmd3 : 1;
        unsigned int resource5lock_id3 : 3;
        unsigned int resource5lock_cmd4 : 1;
        unsigned int resource5lock_id4 : 3;
        unsigned int resource5lock_cmd5 : 1;
        unsigned int resource5lock_id5 : 3;
        unsigned int resource5lock_cmd6 : 1;
        unsigned int resource5lock_id6 : 3;
        unsigned int resource5lock_cmd7 : 1;
        unsigned int resource5lock_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE5_LOCK_UNION;
#endif
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_cmd0_START (0)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_cmd0_END (0)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_id0_START (1)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_id0_END (3)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_cmd1_START (4)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_cmd1_END (4)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_id1_START (5)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_id1_END (7)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_cmd2_START (8)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_cmd2_END (8)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_id2_START (9)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_id2_END (11)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_cmd3_START (12)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_cmd3_END (12)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_id3_START (13)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_id3_END (15)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_cmd4_START (16)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_cmd4_END (16)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_id4_START (17)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_id4_END (19)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_cmd5_START (20)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_cmd5_END (20)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_id5_START (21)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_id5_END (23)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_cmd6_START (24)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_cmd6_END (24)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_id6_START (25)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_id6_END (27)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_cmd7_START (28)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_cmd7_END (28)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_id7_START (29)
#define SOC_PCTRL_RESOURCE5_LOCK_resource5lock_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource5unlock_cmd0 : 1;
        unsigned int resource5unlock_id0 : 3;
        unsigned int resource5unlock_cmd1 : 1;
        unsigned int resource5unlock_id1 : 3;
        unsigned int resource5unlock_cmd2 : 1;
        unsigned int resource5unlock_id2 : 3;
        unsigned int resource5unlock_cmd3 : 1;
        unsigned int resource5unlock_id3 : 3;
        unsigned int resource5unlock_cmd4 : 1;
        unsigned int resource5unlock_id4 : 3;
        unsigned int resource5unlock_cmd5 : 1;
        unsigned int resource5unlock_id5 : 3;
        unsigned int resource5unlock_cmd6 : 1;
        unsigned int resource5unlock_id6 : 3;
        unsigned int resource5unlock_cmd7 : 1;
        unsigned int resource5unlock_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE5_UNLOCK_UNION;
#endif
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_cmd0_START (0)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_cmd0_END (0)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_id0_START (1)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_id0_END (3)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_cmd1_START (4)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_cmd1_END (4)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_id1_START (5)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_id1_END (7)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_cmd2_START (8)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_cmd2_END (8)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_id2_START (9)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_id2_END (11)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_cmd3_START (12)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_cmd3_END (12)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_id3_START (13)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_id3_END (15)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_cmd4_START (16)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_cmd4_END (16)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_id4_START (17)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_id4_END (19)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_cmd5_START (20)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_cmd5_END (20)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_id5_START (21)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_id5_END (23)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_cmd6_START (24)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_cmd6_END (24)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_id6_START (25)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_id6_END (27)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_cmd7_START (28)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_cmd7_END (28)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_id7_START (29)
#define SOC_PCTRL_RESOURCE5_UNLOCK_resource5unlock_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource5lock_st0 : 1;
        unsigned int resource5lock_st_id0 : 3;
        unsigned int resource5lock_st1 : 1;
        unsigned int resource5lock_st_id1 : 3;
        unsigned int resource5lock_st2 : 1;
        unsigned int resource5lock_st_id2 : 3;
        unsigned int resource5lock_st3 : 1;
        unsigned int resource5lock_st_id3 : 3;
        unsigned int resource5lock_st4 : 1;
        unsigned int resource5lock_st_id4 : 3;
        unsigned int resource5lock_st5 : 1;
        unsigned int resource5lock_st_id5 : 3;
        unsigned int resource5lock_st6 : 1;
        unsigned int resource5lock_st_id6 : 3;
        unsigned int resource5lock_st7 : 1;
        unsigned int resource5lock_st_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE5_LOCK_ST_UNION;
#endif
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st0_START (0)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st0_END (0)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st_id0_START (1)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st_id0_END (3)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st1_START (4)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st1_END (4)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st_id1_START (5)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st_id1_END (7)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st2_START (8)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st2_END (8)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st_id2_START (9)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st_id2_END (11)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st3_START (12)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st3_END (12)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st_id3_START (13)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st_id3_END (15)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st4_START (16)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st4_END (16)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st_id4_START (17)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st_id4_END (19)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st5_START (20)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st5_END (20)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st_id5_START (21)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st_id5_END (23)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st6_START (24)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st6_END (24)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st_id6_START (25)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st_id6_END (27)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st7_START (28)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st7_END (28)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st_id7_START (29)
#define SOC_PCTRL_RESOURCE5_LOCK_ST_resource5lock_st_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource6lock_cmd0 : 1;
        unsigned int resource6lock_id0 : 3;
        unsigned int resource6lock_cmd1 : 1;
        unsigned int resource6lock_id1 : 3;
        unsigned int resource6lock_cmd2 : 1;
        unsigned int resource6lock_id2 : 3;
        unsigned int resource6lock_cmd3 : 1;
        unsigned int resource6lock_id3 : 3;
        unsigned int resource6lock_cmd4 : 1;
        unsigned int resource6lock_id4 : 3;
        unsigned int resource6lock_cmd5 : 1;
        unsigned int resource6lock_id5 : 3;
        unsigned int resource6lock_cmd6 : 1;
        unsigned int resource6lock_id6 : 3;
        unsigned int resource6lock_cmd7 : 1;
        unsigned int resource6lock_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE6_LOCK_UNION;
#endif
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_cmd0_START (0)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_cmd0_END (0)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_id0_START (1)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_id0_END (3)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_cmd1_START (4)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_cmd1_END (4)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_id1_START (5)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_id1_END (7)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_cmd2_START (8)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_cmd2_END (8)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_id2_START (9)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_id2_END (11)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_cmd3_START (12)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_cmd3_END (12)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_id3_START (13)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_id3_END (15)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_cmd4_START (16)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_cmd4_END (16)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_id4_START (17)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_id4_END (19)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_cmd5_START (20)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_cmd5_END (20)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_id5_START (21)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_id5_END (23)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_cmd6_START (24)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_cmd6_END (24)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_id6_START (25)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_id6_END (27)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_cmd7_START (28)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_cmd7_END (28)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_id7_START (29)
#define SOC_PCTRL_RESOURCE6_LOCK_resource6lock_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource6unlock_cmd0 : 1;
        unsigned int resource6unlock_id0 : 3;
        unsigned int resource6unlock_cmd1 : 1;
        unsigned int resource6unlock_id1 : 3;
        unsigned int resource6unlock_cmd2 : 1;
        unsigned int resource6unlock_id2 : 3;
        unsigned int resource6unlock_cmd3 : 1;
        unsigned int resource6unlock_id3 : 3;
        unsigned int resource6unlock_cmd4 : 1;
        unsigned int resource6unlock_id4 : 3;
        unsigned int resource6unlock_cmd5 : 1;
        unsigned int resource6unlock_id5 : 3;
        unsigned int resource6unlock_cmd6 : 1;
        unsigned int resource6unlock_id6 : 3;
        unsigned int resource6unlock_cmd7 : 1;
        unsigned int resource6unlock_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE6_UNLOCK_UNION;
#endif
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_cmd0_START (0)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_cmd0_END (0)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_id0_START (1)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_id0_END (3)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_cmd1_START (4)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_cmd1_END (4)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_id1_START (5)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_id1_END (7)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_cmd2_START (8)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_cmd2_END (8)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_id2_START (9)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_id2_END (11)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_cmd3_START (12)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_cmd3_END (12)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_id3_START (13)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_id3_END (15)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_cmd4_START (16)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_cmd4_END (16)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_id4_START (17)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_id4_END (19)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_cmd5_START (20)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_cmd5_END (20)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_id5_START (21)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_id5_END (23)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_cmd6_START (24)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_cmd6_END (24)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_id6_START (25)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_id6_END (27)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_cmd7_START (28)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_cmd7_END (28)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_id7_START (29)
#define SOC_PCTRL_RESOURCE6_UNLOCK_resource6unlock_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource6lock_st0 : 1;
        unsigned int resource6lock_st_id0 : 3;
        unsigned int resource6lock_st1 : 1;
        unsigned int resource6lock_st_id1 : 3;
        unsigned int resource6lock_st2 : 1;
        unsigned int resource6lock_st_id2 : 3;
        unsigned int resource6lock_st3 : 1;
        unsigned int resource6lock_st_id3 : 3;
        unsigned int resource6lock_st4 : 1;
        unsigned int resource6lock_st_id4 : 3;
        unsigned int resource6lock_st5 : 1;
        unsigned int resource6lock_st_id5 : 3;
        unsigned int resource6lock_st6 : 1;
        unsigned int resource6lock_st_id6 : 3;
        unsigned int resource6lock_st7 : 1;
        unsigned int resource6lock_st_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE6_LOCK_ST_UNION;
#endif
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st0_START (0)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st0_END (0)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st_id0_START (1)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st_id0_END (3)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st1_START (4)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st1_END (4)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st_id1_START (5)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st_id1_END (7)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st2_START (8)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st2_END (8)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st_id2_START (9)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st_id2_END (11)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st3_START (12)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st3_END (12)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st_id3_START (13)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st_id3_END (15)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st4_START (16)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st4_END (16)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st_id4_START (17)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st_id4_END (19)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st5_START (20)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st5_END (20)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st_id5_START (21)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st_id5_END (23)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st6_START (24)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st6_END (24)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st_id6_START (25)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st_id6_END (27)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st7_START (28)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st7_END (28)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st_id7_START (29)
#define SOC_PCTRL_RESOURCE6_LOCK_ST_resource6lock_st_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource7lock_cmd0 : 1;
        unsigned int resource7lock_id0 : 3;
        unsigned int resource7lock_cmd1 : 1;
        unsigned int resource7lock_id1 : 3;
        unsigned int resource7lock_cmd2 : 1;
        unsigned int resource7lock_id2 : 3;
        unsigned int resource7lock_cmd3 : 1;
        unsigned int resource7lock_id3 : 3;
        unsigned int resource7lock_cmd4 : 1;
        unsigned int resource7lock_id4 : 3;
        unsigned int resource7lock_cmd5 : 1;
        unsigned int resource7lock_id5 : 3;
        unsigned int resource7lock_cmd6 : 1;
        unsigned int resource7lock_id6 : 3;
        unsigned int resource7lock_cmd7 : 1;
        unsigned int resource7lock_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE7_LOCK_UNION;
#endif
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_cmd0_START (0)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_cmd0_END (0)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_id0_START (1)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_id0_END (3)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_cmd1_START (4)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_cmd1_END (4)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_id1_START (5)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_id1_END (7)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_cmd2_START (8)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_cmd2_END (8)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_id2_START (9)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_id2_END (11)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_cmd3_START (12)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_cmd3_END (12)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_id3_START (13)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_id3_END (15)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_cmd4_START (16)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_cmd4_END (16)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_id4_START (17)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_id4_END (19)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_cmd5_START (20)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_cmd5_END (20)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_id5_START (21)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_id5_END (23)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_cmd6_START (24)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_cmd6_END (24)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_id6_START (25)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_id6_END (27)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_cmd7_START (28)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_cmd7_END (28)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_id7_START (29)
#define SOC_PCTRL_RESOURCE7_LOCK_resource7lock_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource7unlock_cmd0 : 1;
        unsigned int resource7unlock_id0 : 3;
        unsigned int resource7unlock_cmd1 : 1;
        unsigned int resource7unlock_id1 : 3;
        unsigned int resource7unlock_cmd2 : 1;
        unsigned int resource7unlock_id2 : 3;
        unsigned int resource7unlock_cmd3 : 1;
        unsigned int resource7unlock_id3 : 3;
        unsigned int resource7unlock_cmd4 : 1;
        unsigned int resource7unlock_id4 : 3;
        unsigned int resource7unlock_cmd5 : 1;
        unsigned int resource7unlock_id5 : 3;
        unsigned int resource7unlock_cmd6 : 1;
        unsigned int resource7unlock_id6 : 3;
        unsigned int resource7unlock_cmd7 : 1;
        unsigned int resource7unlock_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE7_UNLOCK_UNION;
#endif
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_cmd0_START (0)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_cmd0_END (0)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_id0_START (1)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_id0_END (3)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_cmd1_START (4)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_cmd1_END (4)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_id1_START (5)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_id1_END (7)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_cmd2_START (8)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_cmd2_END (8)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_id2_START (9)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_id2_END (11)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_cmd3_START (12)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_cmd3_END (12)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_id3_START (13)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_id3_END (15)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_cmd4_START (16)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_cmd4_END (16)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_id4_START (17)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_id4_END (19)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_cmd5_START (20)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_cmd5_END (20)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_id5_START (21)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_id5_END (23)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_cmd6_START (24)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_cmd6_END (24)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_id6_START (25)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_id6_END (27)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_cmd7_START (28)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_cmd7_END (28)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_id7_START (29)
#define SOC_PCTRL_RESOURCE7_UNLOCK_resource7unlock_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int resource7lock_st0 : 1;
        unsigned int resource7lock_st_id0 : 3;
        unsigned int resource7lock_st1 : 1;
        unsigned int resource7lock_st_id1 : 3;
        unsigned int resource7lock_st2 : 1;
        unsigned int resource7lock_st_id2 : 3;
        unsigned int resource7lock_st3 : 1;
        unsigned int resource7lock_st_id3 : 3;
        unsigned int resource7lock_st4 : 1;
        unsigned int resource7lock_st_id4 : 3;
        unsigned int resource7lock_st5 : 1;
        unsigned int resource7lock_st_id5 : 3;
        unsigned int resource7lock_st6 : 1;
        unsigned int resource7lock_st_id6 : 3;
        unsigned int resource7lock_st7 : 1;
        unsigned int resource7lock_st_id7 : 3;
    } reg;
} SOC_PCTRL_RESOURCE7_LOCK_ST_UNION;
#endif
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st0_START (0)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st0_END (0)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st_id0_START (1)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st_id0_END (3)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st1_START (4)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st1_END (4)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st_id1_START (5)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st_id1_END (7)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st2_START (8)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st2_END (8)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st_id2_START (9)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st_id2_END (11)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st3_START (12)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st3_END (12)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st_id3_START (13)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st_id3_END (15)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st4_START (16)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st4_END (16)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st_id4_START (17)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st_id4_END (19)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st5_START (20)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st5_END (20)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st_id5_START (21)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st_id5_END (23)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st6_START (24)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st6_END (24)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st_id6_START (25)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st_id6_END (27)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st7_START (28)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st7_END (28)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st_id7_START (29)
#define SOC_PCTRL_RESOURCE7_LOCK_ST_resource7lock_st_id7_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int lpm3_mid : 6;
        unsigned int iomcu_mid : 6;
        unsigned int emmc_ufs_mid : 6;
        unsigned int sd3_mid : 6;
        unsigned int sdio_mid : 6;
        unsigned int reserved : 2;
    } reg;
} SOC_PCTRL_PERI_CTRL5_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL5_lpm3_mid_START (0)
#define SOC_PCTRL_PERI_CTRL5_lpm3_mid_END (5)
#define SOC_PCTRL_PERI_CTRL5_iomcu_mid_START (6)
#define SOC_PCTRL_PERI_CTRL5_iomcu_mid_END (11)
#define SOC_PCTRL_PERI_CTRL5_emmc_ufs_mid_START (12)
#define SOC_PCTRL_PERI_CTRL5_emmc_ufs_mid_END (17)
#define SOC_PCTRL_PERI_CTRL5_sd3_mid_START (18)
#define SOC_PCTRL_PERI_CTRL5_sd3_mid_END (23)
#define SOC_PCTRL_PERI_CTRL5_sdio_mid_START (24)
#define SOC_PCTRL_PERI_CTRL5_sdio_mid_END (29)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int secp_mid : 6;
        unsigned int secs_mid : 6;
        unsigned int socp_mid : 6;
        unsigned int usb3otg_mid : 6;
        unsigned int sc_perf_stat_mid : 6;
        unsigned int reserved : 2;
    } reg;
} SOC_PCTRL_PERI_CTRL6_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL6_secp_mid_START (0)
#define SOC_PCTRL_PERI_CTRL6_secp_mid_END (5)
#define SOC_PCTRL_PERI_CTRL6_secs_mid_START (6)
#define SOC_PCTRL_PERI_CTRL6_secs_mid_END (11)
#define SOC_PCTRL_PERI_CTRL6_socp_mid_START (12)
#define SOC_PCTRL_PERI_CTRL6_socp_mid_END (17)
#define SOC_PCTRL_PERI_CTRL6_usb3otg_mid_START (18)
#define SOC_PCTRL_PERI_CTRL6_usb3otg_mid_END (23)
#define SOC_PCTRL_PERI_CTRL6_sc_perf_stat_mid_START (24)
#define SOC_PCTRL_PERI_CTRL6_sc_perf_stat_mid_END (29)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int modem_arqos : 5;
        unsigned int modem_awqos : 5;
        unsigned int modemcpu_mid : 6;
        unsigned int reserved : 4;
        unsigned int a53_mid : 6;
        unsigned int a57_mid : 6;
    } reg;
} SOC_PCTRL_PERI_CTRL7_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL7_modem_arqos_START (0)
#define SOC_PCTRL_PERI_CTRL7_modem_arqos_END (4)
#define SOC_PCTRL_PERI_CTRL7_modem_awqos_START (5)
#define SOC_PCTRL_PERI_CTRL7_modem_awqos_END (9)
#define SOC_PCTRL_PERI_CTRL7_modemcpu_mid_START (10)
#define SOC_PCTRL_PERI_CTRL7_modemcpu_mid_END (15)
#define SOC_PCTRL_PERI_CTRL7_a53_mid_START (20)
#define SOC_PCTRL_PERI_CTRL7_a53_mid_END (25)
#define SOC_PCTRL_PERI_CTRL7_a57_mid_START (26)
#define SOC_PCTRL_PERI_CTRL7_a57_mid_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int g3d0_arnasid : 5;
        unsigned int g3d0_awnasid : 5;
        unsigned int g3d1_arnasid : 5;
        unsigned int g3d1_awnasid : 5;
        unsigned int g3d_arqos : 4;
        unsigned int reserved_0 : 1;
        unsigned int g3d_awqos : 4;
        unsigned int reserved_1 : 3;
    } reg;
} SOC_PCTRL_PERI_CTRL8_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL8_g3d0_arnasid_START (0)
#define SOC_PCTRL_PERI_CTRL8_g3d0_arnasid_END (4)
#define SOC_PCTRL_PERI_CTRL8_g3d0_awnasid_START (5)
#define SOC_PCTRL_PERI_CTRL8_g3d0_awnasid_END (9)
#define SOC_PCTRL_PERI_CTRL8_g3d1_arnasid_START (10)
#define SOC_PCTRL_PERI_CTRL8_g3d1_arnasid_END (14)
#define SOC_PCTRL_PERI_CTRL8_g3d1_awnasid_START (15)
#define SOC_PCTRL_PERI_CTRL8_g3d1_awnasid_END (19)
#define SOC_PCTRL_PERI_CTRL8_g3d_arqos_START (20)
#define SOC_PCTRL_PERI_CTRL8_g3d_arqos_END (23)
#define SOC_PCTRL_PERI_CTRL8_g3d_awqos_START (25)
#define SOC_PCTRL_PERI_CTRL8_g3d_awqos_END (28)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int asp_mid : 6;
        unsigned int pcie_mid : 6;
        unsigned int a7_toviviobus_mid : 6;
        unsigned int a7_cfg_mid : 6;
        unsigned int reserved : 4;
        unsigned int dss0_mid : 2;
        unsigned int dss1_mid : 2;
    } reg;
} SOC_PCTRL_PERI_CTRL9_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL9_asp_mid_START (0)
#define SOC_PCTRL_PERI_CTRL9_asp_mid_END (5)
#define SOC_PCTRL_PERI_CTRL9_pcie_mid_START (6)
#define SOC_PCTRL_PERI_CTRL9_pcie_mid_END (11)
#define SOC_PCTRL_PERI_CTRL9_a7_toviviobus_mid_START (12)
#define SOC_PCTRL_PERI_CTRL9_a7_toviviobus_mid_END (17)
#define SOC_PCTRL_PERI_CTRL9_a7_cfg_mid_START (18)
#define SOC_PCTRL_PERI_CTRL9_a7_cfg_mid_END (23)
#define SOC_PCTRL_PERI_CTRL9_dss0_mid_START (28)
#define SOC_PCTRL_PERI_CTRL9_dss0_mid_END (29)
#define SOC_PCTRL_PERI_CTRL9_dss1_mid_START (30)
#define SOC_PCTRL_PERI_CTRL9_dss1_mid_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int ddrc0_mid : 1;
        unsigned int ddrc1_mid : 1;
        unsigned int ddrc2_mid : 1;
        unsigned int ddrc3_mid : 1;
        unsigned int ddrc4_mid : 1;
        unsigned int ddrc5_mid : 1;
        unsigned int ddrc6_mid : 1;
        unsigned int ddrc7_mid : 1;
        unsigned int ddrc8_mid : 1;
        unsigned int ddrc9_mid : 1;
        unsigned int ddrc10_mid : 1;
        unsigned int reserved : 3;
        unsigned int ipf_mid : 6;
        unsigned int cssys_mid : 6;
        unsigned int iomcu_dma_mid : 6;
    } reg;
} SOC_PCTRL_PERI_CTRL10_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL10_ddrc0_mid_START (0)
#define SOC_PCTRL_PERI_CTRL10_ddrc0_mid_END (0)
#define SOC_PCTRL_PERI_CTRL10_ddrc1_mid_START (1)
#define SOC_PCTRL_PERI_CTRL10_ddrc1_mid_END (1)
#define SOC_PCTRL_PERI_CTRL10_ddrc2_mid_START (2)
#define SOC_PCTRL_PERI_CTRL10_ddrc2_mid_END (2)
#define SOC_PCTRL_PERI_CTRL10_ddrc3_mid_START (3)
#define SOC_PCTRL_PERI_CTRL10_ddrc3_mid_END (3)
#define SOC_PCTRL_PERI_CTRL10_ddrc4_mid_START (4)
#define SOC_PCTRL_PERI_CTRL10_ddrc4_mid_END (4)
#define SOC_PCTRL_PERI_CTRL10_ddrc5_mid_START (5)
#define SOC_PCTRL_PERI_CTRL10_ddrc5_mid_END (5)
#define SOC_PCTRL_PERI_CTRL10_ddrc6_mid_START (6)
#define SOC_PCTRL_PERI_CTRL10_ddrc6_mid_END (6)
#define SOC_PCTRL_PERI_CTRL10_ddrc7_mid_START (7)
#define SOC_PCTRL_PERI_CTRL10_ddrc7_mid_END (7)
#define SOC_PCTRL_PERI_CTRL10_ddrc8_mid_START (8)
#define SOC_PCTRL_PERI_CTRL10_ddrc8_mid_END (8)
#define SOC_PCTRL_PERI_CTRL10_ddrc9_mid_START (9)
#define SOC_PCTRL_PERI_CTRL10_ddrc9_mid_END (9)
#define SOC_PCTRL_PERI_CTRL10_ddrc10_mid_START (10)
#define SOC_PCTRL_PERI_CTRL10_ddrc10_mid_END (10)
#define SOC_PCTRL_PERI_CTRL10_ipf_mid_START (14)
#define SOC_PCTRL_PERI_CTRL10_ipf_mid_END (19)
#define SOC_PCTRL_PERI_CTRL10_cssys_mid_START (20)
#define SOC_PCTRL_PERI_CTRL10_cssys_mid_END (25)
#define SOC_PCTRL_PERI_CTRL10_iomcu_dma_mid_START (26)
#define SOC_PCTRL_PERI_CTRL10_iomcu_dma_mid_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int mcpu_boot_addr : 16;
        unsigned int reserved : 16;
    } reg;
} SOC_PCTRL_PERI_CTRL11_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL11_mcpu_boot_addr_START (0)
#define SOC_PCTRL_PERI_CTRL11_mcpu_boot_addr_END (15)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int g3d0_mid0 : 6;
        unsigned int g3d0_mid1 : 6;
        unsigned int g3d1_mid0 : 6;
        unsigned int g3d1_mid1 : 6;
        unsigned int reserved_0: 6;
        unsigned int reserved_1: 2;
    } reg;
} SOC_PCTRL_PERI_CTRL34_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL34_g3d0_mid0_START (0)
#define SOC_PCTRL_PERI_CTRL34_g3d0_mid0_END (5)
#define SOC_PCTRL_PERI_CTRL34_g3d0_mid1_START (6)
#define SOC_PCTRL_PERI_CTRL34_g3d0_mid1_END (11)
#define SOC_PCTRL_PERI_CTRL34_g3d1_mid0_START (12)
#define SOC_PCTRL_PERI_CTRL34_g3d1_mid0_END (17)
#define SOC_PCTRL_PERI_CTRL34_g3d1_mid1_START (18)
#define SOC_PCTRL_PERI_CTRL34_g3d1_mid1_END (23)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int ocbc_mid : 6;
        unsigned int hisee_mid : 6;
        unsigned int djtag_mid : 6;
        unsigned int dmac_mid : 6;
        unsigned int reserved_0: 6;
        unsigned int reserved_1: 2;
    } reg;
} SOC_PCTRL_PERI_CTRL35_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL35_ocbc_mid_START (0)
#define SOC_PCTRL_PERI_CTRL35_ocbc_mid_END (5)
#define SOC_PCTRL_PERI_CTRL35_hisee_mid_START (6)
#define SOC_PCTRL_PERI_CTRL35_hisee_mid_END (11)
#define SOC_PCTRL_PERI_CTRL35_djtag_mid_START (12)
#define SOC_PCTRL_PERI_CTRL35_djtag_mid_END (17)
#define SOC_PCTRL_PERI_CTRL35_dmac_mid_START (18)
#define SOC_PCTRL_PERI_CTRL35_dmac_mid_END (23)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int venc_mid : 6;
        unsigned int vdec_mid : 6;
        unsigned int g3d0_crnasid : 5;
        unsigned int g3d1_crnasid : 5;
        unsigned int reserved : 10;
    } reg;
} SOC_PCTRL_PERI_CTRL38_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL38_venc_mid_START (0)
#define SOC_PCTRL_PERI_CTRL38_venc_mid_END (5)
#define SOC_PCTRL_PERI_CTRL38_vdec_mid_START (6)
#define SOC_PCTRL_PERI_CTRL38_vdec_mid_END (11)
#define SOC_PCTRL_PERI_CTRL38_g3d0_crnasid_START (12)
#define SOC_PCTRL_PERI_CTRL38_g3d0_crnasid_END (16)
#define SOC_PCTRL_PERI_CTRL38_g3d1_crnasid_START (17)
#define SOC_PCTRL_PERI_CTRL38_g3d1_crnasid_END (21)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved : 32;
    } reg;
} SOC_PCTRL_PERI_CTRL39_UNION;
#endif
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved : 32;
    } reg;
} SOC_PCTRL_PERI_CTRL40_UNION;
#endif
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved : 32;
    } reg;
} SOC_PCTRL_PERI_CTRL41_UNION;
#endif
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved : 32;
    } reg;
} SOC_PCTRL_PERI_CTRL42_UNION;
#endif
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int media_fama_deft_gran : 4;
        unsigned int media_fama_en : 1;
        unsigned int reserved_0 : 1;
        unsigned int media_fama_perf_pwr_sel : 1;
        unsigned int reserved_1 : 25;
    } reg;
} SOC_PCTRL_PERI_CTRL43_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL43_media_fama_deft_gran_START (0)
#define SOC_PCTRL_PERI_CTRL43_media_fama_deft_gran_END (3)
#define SOC_PCTRL_PERI_CTRL43_media_fama_en_START (4)
#define SOC_PCTRL_PERI_CTRL43_media_fama_en_END (4)
#define SOC_PCTRL_PERI_CTRL43_media_fama_perf_pwr_sel_START (6)
#define SOC_PCTRL_PERI_CTRL43_media_fama_perf_pwr_sel_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int djtag_addr_msb : 7;
        unsigned int reserved : 25;
    } reg;
} SOC_PCTRL_PERI_CTRL44_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL44_djtag_addr_msb_START (0)
#define SOC_PCTRL_PERI_CTRL44_djtag_addr_msb_END (6)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved : 32;
    } reg;
} SOC_PCTRL_PERI_CTRL48_UNION;
#endif
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved : 32;
    } reg;
} SOC_PCTRL_PERI_CTRL49_UNION;
#endif
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int modem_fama_icfg_addr_src0 : 20;
        unsigned int reserved : 12;
    } reg;
} SOC_PCTRL_PERI_CTRL50_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL50_modem_fama_icfg_addr_src0_START (0)
#define SOC_PCTRL_PERI_CTRL50_modem_fama_icfg_addr_src0_END (19)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int modem_fama_icfg_addr_len0 : 20;
        unsigned int reserved : 12;
    } reg;
} SOC_PCTRL_PERI_CTRL51_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL51_modem_fama_icfg_addr_len0_START (0)
#define SOC_PCTRL_PERI_CTRL51_modem_fama_icfg_addr_len0_END (19)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int modem_fama_icfg_addr_dst0 : 20;
        unsigned int modem_fama_addr_msb0 : 7;
        unsigned int reserved : 5;
    } reg;
} SOC_PCTRL_PERI_CTRL52_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL52_modem_fama_icfg_addr_dst0_START (0)
#define SOC_PCTRL_PERI_CTRL52_modem_fama_icfg_addr_dst0_END (19)
#define SOC_PCTRL_PERI_CTRL52_modem_fama_addr_msb0_START (20)
#define SOC_PCTRL_PERI_CTRL52_modem_fama_addr_msb0_END (26)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int modem_fama_icfg_addr_src1 : 20;
        unsigned int reserved : 12;
    } reg;
} SOC_PCTRL_PERI_CTRL53_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL53_modem_fama_icfg_addr_src1_START (0)
#define SOC_PCTRL_PERI_CTRL53_modem_fama_icfg_addr_src1_END (19)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int modem_fama_icfg_addr_len1 : 20;
        unsigned int reserved : 12;
    } reg;
} SOC_PCTRL_PERI_CTRL54_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL54_modem_fama_icfg_addr_len1_START (0)
#define SOC_PCTRL_PERI_CTRL54_modem_fama_icfg_addr_len1_END (19)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int modem_fama_icfg_addr_dst1 : 20;
        unsigned int modem_fama_addr_msb1 : 7;
        unsigned int reserved : 5;
    } reg;
} SOC_PCTRL_PERI_CTRL55_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL55_modem_fama_icfg_addr_dst1_START (0)
#define SOC_PCTRL_PERI_CTRL55_modem_fama_icfg_addr_dst1_END (19)
#define SOC_PCTRL_PERI_CTRL55_modem_fama_addr_msb1_START (20)
#define SOC_PCTRL_PERI_CTRL55_modem_fama_addr_msb1_END (26)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int modem_fama_icfg_addr_src2 : 20;
        unsigned int reserved : 12;
    } reg;
} SOC_PCTRL_PERI_CTRL56_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL56_modem_fama_icfg_addr_src2_START (0)
#define SOC_PCTRL_PERI_CTRL56_modem_fama_icfg_addr_src2_END (19)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int modem_fama_icfg_addr_len2 : 20;
        unsigned int reserved : 12;
    } reg;
} SOC_PCTRL_PERI_CTRL57_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL57_modem_fama_icfg_addr_len2_START (0)
#define SOC_PCTRL_PERI_CTRL57_modem_fama_icfg_addr_len2_END (19)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int modem_fama_icfg_addr_dst2 : 20;
        unsigned int modem_fama_addr_msb2 : 7;
        unsigned int reserved : 5;
    } reg;
} SOC_PCTRL_PERI_CTRL58_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL58_modem_fama_icfg_addr_dst2_START (0)
#define SOC_PCTRL_PERI_CTRL58_modem_fama_icfg_addr_dst2_END (19)
#define SOC_PCTRL_PERI_CTRL58_modem_fama_addr_msb2_START (20)
#define SOC_PCTRL_PERI_CTRL58_modem_fama_addr_msb2_END (26)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int modem_fama_icfg_addr_src3 : 20;
        unsigned int reserved : 12;
    } reg;
} SOC_PCTRL_PERI_CTRL59_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL59_modem_fama_icfg_addr_src3_START (0)
#define SOC_PCTRL_PERI_CTRL59_modem_fama_icfg_addr_src3_END (19)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int modem_fama_icfg_addr_len3 : 20;
        unsigned int reserved : 12;
    } reg;
} SOC_PCTRL_PERI_CTRL60_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL60_modem_fama_icfg_addr_len3_START (0)
#define SOC_PCTRL_PERI_CTRL60_modem_fama_icfg_addr_len3_END (19)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int modem_fama_icfg_addr_dst3 : 20;
        unsigned int modem_fama_addr_msb3 : 7;
        unsigned int reserved : 5;
    } reg;
} SOC_PCTRL_PERI_CTRL61_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL61_modem_fama_icfg_addr_dst3_START (0)
#define SOC_PCTRL_PERI_CTRL61_modem_fama_icfg_addr_dst3_END (19)
#define SOC_PCTRL_PERI_CTRL61_modem_fama_addr_msb3_START (20)
#define SOC_PCTRL_PERI_CTRL61_modem_fama_addr_msb3_END (26)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved_0 : 8;
        unsigned int modem_fama_icfg_id : 8;
        unsigned int modem_fama_addr_id_remap : 7;
        unsigned int reserved_1 : 1;
        unsigned int modem_fama_addr_default : 7;
        unsigned int modem_id_flt_en : 1;
    } reg;
} SOC_PCTRL_PERI_CTRL62_UNION;
#endif
#define SOC_PCTRL_PERI_CTRL62_modem_fama_icfg_id_START (8)
#define SOC_PCTRL_PERI_CTRL62_modem_fama_icfg_id_END (15)
#define SOC_PCTRL_PERI_CTRL62_modem_fama_addr_id_remap_START (16)
#define SOC_PCTRL_PERI_CTRL62_modem_fama_addr_id_remap_END (22)
#define SOC_PCTRL_PERI_CTRL62_modem_fama_addr_default_START (24)
#define SOC_PCTRL_PERI_CTRL62_modem_fama_addr_default_END (30)
#define SOC_PCTRL_PERI_CTRL62_modem_id_flt_en_START (31)
#define SOC_PCTRL_PERI_CTRL62_modem_id_flt_en_END (31)
#ifndef __SOC_H_FOR_ASM__
typedef union
{
    unsigned int value;
    struct
    {
        unsigned int reserved : 32;
    } reg;
} SOC_PCTRL_PERI_CTRL63_UNION;
#endif
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
#endif
