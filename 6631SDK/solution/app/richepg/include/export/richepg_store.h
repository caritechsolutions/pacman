#ifndef __RICHEPG_STORE_H__
#define __RICHEPG_STORE_H__

#include "gxtype.h"
#include "module/config/gxconfig.h"
#include "richepg_json.h"
#include <time.h>

enum richepg_store_id
{
    RICHEPG_STORE_START = 0,

    RICHEPG_FLASH_STORE,

    RICHEPG_ECL_STATE,
    RICHEPG_EPG_STATE,
    RICHEPG_EAR_STATE,
    RICHEPG_ECL_TIMESTAMP,
    RICHEPG_EPG_TIMESTAMP,
    RICHEPG_EAR_TIMESTAMP,

    RICHEPG_BAT_VERSION,
    RICHEPG_BAT_LASTUPDATED,
    RICHEPG_NIT_VERSION,
    RICHEPG_NIT_LASTUPDATED,

    RICHEPG_MASTERINDEX_VERSION,
    RICHEPG_MASTERINDEX_LASTUPDATED,
    RICHEPG_ECF_LASTUPDATED,
    RICHEPG_ECL_LASTUPDATED,
    RICHEPG_ELLI_LASTUPDATED,
    RICHEPG_ELLD_LASTUPDATED,
    RICHEPG_EAR_LASTUPDATED,
    RICHEPG_EPG_START_TIME,
    RICHEPG_EPG_END_TIME,
    RICHEPG_EPG_FORCE_UPDATE,

    RICHEPG_LINEUP_ID,
    RICHEPG_LINEUP_TAG,
    RICHEPG_LINEUP_WAKEUP_CHANNEL,

    RICHEPG_LINEUP_NAME,
    RICHEPG_LINEUP_LOGO,
    RICHEPG_LINEUP_CUSTOMER_PHONE,
    RICHEPG_LINEUP_CUSTOMER_URL,
    RICHEPG_EAR_GLOGO,

    RICHEPG_USB_CACHEPATH,
    RICHEPG_LOCALE_LANGUAGE,

    RICHEPG_RICH_MODE,

    RICHEPG_STORE_END
};

enum usb_timestamp_type {
    USB_ECL_TIMESTAMP = 0,
    USB_EPG_TIMESTAMP,
    USB_EAR_TIMESTAMP
};

int richepg_store_init(int cfg_num);
int richepg_store_flush(void);
int richepg_store_getsel(void);
int richepg_store_setsel(int cfg_sel);
int richepg_store_getint(enum richepg_store_id id);
int richepg_store_setint(enum richepg_store_id id, int value);
char *richepg_store_getstr(enum richepg_store_id id, char *buf, size_t size);
int richepg_store_setstr(enum richepg_store_id id, char *value);
int richepg_store_setlocales(enum richepg_store_id id, int total, struct eutelsat_locale *locales);
time_t richepg_usb_timestamp_get(enum usb_timestamp_type type);
int richepg_usb_timestamp_set(enum usb_timestamp_type type, time_t timestamp);

#endif
