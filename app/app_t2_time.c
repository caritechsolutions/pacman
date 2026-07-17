#include "app.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_book.h"
#include "app_module.h"
#include "app_windows.h"
#include "app_wnd_system_setting_opt.h"

#include "math.h"
#if (DEMOD_DVB_T) && (0 == DEMOD_DVB_S)
#include "app_t2_area_config.h"

#define CURRENTTIME_LENGTH	24


#define TIMEZONECONTENT     "[GMT -12,GMT -11.5,GMT -11,GMT -10.5,GMT -10,GMT -9.5,GMT -9,GMT -8.5,GMT -8,GMT -7.5,GMT -7,GMT -6.5,GMT -6,GMT -5.5,GMT -5,GMT -4.5,GMT -4,GMT -3.5,GMT -3,GMT -2.5,GMT -2,GMT -1.5,GMT -1,GMT -0.5,GMT +0,GMT +0.5,GMT +1,GMT +1.5,GMT +2,GMT +2.5,GMT +3,GMT +3.5,GMT +4,GMT +4.5,GMT +5,GMT +5.5,GMT +6,GMT +6.5,GMT +7,GMT +7.5,GMT +8,GMT +8.5,GMT +9,GMT +9.5,GMT +10,GMT +10.5,GMT +11,GMT +11.5,GMT +12]"

typedef struct _Zone
{
    char *RegionName;
    float ZoneValue;
} Zone;
typedef struct _RegionZone
{
    char *CountryName;
    int RegionNum;
    Zone *ZoneInfo;
} RegionZone;

enum REGIONTIME_ITEM_NAME
{
    ITEM_REGIONTIME_STATUS = 0,
    ITEM_REGIONTIME_REGION,
    ITEM_REGIONTIME_ZONE,
    ITEM_REGIONTIME_SUMMERTIME,
    ITEM_REGIONTIME_CURRENTTIME,
    ITEM_REGIONTIME_TOTAL
};

Zone EnglandZones[] = {{"London", 0}};
Zone RussiaZones[] = {{"Moscow", 3}, {"Saint Petersburg", 3}, {"Krasnodar", 3},
    {"Rostov-on-Don", 3}, {"Yekaterinburg", 5}, {"Novosibirsk", 6},
    {"Krasnoyarsk", 7}, {"Ulam Uhde", 8}, {"Vladivostok", 11}
};
Zone ThailandZones[] = {{"Bangkok", 7}};
Zone AustralisZones[] = {{"New South Wales", 10}, {"Victoria", 10},
    {"Queensland", 10}, {"Tasmania", 10},
    {"Western Australia", 8}, {"South Australia", 9.5},
    {"Northern Territory", 9.5}};
Zone IranZones[] = {{"Tehran", 3}};
Zone SpainZones[] = {{"Madrid", 1}, {"Canary", 0}};
Zone PortugalZones[] = {{"Lisbon", 0}};
Zone NetherlandsZones[] = {{"Amsterdam", 1}};
Zone SerbiaZones[] = {{"Beograd", 1}};
Zone GermanyZones[] = {{"Berlin", 1}};
Zone BelgiumZones[] = {{"Brussels", 1}};
Zone HungaryZones[] = {{"Budapest", 1}};
Zone DenmarkZones[] = {{"Copenhagen", 1}};
Zone SloveniaZones[] = {{"Ljubljana", 1}};
Zone LuxembourgZones[] = {{"Luxembourg", 1}};
Zone NorwayZones[] = {{"Oslo", 1}};
Zone FranceZones[] = {{"Paris", 1}};
Zone CzechZones[] = {{"Prague", 1}};
Zone ItalyZones[] = {{"Rome", 1}};
Zone SwedenZones[] = {{"Stockholm", 1}};
Zone PolandZones[] = {{"Warsaw", 1}};
Zone AustriaZones[] = {{"Vienna", 1}};
Zone CroatiaZones[] = {{"Zagreb", 1}};
Zone GreeceZones[] = {{"Athens", 2}};
Zone RumaniaZones[] = {{"Bucuresti", 2}};
Zone FinlandZones[] = {{"Helsinki", 2}};
Zone BulgariaZones[] = {{"Sofia", 2}};
Zone VietnamZones[] = {{"Ha Noi", 7}};
Zone MyanmarZones[] = {{"Naypyidaw", 6.5}};
Zone EstoniaZones[] = {{"Tallinn", 2}};
Zone IrelandZones[] = {{"Dublin", 0}};
Zone LatviaZones[] = {{"Riga", 2}};
Zone LithuaniaZones[] = {{"Vilnius", 2}};
Zone SlovakiaZones[] = {{"Bratislava", 1}};
Zone UgandaZones[] = {{"Kampala", 3}};
Zone UkraineZones[] = {{"Kiev", 2}};
Zone UzbekistanZones[] = {{"Tashkent city", 5}};
Zone KazakhstanZones[] = {{"Astana", 6}};
Zone TurkmenistanZones[] = {{"Ashgabat", 5}};
Zone TajikistanZones[] = {{"Dushanbe", 5}};
Zone KyrgyzstanZones[] = {{"Bishkek", 6}};
Zone ColombiaZones[] = {{"Bogota", -5}};
Zone LebanonZones[] = {{"Beirut", 2}};
Zone SingaporeZones[] = {{"Singapore", 8}};
Zone MalaysiaZones[] = {{"Kuala Lumpur", 8}};
Zone IsraelZones[] = {{"Jerusalem", 2}};
Zone ArmeniaZones[] = {{"Yerevan", 4}};
Zone AfghanistanZones[] = {{"Kabul", 5}};
Zone PhilippinesZones[] = {{"Malina", 8}};
Zone IndonesiaZones[] = {{"Jakarta", 7}};
Zone MongoliaZones[] = {{"Ulaanbaatar", 8}};
Zone GeorgiaZones[] = {{"Tbilisi", 4}};
Zone CongoZones[] = {{"Brazzaville", 1}};
Zone AlbaniaZones[] = {{"Tirana", 1}};
Zone BangladeshZones[] = {{"Dhaka", 6}};
Zone SaudiArabiaZones[] = {{"Riyadh", 3}};

static RegionZone TimeZone[] =
{
    {"England", 1, EnglandZones},
    {"Russia", 9, RussiaZones},
    {"Thailand", 1, ThailandZones},
    {"Australia", 7, AustralisZones},
    {"Iran", 1, IranZones},
    {"Spain", 2, SpainZones},
    {"Portugal", 1, PortugalZones},
    {"Netherlands", 1, NetherlandsZones},
    {"Serbia", 1, SerbiaZones},
    {"Germany", 1, GermanyZones},
    {"Belgium", 1, BelgiumZones},
    {"Hungary", 1, HungaryZones},
    {"Denmark", 1, DenmarkZones},
    {"Slovenia", 1, SloveniaZones},
    {"Luxembourg", 1, LuxembourgZones},
    {"Norway", 1, NorwayZones},
    {"France", 1, FranceZones},
    {"Czech", 1, CzechZones},
    {"Italy", 1, ItalyZones},
    {"Sweden", 1, SwedenZones},
    {"Poland", 1, PolandZones},
    {"Austria", 1, AustriaZones},
    {"Croatia", 1, CroatiaZones},
    {"Greece", 1, GreeceZones},
    {"Rumania", 1, RumaniaZones},
    {"Finland", 1, FinlandZones},
    {"Bulgaria", 1, BulgariaZones},
    {"Vietnam", 1, VietnamZones},
    {"Myanmar", 1, MyanmarZones},
    {"Estonia", 1, EstoniaZones},
    {"Ireland", 1, IrelandZones},
    {"Latvia", 1, LatviaZones},
    {"Lithuania", 1, LithuaniaZones},
    {"Slovakia", 1, SlovakiaZones},
    {"Uganda", 1, UgandaZones},
    {"Ukraine", 1, UkraineZones},
    {"Uzbekistan", 1, UzbekistanZones},
    {"Kazakhstan", 1, KazakhstanZones},
    {"Turkmenistan", 1, TurkmenistanZones},
    {"Tajikistan", 1, TajikistanZones},
    {"Kyrgyzstan", 1, KyrgyzstanZones},
    {"Colombia", 1, ColombiaZones},
    {"Lebanon", 1, LebanonZones},
    {"Singapore", 1, SingaporeZones},
    {"Malaysia", 1, MalaysiaZones},
    {"Israel", 1, IsraelZones},
    {"Armenia", 1, ArmeniaZones},
    {"Afghanistan", 1, AfghanistanZones},
    {"Philippines", 1, PhilippinesZones},
    {"Indonesia", 1, IndonesiaZones},
    {"Mongolia", 1, MongoliaZones},
    {"Georgia", 1, GeorgiaZones},
    {"Congo", 1, CongoZones},
    {"Albania", 1, AlbaniaZones},
    {"Bangladesh", 1, BangladeshZones},
    {"Saudi arabia", 1, SaudiArabiaZones},
};
static float TimeZoneArray[] = {-12, -11.5, -11, -10.5, -10, -9.5, -9, -8.5, -8, -7.5, -7, -6.5, -6, -5.5, -5, -4.5, -4, -3.5, -3, -2.5, -2, -1.5, -1, -0.5, 0, 0.5, 1, 1.5, 2, 2.5, 3, 3.5, 4, 4.5, 5, 5.5, 6, 6.5, 7, 7.5, 8, 8.5, 9, 9.5, 10, 10.5, 11, 11.5, 12};
#define TOTAL_ZONE_NUM  (sizeof(TimeZoneArray)/sizeof(float))
char gCurrentTime[CURRENTTIME_LENGTH] = {0};

static SystemSettingOpt s_RegionTime_opt;
static SystemSettingItem s_RegionTime_item[ITEM_REGIONTIME_TOTAL];
static char *sgRegionContent = NULL;
static int   sgTimeMode = 0;
static int   sgRegion_num = 0;
static int   sgSummerTime = 0;
static int   sgRegionValue = 0;
static int   sgZoneValue = 0;
static int   sgCurrentCountry = 0;

int app_RegionTime_get_region_content(int *Country);

extern int AppDvbt2GetCountry(void);

void app_region_value_init(void)
{
    sgRegionValue=0;
    GxBus_ConfigSetInt(T2_REGION_KEY, sgRegionValue);
}

static int32_t app_t2_get_half_zone(float ZoneValue)
{
    double Fraction, Integer;
    int32_t HalfZoneFlag = 0;
    Fraction = modf(ZoneValue, &Integer);
    if (Fraction > 0)
    {
        HalfZoneFlag = 1;
    }
    else if (Fraction < 0)
    {
        HalfZoneFlag = -1;
    }
    else
    {
        HalfZoneFlag = 0;
    }
    return HalfZoneFlag;
}

static void app_t2_check_half_zone(float ZoneValue)
{
    int32_t HalfZoneFlag = app_t2_get_half_zone(ZoneValue);
    GxBus_ConfigSetInt(TIME_HALF_ZONE, HalfZoneFlag);
}

int app_t2_time_zone_init(void)
{
    time_cfg time = {0};
    float TimeZoneValue = 0;
    int32_t ZoneValue = 0;
    int32_t HalfZoneValue = 0;
    int   CurrentCountry = 0;
    int   RegionValue = 0;
    int   TimeMode = 0;
    int   SummerTime = 0;

    GxBus_ConfigGetInt(TIME_MODE, &TimeMode, TIME_MODE_VALUE);
    GxBus_ConfigGetInt(TIME_SUMMER, &SummerTime, TIME_SUMMER_VALUE);
    GxBus_ConfigGetInt(T2_REGION_KEY, &RegionValue, T2_REGION_VALUE);
    app_RegionTime_get_region_content(&CurrentCountry);
    if (TimeMode == TIME_LOCAL)
    {
        GxBus_ConfigGetInt(TIME_ZONE, &ZoneValue, TIME_ZONE_VALUE);
        GxBus_ConfigGetInt(TIME_HALF_ZONE, &HalfZoneValue, TIME_HALF_ZONE_VALUE);

        TimeZoneValue = (ZoneValue + (float)HalfZoneValue / 2);
    }
    else if ((TimeMode == TIME_GMT)||(TimeMode == TIME_NET))
    {
        TimeZoneValue = TimeZone[CurrentCountry].ZoneInfo[RegionValue].ZoneValue;
    }

    time.ymd = TIME_YMD;
    time.gmt_local = TimeMode;
    time.summer = SummerTime;
    time.zone = (int32_t)TimeZoneValue;
    time.half_zone = app_t2_get_half_zone(TimeZoneValue);
    g_AppTime.mode_set(&g_AppTime, &time);

    return 0;
}

int app_t2_time_set_zone(int RegionValue)
{
    float ZoneValue;

    app_RegionTime_get_region_content(&sgCurrentCountry);
    ZoneValue = TimeZone[sgCurrentCountry].ZoneInfo[RegionValue].ZoneValue;
    app_t2_check_half_zone(ZoneValue);

    GxBus_ConfigSetInt(TIME_ZONE, (int32_t)ZoneValue);
    return (int)ZoneValue;
}

int app_t2_time_auto_set(void)
{
    int TimeMode;
    int init_value;

    GxBus_ConfigGetInt(TIME_MODE, &TimeMode, TIME_MODE_VALUE);
    if ((TimeMode == TIME_GMT)|| (TimeMode == TIME_NET))
    {
        GxBus_ConfigSetInt(T2_REGION_KEY, T2_REGION_VALUE);
        init_value = app_t2_time_set_zone(T2_REGION_VALUE);

        GxBus_ConfigSetInt(TIME_ZONE, init_value);
    }
    return 0;
}

int app_RegionTime_free_region_content(void)
{
    if (sgRegionContent != NULL)
    {
        GxCore_Free(sgRegionContent);
        sgRegionContent = NULL;
    }
    return 0;
}

int app_RegionTime_get_region_content(int *Country)
{
#define REGION_NAME_LEN	32
    int32_t CountryValue;
    int i = 0;
    int Zone_Max_Size = 0;
    int RegionNum = 0;
    int RegionCount = 0;
    int Ret = -1;
    char *CmbBuffer = NULL;
    char Buffer[REGION_NAME_LEN];

    app_RegionTime_free_region_content();
    CountryValue = AppDvbt2GetCountry();
    GET_ARRAY_LEN(TimeZone,RegionZone,Zone_Max_Size);

    for (i = 0; i < Zone_Max_Size; i++)
    {
        Ret = strcmp(g_CountryName[CountryValue], TimeZone[i].CountryName);
        if (Ret == 0)
        {
            RegionNum = TimeZone[i].RegionNum;
            *Country = i;
            break;
        }
    }
    if (i >= Zone_Max_Size)
    {
        RegionNum = TimeZone[0].RegionNum;
        *Country = 0;
        i = 0;
    }

    CmbBuffer = GxCore_Malloc(TimeZone[i].RegionNum * REGION_NAME_LEN);
    if (CmbBuffer != NULL)
    {
        memset(CmbBuffer, 0, TimeZone[i].RegionNum * REGION_NAME_LEN);
        strcpy(CmbBuffer, "[");
        for (RegionCount = 0; RegionCount < TimeZone[i].RegionNum; RegionCount++)
        {
            if (0 != RegionCount)
            {
                strcat(CmbBuffer, ",");
            }

            memset(Buffer, 0, REGION_NAME_LEN);
            sprintf(Buffer, "%s", TimeZone[i].ZoneInfo[RegionCount].RegionName);
            strcat(CmbBuffer, Buffer);
        }
        strcat(CmbBuffer,  "]");
    }

    sgRegionContent = CmbBuffer;

    return RegionNum;
}

static int app_RegionTime_Zone2sel(void)
{
    int i = 0;
    int32_t ZoneValue = 0;
    int32_t HalfZoneValue = 0;
    float CurrentZone = 0;
    static int CurrentZoneSel = 13;

    if (sgTimeMode == TIME_LOCAL)
    {
        GxBus_ConfigGetInt(TIME_ZONE, &ZoneValue, TIME_ZONE_VALUE);
        GxBus_ConfigGetInt(TIME_HALF_ZONE, &HalfZoneValue, TIME_HALF_ZONE_VALUE);

        CurrentZone = (ZoneValue + (float)HalfZoneValue / 2);
    }
    else if ((sgTimeMode == TIME_GMT)||(sgTimeMode == TIME_NET))
    {
        CurrentZone = TimeZone[sgCurrentCountry].ZoneInfo[sgRegionValue].ZoneValue;
    }
    for (i = 0; i < TOTAL_ZONE_NUM; i++)
    {
        if (TimeZoneArray[i] == CurrentZone)
        {
            CurrentZoneSel = i;
            break;
        }
    }
    sgZoneValue = CurrentZoneSel;
    return CurrentZoneSel;
}
static void app_RegionTime_show_current_time(void)
{
    g_AppTime.time_get(&g_AppTime, gCurrentTime, sizeof(gCurrentTime));
    s_RegionTime_item[ITEM_REGIONTIME_CURRENTTIME].itemProperty.itemPropertyBtn.string = gCurrentTime;
    app_system_set_item_property(ITEM_REGIONTIME_CURRENTTIME, &s_RegionTime_item[ITEM_REGIONTIME_CURRENTTIME].itemProperty);

    return ;
}

static int app_RegionTime_Status_change_callback(int sel)
{
    time_cfg time = {0};
    float TimeZoneValue;

    sgTimeMode = sel;

    s_RegionTime_item[ITEM_REGIONTIME_SUMMERTIME].itemProperty.itemPropertyCmb.sel = sgSummerTime;
    s_RegionTime_item[ITEM_REGIONTIME_REGION].itemProperty.itemPropertyCmb.sel = sgRegionValue;

    if ((sel == TIME_GMT)||(sel == TIME_NET))
    {
        if (sgRegion_num > 1)
        {
            app_system_set_item_state_update(ITEM_REGIONTIME_REGION, ITEM_NORMAL);
        }
        else
        {
            app_system_set_item_state_update(ITEM_REGIONTIME_REGION, ITEM_DISABLE);
        }

        app_system_set_item_state_update(ITEM_REGIONTIME_ZONE, ITEM_DISABLE);
        app_system_set_item_state_update(ITEM_REGIONTIME_SUMMERTIME, ITEM_NORMAL);

        TimeZoneValue = TimeZone[sgCurrentCountry].ZoneInfo[sgRegionValue].ZoneValue;
    }
    else
    {
        app_system_set_item_state_update(ITEM_REGIONTIME_REGION, ITEM_DISABLE);
        app_system_set_item_state_update(ITEM_REGIONTIME_ZONE, ITEM_NORMAL);
        app_system_set_item_state_update(ITEM_REGIONTIME_SUMMERTIME, ITEM_NORMAL);

        TimeZoneValue = TimeZoneArray[sgZoneValue];
    }
    time.ymd = TIME_YMD;
    time.gmt_local = sel;
    time.summer = sgSummerTime;
    time.zone = (int32_t)TimeZoneValue;
    time.half_zone = app_t2_get_half_zone(TimeZoneValue);
    g_AppTime.mode_set(&g_AppTime, &time);
    g_AppTime.start(&g_AppTime);

    s_RegionTime_item[ITEM_REGIONTIME_ZONE].itemProperty.itemPropertyCmb.sel = app_RegionTime_Zone2sel();
    app_system_set_item_property(ITEM_REGIONTIME_ZONE, &s_RegionTime_item[ITEM_REGIONTIME_ZONE].itemProperty);

    app_RegionTime_show_current_time();

    return EVENT_TRANSFER_STOP;
}

static int app_RegionTime_Region_change_callback(int sel)
{
    time_cfg time = {0};
    float TimeZoneValue = 0;

    GxBus_ConfigSetInt(T2_REGION_KEY, sel);
    sgRegionValue = sel;
    time.ymd = TIME_YMD;
    if ((sgTimeMode == TIME_GMT)||(sgTimeMode == TIME_NET))
    {
        TimeZoneValue = TimeZone[sgCurrentCountry].ZoneInfo[sel].ZoneValue;
    }
    else
    {
        TimeZoneValue = TimeZoneArray[sgZoneValue];
    }
    time.gmt_local = sgTimeMode;
    time.summer = sgSummerTime;
    time.zone = (int32_t)TimeZoneValue;
    time.half_zone = app_t2_get_half_zone(TimeZoneValue);
    g_AppTime.mode_set(&g_AppTime, &time);
    g_AppTime.start(&g_AppTime);

    s_RegionTime_item[ITEM_REGIONTIME_ZONE].itemProperty.itemPropertyCmb.sel = app_RegionTime_Zone2sel();
    app_system_set_item_property(ITEM_REGIONTIME_ZONE, &s_RegionTime_item[ITEM_REGIONTIME_ZONE].itemProperty);
    app_RegionTime_show_current_time();

    return EVENT_TRANSFER_STOP;
}
static int app_RegionTime_Zone_change_callback(int sel)
{
    time_cfg time = {0};
    float TimeZoneValue = 0;

    sgZoneValue = sel;
    time.ymd = TIME_YMD;
    TimeZoneValue = TimeZoneArray[sel];
    app_t2_check_half_zone(TimeZoneValue);
    time.gmt_local = sgTimeMode;
    time.summer = sgSummerTime;
    time.zone = (int32_t)TimeZoneValue;
    time.half_zone = app_t2_get_half_zone(TimeZoneValue);
    g_AppTime.mode_set(&g_AppTime, &time);
    g_AppTime.start(&g_AppTime);

    app_RegionTime_show_current_time();

    return EVENT_TRANSFER_STOP;
}

static int app_RegionTime_SummerTime_change_callback(int sel)
{
    time_cfg time = {0};
    float TimeZoneValue;

    sgSummerTime = sel;

    time.ymd = TIME_YMD;
    TimeZoneValue = TimeZoneArray[sgZoneValue];
    time.gmt_local = sgTimeMode;
    time.summer = sel;
    time.zone = (int32_t)TimeZoneValue;
    time.half_zone = app_t2_get_half_zone(TimeZoneValue);
    g_AppTime.mode_set(&g_AppTime, &time);
    g_AppTime.start(&g_AppTime);

    app_RegionTime_show_current_time();

    return EVENT_TRANSFER_STOP;
}
static int app_RegionTime_exit_callback(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;

    app_RegionTime_free_region_content();
    return ret;
}

static void app_RegionTime_Status_item_init(void)
{
    GxBus_ConfigGetInt(TIME_MODE, &sgTimeMode, TIME_MODE_VALUE);

    s_RegionTime_item[ITEM_REGIONTIME_STATUS].itemTitle = "Time Offset";
    s_RegionTime_item[ITEM_REGIONTIME_STATUS].itemType = ITEM_CHOICE;
#if GET_NET_TIME_SUPPORT
    s_RegionTime_item[ITEM_REGIONTIME_STATUS].itemProperty.itemPropertyCmb.content = "[DVB,Manual,Net]";
#else
    s_RegionTime_item[ITEM_REGIONTIME_STATUS].itemProperty.itemPropertyCmb.content = "[DVB,Manual]";
#endif
    s_RegionTime_item[ITEM_REGIONTIME_STATUS].itemProperty.itemPropertyCmb.sel = sgTimeMode;
    s_RegionTime_item[ITEM_REGIONTIME_STATUS].itemCallback.cmbCallback.CmbChange = app_RegionTime_Status_change_callback;
    s_RegionTime_item[ITEM_REGIONTIME_STATUS].itemStatus = ITEM_NORMAL;
}
static void app_RegionTime_Region_item_init(void)
{
    sgRegion_num = app_RegionTime_get_region_content(&sgCurrentCountry);
    GxBus_ConfigGetInt(T2_REGION_KEY, &sgRegionValue, T2_REGION_VALUE);

    s_RegionTime_item[ITEM_REGIONTIME_REGION].itemTitle = "Region";
    s_RegionTime_item[ITEM_REGIONTIME_REGION].itemType = ITEM_CHOICE;
    s_RegionTime_item[ITEM_REGIONTIME_REGION].itemProperty.itemPropertyCmb.content = sgRegionContent;
    s_RegionTime_item[ITEM_REGIONTIME_REGION].itemProperty.itemPropertyCmb.sel = sgRegionValue;
    s_RegionTime_item[ITEM_REGIONTIME_REGION].itemCallback.cmbCallback.CmbChange = app_RegionTime_Region_change_callback;
    if ((sgTimeMode == TIME_GMT)||(TIME_NET == sgTimeMode))
    {
        if (sgRegion_num > 1)
        {
            s_RegionTime_item[ITEM_REGIONTIME_REGION].itemStatus = ITEM_NORMAL;
        }
        else
        {
            s_RegionTime_item[ITEM_REGIONTIME_REGION].itemStatus = ITEM_DISABLE;
        }
    }
    else
    {
        s_RegionTime_item[ITEM_REGIONTIME_REGION].itemStatus = ITEM_DISABLE;
    }
}
static void app_RegionTime_Zone_item_init(void)
{
    s_RegionTime_item[ITEM_REGIONTIME_ZONE].itemTitle = "Time Zone";
    s_RegionTime_item[ITEM_REGIONTIME_ZONE].itemType = ITEM_CHOICE;
    s_RegionTime_item[ITEM_REGIONTIME_ZONE].itemProperty.itemPropertyCmb.content = TIMEZONECONTENT;
    s_RegionTime_item[ITEM_REGIONTIME_ZONE].itemProperty.itemPropertyCmb.sel = app_RegionTime_Zone2sel();
    s_RegionTime_item[ITEM_REGIONTIME_ZONE].itemCallback.cmbCallback.CmbChange = app_RegionTime_Zone_change_callback;

    if ((sgTimeMode == TIME_GMT)|| (sgTimeMode == TIME_NET))
    {
        s_RegionTime_item[ITEM_REGIONTIME_ZONE].itemStatus = ITEM_DISABLE;
    }
    else
    {
        s_RegionTime_item[ITEM_REGIONTIME_ZONE].itemStatus = ITEM_NORMAL;
    }
}
static void app_RegionTime_SummerTime_item_init(void)
{
    GxBus_ConfigGetInt(TIME_SUMMER, &sgSummerTime, TIME_SUMMER_VALUE);

    s_RegionTime_item[ITEM_REGIONTIME_SUMMERTIME].itemTitle = "Summer Time";
    s_RegionTime_item[ITEM_REGIONTIME_SUMMERTIME].itemType = ITEM_CHOICE;
    s_RegionTime_item[ITEM_REGIONTIME_SUMMERTIME].itemProperty.itemPropertyCmb.content = "[Off,On]";
    s_RegionTime_item[ITEM_REGIONTIME_SUMMERTIME].itemProperty.itemPropertyCmb.sel = sgSummerTime;
    s_RegionTime_item[ITEM_REGIONTIME_SUMMERTIME].itemCallback.cmbCallback.CmbChange = app_RegionTime_SummerTime_change_callback;
    s_RegionTime_item[ITEM_REGIONTIME_SUMMERTIME].itemStatus = ITEM_NORMAL;
}

static void app_RegionTime_CurrentTime_item_init(void)
{
    s_RegionTime_item[ITEM_REGIONTIME_CURRENTTIME].itemTitle = "Current Time";
    s_RegionTime_item[ITEM_REGIONTIME_CURRENTTIME].itemType = ITEM_PUSH;
    g_AppTime.time_get(&g_AppTime, gCurrentTime, sizeof(gCurrentTime));
    s_RegionTime_item[ITEM_REGIONTIME_CURRENTTIME].itemProperty.itemPropertyBtn.string = gCurrentTime;
    s_RegionTime_item[ITEM_REGIONTIME_CURRENTTIME].itemCallback.btnCallback.BtnPress = NULL;
    s_RegionTime_item[ITEM_REGIONTIME_CURRENTTIME].itemStatus = ITEM_NORMAL;
}

void app_dvbt_time_menu_exec(void)
{
    memset(&s_RegionTime_opt, 0, sizeof(s_RegionTime_opt));

    s_RegionTime_opt.menuTitle = STR_ID_REGION_TIME;
    s_RegionTime_opt.itemNum = ITEM_REGIONTIME_TOTAL;
    s_RegionTime_opt.topTipImgDisplay = TIP_HIDE;
    s_RegionTime_opt.bottmTipDisplay = TIP_HIDE;
    s_RegionTime_opt.timeDisplay = TIP_HIDE;
    s_RegionTime_opt.item = s_RegionTime_item;
    s_RegionTime_opt.exit = app_RegionTime_exit_callback;

    app_RegionTime_Status_item_init();
    app_RegionTime_Region_item_init();
    app_RegionTime_Zone_item_init();
    app_RegionTime_SummerTime_item_init();
    app_RegionTime_CurrentTime_item_init();

    app_system_set_create(&s_RegionTime_opt);
}
#endif
