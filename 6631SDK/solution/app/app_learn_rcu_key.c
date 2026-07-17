#include "app.h"
#if LEARN_RCU_SUPPORT
#include "app_send_msg.h"
#include "app_key.h"
#include "app_default_params.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_windows.h"

#define PROPMT_MSG_INPUT_KEY  "Press new remote control key to start adapting the new RCU."
#define PROPMT_MSG_RESET_KEY  "Press the blue key to delete the assigned key value of the current RCU."
#define PROPMT_MSG_ERROR_KEY  "The current button has been set, please delete the current button first."
#define PROPMT_MSG_REPEAT_KEY "The current key has been duplicated, please reselect the key!"
#define LEARN_RCU_KEY "LearnRCUkey"

#define GX_PM_LEARN_RCU_FILE_PATH      "/home/gx/learn_rcu/keymap.xml"
#define GX_PM_LEARN_RCU_THEME_PATH     "/home/gx/learn_rcu/theme.xml"
#define GX_PM_LEARN_RCU_DIR      "/home/gx/learn_rcu"
#define LISTVIEW_KEY_LEARN_RCU   "listview_key_learn_rcu"
#define CMB_KEY_LEARN_RCU   "cmb_key_learn_rcu"
#define LEARN_RCU_MAX 3
void app_remote_learn_rcu_result_remove(void);

typedef struct learn_rcu_keyinfo_s
{
    char learn_rcu_key[16];
	int stb_keycode;
}learn_rcu_keyinfo_t;

static learn_rcu_keyinfo_t learn_rcu_key_array[]=
{
    {"NUMBER 1", STBK_1},
    {"NUMBER 2", STBK_2},
    {"NUMBER 3", STBK_3},
    {"NUMBER 4", STBK_4},
    {"NUMBER 5", STBK_5},
    {"NUMBER 6", STBK_6},
    {"NUMBER 7", STBK_7},
    {"NUMBER 8", STBK_8},
    {"NUMBER 9", STBK_9},
    {"NUMBRT 0", STBK_0},
    {"MENU", STBK_MENU},
    {"EXIT", STBK_EXIT},
    {"OK", STBK_OK},
    {"UP", STBK_UP},
    {"DOWN", STBK_DOWN},
    {"LEFT", STBK_LEFT},
    {"RIGHT", STBK_RIGHT},
    {"MUTE", STBK_MUTE},
    {"TV_RADIO", STBK_TV_RADIO},
    {"POWER", STBK_HALT},
    {"RECALL", STBK_RECALL},
    {"SAT", STBK_SAT},
    {"AUDIO", STBK_AUDIO},
    {"QR", STBK_QR},
    {"PAUSE", STBK_PAUSE},
    {"ZOOM", STBK_ZOOM},
 //   {"STB PAUSE", STBK_PAUSE_STB},
    {"INFO", STBK_INFO},
    {"SUBTITLE", STBK_SUBT},
    {"MULTIFEED", STBK_MUL_PIC},
    {"TTX", STBK_TTX},
    {"EPG", STBK_EPG},
    {"FAV", STBK_FAV},
    {"REC", STBK_REC_START},
    {"STOP", STBK_REC_STOP},
    {"PAGE_UP", STBK_PAGE_UP},
    {"PAGE_DOWN", STBK_PAGE_DOWN},
    {"F1", STBK_F1},
    {"CH +", STBK_CH_UP},
    {"CH -", STBK_CH_DOWN},
    {"REPEAT", STBK_REPEAT},
    {"TIMER", STBK_TIMER},
    {"ASPECT", STBK_ASPECT},
//    {"CHANNEL EDIT", STBK_CH_EDIT},
//    {"SLEEP", STBK_SLEEP},
    {"VOLUME_UP", STBK_VOLUP},
    {"VOLUME_DOWN", STBK_VOLDOWN},
    {"TV", STBK_TV},
    {"RADIO", STBK_RADIO},
//    {"ARROW LEFT", STBK_ARROW_LEFT},
//    {"ARROW RIGHT", STBK_ARROW_RIGHT},
    {"FIND", STBK_FIND},
    {"DISK INFO", STBK_DISK_INFO},
//   {"SYSTEM INFO", STBK_SYS_INFO},
//   {"PAUSE PLAY", STBK_PAUSE_PLAY},
    {"TIMESHIFT", STBK_TMS},
    {"P/N", STBK_PN},
    {"MEDIA", STBK_MEDIA},
    {"PREV", STBK_LAST},
    {"NEXT", STBK_NEXT},
    {"FB", STBK_FB},
    {"FF", STBK_FF},
    {"PLAY", STBK_PLAY},
    {"SEEK", APPK_SEEK},
    {"RED", STBK_RED},
    {"BLUE", STBK_BLUE},
    {"GREEN", STBK_GREEN},
    {"YELLOW", STBK_YELLOW},
};

#define LEARN_RCU_KEY_NUM (sizeof(learn_rcu_key_array)/sizeof(learn_rcu_keyinfo_t))

typedef struct remote_key_info_s
{
    char gui_key_name[16];
	int stb_keycode;
    unsigned int scancode;
    bool flag;
    bool repeat_flag;
}remote_key_info_s;

typedef struct remote_key_list
{
    int key_num;
    remote_key_info_s remote_key_info[LEARN_RCU_KEY_NUM];

}remote_key_list;

static remote_key_list* s_remote_list = NULL;
static remote_key_list* s_remote_backups = NULL;
static int s_learn_remote = 0;

static int remote_key_list_add(remote_key_list* list, unsigned int index, unsigned int scancode)
{
    int i = 0;
    if(list == NULL)
        return -3;

    if(index > (LEARN_RCU_KEY_NUM-1))
        return -4;

    if(list->remote_key_info[index].flag == true)
        return -2;

    if(list->key_num != 0)
    {
        for(i = 0; i <  LEARN_RCU_KEY_NUM; i++)
        {
            if(list->remote_key_info[i].flag == false)
                continue;

            if(i == index)
                continue;

            if(scancode == list->remote_key_info[i].scancode)
            {
                if(list->remote_key_info[index].stb_keycode == STBK_RED
                        || list->remote_key_info[index].stb_keycode == STBK_BLUE
                        || list->remote_key_info[index].stb_keycode == STBK_GREEN
                        || list->remote_key_info[index].stb_keycode == STBK_YELLOW)
                {
                    if(list->remote_key_info[i].stb_keycode == STBK_RED
                            || list->remote_key_info[i].stb_keycode == STBK_BLUE
                            || list->remote_key_info[i].stb_keycode == STBK_GREEN
                            || list->remote_key_info[i].stb_keycode == STBK_YELLOW)
                    {
                        return -1;
                    }
                    else
                    {
                        list->remote_key_info[index].repeat_flag = true;
                        break;
                    }
                }
                else
                {
                    if(list->remote_key_info[i].stb_keycode == STBK_RED
                            || list->remote_key_info[i].stb_keycode == STBK_BLUE
                            || list->remote_key_info[i].stb_keycode == STBK_GREEN
                            || list->remote_key_info[i].stb_keycode == STBK_YELLOW)
                    {
                        list->remote_key_info[i].repeat_flag = true;
                        break;
                    }
                    else
                    {
                        return -1;
                    }
                }
            }
        }
    }
    list->key_num++;
    list->remote_key_info[index].scancode = scancode;
    list->remote_key_info[index].flag = true;
    return 0;
}

static int remote_key_list_del(remote_key_list* list, unsigned int index)
{
    if(list == NULL)
        return -1;

    if(index > (LEARN_RCU_KEY_NUM-1))
        return -1;


    if(list->remote_key_info[index].flag == false)
        return 0;

    if(list->remote_key_info[index].flag == true)
    {
        list->remote_key_info[index].scancode = 0x00;
        list->key_num--;
        list->remote_key_info[index].flag = false;
        list->remote_key_info[index].repeat_flag = false;
    }
    return 0;
}

static int remote_key_list_clear(remote_key_list* list)
{
    int i = 0;
    if(list == NULL)
        return -1;

    for(i = 0; i <  LEARN_RCU_KEY_NUM; i++)
    {
        if(list->remote_key_info[i].flag == false)
            continue;

        list->remote_key_info[i].scancode = 0x00;
        list->remote_key_info[i].flag = false;
        list->remote_key_info[i].repeat_flag = false;
    }

    list->key_num = 0;
    return 0;
}

static bool learn_rcu_check_exist(void)
{
    int i = 0;
    for(i = 0; i <  LEARN_RCU_MAX; i++)
    {
        if(s_remote_list[i].key_num != 0)
            return true;
    }
    return false;
}

static int remote_key_list_save(void)
{
    FILE *fp;
    FILE *file;
    int n = 1;
    int i = 0;
    int j = 0;
    if(learn_rcu_check_exist() == false)
    {
        app_remote_learn_rcu_result_remove();
        return 0;
    }
    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(GX_PM_LEARN_RCU_DIR))
    {
        if(GxCore_Mkdir(GX_PM_LEARN_RCU_DIR) < 0)
        {
            app_log_error("create learn rcu dir failed\n");
        }
        fp = fopen(GX_PM_LEARN_RCU_THEME_PATH, "w");
        if (fp == NULL) {
            app_log_min("Error opening file.\n");
            return -1;
        }
        fprintf(fp, "<?xml version=\"1.0\" encoding=\"GB2312\" standalone=\"no\"?>\n");
        fprintf(fp, "<config>\n");
        fprintf(fp, "    <file_keymap>keymap.xml</file_keymap>\n");
        fprintf(fp, "</config>\n");
        fclose(fp);
    }
    file = fopen(GX_PM_LEARN_RCU_FILE_PATH, "w");

    if (file == NULL) {
        app_log_min("Error opening file.\n");
        return -1;
    }

    fprintf(file, "<?xml version=\"1.0\" encoding=\"GB2312\" standalone=\"no\"?>\n");
    fprintf(file, "<keymap>\n");
    fprintf(file, "     <tree>\n");
    fprintf(file, "         <item display=\"Remote Key map\">\n");
    for(j = 0; j < LEARN_RCU_MAX; j++)
    {
        if(s_remote_list[j].key_num != 0)
        {
            fprintf(file, "             <item display=\"Remote %d\" group=\"public%d\"/>\n",n,j+1);
            n++;
        }
    }
    fprintf(file, "         </item>\n");
    fprintf(file, "     </tree>\n");
    j = 0;
    for(j = 0; j < LEARN_RCU_MAX; j++)
    {
        if(s_remote_list[j].key_num != 0)
        {
            n = 0;
            fprintf(file, "     <group name=\"public%d\">\n",j+1);
            for(i = 0; i < LEARN_RCU_KEY_NUM; i++ )
            {
                if(s_remote_list[j].remote_key_info[i].flag == true)
                {
                    if((s_remote_list[j].remote_key_info[i].repeat_flag == true)&&
                            ((learn_rcu_key_array[i].stb_keycode == STBK_RED)
                            || (learn_rcu_key_array[i].stb_keycode == STBK_BLUE)
                            || (learn_rcu_key_array[i].stb_keycode == STBK_GREEN)
                            || (learn_rcu_key_array[i].stb_keycode == STBK_YELLOW)))
                    {
                        fprintf(file, "     <virtualkey name=\"%s\">0x%x</virtualkey>\n",s_remote_list[j].remote_key_info[i].gui_key_name,s_remote_list[j].remote_key_info[i].scancode);
                    }
                    else
                    {
                        fprintf(file, "     <key name=\"%s\">0x%x</key>\n",s_remote_list[j].remote_key_info[i].gui_key_name,s_remote_list[j].remote_key_info[i].scancode);
                    }
                    n++;
                }
                if(n == s_remote_list[j].key_num)
                    break;;

            }
            fprintf(file, "     </group>\n");
        }
    }
    fprintf(file, "</keymap>\n");

    fclose(file);
    return 0;
}

int remote_key_parse_line(char* line, remote_key_list* list) {
    int i = 0;
    char *start = strstr(line, "\"") + 1;
    char *end = strstr(start, "\"");
    char key[16];
    unsigned int hexValue;
    strncpy(key, start, end - start);
    key[end - start] = '\0';

    start = strstr(end, ">") + 1;
    end = strstr(start, "<");
    char value[10];
    strncpy(value, start, end - start);
    value[end - start] = '\0';
    hexValue = strtol(value, NULL, 16);

    for(i = 0; i <  LEARN_RCU_KEY_NUM; i++)
    {
       if(strcmp(list->remote_key_info[i].gui_key_name,key) == 0)
        {
            list->remote_key_info[i].scancode = hexValue;
            list->remote_key_info[i].flag = true;
            list->key_num++;
            break;;
        }
    }
    return 0;
}

int extract_and_convert_to_int(const char *str) {
    const char *start = NULL;
    for (; *str; ++str) {
        if (isdigit((unsigned char)*str)) {
            if (start == NULL) {
                start = str;
            }
        } else if (start != NULL) {
            break;
        }
    }

    if (start != NULL) {
        return atoi(start);
    }
    return 0;
}

static int remote_key_list_get(void)
{
    FILE *file;
    char line[256];
    bool list_flag = false;
    int d = 0;

    file = fopen(GX_PM_LEARN_RCU_FILE_PATH, "r");
    if (file == NULL) {
        app_log_error("can't open the file.\n");
        return 1;
    }

    while (fgets(line, sizeof(line), file)) {
        if(strstr(line, "name=\"public") != NULL)
        {
            d = extract_and_convert_to_int(line);
            if(d == 0)
            {
                 app_log_error("the file is error.\n");
            }
            list_flag = true;
            continue;
        }
        if(list_flag == true)
        {
            if(strcmp(line, "     </group>\n") == 0)
            {
                list_flag = false;
                continue;
            }
            remote_key_parse_line(line,&s_remote_list[d-1]);
            continue;
        }
    }

    fclose(file);

    return 0;
}

static void remote_key_list_init(remote_key_list* list)
{
    int i = 0;
    list->key_num = 0;
    for(i = 0; i <  LEARN_RCU_KEY_NUM; i++)
    {
        list->remote_key_info[i].stb_keycode = learn_rcu_key_array[i].stb_keycode;
        strcpy(list->remote_key_info[i].gui_key_name, GUI_GetNameByKey(learn_rcu_key_array[i].stb_keycode));
    }
}

static int _learn_rcu_init(void)
{
    int i = 0;
    s_remote_list = (remote_key_list *)GxCore_Mallocz(LEARN_RCU_MAX * sizeof(remote_key_list));
    if(s_remote_list == NULL)
    {
        app_log_error("malloc s_remote_list faild!!!!!!!!!!\n");
        return -1;
    }
    s_remote_backups = GxCore_Mallocz(sizeof(remote_key_list));
    if(s_remote_backups == NULL)
    {
        app_log_error("malloc s_remote_backups faild!!!!!!!!!!\n");
        GxCore_Free(s_remote_list);
        return -1;
    }
    for(i = 0; i <  LEARN_RCU_MAX; i++)
    {
        remote_key_list_init(&s_remote_list[i]);
    }
    remote_key_list_get();
    return 0;
}

static int _learn_rcu_destroy(void)
{
    GxCore_Free(s_remote_list);
    GxCore_Free(s_remote_backups);
    s_remote_list = NULL;
    s_remote_backups = NULL;
    return 0;
}

static void _learn_rcu_cmb_remote_rebuild(void)
{
#define BUFFER_LENGTH   (32)
    int i = 0;
    bool first_flag = false;
    char buffer[BUFFER_LENGTH] = {0};
    char *buffer_all = NULL;
    buffer_all = GxCore_Mallocz(LEARN_RCU_MAX * BUFFER_LENGTH);
    if(buffer_all == NULL)
    {
        app_log_error("_learn_rcu_cmb_remote_rebuild, malloc failed...%s,%d\n",__FILE__,__LINE__);
        return;
    }
    sprintf(buffer_all,"[");
    for(i = 0; i < LEARN_RCU_MAX; i++)
    {
        if(first_flag == false)
        {
            sprintf(buffer,"(%d/%d)New remote control %d", i+1, LEARN_RCU_MAX, i+1);
            strcat(buffer_all, buffer);
            first_flag = true;
        }
        else
        {
            memset(buffer, 0, BUFFER_LENGTH);
            sprintf(buffer,",(%d/%d)New remote control %d", i+1, LEARN_RCU_MAX, i+1);
            strcat(buffer_all, buffer);
        }
    }

    strcat(buffer_all, "]");
    GUI_SetProperty(CMB_KEY_LEARN_RCU,"content",buffer_all);
    GxCore_Free(buffer_all);
    return;
}

static void _learn_rcu_reset_tip_display(bool flag)
{
#define IMG_BLUE    "img_key_learn_rcu_blue"
#define TXT_BLUE        "text_key_learn_rcu_blue"
    if(flag)
    {
        GUI_SetProperty(IMG_BLUE, "state", "show");
        GUI_SetProperty(TXT_BLUE, "state", "show");
    }
    else
    {
        GUI_SetProperty(IMG_BLUE, "state", "hide");
        GUI_SetProperty(TXT_BLUE, "state", "hide");
    }
}

static void _learn_rcu_show_propmt_msg(char*  propmtmsg)
{
#define TXT_LERARN_RCU        "text_key_learn_rcu_start"
    GUI_SetProperty(TXT_LERARN_RCU, "string", propmtmsg);
}

void app_remote_learn_rcu_key(GUI_Event* event)
{
    const char *focus_widget = NULL;
    uint32_t sel = 0;
    int ret = 0;
    if(event->type == GUI_KEYUP)
    {
        focus_widget = GUI_GetFocusWidget();
        if(focus_widget && 0 == strcmp(focus_widget, LISTVIEW_KEY_LEARN_RCU))
        {
            GUI_GetProperty(LISTVIEW_KEY_LEARN_RCU, "select", &sel);
            ret = remote_key_list_add(&s_remote_list[s_learn_remote],sel,event->key.scancode);
            if(ret == -2)
            {
                _learn_rcu_show_propmt_msg(PROPMT_MSG_ERROR_KEY);
            }
            if(ret == -1)
            {
                _learn_rcu_show_propmt_msg(PROPMT_MSG_REPEAT_KEY);
            }
            if(ret == 0)
            {
                _learn_rcu_reset_tip_display(true);
                _learn_rcu_show_propmt_msg(PROPMT_MSG_RESET_KEY);
            }
            GUI_SetProperty(LISTVIEW_KEY_LEARN_RCU,"update_all",NULL);
        }
    }
    return;
}

static int _learn_rcu_result_change_check(int index)
{
    if(memcmp(&s_remote_list[index],s_remote_backups,sizeof(remote_key_list)) == 0)
        return 0;
    else
        return 1;
}

static bool _learn_rcu_get_cur_flag(int index,int sel)
{
    return s_remote_list[index].remote_key_info[sel].flag;
}

static void _learn_rcu_result_backup(int index)
{
    memset(s_remote_backups, 0, sizeof(remote_key_list));
    memcpy(s_remote_backups,&s_remote_list[index],sizeof(remote_key_list));
    return;
}

static void _learn_rcu_reset_smote(int index)
{
    memset(&s_remote_list[index], 0, sizeof(remote_key_list));
    memcpy(&s_remote_list[index], s_remote_backups,sizeof(remote_key_list));
    return;
}

void app_remote_learn_rcu_result_init(void)
{
    if(GXCORE_FILE_EXIST == GxCore_FileExists(GX_PM_LEARN_RCU_DIR))
        GUI_Load(LEARN_RCU_KEY, GX_PM_LEARN_RCU_THEME_PATH);
    return;
}

void app_remote_learn_rcu_result_remove(void)
{
    if(GXCORE_FILE_EXIST == GxCore_FileExists(GX_PM_LEARN_RCU_DIR))
        GxCore_FileDelete(GX_PM_LEARN_RCU_DIR);
    return;
}

static int _learn_rcu_clear_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        remote_key_list_clear(&s_remote_list[s_learn_remote]);
        GUI_SetProperty(LISTVIEW_KEY_LEARN_RCU,"update_all",NULL);
    }
    return 0;
}

static int _learn_rcu_left_cb(PopDlgRet ret)
{
    uint32_t sel = 0;
    GUI_GetProperty(CMB_KEY_LEARN_RCU, "select", &sel);
    if(POP_VAL_OK == ret)
    {
        remote_key_list_save();
        GUI_ImmediateUnload(LEARN_RCU_KEY);
        if(GXCORE_FILE_EXIST == GxCore_FileExists(GX_PM_LEARN_RCU_DIR))
            GUI_Load(LEARN_RCU_KEY, GX_PM_LEARN_RCU_THEME_PATH);
    }
    else
    {
        _learn_rcu_reset_smote(s_learn_remote);
    }
    if(sel == 0)
        sel = LEARN_RCU_MAX-1;
    else
        sel--;
    GUI_SetProperty(CMB_KEY_LEARN_RCU, "select", &sel);
    return 0;
}

static int _learn_rcu_right_cb(PopDlgRet ret)
{
    uint32_t sel = 0;
    GUI_GetProperty(CMB_KEY_LEARN_RCU, "select", &sel);
    if(POP_VAL_OK == ret)
    {
        remote_key_list_save();
        GUI_ImmediateUnload(LEARN_RCU_KEY);
        if(GXCORE_FILE_EXIST == GxCore_FileExists(GX_PM_LEARN_RCU_DIR))
            GUI_Load(LEARN_RCU_KEY, GX_PM_LEARN_RCU_THEME_PATH);
    }
    else
    {
        _learn_rcu_reset_smote(s_learn_remote);
    }
    if(sel == (LEARN_RCU_MAX-1))
        sel = 0;
    else
        sel++;
    GUI_SetProperty(CMB_KEY_LEARN_RCU, "select", &sel);
    return 0;
}

static int _learn_rcu_save_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        remote_key_list_save();
        GUI_ImmediateUnload(LEARN_RCU_KEY);
        if(GXCORE_FILE_EXIST == GxCore_FileExists(GX_PM_LEARN_RCU_DIR))
            GUI_Load(LEARN_RCU_KEY, GX_PM_LEARN_RCU_THEME_PATH);
        _learn_rcu_result_backup(s_learn_remote);
    }
    return 0;
}

static int _learn_rcu_exit_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        remote_key_list_save();
        GUI_ImmediateUnload(LEARN_RCU_KEY);
        if(GXCORE_FILE_EXIST == GxCore_FileExists(GX_PM_LEARN_RCU_DIR))
            GUI_Load(LEARN_RCU_KEY, GX_PM_LEARN_RCU_THEME_PATH);
    }
    GUI_EndDialog(WND_LEARN_RCU_KEY);
    return 0;
}

static void _learn_rcu_save_jump_popdlg(ExitCb jump_cb)
{
    PopDlg pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_YES_NO;
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    pop.exit_cb = jump_cb;
    pop.str = STR_ID_SAVE_INFO;
    popdlg_create(&pop);
}

SIGNAL_HANDLER int app_key_learn_rcu_list_get_total(GuiWidget *widget, void *usrdata)
{
    return LEARN_RCU_KEY_NUM;
}

SIGNAL_HANDLER int app_key_learn_rcu_list_get_data(GuiWidget *widget, void *usrdata)
{
#define MAX_LEN 30
    ListItemPara* item = NULL;
    static char buffer1[MAX_LEN];
    char key_buffer[64] = {0};

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return GXCORE_ERROR;
    if(0 > item->sel)
        return GXCORE_ERROR;

    item->x_offset = 0;
    item->image = NULL;
    memset(buffer1, 0, MAX_LEN);
    sprintf(buffer1, " %d", (item->sel+1));
    item->string = buffer1;

    item = item->next;
    item->image = NULL;
    memset(key_buffer, 0, sizeof(key_buffer));
	sprintf(key_buffer,"%s",learn_rcu_key_array[item->sel].learn_rcu_key);
    item->string = (char *)learn_rcu_key_array[item->sel].learn_rcu_key;

    item = item->next;
    item->x_offset = 0;
    item->string = NULL;
    if(_learn_rcu_get_cur_flag(s_learn_remote,item->sel) == true)
        item->image = "s_choice";
    else
        item->image = NULL;
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_key_learn_rcu_list_change(GuiWidget *widget, void *usrdata)
{
    uint32_t sel=0;
    GUI_GetProperty(LISTVIEW_KEY_LEARN_RCU, "select", &sel);
    if(_learn_rcu_get_cur_flag(s_learn_remote,sel) == false)
    {
        _learn_rcu_reset_tip_display(false);
        _learn_rcu_show_propmt_msg(PROPMT_MSG_INPUT_KEY);
    }
    else
    {
        _learn_rcu_reset_tip_display(true);
        _learn_rcu_show_propmt_msg(PROPMT_MSG_RESET_KEY);
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_key_learn_rcu_cmb_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;
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
                case STBK_DOWN:
                    break;
                case STBK_LEFT:
                    if(_learn_rcu_result_change_check(s_learn_remote) == 0)
                    {
                        break;
                    }
                    _learn_rcu_save_jump_popdlg(_learn_rcu_left_cb);
                    ret = EVENT_TRANSFER_STOP;
                    break;

                case STBK_RIGHT:
                    if(_learn_rcu_result_change_check(s_learn_remote) == 0)
                    {
                        break;
                    }
                    _learn_rcu_save_jump_popdlg(_learn_rcu_right_cb);
                    ret = EVENT_TRANSFER_STOP;
                    break;

                default:
                    break;
            }
        default:
            break;
    }
    return ret;
}

SIGNAL_HANDLER int app_key_learn_rcu_cmb_change(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
    GUI_GetProperty(CMB_KEY_LEARN_RCU, "select", &sel);
    s_learn_remote = sel;
    _learn_rcu_result_backup(s_learn_remote);
    GUI_SetProperty(LISTVIEW_KEY_LEARN_RCU,"select",&cur_sel);
    GUI_SetProperty(LISTVIEW_KEY_LEARN_RCU,"update_all",NULL);
    if(_learn_rcu_get_cur_flag(s_learn_remote,sel) == false)
    {
        _learn_rcu_reset_tip_display(false);
        _learn_rcu_show_propmt_msg(PROPMT_MSG_INPUT_KEY);
    }
    else
    {
        _learn_rcu_reset_tip_display(true);
        _learn_rcu_show_propmt_msg(PROPMT_MSG_RESET_KEY);
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_key_learn_rcu_list_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;

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
                case STBK_DOWN:
                    break;
                case STBK_LEFT:
                case STBK_RIGHT:
                    GUI_SendEvent(CMB_KEY_LEARN_RCU, event);
                    break;
                default:
                    break;
            }
        default:
            break;
    }
    return ret;
}

SIGNAL_HANDLER  int app_key_learn_rcu_create(const char* widgetname, void *usrdata)
{
    s_learn_remote = 0;
    _learn_rcu_init();
    _learn_rcu_result_backup(s_learn_remote);
    _learn_rcu_cmb_remote_rebuild();
    uint32_t  cur_sel = 0;
    GUI_SetFocusWidget(LISTVIEW_KEY_LEARN_RCU);
    GUI_SetProperty(LISTVIEW_KEY_LEARN_RCU, "select", &cur_sel);
    if(_learn_rcu_get_cur_flag(s_learn_remote,cur_sel) == false)
    {
        _learn_rcu_reset_tip_display(false);
        _learn_rcu_show_propmt_msg(PROPMT_MSG_INPUT_KEY);
    }
    else
    {
        _learn_rcu_reset_tip_display(true);
        _learn_rcu_show_propmt_msg(PROPMT_MSG_RESET_KEY);
    }
    GUI_SetProperty(LISTVIEW_KEY_LEARN_RCU, "update_all", NULL);
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int app_key_learn_rcu_destroy(const char* widgetname, void *usrdata)
{
    _learn_rcu_destroy();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int app_key_learn_rcu_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;
	event = (GUI_Event *)usrdata;
    const char *focus_widget = NULL;
    uint32_t sel = 0;
	if(event->type == GUI_KEYDOWN)
	{
		switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
		{
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;
            case STBK_MENU:
			case STBK_EXIT:
                {
                    if(_learn_rcu_result_change_check(s_learn_remote) == 0)
                    {
                        GUI_EndDialog(WND_LEARN_RCU_KEY);
                        break;
                    }
                    _learn_rcu_save_jump_popdlg(_learn_rcu_exit_cb);
                }
				break;
            case STBK_BLUE:
                {
                    focus_widget = GUI_GetFocusWidget();
                    if(focus_widget && 0 == strcmp(focus_widget, LISTVIEW_KEY_LEARN_RCU))
                    {
                        GUI_GetProperty(LISTVIEW_KEY_LEARN_RCU, "select", &sel);
                        remote_key_list_del(&s_remote_list[s_learn_remote],sel);
                        _learn_rcu_show_propmt_msg(PROPMT_MSG_INPUT_KEY);
                        GUI_SetProperty(LISTVIEW_KEY_LEARN_RCU,"update_all",NULL);
                    }
                }
                break;
            case STBK_GREEN:
                {
                    PopDlg  pop;
                    memset(&pop, 0, sizeof(PopDlg));
                    pop.type = POP_TYPE_YES_NO;
                    pop.mode = POP_MODE_UNBLOCK;
                    pop.str = STR_ID_DEL_ALL;
                    pop.exit_cb = _learn_rcu_clear_cb;
                    popdlg_create(&pop);
                }
                break;
            case STBK_RED:
                {
                    _learn_rcu_save_jump_popdlg(_learn_rcu_save_cb);
                }
                break;
		}
	}

	return EVENT_TRANSFER_STOP;
}

void app_create_learn_new_rcu_menu(void)
{
    GUI_CreateDialog(WND_LEARN_RCU_KEY);
    return;
}

#endif
