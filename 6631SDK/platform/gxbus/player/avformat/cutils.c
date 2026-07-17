/*
*  Various simple utilities for ffmpeg system
*  Copyright (c) 2000, 2001, 2002 Fabrice Bellard
*
*  This file is part of FFmpeg.
*
*  FFmpeg is free software; you can redistribute it and/or
*  modify it under the terms of the GNU Lesser General Public
*  License as published by the Free Software Foundation; either
*  version 2.1 of the License, or (at your option) any later version.
*
*  FFmpeg is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  Lesser General Public License for more details.
*
*  You should have received a copy of the GNU Lesser General Public
*  License along with FFmpeg; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/
#include "avformat.h"

#define FF_DYNARRAY_ADD(av_size_max, av_elt_size, av_array, av_size, \
                        av_success, av_failure) \
	do { \
		size_t av_size_new = (av_size); \
		if (!((av_size) & ((av_size) - 1))) { \
			av_size_new = (av_size) ? (av_size) << 1 : 1; \
			if (av_size_new > (av_size_max) / (av_elt_size)) { \
				av_size_new = 0; \
			} else { \
				void *av_array_new = \
				av_realloc((av_array), av_size_new * (av_elt_size)); \
				if (!av_array_new) \
					av_size_new = 0; \
				else \
					(av_array) = av_array_new; \
				} \
		} \
		if (av_size_new) { \
			{ av_success } \
			(av_size)++; \
		} else { \
			av_failure \
		} \
	} while (0)

/* add one element to a dynamic array*/
void av_dynarray_add(void *tab_ptr, int *nb_ptr, void *elem)
{
	void **tab;
	memcpy(&tab, tab_ptr, sizeof(tab));

	FF_DYNARRAY_ADD(INT_MAX, sizeof(*tab), tab, *nb_ptr, {
		tab[*nb_ptr] = elem;
		memcpy(tab_ptr, &tab, sizeof(tab));
	}, {
		*nb_ptr = 0;
		av_freep(tab_ptr);
	});
}

int av_dynarray_add_nofree(void *tab_ptr, int *nb_ptr, void *elem)
{
    void **tab;
    memcpy(&tab, tab_ptr, sizeof(tab));

    FF_DYNARRAY_ADD(INT_MAX, sizeof(*tab), tab, *nb_ptr, {
        tab[*nb_ptr] = elem;
        memcpy(tab_ptr, &tab, sizeof(tab));
    }, {
        return AVERROR(ENOMEM);
    });
    return 0;
}

time_t mktimegm(struct tm* tm)
{
	time_t t;

	int y = tm->tm_year + 1900, m = tm->tm_mon + 1, d = tm->tm_mday;

	if (m < 3) {
		m += 12;
		y--;
	}

	t = 86400*  (d + (153 * m - 457) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 719469);

	t += 3600*  tm->tm_hour + 60 * tm->tm_min + tm->tm_sec;

	return t;
}

#define ISLEAP(y) (((y) % 4 == 0) && (((y) % 100) != 0 || ((y) % 400) == 0))
#define LEAPS_COUNT(y) ((y)/4 - (y)/100 + (y)/400)

/* This is our own gmtime_r. It differs from its POSIX counterpart in a
   couple of places, though.*/
struct tm* brktimegm(time_t secs, struct tm *tm)
{
	int days, y, ny, m;
	int md[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	days = secs / 86400;
	secs %= 86400;
	tm->tm_hour = secs / 3600;
	tm->tm_min = (secs % 3600) / 60;
	tm->tm_sec = secs % 60;

	/* oh well, may be someone some day will invent a formula for this stuff*/
	y = 1970;		/* start "guessing"*/
	while (days >= (ISLEAP(y) ? 366 : 365)) {
		ny = (y + days / 366);
		days -= (ny - y)*  365 + LEAPS_COUNT(ny - 1) - LEAPS_COUNT(y - 1);
		y = ny;
	}
	md[1] = ISLEAP(y) ? 29 : 28;
	for (m = 0; days >= md[m]; m++)
		days -= md[m];

	tm->tm_year = y;	/* unlike gmtime_r we store complete year here*/
	tm->tm_mon = m + 1;	/* unlike gmtime_r tm_mon is from 1 to 12*/
	tm->tm_mday = days + 1;

	return tm;
}

/* get a positive number between n_min and n_max, for a maximum length
   of len_max. Return -1 if error.*/
static int date_get_num(const char* *pp, int n_min, int n_max, int len_max)
{
	int i, val, c;
	const char* p;

	p =* pp;
	val = 0;
	for (i = 0; i < len_max; i++) {
		c =* p;
		if (!isdigit(c))
			break;
		val = (val*  10) + c - '0';
		p++;
	}
	/* no number read ?*/
	if (p ==* pp)
		return -1;
	if (val < n_min || val > n_max)
		return -1;
	*pp = p;
	return val;
}

/* small strptime for ffmpeg*/
const char* small_strptime(const char *p, const char *fmt, struct tm *dt)
{
	int c, val;

	for (;;) {
		c =* fmt++;
		if (c == '\0') {
			return p;
		} else if (c == '%') {
			c =* fmt++;
			switch (c) {
			case 'H':
				val = date_get_num(&p, 0, 23, 2);
				if (val == -1)
					return NULL;
				dt->tm_hour = val;
				break;
			case 'M':
				val = date_get_num(&p, 0, 59, 2);
				if (val == -1)
					return NULL;
				dt->tm_min = val;
				break;
			case 'S':
				val = date_get_num(&p, 0, 59, 2);
				if (val == -1)
					return NULL;
				dt->tm_sec = val;
				break;
			case 'Y':
				val = date_get_num(&p, 0, 9999, 4);
				if (val == -1)
					return NULL;
				dt->tm_year = val - 1900;
				break;
			case 'm':
				val = date_get_num(&p, 1, 12, 2);
				if (val == -1)
					return NULL;
				dt->tm_mon = val - 1;
				break;
			case 'd':
				val = date_get_num(&p, 1, 31, 2);
				if (val == -1)
					return NULL;
				dt->tm_mday = val;
				break;
			case '%':
				goto match;
			default:
				return NULL;
			}
		} else {
		      match:
			if (c !=* p)
				return NULL;
			p++;
		}
	}
	return p;
}

time_t av_timegm(struct tm *tm)
{
    time_t t;

    int y = tm->tm_year + 1900, m = tm->tm_mon + 1, d = tm->tm_mday;

    if (m < 3) {
        m += 12;
        y--;
    }

    t = 86400LL *
        (d + (153 * m - 457) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 719469);

    t += 3600 * tm->tm_hour + 60 * tm->tm_min + tm->tm_sec;

    return t;
}

int av_parse_time(int64_t *timeval, const char *timestr, int duration)
{
    const char *p, *q;
    int64_t t;
    time_t now;
    struct tm dt = { 0 };
    int today = 0, negative = 0, microseconds = 0;
    int i;
    static const char * const date_fmt[] = {
        "%Y-%m-%d",
        "%Y%m%d",
    };
    static const char * const time_fmt[] = {
        "%H:%M:%S",
        "%H%M%S",
    };

    p = timestr;
    q = NULL;
    *timeval = INT64_MIN;
    if (!duration) {
        now = time(0);

        if (!av_strcasecmp(timestr, "now")) {
            *timeval = (int64_t) now * 1000000;
            return 0;
        }

        /* parse the year-month-day part */
        for (i = 0; i < FF_ARRAY_ELEMS(date_fmt); i++) {
            q = small_strptime(p, date_fmt[i], &dt);
            if (q)
                break;
        }

        /* if the year-month-day part is missing, then take the
         * current year-month-day time */
        if (!q) {
            today = 1;
            q = p;
        }
        p = q;

        if (*p == 'T' || *p == 't' || *p == ' ')
            p++;

        /* parse the hour-minute-second part */
        for (i = 0; i < FF_ARRAY_ELEMS(time_fmt); i++) {
            q = small_strptime(p, time_fmt[i], &dt);
            if (q)
                break;
        }
    } else {
        /* parse timestr as a duration */
        if (p[0] == '-') {
            negative = 1;
            ++p;
        }
        /* parse timestr as HH:MM:SS */
        q = small_strptime(p, time_fmt[0], &dt);
        if (!q) {
            /* parse timestr as S+ */
            dt.tm_sec = strtol(p, (void *)&q, 10);
            if (q == p) /* the parsing didn't succeed */
                return AVERROR(EINVAL);
            dt.tm_min = 0;
            dt.tm_hour = 0;
        }
    }

    /* Now we have all the fields that we can get */
    if (!q)
        return AVERROR(EINVAL);

    /* parse the .m... part */
    if (*q == '.') {
        int n;
        q++;
        for (n = 100000; n >= 1; n /= 10, q++) {
            if (!isdigit(*q))
                break;
            microseconds += n * (*q - '0');
        }
    }

    if (duration) {
        t = dt.tm_hour * 3600 + dt.tm_min * 60 + dt.tm_sec;
    } else {
        int is_utc = *q == 'Z' || *q == 'z';
        q += is_utc;
        if (today) { /* fill in today's date */
            struct tm dt2 = is_utc ? *gmtime(&now) : *localtime(&now);
            dt2.tm_hour = dt.tm_hour;
            dt2.tm_min  = dt.tm_min;
            dt2.tm_sec  = dt.tm_sec;
            dt = dt2;
        }
        t = is_utc ? av_timegm(&dt) : mktime(&dt);
    }

    /* Check that we are at the end of the string */
    if (*q)
        return AVERROR(EINVAL);

    t *= 1000000;
    t += microseconds;
    *timeval = negative ? -t : t;
    return 0;
}

int av_find_info_tag(char *arg, int arg_size, const char *tag1, const char *info)
{
    const char *p;
    char tag[128], *q;

    p = info;
    if (*p == '?')
        p++;
    for(;;) {
        q = tag;
        while (*p != '\0' && *p != '=' && *p != '&') {
            if ((q - tag) < sizeof(tag) - 1)
                *q++ = *p;
            p++;
        }
        *q = '\0';
        q = arg;
        if (*p == '=') {
            p++;
            while (*p != '&' && *p != '\0') {
                if ((q - arg) < arg_size - 1) {
                    if (*p == '+')
                        *q++ = ' ';
                    else
                        *q++ = *p;
                }
                p++;
            }
        }
        *q = '\0';
        if (!strcmp(tag, tag1))
            return 1;
        if (*p != '&')
            break;
        p++;
    }
    return 0;
}

static int date_get_month(const char **pp) {
    int i = 0;
    char *months[12] = {
        "january", "february", "march", "april", "may", "june", "july", "august",
        "september", "october", "november", "december"
    };

    for (; i < 12; i++) {
        if (!av_strncasecmp(*pp, months[i], 3)) {
            const char *mo_full = months[i] + 3;
            int len = strlen(mo_full);
            *pp += 3;
            if (len > 0 && !av_strncasecmp(*pp, mo_full, len))
                *pp += len;
            return i;
        }
    }
    return -1;
}

char *av_small_strptime(const char *p, const char *fmt, struct tm *dt)
{
    int c, val;

    while((c = *fmt++)) {
        if (c != '%') {
            if (av_isspace(c))
                for (; *p && av_isspace(*p); p++);
            else if (*p != c)
                return NULL;
            else p++;
            continue;
        }

        c = *fmt++;
        switch(c) {
        case 'H':
        case 'J':
            val = date_get_num(&p, 0, c == 'H' ? 23 : INT_MAX, c == 'H' ? 2 : 4);

            if (val == -1)
                return NULL;
            dt->tm_hour = val;
            break;
        case 'M':
            val = date_get_num(&p, 0, 59, 2);
            if (val == -1)
                return NULL;
            dt->tm_min = val;
            break;
        case 'S':
            val = date_get_num(&p, 0, 59, 2);
            if (val == -1)
                return NULL;
            dt->tm_sec = val;
            break;
        case 'Y':
            val = date_get_num(&p, 0, 9999, 4);
            if (val == -1)
                return NULL;
            dt->tm_year = val - 1900;
            break;
        case 'm':
            val = date_get_num(&p, 1, 12, 2);
            if (val == -1)
                return NULL;
            dt->tm_mon = val - 1;
            break;
        case 'd':
            val = date_get_num(&p, 1, 31, 2);
            if (val == -1)
                return NULL;
            dt->tm_mday = val;
            break;
        case 'T':
            p = av_small_strptime(p, "%H:%M:%S", dt);
            if (!p)
                return NULL;
            break;
        case 'b':
        case 'B':
        case 'h':
            val = date_get_month(&p);
            if (val == -1)
                return NULL;
            dt->tm_mon = val;
            break;
        case '%':
            if (*p++ != '%')
                return NULL;
            break;
        default:
            return NULL;
        }
    }

    return (char*)p;
}


