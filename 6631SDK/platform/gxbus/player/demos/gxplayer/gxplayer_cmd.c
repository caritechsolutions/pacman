#include "xshell.h"
#include "log.h"
#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include "gxplayer_module.h"

#define MAX_PLAYER 10
static const char *sPlayStringOP = "n:i:s:v:";
static const char *sStopStringOP = "n:h:";
static const char *sPauseStringOP = "n:h:";
static const char *sResumeStringOP = "n:h:";
static const char *sInfoStringOP = "n:s::";

static char *sPlayerName[MAX_PLAYER] = {0};

static int _check_player(char *name)
{
	int i = 0;

	for (i = 0; i < MAX_PLAYER; i++) {
		if(strcmp(sPlayerName[i], name) == 0) {
			return 0;
		}
	}
	return 1;
}

static int _add_player(char *name)
{
	int i = 0;

	for (i = 0; i < MAX_PLAYER; i++) {
		if (sPlayerName[i] == NULL) {
			sPlayerName[i] = name;
			return 0;
		}
	}
	return 1;
}

static int _delete_player(char *name)
{
	int i = 0;

	for (i = 0; i < MAX_PLAYER; i++) {
		if(strcmp(sPlayerName[i], name) == 0) {
			sPlayerName[i] = NULL;
			return 0;
		}
	}
	return 1;
}

static void _gxplayer_play_help(void)
{
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "----------PLAY COMMAND HELP---------\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "Usage: play -n <name> -i <url>\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "     -n : gxplayer name\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "     -i : gxplayer play url\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "     -s : gxplayer start time\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "     -v : gxplayer volume\n");
}

static void _gxplayer_stop_help(void)
{
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "----------STOP COMMAND HELP---------\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "Usage: stop -n <player name> \n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "     -n : gxplayer name\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "\n");
}

static void _gxplayer_pause_help(void)
{
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "----------PAUSE COMMAND HELP---------\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "Usage: pause -n <player name> \n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "     -n : gxplayer name\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "\n");
}
static void _gxplayer_resume_help(void)
{
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "----------RESUME COMMAND HELP---------\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "Usage: resume -n <playername> \n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "     -n : gxplayer name\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "\n");
}

static void _gxplayer_info_help(void)
{
	int i = 0;
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "----------INFO COMMAND HELP---------\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "Usage: info -n <player name> -s\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "     -n : gxplayer name\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "     -s : gxplayer status\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "\n");
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "gxplayer name:\n");
	for (i = 0; i < MAX_PLAYER; i++) {
		if (sPlayerName[i] != NULL)
			Log_Printf(LOG_GXPLAYER, LOG_INFO, "     %s\n", sPlayerName[i]);
	}
}

static void _gxplayer_init(void)
{
	static int s_player_init = 0;

	if (s_player_init == 0) {
		GxPlayer_ModuleInit();
		GxPlayer_ModuleRegisterALL();
		VideoOutputConfig Config;
		DisplayScreen Screen;

		Config.interface = GX_VIDEO_OUTPUT_HDMI;
		Config.mode = GX_VIDEO_OUTPUT_1080I_50HZ;
		Screen.xres = 1280;
		Screen.yres = 720;
		Screen.aspect = DISPLAY_SCREEN_16X9;
		GxPlayer_SetDisplayScreen(Screen);
		GxPlayer_SetVideoAspect(ASPECT_NORMAL);
		GxPlayer_SetVideoOutputSelect(Config.interface|GX_VIDEO_OUTPUT_RCA);
		GxPlayer_SetVideoOutputConfig(Config);
		GxPlayer_SetAudioVolume(50);
		s_player_init = 1;
	}
}

static int _gxplayer_play(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	int opt, ret;
	unsigned char flags = 0;
	char* url = NULL;
	char* name = NULL;
	int start = 0;
	int volume = 0;

	_gxplayer_init();

	optind = 0;
	while ((opt = GetOpt(argc, argv, sPlayStringOP)) != -1) {
		flags = 1;
		switch (opt) {
		case 'i':
			url = strdup(optarg);
			break;
		case 'v':
			volume = atoi(optarg);
			break;
		case 's':
			start = atoi(optarg);
			break;
		case 'n':
			name = strdup(optarg);
			break;

		default:
			_gxplayer_play_help();
			return 0;
		}
	}

	if (!flags) {
		_gxplayer_play_help();
		return 0;
	}

	ret = GxPlayer_MediaPlay(name, url, start*1000, volume, NULL);
	if (ret == 0) {
		_add_player(name);
	}

	return 0;
}

static int _gxplayer_stop(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	int opt, ret;
	unsigned char flags = 0;
	char *name = NULL;

	optind = 0;
	while ((opt = GetOpt(argc, argv, sStopStringOP)) != -1) {
		flags = 1;
		switch (opt) {
		case 'n':
			name = strdup(optarg);
			break;

		default:
			_gxplayer_stop_help();
			return 0;
		}
	}

	if (!flags) {
		_gxplayer_stop_help();
		return 0;
	}
	if (_check_player(name)) {
		Log_Printf(LOG_GXPLAYER, LOG_ERROR, "no %s player\n", name);
		return -1;
	}

	ret = GxPlayer_MediaStop(name);
	if (ret == 0) {
		_delete_player(name);
	}

	return 0;
}

static int _gxplayer_pause(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	int opt;
	unsigned char flags = 0;
	char *name = NULL;

	optind = 0;
	while ((opt = GetOpt(argc, argv, sPauseStringOP)) != -1) {
		flags = 1;
		switch (opt) {
		case 'n':
			name = strdup(optarg);
			break;

		default:
			_gxplayer_pause_help();
			return 0;
		}
	}

	if (!flags) {
		_gxplayer_pause_help();
		return 0;
	}

	if (_check_player(name)) {
		Log_Printf(LOG_GXPLAYER, LOG_ERROR, "no %s player\n", name);
		return -1;
	}

	GxPlayer_MediaPause(name);

	return 0;
}

static int _gxplayer_resume(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	int opt;
	unsigned char flags = 0;
	char *name = NULL;

	optind = 0;
	while ((opt = GetOpt(argc, argv, sResumeStringOP)) != -1) {
		flags = 1;
		switch (opt) {
		case 'n':
			name = strdup(optarg);
			break;

		default:
			_gxplayer_resume_help();
			return 0;
		}
	}

	if (!flags) {
		_gxplayer_resume_help();
		return 0;
	}

	if (_check_player(name)) {
		Log_Printf(LOG_GXPLAYER, LOG_ERROR, "no %s player\n", name);
		return -1;
	}
	GxPlayer_MediaResume(name);

	return 0;
}

static int _gxplayer_info(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	int opt, status = 0;
	unsigned char flags = 0;
	PlayerProgInfo prog_info;
	char *name = NULL;
	PlayerStatusInfo status_info;
	PlayTimeInfo time_info;

	optind = 0;
	while ((opt = GetOpt(argc, argv, sInfoStringOP)) != -1) {
		flags = 1;
		switch (opt) {
		case 'n':
			name = strdup(optarg);
			break;
		case 's':
			status = 1;
			break;
		default:
			_gxplayer_info_help();
			return 0;
		}
	}

	if (!flags) {
		_gxplayer_info_help();
		return 0;
	}
	if (_check_player(name)) {
		Log_Printf(LOG_GXPLAYER, LOG_ERROR, "no %s player\n", name);
		return -1;
	}

	GxPlayer_MediaGetStatus(name, &status_info);
	switch (status_info.status) {
	case PLAYER_STATUS_PLAY_START:
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "player start play\n");
		break;
	case PLAYER_STATUS_PLAY_PAUSE:
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "player is pausing\n");
		break;
	case PLAYER_STATUS_PLAY_RUNNING:
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "player is running\n");
		break;
	case PLAYER_STATUS_PLAY_END:
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "player end play\n");
		break;
	default:
		break;
	}

	GxPlayer_MediaGetTime(name, &time_info);
	Log_Printf(LOG_GXPLAYER, LOG_INFO, "  current time =  %lld s\t totle time   =  %lld s\n",
			time_info.current/1000, time_info.totle/1000);
	if (status) {
		unsigned int i = 0;
		GxPlayer_MediaGetProgInfoByName(name, &prog_info);
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "  url     =  %s\n", prog_info.url);
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "  size    =  %lld Byte\n",prog_info.file_size);

		Log_Printf(LOG_GXPLAYER, LOG_INFO, "  [VIDEO]\n");
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "  codec   =  %s\n", prog_info.video.codec_id);
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "  width   =  %d\n", prog_info.video.width);
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "  height  =  %d\n", prog_info.video.height);
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "  bitrate =  %d\n", prog_info.video.bitrate);
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "  fps     =  %.2f\n", prog_info.video.fps);

		Log_Printf(LOG_GXPLAYER, LOG_INFO, "  [AUDIO]\n");
		for (i = 0; i < prog_info.audio_num; i++) {
			Log_Printf(LOG_GXPLAYER, LOG_INFO, "    %d. codec: %s\n",i, prog_info.audio[i].codec_id);
			Log_Printf(LOG_GXPLAYER, LOG_INFO, "    %d. lang : %s\n",i, prog_info.audio[i].lang);
		}
		i = prog_info.cur_audio_id;
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "    cur track [%d]\n", prog_info.cur_audio_id);
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "      codec      =  %s\n", prog_info.audio[i].codec_id);
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "      bitreate   =  %d\n", prog_info.audio[i].bitrate);
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "      samplerate =  %d\n", prog_info.audio[i].samplerate);
		Log_Printf(LOG_GXPLAYER, LOG_INFO, "      channels   =  %d\n", prog_info.audio[i].channels);
	}

	return 0;
}

ShellCmd gGxPlayerPlay = {
	.name  = "play",
	.func  = _gxplayer_play,
	.brief = "play   [help]: gxplayer play   command",
};

ShellCmd gGxPlayerStop = {
	.name  = "stop",
	.func  = _gxplayer_stop,
	.brief = "stop   [help]: gxplayer stop   command",
};

ShellCmd gGxPlayerPause = {
	.name  = "pause",
	.func  = _gxplayer_pause,
	.brief = "pause  [help]: gxplayer pause  command",
};

ShellCmd gGxPlayerResume = {
	.name  = "resume",
	.func  = _gxplayer_resume,
	.brief = "resume [help]: gxplayer resume command",
};

ShellCmd gGxPlayerInfo = {
	.name  = "info",
	.func  = _gxplayer_info,
	.brief = "info   [help]: gxplayer info command",
};
