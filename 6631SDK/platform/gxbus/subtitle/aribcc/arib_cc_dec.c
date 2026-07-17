#include "gx_mem.h"
#include "gx_subtitle_module.h"
#include "arib_cc_dec.h"
#include "aribb24/aribb24.h"
#include "aribb24/decoder.h"
#include "aribb24/parser.h"

typedef struct {
	arib_instance_t *p_install;
	void            *priv;
	AribccEvent     ev;
} aribccContext;

void* AribccCreate(void)
{
	aribccContext *ctx = av_mallocz(sizeof(aribccContext));

	ctx->p_install = arib_instance_new((void *)ctx);

	return (void *)ctx;
}

void AribccDestory(void *handle)
{
	aribccContext *ctx = (aribccContext *)handle;

	arib_instance_destroy(ctx->p_install);

	av_free(ctx);
}

void AribccDecode(void *handle, uint8_t *buffer, uint32_t size)
{
	aribccContext  *ctx = (aribccContext *)handle;
	arib_parser_t  *p_parser  = arib_get_parser(ctx->p_install);;
	arib_decoder_t *p_decoder = arib_get_decoder(ctx->p_install);

	if (!ctx) return;

	if (p_parser && p_decoder) {
		int i = 0;
		char *psz_subtitle = NULL;
		size_t i_data_size = 0, i_subtitle_size = 0;
		const unsigned char *psz_data = NULL;
		const arib_buf_region_t *p_region = NULL;
		GxSubtitleShowTextPage page;

		arib_parse_pes(p_parser, buffer, size);
		psz_data = arib_parser_get_data(p_parser, &i_data_size);
		if (!psz_data || !i_data_size)
			return;

		i_subtitle_size = i_data_size * 4;
		psz_subtitle = (char*) av_mallocz((i_subtitle_size + 1)* sizeof(*psz_subtitle));
		if (!psz_subtitle)
			return;

		//arib_initialize_decoder_a_profile(p_decoder);
		arib_initialize_decoder_c_profile(p_decoder);
		i_subtitle_size = arib_decode_buffer(p_decoder, psz_data, i_data_size, psz_subtitle, i_subtitle_size);
		page.lines.lines = 0;
		for (p_region = arib_decoder_get_regions(p_decoder); p_region; p_region = p_region->p_next) {
			int i_size = p_region->p_end - p_region->p_start;

			page.lines.text[page.lines.lines] = (char*)av_mallocz(i_size + 1);
			strncpy(page.lines.text[page.lines.lines], p_region->p_start, i_size);
			page.lines.lines++;
		}

		if (ctx->ev)
			ctx->ev(ctx->priv, &page);

		av_free(psz_subtitle);
		for (i = 0; i < page.lines.lines; i++)
			av_free(page.lines.text[i]);

		arib_finalize_decoder(p_decoder);
	}

	return;
}

void AribccRegisterEvent(void *handle, void *priv, AribccEvent ev)
{
	aribccContext  *ctx = (aribccContext *)handle;

	if (!ctx) return;

	ctx->ev   = ev;
	ctx->priv = priv;

	return;
}
