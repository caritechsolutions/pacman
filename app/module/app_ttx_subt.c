/*
 * =====================================================================================
 *
 *       Filename:  app_ttx_subt.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2011年10月11日 14时49分37秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "app_module.h"
#include "app_send_msg.h"
#include "module/ttx/gxttx.h"
#include "full_screen.h"
#include "app_default_params.h"
#include "app_config.h"
#ifdef COLOR_16BIT_TTX_SUPPORT
#include "module/subtitle/gx_subtitle_setting.h"
#include "bus/module/subtitle/gx_subtitle_magazine.h"

extern char *app_get_short_lang_str(uint32_t lang);
#endif
#ifndef COLOR_16BIT_TTX_SUPPORT
static uint32_t ttx_change_lang_to_charset(void)
{
	int lang_value = 0;

    GxBus_ConfigGetInt(TTX_LANG_KEY, &lang_value, TTX_LANG);

    if(lang_value == 0)/*auto*/
    {
        return 0xFF;
    }
    else
    {
        lang_value = lang_value -1;
    }

    if((lang_value >= LANG_MAX)||(lang_value < 0))
        return 0xFF;

    else if(strcmp(g_LangName[lang_value], STR_ID_ENGLISH) == 0)
        return English;
    else if(strcmp(g_LangName[lang_value], STR_ID_ARABIC) == 0)
        return Arabic;
    else if(strcmp(g_LangName[lang_value], STR_ID_RUSSIAN) == 0)
        return Russian;
    else if(strcmp(g_LangName[lang_value], STR_ID_FRENCH) == 0)
        return French;
    else if(strcmp(g_LangName[lang_value], STR_ID_PORTUGUESE) == 0)
        return Portuguese;
    else if(strcmp(g_LangName[lang_value], STR_ID_TURKISH) == 0)
        return Turkish;
    else if(strcmp(g_LangName[lang_value], STR_ID_SPAIN) == 0)
        return Portuguese;
    else if(strcmp(g_LangName[lang_value], STR_ID_GERMAN) == 0)
        return German;
    else if(strcmp(g_LangName[lang_value], STR_ID_FARSI) == 0)
        return Arabic;
    else if(strcmp(g_LangName[lang_value], STR_ID_THAI) == 0)
        return English;
    else if((strcmp(g_LangName[lang_value], STR_ID_POLISH) == 0)||(strcmp(g_LangName[lang_value], STR_ID_POLSKI) == 0))
        return Polish;
    else if(strcmp(g_LangName[lang_value], STR_ID_ITALIAN) == 0)
        return Italian;
    else if(strcmp(g_LangName[lang_value], STR_ID_UKRAINIAN) == 0)
        return Ukrainian;
    else if(strcmp(g_LangName[lang_value], STR_ID_BULGARIAN) == 0)
        return Russian;
    else if(strcmp(g_LangName[lang_value], STR_ID_SERBIAN) == 0)
        return Serbian_Cyrillic_2_Set;
    else if(strcmp(g_LangName[lang_value], STR_ID_ROMANIAN) == 0)
        return Rumanian;
    else if(strcmp(g_LangName[lang_value], STR_ID_MACEDONIAN) == 0)
        return Serbian_Cyrillic_2_Set;
    else if(strcmp(g_LangName[lang_value], STR_ID_GREECE) == 0)
        return Greek;
    else
        return 0xFF;
}
#endif


static void ttx_info_get(ttx_subt *p, Teletext *p_ttx, uint32_t elem_pid)
{
    if(p != NULL)
    {
        p->elem_pid = elem_pid;

        if(p_ttx != NULL)
        {
            memcpy(p->lang, p_ttx->iso639, 3);
            p->type= p_ttx->type;

            p->ttx.magazine = p_ttx->magazine;
            p->ttx.page = p_ttx->page;
        }
    }
}

static void subt_info_get(ttx_subt *p, Subtiling *p_subt, uint32_t elem_pid)
{
    p->elem_pid = elem_pid;
    memcpy(p->lang, p_subt->iso639, 3);
    p->type= p_subt->type;

    p->subt.composite_page_id = p_subt->composite_page_id;
    p->subt.ancillary_page_id = p_subt->ancillary_page_id;
}

#if ARIBCC_SUPPORT
static void aribcc_info_get(ttx_subt *p, Subtiling *p_aribcc, uint32_t elem_pid)
{
	if(p!=NULL)
	{
    	p->elem_pid = elem_pid;
	}

	if(p_aribcc != NULL)
	{
		if(p_aribcc->iso639 != NULL)
		{
	    	memcpy(p->lang, p_aribcc->iso639, 3);
		}
	    p->type= p_aribcc->type;
	    p->subt.composite_page_id = p_aribcc->composite_page_id;
	    p->subt.ancillary_page_id = p_aribcc->ancillary_page_id;
	}
	return;
}
#endif

static void ttx_subt_sync(TtxSubtOps* p, PmtInfo* pmt)
{
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t k = 0;
    uint32_t subt_num=0;
    uint32_t ExistFlag = 0;
    TeletextDescriptor *p_ttx = NULL;
    SubtDescriptor     *p_subt = NULL;
	#if ARIBCC_SUPPORT
	AribccDescriptor   *p_aribcc = NULL;
	#endif

    if (pmt==NULL || p==NULL)    return;

    if (pmt->ttx_count == 0 && pmt->subt_count == 0
        #if ARIBCC_SUPPORT
            && pmt->aribcc_count == 0
        #endif
       )
    {
        return;
    }

#ifndef COLOR_16BIT_TTX_SUPPORT
   // ttx init here, after init ,then ttx can use
    static uint32_t ttx_init_flag = 0;
    if (ttx_init_flag == 0)
    {
        ttx_init_flag = 1;

        // TTX configs
        if(p->ttx_config)
            p->ttx_config();

        GxTtx_Init();
    }
#endif
    p->ttx_num = 0;
    p->subt_num = 0;
    p->sync_flag= 0;

    // check total won't overflow
    for (i=0; i<pmt->ttx_count; i++)
    {
        p_ttx = (TeletextDescriptor*)pmt->ttx_info[i];
        p->ttx_num += p_ttx->ttx_num;
    }
    for (i=0; i<pmt->subt_count; i++)
    {
        p_subt = (SubtDescriptor*)pmt->subt_info[i];
        p->subt_num += p_subt->subt_num;
    }
#if ARIBCC_SUPPORT
	p->aribcc_num = 0;
	for (i=0; i<pmt->aribcc_count; i++)
    {
        p_aribcc = (AribccDescriptor*)pmt->aribcc_info[i];
        p->aribcc_num += p_aribcc->aribcc_num;
    }
	if (p->ttx_num+p->subt_num+p->aribcc_num > TTX_SUBT_TOTAL)
    {
        app_log_error("[ttx_subt_cc] need lager memory!\n");
        return;
    }
#else
    if (p->ttx_num+p->subt_num > TTX_SUBT_TOTAL)
    {
        app_log_error("[ttx_subt] need lager memory!\n");
        return;
    }
#endif
    memset(&p->magazine, 0, sizeof(ttx_subt));
    memset(p->content, 0, sizeof(ttx_subt)*TTX_SUBT_TOTAL);

    // info get
    for (i=0; i<pmt->ttx_count; i++)
    {
        p_ttx = (TeletextDescriptor*)pmt->ttx_info[i];
        ttx_info_get(&p->magazine, NULL, p_ttx->elem_pid);

        for (j=0; j<p_ttx->ttx_num; j++)
        {
            if ((p_ttx->ttx[j].type == 2)
                    || (p_ttx->ttx[j].type == 5))  /*type 2,5 for ttx subtitle*/
            {
                for(k = 0; k < subt_num;k++)
                {
                    if((p->content[k].elem_pid == p_ttx->elem_pid)
                            &&(0 == memcmp(p->content[k].lang,p_ttx->ttx[j].iso639,3))
                            &&(p->content[k].type == p_ttx->ttx[j].type)
                            &&(p->content[k].ttx.magazine == p_ttx->ttx[j].magazine)
                            &&(p->content[k].ttx.page == p_ttx->ttx[j].page))
                    {
                        ExistFlag = 1;
                        break;
                    }
                }
                if(ExistFlag == 0)
                {// not find the same one
                    ttx_info_get(&p->content[subt_num], &p_ttx->ttx[j], p_ttx->elem_pid);
                    subt_num++;
                }
            }
        }
    }

    p->ttx_num = subt_num;
    ExistFlag = 0;
    for (i=0; i<pmt->subt_count; i++)
    {
        p_subt = (SubtDescriptor*)pmt->subt_info[i];

        for (j=0; j<p_subt->subt_num; j++)
        {
            for(k = p->ttx_num; k < subt_num;k++)
            {
                if((p->content[k].elem_pid == p_subt->elem_pid)
                        &&(0 == memcmp(p->content[k].lang,p_subt->subt[j].iso639,3))
                        &&(p->content[k].type == p_subt->subt[j].type)
                        &&(p->content[k].subt.composite_page_id == p_subt->subt[j].composite_page_id)
                        &&(p->content[k].subt.ancillary_page_id == p_subt->subt[j].ancillary_page_id))
                {
                    ExistFlag = 1;
                    break;
                }
            }
            if(ExistFlag == 0)
            {
                subt_info_get(&p->content[subt_num], &p_subt->subt[j], p_subt->elem_pid);
                subt_num++;
            }
        }
    }
    p->subt_num= subt_num - p->ttx_num;
	#if ARIBCC_SUPPORT
	ExistFlag = 0;
    for (i=0; i<pmt->aribcc_count; i++)
    {
        p_aribcc= (AribccDescriptor*)pmt->aribcc_info[i];

        for (j=0; j<p_aribcc->aribcc_num; j++)
        {
            for(k = p->subt_num; k < subt_num;k++)
            {
                if((p->content[k].elem_pid == p_aribcc->elem_pid)
                        &&(0 == memcmp(p->content[k].lang,p_aribcc->aribcc[j].iso639,3))
                        &&(p->content[k].type == p_aribcc->aribcc[j].type)
                        &&(p->content[k].subt.composite_page_id == p_aribcc->aribcc[j].composite_page_id)
                        &&(p->content[k].subt.ancillary_page_id == p_aribcc->aribcc[j].ancillary_page_id))
                {
                    ExistFlag = 1;
                    break;
                }
            }
            if(ExistFlag == 0)
            {
                aribcc_info_get(&p->content[subt_num], &p_aribcc->aribcc[j], p_aribcc->elem_pid);
                subt_num++;
            }
        }
    }
    p->aribcc_num= subt_num - p->ttx_num - p->subt_num;
	if(pmt->aribcc_count&&(p_aribcc->aribcc_num == 0))
	{
		aribcc_info_get(&p->content[subt_num], &p_aribcc->aribcc[j], p_aribcc->elem_pid);
		p->aribcc_num = pmt->aribcc_count;
	}
	#endif
    p->sync_flag= 1;
    return;
}

static status_t ttx_magazine_open(TtxSubtOps* p)
{
    if (p == NULL)  return GXCORE_ERROR;

#ifdef COLOR_16BIT_TTX_SUPPORT
    int lang_value = 0;
    GX_SUBTITLE_LANG lang;
    GxMsgProperty_PlayerSubtitleLoad subt = {0};
    if(p->magazine.elem_pid != 0)
    {
#if CA_SUPPORT
        if(app_tsd_check())
        {
            app_exshift_pid_add(1, (int*)&(p->magazine.elem_pid));
        }
#endif
        memset(&subt,0,sizeof(GxMsgProperty_PlayerSubtitleLoad));
        subt.player = PLAYER_FOR_NORMAL;
        subt.para.pid.pid =  p->magazine.elem_pid;
        subt.para.pid.major = p->magazine.ttx.magazine;
        subt.para.pid.minor = p->magazine.ttx.page;
        subt.para.type = PLAYER_SUB_TYPE_DVB_MAG;
        GX_SUBTITLE_DISPLAY dis ={GX_SUBTITLE_SPP,1,60,60,30,30};
        GxSubtitle_SetDisplay(dis);
        GxBus_ConfigGetInt(TTX_LANG_KEY, &lang_value, TTX_LANG);
        GX_SUBTITLE_BACK_COLOR back = {1, 0x00ffffff};
        GxSubtitle_SetBackColor(back);
        if(lang_value > 0)
        {
            memset((void*)lang.str ,0 ,sizeof(lang.str));
            strcpy((char*)lang.str,app_get_short_lang_str(lang_value-1));
            lang.force_flag = GX_SUBTITLE_LANG_FORCE;
            GxSubtitle_SetLang(lang);
        }
        else
        {
            GX_SUBTITLE_PGLANG pglang = {0};
            pglang.count = p->ttx_num;
            int i = 0;
            for(i = 0; i < p->ttx_num; i++)
            {
                pglang.page[i].comp_page_id = p->content[i].ttx.magazine;
                pglang.page[i].anci_page_id = p->content[i].ttx.page;
                memcpy(pglang.page[i].pglang,p->content[i].lang,3);
            }
            GxSubtitle_MagSetPageLang(&pglang); //Forcefully set subtitle page language encoding based on subtitle language information
            lang.force_flag = GX_SUBTITLE_LANG_AUTO;
            GxSubtitle_SetLang(lang);
        }
        GxSubtitle_MagSetOpacity(GX_SUBTITLE_MAG_FULL_OPACITY);
        app_send_msg_exec(GXMSG_PLAYER_SUBTITLE_LOAD, (void*)&subt);
        p->player_subt =  (PlayerSubtitle*)subt.sub;
#if CA_SUPPORT
        app_extend_add_element(p->magazine.elem_pid, CONTROL_NORMAL, DATA_TYPE_SUBTITLE);
#endif
        g_AppFullArb.state.ttx = STATE_ON;
        GxSubtitle_MagJumpPage(100);
        GxSubtitle_MagShowString("P100");

        return GXCORE_SUCCESS;
    }

#else

    if (p->magazine.elem_pid != 0)
    {
#if CA_SUPPORT
        if(app_tsd_check())
        {
            app_exshift_pid_add(1, (int*)&(p->magazine.elem_pid));
        }
#endif
		g_AppFullArb.state.ttx = STATE_ON;
		GxTtx_TtxStart(p->magazine.elem_pid);
		GxTtx_SetLang(ttx_change_lang_to_charset());
		GxTtx_SetPid(p->magazine.elem_pid,p->magazine.ttx.magazine,p->magazine.ttx.page);
		GxTtx_Enable();
#if CA_SUPPORT
        app_extend_add_element(p->magazine.elem_pid, CONTROL_NORMAL, DATA_TYPE_TELETEXT);
#endif

		return GXCORE_SUCCESS;
	}
#endif
	return GXCORE_ERROR;
}

#ifdef COLOR_16BIT_TTX_SUPPORT
int app_teletext_key_oprerat(TtxSubtOps* p,uint16_t key_value)
{
	unsigned int curpage = 0;
	unsigned int pgno[6];
	unsigned int keyvalue = 0;
	char strpage[10] = {0};
	static uint8_t chPostion = 0;
    static uint8_t pPageNumTem[3]={0};
    static GX_SUBTITLE_MAG_OPACITY opacity = GX_SUBTITLE_MAG_FULL_OPACITY;

    if(NULL == p->player_subt || g_AppFullArb.state.ttx == STATE_OFF)
    {
        return GXCORE_ERROR;
    }
	switch(key_value)
	{
        case STBK_OK:
            {
                if(opacity == GX_SUBTITLE_MAG_NONE_OPACITY)
                {
                    opacity = GX_SUBTITLE_MAG_FULL_OPACITY;
                }
                else
                {
                    opacity++;
                }
                GxSubtitle_MagSetOpacity(opacity);
            }
            break;
		case STBK_UP:
			{
				memset(strpage, 0, sizeof(strpage));

				GxSubtitle_MagNextPage();
				curpage = GxSubtitle_MagGetCurrentPgNo();

				sprintf(strpage, " P%3d", curpage);
				GxSubtitle_MagShowString(strpage);
			}
			break;
		case STBK_DOWN:
			{
				memset(strpage, 0, sizeof(strpage));

				GxSubtitle_MagPrevPage();
				curpage = GxSubtitle_MagGetCurrentPgNo();

				sprintf(strpage, " P%3d", curpage);
				GxSubtitle_MagShowString(strpage);
			}
			break;
		case STBK_LEFT:
			{
				GxSubtitle_MagPrevSubPage();
			}
			break;
		case STBK_RIGHT:
			{
				GxSubtitle_MagNextSubPage();
			}
			break;
		case STBK_RED:
			{
				GxSubtitle_MagGetSixLinkPgNo(pgno);
				GxSubtitle_MagJumpPage(pgno[0]);
				memset(strpage, 0, sizeof(strpage));
				sprintf(strpage, " P%3d", pgno[0]);
				GxSubtitle_MagShowString(strpage);
			}
			break;
		case STBK_GREEN:
			{

				GxSubtitle_MagGetSixLinkPgNo(pgno);
				GxSubtitle_MagJumpPage(pgno[1]);
				/*显示页码*/
				memset(strpage, 0, sizeof(strpage));
				sprintf(strpage, " P%3d", pgno[1]);
				GxSubtitle_MagShowString(strpage);
			}
			break;
		case STBK_YELLOW:
			{
				GxSubtitle_MagGetSixLinkPgNo(pgno);
				GxSubtitle_MagJumpPage(pgno[2]);
				/*显示页码*/
				memset(strpage, 0, sizeof(strpage));
				sprintf(strpage, " P%3d", pgno[2]);
				GxSubtitle_MagShowString(strpage);
			}
			break;
		case STBK_BLUE:
			{

				GxSubtitle_MagGetSixLinkPgNo(pgno);
				GxSubtitle_MagJumpPage(pgno[3]);
				/*显示页码*/
				memset(strpage, 0, sizeof(strpage));
				sprintf(strpage, " P%3d", pgno[3]);
				GxSubtitle_MagShowString(strpage);
			}
			break;
		case STBK_0:
		case STBK_1:
		case STBK_2:
		case STBK_3:
		case STBK_4:
		case STBK_5:
		case STBK_6:
		case STBK_7:
		case STBK_8:
		case STBK_9:
			{
				if (STBK_0 == key_value && 0 == chPostion)
				{
					break;
				}

				keyvalue = key_value - STBK_0;

				if (0 == chPostion)
				{
                    pPageNumTem[0] = keyvalue;
					memset(strpage, 0, sizeof(strpage));
					sprintf(strpage, " P%d__", pPageNumTem[0]);

					GxSubtitle_MagShowString(strpage);
				}

				if (1 == chPostion)
				{
                    pPageNumTem[1]= keyvalue;
					memset(strpage, 0, sizeof(strpage));
					sprintf(strpage, " P%d%d_", pPageNumTem[0], pPageNumTem[1]);

					GxSubtitle_MagShowString(strpage);
				}

				if (2 == chPostion)
				{
                    pPageNumTem[2]= keyvalue;
					memset(strpage, 0, sizeof(strpage));
					sprintf(strpage, " P%d%d%d", pPageNumTem[0], pPageNumTem[1],pPageNumTem[2]);

					GxSubtitle_MagShowString(strpage);
				}

				chPostion ++;
				if (3 == chPostion)
				{
                    curpage = 100*pPageNumTem[0]+pPageNumTem[1]*10+pPageNumTem[2];
					memset(strpage, 0, sizeof(strpage));
					sprintf(strpage, " P%3d", curpage);
					GxSubtitle_MagJumpPage(curpage);
					GxSubtitle_MagShowString(strpage);
					chPostion = 0;
				}

			}
			break;
        case STBK_EXIT:
            {
                GxMsgProperty_PlayerSubtitleUnLoad unload;
                unload.player = PLAYER_FOR_NORMAL;
                unload.sub = p->player_subt;
                app_send_msg_exec(GXMSG_PLAYER_SUBTITLE_UNLOAD, (void*)&unload);
                GX_SUBTITLE_DISPLAY dis ={GX_SUBTITLE_SPP,1,20,20,20,20};
                GxSubtitle_SetDisplay(dis);
                GX_SUBTITLE_BACK_COLOR back = {0, 0x00000000};
                GxSubtitle_SetBackColor(back);
                p->player_subt = NULL;
#if CA_SUPPORT
                app_descrambler_close_by_type(CONTROL_NORMAL, DATA_TYPE_SUBTITLE);
#endif
                g_AppFullArb.timer_start();
                g_AppFullArb.state.ttx = STATE_OFF;
                app_subt_resume();
            }
            break;
		default:
			break;
	}
    return GXCORE_SUCCESS;
}
#else
int app_teletext_key_oprerat(TtxSubtOps* p,uint16_t key_value)
{
    if(GxTtx_IsnotEnable() && !GxTtx_CcIsnotEnable())
    {
        if(GxTtx_IsnotWorking())
        {
            switch(key_value)
            {

                case STBK_UP:
                    GxTtx_SetKey(DownKey);
                    break;

                case STBK_DOWN:
                    GxTtx_SetKey(UpKey);
                    break;

                case STBK_LEFT:
                    GxTtx_SetKey(LeftKey);
                    break;

                case STBK_RIGHT:
                    GxTtx_SetKey(RightKey);
                    break;
                case STBK_0:
                    GxTtx_QuickSearchEnable();
                    GxTtx_SetKey(NumKey0);
                    break;
                case STBK_1:
                    GxTtx_QuickSearchEnable();
                    GxTtx_SetKey(NumKey1);
                    break;
                case STBK_2:
                    GxTtx_QuickSearchEnable();
                    GxTtx_SetKey(NumKey2);
                    break;
                case STBK_3:
                    GxTtx_QuickSearchEnable();
                    GxTtx_SetKey(NumKey3);
                    break;
                case STBK_4:
                    GxTtx_QuickSearchEnable();
                    GxTtx_SetKey(NumKey4);
                    break;
                case STBK_5:
                    GxTtx_QuickSearchEnable();
                    GxTtx_SetKey(NumKey5);
                    break;
                case STBK_6:
                    GxTtx_QuickSearchEnable();
                    GxTtx_SetKey(NumKey6);
                    break;
                case STBK_7:
                    GxTtx_QuickSearchEnable();
                    GxTtx_SetKey(NumKey7);
                    break;
                case STBK_8:
                    GxTtx_QuickSearchEnable();
                    GxTtx_SetKey(NumKey8);
                    break;
                case STBK_9:
                    GxTtx_QuickSearchEnable();
                    GxTtx_SetKey(NumKey9);
                    break;

                case STBK_RED:
                    GxTtx_SetKey(RedKey);
                    break;

                case STBK_GREEN:
                    GxTtx_SetKey(GreenKey);
                    break;

                case STBK_YELLOW:
                    GxTtx_SetKey(YellowKey);
                    break;

                case STBK_BLUE:
                    GxTtx_SetKey(BlueKey);
                    break;

                case STBK_OK:
                    GxTtx_SetKey(OkKey);
                    break;

                case STBK_EXIT:
                    GxTtx_SubtDisable();
                    GxTtx_TtxStop();
                    g_AppFullArb.timer_start();
					g_AppFullArb.state.ttx = STATE_OFF;
                    app_subt_resume();
#if CA_SUPPORT
                    app_descrambler_close_by_type(CONTROL_NORMAL, DATA_TYPE_TELETEXT);
#endif
                    g_AppTtxSubt.ttx_pre_process_init(&g_AppTtxSubt);
                    break;

                default:
                    break;

            }
            return GXCORE_SUCCESS;
        }
    }

    return GXCORE_ERROR;
}
#endif


static status_t ttx_magazine_operat(TtxSubtOps* p, uint16_t key_value)
{
        return app_teletext_key_oprerat(p,key_value);
}

static status_t ttx_subt_open(TtxSubtOps *p, uint32_t sel)
{
    if (p==NULL) return GXCORE_ERROR;
#ifdef COLOR_16BIT_TTX_SUPPORT
    int lang_value = 0;
    GX_SUBTITLE_LANG lang;
    GxMsgProperty_PlayerSubtitleLoad subt = {0};
    GxMsgProperty_PlayerSubtitleSwitch subt_switch = {0};

    if(p->ttx_num > sel)
    {
#if CA_SUPPORT
        if(app_tsd_check())
        {
            app_exshift_pid_add(1, (int*)&(p->content[sel].elem_pid));
        }
#endif
        GxBus_ConfigGetInt(TTX_LANG_KEY, &lang_value, TTX_LANG);
        if(lang_value > 0)
        {
            memset((void*)lang.str ,0 ,sizeof(lang.str));
            strcpy((char*)lang.str,app_get_short_lang_str(lang_value-1));
            lang.force_flag = GX_SUBTITLE_LANG_FORCE;
            GxSubtitle_SetLang(lang);
        }
        else if(p->content[sel].lang != NULL)
        {
            memset((void*)lang.str ,0 ,sizeof(lang.str));
            strncpy((char*)lang.str,(char*)p->content[sel].lang,3);
            lang.force_flag = GX_SUBTITLE_LANG_AUTO;
            GxSubtitle_SetLang(lang);
        }
        if(NULL == p->player_subt)
        {
            memset(&subt,0,sizeof(GxMsgProperty_PlayerSubtitleLoad));
            subt.player = PLAYER_FOR_NORMAL;
            subt.para.pid.pid =  p->content[sel].elem_pid;
            subt.para.pid.major = p->content[sel].ttx.magazine;
            subt.para.pid.minor = p->content[sel].ttx.page;
            subt.para.type = PLAYER_SUB_TYPE_DVB_TTX;
            app_send_msg_exec(GXMSG_PLAYER_SUBTITLE_LOAD, (void*)&subt);
            p->player_subt =  (PlayerSubtitle*)subt.sub;
        }
        else
        {
            subt_switch.player = PLAYER_FOR_NORMAL;
            subt_switch.sub = p->player_subt;
            subt_switch.pid.pid =p->content[sel].elem_pid;
            subt_switch.pid.major = p->content[sel].ttx.magazine;
            subt_switch.pid.minor = p->content[sel].ttx.page;
            app_send_msg_exec(GXMSG_PLAYER_SUBTITLE_SWITCH, (void*)&subt_switch);
        }
#if CA_SUPPORT
        app_extend_add_element(p->content[sel].elem_pid, CONTROL_NORMAL, DATA_TYPE_SUBTITLE);
#endif
        return GXCORE_SUCCESS;
    }
#else
    if((sel<p->ttx_num) && (!GxTtx_IsnotEnable()))
    {
        GxTtx_TtxStop();
        GxTtx_SetLang(ttx_change_lang_to_charset());
        GxTtx_TtxStart(p->content[sel].elem_pid);
#if CA_SUPPORT
        if(app_tsd_check())
        {
            app_exshift_pid_add(1, (int*)&(p->content[sel].elem_pid));
        }
#endif
        GxTtx_SubtEnable(p->content[sel].elem_pid,
                p->content[sel].ttx.magazine,
                p->content[sel].ttx.page);
#if CA_SUPPORT
        app_extend_add_element(p->content[sel].elem_pid, CONTROL_NORMAL, DATA_TYPE_TELETEXT);
#endif

        return GXCORE_SUCCESS;
    }
#endif
    return GXCORE_ERROR;
}

static void ttx_subt_close(TtxSubtOps *p)
{
    if (p==NULL) return;
#ifdef COLOR_16BIT_TTX_SUPPORT
    GxMsgProperty_PlayerSubtitleUnLoad unload;
    if(p->player_subt != NULL)
    {
        unload.player = PLAYER_FOR_NORMAL;
        unload.sub = p->player_subt;
        app_send_msg_exec(GXMSG_PLAYER_SUBTITLE_UNLOAD, (void*)&unload);
        p->player_subt = NULL;
#if CA_SUPPORT
        app_descrambler_close_by_type(CONTROL_NORMAL, DATA_TYPE_SUBTITLE);
#endif
    }
#else
    if(GxTtx_CcIsnotEnable())
    {
        GxTtx_SubtDisable();
        GxTtx_TtxStop();
#if CA_SUPPORT
        app_descrambler_close_by_type(CONTROL_NORMAL, DATA_TYPE_TELETEXT);
#endif
    }
#endif
}

static status_t dvb_subt_open(TtxSubtOps *p, uint32_t sel)
{
    GxMsgProperty_PlayerSubtitleLoad subt = {0};
    GxMsgProperty_PlayerSubtitleSwitch subt_switch = {0};

    if(p->subt_num > sel)
    {
        // offset ttx subt
        sel += p->ttx_num;
#if CA_SUPPORT
        if(app_tsd_check())
        {
            app_exshift_pid_add(1, (int*)&(p->content[sel].elem_pid));
        }
#endif
        if(NULL == p->player_subt)
        {
            // first open subt
            memset(&subt,0,sizeof(GxMsgProperty_PlayerSubtitleLoad));
            subt.player = PLAYER_FOR_NORMAL;
            subt.para.pid.pid =  p->content[sel].elem_pid;

            subt.para.pid.major = p->content[sel].subt.composite_page_id;
            subt.para.pid.minor = p->content[sel].subt.ancillary_page_id;

            subt.para.type = PLAYER_SUB_TYPE_DVB;

            app_send_msg_exec(GXMSG_PLAYER_SUBTITLE_LOAD, (void*)&subt);
            p->player_subt =  (PlayerSubtitle*)subt.sub;


        }
        else
        {
            // subt switch
            subt_switch.player = PLAYER_FOR_NORMAL;
            subt_switch.sub = p->player_subt;
            subt_switch.pid.pid =p->content[sel].elem_pid;

            subt_switch.pid.major = p->content[sel].subt.composite_page_id;
            subt_switch.pid.minor = p->content[sel].subt.ancillary_page_id;

            app_send_msg_exec(GXMSG_PLAYER_SUBTITLE_SWITCH, (void*)&subt_switch);
        }

#if CA_SUPPORT
    //app_descrambler_init(g_AppPlayOps.normal_play.dmx_id, CONTROL_NORMAL, DATA_TYPE_SUBTITLE);
    //app_descrambler_set_pid(p->content[sel].elem_pid, CONTROL_NORMAL, DATA_TYPE_SUBTITLE);
    app_extend_add_element(p->content[sel].elem_pid, CONTROL_NORMAL, DATA_TYPE_SUBTITLE);
#endif
        return GXCORE_SUCCESS;
    }

    return GXCORE_ERROR;
}

static void dvb_subt_close(TtxSubtOps *p)
{
    GxMsgProperty_PlayerSubtitleUnLoad unload;

    if (p==NULL)    return;

    if(p->player_subt != NULL)
    {
        unload.player = PLAYER_FOR_NORMAL;
        unload.sub = p->player_subt;

        app_send_msg_exec(GXMSG_PLAYER_SUBTITLE_UNLOAD, (void*)&unload);
        p->player_subt = NULL;
#if CA_SUPPORT
        app_descrambler_close_by_type(CONTROL_NORMAL, DATA_TYPE_SUBTITLE);
#endif
    }
}

static void ttx_subt_clean(TtxSubtOps *p)
{
	if(p == NULL)
	{
		app_log_error("\nAPP TTX SUBT, error, %s, %d\n", __FUNCTION__, __LINE__);
		return;
	}
	p->magazine.elem_pid = 0;
	p->subt_num = 0;
	p->ttx_num = 0;

}

static void ttx_pre_process_initialize(TtxSubtOps *p)
{
#ifndef COLOR_16BIT_TTX_SUPPORT
#if TTX_PRE_PROCESS_ENABLE > 0
	TeletextPreProcessConfig_t configs = {0};
	if(p == NULL)
	{
		app_log_error("\nAPP TTX SUBT, error, %s, %d\n", __FUNCTION__, __LINE__);
		return;
	}
	if((p->magazine.elem_pid > 0)
			&& (g_AppFullArb.state.subt == STATE_OFF)
			&& (g_AppFullArb.state.ttx == STATE_OFF))
	{
		configs.TtxPid = p->magazine.elem_pid;
		GxTtx_TtxPreStart(configs);
#if CA_SUPPORT
        if(app_tsd_check())
        {
            app_exshift_pid_add(1, (int*)&(p->magazine.elem_pid));
        }
        app_extend_add_element(p->magazine.elem_pid, CONTROL_NORMAL, DATA_TYPE_TELETEXT);
#endif

	}
#endif
#endif
}
static void ttx_pre_process_destroy(TtxSubtOps *p)
{
#ifndef COLOR_16BIT_TTX_SUPPORT
#if TTX_PRE_PROCESS_ENABLE > 0
	if(p == NULL)
	{
		app_log_error("\nAPP TTX SUBT, error, %s, %d\n", __FUNCTION__, __LINE__);
		return;
	}
	GxTtx_TtxPreStop();
#if CA_SUPPORT
    app_descrambler_close_by_type(CONTROL_NORMAL, DATA_TYPE_TELETEXT);
#endif

#endif
#endif
}

static void ttx_configs(void)
{
#ifndef COLOR_16BIT_TTX_SUPPORT
#if TTX_PRE_PROCESS_ENABLE > 0
	TeletextConfig_t Config = {0};
	Config.MagazineCacheNum = 8;// cache  magazines, from 1~8
	Config.TeletextPreProcessEnable = 1;// enable the pre-process
	GxTtx_TtxConfigs(Config);
#endif
#endif
}
#if ARIBCC_SUPPORT
extern int app_aribcc_sub_start(int demux,unsigned int pid,int composition,int ancillary);
extern void app_aribcc_sub_stop(void);
extern bool app_aribcc_is_running(void);

static status_t aribcc_subt_open(TtxSubtOps *p, uint32_t sel)
{
    if(p->aribcc_num> sel)
    {
        // offset ttx subt
        sel += p->ttx_num+p->subt_num;
#if CA_SUPPORT
        if(app_tsd_check())
        {
            app_exshift_pid_add(1, (int*)&(p->content[sel].elem_pid));
        }
#endif
		app_aribcc_sub_start(0,p->content[sel].elem_pid,
		p->content[sel].subt.composite_page_id,
		p->content[sel].subt.ancillary_page_id);

#if CA_SUPPORT
    app_extend_add_element(p->content[sel].elem_pid, CONTROL_NORMAL, DATA_TYPE_SUBTITLE);
#endif
        return GXCORE_SUCCESS;
    }

    return GXCORE_ERROR;
}


static void aribcc_subt_close(TtxSubtOps *p)
{
    if (p==NULL)    return;

    if(app_aribcc_is_running())
    {

		app_aribcc_sub_stop();
#if CA_SUPPORT
        app_descrambler_close_by_type(CONTROL_NORMAL, DATA_TYPE_SUBTITLE);
#endif
    }
}
#endif

TtxSubtOps g_AppTtxSubt =
{
    .sync                  = ttx_subt_sync,

    .ttx_magz_open         = ttx_magazine_open,
    .ttx_magz_opt          = ttx_magazine_operat,

    .ttx_subt_open         = ttx_subt_open,
    .ttx_subt_close        = ttx_subt_close,

    .dvb_subt_open         = dvb_subt_open,
    .dvb_subt_close        = dvb_subt_close,
    .clean                 = ttx_subt_clean,

    .ttx_pre_process_init  = ttx_pre_process_initialize,
    .ttx_pre_process_close = ttx_pre_process_destroy,
    .ttx_config            = ttx_configs,
#if ARIBCC_SUPPORT
    .aribcc_subt_open         = aribcc_subt_open,
    .aribcc_subt_close        = aribcc_subt_close,
#endif
};

