#include "lcdkit_i2c.h"
#include "lcd_bl.h"
#include "lcd_bl_linear_exponential_table.h"


static struct backlight_ic_info *g_lcd_backlight_chip = NULL;

int lckdit_backlight_ic_get_i2c_num(unsigned char *buf)
{
    if(NULL == g_lcd_backlight_chip)
    {
        return -1;
    }
    *buf = g_lcd_backlight_chip->i2c_num;
	return 0;
}

int lcdkit_backlight_ic_get_i2c_qup_id(unsigned char *buf)
{
    if(NULL == g_lcd_backlight_chip)
    {
        return -1;
    }
    *buf = g_lcd_backlight_chip->qup_id;
    return 0;
}

static int lcdkit_backlight_ic_i2c_update_u8(unsigned char chip_addr, unsigned char reg, unsigned char mask, unsigned char val)
{
    int ret = 0;
    unsigned char orig_val = 0;
    unsigned char value = 0;

    ret = lcdkit_backlight_ic_i2c_read_u8(chip_addr, reg, &orig_val);
    if(ret < 0)
    {
        LCDKIT_DEBUG_ERROR("lcdkit_i2c_update_u8 read failed\n");
        return ret;
    }
	
    value = orig_val & (~mask);

    value |= (val & mask);

    if(value != orig_val)
    {
        ret = lcdkit_backlight_ic_i2c_write_u8(chip_addr, reg, value);
        if(ret < 0)
        {
            LCDKIT_DEBUG_ERROR("lcdkit_i2c_update_u8 write failed\n");
            return ret;
        }
    }
    LCDKIT_DEBUG_ERROR("lcdkit_i2c_update_u8 addr is 0x%x, val is 0x%x\n", reg, value);
    return ret;
}

static int lcdkit_backlight_ic_check(struct backlight_ic_info *pinfo)
{
    int ret = 0;
	unsigned char readval = 0;
    LCDKIT_DEBUG_ERROR("lcd_backlight_ic_check\n");
    if(pinfo == NULL)
    {
        return -1;
    }
    g_lcd_backlight_chip = pinfo;
    switch(pinfo->check_cmd.ops_type)
    {
        case 0:
            ret = lcdkit_backlight_ic_i2c_read_u8(pinfo->i2c_addr, pinfo->check_cmd.cmd_reg, &readval);
            break;
        case 1:
            ret = lcdkit_backlight_ic_i2c_write_u8(pinfo->i2c_addr, pinfo->check_cmd.cmd_reg, pinfo->check_cmd.cmd_val);
            break;
        case 2:
            ret = lcdkit_backlight_ic_i2c_update_u8(pinfo->i2c_addr, pinfo->check_cmd.cmd_reg, pinfo->check_cmd.cmd_mask, pinfo->check_cmd.cmd_val);
            break;
        default:
            ret = -1;
            break;
    }
	
    if(ret < 0)
    {
        return ret;		
    }
	if(0 == pinfo->check_cmd.ops_type)
	{
	    if(readval != pinfo->check_cmd.cmd_val)
		{
		    return -1;
		}
	}
    return 0;
}
int lcdkit_backlight_ic_inital(struct backlight_ic_info *pinfo)
{
    int ret = 0;
    int i = 0;
    LCDKIT_DEBUG_ERROR("lcd_backlight_ic_inital\n");
    if(pinfo == NULL)
    {
        return -1;
    }

    for(i=0; i<pinfo->num_of_init_cmds; i++)
    {
        switch(pinfo->init_cmds[i].ops_type)
        {
            case 0:
                ret = lcdkit_backlight_ic_i2c_read_u8(pinfo->i2c_addr, pinfo->init_cmds[i].cmd_reg, &(pinfo->init_cmds[i].cmd_val));
                break;
            case 1:
                ret = lcdkit_backlight_ic_i2c_write_u8(pinfo->i2c_addr, pinfo->init_cmds[i].cmd_reg, pinfo->init_cmds[i].cmd_val);
                break;
            case 2:
                ret = lcdkit_backlight_ic_i2c_update_u8(pinfo->i2c_addr, pinfo->init_cmds[i].cmd_reg, pinfo->init_cmds[i].cmd_mask, pinfo->init_cmds[i].cmd_val);
                break;
            default:
                break;
        }
        if(ret < 0)
        {
            LCDKIT_DEBUG_ERROR("operation chip_addr 0x%x  reg 0x%x failed!\n",pinfo->i2c_addr,pinfo->init_cmds[i].cmd_reg);
            return ret;
        }
    }

    return ret;
}

int lcdkit_backlight_ic_set_brightness(struct backlight_ic_info *pinfo, unsigned int level)
{
    unsigned char level_lsb = 0;
    unsigned char level_msb = 0;
    int ret = 0;

    if(pinfo == NULL)
    {
        return -1;
    }
    LCDKIT_DEBUG_ERROR("lcd_backlight_ic_set_brightness\n");

    level_lsb = level & pinfo->bl_lsb_reg_cmd.cmd_mask;
    level_msb = (level >> pinfo->bl_lsb_reg_cmd.val_bits)&(pinfo->bl_msb_reg_cmd.cmd_mask);
    if(pinfo->bl_lsb_reg_cmd.val_bits != 0)
    {
        ret = lcdkit_backlight_ic_i2c_write_u8(pinfo->i2c_addr, pinfo->bl_lsb_reg_cmd.cmd_reg, level_lsb);
        if(ret < 0)
        {
            LCDKIT_DEBUG_ERROR("set backlight ic brightness failed!\n");
            return ret;
        }
    }
    ret = lcdkit_backlight_ic_i2c_write_u8(pinfo->i2c_addr, pinfo->bl_msb_reg_cmd.cmd_reg, level_msb);
    if(ret < 0)
    {
        LCDKIT_DEBUG_ERROR("set backlight ic brightness failed!\n");
    }

    return ret;
}

int lcdkit_backlight_ic_enable_brightness(struct backlight_ic_info *pinfo)
{
    int ret = 0;
    LCDKIT_DEBUG_ERROR("lcd_backlight_ic_enable_brightness\n");
    if(pinfo == NULL)
    {
        return -1;
    }

    ret = lcdkit_backlight_ic_i2c_update_u8(pinfo->i2c_addr, pinfo->bl_enable_cmd.cmd_reg, pinfo->bl_enable_cmd.cmd_mask, pinfo->bl_enable_cmd.cmd_val);
    if(ret < 0)
    {
        LCDKIT_DEBUG_ERROR("enable backlight ic brightness failed!\n");
    }

    return ret;
}

int lcdkit_backlight_ic_disable_brightness(struct backlight_ic_info *pinfo)
{
    int ret = 0;
    LCDKIT_DEBUG_ERROR("lcd_backlight_ic_disable_brightness\n");
    if(pinfo == NULL)
    {
        return -1;
    }
	
    ret = lcdkit_backlight_ic_i2c_update_u8(pinfo->i2c_addr, pinfo->bl_disable_cmd.cmd_reg, pinfo->bl_disable_cmd.cmd_mask, pinfo->bl_disable_cmd.cmd_val);
    if(ret < 0)
    {
        LCDKIT_DEBUG_ERROR("disable backlight ic brightness failed!\n");
    }

    return ret;
}
int lcdkit_backlight_ic_disable_device(struct backlight_ic_info *pinfo)
{
    int ret = 0;
    LCDKIT_DEBUG_ERROR("lcd_backlight_ic_disable_device\n");
    if(pinfo == NULL)
    {
        return -1;
    }
    ret = lcdkit_backlight_ic_i2c_update_u8(pinfo->i2c_addr, pinfo->disable_dev_cmd.cmd_reg, pinfo->disable_dev_cmd.cmd_mask, pinfo->disable_dev_cmd.cmd_val);
    if(ret < 0)
    {
        LCDKIT_DEBUG_ERROR("disable backlight ic device failed!\n");
    }
    return ret;
}

int lcdkit_backlight_ic_bias(struct backlight_ic_info *pinfo, bool enable)
{
    int ret = 0;
    LCDKIT_DEBUG_ERROR("lcd_backlight_ic_bias enable is %d\n",enable);
    if(pinfo == NULL)
    {
        return -1;
    }
    if(enable)
    {
        ret = lcdkit_backlight_ic_i2c_update_u8(pinfo->i2c_addr, pinfo->bias_enable_cmd.cmd_reg, pinfo->bias_enable_cmd.cmd_mask, pinfo->bias_enable_cmd.cmd_val);
        if(ret < 0)
        {
            LCDKIT_DEBUG_ERROR("disable backlight ic enable bias failed!\n");
            return ret;
        }
    }
    else
    {
        ret = lcdkit_backlight_ic_i2c_update_u8(pinfo->i2c_addr, pinfo->bias_disable_cmd.cmd_reg, pinfo->bias_disable_cmd.cmd_mask, pinfo->bias_disable_cmd.cmd_val);
        if(ret < 0)
        {
            LCDKIT_DEBUG_ERROR("disable backlight ic enable bias failed!\n");
            return ret;
        }
    }
    return 0;
}

void lcdkit_backlight_ic_select(struct backlight_ic_info **pinfo, int len)
{
    int i = 0;
    int ret = 0;
    LCDKIT_DEBUG_ERROR("lcd_backlight_ic_select\n");

    if(NULL == pinfo)
    {
        LCDKIT_DEBUG_ERROR("lcd_backlight_ic_select pointer is null\n");
        return;
    }
	
    if(len <= 0)
    {
        LCDKIT_DEBUG_ERROR("lcd_backlight_ic_select no config backlight ic\n");
        return;
    }
    
    for (i = 0; i < len; ++i) 
    {
        ret = lcdkit_backlight_ic_check(pinfo[i]);
        if(ret == 0)
        {
            LCDKIT_DEBUG_ERROR("lcd_backlight_ic_select  backlight ic index is %d\n",i);
            break;
        }
    }

    if(i == len)
    {
        LCDKIT_DEBUG_ERROR("lcd_backlight_ic_select no lcd backlight ic is found!\n");
        g_lcd_backlight_chip = NULL;
    }

    return;	
}

struct backlight_ic_info * get_lcd_backlight_ic_info(void)
{
    return g_lcd_backlight_chip;
}

int lcdkit_backlight_common_set(int bl_level)
{
	struct backlight_ic_info *tmp = NULL;
	int bl_ctrl_mod = -1;
	unsigned int backlight_level = 0;

	tmp =  get_lcd_backlight_ic_info();
	if(tmp != NULL)
	{
		if(tmp->ic_exponential_ctrl == BL_IC_EXPONENTIAL_MODE)
		{
			backlight_level = linear_exponential_table[bl_level*tmp->bl_level/255];
		}
		else
		{
			backlight_level = bl_level*tmp->bl_level/255;
		}
		LCDKIT_DEBUG_ERROR("lcdkit_backlight_common_set backlight level is %d\n",backlight_level);
		bl_ctrl_mod = tmp->bl_ctrl_mod;
		switch(bl_ctrl_mod)
		{
			case BL_REG_ONLY_MODE:
			case BL_MUL_RAMP_MODE:
			case BL_RAMP_MUL_MODE:
				lcdkit_backlight_ic_set_brightness(tmp,backlight_level);
				break;
			case BL_PWM_ONLY_MODE:
			default:
				break;
		}
	}

	return bl_ctrl_mod;
}
