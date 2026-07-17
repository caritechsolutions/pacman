#include "../../include/vod_trans_api.h"
#include "../../include/vod_in_typedef.h"
#include "vod_trans_ts.h"
#include "vod_trans_ts_parse.h"


#define DEFAULT_BUFFER_SIZE 1500

static int32_t buffer_size = 0;


int32_t vod_trans_ts_open()
{
	buffer_size = DEFAULT_BUFFER_SIZE;

	ts_session_init();

	return 0;
}

void vod_trans_ts_close(void)
{
	ts_session_release();
}

uint8_t* vod_trans_ts_malloc_buffer(int32_t len, uint32_t* token)
{
	uint8_t *pak = NULL;
	uint32_t alltsnum;

#if 1
	alltsnum = ts_session_get_vts_number() + ts_session_get_ats_number();
	if (alltsnum >= buffer_size)
	{
		gxlogd("vod_trans_ts_malloc_buffer: buffer full, size=%d\n", buffer_size);
		return NULL;
	}
#endif

	pak = xmalloc(len + RTP_PACKET_HEADER_SIZE);
	if (pak == 0)
	{
		gxlogd("vod_trans_malloc_buffer: malloc fail, len=%d\n", len + RTP_PACKET_HEADER_SIZE);
		return NULL;
	}
	memset(pak, 0, len + RTP_PACKET_HEADER_SIZE);
	((rtp_packet*)pak)->rtp_data_len = len;
	*token = (uint32_t)pak;
	return (pak + RTP_PACKET_HEADER_SIZE);
}


void vod_trans_ts_insert_buffer(uint32_t token, int32_t state)
{
	rtp_packet* pak = (rtp_packet*)token;
	if (state == 0)
	{
		xfree(pak);
		return;
	}

	ts_session_insert_rtp_packet(pak);
}


int32_t vod_trans_ts_get_frame(int32_t aud_or_vid, uint8_t** buffer, int32_t* len, uint32_t* timestamp, int32_t* iframe)
{
	int re = -1;

	if (aud_or_vid == 0)
	{
		re = ts_session_get_audio_pes_packet(buffer, len, timestamp);
	}
	else
	{
		re = ts_session_get_video_pes_packet(buffer, len, timestamp, iframe);
	}

	return re;
}


void vod_trans_ts_set_bufsize(int32_t size)
{
	buffer_size = size;
}

int32_t vod_trans_ts_get_bufsize()
{
	return buffer_size;
}


void vod_trans_ts_clean(void)
{
	ts_session_clean_buffer();
}

uint32_t vod_trans_ts_get_v_packnum(void)
{
	return ts_session_get_vts_number();
}

uint32_t vod_trans_ts_get_a_packnum(void)
{
	return ts_session_get_ats_number();
}


void vod_trans_ts_get_media_head(int32_t aud_or_vid, uint8_t** buffer, int32_t* len)
{

}

int32_t vod_trans_ts_sync(char* data, int len)
{
	return ts_session_sync(data, len);
}
