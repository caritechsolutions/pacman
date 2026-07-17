#ifndef __APP_CAT_H__
#define __APP_CAT_H__

#include "gxcore.h"
#include "app_module.h"
#include "gxsi.h"
#include "gxmsg.h"
#include "app_send_msg.h"
#if EMM_SUPPORT


typedef struct app_cat AppCat;
struct app_cat
{
    uint16_t ts_src;
	uint32_t demuxid;
    uint32_t pid;
    uint32_t timeout;

    int16_t  subt_id;
    uint32_t req_id;
    uint32_t ver;

	void		(*start)(AppCat*);
	uint32_t	(*stop)(AppCat*);
	status_t	(*analyse)(AppCat*, unsigned char*); 
};

extern AppCat       g_AppCat;

int app_cat_start(struct app_cat *cat);
int app_cat_restart(void);
int app_cat_release(void);
int app_cat_get(GxMsgProperty_SiSubtableOk *data);

#endif
#endif
