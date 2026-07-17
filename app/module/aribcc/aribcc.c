#include "gxaribcc.h"
#include "aribcc.h"

#if ARIBCC_SUPPORT
#include "gxcore.h"
#include "assert.h"

#define ARIB_NUL    0x00
#define ARIB_BEL    0x07
#define ARIB_APB    0x08
#define ARIB_APF    0x09
#define ARIB_APD    0x0A
#define ARIB_APU    0x0B
#define ARIB_CS     0x0C
#define ARIB_APR    0x0D
#define ARIB_LS1    0x0E
#define ARIB_LS0    0x0F
#define ARIB_PAPF   0x16
#define ARIB_CAN    0x18
#define ARIB_SS2    0x19
#define ARIB_ESC    0x1B
#define ARIB_APS    0x1C
#define ARIB_SS3    0x1D
#define ARIB_RS     0x1E
#define ARIB_US     0x1F
#define ARIB_SP     0x20

#define ARIB_DEL    0x7F
#define ARIB_BKF    0x80
#define ARIB_RDF    0x81
#define ARIB_GRF    0x82
#define ARIB_YLF    0x83
#define ARIB_BLF    0x84
#define ARIB_MGF    0x85
#define ARIB_GNF    0x86
#define ARIB_WHF    0x87
#define ARIB_SSZ    0x88
#define ARIB_MSZ    0x89
#define ARIB_NSZ    0x8A
#define ARIB_SZX    0x8B
#define ARIB_COL    0x90
#define ARIB_FLC    0x91
#define ARIB_CDC    0x92
#define ARIB_POL    0x93
#define ARIB_WMM    0x94
#define ARIB_MACRO  0x95
#define ARIB_HLC    0x97
#define ARIB_RPC    0x98
#define ARIB_SPL    0x99
#define ARIB_STL    0x9A
#define ARIB_CSI    0x9B
#define ARIB_TIME   0x9D
#define ARIB_10_0   0xA0

#define G0_CHAR_SET_SET 0
#define G1_CHAR_SET_SET 0
#define ARIB_USE_ALPHANUMERIC_TABLE 0x4A
#define ARIB_USE_KATAKANA_TABLE 0x31
#define ARIB_USE_HIRAGANA_TABLE 0x30

static char arib_buf[256];
static char  caption_lang[8][3];
static uint8_t total_lang = 0;
static uint8_t cur_lang = 0;
static uint8_t cur_lang_index = 0;

static const unsigned int decoder_alnum_down_table[] = {
    0xc2a1,0xc2a2,0xc2a3,0xc2a4,0xc2a5,0xc2a6,0xc2a7,0xc2a8,
    0xc2a9,0xc2aa,0xc2ab,0xc2ac,0xc2ad,0xc2ae,0xc2af,0xc2b0,
    0xc2b1,0xc2b2,0xc2b3,0xc2b4,0xc2b5,0xc2b6,0xc2b7,0xc2b8,
    0xc2b9,0xc2ba,0xc2bb,0xc2bc,0xc2bd,0xc2be,0xc2bf,0xc380,
    0xc381,0xc382,0xc383,0xc384,0xc385,0xc386,0xc387,0xc388,
    0xc389,0xc38a,0xc38b,0xc38c,0xc38d,0xc38e,0xc38f,0xc390,
    0xc391,0xc392,0xc393,0xc394,0xc395,0xc396,0xc397,0xc398,
    0xc399,0xc39a,0xc39b,0xc39c,0xc39d,0xc39e,0xc39f,0xc3a0,
    0xc3a1,0xc3a2,0xc3a3,0xc3a4,0xc3a5,0xc3a6,0xc3a7,0xc3a8,
    0xc3a9,0xc3aa,0xc3ab,0xc3ac,0xc3ad,0xc3ae,0xc3af,0xc3b0,
    0xc3b1,0xc3b2,0xc3b3,0xc3b4,0xc3b5,0xc3b6,0xc3b7,0xc3b8,
    0xc3b9,0xc3ba,0xc3bb,0xc3bc,0xc3bd,0xc3be,0xc3bf,
};

int arib_init_caption(void)
{
	total_lang = 0;
	cur_lang_index = 0;
	return 0;
}

static int arib_decode_data_unit(void *arib_dec,uint8_t *pdata, uint32_t len)
{
	uint8_t unit_param, next_param;
	//uint8_t g3charset = 0;
	uint32_t unit_size, i;
	uint32_t current_table = 0;
	char *pbuf;

	if (len == 0 || *pdata != 0x1F)
		return 1;

	unit_param = pdata[1];
	unit_size = pdata[2];
	unit_size = (unit_size<<8)|pdata[3];
	unit_size = (unit_size<<8)|pdata[4];

	if (unit_size + 5 > len)
		return 2;

	//parse data
	if (unit_param != 0x20)
		return 3;

	//Statement body
	i = 5;
	next_param = 0;
	pbuf = arib_buf;
	while (i < len) {
		if (next_param == 0) {
			unit_param = pdata[i];
			if (unit_param < ARIB_SP) { //C0 area
				switch (unit_param) {
					case ARIB_PAPF:
						i++;
						break;
					case ARIB_APS:
						i += 2;
						break;
					case ARIB_CS:
						//printf("\033[31m _____cs____\033[0m\n");
						aribcc_decoder_draw_region(arib_dec,NULL,0);
						break;
				}
			}
			else if (unit_param >= ARIB_DEL && unit_param <= ARIB_10_0) { //C1 area
				switch (unit_param) {
					case ARIB_POL:
					case ARIB_SZX:
					case ARIB_FLC:
					case ARIB_WMM:
					case ARIB_RPC:
					case ARIB_HLC:
						i++;
						break;
					case ARIB_CDC:
						if(pdata[i+1]==0x20)
							i+=2;
						else if(pdata[i+1]==0x40||pdata[i+1]==0x4f)
							i++;
						break;
					case ARIB_COL:
						if (pdata[i+1] == 0x20) {
							i += 2;
						} else {
							if( pdata[i+1]>=0x48 && pdata[i+1]<=0x7f) {
								if((pbuf-arib_buf)>0)
									*pbuf++ = 0x90;///0x90 --0x20
								//aribcc_decoder_draw_region(arib_dec,(int8_t *)arib_buf,strlen(arib_buf));
								//memset(arib_buf, 0, sizeof(arib_buf));
							}
							i++;
						}
						break;
					case ARIB_TIME:
					#if 1
						if (pdata[i+1] == 0x20 || pdata[i+1] == 0x28)
							i += 2;
					#else
						if (pdata[i+1] == 0x20 )
							i += 2;
						else if(pdata[i+1] == 0x28)
							i += 10;
					#endif
						else if(pdata[i+1] == 0x29 ) {
							next_param = 0x20;
						}
						else
							i++;
						break;
					case ARIB_MACRO:
						next_param = 0x4E;
						break;
					case ARIB_CSI:
						if (pdata[i+1] == 0x5B || //PLD
							pdata[i+1] == 0x5C ) {//PLU
							i++;
						}
						else
						{
							//i += aribcc_csi_excpara(pdata);
							next_param = 0x20;
						}
						break;

				}
			}
			else if (unit_param != 0xFF)
			{
				if((unit_param > ARIB_US && unit_param < ARIB_DEL)
					|| (unit_param > ARIB_10_0 && unit_param < 0xFF))
				{
					if (unit_param > ARIB_DEL && unit_param < 256)
					{
						int j = unit_param-ARIB_10_0-1;
						int data = decoder_alnum_down_table[j];
						*pbuf++ = (data&0xFF00)>>8;
						*pbuf++ = data&0x00FF;
					}
					else
					{
						*pbuf++ = unit_param;//pdata[i];//
					}
				}
				else if (current_table != 0)
				{
					printf("_[%d]___*__*_**(%X) \n",__LINE__,pdata[i]);
				}
				else if ((unit_param >= ARIB_NUL && unit_param <ARIB_US)
					|| (unit_param > 0x7E && unit_param <0xA0)
					|| unit_param >= 0xFE)
				{
					printf("---*__*__*__*(%X) %d\n",pdata[i],__LINE__);
				}
				else
				{
					//*pbuf++ = unit_param;
				}
			}
		}
		else if (pdata[i] == next_param) {
			next_param = 0;
			if (unit_param == ARIB_CSI||unit_param == ARIB_TIME) {
				i++;
			}
		}

		i++;
	}

	*pbuf = '\0';

	aribcc_decoder_draw_region(arib_dec,(int8_t *)arib_buf,strlen(arib_buf));
	return 0;
}

static uint8_t arib_ISO639_to_lang(char *lang_code)
{
	if(memcmp(lang_code, "eng", 3) == 0){
		return ARIB_LAN_ID_ENG;
	}
	else if((memcmp(lang_code, "fra", 3) == 0) || (memcmp(lang_code, "fre", 3) == 0)) {
		return ARIB_LAN_ID_FRH;
	}
	else if((memcmp(lang_code, "deu", 3) == 0) || (memcmp(lang_code, "ger", 3) == 0)) {
		return ARIB_LAN_ID_DEU;
	}
	else if(memcmp(lang_code, "ita", 3) == 0) {
		return ARIB_LAN_ID_ITA;
	}
	else if(memcmp(lang_code, "por", 3) == 0) {
		return ARIB_LAN_ID_POR;
	}
	else if(memcmp(lang_code, "rus", 3) == 0) {
		return ARIB_LAN_ID_RUS;
	}
	else if((memcmp(lang_code, "esl", 3) == 0) || (memcmp(lang_code, "spa", 3) == 0)) {
		return ARIB_LAN_ID_SPA;
	}
	else if((memcmp(lang_code, "ell", 3) == 0) || (memcmp(lang_code, "gre", 3) == 0)) {
		return ARIB_LAN_ID_GRK;
	}
	else if(memcmp(lang_code, "ara", 3) == 0) {
		return ARIB_LAN_ID_ARB;
	}
	else if((memcmp(lang_code, "chi", 3) == 0) || (memcmp(lang_code, "zho", 3) == 0)) {
		return ARIB_LAN_ID_CHN;
	}
	else {
		return ARIB_LAN_ID_ENG;
	}
}

static int arib_decode_caption_management(void *arib_dec,uint8_t *pdata, uint16_t len)
{
	uint16_t index = 0;
	uint8_t i, TMD, DMF;
	uint32_t data_loop_len;

	TMD = pdata[index++]>>6;

	if (TMD == 2) {
		index += 5;
	}
	total_lang = pdata[index++];

	for (i=0; i<total_lang; i++) {
		DMF = pdata[index++] & 0x0F;
		if (DMF >= 0x0C && DMF <= 0x0E)
			index++;

		caption_lang[i][0] = pdata[index++];
		caption_lang[i][1] = pdata[index++];
		caption_lang[i][2] = pdata[index++];

		index++;
	}

	cur_lang = arib_ISO639_to_lang(caption_lang[cur_lang_index]);

	data_loop_len = pdata[index++];
	data_loop_len = (data_loop_len<<8)|pdata[index++];
	data_loop_len = (data_loop_len<<8)|pdata[index++];
//	printf("data_loop_len = %d\n",data_loop_len);
	if (data_loop_len)
		arib_decode_data_unit(arib_dec,pdata + index, data_loop_len);
	return 0;
}

static int arib_decode_caption_statement(void *arib_dec,uint8_t *pdata, uint16_t len)
{
	uint8_t TMD;
	uint32_t data_loop_len;
	uint16_t index = 0;

	TMD = pdata[index++]>>6;

	if (TMD == 1 || TMD == 2)
		index += 5;

	data_loop_len = pdata[index++];
	data_loop_len = (data_loop_len<<8)|pdata[index++];
	data_loop_len = (data_loop_len<<8)|pdata[index++];

	if (data_loop_len)
		arib_decode_data_unit(arib_dec,pdata + index, data_loop_len);

	return 0;
}

static int arib_decode_data_group(PESContext *pes)
{
	void *arib_dec = pes->priv;
	uint8_t *pdata = pes->data_buffer;
	uint8_t data_identifier,private_stream_id;
	uint8_t data_grup_id;
	uint16_t data_group_size;

	if(pes->total_size != MAX_PES_PAYLOAD &&
		pes->pes_header_size + pes->data_index != pes->total_size + PES_START_SIZE) {
		printf("pes->pes_header_size %02d + pes->data_index %04d %s pes->total_size %04d + PES_START_SIZE %02d\n",
				pes->pes_header_size,pes->data_index,
				pes->pes_header_size + pes->data_index == pes->total_size + PES_START_SIZE ? "==" : "!=",
				pes->total_size,PES_START_SIZE);
		if((pes->total_size + PES_START_SIZE) > (pes->pes_header_size + pes->data_index)) {
			uint32_t left_len = (pes->total_size + PES_START_SIZE) - (pes->pes_header_size + pes->data_index);
			memset(&pes->data_buffer[pes->data_index],0xff,left_len);
			pes->data_index += left_len;
		}
	}

	dvbprivate_wait_pcr(pes);

	data_identifier = *pdata++;
	private_stream_id = *pdata++;
	if(data_identifier != 0x80 || private_stream_id != 0xff) {
		printf("data_identifier %02x or private_stream_id %02x error\n",
			data_identifier,private_stream_id);
		return -1;
	}

	if(*pdata++ != 0xf0) {
		printf("sync byte error\n");
		return -1;
	}

	data_grup_id = pdata[0]>>2;
	data_group_size = (pdata[3]<<8) | pdata[4];
//	printf("data_grup_id = %02x data_group_size = %d\n",data_grup_id,data_group_size);
	if (data_grup_id == 0x00 || data_grup_id == 0x20) {
		arib_decode_caption_management(arib_dec,pdata + 5, data_group_size);
	}
	else if (data_grup_id == (0x01+cur_lang_index)|| data_grup_id == (0x21+cur_lang_index)) {
		arib_decode_caption_statement(arib_dec,pdata + 5, data_group_size);
	}

	return 0;
}

int arib_get_cur_lang(void)
{
	return cur_lang;
}

char *arib_get_cur_caption_lang(void)
{
	return caption_lang[cur_lang_index];
}

char *arib_set_langid(int langid)
{
	int i,iso639_lang;

	if(langid < ARIB_LAN_ID_ENG || langid >= ARIB_LAN_ID_UNKNOW)
		return NULL;

	if(langid == cur_lang)
		return caption_lang[cur_lang_index];

	for(i = 0;i < total_lang;i++) {
		iso639_lang = arib_ISO639_to_lang(caption_lang[i]);
		if(iso639_lang == langid) {
			cur_lang = langid;
			cur_lang_index = i;
			break;
		}
	}

	if(i >= total_lang) {
		cur_lang_index = 0;
		//return NULL;
	}

	return caption_lang[cur_lang_index];
}


static void reset_pes_packet_state(PESContext *pes)
{
	pes->pts        = -1;
	pes->dts        = -1;
	pes->data_index = 0;
	pes->flags      = 0;
}

/**
 *  * Parse MPEG-PES five-byte timestamp
 *   */
static inline int64_t parse_pes_pts(const uint8_t *buf) {
	return (int64_t)(*buf & 0x0e) << 29 |
		(AV_RB16(buf+1) >> 1) << 15 |
		AV_RB16(buf+3) >> 1;
}

static int read_sl_header(PESContext *pes, SLConfigDescr *sl,
		const uint8_t *buf, int buf_size)
{
	printf("unsupport read sl header....\n");
	return 0;
}

/* return non zero if a packet could be constructed */
int process_dvbs_pes_cc(PESContext *pes,uint8_t *buf, int buf_size, int is_start,int64_t pos)
{
	const uint8_t *p;
	int len, code;

	if (is_start) {
		if (pes->state == MPEGTS_PAYLOAD && pes->data_index > 0) {
			arib_decode_data_group(pes);
			reset_pes_packet_state(pes);
		} else {
			reset_pes_packet_state(pes);
		}
		pes->state         = MPEGTS_HEADER;
		pes->ts_packet_pos = pos;
	}
	p = buf;
	while (buf_size > 0) {
		switch (pes->state) {
			case MPEGTS_HEADER:
				len = PES_START_SIZE - pes->data_index;
				if (len > buf_size)
					len = buf_size;
				memcpy(pes->header + pes->data_index, p, len);
				pes->data_index += len;
				p += len;
				buf_size -= len;
				if (pes->data_index == PES_START_SIZE) {
					/* we got all the PES or section header. We can now
					 * decide */
					if (pes->header[0] == 0x00 && pes->header[1] == 0x00 &&
							pes->header[2] == 0x01) {
						/* it must be an mpeg2 PES stream */
						code = pes->header[3] | 0x100;

						pes->stream_id = pes->header[3];

						if (code == 0x1be) /* padding_stream */
							goto skip;

						pes->total_size = AV_RB16(pes->header + 4);
						/* NOTE: a zero total size means the PES size is
						 * unbounded */
						if (!pes->total_size)
							pes->total_size = MAX_PES_PAYLOAD;

						/* allocate pes buffer */
						if(!pes->data_buffer)
							pes->data_buffer = MALLOC(pes->total_size +
									BUFFER_PADDING_SIZE);
						if (!pes->data_buffer)
							return -1;

						if (code != 0x1bc && code != 0x1bf && /* program_stream_map, private_stream_2 */
								code != 0x1f0 && code != 0x1f1 && /* ECM, EMM */
								code != 0x1ff && code != 0x1f2 && /* program_stream_directory, DSMCC_stream */
								code != 0x1f8) {                  /* ITU-T Rec. H.222.1 type E stream */
							pes->state = MPEGTS_PESHEADER;
						} else {
							pes->pes_header_size = 6;
							pes->state      = MPEGTS_PAYLOAD;
							pes->data_index = 0;
						}
					} else {
						/* otherwise, it should be a table */
						/* skip packet */
skip:
						pes->state = MPEGTS_SKIP;
						continue;
					}
				}
				break;
				/**********************************************/
				/* PES packing parsing */
			case MPEGTS_PESHEADER:
				len = PES_HEADER_SIZE - pes->data_index;
				if (len < 0)
					return -1;
				if (len > buf_size)
					len = buf_size;
				memcpy(pes->header + pes->data_index, p, len);
				pes->data_index += len;
				p += len;
				buf_size -= len;
				if (pes->data_index == PES_HEADER_SIZE) {
					pes->pes_header_size = pes->header[8] + 9;
					pes->state           = MPEGTS_PESHEADER_FILL;
				}
				break;
			case MPEGTS_PESHEADER_FILL:
				len = pes->pes_header_size - pes->data_index;
				if (len < 0)
					return -1;
				if (len > buf_size)
					len = buf_size;
				memcpy(pes->header + pes->data_index, p, len);
				pes->data_index += len;
				p += len;
				buf_size -= len;
				if (pes->data_index == pes->pes_header_size) {
					const uint8_t *r;
					unsigned int flags, pes_ext, skip;

					flags = pes->header[7];
					r = pes->header + 9;
					pes->pts = -1;
					pes->dts = -1;
					if ((flags & 0xc0) == 0x80) {
						pes->dts = pes->pts = parse_pes_pts(r);
						r += 5;
					} else if ((flags & 0xc0) == 0xc0) {
						pes->pts = parse_pes_pts(r);
						r += 5;
						pes->dts = parse_pes_pts(r);
						r += 5;
					}
					pes->extended_stream_id = -1;
					if (flags & 0x01) { /* PES extension */
						pes_ext = *r++;
						/* Skip PES private data, program packet sequence counter and P-STD buffer */
						skip  = (pes_ext >> 4) & 0xb;
						skip += skip & 0x9;
						r    += skip;
						if ((pes_ext & 0x41) == 0x01 &&
								(r + 2) <= (pes->header + pes->pes_header_size)) {
							/* PES extension 2 */
							if ((r[0] & 0x7f) > 0 && (r[1] & 0x80) == 0)
								pes->extended_stream_id = r[1];
						}
					}

					/* we got the full header. We parse it and get the payload */
					pes->state = MPEGTS_PAYLOAD;
					pes->data_index = 0;
					if (pes->stream_type == 0x12 && buf_size > 0) {
						int sl_header_bytes = read_sl_header(pes, &pes->sl, p,
								buf_size);
						pes->pes_header_size += sl_header_bytes;
						p += sl_header_bytes;
						buf_size -= sl_header_bytes;
					}
					if (pes->stream_type == 0x15 && buf_size >= 5) {
						/* skip metadata access unit header */
						pes->pes_header_size += 5;
						p += 5;
						buf_size -= 5;
					}
				}
				break;
			case MPEGTS_PAYLOAD:
				if (pes->data_buffer) {
					if (pes->data_index > 0 &&
							pes->data_index + buf_size > pes->total_size) {
						arib_decode_data_group(pes);
						reset_pes_packet_state(pes);
						pes->total_size = MAX_PES_PAYLOAD;
						if(!pes->data_buffer)
							pes->data_buffer = MALLOC(pes->total_size +
									BUFFER_PADDING_SIZE);
						if (!pes->data_buffer)
							return -1;
					} else if (pes->data_index == 0 &&
							buf_size > pes->total_size) {
						// pes packet size is < ts size packet and pes data is padded with 0xff
						// not sure if this is legal in ts but see issue #2392
						buf_size = pes->total_size;
					}
					memcpy(pes->data_buffer + pes->data_index, p, buf_size);
					pes->data_index += buf_size;
					/* emit complete packets with known packet size
					 * decreases demuxer delay for infrequent packets like subtitles from
					 * a couple of seconds to milliseconds for properly muxed files.
					 * total_size is the number of bytes following pes_packet_length
					 * in the pes header, i.e. not counting the first PES_START_SIZE bytes */
					if (pes->total_size < MAX_PES_PAYLOAD &&
							pes->pes_header_size + pes->data_index == pes->total_size + PES_START_SIZE) {
						arib_decode_data_group(pes);
						reset_pes_packet_state(pes);
					}
				}
				buf_size = 0;
				break;
			case MPEGTS_SKIP:
				buf_size = 0;
				break;
		}
	}

	return 0;
}
#endif
