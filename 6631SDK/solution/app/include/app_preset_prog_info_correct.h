#ifndef __APP_PRESET_PROG_INFO_CORRECT_H__
#define __APP_PRESET_PROG_INFO_CORRECT_H__

#include "app_config.h"
#if PRESET_PROG_INFO_CORRECT_SUPPORT
#include <gxtype.h>
#include "module/pm/gxpm_manage.h"

#ifndef APP_FREE
#define APP_FREE(x) if(x){GxCore_Free(x);x=NULL;}
#endif

/*Currently only ts_id has been corrected, and further improvements are needed*/

/**
 * @brief Check whether the correct program info function is running
 *
 * @param void
 *
 * @return  true: function is running
 *          false: function is not running
 */
extern bool app_preset_prog_info_correct_running_check(void);
/**
 * @brief Preset program info correction stop
 *
 * @param void
 *
 * @return  void
 */
extern void app_preset_prog_info_correct_stop(void);
/**
 * @brief Correct the wrong program information under current tp
 *
 * @param dmx_id: dmx_id currently bound to ts
 *        tp_id: the tp_id of current program
 *
 * @return  0: success
 *          -1: fail
 */
extern int32_t app_preset_prog_info_correct_start(uint32_t dmx_id, uint32_t tp_id);
#endif
#endif
