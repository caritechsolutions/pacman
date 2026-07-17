#include "../../include/vod_in_common_def.h"
#include "../../include/vod_in_typedef.h"
#include "vod_trans_pes_parse.h"
#include "../../include/vod_decode_api.h"

#define VOD_SYS_TIMEOUT_INFINITY 0xffffffff

#define pes_session_pak_lock() 	if (session_info.lock){ \
		vod_porting_sem_wait(session_info.lock, VOD_SYS_TIMEOUT_INFINITY);\
	}

#define pes_session_pak_unlock() 	if (session_info.lock){ \
		vod_porting_sem_send(session_info.lock);\
	}

static pes_session_t session_info = {0};
static uint32_t ___pes_begin_time = 0;
static uint8_t *last_insert_decode_vpak = NULL;
static uint8_t *last_insert_decode_apak = NULL;
static uint32_t last_insert_decode_vlen = 0;
static uint32_t last_insert_decode_alen = 0;

static void _remove_pes_packet (pes_session_pes_t* pak, int32_t video)
{

	pes_session_pes_t ** head, **tail;

	if (pak == NULL) return;

	if (video == 1)
	{
		head = &session_info.video_queue_head;
		tail = &session_info.video_queue_tail;

		session_info.v_pes_num--;
	}
	else
	{
		head = &session_info.audio_queue_head;
		tail = &session_info.audio_queue_tail;

		session_info.a_pes_num--;
	}

	if ((*head == pak) &&((*head)->next == NULL || (*head)->next == *head))
	{
		*head = NULL;
		*tail = NULL;
	}
	else
	{
		pak->next->prev = pak->prev;
		pak->prev->next = pak->next;
		if (*head == pak)
		{
			*head = pak->next;
		}
		if (*tail == pak)
		{
			*tail = pak->prev;
		}
	}
	
	return ;
}


static pes_session_pes_t *_get_one_audio_video_pes_packet ( int32_t video ,int32_t lock)
{
	pes_session_pes_t *packet = NULL;
	if(lock)
	{
		pes_session_pak_lock() ;
	}
	
	if ( video == 1 )
	{
		packet = session_info.video_queue_head;
	}
	else
	{
		packet = session_info.audio_queue_head;
	}
	
	_remove_pes_packet(packet, video);

	if(lock)
	{
		pes_session_pak_unlock() ;
	}
	return (packet);
}



static int32_t _insert_video_audio_pes_packet( pes_session_pes_t *pak ,int32_t video)
{
	pes_session_pes_t **head, **tail;

	pes_session_pak_lock();
	
	if (video == 1)
	{
		head = &session_info.video_queue_head;
		tail = &session_info.video_queue_tail;
		
		session_info.v_pes_num++;
	}
	else
	{
		head = &session_info.audio_queue_head;
		tail = &session_info.audio_queue_tail;

		session_info.a_pes_num++;
	}
	
	/* head deal */
	if (*head == NULL)
	{
		*head = *tail = pak;
		pak->next = pak->prev = pak;
	}	
	/* second packet deal */
	else if (*head == *tail)
	{
		(*head)->next = pak;
		(*head)->prev = pak;
		pak->next = pak->prev = *head;
		*tail = pak;
	}
	else
	{
		(*tail)->next = pak;
		pak->prev = *tail;
		(*head)->prev = pak;
		pak->next = *head;
		*tail = pak;
	}
	
	pes_session_pak_unlock();
	
	return (1);
}

int32_t pes_session_init( void )
{
//	pes_session_pes_t *pespak = NULL;

	if(session_info.have_init)
	{
		pes_session_release();
	}
	session_info.have_init = 1;

	vod_porting_sem_create(1, &session_info.lock);


	___pes_begin_time = vod_porting_get_ms();

	return 0;
}


int32_t pes_session_release(void)
{
	pes_session_pes_t *pespak = NULL;

	if(session_info.have_init ==0)
	{
		return 0;
	}

	session_info.have_init = 0;

	pespak = _get_one_audio_video_pes_packet(1,0);
	while(pespak != NULL)
	{
		xfree(pespak);
		pespak = _get_one_audio_video_pes_packet(1,0);
	}

	pespak = _get_one_audio_video_pes_packet(0,0);
	while(pespak != NULL)
	{
		xfree(pespak);
		pespak = _get_one_audio_video_pes_packet(0,0);
	}

	session_info.video_queue_head = NULL;
	session_info.audio_queue_head = NULL;

	session_info.video_queue_tail = NULL;
	session_info.audio_queue_tail = NULL;
	session_info.v_pes_num = 0;
	session_info.a_pes_num = 0;

	if(last_insert_decode_vpak)
	{
		xfree(last_insert_decode_vpak);
		last_insert_decode_vpak = NULL;
	}
	if(last_insert_decode_apak)
	{
		xfree(last_insert_decode_apak);
		last_insert_decode_apak = NULL;
	}
	
	if(session_info.lock)
	{
		vod_porting_sem_delete(session_info.lock);
	}

	memset(&session_info,0,sizeof(session_info));
	
	return 0;
}

int32_t pes_session_clean_buffer (void)
{
	pes_session_pes_t * pespak = NULL;

	pes_session_pak_lock();

	pespak = _get_one_audio_video_pes_packet(1,0);
	while(pespak != NULL)
	{
		xfree(pespak);
		pespak = _get_one_audio_video_pes_packet(1,0);
	}
	
	pespak = _get_one_audio_video_pes_packet(0,0);
	while(pespak != NULL)
	{
		xfree(pespak);
		pespak = _get_one_audio_video_pes_packet(0,0);
	}

	if(last_insert_decode_vpak)
	{
		xfree(last_insert_decode_vpak);
		last_insert_decode_vpak = NULL;
	}
	if(last_insert_decode_apak)
	{
		xfree(last_insert_decode_apak);
		last_insert_decode_apak = NULL;
	}
	
	session_info.v_pes_num = 0;
	session_info.a_pes_num = 0;

	pes_session_pak_unlock();

	return 0;
}



uint32_t pes_session_get_vpes_number (void)
{
	return session_info.v_pes_num;
}

uint32_t pes_session_get_apes_number (void)
{
	return session_info.a_pes_num;
}

int32_t pes_session_get_video_pes_packet(uint8_t** buffer, int32_t* len, uint32_t* timestamp, int32_t* iframe)
{
	pes_session_pes_t * pes = NULL;
	uint8_t *peshead = NULL;

	pes = _get_one_audio_video_pes_packet(1, 1);

	if(pes == NULL)
	{
		return -1;
	}

	*timestamp = pes->timestamp;
	*len = pes->len - sizeof(PVR_PES_Packet_t);
	peshead = (uint8_t*)((uint8_t*)pes + PES_PACKET_HEADER_SIZE);
	//gxlogd ("get[%x] v[%c][%c][%c][%c] len = %d\n", peshead, peshead[0], peshead[1], peshead[2], peshead[3], *len);

	*buffer = (uint8_t*)((uint8_t*)pes + sizeof(PVR_PES_Packet_t) +PES_PACKET_HEADER_SIZE);

	if(last_insert_decode_vpak)
	{
		xfree(last_insert_decode_vpak);
		//gxlogd ("xfree v %x len = %ld\n", last_insert_decode_vpak, last_insert_decode_vlen);
		last_insert_decode_vpak = NULL;
		last_insert_decode_vlen = 0;
	}
	last_insert_decode_vpak = (uint8_t*)pes;
	last_insert_decode_vlen = pes->len;
	return 0;
}



int32_t pes_session_get_audio_pes_packet(uint8_t** buffer, int32_t* len, uint32_t* timestamp)
{
	pes_session_pes_t * pes = NULL;
	uint8_t *peshead = NULL;

	pes = _get_one_audio_video_pes_packet(0, 1);

	if(pes == NULL)
	{
		return -1;
	}

	*timestamp = pes->timestamp;
	*len = pes->len - sizeof(PVR_PES_Packet_t);

	peshead = (uint8_t*)((uint8_t*)pes + PES_PACKET_HEADER_SIZE);
	//gxlogd ("get[%x] v[%c][%c][%c][%c] len = %d\n", peshead, peshead[0], peshead[1], peshead[2], peshead[3], *len);
	*buffer = (uint8_t*)((uint8_t*)pes + sizeof(PVR_PES_Packet_t) +PES_PACKET_HEADER_SIZE);

	if(last_insert_decode_apak)
	{
		xfree(last_insert_decode_apak);
		//gxlogd ("xfree a %x len = %ld\n", last_insert_decode_apak, last_insert_decode_alen);
		last_insert_decode_apak = NULL;
		last_insert_decode_alen = 0;
	}
	last_insert_decode_apak = (uint8_t*)pes;
	last_insert_decode_alen = pes->len;
	return 0;
}

int32_t pes_session_insert_pes_packet(pes_session_pes_t * pak)
{
//	PVR_PES_Packet_t *packethead = NULL;

	if(pak == NULL)
	{
		return -1;
	}
	if(pak->frame_content == PES_SESSION_PES_CONTENT_AUDIO)
	{
		_insert_video_audio_pes_packet(pak, 0);
	}
	else
	{
		_insert_video_audio_pes_packet(pak, 1);
	}

	return 0;
}

char pes_session_get_video_type( void )
{
	return session_info.video_type;
}
char pes_session_get_audio_type( void )
{
	return session_info.audio_type;
}



