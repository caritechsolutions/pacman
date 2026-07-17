#ifndef __GUI_DEBUG_H__
#define __GUI_DEBUG_H__

#include <stdio.h>
#include <stdlib.h>
#include "gxcore.h"

__BEGIN_DECLS

//#define GUI_DEBUG
//#define GUI_LESS_PRINT

#ifdef GXDBG
#ifndef GUI_Debug_Print
#ifndef GUI_LESS_PRINT
#define GUI_Debug_Print(...) {int count = 0; \
			      do{ \
				gxlogd("[GUI]"); \
				gxlogd(__VA_ARGS__); \
			        count++; \
			      }while(count < 1); \
			    }
#else
#define GUI_Debug_Print(...) while(0);
#endif	/*GUI_LESS_PRINT*/
#endif  /*GUI_Debug_Print*/
#else	/*GXDBG else*/
#ifndef GUI_Debug_Print
#define GUI_Debug_Print(...) while(0);
#endif /*GUI_Debug_Print*/
#endif /*GXDBG*/

#ifdef GUI_DEBUG

#ifndef GUI_Printf
#define GUI_Printf(...)	gxlogd(__VA_ARGS__)
#endif

#ifndef GUI_Error
#define GUI_Error(...)	{gxlogd("[%s: %d] %s()\n", __FILE__, __LINE__, __FUNCTION__);\
	gxlogd(__VA_ARGS__);}
#endif

#ifndef GUI_ASSERT
#define GUI_ASSERT(exp)  if(!(exp)){gxlogd("\n#ASSERT# %s: %d	%s\n", __FILE__, __LINE__, #exp); while(1);}
#endif

#ifndef GUI_Printf_Blue
#define GUI_Printf_Blue(...)	{gxlogd("\033[036m");\
	gxlogd(__VA_ARGS__);\
	gxlogd("\033[0m"); fflush(stdout);}
#endif

#ifndef GUI_Printf_Yellow
#define GUI_Printf_Yellow(...)	{gxlogd("\033[033m");\
	gxlogd(__VA_ARGS__);\
	gxlogd("\033[0m");fflush(stdout);}
#endif


#else

#define GUI_Printf(...)	do{}while(0);
#define GUI_Error(...)	do{}while(0);
#define GUI_ASSERT(exp)	do{}while(0);
#define GUI_Printf_Blue(...)  do{}while(0);
#define GUI_Printf_Yellow(...) do{}while(0);

#endif

__END_DECLS

#endif

