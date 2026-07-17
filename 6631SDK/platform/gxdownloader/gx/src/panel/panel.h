#ifndef __PANEL_API_H__
#define __PANEL_API_H__

typedef enum led_s {
	GXLED_BOOT=0,
	GXLED_INIT,
	GXLED_ON,
	GXLED_OFF,
	GXLED_E,
	GXLED_P,
	GXLED_USB,
	GXLED_SEAR,
	GXLED_OTA,
	GXLED_HIDE,
}led_t;

typedef enum panelmode_s {
	PANEL_SHOW_NORMAL = 0,
	PANEL_SHOW_OFF,
	PANEL_SHOW_TIME
}panelmode_t;

typedef struct panel_s {
	uint32_t (*config)(void);
	void (*init_pin)(void);
	void (*isr)(void);
	void (*set_led_value)(uint32_t);
	void (*set_led_signal_value)(uint32_t);
	void (*set_led_string)(unsigned char*);
	void (*lock)(void);
	void (*unlock)(void);
	void (*stand_by)(void);
	void (*wake_up)(void);
	void (*show_mode)(panelmode_t);
	void (*scan_manager)(void);
	void (*power_off)(void);
}panel_t;

extern panel_t ct1642_haixin;
extern panel_t ct1642_jinya;
extern panel_t fd650_runde;

int32_t panel_register(panel_t *Panel);
int32_t panel_showvalue(uint32_t wwww);
int32_t panel_readkey(void);
int32_t panel_showstring(unsigned char *pString);
int32_t panel_updatetype(void);
#endif

