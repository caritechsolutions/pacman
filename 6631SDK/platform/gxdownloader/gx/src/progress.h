#ifndef __PROGRESS_H__
#define __PROGRESS_H__

#include "gxupdate_msg.h"
#include "gxota_msg.h"

typedef enum {
	UPDATE_NONE_PROGRESS_MSG,
	UPDATE_SEND_PROGRESS_MSG,
	UPDATE_SHOW_PROGRESS_MSG
} UPDATE_PROGRESS_MODE;

typedef void (*show_progress_fn)(GxUpdateMsg *);

void update_progress_init(GxDLP_Download_Type download_type, UPDATE_PROGRESS_MODE mode, show_progress_fn handler);
void update_progress_percent(int perc);
void update_progress_status(GXUPDATE_STATUS status);
void update_progress_error(GXUPDATE_ERROR_CODE error);
void update_progress_clean_msg(void);
int update_msg_to_ota_msg(GxUpdateMsg *up_msg, struct GxOTA_Msg *ota_msg);
void update_get_cur_progress(GxUpdateMsg *msg);

#endif
