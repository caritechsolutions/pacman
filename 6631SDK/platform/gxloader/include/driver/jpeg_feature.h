#ifndef __JPEG_FEATURE_H__
#define __JPEG_FEATURE_H__

/**
 *
 * \brief JPEG 模块上报解码中断状态
 */
typedef enum _GX_JPEG_INT_BITS {
	GX_JPEG_INT_DEC_PIC_FINISH = 0,      ///< 整图解码完成
	GX_JPEG_INT_DEC_SLICE_FINISH,        ///< Progressiv（渐进式）图片，一帧解码完成
	GX_JPEG_INT_DEC_SOF_FINISH,          ///< 图片 SOF 头解码完成
	GX_JPEG_INT_FRAME_BUF_ERROR,         ///< 解码目标地址错误
	GX_JPEG_INT_CLIP_ERROR,              ///< 解码剪切举行错误
	GX_JPEG_INT_BS_BUF_ERROR,            ///< 解码输入数据地址错误
	GX_JPEG_INT_DEC_ERROR,               ///< 解码错误
	GX_JPEG_INT_LINE_CNT_GATE,           ///< 解码到达行计数上限
	GX_JPEG_INT_BS_BUFFER_ALMOST_EMPTY,  ///< 解码输入数据到达快空门限
	GX_JPEG_INT_BS_BUFFER_EMPTY,         ///< 解码输入数据到达空门限
	GX_JPEG_INT_MAX,                     ///< 解码中断上限，此枚举不可作为状态使用
} GX_JPEG_INT_BITS_e;

/**
 *
 * \brief JPEG 解码后数据重采样方式
 */
typedef enum _GX_JPEG_RESAMPLE_MODE {
	GX_JPEG_RESAMPLE_DEP_ON_RATIO,            ///< 按比例重采样
	GX_JPEG_RESAMPLE_ZOOM,                    ///< 缩放重采样
	GX_JPEG_RESAMPLE_DEP_ON_FRAME_BUFFER,     ///< 按目标宽高重采样
	GX_JPEG_NO_RESAMPLE,                      ///< 无重采样
	GX_JPEG_RESAMPLE_MAX,                     ///< 重采样上限，此枚举不可作重采样方式参数使用
} GX_JPEG_RESAMPLE_MODE_e;

/**
 *
 * /brief JPEG 数据大小端排列方式，按原数据为0x0123456789ABCDEF进行
 */
typedef enum _GX_JPEG_ENDIAN_FORMAT {
	GX_JPEG_ENDIAN_0123456789ABCDEF,        ///< 按 0x0123456789ABCDEF 排列
	GX_JPEG_ENDIAN_FEDCBA9876543210,        ///< 按 0xFEDCBA9876543210 排列
	GX_JPEG_ENDIAN_89ABCDEF01234567,        ///< 按 0x89ABCDEF01234567 排列
	GX_JPEG_ENDIAN_76543210FEDCBA98,        ///< 按 0x76543210FEDCBA98 排列
} GX_JPEG_ENDIAN_FORMAT_e;

/**
 *
 * /brief JPEG 解码后数据格式
 */
typedef enum _GX_JPEG_DECODE_FORMAT {
	GX_JPEG_DECODE_YUV420,           ///< YUV 4:2:0
	GX_JPEG_DECODE_YUV422,           ///< YUV 4:2:2
	GX_JPEG_DECODE_VUY224,           ///< VUY 2:2:4
	GX_JPEG_DECODE_YUV444,           ///< YUV 4:4:4
	GX_JPEG_DECODE_YUV400,           ///< YUV 4:0:0
} GX_JPEG_DECODE_FORMAT_e;

/**
 *
 * /brief JPEG 存储格式
 */
typedef enum _GX_JPEG_DECODE_PROFILE {
	GX_JPEG_DECODE_BASELINE,              ///< 基线存储
	GX_JPEG_DECODE_PROGRESSIVE,           ///< 渐进式存储
} GX_JPEG_DECODE_PROFILE_e;

/**
 *
 * JPEG 模块按基地址安装
 * \param addr JPEG模块基地址，地址为映射后的虚拟地址
 * \return 无
 *
 */
void jpeg_install(unsigned int addr);

/**
 *
 * JPEG 模块卸载
 * \param 无
 * \return 无
 *
 */
void jpeg_uninstall(void);

/**
 *
 * JPEG 中断使能
 * \param bit      中断参数
 * \param en       开关，0为关闭，非0为开启
 * \return 无
 *
 */
void jpeg_int_en(GX_JPEG_INT_BITS_e bit, int en);

/**
 *
 * JPEG 清中断
 * \param bit      中断参数
 * \return 无
 *
 */
void jpeg_int_clr(GX_JPEG_INT_BITS_e bit);

/**
 *
 * JPEG 获取中断是能
 * \param bit      中断参数
 * \return 是否是能
 * \retval 0 未开启
 * \retval 非0 开启
 *
 */
int  jpeg_int_en_status(GX_JPEG_INT_BITS_e bit);

/**
 *
 * JPEG 获取当前中断状态
 * \param  无
 * \return 中断状态
 *
 */
int  jpeg_int_status(void);

/**
 *
 * JPEG 设置输入数据大小端
 * \param  endian； gxdocref GX_JPEG_ENDIAN_FORMAT_e
 * \return 无
 *
 */
void jpeg_set_bs_buf_endian(GX_JPEG_ENDIAN_FORMAT_e endian);

/**
 *
 * JPEG 设置目标数据大小端
 * \param  endian； gxdocref GX_JPEG_ENDIAN_FORMAT_e
 * \return 无
 *
 */
void jpeg_set_frame_buf_endian(GX_JPEG_ENDIAN_FORMAT_e endian);

/**
 *
 * JPEG 设置重采样模式
 * \param  mode； gxdocref GX_JPEG_RESAMPLE_MODE_e
 * \return 无
 *
 */
void jpeg_set_resample_mode(GX_JPEG_RESAMPLE_MODE_e mode);

/**
 *
 * JPEG 启动/停止 JPEG 解码
 * \param  en 0 表示停止解码，非 0 表示启动解码
 * \return 无
 *
 */
void jpeg_set_start(int en);

/**
 *
 * JPEG 解码结束
 * \param  en 0 表示解码未结束，非 0 表示解码结束
 * \return 无
 *
 */
void jpeg_set_end(int en);

/**
 *
 * JPEG 设置开启/关闭所有中断
 * \param  en 0 表示关闭所有中断，非 0 表示开启所有中断
 * \return 无
 *
 */
void jpeg_set_interrupt_enable(int en);

/**
 *
 * 获取当前 JPEG 解码器是否工作
 * \param 无
 * \return
 * \retval 0 空闲，未工作
 * \retval 1 忙，解码器在工作
 *
 */
int  jpeg_get_busy(void);

/**
 *
 * 设置 JPEG 解码超时门限
 * \param time 超时门限值
 * \return 无
 *
 */
void jpeg_set_time_gate(int time);

/**
 *
 * 设置 JPEG 解码输入数据大小
 * \param size 输入数据大小
 * \return 无
 *
 */
void jpeg_set_buf_size(int size);

/**
 *
 * 获取 JPEG 解码输入数据大小
 * \param 无
 * \return 输入数据大小
 *
 */
int  jpeg_get_buf_size(void);

/**
 *
 * 设置 JPEG 解码写指针
 * \param ptr 写指针位置
 * \return 无
 *
 */
void jpeg_set_wr_ptr(int ptr);

/**
 *
 * 获取 JPEG 解码写指针
 * \param 无
 * \return 写指针位置
 *
 */
int  jpeg_get_wr_ptr(void);

/**
 *
 * 获取 JPEG 解码读指针
 * \param 无
 * \return 读指针位置
 *
 */
int  jpeg_get_rd_ptr(void);

/**
 *
 * 获取 JPEG 解码目标数据颜色格式
 * \param 无
 * \return 颜色格式； gxdocref GX_JPEG_DECODE_FORMAT_e
 *
 */
GX_JPEG_DECODE_FORMAT_e jpeg_get_format(void);

/**
 *
 * 获取 JPEG 存储格式
 * \param 无
 * \return 存储格式； gxdocref GX_JPEG_DECODE_FORMAT_e
 *
 */
GX_JPEG_DECODE_PROFILE_e jpeg_get_profile(void);

/**
 *
 * 获取 JPEG IDCT 值
 * \param 无
 * \return 当前 JPEG 的 IDCT 值
 *
 */
int  jpeg_get_idct(void);

/**
 *
 * 获取 JPEG 缩放参数
 * \param hzoom 为水平缩放参数指针，vzoom 为垂直缩放参数指针
 * \return 无
 *
 */
void jpeg_get_zoom(unsigned int *hzoom, unsigned int *vzoom);

/**
 *
 * 获取 JPEG 缩放参数
 * \param hzoom 为水平缩放参数指针，vzoom 为垂直缩放参数指针
 * \return 无
 *
 */
void jpeg_get_original_size(unsigned int *width, unsigned int *height);

/**
 *
 * 获取 JPEG 解码回写宽高
 * \param width 为回写宽度，height 为回写高度
 * \return 无
 *
 */
void jpeg_get_wb_size(unsigned int *width, unsigned int *height);

/**
 *
 * 获取 JPEG 解码显示宽高
 * \param width 为显示宽度，height 为显示高度
 * \return 无
 *
 */
void jpeg_get_display_size(unsigned int *width, unsigned int *height);

/**
 *
 * 设置 JPEG 实际坐标位置
 * \param x 为水平坐标，y 为垂直坐标
 * \return 无
 *
 */
void jpeg_set_coordinate(unsigned int x, unsigned int y);

/**
 *
 * 获取 JPEG 实际坐标位置
 * \param x 为水平坐标，y 为垂直坐标
 * \return 无
 *
 */
void jpeg_get_coordinate(unsigned int *x, unsigned int *y);

/**
 *
 * 设置 JPEG 解码图片左上位置
 * \param x 水平位置，y 垂直位置
 * \return 无
 *
 */
void jpeg_set_upper_left(unsigned int x, unsigned int y);

/**
 *
 * 设置 JPEG 解码图片右下位置
 * \param x 水平位置，y 垂直位置
 * \return 无
 *
 */
void jpeg_set_lower_right(unsigned int x, unsigned int y);

/**
 *
 * 设置 JPEG 继续解码
 * \param en 0 为关闭继续解码， 非0 为开启继续解码
 * \return 无
 *
 */
void jpeg_set_goon(int en);

/**
 *
 * 获取 JPEG 继续解码
 * \param 无
 * \return en 0 为关闭继续解码， 非0 为开启继续解码
 *
 */
int  jpeg_get_goon(void);

/**
 *
 * 设置 JPEG 最大解码宽高
 * \param width 最大解码宽度，height 最大解码高度
 * \return 无
 *
 */
void jpeg_set_max_size(unsigned int width, unsigned int height);

/**
 *
 * 设置 JPEG 解码基宽
 * \param stride 基宽值
 * \return 无
 *
 */
void jpeg_set_stride(unsigned int stride);

/**
 *
 * 设置 JPEG 输入解码地址
 * \param addr 地址，size 大小
 * \return 无
 *
 */
void jpeg_set_bs_buf(int addr, int size);

/**
 *
 * 设置 JPEG 空门限
 * \param size 门限值
 * \return 无
 *
 */
void jpeg_set_empty_th(int size);

/**
 *
 * 获取 JPEG 空门限
 * \param 无
 * \return 门限值
 *
 */
int  jpeg_get_empty_th(void);

/**
 *
 * 设置 JPEG 快空门限
 * \param size 门限值
 * \return 无
 *
 */
void jpeg_set_almostempty_th(int size);

/**
 *
 * 获取 JPEG 快空门限
 * \param 无
 * \return 门限值
 *
 */
int  jpeg_get_almostempty_th(void);

/**
 *
 * 设置 JPEG 剪切模式使能/关闭
 * \param en 0为关闭，非0 为开启使能
 * \return 无
 *
 */
void jpeg_set_clip_mode(int en);

/**
 *
 * 获取 JPEG 剪切模式使能/关闭
 * \param 无
 * \return 0为关闭，非0 为开启使能
 *
 */
int  jpeg_get_clip_mode(void);

/**
 *
 * 设置 JPEG 解码只解 SOF 头后即暂停模式
 * \param en 0为关闭，非0 为开启使能
 * \return 无
 *
 */
void jpeg_set_sof_pause(int en);

/**
 *
 * 获取 JPEG 解码只解 SOF 头后即暂停模式
 * \param 无
 * \return 0为关闭，非0 为开启使能
 *
 */
int  jpeg_get_sof_pause(void);

/**
 *
 * 设置渐进式(Progressiv) JPEG 存储方式的图片单帧暂停模式； gxdocref GX_JPEG_DECODE_PROFILE_e
 * \param en 0为关闭，非0 为开启使能
 * \return 无
 *
 */
void jpeg_set_slice_pause(int en);

/**
 *
 * 获取渐进式(Progressiv) JPEG 存储方式的图片单帧暂停模式； gxdocref GX_JPEG_DECODE_PROFILE_e
 * \param 无
 * \return 0为关闭，非0 为开启使能
 *
 */
int  jpeg_get_slice_pause(void);

/**
 *
 * 设置 JPEG 手动水平、垂直比率
 * \param h_ratio 水平比率，h_ratio 垂直比率
 * \return 无
 *
 */
void jpeg_set_manual_ration(int h_ratio, int v_ratio);

/**
 *
 * 设置 JPEG 解码行数
 * \param line 行数
 * \return 无
 *
 */
void jpeg_set_line_cnt(int line);

/**
 *
 * 设置 JPEG 目标地址
 * \param y_addr Y 分量地址，cb_addr CB 分量地址，cr_addr CR 分量地址
 * \return 无
 *
 */
void jpeg_set_framebuffer(int y_addr, int cb_addr, int cr_addr);

/**
 *
 * 获取渐进式(Progressiv) JPEG 帧总数
 * \param 无
 * \return 帧总数
 *
 */
int  jpeg_get_wb_layer_cnt(void);

#endif /*__JPEG_FEATURE_H__*/

