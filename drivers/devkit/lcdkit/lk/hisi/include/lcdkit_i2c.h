#ifndef _LCDKIT_I2C_H_
#define _LCDKIT_I2C_H_

#include <platform.h>
#include <debug.h>
#include <sys.h>
#include <module.h>
#include <i2c_ops.h>
#include <gpio.h>
#include <boot.h>
#include <dtimage_ops.h>
#include <fdt_ops.h>

#define LCDKIT_DEBUG_ERROR(msg, ...)  PRINT_ERROR(msg,## __VA_ARGS__)
int lcdkit_bias_ic_i2c_read_u8(unsigned char chip_addr, unsigned char addr, unsigned char *buf);
int lcdkit_bias_ic_i2c_write_u8(unsigned char chip_addr, unsigned char addr, unsigned char val);
int lcdkit_backlight_ic_i2c_read_u8(unsigned char chip_addr, unsigned char addr, unsigned char *buf);
int lcdkit_backlight_ic_i2c_write_u8(unsigned char chip_addr, unsigned char addr, unsigned char val);
#endif