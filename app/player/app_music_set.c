#include "app.h"
#include "app_windows.h"

#define BOX_MUSIC_SET					"music_set_box"
#define COMBOBOX_SWITCH_SEQUENCE	 	"music_set_combo_play_sequence"
#define COMBOBOX_VIEW_MODE			"music_set_combo_view_mode"

#define COMBOBOX_LANGUAGE     "music_set_combo_language"
#define COMBOBOX_CHARSET      "music_set_combo_charset"
#define NOTPAD_LRC                				"music_view_notepad_lrc"

static uint32_t s_view_mode_bak = PMPSET_MUSIC_VIEW_MODE_SPECTRUM;

char ISO8859_music_code[3] = {DVB_FIRST_ISO8859,0,DVB_THIRD_ISO8859_1};

uint32_t app_music_info_lang_to_charset(void)
{
    int lang_value = 0;
    unsigned int codepage_value = 0;/**/

    GxBus_ConfigGetInt(MUSIC_ID3_LANG, &lang_value, OSD_LANG);

    if(lang_value >= LANG_MAX)
    {
        lang_value = 0;
    }
    if ((strcmp(g_LangName[lang_value], STR_ID_ENGLISH) == 0)
            ||(strcmp(g_LangName[lang_value], STR_ID_FRENCH) == 0)
            ||(strcmp(g_LangName[lang_value], STR_ID_PORTUGUESE) == 0)
            ||(strcmp(g_LangName[lang_value], STR_ID_SPAIN) == 0)
            ||(strcmp(g_LangName[lang_value], STR_ID_ITALIAN) == 0))
    {
        codepage_value = ((0x2<<16)|(0x1));/*windows1252|ISO8859-01*/
    }
    else if ((strcmp(g_LangName[lang_value], STR_ID_ARABIC) == 0)
            ||(strcmp(g_LangName[lang_value], STR_ID_FARSI) == 0))
    {
        codepage_value = ((0x6<<16)|(0x6));/*windows1256|ISO8859-06*/
    }
    else if ((strcmp(g_LangName[lang_value], STR_ID_RUSSIAN) == 0)
            ||(strcmp(g_LangName[lang_value], STR_ID_BULGARIAN) == 0)
            ||(strcmp(g_LangName[lang_value], STR_ID_UKRAINIAN) == 0))
    {
        codepage_value = ((0x1<<16)|(0x5));/*windows1251|ISO8859-05*/
    }
    else if(strcmp(g_LangName[lang_value], STR_ID_TURKISH) == 0)
    {
        codepage_value = ((0x4<<16)|(0x3));/*windows1254|ISO8859-03*/
    }
    else if(strcmp(g_LangName[lang_value], STR_ID_GERMAN) == 0)
    {
        codepage_value = ((0x2<<16)|(0x2));/*windows1252|ISO8859-02*/
    }
    else if(strcmp(g_LangName[lang_value], STR_ID_THAI) == 0)
    {
        codepage_value = ((0x9<<16)|(0x11));/*784|ISO8859-11*/
    }
    else if((strcmp(g_LangName[lang_value], STR_ID_POLISH) == 0)
            ||(strcmp(g_LangName[lang_value], STR_ID_CZECH) == 0))
    {
        codepage_value = ((0x0<<16)|(0x2));/*windows1250|ISO8859-02*/
    }
    else
    {
        codepage_value = ((0x0<<16)|(0x1));/*windows1250|ISO8859-01*/
    }
    GxBus_ConfigSetInt(MUSIC_ID3_CP, codepage_value);

    return 0;
}


static status_t key_left_right(void)
{
	uint32_t item_sel = 0;
	uint32_t value_sel = 0;

	GUI_GetProperty(BOX_MUSIC_SET, "select", (void*)&item_sel);

	switch(item_sel)
	{
		case 0:
			GUI_GetProperty(COMBOBOX_SWITCH_SEQUENCE, "select", (void*)&value_sel);
			pmpset_set_int(PMPSET_MUSIC_PLAY_SEQUENCE, value_sel);
			break;
		case 1:
			GUI_GetProperty(COMBOBOX_VIEW_MODE, "select", (void*)&value_sel);
			pmpset_set_int(PMPSET_MUSIC_VIEW_MODE, value_sel);
			break;
		case 2:
			GUI_GetProperty(COMBOBOX_LANGUAGE, "select", (void*)&value_sel);
			GxBus_ConfigSetInt(MUSIC_ID3_LANG, value_sel);
			app_music_info_lang_to_charset();

		case 3:
			GUI_GetProperty(COMBOBOX_CHARSET, "select", &value_sel);
			GxBus_ConfigSetInt(MUSIC_ID3_CHARSET, value_sel);
			app_music_info_lang_to_charset();

		default:
			break;
	}

	app_log_debug("[MUSIC] keypress_music_set_ok item:%d, value:%d\n", item_sel, value_sel);

	return GXCORE_SUCCESS;
}

SIGNAL_HANDLER  int music_set_service(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	event = (GUI_Event *)usrdata;

	GUI_SendEvent(WIN_MUSIC_VIEW, event);
	GUI_SetProperty(WIN_MUSIC_SET, "update", NULL);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int music_set_init(const char* widgetname, void *usrdata)
{
	int32_t value = 0;
	int32_t sel = 0,i = 0;
	char combobox_buf[256];

	value = pmpset_get_int(PMPSET_MUSIC_PLAY_SEQUENCE);
	GUI_SetProperty(COMBOBOX_SWITCH_SEQUENCE, "select", (void*)&value);

	value = pmpset_get_int(PMPSET_MUSIC_VIEW_MODE);
	GUI_SetProperty(COMBOBOX_VIEW_MODE, "select", (void*)&value);

  /*music language*/
    memset(combobox_buf,0x0,256);
    strcpy(combobox_buf,  "[");

    for(i = 0;i < LANG_MAX;i++)
    {
    if(0 != i) strcat(combobox_buf, ",");
    strcat(combobox_buf, g_LangName[i]);

    }
    strcat(combobox_buf,  "]");

    app_log_info("music: combobox_buf %s\n", combobox_buf);
    GUI_SetProperty(COMBOBOX_LANGUAGE, "content", (void*)combobox_buf);
    GxBus_ConfigGetInt(MUSIC_ID3_LANG, &sel, OSD_LANG);
    GUI_SetProperty(COMBOBOX_LANGUAGE, "select", &sel);

	/*charset select*/
	GxBus_ConfigGetInt(MUSIC_ID3_CHARSET, &sel, MUSIC_ID3_CHARSET_VALUE);
	GUI_SetProperty(COMBOBOX_CHARSET, "select", &sel);
	s_view_mode_bak = value;

	return 0;
}

SIGNAL_HANDLER int music_set_destroy(const char* widgetname, void *usrdata)
{
	uint32_t value_sel = 0;

	value_sel = pmpset_get_int(PMPSET_MUSIC_VIEW_MODE);
	if(PMPSET_MUSIC_VIEW_MODE_LRC == value_sel)
	{
		if(s_view_mode_bak != PMPSET_MUSIC_VIEW_MODE_LRC)
		{
			if(s_view_mode_bak == PMPSET_MUSIC_VIEW_MODE_SPECTRUM)
			{
                spectrum_destroy();
			}

            lyrics_create(NOTPAD_LRC);
		}
	}
	else if(PMPSET_MUSIC_VIEW_MODE_SPECTRUM == value_sel)
	{
		if(s_view_mode_bak != PMPSET_MUSIC_VIEW_MODE_SPECTRUM)
		{
			if(s_view_mode_bak == PMPSET_MUSIC_VIEW_MODE_LRC)
			{
				lyrics_destroy();
			}
			extern void music_view_spectrum_create(void);
			music_view_spectrum_create();
		}
	}
	return 0;
}


SIGNAL_HANDLER int music_set_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	if(NULL == usrdata) return EVENT_TRANSFER_STOP;

	event = (GUI_Event *)usrdata;
	switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
	{
		case VK_BOOK_TRIGGER:
            GUI_SetProperty("music_view_text_name1", "string", NULL);
            GUI_EndDialog("after wnd_full_screen");
			break;

		case APPK_OK:
			//key_ok();
			break;
		case APPK_RIGHT:
		case APPK_LEFT:
			key_left_right();
			break;

		case APPK_VOLUP:
		case APPK_VOLDOWN:
		case APPK_GREEN:
		case APPK_YELLOW:
			return EVENT_TRANSFER_STOP;

		case APPK_REPEAT:
		{
			int value;

			value = pmpset_get_int(PMPSET_MUSIC_PLAY_SEQUENCE);
			if(value == PMPSET_MUSIC_PLAY_SEQUENCE_SEQUENCE)
			{
				value = PMPSET_MUSIC_PLAY_SEQUENCE_ONLY_ONCE;
			}
			else
			{
				value++;
			}
				pmpset_set_int(PMPSET_MUSIC_PLAY_SEQUENCE, value);

			GUI_SetProperty(COMBOBOX_SWITCH_SEQUENCE, "select", (void*)&value);
			break;
		}
		default:
			break;
	}

	return EVENT_TRANSFER_KEEPON;
}

