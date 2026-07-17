#ifndef __VOD_TRANS_PES_PARSE_H__
#define __VOD_TRANS_PES_PARSE_H__

#include "../../include/vod_porting_all.h"
#include "../../include/vod_in_common_def.h"

#define PES_PACKET_HEADER_SIZE	(sizeof(pes_session_pes_t))


typedef enum
{
	PES_SESSION_PES_FRAME_I = 0,
	PES_SESSION_PES_FRAME_B,
	PES_SESSION_PES_FRAME_P
}pes_session_frame_type;

typedef enum
{
	PES_SESSION_PES_CONTENT_AUDIO = 0,
	PES_SESSION_PES_CONTENT_VIDEO
}pes_session_frame_content_t;

typedef struct pes_session_pes_
{
	uint32_t timestamp;/*ms*/
	int32_t len;
	pes_session_frame_type frame_type;
	pes_session_frame_content_t frame_content;

	int32_t error;

	int32_t encode_type;

	uint32_t len_in_head;
	uint32_t dts ;

	struct pes_session_pes_* next;
	struct pes_session_pes_* prev;
}pes_session_pes_t;



typedef struct
{
	int32_t have_init;

	uint32_t		v_pes_num;
	uint32_t     a_pes_num;

	pes_session_pes_t* audio_queue_head;
	pes_session_pes_t* audio_queue_tail;

	pes_session_pes_t* video_queue_head;
	pes_session_pes_t* video_queue_tail;

	char video_type;
	char audio_type;

	uint32_t lock;
}pes_session_t;

typedef struct pvr_pes_packet_t{
	int32_t len;
	int32_t frame_content;
}PVR_PES_Packet_t;


int32_t pes_session_init( void );
int32_t pes_session_release(void);
int32_t pes_session_insert_pes_packet(pes_session_pes_t * pak);
int32_t pes_session_get_audio_pes_packet(uint8_t** buffer, int32_t* len, uint32_t* timestamp);
int32_t pes_session_get_video_pes_packet(uint8_t** buffer, int32_t* len, uint32_t* timestamp, int32_t* iframe);
int32_t pes_session_clean_buffer (void);
uint32_t pes_session_get_vpes_number (void);
uint32_t pes_session_get_apes_number (void);
#endif
