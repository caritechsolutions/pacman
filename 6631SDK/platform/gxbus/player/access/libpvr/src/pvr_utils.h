#ifndef __PVR_UTILS_H__
#define __PVR_UTILS_H__

#define PVR_MAX_BUF_SIZE   (2048)

#define PVR_MAX_LINE_LEN   PVR_MAX_BUF_SIZE
#define PVR_MAX_URL_LEN    PVR_MAX_BUF_SIZE

#define PVR_RW_ROOT        (O_RDWR|O_CREAT)
#define PVR_IS_WROOT(m)    (m != O_RDONLY)

typedef struct {
	unsigned char  buf[1024];
	unsigned short wptr;
	unsigned short rptr;
	unsigned char  eof;
} PVRLineCache;

extern int   pvr_strlen_line(char *line);
extern char *pvr_strdup_line(char *line);
extern int   pvr_cache_reset(PVRLineCache *cache);
extern int   pvr_cache_read_line(struct record_file *fd,
		PVRLineCache *cache, char *line, unsigned int max_bytes);
extern char* pvr_parse_path(char* filename, int *filename_size);
extern char* pvr_parse_prefix(char* filename, char *suffix);
extern char* pvr_parse_path_remove_suffix(char* filename, char *suffix, int *size);
extern void  pvr_get_absolute_url(char* filename, int filename_size);
extern void  pvr_get_relative_url(char *base_url, char *dst_url, char *result_url);

extern int   pvr_mk_dir(const char *path);
extern int   pvr_rm_dir(const char *path);
extern int   pvr_file_exist(const char *url);
extern int   pvr_rm_file(const char *url);

extern void  pvr_data_to_hex(const char *data, char *hex, unsigned int size);
extern char *pvr_strdup_hex_to_data(char *line);
#endif
