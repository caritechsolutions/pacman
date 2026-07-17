#include "app_config.h"

#if SAMBA_SUPPORT
#include "app.h"
#include "app_module.h"
#include "app_samba_db.h"
#include "app_windows.h"
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define LOCAL_DIR_HEAD    "/home/gx/local/samba/"
#define MOUNT_CMD    "mount -t cifs -o"
#define UMOUNT_CMD    "umount -f"
#define MKDIR_CMD    "mkdir -p"

#define DEFAULT_SAMBA_USERNAME    "stb"

typedef struct _AppSambaCtrl
{
    bool samba_try_mount;
    handle_t samba_connect_thread;
    handle_t samba_disconnect_thread;
    ubyte64 if_dev;
}AppSambaCtrl;

static AppSambaCtrl thiz_samba_ctrl_list[MAX_SAMBA_SVR_NUM];
static AppSambaInfoClass thiz_samba_info_list[MAX_SAMBA_SVR_NUM];
//static handle_t thiz_samba_wait_thread;

extern status_t app_mcentre_network_plug_out(void);

static bool _samba_parse_mounts_line(AppSambaInfoClass *inf, char *mounts_line)
{
    if((inf == NULL) || (mounts_line == NULL))
        return false;

   //app_log_error("[%s]%d, mounts line: %s \n", __func__, __LINE__, mounts_line);
    if((strstr(mounts_line, "cifs")) != NULL
         && (strstr(mounts_line, inf->samba_svr_path)) != NULL)
        return true;

    return false;
}

static bool _samba_check_mounts(AppSambaInfoClass *inf, int timeout_ms)
{
    uint32_t tick;
    FILE *fp = NULL;
    char ret_buf[100] = {0};
    bool ret = false;

    if(inf == NULL)
        return false;

    if((fp = fopen("/proc/mounts", "r")) == NULL)
        return false;

    tick = GxCore_TickStart(timeout_ms);
    do
    {
        memset(ret_buf, 0, sizeof(ret_buf));
        rewind(fp);
        while(fgets(ret_buf, sizeof(ret_buf) - 1, fp) != NULL)
        {
            if(_samba_parse_mounts_line(inf, ret_buf))
            {
                //app_log_error("[%s]%d, mounts line: %s \n", __func__, __LINE__, ret_buf);
                ret = true;
                break;
            }
        }

        if((ret == true) || (timeout_ms < 200))
            break;
        GxCore_ThreadDelay(200);
    }while(!GxCore_TickEnd(tick));

    fclose(fp);

    return ret;
}

static void _samba_umount_all_mounts(void)
{
	FILE *fp = NULL;
	FILE *read_fp = NULL;
	char ret_buf[256];
	char cmd_str[100];
	char *begin = NULL;
	char *end = NULL;

	if((fp = fopen("/proc/mounts", "r")) == NULL)
		return;

	memset(ret_buf, 0, sizeof(ret_buf));
	while(fgets(ret_buf, sizeof(ret_buf)-1, fp) != NULL)
	{
		end = strstr(ret_buf, " cifs ");
		if(end)
		{
			*end = '\0';
			begin = strrchr(ret_buf, ' ');
			if(begin&&(strncmp(begin, LOCAL_DIR_HEAD, strlen(LOCAL_DIR_HEAD))==0))
			{
				snprintf(cmd_str, sizeof(cmd_str)-1, "%s %s", UMOUNT_CMD, begin);
				read_fp = popen(cmd_str, "r");
				if(read_fp)
				{
					pclose(read_fp);
				}
			}
		}
		memset(ret_buf, 0, sizeof(ret_buf));
		read_fp = NULL;
    		begin = NULL;
		end = NULL;
	}

	fclose(fp);
}

static bool _samba_parse_ps_line(AppSambaInfoClass *inf, char *ps_line)
{
    char *path = NULL;
    bool ret = false;

    if((inf)&&(ps_line)&&(inf->samba_svr_path))
    {
        //app_log_error("[%s]%d, ps line: %s \n", __func__, __LINE__, ps_line);
        path = (char *)GxCore_Mallocz(strlen(inf->samba_svr_path)+3);
	 sprintf(path, " %s ", inf->samba_svr_path);
        if((strstr(ps_line, "exe"))&&(strstr(ps_line, " cifs "))&&(strstr(ps_line, path)))
        {
            ret = true;
        }
        GxCore_Free(path);
    }
    return ret;
}

static int _get_ps_pid(const char *ps_line)
{
    char *p = NULL;
    char *num_str = NULL;
    int num = 0;

    if(ps_line == NULL)
        return 0;

    if((p = strstr(ps_line, "root")) != NULL)
    {
        num_str = (char*)GxCore_Calloc(p - ps_line, 1);
        if(num_str != NULL)
        {
            memmove(num_str, ps_line, p - ps_line - 1);
            num = atoi(num_str);
            GxCore_Free(num_str);
        }
    }

    return num;
}

static void _kill_ps_pid(int pid)
{
    char kill_cmd[20] = {0};

    sprintf(kill_cmd, "kill -9 %d", pid);
    system(kill_cmd);
}

static void _samba_kill_mount(AppSambaInfoClass *inf)
{
    FILE *fp = NULL;
    char ret_buf[100] = {0};
    char log_path[20] = {0};
    int samba_pid = 0;

    if(inf == NULL)
        return;

    sprintf(log_path, "/tmp/ps%d.log", inf->samba_id);
    strcat(ret_buf, "ps>");
    strcat(ret_buf, log_path);
    system(ret_buf);

    fp = fopen(log_path, "r");
    if(fp != NULL)
    {
        memset(ret_buf, 0, sizeof(ret_buf));
        while(fgets(ret_buf, sizeof(ret_buf) - 1, fp) != NULL)
        {
            if(_samba_parse_ps_line(inf, ret_buf) == true)
            {
            //    app_log_error("[%s]%d, ps line: %s \n", __func__, __LINE__, ret_buf);
                samba_pid = _get_ps_pid(ret_buf);
                _kill_ps_pid(samba_pid);
                break;
            }
        }

        fclose(fp);
    }

    GxCore_FileDelete(log_path);

    return;
}

static inline bool _check_samba_id(uint32_t samba_id)
{
    bool ret = false;

    if((samba_id > 0)
            && (samba_id <= MAX_SAMBA_SVR_NUM)
            && (thiz_samba_info_list[samba_id - 1].samba_id == samba_id))
    {
        ret = true;
    }

    return ret;
}

/*
 * free return value if != NULL
 */
static char *_samba_mount_cmd_genrerate(AppSambaInfoClass *samba_info)
{
    char *cmd_str = NULL;

    if((samba_info == NULL)
            || (samba_info->samba_svr_path == NULL)
            || (strlen(samba_info->samba_svr_path) == 0)
            || (samba_info->samba_local_path == NULL)
            || (GxCore_FileExists(samba_info->samba_local_path) == GXCORE_FILE_UNEXIST))
        return NULL;
    //eg: mount -t cifs -o username=xxxxx,password=xxxx //192.168.111.254/test /home/gx/local/samba/
    cmd_str = GxCore_Calloc(300, 1);
    if(cmd_str == NULL)
        return NULL;
    sprintf(cmd_str, "%s", MOUNT_CMD);
    //sprintf(cmd_str, "%s \"%s\" %s", MOUNT_CMD, samba_info->samba_svr_path, samba_info->samba_local_path);
    if((samba_info->samba_username != NULL) && (strlen(samba_info->samba_username) > 0))
    {
        strcat(cmd_str, " username=");
        strcat(cmd_str, samba_info->samba_username);
    }
    else
    {
        if(samba_info->samba_username != NULL)
        {
            strcat(cmd_str, " username=");
            strcat(cmd_str, DEFAULT_SAMBA_USERNAME);
        }
    }

    if((samba_info->samba_passwd != NULL) && (strlen(samba_info->samba_passwd) > 0))
    {
        strcat(cmd_str, ",password=");
        strcat(cmd_str, samba_info->samba_passwd);
    }
    strcat(cmd_str," ");
    strcat(cmd_str,samba_info->samba_svr_path);
    strcat(cmd_str," ");
    strcat(cmd_str,samba_info->samba_local_path);


    //app_log_error("[%s]cmd_str: %s\n", __func__, cmd_str);

    return cmd_str;
}

#if 1
/*
 * free return value if != NULL
 */
static char *_samba_umount_cmd_genrerate(AppSambaInfoClass *samba_info)
{
    char *cmd_str = NULL;
    int str_len = 0;

    if((samba_info == NULL)
            || (samba_info->samba_local_path == NULL)
            || (GxCore_FileExists(samba_info->samba_local_path) == GXCORE_FILE_UNEXIST))
        return NULL;

    str_len = strlen(UMOUNT_CMD);
    str_len += strlen(samba_info->samba_local_path);
    str_len += 5;

    cmd_str = GxCore_Calloc(str_len, 1);
    if(cmd_str == NULL)
        return NULL;

    sprintf(cmd_str, "%s %s", UMOUNT_CMD, samba_info->samba_local_path);
    //app_log_error("[%s]cmd_str: %s\n", __func__, cmd_str);

    return cmd_str;
}
#endif

static void _samba_disconnect_thread(void *arg)
{
    AppSambaInfoClass *inf = (AppSambaInfoClass*)arg;
    AppSambaCtrl *p_ctrl = NULL;
    char *cmd_str = NULL;

    GxCore_ThreadDetach();

    p_ctrl = &thiz_samba_ctrl_list[inf->samba_id - 1];
    cmd_str = _samba_umount_cmd_genrerate(&thiz_samba_info_list[inf->samba_id - 1]);
    if(cmd_str != NULL)
    {
        system(cmd_str);
        GxCore_Free(cmd_str);
        cmd_str = NULL;
    }

    p_ctrl->samba_disconnect_thread = 0;
}

static uint32_t _samba_disconnect(uint32_t samba_id)
{
    AppSambaCtrl *p_ctrl = NULL;

    if(_check_samba_id(samba_id) == true)
    {
        p_ctrl = &thiz_samba_ctrl_list[samba_id - 1];
        p_ctrl->samba_try_mount = false;

        if(p_ctrl->samba_connect_thread > 0)
        {
            _samba_kill_mount(&thiz_samba_info_list[samba_id - 1]);
        }

#if 1
        GxCore_ThreadCreate("samba_disconnect", &thiz_samba_ctrl_list[samba_id - 1].samba_disconnect_thread, _samba_disconnect_thread,
                (void*)(&thiz_samba_info_list[samba_id - 1]), 10 * 1024, GXOS_DEFAULT_PRIORITY);

#else
        pid_t pid;

        if((pid = fork()) < 0)
        {
            //goto connect_ret;
        }
        else if(pid == 0) //exec umount
        {
            execl("/bin/umount", "umount", "-f", thiz_samba_info_list[samba_id - 1].samba_local_path, NULL);
        }
        else //check mount ret
        {
            memset(p_ctrl->if_dev, 0, sizeof(ubyte64));
            ret = samba_id;
        }

#endif
    }

    return samba_id;
}

#if 0
static status_t _samba_connect_all(void)
{
    int i;
    AppSambaInfoClass *p_inf = NULL;

    for(i = 0; i < MAX_SAMBA_SVR_NUM; i++)
    {
        p_inf = &thiz_samba_info_list[i];
        if(p_inf->samba_enable > 0)
        {
            app_samba_connect(p_inf->samba_id);
        }
    }

    return GXCORE_SUCCESS;
}
#endif

static status_t _samba_disconnect_all(void)
{
    int i;
    AppSambaInfoClass *p_inf = NULL;

    for(i = 0; i < MAX_SAMBA_SVR_NUM; i++)
    {
        p_inf = &thiz_samba_info_list[i];
        if(app_samba_check_connect(p_inf->samba_id) != SAMBA_STATUS_DISCONNECTED)
        {
            _samba_disconnect(p_inf->samba_id);
        }
    }

    return GXCORE_SUCCESS;
}

#if 0
static void _samba_wait_thread(void *arg)
{
    while(1)
    {
        if(wait(NULL) < 0)
        {
            GxCore_ThreadDelay(10000);
        }
    }
}
#endif

status_t app_samba_init(void)
{

    memset(thiz_samba_info_list, 0, sizeof(thiz_samba_info_list));
    app_samba_db_int(&thiz_samba_info_list);
    //_samba_connect_all();

//    GxCore_ThreadCreate("samba_wait", &thiz_samba_wait_thread, _samba_wait_thread,
//                        NULL, 8 * 1024, GXOS_DEFAULT_PRIORITY);

    return GXCORE_SUCCESS;
}

status_t app_samba_destroy(void)
{
    memset(thiz_samba_info_list, 0, sizeof(thiz_samba_info_list));
    _samba_disconnect_all();

    return GXCORE_SUCCESS;
}

AppSambaListClass *app_get_samba_List(void)
{
    static AppSambaListClass ret_list;
    AppSambaIDListClass *idlist = NULL;
    int i = 0;

    memset(&ret_list, 0, sizeof(AppSambaListClass));
    idlist = app_samba_db_get_idlist();
    for(i=0; i<idlist->id_num; i++)
    {
        if(_check_samba_id(idlist->id_list[i]) == true)
        {
            memmove(&(ret_list.samba_list[ret_list.samba_num++]),
                    &thiz_samba_info_list[idlist->id_list[i]-1], sizeof(AppSambaInfoClass));
        }
    }
    return &ret_list;
}

int32_t app_get_samba_num(void)
{
    int i = 0;
    int count = 0;

    for(i = 0; i < MAX_SAMBA_SVR_NUM; i++)
    {
        if(_check_samba_id(thiz_samba_info_list[i].samba_id) > 0)
        {
            count++;
        }
    }

    return count;
}

AppSambaInfoClass *app_get_samba_by_id(uint32_t samba_id)
{
    AppSambaInfoClass *ret_inf = NULL;

    if(_check_samba_id(samba_id) == true)
    {
        ret_inf = &thiz_samba_info_list[samba_id - 1];
    }

    return ret_inf;
}

uint32_t app_samba_new(AppSambaInfoClass *samba_info)
{
    int i = 0;
    AppSambaInfoClass *p_info = NULL;

    if((samba_info == NULL) || (samba_info->samba_svr_path == NULL))
        return 0;

    if(app_samba_svr_exist(samba_info->samba_svr_path) > 0)
        return 0;

    for(i = 0; i < MAX_SAMBA_SVR_NUM; i++)
    {
        if(thiz_samba_info_list[i].samba_id == 0)
        {
            p_info = &thiz_samba_info_list[i];
            memset(p_info, 0, sizeof(AppSambaInfoClass));
            strlcpy(p_info->samba_svr_path, samba_info->samba_svr_path, sizeof(p_info->samba_svr_path));
            if(samba_info->samba_username != NULL)
                strlcpy(p_info->samba_username, samba_info->samba_username, sizeof(p_info->samba_username));
            if(samba_info->samba_passwd != NULL)
                strlcpy(p_info->samba_passwd, samba_info->samba_passwd, sizeof(p_info->samba_passwd));

            p_info->samba_id = i + 1;

            app_samba_db_add(p_info);

            return thiz_samba_info_list[i].samba_id;
        }
    }

    return 0;
}


uint32_t app_samba_modify(uint32_t samba_id, AppSambaInfoClass *new_info)
{
    if(_check_samba_id(samba_id) == false)
        return 0;

    if((new_info == NULL) || (new_info->samba_svr_path == NULL))
        return 0;

    if(app_samba_check_connect(samba_id) != SAMBA_STATUS_DISCONNECTED)
        _samba_disconnect(samba_id);

    memcpy(&thiz_samba_info_list[samba_id - 1], new_info, sizeof(AppSambaInfoClass));
    thiz_samba_info_list[samba_id - 1].samba_id = samba_id;
    app_samba_db_modify(samba_id, new_info);

    return samba_id;
}

uint32_t app_samba_delete(uint32_t samba_id)
{
    if(_check_samba_id(samba_id) == false)
        return 0;

    app_samba_disconnect(samba_id);

    memset(&thiz_samba_info_list[samba_id - 1], 0, sizeof(AppSambaInfoClass));
    app_samba_db_delete(samba_id);

    return samba_id;
}

static status_t _samba_get_local_dir(uint32_t samba_id, char (*ret_path)[MAX_SAMBA_LOCAL_PATH_LEN + 1])
{
    char *path = NULL;

    if((ret_path == NULL) || (*ret_path == NULL))
        return GXCORE_ERROR;

    path = *ret_path;
    memset(path, 0, MAX_SAMBA_LOCAL_PATH_LEN + 1);
    snprintf(path,MAX_SAMBA_LOCAL_PATH_LEN, "%s%d", LOCAL_DIR_HEAD, samba_id);

    return GXCORE_SUCCESS;
}

static status_t _samba_make_local_dir(char *local_path)
{
    char cmd_str[100] = {0};

    if((local_path == NULL) || (strlen(local_path) == 0))
        return GXCORE_ERROR;

    snprintf(cmd_str, sizeof(cmd_str) - 1, "%s %s", MKDIR_CMD, local_path);

    system(cmd_str);

    if(GxCore_FileExists(local_path) == GXCORE_FILE_EXIST)
        return GXCORE_SUCCESS;
    else
        return GXCORE_ERROR;
}

static void _samba_connect_thread(void *arg)
{
    AppSambaInfoClass *inf = (AppSambaInfoClass*)arg;
    AppSambaCtrl *p_ctrl = NULL;
    char *cmd_str = NULL;

    GxCore_ThreadDetach();

    if(inf == NULL)
        goto connect_ret;

    if(_samba_get_local_dir(inf->samba_id, &inf->samba_local_path) == GXCORE_ERROR)
        goto connect_ret;

    if(_samba_make_local_dir(inf->samba_local_path) == GXCORE_ERROR)
        goto connect_ret;

    cmd_str = _samba_mount_cmd_genrerate(inf);
    if(cmd_str == NULL)
        goto connect_ret;

    p_ctrl = &thiz_samba_ctrl_list[inf->samba_id - 1];
    while(p_ctrl->samba_try_mount == true)
    {
        p_ctrl->samba_try_mount = false;
#if 1
        FILE *read_fp = NULL;
        bool ret = false;
        read_fp = popen(cmd_str, "r");
        if (read_fp != NULL)
        {
            if((ret = _samba_check_mounts(inf, 5000)) == true)
            {
                //app_log_error("[%s]%d: samba_id: %d, cifs OK!!!!\n", __func__, __LINE__, inf->samba_id);
            }
            else
            {
                //app_log_error("[%s]%d: samba_id: %d, cifs ERR!!!\n", __func__, __LINE__, inf->samba_id);
                _samba_kill_mount(inf);
            }

            pclose(read_fp);
        }
        else
        {
        }

        if(ret == true)
        {
            ubyte64 cur_dev;

            memset(cur_dev, 0, sizeof(ubyte64));
            app_send_msg_exec(GXMSG_NETWORK_GET_CUR_DEV, &cur_dev);

            memset(p_ctrl->if_dev, 0, sizeof(ubyte64));
            strlcpy(p_ctrl->if_dev, cur_dev, sizeof(ubyte64));

            break;
        }
#else
        pid_t pid;
        if((pid = fork()) < 0)
        {
            goto connect_ret;
        }
        else if(pid == 0) //exec mount
        {
            execl("/bin/sh", "sh", "-c", cmd_str, NULL);
            exit(0);
        }
        else //check mount ret
        {
            if(_samba_check_mounts(inf, 5000) == true)
            {
                //    app_log_error("[%s]%d: samba_id: %d, cifs OK!!!!\n", __func__, __LINE__, inf->samba_id);
            }
            else
            {
                //   app_log_error("[%s]%d: samba_id: %d, cifs ERR!!!\n", __func__, __LINE__, inf->samba_id);
                kill(pid, SIGKILL);
            }
            while(waitpid(pid, NULL, WNOHANG) > 0);
        }
#endif
    }

connect_ret:
    if(cmd_str != NULL)
        GxCore_Free(cmd_str);

    p_ctrl->samba_try_mount = false;
    p_ctrl->samba_connect_thread = 0;
}

uint32_t app_samba_connect(uint32_t samba_id)
{
    status_t ret = 0;

    if(_check_samba_id(samba_id) == true)
    {
        if(thiz_samba_info_list[samba_id - 1].samba_enable == 0)
        {
            thiz_samba_info_list[samba_id - 1].samba_enable = 1;
            app_samba_db_modify(samba_id, &thiz_samba_info_list[samba_id - 1]);
        }

        if(app_samba_check_connect(samba_id) != SAMBA_STATUS_CONNECTED)
        {
            thiz_samba_ctrl_list[samba_id - 1].samba_try_mount = true;
            if(thiz_samba_ctrl_list[samba_id - 1].samba_connect_thread == 0)
            {
                GxCore_ThreadCreate("samba_connect", &thiz_samba_ctrl_list[samba_id - 1].samba_connect_thread, _samba_connect_thread,
                        (void*)(&thiz_samba_info_list[samba_id - 1]), 10 * 1024, GXOS_DEFAULT_PRIORITY);
            }
        }

        ret = samba_id;
    }

    return ret;
}

uint32_t app_samba_disconnect(uint32_t samba_id)
{
    uint32_t ret = 0;

    if(_check_samba_id(samba_id) == true)
    {
        if(thiz_samba_info_list[samba_id - 1].samba_enable > 0)
        {
            thiz_samba_info_list[samba_id - 1].samba_enable = 0;
            app_samba_db_modify(samba_id, &thiz_samba_info_list[samba_id - 1]);
        }

        if(app_samba_check_connect(samba_id) != SAMBA_STATUS_DISCONNECTED)
        {
            ret = _samba_disconnect(samba_id);
        }
    }

    return ret;
}

AppSambaConnectStatus app_samba_check_connect(uint32_t samba_id)
{
    AppSambaConnectStatus ret = SAMBA_STATUS_DISCONNECTED;
    ubyte64 cur_dev;

    memset(cur_dev, 0, sizeof(ubyte64));
    app_send_msg_exec(GXMSG_NETWORK_GET_CUR_DEV, &cur_dev);
    if(strlen(cur_dev) == 0)
        return ret;

    if(_check_samba_id(samba_id) == true)
    {
        if(_samba_check_mounts(&thiz_samba_info_list[samba_id - 1], 0) == true)
        {
            ret = SAMBA_STATUS_CONNECTED;
        }
        else
        {
            if(thiz_samba_ctrl_list[samba_id - 1].samba_connect_thread != 0)
            {
                ret = SAMBA_STATUS_CONNECTING;
            }
        }
    }
    return ret;
}

uint32_t app_samba_svr_exist(const char *samba_svr_path)
{
    uint32_t ret = 0;
    int i = 0;

    if(samba_svr_path == NULL)
        return 0;

    for(i = 0; i < MAX_SAMBA_SVR_NUM; i++)
    {
        if(strncmp(thiz_samba_info_list[i].samba_svr_path, samba_svr_path, MAX_SAMBA_SVR_PATH_LEN) == 0)
        {
            //app_log_error("[%s]%d samba svr exist !!!\n", __func__, __LINE__);
            ret = thiz_samba_info_list[i].samba_id;
            break;
        }
    }

    return ret;
}

static status_t _samba_network_connect_proc(void)
{
    int i;
    AppSambaInfoClass *p_inf = NULL;
    ubyte64 cur_dev;

    _samba_umount_all_mounts();
    memset(cur_dev, 0, sizeof(ubyte64));
    app_send_msg_exec(GXMSG_NETWORK_GET_CUR_DEV, &cur_dev);

    for(i = 0; i < MAX_SAMBA_SVR_NUM; i++)
    {
        p_inf = &thiz_samba_info_list[i];
        if(p_inf->samba_enable > 0)
        {
            app_samba_connect(p_inf->samba_id);
        }
    }

    return GXCORE_SUCCESS;
}

static int samba_is_valid_network(char *dev_name)
{
	int valid = 0;
	if(dev_name&&(strlen(dev_name)>0)
		&&((strncmp(dev_name, "eth", 3) ==0)
		||(strncmp(dev_name, "ra", 2)==0)
		||(strncmp(dev_name, "wlan", 4)==0))
		)
	{
		valid = 1;
	}
	return valid;
}

void app_samba_msg_proc(GxMessage *msg)
{
    if(msg == NULL)
        return;

    switch(msg->msg_id)
    {
        case GXMSG_NETWORK_STATE_REPORT:
            {
                GxMsgProperty_NetDevState *net_state = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevState);
                if(net_state == NULL)
                    break;

                if(samba_is_valid_network(net_state->dev_name)&&(net_state->dev_state == IF_STATE_CONNECTED))
                {
                    _samba_network_connect_proc();
                }
                break;
            }

        case GXMSG_NETWORK_PLUG_OUT:
            {
                ubyte64 *net_dev = GxBus_GetMsgPropertyPtr(msg, ubyte64);
                if(samba_is_valid_network(*net_dev) == 0)
                {
                    break;
                }
                if(GUI_CheckDialog(WIN_FILE_BROWSER) == GXCORE_SUCCESS) //media centre
                    app_mcentre_network_plug_out();
                break;
            }

        default:
            break;
    }
}

#endif

