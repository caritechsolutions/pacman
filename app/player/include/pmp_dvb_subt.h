#ifndef __PMP_DVB_SUBT_H__
#define __PMP_DVB_SUBT_H__

#include "module/app_ttx_subt.h"

///////////////dvb subtitle////////////////
status_t movie_dvb_subt_start(uint8_t subt_index);
status_t movie_dvb_subt_stop(void);
status_t movie_dvb_subt_switch(uint8_t subt_index);
status_t movie_dvb_subt_hide(void);
status_t movie_dvb_subt_show(void);

#endif
