#ifndef _APP_WEBLET_SERVER_H
#define _APP_WEBLET_SERVER_H

typedef enum{
    SWITCH_PROG,
    SWITCH_GROUP
}WebletUICheck;

typedef struct _WebletUIMsg{
    int type;
    int result;
    WebletUICheck mode;
    unsigned int keycode;
	unsigned int prog_id;
	unsigned int group_dir;
	char *input;
	bool input_confirm;
}WebletUIMsg;

typedef WebletUIMsg AppMsg_WebletUIControl;

enum MSG_WEBLET_CONTROL_STATUS{
	MSG_WEBLET_KEYBOARD_INPUT,
	MSG_WEBLET_KEYBOARD_INPUT_CONFIRM,
	MSG_WEBLET_GET_KEYBOARD_INPUT,
	MSG_WEBLET_PLAY_PROGRAM,
	MSG_WEBLET_UI_CHECK,
	MSG_WEBLET_CHANGE_GROUP,
	MSG_WEBLET_REMOTE,
};
#endif
