/**
 * @file gxservices.h
 * @author lixb
 * @brief Goxceed Service Bus服务定义列表
 * @defgroup services_list Goxceed Bus Services List
 */
 /*@{*/
#ifndef __GX_SERVICES_H__
#define __GX_SERVICES_H__

__BEGIN_DECLS

#include "gxbus.h"
#include "gxavdev.h"
#include "gxmsg.h"
#include "service/gxconsole.h"
#include "service/gxgui_view.h"
#include "service/gxsolution.h"
#include "service/gxhotplug.h"
#include "service/gxplayer.h"
#include "service/gxsi.h"
#include "service/gxbook.h"
#include "service/gxupdate.h"

/**服务总线控制台服务*/
extern GxServiceClass console_app_service;
/**DVB EPG服务*/
extern GxServiceClass epg_service;
/**ATSC EPG服务*/
extern GxServiceClass atsc_epg_service;
/**ADCANCE EPG服务*/
extern GxServiceClass advance_epg_service;
/**ADVANCE SEARCH服务*/
extern GxServiceClass advance_search_service;
/** blind SEARCH服务*/
extern GxServiceClass blind_search_service;

/**GUI VIEW服务*/
extern GxServiceClass gui_view_service;
/**USB/MMC HOT PLUG服务*/
extern GxServiceClass hotplug_service;
/**HDMI HOT PLUG服务*/
extern GxServiceClass hdmi_hotplug_service;
#ifdef ECOS_OS
/**USBWIFI服务*/
extern GxServiceClass usbwifi_hotplug_service;
/**USB3G服务*/
extern GxServiceClass usb3g_hotplug_service;
/**USBNET服务*/
extern GxServiceClass usbnet_hotplug_service;
/**USBSERIAL服务*/
extern GxServiceClass usbserial_hotplug_service;
/**ETH HOT PLUG服务*/
extern GxServiceClass eth_hotplug_service;
/**USB BT PLUG服务*/
extern GxServiceClass usbbt_hotplug_service;
/**USB audio PLUG服务*/
extern GxServiceClass usbaudio_hotplug_service;
/**USB hid PLUG服务*/
extern GxServiceClass usbhid_hotplug_service;
/**USB uvc PLUG服务*/
extern GxServiceClass usbuvc_hotplug_service;
#endif
#ifdef LINUX_OS
/**WIFI状态服务*/
extern GxServiceClass net_status_service;
#endif
/**播放器服务*/
extern GxServiceClass player_service;
/**DVB SEARCH服务*/
extern GxServiceClass search_service;
/**DVB SI服务*/
extern GxServiceClass si_service;
/**DVB 锁频服务*/
extern GxServiceClass frontend_service;
/**预约服务*/
extern GxServiceClass book_service;
extern GxServiceClass extra_service;
extern GxServiceClass solution_service;

/**Goxceed在线升级服务*/
extern GxServiceClass update_service;
extern GxServiceClass gxtest_service;

extern GxServiceClass box_service;
/* ATSC SEARCH */
extern GxServiceClass Atsc_search_service;

extern GxServiceClass chiptest_service;

extern GxServiceClass serial_service;
/* Debug */
extern GxServiceClass debug_service;

extern GxServiceClass net_service;

extern GxServiceClass vsame_service;

__END_DECLS

#endif
/*@}*/
