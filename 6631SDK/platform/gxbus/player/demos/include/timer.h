#ifndef __TIMER_H__
#define __TIMER_H__

void* Timer_Create (void);
void* Timer_Clear  (void* handle);
int   Timer_GetMs  (void* handle);
void  Timer_Destroy(void* handle);

#endif
