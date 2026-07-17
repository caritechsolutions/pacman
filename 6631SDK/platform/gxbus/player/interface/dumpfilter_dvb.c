#include "dumpfilter_dvb.h"

static int _config_df_param(GxDumpFilter* df)
{
	GxDumpFilterDVBPriv* priv = df->priv;

	if (priv->is_config == 0) {
		GxFifoShallowInfo info;
		GxStreamSecurity sec;

		memset(&info, 0, sizeof(GxFifoShallowInfo));
		GxFifo_GetShallowInfo(df->inpin->source->fifo, &info);
		if (info.block_size == 0) {
			if (priv->ctrl_info.encrypt_enable)
				priv->block_size = priv->ctrl_info.block_size;
			if (priv->block_size == 0) {
				priv->buffer_size = RECODER_BLOCK_SIZE;
				priv->block_size  = 188;//处理低码率的码流,保证时间均匀跳变
			} else {
				priv->buffer_size = ((RECODER_BLOCK_SIZE / priv->block_size) * priv->block_size);
				priv->buffer_size = (priv->buffer_size == 0) ? priv->block_size : priv->buffer_size;
			}
		} else
			priv->block_size = info.block_size;
		priv->buffer_rptr = 0;
		priv->timeout_us  = DUMP_DVB_TIMEOUTUS;
		priv->tsw_ptr_en  = info.ptr_enable;
		if (priv->tsw_ptr_en == 0) {
			priv->page.size   = 0;
			priv->page.buffer = av_malloc(priv->buffer_size);
			if (priv->page.buffer == NULL) {
				gxloge("%s %d: malloc fail\n", __func__, __LINE__);
				return -1;
			}
		}

		sec.security_flag = (priv->tsw_ptr_en == 0) ? 0 : info.security_flag;
		sec.block_size    = priv->block_size;
		sec.crypt_enable  = priv->ctrl_info.encrypt_enable;
		GxStream_Control(df->s, GX_STREAM_CTRL_SET_SECURITY, &sec);
		priv->ts_encrypt = (info.security_flag || sec.crypt_enable) ? 1 : 0;
		priv->is_config  = 1;
	}

	return 0;
}

static int _config_hdr_buf(GxDumpFilter* df)
{
	GxDumpFilterDVBPriv* priv = df->priv;

	if (priv->ts_hdr.buffer != NULL)
		return -1;

	if ((priv->header_info.len > 0) || (priv->user_info.userdata_len > 0)) {
		unsigned int offset = 0;

		priv->ts_hdr.size   = priv->header_info.len + priv->user_info.userdata_len;
		priv->ts_hdr.buffer = av_malloc(priv->ts_hdr.size);
		if (priv->header_info.len > 0) {
			memcpy(priv->ts_hdr.buffer + offset, priv->header_info.data, priv->header_info.len);
			offset += priv->header_info.len;
			priv->header_info.len = 0; //only write once; record avpid;
		}

		if (priv->user_info.userdata_len > 0) {
			memcpy(priv->ts_hdr.buffer + offset, priv->user_info.userdata, priv->user_info.userdata_len);
			offset += priv->user_info.userdata_len;
			priv->user_info.userdata_len = 0; //only write once; record avpid;
		}
	}

	return 0;
}

static void _modify_hdr_cc(GxDumpFilter* df)
{
	GxDumpFilterDVBPriv* priv = df->priv;
	unsigned char *buf = priv->ts_hdr.buffer;
	unsigned int   len = priv->ts_hdr.size;

	if (buf == NULL)
		return;

	while (len >= 188) {
		int cc  = 0, i = 0;
		int pid = ((buf[1] & 0x1f) << 8) | buf[2];
		for (i = 0; i < RECODER_USER_MAX_COUNT; i++) {
			if ((priv->user_pid[i] == pid) || (priv->user_pid[i] == -1)) {
				priv->user_pid[i] = pid;
				cc = priv->user_cc[i];
				break;
			}
		}
		if (i < RECODER_USER_MAX_COUNT) {
			buf[3] = ((buf[3]&0xf0)|(cc&0xf));
			priv->user_cc[i] = cc+1;
		}
		buf += 188;
		len -= 188;
	}
	return;
}

static int _fill_hdr_packet(GxDumpFilter *df)
{
	GxDumpFilterDVBPriv* priv = df->priv;

	if ((priv->fill_hdr_timems == 0) ||
			((df->timems[1] - priv->fill_hdr_timems) >= 2000)) {
		priv->fill_hdr_timems = df->timems[1];
		return 1;
	}

	return 0;
}

static int df_dvb_open(GxDumpFilter* df)
{
	unsigned int i = 0;
	GxDumpFilterDVBPriv* priv = av_mallocz(sizeof(GxDumpFilterDVBPriv));

	if (priv == NULL) {
		gxloge("%s %d: open fail\n", __func__, __LINE__);
		return -1;
	}
	for (i = 0; i < RECODER_USER_MAX_COUNT; i++) {
		priv->user_pid[i] = -1;
		priv->user_cc[i]  = 0;
	}
	df->priv = priv;
	return 0;
}

static int df_dvb_close(GxDumpFilter* df)
{
	if (df && df->priv) {
		GxDumpFilterDVBPriv* priv = df->priv;

		if (priv->tsw_ptr_en == 0) {
			if (priv->page.buffer) {
				av_free(priv->page.buffer);
				priv->page.buffer = NULL;
			}
		}
		if (priv->ts_hdr.buffer) {
			av_free(priv->ts_hdr.buffer);
			priv->ts_hdr.buffer = NULL;
		}
		av_free(df->priv);
		df->priv = NULL;
	}

	return 0;
}

static int df_dvb_read(GxDumpFilter* df)
{
	GxDumpFilterDVBPriv* priv = df->priv;
	struct cache_page* page = &priv->page;

	if (_config_df_param(df) < 0)
		return -1;

	if (priv->tsw_ptr_en) {
		if (priv->block_size == 0)
			gxloge("%s %d: block size error\n", __func__, __LINE__);
		page->size = GxFifo_ShallowRead(df->inpin->source->fifo, &page->buffer, priv->block_size, priv->timeout_us);
	} else {
		size_t rsize = priv->buffer_rptr;
		size_t msize = 0;
		size_t size  = 0;

		if (priv->block_size == 0)
			msize = priv->buffer_size;
		else
			msize = (rsize == 0) ? priv->buffer_size : ((rsize / priv->block_size) + 1) * priv->block_size;

		size = GxFifo_Read(df->inpin->source->fifo, page->buffer + rsize, msize - rsize, priv->timeout_us);
		if (size < 0) {
			page->size = 0;
			return -1;
		}

		rsize += size;
		priv->buffer_rptr = (rsize < msize) ? rsize : 0;
		page->size = (rsize < msize) ? 0 : rsize;
	}

	return page->size;
}

static int df_dvb_write(GxDumpFilter* df)
{
	size_t write_size, size = 0;
	GxDumpFilterDVBPriv* priv = df->priv;
	struct cache_page* page = &priv->page;

	if (page->size <= 0)
		return 0;

	GxDumpfilter_Heartbeats(df);
	GxStream_Control(df->s, GX_STREAM_CTRL_SET_TIMESTAMP, &df->timems[0]);

	if (_config_hdr_buf(df) == 0) {
		if (GX_PLAYER_OK != GxStream_Control(df->s, GX_STREAM_CTRL_WRITE_HDR, (void *)&priv->ts_hdr))
			return -1;
	}

	if (priv->ts_encrypt == 0) {
		unsigned int fill_packet = _fill_hdr_packet(df);
		struct vol_new vol = { 0 };
		size_t total_size = 0;

		total_size += (fill_packet ? priv->ts_hdr.size : 0);
		total_size += page->size;
		vol.size = total_size;
		if (GxStream_Control(df->s, GX_STREAM_CTRL_GET_NEWVOLUME, &vol) == 0) {
			if (vol.is_newvol) {
				if (vol.newvol_ok) {
					fill_packet = 1;
				} else {
					gxloge("%s %d: fill hdr packet fail\n", __func__, __LINE__);
					return -1;
				}
			}
		}
		if (fill_packet && (priv->ts_hdr.size > 0)) {
			_modify_hdr_cc(df);
			GxStream_Write(df->s, priv->ts_hdr.buffer, priv->ts_hdr.size);
		}
	}

	write_size = GxStream_Write(df->s, page->buffer, page->size);
	if ((int)write_size != page->size)
		return -1;

	if (priv->tsw_ptr_en)
		GxFifo_ShallowWrite(df->inpin->source->fifo, page->buffer, page->size);

	page->size = 0;
	return write_size;
}

static int df_dvb_control(GxDumpFilter* df, int cmd, void* args)
{
	switch (cmd)
	{
	case GX_DUMPER_SET_EXTRA:
		{
			GxDumpFilterExtra* extra = args;
			GxDumpFilterDVBPriv* priv = df->priv;
			if (extra) {
				priv->header_info = extra->header_info;
				priv->user_info   = extra->user_info;
				priv->ctrl_info   = extra->ctrl_info;
				return 0;
			}
		}
		break;
	default:
		break;
	}

	return -1;
}

GxDumpFilterClass gx_dumpfilter_dvb = {
	._inherit = {
		._inherit = {
			.name    = "DumpFilterDVB",
			.parent  = &gx_dumpfilter_base,
			.size    = sizeof(GxDumpFilter),
			.create  = NULL,
			.release = NULL,
		},
		.run   = NULL,
		.pause = NULL,
		.resume= NULL,
		.stop  = NULL,
	}
	,
		.type = DUMPFILTER_DVB,
		.open = df_dvb_open,
		.close = df_dvb_close,
		.read = df_dvb_read,
		.write = df_dvb_write,
		.control = df_dvb_control,
};

