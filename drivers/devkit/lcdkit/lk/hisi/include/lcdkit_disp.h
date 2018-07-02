#ifndef _LCDKIT_DISP_H_
#define _LCDKIT_DISP_H_

#include <stdint.h>
#include "hisi_mipi_dsi.h"
#include "lcdkit_display_effect.h"
#include "lcdkit_bias_ic_common.h"
#include "lcd_bl.h"
/********************************************************************
*macro define
********************************************************************/
#define LCDKIT_MODULE_NAME          lcdkit_panel
#define LCDKIT_MODULE_NAME_STR     "lcdkit_panel"

#define LCDKIT_REG_TOTAL_BYTES    4
#define LCDKIT_REG_TOTAL_WORDS    ((LCDKIT_REG_TOTAL_BYTES + 3)/4)

#define LCDKIT_POWER_STATUS_CHECK (1)
#define LCDKIT_TP_COLOR   (1)
#define LCD_OEM_LEN 64
#define HYBRID   (1)
#define AMOLED   (3)


#define BITS(x)     (1<<x)
/*panel type
 *0x01:LCD
 *0x10:OLED
 *
*/
#define  PANEL_TYPE_LCD  BITS(0)
#define  PANEL_TYPE_OLED BITS(1)

#define  POWER_CTRL_BY_NONE BITS(4)
#define  POWER_CTRL_BY_REGULATOR  BITS(0)
#define  POWER_CTRL_BY_GPIO BITS(1)
#define  POWER_CTRL_BY_IC BITS(2)

/*******************************************************************************
** LCD GPIO
*/
#define GPIO_LCDKIT_RESET_NAME	"gpio_lcdkit_reset"
#define GPIO_LCDKIT_TE0_NAME	"gpio_lcdkit_te0"
#define GPIO_LCDKIT_BL_ENABLE_NAME	      "gpio_lcdkit_bl_enable"
#define GPIO_LCDKIT_BL_POWER_ENABLE_NAME   "gpio_lcdkit_bl_power_enable"
#define GPIO_LCDKIT_P5V5_ENABLE_NAME	   "gpio_lcdkit_p5v5_enable"
#define GPIO_LCDKIT_N5V5_ENABLE_NAME       "gpio_lcdkit_n5v5_enable"
#define GPIO_LCDKIT_IOVCC_NAME       "gpio_lcdkit_iovcc"
#define GPIO_LCDKIT_VCI_NAME       "gpio_lcdkit_vci"
#define GPIO_LCDKIT_TP_RESET_NAME	"gpio_lcdkit_tp_reset"
#define GPIO_LCDKIT_VBAT_NAME       "gpio_lcdkit_vbat"

static uint32_t gpio_lcdkit_reset;
static uint32_t gpio_lcdkit_te0;
static uint32_t gpio_lcdkit_iovcc;
static uint32_t gpio_lcdkit_vci;
static uint32_t gpio_lcdkit_vsp;
static uint32_t gpio_lcdkit_vsn;
static uint32_t gpio_lcdkit_bl;
static uint32_t gpio_lcdkit_bl_power;
static uint32_t gpio_lcdkit_tp_reset;
static uint32_t gpio_lcdkit_vbat;

extern char lcd_type_buf[LCD_TYPE_NAME_MAX];
void lcdkit_dts_set_ic_name(char *prop_name, char *name);
void lcdkit_power_on_bias_enable(void);
/********************************
*tp reset gpio
*/
static struct gpio_desc lcdkit_gpio_tp_reset_normal_cmds[] =
{
    {
        DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 1,
        GPIO_LCDKIT_TP_RESET_NAME, &gpio_lcdkit_tp_reset, 1
    },
};
static struct gpio_desc lcdkit_pinctrl_tp_normal_cmds[] =
{
    {
        DTYPE_GPIO_PMX, WAIT_TYPE_MS, 1,
        GPIO_LCDKIT_TP_RESET_NAME, &gpio_lcdkit_tp_reset, FUNCTION0
    },
};
/********************************
*reset gpio
*/
static struct gpio_desc lcdkit_gpio_reset_request_cmds[] =
{
    /* reset */
    {
        DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
        GPIO_LCDKIT_RESET_NAME, &gpio_lcdkit_reset, 0
    },
};

static struct gpio_desc lcdkit_gpio_reset_free_cmds[] =
{
    /* reset */
    {
        DTYPE_GPIO_FREE, WAIT_TYPE_US, 100,
        GPIO_LCDKIT_RESET_NAME, &gpio_lcdkit_reset, 0
    },
};

static struct gpio_desc lcdkit_gpio_reset_normal_cmds[] =
{
    /* reset */
    {
        DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
        GPIO_LCDKIT_RESET_NAME, &gpio_lcdkit_reset, 1
    },
    {
        DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
        GPIO_LCDKIT_RESET_NAME, &gpio_lcdkit_reset, 0
    },
    {
        DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
        GPIO_LCDKIT_RESET_NAME, &gpio_lcdkit_reset, 1
    },
};

static struct gpio_desc lcdkit_gpio_reset_high_cmds[] =
{
    /* reset */
    {
        DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
        GPIO_LCDKIT_RESET_NAME, &gpio_lcdkit_reset, 1
    },
};

static struct gpio_desc lcdkit_gpio_reset_low_cmds[] =
{
    /* reset */
    {
        DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
        GPIO_LCDKIT_RESET_NAME, &gpio_lcdkit_reset, 0
    },
};

/*************************
**vcc and bl gpio
*/
static struct gpio_desc lcdkit_bias_request_cmds[] =
{
    /*AVDD +5.5V*/
    {
        DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
        GPIO_LCDKIT_P5V5_ENABLE_NAME, &gpio_lcdkit_vsp, 0
    },
    /* AVEE_-5.5V */
    {
        DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
        GPIO_LCDKIT_N5V5_ENABLE_NAME, &gpio_lcdkit_vsn, 0
    },
};

static struct gpio_desc lcdkit_bias_enable_cmds[] =
{
    /* AVDD_5.5V */
    {
        DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
        GPIO_LCDKIT_P5V5_ENABLE_NAME, &gpio_lcdkit_vsp, 1
    },
    /* AVEE_-5.5V */
    {
        DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
        GPIO_LCDKIT_N5V5_ENABLE_NAME, &gpio_lcdkit_vsn, 1
    },
};

static struct gpio_desc lcdkit_bl_enable_cmds[] =
{
    /* backlight enable */
    {
        DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
        GPIO_LCDKIT_BL_ENABLE_NAME, &gpio_lcdkit_bl, 1
    },

};

static struct gpio_desc lcdkit_bl_power_enable_cmds[] =
{
    /* backlight power enable */
    {
        DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
        GPIO_LCDKIT_BL_POWER_ENABLE_NAME, &gpio_lcdkit_bl_power, 1
    }
};
static struct gpio_desc lcdkit_bias_free_cmds[] =
{
    /* AVEE_-5.5V */
    {
        DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
        GPIO_LCDKIT_N5V5_ENABLE_NAME, &gpio_lcdkit_vsn, 0
    },
    /* AVDD_5.5V */
    {
        DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
        GPIO_LCDKIT_P5V5_ENABLE_NAME, &gpio_lcdkit_vsp, 0
    },
};

static struct gpio_desc lcdkit_bias_disable_cmds[] =
{
    /* AVEE_-5.5V */
    {
        DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
        GPIO_LCDKIT_N5V5_ENABLE_NAME, &gpio_lcdkit_vsn, 0
    },

    /* AVDD_5.5V */
    {
        DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
        GPIO_LCDKIT_P5V5_ENABLE_NAME, &gpio_lcdkit_vsp, 0
    },

    /* AVEE_-5.5V input */
    {
        DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 5,
        GPIO_LCDKIT_N5V5_ENABLE_NAME, &gpio_lcdkit_vsn, 0
    },
    /* AVDD_5.5V input */
    {
        DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 5,
        GPIO_LCDKIT_P5V5_ENABLE_NAME, &gpio_lcdkit_vsp, 0
    },
};

static struct gpio_desc lcdkit_bl_repuest_cmds[] =
{
    /*BL request*/
    {
        DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
        GPIO_LCDKIT_BL_ENABLE_NAME, &gpio_lcdkit_bl, 0
    },
};

static struct gpio_desc lcdkit_bl_power_request_cmds[] =
{
    /*BL power request*/
    {
        DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
        GPIO_LCDKIT_BL_POWER_ENABLE_NAME, &gpio_lcdkit_bl_power, 0
    }
};
static struct gpio_desc lcdkit_bl_disable_cmds[] =
{
    /* backlight disable */
    {
        DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
        GPIO_LCDKIT_BL_ENABLE_NAME, &gpio_lcdkit_bl, 0
    },

    {
        DTYPE_GPIO_INPUT, WAIT_TYPE_US, 100,
        GPIO_LCDKIT_BL_ENABLE_NAME, &gpio_lcdkit_bl, 0
    },

};

static struct gpio_desc lcdkit_bl_power_disable_cmds[] =
{
    /* backlight power disable */
    {
        DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
        GPIO_LCDKIT_BL_POWER_ENABLE_NAME, &gpio_lcdkit_bl_power, 0
    },

    {
        DTYPE_GPIO_INPUT, WAIT_TYPE_US, 100,
        GPIO_LCDKIT_BL_POWER_ENABLE_NAME, &gpio_lcdkit_bl_power, 0
    }
};
static struct gpio_desc lcdkit_bl_free_cmds[] =
{
    /* BL free*/
    {
        DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
        GPIO_LCDKIT_BL_ENABLE_NAME, &gpio_lcdkit_bl, 0
    },
};

static struct gpio_desc lcdkit_bl_power_free_cmds[] =
{
    /* BL free*/
    {
        DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
        GPIO_LCDKIT_BL_POWER_ENABLE_NAME, &gpio_lcdkit_bl_power, 0
    }
};
/***************************
*Iovcc and vci gpio
*/
static struct gpio_desc lcdkit_iovcc_request_cmds[] =
{
    {
        DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
        GPIO_LCDKIT_IOVCC_NAME, &gpio_lcdkit_iovcc, 0
    },
};

static struct gpio_desc lcdkit_vbat_request_cmds[] =
{
    {
        DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
        GPIO_LCDKIT_VBAT_NAME, &gpio_lcdkit_vbat, 0
    },
};

static struct gpio_desc lcdkit_vci_request_cmds[] =
{
    {
        DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
        GPIO_LCDKIT_VCI_NAME, &gpio_lcdkit_vci, 0
    },
};

static struct gpio_desc lcdkit_iovcc_enable_cmds[] =
{
    {
        DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
        GPIO_LCDKIT_IOVCC_NAME, &gpio_lcdkit_iovcc, 1
    },
};

static struct gpio_desc lcdkit_vbat_enable_cmds[] =
{
    {
        DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
        GPIO_LCDKIT_VBAT_NAME, &gpio_lcdkit_vbat, 1
    },
};

static struct gpio_desc lcdkit_vci_enable_cmds[] =
{
    {
        DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
        GPIO_LCDKIT_VCI_NAME, &gpio_lcdkit_vci, 1
    },
};

static struct gpio_desc lcdkit_iovcc_free_cmds[] =
{
    {
        DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
        GPIO_LCDKIT_IOVCC_NAME, &gpio_lcdkit_iovcc, 0
    },
};

static struct gpio_desc lcdkit_vbat_free_cmds[] =
{
    {
        DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
        GPIO_LCDKIT_VBAT_NAME, &gpio_lcdkit_vbat, 0
    },
};

static struct gpio_desc lcdkit_vci_free_cmds[] =
{
    {
        DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
        GPIO_LCDKIT_VCI_NAME, &gpio_lcdkit_vci, 0
    },
};

static struct gpio_desc lcdkit_iovcc_disable_cmds[] =
{
    {
        DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
        GPIO_LCDKIT_IOVCC_NAME, &gpio_lcdkit_iovcc, 0
    },

    {
        DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 5,
        GPIO_LCDKIT_IOVCC_NAME, &gpio_lcdkit_iovcc, 0
    },
};

static struct gpio_desc lcdkit_vbat_disable_cmds[] =
{
    {
        DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
        GPIO_LCDKIT_VBAT_NAME, &gpio_lcdkit_vbat, 0
    },

    {
        DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 5,
        GPIO_LCDKIT_VBAT_NAME, &gpio_lcdkit_vbat, 0
    },
};

static struct gpio_desc lcdkit_vci_disable_cmds[] =
{
    {
        DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
        GPIO_LCDKIT_VCI_NAME, &gpio_lcdkit_vci, 0
    },

    {
        DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 5,
        GPIO_LCDKIT_VCI_NAME, &gpio_lcdkit_vci, 0
    },
};

/*******************************************************************************
** LCD IOMUX
*/
static struct gpio_desc lcdkit_pinctrl_normal_cmds[] =
{
    /* reset */
    {
        DTYPE_GPIO_PMX, WAIT_TYPE_MS, 0,
        GPIO_LCDKIT_RESET_NAME, &gpio_lcdkit_reset, FUNCTION0
    },

    /* te0 */
    {
        DTYPE_GPIO_PMX, WAIT_TYPE_MS, 0,
        GPIO_LCDKIT_TE0_NAME, &gpio_lcdkit_te0, FUNCTION2
    },
};

static struct gpio_desc lcdkit_bias_pinctrl_normal_cmds[] =
{
    /* vsp */
    {
        DTYPE_GPIO_PMX, WAIT_TYPE_MS, 0,
        GPIO_LCDKIT_P5V5_ENABLE_NAME, &gpio_lcdkit_vsp, FUNCTION0
    },
    /* vsn */
    {
        DTYPE_GPIO_PMX, WAIT_TYPE_MS, 0,
        GPIO_LCDKIT_N5V5_ENABLE_NAME, &gpio_lcdkit_vsn, FUNCTION0
    },
    /* bl */
    {
        DTYPE_GPIO_PMX, WAIT_TYPE_MS, 0,
        GPIO_LCDKIT_BL_ENABLE_NAME, &gpio_lcdkit_bl, FUNCTION0
    },
};

static struct gpio_desc lcdkit_pinctrl_low_cmds[] =
{
    /* reset */
    {
        DTYPE_GPIO_PMX, WAIT_TYPE_MS, 0,
        GPIO_LCDKIT_RESET_NAME, &gpio_lcdkit_reset, FUNCTION0
    },
    /* te0 */
    {
        DTYPE_GPIO_PMX, WAIT_TYPE_MS, 0,
        GPIO_LCDKIT_TE0_NAME, &gpio_lcdkit_te0, FUNCTION0
    },
    /* te0 input */
    {
        DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 0,
        GPIO_LCDKIT_TE0_NAME, &gpio_lcdkit_te0, 0
    },

};

static struct gpio_desc lcdkit_bias_pinctrl_low_cmds[] =
{
    /* vsp input */
    {
        DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 0,
        GPIO_LCDKIT_P5V5_ENABLE_NAME, &gpio_lcdkit_vsp, 0
    },
    /* vsn input */
    {
        DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 0,
        GPIO_LCDKIT_N5V5_ENABLE_NAME, &gpio_lcdkit_vsn, 0
    },
};

static struct gpio_desc lcdkit_bl_pinctrl_low_cmds[] =
{
    /* bl input */
    {
        DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 0,
        GPIO_LCDKIT_BL_ENABLE_NAME, &gpio_lcdkit_bl, 0
    },
};
/*******************************************************************************
**variable define
*/
/*panel data*/
extern struct lcdkit_panel_data lcdkit_data;
/*panel info*/
extern struct lcdkit_disp_info lcdkit_infos;

/********************************************************************
*extern
********************************************************************/
extern int lcdkit_get_lcd_id(void);

/********************************************************************
*struct
********************************************************************/
/*panel structure*/
struct lcdkit_panel_info
{
    uint32_t xres;
    uint32_t yres;
    uint32_t width;
    uint32_t height;
    uint32_t orientation;
    uint32_t bpp;
    uint32_t bgr_fmt;
    uint32_t bl_set_type;
    uint32_t bl_min;
    uint32_t bl_max;
    uint32_t bl_v200;
    uint32_t bl_otm;
    uint32_t blpwm_intr_value;
    uint32_t blpwm_max_value;
    uint32_t type;
    char* compatible;
    uint32_t ifbc_type;
    char* lcd_name;
    uint8_t frc_enable;
    uint8_t esd_enable;
    uint8_t prefix_ce_support;
    uint8_t prefix_sharpness_support;
    uint8_t sbl_support;
    uint8_t acm_support;
    uint8_t acm_ce_support;
    uint8_t gamma_support;
    /*for dynamic gamma*/
    uint8_t dynamic_gamma_support;
    uint64_t pxl_clk_rate;
    uint32_t pxl_clk_rate_div;
    uint8_t dirty_region_updt_support;
    uint8_t dsi_bit_clk_upt_support;
    uint32_t vsync_ctrl_type;
    uint32_t blpwm_precision_type;
    uint8_t lcd_uninit_step_support;
    uint8_t dsi1_snd_cmd_panel_support;
    uint8_t dpi01_set_change;
    uint32_t blpwm_precision;
    uint32_t blpwm_div;
};

struct lcdkit_panel_mipi
{
    uint8_t vc;
    uint8_t lane_nums;
    uint8_t color_mode;
    uint32_t dsi_bit_clk;
    uint32_t burst_mode;
    uint32_t max_tx_esc_clk;
    uint32_t dsi_bit_clk_val1;
    uint32_t dsi_bit_clk_val2;
    uint32_t dsi_bit_clk_val3;
    uint32_t dsi_bit_clk_val4;
    uint32_t dsi_bit_clk_val5;
    uint32_t dsi_bit_clk_upt;
    uint8_t non_continue_en;
    uint8_t hs_clk_disable_delay;
    uint32_t clk_post_adjust;
    uint32_t clk_pre_adjust;
    uint32_t clk_ths_prepare_adjust;
    uint32_t clk_tlpx_adjust;
    uint32_t clk_ths_trail_adjust;
    uint32_t clk_ths_exit_adjust;
    uint32_t clk_ths_zero_adjust;
    uint32_t data_t_hs_trial_adjust;
    int data_t_lpx_adjust;
    uint32_t rg_vrefsel_vcm_adjust;
    uint32_t phy_mode;  //0: DPHY, 1:CPHY
    uint32_t lp11_flag; /* 0: nomal_lp11, 1:short_lp11, 2:disable_lp11 */
    uint32_t hs_wr_to_time;
    uint32_t phy_m_n_count_update;  // 0:old ,1:new can get 988.8M
};

struct lcdkit_panel_ldi
{
    uint32_t h_back_porch;
    uint32_t h_front_porch;
    uint32_t h_pulse_width;
    uint32_t v_back_porch;
    uint32_t v_front_porch;
    uint32_t v_pulse_width;
    uint8_t hsync_plr;
    uint8_t vsync_plr;
    uint8_t pixelclk_plr;
    uint8_t data_en_plr;
    uint8_t dpi0_overlap_size;
    uint8_t dpi1_overlap_size;
};

struct lcdkit_platform_config
{
    /*reset gpio*/
    uint32_t gpio_lcd_reset;
    /*te gpio*/
    uint32_t gpio_lcd_te;
    /*iovcc gpio*/
    uint32_t gpio_lcd_iovcc;
    /*vci gpio*/
    uint32_t gpio_lcd_vci;
    /*vci gpio*/
    uint32_t gpio_tp_rst;
    /*vsp gpio*/
    uint32_t gpio_lcd_vsp;
    /*vsn gpio*/
    uint32_t gpio_lcd_vsn;
    /*bl gpio*/
    uint32_t gpio_lcd_bl;
    uint32_t gpio_lcd_bl_power;
    /*vbat gpio*/
    uint32_t gpio_lcd_vbat;
    /*LcdanalogVcc*/
    uint32_t lcdanalog_vcc;
    /*LcdioVcc*/
    uint32_t lcdio_vcc;
    /*LcdBias*/
    uint32_t lcd_bias;
    /*LcdVsp*/
    uint32_t lcd_vsp;
    /*LcdVsn*/
    uint32_t lcd_vsn;
};

struct lcdkit_misc_info
{
    /*orise ic is need reset after iovcc power on*/
    uint8_t first_reset;
    uint8_t second_reset;
    uint8_t reset_pull_high_flag;
    /*incell panel, lcd control tp*/
    uint8_t lcd_type;
    /*is default lcd*/
    uint8_t tp_color_support;
    uint8_t id_pin_read_support;
    uint8_t lcd_panel_type;
    uint8_t bias_power_ctrl_mode;
    uint8_t iovcc_power_ctrl_mode;
    uint8_t vci_power_ctrl_mode;
    uint8_t vbat_power_ctrl_mode;
    uint8_t bl_power_ctrl_mode;
    uint8_t display_on_in_backlight;
    /* display on new seq and reset time for synaptics only */
    uint8_t panel_display_on_new_seq;
    uint8_t display_effect_on_support;
    uint8_t lcd_brightness_color_uniform_support;
    /* for Hostprocessing TP/LCD OEM infor only */
    uint8_t host_tp_oem_support;
    uint8_t host_panel_oem_pagea_support;
    uint8_t host_panel_oem_pageb_support;
    uint8_t host_panel_oem_backtouser_support;
    uint8_t host_panel_oem_readpart1_support;
    uint8_t host_panel_oem_readpart2_support;
    uint8_t host_panel_oem_readpart1_len;
    uint8_t host_panel_oem_readpart2_len;
    uint8_t reset_shutdown_later;
    uint8_t dis_on_cmds_delay_margin_support;
    uint8_t dis_on_cmds_delay_margin_time;
    uint8_t rgbw_support;
    uint8_t use_gpio_lcd_bl;
    uint8_t use_gpio_lcd_bl_power;
    uint8_t bias_change_lm36274_from_panel_support;
    uint8_t init_lm36923_after_panel_power_on_support;
    uint8_t lcd_otp_support;
};

struct lcdkit_delay_ctrl
{
    /*power on delay ctrl*/
    uint32_t delay_af_vci_on;
    uint32_t delay_af_iovcc_on;
    uint32_t delay_af_vbat_on;
    uint32_t delay_af_bias_on;
    uint32_t delay_af_vsp_on;
    uint32_t delay_af_vsn_on;
    uint32_t delay_af_LP11;
    uint8_t reset_step1_H;
    uint8_t reset_L;
    uint8_t reset_step2_H;
    /*power off delay ctrl*/
    uint32_t delay_af_vsn_off;
    uint32_t delay_af_vsp_off;
    uint32_t delay_af_bias_off;
    uint32_t delay_af_vbat_off;
    uint32_t delay_af_iovcc_off;
    uint32_t delay_af_vci_off;
    uint32_t delay_af_first_iovcc_off;
    uint32_t delay_af_display_on;
    uint32_t delay_af_display_off;
    uint32_t delay_af_display_off_second;
};

struct lcdkit_panel_data
{
    struct lcdkit_panel_info* panel;
    struct lcdkit_panel_ldi* ldi;
    struct lcdkit_panel_mipi* mipi;
};

/*dsi command structure*/
struct lcdkit_dsi_panel_cmd
{
    struct dsi_cmd_desc* cmds_set;
    uint32_t cmd_cnt;
};

struct lcdkit_disp_info
{
    /*panel on dsi command*/
    struct lcdkit_dsi_panel_cmd display_on_cmds;
    struct lcdkit_dsi_panel_cmd display_on_second_cmds;
    struct lcdkit_dsi_panel_cmd display_on_in_backlight_cmds;
    struct lcdkit_dsi_panel_cmd display_on_effect_cmds;
    /*panel off dsi command*/
    struct lcdkit_dsi_panel_cmd display_off_cmds;
    struct lcdkit_dsi_panel_cmd display_off_second_cmds;
    struct lcdkit_platform_config* lcd_platform;
    struct lcdkit_misc_info* lcd_misc;
    struct lcd_bias_ic  lcd_bias_ic_info;
    struct lcd_backlight_ic lcd_backlight_ic_info;
    struct lcdkit_delay_ctrl* lcd_delay; 
    struct lcdkit_dsi_panel_cmd tp_color_cmds;
    struct lcdkit_dsi_panel_cmd lcd_oemprotectoffpagea;
    struct lcdkit_dsi_panel_cmd lcd_oemreadfirstpart;
    struct lcdkit_dsi_panel_cmd lcd_oemprotectoffpageb;
    struct lcdkit_dsi_panel_cmd lcd_oemreadsecondpart;
    struct lcdkit_dsi_panel_cmd lcd_oembacktouser;
    struct lcdkit_dsi_panel_cmd id_pin_read_cmds;
	/* color consistency support*/
    struct lcdkit_dsi_panel_cmd color_coordinate_enter_cmds;
    struct lcdkit_dsi_panel_cmd color_coordinate_cmds;
    struct lcdkit_dsi_panel_cmd color_coordinate_exit_cmds;
    /*panel info consistency support*/
    struct lcdkit_dsi_panel_cmd panel_info_consistency_enter_cmds;
    struct lcdkit_dsi_panel_cmd panel_info_consistency_cmds;
    struct lcdkit_dsi_panel_cmd panel_info_consistency_exit_cmds;
    /*set backlight cmd reg 51h or 61h */
    struct lcdkit_dsi_panel_cmd backlight_cmds;
};
#endif
