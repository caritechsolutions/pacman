/** \addtogroup  gxplayer_url
 *  @{
 */
#ifndef __GXPLAYER_URL_H__
#define __GXPLAYER_URL_H__

#include "gxcore.h"

__BEGIN_DECLS

#define GX_URL_MAX_LEN         (2048)                      ///< URL 最大字节数.
#define GX_URL_KEY_FRE         "fre"                       ///< URL 频点字段标识.
#define GX_URL_KEY_SYM         "symbol"                    ///< URL 符号率字段标识.
#define GX_URL_KEY_QAM         "qam"                       ///< URL 调制模式字段标识.
#define GX_URL_KEY_TUNER       "tuner"                     ///< URL tuner 字段标识.
#define GX_URL_KEY_TSID        "tsid"                      ///< URL TS ID 字段标识.
#define GX_URL_KEY_DMXID       "dmxid"                     ///< URL 解复用 ID 字段标识.
#define GX_URL_KEY_POLAR       "polar"                     ///< URL polar 字段标识,取值范围:
														   ///< - gxdocref GX_URL_VAL_POLAR_V.
														   ///< - gxdocref GX_URL_VAL_POLAR_H.
#define GX_URL_KEY_BANDW       "bandwidth"                 ///< URL 带宽字段标识,取值范围:
														   ///< - gxdocref GX_URL_VAL_BAND_8M.
														   ///< - gxdocref GX_URL_VAL_BAND_7M.
														   ///< - gxdocref GX_URL_VAL_BAND_6M.
#define GX_URL_KEY_22K         "22k"                       ///< URL 22k 字段标识.
#define GX_URL_KEY_CWMODE      "workmode"                  ///< URL DVB-C / DVB-T 字段标识, 参数废弃,取值范围:
														   ///< - gxdocref GX_URL_VAL_CW_DTMB.
														   ///< - gxdocref GX_URL_VAL_CW_DVBC.
#define GX_URL_KEY_SCRAM       "scramble"                  ///< URL 是否加扰字段标识.
#define GX_URL_KEY_TSBUFF_MS   "tsbuff"                    ///< URL TS-BUFF 延时字段标识.
#define GX_URL_KEY_TSBUFF_DMXID "tsbuff_dmxid"             ///< URL TS-BUFF 后台录制使用的 解复用 ID 字段标识.默认值:3211/Sirius:3,其他芯片:5
#define GX_URL_KEY_TSBUFF_MEMHOLE "tsbuff_memhole_size"    ///< URL TS-BUFF 可用洞内存大小字段标识. 优先使用系统缓存，系统缓存不足时候才使用洞内存.
#define GX_URL_KEY_DMXMODE     "dmxmode"                   ///< URL dmxmode 字段标识, 参数废弃.
#define GX_URL_KEY_TSMODE      "tsmode"                    ///< URL TS 串行/并行字段标识, 参数废弃.
#define GX_URL_KEY_UNICABLE_IF_INDEX    "ifindex"          ///< URL ifindex 字段标识.
#define GX_URL_KEY_UNICABLE_LNB_INDEX   "lnbindex"         ///< URl lnbindex 字段标识.
#define GX_URL_KEY_UNICABLE_CENTRE_FRE  "centrefre"        ///< URL 中心频点字段标识.
#define GX_URL_KEY_UNICABLE_IF_FRE      "iffre"            ///< URL iffre 字段标识.
#define GX_URL_KEY_DATA_PLPID           "data_plp_id"      ///< URL data_plp_id 字段标识.
#define GX_URL_KEY_COMM_PLPID           "common_plp_id"    ///< URL common_plp_id 字段标识.
#define GX_URL_KEY_COMM_PLPID_EXIST     "common_plp_exist" ///< URL common_plp_exist 字段标识.

#define GX_URL_KEY_TSCACHE_MS      "tscache_ms"            ///< URL 快速切台缓存时间字段标识,单位 ms.
#define GX_URL_KEY_TSCACHE_MODE    "tscache_mode"          ///< URL 快速切台申请缓存模式字段标识,取值范围:
														   ///< - gxdocref GX_URL_VAL_TSCACHE_MODE_USER.
														   ///< - gxdocref GX_URL_VAL_TSCACHE_MODE_KERNEL_NORMAL.
														   ///< - gxdocref GX_URL_VAL_TSCACHE_MODE_KERNEL_SHARED.
#define GX_URL_KEY_TSCACHE_DMXID   "tscache_dmxid"         ///< URL 快速切台的解复用 ID 字段标识.
#define GX_URL_KEY_TSCACHE_BUFSIZE "tscache_bufsize"       ///< URL 快速切台的缓存大小 字段标识.

#define GX_URL_KEY_PLS_TSID     "pls_ts_id"                ///< URL pls_ts_id 字段标识.
#define GX_URL_KEY_PLS_CODE     "pls_code"                 ///< URL pls_code 字段标识.
#define GX_URL_KEY_PLS_CHANGE   "pls_change"               ///< URL pls_change 字段标识.
#define GX_URL_KEY_PLS_STATUS   "pls_status"               ///< URL pls_status 字段标识.
#define GX_URL_KEY_PLS_NUM      "pls_num"                  ///< URL pls_num 字段标识.

#define GX_URL_KEY_TP_MODE      "tp_mode"                  ///< URL tp mode参数，用于DVBT/DVBT2参数配置.

#define GX_URL_KEY_IP           "ip"                       ///< URL 网络虚拟前端 IP 字段标识,参数废弃.
#define GX_URL_KEY_PORT         "port"                     ///< URL 网络虚拟前端端口字段标识,参数废弃.

#define GX_URL_KEY_VPID        "vpid"                      ///< URL 视频流 PID 字段标识.
#define GX_URL_KEY_APID        "apid"                      ///< URL 音频流 PID 字段标识.
#define GX_URL_KEY_SPID        "subpid"                    ///< URL 字幕流 PID 字段标识.
#define GX_URL_KEY_PCRPID      "pcrpid"                    ///< URL PCR PID 字段标识.
#define GX_URL_KEY_PMTPID      "pmt"                       ///< URL PMT PID 字段标识.
#define GX_URL_KEY_APID1       "adpid"                     ///< URL 次路音频流 PID 字段标识.
#define GX_URL_KEY_VCODEC      "vcodec"                    ///< URL 视频流编码标准字段标识.
														   ///< - gxdocref GX_URL_VAL_VCODEC_MPG2.
														   ///< - gxdocref GX_URL_VAL_VCODEC_AVS.
														   ///< - gxdocref GX_URL_VAL_VCODEC_H264.
														   ///< - gxdocref GX_URL_VAL_VCODEC_H265.
														   ///< - gxdocref GX_URL_VAL_VCODEC_MPG4.
														   ///< - gxdocref GX_URL_VAL_VCODEC_H263.
														   ///< - gxdocref GX_URL_VAL_VCODEC_REAL.
#define GX_URL_KEY_ACODEC      "acodec"                    ///< URL 音频流编码标准字段标识.
														   ///< - gxdocref GX_URL_VAL_ACODEC_MPG1.
														   ///< - gxdocref GX_URL_VAL_ACODEC_MPG2.
														   ///< - gxdocref GX_URL_VAL_ACODEC_LATM.
														   ///< - gxdocref GX_URL_VAL_ACODEC_AC3.
														   ///< - gxdocref GX_URL_VAL_ACODEC_ATDS.
														   ///< - gxdocref GX_URL_VAL_ACODEC_EAC3.
														   ///< - gxdocref GX_URL_VAL_ACODEC_LPCM.
														   ///< - gxdocref GX_URL_VAL_ACODEC_DTS.
														   ///< - gxdocref GX_URL_VAL_ACODEC_DOLBY_TRUEHD.
														   ///< - gxdocref GX_URL_VAL_ACODEC_AC3_PLUS.
														   ///< - gxdocref GX_URL_VAL_ACODEC_DTS_HD.
														   ///< - gxdocref GX_URL_VAL_ACODEC_DTS_MA.
														   ///< - gxdocref GX_URL_VAL_ACODEC_AC3_PLUS_SEC.
														   ///< - gxdocref GX_URL_VAL_ACODEC_DTS_HD_SEC.
														   ///< - gxdocref GX_URL_VAL_ACODEC_DRA1.
#define GX_URL_KEY_ACODEC1     "adcodec"                   ///< URL 次路音频流编码标准字段标识.
#define GX_URL_KEY_SYNC        "sync"                      ///< URL 恢复 STC 模式字段标识:
														   ///< - gxdocref GX_URL_VAL_SYNC_AVPTS.
														   ///< - gxdocref GX_URL_VAL_SYNC_VPTS.
														   ///< - gxdocref GX_URL_VAL_SYNC_APTS.
														   ///< - gxdocref GX_URL_VAL_SYNC_PCR.
#define GX_URL_KEY_DVBT        "dvbt://"                   ///< URL DVB-T 协议字段标识,包括: DVB-T / DVB-T2.
#define GX_URL_KEY_DVBS        "dvbs://"                   ///< URL DVB-S 协议字段标识,包括: DVB-S / DVB-S2.
#define GX_URL_KEY_DVBC        "dvbc://"                   ///< URL DVB-C 协议字段标识.
#define GX_URL_KEY_DTMB        "dtmb://"                   ///< URL DTMB 协议字段标识.
#define GX_URL_KEY_DVBNET      "dvbnet://"                 ///< URL DVB-NET 协议字段标识.

#define GX_URL_KEY_MEM         "mem://"                    ///< URL MEM 协议字段标识.
#define GX_URL_KEY_ADDR        "addr"                      ///< URL MEM 地址字段标识.
#define GX_URL_KEY_SIZE        "size"                      ///< URL MEM 大小字段标识.
#define GX_URL_KEY_FMT         "fmt"                       ///< URL MEN 数据格式字段标识,取值范围:
#define GX_URL_KEY_HWSAFE      "hwsafe"                    ///< URL MEN 数据是否需要安全硬件解密．
                                                           ///< - gxdocref GX_MEM_FMT_MP3.
														   ///< - gxdocref GX_MEM_FMT_ES.
														   ///< - gxdocref GX_MEM_FMT_PES.
#define GX_MEM_FMT_MP3         1                           ///< URL MEM 数据格式为 MP3.
#define GX_MEM_FMT_ES          2                           ///< URL MEM 数据格式为 ES.
#define GX_MEM_FMT_PES         3                           ///< URL MEM 数据格式为 PES.
#define GX_MEM_FMT_TS          4                           ///< URL MEM 数据格式为 TS.

#define GX_URL_VAL_POLAR_V     0                           ///< URL polar V, 13V
#define GX_URL_VAL_POLAR_H     1                           ///< URL polar H, 18V.

#define GX_URL_VAL_BAND_8M     8000                        ///< URL 8M 带宽.
#define GX_URL_VAL_BAND_7M     7000                        ///< URL 7M 带宽.
#define GX_URL_VAL_BAND_6M     6000                        ///< URL 6M 带宽.

#define GX_URL_VAL_CW_DTMB     0                           ///< URL DTMB 工作模式.
#define GX_URL_VAL_CW_DVBC     1                           ///< URL DVBC 工作模式.

#define GX_URL_VAL_ACODEC_MPG1  0                          ///< MP3 LAYER I 音频编码标准.
#define GX_URL_VAL_ACODEC_MPG2  1                          ///< MP3 LAYER II/III 音频编码标准.
#define GX_URL_VAL_ACODEC_LATM  2                          ///< MPEG4-AAC LATM格式的音频编码标准.
#define GX_URL_VAL_ACODEC_AC3   3                          ///< DOLBY 音频编码标准.
#define GX_URL_VAL_ACODEC_ATDS  4                          ///< MPEG4-AAC ATDS格式的音频编码标准.
#define GX_URL_VAL_ACODEC_EAC3  5                          ///< DOLBY-PLUS 音频编码标准.
#define GX_URL_VAL_ACODEC_LPCM  6                          ///< LINE-PCM 音频编码标准.
#define GX_URL_VAL_ACODEC_DTS   7                          ///< DTS 音频编码标准.
#define GX_URL_VAL_ACODEC_DOLBY_TRUEHD 8                   ///< DOLBY-PLUS 音频编码标准.
#define GX_URL_VAL_ACODEC_AC3_PLUS 9                       ///< DOLBY-PLUS 音频编码标准.
#define GX_URL_VAL_ACODEC_DTS_HD   10                      ///< DTS-HD 音频编码标准.
#define GX_URL_VAL_ACODEC_DTS_MA   11                      ///< DTS-HD 音频编码标准.
#define GX_URL_VAL_ACODEC_AC3_PLUS_SEC 12                  ///< DOLBY-PLUS 音频编码标准.
#define GX_URL_VAL_ACODEC_DTS_HD_SEC   13                  ///< DTS-HD 音频编码标准.
#define GX_URL_VAL_ACODEC_DRA1         14                  ///< DRA1 音频编码标准.

#define GX_URL_VAL_VCODEC_MPG2  0                          ///< MPEG2 视频编码标准.
#define GX_URL_VAL_VCODEC_AVS   1                          ///< AVS 视频编码标准.
#define GX_URL_VAL_VCODEC_H264  2                          ///< H264 视频编码标准.
#define GX_URL_VAL_VCODEC_H265  3                          ///< H265 视频编码标准.
#define GX_URL_VAL_VCODEC_MPG4  4                          ///< MPEG4 视频编码标准.
#define GX_URL_VAL_VCODEC_H263  5                          ///< H263 视频编码标准.
#define GX_URL_VAL_VCODEC_REAL  6                          ///< REAL 视频编码标准.

#define GX_URL_VAL_SYNC_AVPTS     0                        ///< 音视频 PTS 恢复 STC.
#define GX_URL_VAL_SYNC_VPTS      1                        ///< 视频 PTS 恢复 STC.
#define GX_URL_VAL_SYNC_APTS      2                        ///< 音频 PTS 恢复 STC.
#define GX_URL_VAL_SYNC_PCR       3                        ///< PCR 恢复 STC.

#define GX_URL_VAL_TSCACHE_MODE_USER          0            ///< 快速切台缓存使用用户态模式.
#define GX_URL_VAL_TSCACHE_MODE_KERNEL_NORMAL 1            ///< 快速切台缓存使用内核态模式, TSR / TSW 单独 BUF.
#define GX_URL_VAL_TSCACHE_MODE_KERNEL_SHARED 2            ///< 快速切台缓存使用内核态模式, TSR / TSW 共同 BUF.

/**
 * URL 的协议类型
 **/
typedef enum {
	GX_URL_DVBT,    ///< DVB-T 协议.
	GX_URL_DVBS,    ///< DVB-S 协议.
	GX_URL_DVBC,    ///< DVB-C 协议.
	GX_URL_DTMB,    ///< DTMB 协议.
	GX_URL_MEM,     ///< MEM 协议.
	GX_URL_DVBNET   ///< DVB-NET 协议.
} GxUrlType;

/**
 * \brief   申请 URL.
 * \details 1. 通过 GxUrl_Destroy 释放资源.
 *
 * \param   urltype [in] 协议类型,详见: gxdocref GxUrlType.
 * \retval  非NULL       成功, URL 地址.
 * \retval  NULL         失败.
 **/
extern char* GxUrl_Create(GxUrlType urltype);

/**
 * \brief   释放申请的 URL.
 *
 * \param   url [in]  申请 URL 的地址.
 * \retval  void.
 **/
extern void  GxUrl_Destroy(char* url);

/**
 * \brief   删除 URL 中的字段标识.
 * \details 1. URL 地址保持不变.
 *
 * \param   url  [in,out] 申请 URL 的地址.
 * \param   item [in]     URL 的字段标识.
 * \retval  void.
 **/
extern void  GxUrl_DelItem(char* url, const char* item);

/**
 * \brief   增加/修改 URL 中的字段标识.
 * \details 1. URL 地址保持不变.
 *
 * \param   url     [in,out]  申请 URL 的地址.
 * \param   item    [in]      URL 的字段标识.
 * \param   itemval [in]      URL 的字段标识对应的值.
 * \param   max_len [in]     参数最大长度,参数废弃.
 * \retval   0  成功.
 * \retval  -1  失败.
 **/
extern int  GxUrl_SetItem(char* url, const char* item, int32_t itemval, int32_t max_len);

/**
 * \brief   获取 URL 中的字段标识对应的整数值.
 *
 * \param   url     [in]   申请 URL 的地址.
 * \param   item    [in]   URL 的字段标识.
 * \param   retVal  [out]  URL 的字段标识对应的值.
 * \retval  void.
 **/
extern void  GxUrl_GetItem(const char* url, const char *item, int32_t *retVal);

/**
 * \brief   获取 URL 中的字段标识对应的字符串.
 *
 * \param   url       [in]     申请 URL 的地址.
 * \param   item      [in]     URL 的字段标识.
 * \param   strBuffer [in,out] 存储地址, URL 字符标识对应的字符串.
 * \param   strBufferSize [in] 存储大小
 * \retval  void.
 **/
extern void  GxUrl_GetItemStr(const char* url, const char* item, uint8_t *strBuffer, int32_t strBufferSize);

__END_DECLS

#endif

/** @}*/
