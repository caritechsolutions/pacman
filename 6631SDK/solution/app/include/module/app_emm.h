#ifndef __APP_EMM_H__
#define __APP_EMM_H__

#include "gxcore.h"
#include "app_module.h"
#include "gxsi.h"
#include "gxmsg.h"
#include "app_send_msg.h"
#if EMM_SUPPORT

#define EMM_MAX_COUNT 16
#define EMM_DATA_LENGTH    (1024)
#define EMM_DATA_FIFO_NUM    (30)

typedef struct AppEmmSub
{
	uint32_t    pid;
    uint32_t    ca_id;
	int32_t     subt_id;
    uint32_t    req_id;
    uint32_t    ver;
}AppEmmSub_t;

struct app_emm
{
    uint16_t ts_src;
	uint16_t demuxid;
	uint32_t emm_count;
    uint32_t timeout;
    AppEmmSub_t emm_sub[EMM_MAX_COUNT];
};

void app_emm_init(void);
void app_emm_start(uint16_t ts_src, uint32_t demuxid, uint32_t pid, uint32_t caid, uint32_t timeout);
uint32_t app_emm_release_one(uint32_t pid);
uint32_t app_emm_release_all(void);
status_t app_emm_timeout(GxParseResult *parse_result);
status_t app_emm_analyse(uint32_t sub_idx, uint8_t *section_data);
status_t app_emm_get(GxMsgProperty_SiSubtableOk *data);

#endif
#endif
