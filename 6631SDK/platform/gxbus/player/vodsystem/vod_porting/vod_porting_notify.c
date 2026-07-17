#include "gx_common.h"
#include "gx_system.h"
#include "gxplayer_module.h"
#include "../vod_include/vod_common_def.h"

PlayerEventStatus status;
PLAYER_EVENT_REPORT cbk_report = 0;
int vod_porting_event_notify(int eventid, int errorcode)
{
	GxVod_debug("********************* notify %d  %d ********************\n", eventid, errorcode);
	switch(eventid)
	{
		case EVENT_END_OF_FILE:
		case EVENT_PROGRAM_END:
			status.status = PLAYER_STATUS_PLAY_END;
			status.error  = PLAYER_ERROR_NO_ERROR;
			break;
		case EVENT_RETURN_CODE_ERROR:
		case EVENT_SERVER_CLOSED:
			status.status = PLAYER_STATUS_SERVER_ERROR;
			status.error  = errorcode;
			break;
		default:
			return -1;
	}
	if(cbk_report)
		cbk_report(PLAYER_EVENT_STATUS_REPORT, &status);
	return 0;
}

void vod_porting_event_seturl(const char* player_name, const char* src_url, void* cb)
{
	if(player_name==NULL ||
			strlen(player_name) >= PLAYER_NAME_LONG){
		memset(&status, 0 ,sizeof(PlayerEventStatus));
		cbk_report = 0;
		return;
	}

	if(src_url==NULL ||
			strlen(src_url) >= PLAYER_URL_LONG){
		memset(&status, 0 ,sizeof(PlayerEventStatus));
		cbk_report = 0;
		return;
	}

	memset(&status, 0 ,sizeof(PlayerEventStatus));
	memcpy(status.player, (char*)player_name, PLAYER_NAME_LONG);
	memcpy(status.url, (char*)src_url, PLAYER_URL_LONG);
	cbk_report = cb;
	return;
}
