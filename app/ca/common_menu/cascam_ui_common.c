#include "app.h"
#if CASCAM_SUPPORT
#include "cascam_ui_porting.h"
#include "cascam_ui_common.h"
#include "cascam.h"

#if CASCAM_UI_MULTI_LAYER
static handle_t s_mutex = 0;
static uint8_t s_win_flag = 0;// 0, initial; 1, has win; 2, no win
extern int32_t app_cascam_osd_send_message(uint32_t sub_id, void *para);

uint8_t cascam_ui_get_win_flag(void)
{
	uint8_t flag = 0;
	
	GxCore_MutexLock(s_mutex);
	flag = s_win_flag;
	GxCore_MutexUnlock(s_mutex);
	return flag;
}

void cascam_ui_set_win_flag(uint8_t flag)
{
	GxCore_MutexLock(s_mutex);
	s_win_flag = flag;
	GxCore_MutexUnlock(s_mutex);
}

int32_t cascam_ui_check_gui_dialog(void* p)
{
	if(NULL == p){
		printf("p = NULL! ----%s, %d----\n", __func__, __LINE__);
		return -1;
	}
	CascamBuffer *data = (CascamBuffer*)p;
	if(GUI_CheckDialog((char*)(data->data)) == GXCORE_SUCCESS)
		cascam_ui_set_win_flag(1);
	else
		cascam_ui_set_win_flag(2);
	GxCore_Free(data->data);
	return 0;
}

int32_t cascam_ui_check_gui_prompt(void* p)
{
	if(NULL == p){
		printf("p = NULL! ----%s, %d----\n", __func__, __LINE__);
		return -1;
	}
	CascamBuffer *data = (CascamBuffer*)p;
	if(GUI_CheckPrompt((char*)(data->data)) == GXCORE_SUCCESS)
		cascam_ui_set_win_flag(1);
	else
		cascam_ui_set_win_flag(2);
	GxCore_Free(data->data);
	return 0;
}

#endif

//this function used for event MSG deal, to check win/prompt exist or not
int32_t cascam_ui_check_window_exist(char* name, CascamWindowType type)
{
#if CASCAM_UI_MULTI_LAYER
	uint32_t sub_id = CASCAM_MSG_CHECK_WIN;
	CascamBuffer *data;
	uint32_t length = 0;
	static uint8_t count = 0;

	if(NULL == name){
		printf("Name Err! ---%s,%d---\n", __func__, __LINE__);
		return -1;
	}

	if(s_mutex == 0)
		GxCore_MutexCreate(&s_mutex);

	if(type == CASCAM_WINDOW_WIN)
		sub_id = CASCAM_MSG_CHECK_WIN;
	else
		sub_id = CASCAM_MSG_CHECK_PROMPT;
		
	data = GxCore_Mallocz(sizeof(CascamBuffer));
	if(NULL == data){
		printf("Malloc Err! ---%s,%d---\n", __func__, __LINE__);
		return -1;
	}
	length = strlen(name);
	data->data = GxCore_Mallocz(length+1);
	if(NULL == data->data){
		printf("Malloc Err! ---%s,%d---\n", __func__, __LINE__);
		return -1;
	}
	memcpy(data->data, name, length);
	data->length = length;
	app_cascam_osd_send_message(sub_id, (void*)data);
	count = 0;
	while(1){
		if(1 == cascam_ui_get_win_flag()){
			cascam_ui_set_win_flag(0);
			return 1;
		}else if(2 == cascam_ui_get_win_flag()){
			cascam_ui_set_win_flag(0);
			return 0;
		}
		if(count>=10)
			break;
		
		count++;
		GxCore_ThreadDelay(200);
	}
#else
	if(GUI_CheckDialog(name) == GXCORE_SUCCESS)
		return 1;
#endif

	return 0;
}

int32_t cascam_ui_send_msg_to_gui(uint32_t sub_id, CascamBuffer* data)
{
#if CASCAM_UI_MULTI_LAYER
	app_cascam_osd_send_message(sub_id, (void*)data);
	return 0;
#else
	return 0;
#endif
}

#endif
