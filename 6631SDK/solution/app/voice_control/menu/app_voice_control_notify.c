#include "app.h"

#if VOICE_CONTROL_SUPPORT
// use style.xml
#define PROMPT_W_NAME "vc_notify_prompt"
#define PROMPT_W_TEXT "txt_vc_notify_content"
#define PROMPT_NAME "_notify_"
#define NOTIFY_X_POS 30
#define NOTIFY_Y_POS 40

static event_list *thiz_notify_timer = NULL;
static event_list *thiz_reset_timer = NULL;
static int _notify_timer_cb(void *usrdata)
{
    APP_TIMER_REMOVE(thiz_notify_timer);
    if (GUI_CheckPrompt(PROMPT_NAME) == GXCORE_SUCCESS)
    {
        GUI_EndPrompt(PROMPT_NAME);
    }
    return 0;
}

static int _reset_timer_cb(void *usrdata)
{
    APP_TIMER_REMOVE(thiz_reset_timer);
    if (GUI_CheckPrompt(PROMPT_NAME) == GXCORE_SUCCESS)
    {
         GUI_SetProperty(PROMPT_W_TEXT, "string", "What else can I do for you?");
    }
    return 0;
}

void app_voice_control_notify(char *str)
{
    if(NULL == str)
        return;
    APP_TIMER_REMOVE(thiz_reset_timer);
    APP_TIMER_REMOVE(thiz_notify_timer);
    if(GUI_CheckPrompt(PROMPT_NAME) == GXCORE_SUCCESS)
    {
        char *string = NULL;
        GUI_GetProperty(PROMPT_W_TEXT, "string", &string);
        if((NULL == string)||(0 != memcmp(string, str, strlen(string))))
        {
            GUI_SetProperty(PROMPT_W_TEXT, "string", str);
            APP_TIMER_REMOVE(thiz_reset_timer);
        }
        return;
    }
    GUI_CreatePrompt(NOTIFY_X_POS, NOTIFY_Y_POS, PROMPT_NAME, PROMPT_W_NAME, "", "ontop");
    GUI_SetProperty(PROMPT_W_TEXT, "string", str);
}

void app_voice_control_notify_send(char *str)
{
    if(NULL == str)
        return;
    if(GUI_CheckPrompt(PROMPT_NAME) == GXCORE_SUCCESS)
    {
        // TODO:updata the string
        char *string = NULL;
        GUI_GetProperty(PROMPT_W_TEXT, "string", &string);
        if((NULL == string)||(0 != memcmp(string, str, strlen(string))))
        {
            GUI_SetProperty(PROMPT_W_TEXT, "string", str);
        }
        APP_TIMER_ADD(thiz_reset_timer, _reset_timer_cb, 5000, TIMER_REPEAT);
        return;
    }
}

void app_voice_control_notify_clear(void)
{
    if(GUI_CheckPrompt(PROMPT_NAME) == GXCORE_SUCCESS)
    {
        APP_TIMER_REMOVE(thiz_reset_timer);
        GUI_SetProperty(PROMPT_W_TEXT, "string", "Bye bye!!");
        APP_TIMER_ADD(thiz_notify_timer, _notify_timer_cb, 1000, TIMER_REPEAT);
    }
}

//
#define _WAIT_1  "."
#define _WAIT_2  ".."
#define _WAIT_3  "..."
static event_list *thiz_wait_timer = NULL;
static int thiz_wait_index = 0;
static int _wait_timer_cb(void *usrdata)
{
    char *str[] = {_WAIT_1,_WAIT_2,_WAIT_3};

    if ((GUI_CheckPrompt(PROMPT_NAME) == GXCORE_SUCCESS)
            && (thiz_wait_index >= 0) && (thiz_wait_index < 3))
    {
        if(thiz_wait_index == 2)
        {
            thiz_wait_index = 0;
            GUI_SetProperty(PROMPT_W_TEXT, "string", " ");
            //GUI_SetInterface("flush", NULL);
        }
        else
        {
            thiz_wait_index++;
        }
        GUI_SetProperty(PROMPT_W_TEXT, "string", str[thiz_wait_index]);
    }
    return 0;
}

void app_voice_control_notify_ex(char *str)
{
    if(NULL == str)
        return;

    APP_TIMER_REMOVE(thiz_wait_timer);
    if(GUI_CheckPrompt(PROMPT_NAME) == GXCORE_SUCCESS)
    {
        // TODO:updata the string
        char *string = NULL;
        GUI_GetProperty(PROMPT_W_TEXT, "string", &string);
        if((NULL == string)||(0 != memcmp(string, str, strlen(string))))
        {
            GUI_SetProperty(PROMPT_W_TEXT, "string", str);
        }
        APP_TIMER_ADD(thiz_notify_timer, _notify_timer_cb, 3000, TIMER_REPEAT);
        return;
    }
    GUI_CreatePrompt(NOTIFY_X_POS, NOTIFY_Y_POS, PROMPT_NAME, PROMPT_W_NAME, "", "ontop");
    GUI_SetProperty(PROMPT_W_TEXT, "string", str);
    APP_TIMER_ADD(thiz_notify_timer, _notify_timer_cb, 3000, TIMER_REPEAT);
}


void app_voice_control_notify_waiting(void)
{
    if(GUI_CheckPrompt(PROMPT_NAME) == GXCORE_SUCCESS)
    {
        APP_TIMER_ADD(thiz_notify_timer, _notify_timer_cb, 500, TIMER_REPEAT);
        return;
    }
    GUI_CreatePrompt(NOTIFY_X_POS, NOTIFY_Y_POS, PROMPT_NAME, PROMPT_W_NAME, "", "ontop");
    GUI_SetProperty(PROMPT_W_TEXT, "string", _WAIT_1);
    thiz_wait_index = 0;
    APP_TIMER_ADD(thiz_wait_timer, _wait_timer_cb, 500, TIMER_REPEAT);
}


#endif

