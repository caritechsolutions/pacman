/*
 * =====================================================================================
 *
 *       Filename:  app_pvr.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2011年09月20日 14时18分15秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#ifndef __APP_PVR_H__
#define __APP_PVR_H__

#include "gxcore.h"
#include "app_pop.h"
#include "gxhotplug.h"

#define PVR_DEV_STR_LEN         11  // ECOS: "/dev/usbx/x" LINUX:"/dev/sdxx"
#define PVR_DEV_LEN             12  // PVR_DEV_STR_LEN + 1

#define PVR_ENTRY_STR_LEN       11  // ECOS: "/mnt/usbxx" LINUX:"/media/sdxx"
#define PVR_ENTRY_LEN           12  // PVR_ENTRY_STR_LEN + 1

#define PVR_DIR_STR             "/GxPvr"
#define PVR_DIR_STR_LEN         6
#define PVR_DIR_LEN             7   // PVR_DIR_STR_LEN + 1

#define PVR_PATH_STR_LEN        17  // PVR_ENTRY_STR_LEN+PVR_DIR_STR_LEN
#define PVR_PATH_LEN            18  // PVR_PATH_STR_LEN + 1

#define PVR_NAME_LEN            64  // "hntv_19700101000000.ts.dvr": >(MAX_PROG_NAME(32)+1+14+7+1/MAX_PROG_NAME(32)+1+14+8+1)
#define PVR_FULL_NAME_LEN       128 // PVR_PATH_LEN + PVR_NAME_LEN
#define PVR_INDEX_SUFFIX        "dvr;DVR"
#if SECURE_PVR_SUPPORT
#define PVR_MEDIE_SUFFIX        "ets;ETS"
#define PVR_FIX_SIFFIX          ".ets.dvr"
#define PVR_FIX_SIFFIX_LEN      8
#else
#define PVR_MEDIE_SUFFIX        "ts;TS"
#define PVR_FIX_SIFFIX          ".ts.dvr"
#define PVR_FIX_SIFFIX_LEN      7
#endif

typedef enum
{
    PVR_DUMMY,
    PVR_RECORD,
    PVR_TIMESHIFT,
    PVR_TMS_AND_REC,
    PVR_SPEED,
    PVR_TP_RECORD,
}pvr_state;

typedef struct
{
    uint32_t    cur_tick;
    uint32_t    total_tick;
    uint32_t    seekmin_tick;
    uint32_t    remaining_tick;
    uint32_t    rec_duration;
}pvr_time;


typedef struct
{
    uint8_t lock_flag;
}PrivateProgInfo;

typedef struct
{
    uint32_t    prog_id;
    char        *dev;   //
    char        *path;  // 录制的路径
    char        *name;  // 录制的件名
    char        *url;   // 发给player时移或录制的url
}pvr_env;

typedef enum
{
    PVR_SPD_1    =   (1),
    PVR_SPD_2    =   (2),
    PVR_SPD_4    =   (4),
    PVR_SPD_8    =   (8),
    PVR_SPD_16   =   (16),
    PVR_SPD_24   =   (24),
    PVR_SPD_32   =   (32)
}pvr_speed;

typedef enum
{
    USB_OK = 0,
    USB_NO_SPACE,
    USB_ERROR,
    USB_READ_ONLY,
}usb_check_state;

typedef struct _pvr_ops AppPvrOps;
struct _pvr_ops
{
    pvr_state       state;
    pvr_time        time;
    pvr_env         env;
    int32_t         spd;
	int32_t         enter_shift;
    usb_check_state    (*usb_check)(AppPvrOps*);
    status_t        (*env_sync)(AppPvrOps*);
    void            (*env_clear)(AppPvrOps*);
    void            (*rec_start)(AppPvrOps*);
    void            (*tp_rec_start)(AppPvrOps*);
    void            (*tplist_rec_start)(AppPvrOps*);
    void            (*rec_stop)(AppPvrOps*);   // RECORD only
    void            (*tms_stop)(AppPvrOps*); // TMS onlt
    void            (*pause)(AppPvrOps*);
    void            (*resume)(AppPvrOps*);
    void            (*seek)(AppPvrOps*, int64_t);
    void            (*speed)(AppPvrOps*, float);
    int32_t        (*percent)(AppPvrOps*);
    void            (*tms_delete)(AppPvrOps*);  // delete tms file
    //status_t        (*free_space)(AppPvrOps*);
};

typedef enum
{
    DEALWITH_NONE = 0,
    DEALWITH_KEEPON,
    DEALWITH_STOP
}PvrDealWithMode;

extern AppPvrOps g_AppPvrOps;

extern int app_pvr_get_free_space(AppPvrOps *pvr);
extern int app_pvr_check_change_state(void);
extern pvr_state app_pvr_get_state(void);
extern char *app_pvr_get_tips(pvr_state state);
extern PvrDealWithMode app_pvr_popdlg_handler(ExitCb exit_cb);
extern int app_pvr_create_no_btn_popdlg(ExitCb exit_cb);

int pvr_partition_get(HotplugPartition *partition);
uint32_t pvr_partition_and_path_get(HotplugPartition *partition, char *path, uint32_t max_len); // 0, err; >0, path_len
#endif

