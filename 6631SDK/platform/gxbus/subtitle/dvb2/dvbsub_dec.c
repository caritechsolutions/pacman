 /* DVB subtitle decoding
 * Copyright (c) 2005 Ian Caulfield
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "get_bits.h"
#include "gx_subtitle_show_pixel.h"
#include "gx_subtitle_config.h"

static char debug_dvb_sub = 0;
#define DVBSUB_PAGE_SEGMENT     0x10
#define DVBSUB_REGION_SEGMENT   0x11
#define DVBSUB_CLUT_SEGMENT     0x12
#define DVBSUB_OBJECT_SEGMENT   0x13
#define DVBSUB_DISPLAYDEFINITION_SEGMENT 0x14
#define DVBSUB_DISPLAY_SEGMENT  0x80

#define MAX_NEG_CROP 1024

#define times4(x) x, x, x, x
#define times256(x) times4(times4(times4(times4(times4(x)))))
const uint8_t ff_crop_tab[256 + 2 * MAX_NEG_CROP] = {
	times256(0x00),
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
	0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
	0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
	0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
	0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
	0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
	0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
	0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF,
	times256(0xFF)};

#define cm (ff_crop_tab + MAX_NEG_CROP)

#define DVB_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define DVB_MIN(a, b) (((a) > (b)) ? (b) : (a))
#define RGBA(r,g,b,a) (((unsigned)(a) << 24) | ((r) << 16) | ((g) << 8) | (b))

#define SCALEBITS 10
#define ONE_HALF  (1 << (SCALEBITS - 1))
#define FIX(x)    ((int) ((x) * (1<<SCALEBITS) + 0.5))
#define YUV_TO_RGB1_CCIR(cb1, cr1)\
{\
	cb = (cb1) - 128;\
	cr = (cr1) - 128;\
	r_add = FIX(1.40200*255.0/224.0) * cr + ONE_HALF;\
	g_add = - FIX(0.34414*255.0/224.0) * cb - FIX(0.71414*255.0/224.0) * cr + ONE_HALF;\
	b_add = FIX(1.77200*255.0/224.0) * cb + ONE_HALF;\
}

#define YUV_TO_RGB2_CCIR(r, g, b, y1)\
{\
	y = ((y1) - 16) * FIX(255.0/219.0);\
	r = cm[(y + r_add) >> SCALEBITS];\
	g = cm[(y + g_add) >> SCALEBITS];\
	b = cm[(y + b_add) >> SCALEBITS];\
}

typedef struct DVBSubCLUT {
	int id;
	int version;

	uint32_t clut4[4];
	uint32_t clut16[16];
	uint32_t clut256[256];

	struct DVBSubCLUT *next;
} DVBSubCLUT;

typedef struct DVBSubObjectDisplay {
	int object_id;
	int region_id;

	int x_pos;
	int y_pos;

	int fgcolor;
	int bgcolor;

	struct DVBSubObjectDisplay *region_list_next;
	struct DVBSubObjectDisplay *object_list_next;
} DVBSubObjectDisplay;

typedef struct DVBSubObject {
	int id;
	int version;

	int type;

	DVBSubObjectDisplay *display_list;

	struct DVBSubObject *next;
} DVBSubObject;

typedef struct DVBSubRegionDisplay {
	int region_id;

	int x_pos;
	int y_pos;

	struct DVBSubRegionDisplay *next;
} DVBSubRegionDisplay;

typedef struct DVBSubRegion {
	int id;
	int version;

	int width;
	int height;
	int depth;

	int clut;
	int bgcolor;

	uint8_t computed_clut[4*256];
	int has_computed_clut;

	uint8_t *pbuf;
	int buf_size;
	int dirty;
	int display;

	DVBSubObjectDisplay *display_list;

	struct DVBSubRegion *next;
} DVBSubRegion;

typedef struct DVBSubDisplayDefinition {
	int version;

	int x;
	int y;
	int width;
	int height;
} DVBSubDisplayDefinition;

typedef struct DVBSubContext {
	handle_t show_handle;
	int composition_id;
	int ancillary_id;

	int version;
	int time_out;
	int compute_clut;
	int clut_count2[257][256];
	unsigned char first_display; //第一次显示，需要保证有足够资源，否则显示异常
	DVBSubRegion *region_list;
	DVBSubCLUT   *clut_list;
	DVBSubObject *object_list;

	DVBSubRegionDisplay     *display_list;
	DVBSubDisplayDefinition *display_definition;

	DVBSubCLUT default_clut;

	GxSubtitleShowCanvas cv;
} DVBSubContext;

static void dvbsub_find_right_window(unsigned int resx, unsigned int resy,
		unsigned int *def_width, unsigned int *def_height)
{
	if ((*def_width == 0) || (*def_height == 0)) {
		*def_width  = 720;
		*def_height = 576;
	}

	if ((resx <= *def_width) && (resy <= *def_height))
		return;
	else {
		if ((resx > 1280) || (resy > 720)) {
			*def_width  = 1920;
			*def_height = 1080;
		} else if ((resx > 720) || (resy > 576)) {
			*def_width  = 1280;
			*def_height = 720;
		} else {
			*def_width  = 720;
			*def_height = 576;
		}
	}

	return;
}

static void compute_default_clut(DVBSubContext *ctx, DVBSubRegion *region, int w, int h)
{
	uint8_t list[256] = {0};
	uint8_t list_inv[256];
	int counttab[256] = {0};
	int (*counttab2)[256] = ctx->clut_count2;
	int count, i, x, y;
	ptrdiff_t stride = w;

	memset(ctx->clut_count2, 0 , sizeof(ctx->clut_count2));
#define V(x,y) region->pbuf[(x) + (y)*stride]
	for (y = 0; y<h; y++) {
		for (x = 0; x<w; x++) {
			int v = V(x,y) + 1;
			int vl = x     ? V(x-1,y) + 1 : 0;
			int vr = x+1<w ? V(x+1,y) + 1 : 0;
			int vt = y     ? V(x,y-1) + 1 : 0;
			int vb = y+1<h ? V(x,y+1) + 1 : 0;
			counttab[v-1] += !!((v!=vl) + (v!=vr) + (v!=vt) + (v!=vb));
			counttab2[vl][v-1] ++;
			counttab2[vr][v-1] ++;
			counttab2[vt][v-1] ++;
			counttab2[vb][v-1] ++;
		}
	}
#define L(x,y) list[d[(x) + (y)*stride]]

	for (i = 0; i<256; i++) {
		counttab2[i+1][i] = 0;
	}

	for (i = 0; i<256; i++) {
		int bestscore = 0;
		int bestv = 0;
		for (x = 0; x < 256; x++) {
			int scorev = 0;
			if (list[x])
				continue;
			scorev += counttab2[0][x];
			for (y = 0; y < 256; y++) {
				scorev += list[y] * counttab2[y+1][x];
			}
			if (scorev) {
				int score = 1024LL*scorev / counttab[x];
				if (score > bestscore) {
					bestscore = score;
					bestv = x;
				}
			}
		}
		if (!bestscore)
			break;
		list    [ bestv ] = 1;
		list_inv[     i ] = bestv;
	}
	count = FFMAX(i - 1, 1);
	for (i--; i>=0; i--) {
		int v = i*255/count;
		uint32_t *clut_color = (uint32_t *)(region->computed_clut + 4*list_inv[i]);

		*clut_color =  RGBA(v,v,v,v);
	}
}

static DVBSubObject* get_object(DVBSubContext *ctx, int object_id)
{
	DVBSubObject *ptr = ctx->object_list;

	while (ptr && ptr->id != object_id) {
		ptr = ptr->next;
	}

	return ptr;
}

static DVBSubCLUT* get_clut(DVBSubContext *ctx, int clut_id)
{
	DVBSubCLUT *ptr = ctx->clut_list;

	while (ptr && ptr->id != clut_id) {
		ptr = ptr->next;
	}

	return ptr;
}

static DVBSubRegion* get_region(DVBSubContext *ctx, int region_id)
{
	DVBSubRegion *ptr = ctx->region_list;

	while (ptr && ptr->id != region_id) {
		ptr = ptr->next;
	}

	return ptr;
}

static DVBSubRegionDisplay *get_display(DVBSubContext *ctx, int region_id)
{
	DVBSubRegionDisplay *ptr = ctx->display_list;

	while (ptr && ptr->region_id != region_id) {
		ptr = ptr->next;
	}

	return ptr;
}

static void delete_region_display_list(DVBSubContext *ctx, DVBSubRegion *region)
{
	DVBSubObject *object, *obj2, **obj2_ptr;
	DVBSubObjectDisplay *display, *obj_disp, **obj_disp_ptr;

	while (region->display_list) {
		display = region->display_list;

		object = get_object(ctx, display->object_id);

		if (object) {
			obj_disp_ptr = &object->display_list;
			obj_disp = *obj_disp_ptr;

			while (obj_disp && obj_disp != display) {
				obj_disp_ptr = &obj_disp->object_list_next;
				obj_disp = *obj_disp_ptr;
			}

			if (obj_disp) {
				*obj_disp_ptr = obj_disp->object_list_next;

				if (!object->display_list) {
					obj2_ptr = &ctx->object_list;
					obj2 = *obj2_ptr;

					while (obj2 != object) {
						obj2_ptr = &obj2->next;
						obj2 = *obj2_ptr;
					}

					*obj2_ptr = obj2->next;

					av_freep(&obj2);
				}
			}
		}

		region->display_list = display->region_list_next;

		av_freep(&display);
	}

}

static void delete_cluts(DVBSubContext *ctx)
{
	while (ctx->clut_list) {
		DVBSubCLUT *clut = ctx->clut_list;

		ctx->clut_list = clut->next;

		av_freep(&clut);
	}
}

static void delete_objects(DVBSubContext *ctx)
{
	while (ctx->object_list) {
		DVBSubObject *object = ctx->object_list;

		ctx->object_list = object->next;

		av_freep(&object);
	}
}

static void delete_regions(DVBSubContext *ctx)
{
	while (ctx->region_list) {
		DVBSubRegion *region = ctx->region_list;

		ctx->region_list = region->next;

		delete_region_display_list(ctx, region);

		av_freep(&region->pbuf);
		av_freep(&region);
	}
}

static int dvbsub_read_2bit_string(uint8_t *destbuf, int dbuf_len,
		const uint8_t **srcbuf, int buf_size,
		int non_mod, uint8_t *map_table, int x_pos)
{
	GetBitContext gb;

	int bits;
	int run_length;
	int pixels_read = x_pos;

	init_get_bits(&gb, *srcbuf, buf_size << 3);

	destbuf += x_pos;

	while (get_bits_count(&gb) < buf_size << 3 && pixels_read < dbuf_len) {
		bits = get_bits(&gb, 2);

		if (bits) {
			if (non_mod != 1 || bits != 1) {
				if (map_table)
					*destbuf++ = map_table[bits];
				else
					*destbuf++ = bits;
			}
			pixels_read++;
		} else {
			bits = get_bits1(&gb);
			if (bits == 1) {
				run_length = get_bits(&gb, 3) + 3;
				bits = get_bits(&gb, 2);

				if (non_mod == 1 && bits == 1)
					pixels_read += run_length;
				else {
					if (map_table)
						bits = map_table[bits];
					while (run_length-- > 0 && pixels_read < dbuf_len) {
						*destbuf++ = bits;
						pixels_read++;
					}
				}
			} else {
				bits = get_bits1(&gb);
				if (bits == 0) {
					bits = get_bits(&gb, 2);
					if (bits == 2) {
						run_length = get_bits(&gb, 4) + 12;
						bits = get_bits(&gb, 2);

						if (non_mod == 1 && bits == 1)
							pixels_read += run_length;
						else {
							if (map_table)
								bits = map_table[bits];
							while (run_length-- > 0 && pixels_read < dbuf_len) {
								*destbuf++ = bits;
								pixels_read++;
							}
						}
					} else if (bits == 3) {
						run_length = get_bits(&gb, 8) + 29;
						bits = get_bits(&gb, 2);

						if (non_mod == 1 && bits == 1)
							pixels_read += run_length;
						else {
							if (map_table)
								bits = map_table[bits];
							while (run_length-- > 0 && pixels_read < dbuf_len) {
								*destbuf++ = bits;
								pixels_read++;
							}
						}
					} else if (bits == 1) {
						if (map_table)
							bits = map_table[0];
						else
							bits = 0;
						run_length = 2;
						while (run_length-- > 0 && pixels_read < dbuf_len) {
							*destbuf++ = bits;
							pixels_read++;
						}
					} else {
						(*srcbuf) += (get_bits_count(&gb) + 7) >> 3;
						return pixels_read;
					}
				} else {
					if (map_table)
						bits = map_table[0];
					else
						bits = 0;
					*destbuf++ = bits;
					pixels_read++;
				}
			}
		}
	}

	if (get_bits(&gb, 6))
		gxlogd("%s %d: line overflow\n", __FUNCTION__, __LINE__);

	(*srcbuf) += (get_bits_count(&gb) + 7) >> 3;

	return pixels_read;
}

static int dvbsub_read_4bit_string(uint8_t *destbuf, int dbuf_len,
		const uint8_t **srcbuf, int buf_size,
		int non_mod, uint8_t *map_table, int x_pos)
{
	GetBitContext gb;

	int bits;
	int run_length;
	int pixels_read = x_pos;

	init_get_bits(&gb, *srcbuf, buf_size << 3);

	destbuf += x_pos;

	while (get_bits_count(&gb) < buf_size << 3 && pixels_read < dbuf_len) {
		bits = get_bits(&gb, 4);

		if (bits) {
			if (non_mod != 1 || bits != 1) {
				if (map_table)
					*destbuf++ = map_table[bits];
				else
					*destbuf++ = bits;
			}
			pixels_read++;
		} else {
			bits = get_bits1(&gb);
			if (bits == 0) {
				run_length = get_bits(&gb, 3);

				if (run_length == 0) {
					(*srcbuf) += (get_bits_count(&gb) + 7) >> 3;
					return pixels_read;
				}

				run_length += 2;

				if (map_table)
					bits = map_table[0];
				else
					bits = 0;

				while (run_length-- > 0 && pixels_read < dbuf_len) {
					*destbuf++ = bits;
					pixels_read++;
				}
			} else {
				bits = get_bits1(&gb);
				if (bits == 0) {
					run_length = get_bits(&gb, 2) + 4;
					bits = get_bits(&gb, 4);

					if (non_mod == 1 && bits == 1)
						pixels_read += run_length;
					else {
						if (map_table)
							bits = map_table[bits];
						while (run_length-- > 0 && pixels_read < dbuf_len) {
							*destbuf++ = bits;
							pixels_read++;
						}
					}
				} else {
					bits = get_bits(&gb, 2);
					if (bits == 2) {
						run_length = get_bits(&gb, 4) + 9;
						bits = get_bits(&gb, 4);

						if (non_mod == 1 && bits == 1)
							pixels_read += run_length;
						else {
							if (map_table)
								bits = map_table[bits];
							while (run_length-- > 0 && pixels_read < dbuf_len) {
								*destbuf++ = bits;
								pixels_read++;
							}
						}
					} else if (bits == 3) {
						run_length = get_bits(&gb, 8) + 25;
						bits = get_bits(&gb, 4);

						if (non_mod == 1 && bits == 1)
							pixels_read += run_length;
						else {
							if (map_table)
								bits = map_table[bits];
							while (run_length-- > 0 && pixels_read < dbuf_len) {
								*destbuf++ = bits;
								pixels_read++;
							}
						}
					} else if (bits == 1) {
						if (map_table)
							bits = map_table[0];
						else
							bits = 0;
						run_length = 2;
						while (run_length-- > 0 && pixels_read < dbuf_len) {
							*destbuf++ = bits;
							pixels_read++;
						}
					} else {
						if (map_table)
							bits = map_table[0];
						else
							bits = 0;
						*destbuf++ = bits;
						pixels_read ++;
					}
				}
			}
		}
	}

	if (get_bits(&gb, 8))
		gxlogd("%s %d: line overflow\n", __FUNCTION__, __LINE__);

	(*srcbuf) += (get_bits_count(&gb) + 7) >> 3;

	return pixels_read;
}

static int dvbsub_read_8bit_string(uint8_t *destbuf, int dbuf_len,
		const uint8_t **srcbuf, int buf_size,
		int non_mod, uint8_t *map_table, int x_pos)
{
	const uint8_t *sbuf_end = (*srcbuf) + buf_size;
	int bits;
	int run_length;
	int pixels_read = x_pos;

	destbuf += x_pos;

	while (*srcbuf < sbuf_end && pixels_read < dbuf_len) {
		bits = *(*srcbuf)++;

		if (bits) {
			if (non_mod != 1 || bits != 1) {
				if (map_table)
					*destbuf++ = map_table[bits];
				else
					*destbuf++ = bits;
			}
			pixels_read++;
		} else {
			bits = *(*srcbuf)++;
			run_length = bits & 0x7f;
			if ((bits & 0x80) == 0) {
				if (run_length == 0) {
					return pixels_read;
				}

				bits = 0;
			} else {
				bits = *(*srcbuf)++;
			}
			if (non_mod == 1 && bits == 1)
				pixels_read += run_length;
			else {
				if (map_table)
					bits = map_table[bits];
				while (run_length-- > 0 && pixels_read < dbuf_len) {
					*destbuf++ = bits;
					pixels_read++;
				}
			}
		}
	}

	if (*(*srcbuf)++)
		gxlogd("%s %d: line overflow\n", __FUNCTION__, __LINE__);

	return pixels_read;
}

static void dvbsub_parse_pixel_data_block(DVBSubContext *ctx,
		DVBSubObjectDisplay *display,
		const uint8_t *buf, int buf_size, int top_bottom, int non_mod)
{
	DVBSubRegion *region = get_region(ctx, display->region_id);
	const uint8_t *buf_end = buf + buf_size;
	uint8_t *pbuf;
	int x_pos, y_pos;
	int i;

	uint8_t map2to4[] = { 0x0,  0x7,  0x8,  0xf};
	uint8_t map2to8[] = {0x00, 0x77, 0x88, 0xff};
	uint8_t map4to8[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
		0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
	uint8_t *map_table;

	if (!region)
		return;

	pbuf = region->pbuf;
	region->dirty = 1;

	x_pos = display->x_pos;
	y_pos = display->y_pos;

	y_pos += top_bottom;

	while (buf < buf_end) {
		if (y_pos >= region->height) {
			gxlogd("Invalid object location! %d-%d %d-%d %02x\n", x_pos, region->width, y_pos, region->height, *buf);
			return;
		}

		switch (*buf++) {
		case 0x10:
			if (region->depth == 8)
				map_table = map2to8;
			else if (region->depth == 4)
				map_table = map2to4;
			else
				map_table = NULL;

			x_pos = dvbsub_read_2bit_string(pbuf + (y_pos * region->width),
					region->width, &buf, buf_end - buf,
					non_mod, map_table, x_pos);
			break;
		case 0x11:
			if (region->depth < 4) {
				gxlogd("4-bit pixel string in %d-bit region!\n", region->depth);
				return;
			}

			if (region->depth == 8)
				map_table = map4to8;
			else
				map_table = NULL;

			x_pos = dvbsub_read_4bit_string(pbuf + (y_pos * region->width),
					region->width, &buf, buf_end - buf,
					non_mod, map_table, x_pos);
			break;
		case 0x12:
			if (region->depth < 8) {
				gxlogd("8-bit pixel string in %d-bit region!\n", region->depth);
				return;
			}

			x_pos = dvbsub_read_8bit_string(pbuf + (y_pos * region->width),
					region->width, &buf, buf_end - buf,
					non_mod, NULL, x_pos);
			break;

		case 0x20:
			map2to4[0] = (*buf) >> 4;
			map2to4[1] = (*buf++) & 0xf;
			map2to4[2] = (*buf) >> 4;
			map2to4[3] = (*buf++) & 0xf;
			break;
		case 0x21:
			for (i = 0; i < 4; i++)
				map2to8[i] = *buf++;
			break;
		case 0x22:
			for (i = 0; i < 16; i++)
				map4to8[i] = *buf++;
			break;

		case 0xf0:
			x_pos = display->x_pos;
			y_pos += 2;
			break;
		default:
			//gxlogd("Unknown/unsupported pixel block 0x%x\n", *(buf-1));
			break;
		}
	}

	region->has_computed_clut = 0;
}

static int dvbsub_parse_object_segment(DVBSubContext *ctx,
		const uint8_t *buf, int buf_size)
{
	const uint8_t *buf_end = buf + buf_size;
	int object_id;
	DVBSubObject *object;
	DVBSubObjectDisplay *display;
	int top_field_len, bottom_field_len;

	int coding_method, non_modifying_color;

	object_id = AV_RB16(buf);
	buf += 2;

	object = get_object(ctx, object_id);

	if (!object)
		return AVERROR_INVALIDDATA;

	coding_method = ((*buf) >> 2) & 3;
	non_modifying_color = ((*buf++) >> 1) & 1;

	if (coding_method == 0) {
		top_field_len = AV_RB16(buf);
		buf += 2;
		bottom_field_len = AV_RB16(buf);
		buf += 2;

		if (buf + top_field_len + bottom_field_len > buf_end) {
			gxlogd("Field data size %d+%d too large\n", top_field_len, bottom_field_len);
			return AVERROR_INVALIDDATA;
		}

		for (display = object->display_list; display; display = display->object_list_next) {
			const uint8_t *block = buf;
			int bfl = bottom_field_len;

			dvbsub_parse_pixel_data_block(ctx, display, block, top_field_len, 0,
					non_modifying_color);

			if (bottom_field_len > 0)
				block = buf + top_field_len;
			else
				bfl = top_field_len;

			dvbsub_parse_pixel_data_block(ctx, display, block, bfl, 1,
					non_modifying_color);
		}

	} else {
		gxlogd("Unknown object coding %d\n", coding_method);
	}

return 0;
}

static int dvbsub_parse_clut_segment(DVBSubContext *ctx,
		const uint8_t *buf, int buf_size)
{
	const uint8_t *buf_end = buf + buf_size;
	int clut_id;
	int version;
	DVBSubCLUT *clut;
	int entry_id, depth , full_range;
	int y, cr, cb, alpha;
	int r, g, b, r_add, g_add, b_add;

	clut_id = *buf++;
	version = ((*buf)>>4)&15;
	buf += 1;

	clut = get_clut(ctx, clut_id);

	if (!clut) {
		clut = av_malloc(sizeof(DVBSubCLUT));
		if (!clut)
			return AVERROR(ENOMEM);

		memcpy(clut, &ctx->default_clut, sizeof(DVBSubCLUT));

		clut->id = clut_id;
		clut->version = -1;

		clut->next = ctx->clut_list;
		ctx->clut_list = clut;
	}

	if (clut->version != version) {

		clut->version = version;

		while (buf + 4 < buf_end) {
			entry_id = *buf++;

			depth = (*buf) & 0xe0;

			if (depth == 0) {
				gxlogd("Invalid clut depth 0x%x!\n", *buf);
			}

			full_range = (*buf++) & 1;

			if (full_range) {
				y = *buf++;
				cr = *buf++;
				cb = *buf++;
				alpha = *buf++;
			} else {
				y = buf[0] & 0xfc;
				cr = (((buf[0] & 3) << 2) | ((buf[1] >> 6) & 3)) << 4;
				cb = (buf[1] << 2) & 0xf0;
				alpha = (buf[1] << 6) & 0xc0;

				buf += 2;
			}

			if (y == 0) {
				y = 0x0;
				cb = 0x0;
				cr = 0x0;
				alpha = 0xff;
			}

			YUV_TO_RGB1_CCIR(cb, cr);
			YUV_TO_RGB2_CCIR(r, g, b, y);

			if (!!(depth & 0x80) + !!(depth & 0x40) + !!(depth & 0x20) > 1) {
				gxlogd("More than one bit level marked: %x\n", depth);
			}

			if (depth & 0x80 && entry_id < 4)
				clut->clut4[entry_id] = RGBA(r,g,b,255 - alpha);
			else if (depth & 0x40 && entry_id < 16)
				clut->clut16[entry_id] = RGBA(r,g,b,255 - alpha);
			else if (depth & 0x20)
				clut->clut256[entry_id] = RGBA(r,g,b,255 - alpha);
		}
	}

	return 0;
}

static int dvbsub_parse_region_segment(DVBSubContext *ctx,
		const uint8_t *buf, int buf_size)
{
	const uint8_t *buf_end = buf + buf_size;
	int region_id, object_id;
	int av_unused version;
	DVBSubRegion *region;
	DVBSubObject *object;
	DVBSubObjectDisplay *display;
	int fill;

	if (buf_size < 10)
		return AVERROR_INVALIDDATA;

	region_id = *buf++;

	region = get_region(ctx, region_id);

	if (!region) {
		region = av_mallocz(sizeof(DVBSubRegion));
		if (!region)
			return AVERROR(ENOMEM);

		region->id = region_id;
		region->version = -1;

		region->next = ctx->region_list;
		ctx->region_list = region;
	}

	version = ((*buf)>>4) & 15;
	fill = ((*buf++) >> 3) & 1;

	region->width = AV_RB16(buf);
	buf += 2;
	region->height = AV_RB16(buf);
	buf += 2;

	if (region->width * region->height != region->buf_size) {
		av_free(region->pbuf);

		region->buf_size = region->width * region->height;

		region->pbuf = av_malloc(region->buf_size);
		if (!region->pbuf) {
			region->buf_size =
				region->width =
				region->height = 0;
			return AVERROR(ENOMEM);
		}

		fill = 1;
		region->dirty = 0;
	}

	region->depth = 1 << (((*buf++) >> 2) & 7);
	if(region->depth<2 || region->depth>8){
		region->depth= 4;
	}
	region->clut = *buf++;

	if (region->depth == 8) {
		region->bgcolor = *buf++;
		buf += 1;
	} else {
		buf += 1;

		if (region->depth == 4)
			region->bgcolor = (((*buf++) >> 4) & 15);
		else
			region->bgcolor = (((*buf++) >> 2) & 3);
	}


	if (fill) {
		memset(region->pbuf, region->bgcolor, region->buf_size);
	}

	delete_region_display_list(ctx, region);

	while (buf + 5 < buf_end) {
		object_id = AV_RB16(buf);
		buf += 2;

		object = get_object(ctx, object_id);

		if (!object) {
			object = av_mallocz(sizeof(DVBSubObject));
			if (!object)
				return AVERROR(ENOMEM);

			object->id = object_id;
			object->next = ctx->object_list;
			ctx->object_list = object;
		}

		object->type = (*buf) >> 6;

		display = av_mallocz(sizeof(DVBSubObjectDisplay));
		if (!display)
			return AVERROR(ENOMEM);

		display->object_id = object_id;
		display->region_id = region_id;

		display->x_pos = AV_RB16(buf) & 0xfff;
		buf += 2;
		display->y_pos = AV_RB16(buf) & 0xfff;
		buf += 2;

		if ((object->type == 1 || object->type == 2) && buf+1 < buf_end) {
			display->fgcolor = *buf++;
			display->bgcolor = *buf++;
		}

		display->region_list_next = region->display_list;
		region->display_list = display;

		display->object_list_next = object->display_list;
		object->display_list = display;
	}

	return 0;
}

static int dvbsub_parse_page_segment(DVBSubContext *ctx, const uint8_t *buf, int buf_size)
{
	DVBSubRegionDisplay *display;
	DVBSubRegionDisplay *tmp_display_list, **tmp_ptr;

	const uint8_t *buf_end = buf + buf_size;
	int region_id;
	int page_state;
	int timeout;
	int version;

	if (buf_size < 1)
		return AVERROR_INVALIDDATA;

	timeout = *buf++;
	version = ((*buf)>>4) & 15;
	page_state = ((*buf++) >> 2) & 3;

	if (ctx->version == version) {
		return 0;
	}

	ctx->time_out = (timeout * 1000);
	ctx->version = version;

	if (page_state == 0) {  //解决某些码流页面不清除问题．
		if (ctx->show_handle)
			GxSubtitleShowPixelClean(ctx->show_handle);
	}

	if (page_state == 1 || page_state == 2) {
		if (ctx->show_handle)
			GxSubtitleShowPixelClean(ctx->show_handle);
		delete_regions(ctx);
		delete_objects(ctx);
		delete_cluts(ctx);
	}

	tmp_display_list = ctx->display_list;
	ctx->display_list = NULL;

	while (buf + 5 < buf_end) {
		region_id = *buf++;
		buf += 1;

		display = ctx->display_list;
		while (display && display->region_id != region_id) {
			display = display->next;
		}
		if (display) {
			gxlogd("%s %d: duplicate region\n", __FUNCTION__, __LINE__);
			break;
		}

		display = tmp_display_list;
		tmp_ptr = &tmp_display_list;

		while (display && display->region_id != region_id) {
			tmp_ptr = &display->next;
			display = display->next;
		}

		if (!display) {
			display = av_mallocz(sizeof(DVBSubRegionDisplay));
			if (!display)
				return AVERROR(ENOMEM);
		}

		display->region_id = region_id;

		display->x_pos = AV_RB16(buf);
		buf += 2;
		display->y_pos = AV_RB16(buf);
		buf += 2;

		*tmp_ptr = display->next;

		display->next = ctx->display_list;
		ctx->display_list = display;
	}

	while (tmp_display_list) {
		display = tmp_display_list;

		tmp_display_list = display->next;

		av_freep(&display);
	}

	return 0;
}

static int dvbsub_parse_display_definition_segment(DVBSubContext *ctx, const uint8_t *buf, int buf_size)
{
	DVBSubDisplayDefinition *display_def = ctx->display_definition;
	int dds_version, info_byte;
	const uint8_t *pbuf = buf;

	if (buf_size < 5)
		return AVERROR_INVALIDDATA;

	info_byte   = pbuf[0];
	pbuf++;
	dds_version = info_byte >> 4;
	if (display_def && display_def->version == dds_version)
		return 0; // already have this display definition version

	if (!display_def) {
		display_def             = av_mallocz(sizeof(*display_def));
		if (!display_def)
			return AVERROR(ENOMEM);
		ctx->display_definition = display_def;
	}

	display_def->version = dds_version;
	display_def->x       = 0;
	display_def->y       = 0;
	display_def->width   = (pbuf[0] << 8| pbuf[1]) + 1;
	display_def->height  = (pbuf[2] << 8| pbuf[3]) + 1;
	if (!display_def->height && !display_def->width)
		ctx->first_display = 1;
	pbuf +=4;

	if (info_byte & 1<<3) { // display_window_flag
		if (buf_size < 13)
			return AVERROR_INVALIDDATA;

		display_def->x      = (pbuf[0] << 8 | pbuf[1]);
		display_def->width  = (pbuf[2] << 8 | pbuf[3]) - display_def->x + 1;
		display_def->y      = (pbuf[4] << 8 | pbuf[5]);
		display_def->height = (pbuf[6] << 8 | pbuf[7]) - display_def->y + 1;
		pbuf += 6;
	}

	return 0;
}

static int dvbsub_parse_page_by_region(DVBSubContext *ctx)
{
	//码流未定义显示区域，使用OBjectRegion信息
	//该信息x_pos, y_pos = 0, 显示偏移下边yWinBottom + region height
	DVBSubRegion *region = NULL;
	DVBSubRegionDisplay *display = NULL;
	unsigned int max_y = 0, max_x = 0;
	unsigned int height = 0, y_pos = 0;

	for (region = ctx->region_list; region; region = region->next) {
		DVBSubObjectDisplay *object_display = region->display_list;

		height += region->height;
		if (!object_display)
			continue;

		if (region->display)
			continue;

		if (!get_display(ctx, object_display->region_id)) {
			DVBSubRegionDisplay **display_ptr = &ctx->display_list;

			display = av_mallocz(sizeof(DVBSubRegionDisplay));
			display->region_id = object_display->region_id;
			display->x_pos     = object_display->x_pos;
			display->y_pos     = object_display->y_pos + (region->id * region->height);

			while (*display_ptr) {
				display_ptr = &((*display_ptr)->next);
			}

			*display_ptr = display;

			max_y = DVB_MAX(max_y, display->y_pos + region->height);
			max_x = DVB_MAX(max_x, display->x_pos + region->width);
		}
	}

	if (max_x > 0 && max_y > 0) {
		unsigned int def_width = 0, def_height = 0;

		dvbsub_find_right_window(max_x, max_y, &def_width, &def_height);
		y_pos = def_height - (height + subtitleConfig.defDisPos.yb);
		for (display = ctx->display_list; display; display = display->next)
			display->y_pos += y_pos;
	} else
		return 1;

	return 0;
}

static int dvbsub_display_end_segment(DVBSubContext *ctx)
{
	DVBSubRegionDisplay     *display;
	DVBSubDisplayDefinition *display_def = ctx->display_definition;
	DVBSubRegion *region;
	DVBSubCLUT   *clut;
	unsigned int num_rects = 0;
	unsigned int min_y = 0x1fffffff, max_y = 0;
	unsigned int min_x = 0x1fffffff, max_x = 0;
	unsigned int max_w = 0, max_h = 0;
	GxSubtitleShowDisplay pos;
	unsigned int def_width = 0, def_height = 0;
	unsigned int def_yb = subtitleConfig.defDisPos.yb;
	unsigned int def_yt = subtitleConfig.defDisPos.yt;
	unsigned int bgColor = 0;
	unsigned int *drawBuffer = NULL;
	int y_offset = subtitleConfig.yOffset;

	if (display_def) {
		def_width  = display_def->width;
		def_height = display_def->height;
	} else {
		def_width  = 0;
		def_height = 0;
	}

	if (!ctx->display_list) {
		if (dvbsub_parse_page_by_region(ctx) == 0) {
			if (ctx->show_handle)
				GxSubtitleShowPixelClean(ctx->show_handle);
		}
	}

	for (display = ctx->display_list; display; display = display->next) {
		region = get_region(ctx, display->region_id);
		if (region && region->dirty) {
			num_rects++;
			min_y = DVB_MIN(min_y, display->y_pos);
			max_y = DVB_MAX(max_y, display->y_pos + region->height);
			min_x = DVB_MIN(min_x, display->x_pos);
			max_x = DVB_MAX(max_x, display->x_pos + region->width);
		}
	}

	if ((min_x == 0x1fffffff) || (min_y == 0x1fffffff)) {
		if (ctx->show_handle)
			GxSubtitleShowPixelClean(ctx->show_handle);
		return 0;
	}

	max_w = (max_x - min_x);
	max_h = (max_y - min_y);

	dvbsub_find_right_window(max_x, max_y, &def_width, &def_height);

	if (!ctx->show_handle) {
		GxSubtitleShowScreen sr;
		GxSubtitleShowCanvas cv;

		memcpy(&sr, &subtitleConfig.screen, sizeof(GxSubtitleShowScreen));
		memcpy(&cv, &subtitleConfig.canvas, sizeof(GxSubtitleShowCanvas));
		ctx->cv = cv;
		ctx->show_handle = GxSubtitleShowPixelOpen(&cv, &sr);
		if (!ctx->show_handle) {
			gxloge("subtitle show pixel open error!!!\n");
			return -1;
		}
	}

	drawBuffer = GxSubtitleShowPixelAllocARGB888Buffer(ctx->show_handle, max_w, max_h);
	if (drawBuffer == NULL) {
		gxloge("ARGB8888 buffer alloc fail\n");
		return -1;
	}

	if (num_rects > 0) {
		for (display = ctx->display_list; display; display = display->next) {
			unsigned int i = 0, j = 0;
			uint32_t *clut_table  = NULL;

			region = get_region(ctx, display->region_id);

			if (!region)
				continue;

			if (!region->dirty)
				continue;

			region->display = 1;
			clut = get_clut(ctx, region->clut);
			if (!clut) {
				if (subtitleConfig.dvbClut.type == GX_SUBTITLE_DVB_COMPUTE_CLUT) {
					if (!region->has_computed_clut) {
						compute_default_clut(ctx, region, region->width, region->height);
						region->has_computed_clut = 1;
					}
					clut_table = (uint32_t *)region->computed_clut;
				} else
					clut = &ctx->default_clut;
			}

			if (!clut_table) {
				switch (region->depth) {
				case 2:
					clut_table = clut->clut4;
					break;
				case 8:
					clut_table = clut->clut256;
					break;
				case 4:
				default:
					clut_table = clut->clut16;
					break;
				}
			}

			for (i = 0; i < region->width; i++) {
				for (j = 0; j < region->height; j++) {
					unsigned int x0 = (display->x_pos + i - min_x);
					unsigned int y0 = (display->y_pos + j - min_y) * max_w;
					unsigned int y1 = j * region->width;
					unsigned char r = 0x00, g = 0x00, b = 0x00, a = 0x00;

					r = ((clut_table[region->pbuf[y1 + i]]) >> 16) & 0xff;
					g = ((clut_table[region->pbuf[y1 + i]]) >> 8 ) & 0xff;
					b = ((clut_table[region->pbuf[y1 + i]])) & 0xff;
					a = ((clut_table[region->pbuf[y1 + i]]) >> 24) & 0xff;

					drawBuffer[(x0 + y0)] = ((a << 24) | (r << 16) | (g << 8) | (b));
				}
			}
		}
	}

	if (debug_dvb_sub)
		gxlogi("[debug] x:%d, y:%d, w:%d, h:%d\n", min_x, min_y, max_w, max_h);

	//用户控制y_offset，来调整字幕上下位置.
	if (y_offset > 0)
		min_y = min_y + y_offset;
	else if (((int)min_y + y_offset) > 0)
		min_y = min_y + y_offset;

	//字幕的min_y介于[def_yt, def_height - def_yb]之间，用户可以控制def_yt, def_yb来保证字幕在某些制式下不会超出边缘．
	if (min_y < def_yt)
		min_y = def_yt;
	else if ((min_y + max_h + def_yb) > def_height)
		min_y = def_height - (max_h + def_yb);

	pos.xl = min_x * ctx->cv.w / def_width;
	pos.yt = min_y * ctx->cv.h / def_height;
	pos.xr = (def_width  -  (min_x + max_w)) * ctx->cv.w / def_width;
	pos.yb = (def_height -  (min_y + max_h)) * ctx->cv.h / def_height;
	pos.autoPos = 0;
	GxSubtitleShowPixelSetScreenColor(ctx->show_handle, 0x00000000);
	GxSubtitleShowPixelSetDisplayPos(ctx->show_handle, &pos);
	GxSubtitleShowPixelDrawARGB8888Buffer(ctx->show_handle, drawBuffer, 0);
	GxSubtitleShowPixelFreeARGB8888Bufeer(ctx->show_handle, drawBuffer);

	return 0;
}

int dvbsub_decode(handle_t handle, const uint8_t *buf, int buf_size)
{
	DVBSubContext *ctx = (DVBSubContext *)handle;
	const uint8_t *p, *p_end;
	int segment_type;
	int page_id;
	int segment_length;
	int ret = 0;
	int got_segment = 0;
	int got_dds = 0;

	if (buf_size <= 6 || *buf != 0x0f) {
		gxlogd("%s %d: incomplete or broken packet\n", __FUNCTION__, __LINE__);
		return -1;
	}

	p = buf;
	p_end = buf + buf_size;

	while (p_end - p >= 6 && *p == 0x0f) {
		p += 1;
		segment_type = *p++;
		page_id = AV_RB16(p);
		p += 2;
		segment_length = AV_RB16(p);
		p += 2;

		if (p_end - p < segment_length) {
			ret = -1;
			goto end;
		}

		if (page_id == ctx->composition_id || page_id == ctx->ancillary_id ||
				ctx->composition_id == -1 || ctx->ancillary_id == -1) {
			int ret = 0;

			if (debug_dvb_sub)
				gxlogi("[debug] segment_type: %x\n", segment_type);
			switch (segment_type) {
			case DVBSUB_PAGE_SEGMENT:
				ret = dvbsub_parse_page_segment(ctx, p, segment_length);
				got_segment |= 1;
				break;
			case DVBSUB_REGION_SEGMENT:
				ret = dvbsub_parse_region_segment(ctx, p, segment_length);
				got_segment |= 2;
				break;
			case DVBSUB_CLUT_SEGMENT:
				ret = dvbsub_parse_clut_segment(ctx, p, segment_length);
				if (ret < 0) goto end;
				got_segment |= 4;
				break;
			case DVBSUB_OBJECT_SEGMENT:
				ret = dvbsub_parse_object_segment(ctx, p, segment_length);
				got_segment |= 8;
				break;
			case DVBSUB_DISPLAYDEFINITION_SEGMENT:
				ret = dvbsub_parse_display_definition_segment(ctx, p, segment_length);
				got_dds = 1;
				break;
			case DVBSUB_DISPLAY_SEGMENT:
				ctx->first_display |= ((got_segment == 15) ? 1 : 0);
				if (ctx->first_display)
					ret = dvbsub_display_end_segment(ctx);
				got_segment |= 16;
				break;
			default:
				gxlogd("Subtitling segment type 0x%x, page id %d, length %d\n",
						segment_type, page_id, segment_length);
				break;
			}
			if (ret < 0)
				goto end;
		}

		p += segment_length;
	}
	// Some streams do not send a display segment but if we have all the other
	// segments then we need no further data.
	if (got_segment == 15) {
		dvbsub_display_end_segment(ctx);
	}

end:
	return 0;
}

handle_t dvbsub_init_decoder(void)
{
	int i, r, g, b, a = 0;
	DVBSubContext *ctx = av_mallocz(sizeof(DVBSubContext));

	if (ctx == NULL)
		return 0;

	ctx->composition_id = -1;
	ctx->ancillary_id   = -1;

	ctx->version = -1;

	ctx->default_clut.id = -1;
	ctx->default_clut.next = NULL;

	if (subtitleConfig.dvbClut.type == GX_SUBTITLE_DVB_CONFIG_CLUT) {
		memcpy(ctx->default_clut.clut4,   subtitleConfig.dvbClut.clut4,   sizeof(unsigned int) * 4  );
		memcpy(ctx->default_clut.clut16,  subtitleConfig.dvbClut.clut16,  sizeof(unsigned int) * 16 );
		memcpy(ctx->default_clut.clut256, subtitleConfig.dvbClut.clut256, sizeof(unsigned int) * 256);
	} else {
		ctx->default_clut.clut4[0] = RGBA(  0,   0,   0,   0);
		ctx->default_clut.clut4[1] = RGBA(255, 255, 255, 255);
		ctx->default_clut.clut4[2] = RGBA(  0,   0,   0, 255);
		ctx->default_clut.clut4[3] = RGBA(127, 127, 127, 255);

		ctx->default_clut.clut16[0] = RGBA(  0,   0,   0,   0);
		for (i = 1; i < 16; i++) {
			if (i < 8) {
				r = (i & 1) ? 255 : 0;
				g = (i & 2) ? 255 : 0;
				b = (i & 4) ? 255 : 0;
			} else {
				r = (i & 1) ? 127 : 0;
				g = (i & 2) ? 127 : 0;
				b = (i & 4) ? 127 : 0;
			}
			ctx->default_clut.clut16[i] = RGBA(r, g, b, 255);
		}

		ctx->default_clut.clut256[0] = RGBA(  0,   0,   0,   0);
		for (i = 1; i < 256; i++) {
			if (i < 8) {
				r = (i & 1) ? 255 : 0;
				g = (i & 2) ? 255 : 0;
				b = (i & 4) ? 255 : 0;
				a = 63;
			} else {
				switch (i & 0x88) {
				case 0x00:
					r = ((i & 1) ? 85 : 0) + ((i & 0x10) ? 170 : 0);
					g = ((i & 2) ? 85 : 0) + ((i & 0x20) ? 170 : 0);
					b = ((i & 4) ? 85 : 0) + ((i & 0x40) ? 170 : 0);
					a = 255;
					break;
				case 0x08:
					r = ((i & 1) ? 85 : 0) + ((i & 0x10) ? 170 : 0);
					g = ((i & 2) ? 85 : 0) + ((i & 0x20) ? 170 : 0);
					b = ((i & 4) ? 85 : 0) + ((i & 0x40) ? 170 : 0);
					a = 127;
					break;
				case 0x80:
					r = 127 + ((i & 1) ? 43 : 0) + ((i & 0x10) ? 85 : 0);
					g = 127 + ((i & 2) ? 43 : 0) + ((i & 0x20) ? 85 : 0);
					b = 127 + ((i & 4) ? 43 : 0) + ((i & 0x40) ? 85 : 0);
					a = 255;
					break;
				case 0x88:
					r = ((i & 1) ? 43 : 0) + ((i & 0x10) ? 85 : 0);
					g = ((i & 2) ? 43 : 0) + ((i & 0x20) ? 85 : 0);
					b = ((i & 4) ? 43 : 0) + ((i & 0x40) ? 85 : 0);
					a = 255;
					break;
				}
			}
			ctx->default_clut.clut256[i] = RGBA(r, g, b, a);
		}
	}

	ctx->show_handle = 0;

	return (handle_t)ctx;
}

int dvbsub_close_decoder(handle_t handle)
{
	DVBSubContext *ctx = (DVBSubContext *)handle;
	DVBSubRegionDisplay *display;

	delete_regions(ctx);

	delete_objects(ctx);

	delete_cluts(ctx);

	av_freep(&ctx->display_definition);

	while (ctx->display_list) {
		display = ctx->display_list;
		ctx->display_list = display->next;

		av_freep(&display);
	}

	if (ctx->show_handle) {
		GxSubtitleShowPixelClose(ctx->show_handle);
		ctx->show_handle = 0;
	}

	av_free(ctx);

	return 0;
}

int dvbsub_config_decoder(handle_t handle, int comp_page_id, int anci_page_id)
{
	DVBSubContext *ctx = (DVBSubContext *)handle;

	ctx->composition_id = comp_page_id;
	ctx->ancillary_id   = anci_page_id;

	return 0;
}

int dvbsub_clean(handle_t handle)
{
	DVBSubContext *ctx = (DVBSubContext *)handle;

	if (ctx->show_handle)
		GxSubtitleShowPixelClean(ctx->show_handle);

	return 0;
}

int dvbsub_display_init(handle_t handle)
{
	DVBSubContext *ctx = (DVBSubContext *)handle;

	ctx->first_display = 0;
	return 0;
}

int dvbsub_show_timer(handle_t handle)
{
	return 0;
}
