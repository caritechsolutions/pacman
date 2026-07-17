#include "app.h"

#if NETAPPS_SUPPORT
#include "app_module.h"
#include "app_msg.h"
#include "app_netapps_msg.h"

#if VOICE_CONTROL_SUPPORT
#include "vc_cmd.h"
extern int app_vc_cloud_msg_transfer(void *message, unsigned int *cmd, char **content);

static int is_number_str(const char *str)
{
	int len = 0;
	int i = 0;

	if(str&&((len=strlen(str))>0))
	{
		for(i=0; i<len; i++)
		{
			if(!isdigit(str[i]))
			{
				return 0;
			}
		}
		return 1;
	}
	return 0;
}

static netapps_msg_t *app_netapps_get_search_msg(char *content)
{
	netapps_msg_t *msg = NULL;

	if(content)
	{
		msg = (netapps_msg_t *)GxCore_Calloc(1, sizeof(netapps_msg_t));
		msg->msg_id = NETAPPS_MSG_SEARCH;
		msg->content_str = strdup(content);
	}

	return msg;
}

static netapps_msg_t *app_netapps_get_category_msg(char *content)
{
	netapps_msg_t *msg = NULL;

	if(content)
	{
		msg = (netapps_msg_t *)GxCore_Calloc(1, sizeof(netapps_msg_t));
		if(is_number_str(content))
		{
			msg->msg_id = NETAPPS_MSG_CATEGORY_NUM;
			msg->content_int = atoi(content);
		}
		else if(strcmp(content, "next") == 0)
		{
			msg->msg_id = NETAPPS_MSG_CATEGORY_NEXT;
		}
		else if(strcmp(content, "prev") == 0)
		{
			msg->msg_id = NETAPPS_MSG_CATEGORY_PREV;
		}
		else if(strcmp(content, "first") == 0)
		{
			msg->msg_id = NETAPPS_MSG_CATEGORY_NUM;
			msg->content_int = 1;
		}
		else if(strcmp(content, "last") == 0)
		{
			msg->msg_id = NETAPPS_MSG_CATEGORY_LAST;
		}
		else
		{
			msg->msg_id = NETAPPS_MSG_CATEGORY_NAME;
			msg->content_str = strdup(content);
		}
	}

	return msg;
}

static netapps_msg_t *app_netapps_get_group_msg(char *content)
{
	netapps_msg_t *msg = NULL;

	if(content)
	{
		msg = (netapps_msg_t *)GxCore_Calloc(1, sizeof(netapps_msg_t));
		if(strcmp(content, "next") == 0)
		{
			msg->msg_id = NETAPPS_MSG_GROUP_NEXT;
		}
		else if(strcmp(content, "prev") == 0)
		{
			msg->msg_id = NETAPPS_MSG_GROUP_PREV;
		}
		else
		{
			msg->msg_id = NETAPPS_MSG_GROUP_NAME;
			msg->content_str = strdup(content);
		}
	}

	return msg;
}

static netapps_msg_t *app_netapps_get_page_msg(char *content)
{
	netapps_msg_t *msg = NULL;

	if(content)
	{
		msg = (netapps_msg_t *)GxCore_Calloc(1, sizeof(netapps_msg_t));
		if(is_number_str(content))
		{
			msg->msg_id = NETAPPS_MSG_PAGE_NUM;
			msg->content_int = atoi(content);
		}
		else if(strcmp(content, "next") == 0)
		{
			msg->msg_id = NETAPPS_MSG_PAGE_NEXT;
		}
		else if(strcmp(content, "prev") == 0)
		{
			msg->msg_id = NETAPPS_MSG_PAGE_PREV;
		}
		else if(strcmp(content, "first") == 0)
		{
			msg->msg_id = NETAPPS_MSG_PAGE_NUM;
			msg->content_int = 1;
		}
		else if(strcmp(content, "last") == 0)
		{
			msg->msg_id = NETAPPS_MSG_PAGE_LAST;
		}
		else
		{
			GxCore_Free(msg);
			msg = NULL;
		}
	}

	return msg;
}

static netapps_msg_t *app_netapps_get_play_msg(char *content)
{
	netapps_msg_t *msg = NULL;

	if(content)
	{
		msg = (netapps_msg_t *)GxCore_Calloc(1, sizeof(netapps_msg_t));
		if(is_number_str(content))
		{
			msg->msg_id = NETAPPS_MSG_PLAY_NUM;
			msg->content_int = atoi(content);
		}
		else if(strcmp(content, "next") == 0)
		{
			msg->msg_id = NETAPPS_MSG_PLAY_NEXT;
		}
		else if(strcmp(content, "prev") == 0)
		{
			msg->msg_id = NETAPPS_MSG_PLAY_PREV;
		}
		else if(strcmp(content, "first") == 0)
		{
			msg->msg_id = NETAPPS_MSG_PLAY_NUM;
			msg->content_int = 1;
		}
		else if(strcmp(content, "last") == 0)
		{
			msg->msg_id = NETAPPS_MSG_PLAY_LAST;
		}
		else
		{
			GxCore_Free(msg);
			msg = NULL;
		}
	}

	return msg;
}

static netapps_msg_t *app_netapps_get_setting_msg(char *content)
{
	netapps_msg_t *msg = NULL;

	msg = (netapps_msg_t *)GxCore_Calloc(1, sizeof(netapps_msg_t));
	msg->msg_id = NETAPPS_MSG_SETTING;

	return msg;
}

static netapps_msg_t *app_netapps_get_user_msg(char *content)
{
	netapps_msg_t *msg = NULL;

	if(content)
	{
		msg = (netapps_msg_t *)GxCore_Calloc(1, sizeof(netapps_msg_t));
		if(strcmp(content, "login") == 0)
		{
			msg->msg_id = NETAPPS_MSG_USER_LOGIN;
		}
		else if(strcmp(content, "logout") == 0)
		{
			msg->msg_id = NETAPPS_MSG_USER_LOGOUT;
		}
		else
		{
			GxCore_Free(msg);
			msg = NULL;
		}
	}

	return msg;
}

static netapps_msg_t *app_netapps_get_private_msg(char *content)
{
	netapps_msg_t *msg = NULL;

	if(content)
	{
		msg = (netapps_msg_t *)GxCore_Calloc(1, sizeof(netapps_msg_t));
		msg->msg_id = NETAPPS_MSG_PRIVATE;
		msg->content_str = strdup(content);
	}

	return msg;
}
#endif

static netapps_msg_t *app_netapps_get_app_msg(GxMessage *gui_msg)
{
	netapps_msg_t *msg = NULL;
	AppMsgMessageClass* app_msg = NULL;

	app_msg = (AppMsgMessageClass *)GxBus_GetMsgPropertyPtr(gui_msg, AppMsgMessageClass);
	if(app_msg)
	{
#if  VOICE_CONTROL_SUPPORT
		if((app_msg->type == APPMSG_MESSAGE_VC)&&app_msg->data)
		{
            unsigned int cmd = 0;
            char *content = NULL;
			if(app_vc_cloud_msg_transfer((void *)app_msg->data, &cmd, &content) == 0)
			{
				switch(cmd)
				{
					case VOICE_CMD_NETAPPS_SEARCH:
						msg = app_netapps_get_search_msg(content);
						break;

					case VOICE_CMD_NETAPPS_CATEGORY:
						msg = app_netapps_get_category_msg(content);
						break;

					case VOICE_CMD_NETAPPS_GROUP:
						msg = app_netapps_get_group_msg(content);
						break;

					case VOICE_CMD_NETAPPS_PAGE:
						msg = app_netapps_get_page_msg(content);
						break;

					case VOICE_CMD_NETAPPS_PLAY:
						msg = app_netapps_get_play_msg(content);
						break;

					case VOICE_CMD_NETAPPS_SETTING:
						msg = app_netapps_get_setting_msg(content);
						break;

					case VOICE_CMD_NETAPPS_USER:
						msg = app_netapps_get_user_msg(content);
						break;

					case VOICE_CMD_NETAPPS_PRIVATE:
						msg = app_netapps_get_private_msg(content);
						break;

					default:
						break;
				}
			}
		}
#endif
	}

	return msg;
}


netapps_msg_t *app_netapps_message_get(void *usrdata)
{
	GUI_Event *gui_event = NULL;
	GxMessage *gui_msg = NULL;
	netapps_msg_t *msg = NULL;

	if(usrdata)
	{
		gui_event = (GUI_Event*)usrdata;
		gui_msg = (GxMessage*)gui_event->msg.service_msg;
	}
	switch(gui_msg->msg_id)
	{
		case APPMSG_MESSAGE:
			msg = app_netapps_get_app_msg(gui_msg);
			break;
		default:
			break;
	}

	return msg;
}

void app_netapps_message_free(netapps_msg_t *msg)
{
	if(msg)
	{
		if(msg->content_str)
		{
			GxCore_Free(msg->content_str);
			msg->content_str = NULL;
		}
		GxCore_Free(msg);
		msg = NULL;
	}
}

#endif

