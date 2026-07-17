#include "../../include/vod_in_common_def.h"
#include "../../include/vod_in_typedef.h"
#include "vod_trans_ts_parse.h"
#include "../../../vod_include/vod_decode_def.h"


/* 支持过滤重复的数据 */
#define TS_SESSION_SUPPORT_FILTER_REPEATED

//#define _DROP_ZERO_LEN_TS
#define FIND_PAYLOAD_START
#define CHECK_TS_SEQ

//#define DROP_IF_SEQ_ERROR
#define DROP_IF_NO_PAYLOAD_START

#define CHECK_RTP_SEQ 

#define SECTION_LENGTH(a,b)	(((uint16_t)((a)&0x0F) << 8) | ((uint16_t)(b)))
#define PID_LENGTH(a,b)	(((uint16_t)((a)&0x1F) << 8) | ((uint16_t)(b)))
#define USHORT_LENGTH(a,b)	(((uint16_t)(a) << 8) | ((uint16_t)(b)))
#define ULONG_LENGTH(a,b)	(((uint32_t)(a) << 16) | ((uint16_t)(b)))

#define VOD_SYS_TIMEOUT_INFINITY 0xffffffff

#define ts_session_pak_lock() 	if (session_info.lock){ \
	vod_porting_sem_wait(session_info.lock, VOD_SYS_TIMEOUT_INFINITY);\
}

#define ts_session_pak_unlock() 	if (session_info.lock){ \
	vod_porting_sem_send(session_info.lock);\
}

static ts_session_t session_info = {0};
static unsigned int ___ts_begin_time = 0;

int ts_session_pmt_is_ok(void)
{
	return session_info.pmt_ok;
}

static int GetPatHead(uint8_t* pSectionData, PAT_HEAD *pHead)
{

	if(NULL == pSectionData || NULL == pHead){
		return -1;
	}else{
		pHead->m_uTableID = pSectionData[0];
		pHead->m_uSectLen = SECTION_LENGTH(pSectionData[1], pSectionData[2]);
		pHead->m_uTsId = USHORT_LENGTH(pSectionData[3], pSectionData[4]);
		pHead->m_uVersion = (pSectionData[5] & 0x3E) >> 1;
		pHead->m_uLoopLen = pHead->m_uSectLen - 9;
		pHead->m_pLoopData = pSectionData + 8;
	}
	return 0;
}

static void _remove_ts_packet (ts_packet_t* pak, int video)
{

	ts_packet_t ** head, **tail;

	if (pak == NULL) return;

	if (video == 1)
	{
		head = &session_info.video_queue_head;
		tail = &session_info.video_queue_tail;

		session_info.v_ts_num--;
	}
	else
	{
		head = &session_info.audio_queue_head;
		tail = &session_info.audio_queue_tail;

		session_info.a_ts_num--;
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


static ts_packet_t *_get_one_audio_video_ts_packet ( int video ,int lock)
{
	ts_packet_t *packet = NULL;
	if(lock)
	{
		ts_session_pak_lock() ;
	}

	if ( video == 1 )
	{
		packet = session_info.video_queue_head;
	}
	else
	{
		packet = session_info.audio_queue_head;
	}

	_remove_ts_packet(packet, video);

	if(lock)
	{
		ts_session_pak_unlock() ;
	}
	return (packet);
}



static int _insert_video_audio_ts_packet( ts_packet_t *pak ,int video)
{
	ts_packet_t **head, **tail;

	ts_session_pak_lock();

	if (video == 1)
	{
		head = &session_info.video_queue_head;
		tail = &session_info.video_queue_tail;

		session_info.v_ts_num++;
	}
	else
	{
		head = &session_info.audio_queue_head;
		tail = &session_info.audio_queue_tail;

		session_info.a_ts_num++;
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

	ts_session_pak_unlock();

	return (1);
}

int ts_session_init( void )
{
	int rv = 0;

	if(session_info.have_init)
	{
		ts_session_release();
	}
	session_info.have_init = 1;

	vod_porting_sem_create(1, &session_info.lock);

	session_info.audio_pes_buffer = xmalloc(MAX_AUDIO_PES_PACKET_LEN);
	if(session_info.audio_pes_buffer == NULL)
	{
		goto error;
	}
	session_info.video_pes_buffer = xmalloc(MAX_VIDEO_PES_PACKET_LEN);
	if(session_info.video_pes_buffer == NULL)
	{
		goto error;
	}

	session_info.video_counter = -1;
	session_info.audio_counter = -1;
	session_info.audio_pid = 0;
	session_info.video_pid = 0;
	session_info.find_pat = 0;
	session_info.pmt_pid = 0;
	session_info.pmt_ok = 0;

	/* 按照dts进行TS的顺序判断 */
	session_info.audio_pes.dts = 0 ;
	session_info.video_pes.dts = 0 ;

	___ts_begin_time = vod_porting_get_ms();
	return rv;

error:

	ts_session_release();
	gxlogd("ts_session_init error\n");
	return -1;
}

int ts_session_sync(char* data, int len)
{
	int c = 0;
	int synced = 0;
	int pos = 0, first_pos=0;

resync:
	while(((c = data[pos]) != 0x47)){
		pos++;
		if(pos > len)
			return -1;
	}

	first_pos = pos;
	if(c == 0x47) {
		pos += TS_PACKET_LEN;
		if(pos < len){
			if(data[pos] != 0x47){
				pos = first_pos+1;
				goto resync;
			}
			else {
				pos += TS_PACKET_LEN;
				if(pos < len){
					if(data[pos] != 0x47){
						pos = first_pos+1;
						goto resync;
					}
					else {
						synced = 1;
					}
				}else{
					if(len == pos)
						synced = 1;
					else
						synced = 0;
				}
			}
		}else{
			if(len == pos)
				synced = 1;
			else
				synced = 0;
		}
	}
	else
		synced = 0;

	if(synced == 0)
		return -1;

	//gxlogd("[%s-%d]: find ts sync pos %d\n", __FUNCTION__, __LINE__, first_pos);
	return first_pos;
}

int ts_session_release(void)
{
	rtp_packet * pak = NULL;
	ts_packet_t *tspak = NULL;

	if(session_info.have_init ==0)
	{
		return 0;
	}

	session_info.have_init = 0;

	while(session_info.rtp_featue_head)
	{
		pak = session_info.rtp_featue_head;
		session_info.rtp_featue_head = pak->rtp_next;
		xfree(pak);
	}
	session_info.rtp_featue_num = 0;
	session_info.curr_rtp_seq = 0;


	tspak = _get_one_audio_video_ts_packet(1,0);
	while(tspak != NULL)
	{
		xfree(tspak);
		tspak = _get_one_audio_video_ts_packet(1,0);
	}

	tspak = _get_one_audio_video_ts_packet(0,0);
	while(tspak != NULL)
	{
		xfree(tspak);
		tspak = _get_one_audio_video_ts_packet(0,0);
	}


	session_info.video_queue_head = NULL;
	session_info.audio_queue_head = NULL;

	session_info.video_queue_tail = NULL;
	session_info.audio_queue_tail = NULL;
	session_info.v_ts_num = 0;
	session_info.a_ts_num = 0;
	session_info.video_pid = session_info.audio_pid = 0;
	if(session_info.video_pes_buffer != NULL)
	{
		xfree(session_info.video_pes_buffer);
		session_info.video_pes_buffer = NULL;
	}

	if(session_info.audio_pes_buffer != NULL)
	{
		xfree(session_info.audio_pes_buffer);
		session_info.audio_pes_buffer = NULL;
	}
	if(session_info.lock)
	{
		vod_porting_sem_delete(session_info.lock);
	}

	memset(&session_info,0,sizeof(session_info));

	return 0;
}



void ts_session_set_avpid( unsigned short vpid, unsigned short apid ,unsigned short pcr_pid)
{
	session_info.video_pid = vpid;
	session_info.audio_pid = apid;
	session_info.pcr_pid = pcr_pid;
}

unsigned short ts_get_video_pid( void )
{
	return session_info.video_pid ;
}
unsigned short ts_get_audio_pid( void )
{
	return session_info.audio_pid ;
}
unsigned short ts_get_pcr_pid( void )
{
	return session_info.pcr_pid ;
}

static ts_packet_t*  __get_one_ts_packet(unsigned char * data_p )
{
	unsigned char * pdata = NULL;
	ts_packet_t* tspak = NULL;
	if( data_p == NULL)
	{
		gxlogd("[__get_one_ts_packet]data_p==NULL\n");
		return NULL;
	}

	if(*data_p!=0x47)
	{
		gxlogd("[__get_one_ts_packet]!=0x47\n");
		return NULL;
	}

	tspak = xmalloc(sizeof(ts_packet_t)+TS_PACKET_LEN);
	if(tspak == NULL)
	{
		gxlogd("[__get_one_ts_packet]xmlloc err1\n");
		return NULL;
	}
	tspak->sync_byte = data_p[0];

	if((data_p[1] & 0x80) != 0)
	{
		tspak->err_packet = 1;
	}
	else
	{
		tspak->err_packet = 0;
	}

	if((data_p[1] & 0x40) != 0)
	{
		tspak->payload_start = 1;
	}
	else
	{
		tspak->payload_start = 0;
	}
	tspak->pid = ((((uint16_t)(data_p[1] & 0x1f)) << 8) | ((uint16_t)data_p[2]));


	if ( ( data_p[3]&0x30 ) == 0x30)
	{
		tspak->data_len = TS_PACKET_LEN - 4 - 1 - data_p[4];
		pdata = data_p + TS_PACKET_LEN - tspak->data_len;
	}
	else if(( data_p[3]&0x30 ) == 0x10)
	{
		tspak->data_len = 184;
		pdata = data_p + TS_PACKET_LEN - tspak->data_len;
	}
	else
	{
		tspak->data_len = 0;
	}

	tspak->counter = ( data_p[3]&0x0f );
	if ( tspak->data_len > 0)
	{
		memcpy(tspak+1,pdata,tspak->data_len);	
	}
	else
	{
#ifdef _DROP_ZERO_LEN_TS
		xfree(tspak);
		return NULL;
#endif
	}

	return tspak;
}


int insert_data_to_ts(unsigned char *data,int data_len)
{
	ts_packet_t* packet = NULL;
	int packetn;


	packetn = data_len/TS_PACKET_LEN;
	if(data_len % TS_PACKET_LEN != 0)
	{
		gxlogd("may be error packet ,not n * 188\n");
	}

	for(; packetn > 0; packetn--)
	{
		packet = __get_one_ts_packet(data);
		if ( packet == NULL)
		{
			data = data + TS_PACKET_LEN;
			continue;
		}

		if ( packet->pid == session_info.video_pid)
		{
			_insert_video_audio_ts_packet(packet,1);
		}
		else if( packet->pid == session_info.audio_pid )
		{
			_insert_video_audio_ts_packet(packet,0);
		}
		else
		{
			xfree(packet);
			packet = NULL;
			data = data + TS_PACKET_LEN;

			continue;
		}

		data = data + TS_PACKET_LEN;	
	}

	session_info.tot_rcv_rtp_num++;
	return 0;
}


int featue_rtp_insert(rtp_packet *pak)
{
	rtp_packet*rpk;
	rtp_packet*next;

	pak->rtp_next = NULL;
	session_info.rtp_featue_num++;

	if(session_info.rtp_featue_head == NULL)
	{
		session_info.rtp_featue_head = pak;
		return 0;
	}
	else
	{
		rpk = session_info.rtp_featue_head;
		if(chk_seq_lit(pak->rtp_pak_seq, rpk->rtp_pak_seq))
		{
			/*插入到头上*/
			pak->rtp_next = session_info.rtp_featue_head;
			session_info.rtp_featue_head = pak;

			gxlogd("1ts_session_insert_rtp_packet : udp rtp 数据包重排序起作用:%d %d\n",pak->rtp_pak_seq,rpk->rtp_pak_seq);
			return 0;
		}

		while(rpk)
		{
			next = rpk->rtp_next;
			if(next)
			{
				if(chk_seq_lit(pak->rtp_pak_seq, next->rtp_pak_seq))
				{
					rpk->rtp_next = pak;
					pak->rtp_next = next;
					gxlogd("2ts_session_insert_rtp_packet : udp rtp 数据包重排序起作用:%d %d\n",pak->rtp_pak_seq,next->rtp_pak_seq);
					return 0;
				}
			}
			else
			{
				/*插在末尾*/
				rpk->rtp_next = pak;
				return 0;
			}
			rpk = next;
		}
	}
	return 0;
}

rtp_packet*featue_rtp_get(void)
{
	rtp_packet *head;
	//	rtp_packet *next;

	if(session_info.rtp_featue_head == NULL)
	{
		if(session_info.rtp_featue_num!=0)
		{
			gxlogd("featue_rtp_get err1\n");
		}

		session_info.rtp_featue_num = 0;
		return NULL;
	}
	head = session_info.rtp_featue_head;

	if(session_info.rtp_featue_num >= chk_seq_max)
	{
		goto get_head;
	}

	if(chk_seq_continue(session_info.curr_rtp_seq,head->rtp_pak_seq))
	{
		goto get_head;
	}

	return NULL;

get_head:

	session_info.rtp_featue_head = head->rtp_next;
	session_info.rtp_featue_num--;
	return head;
}

int ts_session_insert_rtp_packet (rtp_packet *pak)
{
	/* rtp_packet *q; */
	int data_len = 0;
	//	int packetn = 0;
	unsigned char *data = NULL;
	//	ts_packet_t* packet = NULL;
	//	int first = 1;
	uint32_t pVidPid, pAudPid, pPcrPid;
	int ret;
	int pak_len;
	if ( pak == NULL ) return -1;

	pak_len = pak->rtp_data_len;

	/*lu_yuefei : 兼容不带rtp头的 ts 流。*/
	if((pak_len%188))
	{
		if(pak->rtp_pak_x)
		{
			uint32_t extlen;
			rtp_packet_ext_header* ext = (rtp_packet_ext_header*)((uint8_t *)pak + RTP_PACKET_HEADER_SIZE + sizeof(rtp_packet_header));
			ext->length = vod_porting_ntohs(ext->length);

			extlen = 4 + ext->length*4;

			pak->rtp_data_len = pak_len - sizeof(rtp_packet_header) - extlen;
			pak->rtp_data = (uint8_t *)ext + extlen;
			pak->rtp_pak_seq  = vod_porting_ntohs(pak->rtp_pak_seq);
			pak->rtp_pak_ts   = vod_porting_ntohl(pak->rtp_pak_ts);
			if(session_info.print_flag==0)
			{
				session_info.print_flag=1;
				gxlogd("[AASSDD] [rtp头 + 扩展头 %d] + 数据\n",extlen);
			}
		}
		else
		{
			pak->rtp_data_len = pak_len - sizeof(rtp_packet_header);
			pak->rtp_data = (uint8_t *)pak + RTP_PACKET_HEADER_SIZE + sizeof(rtp_packet_header);
			pak->rtp_pak_seq  = vod_porting_ntohs(pak->rtp_pak_seq);
			pak->rtp_pak_ts   = vod_porting_ntohl(pak->rtp_pak_ts);
			if(session_info.print_flag==0)
			{
				session_info.print_flag=1;
				gxlogd("[AASSDD] [rtp头] + 数据\n");
			}
		}
	}
	else
	{
		pak->rtp_data_len = pak_len;
		pak->rtp_data = (uint8_t *)pak + RTP_PACKET_HEADER_SIZE;
		if(session_info.print_flag==0)
		{
			session_info.print_flag=1;
			gxlogd("[AASSDD]  不带头\n");
		}
	}

	session_info.recv_rtp_length+=pak_len;
	if(session_info.recv_rtp_length<5000)
	{
		gxlogd("ts_session_insert_rtp_packet rtp_len=%d\n",pak_len);
	}

	if(session_info.video_pid == 0 || session_info.audio_pid == 0)
	{
		ret = ts_session_parse_avpid(pak->rtp_data, pak->rtp_data_len, &pVidPid, &pAudPid, &pPcrPid);
		if(ret == 0)
		{
			ts_session_set_avpid((unsigned short)pVidPid,(unsigned short)pAudPid,(unsigned short)pPcrPid);
		}
    	gxlogd("a_pid %d, v_pid %d pcr_pid %d\n", pAudPid, pVidPid, pPcrPid);
	}

	/*
	   统计丢包率，解决udp乱序
	   */
	data = pak->rtp_data;
	data_len = pak->rtp_data_len;

	if(data == ((uint8_t*)pak + RTP_PACKET_HEADER_SIZE))
	{
		/*不带rtp头的数据，暂时不做处理*/
		insert_data_to_ts(data,data_len);
		xfree(pak);
		return 0;
	}
	else
	{
		if(session_info.tot_rcv_rtp_num < 5)
		{
			gxlogd("[ASDCF]pak->rtp_pak_seq=%d\n",pak->rtp_pak_seq);
		}

		if(chk_seq_continue(session_info.curr_rtp_seq,pak->rtp_pak_seq))
		{
			session_info.curr_rtp_seq = pak->rtp_pak_seq;
			insert_data_to_ts(data,data_len);
			xfree(pak);

			pak=featue_rtp_get();
			if(pak)
			{
				gxlogd("[%s][%d]ts_session_insert_rtp_packet %d: udp rtp %d 数据包重排序起作用\n",
						__FILE__ , __LINE__ , session_info.curr_rtp_seq,pak->rtp_pak_seq);
			}
		}
		else
		{
			if(session_info.rtp_featue_num==0)
			{
				gxlogd("[%s][%d][KKLLJJ]curr_rtp_seq=%d,rtp_pak_seq=%d\n",
						__FILE__ , __LINE__ , session_info.curr_rtp_seq,pak->rtp_pak_seq);
			}

			featue_rtp_insert(pak);
			pak=featue_rtp_get();
		}
	}

	while(pak)
	{
		data = pak->rtp_data;
		data_len = pak->rtp_data_len;
		session_info.curr_rtp_seq = pak->rtp_pak_seq;
		insert_data_to_ts(data,data_len);
		xfree(pak);
		pak=featue_rtp_get();
	}

	return (0);
}


int ts_session_clean_buffer (void)
{
	rtp_packet *pak = NULL;
	ts_packet_t * tspak = NULL;

	ts_session_pak_lock();

	while(session_info.rtp_featue_head)
	{
		pak = session_info.rtp_featue_head;
		session_info.rtp_featue_head = pak->rtp_next;
		xfree(pak);
	}
	session_info.rtp_featue_num = 0;
	session_info.curr_rtp_seq = 0;

	tspak = _get_one_audio_video_ts_packet(1,0);
	while(tspak != NULL)
	{
		xfree(tspak);
		tspak = _get_one_audio_video_ts_packet(1,0);
	}

	tspak = _get_one_audio_video_ts_packet(0,0);
	while(tspak != NULL)
	{
		xfree(tspak);
		tspak = _get_one_audio_video_ts_packet(0,0);
	}

	session_info.video_pes_len = 0;
	session_info.audio_pes_len = 0;
	session_info.video_ts_buffer_len = 0;
	session_info.audio_ts_buffer_len = 0;

	session_info.video_pes_ok = 0;
	session_info.audio_pes_ok = 0;
	session_info.v_ts_num = 0;
	session_info.a_ts_num = 0;

	session_info.video_counter = -1;
	session_info.audio_counter = -1;


	session_info.video_timestamp_num = 0;
	session_info.video_timestamp_index = 0;

	/*这里是否需要增加的 */
	memset(session_info.video_timestamps, 0, sizeof(session_info.video_timestamps));
	session_info.audio_timestamp_num = 0;
	session_info.audio_timestamp_index = 0;
	memset(session_info.audio_timestamps, 0, sizeof(session_info.audio_timestamps));

	session_info.have_one_pes = 0;

	/* 按照dts进行TS的顺序判断 */
	session_info.audio_pes.dts = 0 ;
	session_info.video_pes.dts = 0 ;

	ts_session_pak_unlock();

	return 0;
}


#define invalid_ts_seq(bak,curr) ((bak)==(curr) || (bak) == ((curr) + 15 ) % 16)
#define ts_copy_data(dst,off,src,len,off_max) \
	if(len) \
{ \
	if((off+len)<=off_max)\
	{\
		memcpy(dst+off,src,len); \
		off=off+len; \
	}\
	else\
	{\
		gxlogd("超出范围:%d\n",off_max);\
		*bak_ts_seq = -1;\
		return 0;\
	}\
} \
else \
{}




static int _ts_get_pes(ts_packet_t * pak,uint8_t*pes,int32_t*len,uint32_t pes_max,uint8_t*bak_buf,int32_t*bak_buff_len,char*bak_ts_seq)
{
	//	int ok=0;
	//	uint32_t tmp;
	//	int copy_len;

	if(*bak_ts_seq == -1)
	{
		/*头*/
#ifdef FIND_PAYLOAD_START
		if (pak->payload_start != 1)
		{
			/*没有找到头*/
			gxlogd("Error [%s][%d]_ts_get_pes:payload_start!=1\n" , __FILE__ , __LINE__ );

#ifdef DROP_IF_NO_PAYLOAD_START
			return -1;
#endif
		}
#endif

		/*pes 头开始拼装，清除参数*/
		*len = 0;
		*bak_buff_len = 0;

		ts_copy_data(pes , *len,pak+1,pak->data_len,pes_max);

		*bak_ts_seq = pak->counter;
		return -2;
	}

	ts_copy_data(pes , *len,bak_buf,*bak_buff_len,pes_max);
	*bak_buff_len = 0;

	if ( pak->err_packet == 1 )
	{
		gxlogd("[%s][%d]a error ts packet\n" , __FILE__ , __LINE__ );
		if ( pak->payload_start == 1 && (*len > 0))
		{
			*bak_ts_seq = -1;
			return 0;
		}
		return -1;
	}

	if ( pak->payload_start == 1 && (*len > 0))
	{
		ts_copy_data(bak_buf , *bak_buff_len,pak+1,pak->data_len,pes_max);
		*bak_ts_seq = pak->counter;
		return 0;
	}


#if defined DROP_IF_SEQ_ERROR || defined CHECK_TS_SEQ
	if (!invalid_ts_seq(*bak_ts_seq,pak->counter))
	{
		gxlogd("invalid_ts_seq old counter:%d new:%d\n",*bak_ts_seq,pak->counter);

#ifdef DROP_IF_SEQ_ERROR
		*bak_ts_seq = -1;
		return -1;
#endif

	}
#endif

	*bak_ts_seq = pak->counter;
	ts_copy_data(pes , *len,pak+1,pak->data_len,pes_max);

	return -2;
}

void ts_session_release_video_pes(void)
{
	session_info.video_pes_len = 0;
	session_info.video_pes_ok = 0;
}
void ts_session_release_audio_pes(void)
{
	session_info.audio_pes_len = 0;
	session_info.audio_pes_ok = 0;
}

static int _parse_pes(ts_session_pes_t *pes )
{
	unsigned char * p = pes->data;
	uint32_t pts = 0;
	uint32_t dts = 0 ;

	int head_len;
	unsigned char type_byte;
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

		gxlogd("dts=%u pts=%u\n" , dts , pts );

		if ( (pes->dts >= dts && pes->dts != 0) )
		{
			gxlogd("-------predts=%u dts=%u\n", pes->dts , dts );

			//return 0 ;
		}
		else
		{
			pes->dts = dts ;
		}
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

	pes->timestamp = pts;
	pes->len_in_head =  ((p[4]<<8) | p[5]);

	if(pes->frame_content == TS_SESSION_PES_CONTENT_VIDEO
			&& pes->encode_type == VOD_VIDEO_H264)
	{
		head_len = p[8];
		type_byte = p[head_len + 9 +4];

		if ( (type_byte & 0x60) == 0x60 )
		{
			pes->frame_type = TS_SESSION_PES_FRAME_I;
		}
		else
		{
			/*暂时不用，写死为P*/
			pes->frame_type = TS_SESSION_PES_FRAME_P;
		}
	}

	return 0;
}


int ts_session_get_pes(ts_session_frame_content_t frame_content,ts_session_pes_t *pes)
{
	ts_packet_t * pak = NULL;
	int ret=-1;
	int counts = 0 ;

	if(pes ==NULL)
	{
		return -1;
	}

	switch(frame_content)
	{
		case TS_SESSION_PES_CONTENT_AUDIO:


			if(session_info.audio_pes_ok)
			{
				memcpy(pes,&session_info.audio_pes,sizeof(ts_session_pes_t));
				return 0;
			}

			do
			{
				pak = _get_one_audio_video_ts_packet(frame_content,1);

				if(pak)
				{
					counts ++ ;

					ret = _ts_get_pes(
							pak
							,session_info.audio_pes_buffer
							,&session_info.audio_pes_len
							,MAX_AUDIO_PES_PACKET_LEN
							,session_info.audio_ts_buffer
							,&session_info.audio_ts_buffer_len
							,&session_info.audio_counter);

					xfree(pak);

					if(ret==0)
					{
						session_info.audio_pes.data = session_info.audio_pes_buffer;
						session_info.audio_pes.encode_type = session_info.audio_type;
						session_info.audio_pes	.len = 	session_info.audio_pes_len;
						session_info.audio_pes	.frame_content = frame_content;
						ret = _parse_pes(&session_info.audio_pes);
						if ( ret == 0 )
						{
							memcpy(pes,&session_info.audio_pes,sizeof(ts_session_pes_t));
							session_info.audio_pes_ok = 1;
							return 0;
						}
						else
						{
							ts_session_release_audio_pes();
							return -1 ;
						}
					}
				}
			}while( pak );
			break;

		case TS_SESSION_PES_CONTENT_VIDEO:
			if(session_info.video_pes_ok)
			{
				memcpy(pes,&session_info.video_pes,sizeof(ts_session_pes_t));
				return 0;
			}

			do
			{

				pak = _get_one_audio_video_ts_packet(frame_content,1);
				if(pak)
				{
					counts ++ ;

					ret = _ts_get_pes(
							pak
							,session_info.video_pes_buffer
							,&session_info.video_pes_len
							,MAX_VIDEO_PES_PACKET_LEN
							,session_info.video_ts_buffer
							,&session_info.video_ts_buffer_len
							,&session_info.video_counter);

					xfree(pak);

					if(ret==0)
					{
						session_info.video_pes.data = session_info.video_pes_buffer;
						session_info.video_pes.encode_type = session_info.video_type;
						session_info.video_pes	.len = 	session_info.video_pes_len;
						session_info.video_pes	.frame_content = frame_content;
						ret = _parse_pes(&session_info.video_pes);
						if ( ret == 0 )
						{
							memcpy(pes,&session_info.video_pes,sizeof(ts_session_pes_t));
							session_info.video_pes_ok = 1;
							return 0;
						}
						else
						{
							ts_session_release_video_pes();
							return -1 ;
						}
					}
				}
			}while( pak );
			break;

		default:
			gxlogd("ts_session_get_pes err 4\n");
			return -1;
			break;
	}

	return ret;
}



uint32_t ts_session_get_vts_number (void)
{
	return session_info.v_ts_num;
}

uint32_t ts_session_get_ats_number (void)
{
	return session_info.a_ts_num;
}


int isma_ts_audio_pids_init(void)
{
	memset(&session_info.audio_pids,0,sizeof(isma_ts_audio_pids_t));
	return 0;
}

int isma_ts_audio_pids_add(uint16_t pid,int aud_type)
{
	int i;
	i = session_info.audio_pids.pid_num;
	if(i >= 6)
	{
		session_info.audio_pids.pid_num = 6;
		i= 0;
	}
	else
	{
		session_info.audio_pids.pid_num = i+1;
	}
	session_info.audio_pids.pids[i] = pid;
	session_info.audio_pids.aud_type[i] = aud_type;

	session_info.audio_pids.curr_pid = pid;
	return 0;
}

int isma_ts_audio_pids_get(uint16_t*pid_num,uint16_t *pids,int *aud_type,uint16_t *curr_pid)
{
	int i;
	*pid_num = session_info.audio_pids.pid_num;
	for(i=0;i<*pid_num;i++)
	{
		pids[i] = session_info.audio_pids.pids[i];
		switch(session_info.audio_pids.aud_type[i])
		{
			case VOD_AUDIO_MPEG1:
				aud_type[i]=VOD_AUDIO_MPEG1;
				break;	

			case VOD_AUDIO_MPEG2:
				aud_type[i]=VOD_AUDIO_MPEG1;
				break;

			default:
				aud_type[i]=VOD_AUDIO_AAC;
				break;
		}
	}
	*curr_pid = session_info.audio_pids.curr_pid;
	return 0;
}



static int _parse_pmt(const uint8_t *Buffer, uint32_t *VideoPid, uint32_t *AudioPid, uint32_t *PcrPid)
{
	int32_t SectionLen, ProgramInfoLen, AllStreamInfoLen,StreamInfoLen;
	const uint8_t *pData;
	isma_ts_audio_pids_init();
	*VideoPid = STPTI_InvalidPid();
	*AudioPid = STPTI_InvalidPid();
	*PcrPid = STPTI_InvalidPid();

	SectionLen = Buffer[2];
	*PcrPid = (uint32_t)(Buffer[8]&0x1f)<<8;
	*PcrPid += (uint32_t)Buffer[9];

	ProgramInfoLen = (uint32_t)((Buffer[10]&0x0f)<<8);
	ProgramInfoLen += Buffer[11];
	gxlogd("ProgramInfoLen  :%d\n", ProgramInfoLen);
	AllStreamInfoLen = SectionLen -9 - ProgramInfoLen-4;
	gxlogd("AllStreamInfoLen  :%d \n", AllStreamInfoLen);

	pData = &Buffer[12+ProgramInfoLen];
	while (AllStreamInfoLen>0)
	{
		/*mpeg4以及mpeg2的定义里面 2 代表的是video pid*/
		if ( (pData[0] == 2) && (*VideoPid == STPTI_InvalidPid() ) )
		{
			*VideoPid = (pData[1]&0x1f)<<8;
			*VideoPid += pData[2];
			session_info.video_type  = VOD_VIDEO_MPEG2;
		}
		/*264定义里面 0x1b 代表的是video pid*/
		else if ( (pData[0] == /*2*/0x1b) && (*VideoPid == STPTI_InvalidPid() ) )
		{
			*VideoPid = (pData[1]&0x1f)<<8;
			*VideoPid += pData[2];
			session_info.video_type = VOD_VIDEO_H264;

		}
		else if ( (pData[0] == 3) && (*AudioPid == STPTI_InvalidPid()) )
		{
			*AudioPid = (pData[1]&0x1f)<<8;
			*AudioPid += pData[2];
			session_info.audio_type = VOD_AUDIO_MPEG1;
			isma_ts_audio_pids_add(*AudioPid,session_info.audio_type);
		}
		else if ( (pData[0] == 4) && (*AudioPid == STPTI_InvalidPid()) )
		{
			*AudioPid = (pData[1]&0x1f)<<8;
			*AudioPid += pData[2];
			session_info.audio_type = VOD_AUDIO_MPEG2;
			isma_ts_audio_pids_add(*AudioPid,session_info.audio_type);
		}
		else if ( (pData[0] == 0xf || pData[0] == 0x11) && (*AudioPid == STPTI_InvalidPid()) )
		{
			*AudioPid = (pData[1]&0x1f)<<8;
			*AudioPid += pData[2];
			session_info.audio_type = VOD_AUDIO_AAC;
			isma_ts_audio_pids_add(*AudioPid,session_info.audio_type);
		}
		else if(pData[0] == 0x81)
		{
			*AudioPid = (pData[1]&0x1f)<<8;
			*AudioPid += pData[2];
			session_info.audio_type = VOD_AUDIO_AC3;
		}

		if((*VideoPid != STPTI_InvalidPid()) && (*AudioPid != STPTI_InvalidPid()))
		{
			break;
		}

		StreamInfoLen = (pData[3]&0x0f)<<8;
		StreamInfoLen += pData[4];
		pData = pData + 5+ StreamInfoLen;
		AllStreamInfoLen = AllStreamInfoLen - 5 - StreamInfoLen;
	}

	if ( (*VideoPid == STPTI_InvalidPid() ) 
			|| (*AudioPid== STPTI_InvalidPid() )
			|| (*PcrPid == STPTI_InvalidPid() ) )
	{
		gxlogd("_parse_pmt err\n");
		return -1;
	}
	else
	{
		session_info.pmt_ok = 1;
		return 0;
	}
	return -1;
}
static int	GetLoopData(unsigned char ** ioData, unsigned short * ioInfoLength, int eType, unsigned char * * opDesc)
{
	//	unsigned char	*pResult = NULL;
	unsigned short		length = 0;

	if (*ioInfoLength == 0)
	{
		return FALSE;
	}

	// 循环内长度
	switch(eType)
	{
		case LOOP_DESC:
			length = (*ioData)[1] + 2;		//为什么+2?
			break;
		case LOOP_PAT:
			length = 4;
			break;
		case LOOP_PMT:
		case LOOP_SDT:
			length = SECTION_LENGTH((*ioData)[3], (*ioData)[4]);
			length += 5;
			break;
		case LOOP_NIT:
		case LOOP_BAT:
			length = SECTION_LENGTH((*ioData)[4], (*ioData)[5]);
			length += 6;
			break;
		case LOOP_EIT:
			length = SECTION_LENGTH((*ioData)[10], (*ioData)[11]);
			length += 12;
			break;
		default:
			break;
	}

	// 如果循环内长度为0，则返回FALSE
	if (length == 0)
	{
		return FALSE;
	}

	// 如果循环内长度大于总长度，则返回FALSE
	if (length > *ioInfoLength)
	{
		*ioData += length - *ioInfoLength;
		return FALSE;
	}

	*opDesc = *ioData;

	*ioInfoLength -= length;
	*ioData += length;

	return TRUE;
}

int32_t ts_session_parse_avpid( uint8_t * pTsStream, uint16_t streamlen ,uint32_t *VideoPid, uint32_t *AudioPid, uint32_t *PcrPid)
{
	uint16_t u16TsPid = 0;
	uint8_t u8PayloadUnitStartIndicator = 0;	
	uint8_t *pTempData = NULL;
	//	uint8_t u8Index = 0;
	uint32_t u32Count = 0;
	uint32_t i = 0;
	int ret = -1;
	unsigned char adaption_control = 0;
	unsigned char adaption_len = 0;

	uint8_t * s_pData = NULL;
	uint16_t s_SectLen = 0;
	PAT_HEAD pPatHead = {0};

	if( NULL == pTsStream  )
	{
		return -1;
	}

	if(((streamlen%TS_PACKET_LEN) != 0) ||  (streamlen < TS_PACKET_LEN))
	{
		return -1;
	}

	u32Count = streamlen/TS_PACKET_LEN;


	for(i=0; i < u32Count; i++)
	{
		if( pTsStream[i*TS_PACKET_LEN] != 0x47 )
		{
			continue;
		}
		adaption_len = 0;

		u16TsPid = (((uint16_t)(pTsStream[i*TS_PACKET_LEN+1] & 0x1f)) << 8) | ((uint16_t)pTsStream[i*TS_PACKET_LEN+2]);
		u8PayloadUnitStartIndicator = pTsStream[i*TS_PACKET_LEN+1] & 0x40;
		adaption_control = ((pTsStream[i*TS_PACKET_LEN+3] & 0x30)>>4);
		if (adaption_control == 3 )
		{
			adaption_len = pTsStream[i*TS_PACKET_LEN+4] + 1;
		}
		else if (adaption_control == 2 )
		{
			continue;
		}
		if(0 == u16TsPid)
		{
			gxlogd("u16TsPid:%d---adaption control:%d\n",u16TsPid,adaption_control);
		}
		if( 0 == u8PayloadUnitStartIndicator)
		{
			continue;
		}

		s_SectLen = SECTION_LENGTH(pTsStream[i*TS_PACKET_LEN+6+adaption_len], pTsStream[i*TS_PACKET_LEN+7+adaption_len]);	
		if(0 == u16TsPid)
		{
			if(session_info.find_pat == 1)
			{
				continue;
			}
			if( s_SectLen > 180 )
			{
				gxlogd(" 1s_SectLen > 180   : %s :%d\n",__FILE__,s_SectLen);
				continue;
			}

			s_pData = xmalloc(s_SectLen + 3);
			memcpy(s_pData, pTsStream+i*TS_PACKET_LEN + 5+adaption_len,  s_SectLen + 3);

			GetPatHead(s_pData, &pPatHead);
			while (GetLoopData(&(pPatHead.m_pLoopData), &(pPatHead.m_uLoopLen), LOOP_PAT, &pTempData))
			{
				session_info.pmt_pid = PID_LENGTH(pTempData[2], pTempData[3]);
				session_info.find_pat = 1;
			}

			xfree(s_pData);
			s_pData = NULL;
			s_SectLen = 0;
			continue;
		}
		else
		{
			if(session_info.find_pat == 0 || session_info.pmt_pid != u16TsPid)
			{
				continue;
			}


			if( s_SectLen > 180 )
			{
				gxlogd(" 2s_SectLen > 180   : %s\n",__FILE__);
				continue;
			}

			s_pData = xmalloc(s_SectLen + 3);
			memcpy(s_pData, pTsStream+i*TS_PACKET_LEN + 5+adaption_len,  s_SectLen + 3);

			ret = _parse_pmt(s_pData, VideoPid, AudioPid, PcrPid);

			xfree(s_pData);
			s_pData = NULL;
			s_SectLen = 0; 

			if(ret == 0)
			{
				break;
			}
			else
			{
				continue;
			}
		}
	}	

	return ret;
}

int ts_session_video_packet_overlaped( unsigned int stamp )
{
	int i;
	int j=0;
	int num;
	int index;
	uint32_t *p;
	num = session_info.video_timestamp_num;
	index = session_info.video_timestamp_index;
	p = session_info.video_timestamps;

	index = (index + MAX_SAVE_TIMESTAMP - num)%MAX_SAVE_TIMESTAMP;

	for(i=0;i<num;i++)
	{
		j = (index + i)%MAX_SAVE_TIMESTAMP;
		if(p[j] == stamp)
		{
			if(j == session_info.video_timestamp_index)
			{
				return 2;
			}
			else
			{
				return 1;
			}
		}
	}
	return -1;
}


void ts_session_save_video_stamp( unsigned int stamp  )
{
	int num;
	int index;
	uint32_t *p;
	num = session_info.video_timestamp_num;
	index = session_info.video_timestamp_index;
	p = session_info.video_timestamps;

	index=(index + 1)%MAX_SAVE_TIMESTAMP;;
	p[index] = stamp;

	if(num<MAX_SAVE_TIMESTAMP)
	{
		session_info.video_timestamp_num = num + 1;
	}

	session_info.video_timestamp_index = index;
}



int ts_session_audio_packet_overlaped( unsigned int stamp )
{
	int i;
	int j=0;
	int num;
	int index;
	uint32_t *p;
	num = session_info.audio_timestamp_num;
	index = session_info.audio_timestamp_index;
	p = session_info.audio_timestamps;
	index = (index + MAX_SAVE_TIMESTAMP - num)%MAX_SAVE_TIMESTAMP;

	for(i=0;i<num;i++)
	{
		j = (index + i)%MAX_SAVE_TIMESTAMP;
		if(p[j] == stamp)
		{
			if(j == session_info.audio_timestamp_index)
			{
				return 2;
			}
			else
			{
				return 1;
			}
		}
	}
	return -1;
}


void ts_session_save_audio_stamp( unsigned int stamp  )
{
	int num;
	int index;
	uint32_t *p;
	num = session_info.audio_timestamp_num;
	index = session_info.audio_timestamp_index;
	p = session_info.audio_timestamps;

	index=(index + 1)%MAX_SAVE_TIMESTAMP;;
	p[index] = stamp;

	if(num<MAX_SAVE_TIMESTAMP)
	{
		session_info.audio_timestamp_num = num + 1;
	}

	session_info.audio_timestamp_index = index;
}


int ts_session_get_video_pes_packet(uint8_t** buffer, int32_t* len, uint32_t* timestamp, int32_t* iframe)
{
	unsigned int videonpt = 0;
	int result = -1;

	ts_session_pes_t pes;

	if(session_info.video_pes_ok)
	{
		ts_session_release_video_pes();
	}

	result = ts_session_get_pes(TS_SESSION_PES_CONTENT_VIDEO,&pes);
	if (result < 0)
	{
		return -1;
	}

	videonpt = pes.timestamp;

	if(pes.frame_type == TS_SESSION_PES_FRAME_I)
	{
		gxlogd("%s:%d videonpt--I--:%d\n", __FILE__, __LINE__, videonpt);
		*iframe = 1;
	}
	else
	{
		gxlogd("%s:%d videonpt:%d\n", __FILE__, __LINE__, videonpt);
		*iframe = 0;
	}

#ifdef  TS_SESSION_SUPPORT_FILTER_REPEATED
	if ( ts_session_video_packet_overlaped(videonpt) == 1 )
	{
		gxlogd("ts_session_video_packet_overlaped\n");
		ts_session_release_video_pes();
		return -2;
	}
#endif

	ts_session_save_video_stamp(videonpt);
	ts_session_release_video_pes();
	*timestamp = videonpt;
	*len = pes.len;
	*buffer = pes.data;
	return 0;
}



int ts_session_get_audio_pes_packet(uint8_t** buffer, int32_t* len, uint32_t* timestamp)
{
	unsigned int audionpt = 0;
	int result = -1;

	ts_session_pes_t pes;

	if(session_info.audio_pes_ok)
	{
		ts_session_release_audio_pes();
	}

	result = ts_session_get_pes(TS_SESSION_PES_CONTENT_AUDIO,&pes);
	if (result < 0 )
	{
		return -1;
	}

	audionpt = pes.timestamp;

#ifdef  TS_SESSION_SUPPORT_FILTER_REPEATED
	if ( ts_session_audio_packet_overlaped(audionpt) == 1 )
	{
		gxlogd("ts_session_audio_packet_overlaped\n");
		ts_session_release_audio_pes();
		return -2;
	}
#endif

	if (pes.len_in_head != (pes.len-6))
	{
		gxlogd("%s %d [SSDDD]audio_pes error a packet length:%d---%d,audionpt=%d\n", __FILE__, __LINE__, pes.len_in_head,(pes.len-6),audionpt);
	}

	ts_session_save_audio_stamp(audionpt);
	ts_session_release_audio_pes();
	*timestamp = audionpt;
	*len = pes.len;
	*buffer = pes.data;

	return 0;
}

char ts_session_get_video_type( void )
{
	return session_info.video_type;
}
char ts_session_get_audio_type( void )
{
	return session_info.audio_type;
}



