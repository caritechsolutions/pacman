#include "app.h"
#include "app_module.h"
#include "app_msg.h"
#include "gui_core.h"
#include "app_default_params.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_keyboard_language.h"
#include "app_qrencode.h"
#if WEBLET_SUPPORT
#include "app_weblet.h"
#endif
#include "app_windows.h"
#ifdef GXTEST_GET_INFO
#include "gxtest.h"
#endif

/***********************************************************************************/
// cur_sel: from 1 to max_sel
/***********************************************************************************/
static int8_t _get_key_num_by_coordinate(int8_t x, int8_t y);
static void app_language_keyboard_move(uint16_t key);

static char * s_lang_keyboard_input = NULL;
static bool is_Superscript_subscript = false;
static uint8_t s_lan_keyboard_caps = 1;
static uint8_t old_lan_keyboard_caps = 0;
static int8_t s_lan_keyboard_x = 3;
static int8_t s_lan_keyboard_y = 3;
static int s_select_language_status = 0;
static uint8_t thiz_pos_num = 1;
static PopKeyboard s_keyboard_full;
static int s_kb_mode = 0;

extern int gxgdi_is_arabic(const unsigned char *str);
extern int app_pop_list_end_dialog(void);
static void app_language_keyboard_switch_input_mode(int mode);
#if WEBLET_SUPPORT && QR_POP_SUPPORT
static char url[256] = {0};
static int isfirstin = 0;
static int app_qrencode_destroy(void);
#endif
static keyboardsavekeycb s_save_cb = NULL;

void app_input_save_space_cb(void)
{
	uint8_t len = strlen(s_lang_keyboard_input);

	if (len == 0)
	{
		PopDlg pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_OK;
		pop.mode = POP_MODE_UNBLOCK;
        pop.str = STR_ID_INPUT_EMPTY_TIP;
        popdlg_create(&pop);
		return;
	}

	GUI_EndDialog(WND_KEYBOARD_LANGUAGE);
	GUI_SetInterface("flush",NULL);
	if (s_keyboard_full.release_cb != NULL)
		s_keyboard_full.release_cb(&s_keyboard_full);

	return;
}

void app_keyboard_save_cb(keyboardsavekeycb save_callbak)
{
    s_save_cb = save_callbak;
}

#ifdef GXTEST_GET_INFO
void app_keyboard_info_get(test_keyboard_info *info)
{
    if(NULL == info)
        return;

    info->cur_lang = g_KB_LangName[s_select_language_status];
    info->cur_caps = s_lan_keyboard_caps;
    info->coordinate.x = s_lan_keyboard_x;
    info->coordinate.y = s_lan_keyboard_y;
    info->key_num = _get_key_num_by_coordinate(s_lan_keyboard_x, s_lan_keyboard_y);

    if(info->key_num >= KEY_LAM_CLR_NUM || info->key_num < 0 || info->cur_caps > 2)
    {
        info->str = NULL;
        return;
    }

    if(2 == s_lan_keyboard_caps)
        info->str = s_english_keymap[info->key_num][s_lan_keyboard_caps];
    else if(ITEM_ARABIC == s_select_language_status)
        info->str = s_arabic_keymap[info->key_num][s_lan_keyboard_caps];
    else if(ITEM_FARSI == s_select_language_status)
        info->str = s_persian_keymap[info->key_num][s_lan_keyboard_caps];
    else if(ITEM_VIETNAMESE == s_select_language_status)
        info->str = s_vietnamese_keymap[info->key_num][s_lan_keyboard_caps];
    else if(ITEM_RUSSIAN == s_select_language_status)
        info->str = s_russian_keymap[info->key_num][s_lan_keyboard_caps];
    else if(ITEM_TURKISH == s_select_language_status)
        info->str = s_turkish_keymap[info->key_num][s_lan_keyboard_caps];
    else if(ITEM_BULGARAIN == s_select_language_status)
        info->str = s_bulgarain_keymap[info->key_num][s_lan_keyboard_caps];
    else if(ITEM_PORTUGUESE == s_select_language_status)
        info->str = s_portuguese_keymap[info->key_num][s_lan_keyboard_caps];
    else if(ITEM_SPANISH == s_select_language_status)
        info->str = s_spanish_keymap[info->key_num][s_lan_keyboard_caps];
    else if(ITEM_UKRAINIAN == s_select_language_status)
        info->str = s_ukrainian_keymap[info->key_num][s_lan_keyboard_caps];
    else if(ITEM_THAI == s_select_language_status)
        info->str = s_thailand_keymap[info->key_num][s_lan_keyboard_caps];
	else if(ITEM_POLSKI == s_select_language_status)
        info->str = s_polski_keymap[info->key_num][s_lan_keyboard_caps];
    else
        info->str = s_english_keymap[info->key_num][s_lan_keyboard_caps];

    return;
}

void app_keyboard_coordinate_get_by_str(char *str, test_coordinate *coordinate)
{
    int32_t i = 0;
    int32_t str_len = 0;

    if(NULL == str || NULL == coordinate)
        return;

    str_len = strlen(str);

    if(s_lan_keyboard_caps > 2 || 0 == str_len)
    {
        coordinate->x = -1;
        coordinate->y = -1;
        app_log_error("\033[31mError, caps: %d, str_len: %d\033[0m\n", s_lan_keyboard_caps, str_len);
        return;
    }

    for(i = 0; i < KEY_COUNT; i++)
    {
        if(2 == s_lan_keyboard_caps)
        {
            if(0 == strncmp(s_english_keymap[i][s_lan_keyboard_caps], str, str_len))
                break;
        }
        else if(ITEM_ARABIC == s_select_language_status)
        {
            if(0 == strncmp(s_arabic_keymap[i][s_lan_keyboard_caps], str, str_len))
                break;
        }
        else if(ITEM_FARSI == s_select_language_status)
        {
            if(0 == strncmp(s_persian_keymap[i][s_lan_keyboard_caps], str, str_len))
                break;
        }
        else if(ITEM_VIETNAMESE == s_select_language_status)
        {
            if(0 == strncmp(s_vietnamese_keymap[i][s_lan_keyboard_caps], str, str_len))
                break;
        }
        else if(ITEM_RUSSIAN == s_select_language_status)
        {
            if(0 == strncmp(s_russian_keymap[i][s_lan_keyboard_caps], str, str_len))
                break;
        }
        else if(ITEM_TURKISH == s_select_language_status)
        {
            if(0 == strncmp(s_turkish_keymap[i][s_lan_keyboard_caps], str, str_len))
                break;
        }
        else if(ITEM_THAI == s_select_language_status)
        {
            if(0 == strncmp(s_thailand_keymap[i][s_lan_keyboard_caps], str, str_len))
                break;
        }
        else if(ITEM_BULGARAIN == s_select_language_status)
        {
            if(0 == strncmp(s_bulgarain_keymap[i][s_lan_keyboard_caps], str, str_len))
                break;
        }
        else if(ITEM_PORTUGUESE == s_select_language_status)
        {
            if(0 == strncmp(s_portuguese_keymap[i][s_lan_keyboard_caps], str, str_len))
                break;
        }
        else if(ITEM_SPANISH == s_select_language_status)
        {
            if(0 == strncmp(s_spanish_keymap[i][s_lan_keyboard_caps], str, str_len))
                break;
        }
        else if(ITEM_UKRAINIAN == s_select_language_status)
        {
            if(0 == strncmp(s_ukrainian_keymap[i][s_lan_keyboard_caps], str, str_len))
                break;
        }
		else if(ITEM_POLSKI == s_select_language_status)
        {
            if(0 == strncmp(s_polski_keymap[i][s_lan_keyboard_caps], str, str_len))
                break;
        }
        else
        {
            if(0 == strncmp(s_english_keymap[i][s_lan_keyboard_caps], str, str_len))
                break;
        }
    }

    if(i == KEY_COUNT)
    {
        app_log_error("\033[31mCan't find %s in language map\033[0m\n", str);
        coordinate->x = -1;
        coordinate->y = -1;
    }
    else
    {
        if(i < 12)
        {
            coordinate->x = i;
            coordinate->y = 0;
        }
        else if(i < 24)
        {
            coordinate->x = i - 12;
            coordinate->y = 1;
        }
        else if(i < 35)
        {
            coordinate->x = i - 24;
            coordinate->y = 2;
        }
        else
        {
            coordinate->x = i - 35;
            coordinate->y = 3;
        }
    }

    return;
}
#endif
static status_t keyboard_update_input(uint32_t input_len)
{
    if(s_lang_keyboard_input != NULL)
    {
        GxCore_Free(s_lang_keyboard_input);
        s_lang_keyboard_input = NULL;
    }
    if (s_lang_keyboard_input == NULL)
    {
        s_lang_keyboard_input = (char *)GxCore_Malloc((input_len+1) * sizeof(char));
        if(s_lang_keyboard_input == NULL)
        {
            app_log_error("Memory allocation failed\n");
            return GXCORE_ERROR;
        }
    }
    return GXCORE_SUCCESS;
}
/*
U-00000000 - U-0000007F 	: 0xxxxxxx
U-00000080 - U-000007FF 	: 110xxxxx 10xxxxxx
U-00000800 - U-0000FFFF	: 1110xxxx 10xxxxxx 10xxxxxx
U-00010000 - U-001FFFFF  	: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
U-00200000 - U-03FFFFFF	: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
U-04000000 - U-7FFFFFFF	: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
*/
static signed char calculte_utf8_length(const char* InStr, unsigned char StrLen)
{
	const char *p = InStr;
	if((p == NULL) ||(StrLen == 0) || (strlen(InStr) < StrLen))
	{
		app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	if((p[0] >> 7) == 0)
	{
		return 1; // one byte
	}
	else if((p[0] >>5) == 0x06)
	{
		if(StrLen < 2)
		{
			app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
			return -1;
		}
		else
		{
			if((p[1] >>6) == 0x02)
				return 2;// two bytes
			else
			{
				app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
				return -1;
			}
		}
	}
	else if((p[0] >>4) == 0x0e)
	{
		if(StrLen < 3)
		{
			app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
			return -1;
		}
		else
		{
			if(((p[1] >>6) == 0x02) && ((p[2] >>6) == 0x02))
				return 3;// three bytes
			else
			{
				app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
				return -1;
			}
		}
	}
	else if((p[0] >>3) == 0x1e)
	{
		if(StrLen < 4)
		{
			app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
			return -1;
		}
		else
		{
			if(((p[1] >>6) == 0x02) && ((p[2] >>6) == 0x02) && ((p[3] >>6) == 0x02))
				return 4;// four bytes
			else
			{
				app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
				return -1;
			}
		}
	}
	else if((p[0] >>2) == 0x3e)
	{
		if(StrLen < 5)
		{
			app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
			return -1;
		}
		else
		{
			if(((p[1] >>6) == 0x02) && ((p[2] >>6) == 0x02) && ((p[3] >>6) == 0x02)  && ((p[4] >>6) == 0x02))
				return 5;// five bytes
			else
			{
				app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
				return -1;
			}
		}
	}
	else if((p[0] >>1) == 0x7e)
	{
		if(StrLen < 6)
		{
			app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
			return -1;
		}
		else
		{
			if(((p[1] >>6) == 0x02) && ((p[2] >>6) == 0x02) && ((p[3] >>6) == 0x02)  && ((p[4] >>6) == 0x02) && ((p[4] >>6) == 0x02))
				return 6;// five bytes
			else
			{
				app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
				return -1;
			}
		}
	}
	else
	{
		app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}

}

static char get_utf8_position(const char* InStr, unsigned char CurSelIndex)
{
	int TempLen = 0 ;
	int TempLen2 = 0;
	signed char SkipLen = 0;
	char FindFlag = 0;
	char Count = 0;

	if((InStr == NULL) || (strlen(InStr)  < CurSelIndex))
	{
		app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	TempLen = strlen(InStr) ;
	if((TempLen == 0) &&(CurSelIndex == 0))
	{
		return 0;
	}
  // find the insert position
       while(TempLen2 < TempLen)
       	{
		SkipLen = calculte_utf8_length(InStr + TempLen2 , TempLen - TempLen2);
		if(SkipLen < 0)
		{
			app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
			return -1;
		}
		if(Count == CurSelIndex)
		{
			FindFlag = 1;
			break;
		}
		TempLen2 += SkipLen;
		Count++;
	}
	if(FindFlag)
		return TempLen2;
	else
	{
		app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}

}

signed char init_utf8_position(const char* InStr,unsigned int *LastCharSel, unsigned int *MaxCharCount)
{
	int TempLen = strlen(InStr) ;
	int TempLen2 = 0;
	signed char SkipLen = 0;
	char Count = 0;
    while(TempLen2 < TempLen)
    {
        SkipLen = calculte_utf8_length(InStr + TempLen2 , TempLen - TempLen2);
        if(SkipLen < 0)
        {
            app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
            return -1;
        }
        TempLen2 += SkipLen;
        Count++;
    }
	if(Count == 0)
		*LastCharSel = 0;
	else
		*LastCharSel = Count;
	*MaxCharCount = Count;
	return (TempLen2 - SkipLen);

}


static char strInsert(char* strRetDate,const char* strUseDate,unsigned char strInser)
{
#define TEMP_BUFFER_LEN 256
	char InsertLen = 0;
	signed char SkipLen = 0;
	int TempLen = strlen(strUseDate) ;
	int TempLen2 = 0;
	char Bufstring[STR_BUF_LEN] = {0};

	if((strRetDate == NULL) || (strUseDate == NULL) || (TempLen == 0) )
	{
		app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}

	InsertLen = calculte_utf8_length(strUseDate, TempLen);
	if((InsertLen < 0) || (InsertLen != TempLen))
	{
		app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	TempLen = strlen(strRetDate);
	if((TempLen == 0) && (strlen(strUseDate) < TEMP_BUFFER_LEN))
	{
		goto TAG1;
	}
	if(strInser == (TEMP_BUFFER_LEN-1))
	{//when strInser = 255 It's mean Insert word to head.
	 //为了解决光标在第一位时插入字符却在第一位后面的问题。
		goto TAG1;
	}
	if((strInser >= TempLen) || ((TempLen + strlen(strUseDate)) > TEMP_BUFFER_LEN))
	{
		app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}

	memset(Bufstring, 0, sizeof(Bufstring));
	TempLen2 = get_utf8_position(strRetDate,strInser);
	if(TempLen2 < 0)
	{
		app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}

	SkipLen = calculte_utf8_length(strRetDate + TempLen2, TempLen - TempLen2);
	if((SkipLen < 0) || (SkipLen > (TempLen - TempLen2)))
	{
		app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	if((TempLen > 0) && (TempLen2 == (TempLen - SkipLen)))
	{
		memcpy(Bufstring,strRetDate,TempLen);
		strcat(Bufstring,strUseDate);
		strlcpy(strRetDate, Bufstring, s_keyboard_full.max_num+1);
	}
	else if((TempLen2 + SkipLen) == 0)
	{
TAG1:
		memcpy(Bufstring,strUseDate,InsertLen);
		strcat(Bufstring,strRetDate);
		strlcpy(strRetDate, Bufstring, s_keyboard_full.max_num+1);
	}
	else
	{
        if((TempLen2 + SkipLen) > 0)
	  	{
		       memcpy(Bufstring,strRetDate,TempLen2+SkipLen);
			strcat(Bufstring,strUseDate);
			memcpy(Bufstring + strlen(Bufstring),strRetDate+TempLen2 + SkipLen,TempLen - (TempLen2 + SkipLen));
			strlcpy(strRetDate, Bufstring, s_keyboard_full.max_num+1);
	  	}
		else
		{
			app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
			return -1;
		}
	}
	return InsertLen;
}

static unsigned int strDel(char* str,char SelToDel)
{
	signed int SkipLen = 0;
	int Temp = 0;
	int TempLen = 0;

	if((str == NULL) || ((TempLen = strlen(str)) < SelToDel))
        	return -1;
	Temp = get_utf8_position((const char*)str, SelToDel);
	if(Temp < 0)
	{
		app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}

	SkipLen = calculte_utf8_length(str + Temp , TempLen - Temp);
	if(SkipLen < 0)
	{
		app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}

	if(TempLen > (Temp + SkipLen))
	{
		int n = Temp;
		while(str[n+SkipLen] != '\0')
		{
			str[n] = str[n+SkipLen];
			n++;
		}
		str[n] = '\0';
	}
	else if(TempLen ==  (Temp + SkipLen))
	{
		str[Temp] = '\0';
	}
	else
	{
		app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}

    	return SkipLen;
}

static int insert_in_different_lang(AppInput *pInput, const char *pLangMap, const char *pEnglishMap)
{
	int Temp = 0;
	int StringLen = 0;
	unsigned int CurPos = 0;

	if((NULL == pInput) || (NULL == pLangMap) || (NULL == pEnglishMap))
	{
		app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	if((pInput->cur_sel) == 0 && (pInput->cur_max > 0))
	{
		CurPos = 255;
	}
	else
	{
		CurPos = (pInput->cur_sel > 0)?(pInput->cur_sel - 1):0;

	}

    StringLen = strlen(s_lang_keyboard_input);
	if(((2 == s_lan_keyboard_caps) && (ITEM_THAI != s_select_language_status && ITEM_PORTUGUESE != s_select_language_status && ITEM_SPANISH != s_select_language_status))
        ||((3 == s_lan_keyboard_caps) && (ITEM_THAI == s_select_language_status)))
    {
        Temp = strlen(pEnglishMap);
    }
    else
    {
        Temp = strlen(pLangMap);
    }
	if((StringLen + Temp) <= pInput->max_len)
	{
		 pInput->cur_max++;
	}
	else
	{
		app_log_error("\nKEYBOARD, reach end\n");
		return  -1;
	}
	Temp = 0;
	if(((2 == s_lan_keyboard_caps) && (ITEM_THAI != s_select_language_status && ITEM_PORTUGUESE != s_select_language_status && ITEM_SPANISH != s_select_language_status))
        ||((3 == s_lan_keyboard_caps) && (ITEM_THAI == s_select_language_status)))
    {

		Temp = strInsert(s_lang_keyboard_input,pEnglishMap,CurPos);
		if(Temp < 0)
		{
			app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
			return -1;
		}
	}
	else
	{

		Temp = strInsert(s_lang_keyboard_input,pLangMap,CurPos);
		if(Temp < 0)
		{
			app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
			return -1;
		}
	}
    pInput->cur_sel ++;
	return 0;
}

/*return value: true: disable, false: enable*/
static bool _check_char_status(int32_t cur_lanuage, uint8_t caps, int8_t button_num)
{
    bool ret = false;

    if(button_num >= KEY_LAM_CLR_NUM || button_num < 0)
        return false;

    if(caps > 2)
    {
        app_log_error("err! keyboard caps = %d, button_num = %d, %s:%d", caps, button_num, __func__, __LINE__);
        return false;
    }
    if(caps == 2)
    {
        if(ITEM_PORTUGUESE == cur_lanuage)
        {
            ret = (('\0' != *s_portuguese_keymap[button_num][caps]) && (0 == strcmp("\x20\x20", s_portuguese_keymap[button_num][caps])));
        }
        else if (ITEM_SPANISH == cur_lanuage)
        {
            ret = (('\0' != *s_spanish_keymap[button_num][caps] )&& (0 == strcmp("\x20\x20", s_spanish_keymap[button_num][caps])));
        }
        else
        {
            if(NULL == s_keyboard_full.usr_data)
            {
                ret = false;
            }
            else
            {
                ret = ('\0' != *s_english_keymap[button_num][caps] && NULL != strstr(s_keyboard_full.usr_data, s_english_keymap[button_num][caps]));
            }
        }
        return ret;
    }

    if(NULL == s_keyboard_full.usr_data){
        return false;
    }

    switch(cur_lanuage)
    {
        case ITEM_ENGLISH:
            ret = ('\0' != *s_english_keymap[button_num][caps] && NULL != strstr(s_keyboard_full.usr_data, s_english_keymap[button_num][caps]));
            break;

        case ITEM_VIETNAMESE:
            ret = ('\0' != *s_vietnamese_keymap[button_num][caps] && NULL != strstr(s_keyboard_full.usr_data, s_vietnamese_keymap[button_num][caps]));
            break;

        case ITEM_ARABIC:
            ret = ('\0' != *s_arabic_keymap[button_num][caps] && NULL != strstr(s_keyboard_full.usr_data, s_arabic_keymap[button_num][caps]));
            break;
        case ITEM_FARSI:
            ret = ('\0' != *s_persian_keymap[button_num][caps] && NULL != strstr(s_keyboard_full.usr_data, s_persian_keymap[button_num][caps]));
            break;

        case ITEM_TURKISH:
            ret = ('\0' != *s_turkish_keymap[button_num][caps] && NULL != strstr(s_keyboard_full.usr_data, s_turkish_keymap[button_num][caps]));
            break;

        case ITEM_RUSSIAN:
            ret = ('\0' != *s_russian_keymap[button_num][caps] && NULL != strstr(s_keyboard_full.usr_data, s_russian_keymap[button_num][caps]));
            break;
        case ITEM_THAI:
            ret = ('\0' != *s_thailand_keymap[button_num][caps] && NULL != strstr(s_keyboard_full.usr_data, s_thailand_keymap[button_num][caps]));
            break;
        case ITEM_SPANISH:
            ret = ('\0' != *s_spanish_keymap[button_num][caps] && NULL != strstr(s_keyboard_full.usr_data, s_spanish_keymap[button_num][caps]));
            break;
        case ITEM_BULGARAIN:
            ret = ('\0' != *s_bulgarain_keymap[button_num][caps] && NULL != strstr(s_keyboard_full.usr_data, s_bulgarain_keymap[button_num][caps]));
            break;
        case ITEM_PORTUGUESE:
            ret = ('\0' != *s_portuguese_keymap[button_num][caps] && NULL != strstr(s_keyboard_full.usr_data, s_portuguese_keymap[button_num][caps]));
            break;
        case ITEM_UKRAINIAN:
            ret = ('\0' != *s_ukrainian_keymap[button_num][caps] && NULL != strstr(s_keyboard_full.usr_data, s_ukrainian_keymap[button_num][caps]));
            break;
		case ITEM_POLSKI:
			ret = ('\0' != *s_polski_keymap[button_num][caps] && NULL != strstr(s_keyboard_full.usr_data, s_polski_keymap[button_num][caps]));
			break;
        default:
            break;
    }

    return ret;
}

static int app_lang_change_language_in_keyboard(int select_mode)
{
	int i=0;

    if(ITEM_ENGLISH == select_mode|| ITEM_THAI == select_mode)
	{
		if(s_lan_keyboard_caps >= 3)
		{
			s_lan_keyboard_caps = 1;
		}
	}
	else if(ITEM_SPANISH == select_mode)
	{
		if(s_lan_keyboard_caps >= 4)
		{
			s_lan_keyboard_caps = 1;
		}
	}
	else if(ITEM_PORTUGUESE == select_mode)
	{
		if(s_lan_keyboard_caps >= 4)
		{
			s_lan_keyboard_caps = 1;
		}
	}
	else
	{
		if(s_lan_keyboard_caps >= 2)
		{
			s_lan_keyboard_caps = 1;
		}
	}

    for(i = 0; i < KEY_COUNT; i++)
    {
        switch(select_mode)
        {
            case ITEM_ARABIC:
                GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", s_arabic_keymap[i][s_lan_keyboard_caps]);
                break;
            case ITEM_FARSI:
                GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", s_persian_keymap[i][s_lan_keyboard_caps]);
                break;
            case ITEM_TURKISH:
                GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", s_turkish_keymap[i][s_lan_keyboard_caps]);
                break;
            case ITEM_VIETNAMESE:
                GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", s_vietnamese_keymap[i][s_lan_keyboard_caps]);
                break;
            case ITEM_RUSSIAN:
                GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", s_russian_keymap[i][s_lan_keyboard_caps]);
                break;
            case ITEM_THAI:
                {
                    if(0 == strcmp(s_thailand_keymap[i][s_lan_keyboard_caps], "\xe0\xb9\x89") ||
                        0 == strcmp(s_thailand_keymap[i][s_lan_keyboard_caps], "\xe0\xb9\x88") ||
                        0 == strcmp(s_thailand_keymap[i][s_lan_keyboard_caps], "\xe0\xb9\x87") ||
                        0 == strcmp(s_thailand_keymap[i][s_lan_keyboard_caps], "\xe0\xb9\x8b") ||
                        0 == strcmp(s_thailand_keymap[i][s_lan_keyboard_caps], "\xe0\xb8\xba") ||
                        0 == strcmp(s_thailand_keymap[i][s_lan_keyboard_caps], "\xe0\xb9\x8c") ||
                        0 == strcmp(s_thailand_keymap[i][s_lan_keyboard_caps], "\xe0\xb9\x8d") ||
                        0 == strcmp(s_thailand_keymap[i][s_lan_keyboard_caps], "\xe0\xb9\x8a") ||
                        0 == strcmp(s_thailand_keymap[i][s_lan_keyboard_caps], "\xe0\xb8\xb1") ||
                        0 == strcmp(s_thailand_keymap[i][s_lan_keyboard_caps], "\xe0\xb8\xb5") ||
                        0 == strcmp(s_thailand_keymap[i][s_lan_keyboard_caps], "\xe0\xb8\xb4") ||
                        0 == strcmp(s_thailand_keymap[i][s_lan_keyboard_caps], "\xe0\xb8\xb7") ||
                        0 == strcmp(s_thailand_keymap[i][s_lan_keyboard_caps], "\xe0\xb8\xb8") ||
                        0 == strcmp(s_thailand_keymap[i][s_lan_keyboard_caps], "\xe0\xb8\xb9") ||
                        0 == strcmp(s_thailand_keymap[i][s_lan_keyboard_caps], "\xe0\xb8\xb1") ||
                        0 == strcmp(s_thailand_keymap[i][s_lan_keyboard_caps], "\xe0\xb8\xb6"))
                    {
                        char str_tmp[10] = {0};
                        sprintf(str_tmp, "%s%s", " ", s_thailand_keymap[i][s_lan_keyboard_caps]);
                        GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", str_tmp);
                    }
                    else
                    {
                        GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", s_thailand_keymap[i][s_lan_keyboard_caps]);
                    }
                }
                break;
            case ITEM_BULGARAIN:
                GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", s_bulgarain_keymap[i][s_lan_keyboard_caps]);
                break;
            case ITEM_PORTUGUESE:
                GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", s_portuguese_keymap[i][s_lan_keyboard_caps]);
                break;
            case ITEM_SPANISH:
                GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", s_spanish_keymap[i][s_lan_keyboard_caps]);
                break;
            case ITEM_UKRAINIAN:
                GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", s_ukrainian_keymap[i][s_lan_keyboard_caps]);
                break;
			case ITEM_POLSKI:
                GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", s_polski_keymap[i][s_lan_keyboard_caps]);
                break;
            default :
                GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", s_english_keymap[i][s_lan_keyboard_caps]);
                break;
        }

        if(true == _check_char_status(select_mode, s_lan_keyboard_caps, i))
            GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "state", "disable");
        else
            GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "state", "enable");

        if( i == KEY_LAN_SPACE_NUM)
            GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", "Space");
    }
    if((ITEM_ARABIC == select_mode)||(ITEM_FARSI == select_mode))
    {
        int arabic_sel = 0;
        GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &arabic_sel);
    }

    return 0;
}

static int app_language_get_move_key_row_num(int key)
{
	int select_key_map_num = 0;
	int select_key_x_num = 0;
	int fouces_button =0;

	switch(key)
	{
		case STBK_UP:
			if(s_lan_keyboard_y == KEY_LAN_Y)
			{
				select_key_map_num = KEY_ROW_COUNT*s_lan_keyboard_y-1;
				if((s_lan_keyboard_x>= KEYBOARD_BUTTON104)&&(s_lan_keyboard_x<= KEYBOARD_BUTTON107))
				{
					s_lan_keyboard_x = 3;
				}
				else if((s_lan_keyboard_x>= 7)&&(s_lan_keyboard_x<= 11))
				{
					select_key_x_num = s_lan_keyboard_x -3 ;
					if(select_key_x_num >= 7)
					{
						select_key_x_num = 7;
					}
					s_lan_keyboard_x = select_key_x_num;
				}
			}
			else if(2 == s_lan_keyboard_y)
			{
				select_key_map_num = KEY_ROW_COUNT*s_lan_keyboard_y;
				if((s_lan_keyboard_x>= KEYBOARD_BUTTON104)&&(s_lan_keyboard_x<= KEYBOARD_BUTTON108))
				{
					select_key_x_num = 3+s_lan_keyboard_x;
					s_lan_keyboard_x = select_key_x_num;
				}
			}
			else if(1 == s_lan_keyboard_y)
			{
				select_key_map_num = KEY_ROW_COUNT*s_lan_keyboard_y;
				if(s_lan_keyboard_x== KEYBOARD_BUTTON111)
				{
					s_lan_keyboard_x = s_lan_keyboard_x+1;
				}
			}
			else
			{
				select_key_map_num = KEY_ROW_COUNT*s_lan_keyboard_y;
			}
			break;
		case STBK_DOWN:
			if(s_lan_keyboard_y == KEY_LAN_Y)
			{
				select_key_map_num = KEY_ROW_COUNT*s_lan_keyboard_y-1;
				if((s_lan_keyboard_x>= KEYBOARD_BUTTON104)&&(s_lan_keyboard_x<= KEYBOARD_BUTTON107))
				{
					s_lan_keyboard_x = 3;
				}
				else if((s_lan_keyboard_x>= 7)&&(s_lan_keyboard_x<= 11))
				{
					select_key_x_num = s_lan_keyboard_x -3 ;
					if(select_key_x_num >= 7)
					{
						select_key_x_num = 7;
					}
					s_lan_keyboard_x = select_key_x_num;
				}
			}
			else if(2 == s_lan_keyboard_y)
			{
				select_key_map_num = KEY_ROW_COUNT*s_lan_keyboard_y;
				if(s_lan_keyboard_x== KEYBOARD_BUTTON112)
				{
					s_lan_keyboard_x = KEYBOARD_BUTTON111;
				}
			}
			else if(0 == s_lan_keyboard_y)
			{
				select_key_map_num = KEY_ROW_COUNT*s_lan_keyboard_y;
				if((s_lan_keyboard_x>= 4)&&(s_lan_keyboard_x<= 7))
				{
					select_key_x_num = s_lan_keyboard_x+3;
					s_lan_keyboard_x = select_key_x_num;
				}
			}
			else
			{
				select_key_map_num = KEY_ROW_COUNT*s_lan_keyboard_y;
			}
			break;
		case STBK_LEFT:
		case STBK_RIGHT:
		case STBK_OK:
			if(s_lan_keyboard_y == KEY_LAN_Y)
			{
				select_key_map_num = KEY_ROW_COUNT*s_lan_keyboard_y - 1;
			}
			else
			{
				select_key_map_num = KEY_ROW_COUNT*s_lan_keyboard_y;
			}
			break;
		default:
			break;
	}

	if(s_lan_keyboard_y == (KEY_LAN_Y+1))
	{
		fouces_button = KEY_LAM_CLR_NUM;
		return fouces_button;
	}

	fouces_button = select_key_map_num+s_lan_keyboard_x;
//app_log_debug("______________________key_x_num=%d.---%d\n",s_lan_keyboard_x,fouces_button);
	if(fouces_button > KEY_COUNT)
	{
		app_log_debug("____key_x_num=%d.---%d\n",select_key_x_num,fouces_button);
		fouces_button = KEY_COUNT -1;
	}
	return fouces_button;
}

static char* app_language_change_fouce_pic(int key)
{
	int row_num = 0;
	static int recall_row_num = KEY_LAN_SPACE_NUM;
	char *img_name = WND_LANG_FOCUS_BMP56X38;
	if (APP_THEME_CLASSIC_DARK || APP_THEME_CLASSIC_PURPLE || APP_THEME_SKY_BLUE || APP_THEME_GRID_PURPLE){
	if(KEY_LAN_DEL_NUM == recall_row_num)
	{
		img_name = WND_LANG_UNFOCUS_BACKSPACE;
		GUI_SetProperty(s_english_keymap[recall_row_num][BUTTON_NUM],"unfocus_img", img_name);
	}
	else if(KEY_LAN_RIGHT_CURSOR == recall_row_num)
	{
		img_name = WND_LANG_UNFOCUS_RIGHT_ARROW;
		GUI_SetProperty(s_english_keymap[recall_row_num][BUTTON_NUM],"unfocus_img", img_name);
	}
	else if(KEY_LAN_LEFT_CURSOR == recall_row_num)
	{
		img_name = WND_LANG_UNFOCUS_LEFT_ARROW;
		GUI_SetProperty(s_english_keymap[recall_row_num][BUTTON_NUM],"unfocus_img", img_name);
	}
	else
	{
		if(KEY_LAN_CAPS_NUM == recall_row_num)
		{
			img_name = "[key_red,key_red,keyboard_button_dis_color]";
		}
		else if(KEY_LAN_NUMBER_NUM == recall_row_num)
		{
			img_name = "[key_green,key_green,keyboard_button_dis_color]";
		}
		else if(KEY_LAN_SAVE_OK_NUM == recall_row_num)
		{
			img_name = "[key_yellow,key_yellow,keyboard_button_dis_color]";
		}
		else if(KEY_LAN_DEL_NUM == recall_row_num)
		{
			img_name = "[key_blue,key_blue,keyboard_button_dis_color]";
		}
		else
		{
			img_name = "[text_color,text_color,keyboard_button_dis_color]";
		}
		if(KEY_LAM_CLR_NUM == recall_row_num)
		{
			GUI_SetProperty("button_key_43", "backcolor", img_name);
			GUI_SetProperty("button_key_43", "forecolor", "[black,black,black]");
		}
		else
		{
			GUI_SetProperty(s_english_keymap[recall_row_num][BUTTON_NUM],"backcolor", img_name);
		}
	}
        if(KEY_LAM_CLR_NUM  != recall_row_num)
        {
            GUI_SetProperty(s_english_keymap[recall_row_num][BUTTON_NUM], "forecolor", "[black,black,black]");
        }
	}
	else{
	if(KEY_LAN_DEL_NUM == recall_row_num)
	{
		img_name = WND_LANG_UNFOCUS_BACKSPACE;
	}
	else if(KEY_LAN_RIGHT_CURSOR == recall_row_num)
	{
        img_name = WND_LANG_UNFOCUS_RIGHT_ARROW;
	}
	else if(KEY_LAN_SPACE_NUM == recall_row_num)
	{
		img_name = WND_LANG_UNFOCUS_BMP254X38;
	}
	else if(KEY_LAN_LEFT_CURSOR == recall_row_num)
	{
        img_name = WND_LANG_UNFOCUS_LEFT_ARROW;
	}
	else if(KEY_LAN_CAPS_NUM == recall_row_num)
	{
		img_name = WND_LANG_UNFOCUS_RED;
	}
	else if(KEY_LAN_NUMBER_NUM == recall_row_num)
	{
		img_name = WND_LANG_UNFOCUS_GREEN;
	}
	else if(KEY_LAN_SAVE_OK_NUM == recall_row_num)
	{
		img_name = WND_LANG_UNFOCUS_YELLOW;
	}
	else if(KEY_LAM_QR_NUM == recall_row_num)
	{
		img_name = WND_LANG_UNFOCUS_CLEAR122;
	}
	else if(KEY_CHANGE_LANGUAGE == recall_row_num)
	{
		img_name = WND_LANG_UNFOCUS_CLEAR122;
	}
	else
	{
		img_name = WND_LANG_UNFOCUS_BMP56X38;
	}

	if(recall_row_num == KEY_LAM_CLR_NUM)
	{
		GUI_SetProperty("button_key_43", "unfocus_img", WND_LANG_UNFOCUS_CLR);
		GUI_SetProperty("button_key_43", "forecolor", "[#000000,#000000,#000000]");
	}
	else
	{
		GUI_SetProperty(s_english_keymap[recall_row_num][BUTTON_NUM],"unfocus_img", img_name);
		GUI_SetProperty(s_english_keymap[recall_row_num][BUTTON_NUM], "forecolor", "[#000000,#000000,#000000]");
	}
	}
	row_num = _get_key_num_by_coordinate(s_lan_keyboard_x, s_lan_keyboard_y);
    if(-1 == row_num)
        row_num = KEY_LAM_CLR_NUM;

	recall_row_num = row_num;

	if (APP_THEME_CLASSIC_DARK || APP_THEME_CLASSIC_PURPLE || APP_THEME_SKY_BLUE || APP_THEME_GRID_PURPLE){
	if(KEY_LAN_DEL_NUM == row_num)
	{
		img_name = WND_LANG_FOCUS_BACKSPACE;
		GUI_SetProperty(s_english_keymap[row_num][BUTTON_NUM],"unfocus_img", img_name);
	}
	else if(KEY_LAN_RIGHT_CURSOR == row_num)
	{
		img_name = WND_LANG_FOCUS_RIGHT_ARROW;
		GUI_SetProperty(s_english_keymap[row_num][BUTTON_NUM],"unfocus_img", img_name);
	}
	else if(KEY_LAN_LEFT_CURSOR == row_num)
	{
		img_name = WND_LANG_FOCUS_LEFT_ARROW;
		GUI_SetProperty(s_english_keymap[row_num][BUTTON_NUM],"unfocus_img", img_name);
	}
	else
	{
		img_name = "[key_focus_color,key_focus_color,keyboard_button_dis_color]";
		if(KEY_LAM_CLR_NUM == row_num)
		{
			GUI_SetProperty("button_key_43", "backcolor", img_name);
			GUI_SetProperty("button_key_43", "forecolor", "[text_color,text_color,black]");
		}
		else
		{
			GUI_SetProperty(s_english_keymap[row_num][BUTTON_NUM],"backcolor", img_name);
			GUI_SetProperty(s_english_keymap[row_num][BUTTON_NUM], "forecolor", "[text_color,text_color,black]");
		}
	}
	}
	else{
	if(KEY_LAN_DEL_NUM == row_num)
	{
		img_name = WND_LANG_FOCUS_BACKSPACE;
	}
	else if(KEY_LAN_RIGHT_CURSOR == row_num)
	{
        img_name = WND_LANG_FOCUS_RIGHT_ARROW;
	}
	else if(KEY_LAN_SPACE_NUM == row_num)
	{
		img_name = WND_LANG_FOCUS_BMP254X38;
	}
	else if((KEY_LAN_LEFT_CURSOR == row_num) /*&&(s_select_language_status != ITEM_ARABIC )*/)
	{
		img_name = WND_LANG_FOCUS_LEFT_ARROW;
	}
	else if(KEY_LAM_QR_NUM == row_num)
	{
		img_name = WND_LANG_FOCUS_CLEAR122;
	}
	else if(KEY_CHANGE_LANGUAGE == row_num)
	{
		img_name = WND_LANG_FOCUS_CLEAR122;//WND_LANG_FOCUS_BLUE_P122;
	}
	else
	{
		img_name = WND_LANG_FOCUS_BMP56X38;
	}

	if(row_num == KEY_LAM_CLR_NUM)
	{
		GUI_SetProperty("button_key_43", "unfocus_img", WND_LANG_FOCUS_CLR);
		GUI_SetProperty("button_key_43", "forecolor", "[#f4f4f4,#f4f4f4,#f4f4f4]");
	}
	else
	{
		GUI_SetProperty(s_english_keymap[row_num][BUTTON_NUM],"unfocus_img", img_name);
		GUI_SetProperty(s_english_keymap[row_num][BUTTON_NUM], "forecolor", "[#f4f4f4,#f4f4f4,#f4f4f4]");
	}
	}
	return img_name;
}

static void app_lang_keyboard_check_focus_for_color_key(int key)
{
    int row_num = 0;

    //当前key是enable, 切换键盘模式后变为disable, 聚焦跳转到对应颜色键上
    row_num = _get_key_num_by_coordinate(s_lan_keyboard_x,s_lan_keyboard_y);
    if(-1 == row_num)
        row_num = KEY_LAM_CLR_NUM;

    if(_check_char_status(s_select_language_status, s_lan_keyboard_caps, row_num))
    {
        if(key == STBK_GREEN)
            s_lan_keyboard_x = 1;
        else if(key == STBK_RED)
            s_lan_keyboard_x = 0;
        else
            app_log_error("\033[34m!!!!!!!!!!!%s, %d, error key!!!!!!!!!!!\033[0m\n", __func__, __LINE__);
        s_lan_keyboard_y = 3;
        app_language_change_fouce_pic(key);
    }

    return;
}

static void  app_lang_keyboard_green_key_func(int sel_num)
{
	int i = 0;
	static uint8_t old_keyboard_caps = 0;
    if(sel_num >= BUTTON_NUM-1)
	{
		sel_num = 0;
	}

	if((s_lan_keyboard_caps != 2)&&(ITEM_THAI != s_select_language_status && ITEM_SPANISH != s_select_language_status && ITEM_PORTUGUESE != s_select_language_status))
	{
		old_keyboard_caps = s_lan_keyboard_caps;
		s_lan_keyboard_caps = 2;
		for(i = 0; i < KEY_COUNT; i++)
		{
			GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", s_english_keymap[i][2]);

            if(NULL != s_keyboard_full.usr_data
                    && '\0' != *s_english_keymap[i][s_lan_keyboard_caps]
                    && NULL != strstr(s_keyboard_full.usr_data, s_english_keymap[i][s_lan_keyboard_caps]))
                GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "state", "disable");
            else
                GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "state", "enable");

			if( i == KEY_LAN_SPACE_NUM)
			{
				GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", "Space");
			}
			if( i == KEY_CHANGE_LANGUAGE)
			{
				GUI_SetProperty(s_english_keymap[KEY_CHANGE_LANGUAGE][BUTTON_NUM], "string", g_KB_LangName[s_select_language_status]);
			}

		}
	}
    else if((s_lan_keyboard_caps != 3) && (ITEM_THAI == s_select_language_status || ITEM_SPANISH == s_select_language_status || ITEM_PORTUGUESE == s_select_language_status))
	{
		old_keyboard_caps = s_lan_keyboard_caps;
		s_lan_keyboard_caps = 3;
		for(i = 0; i < KEY_COUNT; i++)
		{
			GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", s_english_keymap[i][2]);
			if( i == KEY_LAN_SPACE_NUM)
			{
				GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "string", "Space");
			}
			if( i == KEY_CHANGE_LANGUAGE)
			{
				GUI_SetProperty(s_english_keymap[KEY_CHANGE_LANGUAGE][BUTTON_NUM], "string", g_KB_LangName[s_select_language_status]);
			}

		}

	}
	else
	{
		s_lan_keyboard_caps = old_keyboard_caps;
		app_lang_change_language_in_keyboard(s_select_language_status);
	}
    if(ITEM_ARABIC == s_select_language_status)
	{
		GUI_SetProperty(s_english_keymap[KEY_LAN_CAPS_NUM][BUTTON_NUM], "string", s_arabic_keymap[KEY_LAN_CAPS_NUM][old_keyboard_caps]);
    }
    else if(ITEM_FARSI == s_select_language_status)
    {
        GUI_SetProperty(s_english_keymap[KEY_LAN_CAPS_NUM][BUTTON_NUM], "string", s_persian_keymap[KEY_LAN_CAPS_NUM][old_keyboard_caps]);
	}
    else if(s_select_language_status == ITEM_SPANISH)
	{
		GUI_SetProperty(s_english_keymap[KEY_LAN_CAPS_NUM][BUTTON_NUM], "string", s_spanish_keymap[KEY_LAN_CAPS_NUM][old_keyboard_caps]);
	}
	else if(s_select_language_status == ITEM_PORTUGUESE)
	{
		GUI_SetProperty(s_english_keymap[KEY_LAN_CAPS_NUM][BUTTON_NUM], "string", s_portuguese_keymap[KEY_LAN_CAPS_NUM][old_keyboard_caps]);
	}
	else if(s_select_language_status == ITEM_THAI)
    {
        GUI_SetProperty(s_english_keymap[KEY_LAN_CAPS_NUM][BUTTON_NUM], "string", s_thailand_keymap[KEY_LAN_CAPS_NUM][old_keyboard_caps]);
    }
	else
	{
		GUI_SetProperty(s_english_keymap[KEY_LAN_CAPS_NUM][BUTTON_NUM], "string", s_english_keymap[KEY_LAN_CAPS_NUM][old_keyboard_caps]);
	}
    app_lang_keyboard_check_focus_for_color_key(STBK_GREEN);

	return;
}

static void app_lang_keyboard_yellow_key_func(void)
{
    if(s_save_cb != NULL)
    {
        s_save_cb();
        return;
    }
	uint8_t len = strlen(s_lang_keyboard_input);
	bool blank = true;
	uint8_t i;

	if (len == 0)
	{
		PopDlg pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_OK;
		pop.mode = POP_MODE_UNBLOCK;
        pop.str = STR_ID_INPUT_EMPTY_TIP;
        popdlg_create(&pop);
		return;
	}

	for(i = 0; i < len ; i++)
	{
		if(s_lang_keyboard_input[i] != ' ')
		{
			blank = false;
			break;
		}
	}

	if (blank == true)
	{
		PopDlg pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_OK;
		pop.mode = POP_MODE_UNBLOCK;
		pop.str = STR_ID_INPUT_ONLY_SPACE_TIP;
		popdlg_create(&pop);
		return;
	}

	GUI_EndDialog(WND_KEYBOARD_LANGUAGE);
	GUI_SetInterface("flush",NULL);
	if (s_keyboard_full.release_cb != NULL)
		s_keyboard_full.release_cb(&s_keyboard_full);

	return;
}

static void app_lang_keyboard_blue_key_func(void)
{
	AppInput *input = &g_AppInput;
	unsigned int Temp = 0;
	unsigned int CurPos = 0;
	uint8_t str_len = 0;

	str_len = strlen(s_lang_keyboard_input);
	if((str_len == 0)   || (input->cur_sel > str_len))
	{
		app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return;
	}
	CurPos = (input->cur_sel > 0)?(input->cur_sel -1):0;
	Temp = strDel(s_lang_keyboard_input,CurPos);
	if((Temp < 0) || ( str_len  < Temp))
	{
		app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return ;
	}
	str_len = str_len - Temp;
	if(input->cur_sel > 0)
	{
		input->cur_sel--;
	}
	if(input->cur_max > 0)
	{
		input->cur_max--;
	}

	if((strlen(s_lang_keyboard_input) == 0)||(str_len == 0))
	{
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", NULL);
		memset(s_lang_keyboard_input, 0, input->max_len);
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "clear", NULL);
	}
	else
	{
		int arabic_sel = 0;
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", s_lang_keyboard_input);
        if((ITEM_ARABIC == s_select_language_status)||(ITEM_FARSI == s_select_language_status))
		{
			GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &arabic_sel);
		}
		else
		{
			GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &(input->cur_sel));
		}
	}
}

static int32_t _keyboard_pop_list_cb(int32_t ret_sel, unsigned short key)
{
	GUI_SetProperty(s_english_keymap[KEY_CHANGE_LANGUAGE][BUTTON_NUM], "string", g_KB_LangName[ret_sel]);

	app_language_change_fouce_pic(STBK_RIGHT);
	if(ret_sel != s_select_language_status)
	{
        if((ret_sel == ITEM_ARABIC || ret_sel == ITEM_FARSI) ||(ret_sel == ITEM_THAI))
        {
            s_lan_keyboard_caps = 0;
        }
        else
        {
            s_lan_keyboard_caps = 1;
        }
        old_lan_keyboard_caps = s_lan_keyboard_caps;
        thiz_pos_num = s_lan_keyboard_caps;
		app_lang_change_language_in_keyboard(ret_sel);
	}
	s_select_language_status = ret_sel;
    return 0;
}

static int app_language_keyboard_pop(int sel)
{
	PopList pop_list;

	memset(&pop_list, 0, sizeof(PopList));
    pop_list.mode = POP_MODE_UNBLOCK;
	pop_list.title = STR_ID_LANG;
	pop_list.item_num = KB_LANG_MAX;
	pop_list.item_content = g_KB_LangName;
	pop_list.sel = sel;
	pop_list.show_num = false;
    pop_list.exit_cb = _keyboard_pop_list_cb;

	poplist_create(&pop_list);
	return 0;
}

static int8_t _get_key_num_by_coordinate(int8_t x, int8_t y)
{
    if(x < 0 || y < 0 || x > KEY_LAN_X || y > KEY_LAN_Y)
        return -1;

    if(y < 3)
        return (y * (KEY_LAN_X + 1) + x);
    else if(3 == y)
        return (y * (KEY_LAN_X + 1) - 1 + x);

    return -1;
}

static void _language_keyboard_move_by_up_keypress(void)
{
    int8_t button_num = -1;

    do
    {
        if(0 == s_lan_keyboard_y && KEY_LAN_X == s_lan_keyboard_x)
            s_lan_keyboard_y = KEY_LAN_Y + 1;
        else if(0 == s_lan_keyboard_y || (KEY_LAN_Y + 1) == s_lan_keyboard_y)
            s_lan_keyboard_y = KEY_LAN_Y;
        else
            s_lan_keyboard_y--;

        button_num = app_language_get_move_key_row_num(STBK_UP);
    }while(true == _check_char_status(s_select_language_status, s_lan_keyboard_caps, button_num));

    return;
}

static void _language_keyboard_move_by_down_keypress(void)
{
    int8_t button_num = -1;

    do
    {
        if(KEY_LAN_Y == s_lan_keyboard_y || (KEY_LAN_Y + 1) == s_lan_keyboard_y)
            s_lan_keyboard_y = 0;
        else
            s_lan_keyboard_y++;

        button_num = app_language_get_move_key_row_num(STBK_DOWN);
    }while(true == _check_char_status(s_select_language_status, s_lan_keyboard_caps, button_num));

    return;
}

static void _language_keyboard_move_by_left_keypress(void)
{
    int8_t button_num = -1;

    if(KEY_LAN_Y + 1 == s_lan_keyboard_y)
        return;

    do
    {
        if(0 == s_lan_keyboard_x)
        {
            if(2 == s_lan_keyboard_y)
                s_lan_keyboard_x = 10;
            else if(3 == s_lan_keyboard_y)
                s_lan_keyboard_x = 7;
            else
                s_lan_keyboard_x = KEY_LAN_X;
        }
        else
        {
            s_lan_keyboard_x--;
        }

        button_num = app_language_get_move_key_row_num(STBK_LEFT);
    }while(true == _check_char_status(s_select_language_status, s_lan_keyboard_caps, button_num));

    return;
}

static void _language_keyboard_move_by_right_keypress(void)
{
    int8_t button_num = -1;

    if(KEY_LAN_Y + 1 == s_lan_keyboard_y)
        return;

    do
    {
        if((s_lan_keyboard_x == KEY_LAN_X)
                || ((s_lan_keyboard_x == 10) && (s_lan_keyboard_y == 2))
                || ((s_lan_keyboard_x == 7) && (s_lan_keyboard_y == 3)))
            s_lan_keyboard_x = 0;
        else
            s_lan_keyboard_x++;

        button_num = app_language_get_move_key_row_num(STBK_RIGHT);
    }while(true == _check_char_status(s_select_language_status, s_lan_keyboard_caps, button_num));

    return;
}

static void app_language_keyboard_move(uint16_t key)
{
	switch(key)
	{
		case STBK_UP:
            _language_keyboard_move_by_up_keypress();
			break;
		case STBK_DOWN:
            _language_keyboard_move_by_down_keypress();
			break;
		case STBK_LEFT:
            _language_keyboard_move_by_left_keypress();
			break;
		case STBK_RIGHT:
            _language_keyboard_move_by_right_keypress();
			break;
		default:
			break;
	}

	app_language_change_fouce_pic(key);
	return;
}

static void app_language_keyboard_ok(int key)
{
    AppInput *input = &g_AppInput;
	uint8_t key_num = 0;
	uint8_t max_len = 0;
    int cur_sel = 0;
	int length  = 0;
	char input_temp[STR_BUF_LEN + 1] = {0};
	key_num =  app_language_get_move_key_row_num(key);
	if((key_num == KEY_LAM_CLR_NUM)
#if WEBLET_SUPPORT == 0
	|| (key_num == KEY_LAM_QR_NUM)
#endif
	)
	{
		input->cur_sel = 0;
		input->cur_max = 0;
		memset(s_lang_keyboard_input, 0, strlen(s_lang_keyboard_input));
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", NULL);
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "clear", NULL);
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &(input->cur_sel));
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", s_lang_keyboard_input);
	}
#if WEBLET_SUPPORT
	else if (key_num == KEY_LAM_QR_NUM)
	{
		s_kb_mode = 1;
		app_language_keyboard_switch_input_mode(s_kb_mode);
	}
#endif
	else if(key_num == KEY_LAN_DEL_NUM)
	{
		app_lang_keyboard_blue_key_func();
	}
	else if(key_num == KEY_LAN_CAPS_NUM)
	{
        if ((ITEM_SPANISH == s_select_language_status)
            || (ITEM_PORTUGUESE == s_select_language_status)
            || (ITEM_THAI == s_select_language_status))
		{
			s_lan_keyboard_caps = (old_lan_keyboard_caps+1)%3;
		}
		else
		{
            s_lan_keyboard_caps = (old_lan_keyboard_caps+1)%2;
        }
		old_lan_keyboard_caps = s_lan_keyboard_caps;
		thiz_pos_num = s_lan_keyboard_caps;
		app_lang_change_language_in_keyboard(s_select_language_status);
	}
	else if(key_num == KEY_LAN_NUMBER_NUM)
	{
		app_lang_keyboard_green_key_func(thiz_pos_num);
	}
	else if(key_num == KEY_LAN_SAVE_OK_NUM)
	{
		app_lang_keyboard_yellow_key_func();
	}
	else if(KEY_LAN_LEFT_CURSOR == key_num)
	{
		if((gxgdi_is_arabic((unsigned char*)s_lang_keyboard_input)|| (0 == strlen(s_lang_keyboard_input)))
            ||(s_select_language_status == ITEM_ARABIC)||(s_select_language_status == ITEM_FARSI))
		{
			return;
		}
		if (input->cur_sel != 0)
		{
			input->cur_sel--;
			input->first_click = 0;
		}
        else
        {
			input->cur_sel = input->cur_max;
			input->first_click = 0;
        }
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", s_lang_keyboard_input);
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &(input->cur_sel));
	}
	else if(KEY_LAN_RIGHT_CURSOR == key_num)
	{
		if((gxgdi_is_arabic((unsigned char*)s_lang_keyboard_input)|| (0 == strlen(s_lang_keyboard_input)))
            ||(s_select_language_status == ITEM_ARABIC) ||(s_select_language_status == ITEM_FARSI))
		{
			return;
		}
		if(input->cur_sel <= input->cur_max - 1)
		{
			input->cur_sel++;
			input->first_click = 0;
		}
        else
        {
            input->cur_sel=0;
            input->first_click = 0;
        }
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", s_lang_keyboard_input);
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &(input->cur_sel));
	}
	else if(KEY_CHANGE_LANGUAGE == key_num)
	{
		app_language_keyboard_pop(s_select_language_status);
	}
	else
	{
		max_len = s_keyboard_full.max_num;
		if(max_len > STR_BUF_LEN)
		{
			max_len = STR_BUF_LEN-1;
		}

		if(strlen(s_lang_keyboard_input) < max_len)
		{
            if(s_select_language_status == ITEM_ARABIC)
			{
				insert_in_different_lang(input, s_arabic_keymap[key_num][s_lan_keyboard_caps], s_english_keymap[key_num][s_lan_keyboard_caps]);
            }
            else if(s_select_language_status == ITEM_FARSI)
            {
                insert_in_different_lang(input, s_persian_keymap[key_num][s_lan_keyboard_caps], s_english_keymap[key_num][s_lan_keyboard_caps]);
			}
			else if(ITEM_VIETNAMESE == s_select_language_status)
			{
				insert_in_different_lang(input, s_vietnamese_keymap[key_num][s_lan_keyboard_caps], s_english_keymap[key_num][s_lan_keyboard_caps]);
			}
			else if(ITEM_RUSSIAN== s_select_language_status)
			{
				insert_in_different_lang(input, s_russian_keymap[key_num][s_lan_keyboard_caps], s_english_keymap[key_num][s_lan_keyboard_caps]);
			}
			else if(ITEM_TURKISH== s_select_language_status)
			{
				insert_in_different_lang(input, s_turkish_keymap[key_num][s_lan_keyboard_caps], s_english_keymap[key_num][s_lan_keyboard_caps]);
			}
            else if(ITEM_BULGARAIN == s_select_language_status)
            {
                insert_in_different_lang(input, s_bulgarain_keymap[key_num][s_lan_keyboard_caps], s_english_keymap[key_num][s_lan_keyboard_caps]);
            }
            else if(ITEM_PORTUGUESE == s_select_language_status)
            {
                insert_in_different_lang(input, s_portuguese_keymap[key_num][s_lan_keyboard_caps], s_english_keymap[key_num][s_lan_keyboard_caps]);
            }
            else if(ITEM_SPANISH == s_select_language_status)
            {
                insert_in_different_lang(input, s_spanish_keymap[key_num][s_lan_keyboard_caps], s_english_keymap[key_num][s_lan_keyboard_caps]);
            }
            else if(ITEM_UKRAINIAN == s_select_language_status)
            {
                insert_in_different_lang(input, s_ukrainian_keymap[key_num][s_lan_keyboard_caps], s_english_keymap[key_num][s_lan_keyboard_caps]);
            }
            else if(ITEM_THAI == s_select_language_status)
            {
                insert_in_different_lang(input, s_thailand_keymap[key_num][s_lan_keyboard_caps], s_english_keymap[key_num][s_lan_keyboard_caps]);
            }
			else if(ITEM_POLSKI == s_select_language_status)
            {
                insert_in_different_lang(input, s_polski_keymap[key_num][s_lan_keyboard_caps], s_english_keymap[key_num][s_lan_keyboard_caps]);
            }
            else
            {
                insert_in_different_lang(input, s_english_keymap[key_num][s_lan_keyboard_caps], s_english_keymap[key_num][s_lan_keyboard_caps]);
            }
            app_log_info("~~~~~~[%s]%d, s lang keyboard input is %s \n", __func__, __LINE__,s_lang_keyboard_input);
            if(ITEM_THAI == s_select_language_status )
            {
                length = strlen(s_lang_keyboard_input);
                memset(input_temp, 0, (STR_BUF_LEN + 1));
                memcpy(input_temp,s_lang_keyboard_input,length);
                if (((s_lang_keyboard_input[length - 1] == 0xb1)&&(s_lang_keyboard_input[length - 2] == 0xb8)&&(s_lang_keyboard_input[length - 3] == 0xe0))
                    || ((s_lang_keyboard_input[length - 1] == 0xb5)&&(s_lang_keyboard_input[length - 2] == 0xb8)&&(s_lang_keyboard_input[length - 3] == 0xe0))
                    || ((s_lang_keyboard_input[length - 1] == 0x88)&&(s_lang_keyboard_input[length - 2] == 0xb9)&&(s_lang_keyboard_input[length - 3] == 0xe0))
                    || ((s_lang_keyboard_input[length - 1] == 0x89)&&(s_lang_keyboard_input[length - 2] == 0xb9)&&(s_lang_keyboard_input[length - 3] == 0xe0))
                    || ((s_lang_keyboard_input[length - 1] == 0xb4)&&(s_lang_keyboard_input[length - 2] == 0xb8)&&(s_lang_keyboard_input[length - 3] == 0xe0))
                    || ((s_lang_keyboard_input[length - 1] == 0xb7)&&(s_lang_keyboard_input[length - 2] == 0xb8)&&(s_lang_keyboard_input[length - 3] == 0xe0))
                    || ((s_lang_keyboard_input[length - 1] == 0xb8)&&(s_lang_keyboard_input[length - 2] == 0xb8)&&(s_lang_keyboard_input[length - 3] == 0xe0))
                    || ((s_lang_keyboard_input[length - 1] == 0xb9)&&(s_lang_keyboard_input[length - 2] == 0xb8)&&(s_lang_keyboard_input[length - 3] == 0xe0))
                    || ((s_lang_keyboard_input[length - 1] == 0xb6)&&(s_lang_keyboard_input[length - 2] == 0xb8)&&(s_lang_keyboard_input[length - 3] == 0xe0))
                    || ((s_lang_keyboard_input[length - 1] == 0x8a)&&(s_lang_keyboard_input[length - 2] == 0xb9)&&(s_lang_keyboard_input[length - 3] == 0xe0))
                    || ((s_lang_keyboard_input[length - 1] == 0x8b)&&(s_lang_keyboard_input[length - 2] == 0xb9)&&(s_lang_keyboard_input[length - 3] == 0xe0))
                    || ((s_lang_keyboard_input[length - 1] == 0x87)&&(s_lang_keyboard_input[length - 2] == 0xb9)&&(s_lang_keyboard_input[length - 3] == 0xe0))
                    || ((s_lang_keyboard_input[length - 1] == 0x8c)&&(s_lang_keyboard_input[length - 2] == 0xb9)&&(s_lang_keyboard_input[length - 3] == 0xe0))
                    )
                {
                       is_Superscript_subscript = true;
                }
                else
                {
                      is_Superscript_subscript = false;
                }

                cur_sel = input->cur_sel;

                if ((length + 2) < STR_BUF_LEN)
                {
                    input_temp[length] = 0x20;
                    input_temp[length+1] = 0x20;
                    input_temp[length+2] = 0x20;
                    cur_sel += 3;
                }
                GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string",input_temp);
            }
            else
            {
                GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", s_lang_keyboard_input);
            }
            GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", s_lang_keyboard_input);
            if((s_select_language_status == ITEM_ARABIC) ||(s_select_language_status == ITEM_FARSI))
			{
				int32_t arabic_sel = 0; /*arabic write from right to left*/
				GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &arabic_sel);
			}
			else if(ITEM_THAI == s_select_language_status )
			{
			       GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &(cur_sel));
			}
			else
			{
				GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &(input->cur_sel));
			}
		}
	}

}

SIGNAL_HANDLER int app_language_keyboard_create(const char* widgetname, void *usrdata)
{
	AppInput *input = &g_AppInput;
	signed char ret_value = 0;

	if(s_select_language_status == ITEM_ENGLISH)
	{
		s_lan_keyboard_caps = 1;
		old_lan_keyboard_caps = 1;
	}
	else
	{
		s_lan_keyboard_caps = 0;
		old_lan_keyboard_caps = 0;
	}

	app_lang_change_language_in_keyboard(s_select_language_status);

	s_lan_keyboard_x = 0;
	s_lan_keyboard_y = 0;
	app_language_change_fouce_pic(STBK_RIGHT);
	GUI_SetFocusWidget(EDIT_LANG_KEYBOARD_INPUT);

	GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", input->name_get());
	if((s_keyboard_full.in_name == NULL) || (strlen(s_keyboard_full.in_name) == 0))
	{
		input->cur_sel = 0;
		input->cur_max = 0;
		GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", NULL);
	}
	else
	{
		ret_value = init_utf8_position(s_lang_keyboard_input,&(input->cur_sel),&(input->cur_max));
		if(-1 == ret_value)
		{
			input->cur_sel = 0;
			input->cur_max = 0;
			memset(s_lang_keyboard_input, 0, input->max_len);
			GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", NULL);
			GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "clear", NULL);
			GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", s_lang_keyboard_input);
		}
	}
	GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &(input->cur_sel));
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_language_keyboard_destroy(const char* widgetname, void *usrdata)
{
    if(GUI_CheckDialog(WND_POP_OPTION_LIST) == GXCORE_SUCCESS)
    {
        app_pop_list_end_dialog();//直接用EndDialog会造成GUI Msg收不消息
    }
    thiz_pos_num = 0;
    s_save_cb = NULL;
#if WEBLET_SUPPORT && QR_POP_SUPPORT
	app_qrencode_destroy();
#endif
	return EVENT_TRANSFER_STOP;
}

static void inputnum_by_remoter(const char *inputnum)
{
       AppInput *input = &g_AppInput;

	unsigned int Len = 0;
	unsigned int LenOri = 0;
	char Ret = 0;
	unsigned int CurPos = 0;
	uint8_t max_len = 0;

	if((inputnum ==NULL) ||(input == NULL))
	{
		app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return;
	}
	Len = strlen(inputnum);
	LenOri = strlen(s_lang_keyboard_input);
	if((Len != 1) ||(LenOri + Len) > input->max_len)
	{
		app_log_error("\nKEYBOARD, parameter error, %s,%d\n",__FUNCTION__,__LINE__);
		return;
	}

	max_len = s_keyboard_full.max_num;
	if(max_len > STR_BUF_LEN)
	{
		max_len = STR_BUF_LEN - 1;
	}
	if( strlen(s_lang_keyboard_input) < max_len )
	{
		if((input->cur_sel) == 0 && (input->cur_max > 0))
		{
			CurPos = 255;
		}
		else
		{
		CurPos = (input->cur_sel > 0)?(input->cur_sel - 1):0;
		}
		Ret = strInsert(s_lang_keyboard_input,inputnum,CurPos);
		if(Ret == Len)
		{
			input->cur_sel++;
			input->cur_max++;
		}
       	GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", s_lang_keyboard_input);
        if((s_select_language_status == ITEM_ARABIC) ||(s_select_language_status == ITEM_FARSI))
		{
				int32_t arabic_sel = 0; /*arabic write from right to left*/
				GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &arabic_sel);
		}
        else
        {
            GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &(input->cur_sel));
        }
	}
}
SIGNAL_HANDLER int app_language_keyboard_keypress(const char* widgetname, void *usrdata)
{
	AppInput *input = &g_AppInput;
	GUI_Event *event = NULL;
	char num[2] = {'\0','\0'};
	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type && s_kb_mode == 0)
	{
        switch(event->key.sym)
        {
			case STBK_UP:
			case STBK_DOWN:
			case STBK_LEFT:
			case STBK_RIGHT:
				app_language_keyboard_move(event->key.sym);
				break;

        }

		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
            case VK_BOOK_TRIGGER:
                app_multi_lang_keyborad_menu_destroy();
                GUI_EndDialog("after wnd_full_screen");
                break;
			case STBK_OK:
				app_language_keyboard_ok(event->key.sym);
#if WEBLET_SUPPORT
				app_kb_exec_edit_key();
#endif
				break;
			case STBK_EXIT:
			case STBK_MENU:
                app_multi_lang_keyborad_menu_destroy();
                break;
			case STBK_F1:
				memset(s_lang_keyboard_input, 0, input->max_len);
				GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", NULL);
				GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "clear", NULL);
				GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", s_lang_keyboard_input);

				input->cur_sel = 0;
				input->cur_max = 0;
				if(s_keyboard_full.change_cb != NULL)
				{
					s_keyboard_full.change_cb(&s_keyboard_full);
				}
				break;
			case STBK_BLUE:
				app_lang_keyboard_blue_key_func();
#if WEBLET_SUPPORT
				app_kb_exec_edit_key();
#endif
				break;
			case STBK_RED:
                if((ITEM_SPANISH == s_select_language_status)
                    || (ITEM_PORTUGUESE == s_select_language_status)
                    || (ITEM_THAI == s_select_language_status))
                {
                    s_lan_keyboard_caps = (old_lan_keyboard_caps+1)%3;
                }
                else
                {
                    s_lan_keyboard_caps = (old_lan_keyboard_caps+1)%2;
                }
                old_lan_keyboard_caps = s_lan_keyboard_caps;
                thiz_pos_num = s_lan_keyboard_caps;
                app_lang_change_language_in_keyboard(s_select_language_status);
                app_lang_keyboard_check_focus_for_color_key(STBK_RED);
				break;
			case STBK_GREEN:
				app_lang_keyboard_green_key_func(thiz_pos_num);//caps_num
				break;
			case STBK_YELLOW:
                app_lang_keyboard_yellow_key_func();
				break;
			case STBK_0:
				memset(num,'0',1);
				inputnum_by_remoter(num);
				break;
			case STBK_1:
				memset(num,'1',1);
				inputnum_by_remoter(num);
				break;
			case STBK_2:
				memset(num,'2',1);
				inputnum_by_remoter(num);
				break;
			case STBK_3:
				memset(num,'3',1);
				inputnum_by_remoter(num);
				break;
			case STBK_4:
				memset(num,'4',1);
				inputnum_by_remoter(num);
				break;
			case STBK_5:
				memset(num,'5',1);
				inputnum_by_remoter(num);
				break;
			case STBK_6:
				memset(num,'6',1);
				inputnum_by_remoter(num);
				break;
			case STBK_7:
				memset(num,'7',1);
				inputnum_by_remoter(num);
				break;
			case STBK_8:
				memset(num,'8',1);
				inputnum_by_remoter(num);
				break;
			case STBK_9:
				memset(num,'9',1);
				inputnum_by_remoter(num);
				break;
			default:
				break;
		}
	}
	else if(GUI_KEYDOWN ==  event->type && s_kb_mode == 1)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
            case VK_BOOK_TRIGGER:
                app_multi_lang_keyborad_menu_destroy();
                GUI_EndDialog("after wnd_full_screen");
                break;
			case STBK_OK:
				s_kb_mode = 0;
#if WEBLET_SUPPORT
				app_language_keyboard_switch_input_mode(s_kb_mode);
#endif
				break;

			case STBK_EXIT:
			case STBK_MENU:
                app_multi_lang_keyborad_menu_destroy();
                break;
			default:
				break;
		}
	}
	else
	{;}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_lang_edit_keyboard_keypress(GuiWidget *widget, void *usrdata)
{
    int key_num = 0;
    GUI_Event *event = (GUI_Event *)usrdata;
    int key = find_virtualkey_ex(event->key.scancode,event->key.sym);

    GUI_SendEvent(WND_KEYBOARD_LANGUAGE, event);
    if (s_keyboard_full.change_cb != NULL)
    {
        if(((key>=STBK_0) && (key<=STBK_9))
                || (key==STBK_OK)
                || (key==STBK_BLUE))
        {
            key_num = app_language_get_move_key_row_num(STBK_OK);
            if((key == STBK_OK)
                && ((key_num == KEY_CHANGE_LANGUAGE)
                    || (key_num == KEY_LAN_CAPS_NUM)
                    || (key_num == KEY_LAN_NUMBER_NUM)
                    || (key_num == KEY_LAN_LEFT_CURSOR)
                    || (key_num == KEY_LAN_RIGHT_CURSOR)
                    || (key_num == KEY_LAN_SAVE_OK_NUM)))
            {
                return EVENT_TRANSFER_STOP;
            }
            s_keyboard_full.change_cb(&s_keyboard_full);
        }
    }
    //keyboard EVENT_TRANSFER_STOP,so the UI can't receive the change msg.
    //add change callback here, just response the num key and Del key.
    return EVENT_TRANSFER_STOP;
}

status_t multi_language_keyboard_create(PopKeyboard *keyboard)
{
    int32_t pos_x = 0;
    int32_t pos_y = 0;
    int32_t input_len = 0;

    if(keyboard->max_num <= 0)
    {
        keyboard->max_num =  STR_BUF_LEN-1;
    }

    if(GXCORE_ERROR ==  keyboard_update_input(keyboard->max_num))
    {
        return GXCORE_ERROR;
    }
    memset(s_lang_keyboard_input, 0, keyboard->max_num+1);
    if (GXCORE_ERROR == g_AppInput.init(keyboard->in_name, keyboard->max_num))
    {
        return GXCORE_ERROR;
    }
    if(keyboard->in_name != NULL)
    {
        input_len = strlen(keyboard->in_name);
        if(input_len > keyboard->max_num){
            input_len = keyboard->max_num;
        }
        memcpy(s_lang_keyboard_input, keyboard->in_name, input_len);
    }
    s_keyboard_full.in_ret = POP_VAL_OK;
    memcpy(&s_keyboard_full, keyboard, sizeof(PopKeyboard));
    s_keyboard_full.out_name = s_lang_keyboard_input;

    s_select_language_status = keyboard->lang_sel;

    GUI_CreateDialog(WND_KEYBOARD_LANGUAGE);

    if((keyboard->pos.x == APP_WND_KB_IN_POS_X)&&(keyboard->pos.y == APP_WND_KB_IN_POS_Y))
    {
        pos_x = APP_WND_KB_SET_POS_X;
        pos_y = APP_WND_KB_IN_POS_Y;
        s_keyboard_full.pos.x = APP_WND_KB_IN_POS_X;
        GUI_SetProperty(WND_KEYBOARD_LANGUAGE, "move_window_x", &pos_x);
        GUI_SetProperty(WND_KEYBOARD_LANGUAGE, "move_window_y", &pos_y);
    }
    else
    {
        pos_x = keyboard->pos.x;
        pos_y = keyboard->pos.y;
        s_keyboard_full.pos.x = keyboard->pos.x;
    }

    if(keyboard->usr_data != NULL)
    {
        GUI_SetProperty("txt_keyboard_name", "string", keyboard->usr_data);
    }
	s_kb_mode = 0;
	app_language_keyboard_switch_input_mode(s_kb_mode);
    return GXCORE_SUCCESS;
}

void app_multi_lang_keyborad_menu_destroy(void)
{
    GUI_EndDialog(WND_KEYBOARD_LANGUAGE);
    GUI_SetProperty(WND_KEYBOARD_LANGUAGE, "draw_now", NULL);
    s_keyboard_full.in_ret = POP_VAL_CANCEL;
    if(s_keyboard_full.release_cb != NULL)
        s_keyboard_full.release_cb(&s_keyboard_full);
    s_keyboard_full.out_name = NULL;
    GxCore_Free(s_lang_keyboard_input);
    s_lang_keyboard_input = NULL;
}

#if WEBLET_SUPPORT && QR_POP_SUPPORT
static unsigned int kb_qr_handle = 0;
static int app_kb_qrencode_create(char* ipaddr)
{
    if(isfirstin == 0)
    {
        memset(url, 0, sizeof(url));
        sprintf(url, "http://%s/keyboard.html", ipaddr);
        isfirstin = 1;
    }
	/* don't show here, create one timer instead */
    kb_qr_handle = qren_encode_init(url, 16);
    if(kb_qr_handle>0)
    {
        qren_encode_show(kb_qr_handle, KB_QRENCODE);
    }

    return 0;
}

static int app_qrencode_destroy(void)
{
    if(kb_qr_handle>0)
    {
        qren_encode_close(kb_qr_handle);
        kb_qr_handle = 0;
    }
    isfirstin = 0;
    memset(url, 0 , sizeof(url));
    return 0;
}
#endif
static void app_language_keyboard_switch_input_mode(int mode)
{
	int i = 0;
	if(mode == 0)//full keyboard
	{
		//hide qrencode
		GUI_SetProperty(KB_QRENCODE, "state", "hide");
		GUI_SetProperty(KB_BTN_CLR, "unfocus_img", WND_LANG_UNFOCUS_CLR);
		GUI_SetProperty(WND_LANG_BACK_CLR, "state", "show");
		//show full keyboard
		for(i = 0; i < KEY_COUNT; i++)
		{
			GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "state", "show");
		}
	}
	else //qrencode
	{
#if WEBLET_SUPPORT && QR_POP_SUPPORT
        //hide full keyboard
        char ipaddr[64] = {0};
        memset(ipaddr, 0, sizeof(ipaddr));
        if(app_qr_ip_addr_get(ipaddr) < 0)
        {
            s_kb_mode = 0;
            return;
        }
        for(i = 0; i < KEY_COUNT; i++)
        {
            GUI_SetProperty(s_english_keymap[i][BUTTON_NUM], "state", "hide");
        }
        //show qrencode
        GUI_SetProperty(KB_QRENCODE, "state", "show");
        GUI_SetProperty(KB_BTN_CLR, "unfocus_img", KB_IMG_KEYBOARD);
        GUI_SetProperty(WND_LANG_BACK_CLR, "state", "show");
        app_kb_qrencode_create(ipaddr);
#endif
	}
}

void app_update_keyboard_dlg(void)
{
    if(GUI_CheckDialog(WND_KEYBOARD_LANGUAGE) == GXCORE_SUCCESS)
    {
         GUI_SetProperty(WND_KEYBOARD_LANGUAGE, "update", NULL);
         app_update_pop_dlg(WND_POP_OPTION_LIST);
    }
}

#if WEBLET_SUPPORT
int app_kl_get_edit_content(char *input)
{
    //input 长度由app_weblet_server_kb_update和app_weblet_server_kb_edit_sync两个函数中定义，为128bytes
	if(input == NULL) return -1;
	if(0 != strcasecmp(WND_KEYBOARD_LANGUAGE, GUI_GetFocusWindow()))
	{
		return -1;
	}
	strlcpy(input, s_lang_keyboard_input, 128);
	return 0;
}

static void app_kb_get_weblet_content_prepare(char *edit)
{
	int max_len;

	if(edit == NULL) return;

	max_len = strlen(edit);
	if(max_len > s_keyboard_full.max_num)
		max_len = s_keyboard_full.max_num;
	if(max_len > STR_BUF_LEN)
	{
		max_len = STR_BUF_LEN-1;
	}
    keyboard_update_input(max_len);
	memset(s_lang_keyboard_input, 0, max_len);
	strncpy(s_lang_keyboard_input, edit, max_len);
	init_utf8_position((const char*)s_lang_keyboard_input,&(g_AppInput.cur_sel),&(g_AppInput.cur_max));

	GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", NULL);
	GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "clear", NULL);
	GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "select", &(g_AppInput.cur_sel));
	GUI_SetProperty(EDIT_LANG_KEYBOARD_INPUT, "string", s_lang_keyboard_input);
	if(s_keyboard_full.change_cb != NULL)
	{
		s_keyboard_full.change_cb(&s_keyboard_full);
	}
}

status_t app_kb_get_weblet_content_to_edit(char *edit)
{
	if(0 != strcasecmp(WND_KEYBOARD_LANGUAGE, GUI_GetFocusWindow()))
	{
		return GXCORE_ERROR;
	}
	app_kb_get_weblet_content_prepare(edit);

    return GXCORE_SUCCESS;
}

status_t app_kb_get_weblet_content_to_edit_save(char *edit)
{
	if(0 != strcasecmp(WND_KEYBOARD_LANGUAGE, GUI_GetFocusWindow()))
	{
		return GXCORE_ERROR;
	}
	app_kb_get_weblet_content_prepare(edit);
	app_lang_keyboard_yellow_key_func();

    return GXCORE_SUCCESS;
}
#endif
