#include "app_config.h"
#if PCDEMO_TEST_ENABLE
#include <stddef.h>
#include <stdio.h>
#include "./include/module_device.h"
#include "./include/common_autotest.h"
#include "frontend/fe_property.h"

#define KEYWORD_PCDEMO_REG_WRITE_FLAG		"write"
#define KEYWORD_PCDEMO_REG_BASE			"base"
#define KEYWORD_PCDEMO_REG_REGISTER		"register"
#define KEYWORD_PCDEMO_REG_VALUE		"value"
#define KEYWORD_PCDEMO_REG_TYPE			"type"
#define KEYWORD_PCDEMO_REG_COUNT		"count"

static unsigned int sg_reg_data = 0;
static struct cmd_reg_params sg_reg_params;

static int _pcdemo_mode_parse_frontend_register_para(char *Command, void **ppInOutRegInfo, void **ppInOutInfo)
{
	int ret = 0;
	char *p = NULL;
	char Buffer[50] = {0};

	if((ppInOutRegInfo == NULL) || (Command == NULL))
	{
		app_log_error("\nTEST, error, %s, %d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	memset(&sg_reg_params, 0, sizeof(struct cmd_reg_params));

	//base addr
	p = strstr(Command, KEYWORD_PCDEMO_REG_BASE);
	if(p == NULL)
	{// DEFAULT BASE
		sg_reg_params.base = 0xF2;
	}
	else
	{// find the keyword
		memset(Buffer, 0, sizeof(Buffer));
		sscanf(p,"%*[^=]=%[^ ]", Buffer);
		sscanf(Buffer, "%x", &sg_reg_params.base);
	}

	if(sg_reg_params.base == 0)
		sg_reg_params.width = 1;
	else
		sg_reg_params.width = 2;

	//register addr
	p = strstr(Command, KEYWORD_PCDEMO_REG_REGISTER);
	if(p == NULL)
	{// DEFAULT reg
		app_log_error("[%s:%d] cannot find reg addr\n",__FUNCTION__,__LINE__);
		return -1;
	}
	else
	{// find the keyword
		memset(Buffer, 0, sizeof(Buffer));
		sscanf(p,"%*[^=]=%[^ ]", Buffer);
		sscanf(Buffer, "%x", &sg_reg_params.reg);
	}

	//type 0:repeat 1:master
	p = strstr(Command, KEYWORD_PCDEMO_REG_TYPE);
	if(p == NULL)
	{// DEFAULT type
		sg_reg_params.type = 0;
	}
	else
	{// find the keyword
		memset(Buffer, 0, sizeof(Buffer));
		sscanf(p,"%*[^=]=%[^ ]", Buffer);
		sscanf(Buffer, "%d", &sg_reg_params.type);
	}

	//count
	p = strstr(Command, KEYWORD_PCDEMO_REG_COUNT);
	if(p == NULL)
	{// DEFAULT count
		sg_reg_params.count = 1;
	}
	else
	{// find the keyword
		memset(Buffer, 0, sizeof(Buffer));
		sscanf(p,"%*[^=]=%[^ ]", Buffer);
		sscanf(Buffer, "%d", &sg_reg_params.count);
	}

	//whether need parse write value
	p = strstr(Command, KEYWORD_PCDEMO_REG_WRITE_FLAG);
	if(p != NULL)
	{
		//write value
		p = strstr(Command, KEYWORD_PCDEMO_REG_VALUE);
		if(p == NULL)
		{
			app_log_error("[%s:%d] cannot find write value\n",__FUNCTION__,__LINE__);
			return -1;
		}
		else
		{// find the keyword
			memset(Buffer, 0, sizeof(Buffer));
			sscanf(p,"%*[^=]=%[^ ]", Buffer);
			sscanf(Buffer, "%x", &sg_reg_data);
		}
	}

	sg_reg_params.data = &sg_reg_data;

	*ppInOutRegInfo = (void*)&(sg_reg_params);
	return ret;
}


static int pcdemo_mode_parse(char* Command, void **ppInOut1, void **ppInOut2)
{
extern dev_tbl_t dev_pcdemo_reg;
	int ret = 0;
	char Buffer[256] = {0};
	int i = 0;
	if((ppInOut1 == NULL) || (Command) == NULL)
	{
		app_log_error("\nTEST, error, %s, %d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	//start parse
	if((Command[0] < 'a') || (Command[0] > 'z'))
		sscanf(Command, "%*[^a-z]%s",Buffer);
	else
		sscanf(Command, "%s", Buffer);
	//sscanf(Command, "%s",Buffer);
	if(strlen(Buffer) == 0)
	{
		app_log_error("\nTEST, error, %s, %d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	for(i = 0; i < dev_pcdemo_reg.UserDefineFuncs.CmdNum; i++)
	{
		if(0 == memcmp(Buffer, dev_pcdemo_reg.UserDefineFuncs.Cmds[i].CommandName, strlen(dev_pcdemo_reg.UserDefineFuncs.Cmds[i].CommandName)))
		{// need to parse
			if(dev_pcdemo_reg.UserDefineFuncs.Cmds[i].ParseFunction)
			{
				ret = dev_pcdemo_reg.UserDefineFuncs.Cmds[i].ParseFunction(Command, ppInOut1, ppInOut2);
				if(ret < 0)
				{
					app_log_error("\nTEST, error, %s, %d\n",__FUNCTION__,__LINE__);
					return -1;
				}
				return 0;
			}
			break;
		}
	}
	return -1;
}

static int _pcdemo_mode_generate_url(char *pUrl, ProgInfo_t *pProgInfo)
{
	char Buffer[50] = {0};
	if((pUrl == NULL) || (pProgInfo == NULL))
	{
		app_log_error("\nTEST, error, %s, %d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	// head
	strcat(pUrl,"dvbs://fre:0&polar:0&symbol:0&22k:0");
	// vpid, apid, pcrpid
	sprintf(Buffer,"&vpid:%d&apid:%d&pcrpid:%d", pProgInfo->VideoPid,pProgInfo->AudioPid, pProgInfo->PCRPid);
	strcat(pUrl, Buffer);
	//vcodeformat, acodeformat
	memset(Buffer, 0, 50);
	sprintf(Buffer,"&vcodec:%d&acodec:%d", pProgInfo->VCodecFormat,pProgInfo->ACodecFormat);
	strcat(pUrl, Buffer);
	//demux parameter
	memset(Buffer, 0, 50);
	sprintf(Buffer,"&tuner:%d&tsid:%d&tsmode:%d&dmxid:%d",pProgInfo->TunerSelect,pProgInfo->TsSource,pProgInfo->TsMode,pProgInfo->DemuxId);
	strcat(pUrl, Buffer);
	app_log_debug("\nTEST, play url = %s, len = %d\n",pUrl,strlen(pUrl));
	return 0;
}

static int pcdemo_mode_play(void *pInfo)
{
	int ret = 0;
	GxMsgProperty_PlayerPlay play = {0};
	PlayerWindow VideoRect = {0, 0, 1280, 720};
	ProgInfo_t *pProgInfo = NULL;

	if(pInfo == NULL)
	{
		app_log_error("\nTEST, error, %s, %d\n",__FUNCTION__,__LINE__);
		return -1;
	}

	pProgInfo = (ProgInfo_t*)pInfo;
	play.player = PLAYER_NAME;
	memset(play.url, 0, PLAYER_URL_LONG);
	ret = _pcdemo_mode_generate_url(play.url, pProgInfo);
	if(ret < 0)
	{
		app_log_error("\nTEST, error, %s, %d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	memcpy(&play.window, &VideoRect, sizeof(PlayerWindow));

	GxPlayer_MediaPlay(play.player, play.url, 0, 0, NULL);
	return ret;
}


dev_tbl_t dev_pcdemo_reg = {
	.name       = "pcdemo",
	.lock_tp    = NULL,
	.play       = pcdemo_mode_play,
	.parse      = pcdemo_mode_parse,
	.UserDefineFuncs = {
		.CmdNum = 6,
		.Cmds   = {
			{.CommandName = "read_demod_reg",  .ParseFunction = _pcdemo_mode_parse_frontend_register_para,},
			{.CommandName = "write_demod_reg", .ParseFunction = _pcdemo_mode_parse_frontend_register_para,},
			{.CommandName = "read_tuner_reg",  .ParseFunction = _pcdemo_mode_parse_frontend_register_para,},
			{.CommandName = "write_tuner_reg", .ParseFunction = _pcdemo_mode_parse_frontend_register_para,},
			{.CommandName = "read_other_reg",  .ParseFunction = _pcdemo_mode_parse_frontend_register_para,},
			{.CommandName = "write_other_reg", .ParseFunction = _pcdemo_mode_parse_frontend_register_para,},
		},
	},

	.usage      = "function for pcdemo",
};

#endif


