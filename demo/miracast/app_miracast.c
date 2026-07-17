/*
 * =====================================================================================
 *
 *       Filename:  app_miracast.c
 *
 *    Description:  Miracast
 *
 *        Version:  1.0
 *        Created:  2014年07月10日 15时07分11秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *   Organization:
 *
 * =====================================================================================
 */

#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#define DHCPD_LEASE_PATH "/var/dhcpd.leases"

extern status_t GxPlayer_MediaVodPlay(const char* name, const char* srcurl, int64_t start, void* cb);
extern status_t GxPlayer_MediaVodStop(const char* name);

static event_list * sp_TimerMiracastInit = NULL;
static event_list * sp_TimerMiracastCheck = NULL;

static time_t prev_ts = 0;
static char IP[20] = {0};

static inline void _remove_timer(event_list ** timer)
{
    if(NULL != *timer)
    {
		remove_timer(*timer);
		*timer = NULL;
    }
}

static inline time_t _get_lease_file_timestamp(void)
{
    struct stat buf = {0};

    memset(&buf, 0, sizeof(struct stat));
    if( stat( "/var/dhcpd.leases", &buf ) != 0 )
    {
        app_log_min("[Miracast]No such file or directory : /var/dhcpd.leases ! \n");
        return 0;
    }

    return buf.st_mtime;
}

static inline void _shell(char *cmd)
{
    char buf[256];
	memset(buf,0,sizeof(buf));
	snprintf(buf, sizeof(buf), "%s", cmd);
    app_log_min("shell cmd : <<%s>>\n", buf);
	system(buf);
}

static int _miracast_check(void* data)
{
    FILE *pp;
    time_t curr_ts = 0;
    char buf[512] = {0};

    curr_ts = _get_lease_file_timestamp();

    if(prev_ts != curr_ts)
    {
        memset(buf, 0, sizeof(buf));
#ifdef USE_DHCPD
        //dhcpd
        if((pp = popen("grep \"lease 192.168.2\" /var/dhcpd.leases | cut -d \" \" -f 2", "r")) == NULL)
#else
        //udhcpd as default DHCP Server
        if((pp = popen("dumpleases -f /var/dhcpd.leases | grep \"192.168.2\" | cut -d \" \" -f 2", "r")) == NULL)
#endif
        {
            app_log_min("popen() error!\n");
            exit(1);
            return 0;
        }

        while(fgets(IP, sizeof(IP), pp)) {

            if (IP[strlen(IP) - 1] == '\n') {
                IP[strlen(IP) - 1] = '\0'; //去除换行符
            }

            memset(buf, 0, sizeof(buf));
            snprintf(buf, sizeof(buf), "wfd://%s:7236", IP);
            app_log_min("\033[32m[Miracast] IP: %s\n\033[0m", buf);

            if(GXCORE_SUCCESS == GxPlayer_MediaVodPlay(PLAYER_FOR_IPTV, buf, 0, NULL)) {
                prev_ts = curr_ts;
                _remove_timer(&sp_TimerMiracastCheck);
                break;
            }

            memset(IP, 0, sizeof(IP));
        }
        pclose(pp);
    }
    else
    {
        app_log_min("lease table has not been updated, wait for a second...\n");
    }

    return 0;
}

static int _miracast_init(void* data)
{
    _shell("./miracast.sh");

	if (reset_timer(sp_TimerMiracastCheck) != 0)
	{
		sp_TimerMiracastCheck = create_timer(_miracast_check, 1000, NULL, TIMER_REPEAT);
	}

    return 0;
}

SIGNAL_HANDLER int miracast_init(GuiWidget *widget, void *usrdata)
{
	if (reset_timer(sp_TimerMiracastInit) != 0)
	{
		sp_TimerMiracastInit = create_timer(_miracast_init, 1000, NULL, TIMER_ONCE);
	}
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int miracast_destroy(GuiWidget *widget, void *usrdata)
{
    _remove_timer(&sp_TimerMiracastCheck);
    _remove_timer(&sp_TimerMiracastInit);

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int miracast_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type)
    {
		case GUI_KEYDOWN:
			{
				switch(event->key.sym)
				{
					case STBK_EXIT:
                        prev_ts = 0;
                        GxPlayer_MediaVodStop(PLAYER_FOR_IPTV);
                        GUI_EndDialog("wnd_miracast");
						break;
					case STBK_VOLDOWN:
					case STBK_VOLUP:
					case STBK_LEFT:
					case STBK_RIGHT:
					case STBK_OK:
						break;
					default:
						break;
				}
			}
	}

	return ret;
}


