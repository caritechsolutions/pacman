#include "gxota_panel.h"
#include "gpio.h"

#define OPEN_MMU    1
GXPANEL_PUBLIC_Config_t g_PanelPublicConfig;
static void gxota_panel_cfg_multiplex(uint64_t nCfgPin)
{
	return;
}

static void gxota_panel_set_gpio_level(uint8_t eport, uint8_t nLevel)
{
	int num = eport;
	int level = nLevel;
	gx_gpio_setlevel(num,level);
	return ;
}

static void gxota_panel_get_gpio_level(uint8_t eport, uint64_t *nLevel)
{
	int num = eport;

	int lev;
	lev = gx_gpio_getlevel(num);
	*nLevel = lev;
	return;
}

static void gxota_panel_set_gpio_output(uint8_t eport)
{
	int num = eport;
	int ret = -1;
	ret = gx_gpio_setio(num,1);  

	return;
}

static void gxota_panel_set_gpio_input(uint8_t eport)
{
	int num = eport;
	gx_gpio_setio(num,0); 

	return;
}

static void gxota_panel_gpio_init(uint64_t nCfgPin)
{
	gxota_panel_cfg_multiplex(nCfgPin);
}

static void gxota_panel_gpio_open(uint8_t chGpio,GpioOutput_t GpioOutput)
{
	if(GPIO_INPUT == GpioOutput)
	{
		gxota_panel_set_gpio_input(chGpio);
	}
	else
	{
		gxota_panel_set_gpio_output(chGpio);
		gxota_panel_set_gpio_level(chGpio, 1);
	}
}

static void gxota_panel_deal_key(uint32_t nKeyIndex)
{
	return;	
	//panel_printf("[panel] gxota_panel_deal_key s_panel_key :%d= %d \n", nKeyIndex,s_panel_key);
}

void gxota_panel_public_config(void)
{
	g_PanelPublicConfig.m_PanelCfgMultiplexFun = (void *)gxota_panel_cfg_multiplex;
	g_PanelPublicConfig.m_PanelSetGpioOutPutFun = (void *)gxota_panel_set_gpio_output;
	g_PanelPublicConfig.m_PanelSetGpioInPutFun = (void *)gxota_panel_set_gpio_input;
	g_PanelPublicConfig.m_PanelSetGpioLevelFun = (void *)gxota_panel_set_gpio_level;
	g_PanelPublicConfig.m_PanelGetGpioLevelFun = (void *)gxota_panel_get_gpio_level;
	g_PanelPublicConfig.m_PanelInitGpioFun = (void *)gxota_panel_gpio_init;
	g_PanelPublicConfig.m_PanelOpenGpioFun = (void *)gxota_panel_gpio_open;
	g_PanelPublicConfig.m_PanelDealKey = (void *)gxota_panel_deal_key;
}


