/**
 *
 * @file        dvb_sub_decode.c
 * @brief
 * @version     1.1.0
 * @date        04/30/2010 09:34:50 AM
 * @author      Li Xiaobin (lixb), lixb@nationalchip.com
 *
 */
#include "gxcore.h"
#include "gx_mem.h"
#include "subtitle/gxsubtitle.h"
#include "sub_dvb.h"
#include "sub_dvb_segment.h"
#include "dvbsub_decoder.h"
#include "dvbsub_renderer.h"
#include "dvbsub_helpers.h"
#include "display_set.h"

#define SUB_PES_BUF_LEN                         (1024*16)

#define GET_PES_PACKETS(decoder, pptr, size)      ((decoder)->demux->ops->get_pes((decoder)->demux->handle, (pptr), (size)))

static void draw_hline(void *priv, int x, int y, int length, int thickness,
		uint8_t pseudo_color)
{
	drawing_context_t*      dc = (drawing_context_t*)priv;
	GxSubtitle_RenderLine   line;

	ASSERT(priv != NULL);

	dc = (drawing_context_t*)priv;

	line.start_point.x = x + dc->x;
	line.start_point.y = y + dc->y;
	line.len = length;
	line.thickness = thickness;
	dc->render->ops->draw_line(dc->render->handle,
			&line, pseudo_color);

}

static void fill_region(void *priv, int x, int y, int width, int height,
		uint8_t pseudo_color)
{
	drawing_context_t*      dc = (drawing_context_t*)priv;
	GxSubtitle_RenderRect   rect;

	ASSERT(priv != NULL);

	dc = (drawing_context_t*)priv;
	rect.x = x;
	rect.y = y;
	rect.width = width;
	rect.height = height;

	dc->render->ops->fill_region(dc->render->handle, &rect, pseudo_color);
}

static void clear_screen(void *priv)
{
	drawing_context_t*      dc = (drawing_context_t*)priv;

	ASSERT(priv != NULL);

	dc = (drawing_context_t*)priv;

	dc->render->ops->clear(dc->render->handle);
}

static void screen_hide(void *priv)
{
	drawing_context_t* dc = (drawing_context_t*)priv;

	ASSERT(priv != NULL);

	dc = (drawing_context_t*)priv;
	dc->render->ops->hide(dc->render->handle);
}

static void screen_show(void* priv)
{
	drawing_context_t* dc = (drawing_context_t*)priv;

	ASSERT(priv != NULL);

	dc = (drawing_context_t*)priv;
	dc->render->ops->show(dc->render->handle);
}

static void set_color(void *priv, uint8_t pseudo_color, uint8_t Y,
		uint8_t Cb, uint8_t Cr, uint8_t T)
{
#define FULL_TRANSPRANCY    (0x00)
#define HALF_TRANSPRANCY    (0x40)
#define HALF_NO_TRANSPRANCY (0x80)
#define NO_TRANSPRANCY      (0xC0)

	drawing_context_t*      dc = (drawing_context_t*)priv;
	GxSubtitle_RenderColor  color;

	ASSERT(priv != NULL);

	dc = (drawing_context_t*)priv;

	if (Y == 0) {
		color.color.ycc.y = 16;
		color.color.ycc.cb = 128;
		color.color.ycc.cr = 128;
		color.color.ycc.t = 0;
	} else {
		color.color.ycc.y = Y;
		color.color.ycc.cb = Cb;
		color.color.ycc.cr = Cr;
		if (T == 255) {
			color.color.ycc.t = FULL_TRANSPRANCY;
		} else if (T < 64) {
			color.color.ycc.t = NO_TRANSPRANCY;
		} else if (T < 128) {
			color.color.ycc.t = HALF_NO_TRANSPRANCY;
		} else if (T < 192) {
			color.color.ycc.t = HALF_TRANSPRANCY;
		} else {
			color.color.ycc.t = FULL_TRANSPRANCY;
		}
	}

	dc->render->ops->set_color(dc->render->handle, pseudo_color, &color);
}


static void render(void *priv, const dvbsub_display_set_t *display_set)
{
	int                                     i;
	dvbsub_page_composition_segment_t*      pcs;
	dvbsub_decoder_t*                       decoder;
	dvbsub_rendering_context_t*             cntx;
	dvbsub_region_composition_segment_t*    rcs;
	int                                     color;

	ASSERT(priv != NULL);
	ASSERT(display_set != NULL);

	decoder = (dvbsub_decoder_t*)priv;
	cntx = decoder->cntx;

	ASSERT(cntx != NULL);

	pcs = display_set->page_composition_segment;

#if 1
	DBG_SUB("display_set=%p,page state =%d\n", display_set, pcs->page_state);
	DBG_SUB("number_of_region_compositions=%d\n", pcs->number_of_region_compositions);
#endif

	screen_hide(decoder->dc);
	//if (pcs->page_state != 0) {
	clear_screen(decoder->dc);
	//}
	for (i = 0; i < pcs->number_of_region_compositions; ++i) {
		rcs = dvbsub_find_region_composition_segment(display_set,
				pcs->regions[i].region_id);
		if (rcs == 0)
			continue;
		decoder->dc->x = pcs->regions[i].region_horizontal_address;
		decoder->dc->y = pcs->regions[i].region_vertical_address;
		decoder->dc->width = rcs->region_width;
		decoder->dc->height = rcs->region_height;
#if 0
		DBG_SUB("regino_id=%d,x=%d,y=%d,w=%d,h=%d\n",
				rcs->region_id, decoder->dc->x, decoder->dc->y,
				decoder->dc->width, decoder->dc->height);
#endif
		cntx->region_composition = rcs;
		cntx->display_set = display_set;
		if (cntx->set_color_callback && pcs->page_state != 0) {
			dvbsub_rendering_color_map(cntx);
		}
		/* Region background color may be given in region composition segment
		   or it may be left undefined */
		color = dvbsub_get_region_background_color(rcs);
		if (color < 0 && color > 255) {
			color = 0;
		}
		fill_region(decoder->dc,
				decoder->dc->x,
				decoder->dc->y,
				decoder->dc->width, decoder->dc->height, color);
		/* Do actual rendering */
		dvbsub_render_region_composition(cntx);
	}
	screen_show(decoder->dc);
}


static handle_t dvbsub_decoder_open(GxSubtitle_Render* render,
		GxSubtitle_Timer*  timer)
{
	dvbsub_decoder_t*   decoder;

	ASSERT(render != NULL);

	decoder = (dvbsub_decoder_t*)CALLOC(1, sizeof(dvbsub_decoder_t));

	/* Create subtitle decoder. */
	decoder = dvbsub_decoder_create();
	if (decoder == NULL) {
		return E_INVALID_HANDLE;
	}
	decoder->sync_milliseconds = -600;
	decoder->dc = (drawing_context_t*)CALLOC(1, sizeof(drawing_context_t));
	if (decoder->dc == NULL) {
		dvbsub_decoder_destroy(decoder);
		return E_INVALID_HANDLE;
	}
	decoder->dc->render = render;
	GX_INIT_LIST_HEAD(&decoder->ready_list);
	GxCore_SemCreate(&decoder->ready_sem, 0);

	/* Create rendering context */
	decoder->cntx = dvbsub_rendering_context_create(draw_hline, NULL,
			set_color, decoder->dc);
	if (decoder->cntx == 0) {
		dvbsub_decoder_destroy(decoder);
		av_free(decoder->dc);
		return E_INVALID_HANDLE;
	}

	return (handle_t)decoder;
}

static int32_t dvbsub_decoder_set_stream(handle_t handle,
		uint16_t comp_page_id,
		uint16_t anci_page_id)
{
	int32_t                     i;
	int32_t                     count;
	dvbsub_decoder_t*           decoder = (dvbsub_decoder_t*)handle;

	ASSERT(decoder != NULL);

	GxCore_SemGetValue(decoder->ready_sem, &count);
	for (i = 0; i < count; i++) {
		GxCore_SemWait(decoder->ready_sem);
	}

	if (decoder->comp_page_decoders != NULL) {
		dvbsub_page_decoder_destroy(decoder->comp_page_decoders);
		decoder->comp_page_decoders = NULL;
	} 

	if (decoder->anci_page_decoders != NULL) {
		dvbsub_page_decoder_destroy(decoder->anci_page_decoders);
		decoder->anci_page_decoders = NULL;
	}


	if (anci_page_id != -1) {
		/* Create decoder for ancillary pages */
		decoder->anci_page_decoders = dvbsub_page_decoder_create(decoder,
				anci_page_id,
				decoder);
		if (decoder->anci_page_decoders == NULL) {
			dvbsub_decoder_destroy(decoder);
			return E_FAILURE;
		}
	} 
	if (comp_page_id != -1) {
		/* Create a page decoder */

		decoder->comp_page_decoders = dvbsub_page_decoder_create(decoder, 
				comp_page_id,
				decoder);

		if (decoder->comp_page_decoders == NULL) {
			if (anci_page_id != -1) {
				dvbsub_page_decoder_destroy(decoder->anci_page_decoders);
			}
			dvbsub_decoder_destroy(decoder);
			return E_FAILURE;
		}

	}

	return E_OK;
}

static int32_t dvbsub_decoder_synchronize(handle_t handle, int32_t milliseconds)
{
	dvbsub_decoder_t* decoder = (dvbsub_decoder_t*)handle;

	ASSERT(decoder != NULL);

	decoder->sync_milliseconds += milliseconds;

	return E_OK;
}

static int32_t dvbsub_decoder_close(handle_t handle)
{
	dvbsub_decoder_t* decoder = (dvbsub_decoder_t*)handle;

	ASSERT(decoder != NULL);

	av_free(decoder->dc);
	GxCore_SemDelete(decoder->ready_sem);
	dvbsub_rendering_context_destroy(decoder->cntx);
	dvbsub_decoder_destroy(decoder);

	return E_OK;
}

static int32_t dvbsub_decode(handle_t handle, uint8_t* packet, int total_size)
{
	dvbsub_decoder_t*           decoder = (dvbsub_decoder_t*)handle;
	int32_t                     packet_length = 0;

	ASSERT(decoder != NULL);

	if (packet == NULL || total_size <= 0)
		return E_FAILURE;

	while(total_size > 0) {
		packet_length = dvbsub_push_pes_packet(decoder, packet + packet_length);
		if (packet_length == -1) {
			return E_OK;
		}
		total_size    -= packet_length;
	}

	return E_OK;
}

static int32_t dvbsub_start(handle_t handle)
{
	dvbsub_decoder_t* decoder = (dvbsub_decoder_t*)handle;

	decoder->time_out = 0;
	decoder->stop_flag = FALSE;
	return E_OK;
}

static int32_t dvbsub_stop(handle_t handle)
{
	dvbsub_decoder_t* decoder = (dvbsub_decoder_t*)handle; 

	decoder->time_out = 0;
	decoder->stop_flag = TRUE;
	GxCore_SemPost(decoder->ready_sem);
	return E_OK;
}


static int32_t dvbsub_render(handle_t handle)
{
#define SUB_DVB_WAIT_DISPLAY_SET    50000
#define SUB_DVB_CLEAR_DISPLAY_SET   1000
	dvbsub_decoder_t*           decoder = (dvbsub_decoder_t*)handle;
	dvbsub_display_set_t*       ds = NULL;
	GxTime                      tm;
	int                         ret;
	static int32_t              timeout = SUB_DVB_WAIT_DISPLAY_SET;

	ret = GxCore_SemTimedWait(decoder->ready_sem, timeout);
	if (ret == GXCORE_SEM_TIMEOUT) {
		GxCore_GetTickTime(&tm);
		if (tm.seconds >= decoder->time_out) {
			decoder->time_out = 0;
			timeout = SUB_DVB_WAIT_DISPLAY_SET;
			clear_screen(decoder->dc);
			dvbsub_cleanup_before_PTS(decoder, 0);
		}
	} else if (ret == GXCORE_SUCCESS) {
		while(!decoder->stop_flag ) {
			ds = dvbsub_find_visible_display_set(decoder->comp_page_decoders, 0);
			if (ds != NULL) {
				GxCore_GetTickTime(&tm);
				decoder->time_out = tm.seconds + 10;
				render((void*)decoder, ds);
				timeout = SUB_DVB_CLEAR_DISPLAY_SET;
				dvbsub_cleanup_before_PTS(decoder, 0);
				return E_OK;
			}
			GxCore_ThreadDelay(200);
		}
	}

	return E_OK;
}

GxSubtitle_DecoderOps gxsub_dvb_decoder = {
	dvbsub_decoder_open,
	dvbsub_decoder_close,
	dvbsub_start,
	dvbsub_stop,
	dvbsub_decoder_synchronize,
	dvbsub_decoder_set_stream,
	dvbsub_decode,
	dvbsub_render
};
