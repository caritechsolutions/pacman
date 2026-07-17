/*
 *  Copyright (C) 2009 Andrej Stepanchuk
 *  Copyright (C) 2009-2010 Howard Chu
 *
 *  This file is part of librtmp.
 *
 *  librtmp is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1,
 *  or (at your option) any later version.
 *
 *  librtmp is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with librtmp see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *  http://www.gnu.org/copyleft/lgpl.html
 */

#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include <ctype.h>

#include "rtmp_sys.h"
#include "log.h"
#include "gx_common.h"

static void _rtmp_parse_app(AVal* app, AVal* playpath)
{
	char* p = NULL;
	char* p_str = NULL;
	char* p_str1 = NULL;
	int len = 0;
	int len1 = 0;
	if(playpath->av_len == 0 && app->av_len > 0){
		p = strstr(app->av_val, " playpath=");
		if(p){
			len = p-app->av_val;
			p_str = av_malloc(len+1);
			memcpy(p_str, app->av_val, len);
			p_str[len] = '\0';
			len1 = strlen(app->av_val)+app->av_val-p-10;
			p_str1 = av_malloc(len1+1);
			memcpy(p_str1, p+10, len1);
			p_str1[len1] = '\0';
			av_free(app->av_val);
			app->av_val = p_str;
			app->av_len = len;
			playpath->av_val = p_str1;
			playpath->av_len = len1;
		}
	}else if(playpath->av_len > 0){
		p = strstr(playpath->av_val, " playpath=");
		if(p){
			len = strlen(playpath->av_val)+playpath->av_val-p-10;
			p_str = av_malloc(len+1);
			memcpy(p_str, p+10, len);
			p_str[len] = '\0';
			av_free(playpath->av_val);
			playpath->av_val = p_str;
			playpath->av_len = len;
		}
	}
	return ;
}

static void _rtmp_get_key_strings(const char* url, char* keystring, AVal* val)
{
	char *p = NULL;
	int pl = 0, len = 0;
	char c = 0;

	if(url == NULL || keystring == NULL)
		return;

	p = strstr(url, keystring);
	if(p == NULL)
		return;

	len = strlen(keystring);
	p += len;
	c = p[0];
	while((c != ' ')&&(c != '\0')){
		pl++;
		c = p[pl];
	}
	val->av_val = av_malloc(pl+1);
	if(val->av_val == NULL)
		return;
	memcpy(val->av_val, p, pl);
	val->av_val[pl] = '\0';
	val->av_len = pl;
	return;
}

static int _rtmp_parse_url(const char* s_url, char* url,
		AVal *pageurl, AVal *swfurl, AVal* tcurl, AVal *playpath, int* blive)
{
	int len = 0;
	char *p = NULL;
	AVal word = {0, 0};

	p = strchr(s_url, ' ');
	if(p == NULL){
		len = strlen(s_url);
		memcpy(url, s_url, len);
		url[len] = '\0';
		return TRUE;
	}
	len = p - s_url;
	memcpy(url, s_url, len);
	url[len] = '\0';

	p = strstr(s_url, "pageUrl=");
	if(p)
		_rtmp_get_key_strings(s_url, "pageUrl=", pageurl);

	p = strstr(s_url, "swfUrl=");
	if(p)
		_rtmp_get_key_strings(s_url, "swfUrl=", swfurl);

	p = strstr(s_url, "tcUrl=");
	if(p)
		_rtmp_get_key_strings(s_url, "tcUrl=", tcurl);

	p = strstr(s_url, "playPath=");
	if(p)
		_rtmp_get_key_strings(s_url, "playPath=", playpath);

	p = strstr(s_url, "live=");
	if(p){
		_rtmp_get_key_strings(s_url, "live=", &word);
		if(word.av_len){
			*blive = atoi(word.av_val);
			av_free(word.av_val);
			word.av_len = 0;
			word.av_val = NULL;
		}
	}

	return TRUE;
}

int RTMP_ParseURL(const char *s_url, int *protocol, AVal *host, unsigned int *port,
	AVal *playpath, AVal *app, AVal *tcurl, AVal *pageurl, AVal *swfurl, int* blive)
{
	char url[2048];
	char *p, *end, *col, *ques, *slash;

	*protocol = RTMP_PROTOCOL_RTMP;
	*port = 0;
	playpath->av_len = 0;
	playpath->av_val = NULL;
	app->av_len = 0;
	app->av_val = NULL;
	tcurl->av_len = 0;
	tcurl->av_val = NULL;

	/* Old School Parsing */
	if(_rtmp_parse_url(s_url, url, pageurl, swfurl, tcurl, playpath, blive)==FALSE){
		gxlogd("rtmp parse url error\n");
		return FALSE;
	}

	/* look for usual :// pattern */
	p = strstr(url, "://");
	if(!p) {
		RTMP_Log(RTMP_LOGERROR, "RTMP URL: No :// in url!");
		return FALSE;
	}
	{
	int len = (int)(p-url);

	if(len == 4 && strncasecmp(url, "rtmp", 4)==0)
		*protocol = RTMP_PROTOCOL_RTMP;
	else if(len == 5 && strncasecmp(url, "rtmpt", 5)==0)
		*protocol = RTMP_PROTOCOL_RTMPT;
	else if(len == 5 && strncasecmp(url, "rtmps", 5)==0)
	        *protocol = RTMP_PROTOCOL_RTMPS;
	else if(len == 5 && strncasecmp(url, "rtmpe", 5)==0)
	        *protocol = RTMP_PROTOCOL_RTMPE;
	else if(len == 5 && strncasecmp(url, "rtmfp", 5)==0)
	        *protocol = RTMP_PROTOCOL_RTMFP;
	else if(len == 6 && strncasecmp(url, "rtmpte", 6)==0)
	        *protocol = RTMP_PROTOCOL_RTMPTE;
	else if(len == 6 && strncasecmp(url, "rtmpts", 6)==0)
	        *protocol = RTMP_PROTOCOL_RTMPTS;
	else {
		RTMP_Log(RTMP_LOGWARNING, "Unknown protocol!\n");
		goto parsehost;
	}
	}

	RTMP_Log(RTMP_LOGDEBUG, "Parsed protocol: %d", *protocol);

parsehost:
	/* let's get the hostname */
	p+=3;

	/* check for sudden death */
	if(*p==0) {
		RTMP_Log(RTMP_LOGWARNING, "No hostname in URL!");
		return FALSE;
	}

	end   = p + strlen(p);
	col   = strchr(p, ':');
	ques  = strchr(p, '?');
	slash = strchr(p, '/');

	{
	int hostlen;
	if(slash)
		hostlen = slash - p;
	else
		hostlen = end - p;
	if(col && col -p < hostlen)
		hostlen = col - p;

	if(hostlen < 256) {
		host->av_val = (char*)av_malloc(hostlen+1);
		host->av_len = hostlen;
		memcpy(host->av_val,p,hostlen);
		host->av_val[hostlen]='\0';
		RTMP_Log(RTMP_LOGDEBUG, "Parsed host    : %.*s", hostlen, host->av_val);
	} else {
		RTMP_Log(RTMP_LOGWARNING, "Hostname exceeds 255 characters!");
	}

	p+=hostlen;
	}

	/* get the port number if available */
	if(*p == ':') {
		unsigned int p2;
		p++;
		p2 = atoi(p);
		if(p2 > 65535) {
			RTMP_Log(RTMP_LOGWARNING, "Invalid port number!");
		} else {
			*port = p2;
		}
	}

	if(!slash) {
		RTMP_Log(RTMP_LOGWARNING, "No application or playpath in URL!");
		return TRUE;
	}
	p = slash+1;

	{
	/* parse application
	 *
	 * rtmp://host[:port]/app[/appinstance][/...]
	 * application = app[/appinstance]
	 */

	char *slash2, *slash3 = NULL;
	int applen, appnamelen;

	slash2 = strchr(p, '/');
	if(slash2)
		slash3 = strchr(slash2+1, '/');

	applen = end-p; /* ondemand, pass all parameters as app */
	appnamelen = applen; /* ondemand length */

	if(ques && strstr(p, "slist=")) { /* whatever it is, the '?' and slist= means we need to use everything as app and parse plapath from slist= */
		appnamelen = ques-p;
	}
	else if(strncmp(p, "ondemand/", 9)==0) {
                /* app = ondemand/foobar, only pass app=ondemand */
                applen = 8;
                appnamelen = 8;
        }
	else { /* app!=ondemand, so app is app[/appinstance] */
		if(slash3)
			appnamelen = slash3-p;
		else if(slash2)
			appnamelen = slash2-p;

		applen = appnamelen;
	}

	app->av_val = (char*)av_malloc(applen+1);
	memcpy(app->av_val,p,applen);
	app->av_val[applen]='\0';
	app->av_len = applen;
	RTMP_Log(RTMP_LOGDEBUG, "Parsed app     : %.*s", applen, p);

	p += appnamelen;
	}

	if (*p == '/')
		p++;

	if (end-p) {
		AVal av = {p, end-p};
		RTMP_ParsePlaypath(&av, playpath);
	}
	_rtmp_parse_app(app, playpath);

	if(tcurl->av_len <= 0){
		char str[1024];
		tcurl->av_len = snprintf(str, 1023, "%s://%.*s:%d/%.*s",
				RTMPProtocolStringsLower[*protocol], host->av_len, host->av_val, (*port>0)?(*port):1935, app->av_len, app->av_val);
		tcurl->av_val = (char *) av_malloc(tcurl->av_len + 1);
		strncpy(tcurl->av_val, str, tcurl->av_len);
	}

	gxlogf("tcurl:    %s\n", tcurl->av_val);
	gxlogf("app:      %s\n", app->av_val);
	gxlogf("playpath: %s\n", playpath->av_val);
	gxlogf("swfurl:   %s\n", swfurl->av_val);
	gxlogf("pageurl:  %s\n", pageurl->av_val);
	gxlogf("url:      %s\n", url);
	return TRUE;
}

/*
 * Extracts playpath from RTMP URL. playpath is the file part of the
 * URL, i.e. the part that comes after rtmp://host:port/app/
 *
 * Returns the stream name in a format understood by FMS. The name is
 * the playpath part of the URL with formatting depending on the stream
 * type:
 *
 * mp4 streams: prepend "mp4:", remove extension
 * mp3 streams: prepend "mp3:", remove extension
 * flv streams: remove extension
 */
void RTMP_ParsePlaypath(AVal *in, AVal *out) {
	int addMP4 = 0;
	int addMP3 = 0;
	int subExt = 0;
	const char *playpath = in->av_val;
	const char *temp, *q, *ext = NULL;
	const char *ppstart = playpath;
	char *streamname, *destptr, *p;

	int pplen = in->av_len;

	out->av_val = NULL;
	out->av_len = 0;

	if ((*ppstart == '?') &&
	    (temp=strstr(ppstart, "slist=")) != 0) {
		ppstart = temp+6;
		pplen = strlen(ppstart);

		temp = strchr(ppstart, '&');
		if (temp) {
			pplen = temp-ppstart;
		}
	}

	q = strchr(ppstart, '?');
	if (pplen >= 4) {
		if (q)
			ext = q-4;
		else
			ext = &ppstart[pplen-4];
		if ((strncmp(ext, ".f4v", 4) == 0) ||
		    (strncmp(ext, ".mp4", 4) == 0)) {
			addMP4 = 1;
			subExt = 1;
		/* Only remove .flv from rtmp URL, not slist params */
		} else if ((ppstart == playpath) &&
		    (strncmp(ext, ".flv", 4) == 0)) {
			subExt = 1;
		} else if (strncmp(ext, ".mp3", 4) == 0) {
			addMP3 = 1;
			subExt = 1;
		}
	}

	streamname = (char *)av_malloc((pplen+4+1)*sizeof(char));
	if (!streamname)
		return;

	destptr = streamname;
	if (addMP4) {
		if (strncmp(ppstart, "mp4:", 4)) {
			strncpy(destptr, "mp4:", (pplen+4)*sizeof(char));
			destptr += 4;
		} else {
			subExt = 0;
		}
	} else if (addMP3) {
		if (strncmp(ppstart, "mp3:", 4)) {
			strncpy(destptr, "mp3:", (pplen+4)*sizeof(char));
			destptr += 4;
		} else {
			subExt = 0;
		}
	}

 	for (p=(char *)ppstart; pplen >0;) {
		/* skip extension */
		if (subExt && p == ext) {
			p += 4;
			pplen -= 4;
			continue;
		}
		if (*p == '%') {
			unsigned int c;
			sscanf(p+1, "%02x", &c);
			*destptr++ = c;
			pplen -= 3;
			p += 3;
		} else {
			*destptr++ = *p++;
			pplen--;
		}
	}
	*destptr = '\0';

	out->av_val = streamname;
	out->av_len = destptr - streamname;
	return;
}

static void free_property(AVal* property)
{
	if(property && property->av_len && property->av_val){
		av_free(property->av_val);
		property->av_len = 0;
	}
	return;
}

void RTMP_FreeURL(AVal *host, AVal *playpath, AVal *app, AVal *tcurl,
		AVal *pageurl, AVal *swfurl)
{
	if(app)
		free_property(app);
	if(host)
		free_property(host);
	if(playpath)
		free_property(playpath);
	if(tcurl)
		free_property(tcurl);
	if(pageurl)
		free_property(pageurl);
	if(swfurl)
		free_property(swfurl);
	return;
}
