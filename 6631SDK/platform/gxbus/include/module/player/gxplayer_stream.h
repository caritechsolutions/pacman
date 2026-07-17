/** \addtogroup gxplayer_stream
 **  @{
 **/
#ifndef __GXPLAYER_STREAM_H__
#define __GXPLAYER_STREAM_H__

#include "gxplayer_module.h"

__BEGIN_DECLS
/**
 * stream 选项类型
 **/
typedef enum {
	GX_STREAM_OPTION_DRM        = 0x0,  ///< 是否开启DRM.
	GX_STREAM_OPTION_USER_AGENT = 0xf0, ///< 链接的用户代理.
	GX_STREAM_OPTION_CONECTTION = 0xf1  ///< 链接的链接类型.
} GxStreamOptionType;

/**
 * stream 加解密参数
 **/
typedef struct {
	const char    *keyUrl;   ///< 密钥 URL 的链接.
	const char    *dataUrl;  ///< 数据 URL 的链接.
	unsigned char *keyIV;    ///< 密钥初始向量地址.
	unsigned int  keyIVSize; ///< 密钥初始向量长度.
	unsigned char *dataIn;   ///< 输入数据地址.
	unsigned int  dataInLen; ///< 输入数据长度.
	unsigned char *dataOut;  ///< 输出数据地址.
	unsigned int  dataOutLen;///< 输出数据长度.
	int           newTSFlag; ///< 是否是新的一段 TS 流.
	unsigned int  custom_num; ///< 自定义个数.
	char          *custom_flag; ///< 自定义的标识符.
	char          **custom_str; ///< 自定义内容.
} GxStreamOptionCallbackParam;

/**
 * stream 回调参数
 **/
typedef int (*GxStreamOptionCallback) (GxStreamOptionCallbackParam *param);

/**
 * mpegts 加解密参数
 **/
typedef struct {
	unsigned char *dataIn;   ///< 输入数据地址.
	unsigned int  dataInLen; ///< 输入数据长度.
	unsigned char *dataOut;  ///< 输出数据地址.
	unsigned int  dataOutLen;///< 输出数据长度.
} GxStreamMpegtsCallbackParam;

/**
 * stream 回调参数
 **/
typedef int (*GxStreamMpegtsCallback) (GxStreamMpegtsCallbackParam *param);

/**
 * 外部网络时间 回调参数
 **/
typedef uint64_t (*GxExternalNetTimeCallback)(void);

/**
 * \brief   配置 stream 选项.
 * \details 1. 配置 stream 的选项后,需要通过清除 stream 的选项来释放资源.
 *
 * \param   type    [in]  stream 选项类型参数,详见: gxdocref GxStreamOptionType.
 * \param   strings [in]  stream 选项类型对应的字符串参数.
 * \retval   0            成功.
 * \retval  -1            失败.
 **/
extern int GxStream_ConfigOptions(GxStreamOptionType type, const char *strings);

/**
 * \brief   清除 stream 选项.
 * \details 1. 清除 stream 选项后,会释放配置时申请的资源.
 *
 * \param   void
 * \retval   0     成功.
 * \retval  -1     失败.
 **/
extern int GxStream_ClearOptions (void);

/**
 * \brief   注册 stream 回调.
 * \details 1. 特定应用场景: DRM.
 *
 * \param   cb [in] stream 回调参数,详见: gxdocref GxStreamOptionCallback.
 * \retval   0      成功.
 * \retval  -1      失败.
 **/
extern int GxStream_RegisterCallback(GxStreamOptionCallback cb);

/**
 * \brief   注册 外部网络时间 回调.
 * \details 1. 特定应用场景: IPTV.
 *
 * \param   cb [in] stream 回调参数,详见: gxdocref GxStreamOptionCallback.
 * \retval   0      成功.
 * \retval  -1      失败.
 **/
extern int GxExternalNetTime_RegisterCallback(GxExternalNetTimeCallback cb);

/**
 * \brief   清除 外部时间回调函数 选项.
 * \details 1. 清除 外部时间回调函数.
 *
 * \param   void
 * \retval   0     成功.
 * \retval  -1     失败.
 **/
extern int GxExternalNetTime_UnregisterCallback(void);

/**
 * \brief   注册 stream mpegts 回调.
 * \details 1. 特定应用场景: 部分数据解码(客户私有方式).
 *
 * \param   cb [in] stream 回调参数,详见: gxdocref GxStreamMpegtsCallback.
 * \retval   0      成功.
 * \retval  -1      失败.
 **/
extern int GxStream_MpegtsRegisterCallback(GxStreamMpegtsCallback cb);

/**
 * \brief   注销掉 stream mpegts 回调.
 * \details 1. 特定应用场景: (客户私有方式).
 *
 **/
extern int GxStream_MpegtsUnregisterCallback(void);

/**
 * stream 的 TS-BUFF 包数据参数
 **/
typedef struct {
	unsigned char* data; ///< 数据包地址.
	unsigned int   size; ///< 数据包大小.
	unsigned int   pts;  ///< 数据包 PTS.
	void*          priv; ///< 数据包私有句柄.
} GxPlayerDataPacket;

/**
 * stream 的 TS-BUFF 初始化回调, 参数: blksize - 块大小; delayms - 延时参数.
 **/
typedef handle_t (*PLAYER_STREAM_PORT_INIT_CBK)(size_t blksize, unsigned int delayms);

/**
 * stream 的 TS-BUFF 取消初始化回调, 参数: handle - 初始化句柄.
 **/
typedef void     (*PLAYER_STREAM_PORT_UNINIT_CBK)(handle_t handle);

/**
 * stream 的 TS-BUFF 退出回调, 参数: handle - 初始化句柄.
 **/
typedef void     (*PLAYER_STREAM_PORT_EXIT_CBK)(handle_t handle);

/**
 * stream 的 TS-BUFF 复位回调, 参数: handle - 初始化句柄.
 **/
typedef void     (*PLAYER_STREAM_PORT_RESET_CBK)(handle_t handle);

/**
 * stream 的 TS-BUFF 申请数据包回调, 参数: handle - 初始化句柄; pkt - 返回数据包信息; size - 申请数据包大小.
 **/
typedef int      (*PLAYER_STREAM_PORT_NEWPKT_CBK)(handle_t handle, GxPlayerDataPacket* pkt, ssize_t size);

/**
 * stream 的 TS-BUFF 释放数据包回调, 参数: handle - 初始化句柄; pkt - 输入数据包信息.
 **/
typedef void     (*PLAYER_STREAM_PORT_DELPKT_CBK)(handle_t handle, GxPlayerDataPacket* pkt);

/**
 * stream 的 TS-BUFF 获取数据包回调, 参数: handle - 初始化句柄; pkt - 输入数据包信息.
 **/
typedef int      (*PLAYER_STREAM_PORT_GETPKT_CBK)(handle_t handle, GxPlayerDataPacket* pkt);

/**
 * stream 的 TS-BUFF 输入数据包回调, 参数: handle - 初始化句柄; pkt - 输入数据包信息.
 **/
typedef int      (*PLAYER_STREAM_PORT_PUTPKT_CBK)(handle_t handle, GxPlayerDataPacket* pkt);


/**
 * stream 的 TS-BUFF 回调参数.
 **/
typedef struct {
	PLAYER_STREAM_PORT_INIT_CBK    Init;       ///< 初始化回调函数.
	PLAYER_STREAM_PORT_UNINIT_CBK  Uninit;     ///< 取消初始化回调函数
	PLAYER_STREAM_PORT_EXIT_CBK    Exit;       ///< 退出回调函数.
	PLAYER_STREAM_PORT_RESET_CBK   Reset;      ///< 复位回调函数.
	PLAYER_STREAM_PORT_NEWPKT_CBK  NewPacket;  ///< 申请数据包回调函数.
	PLAYER_STREAM_PORT_DELPKT_CBK  DelPacket;  ///< 释放数据包回调函数.
	PLAYER_STREAM_PORT_GETPKT_CBK  GetPacket;  ///< 获取数据包回调函数.
	PLAYER_STREAM_PORT_PUTPKT_CBK  PutPacket;  ///< 推入数据包回调函数.
} PlayerStreamPortCallback;

/**
 * /brief   设置 stream 的 TS-BUFF 回调参数.
 * /details 1. 使用场景: 应用需要控制 TS-BUFF 的输入/输出.
 *
 * /param   cbk [in]  回调参数,详见: gxdocref PlayerStreamPortCallback.
 * /retval  void.
 **/
extern void GxPlayer_SetStreamCallback(PlayerStreamPortCallback *cbk);

/**
 * stream 交互命令
 */
typedef enum {
	GX_PLAYER_STREAM_GET_TOTLE_SIZE, ///< 文件大小.
	GX_PLAYER_STREAM_GET_DURATION,   ///< 文件时长.
	GX_PLAYER_STREAM_GET_BITRATE,    ///< 文件比特率，单位bps.
	GX_PLAYER_STREAM_GET_CURPOS,     ///< 文件读写位置.
} GxPlayerStreamCmd;

/**
 * stream 打开模式
 **/
typedef enum {
	GX_PLAYER_STREAM_RONLY  = 0,   ///< stream 只读模式.
	GX_PLAYER_STREAM_WONLY  = 1,   ///< stream 只写模式.
	GX_PLAYER_STREAM_TRUNC  = 2,   ///< stream 以读写方式打开,清空文件全部内容.
	GX_PLAYER_STREAM_APPEND = 3,   ///< stream 以读写方式打开,写操作写入文件的末尾.
} GxPlayerStreamMode;

/**
 * stream 交互命令 GX_PLAYER_STREAM_GET_TOTLE_SIZE 的参数
 */
typedef off_t   GxPlayerStreamCmdTotleSize;

/**
 * stream 交互命令 GX_PLAYER_STREAM_GET_DURATION 的参数
 */
typedef uint64_t GxPlayerStreamCmdDuration;

/**
 * stream 交互命令 GX_PLAYER_STREAM_GET_BITRATE 的参数
 */
typedef uint32_t GxPlayerStreamCmdBitRate;

/**
 * stream 交互命令 GX_PLAYER_STREAM_GET_CURPOS 的参数
 */
typedef off_t   GxPlayerStreamCmdCurPos;

/**
 * \brief   打开一个 stream
 * \details 1. stream 支持 file / udp / rtp / http / rtsp /rtmp / mpegdash 等交互协议.
 * \details 2. 需要通过 GxPlayer_ModuleRegisterStreamXXX 注册需要的协议才能使用.
 * \details 2. 例如: GxPlayer_ModuleRegisterStreamHTTP 以后,才能使用获取 HTTP 的数据.
 *
 * \param   url  [in]  URL 参数,详见: gxdocref.
 * \param   mode [in]  打开模式,详见: gxdocref GxPlayerStreamMode.
 * \retval  非NULL     成功.
 * \retval    NULL     失败.
 */
extern void *GxPlayer_StreamOpen(const char *ur, GxPlayerStreamMode mode);

/**
 * \brief  关闭一个已打开的 stream
 *
 * \param   handle [in]  句柄, GxPlayer_StreamOpen 返回值.
 * \retval   0           成功.
 * \retval  -1           失败.
 */
extern int GxPlayer_StreamClose(void *handle);

/**
 * \brief   从打开的 stream 读取数据
 *
 * \param   handle [in]  句柄, GxPlayer_StreamOpen 返回值.
 * \param   buffer [in]  读取数据 buffer 指针.
 * \param   size   [in]  读取数据 buffer 大小.
 * \retval  >=0          返回读取的数据长度.
 * \retval  <0           失败.
 */
extern size_t GxPlayer_StreamRead   (void *handle, uint8_t *buffer, size_t size);

/**
 * \brief   数据写入到打开的 stream
 *
 * \param   handle [in]  句柄, GxPlayer_StreamOpen 返回值.
 * \param   buffer [in]  写入数据 buffer 指针.
 * \param   size   [in]  写入数据 buffer 大小.
 * \retval  >=0          返回写入的数据长度.
 * \retval  <0           失败.
 */
extern size_t GxPlayer_StreamWrite  (void *handle, uint8_t *buffer, size_t size);

/**
 * \brief   跳转 stream 读写位置.
 *
 * \param   handle [in]  句柄, GxPlayer_StreamOpen 返回值.
 * \param   pos    [in]  位置.
 * \retval  >=0          返回跳转到的位置.
 * \retval  <0           失败.
 */
extern off_t GxPlayer_StreamSeek   (void *handle, off_t pos);

/**
 * \brief   stream 交互操作
 * \details 例如: 获取 stream 大小/时长等.
 *
 * \param   handle [in]  句柄, GxPlayer_StreamOpen 返回值.
 * \param   cmd    [in]  位置.
 * \param   arg    [in]  参数指针，不同 cmd 对应不同结构参数.
 * \retval   0           成功.
 * \retval  -1           失败.
 */
extern int GxPlayer_StreamControl(void *handle, GxPlayerStreamCmd cmd, void *arg);

__END_DECLS

#endif
/** @}*/
