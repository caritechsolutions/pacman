#ifndef _DSM_API_H_
#define _DSM_API_H_

#define NPT_NOW  			0x80000000
#define NPT_END 			0x7FFFFFFF

#define DSM_NODATA  	-2
#define DSM_TIMEOUT	-3
#define DSM_SOCK_ERR 	-4
#define DSM_SEND_ERR 	-5

S32 dsm_session_init( U16 sport,U32 saddr,U32 sgate_way, U16 groupid ,U8*assertid,U32 a_len,U32 startnpt);
S32 dsm_cmd_pause( U32 pause_time );
S32 dsm_cmd_resume( U32 start_time );
S32 dsm_cmd_seek( U32 position );
/***********************************************************
* dsm_cmd_scale
*parameters:
*scale:		the play speed,you can select -33,-18,-9,-3,-1/2,
			1/2,3,9,18,33;
			be careful please:if you want select -1/2 the scale 
			must is -100,if you select 1/2,the scale must is 100
************************************************************/
S32 dsm_cmd_scale( S16 scale );
/**********************************************************
*dsm_cmd_keep_live
*description: 	keep stream alive with server
*
**********************************************************/
S32 dsm_cmd_keep_live( void );
/**********************************************************
*dsm_cmd_status
*description: 	get the server status
*return:		>0,current npt in server mode
*			<=0,get status error
**********************************************************/
S32 dsm_cmd_status( void );
/**********************************************************
*dsm_session_close
*description: 	close the session
*
**********************************************************/
S32 dsm_session_close(void );
/**********************************************************
*dsm_loop_done_event
*description: 	loop the server event
*return:		1,received the server done(exit)event
*			0,status is ok,no done expend
**********************************************************/
S32 dsm_loop_done_event(void);
/**********************************************************
*dsm_get_heart_beat
*description: 	get the heartbeat time interval unit is sec
*
**********************************************************/
U32 dsm_get_heart_beat(void);
U32 dsm_get_frequency( void );
U16 dsm_get_program_number( void );

#endif

