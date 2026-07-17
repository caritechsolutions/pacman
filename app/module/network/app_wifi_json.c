#include "app.h"
#include "jansson.h"

#ifdef ECOS_OS
#if NETWORK_SUPPORT && WIFI_SUPPORT

#define AP_RECORD_FILE "/home/gx/ap_psk_record"

static handle_t s_ap_record_mutex = -1;

static int _save_object(json_t * pObj, char * pMac, char * pSsid, char * pPsk, int hidden)
{
    if(json_object_set(pObj, "mac", json_string(pMac)) < 0)
        return -1;

    if(pSsid && strlen(pSsid) != 0)
    {
        if(json_object_set(pObj, "ssid", json_string(pSsid)) < 0)
            return -1;
    }

    if(pPsk && strlen(pPsk) != 0)
    {
        if(json_object_set(pObj, "psk", json_string(pPsk)) < 0)
            return -1;
    }

    if(json_object_set(pObj, "hidden", json_integer(hidden)) < 0)
        return -1;

    return 0;
}

static int app_wifi_psk_spacial_string_replace(char *psk,int pskmaxlen)
{
	int len = 0, new_len = 0;
	int i = 0, j = 0, m = 0;
	int replace = 0;
	char *new = NULL;
	char specialString[] = {'\\', '"', '/'};

	if (psk == NULL)
		return GXCORE_ERROR;

	len = strlen(psk);
	new_len = len;
	new = GxCore_Malloc(len);
	memset(new, 0, len);
	for(i=0; i<len; i++)
	{
		for(m=0; m<sizeof(specialString); m++)
		{
			if(psk[i] == specialString[m])
			{
				new_len += 1;
				new = (char *)GxCore_Realloc(new, new_len);
				memset(new+(new_len-1), 0, 1);
				new[j] = '\\';
				new[++j] = psk[i];
				replace = 1;
				break;
			}
		}
		new[j] = psk[i];
		j++;
	}
	if(new_len >= pskmaxlen)
	{
		replace = -1;
		GxCore_Free(new);
		return replace;
	}
	memset(psk, 0, len);
	memcpy(psk, new, new_len);
	GxCore_Free(new);

	return replace;
}

status_t app_wifi_ap_save(char * mac, char * ssid, char * psk,int pskmaxlen, int hidden)
{
    json_t *pArray   = NULL;
    json_t *pObj     = NULL;
    json_t *pMac     = NULL;
    json_t *pPsk     = NULL;
    json_error_t error;

    const char *pMacGet = NULL;
    const char *pPskGet = NULL;
    char pPskGetTmp[66] = {0};
    uint32_t i = 0, j = 0;
    uint32_t nSize = 0;
	int ret = 0;

    if(IS_EMPTY_OR_NULL(mac)) {
        app_log_error("[AP SAVE] mac is NULL!\n");
        return GXCORE_ERROR;
    }

    hidden = hidden ? 1:0;

	if(app_wifi_psk_spacial_string_replace(psk,pskmaxlen) < 0)
    {
        app_log_error("[AP SAVE] spacial_string!too long\n");
        return GXCORE_ERROR;
    }

    if(-1 == s_ap_record_mutex) {
        GxCore_MutexCreate(&s_ap_record_mutex);
    }

    if(GxCore_FileExists(AP_RECORD_FILE) != GXCORE_FILE_EXIST) {
        pObj = json_object();
        if(_save_object(pObj, mac, ssid, psk, hidden) < 0) {
            json_decref(pObj);
            return GXCORE_ERROR;
        }

        pArray = json_array();
        if(json_array_append(pArray, pObj) < 0) {
            json_decref(pObj);
            json_decref(pArray);
            return GXCORE_ERROR;
        }

        if((json_dump_file(pArray, AP_RECORD_FILE, 4) < 0)) {
            app_log_error("[AP SAVE0] dump file failed!\n");
            json_decref(pObj);
            json_decref(pArray);
            return GXCORE_ERROR;
        }

    } else {
        GxCore_MutexLock(s_ap_record_mutex);
        pArray = json_load_file(AP_RECORD_FILE, 0, &error);
        if(!json_is_array(pArray)) {
            app_log_error("\033[32m------[%s, %d] %s------\033[0m\n", __func__, __LINE__, error.text);
            json_decref(pArray);
            GxCore_MutexUnlock(s_ap_record_mutex);
            return GXCORE_ERROR;
        }

        nSize = json_array_size(pArray);
        for(i = 0; i < nSize; i++)
        {
            pObj = json_array_get(pArray, i);
            pMac = json_object_get(pObj, "mac");
            pMacGet = json_string_value(pMac);
            if(0 == strcmp(pMacGet, mac)) {
                break;
            }
        }

		// replace special string in the file
        for(j=0; j<nSize; j++)
        {
            pObj = json_array_get(pArray, j);
            pPsk = json_object_get(pObj, "psk");
            if(pPsk == NULL)
            {
                continue;
            }
            pPskGet = json_string_value(pPsk);
			memset(pPskGetTmp, 0, sizeof(pPskGetTmp));
			strncpy(pPskGetTmp, pPskGet, strlen(pPskGet));
			ret = app_wifi_psk_spacial_string_replace(pPskGetTmp,sizeof(pPskGetTmp));
            if(ret < 0)
            {
                json_decref(pObj);
                json_decref(pArray);
                GxCore_MutexUnlock(s_ap_record_mutex);
                return GXCORE_ERROR;
            }
			if(ret == 1) //replace
			{
				if(json_object_set(pObj, "psk", json_string(pPskGetTmp)) < 0) {
					json_decref(pObj);
					json_decref(pArray);
					GxCore_MutexUnlock(s_ap_record_mutex);
					return GXCORE_ERROR;
				}

				if(json_array_set(pArray, j, pObj) < 0) {
					json_decref(pObj);
					json_decref(pArray);
					GxCore_MutexUnlock(s_ap_record_mutex);
					return GXCORE_ERROR;
				}
			}
        }

        pObj = json_object();
        if(_save_object(pObj, mac, ssid, psk, hidden) < 0) {
            json_decref(pObj);
            json_decref(pArray);
            GxCore_MutexUnlock(s_ap_record_mutex);
            return GXCORE_ERROR;
        }

        if(i == nSize) { //no found
            if(20 == nSize) {
                app_log_error(" @@@@@@@@@@@@@@@@@@@ >> delete old ap record!\n");
                if(json_array_remove(pArray, 0) < 0) {
                    json_decref(pObj);
                    json_decref(pArray);
                    GxCore_MutexUnlock(s_ap_record_mutex);
                    return GXCORE_ERROR;
                }
            }

            if(json_array_append(pArray, pObj) < 0) {
                json_decref(pObj);
                json_decref(pArray);
                GxCore_MutexUnlock(s_ap_record_mutex);
                return GXCORE_ERROR;
            }
        }
        else
        {
            if(json_array_set(pArray, i, pObj) < 0) {
                json_decref(pObj);
                json_decref(pArray);
                GxCore_MutexUnlock(s_ap_record_mutex);
                return GXCORE_ERROR;
            }
        }

        //app_log_debug("\n\nJson : %s\n", json_dumps(pArray, 2));
        if(json_dump_file(pArray, AP_RECORD_FILE, 4) < 0) {
            app_log_error("[AP SAVE] dump file failed!\n");
            json_decref(pObj);
            json_decref(pArray);
            GxCore_MutexUnlock(s_ap_record_mutex);
            return GXCORE_ERROR;
        }
        GxCore_MutexUnlock(s_ap_record_mutex);
    }

    json_decref(pObj);
    json_decref(pArray);
    return GXCORE_SUCCESS;
}

status_t app_wifi_ap_load(char * mac, char * ssid, char * psk, int *hidden)
{
    json_t *pArray    = NULL;
    json_t *pObj      = NULL;
    json_t *pMac      = NULL;
    json_t *pSsid = NULL;
    json_t *pPsk  = NULL;
    json_error_t error;

    const char *pMacGet = NULL ;
    const char *pSsidGet = NULL ;
    const char *pPskGet = NULL ;
    int i = 0;
    uint32_t nSize = 0;

    if(IS_EMPTY_OR_NULL(mac)) {
        app_log_error("[AP LOAD] mac is NULL!\n");
        return GXCORE_ERROR;
    }

    if(GxCore_FileExists(AP_RECORD_FILE) != GXCORE_FILE_EXIST){
        app_log_error("[AP LOAD] file /home/gx/ap_psk_record unexist!\n");
        return GXCORE_ERROR;
    }
    if(-1 == s_ap_record_mutex) {
        GxCore_MutexCreate(&s_ap_record_mutex);
    }

    GxCore_MutexLock(s_ap_record_mutex);
    pArray = json_load_file(AP_RECORD_FILE, 0, &error);
    //app_log_debug("Json : %s\n", json_dumps(pArray, 2));
    if(!json_is_array(pArray)) {
        app_log_error("\033[32m------[%s, %d] %s------\033[0m\n", __func__, __LINE__, error.text);
        json_decref(pArray);
        GxCore_MutexUnlock(s_ap_record_mutex);
        return GXCORE_ERROR;
    }

    nSize = json_array_size(pArray);
    for(i = 0; i < nSize; i++)
    {
        pObj = json_array_get(pArray, i);
        pMac = json_object_get(pObj, "mac");
        pMacGet = json_string_value(pMac);
        if(0 == strcmp(pMacGet, mac)) {
            pPsk = json_object_get(pObj, "psk");
            if (pPsk != NULL) {
                pPskGet = json_string_value(pPsk);
                strcpy(psk, pPskGet);
            }
            pSsid = json_object_get(pObj, "ssid");
            if (pSsid != NULL)
            {
                pSsidGet = json_string_value(pSsid);
                strcpy(ssid, pSsidGet);
            }
            *hidden = json_integer_value(json_object_get(pObj, "hidden"));
            break;
        }
    }

#if 0
    if(json_dump_file(pArray, AP_RECORD_FILE, 4) < 0) {
        app_log_error("[AP LOAD] dump file failed!\n");
        json_decref(pArray);
        GxCore_MutexUnlock(s_ap_record_mutex);
        return GXCORE_ERROR;
    }
#endif
    GxCore_MutexUnlock(s_ap_record_mutex);

    json_decref(pArray);
    if(i < nSize)
        return GXCORE_SUCCESS;

    return GXCORE_ERROR;
}

#endif
#endif
