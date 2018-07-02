/*
 *  drivers/misc/inputhub/inputhub_route.h
 *  Sensor Hub Channel driver
 *
 *  Copyright (C) 2012 Huawei, Inc.
 *  Author: qindiwen <inputhub@huawei.com>
 *
 */
#ifndef	__SENSORS_H__
#define	__SENSORS_H__

#include "sensor_detect.h"

#define EXTEND_DATA_TYPE_IN_DTS_BYTE        0
#define EXTEND_DATA_TYPE_IN_DTS_HALF_WORD   1
#define EXTEND_DATA_TYPE_IN_DTS_WORD        2

#define HALL_COVERD     (1)

#define NV_READ_TAG	1
#define NV_WRITE_TAG	0
#define PS_CALIDATA_NV_NUM	334
#define PS_CALIDATA_NV_SIZE  12
#define ALS_CALIDATA_NV_NUM	339
#define ALS_CALIDATA_NV_SIZE  12
#define GYRO_CALIDATA_NV_NUM	341
#define GYRO_TEMP_CALI_NV_NUM	377
#define GYRO_CALIDATA_NV_SIZE  72
#define GYRO_TEMP_CALI_NV_SIZE  40
#define HANDPRESS_CALIDATA_NV_NUM  354
#define HANDPRESS_CALIDATA_NV_SIZE  24
#define AIRPRESS_CALIDATA_NV_NUM	332
#define AIRPRESS_CALIDATA_NV_SIZE  4
#define CAP_PROX_CALIDATA_NV_NUM  310
#define CAP_PROX_CALIDATA_NV_SIZE  28
#define pinhole_para_size (6)
#define TMD2745_PARA_SIZE (6)
#define RPR531_PARA_SIZE (12)
#define ACC_OFFSET_NV_NUM	307
#define ACC_OFFSET_NV_SIZE	(60)
#define MAG_CALIBRATE_DATA_NV_NUM 233
#define MAG_CALIBRATE_DATA_NV_SIZE (MAX_MAG_CALIBRATE_DATA_LENGTH)
#define MAG_AKM_CALIBRATE_DATA_NV_SIZE (MAX_MAG_AKM_CALIBRATE_DATA_LENGTH)

enum ALS_SENSNAME{
	APDS9922 = 1,
	LTR578 = 2,
};

typedef enum {
	RET_INIT = 0,
	SUC = 1,
	EXEC_FAIL,
	NV_FAIL,
	COMMU_FAIL,
	POSITION_FAIL,
	RET_TYPE_MAX
} RET_TYPE;

enum detect_state {
	DET_INIT = 0,
	DET_FAIL,
	DET_SUCC
};

struct sar_cap_proc_calibrate_data {
	uint16_t swap_flag[3];
	uint16_t cal_fact_base[3];
	uint16_t cal_offset[3];
	uint16_t digi_offset[3];
	uint16_t cap_prox_extend_data[2];
};

struct sar_cypress_calibrate_data {
	uint16_t sar_idac;
	uint16_t raw_data;
	uint16_t near_signaldata;
	uint16_t far_signaldata;
};

struct sar_semtech_calibrate_data {
	uint16_t offset;
	uint16_t diff;
};
union sar_calibrate_data {
	struct sar_cap_proc_calibrate_data cap_cali_data;
	struct sar_cypress_calibrate_data cypres_cali_data;
	struct sar_semtech_calibrate_data semtech_cali_data;
};

typedef struct _BH1745_ALS_PARA_TABLE {
	uint8_t phone_type;
	uint8_t phone_version;
	uint8_t tp_lcd_manufacture;
	uint8_t tp_color;
	s16 bh745_para[25];	/*give to bh1745 rgb sensor use,output lux and cct will use these para*/
} BH1745_ALS_PARA_TABLE;

typedef struct _APDS9251_ALS_PARA_TABLE {
	uint8_t phone_type;
	uint8_t phone_version;
	uint8_t tp_lcd_manufacture;
	uint8_t tp_color;
	s16 apds251_para[21];	/*give to apds251 rgb sensor use,output lux and cct will use these par*/
} APDS9251_ALS_PARA_TABLE;	/*the apds251_para size must small SENSOR_PLATFORM_EXTEND_DATA_SIZE*/

typedef struct _TMD3725_ALS_PARA_TABLE {
	uint8_t phone_type;
	uint8_t phone_version;
	uint8_t tp_lcd_manufacture;
	uint8_t tp_color;
	s16 tmd3725_para[19];/*give to tmd3725 rgb sensor use,output lux and cct will use these par*/
} TMD3725_ALS_PARA_TABLE;/*the apds251_para size must small SENSOR_PLATFORM_EXTEND_DATA_SIZE*/


typedef struct _PINHOLE_ALS_PARA_TABLE {
	uint8_t phone_type;
	uint8_t phone_version;
	uint8_t sens_name;
	uint8_t tp_manufacture;
	s16 pinhole_para[pinhole_para_size];/*modify the size of the array to pass more data */
} PINHOLE_ALS_PARA_TABLE;	/*the ph_para size must small SENSOR_PLATFORM_EXTEND_DATA_SIZE*/

typedef struct _TMD2745_ALS_PARA_TABLE {
	uint8_t phone_type;
	uint8_t phone_version;
	uint8_t tp_manufacture;
	uint8_t tp_color;
	s16 als_para[TMD2745_PARA_SIZE];/*modify the size of the array to pass more data */
} TMD2745_ALS_PARA_TABLE;           /*keep als_para size smaller than SENSOR_PLATFORM_EXTEND_DATA_SIZE*/

typedef struct _RPR531_ALS_PARA_TABLE{
	uint8_t phone_type;
	uint8_t phone_version;
	uint8_t tp_manufacture;
	s16 rpr531_para[RPR531_PARA_SIZE];
}RPR531_ALS_PARA_TABLE;

extern int fill_extend_data_in_dts(struct device_node *dn, const char *name, unsigned char *dest, size_t max_size, int flag);
extern int mcu_i2c_rw(uint8_t bus_num, uint8_t i2c_add, uint8_t *tx, uint32_t tx_len, uint8_t *rx_out, uint32_t rx_len);
extern int mcu_spi_rw(uint8_t bus_num, union SPI_CTRL ctrl, uint8_t *tx, uint32_t tx_len, uint8_t *rx_out, uint32_t rx_len);
extern int combo_bus_trans(struct sensor_combo_cfg *p_cfg, uint8_t *tx, uint32_t tx_len, uint8_t *rx_out, uint32_t rx_len);
extern void __dmd_log_report(int dmd_mark, const char *err_func, const char *err_msg);
extern int write_gyro_sensor_offset_to_nv(char *temp, int length);
extern int write_magsensor_calibrate_data_to_nv(char *src);

extern void reset_calibrate_data(void);
extern void reset_add_data(void);
extern int send_gsensor_calibrate_data_to_mcu(void);
extern int send_airpress_calibrate_data_to_mcu(void);
extern int send_gyro_calibrate_data_to_mcu(void);
extern int send_handpress_calibrate_data_to_mcu(void);
extern int mag_current_notify(void);
extern void read_tp_color_cmdline(void);
extern int write_calibrate_data_to_nv(int nv_number, int nv_size, char *nv_name, char *temp);
extern int write_gsensor_offset_to_nv(char *temp, int length);
extern int write_gyro_temperature_offset_to_nv(char *temp, int length);
#endif /* __SENSORS_H__ */
