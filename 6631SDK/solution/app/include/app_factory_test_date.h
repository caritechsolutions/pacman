#ifndef __APP_FACTORY_TEST_DATE_H__
#define __APP_FACTORY_TEST_DATE_H__

/*
Declaration: 用于声明我们自己定义的数据结构和函数声明

*/

typedef struct frontpara{
	unsigned int switch22k;
	unsigned int lnb_power;
} FRONTPARA;

enum _threadflag
{
	ThreadFlag,
	ThreadHandle,
};

enum _threadstate
{
	NullThread = 0,
	EnableThread,
	WaitThread,
	DisableThread,

};

extern void destroy_thread(int * pthread_id);


extern int uart_thread[];
extern int ca_thread[];
extern int sg_NetworkTaskThread[];
#if FACTORY_TEST_SUPPORT
//extern void MKTechSetLed(char * pstr,int iFlag);

extern int Gui_WriteKeyBuf(unsigned int key);
extern void app_test_send_key(unsigned int key);
extern int app_test_auto_search_timer_remove(void);
extern void app_test_sort_channels_by_fta(void);

extern void app_set_diseqc_10(uint32_t tuner, AppFrontend_DiseqcParameters *diseqc10, bool force);
extern void app_factory_scart_gpio_init(void);
extern void app_factory_scart_mute_control(int32_t mute);
extern char * app_system_info_get_sw_ver(void);
extern status_t app_factory_test_add_sat( unsigned int tuner , FRONTPARA *para, unsigned int * psat_id);
extern status_t app_factory_test_add_tp(uint32_t sat_id,uint32_t * ptp_id);
extern void app_system_reset(void);
#endif

extern  int network_check_cable_link(void);
#endif

