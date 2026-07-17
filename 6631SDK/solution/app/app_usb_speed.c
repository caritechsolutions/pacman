/*****************************************************************************
* 			  CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009-2014, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_usb_speed.c
* Author    :
* Project   :
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
*****************************************************************************/



#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "app_windows.h"
#include "app.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_config.h"
#ifdef LINUX_OS

#include "app_tools.h"

typedef enum
{
	ITEM_USB_SPEED0= 0,
	ITEM_USB_SPEED1,
	ITEM_USB_SPEED_TOTAL
}UsbSpeedOptSel;

static char *s_item_usb_speed_text[] =
{
	"text_usb_speed_item1",
	"text_usb_speed_item2",
};

static char *s_item_usb_speed_btn[] =
{
	"btn_usb_speed_opt1",
	"btn_usb_speed_opt2",
};

//about menu
static int usb_dev_count = 0;
static char *usb_dev_name = NULL;
static uint32_t usb_dev_sel = 0;

#define USB_SPEED_CMD       "usb_speed"
#define USB_SPEED_TMP_FILE  "/tmp/usb_speed.txt"


static event_list* s_usb_speed_gif_timer = NULL;
static void app_usb_speed_load_gif(void);
static void app_usb_speed_free_gif(void);
static void app_usb_speed_show_gif(void);
static void app_usb_speed_hide_gif(void);

static void app_usb_speed_load_gif(void)
{
#define GIF_PATH WORK_PATH"theme/image/netapps/loading.gif"
	int alu = GX_ALU_ROP_COPY_INVERT;
	GUI_SetProperty("img_usb_speed_gif", "load_img", GIF_PATH);
	GUI_SetProperty("img_usb_speed_gif", "init_gif_alu_mode", &alu);
}
static void app_usb_speed_free_gif(void)
{
	GUI_SetProperty("img_usb_speed_gif", "load_img", NULL);
	GUI_SetProperty("img_usb_speed_gif", "state", "hide");
	if(s_usb_speed_gif_timer != NULL)
	{
		remove_timer(s_usb_speed_gif_timer);
		s_usb_speed_gif_timer = NULL;
	}
}

static int app_usb_speed_draw_gif(void* usrdata)
{
	int alu = GX_ALU_ROP_COPY_INVERT;
	GUI_SetProperty("img_usb_speed_gif", "draw_gif", &alu);
	return 0;
}
static void app_usb_speed_show_gif(void)
{
	GUI_SetProperty("img_usb_speed_gif", "state", "show");
	if(0 != reset_timer(s_usb_speed_gif_timer))
	{
		s_usb_speed_gif_timer = create_timer(app_usb_speed_draw_gif, 100, NULL, TIMER_REPEAT);
	}
}
static void app_usb_speed_hide_gif(void)
{
	//remove_timer(s_usb_speed_gif_timer);
	//s_usb_speed_gif_timer = NULL;

	if(s_usb_speed_gif_timer != NULL)
		timer_stop(s_usb_speed_gif_timer);

	GUI_SetProperty("img_usb_speed_gif", "state", "hide");
}

static void usb_speed_item_status_update(void)
{
	if(usb_dev_name != NULL)
	{
		//HotplugPartitionList* partition_list = NULL;
		//partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
		//usb_dev_count = partition_list->partition_num;
		usb_dev_count = 1;

		if(usb_dev_sel == 0)
		{
			GUI_SetProperty(s_item_usb_speed_text[ITEM_USB_SPEED1], "string", "USB(C:) Speed");
		}
		else if(usb_dev_sel == 1)
		{
			GUI_SetProperty(s_item_usb_speed_text[ITEM_USB_SPEED1], "string", "USB(D:) Speed");
		}
		else if(usb_dev_sel == 2)
		{
			GUI_SetProperty(s_item_usb_speed_text[ITEM_USB_SPEED1], "string", "USB(E:) Speed");
		}
		else if(usb_dev_sel == 3)
		{
			GUI_SetProperty(s_item_usb_speed_text[ITEM_USB_SPEED1], "string", "USB(F:) Speed");
		}
		else if(usb_dev_sel == 4)
		{
			GUI_SetProperty(s_item_usb_speed_text[ITEM_USB_SPEED1], "string", "USB(G:) Speed");
		}

		GUI_SetProperty(s_item_usb_speed_btn[ITEM_USB_SPEED1], "string", "0 KB/s");
	}

}

static void app_usb_speed_finish(void *usr_data)
{
	FILE *fd = NULL;
	char buf[32] = {0};
	int i = 0;

	memset(buf,0,sizeof(buf));
	// hide gif
	app_usb_speed_hide_gif();

	GxCore_ThreadDelay(500);
	app_log_info("\n############# %s $$$$$$$$$$$$$$\n",__FUNCTION__);
	system("cat /tmp/usb_speed.txt");
	app_log_info("\n############# %s $$$$$$$$$$$$$$\n",__FUNCTION__);

	fd = fopen(USB_SPEED_TMP_FILE ,"r");
	if(fd == NULL)
	{
		app_log_error("\nLINE:%d ,fopen  err !!!\n",__LINE__);
		return;
	}
	for(i=0;i<usb_dev_count;i++)
	{
		memset(buf,0,sizeof(buf));
		fgets(buf,sizeof(buf),fd);

		if(strcmp(buf,"error") == 0)
		{
			fclose(fd);
			app_log_error("\nLINE:%d ,fget error \n",__LINE__);
			return;
		}
		else
		{
			GUI_SetProperty(s_item_usb_speed_btn[ITEM_USB_SPEED1], "string", buf);
		}
	}
	fclose(fd);

}
static void app_usb_speed_gif_proc(void *usr_data)
{
	app_usb_speed_show_gif();
}

static int app_usb_speed_exit_callback(void)
{
	int ret = 0;
	remove(USB_SPEED_TMP_FILE);

	return ret;
}


SIGNAL_HANDLER int app_usb_speed_btn1_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	char buf[32] = {0};
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_KEYDOWN:
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
				case STBK_OK:

				if(check_usb_status() == false)
				{
					PopDlg  pop;
					memset(&pop, 0, sizeof(PopDlg));
					pop.type = POP_TYPE_OK;
					pop.str = STR_ID_INSERT_USB;
					pop.mode = POP_MODE_UNBLOCK;
					popdlg_create(&pop);
					GUI_SetInterface("flush", NULL);
				}
				else
				{
					usb_speed_item_status_update();
					if(usb_dev_name == NULL)
					{
						sprintf(buf,"%s %s",USB_SPEED_CMD,USB_SPEED_TMP_FILE);
						system_shell(buf, usb_dev_count*5000, app_usb_speed_gif_proc, app_usb_speed_finish, NULL);
					}
					else
					{
						sprintf(buf,"%s %s %s",USB_SPEED_CMD,USB_SPEED_TMP_FILE,usb_dev_name);
						system_shell(buf, usb_dev_count*5000, app_usb_speed_gif_proc, app_usb_speed_finish, NULL);
					}
				}

				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

SIGNAL_HANDLER int app_usb_speed_box_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
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
					{
						uint32_t box_sel;

						GUI_GetProperty("box_usb_speed", "select", &box_sel);
						if(ITEM_USB_SPEED0== box_sel)
						{
							box_sel = ITEM_USB_SPEED1;
							GUI_SetProperty("box_usb_speed", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
						else
						{
							ret = EVENT_TRANSFER_KEEPON;
						}
					}
					break;

				case STBK_DOWN:
					{
						uint32_t box_sel;

						GUI_GetProperty("box_usb_speed", "select", &box_sel);
						if(ITEM_USB_SPEED1== box_sel)
						{
							box_sel = ITEM_USB_SPEED0;
							GUI_SetProperty("box_usb_speed", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
						else
						{
							ret = EVENT_TRANSFER_KEEPON;
						}
					}
					break;

				case STBK_OK:
				case STBK_MENU:
				case STBK_EXIT:
					ret = EVENT_TRANSFER_KEEPON;
					break;

				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

SIGNAL_HANDLER int app_usb_speed_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
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
				case VK_BOOK_TRIGGER:
					GUI_EndDialog("after wnd_full_screen");
					break;

				case STBK_EXIT:
				case STBK_MENU:
					{
						app_usb_speed_exit_callback();
						GUI_EndDialog(WND_USB_SPEED);
					}
					break;

				case STBK_OK:
					break;

				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

SIGNAL_HANDLER int app_usb_speed_create(GuiWidget *widget, void *usrdata)
{
	app_usb_speed_load_gif();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_usb_speed_destroy(GuiWidget *widget, void *usrdata)
{
	app_usb_speed_free_gif();
	return EVENT_TRANSFER_STOP;
}

void app_usb_speed_menu_exec(void *usb_dev,uint32_t sel)
{

	usb_dev_name = (char *)usb_dev;
	usb_dev_sel  = sel;
	if(usb_dev_name == NULL)
		return;
	usb_dev_count = 1;
	GUI_CreateDialog(WND_USB_SPEED);
}

//time dd if=/dev/zero of=/testw.dbf bs=4k count=100000//write
//time dd if=/dev/sdb of=/dev/null bs=4k //read
#define GX_USB_FILE_PATH_TEXT "/mnt/usb01/test.txt"
#define O_RDWR__	"rc"
#define TOTAL_USB_NUM	10
#define USB_BUFSIZE	(1024*1024)
void app_test_usb_speed(void)
{
	//handle_t fd = 0;
	GxTime tick_start,tick_end;
	int index = 0;
	if(check_usb_status() == false)
	{
		app_log_error("____usb is not exist________%s,%d\n",__FUNCTION__,__LINE__);
		return;
	}
	//fd = GxCore_Open(GX_USB_FILE_PATH_TEXT,O_RDWR__);

	{
                handle_t usb_fd;
                char *buf = NULL;
                static int times = 0;

                int32_t path_len;
                char *filename = NULL;
                char *path = NULL;
                buf = (char *)GxCore_Malloc(1024 * 1024);
                if(buf == NULL)
                {
                    return;
                }
                times += 1;
	            HotplugPartitionList *phpl = NULL;

	            phpl = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
	            if (phpl->partition_num < 1)
	            {
                    APP_FREE(buf);
	            	return ;
	            }

	            // malloc pvr path space
	            path_len = strlen(phpl->partition[0].partition_entry)+strlen("/GxPvr/")+1; // 1 means '/0'
	            path = (char*)GxCore_Malloc(path_len);
	            if (path == NULL)
	            {
                    APP_FREE(buf);
	            	return ;
	            }

	            // set pvr path
	            memset(path, 0, path_len);
	            strcpy(path, phpl->partition[0].partition_entry);
	            strcat(path, "/GxPvr/");
                filename = (char *) GxCore_Malloc(10);
                if(filename == NULL)
                {
                    APP_FREE(buf);
                    APP_FREE(path);
                    return;
                }
                memset(filename,0,10);
                snprintf(filename,10,"%d",times);
                strcat(path,filename);
                app_log_info("__11111---111111__=%s\n",path);
                usb_fd = GxCore_Open(path,"w");
                if(usb_fd < 0)
                {
                    APP_FREE(filename);
                    APP_FREE(buf);
                    APP_FREE(path);
                    return ;
                }
                memset(buf,'a',1024*1024);
                //GxCore_Write(usb_fd,buf,1,1024*1024);
                GxCore_GetTickTime(&tick_start);
                app_log_info("__________________[%d]___-----\n",tick_start.seconds);
		   while(index++<TOTAL_USB_NUM)
		   {
			if(GxCore_Write(usb_fd,buf,1,USB_BUFSIZE)!=USB_BUFSIZE)
           			 break;
    		   }
		  GxCore_GetTickTime(&tick_end);
		  app_log_debug("end   time:  %02d:%03d\n",tick_end.seconds, tick_end.microsecs );

		  if((tick_end.microsecs- tick_start.microsecs) >=0 )
		        app_log_debug("second used: %d.%03d\n", (int)(tick_end.seconds- tick_start.seconds), (int)(tick_end.microsecs- tick_start.microsecs) );
		    else
		        app_log_debug("second used: %d.%03d\n", (int)(tick_end.seconds- tick_start.seconds - 1), (int)(1000 + tick_end.microsecs- tick_start.microsecs) );
                GxCore_Close(usb_fd);
                APP_FREE(filename);
                APP_FREE(path);
                APP_FREE(buf);
       }


	app_log_debug("[%s],line=%d---usb write status:[%d m/s]\n",__FUNCTION__,__LINE__,(tick_end.seconds-tick_start.seconds)/TOTAL_USB_NUM);
	return;

}
#endif

