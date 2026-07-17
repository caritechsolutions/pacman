#ifndef __GX_AVTIMER_H__
#define __GX_AVTIMER_H__

extern void *avtimer_create (void);
extern void  avtimer_destroy(void *handle);
extern void  avtimer_update (void *handle);
extern void  avtimer_pause  (void *handle);
extern void  avtimer_resume (void *handle);
extern unsigned int avtimer_getms(void *handle);

#endif
