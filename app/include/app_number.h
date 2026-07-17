#ifndef __APP_NUMBER_H__
#define __APP_NUMBER_H__

typedef int (*_ex_number_response_func)(unsigned int number_value);

extern void app_number_respones_ex_func_register(_ex_number_response_func response_func);
extern _ex_number_response_func app_number_respones_ex_func_get(void);
#endif
