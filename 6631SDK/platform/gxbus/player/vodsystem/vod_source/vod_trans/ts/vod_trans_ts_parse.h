#ifndef __VOD_TRANS_TS_PARSE_H__
#define __VOD_TRANS_TS_PARSE_H__

#include "../../include/vod_porting_all.h"
#include "../../include/vod_in_common_def.h"

#define RTP_PACKET_HEADER_SIZE	(sizeof(rtp_packet_data))
#define RTP_VERSION 2
#define RTP_MAX_PACKET_LEN 1500

struct rtp;

/* XXX gtkdoc doesn't seem to be able to handle functions that return
* struct *'s. */
typedef struct rtp *rtp_t;

typedef struct rtp_packet_data
{
	struct rtp_packet *rtp_pd_next, *rtp_pd_prev;
	uint32_t	*rtp_pd_csrc;
	uint8_t	*rtp_pd_data;
	int32_t		 rtp_pd_data_len;
	uint8_t	*rtp_pd_extn;
	uint16_t	 rtp_pd_extn_len; /* Size of the extension in 32 bit words minus one */
	uint16_t	 rtp_pd_extn_type;/* Extension type field in the RTP packet header   */
	int32_t            rtp_pd_buflen; /* received buffer len (w/rtp header) */
	/*int32_t            rtp_pd_have_timestamp;*/
	/*uint64_t       rtp_pd_timestamp;*/
}
rtp_packet_data;

typedef struct rtp_packet_header
{
#ifdef WORDS_BIGENDIAN
	uint32_t   ph_v:2;	/* packet type                */
	uint32_t   ph_p:1;	/* padding flag               */
	uint32_t   ph_x:1;	/* header extension flag      */
	uint32_t   ph_cc:4;	/* CSRC count                 */
	uint32_t   ph_m:1;	/* marker bit                 */
	uint32_t   ph_pt:7;	/* payload type               */
	uint32_t   ph_seq:16; /*sequence number    */
#else
	uint32_t   ph_cc:4;	/* CSRC count                 */
	uint32_t   ph_x:1;	/* header extension flag      */
	uint32_t   ph_p:1;	/* padding flag               */
	uint32_t   ph_v:2;	/* packet type                */
	uint32_t   ph_pt:7;	/* payload type               */
	uint32_t   ph_m:1;	/* marker bit                 */
	uint32_t   ph_seq:16; /*sequence number   */
#endif
	uint32_t          ph_ts;	/* timestamp                  */
	uint32_t          ph_ssrc;	/* synchronization source     */
	/* The csrc list, header extension and data follow, but can't */
	/* be represented in the struct.                              */
}
rtp_packet_header;


typedef struct rtp_packet_ext_header
{
	uint16_t profile;
	uint16_t length;
}rtp_packet_ext_header;





#define rtp_next      pd.rtp_pd_next
#define rtp_prev      pd.rtp_pd_prev
#define rtp_csrc      pd.rtp_pd_csrc
#define rtp_data      pd.rtp_pd_data
#define rtp_data_len  pd.rtp_pd_data_len
#define rtp_extn      pd.rtp_pd_extn
#define rtp_extn_len  pd.rtp_pd_extn_len
#define rtp_extn_type pd.rtp_pd_extn_type

#define rtp_pak_v    ph.ph_v
#define rtp_pak_p    ph.ph_p
#define rtp_pak_x    ph.ph_x
#define rtp_pak_cc   ph.ph_cc
#define rtp_pak_m    ph.ph_m
#define rtp_pak_pt   ph.ph_pt
#define rtp_pak_seq  ph.ph_seq
#define rtp_pak_ts   ph.ph_ts
#define rtp_pak_ssrc ph.ph_ssrc

typedef struct rtp_packet
{
	/* The following are pointers to the data in the packet as    */
	/* it came off the wire. The packet it read in such that the  */
	/* header maps onto the latter part of this struct, and the   */
	/* fields in this first part of the struct point into it. The */
	/* entire packet can be freed by freeing this struct, without */
	/* having to free the csrc, data and extn blocks separately.  */
	rtp_packet_data pd;
	/* The following map directly onto the RTP packet header...   */
	rtp_packet_header ph;
}
rtp_packet;

typedef struct
{
	uint32_t         ssrc;
	uint32_t         ntp_sec;
	uint32_t         ntp_frac;
	uint32_t         rtp_ts;
	uint32_t         sender_pcount;
	uint32_t         sender_bcount;
}
rtcp_sr;

typedef struct
{
	uint32_t	ssrc;		/* The ssrc to which this RR pertains */
#ifdef WORDS_BIGENDIAN
	uint32_t	fract_lost:8;
	uint32_t	total_lost:24;
#else
	uint32_t	total_lost:8;
	uint32_t	fract_lost:24;
#endif
	uint32_t	last_seq;
	uint32_t	jitter;
	uint32_t	lsr;
	uint32_t	dlsr;
}
rtcp_rr;

typedef struct
{
#ifdef WORDS_BIGENDIAN
	uint32_t  version:2;	/* RTP version            */
	uint32_t  p:1;		/* padding flag           */
	uint32_t  subtype:5;	/* application dependent  */
#else
	uint32_t  subtype:5;	/* application dependent  */
	uint32_t  p:1;		/* padding flag           */
	uint32_t  version:2;	/* RTP version            */
#endif
	uint32_t  pt:8;		/* packet type            */
	uint32_t        length:16;		/* packet length          */
	uint32_t        ssrc;
	char            name[4];        /* four ASCII characters  */
	char            data[1];        /* variable length field  */
}
rtcp_app;

/* rtp_event type values. */
typedef enum
{
	RX_RTP,
	RX_SR,
	RX_RR,
	RX_SDES,
	RX_BYE,         /* Source is leaving the session, database entry is still valid                           */
	SOURCE_CREATED,
	SOURCE_DELETED, /* Source has been removed from the database                                              */
	RX_RR_EMPTY,    /* We've received an empty reception report block                                         */
	RX_RTCP_START,  /* Processing a compound RTCP packet about to start. The SSRC is not valid in this event. */
	RX_RTCP_FINISH,	/* Processing a compound RTCP packet finished. The SSRC is not valid in this event.       */
	RR_TIMEOUT,
	RX_APP
}
rtp_event_type;

typedef struct
{
	uint32_t	 ssrc;
	rtp_event_type	 type;
	void		*data;
	struct timeval	*ts;
}
rtp_event;

/* Callback types */
typedef void (*rtp_callback)(struct rtp *session, rtp_event *e);
typedef rtcp_app* (*rtcp_app_callback)(struct rtp *session, uint32_t rtp_ts, int32_t max_size);
typedef int32_t (*rtcp_send_packet_t)(void *userdata, uint8_t *buffer, int32_t buflen);

/* SDES packet types... */
typedef enum 
{
	RTCP_SDES_END   = 0,
	RTCP_SDES_CNAME = 1,
	RTCP_SDES_NAME  = 2,
	RTCP_SDES_EMAIL = 3,
	RTCP_SDES_PHONE = 4,
	RTCP_SDES_LOC   = 5,
	RTCP_SDES_TOOL  = 6,
	RTCP_SDES_NOTE  = 7,
	RTCP_SDES_PRIV  = 8
}
rtcp_sdes_type;

typedef struct
{
	uint8_t		type;		/* type of SDES item              */
	uint8_t		length;		/* length of SDES item (in bytes) */
	char		data[1];	/* text, not zero-terminated      */
}
rtcp_sdes_item;

/* RTP options */
typedef enum
{
	RTP_OPT_PROMISC =	    1,
	RTP_OPT_WEAK_VALIDATION	=   2,
	RTP_OPT_FILTER_MY_PACKETS = 3
}
rtp_option;













#define STPTI_InvalidPid()  0xe000

#define MAX_VIDEO_PES_PACKET_LEN (128*1024)//(500*1024)
#define MAX_AUDIO_PES_PACKET_LEN  (5*1024)//(50*1024)
#define chk_seq_max 10
#define ts_recv_buff_max 1500


#define MAX_SAVE_TIMESTAMP 100

#define  chk_seq_continue(pre,curr)   \
(((pre)==65535 && (curr) == 0) || ((curr) == (pre) + 1) )

#define chk_seq_lit(a,b)  \
((((a)<(b)) && (((b)-(a)) <= chk_seq_max))\
|| ((a)>=(65535-chk_seq_max) && (b)<=chk_seq_max)\
)
#define chk_seq_big(a,b) (!chk_seq_lit(a,b))

#define TS_PACKET_LEN 188
#define MAX_TS_STREAM_LEN (8 * TS_PACKET_LEN)




typedef enum
{
	TS_RECV_MODE_NONE =0,
	TS_RECV_MODE_UDP,
	TS_RECV_MODE_GROUP
}ts_recv_mode_t;

typedef enum
{
	TS_SESSION_PES_FRAME_I = 0,
	TS_SESSION_PES_FRAME_B,
	TS_SESSION_PES_FRAME_P
}ts_session_frame_type;

typedef enum
{
	TS_SESSION_PES_CONTENT_AUDIO = 0,
	TS_SESSION_PES_CONTENT_VIDEO
}ts_session_frame_content_t;

typedef struct
{
	uint32_t timestamp;/*ms*/
	uint8_t *data;
	int32_t len;
	ts_session_frame_type frame_type;
	ts_session_frame_content_t frame_content;

	int32_t error;

	int32_t encode_type;

	uint16_t len_in_head;
	uint32_t dts ;
}ts_session_pes_t;



typedef struct ts_packet
{
	unsigned char sync_byte;
	char err_packet;
	char payload_start;
	short pid;
	char counter;
	unsigned char data_len;

	struct ts_packet* next;
	struct ts_packet* prev;
}ts_packet_t;

typedef struct
{
	uint16_t curr_pid;
	uint16_t pid_num;
	uint16_t pids[6];
	int32_t aud_type[6];
}isma_ts_audio_pids_t;

typedef struct
{
	int32_t have_init;
	rtp_packet* rtp_featue_head;
	int32_t rtp_featue_num;
//	int32_t rtp_featue_max;
	uint16_t curr_rtp_seq;
	
	uint32_t		v_ts_num;
	uint32_t     a_ts_num;
	uint32_t ts_over_run;
	
	/*用来统计丢包率*/
	uint32_t     tot_rcv_ts_num;
	uint32_t     tot_rcv_rtp_num;

	ts_packet_t* audio_queue_head;
	ts_packet_t* audio_queue_tail;

	ts_packet_t* video_queue_head;
	ts_packet_t* video_queue_tail;

	uint32_t video_pid;
	uint32_t audio_pid;
	uint32_t pcr_pid;
	
	char video_type;
	char audio_type;


	ts_session_pes_t video_pes;
	ts_session_pes_t audio_pes;
	
	unsigned char * video_pes_buffer;
	int32_t video_pes_len;
	int32_t video_pes_ok;
	
	unsigned char * audio_pes_buffer;
	int32_t audio_pes_len;
	int32_t audio_pes_ok;
	
	unsigned char video_ts_buffer[200];
	int32_t video_ts_buffer_len;
	
	unsigned char audio_ts_buffer[200];
	int32_t audio_ts_buffer_len;

	char video_counter;
	char audio_counter;
	
	int32_t audio_timestamp_num;
	int32_t audio_timestamp_index;
	uint32_t audio_timestamps[MAX_SAVE_TIMESTAMP];
	int32_t video_timestamp_num;
	int32_t video_timestamp_index;
	uint32_t video_timestamps[MAX_SAVE_TIMESTAMP];

	int32_t have_one_pes;

	int32_t find_pat;
	uint16_t pmt_pid;
	int32_t pmt_ok;

	isma_ts_audio_pids_t audio_pids;

	uint32_t recv_rtp_length;
	int32_t print_flag;

	uint32_t lock;
}ts_session_t;

typedef struct pat_head
{
	uint8_t	m_uTableID;
	uint16_t	m_uSectLen;
	uint16_t	m_uTsId;
	uint8_t	m_uVersion;
	uint16_t m_uLoopLen;
	uint8_t	*m_pLoopData;
}PAT_HEAD;

typedef enum
{
	LOOP_DESC,
	LOOP_PAT,
	LOOP_PMT,
	LOOP_SDT,
	LOOP_NIT,
	LOOP_BAT,
	LOOP_EIT
}LOOPTYPE;

int ts_session_init( void );
int ts_session_release(void);
uint32_t ts_session_get_vts_number (void);
uint32_t ts_session_get_ats_number (void);
int ts_session_insert_rtp_packet (rtp_packet *pak);
int ts_session_get_audio_pes_packet(uint8_t** buffer, int32_t* len, uint32_t* timestamp);
int ts_session_get_video_pes_packet(uint8_t** buffer, int32_t* len, uint32_t* timestamp, int32_t* iframe);
int32_t ts_session_parse_avpid( uint8_t * pTsStream, uint16_t streamlen ,uint32_t *VideoPid, uint32_t *AudioPid, uint32_t *PcrPid);
int ts_session_clean_buffer (void);
int ts_session_sync(char* data, int len);


#endif
