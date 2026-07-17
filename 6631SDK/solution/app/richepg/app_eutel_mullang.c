#include "app.h"
#if RICHEPG_SUPPORT
#include "richepg.h"
#include "richepg_store.h"
#include "app_eutel_common.h"

bool app_richepg_gui_reverse(void)
{
	int32_t init_value = 0;

	GxBus_ConfigGetInt(OSD_LANG_KEY, &init_value, OSD_LANG);
	if ((strcmp(g_LangName[init_value], STR_ID_ARABIC) == 0)
			|| (strcmp(g_LangName[init_value], STR_ID_FARSI) == 0))
		return true;
	else
		return false;
}

int app_richepg_keyboard_sel(void)
{
	int32_t init_value = 0;
	int sel = ITEM_ENGLISH;

	GxBus_ConfigGetInt(OSD_LANG_KEY, &init_value, OSD_LANG);
	if (strcmp(g_LangName[init_value], STR_ID_ARABIC) == 0)
	{
		sel = ITEM_ARABIC;
	}
	else if (strcmp(g_LangName[init_value], STR_ID_FARSI) == 0)
	{
		sel = ITEM_FARSI;
	}
	else if (strcmp(g_LangName[init_value], STR_ID_FRENCH) == 0)
	{
		sel = ITEM_ENGLISH;
	}
	else if (strcmp(g_LangName[init_value], STR_ID_RUSSIAN) == 0)
	{
		sel = ITEM_RUSSIAN;
	}

	return sel;
}

char *app_richepg_translate_str(char *str)
{
	int32_t init_value = 0;

	if (!str)
		return str;

	GxBus_ConfigGetInt(OSD_LANG_KEY, &init_value, OSD_LANG);
	if (strcmp(g_LangName[init_value], STR_ID_ARABIC) == 0)
	{
		if (strcmp(str, STR_ID_NO_INFO) == 0)
			return "لا يوجد معلومات";
		else if (strcmp(str, STR_ID_NO_EPG_INFO) == 0)
			return "للحصول على أوصاف البرامج الكاملة والصور المصغرة ، يرجى إدخال وحدة تخزين USB!";
		else if (strcmp(str, STR_ID_UNKNOW) == 0)
			return "غير معروف";
		else if (strcmp(str, STR_ID_NO_EVENT) == 0)
			return "لا حدث!";
		else if (strcmp(str, "Got") == 0)
			return "تم الحصول على";
		else if (strcmp(str, STR_ID_FREE_SCAN_RECORD) == 0)
			return "تسجيل مسح ضوئي مجاني";
	}
	else if (strcmp(g_LangName[init_value], STR_ID_FARSI) == 0)
	{
		if (strcmp(str, STR_ID_NO_INFO) == 0)
			return "بدون اطلاعات";
		else if (strcmp(str, STR_ID_NO_EPG_INFO) == 0)
			return "برای توضیحات کامل برنامه و تصاویر کوچک ، لطفاً USB USB را وارد کنید!";
		else if (strcmp(str, STR_ID_UNKNOW) == 0)
			return "نامعلوم";
		else if (strcmp(str, STR_ID_NO_EVENT) == 0)
			return "هیچ رویدادی!";
		else if (strcmp(str, "Got") == 0)
			return "بدست آورد";
		else if (strcmp(str, STR_ID_FREE_SCAN_RECORD) == 0)
			return "تسجيل مسح ضوئي مجاني";
	}
	else if (strcmp(g_LangName[init_value], STR_ID_FRENCH) == 0)
	{
		if (strcmp(str, STR_ID_NO_INFO) == 0)
			return "Aucune information";
		else if (strcmp(str, STR_ID_NO_EPG_INFO) == 0)
			return "Pour une description complète du programme et des illustrations, veuillez insérer une clé USB!";
		else if (strcmp(str, STR_ID_UNKNOW) == 0)
			return "Inconnu";
		else if (strcmp(str, STR_ID_NO_EVENT) == 0)
			return "Pas d'évènement!";
		else if (strcmp(str, "Got") == 0)
			return "Got";
		else if (strcmp(str, STR_ID_FREE_SCAN_RECORD) == 0)
			return "Enregistrement de scan gratuit";
	}
	else if (strcmp(g_LangName[init_value], STR_ID_RUSSIAN) == 0)
	{
		if (strcmp(str, STR_ID_NO_INFO) == 0)
			return "Нет информации";
		else if (strcmp(str, STR_ID_NO_EPG_INFO) == 0)
			return "Для полного описания программы и миниатюр изображений, пожалуйста, вставьте флешку!";
		else if (strcmp(str, STR_ID_UNKNOW) == 0)
			return "не известно";
		else if (strcmp(str, STR_ID_NO_EVENT) == 0)
			return "Нет событий!";
		else if (strcmp(str, "Got") == 0)
			return "Получил";
		else if (strcmp(str, STR_ID_FREE_SCAN_RECORD) == 0)
			return "Бесплатная запись сканирования";
	}

	return str;
}

char *app_richepg_epg_no_info_str_get(void)
{
	if (richepg_get_profile_type(USB_EPG_TIMESTAMP) == RICHEPG_MID_PROFILE)
		return app_richepg_translate_str(STR_ID_NO_EPG_INFO);
	else
		return app_richepg_translate_str(STR_ID_NO_INFO);
}

#endif

