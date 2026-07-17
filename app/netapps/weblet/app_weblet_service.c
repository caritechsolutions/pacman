#include"app.h"
#if WEBLET_SUPPORT
#include "app_weblet_service.h"

void app_weblet_msg_register(void)
{
    GxBus_MessageRegister(APPMSG_WEBLET_UI_CONTROL, sizeof(AppMsg_WebletUIControl));
    app_log_debug("%s[%d]\n", __func__, __LINE__);
}

void app_weblet_msg_unregister(void)
{
    GxBus_MessageUnregister(APPMSG_WEBLET_UI_CONTROL);
    app_log_debug("%s[%d]\n", __func__, __LINE__);
}

#endif
