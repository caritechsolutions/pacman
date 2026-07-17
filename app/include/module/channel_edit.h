/*
 * =====================================================================================
 *
 *       Filename:  channel_edit.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2011年07月15日 14时51分26秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "gxcore.h"
#include "app_config.h"
#define    MODE_FLAG_FAV1      ((uint64_t)1)
#define    MODE_FLAG_FAV2      ((uint64_t)1<<1)
#define    MODE_FLAG_FAV3      ((uint64_t)1<<2)
#define    MODE_FLAG_FAV4      ((uint64_t)1<<3)
#define    MODE_FLAG_FAV5      ((uint64_t)1<<4)
#define    MODE_FLAG_FAV6      ((uint64_t)1<<5)
#define    MODE_FLAG_FAV7      ((uint64_t)1<<6)
#define    MODE_FLAG_FAV8      ((uint64_t)1<<7)
#define    MODE_FLAG_FAV9      ((uint64_t)1<<8)
#define    MODE_FLAG_FAV10     ((uint64_t)1<<9)
#define    MODE_FLAG_FAV11     ((uint64_t)1<<10)
#define    MODE_FLAG_FAV12     ((uint64_t)1<<11)
#define    MODE_FLAG_FAV13     ((uint64_t)1<<12)
#define    MODE_FLAG_FAV14     ((uint64_t)1<<13)
#define    MODE_FLAG_FAV15     ((uint64_t)1<<14)
#define    MODE_FLAG_FAV16     ((uint64_t)1<<15)
#define    MODE_FLAG_FAV17     ((uint64_t)1<<16)
#define    MODE_FLAG_FAV18     ((uint64_t)1<<17)
#define    MODE_FLAG_FAV19     ((uint64_t)1<<18)
#define    MODE_FLAG_FAV20     ((uint64_t)1<<19)
#define    MODE_FLAG_FAV21     ((uint64_t)1<<20)
#define    MODE_FLAG_FAV22     ((uint64_t)1<<21)
#define    MODE_FLAG_FAV23     ((uint64_t)1<<22)
#define    MODE_FLAG_FAV24     ((uint64_t)1<<23)
#define    MODE_FLAG_FAV25     ((uint64_t)1<<24)
#define    MODE_FLAG_FAV26     ((uint64_t)1<<25)
#define    MODE_FLAG_FAV27     ((uint64_t)1<<26)
#define    MODE_FLAG_FAV28     ((uint64_t)1<<27)
#define    MODE_FLAG_FAV29     ((uint64_t)1<<28)
#define    MODE_FLAG_FAV30     ((uint64_t)1<<29)
#define    MODE_FLAG_FAV31     ((uint64_t)1<<30)
#define    MODE_FLAG_FAV32     ((uint64_t)1<<31)
#define    MODE_FLAG_DEL       ((uint64_t)1<<32)
#define    MODE_FLAG_SKIP      ((uint64_t)1<<33)
#define    MODE_FLAG_LOCK      ((uint64_t)1<<34)
#define    MODE_FLAG_MOVE      ((uint64_t)1<<35)
#define    MODE_FLAG_MOVING    ((uint64_t)1<<36)
#define    MODE_FLAG_FAV       ((uint64_t)0xffffffff)


typedef enum{
    SORT_A_Z,
    SORT_Z_A,
    SORT_TP,
    SORT_FREE_SCRAMBLE,
    SORT_FAV_UNFAV,
    SORT_LOCK_UNLOCK,
    SORT_HD_SD,
#if TKGS_SUPPORT || LCN_SUPPORT
    SORT_BY_LCN,
#endif
    SORT_BY_SERVICE_ID
}sort_mode;

// for channel edit
typedef struct
{
    status_t    (*create)(uint32_t);               // (total)
    void        (*destroy)(void);                  // ()
    void        (*set)(uint32_t, uint64_t);        // (pos, flag)
    void        (*clear)(uint32_t, uint64_t);      // (pos, flag)
    void        (*group_clear)(uint64_t);      // (flag)
    bool        (*check)(uint32_t, uint64_t);      // (pos, flag)
    uint32_t        (*group_check)(uint64_t);      // (flag)return num
    uint32_t    (*get_id)(uint32_t);                // (pos)return id
    void        (*move)(uint32_t, uint32_t);       // (pos_src, pos_dst)
    void        (*group_move)(uint32_t);       // (pos_dst)
    void        (*sort)(sort_mode, uint32_t*);
    bool        (*verity)(void);
    void        (*save)(void);
    uint32_t    (*real_pos)(uint32_t);
    uint32_t    (*real_select)(uint32_t);
    uint32_t    (*get_next_pos)(uint32_t);
    uint32_t    (*get_prev_pos)(uint32_t);
    bool        (*get_visible_flag)(uint32_t);
    uint32_t    (*get_list_total)(void);
    uint32_t    (*get_cur_pos)(uint32_t);
    void        (*update_prog_num)(void);
    uint32_t    (*get_tv_num)(void);
    uint32_t    (*get_radio_num)(void);
#if TKGS_SUPPORT || LCN_SUPPORT
    uint32_t    (*get_lcn)(uint32_t);                // (pos)return lcn
#endif
}AppChOps;


extern AppChOps g_AppChOps;

