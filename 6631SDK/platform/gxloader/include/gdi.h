/* *****************************************************************************/
/*******************************     readme     ********************************/
/*
palette format example(ABGR):
unsigned int osd_palette[]={
	0xffff0000,	//blue
	0xff00ff00,	//green
	0xff0000ff,	//red
	0x11223344,	//A:0x11, B:0x22, G:0x33, R:0x44
};
palette_count example:
palette_count = sizeof(osd_palette) / sizeof(unsigned int);
*/
/* *****************************************************************************/

#ifndef __GDI_H__
#define __GDI_H__

typedef struct gdi_rect_s{
	u32 x;
	u32 y;
	u32 w;
	u32 h;
}gdi_rect_t;

/* *****************************************************************************/
/* @function   gdi_init
 * @brief      只支持8bpp索引色
 * unsigned int osd_palette[]，格式为ABGR形式。
 * @param      [in]  width                创建的osd显存的宽
 * @param      [in]  height               创建的osd显存的高
 * @param      [in]  palette              色表数组首地址
 * @param      [in]  palette_count        色表数组元素个数
 * @return     0                          执行成功
 * @return     -1                         执行出错
 */
/* *****************************************************************************/
int gdi_init(u32 width, u32 height, unsigned int *palette, u32 palette_count);

/* *****************************************************************************/
/* @function   gdi_draw_text
 * @brief      打字接口
 * @param      [in]  rect                 指示打字位置
 * @param      [in]  front_color          指示字体前景色(abgr)
 * @param      [in]  back_color           指示字体背景色(abgr)
 * @param      [in]  buf                  数据buf，1bpp表示一个像素
 * @return     0                          执行成功
 * @return     -1                         执行出错
 */
/* *****************************************************************************/
int gdi_draw_text(gdi_rect_t rect, u32 front_color, u32 back_color, const unsigned char *buf);

/* *****************************************************************************/
/* @function   gdi_fill_rect
 * @brief      填充矩形接口
 * @param      [in]  rect                 指示填充矩形位置
 * @param      [in]  fill_color           指示填充颜色(abgr)
 * @return     0                          执行成功
 * @return     -1                         执行出错
 */
/* *****************************************************************************/
int gdi_fill_rect(gdi_rect_t rect, u32 fill_color);

/* *****************************************************************************/
/* @function   gdi_draw_pixel
 * @brief      打点接口
 * @param      [in]  x                    指示打点横坐标
 * @param      [in]  y                    指示打点纵坐标
 * @param      [in]  color_index          指示打点索引值
 * @return     0                          执行成功
 * @return     -1                         执行出错
 */
/* *****************************************************************************/
int gdi_draw_pixel(u32 x, u32 y, u8 color_index);

/* *****************************************************************************/
/* @function   gdi_term
 * @brief      当gdi使用完毕后调用该接口关闭osd显示
 * @return     0                          执行成功
 * @return     -1                         执行出错
 */
/* *****************************************************************************/
int gdi_term(void);

#endif

