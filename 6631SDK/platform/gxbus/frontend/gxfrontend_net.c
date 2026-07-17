#include "gxfrontend_net.h"
#include "gxfrontend_demux_fifo.h"
#include "module/player/gxplayer_stream.h"
#include "module/player/gxplayer_module.h"
#include "module/frontend/gxfrontend_module.h"

#define TS_PACKET_SIZE    (188)
#define ONE_KB_SIZE       (1024)
#define MAX_NET_READ_SIZE (TS_PACKET_SIZE*ONE_KB_SIZE)
#define SW_BUFFER_SIZE    (TS_PACKET_SIZE*ONE_KB_SIZE*2)//32
#define HW_BUFFER_SIZE    (TS_PACKET_SIZE*ONE_KB_SIZE*4)//64
#define ALMOST_FULL_GATE  (TS_PACKET_SIZE*30)
#define NET_UNLOCK_COUNT  (5)
#define DEMUX_ID          (0)

struct gxfrontend_netinfo {
	void     *stream_info;
	char     *url;
	char	 *player_name;
	int32_t   read_bytes;
	uint32_t  remain_bytes;
	uint8_t   is_importing_data;
	uint8_t   pause_flag;
	int       read_status;
	handle_t  mutex;
	handle_t  dev;
	handle_t  dmx;
	handle_t  dvr;
};

GxFrontendNetOps net_ops = {0};
static struct gxfrontend_netinfo s_netinfo = {NULL, NULL, NULL, 0, 0, 0, 0, 0, -1, -1, -1, -1};


static int32_t _GxFrontendNet_ReadDataInit(char* player_name)
{
	if(player_name == NULL) {
		return GXCORE_ERROR;
	} else {
		s_netinfo.player_name = (char*)GxCore_Malloc((strlen(player_name) + 1) * sizeof(char));
		if(s_netinfo.player_name != NULL) {
			memcpy(s_netinfo.player_name, player_name, strlen(player_name));
			return GXCORE_SUCCESS;
		} else {
			return GXCORE_ERROR;
		}
	}
}

static int32_t _GxFrontendNet_DemuxInit(void)
{
	GxDemuxProperty_ConfigDemux cfg_dmx ={
		.stream_mode      = DEMUX_PARALLEL,
		.ts_select        = 0,
		.source           = DEMUX_SDRAM,
		.time_gate        = 0xF,
		.sync_lock_gate   = 0x3,
		.sync_loss_gate   = 0x3,
		.byt_cnt_err_gate = 0x3,
		.flags = 0,
	};
	s_netinfo.dev = -1;
	s_netinfo.dmx = -1;
	s_netinfo.dvr = -1;

	s_netinfo.dev = GxAVCreateDevice(DEMUX_ID);
	if(s_netinfo.dev == -1){
		gxloge("%s GxAVCreateDevice error:%d\n", __func__, s_netinfo.dev);
		goto error;
	}
	s_netinfo.dmx = GxAVOpenModule(s_netinfo.dev, GXAV_MOD_DEMUX, DEMUX_ID);
	if(s_netinfo.dmx == -1){
		gxloge("%s open demux error:%d\n", __func__, s_netinfo.dmx);
		goto error;
	}
	s_netinfo.dvr = GxAVOpenModule(s_netinfo.dev, GXAV_MOD_DVR, DEMUX_ID);
	if(s_netinfo.dvr == -1){
		gxloge("%s open dvr error:%d\n", __func__, s_netinfo.dvr);
		goto error;
	}
	if(GxAVSetProperty(s_netinfo.dev, s_netinfo.dmx, GxDemuxPropertyID_Config,
			&cfg_dmx, sizeof(GxDemuxProperty_ConfigDemux)) < 0){
		gxloge("%s set property error:%d\n", __func__);
		goto error;
	}
	return GXCORE_SUCCESS;
error:
	if(s_netinfo.dmx != -1)
		GxAVCloseModule(s_netinfo.dev, s_netinfo.dmx);
	if(s_netinfo.dvr != -1)
		GxAVCloseModule(s_netinfo.dev, s_netinfo.dvr);
	if(s_netinfo.dev != -1)
		GxAVDestroyDevice(s_netinfo.dev);
	s_netinfo.dev = -1;
	s_netinfo.dmx = -1;
	s_netinfo.dvr = -1;
	return GXCORE_ERROR;
}

static int32_t _GxFrontendNet_DemuxDeInit(void)
{
	if(s_netinfo.dev != -1){
		if(s_netinfo.dvr != -1){
			GxAVSetProperty(s_netinfo.dev, s_netinfo.dvr, GxDvrPropertyID_Stop, NULL, 0);
			GxAVCloseModule(s_netinfo.dev, s_netinfo.dvr);
			s_netinfo.dvr = -1;
		}
		if(s_netinfo.dmx != -1){
			GxAVCloseModule(s_netinfo.dev, s_netinfo.dmx);
			s_netinfo.dmx = -1;
		}
		GxAVDestroyDevice(s_netinfo.dev);
		s_netinfo.dev = -1;
	}
	return GXCORE_SUCCESS;
}

static int32_t _GxFrontendNet_TSRConfig(void)
{
	GxDvrProperty_Config config = {
		.src = DVR_INPUT_MEM,
		.dst = DVR_OUTPUT_DMX,
	};
	GxDvrProperty_TSRFlowControl ctrl = {0};
	ctrl.flags = DVR_FLOW_CONTROL_ES;

	if((s_netinfo.dev == -1) || (s_netinfo.dvr == -1))
		return GXCORE_ERROR;

	config.src_buf.sw_buffer_size   = SW_BUFFER_SIZE;
	config.src_buf.hw_buffer_size   = HW_BUFFER_SIZE;
	config.src_buf.almost_full_gate = ALMOST_FULL_GATE;
	config.dst_buf.sw_buffer_size   = SW_BUFFER_SIZE;
	config.dst_buf.hw_buffer_size   = HW_BUFFER_SIZE;
	config.dst_buf.almost_full_gate = ALMOST_FULL_GATE;

	GxAVSetProperty(s_netinfo.dev, s_netinfo.dvr, GxDvrPropertyID_TSRFlowControl, &ctrl, sizeof(GxDvrProperty_TSRFlowControl));

	GxAVSetProperty(s_netinfo.dev, s_netinfo.dvr, GxDvrPropertyID_Config,
			&config, sizeof(GxDvrProperty_Config));


	return GxAVSetProperty(s_netinfo.dev, s_netinfo.dvr, GxDvrPropertyID_Run, NULL, 0);
}

static int32_t GxFrontendNet_Open(const char* url)
{
	if(NULL == url || 0 == strlen(url))
		return GXCORE_INVALID_POINTER;
	GxCore_MutexLock(s_netinfo.mutex);
	s_netinfo.read_status        = 0;
	if(s_netinfo.stream_info)
		GxPlayer_StreamClose(s_netinfo.stream_info);
	s_netinfo.stream_info = GxPlayer_StreamOpen(url, GX_PLAYER_STREAM_RONLY);
	if(NULL == s_netinfo.stream_info){
		GxCore_MutexUnlock(s_netinfo.mutex);
		return GXCORE_ERROR;
	}
	if(s_netinfo.url){
		GxCore_Free(s_netinfo.url);
		s_netinfo.url = NULL;
	}
	s_netinfo.url = (char*)GxCore_Malloc((strlen(url) + 1) * sizeof(char));
	if(s_netinfo.url == NULL){
		if(s_netinfo.stream_info){
			GxPlayer_StreamClose(s_netinfo.stream_info);
			s_netinfo.stream_info = NULL;
		}
		GxCore_MutexUnlock(s_netinfo.mutex);
		return GXCORE_ERROR;
	}
	memset(s_netinfo.url, 0, strlen(url)+1);
	strncpy(s_netinfo.url, url, strlen(url)+1);

	GxCore_MutexUnlock(s_netinfo.mutex);
	return GXCORE_SUCCESS;
}

static int32_t GxFrontendNet_Close(void)
{
	GxCore_MutexLock(s_netinfo.mutex);
	if(0 != s_netinfo.is_importing_data)
		s_netinfo.is_importing_data = 0;
	if(0 != s_netinfo.pause_flag)
		s_netinfo.pause_flag = 0;
	if(s_netinfo.stream_info)
		GxPlayer_StreamClose(s_netinfo.stream_info);
	s_netinfo.stream_info = NULL;
	if(s_netinfo.url){
		GxCore_Free(s_netinfo.url);
		s_netinfo.url = NULL;
	}
	GxCore_MutexUnlock(s_netinfo.mutex);
	return GXCORE_SUCCESS;
}

static int32_t GxFrontendNet_CheckConnectionStatus(uint32_t* status)
{
#if 1
	#define NMKTAG(a,b,c,d) (a | (b << 8) | (c << 16) | (d << 24))
	#define NAVERROR_EOF                -NMKTAG( 'E','O','F',' ')
	if(status){
		*status = FE_HAS_LOCK;
		if (s_netinfo.read_status < 0) {
			*status = FE_TIMEDOUT;
			if (NAVERROR_EOF == s_netinfo.read_status) {
				*status = FE_HAS_LOCK;
				return GXCORE_SUCCESS;
			}
		}
		return GXCORE_SUCCESS;
	}
	else
		return GXCORE_INVALID_POINTER;

#else
	uint8_t buff[4] = {0};
	uint32_t nBytes = 0;
	static uint8_t unlock_count = 0;

	if(status){
		GxCore_MutexLock(s_netinfo.mutex);
		if(NULL == s_netinfo.stream_info){
			*status = FE_TIMEDOUT;
			unlock_count = 0;
		}else{
			if(s_netinfo.is_importing_data){
				if(s_netinfo.read_bytes){
					*status = FE_HAS_LOCK;
					unlock_count = 0;
				}else{
					if(unlock_count > NET_UNLOCK_COUNT){
						*status = FE_TIMEDOUT;
					}else{
						*status = FE_HAS_LOCK;
						unlock_count++;
					}
				}
			}else{
				nBytes = GxPlayer_StreamRead(s_netinfo.stream_info, buff, sizeof(buff));
				if (nBytes < 0) {
					if(s_netinfo.stream_info){
						GxPlayer_StreamClose(s_netinfo.stream_info);
						s_netinfo.stream_info = NULL;
					}
					if(s_netinfo.url)
						s_netinfo.stream_info = GxPlayer_StreamOpen(s_netinfo.url, GX_PLAYER_STREAM_RONLY);
					if(s_netinfo.stream_info == NULL)
					{
						*status = FE_TIMEDOUT;
					}
					else
						nBytes = GxPlayer_StreamRead(s_netinfo.stream_info, buff, sizeof(buff));
				}
				if(nBytes > 0)
					*status = FE_HAS_LOCK;
				else
					*status = FE_TIMEDOUT;
			}
		}
		GxCore_MutexUnlock(s_netinfo.mutex);
		return GXCORE_SUCCESS;
	}else
		return GXCORE_INVALID_POINTER;
#endif
}

#if 0//not used now
static int32_t _net_stream_import_probe(const char* srcurl)
{
	int32_t ret = GXCORE_ERROR;
	PlayerProgInfo *info = NULL;

	info = (PlayerProgInfo*)GxCore_Mallocz(sizeof(PlayerProgInfo));
	if(NULL == info){
		gxloge("%s malloc fail!\n", __func__);
		return ret;
	}
	ret = GxPlayer_MediaGetProgInfoByUrl(srcurl, info);
	GxCore_Free(info);
	return ret;
}
#endif

static void _net_stream_import_proc(void *arg)
{
	uint8_t* buffer = NULL;
	uint32_t write_data = 0;
    PlayerStatusInfo player_status;

	GxCore_ThreadDetach();

	buffer = GxCore_Malloc(MAX_NET_READ_SIZE);
	if(NULL == buffer){
		gxloge("%s malloc fail!\n", __func__);
		return;
	}
	s_netinfo.remain_bytes = 0;
	while(1){
		GxCore_MutexLock(s_netinfo.mutex);

		if (s_netinfo.is_importing_data){
			memset(&player_status, 0, sizeof(player_status));
			GxPlayer_MediaGetStatus(s_netinfo.player_name, &player_status);
			if (player_status.status == PLAYER_STATUS_PLAY_PAUSE) {
				s_netinfo.pause_flag = 1;
			} else {
				s_netinfo.pause_flag = 0;
			}
			if(!s_netinfo.pause_flag){
				write_data = 0;
				s_netinfo.read_bytes = GxPlayer_StreamRead(s_netinfo.stream_info, buffer + s_netinfo.remain_bytes, MAX_NET_READ_SIZE - s_netinfo.remain_bytes);
				gxlogd("%s read %dBytes\n", __func__, s_netinfo.read_bytes);
				if (s_netinfo.read_bytes < 0) {
					if (s_netinfo.remain_bytes) {
						GxAVModuleWrite(s_netinfo.dev, s_netinfo.dvr, buffer, s_netinfo.remain_bytes, 0);
						s_netinfo.remain_bytes = 0;
					}
					if(s_netinfo.stream_info){
						GxPlayer_StreamClose(s_netinfo.stream_info);
						s_netinfo.stream_info = NULL;
					}
					if(s_netinfo.url)
						s_netinfo.stream_info = GxPlayer_StreamOpen(s_netinfo.url, GX_PLAYER_STREAM_RONLY);
					if(NULL == s_netinfo.stream_info){
						s_netinfo.read_status = s_netinfo.read_bytes;
						goto exit;
					}
				} else {
					s_netinfo.read_bytes += s_netinfo.remain_bytes;
					s_netinfo.remain_bytes = s_netinfo.read_bytes % (TS_PACKET_SIZE * 4);
					s_netinfo.read_bytes = s_netinfo.read_bytes - s_netinfo.remain_bytes;
					while(s_netinfo.read_bytes > write_data) {
						write_data += GxAVModuleWrite(s_netinfo.dev, s_netinfo.dvr, buffer + write_data, s_netinfo.read_bytes - write_data, 0);
						GxCore_MutexUnlock(s_netinfo.mutex);
						GxCore_ThreadDelay(10);
						GxCore_MutexLock(s_netinfo.mutex);
						if(!s_netinfo.is_importing_data)
							goto exit;
					}
					if (s_netinfo.remain_bytes)
						memcpy(buffer, buffer + s_netinfo.read_bytes, s_netinfo.remain_bytes);
				}
			}
		}else{
			break;
		}
		GxCore_MutexUnlock(s_netinfo.mutex);
		GxCore_ThreadDelay(20);
	}
exit:
	s_netinfo.is_importing_data = 0;
	s_netinfo.pause_flag = 0;
	GxCore_MutexUnlock(s_netinfo.mutex);
	GxCore_Free(buffer);
}

static int32_t GxFrontendNet_StartGetData(void)
{
	handle_t net_stream_thread = 0;

	GxCore_MutexLock(s_netinfo.mutex);
	if(s_netinfo.is_importing_data){
		GxCore_MutexUnlock(s_netinfo.mutex);
		return GXCORE_SUCCESS;
	}
	s_netinfo.is_importing_data = 1;
	s_netinfo.pause_flag = 0;
	GxCore_MutexUnlock(s_netinfo.mutex);
	GxCore_ThreadCreate("NetStreamThread", &net_stream_thread, _net_stream_import_proc, NULL, 10 * 1024, GXOS_DEFAULT_PRIORITY - 1);
	return GXCORE_SUCCESS;
}

static int32_t GxFrontendNet_StopGetData(void)
{
	GxCore_MutexLock(s_netinfo.mutex);
	if(0 == s_netinfo.is_importing_data){
		GxCore_MutexUnlock(s_netinfo.mutex);
		return GXCORE_SUCCESS;
	}
	s_netinfo.is_importing_data = 0;
	s_netinfo.pause_flag = 0;
	GxCore_MutexUnlock(s_netinfo.mutex);
	return GXCORE_SUCCESS;
}

static int32_t GxFrontendNet_PauseGetData(void)
{
	GxCore_MutexLock(s_netinfo.mutex);
	if(s_netinfo.is_importing_data)
		s_netinfo.pause_flag = 1;
	GxCore_MutexUnlock(s_netinfo.mutex);
	return GXCORE_SUCCESS;
}

static int32_t GxFrontendNet_ContinueGetData(void)
{
	GxCore_MutexLock(s_netinfo.mutex);
	if(s_netinfo.is_importing_data)
		s_netinfo.pause_flag = 0;
	GxCore_MutexUnlock(s_netinfo.mutex);
	return GXCORE_SUCCESS;
}

int32_t GxFrontendNet_UnRegister(void)
{
	int32_t ret = GXCORE_SUCCESS;

	if(s_netinfo.stream_info)
		GxPlayer_StreamClose(s_netinfo.stream_info);
	if(s_netinfo.url)
		GxCore_Free(s_netinfo.url);
	if(s_netinfo.player_name)
		GxCore_Free(s_netinfo.player_name);
	if(s_netinfo.mutex != -1){
		if(GxCore_MutexDelete(s_netinfo.mutex) < 0){
			gxloge("%s mutex delete error\n", __func__);
			ret = GXCORE_ERROR;
		}
	}
	if(_GxFrontendNet_DemuxDeInit() < 0){
		gxloge("%s demux deinit error\n", __func__);
		ret = GXCORE_ERROR;
	}

	s_netinfo.stream_info       = NULL;
	s_netinfo.url               = NULL;
	s_netinfo.read_bytes        = 0;
	s_netinfo.is_importing_data = 0;
	s_netinfo.pause_flag        = 0;
	s_netinfo.read_status        = 0;
	s_netinfo.mutex             = -1;
	s_netinfo.dev               = -1;
	s_netinfo.dmx               = -1;
	s_netinfo.dvr               = -1;
	memset(&net_ops, 0, sizeof(GxFrontendNetOps));
	return ret;
}

int32_t GxFrontendNet_Register(char *player_name)
{
	if(GxCore_MutexCreate(&s_netinfo.mutex) < 0){
		gxloge("%s mutex create error\n", __func__);
		return GXCORE_ERROR;
	}
	if(_GxFrontendNet_DemuxInit() < 0){
		gxloge("%s demux init error\n", __func__);
		goto err;
	}else if(_GxFrontendNet_TSRConfig() < 0){
		gxloge("%s demux tsr config error\n", __func__);
		goto err;
	} else if(_GxFrontendNet_ReadDataInit(player_name) < 0){
		goto err;
		gxloge("%s read data config error\n", __func__);
	}
	net_ops.open              = GxFrontendNet_Open;
	net_ops.close             = GxFrontendNet_Close;
	net_ops.read_status       = GxFrontendNet_CheckConnectionStatus;
	net_ops.start_get_data    = GxFrontendNet_StartGetData;
	net_ops.stop_get_data     = GxFrontendNet_StopGetData;
	net_ops.pause_get_data    = GxFrontendNet_PauseGetData;
	net_ops.continue_get_data = GxFrontendNet_ContinueGetData;
	return GXCORE_SUCCESS;
err:
	GxFrontendNet_UnRegister();
	return GXCORE_ERROR;
}



