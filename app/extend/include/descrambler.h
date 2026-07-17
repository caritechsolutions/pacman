/**
 * @file descrambler.h
 * @author lixb
 * @brief CAS系统解挠抽象模块
 * @addtogroup ca_module
 */
 /*@{*/
#ifndef __DESCRAMBLER_H__
#include <gxtype.h>
#include "gxcore.h"
#include "devapi/gxmtc_api.h"
__BEGIN_DECLS
#ifndef IN
#define IN
#endif
#ifndef OUT
#define OUT
#endif

typedef struct _DescramblerAdvancedSecurityClass{
    struct mtc_api_input_s keys;
    enum mtc_api_algorithm algorithm;
    enum mtc_api_sub_mode sub_mode;
}DescramblerAdvancedSecurityClass;

/**
* @brief    open a descrambler and return the handle
* @param []	DemuxID :demux id .0~1 is valid
* @return   handle == E_INVALID_HANDLE failure;or sucess
*/
int Descrmb_Open(unsigned int  DemuxID);

/**
* @brief    close a descrambler release resourse
* @param []	Descrambler :handle of descrambler,get via Descrmb_Open();
* @return   int  >= 0, sucess.<0 failure
*/
int Descrmb_Close(int Descrambler);

/**
* @brief    tell descrambler that which pid channel need to bind. the pid will used to find the
		    id of slot,then descrambler can descramble the stream.
* @param []	Descrambler :handle of descrambler,get via Descrmb_Open();
* @param []	StreamPID :The pid of stream;
* @return   int  >= 0, sucess.<0 failure
*/
int Descrmb_SetStreamPID(int Descrambler, unsigned short  StreamPID);

/**
* @brief        Send odd key to descrambler.
* @param []	    Descrambler :handle of descrambler,get via Descrmb_Open();
* @param [in]   OddKey :buffer of odd key
* @param []     KeyLen :length of key
 * @return      int  >= 0, sucess.<0 failure
*/
int Descrmb_SetOddKey(  int Descrambler, IN const unsigned char* OddKey, unsigned int KeyLen, int DecryptFlag);

/**
* @brief        Send even key to descrambler.
* @param []	    Descrambler :handle of descrambler,get via Descrmb_Open();
* @param [in]   EvenKey :buffer of even key
* @param []     KeyLen :length of key
 * @return      int  >= 0, sucess.<0 failure
*/
int Descrmb_SetEvenKey( int Descrambler, IN const unsigned char*  EvenKey, unsigned int KeyLen, int DecryptFlag);


/**
* @brief        Send cw (odd key & even key) to descrambler.
* @param []	    Descrambler :handle of descrambler,get via Descrmb_Open();
* @param [in]   OddKey :buffer of odd key
* @param [in]   EvenKey :buffer of even key
* @param []     KeyLen :length of key
* @return       int  >= 0, sucess.<0 failure
* @note		we sugest to use this interface, user can save even key or odd key ,and send them to
			descrambler together.
*/
int Descrmb_SetCW(int Descrambler,
                                const unsigned char* OddKey,
                                const unsigned char* EvenKey,
                                unsigned int KeyLen,
                                int DecryptFlag);

int Descrmb_GetCW(int Descrambler,
                                unsigned char* OddKey,
                                unsigned char* EvenKey);

// for mtc control, temp for 3211
int Descrmb_MTCSet(int Descrambler,
                                enum mtc_api_desc_type   des_type,
                                /*struct mtc_api_input_s*/void *keys,
                                enum mtc_arith_type arith_type,
                                enum mtc_arith_mode arith_mode);

__END_DECLS

#endif
/*@}*/
