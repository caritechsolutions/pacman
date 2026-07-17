#include "app.h"
#include "../message.h"

#ifdef MEMORY_TEST_OUTPUT_OSD
#define WND_MT_MESSAGE "wnd_mt_message"
#define MT_MESSAGE_XML_PATH    "wnd_mt_message.xml"

#define MT_MESSAGE_TXT_COUNT    12
#define MT_MESSAGE_TXT_BASE "text_mt_message_"
#define MT_MESSAGE_TXT_ERR_COUNT    4
#define MT_MESSAGE_TXT_ERR_BASE "text_mt_message_err_"
#define MT_MESSAGE_TXT_ERR_TOTAL "text_mt_message_err_5"

static char thiz_dialog_exist = 0;
static char thiz_mt_message_txt_sel = -1;
static char thiz_mt_message_txt_err_sel = -1;
static unsigned int thiz_mt_total_error = 0;

static char* _mt_message_get_widget_name_by_index(char* name_base, int index)
{
    static char buf[32] = {0};
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "%s%d", name_base, index);
    return buf;
}

static void _mt_message_osd_display(MTMessageClass* message)
{
    char *widget_name = NULL;
    int i = 0;
    char *widget_str = NULL;
    char buf[100] = {0};

    if(thiz_mt_message_txt_sel == MT_MESSAGE_TXT_COUNT)
    {// reflash
        for(i = 0; i < (MT_MESSAGE_TXT_COUNT - 1); i++)
        {
            widget_name = _mt_message_get_widget_name_by_index(MT_MESSAGE_TXT_BASE, (i+2));
            GUI_GetProperty(widget_name, "string", &widget_str);
            widget_name = _mt_message_get_widget_name_by_index(MT_MESSAGE_TXT_BASE, (i+1));
            GUI_SetProperty(widget_name, "string", widget_str);
        }
    }
    else
    {
        thiz_mt_message_txt_sel++;
    }
    widget_name = _mt_message_get_widget_name_by_index(MT_MESSAGE_TXT_BASE, thiz_mt_message_txt_sel);
    GUI_SetProperty(widget_name, "string", message->str);

    // err display
    if(MT_ERR == message->err_flag)
    {
        if(thiz_mt_message_txt_err_sel == MT_MESSAGE_TXT_ERR_COUNT)
        {//
            for(i = 0; i < (MT_MESSAGE_TXT_ERR_COUNT - 1); i++)
            {
                widget_name = _mt_message_get_widget_name_by_index(MT_MESSAGE_TXT_ERR_BASE, (i+2));
                GUI_GetProperty(widget_name, "string", &widget_str);
                widget_name = _mt_message_get_widget_name_by_index(MT_MESSAGE_TXT_ERR_BASE, (i+1));
                GUI_SetProperty(widget_name, "string", widget_str);
            }
        }
        else
        {// reflash
            thiz_mt_message_txt_err_sel++;
        }
        widget_name = _mt_message_get_widget_name_by_index(MT_MESSAGE_TXT_ERR_BASE, thiz_mt_message_txt_err_sel);
        GUI_SetProperty(widget_name, "string", message->str);
    }
    //
    if(message->err_count != thiz_mt_total_error)
    {
        thiz_mt_total_error = message->err_count;
        sprintf(buf, "Number of error occurrences: %d", thiz_mt_total_error);
        GUI_SetProperty(MT_MESSAGE_TXT_ERR_TOTAL, "string", buf);
    }
}

static int _mt_mesage_service(GxMessage *pMsg)
{
    AppMsgMessageClass *new_message = NULL;
    switch(pMsg->msg_id)
    {
        case APPMSG_MESSAGE:
             new_message = (AppMsgMessageClass*)GxBus_GetMsgPropertyPtr(pMsg,AppMsgMessageClass);
             if((NULL == new_message) || (APPMSG_MESSAGE_MT != new_message->type) || (NULL == new_message->data))
             {
                 break;
             }
            _mt_message_osd_display((MTMessageClass*)new_message->data);
             break;
        default:
             break;
    }
    return GXMSG_OK;
}


SIGNAL_HANDLER int app_mt_message_create(const char* widgetname, void *usrdata)
{
    int i = 0;
    char buf[100] = {0};
    // clear data
    thiz_mt_message_txt_sel = 0;
    thiz_mt_message_txt_err_sel = 0;
    for(i = 0; i < MT_MESSAGE_TXT_COUNT; i++)
    {
        GUI_SetProperty(_mt_message_get_widget_name_by_index(MT_MESSAGE_TXT_BASE, (i+1)), "string", " ");
    }
    for(i = 0; i < MT_MESSAGE_TXT_ERR_COUNT; i++)
    {
        GUI_SetProperty(_mt_message_get_widget_name_by_index(MT_MESSAGE_TXT_ERR_BASE, (i+1)), "string", " ");
    }
    thiz_dialog_exist = 1;
    //
    sprintf(buf, "Number of error occurrences: %d", thiz_mt_total_error);
    GUI_SetProperty(MT_MESSAGE_TXT_ERR_TOTAL, "string", buf);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_mt_message_destroy(const char* widgetname, void *usrdata)
{
    thiz_dialog_exist = 0;

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_mt_message_keypress(const char* widgetname, void *usrdata)
{

    return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_mt_message_service(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;
    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            _mt_mesage_service((GxMessage*)(event->msg.service_msg));
            break;
        default:
            break;
    }
    return ret;
}


void app_mt_message_osd_exec(void)
{
    static char flag = 0;

    if(thiz_dialog_exist)
        return;

    if(flag == 0)
    {
        GUI_LinkDialog(WND_MT_MESSAGE, MT_MESSAGE_XML_PATH);
        flag = 1;
    }
    GUI_CreateDialog(WND_MT_MESSAGE);
}

int app_mt_message_dialog_is_exist(void)
{
    return thiz_dialog_exist;
}
#endif
