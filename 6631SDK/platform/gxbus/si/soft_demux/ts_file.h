#ifndef __TS_FILE__
#define __TS_FILE__

#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <gxtype.h>

#define STREAM_READ     0
#define STREAM_WRITE   1

typedef struct {
	int                fd;
	uint32_t            pos;
	uint32_t           start_pos;
	uint32_t            end_pos;
	int                eof;
	char*            url;
}GxTsFile;

typedef struct
{
	char*               name;

	int  (*open)        (GxTsFile *stream, int mode);
	int  (*read)        (GxTsFile *stream, uint8_t* buffer, size_t max_len);
	int  (*write)       (GxTsFile *stream, uint8_t* buffer, size_t len);
	int  (*seek)        (GxTsFile *stream, uint32_t pos);
//	int  (*control)     (GxTsFile *stream, int cmd, va_list arg);
	void (*close)       (GxTsFile *stream);
}GxTsFileCtrl;



extern GxTsFileCtrl TsFileCtrl;
extern GxTsFile g_TsFile;

#endif

