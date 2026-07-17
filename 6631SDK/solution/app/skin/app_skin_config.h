#ifndef __APP_SKIN_CONFIG_H_202309201023__
#define __APP_SKIN_CONFIG_H_202309201023__

#include "app_skin_version.h"


//皮肤在 usb ,online 的配置信息文件名字
#define SKIN_XML_NAME              "skin.xml"

// 网络和usb存放的路径和xml文件名字 path version  & /xml
#define VER_MANUFACTURER          "GxSkin"  //主目录

// eg : 定义来自文件app_skin_version.h
#define VER_DRVIER_APPLICATION  "D"PLATFORM_DRIVER_VERSION"A"APP_SOLUTION_VERSION"T"APP_SKIN_VERSION

//USB&Online path : //此目录下放着很多文件夹，每个文件下放着换肤的文件
#define PATH_VER_MAIN     VER_MANUFACTURER"/"VER_DRVIER_APPLICATION

//以上使用规则，比如 bin 版本为　DxxxAxxxTxx, 则在usb&online文件夹为 Gxskin/DxxxAxxxTxx/,此目录有很多文件夹,
//每个文件夹代表一个换肤。每一个文件夹下有一个skin.xml，配置着详细的信息。skin.xml也有个版本号要和DxxxAxxxTxx匹配


// default online url
#define SKIN_ONLINE_URL_LEN      128
#define SKIN_ONLINE_URL_DEFALUT  "http://192.168.108.212:8080"

// default usb path
#define SKIN_USB_URL_LEN         128
#define SKIN_USB_URL_DEFALUT     "/mnt/usb01"


/*
以下内容都是flash存放的相关的内容
*/

//分区名，flash.conf中定义为SKIN0,如果有其它分区则依次SKIN1,SKIN2...
#define SKIN_BASE_FLASH_THEME_PARITION_NAME  "SKIN"

//存在 Flash 内的皮肤，配置信息文件名
#define THEME_IMG_XML_NAME       "skin.xml"

//gui操作的主目录名字，分区开机挂载。不要改。
#define ACTIVE_MOUNT_THEME_PATH              "/dvb"

//换肤的分区存放的xml目录
#define ACTIVE_FLASH_THEME_XML_PATH          ACTIVE_MOUNT_THEME_PATH"/theme/"THEME_IMG_XML_NAME

#define ACTIVE_FLASH_THEME_BASE_PATH         ACTIVE_MOUNT_THEME_PATH"/theme/theme.xml"

//存在 Flash 内的皮肤，预览图片路径
#define  ACTIVE_FLASH_PREVIEW_PATH           ACTIVE_MOUNT_THEME_PATH"/theme/image/skin"

//发生异常错误时，无法开机的时候，需要开机2次才进入recovery
#define ACTIVE_FLASH_THEME_ERR_NUM_OVER_RECOVERY         2

/*
存在 Flash 内的皮肤，默认只有第1个分区是安装的状态
*/
#define FLASH_THEME_INSTALLED_STATUS  (1) 


//存在 Flash 内的其它皮肤分区，临时挂载的目录，要和mkimg.sh文件匹配的
#define OTHER_MOUNT_THEME_PATH           "/dvb_temp"

//存在 Flash 内的其它皮肤分区，配置信息文件名
#define OTHER_FLASH_THEME_XML_PATH     OTHER_MOUNT_THEME_PATH"/theme/"THEME_IMG_XML_NAME
#define OTHER_FLASH_THEME_BASE_PATH    OTHER_MOUNT_THEME_PATH"/theme/theme.xml"

//存在 Flash 内的其它皮肤分区，预览图片路径
#define  OTHER_FLASH_PREVIEW_PATH       OTHER_MOUNT_THEME_PATH"/theme/image/skin"

//分区个数来自宏定义，否则通过分区表自动获取
//#define SUPPORT_USER_SKIN_PARTITION_NUM_FROM_DEFINE

#ifdef SUPPORT_USER_SKIN_PARTITION_NUM_FROM_DEFINE
//1: 分区个数来自宏定义
#define USER_SKIN_PARTITION_MAX_NUM    (1)

#endif


//开机后，按特殊按键，能进入recovery模式
#define SUPPORT_HELP_KEY

#ifdef SUPPORT_HELP_KEY

#define HELP_KEY_READ_TIMES  (150)

#define MAP_0_UP    0xbb77   //keymap public2 UP
#define MAP_1_UP    0xbf2f   //keymap public  UP

#define MAP_0_MENU  0xbbcf   //keymap public2 MENU
#define MAP_1_MENU  0xbf97   //keymap public  MENU

#define IS_FIRST_KEY(value) \
(value == MAP_0_UP) || (value == MAP_1_UP) 

#define IS_HELP_KEY(key1,key2) \
((key1 == MAP_0_UP)&&(key2 == MAP_0_MENU)) || ((key1 == MAP_1_UP)&&(key2 == MAP_1_MENU))

#endif


#endif
// end of file
