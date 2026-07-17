#include "app_module.h"

#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
#include "app_rf_channels.h"
#include "app_t2_area_config.h"

RfChannelItemPara Tp_list[MAX_DVBT2_TP_NUM] = {{0, 0},};
uint32_t Total_RfCh = 0;
#define DEFAULT_CURRENT_FRE		474000L
#define DEFAULT_CURRENT_BW		BANDWIDTH_8_MHZ
FreqTblType FreqTbl[FREQ_TAL_SIZE]  =
{
    //must sort by channel no.  from small to large
    //if the channel is invalid,set the frequency 0
    //No. offset ----->for displaying 0:= No. ; +X:No.-X and add "A"  ; -X:No.-X
    {6, 177500, 0}, // 6~9
    {10, 205500, 1}, // 9A
    {11, 212500, -1}, // 10~13
    {15, 0, -1}, //14~20
    {21, 480500, 0}, // 21~27
    {28, 527500, 1}, // 27A
    {29, 529500, -1}, // 28~75
};
uint32_t app_channel_chnum2fre(uint8_t country, uint8_t chnum, uint16_t *bandwidth, float_t *frequency);
uint32_t app_get_min_channel_num(uint8_t country);
extern int AppDvbt2GetCountry(void);

uint8_t app_get_country_type(void)
{
    int32_t s_country = 0;
    s_country = AppDvbt2GetCountry();
    app_log_debug("%s %d  s_country = %s\n", __FUNCTION__, __LINE__, g_CountryName[s_country]);
    if (0 == strcmp(COUNTRY_RUSSIAN, g_CountryName[s_country]))
        return COUNTRY_TYPE_RUSSIAN;
    else if (0 == strcmp(COUNTRY_THAILAND, g_CountryName[s_country]))
        return COUNTRY_TYPE_THAI;
    else if (0 == strcmp(COUNTRY_UK, g_CountryName[s_country]))
        return COUNTRY_TYPE_UK;
    else if (0 == strcmp(COUNTRY_ITALY, g_CountryName[s_country]))
        return COUNTRY_TYPE_EUROPE;
    else if (0 == strcmp(COUNTRY_FRANCE, g_CountryName[s_country]))
        return COUNTRY_TYPE_EUROPE;
    else if (0 == strcmp(COUNTRY_GERMANY, g_CountryName[s_country]))
        return COUNTRY_TYPE_EUROPE;
    else if (0 == strcmp(COUNTRY_AUSTRALIA, g_CountryName[s_country]))
        return COUNTRY_TYPE_AUSTRALIA;
    else if (0 == strcmp(COUNTRY_CHINA, g_CountryName[s_country]))
        return COUNTRY_TYPE_CHINA;
    else if (0 == strcmp(COUNTRY_BRAZIL, g_CountryName[s_country]))
        return COUNTRY_TYPE_BRAZIL;
    else if (0 == strcmp(COUNTRY_ARGENTINA, g_CountryName[s_country]))
        return COUNTRY_TYPE_ARGENTINA;
    else if (0 == strcmp(COUNTRY_CHILE, g_CountryName[s_country]))
        return COUNTRY_TYPE_CHILE;
    else if (0 == strcmp(COUNTRY_PERU, g_CountryName[s_country]))
        return COUNTRY_TYPE_PERU;
    else if (0 == strcmp(CONNTRY_INDIA, g_CountryName[s_country]))
        return COUNTRY_TYPE_INDIA;
    else if (0 == strcmp(COUNTRY_POLAND, g_CountryName[s_country]))
        return COUNTRY_TYPE_EUROPE;
    else if (0 == strcmp(COUNTRY_ESTONIA, g_CountryName[s_country]))
        return COUNTRY_TYPE_EST;
    else if (0 == strcmp(COUNTRY_IRELAND, g_CountryName[s_country]))
        return COUNTRY_TYPE_EUROPE;
    else if (0 == strcmp(COUNTRY_LATVIA, g_CountryName[s_country]))
        return COUNTRY_TYPE_LAT;
    else if (0 == strcmp(COUNTRY_LITHUANIA, g_CountryName[s_country]))
        return COUNTRY_TYPE_LTU;
    else if (0 == strcmp(COUNTRY_SLOVAKIA, g_CountryName[s_country]))
        return COUNTRY_TYPE_EUROPE;
    else if (0 == strcmp(COUNTRY_PORTUGAL, g_CountryName[s_country]))
        return COUNTRY_TYPE_EUROPE;
    else if (0 == strcmp(CONNTRY_VIET, g_CountryName[s_country]))
        return COUNTRY_TYPE_VIET;
    else if (0 == strcmp(COUNTRY_COLOMBIA, g_CountryName[s_country]))
        return COUNTRY_TYPE_COL;
    else if (0 == strcmp(COUNTRY_SINGAPORE, g_CountryName[s_country]))
        return COUNTRY_TYPE_UK;
    else if (0 == strcmp(COUNTRY_PHILIPPINES, g_CountryName[s_country]))
        return COUNTRY_TYPE_PHL;
    else if (0 == strcmp(COUNTRY_INDONESIA, g_CountryName[s_country]))
        return COUNTRY_TYPE_INA;
    else
        return COUNTRY_TYPE_EUROPE;
}

uint8_t app_get_current_fre_info(float_t *current_fre, uint8_t *current_bandwidth)
{
    GxMsgProperty_NodeByPosGet node = {0};
    GxMsgProperty_NodeByIdGet node_tp = {0};

    *current_fre = DEFAULT_CURRENT_FRE;
    *current_bandwidth = DEFAULT_CURRENT_BW;//8M

    if (g_AppPlayOps.normal_play.play_total != 0)
    {
        node.node_type = NODE_PROG;
        node.pos = g_AppPlayOps.normal_play.play_count;
        if (GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
        {
            node_tp.node_type = NODE_TP;
            node_tp.id = node.prog_data.tp_id;
            if (GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_tp))
            {
                *current_fre = node_tp.tp_data.frequency / 1000;
                *current_bandwidth = node_tp.tp_data.tp_dvbt2.bandwidth;
                return 0;
            }
        }
    }
    return 1;
}

uint16_t app_get_current_fre_rfch_num(uint8_t country, uint8_t *rfch)
{
    GxMsgProperty_NodeByPosGet node = {0};
    GxMsgProperty_NodeByIdGet node_tp = {0};
    float_t FreValue  = 0;
    uint16_t BandMode = BAND_VHF_MODE;
    uint16_t BandWith = BANDWIDTH_6_MHZ;

    if (g_AppPlayOps.normal_play.play_total != 0)
    {
        node.node_type = NODE_PROG;
        node.pos = g_AppPlayOps.normal_play.play_count;
        if (GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
        {
            node_tp.node_type = NODE_TP;
            node_tp.id = node.prog_data.tp_id;
            if (GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_tp))
            {
                FreValue = node_tp.tp_data.frequency;
                *rfch = app_channel_fre2chnum(country, FreValue);
                //GUI_SetProperty("cmb_t2_manual_search_band","select",&BandMode);
                //return 0;
                if (*rfch == 0)
                {
                    *rfch = app_get_min_channel_num(country);
                    app_channel_chnum2fre(country, *rfch, &BandWith, &FreValue);
                }

            }
        }
        else
        {
            *rfch = app_get_min_channel_num(country);
            app_channel_chnum2fre(country, *rfch, &BandWith, &FreValue);

        }
    }
    else
    {
        //GUI_SetProperty("cmb_t2_manual_search_band","select",&BandMode);
        *rfch = app_get_min_channel_num(country);
        app_channel_chnum2fre(country, *rfch, &BandWith, &FreValue);
    }

    if (FreValue >= UHF_BAND_MIN)
        BandMode = BAND_UHF_MODE;
    return BandMode;
}

uint32_t app_get_max_channel_num(uint8_t country)
{
    uint32_t value = 0;
    GUI_GetProperty("cmb_t2_manual_search_band", "select", &value); //0 vhf 1 uhf

    switch (country)
    {

        case COUNTRY_TYPE_COL:
            if (BAND_VHF_MODE == value)
                return 13;
            else
                return 69;
        case COUNTRY_TYPE_INA:
            if (BAND_VHF_MODE == value)
                return 15;
            else
                return 69;
        case COUNTRY_TYPE_CHILE:
            return 79;
        case COUNTRY_TYPE_PERU:
        case COUNTRY_TYPE_BRAZIL:
            return 69;
        case COUNTRY_TYPE_ARGENTINA:
            return 15;
        case COUNTRY_TYPE_HONGKONG:
        case COUNTRY_TYPE_CHINA:
            return 69;
        case COUNTRY_TYPE_UK:
            return 70;
        case COUNTRY_TYPE_THAI:
            if (BAND_VHF_MODE == value)
                return 20;
            else
                return 69;
        case COUNTRY_TYPE_RUSSIAN:
        case COUNTRY_TYPE_EUROPE:
        case COUNTRY_TYPE_PHL:
            if (BAND_VHF_MODE == value)
                return 12;
            else
                return 70;
        case COUNTRY_TYPE_AUSTRALIA:
            if (BAND_VHF_MODE == value)
                return 13;
            else
                return 76;

        case COUNTRY_TYPE_TAIWAN:
            return 35;

        case COUNTRY_TYPE_LTU:
        case COUNTRY_TYPE_LAT:
        case COUNTRY_TYPE_EST:
        {
            if (BAND_VHF_MODE == value)
                return 20;
            else
                return 91;
        }
        break;
        case COUNTRY_TYPE_VIET:
        {
            if (BAND_VHF_MODE == value)
                return 13;
            else
                return 70;
        }

        break;

        case COUNTRY_TYPE_OTHER:
            return 100 ;

        default:
            return 0;
    }
}
uint32_t app_get_min_channel_num(uint8_t country)
{
    uint32_t value = 0;
    GUI_GetProperty("cmb_t2_manual_search_band", "select", &value); //0 vhf 1 uhf
    switch (country)
    {
        case COUNTRY_TYPE_CHILE:
            return 7;
        //-------------------------//
        case COUNTRY_TYPE_PERU:
        case COUNTRY_TYPE_BRAZIL:
            return 7;
        //-------------------------//
        case COUNTRY_TYPE_ARGENTINA:
            return 14;
        //-------------------------//
        case COUNTRY_TYPE_HONGKONG:
        case COUNTRY_TYPE_CHINA:
        {
            return 21;
        }
        //-------------------------//
        case COUNTRY_TYPE_UK:
            return 21;

        case COUNTRY_TYPE_COL:
        {
            if (BAND_VHF_MODE == value)
                return 1;
            else
                return 14;
        }

        case COUNTRY_TYPE_PHL:
        case COUNTRY_TYPE_RUSSIAN:
        {
            if (BAND_VHF_MODE == value)
                return 5;
            else
                return 21;
        }
        case COUNTRY_TYPE_THAI:
        {
            if (BAND_VHF_MODE == value)
                return 0;
            else
                return 21;
        }
        case COUNTRY_TYPE_INA:
        {
            if (BAND_VHF_MODE == value)
                return 2;
            else
                return 21;
        }
        case COUNTRY_TYPE_EUROPE:
        {
            if (BAND_VHF_MODE == value)
                return 5;
            else
                return 21;
        }

        //-------------------------//
        case COUNTRY_TYPE_AUSTRALIA:
        {
            if (BAND_VHF_MODE == value)
                return 6;
            else
                return 21;
        }
        //-------------------------//
        case COUNTRY_TYPE_TAIWAN:
        {
            return 24;
        }
        //-------------------------//

        case COUNTRY_TYPE_POLAND:
        {
            if (BAND_VHF_MODE == value)
                return 5;
            else
                return 21;
        }
        break;
        case COUNTRY_TYPE_LTU:
        case COUNTRY_TYPE_LAT:
        case COUNTRY_TYPE_EST:
        {
            if (BAND_VHF_MODE == value)
                return 2;
            else
                return 21;
        }
        break;
        case COUNTRY_TYPE_VIET:
        {
            if (BAND_VHF_MODE == value)
                return 6;
            else
                return 21;
        }

        break;

        case COUNTRY_TYPE_OTHER:
            return 0;
        default:
            return 0;
    }
}
uint32_t app_get_max_channel_num_autosearch(uint8_t country)
{
    switch (country)
    {
        case COUNTRY_TYPE_CHILE:
            return 79;
        case COUNTRY_TYPE_PERU:
        case COUNTRY_TYPE_BRAZIL:
            return 69;
        case COUNTRY_TYPE_ARGENTINA:
            return 15;
        case COUNTRY_TYPE_HONGKONG:
        case COUNTRY_TYPE_CHINA:
            return 69;
        case COUNTRY_TYPE_THAI:
            return 69;
        case COUNTRY_TYPE_INA:
        case COUNTRY_TYPE_RUSSIAN:
        case COUNTRY_TYPE_UK:
        case COUNTRY_TYPE_EUROPE:
        case COUNTRY_TYPE_PHL:
            return 71;
        case COUNTRY_TYPE_COL:
            return 70;
        case COUNTRY_TYPE_AUSTRALIA:
            return 76;

        case COUNTRY_TYPE_TAIWAN:
            return 35;

        case COUNTRY_TYPE_LTU:
        case COUNTRY_TYPE_LAT:
        case COUNTRY_TYPE_EST:
        {
            return 92;
        }
        break;
        case COUNTRY_TYPE_VIET:
        {
            return 71;
        }
        case COUNTRY_TYPE_OTHER:
            return 100 ;

        default:
            return 0;
    }
}
uint32_t app_get_min_channel_num_autosearch(uint8_t country)
{
    switch (country)
    {
        case COUNTRY_TYPE_CHILE:
            return 7;
        //-------------------------//
        case COUNTRY_TYPE_PERU:
        case COUNTRY_TYPE_BRAZIL:
            return 7;
        //-------------------------//
        case COUNTRY_TYPE_ARGENTINA:
            return 14;
        //-------------------------//
        //#if (MARKET == MARKET_HONGKONG || MARKET == MARKET_CHINA)
        case COUNTRY_TYPE_HONGKONG:
        case COUNTRY_TYPE_CHINA:
        {
            return 21;
        }
        //#endif
        //-------------------------//
        case COUNTRY_TYPE_UK:
            return 21;
        case COUNTRY_TYPE_THAI:
        {
            return 0;
        }
        case COUNTRY_TYPE_PHL:
        case COUNTRY_TYPE_RUSSIAN:
        {
            return 5;

        }
        case COUNTRY_TYPE_INA:
        {
            return 2;
        }
        case COUNTRY_TYPE_COL:
        case COUNTRY_TYPE_FRANCE:
        {
            return 1;
        }

        case COUNTRY_TYPE_EUROPE:
        {
            return 5;
        }

        //-------------------------//
        case COUNTRY_TYPE_AUSTRALIA:
        {
            return 6;
        }
        //-------------------------//
        case COUNTRY_TYPE_TAIWAN:
        {
            return 24;
        }
        case COUNTRY_TYPE_LTU:
        case COUNTRY_TYPE_LAT:
        case COUNTRY_TYPE_EST:
        {
            return 2;
        }
        break;
        case COUNTRY_TYPE_VIET:
        {
            return 6;
        }
        case COUNTRY_TYPE_OTHER:
            return 0;
        default:
            return 0;
    }
}

uint32_t app_channel_fre2chnum(uint8_t country, float_t frequency)
{
    uint8_t  u8RF_CH = 0;
    uint32_t BandValue = BAND_VHF_MODE;

    switch (country)
    {
        default:
            u8RF_CH = 0;
            break;
        //-------------------------------------//
        case COUNTRY_TYPE_COL:
        {
            if (frequency <= 213000L)
            {
                switch ((int)frequency)
                {
                    case 51000L:
                        u8RF_CH = 1;
                        break;
                    case 57000L:
                        u8RF_CH = 2;
                        break;
                    case 63000L:
                        u8RF_CH = 3;
                        break;
                    case 69000L:
                        u8RF_CH = 4;
                        break;
                    case 79000L:
                        u8RF_CH = 5;
                        break;
                    case 85000L:
                        u8RF_CH = 6;
                        break;
                    case 177000L:
                        u8RF_CH = 7;
                        break;
                    case 183000L:
                        u8RF_CH = 8;
                        break;
                    case 189000L:
                        u8RF_CH = 9;
                        break;
                    case 195000L:
                        u8RF_CH = 10;
                        break;
                    case 201000L:
                        u8RF_CH = 11;
                        break;
                    case 207000L:
                        u8RF_CH = 12;
                        break;
                    case 213000L:
                        u8RF_CH = 13;
                        break;
                    default:
                        u8RF_CH = 1;
                        break;
                }
                BandValue = BAND_VHF_MODE;
            }
            else
            {
                u8RF_CH = (uint8_t)((frequency - 473000L) / 6000L) + 14;
                BandValue = BAND_UHF_MODE;
            }
            break;
        }

        case COUNTRY_TYPE_CHILE:
        {
            if (frequency < 473000L)
            {
                // 177.143 ~ 473.143
                u8RF_CH = (uint8_t)((frequency - 177000L) / 6000L) + 7;
                BandValue = BAND_VHF_MODE;
            }
            else
            {
                u8RF_CH = (uint8_t)((frequency - 473000L) / 6000L) + 14;
                BandValue = BAND_UHF_MODE;
            }
            break;
        }

        //-------------------------------------//
        case COUNTRY_TYPE_PERU:
        case COUNTRY_TYPE_BRAZIL:
        {
            if (frequency < 473143L)
            {
                // 177.143 ~ 473.143
                u8RF_CH = (uint8_t)((frequency - 177143L) / 6000L) + 7;
                BandValue = BAND_VHF_MODE;
            }
            else
            {
                u8RF_CH = (uint8_t)((frequency - 473143L) / 6000L) + 14;
                BandValue = BAND_UHF_MODE;
            }
            break;
        }
        //-------------------------------------//
        case COUNTRY_TYPE_ARGENTINA:
        {
            if (frequency == 527000)
            {
                // 177.143 ~ 473.143
                u8RF_CH = 14;
            }
            else
            {
                u8RF_CH = 15;
            }
            BandValue = BAND_UHF_MODE;
            break;
        }
        //-------------------------------------//
        case COUNTRY_TYPE_HONGKONG:
        case COUNTRY_TYPE_EUROPE:
        case COUNTRY_TYPE_UK:
        case COUNTRY_TYPE_CHINA:
        {
            if ((47000L < frequency) && (frequency < 68000L))
            {
                // 50.5 ~ 64.5
                u8RF_CH = (uint8_t)((frequency - 50500L) / 7000L) + 2;
                BandValue = BAND_VHF_MODE;
            }
            else if ((174000L < frequency) && (frequency < 230000L))
            {
                // 177.5 ~ 226.5
                u8RF_CH = (uint8_t)((frequency - 177500L) / 7000L) + 5;
                BandValue = BAND_VHF_MODE;
            }
            else  if (470000 < frequency)
            {
                u8RF_CH = (uint8_t)((frequency - 474000L) / 8000L) + 21;
                BandValue = BAND_UHF_MODE;
            }
            break;
        }
        case COUNTRY_TYPE_THAI:
        {
            if ((302000L < frequency) && (frequency < 462000L))
            {
                // 306 ~ 458
                u8RF_CH = (uint8_t)((frequency - 306000L) / 8000L) ;
                BandValue = BAND_VHF_MODE;
            }

            else  if (470000 < frequency)
            {
                u8RF_CH = (uint8_t)((frequency - 474000L) / 8000L) + 21;
                BandValue = BAND_UHF_MODE;
            }
            break;
        }

        case COUNTRY_TYPE_PHL:
        {
            if ((47000L < frequency) && (frequency < 68000L))
            {
                // 50.5 ~ 64.5
                u8RF_CH = (uint8_t)((frequency - 50500L) / 6000L) + 2;
                BandValue = BAND_VHF_MODE;
            }
            else if ((174000L < frequency) && (frequency < 230000L))
            {
                // 177.5 ~ 226.5
                u8RF_CH = (uint8_t)((frequency - 178000L) / 6000L) + 5;
                BandValue = BAND_VHF_MODE;
            }
            else  if (470000 < frequency)
            {
                u8RF_CH = (uint8_t)((frequency - 474000L) / 6000L) + 21;
                BandValue = BAND_UHF_MODE;
            }
            break;
        }
        case COUNTRY_TYPE_INA:
        {
            if ((47000L < frequency) && (frequency < 68000L))
            {
                // 50.5 ~ 64.5
                u8RF_CH = (uint8_t)((frequency - 50500L) / 7000L) + 2;
                BandValue = BAND_VHF_MODE;
            }
            else if ((174000L < frequency) && (frequency < 230000L))
            {
                // 177.5 ~ 226.5
                u8RF_CH = (uint8_t)((frequency - 178000L) / 7000L) + 5;
                BandValue = BAND_VHF_MODE;
            }

            else if ((287000L < frequency) && (frequency < 294000L))
            {
                //290.5
                u8RF_CH =  13;
                BandValue = BAND_VHF_MODE;
            }
            else if ((310000L < frequency) && (frequency < 324000L))
            {
                // 177.5 ~ 226.5
                u8RF_CH = (uint8_t)((frequency - 3135000L) / 7000L) + 14;
                BandValue = BAND_VHF_MODE;
            }
            else  if (470000 < frequency)
            {
                u8RF_CH = (uint8_t)((frequency - 474000L) / 8000L) + 21;
                BandValue = BAND_UHF_MODE;
            }
            break;
        }
        case COUNTRY_TYPE_RUSSIAN:
        {
            if ((47000L < frequency) && (frequency < 68000L))
            {
                // 50.5 ~ 64.5
                u8RF_CH = (uint8_t)((frequency - 50500L) / 7000L) + 2;
                BandValue = BAND_VHF_MODE;
            }
            else if ((174000L < frequency) && (frequency < 230000L))
            {
                // 177.5 ~ 226.5
                u8RF_CH = (uint8_t)((frequency - 178000L) / 8000L) + 5;
                BandValue = BAND_VHF_MODE;
            }
            else  if (470000 < frequency)
            {
                u8RF_CH = (uint8_t)((frequency - 474000L) / 8000L) + 21;
                BandValue = BAND_UHF_MODE;
            }
            break;
        }
        //-------------------------------------//
        case COUNTRY_TYPE_AUSTRALIA:
        {

            int8_t s8Index = FREQ_TAL_SIZE - 1;

            for (s8Index = FREQ_TAL_SIZE - 1; s8Index >= 0; s8Index--)
            {
                if (FreqTbl[s8Index].u32Freq > 0 && frequency >= FreqTbl[s8Index].u32Freq)
                {
                    break;
                }
            }
            if (s8Index >= 0)
            {
                u8RF_CH = (uint8_t)((frequency - FreqTbl[s8Index].u32Freq) / 7000L) + FreqTbl[s8Index].u8CHNo;
                if (FreqTbl[s8Index].u8CHNo < 21)
                    BandValue = BAND_VHF_MODE;
                else
                    BandValue = BAND_UHF_MODE;
            }
            else
            {
                u8RF_CH = FreqTbl[0].u8CHNo;
            }
            break;
        }
        //-------------------------------------//
        case COUNTRY_TYPE_TAIWAN:
        {
            //if (u8RF_CH >= 24 && u8RF_CH <= 36)
            if (frequency >= 533000L && frequency <= 605000L)
            {
                //533 ~ 605 Mhz
                u8RF_CH = (uint8_t)((frequency - 533000L) / 6000L) + 24;
                BandValue = BAND_UHF_MODE;
            }
            break;
        }
        case 	COUNTRY_TYPE_EST:
        case 	COUNTRY_TYPE_LAT:
        case 	COUNTRY_TYPE_LTU:
        {
            if ((47000L < frequency) && (frequency < 68000))   //50.5 ~ 64.5 Mhz
            {
                u8RF_CH = (uint8_t)((frequency - 50500L) / 7000L) + 2;
                BandValue = BAND_VHF_MODE;
            }
            else  if ((174000L < frequency) && (frequency < 230000L))    //177.5 ~ 226.5 Mhz
            {
                u8RF_CH = (uint8_t)((frequency - 177500L) / 7000L) + 5;
                BandValue = BAND_VHF_MODE;
            }
            else if ((230000L < frequency) && (frequency < 302000L)) //234 ~ 298 Mhz
            {
                u8RF_CH = (uint8_t)((frequency - 234000L) / 7000L) + 13;
                BandValue = BAND_VHF_MODE;
            }
            else if (302000L < frequency) //306~ 866 Mhz
            {
                u8RF_CH = (uint8_t)((frequency - 306000L) / 7000L) + 21;
                BandValue = BAND_UHF_MODE;
            }

        }

        break;
        case COUNTRY_TYPE_VIET:
        {
            if ((174000L < frequency) && (frequency < 238000L))   //178 ~ 234 Mhz
            {
                u8RF_CH = (uint8_t)((frequency - 178000L) / 8000L) + 6;
                BandValue = BAND_VHF_MODE;

            }
            else if (frequency > 470000L)    //474 ~ 858 Mhz
            {
                u8RF_CH = (uint8_t)((frequency - 474000L) / 8000L) + 21;
                BandValue = BAND_UHF_MODE;

            }
        }
        break;

        //-------------------------------------//
        case COUNTRY_TYPE_OTHER:
            u8RF_CH = (uint8_t)((frequency - 98000) / 8000) ;
            BandValue = BAND_VHF_MODE;
            break;
            //-------------------------------------//
    }
    GUI_SetProperty("cmb_t2_manual_search_band", "select", &BandValue);
    return u8RF_CH;
}
uint32_t app_channel_chnum2fre(uint8_t country, uint8_t chnum, uint16_t *bandwidth, float_t *frequency)
{
    uint8_t u8MaxRFCh, u8MinRFCh;

    u8MaxRFCh = app_get_max_channel_num(country);
    u8MinRFCh = app_get_min_channel_num(country);
    if (chnum < u8MinRFCh || chnum > u8MaxRFCh)
    {
        *frequency = 0;
        return FALSE;
    }

    //---------------------------------------------
    switch (country)
    {
        case COUNTRY_TYPE_COL:
        {
            if (chnum >= 14 && chnum <= 69)
            {
                *frequency = 473000L + (chnum - 14) * 6000L;
            }
            else
            {
                switch (chnum)
                {
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                        *frequency = 51000L + (chnum - 1) * 6000L;
                        break;
                    case 5:
                        *frequency = 79000L;
                        break;
                    case 6:
                        *frequency = 85000L;
                        break;
                    case 7:
                        *frequency = 177000L;
                        break;
                    case 8:
                    case 9:
                    case 10:
                    case 11:
                    case 12:
                    case 13:
                        *frequency = 183000L + (chnum - 8) * 6000L;
                        break;
                    default:
                        *frequency = 0;
                        break;
                }
            }
            *bandwidth = BANDWIDTH_6_MHZ;
        }
        break;

        case COUNTRY_TYPE_CHILE:
            if ((chnum >= 14) && (chnum <= 79))   //473.000 ~ 803.000 Mhz
            {
                *frequency = 473000L + (chnum - 14) * 6000L;
                *bandwidth = BANDWIDTH_6_MHZ;
            }
            else if ((chnum >= 7) && (chnum <= 13)) //177.000~473.000 Mhz
            {
                *frequency = 177000L + (chnum - 7) * 6000L;
                *bandwidth = BANDWIDTH_6_MHZ;
            }
            else
            {
                *frequency = 0;
            }
            break;

        //-------------------------------------//
        case COUNTRY_TYPE_PERU:
        case COUNTRY_TYPE_BRAZIL:
            if ((chnum >= 14) && (chnum <= 69))   //473.143 ~ 803.143 Mhz
            {
                *frequency = 473143L + (chnum - 14) * 6000L;
                *bandwidth = BANDWIDTH_6_MHZ;
            }
            else if ((chnum >= 7) && (chnum <= 13)) //177.143~473.143 Mhz
            {
                *frequency = 177143L + (chnum - 7) * 6000L;
                *bandwidth = BANDWIDTH_6_MHZ;
            }
            else
            {
                *frequency = 0;
            }
            break;
        case COUNTRY_TYPE_ARGENTINA:
        {
            if ((chnum == 14))
            {
                *frequency = 527000;
                *bandwidth = BANDWIDTH_6_MHZ;
            }
            else
            {
                *frequency = 647000;
                *bandwidth = BANDWIDTH_6_MHZ;
            }
            break;
        }
        break;

        //-------------------------------------//
        case COUNTRY_TYPE_UK:
        case COUNTRY_TYPE_HONGKONG:
        case COUNTRY_TYPE_EUROPE:
        case COUNTRY_TYPE_CHINA:
            if ((chnum >= 2) && (chnum <= 4))   //50.5,~ 71.5 Mhz
            {
                *frequency = 50500L + (chnum - 2) * 7000L;
                *bandwidth = BANDWIDTH_7_MHZ;
            }
            else if ((chnum >= 5) && (chnum <= 12))   //177.5 ~ 226.5 Mhz
            {
                *frequency = 177500L + (chnum - 5) * 7000L;
                *bandwidth = BANDWIDTH_7_MHZ;
            }
            else if ((chnum >= 21) && (chnum <= 70))   //474 ~ 858 Mhz
            {
                *frequency = 474000L + (chnum - 21) * 8000L;
                *bandwidth = BANDWIDTH_8_MHZ;
            }
            else
            {
                *frequency = 0;
            }
            break;
        case COUNTRY_TYPE_THAI:
            if ((chnum >= 0) && (chnum <= 20))   //306,~ 458 Mhz
            {
                *frequency = 306000L + chnum * 8000L;
                *bandwidth = BANDWIDTH_8_MHZ;
            }
            else if ((chnum >= 21) && (chnum <= 70))   //474 ~ 858 Mhz
            {
                *frequency = 474000L + (chnum - 21) * 8000L;
                *bandwidth = BANDWIDTH_8_MHZ;
            }
            else
            {
                *frequency = 0;
            }
            break;
        case COUNTRY_TYPE_PHL:
            if ((chnum >= 2) && (chnum <= 4))   //50.5,~ 71.5 Mhz
            {
                *frequency = 50500L + (chnum - 2) * 6000L;
                *bandwidth = BANDWIDTH_6_MHZ;
            }
            else if ((chnum >= 5) && (chnum <= 12))   //177.5 ~ 226.5 Mhz
            {
                *frequency = 178000L + (chnum - 5) * 6000L;
                *bandwidth = BANDWIDTH_6_MHZ;
            }
            else if ((chnum >= 21) && (chnum <= 70))   //474 ~ 858 Mhz
            {
                *frequency = 474000L + (chnum - 21) * 6000L;
                *bandwidth = BANDWIDTH_6_MHZ;
            }
            else
            {
                *frequency = 0;
            }
            break;
        case COUNTRY_TYPE_INA:
            if ((chnum >= 2) && (chnum <= 4))   //50.5,~ 71.5 Mhz
            {
                *frequency = 50500L + (chnum - 2) * 7000L;
                *bandwidth = BANDWIDTH_7_MHZ;
            }
            else if ((chnum >= 5) && (chnum <= 12))   //177.5 ~ 226.5 Mhz
            {
                *frequency = 178000L + (chnum - 5) * 7000L;
                *bandwidth = BANDWIDTH_7_MHZ;
            }
            else if (chnum == 13)   //290..5
            {
                *frequency = 290500;
                *bandwidth = BANDWIDTH_7_MHZ;
            }
            else if ((chnum >= 14) && (chnum <= 15))   //313.5 ~ 32.5 Mhz
            {
                *frequency = 313500 + (chnum - 14) * 7000L;
                *bandwidth = BANDWIDTH_7_MHZ;
            }
            else if ((chnum >= 21) && (chnum <= 70))   //474 ~ 858 Mhz
            {
                *frequency = 474000L + (chnum - 21) * 8000L;
                *bandwidth = BANDWIDTH_8_MHZ;
            }
            else
            {
                *frequency = 0;
            }
            break;
        case COUNTRY_TYPE_RUSSIAN:
            if ((chnum >= 2) && (chnum <= 4))   //50.5,~ 71.5 Mhz
            {
                *frequency = 50500L + (chnum - 2) * 7000L;
                *bandwidth = BANDWIDTH_7_MHZ;
            }
            else if ((chnum >= 5) && (chnum <= 12))   //177.5 ~ 226.5 Mhz
            {
                *frequency = 178000L + (chnum - 5) * 8000L;
                *bandwidth = BANDWIDTH_8_MHZ;
            }
            else if ((chnum >= 21) && (chnum <= 70))   //474 ~ 858 Mhz
            {
                *frequency = 474000L + (chnum - 21) * 8000L;
                *bandwidth = BANDWIDTH_8_MHZ;
            }
            else
            {
                *frequency = 0;
            }
            break;
        //-------------------------------------//
        case COUNTRY_TYPE_AUSTRALIA:
        {
            int8_t s8Index = FREQ_TAL_SIZE - 1;

            for (s8Index = FREQ_TAL_SIZE - 1; s8Index >= 0; s8Index--)
            {
                if (chnum >= FreqTbl[s8Index].u8CHNo)
                {
                    break;
                }
            }
            if (s8Index >= 0 && FreqTbl[s8Index].u32Freq > 0)
            {
                *frequency = FreqTbl[s8Index].u32Freq + (chnum - FreqTbl[s8Index].u8CHNo) * 7000L;
            }
            else
            {
                *frequency = 0;
            }
            *bandwidth = BANDWIDTH_7_MHZ;
            break;
        }
        //-------------------------------------//
        case COUNTRY_TYPE_TAIWAN:
            if ((chnum >= 24) && (chnum <= 36))   //533 ~ 605 Mhz
            {
                *frequency = 533000L + (chnum - 24) * 6000L;
                *bandwidth = BANDWIDTH_6_MHZ;
            }
            else
            {
                *frequency = 0;
            }
            break;

        //-------------------------------------//
        case 	COUNTRY_TYPE_EST:
        case 	COUNTRY_TYPE_LAT:
        case 	COUNTRY_TYPE_LTU:
        {
            if ((chnum >= 2) && (chnum <= 4))   //50.5,~ 71.5 Mhz
            {
                *frequency = 50500L + (chnum - 2) * 7000L;
                *bandwidth = BANDWIDTH_7_MHZ;
            }
            else if ((chnum >= 5) && (chnum <= 12))   //177.5 ~ 226.5 Mhz
            {
                *frequency = 177500L + (chnum - 5) * 7000L;
                *bandwidth = BANDWIDTH_7_MHZ;
            }
            else if ((chnum >= 13) && (chnum <= 20))   //234 ~466 Mhz
            {
                *frequency = 234000L + (chnum - 13) * 8000L;
                *bandwidth = BANDWIDTH_8_MHZ;
            }

            else if ((chnum >= 21) && (chnum <= 92))   //306 ~ 858 Mhz
            {
                *frequency = 306000L + (chnum - 21) * 8000L;
                *bandwidth = BANDWIDTH_8_MHZ;
            }
            else
            {
                *frequency = 0;
            }
        }

        break;

        case COUNTRY_TYPE_OTHER:
            *frequency  = 98000 + (chnum * 8000L);
            *bandwidth = BANDWIDTH_8_MHZ;
            //-------------------------------------//
            break;
        case COUNTRY_TYPE_VIET:
        {

            if ((chnum >= 6) && (chnum <= 13))   //178 ~ 234 Mhz
            {
                *frequency = 178000L + (chnum - 6) * 8000L;
                *bandwidth = BANDWIDTH_8_MHZ;
            }
            else if ((chnum >= 21) && (chnum <= 70))   //474 ~ 858 Mhz
            {
                *frequency = 474000L + (chnum - 21) * 8000L;
                *bandwidth = BANDWIDTH_8_MHZ;
            }
            else
            {
                *frequency = 0;
            }
        }
        break;
        default:
            *frequency = 0;
            break;
    }

    /* Frequency and symbol rate already verified in Scan Menu.
       0 = default value. */
    if (*frequency != 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
uint32_t app_get_next_channel_num(uint8_t country, uint8_t chnum)
{
    //--------------------------------------------
    //wyf 20090324 to avoid invalid channel number when manul scan
    uint8_t u8MaxRFCh, u8MinRFCh;

    u8MaxRFCh = app_get_max_channel_num(country);
    u8MinRFCh = app_get_min_channel_num(country);
    if (chnum == u8MaxRFCh)
    {
        chnum = u8MinRFCh;
        return chnum;
    }

    //---------------------------------------------
    switch (country)
    {
        case COUNTRY_TYPE_COL:
        {

            if (chnum >= u8MinRFCh && chnum < 13)
            {
                chnum++;
            }
            else if (chnum >= 13 && chnum < 14)
            {
                chnum =  14;
            }
            else if (chnum >= 14 && chnum < u8MaxRFCh)
            {
                chnum++;
            }
            else
            {
                chnum = u8MinRFCh;
            }

            return chnum;
        }

        case COUNTRY_TYPE_CHILE:
        case COUNTRY_TYPE_OTHER:
        case COUNTRY_TYPE_PERU:
        case COUNTRY_TYPE_BRAZIL:
        case COUNTRY_TYPE_ARGENTINA:
        {
            if (chnum < u8MaxRFCh)
            {
                chnum++;
            }
            else
            {
                chnum =  u8MinRFCh;
            }
            return chnum;
        }
        //------------------------------------------//
        case COUNTRY_TYPE_HONGKONG:
        case COUNTRY_TYPE_CHINA:
        {
            if (chnum < u8MaxRFCh)
            {
                chnum++;
            }
            else
            {
                chnum =  21;
            }

            return chnum;
        }
        case COUNTRY_TYPE_THAI:
        {
            if (chnum < u8MaxRFCh)
            {
                chnum++;
            }
            else
            {
                chnum =  21;
            }

            return chnum;
        }
        case COUNTRY_TYPE_INA:
            if (chnum >= u8MinRFCh && chnum < 16)
            {
                chnum++;
            }
            else if (chnum >= 16 && chnum < 21)
            {
                chnum =  21;
            }
            else if (chnum >= 21 && chnum < u8MaxRFCh)
            {
                chnum++;
            }
            else
            {
                chnum = u8MinRFCh;
            }

            return chnum;
        //------------------------------------------//
        case COUNTRY_TYPE_RUSSIAN:
        case COUNTRY_TYPE_UK:
        case COUNTRY_TYPE_EUROPE:
        case COUNTRY_TYPE_PHL:
        {
            if (chnum >= u8MinRFCh && chnum < 12)
            {
                chnum++;
            }
            else if (chnum >= 12 && chnum < 21)
            {
                chnum =  21;
            }
            else if (chnum >= 21 && chnum < u8MaxRFCh)
            {
                chnum++;
            }
            else
            {
                chnum = u8MinRFCh;
            }

            return chnum;
        }
        case COUNTRY_TYPE_LTU:
        case COUNTRY_TYPE_LAT:
        case COUNTRY_TYPE_EST:
        {
            if (chnum < u8MaxRFCh)
            {
                chnum++;
            }
            else
            {
                chnum =  u8MinRFCh;
            }
            return chnum;

        }
        break;
        case COUNTRY_TYPE_VIET:
        {
            if (chnum >= u8MinRFCh && chnum < 13)
            {
                chnum++;
            }
            else if (chnum >= 13 && chnum < 21)
            {
                chnum =  21;
            }
            else if (chnum >= 21 && chnum < u8MaxRFCh)
            {
                chnum++;
            }
            else
            {
                chnum = u8MinRFCh;
            }

            return chnum;
        }
        break;

        //------------------------------------------//
        case COUNTRY_TYPE_AUSTRALIA:
        {
            if (chnum < 13)
            {
                chnum++;
            }
            else if (chnum >= 13 && chnum < 21)
            {
                chnum =  21;
            }
            else if (chnum >= 21 && chnum < u8MaxRFCh)
            {
                chnum++;
            }
            else
            {
                chnum =  u8MinRFCh;
            }

            return chnum;
        }
        //------------------------------------------//
        case COUNTRY_TYPE_TAIWAN:
        {
            if (chnum >= 24 && chnum < 35)
            {
                chnum++;
            }
            else
            {
                chnum = 24;
            }
            return chnum;
        }

        default:
            return 0;
    }
}
uint32_t app_channel_chnum2fre_autosearch(uint8_t country, uint8_t chnum, uint16_t *bandwidth, float_t *frequency)
{
    uint8_t u8MaxRFCh, u8MinRFCh;

    u8MaxRFCh = app_get_max_channel_num_autosearch(country);
    u8MinRFCh = app_get_min_channel_num_autosearch(country);
    if (chnum < u8MinRFCh || chnum > u8MaxRFCh)
    {
        *frequency = 0;
        return FALSE;
    }

    //---------------------------------------------
    switch (country)
    {
        case COUNTRY_TYPE_COL:
        {
            if (chnum >= 14 && chnum <= 69)
            {
                *frequency = 473000L + (chnum - 14) * 6000L;
            }
            else
            {
                switch (chnum)
                {
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                        *frequency = 51000L + (chnum - 1) * 6000L;
                        break;
                    case 5:
                        *frequency = 79000L;
                        break;
                    case 6:
                        *frequency = 85000L;
                        break;
                    case 7:
                        *frequency = 177000L;
                        break;
                    case 8:
                    case 9:
                    case 10:
                    case 11:
                    case 12:
                    case 13:
                        *frequency = 183000L + (chnum - 8) * 6000L;
                        break;
                    default:
                        *frequency = 0;
                        break;
                }
            }
            *bandwidth = BANDWIDTH_6_MHZ;
        }
        break;

        case COUNTRY_TYPE_CHILE:
            if ((chnum >= 14) && (chnum <= 79))   //473.000 ~ 803.000 Mhz
            {
                *frequency = 473000L + (chnum - 14) * 6000L;
                *bandwidth = BANDWIDTH_6_MHZ;
            }
            else if ((chnum >= 7) && (chnum <= 13)) //177.000~473.000 Mhz
            {
                *frequency = 177000L + (chnum - 7) * 6000L;
                *bandwidth = BANDWIDTH_6_MHZ;
            }
            else
            {
                *frequency = 0;
            }
            break;

        //-------------------------------------//
        case COUNTRY_TYPE_PERU:
        case COUNTRY_TYPE_BRAZIL:
            if ((chnum >= 14) && (chnum <= 69))   //473.143 ~ 803.143 Mhz
            {
                *frequency = 473143L + (chnum - 14) * 6000L;
                *bandwidth = BANDWIDTH_6_MHZ;
            }
            else if ((chnum >= 7) && (chnum <= 13)) //177.143~473.143 Mhz
            {
                *frequency = 177143L + (chnum - 7) * 6000L;
                *bandwidth = BANDWIDTH_6_MHZ;
            }
            else
            {
                *frequency = 0;
            }
            break;
        case COUNTRY_TYPE_ARGENTINA:
        {
            if ((chnum == 14))
            {
                *frequency = 527000;
                *bandwidth = BANDWIDTH_6_MHZ;
            }
            else
            {
                *frequency = 647000;
                *bandwidth = BANDWIDTH_6_MHZ;
            }
            break;
        }
        break;

        //-------------------------------------//
        case COUNTRY_TYPE_UK:
        case COUNTRY_TYPE_HONGKONG:
        case COUNTRY_TYPE_EUROPE:
        case COUNTRY_TYPE_CHINA:
            if ((chnum >= 2) && (chnum <= 4))   //50.5,~ 71.5 Mhz
            {
                *frequency = 50500L + (chnum - 2) * 7000L;
                *bandwidth = BANDWIDTH_7_MHZ;
            }
            else if ((chnum >= 5) && (chnum <= 12))   //177.5 ~ 226.5 Mhz
            {
                *frequency = 177500L + (chnum - 5) * 7000L;
                *bandwidth = BANDWIDTH_7_MHZ;
            }
            else if ((chnum >= 21) && (chnum <= 70))   //474 ~ 858 Mhz
            {
                *frequency = 474000L + (chnum - 21) * 8000L;
                *bandwidth = BANDWIDTH_8_MHZ;
            }
            else
            {
                *frequency = 0;
            }
            break;
        case COUNTRY_TYPE_THAI:

            if ((chnum >= 0) && (chnum <= 20))   //306,~ 458 Mhz
            {
                *frequency = 306000L + chnum * 8000L;
                *bandwidth = BANDWIDTH_8_MHZ;

            }
            else if ((chnum >= 21) && (chnum <= 70))   //474 ~ 858 Mhz
            {
                *frequency = 474000L + (chnum - 21) * 8000L;
                *bandwidth = BANDWIDTH_8_MHZ;
            }
            else
            {
                *frequency = 0;
            }
            break;

        case COUNTRY_TYPE_PHL:
            if ((chnum >= 2) && (chnum <= 4))   //50.5,~ 71.5 Mhz
            {
                *frequency = 50500L + (chnum - 2) * 6000L;
                *bandwidth = BANDWIDTH_6_MHZ;
            }
            else if ((chnum >= 5) && (chnum <= 12))   //177.5 ~ 226.5 Mhz
            {
                *frequency = 178000L + (chnum - 5) * 6000L;
                *bandwidth = BANDWIDTH_6_MHZ;
            }
            else if ((chnum >= 21) && (chnum <= 70))   //474 ~ 858 Mhz
            {
                *frequency = 474000L + (chnum - 21) * 6000L;
                *bandwidth = BANDWIDTH_6_MHZ;
            }
            else
            {
                *frequency = 0;
            }
            break;
        case COUNTRY_TYPE_INA:
            if ((chnum >= 2) && (chnum <= 4))   //50.5,~ 71.5 Mhz
            {
                *frequency = 50500L + (chnum - 2) * 7000L;
                *bandwidth = BANDWIDTH_7_MHZ;
            }
            else if ((chnum >= 5) && (chnum <= 12))   //177.5 ~ 226.5 Mhz
            {
                *frequency = 177500L + (chnum - 5) * 7000L;
                *bandwidth = BANDWIDTH_7_MHZ;
            }
            else if (chnum == 13)   //290.5mhz
            {
                *frequency = 290500;
                *bandwidth = BANDWIDTH_7_MHZ;
            }
            else if ((chnum >= 14) && (chnum <= 15))   //313.5 ~ 320.5 Mhz
            {
                *frequency = 313500 + (chnum - 14) * 7000L;
                *bandwidth = BANDWIDTH_7_MHZ;
            }
            else if ((chnum >= 21) && (chnum <= 70))   //474 ~ 858 Mhz
            {
                *frequency = 474000L + (chnum - 21) * 8000L;
                *bandwidth = BANDWIDTH_8_MHZ;
            }
            else
            {
                *frequency = 0;
            }
            break;
        case COUNTRY_TYPE_RUSSIAN:
            if ((chnum >= 2) && (chnum <= 4))   //50.5,~ 71.5 Mhz
            {
                *frequency = 50500L + (chnum - 2) * 7000L;
                *bandwidth = BANDWIDTH_7_MHZ;
            }
            else if ((chnum >= 5) && (chnum <= 12))   //177.5 ~ 226.5 Mhz
            {
                *frequency = 178000L + (chnum - 5) * 8000L;
                *bandwidth = BANDWIDTH_8_MHZ;
            }
            else if ((chnum >= 21) && (chnum <= 70))   //474 ~ 858 Mhz
            {
                *frequency = 474000L + (chnum - 21) * 8000L;
                *bandwidth = BANDWIDTH_8_MHZ;
            }
            else
            {
                *frequency = 0;
            }
            break;
        //-------------------------------------//
        case COUNTRY_TYPE_AUSTRALIA:
        {
            int8_t s8Index = FREQ_TAL_SIZE - 1;

            for (s8Index = FREQ_TAL_SIZE - 1; s8Index >= 0; s8Index--)
            {
                if (chnum >= FreqTbl[s8Index].u8CHNo)
                {
                    break;
                }
            }
            if (s8Index >= 0 && FreqTbl[s8Index].u32Freq > 0)
            {
                *frequency = FreqTbl[s8Index].u32Freq + (chnum - FreqTbl[s8Index].u8CHNo) * 7000L;
            }
            else
            {
                *frequency = 0;
            }
            *bandwidth = BANDWIDTH_7_MHZ;
            break;
        }

        //-------------------------------------//

        //-------------------------------------//
        case COUNTRY_TYPE_TAIWAN:
            if ((chnum >= 24) && (chnum <= 36))   //533 ~ 605 Mhz
            {
                *frequency = 533000L + (chnum - 24) * 6000L;
                *bandwidth = BANDWIDTH_6_MHZ;
            }
            else
            {
                *frequency = 0;
            }
            break;

        //-------------------------------------//
        case 	COUNTRY_TYPE_EST:
        case 	COUNTRY_TYPE_LAT:
        case 	COUNTRY_TYPE_LTU:
        {
            if ((chnum >= 2) && (chnum <= 4))   //50.5,~ 71.5 Mhz
            {
                *frequency = 50500L + (chnum - 2) * 7000L;
                *bandwidth = BANDWIDTH_7_MHZ;
            }
            else if ((chnum >= 5) && (chnum <= 12))   //177.5 ~ 226.5 Mhz
            {
                *frequency = 177500L + (chnum - 5) * 7000L;
                *bandwidth = BANDWIDTH_7_MHZ;
            }
            else if ((chnum >= 13) && (chnum <= 20))   //234 ~466 Mhz
            {
                *frequency = 234000L + (chnum - 13) * 8000L;
                *bandwidth = BANDWIDTH_8_MHZ;
            }

            else if ((chnum >= 21) && (chnum <= 92))   //306 ~ 858 Mhz
            {
                *frequency = 306000L + (chnum - 22) * 8000L;
                *bandwidth = BANDWIDTH_8_MHZ;
            }
            else
            {
                *frequency = 0;
            }
        }

        break;
        case COUNTRY_TYPE_VIET:
        {
            if ((chnum >= 6) && (chnum <= 13))   //178 ~ 234 Mhz
            {
                *frequency = 178000L + (chnum - 6) * 8000L;
                *bandwidth = BANDWIDTH_8_MHZ;
            }
            else if ((chnum >= 21) && (chnum <= 70))   //474 ~ 858 Mhz
            {
                *frequency = 474000L + (chnum - 21) * 8000L;
                *bandwidth = BANDWIDTH_8_MHZ;
            }
            else
            {
                *frequency = 0;
            }
        }
        break;
        case COUNTRY_TYPE_OTHER:
            *frequency  = 98000 + (chnum * 8000L);
            *bandwidth = BANDWIDTH_8_MHZ;
            //-------------------------------------//
            break;
        default:
            *frequency = 0;
            break;
    }

    /* Frequency and symbol rate already verified in Scan Menu.
       0 = default value. */
    if (*frequency != 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
uint32_t app_get_next_channel_num_autosearch(uint8_t country, uint8_t chnum)
{
    //--------------------------------------------
    //wyf 20090324 to avoid invalid channel number when manul scan
    uint8_t u8MaxRFCh, u8MinRFCh;

    u8MaxRFCh = app_get_max_channel_num_autosearch(country);
    u8MinRFCh = app_get_min_channel_num_autosearch(country);
    if (chnum == u8MaxRFCh)
    {
        chnum = u8MinRFCh;
        return chnum;
    }
    //---------------------------------------------
    switch (country)
    {

        case COUNTRY_TYPE_COL:
        {
            if (chnum >= u8MinRFCh && chnum < 13)
            {
                chnum++;
            }
            else if (chnum >= 13 && chnum < 14)
            {
                chnum =  14;
            }
            else if (chnum >= 14 && chnum < u8MaxRFCh)
            {
                chnum++;
            }
            else
            {
                chnum = u8MinRFCh;
            }

            return chnum;
        }

        case COUNTRY_TYPE_CHILE:
        case COUNTRY_TYPE_OTHER:
        case COUNTRY_TYPE_PERU:
        case COUNTRY_TYPE_BRAZIL:
        case COUNTRY_TYPE_ARGENTINA:
        {
            if (chnum < u8MaxRFCh)
            {
                chnum++;
            }
            else
            {
                chnum =  u8MinRFCh;
            }
            return chnum;
        }
        //------------------------------------------//
        case COUNTRY_TYPE_HONGKONG:
        case COUNTRY_TYPE_CHINA:
        {
            if (chnum < u8MaxRFCh)
            {
                chnum++;
            }
            else
            {
                chnum =  21;
            }

            return chnum;
        }
        //------------------------------------------//

        case COUNTRY_TYPE_THAI:
        {
            if (chnum >= u8MinRFCh && chnum <= 20)
            {
                chnum++;
            }

            else if (chnum >= 21 && chnum < u8MaxRFCh)
            {
                chnum++;
            }
            else
            {
                chnum = u8MinRFCh;
            }

            return chnum;
        }
        case COUNTRY_TYPE_INA:
        {
            if (chnum >= u8MinRFCh && chnum < 15)
            {
                chnum++;
            }
            else if (chnum >= 15 && chnum < 21)
            {
                chnum =  21;
            }
            else if (chnum >= 21 && chnum < u8MaxRFCh)
            {
                chnum++;
            }
            else
            {
                chnum = u8MinRFCh;
            }
            return chnum;
        }
        case COUNTRY_TYPE_RUSSIAN:
        case COUNTRY_TYPE_UK:
        case COUNTRY_TYPE_EUROPE:
        case COUNTRY_TYPE_PHL:
        {
            if (chnum >= u8MinRFCh && chnum < 12)
            {
                chnum++;
            }
            else if (chnum >= 12 && chnum < 21)
            {
                chnum =  21;
            }
            else if (chnum >= 21 && chnum < u8MaxRFCh)
            {
                chnum++;
            }
            else
            {
                chnum = u8MinRFCh;
            }

            return chnum;
        }
        case COUNTRY_TYPE_LTU:
        case COUNTRY_TYPE_LAT:
        case COUNTRY_TYPE_EST:
        {
            if (chnum < u8MaxRFCh)
            {
                chnum++;
            }
            else
            {
                chnum =  u8MinRFCh;
            }
            return chnum;

        }
        break;
        case COUNTRY_TYPE_VIET:
        {
            if (chnum >= u8MinRFCh && chnum < 13)
            {
                chnum++;
            }
            else if (chnum >= 13 && chnum < 21)
            {
                chnum =  21;
            }
            else if (chnum >= 21 && chnum < u8MaxRFCh)
            {
                chnum++;
            }
            else
            {
                chnum = u8MinRFCh;
            }

            return chnum;
        }
        break;
        //------------------------------------------//
        case COUNTRY_TYPE_AUSTRALIA:
        {
            if (chnum < 13)
            {
                chnum++;
            }
            else if (chnum >= 13 && chnum < 21)
            {
                chnum =  21;
            }
            else if (chnum >= 21 && chnum < u8MaxRFCh)
            {
                chnum++;
            }
            else
            {
                chnum =  u8MinRFCh;
            }

            return chnum;
        }
        //------------------------------------------//
        case COUNTRY_TYPE_TAIWAN:
        {
            if (chnum >= 24 && chnum < 35)
            {
                chnum++;
            }
            else
            {
                chnum = 24;
            }
            return chnum;
        }

        default:
            return 0;
    }
}

uint32_t app_get_prev_channel_num(uint8_t country, uint8_t chnum)
{
    //--------------------------------------------
    //wyf 20090324 to avoid  invalid channel number when manul scan
    uint8_t u8MaxRFCh, u8MinRFCh;

    u8MaxRFCh = app_get_max_channel_num(country);
    u8MinRFCh = app_get_min_channel_num(country);
    if (chnum == u8MinRFCh)
    {
        chnum = u8MaxRFCh;
        return chnum;
    }
    //---------------------------------------------
    switch (country)
    {
        case COUNTRY_TYPE_CHILE:
        case COUNTRY_TYPE_OTHER:
        case COUNTRY_TYPE_PERU:
        case COUNTRY_TYPE_BRAZIL:
        case COUNTRY_TYPE_ARGENTINA:
        {
            if (chnum > u8MinRFCh)
            {
                chnum--;
            }
            else
            {
                chnum =  u8MaxRFCh;
            }
            return chnum;
        }
        //------------------------------------------//
        case COUNTRY_TYPE_HONGKONG:
        case COUNTRY_TYPE_CHINA:
        {
            if (chnum > 21)
            {
                chnum--;
            }
            else
            {
                chnum =  u8MaxRFCh;
            }

            return chnum;
        }
        case COUNTRY_TYPE_THAI:
        {
            if (chnum > u8MinRFCh)
            {
                chnum--;
            }
            else
            {
                chnum =  u8MaxRFCh;
            }
            return chnum;
        }
        //------------------------------------------//
        case COUNTRY_TYPE_UK:
        case COUNTRY_TYPE_RUSSIAN:
        case COUNTRY_TYPE_EUROPE:
        case COUNTRY_TYPE_LTU:
        case COUNTRY_TYPE_LAT:
        case COUNTRY_TYPE_EST:
        case COUNTRY_TYPE_VIET:
        case COUNTRY_TYPE_COL:
        case COUNTRY_TYPE_PHL:
        case COUNTRY_TYPE_INA:
        {
            if (chnum > u8MinRFCh)
            {
                chnum--;
            }
            else
            {
                chnum =  u8MaxRFCh;
            }
            return chnum;
        }
        //------------------------------------------//
        case COUNTRY_TYPE_AUSTRALIA:
        {
            if (chnum > u8MinRFCh && chnum <= 13)
            {
                chnum--;
            }
            else if (chnum > 13 && chnum <= 21)
            {
                chnum =  13;
            }
            else if (chnum > 21 && chnum <= u8MaxRFCh)
            {
                chnum--;
            }
            else
            {
                chnum =  u8MaxRFCh;
            }
            return chnum;
        }
        //------------------------------------------//
        case COUNTRY_TYPE_TAIWAN:
        {
            if (chnum > 24 && chnum <= 35)
            {
                chnum--;
            }
            else
            {
                chnum = 35;
            }
            return chnum;
        }

        break;
        default:
            return 0;
    }
}

uint32_t app_initial_channels_by_country(uint32_t *total_num)
{
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
        //app_log_debug("%d:%d:(%d/%d)\n",country,channel_num,frequency,bandwidth);
        Tp_list[fre_num].bandwidth = (int)bandwidth;
        Tp_list[fre_num].frequency = frequency;
        Tp_list[fre_num].ch_num = channel_num;
        fre_num ++;
        channel_num = app_get_next_channel_num_autosearch(country, channel_num);
    }
    while (channel_num < u8MaxRFCh);
    if (NULL != total_num)
        *total_num = fre_num;
    return 0;
}
int app_chnum2chstr(int chnum, char *channel, int country_type)
{
    bool					ACh = FALSE;
    uint8_t 				ChNo = chnum;

    if (country_type >= COUNTRY_MAX)
    {
        country_type = 0;
    }

    if (COUNTRY_TYPE_AUSTRALIA == country_type)
    {
        int8_t Index = FREQ_TAL_SIZE - 1;

        for (Index = FREQ_TAL_SIZE - 1; Index >= 0; Index--)
        {
            if (ChNo >= FreqTbl[Index].u8CHNo)
            {
                break;
            }
        }
        if (Index >= 0)
        {
            if (FreqTbl[Index].u8CHNoOffset > 0)
            {
                ACh = TRUE;
                ChNo -= FreqTbl[Index].u8CHNoOffset;
            }
            else if (FreqTbl[Index].u8CHNoOffset <= 0)
            {
                ChNo += FreqTbl[Index].u8CHNoOffset;
            }
        }
        if (ACh)
        {
            //frq_num = ChNo;
            sprintf(channel, "%dA", ChNo);
        }
        else
        {
            sprintf(channel, "%d", ChNo);
        }
        return 0;
    }
    if (COUNTRY_TYPE_EUROPE == country_type)
    {
        sprintf(channel, "C%d", chnum);
        return 0;
    }
    if (COUNTRY_TYPE_THAI == country_type)
    {
        if ((0 <= chnum) && (chnum <= 20))
        {
            sprintf(channel, "S%d", chnum + 21);
        }
        else
        {
            sprintf(channel, "C%d", chnum);
        }

    }
    else
    {
        sprintf(channel, "%d", chnum);
    }
    return 0;
}
#endif
