#include "lcdkit_i2c.h"
#include "lcdkit_bias_ic_common.h"

static struct lcd_bias_voltage_info *g_lcd_bias_chip = NULL;

static int lcdkit_bias_voltage_ic_i2c_update_u8(unsigned char chip_addr, unsigned char reg, unsigned char mask, unsigned char val)
{
    int ret = 0;
    unsigned char orig_val = 0;
    unsigned char value = 0;

    ret = lcdkit_bias_ic_i2c_read_u8(chip_addr, reg, &orig_val);
    if(ret < 0)
    {
        LCDKIT_DEBUG_ERROR("i2c read error chip_addr = 0x%x, reg = 0x%x\n", chip_addr, reg);
        return ret;
    }

    value = orig_val & (~mask);
    value |= (val & mask);

    if(value != orig_val)
    {
        ret = lcdkit_bias_ic_i2c_write_u8(chip_addr, reg, value);
        if(ret < 0)
        {
            LCDKIT_DEBUG_ERROR("i2c write error chip_addr = 0x%x, reg = 0x%x\n", chip_addr, reg);
            return ret;
        }
    }
    LCDKIT_DEBUG_ERROR("i2c update  chip_addr = 0x%x, reg = 0x%x, value = 0x%x \n", chip_addr, reg, val);
    return ret;
}

static int lcdkit_bias_voltage_inited(unsigned char vpos_target_cmd, unsigned char vneg_target_cmd)
{
    unsigned char vpos = 0;
    unsigned char vneg = 0;
    int ret = 0;

    if(!(g_lcd_bias_chip->ic_type & BIAS_IC_HAVE_E2PROM))
    {
        return 0;
    }
    ret = lcdkit_bias_ic_i2c_read_u8(g_lcd_bias_chip->i2c_addr, g_lcd_bias_chip->vpos_reg, &vpos);
    if (ret < 0)
    {
        LCDKIT_DEBUG_ERROR("lcd_bias_voltage_inited read vpos voltage failed\n");
        goto exit;
    }

    ret = lcdkit_bias_ic_i2c_read_u8(g_lcd_bias_chip->i2c_addr, g_lcd_bias_chip->vneg_reg, &vneg);
    if (ret < 0)
    {
        LCDKIT_DEBUG_ERROR("lcd_bias_voltage_inited read vneg voltage failed\n");
        goto exit;
    }

    LCDKIT_DEBUG_ERROR("target vpos : 0x%x,  target vneg: 0x%x\n", vpos_target_cmd, vneg_target_cmd);

    if(((vpos & g_lcd_bias_chip->vpos_mask) == vpos_target_cmd)
        && ((vneg & g_lcd_bias_chip->vneg_mask) == vneg_target_cmd))
    {
        ret = 1;
    }
	else
    {
        ret = 0;
    }
exit:
    return ret;
}

int lcdkit_bias_voltage_init(unsigned char vpos_cmd, unsigned char vneg_cmd)
{
    unsigned char vpos = 0;
    unsigned char vneg = 0;
    int ret = 0;

    vpos = (g_lcd_bias_chip->vpos_mask) & vpos_cmd;
    vneg = (g_lcd_bias_chip->vneg_mask) & vneg_cmd;

    ret = lcdkit_bias_ic_i2c_write_u8(g_lcd_bias_chip->i2c_addr, g_lcd_bias_chip->vpos_reg, vpos);
    if (ret < 0)
    {
        LCDKIT_DEBUG_ERROR("%s write vpos failed\n",__FUNCTION__);
        goto exit;
    }

    ret = lcdkit_bias_ic_i2c_write_u8(g_lcd_bias_chip->i2c_addr, g_lcd_bias_chip->vneg_reg, vneg);
    if (ret < 0)
    {
        LCDKIT_DEBUG_ERROR("%s write vneg failed\n",__FUNCTION__);
        goto exit;
    }

    if(0x00 != g_lcd_bias_chip->state_mask)
    {
        ret = lcdkit_bias_voltage_ic_i2c_update_u8(g_lcd_bias_chip->i2c_addr, g_lcd_bias_chip->state_reg, g_lcd_bias_chip->state_mask, g_lcd_bias_chip->state_val);
        if(ret < 0)
        {
            LCDKIT_DEBUG_ERROR("lcd_bias_voltage_init update addr = 0x%x failed\n",g_lcd_bias_chip->state_reg);
            goto exit;
        }
    }

    if(g_lcd_bias_chip->ic_type & BIAS_IC_HAVE_E2PROM)
    {
        if(0x00 != g_lcd_bias_chip->write_mask)
        {
            ret = lcdkit_bias_voltage_ic_i2c_update_u8(g_lcd_bias_chip->i2c_addr, g_lcd_bias_chip->write_reg, g_lcd_bias_chip->write_mask, g_lcd_bias_chip->write_val);
            if(ret < 0)
            {
                LCDKIT_DEBUG_ERROR("lcd_bias_voltage_init update addr = 0x%x failed\n",g_lcd_bias_chip->write_reg);
                goto exit;
            }
        }
    }

    if(g_lcd_bias_chip->delay)
    {
        mdelay(g_lcd_bias_chip->delay);
    }

exit:
    return ret;
}

static void lcdkit_bias_voltage_set_voltage(void)
{
    int ret = 0;

    if(g_lcd_bias_chip == NULL)
    {
        return;
    }

    ret = lcdkit_bias_voltage_inited(g_lcd_bias_chip->vpos_val, g_lcd_bias_chip->vneg_val);
    if (ret > 0)
    {
        LCDKIT_DEBUG_ERROR("lcd bias chip inited needn't reset value\n");
    }
    else if (ret < 0)
    {
        LCDKIT_DEBUG_ERROR("lcd bias chip I2C read fail\n");
    }
    else
    {
        ret = lcdkit_bias_voltage_init(g_lcd_bias_chip->vpos_val, g_lcd_bias_chip->vneg_val);
        if (ret < 0)
        {
            LCDKIT_DEBUG_ERROR("lcd_bias_voltage_init failed\n");
        }
        LCDKIT_DEBUG_ERROR("lcd bias chip inited succeed\n");
    }
}

static int lcdkit_bias_voltage_ic_select(struct lcd_bias_voltage_info *pinfo)
{
    int ret = 0;

    if(pinfo == NULL)
    {
        return -1;
    }
    g_lcd_bias_chip = pinfo;
    LCDKIT_DEBUG_ERROR("lcd bias ic name is %s  i2c_addr is 0x%x  i2c_num is 0x%x ic_type is %d\n",pinfo->name,pinfo->i2c_addr,pinfo->i2c_num,pinfo->ic_type);

    if(pinfo->ic_type & BIAS_IC_READ_INHIBITION)
    {
        ret = lcdkit_bias_ic_i2c_write_u8(pinfo->i2c_addr, pinfo->check_reg, pinfo->check_val);
        if(ret < 0)
        {
            LCDKIT_DEBUG_ERROR("lcd_bias_voltage_ic_select write failed!\n");
            g_lcd_bias_chip = NULL;
            goto exit;
        }
        else
        {
            lcdkit_bias_voltage_set_voltage();
            ret = 0;
        }
    }
    else
    {
        unsigned char val = 0;

        if(0x00 != pinfo->check_mask)
        {
            ret = lcdkit_bias_ic_i2c_read_u8(pinfo->i2c_addr,  pinfo->check_reg, &val);
		    if(ret < 0)
		    {
                LCDKIT_DEBUG_ERROR("lcd_bias_voltage_ic_select read failed!\n");
                g_lcd_bias_chip = NULL;
                goto exit;
            }
            else
            {
                if(val == pinfo->check_val)
                {
                    lcdkit_bias_voltage_set_voltage();
                    ret = 0;
                }
		else
                {
                    ret = -1;
                    g_lcd_bias_chip = NULL;
                    goto exit;      
                }
            }
        }
	else
        {
            ret = lcdkit_bias_ic_i2c_read_u8(pinfo->i2c_addr,  pinfo->check_reg, &val);
            if(ret < 0)
            {
                LCDKIT_DEBUG_ERROR("lcd_bias_voltage_ic_select read failed!\n");
                g_lcd_bias_chip = NULL;
                goto exit;
            }
            else
            {
                lcdkit_bias_voltage_set_voltage();
                ret = 0;
            }
        }
    }
exit:
    return ret;
}

void lcdkit_bias_ic_init(struct lcd_bias_voltage_info **pinfo, int len)
{
    int i = 0;
    int ret = 0;

    if(NULL == pinfo)
    {
        LCDKIT_DEBUG_ERROR("pinfo pointer is null\n");
        return;
    }
    if(len <= 0)
    {
        LCDKIT_DEBUG_ERROR("no config bais ic\n");
        return;
    }

    for (i = 0; i < len; ++i)
    {
        ret = lcdkit_bias_voltage_ic_select(pinfo[i]);
        if(ret == 0)
        {
            LCDKIT_DEBUG_ERROR("bais ic index is %d\n",i);
            break;
        }
    }

    if(i == len)
    {
        LCDKIT_DEBUG_ERROR("no lcd bais ic is found!\n");
        g_lcd_bias_chip = NULL;
    }

    return;
}

int lckdit_bias_ic_get_i2c_num(unsigned char *buf)
{
    if(NULL == g_lcd_bias_chip)
    {
        return -1;
    }
    *buf = g_lcd_bias_chip->i2c_num;
	return 0;
}

int lcdkit_bias_ic_get_i2c_qup_id(unsigned char *buf)
{
    if(NULL == g_lcd_bias_chip)
    {
        return -1;
    }
    *buf = g_lcd_bias_chip->qup_id;
    return 0;
}

struct lcd_bias_voltage_info * get_lcd_bias_ic_info(void)
{
    return g_lcd_bias_chip;
}
