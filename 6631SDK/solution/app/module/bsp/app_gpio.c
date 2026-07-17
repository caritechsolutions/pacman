/*************************************************************************
	> File Name: app_gpio.c
	> Author:
	> Mail:
	> Created Time: 2018年07月20日 星期五 15时01分37秒
 ************************************************************************/

#include "gxcore.h"
#include "tree.h"
#include "parser.h"
#include "app_module.h"
#include "app.h"
#include "app_bsp.h"
#include <fcntl.h>
#include <stdlib.h>
#include "devapi/gxserial_api.h"

static BspItem gpio_item[] = {
    { "powercut",   "powercut"},
    { "panel", "panel" },
    { "mute",     "mute" },
    { "remote led", "remote led" },
    { "xtal",     "xtal" },
    { "gpio level", "gpio level"},
    { "port", "port"},
    { NULL,    NULL},
};
#define APP_GPIO_PATH WORK_PATH"theme/gpio.xml"
#define CVBS_MUTE_SERIAL_PORT GXSERIAL_UART1

static handle_t bsp_gpio_handle;


static bool s_gpio_config_mute = false;
static bool s_gpio_config_remote_led = false;
static uint32_t s_gpio_config_mute_level = 0;
static uint32_t s_gpio_config_remote_led_on_level = 0;
static uint32_t s_gpio_config_lnb_power_level = 0;
#if LNB_SHORT_DETECT_SUPPORT
static uint32_t s_gpio_config_lnb_short_level = 1;
#endif
static event_list* s_panel_remote_led_timer = NULL;


handle_t app_get_gpio_handle(void)
{
    return bsp_gpio_handle;
}

status_t app_get_gpio_config(app_gpio_type type, char *buf, size_t size)
{
    return bsp_parser(APP_GPIO_PATH, &gpio_item[type], buf, size, 0);
}

static void _serial2_cvbs_mute_config(bool flag) //false: mute off; true: mute on
{
    static int uart_fd = -1;
    if((uart_fd != -1) || ((uart_fd = GxSerial_Open(CVBS_MUTE_SERIAL_PORT, GXSERIAL_MODE_NONBLOCK)) > 0))
    {
        if(flag == false)
            GxSerial_StartBreak(uart_fd);
        else
            GxSerial_EndBreak(uart_fd);
    }// GxSerial_Close(uart_fd);//linux 关闭uart_fd会导致GxSerial_EndBreak无效
    return;
}

int app_gpio_tuner_reset(int32_t tuner, int32_t invert)
{
	GpioData gpio = {0};
	uint8_t tuner_gpio[NIM_MODULE_NUM] = {V_GPIO_TUNER_RST, V_GPIO_TUNER2_RST};

	if(tuner >= NIM_MODULE_NUM)
		return -1;

	gpio.port = tuner_gpio[tuner];
	ioctl(bsp_gpio_handle, GPIO_OUTPUT, &gpio);
	if(invert)
		ioctl(bsp_gpio_handle, GPIO_HIGH, &gpio);
	else
		ioctl(bsp_gpio_handle, GPIO_LOW, &gpio);

	GxCore_ThreadDelay(100);

	if(invert)
		ioctl(bsp_gpio_handle, GPIO_LOW, &gpio);
	else
		ioctl(bsp_gpio_handle, GPIO_HIGH, &gpio);

	return 0;
}

static status_t app_parse_gpio_config(void)
{
#define BUF_SIZE 10
    char buf[BUF_SIZE] = {0};

    bsp_parser(APP_GPIO_PATH, &gpio_item[GPIO_MUTE], buf, BUF_SIZE, 0);
    if(!strcmp(buf,"enable"))
    {
        char *mute_level[1] = {"mute"};

        s_gpio_config_mute = true;
        app_get_bsp_key_list(APP_GPIO_PATH,mute_level, &gpio_item[GPIO_GPIO_LEVEL], &s_gpio_config_mute_level, 1);
    }
    bsp_parser(APP_GPIO_PATH, &gpio_item[GPIO_REMOTE_LED], buf, BUF_SIZE, 0);
    if(!strcmp(buf,"enable"))
    {
        char *remote_led_on_level[1] = {"remote led on"};

        s_gpio_config_remote_led = true;
        app_get_bsp_key_list(APP_GPIO_PATH, remote_led_on_level, &gpio_item[GPIO_GPIO_LEVEL], &s_gpio_config_remote_led_on_level, 1);
    }
    char *lnb_power_level[1] = {"lnb power"};
    app_get_bsp_key_list(APP_GPIO_PATH, lnb_power_level, &gpio_item[GPIO_GPIO_LEVEL], &s_gpio_config_lnb_power_level, 1);
#if LNB_SHORT_DETECT_SUPPORT
    char *lnb_short_level[1] = {"lnb short"};
    app_get_bsp_key_list(APP_GPIO_PATH, lnb_short_level, &gpio_item[GPIO_GPIO_LEVEL], &s_gpio_config_lnb_short_level, 1);
#endif

    return GXCORE_SUCCESS;
}

uint32_t app_bsp_get_gpio_port(uint32_t *mask,uint32_t *data)
{
#define MAX_BUF_SIZE 64
    char buffer[MAX_BUF_SIZE] = {0};
    char temp_buf[MAX_BUF_SIZE] = {0,0,};
	uint32_t ret = 0,j=0;
	uint32_t value_port = 0;
	uint32_t mask_port = 0;
    uint32_t level = 0;
    char *p;

    memset(buffer,0,sizeof(buffer));
    ret = bsp_parser(APP_GPIO_PATH, &gpio_item[GPIO_PORT_NUM], buffer, MAX_BUF_SIZE, 0);
    if(ret == 0)
    {
        temp_buf[j] = atoi(buffer);
        p = buffer;
        for(j=0;j<MAX_BUF_SIZE;j++)
        {
            if(j==0)
            {
                temp_buf[j] = atoi(buffer);
                p = strchr(p, ':');
                if(p==NULL)
                    break;
                p++;
                level = atoi(p);
                p++;
            }
            else
            {
                if(buffer[j]!='\0')
                {
                    p = strchr(p, ',');
                    if(p==NULL)
                        break;
                    p++;
                    temp_buf[j] = atoi(p);
                    p = strchr(p, ':');
                    if(p==NULL)
                        break;
                    p++;
                    level = atoi(p);
                    p++;

                }
            }

            mask_port  |= (1<<temp_buf[j]);
            value_port |= (level<<temp_buf[j]);
        }
    }
    *mask = ((~mask_port)&0xffffffff);
    *data = (value_port&0xffffffff);
	return ret;
}

int app_bsp_gpio_init(void)
{
    bsp_gpio_handle = GxCore_Open(GPIO_NAME, "r");
    if (bsp_gpio_handle <= 0)
    {
        app_log_error("[gpio] open failed!\n");
    }
    app_parse_gpio_config();
    return 0;
}

int app_bsp_gpio_close(void)
{
    if (bsp_gpio_handle >= 0)
        GxCore_Close(bsp_gpio_handle);

    return 0;
}

int app_set_hardware_mute(void)
{
 // mute
    if(s_gpio_config_mute)
    {
        GpioData _gpio;
        _gpio.port = V_GPIO_MUTE;
        ioctl(bsp_gpio_handle, GPIO_OUTPUT, &_gpio);
        if(s_gpio_config_mute_level)
            ioctl(bsp_gpio_handle, GPIO_HIGH, &_gpio);
        else
            ioctl(bsp_gpio_handle, GPIO_LOW, &_gpio);
    }
    else
    {
        _serial2_cvbs_mute_config(true);
    }
    return 0;
}

int app_set_hardware_unmute(void)
{
 // unmute
    if(s_gpio_config_mute)
    {
        GpioData _gpio;
        _gpio.port = V_GPIO_MUTE;
        ioctl(bsp_gpio_handle, GPIO_OUTPUT, &_gpio);
        if(s_gpio_config_mute_level)
            ioctl(bsp_gpio_handle, GPIO_LOW, &_gpio);
        else
            ioctl(bsp_gpio_handle, GPIO_HIGH, &_gpio);
    }
    else
    {
        _serial2_cvbs_mute_config(false);
    }
    return 0;
}

int app_power_gpio_set_off(void)
{
    GpioData val = {0};

    app_lnb_gpio_set_off(0);
    app_lnb_gpio_set_off(1);

    val.port = V_GPIO_USB_A;
    ioctl(bsp_gpio_handle, GPIO_OUTPUT, &val);
    ioctl(bsp_gpio_handle, GPIO_LOW, &val);

    val.port = V_GPIO_USB_B;
    ioctl(bsp_gpio_handle, GPIO_OUTPUT, &val);
    ioctl(bsp_gpio_handle, GPIO_LOW, &val);

    return 0;
}

int app_power_gpio_set_on(void)
{
    GpioData val = {0};

    app_lnb_gpio_set_on(0);
    app_lnb_gpio_set_on(1);

    val.port = V_GPIO_USB_A;
    ioctl(bsp_gpio_handle, GPIO_OUTPUT, &val);
    ioctl(bsp_gpio_handle, GPIO_HIGH, &val);

    val.port = V_GPIO_USB_B;
    ioctl(bsp_gpio_handle, GPIO_OUTPUT, &val);
    ioctl(bsp_gpio_handle, GPIO_HIGH, &val);

    return 0;
}

int app_lnb_gpio_set_off(int32_t tuner)
{
    GpioData val = {0};
    uint8_t tuner_gpio[NIM_MODULE_NUM] = {V_GPIO_LNB_A, V_GPIO_LNB_B};

    if(tuner >= NIM_MODULE_NUM)
        return 0;

    val.port = tuner_gpio[tuner];
    ioctl(bsp_gpio_handle, GPIO_OUTPUT, &val);
    if(1 == s_gpio_config_lnb_power_level)
        ioctl(bsp_gpio_handle, GPIO_HIGH, &val);
    else
        ioctl(bsp_gpio_handle, GPIO_LOW, &val);

    return 0;
}

int app_lnb_gpio_set_on(int32_t tuner)
{
    GpioData val = {0};
    uint8_t tuner_gpio[NIM_MODULE_NUM] = {V_GPIO_LNB_A, V_GPIO_LNB_B};

    if(tuner >= NIM_MODULE_NUM)
        return 0;

    val.port = tuner_gpio[tuner];
    ioctl(bsp_gpio_handle, GPIO_OUTPUT, &val);
    if(1 == s_gpio_config_lnb_power_level)
        ioctl(bsp_gpio_handle, GPIO_LOW, &val);
    else
        ioctl(bsp_gpio_handle, GPIO_HIGH, &val);

    return 0;
}

//0:off 1:on
int app_gpio_lnb_power_get(int32_t index)
{
    int ret = 1;

    GpioData val = {0};
    uint8_t tuner_gpio[NIM_MODULE_NUM] = {V_GPIO_LNB_A, V_GPIO_LNB_B};

    if(index >= NIM_MODULE_NUM)
        return 0;

    val.port = tuner_gpio[index];
    ioctl(bsp_gpio_handle, GPIO_READ, &val);
    if(val.value == s_gpio_config_lnb_power_level)
        ret = 0;
    return ret;
}

#if LNB_SHORT_DETECT_SUPPORT
//0:normal 1:short
int app_gpio_lnb_short_get(int32_t index)
{
    int ret = 0;
    GpioData val = {0};
    uint8_t tuner_gpio[NIM_MODULE_NUM] = {V_GPIO_LNB_A_SHORT, V_GPIO_LNB_B_SHORT};

    if(index >= NIM_MODULE_NUM)
        return 0;

    val.port = tuner_gpio[index];
    ioctl(bsp_gpio_handle, GPIO_READ, &val);
    if(val.value == s_gpio_config_lnb_short_level)
        ret = 1;
    return ret;
}
#endif

#ifdef HAVE_LIB_CIM
int app_ci_gpio_setlevel(unsigned long gpio, unsigned long value)
{

    GpioData val = {0};
    val.port = gpio;
	if(value)
    	ioctl(bsp_gpio_handle, GPIO_HIGH, &val);
	else
    	ioctl(bsp_gpio_handle, GPIO_LOW, &val);

    return 0;
}

int app_ci_gpio_getlevel(unsigned long gpio)
{

    GpioData val = {0};
    val.port = gpio;
    ioctl(bsp_gpio_handle, GPIO_READ, &val);
	return val.value;

}

int app_ci_gpio_setio(unsigned long gpio, int value)
{
    GpioData val = {0};
    val.port = gpio;
	if(value)
    	ioctl(bsp_gpio_handle, GPIO_OUTPUT, &val);
	else
    	ioctl(bsp_gpio_handle, GPIO_INPUT, &val);

    return 0;
}
#endif

#if DEMOD_DVB_S && DEMOD_DVB_T && NDEMOD_WITH_1TUNER
/*板子pin脚功能查看gxloader里对应板级的board_init.c,以下面chip_core_num=32的pin脚为例展示接口使用
 * chip_core_num | bit0 | bit1    | bit2       | func_select      // package_num | func_list
 *{32,              10,    37,       69,          0},             // NC          | DiSEqCi_PORT10_TSI1DATA5_ADCOCLK
 *
 *      pin.sel0 = 10; bit0
 *      pin.sel1 = 37; bit1
 *      pin.sel2 = 69; bit2
 *      pin.fun  = 2;  TSI1DATA5 func
 *      app_gpio_pin_fun_switch(pin);
 * */
int app_gpio_pin_fun_switch(MulPin pin)
{
    return ioctl(bsp_gpio_handle, GPIO_PIN_FUN_SWITCH, &pin);
}

int32_t app_gpio_polar_set(uint32_t hv)
{
    GpioData val = {0};
    val.port = V_GPIO_POLAR;

    ioctl(bsp_gpio_handle, GPIO_OUTPUT, &val);
    if(0 == hv)
        ioctl(bsp_gpio_handle, GPIO_LOW, &val);
    else
        ioctl(bsp_gpio_handle, GPIO_HIGH, &val);

    return val.value;
}
#endif
int app_get_panel_gpiomap(PanelGpioMap *panel_gpio)
{
    panel_gpio->panel_gpio_sda=V_GPIO_PANEL_SDA;
    panel_gpio->panel_gpio_scl=V_GPIO_PANEL_SCL;
    panel_gpio->panel_gpio_key=V_GPIO_PANEL_KEY;

    return 0;
}

static int app_remote_led_show_off(void* userdata)
{
    APP_TIMER_REMOVE(s_panel_remote_led_timer);

    GpioData data = {0};
    data.port = V_GPIO_REMOTE;
    ioctl(bsp_gpio_handle, GPIO_OUTPUT, &data);
    if(s_gpio_config_remote_led_on_level)
        ioctl(bsp_gpio_handle, GPIO_LOW, &data);
    else
        ioctl(bsp_gpio_handle, GPIO_HIGH, &data);

    return 0;
}

static int app_remote_led_show_on(void)
{
    GpioData data = {0};
    data.port = V_GPIO_REMOTE;
    ioctl(bsp_gpio_handle, GPIO_OUTPUT, &data);
    if(s_gpio_config_remote_led_on_level)
        ioctl(bsp_gpio_handle, GPIO_HIGH, &data);
    else
        ioctl(bsp_gpio_handle, GPIO_LOW, &data);

    return 0;

}

status_t app_panel_remote_led_show(bool state)
{
    if(s_gpio_config_remote_led)
    {
        if(state)
        {
            APP_TIMER_ADD(s_panel_remote_led_timer, app_remote_led_show_off, 300, TIMER_REPEAT);
            app_remote_led_show_on();
        }
        else
        {
            app_remote_led_show_off(NULL);
        }
    }

    return GXCORE_SUCCESS;
}

#ifdef SUPPORT_SCART
void app_gpio_set_scart_rgb(int sel)
{
    GpioData _gpio = {0};
    VideoOutputScartMode config = GX_SCART_CVBS;
    _gpio.port = V_GPIO_SCART_RGB;

    ioctl(bsp_gpio_handle, GPIO_OUTPUT, &_gpio);

    if(sel == 0)
    {
        config = GX_SCART_CVBS;
        ioctl(bsp_gpio_handle, GPIO_HIGH, &_gpio);//CVBS
    }
    else if(sel == 1)
    {
        config = GX_SCART_RGB;
        ioctl(bsp_gpio_handle, GPIO_LOW, &_gpio);//RGB
    }
    GxPlayer_SetScartMode(config);
}

 void app_gpio_set_scart_onoff(int sel)
{
    GpioData _gpio = {0};

    _gpio.port = V_GPIO_SCART_SWITCH;
    ioctl(bsp_gpio_handle, GPIO_OUTPUT, &_gpio);

    if(sel==1)
    {
        ioctl(bsp_gpio_handle, GPIO_HIGH, &_gpio);
    }
    else if(sel==0)
    {
        ioctl(bsp_gpio_handle, GPIO_LOW, &_gpio);
    }
}

void app_gpio_set_scart_tv(int sel)
{
    GpioData _gpio = {0};

    _gpio.port = V_GPIO_SCART_TV;
    ioctl(bsp_gpio_handle, GPIO_OUTPUT, &_gpio);

    if(sel==1)
    {
        ioctl(bsp_gpio_handle, GPIO_HIGH, &_gpio);
    }
    else if(sel==0)
    {
        ioctl(bsp_gpio_handle, GPIO_LOW, &_gpio);
    }
}

#endif

#if (DVBT2_ANT_POWER > 0)
int app_gpio_ant_power_set(int ant_power)
{
    GpioData _gpio ={0};

    _gpio.port = V_GPIO_ANT_POWER;
    ioctl(bsp_gpio_handle, GPIO_OUTPUT, &_gpio);
    if (ant_power)
    {
        ioctl(bsp_gpio_handle,GPIO_HIGH , &_gpio);
    }
    else
    {
        ioctl(bsp_gpio_handle, GPIO_LOW, &_gpio);
    }
   return 0;
}

unsigned int app_gpio_ant_power_get(void)
{
    unsigned int level = 0;
    GpioData _gpio ={0};

    _gpio.port = V_GPIO_ANT_POWER;
    ioctl(bsp_gpio_handle, GPIO_INPUT, &_gpio);
    ioctl(bsp_gpio_handle, GPIO_READ, &_gpio);
    level = _gpio.value;
    app_log_debug("level = %d \n",level);
    return level;
}
#endif

