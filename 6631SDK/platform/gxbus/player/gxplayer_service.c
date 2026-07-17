#include "gxbus.h"
#include "gxmsg.h"
#include "gx_common.h"
#include "../include/service/gxplayer.h"
#include "../include/module/config/gxconfig.h"
#include "../include/gxavdev.h"
#include "../include/gxdebug.h"
#include "gxplayer_internal.h"

#ifndef I386_LINUX

static const char*  msg_print[] = {
	"GXMSG_PLAYER_PLAY",
	"GXMSG_PLAYER_STOP",
	"GXMSG_PLAYER_PAUSE",
	"GXMSG_PLAYER_RESUME",
	"GXMSG_PLAYER_SEEK",
	"GXMSG_PLAYER_SPEED",
	"GXMSG_PLAYER_RECORD",
	"GXMSG_PLAYER_VIDEO_COLOR",
	"GXMSG_PLAYER_VIDEO_CLIP",
	"GXMSG_PLAYER_VIDEO_WINDOW",
	"GXMSG_PLAYER_VIDEO_ASPECT",
	"GXMSG_PLAYER_VIDEO_INTERFACE",
	"GXMSG_PLAYER_VIDEO_MODE_CONFIG",
	"GXMSG_PLAYER_VIDEO_OUTPUT_DEF",
	"GXMSG_PLAYER_VIDEO_OUTPUT_POWER",
	"GXMSG_PLAYER_VIDEO_AUTO_ADAPT",
	"GXMSG_PLAYER_VIDEO_HIDE",
	"GXMSG_PLAYER_VIDEO_SHOW",
	"GXMSG_PLAYER_VIDEO_MOSAIC_DROP",
	"GXMSG_PLAYER_VIDEO_USERDATA_CONFIG",
	"GXMSG_PLAYER_AUDIO_DOWNMIX",
	"GXMSG_PLAYER_AUDIO_SYNC",
	"GXMSG_PLAYER_AUDIO_MUTE",
	"GXMSG_PLAYER_AUDIO_VOLUME",
	"GXMSG_PLAYER_AUDIO_TRACK",
    "GXMSG_PLAYER_AUDIO_DESCRIPTOR_TRACK" ,
	"GXMSG_PLAYER_AUDIO_SWITCH",
	"GXMSG_PLAYER_AUDIO_INTERFACE",
	"GXMSG_PLAYER_AUDIO_MODE_CONFIG",
	"GXMSG_PLAYER_AUDIO_AC3_MODE",
	"GXMSG_PLAYER_AUDIO_AAC_MODE",
	"GXMSG_PLAYER_SUBTITLE_SYNC",
	"GXMSG_PLAYER_SUBTITLE_LOAD",
	"GXMSG_PLAYER_SUBTITLE_UNLOAD",
	"GXMSG_PLAYER_SUBTITLE_SWITCH",
	"GXMSG_PLAYER_SUBTITLE_HIDE",
	"GXMSG_PLAYER_SUBTITLE_SHOW",
	"GXMSG_PLAYER_SUBTITLE_RESOLUTION",
	"GXMSG_PLAYER_PLAY_CONFIG",
	"GXMSG_PLAYER_DELAY_CONFIG",
	"GXMSG_PLAYER_PVR_CONFIG",
	"GXMSG_PLAYER_PLAY_PVR_CONFIG",
	"GXMSG_PLAYER_RECORD_CONFIG",
	"GXMSG_PLAYER_DISPLAY_SCREEN",
	"GXMSG_PLAYER_TIME_SHIFT",
	"GXMSG_PLAYER_MULTISCREEN_PLAY",
	"GXMSG_PLAYER_FREEZE_FRAME_SWITCH",
	"GXMSG_PLAYER_STATUS_REPORT",
	"GXMSG_PLAYER_AVCODEC_REPORT",
	"GXMSG_PLAYER_SPEED_REPORT",
	"GXMSG_PLAYER_RESOLUTION_REPORT",
	"GXMSG_PLAYER_RECORD_OVERFLOW",
	"GXMSG_PLAYER_PROCESS_FINISH",
	"GXMSG_PLAYER_VIDEO_DYNAMICRANGE_CHANGED",
	"GXMSG_PLAYER_TRACK_ADD",
	"GXMSG_PLAYER_TRACK_DEL",
	"GXMSG_PLAYER_BANDWIDTH_SWITCH",
	"GXMSG_PLAYER_AD_AUDIO_ENABLE",
	"GXMSG_PLAYER_AD_AUDIO_DISABLE",
	"GXMSG_PLAYER_AUDIO_DELAY",
	"GXMSG_PLAYER_VIDEO_DELAY",
};

typedef struct {
	int msg;
	int size;
}MsgBlock;


static MsgBlock msg_recv[] =  {
	{GXMSG_PLAYER_PLAY                 ,sizeof(GxMsgProperty_PlayerPlay)},
	{GXMSG_PLAYER_STOP                 ,sizeof(GxMsgProperty_PlayerStop)},
	{GXMSG_PLAYER_PAUSE                ,sizeof(GxMsgProperty_PlayerPause)},
	{GXMSG_PLAYER_RESUME               ,sizeof(GxMsgProperty_PlayerResume)},
	{GXMSG_PLAYER_SEEK                 ,sizeof(GxMsgProperty_PlayerSeek)},
	{GXMSG_PLAYER_SPEED                ,sizeof(GxMsgProperty_PlayerSpeed)},
	{GXMSG_PLAYER_RECORD               ,sizeof(GxMsgProperty_PlayerRecord)},
	{GXMSG_PLAYER_VIDEO_CLIP           ,sizeof(GxMsgProperty_PlayerVideoClip)},
	{GXMSG_PLAYER_VIDEO_WINDOW         ,sizeof(GxMsgProperty_PlayerVideoWindow)},
	{GXMSG_PLAYER_VIDEO_COLOR          ,sizeof(GxMsgProperty_PlayerVideoColor)},
	{GXMSG_PLAYER_VIDEO_ASPECT         ,sizeof(GxMsgProperty_PlayerVideoAspect)},
	{GXMSG_PLAYER_VIDEO_INTERFACE      ,sizeof(GxMsgProperty_PlayerVideoInterface)},
	{GXMSG_PLAYER_VIDEO_MODE_CONFIG    ,sizeof(GxMsgProperty_PlayerVideoModeConfig)},
	{GXMSG_PLAYER_VIDEO_OUTPUT_DEF	   ,sizeof(GxMsgProperty_PlayerVideoOutputDef)},
	{GXMSG_PLAYER_VIDEO_OUTPUT_POWER   ,sizeof(GxMsgProperty_PlayerVideoOutputPower)},
	{GXMSG_PLAYER_VIDEO_AUTO_ADAPT     ,sizeof(GxMsgProperty_PlayerVideoAutoAdapt)},
	{GXMSG_PLAYER_VIDEO_HIDE           ,sizeof(GxMsgProperty_PlayerVideoHide)},
	{GXMSG_PLAYER_VIDEO_SHOW           ,sizeof(GxMsgProperty_PlayerVideoShow)},
	{GXMSG_PLAYER_VIDEO_MOSAIC_DROP    ,sizeof(GxMsgProperty_PlayerVideoMosaicDrop)},
	{GXMSG_PLAYER_VIDEO_USERDATA_CONFIG,sizeof(GxMsgProperty_PlayerVideoUserdataConfig)},
	{GXMSG_PLAYER_AUDIO_DOWNMIX        ,sizeof(GxMsgProperty_PlayerAudioDownMix)},
	{GXMSG_PLAYER_AUDIO_SYNC           ,sizeof(GxMsgProperty_PlayerAudioSync)},
	{GXMSG_PLAYER_AUDIO_MUTE           ,sizeof(GxMsgProperty_PlayerAudioMute)},
	{GXMSG_PLAYER_AUDIO_VOLUME         ,sizeof(GxMsgProperty_PlayerAudioVolume)},
	{GXMSG_PLAYER_AUDIO_TRACK          ,sizeof(GxMsgProperty_PlayerAudioTrack)},
	{GXMSG_PLAYER_AUDIO_DESCRIPTOR_TRACK     ,sizeof(GxMsgProperty_PlayerAudioDescriptorTrack)},
	{GXMSG_PLAYER_AUDIO_SWITCH         ,sizeof(GxMsgProperty_PlayerAudioSwitch)},
	{GXMSG_PLAYER_AUDIO_INTERFACE	   ,sizeof(GxMsgProperty_PlayerAudioInterface)},
	{GXMSG_PLAYER_AUDIO_MODE_CONFIG    ,sizeof(GxMsgProperty_PlayerAudioModeConfig)},
	{GXMSG_PLAYER_AUDIO_AC3_MODE       ,sizeof(GxMsgProperty_PlayerAudioAC3Mode)},
	{GXMSG_PLAYER_AUDIO_AAC_MODE       ,sizeof(GxMsgProperty_PlayerAudioAACMode)},
	{GXMSG_PLAYER_TIME_SHIFT           ,sizeof(GxMsgProperty_PlayerTimeShift)},
	{GXMSG_PLAYER_MULTISCREEN_PLAY     ,sizeof(GxMsgProperty_PlayerMultiScreenPlay)},
	{GXMSG_PLAYER_FREEZE_FRAME_SWITCH  ,sizeof(GxMsgProperty_PlayerFreezeFrameSwitch)},
	{GXMSG_PLAYER_SUBTITLE_LOAD        ,sizeof(GxMsgProperty_PlayerSubtitleLoad)},
	{GXMSG_PLAYER_SUBTITLE_UNLOAD      ,sizeof(GxMsgProperty_PlayerSubtitleUnLoad)},
	{GXMSG_PLAYER_SUBTITLE_SWITCH      ,sizeof(GxMsgProperty_PlayerSubtitleSwitch)},
	{GXMSG_PLAYER_SUBTITLE_SYNC        ,sizeof(GxMsgProperty_PlayerSubtitleSync)},
	{GXMSG_PLAYER_SUBTITLE_HIDE        ,sizeof(GxMsgProperty_PlayerSubtitleHide)},
	{GXMSG_PLAYER_SUBTITLE_SHOW        ,sizeof(GxMsgProperty_PlayerSubtitleShow)},
	{GXMSG_PLAYER_SUBTITLE_RESOLUTION  ,sizeof(GxMsgProperty_PlayerSubtitleResolution)},
	{GXMSG_PLAYER_DISPLAY_SCREEN       ,sizeof(GxMsgProperty_PlayerDisplayScreen)},
	{GXMSG_PLAYER_PLAY_CONFIG          ,sizeof(GxMsgProperty_PlayerPlayConfig)},
	{GXMSG_PLAYER_DELAY_CONFIG         ,sizeof(GxMsgProperty_PlayerDelayConfig)},
	{GXMSG_PLAYER_PVR_CONFIG           ,sizeof(GxMsgProperty_PlayerPVRConfig)},
	{GXMSG_PLAYER_PLAY_PVR_CONFIG      ,sizeof(GxMsgProperty_PlayerPlayPVRConfig)},
	{GXMSG_PLAYER_RECORD_CONFIG        ,sizeof(GxMsgProperty_PlayerRecordConfig)},
	{GXMSG_PLAYER_TRACK_ADD            ,sizeof(GxMsgProperty_PlayerTrackAdd)},
	{GXMSG_PLAYER_TRACK_DEL            ,sizeof(GxMsgProperty_PlayerTrackDel)},
	{GXMSG_PLAYER_BANDWIDTH_SWITCH     ,sizeof(GxMsgProperty_PlayerBandwidthSwitch)},
	{GXMSG_PLAYER_AD_AUDIO_ENABLE      ,sizeof(GxMsgProperty_PlayerAdAudio)},
	{GXMSG_PLAYER_AD_AUDIO_DISABLE     ,sizeof(GxMsgProperty_PlayerAdAudio)},
	{GXMSG_PLAYER_AUDIO_DELAY          ,sizeof(GxMsgProperty_PlayerAudioDelay)},
	{GXMSG_PLAYER_VIDEO_DELAY          ,sizeof(GxMsgProperty_PlayerVideoDelay)},
};

static MsgBlock msg_send[] = {
	{GXMSG_PLAYER_STATUS_REPORT        ,sizeof(GxMsgProperty_PlayerStatusReport)},
	{GXMSG_PLAYER_AVCODEC_REPORT       ,sizeof(GxMsgProperty_PlayerAVCodecReport)},
	{GXMSG_PLAYER_SPEED_REPORT         ,sizeof(GxMsgProperty_PlayerSpeedReport)},
	{GXMSG_PLAYER_RESOLUTION_REPORT    ,sizeof(GxMsgProperty_PlayerResolutionReport)},
	{GXMSG_PLAYER_RECORD_OVERFLOW      ,sizeof(GxMsgProperty_PlayerRecordOverflow)},
	{GXMSG_PLAYER_PROCESS_FINISH       ,sizeof(GxMsgProperty_PlayerProcessFinish)},
	{GXMSG_PLAYER_VIDEO_DYNAMICRANGE_CHANGED, sizeof(GxMsgProperty_PlayerVideoDynamicRangeChanged)},
} ;

#define DEF_PLAY_NAME        "DeF_PlAyEr_NaMe"
#define DEF_PLAY_URL         "DeF_PlAyEr_UrL"

#define SERVICE_GET_MSG_PARAM(type)                  \
	type* param = GxBus_GetMsgPropertyPtr(msg,type); \
if(param == NULL) return GXCORE_ERROR;

#define EVENT_REPORT(msgid,args,type)                             \
	do{                                                           \
		GxMessage* new_msg = NULL;                                \
		type* param ;                                             \
		\
		new_msg = GxBus_MessageNew(msgid);                        \
		if(new_msg)                                               \
		{                                                         \
			param = (type*)GxBus_GetMsgPropertyPtr(new_msg,type); \
			if(param)                                             \
			{                                                     \
				memcpy(param,args,sizeof(type));                  \
				GxBus_MessageSend(new_msg);                       \
			}                                                     \
			else                                                  \
			{                                                     \
				gxloge("[Player]: Msgid And Type Do Not Match\n");\
			}                                                     \
		}                                                         \
	}while(0);

static void _service_event_report(PlayerEventID event,void* args)
{
	switch(event)
	{
		case PLAYER_EVENT_STATUS_REPORT:
			EVENT_REPORT(GXMSG_PLAYER_STATUS_REPORT, args, GxMsgProperty_PlayerStatusReport);
			break;
		case PLAYER_EVENT_AVCODEC_REPORT:
			EVENT_REPORT(GXMSG_PLAYER_AVCODEC_REPORT, args, GxMsgProperty_PlayerAVCodecReport);
			break;
		case PLAYER_EVENT_SPEED_REPORT:
			EVENT_REPORT(GXMSG_PLAYER_SPEED_REPORT, args, GxMsgProperty_PlayerSpeedReport);
			break;
		case PLAYER_EVENT_RESOLUTION_REPORT:
			EVENT_REPORT(GXMSG_PLAYER_RESOLUTION_REPORT, args, GxMsgProperty_PlayerResolutionReport);
			break;
		case PLAYER_EVENT_RECORD_OVERFLOW:
			EVENT_REPORT(GXMSG_PLAYER_RECORD_OVERFLOW, args, GxMsgProperty_PlayerRecordOverflow);
			break;
		case PLAYER_EVENT_DYNAMIC_RANGE_CHANGED:
			EVENT_REPORT(GXMSG_PLAYER_VIDEO_DYNAMICRANGE_CHANGED, args, GxMsgProperty_PlayerVideoDynamicRangeChanged);
			break;
		default:
			break;
	}
}

static void _player_autoplay(void)
{
	int autoplay,radioflag;

	GxBus_ConfigGetInt(PLAYER_CONFIG_AUTOPLAY_PLAY_FLAG, &autoplay, 0);
	GxBus_ConfigGetInt(PLAYER_CONFIG_AUTOPLAY_RADIO_FLAG, &radioflag, 0);

	if(autoplay) {
		int volume;
		DisplayScreen Screen;
		PlayerWindow Window;
		char *url, name[PLAYER_NAME_LONG];
		GxPlayer_SystemGet(PSYS_AOUT_VOLUME, &volume);
		GxPlayer_SystemGet(PSYS_VOUT_SCREEN, &Screen);

		Window.x = Window.y = 0;
		Window.width = Screen.xres;
		Window.height = Screen.yres;

		if ((url = av_mallocz(PLAYER_URL_LONG+1)) == NULL)
			return;
		GxBus_ConfigGet(PLAYER_CONFIG_AUTOPLAY_VIDEO_NAME,name, PLAYER_NAME_LONG, DEF_PLAY_NAME);
		GxBus_ConfigGet(PLAYER_CONFIG_AUTOPLAY_VIDEO_URL,url, PLAYER_URL_LONG, DEF_PLAY_URL);
		if(strcmp(name, DEF_PLAY_NAME) && strcmp(url, DEF_PLAY_URL)) {
			gxlogf("[Player]: Auto Play : name = %s \n",name);
			gxlogf("[Player]: Auto Play : url  = %s \n",url);
			GxPlayer_MediaPlay(name, url, 0, volume, &Window);
		}
		if(radioflag) {
			GxBus_ConfigGet(PLAYER_CONFIG_AUTOPLAY_RADIO_NAME, name, PLAYER_NAME_LONG, DEF_PLAY_NAME);
			GxBus_ConfigGet(PLAYER_CONFIG_AUTOPLAY_RADIO_URL, url, PLAYER_URL_LONG, DEF_PLAY_URL);
			if(strcmp(name, DEF_PLAY_NAME) && strcmp(url, DEF_PLAY_URL)) {
				gxlogf("[Player]: Auto Play : name = %s \n",name);
				gxlogf("[Player]: Auto Play : url  = %s \n",url);
				GxPlayer_MediaPlay(name, url, 0, volume, &Window);
			}
		}
		av_free(url);
	}
}

status_t GxPlayerServiceInit(handle_t self,int priority_offset)
{
	int i;
	handle_t sch;

	for(i=0;i<sizeof(msg_send)/sizeof(MsgBlock);i++){
		GxBus_MessageRegister(msg_send[i].msg,msg_send[i].size);
	}

	for(i=0;i<sizeof(msg_recv)/sizeof(MsgBlock);i++){
		GxBus_MessageRegister(msg_recv[i].msg,msg_recv[i].size);
		GxBus_MessageListen(self,msg_recv[i].msg);
	}


	GxPlayer_ModuleInit();
	GxPlayer_SetEventCallback(_service_event_report);

	_player_autoplay();

	sch = GxBus_SchedulerCreate("PlayerMsgScheduler", GXBUS_SCHED_MSG, 32*1024, GXOS_DEFAULT_PRIORITY-1+priority_offset);
	GxBus_ServiceLink(self, sch);
//	sch = GxBus_SchedulerCreate("PlayerConsoleScheduler", GXBUS_SCHED_CONSOLE, 1024*100, GXOS_DEFAULT_PRIORITY-1+priority_offset);
//	GxBus_ServiceLink(self, sch);

	return GXCORE_SUCCESS;
}

void  GxPlayerServiceDestroy(handle_t self)
{
	int i;

	for(i=0;i<sizeof(msg_recv)/sizeof(MsgBlock);i++){
		GxBus_MessageUnListen(self,msg_recv[i].msg);
		GxBus_MessageUnregister(msg_recv[i].msg);
	}

	for(i=0;i<sizeof(msg_send)/sizeof(MsgBlock);i++){
		GxBus_MessageUnregister(msg_send[i].msg);
	}

	GxPlayer_ModuleDestroy();

	GxBus_ServiceUnlink(self);
}

GxMsgStatus GxPlayerServiceRecvMsg(handle_t self, GxMessage*  msg)
{
	const char* player = NULL;
	GxMsgStatus ret = GXCORE_SUCCESS;

	if(msg == NULL)
		return -1;

	gxlogf("[Player]: Recv Msg[%-2d] : %s\n",msg->msg_id,msg_print[msg->msg_id]);

	switch(msg->msg_id){
		case GXMSG_PLAYER_PLAY:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerPlay);
				GxPlayer_MediaPlay(param->player ,(const char*)param->url,param->start,param->volume,&param->window);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_RECORD:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerRecord);
				ret = GxPlayer_MediaRecord(param->player ,(const char*)param->url,(const char*)param->file);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_STOP:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerStop);
				ret = GxPlayer_MediaStop(param->player);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_PAUSE:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerPause);
				ret = GxPlayer_MediaPause(param->player);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_RESUME:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerResume);
				ret = GxPlayer_MediaResume(param->player);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_TRACK_ADD:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerTrackAdd);
				ret = GxPlayer_MediaTrackAdd(param->player, &param->track);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_TRACK_DEL:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerTrackDel);
				ret = GxPlayer_MediaTrackDel(param->player, &param->track);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_BANDWIDTH_SWITCH:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerBandwidthSwitch);
				ret = GxPlayer_MediaBandwidthSwitch(param->player, param->index);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_AD_AUDIO_ENABLE:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerAdAudio);
				ret = GxPlayer_MediaAdAudioEnable(param->player, param->apid, param->atype, param->admxid);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_AD_AUDIO_DISABLE:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerAdAudio);
				ret = GxPlayer_MediaAdAudioDisable(param->player);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_AUDIO_DELAY:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerAudioDelay);
				ret = GxPlayer_MediaAudioDelayMs(param->player, param->delayms);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_VIDEO_DELAY:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerVideoDelay);
				ret = GxPlayer_MediaVideoDelayMs(param->player, param->delayms);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_SEEK:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerSeek);
				ret = GxPlayer_MediaSeek(param->player,param->time,param->flag);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_SPEED:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerSpeed);
				ret = GxPlayer_MediaSpeed(param->player,param->speed);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_AUDIO_SYNC:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerAudioSync);
				ret = GxPlayer_MediaAudioSync(param->player,param->timems);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_AUDIO_MUTE:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerAudioMute);
				ret = GxPlayer_SetAudioMute(*param);
				break;
			}

		case GXMSG_PLAYER_AUDIO_VOLUME:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerAudioVolume);
				ret = GxPlayer_SetAudioVolume(*param);
				break;
			}

		case GXMSG_PLAYER_AUDIO_TRACK:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerAudioTrack);
				ret = GxPlayer_SetAudioTrack(*param);
				break;
			}

		case GXMSG_PLAYER_AUDIO_DESCRIPTOR_TRACK:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerAudioDescriptorTrack);
				ret = GxPlayer_SetAudioDescriptorTrack(*param);
				break;
			}

		case GXMSG_PLAYER_AUDIO_SWITCH:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerAudioSwitch);
				ret = GxPlayer_MediaAudioSwitch(param->player, param->pid, param->type, param->url);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_AUDIO_INTERFACE:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerAudioInterface);
				ret = GxPlayer_SetAudioOutputSelect(*param);
				break;
			}

		case GXMSG_PLAYER_AUDIO_MODE_CONFIG:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerAudioModeConfig);
				ret = GxPlayer_SetAudioOutputConfig(*param);
				break;
			}

		case GXMSG_PLAYER_AUDIO_AC3_MODE:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerAudioAC3Mode);
				ret = GxPlayer_SetAudioAC3Mode(param->mode);
				break;
			}

		case GXMSG_PLAYER_AUDIO_AAC_MODE:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerAudioAACMode);
				ret = GxPlayer_SetAudioAACMode(param->mode);
				break;
			}

		case GXMSG_PLAYER_AUDIO_DOWNMIX:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerAudioDownMix);
				ret = GxPlayer_SetAudioDownMix(param->enable);
				break;
			}

		case GXMSG_PLAYER_VIDEO_MOSAIC_DROP:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerVideoMosaicDrop);
				ret = GxPlayer_SetVideoMosaicDrop(param->enable);
				break;
			}

		case GXMSG_PLAYER_VIDEO_USERDATA_CONFIG:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerVideoUserdataConfig);
				ret = GxPlayer_SetVideoUserdataConfig(param);
				break;
			}

		case GXMSG_PLAYER_VIDEO_COLOR:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerVideoColor);
				ret = GxPlayer_SetVideoColors(*param);
				break;
			}

		case GXMSG_PLAYER_VIDEO_HIDE:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerVideoHide);
				ret = GxPlayer_MediaVideoHide(param->player);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_VIDEO_SHOW:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerVideoShow);
				ret = GxPlayer_MediaVideoShow(param->player);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_VIDEO_WINDOW:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerVideoWindow);
				ret = GxPlayer_MediaWindow(param->player,&param->rect);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_VIDEO_CLIP:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerVideoClip);
				ret = GxPlayer_MediaClip(param->player,&param->rect);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_VIDEO_ASPECT:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerVideoAspect);
				ret = GxPlayer_SetVideoAspect(*param);
				break;
			}

		case GXMSG_PLAYER_VIDEO_INTERFACE:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerVideoInterface);
				ret = GxPlayer_SetVideoOutputSelect(*param);
				break;
			}

		case GXMSG_PLAYER_VIDEO_MODE_CONFIG:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerVideoModeConfig);
				ret = GxPlayer_SetVideoOutputConfig(*param);
				break;
			}

		case GXMSG_PLAYER_VIDEO_OUTPUT_DEF:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerVideoOutputDef);
				ret = GxPlayer_SetVideoOutputDef(*param);
				break;
			}

		case GXMSG_PLAYER_VIDEO_OUTPUT_POWER:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerVideoOutputPower);
				ret = GxPlayer_SetVideoOutputPower(*param);
				break;
			}

		case GXMSG_PLAYER_FREEZE_FRAME_SWITCH:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerFreezeFrameSwitch);
				ret = GxPlayer_SetFreezeFrameSwitch(*param);
				break;
			}

		case GXMSG_PLAYER_TIME_SHIFT:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerTimeShift);
				ret = GxPlayer_SetTimeShiftConfig(param);
				break;
			}

		case GXMSG_PLAYER_SUBTITLE_LOAD:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerSubtitleLoad);
				param->sub = GxPlayer_MediaSubLoad(param->player,&param->para);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_SUBTITLE_UNLOAD:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerSubtitleUnLoad);
				ret = GxPlayer_MediaSubUnLoad(param->player,param->sub);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_SUBTITLE_SWITCH:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerSubtitleSwitch);
				ret = GxPlayer_MediaSubSwitch(param->player,param->sub,param->pid);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_SUBTITLE_SYNC:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerSubtitleSync);
				ret = GxPlayer_MediaSubSync(param->player,param->sub,param->timems);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_SUBTITLE_HIDE:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerSubtitleSync);
				ret = GxPlayer_MediaSubHide(param->player,param->sub);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_SUBTITLE_SHOW:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerSubtitleSync);
				ret = GxPlayer_MediaSubShow(param->player,param->sub);
				player = param->player;
				break;
			}

		case GXMSG_PLAYER_DISPLAY_SCREEN:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerDisplayScreen);
				ret = GxPlayer_SetDisplayScreen(*param);
				break;
			}

		case GXMSG_PLAYER_PLAY_CONFIG:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerPlayConfig);
				ret = GxPlayer_MediaPlayConfig(param->player, &param->config);
				break;
			}

		case GXMSG_PLAYER_DELAY_CONFIG:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerDelayConfig);
				ret = GxPlayer_MediaDelayConfig(param->player, &param->config);
				break;
			}

		case GXMSG_PLAYER_PVR_CONFIG:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerPVRConfig);
				ret = GxPlayer_SetPVRConfig(param);
				break;
			}
		case GXMSG_PLAYER_PLAY_PVR_CONFIG:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerPlayPVRConfig);
				ret = GxPlayer_SetPlayPVRConfig(param);
				break;
			}

		case GXMSG_PLAYER_RECORD_CONFIG:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerRecordConfig);
				ret = GxPlayer_MediaRecordConfig(param->player, &param->config);
				break;
			}

		case GXMSG_PLAYER_VIDEO_AUTO_ADAPT:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerVideoAutoAdapt);
				ret = GxPlayer_SetVideoAutoAdapt(param);
				break;
			}

		case GXMSG_PLAYER_SUBTITLE_RESOLUTION:
			{
				SERVICE_GET_MSG_PARAM(GxMsgProperty_PlayerSubtitleResolution);
				ret = GxPlayer_MediaSubResolution(param->player, param->sub, param->res);
				player = param->player;
				break;
			}

		default:
			ret = GXCORE_ERROR;
			break;
	}

	ret = (ret==GXCORE_SUCCESS ? GXMSG_OK : -1);

	gxlogf("[Player]: End Msg[%-2d] : %s\n",msg->msg_id,msg_print[msg->msg_id]);

	if(1)
	{
		GxMsgProperty_PlayerProcessFinish param1;

		memset(&param1,0,sizeof(GxMsgProperty_PlayerProcessFinish));
		param1.msg = msg->msg_id;
		param1.ret = ret;
		if(player)
			strncpy(param1.player,player,PLAYER_NAME_LONG);

		EVENT_REPORT(GXMSG_PLAYER_PROCESS_FINISH,&param1,GxMsgProperty_PlayerProcessFinish);
	}

	return ret;
}


GxServiceClass player_service = {
	"Player service",
	GxPlayerServiceInit,
	GxPlayerServiceDestroy,
	GxPlayerServiceRecvMsg,
	NULL,
	.priority_offset = 0,
};

#endif
