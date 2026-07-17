/*****************************************************************************
* 			  CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	xxx.c
* Author    : 	xxx
* Project   :	GoXceed -S
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2009.12.04	            xxx         creation
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __APP_SEND_MSG_H__
#define __APP_SEND_MSG_H__

/* Includes --------------------------------------------------------------- */

/* Cooperation with C and C++ */
#ifdef __cplusplus
extern "C" {
#endif

#include "gxcore.h"
#include "gxmsg.h"
#include "app_msg.h"
/* Exported Constants ----------------------------------------------------- */


extern status_t app_send_msg_exec(uint32_t msg_id,void* params);
extern status_t app_send_msg_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __GX_BOOK_H__ */
/*@}*/
/* End of file -------------------------------------------------------------*/


