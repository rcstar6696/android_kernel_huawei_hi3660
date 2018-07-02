

#include <chipset_common/abns/abns_shutdown.h>

static int  g_abns_type = 0;
static char g_abns_swtype[ABNS_PARAM_STR_LEN] = {0};
static char g_abns_runmode[ABNS_PARAM_STR_LEN] = {0};
static char g_abns_verifiedbootmod[ABNS_PARAM_STR_LEN] = {0};


struct abns_word abns_type_map[] = {
    {"ABNS_NORMAL",ABNS_NORMAL},
    {"ABNS_PRESS10S",ABNS_PRESS10S},
    {"ABNS_LTPOWEROFF",ABNS_LTPOWEROFF},
    {"ABNS_PMU_EXP",ABNS_PMU_EXP},
    {"ABNS_PMU_SMPL",ABNS_PMU_SMPL},
    {"ABNS_UNKOWN",ABNS_UNKOWN},
    {"ABNS_BAT_LOW",ABNS_BAT_LOW},
    {"ABNS_BAT_TEMP_HIGH",ABNS_BAT_TEMP_HIGH},
    {"ABNS_BAT_TEMP_LOW",ABNS_BAT_TEMP_LOW},
    {"ABNS_CHIPSETS_TEMP_HIGH",ABNS_CHIPSETS_TEMP_HIGH},
};
int abns_atoi(char *p)
{
    int val = 0;

    if (!p)
        return -1;

    while (isdigit(*p))
        val = val*10 + (*p++ - '0');

    return val;
}


/************************************************************
Return:         1:   line over the max lenth.
                    0:   Success
                    -1: end of the file.
************************************************************/
static int abns_read_line(struct file* fp, char* buffer, loff_t* pPos) {
    ssize_t ret = 0;
    char pBuff[MAXSIZES_PATH] = {0};
    loff_t pos =  *pPos;
    int read_lenth = 0;

    if ((ret = vfs_read(fp, pBuff, MAXSIZES_PATH - 1, &pos)) > 0) {
        char* pStr = NULL;

        read_lenth = (int)(pos - *pPos);

        pStr = strchr(pBuff, '\n');
        if (NULL == pStr) {
            *pPos = pos;
            return 1;
        }

        *pStr = 0;
        strncpy(buffer, pBuff, MAXSIZES_PATH);
        *pPos = pos - (read_lenth - strlen(buffer) - 1);
        return 0;
    }
    else {
        return -1;
    }
}

static int __init early_parse_abns_flag(char *p)
{
    if (NULL == p){
        g_abns_type = ABNS_NORMAL;
        return -1;
    }

    if (NULL != p)
    {
        ABNS_PRINT_KEY_INFO(ABNS_TYPE_CMDLINE_STR "=%s\n", p);
        g_abns_type = abns_atoi(p);
        if(g_abns_type<ABNS_NORMAL || g_abns_type > ABNS_CHIPSETS_TEMP_HIGH){
            g_abns_type = ABNS_NORMAL;
        }
    }
    return 0;
}

static int __init early_parse_abns_swtype(char *p)
{

    if (NULL == p){
        snprintf(g_abns_swtype,ABNS_PARAM_STR_LEN-1,"%s",ABNS_UNKNOWN_STR);
    }

    if (NULL != p)
    {
        snprintf(g_abns_swtype,ABNS_PARAM_STR_LEN-1,"%s",p);
    }
    return 0;
}

static int __init early_parse_abns_runmode(char *p)
{

    if (NULL == p){
        snprintf(g_abns_runmode,ABNS_PARAM_STR_LEN-1,"%s",ABNS_UNKNOWN_STR);
        return -1;
    }

    if (NULL != p)
    {
        snprintf(g_abns_runmode,ABNS_PARAM_STR_LEN-1,"%s",p);
    }
    return 0;
}

static int __init early_parse_abns_verifiedbootmod(char *p)
{

    if (NULL == p){
        snprintf(g_abns_verifiedbootmod,ABNS_PARAM_STR_LEN-1,"%s",ABNS_UNKNOWN_STR);
        return -1;
    }

    if (NULL != p)
    {
        snprintf(g_abns_verifiedbootmod,ABNS_PARAM_STR_LEN-1,"%s",p);
    }
    return 0;
}


early_param(ABNS_TYPE_CMDLINE_STR, early_parse_abns_flag);
early_param(ABNS_SWTYPE_CMDLINE_STR, early_parse_abns_swtype);
early_param(ABNS_RUNMODE_CMDLINE_STR, early_parse_abns_runmode);
early_param(ABNS_VERIFIEDMD_CMDLINE_STR, early_parse_abns_verifiedbootmod);

/*
 *Set shutdown flag into reserved2 Part in kernel
*/
int set_unclean_shutdown_flag(int flag)
{
    int ret=0;
    ABNS_BTFG_T abns_btfg;
    if(flag != BOOT_FLAG_SUCC||flag != BOOT_FLAG_MANUAL_SHUTDOWN){/*lint !e650 */
        ABNS_PRINT_ERR("Can not set shutdown_flag %x in kernel!",flag);
        ret = -1;
    }
    ABNS_PRINT_ENTER();
    memset(&abns_btfg,0,sizeof(ABNS_BTFG_T));
    abns_btfg.magic =ABNS_INFO_MAGIC;
    abns_btfg.boot_flag=flag;
    ret = abns_write_emmc_raw_part(HISI_RSV_PART_NAME_STR, 0, (char*)&abns_btfg, sizeof(ABNS_BTFG_T));
    if ( 0 != ret )
    {
        ABNS_PRINT_ERR("abns_write_emmc_raw_part failed : %d!\n", ret);
        ret = -1;
    }
    ABNS_PRINT_EXIT();
    return ret;
}

/*
 *Get shutdown flag into reserved2 Part in kernel
*/
int get_unclean_shutdown_flag(void)
{
    int ret=0;
    ABNS_BTFG_T abns_btfg;
    ABNS_PRINT_ENTER();    
    memset(&abns_btfg,0,sizeof(ABNS_BTFG_T));
    ret = abns_read_emmc_raw_part(HISI_RSV_PART_NAME_STR, 0, (char*)&abns_btfg, sizeof(ABNS_BTFG_T));
    if (0 != ret)
    {
        ABNS_PRINT_ERR("abns_read_emmc_raw_part failed : %d!\n", ret);
    }
    if(abns_btfg.magic != ABNS_INFO_MAGIC){
        ABNS_PRINT_ERR("ABNS_INFO_MAGIC Error: 0x%X!\n", abns_btfg.magic);
        return -1;
    }
    ABNS_PRINT_EXIT();
    return abns_btfg.boot_flag;
}

static int abns_save_info_txt(ABNS_INFO_T* abns_info, char* log_path)
{
    int ret = -1;
    char *pdata = NULL;

    if (unlikely(NULL == abns_info) || unlikely(NULL == log_path))
    {
        ABNS_PRINT_INVALID_PARAMS("abns_info: %p,log_path: %p\n", abns_info,log_path);
        return -1;
    }

    pdata = (char *)bfmr_malloc(BFMR_TEMP_BUF_LEN);
    if (NULL == pdata)
    {
        ABNS_PRINT_ERR("abns_malloc failed!\n");
        return -1;
    }
    memset(pdata, 0, BFMR_TEMP_BUF_LEN);
    snprintf(pdata, BFMR_TEMP_BUF_LEN - 1, ABNS_INFO_FILE_CONTENT_FORMAT,
        abns_info->asctime,abns_info->abns_magic,abns_info->abns_stage,
        abns_info->abns_tpye,abns_info->abns_tpye_name,abns_info->sw_mode,
        abns_info->run_mode,abns_info->verifiedbootmod);
    ret = abns_save_log_file(log_path, ABNS_INFO_FILE_NAME, (void *)pdata, strlen(pdata), 0);
    if (0 != ret)
    {
        ABNS_PRINT_ERR("save [%s] failed!\n", ABNS_INFO_FILE_NAME);
    }

    bfmr_free(pdata);
    return ret;
}

static int abns_save_fastboot_log(char*dst_path, char*dst_filenm,char*src_path)
{
    long file_size=0;
    char *data_buf = NULL;
    int ret = -1;

    if((NULL == dst_path) || (NULL == src_path) || (NULL == dst_filenm)){
        ABNS_PRINT_INVALID_PARAMS("dst_path=%p,src_path=%p,dst_filenm=%p\n", dst_path,src_path,dst_filenm);
        return -1;
    }

    file_size= abns_get_proc_file_length(src_path);
    if (file_size <= 0)
    {
        goto out;
    }

    data_buf = (char *)bfmr_malloc(file_size + 1);
    if (NULL == data_buf)
    {
        ABNS_PRINT_ERR("bfmr_malloc failed!\n");
        goto out;
    }

    memset(data_buf, 0, file_size + 1);
    abns_capture_log_from_src_file(data_buf,file_size+1,src_path);
    abns_save_log_file(dst_path,dst_filenm,data_buf,file_size + 1,0);
    ret=0;
out:

    if (NULL != data_buf)
    {
        bfmr_free(data_buf);
    }
    return ret;
}

static int abns_save_kmsg_log(char*dst_path)
{

    long file_size=0;
    char *data_buf = NULL;
    int ret =-1;

    if(NULL == dst_path){
        ABNS_PRINT_INVALID_PARAMS("dst_path=%p\n", dst_path);
        return -1;
    }

    file_size= abns_get_file_length(kmesglog_path);
    if (file_size <= 0)
    {
        goto out;
    }
    data_buf = (char *)bfmr_malloc(file_size + 1);
    if (NULL == data_buf)
    {
        ABNS_PRINT_ERR("bfmr_malloc failed!\n");
        goto out;
    }
    memset(data_buf, 0, file_size + 1);
    abns_capture_log_from_src_file(data_buf,file_size,kmesglog_path);
    abns_save_log_file(dst_path,ABNS_A_LOG_KMEGSLOG,data_buf,file_size + 1,0);
    ret=0;
out:
    if (NULL != data_buf)
    {
        bfmr_free(data_buf);
    }
    return ret;

}


int abns_find_oldest_log_by_history(const char* path_history,char*poldest)
{
    mm_segment_t fs;
    loff_t pos;
    ssize_t ret = 1;
    struct file* fp = NULL;
    char* p = NULL;
    char pBuff[MAXSIZES_PATH] = {0};

    if((path_history == NULL)||(poldest==NULL)){
        ABNS_PRINT_INVALID_PARAMS("path_history=%p,poldest=%p\n",path_history,poldest);
        return -1;
    }

    memset(poldest,0,MAXSIZES_PATH);

    fs = get_fs();
    set_fs(KERNEL_DS);/*lint !e501 */
    pos = 0;

    fp = filp_open(path_history, O_RDONLY, 0);
    if (IS_ERR(fp)) {
        ABNS_PRINT_ERR("%s():%d:Can not find file:%s\n", __func__, __LINE__, path_history);
        return -1;
    }

    while (1) {
        ret = abns_read_line(fp, pBuff, &pos);

        if(-1 == ret) {
            ABNS_PRINT_ERR("%s():%d:end to read\n", __func__, __LINE__);
            break;
        }
        else if (1 == ret) {
            continue;
        }

        if( strlen(pBuff) < ABNS_A_LOG_DIR_NAME_LEN){
            continue;
        }

        p = strstr(pBuff, "abns_");
        if (p) {
            if(strlen(poldest)==0){
                strncpy(poldest,pBuff,strlen(pBuff));
                continue;
            }
            ret=strncmp(pBuff, poldest, ABNS_A_LOG_DIR_NAME_LEN);
            if(ret < 0){
                memset(poldest,0,MAXSIZES_PATH);
                strncpy(poldest,pBuff,ABNS_A_LOG_DIR_NAME_LEN);
            }
        }
    }
    if(NULL != fp) {
        filp_close(fp, NULL);
        fp= NULL;
    }
    set_fs(fs);
    return 0;

}



/*delete oldest log dir*/
void abns_check_log_space(void)
{

    int ret=-1;
    char poldest[MAXSIZES_PATH] = {0};
    char pdel[MAXSIZES_PATH] = {0};

    /*if log dir num eq 10s,then delete oldest one.*/
    if( abns_get_log_count(ABNS_LOG_ROOT_PATH) >= ABNS_LOG_MAX_COUNT){
        ret=abns_find_oldest_log_by_history(ABNS_HISTORY_FILE_NAME,poldest);
        //find the oldest log in abns_history.txt
        if((ret==0)&&(strlen(poldest)!=0)){
            snprintf(pdel,ABNS_LOG_PATH_LEN-1, "%s/%s", ABNS_LOG_ROOT_PATH,poldest);
            abns_delete_dir(pdel);
        }
    }
}


static int abns_capture_system_log(ABNS_INFO_T* abns_info)
{
    char log_buf[ABNS_LOG_PATH_LEN]={0};
    int ret = -1;
    char *log_full_path=log_buf;

    if(NULL == abns_info){
        ABNS_PRINT_INVALID_PARAMS(" abns_info=%p \n", abns_info);
        return -1;
    }
    ABNS_PRINT_KEY_INFO("============ wait for log part start =============\n");
    if (0 != abns_wait_for_part_mount_with_timeout(abns_get_bfmr_log_part_mount_point(),
        ABNS_WAIT_FOR_LOG_PART_TIMEOUT))
    {
        BFMR_PRINT_ERR("[%s] is not ready, the boot fail logs can't be saved!\n", abns_get_bfmr_log_part_mount_point());
        goto __out;
    }
    ABNS_PRINT_KEY_INFO("============ wait for log part end =============\n");

    ret = abns_create_log_path(ABNS_LOG_ROOT_PATH);

    if(ret == 0){
        snprintf(log_full_path,ABNS_LOG_PATH_LEN-1,ABNS_A_LOG_DIR_NAME_FORMAT,ABNS_LOG_ROOT_PATH,abns_info->asctime);
        /*check log times,if more than 10 times,then delete oldest one*/
        abns_check_log_space();
        ret=abns_create_log_path(log_full_path);
        if(ret == 0){
            abns_save_info_txt(abns_info,log_full_path);
            abns_save_fastboot_log(log_full_path,ABNS_A_LOG_FASTBOOT,fastboot_log_path);
            abns_save_fastboot_log(log_full_path,ABNS_A_LOG_LAST_FASTBOOT,last_fastboot_log_path);
            abns_save_kmsg_log(log_full_path);
        }else{
            ABNS_PRINT_ERR("abns_create_log_path for one exception [%d] failed!\n", ret);
        }

    }
    memset(log_buf, 0,ABNS_LOG_PATH_LEN);
    snprintf(log_buf,ABNS_LOG_PATH_LEN-1,"abns_%s-%s,\n",abns_info->asctime,abns_info->abns_tpye_name);
    ret= abns_save_log_file(ABNS_LOG_ROOT_PATH, ABNS_HISTORY_FILE_NAME,log_buf, strlen(log_buf), 1);
    if (0 != ret)
    {
        ABNS_PRINT_ERR("save [%s] failed!\n", ABNS_HISTORY_FILE_NAME);
    }
__out:

    return ret;
}

int abns_get_system_time(char*asctime)
{
    struct rtc_time tm_android;
    struct timeval  tv_android;
    if(NULL == asctime){
        ABNS_PRINT_ERR("asctime is null!");
        return -1;
    }

    memset(asctime,0,TIME_STR_LEN);
    memset(&tm_android, 0, sizeof(struct rtc_time));
    memset(&tv_android, 0, sizeof(struct timeval));

    do_gettimeofday(&tv_android);

    tv_android.tv_sec -= sys_tz.tz_minuteswest*60;/*lint !e647 */
    rtc_time_to_tm(tv_android.tv_sec, &tm_android);

    snprintf(asctime, TIME_STR_LEN-1, "%04d%02d%02d%02d%02d%02d",
        tm_android.tm_year + 1900, tm_android.tm_mon + 1, tm_android.tm_mday,
        tm_android.tm_hour, tm_android.tm_min, tm_android.tm_sec);

    return 0;
}

int get_abns_type_name(ABNS_INFO_T* abns_info)
{
    int i=0;
    if(abns_info==NULL){
        ABNS_PRINT_ERR("abns_info is FAIL!");
        return -1;
    }
    for(i=ABNS_NORMAL;i<ABNS_CHIPSETS_TEMP_HIGH;i++)
    {
         if((abns_info->abns_tpye==i) && (i == abns_type_map[i].num)){
            snprintf(abns_info->abns_tpye_name,ABNS_PARAM_STR_LEN-1,"%s", abns_type_map[i].name);
            return 0;
         }
    }

    ABNS_PRINT_ERR("abns internal error:can not find expt. type name!");
    snprintf(abns_info->abns_tpye_name,ABNS_PARAM_STR_LEN-1,"%s", abns_type_map[ABNS_NORMAL].name);
    return -2;
}
static void abns_save_all_log(struct work_struct* abns_work)
{
    ABNS_INFO_T* abns_info = NULL;
    int ret=0;

    if(NULL == abns_work){
        return;
    }

    abns_info = container_of(abns_work, ABNS_INFO_T, abns_work);
    if(NULL == abns_info){
        return;
    }
    //msleep_interruptible(30000);
    abns_info->abns_magic=ABNS_INFO_MAGIC;
    abns_info->abns_stage=get_unclean_shutdown_flag();
    abns_info->abns_tpye=g_abns_type;
    ret = get_abns_type_name(abns_info);
    if(ret!=0){
        goto out;
    }
    //get abns_info->asctime
    ret=abns_get_system_time(abns_info->asctime);
    if(ret!=0){
        goto out;
    }
    abns_info->sw_mode=g_abns_swtype;
    abns_info->run_mode=g_abns_runmode;
    abns_info->verifiedbootmod=g_abns_verifiedbootmod;
    abns_info->is_info_valid=ABNS_TRUE;
    ret=abns_capture_system_log(abns_info);
    if(ret!=0){
        goto out;
    }

out:
    bfmr_free(abns_info);
    abns_info=NULL;
    return ;
}

int start_save_abns_log(void)
{
    ABNS_INFO_T* abns_info = NULL;

    abns_info = (ABNS_INFO_T*)bfmr_malloc(sizeof(ABNS_INFO_T));
    if(NULL == abns_info){
        ABNS_PRINT_ERR("kmalloc is FAIL!");
        return -1;
    }

    memset(abns_info, 0, sizeof(ABNS_INFO_T));
    INIT_WORK(&(abns_info->abns_work),abns_save_all_log);
    schedule_work(&(abns_info->abns_work));
    return 0;/*lint !e429 */
}

int abns_check(void)
{
    if(g_abns_type == ABNS_NORMAL)
    {
        ABNS_PRINT_KEY_INFO("This is ABNS_NORMAL:%d!",g_abns_type);
    }else{
        ABNS_PRINT_KEY_INFO("This is ABNS exception:%d!",g_abns_type);
        start_save_abns_log();
    }
    return 0;
}

static int __init abns_init(void)
{
    if( g_abns_type == ABNS_NORMAL ){
        return 0;
    }
    abns_check();
    return 0;
}

late_initcall(abns_init);

