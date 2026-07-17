/*************************************************************************
	> File Name: app_gpio.h
	> Author:
	> Mail:
	> Created Time: 2018年07月20日 星期五 18时41分48秒
 ************************************************************************/

#ifndef __APP_GPIO_H__
#define __APP_GPIO_H__


#define V_GPIO_PANEL_SDA    0
#define V_GPIO_PANEL_SCL    1
#define V_GPIO_PANEL_KEY    2
#define V_GPIO_MUTE         3
#define V_GPIO_LNB_B        4
#define V_GPIO_LNB_A        5
#define V_GPIO_TUNER_RST    6
#define V_GPIO_NET_RST      7
#define V_GPIO_USB_A        8
#define V_GPIO_USB_B        9
#define V_GPIO_TUNER2_RST   10
#define V_GPIO_POWER_CTL    16


#if DEMOD_DVB_S && DEMOD_DVB_T && NDEMOD_WITH_1TUNER
#define V_GPIO_POLAR        13
#endif

#define V_GPIO_ANT_POWER  	15
#define V_GPIO_LNB_A_SHORT   20
#define V_GPIO_LNB_B_SHORT   21

#define V_GPIO_SCART_RGB      17     /*SCART RGB*/
#define V_GPIO_SCART_SWITCH   18/*SCART ON/OFF*/
#define V_GPIO_SCART_TV       19    /*SCART 16:9 /4:3*/
#define V_GPIO_REMOTE       30    //remote led (flicker with key), 0:light up, 1:light off

#define V_GPIO_VCONT_1      31
#define V_GPIO_VCONT_2      32


typedef enum
{
    GPIO_POWERCUT,
    GPIO_PANEL,
    GPIO_MUTE,
    GPIO_REMOTE_LED,
    GPIO_XTAL,
    GPIO_GPIO_LEVEL,
    GPIO_PORT_NUM,
    GPIO_TYPE_MAX
}app_gpio_type;

extern int app_bsp_gpio_init(void);
extern int app_bsp_gpio_close(void);
extern int app_set_hardware_mute(void);
extern int app_set_hardware_unmute(void);
extern int app_power_gpio_set_off(void);
extern int app_power_gpio_set_on(void);
extern int app_get_panel_gpiomap(PanelGpioMap *panel_gpio);
extern int app_gpio_tuner_reset(int32_t tuner, int32_t value);
extern status_t app_get_gpio_config(app_gpio_type type, char *buf, size_t size);
extern status_t app_panel_remote_led_show(bool state);
extern uint32_t app_bsp_get_gpio_port(uint32_t *mask,uint32_t *data);
extern handle_t app_get_gpio_handle(void);

extern int app_lnb_gpio_set_off(int32_t tuner);
extern int app_lnb_gpio_set_on(int32_t tuner);
extern int app_gpio_lnb_power_get(int32_t index);
#if LNB_SHORT_DETECT_SUPPORT
extern int app_gpio_lnb_short_get(int32_t index);
#endif

#ifdef HAVE_LIB_CIM
extern int app_ci_gpio_setlevel(unsigned long gpio, unsigned long value);
extern int app_ci_gpio_getlevel(unsigned long gpio);
extern int app_ci_gpio_setio(unsigned long gpio, int value);
#endif

#if DEMOD_DVB_S && DEMOD_DVB_T && NDEMOD_WITH_1TUNER
extern int app_gpio_pin_fun_switch(MulPin pin);
extern int32_t app_gpio_polar_set(uint32_t on);
#endif

#ifdef SUPPORT_SCART
void app_gpio_set_scart_rgb(int sel);
void app_gpio_set_scart_onoff(int sel);
void app_gpio_set_scart_tv(int sel);
#endif
#if (DVBT2_ANT_POWER > 0)
int app_gpio_ant_power_set(int ant_power);
unsigned int app_gpio_ant_power_get(void);
#endif
#endif
