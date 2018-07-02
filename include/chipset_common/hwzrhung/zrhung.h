#ifndef __ZRHUNG_H
#define __ZRHUNG_H

#include <linux/types.h>
#include <linux/ioctl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ZRHUNG_WP_NONE         = 0,
    ZRHUNG_WP_HUNGTASK     = 1,
    ZRHUNG_WP_SCREENON     = 2,
    ZRHUNG_WP_SCREENOFF    = 3,
    ZRHUNG_WP_SR           = 4,
    ZRHUNG_WP_LMKD         = 5,
    ZRHUNG_WP_IO           = 6,
    ZRHUNG_WP_CPU          = 7,
    ZRHUNG_WP_GC           = 8,
    ZRHUNG_WP_TP           = 9,
    ZRHUNG_WP_FENCE        = 10,
    ZRHUNG_WP_SF           = 11,
    ZRHUNG_WP_SERVICE      = 12,
    ZRHUNG_WP_GPU          = 13,
    ZRHUNG_WP_TRANS_WIN    = 14,
    ZRHUNG_WP_FOCUS_WIN    = 15,
    ZRHUNG_WP_DARK_WIN     = 16,
    ZRHUNG_WP_RES_LEAK     = 17,
    ZRHUNG_WP_IPC_THREAD   = 18,
    ZRHUNG_WP_IPC_OBJECT   = 19,
    ZRHUNG_WP_NUM_MAX
} zrhung_wp_id;

#define ZRHUNG_CMD_LEN_MAX (512)
#define ZRHUNG_MSG_LEN_MAX (15 * 1024)
#define ZRHUNG_CMD_INVALID (0xFF)

typedef struct {
	uint32_t magic;
	uint16_t len;
	uint16_t wp_id;
	uint16_t cmd_len; // end with '\0', = cmd actual len + 1
	uint16_t msg_len; // end with '\0', = msg actual len + 1
	char info[0];     // <cmd buf>0<buf>0
} zrhung_write_event;

int zrhung_send_event(zrhung_wp_id id, const char* cmd_buf, const char*  buf);
int zrhung_get_config(zrhung_wp_id id, char *data, uint32_t maxlen);
long zrhung_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
long zrhung_save_lastword(void);

int lmkwp_init(void);
void lmkwp_report(struct task_struct *selected, struct shrink_control *sc, long cache_size, long cache_limit, short adj, long free);
#ifdef __cplusplus
}
#endif
#endif /* __ZRHUNG_H */

