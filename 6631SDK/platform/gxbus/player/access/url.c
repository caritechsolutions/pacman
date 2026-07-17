#include "gx_common.h"
#include "gx_url.h"
#include "gx_system.h"
#include "../avutil/avutil.h"

#define SPACE_CHARS " \t\r\n"

#ifndef GX_SIZE_MAX
#define GX_SIZE_MAX ((size_t)-1)
#endif

static int is_proxy(const GxURL *url) {
	return !strcasecmp(url->protocol, "http_proxy") && url->file && strstr(url->file, "://");
}

int url_is_protocol(const GxURL *url, const char *proto) {
	int proxy = is_proxy(url);
	if (proxy) {
		GxURL *tmp = url_new(url->file + 1);
		int res = !strcasecmp(tmp->protocol, proto);
		url_free(tmp);
		return res;
	}
	return !strcasecmp(url->protocol, proto);
}

void url_set_protocol(GxURL *url, const char *proto) {
	int proxy = is_proxy(url);
	if (proxy) {
		char *dst = url->file + 1;
		int oldlen = strstr(dst, "://") - dst;
		int newlen = strlen(proto);
		if (newlen != oldlen) {
			gx_msg(MSGT_NETWORK, MSGL_ERR, "Setting protocol not implemented!\n");
			return;
		}
		memcpy(dst, proto, newlen);
		return;
	}
	av_free(url->protocol);
	url->protocol = av_strdup(proto);
}

#ifndef CONFIG_FFMPEG_ENABLE
char *av_asprintf(const char *fmt, ...)
{
    char *p = NULL;
    va_list va;
    int len;

    va_start(va, fmt);
    len = vsnprintf(NULL, 0, fmt, va);
    va_end(va);
    if (len < 0)
        goto end;

    p = av_malloc(len + 1);
    if (!p)
        goto end;

    va_start(va, fmt);
    len = vsnprintf(p, len + 1, fmt, va);
    va_end(va);
    if (len < 0)
        av_freep(&p);

end:
    return p;
}

static int gx_tolower(int c)
{
    if (c >= 'A' && c <= 'Z')
        c ^= 0x20;
    return c;
}

static int gx_isdigit(int c)
{
    return c >= '0' && c <= '9';
}

int av_isxdigit(int c)
{
    c = gx_tolower(c);
    return gx_isdigit(c) || (c >= 'a' && c <= 'f');
}
#endif

char *get_http_proxy_url(const GxURL *proxy, const char *host_url)
{
	if (proxy->username)
		return av_asprintf("http_proxy://%s:%s@%s:%d/%s",
				proxy->username,
				proxy->password ? proxy->password : "",
				proxy->hostname, proxy->port, host_url);
	else
		return av_asprintf("http_proxy://%s:%d/%s",
				proxy->hostname, proxy->port, host_url);
}

GxURL* url_redirect(GxURL ** url, const char *redir)
{
	GxURL *u = *url;
	int proxy = is_proxy(u);
	const char *oldurl = proxy ? u->file + 1 : u->url;
	const char *newurl = redir;
	char *buffer = NULL;
	GxURL *res;
	if (!strchr(redir, '/') || *redir == '/') {
		char *tmp;
		newurl = buffer = av_malloc(strlen(oldurl) + strlen(redir) + 1);
		strncpy(buffer, oldurl, strlen(oldurl) + strlen(redir));
		if (*redir == '/') {
			redir++;
			tmp = strstr(buffer, "://");
			if (tmp) tmp = strchr(tmp + 3, '/');
		} else
			tmp = strrchr(buffer, '/');
		if (tmp) tmp[1] = 0;
		strncat(buffer, redir, strlen(redir));
	}
	if (proxy) {
		char *tmp = get_http_proxy_url(u, newurl);
		av_free(buffer);
		newurl = buffer = tmp;
	}
	res = url_new(newurl);
	if(buffer)
		av_free(buffer);
	url_free(u);
	*url = res;
	return res;
}

static char *get_noauth_url(const GxURL *url)
{
	if (url->port)
		return av_asprintf("%s://%s:%d%s",
				url->protocol, url->hostname, url->port, url->file);
	else
		return av_asprintf("%s://%s%s",
				url->protocol, url->hostname, url->file);
}

void GxUrl_GetItemStr(const char* url, const char* item, uint8_t *strBuffer, int32_t strBufferSize)
{
	char *ptr = (char *)url, *ptr1 = NULL, *ptr2 = NULL;

	if(ptr && item){
next:
		ptr1 = strstr(ptr,item);
		if(ptr1){
			if (ptr1[strlen(item)] == '\0' || ptr1[strlen(item)] == ':') {
				ptr1 += (strlen(item)+1);
				ptr2 = strstr(ptr1,"&");
				if(ptr2 == NULL)
					ptr2 =(char*)(ptr+ strlen(ptr));
				if(ptr2-ptr1 > (strBufferSize - 1)) {
					gxlogd("[ERROR]:(%s)-->(%s) too long !...\n",__func__, item);
					return ;
				} else if ((ptr2-ptr1) <= 0) {
					gxlogd("not find item id value\n");
					return ;
				}
				memcpy(strBuffer, ptr1, ptr2-ptr1);
				strBuffer[ptr2-ptr1] = '\0';
				return;
			} else {
				ptr = ptr1 + 1;
				goto next;
			}
		}
	}
}

static int _htoi(const char *s)
{
	int n = 0;

	if (!s) return 0;
	if (*s == '0') {
		s++;
		if ( *s == 'x' || *s == 'X' ) s++;
	}


	while (*s) {
		n <<= 4;
		if(*s <= '9')
			n |= (*s & 0xf);
		else
			n |= ((*s & 0xf) + 9);
		s++;
	}

	return n;
}

void GxUrl_GetItem(const char* url, const char* item, int32_t *retVal)
{
#define VAL_MAX 32

	char *ptr = (char *)url, *ptr1 = NULL, *ptr2 = NULL;
	char value[VAL_MAX] = {0};

	if(ptr && item) {
next:
		ptr1 = strstr(ptr, item);
		if (ptr1) {
			if (ptr1[strlen(item)] == '\0' || ptr1[strlen(item)] == ':') {
				ptr1 += (strlen(item)+1);
				ptr2 = strstr(ptr1,"&");
				if(ptr2 == NULL)
					ptr2 =(char*)(ptr+ strlen(ptr));
				if(ptr2-ptr1 > VAL_MAX) {
					gxlogd("[ERROR]:(%s)-->(%s) too long !...\n",__func__, item);
					return ;
				} else if (ptr2-ptr1 <= 0) {
					gxlogd ("not find id value\n");
					return ;
				}
				memcpy(value,ptr1,ptr2-ptr1);
				value[ptr2-ptr1] = '\0';
				if ((ptr2 - ptr1 > 2) &&
						(ptr1[0] == '0') &&
						(ptr1[1] == 'x'))
					*retVal = _htoi((char*)value);
				else
					*retVal = atoi((char*)value);
				return;
			} else {
				ptr = ptr1 + 1;
				goto next;
			}
		}
	}
	*retVal = -1;
}

char* GxUrl_Create(GxUrlType urltype)
{
	char *url = (char*)av_malloc(sizeof(char)*GX_URL_MAX_LEN);

	if(url) {
		switch(urltype) {
		case GX_URL_DVBT:
			strncpy(url, GX_URL_KEY_DVBT, GX_URL_MAX_LEN-1);
			break;
		case GX_URL_DVBS:
			strncpy(url, GX_URL_KEY_DVBS, GX_URL_MAX_LEN-1);
			break;
		case GX_URL_DVBC:
			strncpy(url, GX_URL_KEY_DVBC, GX_URL_MAX_LEN-1);
			break;
		case GX_URL_DTMB:
			strncpy(url, GX_URL_KEY_DTMB, GX_URL_MAX_LEN-1);
			break;
		case GX_URL_MEM:
			strncpy(url, GX_URL_KEY_MEM, GX_URL_MAX_LEN-1);
			break;
		case GX_URL_DVBNET:
			strncpy(url, GX_URL_KEY_DVBNET, GX_URL_MAX_LEN-1);
			break;
		default:
			av_free(url);
			return NULL;
		}
		return url;
	}

	return NULL;
}

void  GxUrl_Destroy(char *url)
{
	if(url) {
		av_free(url);
	}
}

int GxUrl_SetItem(char* url, const char* item, int32_t itemval, int32_t max_len)
{
	int len, ret = 0;
	char *ptr1 = NULL, *ptr2 = NULL;

	if (url && item) {
		char *tmpstr = av_mallocz(GX_URL_MAX_LEN);
		if (tmpstr == NULL)
			return -1;

		ptr1 = strstr(url, item);
		if (ptr1) {
			ptr2  = strstr(ptr1, "&" );
			ptr1 += strlen(item);
			memcpy(tmpstr, url, ptr1-url+1);
			len = (ptr1-url) + 1;
			if (ptr2 == NULL)
				snprintf(tmpstr + len, GX_URL_MAX_LEN - len, "%d", itemval);
			else
				snprintf(tmpstr + len, GX_URL_MAX_LEN - len, "%d%s", itemval, ptr2);

			len = strlen(tmpstr);
			if (len < max_len) {
				strncpy(url, tmpstr, (len + 1));
			} else {
				gxloge("new url need %d bytes, but max len %d bytes\n", len, max_len);
				ret = -1;
			}
		} else {
			int tmplen = 0;

			len = strlen(url);
			if(*(url+len-1) == '/')
				snprintf(tmpstr, GX_URL_MAX_LEN, "%s:%d", item, itemval);
			else
				snprintf(tmpstr, GX_URL_MAX_LEN, "&%s:%d", item, itemval);
			tmplen = strlen(tmpstr);
			if ((tmplen + len) < max_len) {
				strncat(url, tmpstr, (tmplen + 1));
			} else {
				gxloge("new url need %d bytes, but max len %d bytes\n", len + tmplen, max_len);
				ret = -1;
			}
		}
		av_free(tmpstr);
	}

	return ret;
}


void GxUrl_DelItem(char* url,const char* item)
{
	char *ptr1 = NULL, *ptr2 = NULL;

	if(url && item)
	{
		ptr1= strstr(url, item);
		if(ptr1)
		{
			ptr2  = strstr(ptr1, "&" );
			if(ptr2 == NULL)
			{
				(*(ptr1-1)=='&') ? (*(ptr1-1)='\0') : (*ptr1='\0');
				return ;
			}
			ptr2++;
			strncpy(ptr1, ptr2, (strlen(ptr2) + 1));
		}
	}
}

/**
 * Escape unsafe characters in path in order to pass them safely to the HTTP
 * request. Insipred by the algorithm in GNU wget:
 * - escape "%" characters not followed by two hex digits
 * - escape all "unsafe" characters except which are also "reserved"
 * - pass through everything else
 */
static void escaped_path(char *escfilename, const char *path, char *buf, int buf_size)
{
    int len = 0;
#define NEEDS_ESCAPE(ch) \
    ((ch) <= ' ' || (ch) >= '\x7f' || \
     (ch) == '"' || (ch) == '%' || (ch) == '<' || (ch) == '>' || (ch) == '\\' || \
     (ch) == '^' || (ch) == '`' || (ch) == '{' || (ch) == '}' || (ch) == '|')
    while (*path) {
        memset (buf, 0, buf_size);
        char *q = buf;
        while (*path && q - buf < buf_size - 4) {
            if (path[0] == '%' && av_isxdigit(path[1]) && av_isxdigit(path[2])) {
                *q++ = *path++;
                *q++ = *path++;
                *q++ = *path++;
            } else if (NEEDS_ESCAPE(*path)) {
                q += snprintf(q, 4, "%%%02X", (uint8_t)*path++);
            } else {
                *q++ = *path++;
            }
        }
	    len += snprintf(escfilename + len, q - buf + 1, "%s", buf);
    }
}

GxURL* url_new(const char *s_url)
{
	int pos1, pos2, v6addr = 0;
	GxURL* Curl = NULL;
	char* escfilename = NULL;
	char* url = NULL;
	char* headers = NULL;
	char* ptr1 = NULL, *ptr2 = NULL, *ptr3 = NULL, *ptr4 = NULL;
	int jumpSize = 3;
	int flags = 0;

	if (s_url == NULL)
		return NULL;

	if (strlen(s_url) > (SIZE_MAX / 3 - 1)) {
		// mp_msg(MSGT_NETWORK,MSGL_FATAL,MSGTR_MemAllocFailed);
		goto err_out;
	}

	ptr1 = strstr(s_url, " -H");
	if(ptr1 != NULL){
		int len = ptr1-s_url;
		url = av_malloc(len+1);
		memcpy(url, s_url, len);
		url[len] = '\0';
		len = s_url+strlen(s_url) - ptr1 - 3;
		headers = av_malloc(len+1);
		memcpy(headers, ptr1+3, len);
		headers[len]='\0';
		flags = 1;
		ptr1 = NULL;
	}else
		url = (char*)s_url;

	escfilename = av_malloc(strlen(url)*  3 + 1);
	if (!escfilename) {
		//mp_msg(MSGT_NETWORK,MSGL_FATAL,MSGTR_MemAllocFailed);
		goto err_out;
	}
	// Create the URL container
	Curl = av_malloc(sizeof(GxURL));
	if (Curl == NULL) {
		//mp_msg(MSGT_NETWORK,MSGL_FATAL,MSGTR_MemAllocFailed);
		goto err_out;
	}
	// Initialisation of the URL container members
	memset(Curl, 0, sizeof(GxURL));

	url_escape_string(escfilename, url);

	// Copy the url in the URL container
	Curl->url = av_strdup(escfilename);
	if (Curl->url == NULL) {
		//mp_msg(MSGT_NETWORK,MSGL_FATAL,MSGTR_MemAllocFailed);
		goto err_out;
	}

	//gxlogf("Filename for url is now %s\n", escfilename);

	// extract the protocol
	ptr1 = strstr(escfilename, "://");
	if (ptr1 == NULL) {
		// Check for a special case: "sip:" (without "//"):
		if (strstr(escfilename, "sip:") == escfilename) {
			ptr1 = (char* )&url[3];	// points to ':'
			jumpSize = 1;
		} else {
			// mp_msg(MSGT_NETWORK,MSGL_V,"Not an URL!\n");
			goto err_out;
		}
	}
	pos1 = ptr1 - escfilename;
	Curl->protocol = av_malloc(pos1 + 1);

	if (Curl->protocol == NULL) {
		//mp_msg(MSGT_NETWORK,MSGL_FATAL,MSGTR_MemAllocFailed);
		goto err_out;
	}
	strncpy(Curl->protocol, escfilename, pos1);
	Curl->protocol[pos1] = '\0';

	gxlogf("protocol:%s\n", Curl->protocol);
	// jump the "://"
	ptr1 += jumpSize;
	pos1 += jumpSize;

	// check if a username:password is given
	{
		char *at, *at2;

		at2 = ptr1;
		ptr2 = NULL;
		while ((at = strstr(at2, "@")) && ((at - ptr1) < 1024)) {
			at2 = at + 1;
			ptr2 = at;
		}
	}
	ptr3 = strstr(ptr1, "/");
	if (ptr3 != NULL && ptr3 < ptr2) {
		// it isn't really a username but rather a part of the path
		ptr2 = NULL;
	}
	if (ptr2 != NULL) {
		// We got something, at least a username...
		int len = ptr2 - ptr1;
		Curl->username = av_malloc(len + 1);
		if (Curl->username == NULL) {
			//mp_msg(MSGT_NETWORK,MSGL_FATAL,MSGTR_MemAllocFailed);
			goto err_out;
		}
		strncpy(Curl->username, ptr1, len);
		Curl->username[len] = '\0';

		ptr3 = strstr(ptr1, ":");
		if (ptr3 != NULL && ptr3 < ptr2) {
			// We also have a password
			int len2 = ptr2 - ptr3 - 1;
			Curl->username[ptr3 - ptr1] = '\0';
			Curl->password = av_malloc(len2 + 1);
			if (Curl->password == NULL) {
				//mp_msg(MSGT_NETWORK,MSGL_FATAL,MSGTR_MemAllocFailed);
				goto err_out;
			}
			strncpy(Curl->password, ptr3 + 1, len2);
			Curl->password[len2] = '\0';
			url_unescape_string(Curl->password, Curl->password);
		}
		url_unescape_string(Curl->username, Curl->username);
		ptr1 = ptr2 + 1;
		pos1 = ptr1 - escfilename;
	}

	gxlogf("[HTTP] username=%s:passwd=%s\n", Curl->username, Curl->password);

	// before looking for a port number check if we have an IPv6 type numeric address
	// in IPv6 URL the numeric address should be inside square braces.
	ptr2 = strstr(ptr1, "[");
	ptr3 = strstr(ptr1, "]");
	ptr4 = strstr(ptr1, "/");
	if (ptr2 != NULL && ptr3 != NULL && ptr2 < ptr3 && (!ptr4 || ptr4 > ptr3)) {
		// we have an IPv6 numeric address
		ptr1++;
		pos1++;
		ptr2 = ptr3;
		v6addr = 1;
	} else {
		ptr2 = ptr1;

	}

	// look if the port is given
	ptr2 = strstr(ptr2, ":");
	// If the : is after the first / it isn't the port
	ptr3 = strstr(ptr1, "/");
	if (ptr3 && ptr3 - ptr2 < 0)
		ptr2 = NULL;
	if (ptr2 == NULL) {
		// No port is given
		// Look if a path is given
		if (ptr3 == NULL) {
			// No path/filename
			// So we have an URL like http://www.hostname.com
			pos2 = strlen(escfilename);
		} else {
			// We have an URL like http://www.hostname.com/file.txt
			pos2 = ptr3 - escfilename;
		}
	} else {
		// We have an URL beginning like http://www.hostname.com:1212
		// Get the port number
		Curl->port = atoi(ptr2 + 1);
		pos2 = ptr2 - escfilename;
	}
	if (v6addr)
		pos2--;
	// copy the hostname in the URL container
	Curl->hostname = av_malloc(pos2 - pos1 + 1);
	if (Curl->hostname == NULL) {
		//mp_msg(MSGT_NETWORK,MSGL_FATAL,MSGTR_MemAllocFailed);
		goto err_out;
	}
	strncpy(Curl->hostname, ptr1, pos2 - pos1);
	Curl->hostname[pos2 - pos1] = '\0';

	// Look if a path is given
	ptr2 = strstr(ptr1, "/");
	if (ptr2 != NULL) {
		// A path/filename is given
		// check if it's not a trailing '/'
		if (strlen(ptr2) > 1) {
			// copy the path/filename in the URL container
			if ((ptr3 = strstr(ptr2,"#")) != NULL) {
				jumpSize = ptr3 - ptr2;
			} else if((ptr3 = strstr(ptr2,"%0D%0A"))==NULL) {
				if((ptr3 = strstr(ptr2,"%0A"))==NULL)
					jumpSize = strlen(ptr2);
				else
					jumpSize = ptr3 - ptr2;
			} else
				jumpSize = ptr3 - ptr2;
			Curl->file = av_malloc(jumpSize+1);
			if (Curl->file == NULL) {
				//mp_msg(MSGT_NETWORK,MSGL_FATAL,MSGTR_MemAllocFailed);
				goto err_out;
			}
			memcpy(Curl->file, ptr2, jumpSize);
			Curl->file[jumpSize]='\0';
		}
	} else {
		ptr2 = strstr(ptr1, "?");
		if (ptr2 != NULL) {
			if (strlen(ptr2) > 1) {
				if((ptr3 = strstr(ptr2,"%0D%0A"))==NULL)
				{
					if((ptr3 = strstr(ptr2,"%0A"))==NULL)
						jumpSize = strlen(ptr2);
					else
						jumpSize = ptr3 - ptr2;
				}
				else
					jumpSize = ptr3 - ptr2;
				Curl->file = av_malloc(jumpSize+2);
				if (Curl->file == NULL) {
					goto err_out;
				}
				Curl->file[0] = '/';
				memcpy(Curl->file+1, ptr2, jumpSize);
				Curl->file[jumpSize] = '\0';
			}
		}
	}
	// Check if a filename was given or set, else set it with '/'
	if (Curl->file == NULL) {
		Curl->file = av_malloc(2);
		if (Curl->file == NULL) {
			//mp_msg(MSGT_NETWORK,MSGL_FATAL,MSGTR_MemAllocFailed);
			goto err_out;
		}
		strncpy(Curl->file, "/", 2);
	}

	Curl->noauth_url = get_noauth_url(Curl);
	if (!Curl->noauth_url) {
		//mp_msg(MSGT_NETWORK, MSGL_FATAL, MSGTR_MemAllocFailed);
		goto err_out;
	}
	if((strlen(Curl->url) > 6) && strcmp(Curl->url+(strlen(Curl->url)-6), "%0D%0A") == 0)
		Curl->url[strlen(Curl->url)-6] = '\0';
	else if((strlen(Curl->url) > 3) && strcmp(Curl->url+(strlen(Curl->url)-3), "%0A") == 0)
		Curl->url[strlen(Curl->url)-3] = '\0';

	gxlogf("url=%s\n", Curl->url);
	gxlogf("hostname=%s\n", Curl->hostname);
	gxlogf("file=%s\n", Curl->file);
	if(flags){
		av_free(url);
		av_free(headers);
	}
	av_free(escfilename);
	return Curl;
err_out:
	if(flags){
		av_free(url);
		av_free(headers);
	}
	if (escfilename)
		av_free(escfilename);
	if (Curl)
		url_free(Curl);
	return NULL;
}

void url_free(GxURL*  url)
{
	if (!url)
		return;
	if (url->url)
		av_free(url->url);
	if (url->protocol)
		av_free(url->protocol);
	if (url->hostname)
		av_free(url->hostname);
	if (url->file)
		av_free(url->file);
	if (url->username)
		av_free(url->username);
	if (url->password)
		av_free(url->password);
	if (url->noauth_url)
		av_free(url->noauth_url);
	if(url->user_agent)
		av_free(url->user_agent);
	if(url->referer)
		av_free(url->referer);
	av_free(url);
}


/* Replace escape sequences in an URL (or a part of an URL) */
/* works like strcpy(), but without return argument,
   except that outbuf == inbuf is allowed */
void url_unescape_string(char *outbuf, const char *inbuf)
{
	unsigned char c,c1,c2;
	int i,len=strlen(inbuf);
	for (i=0;i<len;i++){
		c = inbuf[i];
		if (c == '%' && i<len-2) { //must have 2 more chars
			c1 = toupper(inbuf[i+1]); // we need uppercase characters
			c2 = toupper(inbuf[i+2]);
			if (	((c1>='0' && c1<='9') || (c1>='A' && c1<='F')) &&
					((c2>='0' && c2<='9') || (c2>='A' && c2<='F')) ) {
				if (c1>='0' && c1<='9') c1-='0';
				else c1-='A'-10;
				if (c2>='0' && c2<='9') c2-='0';
				else c2-='A'-10;
				c = (c1<<4) + c2;
				i=i+2; //only skip next 2 chars if valid esc
			}
		}
		*outbuf++ = c;
	}
	*outbuf++ = '\0';    //add nullterm to string
}


/* Replace specific characters in the URL string by an escape sequence*/
/* works like strcpy(), but without return argument*/
void url_escape_string(char* outbuf, const char *inbuf)
{
	unsigned char c = 0;
	int i = 0, j, len = strlen(inbuf);
	char* tmp, *unesc = NULL, *in;

	// Look if we have an ip6 address, if so skip it there is
	// no need to escape anything in there.
	tmp = strstr(inbuf, "://[");
	if (tmp) {
		tmp = strchr(tmp + 4, ']');
		if (tmp && (tmp[1] == '/' || tmp[1] == ':' || tmp[1] == '\0')) {
			i = tmp + 1 - inbuf;
			strncpy(outbuf, inbuf, i);
			outbuf += i;
			tmp = NULL;
		}
	}

	tmp = NULL;
	while (i < len) {
		// look for the next char that must be kept
		for (j = i; j < len; j++) {
			c = inbuf[j];
			if (c == '-' || c == '_' || c == '.' || c == '!' || c == '~' ||	/* mark characters*/
					c == '*' || c == '\'' || c == '(' || c == ')' ||	/* do not touch escape character*/
					c == ';' || c == '/' || c == '?' || c == ':' || c == '@' ||	/* reserved characters*/
					c == '#' || c == '&' || c == '=' || c == '+' || c == '$' || c == ',')	/* see RFC 2396*/
				break;
		}
		// we are on a reserved char, write it out
		if (j == i) {
			*outbuf++ = c;
			i++;
			continue;
		}
		// we found one, take that part of the string
		if (j < len) {
			if (!tmp)
				tmp = av_malloc(len + 1);
			strncpy(tmp, inbuf + i, j - i);
			tmp[j - i] = '\0';
			in = tmp;
		} else		// take the rest of the string
			in = (char* )inbuf + i;

		if (!unesc)
			unesc = av_malloc(len + 1);
		memset (unesc, 0, len+1);
		escaped_path(outbuf, in, unesc, len + 1);
		outbuf += strlen(outbuf);
		i += strlen(in);
	}
	*outbuf = '\0';
	if (tmp)
		av_free(tmp);
	if (unesc)
		av_free(unesc);
}

int url_str_copy(char* src_url, char* dst_url, int dst_len)
{
	if((src_url==NULL)||(strlen(src_url)>=dst_len)){
		return -1;
	}
	memset(dst_url, '\0', dst_len);
	strncpy(dst_url, src_url, dst_len);
	return 0;
}

char* url_new_nooptions(const char* src_url)
{
	char *url = NULL;
	char *ptr = NULL;

	ptr = strstr(src_url, " -H");
	if (ptr != NULL) {
		int len = ptr-src_url;

		url = av_malloc(len+1);
		if (!url)
			return NULL;

		memcpy(url, src_url, len);
		url[len] = '\0';
		return url;
	} else
		return av_strdup(src_url);

	return NULL;
}

char *data_to_hex(char *buff, const uint8_t *src, int s, int lowercase)
{
	int i;
	static const char hex_table_uc[16] = { '0', '1', '2', '3',
		'4', '5', '6', '7',
		'8', '9', 'A', 'B',
		'C', 'D', 'E', 'F' };
	static const char hex_table_lc[16] = { '0', '1', '2', '3',
		'4', '5', '6', '7',
		'8', '9', 'a', 'b',
		'c', 'd', 'e', 'f' };
	const char *hex_table = lowercase ? hex_table_lc : hex_table_uc;

	for (i = 0; i < s; i++) {
		buff[i * 2]     = hex_table[src[i] >> 4];
		buff[i * 2 + 1] = hex_table[src[i] & 0xF];
	}

	return buff;
}

int hex_to_data(unsigned char *data, const char *p)
{
	int c, len, v;

	len = 0;
	v = 1;
	for (;;) {
		p += strspn(p, SPACE_CHARS);
		if (*p == '\0')
			break;
		c = av_toupper((unsigned char) *p++);
		if (c >= '0' && c <= '9')
			c = c - '0';
		else if (c >= 'A' && c <= 'F')
			c = c - 'A' + 10;
		else
			break;
		v = (v << 4) | c;
		if (v & 0x100) {
			if (data)
				data[len] = v;
			len++;
			v = 1;
		}
	}
	return len;
}

static void get_key_strings(const char* url, char* keystring, char* buffer)
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
	memcpy(buffer, p, pl);
	buffer[pl] = '\0';
	return;
}

int parse_by_url(char *src, char *keystring, char *buffer)
{
	int l;
	char *ptr1, *ptr2, *ptr3;
	char world[33];

	if (!src || !keystring) return -1;

	ptr1 = src;
	ptr2 = strstr(ptr1, " -H");
	ptr3 = ptr2?strstr(ptr2, keystring):NULL;

	if (!strcasecmp(keystring, "url")) {
		l = ptr2?(ptr2-ptr1):strlen(ptr1);
		memcpy(buffer, ptr1, l);
		buffer[l] = '\0';
		return l;
	}

	if (!ptr2 || !ptr3) return -1;

	if (!strcasecmp(keystring, "enc:")) {
		get_key_strings((const char*)ptr2, keystring, buffer);
		return 0;
	} else if (!strcasecmp(keystring, "key:") || !strcasecmp(keystring, "iv:")) {
		get_key_strings((const char*)ptr2, keystring, world);
		return hex_to_data((unsigned char*)buffer, world);
	}
	return 0;
}

void protocol_by_url(char *url, char *proto, int proto_len)
{
	char *dot = NULL;

	memset(proto, 0, proto_len);
	dot = strstr(url, "://");
	if (dot == NULL)
		memcpy(proto, "file", 4);
	else if ((dot - url) > proto_len)
		return;
	else
		memcpy(proto, url, dot - url);

	return;
}

#ifdef __URL_DEBUG
void GxURL_Debug(const GxURL*  url)
{
	if (url == NULL) {
		gxlogf("URL pointer NULL\n");
		return;
	}
	if (url->url != NULL) {
		gxlogf("url=%s\n", url->url);
	}
	if (url->protocol != NULL) {
		gxlogf("protocol=%s\n", url->protocol);
	}
	if (url->hostname != NULL) {
		gxlogf("hostname=%s\n", url->hostname);
	}
	gxlogf("port=%d\n", url->port);
	if (url->file != NULL) {
		gxlogf("file=%s\n", url->file);
	}
	if (url->username != NULL) {
		gxlogf("username=%s\n", url->username);
	}
	if (url->password != NULL) {
		gxlogf("password=%s\n", url->password);
	}
}
#endif				//__URL_DEBUG
