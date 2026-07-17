#include <gxtype.h>
#include "app.h"
#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
#include "app_rf_channels.h"
#include "app_dvbt2_search.h"
#include "app_t2_area_config.h"

static classDefaultFreqCh gdvbt2_freq_channel[MAX_DVBT2_TP_NUM];
extern int app_chnum2chstr(int chnum, char *channel, int country_type);

#if (DEMOD_ISDBT > 0)
static classDefaultFreqCh isdbt_freq_channel[] =
{
    {"7", 177.143, 2},	//the first one as main freq is set to nit search when auto scan.by dugan
    {"8", 183.143, 2},
    {"9", 189.143, 2},
    {"10", 195.143, 2},
    {"11", 201.143, 2},
    {"12", 207.143, 2},
    {"13", 213.143, 2},
    {"14", 473.143, 2},
    {"15", 479.143, 2},
    {"16", 485.143, 2},
    {"17", 491.143, 2},
    {"18", 497.143, 2},
    {"19", 503.143, 2},
    {"20", 509.143, 2},
    {"21", 515.143, 2},
    {"22", 521.143, 2},
    {"23", 527.143, 2},
    {"24", 533.143, 2},
    {"25", 539.143, 2},
    {"26", 545.143, 2},
    {"27", 551.143, 2},
    {"28", 557.143, 2},
    {"29", 563.143, 2},
    {"30", 569.143, 2},
    {"31", 575.143, 2},
    {"32", 581.143, 2},
    {"33", 587.143, 2},
    {"34", 593.143, 2},
    {"35", 599.143, 2},
    {"36", 605.143, 2},
    {"37", 611.143, 2},
    {"38", 617.143, 2},
    {"39", 623.143, 2},
    {"40", 629.143, 2},
    {"41", 635.143, 2},
    {"42", 641.143, 2},
    {"43", 647.143, 2},
    {"44", 653.143, 2},
    {"45", 659.143, 2},
    {"46", 665.143, 2},
    {"47", 671.143, 2},
    {"48", 677.143, 2},
    {"49", 683.143, 2},
    {"50", 689.143, 2},
    {"51", 695.143, 2},
    {"52", 701.143, 2},
    {"53", 707.143, 2},
    {"54", 713.143, 2},
    {"55", 719.143, 2},
    {"56", 725.143, 2},
    {"57", 731.143, 2},
    {"58", 737.143, 2},
    {"59", 743.143, 2},
    {"60", 749.143, 2},
    {"61", 755.143, 2},
    {"62", 761.143, 2},
    {"63", 767.143, 2},
    {"64", 773.143, 2},
    {"65", 779.143, 2},
    {"66", 785.143, 2},
    {"67", 791.143, 2},
    {"68", 797.143, 2},
    {"69", 803.143, 2},
};
#define ISDBT_CH_NUM	(sizeof(isdbt_freq_channel)/sizeof(classDefaultFreqCh))

#endif

int AppDvbt2GetFreqList(classDefaultFreqCh **fch)
{
    int num = 0;
    uint8_t channel_num = 0;
    uint32_t fre_num = 0;

    uint8_t country = 0;
    uint8_t u8MaxRFCh, u8MinRFCh;
    uint16_t bandwidth;
    float_t frequency;

    country = app_get_country_type();
    u8MinRFCh = app_get_min_channel_num_autosearch(country);
    u8MaxRFCh = app_get_max_channel_num_autosearch(country);
    channel_num = u8MinRFCh;
    do
    {
        app_channel_chnum2fre_autosearch(country, channel_num, &bandwidth, &frequency);
        app_log_debug("%d:%d:(%f/%d)\n", country, channel_num, frequency, bandwidth);
        gdvbt2_freq_channel[fre_num].band = (int)bandwidth;
        gdvbt2_freq_channel[fre_num].freq = frequency / 1000;
        app_chnum2chstr(channel_num, (char *)gdvbt2_freq_channel[fre_num].channel, country);
        //gdvbt2_freq_channel[fre_num].channel = channel_num;
        fre_num ++;
        channel_num = app_get_next_channel_num_autosearch(country, channel_num);
    }
    while (channel_num < u8MaxRFCh);

    *fch  = gdvbt2_freq_channel;//for test
    num = fre_num;

    app_log_debug("======AppDvbt2GetFreqList:(country:%d)%5.1f-%5.1f========\n", country, gdvbt2_freq_channel[0].freq, gdvbt2_freq_channel[fre_num - 1].freq);

#if DEMOD_ISDBT
    *fch = isdbt_freq_channel;
    num = ISDBT_CH_NUM;
#endif
    return num;
}

void AppDvbt2SetCountry(int s_country)
{
    GxBus_ConfigSetInt(T2_COUNTRY, s_country);
#if DEMOD_DVB_C && DEMOD_DVB_T && (0 == DEMOD_DVB_S) && NDEMOD_WITH_1TUNER
    extern  int app_t2_time_auto_set(void);
    app_t2_time_auto_set();
#endif
    return ;
}
int AppDvbt2GetCountry(void)
{
    int32_t s_country = -1;
    GxBus_ConfigGetInt(T2_COUNTRY, &s_country, T2_COUNTRY_VALUE);
    return s_country;
}

char *AppDvbt2GetCountryName(int s_country)
{
    return g_CountryName[s_country];
}
int AppDvbt2GetCountryNum(void)
{
    return COUNTRY_MAX;
}

#endif
