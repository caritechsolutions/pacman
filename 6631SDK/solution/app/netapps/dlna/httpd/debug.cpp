#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include "debug.h"
#include "chttpdconfig.h"

CDEBUG *CDEBUG::instance = NULL;

CDEBUG *CDEBUG::getInstance(void)
{
	if (!instance)
		instance = new CDEBUG();

	return instance;
}

CDEBUG::CDEBUG(void)
{
	Debug = false;
	buffer = new char[1024 * 4];
	pthread_mutex_init(&Log_mutex, NULL);
}

CDEBUG::~CDEBUG(void)
{
	delete[] buffer;
}

void CDEBUG::LogRequest(CWebserverRequest *Request)
{
	if (Debug)
	{
		pthread_mutex_lock(&Log_mutex);
		std::string method;

		switch (Request->Method) {
		case M_GET:
			method = "GET";
			break;
		case M_POST:
			method = "POST";
			break;
		case M_HEAD:
			method = "HEAD";
			break;
		default:
			method = "unknown";
			break;
		}

		struct tm *time_now;
		time_t now = time(NULL);
		char zeit[80];

		time_now = localtime(&now);
		strftime(zeit, 80, "[%d/%b/%Y:%H:%M:%S]", time_now);

		::sprintf(buffer,"%s %s %s %d %s %s\n",
			Request->Client_Addr.c_str(),
			zeit,
			method.c_str(),
			Request->HttpStatus,
			Request->URL.c_str(),
			//Request->ContentType.c_str(),
			Request->Param_String.c_str());

		::printf("%s",buffer);

		pthread_mutex_unlock(&Log_mutex);
	}
}

void CDEBUG::debugprintf ( const char *fmt, ... )
{
	if (Debug)
	{
		pthread_mutex_lock( &Log_mutex );

		va_list arglist;
		va_start( arglist, fmt );
		vsprintf( buffer, fmt, arglist );
		va_end(arglist);

		::puts(buffer);

		pthread_mutex_unlock( &Log_mutex );
	}
}

void CDEBUG::logprintf ( const char *fmt, ... )
{
	if (Debug)
	{
		pthread_mutex_lock( &Log_mutex );

		va_list arglist;
		va_start( arglist, fmt );
		vsprintf( buffer, fmt, arglist );
		va_end(arglist);

		::puts(buffer);

		pthread_mutex_unlock( &Log_mutex );
	}
}

void CDEBUG::printf ( const char *fmt, ... )
{
	pthread_mutex_lock( &Log_mutex );

	va_list arglist;
	va_start( arglist, fmt );
	vsprintf( buffer, fmt, arglist );
	va_end(arglist);

	::puts(buffer);

	pthread_mutex_unlock( &Log_mutex );
}

