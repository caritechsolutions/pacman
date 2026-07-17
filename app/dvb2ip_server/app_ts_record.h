#ifndef __APP_TS_RECORD_H__
#define __APP_TS_RECORD_H__
#include "gxcore.h"
#include "gxos/gxcore_os.h"
#include "gxavdev.h"
#include "module/app_psi.h"
#include "module/pm/gxpm_manage.h"
#include "app_config.h"

#if DVB2IP_SERVER_SUPPORT
typedef struct _TsRecExtInfo
{
#define TS_REC_MAX_EXTPID_NUM (32)
    uint32_t ext_num;
    uint32_t ext_pids[TS_REC_MAX_EXTPID_NUM];
}TsRecExtInfo;

typedef struct _TsRecConfig
{
    uint16_t prog_id;  // prog id in pm
    bool user_pmt;     //true:app provide pmt data, false: filter from demux
    TsRecExtInfo ext_info;
}TsRecConfig;

/**
 * @brief init prog ts record control
 * @param fifo_size: prog multi fifo size, min size should be (20 * 256 * 188)
 *        max_prog:  the num of prog support
 *
 * @return 0:success
 *         -1:fail
 */
extern int32_t app_ts_record_init(int32_t fifo_size, int32_t max_prog);

/**
 * @brief release prog ts record control
 */
extern void app_ts_record_destroy(void);

/**
 * @brief start one prog recording
 * @param config: the parameter of prog record
 *
 * @return handle for reading
 *         0:     failed
 *         other: success
 */
extern handle_t app_ts_record_start(TsRecConfig *config);

/**
 * @brief stop prog recording
 * @param handle: prog handle, get from app_ts_record_start
 *
 */
extern void app_ts_record_stop(handle_t handle);

/**
 * @brief read ts packet
 * @param handle: prog handle, get from app_ts_record_start
 *        buffer: the memory to store ts packet, alloced by caller
 *        size:   the size you want to read
 *
 * @return the size of read data
 *
 */
extern int32_t app_ts_record_read(handle_t handle, uint8_t *buffer, int32_t size);

#endif
#endif
