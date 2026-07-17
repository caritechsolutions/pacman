#include "gxtype.h"
#include "gxcore.h"
#include "file_manage.h"
#include "module/app_log.h"

struct ThisCtrl {
	int device_total;
    FileDeviceManage dev_manage[MAX_DEVICE];
};

struct ThisCtrl this = {0};

int file_get_device_total(void)
{
    return this.device_total;
}

FileDeviceManage *file_get_device_info(int device_index)
{
    if(device_index > this.device_total)
        return NULL;

    return &(this.dev_manage[device_index]);
}

int file_manage_init(void)
{
    int i = 0;
    size_t len = 0, cp_size = 0;
    HotplugPartitionList *partition_list = NULL;

    partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);

    memset(&this, 0, sizeof(struct ThisCtrl));
    this.device_total = partition_list->partition_num;

    if(this.device_total > MAX_DEVICE)
        this.device_total = MAX_DEVICE;

    for(i = 0; i < this.device_total; i++)
    {
        len = strlen(partition_list->partition[i].partition_entry);
        cp_size = (len > MAX_DEVICE_PATH) ? MAX_DEVICE_PATH: len;
        memcpy(this.dev_manage[i].device_path, partition_list->partition[i].partition_entry, cp_size);
        len = strlen(partition_list->partition[i].partition_name);
        cp_size = (len > MAX_DEVICE_NAME) ? MAX_DEVICE_NAME: len;
        memcpy(this.dev_manage[i].device_name, partition_list->partition[i].partition_name, cp_size);
    }

    return this.device_total;
}

static int _path_get_depth(char* path)
{
    int depth = 0;

    while('\0' != *(path++))
    {
        if('/' == *path)
            depth++;
    }

    return depth;
}

static int _tree_create(FileDirectoryTree **tree, char *path, int *max_depth)
{
    int i = 0;
    int ret = -1;
    int depth = 0;
	char *tmp_path = NULL;
    FileDirectoryNode *dir_node = NULL;

    depth = _path_get_depth(path);

    dir_node = (FileDirectoryNode *) GxCore_Calloc(1, sizeof(FileDirectoryNode));
    if(NULL == dir_node)
    {
        app_log_error("%s,%d malloc error\n", __func__, __LINE__);
        return -1;
    }
    tmp_path = (char *) GxCore_Calloc(MAX_PATH_LEN, sizeof(char));
    if(NULL == tmp_path)
    {
        app_log_error("%s,%d malloc error\n", __func__, __LINE__);
        return -1;
    }

    dir_node->nents = GxCore_GetDir(path, &(dir_node->ents), FILE_FILTER);
	GxCore_SortDir(dir_node->ents, dir_node->nents, NULL);
    if(dir_node->nents > 0)
    {
        dir_node->child = (FileDirectoryTree *)GxCore_Calloc(dir_node->nents, sizeof(FileDirectoryTree));
    }
    for(i = 0; i < dir_node->nents; i++)
    {
        GxDirent* ent = &(dir_node->ents[i]);
        if(GX_FILE_DIRECTORY == ent->ftype && '.' != ent->fname[0])
        {
            if(depth > MAX_TREE_DEPTH)
            {
           //     app_log_debug("%s,%d depth over  MAX_TREE_DEPTH(%d)\n", __func__, __LINE__, MAX_TREE_DEPTH);
                continue;
            }
            snprintf(tmp_path, MAX_PATH_LEN, "%s/%s",path, ent->fname);
            FileDirectoryTree *child_tree = NULL;
            ret = _tree_create(&child_tree, tmp_path, max_depth);
            dir_node->child[i] = *child_tree;
            if(ret < 0)
            {
                GxCore_Free(tmp_path);
                return -1;
            }
        }
    }

    if(*max_depth < depth)
        *max_depth = depth;

    *tree = dir_node;

    GxCore_Free(tmp_path);
    return 0;
}

static void _tree_free(FileDirectoryTree **tree)
{
    int i = 0;
    FileDirectoryNode *dir_node = *tree;

    for(i = 0; i < dir_node->nents; i++)
    {
        GxDirent* ent = &(dir_node->ents[i]);
        if(GX_FILE_DIRECTORY == ent->ftype)
        {
            FileDirectoryTree *child_tree = &(dir_node->child[i]);
            _tree_free(&child_tree);
        }
    }
    if(NULL != dir_node->child)
    {
        GxCore_Free(dir_node->child);
        dir_node->child = NULL;
    }
	GxCore_FreeDir(dir_node->ents, dir_node->nents);
    *tree = NULL;
}

int file_directory_tree_create(void)
{
    int i = 0;
    int ret = -1;

    for(i = 0; i < this.device_total; i++)
    {
      ret = _tree_create(&(this.dev_manage[i].dir_tree), this.dev_manage[i].device_path, &(this.dev_manage[i].tree_depth));
      if(ret < 0)
        return -1;
    }
    return 0;
}

int file_directory_tree_free(void)
{
    int i = 0;

    for(i = 0; i < this.device_total; i++)
    {
        _tree_free(&(this.dev_manage[i].dir_tree));
    }
    return 0;
}

static int _object_get_device_id(const char* obj_id)
{
    char obj_str[MAX_ID_STR_LEN];
    char *obj = obj_str;

    memcpy(obj_str, obj_id, MAX_ID_STR_LEN);
    while('\0' != *(obj++))
    {
        if('$' == *obj)
        {
            *obj = '\0';
            return atoi(obj_str);
        }
    }

    return atoi(obj_id);
}

static int _object_get_cur_depth(const char* obj_id)
{
    int depth = 1;
    char *obj_str = (char *)obj_id;

    while('\0' != *(obj_str++))
    {
        if('$' == *obj_str)
            depth++;
    }

    return depth;
}

int file_get_directory_info_by_object_id(int user_id, const char *obj_id, FileDirectoryNode **dir_node, char *path)
{
    int depth = 0;
    int device_index = 0, index = 0;
    char *id_str = NULL, *save_tmp = NULL, *path_ptr = NULL;
    char obj_str[MAX_PATH_LEN];
    FileDirectoryNode *node = NULL;

    depth = _object_get_cur_depth(obj_id);
    device_index = _object_get_device_id(obj_id) - user_id;

    if(device_index < 0 || device_index >= MAX_DEVICE || this.dev_manage[device_index].tree_depth < depth)
    {
        app_log_error("%s,%d client data not right\n", __func__, __LINE__);
        return -1;
    }

    path_ptr = this.dev_manage[device_index].device_path;
    while('/' != *(++path_ptr));
    snprintf(path, MAX_PATH_LEN, "%s", path_ptr);
    node = this.dev_manage[device_index].dir_tree;

    memcpy(obj_str, obj_id, MAX_ID_STR_LEN);
    id_str = strtok_r(obj_str, "$", &save_tmp);
    if(NULL != id_str)
        id_str = strtok_r(NULL, "$", &save_tmp);
    while(NULL != id_str)
    {
        index = atoi(id_str) - 1;
        if(index >= node->nents)
        {
            app_log_error("%s,%d id error\n", __func__, __LINE__);
            return -1;
        }
        snprintf(path+strlen(path), MAX_PATH_LEN - strlen(path), "/%s", node->ents[index].fname);
        node = &(node->child[index]);

        id_str = strtok_r(NULL, "$", &save_tmp);
    }

    *dir_node = node;
    return 0;
}

static char* file_get_file_suffix(char* file)
{
	char* file_name = NULL;
	char* file_suffix = NULL;
	int len = 0;
	int i = 0;

	if(NULL == file) return NULL;

	file_name = file;
	len = strlen(file_name);
	if(3 > len) return NULL;

	for(i = len -2; i >0; i--)
	{
		if('.' == file_name[i])
			break;
	}

	if(0 == i) return NULL;

	file_suffix = &file_name[i+1];

	return file_suffix;
}


MediaTypeEnum file_get_dlna_media_type(char* file)
{
	char* file_suffix = NULL;

	if(NULL == file) return DLNA_MEDIA_NONE;

	file_suffix = file_get_file_suffix(file);
	if(NULL == file_suffix) return DLNA_MEDIA_NONE;

	if(0 == strcasecmp(file_suffix, "bmp")
		|| 0 == strcasecmp(file_suffix, "jpg")
		|| 0 == strcasecmp(file_suffix, "png")
		|| 0 == strcasecmp(file_suffix, "jpeg"))
	{
		return DLNA_IMAGE;
	}

	else if(0 == strcasecmp(file_suffix, "mp3")
		|| 0 == strcasecmp(file_suffix, "aac")
		|| 0 == strcasecmp(file_suffix, "m4a")
		|| 0 == strcasecmp(file_suffix, "ac3"))
	{
		return DLNA_AUDIO;
	}

	else if(0 == strcasecmp(file_suffix, "mkv")
		|| 0 == strcasecmp(file_suffix, "avi")
		|| 0 == strcasecmp(file_suffix, "ts")
		|| 0 == strcasecmp(file_suffix, "mp4")
		|| 0 == strcasecmp(file_suffix, "mpg")
		|| 0 == strcasecmp(file_suffix, "flv")
		|| 0 == strcasecmp(file_suffix, "dat")
		|| 0 == strcasecmp(file_suffix, "vob")
		|| 0 == strcasecmp(file_suffix, "tp")
		|| 0 == strcasecmp(file_suffix, "trp")
		|| 0 == strcasecmp(file_suffix, "dvr")
		|| 0 == strcasecmp(file_suffix, "mov")
		|| 0 == strcasecmp(file_suffix, "mpeg")
		|| 0 == strcasecmp(file_suffix, "m2ts"))
	{
		return DLNA_VIDEO;
	}

	return DLNA_MEDIA_NONE;
}

