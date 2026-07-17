#include "app.h"
#if HDMI_CEC_SUPPORT
#include "app_cec.h"
#include "app_cec_config.h"
#include "app_cec_private.h"
#include "app_cec_message.h"
#include "app_cec_operation.h"
#include "app_cec_message_rx.h"
#include <common/gxpm.h>
#include "module/app_time.h"

#define CEC_MSG_SIMULATION_TEST  (1)
#if CEC_MSG_SIMULATION_TEST
static bool g_cec_msg_test_chage = false;

extern int app_hdmi_cec_msg_ring_buffer(GxHdmiCecMessage *msg);

static int app_cec_get_digital_service_Info(P_DIGITAL_SERVICE_ID digital_service_info)
{
	unsigned int channel_number = 0;
	unsigned int channel_position = 0;
	unsigned char service_id_method = 0;
	unsigned char channel_num_format = 0;
	unsigned char digital_broadcast_system = 0;
	P_DIGITAL_SERVICE_ID pDevInfo = {0};
	GxMsgProperty_NodeByPosGet node_prog = {0};

#if (DEMOD_DVB_S > 0)||(DEMOD_DVB_COMBO > 0)||(DOUBLE_S2_SUPPORT > 0)
    digital_broadcast_system = BCAST_DVB_S2;
#elif (DEMOD_DVB_C > 0)
    digital_broadcast_system = BCAST_DVB_C;
#elif (DEMOD_DVB_T > 0)
    digital_broadcast_system = BCAST_DVB_T;
#else
    digital_broadcast_system = BCAST_DVB_GENERIC;
#endif
    channel_position	= g_AppPlayOps.normal_play.play_count;
	channel_number =	channel_position + 1;
	node_prog.node_type = NODE_PROG;
    node_prog.pos = channel_position;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
	service_id_method = app_cec_get_service_id_method();
	channel_num_format =	app_cec_get_channel_number_format();
	pDevInfo = (P_DIGITAL_SERVICE_ID)digital_service_info;
	pDevInfo->service_id_method = service_id_method;
	pDevInfo->broadcast_system = digital_broadcast_system;
	if(!pDevInfo->service_id_method)
	{
		if(channel_num_format == CHANNEL_NUM_1_PART)
		{
			pDevInfo->service_id.channel.reserved = 0;
			pDevInfo->service_id.channel.major_ch_num = 0;
			pDevInfo->service_id.channel.min_ch_num = channel_number&0xFFFF;
			pDevInfo->service_id.channel.ch_numr_format = CHANNEL_NUM_1_PART;
		}
		else if(channel_num_format == CHANNEL_NUM_2_PART)
		{
            pDevInfo->service_id.channel.reserved = 0;
			pDevInfo->service_id.channel.min_ch_num = 0;
			pDevInfo->service_id.channel.major_ch_num = channel_number&0xFFFF;
			pDevInfo->service_id.channel.ch_numr_format = CHANNEL_NUM_2_PART;
		}
	}
	else
	{
		switch(pDevInfo->broadcast_system)
		{
			case BCAST_ARIB_T:
			case BCAST_ARIB_BS:
			case BCAST_ARIB_CS:
			case BCAST_ARIB_GENERIC:
				pDevInfo->service_id.arib.service_id = node_prog.prog_data.service_id;
				pDevInfo->service_id.arib.transport_stream_id = node_prog.prog_data.ts_id;
				pDevInfo->service_id.arib.original_network_id = node_prog.prog_data.original_id;
			break;
			case BCAST_ATSC_C:
			case BCAST_ATSC_T:
			case BCAST_ATSC_S:
			case BCAST_ATSC_GENERIC:
				pDevInfo->service_id.atsc.program_number = node_prog.prog_data.service_id;
				pDevInfo->service_id.atsc.transport_stream_id = node_prog.prog_data.ts_id;
			break;
			case BCAST_DVB_C:
			case BCAST_DVB_T:
			case BCAST_DVB_S:
			case BCAST_DVB_S2:
			case BCAST_DVB_GENERIC:
				pDevInfo->service_id.dvb.service_id	= node_prog.prog_data.service_id;
				pDevInfo->service_id.dvb.transport_stream_id	 = node_prog.prog_data.ts_id;
				pDevInfo->service_id.dvb.original_network_id	 = node_prog.prog_data.original_id;			
			break;
		}
	}
	return 0;
}

static int app_cec_msg_opcode_test(E_CEC_OPCODE opcode)
{
    time_t time = 0;
	time_t temp_sec = 0;
    struct tm temp_tm = {0};
    GxHdmiCecMessage cec_msg_test;
	DIGITAL_SERVICE_ID	stDigitalServiceInfo = {0};

    cec_msg_test.header = (CEC_LA_TV<<4)|CEC_LA_TUNER_1;
	cec_msg_test.message[0] = opcode;
	app_cec_set_device_logical_address(CEC_LA_TUNER_1);
    switch (opcode) {
		case OPCODE_ABORT:
		case OPCODE_SYSTEM_STANDBY:
		case OPCODE_GET_CEC_VERSION:
		case OPCODE_TUNER_STEP_DECREMENT:
		case OPCODE_TUNER_STEP_INCREMENT:
		case OPCODE_GET_MENU_LANGUAGE:
		case OPCODE_GIVE_PHYSICAL_ADDR:
		case OPCODE_GIVE_AUDIO_STATUS:
		case OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS:
		case OPCODE_SYSTEM_AUDIO_MODE_REQUEST:
		case OPCODE_GIVE_OSD_NAME:
		case OPCODE_RECORD_OFF:
		case OPCODE_RECORD_STATUS:
		case OPCODE_RECORD_TV_SCREEN:
			cec_msg_test.message_len = 2;
			break;
		case OPCODE_CEC_VERSION:
			cec_msg_test.message[1] = CEC_VER_1_4A;
			cec_msg_test.message_len = 3;
			break;
		case OPCODE_USER_CTRL_PRESSED:
			cec_msg_test.message[1] = CEC_KEY_UP;
			cec_msg_test.message_len = 3;
			break;
		case OPCODE_REPORT_AUDIO_STATUS:
			cec_msg_test.message[1] = (0<<7)|60;
			cec_msg_test.message_len = 3;
			break;
		case OPCODE_GIVE_TUNER_DEVICE_STATUS:
			cec_msg_test.message[1] = CEC_STATUS_REQUEST_ON;
			cec_msg_test.message_len = 3;
			break;
		case OPCODE_SET_SYSTEM_AUDIO_MODE:
			if (g_cec_msg_test_chage)
			{
			    cec_msg_test.message[1] = 0x0;
				g_cec_msg_test_chage = false;
			}
			else
			{
				cec_msg_test.message[1] = 0x1;
				g_cec_msg_test_chage = true;
			}
			cec_msg_test.message_len = 3;
			break;
		case OPCODE_MENU_REQUEST:
			if (g_cec_msg_test_chage)
			{
			    cec_msg_test.message[1] = CEC_MENU_REQUEST_DEACTIVATE;
				g_cec_msg_test_chage = false;
			}
			else
			{
				cec_msg_test.message[1] = CEC_MENU_REQUEST_ACTIVATE;
				g_cec_msg_test_chage = true;
			}
			cec_msg_test.message_len = 3;
			break;
		case OPCODE_GIVE_DECK_STATUS:
			cec_msg_test.message[1] = CEC_STATUS_REQUEST_ON;
			cec_msg_test.message_len = 3;
			break;
		case OPCODE_DECK_CONTROL:
			cec_msg_test.message[1] = DECK_CONTROL_MODE_SKIP_FORWARD_WIND;
			cec_msg_test.message_len = 3;
			break;
		case OPCODE_PLAY:
			cec_msg_test.message[1] = PLAY_MODE_FAST_FORWARD_MIN_SPEED;
			cec_msg_test.message_len = 3;
			break;
		case OPCODE_REPORT_PHYSICAL_ADDR:
			cec_msg_test.header = (CEC_LA_TV<<4)|CEC_LA_BROADCAST;
			cec_msg_test.message[1] = 0xbc;
			cec_msg_test.message[2] = 0xde;
			cec_msg_test.message[3] = CEC_DEV_TV;
			cec_msg_test.message_len = 5;
			break;
		case OPCODE_SET_MENU_LANGUAGE:
			cec_msg_test.header = (CEC_LA_TV<<4)|CEC_LA_BROADCAST;
			if (g_cec_msg_test_chage)
			{
			    cec_msg_test.message[1] = 'e';
				cec_msg_test.message[2] = 'n';
				cec_msg_test.message[3] = 'g';
				g_cec_msg_test_chage = false;
			}
			else
			{
				cec_msg_test.message[1] = 'd';
				cec_msg_test.message[2] = 'e';
				cec_msg_test.message[3] = 'u';
				g_cec_msg_test_chage = true;
			}
			cec_msg_test.message_len = 5;
			break;
		case OPCODE_SELECT_DIGITAL_SERVICE:
			app_cec_get_digital_service_Info(&stDigitalServiceInfo);				
			stDigitalServiceInfo.service_id.channel.min_ch_num += 2;
			memcpy(&cec_msg_test.message[1], &stDigitalServiceInfo, sizeof(DIGITAL_SERVICE_ID));
            /* Little-Endian to Big-Endian */
			CEC_SWAP8(cec_msg_test.message+2);
            CEC_SWAP8(cec_msg_test.message+4);
            CEC_SWAP8(cec_msg_test.message+6);
			cec_msg_test.message_len = 9;
			break;
		case OPCODE_RECORD_ON:
			app_cec_get_digital_service_Info(&stDigitalServiceInfo);				
			stDigitalServiceInfo.service_id.channel.min_ch_num += 2;
			cec_msg_test.message[1] = 2;
			//cec_msg_test.message_len = 3;
			//break;
			memcpy(&cec_msg_test.message[2], &stDigitalServiceInfo, sizeof(DIGITAL_SERVICE_ID));
            /* Little-Endian to Big-Endian */
			CEC_SWAP8(cec_msg_test.message+3);
            CEC_SWAP8(cec_msg_test.message+5);
            CEC_SWAP8(cec_msg_test.message+7);
			cec_msg_test.message_len = 10;
			break;
		case OPCODE_SET_DIGITAL_TIMER:
		case OPCODE_CLEAR_DIGITAL_TIMER:
			time = g_AppTime.utc_get(&g_AppTime);
			temp_sec = get_display_time_by_timezone(time);
			if(temp_sec >= 0)
			{
				time = temp_sec;
			}
			time = time - time % 60;
			localtime_r(&time, &temp_tm);
			memset(&stDigitalServiceInfo, 0x0, sizeof(stDigitalServiceInfo));
			app_cec_get_digital_service_Info(&stDigitalServiceInfo);	
			cec_msg_test.message[1] = temp_tm.tm_mday;
			cec_msg_test.message[2] = temp_tm.tm_mon;
			cec_msg_test.message[3] = temp_tm.tm_hour;
	        cec_msg_test.message[4] = temp_tm.tm_min + 2;
			cec_msg_test.message[5] = 0;
	        cec_msg_test.message[6] = 30;
			cec_msg_test.message[7] = (unsigned char)((0<<7) | CEC_TIMER_ONCE_ONLY);
			memcpy(&cec_msg_test.message[8], &stDigitalServiceInfo, sizeof(DIGITAL_SERVICE_ID));
            /* Little-Endian to Big-Endian */
			CEC_SWAP8(cec_msg_test.message+9);
            CEC_SWAP8(cec_msg_test.message+11);
            CEC_SWAP8(cec_msg_test.message+13);
			cec_msg_test.message_len = 16;
			break;
		default:
			return -1;
			break;
	}
	app_hdmi_cec_msg_ring_buffer(&cec_msg_test);
    return 0;
}

int app_cec_msg_test(int key)
{
    switch (key) {
		case STBK_0:
			app_cec_msg_opcode_test(OPCODE_SYSTEM_STANDBY);
			break;
		case STBK_1:
			app_cec_msg_opcode_test(OPCODE_GET_CEC_VERSION);
			app_cec_msg_opcode_test(OPCODE_CEC_VERSION);
			break;
		case STBK_2:
			app_cec_msg_opcode_test(OPCODE_USER_CTRL_PRESSED);
			break;
		case STBK_3:
			app_cec_msg_opcode_test(OPCODE_TUNER_STEP_DECREMENT);
			break;
		case STBK_4:
			app_cec_msg_opcode_test(OPCODE_TUNER_STEP_INCREMENT);
			break;
		case STBK_5:
			app_cec_msg_opcode_test(OPCODE_GET_MENU_LANGUAGE);
			app_cec_msg_opcode_test(OPCODE_SET_MENU_LANGUAGE);
			break;
		case STBK_6:
			app_cec_msg_opcode_test(OPCODE_GIVE_PHYSICAL_ADDR);
			//app_cec_msg_opcode_test(OPCODE_REPORT_PHYSICAL_ADDR);
			break;
		case STBK_7:
			//app_cec_msg_opcode_test(OPCODE_GIVE_AUDIO_STATUS);
			//app_cec_msg_opcode_test(OPCODE_SET_SYSTEM_AUDIO_MODE);
			//app_cec_msg_opcode_test(OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS);
			app_cec_msg_opcode_test(OPCODE_GIVE_DECK_STATUS);
			break;
		case STBK_8:
			//app_cec_msg_opcode_test(OPCODE_REPORT_AUDIO_STATUS);
			//app_cec_msg_opcode_test(OPCODE_GIVE_AUDIO_STATUS);
			//app_cec_msg_opcode_test(OPCODE_SYSTEM_AUDIO_MODE_REQUEST);
			//app_cec_msg_opcode_test(OPCODE_RECORD_OFF);
			//app_cec_msg_opcode_test(OPCODE_SELECT_DIGITAL_SERVICE);
			//app_cec_msg_opcode_test(OPCODE_PLAY);
			app_cec_msg_opcode_test(OPCODE_CLEAR_DIGITAL_TIMER);
			break;
		case STBK_9:
			//app_cec_msg_opcode_test(OPCODE_GIVE_OSD_NAME);
			//app_cec_msg_opcode_test(OPCODE_GIVE_TUNER_DEVICE_STATUS);
			//app_cec_msg_opcode_test(OPCODE_RECORD_ON);
			//app_cec_msg_opcode_test(OPCODE_MENU_REQUEST);
			//app_cec_msg_opcode_test(OPCODE_DECK_CONTROL);
			app_cec_msg_opcode_test(OPCODE_SET_DIGITAL_TIMER);
			break;
		default:
			break;
	}
    return 0;
}
#endif
#endif
