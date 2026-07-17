#include "app.h"
#include "app_module.h"
#include "app_msg.h"
#include "app_pop.h"
#include "gui_core.h"
#include "app_default_params.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "app_keyboard_language.h"
#include "app_book.h"
#include "app_windows.h"
#include "gxos/gxcore_os_bt.h"
#include "bus/service/gxusbbtplug.h"
#include "module/app_network_service.h"
#include "bus/module/player/gxaudio_dongle.h"
#include "full_screen.h"
#if NETWORK_SUPPORT && BLUETOOTH_SUPPORT
static GxBTDev bluez_source_device;
static unsigned char s_bluetooth_set_connecting = 0;
static unsigned char s_bluetooth_set_fifo_change = 0;
static unsigned char s_bluetooth_ap_num = 0;
static unsigned char thiz_bt_keep_status = 0;
static int thiz_bt_connect_status = 0;
static volatile int  s_bt_source_play_flag = 0;

char net_if_dev_usb_bt[16] = NET_IF_DEV_USBBT;
extern BtApInfo thiz_bt_ap_list[MAX_AP_NUM];
extern handle_t s_bt_aplist_mutex ;
extern handle_t s_bt_display_mutex ;
extern handle_t s_bt_scan_mutex ;
//#define APP_DEBUG_AUDIO_DONGLE_RECORD
#define BT_ADDRESS_LEN 6
#define BT_APP_COUNTER 30
#define BT_SSID_LEN   (33)
#define GIF_PATH WORK_PATH"theme/image/netapps/loading.gif"
#define BUF_SMALL_LEN		33
#define BT_LINK_KEY_DATA "44:85:00:96:32:09"
#define BT_LINK_MAC_DATA "E7:86:5A:5F:63:AB"

#define LIST_BLUETOOTH						"list_bluetooth"
#define TXT_BLUETOOTH_LINK_STATUS			"text_bluetooth_link_status"
#define TXT_BLUETOOTH_LINK_TITLE			"text_bluetooth_link_title"
#define IMG_BLEUTOOTH_LINK			"img_bluetooth_link"
#define TXT_BLUETOOTH_MOVE				"text_bluetooth_move"
#define IMG_BLUETOOTH_MOVE				"img_bluetooth_move"
#define TXT_BLEUTOOTH_OK					"text_bluetooth_ok"
#define IMG_BLUETOOTH_OK					"img_bluetooth_ok"
#define TXT_BLUETOOTH_EXIT					"text_bluetooth_exit"
#define IMG_BLUETOOTH_EXIT					"img_bluetooth_exit"
#define TXT_BLUETOOTH_REFRESH				"text_bluetooth_refresh"
#define IMG_BLUETOOTH_REFRESH				"img_bluetooth_refresh"

#define APP_BT_DEBUG_PRINTF_SUPPORT
#ifdef APP_BT_DEBUG_PRINTF_SUPPORT
#define app_bt_debug app_log_info
#else
#define app_bt_debug app_log_info
#endif

typedef enum
{
    BT_SCAN_STATUS_OFF = 0,
    BT_SCAN_STATUS_ON,
    BT_SCAN_STATUS_STOP
}BluetoothScanStatue;

typedef struct {
    int transfer_mode; /*0 - close; 1 - sync play; 2 - only bluetooth play*/
    int bluetooth_mute;
    int bluetooth_volume;
}PlayerBluetoothConfig;

typedef struct _bluetoothap_para_t
{
    char essid[ESSID_MAX_SIZE + 1];
    char mac[BUF_SMALL_LEN];
    uint8_t is_link;
    struct _bluetoothap_para_t* next;
}bluetoothApPara;

typedef enum
{
    bluetooth_MENU_SINGLE,
    bluetooth_MENU_ALL,
}bluetoothMenuMode;

typedef enum
{
    BLUETOOTH_LINK_SOURCE,
    BLUETOOTH_LINK_SINK,
}BluetoothLinkMode;

static unsigned char s_scanning = BT_SCAN_STATUS_OFF;
static bluetoothMenuMode s_bluetooth_menu_mode = bluetooth_MENU_SINGLE;
static uint8_t s_dev_sel = 0;
static uint8_t s_bluetooth_dev_total = 0;
static int s_ap_sel= 0;
static int s_ap_total = 0;
static ubyte64 *s_bluetooth_dev = NULL;
static ubyte64 s_cur_dev = {0};
static uint8_t s_bluetooth_connect = 0;
static bluetoothApPara *sp_ap_head = NULL;
static bluetoothApPara *sp_ap_cur = NULL;
static bool s_bluetooth_linking = false;
static IfState s_bluetooth_state = IF_STATE_INVALID;
static uint8_t s_wait_cnt = 0;
static event_list* s_bluetooth_init_timer = NULL;
static event_list* s_bluetooth_gif_timer = NULL;
static event_list* s_bluetooth_scan_timer = NULL;
static event_list* s_bluetooth_reset_scan_timer = NULL;
static GxMsgProperty_ApList s_scan_ap_list;
static bool s_bluetooth_set_show = false;
static bool s_bluetooth_ap_updating = false;
static ubyte64 s_set_bluetooth = {0};
static bluetoothApPara s_set_ap;
static GxMsgProperty_CurAp s_cur_ap = {{0},};

extern unsigned int app_bluetooth_sink_audio_enable(void);
extern unsigned int app_bluetooth_sink_audio_disable(void);
extern int app_pop_list_end_dialog(void);
extern void app_bluetooth_sink_ui_status(GxMessage *msg);
void app_bluetooth_dev_update(char *bluetooth_dev);
void app_bluetooth_ap_list_update(GxMsgProperty_ApList *ap_list);
void app_bluetooth_ap_list_clean(void);
void app_bluetooth_tip_hide(void);
int app_bluez_source_scan_off(void);
static int app_bluez_source_force_scan_off(void);
int app_reset_scan_ap(int mode);
static bool app_check_bluetooth_ap_exist(ubyte64 ap_name, ubyte32 ap_mac);
extern int app_is_sink_menu_mode(void);
extern void app_create_sink_menu(void);

static int app_bt_menu_display_protect(void)
{
    GxCore_MutexLock(s_bt_display_mutex);
    return 0 ;
}

static int app_bt_menu_display_unprotect(void)
{
    GxCore_MutexUnlock(s_bt_display_mutex);
    return 0 ;
}

/*
static void convert_btaddr_to_str(char* data, unsigned char* addr)
{
    sprintf(data, "%02X:%02X:%02X:%02X:%02X:%02X", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
}
*/
static int nibble_for_char(char c)
{
    if ((c >= '0') && (c <= '9')) return c - '0';
    if ((c >= 'a') && (c <= 'f')) return c - 'a' + 10;
    if ((c >= 'A') && (c <= 'F')) return c - 'A' + 10;
    return -1;
}

static int bt_scan_hex_byte(const char * byte_string)
{
    int upper_nibble = nibble_for_char(*byte_string++);
    if (upper_nibble < 0)
        return -1;
    int lower_nibble = nibble_for_char(*byte_string);
    if (lower_nibble < 0)
        return -1;
    return (upper_nibble << 4) | lower_nibble;
}


static int bt_addr_mac_convert(const char * addr_string, unsigned char *addr)
{
	uint8_t buffer[BT_ADDRESS_LEN];
	int result = 0;
	int i;

	for (i = 0; i < BT_ADDRESS_LEN; i++) {
		int single_byte = bt_scan_hex_byte(addr_string);
		if (single_byte < 0)
			break;

		addr_string += 2;
		buffer[i] = (uint8_t)single_byte;
		// don't check seperator after last byte
		if (i == (BT_ADDRESS_LEN - 1)) {
			result = 1;
			break;
		}
		// skip supported separators
		char next_char = *addr_string;
		if ((next_char == ':') || (next_char == '-') || (next_char == ' ')) {
			addr_string++;
		}
	}

	if (result){
		memcpy(addr, buffer, BT_ADDRESS_LEN);
	}

	return result;
}

static void __app_bluetooth_stop_reset_scan_timer(void)
{
    if(s_bluetooth_reset_scan_timer !=NULL)
    {
        APP_TIMER_REMOVE(s_bluetooth_reset_scan_timer);
    }
}

static void app_bluetooth_stop_reset_scan(void)
{
    GxCore_MutexLock(s_bt_scan_mutex);
    __app_bluetooth_stop_reset_scan_timer();
    GxCore_MutexUnlock(s_bt_scan_mutex);
}

static int app_bluetooth_reset_scan_proc(void* usrdata)
{
    app_reset_scan_ap(2);

    app_bluetooth_stop_reset_scan();

    return 0;
}

static void __app_bluetooth_start_reset_scan_timer(void)
{
    if(0 != reset_timer(s_bluetooth_reset_scan_timer))
    {
        s_bluetooth_reset_scan_timer = create_timer(app_bluetooth_reset_scan_proc, 1000, NULL, TIMER_REPEAT);
    }
}

static int app_bluetooth_enable_scan(void* usrdata)
{
    app_bluez_source_scan_off();

    return 0;
}

static void app_bluetooth_start_scan_status(void)
{
    if(0 != reset_timer(s_bluetooth_scan_timer))
    {
        s_bluetooth_scan_timer = create_timer(app_bluetooth_enable_scan, 5000, NULL, TIMER_REPEAT);
    }
}

static void app_bluetooth_remove_scan(void)
{
    if(s_bluetooth_scan_timer !=NULL)
    {
        APP_TIMER_REMOVE(s_bluetooth_scan_timer);
    }
}

int app_check_bluetooth_status(void)
{
    return thiz_bt_connect_status;
}

int app_check_bt_connect_status(void)
{
    return thiz_bt_connect_status;
}

static int app_bluez_set_link_key_data(GxMsgProperty_UsbBtLinkKey *cfg_data)
{
    GxBus_ConfigSetInt(BT_LINK_KEY_TYPE, cfg_data->key.type);
    GxBus_ConfigSet(BT_LINK_KEY_MAC, (const char *)cfg_data->key.mac);
    GxBus_ConfigSet(BT_LINK_KEY, (const char *)cfg_data->key.link_key);

    GxBus_ConfigSetInt(BT_LINK_KEY_MODE, BLUETOOTH_LINK_SOURCE);
    return 0;
}

static int app_bluez_set_link_key_mode(BluetoothLinkMode mode)
{
    GxBus_ConfigSetInt(BT_LINK_KEY_MODE, mode);
    return 0;
}

static int app_bluez_get_link_key_data(GxMsgProperty_UsbBtLinkKey *cfg_data)
{
    int32_t init_value = 0;
    int mode_val = 0;

    GxBus_ConfigGetInt(BT_LINK_KEY_MODE, &mode_val, BLUETOOTH_LINK_SOURCE);

    GxBus_ConfigGetInt(BT_LINK_KEY_TYPE, &init_value, DEV_MODE_DEFALT);
    cfg_data->key.type = init_value;
    GxBus_ConfigGet(BT_LINK_KEY_MAC, (char *)cfg_data->key.mac, sizeof(cfg_data->key.mac), BT_LINK_MAC_DATA);
    GxBus_ConfigGet(BT_LINK_KEY, (char *)cfg_data->key.link_key, sizeof(cfg_data->key.link_key), BT_LINK_KEY_DATA);

    return mode_val;
}

int app_bluez_dev_auto_sink_connect(void)
{
    int ret = 0;
    GxBTDev auto_sink_device;
    GxMsgProperty_UsbBtLinkKey link_data;

    ret = app_bluez_get_link_key_data(&link_data);
    if(link_data.key.mac != NULL)
    {
        if(ret == BLUETOOTH_LINK_SINK)
        {
            memcpy(auto_sink_device.bd_addr,link_data.key.mac, BT_ADDRESS_LEN);
            app_bt_debug("sink mac : %x:%x:%x:%x:%x:%x \r\n",
                                link_data.key.mac[0],link_data.key.mac[1],
                                link_data.key.mac[2],link_data.key.mac[3],
                                link_data.key.mac[4],link_data.key.mac[5]);
            ret = GxCore_BTSinkConnect(&auto_sink_device);
            if (ret == 0)
            {
                thiz_bt_connect_status = BT_A2DP_SINK_CONNETING ;
            }
        }
        else
        {
            return -1;
        }
    }
    else
    {
        ret = -1 ;
    }
    return ret;
}

int app_bluez_add_link_key(void)
{
    int ret = 0;
    GxMsgProperty_UsbBtLinkKey link_data;

    ret = app_bluez_get_link_key_data(&link_data);
    if(link_data.key.mac != NULL)
    {
        if(ret == BLUETOOTH_LINK_SINK)
        {
            app_bt_debug("sink add key \n");
            GxCore_BTAddLinkKey(&link_data.key);
        }
    }
    return ret;
}

int app_bluetooth_remove_link_key(void)
{
    int ret = 0;

    GxMsgProperty_UsbBtLinkKey link_data;

    ret = app_bluez_get_link_key_data(&link_data);
    if(link_data.key.mac != NULL)
    {
        GxCore_BTRemoveLinkKey(&link_data.key);
    }

    return ret;
}

int app_bluez_sink_dev_disconnect(void)
{
    GxMsgProperty_UsbBtLinkKey link_data;
    GxBTDev bluez_sink_device;

    if ((thiz_bt_connect_status == BT_A2DP_SINK_CONNECT) ||
        (thiz_bt_connect_status == BT_A2DP_SINK_PLAY)    ||
        (thiz_bt_connect_status == BT_A2DP_SINK_PAUSE)   ||
        (thiz_bt_connect_status == BT_A2DP_SINK_CONNETING))
    {
        app_bluez_get_link_key_data(&link_data);
        memcpy(bluez_sink_device.bd_addr,link_data.key.mac, BT_ADDRESS_LEN);
        GxCore_BTSinkDisconnect(&bluez_sink_device);
        thiz_bt_connect_status = BT_A2DP_SINK_DISCONNECT_NO_REP ;
    }
    return 0 ;
}

int app_bluez_sink_disconnect(void)
{
   if ( GXCORE_SUCCESS == GUI_CheckDialog(WND_SINK_VIEW))
   {
       app_bluetooth_sink_audio_disable();
   }
   return 0 ;
}

int app_bluez_sink_play(void)
{
    if ( GXCORE_SUCCESS == GUI_CheckDialog(WND_SINK_VIEW))
    {
        app_bluetooth_sink_audio_enable();
    }
    return 0;
}

int app_bluez_sink_pause(void)
{
    if ( GXCORE_SUCCESS == GUI_CheckDialog(WND_SINK_VIEW))
    {
        app_bluetooth_sink_audio_disable();
    }
    return 0;
}

int app_bluez_source_connect(ubyte32 mac)
{
    int ret = 0;

    if( s_bluetooth_set_connecting )
    {
        app_log_info("=======scanning on not allowed connect %d\n",s_bluetooth_set_connecting);
        return -1;
    }
    ret = app_bluez_source_force_scan_off();
    if (ret == 0)
    {
        GxCore_ThreadDelay(50);
    }

    app_bt_debug("mac = %s \n",mac);
    bt_addr_mac_convert(mac,bluez_source_device.bd_addr);
    ret = GxCore_BTSourceConnect(&bluez_source_device);
    if (ret != 0 ){
        app_log_error("source connect ret=%d \n",ret);
    }

    return ret;
}

int app_blue_api_source_disconnect(void)
{
    return GxCore_BTSourceDisconnect(&bluez_source_device);
}

int app_bluez_source_disconnect(void)
{
    s_bluetooth_set_connecting  = 0;
    return app_blue_api_source_disconnect() ;
}

int app_bluez_source_dev_is_connected(void)
{
    if ((thiz_bt_connect_status == BT_A2DP_SOURCE_CONNECT) ||
        (thiz_bt_connect_status == BT_A2DP_SOURCE_PLAY)    ||
        (thiz_bt_connect_status == BT_A2DP_SOURCE_PAUSE))
    {
        return 1 ;
    }
    return 0 ;
}
/*
int app_bluez_source_scan_on(void)
{
    GxCore_MutexLock(s_bt_scan_mutex);

    if(s_scanning != BT_SCAN_STATUS_OFF)
    {
        GxCore_MutexUnlock(s_bt_scan_mutex);
        return 0;
    }

    if(s_bluetooth_set_connecting)
    {
        app_log_error("connecting on not allowed scan\n");
        GxCore_MutexUnlock(s_bt_scan_mutex);
        return -1;
    }

    app_bt_debug("scan on\n");
    GxCore_BTScan(1);
    s_scanning = BT_SCAN_STATUS_ON;

    GxCore_MutexUnlock(s_bt_scan_mutex);

    return 0;
}
*/
static int app_bluez_source_force_scan_off(void)
{
    GxCore_MutexLock(s_bt_scan_mutex);

    __app_bluetooth_stop_reset_scan_timer();

    if(s_scanning != BT_SCAN_STATUS_ON)
    {
        s_scanning = BT_SCAN_STATUS_STOP ;
        GxCore_MutexUnlock(s_bt_scan_mutex);
        return -1;
    }

    app_bt_debug("force scan off \n");
    GxCore_BTScan(0);
    s_scanning = BT_SCAN_STATUS_STOP ;

    GxCore_MutexUnlock(s_bt_scan_mutex);

    return 0;
}

int app_bluez_source_scan_off(void)
{
    GxCore_MutexLock(s_bt_scan_mutex);

    __app_bluetooth_stop_reset_scan_timer();

    if(s_scanning != BT_SCAN_STATUS_ON)
    {
        GxCore_MutexUnlock(s_bt_scan_mutex);
        return -1;
    }

    app_bt_debug("scan off\n");
    GxCore_BTScan(0);
    s_scanning = BT_SCAN_STATUS_OFF;

    GxCore_MutexUnlock(s_bt_scan_mutex);

    return 0;
}

int app_reset_scan_ap(int mode)
{
    GxCore_MutexLock(s_bt_scan_mutex);

    if(s_bluetooth_set_connecting)
    {
        app_log_error("connecting on not allowed scan\n");
        GxCore_MutexUnlock(s_bt_scan_mutex);
        return -1;
    }

    //if( s_scanning == BT_SCAN_STATUS_STOP )
    if( s_scanning != BT_SCAN_STATUS_OFF )
    {
        app_bt_debug("reset scan pass %d \n",mode);
        GxCore_MutexUnlock(s_bt_scan_mutex);
        return -1;
    }
    app_bt_debug("reset scan %d \n",mode);
    if(mode)
    {
        app_bluetooth_start_scan_status();
    }
    GxCore_BTScan(1);
    s_scanning = BT_SCAN_STATUS_ON;

    GxCore_MutexUnlock(s_bt_scan_mutex);

    return 0;
}

int app_force_reset_scan_ap(int mode)
{
    GxCore_MutexLock(s_bt_scan_mutex);

    if(s_bluetooth_set_connecting)
    {
        app_log_error("connecting on not allowed scan\n");
        GxCore_MutexUnlock(s_bt_scan_mutex);
        return -1;
    }

    if( s_scanning == BT_SCAN_STATUS_ON )
    {
        app_bt_debug("force reset scan pass \n");
        GxCore_MutexUnlock(s_bt_scan_mutex);
        return -1;
    }
    app_bt_debug("force reset scan %d \n",mode);
    if(mode)
    {
        app_bluetooth_start_scan_status();
    }
    GxCore_BTScan(1);
    s_scanning = BT_SCAN_STATUS_ON;

    GxCore_MutexUnlock(s_bt_scan_mutex);

    return 0;
}

int app_scan_finish_to_scan_again(void)
{
    GxCore_MutexLock(s_bt_scan_mutex);

    if(s_bluetooth_set_connecting)
    {
        app_log_error("connecting on not allowed scan\n");
        GxCore_MutexUnlock(s_bt_scan_mutex);
        return -1;
    }

    if( s_scanning != BT_SCAN_STATUS_OFF )
    {
        app_bt_debug("scan finish pass, s_scanning = %d \n",s_scanning);
        GxCore_MutexUnlock(s_bt_scan_mutex);
        return -1;
    }

    app_bt_debug("scan finish timer \n");
    s_scanning = BT_SCAN_STATUS_OFF ;

    __app_bluetooth_start_reset_scan_timer();

    GxCore_MutexUnlock(s_bt_scan_mutex);

    return 0;
}

int app_bluez_source_link_stop(void)
{
    s_bluetooth_set_connecting  = 0;

    memset(&bluez_source_device,0,sizeof(bluez_source_device));

    app_log_debug("app_bluez_source_stop - success\n");

    return 0;
}

int app_bluez_source_stop(void)
{
    //app_bluez_source_scan_off();
    app_bluez_source_disconnect();

    memset(&bluez_source_device,0,sizeof(bluez_source_device));

    app_log_debug("app_bluez_source_stop - success\n");

    return 0;
}

int app_bluez_stop(void)
{

    app_bluez_source_force_scan_off();
    app_bluez_source_disconnect();

    GxCore_BTDeinit();

    memset(&bluez_source_device,0,sizeof(bluez_source_device));

    app_log_debug("app_bluez_stop - success\n");

    return 0;
}

static unsigned int app_bluetooth_source_get_cap(GxAudioDongleCap  *cap)
{
    GxBtA2dpCodecInfo info;

    GxCore_BTSourceGetCodecInfo(&info);
    app_log_info("sample_rate=%d,channel_num=%d,bit_width=%d\n",info.sample_rate,info.channel_num,info.bit_width);
    app_log_info("interlace=%d,little_endian=%d\n",info.interlace,info.little_endian);
    cap->format = GXAUDIO_DONGLE_PCM;
    cap->keep_pulse = thiz_bt_keep_status;
    cap->pcm.sample_freq = 44100;
	cap->pcm.channel_num = 2;
	cap->pcm.bits        = 16;
	cap->pcm.interlace   = 1;
	cap->pcm.endian      = 0;
    return 0;
}

static unsigned int app_bluetooth_source_get_bufinfo(GxAudioDongleBufInfo *buf)
{
    GxBtFifoInfo info;
    GxCore_BTSourceGetFifoInfo(&info);
    app_log_info("free_size=%d,total_size=%d\n ",info.free_size,info.total_size);
    return 0;
}

#ifdef APP_DEBUG_AUDIO_DONGLE_RECORD
static unsigned char *pmem = NULL;
static unsigned int   psize = 5 * 1024*1024;
static unsigned int   ppwtr = 0;
static unsigned int pstart = 0;

void app_rec_start(void) {
    pstart = 1;
    app_log_info("--------bluetoooth start--------------\n");
}

void app_rec_stop(void) {
    pstart = 0;
    ppwtr  = 0;
    app_log_info("--------bluetoooth start--------------\n");
}
#endif
static unsigned int app_bluetooth_source_push_data(GxAudioDongleData *data)
{
    GxBtFifoInfo info;
    GxCore_BTSourceGetFifoInfo(&info);
//printf("play status=[%d]\n",s_bluetooth_set_fifo_change);
    if(s_bt_source_play_flag == 1)
    {
#ifdef APP_DEBUG_AUDIO_DONGLE_RECORD
        if (pstart) {
            if (pmem == NULL) {
                pmem  = malloc(psize);
                ppwtr = 0;
            }
            if ((ppwtr + data->buf_size) <= psize) {
                memcpy(pmem + ppwtr, data->buf_ptr, data->buf_size);
                ppwtr += data->buf_size;
            }
            app_log_info("===> %p, %p\n", pmem, pmem + ppwtr);
        }
        //printf("start**free/total=[%d/%d]---%d\n ",info.free_size,info.total_size,s_bluetooth_set_connecting);
#endif
        GxCore_BTSourceSendData(data->buf_ptr,data->buf_size);
        //printf("later---------*free_size=%d,total_size=%d\n ",info.free_size,info.total_size);
    }
    return 0;
}

static void initialize_net_if_dev_usb_bt(void)
{
    int chipsn_len = 0,i=0;
    char chipsn[8] = {0};
    char *ptr = net_if_dev_usb_bt;

    app_chip_sn_get(chipsn, sizeof(chipsn));
    for (chipsn_len = 0; chipsn_len < sizeof(chipsn) && chipsn[chipsn_len] != '\0'; chipsn_len++);

    if (chipsn_len >= 4) {
        snprintf(net_if_dev_usb_bt, sizeof(net_if_dev_usb_bt), "USBBT%02X%02X%02X%02X",
                 (unsigned char)chipsn[chipsn_len - 4], (unsigned char)chipsn[chipsn_len - 3],
                 (unsigned char)chipsn[chipsn_len - 2], (unsigned char)chipsn[chipsn_len - 1]);
    } else {
        ptr += snprintf(ptr, sizeof(net_if_dev_usb_bt), "USBBT");
        for (i = 0; i < chipsn_len; i++) {
            ptr += snprintf(ptr, sizeof(net_if_dev_usb_bt) - (ptr - net_if_dev_usb_bt), "%02X", (unsigned char)chipsn[i]);
        }
    }
}

static GxAudioDongleOps thiz_source_ops = {
    .name = net_if_dev_usb_bt,
    .author = "NC",
    .get_cap = app_bluetooth_source_get_cap,
    .get_bufinfo = app_bluetooth_source_get_bufinfo,
    .push = app_bluetooth_source_push_data,
    .pull = NULL,
};

status_t app_bluetooth_register_dogle(void)
{

    GxAudioRegisterDongle(&thiz_source_ops);
    return GXCORE_SUCCESS;
}

status_t app_bluetooth_unregister_dogle(void)
{
    GxAudioUnRegisterDongle(&thiz_source_ops);
    return GXCORE_SUCCESS;
}
extern int app_get_mute_state(void);
extern uint32_t app_system_vol_get(void);
unsigned int app_bluetooth_source_audio_enable(void)
{
    int ret = 0;
    GxAudioDongleParam param;
    GxMsgProperty_PlayerAudioMute player_mute;

    if((thiz_bt_connect_status == BT_A2DP_SOURCE_CONNECT)
        || (thiz_bt_connect_status == BT_A2DP_SOURCE_PLAY))
    {
        player_mute = 1;
        app_send_msg_exec(GXMSG_PLAYER_AUDIO_MUTE, (void *)(&player_mute));

        param.mute = app_get_mute_state();
        param.volume = app_system_vol_get();
        param.channel = GXAUDIO_DONGLE_STEREO_CHANNEL;

        ret = GxAudioDongleEnable(thiz_source_ops.name,&param);
    }
    return ret;
}

unsigned int app_bluetooth_source_audio_disable(void)
{
    int ret = 0;

    ret = GxAudioDongleDisable(thiz_source_ops.name);
    return ret;
}

int app_bluetooth_source_set_volume(int volume)
{
    int ret = 0;
    if((thiz_bt_connect_status == BT_A2DP_SOURCE_CONNECT)
       || ( thiz_bt_connect_status == BT_A2DP_SOURCE_PLAY)
       || ( thiz_bt_connect_status == BT_A2DP_SOURCE_PAUSE))
    {
        ret = GxAudioDongleSetVolume(thiz_source_ops.name,volume);
    }
    return ret;
}

int app_bluetooth_source_set_mute(int mute)
{
    int ret = 1;
    if((thiz_bt_connect_status == BT_A2DP_SOURCE_CONNECT)
        || ( thiz_bt_connect_status == BT_A2DP_SINK_CONNECT)
        || ( thiz_bt_connect_status == BT_A2DP_SOURCE_PLAY)
        || ( thiz_bt_connect_status == BT_A2DP_SOURCE_PAUSE))
    {
        ret = GxAudioDongleSetMute(thiz_source_ops.name,mute);
    }
    return ret;
}

int app_bluetooth_source_exec_mute(void)
{
    int ret = 0;
    GxMsgProperty_PlayerAudioMute player_mute;

    if(g_AppFullArb.state.mute == STATE_OFF)
    {
        player_mute = 0;
        g_AppFullArb.state.mute = STATE_OFF;
    }
    else
    {
        player_mute = 1;
        g_AppFullArb.state.mute = STATE_ON;
    }
    app_send_msg_exec(GXMSG_PLAYER_AUDIO_MUTE, (void *)(&player_mute));

    return ret;
}

int app_bluetooth_source_set_channel(int channel)
{
    int ret = 0;

    if((thiz_bt_connect_status == BT_A2DP_SOURCE_CONNECT)
        || ( thiz_bt_connect_status == BT_A2DP_SOURCE_PLAY)
        || ( thiz_bt_connect_status == BT_A2DP_SOURCE_PAUSE))
    {
        ret = GxAudioDongleSetChannel(thiz_source_ops.name,channel);
    }
    return ret;
}
int app_bluez_source_play(void)
{
    int ret = 0;
    thiz_bt_keep_status = 0;
    app_bluetooth_source_audio_enable();
    s_bt_source_play_flag = 1;
    s_bluetooth_set_fifo_change = thiz_bt_connect_status;
    GxCore_BTSourcePlay();
    //printf("[%d]***%d\n",__LINE__,s_bluetooth_set_fifo_change);
    return ret;
}

int app_bluez_source_pause(void)
{
    int ret =0;
    thiz_bt_keep_status = 1;
    app_bluetooth_source_audio_disable();
    s_bt_source_play_flag = 0;
    GxCore_BTSourceClearFifo();
    GxCore_BTSourcePause();
    app_log_info("clear src fifo\n");
    return ret;
}

int app_bluez_get_source_connect_statue(void)
{
    if((thiz_bt_connect_status == BT_A2DP_SOURCE_CONNECT)
        || ( thiz_bt_connect_status == BT_A2DP_SOURCE_PLAY))
    {
        return 1 ;
    }
    return 0 ;
}

static int app_bluetooth_draw_gif(void* usrdata)
{
	int alu = GX_ALU_ROP_COPY_INVERT;
	GUI_SetProperty("img_bluetooth_gif", "draw_gif", &alu);
    return 0;
}

static void app_bluetooth_load_gif(void)
{
	int alu = GX_ALU_ROP_COPY_INVERT;
	GUI_SetProperty("img_bluetooth_gif", "load_img", GIF_PATH);
	GUI_SetProperty("img_bluetooth_gif", "init_gif_alu_mode", &alu);
}

static void app_bluetooth_free_gif(void)
{
	GUI_SetProperty("img_bluetooth_gif", "load_img", NULL);
}

static void app_bluetooth_show_gif(void)
{
    GUI_SetProperty("img_bluetooth_gif", "state", "show");
    if(0 != reset_timer(s_bluetooth_gif_timer))
	{
		s_bluetooth_gif_timer = create_timer(app_bluetooth_draw_gif, 100, NULL, TIMER_REPEAT);
	}
}

static void app_bluetooth_hide_gif(void)
{
    APP_TIMER_REMOVE(s_bluetooth_gif_timer);
	GUI_SetProperty("img_bluetooth_gif", "state", "hide");
}

static void ap_list_add_bluetooth(bluetoothApPara *p)
{
	if(sp_ap_head == NULL)
	{
		sp_ap_head = sp_ap_cur = p;
	}
	else
	{
		sp_ap_cur->next = p;
		sp_ap_cur = p;
	}
}

static bluetoothApPara *ap_list_get_bluetooth(uint8_t index)
{
	bluetoothApPara *p = sp_ap_head;
	uint8_t i = 0;

	if(p == NULL)
		return p;
	for(i = 0; i < index; i++)
    {
		p = p->next;
        if(NULL == p)
        {
            if(i < index - 1)
                app_log_debug("index exceeds range\n");
            break;
        }
    }
	return p;
}

static int8_t ap_list_get_sel_bluetooth(bluetoothApPara *s_ap)
{
	bluetoothApPara *p = sp_ap_head;
	int8_t i = 0;

	if(p == NULL)
		return -1;

    while(p != NULL)
    {
        if((strcmp(s_ap->essid, p->essid) == 0) && (strcmp(s_ap->mac, p->mac) == 0))
        {
            break;
        }
        i++;
		p = p->next;
    }

    if(p == NULL)
        i = -1;

	return i;
}

static void ap_list_changeToHead_bluetooth(bluetoothApPara *p_ap)
{
	if(sp_ap_head == p_ap)
	{
		return;
	}
	else
	{
		bluetoothApPara *p_fap = NULL;
		bluetoothApPara *p_tmp = NULL;
		p_tmp = sp_ap_head;
		while(p_tmp)
		{
			if(p_tmp->next&&(p_tmp->next == p_ap))
			{
				p_fap = p_tmp;
				break;
			}
			p_tmp = p_tmp->next;
		}
		if(p_fap)
		{
			p_fap->next = p_ap->next;
			p_ap->next =  sp_ap_head;
			sp_ap_head = p_ap;
		}
	}
}

static void ap_list_destroy_bluetooth(void)
{
	bluetoothApPara *p = sp_ap_head;

    if(sp_ap_head)
	{
		while(p)
		{
			sp_ap_head = p->next;
			GxCore_Free(p);
			p = sp_ap_head;
		}
	}
	sp_ap_head = sp_ap_cur = NULL;
}

static void ap_list_create_bluetooth(GxMsgProperty_ApList *ap_list)
{
	uint8_t i =0;
	bluetoothApPara *p_ap = NULL;
	bluetoothApPara *p_cur = NULL;

    GxMsgProperty_CurAp cur_ap;
    GxMsgProperty_NetDevState state;
    if(ap_list->ap_num == 0)
		return;


    memset(&state, 0, sizeof(GxMsgProperty_NetDevState));
    strcpy(state.dev_name, ap_list->bluetooth_dev);
    app_send_msg_exec(GXMSG_NETWORK_GET_STATE, &state);
    memset(&cur_ap, 0, sizeof(GxMsgProperty_CurAp));
    strcpy(cur_ap.bluetooth_dev, ap_list->bluetooth_dev);
    app_send_msg_exec(GXMSG_BLUETOOTH_GET_CUR_AP, &cur_ap);
    for(i = 0; i < ap_list->ap_num; i++)
	{
		p_ap = (bluetoothApPara *)GxCore_Calloc(sizeof(bluetoothApPara), 1);
        ap_list_add_bluetooth(p_ap);
        if((p_ap == NULL)||(ap_list->bt_ap_info == NULL))
        {
            app_log_error("*error*[name=%s]***\n",ap_list->bt_ap_info[i].bt_name);
            return;
        }
        strcpy(sp_ap_cur->mac, ap_list->bt_ap_info[i].ap_mac);
        strcpy(sp_ap_cur->essid, ap_list->bt_ap_info[i].bt_name);

        if(strcmp(sp_ap_cur->mac, cur_ap.ap_mac) == 0)
        {
            p_cur = sp_ap_cur;

            if(state.dev_state == IF_STATE_CONNECTED)
            {
                GxMsgProperty_NetDevIpcfg ip_cfg;
                memset(&ip_cfg, 0, sizeof(GxMsgProperty_NetDevIpcfg));
                strcpy(ip_cfg.dev_name, ap_list->bluetooth_dev);
                app_send_msg_exec(GXMSG_NETWORK_GET_IPCFG, &ip_cfg);

                sp_ap_cur->is_link = 1;
            }
        }
#if 0
        app_log_debug("---dev:%s, ap%d: %s, %s, %d, %d, link: %d---\n", ap_list->bluetooth_dev, i, ap_list->ap_info[i].ap_essid, ap_list->ap_info[i].ap_mac,
                ap_list->ap_info[i].ap_quality, ap_list->ap_info[i].encryption, sp_ap_cur->is_link);
#endif
	}

	if(p_cur != NULL)
	{
		ap_list_changeToHead_bluetooth(p_cur);
	}
}

static bool s_bluetooth_tip_flag =false;
static void ap_list_update_bluetooth_connect(BtApInfo *ap_list,unsigned int ap_num)
{
    //uint8_t i =0;
	bluetoothApPara *p_ap = NULL;

    if((ap_num <= 0)||(s_bluetooth_dev[s_dev_sel]== NULL))
		return;

    if(GUI_CheckDialog(WND_BLUETOOTH) != GXCORE_SUCCESS)
    {
        return;
    }
    if ( s_bluetooth_set_show )
    {
		return ;
    }

	p_ap = (bluetoothApPara *)GxCore_Calloc(sizeof(bluetoothApPara), 1);
    if (p_ap == NULL )
        return ;

    app_bt_menu_display_protect();

    ap_list_add_bluetooth(p_ap);
    strcpy(sp_ap_cur->mac, ap_list[ap_num-1].ap_mac);
    strcpy(sp_ap_cur->essid, ap_list[ap_num-1].bt_name);

    s_ap_total = s_ap_total+1;

    GUI_SetProperty(LIST_BLUETOOTH, "update_all", NULL);
    if ( (s_ap_sel < 0) || (s_ap_sel >= ap_num))
    {
         s_ap_sel = 0 ;
         GUI_SetProperty(LIST_BLUETOOTH, "select", &s_ap_sel);
    }

    app_bt_menu_display_unprotect();

    app_bluetooth_hide_gif();
    app_bluetooth_tip_hide();

    app_update_pop_dlg(WND_POP_TIP);
    app_update_pop_dlg(WND_POP_BOOK);
}


void app_bluetooth_tip_show(char *info_str)
{
    if(true == s_bluetooth_tip_flag)
        return;

	s_bluetooth_tip_flag = true;
    GUI_SetInterface("flush",NULL);
    GUI_SetProperty(IMG_BLEUTOOTH_LINK, "state", "show");
	GUI_SetProperty(TXT_BLUETOOTH_LINK_TITLE, "state", "show");
	GUI_SetProperty(TXT_BLUETOOTH_LINK_STATUS, "state", "show");
	GUI_SetProperty(TXT_BLUETOOTH_LINK_STATUS, "string", info_str);
}

void app_bluetooth_tip_hide(void)
{
    if(false == s_bluetooth_tip_flag)
        return;

	s_bluetooth_tip_flag = false;
	GUI_SetProperty(IMG_BLEUTOOTH_LINK, "state", "hide");
	GUI_SetProperty(TXT_BLUETOOTH_LINK_STATUS, "state", "hide");
	GUI_SetProperty(TXT_BLUETOOTH_LINK_TITLE, "state", "hide");
}

void app_bluetooth_tip_lost_focus_hide(void)
{
	GUI_SetProperty(IMG_BLEUTOOTH_LINK, "state", "hide");
	GUI_SetProperty(TXT_BLUETOOTH_LINK_STATUS, "state", "hide");
	GUI_SetProperty(TXT_BLUETOOTH_LINK_TITLE, "state", "hide");
}

void app_bluetooth_tip_flush(void)
{
	if(s_bluetooth_tip_flag == true)
	{
		app_bluetooth_tip_hide();
		GUI_SetInterface("flush",NULL);
		GUI_SetProperty(IMG_BLEUTOOTH_LINK, "state", "show");
		GUI_SetProperty(TXT_BLUETOOTH_LINK_TITLE, "state", "show");
		GUI_SetProperty(TXT_BLUETOOTH_LINK_STATUS, "state", "show");
	}
}

void app_blutetooth_update_status(IfState mode)
{
    if(mode == IF_STATE_CONNECTED)
    {
        GUI_SetProperty(IMG_BLUETOOTH_REFRESH,"state","hide");
        GUI_SetProperty(TXT_BLUETOOTH_REFRESH,"state","hide");
    }
    else
    {
        GUI_SetProperty(IMG_BLUETOOTH_REFRESH,"state","show");
        GUI_SetProperty(TXT_BLUETOOTH_REFRESH,"state","show");
    }
}

void app_bluetooth_hint_show(void)
{
	GUI_SetProperty(IMG_BLUETOOTH_OK, "state", "show");
	GUI_SetProperty(TXT_BLEUTOOTH_OK, "state", "show");
	GUI_SetProperty(IMG_BLUETOOTH_MOVE, "state", "show");
	GUI_SetProperty(TXT_BLUETOOTH_MOVE, "state", "show");
	GUI_SetProperty(IMG_BLUETOOTH_EXIT, "state", "show");
	GUI_SetProperty(TXT_BLUETOOTH_EXIT, "state", "show");
	GUI_SetProperty(IMG_BLUETOOTH_REFRESH, "state", "show");
	GUI_SetProperty(TXT_BLUETOOTH_REFRESH, "state", "show");
}

void app_bluetooth_hint_hide(void)
{
	GUI_SetProperty(IMG_BLUETOOTH_OK, "state", "hide");
	GUI_SetProperty(TXT_BLEUTOOTH_OK, "state", "hide");
	GUI_SetProperty(IMG_BLUETOOTH_MOVE, "state", "hide");
	GUI_SetProperty(TXT_BLUETOOTH_MOVE, "state", "hide");
	GUI_SetProperty(IMG_BLUETOOTH_EXIT, "state", "hide");
	GUI_SetProperty(TXT_BLUETOOTH_EXIT, "state", "hide");
	GUI_SetProperty(IMG_BLUETOOTH_REFRESH, "state", "hide");
	GUI_SetProperty(TXT_BLUETOOTH_REFRESH, "state", "hide");
}

void app_bluetooth_set_show(void)
{
    bluetoothApPara *p_ap = NULL ;

    app_bt_menu_display_protect();

    p_ap = ap_list_get_bluetooth(s_ap_sel);
    memset(&s_set_ap, 0, sizeof(bluetoothApPara));
    if(p_ap == NULL)
    {
        app_bt_menu_display_unprotect();
		return;
    }
    s_bluetooth_set_show = true;

    memcpy(&s_set_ap, p_ap, sizeof(bluetoothApPara));

    app_bt_menu_display_unprotect();

    strcpy(s_set_bluetooth, s_bluetooth_dev[s_dev_sel]);

    app_bluetooth_hide_gif();

    GUI_SetProperty("img_bluetooth_set_bk", "state", "show");

	GUI_SetProperty("text_bluetooth_set0", "state", "show");
	GUI_SetProperty("text_bluetooth_set1", "state", "show");

	GUI_SetProperty("text_bluetooth_essid", "string", s_set_ap.essid);
	GUI_SetProperty("text_bluetooth_essid", "state", "show");
	GUI_SetProperty("text_bluetooth_mac", "string", s_set_ap.mac);
	GUI_SetProperty("text_bluetooth_mac", "state", "show");

	if(s_set_ap.is_link == 0)
	{
		s_bluetooth_connect = 0;
		GUI_SetProperty("button_bluetooth_connect", "string", STR_ID_SAVE);
	}
	else
	{
		s_bluetooth_connect = 1;
		GUI_SetProperty("button_bluetooth_connect", "string", "Disconnect");
	}

    GUI_SetProperty("img_bluetooth_tip_opt","img","s_bar_choice_blue4_l");
	GUI_SetProperty("img_bluetooth_tip_opt", "state", "show");
	GUI_SetProperty("button_bluetooth_connect", "state", "show");
	GUI_SetProperty("button_bluetooth_cancel", "state", "show");

	GUI_SetFocusWidget("button_bluetooth_connect");
}

void app_bluetooth_set_hide(void)
{
    const char *wnd = NULL;

    wnd = GUI_GetFocusWindow();
	GUI_SetProperty("img_bluetooth_set_bk", "state", "hide");
	GUI_SetProperty("text_bluetooth_set0", "state", "hide");
	GUI_SetProperty("text_bluetooth_set1", "state", "hide");

	GUI_SetProperty("text_bluetooth_essid", "state", "hide");
	GUI_SetProperty("text_bluetooth_mac", "state", "hide");
	GUI_SetProperty("img_bluetooth_tip_opt", "state", "hide");
	GUI_SetProperty("button_bluetooth_connect", "state", "hide");
	GUI_SetProperty("button_bluetooth_cancel", "state", "hide");
    if(wnd != NULL && 0 == strcmp(wnd, WND_BLUETOOTH))
    {
	    GUI_SetFocusWidget(LIST_BLUETOOTH);
    }

    s_bluetooth_set_show = false;
}

void app_bluetooth_save_ap(void)
{
    if(s_bluetooth_dev_total ==  0)
        return;

    memset(&s_cur_ap, 0, sizeof(GxMsgProperty_CurAp));
    strcpy(s_cur_ap.bluetooth_dev, s_bluetooth_dev[s_dev_sel]);
    strcpy(s_cur_ap.ap_essid, s_set_ap.essid);
    strcpy(s_cur_ap.ap_mac, s_set_ap.mac);
    app_send_msg_exec(GXMSG_BLUETOOTH_SET_CUR_AP, &s_cur_ap);

}

void app_bluetooth_disconnect(void)
{
    ubyte64 bluetooth_name;
    if(s_bluetooth_dev_total > 0)
    {
        memset(bluetooth_name, 0, sizeof(ubyte64));
        strcpy(bluetooth_name, s_bluetooth_dev[s_dev_sel]);
        app_log_debug("[%s] del bluetooth %s\n", __func__, bluetooth_name);
        app_send_msg_exec(GXMSG_BLUETOOTH_DEL_CUR_AP, &bluetooth_name);
        memset(&s_cur_ap, 0, sizeof(GxMsgProperty_CurAp));
    }
}

void app_bluetooth_ap_list_update(GxMsgProperty_ApList *ap_list)
{
    s_ap_total = 0;
    ap_list_destroy_bluetooth();
    if ( ap_list->bt_ap_info != NULL )
    {
        ap_list->ap_num = s_bluetooth_ap_num ;
    }
    else
    {
        s_ap_total = 0 ;
        return ;
	}
    ap_list_create_bluetooth(ap_list);
    s_ap_total = ap_list->ap_num;
}

void app_bluetooth_ap_list_clean(void)
{
    s_ap_total = 0;
    ap_list_destroy_bluetooth();
}

void app_bluetooth_dev_update(char *bluetooth_dev)
{
    ubyte64 display_name = {0};
    ubyte64 bluetooth_name = {0};
    GxMsgProperty_NetDevState state;

    if((bluetooth_dev == NULL) || (strlen(bluetooth_dev) == 0))
        return;

    app_if_name_convert(bluetooth_dev, display_name);
    GUI_SetProperty("text_bluetooth_dev", "string", display_name);
	memset(&state, 0, sizeof(GxMsgProperty_NetDevState));
	strcpy(state.dev_name, bluetooth_dev);
	app_send_msg_exec(GXMSG_NETWORK_GET_STATE, &state);
	s_bluetooth_state = state.dev_state;
	if(s_bluetooth_ap_updating)
	{
		app_bluetooth_tip_show(STR_ID_PLZ_WAIT);
		if(s_bluetooth_state == IF_STATE_CONNECTING)
        {
            app_bluetooth_show_gif();
        }
		return;
	}
	else
	{
		app_bluetooth_hide_gif();
		app_bluetooth_tip_hide();

        app_bt_menu_display_protect();

		app_bluetooth_ap_list_clean();
		GUI_SetProperty(LIST_BLUETOOTH, "update_all", NULL);

		//memset(&s_scan_ap_list, 0, sizeof(GxMsgProperty_ApList));
        s_scan_ap_list.ap_num = 0 ;

        app_bt_menu_display_unprotect();

        if(s_bluetooth_state == IF_STATE_INVALID)
		{
			app_bluetooth_tip_show(STR_ID_NO_BLUETOOTH);
		}
		else
		{
			app_bluetooth_tip_show(STR_ID_PLZ_WAIT);
			memset(bluetooth_name, 0, sizeof(ubyte64));
			strcpy(bluetooth_name, bluetooth_dev);
			if((s_bluetooth_state == IF_STATE_CONNECTING) || (s_bluetooth_state == IF_STATE_START))
			{
				app_bluetooth_show_gif();
			}
			app_send_msg_exec(GXMSG_BLUETOOTH_SCAN_AP, &bluetooth_name);
		}
		s_bluetooth_ap_updating = true;
	}
}

static void app_bluetooth_set_exit(void)
{
    if(s_bluetooth_dev_total > 0)
    {
        if(s_bluetooth_dev_total > 1)
        {
            GUI_SetProperty("img_bluetooth_l","state","show");
            GUI_SetProperty("img_bluetooth_r","state","show");
        }
        else
        {
            GUI_SetProperty("img_bluetooth_l","state","hide");
            GUI_SetProperty("img_bluetooth_r","state","hide");
        }

        if(strcmp(s_set_bluetooth, s_bluetooth_dev[s_dev_sel]) == 0) //same bluetooth dev
        {
            if((s_bluetooth_state == IF_STATE_CONNECTING) || (s_bluetooth_state == IF_STATE_START))
            {
                app_bluetooth_show_gif();
            }
            else
            {
                app_bluetooth_hide_gif();
            }

            app_bt_menu_display_protect();

            app_bluetooth_ap_list_update(&s_scan_ap_list);
            s_ap_sel = ap_list_get_sel_bluetooth(&s_set_ap);

            if(s_ap_sel == -1) //ap lost
            {
                s_ap_sel = 0;
            }

            GUI_SetProperty(LIST_BLUETOOTH, "update_all", NULL);
            GUI_SetProperty(LIST_BLUETOOTH, "select", &s_ap_sel);

            app_bt_menu_display_unprotect();
        }
        else
        {
            app_bluetooth_dev_update(s_bluetooth_dev[s_dev_sel]);
        }

		if(s_bluetooth_set_show)
	        app_bluetooth_set_hide();
    }
    else
    {
        if(s_bluetooth_set_show)
	        app_bluetooth_set_hide();
        app_bluetooth_ap_list_clean();
        s_ap_sel = -1;
        app_bluetooth_hide_gif();
        app_bluetooth_tip_hide();
        GUI_SetProperty(LIST_BLUETOOTH, "update_all", NULL);
        GUI_SetProperty(LIST_BLUETOOTH, "select", &s_ap_sel);
        GUI_SetProperty("text_bluetooth_dev", "string", NULL);
        app_bluetooth_tip_show(STR_ID_NO_BLUETOOTH);
    }
}

static int app_bluetooth_menu_button_disconnect(void)
{
    app_bluetooth_disconnect();
    app_blue_api_source_disconnect();
    app_bluetooth_remove_link_key();

    return 0 ;
}

static int app_bluetooth_clean_scan_aplist_num(void)
{
    GxCore_MutexLock(s_bt_aplist_mutex);
    s_bluetooth_ap_num = 0;
    memset(thiz_bt_ap_list, 0, sizeof(BtApInfo) * MAX_AP_NUM);
    GxCore_MutexUnlock(s_bt_aplist_mutex);

    return 0;
}

static int app_bluetooth_is_connect_state(void)
{
    GxMsgProperty_NetDevState state;
    if ( s_bluetooth_dev == NULL )
        return 0 ;
    if ( s_bluetooth_dev[s_dev_sel] == NULL )
        return 0 ;

    memset(&state, 0, sizeof(GxMsgProperty_NetDevState));
    strcpy(state.dev_name, s_bluetooth_dev[s_dev_sel]);
    app_send_msg_exec(GXMSG_NETWORK_GET_STATE, &state);

    if( (state.dev_state == IF_STATE_CONNECTED)  ||
        (state.dev_state == IF_STATE_CONNECTING) ||
        (state.dev_state == IF_STATE_START))
    {
        return 1 ;
    }

    return 0 ;
}

SIGNAL_HANDLER int app_bluetooth_button_connect_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
    GxMsgProperty_NetDevMode dev_mode;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case STBK_EXIT:
			case STBK_MENU:
                app_bluetooth_set_exit();
				break;
			case STBK_UP:
			case STBK_DOWN:
				break;
			case STBK_LEFT:
			case STBK_RIGHT:
				GUI_SetProperty("img_bluetooth_tip_opt","img","s_bar_choice_blue4");
				GUI_SetFocusWidget("button_bluetooth_cancel");
				break;
			case STBK_OK:
				if(s_bluetooth_connect == 1)
				{
                    app_bluetooth_menu_button_disconnect();
				}
				else
				{
                    if ( app_bluez_source_dev_is_connected() )
                    {
                        app_bluetooth_menu_button_disconnect();
                    }
                    memset(&dev_mode, 0, sizeof(GxMsgProperty_NetDevMode));
                    strcpy(dev_mode.dev_name, s_bluetooth_dev[s_dev_sel]);
                    app_send_msg_exec(GXMSG_NETWORK_GET_MODE, &dev_mode);
                    PopDlg  pop;
                    if(dev_mode.mode == 0)  //bluetooth disable
                    {
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_OK;
                        pop.str = STR_ID_OPEN_BLUETOOTH;
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.format = POP_FORMAT_DLG;
                        popdlg_create(&pop);
                        break;
                    }
                    if ( thiz_bt_connect_status == BT_A2DP_SOURCE_CONNETING )
                    {
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_OK;
                        pop.str = STR_ID_BLUETOOTH_SOURCE_CONNECT_ING;
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.format = POP_FORMAT_DLG;
                        popdlg_create(&pop);
                        break;
                    }
                    else if ( thiz_bt_connect_status == BT_A2DP_SINK_CONNECT )
                    {
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_OK;
                        pop.str = STR_ID_BLUETOOTH_CONNECT_ING;
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.format = POP_FORMAT_DLG;
                        popdlg_create(&pop);
                        break;
                    }
                    s_bluetooth_set_connecting = 0;
					app_bluetooth_save_ap();
                }
                app_bluetooth_set_exit();
				break;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_bluetooth_button_cancel_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case STBK_OK:
			case STBK_EXIT:
			case STBK_MENU:
				app_bluetooth_set_exit();
				break;
			case STBK_UP:
			case STBK_DOWN:
				if(s_bluetooth_connect == 0)
				{
					GUI_SetProperty("img_bluetooth_tip_opt","state","hide");
				}
				break;
			case STBK_LEFT:
			case STBK_RIGHT:
				GUI_SetProperty("img_bluetooth_tip_opt","img","s_bar_choice_blue4_l");
				GUI_SetFocusWidget("button_bluetooth_connect");
				break;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_bluetooth_list_create(const char* widgetname, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_bluetooth_list_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_KEEPON;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
			case STBK_MENU:
			case STBK_EXIT:
                if(s_bluetooth_tip_flag)
                    app_bluetooth_tip_hide();
                s_bluetooth_ap_updating = false;
				app_bluetooth_ap_list_clean();
				GUI_EndDialog(WND_BLUETOOTH);
				ret = EVENT_TRANSFER_STOP;
				break;

			case STBK_DOWN:
				if(s_ap_sel == s_ap_total -1)
					s_ap_sel = 0;
				else
					s_ap_sel++;
				break;
			case STBK_UP:
				if(s_ap_sel == 0)
					s_ap_sel = (s_ap_total-1);
				else
					s_ap_sel--;
				break;

			case STBK_PAGE_DOWN:
				if((s_bluetooth_dev_total>0)&&(s_ap_total>0))
				{
					if(s_ap_sel == s_ap_total-1)
						s_ap_sel = 0;
					else
					{
						s_ap_sel += 7;
						if(s_ap_sel > s_ap_total -1)
							s_ap_sel = s_ap_total-1;
					}
					GUI_SetProperty(LIST_BLUETOOTH, "select", &s_ap_sel);
					GUI_SetProperty(LIST_BLUETOOTH, "update_all", NULL);
				}
				ret = EVENT_TRANSFER_STOP;
				break;

			case STBK_PAGE_UP:
				if((s_bluetooth_dev_total>0)&&(s_ap_total>0))
				{
					if(s_ap_sel == 0)
						s_ap_sel = s_ap_total-1;
					else
					{
						s_ap_sel -= 7;
						if(s_ap_sel < 0)
							s_ap_sel = 0;
					}
	                		GUI_SetProperty(LIST_BLUETOOTH, "select", &s_ap_sel);
	                		GUI_SetProperty(LIST_BLUETOOTH, "update_all", NULL);
				}
				ret = EVENT_TRANSFER_STOP;
				break;

			case STBK_GREEN:
                {
                    if(s_bluetooth_dev_total == 0)
                        break;

                    if( 0 == app_bluetooth_is_connect_state() )
                    {
                        app_bluez_source_force_scan_off();

                        app_bt_menu_display_protect();

                        s_ap_sel = 0 ;
                        if ( s_ap_total > 0 )
                            GUI_SetProperty(LIST_BLUETOOTH, "select", &s_ap_sel);
                        app_bluetooth_ap_list_clean();

                        app_bt_menu_display_unprotect();

                        s_bluetooth_set_connecting = 0;
                        app_bluetooth_clean_scan_aplist_num();
                        //s_scanning = BT_SCAN_STATUS_OFF ;
                        //app_bluez_source_scan_on();
                        app_force_reset_scan_ap(3);
                        app_bluetooth_dev_update(s_bluetooth_dev[s_dev_sel]);
                    }
                }
				break;
            case STBK_BLUE:
                break;
            case STBK_RED:
                break;
			case STBK_OK:
                if(s_bluetooth_dev_total > 0)
                    app_bluetooth_set_show();
				ret = EVENT_TRANSFER_STOP;
				break;
			default:
				break;
		}
	}

	return ret;
}

SIGNAL_HANDLER int app_bluetooth_list_get_total(const char* widgetname, void *usrdata)
{
	return s_ap_total;
}

SIGNAL_HANDLER int app_bluetooth_list_get_data(const char* widgetname, void *usrdata)
{
	ListItemPara* item = NULL;
	static char buf[8];
	bluetoothApPara *p_ap = NULL;

	item = (ListItemPara*)usrdata;
	if(NULL == item)
		return GXCORE_ERROR;
	if(0 > item->sel)
		return GXCORE_ERROR;

	p_ap = ap_list_get_bluetooth(item->sel);
	if(p_ap == NULL)
		return GXCORE_ERROR;

	//col-0
	item->x_offset = 5;
	item->image = NULL;
	memset(buf,  0, 8);
	sprintf(buf, "%d. ", item->sel+1);
	item->string = buf;

	//col-1
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
	item->string = p_ap->essid;

	//col-2
	item = item->next;
	item->x_offset = 0;
	item->string = NULL;
	if(p_ap->is_link == 1)
        item->image = "s_choice";
    else
        item->image = NULL;

    //col-3
	item = item->next;
	item->x_offset = 0;
    if(p_ap->is_link == 1)
        item->string = (char*)GXGDI_I18N_String((const char*)STR_ID_CONNECTED);
    else
        item->string = NULL;//STR_ID_DISCONNECT;
	item->image = NULL;

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_bluetooth_create(const char* widgetname, void *usrdata)
{
    if ( s_scanning == BT_SCAN_STATUS_STOP )
    {
       s_scanning = BT_SCAN_STATUS_OFF;
    }
    app_bluetooth_load_gif();

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_bluetooth_destroy(const char* widgetname, void *usrdata)
{
	s_bluetooth_linking= FALSE;
	s_ap_total = 0;
	s_ap_sel = 0;

    app_bluetooth_stop_reset_scan();

    if(s_bluetooth_init_timer != NULL)
    {
        remove_timer(s_bluetooth_init_timer);
        s_bluetooth_init_timer = NULL;
    }
    s_wait_cnt = 0;

    app_bluetooth_hide_gif();
    app_bluetooth_free_gif();

	if(s_bluetooth_dev)
	{
		GxCore_Free(s_bluetooth_dev);
		s_bluetooth_dev = NULL;
	}
    app_bluez_source_force_scan_off();
	ap_list_destroy_bluetooth();
    app_bluetooth_remove_scan();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_bluetooth_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		if(s_bluetooth_dev_total ==0 && event->key.sym != STBK_EXIT && event->key.sym != STBK_MENU && event->key.sym != VK_BOOK_TRIGGER)
		{
			return EVENT_TRANSFER_STOP;
		}
		switch(event->key.sym)
		{
			case VK_BOOK_TRIGGER:
				s_bluetooth_set_show = false;
				GUI_EndDialog("after wnd_full_screen");
				break;
			case STBK_RIGHT:
				if(s_bluetooth_dev_total == 1 || s_bluetooth_dev_total == 0)
					return EVENT_TRANSFER_STOP;
				if(s_dev_sel < s_bluetooth_dev_total-1)
					s_dev_sel++;
				else
					s_dev_sel = 0;

                app_bluetooth_dev_update(s_bluetooth_dev[s_dev_sel]);
				break;
			case STBK_LEFT:
				if(s_bluetooth_dev_total == 1 || s_bluetooth_dev_total == 0)
					return EVENT_TRANSFER_STOP;
				if(s_dev_sel >0)
					s_dev_sel--;
				else
					s_dev_sel = s_bluetooth_dev_total-1;

                app_bluetooth_dev_update(s_bluetooth_dev[s_dev_sel]);
				break;

            default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

//plz free bluetooth_dev if ret > 0 with GxCore_Free()
uint32_t app_get_bluetooth_list(ubyte64 **bluetooth_dev)
{
    uint32_t i = 0;
    uint32_t bluetooth_num = 0;
    GxMsgProperty_NetDevList dev_list;
    GxMsgProperty_NetDevType type;
    ubyte64 *temp = NULL;

    if(bluetooth_dev == NULL)
        return 0;

    memset(&dev_list, 0, sizeof(GxMsgProperty_NetDevList));
    app_send_msg_exec(GXMSG_NETWORK_DEV_LIST, &dev_list);

    for(i = 0; i < dev_list.dev_num; i++)
    {
        memset(&type, 0, sizeof(GxMsgProperty_NetDevType));
        strcpy(type.dev_name, dev_list.dev_name[i]);
        app_send_msg_exec(GXMSG_NETWORK_GET_TYPE, &type);
        if(type.dev_type == IF_TYPE_BLUETOOTH)
        {
            if((temp = GxCore_Realloc(*bluetooth_dev, sizeof(ubyte64) * (bluetooth_num + 1))) != NULL)
            {
                *bluetooth_dev = temp;
                strcpy((*bluetooth_dev)[bluetooth_num], dev_list.dev_name[i]);
                app_log_debug("~~~~~~[%s]%d, bluetooth: %s!!!!!\n", __func__, __LINE__, (*bluetooth_dev)[bluetooth_num]);
                bluetooth_num++;
            }
        }
    }
    return bluetooth_num;
}

static uint32_t app_bluetooth_dev_list_update(ubyte64 **bluetooth_dev)
{
    ubyte64 *bluetooth_list = NULL;
    uint32_t bluetooth_num = 0;
    uint32_t i = 0;

    if(bluetooth_dev == NULL)
	{
        return 0;
	}

    bluetooth_num = app_get_bluetooth_list(&bluetooth_list);
    if((bluetooth_num > 0) && (bluetooth_list != NULL))
    {
        if(s_bluetooth_menu_mode == bluetooth_MENU_ALL) //all mode
        {
            if((*bluetooth_dev = (ubyte64*)GxCore_Calloc(sizeof(ubyte64), bluetooth_num)) != NULL)
            {
                for(i = 0; i < bluetooth_num; i++)
                {
                    strcpy((*bluetooth_dev)[i], bluetooth_list[i]);
                }
            }
        }
        else //single mode
        {
            for(i = 0; i < bluetooth_num; i++)
            {
                if(strcmp(bluetooth_list[i], s_cur_dev) == 0)
                {
                    bluetooth_num = 1;
                    if((*bluetooth_dev = (ubyte64*)GxCore_Calloc(sizeof(ubyte64), bluetooth_num)) != NULL)
                    {
                        strcpy(**bluetooth_dev, s_cur_dev);
                    }
                    break;
                }
            }
        }

        GxCore_Free(bluetooth_list);
        bluetooth_list = NULL;
    }
    return bluetooth_num;
}

static status_t app_bluetooth_status_msg(void)
{
    GxMessage *new_msg = NULL;

    if((new_msg = GxBus_MessageNew(APPMSG_BLUETOOTH_UI_CONTROL)) != NULL)
    {
        GxBus_MessageSend(new_msg);
        return GXCORE_SUCCESS;
    }
    return GXCORE_ERROR;
}

void _bluetooth_ap_scan_msg_proc(GxMsgProperty_ApList *ap_list)
{
    const char *wnd = GUI_GetFocusWindow();

    if(ap_list == NULL)
        return;

    if((s_bluetooth_dev == NULL) || (strcmp(s_bluetooth_dev[s_dev_sel], ap_list->bluetooth_dev) != 0))
        return;

    app_log_info("~~~~~~[%s]%d, %s ap scan over, %d ap!!!!!\n", __func__, __LINE__, ap_list->bluetooth_dev, ap_list->ap_num);

    memcpy(&s_scan_ap_list, ap_list, sizeof(GxMsgProperty_ApList));

    if((wnd == NULL) || (strcmp(wnd, WND_BLUETOOTH) != 0) || (s_bluetooth_set_show))
    {
        return ;
    }

    app_bt_menu_display_protect();

    if( (s_ap_total == 0) && (s_bluetooth_ap_num >0) )
    {
        s_ap_sel = 0;
        app_bluetooth_ap_list_update(&s_scan_ap_list);
        GUI_SetProperty(LIST_BLUETOOTH, "update_all", NULL);
        GUI_SetProperty(LIST_BLUETOOTH, "select", &s_ap_sel);
    }

    app_bt_menu_display_unprotect();

    if(s_ap_total == 0)
    {
        s_ap_sel = -1;
        if(thiz_bt_connect_status >= BT_A2DP_SCAN_FINISH)
        {
            app_bluetooth_tip_hide();
            app_bluetooth_tip_show(STR_ID_NO_AP);
        }
    }
    else
    {
        //app_bluetooth_remove_scan();
        app_bluetooth_tip_hide();
        app_bluetooth_hide_gif();
        app_update_pop_dlg(WND_POP_TIP);
        app_update_pop_dlg(WND_POP_BOOK);
    }
    GUI_SetInterface("flush", NULL);
}

void _bluetooth_state_msg_proc(GxMsgProperty_NetDevState *bluetooth_state)
{
    const char *wnd = GUI_GetFocusWindow();

    if(bluetooth_state == NULL)
        return;

    if((s_bluetooth_dev == NULL) || (strcmp(s_bluetooth_dev[s_dev_sel], bluetooth_state->dev_name) != 0))
        return;

    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_BLUETOOTH))
    {
        if(((bluetooth_state->dev_state == IF_STATE_CONNECTING) || (bluetooth_state->dev_state == IF_STATE_START))&& (s_bluetooth_set_show == false))
        {
            popdlg_destroy();
        //    app_log_debug("~~~~~~[%s]%d, bluetooth %s state = %d!!!!!\n", __func__, __LINE__, bluetooth_state->dev_name, bluetooth_state->dev_state);
            app_bluetooth_show_gif();
		//	s_bluetooth_set_connecting = 0;
            app_blutetooth_update_status(IF_STATE_CONNECTED);

        }
        else
        {
            app_blutetooth_update_status(bluetooth_state->dev_state);

			if(s_bluetooth_tip_flag == true)
			{
				s_bluetooth_ap_updating = false;
				app_bluetooth_dev_update(s_bluetooth_dev[s_dev_sel]);
			}
        //    app_log_debug("~~~~~~[%s]%d, bluetooth %s state = %d!!!!!\n", __func__, __LINE__, bluetooth_state->dev_name, bluetooth_state->dev_state);
            app_bluetooth_hide_gif();
        }

        if(bluetooth_state->dev_state == IF_STATE_CONNECTED)
        {
            if(s_bluetooth_set_show)
            {
				popdlg_destroy();
                if(GXCORE_SUCCESS == GUI_CheckDialog(WND_KEYBOARD_LANGUAGE))
                    GUI_EndDialog(WND_KEYBOARD_LANGUAGE);
                GUI_SetInterface("flush", NULL);
                app_bluetooth_set_hide();
            }

            if(false == s_bluetooth_ap_updating)
            {
                app_bt_menu_display_protect();

                s_ap_sel = 0;
                app_bluetooth_ap_list_update(&s_scan_ap_list);

                GUI_SetProperty(LIST_BLUETOOTH, "update_all", NULL);
                GUI_SetProperty(LIST_BLUETOOTH, "select", &s_ap_sel);

                app_bt_menu_display_unprotect();

                if(s_bluetooth_tip_flag)
                {
                    app_bluetooth_tip_hide();
                }
            }
        }
        else if(bluetooth_state->dev_state == IF_STATE_IDLE)
        {
            memset(&s_cur_ap, 0, sizeof(GxMsgProperty_CurAp));
            if(s_bluetooth_state == IF_STATE_CONNECTED) //stop
            {
                app_bt_menu_display_protect();

                app_bluetooth_ap_list_update(&s_scan_ap_list);
                GUI_SetProperty(LIST_BLUETOOTH, "update_all", NULL);

                app_bt_menu_display_unprotect();

                if(s_bluetooth_tip_flag)
                    app_bluetooth_tip_hide();
                GUI_EndDialog("after wnd_bluetooth");
                if(s_bluetooth_set_show != false)
                  app_bluetooth_set_hide();
                GUI_SetInterface("flush", NULL);
            }
            else if((s_bluetooth_state == IF_STATE_CONNECTING) || (s_bluetooth_state == IF_STATE_START))//fail
            {
            }
        }
        else if(bluetooth_state->dev_state == IF_STATE_REJ)
        {
			popdlg_destroy();
            if(GXCORE_SUCCESS == GUI_CheckDialog(WND_KEYBOARD_LANGUAGE))
                GUI_EndDialog(WND_KEYBOARD_LANGUAGE);

            app_bluetooth_disconnect();

            app_bluetooth_set_exit();
            app_bluetooth_hide_gif();
            if(s_bluetooth_tip_flag)
                app_bluetooth_tip_hide();

            PopDlg pop = {0};
            pop.type = POP_TYPE_OK;
            pop.str = STR_ID_NO_CONNECT;

            pop.mode = POP_MODE_UNBLOCK;
            popdlg_create(&pop);
        }
        else if(bluetooth_state->dev_state == IF_STATE_DISCONNECTED && s_bluetooth_state != bluetooth_state->dev_state)
        {
            if(wnd != NULL && 0 == strcmp(wnd, WND_BLUETOOTH))
            {
                app_bt_menu_display_protect();

                app_bluetooth_ap_list_update(&s_scan_ap_list);

                app_bt_menu_display_unprotect();

                if(s_bluetooth_set_show)
                    app_bluetooth_set_exit();
                app_bluetooth_hide_gif();
                if(s_bluetooth_tip_flag)
                   app_bluetooth_tip_hide();

                memset(&s_cur_ap, 0, sizeof(GxMsgProperty_CurAp));
            }
        }
    }
    app_update_pop_dlg(WND_POP_TIP);
    app_update_pop_dlg(WND_POP_BOOK);

    s_bluetooth_state = bluetooth_state->dev_state;
}

static void _bluetooth_plug_in_proc(void)
{
    ubyte64 cur_dev = {0};
    uint32_t dev_num = 0;
    uint32_t i = 0;
    const char *wnd = GUI_GetFocusWindow();


    if(s_bluetooth_dev != NULL)
    {
        strcpy(cur_dev, s_bluetooth_dev[s_dev_sel]);
        GxCore_Free(s_bluetooth_dev);
        s_bluetooth_dev = NULL;
    }

    dev_num = 1;//
    app_bluetooth_dev_list_update(&s_bluetooth_dev);
    if((dev_num > 0) && (s_bluetooth_dev != NULL))
    {
        s_dev_sel = 0;
        for(i = 0; i < dev_num; i++)
        {
            if(strcmp(cur_dev, s_bluetooth_dev[i]) == 0)
            {
                s_dev_sel = i;
                break;
            }
        }

        if((wnd != NULL) && (strcmp(wnd, WND_BLUETOOTH) == 0) && (s_bluetooth_set_show == false))
        {
            if(dev_num > 1)
            {
                GUI_SetProperty("img_bluetooth_l","state","show");
                GUI_SetProperty("img_bluetooth_r","state","show");
            }
            else
            {
                GUI_SetProperty("img_bluetooth_l","state","hide");
                GUI_SetProperty("img_bluetooth_r","state","hide");
            }

            if(s_bluetooth_dev_total == 0)
            {
                app_bluetooth_dev_update(s_bluetooth_dev[s_dev_sel]);
            }
        }

        s_bluetooth_dev_total = dev_num;
    }
    else
    {
        s_bluetooth_dev_total = 0;
        if((wnd != NULL) && (strcmp(wnd, WND_BLUETOOTH) == 0) && (s_bluetooth_set_show == false))
        {
            app_bluetooth_ap_list_clean();
            app_bluetooth_hide_gif();
            app_bluetooth_tip_hide();
            GUI_SetProperty(LIST_BLUETOOTH, "update_all", NULL);
            GUI_SetProperty(LIST_BLUETOOTH, "select", &s_ap_sel);
            GUI_SetProperty("text_bluetooth_dev", "string", NULL);
            app_bluetooth_tip_show(STR_ID_NO_BLUETOOTH);
        }
    }
}

static void _bluetooth_plug_out_proc(void)
{
    ubyte64 cur_dev = {0};
    uint32_t i = 0;
    const char *wnd = GUI_GetFocusWindow();

    if(s_bluetooth_dev != NULL)
    {
        strcpy(cur_dev, s_bluetooth_dev[s_dev_sel]);
        GxCore_Free(s_bluetooth_dev);
        s_bluetooth_dev = NULL;
    }

    if(s_bluetooth_set_show)
    {
        GUI_SetProperty(WND_BLUETOOTH, "update", NULL);
        app_bluetooth_set_hide();
    }

    s_bluetooth_dev_total = 1;// app_bluetooth_dev_list_update(&s_bluetooth_dev);
    if((s_bluetooth_dev_total > 0) && (s_bluetooth_dev != NULL))
    {
        s_dev_sel = 0;
        for(i = 0; i < s_bluetooth_dev_total; i++)
        {
            if(strcmp(cur_dev, s_bluetooth_dev[i]) == 0)
            {
                s_dev_sel = i;
                break;
            }
        }

        if((wnd != NULL) && (strcmp(wnd, WND_BLUETOOTH) == 0) && (s_bluetooth_set_show == false))
        {
            if(s_bluetooth_dev_total > 1)
            {
                GUI_SetProperty("img_bluetooth_l","state","show");
                GUI_SetProperty("img_bluetooth_r","state","show");
            }
            else
            {
                GUI_SetProperty("img_bluetooth_l","state","hide");
                GUI_SetProperty("img_bluetooth_r","state","hide");
            }

            if(i > s_bluetooth_dev_total)
            {
                app_bluetooth_dev_update(s_bluetooth_dev[s_dev_sel]);
            }
        }
    }
    else
    {
        if((wnd != NULL) && (strcmp(wnd, WND_BLUETOOTH) == 0) && (s_bluetooth_set_show == false))
        {
            app_bluetooth_ap_list_clean();
            s_ap_sel = -1;
            app_bluetooth_hide_gif();
            app_bluetooth_tip_hide();
            GUI_SetProperty(LIST_BLUETOOTH, "update_all", NULL);
            GUI_SetProperty(LIST_BLUETOOTH, "select", &s_ap_sel);
            GUI_SetProperty("text_bluetooth_dev", "string", NULL);
            //app_bluetooth_tip_show(STR_ID_NO_BLUETOOTH);
        }
    }
}

static bool app_check_bluetooth_ap_exist(ubyte64 ap_name, ubyte32 ap_mac)
{
    bool ret = false;
    int i = 0;

    for(i = 0;i < s_bluetooth_ap_num;i++)
    {
        if( strcmp(thiz_bt_ap_list[i].ap_mac, ap_mac) == 0 )
        {
            ret = true;
            break;
        }
        else if( strcmp(thiz_bt_ap_list[i].bt_name, ap_name) == 0 )
        {
            app_bt_debug("same name: %s but addr: \"%s\"\n", ap_name, ap_mac);
            ret = true;
            break;
        }
    }

    return ret;
}

status_t app_get_bt_security(char *mac)
{
    status_t ret = GXCORE_ERROR;
    unsigned char i = 0;

    GxCore_MutexLock(s_bt_aplist_mutex);
    for(i = 0; i < s_bluetooth_ap_num; i++)
    {
        if(strcmp(mac, thiz_bt_ap_list[i].ap_mac) == 0)
        {
            ret = GXCORE_SUCCESS;
            break;
        }
    }
    GxCore_MutexUnlock(s_bt_aplist_mutex);

    if ( (ret == GXCORE_ERROR) && (thiz_bt_connect_status != BT_A2DP_NONE) )
    {
        app_reset_scan_ap(3);
    }

    return ret;
}

int app_get_bluetooth_apnum(unsigned char *ap_num)
{
    GxCore_MutexLock(s_bt_aplist_mutex);
    *ap_num = s_bluetooth_ap_num;
    GxCore_MutexUnlock(s_bt_aplist_mutex);

    return 0;
}

status_t app_bluetoot_start(char *bt_dev,char *mac)
{
    int ret = 0;
    if(bt_dev == NULL  || mac == NULL)
        return GXCORE_ERROR;
    if(GXCORE_SUCCESS == app_get_bt_security(mac))
    {
        if(thiz_bt_connect_status != BT_A2DP_SOURCE_CONNECT)
        {
            thiz_bt_connect_status = BT_A2DP_SOURCE_CONNETING;
            ret = app_bluez_source_connect(mac);
            if ( ret != 0 )
            {
                thiz_bt_connect_status = BT_A2DP_SOURCE_CONNECT_FAILED ;
            }
        }
        return GXCORE_SUCCESS;
    }

    thiz_bt_connect_status = BT_A2DP_SOURCE_CONNECT_FAILED ;

    return GXCORE_ERROR;
}


void app_bluetooth_msg_set_ap_false(void)
{
	s_bluetooth_ap_updating = false;
}

void app_bluetooth_msg_proc(GxMessage *msg)
{
    int ret = 0;
    GxMsgProperty_NetDevType type;
    GxMsgProperty_UsbBtScanResult *bt_scan;
    GxMsgProperty_PlayerAudioMute player_mute;

    if(msg == NULL)
        return;

    switch(msg->msg_id)
    {
        case GXMSG_USBBT_HOTPLUG_IN:
             initialize_net_if_dev_usb_bt();
             ret = GxCore_BTInit(net_if_dev_usb_bt);
             if(ret < 0)
             {
                 app_log_error("bt init err = %d \n",ret);
             }
             break;
        case GXMSG_USBBT_HOTPLUG_OUT:
             thiz_bt_connect_status = BT_A2DP_NONE;
             app_bluetooth_source_audio_disable();
             app_bluetooth_source_exec_mute();
             app_bluez_stop();
             app_bluetooth_msg_set_ap_false();
             app_bluetooth_unregister_dogle();
             app_bluetooth_clean_scan_aplist_num();
             app_bluetooth_ap_list_clean();
             s_bluetooth_set_connecting = 0;
             break;
        case GXMSG_USBBT_INIT_SUCCESS:
             app_bt_debug("============== recv bt msg : INIT SUCCESS\n");
             app_bluetooth_clean_scan_aplist_num();
             app_bluetooth_register_dogle();
             app_bluez_add_link_key();
             s_scanning = BT_SCAN_STATUS_OFF ;
             thiz_bt_connect_status = BT_A2DP_INIT_SUCCESSFUL;
             break;
        case GXMSG_USBBT_SCAN_RESULT:
             bt_scan = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbBtScanResult);
             if((s_bluetooth_ap_num < MAX_AP_NUM)||(NULL != bt_scan))
             {
                 GxCore_MutexLock(s_bt_aplist_mutex);
                 if(app_check_bluetooth_ap_exist(bt_scan->info.name,bt_scan->info.bd_addr_str) == false)
                 {
                     strcpy(thiz_bt_ap_list[s_bluetooth_ap_num].ap_mac,bt_scan->info.bd_addr_str);
                     strcpy(thiz_bt_ap_list[s_bluetooth_ap_num].bt_name,bt_scan->info.name);
                     s_bluetooth_ap_num++;

                     ap_list_update_bluetooth_connect(thiz_bt_ap_list,s_bluetooth_ap_num);
                     app_bt_debug("device name: %s addr: \"%s\"\n", bt_scan->info.name, bt_scan->info.bd_addr_str);
                 }
                 GxCore_MutexUnlock(s_bt_aplist_mutex);
             }
             break;
        case GXMSG_USBBT_SCAN_FINISH:
             if ( (thiz_bt_connect_status <= BT_A2DP_SCAN_FINISH) || (thiz_bt_connect_status == BT_A2DP_SOURCE_CONNECT_FAILED) )
             {
                 thiz_bt_connect_status = BT_A2DP_SCAN_FINISH;
             }
             app_bt_debug("======ap_num=%d======= recv bt msg : scan finisih\n",s_bluetooth_ap_num);
             if(s_bluetooth_ap_num < (BT_APP_COUNTER/2))
             {
                 //app_reset_scan_ap(1);
                 app_scan_finish_to_scan_again();
             }
             break;
        case GXMSG_USBBT_LINK_KEY_NOTIFY:
             {
                 GxMsgProperty_UsbBtLinkKey    *bt_key;
                 bt_key = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_UsbBtLinkKey);
                 if ( bt_key != NULL )
                 {
                    app_bluez_set_link_key_data(bt_key);
                 }
                 app_bt_debug("============== recv bt msg : LINK_KEY_NOTIFY\n");
             }
             break;
        case GXMSG_USBBT_SINK_CONNECT:
             thiz_bt_connect_status = BT_A2DP_SINK_CONNECT;
             app_bt_debug("============== recv bt msg : sink connect\n");
             app_bluez_set_link_key_mode(BLUETOOTH_LINK_SINK);
             if ( app_is_sink_menu_mode() )
             {
                 app_bt_debug("enter sink menu \n");
                 app_create_sink_menu();
             }
             break;
        case GXMSG_USBBT_SINK_DISCONNECT:
             thiz_bt_connect_status = BT_A2DP_SINK_DISCONNECT;
             app_bt_debug("============== recv bt msg : sink disconnect\n");
             app_bluez_sink_disconnect();
             break;
        case GXMSG_USBBT_SINK_CONNECT_FAILED:
             thiz_bt_connect_status = BT_A2DP_SINK_CONNECT_FAILED ;
             app_bt_debug("============== recv bt msg : sink connect failed\n");
             if ( app_is_sink_menu_mode() )
             {
                 app_pop_set_string(STR_ID_BLUETOOTH_CONNECT_FAILER);
             }
             break;
        case GXMSG_USBBT_SINK_PLAY:
             thiz_bt_connect_status = BT_A2DP_SINK_PLAY;
             app_bt_debug("============== recv bt msg : sink play\n");
             app_bluez_sink_play();
             app_bluetooth_status_msg();
             break;
        case GXMSG_USBBT_SINK_PAUSE:
             thiz_bt_connect_status = BT_A2DP_SINK_PAUSE;
             app_bt_debug("============== recv bt msg : sink pause\n");
             app_bluez_sink_pause();
             app_bluetooth_status_msg();
             break;
        case GXMSG_USBBT_SOURCE_CONNECT:
             thiz_bt_connect_status = BT_A2DP_SOURCE_CONNECT;
             s_bluetooth_set_connecting = 1;
             app_bluetooth_remove_scan();
             app_bt_debug("============== recv bt msg : source connect\n");
             break;
        case GXMSG_USBBT_SOURCE_CONNECT_FAILED:
             thiz_bt_connect_status = BT_A2DP_SOURCE_CONNECT_FAILED;
             s_bluetooth_set_connecting = 0;
             /*
             if(s_bluetooth_ap_num < BT_APP_COUNTER)
             {
                 app_bluez_source_scan_on();
             }*/
             app_bt_debug("============== recv bt msg : source connect failed\n");
             break;
        case GXMSG_USBBT_SOURCE_DISCONNECT:
             thiz_bt_connect_status = BT_A2DP_SOURCE_DISCONNECT;
             s_bluetooth_set_fifo_change = thiz_bt_connect_status;
             s_bluetooth_set_connecting = 0;
             app_bt_debug("============== recv bt msg : source disconnect\n");
             app_bluez_source_pause();
             app_bluetooth_source_exec_mute();
             break;
        case GXMSG_USBBT_SOURCE_PLAY:
             {
                 thiz_bt_connect_status = BT_A2DP_SOURCE_PLAY;
                 app_bt_debug("============== recv bt msg : source play\n");
                 app_bluetooth_source_audio_enable();
                 break;
             }
        case GXMSG_USBBT_SOURCE_PAUSE:
             {
                 player_mute = 0;
                 app_send_msg_exec(GXMSG_PLAYER_AUDIO_MUTE, (void *)(&player_mute));
                 thiz_bt_connect_status = BT_A2DP_SOURCE_PAUSE;
                 app_bt_debug("============== recv bt msg : source pause\n");
                 app_bluetooth_source_audio_disable();
                 break;
             }
        case GXMSG_USBBT_SOURCE_STREAM_ESTABLISHED:
             {
                 app_bt_debug("============== recv bt msg : source stream es table lished\n");
                 GxCore_BTSourceClearFifo();
                 GxCore_BTSourcePlay();
                 app_bluez_source_play();
                 break;
             }
        case GXMSG_BLUETOOTH_SCAN_AP_OVER:
             {
                 GxMsgProperty_ApList *ap_list = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_ApList);
                 if(ap_list != NULL)
                 {
                     _bluetooth_ap_scan_msg_proc(ap_list);
                 }
                 break;
             }
        case GXMSG_BLUETOOTH_SCAN_AP:
        case GXMSG_BLUETOOTH_GET_CUR_AP:
        case GXMSG_BLUETOOTH_SET_CUR_AP:
        case GXMSG_BLUETOOTH_DEL_CUR_AP:
             {
                 break;
             }
        case GXMSG_NETWORK_PLUG_IN:
             {
                 ubyte64 *bluetooth_dev = GxBus_GetMsgPropertyPtr(msg, ubyte64);
                 if(bluetooth_dev != NULL)
                 {
                     memset(&type, 0, sizeof(GxMsgProperty_NetDevType));
                     strcpy(type.dev_name, *bluetooth_dev);
                     app_send_msg_exec(GXMSG_NETWORK_GET_TYPE, &type);

                     if(type.dev_type == IF_TYPE_BLUETOOTH)
                     {
                         _bluetooth_plug_in_proc();
                     }
                 }
                 break;
             }
        case GXMSG_NETWORK_PLUG_OUT:
             {
                 ubyte64 *bluetooth_dev = GxBus_GetMsgPropertyPtr(msg, ubyte64);
                 if(bluetooth_dev != NULL)
                 {
                     memset(&type, 0, sizeof(GxMsgProperty_NetDevType));
                     strcpy(type.dev_name, *bluetooth_dev);
                     app_send_msg_exec(GXMSG_NETWORK_GET_TYPE, &type);
                     if(type.dev_type == IF_TYPE_BLUETOOTH)
                     {
                         if(GUI_CheckDialog(WND_BLUETOOTH) == GXCORE_SUCCESS)
                         {
                             if(GUI_CheckDialog(WND_POP_OPTION_LIST) == GXCORE_SUCCESS)
                             {
                                 app_pop_list_end_dialog();
                             }
                             popdlg_destroy();
                             if(GXCORE_SUCCESS == GUI_CheckDialog(WND_KEYBOARD_LANGUAGE))
                                 GUI_EndDialog(WND_KEYBOARD_LANGUAGE);
                             _bluetooth_plug_out_proc();
                             GUI_EndDialog(WND_BLUETOOTH);
                         }
                         else
                         {
                             if(GUI_CheckDialog(WND_SINK_VIEW) == GXCORE_SUCCESS)
                             {
                                 GUI_EndDialog(WND_SINK_VIEW);
                             }
                         }

                     }
                 }
                 break;
             }

        case GXMSG_NETWORK_STATE_REPORT:
             {
                 GxMsgProperty_NetDevState *bluetooth_state = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevState);
                 if(bluetooth_state != NULL)
                 {
                     memset(&type, 0, sizeof(GxMsgProperty_NetDevType));
                     strcpy(type.dev_name, bluetooth_state->dev_name);
                     app_send_msg_exec(GXMSG_NETWORK_GET_TYPE, &type);
                     if(type.dev_type == IF_TYPE_BLUETOOTH)
                     {
                         _bluetooth_state_msg_proc(bluetooth_state);
                     }
                 }
                 break;
             }

        default:
             break;
    }
    //app_bluetooth_sink_ui_status(msg);
}

status_t app_create_bluetooth_menu(ubyte64 *bluetooth_dev)
{
    if(bluetooth_dev == NULL)
    {
        return GXCORE_ERROR;
    }

    s_dev_sel = 0;
    strcpy(s_cur_dev, *bluetooth_dev);
    app_bluez_sink_dev_disconnect();
    app_bluez_source_pause();
    GUI_CreateDialog(WND_BLUETOOTH);

    if(strcmp(*bluetooth_dev, "bluetooth-ALL") == 0) //all mode
        s_bluetooth_menu_mode = bluetooth_MENU_ALL;
    else
        s_bluetooth_menu_mode = bluetooth_MENU_SINGLE;

    s_bluetooth_dev_total = 0;
    if(s_bluetooth_dev)
    {
        GxCore_Free(s_bluetooth_dev);
        s_bluetooth_dev = NULL;
    }

    s_bluetooth_dev_total = app_bluetooth_dev_list_update(&s_bluetooth_dev);
    if((s_bluetooth_dev_total > 0) && (s_bluetooth_dev != NULL))
    {
        if(s_bluetooth_dev_total > 1)
        {
            GUI_SetProperty("img_bluetooth_l","state","show");
            GUI_SetProperty("img_bluetooth_r","state","show");
        }
        else
        {
            GUI_SetProperty("img_bluetooth_l","state","hide");
            GUI_SetProperty("img_bluetooth_r","state","hide");
        }

        if( (0 == app_bluetooth_is_connect_state() )   &&
            (s_bluetooth_state != IF_STATE_CONNECTED)  &&
            (s_bluetooth_state != IF_STATE_CONNECTING))
        {
            app_blutetooth_update_status(s_bluetooth_state);
        }
        else
        {
            app_blutetooth_update_status(IF_STATE_CONNECTED);
        }
        app_bluetooth_dev_update(s_bluetooth_dev[s_dev_sel]);
    }
    else
        app_bluetooth_tip_show(STR_ID_NO_BLUETOOTH);

    return GXCORE_SUCCESS;
}

void app_bluetooth_menu_exec(void)
{
    ubyte64 bluetooth_dev = {0};

    if (thiz_bt_connect_status == BT_A2DP_NONE)
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.str = STR_ID_BLUETOOTH_DO_NOT_INIT;
        pop.mode = POP_MODE_UNBLOCK;
        pop.format = POP_FORMAT_DLG;
        popdlg_create(&pop);
        return ;
    }

    memset(bluetooth_dev, 0, sizeof(ubyte64));
    strcpy(bluetooth_dev, net_if_dev_usb_bt);
    app_create_bluetooth_menu(&bluetooth_dev);
}
#endif //NETWORK_SUPPORT && bluetooth_SUPPORT
