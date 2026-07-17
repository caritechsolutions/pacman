#ifndef _RTSP_API_H_
#define _RTSP_API_H_

/*****************************************************************
	rtsp_api.c
	2008-11-12
******************************************************************/
#include"vod_in_common_def.h"
#include "vod_in_typedef.h"

typedef enum{
	RTSP_CMD_NONE = 0,
	RTSP_CMD_RESPONSE,
	RTSP_CMD_SETPARAM_BOS,
	RTSP_CMD_SETPARAM_EOS,
	RTSP_CMD_SETPARAM_CLOSE,
	RTSP_CMD_GETPARAM,
	RTSP_CMD_OPTIONTIME,
	RTSP_CMD_ANNOUNCE,
	RTSP_EVT_SOCK_ERR,

	RTSP_CMD_END
}RTSP_CMD_t;
/*************************************************************
**rtsp_session_init
**
*************************************************************/
S32 rtsp_session_init ( const U8 *url, S32 reagionid, S32 range_type,S32 wms );
void rtsp_session_release (void );
S32 rtsp_play_all_media ( const char* range, S32 prebuf_s, S32 scale,S32 wms);
S32 rtsp_stop_all_media ( void );
S32 rtsp_wait_server_response ( int threw_data, int wms );
S32 rtsp_cmd_play(char *range, int scale, int speed,int prebuf_s,int wms,int wait );
S32 rtsp_cmd_pause(int wms,int wait );
S32 rtsp_cmd_options(S32 wms ,S32 wait);
S32 rtsp_cmd_get_parameter( S32 wms ,S32 wait );
S32  rtsp_loop_media_data (int wms);
RTSP_CMD_t rtsp_loop_server_command( int wms );
S32 rtsp_get_trans_type( void );
U32 rtsp_get_range_type(void);
U32 rtsp_get_start_range(void);
U32 rtsp_get_current_range(void);
U32 rtsp_get_end_range(void);
U32 rtsp_get_start_duration( void );
U32 rtsp_get_end_duration( void );

//S8 * rtsp_get_vid_fmt_param( void );
S8* rtsp_get_vid_config_ascii( void );
U32 rtsp_get_vid_clock_rate( void );

//S8* rtsp_get_aud_fmt_param( void );
U32 rtsp_get_aud_clock_rate( void );
S32 rtsp_get_aud_size_length( void );
S32 rtsp_get_aud_index_length( void );
S32 rtsp_get_aud_index_delta_length( void );
S8* rtsp_get_aud_config_ascii( void );



#endif

