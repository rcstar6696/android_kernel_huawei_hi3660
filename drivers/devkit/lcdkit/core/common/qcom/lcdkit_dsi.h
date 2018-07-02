
#ifndef __LCDKIT_DSI_H__
#define __LCDKIT_DSI_H__

#include <linux/lcdkit_dsm.h>
#include "lcdkit_dbg.h"

enum lcd_run_mode_enum{
	LCD_RUN_MODE_INIT = 0,
	LCD_RUN_MODE_FACTORY,
	LCD_RUN_MODE_NORMAL,
};

struct regulator {
	struct device *dev;
	struct list_head list;
	unsigned int always_on:1;
	unsigned int bypass:1;
	int uA_load;
	int min_uV;
	int max_uV;
	int enabled;
	char *supply_name;
	struct device_attribute dev_attr;
	struct regulator_dev *rdev;
	struct dentry *debugfs;
};

void lcdkit_set_dsi_ctrl_pdev(struct platform_device *pdev);
static int mdss_dsi_panel_power_on(struct mdss_panel_data *pdata);
static int mdss_dsi_panel_power_off(struct mdss_panel_data *pdata);
static void lcdkit_regulator_init(struct dss_vreg *vreg, int cnt);
static int lcdkit_gpio_config(unsigned gpio, const char *label);
static void lcdkit_ctrl_gpio_init(struct mdss_dsi_ctrl_pdata *ctrl_pdata);
static int lcdkit_power_config(struct platform_device *ctrl_pdev,
    struct mdss_dsi_ctrl_pdata *ctrl_pdata, struct device_node *pan_node);
static void mdss_dsi_parse_lane_swap(struct device_node *np, char *dlane_swap);

#endif

