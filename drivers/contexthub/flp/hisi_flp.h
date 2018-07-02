/*
 * hisi flp driver.
 *
 * Copyright (C) 2015 huawei Ltd.
 * Author: lijiangxiong <lijiangxiong@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/*ioctrl cmd type*/
#define FLP_TAG_FLP         0
#define FLP_TAG_GPS         1

#define FLP_GPS_BATCHING_MAX_SIZE   50
#define FLP_GEOFENCE_MAX_NUM        48

#define FLP_IOCTL_CMD_MASK              (0xFFFF00FF)

#define FLP_IOCTL_TAG_MASK              (0xFFFFFF00)
#define FLP_IOCTL_TAG_FLP               (0x464C0000)
#define FLP_IOCTL_TAG_GPS               (0x464C0100)
#define FLP_IOCTL_TAG_AR                (0x464C0200)
#define FLP_IOCTL_TAG_COMMON            (0x464CFF00)

#define FLP_IOCTL_TYPE_MASK             (0xFFFF00F0)
#define FLP_IOCTL_TYPE_PDR              (0x464C0000)
#define FLP_IOCTL_TYPE_GEOFENCE         (0x464C0020)
#define FLP_IOCTL_TYPE_BATCHING         (0x464C0030)
#define FLP_IOCTL_TYPE_COMMON           (0x464C00F0)

#define FLP_IOCTL_PDR_START(x)          (0x464C0000 + ((x) * 0x100) + 1)
#define FLP_IOCTL_PDR_STOP(x)           (0x464C0000 + ((x) * 0x100) + 2)
#define FLP_IOCTL_PDR_READ(x)           (0x464C0000 + ((x) * 0x100) + 3)
#define FLP_IOCTL_PDR_WRITE(x)          (0x464C0000 + ((x) * 0x100) + 4)
#define FLP_IOCTL_PDR_CONFIG(x)         (0x464C0000 + ((x) * 0x100) + 5)
#define FLP_IOCTL_PDR_UPDATE(x)         (0x464C0000 + ((x) * 0x100) + 6)
#define FLP_IOCTL_PDR_FLUSH(x)          (0x464C0000 + ((x) * 0x100) + 7)
#define FLP_IOCTL_PDR_STEP_CFG(x)       (0x464C0000 + ((x) * 0x100) + 8)


#define FLP_IOCTL_GEOFENCE_ADD                  (0x464C0000 + 0x21)
#define FLP_IOCTL_GEOFENCE_REMOVE               (0x464C0000 + 0x22)
#define FLP_IOCTL_GEOFENCE_MODIFY               (0x464C0000 + 0x23)

#define FLP_IOCTL_BATCHING_START                (0x464C0000 + 0x31)
#define FLP_IOCTL_BATCHING_STOP                 (0x464C0000 + 0x32)
#define FLP_IOCTL_BATCHING_UPDATE               (0x464C0000 + 0x33)
#define FLP_IOCTL_BATCHING_CLEANUP              (0x464C0000 + 0x34)
#define FLP_IOCTL_BATCHING_LASTLOCATION         (0x464C0000 + 0x35)
#define FLP_IOCTL_BATCHING_FLUSH                (0x464C0000 + 0x36)
#define FLP_IOCTL_BATCHING_INJECT               (0x464C0000 + 0x37)
#define FLP_IOCTL_BATCHING_GET_SIZE             (0x464C0000 + 0x38)
#define FLP_IOCTL_COMMON_HW_RESET               (0x464C0000 + 0x39)

/*common ioctrl*/
#define FLP_IOCTL_COMMON_GET_UTC            (0x464C0000 + 0xFFF1)
#define FLP_IOCTL_COMMON_SLEEP              (0x464C0000 + 0xFFF2)
#define FLP_IOCTL_COMMON_AWAKE_RET          (0x464C0000 + 0xFFF3)
#define FLP_IOCTL_COMMON_SETPID             (0x464C0000 + 0xFFF4)
#define FLP_IOCTL_COMMON_CLOSE_SERVICE      (0x464C0000 + 0xFFF5)
#define FLP_IOCTL_COMMON_HOLD_WAKELOCK      (0x464C0000 + 0xFFF6)
#define FLP_IOCTL_COMMON_RELEASE_WAKELOCK   (0x464C0000 + 0xFFF7)

enum {
    FLP_SERVICE_RESET,
    FLP_GNSS_RESET,
    FLP_GNSS_RESUME,
    FLP_IOMCU_RESET,
};

typedef struct flp_pdr_data {
    unsigned long       msec ;
    unsigned int        step_count;
    int                 relative_position_x;
    int                 relative_position_y;
    short               velocity_x;
    short               velocity_y;
    unsigned int        migration_distance;
    unsigned int        absolute_altitude;
    unsigned short      absolute_bearing;
    unsigned short      reliability_flag;
}  __packed flp_pdr_data_t  ;

typedef struct compensate_data {
    unsigned int        compensate_step;
    int                 compensate_position_x;
    int                 compensate_position_y;
    unsigned int        compensate_distance;
} compensate_data_t ;

typedef struct  step_report {
    int data[12] ;
} step_report_t;

typedef struct buf_info {
    char            *buf;
    unsigned long   len;
} buf_info_t;

typedef struct pdr_start_config {
    unsigned int   report_interval;
    unsigned int   report_precise;
    unsigned int   report_count;
    unsigned int   report_times;   /*plan to remove */
} pdr_start_config_t;
/********************************************
      define Batching and Geofence struct
********************************************/
typedef struct {
    unsigned short		flags;
    double              latitude;
    double              longitude;
    double              altitude;
    float               speed;
    float               bearing;
    float               accuracy;
    long		        timestamp;
    unsigned int        sources_used;
} __packed iomcu_location;

typedef struct gps_batching_report{
    int num_locations;
    iomcu_location location[FLP_GPS_BATCHING_MAX_SIZE];
} __packed gps_batching_report_t;

/*Geofence event report*/
typedef struct geofencing_transition {
    int geofence_id;
    int transition;
    unsigned int sources_used;
    unsigned long timestamp;
    iomcu_location location;
} __packed geofencing_transition_t;

/*Geofence status report*/
typedef struct geofencing_monitor_status {
    unsigned char status;
    unsigned int source;
    iomcu_location last_location;
}__packed geofencing_monitor_status_t;

/*modify Geofence*/
typedef struct geofencing_option_info {
    unsigned char  virtual_id;
    unsigned char last_transition;
    unsigned char monitor_transitions;
    unsigned char reserved;
    int unknown_timer_ms;
    unsigned int sources_to_use;
}__packed  geofencing_option_info_t;

typedef struct {
double latitude;
double longitude;
double radius_m;
}__packed geofencing_circle_t;

typedef struct {
    unsigned char geofence_type;
    union {
        geofencing_circle_t circle;
    };
}__packed geofencing_data_t;

typedef struct geofencing_useful_data {
    geofencing_option_info_t  opt;
    geofencing_data_t info;
}__packed   geofencing_useful_data_t;

/*add Geofence*/
typedef struct geofencing_add_config {
    unsigned char  number_of_geofences;
    geofencing_useful_data_t geofences[FLP_GEOFENCE_MAX_NUM];
}__packed  geofencing_add_config_t;

/*remove Geofence*/
typedef struct geofencing_remove_config {
    unsigned char  number_of_geofences;
    unsigned char index_id[FLP_GEOFENCE_MAX_NUM];
}__packed  geofencing_remove_config_t;

typedef struct geofencing_hal_config {
    unsigned int  length;
    void *buf;
}__packed  geofencing_hal_config_t;

/*start or update batching cmd*/
typedef struct {
    double max_power_allocation_mW;
    unsigned int sources_to_use;
    unsigned int flags;
    long period_ns;
    float smallest_displacement_meters;
} FlpBatchOptions;

typedef struct batching_config {
    int  id;
    int  batching_distance;
    FlpBatchOptions opt;
} __packed  batching_config_t;

/********************************************
            define flp netlink
********************************************/
#define FLP_GENL_NAME                   "TASKFLP"
#define TASKFLP_GENL_VERSION            0x01

enum {
    FLP_GENL_ATTR_UNSPEC,
    FLP_GENL_ATTR_EVENT,
    __FLP_GENL_ATTR_MAX,
};
#define FLP_GENL_ATTR_MAX (__FLP_GENL_ATTR_MAX - 1)

enum {
    FLP_GENL_CMD_UNSPEC,
    FLP_GENL_CMD_PDR_DATA,
    FLP_GENL_CMD_AR_DATA,
    FLP_GENL_CMD_PDR_UNRELIABLE,
    FLP_GENL_CMD_NOTIFY_TIMEROUT,
    FLP_GENL_CMD_AWAKE_RET,
    FLP_GENL_CMD_GEOFENCE_TRANSITION,
    FLP_GENL_CMD_GEOFENCE_MONITOR,
    FLP_GENL_CMD_GNSS_LOCATION,
    FLP_GENL_CMD_IOMCU_RESET,
    FLP_GENL_CMD_ENV_DATA,
    __FLP_GENL_CMD_MAX,
};
#define FLP_GENL_CMD_MAX (__FLP_GENL_CMD_MAX - 1)


