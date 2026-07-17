#ifndef __APP_CEC_LINK_H__
#define __APP_CEC_LINK_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "app_config.h"
#if HDMI_CEC_SUPPORT
#include <gxtype.h>
#include "app_cec_private.h"

typedef enum _CecUIControlTypeEnum {
    CEC_UI_SHOW_MESSAGE,
    CEC_UI_LINK_OPCODE_HANDLER
}CecUIControlTypeEnum;

typedef struct _AppMsg_CecUIControl{
    CecUIControlTypeEnum type;
	CEC_OPCODE_HANDLER opcode_handler;
}AppMsg_CecUIControl;

extern int app_cec_link_ui_msg_proc(GxMessage *msg);
extern E_CEC_MENU_STATE app_cec_link_get_current_menu_activate_status(void);
extern int app_cec_link_opcode_handler(P_CEC_OPCODE_HANDLER opcode_handler);
extern int app_send_cec_link_opcode_handler_msg(P_CEC_OPCODE_HANDLER opcode_handler);
extern int app_cec_link_show_osd_message(P_CEC_OPCODE_HANDLER opcode_handler);
extern char *app_cec_convert_menu_lang_str_to_iso639_lang_str(void);

#ifdef __cplusplus
 }
#endif
#endif
#endif
