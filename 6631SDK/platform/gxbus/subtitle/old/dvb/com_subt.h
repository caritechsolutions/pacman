/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2008, All right reserved
******************************************************************************

******************************************************************************
* File Name :	com_subt.h
* Author    : 	brucechow
* Project   :	GX6102 
* Type      :	
******************************************************************************
* Purpose   :	
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2008.06.30	      brucechow	         creation
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __COM_SUBT_H__
#define __COM_SUBT_H__

/* Includes --------------------------------------------------------------- */
#include "com_subt_sub.h"


/* Cooperation with C and C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */

/* Error Constants */

/* Exported Types --------------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

//YCbCrCount定义每像素的位数，当为13时颜色失真比较明显，14和15从肉眼看区别几乎看不出
#define YCbCrCount		0



/* Exported Macros -------------------------------------------------------- */

/* Exported Messages ------------------------------------------------------ */

/* Exported Functions ----------------------------------------------------- */
//extern AppErr_t subt_pic_layer_disable(void);

AppErr_t com_show_cc(u16 , u8 , u16 );

AppErr_t com_close_cc(void);


void com_subt_dec_start(u16 wPid,u16 wCompositionPageId,u16 wAncillaryPageId);
void com_subt_dec_stop(void);

void com_get_cur_subt_pid(u16 *pPID,U16 *pPage,U16 *pAcillary);

void com_subt_get_show_flag(u8 *pFlag);

#ifdef __cplusplus
}
#endif

#endif /* __COM_SUBT_H__ */

/* End of file -------------------------------------------------------------*/





































