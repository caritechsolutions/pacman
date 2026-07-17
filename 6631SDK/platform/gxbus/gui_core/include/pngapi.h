#ifndef _PNG32_HELPER_
#define _PNG32_HELPER_

#include "IMG.h"
#include "gxcore.h"

__BEGIN_DECLS

typedef struct PNG_Info_s
{
	int nWidth;
	int nHeight;
	unsigned char **ppbyRow;

	unsigned char *pPattle    ;  // 调色板
	unsigned char nBitDepth   ;  // 图像深度
	unsigned char nColorType  ;  // 颜色类型 0,1,2,3,4,6
	unsigned char nPixelDepth ;  // 象素
}PNG_Info_t;

/******************************************************************************
 * Function    : PNG_LoadData
 * Drmcription : 得到png解压后数据,
 * Arguments   : [IN]  pPngData 						        原始的数据
                 [IN]  DataSize                               数据长度
 * Returns     : NULL                                       失败
 			     PNG_Info_t                                 一个内部申请的png结构
 * Other       :
 *****************************************************************************/
image_desc *PNG_LoadData(image_desc *pDes, handle_t hfile, unsigned int dwSize);

/*Check if the data is too big or not*/
int PNG_Check(const char *filename);


/******************************************************************************
 * Function    : PNG_Free
 * Drmcription : 释放所有的资源
 * Arguments   : [IN]  pPNG_Info_t 						    png资源结构指针
 * Returns     :

 * Other       :
 *****************************************************************************/
void	    PNG_Free(PNG_Info_t *pPNG_Info_t);


/*

内部实现，如果以库的形式，需要应用提前完成一些内部需要
1：osd handle
2:
如果以接口形式出现，那会专门提供一套接口提前完成。

*/

/******************************************************************************
 * Function    : PNG_Show
 * Drmcription : 直接显示png图片，内部显示完成
 * Arguments   : [IN]  pPngData 						    原始的数据
                 [IN]  DataSize                             数据长度
                 [IN]  x0                                   起始x坐标
				 [IN]  y0                                   起始y坐标
 * Returns     :

 * Other       :
 *****************************************************************************/
void        PNG_Show(const unsigned char *pPngData,unsigned int DataSize,int x0,int y0);



/*文件接口不提供*/
PNG_Info_t *PNG_LoadFile(const char *pszFileName);

__END_DECLS

#endif//_PNG32_HELPER_
