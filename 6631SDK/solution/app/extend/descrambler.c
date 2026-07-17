#include "include/descrambler.h"
#include "module/app_descrambler_api.h"


/**
* @brief      open a descrambler and return the handle
* @param []	DemuxID :demux id .0~1 is valid
* @return      handle == 0 failure;or sucess
*/
int Descrmb_Open(unsigned int  DemuxID)
{
    return GxDesc_Open(DemuxID,APP_DESC_CSA2);
}

/**
* @brief        close a descrambler release resourse
* @param []	    Descrambler :handle of descrambler,get via Descrmb_Open();
* @return       int  >= 0, sucess.<0 failure
*/
int Descrmb_Close(int Descrambler)
{
    return GxDesc_Close(Descrambler);
}

/**
* @brief    tell descrambler that which pid channel need to bind. the pid will used to find the
		    id of slot,then descrambler can descramble the stream.
* @param []	Descrambler :handle of descrambler,get via Descrmb_Open();
* @param []	StreamPID :The pid of stream;
* @return   int  >= 0, sucess.<0 failure
*/
int Descrmb_SetStreamPID(int Descrambler, unsigned short StreamPID)
{
    return GxDesc_SetStreamPID(Descrambler,StreamPID);
}

int Descrmb_SetCW_Config(int Descrambler,int DecryptFlag,uint8_t softkey_flag)
{
    return GxDesc_SetConfig(Descrambler,DecryptFlag);
}

int Descrmb_SetOddKey(int Descrambler, const unsigned char* OddKey, unsigned int KeyLen, int DecryptFlag)
{
    GxDesc_SetConfig(Descrambler,DecryptFlag);
    return GxDesc_SetOddKey(Descrambler,OddKey,KeyLen);
}

int Descrmb_SetEvenKey( int Descrambler, const unsigned char*  EvenKey, unsigned int KeyLen, int DecryptFlag)
{
    GxDesc_SetConfig(Descrambler,DecryptFlag);
    return GxDesc_SetEvenKey(Descrambler,EvenKey,KeyLen);
}



/**
* @brief      Send cw (odd key & even key) to descrambler.
* @param []	Descrambler :handle of descrambler,get via Descrmb_Open();
* @param [in] OddKey :buffer of odd key
* @param [in] EvenKey :buffer of even key
* @param [] KeyLen :length of key
* @param [] DecryptFlag: (CAS248bit MODE),1(CAS264bit MODE),3(AES_ECB),5(AES_CBC),6(DES_ECB),7(TDES_ECB),10(MUTL12)
* @return      int  >= 0, sucess.<0 failure
* @note		we sugest to use this interface, user can save even key or odd key ,and send them to
			descrambler together.
*/


int Descrmb_SetCW(int Descrambler,
                                const unsigned char* OddKey,
                                const unsigned char* EvenKey,
                                unsigned int KeyLen,
                                int DecryptFlag)
{
    GxDesc_SetConfig(Descrambler,DecryptFlag);
    return GxDesc_SetCW(Descrambler,OddKey,EvenKey,KeyLen);
}


int Descrmb_GetCW(int Descrambler,
                                unsigned char* OddKey,
                                unsigned char* EvenKey)
{
	return -1;
}


int Descrmb_MTCSet(int Descrambler,
                                enum mtc_api_desc_type   des_type,
                                /*struct mtc_api_input_s*/void *keys,
                                enum mtc_arith_type arith_type,
                                enum mtc_arith_mode arith_mode)
{
   return -1;
}
