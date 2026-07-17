/**
 *
 * @file        app_dvbt2_search.h
 * @brief
 * @version     1.1.0
 * @date        06/22/2014 09:40:49 AM
 * @author      dugan
 *
 */
#ifndef __APP_DVBT2_SEARCH__H__
#define __APP_DVBT2_SEARCH__H__
#ifdef __cplusplus
extern "C" {
#endif
#include <gxtype.h>

#define MAX_CH_NAME_LEN	(5)
typedef struct
{
    uint8_t  channel[MAX_CH_NAME_LEN];
    float freq;
    fe_bandwidth_t band;//bandwidth
}classDefaultFreqCh;

#define MAX_COUNTRY_NAME_LEN	(16)
extern int AppDvbt2GetFreqList(classDefaultFreqCh **fch);
extern void app_dvbt2_auto_search_entry(void);
extern void AppDvbt2SetCountry(int s_country);
extern int AppDvbt2GetCountry(void);
extern char *AppDvbt2GetCountryName(int s_country);
extern int AppDvbt2GetCountryNum(void);
#ifdef __cplusplus
}
#endif
#endif /*__APP_CABLE_SEARCH__H__*/

