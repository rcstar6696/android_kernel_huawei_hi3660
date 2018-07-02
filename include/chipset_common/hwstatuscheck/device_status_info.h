
#ifndef _DEVICE_STATUS_INFO_H_
#define _DEVICE_STATUS_INFO_H_

#ifdef __cplusplus
extern "C" {
#endif
/* hw check list */
typedef enum
{
    CHECK_STATUS_ROOT ,
    DEV_CHECK_STATUS_MAX,
}hw_check_dev_status_type;

extern int set_hw_check_dev_status_flag(int check_id);
#ifdef __cplusplus
}
#endif
#endif
