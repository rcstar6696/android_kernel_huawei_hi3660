#include "lcdkit_dbg.h"
#include "lcdkit_btb_check.h"

#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#include <linux/sched.h>
#include "lcdkit_dbg.h"
#endif

extern int lcdkit_gpio_cmds_tx(unsigned int gpio_lcdkit_btb, int gpio_optype);
extern int lcdkit_gpio_pulldown(void * btb_vir_addr);
extern int lcdkit_gpio_pullup(void * btb_vir_addr);

extern struct dsm_client* lcd_dclient;

/*lcd btb check*/
int btb_floating_times = 0;
int btb_highlever_times = 0;
struct lcdkit_btb_info lcdkit_btb_inf = {0, 0, 0};

static int btb_check_state(void)
{
	int ret = 0;
	struct lcdkit_btb_info *btb_info_pr = &lcdkit_btb_inf;
	void * btb_vir_addr = 0;
	int error_floating = 0;
	int error_highlever = 0;
	int pulldown_read = 1;
	int pullup_read = 1;
	char buf[BUF_LENGTH] = {'\0'};

	if (btb_info_pr->gpio_lcdkit_btb == 0) {
		LCDKIT_ERR("get btb gpio failed!\n");
		return CHECK_SKIPPED;
	}
	lcdkit_gpio_cmds_tx(btb_info_pr->gpio_lcdkit_btb, BTB_GPIO_REQUEST);

	if (btb_info_pr->btb_con_addr == 0) {
		LCDKIT_ERR("get btb config address failed!\n");
		return CHECK_SKIPPED;
	}
	btb_vir_addr = (void *)ioremap_wc(btb_info_pr->btb_con_addr, sizeof(btb_info_pr->btb_con_addr));	/* IO config address remap */
	if (btb_vir_addr == NULL) {
		LCDKIT_ERR("btb_con_addr ioremap error !\n");
		return CHECK_SKIPPED;
	}

	if (!lcdkit_gpio_pulldown(btb_vir_addr)) {		/* config pull-down and read */
		LCDKIT_ERR("btb set gpio pulldown failed!\n");
		return CHECK_SKIPPED;
	}
	pulldown_read = lcdkit_gpio_cmds_tx(btb_info_pr->gpio_lcdkit_btb, BTB_GPIO_READ);

	if (!lcdkit_gpio_pullup(btb_vir_addr)) {		/* config pull-up and read */
		LCDKIT_ERR("btb set gpio pullup failed!\n");
		return CHECK_SKIPPED;
	}
	pullup_read = lcdkit_gpio_cmds_tx(btb_info_pr->gpio_lcdkit_btb, BTB_GPIO_READ);

	lcdkit_gpio_cmds_tx(btb_info_pr->gpio_lcdkit_btb, BTB_GPIO_FREE);
	iounmap(btb_vir_addr);		/* IO config address unmap */

	if(pulldown_read != pullup_read) {
		error_floating = 1;	/*make error flag*/
		if (btb_floating_times < MAX_REPPORT_TIMES+1){	/*Device Radar error report is limited to 5, too many error records are not necessary*/
			btb_floating_times++;
		}
		memcpy(buf, "LCD not connected!\n", sizeof("LCD not connected!\n"));
		LCDKIT_ERR("btb is floating status, LCD not connected!\n");
		goto check_error;
	} else if(pulldown_read == BTB_STATE_HIGH_LEVER || pullup_read == BTB_STATE_HIGH_LEVER) {
		error_highlever = 1;	/*make error flag*/
		if (btb_highlever_times < MAX_REPPORT_TIMES+1){
			btb_highlever_times++;
		}
		memcpy(buf, "LCD is connected error!\n", sizeof("LCD is connected error!\n"));
		LCDKIT_ERR("btb is high-lever status, LCD is connected error!\n");
		goto check_error;
	} else {
		return CHECKED_OK;
	}

check_error:
#if defined (CONFIG_HUAWEI_DSM)
	if ( NULL == lcd_dclient )
	{
		LCDKIT_ERR(": there is not lcd_dclient!\n");
		return CHECK_SKIPPED;
	}
	if ((error_floating == 1 && btb_floating_times <= MAX_REPPORT_TIMES)
		|| (error_highlever == 1 && btb_highlever_times <= MAX_REPPORT_TIMES)) {	/*when error times is more than 5, don't report anymore*/
		ret = dsm_client_ocuppy(lcd_dclient);
		if ( !ret ) {
			dsm_client_record(lcd_dclient, buf);
			dsm_client_notify(lcd_dclient, DSM_LCD_BTB_CHECK_ERROR_NO);
		}else{
			LCDKIT_ERR("dsm_client_ocuppy ERROR:retVal = %d\n", ret);
			return CHECK_SKIPPED;
		}
	}
#endif
	return CHECKED_ERROR;
}

int mipi_lcdkit_btb_check(void)
{
	struct lcdkit_btb_info *btb_info_pr = &lcdkit_btb_inf;

	if(btb_info_pr->btb_support == 0){	/*not support btb check*/
		LCDKIT_INFO("not support btb check\n");
		return CHECK_SKIPPED;
	}

	return btb_check_state();
}

