

/*****************************************************************************
*                          CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2010, All right reserved
******************************************************************************

******************************************************************************
* File Name :   app_hdcp.c
* Author    :   glen
* Project   :   goxceed
* Type      :
******************************************************************************
* Purpose   :   模块头文件
******************************************************************************
* Release History:
VERSION   Date              AUTHOR         Description
1.0      2010.04.12         glen             creation
*****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app.h"

#if HDMI_HDCP_SUPPORT
#include <devapi/gxotp_api.h>
#include "module/hdmi/gxhdmi_module.h"
#include "av/hal/gxav_hal_hdmi.h"


#define HDCP_KEY_LEN 314

static uint8_t s_hdcp_time = 0; //try hdcp times 0--success
static uint8_t s_sd_flag=0; //hdcp fail,change to sd
#define HDCP_TEST_KEY
#ifdef HDCP_TEST_KEY
static unsigned char dpk_aksv[5] = {0x7a,0x54,0x20,0xd7,0xe9};
//          encrypt seed   byte1  0
static unsigned char sw_enc_key[2] = {0x12, 0x34};
static unsigned char dpk_keys[280] = {
//          //加密后的key 排序为
//          // key 0 : byte6    5     4    3   2   1   0
//          // key 1 : byte6    5     4    3   2   1   0
//          // ...
//          // key 39: byte6    5     4    3   2   1   0
         };
#endif


static int32_t app_flash_get_config_video_hdmi_mode(void)
{
	int32_t init_value = GX_VIDEO_OUTPUT_576I;
	 // tv standard
    GxBus_ConfigGetInt(TV_STANDARD_KEY, &init_value, TV_STANDARD);
	return init_value;
}

static int32_t app_play_set_hdmi_mode(int32_t vout_mode)
{
    int init_value = 0;
	GxMsgProperty_PlayerVideoModeConfig video_mode;

	GxBus_ConfigSetInt(TV_STANDARD_KEY, vout_mode);
	//HDMI - standard
    video_mode.interface = GX_VIDEO_OUTPUT_HDMI;
    video_mode.mode = vout_mode;
    app_send_msg_exec(GXMSG_PLAYER_VIDEO_MODE_CONFIG,&video_mode);
    GxBus_ConfigGetInt(AV_VIDEO_OUT_MODE, &init_value, VIDEO_OUT_MODE_DEFAULT_VALUE);
    app_av_set_videoout_mode(init_value);
	return 0;
}


static void init_hdcp_function(void)
{
	GxAvIOCTL_HdcpKeyRegister hdcp;
	//unsigned char hdcp_key[314] = { };
	 //TODO  read key

	 //memcpy(hdcp.ksv1, hdcp_key, 5);
	 hdcp.key_encrypt = 1;
	 //memcpy(hdcp.key_encrypt_seed, hdcp_key + 5, 2);
	 //memcpy(hdcp.keys, hdcp_key + 14, 280);
	 //memset(hdcp.keys, 0, 280);
#ifdef HDCP_TEST_KEY
	 memcpy(hdcp.ksv1, dpk_aksv, sizeof(dpk_aksv));
 	 memcpy(hdcp.ksv2, dpk_aksv, sizeof(dpk_aksv));
	 memcpy(hdcp.keys, dpk_keys, sizeof(dpk_keys));
	 memcpy(hdcp.key_encrypt_seed, sw_enc_key, sizeof(sw_enc_key));
	 memset(hdcp.hash, 0, sizeof(hdcp.hash));
#endif
	 app_log_flow("====%s, %d====", __func__, __LINE__);
	 GxVoutHAL_HdmiHdcpKeyRegister(&hdcp);// funtion v1.9.70

	 return;
}

//主线程
void app_hdcp_init(void)
{
	init_hdcp_function();
//hdcp function
	GxVoutHAL_HdmiSetHdcpEnable(1);
	return;
}



static void app_hdcp_thread(void *arg)
{
	int32_t flash_value = GX_VIDEO_OUTPUT_576I;

	while(1)
	{
		//app_log_debug(" <%s>  s_hdcp_time = %d\n",__FUNCTION__,s_hdcp_time);
		if(s_hdcp_time>5&&s_hdcp_time<=10)
		{
			flash_value = app_flash_get_config_video_hdmi_mode();
			if(flash_value != GX_VIDEO_OUTPUT_720P_50HZ)
				flash_value = GX_VIDEO_OUTPUT_720P_50HZ;
			else
				flash_value = GX_VIDEO_OUTPUT_1080I_50HZ;
			app_play_set_hdmi_mode(flash_value);
			GxVoutHAL_HdmiSetHdcpEnable(0);
			GxVoutHAL_HdmiSetHdcpEnable(1);
			app_log_info("\n------------------------------------\n");
			app_log_info("\n------[HDCP THREAD]-----HDCP reset-------------\n");
			app_log_info("\n------------------------------------\n");
			s_sd_flag = 0;
		}else if(s_hdcp_time>10){
			//set 576i
			if(s_sd_flag == 0){
				flash_value = GX_VIDEO_OUTPUT_576I;
				app_play_set_hdmi_mode(flash_value);
				GxVoutHAL_HdmiSetHdcpEnable(0);
				app_log_info("\n------------------------------------\n");
				app_log_info("\n------[HDCP THREAD]-----HDCP failed change SD-------------\n");
				app_log_info("\n------------------------------------\n");
				s_sd_flag = 1;
			}
		}else if(s_hdcp_time>150){
			s_hdcp_time = 11;
		}

		if(s_hdcp_time > 10) // hdcp success (0)  not Increment
			s_hdcp_time++;

		GxCore_ThreadDelay(3000);
	}
}

int app_hdcp_create_thread(void)
{
	handle_t app_hdcp;
	app_hdcp_init();
	GxCore_ThreadCreate("app_hdcp_thread", &app_hdcp, app_hdcp_thread,NULL, 16 * 1024, GXOS_DEFAULT_PRIORITY);

	return 0;
}


int app_hdcp_hdmi_plug_status(GxMessage* msg)
{
	switch(msg->msg_id)
	{
		case GXMSG_HDMI_HOTPLUG_IN:
		case GXMSG_HDMI_HOTPLUG_OUT:
		{
			app_log_info("[service_msg_to_app] GXMSG_HDMI_HOTPLUG_IN!\n");
			if(s_sd_flag > 0)
			{
				//recover hdmi mode
				app_play_set_hdmi_mode(app_flash_get_config_video_hdmi_mode());
				//reset hdcp
				GxVoutHAL_HdmiSetHdcpEnable(1);
				//recount
				s_sd_flag = 0;
				s_hdcp_time = 0;
			}
			return EVENT_TRANSFER_STOP;
		}
		case GXMSG_HDMI_HDCP_FAIL:
		{
			s_hdcp_time++;
			app_log_info("[service_msg_to_appyy] GXMSG_HDMI_HDCP_FAIL! time = %d\n",s_hdcp_time);
			//app_log_debug("[service_msg_to_app] GXMSG_HDMI_HDCP_FAIL!\n reg[5002]=%x\n, reg[5003]=%x\n, reg[5004]=%x\n, reg[5005]=%x\n, reg[5007]=%x\n",
			//		*(volatile unsigned*)(0xa4f00000+0x5002*4),*(volatile unsigned*)(0xa4f00000+0x5003*4)
			//		,*(volatile unsigned*)(0xa4f00000+0x5004*4),*(volatile unsigned*)(0xa4f00000+0x5005*5),*(volatile unsigned*)(0xa4f00000+0x5007*5));
			if(s_hdcp_time<=5)
			{
				GxVoutHAL_HdmiSetHdcpEnable(0);
				GxVoutHAL_HdmiSetHdcpEnable(1);
			}
			return EVENT_TRANSFER_STOP;
		}
		case GXMSG_HDMI_HDCP_SUCCESS:
		{
			s_hdcp_time = 0;
			app_log_info("[service_msg_to_app] GXMSG_HDMI_HDCP_SUCCESS!\n");
			return EVENT_TRANSFER_STOP;
		}
        default:
			break;
	}

	return EVENT_TRANSFER_STOP;
}
#endif


