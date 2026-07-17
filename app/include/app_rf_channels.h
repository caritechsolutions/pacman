#include "app.h"
#ifndef __APP_RF_CHANNELS_H__
#define __APP_RF_CHANNELS_H__

#define MAX_DVBT2_TP_NUM 100

typedef float float_t;

typedef struct  _FreqTblType
{
       uint8_t u8CHNo;   //channel no.
       float_t u32Freq;   //frequency
       int8_t u8CHNoOffset;  //No. offset ----->for displaying 0:= No.  +X:No.-X and add "A"   -X:No.-X
}FreqTblType;

#define VHF_BAND_MIN	30000L
#define UHF_BAND_MIN	4700000L

#define FREQ_TAL_SIZE   7


#define	COUNTRY_AUSTRIA			"Austria"
#define	COUNTRY_BELGIUM     	"Belgium"
#define	COUNTRY_BULGARIA    	"Bulgaria"
#define	COUNTRY_CROATIA     	"Croatia"
#define	COUNTRY_CZECH       	"Czech"
#define	COUNTRY_DENMARK     	"Denmark"
#define	COUNTRY_FINLAND     	"Finland"
#define	COUNTRY_FRANCE      	"France"
#define	COUNTRY_GERMANY     	"Germany"
#define	COUNTRY_GREECE      	"Greece"
#define	COUNTRY_HUNGARY      	"Hungary"
#define	COUNTRY_ITALY       	"Italy"
#define	COUNTRY_LUXEMBOURG  	"Luxembourg"
#define	COUNTRY_NETHERLANDS 	"Netherlands"
#define	COUNTRY_NORWAY      	"Norway"
#define	COUNTRY_POLAND      	"Poland"
#define	COUNTRY_PORTUGAL    	"Portugal"
#define	COUNTRY_ROMANIA     	"Romania"
#define	COUNTRY_RUSSIAN 		"Russia"
#define	COUNTRY_SERBIA      	"Serbia"
#define	COUNTRY_SLOVENIA    	"Slovenia"
#define	COUNTRY_SPAIN       	"Spain"
#define	COUNTRY_SWEDEN      	"Sweden"
#define	COUNTRY_SWITZERLAND 	"Switzerland"
#define	COUNTRY_UK          	"England"
#define	COUNTRY_CHINA       	"China"
#define	COUNTRY_AUSTRALIA   	"Australia"
#define	COUNTRY_BRAZIL      	"Brazil"
#define	COUNTRY_PERU        	"Peru"
#define	COUNTRY_IRAN        	"Iran"
#define	COUNTRY_COLOMBIA    	"Colombia"
#define	COUNTRY_ARGENTINA   	"Argentina"
#define	COUNTRY_PARAGUAY    	"Paraguay"
#define	COUNTRY_CHILE       	"Chile"
#define	COUNTRY_URUGUAY     	"Uruguay"
#define	CONNTRY_INDIA 			"India"
#define	CONNTRY_VIET 			"Vietnam"
#define COUNTRY_ESTONIA 		"Estonia"
#define COUNTRY_IRELAND			"Ireland"
#define COUNTRY_LATVIA 			"Latvia"
#define COUNTRY_LITHUANIA 		"Lithuania"
#define COUNTRY_SLOVAKIA 		"slovakia"
#define COUNTRY_COLOMBIA  		"Colombia"
#define COUNTRY_SINGAPORE   	"Singapore"
#define COUNTRY_PHILIPPINES  	"Philippines"
#define COUNTRY_THAILAND   		"Thailand"
#define COUNTRY_INDONESIA       "Indonesia"

typedef enum _COUNTRY_TYPE
{
	COUNTRY_TYPE_EUROPE,
	COUNTRY_TYPE_GERMANY,
	COUNTRY_TYPE_TAIWAN,
	COUNTRY_TYPE_ITALY,
	COUNTRY_TYPE_FRANCE,
	COUNTRY_TYPE_CHINA,
	COUNTRY_TYPE_AUSTRALIA,
	COUNTRY_TYPE_BRAZIL,
	COUNTRY_TYPE_CHILE,
	COUNTRY_TYPE_HONGKONG,
	COUNTRY_TYPE_ARGENTINA,
	COUNTRY_TYPE_PERU,
	COUNTRY_TYPE_UK,
	COUNTRY_TYPE_RUSSIAN,
	COUNTRY_TYPE_INDIA,
	COUNTRY_TYPE_POLAND,
	COUNTRY_TYPE_EST,
	COUNTRY_TYPE_SK,
	COUNTRY_TYPE_LAT,
	COUNTRY_TYPE_LTU,
	COUNTRY_TYPE_VIET,
	COUNTRY_TYPE_COL,
	COUNTRY_TYPE_PHL,
	COUNTRY_TYPE_THAI,
	COUNTRY_TYPE_INA,
	COUNTRY_TYPE_OTHER,
	COUNTRY_TYPE_NUM,
}COUNTRY_TYPE;
typedef enum _BAND_TYPE_MODE
{
	BAND_VHF_MODE = 0,
	BAND_UHF_MODE,
	BAND_TOTAL
}BAND_TYPE_MODE;

typedef struct
{
	uint8_t 		ch_num;
	float_t 		frequency;
	int 			bandwidth;
}RfChannelItemPara;
uint8_t app_get_country_type(void);
uint8_t app_get_current_fre_info(float_t *current_fre,uint8_t *current_bandwidth);
uint16_t app_get_current_fre_rfch_num(uint8_t country,uint8_t *rfch);
uint32_t app_get_max_channel_num(uint8_t country);
uint32_t app_get_min_channel_num(uint8_t country);
uint32_t app_channel_fre2chnum(uint8_t country,float_t frequency);
uint32_t app_channel_chnum2fre(uint8_t country,uint8_t chnum,uint16_t *bandwidth,float_t *frequency);
uint32_t app_get_next_channel_num(uint8_t country,uint8_t chnum);
uint32_t app_get_prev_channel_num(uint8_t country,uint8_t chnum);
uint32_t app_initial_channels_by_country(uint32_t *total_num);
uint32_t app_get_max_channel_num_autosearch(uint8_t country);
uint32_t app_get_min_channel_num_autosearch(uint8_t country);
uint32_t app_channel_chnum2fre_autosearch(uint8_t country,uint8_t chnum,uint16_t *bandwidth,float_t *frequency);
uint32_t app_get_next_channel_num_autosearch(uint8_t country,uint8_t chnum);

extern uint32_t Total_RfCh;
extern RfChannelItemPara Tp_list[MAX_DVBT2_TP_NUM];

#endif /*__APP_RF_CHANNELS_H__ */

