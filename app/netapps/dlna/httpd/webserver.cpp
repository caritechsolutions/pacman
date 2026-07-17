#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include "webserver.h"
#include "request.h"
#include "debug.h"
#include "module/app_log.h"

#define MAX_CLIENT 32

void * WebServerThread(void * param)
{
	CWebserver *webserver = (CWebserver*)param;

	DBG_TRACE("\n");
	webserver->DoLoop();
	pthread_exit((void *)NULL);
	return NULL;
}

bool CWebserver::Start()
{
	SAI servaddr;
	int optval = 1;
	pthread_attr_t       custom_attr ;
	struct sched_param schedparam;
	uint32_t             local_stack_size = 65536;
	uint32_t priority = 15;

	DBG_TRACE("\n");

	//network-setup
	ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt( ListenSocket, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int) );

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(cfg.Port);

	if ( bind(ListenSocket, (SA *) &servaddr, sizeof(servaddr)) != 0) {
		int i = 1;
		do {
            i++;
			DBG_ERR("bind to port %d failed...\n", cfg.Port);
			DBG_ERR("%d. Versuch, warte 5 Sekunden\n", i);
			sleep(5);
		} while (bind(ListenSocket, (SA *) &servaddr, sizeof(servaddr)) != 0);
	}

	if (listen(ListenSocket, MAX_CLIENT) != 0) {
			perror("listen failed...");
			return false;
	}
	pthread_attr_init(&custom_attr);
	pthread_attr_setstacksize(&custom_attr, (size_t)local_stack_size);
	schedparam.sched_priority = 31 - priority;
	pthread_attr_setschedparam( &custom_attr, &schedparam );
	pthread_create (&server_thread, &custom_attr, WebServerThread, this);
	DBG_TRACE("========WebServer Start========\n");

	return true;
}

void * WebThread(void * args)
{
	CWebserverRequest *req = (CWebserverRequest*) args;
	pthread_detach(pthread_self());
	if(req->GetRawRequest()) {
		if(req->ParseRequest()) {
			req->SendResponse();
			req->PrintRequest();
			req->EndRequest();
		}
	}
	delete req;
	pthread_exit((void *)NULL);
	return NULL;
}

#if 0
void sig_chld(int signo)
{
	pid_t pid;
	int stat;

	while((pid = waitpid(-1, &stat, WNOHANG)) > 0){
	}
}
#endif

void CWebserver::DoLoop()
{
	socklen_t clilen;
	SAI cliaddr;
	CWebserverRequest *req;
	int sock_connect;
//	int t = 1;
	fd_set rfds;
	struct timeval tv;
	struct sched_param schedparam;
	uint32_t             local_stack_size = 65536;
	uint32_t priority = 15;

	DBG_TRACE("\n");

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, (size_t)local_stack_size);
	schedparam.sched_priority = 31 - priority;
	pthread_attr_setschedparam( &attr, &schedparam );
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
//#if  !LINUX_VERSION_3_0_8 //cause sysem return value -1 or 0
//	signal(SIGCHLD,sig_chld);
//#endif
	clilen = sizeof(cliaddr);
	while(!STOP) {
		FD_ZERO(&rfds);
		FD_SET(ListenSocket, &rfds);

		tv.tv_sec = 2;
		tv.tv_usec = 0;

		if ( select(ListenSocket + 1, &rfds, NULL, NULL, &tv) <= 0) {
//			DBG_TRACE("no socket,listenning......\n");
			continue;
		}

		if (!FD_ISSET(ListenSocket, &rfds))
			continue;

		memset(&cliaddr, 0, sizeof(cliaddr));
		if ((sock_connect = accept(ListenSocket, (SA *) &cliaddr, &clilen)) == -1)	// accepting requests
			continue;
//		setsockopt(sock_connect, SOL_TCP, TCP_CORK, &t, sizeof(t));
		DBG_TRACE("Got connection from %s, ready to get request\n", inet_ntoa(cliaddr.sin_addr));

		req = new CWebserverRequest(this);	// create new request
		req->Socket = sock_connect;
		req->Client_Addr = inet_ntoa(cliaddr.sin_addr);

		{
			int nrecv=64*1024; //64K
			setsockopt(req->Socket,SOL_SOCKET,SO_RCVBUF,(const char*)&nrecv,sizeof(int));
		}

		pthread_create (&req->thread_id, &attr, WebThread, (void *)req);
	}
}

void CWebserver::Stop()
{
	if (ListenSocket != 0) {
		STOP = true;
		close( ListenSocket );
		ListenSocket = 0;
	}
	if (server_thread != 0){
		pthread_join(server_thread,NULL);
		server_thread = 0;
	}
}

CWebserver::CWebserver(void)
{
	CDEBUG::getInstance()->Debug = false;

	DBG_TRACE("\n");
	server_thread = 0;
	ListenSocket = 0;
	STOP = false;
}

CWebserver::~CWebserver()
{
	Stop();
}


extern "C" {

static handle_t s_webserver_run_mutex=0;
static bool s_webserver_run_status = false;

bool WebServer_RunStatus(void)
{
	return s_webserver_run_status ;
}

void WebServer_Protect(void)
{
	if ( 0 ==  s_webserver_run_mutex )
	{
		if(GXCORE_SUCCESS != GxCore_MutexCreate(&s_webserver_run_mutex))
		{
			printf("mutex protect failed");
		}
	}
	GxCore_MutexLock(s_webserver_run_mutex);
}

void WebServer_UnProtect(void)
{
	if ( 0 ==  s_webserver_run_mutex )
	{
		printf("mutex unprotect failed");
	}
	GxCore_MutexUnlock(s_webserver_run_mutex);
}

CWebserver *web;
void Http_web_server_start(void)
{
	web = new CWebserver();
	s_webserver_run_status = true ;
    web->Start();
}

void Http_web_server_stop(void)
{
	WebServer_Protect();
	s_webserver_run_status = false ;
	WebServer_UnProtect();
	delete web;
}

}
