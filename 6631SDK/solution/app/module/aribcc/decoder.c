#include "gxcore.h"
#include "assert.h"
#include "gxaribcc.h"
#include "aribcc.h"
#include "module/app_log.h"

#if ARIBCC_SUPPORT
struct gxaribcc_dec {
	GxAribCC_demux*   demux;
	GxAribCC_Render*  render;
	GxAribCC_Timer* timer;
	PESContext *pes;
	int32_t sync_milliseconds;
	int32_t running;
};

#define GET_STC(sub)    ((sub)->demux->ops->get_stc((sub)->demux->handle))
#define GET_PES_PACKETS(sub, pptr, size)      ((sub)->demux->ops->get_pes((sub)->demux->handle, (pptr), (size)))

uint8_t* g_AribccPesBufferPtr;
uint8_t  g_AribccStuffingByteLen;
uint16_t g_AribccCompositionPageId;
uint16_t g_AribccAncillaryPageId;

static handle_t aribcc_decoder_open(GxAribCC_demux* demux, GxAribCC_Render* render, GxAribCC_Timer* timer)
{
	struct gxaribcc_dec*           dec;

	dec = CALLOC(1, sizeof(struct gxaribcc_dec));
	if (dec == NULL) {
		return E_INVALID_HANDLE;
	}

	dec->demux = demux;
	dec->render = render;
	dec->timer = timer;

	dec->pes = MALLOC(sizeof(PESContext));
	if (!dec->pes)
		return 0;

	memset(dec->pes,0,sizeof(PESContext));

	dec->pes->pcr_pid = 0x1fff;
	dec->pes->data_buffer  = MALLOC(MAX_PES_PAYLOAD + BUFFER_PADDING_SIZE);
	if(!dec->pes->data_buffer) {
		FREE(dec);
		return 0;
	}
	dec->pes->state   = MPEGTS_SKIP;
	dec->pes->pts     = -1;
	dec->pes->dts     = -1;
	dec->pes->stream_id = 0xff;
	dec->pes->priv = dec;

	dec->sync_milliseconds = 11250;
	arib_init_caption();

	dec->running = 1;

	return (handle_t)dec;
}

static int32_t aribcc_decoder_close(handle_t handle)
{
	struct gxaribcc_dec*       dec = (struct gxaribcc_dec*)handle;

	ASSERT(dec != NULL);

	dec->running = 0;

	FREE(dec->pes->data_buffer);
	FREE(dec->pes);
	FREE(dec);

	return E_OK;
}

static int32_t aribcc_decoder_start(handle_t handle)
{
	struct gxaribcc_dec*       dec = (struct gxaribcc_dec*)handle;
    if(dec == NULL)
        app_log_error("dec is NULL!!!!!!!!!!!!!!!!!!!\n");

	ASSERT(dec != NULL);

	g_AribccStuffingByteLen=0;
	g_AribccPesBufferPtr = NULL;

	return E_OK;
}

static int32_t aribcc_decoder_stop(handle_t handle)
{
	g_AribccCompositionPageId = 0;
	g_AribccAncillaryPageId = 0;

	return E_OK;
}

static int32_t aribcc_decoder_set_stream(handle_t handle, uint16_t comp_page_id, uint16_t anci_page_id)
{
	g_AribccCompositionPageId = comp_page_id;
	g_AribccAncillaryPageId = anci_page_id;

	return E_OK;
}

static int32_t aribcc_decoder_switch_lang(handle_t handle, GxAribCC_LangID langid)
{
	struct gxaribcc_dec*       dec = (struct gxaribcc_dec*)handle;
    if(dec == NULL)
        app_log_error("dec is NULL!!!!!!!!!!!!!!!!!!!\n");

	ASSERT(dec != NULL);

	if(arib_set_langid(langid) == NULL)
		return E_FAILURE;

	return E_OK;
}

void aribcc_decoder_draw_region(void *arib_dec,int8_t *data, int32_t len)
{
	struct gxaribcc_dec*       dec = (struct gxaribcc_dec*)arib_dec;
	int langid;

	ASSERT(dec != NULL);

	langid = arib_get_cur_lang();
	if(langid < ARIB_LAN_ID_ENG || langid >= ARIB_LAN_ID_UNKNOW)
		return;

	if(dec->render && dec->render->ops)
		dec->render->ops->fill_region(dec->render->handle,data,len);
}

static int dvbprivate_verify_packet(const uint8_t* packet)
{
	/* Packet header == 0x000001 */
	if (packet[0] != 0x00 || packet[1] != 0x00 || packet[2] != 0x01) {
		printf("Invalid header\n");
		return 0;
	}

	/* stream_id == private_stream_1 */
	if (packet[3] != 0xBD) {
		printf("Not a private_stream_1 stream (%x)\n", packet[3]);
		return 0;
	}

	return 1;
}

uint8_t dvbprivate_wait_pcr(PESContext *pes)
{
#define REBACK_STC_VALUE	(20*45000)//20s
#define PLAY_OFFSET		(100*45)
	struct gxaribcc_dec*       dec = (struct gxaribcc_dec*)pes->priv;
	uint32_t timeout=0;
	int64_t pts=0, stc=0, offset = 0;

	pts  = pes->pts;
	pts &= 0xffffffff;

	while(1) {
		stc = (GET_STC(dec)) & 0xffffffff;
		offset  = abs(pts-stc);
		/*if stop running should return*/
		if(dec->running == 0) {
			break;
		}
		/* interval time about pts and stc is right !*/
		if(offset <= dec->sync_milliseconds + PLAY_OFFSET) {
			break;
		}
		/*if the PES is later than STC, then show it immediately*/
		else if(pts < stc) {
			break;
		}
		/* interval time is too large ? */
		else if(offset >= REBACK_STC_VALUE) {
			break;
		} else {
			GxCore_ThreadDelay(50);
			timeout ++;
			if(timeout++ >= 160) {
				timeout = 0;
				break;
			}
		}
	}

	return 1;
}

static int32_t aribcc_decoder_decode(handle_t handle)
{
#define SUB_PES_BUF_LEN                         (1024*64)
	struct gxaribcc_dec*       dec = (struct gxaribcc_dec*)handle;
	uint8_t*                    packet;
	int32_t                     packet_length = 0;
	int                         total_size;

	ASSERT(dec != NULL);

	total_size = GET_PES_PACKETS(dec, &packet, SUB_PES_BUF_LEN);

	if(total_size <=0 )
		GxCore_ThreadDelay(10);

	while(total_size > 0) {
		if (dvbprivate_verify_packet(packet) == 0) {
			packet_length = ((uint16_t)packet[4] << 8) + packet[5] + 6;
			total_size  -= packet_length;
			packet      += packet_length;
			printf("[ARIBCC] Error! Get PES packet length=%d, total_size=%d\n", packet_length, total_size);
			continue;
		}
		packet_length = ((uint16_t)packet[4] << 8) + packet[5] + 6;

		g_AribccStuffingByteLen = packet[8];
		g_AribccPesBufferPtr = packet;
		process_dvbs_pes_cc(dec->pes,packet,packet_length,1,0);
		packet      += packet_length;
		total_size  -= packet_length;
	}

	return E_OK;
}

GxAribCC_DecoderOps   aribcc_decoder = {
	.open           = aribcc_decoder_open,
	.close          = aribcc_decoder_close,
	.start          = aribcc_decoder_start,
	.stop           = aribcc_decoder_stop,
	.set_stream     = aribcc_decoder_set_stream,
	.switch_lang    = aribcc_decoder_switch_lang,
	.decode         = aribcc_decoder_decode,
};
#endif

