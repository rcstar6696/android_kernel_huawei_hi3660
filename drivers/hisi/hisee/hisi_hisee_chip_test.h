#ifndef __HISI_HISEE_CHIP_TEST_H__
#define __HISI_HISEE_CHIP_TEST_H__

/* check ret is ok or otherwise goto err_process*/
#define CHECK_OK(ret) do { if (HISEE_OK != (ret)) goto err_process; } while (0)

int verify_key(void);
int hisee_parallel_manufacture_func(void *buf, int para);
#ifdef CONFIG_HISI_HISEE_NVMFORMAT_TEST
int hisee_nvmformat_func(void *buf, int para);
#endif
#ifdef __SLT_FEATURE__
int hisee_parallel_total_slt_func(void *buf, int para);
int hisee_read_slt_func(void *buf, int para);
int hisee_total_slt_func(void *buf, int para);
#endif

#endif
