#ifndef _RECV_QAM_API_H_
#define _RECV_QAM_API_H_

S32 ipqam_get_regionid( void );
S32 ipqam_recv_start( unsigned int frequency, unsigned int symborate,unsigned int qam,unsigned int pid);
S32 ipqam_recv_close( void );

#endif
