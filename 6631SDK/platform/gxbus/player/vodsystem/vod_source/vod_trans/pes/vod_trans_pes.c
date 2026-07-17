#include "../../include/vod_trans_api.h"
#include "../../include/vod_in_typedef.h"
#include "vod_trans_pes_parse.h"


#define DEFAULT_BUFFER_SIZE 5

static int32_t buffer_size = 0;



int32_t vod_trans_pes_open(void)
{
	buffer_size = DEFAULT_BUFFER_SIZE;

	pes_session_init();
	return 0;
}

void vod_trans_pes_close(void)
{
	pes_session_release();
}

uint8_t* vod_trans_pes_malloc_buffer(int32_t len, uint32_t* token)
{
	uint8_t *pak = NULL;

#if 0
	uint32_t allpesnum;
	allpesnum = pes_session_get_vpes_number() + pes_session_get_apes_number();
	if (allpesnum >= buffer_size)
	{
		return NULL;
	}
#endif

	pak = xmalloc(len + PES_PACKET_HEADER_SIZE);
	if (pak == 0)
	{
		gxlogd("vod_trans_pes_malloc_buffer: malloc fail, len=%d\n", len + PES_PACKET_HEADER_SIZE);
		return NULL;
	}
	memset(pak, 0, len + PES_PACKET_HEADER_SIZE);

	((pes_session_pes_t*)pak)->len = len;

	*token = (uint32_t)pak;
	return (pak + PES_PACKET_HEADER_SIZE);
}

static uint32_t _get_pts_from_pes(unsigned char * p)
{
	uint32_t pts = 0;
	uint32_t dts = 0 ;

	//	int head_len;
	//	unsigned char type_byte;
	uint32_t flags ;

	flags = p[7] ;

	if ( ( flags & 0xC0 ) == 0x80 )
	{
		pts |= ((unsigned int)(p[9] & 0x06) << 29);
		pts |= ((unsigned int)(p[10] & 0xff) << 22);
		pts |= ((unsigned int)(p[11] & 0xfe) << 14);
		pts |= ((unsigned int)(p[12] & 0xff) << 7);
		pts |= ((unsigned int)(p[13] & 0xfe) >>1);
	}
	else if ( ( flags & 0xC0 ) == 0xC0 )
	{
		pts |= ((unsigned int)(p[9] & 0x06) << 29);
		pts |= ((unsigned int)(p[10] & 0xff) << 22);
		pts |= ((unsigned int)(p[11] & 0xfe) << 14);
		pts |= ((unsigned int)(p[12] & 0xff) << 7);
		pts |= ((unsigned int)(p[13] & 0xfe) >>1);

		dts |= ((unsigned int)(p[14] & 0x06) << 29);
		dts |= ((unsigned int)(p[15] & 0xff) << 22);
		dts |= ((unsigned int)(p[16] & 0xfe) << 14);
		dts |= ((unsigned int)(p[17] & 0xff) << 7);
		dts |= ((unsigned int)(p[18] & 0xfe) >>1);


		/*if ( (pes->dts >= dts && pes->dts != 0) )
		  {

		//return 0 ;
		}
		else
		{
		pes->dts = dts ;
		}*/
	}
	else
	{
		gxlogd("can not find pts informaiton\n");
	}

	pts = pts/90 ;
	if ((p[9] & 0x08) != 0)
	{
		pts = pts + 0xffffffff/90;
	}

	return pts;
}

void vod_trans_pes_insert_buffer(uint32_t token, int32_t state)
{
	PVR_PES_Packet_t * peshead = NULL;
	pes_session_pes_t* pak = (pes_session_pes_t*)token;
	if (state == 0)
	{
		xfree(pak);
		return;
	}
	peshead = (PVR_PES_Packet_t*)(token + PES_PACKET_HEADER_SIZE);

	//gxlogd ("insert [%x][%c][%c][%c][%c] \n", peshead, peshead->pesFlag[0], peshead->pesFlag[1], peshead->pesFlag[2], peshead->pesFlag[3]);

	pak->timestamp = _get_pts_from_pes((uint8_t*)((uint8_t*)peshead + sizeof(PVR_PES_Packet_t)));

	if(peshead->frame_content == 0)
	{
		//gxlogd ("insert audio[%x] %ld len = %d\n", pak, pak->timestamp, pak->len);
		pak->frame_content = PES_SESSION_PES_CONTENT_AUDIO;
	}
	else
	{
		//gxlogd ("insert video[%x] %ld len = %d\n", pak, pak->timestamp, pak->len);
		pak->frame_content = PES_SESSION_PES_CONTENT_VIDEO;
	}

	pes_session_insert_pes_packet(pak);
}


int32_t vod_trans_pes_get_frame(int32_t aud_or_vid, uint8_t** buffer, int32_t* len, uint32_t* timestamp, int32_t* iframe)
{
	int re = -1;

	if (aud_or_vid == 0)
	{
		re = pes_session_get_audio_pes_packet(buffer, len, timestamp);
	}
	else
	{
		re = pes_session_get_video_pes_packet(buffer, len, timestamp, iframe);
	}

	return re;
}


void vod_trans_pes_set_bufsize(int32_t size)
{
	buffer_size = size;
}

int32_t vod_trans_pes_get_bufsize()
{
	return buffer_size;
}


void vod_trans_pes_clean(void)
{
	pes_session_clean_buffer();
}

uint32_t vod_trans_pes_get_v_packnum(void)
{
	return pes_session_get_vpes_number();
}

uint32_t vod_trans_pes_get_a_packnum(void)
{
	return pes_session_get_apes_number();
}


void vod_trans_pes_get_media_head(int32_t aud_or_vid, uint8_t** buffer, int32_t* len)
{

}

