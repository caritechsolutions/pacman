/*****************************************************************************
* 			  CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009-2014, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_unicable.c
* Author    : 	gechq
* Project   :	unicable Foundation
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   1.0  	2014.06	          glen           Creation
*****************************************************************************/

#include "app.h"
#include "app_module.h"
#include "app_pop.h"
#include "module/app_frontend.h"
#include "app_dvbs_sat_opt.h"
#include "app_windows.h"

#if UNICABLE_SUPPORT
enum UNICABLE_SET_POPUP_BOX
{
    UNICABLE_POPUP_BOX_ITEM_LNB_FRE = 0,
    UNICABLE_POPUP_BOX_ITEM_IF_CHANNEL,
    UNICABLE_POPUP_BOX_ITEM_CENTRE_FRE,
    UNICABLE_POPUP_BOX_ITEM_SAT_POSITION,
    UNICABLE_POPUP_BOX_ITEM_OK
};

enum UNICABLE_GET_ENABLE
{
     UNICABLE_STATUS_DISABLE = 0,
     UNICABLE_STATUS_ENABLE
};

extern int app_antenna_get_cur_sat_info(GxMsgProperty_NodeByPosGet * p_node_sat);
extern int app_antenna_timer_unicable_set_frontend(void);
extern int app_antenna_unicable_sat_para_save(GxMsgProperty_NodeByPosGet *p_node_sat);
extern void app_lnb_freq_sel_to_data(uint16_t cur_sel, GxBusPmDataSat *p_sat);
static GxMsgProperty_NodeByPosGet CurAtnSat;

static uint16_t ub1_slot_freq[]={
    1210,1420,1680,2040,
    1248,1516,1632,1748
};

static uint16_t ub2_slot_freq[]={
    1210,1420,1680,2040,
    984 ,1020,1056,1092,
    1128,1164,1256,1292,
    1328,1364,1458,1494,
    1530,1860,1566,1896,
    1602,1932,1638,1968,
    1716,2004,1752,2076,
    1788,2112,1824,2148
};

static char* s_UnicableLnbFreq[6] = {"9750/10600","9750/10700",
    "9750/10750","9750","10600","11300"};

#define UNICABLE_LNB_FREQ	"[9750/10600,9750/10700,9750/10750,9750,10600,11300]"
#define SAT_POSITTION	((CurAtnSat.sat_data.sat_s.unicable_para.type==GXBUS_PM_SAT_UNICABLE) ? "[A,B]" : "[A,B,C,D,E,F,G,H]")
#define IF_CHANNEL	((CurAtnSat.sat_data.sat_s.unicable_para.type==GXBUS_PM_SAT_UNICABLE) ? "[1,2,3,4,5,6,7,8]" : "[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32]")

static int32_t _unicable_lnbfreq_poplist_cb(int32_t ret_sel, unsigned short key)
{
    GUI_SetProperty("cmbox_unicable_lnb_freq", "select", &ret_sel);
    GUI_SetProperty("box_unicable_popup_para", "update", NULL);
    return 0;
}

static int _unicable_lnbfreq_poplist_create(int sel)
{
    PopList pop_list;
    int ret =-1;

    memset(&pop_list, 0, sizeof(PopList));
    pop_list.title = STR_ID_LNB_FREQ;
    pop_list.item_num = sizeof(s_UnicableLnbFreq)/sizeof(*s_UnicableLnbFreq);
    pop_list.item_content = s_UnicableLnbFreq;
    pop_list.sel = sel;
    pop_list.show_num= false;
    pop_list.mode = POP_MODE_UNBLOCK;
    pop_list.exit_cb = _unicable_lnbfreq_poplist_cb ;

    ret = poplist_create(&pop_list);

    return ret;
}

status_t app_unicable_popup_ok_keypress(void)
{
    status_t ret = GXCORE_ERROR;

    uint32_t box_sel;
    uint32_t cmb_sel;
    uint32_t if_num_sel;

    char *atoi_str = NULL;

    GUI_GetProperty("box_unicable_popup_para", "select", &box_sel);
    if(UNICABLE_POPUP_BOX_ITEM_LNB_FRE== box_sel)
    {
        GUI_GetProperty("cmbox_unicable_lnb_freq", "select", &cmb_sel);
        cmb_sel = _unicable_lnbfreq_poplist_create(cmb_sel);
        return ret ;
    }
    else if(UNICABLE_POPUP_BOX_ITEM_OK == box_sel)
    {
        GUI_GetProperty("cmbox_unicable_lnb_freq", "select", &box_sel);
        CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index = box_sel;

        GUI_GetProperty("cmb_unicable_popup_if_channel", "select", &if_num_sel);
        CurAtnSat.sat_data.sat_s.unicable_para.if_channel = if_num_sel;
        //centre fre
        GUI_GetProperty("edit_unicable_popup_centre_freq", "string", &atoi_str);
        box_sel =  atoi(atoi_str);
        CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_num_sel]=box_sel;

        GUI_GetProperty("cmb_unicable_popup_sat_pos", "select", &box_sel);
        CurAtnSat.sat_data.sat_s.unicable_para.sat_pos = box_sel;
        app_antenna_unicable_sat_para_save(&CurAtnSat);
		s_dvbs_sat_data.set_tp(SET_TP_FORCE);
        ret = GXCORE_SUCCESS;
    }
    GUI_SetProperty("box_unicable_popup_para", "update", NULL);

    return ret;
}

/*
GxCalculateSetFreqAndIniFreq:计算Unicable锁定的下行频率与中频
p_sat->sat_s.lnb1：卫星低本振
p_sat->sat_s.lnb2: 卫星高本振
pSetTPFreq:		Unicable锁定的下行频率(out)
InitialFrequency:	Unicable锁定中频(out)
return:LnbIndex
*/
int app_calculate_set_freq_and_initfreq(GxBusPmDataSat *p_sat, GxBusPmDataTP *pTpPara,
        uint32_t *pSetTPFreq,uint32_t* pInitialFrequency)
{
    //	uint32_t TempLocalFreq=0;
    int lnb_index=0;
    uint32_t if_channel=0;
    uint16_t lnb_fre = 0;
    uint8_t hiband = 0;
    uint8_t diseqc = 0;
	uint8_t polar = 0;

#define DIVERSION_FREQUENCY     (11700)

    if_channel = p_sat->sat_s.unicable_para.if_channel;

    if(p_sat->sat_s.lnb1>7000)//KU波段
    {
        if((p_sat->sat_s.lnb2==0)||(p_sat->sat_s.lnb1==p_sat->sat_s.lnb2))//单本振
        {
            (*pInitialFrequency)=pTpPara->frequency-p_sat->sat_s.lnb1;
            if(!pTpPara->tp_s.polar)
            {
                lnb_index=2;
            }
            else
            {
                lnb_index=0;
            }
            (*pSetTPFreq)=p_sat->sat_s.unicable_para.centre_fre[if_channel];
        }
        else//双本振
        {
            if (pTpPara->frequency >= DIVERSION_FREQUENCY)
            {
                lnb_fre = p_sat->sat_s.lnb2;
                if(!pTpPara->tp_s.polar)
                {
                    lnb_index=3;
                }
                else
                {
                    lnb_index=1;
                }
            }
            else
            {
                lnb_fre = p_sat->sat_s.lnb1;
                if(!pTpPara->tp_s.polar)
                {
                    lnb_index=2;
                }
                else
                {
                    lnb_index=0;
                }
            }
            (*pInitialFrequency)=pTpPara->frequency-lnb_fre;
            (*pSetTPFreq)=p_sat->sat_s.unicable_para.centre_fre[if_channel];

        }

    }

    if(GXBUS_PM_SAT_UNICABLE2 == p_sat->sat_s.unicable_para.type)
    {
		hiband = lnb_index;
		polar = app_show_to_tp_polar(pTpPara);
		diseqc = (p_sat->sat_s.unicable_para.sat_pos <<2);
		lnb_index = (polar << 5)|diseqc|hiband;
        lnb_index = lnb_index|0x80;
    }

    app_log_debug("_calculate_set_freq_and_initfreq,pSetTPFreq=%u,pInitialFrequency=%u\n",(*pSetTPFreq),(*pInitialFrequency));

    return lnb_index;


}

void app_unicable_init_if_channel_num(GxBusPmDataSat *p_sat)
{
    int i;

	if(GXBUS_PM_SAT_UNICABLE == p_sat->sat_s.unicable_para.type)
    {
        for(i = 0; i < 8; i++)
        {
            if(p_sat->sat_s.unicable_para.if_channel == i && p_sat->sat_s.unicable_para.centre_fre[i] != 0)
                continue;

            p_sat->sat_s.unicable_para.centre_fre[i] = ub1_slot_freq[i];
        }
    }
    else if(GXBUS_PM_SAT_UNICABLE2 == p_sat->sat_s.unicable_para.type)
    {
        for(i=0;i<32;i++)
        {
            if(p_sat->sat_s.unicable_para.if_channel == i && p_sat->sat_s.unicable_para.centre_fre[i] != 0)
                continue;

            p_sat->sat_s.unicable_para.centre_fre[i] = ub2_slot_freq[i];
        }
    }

}

int app_cur_sat_unicable_type_check(void)
{
    if(s_dvbs_sat_data.cur_sat.sat_data.sat_s.unicable_para.type != GXBUS_PM_SAT_UNICABLE_OFF)
    {
        return UNICABLE_STATUS_ENABLE;
    }
    else
    {
        return UNICABLE_STATUS_DISABLE;
    }
}

int app_unicable_param_check(GxBusPmDataSat sat)
{
    if(sat.sat_s.unicable_para.type != GXBUS_PM_SAT_UNICABLE_OFF
            && sat.sat_s.unicable_para.lnb_fre_index < 0x10
            && sat.sat_s.unicable_para.if_channel < MAX_IF_INDEX_NUM
            && sat.sat_s.unicable_para.centre_fre[sat.sat_s.unicable_para.if_channel] != 0)
    {
        return UNICABLE_STATUS_ENABLE;
    }
    else
    {
        return UNICABLE_STATUS_DISABLE;
    }
}

SIGNAL_HANDLER int app_unicable_set_popup_create(GuiWidget *widget, void *usrdata)
{
#define STR_LEN	20
    uint32_t sel=0;
    uint32_t if_channel=0;
    char buffer[8] = {0};
    GxBusPmSatUnicableType type = GXBUS_PM_SAT_UNICABLE_OFF;

    app_antenna_get_cur_sat_info(&CurAtnSat);
    type = CurAtnSat.sat_data.sat_s.unicable_para.type;
    if(GXBUS_PM_SAT_UNICABLE_OFF == type)
        return EVENT_TRANSFER_STOP;

    if_channel = CurAtnSat.sat_data.sat_s.unicable_para.if_channel;
	if(GXBUS_PM_SAT_UNICABLE == type && if_channel > 7)
        if_channel = 0;

    //title
    GUI_SetProperty("text_unicable_popup_title", "string", STR_ID_UNICABLE_SET);
    GUI_SetProperty("cmbox_unicable_lnb_freq", "content", UNICABLE_LNB_FREQ);
    GUI_SetProperty("cmb_unicable_popup_if_channel", "content", IF_CHANNEL);

    if(0 == CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_channel])
        CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_channel] = 1210;

    sprintf(buffer,"%04d",CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_channel]);
    GUI_SetProperty("edit_unicable_popup_centre_freq","string",buffer);
    //sat position
    GUI_SetProperty("cmb_unicable_popup_sat_pos", "content", SAT_POSITTION);

    sel = CurAtnSat.sat_data.sat_s.unicable_para.lnb_fre_index;
    if(sel >= ID_UNICANBLE_TATAL_LNB)
        sel = 0;

    GUI_SetProperty("cmbox_unicable_lnb_freq", "select", &sel);
    app_lnb_freq_sel_to_data(sel, &CurAtnSat.sat_data);
    GUI_SetProperty("cmb_unicable_popup_if_channel", "select", &if_channel);
    sel = CurAtnSat.sat_data.sat_s.unicable_para.sat_pos;
	if(GXBUS_PM_SAT_UNICABLE == type && sel > 1)
        sel = 0;

    GUI_SetProperty("cmb_unicable_popup_sat_pos", "select", &sel);

    GUI_SetProperty("img_unicablie_popup_ok_back", "state", "hide");
    GUI_SetProperty("box_unicable_popup_para", "state", "show");
    GUI_SetFocusWidget("box_unicable_popup_para");
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_unicable_set_popup_destroy(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_unicable_set_popup_keypress(GuiWidget *widget, void *usrdata)
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
            switch(event->key.sym)
            {
                case STBK_EXIT:
                case STBK_MENU:
                    {
                        GUI_EndDialog(WND_UNICABLE_SET);
                    }
                    break;

                case STBK_OK:
                    if((app_unicable_popup_ok_keypress() == GXCORE_SUCCESS))
                    {
                        GUI_EndDialog(WND_UNICABLE_SET);
                    }
                    break;

                default:
                    break;
            }
        default:
            break;
    }
    return ret;
}


SIGNAL_HANDLER int app_unicable_popup_box_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;
    uint32_t box_sel;

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
                case STBK_UP:
                    {
                        GUI_GetProperty("box_unicable_popup_para", "select", &box_sel);
                        if(UNICABLE_POPUP_BOX_ITEM_LNB_FRE == box_sel)
                        {
                            box_sel = UNICABLE_POPUP_BOX_ITEM_OK;
                            GUI_SetProperty("box_unicable_popup_para", "select", &box_sel);
                            ret = EVENT_TRANSFER_STOP;
                        }

                    }
                    break;

                case STBK_DOWN:
                    {
                        GUI_GetProperty("box_unicable_popup_para", "select", &box_sel);
                        if(UNICABLE_POPUP_BOX_ITEM_OK == box_sel)
                        {
                            box_sel = UNICABLE_POPUP_BOX_ITEM_LNB_FRE;
                            GUI_SetProperty("box_unicable_popup_para", "select", &box_sel);
                            ret = EVENT_TRANSFER_STOP;
                        }
                    }
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

SIGNAL_HANDLER int app_unicable_cmb_lnb_freq_change(GuiWidget *widget, void *usrdata)
{
    uint32_t box_sel = 0;
    GUI_GetProperty("cmbox_unicable_lnb_freq", "select", &box_sel);

    app_lnb_freq_sel_to_data(box_sel, &CurAtnSat.sat_data);
    app_antenna_timer_unicable_set_frontend();

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_unicable_cmb_if_channel_num_change(GuiWidget *widget, void *usrdata)
{
    uint32_t if_num_sel = 0;
    char buffer[8] = {0};

    GUI_GetProperty("cmb_unicable_popup_if_channel", "select", &if_num_sel);

    sprintf(buffer,"%04d",CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_num_sel]);
    GUI_SetProperty("edit_unicable_popup_centre_freq","string",buffer);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_edit_unicable_popup_centre_freq_reach_end(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_UNICABLE_POPUP_CENTRE_FREQ_LEN 4
    GUI_GetProperty("edit_unicable_popup_centre_freq", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_UNICABLE_POPUP_CENTRE_FREQ_LEN - 1 : 0);
    GUI_SetProperty("edit_unicable_popup_centre_freq", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_unicable_edit_if_fre_change(GuiWidget *widget, void *usrdata)
{
    int box_fre_sel = 0;
    uint32_t if_num_sel = 0;
    char buffer[8] = {0};
    char *atoi_str = NULL;

    //centre fre
    GUI_GetProperty("edit_unicable_popup_centre_freq", "string", &atoi_str);
    GUI_GetProperty("cmb_unicable_popup_if_channel", "select", &if_num_sel);
    box_fre_sel =  atoi(atoi_str);
    app_log_debug("fre_change=%d\n",box_fre_sel);
    if(box_fre_sel <950 || box_fre_sel > 2150)
    {
        CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_num_sel]=950;
    }
    else
    {
        CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_num_sel]=box_fre_sel;
    }
    sprintf(buffer,"%04d",CurAtnSat.sat_data.sat_s.unicable_para.centre_fre[if_num_sel]);
    GUI_SetProperty("edit_unicable_popup_centre_freq","string",buffer);
    return EVENT_TRANSFER_STOP;
}

#endif
