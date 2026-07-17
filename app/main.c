#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gui_core.h"
#include "gdi_core.h"
#include "gxcore.h"
#include "gxbus.h"
#include "gxservices.h"
#include "gxgui_view.h"
#include "app.h"
#include "app_config.h"
#include "module/frontend/gxfrontend_module.h"
#include "module/app_ca.h"
#include "module/app_frontend_init.h"
#include "app_utility.h"
#include "module/app_network_api.h"
#include "module/subtitle/gx_subtitle_setting.h"
#include "common/gx_memblk_pool.h"
#include "memmgr.h"
#include "app_module.h"
#include "devapi/chip_info.h"

#if NETAPPS_SUPPORT
#include <app_netapps_init.h>
#endif
#if BLUETOOTH_SUPPORT
#include "module/player/gxmedia_api.h"
#endif
#ifdef SCORPIO_TAURUS
#include "av/hal/gxav_hal_firmware.h"
#endif

#ifdef ECOS_OS
#include <network.h>
#include <dhcp.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <net/route.h>
#include <net/if_dl.h>
#include <netinet/in_pcb.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/ip_var.h>
#include <netinet/icmp_var.h>
#include <netinet/udp_var.h>
#include <netinet/tcp_var.h>
#include "module/app_frontend_init.h"
//#include <cyg/cpuload/cpuload.h>
#endif

extern void app_system_params_init(void);
extern void app_frontend_config_init(void);
extern status_t app_send_msg_init(void);
//extern void av_register_network(void);
//extern void av_register_file(void);
static GxMemBlockPool* thiz_PoolBucket = NULL;
#ifdef ECOS_OS
#if MINI_MEM_SUPPORT
#define STARTUP_STACK_SIZE		(10*1024)
#define USER_MINI_MEM_DEMO_SUPPORE  0
#else
#define STARTUP_STACK_SIZE		(100*1024)
#define USER_MINI_MEM_DEMO_SUPPORE  1
#endif
#else
#define STARTUP_STACK_SIZE		(200*1024)
#endif

#ifdef ECOS_OS
//#define STACK_INFO
#ifdef STACK_INFO
#include <cyg/kernel/kapi.h>
//#include <cyg/cpuload/cpuload.h>
//
void gx_rtc_delay_ms(int ms)
{
   cyg_thread_delay((ms>10?ms/10:1));
}

void _stack_check_info(char *s, int line)
{
	struct gx_malloc_info *heap;
	extern struct gx_malloc_info *gx_malloc_info_get(void);

	heap = (struct gx_malloc_info *)gx_malloc_info_get();
	app_log_info("--------------**start**------------------\n");
	app_log_info("\n function = %s, line = %d\n",s,line);
	app_log_info("heap.heap_total = 0x%x\n",heap->heap_total);
	app_log_info("heap.heap_blocks = 0x%x\n",heap->heap_blocks);
	app_log_info("heap.heap_allocated = 0x%x\n",heap->heap_allocated);
	app_log_info("heap.heap_free = 0x%x\n",heap->heap_free);
	app_log_info("heap.heap_maxfree = 0x%x\n",heap->heap_maxfree);
	app_log_info("--------------**end**------------------\n");
	return;
}

static void stack_check_thread(void*arg)
{
	#if 0
	cyg_uint32 calibration;
	cyg_cpuload_t cpuload;
	cyg_handle_t handle;
	cyg_uint32 average_point1s;
	cyg_uint32 average_1s;
	cyg_uint32 average_10s;

	cyg_cpuload_calibrate(&calibration);
	cyg_cpuload_create(&cpuload, calibration, &handle);
	#endif

	struct gx_malloc_info *heap;
	extern struct gx_malloc_info *gx_malloc_info_get(void);
	while(1)
	{
		#if 0 // CPU Load Begin
		cyg_cpuload_get(handle,&average_point1s,&average_1s,&average_10s);
		app_log_info("\n------------------------------------\n");
		app_log_info("cpuload average 0.1s = %d%%\n", average_point1s);
		app_log_info("cpuload average 1s = %d%%\n", average_1s);
		app_log_info("cpuload average 10s = %d%%\n", average_10s);
		#endif

		#if 1 // Memory Heap
		heap = (struct gx_malloc_info *)gx_malloc_info_get();
		app_log_info("[heap]: blocks = %d Blocks, maxfree = %d Bytes, free = %d Bytes, allocated = %d Bytes, total = %d Bytes\n",
				heap->heap_blocks, heap->heap_maxfree, heap->heap_free, heap->heap_allocated, heap->heap_total);
        GuiPool_Info();
        GxCore_MemblockPool_Info(thiz_PoolBucket);
		//GxCore_MemoryShowDebug(1);
		#endif

		#if 0 // Chip ID
		extern int new_vip_chip_id_read(void);
		app_log_info("Chip id: --------------\n");
		app_log_info("Chip id: %d\n", new_vip_chip_id_read());
		app_log_info("Chip id: --------------\n");

		#endif

		#if 0 // Thread Stack
		cyg_handle_t thread = 0;
        cyg_uint16 id = 0;

        app_log_info("Id\tState\tPrio.\tStack Base\tStack Usage\tName\n");
        while( cyg_thread_get_next( &thread, &id ) )
        {
			cyg_thread_info info;
			char *state_string;

			cyg_thread_get_info( thread, id, &info );

			if( info.name == NULL )
				info.name = "----------";

			/* Translate the state into a string.
			*/
			if( info.state == 0 )
				state_string = "RUN";
			else if( info.state & 0x04 )
				state_string = "SUSP";
			else switch( info.state & 0x1b )
			{
				case 0x01: state_string = "SLEEP"; break;
				case 0x02: state_string = "CNTSLEEP"; break;
				case 0x08: state_string = "CREATE"; break;
				case 0x10: state_string = "EXIT"; break;
				default: state_string = "????"; break;
			}

			app_log_info("%d\t", id);
			app_log_info("%s\t", state_string);
			app_log_info("%d/%d\t", info.cur_pri, info.set_pri);
			app_log_info("0x%08x\t", info.stack_base);
			if(info.stack_used > info.stack_size * 4/5)
				app_log_info("O!");
			else if(info.stack_used < info.stack_size * 1/5)
				app_log_info("W!");
			else
				app_log_info("--");
			app_log_info("%d/%d\t", info.stack_used, info.stack_size);
			app_log_info("%s\t", info.name);
			app_log_info("\n");
		}
		app_log_info("-------------------------------------\n");
		#endif

        #if 0//#if NETWORK_SUPPORT // network
        app_log_info("Interfaces\n" );
		struct ifaddrs *iflist, *ifp;

    	if(getifaddrs(&iflist)==0)
        {
            char addr[64];
            int i;
            ifp = iflist;

            while( ifp != (struct ifaddrs *)NULL)
            {
                if (ifp->ifa_addr->sa_family != AF_LINK)
                {
					app_log_info("%s => {\n", ifp->ifa_name);
                    /* Get the interface's flags and display
                             	 * the interesting ones.
                             	*/
       				app_log_info( "\tFlags:" );
                    for( i = 0; i < 16; i++ )
					{
						switch( ifp->ifa_flags & (1<<i) )
						{
						default: break;
						case IFF_UP: app_log_info( " UP" ); break;
						case IFF_BROADCAST: app_log_info( " BROADCAST" ); break;
						case IFF_DEBUG: app_log_info( " DEBUG" ); break;
						case IFF_LOOPBACK: app_log_info( " LOOPBACK" ); break;
						case IFF_PROMISC: app_log_info( " PROMISCUOUS" ); break;
						case IFF_RUNNING: app_log_info( " RUNNING" ); break;
						case IFF_SIMPLEX: app_log_info( " SIMPLEX" ); break;
						case IFF_MULTICAST: app_log_info( " MULTICAST" ); break;
						}
                    }

					getnameinfo(ifp->ifa_addr, sizeof(*ifp->ifa_addr),
					            addr, sizeof(addr), NULL, 0, NI_NUMERICHOST);
					app_log_info( "\n\tAddress:%s\n", addr);

                    if (ifp->ifa_netmask)
                    {
						getnameinfo(ifp->ifa_netmask, sizeof(*ifp->ifa_netmask),
						          addr, sizeof(addr), NULL, 0, NI_NUMERICHOST);
						app_log_info( "\tMask:%s\n", addr);
                    }

                    if (ifp->ifa_broadaddr)
                    {
						getnameinfo(ifp->ifa_broadaddr, sizeof(*ifp->ifa_broadaddr),
						          addr, sizeof(addr), NULL, 0, NI_NUMERICHOST);
						app_log_info( "\tBroadcast:%s\n", addr);
                    }
                	app_log_info("\t};\n");
				}
                ifp = ifp->ifa_next;
        	}

            freeifaddrs(iflist);
        }
		app_log_info("-------------------------------------\n");
		#endif

		GxCore_ThreadDelay(1500);
	}

}
#endif
#endif

#if NETWORK_SUPPORT
extern GxServiceClass app_network_service;
extern GxServiceClass app_nettime_service;
#endif

#if TKGS_SUPPORT
extern GxServiceClass tkgs_service;
#endif

extern GxServiceClass app_deamon_service;

extern GxServiceClass hdmi_hotplug_service;

#if AUTO_TEST_ENABLE
extern GxServiceClass service_autotest;
#endif

#if SAT2IP_SERVER_SUPPORT
extern GxServiceClass app_satip_service;
#endif

#ifdef ECOS_OS
#if VPN_SUPPORT
extern GxServiceClass vpn_status_service;
#endif
#if USB_MICROPHONE_SUPPORT
extern GxServiceClass usbaudio_hotplug_service;
#endif
#endif

static void app_player_module_register(void);

#if HDMI_CEC_SUPPORT
extern GxServiceClass app_cec_service;
#endif
#if VOICE_CONTROL_SUPPORT
#include "./voice_control/include/app_voice_control_api.h"
#endif

GxServiceClass g_service_list[SERVICE_TOTAL];
static void app_thread_init(void* arg)
{
	int i;

    GxCore_ThreadDetach();
	GxServiceClass* p_serv_list[SERVICE_TOTAL] =
    {
        &player_service,
        &frontend_service,
        &app_deamon_service,
        &si_service,
        &extra_service,
        &epg_service,
        &gui_view_service,
        &search_service,
        &book_service,
#if NETWORK_SUPPORT
        &app_network_service,
        &app_nettime_service,
#endif
        &hotplug_service,
#ifdef ECOS_OS
#if WIFI_SUPPORT
		&usbwifi_hotplug_service,
#endif
#if MOD_3G_SUPPORT
        &usb3g_hotplug_service,
#endif
#if ETH_SUPPORT
		&eth_hotplug_service,
#endif
#if USB_ETH_SUPPORT
		&usbnet_hotplug_service,
#endif
#if VPN_SUPPORT
		&vpn_status_service,
#endif
#if USB_TO_SERIAL_SUPPORT
        &usbserial_hotplug_service,
#endif
#if BLUETOOTH_SUPPORT
        &usbbt_hotplug_service,
#endif
#if USB_MICROPHONE_SUPPORT
        &usbaudio_hotplug_service,
#endif
#endif
        &update_service,
#if (DEMOD_DVB_S)
        &blind_search_service,
#endif

#if TKGS_SUPPORT
		&tkgs_service,
#endif
#if AUTO_TEST_ENABLE
		&service_autotest,
#endif
		&hdmi_hotplug_service,
#if SAT2IP_SERVER_SUPPORT
		&app_satip_service,
#endif
#if HDMI_CEC_SUPPORT
		&app_cec_service,
#endif
    };

    memcpy(&g_service_list[0], p_serv_list[0], sizeof(GxServiceClass*)*SERVICE_TOTAL);

#if SKIN_SUPPORT
    extern int gxapp_skin_flash_partition_init(void);
    gxapp_skin_flash_partition_init();
#endif
	gxapp_main_menu_theme_obj_init();

	app_send_msg_init(); //initial array app_send_msg[]
	app_system_params_init();
#if !OTT_SUPPORT
    app_frontend_config_init();
#endif

    // Player service init
    app_player_module_register();
    GxBookSetStopFlag(1);
	GxBus_Init(p_serv_list[0], 1);
#if USB_MICROPHONE_SUPPORT
{
    extern status_t app_microphone_register_mike_dogle(void);
    app_microphone_register_mike_dogle();
}
#endif
    // boot config
{
    extern void app_boot_config(void);
    app_boot_config();
}
#if BOOT_AD_SUPPORT
	int ad_sel = 0;
	GxBus_ConfigGetInt(WIDGET_TEXT_VIDEO_AD_KEY, &ad_sel, WIDGET_TEXT_VIDEO_AD_DEFAULT_KEY);
    if (1 == ad_sel)
    {
		GxCore_ThreadDelay(AD_GX_VIDEO_DEFAULT_TIMEOUT);
    }
    GxPlayer_MediaStop(AD_PLAYER);
#endif
    // GUI
{
    GuiViewAppCallback app_cb;
	app_cb.app_init = app_init;
	app_cb.app_msg_init = app_msg_init;
	app_cb.app_msg_destroy = app_msg_destroy;
	GxGuiViewRegisterApp(&app_cb);
}
    // Other service init
	hotplug_service.priority_offset = 1;
    //frontend_service.priority_offset = -1;
	for(i = 1; i < SERVICE_TOTAL; i++)
	{
        if(p_serv_list[i] == &gui_view_service)
        {
            // before ui init
            extern void app_boot_init_before_ui(void);
            app_boot_init_before_ui();
        }
		GxBus_ServiceCreate(p_serv_list[i]);
    }
#ifdef STACK_INFO
	stack_check_thread(NULL);
#endif
	return;
}

#ifdef LINUX_OS
#include <signal.h>
#include <assert.h>
static void sig_print(int signo)
{
    app_log_info("\033[1;31m--- [PID:%d] catch signo %d---\n\033[0m",getpid(), signo);
    signal(signo, SIG_DFL);
    kill(getpid(), signo);
}

static void init_signal(void)
{
    signal(SIGSEGV, sig_print);
    signal(SIGFPE, sig_print);
    signal(SIGSTKFLT, sig_print);
    signal(SIGHUP, sig_print);
    signal(SIGILL, sig_print);
    signal(SIGTRAP, sig_print);
    signal(SIGABRT, sig_print);
    signal(SIGBUS, sig_print);
    signal(SIGTERM, sig_print);

    signal(SIGPIPE, SIG_IGN);
}
#endif

static void app_player_module_register(void)
{
    //DVB
	GxPlayer_ModuleRegisterStreamDVB();//
#if SMART_VOLUME_SUPPORT
    GxPlayer_MoudleRegisterAudioResampler();
#endif
#if QUICK_SWITCH_SUPPORT
	GxPlayer_ModuleRegisterDVBSourceTSCache();
#endif
#if BLUETOOTH_SUPPORT
    GxMediaApi_AdecResampleRegister();
#endif
#if EX_SHIFT_SUPPORT
    GxPlayer_ModuleRegisterDVBSourceTSBuff();
#endif

// PVR
    GxPlayer_ModuleRegisterRecorderDVB();
#if SECURE_PVR_SUPPORT
    if(GxCore_MemProtectFlagGet() & GXMEM_FLAG_DEMUX_TSW)
    {
        GxPlayer_ModuleRegisterStreamHWCRYPTO();// pvr record protect
    }
#endif

// subtitle
    GxPlayer_ModuleRegisterSubtitleDVB();
#if ATSCCC_SUPPORT
    GxPlayer_ModuleRegisterSubtitleAtscCC();
#endif
#if ARIBCC_SUPPORT
    GxPlayer_ModuleRegisterSubtitleAribCC();
#endif
#if MEDIA_SUBTITLE_SUPPORT
	GxPlayer_ModuleRegisterSubtitleInside();
	GxPlayer_ModuleRegisterSubtitleOutside();
#endif
#ifdef COLOR_16BIT_TTX_SUPPORT
    GxPlayer_ModuleRegisterSubtitleTeletext();
#endif

    GX_SUBTITLE_SCREEN sr = {0, 0, APP_THEME_XRES, APP_THEME_YRES};
    GxSubtitle_ConfigScreen(sr);
    GX_SUBTITLE_CANVAS cv = {1280, 720};
    GxSubtitle_ConfigLimitSrcCV(cv);
    GX_SUBTITLE_FONT_LIB lib = {.str = "font_prog"};
    GxSubtitle_SetFontLib(lib);
    GxSubtitle_SetDisplayDefaultYB(50);

// media play
	GxPlayer_ModuleRegisterStreamFILE();//
	GxPlayer_ModuleRegisterStreamFIFO();// 读取/播放 FIFO 数据的功能
	GxPlayer_ModuleRegisterDemuxerMP3();
	GxPlayer_ModuleRegisterDemuxerMP4();
	GxPlayer_ModuleRegisterDemuxerAVI();
	GxPlayer_ModuleRegisterDemuxerMKV();
#if FM_SUPPORT
	GxPlayer_ModuleRegisterDemuxerWAV();
#endif
	GxPlayer_ModuleRegisterDemuxerFLV();
	GxPlayer_ModuleRegisterDemuxerAAC();
	GxPlayer_ModuleRegisterDemuxerSWTS();
	GxPlayer_ModuleRegisterDemuxerHWTS();
	GxPlayer_ModuleRegisterDemuxerPS ();
#if OGG_SUPPORT
	GxPlayer_ModuleRegisterDemuxerOGG();
#endif
#if DOLBY_SUPPORT
    GxPlayer_ModuleRegisterDemuxerDOLBY();
#endif
#if SAT2IP_SERVER_SUPPORT
	GxPlayer_ModuleRegisterStreamRINGMEM();// 播放循环内存数据的功能
#endif

#if FM_SUPPORT
	GxPlayer_ModuleRegisterStreamFM();
	GxPlayer_ModuleRegisterDemuxerFM();
#endif
// NET Stream
#if NETWORK_SUPPORT
	GxPlayer_ModuleRegisterStreamHTTP_V2();
	GxPlayer_ModuleRegisterStreamM3U8_V2();
	GxPlayer_ModuleRegisterStreamRTMP_V2();
	GxPlayer_ModuleRegisterStreamUDP_V2();
	GxPlayer_ModuleRegisterStreamRTP_V2();
	GxPlayer_ModuleRegisterStreamRTSP_V2();
	GxPlayer_ModuleRegisterStreamSWCRYPTO_V2();//https
#if NETAPPS_SUPPORT
	app_netapps_player_init();
#endif
#endif
}

#if MINI_MEM_SUPPORT
// FOR DEMUX CONFIG
static void _demux_minbuffer_cfg(void)
{
    int ret = 0;
    int dev = 0;
    int dmxmodule = 0;
    dev = GxAvdev_CreateDevice(0);
    if(dev <= 0)
    {
        app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
        return ;
    }
    dmxmodule = GxAvdev_OpenModule(dev, GXAV_MOD_DEMUX, 0);
    if(dmxmodule <= 0)
    {
        app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
        GxAvdev_DestroyDevice(dev);
        return ;
    }
    GxDemuxProperty_FilterMinBufSize config = {
        .psi_sw_bufsize = 5*1024,
        .psi_hw_bufsize = 16*1024,
        .pes_sw_bufsize = 0,//use default
        .pes_hw_bufsize = 0,//use default
    };

    ret =  GxAVSetProperty(dev, dmxmodule, GxDemuxPropertyID_FilterMinBufSizeConfig, &config, sizeof(GxDemuxProperty_FilterMinBufSize));
    if(ret < 0)
    {
        app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
    }
    GxAvdev_CloseModule(dev, dmxmodule);
    GxAvdev_DestroyDevice(dev);
}
#endif

#if DOLBY_SUPPORT
#define DOLBY_FILE_PATH "/usr/sbin/dolby.fw"
static void _dolby_regist(void)
{
    int dev = 0;
    FILE *pf = NULL;
    int len = 0;
    GxHwMallocObj hw_data = {0};
    // for file get
    if(access(DOLBY_FILE_PATH, F_OK) == 0)
    {
        pf = fopen(DOLBY_FILE_PATH, "rb");
        if(NULL == pf)
        {
            app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
            return ;
        }
        // get size
        fseek(pf, 0L, SEEK_END);
        len = ftell(pf);
        if(0 >= len)
        {
            app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
            fclose(pf);
            return;
        }
        fseek(pf, 0L, SEEK_SET);
        // get buffer
        memset(&hw_data, 0, sizeof(GxHwMallocObj));
        hw_data.size = len;
        GxCore_HwMalloc(&hw_data,MALLOC_WITH_CACHE);
        if(NULL == hw_data.usr_p)
        {
            app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
            fclose(pf);
            return;
        }
        len = fread((unsigned char*)hw_data.usr_p, len, 1, pf);
        if(1 != len)
        {
            app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
            fclose(pf);
            GxCore_HwFree(&hw_data);
            return;
        }

        dev = GxAvdev_CreateDevice(0);
        if(dev <= 0)
        {
            app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
            fclose(pf);
            GxCore_HwFree(&hw_data);
            return ;
        }

        GxAVAudioCodecRegister(dev, (unsigned char*)hw_data.usr_p, hw_data.size, GXAV_FMID_DOLBY);
        GxAvdev_DestroyDevice(dev);
        fclose(pf);
        GxCore_HwFree(&hw_data);
    }
    else
    {
        app_log_info("\nNO DOLBY\n");
    }
}
#endif

#ifdef SCORPIO_TAURUS
#define VIDEO_HEVC_FW_PATH "/lib/firmware/avfw/taurus/video/hevc.bin"
#define VIDEO_AVS_FW_PATH "/lib/firmware/avfw/taurus/video/avs.bin"
#define VIDEO_AVC_FW_PATH "/lib/firmware/avfw/taurus/video/avc.bin"
#define VIDEO_MEPG2_FW_PATH "/lib/firmware/avfw/taurus/video/mpeg2.bin"
#define VIDEO_MEPG4_FW_PATH "/lib/firmware/avfw/taurus/video/mpeg4.bin"
#if (AUDIO_DESCRIPTOR_SUPPORT < 1 && PLAYBACK_SPEED_SUPPORT < 1)
#define TAURUS_AUDIO_MPEG_FW_PATH "/lib/firmware/avfw/taurus/audio/mpeg.mini.bin"
#define SCORPIO_AUDIO_MPEG_FW_PATH "/lib/firmware/avfw/scorpio/audio/mpeg.mini.bin"
#else
#define SCORPIO_AUDIO_MPEG_FW_PATH "/lib/firmware/avfw/scorpio/audio/mpeg.bin"
#define TAURUS_AUDIO_MPEG_FW_PATH "/lib/firmware/avfw/taurus/audio/mpeg.bin"
#endif
#if (FLAC_SUPPORT && OGG_SUPPORT)
#define TAURUS_AUDIO_FOV_FW_PATH "/lib/firmware/avfw/taurus/audio/fov.bin"
#define SCORPIO_AUDIO_FOV_FW_PATH "/lib/firmware/avfw/scorpio/audio/fov.bin"
#elif FLAC_SUPPORT
#define TAURUS_AUDIO_FLAC_FW_PATH "/lib/firmware/avfw/taurus/audio/flac.bin"
#define SCORPIO_AUDIO_FLAC_FW_PATH "/lib/firmware/avfw/scorpio/audio/flac.bin"
#elif OGG_SUPPORT
#define TAURUS_AUDIO_OPUS_FW_PATH "/lib/firmware/avfw/taurus/audio/opus.bin"
#define SCORPIO_AUDIO_OPUS_FW_PATH "/lib/firmware/avfw/scorpio/audio/opus.bin"
#define TAURUS_AUDIO_VORBIS_FW_PATH "/lib/firmware/avfw/taurus/audio/vorbis.bin"
#define SCORPIO_AUDIO_VORBIS_FW_PATH "/lib/firmware/avfw/scorpio/audio/vorbis.bin"
#endif
#define SCORPIO_PUBLID_ID   0x3113
#define TAURUS_PUBLID_ID    0x6616
static void _av_fireware_regist(void)
{
	uint32_t chipid = 0;

	chipid = app_chip_id_get();
	if ((chipid == SCORPIO_PUBLID_ID) || (chipid == TAURUS_PUBLID_ID)) {
        if(chipid == TAURUS_PUBLID_ID)
        {
            GxFirewareHAL_RegisterFromFile(GXAV_FMID_VIDEO_HEVC, VIDEO_HEVC_FW_PATH, NULL);
            GxFirewareHAL_RegisterFromFile(GXAV_FMID_VIDEO_AVS, VIDEO_AVS_FW_PATH, NULL);
        }
        GxFirewareHAL_RegisterFromFile(GXAV_FMID_VIDEO_AVC, VIDEO_AVC_FW_PATH, NULL);
		GxFirewareHAL_RegisterFromFile(GXAV_FMID_VIDEO_MPEG2, VIDEO_MEPG2_FW_PATH, NULL);
		GxFirewareHAL_RegisterFromFile(GXAV_FMID_VIDEO_MPEG4, VIDEO_MEPG4_FW_PATH, NULL);
	}

	if (chipid == TAURUS_PUBLID_ID) {
		GxFirewareHAL_RegisterFromFile(GXAV_FMID_AUDIO_MPEG, TAURUS_AUDIO_MPEG_FW_PATH, NULL);
#if (FLAC_SUPPORT && OGG_SUPPORT)
        GxFirewareHAL_RegisterFromFile(GXAV_FMID_AUDIO_FOV, TAURUS_AUDIO_FOV_FW_PATH, NULL);
#elif FLAC_SUPPORT
        GxFirewareHAL_RegisterFromFile(GXAV_FMID_AUDIO_FOV, TAURUS_AUDIO_FLAC_FW_PATH, NULL);
#elif OGG_SUPPORT
        GxFirewareHAL_RegisterFromFile(GXAV_FMID_AUDIO_FOV, TAURUS_AUDIO_OPUS_FW_PATH, NULL);
        GxFirewareHAL_RegisterFromFile(GXAV_FMID_AUDIO_FOV, TAURUS_AUDIO_VORBIS_FW_PATH, NULL);
#endif
	} else if (chipid == SCORPIO_PUBLID_ID) {
		GxFirewareHAL_RegisterFromFile(GXAV_FMID_AUDIO_MPEG, SCORPIO_AUDIO_MPEG_FW_PATH, NULL);
#if (FLAC_SUPPORT && OGG_SUPPORT)
		GxFirewareHAL_RegisterFromFile(GXAV_FMID_AUDIO_FOV, SCORPIO_AUDIO_FOV_FW_PATH, NULL);
#elif FLAC_SUPPORT
        GxFirewareHAL_RegisterFromFile(GXAV_FMID_AUDIO_FOV, SCORPIO_AUDIO_FLAC_FW_PATH, NULL);
#elif OGG_SUPPORT
        GxFirewareHAL_RegisterFromFile(GXAV_FMID_AUDIO_FOV, SCORPIO_AUDIO_OPUS_FW_PATH, NULL);
        GxFirewareHAL_RegisterFromFile(GXAV_FMID_AUDIO_FOV, SCORPIO_AUDIO_VORBIS_FW_PATH, NULL);
#endif
	}
}
#endif

#if ETH_SUPPORT
static int enable_eth = 1;
int get_enable_eth(void)
{
	return enable_eth;
}
#endif

#if HIGH_DYNAMIC_RANGE_SUPPORT
static int thiz_hdr_enable = 1;
int app_get_hdr_enable(void)
{
    return thiz_hdr_enable;
}
#endif

#if (defined DYNAMIC_DRIVER_CONTROL)
#if (defined GEMINI)
#define GEMINI_PUBLID_ID 0x6701
#define GEMINI_CHIPNAME_6701   "6701-"
#define GEMINI_CHIPNAME_6702S5 "6702S5-"
#define GEMINI_CHIPNAME_6702H5 "6702H5-"
#define GEMINI_CHIPNAME_6702H1 "6702H1-"
#endif

#if (defined CYGNUS)
#define CYGNUS_PUBLID_ID 0x6705
#define CYGNUS_CHIPNAME_6705   "6705-"
#define CYGNUS_CHIPNAME_6706S5 "6706S5-"
#define CYGNUS_CHIPNAME_6706H5 "6706H5-"
#endif

#if USB_ETH_SUPPORT
static int enable_usbeth = 1;
#endif

void ecos_startup_config(void)
{
    uint32_t chipid = 0;
    char *chipname = NULL;

    chipid = app_chip_id_get();
    chipname = (char *)GxChip_GetChipName();
    if(!chipname)
        return;

// ETH & USBETH
#if (USB_ETH_SUPPORT && ETH_SUPPORT )
#if (defined GEMINI)
    if (chipid == GEMINI_PUBLID_ID)
    {
        if (strstr(chipname, GEMINI_CHIPNAME_6702H5) || strstr(chipname, GEMINI_CHIPNAME_6702H1))
        {
            enable_usbeth = 0;
            enable_eth = 1;
        }

        if (strstr(chipname, GEMINI_CHIPNAME_6701) || strstr(chipname, GEMINI_CHIPNAME_6702S5))
        {
            enable_usbeth = 1;
            enable_eth = 0;
        }
    }
#endif

#if (defined CYGNUS)
    if (chipid == CYGNUS_PUBLID_ID)
    {
        if (strstr(chipname, CYGNUS_CHIPNAME_6706H5))
        {
            enable_usbeth = 0;
            enable_eth = 1;
        }
        else
        {
            enable_usbeth = 1;
            enable_eth = 0;
        }
    }
#endif
#endif

#if USB_ETH_SUPPORT
    GxCore_ModuleControl(GX_MODULE_TYPE_USB_ETH, enable_usbeth ? GX_MODULE_CONTROL_ENABLE : GX_MODULE_CONTROL_DISABLE);
#endif
#if ETH_SUPPORT
    GxCore_ModuleControl(GX_MODULE_TYPE_ETH, enable_eth ? GX_MODULE_CONTROL_ENABLE : GX_MODULE_CONTROL_DISABLE);
#endif

// HDR
#if HIGH_DYNAMIC_RANGE_SUPPORT
#if (defined CYGNUS)
    if (chipid == CYGNUS_PUBLID_ID)
    {
        if (strstr(chipname, CYGNUS_CHIPNAME_6705))
        {
            thiz_hdr_enable = 0;
        }
        else if (strstr(chipname, CYGNUS_CHIPNAME_6706S5) || strstr(chipname, CYGNUS_CHIPNAME_6706H5))
        {
            thiz_hdr_enable = 1;
        }
    }
#endif
#endif
}
#endif

static void *app_xml_malloc(unsigned int size)
{
    return GxCore_MemblockPool_Alloc(thiz_PoolBucket, size);
}

static void *app_xml_realloc(void *block, unsigned int size)
{
    return GxCore_MemblockPool_Realloc(thiz_PoolBucket, block, size);
}

static void app_xml_free(void *block)
{
    GxCore_MemblockPool_Free(thiz_PoolBucket, block);
}


int GxCore_Startup(int argc, char **argv)
{
#define POOL_BUCKET_BLOCK  7
    GxMemblockPoolBucket block_info = {0};
    int bk_size[POOL_BUCKET_BLOCK] = {4, 8, 16, 32, 64, 128, 256};
    int bk_count[POOL_BUCKET_BLOCK] = {1300, 4100, 1000, 2000, 4000, 45, 10};
    xmlMemManager xmlMemFuncSet = {0};
    static int thread = 0;

    block_info.size_info_set = bk_size;
    block_info.count_info_set = bk_count;
    block_info.total_size = POOL_BUCKET_BLOCK;

    thiz_PoolBucket = GxCore_MemblockPool_Init(GX_MEMBLOCK_POOL_SIZE, &block_info);
    if(thiz_PoolBucket)
    {
        xmlMemFuncSet.alloc = app_xml_malloc;
        xmlMemFuncSet.realloc = app_xml_realloc;
        xmlMemFuncSet.free = app_xml_free;

        GXxmlRegisterMemFunc(&xmlMemFuncSet);
    }

#if GPRS_SUPPORT
    GxCore_PrintSwitch(false);
#endif

// DVB
#if MINI_MEM_SUPPORT
    _demux_minbuffer_cfg();
#endif

// player
#if DOLBY_SUPPORT
    _dolby_regist();
#endif

// av fireware
#ifdef SCORPIO_TAURUS
    _av_fireware_regist();
#endif

// debug
#ifdef LINUX_OS
    init_signal();
#endif
    app_debug_set(APP_NETWORK | APP_SUBT | APP_MISC | APP_NETAPPS);

// device control
	GxCore_ThreadCreate("init", &thread, app_thread_init, NULL,STARTUP_STACK_SIZE,GXOS_DEFAULT_PRIORITY);
	GxCore_Loop();

    return 0;
}

