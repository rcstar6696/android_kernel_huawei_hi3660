#ifndef _VBAT_OVP
#define _VBAT_OVP


//int hisi_nve_direct_access(struct hisi_nve_info_user *user_info);

/*battery ovp nv */
#define BATOVP_DATA_NV_NUM    (375)
#define BATOVP_DATA_NV_SIZE    (64)
#define NV_READ_TAG                    (1)
#define NV_WRITE_TAG                  (0)
#define VBAT_OVP_WORK_DELAY   0


extern int vbat_ovp_set_input_current_flag;
extern struct atomic_notifier_head vbat_ovp_notifier_list;
extern struct completion usb_detach_completion;
struct vbat_ovp_device_info {
	struct delayed_work vbat_ovp_work;
	struct notifier_block vbat_ovp_nb;
	struct vbat_ovp_ops *vbat_ovp_ops;
	int vbat_ovp_handle_start_flag;
	int vbat_ovp_status;
};
struct vbat_ovp_ops{
	int (*cut_off_battery)(void);
};
int vbat_ovp_ops_register(struct vbat_ovp_ops *ops);
int get_vbat_ovp_status(void);
#endif
