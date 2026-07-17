/** \addtogroup gxsubtitle
 *  @{
 */

#ifndef __GX_SUBTITLE_MAGAZINE_H__
#define __GX_SUBTITLE_MAGAZINE_H__

#define MAX_PAGE_LANG_COUNT (32)
/**
 * 透明度参数
 */
typedef enum {
	GX_SUBTITLE_MAG_DEFAULT_OPACITY = 0, ///< 默认背景色的透明度.
	GX_SUBTITLE_MAG_FULL_OPACITY    = 1, ///< 不透明.
	GX_SUBTITLE_MAG_WELL_OPACITY    = 2, ///< 1/4透明.
	GX_SUBTITLE_MAG_SEMI_OPACITY    = 3, ///< 半透明.
	GX_SUBTITLE_MAG_NONE_OPACITY    = 4, ///< 全透明.
} GX_SUBTITLE_MAG_OPACITY;

typedef struct {
	int pgno;                            ///< Teletext-Magazine 字幕页号
	int subno;                           ///< Teletext-Magazine 字幕子页号
	int row;                             ///< Teletext-Magazine 字幕行号, -1: 为整行
	int column;                          ///< Teletext-Magazine 字幕列号，-1: 为列行
	unsigned int unicode;                ///< Teletext-Magazine 字幕替换的字符
} GX_SUBTITLE_VBI_DCHAR;

typedef struct {
	unsigned int  char_set;              ///< Teletext-Magazine 字幕字符库
	unsigned int  sub_set;               ///< Teletext-Magazine 字幕语言
	unsigned int  char_val;              ///< Teletext-Magazine 字幕字符
	unsigned char char_pic[20];          ///< Teletext-Magazine 字幕字符打点数组
} GX_SUBTITLE_VBI_MCHAR;

typedef struct {
	int count;
	struct {
		int16_t  comp_page_id;           ///< Teletext-Magazine 字幕页号高位
		int16_t  anci_page_id;           ///< Teletext-Magazine 字幕页号低位
		char     pglang[4];              ///< Teletext-Magazine 字幕语言
	}page[MAX_PAGE_LANG_COUNT];
} GX_SUBTITLE_PGLANG;

/**
 * \brief   跳转到首页.
 * \details 1. GxPlayer_MediaSubLoad 加载 Teletext-Magazine 字幕后,调用接口有效.
 * \details 2. 首页号通过 DVB 搜台获得, GxPlayer_MediaSubLoad 的 pid.major、pis.minor 参数传给播放器.
 * \details 3. 例如首页为 P100 时，输入参数 pid.major = 0x1, pis.minor = 0x00.
 *
 * \param   void.
 * \retval   0   成功.
 * \retval  -1   失败.
 */
extern int GxSubtitle_MagFirstPage(void);

/**
 * \brief   跳转到指定页面.
 *
 * \param   pgno [in]  页面号,取值范围: 100 ~ 899.
 * \retval   0         成功.
 * \retval  -1         失败.
 */
extern int GxSubtitle_MagJumpPage(unsigned int pgno);

/**
 * \brief   获取mag页信息.
 *
 * \param   pginfo [in]  页面信息,最大支持32个页面信息.
 * \retval   0           成功.
 * \retval  -1           失败.
 */
extern int GxSubtitle_MagSetPageLang(GX_SUBTITLE_PGLANG *pglang);

/**
 * \brief   自动跳转到下一页.
 * \details 1. 下一页是当前页面在保存页面中的下一页,保存页面实时更新,更新页面策略详见: gxdocref.
 * \details 2. 例如：当前显示页面 P100, 保存页面有 P102, 没有 P101, 那么下一页为 P102.
 *
 * \param   void.
 * \retval   0   成功.
 * \retval  -1   失败.
 */
extern int GxSubtitle_MagNextPage(void);

/**
 * \brief   自动跳转到上一页.
 * \details 1. 上一页是当前页面在保存页面的上一页,保存页面实时更新,更新页面策略详见: gxdocref.
 * \details 2. 例如: 当前显示页面 P102, 保存页面有 P100, 没有 P101, 那么上一页为 P100.
 *
 * \param   void.
 * \retval   0   成功.
 * \retval  -1   失败.
 */
extern int GxSubtitle_MagPrevPage(void);

/**
 * \brief   自动跳转当前页的下一个子页.
 * \details 1. 一个页面包含很多子页,通过接口切换到保存子页的下一子页,保存子页实时更新,更新子页面策略详见: gxdocref.
 *
 * \param   void.
 * \retval   0   成功.
 * \retval  -1   失败.
 */
extern int GxSubtitle_MagNextSubPage(void);

/**
 * \brief   自动跳转当前页的上一个子页.
 * \details 1. 一个页面包含很多子页,通过接口切换到保存子页的上一子页,保存子页实时更新,更新子页面策略详见: gxdocref.
 *
 * \param   void.
 * \retval   0   成功.
 * \retval  -1   失败.
 */
 extern int GxSubtitle_MagPrevSubPage(void);

/**
 * \brief   自定义区域显示用户指定内容.
 * \details 1. 自定义区域为页面第一行的前 7 个字符.
 * \details 2. 使用场景: 按键输入 100，需要有显示 “P1”, “P10”, “P100” 的过程.
 *
 * \param   str [in] 输入字符串参数.
 * \retval   0       成功.
 * \retval  -1       失败.
 */
extern int GxSubtitle_MagShowString(char str[7]);

/**
 * \brief   获取首页号.
 *
 * \param   void.
 * \retval  100~899     成功.
 * \retval  其他        失败.
 */
extern unsigned int GxSubtitle_MagGetFirstPgNo(void);

/**
 * \brief   获取当前页号.
 *
 * \param   void.
 * \retval  100~899     成功.
 * \retval  其他        失败.
 */
extern unsigned int GxSubtitle_MagGetCurrentPgNo(void);

/**
 * \brief  获取当前页面的子页号.
 *
 * \param  void.
 * \retval 0~14      成功.
 * \retval 其他      失败.
 */
extern unsigned int GxSubtitle_MagGetCurrentSubPgNo(void);

/**
 * \brief  获取当前页面的下一页号.
 *
 * \param  void.
 * \retval 0~14      成功.
 * \retval 其他      失败.
 */
extern unsigned int GxSubtitle_MagGetNextPgNo(void);

/**
 * \brief  获取当前页面的上一页号.
 *
 * \param  void.
 * \retval 0~14      成功.
 * \retval 其他      失败.
 */
extern unsigned int GxSubtitle_MagGetPrevPgNo(void);

/**
 * \brief   获取链接页面.
 * \details 1. 使用场景：红\黄\蓝\绿按键,通过链接页面快速跳转.
 *
 * \param   pgno [out] 链接页面参数,最大支持 6 个链接页面,有效范围 100 ~ 899.
 * \retval   0         成功.
 * \retval  -1         失败.
 */
extern int GxSubtitle_MagGetSixLinkPgNo(unsigned int pgno[6]);

/**
 * \brief   设置页面透明度.
 * \details 1. 页面透明度设置立即生效.
 *
 * \param   opacity [in] 透明度参数,详见: gxdocref GX_SUBTITLE_MAG_OPACITY.
 * \retval   0           成功.
 * \retval  -1           失败.
 */
extern int GxSubtitle_MagSetOpacity(GX_SUBTITLE_MAG_OPACITY opacity);

/**
 * \brief   调试页面字符.
 *
 * \param   dchar    [in] 字符位置,详见: gxdocref GX_SUBTITLE_VBI_DCHAR
 * \retval   0            成功.
 * \retval  -1            失败.
 */
extern int GxSubtitle_VBIDebugChar(GX_SUBTITLE_VBI_DCHAR dchar);

/**
 * \brief   修改页面字符.
 *
 * \param   mchar    [in] 字符位置,详见: gxdocref GX_SUBTITLE_MAG_MCHAR
 * \retval   0            成功.
 * \retval  -1            失败.
 */
extern int GxSubtitle_VBIModifyChar(GX_SUBTITLE_VBI_MCHAR mchar);

/**
 * \brief   替换捷克语的字符，要替换的字符为历史问题中整理的字符集合.
 *
 * \param   无
 * \retval   0            成功.
 * \retval  -1            失败.
 */
extern int GxSubtitle_CZEModifyChar(void);
#endif

/** @}*/
