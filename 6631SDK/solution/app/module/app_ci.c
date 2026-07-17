/*
 * =====================================================================================
 *
 *       Filename:  app_ci.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2010年12月21日 15时04分04秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */
#include "module/app_ci.h"
#include "dvb_ci/cim_includes.h"

#ifdef HAVE_LIB_CIM
/* ci menu */
static status_t menu_selectable(void)
{
    return GXCORE_SUCCESS;
}

static status_t menu_entries(int choice)
{
#define CHOICE_BASE    (1)

    cim_set_choice(ci.slot, CHOICE_BASE+choice);

    return GXCORE_SUCCESS;
}

static status_t menu_cancel(void)
{
    cim_set_choice(ci.slot, 0);

    return GXCORE_SUCCESS;
}

static status_t menu_abort(void)
{
    return GXCORE_SUCCESS;
}


/* ci enquiry */
static status_t enquiry_reply(const char* answer_data, int data_len)
{
    cim_set_answer(ci.slot, answer_data, data_len);

    return GXCORE_SUCCESS;
}

static status_t enquiry_cancel(void)
{
    cim_set_choice(ci.slot, 0);

    return GXCORE_SUCCESS;
}

static status_t enquiry_abort(void)
{
    return GXCORE_SUCCESS;
}


/* struct ci_menu & ci_enquiry */
static ci_menu menu =
{
    .selectable = menu_selectable,
    .select = menu_entries,
    .cancel = menu_cancel,
    .abort = menu_abort
};

static ci_enquiry enquiry =
{
    .reply = enquiry_reply,
    .cancel = enquiry_cancel,
    .abort = enquiry_abort
};


/* ci ops */
static ci_menu* ci_get_menu(void)
{
    return &menu;
}

static ci_enquiry* ci_get_enquiry(void)
{
    return &enquiry;
}

static const char* ci_get_name(ci_slot slot)
{
    return cim_get_name(slot);
}

static status_t ci_mmi_open(ci_slot slot)
{
    cim_open_mmi(slot);

    ci.slot = slot;

    return GXCORE_SUCCESS;
}

static status_t ci_mmi_close(ci_slot slot)
{
    cim_close_mmi(slot);

    // needn't care about slot, when open will set a new value
    return GXCORE_SUCCESS;
}

static slot_entry* ci_state_change(void)
{
    uint32_t i;
    static slot_entry entry;

    for (i=0; i<SOLT_TOTAL; i++)
    {
        if (TRUE == cim_is_state_changed(i))
        {
            entry.slot = i;
            entry.state = cim_get_state(i);

            return &entry;
        }
    }

    return NULL;
}

static void text_copy(const char *src, char **dst)
{
#define STR_END_LEN (1)

    if (*dst != NULL)
    {
        GxCore_Free(*dst);
        *dst = NULL;
    }

    *dst = GxCore_Malloc(strlen(src)+STR_END_LEN);
    if (*dst != NULL)
    {
        strcpy(*dst, src);
    }
}

static mmi_state* ci_mode_change(void)
{
    static mmi_state state;
    uint32_t i;

    if (TRUE == cim_is_mode_changed(ci.slot))
    {
        state = cim_get_mode(ci.slot);

        switch (state)
        {
            case CI_MMI_EXIT:
                cim_set_choice(ci.slot, 0);
                break;
            case CI_MMI_MENU:
            case CI_MMI_LIST:
                text_copy(cim_get_text(ci.slot, CI_INDEX_TITLE), &(menu.title_text));
                text_copy(cim_get_text(ci.slot, CI_INDEX_SUBTITLE), &(menu.subtitle_text));
                text_copy(cim_get_text(ci.slot, CI_INDEX_BOTTOM), &(menu.bottom_text));

                menu.entries_num = cim_get_item_cnt(ci.slot);

				if(menu.entries_num > MAX_CIMENU_ENTRIES)
					menu.entries_num = 	MAX_CIMENU_ENTRIES;

                for (i=0; i<menu.entries_num; i++)
                {
                    text_copy(cim_get_text(ci.slot, CI_INDEX_ENTRIES+i), &(menu.entries_text[i]));
                }

                break;
            case CI_MMI_ENQ:
                text_copy(cim_get_text(ci.slot, CI_INDEX_ENQUIRY), &(enquiry.text));
                enquiry.blind = cim_is_blind_answer(ci.slot);
                enquiry.expected_length = cim_get_answer_length(ci.slot);

                break;
            default:
                return NULL;
        }

        return &state;
    }

    return NULL;
}

void ci_init(void)
{
    static int thread = 0;
    GxCore_ThreadCreate("cim", &thread, (void*)cim_daemon_thread, NULL,20*1024,GXOS_DEFAULT_PRIORITY);
}

static void ci_update_pmt(ci_slot slot, const char *pmt_data,int len)
{
    cim_update_pmt((const unsigned char*)pmt_data,len , slot);
}

/* extern ci_ops */
ci_ops ci =
{
    .get_menu = ci_get_menu,
    .get_enquiry = ci_get_enquiry,
    .get_name = ci_get_name,
    .mmi_open = ci_mmi_open,
    .mmi_close = ci_mmi_close,
    .cim_state_change = ci_state_change,
    .mmi_mode_change = ci_mode_change,
    .update_pmt = ci_update_pmt,
    .init = ci_init,
};

#endif

