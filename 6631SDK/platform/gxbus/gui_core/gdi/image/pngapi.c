//32PNG图片处理
//修改为数据载入时做一次颜色的乘法。
//注：图象失真：1、数据载入时做乘法会丢失低字节数据；2、在做/255的计算时采用了只取高字节的方式，等价于/256会导致失真。
//2008年4月17日：增加从内存载入图象的方法。

#include <stdlib.h>
#include <setjmp.h>
#include <sys/stat.h>

#include "pngapi.h"
#include "png.h"
#include "gal.h"
#ifdef NDS_SUPPORT
#include "pngstruct.h"
#include "pnginfo.h"
#endif
#include "gui_private.h"
#include "hd_system.h"
//#include "sha_image_porting.h"



#ifndef WIDTHBYTES
#define WIDTHBYTES(bits)    (((bits) + 31) / 32 * 4)
#endif

//#define ASM_CORE

//#define HFILE

typedef struct tagMEMFILE
{
	unsigned int pbyData;	//data
	unsigned int pbyCur;	//cur data
	unsigned int  dwRemain;			//valid data size
#ifdef HFILE
	handle_t hfile;
#else
	FILE *fp;
#endif
}MEMFILE,*PMEMFILE;

static jmp_buf png_jmp_buf;

static void MemFile_Init(PMEMFILE pMemFile,handle_t hfile,unsigned int dwSize)
{
#ifdef HFILE
	pMemFile->hfile = hfile;
#else
	pMemFile->fp = (FILE *)hfile;
#endif
	pMemFile->pbyData=pMemFile->pbyCur=0;
	pMemFile->dwRemain=dwSize;
}

static void MemFile_Read(png_structp png_ptr, png_bytep pbyData, png_size_t nLength)
{
	PMEMFILE pMemFile=(PMEMFILE)png_ptr->io_ptr;

	if(NULL == pMemFile)
	    longjmp(png_jmp_buf, 1);

	if(pMemFile->dwRemain>=nLength)
	{
#ifdef HFILE
		GxCore_Read(pMemFile->hfile, pbyData, nLength, 1);
#else
		fread(pbyData, nLength, 1, pMemFile->fp);
#endif
		pMemFile->pbyCur+=nLength;
		pMemFile->dwRemain-=nLength;
	}
	else
	{
		//png_error(png_ptr,"io error");
		longjmp(png_jmp_buf, 1);
	}
}

int _find_color_index(int *clut, int num, int color)
{
	int i = 0;

	if(NULL == clut)
	{
		return (0);
	}

	for(i = 0; i < num; i++)
	{
		if(clut[i] == color)
		{
			return (i);
		}
	}

	return (0);
}

static int _estimate_size(int width, int height, int bpp)
{
	int size = 0;

	switch(bpp) {
	case 8:
		size = (width * height * (bpp + gui.config.bpp)) / 8;
		break;
	case 16:
		size = width * height * bpp / 8;
		break;
	case 24:
		size = width * height *(bpp + 32) / 8;
		break;
	case 32:
	default:
		size = width * height * 3;
		break;
	}

	return (size);
}

int PNG_Check(const char *filename)
{
#ifdef CONFIG_PNG
    MEMFILE memfile;
    int ret = 0, dwSize = 0;
    struct stat buf = {0};
#ifdef HFILE
    handle_t hfile;
#else
    FILE *fp = NULL;
#endif

    ret = stat(filename, &buf);
    if(ret < 0)
	return (GUI_FALSE);

    dwSize = buf.st_size;

    // create read struct
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if(NULL == png_ptr) {
	return (GUI_FALSE);
    }

#ifdef HFILE
    hfile = GxCore_Open((char *)filename, "r");
    if(hfile <= 0) {
	png_destroy_read_struct(&png_ptr, NULL, 0);
	return (GUI_FALSE);
    }
#else
    fp = fopen(filename, "r");
    if(NULL == fp) {
	png_destroy_read_struct(&png_ptr, NULL, 0);
	return (GUI_FALSE);
    }
#endif

    // create info struct
    png_infop info_ptr = png_create_info_struct(png_ptr);
    // create private read struct
#ifdef HFILE
    MemFile_Init(&memfile,hfile,dwSize);
#else
    MemFile_Init(&memfile,(handle_t)fp,dwSize);
#endif
    // init io mode
    if(setjmp(png_jmp_buf))
    {
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);
#ifdef HFILE
	GxCore_Close(hfile);
#else
	fclose(fp);
#endif
	return (GUI_FALSE);
    }
#ifndef NDS_SUPPORT
    if(setjmp(png_ptr->error_jmpbuf))
    {
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
#ifdef HFILE
		GxCore_Close(hfile);
#else
		fclose(fp);
#endif
		return (GUI_FALSE);
    }
    if(setjmp(png_ptr->jmpbuf))
    {
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
#ifdef HFILE
		GxCore_Close(hfile);
#else
		fclose(fp);
#endif
		return (GUI_FALSE);
    }
#else
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
#ifdef HFILE
		GxCore_Close(hfile);
#else
		fclose(fp);
#endif
	    return (GUI_FALSE);
	}
#endif
    png_set_read_fn(png_ptr,&memfile,MemFile_Read);
    png_read_info(png_ptr, info_ptr);
    dwSize = _estimate_size(info_ptr->width, info_ptr->height, info_ptr->bit_depth);
    if(image_config.threshold_size) {
	    if(dwSize > image_config.threshold_size) {
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
#ifdef HFILE
		GxCore_Close(hfile);
#else
		fclose(fp);
#endif
		return (GUI_FALSE);
	    }
    }
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
#ifdef HFILE
		GxCore_Close(hfile);
#else
		fclose(fp);
#endif
    return (GUI_TRUE);
#else
	return (GUI_FALSE);
#endif /*CONFIG_PNG*/
}

static hash_t *g_png_mem_hash = NULL;
static int png_total_size;

void _png_hash_free(void *p)
{
	GALMemory *png_obj = NULL;

	if(p) {
		png_obj = (GALMemory *)p;
		png_total_size -= png_obj->size;
		gal_free_memory(png_obj);
	}

	return;
}

static png_voidp png_user_malloc(png_structp png_ptr, png_size_t size)
{
	GALMemory *png_obj = NULL;
	char key_name[15] = {0};
	png_voidp ptr = NULL;

	if(NULL == g_png_mem_hash) {
		g_png_mem_hash = hash_new(10, _png_hash_free);
		if(NULL == g_png_mem_hash) {
			return (NULL);
		}
	}

	png_obj = gal_alloc_memory(size);
	if(NULL == png_obj) {
		return (NULL);
	}

	if(IMAGE_FROM_VIDEO_MEMORY == png_obj->source) {
		ptr = png_obj->block.vq->usr_p;
	} else {
		ptr = png_obj->block.ptr;
	}
	sprintf(key_name, "%p", ptr);

	hash_add(g_png_mem_hash, key_name, png_obj);
	png_total_size += size;

	return (ptr);
}

static void png_user_free(png_structp png_ptr, png_voidp ptr)
{
	char key_name[15] = {0};
	void *uptr = NULL;

	sprintf(key_name, "%p", ptr);

	uptr = hash_get(g_png_mem_hash, key_name);
	if(NULL == uptr) {
		png_free_default(png_ptr, ptr);
	} else {
		hash_drop(g_png_mem_hash, key_name);
	}
}

static void png_user_clear(void)
{
	png_total_size = 0;
	if(g_png_mem_hash) {
		hash_release(g_png_mem_hash);
		g_png_mem_hash = NULL;
	}
}

image_desc *PNG_LoadData(image_desc *pDes, handle_t hfile, unsigned int dwSize)
{
#ifdef CONFIG_PNG
	MEMFILE memfile;
	int num_entries = 0, i = 0;
	png_colorp pal = NULL;
	int start_ms = 0, end_ms = 0;

	// create read struct
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if(NULL == png_ptr) {
		png_user_clear();
		return (NULL);
	}

	png_set_mem_fn(png_ptr, NULL, png_user_malloc, png_user_free);
	// create info struct
	png_infop info_ptr = png_create_info_struct(png_ptr);
	// create private read struct
	MemFile_Init(&memfile,hfile,dwSize);
	// init io mode
	if(setjmp(png_jmp_buf))
	{
	    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	    png_user_clear();
	    return (NULL);
	}

#ifndef NDS_SUPPORT
	if(setjmp(png_ptr->error_jmpbuf))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		png_user_clear();
		return (NULL);
	}
	if(setjmp(png_ptr->jmpbuf))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
#ifdef HFILE
		GxCore_Close(hfile);
#else
		fclose((FILE *)hfile);
#endif
		png_user_clear();
		return (GUI_FALSE);
	}
#else
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		png_user_clear();
	        return (NULL);
	}
#endif
	png_set_read_fn(png_ptr,&memfile,MemFile_Read);
	// read entire image (high level)
	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND, 0);

	if(NULL == pDes)
	    pDes = gal_img_new();

	if(NULL == pDes) {
	    png_destroy_read_struct(&png_ptr, NULL, 0);
	    return NULL;
	}

	if(info_ptr->valid & PNG_INFO_IDAT) //&& info_ptr->pixel_depth==32)
	{
		int nPixelDepth = 0;

		nPixelDepth = info_ptr->pixel_depth;

		pDes->width = info_ptr->width;
		pDes->height = info_ptr->height;

		pDes->bpp = nPixelDepth;//info_ptr->bit_depth ;

		if(8 == pDes->bpp)
		{
			num_entries = info_ptr->num_palette;
			if(num_entries)
			{
				pDes->pal = (void *)GUI_MALLOC(num_entries * sizeof(int));
			}
			if(pDes->pal)
			{
				memset(pDes->pal, 0, num_entries * sizeof(int));


				pal = info_ptr->palette;

				for(i = 0; i < num_entries; i++)
				{
					((int*)(pDes->pal))[i] = ((pal->red) << 16) |
										  ((pal->green) << 8) |
										  (pal->blue);
					pal++;
				}
			}

			#if 1
			num_entries = info_ptr->rowbytes;
			#else
			num_entries = png_ptr->num_rows;//info_ptr->rowbytes;
			#endif
			pDes->data = (void *)gal_img_alloc_memory(pDes);
			if(NULL == pDes->data)
			{
			    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
			    png_user_clear();
			    return NULL;
			}

			if(info_ptr->valid & PNG_INFO_IDAT)
			{
				int i,j;
				int nPixelDepth = info_ptr->pixel_depth ;

				if (nPixelDepth == 32)
				{
					//do pre-muliple
					for(i=0;i<pDes->height;i++)
					{
						unsigned char *pdata=info_ptr->row_pointers[i];
						for(j=0;j<pDes->width;j++)
						{
							pdata[3] = 0;
							((char *)(pDes->data))[i * pDes->width + j] = _find_color_index((int *)pDes->pal,
																         			      num_entries,
																         			      (((int)pdata[3] << 24) & 0xFF000000)
																				      | ((pdata[0] << 16) & 0x00FF0000)
																				      | ((pdata[1] << 8) & 0x0000FF00)
																				      | (pdata[2] & 0x000000FF));
							pdata += 4;
						}
					}
				}
				else if(nPixelDepth == 24)
				{
					//do pre-muliple
					for(i=0;i<pDes->height;i++)
					{
						unsigned char *pdata=info_ptr->row_pointers[i];
						for(j=0;j<pDes->width;j++)
						{
							((char *)(pDes->data))[i * pDes->width + j] = _find_color_index((int *)pDes->pal,
																         			      num_entries,
																         			      ((pdata[0] << 16) & 0x00FF0000)
																				      | ((pdata[1] << 8) & 0x0000FF00)
																				      | (pdata[2] & 0x000000FF));
							pdata += 3;
						}
					}
				}
				else
				{
					for(i=0;i<pDes->height;i++)
					{
						memcpy(pDes->data + i * pDes->width, info_ptr->row_pointers[i], pDes->width);
					}
				}
			}
		}
		else if(16 == pDes->bpp)
		{
			char *dst_data = NULL;
			unsigned short *src_short = NULL, rgb_value = 0;;
			int i = 0, j = 0, r = 0, g = 0, b = 0, a = 0, br = 0, bg = 0, bb = 0;
			int color = 0xFFFF;

			pDes->data = (void *)gal_img_alloc_memory(pDes);
			if(NULL == pDes->data)
			{
				png_destroy_read_struct(&png_ptr, &info_ptr, 0);
				png_user_clear();
				return NULL;
			}
			dst_data = pDes->data;

			for(i = 0; i < pDes->height; i++)
			{
				src_short = (unsigned short*)(info_ptr->row_pointers[i]);

				for(j = 0; j < pDes->width; j++)
				{
					rgb_value = src_short[j];

					a = (rgb_value&0x0000FF00) >> 8;
					r = g = b = (rgb_value&0x000000ff);

					br = (color & 0x00F80000) >> 16;
					bg = (color & 0x0000FC00) >> 8;
					bb = color & 0x000000F8;

					r = (r * a / 0xFF) + (br - br * a / 0xFF);
					g = (g * a / 0xFF) + (bg - bg * a / 0xFF);
					b = (b * a / 0xFF) + (bb - bb * a / 0xFF);

		#ifdef SWITCH_ELEB
					dst_data[0] = (r & 0xF8) | ( (g&0xE0) >> 5 );
					dst_data[1] = ( (g&0x1C) << 3 ) | ( (b & 0xF8) >> 3);
		#else
					dst_data[1] = (r & 0xF8) | ( (g&0xE0) >> 5 );
					dst_data[0] = ( (g&0x1C) << 3 ) | ( (b & 0xF8) >> 3);
		#endif

					dst_data += 2;
				}
			}
		}
		else if(24 == pDes->bpp)
		{
			num_entries = info_ptr->rowbytes;

			pDes->data = (void *)gal_img_alloc_memory(pDes);
			if(NULL == pDes->data)
			{
			    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
			    png_user_clear();
			    return NULL;
			}


			for(i = 0; i < info_ptr->height; i++)
			{
				if(start_ms == 0) {
					start_ms = hd_get_tick();
				}

				memcpy(&((char *)pDes->data)[i * info_ptr->rowbytes],
					        info_ptr->row_pointers[i],
					        info_ptr->rowbytes);

				end_ms = hd_get_tick();
				if((end_ms - start_ms) >= 500) {
					gui_delay(10);
					end_ms = start_ms = 0;
				}
			}
		}
		else if(32 == pDes->bpp)
		{
		    int i = 0, j = 0;
		    int nPixelDepth = info_ptr->pixel_depth;

		    pDes->data = (void *)gal_img_alloc_memory(pDes);
		    if(NULL == pDes->data)
		    {
		    	png_destroy_read_struct(&png_ptr, &info_ptr, 0);
			png_user_clear();
		    	return NULL;
		    }

		    if(32 == nPixelDepth)
		    {
			num_entries = info_ptr->rowbytes;
			for(i = 0; i < info_ptr->height; i++)
			{
			    if(start_ms == 0) {
				start_ms = hd_get_tick();
			    }

			    for(j = 0; j < info_ptr->width; j++)
			    {
				((char *)pDes->data)[i * info_ptr->rowbytes + j * 4] = info_ptr->row_pointers[i][j * 4 + 2];
				((char *)pDes->data)[i * info_ptr->rowbytes + j * 4 + 1] = info_ptr->row_pointers[i][j * 4 + 1];
				((char *)pDes->data)[i * info_ptr->rowbytes + j * 4 + 2] = info_ptr->row_pointers[i][j * 4];
				((char *)pDes->data)[i * info_ptr->rowbytes + j * 4 + 3] = info_ptr->row_pointers[i][j * 4 + 3];
			    }
			    end_ms = hd_get_tick();
			    if((end_ms - start_ms) >= 500) {
				gui_delay(10);
				end_ms = start_ms = 0;
			    }
			}
		    }
		}
		else
		{
			;
		}


	}

	png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	png_user_clear();
	return pDes;
#else
	return NULL;
#endif /*CONFIG_PNG*/
}

//从文件中载入一个32位png图
PNG_Info_t *PNG_LoadFile(const char * pszFileName)
{
	FILE* file = fopen(pszFileName, "rb");

	// unable to open
	if (file)
	{
		PNG_Info_t *pRet=NULL;
		// create read struct
		png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		if(NULL == png_ptr) {
			fclose(file);
			return (NULL);
		}

		// create info struct
		png_infop info_ptr = png_create_info_struct(png_ptr);

		//init 把文件名字赋值给  io_ptr
		png_init_io(png_ptr, file);

		// read entire image (high level)
		png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND, 0);

		fclose(file);

		if(info_ptr->valid & PNG_INFO_IDAT) //&& info_ptr->pixel_depth==32)
		{
			int i,j;
			int nPixelDepth = info_ptr->pixel_depth ;

			pRet=(PNG_Info_t*)GUI_MALLOC(sizeof(PNG_Info_t));
			pRet->nWidth=info_ptr->width;
			pRet->nHeight=info_ptr->height;
			pRet->ppbyRow=info_ptr->row_pointers;

			pRet->nBitDepth = info_ptr->bit_depth ;
			pRet->nColorType = info_ptr->color_type ;
			pRet->nPixelDepth = info_ptr->pixel_depth ;

			info_ptr->row_pointers=NULL;

			if (nPixelDepth == 32 )
			{
				//do pre-muliple
				for(i=0;i<pRet->nHeight;i++)
				{
					unsigned char *pdata=pRet->ppbyRow[i];
					for(j=0;j<pRet->nWidth;j++)
					{

						pdata[0]=(pdata[0]*pdata[3])/255;
						pdata[1]=(pdata[1]*pdata[3])/255;
						pdata[2]=(pdata[2]*pdata[3])/255;
						pdata[3]=~pdata[3];
						pdata+=4;
					}
				}
			}
			else if (nPixelDepth == 8)
			{
				pRet->pPattle = (unsigned char *)info_ptr->palette ;
			}
		}
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		return pRet;
	}else
	{
		return NULL;
	}
}

//释放png占用的内存
void	PNG_Free(PNG_Info_t *pPNG_Info_t)
{
	int row;
	if(!pPNG_Info_t->ppbyRow) return;
	for (row = 0; row < pPNG_Info_t->nHeight; row++)
	{
		GUI_FREE(pPNG_Info_t->ppbyRow[row]);
	}
	GUI_FREE(pPNG_Info_t->ppbyRow);
	GUI_FREE(pPNG_Info_t);
}


//#include "gxosd.h"
//#include "gxuart.h"

//extern GXOSD_RegionHandle_t g_RegionHandle;
//extern GXUART_Handle_t             UartHandle;

#if 0
void   PNG_Show(const unsigned char *pbyData,unsigned int dwSize,int x0,int y0)
{
   	PNG_Info_t *pPNGRet = NULL;


    unsigned char **row_pointers  ;

	int i,j;
	unsigned char r,g,b ;
    unsigned char r1,g1,b1 ;
	unsigned char nPattle ;
	U16 pCbYCrY[700];
	 U32 y = 0;
	 U32 cb0 = 0;
	 U32 cr0 = 0;
	 U16 cby = 0;
	 U16 cry = 0;
	int nPixelDepth = 0 ;
    //Color_t cr ;

    pPNGRet = PNG_LoadData(pbyData,dwSize);
    if (pPNGRet == NULL )
        return ;

 //   GXUART_Printf(UartHandle,"png loaddata end \n");

	row_pointers = pPNGRet->ppbyRow ;
	nPixelDepth = pPNGRet->nPixelDepth ;
//	Shap_Image_Init(0, 0, 700, 576, 0);

	if (nPixelDepth == 32)
	{

		//GXOSD_GetPixel(g_RegionHandle, x0, y0, &cr);
       	// r1 = (cr.Value.RGBA.R <<3) | (cr.Value.RGBA.R>>2)  ;
		//g1 = (cr.Value.RGBA.G <<2) | (cr.Value.RGBA.G>>4) ;
		//b1 = (cr.Value.RGBA.B <<3) | (cr.Value.RGBA.B>>2) ;
		//Shap_Image_Init(0, 0, 700, 576, 0);
		for(i=0;i<pPNGRet->nHeight;i++)
		{
			unsigned char *p2=*(row_pointers++);

			for(j=0;j<pPNGRet->nWidth;j++)
			{
				r = p2[0];//r = ((p2[3]*(p2[0]))>>8)+p2[0];
				g = p2[1];//g = ((p2[3]*(p2[1]))>>8)+p2[1];
				b = p2[2];//b = ((p2[3]*(p2[2]))>>8)+p2[2];
				p2+=4;

              /* cr.Type = COLOR_TYPE_RGB565 ;
                cr.Value.RGBA.R = r ;
                cr.Value.RGBA.G = g ;
                cr.Value.RGBA.B = b ;
                GXOSD_SetPixel(g_RegionHandle, x0+j,y0+i, &cr);*/
               		if(j%2)
               		{
					y = 16*p2[3]+((4*r+8*g+2*b)>>4);
					cr0 = 128+((7*r-6*g-b)>>4);
					cry = (cr0<<8)+y;
					pCbYCrY[j] = cry;
				}
				else
				{
					y = 16*p2[3]+((4*r+8*g+2*b)>>4);
					cb0 = 128+((-2*r-5*g+7*b)>>4);
					cby = (cb0<<8)+y;
					pCbYCrY[j]= cby;
				}

			}
//			Shap_Image_CopyRows(pPNGRet->nWidth, 700,  pCbYCrY, x0, y0,i);
		}
	}
	else if (nPixelDepth == 24)
	{
			for(i=0;i<pPNGRet->nHeight;i++)
			{
				unsigned char *p2=*(row_pointers++);
				for(j=0;j<pPNGRet->nWidth;j++)
				{
					r = p2[0];
					g = p2[1];
					b = p2[2];
					p2+=3;
					if(j%2)
	               		{
						y = 16*p2[3]+((4*r+8*g+2*b)>>4);
						cr0 = 128+((7*r-6*g-b)>>4);
						cry = (cr0<<8)+y;
						pCbYCrY[j] = cry;
					}
					else
					{
						y = 16*p2[3]+((4*r+8*g+2*b)>>4);
						cb0 = 128+((-2*r-5*g+7*b)>>4);
						cby = (cb0<<8)+y;
						pCbYCrY[j]= cby;
					}

                 /*   cr.Type = COLOR_TYPE_RGB565 ;
                	cr.Value.RGBA.R = r ;
                	cr.Value.RGBA.G = g ;
                	cr.Value.RGBA.B = b ;
			GXOSD_SetPixel(g_RegionHandle, x0+j,y0+i, &cr);*/
				}
//			Shap_Image_CopyRows(pPNGRet->nWidth, 700,  pCbYCrY, x0, y0,i);
			}
	}
	else if (nPixelDepth == 8)
	{
		for(i=0;i<pPNGRet->nHeight;i++)
		{
			unsigned char *p2=*(row_pointers++);

			for(j=0;j<pPNGRet->nWidth;j++)
			{
				nPattle = p2[j];

				r = pPNGRet->pPattle[nPattle*3+0]; //
				g = pPNGRet->pPattle[nPattle*3+1];
				b = pPNGRet->pPattle[nPattle*3+2];
				if(j%2)
               		{
					y = 16*p2[3]+((4*r+8*g+2*b)>>4);
					cr0 = 128+((7*r-6*g-b)>>4);
					cry = (cr0<<8)+y;
					pCbYCrY[j] = cry;
				}
				else
				{
					y = 16*p2[3]+((4*r+8*g+2*b)>>4);
					cb0 = 128+((-2*r-5*g+7*b)>>4);
					cby = (cb0<<8)+y;
					pCbYCrY[j]= cby;
				}

              /*cr.Type = COLOR_TYPE_RGB565 ;
                cr.Value.RGBA.R = r ;
                cr.Value.RGBA.G = g ;
                cr.Value.RGBA.B = b ;
		GXOSD_SetPixel(g_RegionHandle, x0+j,y0+i, &cr);*/
			}
//		Shap_Image_CopyRows(pPNGRet->nWidth, 700,  pCbYCrY, x0,y0, i);
		}
	}

    //GXUART_Printf(UartHandle,"png show end \n");

	if (pPNGRet)
	    PNG_Free(pPNGRet);

  	return ;
}
#endif

