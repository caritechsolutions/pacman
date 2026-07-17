#include "app.h"
#include "app_module.h"
#include "app_windows.h"
#include "app_eutel_information.h"
#include <time.h>
#if RICHEPG_SUPPORT
#include "richepg.h"
#include "richepg_store.h"
#include "app_eutel_common.h"
#ifdef ECOS_OS
#include "devapi/chip_info.h"
#endif

#define INFO_MAX_LEN    64
static char s_info_buf[INFO_MAX_LEN] = {0};

// Eutelsat requires that the certified version must be displayed.
static char *_get_sattv_ver(void)
{
#define SATTV_VERSION "Sat.tv_V3.0.0"
    return SATTV_VERSION;
}

static char *_get_manufacturer_name(void)
{
    return "NationalChip";
}

static char *_get_stb_sn(void)
{
    return "-";
}

static char *_get_data_str(time_t t)
{
    if (t == 0)
        return "-";

    static char data[32];
    memset(data, 0, sizeof(data));
    struct tm *p = gmtime(&t);
#if 0
    strftime(data, sizeof(data), "%Y-%m-%d %H:%M:%S", p);
#else
    strftime(data, sizeof(data), "%a %b %d %H:%M:%S %Y", p);
#endif
    return data;
}

static char *_get_timestamp_str(enum richepg_store_id id)
{
    enum richepg_store_id state_id = RICHEPG_ECL_STATE;
    switch (id)
    {
        case RICHEPG_EPG_START_TIME:
        case RICHEPG_EPG_END_TIME:
        case RICHEPG_EPG_TIMESTAMP:
            state_id = RICHEPG_EPG_STATE;
            break;
        default:
            break;
    }
    if (0 == richepg_store_getint(state_id)) {
        return "-";
    }
    return _get_data_str(richepg_store_getint(id));
}

static char *_get_version_and_timestamp_str(enum richepg_store_id ver, enum richepg_store_id id)
{
    if (0 == richepg_store_getint(RICHEPG_ECL_STATE)) {
        return "-";
    }
    memset(s_info_buf, 0, INFO_MAX_LEN);
    snprintf(s_info_buf, INFO_MAX_LEN, "%s / %d", _get_data_str(richepg_store_getint(id)), richepg_store_getint(ver));
    return s_info_buf;
}

static char *_get_string_str(enum richepg_store_id id)
{
    if (0 == richepg_store_getint(RICHEPG_ECL_STATE)) {
        return "-";
    }
    memset(s_info_buf, 0, INFO_MAX_LEN);
    if (richepg_store_getstr(id, s_info_buf, INFO_MAX_LEN) == NULL) {
        return "-";
    }
    return s_info_buf;
}

static char *_get_lineup_name(void)
{
    return _get_string_str(RICHEPG_LINEUP_NAME);
}

static char *_get_elld(void)
{
    return _get_timestamp_str(RICHEPG_ELLD_LASTUPDATED);
}

static char *_get_customer_phone(void)
{
    return _get_string_str(RICHEPG_LINEUP_CUSTOMER_PHONE);
}

static char *_get_customer_website(void)
{
    return _get_string_str(RICHEPG_LINEUP_CUSTOMER_URL);
}

static char *_get_nit(void)
{
    return _get_version_and_timestamp_str(RICHEPG_NIT_VERSION, RICHEPG_NIT_LASTUPDATED);
}

static char *_get_bat(void)
{
    return _get_version_and_timestamp_str(RICHEPG_BAT_VERSION, RICHEPG_BAT_LASTUPDATED);
}

static char *_get_masterindex(void)
{
    return _get_version_and_timestamp_str(RICHEPG_MASTERINDEX_VERSION, RICHEPG_MASTERINDEX_LASTUPDATED);
}

static char *_get_elli(void)
{
    return _get_timestamp_str(RICHEPG_ELLI_LASTUPDATED);
}

static char *_get_ecl(void)
{
    return _get_timestamp_str(RICHEPG_ECL_LASTUPDATED);
}

static char *_get_ecf(void)
{
    return _get_timestamp_str(RICHEPG_ECF_LASTUPDATED);
}

static char *_get_ear(void)
{
    return _get_timestamp_str(RICHEPG_EAR_LASTUPDATED);
}

static char *_get_s_epg(void)
{
    return _get_timestamp_str(RICHEPG_EPG_START_TIME);
}

static char *_get_e_epg(void)
{
    return _get_timestamp_str(RICHEPG_EPG_END_TIME);
}

static char *_get_start_maintenance(void)
{
    return _get_data_str(app_richepg_start_maint_time_get());
}

static char *_get_finish_maintenance(void)
{
    char *str = NULL;
    if(richepg_store_getint(RICHEPG_EPG_TIMESTAMP))
        str = _get_timestamp_str(RICHEPG_EPG_TIMESTAMP);
    else
        str = _get_timestamp_str(RICHEPG_ECL_TIMESTAMP);
    return str;
}

static char *_get_mode(void)
{
    char *ecl_str = NULL;
    char *epg_str = NULL;

    if (0 == richepg_store_getint(RICHEPG_ECL_STATE)) {
        ecl_str = "-";
    } else if (richepg_get_profile_type(USB_ECL_TIMESTAMP) == RICHEPG_FULL_PROFILE){
        ecl_str = "Enhanced Mode";
    } else {
        ecl_str = "Standard Mode";
    }

    if (0 == richepg_store_getint(RICHEPG_EPG_STATE)) {
        epg_str = "-";
    } else if (richepg_get_profile_type(USB_EPG_TIMESTAMP) == RICHEPG_FULL_PROFILE){
        if(1 == richepg_store_getint(RICHEPG_RICH_MODE))
            epg_str = "Rich Mode";
        else
            epg_str = "Enhanced Mode";
    } else {
        epg_str = "Standard Mode";
    }

    snprintf(s_info_buf, INFO_MAX_LEN, "%s / %s", ecl_str, epg_str);
    return s_info_buf;
}

#ifdef ECOS_OS
#include <cyg/kernel/kapi.h>
static char *_get_usage(void)
{
    GxDiskInfo disk_info = {0};
    struct gx_malloc_info *heap = NULL;
    extern struct gx_malloc_info *gx_malloc_info_get(void);

    heap = (struct gx_malloc_info *)gx_malloc_info_get();
    if (GxCore_DiskInfo("/home/gx", &disk_info) == GXCORE_SUCCESS) {
        snprintf(s_info_buf, INFO_MAX_LEN, "%llu%% / %d%%",
                disk_info.used_size * 100 / disk_info.total_size,
                heap->heap_allocated * 100 / heap->heap_total);
    } else {
        snprintf(s_info_buf, INFO_MAX_LEN, "- / %d%%",
                heap->heap_allocated * 100 / heap->heap_total);
    }

    return s_info_buf;
}

#else
static char *_get_usage(void)
{
    GxDiskInfo disk_info = {0};

    if (GxCore_DiskInfo("/home/gx", &disk_info) == GXCORE_SUCCESS) {
        snprintf(s_info_buf, INFO_MAX_LEN, "%llu%%",
                disk_info.used_size * 100 / disk_info.total_size);
    } else {
        snprintf(s_info_buf, INFO_MAX_LEN, "-");
    }

    return s_info_buf;
}
#endif

static TableInfoItem s_epginfo_array[] = {
    {STR_ID_SATTV_VER,  &_get_sattv_ver         },
    {"Manufacturer",    &_get_manufacturer_name },
    {"STB S/N",         &_get_stb_sn            },
    {NULL,              NULL                    },
    {NULL,              NULL                    },
    {NULL,              NULL                    },
    {NULL,              NULL                    },
    {NULL,              NULL                    },
    {NULL,              NULL                    },
    {NULL,              NULL                    },
    {"Lineup",          &_get_lineup_name       },
    {"ELL-d",           &_get_elld              },
    {"Phone",           &_get_customer_phone    },
    {"Website",         &_get_customer_website  },
    {"NIT",             &_get_nit               },
    {"BAT",             &_get_bat               },
    {"MasterIndex",     &_get_masterindex       },
    {"ELL-i",           &_get_elli              },
    {"ECL",             &_get_ecl               },
    {"ECF",             &_get_ecf               },
    {"EAR",             &_get_ear               },
    {"Earliest EPG",    &_get_s_epg             },
    {"Latest EPG",      &_get_e_epg             },
    {"Start Maintenance", &_get_start_maintenance},
    {"Finish Maintenance", &_get_finish_maintenance},
    {"Mode(ECL/EPG)",   &_get_mode              },
#ifdef ECOS_OS
    {"Usage(Flash/Memory)", &_get_usage         },
#else
    {"Usage(Flash)",    &_get_usage             },
#endif
};

void app_richepginfo_menu_exec(void)
{
    TableInfoPara para = {0};
    para.title = STR_ID_FTAEPG_INFO;
    para.item_total = sizeof(s_epginfo_array) / sizeof(TableInfoItem);
    para.item_array = s_epginfo_array;
    app_table_info_create_dialog(&para);
}
#endif
