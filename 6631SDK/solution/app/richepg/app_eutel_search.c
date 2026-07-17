#include "app.h"

#if RICHEPG_SUPPORT
#include "app_windows.h"
#include "app_eutel_database.h"
#include "app_eutel_sat_fuse.h"
#include "app_eutel_windows.h"
#include "app_eutel_sat_fuse.h"
#include "app_eutel_common.h"

#define EUTEL_SAT_NAME_LENGTH	(MAX_SAT_NAME+20)

struct _eutel_search_item
{
	unsigned int used_id   ;
    unsigned int count     ;
	unsigned int   area[MAX_SAT_NAME] ;
};

struct _eutel_search_item s_eutel_search ;

void app_eutel_search_opt_init(void)
{
	int i ;
	int sel ;
	char *pname = NULL ;
	uint32_t box_sel;
    box_sel = 2;
	
	char buffer[EUTEL_SAT_NAME_LENGTH] = {0};
	char buffer_all[EUTEL_SAT_NAME_LENGTH*4] = {0};

	if (s_eutel_search.count <=1 )
	{
		//hide show 会造成记录state状态,改成disable
		GUI_SetProperty("boxitem_eutel_search_area","disable",NULL);
		GUI_SetProperty("text_eutel_search_area","string"," ");
		GUI_SetProperty("cmb_eutel_search_mode_area", "content", " ");
		return ;
	}
	
	memset(buffer_all, 0, 4*EUTEL_SAT_NAME_LENGTH);

	strcat(buffer_all, "[");
	
	for(i=0;i<s_eutel_search.count;i++)
	{
		memset(buffer, 0, EUTEL_SAT_NAME_LENGTH);

		pname = (char*)app_eutel_sat_get_area_name(s_eutel_search.area[i]);
		if ( pname )
			sprintf(buffer,"%s,", pname);
		
		strcat(buffer_all, buffer);
	}

	memcpy(buffer_all+strlen(buffer_all)-1, "]", 1);
	GUI_SetProperty("cmb_eutel_search_mode_area", "content", buffer_all);
	sel = s_eutel_search.used_id ;
	GUI_SetProperty("cmb_eutel_search_mode_area", "select", &sel);
	box_sel = 1;
	GUI_SetProperty("box_eutel_search_all","select",&box_sel);
}

SIGNAL_HANDLER int app_eutel_search_create(GuiWidget *widget, void *usrdata)
{
    GUI_SetProperty("boxitem_eutel_search_mode","disable",NULL);
    GUI_SetProperty("cmb_eutel_search_mode", "content", "[Sat.tv install]");

	app_eutel_search_opt_init();

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_search_destroy(GuiWidget *widget, void *usrdata)
{
    s_eutel_search.count = 0 ;

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_search_box_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    uint32_t box_sel;
	uint32_t area_sel;
	char value_str[20] = {0};

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
                case STBK_EXIT:
                case STBK_MENU:
                    {
                        GUI_EndDialog(WND_EUTEL_SEARCH);
                    }
                    break;

                case STBK_DOWN:
                    {
                        ret = EVENT_TRANSFER_KEEPON;
                    }
                    break;
				case STBK_UP:
                    {
                        ret = EVENT_TRANSFER_KEEPON;
                    }
                    break;

                case STBK_OK:
                    {
						GUI_GetProperty("box_eutel_search_all","select",&box_sel);
						if (box_sel == 2)
						{
							GUI_GetProperty("boxitem_eutel_search_area", "state", value_str);
							if(0==strcmp(value_str,"show"))
							{
								GUI_GetProperty("cmb_eutel_search_mode_area","select",&area_sel);
							}
							else
							{
								area_sel = 0 ;
							}
							if ( area_sel < 0 || area_sel >= EUTUL_SAT_MAX_NUM )
								return ret;

							GUI_EndDialog(WND_EUTEL_SEARCH);

							extern void app_eutel_ant_install_exec(int sel);
							app_eutel_ant_install_exec(s_eutel_search.area[area_sel]);
						}
                        ret = EVENT_TRANSFER_STOP;
                    }
                    break;

                default:
                    break;
            }
        default:
            break;
    }
	return ret ;
}

void app_eutel_search_create_menu(int sat_id,int tp_id)
{
	s_eutel_search.used_id = 0 ;
	s_eutel_search.count = app_eutel_sat_fuse_get_all_area_count(sat_id,
				s_eutel_search.area);

	if (  s_eutel_search.count > 0)
	{
		if ( (tp_id >=0) && ( tp_id < s_eutel_search.count ))
		{
			s_eutel_search.used_id = tp_id ;
		}

		GUI_CreateDialog(WND_EUTEL_SEARCH);
	}
}

#endif

