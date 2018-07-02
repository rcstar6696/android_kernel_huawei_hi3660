#ifndef _LCDKIT_DISP_H_
#define _LCDKIT_DISP_H_

#define CABC_PWM_MAX                0xff
#define BL_CMD_LEVEL_INDEX_OFFSET 3  /*bl level index offset in bl cmd buffer from end*/
#define BL_LEVEL_11BITS_MULTI 3   /*backlight level multiple for 11bits than default 8bits*/
#define GPIO_STATE_LOW              0
#define GPIO_STATE_HIGH             2
#define RESET_GPIO_SEQ_LEN          3

#define PMIC_WLED_SLAVE_ID          3

#define LCD_ID_0_GPIO               59
#define LCD_ID_1_GPIO               66

#define LCD_ID_PULL_DOWN	        0
#define LCD_ID_PULL_UP	            1

#define LCM_ID_POWER_GPIO_BAH       33	/* Bach uses GPIO33 to pull up LCM ID pin power. */
#define LCM_ID_POWER_GPIO_AGS       8	/* Agassi uses GPIO8 to pull up LCM ID pin power. */

#define LCM_ID_POWER_DELAY          20	/* Needs 20ms to wait power stable. */

#define BIT(bit)                    (1 << (bit))

#define  PANEL_TYPE_LCD             BIT(0)
#define  PANEL_TYPE_OLED            BIT(1)

#define  POWER_CTRL_BY_REGULATOR    BIT(0)
#define  POWER_CTRL_BY_GPIO         BIT(1)
#define  POWER_CTRL_BY_IC_COMMON    BIT(2)
#define update_value(newvalue, oldvalue) (newvalue ? newvalue : oldvalue)

#define lcdkit_delay(delayvalue, defaultvalue)    \
    do {   \
        if (delayvalue) \
        {   \
            mdelay(delayvalue); \
        }   \
        else    \
        {   \
            if (defaultvalue)   \
                mdelay(defaultvalue);   \
        }   \
    } while (0)

#define lcdkit_gpio_cfg(targpio, func, dir)    \
    do {   \
        gpio_tlmm_config(targpio.pin_id, func, targpio.pin_direction,   \
            targpio.pin_pull, targpio.pin_strength, targpio.pin_state);     \
        gpio_set_dir(targpio.pin_id, dir);  \
    } while (0)

enum TP_ic_type
{
    ONCELL = 0,     //lcd & tp have separate regulator
    HYBRID,		    //lcd &tp share 1.8V
    TDDI,	        //lcd ctrl all the regulator
    TP_IC_TYPE_MAX = 255,
};

enum {
    BL_MODULE_LP8556 = 8556,
    BL_MODULE_LM36923 = 36923,
};

struct lcdkit_platform_config
{
    uint32_t wled_slaveid;
    uint32_t use_bl_gpio;
    uint32_t use_mode_gpio;
    uint32_t use_bl_en_gpio;

    uint32_t on_ldomap;
    uint32_t off_ldomap;
    uint32_t vci_ldomap;
    uint32_t iovcc_ldomap;

    uint32_t gpio_tp_reset;
    uint32_t gpio_enable;
    uint32_t gpio_mode;
    uint32_t gpio_reset;
    uint32_t gpio_te;
    uint32_t gpio_iovcc;
    uint32_t gpio_vci;
    uint32_t gpio_vsp;
    uint32_t gpio_vsn;
    uint32_t gpio_bl;
    uint32_t gpio_bl_en;

    unsigned int lcd_analog_vcc;
    unsigned int lcd_iovcc;
    unsigned int lcd_bias;
    unsigned int lcd_vsp;
    unsigned int lcd_vsn;
    unsigned int bl_default_level;  //normal boot bl level
    unsigned int bl_low_power_default_level; //low power boot bl level
    unsigned int bl_chip_init;//use bl chip init
    unsigned int bl_chip_use_i2c;
    unsigned int lcd_read_tp_color;	// Whether to support reading tp color by LCD.
    unsigned int panel_ssc_enable; //use for lk ssc enable
    unsigned int panel_lk_power_on_timing_control; //use for modify the bach boe screen sequence parameter
    unsigned int lp8556_bl_channel_config;
    unsigned int lp8556_bl_max_vboost_select;
};

struct lcdkit_misc_info
{
    /*otm1906c ic is need reset after iovcc power on*/
    uint8_t first_reset;
    uint8_t second_reset;

    uint8_t tp_ic_type;

    uint8_t panel_type;
    uint8_t bias_ctrl_mode;
    uint8_t iovcc_ctrl_mode;
    uint8_t vci_ctrl_mode;

    uint8_t tx_eot_append;
    uint8_t rx_eot_ignore;

    uint8_t mipi_regulator_mode;
};

struct lcdkit_delay_ctrl
{
    uint32_t delay_af_vci_on;
    uint32_t delay_af_iovcc_on;
    uint32_t delay_af_bias_on;
    uint32_t delay_af_vsp_on;
    uint32_t delay_af_vsn_on;
    //uint32_t delay_af_lp11;

    uint32_t delay_af_vsn_off;
    uint32_t delay_af_vsp_off;
    uint32_t delay_af_bias_off;
    uint32_t delay_af_iovcc_off;
    uint32_t delay_af_vci_off;

    uint32_t delay_bf_bl;
    uint32_t delay_af_rst_off;
    uint32_t delay_af_blic_init;
    uint32_t delay_af_panel_lk_power_on;
};

struct lcdkit_cfg
{
    struct lcdkit_platform_config *lcd_platform;
    struct lcdkit_misc_info *lcd_misc;
    struct lcdkit_delay_ctrl *lcd_delay;
    struct mipi_dsi_cmd *backlight_cmds;
    int num_of_backlight_cmds;
};

extern struct lcdkit_cfg lcdkit_config;
extern uint32_t panel_regulator_settings[7];
int hw_get_lcd_id(uint32_t hw_id);
uint8_t lcdkit_get_slave_id();
int wled_backlight_ctrl(uint8_t enable);
int wled_init(struct msm_panel_info *pinfo);
uint32_t mdss_dsi_panel_reset(uint8_t enable);

#endif

