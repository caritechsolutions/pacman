#ifndef _GX_SUBTTRANS_H
#define _GX_SUBTTRANS_H

#include "gx_subttrans_service.h"

//===Support trans lang =======>
#define ST_LANG_ENG     "Englist"
#define ST_LANG_ARA     "Arabic"
#define ST_LANG_FRE     "French"
//==========ios639=============
#define ST_LANG639_ENG  "en"
#define ST_LANG639_ARA  "ar"
#define ST_LANG639_FRE  "fr"
#define ST_LANG639_ZH   "zh"
#define ST_LANG639_POL  "pl"
#define ST_LANG639_DEU  "de"
//<============================
#define SUBT_LANG_MAX (3)

#define ST_TRANS_ENABLE_KEY  "subttrans>enable"
#define ST_LANG_SEL_KEY      "subttrans>lang"
#define ST_FONTSIZE_SEL_KEY  "subttrans>font_size"
#define ST_FONTCOLOR_SEL_KEY "subttrans>font_color"
#define ST_BAGKCOLOR_SEL_KEY "subttrans>bakg_color"
#define ST_CODE_KEY          "subttrans>code"
#define ST_DELAY_KEY        "subttrans>delay"
#define ST_LANG_SEL 0
#define ST_FONTSIZE_SEL 0
#define ST_FONTCOLOR_SEL 0
#define ST_BAGKCOLOR_SEL 0
#define ST_CODE_DEFAULT "000000000000"
#define ST_DELAY_DEFAULT 0
#define ST_TRANS_ENABLE_DEFAULT  (0)
#define ST_REQUEST_DEFAULT (1000)

#define ST_LOGIN_OEM_KEY   "subttrans>oem"     // 默认工厂序列
#define ST_LOGIN_BRAND_KEY "subttrans>brand"   // 默认工厂项目序列
#define ST_LOGIN_MAC_KEY   "subttrans>mac"     //机顶盒Mac信息
#define ST_LOGIN_MOD_KEY   "subttrans>model"     //机顶盒model信息
#define ST_LOGIN_DEF_OEM   "gx" // Only for gx Company Use
#define ST_LOGIN_DEF_BRAND "gxbrand"
#define ST_LOGIN_DEF_MAC   "151692330489" //六组十六进制数字, 不带分隔符
#define ST_LOGIN_DEF_MODEL "GX-MODEL"

//#define SUBTTRANS_API_DEBUG
#ifdef  ST_API_DEBUG
#define ST_API_PRINTF  printf
#else
#define ST_API_PRINTF(...) do{}while(0)
#endif

typedef enum _STControlType
{
    STMSG_DVB_SUBT_GET = 0,
    STMSG_DVB_SUBT_DRAW,
    STMSG_DVB_SUBT_ERR
}STControlType_e;

typedef struct _AppMsg_SubtTransControl {
    STControlType_e type;
    uint8_t  subt_type;
    uint16_t service_id;
    //uint16_t request_id;
    uint8_t  *data;
    uint32_t  data_len;
    char     lang[SUBT_LANG_MAX];
} AppMsg_SubtTransControl;

// 绘制字幕数据结构
typedef struct _st_drawinfo
{
    char *subt_data;
    double pts;
}STDrawInfo_t;

//初始化字幕翻译整体模块：
void Gx_SubtTransInit(void);

//停止字幕翻译整体模块：
void Gx_SubtTransDestory(void);

/* 字幕服务器对每台STB独特的注册信息
 * 传入参数：oem: 工厂自定义序列
 *         ：brand: 对应项目自定义序列
 *         ：mac: STB的唯一Mac地址, 不可重复 */
void Gx_SubtTrans_Set_CustomInfo(char *oem, char *brand, char *mac);

//使能字幕翻译功能
void Gx_SubtTransStart(void);
//停止字幕翻译功能
void Gx_SubtTransStop(void);

//获得字幕翻译开启状态
//返回参数：enable=1,disable=0
int Gx_SubtTransGetStatus(void);
void Gx_SubtTrans_SetSubtPid(int pid, char *lang, uint16_t service_id, uint8_t subt_type);
int Gx_SubtTrans_ChannelSwitch(void);
int Gx_SubtTrans_GetDelayms(void);
status_t Gx_SubtTrans_Replay_ByTsbuff(void);

/********** SUBTITLE TRANSLATION SubtDraw State ************/
//字幕绘制清除
void Gx_SubtTrans_DrawClean(void);
//字幕绘制暂停显示
void Gx_SubtTrans_DrawPause(void);
//字幕绘制暂停恢复
void Gx_SubtTrans_DrawResume(void);
/************ SUBTITLE TRANSLATION SubtDraw **************/


/********** SUBTITLE TRANSLATION SubtShow DEFINE ************/
char **Gx_SubtTrans_SubtLangGet(int *num);
char **Gx_SubtTrans_SubtFontGet(int *num);
char **Gx_SubtTrans_SubtFontColorGet(int *num);
char **Gx_SubtTrans_SubtBackgroudColorGet(int *num);
//传入参数： font_size: 显示翻译后字幕语言的字体大小
//         fontcolor: 显示翻译后字幕语言的前景色
//         bgcolor:   显示翻译后字幕语言的背景色
void Gx_SubtTrans_SubtShowSet(int font_size, int fontcolor, int bgcolor);
/************* SUBTITLE TRANSLATION SubtShow ***************/

#endif
