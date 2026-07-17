/*****************************************************************************
 *   "Gif-Lib" - Yet another gif library.				     *
 *									     *
 * Written by:  Gershon Elber			IBM PC Ver 0.1,	Jun. 1989    *
 ******************************************************************************
 * Program to dump GIF file content as TEXT information			     *
 * Options:								     *
 * -c : include the color maps as well.					     *
 * -e : include encoded information packed as bytes as well.		     *
 * -z : include encoded information (12bits) codes as result from the zl alg. *
 * -p : dump pixel information instead of encoded information.		     *
 * -h : on line help.							     *
 ******************************************************************************
 * History:								     *
 * 28 Jun 89 - Version 1.0 by Gershon Elber.				     *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
//#include <conio.h>
//#include <curses.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include "gif_lib.h"
#include "getarg.h"
#include "gal.h"
#include "gdi.h"
#include "hd_gal.h"
#include "gui_private.h"

#ifdef LINUX_OS
#include <unistd.h> //for mmap

#if (_FILE_OFFSET_BITS==64)
#undef __USE_FILE_OFFSET64
#include <sys/mman.h> //for mmap

#define __USE_FILE_OFFSET64

#else
#include <sys/mman.h> //for mmap
#endif

#endif

#define SOFT_GIF_BLIT

#define PROGRAM_NAME	"GifText"
#define VERSION		"?Version 1.0, "

#define MAX_FILE_SIZE        (1024 * 1024)

#define GIF_DISPOSAL_BACKGROUND                  (8)
#define GIF_DISPOSAL_PREV_FRAME                  (4)

#define ALIGN_EVEN(n)   \
	if(n % 2) { \
		n -= 1; \
	}

#define MAKE_PRINTABLE(c)  (isprint(c) ? (c) : ' ')
static int InterlacedOffset[] = { 0, 4, 2, 1 }, /* The way Interlaced image should  */
	   InterlacedJumps[] = { 8, 8, 4, 2 };    /* be read - offsets and jumps... */

image_desc *IMG_GIF_Load(image_desc *gif_desc, const char *pFile)
{
	GIF_Para *gif_src = NULL;
	GIF_Image *pGifImag = NULL, *pPreImg = NULL, *pNewImg = NULL;
	GifFileType *GifFile = NULL;
	unsigned char *GIF_buffer = NULL, *pData = NULL;
	char *filename = NULL;
	struct stat buf;
	ByteType *Extension = NULL;
	int ret = 0, Len = 0;
	int i = 0, j = 0, ExtCode = 0;
	int disposal = 0, delaytime = 0;
	Color nClut = 0, ColorKeyIndex = 0, enabeltran = 0;
	GifRecordType RecordType;
	GifColorType *pClut = NULL;
	handle_t pSrc = 0;

	if (pFile == NULL)
		return (NULL);

	ret = stat(pFile, &buf);
	if (ret < 0)
		return (NULL);

	if(buf.st_size > MAX_FILE_SIZE) {
		return (NULL);
	}

	pSrc = GxCore_Open((char *)pFile, "r");

	if (pSrc) {
        if(NULL == gif_desc) {
            gif_desc = (image_desc *) GUI_MALLOCZ(sizeof(image_desc));
            if(NULL == gif_desc)
                goto err;
        }

		GIF_buffer = (unsigned char *)GUI_MALLOCZ(buf.st_size + 100);
		if(NULL == GIF_buffer) {
			goto err;
		}

		gif_src = (GIF_Para *)GUI_MALLOCZ(sizeof(GIF_Para));
		if(NULL == gif_src) {
			goto err;
		}

		GxCore_Read(pSrc, GIF_buffer, buf.st_size, 1);

		if(pFile)
		{
		    filename = GUI_STRDUP(pFile);
		    GUI_FREE(gif_desc->filename);
		    gif_desc->filename = filename;
		}

		GifFile = gif_header_parse((char *)GIF_buffer);
		if(NULL == GifFile) {
			goto err;
		}

		/*Basic arguments configure*/
		if(7 == GifFile->SBitsPerPixel)
		{
			gif_src->bpp           = GifFile->SBitsPerPixel + 1;
		}
		else
		{
			gif_src->bpp           = GifFile->SBitsPerPixel;
		}
		gif_src->screen_width  = GifFile->SWidth;
		gif_src->screen_height = GifFile->SHeight;

		gif_desc->width = GifFile->SWidth;
		gif_desc->height = GifFile->SHeight;
		gif_desc->bpp	       = gif_src->bpp;

		/*Global color look-up table*/
		if(GifFile->SColorMap) {
			Len = 1 << GifFile->SBitsPerPixel;
			gif_src->num_color = Len;

			gif_src->pal = (Color *)GUI_MALLOC(gif_src->num_color * sizeof(Color));
			for(i = 0; i < Len; i += 4) {
				for(j = 0; j < 4 && j < Len; j++) {
					nClut = ((GifFile->SColorMap[i + j].Red) << 16) |
						((GifFile->SColorMap[i + j].Green) << 8) |
						((GifFile->SColorMap[i + j].Blue));

					gif_src->pal[i + j] = nClut;
				}
			}

			gif_src->backcolor = gif_src->pal[GifFile->SBackGroundColor];
		}

		do {
			if(GifGetRecordType(GifFile, &RecordType) == ERROR) {
				goto err;
			}

			switch (RecordType) {
				case IMAGE_DESC_RECORD_TYPE:
					pNewImg = (GIF_Image *)GUI_MALLOC(sizeof(GIF_Image));
					if(NULL == pNewImg) {
						goto err;
					}
					memset(pNewImg, 0, sizeof(GIF_Image));

					if(gif_src->each_image)
						pNewImg->delaytime = delaytime;
					pNewImg->disposal = disposal;

					pPreImg = pGifImag = gif_src->each_image;
					while(pGifImag) {
						pPreImg = pGifImag;
						pGifImag = pGifImag->next;
					}
					if(pPreImg)
						pPreImg->next = pNewImg;
					else
						gif_src->each_image = pNewImg;

					if(ERROR == GifGetImageDesc(GifFile)) {
						goto err;
					}
					/*Parse local palette in each image*/
					if(GifFile->IColorMap) {
						Len = 1 << GifFile->IBitsPerPixel;

						pNewImg->num_color = Len;
						if(pNewImg->num_color) {
							pNewImg->img_pal = (Color *)GUI_MALLOC(sizeof(Color) * Len);
							if(NULL == pNewImg->img_pal) {
								goto err;
							}
							memset(pNewImg->img_pal, 0, sizeof(Color) * Len);

							for (i = 0; i < Len; i+=4) {
								for (j = 0; j < 4 && j < Len; j++) {
									nClut = ((GifFile->IColorMap[i + j].Red) << 16) |
										((GifFile->IColorMap[i + j].Green) << 8) |
										((GifFile->IColorMap[i + j].Blue));

									pNewImg->img_pal[i + j] = nClut;
								}
							}
						}
					} else {
						Len = 1 << GifFile->SBitsPerPixel;
						pNewImg->num_color = Len;
						if(pNewImg->num_color) {
							pNewImg->img_pal = (Color *)GUI_MALLOC(sizeof(Color) * Len);
							if(NULL == pNewImg->img_pal) {
								goto err;
							}
							memset(pNewImg->img_pal, 0, sizeof(Color) * Len);
							for (i = 0; i < Len; i+=4) {
								for (j = 0; j < 4 && j < Len; j++) {
									nClut = ((GifFile->SColorMap[i + j].Red) << 16) |
										((GifFile->SColorMap[i + j].Green) << 8) |
										((GifFile->SColorMap[i + j].Blue));

									pNewImg->img_pal[i + j] = nClut;
								}
							}
						}
					}
					if(enabeltran == 1) {
						if(GifFile->IColorMap)
							pClut = GifFile->IColorMap;
						else
							pClut = GifFile->SColorMap;
						if(!pClut)
							goto err;

						nClut = ((pClut[ColorKeyIndex].Red) << 16) |
							((pClut[ColorKeyIndex].Green) << 8) |
							(pClut[ColorKeyIndex].Blue);

						pNewImg->img_trans = nClut;
						pNewImg->color_key_index = ColorKeyIndex;
						pNewImg->enable_trans = enabeltran;
						enabeltran = 0;
					}

					/*Parse pixel information in each image*/
					pNewImg->img_left = GifFile->ILeft;
					pNewImg->img_top = GifFile->ITop;
					pNewImg->img_width = GifFile->IWidth;
					pNewImg->img_height = GifFile->IHeight;
					pNewImg->img_data = (unsigned char *)GUI_MALLOC(GifFile->IWidth *
							GifFile->IHeight *
							sizeof(unsigned char));
					if(NULL == pNewImg->img_data) {
							goto err;
					}

					memset(pNewImg->img_data,
							0,
							GifFile->IWidth * GifFile->IHeight * sizeof(unsigned char));

					if(GifFile->IInterlace)
					{
					    int iRow = 0, iCol = 0, iWidth = 0, iHeight = 0;

					    iRow = GifFile->ITop;
					    iCol = GifFile->ILeft;
					    iWidth = GifFile->IWidth;
					    iHeight = GifFile->IHeight;

					    for (i=0; i<4; i++)
					    {
						for (j=iRow + InterlacedOffset[i]; j<iRow + iHeight;j += InterlacedJumps[i])
						{
						    pData = pNewImg->img_data + j * GifFile->IWidth;
						    if(GifGetLine(GifFile, pData, GifFile->IWidth) == ERROR) {
							    gif_desc->data = (void *)gif_src;
							    gif_desc->type = GIF_TYPE;
							    goto err;
						    }
						}
					    }
					}
					else
					{
					    for(i = 0; i < GifFile->IHeight; i++) {
						    pData = pNewImg->img_data + i * GifFile->IWidth;
						    if(GifGetLine(GifFile, pData, GifFile->IWidth) == ERROR) {
							    gif_desc->data = (void *)gif_src;
							    gif_desc->type = GIF_TYPE;
							    goto err;
						    }
					    }
					}
					gif_src->frame_num++;
					break;
				case EXTENSION_RECORD_TYPE:
					if (GifGetExtension(GifFile, &ExtCode, &Extension) == ERROR) {
							goto err;
					}
					if(ExtCode == 0xF9) {
						disposal = Extension[1] & 0x1c;
						delaytime = Extension[2] | (Extension[3] << 8);
						if((Extension[1] & 0x01) == 1) {
							ColorKeyIndex = Extension[4];
							enabeltran = 1;
						}
					}

					while (Extension != NULL) {
						if (GifGetExtensionNext(GifFile, &Extension) == ERROR) {
							goto err;
						}
					}
					break;
				case TERMINATE_RECORD_TYPE:
					break;
				default:		     /* Should be traps by GxDGifGetRecordType */
					break;
			}
		}while (RecordType != TERMINATE_RECORD_TYPE);
	} else {
		gxlogd("Can't open file : %s\n", pFile);
		return (NULL);
	}

	gif_desc->data = (void *)gif_src;
	gif_desc->type = GIF_TYPE;

	if (GIF_buffer)
		GUI_FREE(GIF_buffer);
	GxCore_Close(pSrc);
	GxDGifCloseFile(GifFile);
	return gif_desc;
err:
	if (gif_src)
		IMG_GIF_Free(gif_src);
	if (gif_desc)
		GUI_FREE(gif_desc);
	if (GIF_buffer)
		GUI_FREE(GIF_buffer);
	GxDGifCloseFile(GifFile);
	GxCore_Close(pSrc);
	return NULL;
}

int IMG_GIF_Free(void *data)
{
	GIF_Para *gif_src = NULL;
	GIF_Image *pGifImage = NULL, *pNext = NULL;

	if(NULL == data)
	{
		return (1);
	}

	gif_src = (GIF_Para *)data;
	pGifImage = gif_src->each_image;

	while(pGifImage)
	{
		pNext = pGifImage->next;

		if(pGifImage->img_pal != gif_src->pal) {
			GUI_FREE(pGifImage->img_pal);
		}
		GUI_FREE(pGifImage->img_data);
		GUI_FREE(pGifImage);

		pGifImage = pNext;
	}

	GUI_FREE(gif_src->pal);

	return (0);
}

static handle_t gif_mutex = -1;

static int gif_lock()
{
	int ret = 0;

	if(gif_mutex == -1) {
		ret = GxCore_MutexCreate(&gif_mutex);
	}
	ret |= GxCore_MutexLock(gif_mutex);

	return (ret);
}

static int gif_unlock()
{
	int ret = 0;

	ret = GxCore_MutexUnlock(gif_mutex);

	return (ret);
}

static int g_GIF_Stop = GUI_TRUE;
static int g_GIF_Exited = GUI_TRUE;
static int gif_pid = 0;

static int gif_thread_stop(void)
{
	int i = 0;

	if(g_GIF_Stop == 1)
		return (0);
	gdi_lock();
	g_GIF_Stop = 1;
	gif_pid = 0;
	while(!g_GIF_Exited) {
		if(i > 500) {
			g_GIF_Exited = GUI_TRUE;
			break;
		}
		gui_delay(10);
		i++;
	}
	gdi_unlock();

	return (0);
}

int IMG_GIF_Stop(void)
{
	int ret = 0;

	ret = gif_thread_stop();

	return (ret);
}

#ifdef SOFT_GIF_BLIT
static int _gif_soft_img_desc(GuiSurface *surface,
			      const image_desc *pDesc,
			      int x,
			      int y,
			      int w,
			      int h)
{
	int ret = 0;

	char *src_data = NULL;
	int *dst_data = NULL, *pal = NULL, i = 0, j = 0, index = 0;

	if(pDesc && pDesc->data) {
		src_data = (char *)(pDesc->data);
	}

	if(pDesc && pDesc->pal) {
		pal = (int *)(pDesc->pal);
	}

	if(surface && surface->data) {
		dst_data = (int *)(surface->data);
	}

	for(i = 0; i < pDesc->height; i++) {
		for(j = 0; j < pDesc->width; j++) {
			index = src_data[i * pDesc->width + j];
			if(pDesc->effect == EFFECT_SRC_TO_DST) {
			   if(index != pDesc->color)
				dst_data[(y + i) * surface->sf.width + x + j] = (int)(pal[index] | 0xFF000000);
			}
			else
				dst_data[(y + i) * surface->sf.width + x + j] = (int)(pal[index] | 0xFF000000);
		}
	}

	return (ret);
}
#endif /*SOFT_GIF_BLIT*/

typedef struct _GIF_Conf
{
	void *screen;
	const char *pGIF;
	image_desc *pdesc;
	int x0;
	int y0;
}GIF_Conf;

static GIF_Conf gif_conf = {0};


static int gif_draw_exec(void *screen, const char *pGIF, int x0, int y0)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GAL_Rect gif_rect = {0}, dst_rect = {0};
	GuiSurface *spp_surface, *gif_surface = NULL;
	image_desc *gif_desc = NULL;
	GIF_Para *gif_src = NULL;
	GIF_Image *pGifImage = NULL;
	int one_frame = 0;

	if((NULL == screen) || (NULL == pGIF) || (NULL == gui_core))
	{
		return (1);
	}

	if(gif_conf.pdesc) {
		gif_desc = gif_conf.pdesc;
	} else {
		gif_desc = IMG_GIF_Load(NULL, pGIF);
		if(NULL == gif_desc)
		{
			return (1);
		}
	}

	spp_surface = (GuiSurface *)spp_screen;

	hd_img_clear();
	gdi_commit();

	gif_rect.x = gif_rect.y = 0;
	gif_rect.w = gif_desc->width;
	gif_rect.h = gif_desc->height;
	gif_surface = hd_get_surface(&gif_rect, 32, NULL, GUI_FALSE);
	if(NULL == gif_surface) {
		gal_img_release(gif_desc);
		return (1);
	}

	gif_src = (GIF_Para *)(gif_desc->data);
	if(NULL == gif_src) {
		gal_img_release(gif_desc);
		return (1);
	}

	pGifImage = (GIF_Image *)(gif_src->each_image);
	if(NULL == pGifImage) {
		gal_img_release(gif_desc);
		return (1);
	}

	if(pGifImage->next == NULL) {
		one_frame = GUI_TRUE;
	}

	gdi_begin();
	hd_fillrect(gif_surface, &gif_rect, 0xFF000000 | gif_src->backcolor);
	gdi_end();

	dst_rect = gif_rect;
	if(gui_core->config.width > gif_desc->width) {
		dst_rect.x = (gui_core->config.width - gif_desc->width) / 2;
	}

	if(gui_core->config.height > gif_desc->height) {
		dst_rect.y = (gui_core->config.height - gif_desc->height) / 2;
	}

	while(1) {
		g_GIF_Exited = GUI_FALSE;
		image_desc desc = {0};

		if(g_GIF_Stop) {
			break;
		}

		desc.type = GIF_TYPE;
		desc.width = pGifImage->img_width;
		desc.height = pGifImage->img_height;
		desc.bpp = gif_desc->bpp;
		if(desc.bpp < 8) {
			desc.bpp = 8;
		}
		desc.size = desc.width * desc.height * desc.bpp / 8;
		desc.data = pGifImage->img_data;

		if(pGifImage->enable_trans) {
			desc.color = pGifImage->color_key_index;
			desc.effect = EFFECT_SRC_TO_DST;
		} else {
			desc.effect = EFFECT_COPY;
		}

		if(pGifImage->img_pal) {
			desc.pal = pGifImage->img_pal;
		} else {
			desc.pal = gif_src->pal;
		}

		gdi_lock();
		gdi_begin();
		if(pGifImage->disposal == GIF_DISPOSAL_BACKGROUND)
			hd_fillrect(gif_surface, &gif_rect, 0xFF000000 | gif_src->backcolor);

#ifdef SOFT_GIF_BLIT
		if(pGifImage->num_color && (pGifImage->num_color <= 2)) {
			desc.bpp = 1;
			desc.size = desc.width * desc.height * desc.bpp / 8;
			hd_img_desc(gif_surface,
				&desc,
				pGifImage->img_left,
				pGifImage->img_top,
				desc.width,
				desc.height);
			gdi_end();
		} else {
			gdi_end();
			_gif_soft_img_desc(gif_surface,
					&desc,
					pGifImage->img_left,
					pGifImage->img_top,
					desc.width,
					desc.height);
		}
#else
		hd_img_desc(gif_surface,
			    &desc,
			    pGifImage->img_left,
			    pGifImage->img_top,
			    desc.width,
			    desc.height);
		gdi_end();
#endif /*SOFT_GIF_BLIT*/

		gdi_begin();
		if((gif_surface->sf.width > spp_surface->sf.width) ||
		   (gif_surface->sf.height > spp_surface->sf.height)) {
			int adjust_width = 0, adjust_height = 0;
			GuiSurface *zoom_surface = NULL;
			GAL_Rect zoom_rect = {0};

			// height
			adjust_height = gui_core->config.height;
			adjust_width = gif_surface->sf.width * adjust_height / gif_surface->sf.height;
			if(adjust_width < gui_core->config.width) {
				;
			} else {
			// width
				adjust_width = gui_core->config.width;
				adjust_height = gif_surface->sf.height * adjust_width / gif_surface->sf.height;
			}
			dst_rect.w = adjust_width;
			dst_rect.h = adjust_height;
			dst_rect.x = (gui_core->config.width - adjust_width) / 2;
			dst_rect.y = (gui_core->config.height - adjust_height) / 2;

			zoom_rect = dst_rect;
			zoom_rect.x = zoom_rect.y = 0;
			zoom_surface = hd_get_surface(&zoom_rect, 32, NULL, GUI_FALSE);
			if(NULL == zoom_surface) {
				break;
			}
			gdi_begin();
			hd_zoom_surface(gif_surface, &gif_rect, zoom_surface, &zoom_rect);

			// YUV422 alignment
			ALIGN_EVEN(gif_rect.w);
			ALIGN_EVEN(dst_rect.w);
			ALIGN_EVEN(dst_rect.x);
			hd_copy_surface(zoom_surface, &zoom_rect, spp_surface, &dst_rect);
			gdi_end();
			if(zoom_surface) {
				hd_free_surface(zoom_surface);
			}
		} else {
			// YUV422 alignment
			ALIGN_EVEN(gif_rect.w);
			ALIGN_EVEN(dst_rect.w);
			ALIGN_EVEN(dst_rect.x);
			hd_copy_surface(gif_surface, &gif_rect, spp_surface, &dst_rect);
		}
		gdi_end();
		gdi_unlock();

		if(one_frame) {
			g_GIF_Stop = GUI_TRUE;
			gif_pid = 0;
			break;
		}
		pGifImage = pGifImage->next;
		if(NULL == pGifImage) {
			pGifImage = (GIF_Image *)(gif_src->each_image);
			gdi_lock();
			gdi_begin();
			hd_fillrect(gif_surface, &gif_rect, 0xFF000000 | gif_src->backcolor);
			gdi_end();
			gdi_unlock();
		}
		memset(&desc, 0, sizeof(image_desc));
		if(pGifImage->delaytime) {
			gui_delay(pGifImage->delaytime * 10);
		} else {
			gui_delay(100);
		}
	}

	if(gif_desc->ops == NULL) {
		IMG_GIF_Free(gif_desc->data);
	}
	gal_img_release(gif_desc);
	hd_free_surface(gif_surface);
	g_GIF_Exited = GUI_TRUE;

	return (0);
}

static void gif_draw(void *data)
{
	GIF_Conf *gif_conf = NULL;

	gif_conf = (GIF_Conf*)data;

	GxCore_ThreadDetach();
	if(NULL == gif_conf)
	{
		return;
	}

	gif_lock();
	g_GIF_Stop = GUI_FALSE;
	while(!g_GIF_Exited) {
		gui_delay(10);
	}

	gif_draw_exec(gif_conf->screen,
		      gif_conf->pGIF,
		      gif_conf->x0,
		      gif_conf->y0);
	gif_unlock();
}

/******************************************************************************
 * Interpret the command line and scan the given GIF file.		      *
 ******************************************************************************/
int IMG_GIF_Draw(void *screen, const char *pGIF, int x0, int y0, int screen_width, int screen_height)
{
	gif_conf.screen = screen;
	gif_conf.pGIF = pGIF;
	gif_conf.x0 = x0;
	gif_conf.y0 = y0;
	if(0 == gif_pid)
	{
		gif_lock();
		gif_conf.pdesc = NULL;
		gif_conf.pdesc = IMG_GIF_Load(NULL, pGIF);
		if(NULL == gif_conf.pdesc) {
			gif_unlock();
			return (1);
		}

		GxCore_ThreadCreate("gif_draw",
				&gif_pid,
				gif_draw,
				&gif_conf,
				1024*50,
				GXOS_DEFAULT_PRIORITY);
		gif_unlock();
	}

	return (0);
}

int IMG_GIF_GetInfo(const char *pFile, Image_Exif *img_info)
{
	int ret = 0, frame = 0;
	handle_t hfile = 0;
	struct stat buf = {0};
	GifFileType *GifFile = NULL;
	unsigned char *buffer, *pData = NULL;

	if((NULL == pFile) || (NULL == img_info))
	{
		return (1);
	}

	hfile = GxCore_Open((char *)pFile, "r");
	if(0 == hfile)
	{
		return (1);
	}

	ret = stat(pFile, &buf);
	if (ret < 0)
	{
		return (1);
	}

	if(buf.st_size > MAX_FILE_SIZE) {
		return (1);
	}

	buffer = (unsigned char *)GUI_MALLOC(buf.st_size + 100);
	if(NULL == buffer)
	{
		GxCore_Close(hfile);
		return (1);
	}

	memset(buffer, 0, buf.st_size + 100);

	GxCore_Read(hfile, buffer, buf.st_size, 1);

	GifFile = gif_header_parse((char *)buffer);
	if(NULL == GifFile)
	{
		GxCore_Close(hfile);
		return (1);
	}

	img_info->bpp = GifFile->SBitsPerPixel + 1;
	img_info->frame_num = frame;
	img_info->width = GifFile->SWidth;
	img_info->height = GifFile->SHeight;
	img_info->horizontal_pixel = -1;
	img_info->vertical_pixel = -1;
	img_info->type = IMAGE_GIF;

	GUI_FREE(GifFile->Private);
	GUI_FREE(GifFile->SColorMap);
	GUI_FREE(GifFile);
	GUI_FREE(buffer);
	GUI_FREE(pData);

	GxCore_Close(hfile);

	return (ret);
}

#if 0
/******************************************************************************
 * Print the given CodeBlock - a string in pascal notation (size in first      *
 * place). Save local information so printing can be performed continuously,   *
 * or reset to start state if Reset. If CodeBlock is NULL, output is flushed   *
 ******************************************************************************/
static void PrintCodeBlock(GifFileType * GifFile, ByteType * CodeBlock, int Reset)
{
	static int CrntPlace = 0, Print = TRUE;
	static long CodeCount = 0;
	//char c;
	int i, Percent, Len = CodeBlock[0];
	long NumBytes;

	if (Reset || CodeBlock == NULL) {
		if (CodeBlock == NULL) {
			if (CrntPlace > 0) {
				if (Print)
					gxlogd("\n");
				CodeCount += CrntPlace - 16;
			}
			if (GifFile->IColorMap)
				NumBytes = ((((long)GifFile->IWidth) * GifFile->IHeight)
						* GifFile->IBitsPerPixel) / 8;
			else
				NumBytes = ((((long)GifFile->IWidth) * GifFile->IHeight)
						* GifFile->SBitsPerPixel) / 8;
			Percent = 100 * CodeCount / NumBytes;
			gxlogd("\nCompression ratio: %ld/%ld (%d)\n", CodeCount, NumBytes, Percent);
			return;
		}
		CrntPlace = 0;
		CodeCount = 0;
		Print = TRUE;
	}

	for (i = 1; i <= Len; i++) {
		if (CrntPlace == 0) {
			if (Print)
				gxlogd("\n%05lxh:  ", CodeCount);
			CodeCount += 16;
		}
		//if (/*kbhit() && */((c = getch()) == 'q' || c == 'Q')) Print = FALSE;
		if (Print)
			gxlogd(" %02xh", CodeBlock[i]);
		if (++CrntPlace >= 16)
			CrntPlace = 0;
	}
}

/******************************************************************************
 * Print the given Extension - a string in pascal notation (size in first      *
 * place). Save local information so printing can be performed continuously,   *
 * or reset to start state if Reset. If Extension is NULL, output is flushed   *
 ******************************************************************************/
static void PrintExtBlock(ByteType * Extension, int Reset)
{
	static int CrntPlace = 0, Print = TRUE;
	static long ExtCount = 0;
	static char HexForm[49], AsciiForm[17];
	//char c;
	int i, Len;

	if (Reset || Extension == NULL) {
		if (Extension == NULL) {
			if (CrntPlace > 0) {
				HexForm[CrntPlace * 3] = 0;
				AsciiForm[CrntPlace] = 0;
				if (Print)
					gxlogd("\n%05lx: %-49s  %-17s\n", ExtCount, HexForm, AsciiForm);
				return;
			} else if (Print)
				gxlogd("\n");
		}
		CrntPlace = 0;
		ExtCount = 0;
		Print = TRUE;
	}

	if (!Print)
		return;

	Len = Extension[0];

	for (i = 1; i <= Len; i++) {
		sprintf(&HexForm[CrntPlace * 3], " %02x", Extension[i]);
		sprintf(&AsciiForm[CrntPlace], "%c", MAKE_PRINTABLE(Extension[i]));
		//if (/*kbhit() && */((c = getch()) == 'q' || c == 'Q')) Print = FALSE;
		if (++CrntPlace == 16) {
			HexForm[CrntPlace * 3] = 0;
			AsciiForm[CrntPlace] = 0;
			if (Print)
				gxlogd("\n%05lx: %-49s  %-17s", ExtCount, HexForm, AsciiForm);
			ExtCount += 16;
			CrntPlace = 0;
		}
	}
}

/******************************************************************************
 * Print the given PixelBlock of length Len.				      *
 * Save local information so printing can be performed continuously,           *
 * or reset to start state if Reset. If PixelBlock is NULL, output is flushed  *
 ******************************************************************************/
static void PrintPixelBlock(ByteType * PixelBlock, int Len, int Reset)
{
	static int CrntPlace = 0, Print = TRUE;
	static long ExtCount = 0;
	static char HexForm[49], AsciiForm[17];
	//char c;
	int i;

	if (Reset || PixelBlock == NULL) {
		if (PixelBlock == NULL) {
			if (CrntPlace > 0) {
				HexForm[CrntPlace * 3] = 0;
				AsciiForm[CrntPlace] = 0;
				if (Print)
					gxlogd("\n%05lx: %-49s  %-17s\n", ExtCount, HexForm, AsciiForm);
			} else if (Print)
				gxlogd("\n");
		}
		CrntPlace = 0;
		ExtCount = 0;
		Print = TRUE;
		if (PixelBlock == NULL)
			return;
	}

	if (!Print)
		return;

	for (i = 0; i < Len; i++) {
		sprintf(&HexForm[CrntPlace * 3], " %02x", PixelBlock[i]);
		sprintf(&AsciiForm[CrntPlace], "%c", MAKE_PRINTABLE(PixelBlock[i]));
		//if (/*kbhit() && */((c = getch()) == 'q' || c == 'Q')) Print = FALSE;
		if (++CrntPlace == 16) {
			HexForm[CrntPlace * 3] = 0;
			AsciiForm[CrntPlace] = 0;
			if (Print)
				gxlogd("\n%05lx: %-49s  %-17s", ExtCount, HexForm, AsciiForm);
			ExtCount += 16;
			CrntPlace = 0;
		}
	}
}

/******************************************************************************
 * Print the image as LZ codes (each 12bits), until EOF marker is reached.     *
 ******************************************************************************/
static void PrintLZCodes(GifFileType * GifFile)
{
	//char c;
	int Code, Print = TRUE, CrntPlace = 0;
	long CodeCount = 0;

	do {
		if (Print && CrntPlace == 0)
			gxlogd("\n%05lx:", CodeCount);
		if (GxDGifGetLZCodes(GifFile, &Code) == ERROR) {
			PrintGifError();
			exit(-1);
		}
		if (Print && Code >= 0)
			gxlogd(" %03x", Code);	/* EOF Code is returned as -1 */
		CodeCount++;
		if (++CrntPlace >= 16)
			CrntPlace = 0;
		//if (/*kbhit() && */((c = getch()) == 'q' || c == 'Q')) Print = FALSE;
	}
	while (Code >= 0);
}
#endif

