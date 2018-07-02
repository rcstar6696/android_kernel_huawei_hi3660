#ifndef __MDRV_HDS_COMMON_H__
#define __MDRV_HDS_COMMON_H__
#ifdef __cplusplus
extern "C"
{
#endif

/*底软cmdid注册规则:31-28bit(组件ID):0x9; 27-24bit(模指示); 23-19bit(消息类型):0x0; 18-0bit(消息ID):自己定义*/
/* BSP CFG BEGIN*/
#define DIAG_CMD_LOG_SET                        (0x90035308)

/* 文件操作类（0x5600-0x56ff）*/
#define DIAG_CMD_FS_QUERY_DIR                   (0x90015601)
#define DIAG_CMD_FS_SCAN_DIR                    (0x90015602)
#define DIAG_CMD_FS_MAKE_DIR                    (0x90015603)
#define DIAG_CMD_FS_OPEN                        (0x90015604)
#define DIAG_CMD_FS_IMPORT                      (0x90015605)
#define DIAG_CMD_FS_EXPORT                      (0x90015606)
#define DIAG_CMD_FS_DELETE                      (0x90015607)
#define DIAG_CMD_FS_SPACE                       (0x90015608)

/* NV操作类（0x5500-0x55ff）*/
#define DIAG_CMD_NV_WR                          (0x90015001)
#define DIAG_CMD_NV_RD                          (0x90015003)
#define DIAG_CMD_GET_NV_LIST                    (0x90015005)
#define DIAG_CMD_GET_NV_RESUM_LIST              (0x90015006)
#define DIAG_CMD_NV_AUTH                        (0x90015007)

/* BSP CFG END*/

typedef int (*bsp_hds_func)(unsigned char *pstReq);
typedef int (*hds_cnf_func)(void *hds_cnf, void *data, unsigned int len);
int mdrv_hds_printlog_conn(void);
int mdrv_hds_translog_conn(void);
int mdrv_hds_msg_proc(void *pstReq);
void mdrv_hds_cmd_register(unsigned int cmdid, bsp_hds_func fn);
void mdrv_hds_get_cmdlist(unsigned int *cmdlist, unsigned int *num);
void mdrv_hds_cnf_register(hds_cnf_func fn);

#ifdef __cplusplus
}
#endif
#endif

