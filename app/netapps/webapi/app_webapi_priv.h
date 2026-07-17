#ifndef __APP_WEBAPI_PRIV_H__
#define __APP_WEBAPI_PRIV_H__
#include <mongoose.h>
#include <cJSON.h>

//#define WEBAPI_DEBUG
#ifdef WEBAPI_DEBUG
#define WEBAPI_PRINTF  app_log_debug
#else
#define WEBAPI_PRINTF(...)	do{}while(0)
#endif

#define WEBAPI_PORT "8000"
#define WEBAPI_SERVER_HEADER "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
#define WEBAPI_VERSION 1
#define WEBAPI_DEFAULT_PROGRAM_LIMIT 20
#define WEBAPI_STATUS_STR "status"
#define WEBAPI_OK_STR "ok"
#define WEBAPI_FAIL_STR "fail"
#define WEBAPI_ERRMSG_STR "err_msg"
#define WEBAPI_ERRCODE_STR "err_code"
#define WEBAPI_VOLUME_STR "volume"
#define WEBAPI_CURRENT_STR "current"
#define WEBAPI_MAX_STR "max"
#define WEBAPI_TIME_STR "time"
#define WEBAPI_TIMEZONE_STR "timezone"
#define WEBAPI_SUMMERTIME_STR "summer_time"
#define WEBAPI_YEAR_STR "year"
#define WEBAPI_MONTH_STR "month"
#define WEBAPI_DAY_STR "day"
#define WEBAPI_HOUR_STR "hour"
#define WEBAPI_MIN_STR "min"
#define WEBAPI_SEC_STR "sec"
#define WEBAPI_LANGUAGE_STR "language"
#define WEBAPI_MENU_STR "menu"
#define WEBAPI_AUDIO1_STR "audio1"
#define WEBAPI_AUDIO2_STR "audio2"
#define WEBAPI_SUBTITLE_STR "subtitle"
#define WEBAPI_TTX_STR "ttx"
#define WEBAPI_EPG_STR "epg"
#define WEBAPI_OPTIONS_STR "options"
#define WEBAPI_AV_STR "av"
#define WEBAPI_STANDARD_STR "tv_standard"
#define WEBAPI_RATIO_STR "tv_ratio"
#define WEBAPI_ASPECT_STR "aspect_mode"
#define WEBAPI_SOUND_STR "sound_mode"
#define WEBAPI_SPDIF_STR "spdif"
#define WEBAPI_AD_STR "ad_mode"
#define WEBAPI_GUI_STR "gui"
#define WEBAPI_TRANSPARENCY_STR "transparency"
#define WEBAPI_TIMEOUT_STR "timeout"
#define WEBAPI_SWITCH_STR "channel_switch"
#define WEBAPI_BRIGHTNESS_STR "brightness"
#define WEBAPI_CHROMA_STR "chroma"
#define WEBAPI_CONTRAST_STR "contrast"
#define WEBAPI_INFOR_STR "infor"
#define WEBAPI_KEY_STR "key"
#define WEBAPI_TOTAL_STR "total"
#define WEBAPI_DATA_STR "data"
#define WEBAPI_FRONTEND_STR "frontend"
#define WEBAPI_SATELLITE_STR "satellite"
#define WEBAPI_SATID_STR "sat_id"
#define WEBAPI_ID_STR "id"
#define WEBAPI_TYPE_STR "type"
#define WEBAPI_TUNER_STR "tuner"
#define WEBAPI_TUNER_OPTIONS_STR "tuner_options"
#define WEBAPI_DVBS_STR "dvbs"
#define WEBAPI_NAME_STR "name"
#define WEBAPI_POSITION_STR "position"
#define WEBAPI_ANTENNA_STR "antenna"
#define WEBAPI_LNB_POWER_STR "lnb_power"
#define WEBAPI_LNB_FREQUENCY_STR "lnb_frequency"
#define WEBAPI_SWITCH_22K_STR "switch_22k"
#define WEBAPI_DISEQC10_STR "diseqc10"
#define WEBAPI_DISEQC11_STR "diseqc11"
#define WEBAPI_TRANSPONDER_STR "transponder"
#define WEBAPI_FREQUENCY_STR "frequency"
#define WEBAPI_POLARIZATION_STR "polarization"
#define WEBAPI_SYMBOLRATE_STR "symbol_rate"
#define WEBAPI_CATEGORY_STR "category"
#define WEBAPI_PROGRAM_STR "program"
#define WEBAPI_TV_STR "tv"
#define WEBAPI_RADIO_STR "radio"
#define WEBAPI_VIDEOTYPE_STR "video_type"
#define WEBAPI_AUDIOTYPE_STR "audio_type"
#define WEBAPI_SCRAMBLE_STR "scramble"
#define WEBAPI_SKIP_STR "skip"
#define WEBAPI_LOCK_STR "lock"
#define WEBAPI_FAV_STR "fav"
#define WEBAPI_TTX_STR "ttx"
#define WEBAPI_SUBTITLE_STR "subtitle"
#define WEBAPI_AC3_STR "ac3"
#define WEBAPI_HD_STR "hd"
#define WEBAPI_SHAREPATH_STR "share_path"
#define WEBAPI_SIGNAL_STR "signal"
#define WEBAPI_STRENGTH_STR "strength"
#define WEBAPI_QUALITY_STR "quality"
#define WEBAPI_FRONTEND_TYPE_STR "frontend_type"
#define WEBAPI_ACTION_STR "action"
#define WEBAPI_FTA_STR "fta"
#define WEBAPI_PROGTYPE_STR "prog_type"
#define WEBAPI_NIT_STR "nit"
#define WEBAPI_SEARCH_STR "search"
#define WEBAPI_STATE_STR "state"
#define WEBAPI_PROGRESS_STR "progress"
#define WEBAPI_PROGNUM_STR "prog_num"
#define WEBAPI_STREAM_TYPE_STR "stream_type"
#define WEBAPI_MEDIA_TYPE_STR "media_type"
#define WEBAPI_URI_STR "uri"
#define WEBAPI_URI_ENC_FLAG "encrypt"
#define WEBAPI_PLAYINFO_STR "playinfo"
#define WEBAPI_VCODEC_STR "video_codec"
#define WEBAPI_ACODEC_STR "audio_codec"
#define WEBAPI_VWIDTH_STR "video_width"
#define WEBAPI_VHEIGHT_STR "video_height"
#define WEBAPI_BITRATE_STR "bitrate"
#define WEBAPI_FPS_STR "framerate"
#define WEBAPI_CURRENT_TIME_STR "current_time"
#define WEBAPI_TOTAL_TIME_STR "total_time"
#define WEBAPI_DISK_STR "disk"
#define WEBAPI_PATH_STR "path"
#define WEBAPI_FILE_STR "file"

#define WEBAPI_CMD_STR "cmd"

typedef enum
{
	WEBAPI_PROG_TV = 0,
	WEBAPI_PROG_RADIO,
	WEBAPI_PROG_ALL,
}WebapiProgType;

typedef enum
{
	WEBAPI_DEMOD_DVBS = 0,
	WEBAPI_DEMOD_DVBT,
	WEBAPI_DEMOD_DVBC,
	WEBAPI_DEMOD_DTMB,
	WEBAPI_DEMOD_UNKNOWN
}WebapiDemodType;

typedef enum
{
	WEBAPI_SEARCH_TP = 0,
	WEBAPI_SEARCH_PRESET,
	WEBAPI_SEARCH_BLIND,
	WEBAPI_SEARCH_UNKNOWN
}WebapiSearchType;

typedef enum
{
	WEBAPI_STREAM_DVB = 0,
	WEBAPI_STREAM_FILE,
	WEBAPI_STREAM_NET,
	WEBAPI_STREAM_UNKNOWN
}WebapiStreamType;

typedef enum
{
	WEBAPI_MEDIA_VIDEO = 0,
	WEBAPI_MEDIA_AUDIO,
	WEBAPI_MEDIA_PICTURE,
	WEBAPI_MEDIA_UNKNOWN
}WebapiMediaType;

typedef enum
{
	WEBAPI_PLAY_ACTION_START = 0,
	WEBAPI_PLAY_ACTION_PAUSE,
	WEBAPI_PLAY_ACTION_STOP,
	WEBAPI_PLAY_ACTION_SEEK,
	WEBAPI_PLAY_ACTION_UNKNOWN
}WebapiPlayActionType;

typedef struct{
	int action;
	int media_type;
	char *url;
	uint32_t time;
}WebapiNetplayParam;

void app_webapi_server_success(struct mg_connection* nc);
void app_webapi_server_fail(struct mg_connection* nc, webapi_ret_t err_ret);
const char *webapi_get_status_str(webapi_ret_t ret);
WebapiMediaType app_webapi_get_media_type(char *media_str);
void app_webapi_server_volume(struct mg_connection* nc, struct http_message* hm);
void app_webapi_server_time(struct mg_connection* nc, struct http_message* hm);
void app_webapi_server_language(struct mg_connection* nc, struct http_message* hm);
void app_webapi_server_av(struct mg_connection* nc, struct http_message* hm);
void app_webapi_server_gui(struct mg_connection* nc, struct http_message* hm);
void app_webapi_server_infor(struct mg_connection* nc, struct http_message* hm);
void app_webapi_server_key(struct mg_connection* nc, struct http_message* hm);
void app_webapi_server_frontend(struct mg_connection* nc, struct http_message* hm);
void app_webapi_server_satellite(struct mg_connection* nc, struct http_message* hm);
void app_webapi_server_antenna(struct mg_connection* nc, struct http_message* hm);
void app_webapi_server_transponder(struct mg_connection* nc, struct http_message* hm);
void app_webapi_server_category(struct mg_connection* nc, struct http_message* hm);
void app_webapi_server_program(struct mg_connection* nc, struct http_message* hm);
void app_webapi_server_signal(struct mg_connection* nc, struct http_message* hm);
void app_webapi_server_search(struct mg_connection* nc, struct http_message* hm);
void app_webapi_server_play(struct mg_connection* nc, struct http_message* hm);
void app_webapi_server_disk(struct mg_connection* nc, struct http_message* hm);
void app_webapi_server_file(struct mg_connection* nc, struct http_message* hm);
void app_webapi_server_status(struct mg_connection* nc, struct http_message* hm);

void app_webapi_server_cmd(struct mg_connection* nc, struct http_message* hm);
#endif

