#ifndef __ARIB_CC_DEC_H__
#define __ARIB_CC_DEC_H__

typedef int (*AribccEvent)(void *priv, GxSubtitleShowTextPage *page);

extern void* AribccCreate(void);

extern void AribccDestory(void *handle);

extern void AribccDecode(void *handle, uint8_t *buffer, uint32_t size);

extern void AribccRegisterEvent(void *handle, void *priv, AribccEvent ev);

#endif
