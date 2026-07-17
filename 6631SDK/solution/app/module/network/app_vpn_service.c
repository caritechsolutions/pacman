#include "app.h"
#include "app_module.h"

#if VPN_SUPPORT
#ifdef ECOS_OS
#include <net/isakmp_ref.h>
#include <cyg/vpn/vpn.h>
#endif
#include <openvpn_api.h>

#include <gxos/gxcore_os_vpn.h>
#include <bus/service/gxvpnmsg.h>
#include <signal.h>
#include <sys/wait.h>

#define VPN_MAX_CONNECT_TIMES    10

#define VPN_SERVICE_NAME    "vpn service"
#define VPN_CONSOLE_DELAY_MS    1000

#define VPN_CONFIG_AUTO_KEY	"netapps>vpn_auto"
#define VPN_CONFIG_AUTO_DEFAULT    0
#ifdef LINUX_OS
#define VPN_DB_PATH    "/usr/share/app/vpn/vpn_db"
#else
#define VPN_DB_PATH    "/home/gx/vpn_db"
#endif

#define PPTP_CONFIG_FILE    "pptpvpn"
#define PPPD_PPTP_CONFIG_PATH    "/etc/ppp/peers/"PPTP_CONFIG_FILE
#define PPPD_PPTP_LINKNAME    PPTP_CONFIG_FILE

typedef struct _AppVpnCtrl
{
    AppVpnInfoClass vpn_info;
    AppVpnState vpn_state;
    bool vpn_enable;
    ubyte64 vpn_master_if_dev;
    ubyte64 vpn_ppp_if_dev;
    int vpn_try_cnt;
    handle_t vpn_mutex;
}AppVpnCtrl;

static AppVpnCtrl thiz_vpn_ctrl;
static status_t AppVpnServiceInit(handle_t self,int priority_offset);
static void AppVpnServiceDestroy(handle_t self);
static GxMsgStatus AppVpnServiceRecvMsg(handle_t self, GxMessage * Msg);
static void AppVpnServiceConsole(handle_t self);

static handle_t thread_openvpn = 0;
static int s_openvpn_start = 0;
#ifdef ECOS_OS
static int pptp_state_old = -1;
#endif

GxServiceClass app_vpn_service =
{
    VPN_SERVICE_NAME,
    AppVpnServiceInit,
    AppVpnServiceDestroy,
    AppVpnServiceRecvMsg,
    AppVpnServiceConsole,
};

static void openvpn_start(void* arg)
{
	char *conf_ovpn[2] = {
		"--config",
		NULL,
	};
	int ret = 0;

	GxCore_ThreadDetach();

	while(1)
	{
		if(1 == s_openvpn_start)
		{
			conf_ovpn[1] = app_vpn_openvpn_get_conf_file();
			app_log_min("openvpn_start -------- ovpn file: %s \n", conf_ovpn[1]);
			if(conf_ovpn[1] != NULL)
			{
				while(1)
				{
					ret = openvpn_main(2, conf_ovpn);
					if(ret == 0)
					{
						app_log_min("stop openvpn_main \n ");
						s_openvpn_start = 0;
						goto STOP;
					}
				}
			}
		}
		GxCore_ThreadDelay(400);

	STOP:
		ret = 1;
	}

	return;
}

static status_t _vpn_set_db(AppVpnInfoClass *vpn_info)
{
    FILE *fp = NULL;
    char *vpn_type[] = {"pptp", "l2tp", "openvpn"};
    char line_str[100] = {0};

    if(vpn_info == NULL)
        return GXCORE_ERROR;

    if((fp = fopen(VPN_DB_PATH, "w+")) == NULL)
        return GXCORE_ERROR;

    //type
    sprintf(line_str, "%s\n", vpn_type[vpn_info->vpn_type]);
    fputs(line_str, fp);

    //pre_shared key
    if( vpn_info->vpn_type == VPN_TYPE_L2TP)
    {
        memset(line_str, 0, sizeof(line_str));
        sprintf(line_str, "%s\n", vpn_info->vpn_psk);
        fputs(line_str, fp);
    }
    //server
    memset(line_str, 0, sizeof(line_str));
    sprintf(line_str, "%s\n", vpn_info->vpn_server);
    fputs(line_str, fp);

    //username
    memset(line_str, 0, sizeof(line_str));
    sprintf(line_str, "%s\n", vpn_info->vpn_username);
    fputs(line_str, fp);

    //password
    memset(line_str, 0, sizeof(line_str));
    sprintf(line_str, "%s\n", vpn_info->vpn_password);
    fputs(line_str, fp);
    fflush(fp);
    fsync(fileno(fp));
    fclose(fp);

    return GXCORE_SUCCESS;
}

static status_t _vpn_get_db(AppVpnInfoClass *vpn_info)
{
    FILE *fp = NULL;
    char line_str[100] = {0};

    if(vpn_info == NULL)
        return GXCORE_ERROR;

    if((fp = fopen(VPN_DB_PATH, "r")) == NULL)
    {
        app_log_error("_vpn_get_db = %s \r\n", VPN_DB_PATH);
        return GXCORE_ERROR;
    }

    memset(vpn_info, 0, sizeof(AppVpnInfoClass));

    //type
    fgets(line_str, sizeof(line_str), fp);
    if(line_str[strlen(line_str) - 1] == '\n')
        line_str[strlen(line_str) - 1] = 0;
    if(strcmp("pptp", line_str) == 0)
        vpn_info->vpn_type = VPN_TYPE_PPTP;
    else if(strcmp("l2tp", line_str) == 0)
        vpn_info->vpn_type = VPN_TYPE_L2TP;
    else if(strcmp("openvpn", line_str) == 0)
        vpn_info->vpn_type = VPN_TYPE_OPENVPN;

    //Pre-shared keys
    if( vpn_info->vpn_type == VPN_TYPE_L2TP)
    {
        memset(line_str, 0, sizeof(line_str));
        fgets(line_str, sizeof(line_str), fp);
        if (line_str[strlen(line_str) - 1] == '\n')
            line_str[strlen(line_str) - 1] = 0;
        strlcpy(vpn_info->vpn_psk, line_str, sizeof(vpn_info->vpn_psk));
    }
    //server
    memset(line_str, 0, sizeof(line_str));
    fgets(line_str, sizeof(line_str), fp);
    if(line_str[strlen(line_str) - 1] == '\n')
        line_str[strlen(line_str) - 1] = 0;
    strlcpy(vpn_info->vpn_server, line_str, sizeof(vpn_info->vpn_server));

    //username
    memset(line_str, 0, sizeof(line_str));
    fgets(line_str, sizeof(line_str), fp);
    if(line_str[strlen(line_str) - 1] == '\n')
        line_str[strlen(line_str) - 1] = 0;
    strlcpy(vpn_info->vpn_username, line_str, sizeof(vpn_info->vpn_username));

    //password
    memset(line_str, 0, sizeof(line_str));
    fgets(line_str, sizeof(line_str), fp);
    if(line_str[strlen(line_str) - 1] == '\n')
        line_str[strlen(line_str) - 1] = 0;
    strlcpy(vpn_info->vpn_password, line_str, sizeof(vpn_info->vpn_password));

    fclose(fp);

    return GXCORE_SUCCESS;
}
#ifdef LINUX_OS
static status_t _vpn_pptp_config_generate(AppVpnInfoClass *vpn_info)
{
    FILE *fp = NULL;
    char line_str[100];

    if(vpn_info == NULL)
        return GXCORE_ERROR;

    if((fp = fopen(PPPD_PPTP_CONFIG_PATH, "w+")) == NULL)
        return GXCORE_ERROR;

    memset(line_str, 0, sizeof(line_str));
    sprintf(line_str, "pty \"pptp %s --nolaunchpppd\"\n", vpn_info->vpn_server);
    fputs(line_str, fp);

    memset(line_str, 0, sizeof(line_str));
    sprintf(line_str, "name %s\n", vpn_info->vpn_username);
    fputs(line_str, fp);

    memset(line_str, 0, sizeof(line_str));
    sprintf(line_str, "password %s\n", vpn_info->vpn_password);
    fputs(line_str, fp);

    memset(line_str, 0, sizeof(line_str));
    sprintf(line_str, "remotename PPTP\n");
    fputs(line_str, fp);

    memset(line_str, 0, sizeof(line_str));
    sprintf(line_str, "require-mppe-128\n");
    fputs(line_str, fp);

    memset(line_str, 0, sizeof(line_str));
    sprintf(line_str, "linkname %s\n", PPPD_PPTP_LINKNAME);
    fputs(line_str, fp);
    fflush(fp);
    fsync(fileno(fp));
    fclose(fp);

    return GXCORE_SUCCESS;
}

static void _vpn_pppd_call(void)
{
#if 0
    pid_t pid = fork();

    if(pid < 0)
        return;

    if(pid == 0) //sub proccess
    {
        execl("/usr/sbin/pppd", "pppd", "call", PPTP_CONFIG_FILE, NULL);
    }
    else
    {
        app_log_debug("###[%s]%d pid = %d###\n", __func__, __LINE__, pid);
        thiz_vpn_ctrl.vpn_fork_pid = pid;
        waitpid(thiz_vpn_ctrl.vpn_fork_pid, NULL, 0);
    }
#else
    char line_str[100] = {0};
    sprintf(line_str, "pppd call %s &", PPTP_CONFIG_FILE);
    system(line_str);
#endif
}

static status_t  _vpn_check_pppd_run_info(ubyte64 *ppp_name)
{
    FILE *fp = NULL;
    char line_str[100] = {0};

    sprintf(line_str, "/var/run/ppp-%s.pid", PPPD_PPTP_LINKNAME);
    if((fp = fopen(line_str, "r")) == NULL)
        return GXCORE_ERROR;

    memset(line_str, 0, sizeof(line_str)); //ppp id
    fgets(line_str, sizeof(line_str), fp);

    memset(line_str, 0, sizeof(line_str));
    fgets(line_str, sizeof(line_str), fp);
    if(ppp_name != NULL)
    {
        if(line_str[strlen(line_str) - 1] == '\n')
            line_str[strlen(line_str) - 1] = 0;
        memset(*ppp_name, 0, sizeof(*ppp_name));
        strlcpy(*ppp_name, line_str, sizeof(*ppp_name));
    }

    fclose(fp);

    return GXCORE_SUCCESS;
}

static void _vpn_pppd_kill(void)
{
    char line_str[100] = {0};
    char cmd_str[100] = {0};
    FILE *read_fp = NULL;
    int pid = 0;

    snprintf(cmd_str,sizeof(cmd_str)-1,"ps | grep \"pppd call "PPTP_CONFIG_FILE"\" | awk '{print $1}'");
    read_fp = popen(cmd_str, "r");
    if(read_fp)
    {
        while(fgets(line_str, sizeof(line_str), read_fp))
        {
            pid = 0;
            pid = atoi(line_str);
            if(pid>0)
            {
                kill(pid, SIGKILL);
            }
            memset(line_str, 0, sizeof(line_str));
        }
        pclose(read_fp);
    }
}
#endif

static int _vpn_openvpn_stop(void)
{
	int ret = 0;
	app_log_min("~~~~~~[%s]%d, make openvpn to stop connect!!!!!\n", __FUNCTION__, __LINE__);
	openvpn_stop();
	return ret;
}


static int _vpn_openvpn_start(void)
{
	char* conf_file = app_vpn_openvpn_get_conf_file();
	if(conf_file == NULL)
	{
		app_log_error("OpenVPN PARAM ERROR\n");
		return -1;
	}
	s_openvpn_start = 1;
	if(0 == thread_openvpn)
	{
		GxCore_ThreadCreate("openvpn_thread", &thread_openvpn, 
				openvpn_start, NULL,
				32*1024,GXOS_DEFAULT_PRIORITY);
	}

	return 0;
}

static void _vpn_init(void)
{
    memset(&thiz_vpn_ctrl, 0 , sizeof(thiz_vpn_ctrl));

    GxCore_MutexCreate(&thiz_vpn_ctrl.vpn_mutex);
    GxCore_MutexLock(thiz_vpn_ctrl.vpn_mutex);
    if(_vpn_get_db(&thiz_vpn_ctrl.vpn_info) == GXCORE_SUCCESS)
    {
        int value = 0;

        GxBus_ConfigGetInt(VPN_CONFIG_AUTO_KEY, &value, VPN_CONFIG_AUTO_DEFAULT);
        (value > 0) ? (thiz_vpn_ctrl.vpn_enable = true) : (thiz_vpn_ctrl.vpn_enable = false);
#ifdef LINUX_OS
        _vpn_pptp_config_generate(&thiz_vpn_ctrl.vpn_info);
#endif
    }
    GxCore_MutexUnlock(thiz_vpn_ctrl.vpn_mutex);
}

static status_t _vpn_state_report(AppVpnState vpn_state)
{
    GxMessage *new_msg = NULL;
    AppMsgProperty_VpnState *state = NULL;

    new_msg = GxBus_MessageNew(APPMSG_VPN_STATE_REPORT);
    state= (AppMsgProperty_VpnState*)GxBus_GetMsgPropertyPtr(new_msg, AppMsgProperty_VpnState);
    if(state != NULL)
    {
        *state = vpn_state;
        GxBus_MessageSend(new_msg);
        return GXCORE_SUCCESS;
    }

    return GXCORE_ERROR;
}
#ifdef LINUX_OS
static status_t _vpn_route_add(void)
{
    char line_str[100] = {0};
    GxMsgProperty_NetDevIpcfg ip_cfg;
    status_t ret = GXCORE_ERROR;

    memset(&ip_cfg, 0, sizeof(GxMsgProperty_NetDevIpcfg));
    strcpy(ip_cfg.dev_name, thiz_vpn_ctrl.vpn_master_if_dev);
    app_send_msg_exec(GXMSG_NETWORK_GET_IPCFG, &ip_cfg);

    if(strlen(ip_cfg.gate_way) > 0)
    {
        //add server route
        sprintf(line_str, "route add -net %s netmask 255.255.255.255 gw %s 2&> /dev/null",
                thiz_vpn_ctrl.vpn_info.vpn_server, ip_cfg.gate_way);
        system(line_str);

        //add default gw
        memset(line_str, 0, sizeof(line_str));
        sprintf(line_str, "route add default dev %s 2&> /dev/null", thiz_vpn_ctrl.vpn_ppp_if_dev);
        system(line_str);

        ret = GXCORE_SUCCESS;
    }

    return ret;
}

static status_t _vpn_route_del(void)
{
    char line_str[100] = {0};
    char cmd_str[100] = {0};
    FILE *read_fp = NULL;
    int len = 0;

    if(strlen(thiz_vpn_ctrl.vpn_ppp_if_dev) > 0)
    {
        //del server route
        snprintf(cmd_str,sizeof(cmd_str)-1,"route | grep \" %s\" | awk '{print $1}'", thiz_vpn_ctrl.vpn_ppp_if_dev);
        read_fp = popen(cmd_str, "r");
        if(read_fp)
        {
            while(fgets(line_str, sizeof(line_str), read_fp))
            {
                len = strlen(line_str);
                if(len>3)
                {
                    if((line_str[len-1]=='\n'))
                    {
                        line_str[len-1] = '\0';
                    }
                    if(strcmp(line_str, "default")!=0)
                    {
                        snprintf(cmd_str, sizeof(cmd_str)-1,"route del %s 2&> /dev/null", line_str);
                        app_log_debug("[%s]%d: %s\n", __func__, __LINE__, cmd_str);
                        system(cmd_str);
                    }
                }
                memset(line_str, 0, sizeof(line_str));
            }
            pclose(read_fp);
        }

        //del default gw
        memset(line_str, 0, sizeof(line_str));
        sprintf(line_str, "route del default %s 2&> /dev/null", thiz_vpn_ctrl.vpn_ppp_if_dev);
        app_log_debug("[%s]%d: %s\n", __func__, __LINE__, line_str);
        system(line_str);
    }

    return GXCORE_SUCCESS;
}
#endif

static status_t _vpn_start(void)
{
    ubyte64 cur_dev = {0};
    GxMsgProperty_NetDevState state;

    app_send_msg_exec(GXMSG_NETWORK_GET_CUR_DEV, &cur_dev);
    if(strlen(cur_dev) == 0)
        return -2;

    memset(&state, 0, sizeof(GxMsgProperty_NetDevState));
    strlcpy(state.dev_name, (char*)cur_dev, sizeof(state.dev_name));
    app_send_msg_exec(GXMSG_NETWORK_GET_STATE, &state);
    if(state.dev_state != IF_STATE_CONNECTED)
        return -2;
    if(thiz_vpn_ctrl.vpn_state == VPN_STATE_START || thiz_vpn_ctrl.vpn_state == VPN_STATE_OK)
    {
        return GXCORE_SUCCESS;
    }

#ifdef LINUX_OS
	if(thiz_vpn_ctrl.vpn_info.vpn_type == VPN_TYPE_PPTP)
	{
		if (GxCore_FileExists(PPPD_PPTP_CONFIG_PATH) == GXCORE_FILE_UNEXIST)
			return GXCORE_ERROR;

		_vpn_pppd_kill();
		_vpn_pppd_call();
	}
	else if(thiz_vpn_ctrl.vpn_info.vpn_type == VPN_TYPE_OPENVPN)
	{
		_vpn_openvpn_start();
	}
#endif
    strlcpy(thiz_vpn_ctrl.vpn_master_if_dev, cur_dev, sizeof(thiz_vpn_ctrl.vpn_master_if_dev));
    thiz_vpn_ctrl.vpn_state = VPN_STATE_START;
#ifdef LINUX_OS
    _vpn_check_pppd_run_info(&thiz_vpn_ctrl.vpn_ppp_if_dev);
    _vpn_state_report(thiz_vpn_ctrl.vpn_state);
#endif

#ifdef ECOS_OS
    if(strcmp(thiz_vpn_ctrl.vpn_master_if_dev, "USB3G") == 0)
    {
        memset(cur_dev, 0, sizeof(cur_dev));
        strcpy(cur_dev, "ppp0");
    }
    if (thiz_vpn_ctrl.vpn_info.vpn_type == VPN_TYPE_PPTP)
    {
        Gxcore_StartPPTP(thiz_vpn_ctrl.vpn_info.vpn_server, thiz_vpn_ctrl.vpn_info.vpn_username, thiz_vpn_ctrl.vpn_info.vpn_password, (char *)cur_dev);
        pptp_state_old = -1;
    }
    else  if (thiz_vpn_ctrl.vpn_info.vpn_type == VPN_TYPE_L2TP)
    {
        struct ike_params *ike = isakmp_build_ike(thiz_vpn_ctrl.vpn_info.vpn_server, thiz_vpn_ctrl.vpn_info.vpn_psk, (char *)cur_dev);
        if ((ike == NULL) || isakmp_set_ipsec(ike))
        {
            app_log_error("ISAKMP PARAM ERROR\n");
            return -1;
        }
        isakmp_on(ike->dst);
        Gxcore_StartL2TP(thiz_vpn_ctrl.vpn_info.vpn_server, thiz_vpn_ctrl.vpn_info.vpn_username, thiz_vpn_ctrl.vpn_info.vpn_password, (char *)cur_dev);
    }
    else  if (thiz_vpn_ctrl.vpn_info.vpn_type == VPN_TYPE_OPENVPN)
    {
        _vpn_openvpn_start();
    }

#endif
    return GXCORE_SUCCESS;
}

static status_t _vpn_stop(void)
{
#ifdef LINUX_OS
	if (thiz_vpn_ctrl.vpn_info.vpn_type == VPN_TYPE_PPTP)
	{
	    _vpn_route_del();
	    _vpn_pppd_kill();
	}
	else if(thiz_vpn_ctrl.vpn_info.vpn_type == VPN_TYPE_OPENVPN)
	{
		_vpn_openvpn_stop();
	}
#endif
#ifdef ECOS_OS
    if (thiz_vpn_ctrl.vpn_info.vpn_type == VPN_TYPE_PPTP)
    {
        Gxcore_StopPPTP();
    }
    else  if (thiz_vpn_ctrl.vpn_info.vpn_type == VPN_TYPE_L2TP)
    {
        isakmp_off();
        Gxcore_StopL2TP();
    }
    else  if (thiz_vpn_ctrl.vpn_info.vpn_type == VPN_TYPE_OPENVPN)
	{
		//s_openvpn_start = 0;
		_vpn_openvpn_stop();
		//GxCore_ThreadJoin(thread_openvpn);
	}
#endif

    memset(thiz_vpn_ctrl.vpn_master_if_dev, 0, sizeof(thiz_vpn_ctrl.vpn_master_if_dev));
    memset(thiz_vpn_ctrl.vpn_ppp_if_dev, 0, sizeof(thiz_vpn_ctrl.vpn_ppp_if_dev));
    thiz_vpn_ctrl.vpn_try_cnt = 0;
    thiz_vpn_ctrl.vpn_state = VPN_STATE_IDLE;
    _vpn_state_report(thiz_vpn_ctrl.vpn_state);

    return GXCORE_SUCCESS;
}
#ifdef LINUX_OS
static bool _vpn_check_route(char *if_name)
{
#define PROC_NET_DEV    "/proc/net/route"
    char line_str[128] = {0};
    FILE *fp = NULL;

    if((if_name == NULL) || (strlen(if_name) == 0))
        return false;

    fp = fopen(PROC_NET_DEV, "r");
    if(fp != NULL)
    {
        fgets(line_str, sizeof(line_str), fp);
        fgets(line_str, sizeof(line_str), fp);

        memset(line_str, 0, sizeof(line_str));
        while(fgets(line_str, sizeof(line_str), fp))
        {
            if(strstr(line_str, if_name) != NULL)
            {
                fclose(fp);
                return true;
            }
            memset(line_str, 0, sizeof(line_str));
        }

        fclose(fp);
    }

    return false;
}
#endif

static status_t _vpn_monitor(void)
{
#ifdef LINUX_OS
    ubyte64 ppp_dev = {0};
    if(_vpn_check_pppd_run_info(&ppp_dev) == GXCORE_SUCCESS)
    {
        strlcpy(thiz_vpn_ctrl.vpn_ppp_if_dev, ppp_dev, sizeof(thiz_vpn_ctrl.vpn_ppp_if_dev));
        if(_vpn_check_route(thiz_vpn_ctrl.vpn_ppp_if_dev) == true)
        {
            if(thiz_vpn_ctrl.vpn_state == VPN_STATE_START)
            {
                if(_vpn_route_add() == GXCORE_SUCCESS)
                {
                    thiz_vpn_ctrl.vpn_try_cnt = 0;
                    thiz_vpn_ctrl.vpn_state = VPN_STATE_OK;
                    _vpn_state_report(thiz_vpn_ctrl.vpn_state);
                }
            }
        }
        else
        {
            //connecting...
        }
    }
    else
    {
        if(thiz_vpn_ctrl.vpn_try_cnt >= VPN_MAX_CONNECT_TIMES && thiz_vpn_ctrl.vpn_info.vpn_type != VPN_TYPE_OPENVPN)
        {
            //failed
			int value = 0;

			_vpn_stop();
			GxBus_ConfigGetInt(VPN_CONFIG_AUTO_KEY, &value, VPN_CONFIG_AUTO_DEFAULT);
			if(!value)
			thiz_vpn_ctrl.vpn_enable = false;
        }
        else
        {
            _vpn_pppd_kill();
            memset(thiz_vpn_ctrl.vpn_master_if_dev, 0, sizeof(thiz_vpn_ctrl.vpn_master_if_dev));
            memset(thiz_vpn_ctrl.vpn_ppp_if_dev, 0, sizeof(thiz_vpn_ctrl.vpn_ppp_if_dev));
            _vpn_state_report(thiz_vpn_ctrl.vpn_state);
            if (thiz_vpn_ctrl.vpn_state == VPN_STATE_START)
                thiz_vpn_ctrl.vpn_try_cnt++;
        }
    }
#endif
#ifdef ECOS_OS
    if (thiz_vpn_ctrl.vpn_state == VPN_STATE_OK)
    {
            thiz_vpn_ctrl.vpn_state = VPN_STATE_OK;
            _vpn_state_report(thiz_vpn_ctrl.vpn_state);
    }
    else
    {
        if (thiz_vpn_ctrl.vpn_try_cnt >= VPN_MAX_CONNECT_TIMES && thiz_vpn_ctrl.vpn_info.vpn_type != VPN_TYPE_OPENVPN)
        {
            //failed
            int value = 0;

            _vpn_stop();
            GxBus_ConfigGetInt(VPN_CONFIG_AUTO_KEY, &value, VPN_CONFIG_AUTO_DEFAULT);
            if (!value)
                thiz_vpn_ctrl.vpn_enable = false;
        }
        else
        {
            memset(thiz_vpn_ctrl.vpn_master_if_dev, 0, sizeof(thiz_vpn_ctrl.vpn_master_if_dev));
            memset(thiz_vpn_ctrl.vpn_ppp_if_dev, 0, sizeof(thiz_vpn_ctrl.vpn_ppp_if_dev));
            _vpn_state_report(thiz_vpn_ctrl.vpn_state);
            if (thiz_vpn_ctrl.vpn_state == VPN_STATE_START)
                thiz_vpn_ctrl.vpn_try_cnt++;
        }
    }
#endif
    return GXCORE_SUCCESS;
}

static void _vpn_network_state_msg_proc(GxMsgProperty_NetDevState *dev_state)
{
    if(dev_state == NULL)
        return;

    GxCore_MutexLock(thiz_vpn_ctrl.vpn_mutex);
    if(strcmp(dev_state->dev_name, thiz_vpn_ctrl.vpn_master_if_dev) == 0)
    {
        if((dev_state->dev_state != IF_STATE_CONNECTED)
                && (thiz_vpn_ctrl.vpn_state != VPN_STATE_IDLE))
        {
            int value = 0;
            _vpn_stop();
            GxBus_ConfigGetInt(VPN_CONFIG_AUTO_KEY, &value, VPN_CONFIG_AUTO_DEFAULT);
            if(!value)
                thiz_vpn_ctrl.vpn_enable = false;
        }
    }
    GxCore_MutexUnlock(thiz_vpn_ctrl.vpn_mutex);
}

static void _vpn_network_plugout_proc(const char *net_dev)
{
    if(net_dev == NULL)
        return;

    GxCore_MutexLock(thiz_vpn_ctrl.vpn_mutex);
    if(strcmp(net_dev, thiz_vpn_ctrl.vpn_master_if_dev) == 0)
    {
        int value = 0;
        _vpn_stop();
        GxBus_ConfigGetInt(VPN_CONFIG_AUTO_KEY, &value, VPN_CONFIG_AUTO_DEFAULT);
        if(!value)
            thiz_vpn_ctrl.vpn_enable = false;

    }
    GxCore_MutexUnlock(thiz_vpn_ctrl.vpn_mutex);
}

static void _vpn_msg_get_state(AppMsgProperty_VpnState *vpn_state)
{
    if(vpn_state != NULL)
    {
        if((thiz_vpn_ctrl.vpn_state == VPN_STATE_IDLE)&&(thiz_vpn_ctrl.vpn_try_cnt>0))
        {
            *vpn_state = VPN_STATE_START;
        }
        else
        {
            *vpn_state = thiz_vpn_ctrl.vpn_state;
        }
    }
}

static void _vpn_msg_start(void)
{
    GxCore_MutexLock(thiz_vpn_ctrl.vpn_mutex);
    thiz_vpn_ctrl.vpn_enable = true;
    thiz_vpn_ctrl.vpn_try_cnt = 0;
    GxCore_MutexUnlock(thiz_vpn_ctrl.vpn_mutex);
}

static void _vpn_msg_stop(void)
{
    GxCore_MutexLock(thiz_vpn_ctrl.vpn_mutex);
    if((thiz_vpn_ctrl.vpn_state != VPN_STATE_IDLE)||(thiz_vpn_ctrl.vpn_try_cnt>0))
        _vpn_stop();
    thiz_vpn_ctrl.vpn_enable = false;
    GxCore_MutexUnlock(thiz_vpn_ctrl.vpn_mutex);
}

static void _vpn_msg_set_auto(AppMsgProperty_VpnAuto *vpn_auto)
{
    int value = 0;
    if(vpn_auto != NULL)
    {
        value = !!(*vpn_auto);
        GxBus_ConfigSetInt(VPN_CONFIG_AUTO_KEY, value);
        if(value)
        {
            thiz_vpn_ctrl.vpn_enable = true;
        }
    }
}

static void _vpn_msg_get_auto(AppMsgProperty_VpnAuto *vpn_auto)
{
    int value = 0;
    if(vpn_auto != NULL)
    {
        GxBus_ConfigGetInt(VPN_CONFIG_AUTO_KEY, &value, VPN_CONFIG_AUTO_DEFAULT);
        *vpn_auto = value;
    }
}

static void _vpn_msg_set_param(AppMsgProperty_VpnInfo *vpn_info)
{
    AppVpnInfoClass info;

    if(vpn_info == NULL)
        return;

    GxCore_MutexLock(thiz_vpn_ctrl.vpn_mutex);

    if(memcmp(vpn_info, &thiz_vpn_ctrl.vpn_info, sizeof(AppVpnInfoClass)) == 0)
    {
        GxCore_MutexUnlock(thiz_vpn_ctrl.vpn_mutex);
        return;
    }

    memcpy(&info, vpn_info, sizeof(AppVpnInfoClass));
    _vpn_set_db(&info);

    if(thiz_vpn_ctrl.vpn_state != VPN_STATE_IDLE && thiz_vpn_ctrl.vpn_info.vpn_type != VPN_TYPE_OPENVPN)
    {
        _vpn_stop();
    }

    if(_vpn_get_db(&thiz_vpn_ctrl.vpn_info) == GXCORE_SUCCESS)
    {
#ifdef LINUX_OS
        _vpn_pptp_config_generate(&thiz_vpn_ctrl.vpn_info);
#endif
    }

    GxCore_MutexUnlock(thiz_vpn_ctrl.vpn_mutex);
}

static void _vpn_msg_get_param(AppMsgProperty_VpnInfo *vpn_info)
{
    if(vpn_info == NULL)
        return;

    GxCore_MutexLock(thiz_vpn_ctrl.vpn_mutex);
    memcpy(vpn_info, &thiz_vpn_ctrl.vpn_info, sizeof(AppMsgProperty_VpnInfo));
    GxCore_MutexUnlock(thiz_vpn_ctrl.vpn_mutex);
}
#ifdef ECOS_OS
static void _vpn_state_change_check(int new_status)
{
    int value = 0;
    if((pptp_state_old != PPTP_STATE_NONE) && (new_status == PPTP_STATE_NONE))
    {
        GxBus_ConfigGetInt(VPN_CONFIG_AUTO_KEY, &value, VPN_CONFIG_AUTO_DEFAULT);
        if (!value || (thiz_vpn_ctrl.vpn_try_cnt >= VPN_MAX_CONNECT_TIMES))
        {
            thiz_vpn_ctrl.vpn_enable = false;
            _vpn_stop();
        }
        else
        {
            if ((thiz_vpn_ctrl.vpn_try_cnt < VPN_MAX_CONNECT_TIMES)&&(thiz_vpn_ctrl.vpn_state == VPN_STATE_START))
            {
                thiz_vpn_ctrl.vpn_try_cnt++;
            }
        }
    }
    pptp_state_old = new_status;
}
#endif

static void _vpn_state_openvpn_update(int* data)
{
	char* vpn_name = "openvpn";
	static int state_openvpn = 0;
	int cur_state = 0;

	if(NULL == data)
	{
		cur_state = openvpn_state(vpn_name);
	}
	else
	{
		cur_state = *data;
	}

	if (state_openvpn != cur_state)
	{
		if(cur_state == OPENVPN_STATES_NONE)
		{
			thiz_vpn_ctrl.vpn_state = VPN_STATE_IDLE;
		}
		else if((cur_state >= OPENVPN_STATES_INIT && cur_state <= OPENVPN_STATES_ADD_ROUTES)
				|| cur_state == OPENVPN_STATES_RECONNECTING)
		{
			thiz_vpn_ctrl.vpn_state = VPN_STATE_START;
		}
		else if(cur_state == OPENVPN_STATES_CONNECTED)
		{
			thiz_vpn_ctrl.vpn_state = VPN_STATE_OK;
		}
		else if(cur_state == OPENVPN_STATES_CLOSE)
		{
			thiz_vpn_ctrl.vpn_state = VPN_STATE_CLOSE;
		}

		state_openvpn = cur_state;
	}
}

static status_t AppVpnServiceInit(handle_t self,int priority_offset)
{
    handle_t sch;

    _vpn_init();

    GxBus_MessageRegister(APPMSG_VPN_STATE_REPORT, sizeof(AppMsgProperty_VpnState));
    GxBus_MessageRegister(APPMSG_VPN_GET_STATE, sizeof(AppMsgProperty_VpnState));
    GxBus_MessageRegister(APPMSG_VPN_START, 0);
    GxBus_MessageRegister(APPMSG_VPN_STOP, 0);
    GxBus_MessageRegister(APPMSG_VPN_SET_AUTO, sizeof(AppMsgProperty_VpnAuto));
    GxBus_MessageRegister(APPMSG_VPN_GET_AUTO, sizeof(AppMsgProperty_VpnAuto));
    GxBus_MessageRegister(APPMSG_VPN_SET_PARAM, sizeof(AppMsgProperty_VpnInfo));
    GxBus_MessageRegister(APPMSG_VPN_GET_PARAM, sizeof(AppMsgProperty_VpnInfo));

    GxBus_MessageListen(self, GXMSG_NETWORK_PLUG_OUT);
    GxBus_MessageListen(self, GXMSG_NETWORK_STATE_REPORT);
    GxBus_MessageListen(self, APPMSG_VPN_GET_STATE);
    GxBus_MessageListen(self, APPMSG_VPN_START);
    GxBus_MessageListen(self, APPMSG_VPN_STOP);
    GxBus_MessageListen(self, APPMSG_VPN_SET_AUTO);
    GxBus_MessageListen(self, APPMSG_VPN_GET_AUTO);
    GxBus_MessageListen(self, APPMSG_VPN_SET_PARAM);
    GxBus_MessageListen(self, APPMSG_VPN_GET_PARAM);
    GxBus_MessageListen(self, GXMSG_VPN_STATUS);

    sch = GxBus_SchedulerCreate("VpnConsoleScheduler", GXBUS_SCHED_CONSOLE,
                                    1024 * 8, GXOS_DEFAULT_PRIORITY+priority_offset);
    GxBus_ServiceLink(self, sch);

    sch = GxBus_SchedulerCreate("VpnMsgScheduler", GXBUS_SCHED_MSG,
                                    1024 * 8, GXOS_DEFAULT_PRIORITY+priority_offset);
    GxBus_ServiceLink(self, sch);

    return GXCORE_SUCCESS;
}

static void AppVpnServiceDestroy(handle_t self)
{
    GxBus_MessageUnListen(self, GXMSG_NETWORK_PLUG_OUT);
    GxBus_MessageUnListen(self, GXMSG_NETWORK_STATE_REPORT);
    GxBus_MessageUnListen(self, APPMSG_VPN_GET_STATE);
    GxBus_MessageUnListen(self, APPMSG_VPN_START);
    GxBus_MessageUnListen(self, APPMSG_VPN_STOP);
    GxBus_MessageUnListen(self, APPMSG_VPN_SET_AUTO);
    GxBus_MessageUnListen(self, APPMSG_VPN_GET_AUTO);
    GxBus_MessageUnListen(self, APPMSG_VPN_SET_PARAM);
    GxBus_MessageUnListen(self, APPMSG_VPN_GET_PARAM);
    GxBus_MessageUnListen(self, GXMSG_VPN_STATUS);

    GxBus_MessageUnregister(APPMSG_VPN_STATE_REPORT);
    GxBus_MessageUnregister(APPMSG_VPN_GET_STATE);
    GxBus_MessageUnregister(APPMSG_VPN_START);
    GxBus_MessageUnregister(APPMSG_VPN_STOP);
    GxBus_MessageUnregister(APPMSG_VPN_SET_AUTO);
    GxBus_MessageUnregister(APPMSG_VPN_GET_AUTO);
    GxBus_MessageUnregister(APPMSG_VPN_SET_PARAM);
    GxBus_MessageUnregister(APPMSG_VPN_GET_PARAM);

    GxBus_ServiceUnlink(self);

    return;
}

static GxMsgStatus AppVpnServiceRecvMsg(handle_t self, GxMessage * msg)
{
    GxMsgStatus status = GXMSG_OK;

    switch (msg->msg_id)
    {
        case APPMSG_VPN_GET_STATE:
            {
                AppMsgProperty_VpnState *vpn_state = GxBus_GetMsgPropertyPtr(msg, AppMsgProperty_VpnState);
                _vpn_msg_get_state(vpn_state);
            }
            break;

        case APPMSG_VPN_START:
            _vpn_msg_start();
            break;

        case APPMSG_VPN_STOP:
            _vpn_msg_stop();
            break;

        case APPMSG_VPN_SET_AUTO:
            {
                AppMsgProperty_VpnAuto *vpn_auto = GxBus_GetMsgPropertyPtr(msg, AppMsgProperty_VpnAuto);
                _vpn_msg_set_auto(vpn_auto);
            }
            break;

        case APPMSG_VPN_GET_AUTO:
            {
                AppMsgProperty_VpnAuto *vpn_auto = GxBus_GetMsgPropertyPtr(msg, AppMsgProperty_VpnAuto);
                _vpn_msg_get_auto(vpn_auto);
            }
            break;

        case APPMSG_VPN_SET_PARAM:
            {
                AppMsgProperty_VpnInfo *vpn_info = GxBus_GetMsgPropertyPtr(msg, AppMsgProperty_VpnInfo);
                _vpn_msg_set_param(vpn_info);
            }
            break;

        case APPMSG_VPN_GET_PARAM:
            {
                AppMsgProperty_VpnInfo *vpn_info = GxBus_GetMsgPropertyPtr(msg, AppMsgProperty_VpnInfo);
                _vpn_msg_get_param(vpn_info);
            }
            break;

        case GXMSG_NETWORK_STATE_REPORT:
            {
                GxMsgProperty_NetDevState *dev_state = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevState);
                _vpn_network_state_msg_proc(dev_state);
                break;
            }

        case GXMSG_NETWORK_PLUG_OUT:
        {
            ubyte64 *net_dev = GxBus_GetMsgPropertyPtr(msg, ubyte64);
            _vpn_network_plugout_proc(*net_dev);
            break;
        }
        case GXMSG_VPN_STATUS:
        {
#ifdef ECOS_OS
            GxMsgProperty_VPNStatus *status_msg = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_VPNStatus);
            if(status_msg->type == VPN_TYPE_L2TP)
            {
                if(status_msg->status == L2TP_STATE_CONNECT)
                {
                    thiz_vpn_ctrl.vpn_state =VPN_STATE_OK;
                }
                else if(status_msg->status == L2TP_STATE_WAIT)
                {
                    thiz_vpn_ctrl.vpn_state = VPN_STATE_START;
                }
                else if(status_msg->status == L2TP_STATE_NONE)
                {
                    thiz_vpn_ctrl.vpn_state = VPN_STATE_IDLE;
                }
            }
            else if(status_msg->type == VPN_TYPE_PPTP)
            {
                _vpn_state_change_check(status_msg->status);

                if(status_msg->status == PPTP_STATE_NONE)
                {
                    thiz_vpn_ctrl.vpn_state = VPN_STATE_IDLE;
                }
                else if((status_msg->status >= PPTP_STATE_WAIT) && (status_msg->status <= PPTP_STATE_GRE))
                {
                    thiz_vpn_ctrl.vpn_state = VPN_STATE_START;
                }
                else if(status_msg->status == PPTP_STATE_PPP)
                {
                    thiz_vpn_ctrl.vpn_state = VPN_STATE_OK;
                }
                else if(status_msg->status == PPTP_STATE_CLOSE)
                {
                    thiz_vpn_ctrl.vpn_state = VPN_STATE_CLOSE;
                }
            }
            else if(status_msg->type == VPN_TYPE_OPENVPN)
            {
            #if 1
                _vpn_state_openvpn_update(&status_msg->status);
            #else
                if(status_msg->status == OPENVPN_STATES_NONE)
                {
                    thiz_vpn_ctrl.vpn_state = VPN_STATE_IDLE;
                }
                else if((status_msg->status >= OPENVPN_STATES_INIT && status_msg->status <= OPENVPN_STATES_ADD_ROUTES)
                		|| status_msg->status == OPENVPN_STATES_RECONNECTING)
                {
                    thiz_vpn_ctrl.vpn_state = VPN_STATE_START;
                }
                else if(status_msg->status == OPENVPN_STATES_CONNECTED)
                {
                    thiz_vpn_ctrl.vpn_state = VPN_STATE_OK;
                }
                else if(status_msg->status == OPENVPN_STATES_CLOSE)
                {
                    thiz_vpn_ctrl.vpn_state = VPN_STATE_CLOSE;
                }
             #endif
            }
#endif
        }
        break;

        default:
            break;
    }

    return status;
}

static void AppVpnServiceConsole(handle_t self)
{
    static int pre_vpn_state = 0;
    GxCore_MutexLock(thiz_vpn_ctrl.vpn_mutex);
    #ifdef LINUX_OS
    if(thiz_vpn_ctrl.vpn_info.vpn_type == VPN_TYPE_OPENVPN){
        _vpn_state_openvpn_update(NULL);
	}
    #endif
    switch(thiz_vpn_ctrl.vpn_state)
    {
        case VPN_STATE_IDLE:
            _vpn_state_report(thiz_vpn_ctrl.vpn_state);
            if(thiz_vpn_ctrl.vpn_enable == true)
            {
                pre_vpn_state = thiz_vpn_ctrl.vpn_state;
                if(GXCORE_ERROR == _vpn_start())
                    _vpn_stop();
            }
            break;
        case VPN_STATE_START:
        case VPN_STATE_OK:
            if(thiz_vpn_ctrl.vpn_enable == true)
            {
                pre_vpn_state = thiz_vpn_ctrl.vpn_state;
                _vpn_monitor();
            }
            else //disable
            {
                _vpn_stop();
            }
            break;
        case VPN_STATE_CLOSE:
            {
				_vpn_state_report(thiz_vpn_ctrl.vpn_state);
				if(thiz_vpn_ctrl.vpn_info.vpn_type == VPN_TYPE_OPENVPN)
				{
					if(pre_vpn_state == VPN_STATE_START || pre_vpn_state == VPN_STATE_OK) /*openvpn from start --to--> close*/
					{
						thiz_vpn_ctrl.vpn_enable = false; /*openvpn closed and then not start auto reconnect*/
						pre_vpn_state = thiz_vpn_ctrl.vpn_state;
					}

					if(thiz_vpn_ctrl.vpn_enable == true)
					{
						if(GXCORE_ERROR == _vpn_start())
							_vpn_stop();
					}
				}
            }
            break;

        default:
            break;
    }

    GxCore_MutexUnlock(thiz_vpn_ctrl.vpn_mutex);
    GxCore_ThreadDelay(VPN_CONSOLE_DELAY_MS);
}

status_t app_vpn_service_init(void)
{
    extern handle_t g_app_msg_self;
    status_t ret = GXCORE_ERROR;
    if(GxBus_ServiceFindByName(VPN_SERVICE_NAME) == GXCORE_INVALID_POINTER)
    {
        GxBus_ServiceCreate(&app_vpn_service);
        GxBus_MessageListen(g_app_msg_self, APPMSG_VPN_STATE_REPORT);
        ret = GXCORE_SUCCESS;
    }
    return ret;
}

void vpn_factory_reset(void)
{
	if(GXCORE_FILE_EXIST == GxCore_FileExists(VPN_DB_PATH))
	{
		GxCore_FileDelete(VPN_DB_PATH);
	}
	if(GXCORE_FILE_EXIST == GxCore_FileExists(PPPD_PPTP_CONFIG_PATH))
	{
		GxCore_FileDelete(PPPD_PPTP_CONFIG_PATH);
	}
}

#endif

