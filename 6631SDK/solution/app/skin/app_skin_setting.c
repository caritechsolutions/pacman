/*************************************************************************
	> File Name: app_applets.c
	> Author:
	> Mail:
	> Created Time: 2022/3/24
 ************************************************************************/

#include "app.h"

#if SKIN_SUPPORT
#include "app_module.h"
#include "netapps/app_netapps_service.h"
#include <sys/stat.h>
#include <gxos/gxcore_os_core.h>
#include <gx_apps.h>
#include <common/gxpm.h>

#include "app_skin_config.h"
#include "app_skin_setting.h"
#include "app_skin_flash.h"
#if DVB_CLOUD_SUPPORT
#include <gx_dvbonline.h>
#endif

static unsigned char skin_online_connected = 0 ;
static unsigned int  skin_value_key[6] ;

int   gxapp_skin_version_is_matched(char *version)
{
	int len1 = 0;
	int len2 = 0 ;

	if ( NULL == version )
		return 0 ;

    len1 = strlen(version);
  	len2 = strlen(VER_DRVIER_APPLICATION);
	if (len1 != len2)
	{
		SKIN_ERR("[skin seta] sys version = %s \n",VER_DRVIER_APPLICATION);
		return 0 ;
	}

	if ( 0 != strcmp(version,VER_DRVIER_APPLICATION) )
	{
		SKIN_ERR("[skin seta] sys version = %s \n",VER_DRVIER_APPLICATION);
		return 0 ;
	}

	return 1 ;
}

int gxapp_skin_online_is_disconnect(void)
{
#if NETWORK_SUPPORT
	if(IF_STATE_CONNECTED == app_network_get_cur_dev_and_state(NULL))
	{
		skin_online_connected = 1 ;
		return 0 ;
	}
#endif
	skin_online_connected = 0 ;
	return 1 ;
}

int gxapp_skin_online_get_connect_state(void)
{
	return skin_online_connected ;
}

int gxapp_skin_get_usb_entry_path(void)
{
	char pvr_path[30];
	char full_path[FULL_XML_PATH_LENGTH];

	char *p_usb_setting = NULL ;

	HotplugPartitionList *list = NULL;
    int i = 0;
	int len = 0 ;
	int ret ;

    list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
    if (list && list->partition_num)
	{
		p_usb_setting = gxapp_skin_usb_get_url() ;

		for (i = 0; i < list->partition_num; i++)
		{
			len = strlen(list->partition[i].partition_entry);
			if ( (len > 0) && (p_usb_setting != NULL ) )
			{
				//SKIN_DBG("partition_entry = %d ,%d, %s \n",i,len,list->partition[i].partition_entry);
				if ( 0 == memcmp(p_usb_setting,list->partition[i].partition_entry,len) )
				{
					memset(full_path,0,FULL_XML_PATH_LENGTH);
					snprintf(full_path, FULL_XML_PATH_LENGTH, "%s/%s",list->partition[i].partition_entry,PATH_VER_MAIN);
					ret = access(full_path, F_OK) ;
					SKIN_DBG("[skin seta] usb setting path=%s, ret=%d \n",full_path,ret);
					if (0 == ret)
					{
						SKIN_DBG("[skin seta] usb setting find id=%d \n",i);
						return 0 ;
					}
				}
			}
        }

		memset(pvr_path,0,30);
		GxBus_ConfigGet(PVR_PARTITION_KEY, pvr_path, PVR_DEV_STR_LEN, PVR_PARTITION);

		for (i = 0; i < list->partition_num; i++)
		{
			if(strcmp((char*)pvr_path,list->partition[i].dev_name)==0)
			{
				//SKIN_DBG("partition_entry = %d ,%d, %s \n",i,len,list->partition[i].partition_entry);
				memset(full_path,0,FULL_XML_PATH_LENGTH);
				snprintf(full_path, FULL_XML_PATH_LENGTH, "%s/%s",list->partition[i].partition_entry,PATH_VER_MAIN);
				ret = access(full_path, F_OK) ;
				SKIN_DBG("[skin seta] usb default path=%s, ret=%d \n",full_path,ret);
				if (0 == ret)
				{
					SKIN_DBG("[skin seta] usb default find id=%d \n",i);
					gxapp_skin_usb_set_url(list->partition[i].partition_entry);
					return 0;
				}
			}
        }

		for (i = 0; i < list->partition_num; i++)
		{
			//SKIN_DBG("partition_entry = %d ,%d, %s \n",i,len,list->partition[i].partition_entry);
			memset(full_path,0,FULL_XML_PATH_LENGTH);
			snprintf(full_path, FULL_XML_PATH_LENGTH, "%s/%s",list->partition[i].partition_entry,PATH_VER_MAIN);
			ret = access(full_path, F_OK) ;
			SKIN_DBG("[skin seta] usb each path=%s, ret=%d \n",full_path,ret);
			if (0 == ret)
			{
				SKIN_DBG("[skin seta] usb each find id=%d \n",i);
				gxapp_skin_usb_set_url(list->partition[i].partition_entry);
				return 0;
			}
        }

		for (i = 0; i < list->partition_num; i++)
		{
			//SKIN_DBG("partition_entry = %d ,%d, %s \n",i,len,list->partition[i].partition_entry);

			memset(full_path,0,FULL_XML_PATH_LENGTH);
			snprintf(full_path, FULL_XML_PATH_LENGTH, "%s/%s/%s",list->partition[i].partition_entry,
									PATH_VER_MAIN,SKIN_UPDATE_BIN_NAME);
			ret = access(full_path, F_OK) ;
			SKIN_DBG("[skin seta] usb bin skin path=%s, ret=%d \n",full_path,ret);
			if (0 == ret)
			{
				SKIN_DBG("[skin seta] usb bin skin find id=%d \n",i);
				gxapp_skin_usb_set_memory_url(list->partition[i].partition_entry);
				return -2;
			}

			memset(full_path,0,FULL_XML_PATH_LENGTH);
			snprintf(full_path, FULL_XML_PATH_LENGTH, "%s/%s",list->partition[i].partition_entry,SKIN_UPDATE_BIN_NAME);
			ret = access(full_path, F_OK) ;
			SKIN_DBG("[skin seta] usb bin base path=%s, ret=%d \n",full_path,ret);
			if (0 == ret)
			{
				SKIN_DBG("[skin seta] usb bin base find id=%d \n",i);
				gxapp_skin_usb_set_memory_url(list->partition[i].partition_entry);
				return -3;
			}
        }

		for (i = 0; i < list->partition_num; i++)
		{
			//SKIN_DBG("partition_entry = %d ,%d, %s \n",i,len,list->partition[i].partition_entry);
			SKIN_DBG("[skin seta] usb end path=%s \n",list->partition[i].partition_entry);
			gxapp_skin_usb_set_memory_url(list->partition[i].partition_entry);
			break ;
        }
        return -4;
    }
    return SKIN_NO_USB_RETURN;
}

int   gxapp_skin_check_usb_entry_path(char *path)
{
    int ret = 0 ;
    char full_path[FULL_XML_PATH_LENGTH];
    if ( path == NULL )
        return -1 ;
    memset(full_path,0,FULL_XML_PATH_LENGTH);
    snprintf(full_path, FULL_XML_PATH_LENGTH, "%s/%s",path,PATH_VER_MAIN);
    ret = access(full_path, F_OK) ;
    SKIN_DBG("[skin seta] check usb path=%s, ret=%d \n",full_path,ret);
    if (0 == ret)
    {
        return 0 ;
    }
    return -1 ;
}

int   gxapp_skin_check_usb_device(void)
{
    HotplugPartitionList *list = NULL;

    list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
    if (list && list->partition_num)
    {
        return 1 ;
    }
    return 0 ;
}

void  gxapp_skin_app_close_all_before_reboot(void)
{
	SKIN_ERR("[skin seta] stop module \n");

#if NETWORK_SUPPORT
    app_send_msg_exec(GXMSG_NETWORK_STOP, NULL);
#endif
#if DVB_CLOUD_SUPPORT
    gx_dvbonline_pause();
#endif
	app_send_msg_exec(GXMSG_PM_CLOSE, NULL);

}

void  gxapp_skin_app_api_reboot(void)
{
#if 1
	SKIN_ERR("[skin seta] ######## api reboot \n");

	GxCore_ThreadDelay(2000);
	GxCore_Reboot();
#endif
}

void gxapp_skin_app_reboot(void)
{
	SKIN_ERR("[skin seta] ready reboot \n");

#if 1

#if NETWORK_SUPPORT
    app_send_msg_exec(GXMSG_NETWORK_STOP, NULL);
#endif
#if DVB_CLOUD_SUPPORT
    gx_dvbonline_pause();
#endif
	app_send_msg_exec(GXMSG_PM_CLOSE, NULL);

	SKIN_ERR("[skin seta] ######## app reboot \n");

	GxCore_ThreadDelay(2000);
	GxCore_Reboot();

#endif
}

int   gxapp_skin_check_key_to_debug(unsigned int key)
{
#if 1
	int i ;

	for(i=0;i<5;i++)
	{
		skin_value_key[i] = skin_value_key[i+1] ;
	}
	skin_value_key[i] = key ;

	if( ( STBK_9      == skin_value_key[0] ) &&
	    ( STBK_8      == skin_value_key[1] ) &&
		( STBK_7      == skin_value_key[2] ) &&
	    ( STBK_6      == skin_value_key[3] ) &&
		( STBK_UP     == skin_value_key[4] ) &&
		( STBK_LEFT   == skin_value_key[5] ))
	{
		SKIN_DBG("[skin seta] #### into recovery mode \n");

		gxapp_skin_flash_recovery_info();
		gxapp_skin_flash_set_update_error_flag(0);

		gxapp_skin_app_reboot();

		return 1 ;
	}
#endif
	return 0 ;
}

//--------------------------------------------------------

#endif
