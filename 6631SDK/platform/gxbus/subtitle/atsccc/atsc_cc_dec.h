#ifndef __ATSC_CC_DEC_H__
#define __ATSC_CC_DEC_H__

typedef struct {
	unsigned int xr;
	unsigned int xl;
	unsigned int yt;
	unsigned int yb; //100相对值
} ATSC_CC_DISPLAY;

typedef int (*AtscccEvent)(vbi_page *page, void *priv, ATSC_CC_DISPLAY *disp);

handle_t AtscccCreate (void);
void     AtscccDestory(handle_t handle);
int      AtscccDecode (handle_t handle, unsigned char *buf, unsigned int len);
void     AtscccRegisterEvent(handle_t handle, AtscccEvent ev, void *priv);

#endif
