/*************************************************************************
	> File Name: app_applets.c
	> Author:
	> Mail:
	> Created Time: 2022/3/24
 ************************************************************************/

#include "app.h"

#if SKIN_SUPPORT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gx_apps.h>
#include <parser.h>
#include <tree.h>

#include "app_skin_xml.h"
#include "app_skin_setting.h"
#include "app_skin_version.h"

#ifdef APP_SDK_VERSION_LOW
#define GXxmlFree GxCore_Free
#else
#include <memmgr.h>
#endif

#define XML_FREE_ITEM(item) do\
{\
	if ( item != NULL)\
	{\
		GXxmlFree(item);\
		item = NULL ;\
	}\
} while(0)

static char *s_xml_parse_server = NULL;
static unsigned int s_installed_uuid[ALL_SKIN_PARTITION_MAX_NUM] = {0};

char *gxapp_get_xml_parse_server(void)
{
	return s_xml_parse_server ;
}

int gxapp_xml_module_uninit(void)
{
	if ( s_xml_parse_server != NULL )
	{
		APP_FREE(s_xml_parse_server);
		s_xml_parse_server = NULL ;
	}
	return 0 ;
}

int gxapp_set_installed_uiid(int i, unsigned int uiid)
{
	if ( (i<0) || (i>=ALL_SKIN_PARTITION_MAX_NUM) )
		return -1 ;
	
	s_installed_uuid[i] = uiid ;
	
	return 0 ;
}

int gxapp_exist_installed_uiid(unsigned int cur_uuid)
{
	int i ;
	if (cur_uuid == 0 )
		return 0 ;

	for (i=0;i<ALL_SKIN_PARTITION_MAX_NUM;i++)
	{
		if ( s_installed_uuid[i] == 0 )
			continue ;

		if ( s_installed_uuid[i] == cur_uuid )
		{
			SKIN_DBG("[skin xml] exist installed id=%d, uiid=%d \n",i,cur_uuid);

			return 1 ;
		}
	}
	return 0 ;
}

static int gxapp_get_xml_data(GXxmlDocPtr doc,skin_item_node_t *item_data)
{
	char path[FULL_XML_PATH_LENGTH] ;

    GXxmlNodePtr root;
	GXxmlNodePtr item = NULL;

	char *pversion = NULL ;
	char *pserver  = NULL ;
	char *name = NULL ;
	char *uiid = NULL ;

	char *icon_name = NULL ;
	char *simg_name = NULL ;
	char *simg_md5  = NULL ;
	char *dimg_md5  = NULL ;
	char *pre0_name = NULL ;
	char *pre1_name = NULL ;
	char *pre2_name = NULL ;
	char *pre3_name = NULL ;
	char *pre4_name = NULL ;

	int parse_num = 0 ;
	int pre_num = 0 ;

    //printf("parse_data_xml\n");
    root = doc->root;
	if((!root )||(0 !=  GXxmlStrcmp(root->name, (const unsigned char *)"list")))
    {
        SKIN_ERR("[skin xml] no valid root node\n");
        return -1;
    }

	item = root->childs;
	if ( NULL == item )
	{
        SKIN_ERR("[skin xml] no valid item node\n");
        return -2;
	}

	while(item)
	{
		if ((!pversion)&&(0 == GXxmlStrcmp(item->name, (const unsigned char *)"version")))
		{
			pversion = (char *)GXxmlNodeGetContent(item);
			if ( pversion )
			{
				SKIN_DBG("[skin xml] version =%s\n", pversion);

				if  ( gxapp_skin_version_is_matched(pversion) <= 0 )
				{
					SKIN_ERR("[skin xml] version matche failer \n");
					return -30;
				}
			}
		}
		else if ((!pserver)&&(0 == GXxmlStrcmp(item->name, (const unsigned char *)"server")))
		{
			pserver = (char *)GXxmlNodeGetContent(item);
			if ( pserver )
			{
				SKIN_DBG("[skin xml] server url=%s\n", pserver);

				if ( s_xml_parse_server == NULL )
				{
					SKIN_DBG("[skin xml] server add \n");
					s_xml_parse_server = GxCore_Strdup(pserver) ;
				}
			}
		}
		else if((!name)&&(0 == GXxmlStrcmp(item->name, (const unsigned char *)"name")))
		{
			name = (char *)GXxmlNodeGetContent(item);
			//printf("name=%s\n", name);
		}
		else if((!uiid)&&(0 == GXxmlStrcmp(item->name, (const unsigned char *)"uiid")))
		{
			uiid = (char *)GXxmlNodeGetContent(item);
			//printf("uiid=%s\n", uiid);
		}
		else if((!icon_name)&&(0 == GXxmlStrcmp(item->name, (const unsigned char *)"icon_name")))
		{   
			icon_name = (char *)GXxmlNodeGetContent(item);
			//printf("icon name=%s\n", icon_name);
		}
		else if((!simg_name)&&(0 == GXxmlStrcmp(item->name, (const unsigned char *)"simg_name")))
		{
			simg_name = (char *)GXxmlNodeGetContent(item);
			//printf("img name=%s\n", simg_name);
		}
		else if((!simg_md5)&&(0 == GXxmlStrcmp(item->name, (const unsigned char *)"simg_md5")))
		{   
			simg_md5 = (char *)GXxmlNodeGetContent(item);
			//printf("img md5=%s\n", simg_md5);
		}
		else if((!dimg_md5)&&(0 == GXxmlStrcmp(item->name, (const unsigned char *)"dimg_md5")))
		{
			dimg_md5 = (char *)GXxmlNodeGetContent(item);
			//printf("img md5=%s\n", simg_md5);
		}
		else if((!pre0_name)&&(0 == GXxmlStrcmp(item->name, (const unsigned char *)"pre0_name")))
		{   
			pre0_name = (char *)GXxmlNodeGetContent(item);
			//printf("pre0 name=%s\n", pre0_name);
		}
		else if((!pre1_name)&&(0 == GXxmlStrcmp(item->name, (const unsigned char *)"pre1_name")))
		{
			pre1_name = (char *)GXxmlNodeGetContent(item);
			//printf("pre1 name=%s\n", pre1_name);
		}
		else if((!pre2_name)&&(0 == GXxmlStrcmp(item->name, (const unsigned char *)"pre2_name")))
		{
			pre2_name = (char *)GXxmlNodeGetContent(item);
			//printf("pre2 name=%s\n", pre2_name);
		}
		else if((!pre3_name)&&(0 == GXxmlStrcmp(item->name, (const unsigned char *)"pre3_name")))
		{
			pre3_name = (char *)GXxmlNodeGetContent(item);
			//printf("pre3 name=%s\n", pre3_name);
		}
		else if((!pre4_name)&&(0 == GXxmlStrcmp(item->name, (const unsigned char *)"pre4_name")))
		{
			pre4_name = (char *)GXxmlNodeGetContent(item);
			//printf("pre4 name=%s\n", pre4_name);
		}
				
		item = item->next;
	}

	pre_num   = 0;

	if(uiid&&simg_name&&simg_md5)
	{
		if ( strlen(uiid) > 0 )
		{
			item_data->uiid = atoi(uiid) ;
		}
		else
		{
			item_data->uiid = 0 ;
		}

		if(name)
			item_data->name = GxCore_Strdup(name) ;
		else
			item_data->name = GxCore_Strdup("default") ;

		memset(path,0,FULL_XML_PATH_LENGTH);
		snprintf(path, FULL_XML_PATH_LENGTH, "%s/%s", item_data->ddir, simg_name);
		item_data->simg_url = GxCore_Strdup(path) ;
		item_data->simg_md5 = GxCore_Strdup(simg_md5) ;

		if(icon_name)
		{
			memset(path,0,FULL_XML_PATH_LENGTH);
			snprintf(path, FULL_XML_PATH_LENGTH, "%s/%s", item_data->ddir, icon_name);
			item_data->icon_url = GxCore_Strdup(path) ;
		}
					
		if(dimg_md5)
		{
			item_data->dimg_md5 = GxCore_Strdup(dimg_md5) ;
		}

		if ( pre0_name && (pre_num < MAX_PREVIEW_NODE_NUM) )
		{
			memset(path,0,FULL_XML_PATH_LENGTH);
			snprintf(path, FULL_XML_PATH_LENGTH, "%s/%s", item_data->ddir, pre0_name);
			item_data->preview[pre_num].pre_url = GxCore_Strdup(path) ;
			pre_num ++;
		}
		if ( pre1_name && (pre_num < MAX_PREVIEW_NODE_NUM) )
		{
			memset(path,0,FULL_XML_PATH_LENGTH);
			snprintf(path, FULL_XML_PATH_LENGTH, "%s/%s", item_data->ddir, pre1_name);
			item_data->preview[pre_num].pre_url = GxCore_Strdup(path) ;
			pre_num ++;
		}
		if ( pre2_name && (pre_num < MAX_PREVIEW_NODE_NUM) )
		{
			memset(path,0,FULL_XML_PATH_LENGTH);
			snprintf(path, FULL_XML_PATH_LENGTH, "%s/%s", item_data->ddir, pre2_name);
			item_data->preview[pre_num].pre_url = GxCore_Strdup(path) ;
			pre_num ++;
		}
		if ( pre3_name && (pre_num < MAX_PREVIEW_NODE_NUM) )
		{
			memset(path,0,FULL_XML_PATH_LENGTH);
			snprintf(path, FULL_XML_PATH_LENGTH, "%s/%s", item_data->ddir, pre3_name);
			item_data->preview[pre_num].pre_url = GxCore_Strdup(path) ;
			pre_num ++;
		}
		if ( pre4_name && (pre_num < MAX_PREVIEW_NODE_NUM) )
		{
			memset(path,0,FULL_XML_PATH_LENGTH);
			snprintf(path, FULL_XML_PATH_LENGTH, "%s/%s", item_data->ddir, pre4_name);
			item_data->preview[pre_num].pre_url = GxCore_Strdup(path) ;
			pre_num ++;
		}

		parse_num++ ;
	}

	XML_FREE_ITEM(pversion);
	XML_FREE_ITEM(pserver);

	XML_FREE_ITEM(name);
	XML_FREE_ITEM(icon_name);

	XML_FREE_ITEM(simg_name);
	XML_FREE_ITEM(simg_md5);
	XML_FREE_ITEM(dimg_md5);

	XML_FREE_ITEM(pre0_name);
	XML_FREE_ITEM(pre1_name);
	XML_FREE_ITEM(pre2_name);
	XML_FREE_ITEM(pre3_name);
	XML_FREE_ITEM(pre4_name);

	SKIN_DBG("[skin xml] get num  = %d \n",parse_num);

	if ( parse_num <= 0)
	{
		return -1 ;
	}

    return 0;
}

int gxapp_buffer_get_data(char *buffer, unsigned int size,skin_data_node_t *pdata)
{
	gxapps_ret_t ret = GXAPPS_SERVER_ERROR;
    GXxmlDocPtr doc;

    //printf("gxapp_buffer_get_data\n");
    doc = GXxmlParseMemory(buffer, size);
    if(!doc)
    {
        SKIN_ERR("[skin xml] doc is null\n");
        return GXAPPS_SERVER_ERROR;
    }
    //ret = gxapp_get_xml_data(doc,pdata);

    GXxmlFreeDoc(doc);
    return ret;
}

int gxapp_file_get_data(char *path,skin_item_node_t *item_data)
{
	gxapps_ret_t ret = GXAPPS_SERVER_ERROR;
	GXxmlDocPtr doc = NULL;

	if(GXCORE_FILE_UNEXIST == GxCore_FileExists(path))
        return -1;
	doc = GXxmlParseFile(path);
	if ( doc == NULL )
		return -2 ;

	ret = gxapp_get_xml_data(doc,item_data);

	GXxmlFreeDoc(doc);
    return ret;
}

int gxapp_htmlfile_get_data(char *path,skin_data_node_t *pdata)
{
	char *buffer = NULL ;
	FILE *fp = NULL;
	
	int nread = 0 ;
	int count = 0 ;
	int len = 0 ;

	char *str_begin = NULL;
    char *str_end = NULL;
	char *str_cur = NULL;

	pdata->dir_all_num  = 0 ;
	pdata->cur_down_num = 0 ;

	fp = fopen(path,"rb");
	if(fp == NULL )
	{
		SKIN_ERR("[skin xml] #### file read err \n");
		return -1 ;
	}
	
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	if(len <=0 )
	{
		SKIN_ERR("[skin xml] #### file len err \n");
		return -2 ;
	}

	SKIN_DBG("[skin xml] file len = %d \n",len);

	len = len + 16 + (16 - (len % 16) ) ;

	buffer = GxCore_Malloc(len);
    if(buffer == NULL)
    {
        SKIN_ERR("[skin xml] #### malloc err \n");
		return -2 ;
    }

	nread = fread(buffer, 1, len, fp);
	
	if (nread <=0 )
	{
		GxCore_Free(buffer);
		fclose(fp);
		SKIN_ERR("[skin xml] #### len err \n");
		return -3 ;
	}
	else
	{
		buffer[nread] = '\0' ;
	}

	SKIN_DBG("[skin xml] file read = %d , %d \n",nread,len);

	//SKIN_DBG("[sk xml] file con = %s \n",buffer);

	str_cur = buffer ;
	while(count<SKIN_MAX_LIST)
	{
		str_begin = NULL;
		str_begin = strstr(str_cur, SKIN_DIR_BEGIN);
		if(str_begin)
		{
            str_begin+=strlen(SKIN_DIR_BEGIN);
            str_end = strchr(str_begin, '"');
            if(str_end)
            {
				if ( (str_end - buffer) > nread )
				{
					SKIN_DBG("[skin xml] over len = %d \n",(str_end-buffer));
					break;
				}

				len = str_end - str_begin ;
				//SKIN_DBG("[sk xml] len=%d,end=%c\n", len,str_begin[len]);
				if ( (len > 1) && (str_begin[len-1] == '/') )
				{
					pdata->node[count].ddir = gx_strndup(str_begin, str_end-str_begin-1);
					SKIN_DBG("[skin xml] dir=%s,count=%d\n", pdata->node[count].ddir,count+1);
					count ++ ;
				}
				else
				{
					SKIN_DBG("[skin xml] dir skip = %d \n", len);
				}

				str_cur = str_end+1;
            }
			else
			{
				SKIN_DBG("[skin xml] break, not find end,n=%d \n",count);
				break;
			}
        }
		else
		{
			SKIN_DBG("[skin xml] break, not find begin,n=%d \n",count);
			break;
		}
	}

	GxCore_Free(buffer);
	fclose(fp);

	pdata->dir_all_num = count ;

	if (count <=0)
		return -10 ;

    return 0;
}

static int gxapp_get_xml_simple_data(GXxmlDocPtr doc,skin_item_node_t *item_data)
{
	//int ret = -1;
    GXxmlNodePtr root;
	GXxmlNodePtr item = NULL;

	char *name = NULL ;
	char *uiid = NULL ;

    //printf("parse_data_xml\n");
    root = doc->root;
	if((!root )||(0 !=  GXxmlStrcmp(root->name, (const unsigned char *)"list")))
    {
        SKIN_ERR("[skin xml] no valid root node\n");
        return -1;
    }
	item = root->childs;
	if ( NULL == item )
	{
        SKIN_ERR("[skin xml] no valid item node\n");
        return -2;
	}

	if ( 0 == GXxmlStrcmp(item->name, (const unsigned char *)"name") )
	{
		name = (char *)GXxmlNodeGetContent(item);
		if(name)
		{
			item_data->name = GxCore_Strdup(name) ;
			GXxmlFree(name);
			name = NULL ;
		}

		item = item->next;
		if ( NULL == item )
		{
			SKIN_ERR("[skin xml] no valid next node\n");
			return -3;
		}
	}
	else
	{
		SKIN_DBG("[skin xml] simple no name item \n");
	}

	item_data->uiid = 0 ;

	if ( 0 == GXxmlStrcmp(item->name, (const unsigned char *)"uiid") )
	{
		uiid = (char *)GXxmlNodeGetContent(item);
		if(uiid)
		{
			item_data->uiid = atoi(uiid) ;
			GXxmlFree(uiid);
			uiid = NULL ;
		}
		else
		{
			SKIN_DBG("[skin xml] no uuid con \n");
		}
	}
	else
	{
		SKIN_DBG("[skin xml] simple no uuid item \n");
	}

	SKIN_DBG("[skin xml] get simple finish \n");

	return 0 ;
}

int gxapp_file_get_simple_data(char *path,int i, skin_item_node_t *item_data)
{
	gxapps_ret_t ret = 0;
	GXxmlDocPtr doc = NULL;

	if(GXCORE_FILE_UNEXIST == GxCore_FileExists(path))
	{
        return -1;
	}
	doc = GXxmlParseFile(path);
	if ( doc != NULL )
	{
		ret = gxapp_get_xml_simple_data(doc,item_data);
		GXxmlFreeDoc(doc);
	}

    return ret ;
}

int gxapp_free_data(skin_data_node_t *pdata)
{
	int i ,j ;
	int num ;

	if (pdata == NULL)
		return -1 ;

	pdata->cur_down_num = 0 ;

	if (pdata->node == NULL)
	{
		pdata->dir_all_num  = 0 ;
		pdata->num = 0 ;
		return -1 ;
	}

	num = pdata->num ;
	for(i=0;i<num ;i++)
	{
		APP_FREE(pdata->node[i].ddir);
		APP_FREE(pdata->node[i].name);
		APP_FREE(pdata->node[i].icon_url);
		APP_FREE(pdata->node[i].simg_url);
		APP_FREE(pdata->node[i].simg_md5);
		APP_FREE(pdata->node[i].dimg_md5);

		for(j=0;j<MAX_PREVIEW_NODE_NUM ;j++)
		{
			APP_FREE(pdata->node[i].preview[j].pre_url);
		}
	}
	j = pdata->dir_all_num ;

	SKIN_DBG("[skin xml] free data,dir=%d,num=%d \n",j,num);

	if ( (j > 0) && (num < j) )
	{
		for(i=num;i<j;i++)
		{
			if (pdata->node[i].ddir)
			{
				SKIN_DBG("[skin xml] free ddir,i=%d \n",i);
			}
			APP_FREE(pdata->node[i].ddir);
		}
	}

	APP_FREE(pdata->node);
    pdata->node = NULL;
	
	pdata->dir_all_num = 0;
	pdata->num = 0 ;

	return 0 ;
}

#endif
