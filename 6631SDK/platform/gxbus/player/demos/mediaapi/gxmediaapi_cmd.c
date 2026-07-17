#include "xshell.h"
#include "log.h"
#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include "gxmedia_api.h"
#include "gxcore.h"
#include "gxplayer_decoder.h"
#include "gxplayer_module.h"

#define READ_LEN (100*1024)
#define PLAY_TS (0)
#define PLAY_ESA (1)
#define PLAY_ESV (2)
typedef struct {
	int play_state;
	int play_type;
	int thread_id;
	char *url;
}PLAY_INFO;

static PLAY_INFO ts_info;
static PLAY_INFO esa_info;
static PLAY_INFO esv_info;
static int init_flag = 0;
static int module_handle;
static const char *sPlayTsStringOP = "i:a:v:u:e:m:";
static const char *sPlayEsaStringOP = "i:s:c:b:t:";
static const char *sPlayEsvStringOP = "i:w:h:f:g:t:d:";
static const char *sHelpStringOP = "n:";

void play_thread(void *arg)
{
	int ret = 0;
	int readLen = 0;
	GxMediaModuleInject inject;
	GxMediaModuleState st;
	uint8_t* buf = GxCore_Malloc(READ_LEN);

	PLAY_INFO *info = (PLAY_INFO*)arg;

	int file_fd = GxCore_Open((char *)info->url, "r");
	if(file_fd < 0) {
		GxCore_Free(buf);
		return;
	}

	while(info->play_state)
	{
		readLen = GxCore_Read(file_fd, buf, 1, READ_LEN);

		if(readLen < 0)
			break;
		else
		{
repeat:
			switch (info->play_type) {
			case PLAY_TS:
				inject.source_type = GXMEDIA_SOURCE_TS;
				inject.buf = buf;
				inject.len = readLen;
				ret = GxMediaApi_ModuleInjectData(module_handle, inject);
				break;
			case PLAY_ESA:
				ret = GxMediaApi_AudioInjectData(module_handle, GXMEDIA_SOURCE_ES, buf, readLen, -1);
				break;
			case PLAY_ESV:
				ret = GxMediaApi_VideoInjectData(module_handle, GXMEDIA_SOURCE_ES, buf, readLen, -1);
				break;
			}

			if(ret <= 0)
			{
				GxCore_ThreadDelay(10);
				goto repeat;
			}
			GxCore_ThreadDelay(1);
		}
	}
	GxCore_Free(buf);
	GxCore_Close(file_fd);

	return;
}

extern int GxPlayer_HeapInit(void);
extern int GxPlayer_SystemInit(void);
static void _gxmedia_init(void)
{
	VideoOutputConfig v_config;
	AudioOutputConfig a_config;

	GxPlayer_SystemInit();
	GxPlayer_HeapInit();

	v_config.interface = GX_VIDEO_OUTPUT_HDMI;
	v_config.mode = GX_VIDEO_OUTPUT_1080I_50HZ;
	a_config.output_data = AUDIO_OUTPUT_PCM;
	a_config.output_port = AUDIO_OUTPUT_HDMI;
	GxPlayer_SetVideoAspect(ASPECT_NORMAL);
	GxPlayer_SetVideoOutputSelect(v_config.interface|GX_VIDEO_OUTPUT_RCA);
	GxPlayer_SetVideoOutputConfig(v_config);
	GxPlayer_SetAudioOutputConfig(a_config);
	GxPlayer_SetAudioVolume(50);
}

static void _printf_audio_codec_type(void)
{
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "0.AUDIO_CODEC_MPEG1\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "1.AUDIO_CODEC_MPEG2\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "2.AUDIO_CODEC_AAC_LATM\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "3.AUDIO_CODEC_AAC_ADTS\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "4.AUDIO_CODEC_VORBIS\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "5.AUDIO_CODEC_AC3\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "6.AUDIO_CODEC_AVS\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "7.AUDIO_CODEC_PCM\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "8.AUDIO_CODEC_EAC3\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "9.AUDIO_CODEC_DTS\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "10.AUDIO_CODEC_DRA1\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "11.AUDIO_CODEC_DTS_HD\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "12.AUDIO_CODEC_REAL\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "13.AUDIO_CODEC_RA_AAC\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "14.AUDIO_CODEC_RA_RA8LBR\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "15.AUDIO_CODEC_FLAC\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "16.AUDIO_CODEC_OPUS\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "17.AUDIO_CODEC_AMR_NB\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "18.AUDIO_CODEC_AMR_WB\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "19.AUDIO_CODEC_APE\n\n");
}

static void _printf_video_codec_type(void)
{
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "0.VIDEO_CODEC_MPEG12\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "1.VIDEO_CODEC_MPEG4\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "2.VIDEO_CODEC_H263\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "3.VIDEO_CODEC_H264\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "4.VIDEO_CODEC_REAL\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "5.VIDEO_CODEC_AVS\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "6.VIDEO_CODEC_H265\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "7.VIDEO_CODEC_VC1\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "8.VIDEO_CODEC_MJPEG\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "9.VIDEO_CODEC_DIV3\n\n");
}

static void _gxmedia_play_ts_help(void)
{
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "----------PLAY COMMAND HELP---------\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "Usage: play_ts  -i <url> -a <audio pid> -v <video pid> -u <audio codectype> -e video codectype -m decode mode\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     -i : gxmedia play url\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     -a : audio pid\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     -v : video pid\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     -u : audio codectype\n");
	_printf_audio_codec_type();

	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     -e : video codectype\n");
	_printf_video_codec_type();

	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     -m : decode mode\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "0.VIDEO_DECODE_ALL\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "1.VIDEO_DECODE_I_ONLY\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "2.VIDEO_DECODE_IP\n");
}

static void _gxmedia_play_esa_help(void)
{
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "----------PLAY COMMAND HELP---------\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "Usage: play_es  -i <url> -s <samplerate> -b <sample bits> -c <channel num> -t <audio codectype>\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     -i : gxmedia play url\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     -s : samplerate\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     -b : sample bits\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     -c : channel num\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     -t : audio codectype\n");
	_printf_audio_codec_type();
}

static void _gxmedia_play_esv_help(void)
{
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "----------PLAY COMMAND HELP---------\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "Usage: play_esv  -i <url> -w <video width> -h <video height> -f <frame rate> -g <mosaic gate> -t <codectype> -d <decode mode>\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     -i : gxmedia play url\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     -w : video width\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     -h : video  height\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     -f : frame rate\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     -g : mosaic gate\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     -t : video codectype\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     -d : decode mode\n");
	_printf_video_codec_type();
}

static void _gxmedia_stop_help(void)
{
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "----------STOP COMMAND HELP---------\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "Usage: stop -n <media name> \n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     -n : gxmedia name\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     0: Stop TS\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     1: Stop Esa\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     2 :Stop Esv\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "\n");
}

static void _gxmedia_resume_help(void)
{
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "----------RESUME COMMAND HELP---------\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "Usage: resume -n <media name> \n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     -n : gxmedia name\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     0: resume TS\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     1: resume Esa\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     2 :resume Esv\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "\n");
}

static void _gxmedia_pause_help(void)
{
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "----------PAUSE COMMAND HELP---------\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "Usage: pause -n <media name> \n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     -n : gxmedia name\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     0: pause TS\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     1: pause Esa\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     2 :pause Esv\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "\n");
}

static void _gxmedia_finished_help(void)
{
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "----------FINISH COMMAND HELP---------\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "Usage: finish -n <media name> \n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     -n: gxmedia name\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     0: finish TS\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     1: finish Esa\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     2: finish Esv\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "\n");
}

static void _gxmedia_info_help(void)
{
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "----------INFO COMMAND HELP---------\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "Usage: info -n <media name> \n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     -n: gxmedia name\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     0: info TS\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     1: info Esa\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "     2: info Esv\n");
	Log_Printf(LOG_MEDIAAPI, LOG_INFO, "\n");
}

static int _play_ts(char *url, GxMediaModulePara input_params)
{
	if (url == NULL) {
		Log_Printf(LOG_MEDIAAPI, LOG_INFO, "url is null\n");
		return -1;
	}

	ts_info.play_state = 1;
	ts_info.play_type  = PLAY_TS;
	ts_info.thread_id = 1;
	ts_info.url = url;

	GxMediaModulePara params;
	memset(&params, 0, sizeof(params));
	module_handle = GxMediaApi_ModuleOpen(GXMEDIA_SOURCE_TS);

	params.aparam.codectype = input_params.aparam.codectype;
	params.vparam.codectype = input_params.vparam.codectype;
	params.vparam.decode_mode = input_params.vparam.decode_mode;

	params.tsid   = -1;
	params.audPid = input_params.audPid;
	params.vidPid = input_params.vidPid;
	params.pcrPid = input_params.audPid;
	params.freq_HZ = 45000;
	params.sync_mode = -1;
	GxMediaApi_ModuleConfig(module_handle, params);

	GxMediaApi_ModuleStart(module_handle);
	GxCore_ThreadCreate("ts_play", &ts_info.thread_id, play_thread, (void*)&ts_info, 100*1024, 10);

	return 0;
}

static int _gxmedia_play_ts(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	unsigned char flags = 0;
	char* url = NULL;
	int opt;
	GxMediaModulePara params;

	memset(&params, 0, sizeof(params));
	optind = 0;
	while ((opt = GetOpt(argc, argv, sPlayTsStringOP)) != -1) {
		flags = 1;
		switch (opt) {
		case 'i':
			url = strdup(optarg);
			break;
		case 'a':
			params.audPid = strtol(optarg, NULL, 16);
			break;
		case 'v':
			params.vidPid = strtol(optarg, NULL, 16);
			break;
		case 'u':
			params.aparam.codectype = (AudioCodecType)(1<<atoi(optarg));
			break;
		case 'e':
			params.vparam.codectype = (VideoCodecType)(1<<atoi(optarg));
			break;
		case 'm':
			params.vparam.decode_mode =(GxMediaVideoDecodeMode)(atoi(optarg));
			break;
		default:
			_gxmedia_play_ts_help();
			return 0;
		}
	}

	if (!flags) {
		_gxmedia_play_ts_help();
		return 0;
	}

	if (!init_flag) {
		_gxmedia_init();
		init_flag = 1;
	}
	_play_ts(url, params);

	return 0;
}

static int _play_esa(char *url, GxMediaAudioPara input_params)
{
	GxMediaAudioPara params;

	if (url == NULL) {
		Log_Printf(LOG_MEDIAAPI, LOG_INFO, "url is null \n");
		return -1;
	}

	esa_info.play_state = 1;
	esa_info.play_type  = PLAY_ESA;
	esa_info.thread_id = 1;
	esa_info.url = url;

	memset(&params, 0, sizeof(params));

	module_handle = GxMediaApi_AudioOpen(0);
	params.sample_rate = input_params.sample_rate;
	params.sample_bits = input_params.sample_bits;
	params.channel_num = input_params.channel_num;
	params.codectype   = input_params.codectype;


	GxMediaApi_AudioConfig(module_handle, params);

	GxMediaApi_AudioStart(module_handle);
	GxCore_ThreadCreate("esa_play", &esa_info.thread_id, play_thread, (void*)&esa_info, 100*1024, 10);

	return 0;
}

static int _gxmedia_play_esa(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	unsigned char flags = 0;
	char* url = NULL;
	int opt;
	GxMediaAudioPara params;

	memset(&params, 0, sizeof(params));
	optind = 0;
	while ((opt = GetOpt(argc, argv, sPlayEsaStringOP)) != -1) {
		flags = 1;
		switch (opt) {
		case 'i':
			url = strdup(optarg);
			break;
		case 's':
			params.sample_rate = atoi(optarg);
			break;
		case 'b':
			params.sample_bits = atoi(optarg);
			break;
		case 'c':
			params.channel_num = atoi(optarg);
			break;
		case 't':
			params.codectype = (AudioCodecType)(1<<atoi(optarg));
			break;
		default:
			_gxmedia_play_esa_help();
			return 0;
		}
	}

	if (!flags) {
		_gxmedia_play_esa_help();
		return 0;
	}

	if (!init_flag) {
		_gxmedia_init();
		init_flag = 1;
	}
	_play_esa(url, params);

	return 0;
}
static int _play_esv(char *url, GxMediaVideoPara input_params)
{
	GxMediaVideoPara params;

	if (url == NULL) {
		Log_Printf(LOG_MEDIAAPI, LOG_INFO, "url为空\n");
		return -1;
	}

	esv_info.play_state = 1;
	esv_info.play_type  = PLAY_ESV;
	esv_info.thread_id = 1;
	esv_info.url = url;

	memset(&params, 0, sizeof(params));

	module_handle = GxMediaApi_VideoOpen(0);
	params.width = input_params.width;
	params.height = input_params.height;
	params.frame_rate = input_params.frame_rate;
	params.mosaic_gate = input_params.mosaic_gate;
	params.decode_mode = input_params.decode_mode;
	params.codectype   = input_params.codectype;
	GxMediaApi_VideoConfig(module_handle, params);

	GxMediaApi_VideoStart(module_handle);
	GxCore_ThreadCreate("esv_play", &esv_info.thread_id, play_thread, (void*)&esv_info, 100*1024, 10);

	return 0;
}

static int _gxmedia_play_esv(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	unsigned char flags = 0;
	char* url = NULL;
	int opt;
	GxMediaVideoPara params;

	memset(&params, 0, sizeof(params));
	optind = 0;
	while ((opt = GetOpt(argc, argv, sPlayEsvStringOP)) != -1) {
		flags = 1;
		switch (opt) {
		case 'i':
			url = strdup(optarg);
			break;
		case 'w':
			params.width = atoi(optarg);
			break;
		case 'h':
			params.height = atoi(optarg);
			break;
		case 'f':
			params.frame_rate = atoi(optarg);
			break;
		case 'g':
			params.mosaic_gate = atoi(optarg);
			break;
		case 't':
			params.codectype = (VideoCodecType)(1<<atoi(optarg));
			break;
		case 'd':
			params.decode_mode = (GxMediaVideoDecodeMode)atoi(optarg);
			break;
		default:
			_gxmedia_play_esv_help();
			return 0;
		}
	}

	if (!flags) {
		_gxmedia_play_esv_help();
		return 0;
	}

	if (!init_flag) {
		_gxmedia_init();
		init_flag = 1;
	}
	_play_esv(url, params);

	return 0;
}
static int _gxmedia_stop(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	int opt;
	unsigned char flags = 0;
	int name = -1;

	optind = 0;
	while ((opt = GetOpt(argc, argv, sHelpStringOP)) != -1) {
		flags = 1;
		switch (opt) {
		case 'n':
			name = atoi(optarg);
			break;

		default:
			_gxmedia_stop_help();
			return 0;
		}
	}

	if (!flags) {
		_gxmedia_stop_help();
		return 0;
	}
	switch (name) {
	case PLAY_TS:
		ts_info.play_state = 0;
		GxMediaApi_ModuleStop(module_handle, 0);
		GxMediaApi_ModuleClose(module_handle);
		break;
	case PLAY_ESA:
		esa_info.play_state = 0;
		GxMediaApi_AudioStop(module_handle);
		GxMediaApi_AudioClose(module_handle);
		break;
	case PLAY_ESV:
		esv_info.play_state = 0;
		GxMediaApi_VideoStop(module_handle, 0);
		GxMediaApi_VideoClose(module_handle);
		break;
	default:
		Log_Printf(LOG_MEDIAAPI, LOG_ERROR, "no  this module\n");
		break;

	}

	return 0;
}

static int _gxmedia_pause(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	int opt;
	unsigned char flags = 0;
	int name = -1;

	optind = 0;
	while ((opt = GetOpt(argc, argv, sHelpStringOP)) != -1) {
		flags = 1;
		switch (opt) {
		case 'n':
			name = atoi(optarg);
			break;
		default:
			_gxmedia_pause_help();
			return 0;
		}
	}

	if (!flags) {
		_gxmedia_pause_help();
		return 0;
	}
	switch (name) {
	case PLAY_TS:
		GxMediaApi_ModulePause(module_handle);
		break;
	case PLAY_ESA:
		GxMediaApi_AudioPause(module_handle);
		break;
	case PLAY_ESV:
		GxMediaApi_VideoPause(module_handle);
		break;
	default:
		Log_Printf(LOG_MEDIAAPI, LOG_ERROR, "no  this module\n");
		break;

	}

	return 0;
}

static int _gxmedia_resume(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	int opt;
	unsigned char flags = 0;
	int name = -1;

	optind = 0;
	while ((opt = GetOpt(argc, argv, sHelpStringOP)) != -1) {
		flags = 1;
		switch (opt) {
		case 'n':
			name = atoi(optarg);
			break;

		default:
			_gxmedia_resume_help();
			return 0;
		}
	}

	if (!flags) {
		_gxmedia_stop_help();
		return 0;
	}
	switch (name) {
	case PLAY_TS:
		GxMediaApi_ModuleResume(module_handle);
		break;
	case PLAY_ESA:
		GxMediaApi_AudioResume(module_handle);
		break;
	case PLAY_ESV:
		GxMediaApi_VideoResume(module_handle);
		break;
	default:
		Log_Printf(LOG_MEDIAAPI, LOG_ERROR, "no  this module\n");
		break;

	}

	return 0;
}

static int _gxmedia_finished(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	int opt;
	unsigned char flags = 0;
	int name = -1;

	optind = 0;
	while ((opt = GetOpt(argc, argv, sHelpStringOP)) != -1) {
		flags = 1;
		switch (opt) {
		case 'n':
			name = atoi(optarg);
			break;

		default:
			_gxmedia_finished_help();
			return 0;
		}
	}

	if (!flags) {
		_gxmedia_finished_help();
		return 0;
	}
	switch (name) {
	case PLAY_TS:
		GxMediaApi_ModuleFinished(module_handle);
		break;
	case PLAY_ESA:
		GxMediaApi_AudioFinished(module_handle);
		break;
	case PLAY_ESV:
		GxMediaApi_VideoFinished(module_handle);
		break;
	default:
		Log_Printf(LOG_MEDIAAPI, LOG_ERROR, "no  this module\n");
		break;

	}

	return 0;
}
static void _printf_module_status(GxMediaModuleDecState state, char *module)
{
	switch (state.state) {
	case GXMEDIA_STATE_NO_DEC:
		Log_Printf(LOG_MEDIAAPI, LOG_INFO, "%s no decode\n", module);
		break;
	case GXMEDIA_STATE_RUNNING:
		Log_Printf(LOG_MEDIAAPI, LOG_INFO, "%s is running\n", module);
		break;
	case GXMEDIA_STATE_STOPPED:
		Log_Printf(LOG_MEDIAAPI, LOG_INFO, "%s is stopping\n", module);
		break;
	case GXMEDIA_STATE_ERROR:
		Log_Printf(LOG_MEDIAAPI, LOG_INFO, "%s dec error\n", module);
		break;
	default:
		break;
	}

	switch (state.err_code) {
	case GXMEDIA_ERR_NO_ERROR:
		Log_Printf(LOG_MEDIAAPI, LOG_INFO, "%s no error\n", module);
		break;
	case GXMEDIA_ERR_UNKOWN_ERROR:
		Log_Printf(LOG_MEDIAAPI, LOG_INFO, "%s unkown error\n", module);
		break;
	case GXMEDIA_ERR_ES_OVERFLOW:
		Log_Printf(LOG_MEDIAAPI, LOG_INFO, "%s es overflow\n", module);
		break;
	case GXMEDIA_ERR_DEC_UNSUPPORT:
		Log_Printf(LOG_MEDIAAPI, LOG_INFO, "%s dec unsupport\n", module);
		break;
	case GXMEDIA_ERR_SIZE_UNSPPORT:
		Log_Printf(LOG_MEDIAAPI, LOG_INFO, "%s size unsupport\n", module);
		break;
	case GXMEDIA_ERR_SIZE_CHANGED:
		Log_Printf(LOG_MEDIAAPI, LOG_INFO, "%s size changed\n", module);
		break;
	case GXMEDIA_ERR_FB_OVERFLOW:
		Log_Printf(LOG_MEDIAAPI, LOG_INFO, "%s fb overflow\n", module);
		break;
	default:
		break;
	}
}

static int _gxmedia_info(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	int opt, name = -1;
	unsigned char flags = 0;
	int free_size = 0, used_size = 0, total_size = 0;
	GxMediaModuleState state;

	optind = 0;
	while ((opt = GetOpt(argc, argv, sHelpStringOP)) != -1) {
		flags = 1;
		switch (opt) {
		case 'n':
			name = atoi(optarg);
			break;

		default:
			_gxmedia_info_help();
			return 0;
		}
	}

	if (!flags) {
		_gxmedia_info_help();
		return 0;
	}
	switch (name) {
	case PLAY_TS:
		{
			GxMediaModuleInfo  info;

			GxMediaApi_ModuleInfo (module_handle, &info);
			GxMediaApi_ModuleState(module_handle, &state);
			_printf_module_status(state.audio, "Audio");
			_printf_module_status(state.video, "Video");
			Log_Printf(LOG_MEDIAAPI, LOG_INFO,
					"[esa] free: %d\t used: %d\n",
					info.audio.esa_free_size,
					info.audio.esa_used_size);
			Log_Printf(LOG_MEDIAAPI, LOG_INFO,
					"[esv] free: %d\t used: %d\n",
					info.video.esv_free_size,
					info.video.esv_used_size);
		}
		break;
	case PLAY_ESA:
		{
			GxMediaModuleAudioFifo info;

			GxMediaApi_AudioGetDecState(module_handle, &state.audio);
			GxMediaApi_AudioGetFifoInfo(module_handle, &info);
			_printf_module_status(state.audio, "Audio");
			Log_Printf(LOG_MEDIAAPI, LOG_INFO,
					"[esa] free: %d\t used: %d\n",
					info.esa_free_size,
					info.esa_used_size);
		}
		break;
	case PLAY_ESV:
		{
			GxMediaModuleVideoFifo info;

			GxMediaApi_VideoGetDecState(module_handle, &state.video);
			GxMediaApi_VideoGetFifoInfo(module_handle, &info);
			_printf_module_status(state.video, "Video");
			Log_Printf(LOG_MEDIAAPI, LOG_INFO,
					"[esv] free: %d\t used: %d\n",
					info.esv_free_size,
					info.esv_used_size);
		}
		break;
	default:
		Log_Printf(LOG_MEDIAAPI, LOG_ERROR, "no  this module\n");
		break;

	}

	return 0;
}

ShellCmd gGxMediaApi_PlayTs = {
	.name  = "play_ts",
	.func  = _gxmedia_play_ts,
	.brief = "play_ts   [help]: gxmediaapi play ts  command",
};

ShellCmd gGxMediaApi_PlayEsa = {
	.name  = "play_esa",
	.func  = _gxmedia_play_esa,
	.brief = "play_esa   [help]: gxmediaapi play esa command",
};

ShellCmd gGxMediaApi_PlayEsv = {
	.name  = "play_esv",
	.func  = _gxmedia_play_esv,
	.brief = "play_esv   [help]: gxmediaapi play esv command",
};

ShellCmd gGxMediaApiStop = {
	.name  = "stop",
	.func  = _gxmedia_stop,
	.brief = "stop   [help]: gxmediaapi stop   command",
};

ShellCmd gGxMediaApiPause = {
	.name  = "pause",
	.func  = _gxmedia_pause,
	.brief = "pause  [help]: gxmediaapi pause  command",
};

ShellCmd gGxMediaApiResume = {
	.name  = "resume",
	.func  = _gxmedia_resume,
	.brief = "resume [help]: gxmediaapi resume command",
};

ShellCmd gGxMediaApiFinished = {
	.name  = "finish",
	.func  = _gxmedia_finished,
	.brief = "finish [help]: gxmediaapi resume command",
};

ShellCmd gGxMediaApiInfo = {
	.name  = "info",
	.func  = _gxmedia_info,
	.brief = "info [help]: gxmediaapi info command",
};
