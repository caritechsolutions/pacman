#include "gx_options.h"

#define CHECK_OPTIONS_PARAM(param) if (param == NULL) continue;
#define ALLOC_OPTIONS_PARAM(dparam, sparam) \
	do {                                    \
		if (!sparam)                        \
			continue;                       \
		if (dparam)                         \
			av_free(dparam);                \
		dparam = (char*)av_strdup(sparam);  \
	}while(0)

static OptionsList option_list[] = {
	{" -H", "User-Agent:",     OPTIONS_USER_AGENT},
	{" -H", "xClientGUID:",    OPTIONS_XCLIENT_GUID},
	{" -H", "Connection:",     OPTIONS_CONNECTION},
	{" -H", "Referer:",        OPTIONS_REFERER},
	{" -H", "BaseUrl:",        OPTIONS_BASE_URL},
	{" -H", "enc:",            OPTIONS_ENC},
	{" -H", "enc_iv:",         OPTIONS_ENC_IV},
	{" -H", "enc_key:",        OPTIONS_ENC_KEY},
	{" -H", "dec:",            OPTIONS_DEC},
	{" -H", "dec_iv:",         OPTIONS_DEC_IV},
	{" -H", "dec_key:",        OPTIONS_DEC_KEY},
	{" -H", "enc_key_url:",    OPTIONS_ENC_KEY_URL},
	{" -H", "enc_drm:",        OPTIONS_ENC_DRM},
	{" -H", "decryption_key:", OPTIONS_DECRYPTION_KEY},
	{" -H", "audio_disable:",  OPTIONS_AUDIO_DISABLE},/*disable audio, other value = invalid settings, 1 = disable audio all(demux+codec), 2 = disable audio codec*/
	{" -H", "video_disable:",  OPTIONS_VIDEO_DISABLE},/*disable video, other value = invalid settings, 1 = disable video all(demux+codec), 2 = disable video codec*/
	{" -H", "subtitle_disable:", OPTIONS_SUBTITLE_DISABLE},/*disable subtitling, other value = invalid settings, 1 = disable subtile all(demux+codec)*/
	{" -H", "no_cache_flag:",  OPTIONS_NOBUFFER},
	{" -H", "transport_type:", OPTIONS_TRANSPORT_TYPE},
	{" -H", "timeout:",        OPTIONS_TIMEOUT},/*timeout:1*N s timeout,not set:default:6s*/
	{" -H", "Set-Cookie:",     OPTIONS_COOKIE},
	{" -H", "ffprobesize:",    OPTIONS_FFPROBESIZE},
	{" -H", "ts_source:",      OPTIONS_TS_SOURCE},
	{" -H", "sdp_media_info:", OPTIONS_SDP_MEDIA_INFO},
	{" -H", "ffnonblock_flag:",OPTIONS_FFNONBLOCK},/*value:0 blocking mode; 1 non-blocking mode*/
	{" -H", "ffanalyzeduration:",OPTIONS_FFANALYZE_DURATION},/*min value:5s*/
	{" -H", "av_freerun:",     OPTIONS_AV_FREERUN},/*set freerun mode;1:freerun. other:not set*/
	{" -H", "net_stream_live_mode:",  OPTIONS_NET_STREAM_LIVE_MODE},/*set network live mode.1:live, other:none,default vod mode*/
	{" -H", "location:",       OPTIONS_LOCATION},/*http location.*/
	{" -H", "live_start_index:", OPTIONS_LIVE_START_INDEX},/*segment index to start live streams at (negative values are from the end).defatlue:-3.*/
	{" -H", "offset:",         OPTIONS_OFFSET},
	{" -H", "end_offset:",     OPTIONS_END_OFFSET},
	{" -H", "http_seekable:",  OPTIONS_SEEKABLE},/*http seekable.Use HTTP partial requests, 0 = disable, 1 = enable*/
	{" -H", "vn_streams:",	   OPTIONS_NET_VN_STREAMS},/*disable stream url video track*/
	{" -H", "an_streams:",	   OPTIONS_NET_AN_STREAMS}, /*disable stream url audio track*/
	{" -H", "fifo_size:",	   OPTIONS_FIFO_SIZE}, /*disable stream url audio track*/
	{" -H", "rtmp_enhanced_codecs:", OPTIONS_RTMP_ENHANCED_CODECS}, /*Specify the codec(s) to use in an enhanced rtmp live stream*/
	{" -H", "ssl_cipher_list:", OPTIONS_SSL_CIPHER_LIST}, /*set ssl cipher list.Refer::https://www.wolfssl.com/documentation/manuals/wolfssl/group__Setup.html#function-wolfssl_ctx_set_cipher_list*/
	{" -H", "Authorization:", OPTIONS_AUTHOORIZATION}, /*Private clients need this Authorization*/
	{" -H", "CustomHeaders:", OPTIONS_CUSTOM_HEADERS}, /*now add in custom headers*/
	{" -H", "socks5_proxy:", OPTIONS_SOCKS5_PROXY}, /*socks5 proxy*/
	{" -H", "http_proxy:",   OPTIONS_HTTP_PROXY}, /*http proxy*/
	{" -H", "stc_recover_mode:", OPTIONS_STC_RECOVER_MODE}, /*set stc recover mode*/
	{" -H", "rtp_queue_size:", OPTIONS_RTP_QUEUE_SIZE}, /*rtp still missing some packet, enqueue this one.set packet queue size */
	{" -H", "IsOpenNetAudioDelay:", OPTIONS_IS_OPEN_NETWORK_AUDIO_DELAY}, /*now add in custom headers*/
	{" -H", "rist_secret:", OPTIONS_RIST_SECRET}, /*set encryption secret */
	{" -H", "rist_encryption_type:", OPTIONS_RIST_ENCRYPTION_TYPE}, /*set encryption type */
	{" -H", "rist_profile:", OPTIONS_RIST_PROFILE}, /*set librist profile.(0:simple, 1:main, 2:advanced) */
	{" -H", "screen_to_dmx_pts_lms:", OPTIONS_SCREEN_TO_DMX_PTS_LMS}, /*set librist profile.(0:simple, 1:main, 2:advanced) */
	{" -H", "screen_to_dmx_pts_hms:", OPTIONS_SCREEN_TO_DMX_PTS_HMS}, /*set librist profile.(0:simple, 1:main, 2:advanced) */
	{" -H", "disable_accept:",     OPTIONS_DISABLE_ACCEPT},
};


GxOptions* GxOptions_Create(void)
{
	int i;
	GxOptions *option = (GxOptions*)av_malloc(sizeof(GxOptions));

	for (i = 0; i < MAX_OPTS_COUNT; i++) {
		option->opts[i].opt_val           = NULL;
		option->opts[i].opt_list.opt_id   = -1;
		option->opts[i].opt_list.opt_sign = NULL;
		option->opts[i].opt_list.opt_name = NULL;
	}

	return option;
}

int GxOptions_Copy(GxOptions *option_src, GxOptions* option_dst)
{
	int i;
	if (option_src == NULL || option_dst == NULL)
		return -1;

	for (i = 0; i < MAX_OPTS_COUNT; i++) {
		ALLOC_OPTIONS_PARAM(option_dst->opts[i].opt_val, option_src->opts[i].opt_val);
		ALLOC_OPTIONS_PARAM(option_dst->opts[i].opt_list.opt_sign, option_src->opts[i].opt_list.opt_sign);
		ALLOC_OPTIONS_PARAM(option_dst->opts[i].opt_list.opt_name, option_src->opts[i].opt_list.opt_name);
		option_dst->opts[i].opt_list.opt_id = option_src->opts[i].opt_list.opt_id;
	}

	return 0;
}

int GxOptions_Set(GxOptions *option, char *opt_sign, char *opt_name, char *opt_val)
{
	int i;

	if (option == NULL)
		return -1;

	if (opt_sign == NULL || opt_name == NULL || opt_val == NULL)
		return -1;

	for (i = 0; i < sizeof(option_list)/sizeof(OptionsList); i++) {
		if (strcasecmp(option_list[i].opt_sign, opt_sign) == 0 &&
				strcasecmp(option_list[i].opt_name, opt_name) == 0) {

			ALLOC_OPTIONS_PARAM(option->opts[i].opt_list.opt_sign, option_list[i].opt_sign);
			ALLOC_OPTIONS_PARAM(option->opts[i].opt_list.opt_name, option_list[i].opt_name);
			ALLOC_OPTIONS_PARAM(option->opts[i].opt_val, opt_val);
			option->opts[i].opt_list.opt_id = option_list[i].opt_id;

			return 0;
		}
	}
	return 0;
}

int GxOptions_Set_By_Url(GxOptions *option, const char *url)
{
	int i;
	char *ptr1, *ptr2, *ptr3 = NULL;

	if ((option == NULL) || (url == NULL))
		return -1;

	for (i = 0; i < sizeof(option_list)/sizeof(OptionsList); i++) {
		ptr1 = strstr(url, option_list[i].opt_sign);
		if (ptr1 == NULL)
			continue;

		ptr2 = strstr(ptr1, option_list[i].opt_name);
		if (ptr2 == NULL)
			continue;

		ALLOC_OPTIONS_PARAM(option->opts[i].opt_list.opt_sign, option_list[i].opt_sign);
		ALLOC_OPTIONS_PARAM(option->opts[i].opt_list.opt_name, option_list[i].opt_name);

		int j = 0, def_opts_len = MAX_OPTS_LEN;
		char *tmp_ptr = NULL;
		def_opts_len = FFMAX(def_opts_len, strlen(ptr2));
		if (!(tmp_ptr = av_mallocz(def_opts_len))) {
			return -1;
		}
		ptr2 += strlen(option_list[i].opt_name);

		while (*ptr2 == ' ')
			ptr2++;
		if (*ptr2 == '"')
			ptr3 = ptr2++;
		while ((j < def_opts_len) &&
				((ptr3 == NULL) || (*ptr2 != ((*ptr3 == '"') ? '"' : ' '))) &&
				(*ptr2 != '\n' && *ptr2 != '\r' && *ptr2 != '\0')) {
			if ((ptr3 == NULL) && (*ptr2 == ' '))
				break;
			tmp_ptr[j] = *(ptr2++);
			j++;
		}
		tmp_ptr[j] = '\0';
		if (!option->opts[i].opt_val) {
			if (!(option->opts[i].opt_val = (char*)av_mallocz((strlen(tmp_ptr)+1)))) {
				av_freep(&tmp_ptr);
				return -1;
			}
		}
		snprintf (option->opts[i].opt_val, strlen(tmp_ptr)+1, "%s", tmp_ptr);
		av_freep(&tmp_ptr);
		option->opts[i].opt_list.opt_id = option_list[i].opt_id;
	}

	return 0;
}

char* GxOptions_Get_By_Name(GxOptions *option, char *opt_sign, char *opt_name)
{
	int i;

	if ((option == NULL) || (opt_sign == NULL) || (opt_name == NULL))
		return NULL;

	for (i = 0; i < MAX_OPTS_COUNT; i++)
	{
		CHECK_OPTIONS_PARAM(option->opts[i].opt_list.opt_sign);
		CHECK_OPTIONS_PARAM(option->opts[i].opt_list.opt_name);

		if (strcasecmp(option->opts[i].opt_list.opt_sign, opt_sign) == 0 &&
				strcasecmp(option->opts[i].opt_list.opt_name, opt_name) == 0) {
			if (option->opts[i].opt_val != NULL)
				return option->opts[i].opt_val;
		}
	}

	return NULL;
}

char* GxOptions_Get_By_Id(GxOptions *option, char *opt_sign, OptionsId opt_id)
{
	int i;

	if (option == NULL)
		return NULL;

	for (i = 0; i < MAX_OPTS_COUNT; i++)
	{
		CHECK_OPTIONS_PARAM(option->opts[i].opt_list.opt_sign);

		if (strcasecmp(option->opts[i].opt_list.opt_sign, opt_sign) == 0 &&
				(option->opts[i].opt_list.opt_id == opt_id)) {
			if (option->opts[i].opt_val != NULL)
				return option->opts[i].opt_val;
		}
	}

	return NULL;
}

int GxOptions_Destory(GxOptions *option)
{
	if (option == NULL)
		return -1;

	int i;
	for (i = 0; i < MAX_OPTS_COUNT; i++)
	{
		if (option->opts[i].opt_list.opt_sign != NULL) {
			av_free(option->opts[i].opt_list.opt_sign);
			option->opts[i].opt_list.opt_sign = NULL;
		}
		if (option->opts[i].opt_list.opt_name != NULL) {
			av_free(option->opts[i].opt_list.opt_name);
			option->opts[i].opt_list.opt_name = NULL;
		}
		if (option->opts[i].opt_val != NULL) {
			av_free(option->opts[i].opt_val);
			option->opts[i].opt_val = NULL;
		}
		option->opts[i].opt_list.opt_id = -1;
	}

	av_free(option);

	return 0;
}

