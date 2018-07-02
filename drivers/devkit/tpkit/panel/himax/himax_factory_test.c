/* Himax Android Driver Sample Code for Himax chipset
*
* Copyright (C) 2016/12/09 Himax Corporation.
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#include "himax_ic.h"

#define HX_FAC_LOG_PRINT	//debug dmesg rawdata test switch
#define HIMAX_PROC_FACTORY_TEST_FILE	"ts/himax_threshold.csv" //"touchscreen/tp_capacitance_data"
#define ABS(x)	(((x) < 0) ? -(x) : (x))

#define RESULT_LEN  100

#define IIR_DIAG_COMMAND   1
#define DC_DIAG_COMMAND     2
#define BANK_DIAG_COMMAND   3
#define BASEC_GOLDENBC_DIAG_COMMAND  5
#define CLOSE_DIAG_COMMAND  0

#define IIR_CMD   1
#define DC_CMD        2
#define BANK_CMD   3
#define GOLDEN_BASEC_CMD  5
#define BASEC_CMD  7
#define RTX_DELTA_CMD  8

#define HX_RAW_DUMP_FILE "/data/hx_fac_dump.txt"

static void himax_free_Rawmem(void);
static int himax_ctoi(char *buf, uint32_t count);
static int himax_parse_threshold_file(void);
static int himax_get_one_value(const char *buf, uint32_t *offset);
static int himax_parse_threshold_file_method(const char *buf, uint32_t file_size);
static int himax_enter_iq_mode(void);
static int himax_write_iq_criteria(void);
static int himax_exit_iq_self_mode(void);
static int himax_iq_self_test(void);
static int himax_self_test(void);
static int read_rawdata_sram(void);

/* ASCII */
#define ASCII_LF (0x0A)
#define ASCII_CR (0x0D)
#define ASCII_COMMA (0x2C)
#define ASCII_ZERO (0x30)
#define ASCII_NINE (0x39)
#define GOLDEN_DATA_NUM 1024
#define HX_CRITERIA_CNT	20
#define HX_CRITERIA_FAC_CNT 14
#define HX_CRITERIA_IQ_CNT 6
#define MAX_CASE_CNT 3
#define HX_CRITERIA 0
#define DC_GOLDEN_LIMIT 1
#define BANK_GOLDEN_LIMIT 2
#define FACTORY_TEST_START_LABEL  "CRITERIA_SELF_BANK_UP"

static uint8_t *fac_dump_buffer = NULL;
static const struct firmware *fw_entry;

int index_count = 0;

uint8_t hx_criteria [HX_CRITERIA_CNT]=  {0};
uint8_t dc_golden_data[GOLDEN_DATA_NUM] = {0};
uint8_t bank_golden_data[GOLDEN_DATA_NUM] = {0};

static char buf_test_result[RESULT_LEN] = { 0 };	/*store mmi test result*/

atomic_t hmx_mmi_test_status = ATOMIC_INIT(0);
#define HX_RAW_DATA_SIZE   (PAGE_SIZE * 60)

struct ts_rawdata_info *info = NULL;
extern struct himax_ts_data *g_himax_ts_data;

//{self_bank_up_limit,self_bank_down_limit,mutual_bank_up_limit,mutual_bank_down_limit,self_DC_up_limit,self_DC_down_limit,mutual_DC_up_limit,mutual_DC_down_limit,self_baseC_dev,mutual_baseC_dev,self_IIR_up_limit,multual_IIR_up_limit}

static uint8_t *mutual_iir 	= NULL;
static uint8_t *self_iir 	= NULL;

static uint8_t *tx_delta 	= NULL;
static uint8_t *rx_delta 	= NULL;

static uint8_t *mutual_bank	= NULL;
static uint8_t *self_bank 	= NULL;

static uint8_t *mutual_dc 	= NULL;
static uint8_t *self_dc 	= NULL;

static uint8_t *mutual_basec = NULL;
static uint8_t *self_basec 	= NULL;

static uint8_t *mutual_golden_basec	= NULL;
static uint8_t *self_golden_basec	= NULL;

static int current_index = 0;

static int g_hx_self_test_result = 0;

static uint8_t *mutual_tmp = NULL;
static uint8_t *self_tmp = NULL;

static uint8_t *rawdata_temp = NULL;

enum hx_test_item
{
	test0 = 0,
	test1,
	test2,
	test3,
	test4,
	test5,
	test6,
	test7,

};
enum hx_limit_index
{
	bank_self_up = 0,
	bank_self_down,
	bank_mutual_up,
	bank_mutual_down,
	dc_self_up,
	dc_self_down,
	dc_mutual_up,
	dc_mutual_down,
	basec_self,
	basec_mutual,
	iir_self,
	iir_mutual,
	delta_up,
	delta_down,

};
char hx_result_fail_str[4] = {0};
char hx_result_pass_str[4] = {0};
static int hx_result_status[8] = {0};

 /*0:bank ,2: iir, 4:basec, 6:dc, 8:golden basec*/
void himax_fac_dump(uint16_t raw_data_step,uint16_t mutual_num,uint16_t self_num,uint8_t *mutual,uint8_t *self)
{
	uint16_t raw_dump_addr = 0;

	raw_dump_addr= (mutual_num+self_num)*raw_data_step;
	TS_LOG_INFO("%s:raw_dump_addr =%d\n", __func__,raw_dump_addr);
	memcpy(fac_dump_buffer+raw_dump_addr, mutual, mutual_num);
	raw_dump_addr += mutual_num;
	TS_LOG_INFO("%s:raw_dump_addr =%d\n", __func__,raw_dump_addr);
	memcpy(fac_dump_buffer+raw_dump_addr, self, self_num);
}

static int himax_alloc_Rawmem(void)
{
	uint16_t self_num = 0;
	uint16_t mutual_num = 0;
	uint16_t tx_delta_num = 0;
	uint16_t rx_delta_num = 0;
	uint16_t rx = getXChannel();
	uint16_t tx = getYChannel();

	mutual_num	= rx * tx;
	self_num	= rx + tx;
	tx_delta_num = rx * (tx - 1);
	rx_delta_num = (rx - 1) * tx;

	mutual_tmp = kzalloc((mutual_num)*sizeof(uint8_t),GFP_KERNEL);
	if (mutual_tmp == NULL) {
		TS_LOG_ERR("%s:mutual_tmp is NULL\n", __func__);
		goto exit_mutual_tmp;
	}

	self_tmp = kzalloc((self_num)*sizeof(uint8_t),GFP_KERNEL);
	if (self_tmp == NULL) {
		TS_LOG_ERR("%s:self_tmp is NULL\n", __func__);
		goto exit_self_tmp;
	}

	mutual_bank = kzalloc(mutual_num * sizeof(uint8_t), GFP_KERNEL);
	if (mutual_bank == NULL) {
		TS_LOG_ERR("%s:mutual_bank is NULL\n", __func__);
		goto exit_mutual_bank;
	}

	self_bank = kzalloc(self_num * sizeof(uint8_t), GFP_KERNEL);
	if (self_bank == NULL) {
		TS_LOG_ERR("%s:self_bank is NULL\n", __func__);
		goto exit_self_bank;
	}

	mutual_dc = kzalloc(mutual_num * sizeof(uint8_t), GFP_KERNEL);
	if (mutual_dc == NULL) {
		TS_LOG_ERR("%s:mutual_dc is NULL\n", __func__);
		goto exit_mutual_dc;
	}

	self_dc = kzalloc(self_num * sizeof(uint8_t), GFP_KERNEL);
	if (self_dc == NULL) {
		TS_LOG_ERR("%s:self_dc is NULL\n", __func__);
		goto exit_self_dc;
	}

	tx_delta = kzalloc(tx_delta_num * sizeof(uint8_t), GFP_KERNEL);
	if (tx_delta == NULL) {
		TS_LOG_ERR("%s:tx_delta is NULL\n", __func__);
		goto exit_tx_delta;
	}

	rx_delta = kzalloc(rx_delta_num * sizeof(uint8_t), GFP_KERNEL);
	if (rx_delta == NULL) {
		TS_LOG_ERR("%s:rx_delta is NULL\n", __func__);
		goto exit_rx_delta;
	}
	mutual_basec = kzalloc(mutual_num * sizeof(uint8_t), GFP_KERNEL);
	if (mutual_basec == NULL) {
		TS_LOG_ERR("%s:mutual_basec is NULL\n", __func__);
		goto exit_mutual_basec;
	}
	self_basec = kzalloc(self_num * sizeof(uint8_t), GFP_KERNEL);
	if (self_basec == NULL) {
		TS_LOG_ERR("%s:self_basec is NULL\n", __func__);
		goto exit_self_basec;
	}

	mutual_golden_basec = kzalloc(mutual_num * sizeof(uint8_t), GFP_KERNEL);
	if (mutual_golden_basec == NULL) {
		TS_LOG_ERR("%s:mutual_golden_basec is NULL\n", __func__);
		goto exit_mutual_golden_basec;
	}
	self_golden_basec = kzalloc(self_num * sizeof(uint8_t), GFP_KERNEL);
	if (self_golden_basec == NULL) {
		TS_LOG_ERR("%s:self_golden_basec is NULL\n", __func__);
		goto exit_self_golden_basec;
	}

	mutual_iir = kzalloc(mutual_num * sizeof(uint8_t), GFP_KERNEL);
	if (mutual_iir == NULL) {
		TS_LOG_ERR("%s:mutual_iir is NULL\n", __func__);
		goto exit_mutual_iir;
	}
	self_iir = kzalloc(self_num * sizeof(uint8_t), GFP_KERNEL);
	if (self_iir == NULL) {
		TS_LOG_ERR("%s:self_iir is NULL\n", __func__);
		goto exit_self_iir;
	}

	memset(mutual_bank, 0xFF, mutual_num * sizeof(uint8_t));
	memset(self_bank, 0xFF, self_num * sizeof(uint8_t));
	memset(mutual_dc, 0xFF, mutual_num * sizeof(uint8_t));
	memset(self_dc, 0xFF, self_num * sizeof(uint8_t));
	memset(tx_delta, 0xFF, tx_delta_num * sizeof(uint8_t));
	memset(rx_delta, 0xFF, rx_delta_num * sizeof(uint8_t));

	memset(mutual_basec, 0xFF, mutual_num * sizeof(uint8_t));
	memset(self_basec, 0xFF, self_num * sizeof(uint8_t));
	memset(mutual_golden_basec, 0xFF, mutual_num * sizeof(uint8_t));
	memset(self_golden_basec, 0xFF, self_num * sizeof(uint8_t));

	memset(mutual_iir, 0xFF, mutual_num * sizeof(uint8_t));
	memset(self_iir, 0xFF, self_num * sizeof(uint8_t));

	return NO_ERR;
exit_self_iir:
	kfree(mutual_iir);
	mutual_iir = NULL;
exit_mutual_iir:
	kfree(self_golden_basec);
	self_golden_basec = NULL;
exit_self_golden_basec:
	kfree(mutual_golden_basec);
	mutual_golden_basec = NULL;
exit_mutual_golden_basec:
	kfree(self_basec);
	self_basec = NULL;
exit_self_basec:
	kfree(mutual_basec);
	mutual_basec = NULL;
exit_mutual_basec:
	kfree(rx_delta);
	rx_delta = NULL;
exit_rx_delta:
	kfree(tx_delta);
	tx_delta = NULL;
exit_tx_delta:
	kfree(self_dc);
	self_dc = NULL;
exit_self_dc:
	kfree(mutual_dc);
	mutual_dc = NULL;
exit_mutual_dc:
	kfree(self_bank);
	self_bank = NULL;
exit_self_bank:
	kfree(mutual_bank);
	mutual_bank = NULL;
exit_mutual_bank:
	kfree(self_tmp);
	self_tmp = NULL;
exit_self_tmp:
	kfree(mutual_tmp);
	mutual_tmp = NULL;
exit_mutual_tmp:
	return ALLOC_FAIL;
}

static void himax_free_Rawmem(void)
{
	kfree(mutual_bank);
	kfree(self_bank);
	kfree(mutual_dc);
	kfree(self_dc);
	kfree(tx_delta);
	kfree(rx_delta);
	kfree(mutual_basec);
	kfree(self_basec);
	kfree(mutual_golden_basec);
	kfree(self_golden_basec);
	kfree(mutual_iir);
	kfree(self_iir);

	mutual_bank			= NULL;
	self_bank			= NULL;
	mutual_dc			= NULL;
	self_dc				= NULL;
	tx_delta			= NULL;
	rx_delta			= NULL;
	mutual_basec		= NULL;
	self_basec			= NULL;
	mutual_golden_basec	= NULL;
	self_golden_basec	= NULL;
	mutual_iir			= NULL;
	self_iir			= NULL;
}
/*data write format:
8C 11   ----open to write data
8B data_address
40 data
8C 00  ----close to write data
*/
int himax_wirte_golden_data(void)
{
	int retval = -1;
	int i=0, j=0;
	int num_data = 0;
	int addr_start = 0x0160;//write golden value to flash start addr
	int remain_data_num = 0;
	uint8_t write_times = 0;
	uint8_t cmdbuf[4]  = {0};
	uint16_t x_channel = getXChannel();
	uint16_t y_channel = getYChannel();
	int m = 0;
	num_data = x_channel*y_channel + x_channel + y_channel;
	TS_LOG_INFO("Number of data = %d\n",num_data);
	if (num_data%128)
		write_times = (num_data/128) + 1;
	else
		write_times = num_data/128;

	TS_LOG_INFO("Wirte Golden data - Start \n");

	//Wirte Golden data - Start
	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_EN;
	retval = i2c_himax_write( HX_REG_SRAM_SWITCH, &cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_wirte_golden_data write 0x8C error!\n");
		return retval;
	}
	msleep(HX_SLEEP_10MS);

	for (j = 0; j < 2; j++){
		remain_data_num = 0;
		for (i = 0; i < write_times; i++) {
			cmdbuf[0] = ((addr_start & 0xFF00) >> 8);
			cmdbuf[1] = addr_start & 0x00FF;
			remain_data_num = num_data - i*128;

			retval = i2c_himax_write(HX_REG_SRAM_ADDR, &cmdbuf[0], 2, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
			if( retval != 0 ){
				TS_LOG_ERR("himax_wirte_golden_data write 0x8B error!\n");
				return retval;
			}

			msleep(HX_SLEEP_10MS);
			if(j == 0){
				if(remain_data_num >= 128){
					m = i*128;
					retval = i2c_himax_write( HX_REG_FLASH_WPLACE, &dc_golden_data[m], 128, sizeof(dc_golden_data), DEFAULT_RETRY_CNT);
					if( retval != 0 ){
						TS_LOG_ERR("himax_wirte_golden_data write 0x40 error!\n");
						return retval;
					}
					addr_start = addr_start + 128;
				} else {
					m = i*128;
					retval = i2c_himax_write( HX_REG_FLASH_WPLACE, &dc_golden_data[m], remain_data_num, sizeof(dc_golden_data), DEFAULT_RETRY_CNT);
					if( retval != 0 ){
						TS_LOG_ERR("himax_wirte_golden_data write 0x40 error!\n");
						return retval;
					}
					addr_start = addr_start + remain_data_num;
				}
				msleep(HX_SLEEP_10MS);
			}else if(j == 1){
				if(remain_data_num >= 128){
					m = i*128;
					retval = i2c_himax_write(HX_REG_FLASH_WPLACE, &bank_golden_data[m], 128, sizeof(bank_golden_data), DEFAULT_RETRY_CNT);
					if( retval != 0 ){
						TS_LOG_ERR("himax_wirte_golden_data write 0x40 > 128 error!\n");
						return retval;
					}
					addr_start = addr_start + 128;
				} else {
					m = i*128;
					retval = i2c_himax_write(HX_REG_FLASH_WPLACE, &bank_golden_data[m], remain_data_num, sizeof(bank_golden_data), DEFAULT_RETRY_CNT);
					if( retval != 0 ){
						TS_LOG_ERR("himax_wirte_golden_data write 0x40 < 128 error!\n");
						return retval;
					}
					addr_start = addr_start + remain_data_num;
				}
				msleep(HX_SLEEP_10MS);
			}
		}
	}

	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_DISABLE;
	retval = i2c_himax_write(HX_REG_SRAM_SWITCH, &cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if( retval != 0 ){
		TS_LOG_ERR("himax_wirte_golden_data write 0x8C error!\n");
		return retval;
	}
	//Write Golden data - End
	return retval;
}

/*data write format:
8C 11   ----open to write data
8B data_address
40 data
8C 00  ----close to write data
*/
int himax_wirte_criteria_data(void)
{
	int i =0;
	int retval = -1;
	int addr_criteria = 0x98;
	uint8_t cmdbuf[4] = {0};

	for(i=0;i<14;i++) {
		TS_LOG_INFO("[Himax]: self test hx_criteria is  data[%d] = %d\n", i, hx_criteria[i]);
	}

	//Write Criteria
	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_EN;
	retval = i2c_himax_write(HX_REG_SRAM_SWITCH, &cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if( retval != 0 ){
		TS_LOG_ERR("himax_wirte_criteria_data write 0x8C error!\n");
		return retval;
	}
	msleep(HX_SLEEP_10MS);

	cmdbuf[0] = 0x00;
	cmdbuf[1] = addr_criteria;
	retval = i2c_himax_write(HX_REG_SRAM_ADDR, &cmdbuf[0], 2, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if( retval != 0 ){
		TS_LOG_ERR("himax_wirte_criteria_data write 0x8B error!\n");
		return retval;
	}

	msleep(HX_SLEEP_10MS);
	retval = i2c_himax_write( HX_REG_FLASH_WPLACE, &hx_criteria[0], 12, sizeof(hx_criteria), DEFAULT_RETRY_CNT);
	if( retval != 0 ){
		TS_LOG_ERR("himax_wirte_criteria_data write 0x40 error!\n");
		return retval;
	}

	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_DISABLE;
	retval = i2c_himax_write(HX_REG_SRAM_SWITCH, &cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if( retval != 0 ){
		TS_LOG_ERR("himax_wirte_criteria_data write 0x8C error!\n");
		return retval;
	}
	return retval;
}

/*data read format:
8C 11   ----open to write data
8B data_address
5a data
8C 00  ----close to write data
*/
int himax_read_result_data(void)
{
	int i=0;
	int retval = -1;
	int addr_result = 0x96;
	uint8_t cmdbuf[4];
	uint8_t databuf[10];

	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_EN;
	retval = i2c_himax_write(HX_REG_SRAM_SWITCH, &cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);//read result
	if( retval != 0 ){
		TS_LOG_ERR("himax_read_result_data write 0x8C error!\n");
		return retval;
	}
	msleep(HX_SLEEP_10MS);

	cmdbuf[0] = 0x00;
	cmdbuf[1] = addr_result;

	retval = i2c_himax_write(HX_REG_SRAM_ADDR ,&cmdbuf[0], 2, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if( retval != 0 ){
		TS_LOG_ERR("himax_read_result_data write 0x8B error!\n");
		return retval;
	}
	msleep(HX_SLEEP_10MS);

	retval = i2c_himax_read( HX_REG_FLASH_RPLACE, databuf, 9, sizeof(databuf), DEFAULT_RETRY_CNT);
	if( retval != 0 ){
		TS_LOG_ERR("himax_read_result_data write 0x5A error!\n");
		return retval;
	}

	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_DISABLE;
	retval = i2c_himax_write( HX_REG_SRAM_SWITCH ,&cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if( retval != 0 ){
		TS_LOG_ERR("himax_read_result_data write 0x8C error!\n");
		return retval;
	}

	for(i=0;i<9;i++) {
		TS_LOG_INFO("[Himax]: After self test %X databuf[%d] = 0x%x\n", addr_result, i, databuf[i]);
	}
	/*databuf [0] 0xAA self test open short pass*/
	if (databuf[0]==0xAA) {
		TS_LOG_INFO("[Himax]: self-test pass\n");
		return SELF_TEST_PASS;
	} else if ((databuf[0]&0xF)==0) {
		TS_LOG_INFO("[Himax]: self-test error\n");
		#ifdef CONFIG_HUAWEI_DSM
			hmx_tp_report_dsm_err(DSM_TP_RAWDATA_ERROR_NO , databuf[0]);
		#endif
		return SELF_TEST_FAIL;
	} else {
		#ifdef CONFIG_HUAWEI_DSM
			hmx_tp_report_dsm_err(DSM_TP_RAWDATA_ERROR_NO , databuf[0]);
		#endif
		TS_LOG_ERR("[Himax]: self-test fail:4F\n");
		return SELF_TEST_FAIL;
	}
}

int himax_bank_test(int step) //for Rawdara
{
	int result = NO_ERR;
	int rx = getXChannel();
	int tx = getYChannel();
	int index1 = 0;
#ifdef HX_FAC_LOG_PRINT
	int m = 0;
#endif
	TS_LOG_INFO("%s: Entering\n",__func__);

	TS_LOG_INFO("Bank Start:\n");
	for(index1=0;index1<rx *tx;index1++)
	{

		if(mutual_bank[index1] < hx_criteria[bank_mutual_down] || mutual_bank[index1] > hx_criteria[bank_mutual_up])
			result=-1;
	}

	for(index1=0;index1<rx +tx;index1++)
	{
		if(self_bank[index1] < hx_criteria[bank_self_down] || self_bank[index1] > hx_criteria[bank_self_up])
			result=-1;
	}
#ifdef HX_FAC_LOG_PRINT
	/*=====debug log======*/
	for (index1 = 0; index1 < rx *tx; index1++) {
		printk("%4d", mutual_bank[index1]);
		if ((index1 % rx) == (rx - 1))
		{
			m = rx + index1/rx;
			printk(" %3d\n", self_bank[m]);
		}
	}
	printk("\n");
	for (index1 = 0; index1 < rx; index1++) {
		printk("%4d", self_bank[index1]);
		if (((index1) % rx) == (rx- 1))
		printk("\n");
	}
	/*=====debug log=====*/
#endif
	TS_LOG_INFO("Bank End\n");

	if(result==0 && hx_result_status[test0] == 0)
	{
		hx_result_pass_str[0] = '0'+step;
		strncat(buf_test_result, hx_result_pass_str ,strlen(hx_result_pass_str)+1);
		TS_LOG_INFO("%s: End --> PASS\n",__func__);
	}
	else
	{
		hx_result_fail_str[0] = '0'+step;
		strncat(buf_test_result, hx_result_fail_str,strlen(hx_result_fail_str)+1);
		TS_LOG_INFO("%s: End --> FAIL\n",__func__);
	}

	return result;
}

int himax_self_delta_test(int step)
{
	int index1 = 0;
	int index2 = 0;
	int result = NO_ERR;
	int tx = getYChannel();
	int rx = getXChannel();
	uint16_t tx_delta_num = 0;
	uint16_t rx_delta_num = 0;
	int m = 0;
	tx_delta_num = rx*(tx-1);
	rx_delta_num = (rx-1)*tx;

	/*TX Delta*/
	TS_LOG_INFO("TX Delta Start:\n");

	for(index1 = 0;index1<tx_delta_num;index1++)
	{
		m = index1+rx;
		tx_delta[index1] = ABS(mutual_bank[m] - mutual_bank[index1]);
		if(tx_delta[index1] > hx_criteria[delta_up])
			result=-1;
	}
	TS_LOG_INFO("TX Delta End\n");

	/*RX Delta*/
	TS_LOG_INFO("TX RX Delta Start:\n");
	/*lint -save -e* */
	for(index1 = 1;index2<rx_delta_num;index1++)
	{
		if(index1%(rx)==0)
			continue;
		rx_delta[index2] = ABS(mutual_bank[index1] - mutual_bank[index1-1]);
		if(rx_delta[index2] > hx_criteria[delta_up])
			result=-1;
		index2++;
	}
	/*lint -restore*/
#ifdef HX_FAC_LOG_PRINT
	//=====debug log======
	printk("TX start\n");
	for (index1 = 0; index1 < tx_delta_num; index1++) {
		printk("%4d", tx_delta[index1]);
		if ((index1 % (rx)) == (rx - 1))
			printk("\n");
	}
	printk("RX start\n");
	for (index1 = 0; index1 < rx_delta_num; index1++) {
		printk("%4d", rx_delta[index1]);
		if (((index1) % (rx-1)) == (rx - 2))
			printk("\n");
	}
	//=====debug log=====
#endif

	TS_LOG_INFO("TX RX Delta End\n");

	if(result==0)
	{
		hx_result_pass_str[0] = '0'+step;
		strncat(buf_test_result, hx_result_pass_str ,strlen(hx_result_pass_str)+1);
		TS_LOG_INFO("%s: End --> PASS\n",__func__);
	}
	else
	{
		hx_result_fail_str[0] = '0'+step;
		strncat(buf_test_result, hx_result_fail_str,strlen(hx_result_fail_str)+1);
		TS_LOG_INFO("%s: End --> FAIL\n",__func__);
	}

	return result;
}

int himax_iir_test(int step) //for Noise Delta
{
	int index1 = 0;
	int result = NO_ERR;
	int tx = getYChannel();
	int rx = getXChannel();
#ifdef HX_FAC_LOG_PRINT
	int m=0;
#endif
	TS_LOG_INFO("%s: Entering\n",__func__);

	TS_LOG_INFO("IIR  Start:\n");
#ifdef HX_FAC_LOG_PRINT
	//=====debug log======
	for (index1 = 0; index1 < rx *tx; index1++) {
		printk("%4d", mutual_iir[index1]);
		if ((index1 % rx) == (rx - 1))
		{
			m=rx + index1/rx;
			printk(" %3d\n", self_iir[m]);
		}
	}
	printk("\n");
	for (index1 = 0; index1 < rx; index1++) {
		printk("%4d", self_iir[index1]);
		if (((index1) % rx) == (rx - 1))
		printk("\n");
	}
	//=====debug log=====
#endif
	for(index1=0;index1<rx*tx;index1++)
	{
		if(mutual_iir[index1] > hx_criteria[iir_mutual])
			result=-1;
	}
	for(index1=0;index1<(rx+tx);index1++)
	{
		// There is no necessary for Huwawei default printer
		if(self_iir[index1] > hx_criteria[iir_self])
			result=-1;
	}

	TS_LOG_INFO("IIR  End\n");

	if(result==0 && hx_result_status[test0] == 0)
	{
		hx_result_pass_str[0] = '0'+step;
		strncat(buf_test_result, hx_result_pass_str ,strlen(hx_result_pass_str)+1);
		TS_LOG_INFO("%s: End --> PASS\n",__func__);
	}
	else
	{
		hx_result_fail_str[0] = '0'+step;
		strncat(buf_test_result, hx_result_fail_str,strlen(hx_result_fail_str)+1);
		TS_LOG_INFO("%s: End --> FAIL\n",__func__);
	}

	return result;

}

int himax_basec_test(int step) //for open/short
{
	int index1 = 0;
	int retval = NO_ERR;
	int rx = getXChannel();
	int tx = getYChannel();
#ifdef HX_FAC_LOG_PRINT
	int m=0;
#endif
	if(hx_result_status[test0] == 0)
	{
		hx_result_pass_str[0] = '0'+step;
		strncat(buf_test_result, hx_result_pass_str ,strlen(hx_result_pass_str)+1);
		TS_LOG_INFO("%s: End --> PASS\n",__func__);
	}
	else
	{
		hx_result_fail_str[0] = '0'+step;
		strncat(buf_test_result, hx_result_fail_str,strlen(hx_result_fail_str)+1);
		TS_LOG_INFO("%s: End --> FAIL\n",__func__);
	}
#ifdef HX_FAC_LOG_PRINT
	//=====debug log======
	printk("DC start:\n");
	for (index1 = 0; index1 < rx *tx; index1++) {
		printk("%4d", mutual_dc[index1]);
		if ((index1 % rx) == (rx - 1))
		{
			m=rx + index1/rx;
			printk(" %3d\n", self_dc[m]);
		}
	}
	printk("\n");
	for (index1 = 0; index1 < rx; index1++) {
		printk("%4d", self_dc[index1]);
		if (((index1) % rx) == (rx- 1))
		printk("\n");
	}
	printk("DC end:\n");
	//=====debug log=====
	//=====debug log======
	printk("BaseC start:\n");
	for (index1 = 0; index1 < rx *tx; index1++) {
		printk("%4d", mutual_basec[index1]);
		if ((index1 % rx) == (rx - 1))
		{
			m=rx + index1/rx;
			printk(" %3d\n", self_basec[m]);
		}
	}
	printk("\n");
	for (index1 = 0; index1 < rx; index1++) {
		printk("%4d", self_basec[index1]);
		if (((index1) % rx) == (rx- 1))
		printk("\n");
	}
	printk("BaseC end:\n");
	//=====debug log=====
	//=====debug log======
	printk("Golden BaseC start:\n");
	for (index1 = 0; index1 < rx *tx; index1++) {
		printk("%4d", mutual_golden_basec[index1]);
		if ((index1 % rx) == (rx - 1))
		{
			m=rx + index1/rx;
			printk(" %3d\n", self_golden_basec[m]);
		}
	}
	printk("\n");
	for (index1 = 0; index1 < rx; index1++) {
		printk("%4d", self_golden_basec[index1]);
		if (((index1) % rx) == (rx- 1))
		printk("\n");
	}
	printk("Golden BaseC end\n");
	//=====debug log=====
#endif

	return retval;
}

void himax_print_rawdata(int mode)
{
	int index1=0;
	uint16_t self_num = 0;
	uint16_t mutual_num = 0;
	uint16_t x_channel = getXChannel();
	uint16_t y_channel = getYChannel();
	uint8_t self_buff[128]  = {0};
	uint8_t mutual_buff[40*40] = {0};

	switch(mode)
	{
		case BANK_CMD:
			mutual_num	= x_channel * y_channel;
			self_num	= x_channel + y_channel;
			memcpy(mutual_buff, mutual_bank, mutual_num);
			memcpy(self_buff, self_bank, self_num);
			break;
		case RTX_DELTA_CMD:
			mutual_num	= x_channel*(y_channel-1);
			self_num	= (x_channel-1)*y_channel;
			memcpy(mutual_buff, tx_delta, mutual_num);
			memcpy(self_buff, rx_delta, self_num);
			break;
		case IIR_CMD:
			mutual_num	= x_channel * y_channel;
			self_num	= x_channel + y_channel;
			memcpy(mutual_buff, mutual_iir, mutual_num);
			memcpy(self_buff, self_iir, self_num);
			break;
	}

	for(index1=0;index1<mutual_num;index1++)
	{
		info->buff[current_index++] = mutual_buff[index1];
	}

	for(index1=0;index1<self_num;index1++)
	{
		info->buff[current_index++] = self_buff[index1];
	}
}

static int read_rawdata_sram(void)
{
	uint16_t self_num = 0;
	uint16_t mutual_num = 0;
	uint16_t rx =0;
	uint16_t tx =0;
	uint16_t all_rawdata_size = 0;

	int retval = NO_ERR;

	uint8_t data[4] = {0};

	rx = getXChannel();
	tx = getYChannel();

	mutual_num	= rx * tx;
	self_num	= rx + tx;

	all_rawdata_size = mutual_num + self_num;

	rawdata_temp = kzalloc((all_rawdata_size)*sizeof(uint8_t),GFP_KERNEL);
	if (rawdata_temp == NULL) {
		TS_LOG_ERR("%s:rawdata_temp is NULL\n", __func__);
		goto exit_rawdata_temp;
	}

	//change to sram test
	data[0] = HX_REG_SRAM_SWITCH;
	data[1] = HX_REG_SRAM_TEST_MODE_EN;
	retval = i2c_himax_master_write(&data[0],2,sizeof(data),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_10MS);
	if(retval < 0)
		goto i2c_fail;

	//read sram for bank
	data[0] = HX_REG_SRAM_ADDR;
	/*sram bank data start addr*/
	data[1] = 0x0B;
	data[2] = 0x4C;
	retval = i2c_himax_master_write(&data[0],3,sizeof(data),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_10MS);
	if(retval < 0)
		goto i2c_fail;
	retval = i2c_himax_read(HX_REG_FLASH_RPLACE,rawdata_temp,all_rawdata_size,all_rawdata_size*sizeof(uint8_t),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_100MS);
	if(retval < 0)
	goto i2c_fail;


	memcpy(mutual_bank,&rawdata_temp[0],mutual_num);
	memcpy(self_bank,&rawdata_temp[mutual_num],self_num);

	//read sram for IIR
	data[0] = HX_REG_SRAM_ADDR;
	/*sram IIR data start addr*/
	data[1] = 0x10;
	data[2] = 0x46;

	retval = i2c_himax_master_write(&data[0],3,sizeof(data),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_10MS);
	if(retval < 0)
		goto i2c_fail;

	retval = i2c_himax_read(HX_REG_FLASH_RPLACE,rawdata_temp,all_rawdata_size,all_rawdata_size*sizeof(uint8_t),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_100MS);
	if(retval < 0)
		goto i2c_fail;

	memcpy(mutual_iir,&rawdata_temp[0],mutual_num);
	memcpy(self_iir,&rawdata_temp[mutual_num],self_num);

	//read sram for DC
	data[0] = HX_REG_SRAM_ADDR;
	/*sramDC data start addr*/
	data[1] = 0x15;
	data[2] = 0x40;
	retval = i2c_himax_master_write( &data[0],3,sizeof(data),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_10MS);
	if(retval < 0)
		goto i2c_fail;

	retval = i2c_himax_read(HX_REG_FLASH_RPLACE,rawdata_temp,all_rawdata_size,all_rawdata_size*sizeof(uint8_t),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_100MS);
	if(retval < 0)
		goto i2c_fail;

	memcpy(mutual_dc,&rawdata_temp[0],mutual_num);
	memcpy(self_dc,&rawdata_temp[mutual_num],self_num);

	//read sram for BaseC
	data[0] = HX_REG_SRAM_ADDR;
	/*sram BaseC data start addr*/
	data[1] = 0x1A;
	data[2] = 0x3A;
	retval = i2c_himax_master_write(&data[0],3,sizeof(data),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_10MS);
	if(retval < 0)
		goto i2c_fail;

	retval = i2c_himax_read(HX_REG_FLASH_RPLACE,rawdata_temp,all_rawdata_size,all_rawdata_size*sizeof(uint8_t),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_100MS);
	if(retval < 0)
		goto i2c_fail;

	memcpy(mutual_basec,&rawdata_temp[0],mutual_num);
	memcpy(self_basec,&rawdata_temp[mutual_num],self_num);

	//read sram for GoldenBaseC
	data[0] = HX_REG_SRAM_ADDR;
	/*sram GoldebBaseC data start addr*/
	data[1] = 0x1F;
	data[2] = 0x34;
	retval = i2c_himax_master_write(&data[0],3,sizeof(data),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_10MS);
	if(retval < 0)
		goto i2c_fail;

	retval = i2c_himax_read(HX_REG_FLASH_RPLACE,rawdata_temp,all_rawdata_size,all_rawdata_size*sizeof(uint8_t),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_100MS);
	if(retval < 0)
		goto i2c_fail;

	memcpy(mutual_golden_basec,&rawdata_temp[0],mutual_num);
	memcpy(self_golden_basec,&rawdata_temp[mutual_num],self_num);

	//close sram test
	data[0] = HX_REG_SRAM_SWITCH;
	data[1] = HX_REG_SRAM_TEST_MODE_DISABLE;
	retval = i2c_himax_master_write(&data[0],2,sizeof(data),DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_10MS);
	if(retval < 0)
		goto i2c_fail;
	retval = i2c_himax_write_command(HX_CMD_TSSON,DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_30MS);
	if(retval < 0)
		goto i2c_fail;
	retval = i2c_himax_write_command(HX_CMD_TSSLPOUT,DEFAULT_RETRY_CNT);
	msleep(HX_SLEEP_30MS);
	if(retval < 0)
		goto i2c_fail;

	return NO_ERR;

i2c_fail:
	kfree(rawdata_temp);
	rawdata_temp = NULL;
	TS_LOG_ERR("%s: Exit because there is I2C fail.\n",__func__);
	himax_free_Rawmem();
	return I2C_WORK_ERR;
exit_rawdata_temp:
	return ALLOC_FAIL;
}
static int himax_enter_iq_mode(void)
{
	int retval = 0;
	uint8_t cmdbuf[IQ_CMDBUF_LEN] = {0};
	TS_LOG_INFO("%s: start \n", __func__);
	//close IC adc
	memset(cmdbuf, 0x00, sizeof(cmdbuf));
	retval = i2c_himax_write( HX_CMD_TSSOFF,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_CMD_TSSOFF error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_120MS);
	retval = i2c_himax_write(HX_CMD_TSSLPIN,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_CMD_TSSLPIN error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_120MS);
	//open sorting mode
	cmdbuf[0] = 0x40;//open sorting mode cmd
	retval = i2c_himax_write( HX_REG_FLASH_MODE, &cmdbuf[0], ONEBYTE, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval <0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_FLASH_MODE error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_120MS);
	//enable sram mode
	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_EN;
	retval = i2c_himax_write( HX_REG_SRAM_SWITCH, &cmdbuf[0], ONEBYTE, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_SRAM_SWITCH error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_10MS);
	//write criteria to sram addr
	cmdbuf[0] = 0x00;
	cmdbuf[1] = HX_CMD_ADDR_CRITERIA;
	retval = i2c_himax_write(HX_REG_SRAM_ADDR, &cmdbuf[0], IQ_SRAM_CMD_LEN, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_SRAM_ADDR error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_10MS);
	TS_LOG_INFO("%s: end \n", __func__);

	return HX_OK;
}
static int himax_write_iq_criteria(void)
{
	int retval = 0;
	TS_LOG_INFO("%s: start \n", __func__);
	//write criteria csv data to 0x98~0x9D register
	retval = i2c_himax_write(HX_REG_FLASH_WPLACE,  &hx_criteria[HX_CRITERIA_FAC_CNT], HX_CRITERIA_IQ_CNT, sizeof(hx_criteria), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_FLASH_WPLACE error %d!\n",__LINE__);
		return HX_ERROR;
	}
	TS_LOG_INFO("%s: end \n", __func__);

	return HX_OK;
}
static int himax_exit_iq_self_mode(void)
{
	int retval = 0;
	uint8_t cmdbuf[IQ_CMDBUF_LEN] = {0};
	memset(cmdbuf, 0x00, sizeof(cmdbuf));
	TS_LOG_INFO("%s: start \n", __func__);
	//disable sram switch
	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_DISABLE;
	retval = i2c_himax_write(HX_REG_SRAM_SWITCH, &cmdbuf[0], ONEBYTE, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_SRAM_SWITCH error %d!\n",__LINE__);
		return HX_ERROR;
	}
	cmdbuf[0] = 0;//close flash reload cmd
	retval = i2c_himax_write(HX_REG_CLOSE_FLASH_RELOAD, &cmdbuf[0], ONEBYTE, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_CLOSE_FLASH_RELOAD error %d!\n",__LINE__);
		return HX_ERROR;
	}

	cmdbuf[0] = 0x06;//back rawdata mode cmd
	retval = i2c_himax_write( HX_REG_RAWDATA_MODE,&cmdbuf[0], ONEBYTE, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_RAWDATA_MODE error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_120MS);

	retval = i2c_himax_write( HX_CMD_TSSON,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_CMD_TSSON error %d!\n",__LINE__);
		return HX_ERROR;
	}
	//open IC adc
	msleep(HX_SLEEP_120MS);
	retval = i2c_himax_write( HX_CMD_TSSLPOUT,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_CMD_TSSLPOUT error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_2S);

	retval = i2c_himax_write(HX_CMD_TSSOFF,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_CMD_TSSOFF error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_120MS);
	//back to rawdata mode
	retval = i2c_himax_write(HX_CMD_TSSLPIN,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_CMD_TSSLPIN error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_120MS);
	cmdbuf[0] = 0x00;
	retval = i2c_himax_write(HX_REG_RAWDATA_MODE,&cmdbuf[0], ONEBYTE, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_RAWDATA_MODE error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_120MS);
	//close sram register switch
	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_EN;
	retval = i2c_himax_write(HX_REG_SRAM_SWITCH ,&cmdbuf[0], ONEBYTE, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_SRAM_SWITCH error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_10MS);
	TS_LOG_INFO("%s: end \n", __func__);

	return HX_OK;
}
static int himax_iq_self_test(void)
{
	int i = 0;
	int pf_value = 0x0;
	uint8_t cmdbuf[IQ_CMDBUF_LEN] = {0};
	uint8_t valuebuf[IQ_VALUEBUF_LEN] = {0};
	int retval = 0;
	memset(cmdbuf, 0x00, sizeof(cmdbuf));
	memset(valuebuf, 0x00, sizeof(valuebuf));
	TS_LOG_INFO("%s: start \n", __func__);

	for(i = HX_CRITERIA_FAC_CNT;i < HX_CRITERIA_CNT;i++) {
		TS_LOG_INFO("[Himax]: himax_iq_self_test hx_criteria is data[%d] = %d\n", i, hx_criteria[i]);
	}
	retval = himax_enter_iq_mode();
	if(retval != 0) {
		TS_LOG_ERR("enter himax_enter_iq_mode fail!\n");
		return HX_ERROR;
	}
	retval = himax_write_iq_criteria();
	if(retval != 0) {
		TS_LOG_ERR("himax_write_iq_criteria fail!\n");
		return HX_ERROR;
	}
	retval = himax_exit_iq_self_mode();
	if(retval != 0) {
		TS_LOG_ERR("himax_exit_iq_self_mode fail!\n");
		return HX_ERROR;
	}

	/*set read addr*/
	cmdbuf[0] = 0x00;
	cmdbuf[1] = HX_CMD_ADDR_RESULT;
	retval = i2c_himax_write(HX_REG_SRAM_ADDR ,&cmdbuf[0], IQ_SRAM_CMD_LEN, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_SRAM_ADDR error %d!\n",__LINE__);
		return HX_ERROR;
	}
	msleep(HX_SLEEP_10MS);
	//read back test result to valuebuf[0]
	retval = i2c_himax_read(HX_REG_FLASH_RPLACE, valuebuf, IQ_BACK_VAL_LEN, sizeof(valuebuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_FLASH_RPLACE error %d!\n",__LINE__);
		return HX_ERROR;
	}
	//disable sram switch
	cmdbuf[0] = HX_REG_SRAM_TEST_MODE_DISABLE;
	retval = i2c_himax_write(HX_REG_SRAM_SWITCH ,&cmdbuf[0], ONEBYTE, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if(retval < 0) {
		TS_LOG_ERR("himax_iq_self_test HX_REG_SRAM_SWITCH error %d!\n",__LINE__);
		return HX_ERROR;
	}

	for(i=0;i<IQ_BACK_VAL_LEN;i++) {
		TS_LOG_INFO("[Himax]: After iq_self_test test %X valuebuf[%d] = 0x%x\n", HX_CMD_ADDR_RESULT,i,valuebuf[i]);
	}

	if (valuebuf[0]==0xAA) {
		TS_LOG_INFO("[Himax]: himax_iq_self_test pass\n");
		pf_value = 0x0;
	} else {
		TS_LOG_ERR("[Himax]: himax_iq_self_test fail\n");
		pf_value = 0x1;
	}
	TS_LOG_INFO("%s: end \n", __func__);
	return pf_value;
}

static int himax_self_test(void)
{
	int retval = NO_ERR;
	uint8_t cmdbuf[4] = {0};
	int iq_test_result = 0;
	TS_LOG_INFO("%s: start \n", __func__);

	himax_int_enable(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id,IRQ_DISABLE);

	iq_test_result = himax_iq_self_test();//iq_test_result   :  0 pass   1  fail
	if(iq_test_result != 0 )
	{
		TS_LOG_ERR("himax_iq_self_test error!\n");
	}
	TS_LOG_INFO("%s: iq_test_result= %d\n",__func__, iq_test_result);

	himax_HW_reset(HX_LOADCONFIG_EN,HX_INT_EN);
	retval = i2c_himax_write(HX_CMD_TSSOFF,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);//sense off--analog- close
	if( retval != 0 )
	{
		hx_result_fail_str[0] = '0';
		TS_LOG_INFO("%s: I2C Test --> fail\n",__func__);
		strncat(buf_test_result, hx_result_fail_str, strlen(hx_result_fail_str)+1);
		goto err_i2c;
	}
	else
	{
		hx_result_pass_str[0] = '0';
		TS_LOG_INFO("%s: I2C Test --> PASS\n",__func__);
		strncat(buf_test_result, hx_result_pass_str, strlen(hx_result_pass_str)+1);
	}
	msleep(HX_SLEEP_120MS);

	retval = i2c_himax_write(HX_CMD_TSSLPIN,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);//sleep in-- digital - close
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(HX_SLEEP_120MS);

	//Write golden data
	retval = himax_wirte_golden_data();
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}

	//Disable flash reload
	cmdbuf[0] = 0x00;
	retval = i2c_himax_write(HX_REG_CLOSE_FLASH_RELOAD, &cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}

	//Write Criteria
	retval = himax_wirte_criteria_data();
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}

	/* sorting mode*/
	cmdbuf[0] = 0xC0;
	retval = i2c_himax_write( HX_REG_FLASH_MODE,&cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);//change to sorting mode
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(HX_SLEEP_120MS);

	cmdbuf[0] = BASEC_CMD;
	retval = i2c_himax_write(HX_REG_RAWDATA_MODE,&cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);//written command: will be test base c
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(HX_SLEEP_120MS);

	retval = i2c_himax_write( HX_CMD_TSSON,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);//sense on -analog- open
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(HX_SLEEP_120MS);

	retval = i2c_himax_write( HX_CMD_TSSLPOUT,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);//sleep out -digital- open
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	/*fw run self_test*/
	msleep(HX_SLEEP_3S);

	retval = i2c_himax_write( HX_CMD_TSSOFF,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);//sense off
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(HX_SLEEP_120MS);
	retval = i2c_himax_write( HX_CMD_TSSLPIN,&cmdbuf[0], 0, sizeof(cmdbuf), DEFAULT_RETRY_CNT);//sleep in
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(HX_SLEEP_120MS);
	//get result.
	g_hx_self_test_result = himax_read_result_data();

	cmdbuf[0] = CLOSE_DIAG_COMMAND;
	retval = i2c_himax_write( HX_REG_RAWDATA_MODE,&cmdbuf[0], 1, sizeof(cmdbuf), DEFAULT_RETRY_CNT);//sleep in
	if( retval != 0 )
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(HX_SLEEP_120MS);

	/* get rawdata from sram and sense on IC */
	retval = read_rawdata_sram();
	if(retval < 0)
	{
		TS_LOG_ERR("himax_factory_self_test error!\n");
		goto err_i2c;
	}

	TS_LOG_INFO("%s: iq_test_result = %d,g_hx_self_test_result= %d\n",__func__, iq_test_result,g_hx_self_test_result);
	retval = g_hx_self_test_result + iq_test_result;  //0 :iq and self test pass  1: iq fail  or self test fail
	TS_LOG_INFO("%s: end \n", __func__);

	himax_int_enable(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id,IRQ_ENABLE);
	return retval;

err_i2c:
	TS_LOG_ERR("%s: Exit because there is I2C fail.\n",__func__);
	himax_int_enable(g_himax_ts_data->tskit_himax_data->ts_platform_data->irq_id,IRQ_ENABLE);
	himax_free_Rawmem();
	return I2C_WORK_ERR;
}



int himax_factory_start(struct himax_ts_data *ts,struct ts_rawdata_info *info_top)
{
	int retval = NO_ERR;
	uint16_t fac_dump_step = 0; //0: bank ,2: iir, 4:basec, 6:dc, 8:golden basec
	uint16_t index1 = 0;
	uint16_t self_num = 0;
	uint16_t mutual_num = 0;
	uint16_t rx = getXChannel();
	uint16_t tx = getYChannel();
	struct file *fn;

	mutual_num	= rx * tx;
	self_num	= rx + tx;

	fac_dump_buffer= kzalloc((mutual_num+self_num)*10 * sizeof(uint8_t), GFP_KERNEL);
	if (!fac_dump_buffer){
		TS_LOG_ERR("device, fac_dump_buffer is NULL \n");
		return HX_ERROR;
	}

	memset(fac_dump_buffer,0x00,(mutual_num+self_num)*10 );
	/*P:self test pass flag*/
	hx_result_pass_str[1] = 'P';
	hx_result_pass_str[2] = '-';
	hx_result_pass_str[3] = '\0';
	/*F:self test fail flag*/
	hx_result_fail_str[1] = 'F';
	hx_result_fail_str[2] = '-';
	hx_result_fail_str[3] = '\0';

	TS_LOG_INFO("%s :Entering\n",__func__);

	info = info_top;

	/* init */
	if(g_himax_ts_data->suspended)
	{
		TS_LOG_ERR("%s: Already suspended. Can't do factory test. \n", __func__);
		return SUSPEND_IN;
	}

	retval = himax_parse_threshold_file();

	if (retval < 0) {
		TS_LOG_ERR("%s: Parse .CSV file Fail.\n", __func__);
	}

	if(atomic_read(&hmx_mmi_test_status)){
		TS_LOG_ERR("%s factory test already has been called.\n",__func__);
		return FACTORY_RUNNING;
	}

	atomic_set(&hmx_mmi_test_status, 1);
	memset(buf_test_result, 0, RESULT_LEN);
	memcpy(buf_test_result, "result: ", strlen("result: ")+1);

	TS_LOG_INFO("himax_gold_self_test enter \n");

	wake_lock(&g_himax_ts_data->ts_flash_wake_lock);

	retval = himax_alloc_Rawmem();
	if( retval != 0 )
	{
		TS_LOG_ERR("%s factory test alloc_Rawmem failed.\n",__func__);
		goto err_alloc;
	}

	/* step0: himax self test*/
	hx_result_status[test0] = himax_self_test(); //0 :iq and self test pass  1: iq fail  or self test fail

	/* step1: cap rawdata */
	hx_result_status[test1] = himax_bank_test(1); //for Rawdata
	himax_fac_dump(fac_dump_step,mutual_num,self_num,mutual_bank,self_bank);
	fac_dump_step += 2;

	/* step2: TX RX Delta */
	hx_result_status[test2] = himax_self_delta_test(2); //for TRX delta

	/* step3: Noise Delta */
    hx_result_status[test3] = himax_iir_test(3); //for Noise Delta
	himax_fac_dump(fac_dump_step,mutual_num,self_num,mutual_iir,self_iir);
	fac_dump_step += 2;

	/* step4: Open/Short */
	hx_result_status[test4] = himax_basec_test(4); //for short/open
	himax_fac_dump(fac_dump_step,mutual_num,self_num,mutual_basec,self_basec);
	fac_dump_step += 2;
	himax_fac_dump(fac_dump_step,mutual_num,self_num,mutual_dc,self_dc);
	fac_dump_step += 2;
	himax_fac_dump(fac_dump_step,mutual_num,self_num,mutual_golden_basec,self_golden_basec);

	//=============Show test result===================
	strncat(buf_test_result, STR_IC_VENDOR,strlen(STR_IC_VENDOR)+1);
	strncat(buf_test_result, "-",strlen("-")+1);
	strncat(buf_test_result, ts->tskit_himax_data->ts_platform_data->chip_data->chip_name,strlen(ts->tskit_himax_data->ts_platform_data->chip_data->chip_name)+1);
	strncat(buf_test_result, "-",strlen("-")+1);
	strncat(buf_test_result, ts->tskit_himax_data->ts_platform_data->product_name,strlen(ts->tskit_himax_data->ts_platform_data->product_name)+1);

	strncat(info->result,buf_test_result,strlen(buf_test_result)+1);

	info->buff[0] = rx;
	info->buff[1] = tx;

	/*print basec and dc*/
	current_index=2;
	himax_print_rawdata(BANK_CMD);
	himax_print_rawdata(RTX_DELTA_CMD);
	himax_print_rawdata(IIR_CMD);
	info->used_size = current_index;
#ifdef HX_FAC_LOG_PRINT
	//=====debug log======
	for (index1 = 0; index1 < (mutual_num+self_num)*10; index1++) {
		printk("%4d", fac_dump_buffer[index1]);
		if ((index1 % rx) == (rx - 1))
			printk("\n");
	}
	//=====debug log=====
#endif
	//========write file into system===========
	TS_LOG_INFO("%s: fac write file start\n",__func__);
	fn = filp_open(HX_RAW_DUMP_FILE,O_CREAT | O_WRONLY ,0);
	if (!IS_ERR(fn))
	{
		TS_LOG_INFO("%s: fac write file \n",__func__);
		fn->f_op->write(fn,fac_dump_buffer,(mutual_num+self_num)*10*sizeof(uint8_t),&fn->f_pos);
		filp_close(fn,NULL);
	}
	else
		TS_LOG_INFO("%s: open file fail\n",__func__);
	kfree(fac_dump_buffer);
	fac_dump_buffer=NULL;
	//================================
	TS_LOG_INFO("%s: End \n",__func__);

	himax_HW_reset(HX_LOADCONFIG_EN,HX_INT_DISABLE);

err_alloc:
	wake_unlock(&g_himax_ts_data->ts_flash_wake_lock);
	atomic_set(&hmx_mmi_test_status, 0);
	return retval;
}

static int himax_parse_threshold_file(void)
{
	int retval = 0;

	struct himax_ts_data *cd = g_himax_ts_data;

	retval = request_firmware(&fw_entry, "ts/CHOPIN_auo_threshold.csv", cd->dev);
	if (retval < 0) {
		TS_LOG_ERR("%s: Fail request firmware\n", __func__);
		goto exit;
	}
	if (fw_entry == NULL) {
		TS_LOG_ERR("%s: fw_entry == NULL\n", __func__);
		retval = -1;
		goto exit;
	}
	if (fw_entry->data == NULL || fw_entry->size == 0) {
		TS_LOG_ERR("%s: No firmware received\n", __func__);
		retval = -2;
		goto exit;
	}
	TS_LOG_INFO("%s: fw_entry->size = %zu\n", __func__, fw_entry->size);

	TS_LOG_DEBUG("%s: Found cmcp threshold file.\n", __func__);

	retval = himax_parse_threshold_file_method(&fw_entry->data[0], fw_entry->size);
	if (retval < 0) {
		TS_LOG_ERR("%s: Parse Cmcp file\n", __func__);
		retval = -3;
		goto exit;
	}

	return retval;

exit:
	release_firmware(fw_entry);
	return retval;
}
static int himax_parse_threshold_file_method(const char *buf, uint32_t file_size)
{
	int retval = 0;
	int index1 = 0;
	int case_num = 0;
	int rx = getXChannel();
	int tx = getYChannel();
	int gold_array_size = rx * tx  + tx  + rx;
	int buf_offset = strlen(FACTORY_TEST_START_LABEL);
	int m=(int)file_size;
	TS_LOG_INFO("%s: rawdata:array_size = %d HX_RX_NUM =%d HX_TX_NUM =%d \n", __func__, gold_array_size,rx,tx);

	for(case_num = 0;case_num < MAX_CASE_CNT; case_num++) {

		switch(case_num){
			case HX_CRITERIA:
				for(index_count = 0; index_count < HX_CRITERIA_CNT; ){
					retval = himax_get_one_value(buf,&buf_offset);
					if(retval >= 0)
					{
						hx_criteria[index_count++] = retval;
						TS_LOG_INFO("%s: rawdata:hx_criteria = %d\n", __func__, retval);
					}
					else if(buf_offset >= m)
					{
						break;
					}
					else
					{
						buf_offset++;//try next
					}
				}
				continue;
			case DC_GOLDEN_LIMIT:
					for(index_count = 0; index_count < gold_array_size; )
					{
						retval = himax_get_one_value(buf,&buf_offset);
						if(retval >= 0)
						{
							dc_golden_data[index_count++] = retval;
						}
						else if(buf_offset >= m)
						{
							break;
						}
						else
						{
							buf_offset++;//try next
						}
					}
					continue;//break;
			case BANK_GOLDEN_LIMIT:
					for(index_count = 0; index_count < gold_array_size; )
					{
						retval = himax_get_one_value(buf,&buf_offset);
						if(retval >= 0)
						{
							bank_golden_data[index_count++] = retval;
						}
						else if(buf_offset >= m)
						{
							break;
						}
						else
						{
							buf_offset++;//try next
						}
					}
					continue;
					default:
					{
						break;
					}
		}
	}

	//=====debug log======
	printk("[himax]%s:himax criteria:",__func__);
	for (index1 = 0; index1 < HX_CRITERIA_CNT; index1++) {
		printk("%4d", hx_criteria[index1]);
	}
	printk("\n");

	printk("[himax]%s:himax dc golden:",__func__);
	for (index1 = 0; index1 < rx *tx+rx+tx; index1++) {
		printk("%4d", dc_golden_data[index1]);
		if (((index1) %  (rx-1)) == 0 && index1 > 0)
		printk("\n");

	}
	printk("\n");

	printk("[himax]%s:himax bank golden:",__func__);
	for (index1 = 0; index1 < rx *tx+rx+tx; index1++) {
		printk("%4d", bank_golden_data[index1]);
		if (((index1) %  (rx-1)) == 0 && index1 > 0)
		printk("\n");
	}

	return retval;
}

static int himax_get_one_value(const char *buf, uint32_t *offset)
{
	int value = -1;
	char tmp_buffer[10] = {0};
	uint32_t count = 0;
	uint32_t tmp_offset = *offset;
	int m=0,n=0;
	/* Bypass extra commas */
	m=tmp_offset + 1;
	while (buf[tmp_offset] == ASCII_COMMA
			&& buf[m] == ASCII_COMMA)
	{
		tmp_offset++;
		m=tmp_offset + 1;
	}
	/* Windows and Linux difference at the end of one line */
	m=tmp_offset + 1;
	n=tmp_offset + 2;
	if (buf[tmp_offset] == ASCII_COMMA
			&& buf[m] == ASCII_CR
			&& buf[n] == ASCII_LF)
		tmp_offset += 2;
	else if (buf[tmp_offset] == ASCII_COMMA
			&& buf[m] == ASCII_LF)
		tmp_offset += 1;

	/* New line for multiple lines start*/
	m=tmp_offset + 1;
	if (buf[tmp_offset] == ASCII_LF && buf[m] == ASCII_COMMA) {
		tmp_offset++;
		m=tmp_offset + 1;
//		line_count++;
		/*dev_vdbg(dev, "\n");*/
	}
	if (buf[tmp_offset] == ASCII_LF && buf[m]!= ASCII_COMMA) {
		for(;;){
			tmp_offset++;
			m=tmp_offset + 1;
			if (buf[m] == ASCII_COMMA) {
				break;
			}
		}
	}
	/* New line for multiple lines end*/
	/* Beginning */
	if (buf[tmp_offset] == ASCII_COMMA) {
		tmp_offset++;
		for (;;) {
			if ((buf[tmp_offset] >= ASCII_ZERO)
			&& (buf[tmp_offset] <= ASCII_NINE)) {
				tmp_buffer[count++] = buf[tmp_offset] - ASCII_ZERO;
				tmp_offset++;
			} else {
				if (count != 0) {
					value = himax_ctoi(tmp_buffer,count);
					/*dev_vdbg(dev, ",%d", value);*/
				} else {
					/* 0 indicates no data available */
					value = -1;
				}
				break;
			}
		}
	} else {
	/*do plus outside tmp_offset++;*/
	}

	*offset = tmp_offset;

	return value;
}

static int himax_ctoi(char *buf, uint32_t count)
{
	int value = 0;
	uint32_t index = 0;
	uint32_t base_array[] = {1, 10, 100, 1000};

	for (index = 0; index < count; index++)
		value += buf[index] * base_array[count - 1 - index];
	return value;
}

