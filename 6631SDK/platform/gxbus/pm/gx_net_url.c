#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"
#include "module/pm/gxpm_urlcontrol.h"
#include "gui_core.h"

#define GX_URL_FS_OPEN(path,mode)               (GxCore_Open(path, mode))
#define GX_URL_FS_EXIST(path)                   (GxCore_FileExists(path))
#define GX_URL_FS_READ(file,buf,size,nmemb)     (GxCore_Read(file,buf,size,nmemb))
#define GX_URL_FS_WRITE(file,buf,size,nmemb)    (GxCore_Write(file,buf,size,nmemb))
#define GX_URL_FS_CLOSE(file)                   (GxCore_Close(file))
#define GX_URL_FS_RM(path)                      (GxCore_FileDelete(path))
#define GX_URL_FS_INFO(file,info)               (GxCore_GetFileStat(file,info))
#define GX_URL_FS_SEEK(file,offset,whence)      (GxCore_Seek(file,offset,whence))
#define GX_URL_WRITEAT(file,buf,size,nmemb,pos) (GxCore_WriteAt(file, buf,size,nmemb,pos))
#define GX_URL_READAT(file,buf,size,nmemb,pos)  (GxCore_ReadAt(file,buf,size,nmemb,pos))
#define FILE_MODE  "c"
#define GX_BUFFER_FREE(x)	if(x){GxCore_Free(x);x=NULL;}

#define GX_SERVER_URL_DEFAULT_PATH              "/home/gx/server_url.json"
#define GX_STREAM_URL_DEFAULT_PATH              "/home/gx/stream_url.json"
#define GX_MAX_SERVER_URL_NUM                   (100)
#define GX_MAX_STREAM_URL_NUM                   (4000)
#define GX_URL_BASE_NUM                         (100)
#define URL_ID_USE                              (1)
#define URL_ID_NOT_USE                          (0)
#define INDEX_DIGIT                             (4)
#define INDEX_STR_LEN							INDEX_DIGIT + 1
#define INDEX_FORMAT							"%04d"

static uint8_t *server_url_id_arry = NULL;
static uint8_t *stream_url_id_arry = NULL;
static uint32_t s_max_server_url_id = 0;
static uint32_t s_max_stream_url_id = 0;
static uint16_t id_malloc_count = 1;
static NetPmCtrl *urlcls_ctrl = NULL;

static int32_t gx_get_server_valid_url_id(void)
{
    uint32_t index = 0;
    int i = 0;
    if(s_max_server_url_id == 0)
    {
        server_url_id_arry = (uint8_t*)GxCore_Calloc(GX_URL_BASE_NUM, sizeof(uint8_t));
        if(server_url_id_arry == NULL)
        {
            gxloge("[%s]:[%d]malloc error\n",__FUNCTION__, __LINE__);
            return -1;
        }
    }
    if(s_max_server_url_id == GX_MAX_SERVER_URL_NUM)
    {
        for(i = 0; i < GX_MAX_SERVER_URL_NUM; i++)
        {
            if(server_url_id_arry[i] == URL_ID_NOT_USE)
            {
                index = i + 1;
                break;
            }
        }
		if(i == GX_MAX_SERVER_URL_NUM)
		{
			return -1;
		}
    }
    else
    {
        s_max_server_url_id ++;
        index = s_max_server_url_id;
    }
    server_url_id_arry[index - 1] = URL_ID_USE;
    return index;
}

char* gx_build_index_to_string_buffer(uint32_t *index_arry, uint32_t index_count)
{
    char *char_index = NULL;
	char *temp_buffer = NULL;
	uint32_t i = 0;
	uint32_t j = 0;

    temp_buffer = (char*)GxCore_Calloc(INDEX_STR_LEN + 1, sizeof(char));
	char_index = temp_buffer;
    if(temp_buffer != NULL)
    {
		for(i = 0; i < index_count; i++)
		{
			sprintf(temp_buffer, INDEX_FORMAT, index_arry[i]);
			temp_buffer = temp_buffer + INDEX_STR_LEN;
		}
    }
	return char_index;
}


static void gx_set_invalid_server_url_id(uint32_t index)
{
    if(index > 0 && index <= s_max_server_url_id)
        server_url_id_arry[index - 1] = URL_ID_NOT_USE;
}

static void gx_set_invalid_stream_url_id(uint32_t index)
{
    if(index > 0 && index <= s_max_stream_url_id)
		stream_url_id_arry[index - 1] = URL_ID_NOT_USE;
}

static uint32_t gx_get_tp_valid_url_id(void)
{
    uint32_t index = 0;
    int i = 0;
    uint8_t *temp_buff = NULL;
    if(s_max_stream_url_id == 0)
    {
        stream_url_id_arry = (uint8_t*)GxCore_Calloc(GX_URL_BASE_NUM, sizeof(uint8_t));
        if(stream_url_id_arry == NULL)
        {
            gxloge("[%s]:[%d]malloc error\n",__FUNCTION__, __LINE__);
            return -1;
        }
    }
    if(s_max_stream_url_id == id_malloc_count * GX_URL_BASE_NUM)
    {
        for(i = 0; i < id_malloc_count * GX_URL_BASE_NUM; i++)
        {
            if(stream_url_id_arry[i] == URL_ID_NOT_USE)
            {
                index = i + 1;
                break;
            }
        }
        if(i == id_malloc_count * GX_URL_BASE_NUM && i < GX_MAX_STREAM_URL_NUM)
        {
            id_malloc_count++;
            temp_buff = (uint8_t*)GxCore_Calloc(id_malloc_count * GX_URL_BASE_NUM, sizeof(uint8_t));
            if(temp_buff == NULL)
            {
                gxloge("[%s]:[%d]malloc error\n",__FUNCTION__, __LINE__);
                id_malloc_count--;
                return -1;
            }
            if(stream_url_id_arry != NULL)
            {
                GX_BUFFER_FREE(stream_url_id_arry);
            }
            stream_url_id_arry = temp_buff;
            memset(stream_url_id_arry, URL_ID_USE, (id_malloc_count - 1) * GX_URL_BASE_NUM);
            s_max_stream_url_id ++;
            index = s_max_stream_url_id;
        }
    }
    else
    {
        s_max_stream_url_id ++;
        index = s_max_stream_url_id;
    }
    stream_url_id_arry[index - 1] = URL_ID_USE;
    return index;

}

static status_t gx_check_server_index_valid(uint32_t *index, uint32_t check_num)
{
	uint32_t i = 0;

	if(index == NULL)
	{
		return GXCORE_ERROR;
	}
	for(i = 0; i < check_num; i++)
	{
		if(server_url_id_arry[index[i] - 1] != URL_ID_USE)
			return GXCORE_ERROR;
	}
	return GXCORE_SUCCESS;
}

static status_t gx_check_stream_index_valid(uint32_t *index, uint32_t check_num)
{
	uint32_t i = 0;

	if(index == NULL)
	{
		return GXCORE_ERROR;
	}
	for(i = 0; i < check_num; i++)
	{
		if(stream_url_id_arry[index[i] - 1] != URL_ID_USE)
			return GXCORE_ERROR;
	}
	return GXCORE_SUCCESS;
}

static cJSON * gx_get_json_object(char *path)
{
	handle_t file;
    uint32_t data_len = 0;
    cJSON *root_json = NULL;
    char *buffer = NULL;

    if(path == NULL)
    {
        return NULL;
    }
    file = GX_URL_FS_OPEN(path, FILE_MODE);
    if(file < 0)
    {
        return NULL;
    }
    data_len = GX_URL_FS_SEEK(file, 0, SEEK_END);
    GX_URL_FS_SEEK(file, 0, SEEK_SET);
    buffer = (char*)GxCore_Calloc(data_len + 1, sizeof(buffer));
    if(buffer == NULL)
    {
        GX_URL_FS_CLOSE(file);
        return NULL;
    }
	if(data_len != 0)
    {
        GX_URL_FS_READ(file, buffer, 1, data_len);
        root_json = cJSON_Parse(buffer);
    }
    else
    {
        root_json = cJSON_CreateObject();
    }
    GX_URL_FS_CLOSE(file);
	GX_BUFFER_FREE(buffer);
	return root_json;
}

static status_t gx_get_url_from_json(char* path, char *key, char **value, uint32_t get_num)
{
	handle_t file;
    uint32_t data_len = 0;
    cJSON *root_json = NULL;
    cJSON *indexItem = NULL;
    char *buffer = NULL;
	char *temp_key = NULL;
	int16_t i = 0;

    if(path == NULL || key == NULL || value == NULL)
    {
        return GXCORE_ERROR;
    }
	for(i = 0; i < get_num; i++)
	{
		if(value[i] == NULL)
		{
			return GXCORE_ERROR;
		}
	}
	temp_key = key;
	root_json = gx_get_json_object(path);
	if(root_json == NULL)
	{
		return GXCORE_ERROR;
	}
	for(i = 0; i < get_num; i++)
	{
		indexItem = cJSON_GetObjectItem(root_json, temp_key);
    	if(indexItem == NULL)
    	{
    	    cJSON_Delete(root_json);
    	    return GXCORE_ERROR;
    	}
		temp_key = temp_key + INDEX_STR_LEN;
		gxlogd("gx_url: %s\n", cJSON_Print(indexItem));
		memcpy(value[i], indexItem->valuestring, strlen(indexItem->valuestring));
	}
    cJSON_Delete(root_json);
    return GXCORE_SUCCESS;
}

static status_t gx_add_url_to_json(char* path, char *key, char *value)
{
	handle_t file;
    char *out = NULL;
    int32_t data_len = 0;
    cJSON *root_json = NULL;

    if(path == NULL || key == NULL || value == NULL)
    {
        return GXCORE_ERROR;
    }
	if(strlen(value) > GX_MAX_SERVER_LEN)
	{
		gxlogd("add server url too long\n");
		return GXCORE_ERROR;
	}
	root_json = gx_get_json_object(path);
	if(root_json == NULL)
	{
		return GXCORE_ERROR;
	}
	cJSON_AddItemToObject(root_json, key, cJSON_CreateString(value));
	file = GX_URL_FS_OPEN(path, FILE_MODE);
	if(file < 0)
	{
		cJSON_Delete(root_json);
		return GXCORE_ERROR;
	}
    out = cJSON_Print(root_json);
	//out = cJSON_PrintUnformatted(root_json);
	if(out != NULL)
	{
		GX_URL_FS_WRITE(file, out, 1, strlen(out));
		GX_BUFFER_FREE(out);
	}
    cJSON_Delete(root_json);
    GX_URL_FS_CLOSE(file);
    return GXCORE_SUCCESS;
}

static status_t gx_delete_json_url_by_index(char *path, char *key, uint32_t del_num)
{
	int i = 0;
    cJSON *root_json = NULL;
	handle_t file;
	char *out = NULL;

    if(path == NULL || key == NULL)
    {
        return GXCORE_ERROR;
    }

	root_json = gx_get_json_object(path);
	if(root_json == NULL)
	{
		return GXCORE_ERROR;
	}
	for(i = 0; i < del_num; i++)
	{
		cJSON_DetachItemFromObject(root_json, key);
		key = key + INDEX_STR_LEN;
	}
	file = GX_URL_FS_OPEN(path, "w+");
	if(file < 0)
	{
		cJSON_Delete(root_json);
		return GXCORE_ERROR;
	}
    out = cJSON_Print(root_json);
	//out = cJSON_PrintUnformatted(root_json);
	if(out != NULL)
	{
		GX_URL_FS_WRITE(file, out, 1, strlen(out));
		GX_BUFFER_FREE(out);
	}
    cJSON_Delete(root_json);
    GX_URL_FS_CLOSE(file);
    return GXCORE_SUCCESS;
}

static status_t gx_modify_json_url_by_index(char *path, char *char_index, char *net_url)
{
    cJSON *root_json = NULL;
	handle_t file;
	char *out = NULL;

    if(path == NULL || char_index == NULL || net_url == NULL)
    {
        return GXCORE_ERROR;
    }
	if(strlen(net_url) > GX_MAX_SERVER_LEN)
	{
		gxlogd("error:net url too long\n");
		return GXCORE_ERROR;
	}
	root_json = gx_get_json_object(path);
	if(root_json == NULL)
	{
		return GXCORE_ERROR;
	}

	cJSON_ReplaceItemInObject(root_json, char_index, cJSON_CreateString(net_url));
	file = GX_URL_FS_OPEN(path, "w+");
	if(file < 0)
	{
		cJSON_Delete(root_json);
		return GXCORE_ERROR;
	}
    out = cJSON_Print(root_json);
	//out = cJSON_PrintUnformatted(root_json);
	if(out != NULL)
	{
		GX_URL_FS_WRITE(file, out, 1, strlen(out));
		GX_BUFFER_FREE(out);
	}
    cJSON_Delete(root_json);
    GX_URL_FS_CLOSE(file);

    return GXCORE_SUCCESS;
}

int gx_add_server_url_to_file(char *server_url, uint32_t *index)
{
    int32_t server_index = 0;
    char char_index[INDEX_STR_LEN]={0};

    if(server_url == NULL || index == NULL)
    {
        return GXCORE_ERROR;
    }

    server_index = gx_get_server_valid_url_id();
    if(server_index <= 0)
    {
        *index = 0;
        return GXCORE_ERROR;
    }
	*index = server_index;
    sprintf(char_index, INDEX_FORMAT, server_index);
    if(GXCORE_ERROR == gx_add_url_to_json(GX_SERVER_URL_DEFAULT_PATH, char_index, server_url))
    {
        gx_set_invalid_server_url_id(server_index);
		*index = 0;
        return GXCORE_ERROR;
    }
    return GXCORE_SUCCESS;
}

status_t gx_add_stream_url_to_file(char *stream_url, uint32_t *index)
{
    uint32_t stream_index = 0;
    char char_index[INDEX_STR_LEN]={0};

    if(stream_url == NULL || index == NULL)
    {
        return GXCORE_ERROR;
    }

    stream_index = gx_get_tp_valid_url_id();
    if(stream_index <= 0)
    {
        *index = 0;
        return GXCORE_ERROR;
    }
    *index = stream_index;
    sprintf(char_index, INDEX_FORMAT, stream_index);
    if(GXCORE_ERROR == gx_add_url_to_json(GX_STREAM_URL_DEFAULT_PATH, char_index, stream_url))
    {
		*index = 0;
        gx_set_invalid_stream_url_id(stream_index);
        return GXCORE_ERROR;
    }

    return stream_index;
}

status_t gx_get_server_url_by_index(uint32_t *server_index_arry, char **server_url_arry, uint32_t get_num)
{
	char *char_index = NULL;

    if(server_url_arry == NULL || server_index_arry == NULL || get_num == 0)
    {
        return GXCORE_ERROR;
    }
	if(GXCORE_SUCCESS != gx_check_server_index_valid(server_index_arry, get_num))
    {
    	return GXCORE_ERROR;
    }
	char_index = gx_build_index_to_string_buffer(server_index_arry, get_num);
	if(char_index == NULL)
		return GXCORE_ERROR;

	if(GXCORE_ERROR == gx_get_url_from_json(GX_SERVER_URL_DEFAULT_PATH, char_index, server_url_arry, get_num))
	{
		GX_BUFFER_FREE(char_index);
		return GXCORE_ERROR;
	}
	GX_BUFFER_FREE(char_index);
	return GXCORE_SUCCESS;
}

status_t gx_get_stream_url_by_index(uint32_t *stream_index_arry, char **stream_url_arry, uint32_t get_num)
{
	char *char_index = NULL;

    if(stream_url_arry == NULL || stream_index_arry == NULL || get_num == 0)
    {
        return GXCORE_ERROR;
    }
	if(GXCORE_SUCCESS != gx_check_stream_index_valid(stream_index_arry, get_num))
    {
    	return GXCORE_ERROR;
    }
	char_index = gx_build_index_to_string_buffer(stream_index_arry, get_num);
	if(char_index == NULL)
		return GXCORE_ERROR;

	if(GXCORE_ERROR == gx_get_url_from_json(GX_STREAM_URL_DEFAULT_PATH, char_index, stream_url_arry, get_num))
	{
		GX_BUFFER_FREE(char_index);
		return GXCORE_ERROR;
	}
	GX_BUFFER_FREE(char_index);
	return GXCORE_SUCCESS;
}

status_t gx_delete_stream_url_by_index(uint32_t *stream_index_arry, uint32_t del_num)
{
	uint32_t i = 0;
	char *char_index = NULL;

	if(stream_index_arry == NULL || del_num == 0)
	{
    	return GXCORE_ERROR;
	}
	if(GXCORE_SUCCESS != gx_check_stream_index_valid(stream_index_arry, del_num))
    {
    	return GXCORE_ERROR;
    }
	char_index = gx_build_index_to_string_buffer(stream_index_arry, del_num);
	if(char_index == NULL)
		return GXCORE_ERROR;
    if(GXCORE_SUCCESS != gx_delete_json_url_by_index(GX_STREAM_URL_DEFAULT_PATH, char_index, del_num))
    {
		GX_BUFFER_FREE(char_index);
        return GXCORE_ERROR;
    }
	for(i = 0; i < del_num; i++)
	{
		gx_set_invalid_stream_url_id(stream_index_arry[i]);
	}

	GX_BUFFER_FREE(char_index);
    return GXCORE_SUCCESS;
}

status_t gx_delete_server_url_by_index(uint32_t *server_index_arry, uint32_t del_num)
{
	uint32_t i = 0;
	char *char_index = NULL;

	if(server_index_arry == NULL || del_num <= 0)
	{
		return GXCORE_ERROR;
	}
	if(GXCORE_SUCCESS != gx_check_server_index_valid(server_index_arry, del_num))
    {
    	return GXCORE_ERROR;
    }
	char_index = gx_build_index_to_string_buffer(server_index_arry, del_num);
	if(char_index == NULL)
		return GXCORE_ERROR;
    if(GXCORE_SUCCESS != gx_delete_json_url_by_index(GX_SERVER_URL_DEFAULT_PATH, char_index, del_num))
    {
		GX_BUFFER_FREE(char_index);
        return GXCORE_ERROR;
    }
	for(i = 0; i < del_num; i++)
	{
		gx_set_invalid_server_url_id(server_index_arry[i]);
	}
	GX_BUFFER_FREE(char_index);
    return GXCORE_SUCCESS;
}

status_t gx_modify_server_url_by_index(uint32_t server_index, char *server_url)
{
	char *char_index = NULL;

	if(server_url == NULL)
	{
		return GXCORE_ERROR;
	}
	if(GXCORE_SUCCESS != gx_check_server_index_valid(&server_index, 1))
    {
    	return GXCORE_ERROR;
    }
	char_index = gx_build_index_to_string_buffer(&server_index, 1);
	if(char_index == NULL)
		return GXCORE_ERROR;
	if(GXCORE_SUCCESS !=  gx_modify_json_url_by_index(GX_SERVER_URL_DEFAULT_PATH, char_index, server_url))
	{
		GX_BUFFER_FREE(char_index);
		return GXCORE_ERROR;
	}
	GX_BUFFER_FREE(char_index);
    return GXCORE_SUCCESS;
}

status_t gx_modify_stream_url_by_index(uint32_t stream_index, char *stream_url)
{
	char *char_index = NULL;

	if(stream_url == NULL)
	{
    	return GXCORE_ERROR;
	}
	if(GXCORE_SUCCESS != gx_check_stream_index_valid(&stream_index, 1))
    {
    	return GXCORE_ERROR;
    }
	char_index = gx_build_index_to_string_buffer(&stream_index, 1);
	if(char_index == NULL)
		return GXCORE_ERROR;
	if(GXCORE_SUCCESS !=  gx_modify_json_url_by_index(GX_STREAM_URL_DEFAULT_PATH, char_index, stream_url))
	{
		GX_BUFFER_FREE(char_index);
		return GXCORE_ERROR;
	}
	GX_BUFFER_FREE(char_index);
    return GXCORE_SUCCESS;
}

status_t gx_url_manage_init(void)
{
    cJSON *root_json = NULL;
	uint32_t url_count = 0;
	int i = 0;
	char buffer[INDEX_STR_LEN] = {0};
	uint32_t used_count = 0;

	root_json = gx_get_json_object(GX_SERVER_URL_DEFAULT_PATH);
	if(root_json == NULL)
	{
		return GXCORE_ERROR;
	}
    url_count = cJSON_GetArraySize(root_json);
	if(url_count > GX_MAX_SERVER_URL_NUM)
	{
		return GXCORE_ERROR;
	}
	GX_BUFFER_FREE(server_url_id_arry);
	server_url_id_arry = (uint8_t*)GxCore_Calloc(GX_URL_BASE_NUM, sizeof(uint8_t));
    if(server_url_id_arry == NULL)
    {
		gxloge("[%s]:[%d]malloc error\n",__FUNCTION__, __LINE__);
        return GXCORE_ERROR;
    }
	if(url_count == 0)
	{
		memset(server_url_id_arry, URL_ID_NOT_USE, GX_URL_BASE_NUM);
		s_max_server_url_id = 0;
	}
	else
	{
		for(i = 1; i <= GX_MAX_SERVER_URL_NUM; i++)
		{
			memset(buffer, 0, INDEX_STR_LEN);
			sprintf(buffer, INDEX_FORMAT, i);
			if(NULL != cJSON_GetObjectItem(root_json, buffer))
			{
				server_url_id_arry[i - 1] = URL_ID_USE;
				used_count++;
			}
			else
			{
				server_url_id_arry[i - 1] = URL_ID_NOT_USE;
			}
			if(used_count == url_count)
			{
				break;
			}
		}
		s_max_server_url_id = i;
	}
	cJSON_Delete(root_json);

	root_json = gx_get_json_object(GX_STREAM_URL_DEFAULT_PATH);
	used_count = 0;
	url_count = 0;
	if(root_json == NULL)
	{
		return GXCORE_ERROR;
	}
    url_count = cJSON_GetArraySize(root_json);
	if(url_count > GX_MAX_STREAM_URL_NUM)
	{
		return GXCORE_ERROR;
	}
	id_malloc_count = url_count / (GX_URL_BASE_NUM + 1) + 1;
	GX_BUFFER_FREE(stream_url_id_arry);
	stream_url_id_arry = (uint8_t*)GxCore_Calloc(id_malloc_count * GX_URL_BASE_NUM, sizeof(uint8_t));
    if(stream_url_id_arry == NULL)
    {
		gxloge("[%s]:[%d]malloc error\n",__FUNCTION__, __LINE__);
        return GXCORE_ERROR;
    }
	if(url_count == 0)
	{
		memset(stream_url_id_arry, URL_ID_NOT_USE, id_malloc_count * GX_URL_BASE_NUM);
		s_max_stream_url_id = 0;
	}
	else
	{
		for(i = 1; i <= id_malloc_count * GX_URL_BASE_NUM; i++)
		{
			memset(buffer, 0, INDEX_STR_LEN);
			sprintf(buffer, INDEX_FORMAT, i);
			if(NULL != cJSON_GetObjectItem(root_json, buffer))
			{
				stream_url_id_arry[i - 1] = URL_ID_USE;
				used_count++;
			}
			else
			{
				stream_url_id_arry[i - 1] = URL_ID_NOT_USE;
			}
			if(used_count == url_count)
			{
				break;
			}
		}
		s_max_stream_url_id = i;
	}
	cJSON_Delete(root_json);
	return GXCORE_SUCCESS;
}

status_t GxNetAddUrlToFile(GxBusNetUrlType url_type, char *server_url, uint32_t *index)
{
	status_t ret = GXCORE_ERROR;

	switch(url_type)
	{
		case SERVER_URL:
			if(urlcls_ctrl->add_server_url_to_file)
			{
				ret = urlcls_ctrl->add_server_url_to_file(server_url, index);
			}
			else
			{
				gxlogd("net_url don`t register\n");
			}
			break;
		case STREAM_URL:
			if(urlcls_ctrl->add_stream_url_to_file)
			{
				ret = urlcls_ctrl->add_stream_url_to_file(server_url, index);
			}
			else
			{
				gxlogd("net_url don`t register\n");
			}
			break;
		default:
			break;
	}
	return ret;
}

status_t GxNetDeleteUrlByIndex(GxBusNetUrlType url_type, uint32_t *index_arry, uint32_t delete_num)
{
	status_t ret = GXCORE_ERROR;

	switch(url_type)
	{
		case SERVER_URL:
			if(urlcls_ctrl->delete_server_url_by_index)
			{
				ret = urlcls_ctrl->delete_server_url_by_index(index_arry, delete_num);
			}
			else
			{
				gxlogd("net_url don`t register\n");
			}

			break;
		case STREAM_URL:
			if(urlcls_ctrl->delete_stream_url_by_index)
			{
				ret = urlcls_ctrl->delete_stream_url_by_index(index_arry, delete_num);
			}
			else
			{
				gxlogd("net_url don`t register\n");
			}
			break;
		default:
			break;
	}
	return ret;
}

status_t GxNetModifyUrlByIndex(GxBusNetUrlType url_type, uint32_t index, char *net_url)
{
	status_t ret = GXCORE_ERROR;

	switch(url_type)
	{
		case SERVER_URL:
			if(urlcls_ctrl->modify_server_by_index)
			{
				ret = urlcls_ctrl->modify_server_by_index(index, net_url);
			}
			else
			{
				gxlogd("net_url don`t register\n");
			}
			break;
		case STREAM_URL:
			if(urlcls_ctrl->modify_stream_url_by_index)
			{
				ret = urlcls_ctrl->modify_stream_url_by_index(index, net_url);
			}
			else
			{
				gxlogd("net_url not register\n");
			}
			break;
		default:
			break;
	}
	return ret;
}

status_t GxNetGetUrlByIndex(GxBusNetUrlType url_type, uint32_t *index_arry, char **url_arry, uint32_t get_num)
{
	status_t ret = GXCORE_ERROR;

	switch(url_type)
	{
		case SERVER_URL:
			if(urlcls_ctrl->get_server_url_by_index)
			{
				ret = urlcls_ctrl->get_server_url_by_index(index_arry, url_arry, get_num);
			}
			else
			{
				gxlogd("strver_url not register\n");
			}
			break;
		case STREAM_URL:
			if(urlcls_ctrl->get_stream_url_by_index)
			{
				ret = urlcls_ctrl->get_stream_url_by_index(index_arry, url_arry, get_num);
			}
			else
			{
				gxlogd("stream_url not register\n");
			}
			break;
		default:
			break;
	}
	return ret;
}

status_t GxNetUrlManageInit(void)
{
	if(urlcls_ctrl->load_url_from_file)
	{
		return urlcls_ctrl->load_url_from_file();
	}
	else
	{
		gxlogd("net_url not register\n");
		return GXCORE_ERROR;
	}
}

void GxNetUrlModuleRegister(NetPmCtrl *cls)
{
	if(cls != NULL)
		urlcls_ctrl = cls;
}

NetPmCtrl  netpmctrl_server = {
	.modify_server_by_index = gx_modify_server_url_by_index,
	.modify_stream_url_by_index = gx_modify_stream_url_by_index,
	.delete_server_url_by_index = gx_delete_server_url_by_index,
	.delete_stream_url_by_index = gx_delete_stream_url_by_index,
	.add_server_url_to_file = gx_add_server_url_to_file,
	.add_stream_url_to_file = gx_add_stream_url_to_file,
	.get_server_url_by_index = gx_get_server_url_by_index,
	.get_stream_url_by_index = gx_get_stream_url_by_index,
	.load_url_from_file = gx_url_manage_init,
};

