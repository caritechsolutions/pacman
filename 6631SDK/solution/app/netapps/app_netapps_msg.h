#ifndef __APP_NETAPPS_MSG_H__
#define __APP_NETAPPS_MSG_H__

typedef enum
{
	//private
	NETAPPS_MSG_PRIVATE = 0,

	//search
	NETAPPS_MSG_SEARCH = 100,

	//category
	NETAPPS_MSG_CATEGORY_NUM = 200,
	NETAPPS_MSG_CATEGORY_NEXT = 201,
	NETAPPS_MSG_CATEGORY_PREV = 202,
	NETAPPS_MSG_CATEGORY_LAST = 203,
	NETAPPS_MSG_CATEGORY_NAME = 204,

	//group
	NETAPPS_MSG_GROUP_NEXT = 300,
	NETAPPS_MSG_GROUP_PREV = 301,
	NETAPPS_MSG_GROUP_NAME = 302,

	//page
	NETAPPS_MSG_PAGE_NUM = 400,
	NETAPPS_MSG_PAGE_NEXT = 401,
	NETAPPS_MSG_PAGE_PREV = 402,
	NETAPPS_MSG_PAGE_LAST = 403,

	//play
	NETAPPS_MSG_PLAY_NUM = 500,
	NETAPPS_MSG_PLAY_NEXT = 501,
	NETAPPS_MSG_PLAY_PREV = 502,
	NETAPPS_MSG_PLAY_LAST = 503,

	//setting
	NETAPPS_MSG_SETTING = 600,

	//user
	NETAPPS_MSG_USER_LOGIN = 700,
	NETAPPS_MSG_USER_LOGOUT = 701,
}netapps_msg_id;

typedef struct netapps_msg_s{
	int msg_id;
	int content_int;
	char *content_str;
}netapps_msg_t;

netapps_msg_t *app_netapps_message_get(void *usrdata);

void app_netapps_message_free(netapps_msg_t *msg);

#endif
