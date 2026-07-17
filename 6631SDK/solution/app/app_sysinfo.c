#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "module/config/gxconfig.h"
#include "app_utility.h"
#include "app_config.h"
#include <fcntl.h>

typedef enum
{
    ITEM_SYSINFO_SPD_LIB=0,
	ITEM_SYSINFO_SOFTWARE,
#if UPDATE_SUPPORT_FLAG
	ITEM_SYSINFO_OTA_TIME,
#endif
#if KERNEL_SUPPORT_FLAG
    ITEM_SYSINFO_LINUX_KERNEL,
#endif
	ITEM_SYSINFO_HARDWARE,
	ITEM_SYSINFO_HARDWARE_INFO,
	ITEM_SYSINFO_BUILD_TIME,
	ITEM_SYSINFO_CHIP_SN,
	ITEM_SYSINFO_TOTAL
}SystemInfomationOptSel;

#define STSTEN_BOXITEM_COUNT		ITEM_SYSINFO_BUILD_TIME

#define SYSTEM_INFO_TOTAL_ITEM		ITEM_SYSINFO_TOTAL

#define VERSION_LEN      64
#define DATE_TIME_LEN    24
#define IMG_BAR_HALF_FOCUS_L_INFO "s_bar_half_focus_l"
#define IMG_BAR_HALF_UNFOCUS_INFO "s_bar_half_unfocus"

static SystemSettingOpt s_sysinfo_opt;
static SystemSettingItem s_sysinfo_item[ITEM_SYSINFO_TOTAL];
static SystemInfomationOptSel s_SystemInfoSel = ITEM_SYSINFO_SPD_LIB;

static char *s_system_item_title[] =
{
	"text_system_info_item1",
	"text_system_info_item2",
	"text_system_info_item3",
	"text_system_info_item4",
	"text_system_info_item5",
	"text_system_info_item6",
	"text_system_info_item7",
	"text_system_info_item8",
	"text_system_info_item9",
};

static char *s_item_content_btn_info[] =
{
	"text_system_info_item10",
	"text_system_info_item11",
	"text_system_info_item12",
	"text_system_info_item13",
	"text_system_info_item14",
	"text_system_info_item15",
	"text_system_info_item16",
	"text_system_info_item17",
	"text_system_info_item18",
};

#ifdef ECOS_OS

char *strnset(char *str1,char ch,unsigned n)
 {
	int index = 0;
	char *tmp = (char *) str1;

	while((*tmp!='\0')||(index < n+12))
	{
		index++;
	    if(index == n)
		{
			*tmp=ch;
		}
		tmp++;
	}
	return str1;
 }
#endif

#if FACTORY_TEST_SUPPORT
char * app_system_info_get_sw_ver(void)
{
	return APP_VER;
}
#endif

#if UPDATE_SUPPORT_FLAG
static void app_ota_ver_info_item_init(void)
{
    static char hw_version[VERSION_LEN];
	time_t rawtime;
	struct tm timeinfo = {0};
	memset(hw_version, 0, VERSION_LEN);

	if (GXCORE_SUCCESS != app_oem_get(OEM_HW_VERSION, hw_version, VERSION_LEN))
	{
        strcat(hw_version," ");
	}
	time(&rawtime);
	localtime_r(&rawtime, &timeinfo);
	snprintf(hw_version, VERSION_LEN, "%s", BOOT_OTA_VER);
	s_sysinfo_item[ITEM_SYSINFO_OTA_TIME].itemTitle = STR_ID_UPDATE;
	s_sysinfo_item[ITEM_SYSINFO_OTA_TIME].itemType = ITEM_PUSH;
	s_sysinfo_item[ITEM_SYSINFO_OTA_TIME].itemProperty.itemPropertyBtn.string= hw_version;
	s_sysinfo_item[ITEM_SYSINFO_OTA_TIME].itemCallback.btnCallback.BtnPress= NULL;
	s_sysinfo_item[ITEM_SYSINFO_OTA_TIME].itemStatus = ITEM_NORMAL;

}
#endif

#if KERNEL_SUPPORT_FLAG
static void app_linux_kernel_info_item_init(void)
{
    static char build_time[VERSION_LEN];
	memset(build_time, 0, VERSION_LEN);

	snprintf(build_time, VERSION_LEN, "%s", LINUX_KERNEL_VER);
	s_sysinfo_item[ITEM_SYSINFO_LINUX_KERNEL].itemTitle = STR_ID_KERNEL_VER;
	s_sysinfo_item[ITEM_SYSINFO_LINUX_KERNEL].itemType = ITEM_PUSH;
	s_sysinfo_item[ITEM_SYSINFO_LINUX_KERNEL].itemProperty.itemPropertyBtn.string= build_time;
	s_sysinfo_item[ITEM_SYSINFO_LINUX_KERNEL].itemCallback.btnCallback.BtnPress= NULL;
	s_sysinfo_item[ITEM_SYSINFO_LINUX_KERNEL].itemStatus = ITEM_NORMAL;
}
#endif

static void app_software_item_init(void)
{
    static char sw_version[VERSION_LEN];

	memset(sw_version, 0, VERSION_LEN);

	if(GXCORE_ERROR == app_oem_get(OEM_SW_VERSION, sw_version, VERSION_LEN))
        snprintf(sw_version, VERSION_LEN, "%s", APP_VER);

	s_sysinfo_item[ITEM_SYSINFO_SOFTWARE].itemTitle = STR_ID_APPLICATION_VER;
	s_sysinfo_item[ITEM_SYSINFO_SOFTWARE].itemType = ITEM_PUSH;
	s_sysinfo_item[ITEM_SYSINFO_SOFTWARE].itemProperty.itemPropertyBtn.string= sw_version;
	s_sysinfo_item[ITEM_SYSINFO_SOFTWARE].itemProperty.itemPropertyBtn.roll_format= true;
	s_sysinfo_item[ITEM_SYSINFO_SOFTWARE].itemCallback.btnCallback.BtnPress= NULL;
	s_sysinfo_item[ITEM_SYSINFO_SOFTWARE].itemStatus = ITEM_NORMAL;
}

static void app_hardware_item_init(void)
{
    static char hw_version[VERSION_LEN] = {0};

    memset(hw_version, 0, VERSION_LEN);

    if(GXCORE_ERROR == app_oem_get(OEM_HW_VERSION, hw_version, VERSION_LEN))
        snprintf(hw_version, VERSION_LEN, "%s", HARDWARE_VER);

	s_sysinfo_item[ITEM_SYSINFO_HARDWARE].itemTitle = STR_ID_HARDWARE_VER;
	s_sysinfo_item[ITEM_SYSINFO_HARDWARE].itemType = ITEM_PUSH;
	s_sysinfo_item[ITEM_SYSINFO_HARDWARE].itemProperty.itemPropertyBtn.string= hw_version;
	s_sysinfo_item[ITEM_SYSINFO_HARDWARE].itemCallback.btnCallback.BtnPress= NULL;
	s_sysinfo_item[ITEM_SYSINFO_HARDWARE].itemStatus = ITEM_NORMAL;
}

static int app_read_flash_size(char *size_str)
{
    uint32_t flash_total_size;
    flash_total_size=app_get_flash_size();
    get_size_str(size_str, flash_total_size);

    return 0;
}
static int app_read_ddr_size(char *size_str)
{
    uint32_t ddr_total_size;
    ddr_total_size=app_get_ddr_size();
    get_size_str(size_str, ddr_total_size);
    return 0;
}

static void app_hardware_info_init(void)
{
    static char hardware_info[VERSION_LEN] = {0};
    char flash_size[VERSION_LEN] = {0};
    char ddr_size[VERSION_LEN] = {0};

    app_read_flash_size(flash_size);
    app_read_ddr_size(ddr_size);
    strcpy(hardware_info, flash_size);
    strcat(hardware_info, "-Flash/");
    strcat(hardware_info, ddr_size);
    strcat(hardware_info, "-DDRAM");

    s_sysinfo_item[ITEM_SYSINFO_HARDWARE_INFO].itemTitle = STR_ID_HARDWARE_INFO;
	s_sysinfo_item[ITEM_SYSINFO_HARDWARE_INFO].itemType = ITEM_PUSH;
	s_sysinfo_item[ITEM_SYSINFO_HARDWARE_INFO].itemProperty.itemPropertyBtn.string= hardware_info;
	s_sysinfo_item[ITEM_SYSINFO_HARDWARE_INFO].itemCallback.btnCallback.BtnPress= NULL;
	s_sysinfo_item[ITEM_SYSINFO_HARDWARE_INFO].itemStatus = ITEM_NORMAL;
}

static void app_spd_lib_item_init(void)
{
    static char spd_version[VERSION_LEN];

	memset(spd_version, 0, VERSION_LEN);

    snprintf(spd_version, VERSION_LEN, "%s", PLATFORM_LIB_VER);
    s_sysinfo_item[ITEM_SYSINFO_SPD_LIB].itemTitle = STR_ID_SPD_LIB_VER;
	s_sysinfo_item[ITEM_SYSINFO_SPD_LIB].itemType = ITEM_PUSH;
	s_sysinfo_item[ITEM_SYSINFO_SPD_LIB].itemProperty.itemPropertyBtn.string= spd_version;
	s_sysinfo_item[ITEM_SYSINFO_SPD_LIB].itemCallback.btnCallback.BtnPress= NULL;
	s_sysinfo_item[ITEM_SYSINFO_SPD_LIB].itemStatus = ITEM_NORMAL;
}

static void app_build_time_item_init(void)
{
	s_sysinfo_item[ITEM_SYSINFO_BUILD_TIME].itemTitle = STR_ID_BUILD_TIME;
	s_sysinfo_item[ITEM_SYSINFO_BUILD_TIME].itemType = ITEM_PUSH;
	s_sysinfo_item[ITEM_SYSINFO_BUILD_TIME].itemProperty.itemPropertyBtn.string= SOFEWARE_TIME;
	s_sysinfo_item[ITEM_SYSINFO_BUILD_TIME].itemCallback.btnCallback.BtnPress= NULL;
	s_sysinfo_item[ITEM_SYSINFO_BUILD_TIME].itemStatus = ITEM_NORMAL;
}

static void app_chip_sn_item_init(void)
{
    char chipidbuf[8] = {0};
    static char chipsn[VERSION_LEN];
    app_chip_sn_get(chipidbuf, 8);
    sprintf(chipsn,"%02x%02x%02x%02x%02x%02x%02x%02x",chipidbuf[0],chipidbuf[1],chipidbuf[2],chipidbuf[3],chipidbuf[4],chipidbuf[5],chipidbuf[6],chipidbuf[7]);
    s_sysinfo_item[ITEM_SYSINFO_CHIP_SN].itemTitle = STR_ID_CHIP_SN;
    s_sysinfo_item[ITEM_SYSINFO_CHIP_SN].itemType = ITEM_PUSH;
    s_sysinfo_item[ITEM_SYSINFO_CHIP_SN].itemProperty.itemPropertyBtn.string= chipsn;
    s_sysinfo_item[ITEM_SYSINFO_CHIP_SN].itemCallback.btnCallback.BtnPress= NULL;
    s_sysinfo_item[ITEM_SYSINFO_CHIP_SN].itemStatus = ITEM_NORMAL;
}

static void app_system_info_item_property(int sel, ItemProPerty *property)
{
	if(sel >= SYSTEM_INFO_TOTAL_ITEM)
		return;

	if(s_sysinfo_opt.item[sel].itemType == ITEM_PUSH)
	{
		if(property->itemPropertyBtn.string != NULL)
		{
			GUI_SetProperty(s_item_content_btn_info[sel], "string", property->itemPropertyBtn.string);
        }
	}
	if(property->itemPropertyBtn.roll_format)
	{
		GUI_SetProperty(s_item_content_btn_info[sel], "format","fix_r2l");
    }

	GUI_SetProperty(s_system_item_title[sel], "string", s_sysinfo_opt.item[sel].itemTitle);
}


static SystemInfomationOptSel app_system_info_last_sel(SystemInfomationOptSel cur_sel)
{
	SystemInfomationOptSel sel = cur_sel;

	while(1)
	{
		(sel  == ITEM_SYSINFO_SPD_LIB) ? sel = s_sysinfo_opt.itemNum - 1 : sel--;

		if((s_sysinfo_opt.item[sel].itemStatus == ITEM_NORMAL)
			|| (sel == cur_sel))
		{
			return sel;
		}
	}
}

static SystemInfomationOptSel app_system_info_next_sel(SystemInfomationOptSel cur_sel)
{
	SystemInfomationOptSel sel = cur_sel;

	while(1)
	{
		(sel + 1 == s_sysinfo_opt.itemNum) ? sel = ITEM_SYSINFO_SPD_LIB : sel++;

		if((s_sysinfo_opt.item[sel].itemStatus == ITEM_NORMAL)
			|| (sel == cur_sel))
		{
			return sel;
		}
	}
}

static void app_system_info_focus_item(int sel)
{
	if(s_sysinfo_opt.item[sel].itemStatus == ITEM_NORMAL)
	{

		GUI_SetProperty(s_system_item_title[sel], "string", s_sysinfo_opt.item[sel].itemTitle);

		GUI_SetProperty(s_item_content_btn_info[sel], "state", "focus");
		GUI_SetProperty(s_item_content_btn_info[sel], "backcolor", "#ff00ff");

		s_SystemInfoSel = sel;
	}
}

static void app_system_info_unfocus_item(int sel)
{
	if(s_sysinfo_opt.item[sel].itemStatus == ITEM_NORMAL)
	{
		GUI_SetProperty(s_system_item_title[sel], "string", s_sysinfo_opt.item[sel].itemTitle);
		GUI_SetProperty(s_item_content_btn_info[sel], "backcolor", "#31314a");
	}
}

static int app_system_info_init(SystemSettingOpt *settingOpt)
{
	int sel;
	int n_item_total  = 0;
	if(settingOpt == NULL)
		return -1;

	GUI_CreateDialog(WND_SYSTEM_INFO);
	GUI_SetProperty("text_system_info_title", "string", settingOpt->menuTitle);

	if(settingOpt->itemNum > SYSTEM_INFO_TOTAL_ITEM)
	{
		n_item_total = SYSTEM_INFO_TOTAL_ITEM;
	}
	else
	{
		n_item_total = settingOpt->itemNum;
	}

	for(sel = 0; sel < n_item_total; sel++)
	{
		app_system_info_item_property(sel, &settingOpt->item[sel].itemProperty);

	}
	app_system_info_focus_item(s_SystemInfoSel);
	return 0;
}

void app_sysinfo_menu_exec(void)
{
	memset(&s_sysinfo_opt, 0 ,sizeof(s_sysinfo_opt));

	s_sysinfo_opt.menuTitle = STR_ID_SYSTEM_INFO;
	s_sysinfo_opt.titleImage = "s_title_left_utility";
	s_sysinfo_opt.itemNum = SYSTEM_INFO_TOTAL_ITEM;
	s_sysinfo_opt.timeDisplay = TIP_HIDE;
	s_sysinfo_opt.item = s_sysinfo_item;

	app_spd_lib_item_init();
	app_software_item_init();
#if UPDATE_SUPPORT_FLAG
	app_ota_ver_info_item_init();
#endif
#if KERNEL_SUPPORT_FLAG
	app_linux_kernel_info_item_init();
#endif
    app_hardware_item_init();
    app_hardware_info_init();
    app_build_time_item_init();
    app_chip_sn_item_init();

	s_sysinfo_opt.exit = NULL;

	app_system_info_init(&s_sysinfo_opt);
}

SIGNAL_HANDLER int app_system_info_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
				case VK_BOOK_TRIGGER:
					GUI_EndDialog("after wnd_full_screen");
                    GUI_SetInterface("flush", NULL);
					break;
				case STBK_UP:
					app_system_info_unfocus_item(s_SystemInfoSel);
					s_SystemInfoSel = app_system_info_last_sel(s_SystemInfoSel);
					app_system_info_focus_item(s_SystemInfoSel);
					break;

				case STBK_DOWN:
					app_system_info_unfocus_item(s_SystemInfoSel);
					s_SystemInfoSel = app_system_info_next_sel(s_SystemInfoSel);
					app_system_info_focus_item(s_SystemInfoSel);
					break;
				case STBK_EXIT:
				case STBK_MENU:
					GUI_EndDialog(WND_SYSTEM_INFO);
					GUI_SetProperty(WND_SYSTEM_INFO,"draw_now",NULL);
					break;
				default:
					break;
			}

		default:
			break;
	}

	return ret;
}

SIGNAL_HANDLER int app_system_info_got_focus(GuiWidget *widget, void *usrdata)
{
    GUI_SetProperty(s_item_content_btn_info[ITEM_SYSINFO_SOFTWARE], "format", "fix_r2l");
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_system_info_lost_focus(GuiWidget *widget, void *usrdata)
{
    GUI_SetProperty(s_item_content_btn_info[ITEM_SYSINFO_SOFTWARE], "format", "static");
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_system_info_box_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;

	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
			switch(event->key.sym)
			{

				case STBK_UP:
				case STBK_DOWN:
                    ret = EVENT_TRANSFER_STOP;
                    break;

				case STBK_LEFT:
				case STBK_RIGHT:

					break;

				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

#if WEBAPI_SUPPORT
#include <cJSON.h>

void app_webapi_make_infor_data(cJSON *json)
{
	char mac[] = "12:34:56:78:12:34";
	char chip_name[16];
	char chip_id[8];
	char chip_idstr[20];
	char infor_str[40];

	//loader version
	cJSON_AddStringToObject(json, STR_ID_LOADER_VER, PLATFORM_LIB_VER);

	//kernel version
#if KERNEL_SUPPORT_FLAG
	cJSON_AddStringToObject(json, STR_ID_KERNEL_VER, LINUX_KERNEL_VER);
#endif

	//library version
	cJSON_AddStringToObject(json, STR_ID_SPD_LIB_VER, PLATFORM_LIB_VER);

	//software version
	cJSON_AddStringToObject(json, STR_ID_APPLICATION_VER, APP_VER);

	//hardware version
	cJSON_AddStringToObject(json, STR_ID_HARDWARE_VER, HARDWARE_VER);

	//build time
	cJSON_AddStringToObject(json, STR_ID_BUILD_TIME, SOFEWARE_TIME);

	//mac
	generatorMac(mac);
	cJSON_AddStringToObject(json, "mac", mac);

	//chip infor
	memset(chip_name, 0, sizeof(chip_name));
	app_chip_name_get(chip_name, sizeof(chip_name));
	memset(chip_id, 0, sizeof(chip_id));
	memset(chip_idstr, 0, sizeof(chip_idstr));
	if(app_chip_sn_get(chip_id, 8) == 0)
	{
		sprintf(chip_idstr,"%02x%02x%02x%02x%02x%02x%02x%02x",
			chip_id[0],chip_id[1],chip_id[2],chip_id[3],
			chip_id[4],chip_id[5],chip_id[6],chip_id[7]);
	}
	sprintf(infor_str, "%s_%s", chip_name, chip_idstr);
	cJSON_AddStringToObject(json, "chip_info", infor_str);

}

#endif


