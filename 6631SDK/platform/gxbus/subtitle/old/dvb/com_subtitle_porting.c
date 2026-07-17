/**
 *
 * @file        com_subtitle_porting.c
 * @brief
 * @version     1.1.0
 * @date        08/19/2010 02:09:02 PM
 * @author      Li Xiaobin (lixb), lixb@nationalchip.com
 *
 */
#include "gx_mem.h"
#include "com_subtitle_porting.h"
#include "com_sub_def.h"
#include "com_subt_sub.h"
#include "sub_debug.h"

#ifndef E_FAILURE
#define E_FAILURE                   -1
#endif
#ifndef E_OK
#define E_OK                        0
#endif
#ifndef E_INVALID_HANDLE
#define E_INVALID_HANDLE            0
#endif

extern u8* g_pSubtitlePesBufferPtr;//存储流中的原始数据
extern u8* g_pSubtitleCompositionBufferPtr;//存储构图页的buffer指针
extern u8* g_pSubtitlePageBufferPtr; //存储解析出的page页指针
extern u8* g_pSubtitleObjectBufferPtr;//存储被OBJECT填充过的REGION的所有象素点
extern u8* g_pStubtitlePixelBufferPtr;//PIC层数据数组的开始位置
extern u8 g_chStuffingByteLen;
extern u8 g_chSubtitleDecState;
extern u8 g_wRegionUseLen[256];//REGION实际占用的长度
extern u8 g_wClutUseLen[256];//CLUT实际占用的长度

uint8_t* g_SppBuffer = NULL;

struct com_subtitle com_sub;

int timer_clear_screen(void *userdata)
{
	DBG_SUB("[subtitle] timer_clear_screen\n");
	{
		GxTime tm;
		GxCore_GetTickTime(&tm);
	}
	GxCore_MutexLock(com_sub.lock);
	com_sub.render->ops->clear(com_sub.render->handle);
	GxCore_MutexUnlock(com_sub.lock);
	com_sub.timer_ops->ops->remove(com_sub.timer);
	return 0;
}

static handle_t com_subtitle_open(GxSubtitle_Render* render, GxSubtitle_Timer* timer)
{
	com_sub.render = render;
	com_sub.timer_ops = timer;

	com_sub.PixelBufferPtr = GxCore_Malloc(PIXEL_BUF_MAX_LEN);//临时存放整个需要显示的REGION的象素点
	if (com_sub.PixelBufferPtr == NULL) {
		return E_FAILURE;
	}
	g_pStubtitlePixelBufferPtr = com_sub.PixelBufferPtr;
	com_sub.ObjectBufferPtr = GxCore_Malloc(OBJECT_BUF_MAX_LEN);//流中OBJECT段等数据的缓存
	if (com_sub.ObjectBufferPtr == NULL) {
		return E_FAILURE;
	}
	g_pSubtitleObjectBufferPtr = com_sub.ObjectBufferPtr;
	com_sub.CompositionBufferPtr = GxCore_Malloc(COMPOSITION_BUF_MAX_LEN);//流中PAGE REGION CLUT段的缓存
	if (com_sub.CompositionBufferPtr == NULL) {
		return E_FAILURE;
	}
	g_pSubtitleCompositionBufferPtr = com_sub.CompositionBufferPtr;
	com_sub.PageBufferPtr = GxCore_Malloc(PAGE_BUF_MAX_LEN);
	if (com_sub.PageBufferPtr == NULL) {
		return E_FAILURE;
	}
	//com_sub.timer = com_sub.timer_ops->ops->create(timer_clear_screen, 3500, NULL,  1);

	com_sub.sync_milliseconds = 11250;
	GxCore_MutexCreate(&com_sub.lock);
	//GxCore_SemCreate(&com_sub.clear_sem, 0);
	return (handle_t)&com_sub;
}

static int32_t com_subtitle_close(handle_t handle)
{
	struct com_subtitle* sub = (struct com_subtitle*)handle;

	GxCore_MutexDelete(com_sub.lock);
	av_free(sub->PixelBufferPtr);
	av_free(sub->ObjectBufferPtr);
	av_free(sub->CompositionBufferPtr);
	av_free(sub->PageBufferPtr);
	com_sub.timer_ops->ops->remove(sub->timer);
	memset(sub, 0, sizeof(struct com_subtitle));
	return E_OK;
}

static int32_t com_subtitle_start(handle_t handle)
{
	struct com_subtitle* sub = (struct com_subtitle*)handle;
	extern u8  g_chSubtEnable;

	g_chStuffingByteLen=0;
	/*解析出的数据放在SDRAM中PIC层的数组中*/

	g_pSubtitlePageBufferPtr = sub->PageBufferPtr;

	g_chSubtEnable = 1;
	subt_initial();

	return E_OK;
}
static int32_t com_subtitle_stop(handle_t handle)
{
	struct com_subtitle*        sub = (struct com_subtitle*)handle;
	g_chSubtEnable = 0;
	com_sub.timer_ops->ops->remove(sub->timer);
	return E_OK;
}
static int32_t com_subtitle_synchronize(handle_t handle, int32_t milliseconds)
{
	//struct com_subtitle*        sub = (struct com_subtitle*)handle;

	//sub->sync_milliseconds += (milliseconds*45);

	return E_OK;
}
static int32_t com_subtitle_set_stream(handle_t handle, uint16_t comp_page_id, uint16_t anci_page_id)
{
	extern u16 g_wSubtitleCompositionPageId;
	extern u16 g_wSubtitleAncillaryPageId;

	g_wSubtitleCompositionPageId = comp_page_id;
	g_wSubtitleAncillaryPageId = anci_page_id;

	return E_OK;
}

static int dvbsub_verify_packet(const uint8_t* packet)
{
	/* Packet header == 0x000001 */
	if (packet[0] != 0x00 || packet[1] != 0x00 || packet[2] != 0x01) {
		DBG_SUB("Invalid header\n");
		return 0;
	}

	/* stream_id == private_stream_1 */
	if (packet[3] != 0xbd) {
		DBG_SUB("Not a private_stream_1 stream (%x)\n", packet[3]);
		return 0;
	}

	return 1;
}

static int32_t com_subtitle_decode(handle_t handle, uint8_t* packet, int total_size)
{
	int32_t                     packet_length = 0;
	extern u8*                  g_pSubtitlePesBufferPtr;//存储流中的原始数据
	extern u8                   g_chStuffingByteLen;

	if (packet == NULL || total_size <= 0)
		return E_FAILURE;

	while (total_size > 0) {
		if (dvbsub_verify_packet(packet) == 0)
			return E_FAILURE;

		packet_length = ((uint16_t)packet[4] << 8) + packet[5] + 6;

		g_chStuffingByteLen = packet[8];
		g_pSubtitlePesBufferPtr = packet;
		subt_send_to_buffer(packet_length-6);
		packet      += packet_length;
		total_size  -= packet_length;
	}
	return E_OK;
}

GxSubtitle_DecoderOps   com_subtitle_ops = {
	com_subtitle_open,
	com_subtitle_close,
	com_subtitle_start,
	com_subtitle_stop,
	com_subtitle_synchronize,
	com_subtitle_set_stream,
	com_subtitle_decode,
	NULL
};
