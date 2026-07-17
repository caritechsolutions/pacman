#ifndef __HD_SYSTEM_H__
#define __HD_SYSTEM_H__

#include "gui_event.h"
#include "gxcore.h"

__BEGIN_DECLS

unsigned char hd_poll_event(GUI_Event *data);
unsigned int hd_get_tick(void);
void hd_delay(unsigned int ms);

__END_DECLS

#endif

