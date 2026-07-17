/*
 * =====================================================================================
 *
 *       Filename:  app_tools.h
 *
 *    Description:  app tools
 *
 *        Version:  1.0
 *        Created:
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */
#ifndef __APP_TOOLS_H__
#define __APP_TOOLS_H__
//#include "app_config.h"

typedef void (*SYSTEM_SHELL_PROC)(void* userdata);

extern int system_shell(const char* s_cmd, int time_out, SYSTEM_SHELL_PROC step_proc, SYSTEM_SHELL_PROC finsh_proc, void* userdata);
extern char * fgets_own(char * s,int* size,FILE * stream);

extern int system_shell_clean(void);


#endif
