#include "libpvr.h"
#include "src/pvr_config.h"

extern PVROps pvr_pvr_ops;
extern PVROps pvr_evt_ops;
extern PVROps pvr_lst_ops;
extern PVROps pvr_seg_ops;

#ifndef CONFIG_FFMPEG_ENABLE
#define av_stristr gx_stristr

static int gx_toupper(int c)
{
    if (c >= 'a' && c <= 'z')
        c ^= 0x20;
    return c;
}

static int gx_stristart(const char *str, const char *pfx, const char **ptr)
{
    while (*pfx && gx_toupper((unsigned)*pfx) == gx_toupper((unsigned)*str)) {
        pfx++;
        str++;
    }
    if (!*pfx && ptr)
        *ptr = str;
    return !*pfx;
}

static char *gx_stristr(const char *s1, const char *s2)
{
	if (!*s2)
		return (char*)s1;

	do
		if (gx_stristart(s1, s2, NULL))
			return (char*)s1;
	while (*s1++);

	return NULL;
}
#endif

PVROps *pvr_get_ops(char *filename)
{
	if (av_stristr(filename, PVR_SEGFILE_SUFFIX))
		return &pvr_seg_ops;
	else if (av_stristr(filename, PVR_EVTFILE_SUFFIX))
		return &pvr_evt_ops;
	else if (av_stristr(filename, PVR_LSTFILE_SUFFIX))
		return &pvr_lst_ops;
	else if (av_stristr(filename, PVR_PVRFILE_SUFFIX))
		return &pvr_pvr_ops;

	return NULL;
}
