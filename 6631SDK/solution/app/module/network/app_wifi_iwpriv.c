#include "app_config.h"

#ifdef ECOS_OS
#if NETWORK_SUPPORT && WIFI_SUPPORT

//#define __ECOS
#include <sys/types.h>
#include <cyg/kernel/kapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <pkgconf/system.h>

#include <cyg/io/flash.h>
#include <cyg/infra/diag.h>

#include <cyg/io/eth/netdev.h>
#include <string.h>

#include <sys/socket.h>
#include <net/if.h>
#include <net/if_var.h>
#include <net/if_arp.h>
#include <cyg/io/eth/eth_drv.h>

#include "gxcore.h"
#include "app.h"
#include "app_module.h"
#include <string.h>
#include <dhcp.h>

#define SIOCIWFIRSTPRIV 0x8BE0
#define RT_PRIV_IOCTL (SIOCIWFIRSTPRIV + 0x01)
#define RTPRIV_IOCTL_SET (SIOCIWFIRSTPRIV + 0x02)
#define RTPRIV_IOCTL_BBP (SIOCIWFIRSTPRIV + 0x03)
#define RTPRIV_IOCTL_MAC (SIOCIWFIRSTPRIV + 0x05)
#define RTPRIV_IOCTL_E2P (SIOCIWFIRSTPRIV + 0x07)
#define RTPRIV_IOCTL_STATISTICS (SIOCIWFIRSTPRIV + 0x09)
#define RTPRIV_IOCTL_GSITESURVEY (SIOCIWFIRSTPRIV + 0x0D)
#define RTPRIV_IOCTL_GET_MAC_TABLE (SIOCIWFIRSTPRIV + 0x0F)
#define RTPRIV_IOCTL_SHOW (SIOCIWFIRSTPRIV + 0x11)
#define RTPRIV_IOCTL_RF (SIOCIWFIRSTPRIV + 0x13)
#define RTPRIV_IOCTL_GET_P2P	 (SIOCIWFIRSTPRIV + 0x06)

#define RTPRIV_IOCTL_FLAG_UI 0x0001	/* Notidy this private cmd send by UI. */
#define RTPRIV_IOCTL_FLAG_NODUMPMSG 0x0002	/* Notify driver cannot dump msg to stdio/stdout when run this private ioctl cmd */
#define RTPRIV_IOCTL_FLAG_NOSPACE 0x0004	/* Notify driver didn't need copy msg to caller due to the caller didn't reserve space for this cmd */

#define WIFI_SCAN_TIMEOUT 20000
#define MAX_P2P_DEV_NUM 5
#define MAX_DEV_NAME_LEN 5

extern char *strsep(char **s, const char *ct);

typedef struct
{
	const char*	cmd;					// Input command string
	int	(*func)(int argc, char* argv[]);
	const char*	msg;					// Help message
} COMMAND_TABLE;

// Define Linux ioctl relative structure, keep only necessary things
struct iw_point
{
	void *		pointer;
	unsigned short		length;
	unsigned short		flags;
};

union iwreq_data
{
	struct iw_point data;
};

struct iwreq {
	union
	{
		char    ifrn_name[IFNAMSIZ];    /* if name, e.g. "eth0" */
	} ifr_ifrn;

	union   iwreq_data      u;
};

typedef struct eth_drv_sc	*PNET_DEV;

#ifndef NULL
#define NULL	0
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif
#define CMD_DATA_LEN 10240

extern int rt28xx_sta_ioctl(char *dev_name, void* rq, signed int cmd);

static char TempString[CMD_DATA_LEN];
static handle_t s_aplist_mutex = -1;
static bool s_scanover_flag = true;

void app_wifi_set_scanover_flag(bool flag)
{
    s_scanover_flag = flag;
}

static bool app_wifi_get_scanover_flag(void)
{
    return s_scanover_flag;
}

int iwpriv(char *dev, char *pcmd, char *paras)
{
    int cmd = -1;
    struct iwreq param;

    if (!pcmd || !dev)
        goto error_exit;

    /*show command*/
    if(paras)
        app_log_debug("\033[31m iwpriv %s %s %s\033[0m\n", dev, pcmd, paras);
    else
        app_log_debug("\033[31m iwpriv %s %s\033[0m\n", dev, pcmd);

    if ((strcmp(dev, "ra0") != 0 )
            && (strcmp(dev, "ra1") != 0 )
            && (strcmp(dev, "ra2") != 0 )
            && (strcmp(dev, "ra3") != 0 )
            && (strcmp(dev, "ra4") != 0 )
            && (strcmp(dev, "ra5") != 0 )
            && (strcmp(dev, "ra6") != 0 )
            && (strcmp(dev, "ra7") != 0 )
            && (strcmp(dev, "wds0") != 0 )
            && (strcmp(dev, "apcli0") != 0 )
            && (strcmp(dev, "mesh0") != 0 )
       )
        goto error_exit;

    memset(&param, 0, sizeof(struct iwreq));

    if ( strcmp(pcmd, "set") == 0 )
    {
        cmd = RTPRIV_IOCTL_SET;
    }
    else if ( strcmp(pcmd, "show") == 0 )
    {
        cmd = RTPRIV_IOCTL_SHOW;
    }
    else if ( strcmp(pcmd, "e2p") == 0 )
    {
        cmd = RTPRIV_IOCTL_E2P;
    }
    else if ( strcmp(pcmd, "bbp") == 0 )
    {
        cmd = RTPRIV_IOCTL_BBP;
        param.u.data.flags |= (RTPRIV_IOCTL_FLAG_NOSPACE | RTPRIV_IOCTL_FLAG_NODUMPMSG);
    }
    else if ( strcmp(pcmd, "macaddr") == 0 )
    {
        cmd = SIOCGIFHWADDR;
        param.u.data.flags |= (RTPRIV_IOCTL_FLAG_NOSPACE | RTPRIV_IOCTL_FLAG_NODUMPMSG);
    }
    else if ( strcmp(pcmd, "mac") == 0 )
    {
        cmd = RTPRIV_IOCTL_MAC;
        param.u.data.flags |= (RTPRIV_IOCTL_FLAG_NOSPACE | RTPRIV_IOCTL_FLAG_NODUMPMSG);
    }
    else if ( strcmp(pcmd, "bssid") == 0)
    {
        cmd = SIOCGIFBSSID;
        param.u.data.flags |= (RTPRIV_IOCTL_FLAG_NOSPACE | RTPRIV_IOCTL_FLAG_NODUMPMSG);
    }
    else if ( strcmp(pcmd, "rf") == 0 )
    {
        cmd = RTPRIV_IOCTL_RF;
        param.u.data.flags |= (RTPRIV_IOCTL_FLAG_NOSPACE | RTPRIV_IOCTL_FLAG_NODUMPMSG);
    }
    else if (( strcmp(pcmd, "GetSiteSurvey") == 0 ) ||( strcmp(pcmd, "get_site_survey") == 0 ))
    {
        cmd = RTPRIV_IOCTL_GSITESURVEY;
        param.u.data.flags |= (RTPRIV_IOCTL_FLAG_NOSPACE | RTPRIV_IOCTL_FLAG_NODUMPMSG);
    }
    else if ( strcmp(pcmd, "stat") == 0 )
    {
        cmd = RTPRIV_IOCTL_STATISTICS;
        param.u.data.flags |= (RTPRIV_IOCTL_FLAG_NOSPACE | RTPRIV_IOCTL_FLAG_NODUMPMSG);
    }
    else
    {
        goto error_exit;
    }

    if(-1 == s_aplist_mutex)
        GxCore_MutexCreate(&s_aplist_mutex);

    GxCore_MutexLock(s_aplist_mutex);
    int original_length = 0;
    //int i =0;

    //char *TempString=NULL;
    //TempString =malloc(7168);//7*1024
    //if(TempString==NULL){app_log_error("Not enough memory for CmdIwpriv!\n");return FALSE;}
    memset(TempString, 0, sizeof(TempString));
    TempString[0] = '\0';

    if (paras)
    {
        sprintf(&TempString[0], "%s", paras);
        original_length = strlen(paras);
    }

    param.u.data.pointer = &TempString[0];
    param.u.data.length = strlen(TempString);
    //app_log_debug("cmd:%s [%d][%d]",TempString,param.u.data.length,original_length);
    if(rt28xx_sta_ioctl(dev, &param, cmd) < 0)
    {
        GxCore_MutexUnlock(s_aplist_mutex);
        goto error_exit;
    }
    if (param.u.data.length != original_length)
    {
        TempString[param.u.data.length] = '\0';
        //for (i =0; i < param.u.data.length;i ++)
        //    diag_printf("%c",TempString[i]);
        //diag_printf("%s", TempString);
    }
    GxCore_MutexUnlock(s_aplist_mutex);

    //free(TempString);
    return TRUE;

error_exit:
    if(paras)
        app_log_error("\033[31m iwpriv %s %s %s failed\033[0m\n", dev, pcmd, paras);
    else
        app_log_error("\033[31m iwpriv %s %s failed\033[0m\n", dev, pcmd);
    app_log_error("usage: iwpriv <interface> set|show|e2p|bbp|macaddr|mac|rf <command>\n");
    app_log_error("usage: iwpriv <interface> stat|GetSiteSurvey\n");
    return FALSE;
}



/*
 * sprintf(msg+strlen(msg),"%-4s%-33s%-20s%-23s%-9s%-7s%-7s%-3s\n",
 *        "Ch", "SSID", "BSSID", "Security", "Siganl(%)", "W-Mode", " ExtCH"," NT");
 */

#define SSID_LEN   (33)
#define MAC_LEN    (20)
#define SECU_LEN   (23)
#define SIG_LEN    (9)
#define RESULT_LEN (4+SSID_LEN+MAC_LEN+SECU_LEN+SIG_LEN+7+7+3+10)  //10:tmp

unsigned short app_check_security(char * pSecu)
{
    unsigned char chAuth = 0;
    unsigned char chEncr = 0;
    int i, j;

    char data[SECU_LEN + 1] = {0};
    char tmp[SECU_LEN + 1] = {0};
    char * p = NULL;

    strcpy(data, pSecu);
    for(i = 0, j = 0; data[i] != '\0'; i++){
        if(data[i] != '/'){        //删除字符'/'
            data[j++] = data[i];
        }
    }
    data[j] = '\0';
    //app_log_debug("%s,len : %d\n", data, strlen(data));

    if(strncmp(data, "WEP", strlen("WEP")) == 0 && strlen(data) == strlen("WEP"))
    {
        chAuth = WIFI_AUTH_SHARED;
        chEncr = WIFI_ENCRYP_WEP;
    }
    else if(strncmp(pSecu, "NONE", strlen("NONE")) == 0 && strlen(data) == strlen("NONE"))
    {
        chAuth = WIFI_AUTH_OPEN;
        chEncr = WIFI_ENCRYP_NONE;
    }
    else
    {
        //AUTH TYPE
        if((p = strstr(data, "WPA2PSK")) != NULL)
        {
            chAuth |= WIFI_AUTH_WPA2PSK;
            strncat(tmp, data, p-data);
            strcat(tmp, p+strlen("WPA2PSK"));
            memset(data, 0, sizeof(data));
            strcpy(data, tmp);
            memset(tmp, 0, sizeof(tmp));
            //app_log_debug("%s,len : %d\n", data, strlen(data));
        }
        if((p = strstr(data, "WPA1PSK")) != NULL)
        {
            chAuth |= WIFI_AUTH_WPA1PSK;
            strncat(tmp, data, p-data);
            strcat(tmp, p+strlen("WPA1PSK"));
            memset(data, 0, sizeof(data));
            strcpy(data, tmp);
            memset(tmp, 0, sizeof(tmp));
            //app_log_debug("%s,len : %d\n", data, strlen(data));
        }
        if((p = strstr(data, "WPAPSK")) != NULL)
        {
            chAuth |= WIFI_AUTH_WPA1PSK;
            strncat(tmp, data, p-data);
            strcat(tmp, p+strlen("WPAPSK"));
            memset(data, 0, sizeof(data));
            strcpy(data, tmp);
            memset(tmp, 0, sizeof(tmp));
            //app_log_debug("%s,len : %d\n", data, strlen(data));
        }
        if((p = strstr(data, "WPA2")) != NULL)
        {
            chAuth |= WIFI_AUTH_WPA2;
            strncat(tmp, data, p-data);
            strcat(tmp, p+strlen("WPA2"));
            memset(data, 0, sizeof(data));
            strcpy(data, tmp);
            memset(tmp, 0, sizeof(tmp));
            //app_log_debug("%s,len : %d\n", data, strlen(data));
        }
        if((p = strstr(data, "WPA1")) != NULL)
        {
            chAuth |= WIFI_AUTH_WPA1;
            strncat(tmp, data, p-data);
            strcat(tmp, p+strlen("WPA1"));
            memset(data, 0, sizeof(data));
            strcpy(data, tmp);
            memset(tmp, 0, sizeof(tmp));
            //app_log_debug("%s,len : %d\n", data, strlen(data));
        }
        if((p = strstr(data, "WPA")) != NULL)
        {
            chAuth |= WIFI_AUTH_WPA1;
            strncat(tmp, data, p-data);
            strcat(tmp, p+strlen("WPA"));
            memset(data, 0, sizeof(data));
            strcpy(data, tmp);
            memset(tmp, 0, sizeof(tmp));
            //app_log_debug("%s,len : %d\n", data, strlen(data));
        }

        //ENCRYP TYPE
        if((p = strstr(data, "AES")) != NULL)
        {
            chEncr |= WIFI_ENCRYP_AES;
            strncat(tmp, data, p-data);
            strcat(tmp, p+strlen("AES"));
            memset(data, 0, sizeof(data));
            strcpy(data, tmp);
            memset(tmp, 0, sizeof(tmp));
            //app_log_debug("%s,len : %d\n", data, strlen(data));
        }
        if((p = strstr(data, "TKIP")) != NULL)
        {
            chEncr |= WIFI_ENCRYP_TKIP;
            strncat(tmp, data, p-data);
            strcat(tmp, p+strlen("TKIP"));
            memset(data, 0, sizeof(data));
            strcpy(data, tmp);
            memset(tmp, 0, sizeof(tmp));
            //app_log_debug("%s,len : %d\n", data, strlen(data));
        }
    }

    //app_log_debug("Encrpy Mode : 0x%x 0x%x\n", chAuth, chEncr);
    return (chAuth << 8) | chEncr;
}

static unsigned int app_get_nowlist(char *dev_name,ApInfo *ap_data, unsigned int max_ap_num)
{
    char *tok = NULL;
    char *s = NULL;
    char *s_bak = NULL;

    char name[SSID_LEN + 1] = {0};
    char mac[MAC_LEN + 1] = {0};
    char signal[SIG_LEN + 1] = {0};
    char secu[SECU_LEN + 1] = {0};
    unsigned int nApNum = 0;
    unsigned int nLine = 0;
    int i = 0;

    if ( !app_network_wifi_check_dev_plug_status(dev_name) )
        return 0 ;

    iwpriv(dev_name, "GetSiteSurvey", NULL);

    GxCore_MutexLock(s_aplist_mutex);
    s = GxCore_Strdup(TempString);
    s_bak = s;//strsep会改变s的值，所以保留备份

    for(tok = strsep(&s, "\n"); tok != NULL; tok = strsep(&s, "\n"))
    {
        if(strlen(tok) == 0)
            continue;

        app_log_debug("\033[33m%s\n\033[0m", tok);
        if(nLine == 0)  //title
        {
            nLine++;
            continue;
        }

        sscanf(tok, "%*4c%33c%20s%23s%9s%*7c%*7c%*3c", name, mac, secu, signal);
        strcpy(ap_data[nApNum].ap_mac, mac);
        ap_data[nApNum].ap_quality = atoi(signal);
        ap_data[nApNum].encryption = app_check_security(secu);

        i = strlen(name) - 1;
        while(name[i] == ' ')
        {
            i--;
        }
        name[i + 1] = '\0';
        strcpy(ap_data[nApNum].ap_essid, name);

        memset(name, 0, sizeof(name));
        memset(mac, 0, sizeof(mac));
        memset(secu, 0, sizeof(secu));
        memset(signal, 0, sizeof(signal));

        nApNum++;
        if(nApNum == max_ap_num)
            break;
        nLine++;
    }
    APP_FREE(s_bak);
    GxCore_MutexUnlock(s_aplist_mutex);

    return nApNum;
}

unsigned int app_get_aplist(char *dev_name,ApInfo *ap_data, unsigned int max_ap_num, char *essid)
{
    unsigned int nApNum = 0;
    char cmd[50] = {0};
    unsigned long tick = 0;

    if (dev_name == NULL )
        return 0;

    if ( !app_network_wifi_check_dev_plug_status(dev_name) )
        return 0 ;

    if(essid)
        snprintf(cmd, sizeof(cmd), "SiteSurvey=%s", essid);
    else
        strcpy(cmd, "SiteSurvey");

    app_wifi_set_scanover_flag(false);
    iwpriv(dev_name, "set", cmd);
    tick = GxCore_TickStart(WIFI_SCAN_TIMEOUT);
    do
    {
        if(app_wifi_get_scanover_flag() == true)
            break;
        if ( !app_network_wifi_check_dev_plug_status(dev_name) )
            break;
        GxCore_ThreadDelay(100);
    }while(!GxCore_TickEnd(tick));
    app_wifi_set_scanover_flag(true);
    nApNum = app_get_nowlist(dev_name,ap_data,max_ap_num);
    return nApNum;
}

static status_t app_get_security(char *dev_name,char *mac, unsigned short *encryption, char* essid, int32_t hidden)
{
    status_t ret = GXCORE_ERROR;
    unsigned int ap_num = 0;
    unsigned int i = 0;
    ApInfo *ap_info = NULL;
    int nTryCount;

    if(hidden == 1 && essid == NULL)
        return GXCORE_ERROR;

    ap_info = GxCore_Malloc(sizeof(ApInfo)*MAX_AP_NUM);
    if(NULL == ap_info)
        return GXCORE_ERROR;
    memset(ap_info, 0, sizeof(ApInfo)*MAX_AP_NUM);

    nTryCount = 5;
    while(nTryCount > 0 && ret == GXCORE_ERROR)
    {
        ap_num = app_get_nowlist(dev_name,ap_info, MAX_AP_NUM);
        //没有扫描过
        if(0 == ap_num || nTryCount !=5)
        {
            app_log_info("\033[32mTry to get ap security again!!\n\033[0m");
            ap_num = app_get_aplist(dev_name,ap_info, MAX_AP_NUM, essid);
        }

        for(i = 0; i < ap_num; i++)
        {
            if(strcmp(mac, ap_info[i].ap_mac) == 0)
            {
                if((hidden == 0) || ((hidden == 1) && (strcmp(essid, ap_info[i].ap_essid) == 0)))
                {
                    *encryption = ap_info[i].encryption;
                    ret = GXCORE_SUCCESS;
                    GxCore_Free(ap_info);
                    return ret;
                }
            }
        }
        nTryCount--;
    }

    GxCore_Free(ap_info);
    ap_info = NULL;

    return ret;
}

extern void app_network_status_set(IfFileRet status);

status_t _wifi_start(char *wifi_dev, char *essid, char *mac, char *psk, int32_t hidden)
{
    char cmd[50] = {0};
    unsigned short nEncr = 0;
    unsigned char chAuth = 0, chEncr = 0;
    if(wifi_dev == NULL || essid == NULL || mac == NULL)
        return GXCORE_ERROR;

    app_log_debug("\033[35m[%s]Name : %s MAC : %s PSK : %s hidden: %d\n\033[0m", wifi_dev, essid, mac, psk, hidden);

    if(GXCORE_SUCCESS == app_get_security(wifi_dev,mac, &nEncr, essid, hidden))
    {
        iwpriv(wifi_dev, "set", "NetworkType=Infra");
        app_log_debug("encryption : 0x%x\n", nEncr);
        chAuth = (unsigned char)((nEncr & 0xFF00) >> 8);
        chEncr = (unsigned char)(nEncr & 0x00FF);

        if((chAuth & WIFI_AUTH_WPA2PSK) == WIFI_AUTH_WPA2PSK) {
            iwpriv(wifi_dev, "set", "AuthMode=WPA2PSK");
        } else {
            if((chAuth & WIFI_AUTH_WPA1PSK) == WIFI_AUTH_WPA1PSK) {
                iwpriv(wifi_dev, "set", "AuthMode=WPAPSK");
            } else {
                if((chAuth & WIFI_AUTH_SHARED) == WIFI_AUTH_SHARED) {
                    iwpriv(wifi_dev, "set", "AuthMode=SHARED");
                } else {
                    if((chAuth & WIFI_AUTH_OPEN) == WIFI_AUTH_OPEN) {
                        iwpriv(wifi_dev, "set", "AuthMode=OPEN");
                    }
                }
            }
        }

        if((chEncr & WIFI_ENCRYP_AES) == WIFI_ENCRYP_AES) {
            iwpriv(wifi_dev, "set", "EncrypType=AES");
            memset(cmd, 0, sizeof(cmd));
            snprintf(cmd, sizeof(cmd), "WPAPSK=%s", psk);
            iwpriv(wifi_dev, "set", cmd);
        } else {
            if((chEncr & WIFI_ENCRYP_TKIP) == WIFI_ENCRYP_TKIP) {
                iwpriv(wifi_dev, "set", "EncrypType=TKIP");
                memset(cmd, 0, sizeof(cmd));
                snprintf(cmd, sizeof(cmd), "WPAPSK=%s", psk);
                iwpriv(wifi_dev, "set", cmd);
            } else {
                if((chEncr & WIFI_ENCRYP_WEP) == WIFI_ENCRYP_WEP) {
                    iwpriv(wifi_dev, "set", "EncrypType=WEP");
                    iwpriv(wifi_dev, "set", "DefaultKeyID=1");
                    memset(cmd, 0, sizeof(cmd));
                    snprintf(cmd, sizeof(cmd), "Key1=%s", psk);
                    iwpriv(wifi_dev, "set", cmd);
                } else {
                    if((chEncr & WIFI_ENCRYP_NONE) == WIFI_ENCRYP_NONE) {
                        iwpriv(wifi_dev, "set", "EncrypType=NONE");
                    }
                }
            }
        }

        memset(cmd, 0, sizeof(cmd));
        if(1 == hidden)
            snprintf(cmd, sizeof(cmd), "SSID=%s", essid);
        else
            snprintf(cmd, sizeof(cmd), "BSSID=%s", mac);
        iwpriv(wifi_dev, "set", cmd);

        app_log_info("\033[31mset wifi status = 0 !!\033[0m\n");
        app_network_status_set(IF_FILE_WAIT);
        return GXCORE_SUCCESS;
    }

    GxCore_ThreadDelay(100);
    app_log_info("\033[31mCann't find current wifi !!\033[0m\n");
    app_network_status_set(IF_FILE_DISCONNECTED);

    return GXCORE_ERROR;
}

int app_get_wifi_mac(char* dev_name,char * mac)
{
    if ( dev_name == NULL )
    {
        return -1;
    }
    iwpriv(dev_name, "macaddr", NULL);

    GxCore_MutexLock(s_aplist_mutex);

    memcpy(mac, TempString, sizeof("00:00:00:00:00:00")); //00:00:00:00:00:00

    GxCore_MutexUnlock(s_aplist_mutex);

    return 0;
}

int app_wifi_linkdown(char* dev_name)
{
    if ( dev_name == NULL )
    {
        return -1;
    }
    iwpriv(dev_name, "set", "Disconnect");
    return 0;
}

int app_get_wifi_bssid(char* dev_name,char* bssid)
{
    if ( dev_name == NULL )
    {
        return -1;
    }
    iwpriv(dev_name, "bssid", NULL);
    GxCore_MutexLock(s_aplist_mutex);
    sprintf(bssid,"%02x:%02x:%02x:%02x:%02x:%02x",TempString[0],TempString[1],TempString[2],TempString[3],TempString[4],TempString[5]);
    GxCore_MutexUnlock(s_aplist_mutex);
    app_log_debug("bssid :::::::%s\r\n",bssid);
    return 0;
}

#endif
#endif

