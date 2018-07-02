
#include <debug.h>
#include <string.h>
#include <smem.h>
#include <err.h>
#include <reg.h>
#include <stdlib.h>
#include <msm_panel.h>
#include <mipi_dsi.h>
#include <pm8x41.h>
#include <pm8x41_wled.h>
#include <pm8x41_hw.h>
#include <qpnp_wled.h>
#include <board.h>
#include <mdp5.h>
#include <scm.h>
#include <regulator.h>
#include <platform/clock.h>
#include <platform/gpio.h>
#include <platform/iomap.h>
#include <target/display.h>
#include <mipi_dsi_autopll_thulium.h>
#include <qtimer.h>
#include <platform.h>
#include "include/panel.h"
#include "include/display_resource.h"
#include "gcdb_display.h"
#include "panel_display.h"
#include <dev/charger.h>

#include "lcdkit.h"
#include "lcdkit_disp.h"
#include "lcdkit_panels.h"
#include "lcdkit_bias_ic_common.h"
#define LCD_BIAS_DELAY_TIME 2
/*---------------------------------------------------------------------------*/
/* Global varible                                                       */
/*---------------------------------------------------------------------------*/
struct lcdkit_cfg lcdkit_config;
extern struct panel_struct panelstruct;
static struct gpio_pin bkl_gpio = {"msmgpio", 91, 3, 1, 0, 1};
static struct gpio_pin disp_bl_gpio = {"msmgpio", 93, 3, 1, 0, 1};
static struct gpio_pin reset_gpio = {"msmgpio", 0, 3, 1, 0, 1};
static struct gpio_pin enable_gpio = {"msmgpio", 90, 3, 1, 0, 1};
static struct gpio_pin tp_reset_gpio = {"msmgpio", 107, 3, 1, 0, 1};
static struct gpio_pin lcd_mode_gpio = {"msmgpio", 107, 3, 1, 0, 1};
static struct gpio_pin lcd_vsp_gpio = {"msmgpio", 93, 3, 1, 0, 1};
static struct gpio_pin lcd_vsn_gpio = {"msmgpio", 94, 3, 1, 0, 1};
static struct gpio_pin lcm_id_power_gpio = {"msmgpio", 38, 3, 1, 0, 1};
uint32_t panel_regulator_settings[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#if 0
struct lcdkit_platform_config lcd_platform;
struct lcdkit_misc_info lcd_misc;
struct lcdkit_delay_ctrl lcd_delay;
void lcdkit_var_init()
{
    lcdkit_config.lcd_misc = &lcd_misc;
    lcdkit_config.lcd_delay = &lcd_delay;
    lcdkit_config.lcd_platform = &lcd_platform;


    lcd_misc.power_ctrl_mode = 0x1;
    lcd_misc.lcd_ctrl_tp_power = 0;
    lcd_misc.first_reset = 0;
    lcd_misc.second_reset = 0;

    lcd_delay.delay_be_iovcc_on = 0;
    lcd_delay.delay_af_iovcc_on = 0;
    lcd_delay.delay_af_vsp_on = 0;

    lcd_platform.lcd_iovcc_ldomap = 0x20;
    lcd_platform.lcd_on_ldomap = 0x10002;
    lcd_platform.lcd_off_ldomap = 0x10000;

    lcd_platform.gpio_lcd_enable = 99;

    lcd_platform.use_bl_gpio = 1;
    lcd_platform.gpio_lcd_bl = 98;

    lcd_platform.gpio_lcd_reset = 60;

    lcd_misc.lcd_ctrl_tp_power = 0;
    lcd_platform.gpio_tp_reset = 0;

    lcd_platform.use_mode_gpio = 0;
    lcd_platform.gpio_lcd_mode = 0;
}

char* get_panel_node_id(void)
{
	return panelstruct.paneldata->panel_node_id;
}
#endif
static inline int lcdkit_bias_is_ic_ctrl_power(void)
{
	dprintf(CRITICAL, "lcd_misc->bias_ctrl_mode=%d\n",lcdkit_config.lcd_misc->bias_ctrl_mode);
    return (lcdkit_config.lcd_misc->bias_ctrl_mode & POWER_CTRL_BY_IC_COMMON) ? true : false;
}
bool lcdkit_is_default_panel(void)
{
	return (strcmp(panelstruct.paneldata->panel_compatible,
        "auo_otm1901a_5p2_1080p_video_default")) ? false: true;
}

bool lcdkit_support_read_tp_color(void)
{
	return lcdkit_config.lcd_platform->lcd_read_tp_color;
}

uint8_t lcdkit_get_slave_id()
{
    return update_value(lcdkit_config.lcd_platform->wled_slaveid, PMIC_WLED_SLAVE_ID);
}

void lcdkit_init()
{
    //lcdkit_var_init();

    enable_gpio.pin_id
        = update_value(lcdkit_config.lcd_platform->gpio_enable, enable_gpio.pin_id);

    bkl_gpio.pin_id
        = update_value(lcdkit_config.lcd_platform->gpio_bl, bkl_gpio.pin_id);

    disp_bl_gpio.pin_id
        = update_value(lcdkit_config.lcd_platform->gpio_bl_en, disp_bl_gpio.pin_id);

    reset_gpio.pin_id
        = update_value(lcdkit_config.lcd_platform->gpio_reset, reset_gpio.pin_id);

    tp_reset_gpio.pin_id
        = update_value(lcdkit_config.lcd_platform->gpio_tp_reset, tp_reset_gpio.pin_id);

    lcd_mode_gpio.pin_id
        = update_value(lcdkit_config.lcd_platform->gpio_mode, lcd_mode_gpio.pin_id);

    return;
}

int wled_backlight_ctrl(uint8_t enable)
{
	pm8x41_wled_config_slave_id(lcdkit_get_slave_id());

	if(lcdkit_is_default_panel())
	{
        dprintf(CRITICAL, "%s can't distinguish expect LCD module type,"
                "panel_node_id = %s\n",
                __func__, panelstruct.paneldata->panel_node_id);

		qpnp_wled_enable_backlight(0);
	}
    else
    {
    	qpnp_wled_enable_backlight(enable);
        qpnp_ibb_enable(enable);
	}

    return NO_ERROR;
}

extern int lp8556_chip_init_and_set_bl(uint8_t bl_i2c_index, int level);
extern int lm36923_chip_init_and_set_bl(uint8_t bl_i2c_index, int level);
extern struct mipi_panel_info get_panel_mipi(void);
int lcdkit_set_bl_level(int level)
{
	int ret=NO_ERROR;
	struct mipi_panel_info mipi = get_panel_mipi();

	if(0>=level || level > CABC_PWM_MAX)
	{
		level = CABC_PWM_MAX;
	}
	if(lcdkit_config.backlight_cmds && lcdkit_config.backlight_cmds[0].size>=BL_CMD_LEVEL_INDEX_OFFSET){
		/*compatible for long and short package*/
		lcdkit_config.backlight_cmds[0].payload[lcdkit_config.backlight_cmds[0].size-BL_CMD_LEVEL_INDEX_OFFSET]=level;
	}
	ret = mdss_dsi_write_cmds(&mipi, lcdkit_config.backlight_cmds, lcdkit_config.num_of_backlight_cmds, 0);
	return ret;
}

int lcdkit_bklt_dcs()
{
	uint32_t ret = NO_ERROR;

	dprintf(INFO, "DCS_backlight_ctrl\n");
	if(is_battery_weak(VBAT_WO_CHG))
		ret = lcdkit_set_bl_level(lcdkit_config.lcd_platform->bl_low_power_default_level);
	else
		ret = lcdkit_set_bl_level(lcdkit_config.lcd_platform->bl_default_level);

	return ret;
}

int lcdkit_bklt_IC_TI()
{
	uint32_t ret = NO_ERROR;
	int bl_level = 0;
	dprintf(INFO, "DCS_backlight_ctrl\n");

	if(is_battery_weak(VBAT_WO_CHG))
		bl_level = lcdkit_config.lcd_platform->bl_low_power_default_level;
	else
		bl_level = lcdkit_config.lcd_platform->bl_default_level;

	if (lcdkit_config.lcd_platform->bl_chip_init == BL_MODULE_LP8556){
		lcdkit_delay(lcdkit_config.lcd_delay->delay_af_blic_init, 0);
		ret = lp8556_chip_init_and_set_bl(lcdkit_config.lcd_platform->bl_chip_use_i2c, bl_level);
	}
	else if (lcdkit_config.lcd_platform->bl_chip_init == BL_MODULE_LM36923){
		int level=bl_level<<BL_LEVEL_11BITS_MULTI;
		ret = lm36923_chip_init_and_set_bl(lcdkit_config.lcd_platform->bl_chip_use_i2c, level);
	}
	dprintf(INFO,"%s chip init and set bl ret=0x%x\n", __func__, ret);

	ret = lcdkit_set_bl_level(CABC_PWM_MAX);

	return ret;
}

int target_backlight_ctrl(struct backlight *bl, uint8_t enable)
{
	uint32_t ret = NO_ERROR;

	if (bl->bl_interface_type == BL_DCS)
	{
		ret = lcdkit_bklt_dcs();
	}
	else if (bl->bl_interface_type == BL_IC_TI)
	{
		ret = lcdkit_bklt_IC_TI();
	}
	else
	{
	ret = wled_backlight_ctrl(enable);
	}

	return ret;
}

void oem_panel_id_power_ctrl(uint32_t hw_id, bool enable)
{
#if 0
	if ((hw_id >= HW_PLATFORM_BACH_AL00_VA) && (hw_id <= HW_PLATFORM_BACH_L09_VA))
	{
		lcm_id_power_gpio.pin_id = LCM_ID_POWER_GPIO_BAH;
	}
	else if (((hw_id >= HW_PLATFORM_AGS_W09HN_VA) && (hw_id <= HW_PLATFORM_AGS_L09_VA)) || (hw_id == HW_PLATFORM_AGS_VDF_VA))
	{
		lcm_id_power_gpio.pin_id = LCM_ID_POWER_GPIO_AGS;
	}
	else
#endif
		return;

	dprintf(INFO,"%s, hw_id=%d, lcm_id_power_gpio.pin_id=%d, enable=%d\n",
		__func__, hw_id, lcm_id_power_gpio.pin_id, enable);
	if (enable)
	{
		lcdkit_gpio_cfg(lcm_id_power_gpio, 0, GPIO_STATE_HIGH);
		mdelay(LCM_ID_POWER_DELAY);
	}
	else
	{
		gpio_set_dir(lcm_id_power_gpio.pin_id, GPIO_STATE_LOW);
	}
}

/****************************************************************
function: get lcd id by gpio

*data structure*
*	   ID1	  ID0   *
 *	----------------- *
 *	|   |   |   |   | *
 *	|   |   |   |   | *
 *	----------------- *
 For each Gpio :
		00 means low  ,
		01 means high ,
		10 means float,
		11 is not defined,

 lcd id(hex):
 0	:ID0 low,	ID1 low
 1	:ID0 high,	ID1 low
 2	:ID0 float,	ID1 low

 4	:ID0 low,	ID1 high
 5	:ID0 high,	ID1 high
 6	:ID0 float,	ID1 high

 8	:ID0 low,	ID1 float
 9	:ID0 high,	ID1 float
 A	:ID0 float,	ID1 float, used for emulator
 ***************************************************************/
int hw_get_lcd_id(uint32_t hw_id)
{
	int id0,id1;
	int gpio_id0,gpio_id1;
	int pullup_read,pulldown_read;
	id0=0;
	id1=0;
	pullup_read = 0;
	pulldown_read = 0;

	oem_panel_id_power_ctrl(hw_id, true);

    lcdkit_get_id_gpio(hw_id, &gpio_id0, &gpio_id1);

    gpio_id0 = update_value(gpio_id0, LCD_ID_0_GPIO);
    gpio_id1 = update_value(gpio_id1, LCD_ID_1_GPIO);

	/*Enable LDO6 to avoid the recognition mistake of lcd's ID pin caused by fuse feature.*/
	uint32_t ldo_num = REG_LDO6;
	regulator_enable(ldo_num);
	mdelay(10);

	/*config id0 to pull down and read*/
	gpio_tlmm_config(gpio_id0,0,GPIO_INPUT,GPIO_PULL_DOWN,GPIO_2MA,GPIO_ENABLE);

    //necessary for a delay, else following hw_gpio_input always get 0
    udelay(10);

    pulldown_read = gpio_status(gpio_id0);

	/*config id0 to pull up and read*/
	gpio_tlmm_config(gpio_id0,0,GPIO_INPUT,GPIO_PULL_UP,GPIO_2MA,GPIO_ENABLE);
	udelay(10);

	pullup_read = gpio_status(gpio_id0);
	//float
	if(pulldown_read != pullup_read)
	{
		id0 = BIT(1);
	}
	//connect
	else
	{
		//pullup_read==pulldown_read
		id0 = pullup_read;
		switch(id0)
		{
			case LCD_ID_PULL_DOWN:
				gpio_tlmm_config(gpio_id0,0,GPIO_INPUT,
                                GPIO_PULL_DOWN,GPIO_2MA,GPIO_ENABLE);
				break;
			case LCD_ID_PULL_UP:
				gpio_tlmm_config(gpio_id0,0,GPIO_INPUT,
                                GPIO_PULL_UP,GPIO_2MA,GPIO_ENABLE);
				break;
			default:
				gpio_tlmm_config(gpio_id0,0,GPIO_INPUT,
                                GPIO_NO_PULL,GPIO_2MA,GPIO_ENABLE);
				break;
		}

	}
	/*config id1 to pull down and read*/
	gpio_tlmm_config(gpio_id1,0,GPIO_INPUT,GPIO_PULL_DOWN,GPIO_2MA,GPIO_ENABLE);
	udelay(10);
	pulldown_read = gpio_status(gpio_id1);

	/*config id1 to pull up and read*/
	gpio_tlmm_config(gpio_id1,0,GPIO_INPUT,GPIO_PULL_UP,GPIO_2MA,GPIO_ENABLE);
	udelay(10);
	pullup_read = gpio_status(gpio_id1);

	//float
	if(pulldown_read != pullup_read)
	{
		id1 = BIT(1);
	}
    //connect
	else
	{
		//pullup_read==pulldown_read
		id1 = pullup_read;
		switch(id1)
		{
			case LCD_ID_PULL_DOWN:
				gpio_tlmm_config(gpio_id1,0,GPIO_INPUT,
                                GPIO_PULL_DOWN,GPIO_2MA,GPIO_ENABLE);
				break;
			case LCD_ID_PULL_UP:
				gpio_tlmm_config(gpio_id1,0,GPIO_INPUT,
                                GPIO_PULL_UP,GPIO_2MA,GPIO_ENABLE);
				break;
			default:
				gpio_tlmm_config(gpio_id1,0,GPIO_INPUT,
                                GPIO_NO_PULL,GPIO_2MA,GPIO_ENABLE);
				break;
		}
	}

	oem_panel_id_power_ctrl(hw_id, false);

	return (id1<<2) | id0;

}

int mdss_dsi_read_tp_color(void *pmipi)
{
	uint32_t rec_buf[1];
	uint32_t *lp = rec_buf, data;
	uint32_t response_value = 0;
	struct mipi_panel_info *mipi = pmipi;

	int ret = response_value;
	char read_id_dah_cmd[4] = { 0xDA, 0x00, 0x06, 0xA0 };

	struct mipi_dsi_cmd read_dah_start_cmd =
		{sizeof(read_id_dah_cmd), read_id_dah_cmd, 0x00};

	ret = mdss_dsi_cmds_tx(mipi, &read_dah_start_cmd, 1, 0);
	if (ret)
	{
		response_value = 0xff;
		dprintf(ERROR,"Read TP COLOR(0xDA) failed! the value = %x\n",
                response_value);
		goto exit_read_tpcolor;
	}

	if (!mdss_dsi_cmds_rx(mipi, &lp, 1, 1))
	{
		response_value = 0xff;
		dprintf(ERROR,"Read TP COLOR(0xDA) failed! the value = %x\n",
                response_value);
		goto exit_read_tpcolor;
	}

	data = ntohl(*lp);
	data = data >> 8;
	response_value = (data & 0xff00) >> 8;
	dprintf(INFO,"Read TP COLOR(0xDA) successfully! the value = %x\n",
            response_value);

exit_read_tpcolor:
	snprintf(tpcolor_buf,sizeof(tpcolor_buf), " TP_COLOR=%d", response_value);

	return 0;
}

int oem_panel_rotation()
{
	return NO_ERROR;
}

int oem_panel_on()
{
	/*
	 *OEM can keep their panel specific on instructions in this
	 *function
	*/
	#ifndef CONFIG_LCDKIT_DRIVER
	if (panel_id == OTM1906C_1080P_CMD_PANEL) {
		/* needs extra delay to avoid unexpected artifacts */
		mdelay(OTM1906C_1080P_CMD_PANEL_ON_DELAY);
	} else if (panel_id == TRULY_1080P_CMD_PANEL ||
			panel_id == TRULY_1080P_VIDEO_PANEL) {
		mdelay(TRULY_1080P_PANEL_ON_DELAY);
	}else if (panel_id == R69006_1080P_CMD_PANEL) {
		mdelay(R69006_1080P_CMD_PANEL_ON_DELAY);
	}
    #endif

	return NO_ERROR;
}

int oem_panel_off()
{
	/* OEM can keep their panel specific off instructions
	 * in this function
	 */
	return NO_ERROR;
}

#define DISPLAY_MAX_PANEL_DETECTION         2
uint32_t oem_panel_max_auto_detect_panels()
{
	return target_panel_auto_detect_enabled() ? DISPLAY_MAX_PANEL_DETECTION : 0;
}

bool oem_panel_select(const char *panel_name,
            struct panel_struct *panelstruct, struct msm_panel_info *pinfo,
            struct mdss_dsi_phy_ctrl *phy_db)
{
    uint32_t panel_id;
	uint32_t hw_id = board_hardware_id();

    dprintf(INFO,"lcd board id = %d\n", hw_id);

	panel_id = lcdkit_panel_init(hw_id);

	if (false == hw_init_panel_data(panelstruct, pinfo, phy_db, panel_id))
	{
        dprintf(CRITICAL, "%s init panel data failed.\n", __func__);
        return PANEL_TYPE_UNKNOWN;
	}

    dprintf(INFO, "%s init panel %d data success.\n", __func__, panel_id);

    lcdkit_init();

	return PANEL_TYPE_DSI;
}

uint32_t lcdkit_panel_reset(uint8_t enable)
{
    uint32_t ret = NO_ERROR;

    ret = mdss_dsi_panel_reset(enable);
   	if (ret) {
        dprintf(CRITICAL, "panel reset disable failed\n");
    }

	return ret;
}

int target_panel_reset(uint8_t enable, struct panel_reset_sequence *resetseq,
						struct msm_panel_info *pinfo)
{
	int ret = NO_ERROR;

    dprintf(INFO,"lcd reset enter\n");

    //lcdkit_init();

	if (enable)
    {
		if (pinfo->mipi.use_enable_gpio)
        {
            dprintf(INFO,"set lcd enable gpio enter\n");
            lcdkit_gpio_cfg(enable_gpio, 0, GPIO_STATE_HIGH);
		}

		if (lcdkit_config.lcd_platform->use_bl_gpio)
        {
            dprintf(INFO,"set lcd bkl gpio enter\n");
            lcdkit_gpio_cfg(bkl_gpio, 0, GPIO_STATE_HIGH);
		}

		if (lcdkit_config.lcd_platform->use_bl_en_gpio)
        {
            dprintf(INFO,"set lcd bl en gpio enter\n");
            lcdkit_gpio_cfg(disp_bl_gpio, 0, GPIO_STATE_HIGH);
		}

        lcdkit_gpio_cfg(reset_gpio, 0, GPIO_STATE_HIGH);

        #if 0
        if (lcdkit_config.lcd_misc->ctrl_tp_vci)
        {
			dprintf(INFO, "%s lcd need tp keep reset high at lk", __func__);

            lcdkit_gpio_cfg(tp_reset_gpio, 0, GPIO_STATE_HIGH);
		}
        else
        {
			;
		}
        #endif

		/* reset */
		for (int i = 0; i < RESET_GPIO_SEQ_LEN; i++)
        {
			if (resetseq->pin_state[i] == GPIO_STATE_LOW)
				gpio_set_dir(reset_gpio.pin_id, GPIO_STATE_LOW);
			else
				gpio_set_dir(reset_gpio.pin_id, GPIO_STATE_HIGH);

			mdelay(resetseq->sleep[i]);
		}

		if (lcdkit_config.lcd_platform->use_mode_gpio)
        {
            dprintf(INFO,"set lcd mode gpio enter\n");

            lcdkit_gpio_cfg(lcd_mode_gpio, 0,
                (pinfo->lcdc.split_display || pinfo->lcdc.dst_split) ?
                GPIO_STATE_LOW : GPIO_STATE_HIGH);
		}

	}
    else if(!target_cont_splash_screen())
    {
		gpio_set_dir(reset_gpio.pin_id, 0);
		lcdkit_delay(lcdkit_config.lcd_delay->delay_af_rst_off, 0);

        if (pinfo->mipi.use_enable_gpio)
		    gpio_set_dir(enable_gpio.pin_id, 0);

        if (lcdkit_config.lcd_platform->use_bl_en_gpio)
            gpio_set_dir(disp_bl_gpio.pin_id, 0);

        if (lcdkit_config.lcd_platform->use_bl_gpio)
            gpio_set_dir(bkl_gpio.pin_id, 0);

        #if 0
        if (lcdkit_config.lcd_misc->ctrl_tp_vci)
            gpio_set_dir(tp_reset_gpio.pin_id, 0);
        #endif

        if (lcdkit_config.lcd_platform->use_mode_gpio)
            gpio_set_dir(lcd_mode_gpio.pin_id, 0);
    }

	return ret;
}

int target_gpio_config(uint32_t gpio, int status)
{
    gpio_tlmm_config(gpio, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA, GPIO_DISABLE);
    gpio_set_dir(gpio, status);
    return 0;
}

static inline int lcdkit_bias_is_gpio_ctrl_power(void)
{
    return (lcdkit_config.lcd_misc->bias_ctrl_mode
                                & POWER_CTRL_BY_GPIO) ? true : false;
}

static inline int lcdkit_bias_is_regulator_ctrl_power(void)
{
    return (lcdkit_config.lcd_misc->bias_ctrl_mode
                                & POWER_CTRL_BY_REGULATOR) ? true : false;
}

static inline int lcdkit_bias_is_used_ctrl_power(void)
{
    return (lcdkit_bias_is_gpio_ctrl_power()||lcdkit_bias_is_regulator_ctrl_power()||lcdkit_bias_is_ic_ctrl_power()) ? true : false;
}

static inline int lcdkit_vci_is_gpio_ctrl_power(void)
{
    return (lcdkit_config.lcd_misc->vci_ctrl_mode
                                & POWER_CTRL_BY_GPIO) ? true : false;
}

static inline int lcdkit_vci_is_regulator_ctrl_power(void)
{
    return (lcdkit_config.lcd_misc->vci_ctrl_mode
                                & POWER_CTRL_BY_REGULATOR) ? true : false;
}

static inline int lcdkit_iovcc_is_gpio_ctrl_power(void)
{
    return (lcdkit_config.lcd_misc->iovcc_ctrl_mode
                                & POWER_CTRL_BY_GPIO) ? true : false;
}

static inline int lcdkit_iovcc_is_regulator_ctrl_power(void)
{
    return (lcdkit_config.lcd_misc->iovcc_ctrl_mode
                                & POWER_CTRL_BY_REGULATOR) ? true : false;
}

static inline int lcdkit_is_lcd_panel(void)
{
    return (lcdkit_config.lcd_misc->panel_type & PANEL_TYPE_LCD) ? true : false;
}

static inline int lcdkit_is_oled_panel(void)
{
    return (lcdkit_config.lcd_misc->panel_type & PANEL_TYPE_OLED) ? true : false;
}

int target_power_on(struct msm_panel_info *pinfo)
{
    int rc = 0;
    uint32_t ldo_num = 0;
	struct panel_struct panelstruct_lcd = mdss_dsi_get_panel_data();
	
    ldo_num = update_value(lcdkit_config.lcd_platform->on_ldomap,ldo_num);
    dprintf(INFO,"set lcd ldo enter 0x%x.\n", ldo_num);
    regulator_enable(ldo_num);

    if (HYBRID == lcdkit_config.lcd_misc->tp_ic_type)
    {
        if (lcdkit_vci_is_regulator_ctrl_power())
        {
            ldo_num = update_value(lcdkit_config.lcd_platform->vci_ldomap, REG_LDO10);
            dprintf(INFO,"set lcd vci enter 0x%x.\n", ldo_num);
            regulator_enable(ldo_num);
        }
        else
        {
            target_gpio_config(lcdkit_config.lcd_platform->gpio_vci, GPIO_STATE_HIGH);
        }
        lcdkit_delay(lcdkit_config.lcd_delay->delay_af_vci_on, 0);
    }

    if (lcdkit_iovcc_is_regulator_ctrl_power())
    {
        ldo_num = update_value(lcdkit_config.lcd_platform->iovcc_ldomap, REG_LDO6);
        dprintf(INFO,"set lcd vcc enter 0x%x.\n", ldo_num);
        regulator_enable(ldo_num);
    }
    else
    {
        target_gpio_config(lcdkit_config.lcd_platform->gpio_iovcc, GPIO_STATE_HIGH);
    }
    lcdkit_delay(lcdkit_config.lcd_delay->delay_af_iovcc_on, 10);

    if (lcdkit_config.lcd_misc->first_reset)
    {
        dprintf(INFO,"lcd first reset.\n");

    	lcdkit_panel_reset(true);
    	mdelay(1);
    }

    if(!lcdkit_bias_is_used_ctrl_power())
    {
        dprintf(CRITICAL, "%s: bias is not used!\n", __func__);
    }
    else if(lcdkit_bias_is_regulator_ctrl_power())
    {
        rc = wled_init(pinfo);
        if (rc)
        {
    	    dprintf(CRITICAL, "%s: wled init failed\n", __func__);
        	return rc;
        }

        rc = qpnp_ibb_enable(true); /*5V boost*/
        if (rc)
        {
    	    dprintf(CRITICAL, "%s: qpnp_ibb failed\n", __func__);
        	return rc;
        }

        //vsp and vsn can only delay once
        //lcdkit_delay(lcdkit_config.lcd_delay->delay_af_vsp_on, 0);
        lcdkit_delay(lcdkit_config.lcd_delay->delay_af_vsn_on, 0);
    }
    else if(lcdkit_bias_is_gpio_ctrl_power())
    {
        rc = wled_init(pinfo);
        if (rc)
        {
            printf( "%s: wled init failed\n", __func__);
            return rc;
        }

        gpio_tlmm_config(lcdkit_config.lcd_platform->gpio_vsp, 0,
            lcd_vsp_gpio.pin_direction, lcd_vsp_gpio.pin_pull,
            lcd_vsp_gpio.pin_strength, lcd_vsp_gpio.pin_state);
        gpio_set_dir(lcdkit_config.lcd_platform->gpio_vsp, 2);
        lcdkit_delay(lcdkit_config.lcd_delay->delay_af_vsp_on, 0);

        gpio_tlmm_config(lcdkit_config.lcd_platform->gpio_vsn, 0,
            lcd_vsn_gpio.pin_direction, lcd_vsn_gpio.pin_pull,
            lcd_vsn_gpio.pin_strength, lcd_vsn_gpio.pin_state);
        gpio_set_dir(lcdkit_config.lcd_platform->gpio_vsn, 2);
        lcdkit_delay(lcdkit_config.lcd_delay->delay_af_vsn_on, 0);
    }
	    else if(lcdkit_bias_is_ic_ctrl_power())
    {	
        if(lcdkit_is_default_panel())
        {
       		 gpio_tlmm_config(lcdkit_config.lcd_platform->gpio_vsp, 0,
                lcd_vsp_gpio.pin_direction, lcd_vsp_gpio.pin_pull,
                lcd_vsp_gpio.pin_strength, lcd_vsp_gpio.pin_state);
            gpio_set_dir(lcdkit_config.lcd_platform->gpio_vsp, 0);
            lcdkit_delay(lcdkit_config.lcd_delay->delay_af_vsp_on, 0);

            gpio_tlmm_config(lcdkit_config.lcd_platform->gpio_vsn, 0,
                lcd_vsn_gpio.pin_direction, lcd_vsn_gpio.pin_pull,
                lcd_vsn_gpio.pin_strength, lcd_vsn_gpio.pin_state);
            gpio_set_dir(lcdkit_config.lcd_platform->gpio_vsn, 0);
            dprintf(CRITICAL,"can't distinguish expect LCD module type, not open bias vsp and vsn\n"); 
        }
		else
        {
            struct lcd_bias_voltage_info *pbias_ic = NULL;
            gpio_tlmm_config(lcdkit_config.lcd_platform->gpio_vsp, 0,
                lcd_vsp_gpio.pin_direction, lcd_vsp_gpio.pin_pull,
                lcd_vsp_gpio.pin_strength, lcd_vsp_gpio.pin_state);
            gpio_set_dir(lcdkit_config.lcd_platform->gpio_vsp, 2);
            lcdkit_delay(lcdkit_config.lcd_delay->delay_af_vsp_on, 0);

            gpio_tlmm_config(lcdkit_config.lcd_platform->gpio_vsn, 0,
                lcd_vsn_gpio.pin_direction, lcd_vsn_gpio.pin_pull,
                lcd_vsn_gpio.pin_strength, lcd_vsn_gpio.pin_state);
            gpio_set_dir(lcdkit_config.lcd_platform->gpio_vsn, 2);
			lcdkit_delay(LCD_BIAS_DELAY_TIME,LCD_BIAS_DELAY_TIME);
		lcdkit_bias_ic_init(panelstruct_lcd.lcd_bias_ic_info.lcd_bias_ic_list, panelstruct_lcd.lcd_bias_ic_info.num_of_lcd_bias_ic_list);
		pbias_ic = get_lcd_bias_ic_info();
		if(NULL != pbias_ic)
		{
		lcdkit_bias_ic_save_name(pbias_ic->name);
		}
	lcdkit_delay(lcdkit_config.lcd_delay->delay_af_vsn_on, 0);
        }
    }
    else
    {
        target_gpio_config(lcdkit_config.lcd_platform->gpio_vsp, GPIO_STATE_HIGH);
        lcdkit_delay(lcdkit_config.lcd_delay->delay_af_vsp_on, 0);
        target_gpio_config(lcdkit_config.lcd_platform->gpio_vsn, GPIO_STATE_HIGH);
        lcdkit_delay(lcdkit_config.lcd_delay->delay_af_vsn_on, 0);
    }
    /*
     * To meet the bach boe time-series standard
     * For 30-60ms BOE power on  mipi clk hs ,remove the 50ms delay
     */
    lcdkit_delay(lcdkit_config.lcd_delay->delay_af_panel_lk_power_on, 0);

    if (lcdkit_config.lcd_misc->second_reset)
    {
        dprintf(INFO,"lcd second reset.\n");
    	lcdkit_panel_reset(true);
    }

    return rc;
}

int target_power_off(struct msm_panel_info *pinfo)
{
    uint32_t ldo_num = 0;

    ldo_num = update_value(lcdkit_config.lcd_platform->off_ldomap, ldo_num);
    /*
     * LDO6, LDO3 and SMPS3 are shared with other subsystems.
     * Do not disable them.
     */
    regulator_disable(ldo_num);

    return 0;
}

int target_ldo_ctrl(uint8_t enable, struct msm_panel_info *pinfo)
{
    int rc = 0;
    dprintf(INFO,"lcd power enter\n");

    if (enable)
    {
        rc = target_power_on(pinfo);
    }
    else
    {
        rc = target_power_off(pinfo);
    }

	return rc;
}


