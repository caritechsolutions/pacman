
#ifndef __PLAYER_FILE_OPS_H__
#define __PLAYER_FILE_OPS_H__

#include "gx_common.h"

struct vol_info{
	off_t size;
	off_t truesize;
};

struct vol_hdr{
	unsigned char *buffer;
	size_t         size;
};

struct vol_new{
	size_t         size;
	int            is_newvol;
	int            newvol_ok;
};

typedef struct {
	int    (*open)     (char* name , mode_t mode);
	int    (*read)     (int fildes, uint8_t * buffer, size_t max_len, int timeoutms);
	int    (*read_ext) (int fildes, uint8_t * buffer, size_t max_len, int timeoutms, off_t *outpos);
	int    (*write)    (int fildes, uint8_t * buffer, size_t len);
	off_t  (*seek)     (int fildes, off_t offset, int whence);
	off_t  (*seek_ext) (int fildes, off_t offset, int whence, off_t *outpos);
	int    (*close)    (int fildes);
	off_t  (*getsize)  (int fildes);
	struct vol_info (*getvolinfo)  (int fildes);
	int    (*geterrorcode)         (int fildes);
	void   (*settimestamp)(int fildes, unsigned int timems);
	int    (*getnewvolume)(int fildes, struct vol_new * volnew);
	int    (*sync_data)(int fildes);
	int    (*read_hdr) (int fildes, uint8_t * buffer, size_t max_len, int timeoutms);
	int    (*write_hdr)(int fildes, uint8_t * buffer, size_t len);
	int    (*probe_hdr)(int fildes);
	int    (*set_pvrctrl)(int fildes, GxRecordPVRControl *ctrl);
	int    (*get_pvrctrl)(int fildes, GxRecordPVRControl *ctrl);
}GxFileOps;

extern GxFileOps* GxFileGetOps(char* name);

#endif


