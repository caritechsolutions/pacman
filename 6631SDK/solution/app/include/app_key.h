#ifndef __APP_KEY_H__
#define __APP_KEY_H__

#include "gui_core.h"

//1 dvbs key
#define STBK_1              GUIK_1
#define STBK_2              GUIK_2
#define STBK_3              GUIK_3
#define STBK_4              GUIK_4
#define STBK_5              GUIK_5
#define STBK_6              GUIK_6
#define STBK_7              GUIK_7
#define STBK_8              GUIK_8
#define STBK_9              GUIK_9
#define STBK_0              GUIK_0
#define STBK_MENU           GUIK_M
#define STBK_EXIT           GUIK_ESCAPE
#define STBK_OK             GUIK_RETURN
#define STBK_UP             GUIK_UP
#define STBK_DOWN           GUIK_DOWN
#define STBK_LEFT           GUIK_LEFT
#define STBK_RIGHT          GUIK_RIGHT
#define STBK_MUTE           GUIK_U
#define STBK_TV_RADIO       GUIK_V
#define STBK_HALT           GUIK_H
#define STBK_RECALL         GUIK_R
#define STBK_SAT            GUIK_S
#define STBK_AUDIO          GUIK_A
#define STBK_QR         GUIK_LSHIFT
#define STBK_PAUSE			GUIK_PAUSE
#define STBK_ZOOM           GUIK_Z
#define STBK_PAUSE_STB      GUIK_G
#define STBK_INFO           GUIK_I
#define STBK_SUBT           GUIK_B
#define STBK_MUL_PIC        GUIK_C
#define STBK_TTX            GUIK_T
#define STBK_EPG            GUIK_E
#define STBK_FAV            GUIK_F
#define STBK_RED            GUIK_RED
#define STBK_BLUE           GUIK_BLUE
#define STBK_GREEN          GUIK_GREEN
#define STBK_YELLOW         GUIK_YELLOW
#define STBK_REC_START      GUIK_PRINT
#define STBK_REC_STOP       GUIK_SCROLLOCK
#define STBK_PAGE_UP        GUIK_PAGE_UP
#define STBK_PAGE_DOWN      GUIK_PAGE_DOWN
#define STBK_F1             GUIK_F1
#define STBK_CH_UP          GUIK_F2
#define STBK_CH_DOWN        GUIK_F3
#define STBK_REPEAT			GUIK_F4
#define STBK_TIMER			GUIK_F5
#define STBK_ASPECT			GUIK_F6
#define STBK_CH_EDIT		GUIK_F7
#define STBK_SLEEP	        GUIK_F8
#define STBK_VOLUP			GUIK_F9
#define STBK_VOLDOWN		GUIK_F10
#define STBK_TV				GUIK_F11
#define STBK_RADIO			GUIK_F12
#define STBK_ARROW_LEFT		GUIK_INSERT // can not use the key define "GUIK_X"
#define	STBK_ARROW_RIGHT	GUIK_Y
#define STBK_FIND           GUIK_W
#define STBK_DISK_INFO		GUIK_D
#define STBK_SYS_INFO		GUIK_J
#define STBK_PAUSE_PLAY	    GUIK_K
#define STBK_TMS			GUIK_O
#define STBK_PN				GUIK_Q
#define STBK_MEDIA			GUIK_HOME

#define STBK_LAST			GUIK_L
#define STBK_NEXT			GUIK_N
#define STBK_FB				GUIK_MINUS
#define STBK_FF			    GUIK_PLUS
#define STBK_PLAY			GUIK_P

#define VK_BOOK_TRIGGER	    GUIK_BACKSLASH //for book trigger
#define VK_DLNA_TRIGGER	    GUIK_QUOTEDBL //for dlna trigger

//1 media player key
#define	APPK_MUTE			STBK_MUTE

#define	APPK_SUBT			STBK_SUBT
#define	APPK_RATIO			STBK_BLUE//STBK_ASPECT
#define	APPK_AUDIO			STBK_AUDIO
#define	APPK_SEEK			GUIK_END


#define	APPK_PREVIOUS		STBK_LAST
#define	APPK_NEXT			STBK_NEXT
#define	APPK_REW			STBK_FB
#define	APPK_FF	            STBK_FF

#define	APPK_STOP			STBK_REC_STOP
#define	APPK_PLAY			STBK_PLAY
#define APPK_PAUSE			STBK_PAUSE
#define APPK_PAUSE_PLAY		STBK_PAUSE_PLAY

#define	APPK_BACK			STBK_EXIT//
#define	APPK_SET			STBK_RED//
#define	APPK_VOLUP			STBK_VOLUP//
#define	APPK_VOLDOWN		STBK_VOLDOWN//
#define APPK_GREEN			STBK_GREEN
#define APPK_YELLOW			STBK_YELLOW
#define	APPK_MENU			STBK_MENU//

#define	APPK_LEFT			STBK_LEFT//
#define	APPK_RIGHT			STBK_RIGHT//
#define	APPK_UP				STBK_UP//
#define	APPK_DOWN			STBK_DOWN//
#define	APPK_INFO			STBK_INFO
#define	APPK_OK				STBK_OK //
#define APPK_PAGE_UP        STBK_PAGE_UP
#define APPK_PAGE_DOWN      STBK_PAGE_DOWN
#define	APPK_REPEAT			STBK_REPEAT//

#define	APPK_0				STBK_0
#define	APPK_1				STBK_1
#define	APPK_2				STBK_2
#define	APPK_3				STBK_3
#define	APPK_4				STBK_4
#define	APPK_5				STBK_5
#define	APPK_6				STBK_6
#define	APPK_7				STBK_7
#define	APPK_8				STBK_8
#define	APPK_9				STBK_9

//Function implementation from gxbus/gui
#define  find_virtualkey_ex  find_virtualkeys

#endif /* __APP_KEY_H__ */

