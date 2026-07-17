#ifndef __APP_SKIN_XML_H_202309251334__
#define __APP_SKIN_XML_H_202309251334__


#define SKIN_DIR_BEGIN  "href=\""


//系统最大支持的分区表
#define ALL_SKIN_PARTITION_MAX_NUM     (4)  //需要xml中保存flash中的所有分区的uuid

#define SKIN_MAX_LIST 15      //显示最大支持的皮肤个数

#define FLASH_PREVIEW_NODE_NUM  2 //flash内部预览的图片最大个数2，不要超过下面。
#define MAX_PREVIEW_NODE_NUM  5   //最大支持的预览个数，这个数不要改，因为和页面的设计有关。


typedef struct skin_preview_node_s{
    char *pre_url ;
}skin_preview_node_t;

typedef struct skin_xml_node_s{
	char *ddir;      //路径
    char *name;      //icon name
	char *icon_url;
	unsigned int   uiid ;   //唯一的编号，如果安装的菜单是相同的uiid，则不能安装。 
	unsigned int   uninstall ;
	char *simg_url;
	char *simg_md5;//enc md5
	char *dimg_md5;//dec md5
	skin_preview_node_t preview[MAX_PREVIEW_NODE_NUM];
}skin_item_node_t;

typedef struct skin_data_node_s{
	int num      ;
	int dir_all_num  ; //路径数
	int cur_down_num ; //依次下载路径目录下的*xml的索引
	skin_item_node_t *node ;
}skin_data_node_t;

int gxapp_buffer_get_data(char *buffer, unsigned int size,skin_data_node_t *pdata);

int gxapp_file_get_data(char *path,skin_item_node_t *item_data);

int gxapp_file_get_simple_data(char *path,int i,skin_item_node_t *item_data);

int gxapp_free_data(skin_data_node_t *pdata);

int gxapp_set_preview_info(int mode,unsigned int uiid, skin_preview_node_t *pPreview);

int gxapp_xml_module_uninit(void);

char *gxapp_get_xml_parse_server(void);

int gxapp_htmlfile_get_data(char *path,skin_data_node_t *pdata);

int gxapp_set_installed_uiid(int i, unsigned int uiid);

int gxapp_exist_installed_uiid(unsigned int cur_uuid);


#endif
