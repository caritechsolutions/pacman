#ifndef __GX_OPTIONS_H__
#define __GX_OPTIONS_H__

#define MAX_OPTS_LEN   64
#define MAX_OPTS_COUNT 51

#include "gx_common.h"

typedef enum {
	OPTIONS_USER_AGENT     = 0,
	OPTIONS_XCLIENT_GUID   = 1,
	OPTIONS_CONNECTION     = 2,
	OPTIONS_REFERER        = 3,
	OPTIONS_BASE_URL       = 4,
	OPTIONS_ENC            = 5,
	OPTIONS_ENC_IV         = 6,
	OPTIONS_ENC_KEY        = 7,
	OPTIONS_DEC            = 8,
	OPTIONS_DEC_IV         = 9,
	OPTIONS_DEC_KEY        = 10,
	OPTIONS_ENC_KEY_URL    = 11,
	OPTIONS_ENC_DRM        = 12,
	OPTIONS_DECRYPTION_KEY = 13,
	OPTIONS_AUDIO_DISABLE  = 14,
	OPTIONS_VIDEO_DISABLE  = 15,
	OPTIONS_SUBTITLE_DISABLE = 16,
	OPTIONS_NOBUFFER       = 17,
	OPTIONS_TRANSPORT_TYPE = 18,
	OPTIONS_TIMEOUT        = 19,
	OPTIONS_COOKIE         = 20,
	OPTIONS_FFPROBESIZE    = 21,
	OPTIONS_TS_SOURCE      = 22,
	OPTIONS_SDP_MEDIA_INFO = 23,
	OPTIONS_FFNONBLOCK     = 24,
	OPTIONS_FFANALYZE_DURATION = 25,
	OPTIONS_AV_FREERUN     = 26,
	OPTIONS_NET_STREAM_LIVE_MODE  = 27,
	OPTIONS_LOCATION       = 28,
	OPTIONS_LIVE_START_INDEX = 29,
	OPTIONS_OFFSET         = 30,
	OPTIONS_END_OFFSET     = 31,
	OPTIONS_SEEKABLE       = 32,
	OPTIONS_NET_VN_STREAMS = 33,
	OPTIONS_NET_AN_STREAMS = 34,
	OPTIONS_FIFO_SIZE      = 35,
	OPTIONS_RTMP_ENHANCED_CODECS = 36,
	OPTIONS_SSL_CIPHER_LIST      = 37,
	OPTIONS_AUTHOORIZATION       = 38,
	OPTIONS_CUSTOM_HEADERS       = 39,
	OPTIONS_SOCKS5_PROXY         = 40,
	OPTIONS_HTTP_PROXY           = 41,
	OPTIONS_STC_RECOVER_MODE     = 42,
	OPTIONS_RTP_QUEUE_SIZE       = 43,
	OPTIONS_IS_OPEN_NETWORK_AUDIO_DELAY = 44,
	OPTIONS_RIST_SECRET          = 45,
	OPTIONS_RIST_ENCRYPTION_TYPE = 46,
	OPTIONS_RIST_PROFILE         = 47,
	OPTIONS_SCREEN_TO_DMX_PTS_LMS = 48,
	OPTIONS_SCREEN_TO_DMX_PTS_HMS = 49,
	OPTIONS_DISABLE_ACCEPT = 50,
} OptionsId;

typedef struct {
	char *opt_sign;
	char *opt_name;
	OptionsId opt_id;
} OptionsList;

typedef struct {
	char *opt_val;
	OptionsList opt_list;
} Options;

typedef struct {
	Options opts[MAX_OPTS_COUNT];
} GxOptions;

extern GxOptions* GxOptions_Create(void);
extern char* GxOptions_Get_By_Name(GxOptions *opt, char *opt_sign, char *opt_name);
extern char* GxOptions_Get_By_Id(GxOptions *opt, char *opt_sign, OptionsId opt_id);
extern int   GxOptions_Set(GxOptions *option, char *opt_sign, char *opt_name, char *opt_val);
extern int   GxOptions_Set_By_Url(GxOptions *opt, const char *src);
extern int   GxOptions_Copy(GxOptions *option_src, GxOptions* option_dst);
extern int   GxOptions_Destory(GxOptions *opt);
#endif
