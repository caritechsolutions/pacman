#ifndef __LIB_PVR_H__
#define __LIB_PVR_H__

#include "gx_common.h"
#include "avstring.h"

typedef struct {
	off_t   tl_filesize;
	int64_t tl_duration;
	off_t   tr_filesize;
	int64_t tr_duration;
} PVRInfo;

typedef struct {
	void*  (*open)        (char* name, mode_t mode);
	int    (*read)        (void* priv, uint8_t * buffer, size_t len);
	int    (*write)       (void* priv, uint8_t * buffer, size_t len);
	off_t  (*seek)        (void* priv, off_t offset);
	int    (*close)       (void* priv);
	int    (*getinfo)     (void* priv, PVRInfo *info);
	off_t  (*getpos)      (void* priv);
	int    (*settime)     (void* priv, unsigned int timems);
	size_t (*probehdr)    (void* priv);
	size_t (*readhdr)     (void* priv, uint8_t * buffer, size_t len);
	size_t (*writehdr)    (void* priv, uint8_t * buffer, size_t len);
	int    (*setctrl)     (void* priv, GxRecordPVRControl *ctrl);
	int    (*getctrl)     (void* priv, GxRecordPVRControl *ctrl);
} PVROps;

PVROps *pvr_get_ops(char *filename);

#endif
