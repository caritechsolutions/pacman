#include "demux_hw.h"
#include "demux_pes.h"
#include "stheader.h"
#include "gx_config.h"
#include "gx_common.h"
#include "gx_decoder.h"

static int demux_parse_url(GxDemuxer* demuxer,int probe)
{
	int acodec, vcodec;
	int adpid, adcodec;
	DemuxHwPriv* priv = (DemuxHwPriv*)demuxer->priv;

	GxUrl_GetItem(demuxer->stream->url,GX_URL_KEY_DMXID, &priv->module);

	GxUrl_GetItem(demuxer->stream->url,GX_URL_KEY_VPID,   &priv->conf.v_id );
	GxUrl_GetItem(demuxer->stream->url,GX_URL_KEY_APID,   &priv->conf.a_id );
	GxUrl_GetItem(demuxer->stream->url,GX_URL_KEY_PCRPID, &priv->conf.pcr_id);
	GxUrl_GetItem(demuxer->stream->url,GX_URL_KEY_PMTPID, &priv->conf.pmt_id);
	GxUrl_GetItem(demuxer->stream->url,GX_URL_KEY_SYNC,   &priv->conf.sync);
	GxUrl_GetItem(demuxer->stream->url,GX_URL_KEY_VCODEC, &vcodec);
	GxUrl_GetItem(demuxer->stream->url,GX_URL_KEY_ACODEC, &acodec);
	GxUrl_GetItem(demuxer->stream->url,GX_URL_KEY_TSID,   &priv->tsid );
	priv->tsid = (priv->tsid) < 0 ? 0 : priv->tsid;
	priv->conf.vfmt = vcodec_url2std(vcodec);
	priv->conf.afmt = acodec_url2std(acodec);
	if (demuxer->flag == GX_DEMUXER_FLAG_PLAYER) {
		if ((priv->conf.vfmt == -1) && (priv->conf.afmt == -1))
			return GX_PLAYER_ERROR;

		if ((!VAILD_PID(priv->conf.a_id)) && (!VAILD_PID(priv->conf.v_id)))
			return GX_PLAYER_ERROR;
	}

	if (VAILD_PID(demuxer->info.ad_pid)) {
		adpid   = demuxer->info.ad_pid;
		adcodec = demuxer->info.ad_codec;
	} else {
		GxUrl_GetItem(demuxer->stream->url,GX_URL_KEY_APID1,  &adpid);
		GxUrl_GetItem(demuxer->stream->url,GX_URL_KEY_ACODEC1,&adcodec);
		adcodec = acodec_url2std(adcodec);
	}
	if (adpid == priv->conf.a_id)
		adpid = -1;

	priv->ts_info.a_fmt   = priv->conf.afmt;
	priv->ts_info.v_fmt   = priv->conf.vfmt;
	priv->ts_info.a_pid   = priv->conf.a_id;
	priv->ts_info.v_pid   = priv->conf.v_id;
	priv->ts_info.v_pid   = priv->conf.v_id;
	priv->ts_info.pcr_pid = priv->conf.pcr_id;
	priv->ts_info.pmt_pid = priv->conf.pmt_id;
	priv->ts_info.a_fmt1  = adcodec;
	priv->ts_info.a_pid1  = adpid;

	if (demuxer->flag == GX_DEMUXER_FLAG_PLAYER)
		priv->conf.type = HW_DEMUX_FUNC_DEMUX;
	else
		priv->conf.type = HW_DEMUX_FUNC_MUXER;

	if (priv->module == -1) priv->module = 0;

	demuxer->audio->id  = 0;
	demuxer->audio1->id = 0;
	demuxer->video->id  = 0;
	demuxer->sub->id    = 0;

	if (VAILD_PID(priv->conf.a_id)) {
		GxStreamAudioHeader* sh = GxStreamHeader_AudioNew(demuxer,0, priv->conf.a_id);

		sh->format = priv->conf.afmt;
		sh->priv.pts_sync = 1;
		sh->ds = demuxer->audio;

		if (demuxer->stream->audio_disable) {
			demuxer->audio->sh = NULL;
			priv->conf.a_quiet = 1;
		} else {
			demuxer->audio->sh = sh;
			priv->conf.a_quiet = 0;
		}

		if (!VAILD_PID(priv->conf.v_id))
			sh->priv.pts_sync = (priv->conf.sync == -1) ? 0 : 1;

		if (VAILD_PID(adpid)) {
			if (acodec_check_ad_enable(priv->conf.afmt, adcodec)) {
				GxStreamAudioHeader* sh1 = GxStreamHeader_Audio1New(demuxer, 1, adpid);
				sh1->format = adcodec;
				sh1->priv.pts_sync = 1;
				sh1->ds = demuxer->audio1;
				sh1->stream_type = AVSTREAM_TS;
				sh1->stream_pid  = adpid;
				if (!VAILD_PID(priv->conf.v_id))
					sh1->priv.pts_sync = (priv->conf.sync == -1) ? 0 : 1;

				demuxer->audio1->sh = demuxer->stream->audio_disable ? NULL : sh1;
			}
		}
	}

	if (VAILD_PID(priv->conf.v_id)) {
		GxStreamVideoHeader* sh = GxStreamHeader_VideoNew(demuxer,0, priv->conf.v_id);
		sh->format = priv->conf.vfmt;
		sh->priv.pts_sync = 1;
		sh->ds = demuxer->video;

		if (demuxer->stream->video_disable) {
			demuxer->video->sh = NULL;
			priv->conf.v_quiet = 1;
		} else {
			demuxer->video->sh = sh;
			priv->conf.v_quiet = 0;
		}

		if (!VAILD_PID(priv->conf.a_id))
			sh->priv.pts_sync = (priv->conf.sync == -1) ? 0 : 1;
	}

	if (probe)
		return GX_PLAYER_OK;

	GxHwDemux_OpenModule(priv->module);

	return GX_PLAYER_OK;
}

int demux_hw_run(GxMediaFilter* filter)
{
	DemuxHwPriv* priv ;
	GxDemuxer* demuxer = GXDEMUXER(filter);

	if (demuxer)
	{
		priv = (DemuxHwPriv*)demuxer->priv;
		if (priv)
		{
			if (HAVE_AUDIO1(demuxer)) {
				if (priv->ad_dmx)
					GxAdDemux_Run(priv->ad_dmx);
			}
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			return GxHwDemux_Run(priv->module, priv->progid, priv->conf.type);
		}
	}
	return GX_PLAYER_ERROR;
}

int demux_hw_pause(GxMediaFilter* filter)
{
	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

int demux_hw_resume(GxMediaFilter* filter)
{
	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

int demux_hw_stop(GxMediaFilter* filter)
{
	DemuxHwPriv* priv ;
	GxDemuxer* demuxer = GXDEMUXER(filter);

	if (demuxer) {
		priv = (DemuxHwPriv*)demuxer->priv;
		if (priv) {
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

			if (HAVE_AUDIO1(demuxer)) {
				if (priv->ad_dmx) {
					GxAdDemux_Stop(priv->ad_dmx);
					GxAdDemux_Close(priv->ad_dmx);
					priv->ad_dmx = NULL;
				}
			}

			GxHwDemux_Stop(priv->module, priv->progid, priv->conf.type);
			if (priv->progid != -1) {
				GxHwDemux_FreeProg(priv->module,priv->progid);
				priv->progid = -1;
			}

			if (priv->module != -1) {
				GxHwDemux_CloseModule(priv->module);
				priv->module = -1;
			}

			return GX_PLAYER_OK;
		}
	}
	return GX_PLAYER_ERROR;
}

int demux_hw_reset(GxMediaFilter* filter)
{
	DemuxHwPriv* priv ;
	GxDemuxer* demuxer = GXDEMUXER(filter);

	if (demuxer)
	{
		priv = (DemuxHwPriv*)demuxer->priv;
		if (priv)
		{
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

			GxHwDemux_Reset(priv->module, priv->progid, priv->conf.type);

			return GX_PLAYER_OK;
		}
	}

	return GX_PLAYER_ERROR;
}

extern void MpegTsPMTCreate(char* url,  char** retBuf, unsigned int * retLen, int autogen_pmt_pid);
extern void MpegTsPMTDestroy(char* pmt);

int demux_hw_config(GxMediaFilter* filter)
{
	int ret;
	GxDemuxer* demuxer = GXDEMUXER(filter);
	DemuxHwPriv* priv = NULL;
	GxPin *apin, *apin1, *vpin, *tspin = NULL;
	GxFifoConfig FifoConfig;
	int fifo_size;
	int audio_dmx_full_gate;


	GxPlayer_SystemGet(PSYS_AUDIO_DMX_FULL_GATE, &audio_dmx_full_gate);
	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	if (demuxer)
	{
		priv = (DemuxHwPriv*)demuxer->priv;

		if (demux_parse_url(demuxer, 0) != GX_PLAYER_OK)
			return GX_PLAYER_ERROR;

		priv->progid = GxHwDemux_AllocProg(priv->module);
		if (priv->progid < 0)
			return GX_PLAYER_ERROR;
		priv->conf.progid = priv->progid;

		demuxer->video->pin = GxMediaFilter_FindPin(filter, GX_DEMUX_PIN_NAME_VO);
		demuxer->audio->pin = GxMediaFilter_FindPin(filter, GX_DEMUX_PIN_NAME_AO);

		switch(priv->conf.type)
		{
		case HW_DEMUX_FUNC_DEMUX:
			{
				apin   = GxMediaFilter_FindPin(filter,GX_DEMUX_PIN_NAME_AO);
				vpin   = GxMediaFilter_FindPin(filter,GX_DEMUX_PIN_NAME_VO);
				apin1  = GxMediaFilter_FindPin(filter,GX_DEMUX_PIN_NAME_ADAO);

				priv->conf.afifo   = (apin && apin->fifo)?apin->fifo:0;
				priv->conf.vfifo   = (vpin && vpin->fifo)?vpin->fifo:0;

				if (priv->conf.afifo)
				{
					fifo_size = apin->fifo->size;
					FifoConfig.empty_gate = 7*fifo_size/8;
					FifoConfig.full_gate = audio_dmx_full_gate;
					GxFifo_Config(apin->fifo, &FifoConfig);
				}

				if (priv->conf.vfifo)
				{
					fifo_size = vpin->fifo->size;
					FifoConfig.empty_gate = 7*fifo_size/8;
					FifoConfig.full_gate = 4096*4;
					GxFifo_Config(vpin->fifo, &FifoConfig);
				}
				priv->conf.bind_descr = 1;
			}
			break;
		case HW_DEMUX_FUNC_MUXER:
			{
				tspin = GxMediaFilter_FindPin(filter,GX_DEMUX_PIN_NAME_TSO);
				priv->conf.afifo = priv->conf.vfifo = 0;
				priv->conf.tsfifo = (tspin && tspin->fifo)?tspin->fifo:0;
				//录制非加扰数据需要绑定desc module.
				priv->conf.bind_descr = priv->ts_info.ext_info.is_encrypt ? 0 : 1;
				memcpy(&priv->conf.ts_info, &priv->ts_info, sizeof(GxDemuxerPrivateTSInfo));
			}
			break;
		default :
			return GX_PLAYER_ERROR;
		}

		gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

		demuxer->stcfreq = 45000;
		ret = GxHwDemux_Config(priv->module, &priv->conf);
		if (ret != GX_PLAYER_ERROR) {
			if (ret&AUDIO_CONFIG_FAILED) {
				if (demuxer->video && demuxer->video->sh) {
					GxStreamVideoHeader* sh = demuxer->video->sh;
					sh->priv.pts_sync = (priv->conf.sync == -1) ? 0 : 1;
				}
			}
			if (ret&VIDEO_CONFIG_FAILED) {
				if (demuxer->audio && demuxer->audio->sh) {
					GxStreamAudioHeader* sh = demuxer->audio->sh;
					sh->priv.pts_sync = (priv->conf.sync == -1) ? 0 : 1;
				}
			}
			if (ret&MUXER_CONFIG_FAILED)
				return GX_PLAYER_ERROR;

#if (defined CONFIG_DVB_RECORD) && (defined CONFIG_FFMPEG_ENABLE)
			if (demuxer->flag == GX_DEMUXER_FLAG_RECORDER) {
				char *pkt_buf;
				unsigned int pkt_len;
				if (priv->conf.ts_info.user_info.have_pmt == 0 && (!VAILD_PID(priv->conf.pmt_id))) {
					MpegTsPMTCreate(demuxer->stream->url, &pkt_buf, &pkt_len, priv->conf.ts_info.autogen_pmt_pid);
					if (pkt_len) {
						memcpy(priv->conf.ts_info.header_info.data, pkt_buf, pkt_len);
						priv->conf.ts_info.header_info.len = pkt_len;
						MpegTsPMTDestroy(pkt_buf);
					}
				}
				ret = GxDemuxer_PrivatePacketCreate(&pkt_buf, &pkt_len, &priv->conf.ts_info);
				if (ret == GX_PLAYER_OK) {
					if ((priv->conf.ts_info.header_info.len + pkt_len) > RECODER_HEADER_MAX_LEN) {
						gxlogd("%s %d: error (%d %d)\n", __FUNCTION__, __LINE__,
								priv->conf.ts_info.header_info.len, pkt_len);
						GxDemuxer_PrivatePacketDestroy(pkt_buf);
						return GX_PLAYER_ERROR;
					}
					memcpy(priv->conf.ts_info.header_info.data+priv->conf.ts_info.header_info.len, pkt_buf, pkt_len);
					priv->conf.ts_info.header_info.len += pkt_len;
					GxDemuxer_PrivatePacketDestroy(pkt_buf);
				}
			}
#endif
		}

		if (HAVE_AUDIO1(demuxer)) {
			GxPin *pin = GxMediaFilter_FindPin(filter, GX_DEMUX_PIN_NAME_ADAO);

			if (!priv->ad_dmx) {
				GxStreamAudioHeader* sh1 = demuxer->audio1->sh;
				priv->ad_dmx = GxAdDemux_Open(sh1->priv.id, priv->tsid);
			}

			if (priv->ad_dmx && pin) {
				demuxer->audio1->pin = pin;
				GxAdDemux_Config(priv->ad_dmx, pin->fifo, priv->conf.bind_descr);
			} else
				gxloge("[Player]: %s %d\n", __func__, __LINE__);
		}

		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

static int demux_hw_check_file(GxDemuxer* demuxer)
{
	return GX_DEMUXER_TYPE_HW;
}

static int demux_hw_fill_buffer(GxDemuxer* demux,GxDemuxStream* ds)
{
	return GX_PLAYER_OK;
}

static int demux_hw_seek(GxDemuxer* demuxer, int64_t rel_seek_secs, int32_t audio_delay, int flags)
{
	return GX_PLAYER_OK;
}

static int demux_hw_control(GxDemuxer* demuxer, int cmd, void* arg)
{
	DemuxHwPriv* priv = NULL;
	int audio_dmx_full_gate;

	GxPlayer_SystemGet(PSYS_AUDIO_DMX_FULL_GATE, &audio_dmx_full_gate);
	if (demuxer)
	{
		priv = (DemuxHwPriv*)demuxer->priv;
		if (priv)
		{
			switch (cmd) {
			case GX_DEMUXER_CTRL_SWITCH_VIDEO:
			case GX_DEMUXER_CTRL_SWITCH_AUDIO:
				{
					if (arg)
					{
						GxDemuxerStreamSwitch* StreamSwitch = (GxDemuxerStreamSwitch*)arg;

						if (VAILD_PID(StreamSwitch->pid)) {
							if (cmd == GX_DEMUXER_CTRL_SWITCH_AUDIO)
							{
								GxStreamAudioHeader* sh = demuxer->audio->sh;
								int apid, acodec;

								sh->format  = StreamSwitch->type;
								sh->priv.id = StreamSwitch->pid;

								priv->conf.a_id = StreamSwitch->pid;
								priv->conf.afmt = StreamSwitch->type;

								if (demuxer->stream) {
									apid   = priv->conf.a_id;
									acodec = acodec_std2url(priv->conf.afmt);
									GxUrl_SetItem(demuxer->stream->url, GX_URL_KEY_APID,   apid,   PLAYER_URL_LONG);
									GxUrl_SetItem(demuxer->stream->url, GX_URL_KEY_ACODEC, acodec, PLAYER_URL_LONG);
								}
								if(sh->format == AUDIO_CODEC_AC3 || sh->format == AUDIO_CODEC_EAC3) {
									sh->big_endian = 1;
								}

								GxHwDemux_SwitchStream(priv->module, priv->progid,
										HW_DEMUX_AUDIO, StreamSwitch->pid, priv->conf.bind_descr);
							}
							else if (cmd == GX_DEMUXER_CTRL_SWITCH_VIDEO)
							{
								GxStreamVideoHeader* sh = demuxer->video->sh;

								sh->format  = StreamSwitch->type;
								sh->priv.id = StreamSwitch->pid;

								priv->conf.v_id = StreamSwitch->pid;
								priv->conf.vfmt = StreamSwitch->type;

								GxHwDemux_SwitchStream(priv->module, priv->progid,
										HW_DEMUX_VIDEO, StreamSwitch->pid, priv->conf.bind_descr);
							}
						}

						return GX_DEMUXER_CTRL_OK;
					}
					return GX_DEMUXER_CTRL_ERROR;
				}
			case GX_DEMUXER_CTRL_SET_VIDEO_DROPMODE:
				{
					int dropmode = *(int*)arg;
					if (dropmode == DROPMODE_UNSUPPORT) {
						GxHwDemux_StopStream(priv->module,priv->progid,HW_DEMUX_VIDEO);
					}
					return GX_DEMUXER_CTRL_OK;
				}
			case GX_DEMUXER_CTRL_SET_AUDIO_DROPMODE:
				{
					int dropmode = *(int*)arg;
					if (dropmode == DROPMODE_UNSUPPORT) {
						GxHwDemux_StopStream(priv->module, priv->progid, HW_DEMUX_AUDIO);
					}
					return GX_DEMUXER_CTRL_OK;
				}
			case GX_DEMUXER_CTRL_RESET_VIDEO:
				{
					if (HAVE_VIDEO(demuxer)) {
						GxHwDemux_ResetStream(priv->module, priv->progid, HW_DEMUX_VIDEO);
						return GX_DEMUXER_CTRL_OK;
					}
					return GX_DEMUXER_CTRL_ERROR;
				}
			case GX_DEMUXER_CTRL_RESET_AUDIO:
				{
					if (HAVE_AUDIO(demuxer)) {
						GxHwDemux_ResetStream(priv->module, priv->progid, HW_DEMUX_AUDIO);
						return GX_DEMUXER_CTRL_OK;
					}
					return GX_DEMUXER_CTRL_ERROR;
				}
			case GX_DEMUXER_CTRL_SET_RECORD_CONFIG:
				{
					GxDemuxerRecordExtInfo* config = (GxDemuxerRecordExtInfo*)arg;
					memcpy(&priv->ts_info.ext_info, config, sizeof(GxDemuxerRecordExtInfo));
					return GX_DEMUXER_CTRL_OK;
				}
			case GX_DEMUXER_CTRL_SET_RECORD_USER_INFO:
				{
					GxDemuxerRecordUserInfo* user_info = (GxDemuxerRecordUserInfo*)arg;
					int copy_num = user_info->userdata_len/188;
					if (user_info->userdata_len%188)
						copy_num += 1;
					if (copy_num*188 >= RECODER_USER_MAX_LEN)
						user_info->userdata_len = RECODER_USER_MAX_LEN;
					else
						user_info->userdata_len = copy_num*188;
					memcpy(&priv->ts_info.user_info, user_info, sizeof(GxDemuxerRecordUserInfo));
					return GX_DEMUXER_CTRL_OK;
				}
			case GX_DEMUXER_CTRL_SET_RECORD_ENCRYPT:
				{
					PlayerRecordEncrypt* enc = (PlayerRecordEncrypt*)arg;

					priv->ts_info.ctrl_info.vaild = 1;
					priv->ts_info.ctrl_info.encrypt_enable = enc->user_encrypt_enable;
					priv->ts_info.ctrl_info.block_size     = enc->user_encrypt_blocksize;
					priv->ts_info.ctrl_info.tsw_security   = 0;
					return GX_DEMUXER_CTRL_OK;
				}
			case GX_DEMUXER_CTRL_GET_RECORD_CTRL_INFO:
				{
					GxDemuxerRecordCtrlInfo *ctrl_info = (GxDemuxerRecordCtrlInfo *)arg;
					memcpy(ctrl_info, &priv->conf.ts_info.ctrl_info, sizeof(GxDemuxerRecordCtrlInfo));
					return GX_DEMUXER_CTRL_OK;
				}
			case GX_DEMUXER_CTRL_GET_RECORD_HEADER_INFO:
				{
					GxDemuxerRecordHeaderInfo* header_info = (GxDemuxerRecordHeaderInfo*)arg;
					memcpy(header_info, &priv->conf.ts_info.header_info, sizeof(GxDemuxerRecordHeaderInfo));
					return GX_DEMUXER_CTRL_OK;
				}
			case GX_DEMUXER_CTRL_GET_RECORD_USER_INFO:
				{
					GxDemuxerRecordUserInfo* user_info = (GxDemuxerRecordUserInfo*)arg;
					memcpy(user_info, &priv->conf.ts_info.user_info, sizeof(GxDemuxerRecordUserInfo));
					return GX_DEMUXER_CTRL_OK;
				}
			case GX_DEMUXER_CTRL_CHECK_RECORD_OVERFLOW:
				{
					*(int*)arg = GxHwDemux_CheckRecordOverflow(priv->module, priv->progid);
					return GX_DEMUXER_CTRL_OK;
				}
			case GX_DEMUXER_CTRL_SUB_ON:
				{
					return GxHwDemux_RunStream(priv->module, priv->progid,
							HW_DEMUX_PES_SUBT, *(int*)arg, priv->conf.bind_descr);
				}
			case GX_DEMUXER_CTRL_SUB_OFF:
				{
					GxHwDemux_StopStream(priv->module, priv->progid, HW_DEMUX_PES_SUBT);
					return GX_DEMUXER_CTRL_OK;
				}
			case GX_DEMUXER_CTRL_SWITCH_SUB:
				{
					GxDemuxerStreamSwitch* StreamSwitch;
					StreamSwitch = (GxDemuxerStreamSwitch*)arg;
					if (StreamSwitch) {
						if (!VAILD_PID(StreamSwitch->pid))
							return GX_DEMUXER_CTRL_ERROR;

						GxHwDemux_StopStream(priv->module, priv->progid, HW_DEMUX_PES_SUBT);
						GxHwDemux_RunStream(priv->module, priv->progid,
								HW_DEMUX_PES_SUBT, StreamSwitch->pid, priv->conf.bind_descr);
						return GX_DEMUXER_CTRL_OK;
					}
					return GX_DEMUXER_CTRL_ERROR;
				}
			case GX_DEMUXER_CTRL_SUB_READ_DATA:
				{
					GxDemuxerPesData* pes_data = (GxDemuxerPesData*)arg;

					if ((pes_data == NULL) || (pes_data->buffer == NULL) || (pes_data->size <= 0))
						return GX_DEMUXER_CTRL_ERROR;

					pes_data->size = GxHwDemux_ReadFilterData(priv->module,
							priv->progid, HW_DEMUX_PES_SUBT, pes_data->buffer, pes_data->size);
					return GX_DEMUXER_CTRL_OK;
				}
			case GX_DEMUXER_CTRL_SUB_PSI_ON:
				{
					return GxHwDemux_RunStream(priv->module, priv->progid,
							HW_DEMUX_PSI_SUBT, *(int*)arg, priv->conf.bind_descr);
				}
			case GX_DEMUXER_CTRL_SUB_PSI_OFF:
				{
					GxHwDemux_StopStream(priv->module, priv->progid, HW_DEMUX_PSI_SUBT);
					return GX_DEMUXER_CTRL_OK;
				}
			case GX_DEMUXER_CTRL_SUB_PSI_READ_DATA:
				{
					GxDemuxerPsiData* psi_data = (GxDemuxerPsiData*)arg;

					if ((psi_data == NULL) || (psi_data->buffer == NULL) || (psi_data->size <= 0))
						return GX_DEMUXER_CTRL_ERROR;

					psi_data->size = GxHwDemux_ReadFilterData(priv->module,
							priv->progid, HW_DEMUX_PSI_SUBT, psi_data->buffer, psi_data->size);
					return GX_DEMUXER_CTRL_OK;
				}
			case GX_DEMUXER_CTRL_OPEN_AD_AUDIO:
				{
					if (!priv->ad_dmx) {
						GxDemuxerStreamSwitch* ad = (GxDemuxerStreamSwitch*)arg;
						GxStreamAudioHeader* sh1 = NULL;

						if (!ad || !VAILD_PID(ad->pid)) {
							gxloge("[Player]: %s %d (pid %d)\n", __func__, __LINE__, ad->pid);
							return GX_DEMUXER_CTRL_ERROR;
						}
						if (HAVE_AUDIO1(demuxer))
							sh1 = demuxer->audio1->sh;
						else
							sh1 = GxStreamHeader_Audio1New(demuxer, 1, ad->pid);
						sh1->priv.pts_sync = 1;
						sh1->ds = demuxer->audio1;
						sh1->format  = ad->type;
						sh1->priv.id = ad->pid;
						if (!VAILD_PID(priv->conf.v_id))
							sh1->priv.pts_sync = (priv->conf.sync == -1) ? 0 : 1;
						sh1->stream_type = AVSTREAM_TS;
						sh1->stream_pid  = ad->pid;
						demuxer->audio1->sh = sh1;
						priv->ad_dmx = GxAdDemux_Open(ad->pid, priv->tsid);
						return GX_DEMUXER_CTRL_OK;
					} else {
						if (HAVE_AUDIO1(demuxer))
							return GX_DEMUXER_CTRL_OK;
						else
							gxloge("[Player]: %s %d\n", __func__, __LINE__);
					}

					return GX_DEMUXER_CTRL_ERROR;
				}
			case GX_DEMUXER_CTRL_CLOSE_AD_AUDIO:
				{
					if (priv->ad_dmx) {
						GxAdDemux_Close(priv->ad_dmx);
						priv->ad_dmx = NULL;
						demuxer->audio1->sh = NULL;
					}
					return GX_DEMUXER_CTRL_OK;
				}
			case GX_DEMUXER_CTRL_RUN_AD_AUDIO:
				{
					if (priv->ad_dmx) {
						GxPin *pin = demuxer->audio1->pin;

						if (!pin)
							return GX_DEMUXER_CTRL_ERROR;

						GxAdDemux_Config(priv->ad_dmx, pin->fifo, priv->conf.bind_descr);
						GxAdDemux_Run(priv->ad_dmx);

						return GX_DEMUXER_CTRL_OK;
					}
					return GX_DEMUXER_CTRL_ERROR;
				}
			case GX_DEMUXER_CTRL_STOP_AD_AUDIO:
				{
					if (priv->ad_dmx) {
						GxAdDemux_Stop(priv->ad_dmx);
						return GX_DEMUXER_CTRL_OK;
					}
					return GX_DEMUXER_CTRL_ERROR;
				}
			case GX_DEMUXER_CTRL_RESET_AD_AUDIO:
				{
					if (priv->ad_dmx) {
						GxPin *pin = demuxer->audio1->pin;

						if (!pin)
							return GX_DEMUXER_CTRL_ERROR;

						GxAdDemux_Stop(priv->ad_dmx);
						GxAdDemux_Config(priv->ad_dmx, pin->fifo, priv->conf.bind_descr);
						GxAdDemux_Run(priv->ad_dmx);
						return GX_DEMUXER_CTRL_OK;
					}
					return GX_DEMUXER_CTRL_ERROR;
				}
			case GX_DEMUXER_CTRL_SAVE_AUDIO:
				{
					GxHwDemux_SaveStream(priv->module, priv->progid, HW_DEMUX_AUDIO);
					return GX_DEMUXER_CTRL_OK;
				}
			case GX_DEMUXER_CTRL_RESTORE_AUDIO:
				{
					GxHwDemux_RestoreStream(priv->module, priv->progid, HW_DEMUX_AUDIO);
					return GX_DEMUXER_CTRL_OK;
				}
			case GX_DEMUXER_CTRL_SAVE_VIDEO:
				{
					GxHwDemux_SaveStream(priv->module, priv->progid, HW_DEMUX_VIDEO);
					return GX_DEMUXER_CTRL_OK;
				}
			case GX_DEMUXER_CTRL_RESTORE_VIDEO:
				{
					GxHwDemux_RestoreStream(priv->module, priv->progid, HW_DEMUX_VIDEO);
					return GX_DEMUXER_CTRL_OK;
				}
			default:
				return GX_DEMUXER_CTRL_NOTIMPL;
			}
		}
	}
	return GX_DEMUXER_CTRL_ERROR;
}

static GxDemuxer* demux_hw_open(GxDemuxer *demuxer)
{
	DemuxHwPriv* priv = NULL;

	priv = (void*)av_malloc(sizeof(DemuxHwPriv));
	if (priv == NULL)
		return NULL;

	demuxer->priv = (void* )priv;
	memset(demuxer->priv, 0, sizeof(DemuxHwPriv));

	priv->module = -1;
	priv->progid = -1;

	demuxer->seekable  = 0;

	if (demux_parse_url(demuxer, 1) != GX_PLAYER_OK) {
		av_free(demuxer->priv);
		demuxer->priv = NULL;
		return NULL;
	}

	priv->dev = GxAvdev_CreateDevice(0);
	priv->chip = GxChipDetect(priv->dev);

	return demuxer;
}

static void demux_hw_close(GxDemuxer* demuxer)
{
	DemuxHwPriv* priv = NULL;

	if (demuxer)
	{
		priv = (DemuxHwPriv*)demuxer->priv;
		if (priv)
		{
			av_free(demuxer->priv);
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
		}
	}
}

static int demux_hw_init(void)
{
	return GxHwDemux_Init();
}

GxDemuxerClass gx_demux_hw = {
	._inherit = {
		._inherit = {
			.name    = "Demuxer HW",
			.parent  = &gx_DemuxerBase,
			.size    = sizeof(GxDemuxer),
			.init    = demux_hw_init,
			.create  = NULL,
			.release = NULL,
		},
		.run    = demux_hw_run,
		.pause  = demux_hw_pause,
		.resume = demux_hw_resume,
		.config = demux_hw_config,
		.stop   = demux_hw_stop,
		.reset  = demux_hw_reset,
	},
	DEF_AUTHOR("demuxer","hw","No description","L.F","No comment"),

	.name        = "HW Demuxer",
	.type        = GX_DEMUXER_TYPE_HW,
	.check_file  = demux_hw_check_file,
	.open        = demux_hw_open,
	.fill_buffer = demux_hw_fill_buffer,
	.close       = demux_hw_close,
	.seek        = demux_hw_seek,
	.control     = demux_hw_control,
};
