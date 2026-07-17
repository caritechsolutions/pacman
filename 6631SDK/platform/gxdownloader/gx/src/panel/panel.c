#include "panel/gxota_panel.h"
#include "gpio.h"
#include "panel.h"
#include "time.h"
#include "gxdlp_api.h"
#include "config.h"

int32_t s_panel_key = 0;
static panel_t *gpanel;
static int32_t panel_scan(void)
{
	if (gpanel->isr)
		gpanel->isr();
	return 0;
}

static int32_t panel_set_ledstring(unsigned char *pString)
{
	if (gpanel->set_led_string )
		gpanel->set_led_string(pString);
	return 0;

}
int32_t panel_register(panel_t *Panel)
{
	if(Panel == NULL)
		return -1;
	gpanel = Panel;
	if(gpanel->config)
		gpanel->config();
	if (gpanel->init_pin)
		gpanel->init_pin();
	if (gpanel->isr)
		gpanel->isr();
	panel_printf("\npanel_Registered\n");
	return 0;
}

int32_t panel_updatetype(void)
{
	int16_t key = 0;
	volatile unsigned int i = CONFIG_CHECK_TIME;
	if(gpanel == NULL)
		return -1;
	do
	{
		panel_scan();
		mdelay(1);
		key = panel_readkey();
		switch(key)
		{
			case PANEL_KEY_OK:
				return GXDLP_OTA_DDTEX_DOWNLOAD;
			case PANEL_KEY_MENU:
				return GXDLP_USB_DOWNLOAD;
			default:
				break;
		}
	}while(i--);

	return -1;
}

int32_t panel_scanmanager()
{
	if(gpanel == NULL)
		return -1;
	if (gpanel->scan_manager)
		gpanel->scan_manager();
	return 0;
}

int32_t panel_lock(void)
{
	if(gpanel == NULL)
		return -1;
	if (gpanel->lock)
		gpanel->lock();
	return 0;
}
int32_t panel_unlock(void)
{
	if(gpanel == NULL)
		return -1;
	if (gpanel->unlock)
		gpanel->unlock();
	return 0;
}

int32_t panel_poweroff(void)
{
	if(gpanel == NULL)
		return -1;
	if(gpanel->power_off)
		gpanel->power_off();
	return 0;
}

int32_t panel_set_signal(uint32_t wwww)
{
	if(gpanel == NULL)
		return -1;
	if (gpanel->set_led_signal_value)
		gpanel->set_led_signal_value(wwww);
	return 0;
}
int32_t panel_showvalue(uint32_t wwww)
{
	if(gpanel == NULL)
		return -1;
	if (gpanel->set_led_value)
		gpanel->set_led_value(wwww);
	return 0;
}

int32_t panel_showstring(unsigned char *pString)
{
	if(gpanel == NULL)
		return -1;
	if(pString == NULL)
		return -1;
	panel_set_ledstring(pString);
	panel_scan();
	return 0;
}
int32_t panel_readkey(void)
{
	uint8_t panelKey = s_panel_key;
	switch(s_panel_key)
	{
		case PANEL_KEY_POWER:
			break;
		case PANEL_KEY_MENU:
			break;
		case PANEL_KEY_UP:
			break;
		case PANEL_KEY_DOWN:
			break;
		case PANEL_KEY_LEFT:
			break;
		case PANEL_KEY_RIGHT:
			break;
		case PANEL_KEY_OK:
			break;
		case PANEL_KEY_EXIT:
			break;
		default:
			break;
	}
	
	s_panel_key =0;
	return panelKey;
}

int32_t panel_set_mode(panelmode_t PanelMode)
{
	if(gpanel == NULL)
		return -1;
	if (gpanel->show_mode)
		gpanel->show_mode(PanelMode);
	return 0;
}

int32_t panel_standby(void)
{
	if(gpanel == NULL)
		return -1;
	if(gpanel->stand_by)
		gpanel->stand_by();
	return 0;
}

int32_t panel_wakeup(void)
{
	if(gpanel == NULL)
		return -1;
	if(gpanel->wake_up)
		gpanel->wake_up();
	return 0;
}

