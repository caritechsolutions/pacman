#ifndef _GX_SUBTTRANS_DEMUX_H
#define _GX_SUBTTRANS_DEMUX_H

/************* SUBTITLE TRANSLATION DEMUX DEFINE ***************/
// Gx_SubtTrans_SiInit: ST SI模块初始化
// Input: dmx_id
//      : tsid
// return: >=0 初始化成功
int Gx_SubtTrans_SiInit(uint32_t dmx_id, uint32_t tsid);

// Gx_SubtTrans_SiDestory: ST SI模块卸载
int Gx_SubtTrans_SiDestory(void);

// Gx_SubtTrans_SiPesStop: ST SI暂停过滤
int Gx_SubtTrans_SiPesStop(void);

// Gx_SubtTrans_SiPesStart: ST SI开始配置过滤指定PID的PES数据
// Input: pid 需过滤字幕的pid
//      : service_id 字幕节目的service_id
//      : subt_type 字幕的格式
//      : lang 字幕的语言
// return: =0, 成功
int Gx_SubtTrans_SiPesStart(uint32_t pid, uint16_t service_id, uint8_t subt_type, char *lang);
/***************** SUBTITLE TRANSLATION DEMUX ******************/

#endif
