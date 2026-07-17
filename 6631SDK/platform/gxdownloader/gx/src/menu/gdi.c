#include "config.h"
#include "common.h"
#include "gui.h"
#include "bus/gui_core/mini_gui.h"
#include "keyboard.h"
#include "string.h"

#ifdef NO_OS
#include "io.h"
#include "gxloader.h"
#include "minifs/api_minifs.h"
#include "usb_download.h"
#include "fat.h"
#endif
#include "devapi/gxirr_api.h"
#include "gxdlp_api.h"

#ifdef CONFIG_ENABLE_GDI

static GuiSurface *surface = NULL;
static GuiSurface *main_surface = NULL;

#define LIST_X                (100) //Location of the X-axis of the list box
#define LIST_Y                (120) //Location of the X-axis of the list box
#define LIST_W                (824) //Width of list box
#define LIST_H                (351) //Height of list box
#define SURFACE_COLOR         (0x0F0F0F) //The color of the container
#define LIST_BACKCOLOR        (0x4A708B) //Background color of list box
#define LIST_FONT_COLOR       (0xFFFFFF) //Font color in list box
#define FOCUS_ITEM_COLOR      (0xCDB38B) //Focus on the color of the font
#define FOCUS_FONT_COLOR      (0x4D4D4D) //Focus on the color of the font
#define FONT_HEIGHT           (45)  //Setting Font Height
#define ROWS                  (8)   //Number of total rows in the list
#define SINGLE_HEIGHT         (39)  //Single row height
#define SPACE                 (1)   //Horizontal dividers between rows
#define GRID                  (1)   //Horizontal dividers between rows
#define FILE_MAX_NUM          (100)
#define MAXSIZE               (2048)
#define EDIT_BOX_COLOR        (0xFFFFFF)
#define EDIT_FONT_COLOR       (0x0F0F0F)
#define EDIT_BOX_X1           (50)
#define EDIT_BOX_X2           (95)
#define EDIT_BOX_X3           (140)
#define EDIT_BOX_X4           (185)
#define EDIT_BOX_X5           (240)
#define EDIT_BOX_Y            (100)
#define EDIT_BOX_W            (40)
#define EDIT_BOX_H            (28)
#define MAX_LENGTH            (16)


typedef struct
{
    int process[2];
    int text[2];
} render_info;


typedef struct _PartitionListNode{
	int file_num;
	char *column_content[FILE_MAX_NUM];
	char size_content[FILE_MAX_NUM][16];
} PartitionListNode;

typedef struct
{
	int pages;
	int last_page_fnum;
	int page_p;
	int select;
	PartitionListNode Node;
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

static int _update_all(void)
{
	MiniGUI_Rect rect = {0};

	rect.w = main_surface->sf.width;
	rect.h = main_surface->sf.height;

	return minigui_copy(surface, &rect, main_surface, &rect);
}

static void _draw_rect(MiniGUI_Rect *rect_array, uint32_t color, char* context)
{
	if (context != NULL) {
		minigui_draw_string(surface, context, rect_array, color, MINI_TA_LEFT | MINI_TA_VCENTRE);
	} else {
		minigui_fillrect(surface, rect_array, color);
	}
}

static void draw_list(int8_t select)
{
    MiniGUI_Rect rect_array = {0};
	int i = 0;
	rect_array.x = LIST_X;
	rect_array.y = LIST_Y;
	rect_array.w = LIST_W;
	rect_array.h = LIST_H;
	_draw_rect(&rect_array, LIST_BACKCOLOR, NULL);
	_draw_rect(&rect_array, LIST_FONT_COLOR, "C>");

	/* Focus on the row currently selected*/
	rect_array.y = LIST_Y + (dir_info.select + 1) * SINGLE_HEIGHT;
	rect_array.w = LIST_W;
	rect_array.h = SINGLE_HEIGHT;
	_draw_rect(&rect_array, FOCUS_ITEM_COLOR, NULL);

	rect_array.y = LIST_Y + SINGLE_HEIGHT;
	rect_array.h = SPACE;
	for (i = 1; i <= ROWS + 1; i++) {
		rect_array.y = LIST_Y + SINGLE_HEIGHT * i;
		_draw_rect(&rect_array, 0xFFFFFF, NULL);
	}

	rect_array.x = LIST_X + 500;
	rect_array.y = LIST_Y + SINGLE_HEIGHT;
	rect_array.w = GRID;
	rect_array.h = LIST_H - SINGLE_HEIGHT;
	_draw_rect(&rect_array, 0xFFFFFF, NULL);


	rect_array.x = LIST_X + 520;
	rect_array.w = 100;
	if (select == 0) {
		_draw_rect(&rect_array, FOCUS_FONT_COLOR, "size");
	} else {
		_draw_rect(&rect_array, LIST_FONT_COLOR, "size");
	}
}

#ifdef CONFIG_ENABLE_USB_UPGRADE
static char* get_content(char *pbuf, unsigned long pbuf_size, int num)
{
	unsigned long i;
	if (num == 0)
		return(pbuf);
	else {
		for (i = 0; i < pbuf_size - 1; i++) {
			if (pbuf[i] == 0) {
				num--;
				if (num == 0)
					return (pbuf + i + 1);
			}
		}
	}

	return (NULL);
}

static int get_file(char *path,char *pbuf, PartitionListNode *Node)
{
	unsigned long pbuf_size = 0;
	unsigned long file_size = 0;
	int i = 0;
	char *pfile = NULL;
	if (0 != usb_get_dir(path, pbuf, 1024, &pbuf_size)) {
		gxloge("read_usb_path failed\n");
	}

	Node->column_content[0] = "../";
	Node->file_num++;

	while(1) {
		pfile = get_content(pbuf, pbuf_size, i++);
		if (NULL == pfile)
			break;
		if (strcmp(pfile, "./") != 0 && strcmp(pfile, "../") != 0) {
			sprintf(Node->size_content[Node->file_num], "%ld", file_size);

			Node->column_content[Node->file_num] = pfile;

			if (pfile[strlen(pfile) - 1] != '/') {
				file_get_size(pfile, &file_size);
				sprintf(Node->size_content[Node->file_num], "%ld", file_size);
			} else {
				strcpy(Node->size_content[Node->file_num],  "--");
			}

			Node->file_num++;
		}
	}

	return (0);
}
#endif
static void _display_item(int pages, int page_p, int last_page_fnum, int select, uint32_t color)
{
	int i = 0;
	int row_fnum = 0;
	MiniGUI_Rect rect_array = {0};
	if (pages == 0) {
		row_fnum = dir_info.Node.file_num - 1;
	} else {
		if (page_p == pages) {
			row_fnum = last_page_fnum;
		} else {
			row_fnum = ROWS - 1;
		}
	}

	draw_list(dir_info.select);

	rect_array.x = LIST_X + 20;
	rect_array.y = LIST_Y + SINGLE_HEIGHT;
	rect_array.w = LIST_W;
	rect_array.h = FONT_HEIGHT;
	if (strcmp(dir_info.Node.column_content[0], "../") == 0 && page_p == 0) {
		_draw_rect(&rect_array, color, dir_info.Node.column_content[0]);
	}

	for (i = 1; i <= row_fnum; i++) {
		/*Compute the position of the filled string in the list*/
		rect_array.x = LIST_X + 20;
		rect_array.y = LIST_Y + SINGLE_HEIGHT * (i + 1);
		rect_array.w = 500;
		_draw_rect(&rect_array, color, dir_info.Node.column_content[page_p * (ROWS - 1) + i]);
		/*Compute the position of the filled size in the list*/
		rect_array.x = LIST_X + 540;
		rect_array.w = 100;
		_draw_rect(&rect_array, color, dir_info.Node.size_content[page_p * (ROWS - 1) + i]);
	}
	/*Focus font color*/
	rect_array.x = LIST_X + 20;
	rect_array.y = LIST_Y + SINGLE_HEIGHT * (dir_info.select + 1);
	rect_array.w = 500;
	_draw_rect(&rect_array, FOCUS_FONT_COLOR, dir_info.Node.column_content[page_p * (ROWS - 1) + dir_info.select]);

	rect_array.x = LIST_X + 540;
	rect_array.w = 100;
	_draw_rect(&rect_array, FOCUS_FONT_COLOR, dir_info.Node.size_content[page_p * (ROWS - 1) + dir_info.select]);

}

static void key_updown(uint32_t value)
{
	switch(value) {
		case OTAK_UP:
			if (dir_info.page_p == 0) {
				if (dir_info.select != 0)
					dir_info.select = dir_info.select - 1;
			} else {
				if (dir_info.select != 1)
					dir_info.select = dir_info.select - 1;
				else {
					dir_info.page_p--;
					dir_info.select = ROWS - 1;
				}
			}
			break;
		case OTAK_DOWN:
			if (dir_info.pages == 0) {
				if (dir_info.select != (dir_info.last_page_fnum - 1))
					dir_info.select = dir_info.select + 1;
			} else {
				if (dir_info.page_p == dir_info.pages) {
					if (dir_info.select != (dir_info.last_page_fnum))
						dir_info.select = dir_info.select + 1;
				} else {
					if(dir_info.select != ROWS - 1)
						dir_info.select = dir_info.select + 1;
					else {
						dir_info.page_p++;
						dir_info.select = 1;
					}
				}
			}
			break;
		default:
			break;
	}
		_display_item(dir_info.pages, dir_info.page_p, dir_info.last_page_fnum, dir_info.select, LIST_FONT_COLOR);
}

#ifdef ECOS_OS
int file_add(const char *name, void *buf, unsigned int size)
{
	handle_t file;

	file = GxCore_Open(name, "wb+");
	if (file < 0) {
		gxloge("file_add open error\n");
		return -1;
	}

	if (GxCore_Write(file, buf, 1, size) < 0) {
		gxloge("file_add write error\n");
		return -1;
	}

	GxCore_Close(file);

	return 0;
}

int file_del(const char *name)
{
//	if (GxCore_FileDelete(name) < 0) {
//		gxloge("file_delete error\n");
//		return -1;
//	}

	return 0;
}
#endif

#ifdef CONFIG_ENABLE_UPGRADE_LOGO
extern unsigned char nc_logo_bmp[];
extern unsigned int nc_logo_bmp_len;
#endif
static void upgrade_show()
{
	MiniGUI_Rect rect = {0};
#ifdef CONFIG_ENABLE_UPGRADE_LOGO
	image_desc *desc = NULL;
	rect.x = 0;
	rect.y = 0;
	rect.w = 1024;
	rect.h = 576;
	_draw_rect(&rect, SURFACE_COLOR, NULL);

	file_add("nc_logo.bmp", nc_logo_bmp, nc_logo_bmp_len);

	desc = minigui_load_image("nc_logo.bmp");

	file_del("nc_logo.bmp");

	minigui_draw_image(surface,
                             desc,
                             430,
                             240,
                             180,
                             80);
#endif

	rect.x = 260;
	rect.y = 380;
	rect.w = 480;
	rect.h = 20;
	_draw_rect(&rect, 0xFFFFFF, NULL);  //0xFFFFFF：显示进度条的底色
	rect.x = 744;
	rect.y = 374;
	rect.w = 100;
	rect.h = FONT_HEIGHT;
	_draw_rect(&rect, 0xFFFFFF, "00%");
}

static void key(int8_t upgrade_type);
#ifdef CONFIG_ENABLE_USB_UPGRADE
static char cur_path[1024] = "/";
static void usb_upgrade_get(int8_t upgrade_type)
{
	char *pbuf = (char *)malloc(MAXSIZE);
	char *pfile = NULL;
	pfile = dir_info.Node.column_content[dir_info.page_p * (ROWS - 1) + dir_info.select];
	strcat(cur_path, pfile);
	memset(&dir_info, 0, sizeof(dir_info));

	memset(pbuf, 0 , MAXSIZE);
	if (pfile[strlen(pfile) - 1] != '/') {
		GxDLP_Set_UpgradeFileName(pfile);
		upgrade_show();
	} else {
		get_file(cur_path, pbuf, &(dir_info.Node));
		if (dir_info.Node.file_num <= ROWS) {
			dir_info.pages = 0;
			dir_info.last_page_fnum = dir_info.Node.file_num;
		} else {
			dir_info.pages = dir_info.Node.file_num / ROWS;
			dir_info.last_page_fnum = (dir_info.Node.file_num - 1) % (ROWS - 1);
		}

		if (dir_info.last_page_fnum == 0)
			dir_info.last_page_fnum = ROWS - 1;

		_display_item(dir_info.pages, dir_info.page_p, dir_info.last_page_fnum, dir_info.select, LIST_FONT_COLOR);
		key(upgrade_type);
	}
	free(pbuf);
}
#endif

static char num_char(int value)
{
	if((value >= 0) && (value <= 9)) {
		return (value + '0');
	} else {
		return ('\0');
	}
}

static void net_edit_show()
{
	MiniGUI_Rect rect = {0};
	rect.x = EDIT_BOX_X1;
	rect.y = EDIT_BOX_Y;
	rect.w = EDIT_BOX_W;
	rect.h = EDIT_BOX_H;
	_draw_rect(&rect, EDIT_BOX_COLOR, NULL);

	rect.x = rect.x + rect.w;
	rect.h = 45;
	_draw_rect(&rect, EDIT_BOX_COLOR, ".");
	rect.x = EDIT_BOX_X2;
	rect.h = EDIT_BOX_H;
	_draw_rect(&rect, EDIT_BOX_COLOR, NULL);

	rect.x = rect.x + rect.w;
	rect.h = 45;
	_draw_rect(&rect, EDIT_BOX_COLOR, ".");
	rect.x = EDIT_BOX_X3;
	rect.h = EDIT_BOX_H;
	_draw_rect(&rect, EDIT_BOX_COLOR, NULL);

	rect.x = rect.x + rect.w;
	rect.h = 45;
	_draw_rect(&rect, EDIT_BOX_COLOR, ".");
	rect.x = EDIT_BOX_X4;
	rect.h = EDIT_BOX_H;
	_draw_rect(&rect, EDIT_BOX_COLOR, NULL);

	rect.x = rect.x + rect.w + 5;
	rect.h = 45;
	_draw_rect(&rect, EDIT_BOX_COLOR, ":");
	rect.x = EDIT_BOX_X5;
	rect.h = EDIT_BOX_H;
	rect.w = EDIT_BOX_W + 10;
	_draw_rect(&rect, EDIT_BOX_COLOR, NULL);

	rect.x = EDIT_BOX_X1;
	rect.y = EDIT_BOX_Y + 45;
	rect.h = EDIT_BOX_H;
	rect.w = EDIT_BOX_W + 50;
	_draw_rect(&rect, EDIT_BOX_COLOR, NULL);
	rect.h = 45;
	_draw_rect(&rect, EDIT_FONT_COLOR, "update.bin");

}

static char net_addr[24] = {0};
static int cur_pos = 0;
static int count = 0;

static void net_addr_get()
{
	char ip_string[17] = {0};
	char port[5] = {0};
	int i = 0, j = 0, count = 0;

	for (i = 0; i < 12; i++) {
		count++;
		ip_string[j] = net_addr[i];
		if(0 == count % 3 && 15 != strlen(ip_string)) {
			j++;
			ip_string[j] = '.';
			count = 0;
		}
		j++;
	}
	GxDLP_Set_SERVER_IP_ADDR(ip_string);
	strncpy(port, net_addr + 12, 4);
	GxDLP_Set_TFTP_PORT(port);
}
static void _draw_cursor()
{
	MiniGUI_Rect cursor_rect = {0};
	int str_width = 0, num = 0;
	int select_edit = 0;
	char *cur_char = NULL;
	char tem_buf[16] = {0};
	char pstart[5] = {0};
	char *tem_value = NULL;

	tem_value = net_addr;

	if (cur_pos <= 12) {
		select_edit = cur_pos / 3;
		num = cur_pos - select_edit * 3;
	}

	if (cur_pos <= 3) {
		strncpy(pstart, tem_value, 3);
	} else if (cur_pos > 3 && cur_pos <= 6) {
		strncpy(pstart, tem_value + 3, 3);
	} else if (cur_pos > 6 && cur_pos <= 9) {
		strncpy(pstart, tem_value + 6, 3);
	} else if (cur_pos > 9 && cur_pos <= 12) {
		strncpy(pstart, tem_value + 9, 3);
	} else {
		strncpy(pstart, tem_value + 12, 4);
	}

	if (cur_pos < 13) {
		if (0 == cur_pos % 3) {
			strncpy(tem_buf, pstart, 1);
			str_width = 0;
			cur_char = tem_buf;
		} else {
			strncpy(tem_buf, pstart, num);
			str_width = minigui_get_string_pixel(tem_buf);
			cur_char = tem_buf + num - 1;
		}
	} else {
		strncpy(tem_buf, pstart, cur_pos - 12);
		str_width = minigui_get_string_pixel(tem_buf);
		cur_char = tem_buf + strlen(tem_buf) - 1;
	}

	if (cur_pos < 3) {
		cursor_rect.x = EDIT_BOX_X1;
	} else if (cur_pos >= 3 && cur_pos < 6) {
		cursor_rect.x = EDIT_BOX_X2;
	} else if (cur_pos >= 6 && cur_pos < 9) {
		cursor_rect.x = EDIT_BOX_X3;
	} else if (cur_pos >= 9 && cur_pos < 12) {
		cursor_rect.x = EDIT_BOX_X4;
	} else {
		cursor_rect.x = EDIT_BOX_X5;
	}
	_draw_rect(&cursor_rect, EDIT_BOX_COLOR, NULL);
	cursor_rect.x = cursor_rect.x + str_width;
	cursor_rect.y = EDIT_BOX_Y + minigui_get_font_height();
	cursor_rect.w = minigui_get_string_pixel(cur_char);
	cursor_rect.h = 1;
	_draw_rect(&cursor_rect, 0x0F0F0F, NULL);
}

static int _draw_ip(int count)
{
	char *ip_string = NULL, *pvalue = NULL, *pstart = NULL, *act_value = NULL;
	MiniGUI_Rect ip_rect = {0};

	pstart = ip_string = (char *)malloc(64);
	if (NULL == ip_string) {
		return (1);
	}

	pvalue = net_addr;
	memset(ip_string, 0, 64);

	act_value = (char *)malloc(64);
	if (NULL == act_value) {
		free(pstart);
		return (1);
	}
	memset(act_value, 0, 64);

	while (*pvalue) {
		*ip_string = *pvalue;
		pvalue++;
		ip_string++;
	}

	ip_rect.y = EDIT_BOX_Y;
	ip_rect.w = EDIT_BOX_W;
	ip_rect.h = EDIT_BOX_H;

	if (count <= 3 || count == strlen(net_addr) || count == 4) {
		ip_rect.x = EDIT_BOX_X1;
		strncpy(act_value, pstart, 3);
		_draw_rect(&ip_rect, EDIT_BOX_COLOR, NULL);
		_draw_rect(&ip_rect, EDIT_FONT_COLOR, act_value);
	}
	if (count >= 1 && count <= 7) {
		ip_rect.x = EDIT_BOX_X2;
		strncpy(act_value, pstart + 3, 3);
		_draw_rect(&ip_rect, EDIT_BOX_COLOR, NULL);
		_draw_rect(&ip_rect, EDIT_FONT_COLOR, act_value);
	}
	if((count >= 5 && count <= 10) || count == 1) {
		ip_rect.x = EDIT_BOX_X3;
		strncpy(act_value, pstart + 6, 3);
		_draw_rect(&ip_rect, EDIT_BOX_COLOR, NULL);
		_draw_rect(&ip_rect, EDIT_FONT_COLOR, act_value);
	}
	if((count >= 8 && count <= 13) || count ==1)
	{
		ip_rect.x = EDIT_BOX_X4;
		strncpy(act_value, pstart + 9, 3);
		_draw_rect(&ip_rect, EDIT_BOX_COLOR, NULL);
		_draw_rect(&ip_rect, EDIT_FONT_COLOR, act_value);
	}
	if(count >= 11 ||count == 1)
	{
		ip_rect.x = EDIT_BOX_X5;
		strncpy(act_value, pstart + 12, 4);
		_draw_rect(&ip_rect, EDIT_BOX_COLOR, NULL);
		_draw_rect(&ip_rect, EDIT_FONT_COLOR, act_value);
	}

	free(act_value);
	free(pstart);
	return (0);
}

static void key_data(uint32_t value)
{
	static int addr_len = 0;
	switch(value) {
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
			net_addr[cur_pos] = num_char(value);
			if (addr_len < MAX_LENGTH) {
				count++;
			}
			addr_len = strlen(net_addr);
			if (cur_pos < (MAX_LENGTH - 1) && addr_len < MAX_LENGTH) {
				cur_pos++;
			}
			break;
		default:
			break;
	}
	_draw_ip(count);
	_draw_cursor();

}

static void key_left_right(int32_t value)
{
	int str_len = 0;

	str_len = strlen(net_addr);
	switch(value) {
		case OTAK_LEFT:
			if (str_len < MAX_LENGTH && cur_pos == count) {
				count = count + 1;
			}
			cur_pos--;
			if (cur_pos < 0) {
				if (str_len < MAX_LENGTH)
					cur_pos = str_len;
				else
					cur_pos = str_len - 1;
			}
			count--;
			if (count < 1) {
				count = str_len;
			}
			break;
		case  OTAK_RIGHT:
			cur_pos++;
			if (str_len < MAX_LENGTH && cur_pos > str_len) {
				cur_pos = 0;
			}
			if (str_len == MAX_LENGTH && cur_pos > str_len - 1) {
				cur_pos = 0;
			}
			count = cur_pos + 1;
			break;
		default:
			break;
	}

	_draw_ip(count);
	_draw_cursor();
}

static void key(int8_t upgrade_type)
{
	int8_t ret = 0;
	uint32_t code = 0;
	key_type key = 0;
	int flag = 1;
	while(flag) {
		ret = GxIRR_Read(&code, GXIRR_NONBLOCK);
		code = ((code >>16)&0xff00) + ((code >> 8)&0xff);
		//if(ret != -1)
		//	gxlogd("ota_key_code = %x\n", code);
		key = get_key(code);

		switch(key) {
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
				if (upgrade_type == GXDLP_NET_TFTP_DOWNLOAD) {
					if (MAX_LENGTH == strlen(net_addr)) {
						net_addr_get();
						upgrade_show();
					} else {
						flag = 1;
						key_data(key);
					}
				}
#ifdef CONFIG_ENABLE_USB_UPGRADE
				if (upgrade_type == GXDLP_USB_DOWNLOAD) {
					usb_upgrade_get(upgrade_type);
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
	MiniGUI_Rect rect = {0};
	rect.x = 0;
	rect.y = 0;
	rect.w = 1024;
	rect.h = 576;
	_draw_rect(&rect, SURFACE_COLOR, NULL);
	if (upgrade_type == GXDLP_NET_TFTP_DOWNLOAD) {
		net_edit_show();
		key(upgrade_type);
	}
#ifdef CONFIG_ENABLE_USB_UPGRADE
	else if (upgrade_type == GXDLP_USB_DOWNLOAD) {
		char *pbuf = (char *)malloc(MAXSIZE);
		memset(pbuf, 0 , MAXSIZE);
		get_file("/", pbuf, &dir_info.Node);

		if (dir_info.Node.file_num <= ROWS) {
			dir_info.pages = 0;
			dir_info.last_page_fnum = dir_info.Node.file_num - 1;
		} else {
			dir_info.pages = dir_info.Node.file_num / ROWS;
			dir_info.last_page_fnum = (dir_info.Node.file_num - 1) % (ROWS - 1);
		}
		if(dir_info.last_page_fnum == 0)
			dir_info.last_page_fnum = ROWS - 1;

		_display_item(dir_info.pages, dir_info.page_p, dir_info.last_page_fnum, 0, LIST_FONT_COLOR);
		key(upgrade_type);
		free(pbuf);
	}
#endif
	else {
		upgrade_show();
	}
}

extern unsigned char Arial[];
extern unsigned int Arial_bin_len;
extern unsigned char seguisb_ttf[];
extern unsigned int seguisb_ttf_len;

int gdiinit(unsigned int chip_definition)
{
#ifdef NO_OS
#define FONT_FILE_NAME "dot_font"
#else
#define FONT_FILE_NAME "/dot_font"
#endif
	int iRect = GXCORE_SUCCESS;
	MiniGUI_Rect rect = {0};

	surface = minigui_init(1024, 576, 16);
	if (NULL == surface) {
		return (GXCORE_ERROR);
	}

	rect.x = 0;
	rect.y = 0;
	rect.w = 1024;
	rect.h = 576;
	main_surface = minigui_get_surface(&rect, GX_COLOR_FMT_RGB565);
	if(NULL == main_surface) {
		return (GXCORE_ERROR);
	} else {
		GuiSurface *tmp_surface = NULL;

		tmp_surface = surface;
		surface = main_surface;
		main_surface = tmp_surface;
	}
	_draw_rect(&rect, 0x00FF00, NULL);

	file_add(FONT_FILE_NAME, Arial, Arial_bin_len);
	minigui_load_font("Arial", FONT_FILE_NAME, 20, MINI_TTF_STYLE_NORMAL);
	minigui_set_font("Arial");
	file_del(FONT_FILE_NAME);
	return (iRect);
}

static int GxOTA_GDIProcessUpdate(int fd, int percent)
{
	MiniGUI_Rect rect = {0};
	char str[10] = {0};
	sprintf(str, "%d%s", percent, "\%");

	rect.x = 744;
	rect.y = 374;
	rect.w = 100;
	rect.h = FONT_HEIGHT;
	_draw_rect(&rect, SURFACE_COLOR, NULL);  //用矩形框填充进度百分比的位置
	_draw_rect(&rect, 0xFFFFFF, str);  //0xFFFFFF：显示百分比字体的颜色
	rect.x = 260;
	rect.y = 380;
	rect.w = 480;
	rect.h = 20;
	rect.w = rect.w * percent / 100;
	_draw_rect(&rect, 0x00EE00, NULL);  //0x00EE00：进度条的颜色
	return (0);
}

static int GxOTA_GDITextShow(int fd, char* str)
{
	MiniGUI_Rect rect = {0};

	rect.x = 260;
	rect.y = 418;
	rect.w = 600;
	rect.h = 50;
	_draw_rect(&rect, SURFACE_COLOR, NULL);
	_draw_rect(&rect, 0xFFFFFF, str);
	return (0);
}

static int _update_all(void);
void gui_show(int processid, const struct GxOTA_Msg* msg, char *rc_string)
{
	static int old_msg_id = -1;
	char tmpstring[100] = {0};
	GxDLP_Language_Type tmplanguage = 0;
	GxDLP_Language_Type lantype = GXDLP_CHINESE;

	switch(processid) {
		case 1:
			if (GXOTA_GUI_PROCESS == msg->type) {
				GxOTA_GDIProcessUpdate(gui_handle.process[0], msg->percent);
			} else if (GXOTA_GUI_STATUS == msg->type) {
				if (old_msg_id != msg->msg_id) {
					GxDLP_Get_Language(&tmplanguage);
					if (-1 == tmplanguage) {
						lantype = GXDLP_CHINESE;
						sprintf(tmpstring, "%s%s", STR_CN_GXOTA_STATUS, str_hint[lantype][msg->msg_id]);
					} else {
						if (tmplanguage == GXDLP_CHINESE) {
							lantype = GXDLP_CHINESE;
							sprintf(tmpstring, "%s%s", STR_CN_GXOTA_STATUS, str_hint[lantype][msg->msg_id]);
						}

						if (tmplanguage == GXDLP_ENGLISH) {
							lantype = GXDLP_ENGLISH;
							sprintf(tmpstring, "%s%s", STR_EN_GXOTA_STATUS, str_hint[lantype][msg->msg_id]);
						}
					}
					GxOTA_GDITextShow(gui_handle.text[1], tmpstring);

					old_msg_id = msg->msg_id;
				}
			}
			break;
		case 0:
			if (GXOTA_GUI_PROCESS == msg->type) {
				GxOTA_GDIProcessUpdate(gui_handle.process[0], msg->percent);
			} else if (GXOTA_GUI_STATUS == msg->type) {
				if (NULL != rc_string) {
					GxOTA_GDITextShow(gui_handle.text[0], rc_string);
				}
			}
			break;
		default:
			break;
	}

	_update_all();
}

void hidemenu(void)
{
	MiniGUI_Rect rect = {0};
	rect.x = 0;
	rect.y = 0;
	rect.w = 1024;
	rect.h = 576;
	_draw_rect(&rect, 0x00FF00, NULL);
}

#ifdef CONFIG_ENABLE_LOGO
void showlogo(char *filename)
{
#define LOGO_FILE_NAME "/logo.bin"
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
#ifdef NO_OS
		minigui_show_logo_from_partition(filename, partition->partition_size);
#else
		char *logo_buf = NULL;
		image_desc *desc = NULL;
		logo_buf = GxCore_Malloc(partition->partition_size);
		if (logo_buf == NULL) {
			gxloge("malloc failed\n");
			return;
		}
		memset(logo_buf, 0x00, partition->partition_size);
		GxCore_PartitionRead(partition, logo_buf, partition->partition_size, 0);
		file_add(LOGO_FILE_NAME, logo_buf, partition->partition_size);
		GxCore_Free(logo_buf);
		GDI_RegisterJPEG();
		desc = minigui_load_image(LOGO_FILE_NAME);
		minigui_show_logo_from_data(desc->data, desc->width, desc->height);
#endif
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
		return;
	}
#endif
}
#endif
#endif

