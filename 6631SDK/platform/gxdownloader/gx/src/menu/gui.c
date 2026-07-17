#ifdef NO_OS
#include "io.h"
#endif
#include "config.h"
#include "common.h"
#include "gui.h"
#ifdef NO_OS
#include "gxloader.h"
#endif
#include "bus/gui_core/mini_gui.h"
#include "gxdlp_api.h"
#include "keyboard.h"
#include "string.h"
#ifdef NO_OS
#include "minifs/api_minifs.h"
#include "fat.h"
#else
#include "gxcore_os_filesys.h"
#endif
#include "devapi/gxirr_api.h"
#include "av.h"

#ifdef CONFIG_ENABLE_GUI

#define FILE_MAX_NUM          (100)
#define MAXSIZE               (2048)
#define MAX_LENGTH            (16)

typedef enum{
	GXOTA_TS  ,
	GXOTA_USB ,
	GXOTA_NET ,
	GXOTA_MAX
}gxota_upgrade_Type;
typedef struct GxOTA_ProcessCtrl
{
	int process;
	char *processwidgetname;
	char *labelwidgetname;
} GxOTA_ProcessInfo;

typedef struct GxOTA_TextCtrl
{
	char *widgetname;
	char *string;
}GxOTA_TextInfo ;
typedef struct GxOTA_TsText
{
	char *widgetname;
	char *string;
}GxOTA_TsTextInfo;

typedef struct
{
	int process[2];
	int text[2];
} render_info;
static char *textinfo[3][20] =
{
		{
			"win_ts_title_label1",
			"win_ts_title_label2",
			"win_ts_title_label3",
			"win_ts_title_label4",
			"win_ts_title_label5",
			"win_ts_title_label6",
			"win_ts_title_label7",
			"win_ts_title_label8",
			"win_ts_title_label9"
		},
		{
			"win_usb_title_label1",
			"win_usb_title_label2",
			"win_usb_title_label3"
		},
		{
			"win_net_title_label1",
			"win_net_title_label2",
			"win_net_title_label3"
		}
};
static char* ts_inter_mode[2][20] =
{
	{
		STR_CN_GXOTA_DEMOD_TYPE               ,
		STR_CN_GXOTA_TUNER_TYPE               ,
		STR_CN_GXOTA_FRE                      ,
		STR_CN_GXOTA_SYM                      ,
		STR_CN_GXOTA_PID                      ,
		STR_CN_GXOTA_TABLEID                  ,
		STR_CN_GXOTA_OK_START                 ,
		STR_CN_GXOTA_UP_DOWN                  ,
		STR_CN_GXOTA_LEFT_RIGHT               ,
		STR_CN_GXOTA_FILE_NAME
	},
	{
		STR_EN_GXOTA_DEMOD_TYPE               ,
		STR_EN_GXOTA_TUNER_TYPE               ,
		STR_EN_GXOTA_FRE                      ,
		STR_EN_GXOTA_SYM                      ,
		STR_EN_GXOTA_PID                      ,
		STR_EN_GXOTA_TABLEID                  ,
		STR_EN_GXOTA_OK_START                 ,
		STR_EN_GXOTA_UP_DOWN                  ,
		STR_EN_GXOTA_LEFT_RIGHT               ,
		STR_EN_GXOTA_FILE_NAME
	}
};
static char* usb_inter_mode[2][20] =
{
	{
		STR_CN_GXOTA_FILE                ,
	},
	{
		STR_EN_GXOTA_FILE                ,
	}
};
static char* net_inter_mode[2][20] =
{
	{
		STR_CN_GXOTA_FILE_NAME                ,
		STR_CN_GXOTA_IP                ,
	},
	{
		STR_EN_GXOTA_FILE_NAME                ,
		STR_EN_GXOTA_IP                ,
	}
};

typedef struct _UsbListNode{
	int file_num;
	char *column_content[FILE_MAX_NUM];
	char file_size[FILE_MAX_NUM][16];
} UsbListNode;

typedef struct
{
	int pages;
	int last_page_fnum;
	int cur_page;
	UsbListNode Node;
} Dir_info;

Dir_info dir_info;

char* str_hint[2][20] =
{
	{
		STR_CN_GXOTA_STATUS_INIT              ,
		STR_CN_GXOTA_STATUS_FRONTEND_ERROR    ,
		STR_CN_GXOTA_STATUS_FRONTEND_SETUP    ,
		STR_CN_GXOTA_STATUS_FRONTEND_LOCK     ,
		STR_CN_GXOTA_STATUS_DMX_ERROR         ,
		STR_CN_GXOTA_STATUS_DMX_START_FILTER  ,
		STR_CN_GXOTA_STATUS_DMX_DATA_FINISH   ,
		STR_CN_GXOTA_STATUS_WRITE_FLASH       ,
		STR_CN_GXOTA_STATUS_WRITE_FLASH_OK    ,
		STR_CN_GXOTA_STATUS_WRITE_FLASH_FAILED,
		STR_CN_GXOTA_STATUS_UPDATE_FAILED     ,
		STR_CN_GXOTA_DEMOD_TYPE               ,
		STR_CN_GXOTA_TUNER_TYPE               ,
		STR_CN_GXOTA_FRE                      ,
		STR_CN_GXOTA_SYM                      ,
		STR_CN_GXOTA_OK_START                 ,
		STR_CN_GXOTA_UP_DOWN                  ,
		STR_CN_GXOTA_LEFT_RIGHT               ,
		STR_CN_GXOTA_FILE_NAME
	},
	{
		STR_EN_GXOTA_STATUS_INIT              ,
		STR_EN_GXOTA_STATUS_FRONTEND_ERROR    ,
		STR_EN_GXOTA_STATUS_FRONTEND_SETUP    ,
		STR_EN_GXOTA_STATUS_FRONTEND_LOCK     ,
		STR_EN_GXOTA_STATUS_DMX_ERROR         ,
		STR_EN_GXOTA_STATUS_DMX_START_FILTER  ,
		STR_EN_GXOTA_STATUS_DMX_DATA_FINISH   ,
		STR_EN_GXOTA_STATUS_WRITE_FLASH       ,
		STR_EN_GXOTA_STATUS_WRITE_FLASH_OK    ,
		STR_EN_GXOTA_STATUS_WRITE_FLASH_FAILED,
		STR_EN_GXOTA_STATUS_UPDATE_FAILED     ,
		STR_EN_GXOTA_DEMOD_TYPE               ,
		STR_EN_GXOTA_TUNER_TYPE               ,
		STR_EN_GXOTA_FRE                      ,
		STR_EN_GXOTA_SYM                      ,
		STR_EN_GXOTA_OK_START                 ,
		STR_EN_GXOTA_UP_DOWN                  ,
		STR_EN_GXOTA_LEFT_RIGHT               ,
		STR_EN_GXOTA_FILE_NAME
	}
};

render_info gui_handle;

int GxOTA_GUIProcessUpdate(int fd, int percent)
{
	char                    str[10] = {0};
	GxOTA_ProcessInfo*      info = (GxOTA_ProcessInfo*)fd;

	memset(str, 0, strlen(str));
	sprintf(str, "%d%s", percent, "\%");

	minigui_widget_set(info->processwidgetname, "value", str);
	minigui_widget_set(info->labelwidgetname, "string", str);

	minigui_flush();

	return 0;
}

int GxOTA_GUIProcessClose(int fd)
{
	return 0;
}

int GxOTA_GUITextShow(int fd, char* str)
{
	GxOTA_TextInfo*      info = (GxOTA_TextInfo*)fd;

	minigui_widget_set(info->widgetname, "string", str);
	minigui_flush();
	return 0;
}

int GxOTA_GUITextClose(int fd)
{
	return 0;
}

static int gxota_draw_init(void)
{
	GxOTA_ProcessInfo *pctrl1;
	GxOTA_TextInfo  *txtctrl ;
	/**/
	GxOTA_TextInfo     *tctrl1 ;
	GxOTA_ProcessInfo    *pctrl;

	tctrl1 = malloc(sizeof(GxOTA_TextInfo));
	pctrl = malloc(sizeof(GxOTA_ProcessInfo));

	memset(pctrl, 0, sizeof(GxOTA_ProcessInfo));
	memset(tctrl1, 0, sizeof(GxOTA_TextInfo));

	pctrl->processwidgetname= "win_ota_progress1";
	pctrl->labelwidgetname = "win_ota_progress_labe_value1";
	pctrl->process = 0;

	tctrl1->widgetname = "win_ota_title_label1";
	tctrl1->string = "";

	gui_handle.text[0] =(int)tctrl1;
	gui_handle.process[0] = (int)pctrl;

	txtctrl = malloc(sizeof(GxOTA_TextInfo));
	pctrl1 = malloc(sizeof(GxOTA_ProcessInfo));
	memset(txtctrl, 0, sizeof(GxOTA_TextInfo));
	memset(pctrl1, 0, sizeof(GxOTA_ProcessInfo));
	pctrl1->processwidgetname= "win_ota_progress2";
	pctrl1->labelwidgetname = "win_ota_progress_labe_value2";
	pctrl1->process = 10;
	txtctrl->widgetname = "win_ota_title_label2";
	txtctrl->string = "Initing...";

	gui_handle.process[1]  = (int)pctrl1;
	gui_handle.text[1]     = (int)txtctrl;

	return (int)(&gui_handle);
}

int GxOTA_RenderClose(int fd)
{
	return 1;
}

static void gxota_windown_init(void)
{
	struct GxOTA_Msg    msg;
	char tmpstring[100] = {0};
	GxDLP_Language_Type value = 0;

	gxota_draw_init();

	GxDLP_Get_Language(&value);
	if (-1 == value)
	{
		memset(tmpstring, 0, strlen(tmpstring));
		sprintf(tmpstring, "%s", STR_CN_GXOTA_PROCESS);
		minigui_widget_set("win_ota_title_label4", "string", tmpstring);

		memset(tmpstring, 0, strlen(tmpstring));
		sprintf(tmpstring, "%s", STR_CN_GXOTA_OTA);
		minigui_widget_set("win_ota_title_label", "string",  tmpstring);
	}
	else
	{
		if (value == GXDLP_CHINESE)
		{
			memset(tmpstring, 0, strlen(tmpstring));
			sprintf(tmpstring, "%s", STR_CN_GXOTA_PROCESS);
			minigui_widget_set("win_ota_title_label4", "string", tmpstring);

			memset(tmpstring, 0, strlen(tmpstring));
			sprintf(tmpstring, "%s", STR_CN_GXOTA_OTA);
			minigui_widget_set("win_ota_title_label", "string",  tmpstring);
		}

		if (value == GXDLP_ENGLISH)
		{
			minigui_widget_set("win_ota_title_label4", "string", "Process:");
			minigui_widget_set("win_ota_title_label", "string", "OTA Upgrade");
		}
	}

	msg.type = GXOTA_GUI_STATUS;
	msg.msg_id = GXOTA_STATUS_INIT;
	gui_show(1, &msg, NULL);

	msg.type = GXOTA_GUI_PROCESS;
	msg.percent = 0;
	gui_show(1, &msg, NULL);

	return;

}
void gui_show(int processid, const struct GxOTA_Msg* msg, char *rc_string)
{
	static int old_msg_id = -1;
	char tmpstring[100] = {0};
	GxDLP_Language_Type tmplanguage = 0;
	GxDLP_Language_Type lantype = GXDLP_CHINESE;

	switch(processid)
	{
		case 1:
			if (GXOTA_GUI_PROCESS == msg->type)
			{
				GxOTA_GUIProcessUpdate(gui_handle.process[0], msg->percent);
			}
			else if (GXOTA_GUI_STATUS == msg->type)
			{
				memset(tmpstring, 0, strlen(tmpstring));
				if (old_msg_id != msg->msg_id)
				{
					GxDLP_Get_Language(&tmplanguage);
					if (-1 == tmplanguage)
					{
						lantype = GXDLP_CHINESE;
						sprintf(tmpstring, "%s%s", STR_CN_GXOTA_STATUS, str_hint[lantype][msg->msg_id]);
					}
					else
					{
						if (tmplanguage == GXDLP_CHINESE)
						{
							lantype = GXDLP_CHINESE;
							sprintf(tmpstring, "%s%s", STR_CN_GXOTA_STATUS, str_hint[lantype][msg->msg_id]);
						}

						if (tmplanguage == GXDLP_ENGLISH)
						{
							lantype = GXDLP_ENGLISH;
							sprintf(tmpstring, "%s%s", STR_EN_GXOTA_STATUS, str_hint[lantype][msg->msg_id]);
						}
					}
					GxOTA_GUITextShow(gui_handle.text[1], tmpstring);

					old_msg_id = msg->msg_id;
				}
			}
			break;
		case 0:
			if (GXOTA_GUI_PROCESS == msg->type)
			{
				GxOTA_GUIProcessUpdate(gui_handle.process[0], msg->percent);
			}
			else if (GXOTA_GUI_STATUS == msg->type)
			{
				if (NULL != rc_string)
				{
					GxOTA_GUITextShow(gui_handle.text[0], rc_string);
				}
			}
			break;
		default:
			break;
	}

}
void windows_init(gxota_upgrade_Type type)
{
	char tmpstring[30] = {0};
	char tmpnamestr[30] = {0};
	int i = 0;
	int num;
	GxDLP_Language_Type value = 0;
	if(type == GXOTA_TS)
		num = 10;
	else
		num = 3;
	GxDLP_Get_Language(&value);
	for(i = 0; i < num; i++)
	{
		if (-1 == value)
		{
			memset(tmpstring, 0, strlen(tmpstring));
			memset(tmpnamestr, 0, strlen(tmpnamestr));
			if(type == GXOTA_TS)
				sprintf(tmpstring, "%s", ts_inter_mode[GXDLP_CHINESE][i]);
			else if(type == GXOTA_NET)
				sprintf(tmpstring, "%s", net_inter_mode[GXDLP_CHINESE][i]);
			else
				sprintf(tmpstring, "%s", usb_inter_mode[GXDLP_CHINESE][i]);

			sprintf(tmpnamestr, "%s", textinfo[type][i]);
			minigui_widget_set(tmpnamestr, "string", tmpstring);
		}
		else
		{
			if (value == GXDLP_CHINESE)
			{
				memset(tmpstring, 0, strlen(tmpstring));
				memset(tmpnamestr, 0, strlen(tmpnamestr));
				if(type == GXOTA_TS)
					sprintf(tmpstring, "%s", ts_inter_mode[GXDLP_CHINESE][i]);
				else if(type == GXOTA_NET)
					sprintf(tmpstring, "%s", net_inter_mode[GXDLP_CHINESE][i]);
				else
					sprintf(tmpstring, "%s", usb_inter_mode[GXDLP_CHINESE][i]);

				sprintf(tmpnamestr, "%s", textinfo[type][i]);
				minigui_widget_set(tmpnamestr, "string", tmpstring);
			}
			else if (value == GXDLP_ENGLISH)
			{
				memset(tmpstring, 0, strlen(tmpstring));
				memset(tmpnamestr, 0, strlen(tmpnamestr));
				if(type == GXOTA_TS)
					sprintf(tmpstring, "%s", ts_inter_mode[GXDLP_CHINESE][i]);
				else if(type == GXOTA_NET)
					sprintf(tmpstring, "%s", net_inter_mode[GXDLP_CHINESE][i]);
				else
					sprintf(tmpstring, "%s", usb_inter_mode[GXDLP_CHINESE][i]);

				sprintf(tmpnamestr, "%s", textinfo[type][i]);
				minigui_widget_set(tmpnamestr, "string", tmpstring);
			}
		}
	}
}
static GxDLP_Tuner_Type tuner_type;
static GxDLP_Demod_Type demod_type;
static int32_t frequency_t = 850;
static int32_t symbolrate_t = 6875;
static int32_t dem_pid = 0;
static int32_t dem_tableid = 0;
static int8_t select = 0;
static char *file_name = NULL;
void ts_list_set()
{
	char *focus = NULL;
	static int flag = 0;
	char * demod_name = NULL;
	char * tuner_name = NULL;
	char oemValue[20] = {0};
	focus = minigui_widget_get_focus();
	if((focus) && (0 == strcasecmp(focus, "win_ts_upgrade_list")))
	{
		minigui_widget_clear_data("win_ts_upgrade_list");
		if(flag == 0)
		{
			demod_name = get_demodname(demod_type);
			if(GXCORE_ERROR == minigui_widget_add_data("win_ts_upgrade_list", demod_name))
			{
				gxloge("win_ts_upgrade_list err");
			}
			tuner_name = get_tunername(tuner_type);
			if(GXCORE_ERROR == minigui_widget_add_data("win_ts_upgrade_list", tuner_name))
			{
				gxloge("win_ts_upgrade_list err");
			}
			memset(oemValue,0,20);
			frequency_t = GxDLP_Get_FeFrequency();
			sprintf((char *)oemValue,"%d",frequency_t);
			if(GXCORE_ERROR == minigui_widget_add_data("win_ts_upgrade_list", oemValue))
			{
				gxloge("win_ts_upgrade_list err");
			}
			memset(oemValue,0,10);
			symbolrate_t = GxDLP_Get_FeSymbolrate();
			sprintf((char *)oemValue,"%d",symbolrate_t);
			if(GXCORE_ERROR == minigui_widget_add_data("win_ts_upgrade_list", oemValue))
			{
				gxloge("win_ts_upgrade_list err");
			}
			memset(oemValue,0,10);
			dem_pid = GxDLP_Get_DmxPid();
			sprintf((char *)oemValue,"%d",dem_pid);
			if(GXCORE_ERROR == minigui_widget_add_data("win_ts_upgrade_list", oemValue))
			{
				gxloge("win_ts_upgrade_list err");
			}
			memset(oemValue,0,10);
			dem_tableid = GxDLP_Get_DmxTableId();
			sprintf((char *)oemValue,"%d",dem_tableid);
			if(GXCORE_ERROR == minigui_widget_add_data("win_ts_upgrade_list", oemValue))
			{
				gxloge("win_ts_upgrade_list err");
			}
			flag = 1;
		}
		else
		{
			demod_name = get_demodname(demod_type);
			if(GXCORE_ERROR == minigui_widget_add_data("win_ts_upgrade_list", demod_name))
			{
				gxloge("win_ts_upgrade_list err");
			}
			tuner_name = get_tunername(tuner_type);
			if(GXCORE_ERROR == minigui_widget_add_data("win_ts_upgrade_list", tuner_name))
			{
				gxloge("win_ts_upgrade_list err");
			}
			memset(oemValue,0,20);
			sprintf((char *)oemValue,"%d",frequency_t);
			if(GXCORE_ERROR == minigui_widget_add_data("win_ts_upgrade_list", oemValue))
			{
				gxloge("win_ts_upgrade_list err");
			}
			memset(oemValue,0,10);
			sprintf((char *)oemValue,"%d",symbolrate_t);
			if(GXCORE_ERROR == minigui_widget_add_data("win_ts_upgrade_list", oemValue))
			{
				gxloge("win_ts_upgrade_list err");
			}
			memset(oemValue,0,10);
			sprintf((char *)oemValue,"%d",dem_pid);
			if(GXCORE_ERROR == minigui_widget_add_data("win_ts_upgrade_list", oemValue))
			{
				gxloge("win_ts_upgrade_list err");
			}
			memset(oemValue,0,10);
			sprintf((char *)oemValue,"%d",dem_tableid);
			if(GXCORE_ERROR == minigui_widget_add_data("win_ts_upgrade_list", oemValue))
			{
				gxloge("win_ts_upgrade_list err");
			}

		}
		minigui_flush();
	}
}
static void ts_list_get()
{
	char *focus = NULL;
	char *oemValue = NULL;
	focus = minigui_widget_get_focus();
	if((focus) && (0 == strcasecmp(focus, "win_ts_apgrade_list")))
	{
		GxDLP_Set_FeDemodType(demod_type);
		GxDLP_Set_FeTunerType(demod_type);
		oemValue = minigui_widget_get("win_ts_lpgrade_list", "select_content");
		frequency_t = atoi(oemValue);
		GxDLP_Set_FeFrequency(oemValue);
		oemValue = minigui_widget_get("win_ts_wpgrade_list", "select_content");
		symbolrate_t = atoi(oemValue);
		GxDLP_Set_FeSymbolrate(oemValue);
		oemValue = minigui_widget_get("win_ts_rpgrade_list", "select_content");
		dem_pid = atoi(oemValue);
		GxDLP_Set_DmxPid(oemValue);
		oemValue = minigui_widget_get("win_ts_ypgrade_list", "select_content");
		dem_tableid = atoi(oemValue);
		GxDLP_Set_DmxTableId(oemValue);
	}
}

#ifdef NO_OS
static char* get_content(char *pbuf, unsigned long size, int num)
{
	unsigned long i;
	if (num == 0)
	{
		return(pbuf);
	}
	else
	{
		for (i = 0; i < size - 1; i++)
		{
			if (pbuf[i] != '\0')
			{
				continue;
			}
			num--;
			if (num == 0)
				return (pbuf + i + 1);
		}
	}
	return (NULL);
}
#endif

static int get_file(char *path,char *pbuf, Dir_info *dir_info)
{
#ifdef NO_OS
	int i = 0;
	unsigned long pbuf_size = 0;
	unsigned long size = 0;
	char *pfile = NULL;
	char str[4] = {" | "};
	char str1[5] = {"Size"};
	char str2[4] = {"../"};
	char *strCat = NULL;
	static int str_len = 0;

	strCat = (char *)malloc(MAXSIZE);
	if(NULL == strCat)
	{
		return (1);
	}
	memset(strCat, 0, MAXSIZE);
	if(0 != usb_get_dir(path, pbuf, MAXSIZE, &pbuf_size))
	{
		gxlogd("read usb failed\n");
	}
	sprintf(strCat, "%s%s%s", str2, str, str1);
	dir_info->Node.column_content[0] = strdup(strCat);
	dir_info->Node.file_num++;
	while(1)
	{
		pfile = get_content(pbuf, pbuf_size, i++);
		if (NULL == pfile)
			break;
		if(0 != strcmp(pfile, "./") && 0 != strcmp(pfile, "../"))
		{
			if(pfile[strlen(pfile) - 1] != '/')
			{
				file_get_size(pfile, &size);
				sprintf(dir_info->Node.file_size[dir_info->Node.file_num], "%ld", size);
			}
			else
			{
				strcpy(dir_info->Node.file_size[dir_info->Node.file_num],  "--");
			}
			str_len += (strlen(dir_info->Node.column_content[dir_info->Node.file_num - 1]) + 1);
			sprintf(strCat + str_len, "%s%s%s", pfile, str, dir_info->Node.file_size[dir_info->Node.file_num]);
			dir_info->Node.column_content[dir_info->Node.file_num] = strdup(strCat + str_len);
			dir_info->Node.file_num++;
		}
	}
	free(strCat);
	if(dir_info->Node.file_num <= 7)
	{
		dir_info->pages = 0;
		dir_info->last_page_fnum = dir_info->Node.file_num;
	}
	else
	{
		dir_info->pages = (dir_info->Node.file_num - 7) / 6 + 1;
		dir_info->last_page_fnum = (dir_info->Node.file_num - 7) % 6;
		if(dir_info->last_page_fnum == 0)
			dir_info->last_page_fnum = 6;
	}
#endif
	return (0);
}
static void usb_list_set()
{
	int i = 0;
	char *pbuf = NULL;
	char *focus = NULL;

	pbuf = (char *)malloc(MAXSIZE);
	if(NULL == pbuf)
	{
		return;
	}
	memset(pbuf, 0, MAXSIZE);
	focus = minigui_widget_get_focus();
	if((focus) && (0 == strcasecmp(focus, "win_usb_upgrade_list")))
	{
		minigui_widget_clear_data("win_usb_upgrade_list");
		memset(&dir_info, 0, sizeof(dir_info));
		get_file("/", pbuf, &dir_info);
		for(i = 0; i < dir_info.Node.file_num; i++)
		{
			minigui_widget_add_data("win_usb_upgrade_list", dir_info.Node.column_content[i]);
		}

	}
	minigui_flush();
}

static void usb_add_data()
{
	int i= 0, num = 0;
	if(dir_info.cur_page == 0)
	{
		minigui_widget_clear_data("win_usb_upgrade_list");
		for(i = 0; i < dir_info.Node.file_num; i++)
		{
			minigui_widget_add_data("win_usb_upgrade_list", dir_info.Node.column_content[i]);
		}
	}
	else
	{
		num = 7 + (dir_info.cur_page - 1) * 6;
		minigui_widget_clear_data("win_usb_upgrade_list");
		minigui_widget_add_data("win_usb_upgrade_list", " | Size");
		for(i = num; i < dir_info.Node.file_num; i++)
		{
			minigui_widget_add_data("win_usb_upgrade_list", dir_info.Node.column_content[i]);
		}
	}

}

static void key(int8_t upgrade_type);
#ifdef CONFIG_ENABLE_USB_UPGRADE
static void usb_list_get(int8_t upgrade_type)
{
	int i = 0;
	char *focus = NULL;
	char *oemValue = NULL;
	char *pfile = NULL;
	char *str = NULL;
	char *tmp = NULL;
	static char cur_path[1024] = "/";

	pfile = (char *)malloc(MAXSIZE);
	tmp = (char *)malloc(MAXSIZE);
	if(NULL == pfile || tmp == NULL)
	{
		if (pfile != NULL)
			free(pfile);
		if (tmp != NULL)
			free(tmp);
		return;
	}

	memset(pfile, 0, MAXSIZE);
	memset(tmp, 0, MAXSIZE);
	focus = minigui_widget_get_focus();
	if((focus) && (0 == strcasecmp(focus, "win_usb_upgrade_list")))
	{
		oemValue = minigui_widget_get("win_usb_upgrade_list", "select_content");
		if((str = strchr(oemValue, ' ')) != NULL)
		{
			strncpy(tmp, oemValue, strlen(oemValue) - strlen(str));
		}

		if(tmp[strlen(tmp) - 1] != '/')
		{
			GxDLP_Set_UpgradeFileName(tmp);
			for(i = 0; i < dir_info.Node.file_num; i ++)
				free(dir_info.Node.column_content[i]);
		}
		else
		{
			strcat(cur_path, tmp);
			memset(&dir_info, 0, sizeof(dir_info));
			get_file(cur_path, pfile, &(dir_info));
			usb_add_data();
			minigui_flush();
			key(upgrade_type);
		}
	}
	free(pfile);
}
#endif

static char num_char(uint32_t value)
{
	if((value >= 0) && (value <= 9))
	{
		return (value + '0');
	}
	else
	{
		return ('\0');
	}

}

static int cur_pos = 0;
static int count = 0;
static int tem_len = 0;
static char ip_addr[24] = {0};
static char port[8] = {0};

void net_list_set()
{
	char tem_addr[4] = {0};
	char temValue[2] = {0};
	char *focus = NULL;
	focus = minigui_widget_get_focus();
	if((focus) && (0 == strcasecmp(focus, "win_net_upgrade_list")))
	{
		if(count <= 3)
		{
			strncpy(tem_addr, ip_addr, 3);
			minigui_widget_set("win_net_edit_box", "string", tem_addr);
		}
		else if(count > 3 && count <= 6)
		{
			strncpy(tem_addr, ip_addr + 3, 3);
			minigui_widget_set("win_net_edit_box1", "string", tem_addr);
		}
		else if(count > 6 && count <= 9)
		{
			strncpy(tem_addr, ip_addr + 6, 3);
			minigui_widget_set("win_net_edit_box2", "string", tem_addr);
		}
		else if(count > 9 && count <= 12 )
		{
			strncpy(tem_addr, ip_addr + 9, 3);
			minigui_widget_set("win_net_edit_box3", "string", tem_addr);
		}
		else
		{
			minigui_widget_set("win_net_edit_box4", "string", port);
		}

		if(cur_pos < 3)
		{
			sprintf(temValue, "%d", cur_pos);
			minigui_widget_set("win_net_edit_box", "state", "focused");
			minigui_widget_set("win_net_edit_box1", "state", "unfocused");
			minigui_widget_set("win_net_edit_box2", "state", "unfocused");
			minigui_widget_set("win_net_edit_box3", "state", "unfocused");
			minigui_widget_set("win_net_edit_box4", "state", "unfocused");
			minigui_widget_set("win_net_edit_box", "cursor", temValue);
		}
		else if(3 <= cur_pos && cur_pos < 6 )
		{
			sprintf(temValue, "%d", cur_pos - 3);
			minigui_widget_set("win_net_edit_box", "state", "unfocused");
			minigui_widget_set("win_net_edit_box2", "state", "unfocused");
			minigui_widget_set("win_net_edit_box3", "state", "unfocused");
			minigui_widget_set("win_net_edit_box4", "state", "unfocused");
			minigui_widget_set("win_net_edit_box1", "state", "focused");
			minigui_widget_set("win_net_edit_box1", "cursor", temValue);
		}
		else if(6 <= cur_pos && cur_pos < 9)
		{
			sprintf(temValue, "%d", cur_pos - 6);
			minigui_widget_set("win_net_edit_box", "state", "unfocused");
			minigui_widget_set("win_net_edit_box1", "state", "unfocused");
			minigui_widget_set("win_net_edit_box3", "state", "unfocused");
			minigui_widget_set("win_net_edit_box4", "state", "unfocused");
			minigui_widget_set("win_net_edit_box2", "state", "focused");
			minigui_widget_set("win_net_edit_box2", "cursor", temValue);
		}
		else if(9 <= cur_pos && cur_pos < 12)
		{
			sprintf(temValue, "%d", cur_pos - 9);
			minigui_widget_set("win_net_edit_box", "state", "unfocused");
			minigui_widget_set("win_net_edit_box1", "state", "unfocused");
			minigui_widget_set("win_net_edit_box2", "state", "unfocused");
			minigui_widget_set("win_net_edit_box4", "state", "unfocused");
			minigui_widget_set("win_net_edit_box3", "state", "focused");
			minigui_widget_set("win_net_edit_box3", "cursor", temValue);
		}
		else
		{
			sprintf(temValue, "%d", cur_pos - 12);
			minigui_widget_set("win_net_edit_box", "state", "unfocused");
			minigui_widget_set("win_net_edit_box1", "state", "unfocused");
			minigui_widget_set("win_net_edit_box2", "state", "unfocused");
			minigui_widget_set("win_net_edit_box3", "state", "unfocused");
			minigui_widget_set("win_net_edit_box4", "state", "focused");
			minigui_widget_set("win_net_edit_box4", "cursor", temValue);

		}
		minigui_flush();
	}
}

#ifdef CONFIG_ENABLE_USB_UPGRADE
static void net_list_get()
{
	char *focus = NULL;
	char *oemValue = NULL;
	char ip_string[16] = {0};
	int i = 0, j = 0, count = 0;
	focus = minigui_widget_get_focus();
	if((focus) && (0 == strcasecmp(focus, "win_net_upgrade_list")))
	{
		for(i = 0; i < 12; i++)
		{
			count++;
			ip_string[j] = ip_addr[i];
			if(0 == count % 3 && 15 != strlen(ip_string))
			{
				j++;
				ip_string[j] = '.';
				count = 0;
			}
			j++;

		}
		minigui_widget_set("win_net_upgrade_list", "string", ip_string);
		oemValue = minigui_widget_get("win_net_upgrade_list", "string");
		GxDLP_Set_SERVER_IP_ADDR(oemValue);
		minigui_widget_set("win_net_upgrade_list", "string", port);
		oemValue = minigui_widget_get("win_net_upgrade_list", "string");
		GxDLP_Set_TFTP_PORT(oemValue);
	}
}
#endif

static void key_data(uint32_t value)
{
	char *focus = NULL;
	static char tem_value[24] = {0};
	focus = minigui_widget_get_focus();
	if((focus) && (0 == strcasecmp(focus, "win_net_upgrade_list")))
	{
		switch(value)
		{
			case OTAK_0:
			case OTAK_1:
			case OTAK_2:
			case OTAK_3:
			case OTAK_4:
			case OTAK_5:
			case OTAK_6:
			case OTAK_7:
			case OTAK_8:
			case OTAK_9:
				tem_value[cur_pos] = num_char(value);
				strncpy(ip_addr, tem_value, 12);
				strncpy(port, tem_value + 12, 5);
				count = cur_pos + 1;
				tem_len = strlen(tem_value);
				if(cur_pos < (MAX_LENGTH - 1) && tem_len < MAX_LENGTH)
				{
					cur_pos++;
				}
				break;
			default:
				break;
		}
		net_list_set();
	}
}
static void key_updown(uint32_t value)
{
	char oemValue[2] = {0};
	char *focus = NULL;
	focus = minigui_widget_get_focus();
	if((focus) && (0 == strcasecmp(focus, "win_ts_upgrade_list")))
	{
		switch(value)
		{
			case OTAK_UP:
				select = select - 1;
				break;
			case OTAK_DOWN:
				select = select + 1;
				break;
			default:
				break;
		}
		if(select > 5)
			select = 0;
		if(select < 0)
			select = 5;
		memset(oemValue,0,strlen(oemValue));
		sprintf(oemValue,"%d",select);
		minigui_widget_set("win_ts_upgrade_list", "select", oemValue);
		minigui_widget_set("win_ts_upgrade_list", "update", NULL);
		minigui_flush();
	}

	if((focus) && (0 == strcasecmp(focus, "win_usb_upgrade_list")))
	{
		switch(value)
		{
			case OTAK_UP:
				if(dir_info.cur_page == 0)
				{
					if(select > 0)
						select = select - 1;
				}
				else
				{
					if(select > 1)
						select = select - 1;
					else
					{
						dir_info.cur_page--;
						select = 6;
					}
				}
				break;
			case OTAK_DOWN:
				if(dir_info.pages == 0)
				{
					if(select < (dir_info.last_page_fnum - 1))
						select = select + 1;
				}
				else
				{
					if(dir_info.cur_page == dir_info.pages)
					{
						if(select < dir_info.last_page_fnum)
							select = select + 1;
					}
					else
					{
						if(select < 6)
							select = select + 1;
						else
						{
							dir_info.cur_page++;
							select = 1;
						}
					}
				}
				break;
			default:
				break;
		}
		usb_add_data();
		memset(oemValue,0,strlen(oemValue));
		sprintf(oemValue,"%d",select);
		minigui_widget_set("win_usb_upgrade_list", "select", oemValue);
		minigui_widget_set("win_usb_upgrade_list", "update", NULL);
		minigui_flush();
	}
}

static void key_left_right(int32_t value)
{
	char *focus = NULL;
	focus = minigui_widget_get_focus();
	switch(value)
	{
		case OTAK_LEFT:
			if((focus) && (0 == strcasecmp(focus, "win_ts_upgrade_list")))
			{
				switch(select)
				{
					case GXOTA_TS_DEMOD_TYPE:
						if(demod_type == GXDLP_DEMOD_GX1001)
						{
							demod_type = GXDLP_DEMOD_GX1001;
							break;
						}
						demod_type = demod_type - 1;
						break;
					case GXOTA_TS_TUNER_TYPE:
						if(tuner_type == GXDLP_TUNER_AV2018)
						{
							tuner_type = GXDLP_TUNER_AV2018;
							break;
						}
						tuner_type = tuner_type - 1;
						break;
					case GXOTA_TS_FRE:
						frequency_t = frequency_t - 1000000;
						if(frequency_t < 0)
							frequency_t = 0;
						break;
					case GXOTA_TS_SYM:
						symbolrate_t = symbolrate_t - 1000;
						if(symbolrate_t < 0)
							symbolrate_t = 0;
						break;
					case GXOTA_TS_PID:
						dem_pid = dem_pid - 1;
						if(dem_pid < 0)
							dem_pid = 0;
						break;
					case GXOTA_TS_TABLEID:
						dem_tableid = dem_tableid - 1;
						if(dem_tableid < 0)
							dem_tableid = 0;
						break;
				}
				ts_list_set();
			}
			if((focus) && (0 == strcasecmp(focus, "win_usb_upgrade_list")))
			{
				sprintf(file_name, "/recoveryall.rcv");
				usb_list_set();
			}
			if((focus) && (0 == strcasecmp(focus, "win_net_upgrade_list")))
			{
				if (tem_len < MAX_LENGTH && cur_pos == count)
				{
					count = count + 1;
				}
				cur_pos--;
				if(cur_pos < 0)
				{
					if(tem_len < MAX_LENGTH)
						cur_pos = tem_len;
					else
						cur_pos = tem_len - 1;
				}
				count--;
				if(count < 1)
				{
					count = tem_len;
				}
				net_list_set();
			}
			break;
		case OTAK_RIGHT:
			if((focus) && (0 == strcasecmp(focus, "win_ts_upgrade_list")))
			{
				switch(select)
				{
					case GXOTA_TS_DEMOD_TYPE:
						demod_type = demod_type + 1;
						if(demod_type >= GXDLP_DEMOD_MAX)
							demod_type = GXDLP_DEMOD_MAX - 1;
						break;
					case GXOTA_TS_TUNER_TYPE:
						tuner_type = tuner_type + 1;
						if(tuner_type >= GXDLP_TUNER_MAX)
							tuner_type = GXDLP_TUNER_MAX - 1;
						break;
					case GXOTA_TS_FRE:
						frequency_t = frequency_t + 1000000;
						break;
					case GXOTA_TS_SYM:
						symbolrate_t = symbolrate_t + 1000;
						break;
					case GXOTA_TS_PID:
						dem_pid = dem_pid + 1;
						break;
					case GXOTA_TS_TABLEID:
						dem_tableid = dem_tableid + 1;
						break;
				}
				ts_list_set();
			}
			if((focus) && (0 == strcasecmp(focus, "win_usb_upgrade_list")))
			{
				sprintf(file_name, "/recovery.rcv");
				usb_list_set();
			}
			if((focus) && (0 == strcasecmp(focus, "win_net_upgrade_list")))
			{
				cur_pos++;
				if(tem_len < MAX_LENGTH && cur_pos > tem_len)
				{
					cur_pos = 0;
				}
				if(tem_len == MAX_LENGTH && cur_pos > tem_len - 1)
				{
					cur_pos = 0;
				}
				count = cur_pos + 1;
				net_list_set();
			}
			break;
	}
}
static void key(int8_t upgrade_type)
{
	int8_t ret = 0;
	uint32_t code = 0;
	key_type key = 0;
	int flag = 1;
	char *focus = NULL;
	focus = minigui_widget_get_focus();
	while(flag)
	{
		ret = GxIRR_Read(&code, GXIRR_NONBLOCK);
		code = ((code >>16)&0xff00) + ((code >> 8)&0xff);
//		if(ret != -1)
//			gxloge("ota_key_code = %x\n", code);
		key = get_key(code);
		switch(key)
		{
			case OTAK_0:
			case OTAK_1:
			case OTAK_2:
			case OTAK_3:
			case OTAK_4:
			case OTAK_5:
			case OTAK_6:
			case OTAK_7:
			case OTAK_8:
			case OTAK_9:
				key_data(key);
				break;
			case OTAK_UP:
			case OTAK_DOWN:
				key_updown(key);
				break;
			case OTAK_LEFT:
			case OTAK_RIGHT:
				key_left_right(key);
				break;
			case OTAK_OK:
				flag = 0;
				if(upgrade_type == GXDLP_OTA_DDTEX_DOWNLOAD)
				{
					ts_list_get();
					minigui_end_dialog("win_ts_upgrade_list");
					minigui_create_dialog("win_ota_screen");
					minigui_flush();
				}
#ifdef CONFIG_ENABLE_USB_UPGRADE
				if(upgrade_type == GXDLP_USB_DOWNLOAD)
				{
					usb_list_get(upgrade_type);
					minigui_end_dialog("win_usb_list");
					minigui_create_dialog("win_ota_screen");
					minigui_flush();
				}
#endif
#ifdef CONFIG_ENABLE_NET
				if(upgrade_type == GXDLP_NET_TFTP_DOWNLOAD)
				{
					if(tem_len == MAX_LENGTH)
					{
						net_list_get();
						minigui_end_dialog("win_net_upgrade_list");
						minigui_create_dialog("win_ota_screen");
						minigui_flush();
					}
					else
					{
						flag = 1;
						key_data(key);
					}
				}
#endif
				break;
			default:
				break;
		}
	}
}
void showmenu(int8_t upgrade_type)
{
	if(upgrade_type == GXDLP_OTA_DDTEX_DOWNLOAD){
		minigui_create_dialog("win_ts_list");
		minigui_flush();
		key(upgrade_type);
	}else if(upgrade_type == GXDLP_USB_DOWNLOAD){
		minigui_create_dialog("win_usb_list");
		minigui_flush();
		key(upgrade_type);
	}else if(upgrade_type == GXDLP_NET_TFTP_DOWNLOAD){
		minigui_create_dialog("win_net_upgrade_list");
		minigui_flush();
		key(upgrade_type);
	}else{
		minigui_create_dialog("win_ota_screen");
	}
	minigui_flush();
	return;
}
void hidemenu(void)
{
	minigui_end_dialog("win_ota_screen");
	minigui_flush();
}
handle_t win_ota_screen_create(void *usrdata)
{
	gxota_windown_init();
	return 0;
}

handle_t win_ota_screen_destroy(void *usrdata)
{
	return 0;
}
handle_t ts_upgrade_create(void *usrdata)
{
	windows_init(GXOTA_TS);
	minigui_widget_set_focus("win_ts_upgrade_list",TRUE);
	ts_list_set();
	return 0;
}

handle_t ts_upgrade_destroy(void *usrdata)
{
	return 0;
}

handle_t usb_upgrade_create(void *usrdata)
{
	windows_init(GXOTA_USB);
	minigui_widget_set_focus("win_usb_upgrade_list",TRUE);
	usb_list_set();
	return 0;
}

handle_t usb_upgrade_destroy(void *usrdata)
{
	return 0;
}

handle_t net_upgrade_create(void *usrdata)
{
	windows_init(GXOTA_NET);
	minigui_widget_set_focus("win_net_upgrade_list",TRUE);
	net_list_set();
	return 0;
}

handle_t net_upgrade_destroy(void *usrdata)
{
	return 0;
}

/*Bind Palette and surface*/
extern GxMiniGUIFile xmlsd;
extern GxMiniGUIFile xmlhd;
extern GxMiniGUIFile xmlnet;
extern GxMiniGUIFile ArialFont;
#ifdef CONFIG_ENABLE_UPGRADE_LOGO
extern unsigned char nc_logo_bmp[];
extern unsigned int nc_logo_bmp_len;
#endif

int guiinit(unsigned int chip_definition)
{
	/* connect signal */
	MINIGUI_SIGNAL_CONNECT(win_ota_screen_create);
	MINIGUI_SIGNAL_CONNECT(win_ota_screen_destroy);
	MINIGUI_SIGNAL_CONNECT(ts_upgrade_create);
	MINIGUI_SIGNAL_CONNECT(ts_upgrade_destroy);
	MINIGUI_SIGNAL_CONNECT(usb_upgrade_create);
	MINIGUI_SIGNAL_CONNECT(usb_upgrade_destroy);
	MINIGUI_SIGNAL_CONNECT(net_upgrade_create);
	MINIGUI_SIGNAL_CONNECT(net_upgrade_destroy);
	/* 初始化minigui */
	int iRect = GXCORE_SUCCESS;

#ifdef NO_OS
	file_add(ArialFont.filename, ArialFont.data, ArialFont.size);
#ifdef CONFIG_ENABLE_UPGRADE_LOGO
	file_add("nc_logo.bmp", nc_logo_bmp, nc_logo_bmp_len);
#endif

	if (HD_DEFINITION == chip_definition){
/*		file_add(xmlhd.filename, xmlhd.data, xmlhd.size);
		iRect = minigui_init_byxml(xmlhd.filename);
		file_del(xmlhd.filename);*/

		file_add(xmlnet.filename, xmlnet.data, xmlnet.size);
		iRect = minigui_init_byxml(xmlnet.filename);
		file_del(xmlnet.filename);
	}else{
		file_add(xmlsd.filename, xmlsd.data, xmlsd.size);
		iRect = minigui_init_byxml(xmlsd.filename);
		file_del(xmlsd.filename);
	}

	file_del(ArialFont.filename);
	file_del("nc_logo.bmp");
#endif
	return iRect;
}

GxMiniGUIFile Logo = {
	.filename = "logo.jpg",
	.size = 0x20000,
};

#ifdef CONFIG_ENABLE_LOGO
void showlogo(char *filename)
{
#ifdef CONFIG_ENABLE_MINIFS
	MiniGUI_Rect rect = {0};
#ifdef NO_OS
	minifs_handle_file logo_file = NULL;
	GxMiniGUIFile Logo = {
		.filename = "logo.jpg",
		.size = 0x20000,
	};
	int32_t mount_size = 0;
	int32_t size = 0;
	char *plogo = NULL;
#endif
#endif
	struct partition *flash_table = NULL;
	struct partition_info *partition = NULL;

	if(NULL == filename)
		return;

	flash_table = GxCore_PartitionFlashInit();

#ifdef CONFIG_ENABLE_MINIFS
	if(0 == strcmp("logo", filename)) {
#endif
		partition = GxCore_PartitionGetByName(flash_table, "LOGO");
		minigui_show_logo_from_partition(filename, partition->partition_size);
#ifdef CONFIG_ENABLE_MINIFS
	} else {
#ifdef NO_OS
		logo_file = gxopen("DATA","/logo.bin");

		size = minifs_size(logo_file);
		plogo = GxCore_Malloc(size);
		if(plogo == NULL)
			return;
		memset(plogo, 0, size);
		mount_size = minifs_read(logo_file, plogo, size, 0);
		if (mount_size != size ) {
			gxloge("mount_size = %d\n", mount_size);
			gxloge("error: %s %d\n", __func__, __LINE__);
			GxCore_Free(plogo);
			return;
		}
		Logo.data = (unsigned char *)plogo;
		file_add(Logo.filename, Logo.data, mount_size);
		minigui_show_logo(NULL, Logo.filename);
		file_del(Logo.filename);

		GxCore_Free(plogo);
		plogo = NULL;
#else
		minigui_show_logo(NULL, "/home/gx/logo.bin");
#endif
		minigui_flush();

		GuiSurface *surface = minigui_get_osd();
		rect.x = rect.y = 0;
		rect.w = surface->sf.width;
		rect.h = surface->sf.height;
		minigui_fillrect(surface, &rect, 0x00FF00);
	}
#endif
}
#endif
#endif
