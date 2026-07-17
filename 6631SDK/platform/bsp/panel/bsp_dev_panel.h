#ifndef __BSP_DEV_PANEL__
#define __BSP_DEV_PANEL__

#ifdef ECOS_OS
#ifdef __cplusplus

extern "C" void bsp_panel_mod_init(void);
class cyg_panel_init_class {
    public:
        cyg_panel_init_class(void)
        {
            bsp_panel_mod_init();
        }
};

#define MOD_PANEL  cyg_panel_init_class   panel0   CYGBLD_ATTRIB_INIT_PRI(60110);
#endif
#endif

#define PANEL_NAME "/dev/gxpanel0"
#define PANEL_END_TIME (~((unsigned int)0))


/* led string */
typedef enum
{
	PANEL_LED_ON = 0,
	PANEL_LED_OFF,
	PANEL_LED_BOOT,
	PANEL_LED_DARK,
	PANEL_LED_UPGRADE,
}PanelString;

/* led brigthness */
typedef enum
{
	PANEL_LED_BRIGTHNESS_LOW = 0,
	PANEL_LED_BRIGTHNESS_MID,
	PANEL_LED_BRIGTHNESS_HIG,
}PanelBrigthness;

/*keymap value*/
typedef enum
{
    PANEL_KEYMAP_MENU = 0,
    PANEL_KEYMAP_EXIT,
    PANEL_KEYMAP_OK,
    PANEL_KEYMAP_UP,
    PANEL_KEYMAP_DOWN,
    PANEL_KEYMAP_LEFT,
    PANEL_KEYMAP_RIGHT,
    PANEL_KEYMAP_PW,// must reserved
    PANEL_KEYMAP_TOTAL,
}PanelKeymap;

typedef struct _panel_parameter_
{
	unsigned int virtual_key[PANEL_KEYMAP_TOTAL];
	unsigned int scan_code[PANEL_KEYMAP_TOTAL];
	unsigned int led_bit[8];
	unsigned int board_type;
	unsigned int lock_led;
	unsigned int led_converse;
	unsigned int standby_led;
	unsigned int digtal_value[4];
	unsigned int longkey_enable;/*1 CH+- long press to L/R  Menu Long press to OK*/
} panel_parameter;

typedef struct
{
    int panel_gpio_sda;
    int panel_gpio_scl;
    int panel_gpio_key;
}PanelGpioMap;

#define PANEL_SHOW_STR		_IOW('x', 0x06, PanelString)
#define PANEL_SHOW_NUM		_IOW('x', 0x07, unsigned int)
#define PANEL_READ_KEY		_IOR('x', 0x08, unsigned int)
#define PANEL_SHOW_LOCK		_IOW('x', 0x09, unsigned int)
#define PANEL_SHOW_QUALITY	_IOW('x', 0x0a, unsigned int)
#define PANEL_SHOW_TIME		_IOW('x', 0x0b, unsigned int)
#define PANEL_SET_GPIOMAP	_IOW('x', 0x0c, PanelGpioMap)
#define PANEL_SET_KEYMAP	_IOW('x', 0x0d, panel_parameter)
#define PANEL_MCU_LOWPOWER	_IO ('x', 0x0e)
#define PANEL_SHOW_STANDBY	_IOW('x', 0x0f, unsigned int)
#define PANEL_SHOW_BRIGHTNESS	_IOW('x', 0x10, unsigned int)


#endif
