#ifndef _ANDROID_FILESYSTEM_CONFIG_H_
#define _ANDROID_FILESYSTEM_CONFIG_H_

/* This is the master Users and Groups config for the platform.
 * DO NOT EVER RENUMBER
 */

#define AID_ROOT          0     /* traditional unix root user */

#define AID_SYSTEM        1000  /* system server */

#define AID_SDCARD_RW     1015  /* external storage write access */

#define AID_MEDIA_RW      1023  /* internal media storage write access */

#define AID_SDCARD_R      1028  /* external storage read access */
#define AID_SDCARD_PICS   1033  /* external storage photos access */
#define AID_SDCARD_AV     1034  /* external storage audio/video access */
#define AID_SDCARD_ALL    1035  /* access all users external storage */

#define AID_PACKAGE_INFO  1032  /* access to installed package details */

#define AID_EVERYBODY     9997  /* shared between all apps in the same profile */

#endif
