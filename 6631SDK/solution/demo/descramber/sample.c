#include "app_config.h"
#include "module/app_descrambler_api.h"
#include "module/app_log.h"
#include "dvbhal/gxdescrambler_hal.h"

#define ACPU_GENERIC_USED 0

static int thiz_test_v = 0;
static int thiz_test_a = 0;
static bool thiz_test_rootkey_select = false;


static int thiz_demux_id = 0;
// 码流的算法类型，该类型决定了解扰器使用的key的长度，比如CSA2使用8偶、奇秘钥要长度是8字节长度的。
// 而TDES可以使用8字节或者16字节（有前段决定），AES只能至少使用16字节。
static int thiz_stream_alg = APP_DESC_CSA2;

static signed short thiz_v_pid = 528;//视频PID
static unsigned short thiz_a_pid = 529;//音频PID

// 明文cw, 数据根据实际码流需要自行替换
// 目前测试码流为CSA2，所以明文的cw只要8字节的奇偶就可以了。
static unsigned char thiz_normal_oddcw[] =  { 0xA8, 0x9C, 0x0F, 0x53, 0x51, 0x08, 0xBD, 0x16 };
static unsigned char thiz_normal_evencw[] = { 0x19, 0xDF, 0x63, 0x5B, 0xDC, 0xB8, 0x90, 0x24 };

// GX芯片提供多rootkey的选择功能
static unsigned char thiz_root_index = 0;

// AES128 2级KeyLadder
// AES算法要求，128bit，加密后数据为16字节。(即使原始流的秘钥长度只有8字节)
//
#if ACPU_GENERIC_USED
static unsigned char thiz_aes128_2level_ecwodd[16] =
    {0xad, 0x25, 0x9a, 0x8f, 0x03, 0xdf, 0x0d, 0x3c, 0x27, 0x3b, 0x90, 0x2c, 0x58, 0x11, 0xb8, 0xbd};
static unsigned char thiz_aes128_2level_ecweven[16] =
    {0x20, 0xc4, 0x3d, 0x71, 0x74, 0x55, 0xb5, 0x9f, 0x94, 0xbf, 0xd5, 0x5c, 0x5c, 0xb0, 0x29, 0x96};
#else
static unsigned char thiz_aes128_2level_ecwodd[16] =
    {0xAB, 0x25, 0x0D, 0x8C, 0x44, 0x97, 0x3C, 0x7D, 0x7E, 0xF4, 0xB6, 0xDD, 0x05, 0xE8, 0x40, 0x98};
static unsigned char thiz_aes128_2level_ecweven[16] =
    {0xc7, 0xbe, 0xc1, 0xaa, 0xba, 0x45, 0xab, 0x68, 0x82, 0x3e, 0xc3, 0xe7, 0x93, 0xce, 0x55, 0xda};
#endif
static unsigned char thiz_aes128_2level_ek1[16] =
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

// AES128 3级KeyLadder
// AES算法要求，128bit，加密后数据为16字节。(即使原始流的秘钥长度只有8字节)
//
#if ACPU_GENERIC_USED
static unsigned char thiz_aes128_3level_ecwodd[16] =
    {0xc1, 0xe6, 0x4e, 0x9e, 0x12, 0x8d, 0x19, 0xf6, 0xb2, 0x33, 0xac, 0x34, 0xb4, 0x03, 0xa8, 0x01};
static unsigned char thiz_aes128_3level_ecweven[16] =
    {0xa3, 0xb3, 0x4d, 0x42, 0x96, 0x11, 0x38, 0x06, 0x33, 0x8b, 0x31, 0xbd, 0xc0, 0xa7, 0x08, 0x4b};
#else
static unsigned char thiz_aes128_3level_ecwodd[16] =
    {0x29, 0xf7, 0xfe, 0x58, 0x22, 0xeb, 0x61, 0x8a, 0x9b, 0x33, 0xae, 0xfe, 0x3b, 0x2d, 0x1b, 0x4f};
static unsigned char thiz_aes128_3level_ecweven[16] =
    {0xb8, 0xb6, 0x04, 0x3a, 0x4f, 0x47, 0x4a, 0x98, 0x54, 0x26, 0x39, 0xbe, 0xf7, 0x8f, 0x34, 0x6a};
#endif
static unsigned char thiz_aes128_3level_ek1[16] =
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
static unsigned char thiz_aes128_3level_ek2[16] =
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

// TDES 2级keyLadder
// 关于TDES，8字节对称加解密，码流是CSA2的情况下，实际有效秘钥是8字节长度，所以可以使用8字节ECW，实际底层
// 驱动会根据实际情况选择使用16字节还是8字节。具体的其实主要关注前端提供的key是什么即可
#if ACPU_GENERIC_USED
static unsigned char thiz_tdes_2level_ecwodd[8] =
    {0x01, 0x6d, 0x6b, 0x14, 0xac, 0x88, 0xde, 0x20};
static unsigned char thiz_tdes_2level_ecweven[8] =
    {0xa8, 0x5d, 0x50, 0xa7, 0x09, 0xd1, 0xdc, 0x84};
#else
static unsigned char thiz_tdes_2level_ecwodd[8] =
    {0x50, 0x72, 0xde, 0x89, 0x37, 0xf2, 0xa2, 0x11};
static unsigned char thiz_tdes_2level_ecweven[8] =
    {0xfe, 0x58, 0x1a, 0xaa, 0x89, 0xbd, 0xc0, 0x91};
#endif
static uint8_t thiz_tdes_2level_ek1[16] =
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

// TDES 3级keyLadder
// 关于TDES，8字节对称加解密，码流是CSA2的情况下，实际有效秘钥是8字节长度，所以可以使用8字节ECW，实际底层
// 驱动会根据实际情况选择使用16字节还是8字节。具体的其实主要关注前端提供的key是什么即可
#if ACPU_GENERIC_USED
static unsigned char thiz_tdes_3level_ecwodd[8] =
    {0xa4, 0x8e, 0x96, 0x64, 0x2f, 0x8d, 0xff, 0xc7};
static unsigned char thiz_tdes_3level_ecweven[8] =
    {0x5e, 0x61, 0x9a, 0x90, 0x8a, 0xb8, 0x4c, 0x25};
#else
static unsigned char thiz_tdes_3level_ecwodd[8] =
    {0x10, 0x8f, 0x1d, 0x55, 0x77, 0xc8, 0x3b, 0xd5};
static unsigned char thiz_tdes_3level_ecweven[8] =
    {0x3b, 0x7c, 0xab, 0x0d, 0x75, 0xd8, 0xbe, 0x2d};
#endif
static unsigned char thiz_tdes_3level_ek1[16] =
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
static unsigned char thiz_tdes_3level_ek2[16] =
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

/*********************old api test*********************/
void CATest_OldApi_SetCw(int32_t flag, bool parity_key)
{
    if(thiz_test_a)
    {
        GxDescrmb_Close(thiz_test_a);
        thiz_test_a = 0;
    }

    if(thiz_test_v)
    {
        GxDescrmb_Close(thiz_test_v);
        thiz_test_v = 0;
    }

    thiz_test_a = GxDescrmb_Open(0);
    thiz_test_v = GxDescrmb_Open(0);

    if(0 == thiz_test_a || 0 == thiz_test_v)
        return;

    GxDescrmb_SetStreamPID(thiz_test_a, thiz_a_pid);
    GxDescrmb_SetStreamPID(thiz_test_v, thiz_v_pid);

    if(0 == flag)//plaintext
    {
        app_log_min("\033[31m %s mode, plaintext test\033[0m\n", parity_key == true ? "parity": "separate parity");
        if(true == parity_key)
        {
            GxDescrmb_SetCW(thiz_test_a, thiz_normal_oddcw, thiz_normal_evencw, 8);
            GxDescrmb_SetCW(thiz_test_v, thiz_normal_oddcw, thiz_normal_evencw, 8);
        }
        else
        {
            //plaintext separate parity keys
            GxDescrmb_SetOddKey(thiz_test_a, thiz_normal_oddcw, 8);
            GxDescrmb_SetEvenKey(thiz_test_a, thiz_normal_evencw, 8);
            GxDescrmb_SetOddKey(thiz_test_v, thiz_normal_oddcw, 8);
            GxDescrmb_SetEvenKey(thiz_test_v, thiz_normal_evencw, 8);
        }
    }
    else if(1 == flag)//aes128 2阶
    {
        app_log_min("\033[31m %s mode, aes128 2nd keyladder test\033[0m\n", parity_key == true ? "parity": "separate parity");
        if(true == parity_key)
        {
            GxDescrmb_SetECW(thiz_test_a, thiz_a_pid, thiz_aes128_2level_ecweven, 16, thiz_aes128_2level_ecwodd, 16, thiz_aes128_2level_ek1, 16, GXMTCCA_AES128_ARITH, 0);
            GxDescrmb_SetECW(thiz_test_v, thiz_v_pid, thiz_aes128_2level_ecweven, 16, thiz_aes128_2level_ecwodd, 16, thiz_aes128_2level_ek1, 16, GXMTCCA_AES128_ARITH, 0);
        }
        else
        {
            GxDescrmb_SetOddECW(thiz_test_a, thiz_aes128_2level_ecwodd, 16, thiz_aes128_2level_ek1, 16, GXMTCCA_AES128_ARITH, 0);
            GxDescrmb_SetEvenECW(thiz_test_a, thiz_aes128_2level_ecweven, 16, thiz_aes128_2level_ek1, 16, GXMTCCA_AES128_ARITH, 0);
            GxDescrmb_SetOddECW(thiz_test_v, thiz_aes128_2level_ecwodd, 16, thiz_aes128_2level_ek1, 16, GXMTCCA_AES128_ARITH, 0);
            GxDescrmb_SetEvenECW(thiz_test_v, thiz_aes128_2level_ecweven, 16, thiz_aes128_2level_ek1, 16, GXMTCCA_AES128_ARITH, 0);
        }
    }
    else if(2 == flag)//3des 2阶
    {
        app_log_min("\033[31m %s mode, tdes 2nd keyladder test\033[0m\n", parity_key == true ? "parity": "separate parity");
        if(true == parity_key)
        {
            GxDescrmb_SetECW(thiz_test_a, thiz_a_pid, thiz_tdes_2level_ecweven, 8, thiz_tdes_2level_ecwodd, 8, thiz_tdes_2level_ek1, 16, GXMTCCA_3DES_ARITH, 0);
            GxDescrmb_SetECW(thiz_test_v, thiz_v_pid, thiz_tdes_2level_ecweven, 8, thiz_tdes_2level_ecwodd, 8, thiz_tdes_2level_ek1, 16, GXMTCCA_3DES_ARITH, 0);
        }
        else
        {
            GxDescrmb_SetOddECW(thiz_test_a, thiz_tdes_2level_ecwodd, 8, thiz_tdes_2level_ek1, 16, GXMTCCA_3DES_ARITH, 0);
            GxDescrmb_SetEvenECW(thiz_test_a, thiz_tdes_2level_ecweven, 8, thiz_tdes_2level_ek1, 16, GXMTCCA_3DES_ARITH, 0);
            GxDescrmb_SetOddECW(thiz_test_v, thiz_tdes_2level_ecwodd, 8, thiz_tdes_2level_ek1, 16, GXMTCCA_3DES_ARITH, 0);
            GxDescrmb_SetEvenECW(thiz_test_v, thiz_tdes_2level_ecweven, 8, thiz_tdes_2level_ek1, 16, GXMTCCA_3DES_ARITH, 0);
        }
    }
    return;
}

#if (defined TAURUS || defined SIRIUS || defined CYGNUS || defined CANOPUS || defined VEGA || defined SCORPIO)
/*********************new api test*********************/
// 如下参数需要根据实际的码流，芯片和应用模式，自行修改
#if 0 == ACPU_GENERIC_USED
extern unsigned char lib_firmware_gxsecure_fw[];//scpu 固件
extern unsigned int lib_firmware_gxsecure_fw_len;//scpu 固件大小
static DescFirmwareAreaEnum thiz_fw_mode = APP_DESC_FW_SRAM;
#endif

void CATest_Init(void)
{
    /*
     * 芯片(目前taurus/gemini/sirius系列芯片包含该功能，其中taurus/gemini只支持该模式)支持，并项目选用
     * “GXDESC_KLM_SCPU_DYNAMIC”模式的时候，需要使用如下接口加载独立固件。
     * 如下接口，固件加载分DRAM和SRAM，选择DRAM的时候，需要修改gxloader的cmdline，分配专属的内存字段"
     * SCPUMEM=?"，配置大小和实际的固件大小相关，当sram大小能容纳固件的时候建议使用sram
     * 该部分固件根据项目单独管理，单独发布。
    */
#if 0 == ACPU_GENERIC_USED
    GxDesc_LoadFirmware(lib_firmware_gxsecure_fw, lib_firmware_gxsecure_fw_len, thiz_fw_mode);
#endif
}
#endif
void CATest_SetCW(int flag, bool parity_key)
{
#if (defined VEGA && ACPU_GENERIC_USED)
    if(2 == flag || 4 == flag)
    {
        printf("\033[31m Solution test case don't support Vega ACPU GENERIC %s 3nd keyladder\033[0m\n", flag == 2 ? "aes128": "tdes");
        return;
    }
#endif
    if(thiz_test_v)
    {
        GxDesc_Close(thiz_test_v);
        thiz_test_v = 0;
    }
    if(thiz_test_a)
    {
        GxDesc_Close(thiz_test_a);
        thiz_test_v = 0;
    }
    thiz_test_v = GxDesc_Open(thiz_demux_id, thiz_stream_alg);
    thiz_test_a = GxDesc_Open(thiz_demux_id, thiz_stream_alg);

    GxDesc_SetStreamPID(thiz_test_v, thiz_v_pid);
    GxDesc_SetStreamPID(thiz_test_a, thiz_a_pid);

    if(flag == 0)
    {
        app_log_min("\033[31m %s mode, plaintext test\033[0m\n", parity_key == true ? "parity": "separate parity");
        // normal
        if(true == parity_key)
        {
            GxDesc_SetCW(thiz_test_v, thiz_normal_oddcw, thiz_normal_evencw, 8);
            GxDesc_SetCW(thiz_test_a, thiz_normal_oddcw, thiz_normal_evencw, 8);
        }
        else
        {
            GxDesc_SetOddKey(thiz_test_a, thiz_normal_oddcw, 8);
            GxDesc_SetEvenKey(thiz_test_a, thiz_normal_evencw, 8);
            GxDesc_SetOddKey(thiz_test_v, thiz_normal_oddcw, 8);
            GxDesc_SetEvenKey(thiz_test_v, thiz_normal_evencw, 8);
        }
    }
    else if(flag == 1)
    {//AES128 2级KeyLadder

        app_log_min("\033[31m %s mode, aes128 2nd keyladder test\033[0m\n", parity_key == true ? "parity": "separate parity");
        DescKLMClass klm_info = {
            .klm_unit = {
                {//eck
                    .alg = APP_DESC_KLM_AES,
                    .ek = {0},//thiz_aes128_2level_ek1,
                    .ek_bytes = sizeof(thiz_aes128_2level_ek1),
                },
            },
            .unit_count = 1,
        };
        memcpy(klm_info.klm_unit[0].ek, thiz_aes128_2level_ek1, klm_info.klm_unit[0].ek_bytes);

        if(true == parity_key)
        {
            GxDesc_SetECW(thiz_test_v, thiz_aes128_2level_ecwodd,
                    thiz_aes128_2level_ecweven, 16, APP_DESC_KLM_AES, thiz_root_index, klm_info);
            GxDesc_SetECW(thiz_test_a, thiz_aes128_2level_ecwodd,
                    thiz_aes128_2level_ecweven, 16, APP_DESC_KLM_AES, thiz_root_index, klm_info);
        }
        else
        {
            GxDesc_SetECWOdd(thiz_test_v, thiz_aes128_2level_ecwodd, 16,
                    APP_DESC_KLM_AES, thiz_root_index, klm_info);
            GxDesc_SetECWEven(thiz_test_v, thiz_aes128_2level_ecweven, 16,
                    APP_DESC_KLM_AES, thiz_root_index, klm_info);
            GxDesc_SetECWOdd(thiz_test_a, thiz_aes128_2level_ecwodd, 16,
                    APP_DESC_KLM_AES, thiz_root_index, klm_info);
            GxDesc_SetECWEven(thiz_test_a, thiz_aes128_2level_ecweven, 16,
                    APP_DESC_KLM_AES, thiz_root_index, klm_info);
        }

    }
    else if(flag == 2)
    {//AES128 3级KeyLadder

        app_log_min("\033[31m %s mode, aes128 3nd keyladder test\033[0m\n", parity_key == true ? "parity": "separate parity");
        DescKLMClass klm_info = {
            .klm_unit = {
                {//ek2
                    .alg = APP_DESC_KLM_AES,
                    .ek = {0},//thiz_aes128_3level_ek2,
                    .ek_bytes = sizeof(thiz_aes128_3level_ek2),
                },

                {//ek1
                    .alg = APP_DESC_KLM_AES,
                    .ek = {0},//thiz_aes128_3level_ek1,
                    .ek_bytes = sizeof(thiz_aes128_3level_ek1),
                },
            },
            .unit_count = 2,
        };

        memcpy(klm_info.klm_unit[0].ek, thiz_aes128_3level_ek2, klm_info.klm_unit[0].ek_bytes);
        memcpy(klm_info.klm_unit[1].ek, thiz_aes128_3level_ek1, klm_info.klm_unit[1].ek_bytes);

        if(true == parity_key)
        {
            GxDesc_SetECW(thiz_test_v, thiz_aes128_3level_ecwodd,
                    thiz_aes128_3level_ecweven, 16, APP_DESC_KLM_AES, thiz_root_index, klm_info);
            GxDesc_SetECW(thiz_test_a, thiz_aes128_3level_ecwodd,
                    thiz_aes128_3level_ecweven, 16, APP_DESC_KLM_AES, thiz_root_index, klm_info);
        }
        else
        {
            GxDesc_SetECWOdd(thiz_test_v, thiz_aes128_3level_ecwodd, 16,
                    APP_DESC_KLM_AES, thiz_root_index, klm_info);
            GxDesc_SetECWEven(thiz_test_v, thiz_aes128_3level_ecweven, 16,
                    APP_DESC_KLM_AES, thiz_root_index, klm_info);
            GxDesc_SetECWOdd(thiz_test_a, thiz_aes128_3level_ecwodd, 16,
                    APP_DESC_KLM_AES, thiz_root_index, klm_info);
            GxDesc_SetECWEven(thiz_test_a, thiz_aes128_3level_ecweven, 16,
                    APP_DESC_KLM_AES, thiz_root_index, klm_info);
        }
    }
    else if(flag == 3)
    {//TDES 2级KeyLadder

        app_log_min("\033[31m %s mode, tdes 2nd keyladder test\033[0m\n", parity_key == true ? "parity": "separate parity");
        DescKLMClass klm_info = {
            .klm_unit = {
                {//ek1
                    .alg = APP_DESC_KLM_3DES,
                    .ek = {0},//thiz_tdes_2level_ek1,
                    .ek_bytes = sizeof(thiz_tdes_2level_ek1),
                },
            },
            .unit_count = 1,
        };

        memcpy(klm_info.klm_unit[0].ek, thiz_tdes_2level_ek1, klm_info.klm_unit[0].ek_bytes);

        if(true == parity_key)
        {
            GxDesc_SetECW(thiz_test_v, thiz_tdes_2level_ecwodd,
                    thiz_tdes_2level_ecweven, 8, APP_DESC_KLM_3DES, thiz_root_index, klm_info);
            GxDesc_SetECW(thiz_test_a, thiz_tdes_2level_ecwodd,
                    thiz_tdes_2level_ecweven, 8, APP_DESC_KLM_3DES, thiz_root_index, klm_info);
        }
        else
        {
            GxDesc_SetECWOdd(thiz_test_v, thiz_tdes_2level_ecwodd,
                    8, APP_DESC_KLM_3DES, thiz_root_index, klm_info);
            GxDesc_SetECWEven(thiz_test_v, thiz_tdes_2level_ecweven,
                    8, APP_DESC_KLM_3DES, thiz_root_index, klm_info);
            GxDesc_SetECWOdd(thiz_test_a, thiz_tdes_2level_ecwodd,
                    8, APP_DESC_KLM_3DES, thiz_root_index, klm_info);
            GxDesc_SetECWEven(thiz_test_a, thiz_tdes_2level_ecweven,
                    8, APP_DESC_KLM_3DES, thiz_root_index, klm_info);
        }
    }
    else if(flag == 4)
    {//TDES 3级KeyLadder

        app_log_min("\033[31m %s mode, tdes 3nd keyladder test\033[0m\n", parity_key == true ? "parity": "separate parity");
        DescKLMClass klm_info = {
            .klm_unit = {
                 {//ek2
                    .alg = APP_DESC_KLM_3DES,
                    .ek = {0},//thiz_tdes_3level_ek2,
                    .ek_bytes = sizeof(thiz_tdes_3level_ek2),
                },

                {//ek1
                    .alg = APP_DESC_KLM_3DES,
                    .ek = {0},//thiz_tdes_3level_ek1,
                    .ek_bytes = sizeof(thiz_tdes_3level_ek1),
                },
            },
            .unit_count = 2,
        };

        memcpy(klm_info.klm_unit[0].ek, thiz_tdes_3level_ek2, klm_info.klm_unit[0].ek_bytes);
        memcpy(klm_info.klm_unit[1].ek, thiz_tdes_3level_ek1, klm_info.klm_unit[1].ek_bytes);

        if(true == parity_key)
        {
            GxDesc_SetECW(thiz_test_v, thiz_tdes_3level_ecwodd,
                    thiz_tdes_3level_ecweven, 8, APP_DESC_KLM_3DES, thiz_root_index, klm_info);
            GxDesc_SetECW(thiz_test_a, thiz_tdes_3level_ecwodd,
                    thiz_tdes_3level_ecweven, 8, APP_DESC_KLM_3DES, thiz_root_index, klm_info);
        }
        else
        {
            GxDesc_SetECWOdd(thiz_test_v, thiz_tdes_3level_ecwodd,
                    8, APP_DESC_KLM_3DES, thiz_root_index, klm_info);
            GxDesc_SetECWEven(thiz_test_v, thiz_tdes_3level_ecweven,
                    8, APP_DESC_KLM_3DES, thiz_root_index, klm_info);
            GxDesc_SetECWOdd(thiz_test_a, thiz_tdes_3level_ecwodd,
                    8, APP_DESC_KLM_3DES, thiz_root_index, klm_info);
            GxDesc_SetECWEven(thiz_test_a, thiz_tdes_3level_ecweven,
                    8, APP_DESC_KLM_3DES, thiz_root_index, klm_info);
        }
    }
}

// 内存加解密功能应用
// APP_M2M_OTP_SOFT：加解密的key来之chip otp
// APP_M2M_ACPU_SOFT：加解密的key来之soft key
// 下面函数使用AES128算法，测试16字节（全0数据）的解密和加密的功能。
void CATest_m2m(int32_t flag)
{
    unsigned char src[16] = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf};//原始数据
    unsigned char dst[16] = {0};//处理后数据空间
    unsigned char soft_key[16] = {0xf, 0xe, 0xd, 0xc, 0xb, 0xa, 0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0};
    int i = 0;

    DescM2MDataClass data = {
        .src = src,
        .in_bytes = 16,
        .dst = dst,
        .out_bytes = 16,
    };
    DescM2MAlgClass alg = {0};
    if(0 == flag)
    {
        alg.alg = APP_M2M_AES128;
        alg.opt = APP_M2M_ECB;
        alg.key_type = APP_M2M_OTP_SOFT;
        memset(&alg.keys, 0, sizeof(DescM2MKeyClass));
        app_log_min("\033[33m test aes128-ecb otp soft\033[0m\n");
    }
    else if(1 == flag)
    {
        alg.alg = APP_M2M_AES128;
        alg.opt = APP_M2M_ECB;
        alg.key_type = APP_M2M_ACPU_SOFT;
        memset(alg.keys.iv, 0, sizeof(alg.keys.iv));
        memcpy(alg.keys.soft_key, soft_key, 16);
        app_log_min("\033[33m test aes128-ecb acpu soft\033[0m\n");
    }
    else if(2 == flag)
    {
        alg.alg = APP_M2M_3DES;
        alg.opt = APP_M2M_ECB;
        alg.key_type = APP_M2M_OTP_SOFT;
        memset(&alg.keys, 0, sizeof(DescM2MKeyClass));
        app_log_min("\033[33m test 3des-ecb otp soft\033[0m\n");
    }
    else
    {
        alg.alg = APP_M2M_3DES;
        alg.opt = APP_M2M_ECB;
        alg.key_type = APP_M2M_ACPU_SOFT;
        memset(alg.keys.iv, 0, sizeof(alg.keys.iv));
        memcpy(alg.keys.soft_key, soft_key, 16);
        app_log_min("\033[33m test 3des-ecb acpu soft\033[0m\n");
    }
    GxM2M_Decrypt(data, alg);
    app_log_min("\nGxM2M_Decrypt\n");
    for(i = 0; i < data.out_bytes; i++)
    {
        app_log_min("%02x ", data.dst[i]);

    }
    app_log_min("\n");

    GxM2M_Encrypt(data, alg);
    app_log_min("\nGxM2M_Encrypt\n");
    for(i = 0; i < data.out_bytes; i++)
    {
        app_log_min("%02x ", data.dst[i]);

    }
    app_log_min("\n");
}

/*因为是验证select rootkey有没有效,只需随便选个Keyloadder的算法测下,当前选择AES128 2阶,选择index = 1的rootkey*/
void CATest_SelectRootKey(void)
{
#if (defined SIRIUS || defined CANOPUS || defined VEGA)
#if ACPU_GENERIC_USED
    unsigned char aes128_2level_ecwodd[16] = {0x90, 0x98, 0xcd, 0x7f, 0xff, 0x97, 0xc1, 0xd3, 0x94, 0x1a, 0x8a, 0xbe, 0x21, 0xab, 0x94, 0x41};
    unsigned char aes128_2level_ecweven[16] = {0x75, 0x1e, 0x05, 0xbf, 0xfb, 0x1b, 0xe7, 0x72, 0x7e, 0xd3, 0x0a, 0x40, 0x56, 0x89, 0x68, 0xd0};
#else
    unsigned char aes128_2level_ecwodd[16] = {0x32, 0x22, 0x07, 0x97, 0x89, 0x9f, 0xc3, 0xed, 0x82, 0xdd, 0x68, 0xbc, 0x68, 0x1b, 0xd5, 0x4a};
    unsigned char aes128_2level_ecweven[16] = {0xf5, 0xd5, 0xbc, 0xed, 0xef, 0xf6, 0x93, 0x70, 0xc9, 0x8c, 0xcb, 0x34, 0x93, 0x1b, 0xec, 0x62};
#endif
#elif ((defined TAURUS || defined CYGNUS || defined SCORPIO) && 0 == ACPU_GENERIC_USED)
    unsigned char aes128_2level_ecwodd[16] = {0x32, 0x22, 0x07, 0x97, 0x89, 0x9f, 0xc3, 0xed, 0x82, 0xdd, 0x68, 0xbc, 0x68, 0x1b, 0xd5, 0x4a};
    unsigned char aes128_2level_ecweven[16] = {0xf5, 0xd5, 0xbc, 0xed, 0xef, 0xf6, 0x93, 0x70, 0xc9, 0x8c, 0xcb, 0x34, 0x93, 0x1b, 0xec, 0x62};
#endif
#if (defined SIRIUS || defined TAURUS || defined CYGNUS || defined CANOPUS || defined VEGA || defined SCORPIO)
    if(thiz_test_v)
    {
        GxDesc_Close(thiz_test_v);
        thiz_test_v = 0;
    }
    if(thiz_test_a)
    {
        GxDesc_Close(thiz_test_a);
        thiz_test_v = 0;
    }
    thiz_test_v = GxDesc_Open(thiz_demux_id, thiz_stream_alg);
    thiz_test_a = GxDesc_Open(thiz_demux_id, thiz_stream_alg);

    GxDesc_SetStreamPID(thiz_test_v, thiz_v_pid);
    GxDesc_SetStreamPID(thiz_test_a, thiz_a_pid);

    app_log_min("\033[31m select rootkey1 test\033[0m\n");
    DescKLMClass klm_info = {
        .klm_unit = {
            {//eck
                .alg = APP_DESC_KLM_AES,
                .ek = {0},
                .ek_bytes = sizeof(thiz_aes128_2level_ek1),
            },
        },
        .unit_count = 1,
    };
    memcpy(klm_info.klm_unit[0].ek, thiz_aes128_2level_ek1, klm_info.klm_unit[0].ek_bytes);

    GxDesc_SetECW(thiz_test_v, aes128_2level_ecwodd, aes128_2level_ecweven, 16, APP_DESC_KLM_AES, 1, klm_info);
    GxDesc_SetECW(thiz_test_a, aes128_2level_ecwodd, aes128_2level_ecweven, 16, APP_DESC_KLM_AES, 1, klm_info);
    thiz_test_rootkey_select = true;
#endif
    return;
}

void CATest_DescClose(void)
{
    if(thiz_test_v)
    {
        if(true == thiz_test_rootkey_select)
            GxDescSelectRootkey(thiz_test_v, 0, thiz_root_index);
        GxDesc_Close(thiz_test_v);
        thiz_test_v = 0;
    }
    if(thiz_test_a)
    {
        if(true == thiz_test_rootkey_select)
            GxDescSelectRootkey(thiz_test_a, 0, thiz_root_index);
        GxDesc_Close(thiz_test_a);
        thiz_test_a = 0;
    }
    thiz_test_rootkey_select = false;
}
