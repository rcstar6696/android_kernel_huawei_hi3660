#include "lcdkit_i2c.h"

extern uint32_t g_fake_lcd_flag;
extern int lckdit_bias_ic_get_i2c_num(unsigned char *buf);
extern int lckdit_backlight_ic_get_i2c_num(unsigned char *buf);
static unsigned int get_i2c_number(unsigned char i2c_number)
{
    unsigned int i2c_num = INVALID_VALUE;
    PRINT_ERROR("lcd bias ic i2c num is %d\n",i2c_number);
    switch(i2c_number)
    {
        case 0:
            i2c_num = REG_BASE_I2C0;
            break;
        case 1:
            i2c_num = REG_BASE_I2C1;
            break;
        case 2:
            i2c_num = REG_BASE_I2C2;
            break;
        case 3:
            i2c_num = REG_BASE_I2C3;
            break;
        case 4:
            i2c_num = REG_BASE_I2C4;
            break;
        case 5:
            i2c_num = REG_BASE_I2C5;
            break;
        case 6:
            i2c_num = REG_BASE_I2C6;
            break;
        case 7:
            i2c_num = REG_BASE_I2C7;
            break;
        default:
            i2c_num = INVALID_VALUE;
            break;
    }
    return i2c_num;
}

int lcdkit_bias_ic_i2c_read_u8(unsigned char chip_addr, unsigned char reg, unsigned char* val)
{
    int ret = 0;
    unsigned char addr_readdata[1] = {reg};
    struct i2c_operators *i2c_ops = get_operators(I2C_MODULE_NAME_STR);
    unsigned int i2c_chip_num = INVALID_VALUE;
    unsigned char i2c_num = 0;

    if (NULL == val)
    {
        PRINT_ERROR("[%s]:NULL pointer!\n", __FUNCTION__);
        return -1;
    }

    if (NULL == i2c_ops || NULL == i2c_ops->i2c_read)
    {
        PRINT_ERROR("bad i2c_ops\n");
        return -1;
    }

    ret = lckdit_bias_ic_get_i2c_num(&i2c_num);
    if(ret < 0)
    {
        return -1;
    }
    i2c_chip_num = get_i2c_number(i2c_num);
    if(i2c_chip_num == INVALID_VALUE)
    {
        return -1;
    }

    i2c_ops->i2c_init(i2c_chip_num);
    ret = i2c_ops->i2c_read(i2c_chip_num, (unsigned char *)addr_readdata, sizeof(addr_readdata), chip_addr);
    if (ret < 0)
    {
        PRINT_ERROR("[%s]:i2c read error, reg = 0x%x\n", __FUNCTION__, reg);
	}
    i2c_ops->i2c_exit(i2c_chip_num);

    *val = addr_readdata[0];
    PRINT_ERROR("lcd bias ic read chip addr = 0x%x,  reg = 0x%x,  val = 0x%x\n",chip_addr, reg, addr_readdata[0]);
    return ret;
}

int lcdkit_bias_ic_i2c_write_u8(unsigned char chip_addr, unsigned char reg, unsigned char val)
{
    int ret = 0;
    unsigned char I2C_write[2] = {reg, val};
    struct i2c_operators *i2c_ops = NULL;
    unsigned int i2c_chip_num = INVALID_VALUE;
    unsigned char i2c_num = 0;

    i2c_ops = get_operators(I2C_MODULE_NAME_STR);
    if (!i2c_ops)
    {
        PRINT_ERROR("can not get i2c_ops!\n");
        return -1;
    }

    ret = lckdit_bias_ic_get_i2c_num(&i2c_num);
    if(ret < 0)
    {
        return -1;
    }
    i2c_chip_num = get_i2c_number(i2c_num);
    if(i2c_chip_num == INVALID_VALUE)
    {
        return -1;
    }

    i2c_ops->i2c_init(i2c_chip_num);
    ret = i2c_ops->i2c_write(i2c_chip_num, (unsigned char *)I2C_write, sizeof(I2C_write), chip_addr);
    if(ret < 0)
    {
        PRINT_ERROR("i2c write error chip_addr = 0x%x, reg = 0x%x\n", chip_addr, reg);
    }
    i2c_ops->i2c_exit(i2c_chip_num);

    PRINT_ERROR("lcd bias ic write chip addr = 0x%x,  reg = 0x%x, val = 0x%x\n",chip_addr, reg,val);
    return ret;
}
int lcdkit_backlight_ic_i2c_read_u8(unsigned char chip_addr, unsigned char reg, unsigned char* val)
{
    int ret = 0;
    unsigned char addr_readdata[1] = {reg};
    struct i2c_operators *i2c_ops = get_operators(I2C_MODULE_NAME_STR);
    unsigned int i2c_chip_num = INVALID_VALUE;
    unsigned char i2c_num = 0;

    if (NULL == val)
    {
        PRINT_ERROR("[%s]:NULL pointer!\n", __FUNCTION__);
        return -1;
    }

    if (NULL == i2c_ops || NULL == i2c_ops->i2c_read)
    {
        PRINT_ERROR("bad i2c_ops\n");
        return -1;
    }

    ret = lckdit_backlight_ic_get_i2c_num(&i2c_num);
    if(ret < 0)
    {
        return -1;
    }
    i2c_chip_num = get_i2c_number(i2c_num);
    if(i2c_chip_num == INVALID_VALUE)
    {
        return -1;
    }

    i2c_ops->i2c_init(i2c_chip_num);
    ret = i2c_ops->i2c_read(i2c_chip_num, (unsigned char *)addr_readdata, sizeof(addr_readdata), chip_addr);
    if (ret < 0)
    {
        PRINT_ERROR("[%s]:i2c read error, reg = 0x%x\n", __FUNCTION__, reg);
    }
    i2c_ops->i2c_exit(i2c_chip_num);

    *val = addr_readdata[0];
    PRINT_ERROR("lcd bias ic read chip addr = 0x%x,  reg = 0x%x,  val = 0x%x\n",chip_addr, reg, addr_readdata[0]);
    return ret;
}

int lcdkit_backlight_ic_i2c_write_u8(unsigned char chip_addr, unsigned char reg, unsigned char val)
{
    int ret = 0;
    unsigned char I2C_write[2] = {reg, val};
    struct i2c_operators *i2c_ops = NULL;
    unsigned int i2c_chip_num = INVALID_VALUE;
    unsigned char i2c_num = 0;

    i2c_ops = get_operators(I2C_MODULE_NAME_STR);
    if (!i2c_ops)
    {
        PRINT_ERROR("can not get i2c_ops!\n");
        return -1;
    }

    ret = lckdit_backlight_ic_get_i2c_num(&i2c_num);
    if(ret < 0)
    {
        return -1;
    }
    i2c_chip_num = get_i2c_number(i2c_num);
    if(i2c_chip_num == INVALID_VALUE)
    {
        return -1;
    }

    i2c_ops->i2c_init(i2c_chip_num);
    ret = i2c_ops->i2c_write(i2c_chip_num, (unsigned char *)I2C_write, sizeof(I2C_write), chip_addr);
    if(ret < 0)
    {
        PRINT_ERROR("i2c write error chip_addr = 0x%x, reg = 0x%x\n", chip_addr, reg);
    }
    i2c_ops->i2c_exit(i2c_chip_num);

    PRINT_ERROR("lcd bias ic write chip addr = 0x%x,  reg = 0x%x, val = 0x%x\n",chip_addr, reg,val);
    return ret;
}