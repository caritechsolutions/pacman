#include "app.h"
#include "app_utility.h"
#include "app_windows.h"
#include "gxhotplug.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_module.h"
#include "app_book.h"
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include "cyassl/openssl/sha.h"
#include <ctype.h>
#include <gxplayer_url.h>
#include "av/hal/gxav_hal_hdmi.h"
#include "av/hal/gxav_hal_vout.h"
#if !OTT_SUPPORT
#include "module/app_frontend_board_config.h"
#endif
#include "module/app_log.h"
#if DEMOD_DVB_T || DEMOD_ISDBT
#include "app_t2_area_config.h"
#endif
#include "module/app_time.h"
#include <frontend/demod.h>
#include "devapi/chip_info.h"
#include "devapi/gxsystem_api.h"
#include <memmgr.h>

#if TKGS_SUPPORT
#include "module/tkgs/gxtkgs.h"
#endif

#if LCN_SUPPORT
#include "app_lcn.h"
#endif
#if TKGS_SUPPORT
#include "module/tkgs/gxtkgs.h"
#endif

#define LOCAL_DATE_LEN 40
#define JPG_BG          "bg"

#if (defined GEMINI || defined CYGNUS)
static unsigned int _festart = 0;
static struct gxav_memhole_info femem_info;
#endif

static char *thiz_utiltiy_iso639_langmap[] = {
    "eng,en",
    "vi,vie",
    "ar,ara",
    "fr,fra,fre",
    "ru,rus",
    "pt,por",
    "es,spa",
    "tr,tur",
    "it,ita",
    "pl,pol",
    "de,deu,ger",
    "fa,fas,per",
    "ms,may,msa",//马来西亚
    "id,ind",//印地语
    "my,bur,mya",//缅甸语
    "th,tha",//泰语
    "hi,hin",//印尼语
    "lo,lao",//老挝语
    "ur,urd",//乌尔都语
    "ko,kor",//韩语
    "el,ell,gre",//希腊语
    "uk,ukr",//乌克兰语
    "ja,jap",//日语
    "zh,chi,zho",
    "cs,ces,cze",//捷克语
    "nl,dut",//荷兰语
    "da,dan,dns",//丹麦语
};

#define UTILITY_ISO639_MAP_COUNT   sizeof(thiz_utiltiy_iso639_langmap)/sizeof(char*)

static char *thiz_utiltiy_iso3166_code[] = {
    "Afghanistan,AF,AFG",
    "Albania,AL,ALB",
    "Algeria,DZ,DZA",
    "Angola,AO,AGO",
    "America,US,USA",
    "Argentina,AR,ARG",
    "Armenia,AM,ARM",
    "Australia,AU,AUS",
    "Austria,AT,AUT",
    "Azerbaijan,AZ,AZE",
    "Bahrain,BH,BHR",
    "Bangladesh,BD,BGD",
    "Belarus,BY,BLR",
    "Belgium,BE,BEL",
    "Bulgaria,BG,BGR",
    "Bolivia,BO,BOL",
    "Bhutan,BT,BTN",
    "Botswana,BW.BWA",
    "Brazil,BR,BRA",
    "Cambodia,KH,KHM",
    "Cameroon,CM,CMR",
    "Canada,CA,CAN",
    "Chile,CL,CHL",
    "China,CN,CHN",
    "Colombia,CO,COL",
    "Congo,CG,COG",
    "Croatia,HR,HRV",
    "Cuba,CU,CUB",
    "Cyprus,CY,CYP",
    "Czech,CZ,CZE",
    "Denmark,DK,DNK",
    "Ecuador,EC,ECU",
    "Egypt,EG,EGY",
    "England,GB,GBR",
    "Estonia,EE,EST",
    "Ethiopia,ET,ETH",
    "Finland,FI,FIN",
    "France,FR,FRA",
    "Gabon,GA,GAB",
    "Georgia,GE,GEO",
    "Germany,DE,DEU",
    "Ghana,GH,GHA",
    "Greece,GR,GRC",
    "Greenland,GL,GRL",
    "Guinea,GN,GIN",
    "Hungary,HU,HUM",
    "Iceland,IS,ISL",
    "Indonesia,ID,IDN",
    "India,IN,IND",
    "Iran,IR,IRN",
    "Iraq,IQ,IRQ",
    "Ireland,IE,IRL",
    "Israel,IL,ISR",
    "Italy,IT,ITA",
    "Japan,JP,JPN",
    "Jordan,JO,JOR",
    "Kazakhstan,KZ,KAZ",
    "Kuwait,KW,KWT",
    "Korea,KR,KOR",
    "Kyrgyzstan,KG,KGZ",
    "Latvia,LV,LVA",
    "Lebanon,LB,LBN",
    "Lithuania,LT,LTU",
    "Liberia,LR,LBR",
    "Luxembourg,LU,LUX",
    "Mali,ML,MLI",
    "Madagascar,MG,MDG",
    "Maldives,MV,MDV",
    "Malaysia,MY,MYS",
    "Mexico,MX,MEX",
    "Mongolia,MN,MNG",
    "Montenegro,ME,MNE",
    "Morocco,MA,MAR",
    "Myanmar,MM,MMR",
    "Namibia,NA,NAM",
    "Nepal,NP,NPL",
    "Nigeria,NG,NGA",
    "Netherlands,NL,NLD",
    "Norway,NO,NOR",
    "Pakistan,PK,PAK",
    "Panama,PA,PAN",
    "Paraguay,PY,PRY",
    "Peru,PE,PER",
    "Philippines,PH,PHL",
    "Portugal,PT,PRT",
    "Poland,PL,POL",
    "Qatar,QA,QAT",
    "Rumania,RO,ROU",
    "Russia,RU,RUA",
    "Senegal,SN,SEN",
    "Serbia,RS,SRB",
    "Singapore,SG,SGP",
    "Slovakia,SK,SVK",
    "Slovenia,SI,SVN",
    "Spain,ES,ESP",
    "Sweden,SE,SWE",
    "Switzerland,CH,CHE",
    "Tajikistan,TJ,TJK",
    "Thailand,TH,THA",
    "Turkey,TR,TUR",
    "Turkmenistan,TM,TKM",
    "Ukraine,UA,UKR",
    "Uzbekistan,UZ,UZB",
    "Vietnam,VN,VNM",
};

#define UTILITY_ISO3166_MAP_COUNT   sizeof(thiz_utiltiy_iso3166_code)/sizeof(char*)

static int _is_matched(char *src, char *dst)
{
    char *p = src;
    char *str = dst;
LOOP:
    if((p = strstr((const char*)p, (const char*)str))
        && ((*(p+strlen((const char*)str)) == ',')
            || (*(p+strlen((const char*)str)) == '\0')
            || (*(p+strlen((const char*)str)) == 0x20))
        && ((p == src)
            || ((p != src)
            && ((*(p-1) == ',') || ((*(p-1) == 0x20)))))
        )
    {
        return 1;
    }
    if(p)
    {
        p += 1;
        goto LOOP;
    }
    return 0;
}

static int _language_match(unsigned char* lang1, unsigned char* lang2, unsigned int lang_len)
{
#define TEMP_LEN  10 // i think it is enough forever
    int i = 0;
    int count = UTILITY_ISO639_MAP_COUNT;
    char str[TEMP_LEN] = {0};
    if(lang_len >= TEMP_LEN)
    {
        gxlogd("\nERROR, %s, %d\n", __func__, __LINE__);
        return -1;
    }
    memcpy(str, lang1, lang_len);
    for(i = 0; i < count; i++)
    {
        if(thiz_utiltiy_iso639_langmap[i])
        {
            if(_is_matched(thiz_utiltiy_iso639_langmap[i], str))
            {// find lang1
                memset(str, 0, TEMP_LEN);
                memcpy(str, lang2, lang_len);
                if(_is_matched(thiz_utiltiy_iso639_langmap[i], str))
                {// fine lang2
                    return 1;
                }
                return 0;
            }
        }
    }
    return 0;
}

int app_language_code_cmp(char* language1, char* language2)
{
#define LANGUAGE_LEN    (3)
    uint8_t language_1[LANGUAGE_LEN];
    uint8_t language_2[LANGUAGE_LEN];
    uint16_t i;

    if(strlen(language1) == 0 || strlen(language2) == 0)
    {
        return -1;
    }
    for (i=0; i<LANGUAGE_LEN; i++)
    {
        language_1[i] = tolower(language1[i]);
        language_2[i] = tolower(language2[i]);
    }
    if(memcmp((const void *)language_1, (const void *)language_2, LANGUAGE_LEN) == 0)
    {
        return 0;
    }
    else
    {
        if(_language_match(language_1, language_2, LANGUAGE_LEN))
            return 0;
        else
            return -1;
    }
}

int app_iso3166_country_code_get(char* countroy_name,char* three_character_code,char* two_character_code)
{
    int i = 0;
    int len = 0;
    char country[32] = {0};
    char country_code1[4] = {0};
    char country_code2[4] = {0};
    if(countroy_name == NULL)
        return -1;

    len = strlen(countroy_name);
    for(i = 0 ; i < UTILITY_ISO3166_MAP_COUNT;i++)
    {
        if(strncasecmp(thiz_utiltiy_iso3166_code[i],countroy_name,len) == 0)
        {
            sscanf(thiz_utiltiy_iso3166_code[i],"%[^','],%[^','],%[^',']",country,country_code1,country_code2);
            if(two_character_code != NULL)
            {
                memcpy(two_character_code,country_code1,sizeof(country_code1));
            }
            if(three_character_code != NULL)
            {
                memcpy(three_character_code,country_code2,sizeof(country_code2));
            }
            return 0;
        }
    }
    return -1;
}

#if UI_MIRROR_SUPPORT
void app_menu_mirror_set(char* language)
{
    char mirror_en[8] = {0};
    ARABIC_STR_PUNCTUATION_MODE mode;

    if(language == NULL)
        return;

    if((strcmp(language,"Persian") == 0)
            ||(strcmp(language,STR_ID_ARABIC) == 0)
            ||(strcmp(language,STR_ID_FARSI) == 0))
    {
        sprintf(mirror_en,"enable");
        mode = ARABIC_PUNCTUATION_MODE1;
    }
    else
    {
        sprintf(mirror_en,"disable");
        mode = ARABIC_PUNCTUATION_MODE0;
    }
    GXGDI_SetArabicPunctuationMode(mode);
    GUI_SetInterface("mirror", &mirror_en);
    GUI_SetInterface("keypress_revert", "disable");
}

int app_menu_mirror_get(void)
{
	int32_t init_value;

	GxBus_ConfigGetInt(OSD_LANG_KEY, &init_value, OSD_LANG);

    if((strcmp(g_LangName[init_value],"Persian") == 0)
            ||(strcmp(g_LangName[init_value],STR_ID_ARABIC) == 0)
            ||(strcmp(g_LangName[init_value],STR_ID_FARSI) == 0))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

#endif

time_t get_local_time_by_timezone(time_t src_sec)
{
    int zone, half_zone, summer;
    app_time_zone_config_get(&zone, &half_zone, &summer);
    return app_time_to_utc_time(src_sec, zone, half_zone, summer);
}

time_t get_display_time_by_timezone(time_t src_sec)
{
    int zone, half_zone, summer;
    app_time_zone_config_get(&zone, &half_zone, &summer);
    return app_time_to_local_time(src_sec, zone, half_zone, summer);
}

WeekDay app_weekday_local2gmt(WeekDay local_weekday, time_t local_day_sec)
{
    WeekDay weekday_ret = local_weekday;
    int temp_sec = get_local_time_by_timezone(local_day_sec);

    if(temp_sec < 0) //gmt+n
    {
        (weekday_ret == WEEK_SUN) ? (weekday_ret = WEEK_SAT) : (weekday_ret--);
    }
    else if(temp_sec >= 24 * HOUR_TO_SEC) //gmt-n
    {
        (weekday_ret == WEEK_SAT) ? (weekday_ret = WEEK_SUN) : (weekday_ret++);
    }

    return weekday_ret;
}

WeekDay app_weekday_gmt2local(WeekDay gmt_weekday, time_t gmt_day_sec)
{
    WeekDay weekday_ret = gmt_weekday;
    int temp_sec = get_display_time_by_timezone(gmt_day_sec);

    if(temp_sec >= 24 * HOUR_TO_SEC)//gmt+n
    {
        (weekday_ret == WEEK_SAT) ? (weekday_ret = WEEK_SUN) : (weekday_ret++);
    }
    else if(temp_sec < 0)//gmt-n
    {
        (weekday_ret == WEEK_SUN) ? (weekday_ret = WEEK_SAT) : (weekday_ret--);
    }

    return weekday_ret;
}

char* get_local_date(char *date_value,uint32_t data_len)
{// data_len must over 17
    GxTime local_time;
    struct tm ptm = {0};
    struct tm *ret = NULL;
    time_t time_sec = 0;

    if(NULL == date_value || LOCAL_DATE_LEN > data_len)
        return NULL;

    GxCore_GetLocalTime(&local_time);
    time_sec = get_display_time_by_timezone((time_t)(local_time.seconds));
    ret = localtime_r(&time_sec, &ptm);

    if(NULL != ret)
    {
        memset(date_value,0,data_len*sizeof(char));
        sprintf(date_value,"%d/%02d/%02d %02d:%02d", ptm.tm_year+1900,ptm.tm_mon+1,ptm.tm_mday,ptm.tm_hour,ptm.tm_min);
        date_value[data_len-1] = 0;
        return date_value;
    }
    return NULL;
}

bool check_usb_status(void)
{
    HotplugPartitionList* list = NULL;
    list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);

    if((NULL != list)
        && (HOTPLUG_TYPE_USB == list->type)
        && (0 < list->partition_num))
    {
        return TRUE;
    }
    return FALSE;
}

bool check_usb_write_permissions(const char *path)
{
#ifdef LINUX_OS
    struct stat st = {0};

    if (path == NULL || lstat(path, &st) < 0) {
        app_log_error("%s,%d:can't stat %s,%s\n", __func__, __LINE__, path,strerror(errno));
        return false;
    }

    if (S_ISDIR(st.st_mode)) {
        if (access(path, W_OK) == 0) {
            return true;
        }
    } else {
        if (st.st_mode & S_IWUSR) {
            return true;
        }
    }

    app_log_debug("%s,%d:%s no write permissions\n", __func__, __LINE__, path);
    return false;
#else
    return true;
#endif
}

uint64_t get_usb_partition_space(char *partition_path)
{
	if(partition_path == NULL)
		return 0;

	GxDiskInfo disk_info;
	if(GxCore_DiskInfo(partition_path, &disk_info) == GXCORE_ERROR)
		return 0;

	return disk_info.freed_size;
}

//free result extern, url:"/mnt/usbx_x/xxx"
char *get_usb_url_partition(char *file_url)
{
	char *p = file_url;
	char *res = NULL;
	int cnt = 0;
	int len = 0;

	if(p == NULL)
		return NULL;

	while(*p != '\0')
	{
		if(*p == '/')
		{
			cnt++;
			if(cnt >= 3)
				break;
		}
		p++;
	}

	if(cnt < 2)
		return NULL;

	len = p - file_url;
	res = (char*)GxCore_Calloc(len + 1, 1);
	if(res != NULL)
	{
		strncpy(res, file_url, len);
	}

	return res;
}

int app_date_string_format(void)
{
#if (DEMOD_DVB_T || DEMOD_ISDBT)
	int 	country = 0;
	GxBus_ConfigGetInt(T2_COUNTRY, &country, T2_COUNTRY_VALUE);
//	printf("app_date_string_format g_CountryName[%d] =%s\n",country,g_CountryName[country]);
	if((strcmp(g_CountryName[country], "Russia") == 0)||
		(strcmp(g_CountryName[country], "Germany") == 0)||
		(strcmp(g_CountryName[country], "Norway") == 0)||
		(strcmp(g_CountryName[country], "Serbia") == 0)||
		(strcmp(g_CountryName[country], "France") == 0))
	{/*DD.MM.YYYY*/
		return DMY_WITH_DOT;
	}
	else if((strcmp(g_CountryName[country], "England") == 0)||
			(strcmp(g_CountryName[country], "Belgium") == 0)||
			(strcmp(g_CountryName[country], "Italy") == 0)||
			(strcmp(g_CountryName[country], "Spain") == 0))
	{/*DD/MM/YYYY*/
		return DMY_WITH_SLASH;
	}
	else if((strcmp(g_CountryName[country], "Denmark") == 0)||
	(strcmp(g_CountryName[country], "Netherlands") == 0)||
	(strcmp(g_CountryName[country], "Portugal") == 0))
	{/*DD-MM-YYYY*/
		return DMY_WITH_HYPHEN;
	}
	else if((strcmp(g_CountryName[country], "Hungary") == 0)||
		(strcmp(g_CountryName[country], "Poland") == 0)||
		(strcmp(g_CountryName[country], "slovakia") == 0)||
		(strcmp(g_CountryName[country], "Slovenia") == 0)||
		(strcmp(g_CountryName[country], "Czech") == 0)||
		(strcmp(g_CountryName[country], "Sweden") == 0)||
		(strcmp(g_CountryName[country], "Finland") == 0))
	{/*YYYY-MM-DD*/
		return YMD_WITH_HYPHEN;
	}
	else
	{
		return YMD_WITH_SLASH;
	}
#else
#if UI_MIRROR_SUPPORT
	if(app_menu_mirror_get())
		return DMY_WITH_SLASH;
#endif
	/*YYYY/MM/DD*/
	return YMD_WITH_SLASH;
#endif
}

char *app_date_edit_format_get(void)
{
    int date_str_format = app_date_string_format();
    switch(date_str_format)
    {
        case YMD_WITH_SLASH: case YMD_WITH_HYPHEN: case YMD_WITH_DOT: return "edit_date_ymd";
        case DMY_WITH_SLASH: case DMY_WITH_HYPHEN: case DMY_WITH_DOT: return "edit_date_dmy";
        case MDY_WITH_SLASH: case MDY_WITH_HYPHEN: case MDY_WITH_DOT: return "edit_date_mdy";
        default:                                                      return "edit_date_ymd";
    }
    return "edit_date_ymd";
}

static char _get_date_edit_connector(int date_str_format)
{
    switch (date_str_format)
    {
        case YMD_WITH_SLASH:  case DMY_WITH_SLASH:  case MDY_WITH_SLASH:  return '/';
        case YMD_WITH_HYPHEN: case DMY_WITH_HYPHEN: case MDY_WITH_HYPHEN: return '-';
        case YMD_WITH_DOT:    case DMY_WITH_DOT:    case MDY_WITH_DOT:    return '.';
        default:                                                          return '/';
    }
    return '/';
}

char *app_common_data_time_edit_str(time_t time, uint32_t mode, char *buf, size_t len)
{
    char date_str[16] = {0};
    char time_str[16] = {0};
    struct tm temp_tm = {0};
    int date_str_format = YMD_WITH_SLASH;
    int t1, t2, t3;
    char c;
    char *date_format = NULL;

    if (mode == 0 || buf == NULL || len == 0)
        return NULL;

    localtime_r(&time, &temp_tm);
    date_str_format = app_date_string_format();
    c = _get_date_edit_connector(date_str_format);

    if (mode & LONG_DATE_EDIT)
    {
        switch (date_str_format)
        {
            case YMD_WITH_SLASH: case YMD_WITH_HYPHEN: case YMD_WITH_DOT:
                date_format = "%d%c%02d%c%02d";
                t1 = temp_tm.tm_year + 1900, t2 = temp_tm.tm_mon + 1, t3 = temp_tm.tm_mday;
                break;
            case DMY_WITH_SLASH: case DMY_WITH_HYPHEN: case DMY_WITH_DOT:
                date_format = "%02d%c%02d%c%d";
                t3 = temp_tm.tm_year + 1900, t2 = temp_tm.tm_mon + 1, t1 = temp_tm.tm_mday;
                break;
            case MDY_WITH_SLASH: case MDY_WITH_HYPHEN: case MDY_WITH_DOT:
                date_format = "%02d%c%02d%c%d";
                t3 = temp_tm.tm_year + 1900, t1 = temp_tm.tm_mon + 1, t2 = temp_tm.tm_mday;
                break;
            default:
                date_format = "%d%c%02d%c%02d";
                t1 = temp_tm.tm_year + 1900, t2 = temp_tm.tm_mon + 1, t3 = temp_tm.tm_mday;
                break;
        }
        sprintf(date_str, date_format, t1, c, t2, c, t3);
    }
    else if (mode & SHORT_DATE_EDIT)
    {
        date_format = "%02d%c%02d";
        switch (date_str_format)
        {
            case YMD_WITH_SLASH: case YMD_WITH_HYPHEN: case YMD_WITH_DOT:
                t1 = temp_tm.tm_mon + 1, t2 = temp_tm.tm_mday;
                break;
            case DMY_WITH_SLASH: case DMY_WITH_HYPHEN: case DMY_WITH_DOT:
                t2 = temp_tm.tm_mon + 1, t1 = temp_tm.tm_mday;
                break;
            case MDY_WITH_SLASH: case MDY_WITH_HYPHEN: case MDY_WITH_DOT:
                t1 = temp_tm.tm_mon + 1, t2 = temp_tm.tm_mday;
                break;
            default:
                t1 = temp_tm.tm_mon + 1, t2 = temp_tm.tm_mday;
                break;
        }
        sprintf(date_str, date_format, t1, c, t2);
    }

    if (mode & LONG_TIME_EDIT)
    {
        sprintf(time_str, "%02d:%02d:%02d", temp_tm.tm_hour, temp_tm.tm_min, temp_tm.tm_sec);
    }
    else if (mode & SHORT_TIME_EDIT)
    {
        sprintf(time_str, "%02d:%02d", temp_tm.tm_hour, temp_tm.tm_min);
    }

    if (((mode & LONG_DATE_EDIT) || (mode & SHORT_DATE_EDIT))
            && ((mode & LONG_TIME_EDIT) || (mode & SHORT_TIME_EDIT)))
    {
        if (mode & TIME_AHEAD_EDIT)
            snprintf(buf, len, "%s %s", time_str, date_str);
        else
            snprintf(buf, len, "%s %s", date_str, time_str);
    }
    else if ((mode & LONG_DATE_EDIT) || (mode & SHORT_DATE_EDIT))
    {
        snprintf(buf, len, "%s", date_str);
    }
    else
    {
        snprintf(buf, len, "%s", time_str);
    }
    return buf;
}

void app_time_ms_to_edit_str(uint64_t time_ms, char* str, size_t len)
{
    uint64_t time_s;
    int hour = 0;
    int sec = 0;
    int min = 0;
    if(str == NULL || len < 10)
        return;
    time_s = time_ms/1000;
    time_s += ((time_ms % 1000) == 0 ? 0 : 1);
    if(time_s < 0)
        time_s = 0;
    hour = time_s/3600;
    min = (time_s/60)%60;
    sec = time_s%60;
    memset(str,0,len);
    sprintf(str, "%02d:%02d:%02d", hour, min, sec);
}

/*please GxCore_Free the memory by yourself*/
char *app_time_to_short_date_edit_str(time_t time)
{
#define SHORT_DATE_STR_LEN 16
    char buf[SHORT_DATE_STR_LEN] = {0};
    if (app_common_data_time_edit_str(time, SHORT_DATE_EDIT, buf, SHORT_DATE_STR_LEN))
        return GxCore_Strdup(buf);
    return NULL;
}

/*please GxCore_Free the memory by yourself*/
char *app_time_to_date_edit_str(time_t time)
{
#define DATE_STR_LEN 16
    char buf[DATE_STR_LEN] = {0};
    if (app_common_data_time_edit_str(time, LONG_DATE_EDIT, buf, DATE_STR_LEN))
        return GxCore_Strdup(buf);
    return NULL;
}

/*please GxCore_Free the memory by yourself*/
char *app_time_to_hourmin_edit_str(time_t time)
{
#define HM_STR_LEN 16
    char buf[HM_STR_LEN] = {0};
    if (app_common_data_time_edit_str(time, SHORT_TIME_EDIT, buf, HM_STR_LEN))
        return GxCore_Strdup(buf);
    return NULL;
}

/*please GxCore_Free the memory by yourself*/
char *app_time_to_hms_str(time_t time)
{
#define HMS_STR_LEN 16
    char buf[HMS_STR_LEN] = {0};
    if (app_common_data_time_edit_str(time, LONG_TIME_EDIT, buf, HMS_STR_LEN))
        return GxCore_Strdup(buf);
    return NULL;
}

/*please GxCore_Free the memory by yourself*/
char *app_time_to_format_str(time_t time)
{
#define TM_STR_LEN 64
    char buf[TM_STR_LEN] = {0};
    if (app_common_data_time_edit_str(time, LONG_DATE_EDIT | LONG_TIME_EDIT, buf, TM_STR_LEN))
        return GxCore_Strdup(buf);
    return NULL;
}

status_t  app_edit_str_to_date_sec(char *date_str, time_t *res_sec)
{
    int i;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    char str1[5];
    char str2[5];
    char str3[5];
    char *p = NULL;
    char *year_str = NULL;
    char *month_str = NULL;
    char *day_str = NULL;
    status_t ret = GXCORE_ERROR;
    int date_str_format = YMD_WITH_SLASH;

    if(date_str == NULL)
    {
        return GXCORE_ERROR;
    }
    app_log_info("app_edit_str_to_date_sec date_str =%s \n",date_str);

    p = date_str;
    i = 0;
    memset(str1, 0, sizeof(str1));
    while((p != NULL) && (*p != '\0')  && ((*p != '/')&&(*p != '-')&&(*p != '.')))
    {
        str1[i++] = *p++;
    }
    p++; // '/' '-' '.'
    i = 0;
    memset(str2, 0, sizeof(str2));
    while((p != NULL) && (*p != '\0') && ((*p != '/')&&(*p != '-')&&(*p != '.')))
    {
        str2[i++] = *p++;
    }
    p++; // '/' '-' '.'
    i = 0;
    memset(str3, 0, sizeof(str3));
    while((p != NULL) && (*p != '\0'))
    {
        str3[i++] = *p++;
    }

    date_str_format = app_date_string_format();
    switch (date_str_format)
    {
        case YMD_WITH_SLASH: case YMD_WITH_HYPHEN: case YMD_WITH_DOT:
            year_str = str1; month_str = str2; day_str = str3;
            break;
        case DMY_WITH_SLASH: case DMY_WITH_HYPHEN: case DMY_WITH_DOT:
            day_str = str1; month_str = str2; year_str = str3;
            break;
        case MDY_WITH_SLASH: case MDY_WITH_HYPHEN: case MDY_WITH_DOT:
            month_str = str1; day_str = str2; year_str = str3;
            break;
        default:
            year_str = str1; month_str = str2; day_str = str3;
            break;
    }

    year = atoi(year_str);
    month = atoi(month_str);
    day = atoi(day_str);
    if(app_month_date_valid(year, month, day) == true)
    {
        *res_sec = app_mktime(year, month, day, 0, 0, 0);
        ret = GXCORE_SUCCESS;
    }
    return ret;
}

time_t app_edit_str_to_hourmin_sec(char *time_str)
{
	char hour_str[5];
	char min_str[5];
	time_t hour;
	time_t min;
	char *p = NULL;
	int i;

	if(time_str == NULL)
	{
		return 0;
	}
	p = time_str;
	i = 0;
	memset(hour_str, 0, sizeof(hour_str));
	while((p != NULL) && (*p != '\0')  && (*p != ':'))
	{
		hour_str[i++] = *p++;
	}
	p++; // ':'
	i = 0;
	memset(min_str, 0, sizeof(min_str));
	while((p != NULL) && (*p != '\0') )
	{
		min_str[i++] = *p++;
	}

	hour = atoi(hour_str);
	min = atoi(min_str);

	return hour * HOUR_TO_SEC + min *60;
}

float app_float_edit_str_to_value(const char *str)
{
	const char *cp;
	float data;
	char chHasDot=0;

    if(NULL == str)
        return 0;

	for (cp = str, data = 0; *cp != 0; ++cp)
	{
		if (*cp >= '0' && *cp <= '9')
		{
			data = data * 10 + *cp - '0';
			if (chHasDot)
			{
				chHasDot ++;
			}
		}
		else if(*cp == '.')
		{
			chHasDot = 1;
		}
		else
			break;
	}
    if(chHasDot >0)
    {
        chHasDot -= 1;
        while(chHasDot--)
        {
            data /=10;
        }
    }
	return data;
}

void app_progbar_update_value(const char *widget, int value)
{
#define PROGBAR_F_L2     "s_progress3_l"
#define PROGBAR_F_M2     "s_progress3_m"
#define PROGBAR_F_R2     "s_progress3_r"

	GUI_SetProperty(widget, "value", &value);
	GUI_SetProperty(widget, "fore_image_l", PROGBAR_F_L2);
	GUI_SetProperty(widget, "fore_image_m", PROGBAR_F_M2);
    GUI_SetProperty(widget, "fore_image_r", PROGBAR_F_R2);

}

uint32_t app_snr_progbar_update(uint32_t tuner, const char *wgt_bar, const char *wgt_val)
{
#define PROGBAR_SNR_LOCKED_L    "s_progress3_l_green"
#define PROGBAR_SNR_LOCKED_M    "s_progress3_m_green"
#define PROGBAR_SNR_LOCKED_R    "s_progress3_r_green"

#define PROGBAR_SNR_UNLOCKED_L  "s_progress3_l_red"
#define PROGBAR_SNR_UNLOCKED_M  "s_progress3_m_red"
#define PROGBAR_SNR_UNLOCKED_R  "s_progress3_r_red"

	char buffer[20] = {0};
	uint32_t value = 0;
	uint32_t ber_value = 0;
	uint32_t E_param = 0;

	app_ioctl(tuner, FRONTEND_ERRORRATE_GET, &E_param);
	if(GxFrontend_QueryStatus(tuner) == 1)
	{
		value = (E_param>>8)&(0xfff);
		E_param = E_param&0xf;
		ber_value = 12*E_param;
		if((ber_value > 100) || ((E_param == 0) && (value == 0)))
			ber_value = 100;
	}
	else
	{
		value = 100;
		E_param = 0;
		ber_value = 10;
	}
	snprintf(buffer, sizeof(buffer), "%d.%02dE-%02d", value/100, value%100, E_param);

	if(ber_value < 48)
	{
		GUI_SetProperty(wgt_bar, "fore_image_l", PROGBAR_SNR_UNLOCKED_L);
		GUI_SetProperty(wgt_bar, "fore_image_m", PROGBAR_SNR_UNLOCKED_M);
		GUI_SetProperty(wgt_bar, "fore_image_r", PROGBAR_SNR_UNLOCKED_M);
	}
	else
	{
		GUI_SetProperty(wgt_bar, "fore_image_l", PROGBAR_SNR_LOCKED_L);
		GUI_SetProperty(wgt_bar, "fore_image_m", PROGBAR_SNR_LOCKED_M);
		GUI_SetProperty(wgt_bar, "fore_image_r", PROGBAR_SNR_LOCKED_R);

	}

	GUI_SetProperty(wgt_bar, "value", &ber_value);
	GUI_SetProperty(wgt_val, "string", buffer);

	return ber_value;
}

status_t app_signal_progbar_update(uint32_t tuner, SignalBarWidget *bar, SignalValue *val)
{
#define PROGBAR_STRENGTH_L  "s_progress3_l"
#define PROGBAR_STRENGTH_M  "s_progress3_m"
#define PROGBAR_STRENGTH_R  "s_progress3_r"

#define PROGBAR_QUALITY_L   "s_progress3_l_green"
#define PROGBAR_QUALITY_M   "s_progress3_m_green"
#define PROGBAR_QUALITY_R   "s_progress3_r_green"

#define PROGBAR_UNLOCKED_L  "s_progress3_l_red"
#define PROGBAR_UNLOCKED_M  "s_progress3_m_red"
#define PROGBAR_UNLOCKED_R  "s_progress3_r_red"

    AppFrontend_LockState lock;

	if((bar == NULL) || (bar->strength_bar == NULL) || (bar->quality_bar== NULL))
		return GXCORE_ERROR;

    app_ioctl(tuner, FRONTEND_LOCK_STATE_GET, &lock);
	if(lock == FRONTEND_LOCKED) //锁定检查
	{
		GUI_SetProperty(bar->strength_bar, "fore_image_l", PROGBAR_STRENGTH_L);
		GUI_SetProperty(bar->strength_bar, "fore_image_m", PROGBAR_STRENGTH_M);
		GUI_SetProperty(bar->strength_bar, "fore_image_r", PROGBAR_STRENGTH_R);

		GUI_SetProperty(bar->quality_bar, "fore_image_l", PROGBAR_QUALITY_L);
		GUI_SetProperty(bar->quality_bar, "fore_image_m", PROGBAR_QUALITY_M);
		GUI_SetProperty(bar->quality_bar, "fore_image_r", PROGBAR_QUALITY_R);
	}
	else
	{
		GUI_SetProperty(bar->strength_bar, "fore_image_l", PROGBAR_UNLOCKED_L);
		GUI_SetProperty(bar->strength_bar, "fore_image_m", PROGBAR_UNLOCKED_M);
		GUI_SetProperty(bar->strength_bar, "fore_image_r", PROGBAR_UNLOCKED_R);

		GUI_SetProperty(bar->quality_bar, "fore_image_l", PROGBAR_UNLOCKED_L);
		GUI_SetProperty(bar->quality_bar, "fore_image_m", PROGBAR_UNLOCKED_M);
		GUI_SetProperty(bar->quality_bar, "fore_image_r", PROGBAR_UNLOCKED_R);
	}

	// strength value
	GUI_SetProperty(bar->strength_bar, "value", &(val->strength_val));

	// progbar quality
	GUI_SetProperty(bar->quality_bar, "value", &(val->quality_val));

	return GXCORE_SUCCESS;
}

void app_free_dir_ent(GxDirent **ents, int *nents)
{
    GxCore_FreeDir(*ents, *nents);
    *ents = NULL;
	*nents = 0;
}

uint32_t app_hexstr2value(uint8_t* string)
{
    int ret = 0;
    char *pstr = NULL;

    if(NULL == string)
    {
        return 0;
    }

    //hex
    if(strlen((char*)string) > 2)
    {
        if('0' == string[0] && 'x' == string[1])
        {
            pstr = (char*)&string[2];
            while(*pstr)
            {
                if (*pstr>='0' && *pstr<='9')
                {
                    ret=ret * 16 + ((int)*pstr-'0');
                }
                else
                {
                    if (*pstr>='A' && *pstr<='F')
                    {
                        ret=ret * 16 +((int)*pstr-'A'+10);
                    }
                    else if (*pstr>='a' && *pstr<='f')
                    {
                        ret=ret *16 + ((int)*pstr-'a'+10);
                    }
                }
                pstr++;
            }
            return ret;
        }
    }

    //dec
    pstr = (char*)string;
    while(*pstr)
    {
        if (*pstr>='0' && *pstr<='9')
        {
            ret=ret * 10 + ((int)*pstr-'0');
        }
        else
        {
            app_log_error("[panel keymap]_hexstr2value error '%s'\n", string);
            return 0;
        }
        pstr++;
    }

    return (ret);
}

uint32_t app_bcd2value(uint32_t bcd)
{
	uint32_t ret = 0;
	uint8_t x = 0;
	uint8_t i = 0;

	while(bcd > 0)
	{
		x = bcd & 0xf;
		ret = x * pow(10, i) + ret;
		bcd >>= 4;
		i++;
	}

	return ret;
}

void app_getvalue(const char* url, const char* item, int32_t *retVal)
{
#define VAL_MAX 32

	char* ptr1 = NULL;
	char* ptr2 = NULL;
	char value[VAL_MAX];

	if(url && item){
		ptr1 = strstr(url,item);
		if(ptr1){
			ptr1 += (strlen(item)+1);
			ptr2 = strstr(ptr1,"&");
			if(ptr2 == NULL)
			{
				ptr2 = strstr(ptr1," ");
				if(ptr2 == NULL)
					ptr2 =(char*)(url+ strlen(url));
			}
			if(ptr2-ptr1 > VAL_MAX)
			{
				app_log_error("[ERROR]:(%s)-->(%s) too long !...\n",__func__, item);
				return ;
			}
			memcpy(value,ptr1,ptr2-ptr1);
			value[ptr2-ptr1] = '\0';
			*retVal = atoi((char*)value);
			return ;
		}
	}
	*retVal = -1;
}

uint32_t app_key_full_code(uint16_t key_code)
{
	uint8_t syscode;
	uint8_t syscode_r;
	uint8_t usrcode;
	uint8_t usrcode_r;
	uint32_t full_code;

	if(key_code == 0)
		return 0;

	syscode = (key_code >> 8) & 0xff;
	syscode_r = ~syscode;
	usrcode = key_code & 0xff;
	usrcode_r = ~usrcode;

	full_code = syscode << 24;
	full_code |= syscode_r << 16;
	full_code |= usrcode << 8;
	full_code |= usrcode_r;

	return full_code;
}

int generatorMac(char *mac)
{
    int i;
    char chipsn[8] = {0};
    char invalid_chipsn[8] = {0};
    unsigned char hash[SHA_DIGEST_LENGTH] = {0};
    unsigned char buf[6] = {0};

    if(NULL == mac || strlen(mac) < 15) {
        return -1;
    }

    if(app_chip_sn_get(chipsn, 8) < 0) {
        return -1;
    }

    if(0 == memcmp((void *)chipsn, (void *)invalid_chipsn, sizeof(chipsn))) {
        srand((int)time(0));
        for(i = 0; i < sizeof(chipsn); i++) {
            chipsn[i] = rand()%0xff;
        }

        if(SHA1((const unsigned char*)chipsn, 8, hash) < 0) {
            return -1;
        }
        memcpy(buf, hash, sizeof(buf));
    }
    else {
        if(GxChip_GetChipId2Mac(buf, sizeof(buf), 1) <= 0)
            return -1;
    }

	sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x",
            buf[0],
            buf[1],
            buf[2],
            buf[3],
            buf[4],
            buf[5]);

    return 0;
}

unsigned int app_get_tick_time(void)
{
	GxTime time;
	unsigned int tick = 0;

	GxCore_GetTickTime(&time);
	tick = time.seconds*1000+time.microsecs/1000;

	return tick;
}

char *netupg_ftp_get_authstr(char *username, char *password)
{
    unsigned int auth_len = 0;
    char *auth_str = NULL;

    if(username&&(strlen(username)>0))
    {
        if(password&&(strlen(password)>0))
        {
            auth_len = strlen(username)+strlen(password)+3;
            auth_str = (char *)GxCore_Malloc(auth_len);
            sprintf(auth_str, "%s:%s@", username, password);
        }
        else
        {
            auth_len = strlen(username)+2;
            auth_str = (char *)GxCore_Malloc(auth_len);
            sprintf(auth_str, "%s@", username);
        }
    }

    return auth_str;
}

int app_upgrade_crc_check_buf(char *p_data, int size)
{
	struct partition tbl;

    if(p_data == NULL)
    {
		app_log_error("app_upgrade_crc_check_buf p_data is NULL.\n");
		return 0;
    }
	if(0 == GxCore_MEM_Partition_Init(p_data,&tbl))
	{
		if(tbl.crc32_enable > 0)
		{
			int i = 0;
			unsigned int new_crc;

			for(i = 0; i < tbl.count; i++)
			{
				if(tbl.tables[i].crc32_enable != 0)
				{
					if(tbl.tables[i].start_addr+tbl.tables[i].total_size >	size)
					{
						app_log_error("(1)------flash size is larger.\n");
						return 0;
					}

					//genflash don't support crc check for partition [TABLE]
					if(strncasecmp(tbl.tables[i].name, PARTITION_TABLE, strlen(PARTITION_TABLE)) == 0)
					{
						continue;
					}

					new_crc = GxCore_Crc32(0, (unsigned char *)(p_data + tbl.tables[i].start_addr), tbl.tables[i].used_size);
					app_log_debug("------0x%X----------------------0x%X-----------\n\n", new_crc, tbl.tables[i].crc_verify);
					if(new_crc != tbl.tables[i].crc_verify)
					{
						return 0;
					}
				}
			}
		}
		return 1;
	}
	return 0;
}

uint32_t app_get_flash_size(void)
{
    int i;
    GxPartitionTable *table = NULL;
    uint32_t flash_total_size = 0;

    table = GxCore_PartitionFlashInit();
    if (table == NULL)
    {
        app_log_info("app get flash size partition init err \n");
        return 0 ;
    }

    for (i = 0; i < table->count; i++)
    {
        flash_total_size += table->tables[i].total_size;
    }

    return flash_total_size;
}


uint32_t app_get_ddr_size(void)
{
    struct Gx_Mem_Info info;

    memset(&info, 0, sizeof(struct Gx_Mem_Info));
    GxCore_MemInfoGet("allmem", &info);
    return info.size;
}

int app_enddialog_after_wnd(char* wnd)
{
    if(GUI_CheckDialog(WND_POP_BOOK) == GXCORE_SUCCESS)
    {
        GUI_EndDialog(wnd);
        book_popdlg_recreate();
    }
    else
    {
        GUI_EndDialog(wnd);
    }

    return 0;
}


int app_enddialog_wnd(char* wnd)
{
    char* p_win = NULL;
    int length =0 ;
    length = 8+strlen(wnd);
    p_win = (char*)GxCore_Malloc(length);

    if(p_win == NULL)
    {
        app_log_error("%s %dmalloc failed!!!!!!!!!!!\n",__FILE__,__LINE__);
        while(1);
    }
    sprintf(p_win,"after %s",wnd);
    if(GUI_CheckDialog(WND_POP_BOOK) == GXCORE_SUCCESS)
    {
        GUI_EndDialog(p_win);
        GUI_EndDialog(wnd);
        book_popdlg_recreate();
    }
    else
    {
        GUI_EndDialog(p_win);
        GUI_EndDialog(wnd);
    }
    GxCore_Free(p_win);
    return 0;
}

int  app_strcat_gui_str(char* prefix, char* str, char* suffix, char *p)
{
    char* i18n_str;

    memset(p,0,strlen(p));
    if(prefix != NULL)
    {
        strcat(p,prefix);
    }
    if(str != NULL)
    {
        i18n_str =(char*)GXGDI_I18N_String(str);
        strcat(p,i18n_str);
    }
    if(suffix != NULL)
    {
        strcat(p,suffix);
    }

    return 0;
}

void app_set_widget_string(const char *wgt, const char *str)
{
    char *tmp_str = NULL;

    if (str == NULL || strlen(str) == 0)
    {
        GUI_SetProperty(wgt, "string", STR_ID_BLANK);
    }
    else
    {
        GUI_GetProperty(wgt, "string", &tmp_str);
        if((tmp_str == NULL)
                || (strcmp(tmp_str, str) != 0))
        {
            GUI_SetProperty(wgt, "string", (void*)str);
        }
    }
}

void app_show_sys_time_and_date(const char *time_wgt, const char *date_wgt)
{
    char sys_time_str[24] = {0};

    g_AppTime.time_get(&g_AppTime, sys_time_str, sizeof(sys_time_str));
    if (strlen(sys_time_str) > 0)
    {
        app_set_widget_string(time_wgt, sys_time_str + MIN_SEC_OFFSET);
        sys_time_str[MIN_SEC_OFFSET] = '\0';
        app_set_widget_string(date_wgt, sys_time_str);
    }
}

void app_set_window_back_ground(const char *wnd, const char *key, bool flush)
{
    if(!key) key = JPG_BG;
    GUI_SetProperty(wnd, "back_ground", (void*)key);
    if (flush)
        GUI_SetInterface("flush", NULL);
    GUI_SetInterface("video_disable", NULL);
}

void app_update_window_back_ground(const char *wnd, const char *key, bool flush)
{
    if(!key) key = "null";
    GUI_SetProperty(wnd, "back_ground", (void*)key);
    if (flush)
        GUI_SetInterface("flush", NULL);
}

void app_unset_window_back_ground(const char *wnd, bool free_space, bool flush)
{
    GUI_SetProperty(wnd, "back_ground", "null");
    if (flush)
        GUI_SetInterface("flush", NULL);
    GUI_SetInterface("video_enable", NULL);
    if (free_space)
    {
#if MENCENT_FREEE_SPACE
        GUI_SetInterface("free_space", "fragment|spp");
        GxCore_HwCleanCache();
#endif
    }
}

void  app_player_femem_init(void)
{
#if (defined GEMINI || defined CYGNUS)
    int dev = -1;
	memset(&femem_info, 0, sizeof(femem_info));
	strcpy(femem_info.name, "femem");
	dev = GxAvdev_CreateDevice(0);
    if(dev > 0)
    {
        GxAVMemHoleGetInfo(dev, &femem_info);
        if(femem_info.start == 0) {
            app_log_error("GxAVMemHoleGetInfo femem = NULL !\n");
            GxAvdev_DestroyDevice(dev);
            return;
        }

        _festart = (unsigned int)GxCore_Map(dev, femem_info.start, femem_info.size);
        GxAvdev_DestroyDevice(dev);
    }
#endif
	return;
}

#if (defined GEMINI || defined CYGNUS)
static void app_player_femem_register(void)
{
	GxPlayer_HeapRegister(_festart, femem_info.size);
}

static void app_player_femem_unregister(void)
{
	GxPlayer_HeapUnregister(_festart);
}
#endif

#define CYGNUS_PUBLIC_ID 0x6705
#define CYGNUS_CHIPNAME_6705   "6705-"
#define CYGNUS_CHIPNAME_6706S5 "6706S5-"
#define CYGNUS_CHIPNAME_6706H5 "6706H5-"

void app_system_performace_dynamic_change(SysDynPlayMode mode)
{
#ifdef CYGNUS
    uint32_t chipid = 0;
    char chipname[16] = {0};
    SystemPerformanceMode p_mode = GX_SYSTEM_PERF_MODE_DEFAULT;

    chipid = app_chip_id_get();
    if(app_chip_name_get(chipname, 16) < 0)
        return;
    if(chipid != CYGNUS_PUBLIC_ID)
        return;
    //DVB play
    if(DVB_PLAY_MODE == mode)
    {
        if(strstr(chipname, CYGNUS_CHIPNAME_6705))
            p_mode = GX_SYSTEM_PERF_MODE_CPU_ENHANCED;
        else if(strstr(chipname, CYGNUS_CHIPNAME_6706S5) || strstr(chipname, CYGNUS_CHIPNAME_6706H5))
            p_mode = GX_SYSTEM_PERF_MODE_DEFAULT;
    }
    else
    {
        if(strstr(chipname, CYGNUS_CHIPNAME_6705) || strstr(chipname, CYGNUS_CHIPNAME_6706S5) || strstr(chipname, CYGNUS_CHIPNAME_6706H5))
            p_mode = GX_SYSTEM_PERF_MODE_CPU_ENHANCED;
    }

    if(p_mode != GxCore_GetSystemPerformanceMode())
        GxCore_SetSystemPerformanceMode(p_mode);
#endif
}

void app_player_femem_control(bool use_femem)
{
#if (defined GEMINI || defined CYGNUS)
    GxBusPmDataSat sat = {0};
    app_get_sat_params(&sat, DEMOD_TYPE_DVBT);

    if(use_femem)
	{
		GxFrontend_FememCtrl(sat.tuner, use_femem);
		app_player_femem_register();
	}
	else
	{
		app_player_femem_unregister();
		GxFrontend_FememCtrl(sat.tuner, use_femem);
	}
#endif
	return;
}
#if !OTT_SUPPORT
void app_all_frontend_monitor_control(AppFrontend_Monitor monitor, bool clean_para)
{
    uint32_t i = 0;

    for(i = 0; i < APP_MULTI_DEMOD_NUM; i++)
    {
        app_ioctl(i, FRONTEND_MONITOR_SET, &monitor);
        if(FRONTEND_MONITOR_ON == monitor)
            app_log_flow("[%s]>> Start tuner%d monitor!\n", __func__, i);
        else
            app_log_flow("[%s]>> Stop tuner%d monitor!\n", __func__, i);
        if(true == clean_para)
            GxFrontend_CleanRecordPara(i);
    }
#if NET_SEARCH_SUPPORT
    app_ioctl(i, FRONTEND_MONITOR_SET, &monitor);
    if(true == clean_para)
        GxFrontend_CleanRecordPara(i);
#endif
    return;
}

void app_only_cur_frontend_monitor_start(uint32_t cur_tuner)
{
    uint32_t i = 0;
    AppFrontend_Monitor monitor = FRONTEND_MONITOR_ON;

    for(i = 0; i < APP_MULTI_DEMOD_NUM; i++)
    {
        if(cur_tuner == i)
        {
            monitor = FRONTEND_MONITOR_ON;
            app_log_flow("[%s]>> Start tuner%d monitor!\n", __func__, i);
        }
        else
        {
            monitor = FRONTEND_MONITOR_OFF;
            app_log_flow("[%s]>> Stop tuner%d monitor!\n", __func__, i);
        }

        app_ioctl(i, FRONTEND_MONITOR_SET, &monitor);
    }
    return;
}

int app_get_sat_params(GxBusPmDataSat* sat, unsigned char demod_type)
{
    unsigned char demod_type_list[APP_MULTI_DEMOD_NUM] = APP_FRONTEND_MODE_CONFIG;
    unsigned char i = 0;

    if(sat == NULL)
        return -1;

    sat->type = GXBUS_PM_SAT_S;
    sat->tuner = 0;
    for(i=0; i<APP_MULTI_DEMOD_NUM; i++)
    {
        if(demod_type == demod_type_list[i])
        {
#if (DEMOD_DVB_C > 0)
        if(demod_type == DEMOD_TYPE_DVBC)
            sat->type = GXBUS_PM_SAT_C;
#endif
#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
        if(demod_type == DEMOD_TYPE_DVBT)
            sat->type = GXBUS_PM_SAT_DVBT2;
#endif
#if (DEMOD_DTMB > 0)
        if(demod_type == DEMOD_TYPE_DTMB)
            sat->type = GXBUS_PM_SAT_DTMB;
#endif
            sat->tuner = i;
            break;
        }
    }
    return 0;
}
#endif

void app_set_fre_zero(char *url)
{
    static char temp_url[PLAYER_URL_LONG+1] = {0};

    if(NULL == url)
        return;

    memset(temp_url, 0, sizeof(temp_url));

    char *p = url;

    while(*p != '\0')
    {
        if(0 == strncmp(p, GX_URL_KEY_FRE, 3))
        {
            memcpy(temp_url, url, p - url);
            strcat(temp_url, "fre:0");
            p = strchr(p, '&');
            if(p != NULL)
                strcat(temp_url, p);
            memcpy(url, temp_url, PLAYER_URL_LONG);
            break;
        }
        p++;
    }
    return;
}

#if FM_SUPPORT
int app_get_fm_tuner_id(void)
{
    AppFrontend_Config cfg = {0};
    app_ioctl(DEMOD_TYPE_FM, FRONTEND_CONFIG_GET, &cfg);
    return cfg.tuner;
}
#endif

static int _reset_playid_after_del_by_sat(uint32_t sat_id)
{
    GxMsgProperty_NodeByIdGet sat_get = {0};
    GxMsgProperty_NodeModify node_modify;
    GxMsgProperty_NodeByIdGet prog_get = {0};
    uint32_t tv_id = 0;
    uint32_t radio_id = 0;

    GxBus_ConfigGetInt(ALL_TV_KEY, (int32_t *)&tv_id, ALL_TV_ID);
    GxBus_ConfigGetInt(ALL_RADIO_KEY, (int32_t *)&radio_id, ALL_RADIO_ID);

    prog_get.node_type = NODE_PROG;
    prog_get.id = tv_id;
    if(app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &prog_get) == GXCORE_ERROR)
    {
        GxBus_ConfigSetInt(ALL_TV_KEY, ALL_TV_ID);
    }
    prog_get.node_type = NODE_PROG;
    prog_get.id = radio_id;
    if(app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &prog_get) == GXCORE_ERROR)
    {
        GxBus_ConfigSetInt(ALL_RADIO_KEY, ALL_RADIO_ID);
    }
    sat_get.node_type = NODE_SAT;
    sat_get.id = sat_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat_get);
    memset(&node_modify, 0, sizeof(GxMsgProperty_NodeModify));
    node_modify.node_type = NODE_SAT;
    memcpy(&node_modify.sat_data, &sat_get.sat_data, sizeof(GxBusPmDataSat));
    node_modify.sat_data.cur_tv = 0;
    node_modify.sat_data.cur_radio = 0;
    app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);
    return 0;
}

int app_all_channels_del_by_sat_id(uint32_t sat_id)
{
    uint32_t num = 0;
    GxBusPmViewInfo view_info_use;
    memset(&view_info_use, 0, sizeof(GxBusPmViewInfo));

    if(0 == sat_id)
        return 0;

    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info_use)));
    {
        view_info_use.group_mode = GROUP_MODE_SAT;
        view_info_use.stream_type = GXBUS_PM_PROG_ALL;
        view_info_use.sat_id = sat_id;
        view_info_use.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
        num = GxBus_PmProgNumGetByViewInfo(&view_info_use);
        if(0 == num)
        {
            app_log_error("-----no channels in sat(id = %d)-----\n", sat_id);
            return 0;
        }

        uint32_t *p_id_array = NULL;

        p_id_array = (uint32_t *)GxCore_Malloc(num * sizeof(uint32_t)); //need GxCore_Free

        if(NULL == p_id_array)
        {
            app_log_error("-----delete all channels in sat(id = %d), malloc failed-----\n", sat_id);
            return -1;
        }
        memset(p_id_array,0,num * sizeof(uint32_t));
        GxBus_PmProgIdArryGetByViewInfo(&view_info_use, num, p_id_array);
        app_log_debug("-----delete all channels in sat(id = %d)-----\n", sat_id);

        GxMsgProperty_NodeDelete node_del = {0};

        node_del.node_type = NODE_PROG;
        node_del.id_array = p_id_array;
        node_del.num = num;
        app_send_msg_exec(GXMSG_PM_NODE_DELETE, &node_del);
        GxCore_Free(p_id_array);
        p_id_array = NULL;
    }
    _reset_playid_after_del_by_sat(sat_id);
    return 0;
}

uint8_t app_all_tp_del_by_sat_id(uint16_t sat_id)
{
   GxMsgProperty_NodeNumGet node_num_get = {0};
    GxMsgProperty_NodeByPosGet node_get = {0};
    GxMsgProperty_NodeDelete node_del = {0};
    uint32_t*p_id_array = NULL;
    uint32_t ret = 0;
    uint32_t i=0;

    if(0 == sat_id)
        return 0;

    node_num_get.node_type = NODE_TP;
    node_num_get.sat_id = sat_id;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));

	if(0 == node_num_get.node_num)
	{
		app_log_error("=======no tp to del\n");
		return ret;
	}

    p_id_array = GxCore_Malloc(node_num_get.node_num*sizeof(uint32_t));//need GxCore_Free
	if(NULL == p_id_array)
	{
	    app_log_error("=======can not malloc id_array\n");
		return ret;
	}

	for(i = 0; i < node_num_get.node_num; i++)
	{
		node_get.node_type = NODE_TP;
		node_get.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_get);
		p_id_array[i] = node_get.tp_data.id;
	}

	node_del.node_type = NODE_TP;
	node_del.id_array = p_id_array;
	node_del.num = node_num_get.node_num;
	app_send_msg_exec(GXMSG_PM_NODE_DELETE, &node_del);
	GxCore_Free(p_id_array);
    _reset_playid_after_del_by_sat(sat_id);
	return ret;
}

uint8_t app_del_noprog_tp_by_sat_id(uint16_t sat_id)
{
    int16_t tp_num = 0;
    uint16_t del_count = 0;
    uint32_t*p_id_array = NULL;
    GxBusPmDataTP  Tp={0};
    uint32_t i=0;

    if(0 == sat_id)
        return 1;
    tp_num = GxBus_PmTpNumGetBySat(sat_id);
    if(0 == tp_num)
    {
        return 1;
    }

    p_id_array = GxCore_Malloc(tp_num*sizeof(uint32_t));//need GxCore_Free
    if(NULL == p_id_array)
    {
        app_log_error("=======can not malloc id_array\n");
        return 0;
    }

    for(i = 0; i < tp_num; i++)
    {
        GxBus_PmTpGetByPosInSat(i,1,&Tp);
        if(GxBus_PmProgNumGetByTpId(Tp.id) == 0)
        {
            p_id_array[del_count] = Tp.id;
            del_count++;
        }
    }
    if(del_count != 0)
        GxBus_PmTpDelete(p_id_array,del_count);
    GxCore_Free(p_id_array);
    return 1;
}

/*
 *此函数只用于数据库中某种卫星类型只有一颗的情况,如多颗s的卫星就不支持
 * */
uint32_t app_sat_id_get_by_type(GxBusPmDataSatType type)
{
    uint32_t num = 0;
    int32_t i = 0;
    bool need_add_sat = true;
    GxBusPmDataSat sat = {0};

    if(GXBUS_PM_SAT_S == type)
        return 0;

    num = GxBus_PmSatNumGet();
    if(num > 0)
    {
        for(i = 0; i < num; i++)
        {
            memset(&sat, 0, sizeof(GxBusPmDataSat));

            if(1 == GxBus_PmSatsGetByPos(i, 1, &sat))
            {
                if(type == sat.type)
                {
                    need_add_sat = false;
                    break;
                }
            }
            else
            {
                app_log_info("[Search] this is not what you want\n");
            }
        }
    }

    if(true == need_add_sat)
    {
#if DEMOD_DVB_T || DEMOD_ISDBT
        if(GXBUS_PM_SAT_T == type || GXBUS_PM_SAT_DVBT2 == type)
            app_get_sat_params(&sat, DEMOD_TYPE_DVBT);
#endif
#if DEMOD_DVB_C
        if(GXBUS_PM_SAT_C == type)
            app_get_sat_params(&sat, DEMOD_TYPE_DVBC);
#endif
#if DEMOD_DTMB
        if(GXBUS_PM_SAT_DTMB == type)
            app_get_sat_params(&sat, DEMOD_TYPE_DTMB);
#endif

        sat.type = type;
        if(GXCORE_ERROR == GxBus_PmSatAdd(&sat))
        {
            app_log_error("%s GxBus_PmSatAdd Error!!!\n", __func__);
            return 0;
        }
        GxBus_PmSync(GXBUS_PM_SYNC_SAT);
    }

    return sat.id;
}

int app_channel_num_check_by_sat_id(uint32_t sat_id, uint32_t *TvNum, uint32_t *RadioNum)
{
    GxBusPmViewInfo view_info;

    if(0 == sat_id)
        return -1;

    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));

    view_info.group_mode = GROUP_MODE_SAT;
    view_info.sat_id = sat_id ;
    view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
    view_info.taxis_mode = TAXIS_MODE_NON;
    view_info.stream_type = GXBUS_PM_PROG_TV;
    *TvNum =  GxBus_PmProgNumGetByViewInfo(&view_info);
    //check radio
    view_info.stream_type = GXBUS_PM_PROG_RADIO;
    *RadioNum =  GxBus_PmProgNumGetByViewInfo(&view_info);
    return 0;
}

int app_all_channels_num_check(uint32_t *TvNum, uint32_t *RadioNum)
{
    GxBusPmViewInfo view_info;

    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));

    view_info.group_mode = GROUP_MODE_ALL;//all

    view_info.taxis_mode = TAXIS_MODE_NON;
    view_info.stream_type = GXBUS_PM_PROG_TV;
    view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
    *TvNum = GxBus_PmProgNumGetByViewInfo(&view_info);
    //check radio
    view_info.stream_type = GXBUS_PM_PROG_RADIO;
    *RadioNum = GxBus_PmProgNumGetByViewInfo(&view_info);
    return 0;
}

status_t app_pop_net_connect_check(void)
{
    status_t ret = GXCORE_ERROR;
#if NETWORK_SUPPORT
    if(IF_STATE_CONNECTED == app_network_get_cur_dev_and_state(NULL))
    {
        ret = GXCORE_SUCCESS;
    }
    else
    {
         PopDlg pop;
         memset(&pop, 0, sizeof(PopDlg));
         pop.type = POP_TYPE_OK;
         pop.format = POP_FORMAT_DLG;
         pop.mode = POP_MODE_UNBLOCK;
         pop.pos.x = APP_POP_DLG_POS_X;
         pop.pos.y = APP_POP_DLG_POS_Y;
         pop.str = STR_ID_CONNECT_TIP;
         popdlg_create(&pop);
         ret = GXCORE_ERROR;
    }
#endif

    return ret;
}

char *app_channel_num_str_get(GxMsgProperty_NodeByPosGet *node_prog, int32_t pos)
{
	static char buffer[8]={0};
	GxMsgProperty_NodeByPosGet node;

	if (!node_prog)
	{
		node.node_type = NODE_PROG;
		node.pos = pos;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
		node_prog = &node;
	}
#if TKGS_SUPPORT
extern GxTkgsOperatingMode app_tkgs_get_operatemode(void);
	if(GX_TKGS_OFF != app_tkgs_get_operatemode())
	{
		AppProgExtInfo prog_ext_info = {{0}};
		if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(node_prog->prog_data.id, sizeof(AppProgExtInfo), &prog_ext_info))
			snprintf(buffer, sizeof(buffer), "%d", prog_ext_info.tkgs.logicnum);
		else
			snprintf(buffer, sizeof(buffer), "%d", pos + 1);
	}
	else
	{
		snprintf(buffer, sizeof(buffer), "%d", pos + 1);
	}
#elif LCN_SUPPORT
	if(app_lcn_config_get() == 1)
	{
		AppProgExtInfo prog_ext_info = {0};
		if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(node_prog->prog_data.id, sizeof(AppProgExtInfo), &prog_ext_info))
			snprintf(buffer, sizeof(buffer), "%d", prog_ext_info.logicnum);
		else
			snprintf(buffer, sizeof(buffer), "%d", pos + 1);
	}
	else
	{
		snprintf(buffer, sizeof(buffer), "%d", pos + 1);
	}
#else
	snprintf(buffer, sizeof(buffer), "%d", pos + 1);
#endif


	return buffer;
}

uint32_t app_channel_panel_led_get(uint32_t prog_id)
{
    uint32_t panel_led = 0;

    if(0 == prog_id)
        return 0;

    // error happend, play No. larger than the total number
    if(g_AppPlayOps.normal_play.play_count >= g_AppPlayOps.normal_play.play_total)
    {
        app_log_info("[play] play count larger than total!!!!!\n");
        g_AppPlayOps.normal_play.play_count = 0;
        panel_led = 0;
    }
    else
    {
#if LCN_SUPPORT
        AppProgExtInfo prog_ext_info;

        memset(&prog_ext_info, 0, sizeof(AppProgExtInfo));
        if(app_lcn_config_get() == 1)
        {
            if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(prog_id, sizeof(AppProgExtInfo),&prog_ext_info))
                panel_led = prog_ext_info.logicnum;
            else
                panel_led = g_AppPlayOps.normal_play.play_count + 1;
        }
        else
        {
            panel_led = g_AppPlayOps.normal_play.play_count + 1;
        }
#else
        panel_led = g_AppPlayOps.normal_play.play_count + 1;
#endif
    }

    return panel_led;
}

uint32_t app_tick_time_msec_get(void)
{
    GxTime time;
    uint32_t tick = 0;

    GxCore_GetTickTime(&time); // ecos cyg_time_get_ms
    tick = time.seconds*1000 + time.microsecs/1000;
    return tick;
}

uint32_t app_tick_time_sec_get(void)
{
    GxTime time;
    uint32_t tick = 0;

    GxCore_GetTickTime(&time);
    tick = time.seconds;
    return tick;
}

struct partition_info *app_get_partition(struct partition *table, const char* name)
{
    int i = 0;
    if (!table || !name)
        return NULL;

    for (i = 0; i < table->count; i++) {
        if (strncasecmp(table->tables[i].name, name, FLASH_PARTITION_NAME_SIZE) == 0)
            return &table->tables[i];
    }

    return NULL;
}

#define APP_OEM_PATH    WORK_PATH"theme/default.xml"
#define OEM_STR         (xmlChar*)"default"
#define ITEM_STR        (xmlChar*)"item"
#define GROUP_NAME      (xmlChar*)"name"
#define ITEM_DISP       (xmlChar*)"display"
#define ITEM_VAL        (xmlChar*)"value"
#define LIST_VAL        (xmlChar*)"value"
#define BUF_LEN         (256)

/*************** xml parser start ****************/
static status_t info_get_list_item(GXxmlNodePtr xml_item, xmlChar *name, char * buf, size_t size)
{
    GXxmlNodePtr item = NULL;
    xmlChar *value = NULL;
    xmlChar *iname = NULL;

    item =  xml_item->childs;
    while (item != NULL)
    {
        iname = GXxmlGetProp(item, LIST_VAL);

        if (GXxmlStrcmp(iname, name) == 0)
        {
            value = GXxmlGetProp(item, ITEM_DISP);
            strncpy(buf, (char*)value, size);

            GXxmlFree(iname);
            GXxmlFree(value);

            return GXCORE_SUCCESS;
        }
        GXxmlFree(iname);
        item = item->next;
    }

    GXxmlFree(value);

    return GXCORE_ERROR;
}
static status_t info_get(GXxmlNodePtr GXxml_item, xmlChar *name, char * buf, size_t size,int mode)
{
    GXxmlNodePtr item = NULL;
    xmlChar *value = NULL;
    xmlChar *iname = NULL;

    item =  GXxml_item->childs;
    while (item != NULL)
    {
        iname = GXxmlGetProp(item, ITEM_DISP);

        if (GXxmlStrcmp(iname, name) == 0)
        {
            value = GXxmlGetProp(item, ITEM_VAL);
            strncpy(buf, (char*)value, size);
            if(mode)
            {
                info_get_list_item(item,value,buf, size);
            }
            GXxmlFree(iname);
            GXxmlFree(value);

            return GXCORE_SUCCESS;
        }
        GXxmlFree(iname);
        item = item->next;
    }

    GXxmlFree(value);

    return GXCORE_ERROR;
}

status_t app_get_netapps_default_xml(char * name, int32_t *result)
{
    char buf[BUF_LEN] = {0};
    GXxmlDocPtr doc;
    xmlChar *gname = NULL;

    doc = GXxmlParseFile(APP_OEM_PATH);
    if (NULL == doc)
    {
        gxlogd("\nERROR, GXxml parse error.\n");
        return GXCORE_ERROR;
    }

    // root
    GXxmlNodePtr root = doc->root;
    if (root == NULL)
    {
        gxlogd("\nERROR, GXxml empty.\n");
        GXxmlFreeDoc(doc);
        return GXCORE_ERROR;
    }

    if (0 !=  GXxmlStrcmp(root->name, OEM_STR))
    {
        GXxmlFreeDoc(doc);
        return GXCORE_ERROR;
    }

    // group
    GXxmlNodePtr group = root->childs;
    while (NULL != group)
    {
        gname = GXxmlGetProp(group, GROUP_NAME);

        if (gname != NULL &&
            0 ==  GXxmlStrcmp(gname, (xmlChar*)"netapp"))
        {
            info_get(group, (xmlChar*)name, buf, BUF_LEN, 0);
            GXxmlFree(gname);
            GXxmlFreeDoc(doc);
            *result = atoi(buf);
            return GXCORE_SUCCESS;
        }

        GXxmlFree(gname);
        gname = NULL;
        group = group->next;
    }

    GXxmlFreeDoc(doc);
    return GXCORE_ERROR;
}

// FOR player
typedef struct _PlayerMemConfClass{
    char *name;
    char *ConfigKey;
}PlayerMemConfClass;

void app_player_mem_config(char *name, unsigned int size)
{
#define ESV_   "esvmem"
#define ESA_   "esamem"
#define PCM_   "pcmmem"

    struct Gx_Mem_Info info;
    int ret = 0;
    int i = 0;
    unsigned int table_count = 0;
    PlayerMemConfClass table[] = {
        {ESV_, PLAYER_CONFIG_VIDEO_ESV_SIZE},
        {ESA_, PLAYER_CONFIG_AUDIO_ESA_SIZE},
        {PCM_, PLAYER_CONFIG_AUDIO_PCM_SIZE},
    };

    if((NULL == name) || (0 == size) || (0 != (size%1024)))
    {
        app_log_error(" Parameters Error!!!\n");
        return;
    }
    memset(&info, 0, sizeof(struct Gx_Mem_Info));
    ret = GxCore_MemInfoGet(name, &info);
    if(0 == ret)
    {// cmdline has been configured
        //if(info.size != size)
        goto Configured;
    }

    table_count = sizeof(table)/sizeof(PlayerMemConfClass);
    for(i = 0; i < table_count; i++)
    {
        if(0 == strncmp(name, table[i].name, strlen(table[i].name)))
        {
            GxBus_ConfigSetInt(table[i].ConfigKey, size);
            break;
        }

    }
    if(i == table_count)
    {
        app_log_error(" Parameter Error!!!\n",name);
    }
    return ;

Configured:
    app_log_error(" Loader has been Configured \"%s\": size= %d (Bytes)!!!\n", name, info.size);
}

bool app_ott_config_get(void)
{
#if OTT_SUPPORT
    return true;
#else
    return false;
#endif
}

void app_display_size_str_get(uint64_t size, char *str, size_t len)
{
    double display_size = 0.00;
    int tmp = 0;

    if(size >= (tmp = 1024 * 1024 * 1024))
    {
        display_size = (double)size / tmp;
        snprintf(str, len, "%3.2f GB", display_size);
    }
    else if(size >= (tmp = 1024 * 1024))
    {
        display_size = (double)size / tmp;
        snprintf(str, len, "%3.2f MB", display_size);
    }
    else if(size >= (tmp = 1024))
    {
        display_size = (double)size / tmp;
        snprintf(str, len, "%3.2f KB", display_size);
    }
    else
    {
        display_size = (double)size;
        snprintf(str, len, "%3.2f B", display_size);
    }
}

int app_av_set_videoout_mode(uint8_t video_out_mode)
{
    GxHdmiStatus state;
    GxVoutHAL_HdmiGetStatus(&state);
    GxBus_ConfigSetInt(AV_VIDEO_OUT_MODE, video_out_mode);
    if(state == GX_HDMI_PLUGE_IN && video_out_mode == 1)
        GxVoutHAL_ChannelPowerOff(GXAV_VOUT_RCA);
    if(state == GX_HDMI_PLUGE_IN && video_out_mode == 0)
        GxVoutHAL_ChannelPowerOn(GXAV_VOUT_RCA);
    return 0;
}

