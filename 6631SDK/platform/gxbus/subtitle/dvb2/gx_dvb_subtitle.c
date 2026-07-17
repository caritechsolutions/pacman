#include "gxcore.h"
#include "gx_dvb_subtitle.h"
#include "dvbsub_dec.h"
#include "gx_subtitle_show_pixel.h"

handle_t GxDvbSubt_Open(GxDvbSubt_Param* param)
{
	return dvbsub_init_decoder();
}

int32_t GxDvbSubt_StreamSet(handle_t handle, GxDvbSubt_Stream* stream)
{
	return dvbsub_config_decoder(handle, stream->comp_page_id, stream->anci_page_id);
}

int32_t  GxDvbSubt_Synchronize(handle_t handle, int32_t milliseconds)
{
	return 0;
}

int32_t GxDvbSubt_Close(handle_t handle)
{
	return dvbsub_close_decoder(handle);
}

int32_t GxDvbSubt_Start(handle_t handle)
{
	dvbsub_clean(handle);
	return 0;
}

int32_t GxDvbSubt_Stop(handle_t handle, int freeze)
{
	dvbsub_display_init(handle);
	dvbsub_clean(handle);

	return 0;
}

int32_t GxDvbSubt_Show(handle_t handle)
{
	GxSubtitleShowPixelShow();

	return 0;
}

int32_t GxDvbSubt_Clear(handle_t handle)
{
	dvbsub_clean(handle);
	return 0;
}

int32_t GxDvbSubt_Hide(handle_t handle)
{
	GxSubtitleShowPixelHide();

	return 0;
}

int32_t GxDvbSubt_SendPesData(handle_t handle, unsigned char* packet, unsigned int packet_length)
{
	unsigned char *buf = packet, *out_buf = NULL, *p, *p_end;
	unsigned int   buf_size = 0, out_size = 0, buf_pos = 0;
	int payload_size = 0, header_len = 0;

	if (buf[0] != 0x00 || buf[1] != 0x00 || buf[2] != 0x01)
		return 0;
	buf +=3;

	payload_size = (buf[1] << 8 | buf[2]);
	if (payload_size > (packet_length - 6))
		return 0;
	buf +=3;

	header_len = buf[2];
	buf += 3;
	buf += header_len;
	buf_size = payload_size - (header_len - 3);

	if (buf_size < 2 || buf[0] != 0x20 || buf[1] != 0x00)
		return 0;

	buf_pos = 2;
	out_buf = p = buf + buf_pos;
	p_end = buf + (buf_size - buf_pos);
	while (p < p_end) {
		if (*p == 0x0f) {
			if (6 <= p_end - p) {
				unsigned int len = (p[4] << 8 | p[5]);
				if (len + 6 <= p_end - p) {
					out_size += len + 6;
					p += len + 6;
				} else
					break;
			} else
				break;
		} else
			break;
	}

	if (out_buf != NULL && out_size > 0)
		dvbsub_decode(handle, (const uint8_t *)out_buf, out_size);

	return 0;
}

int32_t GxDvbSubt_ShowTimer(handle_t handle)
{
	return dvbsub_show_timer(handle);
}
