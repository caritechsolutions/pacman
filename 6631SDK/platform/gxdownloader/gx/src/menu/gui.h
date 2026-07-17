#ifndef __GXOTA_GUI_H__
#define __GXOTA_GUI_H__

#include "bus/gui_core/mini_gui.h"
#include "gxota_msg.h"

#define GXOTA_TS_DEMOD_TYPE                  (0)
#define GXOTA_TS_TUNER_TYPE                  (1)
#define GXOTA_TS_FRE                         (2)
#define GXOTA_TS_SYM                         (3)
#define GXOTA_TS_PID                         (4)
#define GXOTA_TS_TABLEID                     (5)
#define GXOTA_TS_OK_START                    (6)
#define GXOTA_TS_UP_DOWN                     (7)
#define GXOTA_TS_LEFT_RIGHT                  (8)

#define GXOTA_USB_FILE_NAME                  (0)
#define GXOTA_USB_OK_START                   (1)
#define GXOTA_USB_LEFT_RIGHT                 (2)
/*希腊字母和科普特字母编码区单个字符*/
#define GXOTA_TXT_CH_BU                 "Α"            //"不"
#define GXOTA_TXT_CH_ZHONG              "Β"            //"中"
#define GXOTA_TXT_CH_GUAN               "Γ"            //"关"
#define GXOTA_TXT_CH_CHU                "Δ"            //"初"
#define GXOTA_TXT_CH_QIAN               "Ε"            //"前"
#define GXOTA_TXT_CH_GONG               "Ζ"            //"功"
#define GXOTA_TXT_CH_HUA                "Η"            //"化"
#define GXOTA_TXT_CH_SHENG              "Θ"            //"升"
#define GXOTA_TXT_CH_HOU                "Ι"            //"后"
#define GXOTA_TXT_CH_ZAI                "Κ"            //"在"
#define GXOTA_TXT_CH_FU                 "Λ"            //"复"
#define GXOTA_TXT_CH_SHI_1              "Μ"            //"失"
#define GXOTA_TXT_CH_SHI_2              "Ν"            //"始"
#define GXOTA_TXT_CH_WAN                "Ξ"            //"完"
#define GXOTA_TXT_CH_DING               "Ο"            //"定"
#define GXOTA_TXT_CH_DU                 "Π"            //"度"
#define GXOTA_TXT_CH_KAI                "Ρ"            //"开"
#define GXOTA_TXT_CH_ZONG               "Σ"            //"总"
#define GXOTA_TXT_CH_CHENG              "Τ"            //"成"
#define GXOTA_TXT_CH_DA                 "Υ"            //"打"
#define GXOTA_TXT_CH_JU                 "Φ"            //"据"
#define GXOTA_TXT_CH_JIE_1              "Χ"            //"接"
#define GXOTA_TXT_CH_SHOU               "Ψ"            //"收"
#define GXOTA_TXT_CH_SHU                "Ω"            //"数"
#define GXOTA_TXT_CH_DUAN               "α"            //"断"
#define GXOTA_TXT_CH_XIN                "β"            //"新"
#define GXOTA_TXT_CH_GENG               "γ"            //"更"
#define GXOTA_TXT_CH_JI_1               "δ"            //"机"
#define GXOTA_TXT_CH_ZHENG              "ε"            //"正"
#define GXOTA_TXT_CH_YONG               "ζ"            //"用"
#define GXOTA_TXT_CH_DIAN               "η"            //"电"
#define GXOTA_TXT_CH_KONG               "θ"            //"空"
#define GXOTA_TXT_CH_XI                 "ι"            //"系"
#define GXOTA_TXT_CH_JI_2               "κ"            //"级"
#define GXOTA_TXT_CH_TONG               "λ"            //"统"
#define GXOTA_TXT_CH_ZHI                "μ"            //"置"
#define GXOTA_TXT_CH_YAO                "ν"            //"要"
#define GXOTA_TXT_CH_JIE_2              "ξ"            //"解"
#define GXOTA_TXT_CH_SHE                "ο"            //"设"
#define GXOTA_TXT_CH_SHI_3              "π"            //"试"
#define GXOTA_TXT_CH_QING               "ρ"            //"请"
#define GXOTA_TXT_CH_BAI                "σ"            //"败"
#define GXOTA_TXT_CH_JIN                "τ"            //"进"
#define GXOTA_TXT_CH_CHONG              "υ"            //"重"
#define GXOTA_TXT_CH_SUO                "φ"            //"锁#define GXOTA_TXT_CH_SUO                  "φ"            //"锁"
#define GXOTA_TXT_CH_CUO                "χ"            //"错"
#define GXOTA_TXT_CH_WU                 "$"            //"误"
#define GXOTA_TXT_CH_DANG               "ψ"            //"当"
#define GXOTA_TXT_CH_ZHUANG             "ω"            //"状"
#define GXOTA_TXT_CH_TAI                "!"            //"态"
#define GXOTA_TXT_CH_DUAN_2             "#"            //"端"
#define GXOTA_TXT_CH_BAO                "&"            //"包"
#define GXOTA_TXT_CH_GE                 "*"            //"个"

#define GXOTA_TXT_CH_TIAO               "¡"            //"调"
#define GXOTA_TXT_CH_LEI                "¤"            //"类"
#define GXOTA_TXT_CH_XING               "§"            //"型"
#define GXOTA_TXT_CH_XIE                "¨"            //"谐"
#define GXOTA_TXT_CH_PIN                "¼"            //"频"
#define GXOTA_TXT_CH_LV                 "½"            //"率"
#define GXOTA_TXT_CH_FU1                "®"            //"符"
#define GXOTA_TXT_CH_HAO                "°"            //"号"
#define GXOTA_TXT_CH_QUE                "±"            //"确"
#define GXOTA_TXT_CH_ZUO                "²"            //"左"
#define GXOTA_TXT_CH_YOU                "³"            //"右"
#define GXOTA_TXT_CH_SHANG              "´"            //"上"
#define GXOTA_TXT_CH_XIA                "¶"            //"下"
#define GXOTA_TXT_CH_WEN                "·"            //"文"
#define GXOTA_TXT_CH_JIAN               "¸"            //"件"
#define GXOTA_TXT_CH_MING               "¹"            //"名"
/*chinese */
#define STR_CN_GXOTA_STATUS_INIT                "ΘκιλεΚΔΝΗ"              //("升级系统正在初始化")
#define STR_CN_GXOTA_STATUS_FRONTEND_ERROR      "ΥΡιλΕ#Μσ"               //("打开系统前端失败")
#define STR_CN_GXOTA_STATUS_FRONTEND_SETUP      "ομιλΕ#"                 //("设置系统前端")
#define STR_CN_GXOTA_STATUS_FRONTEND_UNLOCK     "Μφ"                     //("失锁")
#define STR_CN_GXOTA_STATUS_FRONTEND_LOCK       "φΟ"                     //("锁定")
#define STR_CN_GXOTA_STATUS_DMX_ERROR           "ΥΡιλξΛζΜσ"              //("打开系统解复用失败")
#define STR_CN_GXOTA_STATUS_DMX_START_FILTER    "ΡΝΧΨΩΦ"                 //("开始接收数据")
#define STR_CN_GXOTA_STATUS_DMX_DATA_FINISH     "ΩΦΧΨΞΤ"                 //("数据接收完成")
#define STR_CN_GXOTA_STATUS_WRITE_FLASH         "εΚγβΩΦ,ρΑναη"           //("正在更新数据，请不要断电")
#define STR_CN_GXOTA_STATUS_WRITE_FLASH_OK      "γβΩΦΤΖ"                 //("更新数据成功")
#define STR_CN_GXOTA_STATUS_WRITE_FLASH_FAILED  "γβΩΦΜσ"                 //("更新数据失败")
#define STR_CN_GXOTA_STATUS_UPDATE_FAILED       "Θκχ$,ρΡΓδΙυπ"           //("升级错误,请开关机后重试")
#define STR_CN_GXOTA_STATUS                     "ψΕω!:"                  //("当前状态")
#define STR_CN_GXOTA_SECTION_NUM                "&Σ*Ω"                   //("包总个数")
#define STR_CN_GXOTA_PROCESS                    "ΣτΠ:"                   //("总进度:")
#define STR_CN_GXOTA_OTA                        "θΒΘκ"                   //("空中升级")

#define STR_CN_GXOTA_DEMOD_TYPE                 "ξ¡¤§"                   //("解调类型")
#define STR_CN_GXOTA_TUNER_TYPE                 "ξ¨¤§"                   //("解谐类型")
#define STR_CN_GXOTA_FRE                        "¼½"                     //("频率")
#define STR_CN_GXOTA_SYM                        "®°½"                    //("符号率")
#define STR_CN_GXOTA_PID                        "PID"
#define STR_CN_GXOTA_TABLEID                    "Table ID"
#define STR_CN_GXOTA_OK_START                   "±Ο: ΡΝ"                 //("确定: 开始")
#define STR_CN_GXOTA_UP_DOWN                    "´/¶"                    //("上/下")
#define STR_CN_GXOTA_LEFT_RIGHT                 "²/³"                    //("左/右")
#define STR_CN_GXOTA_FILE_NAME                  "·¸¹"                    //("文件名")

#define STR_CN_GXOTA_FILE                      "c>"
#define STR_CN_GXOTA_IP                        "IP"
/*english*/
#define STR_EN_GXOTA_STATUS_INIT                ("The OTA system initailizing")
#define STR_EN_GXOTA_STATUS_FRONTEND_ERROR      ("Open frontend failed")
#define STR_EN_GXOTA_STATUS_FRONTEND_SETUP      ("Setup frontend")
#define STR_EN_GXOTA_STATUS_FRONTEND_UNLOCK     ("Unlocked")
#define STR_EN_GXOTA_STATUS_FRONTEND_LOCK       ("Locked")
#define STR_EN_GXOTA_STATUS_DMX_ERROR           ("Open demux failed")
#define STR_EN_GXOTA_STATUS_DMX_START_FILTER    ("Receiving data, please wait...")
#define STR_EN_GXOTA_STATUS_DMX_DATA_FINISH     ("Data receive finish")
#define STR_EN_GXOTA_STATUS_WRITE_FLASH         ("Waiting...Please don't power off while upgrading!")
#define STR_EN_GXOTA_STATUS_WRITE_FLASH_OK      ("OTA upgrade successed")
#define STR_EN_GXOTA_STATUS_WRITE_FLASH_FAILED  ("OTA upgrade failed")
#define STR_EN_GXOTA_STATUS_UPDATE_FAILED       ("OTA upgrade failed, pls retry it")
#define STR_EN_GXOTA_SECTION_NUM                ("Section count")   //包总个数
#define STR_EN_GXOTA_STATUS                     ("Status:")         //当前状态

#define STR_EN_GXOTA_DEMOD_TYPE                 "Demod Type"
#define STR_EN_GXOTA_TUNER_TYPE                 "Tuner Type"
#define STR_EN_GXOTA_FRE                        "Frequency"
#define STR_EN_GXOTA_SYM                        "Symbolrate"
#define STR_EN_GXOTA_PID                        "PID"
#define STR_EN_GXOTA_TABLEID                    "Table ID"
#define STR_EN_GXOTA_OK_START                   "ok: start"
#define STR_EN_GXOTA_UP_DOWN                    "up/down"
#define STR_EN_GXOTA_LEFT_RIGHT                 "left/right"
#define STR_EN_GXOTA_FILE_NAME                  "File Name"

#define STR_EN_GXOTA_FILE                      "c>"
#define STR_EN_GXOTA_IP                        "IP"

void showlogo(char *filename);
void showmenu(int8_t upgrade_type);
void hidemenu(void);
void gui_show(int processid, const struct GxOTA_Msg* msg, char *rc_string);
int guiinit(unsigned int chip_definition);
int gdiinit(unsigned int chip_definition);

#endif

