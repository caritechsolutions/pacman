#ifndef __APP_SKIN_STR_ID_H_20231219094723__
#define __APP_SKIN_STR_ID_H_20231219094723__

//#include "app_config.h"

// 以下内容支持多语言

/*
  考虑模块的独立，此模块字符串默认不支持多语言。需要自己增加。
  可以把以下的字符串增加到 i18n.xls ， 然后用脚本执行下，再编译运行。
  需要考虑之前的字符串是否已经存在。
*/

/*
1: wnd_skin.xml 中定义的字符串

"Skin"       
"Categories"
"Preview"

2: projects/main_menu.ini  主菜单定义的菜单入口

"Skin Upgrade"

3: wnd_skpreview.xml  中定义的字符串

"Preview"
"no preview info"

*/

// 4: 代码中定义的字符串

#define SKIN_STR_ID_INSTALLED  "Installed"
#define SKIN_STR_ID_ONLINE     "Online"
#define SKIN_STR_ID_USB        "USB"

#define SKIN_STR_ID_INSTALL_POP_TITLE     "Installed skin"
#define SKIN_STR_ID_TIPS_REBOOT           "This skin is active, install it will to reboot, do you sure to install?"
#define SKIN_STR_ID_TIPS_WAIT             "Now install skin, please wait and don't power off!"
#define SKIN_STR_ID_TIPS_DOWNLOAD_FAILER  "Skin download failer!"


#define SKIN_STR_ID_ERR_ERR_PARA       "Skin failer:err parameter!"
#define SKIN_STR_ID_ERR_DMD5_PARA      "Skin failer:dmd5 parameter!"
#define SKIN_STR_ID_ERR_FILE_OPEN      "Skin failer:file open error!"
#define SKIN_STR_ID_ERR_FILE_SIZE      "Skin failer:file size error!"
#define SKIN_STR_ID_ERR_FILE_READ      "Skin failer:file read error!"
#define SKIN_STR_ID_ERR_SMD5_NOT_MATCH "Skin failer:smd5 not match!"
#define SKIN_STR_ID_ERR_DMD5_NOT_MATCH "Skin failer:dmd5 not match!"

#define SKIN_STR_ID_RECOVERY_START     "Skin update start!"
#define SKIN_STR_ID_RECOVERY_ETIP_MODE "Skin failer: mode err!"
#define SKIN_STR_ID_RECOVERY_ETIP_FILE "Skin failer: file non!"
#define SKIN_STR_ID_RECOVERY_ETIP_CRC  "Skin failer: crc err!"
#define SKIN_STR_ID_RECOVERY_ETIP_SIZE "Skin failer: size err!"

#endif
// end of file
