#include "../include/vod_in_common_def.h"
#include "../include/vod_common_func.h"
#include "../include/vod_porting_all.h"
#include <sys/time.h>

char* str_get_word(char* line, char* word)
{
	char *lp, *wp;
	int	quot_flag;

	wp = word;
	lp = line;
	if (lp == 0)
	{
		*wp = '\0';
		return 0;
	}

	while (*lp == ' ' || *lp == '\t' || *lp == '=' || *lp == ':' || *lp == ';' || *lp == '\r' || *lp == '\n')
		lp++;

	if (*lp == '"')
	{
		quot_flag = 1;
		lp++;
	}
	else
		quot_flag = 0;

	while (*lp)
	{
		switch (*lp)
		{
		case ' ' :
		case '\t' :
		case ';' :
			if (quot_flag)
			{
				*wp = *lp;
			}
			else
			{
				*wp = '\0';
				lp++;
				return ((wp - word > 0) ? lp : 0);
			}
			break;

		case '\r' :
		case '\n' :
			*wp = '\0';
			lp++;
			return ((wp - word > 0 ) ? lp : 0);

		case '"' :
			if (quot_flag)
			{
				*wp = '\0';
				lp++;
				return ((wp - word > 0) ? lp : 0);
			}
			else
			{
				*wp = *lp;
			}
			break;

		default :
			*wp = *lp;
			break;
		}
		lp++;
		wp++;
	}
	*wp = '\0';
	return ((wp - word > 0) ? lp : 0);
}

static char* stristr(char* pString, const char* pFind)
{
	char* char1 = NULL;
	char* char2 = NULL;
	if((pString == NULL) || (pFind == NULL) || (strlen(pString) < strlen(pFind)))
	{
		return NULL;
	}

	for(char1 = (char*)pString; (*char1) != '\0'; ++char1)
	{
		char* char3 = char1;
		for(char2 = (char*)pFind; (*char2) != '\0' && (*char1) != '\0'; ++char2, ++char1)
		{
			char c1 = (*char1) & 0xDF;
			char c2 = (*char2) & 0xDF;
			if((c1 != c2) || (((c1 > 0x5A) || (c1 < 0x41)) && (*char1 != *char2))) // 此处重新编辑了下  
				break;
		}

		if((*char2) == '\0')
			return char3;

		char1 = char3;
	}
	return NULL;
}

char* str_get_field(char* string, char* title, char* value)
{
	char* pos;

	pos = stristr(string, title);
	if (pos)
	{
		pos += strlen(title);
		return str_get_word(pos, value);
	}
	return 0;
}

char* str_get_html_field(char* string, char* title, char* value)
{
	char* pos;

	pos = strstr(string, title);
	if (pos)
	{
		int n;
		char *p;

		pos += strlen(title);
		p = strchr(pos, '<');
		if (p)
		{
			n = p - pos;
			memcpy(value, pos, n);
			value[n] = '\0';
			return p;
		}
	}
	return 0;
}

char* str_save(char* string)
{
    char *p;

    if (string == 0)
        return 0;

    p = (char*)xmalloc(strlen(string)+1);
    if (p)
    	strcpy(p, string);

    return p;
}

void str_free(char* string)
{
	xfree(string);
}

int socket_recv_by_linebreak(int sock, char* buffer, int buflen, int timeout)
{
	int ret, len;
	char* p;
	
	len = 0;
	p = buffer;

	memset(p, 0, buflen);
	while (1)
	{
		ret = vod_porting_socket_select(sock, timeout);
		if (ret > 0)
		{
			ret = vod_porting_socket_recv(sock, p, 1);
			if (ret < 0)
			{
				GxVod_debug("recv by linebreak: recv err, received %d\n", len);
				return -1;
			}
			else if(ret == 0)
			{
				return -2;
			}

			if ((len+ret >= 4) && (strcmp(p + ret - 4, "\r\n\r\n") == 0))
			{
				len += ret;
				buffer[len-1] = 0;
				break;
			}
			else
			{
				len += ret;
				p += ret;
			}
			if (len >= buflen)
			{
				return 0;
			}
		}
		else if (ret == 0)
		{
			return -2;
		}
		else
		{
			GxVod_debug("recv by linebreak: select err, received %d\n", ret);
			return -1;
		}
	}

	return 0;
}

int socket_recv_by_length(int sock, char* buffer, int length, int timeout)
{
	int ret, left;
	char* p;

	left = length;
	p = buffer;
	while (left > 0)
	{
		ret = vod_porting_socket_select(sock, timeout);
		if (ret > 0)
		{
			ret = vod_porting_socket_recv(sock, p, left);
			if (ret > 0)
			{
				p += ret;
				left -= ret;
			}
			else
			{
				GxVod_debug("recv by length: recv err, received %d\n", length - left);
				return -1;
			}
		}
		else if (ret == 0)
		{
			GxVod_debug("recv by length: select timeout, received %d\n", length - left);
			return -2;
		}
		else
		{
			GxVod_debug("recv by length: select err, received %d\n", length - left);
			return -1;
		}
	}

	buffer[length] = '\0';
	
	return 0;
}

char* time_to_rtspstr(unsigned int time)
{
	struct tm* t;
	static char s[17];

	memset(s, 0, 17);
	t = localtime((time_t*)&time);
	sprintf(s, "%04d%02d%02dT%02d%02d%02dZ", t->tm_year + 1900,
		                                      t->tm_mon + 1,
											  t->tm_mday,
											  t->tm_hour,
											  t->tm_min,
											  t->tm_sec);
	return s;
}

char* time_to_ttvstr(unsigned int time)
{
	struct tm* t;
	static char s[20];

	memset(s, 0, 20);
	t = localtime((time_t*)&time);
	sprintf(s, "%04d-%02d-%02dT%02d:%02d:%02d", t->tm_year + 1900,
		                                      t->tm_mon + 1,
											  t->tm_mday,
											  t->tm_hour,
											  t->tm_min,
											  t->tm_sec);
	return s;
}

char* time_to_short_human_str(unsigned int time)
{
	struct tm* t;
	static char s[15];

	memset(s, 0, 15);
	t = localtime((time_t*)&time);
	sprintf(s, "%d月%02d日 %02d:%02d", t->tm_mon + 1,
									   t->tm_mday,
						               t->tm_hour,
									   t->tm_min);
	return s;
}

char* time_to_human_str(unsigned int time)
{
	struct tm* t;
	static char s[24];

	memset(s, 0, 24);
	t = localtime((time_t*)&time);
	sprintf(s, "%04d年%02d月%02d日 %02d:%02d:%02d", t->tm_year + 1900,
						                             t->tm_mon + 1,
												     t->tm_mday,
						                             t->tm_hour,
													 t->tm_min,
													 t->tm_sec);
	return s;
}

/* str格式: 20080720112612*/
unsigned int urlstr_to_time(char* timestr)
{
	struct tm time;
	char s[5];
	int year, month, day, hour, minute, second;

	memcpy(s, timestr + 0, 4);
	s[4] = '\0';
	year = atoi(s);

	memcpy(s, timestr + 4, 2);
	s[2] = '\0';
	month = atoi(s);
	
	memcpy(s, timestr + 6, 2);
	s[2] = '\0';
	day = atoi(s);

	memcpy(s, timestr + 8, 2);
	s[2] = '\0';
	hour = atoi(s);

	memcpy(s, timestr + 10, 2);
	s[2] = '\0';
	minute = atoi(s);
	
	memcpy(s, timestr + 12, 2);
	s[2] = '\0';
	second = atoi(s);

    memset(&time, 0, sizeof(time));
    time.tm_year = year - 1900;
    time.tm_mon = month - 1;
    time.tm_mday = day;
    time.tm_hour = hour;
    time.tm_min = minute;
    time.tm_sec = second;

    return (mktime(&time));
}

/* str格式: 20080720T112612Z*/
unsigned int rtspstr_to_time(char* timestr)
{
	struct tm time;
	char s[5];
	int year, month, day, hour, minute, second;
	
	memcpy(s, timestr + 0, 4);
	s[4] = '\0';
	year = atoi(s);

	memcpy(s, timestr + 4, 2);
	s[2] = '\0';
	month = atoi(s);
	
	memcpy(s, timestr + 6, 2);
	s[2] = '\0';
	day = atoi(s);

	memcpy(s, timestr + 9, 2);
	s[2] = '\0';
	hour = atoi(s);

	memcpy(s, timestr + 11, 2);
	s[2] = '\0';
	minute = atoi(s);
	
	memcpy(s, timestr + 13, 2);
	s[2] = '\0';
	second = atoi(s);

    memset(&time, 0, sizeof(time));
    time.tm_year = year - 1900;
    time.tm_mon = month - 1;
    time.tm_mday = day;
    time.tm_hour = hour;
    time.tm_min = minute;
    time.tm_sec = second;

    return (mktime(&time));
}

unsigned int get_url_time(int type, char* url)
{
	char *title, *p1, *p2;

	if (type == 0)
		title = "startTime=";
	else
		title = "endTime=";

	p1 = strstr(url, title);
	if (p1)
	{
		p1 += strlen(title);
		p2 = strchr(p1, '&');
		if (p2 - p1 == 14)
		{
			return urlstr_to_time(p1);
		}
	}
	return 0;
}

int64_t get_tick_time(void)
{
	struct timeval tv;
	//  float s;
	gettimeofday(&tv, NULL);
	//  s=tv.tv_usec;s*=0.000001;s+=tv.tv_sec;
	return (int64_t)(tv.tv_sec*  1000 + tv.tv_usec / 1000);
}


