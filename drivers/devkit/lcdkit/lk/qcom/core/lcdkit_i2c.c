#include "lcdkit_i2c.h"

extern int lckdit_bias_ic_get_i2c_num(unsigned char *buf);
extern int lcdkit_bias_ic_get_i2c_qup_id(unsigned char *buf);
char bias_ic_buf[BIAS_IC_CMDLINE_NAME_LEN] = " lcdbias_ic=default";

void lcdkit_bias_ic_save_name(char *pname)
{
    if(NULL == pname)
    {
        return;
    }
    memset(bias_ic_buf,0,sizeof(bias_ic_buf));
    snprintf(bias_ic_buf,BIAS_IC_CMDLINE_NAME_LEN," lcdbias_ic=%s",pname);
	return;
}


int lcdkit_bias_ic_i2c_read_u8(unsigned char chip_addr, unsigned char addr, unsigned char *buf)
{
    int ret = -EIO;
    struct i2c_msg msg_buf[] = {
        {chip_addr, I2C_M_WR, 1, &addr},
        {chip_addr, I2C_M_RD, 1, buf}
    };
    struct qup_i2c_dev *pi2c_dev = NULL;
    unsigned char i2c_num = 0;
	unsigned char qupid = 0;

	if(NULL == buf)
    {
        return ret;
    }
	
    ret = lckdit_bias_ic_get_i2c_num(&i2c_num);
    if(ret < 0)
    {
        return -1;
    }

    ret = lcdkit_bias_ic_get_i2c_qup_id(&qupid);
    if(ret < 0)
    {
        return -1;
    }	
    pi2c_dev = qup_blsp_i2c_init2(i2c_num, qupid, 400000, 19200000);
    if(NULL == pi2c_dev)
	{
		return ret;
	}

    ret = qup_i2c_xfer(pi2c_dev, msg_buf, 2);
    dprintf(CRITICAL, "lcd_i2c_read_u8  addr = 0x%x, data = 0x%x\n", addr, *buf);
    if(ret < 0)
    {
        dprintf(CRITICAL, "qup_i2c_xfer error %d addr = 0x%x, data = 0x%x \n", ret, addr, *buf);
    }
    qup_i2c_deinit2(pi2c_dev);

    return ret;
}

int lcdkit_bias_ic_i2c_write_u8(unsigned char chip_addr, unsigned char addr, unsigned char val)
{
    unsigned char data_buf[] = {addr, val};
    int ret = -EIO;
    struct i2c_msg msg_buf[] = {
        {chip_addr, I2C_M_WR, 2, data_buf}
    };
    struct qup_i2c_dev *pi2c_dev = NULL;
    unsigned char i2c_num = 0;
	unsigned char qupid = 0;

    ret = lckdit_bias_ic_get_i2c_num(&i2c_num);
    if(ret < 0)
    {
        return -1;
    }

    ret = lcdkit_bias_ic_get_i2c_qup_id(&qupid);
    if(ret < 0)
    {
        return -1;
    }

    pi2c_dev = qup_blsp_i2c_init2(i2c_num, qupid, 400000, 19200000);
    if(NULL == pi2c_dev)
	{
		return ret;
	}	
    ret = qup_i2c_xfer(pi2c_dev, msg_buf, 1);
    dprintf(CRITICAL, "lcd_i2c_write_u8 chip_addr is 0x%x addr = 0x%x,data = 0x%x\n", chip_addr, addr, val);
    if (ret < 0) 
    {
        dprintf(CRITICAL, "lcd_i2c_write_u8 write failed\n");
    }
    qup_i2c_deinit2(pi2c_dev);

    return ret;
}

