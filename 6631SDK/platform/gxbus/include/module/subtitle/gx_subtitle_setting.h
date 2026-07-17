/** \addtogroup gxsubtitle
 *  @{
 */

#ifndef __GX_SUBTITLE_SETTING_H__
#define __GX_SUBTITLE_SETTING_H__

/**
 * 显示层类型
 */
typedef enum {
	GX_SUBTITLE_SPP,          ///< SPP 层显示.
	GX_SUBTITLE_OSD,          ///< OSD 层显示, 不支持该层显示.
} GX_SUBTITLE_LAYER;

/**
 * 画布在屏幕显示区域参数
 */
typedef struct {
	unsigned int x, y;       ///< x 轴, y 轴, 以像素为单位,
	unsigned int width;      ///< 宽度.
	unsigned int height;     ///< 高度.
} GX_SUBTITLE_SCREEN;

/**
 * 画布大小参数
 */
typedef struct {
	unsigned int width;      ///< 宽度.
	unsigned int height;     ///< 高度.
} GX_SUBTITLE_CANVAS;

/**
 * 内容在画布画图区域参数
 */
typedef struct {
	GX_SUBTITLE_LAYER layer; ///< 只支持 SSP 层显示.
	unsigned int user;       ///< 模式选择: 0 - 默认模式, xl xr yt yb 配置无效, 1 - 用户模式, xr xl yt yb 配置有效.
	unsigned int xl, xr;     ///< xr xl : x 偏左/右边缘距离.
	unsigned int yt, yb;     ///< yt yb : y 偏上/下边缘距离.
} GX_SUBTITLE_DISPLAY;

/**
 * 文本字幕字体库参数
 */
typedef struct {
	const char       str[32]; ///< 字体库别名,必须与 xml 上描述别名一致,否则为 GUI 默认的字体库.
} GX_SUBTITLE_FONT_LIB;

/**
 * 是否强制设置字幕的语言
 */
typedef enum {
	GX_SUBTITLE_LANG_AUTO,      ///< 不强制设置字幕的语言.
	GX_SUBTITLE_LANG_FORCE,     ///< 强制设置字幕的语言.
} GX_SUBTITLE_LANG_FLAG;

/**
 * 国家语言参数
 */
typedef struct {
	const char             str[4];      ///< 国家语言简写,简写标准参考: gxdocref.
	GX_SUBTITLE_LANG_FLAG  force_flag;  ///< 是否强制设置字幕语言.
} GX_SUBTITLE_LANG;

/**
 * 字体颜色参数
 */
typedef struct {
	unsigned int      user;   ///< 模式选择: 0 - 默认模式, color 配置无效. 1 - 用户模式, color 配置有效.
	unsigned int      color;  ///< 颜色 ARGB8888 格式.
} GX_SUBTITLE_FONT_COLOR;

/**
 * 背景色参数
 */
typedef struct {
	unsigned int      user;   ///< 模式选择: 0 - 默认模式, color 配置无效. 1 - 用户模式, color 配置有效.
	unsigned int      color;  ///< 颜色 ARGB8888 格式.
} GX_SUBTITLE_BACK_COLOR;

/**
 * DVB 字幕调色板类型
 */
typedef enum {
	GX_SUBTITLE_DVB_DEFAULT_CLUT = 0, ///< 默认调色板0.
	GX_SUBTITLE_DVB_COMPUTE_CLUT,     ///< 默认调色板1.
	GX_SUBTITLE_DVB_CONFIG_CLUT,      ///< 用户配置调色板.
} GX_SUBTITLE_DVB_CLUT_TYPE;

/**
 * DVB 字幕调色板参数
 */
typedef struct {
	GX_SUBTITLE_DVB_CLUT_TYPE type;         ///< 调色板类型选择: GX_SUBTITLE_DVB_CONFIG_CLUT: clut4 , clut8, clut8 调色板参数有效. 其他: 内置调色板.
	unsigned int              clut4[4];     ///< 4   索引调色板.
	unsigned int              clut16[16];   ///< 16  索引调色板.
	unsigned int              clut256[256]; ///< 256 索引调色板.
} GX_SUBTITLE_DVB_CLUT;

/**
 * ATSC-CC 字幕显示类型
 */
typedef enum {
	GX_SUBTITLE_ATSC_PIXEL_SHOW,   ///< 位图显示模式.
	GX_SUBTITLE_ATSC_TEXT_SHOW,    ///< 文本显示模式.
} GX_SUBTITLE_ATSCCC_MODE;

/**
 * Teletext-Magazine 透明度配置参数
 */
typedef struct {
	unsigned int user;            ///< 模式选择: 0: 默认模式, alpha 配置无效. 1: 用户模式, alpha 配置有效.
	unsigned int alpha;           ///< 透明度值,范围0x00 ~ 0xff.
} GX_SUBTITLE_MAG_ALPHA;

/**
 * \brief   配置画布在屏幕上的显示区域信息.
 * \details 1. 播放器初始化之后调用,对所有字幕有效,下一次加载字幕生效.
 * \details 2. 字幕画布默认全屏显示,不同分辨率下自动调整至全屏显示.
 * \details 3. 使用场景: 可以小视频窗口显示等功能.
 *
 * \param   sr  [in]   显示区域参数,详见: gxdocref GX_SUBTITLE_SCREEN.
 * \retval   0         成功
 * \retval  -1         失败
 */
int GxSubtitle_ConfigScreen    (GX_SUBTITLE_SCREEN     sr);

/**
 * \brief   配置画布大小.
 * \details 1. 播放器初始化之后调用,对所有字幕有效,下一次加载字幕生效.
 * \details 2. 使用场景: 小内存方案,可以减少画布大小降低使用内存,但显示效果较差.
 *
 * \param   cv  [in]  画布大小参数,详见: gxdocref GX_SUBTITLE_CANVAS. 默认值: 1280 * 720.
 * \retval  0         成功.
 * \retval -1         失败.
 */
int GxSubtitle_ConfigCanvas    (GX_SUBTITLE_CANVAS     cv);

/**
 * \brief   配置字幕宽高限制.
 * \details 1. 播放器初始化之后调用,只对 DVB 字幕有效,下一次加载字幕生效.
 * \details 2. 字幕超过限制宽或高时,字幕不显示.
 * \details 3. 使用场景: 小内存方案,通过限制宽高来减少内存开销.
 *
 * \param   cv  [in]  限制宽高参数,详见: gxdocref GX_SUBTITLE_CANVAS. 默认值: 1920 * 1080.
 * \retval   0        成功.
 * \retval  -1        失败.
 **/
int GxSubtitle_ConfigLimitSrcCV(GX_SUBTITLE_CANVAS     cv);

/**
 * \brief   设置字幕在画布上绘图区域.
 * \details 1. 播放器初始化之后调用,对所有字幕有效,下一条字幕显示生效.
 * \details 2. 使用场景: 通过该设置解决 CVBS / HDMI 输出时字幕显示不全的问题.
 *
 * \param   dis  [in] 画布上绘图区域参数,详见: gxdocref GX_SUBTITLE_DISPLAY. 默认值: .xl = 10, .xr = 10, .yt = 10, .yb = 30.
 * \retval  0         成功.
 * \retval -1         失败.
 **/
int GxSubtitle_SetDisplay      (GX_SUBTITLE_DISPLAY    dis);

/**
 * \brief   设置字幕字体库.
 * \details 1. 播放器初始化之后调用,只对文本字幕有效,下一条字幕显示生效.
 * \details 2. 文本字幕包括,外挂 srt、txt, 内置 txt 等.
 * \details 3. 默认字体库由 GUI 下 XML 描述.
 *
 * \param   lib  [in]  字体库参数,详见: gxdocref GX_SUBTITLE_FONT_LIB.
 * \retval   0         成功.
 * \retval  -1         失败.
 **/
int GxSubtitle_SetFontLib      (GX_SUBTITLE_FONT_LIB   lib);

/**
 * \brief   设置字幕字体颜色
 * \details 1. 播放器初始化之后调用,只对文本字幕有效,下一条字幕显示生效.
 *
 * \param   color  [in]  字体颜色参数,详见: gxdocref GX_SUBTITLE_FONT_COLOR. 默认值: 0xFFFFFFFF.
 * \retval   0           成功.
 * \retval  -1           失败.
 */
int GxSubtitle_SetFontColor    (GX_SUBTITLE_FONT_COLOR color);


/**
 * \brief   配置字幕显示国家语言
 * \details 1. 播放器初始化之后调用,只对 Teletext 字幕有效,下一次加载字幕生效.
 * \details 2. 默认字幕显示为流信息描述的国家语言,可以通过该接口强制切换为其他国家语言.
 *
 * \param   lang [in] 国家语言参数,详见: gxdocref GX_SUBTITLE_LANG.
 * \retval   0        成功.
 * \retval  -1        失败.
 */
int GxSubtitle_SetLang         (GX_SUBTITLE_LANG       lang);

/**
 * \brief   设置字幕显示背景颜色.
 * \details 1. 播放器初始化之后调用,对所有字幕有效,下一条字幕显示生效.
 *
 * \param   color [in] 背景颜色参数,详见: gxdocref GX_SUBTITLE_BACK_COLOR.
 * \retval   0         成功.
 * \retval  -1         失败.
 */
int GxSubtitle_SetBackColor    (GX_SUBTITLE_BACK_COLOR color);

/**
 * \brief   设置字幕在画布中离下边缘的距离.
 * \details 1. 播放器初始化之后调用,对所有字幕有效,下一条字幕显示生效.
 *
 * \param   yb [in]  下边缘距离参数,单位像素点.
 * \retval   0       成功.
 * \retval  -1       失败.
 */
int GxSubtitle_SetDisplayDefaultYB(unsigned int yb);

/**
 * \brief   调整字幕在画布上下位置.
 * \details 1. 播放器初始化之后调用,对所有字幕有效,下一条字幕显示生效.
 *          2. 目前只对dvb字幕生效.
 *
 * \param   y_offset [in]  往下偏移位置,单位像素点.
 * \retval   0       成功.
 * \retval  -1       失败.
 */
int GxSubtitle_SetDisplayYOffset(int y_offset);

/**
 * \brief   配置 ATSC-CC 绘图模式.
 * \details 1. 播放器初始化之后调用,只对 ATSC-CC 字幕有效,下一次加载字幕生效.
 * \details 2. ASTC-CC 字幕以位图模式绘图,还是以文本模式绘图.
 * \details 3.   位图模式：使用 VBI 字符库,字体效果较差.
 * \details 4.   文本模式：可以支持矢量字库等.
 *
 * \param   mode  [in]  ATSC-CC 显示模式参数,详见: gxdocref GX_SUBTITLE_ATSCCC_MODE.
 * \retval   0          成功.
 * \retval  -1          失败.
 **/
int GxSubtitle_ConfigAtscccMode(GX_SUBTITLE_ATSCCC_MODE mode);

/**
 * \brief   设置 DVB 字幕调色板.
 * \details 1. 播放器初始化之后调用,只对 DVB 字幕有效,下一条字幕显示生效.
 *
 * \param   clut [in] 调色板参数,详见: gxdocref GX_SUBTITLE_DVB_CLUT.
 * \retval   0        成功.
 * \retval  -1        失败.
 */
int GxSubtitle_SetDvbSubtClut  (GX_SUBTITLE_DVB_CLUT *clut);

#endif

/** @}*/
