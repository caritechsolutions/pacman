#include "app.h"
#if NETAPPS_SUPPORT
#include "app_module.h"
#include "app_windows.h"
#include "app_module.h"
#include "app_msg.h"
#include "gui_core.h"
#include "app_send_msg.h"
#include "app_pop.h"
#include "app_default_params.h"
#include "app_netvideo_play.h"

#define TEXT_NUMBER	"txt_netvideo_play_number"
#define NUM_MAX  (5)

struct ThisUICtrol{
    NetVideoPlayOps *play_ops;
    event_list *number_set_timer;
    char key_buf[NUM_MAX + 1];
    uint32_t key_num;
};

static struct ThisUICtrol this = {0};


status_t app_netvideo_play_number_call(NetVideoPlayOps *play_ops)
{
	if(play_ops == NULL)
		return GXCORE_ERROR;

	memset(&this, 0, sizeof(struct ThisUICtrol));
    this.play_ops = play_ops;

	if(GXCORE_ERROR == GUI_CheckDialog(WND_NETVIDEO_PLAY_NUMBER))
	{
		GUI_CreateDialog(WND_NETVIDEO_PLAY_NUMBER);
	}
    return 0;
}


static void netvideo_play_number_set(unsigned short key_value)
{
	switch (key_value)
	{
		case STBK_1:
			strcat(this.key_buf, "1");
			break;
		case STBK_2:
			strcat(this.key_buf, "2");
			break;
		case STBK_3:
			strcat(this.key_buf, "3");
			break;
		case STBK_4:
			strcat(this.key_buf, "4");
			break;
		case STBK_5:
			strcat(this.key_buf, "5");
			break;
		case STBK_6:
			strcat(this.key_buf, "6");
			break;
		case STBK_7:
			strcat(this.key_buf, "7");
			break;
		case STBK_8:
			strcat(this.key_buf, "8");
			break;
		case STBK_9:
			strcat(this.key_buf, "9");
			break;
		case STBK_0:
			strcat(this.key_buf, "0");
			break;
		default:
			break;
	}
}

static int _play_by_num(NetVideoPlayOps *playops, unsigned int num)
{
    int32_t num_value = num;
    int32_t total = 0;
    PopDlg pop = {0};

    if(playops->play_ctrl.play_list_total_get)
    {
        total = playops->play_ctrl.play_list_total_get();

        if((num_value > total || num_value == 0) && GXCORE_ERROR == GUI_CheckDialog(WND_POP_BOOK))
        {
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_NO_BTN;
            pop.mode = POP_MODE_UNBLOCK;
            pop.str = STR_ID_NO_PROGRAM;
            pop.timeout_sec = 3;
            popdlg_create(&pop);

            return GXCORE_ERROR;
        }

        num_value--;
        if(playops->play_ctrl.play_list_ok != NULL && num_value != playops->list_focus_item)
        {
            playops->list_focus_item = num_value;
            playops->play_ctrl.play_list_ok(playops->list_focus_item);
            if(playops->play_ctrl.play_list_data_get != NULL)
            {
                playops->netvideo_title = playops->play_ctrl.play_list_data_get(playops->list_focus_item);
                return GXCORE_SUCCESS;
            }
        }
    }
    return GXCORE_ERROR;
}

static int netvideo_play_number_timeout(void* data)
{
    this.number_set_timer = NULL;
    int32_t num_value;

    num_value = atoi(this.key_buf);

    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_NETVIDEO_PLAY_NUMBER))
    {
        GUI_EndDialog(WND_NETVIDEO_PLAY_NUMBER);
        GUI_SetProperty(WND_NETVIDEO_PLAY_NUMBER, "draw_now", NULL);
    }
    return _play_by_num(this.play_ops, num_value);
}

SIGNAL_HANDLER int app_netvideo_play_number_create(const char* widgetname, void *usrdata)
{
	if (reset_timer(this.number_set_timer) != 0)
	{
		this.number_set_timer = create_timer(netvideo_play_number_timeout, 1500, NULL, TIMER_ONCE);
	}

	this.key_num = 0;
	memset(this.key_buf, 0, NUM_MAX + 1);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_netvideo_play_number_destroy(const char* widgetname, void *usrdata)
{
	remove_timer(this.number_set_timer);
	this.number_set_timer = NULL;

	this.key_num = 0;
	memset(this.key_buf, 0, NUM_MAX + 1);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_netvideo_play_number_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case STBK_1:
			case STBK_2:
			case STBK_3:
			case STBK_4:
			case STBK_5:
			case STBK_6:
			case STBK_7:
			case STBK_8:
			case STBK_9:
			case STBK_0:
				reset_timer(this.number_set_timer);
				if (this.key_num >= NUM_MAX)
				{
					this.key_num = 0;
					memset(this.key_buf, 0, NUM_MAX + 1);
				}

				this.key_num++;
				netvideo_play_number_set(event->key.sym);
				GUI_SetProperty(TEXT_NUMBER, "string", this.key_buf);
				break;

			case STBK_OK:
                remove_timer(this.number_set_timer);
                this.number_set_timer = NULL;
				netvideo_play_number_timeout(NULL);
				break;

			case STBK_EXIT:
			case VK_BOOK_TRIGGER:
				GUI_EndDialog(WND_NETVIDEO_PLAY_NUMBER);
				break;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}
#endif

#if VOICE_CONTROL_SUPPORT
#if NETAPPS_SUPPORT
int app_vc_net_number_switch_url(NetVideoPlayOps* playops, unsigned int num)
{
    int ret = 0;
    ret = _play_by_num(playops, num);
    return ret;
}
#endif
#endif

