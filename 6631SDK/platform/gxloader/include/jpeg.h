#ifndef __GX_JPEG_H__
#define __GX_JPEG_H__

/*
 * NOTE:
 * 该结构体是用来存放解码后图片信息的。
 * 通过解码接口能得到w,h,stride以及yuv数据
 * yuv_buf自行申请释放
 * 当调用mix函数的时候可以通过修改(x,y)来调整pic位置以达到图片混合的效果
 * 注意 x + w 不能大于stride
 */
typedef struct {
	u32 x;
	u32 y;
	u32 w;
	u32 h;
	u32 stride; // Stride 也被称作 Pitch，Stride 的值一定大于等于图像的宽度值（用于字节对齐)
	u32 y_buf;
	u32 u_buf;
	u32 v_buf;
}JpegOutBuf;

/* to adapt to all kinds of app logo needs*/

#endif
