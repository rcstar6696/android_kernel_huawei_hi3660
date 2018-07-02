/*
 * Copyright 2013 HUAWEI Tech. Co., Ltd.
 */
#ifndef _LCDKIT_BACKLIGHT_IC_COMMON_H_
#define _LCDKIT_BACKLIGHT_IC_COMMON_H_
#include "lcdkit_i2c.h"
#define LCD_BACKLIGHT_IC_NAME_LEN  128
#define LCD_BACKLIGHT_INIT_CMD_NUM 30

enum baclight_ic_ctrl_flag
{
	BL_IC_LINEAR_MODE = 0,
	BL_IC_EXPONENTIAL_MODE = 1
};

enum backlight_ctrl_mode
{
	BL_REG_ONLY_MODE = 0,
	BL_PWM_ONLY_MODE = 1,
	BL_MUL_RAMP_MODE = 2,
	BL_RAMP_MUL_MODE = 3
};

enum baclight_ic_type
{
	BACKLIGHT_IC = 0,
	BACKLIGHT_BIAS_IC = 1
};

struct backlight_ic_cmd
{
    unsigned char ops_type; //0:read  1:write  2:update
    unsigned char cmd_reg;
    unsigned char cmd_val;
    unsigned char cmd_mask;	
};

struct backlight_reg_info
{
    unsigned char val_bits;
	unsigned char cmd_reg;
    unsigned char cmd_val;
    unsigned char cmd_mask;	
};

struct backlight_ic_info
{
	char            name[LCD_BACKLIGHT_IC_NAME_LEN];
    unsigned char   i2c_addr;
    unsigned char   i2c_num;
	unsigned char   qup_id;
	unsigned int    bl_level;
	unsigned char   bl_ctrl_mod;
	unsigned char   ic_type;
	unsigned int    ic_exponential_ctrl;
    unsigned int    ic_before_init_delay;
    unsigned int    ic_init_delay;
	struct backlight_ic_cmd check_cmd;
	struct backlight_ic_cmd init_cmds[LCD_BACKLIGHT_INIT_CMD_NUM];
	unsigned char    num_of_init_cmds;
    struct backlight_reg_info bl_lsb_reg_cmd;
    struct backlight_reg_info bl_msb_reg_cmd;
	struct backlight_ic_cmd bl_enable_cmd;
	struct backlight_ic_cmd bl_disable_cmd;
	struct backlight_ic_cmd disable_dev_cmd;
	struct backlight_ic_cmd bias_enable_cmd;
	struct backlight_ic_cmd bias_disable_cmd;
};


struct lcd_backlight_ic
{
	struct backlight_ic_info **lcd_backlight_ic_list;
	int num_of_lcd_backlight_ic_list;
};

int lcdkit_backlight_ic_inital(struct backlight_ic_info *pinfo);
int lcdkit_backlight_ic_set_brightness(struct backlight_ic_info *pinfo, unsigned int level);
int lcdkit_backlight_ic_enable_brightness(struct backlight_ic_info *pinfo);
int lcdkit_backlight_ic_disable_brightness(struct backlight_ic_info *pinfo);
int lcdkit_backlight_ic_disable_device(struct backlight_ic_info *pinfo);
int lcdkit_backlight_ic_bias(struct backlight_ic_info *pinfo, bool enable);
void lcdkit_backlight_ic_select(struct backlight_ic_info **pinfo, int len);
struct backlight_ic_info * get_lcd_backlight_ic_info(void);
int lcdkit_backlight_common_set(int bl_level);
#endif