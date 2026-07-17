#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include "app_module.h"
#include "module/app_frontend.h"
#include "app_wnd_search.h"
#include <gx_apps.h>
#include <fcntl.h>
#if SAT_FINDER_SUPPORT
#include "dvbfinder.h"
#ifdef LINUX_OS
#include <poll.h>
#else
#include <sys/select.h>
#endif
#ifndef POLLRDNORM
#define POLLRDNORM POLLIN
#endif
#define HTTP_USER_AGENT     "Mozilla/5.0 (Windows NT 5.1; rv:21.0) Gecko/20100101 Firefox/21.0"

#define DVBFINDER_DEBUG  (APP_LOG_LEVEL >= APP_LOG_DEBUG)

static unsigned int dream_sat_id = -1;
static unsigned dream_tp_id = -1;
static struct pollfd s_pfd[3];

static int dvbfinder_local_tcp_init(int port)
{
    int ret = 0;
	int val;
	struct sockaddr_in servaddr;
	struct timeval tv;

	int fd = socket(AF_INET,SOCK_STREAM,0);
	if(fd==-1)
	{
		app_log_error("%s() line %d\n",__FUNCTION__,__LINE__);
		return -1;
	}
	app_log_debug("%s line %d fd %d\n",__FUNCTION__,__LINE__,fd);
	val = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, val | O_NONBLOCK);
	val = 1;
	ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int));
	if(ret == -1)
	{
		app_log_error("error setsockopt: SO_REUSEADDR\n");
        close(fd);
		return -1;
	}

	tv.tv_sec = 2;
	tv.tv_usec = 0;
	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval))) {
		app_log_error("error setsockopt: SO_RCVTIMEO");
	}
	if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(struct timeval))) {
		app_log_error("error setsockopt: SO_SNDTIMEO");
	}

	memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
	ret = bind(fd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if (ret == -1)
	{
		app_log_error("bind socket error: %s(errno: %d)\n", strerror(errno), errno);
        close(fd);
		return -1;
	}
	ret = listen(fd,5);
	if (ret == -1)
	{
		app_log_error("listen socket error: %s(errno: %d)\n", strerror(errno), errno);
        close(fd);
		return -1;
	}
    return fd;
}

static int dvbfinder_server_tcp_init(char *s,int p)
{
    int fd = -1;
	int error = 0;
	int val;
    struct hostent *h;
    struct hostent server;
    struct sockaddr_in srv_addr;

    memset(&srv_addr,0,sizeof(struct sockaddr_in));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(p);

    if ((fd=socket(AF_INET, SOCK_STREAM,IPPROTO_TCP))<0)
    {
        app_log_error("%s() line %d\n",__FUNCTION__,__LINE__);
        return -1;
    }

    memset(&server, 0, sizeof(struct hostent));
    h = gethostbyname(s);
    if(NULL != h)
    {
        memcpy(&server,h,sizeof(struct hostent));
    }
    if((4 != server.h_length)||(NULL == server.h_addr))
    {
        close(fd);
        app_log_error("%s() line %d\n",__FUNCTION__,__LINE__);
        return -1;
    }
	memmove((char *)&srv_addr.sin_addr.s_addr, (char *)server.h_addr, server.h_length);

	error = connect(fd, (struct sockaddr *)&srv_addr, sizeof(srv_addr));
	if(error < 0) {
		app_log_error("%s connect(fd=%d) failed\n",__FUNCTION__,fd);
		close(fd);
		return -1;
	}
	val = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, val & ~O_NONBLOCK);
	unsigned int tm = 5000;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tm, sizeof(int));
	app_log_debug("%s fd = %d OK\n",__FUNCTION__,fd);

	unsigned int l = 1; //set socket keepalive
	setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,(void *)&l, sizeof(unsigned int));
	setsockopt(fd,IPPROTO_TCP,/*TCP_NODELAY*/0x01,(char*)&l,sizeof(int));

    return fd;
}

void dvbfinder_conn_send(char *buf,int len,int fd)
{
    send(fd,buf,len,0);
}

int dvbfinder_conn_recv(char *buf,int len,int *curr_fd)
{
    int i = 0;
	int ret = 0;

	if(poll(s_pfd,3,500) <=0)
	{
		return 0;
	}
	for (i = 0; i < 3; i++)
	{
		if (s_pfd[i].revents != 0)
		{
			if (i == 0)
			{
				struct sockaddr_in addr;
				socklen_t addrlen = sizeof(struct sockaddr_in);
				int fd = accept(s_pfd[0].fd, (struct sockaddr *) &addr, &addrlen);
			    app_log_debug("%s %d accept fd %d\n",__FUNCTION__,__LINE__,fd);
				if (fd > 0)
				{
					if (s_pfd[2].fd != -1)
					{
						close(s_pfd[2].fd);
					}
	                struct timeval tv;
	                tv.tv_sec = 1;
	                tv.tv_usec = 0;
	                if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval)))
					{
	                    app_log_error("setsockopt: SO_RCVTIMEO error\n");
	                }
	                if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(struct timeval)))
					{
	                    app_log_error("setsockopt: SO_SNDTIMEO error\n");
	                }
	                int optval = 1;
	                if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) == -1)
					{
	                    app_log_error("setsockopt: SO_KEEPALIVE error\n");
	                }
					s_pfd[2].fd = fd;
					s_pfd[2].events = POLLIN | POLLHUP | POLLRDNORM | POLLERR;
					s_pfd[2].revents = 0;
				}
			}
			else
			{
				ret = recv(s_pfd[i].fd,buf,len, 0);
				*curr_fd = -1;
				if(ret==0)
				{
					if(i==1)
					{
	                    close(s_pfd[1].fd);
						s_pfd[1].fd = -1;
						app_log_info("reconnect server\n");
						s_pfd[1].fd = dream_dvbfinder_reconnect();
					}
					else if(i==2)
					{
						close(s_pfd[2].fd);
						s_pfd[2].fd = -1;
					}
					return 0;
				}
				*curr_fd = s_pfd[i].fd;
				//app_log_debug("conn %d recv (%d)\n",i,ret);
			    return ret;
			}
		}
	}
	return 0;
}

int dvbfinder_conn_init(char *s,int p,int lp)
{
    int i=0;
    if(lp!=0)
    {
        for (i = 0; i < 3; i++)
        {
            s_pfd[i].fd = -1;
            s_pfd[i].events = POLLIN | POLLHUP | POLLRDNORM | POLLERR;
            s_pfd[i].revents = 0;
        }
        s_pfd[0].fd = dvbfinder_local_tcp_init(lp);
    }
    s_pfd[1].fd = dvbfinder_server_tcp_init(s,p);

    return s_pfd[1].fd;
}

int dvbfinder_conn_close(void)
{
    int i = 0;
    for (i = 0; i < 3; i++)
	{
		if(s_pfd[i].fd != -1)
		{
			close(s_pfd[i].fd);
            s_pfd[i].fd = -1;
		}
	}
    return 0;
}

int dvbfinder_get_local_ip(char *ip)
{
    GxMsgProperty_NetDevIpcfg s_ip_cfg;

    memset(&s_ip_cfg, 0, sizeof(GxMsgProperty_NetDevIpcfg));
    app_send_msg_exec(GXMSG_NETWORK_GET_CUR_DEV, &s_ip_cfg.dev_name);
    if(s_ip_cfg.dev_name[0] == 0)
        return -1;
    app_send_msg_exec(GXMSG_NETWORK_GET_IPCFG, &s_ip_cfg);
    strcpy(ip, s_ip_cfg.ip_addr);
    return 0;
}

int db_sat_init(void)
{
    GxMsgProperty_NodeAdd node = {0};
    node.node_type = NODE_SAT;
	node.sat_data.type = GXBUS_PM_SAT_S;
	node.sat_data.tuner = 0;
	node.sat_data.sat_s.lnb1 = 5150;
	node.sat_data.sat_s.lnb2 = 5750;
	node.sat_data.sat_s.lnb_power = GXBUS_PM_SAT_LNB_POWER_ON;
	node.sat_data.sat_s.switch_22K = GXBUS_PM_SAT_22K_OFF;
    node.sat_data.sat_s.diseqc11 = 0;
	node.sat_data.sat_s.diseqc12_pos = DISEQ12_USELESS_ID;
	node.sat_data.sat_s.diseqc_version = GXBUS_PM_SAT_DISEQC_OFF;
	node.sat_data.sat_s.diseqc10 = 0;
	node.sat_data.sat_s.switch_12V = GXBUS_PM_SAT_12V_OFF;
	node.sat_data.sat_s.longitude_direct = GXBUS_PM_SAT_LONGITUDE_DIRECT_EAST;
	node.sat_data.sat_s.reserved = 0;
	node.sat_data.sat_s.longitude = 0;
	node.sat_data.sat_s.unicable_para.lnb_fre_index = 32;
	strncpy((char *)node.sat_data.sat_s.sat_name,"FinderSat", MAX_SAT_NAME);

    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_ADD, &node))
	{
	    dream_sat_id = node.sat_data.id;
	    app_log_debug("sat id %d\n",node.sat_data.id);
		memset(&node, 0, sizeof(GxMsgProperty_NodeAdd));
		node.node_type = NODE_TP;
		node.tp_data.sat_id = dream_sat_id;
		node.tp_data.frequency = 3880;
		node.tp_data.tp_s.symbol_rate = 27500;
		node.tp_data.tp_s.polar = GXBUS_PM_TP_POLAR_H;

		if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_ADD, &node))
		{
		    app_log_debug("tp id %d\n",node.tp_data.id);
            dream_tp_id = node.tp_data.id;
            return 0;
		}

	}
    return -1;
}

int db_sat_release(void)
{
    GxMsgProperty_NodeDelete node= {0};
#if NDEMOD_WITH_1TUNER
    app_only_cur_frontend_monitor_start(app_tuner_cur_tuner_get());
#else
    app_all_frontend_monitor_control(FRONTEND_MONITOR_ON, false);
#endif
    node.node_type = NODE_SAT;
    node.num = 1;

    node.id_array = &dream_sat_id;
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_DELETE, &node))
    {
        app_log_info("del sat success\n");
        return 0;
    }
    return -1;
}
#define SATFINDER_SEARCH_PROG_EXIST 0x7ffffffd
#define MAX_CHANNEL_CNT 64
static GxMsgProperty_ScanSatStart tpSearch;
static dvbfinder_channel_t dvbfinder_channel_list[MAX_CHANNEL_CNT];
static int dvbfinder_channel_cnt = 0;
static int dvbfinder_ts_src = 0;
status_t find_new_prog(GxBusPmDataProg* prog)
{
    app_log_debug("find %s %s\n",prog->service_type==0?"tv":"radio",prog->prog_name);
    if(dvbfinder_channel_cnt<MAX_CHANNEL_CNT)
    {
        dvbfinder_channel_list[dvbfinder_channel_cnt].type = prog->service_type;
        memcpy(dvbfinder_channel_list[dvbfinder_channel_cnt].name,prog->prog_name,MAX_PROG_NAME);
        dvbfinder_channel_cnt++;
    }
    return GXCORE_SUCCESS;
}

int dvbfinder_search_tp()
{
    memset(&tpSearch,0,sizeof(GxMsgProperty_ScanSatStart));
    tpSearch.scan_type = GX_SEARCH_MANUAL;
    tpSearch.tv_radio = GX_SEARCH_TV_RADIO_ALL;
    tpSearch.fta_cas = GX_SEARCH_FTA_CAS_ALL;
    tpSearch.nit_switch = GX_SEARCH_NIT_DISABLE;

    tpSearch.params_s.array = &dream_tp_id;
    tpSearch.params_s.ts = (uint32_t *)&dvbfinder_ts_src;
    tpSearch.params_s.max_num = 1;

    tpSearch.time_out.pat = TIMEOUT_PAT;
    tpSearch.time_out.pmt = TIMEOUT_PMT;
    tpSearch.time_out.sdt = TIMEOUT_SDT;
    tpSearch.time_out.nit = TIMEOUT_NIT;

    tpSearch.modify_prog = find_new_prog;
    app_all_frontend_monitor_control(FRONTEND_MONITOR_OFF, false);
    dvbfinder_channel_cnt = 0;
    app_send_msg_exec(GXMSG_SEARCH_SCAN_SAT_START, &tpSearch);

    return 0;
}

int dvbfinder_get_channels(dvbfinder_channel_t *ch,int max)
{
    int cnt = dvbfinder_channel_cnt>max ? max : dvbfinder_channel_cnt;
    memcpy(ch,dvbfinder_channel_list,cnt*sizeof(dvbfinder_channel_t));

    return cnt;
}

void dvbfinder_set_tuner(char pol,char k22,char port,unsigned short frq,unsigned short sym)
{
    AppFrontend_SetTp params;
    AppFrontend_PolarState Polar;
    AppFrontend_22KState _22K;
    AppFrontend_DiseqcParameters diseqc;
    memset(&params,0,sizeof(AppFrontend_SetTp));

    app_log_debug("%s pol %d k22 %d\n",__FUNCTION__,pol,k22);

    Polar = params.polar = pol == 0 ? SEC_VOLTAGE_18 : SEC_VOLTAGE_13;
    _22K = params.sat22k = k22==0 ? SEC_TONE_ON : SEC_TONE_OFF;

    app_ioctl(app_tuner_cur_tuner_get(), FRONTEND_POLAR_SET, (void *)&Polar);

    GxCore_ThreadDelay(50);
    app_ioctl(app_tuner_cur_tuner_get(), FRONTEND_22K_SET, (void *)&_22K);

    GxCore_ThreadDelay(50);

    params.type = FRONTEND_DVB_S;
    params.fre = frq;
    params.symb = sym;

    memset(&diseqc,0,sizeof(AppFrontend_DiseqcParameters));
    diseqc.type = DiSEQC10;
    diseqc.u.params_10.b22k = _22K;
    diseqc.u.params_10.bPolar = Polar;
    diseqc.u.params_10.chDiseqc = port;
    app_set_diseqc_10(app_tuner_cur_tuner_get(),&diseqc, true);
    GxFrontendOccur(app_tuner_cur_tuner_get());
    GxCore_ThreadDelay(50);
    app_set_tp(app_tuner_cur_tuner_get(), &params, SET_TP_FORCE);

}


int dvbfinder_get_lock_signal(char *l,char *s,char *q)
{
    int str_value = 0;
    int qua_value = 0;
    AppFrontend_LockState lock;
    app_ioctl(app_tuner_cur_tuner_get(), FRONTEND_STRENGTH_GET, &str_value);
	app_ioctl(app_tuner_cur_tuner_get(), FRONTEND_QUALITY_GET, &qua_value);
    app_ioctl(app_tuner_cur_tuner_get(), FRONTEND_LOCK_STATE_GET, &lock);
    *s = str_value;
    *q = qua_value;
    *l = lock;
    return 0;
}
int dvbfinder_set_tp(unsigned short frq,unsigned short sym,char pol)
{
    GxMsgProperty_NodeByIdGet sat = {0};
    GxMsgProperty_NodeByIdGet node = {0};
    GxMsgProperty_NodeModify node_new = {0};

    node.node_type = NODE_TP;
    node.id = dream_tp_id;
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node))
    {
        app_log_debug("tp freq %d sym %d pol %d sat id %d\n",node.tp_data.frequency,
            node.tp_data.tp_s.symbol_rate,node.tp_data.tp_s.polar,node.tp_data.sat_id);
        node_new.node_type = NODE_TP;
        memcpy(&node_new.tp_data,&node.tp_data,sizeof(GxBusPmDataTP));
        node_new.tp_data.frequency = frq;
        node_new.tp_data.tp_s.symbol_rate = sym;
        node_new.tp_data.tp_s.polar = pol;
        if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_new))
        {
            sat.node_type = NODE_SAT;
            sat.id = dream_sat_id;
            if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat))
            {
                dvbfinder_set_tuner(node_new.tp_data.tp_s.polar,
                    app_show_to_sat_22k(sat.sat_data.sat_s.switch_22K,&node_new.tp_data),
                    sat.sat_data.sat_s.diseqc10,
                    app_show_to_tp_freq(&sat.sat_data,&node_new.tp_data),
                    node_new.tp_data.tp_s.symbol_rate);
                return 0;
            }
        }
    }

    return -1;
}
int dvbfinder_get_tp(unsigned short *frq,unsigned short *sym,char *pol)
{
    GxMsgProperty_NodeByIdGet node = {0};
    node.node_type = NODE_TP;
    node.id = dream_tp_id;
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node))
    {
        app_log_debug("tp freq %d sym %d pol %d sat id %d\n",node.tp_data.frequency,
            node.tp_data.tp_s.symbol_rate,node.tp_data.tp_s.polar,node.tp_data.sat_id);

        *frq = node.tp_data.frequency>>8;
        *sym = node.tp_data.tp_s.symbol_rate>>8;
        *pol = node.tp_data.tp_s.polar;
        return 0;
    }
    return -1;
}
int dvbfinder_set_antenna(unsigned short lnb1,unsigned short lnb2,char k22,char disqec)
{
    GxMsgProperty_NodeByIdGet node = {0};
    GxMsgProperty_NodeModify node_new = {0};
    GxMsgProperty_NodeByIdGet tp_node = {0};
    node.node_type = NODE_SAT;
    node.id = dream_sat_id;
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node))
    {
        app_log_debug("sat name %s\n",node.sat_data.sat_s.sat_name);
        node_new.node_type = NODE_SAT;
        memcpy(&node_new.sat_data,&node.sat_data,sizeof(GxBusPmDataSat));
        node_new.sat_data.sat_s.lnb1 = lnb1;
        node_new.sat_data.sat_s.lnb2 = lnb2;
        node_new.sat_data.sat_s.switch_22K = k22;
        node_new.sat_data.sat_s.diseqc10 = disqec;
        app_log_debug("lnb1 %d lnb2 %d 22k %d disqec %d\n",node_new.sat_data.sat_s.lnb1,
            node_new.sat_data.sat_s.lnb2,node_new.sat_data.sat_s.switch_22K,
            node_new.sat_data.sat_s.diseqc10);
        if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_new))
        {
            tp_node.node_type = NODE_TP;
            tp_node.id = dream_tp_id;
            if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &tp_node))
            {
                app_log_debug("tp freq %d sym %d pol %d sat id %d\n",tp_node.tp_data.frequency,
                    tp_node.tp_data.tp_s.symbol_rate,tp_node.tp_data.tp_s.polar,tp_node.tp_data.sat_id);
                dvbfinder_set_tuner(tp_node.tp_data.tp_s.polar,
                    app_show_to_sat_22k(node_new.sat_data.sat_s.switch_22K,&tp_node.tp_data),
                    node_new.sat_data.sat_s.diseqc10,
                    app_show_to_tp_freq(&node_new.sat_data,&tp_node.tp_data),
                    tp_node.tp_data.tp_s.symbol_rate);
                return 0;
            }
        }
    }
    return -1;
}

int dvbfinder_get_antenna(unsigned short *lnb1,unsigned short *lnb2,char *k22,char *disqec)
{
    GxMsgProperty_NodeByIdGet node = {0};
    node.node_type = NODE_SAT;
    node.id = dream_sat_id;
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node))
    {
        app_log_debug("sat name %s\n",node.sat_data.sat_s.sat_name);
        *lnb1 = node.sat_data.sat_s.lnb1;
        *lnb2 = node.sat_data.sat_s.lnb2;
        *k22 = node.sat_data.sat_s.switch_22K;
        *disqec = node.sat_data.sat_s.diseqc10;
        return 0;
    }
    return -1;
}

int dvbfinder_http_sync_download(char *url,char *buf,unsigned int *size)
{
    gx_curl_http_options_t opt = {0};
    gx_curl_data_t curldata = {0};
    int ret = 0;
    opt.user_agent = HTTP_USER_AGENT;

    ret = gxapps_curl_httpfunc(url,GX_CURL_HTTP_GET,&opt,&curldata);
    //app_log_debug("ret %d %x %d %d\n",ret,curldata.buffer,curldata.used_size,curldata.buffer_size);
	if(GXAPPS_SUCCESS == ret)
	{
	    *size = curldata.used_size > *size ? *size : curldata.used_size;
        memcpy(buf,curldata.buffer,*size);
    }
    gxapps_curl_free(&curldata);

    return ret;
}

void dvbfinder_sleep(unsigned int ms)
{
    GxCore_ThreadDelay(ms);
}

char *dvbfinder_strcpy(char *s,char *d)
{
    return strcpy(s,d);
}
char *dvbfinder_strncpy(char *s,char *d,int l)
{
    return strncpy(s,d,l);
}
int dvbfinder_strlen(char *s)
{
    return strlen(s);
}
char *dvbfinder_strchr(char *s,char c)
{
    return strchr(s,c);
}
char *dvbfinder_strrchr(char *s,char c)
{
    return strrchr(s,c);
}
void *dvbfinder_memset(void *s,int d,int l)
{
    return memset(s,d,l);
}
void *dvbfinder_memcpy(void *s,void *d,int l)
{
    return memcpy(s,d,l);
}
void *dvbfinder_malloc(int size)
{
    return GxCore_Malloc(size);
}
void dvbfinder_free(void *p)
{
    GxCore_Free(p);
}

int dvbfinder_sprintf(char *d,char *fmt,...)
{
    int ret = 0;
    va_list p_args;
    va_start(p_args,fmt);
    ret = vsprintf(d,fmt,p_args);
    va_end(p_args);
    return ret;
}

int dvbfinder_atoi(char *s)
{
    return atoi(s);
}
int dvbfinder_memcmp(void *s,void *d,int l)
{
    return memcmp(s,d,l);
}


unsigned int dvbfinder_get_tick(void)
{
    unsigned int tmp = 0;
    GxTime t;
    GxCore_GetTickTime(&t);
    tmp = t.seconds*1000+t.microsecs/1000;
    //app_log_debug("%u.%u %u\n",t.seconds,t.microsecs,tmp);
    return tmp;
}

int dvbfinder_task_create(char *name,void(*entry_func)(void *), void *arg,int stack_size)
{
    handle_t thread_id;
	GxCore_ThreadCreate(name,&thread_id,entry_func,arg,stack_size,15);
    return thread_id;
}

void dvbfinder_task_destroy(int id)
{
	GxCore_ThreadJoin(id);
}

int dvbfinder_rand(int max)
{
    return dvbfinder_get_tick()%max;
}

void dvbfinder_get_sn(char *sn,int len)
{
    app_chip_sn_get(sn,len);
}

#endif
