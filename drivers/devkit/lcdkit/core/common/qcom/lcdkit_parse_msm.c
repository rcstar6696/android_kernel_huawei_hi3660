#include "mdss_dba_utils.h"
#include <linux/lcdkit_dsm.h>
#include "mdss_fb.h"
#include "lcdkit_panel.h"
#include "lcdkit_parse.h"
#include "mdss_dsi.h"
#include "mdss_dsi_cmd.h"
#include <linux/leds.h>
#include "lcdkit_dbg.h"

/***********************************************************
*function definition
***********************************************************/
int buf_trans(const char* inbuf, int inlen, char** outbuf, int* outlen)
{
    char *buf;
    int bufsize = inlen;
    /*If use bype property: [], this division should be removed*/
	buf = kzalloc(sizeof(char) * bufsize, GFP_KERNEL);

	if (!buf) {
		return -1;
	}

    memcpy(buf, inbuf, bufsize);

    *outbuf = buf;
    *outlen = bufsize;

	return 0;
}

void lcdkit_dsi_cmd_trans(struct dsi_panel_cmds *dsi_cmds,
                    u32 *flags, struct lcdkit_dsi_panel_cmds *cmds);
void lcdkit_trans_exist_cmds(struct dsi_panel_cmds *outcmds,
                          struct lcdkit_dsi_panel_cmds *incmds)
{
    size_t size;
    size = incmds->cmd_cnt * sizeof(struct dsi_cmd_desc);

    //how to free memory alloc here? free it on deinit lcd module?
    outcmds->cmds = kzalloc(size, GFP_KERNEL);
    if (!outcmds->cmds)
        return ;

    lcdkit_dsi_cmd_trans(outcmds, NULL, incmds);
}

void lcdkit_parse_platform_dts(struct device_node *np, void *pdata)
{
    (void)mdss_panel_parse_dt(np, (struct mdss_dsi_ctrl_pdata *)pdata);
}

