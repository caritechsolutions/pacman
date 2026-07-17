#ifndef __APP_FM_RECORD_H__
#define __APP_FM_RECORD_H__
#include "gxcore.h"
#include "gxos/gxcore_os.h"
#include "app_config.h"

#if DVB2IP_SERVER_SUPPORT
typedef struct _FmRecConfig
{
    uint16_t index;
}FmRecConfig;

/**
 * @brief init prog ts record control
 * @param fifo_size: prog multi fifo size, min size should be (20 * 256 * 188)
 *        max_prog:  the num of prog support
 *
 * @return 0:success
 *         -1:fail
 */
extern int32_t app_fm_record_init(int32_t fifo_size, int32_t max_prog);

/**
 * @brief release prog ts record control
 */
extern void app_fm_record_destroy(void);

/**
 * @brief start one prog recording
 * @param config: the parameter of prog record
 *
 * @return handle for reading
 *         0:     failed
 *         other: success
 */
extern handle_t app_fm_record_start(FmRecConfig *config, int32_t need_lock_tp);

/**
 * @brief stop prog recording
 * @param handle: prog handle, get from app_ts_record_start
 *
 */
extern void app_fm_record_stop(handle_t handle);

/**
 * @brief read ts packet
 * @param handle: prog handle, get from app_ts_record_start
 *        buffer: the memory to store ts packet, alloced by caller
 *        size:   the size you want to read
 *
 * @return the size of read data
 *
 */
extern int32_t app_fm_record_read(handle_t handle, uint8_t *buffer, int32_t size);

extern void app_fm_buffer_free(void);
extern int32_t app_fm_get_freq(int32_t index);
#endif
#endif
