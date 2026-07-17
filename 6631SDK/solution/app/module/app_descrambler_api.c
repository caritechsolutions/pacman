#include <string.h>
#include "module/app_log.h"
#include "module/app_descrambler_api.h"
#include "dvbhal/gxdescrambler_hal.h"
#include "devapi/gxmtc_api.h"

#define GXDES_MAX_DEMUX_SUPPORT  (2)

#ifndef E_INVALID_HANDLE
#define E_INVALID_HANDLE        (0)
#endif

#ifndef ASSERT
#define ASSERT(a) do{if(!(a)){app_log_error("Assert failure %s in %s:%d\n",#a,__func__,__LINE__);return -1;}}while(0)
#endif

#define MAX_SLOT     (64)
#define INVALID_PID  (0xffff)

struct app_descrambler
{
    handle_t    desc_handle;
    uint16_t    stream_pid;/*需要解扰的包pid值,用于对应slotid*/
    DescAlgEnum stream_alg;
};

typedef struct
{
    struct app_descrambler  desc_array[MAX_SLOT];
    handle_t                mutex;
}AppDescCtrl;

static AppDescCtrl thiz_desc_ctrl = {
    .mutex = 0,
};

static int  _GetKLMType(void);

static void _ctrl_init(void)
{
    int32_t i = 0;

    for(i = 0; i < MAX_SLOT; i++)
    {
        thiz_desc_ctrl.desc_array[i].desc_handle = E_INVALID_HANDLE;
        thiz_desc_ctrl.desc_array[i].stream_pid = INVALID_PID;
    }

    return;
}

static int32_t _add_desc_handle(handle_t handle)
{
    int32_t i = 0;

    for(i = 0; i < MAX_SLOT; i++)
    {
        if(E_INVALID_HANDLE == thiz_desc_ctrl.desc_array[i].desc_handle)
        {
            thiz_desc_ctrl.desc_array[i].desc_handle = handle;
            return 0;
        }
    }

    return -1;
}

static int32_t _del_desc_handle(handle_t handle)
{
    int32_t i = 0;

    for(i = 0; i < MAX_SLOT; i++)
    {
        if(handle == thiz_desc_ctrl.desc_array[i].desc_handle)
        {
            thiz_desc_ctrl.desc_array[i].desc_handle = E_INVALID_HANDLE;
            thiz_desc_ctrl.desc_array[i].stream_pid = INVALID_PID;
            return 0;
        }
    }

    return -1;
}

static int32_t _add_stream_pid(handle_t handle, uint16_t stream_pid)
{
    int32_t i = 0;

    for(i = 0; i < MAX_SLOT; i++)
    {
        if(handle == thiz_desc_ctrl.desc_array[i].desc_handle)
        {
            thiz_desc_ctrl.desc_array[i].stream_pid = stream_pid;
            return 0;
        }
    }

    return -1;
}

static uint16_t _get_stream_pid(handle_t handle)
{
    int32_t i = 0;

    for(i = 0; i < MAX_SLOT; i++)
    {
        if(handle == thiz_desc_ctrl.desc_array[i].desc_handle)
            return thiz_desc_ctrl.desc_array[i].stream_pid;
    }

    return INVALID_PID;
}

static int32_t _add_stream_alg(handle_t handle, DescAlgEnum stream_alg)
{
    int32_t i = 0;

    for(i = 0; i < MAX_SLOT; i++)
    {
        if(handle == thiz_desc_ctrl.desc_array[i].desc_handle)
        {
            thiz_desc_ctrl.desc_array[i].stream_alg = stream_alg;
            return 0;
        }
    }

    return -1;
}

static uint16_t _get_stream_alg(handle_t handle)
{
    int32_t i = 0;

    for(i = 0; i < MAX_SLOT; i++)
    {
        if(handle == thiz_desc_ctrl.desc_array[i].desc_handle)
            return thiz_desc_ctrl.desc_array[i].stream_alg;
    }

    return INVALID_PID;
}

/**************************************old api*************************************
 ****************the following old apis work for gx6605s and gx3211****************/

/**
* @brief      open a descrambler and return the handle
* @param [] DemuxID :demux id .0~1 is valid
* @return      handle == E_INVALID_HANDLE failure;or sucess
*/
handle_t GxDescrmb_Open(uint32_t  DemuxID)
{
    static bool init = false;
    handle_t ret_handle = E_INVALID_HANDLE;

    if(false == init)
    {
        GxDescInitParam param = {0};

        param.klm = GXDESC_KLM_NONE | _GetKLMType();
        if(GxDescInit(&param) != 0)
            return E_INVALID_HANDLE;

        if(0 == thiz_desc_ctrl.mutex)
            GxCore_MutexCreate(&thiz_desc_ctrl.mutex);

        _ctrl_init();
        init = true;
    }

    ret_handle = GxDescOpen(GXDESC_MOD_TS, DemuxID, GXDESC_ALG_CAS_CSA2);
    if(ret_handle != -1 && ret_handle != -2)
    {
        _add_desc_handle(ret_handle);
        GxDescConfig(ret_handle, GXDESC_ALG_CAS_CSA2, GXDESC_KLM_NONE, 0);
    }
    else
    {
        ret_handle = E_INVALID_HANDLE;
    }

    return ret_handle;
}

/**
* @brief        close a descrambler release resourse
* @param []     Descrambler :handle of descrambler,get via GxDescrmb_Open();
* @return       int32_t  >= 0, sucess.<0 failure
*/
int32_t GxDescrmb_Close(handle_t Descrambler)
{
    GxDescClose(Descrambler);
    _del_desc_handle(Descrambler);
    return 0;
}

/**
* @brief    tell descrambler that which pid channel need to bind. the pid will used to find the
            id of slot,then descrambler can descramble the stream.
* @param [] Descrambler :handle of descrambler,get via GxDescrmb_Open();
* @param [] StreamPID :The pid of stream;
* @return   int32_t  >= 0, sucess.<0 failure
*/
int32_t GxDescrmb_SetStreamPID(handle_t Descrambler, uint16_t  StreamPID)
{
    ASSERT(StreamPID <= 0x1FFF);

    GxCore_MutexLock(thiz_desc_ctrl.mutex);
    _add_stream_pid(Descrambler, StreamPID);
    GxCore_MutexUnlock(thiz_desc_ctrl.mutex);
    return 0;
}



/**
* @brief      Send odd key to descrambler.
* @param [] Descrambler :handle of descrambler,get via GxDescrmb_Open();
* @param [in] OddKey :buffer of odd key
* @param [] KeyLen :length of key
 * @return      int32_t  >= 0, sucess.<0 failure
*/
int32_t GxDescrmb_SetOddKey(handle_t Descrambler, const uint8_t* OddKey, size_t KeyLen)
{
    uint16_t      stream_pid  = INVALID_PID;
    GxDescCWParam params;

    ASSERT(Descrambler != E_INVALID_HANDLE);
    ASSERT(OddKey != NULL);

    memset(&params, 0, sizeof(GxDescCWParam));

    GxCore_MutexLock(thiz_desc_ctrl.mutex);
    stream_pid = _get_stream_pid(Descrambler);
    GxCore_MutexUnlock(thiz_desc_ctrl.mutex);

    if(INVALID_PID == stream_pid)
        return -1;

    memcpy(params.odd_ECW, OddKey, KeyLen);
    params.ECW_len = KeyLen;

    //普安不会用到klm,所以算法随便填了一个
    return GxDescSetOddCw(Descrambler, stream_pid, GXDESC_KLM_ALG_3DES, &params);
}

/**
* @brief      Send even key to descrambler.
* @param [] Descrambler :handle of descrambler,get via GxDescrmb_Open();
* @param [in] EvenKey :buffer of even key
* @param [] KeyLen :length of key
 * @return      int32_t  >= 0, sucess.<0 failure
*/
int32_t GxDescrmb_SetEvenKey( handle_t Descrambler, const uint8_t*  EvenKey, size_t KeyLen )
{
    uint16_t      stream_pid  = INVALID_PID;
    GxDescCWParam params;

    ASSERT(Descrambler != E_INVALID_HANDLE);
    ASSERT(EvenKey != NULL);

    memset(&params, 0, sizeof(GxDescCWParam));

    GxCore_MutexLock(thiz_desc_ctrl.mutex);
    stream_pid = _get_stream_pid(Descrambler);
    GxCore_MutexUnlock(thiz_desc_ctrl.mutex);

    if(INVALID_PID == stream_pid)
        return -1;

    memcpy(params.even_ECW, EvenKey, KeyLen);
    params.ECW_len = KeyLen;

    //普安不会用到klm,所以算法随便填了一个
    return GxDescSetEvenCw(Descrambler, stream_pid, GXDESC_KLM_ALG_3DES, &params);
}


/**
* @brief      Send cw (odd key & even key) to descrambler.
* @param [] Descrambler :handle of descrambler,get via GxDescrmb_Open();
* @param [in] OddKey :buffer of odd key
* @param [in] EvenKey :buffer of even key
* @param [] KeyLen :length of key
* @return      int32_t  >= 0, sucess.<0 failure
* @note     we sugest to use this interface, user can save even key or odd key ,and send them to
            descrambler together.
*/
int32_t GxDescrmb_SetCW(handle_t Descrambler,
                                const uint8_t* OddKey,
                                const uint8_t* EvenKey,
                                size_t KeyLen )
{
    uint16_t      stream_pid  = INVALID_PID;
    GxDescCWParam params;

    ASSERT(Descrambler != E_INVALID_HANDLE);
    ASSERT(EvenKey != NULL);
    ASSERT(OddKey != NULL);

    memset(&params, 0, sizeof(GxDescCWParam));

    GxCore_MutexLock(thiz_desc_ctrl.mutex);
    stream_pid = _get_stream_pid(Descrambler);
    GxCore_MutexUnlock(thiz_desc_ctrl.mutex);

    if(INVALID_PID == stream_pid)
        return -1;

    memcpy(params.odd_ECW, OddKey, KeyLen);
    memcpy(params.even_ECW, EvenKey, KeyLen);
    params.ECW_len = KeyLen;

    //普安不会用到klm,所以算法随便填了一个
    return GxDescSetCw(Descrambler, stream_pid, GXDESC_KLM_ALG_3DES, &params);
}

int32_t GxDescrmb_GetCW(handle_t Descrambler,
                                uint8_t* OddKey,
                                uint8_t* EvenKey)
{
    return 0;
}

/**
* @brief      Send cw (odd key & even key) to descrambler.It is provide to mtc high level encryption system specially.
* @brief      support gx6605s 3des 2nd order, gx3211 3des 2nd order, aes 2nd order
* @param [] Descrambler :handle of descrambler,get via GxDescrmb_Open();
* @param [in] EcwEven :buffer of even key
* @param [in] EcwOdd :buffer of odd key
* @param [in] Eck :buffer of eck
* @param [] EcwEvenLen :length of key
* @param [] EcwOddLen :length of key
* @param [] EckLen :length of key
* @param [] arith_mode :mode
* @param [] arith_type :type
* @return      int32_t  >= 0, sucess.<0 failure
* @note     we sugest to use this interface, user can save even key or odd key ,and send them to
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
                                enum mtc_arith_mode arith_mode)
{
    GxDescCWParam params;
    GxDescKLMAlg alg = GXDESC_KLM_ALG_3DES;

    ASSERT(Descrambler != E_INVALID_HANDLE);
    ASSERT(EcwEven != NULL);
    ASSERT(EcwOdd != NULL);
    ASSERT(Eck != NULL);

    GxDescConfig(Descrambler, GXDESC_ALG_CAS_CSA2, _GetKLMType(), 2);

    if(GXMTCCA_3DES_ARITH == arith_type)
    {
        alg = GXDESC_KLM_ALG_3DES;
        GxDescSetKn(Descrambler, 0, GXDESC_KLM_ALG_3DES, (uint8_t *)Eck, EckLen);
    }
    else if(GXMTCCA_AES128_ARITH == arith_type)
    {
        alg = GXDESC_KLM_ALG_AES;
        GxDescSetKn(Descrambler, 0, GXDESC_KLM_ALG_AES, (uint8_t *)Eck, EckLen);
    }
    else
    {
        return -1;
    }

    memset(&params, 0, sizeof(GxDescCWParam));

    memcpy(params.odd_ECW, EcwOdd, EcwOddLen);
    memcpy(params.even_ECW, EcwEven, EcwEvenLen);
    params.ECW_len = EcwEvenLen;

    return GxDescSetCw(Descrambler, pid, alg, &params);
}

/**
* @brief      Send  even key to descrambler.It is provide to mtc high level encryption system specially.
* @brief      support gx6605s 3des 2nd order, gx3211 3des 2nd order, aes 2nd order
* @param [] Descrambler :handle of descrambler,get via GxDescrmb_Open();
* @param [in] EcwEven :buffer of even key
* @param [in] Eck :buffer of eck
* @param [] EcwEvenLen :length of key
* @param [] EckLen :length of key
* @param [] arith_mode :mode
* @param [] arith_type :type
* @return      int32_t  >= 0, sucess.<0 failure
*/
int32_t GxDescrmb_SetEvenECW(handle_t Descrambler,
                                const uint8_t* EcwEven,
                                size_t EcwEvenLen,
                                const uint8_t* Eck,
                                size_t EckLen,
                                enum mtc_arith_type arith_type,
                                enum mtc_arith_mode arith_mode)
{
    GxDescCWParam params;
    GxDescKLMAlg alg = GXDESC_KLM_ALG_3DES;
    uint16_t stream_pid = INVALID_PID;

    ASSERT(Descrambler != E_INVALID_HANDLE);
    ASSERT(EcwEven != NULL);
    ASSERT(Eck != NULL);

    GxDescConfig(Descrambler, GXDESC_ALG_CAS_CSA2, _GetKLMType(), 2);

    if(GXMTCCA_3DES_ARITH == arith_type)
    {
        alg = GXDESC_KLM_ALG_3DES;
        GxDescSetKn(Descrambler, 0, GXDESC_KLM_ALG_3DES, (uint8_t *)Eck, EckLen);
    }
    else if(GXMTCCA_AES128_ARITH == arith_type)
    {
        alg = GXDESC_KLM_ALG_AES;
        GxDescSetKn(Descrambler, 0, GXDESC_KLM_ALG_AES, (uint8_t *)Eck, EckLen);
    }
    else
    {
        return -1;
    }

    memset(&params, 0, sizeof(GxDescCWParam));
    memcpy(params.even_ECW, EcwEven, EcwEvenLen);
    params.ECW_len = EcwEvenLen;

    GxCore_MutexLock(thiz_desc_ctrl.mutex);
    stream_pid = _get_stream_pid(Descrambler);
    GxCore_MutexUnlock(thiz_desc_ctrl.mutex);

    if(INVALID_PID == stream_pid)
        return -1;

    return GxDescSetEvenCw(Descrambler, stream_pid, alg, &params);
}

/**
* @brief      Send  even key to descrambler.It is provide to mtc high level encryption system specially.
* @brief      support gx6605s 3des 2nd order, gx3211 3des 2nd order, aes 2nd order
* @param [] Descrambler :handle of descrambler,get via GxDescrmb_Open();
* @param [in] EcwEven :buffer of odd key
* @param [in] Eck :buffer of eck
* @param [] EcwEvenLen :length of key
* @param [] EckLen :length of key
* @param [] arith_mode :mode
* @param [] arith_type :type
* @return      int32_t  >= 0, sucess.<0 failure
*/
int32_t GxDescrmb_SetOddECW(handle_t Descrambler,
                                const uint8_t* EcwOdd,
                                size_t EcwOddLen,
                                const uint8_t* Eck,
                                size_t EckLen,
                                enum mtc_arith_type arith_type,
                                enum mtc_arith_mode arith_mode)
{
    GxDescCWParam params;
    GxDescKLMAlg alg = GXDESC_KLM_ALG_3DES;
    uint16_t stream_pid = INVALID_PID;

    ASSERT(Descrambler != E_INVALID_HANDLE);
    ASSERT(EcwOdd != NULL);
    ASSERT(Eck != NULL);

    GxDescConfig(Descrambler, GXDESC_ALG_CAS_CSA2, _GetKLMType(), 2);

    if(GXMTCCA_3DES_ARITH == arith_type)
    {
        alg = GXDESC_KLM_ALG_3DES;
        GxDescSetKn(Descrambler, 0, GXDESC_KLM_ALG_3DES, (uint8_t *)Eck, EckLen);
    }
    else if(GXMTCCA_AES128_ARITH == arith_type)
    {
        alg = GXDESC_KLM_ALG_AES;
        GxDescSetKn(Descrambler, 0, GXDESC_KLM_ALG_AES, (uint8_t *)Eck, EckLen);
    }
    else
    {
        return -1;
    }

    memset(&params, 0, sizeof(GxDescCWParam));
    memcpy(params.odd_ECW, EcwOdd, EcwOddLen);
    params.ECW_len = EcwOddLen;

    GxCore_MutexLock(thiz_desc_ctrl.mutex);
    stream_pid = _get_stream_pid(Descrambler);
    GxCore_MutexUnlock(thiz_desc_ctrl.mutex);

    if(INVALID_PID == stream_pid)
        return -1;

    return GxDescSetOddCw(Descrambler, stream_pid, alg, &params);
}

int32_t GxDescrmb_Handle2Id(handle_t des)
{
    return 0;
}

// new api func
#include "dvbhal/gxdescrambler_hal.h"
#include "gxsecure/gxsecure_api.h"
#include "gxsecure/gxtfm_api.h"
#include "gxsecure/gxtfm.h"
#include "app_config.h"

// defined by project
#if (SECURE_FULL_SUPPORT || SECURE_SCPU_SUPPORT)
#if (defined TAURUS || defined GEMINI || defined CYGNUS || defined SIRIUS || defined CANOPUS || defined VEGA || defined SCORPIO || defined SCORPIO_TAURUS)
#define SCPU_USED
#endif
#endif


static int  _GetKLMType(void)
{
#ifdef SCPU_USED
    return GXDESC_KLM_SCPU_DYNAMIC;
#elif (SECURE_ACPU_SUPPORT || SECURE_FULL_SUPPORT)
#if (defined SIRIUS || defined CANOPUS || defined VEGA)
    return GXDESC_KLM_ACPU_GENERIC;
#else
    return GXDESC_KLM_MTC;
#endif
#else
#if (defined TAURUS || defined GEMINI || defined CYGNUS || defined SIRIUS || defined CANOPUS || defined VEGA || defined SCORPIO || defined SCORPIO_TAURUS)
    return GXDESC_KLM_NONE;
#else
    return GXDESC_KLM_MTC;
#endif
#endif
}

static int _change_stream_alg(DescAlgEnum stream_alg)
{
    int ret = 0;
    switch(stream_alg)
    {
        case APP_DESC_CSA2:
            ret = GXDESC_ALG_CAS_CSA2;
            break;
        case APP_DESC_CSA2_CONFORMANCE:
            ret = GXDESC_ALG_CAS_CSA2_CONFORMANCE;
            break;
        case APP_DESC_CSA3:
            ret = GXDESC_ALG_CAS_CSA3;
            break;
        case APP_DESC_AES_ECB:
            ret = GXDESC_ALG_CAS_AES_ECB;
            break;
        case APP_DESC_IDSA:
            ret = GXDESC_ALG_CAS_IDSA;
            break;
        case APP_DESC_AES_CBC:
            ret = GXDESC_ALG_CAS_AES_CBC;
            break;
        case APP_DESC_DES:
            ret = GXDESC_ALG_CAS_DES;
            break;
        case APP_DESC_TDES:
            ret = GXDESC_ALG_CAS_TDES;
            break;
        case APP_DESC_SM4_ECB:
            ret = GXDESC_ALG_CAS_SM4_ECB;
            break;
        case APP_DESC_SM4_CBC:
            ret = GXDESC_ALG_CAS_SM4_CBC;
            break;
        case APP_DESC_MULTI2:
            ret = GXDESC_ALG_CAS_MULTI2;
            break;
            default:
            break;
    }
    return ret;
}

static unsigned int _get_firmware_addr(DescFirmwareAreaEnum type)
{
    unsigned int ret = 0;
    switch(type)
    {
        case APP_DESC_FW_SRAM:
             #if (defined TAURUS || defined SCORPIO || defined SCORPIO_TAURUS)
                ret = 0x80000;
             #elif (defined GEMINI)
                ret = 0x80000;
             #elif (defined CYGNUS)
                ret = 0x80000;
             #elif (defined SIRIUS)
                ret = 0x90000;
             #elif (defined CANOPUS)
                ret = 0x90000;
             #elif (defined VEGA)
                ret = 0x90000;
             #endif
            break;
        case APP_DESC_FW_DRAM:
             #if (defined TAURUS || defined SCORPIO || defined SCORPIO_TAURUS)
                ret = 0x40000;
             #elif (defined GEMINI)
                ret = 0x40000;
             #elif (defined CYGNUS)
                ret = 0x40000;
             #endif
            break;
        default:
            break;
    }
    return ret;
}

static int _change_klm_alg(DescKLMAlgEnum alg)
{
    int ret = 0;
    switch(alg)
    {
        case APP_DESC_KLM_3DES:
            ret = GXDESC_KLM_ALG_3DES;
            break;
        case APP_DESC_KLM_AES:
            ret = GXDESC_KLM_ALG_AES;
            break;
        default:
            break;
    }
    return ret;
}

static int _change_m2m_alg(DescM2MAlgEnum alg)
{
    int ret = 0;
    switch(alg)
    {
        case APP_M2M_DES:
            ret = TFM_ALG_DES;
            break;
        case APP_M2M_3DES:
            ret = TFM_ALG_TDES;
            break;
        case APP_M2M_AES128:
            ret = TFM_ALG_AES128;
            break;
        case APP_M2M_AES192:
            ret = TFM_ALG_AES192;
            break;
        case APP_M2M_AES256:
            ret = TFM_ALG_AES256;
            break;
        default:
            break;
    }
    return ret;
}

static int _change_m2m_opt(DescM2MOptEnum opt)
{
    int ret = 0;
    switch(opt)
    {
        case APP_M2M_ECB:
            ret = TFM_OPT_ECB;
            break;
        case APP_M2M_CBC:
            ret = TFM_OPT_CBC;
            break;
        case APP_M2M_CFB:
            ret = TFM_OPT_CFB;
            break;
        case APP_M2M_OFB:
            ret = TFM_OPT_OFB;
            break;
        case APP_M2M_CTR:
            ret = TFM_OPT_CTR;
            break;
        default:
            break;
    }
    return ret;
}

#define ROOTKET_ID_FOR_DESC     0

static int _change_m2m_keytype(DescM2MKeyEnum type)
{
    int ret = 0;
    switch(type)
    {
        case APP_M2M_ACPU_SOFT:
            ret = TFM_KEY_ACPU_SOFT;
            break;
        case APP_M2M_OTP_SOFT:
            ret = TFM_KEY_OTP_SOFT;
            break;
        default:
            break;
    }
    return ret;
}


int GxDesc_Open(unsigned int demux_id, DescAlgEnum stream_alg)
{
    static bool init = false;
    handle_t ret_handle = E_INVALID_HANDLE;

    if(false == init)
    {
        GxDescInitParam param = {0};

        param.klm = GXDESC_KLM_NONE | _GetKLMType();
        if(GxDescInit(&param) != 0)
            return E_INVALID_HANDLE;

        if(0 == thiz_desc_ctrl.mutex)
            GxCore_MutexCreate(&thiz_desc_ctrl.mutex);

        _ctrl_init();
        init = true;
    }

    ret_handle = GxDescOpen(GXDESC_MOD_TS, demux_id, _change_stream_alg(stream_alg)/*GXDESC_ALG_CAS_CSA2*/);
    if(ret_handle != -1 && ret_handle != -2)
    {
        _add_desc_handle(ret_handle);
        _add_stream_alg(ret_handle, stream_alg);
    }
    else
    {
        ret_handle = E_INVALID_HANDLE;
    }
    return ret_handle;
}

int GxDesc_SetConfig(handle_t Descrambler, DescAlgEnum stream_alg)
{
    ASSERT(Descrambler != E_INVALID_HANDLE);
    //GxDescConfig(Descrambler,stream_alg, GXDESC_KLM_NONE, 0);
    GxCore_MutexLock(thiz_desc_ctrl.mutex);
    _add_stream_alg(Descrambler, stream_alg);
    GxCore_MutexUnlock(thiz_desc_ctrl.mutex);
    return 0;
}

int GxDesc_SetOddKey(handle_t Descrambler, const unsigned char* OddKey, unsigned char KeyLen)
{
    unsigned short stream_pid  = INVALID_PID;
    GxDescCWParam params;
    GxDescAlg stream_alg = 0;

    ASSERT(Descrambler != E_INVALID_HANDLE);
    ASSERT(OddKey != NULL);
    ASSERT(KeyLen <= 8);

    memset(&params, 0, sizeof(GxDescCWParam));

    GxCore_MutexLock(thiz_desc_ctrl.mutex);
    stream_pid = _get_stream_pid(Descrambler);
    GxCore_MutexUnlock(thiz_desc_ctrl.mutex);

    if(INVALID_PID == stream_pid)
        return -1;

    GxCore_MutexLock(thiz_desc_ctrl.mutex);
    stream_alg = _change_stream_alg(_get_stream_alg(Descrambler));
    GxCore_MutexUnlock(thiz_desc_ctrl.mutex);

    GxDescConfig(Descrambler,stream_alg, GXDESC_KLM_NONE, 0);

    memcpy(params.odd_ECW, OddKey, KeyLen);
    params.ECW_len = KeyLen;

    //普安不会用到klm,所以算法随便填了一个
    return GxDescSetOddCw(Descrambler, stream_pid, 0, &params);
}

int GxDesc_SetEvenKey(int Descrambler, const unsigned char*  EvenKey, unsigned char KeyLen )
{
    unsigned short  stream_pid  = INVALID_PID;
    GxDescCWParam params;
    GxDescAlg stream_alg = 0;

    ASSERT(Descrambler != E_INVALID_HANDLE);
    ASSERT(EvenKey != NULL);
    ASSERT(KeyLen <= 8);

    memset(&params, 0, sizeof(GxDescCWParam));

    GxCore_MutexLock(thiz_desc_ctrl.mutex);
    stream_pid = _get_stream_pid(Descrambler);
    GxCore_MutexUnlock(thiz_desc_ctrl.mutex);

    if(INVALID_PID == stream_pid)
        return -1;

    GxCore_MutexLock(thiz_desc_ctrl.mutex);
    stream_alg = _change_stream_alg(_get_stream_alg(Descrambler));
    GxCore_MutexUnlock(thiz_desc_ctrl.mutex);

    GxDescConfig(Descrambler,stream_alg, GXDESC_KLM_NONE, 0);

    memcpy(params.even_ECW, EvenKey, KeyLen);
    params.ECW_len = KeyLen;

    //普安不会用到klm,所以算法随便填了一个
    return GxDescSetEvenCw(Descrambler, stream_pid, 0, &params);
}

int GxDesc_SetCW(int Descrambler,
                                const unsigned char* OddKey,
                                const unsigned char* EvenKey,
                                unsigned char KeyLen )
{
    unsigned short  stream_pid  = INVALID_PID;
    GxDescCWParam params;
    GxDescAlg stream_alg = 0;

    ASSERT(Descrambler != E_INVALID_HANDLE);
    ASSERT(EvenKey != NULL);
    ASSERT(OddKey != NULL);
    ASSERT(KeyLen <= 16);

    memset(&params, 0, sizeof(GxDescCWParam));

    GxCore_MutexLock(thiz_desc_ctrl.mutex);
    stream_pid = _get_stream_pid(Descrambler);
    GxCore_MutexUnlock(thiz_desc_ctrl.mutex);

    if(INVALID_PID == stream_pid)
        return -1;

    GxCore_MutexLock(thiz_desc_ctrl.mutex);
    stream_alg = _change_stream_alg(_get_stream_alg(Descrambler));
    GxCore_MutexUnlock(thiz_desc_ctrl.mutex);

    GxDescConfig(Descrambler,stream_alg, GXDESC_KLM_NONE, 0);

    memcpy(params.odd_ECW, OddKey, KeyLen);
    memcpy(params.even_ECW, EvenKey, KeyLen);
    params.ECW_len = KeyLen;

    //普安不会用到klm,所以算法随便填了一个
    return GxDescSetCw(Descrambler, stream_pid, 0, &params);
}

int GxM2M_Decrypt(DescM2MDataClass data, DescM2MAlgClass alg)
{
    GxTfmCrypto param = {
        .module        = TFM_MOD_M2M,
        .iv            = {0},
        .input.length  = data.in_bytes,
        .output.length = data.out_bytes,
        .input.buf     = (unsigned char *)data.src,
        .output.buf    = (unsigned char *)data.dst,

    };

    param.alg         = _change_m2m_alg(alg.alg);
    param.opt         = _change_m2m_opt(alg.opt);
    param.even_key.id = _change_m2m_keytype(alg.key_type);
    if(TFM_KEY_ACPU_SOFT == param.even_key.id)
    {
        memcpy(param.soft_key, alg.keys.soft_key, 32);
    }
    memcpy(param.iv, alg.keys.iv, 16);

    return GxTfm_Decrypt(&param);
}

int GxM2M_Encrypt(DescM2MDataClass data, DescM2MAlgClass alg)
{
    GxTfmCrypto param = {
        .module        = TFM_MOD_M2M,
        .iv            = {0},
        .input.length  = data.in_bytes,
        .output.length = data.out_bytes,
        .input.buf     = (unsigned char *)data.src,
        .output.buf    = (unsigned char *)data.dst,
    };

    param.alg         = _change_m2m_alg(alg.alg);
    param.opt         = _change_m2m_opt(alg.opt);
    param.even_key.id = _change_m2m_keytype(alg.key_type);
    if(TFM_KEY_ACPU_SOFT == param.even_key.id)
    {//
        memcpy(param.soft_key, alg.keys.soft_key, 32);
    }
    memcpy(param.iv, alg.keys.iv, 16);

    return GxTfm_Encrypt(&param);
}

// 当使用desc的GXDESC_KLM_SCPU_DYNAMIC属性的时候，需要使用如下接口注册相关固件
int GxDesc_LoadFirmware(unsigned char *fw, unsigned int fw_len, DescFirmwareAreaEnum type)
{
    int ret = 0;

    GxSecureLoader loader = {
        .loader   = fw,// 安全小组发布，需要依据项目申请
        .size     = fw_len,
        .dst_addr = _get_firmware_addr(type),// 根据项目以及芯片设定不同的值，taurus，如果使用小固件，可以使用0x80000开始的sram的地址空间
    };

    GxSecure_LoadFirmware(&loader);
    // 该部分可以根据方案需求修改，目前是测试使用，死等到数据ok。如果要兼容普通的芯片，那该部分可以根据返回值做差异化处理。
    // FW_RUNNING,FW_RESET,FW_ERROR,FW_APP_START,
    while((ret = GxSecure_GetStatus()) != FW_APP_START);

    return ret;
}

// keyladder
int GxDesc_SetECW(int Descrambler,
                                const unsigned char* OddKey,
                                const unsigned char* EvenKey,
                                unsigned char KeyLen,
                                DescKLMAlgEnum ecw_alg,
                                unsigned char root_index,
                                DescKLMClass klm_info)
{
    unsigned short  stream_pid  = INVALID_PID;
    GxDescCWParam params;
    GxDescAlg stream_alg = 0;
    int i = 0;

    ASSERT(Descrambler != E_INVALID_HANDLE);
    ASSERT(EvenKey != NULL);
    ASSERT(OddKey != NULL);
    ASSERT((KeyLen%8 == 0) && (KeyLen/8 > 0));
    ASSERT((klm_info.unit_count > 0)&&(klm_info.unit_count <= MAX_KLM_LEVE));

    memset(&params, 0, sizeof(GxDescCWParam));

    GxCore_MutexLock(thiz_desc_ctrl.mutex);
    stream_pid = _get_stream_pid(Descrambler);
    GxCore_MutexUnlock(thiz_desc_ctrl.mutex);

    if(INVALID_PID == stream_pid)
        return -1;

    // for config
    GxCore_MutexLock(thiz_desc_ctrl.mutex);
    stream_alg = _change_stream_alg(_get_stream_alg(Descrambler));
    GxCore_MutexUnlock(thiz_desc_ctrl.mutex);

    GxDescConfig(Descrambler,stream_alg, _GetKLMType(), (klm_info.unit_count +1));

    // for root key
    GxDescSelectRootkey(Descrambler, ROOTKET_ID_FOR_DESC, root_index);

    // for eks
    for(i = 0; i < klm_info.unit_count; i++)
    {
        GxDescSetKn(Descrambler, i, _change_klm_alg(klm_info.klm_unit[i].alg), klm_info.klm_unit[i].ek, klm_info.klm_unit[i].ek_bytes);
    }

    memcpy(params.odd_ECW, OddKey, KeyLen);
    memcpy(params.even_ECW, EvenKey, KeyLen);
    params.ECW_len = KeyLen;
    return GxDescSetCw(Descrambler, stream_pid, _change_klm_alg(ecw_alg), &params);
}

int GxDesc_SetECWOdd(int Descrambler,
                                const unsigned char* OddKey,
                                unsigned char KeyLen,
                                DescKLMAlgEnum ecw_alg,
                                unsigned char root_index,
                                DescKLMClass klm_info)
{
    unsigned short  stream_pid  = INVALID_PID;
    GxDescCWParam params;
    GxDescAlg stream_alg = 0;
    int i = 0;

    ASSERT(Descrambler != E_INVALID_HANDLE);
    ASSERT(OddKey != NULL);
    ASSERT((KeyLen%8 == 0) && (KeyLen/8 > 0));
    ASSERT((klm_info.unit_count > 0)&&(klm_info.unit_count <= MAX_KLM_LEVE));

    memset(&params, 0, sizeof(GxDescCWParam));

    GxCore_MutexLock(thiz_desc_ctrl.mutex);
    stream_pid = _get_stream_pid(Descrambler);
    GxCore_MutexUnlock(thiz_desc_ctrl.mutex);

    if(INVALID_PID == stream_pid)
        return -1;

    // for config
    GxCore_MutexLock(thiz_desc_ctrl.mutex);
    stream_alg = _change_stream_alg(_get_stream_alg(Descrambler));
    GxCore_MutexUnlock(thiz_desc_ctrl.mutex);

    GxDescConfig(Descrambler,stream_alg, _GetKLMType(), (klm_info.unit_count +1));

    // for root key
    GxDescSelectRootkey(Descrambler, ROOTKET_ID_FOR_DESC, root_index);

    // for eks
    for(i = 0; i < klm_info.unit_count; i++)
    {
        GxDescSetKn(Descrambler, i, _change_klm_alg(klm_info.klm_unit[i].alg), klm_info.klm_unit[i].ek, klm_info.klm_unit[i].ek_bytes);
    }

    memcpy(params.odd_ECW, OddKey, KeyLen);
    params.ECW_len = KeyLen;
    return GxDescSetOddCw(Descrambler, stream_pid, _change_klm_alg(ecw_alg), &params);
}

int GxDesc_SetECWEven(int Descrambler,
                                const unsigned char* EvenKey,
                                unsigned char KeyLen,
                                DescKLMAlgEnum ecw_alg,
                                unsigned char root_index,
                                DescKLMClass klm_info)
{
    unsigned short  stream_pid  = INVALID_PID;
    GxDescCWParam params;
    GxDescAlg stream_alg = 0;
    int i = 0;

    ASSERT(Descrambler != E_INVALID_HANDLE);
    ASSERT(EvenKey != NULL);
    ASSERT((KeyLen%8 == 0) && (KeyLen/8 > 0));
    ASSERT((klm_info.unit_count > 0)&&(klm_info.unit_count <= MAX_KLM_LEVE));

    memset(&params, 0, sizeof(GxDescCWParam));

    GxCore_MutexLock(thiz_desc_ctrl.mutex);
    stream_pid = _get_stream_pid(Descrambler);
    GxCore_MutexUnlock(thiz_desc_ctrl.mutex);

    if(INVALID_PID == stream_pid)
        return -1;

    // for config
    GxCore_MutexLock(thiz_desc_ctrl.mutex);
    stream_alg = _change_stream_alg(_get_stream_alg(Descrambler));
    GxCore_MutexUnlock(thiz_desc_ctrl.mutex);

    GxDescConfig(Descrambler,stream_alg, _GetKLMType(), (klm_info.unit_count +1));

    // for root key
    GxDescSelectRootkey(Descrambler, ROOTKET_ID_FOR_DESC, root_index);

    // for eks
    for(i = 0; i < klm_info.unit_count; i++)
    {
        GxDescSetKn(Descrambler, i, _change_klm_alg(klm_info.klm_unit[i].alg), klm_info.klm_unit[i].ek, klm_info.klm_unit[i].ek_bytes);
    }

    memcpy(params.even_ECW, EvenKey, KeyLen);
    params.ECW_len = KeyLen;
    return GxDescSetEvenCw(Descrambler, stream_pid, _change_klm_alg(ecw_alg), &params);
}
