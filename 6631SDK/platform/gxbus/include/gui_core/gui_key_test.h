#ifndef __GUI_KEY_TEST_H__
#define __GUI_KEY_TEST_H__

#include "gxcore.h"

__BEGIN_DECLS

int Gui_WriteKeyBuf(unsigned int key);
unsigned int Gui_ReadKeyBuf(void);
void Gui_IrrKeyMute(int flag);

void Gui_SendLoopKey(unsigned int *keys);
int Gui_ExitLoopKey(void);

__END_DECLS

#endif

