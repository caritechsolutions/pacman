/**
 * @file gxca_descrambler.h
 * @author lixb
 * @brief CAS系统解挠抽象模块
 * @addtogroup ca_module
 */
 /*@{*/
#ifndef __GX_CA_DESCRAMBLER_H__
#define __GX_CA_DESCRAMBLER_H__


#include <gxtype.h>
#include "gxcore.h"

__BEGIN_DECLS

/**
* @brief    open a descrambler and return the handle
* @param []	DemuxID :demux id .0~1 is valid
* @return   handle == E_INVALID_HANDLE failure;or sucess
*/
handle_t GxDescrmb_Open(uint32_t  DemuxID);

/**
* @brief    close a descrambler release resourse
* @param []	Descrambler :handle of descrambler,get via GxDescrmb_Open();
* @return   int  >= 0, sucess.<0 failure
*/
int32_t GxDescrmb_Close(handle_t Descrambler);

/**
* @brief    tell descrambler that which pid channel need to bind. the pid will used to find the
		    id of slot,then descrambler can descramble the stream.
* @param []	Descrambler :handle of descrambler,get via GxDescrmb_Open();
* @param []	StreamPID :The pid of stream;
* @return   int  >= 0, sucess.<0 failure
*/
int32_t GxDescrmb_SetStreamPID(handle_t Descrambler, uint16_t  StreamPID);

/**
* @brief        Send odd key to descrambler.
* @param []	    Descrambler :handle of descrambler,get via GxDescrmb_Open();
* @param [in]   OddKey :buffer of odd key
* @param []     KeyLen :length of key
 * @return      int  >= 0, sucess.<0 failure
*/
int32_t GxDescrmb_SetOddKey(  handle_t Descrambler, const uint8_t* OddKey, size_t KeyLen );

/**
* @brief        Send even key to descrambler.
* @param []	    Descrambler :handle of descrambler,get via GxDescrmb_Open();
* @param [in]   EvenKey :buffer of even key
* @param []     KeyLen :length of key
 * @return      int  >= 0, sucess.<0 failure
*/
int32_t GxDescrmb_SetEvenKey( handle_t Descrambler, const uint8_t*  EvenKey, size_t KeyLen );


/**
* @brief        Send cw (odd key & even key) to descrambler.
* @param []	    Descrambler :handle of descrambler,get via GxDescrmb_Open();
* @param [in]   OddKey :buffer of odd key
* @param [in]   EvenKey :buffer of even key
* @param []     KeyLen :length of key
* @return       int  >= 0, sucess.<0 failure
* @note		we sugest to use this interface, user can save even key or odd key ,and send them to
			descrambler together.
*/
int32_t GxDescrmb_SetCW(handle_t Descrambler,
                                const uint8_t* OddKey,
                                const uint8_t* EvenKey,
                                size_t KeyLen );

int32_t GxDescrmb_GetCW(handle_t Descrambler,
                                uint8_t* OddKey,
                                uint8_t* EvenKey);
/**
* @brief        Send cw (odd key & even key) to descrambler.It is provide to mtc high level encryption system specially.
* @param []	    Descrambler :handle of descrambler,get via GxDescrmb_Open();
* @param [in]   EcwEven :buffer of even key
* @param [in]   EcwOdd :buffer of odd key
* @param [in]   Eck :buffer of eck
* @param []     EcwEvenLen :length of key
* @param []     EcwOddLen :length of key
* @param []     EckLen :length of key
* @param []     arith_mode :mode
* @param []     arith_type :type
* @return       int  >= 0, sucess.<0 failure
* @note		    we sugest to use this interface, user can save even key or odd key ,and send them to
			    descrambler together.
*/
int32_t GxDescrmb_SetECW(handle_t Descrambler,
								uint16_t pid,
                                const uint8_t* EcwEven,
                                size_t EcwEvenLen,
                                const uint8_t* EcwOdd,
                                size_t EcwOddLen,
                                const uint8_t* Eck,
                                size_t EckLen,
                                enum mtc_arith_type arith_type,
                                enum mtc_arith_mode arith_mode);

/**
* @brief        Send  even key to descrambler.It is provide to mtc high level encryption system specially.
* @param []	    Descrambler :handle of descrambler,get via GxDescrmb_Open();
* @param [in]   EcwEven :buffer of even key
* @param [in]   Eck :buffer of eck
* @param []     EcwEvenLen :length of key
* @param []     EckLen :length of key
* @param []     arith_mode :mode
* @param []     arith_type :type
* @return       int  >= 0, sucess.<0 failure
*/
int32_t GxDescrmb_SetEvenECW(handle_t Descrambler,
                                const uint8_t* EcwEven,
                                size_t EcwEvenLen,
                                const uint8_t* Eck,
                                size_t EckLen,
                                enum mtc_arith_type arith_type,
                                enum mtc_arith_mode arith_mode);

/**
* @brief        Send  even key to descrambler.It is provide to mtc high level encryption system specially.
* @param []	    Descrambler :handle of descrambler,get via GxDescrmb_Open();
* @param [in]   EcwEven :buffer of odd key
* @param [in]   Eck :buffer of eck
* @param []     EcwEvenLen :length of key
* @param []     EckLen :length of key
* @param []     arith_mode :mode
* @param []     arith_type :type
* @return       int  >= 0, sucess.<0 failure
*/
int32_t GxDescrmb_SetOddECW(handle_t Descrambler,
                                const uint8_t* EcwOdd,
                                size_t EcwOddLen,
                                const uint8_t* Eck,
                                size_t EckLen,
                                enum mtc_arith_type arith_type,
                                enum mtc_arith_mode arith_mode);


/** 以下接口用于统一设置多个解扰器为同一个CW，基于上述接口实现
 *  主要用于博尚方案。以下接口另成一套，不建议于上述基本接口同时使用*/
int32_t GxDescrmb_AddPid(uint16_t pid);

int32_t GxDescrmb_DelPid(uint16_t pid);

int32_t GxDescrmb_SetAllPidECW( const uint8_t* EcwEven,
                                size_t EcwEvenLen,
                                const uint8_t* EcwOdd,
                                size_t EcwOddLen,
                                const uint8_t* Eck,
                                size_t EckLen,
                                enum mtc_arith_type arith_type,
                                enum mtc_arith_mode arith_mode);

int32_t GxDescrmb_Handle2Id(handle_t des);

// new api
#define MAX_KLM_LEVE   5
typedef enum _DescKLMAlgEnum{
    APP_DESC_KLM_3DES,
    APP_DESC_KLM_AES,
}DescKLMAlgEnum;

typedef enum _DescAlgEnum{
    APP_DESC_CSA2,
    APP_DESC_CSA2_CONFORMANCE,
    APP_DESC_CSA3,
    APP_DESC_AES_ECB,
    APP_DESC_IDSA,
    APP_DESC_AES_CBC,
    APP_DESC_DES,
    APP_DESC_TDES,
    APP_DESC_SM4_ECB,
    APP_DESC_SM4_CBC,
    APP_DESC_MULTI2,
}DescAlgEnum;

typedef struct _DescKLMCUnitlass{
    DescKLMAlgEnum alg;
    unsigned char  ek[16];
    unsigned char  ek_bytes;
}DescKLMCUnitlass;

typedef struct _DescKLMClass{
    DescKLMCUnitlass klm_unit[MAX_KLM_LEVE];
    unsigned char unit_count;
}DescKLMClass;
// m2m
typedef struct _DescM2MDataClass{
    unsigned char* src;
    unsigned int in_bytes;
    unsigned char* dst;
    unsigned int out_bytes;
}DescM2MDataClass;

typedef enum _DescM2MAlgEnum{
   APP_M2M_DES,
   APP_M2M_3DES,
   APP_M2M_AES128,
   APP_M2M_AES192,
   APP_M2M_AES256,
}DescM2MAlgEnum;

typedef enum _DescM2MOptEnum{
   APP_M2M_ECB,
   APP_M2M_CBC,
   APP_M2M_CFB,
   APP_M2M_OFB,
   APP_M2M_CTR,
}DescM2MOptEnum;

typedef enum _DescM2MKeyEnum{
   APP_M2M_ACPU_SOFT,
   APP_M2M_OTP_SOFT,
}DescM2MKeyEnum;

typedef struct _DescM2MKeyClass{
    unsigned char iv[16];
    unsigned char soft_key[32];
}DescM2MKeyClass;

typedef struct _DescM2MAlgClass{
    DescM2MAlgEnum alg;
    DescM2MOptEnum opt;
    DescM2MKeyEnum key_type;
    DescM2MKeyClass keys;
}DescM2MAlgClass;

typedef enum _DescFirmwareAreaEnum{
   APP_DESC_FW_SRAM,
   APP_DESC_FW_DRAM,
}DescFirmwareAreaEnum;

int GxDesc_Open(unsigned int demux_id, DescAlgEnum stream_alg);

#define GxDesc_Close(handle) GxDescrmb_Close((handle))

#define GxDesc_SetStreamPID(handle, pid) GxDescrmb_SetStreamPID((handle), (pid))

int GxDesc_SetConfig(handle_t Descrambler, DescAlgEnum stream_alg);

int GxDesc_SetOddKey(handle_t Descrambler, const unsigned char* OddKey, unsigned char KeyLen);

int GxDesc_SetEvenKey(int Descrambler, const unsigned char*  EvenKey, unsigned char KeyLen );

int GxDesc_SetCW(int Descrambler,
                                const unsigned char* OddKey,
                                const unsigned char* EvenKey,
                                unsigned char KeyLen );

int GxDesc_LoadFirmware(unsigned char *fw, unsigned int fw_len, DescFirmwareAreaEnum type);

// keyladder
int GxDesc_SetECW(int Descrambler,
                                const unsigned char* OddKey,
                                const unsigned char* EvenKey,
                                unsigned char KeyLen,
                                DescKLMAlgEnum ecw_alg,
                                unsigned char root_index,
                                DescKLMClass klm_info);

int GxDesc_SetECWOdd(int Descrambler,
                                const unsigned char* OddKey,
                                unsigned char KeyLen,
                                DescKLMAlgEnum ecw_alg,
                                unsigned char root_index,
                                DescKLMClass klm_info);

int GxDesc_SetECWEven(int Descrambler,
                                const unsigned char* EvenKey,
                                unsigned char KeyLen,
                                DescKLMAlgEnum ecw_alg,
                                unsigned char root_index,
                                DescKLMClass klm_info);

// m2m
int GxM2M_Decrypt(DescM2MDataClass data, DescM2MAlgClass alg);

int GxM2M_Encrypt(DescM2MDataClass data, DescM2MAlgClass alg);


__END_DECLS

#endif
/*@}*/
