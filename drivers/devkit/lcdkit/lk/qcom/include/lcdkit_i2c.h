#ifndef _LCDKIT_I2C_H_
#define _LCDKIT_I2C_H_

#include <debug.h>
#include <reg.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <blsp_qup.h>
#include <i2c_qup.h>
#include <qtimer.h>
#define LCDKIT_DEBUG_ERROR(exp, ...)  dprintf(CRITICAL, exp, ##__VA_ARGS__)
#define BIAS_IC_CMDLINE_NAME_LEN 64

int lcdkit_bias_ic_i2c_read_u8(unsigned char chip_addr, unsigned char addr, unsigned char *buf);
int lcdkit_bias_ic_i2c_write_u8(unsigned char chip_addr, unsigned char addr, unsigned char val);
void lcdkit_bias_ic_save_name(char *pname);
//extern void lcdkit_bias_vsp_vsn_enable(void);
extern void lcdkit_bias_vsp_vsn_disable(void);
extern char bias_ic_buf[BIAS_IC_CMDLINE_NAME_LEN];
#endif
