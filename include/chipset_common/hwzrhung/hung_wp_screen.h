#ifndef __LINUX_HUNG_WP_SCREEN_H__
#define __LINUX_HUNG_WP_SCREEN_H__

#define WP_SCREEN_PWK_PRESS 0
#define WP_SCREEN_PWK_RELEASE 1
#define WP_SCREEN_PWK_LONGPRESS 2

#define WP_SCREEN_VDOWN_KEY 1
#define WP_SCREEN_VUP_KEY 2

void hung_wp_screen_powerkey_ncb(unsigned long);
void hung_wp_screen_setbl(int level);
void hung_wp_screen_vkeys_cb(unsigned int knum, unsigned int value);
#endif
