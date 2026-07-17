#ifndef  __FILE_MANAGE_H__
#define  __FILE_MANAGE_H__

#include <stdio.h>
#include <stdlib.h>
#include "gxcore.h"
#include "gxtype.h"
#include "gxos/gxcore_os.h"
#include "dlna.h"
#include "gxhotplug.h"

#define MAX_DEVICE (4)
#define MAX_TREE_DEPTH (6)
#define MAX_DEVICE_PATH (64)
#define MAX_DEVICE_NAME (64)
#define MAX_PATH_LEN (1024)

#define FILE_FILTER "jpg;JPG;jpeg;JPEG;bmp;BMP;png;PNG;tiff;TIFF;gif;GIF;mp3;MP3;aac;AAC;m4a;M4A;ac3;AC3;flac;ogg;mkv;MKV;avi;AVI;ts;TS;mp4;MP4;mpg;MPG;flv;FLV;vob;VOB;trp;TRP;mov;MOV;MPEG;mpeg;m2ts;M2TS"


typedef struct  _FileDirectoryNode {
	int nents;
	GxDirent *ents;
    struct _FileDirectoryNode *child;
}FileDirectoryNode,FileDirectoryTree;


typedef struct  _FileDeviceManage {
	int tree_depth;
    char device_path[MAX_DEVICE_PATH];
    char device_name[MAX_DEVICE_NAME];
    FileDirectoryTree *dir_tree;
}FileDeviceManage;


int file_manage_init(void);
int file_directory_tree_create(void);
int file_directory_tree_free(void);
int file_get_device_total(void);
FileDeviceManage *file_get_device_info(int device_index);
int file_get_directory_info_by_object_id(int user_id, const char *obj_id, FileDirectoryNode **dir_node, char *path);

MediaTypeEnum file_get_dlna_media_type(char* file);

#endif
