/*
 * =====================================================================================
 *
 *       Filename:  app_panel.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2016年05月16日 14时20分15秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include <memmgr.h>
#include "gxcore.h"
#include "tree.h"
#include "parser.h"
#include "app_module.h"
#include "app.h"
#include "app_bsp.h"


#define APP_PANEL_PATH WORK_PATH"theme/panel.xml"

static BspItem panel_item[] = {
    { "board type",   "board type"      },
    { "led converse", "led converse" },
    { "led lock",     "led lock" },
    { "led value",    "led value"},
    { "key value",    "key value"},
    { "led standby",  "led standby" },
	{ "digtal value", "digtal value" },
	{ "longkey value", "longkey value" },
    { NULL,    NULL},
};


static handle_t bsp_panel_handle;
static uint32_t s_panel_keymap[PANEL_KEYMAP_TOTAL] = {0};

uint32_t app_get_panel_keycode(PanelKeymap key_index)
{
	return s_panel_keymap[key_index];
}

static status_t app_get_panel_keymap(void)
{
#define KEYMAP_FILE "/dvb/theme/keymap.xml"
#define KEYMAP_STR  (uint8_t*)"keymap"
#define KEYMAP_GROUP  (uint8_t*)"group"
#define KEYMAP_NAME  (uint8_t*)"name"

	GXxmlDocPtr doc;
	uint32_t key_flag = 0;
	uint32_t flag_temp = 0;
	uint8_t i = 0;
	char *panel_key_name[PANEL_KEYMAP_TOTAL] =
	{
		"GUIK_M",
		"GUIK_ESCAPE",
		"GUIK_RETURN",
		"GUIK_UP",
		"GUIK_DOWN",
		"GUIK_LEFT",
		"GUIK_RIGHT",
		"GUIK_H",
	};

	doc = GXxmlParseFile(KEYMAP_FILE);
	if (NULL == doc)
	{
		app_log_error( "[panel keymap]keyGXxml parse error.\n");
		return GXCORE_ERROR;
	}

	// root
	GXxmlNodePtr root = doc->root;
	if (root == NULL)
	{
		app_log_error( "[panel keymap]keyGXxml empty\n");
		GXxmlFreeDoc(doc);
		return GXCORE_ERROR;
	}

	if (0 !=  GXxmlStrcmp(root->name, KEYMAP_STR))
	{
		GXxmlFreeDoc(doc);
		return GXCORE_ERROR;
	}

	for(i = 0; i < PANEL_KEYMAP_TOTAL; i++)
	{
		flag_temp |= (1 << i);
	}

	// group
	GXxmlNodePtr group = root->childs;
	while (NULL != group)
	{
		GXxmlNodePtr item = NULL;
		uint8_t *value = NULL;
		uint8_t *iname = NULL;

		if (GXxmlStrcmp(group->name, KEYMAP_GROUP) == 0)
		{
			item =  group->childs;
			while (item != NULL)
			{
				iname = GXxmlGetProp(item, KEYMAP_NAME);
				if(iname == NULL)
				{
					item = item->next;
					continue;
				}

				if (GXxmlStrcmp(iname, (uint8_t*)panel_key_name[PANEL_KEYMAP_MENU]) == 0)
				{
					value = GXxmlNodeGetContent(item);
					if(value != NULL)
					{
						s_panel_keymap[PANEL_KEYMAP_MENU] = app_hexstr2value(value);
						key_flag |= (1 << PANEL_KEYMAP_MENU);
						GXxmlFree(value);
					}
				}
				else if (GXxmlStrcmp(iname, (uint8_t*)panel_key_name[PANEL_KEYMAP_EXIT]) == 0)
				{
					value = GXxmlNodeGetContent(item);
					if(value != NULL)
					{
						s_panel_keymap[PANEL_KEYMAP_EXIT] = app_hexstr2value(value);
						key_flag |= (1 << PANEL_KEYMAP_EXIT);
						GXxmlFree(value);
					}
				}
				else if(GXxmlStrcmp(iname, (uint8_t*)panel_key_name[PANEL_KEYMAP_OK]) == 0)
				{
					value = GXxmlNodeGetContent(item);
					if(value != NULL)
					{
						s_panel_keymap[PANEL_KEYMAP_OK] = app_hexstr2value(value);
						key_flag |= (1 << PANEL_KEYMAP_OK);
						GXxmlFree(value);
					}
				}
				else if(GXxmlStrcmp(iname, (uint8_t*)panel_key_name[PANEL_KEYMAP_UP]) == 0)
				{
					value = GXxmlNodeGetContent(item);
					if(value != NULL)
					{
						s_panel_keymap[PANEL_KEYMAP_UP]= app_hexstr2value(value);
						key_flag |= (1 << PANEL_KEYMAP_UP);
						GXxmlFree(value);
					}
				}
				else if(GXxmlStrcmp(iname, (uint8_t*)panel_key_name[PANEL_KEYMAP_DOWN]) == 0)
				{
					value = GXxmlNodeGetContent(item);
					if(value != NULL)
					{
						s_panel_keymap[PANEL_KEYMAP_DOWN] = app_hexstr2value(value);
						key_flag |= (1 << PANEL_KEYMAP_DOWN);
						GXxmlFree(value);
					}
				}
				else if(GXxmlStrcmp(iname, (uint8_t*)panel_key_name[PANEL_KEYMAP_LEFT]) == 0)
				{
					value = GXxmlNodeGetContent(item);
					if(value != NULL)
					{
						s_panel_keymap[PANEL_KEYMAP_LEFT] = app_hexstr2value(value);
						key_flag |= (1 << PANEL_KEYMAP_LEFT);
						GXxmlFree(value);
					}
				}
				else if(GXxmlStrcmp(iname, (uint8_t*)panel_key_name[PANEL_KEYMAP_RIGHT]) == 0)
				{
					value = GXxmlNodeGetContent(item);
					if(value != NULL)
					{
						s_panel_keymap[PANEL_KEYMAP_RIGHT] = app_hexstr2value(value);
						key_flag |= (1 << PANEL_KEYMAP_RIGHT);
						GXxmlFree(value);
					}
				}
				else if(GXxmlStrcmp(iname, (uint8_t*)panel_key_name[PANEL_KEYMAP_PW]) == 0)
				{
					value = GXxmlNodeGetContent(item);
					if(value != NULL)
					{
						s_panel_keymap[PANEL_KEYMAP_PW] = app_hexstr2value(value);
						key_flag |= (1 << PANEL_KEYMAP_PW);
						GXxmlFree(value);
					}
				}
				else
					;

				GXxmlFree(iname);
				iname = NULL;

				if((key_flag & flag_temp) == flag_temp)
				{
					GXxmlFreeDoc(doc);
					return GXCORE_SUCCESS;
				}

				item = item->next;
			}
		}
		group = group->next;
	}

	GXxmlFreeDoc(doc);
	return GXCORE_ERROR;
}

//plz free key_val if result > 0
int app_get_keymap_data(const char *key_name, unsigned int **key_val)
{
#define KEYMAP_STR  (uint8_t*)"keymap"
#define KEYMAP_GROUP  (uint8_t*)"group"
#define KEYMAP_NAME  (uint8_t*)"name"

    char stringArray[2][32] = {
        "/dvb/theme/keymap.xml",
        "/home/gx/learn_rcu/keymap.xml"
    };

	GXxmlDocPtr doc;
    int ret = 0;

    if((key_name == NULL) || (key_val == NULL))
    {
        return 0;
    }

    int i = 0;
    for(i = 0; i < 2; i++)
    {
        if(stringArray[i][0] == '\0')
            continue;
        if(GXCORE_FILE_UNEXIST == GxCore_FileExists(stringArray[i]))
            continue;
        doc = GXxmlParseFile(stringArray[i]);
        if (NULL == doc)
        {
            return 0;
        }

        // root
        GXxmlNodePtr root = doc->root;
        if (root == NULL)
        {
            GXxmlFreeDoc(doc);
            return 0;
        }

        if (0 !=  GXxmlStrcmp(root->name, KEYMAP_STR))
        {
            GXxmlFreeDoc(doc);
            return 0;
        }

        // group
        GXxmlNodePtr group = root->childs;
        while (NULL != group)
        {
            GXxmlNodePtr item = NULL;
            uint8_t *value = NULL;
            uint8_t *iname = NULL;

            if (GXxmlStrcmp(group->name, KEYMAP_GROUP) == 0)
            {
                item =  group->childs;
                while (item != NULL)
                {
                    iname = GXxmlGetProp(item, KEYMAP_NAME);
                    if(iname == NULL)
                    {
                        item = item->next;
                        continue;
                    }

                    if(GXxmlStrcmp(iname, (uint8_t*)key_name) == 0)
                    {
                        value = GXxmlNodeGetContent(item);
                        if(value != NULL)
                        {
                            unsigned int *temp = NULL;
                            temp = (unsigned int*)GxCore_Realloc(*key_val, (ret + 1) * sizeof(unsigned int));
                            if(temp != NULL)
                            {
                                *key_val = temp;
                                (*key_val)[ret] = app_hexstr2value(value);
                                //app_log_debug("##### value: 0x%x ######\n", ((*key_val)[ret]));
                                ret++;
                            }
                            GXxmlFree(value);
                            value = NULL;
                            GXxmlFree(iname);
                            iname = NULL;
                            break; //group = group->next;
                        }
                    }
                    else
                        ;

                    GXxmlFree(iname);
                    iname = NULL;

                    item = item->next;
                }
            }
            group = group->next;
        }

        GXxmlFreeDoc(doc);
    }
    return ret;
}

status_t app_panel_get(app_panel_type type, char *buf, size_t size)
{
    return bsp_parser(APP_PANEL_PATH, &panel_item[type], buf, size, 0);
}

status_t app_panel_get_int(app_panel_type type, int32_t *result)
{
#define BUF_LEN 256
    char buf[BUF_LEN] = {0};

    if (app_panel_get(type, buf, BUF_LEN) != GXCORE_SUCCESS) {
        return GXCORE_ERROR;
    }

    *result = atoi(buf);
    return GXCORE_SUCCESS;
}

status_t app_panel_get_list_value(app_panel_type type, char *buf, size_t size)
{
    return bsp_parser(APP_PANEL_PATH, &panel_item[type], buf, size, 1);
}


static status_t app_get_panel_key_scancode(uint32_t *keyvalue)
{
    char *panel_key_name[PANEL_KEYMAP_TOTAL] = {
        "KEY_MENU",
        "KEY_EXIT",
        "KEY_OK",
        "KEY_UP",
        "KEY_DOWN",
        "KEY_LEFT",
        "KEY_RIGHT",
        "KEY_PW",
    };
    return app_get_bsp_key_list(APP_PANEL_PATH, panel_key_name, &panel_item[PANEL_KEY_VALUE], keyvalue, PANEL_KEYMAP_TOTAL);
}

static status_t app_get_led_bit_value(unsigned int *keyvalue)
{

#define LED_BIT 8
    char *led_name[LED_BIT] = {
        "BIT_A",
        "BIT_B",
        "BIT_C",
        "BIT_D",
        "BIT_E",
        "BIT_F",
        "BIT_G",
        "BIT_P",
    };
    return app_get_bsp_key_list(APP_PANEL_PATH, led_name, &panel_item[PANEL_LED_VALUE], keyvalue, LED_BIT);
}
static status_t app_get_digtal_value(unsigned int *keyvalue)
{

#define DIGTAL_MAX 4
    char *digtal_name[DIGTAL_MAX] =
    {
        "DIGTAL_0",
        "DIGTAL_1",
        "DIGTAL_2",
        "DIGTAL_3",
    };
    return app_get_bsp_key_list(APP_PANEL_PATH, digtal_name, &panel_item[PANEL_DIGTAL_VALUE], keyvalue, DIGTAL_MAX);
}

static void app_panel_init(panel_parameter *panel_init)
{
    int32_t init_value;

    app_panel_get_int(PANEL_BOARD_TYPE, &init_value);
    panel_init->board_type = init_value;
    app_panel_get_int(PANEL_LED_CONVERSE, &init_value);
    panel_init->led_converse = init_value;
    app_panel_get_int(PANEL_LED_LOCK, &init_value);
    panel_init->lock_led = init_value;
    app_panel_get_int(PANEL_LED_STANDBY, &init_value);
    panel_init->standby_led = init_value;
    app_get_led_bit_value(panel_init->led_bit);
    app_get_panel_key_scancode(panel_init->scan_code);
    app_get_digtal_value(panel_init->digtal_value);
    app_panel_get_int(PANEL_LONGKEY_VALUE, &init_value);
    panel_init->longkey_enable= init_value;

    //	app_log_debug("board_type=%d,led_converse=%d,lock_led=%d,standby_led=%d,\n",panel_init->board_type,panel_init->led_converse,panel_init->lock_led,panel_init->standby_led);
}

int app_panel_ioctl_handle(unsigned int cmd, void *value)
{
    int ret=0;
    if (bsp_panel_handle <= 0)
    {
        return -1;
    }

    ioctl(bsp_panel_handle, cmd, value);

    return ret;
}

int app_bsp_panel_init(void)
{
    int i;
    uint16_t keycode = 0;
    uint32_t panel_keymap[PANEL_KEYMAP_TOTAL] = {0};
    panel_parameter panel;
    PanelGpioMap panel_gpio;
    memset(&panel,0,sizeof(panel_parameter));
    app_get_panel_gpiomap(&panel_gpio);
    app_get_panel_keymap();
	bsp_panel_handle = GxCore_Open(PANEL_NAME, "r");
	if (bsp_panel_handle <= 0)
	{
	    app_log_error("\033[1;31m[PANEL] open failed!\n\033[0m");
	}

    for(i = 0; i < PANEL_KEYMAP_TOTAL; i++)
    {
        keycode = app_get_panel_keycode(i);
        panel_keymap[i] = app_key_full_code(keycode);
    }

    app_panel_init(&panel);
    memcpy(panel.virtual_key,panel_keymap,sizeof(uint32_t)*PANEL_KEYMAP_TOTAL);
    app_panel_ioctl_handle(PANEL_SET_GPIOMAP, &panel_gpio);
    app_panel_ioctl_handle(PANEL_SET_KEYMAP, &panel);

#if (PANEL_SETTING_SUPPORT)
    int led_brightness = 0;
    GxBus_ConfigGetInt(PANEL_LED_BRIGHTNESS_KEY, &led_brightness, PANEL_LED_BRIGHTNESS);
    app_panel_ioctl_handle(PANEL_SHOW_BRIGHTNESS, &led_brightness);
#endif

    return 0;
}

int app_bsp_panel_close(void)
{
    if (bsp_panel_handle >= 0)
        GxCore_Close(bsp_panel_handle);

    return 0;
}

uint32_t app_get_panel_key(void)
{
    unsigned int scan_code[PANEL_KEYMAP_TOTAL];
    char *panel_key_name[1] = {
        "KEY_PW"
    };

    if (0 == app_get_bsp_key_list(APP_PANEL_PATH,panel_key_name, &panel_item[PANEL_KEY_VALUE], scan_code, 1)) {
        return scan_code[0];
    }

    return 0;
}


