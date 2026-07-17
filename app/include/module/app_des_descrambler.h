#ifndef __APP_DES_DESCRAMBLER_H__
#define __APP_DES_DESCRAMBLER_H__

#include "module/app_des_cache.h"
#include "module/player/gxplayer_module.h"
#include "devapi/gxmtc_api.h"
#include "gxcore.h"
#include "app_config.h"


#define SYNC_BYTE (0x47)
#define TS_MAXLEN (188)

#define TSD_CHECK_RET(ret)\
    do {\
        if(ret < 0) \
        {\
            app_log_error("[DES Descrambler]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);\
            goto ERR;\
        }\
    }while(0);

enum
{
    M2TS_ADAPTATION_RESERVED    = 0,
    M2TS_ADAPTATION_NONE        = 1,
    M2TS_ADAPTATION_ONLY        = 2,
    M2TS_ADAPTATION_AND_PAYLOAD = 3
};

enum
{
    DES_NO_DATA         = 0,
    DES_AUDIO_ODD_DATA  = 1,
    DES_AUDIO_EVEN_DATA = 2,
    DES_VIDEO_ODD_DATA  = 3,
    DES_VIDEO_EVEN_DATA = 4,
    DES_ALL_ODD_DATA    = 5,
    DES_ALL_EVEN_DATA   = 6
};

typedef enum
{
    CM_DMX_DES,
    CM_DMX_CSA
}CM_DMX_KEY_TYPE;

enum
{
    DES_KEY_EVEN = 2,
    DES_KEY_ODD  = 3
};

//cache ecm key
typedef struct
{
    struct gxlist_head  head;
    unsigned char       type;
    unsigned char       vkey[16];
    unsigned char       akey[16];
} ecmkey;

typedef struct
{
    int running;
    int pthread;
    int cachesize, cache2mem;
    //char* cachedir;
    //unsigned char* fcache_r, *fcache_w;

    int blksize;
    unsigned int delayms;
    struct cachepage *cur_page_r, *cur_page_w;

    handle_t sem;
    struct des_cache cache;
    struct des_cache mtccache;
    //struct file_cache fcache;
} TSBuffStreamPortEx;

int app_des_set_key(unsigned char *cw, unsigned char type);

int app_des_set_pid(unsigned short vpid, unsigned short apid);

int app_des_get_pid(unsigned short *vpid, unsigned short *apid);

int app_des_enable(void);

int app_des_disable(void);
#endif
