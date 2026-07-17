/*
 * =====================================================================================
 *
 *       Filename:  app_tools.c
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
#include "app_config.h"
#include "gui_timer.h"
#include <gxos/gxcore_os.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include "app_tools.h"
#include "module/app_log.h"

#ifdef LINUX_OS
struct shell_call_param
{
    pid_t pid;
    event_list* p_Timer;
    SYSTEM_SHELL_PROC step_proc;
    SYSTEM_SHELL_PROC finsh_proc;
    void* userdata;
    int time_out;
};

static struct shell_call_param* s_plast_call_param = NULL;

int g_system_shell_ret_status = 0;
static int system_shell_timeout(void *userdata)
{
    struct shell_call_param* p_call_param = NULL;

    p_call_param = (struct shell_call_param*)userdata;

    p_call_param->time_out--;
    if(p_call_param->time_out > 0)
    {
        if( waitpid(p_call_param->pid, &g_system_shell_ret_status, WNOHANG) == 0)
        {
            if(p_call_param->step_proc)
                p_call_param->step_proc(p_call_param->userdata);
        }
        else
        {
            if(WIFSIGNALED(g_system_shell_ret_status) == true)
            {
                if(WTERMSIG(g_system_shell_ret_status) == SIGTERM)
                {
                    g_system_shell_ret_status = 0;
                }
            }
            s_plast_call_param = NULL;
            if(p_call_param->finsh_proc)
                p_call_param->finsh_proc(p_call_param->userdata);

            remove_timer(p_call_param->p_Timer);
            GxCore_Free(p_call_param);
        }
    }
    else
    {
        s_plast_call_param = NULL;
        kill(p_call_param->pid, SIGKILL);

        if(p_call_param->finsh_proc)
            p_call_param->finsh_proc(p_call_param->userdata);

        remove_timer(p_call_param->p_Timer);
        GxCore_Free(p_call_param);
    }


    return 0;
}

int system_shell_clean(void)
{
    if(NULL != s_plast_call_param)
    {
        remove_timer(s_plast_call_param->p_Timer);
        kill(s_plast_call_param->pid, SIGKILL);
        GxCore_Free(s_plast_call_param);
        s_plast_call_param = NULL;
    }
    return 0;
}

int system_shell(const char* s_cmd, int time_out, SYSTEM_SHELL_PROC step_proc, SYSTEM_SHELL_PROC finsh_proc, void* userdata)
{
    pid_t pid;
    int status = 0;
    struct shell_call_param* p_call_param = NULL;

    app_log_debug("###system_shell: %s\n", s_cmd);

    if(s_cmd == NULL){
        return (1);
    }

    if(NULL != s_plast_call_param)
    {
        remove_timer(s_plast_call_param->p_Timer);
        kill(s_plast_call_param->pid, SIGKILL);
        GxCore_Free(s_plast_call_param);
        s_plast_call_param = NULL;
    }

    if((pid = fork())<0){
        status = -1;
    }
    else if(pid == 0){
        //execl("/usr/bin/curl", "curl", "-m", "60", "-o", s_out, s_url, (char *)0);
        execl("/bin/sh", "sh", "-c", s_cmd, (char *)0);
        exit(127); //×Ó½ø³ÌÕý³£Ö´ÐÐÔò²»»áÖ´ÐÐ´ËÓï¾ä
    }
    else
    {
        if(0 == time_out)
        {
            if(NULL == step_proc)
            {
                while(waitpid(pid, &status, 0) < 0){
                    if(errno != EINTR){
                        status = -1;
                        break;
                    }
                }
            }
            else
            {
                while(waitpid(pid, &status, WNOHANG) == 0){
                    step_proc(userdata);
                    usleep(50000);
                }

                if(NULL != finsh_proc)
                    finsh_proc(userdata);
            }
        }
        else
        {
            p_call_param = GxCore_Malloc(sizeof(struct shell_call_param));
            if(NULL == p_call_param)
            {
                s_plast_call_param = NULL;

                app_log_error("!!!!error, system_shell malloc p_call_param failed!");
                while(waitpid(pid, &status, 0) < 0){
                    if(errno != EINTR){
                        status = -1;
                        break;
                    }
                }
                if(NULL != finsh_proc)
                    finsh_proc(userdata);
                return status;
            }

            p_call_param->pid = pid;
            p_call_param->time_out = time_out/100;
            p_call_param->step_proc = step_proc;
            p_call_param->finsh_proc = finsh_proc;
            p_call_param->userdata = userdata;

            s_plast_call_param = p_call_param;

            p_call_param->p_Timer = create_timer(system_shell_timeout, 100, (void *)p_call_param, TIMER_REPEAT);
        }
    }
    return status;
}
#endif

char * fgets_own(char * s,int* size,FILE * stream)
{
    char* pret = NULL, *p=NULL;
    pret = fgets(s, *size, stream);
    if(NULL == pret)
    {
        s[0] = '\0';
        *size = 0;
        return NULL;
    }
    else
    {
        *size = strlen(s);
        p = s;
        p += *size;
        p --;
        while ( (*p=='\n' || *p=='\r') && p>=s)
        {
            *p = '\0';
            p--;
            *size -= 1;
        }
    }

    return pret;
}

