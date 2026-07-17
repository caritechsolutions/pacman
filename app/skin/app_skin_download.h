#ifndef __APP_SKIN_DOWNLOAD_H_202310251030__
#define __APP_SKIN_DOWNLOAD_H_202310251030__


// MAX_PAGE_ITEM  & MAX_PREVIEW_NODE_NUM 取一个最大的数据
#define SKIN_DOWNLOAD_PIC_MAX_NUM (6)

#define PIC_PATH_SUFFIX             ".bmp"

#ifdef ECOS_OS
#define DIR_DOWN_PATH             "/mnt/skin_dir.html"
#define XML_DOWN_PATH             "/mnt/skin.xml"
#define PIC_DOWN_PATH             "/mnt/skin_icon_"
#define IMG_DOWN_PATH             "/mnt/skin_update.img"
#else
#define DIR_DOWN_PATH             "/tmp/skin_dir.html"
#define XML_DOWN_PATH             "/tmp/skin.xml"
#define PIC_DOWN_PATH             "/tmp/skin_icon_"
#define IMG_DOWN_PATH             "/tmp/skin_update.img"
#endif

typedef int (*skin_gxapp_timer_callback)(int,unsigned int);

typedef int (*skin_gxapp_timer_multi_callback)(int,char *);

///////////////

int   gxapp_skin_download_delete_img_file(void);

//// xml, img

int gxapp_skin_download_api_download_start(void);
int gxapp_skin_download_api_download_stop(void);


int gxapp_skin_download_api_callback_stop(void);


int gxapp_skin_download_api_timer_start(skin_gxapp_timer_callback callback, int time);
int gxapp_skin_download_api_timer_stop(void);


int gxapp_skin_download_api_async_download(const char *url,char *path);

int gxapp_skin_download_api_async_download_ext(const char *url,char *buffer,unsigned int size);
////pic

int gxapp_skin_download_api_multi_timer_start(skin_gxapp_timer_multi_callback callback, int time);
int gxapp_skin_download_api_multi_timer_stop(void);

int gxapp_skin_download_api_multi_download_start(void);
int gxapp_skin_download_api_multi_download_stop(void);

int gxapp_skin_download_api_multi_async_download(const char *url,int index);

int gxapp_skin_download_api_multi_is_download_finish(int index);


#endif
