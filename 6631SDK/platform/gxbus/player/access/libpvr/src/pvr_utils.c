#include "gx_common.h"
#include "avstring.h"
#include "recfs.h"
#include "pvr_config.h"
#include "pvr_utils.h"
#include "pvr_file.h"
#include "pvr_parser.h"
#include "gx_url.h"

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

int pvr_strlen_line(char *line)
{
	char *cs = line;

	while (1) {
		if (*cs == '\n' || *cs == '\0')
			break;
		cs++;
	}

	return (int)(cs - line);
}

char *pvr_strdup_line(char *line)
{
	int bytes = pvr_strlen_line(line);
	char *ptr = NULL;

	if (bytes == 0)
		return NULL;

	ptr = av_malloc(bytes + 1);
	memcpy(ptr, line, bytes);
	ptr[bytes] = '\0';

	return ptr;
}

int pvr_cache_reset(PVRLineCache *cache)
{
	cache->eof = 0;
	cache->wptr = cache->rptr = 0;

	return 0;
}

void pvr_data_to_hex(const char *data, char *hex, unsigned int size)
{
	int len = strlen(data) + 1;
	int pos = 2 * len + 1;

	if ((pos + 1) >= size) {
		gxloge("%s %d: [%s]\n", __func__, __LINE__, data);
		hex[0] = '\0';
		return;
	}
	data_to_hex(hex, (const uint8_t *)data, len, 1);
	hex[pos] = '\0';
	return;
}

char *pvr_strdup_hex_to_data(char *line)
{
	int bytes = pvr_strlen_line(line);
	char *ptr = NULL;

	if (bytes == 0)
		 return NULL;

	ptr = av_mallocz(bytes / 2 + 1);
	hex_to_data((unsigned char *)ptr, line);

	return ptr;
}

int pvr_cache_read_line(struct record_file *fd,
		PVRLineCache *cache, char *line, unsigned int max_bytes)
{
	char *cs = line;

	while (1) {
		if (cache->eof && (cache->wptr == cache->rptr))
			return (cs - line);

		while (cache->wptr != cache->rptr) {
			if (cache->buf[cache->rptr] == '\n') {
				*cs++ = '\n';
				cache->rptr++;
				return (cs - line);
			}

			*cs++ = cache->buf[cache->rptr];
			cache->rptr++;
			if ((cs - line + 1) >= max_bytes) {
				*cs++ = '\n';
				return (cs - line);
			}
		}

		cache->wptr = recfile_read(fd, cache->buf, sizeof(cache->buf));
		cache->rptr = 0;
		if (cache->wptr < sizeof(cache->buf))
			cache->eof = 1;
	}

	return 0;
}

char* pvr_parse_path(char* filename, int *filename_size)
{
	char *ptr = filename;
	char *path = NULL, *pathptr = NULL;

	while (1) {
		ptr = strchr(ptr, '/');
		if (!ptr)
			break;
		pathptr = ptr;
		ptr++;
	}

	if (pathptr) {
		*filename_size = pathptr - filename + 1;
		if (!(path = av_mallocz(*filename_size)))
			return NULL;
		memcpy(path, filename, pathptr - filename);
	} else
		gxloge("%s %d: [%s]\n", __func__, __LINE__, filename);

	return path;
}

char* pvr_parse_prefix(char* filename, char *suffix)
{
	char *ptr = filename;
	char *prefix = NULL, *pathptr = NULL;

	while (1) {
		ptr = strchr(ptr, '/');
		if (!ptr)
			break;
		pathptr = ptr;
		ptr++;
	}

	if (NULL == pathptr)
		pathptr = filename;
	else
		pathptr++;
	ptr = av_stristr(pathptr, suffix);
	if (ptr) {
		prefix = av_mallocz(ptr - pathptr + 1);
		memcpy(prefix, pathptr, ptr - pathptr);
	} else
		gxloge("%s %d: [%s]\n", __func__, __LINE__, filename);

	return prefix;
}

char* pvr_parse_path_remove_suffix(char* filename, char *suffix, int *size)
{
	char *ptr = filename;
	char *path = NULL, *pathptr = NULL;

	pathptr = av_stristr((const char *)filename, suffix);
	if (!pathptr)
		return NULL;

	if (pathptr) {
		*size = pathptr - filename + 1;
		if (!(path = av_mallocz(*size)))
			return NULL;
		memcpy(path, filename, pathptr - filename);
	} else
		gxloge("%s %d: [%s]\n", __func__, __LINE__, filename);

	return path;
}

void pvr_get_absolute_url(char* filename, int filename_size)
{
	char *ptr = filename;
	char *ptr0 = filename, *ptr1 = NULL, *ptr2 = NULL;

	while (1) {
		ptr1 = av_stristr(ptr0, "../");
		ptr2 = av_stristr(ptr0, "./");
		if (!ptr1 && !ptr2) {
			break;
		} else if (ptr1) {
			if (ptr1 != ptr) {
				int len = 0, i = 0;

				ptr1--;
				ptr1[0] = '\0';
				len = strlen(ptr);
				for (i = len; i > 0; i--) {
					if (ptr[i] == '/') {
						ptr[i] = '\0';
						break;
					}
				}
				ptr1 += 3;
				strncat(ptr, ptr1, filename_size);
			} else
				ptr0 = ptr1 + 3;
		} else if (ptr2) {
			if (ptr2 != ptr) {
				ptr2--;
				ptr2[0] = '\0';
				ptr2 += 2;
				strncat(ptr, ptr2, filename_size);
			} else
				ptr0 = ptr2 + 2;
		}
	}

	return;
}

void pvr_get_relative_url(char *base_url, char *dst_url, char *result_url)
{
	int i = 0, j = 0, k = 0, x = 0;

	pvr_get_absolute_url(base_url, PVR_MAX_URL_LEN);
	pvr_get_absolute_url(dst_url, PVR_MAX_URL_LEN);

	while (base_url[i] != '\0' &&
			dst_url[i] != '\0' &&
			base_url[i] == dst_url[i]) {
		if (base_url[i] == '/')
			x = (i + 1);
		i++;
	}
	k = x;
	while (base_url[k] != '\0') {
		if (base_url[k++] == '/') {
			result_url[j++] = '.';
			result_url[j++] = '.';
			result_url[j++] = '/';
		}
	}
	while (dst_url[x] != '\0') {
		result_url[j++] = dst_url[x++];
	}
	result_url[j] = '\0';

	return;
}

int pvr_mk_dir(const char *path)
{
	if (GxCore_FileExists(path) == GXCORE_FILE_EXIST)
		return 0;

	if (GxCore_Mkdir(path) == GXCORE_SUCCESS)
		return 1;

	return -1;
}

int pvr_rm_dir(const char *path)
{
	if (GxCore_FileExists(path) != GXCORE_FILE_EXIST)
		return 0;

	if (GxCore_Rmdir(path) == GXCORE_SUCCESS)
		return 1;

	return -1;
}

int pvr_file_exist(const char *url)
{
	if (GxCore_FileExists(url) != GXCORE_FILE_EXIST)
		return 0;

	return 1;
}

int pvr_rm_file(const char *url)
{
	if (GxCore_FileExists(url) != GXCORE_FILE_EXIST)
		return 0;

	if (GxCore_FileDelete((const char*)url) != GXCORE_SUCCESS)
		return -1;

	return 1;
}
