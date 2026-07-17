#include "demux_ts.h"
#include "../avutil/common.h"
#include "../avutil/crc.h"
#include <sys/stat.h>

#include "bluray_pcm.h"
#include "iframe_probe.h"

static uint64_t ts_get_duration(GxDemuxer *d);
static int64_t ts_get_priv_timestamp(GxStream *stream, int pkt_size);

static int demux_ts_fill_buffer(GxDemuxer* demuxer,GxDemuxStream* ds);
static char debug_pmt_program = 0;

/*
 * we should parse pat table first
 * the pat table may far for some special stream
 * we can change parse size when pat talbe parsed for optimize reason
 */

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

#define SPS_SEARCH_RANGE (1024*1024*5)

#define SUB_FILTER_SIZE (64*1024 + 6)
#define FFALIGN(x, a) (((x)+(a)-1)&~((a)-1))

static PLAYER_INTERRUPT_CBK ts_interruptcbk = NULL;

#ifndef CONFIG_FFMPEG_ENABLE
static AVCRC gx_crc_table[AV_CRC_MAX][257] = {
    [AV_CRC_8_ATM] = {
        0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15, 0x38, 0x3F, 0x36, 0x31,
        0x24, 0x23, 0x2A, 0x2D, 0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
        0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D, 0xE0, 0xE7, 0xEE, 0xE9,
        0xFC, 0xFB, 0xF2, 0xF5, 0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
        0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85, 0xA8, 0xAF, 0xA6, 0xA1,
        0xB4, 0xB3, 0xBA, 0xBD, 0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
        0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA, 0xB7, 0xB0, 0xB9, 0xBE,
        0xAB, 0xAC, 0xA5, 0xA2, 0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
        0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32, 0x1F, 0x18, 0x11, 0x16,
        0x03, 0x04, 0x0D, 0x0A, 0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
        0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A, 0x89, 0x8E, 0x87, 0x80,
        0x95, 0x92, 0x9B, 0x9C, 0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
        0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC, 0xC1, 0xC6, 0xCF, 0xC8,
        0xDD, 0xDA, 0xD3, 0xD4, 0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
        0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44, 0x19, 0x1E, 0x17, 0x10,
        0x05, 0x02, 0x0B, 0x0C, 0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
        0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B, 0x76, 0x71, 0x78, 0x7F,
        0x6A, 0x6D, 0x64, 0x63, 0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
        0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13, 0xAE, 0xA9, 0xA0, 0xA7,
        0xB2, 0xB5, 0xBC, 0xBB, 0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
        0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB, 0xE6, 0xE1, 0xE8, 0xEF,
        0xFA, 0xFD, 0xF4, 0xF3, 0x01
    },
    [AV_CRC_8_EBU] = {
        0x00, 0x1D, 0x3A, 0x27, 0x74, 0x69, 0x4E, 0x53, 0xE8, 0xF5, 0xD2, 0xCF,
        0x9C, 0x81, 0xA6, 0xBB, 0xCD, 0xD0, 0xF7, 0xEA, 0xB9, 0xA4, 0x83, 0x9E,
        0x25, 0x38, 0x1F, 0x02, 0x51, 0x4C, 0x6B, 0x76, 0x87, 0x9A, 0xBD, 0xA0,
        0xF3, 0xEE, 0xC9, 0xD4, 0x6F, 0x72, 0x55, 0x48, 0x1B, 0x06, 0x21, 0x3C,
        0x4A, 0x57, 0x70, 0x6D, 0x3E, 0x23, 0x04, 0x19, 0xA2, 0xBF, 0x98, 0x85,
        0xD6, 0xCB, 0xEC, 0xF1, 0x13, 0x0E, 0x29, 0x34, 0x67, 0x7A, 0x5D, 0x40,
        0xFB, 0xE6, 0xC1, 0xDC, 0x8F, 0x92, 0xB5, 0xA8, 0xDE, 0xC3, 0xE4, 0xF9,
        0xAA, 0xB7, 0x90, 0x8D, 0x36, 0x2B, 0x0C, 0x11, 0x42, 0x5F, 0x78, 0x65,
        0x94, 0x89, 0xAE, 0xB3, 0xE0, 0xFD, 0xDA, 0xC7, 0x7C, 0x61, 0x46, 0x5B,
        0x08, 0x15, 0x32, 0x2F, 0x59, 0x44, 0x63, 0x7E, 0x2D, 0x30, 0x17, 0x0A,
        0xB1, 0xAC, 0x8B, 0x96, 0xC5, 0xD8, 0xFF, 0xE2, 0x26, 0x3B, 0x1C, 0x01,
        0x52, 0x4F, 0x68, 0x75, 0xCE, 0xD3, 0xF4, 0xE9, 0xBA, 0xA7, 0x80, 0x9D,
        0xEB, 0xF6, 0xD1, 0xCC, 0x9F, 0x82, 0xA5, 0xB8, 0x03, 0x1E, 0x39, 0x24,
        0x77, 0x6A, 0x4D, 0x50, 0xA1, 0xBC, 0x9B, 0x86, 0xD5, 0xC8, 0xEF, 0xF2,
        0x49, 0x54, 0x73, 0x6E, 0x3D, 0x20, 0x07, 0x1A, 0x6C, 0x71, 0x56, 0x4B,
        0x18, 0x05, 0x22, 0x3F, 0x84, 0x99, 0xBE, 0xA3, 0xF0, 0xED, 0xCA, 0xD7,
        0x35, 0x28, 0x0F, 0x12, 0x41, 0x5C, 0x7B, 0x66, 0xDD, 0xC0, 0xE7, 0xFA,
        0xA9, 0xB4, 0x93, 0x8E, 0xF8, 0xE5, 0xC2, 0xDF, 0x8C, 0x91, 0xB6, 0xAB,
        0x10, 0x0D, 0x2A, 0x37, 0x64, 0x79, 0x5E, 0x43, 0xB2, 0xAF, 0x88, 0x95,
        0xC6, 0xDB, 0xFC, 0xE1, 0x5A, 0x47, 0x60, 0x7D, 0x2E, 0x33, 0x14, 0x09,
        0x7F, 0x62, 0x45, 0x58, 0x0B, 0x16, 0x31, 0x2C, 0x97, 0x8A, 0xAD, 0xB0,
        0xE3, 0xFE, 0xD9, 0xC4, 0x01
    },
    [AV_CRC_16_ANSI] = {
        0x0000, 0x0580, 0x0F80, 0x0A00, 0x1B80, 0x1E00, 0x1400, 0x1180,
        0x3380, 0x3600, 0x3C00, 0x3980, 0x2800, 0x2D80, 0x2780, 0x2200,
        0x6380, 0x6600, 0x6C00, 0x6980, 0x7800, 0x7D80, 0x7780, 0x7200,
        0x5000, 0x5580, 0x5F80, 0x5A00, 0x4B80, 0x4E00, 0x4400, 0x4180,
        0xC380, 0xC600, 0xCC00, 0xC980, 0xD800, 0xDD80, 0xD780, 0xD200,
        0xF000, 0xF580, 0xFF80, 0xFA00, 0xEB80, 0xEE00, 0xE400, 0xE180,
        0xA000, 0xA580, 0xAF80, 0xAA00, 0xBB80, 0xBE00, 0xB400, 0xB180,
        0x9380, 0x9600, 0x9C00, 0x9980, 0x8800, 0x8D80, 0x8780, 0x8200,
        0x8381, 0x8601, 0x8C01, 0x8981, 0x9801, 0x9D81, 0x9781, 0x9201,
        0xB001, 0xB581, 0xBF81, 0xBA01, 0xAB81, 0xAE01, 0xA401, 0xA181,
        0xE001, 0xE581, 0xEF81, 0xEA01, 0xFB81, 0xFE01, 0xF401, 0xF181,
        0xD381, 0xD601, 0xDC01, 0xD981, 0xC801, 0xCD81, 0xC781, 0xC201,
        0x4001, 0x4581, 0x4F81, 0x4A01, 0x5B81, 0x5E01, 0x5401, 0x5181,
        0x7381, 0x7601, 0x7C01, 0x7981, 0x6801, 0x6D81, 0x6781, 0x6201,
        0x2381, 0x2601, 0x2C01, 0x2981, 0x3801, 0x3D81, 0x3781, 0x3201,
        0x1001, 0x1581, 0x1F81, 0x1A01, 0x0B81, 0x0E01, 0x0401, 0x0181,
        0x0383, 0x0603, 0x0C03, 0x0983, 0x1803, 0x1D83, 0x1783, 0x1203,
        0x3003, 0x3583, 0x3F83, 0x3A03, 0x2B83, 0x2E03, 0x2403, 0x2183,
        0x6003, 0x6583, 0x6F83, 0x6A03, 0x7B83, 0x7E03, 0x7403, 0x7183,
        0x5383, 0x5603, 0x5C03, 0x5983, 0x4803, 0x4D83, 0x4783, 0x4203,
        0xC003, 0xC583, 0xCF83, 0xCA03, 0xDB83, 0xDE03, 0xD403, 0xD183,
        0xF383, 0xF603, 0xFC03, 0xF983, 0xE803, 0xED83, 0xE783, 0xE203,
        0xA383, 0xA603, 0xAC03, 0xA983, 0xB803, 0xBD83, 0xB783, 0xB203,
        0x9003, 0x9583, 0x9F83, 0x9A03, 0x8B83, 0x8E03, 0x8403, 0x8183,
        0x8002, 0x8582, 0x8F82, 0x8A02, 0x9B82, 0x9E02, 0x9402, 0x9182,
        0xB382, 0xB602, 0xBC02, 0xB982, 0xA802, 0xAD82, 0xA782, 0xA202,
        0xE382, 0xE602, 0xEC02, 0xE982, 0xF802, 0xFD82, 0xF782, 0xF202,
        0xD002, 0xD582, 0xDF82, 0xDA02, 0xCB82, 0xCE02, 0xC402, 0xC182,
        0x4382, 0x4602, 0x4C02, 0x4982, 0x5802, 0x5D82, 0x5782, 0x5202,
        0x7002, 0x7582, 0x7F82, 0x7A02, 0x6B82, 0x6E02, 0x6402, 0x6182,
        0x2002, 0x2582, 0x2F82, 0x2A02, 0x3B82, 0x3E02, 0x3402, 0x3182,
        0x1382, 0x1602, 0x1C02, 0x1982, 0x0802, 0x0D82, 0x0782, 0x0202,
        0x0001
    },
    [AV_CRC_16_CCITT] = {
        0x0000, 0x2110, 0x4220, 0x6330, 0x8440, 0xA550, 0xC660, 0xE770,
        0x0881, 0x2991, 0x4AA1, 0x6BB1, 0x8CC1, 0xADD1, 0xCEE1, 0xEFF1,
        0x3112, 0x1002, 0x7332, 0x5222, 0xB552, 0x9442, 0xF772, 0xD662,
        0x3993, 0x1883, 0x7BB3, 0x5AA3, 0xBDD3, 0x9CC3, 0xFFF3, 0xDEE3,
        0x6224, 0x4334, 0x2004, 0x0114, 0xE664, 0xC774, 0xA444, 0x8554,
        0x6AA5, 0x4BB5, 0x2885, 0x0995, 0xEEE5, 0xCFF5, 0xACC5, 0x8DD5,
        0x5336, 0x7226, 0x1116, 0x3006, 0xD776, 0xF666, 0x9556, 0xB446,
        0x5BB7, 0x7AA7, 0x1997, 0x3887, 0xDFF7, 0xFEE7, 0x9DD7, 0xBCC7,
        0xC448, 0xE558, 0x8668, 0xA778, 0x4008, 0x6118, 0x0228, 0x2338,
        0xCCC9, 0xEDD9, 0x8EE9, 0xAFF9, 0x4889, 0x6999, 0x0AA9, 0x2BB9,
        0xF55A, 0xD44A, 0xB77A, 0x966A, 0x711A, 0x500A, 0x333A, 0x122A,
        0xFDDB, 0xDCCB, 0xBFFB, 0x9EEB, 0x799B, 0x588B, 0x3BBB, 0x1AAB,
        0xA66C, 0x877C, 0xE44C, 0xC55C, 0x222C, 0x033C, 0x600C, 0x411C,
        0xAEED, 0x8FFD, 0xECCD, 0xCDDD, 0x2AAD, 0x0BBD, 0x688D, 0x499D,
        0x977E, 0xB66E, 0xD55E, 0xF44E, 0x133E, 0x322E, 0x511E, 0x700E,
        0x9FFF, 0xBEEF, 0xDDDF, 0xFCCF, 0x1BBF, 0x3AAF, 0x599F, 0x788F,
        0x8891, 0xA981, 0xCAB1, 0xEBA1, 0x0CD1, 0x2DC1, 0x4EF1, 0x6FE1,
        0x8010, 0xA100, 0xC230, 0xE320, 0x0450, 0x2540, 0x4670, 0x6760,
        0xB983, 0x9893, 0xFBA3, 0xDAB3, 0x3DC3, 0x1CD3, 0x7FE3, 0x5EF3,
        0xB102, 0x9012, 0xF322, 0xD232, 0x3542, 0x1452, 0x7762, 0x5672,
        0xEAB5, 0xCBA5, 0xA895, 0x8985, 0x6EF5, 0x4FE5, 0x2CD5, 0x0DC5,
        0xE234, 0xC324, 0xA014, 0x8104, 0x6674, 0x4764, 0x2454, 0x0544,
        0xDBA7, 0xFAB7, 0x9987, 0xB897, 0x5FE7, 0x7EF7, 0x1DC7, 0x3CD7,
        0xD326, 0xF236, 0x9106, 0xB016, 0x5766, 0x7676, 0x1546, 0x3456,
        0x4CD9, 0x6DC9, 0x0EF9, 0x2FE9, 0xC899, 0xE989, 0x8AB9, 0xABA9,
        0x4458, 0x6548, 0x0678, 0x2768, 0xC018, 0xE108, 0x8238, 0xA328,
        0x7DCB, 0x5CDB, 0x3FEB, 0x1EFB, 0xF98B, 0xD89B, 0xBBAB, 0x9ABB,
        0x754A, 0x545A, 0x376A, 0x167A, 0xF10A, 0xD01A, 0xB32A, 0x923A,
        0x2EFD, 0x0FED, 0x6CDD, 0x4DCD, 0xAABD, 0x8BAD, 0xE89D, 0xC98D,
        0x267C, 0x076C, 0x645C, 0x454C, 0xA23C, 0x832C, 0xE01C, 0xC10C,
        0x1FEF, 0x3EFF, 0x5DCF, 0x7CDF, 0x9BAF, 0xBABF, 0xD98F, 0xF89F,
        0x176E, 0x367E, 0x554E, 0x745E, 0x932E, 0xB23E, 0xD10E, 0xF01E,
        0x0001
    },
    [AV_CRC_24_IEEE] = {
        0x000000, 0xFB4C86, 0x0DD58A, 0xF6990C, 0xE1E693, 0x1AAA15, 0xEC3319,
        0x177F9F, 0x3981A1, 0xC2CD27, 0x34542B, 0xCF18AD, 0xD86732, 0x232BB4,
        0xD5B2B8, 0x2EFE3E, 0x894EC5, 0x720243, 0x849B4F, 0x7FD7C9, 0x68A856,
        0x93E4D0, 0x657DDC, 0x9E315A, 0xB0CF64, 0x4B83E2, 0xBD1AEE, 0x465668,
        0x5129F7, 0xAA6571, 0x5CFC7D, 0xA7B0FB, 0xE9D10C, 0x129D8A, 0xE40486,
        0x1F4800, 0x08379F, 0xF37B19, 0x05E215, 0xFEAE93, 0xD050AD, 0x2B1C2B,
        0xDD8527, 0x26C9A1, 0x31B63E, 0xCAFAB8, 0x3C63B4, 0xC72F32, 0x609FC9,
        0x9BD34F, 0x6D4A43, 0x9606C5, 0x81795A, 0x7A35DC, 0x8CACD0, 0x77E056,
        0x591E68, 0xA252EE, 0x54CBE2, 0xAF8764, 0xB8F8FB, 0x43B47D, 0xB52D71,
        0x4E61F7, 0xD2A319, 0x29EF9F, 0xDF7693, 0x243A15, 0x33458A, 0xC8090C,
        0x3E9000, 0xC5DC86, 0xEB22B8, 0x106E3E, 0xE6F732, 0x1DBBB4, 0x0AC42B,
        0xF188AD, 0x0711A1, 0xFC5D27, 0x5BEDDC, 0xA0A15A, 0x563856, 0xAD74D0,
        0xBA0B4F, 0x4147C9, 0xB7DEC5, 0x4C9243, 0x626C7D, 0x9920FB, 0x6FB9F7,
        0x94F571, 0x838AEE, 0x78C668, 0x8E5F64, 0x7513E2, 0x3B7215, 0xC03E93,
        0x36A79F, 0xCDEB19, 0xDA9486, 0x21D800, 0xD7410C, 0x2C0D8A, 0x02F3B4,
        0xF9BF32, 0x0F263E, 0xF46AB8, 0xE31527, 0x1859A1, 0xEEC0AD, 0x158C2B,
        0xB23CD0, 0x497056, 0xBFE95A, 0x44A5DC, 0x53DA43, 0xA896C5, 0x5E0FC9,
        0xA5434F, 0x8BBD71, 0x70F1F7, 0x8668FB, 0x7D247D, 0x6A5BE2, 0x911764,
        0x678E68, 0x9CC2EE, 0xA44733, 0x5F0BB5, 0xA992B9, 0x52DE3F, 0x45A1A0,
        0xBEED26, 0x48742A, 0xB338AC, 0x9DC692, 0x668A14, 0x901318, 0x6B5F9E,
        0x7C2001, 0x876C87, 0x71F58B, 0x8AB90D, 0x2D09F6, 0xD64570, 0x20DC7C,
        0xDB90FA, 0xCCEF65, 0x37A3E3, 0xC13AEF, 0x3A7669, 0x148857, 0xEFC4D1,
        0x195DDD, 0xE2115B, 0xF56EC4, 0x0E2242, 0xF8BB4E, 0x03F7C8, 0x4D963F,
        0xB6DAB9, 0x4043B5, 0xBB0F33, 0xAC70AC, 0x573C2A, 0xA1A526, 0x5AE9A0,
        0x74179E, 0x8F5B18, 0x79C214, 0x828E92, 0x95F10D, 0x6EBD8B, 0x982487,
        0x636801, 0xC4D8FA, 0x3F947C, 0xC90D70, 0x3241F6, 0x253E69, 0xDE72EF,
        0x28EBE3, 0xD3A765, 0xFD595B, 0x0615DD, 0xF08CD1, 0x0BC057, 0x1CBFC8,
        0xE7F34E, 0x116A42, 0xEA26C4, 0x76E42A, 0x8DA8AC, 0x7B31A0, 0x807D26,
        0x9702B9, 0x6C4E3F, 0x9AD733, 0x619BB5, 0x4F658B, 0xB4290D, 0x42B001,
        0xB9FC87, 0xAE8318, 0x55CF9E, 0xA35692, 0x581A14, 0xFFAAEF, 0x04E669,
        0xF27F65, 0x0933E3, 0x1E4C7C, 0xE500FA, 0x1399F6, 0xE8D570, 0xC62B4E,
        0x3D67C8, 0xCBFEC4, 0x30B242, 0x27CDDD, 0xDC815B, 0x2A1857, 0xD154D1,
        0x9F3526, 0x6479A0, 0x92E0AC, 0x69AC2A, 0x7ED3B5, 0x859F33, 0x73063F,
        0x884AB9, 0xA6B487, 0x5DF801, 0xAB610D, 0x502D8B, 0x475214, 0xBC1E92,
        0x4A879E, 0xB1CB18, 0x167BE3, 0xED3765, 0x1BAE69, 0xE0E2EF, 0xF79D70,
        0x0CD1F6, 0xFA48FA, 0x01047C, 0x2FFA42, 0xD4B6C4, 0x222FC8, 0xD9634E,
        0xCE1CD1, 0x355057, 0xC3C95B, 0x3885DD, 0x000001,
    },
    [AV_CRC_32_IEEE] = {
        0x00000000, 0xB71DC104, 0x6E3B8209, 0xD926430D, 0xDC760413, 0x6B6BC517,
        0xB24D861A, 0x0550471E, 0xB8ED0826, 0x0FF0C922, 0xD6D68A2F, 0x61CB4B2B,
        0x649B0C35, 0xD386CD31, 0x0AA08E3C, 0xBDBD4F38, 0x70DB114C, 0xC7C6D048,
        0x1EE09345, 0xA9FD5241, 0xACAD155F, 0x1BB0D45B, 0xC2969756, 0x758B5652,
        0xC836196A, 0x7F2BD86E, 0xA60D9B63, 0x11105A67, 0x14401D79, 0xA35DDC7D,
        0x7A7B9F70, 0xCD665E74, 0xE0B62398, 0x57ABE29C, 0x8E8DA191, 0x39906095,
        0x3CC0278B, 0x8BDDE68F, 0x52FBA582, 0xE5E66486, 0x585B2BBE, 0xEF46EABA,
        0x3660A9B7, 0x817D68B3, 0x842D2FAD, 0x3330EEA9, 0xEA16ADA4, 0x5D0B6CA0,
        0x906D32D4, 0x2770F3D0, 0xFE56B0DD, 0x494B71D9, 0x4C1B36C7, 0xFB06F7C3,
        0x2220B4CE, 0x953D75CA, 0x28803AF2, 0x9F9DFBF6, 0x46BBB8FB, 0xF1A679FF,
        0xF4F63EE1, 0x43EBFFE5, 0x9ACDBCE8, 0x2DD07DEC, 0x77708634, 0xC06D4730,
        0x194B043D, 0xAE56C539, 0xAB068227, 0x1C1B4323, 0xC53D002E, 0x7220C12A,
        0xCF9D8E12, 0x78804F16, 0xA1A60C1B, 0x16BBCD1F, 0x13EB8A01, 0xA4F64B05,
        0x7DD00808, 0xCACDC90C, 0x07AB9778, 0xB0B6567C, 0x69901571, 0xDE8DD475,
        0xDBDD936B, 0x6CC0526F, 0xB5E61162, 0x02FBD066, 0xBF469F5E, 0x085B5E5A,
        0xD17D1D57, 0x6660DC53, 0x63309B4D, 0xD42D5A49, 0x0D0B1944, 0xBA16D840,
        0x97C6A5AC, 0x20DB64A8, 0xF9FD27A5, 0x4EE0E6A1, 0x4BB0A1BF, 0xFCAD60BB,
        0x258B23B6, 0x9296E2B2, 0x2F2BAD8A, 0x98366C8E, 0x41102F83, 0xF60DEE87,
        0xF35DA999, 0x4440689D, 0x9D662B90, 0x2A7BEA94, 0xE71DB4E0, 0x500075E4,
        0x892636E9, 0x3E3BF7ED, 0x3B6BB0F3, 0x8C7671F7, 0x555032FA, 0xE24DF3FE,
        0x5FF0BCC6, 0xE8ED7DC2, 0x31CB3ECF, 0x86D6FFCB, 0x8386B8D5, 0x349B79D1,
        0xEDBD3ADC, 0x5AA0FBD8, 0xEEE00C69, 0x59FDCD6D, 0x80DB8E60, 0x37C64F64,
        0x3296087A, 0x858BC97E, 0x5CAD8A73, 0xEBB04B77, 0x560D044F, 0xE110C54B,
        0x38368646, 0x8F2B4742, 0x8A7B005C, 0x3D66C158, 0xE4408255, 0x535D4351,
        0x9E3B1D25, 0x2926DC21, 0xF0009F2C, 0x471D5E28, 0x424D1936, 0xF550D832,
        0x2C769B3F, 0x9B6B5A3B, 0x26D61503, 0x91CBD407, 0x48ED970A, 0xFFF0560E,
        0xFAA01110, 0x4DBDD014, 0x949B9319, 0x2386521D, 0x0E562FF1, 0xB94BEEF5,
        0x606DADF8, 0xD7706CFC, 0xD2202BE2, 0x653DEAE6, 0xBC1BA9EB, 0x0B0668EF,
        0xB6BB27D7, 0x01A6E6D3, 0xD880A5DE, 0x6F9D64DA, 0x6ACD23C4, 0xDDD0E2C0,
        0x04F6A1CD, 0xB3EB60C9, 0x7E8D3EBD, 0xC990FFB9, 0x10B6BCB4, 0xA7AB7DB0,
        0xA2FB3AAE, 0x15E6FBAA, 0xCCC0B8A7, 0x7BDD79A3, 0xC660369B, 0x717DF79F,
        0xA85BB492, 0x1F467596, 0x1A163288, 0xAD0BF38C, 0x742DB081, 0xC3307185,
        0x99908A5D, 0x2E8D4B59, 0xF7AB0854, 0x40B6C950, 0x45E68E4E, 0xF2FB4F4A,
        0x2BDD0C47, 0x9CC0CD43, 0x217D827B, 0x9660437F, 0x4F460072, 0xF85BC176,
        0xFD0B8668, 0x4A16476C, 0x93300461, 0x242DC565, 0xE94B9B11, 0x5E565A15,
        0x87701918, 0x306DD81C, 0x353D9F02, 0x82205E06, 0x5B061D0B, 0xEC1BDC0F,
        0x51A69337, 0xE6BB5233, 0x3F9D113E, 0x8880D03A, 0x8DD09724, 0x3ACD5620,
        0xE3EB152D, 0x54F6D429, 0x7926A9C5, 0xCE3B68C1, 0x171D2BCC, 0xA000EAC8,
        0xA550ADD6, 0x124D6CD2, 0xCB6B2FDF, 0x7C76EEDB, 0xC1CBA1E3, 0x76D660E7,
        0xAFF023EA, 0x18EDE2EE, 0x1DBDA5F0, 0xAAA064F4, 0x738627F9, 0xC49BE6FD,
        0x09FDB889, 0xBEE0798D, 0x67C63A80, 0xD0DBFB84, 0xD58BBC9A, 0x62967D9E,
        0xBBB03E93, 0x0CADFF97, 0xB110B0AF, 0x060D71AB, 0xDF2B32A6, 0x6836F3A2,
        0x6D66B4BC, 0xDA7B75B8, 0x035D36B5, 0xB440F7B1, 0x00000001
    },
    [AV_CRC_32_IEEE_LE] = {
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
        0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
        0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
        0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
        0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
        0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
        0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
        0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
        0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
        0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
        0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
        0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
        0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
        0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
        0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
        0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
        0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
        0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
        0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
        0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
        0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
        0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
        0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
        0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
        0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
        0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
        0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
        0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
        0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
        0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
        0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
        0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
        0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
        0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
        0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
        0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
        0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
        0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
        0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
        0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
        0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
        0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
        0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D, 0x00000001
    },
    [AV_CRC_16_ANSI_LE] = {
        0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
        0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
        0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
        0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
        0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
        0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
        0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
        0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
        0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
        0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
        0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
        0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
        0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
        0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
        0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
        0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
        0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
        0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
        0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
        0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
        0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
        0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
        0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
        0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
        0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
        0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
        0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
        0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
        0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
        0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
        0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
        0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040,
        0x0001
    },
};

static AVCRC *gx_crc_get_table(AVCRCId crc_id)
{
    return gx_crc_table[crc_id];
}

uint32_t gx_crc(const AVCRC *ctx, uint32_t crc,
                const uint8_t *buffer, size_t length)
{
    const uint8_t *end = buffer + length;

    if (!ctx[256]) {
        while (((unsigned long) buffer & 3) && buffer < end)
            crc = ctx[((uint8_t) crc) ^ *buffer++] ^ (crc >> 8);

        while (buffer < end - 3) {
            crc ^= av_le2ne32(*(const uint32_t *) buffer); buffer += 4;
            crc = ctx[3 * 256 + ( crc        & 0xFF)] ^
                  ctx[2 * 256 + ((crc >> 8 ) & 0xFF)] ^
                  ctx[1 * 256 + ((crc >> 16) & 0xFF)] ^
                  ctx[0 * 256 + ((crc >> 24)       )];
        }
    }
    while (buffer < end)
        crc = ctx[((uint8_t) crc) ^ *buffer++] ^ (crc >> 8);

    return crc;
}

#endif

void* realloc_struct(void *ptr, size_t nmemb, size_t size)
{
	if (nmemb > SIZE_MAX / size) {
		av_free(ptr);
		return NULL;
	}
	return av_realloc(ptr, nmemb* size);

}

static int ts_parse(GxDemuxer* demuxer, ESStream * es, uint8_t * packet, int probe);
static int parse_mp4_descriptors(TSPMT* pmt, uint8_t * buf, int len, void *elem);
static uint16_t parse_mp4_decoder_config_descriptor(TSPMT* pmt, uint8_t * buf, int len, void *elem);
static inline int32_t progid_for_pid(DemuxTSPriv *priv, int pid, int32_t req);
static inline void pid_sub_from_pmt(DemuxTSPriv *priv, int32_t pid, int32_t* sub_num, GxStreamSubStream* sub_stream);
static inline void pid_extra_type_from_pmt(DemuxTSPriv *priv, int32_t pid, uint8_t* extra_type);
static inline uint8_t* pid_lang_from_pmt(DemuxTSPriv * priv, int pid);
static inline int32_t es_pid_in_pmt(TSPMT* pmt, uint16_t pid);

static void demux_avfifo_reset(GxDemuxer* demuxer)
{
	DemuxTSPriv* priv = demuxer->priv;

	memset(priv->fifo, 0, sizeof(priv->fifo));

	priv->fifo[0].ds = demuxer->audio;
	priv->fifo[1].ds = demuxer->video;
	priv->fifo[2].ds = demuxer->sub;
	priv->fifo[3].ds = demuxer->audio1;
	priv->fifo[0].buffer_size = MAX_PES_SIZE;
	priv->fifo[1].buffer_size = MAX_PES_SIZE;
	priv->fifo[2].buffer_size = MAX_PES_SIZE;
	priv->fifo[3].buffer_size = MAX_PES_SIZE;
	priv->fifo[0].flag = 0;
	priv->fifo[1].flag = 1;
	priv->fifo[2].flag = 2;
	priv->fifo[3].flag = 2;
}

static uint8_t ts_get_packet_size(const unsigned char* buf, int size)
{
	int i;

	if (size < (TS_FEC_PACKET_SIZE* NUM_CONSECUTIVE_TS_PACKETS))
		return 0;

	for (i = 0; i < NUM_CONSECUTIVE_TS_PACKETS; i++) {
		if (buf[i* TS_PACKET_SIZE] != 0x47) {
			goto try_fec;
		}
	}
	return TS_PACKET_SIZE;

try_fec:
	for (i = 0; i < NUM_CONSECUTIVE_TS_PACKETS; i++) {
		if (buf[i* TS_FEC_PACKET_SIZE] != 0x47) {
			goto try_philips;
		}
	}
	return TS_FEC_PACKET_SIZE;

try_philips:
	for (i = 0; i < NUM_CONSECUTIVE_TS_PACKETS; i++) {
		if (buf[i* TS_PH_PACKET_SIZE] != 0x47)
			return 0;
	}
	return TS_PH_PACKET_SIZE;
}

static int parse_avc_sps(uint8_t* buf, int len, int *w, int *h)
{
	int sps, sps_len;
	unsigned char* ptr;
	MpegHeader picture;
	if (len < 6)
		return 0;
	sps = buf[5] & 0x1f;
	if (!sps)
		return 0;
	sps_len = (buf[6] << 8) | buf[7];
	if (!sps_len || (sps_len > len - 8))
		return 0;
	ptr = &(buf[8]);
	picture.display_picture_width = picture.display_picture_height = 0;
	h264_parse_sps(&picture, ptr, len - 8);
	if (!picture.display_picture_width || !picture.display_picture_height)
		return 0;
	*w = picture.display_picture_width;
	*h = picture.display_picture_height;
	return 1;
}

static void ts_add_stream(GxDemuxer* demuxer, ESStream * es)
{
	DemuxTSPriv* priv = (DemuxTSPriv *) demuxer->priv;

	if (priv->ts.streams[es->pid].sh)
		return;

	if ((IS_SUBT(es->type)) && priv->last_sid + 1 < MAX_S_STREAMS) {
		GxStreamVideoHeader *sh_video=demuxer->video->sh;
		GxStreamVideoHeader *sh_audio=demuxer->audio->sh;
		if (sh_video) {
			int pv, ps;
			pv = progid_for_pid(demuxer->priv, sh_video->priv.id, 0);
			ps = progid_for_pid(demuxer->priv, es->pid, 0);
			if (pv != ps)
				return;
		}

		if (sh_audio) {
			int pa, ps;
			pa = progid_for_pid(demuxer->priv, sh_audio->priv.id, 0);
			ps = progid_for_pid(demuxer->priv, es->pid, 0);
			if (pa != ps)
				return;
		}

		GxStreamSubHeader* sh = GxStreamHeader_SubNew(demuxer, priv->last_sid, es->pid);
		if (sh) {
			char* lang = (char*)pid_lang_from_pmt(priv, es->pid);
			if(lang)
				strncpy(sh->priv.lang, lang, 4);
			if (es->type == SPU_DVB || es->type == SPU_DVD)
				sh->type   = SUB_CODEC_DVB_DESCRIPTOR;
			else if (es->type == SPU_TXT)
				sh->type   = SUB_CODEC_TXT_DESCRIPTOR;
			else if (es->type == SPU_SCTE)
				sh->type   = SUB_CODEC_SCTE;
			else if ((es->type == SPU_ARIB_A) || (es->type == SPU_ARIB_C))
				sh->type   = SUB_CODEC_ARIB;
			pid_sub_from_pmt(demuxer->priv, es->pid, &sh->priv.sub_num, &sh->priv.sub_stream[0]);
			priv->ts.streams[es->pid].id = priv->last_sid;
			priv->ts.streams[es->pid].sh = sh;
			priv->ts.streams[es->pid].type = TYPE_SUBT;
			priv->last_sid++;
		}
	}

	if ((IS_AUDIO(es->type) || IS_AUDIO(es->subtype)) && priv->last_aid + 1 < MAX_A_STREAMS) {
		GxStreamVideoHeader *sh_video=demuxer->video->sh;
		GxStreamAudioHeader* sh = NULL;
		uint8_t extra_type = 0;

		if (sh_video) {
			int pa, pv;
			pv = progid_for_pid(demuxer->priv, sh_video->priv.id, 0);
			pa = progid_for_pid(demuxer->priv, es->pid, 0);
			if (pv != pa)
				return;
		}

		pid_extra_type_from_pmt(priv, es->pid, &extra_type);

		if ((extra_type == EXTRA_VISUAL_IMPAIRED) && (priv->ts_a_pid != es->pid))
			sh = GxStreamHeader_Audio1New(demuxer, priv->last_aid, es->pid);
		else
			sh = GxStreamHeader_AudioNew(demuxer, priv->last_aid, es->pid);
		if (sh) {
			uint8_t* lang = NULL;

			if ((lang = pid_lang_from_pmt(priv, es->pid)) != NULL)
				strncpy(sh->priv.lang, (char*)lang, 4);
			sh->format = IS_AUDIO(es->type) ? es->type : es->subtype;
			if (es->type == AUDIO_AAC)
				sh->format = AUDIO_CODEC_AAC_LATM;
			else if (es->type == AUDIO_ADTS)
				sh->format = AUDIO_CODEC_AAC_ADTS;
			else if (es->type == AUDIO_OPUS)
				sh->format = AUDIO_CODEC_OPUS;
			else
				sh->format = acodec_fourcc2std(sh->format);

			if (extra_type == EXTRA_VISUAL_IMPAIRED)
				sh->ds = demuxer->audio1;
			else
				sh->ds = demuxer->audio;

			priv->ts.streams[es->pid].id = priv->last_aid;
			priv->ts.streams[es->pid].sh = sh;
			priv->ts.streams[es->pid].type = TYPE_AUDIO;
			priv->last_aid++;
		}

		if (es->extradata && es->extradata_len) {
			sh->wf = (WAVEFORMATEX* ) av_malloc(sizeof(WAVEFORMATEX) + es->extradata_len);
			sh->wf->cbSize = es->extradata_len;
			memcpy(sh->wf + 1, es->extradata, es->extradata_len);
		}
	}

	if ((IS_VIDEO(es->type) || IS_VIDEO(es->subtype)) && priv->last_vid + 1 < MAX_V_STREAMS) {
		GxStreamVideoHeader* sh = GxStreamHeader_VideoNew(demuxer, priv->last_vid, es->pid);
		if (sh) {
			uint8_t* lang = NULL;
			char *codec_name = NULL;

			if ((lang = pid_lang_from_pmt(priv, es->pid)) != NULL)
				strncpy(sh->priv.lang, (char*)lang, 4);
			sh->format = IS_VIDEO(es->type) ? es->type : es->subtype;
			codec_name = vcodec_get_fourcc2_name(sh->format);
			if (codec_name) {
				snprintf (sh->priv.codec, sizeof(sh->priv.codec)-1, "%s", codec_name);
			}
			sh->format = vcodec_fourcc2std(sh->format);
			sh->ds = demuxer->video;

			priv->ts.streams[es->pid].id = priv->last_vid;
			priv->ts.streams[es->pid].sh = sh;
			priv->ts.streams[es->pid].type = TYPE_VIDEO;
			priv->last_vid++;

			if (sh->format == VIDEO_CODEC_H264 && es->extradata && es->extradata_len) {
				int w = 0, h = 0;
				sh->bih = (BITMAPINFOHEADER* ) av_calloc(1, sizeof(BITMAPINFOHEADER) + es->extradata_len);
				sh->bih->biSize = sizeof(BITMAPINFOHEADER) + es->extradata_len;
				sh->bih->biCompression = sh->format;
				memcpy(sh->bih + 1, es->extradata, es->extradata_len);
				if (parse_avc_sps(es->extradata, es->extradata_len, &w, &h)) {
					sh->bih->biWidth = w;
					sh->bih->biHeight = h;
				}
			}
		}
	}
}

static int ts_check_file(GxDemuxer* demuxer)
{
	const int buf_size = (TS_FEC_PACKET_SIZE* NUM_CONSECUTIVE_TS_PACKETS);
	unsigned char *buf, done = 0, *ptr;
	uint32_t _read, i, count = 0, is_ts;
	int pid, cc_ok, c, good, bad, afc;
	int *cc, *last_cc;
	uint8_t size = 0, result =0;
	off_t pos = 0;
	off_t init_pos;

	buf = av_mallocz(buf_size);
	cc =  av_mallocz(sizeof(int) * NB_PID_MAX);
	last_cc = av_mallocz(sizeof(int) * NB_PID_MAX);

	if (buf == NULL || cc == NULL || last_cc == NULL)
		goto ret;

	GxPlayer_SystemGet(PSYS_CBK_INTERRUPT, &ts_interruptcbk);

	init_pos = GxStream_Tell(demuxer->stream);
	is_ts = 0;
	while (!done) {
		i = 1;
		c = 0;

		while (((c = GxStream_ReadChar(demuxer->stream)) != 0x47)
				&& (c >= 0)
				&& (i < MAX_CHECK_SIZE)
				&& !demuxer->stream->eof)
			i++;

		if (c != 0x47) {
			is_ts = 0;
			done = 1;
			continue;
		}

		pos = GxStream_Tell(demuxer->stream) - 1;
		buf[0] = c;
		_read = GxStream_Read(demuxer->stream, &buf[1], buf_size - 1);

		if (_read < buf_size - 1) {
			GxStream_Reset(demuxer->stream);
			goto ret;
		}

		size = ts_get_packet_size(buf, buf_size);
		if (size) {
			done = 1;
			is_ts = 1;
		}

		if (pos - init_pos >= TS_MAX_PROBE_SIZE) {
			done = 1;
			is_ts = 0;
		}
	}

	GxStream_Seek(demuxer->stream, pos);

	if (!is_ts)
		goto ret;

	//LET'S CHECK continuity counters
	good = bad = 0;
	for (count = 0; count < NB_PID_MAX; count++) {
		cc[count] = last_cc[count] = -1;
	}

	for (count = 0; count < NUM_CONSECUTIVE_TS_PACKETS; count++) {
		ptr = &(buf[size* count]);
		pid = ((ptr[1] & 0x1f) << 8) | ptr[2];

		if ((pid == NB_PID_MAX - 1) || (pid < 16))
			continue;

		afc = (ptr[3] >> 4) & 3;
		if (!(afc % 2))
			continue;

		cc[pid] = (ptr[3] & 0xf);
		cc_ok = (last_cc[pid] < 0) || ((((last_cc[pid] + 1) & 0x0f) == cc[pid]));
		if (!cc_ok)
			bad++;
		else
			good++;

		last_cc[pid] = cc[pid];
	}

	if (good >= bad)
		result = size;
ret:
	if (buf) av_free(buf);
	if (cc) av_free(cc);
	if (last_cc) av_free(last_cc);
	return result;
}


static inline void gxlogd_pmt_info(DemuxTSPriv *priv)
{
#define ES_TAGS(es_type, es_type_tag, es_code_tag) \
	switch (es->type) {         \
	case VIDEO_MPEG1:           \
		es_type_tag = "Video";  \
		es_code_tag = "Mpeg1";  \
		break;                  \
	case VIDEO_MPEG2:           \
		es_type_tag = "Video";  \
		es_code_tag = "Mpeg2";  \
		break;                  \
	case VIDEO_MPEG4:           \
		es_type_tag = "Video";  \
		es_code_tag = "Mpeg4";  \
		break;                  \
	case VIDEO_H264:            \
		es_type_tag = "Video";  \
		es_code_tag = "H264";   \
		break;                  \
	case VIDEO_AVC:             \
		es_type_tag = "Video";  \
		es_code_tag = "AVC1";   \
		break;                  \
	case VIDEO_VC1:             \
		es_type_tag = "Video";  \
		es_code_tag = "WVC1";   \
		break;                  \
	case VIDEO_AVS:             \
		es_type_tag = "Video";  \
		es_code_tag = "AVS";    \
		break;                  \
	case VIDEO_HEVC:            \
		es_type_tag = "Video";  \
		es_code_tag = "HEVC";   \
		break;                  \
	case AUDIO_MP2:             \
		es_type_tag = "Audio";  \
		es_code_tag = "Mpeg2";  \
		break;                  \
	case AUDIO_A52:             \
		es_type_tag = "Audio";  \
		es_code_tag = "Ac3";    \
		break;                  \
	case AUDIO_DTS:             \
		es_type_tag = "Audio";  \
		es_code_tag = "Dts";    \
		break;                  \
	case AUDIO_LPCM_BE:         \
		es_type_tag = "Audio";  \
		es_code_tag = "PCM";    \
		break;                  \
	case AUDIO_AAC:             \
		es_type_tag = "Audio";  \
		es_code_tag = "AAC";    \
		break;                  \
	case AUDIO_ADTS:            \
		es_type_tag = "Audio";  \
		es_code_tag = "ADTS";   \
		break;                  \
	case AUDIO_DRA1:            \
		es_type_tag = "Audio";  \
		es_code_tag = "DRA";    \
		break;                  \
	case AUDIO_OPUS:            \
		es_type_tag = "Audio";  \
		es_code_tag = "Opus";    \
	case SPU_DVD:               \
		es_type_tag = "Subtitle"; \
		es_code_tag = "DVD";      \
		break;                    \
	case SPU_DVB:                 \
		es_type_tag = "Subtitle"; \
		es_code_tag = "DVB";      \
		break;                    \
	case SPU_SCTE:                \
		es_type_tag = "Subtitle"; \
		es_code_tag = "SCTE";     \
		break;                    \
	case SPU_TXT:                 \
		es_type_tag = "Subtitle"; \
		es_code_tag = "TXT";      \
		break;                    \
	case SPU_ARIB_A:              \
		es_type_tag = "Subtitle"; \
		es_code_tag = "ARIB-A";   \
		break;                    \
	case SPU_ARIB_C:              \
		es_type_tag = "Subtitle"; \
		es_code_tag = "ARIB-C";   \
		break;                    \
	default:                      \
		es_type_tag = "Unknown";  \
		es_code_tag = "Unknown";  \
		break;                    \
	}
	int i = 0, j = 0, k = 0;
	char* es_type_tag = NULL;
	char* es_code_tag = NULL;
	char* compare_type_tag[4] = {"Video", "Audio", "Subtitle", "Unknown"};

	if (!debug_pmt_program) return;

	for (i = 0; i < priv->pmt_cnt; i++) {
		TSPMT *pmt = &priv->pmt[i];
		gxlogd("Program NO: %d\n", pmt->progid);
		for (k = 0; k < 4; k++) {
			for (j = 0; j < pmt->es_cnt; j++) {
				TSPMTES *es = &pmt->es[j];

				ES_TAGS(es->type, es_type_tag, es_code_tag);
				if (!strcmp(es_type_tag, compare_type_tag[k])) {
					if (es->type == SPU_TXT) {
						int l = 0;
						for (l = 0; l < es->sub_es_num; l++) {
							gxlogd("\t%s, pid 0x%x, %s, major: %x, minor: %x\n",
									es_type_tag,
									es->pid,
									(es->sub_es_stream[l].type == 0x01) ? "MAG" : "TXT",
									es->sub_es_stream[l].major,
									es->sub_es_stream[l].minor);
						}
					} else
						gxlogd("\t%s, pid 0x%x, %s\n", es_type_tag, es->pid, es_code_tag);
				}
			}
		}
	}

	for (k = 0; k < 4; k++) {
		ESStream* es = priv->ts.pids_hdr->next;
		while (es) {
			bool exist_flag = false;
			for (i = 0; i < priv->pmt_cnt; i++) {
				TSPMT *pmt = &priv->pmt[i];
				if (es_pid_in_pmt(pmt, es->pid) >= 0) {
					exist_flag = true;
					break;
				}
			}
			if (exist_flag == false) {
				ES_TAGS(es->type, es_type_tag, es_code_tag);
				if (!strcmp(es_type_tag, compare_type_tag[k])) {
					gxlogd("No Stream %s, pid 0x%x, %s\n", es_type_tag, es->pid, es_code_tag);
				}
			}
			es = es->next;
		}
	}
}

static inline int32_t progid_idx_in_pmt(DemuxTSPriv *priv, uint16_t progid)
{
	int x;

	if (priv->pmt == NULL)
		return -1;

	for (x = 0; x < priv->pmt_cnt; x++) {
		if (priv->pmt[x].progid == progid)
			return x;
	}

	return -1;
}

static inline int32_t progid_for_pid(DemuxTSPriv *priv, int pid, int32_t req)	//finds the first program listing a pid
{
	int i, j;
	TSPMT* pmt;

	if (priv->pmt == NULL)
		return -1;

	for (i = 0; i < priv->pmt_cnt; i++) {
		pmt = &(priv->pmt[i]);

		if (pmt->es == NULL)
			continue;

		for (j = 0; j < pmt->es_cnt; j++) {
			if (pmt->es[j].pid == pid) {
				if ((req == 0) || (req == pmt->progid))
					return pmt->progid;
			}
		}

	}
	return -1;
}

static inline int pid_match_lang(DemuxTSPriv *priv, uint16_t pid, char *lang)
{
	uint16_t i, j;
	TSPMT* pmt;

	if (priv->pmt == NULL)
		return -1;

	for (i = 0; i < priv->pmt_cnt; i++) {
		pmt = &(priv->pmt[i]);

		if (pmt->es == NULL)
			return -1;

		for (j = 0; j < pmt->es_cnt; j++) {
			if (pmt->es[j].pid != pid)
				continue;

			if (strncmp((char* )pmt->es[j].lang, lang, 3) == 0) {
				return 1;
			}
		}
	}
	return -1;
}
#ifdef A52_CHECK
//stripped down version of a52_syncinfo() from liba52
//copyright belongs to Michel Lespinasse <walken@zoy.org> and Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
static int mp_a52_framesize(uint8_t* buf, int *srate)
{
	int rate[] = { 32, 40, 48, 56, 64, 80, 96, 112,
		128, 160, 192, 224, 256, 320, 384, 448,
		512, 576, 640
	};
	uint8_t halfrate[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3 };
	int frmsizecod, bitrate, half;

	if ((buf[0] != 0x0b) || (buf[1] != 0x77))	/* syncword*/
		return 0;

	if (buf[5] >= 0x60)	/* bsid >= 12*/
		return 0;

	half = halfrate[buf[5] >> 3];

	frmsizecod = buf[4] & 63;
	if (frmsizecod >= 38)
		return 0;

	bitrate = rate[frmsizecod >> 1];

	switch (buf[4] & 0xc0) {
	case 0:		/* 48 KHz*/
		*srate = 48000 >> half;
		return 4* bitrate;
	case 0x40:		/* 44.1 KHz*/
		*srate = 44100 >> half;
		return 2* (320 * bitrate / 147 + (frmsizecod & 1));
	case 0x80:		/* 32 KHz*/
		*srate = 32000 >> half;
		return 6* bitrate;
	}
	return 0;
}

//second stage: returns the count of A52 syncwords found
static int a52_check(uint8_t* buf, int len)
{
	int cnt, frame_length, ok, srate;

	cnt = ok = 0;
	if (len < 8)
		return 0;

	while (cnt < len - 7) {
		if (buf[cnt] == 0x0B && buf[cnt + 1] == 0x77) {
			frame_length = mp_a52_framesize(&buf[cnt], &srate);
			if (frame_length >= 7 && frame_length <= 3840) {
				cnt += frame_length;
				ok++;
			} else
				cnt++;
		} else
			cnt++;
	}

	return ok;
}
#endif

static off_t ts_detect_streams(GxDemuxer* demuxer, TSDemuxInit * param)
{
	int num_packets = 0, req_apid, req_vpid, req_spid, req_ad_apid;
	int is_audio, is_video, is_sub, has_tables;
	int video_found = 0, audio_found = 0, sub_found = 0, ad_audio_found = 0;
	int32_t p, chosen_pid = 0;
	off_t pos = 0, ret = 0, init_pos, end_pos;
	ESStream es;
	unsigned char tmp[TS_FEC_PACKET_SIZE];
	DemuxTSPriv* priv = (DemuxTSPriv *) demuxer->priv;
#ifdef A52_CHECK
	int i;
	struct {
		uint8_t* buf;
		int pos;
	} pes_priv1[NB_PID_MAX],* pptr;
	uint8_t* tmpbuf;
#endif
	uint8_t extra_type = 0;

	priv->last_pid = NB_PID_MAX;

	req_apid = param->apid;
	req_vpid = param->vpid;
	req_spid = param->spid;
	req_ad_apid = param->ad_apid;
	if (req_ad_apid <= 0)
		ad_audio_found = 1;
	else
		ad_audio_found = 0;

	has_tables = 0;
#ifdef A52_CHECK
	memset(pes_priv1, 0, sizeof(pes_priv1));
#endif

	init_pos = GxStream_Tell(demuxer->stream);
	end_pos = init_pos + (param->probe ? param->probe : TS_MAX_PROBE_SIZE);
	while (1) {
		pos = GxStream_Tell(demuxer->stream);
		if (pos >= end_pos || demuxer->stream->eof) {
			break;
		}

		if (ts_parse(demuxer, &es, tmp, 1)) {
			//Non PES-aligned A52 audio may escape detection if PMT is not present;
			//in this case we try to find at least 3 A52 syncwords
#ifdef A52_CHECK
			if ((es.type == PES_PRIVATE1) && (!(*audio_found)) && req_apid > -2) {
				pptr = &pes_priv1[es.pid];
				if (pptr->pos < 64 * 1024) {
					tmpbuf = (uint8_t* ) av_realloc(pptr->buf, pptr->pos + es.size);
					if (tmpbuf != NULL) {
						pptr->buf = tmpbuf;
						memcpy(&(pptr->buf[pptr->pos]), es.start, es.size);
						pptr->pos += es.size;
						if (a52_check(pptr->buf, pptr->pos) > 2) {
							param->atype = AUDIO_A52;
							param->apid = es.pid;
							es.type = AUDIO_A52;
						}
					}
				}
			}
#endif

			is_audio = IS_AUDIO(es.type) || ((es.type == SL_PES_STREAM) && IS_AUDIO(es.subtype));
			is_video = IS_VIDEO(es.type) || ((es.type == SL_PES_STREAM) && IS_VIDEO(es.subtype));
			is_sub = IS_SUBT(es.type);

			extra_type = 0;
			if (is_audio)
				pid_extra_type_from_pmt(priv, es.pid, &extra_type);

			if ((priv->atype != UNKNOWN) && (priv->vtype != UNKNOWN)) {
				if ((priv->atype != UNKNOWN) && is_audio &&
						(extra_type != EXTRA_VISUAL_IMPAIRED)) {
					audio_found = 1;
					param->apid  = es.pid;
					param->atype = es.type;
				}

				if ((priv->vtype != UNKNOWN) && is_video) {
					video_found = 1;
					param->vpid  = es.pid;
					param->vtype = es.type;
				}

				if (audio_found && video_found)
					return init_pos;

				continue;
			}

			if ((!is_audio) && (!is_video) && (!is_sub))
				continue;
			if (is_audio && req_apid == -2)
				continue;

			if (is_video) {
				chosen_pid = (req_vpid == es.pid);
				if ((!chosen_pid) && (req_vpid > 0))
					continue;
			} else if (is_audio) {
				if (req_apid > 0) {
					chosen_pid = (req_apid == es.pid);
					if ((req_ad_apid > 0) && (req_ad_apid == es.pid))
						chosen_pid = 1;
					if (!chosen_pid)
						continue;
				}else if (param->alang[0] > 0 && es.lang[0] > 0) {
					if (pid_match_lang(priv, es.pid, param->alang) == -1)
						continue;

					chosen_pid = 1;
					param->apid = req_apid = es.pid;
					param->atype = es.type;
				}
			} else if (is_sub) {
				chosen_pid = (req_spid == es.pid);
				if ((!chosen_pid) && (req_spid > 0))
					continue;
			}

			if (req_apid < 0 && (param->alang[0] == 0) && req_vpid < 0 && req_spid < 0)
				chosen_pid = 1;

			if ((ret == 0) && chosen_pid) {
				ret = GxStream_Tell(demuxer->stream);
			}

			p = progid_for_pid(priv, es.pid, param->prog);
			if (p != -1)
				has_tables++;

			if ((param->prog == 0) && (p != -1)) {
				if (chosen_pid)
					param->prog = p;

				if (priv->pmt_cnt > 0) {
					int i = 0, j = 0;

					for (i = 0; i < priv->pmt_cnt; i++) {
						if (priv->pmt[i].progid == p) {
							j = i;
							break;
						}
					}

					if (j < priv->pmt_cnt) {
						if (is_video && ((req_vpid < 0) || (req_vpid == es.pid))) {
							param->vtype = IS_VIDEO(es.type) ? es.type : es.subtype;
							param->vpid = es.pid;
							video_found = 1;

							for (i = 0; i < priv->pmt[j].es_cnt; i++) {
								if (priv->pmt[j].es[i].pid != es.pid && priv->pmt[j].es[i].pid > 0) {
									if (IS_AUDIO(priv->pmt[j].es[i].type) &&
											((req_apid < 0) || (req_apid == priv->pmt[j].es[i].pid))) {
										param->atype = priv->pmt[j].es[i].type;
										param->apid = priv->pmt[j].es[i].pid;
										audio_found = 1;
										break;
									}
								}
							}

							/*
							 * check the a/v pid are in the same pmt table
							 */
							if (req_apid > 0 && req_vpid < 0 && audio_found == 0) {
								param->vtype = UNKNOWN;
								param->vpid = -1;
								video_found = 0;
							}
						}

						if (is_audio &&
								(extra_type != EXTRA_VISUAL_IMPAIRED) &&
								((req_apid < 0) || (req_apid == es.pid))) {
							param->atype = IS_AUDIO(es.type) ? es.type : es.subtype;
							param->apid = es.pid;
							audio_found = 1;

							for (i = 0; i < priv->pmt[j].es_cnt; i++) {
								if (priv->pmt[j].es[i].pid != es.pid && priv->pmt[j].es[i].pid > 0) {
									if (IS_VIDEO(priv->pmt[j].es[i].type) &&
											((req_vpid < 0) || (req_vpid == priv->pmt[j].es[i].pid))) {
										param->vtype = priv->pmt[j].es[i].type;
										param->vpid = priv->pmt[j].es[i].pid;
										video_found = 1;
										break;
									}
								}
							}

							/*
							 * check the a/v pid are in the same pmt table
							 */
							if (req_vpid > 0 && req_apid < 0 && video_found == 0) {
								param->atype = UNKNOWN;
								param->apid = -1;
								audio_found = 0;
							}
						}
					}
					if (audio_found || video_found) {
						priv->pmt_index = j;
						return 0;
					}
				}
			}

			if ((param->prog > 0) && (param->prog != p)) {
				if (audio_found) {
					if (is_video && (req_vpid == es.pid)) {
						param->vtype = IS_VIDEO(es.type) ? es.type : es.subtype;
						param->vpid = es.pid;
						video_found = 1;
						break;
					}
				}

				if (video_found) {
					if (is_audio &&
							(extra_type != EXTRA_VISUAL_IMPAIRED) &&
							(req_apid == es.pid)) {
						param->atype = IS_AUDIO(es.type) ? es.type : es.subtype;
						param->apid = es.pid;
						audio_found = 1;
					}

					if (is_audio &&
							(extra_type == EXTRA_VISUAL_IMPAIRED) &&
							(req_ad_apid == es.pid)) {
						param->ad_atype = IS_AUDIO(es.type) ? es.type : es.subtype;
						param->ad_apid  = es.pid;
						ad_audio_found = 1;
					}

					if (audio_found && ad_audio_found)
						break;
				}
				continue;
			}

			if (is_video) {
				if ((req_vpid == -1) || (req_vpid == es.pid)) {
					param->vtype = IS_VIDEO(es.type) ? es.type : es.subtype;
					param->vpid = es.pid;
					video_found = 1;
				}
			}

			if (((req_vpid == -2) || (num_packets >= NUM_CONSECUTIVE_AUDIO_PACKETS)) && audio_found
					&& !param->probe) {
				//novideo or we have at least 348 audio packets (64 KB) without video (TS with audio only)
				param->vtype = 0;
				break;
			}

			if (is_sub) {
				if ((req_spid == -1) || (req_spid == es.pid)) {
					param->stype = es.type;
					param->spid = es.pid;
					sub_found = 1;
				}
			}

			if (is_audio) {
				if ((extra_type != EXTRA_VISUAL_IMPAIRED) &&
						((req_apid == -1) ||(req_apid == es.pid))) {
					param->atype = IS_AUDIO(es.type) ? es.type : es.subtype;
					param->apid = es.pid;
					audio_found = 1;
				}else if ((extra_type == EXTRA_VISUAL_IMPAIRED) &&
						(req_ad_apid == es.pid)) {
					param->ad_atype = IS_AUDIO(es.type) ? es.type : es.subtype;
					param->ad_apid = es.pid;
					ad_audio_found = 1;
				}
			}

			if (audio_found && (param->apid == es.pid) && (!(video_found)))
				num_packets++;

			if (has_tables == 0 && (video_found && audio_found && ad_audio_found && pos >= end_pos)) {
				break;
			}
		}
		else {
			if (demuxer->stream->eof)
				break;
			return -1;
		}
	}

#ifdef A52_CHECK
	for (i = 0; i < NB_PID_MAX; i++) {
		if (pes_priv1[i].buf != NULL) {
			av_free(pes_priv1[i].buf);
			pes_priv1[i].buf = NULL;
			pes_priv1[i].pos = 0;
		}
	}
#endif

	if (!(video_found)) {
		param->vtype = UNKNOWN;
	}

	if (!(IS_AUDIO(param->atype))) {
		audio_found = 0;
		param->atype = UNKNOWN;
	}

	if (!(IS_SUBT(param->stype))) {
		param->stype = UNKNOWN;
	}

	if (video_found || audio_found) {
		if (demuxer->stream->eof && (ret == 0))
			ret = init_pos;
	}

	return ret;
}

static int mp4_parse_sl_packet(TSPMT* pmt, uint8_t * buf, uint16_t packet_len, int pid, ESStream * pes_es)
{
	int i, n, m, mp4_es_id = -1;
	uint64_t v = 0;
	uint32_t pl_size = 0;
	int deg_flag = 0;
	Mp4ESDescriptor* es = NULL;
	Mp4SlConfig* sl = NULL;
	uint8_t au_start = 0, au_end = 0, rap_flag = 0, ocr_flag = 0, padding = 0, padding_bits = 0, idle = 0;

	pes_es->is_synced = 0;

	if (!pmt || !packet_len)
		return 0;

	for (i = 0; i < pmt->es_cnt; i++) {
		if (pmt->es[i].pid == pid)
			mp4_es_id = pmt->es[i].mp4_es_id;
	}
	if (mp4_es_id < 0)
		return -1;

	for (i = 0; i < pmt->mp4es_cnt; i++) {
		if (pmt->mp4es[i].id == mp4_es_id)
			es = &(pmt->mp4es[i]);
	}
	if (!es)
		return -1;

	pes_es->subtype = es->decoder.object_type;

	sl = &(es->sl);
	if (!sl)
		return -1;

	n = 0;
	if (sl->au_start)
		pes_es->sl.au_start = au_start = getbits(buf, n++, 1);
	else
		pes_es->sl.au_start = (pes_es->sl.last_au_end ? 1 : 0);
	if (sl->au_end)
		pes_es->sl.au_end = au_end = getbits(buf, n++, 1);

	if (!sl->au_start && !sl->au_end) {
		pes_es->sl.au_start = pes_es->sl.au_end = au_start = au_end = 1;
	}
	pes_es->sl.last_au_end = pes_es->sl.au_end;

	if (sl->ocr_len > 0)
		ocr_flag = getbits(buf, n++, 1);
	if (sl->idle)
		idle = getbits(buf, n++, 1);
	if (sl->padding)
		padding = getbits(buf, n++, 1);
	if (padding) {
		padding_bits = getbits(buf, n, 3);
		n += 3;
	}

	if (idle || (padding && !padding_bits)) {
		pes_es->payload_size = 0;
		return -1;
	}
	//(! idle && (!padding || padding_bits != 0)) is true
	n += sl->packet_seqnum_len;
	if (sl->degr_len)
		deg_flag = getbits(buf, n++, 1);
	if (deg_flag)
		n += sl->degr_len;

	if (ocr_flag) {
		n += sl->ocr_len;
	}

	if (packet_len* 8 <= n)
		return -1;

	if (au_start) {
		int dts_flag = 0, cts_flag = 0, ib_flag = 0;

		if (sl->random_accesspoint)
			rap_flag = getbits(buf, n++, 1);

		//check commented because it seems it's rarely used, and we need this flag set in case of au_start
		//the decoder will eventually discard the payload if it can't decode it
		//if (rap_flag || sl->random_accesspoint_only)
		pes_es->is_synced = 1;

		n += sl->au_seqnum_len;
		if (packet_len* 8 <= n + 8)
			return -1;
		if (sl->use_ts) {
			dts_flag = getbits(buf, n++, 1);
			cts_flag = getbits(buf, n++, 1);
		}
		if (sl->instant_bitrate_len)
			ib_flag = getbits(buf, n++, 1);
		if (packet_len* 8 <= n + 8)
			return -1;
		if (dts_flag && (sl->ts_len > 0)) {
			n += sl->ts_len;
		}
		if (packet_len* 8 <= n + 8)
			return -1;
		if (cts_flag && (sl->ts_len > 0)) {
			int i = 0, m;

			while (i < sl->ts_len) {
				m = GXMIN(8, sl->ts_len - i);
				v |= getbits(buf, n, m);
				if (sl->ts_len - i > 8)
					v <<= 8;
				i += m;
				n += m;
				if (packet_len* 8 <= n + 8)
					return -1;
			}

			pes_es->pts = (float)v / (float)sl->ts_resolution;
		}

		i = 0;
		pl_size = 0;
		while (i < sl->au_len) {
			m = GXMIN(8, sl->au_len - i);
			pl_size |= getbits(buf, n, m);
			if (sl->au_len - i > 8)
				pl_size <<= 8;
			i += m;
			n += m;
			if (packet_len* 8 <= n + 8)
				return -1;
		}
		if (ib_flag)
			n += sl->instant_bitrate_len;
	}

	m = (n + 7) / 8;
	if (0 < pl_size && pl_size < pes_es->payload_size)
		pes_es->payload_size = pl_size;

	return m;
}

struct std_es {
	int std;
	int es;
};
static struct std_es vcodec_std_es[] = {
	{ VIDEO_CODEC_MPEG12,    VIDEO_MPEG2 },
	{ VIDEO_CODEC_MPEG4,     VIDEO_MPEG4 },
	{ VIDEO_CODEC_H264,      VIDEO_H264 },
	{ VIDEO_CODEC_AVS,       VIDEO_AVS },
	{ VIDEO_CODEC_H265,      VIDEO_HEVC },
	{ VIDEO_CODEC_DIV3,      VIDEO_MPEG4 },
	{-1, -1},
};

static int get_vcodec_from_priv(int type)
{
	int i = 0;
	while (vcodec_std_es[i].std != -1) {
		if (vcodec_std_es[i].std == type)
			return vcodec_std_es[i].es;
		i++;
	}

	return VIDEO_MPEG2;
}

static int pes_parse2(DemuxTSPriv* priv,unsigned char* buf, uint16_t packet_len, ESStream * es, int32_t type_from_pmt, TSPMT * pmt,
		int pid)
{
	unsigned char* p;
	uint32_t header_len;
	int64_t pts;
	uint32_t stream_id;
	uint32_t pkt_len, pes_is_aligned;

	if (packet_len == 0 || packet_len > 184) {
		return 0;
	}

	p = buf;
	pkt_len = packet_len;

	if (p[0] || p[1] || (p[2] != 1)) {
		return 0;
	}

	packet_len -= 6;
	if (packet_len == 0) {
		return 0;
	}

	es->payload_size = (p[4] << 8 | p[5]);
	es->extended_stream_id = -1;

	pes_is_aligned = (p[6] & 4);

	stream_id = p[3];

	if (p[7] & 0x80) {	/* pts available*/
		pts = (int64_t) (p[9] & 0x0E) << 29;
		pts |= p[10] << 22;
		pts |= (p[11] & 0xFE) << 14;
		pts |= p[12] << 7;
		pts |= (p[13] & 0xFE) >> 1;

		es->pts = pts / 90.0f;

		if (priv->ts_start_a_pts == -1 && pid == priv->ts_a_pid)
			priv->ts_start_a_pts = es->pts;
		if (priv->ts_start_v_pts == -1 && pid == priv->ts_v_pid)
			priv->ts_start_v_pts = es->pts;
	} else
		es->pts = 0.0f;

	if (1) {
		uint8_t flags = p[7];

		if (flags & 0x1) {
			unsigned char *r = p + 9;
			unsigned int pes_ext, skip;

			if ((flags & 0xC0) == 0x80)
				r += 5;//skip dts
			else if ((flags & 0xC0) == 0xC0)
				r += 10;//skip dts and pts

			pes_ext = *r++;
			/* Skip PES private data, program packet sequence counter and P-STD buffer */
			skip  = (pes_ext >> 4) & 0xb;
			skip += skip & 0x9;
			r    += skip;
			if ((pes_ext & 0x41) == 0x01 &&
					(r + 2) <= (p + pkt_len)) {
				/* PES extension 2 */
				if ((r[0] & 0x7f) > 0 && (r[1] & 0x80) == 0)
					es->extended_stream_id = r[1];
			}
		}
	}

	header_len = p[8];

	if (header_len + 9 > pkt_len)	//9 are the bytes read up to the header_length field
	{
		return 0;
	}

	if (header_len > 15) {
		unsigned char* ad = p + 15;
		int len = ad[0]&0xf;
		if ((len >=8) &&
				(ad[1] == 0x44) && (ad[2] == 0x54) &&
				(ad[3] == 0x47) && (ad[4] == 0x41) &&
				(ad[5] == 0x44))
		{
			es->ad_tags = 1;
			es->ad_fade_byte = ad[7];
			es->ad_pan_byte  = ad[8];
		}
	}

	p += header_len + 9;
	packet_len -= header_len + 3;

	if (es->payload_size)
		es->payload_size -= header_len + 3;

	es->is_synced = 1;	//only for SL streams we have to make sure it's really true, see below
	if (stream_id == 0xbd) {
		/*
		 * we check the descriptor tag first because some stations
		 * these "raw" streams may begin with a byte that looks like a stream type.
		 */

		if ((type_from_pmt == AUDIO_A52) ||	/* A52 - raw*/
				(p[0] == 0x0B && p[1] == 0x77)	/* A52 - syncword*/
		   ) {
			es->start = p;
			es->size = packet_len;
			es->type = AUDIO_A52;
			es->payload_size -= packet_len;

			return 1;
		}
		else if (type_from_pmt == AUDIO_DRA1) {
			es->start = p;
			es->size = packet_len;
			es->type = AUDIO_DRA1;
			es->payload_size -= packet_len;

			return 1;
		}
		/* SPU SUBS*/
		else if (type_from_pmt == SPU_DVB || ((p[0] == 0x20) && pes_is_aligned))	// && p[1] == 0x00))
		{
			es->start = p;
			es->size = packet_len;
			es->type = SPU_DVB;
			es->payload_size -= packet_len;

			return 1;
		} else if (pes_is_aligned && ((p[0] & 0xE0) == 0x20))	//SPU_DVD
		{
			//DVD SUBS
			es->start = p + 1;
			es->size = packet_len - 1;
			es->type = SPU_DVD;
			es->payload_size -= packet_len;

			return 1;
		} else if (pes_is_aligned && (p[0] & 0xF8) == 0x80) {
			es->start = p + 4;
			es->size = packet_len - 4;
			es->type = AUDIO_A52;
			es->payload_size -= packet_len;

			return 1;
		} else if (pes_is_aligned && ((p[0] & 0xf0) == 0xa0)) {
			int pcm_offset;

			for (pcm_offset = 0; ++pcm_offset < packet_len - 1;) {
				if (p[pcm_offset] == 0x01 && p[pcm_offset + 1] == 0x80) {	/* START*/
					pcm_offset += 2;
					break;
				}
			}

			es->start = p + pcm_offset;
			es->size = packet_len - pcm_offset;
			es->type = AUDIO_LPCM_BE;
			es->payload_size -= packet_len;
			if (priv->ts_start_a_pts == -1)
				priv->ts_start_a_pts = es->pts;

			return 1;
		} else {
			es->start = p;
			es->size = packet_len;
			es->type = (type_from_pmt == UNKNOWN ? PES_PRIVATE1 : type_from_pmt);
			es->payload_size -= packet_len;

			return 1;
		}
	} else if (((stream_id >= 0xe0) && (stream_id <= 0xef)) || (stream_id == 0xfd && type_from_pmt != UNKNOWN)) {
		es->start = p;
		es->size = packet_len;

		if (type_from_pmt != UNKNOWN)
			es->type = type_from_pmt;
		else
			es->type = (priv->vtype == UNKNOWN) ? get_vcodec_from_priv(priv->hw.ts_info.v_fmt): priv->vtype;

		if (es->payload_size)
			es->payload_size -= packet_len;

		return 1;
	} else if ((stream_id == 0xfa)) {
		int l;

		es->is_synced = 0;
		if (type_from_pmt != UNKNOWN)	//MP4 A/V or SL
		{
			es->start = p;
			es->size = packet_len;
			es->type = type_from_pmt;

			if (type_from_pmt == SL_PES_STREAM) {
				l = mp4_parse_sl_packet(pmt, p, packet_len, pid, es);
				if (l < 0) {
					l = 0;
				}

				es->start += l;
				es->size -= l;
			}

			if (es->payload_size)
				es->payload_size -= packet_len;
			return 1;
		}
	} else if ((stream_id & 0xe0) == 0xc0) {
		es->start = p;
		es->size = packet_len;

		if (type_from_pmt != UNKNOWN)
			es->type = type_from_pmt;
		else
			es->type = (priv->atype == UNKNOWN) ? AUDIO_MP2 : priv->atype;

		es->payload_size -= packet_len;

		return 1;
	} else if (type_from_pmt != -1) {
		es->start = p;
		es->size = packet_len;
		es->type = type_from_pmt;
		es->payload_size -= packet_len;

		return 1;
	}

	es->is_synced = 0;
	return 0;
}

static int ts_sync_N(GxStream* stream, unsigned int packet_size, int num, int flags)
{
	int c=0, synced=0, synccnt=0;
	off_t start_pos;
	off_t target_pos;

	start_pos = target_pos = GxStream_Tell(stream);
	while (synccnt < num)
	{
		c = GxStream_ReadChar(stream);
		if ((GxStream_Tell(stream) - start_pos >= 16*MAX_CHECK_SIZE) || stream->eof)
			return 0;

		if (c != 0x47) {
			target_pos++;
			if (synccnt != 0) GxStream_Seek(stream, target_pos);
			synccnt = 0;
			synced  = 0;
			if (flags) break;
		} else {
			if (synccnt != (num -1)) GxStream_Skip(stream, packet_size-1);
			synced = 1;
			synccnt++;
		}
	}
	GxStream_Seek(stream, target_pos);
	return synced;
}

static int fill_packet(GxDemuxer* demuxer, AVFIFO*avfifo, TSStreamInfo * si)
{
	int ret = 0;
	DemuxTSPriv* priv = (DemuxTSPriv *) demuxer->priv;
	GxDemuxPacket*dp = NULL;
	GxDemuxStream* ds = NULL;

	if (avfifo && avfifo->offset > 0) {
		ret = 1;
		GxStreamAudioHeader *sh_audio = NULL;
		if (avfifo->ds != NULL && avfifo->ds == demuxer->audio) {
			sh_audio = avfifo->ds->sh;
		}

		if (sh_audio && sh_audio->format == AUDIO_CODEC_PCM) {
			if (avfifo->offset < 4)
				goto ret;
			BlurayPcmHeader hdr;
			if (pcm_bluray_parse_header(&hdr, avfifo->buffer) < 0)
				goto ret;
			else {
				sh_audio->channels   = hdr.channels;
				sh_audio->big_endian = hdr.big_endian;
				sh_audio->samplerate = hdr.sample_rate;
				sh_audio->samplesize = hdr.sample_size;
			}

			dp = lpcm_swith_to_pcm(demuxer, &hdr, avfifo->buffer+4, avfifo->offset-4);
			if (dp) {
				dp->pts = avfifo->pts;
			}
			else {
				gxlogf("%s:#########CreatePacket Failed!\n", __func__);
				goto ret;
			}
			if (dp->pts != -1)
				dp->pts += demuxer->base_time;
			dp->pos = GxStream_Tell(demuxer->stream);
			GxDemuxStream_AddPacket(avfifo->ds, dp);
		}else{
			if (avfifo->extended_stream)
				goto ret;

			dp = GxDemuxPacket_Create(demuxer, NULL, avfifo->offset);
			if (dp) {
				dp->ctrlinfo.ad_tags      = avfifo->ad_tags;
				dp->ctrlinfo.ad_fade_byte = avfifo->ad_fade_byte;
				dp->ctrlinfo.ad_pan_byte  = avfifo->ad_pan_byte;
				dp->pts = avfifo->pts;
				GxDemuxPacket_Write(dp, avfifo->buffer, avfifo->offset);
			}
			else {
				gxlogf("%s:#########CreatePacket Failed!\n", __func__);
				goto ret;
			}
			if (dp->pts != -1)
				dp->pts += demuxer->base_time;
			dp->pos = GxStream_Tell(demuxer->stream);
			//gxlogd("^^^^^^^^^^^^^[%s]: pts =%10lld^^^^^^^^^^^^\n", sh_audio ? "a":"v", dp->pts);
			GxDemuxStream_AddPacket(avfifo->ds, dp);
		}

		if (!priv->time_insert && priv->ts_start_pts >= 0 && dp->pts != -1) {
			if (priv->ts_dur_pid == priv->ts_a_pid) {
				if (avfifo->ds == demuxer->audio)
					priv->last_pts = dp->pts;
			} else {
				if (avfifo->ds == demuxer->video)
					priv->last_pts = dp->pts;
			}
		} else {
			if (avfifo->ds == demuxer->audio)
				priv->audio_lastseekbytes += dp->write_size;
			else if (avfifo->ds == demuxer->video)
				priv->video_lastseekbytes += dp->write_size;
		}

		if (si) {
			int64_t diff = dp->pts - si->last_pts;
			int64_t dur;

			if (abs(diff) > 1)
			{
				si->duration += si->last_pts - si->first_pts;
				si->first_pts = si->last_pts = dp->pts;
			} else {
				si->last_pts = avfifo->pts;
			}
			si->size += ret;
			dur = si->duration + (si->last_pts - si->first_pts);

			if (dur > 0 && ds == demuxer->video) {
				DemuxTSPriv* priv = (DemuxTSPriv *) demuxer->priv;
				if (dur > 1)
					priv->vbitrate = (uint32_t) ((float)si->size / dur);
			}
		}
	}

ret:
	if (avfifo)
		avfifo->offset = 0;
	return ret;
}

static void ts_dump_streams(DemuxTSPriv* priv)
{
#if 0
	int i;
	AVFIFO* avfifo;

	for (i = 0; i < 3; i++) {
		if (priv->fifo[i].offset != 0) {
			avfifo = &priv->fifo[i];
			fill_packet(avfifo->ds->demuxer, avfifo, NULL);
		}
	}
#endif
}

static inline int32_t prog_idx_in_pat(DemuxTSPriv *priv, uint16_t progid)
{
	int x;

	if (priv->pat.progs == NULL)
		return -1;

	for (x = 0; x < priv->pat.progs_cnt; x++) {
		if (priv->pat.progs[x].id == progid)
			return x;
	}

	return -1;
}

static inline int32_t prog_id_in_pat(DemuxTSPriv *priv, uint16_t pid)
{
	int x;

	if (priv->pat.progs == NULL)
		return -1;

	for (x = 0; x < priv->pat.progs_cnt; x++) {
		if (priv->pat.progs[x].pmt_pid == pid)
			return priv->pat.progs[x].id;
	}

	return -1;
}

static int collect_section(TSSection* section, int is_start, unsigned char *buff, int size)
{
	uint8_t* ptr;
	uint16_t tlen;
	int skip, tid;

	if (!is_start && !section->buffer_len)
		return 0;

	if (is_start) {
		if (!section->buffer) {
			section->buffer = (uint8_t* ) av_malloc(4096 + 256);
			if (section->buffer == NULL)
				return 0;
		}
		section->buffer_len = 0;
	}

	if (size + section->buffer_len > 4096 + 256) {
		return 0;
	}

	memcpy(&(section->buffer[section->buffer_len]), buff, size);
	section->buffer_len += size;

	if (section->buffer_len < 3)
		return 0;

	skip = section->buffer[0];
	if (skip + 4 > section->buffer_len)
		return 0;

	ptr = &(section->buffer[skip + 1]);
	tid = ptr[0];
	tlen = ((ptr[1] & 0x0f) << 8) | ptr[2];
	if (section->buffer_len < (skip + 1 + 3 + tlen)) {
		return 0;
	}

	return skip + 1;
}

static int collect_pmt_section(TSSection* section, int is_start, unsigned char *buff, int size)
{
	if (is_start) {
		if (buff[0] > (size - 1))
			return 0;

		is_start = buff[0] ? 0 : 1;
		if (!is_start) {
			size = buff[0];
			buff++;
		}
	}

	return collect_section(section, is_start, buff, size);
}

static int parse_pat(DemuxTSPriv *priv, int is_start, unsigned char *buff, int size)
{
	int skip;
	unsigned char* ptr;
	unsigned char* base;
	int entries, i;
	uint16_t progid;
	struct pat_progs_t* tmp;
	TSSection* section;
	int32_t pmtpid = 0;

	section = &(priv->pat.section);
	skip = collect_section(section, is_start, buff, size);
	if (!skip)
		return 0;

	ptr = &(section->buffer[skip]);
	//PARSING
	priv->pat.table_id = ptr[0];
	if (priv->pat.table_id != 0)
		return 0;
	priv->pat.ssi = (ptr[1] >> 7) & 0x1;
	priv->pat.curr_next = ptr[5] & 0x01;
	priv->pat.ts_id = (ptr[3] << 8) | ptr[4];
	priv->pat.version_number = (ptr[5] >> 1) & 0x1F;
	priv->pat.section_length = ((ptr[1] & 0x03) << 8) | ptr[2];
	priv->pat.section_number = ptr[6];
	priv->pat.last_section_number = ptr[7];

	entries = (int)(priv->pat.section_length - 9) / 4;	//entries per section

	for (i = 0; i < entries; i++) {
		int32_t idx;
		base = &ptr[8 + i* 4];
		progid = (base[0] << 8) | base[1];
		pmtpid = ((base[2] & 0x1F) << 8) | base[3];

	//	if (pmtpid == 0x10)
	//		continue; //NIT PID

		if (progid == 0x0000)
			continue;

		if ((idx = prog_idx_in_pat(priv, progid)) == -1) {
			tmp = realloc_struct(priv->pat.progs, priv->pat.progs_cnt + 1, sizeof(struct pat_progs_t));
			if (tmp == NULL) {
				break;
			}
			priv->pat.progs = tmp;
			idx = priv->pat.progs_cnt;
			priv->pat.progs_cnt++;
		}

		priv->pat.progs[idx].id = progid;
		priv->pat.progs[idx].pmt_pid = pmtpid;
	}

	return 1;
}

static inline int32_t es_pid_in_pmt(TSPMT* pmt, uint16_t pid)
{
	uint16_t i;

	if (pmt == NULL)
		return -1;

	if (pmt->es == NULL)
		return -1;

	for (i = 0; i < pmt->es_cnt; i++) {
		if (pmt->es[i].pid == pid)
			return (int32_t) i;
	}

	return -1;
}

static uint16_t get_mp4_desc_len(uint8_t* buf, int *len)
{
	//uint16_t i = 0, size = 0;
	int i = 0, j, size = 0;

	j = GXMIN(*len, 4);
	while (i < j) {
		size |= (buf[i] & 0x7f);
		if (!(buf[i] & 0x80))
			break;
		size <<= 7;
		i++;
	}

	*len = i + 1;
	return size;
}

static uint16_t parse_mp4_slconfig_descriptor(uint8_t* buf, int len, void *elem)
{
	int i = 0;
	Mp4ESDescriptor* es;
	Mp4SlConfig* sl;

	es = (Mp4ESDescriptor* ) elem;
	if (!es) {
		return len;
	}
	sl = &(es->sl);

	sl->ts_len = sl->ocr_len = sl->au_len = sl->instant_bitrate_len = sl->degr_len = sl->au_seqnum_len =
		sl->packet_seqnum_len = 0;
	sl->ocr = sl->dts = sl->cts = 0;

	if (buf[0] == 0) {
		i++;
		sl->flags = buf[i];
		i++;
		sl->ts_resolution = (buf[i] << 24) | (buf[i + 1] << 16) | (buf[i + 2] << 8) | buf[i + 3];
		i += 4;
		sl->ocr_resolution = (buf[i] << 24) | (buf[i + 1] << 16) | (buf[i + 2] << 8) | buf[i + 3];
		i += 4;
		sl->ts_len = buf[i];
		i++;
		sl->ocr_len = buf[i];
		i++;
		sl->au_len = buf[i];
		i++;
		sl->instant_bitrate_len = buf[i];
		i++;
		sl->degr_len = (buf[i] >> 4) & 0x0f;
		sl->au_seqnum_len = ((buf[i] & 0x0f) << 1) | ((buf[i + 1] >> 7) & 0x01);
		i++;
		sl->packet_seqnum_len = ((buf[i] >> 2) & 0x1f);
		i++;

	} else if (buf[0] == 1) {
		sl->flags = 0;
		sl->ts_resolution = 1000;
		sl->ts_len = 32;
		i++;
	} else if (buf[0] == 2) {
		sl->flags = 4;
		i++;
	} else {
		sl->flags = 0;
		i++;
	}

	sl->au_start = (sl->flags >> 7) & 0x1;
	sl->au_end = (sl->flags >> 6) & 0x1;
	sl->random_accesspoint = (sl->flags >> 5) & 0x1;
	sl->random_accesspoint_only = (sl->flags >> 4) & 0x1;
	sl->padding = (sl->flags >> 3) & 0x1;
	sl->use_ts = (sl->flags >> 2) & 0x1;
	sl->idle = (sl->flags >> 1) & 0x1;
	sl->duration = sl->flags & 0x1;

	if (sl->duration) {
		sl->timescale = (buf[i] << 24) | (buf[i + 1] << 16) | (buf[i + 2] << 8) | buf[i + 3];
		i += 4;
		sl->au_duration = (buf[i] << 8) | buf[i + 1];
		i += 2;
		sl->cts_duration = (buf[i] << 8) | buf[i + 1];
		i += 2;
	} else			//no support for fixed durations atm
		sl->timescale = sl->au_duration = sl->cts_duration = 0;

	return len;
}

static uint16_t parse_mp4_decoder_config_descriptor(TSPMT* pmt, uint8_t * buf, int len, void *elem)
{
	int i = 0, j;
	Mp4ESDescriptor* es;
	Mp4DecoderConfig* dec;

	es = (Mp4ESDescriptor* ) elem;
	if (!es) {
		return len;
	}
	dec = (Mp4DecoderConfig* ) & (es->decoder);

	dec->object_type = buf[i];
	dec->stream_type = (buf[i + 1] >> 2) & 0x3f;

	if (dec->object_type == 1 && dec->stream_type == 1) {
		dec->object_type = MP4_OD;
		dec->stream_type = MP4_OD;
	} else if (dec->stream_type == 4) {
		if (dec->object_type == 0x6a)
			dec->object_type = VIDEO_MPEG1;
		if (dec->object_type >= 0x60 && dec->object_type <= 0x65)
			dec->object_type = VIDEO_MPEG2;
		else if (dec->object_type == 0x20)
			dec->object_type = VIDEO_MPEG4;
		else if (dec->object_type == 0x21)
			dec->object_type = VIDEO_AVC;
		/*else if (dec->object_type == 0x22)
		  gxlogf( "TYPE 0x22\n");*/
		else
			dec->object_type = UNKNOWN;
	} else if (dec->stream_type == 5) {
		if (dec->object_type == 0x40)
			dec->object_type = AUDIO_AAC;
		else if (dec->object_type == 0x6b)
			dec->object_type = AUDIO_MP2;
		else if (dec->object_type >= 0x66 && dec->object_type <= 0x69)
			dec->object_type = AUDIO_MP2;
		else
			dec->object_type = UNKNOWN;
	} else
		dec->object_type = dec->stream_type = UNKNOWN;

	if (dec->object_type != UNKNOWN) {
		//update the type of the current stream
		for (j = 0; j < pmt->es_cnt; j++) {
			if (pmt->es[j].mp4_es_id == es->id) {
				pmt->es[j].type = SL_PES_STREAM;
			}
		}
	}

	if (len > 13)
		parse_mp4_descriptors(pmt, &buf[13], len - 13, dec);

	return len;
}

static uint16_t parse_mp4_decoder_specific_descriptor(uint8_t* buf, int len, void *elem)
{
	Mp4DecoderConfig* dec;

	dec = (Mp4DecoderConfig* ) elem;
	if (!dec) {
		return len;
	}

	if (len > MAX_EXTRADATA_SIZE) {
		return len;
	}
	memcpy(dec->buf, buf, len);
	dec->buf_size = len;

	return len;
}

static uint16_t parse_mp4_es_descriptor(TSPMT* pmt, uint8_t * buf, int len)
{
	int i = 0, j = 0, k, found;
	uint8_t flag;
	Mp4ESDescriptor *es = av_mallocz(sizeof(Mp4ESDescriptor));
	Mp4ESDescriptor *target_es = NULL, *tmp;

	while (i < len) {
		es->id = (buf[i] << 8) | buf[i + 1];
		i += 2;
		flag = buf[i];
		i++;
		if (flag & 0x80)
			i += 2;
		if (flag & 0x40)
			i += buf[i] + 1;
		if (flag & 0x20)	//OCR, maybe we need it
			i += 2;

		j = parse_mp4_descriptors(pmt, &buf[i], len - i, es);
		if (es->decoder.object_type != UNKNOWN && es->decoder.stream_type != UNKNOWN) {
			found = 0;
			//search this ES_ID if we already have it
			for (k = 0; k < pmt->mp4es_cnt; k++) {
				if (pmt->mp4es[k].id == es->id) {
					target_es = &(pmt->mp4es[k]);
					found = 1;
				}
			}

			if (!found) {
				tmp = realloc_struct(pmt->mp4es, pmt->mp4es_cnt + 1, sizeof(Mp4ESDescriptor));
				if (tmp == NULL) {
					gxlogf("CAN'T REALLOC MP4_ES_DESCR\n");
					continue;
				}
				pmt->mp4es = tmp;
				target_es = &(pmt->mp4es[pmt->mp4es_cnt]);
				pmt->mp4es_cnt++;
			}
			memcpy(target_es, es, sizeof(Mp4ESDescriptor));
		}

		i += j;
	}

	av_free(es);
	return len;
}

static void parse_mp4_object_descriptor(TSPMT* pmt, uint8_t * buf, int len, void *elem)
{
	int i, j = 0, id;

	i = 0;
	id = (buf[0] << 2) | ((buf[1] & 0xc0) >> 6);
	if (buf[1] & 0x20) {
		i += buf[2] + 1;	//url
	} else {
		i = 2;

		while (i < len) {
			j = parse_mp4_descriptors(pmt, &(buf[i]), len - i, elem);
			i += j;
		}
	}
}

static void parse_mp4_iod(TSPMT* pmt, uint8_t * buf, int len, void *elem)
{
	int i, j = 0;
	Mp4Od* iod = &(pmt->iod);

	iod->id = (buf[0] << 2) | ((buf[1] & 0xc0) >> 6);
	i = 2;
	if (buf[1] & 0x20) {
		i += buf[2] + 1;	//url
	} else {
		i = 7;
		while (i < len) {
			j = parse_mp4_descriptors(pmt, &(buf[i]), len - i, elem);
			i += j;
		}
	}
}

static int parse_mp4_descriptors(TSPMT* pmt, uint8_t * buf, int len, void *elem)
{
	int tag, descr_len, i = 0, j = 0;

	if (!len)
		return len;

	while (i < len) {
		tag = buf[i];
		j = len - i - 1;
		descr_len = get_mp4_desc_len(&(buf[i + 1]), &j);

		if (descr_len > len - j + 1) {
			return len;
		}
		i += j + 1;

		switch (tag) {
		case 0x1:
			parse_mp4_object_descriptor(pmt, &(buf[i]), descr_len, elem);
			break;
		case 0x2:
			parse_mp4_iod(pmt, &(buf[i]), descr_len, elem);
			break;
		case 0x3:
			parse_mp4_es_descriptor(pmt, &(buf[i]), descr_len);
			break;
		case 0x4:
			parse_mp4_decoder_config_descriptor(pmt, &buf[i], descr_len, elem);
			break;
		case 0x05:
			parse_mp4_decoder_specific_descriptor(&buf[i], descr_len, elem);
			break;
		case 0x6:
			parse_mp4_slconfig_descriptor(&buf[i], descr_len, elem);
			break;
		default:
			break;
		}
		i += descr_len;
	}

	return len;
}

static ESStream* new_pid(DemuxTSPriv * priv, int pid)
{
	ESStream* tss =priv->ts.pids_hdr;

	while (tss)
	{
		if (tss->pid == pid)
			return tss;
		tss = tss->next;
	}

	tss = av_malloc(sizeof(ESStream));
	if (!tss)
		return NULL;
	memset(tss, 0, sizeof(ESStream));
	tss->pid = pid;
	tss->last_cc = -1;
	tss->type = UNKNOWN;
	tss->subtype = UNKNOWN;
	tss->is_synced = 0;
	tss->extradata = NULL;
	tss->extradata_alloc = tss->extradata_len = 0;
	tss->pts = tss->last_pts = GX_NOPTS_VALUE;
	tss->payload_size = 0;
	tss->next = NULL;

	priv->ts.pids_hdr->tail->next = tss;
	priv->ts.pids_hdr->tail = tss;

	return tss;
}

static int parse_program_descriptors(TSPMT* pmt, uint8_t * buf, uint16_t len)
{
	uint16_t i = 0, k, olen = len;

	while (len > 0) {
		if (buf[i + 1] > len - 2) {
			return olen;
		}

		if (buf[i] == 0x1d) {
			if (buf[i + 3] == 2)	//buggy versions of vlc muxer make this non-standard mess (missing iod_scope)
				k = 3;
			else
				k = 4;	//this is standard compliant
			parse_mp4_descriptors(pmt, &buf[i + k], (int)buf[i + 1] - (k - 2), NULL);
		}

		len -= 2 + buf[i + 1];
	}

	return olen;
}

static int parse_descriptors(struct pmt_es_t* es, uint8_t * ptr)
{
	int j, descr_len, len;

	j = 0;
	len = es->descr_length;
	while (len > 2) {
		descr_len = ptr[j + 1];
		if (descr_len > len) {
			return -1;
		}

		if (ptr[j] == 0x6a) {
			if (es->type == 0x6)
				es->type = AUDIO_A52;
		} else if (ptr[j] == 0x7a) { //A52 Descriptor
			if (es->type == 0x6)
				es->type = AUDIO_A53;
		} else if (ptr[j] == 0x7b) { //DVB DTS Descriptor
			if (es->type == 0x6) {
				es->type = AUDIO_DTS;
			}
		} else if (ptr[j] == 0xfd) { //ARIB CC Descriptor
			int arib_component_id = ((ptr[j + 2] << 8) | ptr[j + 3]);
			if (arib_component_id == 0x0008) {
				es->type = SPU_ARIB_A;
				es->sub_es_num++;
			} else if (arib_component_id == 0x0012) {
				es->type = SPU_ARIB_C;
				es->sub_es_num++;
			}
		} else if (ptr[j] == 0x56) {//Subtitling Txt Descriptor
			int lang_count = descr_len / 5;
			int k = 0 , s = j + 2;
			char lang[4];

			for (k = 0; k < lang_count; k++) {
				uint8_t r = 0;
				uint32_t type = ((ptr[s + 5*k + 3] >> 3) & 0x1f);
				uint16_t major = (ptr[s + 5*k + 3] & 0x7);
				uint16_t minor = (ptr[s + 5*k + 4]);

				memcpy(lang, s + ptr + 5 * k, 3);
				lang[3] = '\0';
				for (r = 0; r < es->sub_es_num; r++) {
					if ((es->sub_es_stream[r].major == major) &&
							(es->sub_es_stream[r].minor == minor) &&
							(strncmp((char*)es->sub_es_stream[r].lang, (char*)lang, 4) == 0))
							break;
				}

				if (r >= MAX_SUB_STREAM_NUM)
					break;

				if (r >= es->sub_es_num) {
					memcpy(es->sub_es_stream[r].lang, lang, 4);
					es->sub_es_stream[r].major = major;
					es->sub_es_stream[r].minor = minor;
					es->sub_es_stream[r].type  = type;
					es->sub_es_num++;
					gxlogf("%s: %d. des %x, des len %d, es pid %x, lang %s, major %x, minor %x\n",
							(type == 0x01) ? "dvb-magazine": "dvb-teletext",
							es->sub_es_num, ptr[j], descr_len, es->pid, es->sub_es_stream[r].lang,
							es->sub_es_stream[r].major, es->sub_es_stream[r].minor);
				}
			}
			es->type  = SPU_TXT;
		} else if (ptr[j] == 0x59)	//Subtitling Dvb Descriptor
		{
			if (descr_len >= 8) {
				if ((es->sub_es_num >= 0) && (es->sub_es_num < MAX_SUB_STREAM_NUM)) {
					int k;
					uint16_t major = (ptr[j+6]<<8)|(ptr[j+7]);
					uint16_t minor = (ptr[j+8]<<8)|(ptr[j+9]);
					uint8_t  lang[4];

					memcpy(lang, &ptr[j + 2], 3);
					lang[3] = 0;
					for (k = 0; k < es->sub_es_num; k++) {
						if ((es->sub_es_stream[k].major == major) &&
								(es->sub_es_stream[k].minor == minor) &&
								(strncmp((char*)es->sub_es_stream[k].lang, (char*)lang, 4) == 0))
							break;
					}
					if (k >= es->sub_es_num) {
						memcpy(es->sub_es_stream[k].lang, lang, 4);
						es->sub_es_stream[k].major = major;
						es->sub_es_stream[k].minor = minor;
						es->sub_es_num++;
						gxlogf("dvb-subtitle: %d, des %x, des len %d, es pid %x, lang %s, major %x, minor %x\n",
								es->sub_es_num, ptr[j], descr_len, es->pid, es->sub_es_stream[k].lang,
								es->sub_es_stream[k].major, es->sub_es_stream[k].minor);
					}
				}
				es->type  = SPU_DVB;
			}
		} else if (ptr[j] == 0x50)	//Component Descriptor
		{
			memcpy(es->lang, &ptr[j + 5], 3);
			es->lang[3] = 0;
		} else if (ptr[j] == 0x0a)	//Language Descriptor
		{
			if (descr_len >= 4) {
				memcpy(es->lang, &ptr[j + 2], 3);
				es->lang[3] = 0;
				if (ptr[j+5] == 0x02)
					es->extra_type = EXTRA_HEARING_IMPAIRED;
				else if (ptr[j+5] == 0x3)
					es->extra_type = EXTRA_VISUAL_IMPAIRED;
				else
					es->extra_type = EXTRA_CLEAN_EFFECTS;
				gxlogf("des_len %d, lang %s, %d\n", descr_len, es->lang, ptr[j+5]);
			}
		} else if (ptr[j] == 0x5)	//Registration Descriptor (looks like e fourCC :) )
		{
			if (descr_len >= 4) {
				uint8_t* d;
				memcpy(es->format_descriptor, &ptr[j + 2], 4);
				es->format_descriptor[4] = 0;

				d = &ptr[j + 2];
				if (d[0] == 'A' && d[1] == 'C' && d[2] == '-' && d[3] == '3') {
					es->type = AUDIO_A52;
				} else if (d[0] == 'e' && d[1] == 'c' && d[2] == '-' && d[3] == '3') {
					es->type = AUDIO_A53;
				} else if (d[0] == 'D' && d[1] == 'T' && d[2] == 'S' && d[3] == '1') {
					es->type = AUDIO_DTS;
				} else if (d[0] == 'D' && d[1] == 'T' && d[2] == 'S' && d[3] == '2') {
					es->type = AUDIO_DTS;
				} else if (d[0] == 'V' && d[1] == 'C' && d[2] == '-' && d[3] == '1') {
					es->type = VIDEO_VC1;
				} else if (d[0] == 'D' && d[1] == 'R' && d[2] == 'A' && d[3] == '1') {
					es->type = AUDIO_DRA1;
				} else if (d[0] == 'H' && d[1] == 'E' && d[2] == 'V' && d[3] == 'C') {
					es->type = VIDEO_HEVC;
				} else if (d[0] == 'O' && d[1] == 'p' && d[2] == 'u' && d[3] == 's') {
					es->type = AUDIO_OPUS;
				} else
					es->type = UNKNOWN;
			}
		} else if (ptr[j] == 0x1e) {
			es->mp4_es_id = (ptr[j + 2] << 8) | ptr[j + 3];
		}

		len -= 2 + descr_len;
		j += 2 + descr_len;
	}

	return 1;
}

static int parse_sl_section(TSPMT* pmt, TSSection * section, int is_start, unsigned char *buff, int size)
{
	int tid, len, skip;
	uint8_t* ptr;
	skip = collect_section(section, is_start, buff, size);
	if (!skip)
		return 0;

	ptr = &(section->buffer[skip]);
	tid = ptr[0];
	len = ((ptr[1] & 0x0f) << 8) | ptr[2];
	if (len > 4093 || section->buffer_len < len || tid != 5) {
		return 0;
	}

	if (!(ptr[5] & 1))
		return 0;

	//8 is the current position, len - 9 is the amount of data available
	parse_mp4_descriptors(pmt, &ptr[8], len - 9, NULL);

	return 1;
}

static int parse_pmt(DemuxTSPriv *priv,
		uint16_t progid, uint16_t pid, int is_start, unsigned char *buff, int size)
{
	unsigned char* base, *es_base, *section_base;
	TSPMT* pmt;
	int32_t idx, es_count, section_bytes;
	int skip, section_len;
	TSPMT* tmp;
	struct pmt_es_t* tmp_es;
	TSSection* section;
	ESStream* tss;

	idx = progid_idx_in_pmt(priv, progid);

	if (idx == -1) {
		tmp = realloc_struct(priv->pmt, priv->pmt_cnt + 1, sizeof(TSPMT));
		if (tmp == NULL) {
			return 0;
		}
		priv->pmt = tmp;
		idx = priv->pmt_cnt;
		memset(&(priv->pmt[idx]), 0, sizeof(TSPMT));
		priv->pmt_cnt++;
		priv->pmt[idx].progid = progid;
	}

	pmt = &(priv->pmt[idx]);
	if (pmt->find_prog_id)
		return 0;

	section = &(pmt->section);
	skip = collect_pmt_section(section, is_start, buff, size);
	if (skip == -1)
		return 0;

	section_base = &(section->buffer[skip]);
	section_len  = section->buffer_len - skip;
	if (section_base == NULL) {
		return -1;
	}
	while (!pmt->find_prog_id) {
		base = &section_base[section->buffer_len - skip - section_len];

		pmt->table_id = base[0];
		if (pmt->table_id != 0x02) {
			return -1;
		}

		progid = ((base[3] << 8) | base[4]);
		pmt->ssi = base[1] & 0x80;
		pmt->section_length = (((base[1] & 0xf) << 8) | base[2]);
		pmt->version_number = (base[5] >> 1) & 0x1f;
		pmt->curr_next = (base[5] & 1);
		pmt->section_number = base[6];
		pmt->last_section_number = base[7];
		pmt->PCR_PID = ((base[8] & 0x1f) << 8) | base[9];
		pmt->prog_descr_length = ((base[10] & 0xf) << 8) | base[11];
		if (pmt->prog_descr_length > pmt->section_length - 9) {
			return -1;
		}

		section_len -= (pmt->section_length + 3);
		if (section_len < 0)
			break;

		if (progid != pmt->progid)
			continue;

		pmt->find_prog_id = 1;
		if (pmt->prog_descr_length)
			parse_program_descriptors(pmt, &base[12], pmt->prog_descr_length);

		es_base = &base[12 + pmt->prog_descr_length];	//the beginning of th ES loop

		section_bytes = pmt->section_length - 13 - pmt->prog_descr_length;
		es_count = 0;

		while (section_bytes >= 5) {
			int es_pid, descr_length;
			unsigned char es_type;

			es_type = es_base[0];
			es_pid = ((es_base[1] & 0x1f) << 8) | es_base[2];

			descr_length = ((es_base[3] & 0xf) << 8) | es_base[4];
			if (descr_length > section_bytes - 5) {
				return -1;
			}

			idx = es_pid_in_pmt(pmt, es_pid);
			if (idx == -1) {
				tmp_es = realloc_struct(pmt->es, pmt->es_cnt + 1, sizeof(struct pmt_es_t));
				if (tmp_es == NULL) {
					continue;
				}
				pmt->es = tmp_es;
				idx = pmt->es_cnt;
				memset(&(pmt->es[idx]), 0, sizeof(struct pmt_es_t));
				pmt->es_cnt++;
			}

			pmt->es[idx].descr_length = descr_length;
			pmt->es[idx].pid = es_pid;
			if (es_type != 0x6)
				pmt->es[idx].type = UNKNOWN;
			else
				pmt->es[idx].type = es_type;

			parse_descriptors(&pmt->es[idx], &es_base[5]);

			pmt->es[idx].stream_type = es_type;
			switch (es_type) {
			case 1:
				pmt->es[idx].type = VIDEO_MPEG1;
				break;
			case 2:
				pmt->es[idx].type = VIDEO_MPEG2;
				break;
			case 3:
			case 4:
				pmt->es[idx].type = AUDIO_MP2;
				break;
			case 6:
				if (pmt->es[idx].type == 0x6)	//this could have been ovrwritten by parse_descriptors
					pmt->es[idx].type = UNKNOWN;
				break;
			case 0x10:
				pmt->es[idx].type = VIDEO_MPEG4;
				break;
			case 0x0f:
				pmt->es[idx].type = AUDIO_ADTS;
				break;
			case 0x11:
				pmt->es[idx].type = AUDIO_AAC;
				break;
			case 0x1b:
				pmt->es[idx].type = VIDEO_H264;
				break;
			case 0x12:
				pmt->es[idx].type = SL_PES_STREAM;
				break;
			case 0x13:
				pmt->es[idx].type = SL_SECTION;
				break;
			case 0x24:
				pmt->es[idx].type = VIDEO_HEVC;
				break;
			case 0x81:
			case 0x83:
			case 0x84:/* E-AC3  */
			case 0xA1: /* E-AC3 Secondary Audio */
				pmt->es[idx].type = AUDIO_A52;
				break;
			case 0x82:
				{
					int enable_scte = 0;
					GxPlayer_SystemGet(PSYS_ENABLE_SCTE, &enable_scte);
					if (enable_scte) {
						pmt->es[idx].type = SPU_SCTE;
						pmt->es[idx].sub_es_num++;
					} else
						pmt->es[idx].type = AUDIO_DTS;
					break;
				}
			case 0x85:/* DTS HD */
			case 0x86:/* DTS HD MASTER */
			case 0xA2:/* DTS Express Secondary Audio */
			case 0x8A:/* ATSC ? */
				pmt->es[idx].type = AUDIO_DTS;
				break;
			case 0x80:
				pmt->es[idx].type = AUDIO_LPCM_BE;/* PCM_BLURAY */
				break;
			case 0xEA:
				pmt->es[idx].type = VIDEO_VC1;
				break;
			case 0x42:
				pmt->es[idx].type = VIDEO_AVS;
				break;
			default:
				pmt->es[idx].type = UNKNOWN;
			}

			tss = new_pid(priv, es_pid);
			if (tss)
				tss->type = pmt->es[idx].type;

			section_bytes -= 5 + pmt->es[idx].descr_length;

			es_base += 5 + pmt->es[idx].descr_length;

			es_count++;
		}
	}

	return 1;
}

static TSPMT* pmt_of_pid(DemuxTSPriv * priv, int pid, Mp4DecoderConfig ** mp4_dec)
{
	int32_t i, j, k;

	if (priv->pmt) {
		for (i = 0; i < priv->pmt_cnt; i++) {
			if (priv->pmt[i].es && priv->pmt[i].es_cnt) {
				for (j = 0; j < priv->pmt[i].es_cnt; j++) {
					if (priv->pmt[i].es[j].pid == pid) {
						//search mp4_es_id
						if (priv->pmt[i].es[j].mp4_es_id) {
							for (k = 0; k < priv->pmt[i].mp4es_cnt; k++) {
								if (priv->pmt[i].mp4es[k].id == priv->pmt[i].es[j].mp4_es_id) {
									*mp4_dec = &(priv->pmt[i].mp4es[k].decoder);
									break;
								}
							}
						}
						return &(priv->pmt[i]);
					}
				}
			}
		}
	}

	return NULL;
}

static inline int32_t pid_type_from_pmt(DemuxTSPriv *priv, int pid)
{
	int32_t pmt_idx, pid_idx, i, j;

	pmt_idx = progid_idx_in_pmt(priv, priv->prog);

	if (pmt_idx != -1) {
		pid_idx = es_pid_in_pmt(&(priv->pmt[pmt_idx]), pid);
		if (pid_idx != -1)
			return priv->pmt[pmt_idx].es[pid_idx].type;
	}
	for (i = 0; i < priv->pmt_cnt; i++) {
		TSPMT* pmt = &(priv->pmt[i]);
		for (j = 0; j < pmt->es_cnt; j++)
			if (pmt->es[j].pid == pid)
				return pmt->es[j].type;
	}

	return UNKNOWN;
}

static inline void pid_sub_from_pmt(DemuxTSPriv *priv, int32_t pid, int32_t* sub_num, GxStreamSubStream* sub_stream)
{
	int32_t pmt_idx, pid_idx, i, j;
	struct pmt_es_t *pmt_es = NULL;

	pmt_idx = progid_idx_in_pmt(priv, priv->prog);

	if (pmt_idx != -1) {
		pid_idx = es_pid_in_pmt(&(priv->pmt[pmt_idx]), pid);
		if (pid_idx != -1) {
			pmt_es   = &priv->pmt[pmt_idx].es[pid_idx];
			*sub_num = pmt_es->sub_es_num;
			memcpy(sub_stream, pmt_es->sub_es_stream, MAX_SUB_STREAM_NUM*sizeof(GxStreamSubStream));
			return;
		}
	}
	for (i = 0; i < priv->pmt_cnt; i++) {
		TSPMT* pmt = &(priv->pmt[i]);
		for (j = 0; j < pmt->es_cnt; j++)
			if (pmt->es[j].pid == pid) {
				pmt_es   = &pmt->es[j];
				*sub_num = pmt_es->sub_es_num;
				memcpy(sub_stream, pmt_es->sub_es_stream, MAX_SUB_STREAM_NUM*sizeof(GxStreamSubStream));
				return;
			}
	}

	return;
}

static inline void pid_extra_type_from_pmt(DemuxTSPriv *priv, int32_t pid, uint8_t* extra_type)
{
	int32_t pmt_idx, pid_idx, i, j;

	pmt_idx = progid_idx_in_pmt(priv, priv->prog);

	if (pmt_idx != -1) {
		pid_idx = es_pid_in_pmt(&(priv->pmt[pmt_idx]), pid);
		if (pid_idx != -1) {
			*extra_type = priv->pmt[pmt_idx].es[pid_idx].extra_type;
			return;
		}
	}
	for (i = 0; i < priv->pmt_cnt; i++) {
		TSPMT* pmt = &(priv->pmt[i]);
		for (j = 0; j < pmt->es_cnt; j++)
			if (pmt->es[j].pid == pid) {
				*extra_type = pmt->es[j].extra_type;
				return;
			}
	}

	return;
}

static inline uint8_t* pid_lang_from_pmt(DemuxTSPriv * priv, int pid)
{
	int32_t pmt_idx, pid_idx, i, j;

	pmt_idx = progid_idx_in_pmt(priv, priv->prog);

	if (pmt_idx != -1) {
		pid_idx = es_pid_in_pmt(&(priv->pmt[pmt_idx]), pid);
		if (pid_idx != -1)
			return priv->pmt[pmt_idx].es[pid_idx].lang;
	} else {
		for (i = 0; i < priv->pmt_cnt; i++) {
			TSPMT* pmt = &(priv->pmt[i]);
			for (j = 0; j < pmt->es_cnt; j++)
				if (pmt->es[j].pid == pid)
					return pmt->es[j].lang;
		}
	}

	return NULL;
}

static inline uint8_t pid_stream_type_from_pmt(DemuxTSPriv * priv, int pid)
{
	int32_t pmt_idx, pid_idx, i, j;

	pmt_idx = progid_idx_in_pmt(priv, priv->prog);

	if (pmt_idx != -1) {
		pid_idx = es_pid_in_pmt(&(priv->pmt[pmt_idx]), pid);
		if (pid_idx != -1)
			return priv->pmt[pmt_idx].es[pid_idx].stream_type;
	} else {
		for (i = 0; i < priv->pmt_cnt; i++) {
			TSPMT* pmt = &(priv->pmt[i]);
			for (j = 0; j < pmt->es_cnt; j++)
				if (pmt->es[j].pid == pid)
					return pmt->es[j].stream_type;
		}
	}

	return 0;
}

static int fill_extradata(Mp4DecoderConfig* mp4_dec, ESStream * tss)
{
	uint8_t* tmp;

	if (mp4_dec->buf_size > tss->extradata_alloc) {
		tmp = (uint8_t* ) av_realloc(tss->extradata, mp4_dec->buf_size);
		if (!tmp)
			return 0;
		tss->extradata = tmp;
		tss->extradata_alloc = mp4_dec->buf_size;
	}
	memcpy(tss->extradata, mp4_dec->buf, mp4_dec->buf_size);
	tss->extradata_len = mp4_dec->buf_size;

	return tss->extradata_len;
}

static int ts_filter_pid(GxStream* stream, DemuxTSPriv* priv, int pid, uint8_t* packet, int junk)
{
	int i;
	GxFifo* pes_fifo = NULL;
	GxFifo* sub_fifo = NULL;

	for (i = 0; i < PLAYER_FILTER_MAX; i++) {
		if (priv->filter.data[i].using && (priv->filter.data[i].pid == pid)) {
			pes_fifo = priv->filter.fifo;
			break;
		}
	}

	if (VAILD_PID(priv->sub_filter.pid) && (pid == priv->sub_filter.pid))
		sub_fifo = priv->sub_filter.fifo;

	if (pes_fifo || sub_fifo) {
		int fifo_cap, fifo_len;
		uint8_t buffer[TS_PACKET_SIZE];

		buffer[0] = 0x47;
		buffer[1] = packet[1];
		buffer[2] = packet[2];
		buffer[3] = packet[3];
		GxStream_Read(stream, &buffer[4], TS_PACKET_SIZE-4);

		if (pes_fifo) {
			fifo_cap = GxFifo_GetCap(pes_fifo);
			fifo_len = GxFifo_GetLength(pes_fifo);
			if ((fifo_cap - fifo_len) >= TS_PACKET_SIZE)
				GxFifo_Write(pes_fifo, buffer, TS_PACKET_SIZE, -1);
		}

		if (sub_fifo) {
			fifo_cap = GxFifo_GetCap(sub_fifo);
			fifo_len = GxFifo_GetLength(sub_fifo);
			if ((fifo_cap - fifo_len) >= TS_PACKET_SIZE)
				GxFifo_Write(sub_fifo, buffer, TS_PACKET_SIZE, -1);
		}

		if (junk > 0)
			GxStream_Skip(stream, junk);
		return 0;
	}

	return -1;
}

static int external_mpegts_descramble_callback(unsigned char *data_in, unsigned int dataInLen, unsigned char *dataOut, unsigned int dataOutLen)
{
	extern GxStreamMpegtsCallback public_stream_mpegts_callback;
	if (public_stream_mpegts_callback) {
		GxStreamMpegtsCallbackParam param;
		param.dataIn     = data_in;
		param.dataInLen  = dataInLen;
		param.dataOut     = dataOut;
		param.dataOutLen = dataOutLen;
		if (public_stream_mpegts_callback(&param) != 0) {
			gxloge("mpegts descraamble error.\n");
			return -1;
		}
		return 0;
	}
	memcpy(dataOut, data_in, dataOutLen);
	return 0;
}

/*
 * ts_detect_streams functions is ugly for getting vpid/apid accuratly(multi programs):
 * 1:vpid and apid are't in a same pmt sometimes
 * 2:vpid/apid type may be wrong sometimes
 * this function can get the right pat/pmt usually for ts_detect_streams working well
 */
static int ts_prepare_detect(GxDemuxer* demuxer)
{
	int ret = 0;
	off_t pos = 0, init_pos, end_pos;
	int buf_size, is_start, pid, base;
	int len, afc;
	DemuxTSPriv* priv = (DemuxTSPriv *) demuxer->priv;
	GxStream* stream = demuxer->stream;
	uint8_t* p = NULL;
	int32_t progid, bad, ts_error;
	int junk = 0, rap_flag = 0;
	unsigned char packet[TS_FEC_PACKET_SIZE] ={0}, tmp_packet[TS_FEC_PACKET_SIZE] = {0};
	int timeoutms = demuxer->stream->read_timeoutms;

	init_pos = GxStream_Tell(demuxer->stream);
	end_pos = init_pos + TS_MAX_PROBE_SIZE;
	demuxer->stream->read_timeoutms = 0;

	while (1) {
		pos = GxStream_Tell(demuxer->stream);
		if (pos >= end_pos || demuxer->stream->eof) {
			break;
		}

		bad = ts_error = 0;
		rap_flag = 0;
		if (ts_interruptcbk && ts_interruptcbk()) {
			priv->abort = 1;
			break;
		}

		junk = priv->ts.packet_size - TS_PACKET_SIZE;
		buf_size = priv->ts.packet_size - junk;

		if (!ts_sync_N(stream, priv->ts.packet_size, 1, 1)) {
			if (!ts_sync_N(stream, priv->ts.packet_size, 2, 0))
				return -1;
		}

		packet[0] = GxStream_ReadChar(stream);
		len = GxStream_Read(stream, &packet[1], 3);
		if (len != 3)
			break;
		buf_size -= 4;

		pid = ((packet[1] & 0x1f) << 8) | packet[2];

		if ((packet[1] >> 7) & 0x01)	//transport error
			ts_error = 1;
		is_start = packet[1] & 0x40;

		if (!priv->pat.progs_cnt && pid != 0) {
			GxStream_Skip(stream, buf_size + junk);
			continue;
		}

		bad = ts_error;	// || (! cc_ok);
		if (bad) {
			if (priv->keep_broken == 0) {
				GxStream_Skip(stream, buf_size + junk);
				continue;
			}
			is_start = 0;
		}

		if (((pid > 1) && (pid < 16)) || (pid == NB_PID_MAX-1))	//invalid pid
		{
			GxStream_Skip(stream, buf_size + junk);
			continue;
		}

		afc = (packet[3] >> 4) & 3;
		if (!(afc % 2))	//no payload in this TS packet
		{
			GxStream_Skip(stream, buf_size + junk);
			continue;
		}

		if (afc > 1) {
			int c;
			c = GxStream_ReadChar(stream);
			buf_size--;
			if (c < 0 || c > 183)	//broken from the stream layer or invalid
			{
				GxStream_Skip(stream, buf_size + junk);
				continue;
			}
			//c==0 is allowed!
			if (c > 0) {
				rap_flag = (GxStream_ReadChar(stream) & 0x40) >> 6;
				buf_size--;
				c--;
				GxStream_Skip(stream, c);
				buf_size -= c;
				if (buf_size == 0)
					continue;
			}
		}

		//TABLE PARSING
		base = priv->ts.packet_size - buf_size;
		p = &packet[base];

		len = GxStream_Read(stream, p, buf_size);
		if (len < buf_size) {
			continue;
		}

		if (junk > 0)
			GxStream_Skip(stream, junk);

		if (external_mpegts_descramble_callback(packet, sizeof(packet)/sizeof(packet[0]), tmp_packet, sizeof(tmp_packet)/sizeof(tmp_packet[0])) < 0) {
			continue;
		}
		memset(packet, 0, sizeof(packet)/sizeof(packet[0]));
		memcpy(packet, tmp_packet, sizeof(packet)/sizeof(packet[0]));

		if (pid == 0) {
			parse_pat(priv, is_start, p, buf_size);
		} else {
			int x;
			if (priv->pat.progs == NULL)
				continue;

			for (x = 0; x < priv->pat.progs_cnt; x++) {
				if (priv->pat.progs[x].pmt_pid == pid) {
					progid = priv->pat.progs[x].id;
					parse_pmt(priv, progid, pid, is_start, &packet[base], buf_size);
				}
			}
		}
		continue;
	}

	demuxer->stream->eof = 0;
	demuxer->stream->read_timeoutms = timeoutms;
	GxStream_Seek(demuxer->stream, init_pos);

	return ret;
}

// 0 = EOF or no stream found
// else = [-] number of bytes written to the packet
static int ts_parse(GxDemuxer* demuxer, ESStream * es, uint8_t * packet, int probe)
{
	ESStream* tss;
	int buf_size, is_start, pid, base;
	int len, cc, cc_ok, afc, retv = 0, is_video, is_audio, is_sub;
	DemuxTSPriv* priv = (DemuxTSPriv *) demuxer->priv;
	GxStream* stream = demuxer->stream;
	uint8_t* p = NULL;
	uint8_t tmp_packet[TS_FEC_PACKET_SIZE] = {0};
	int32_t progid, pid_type, bad, ts_error;
	int junk = 0, rap_flag = 0, broken_cnt = 0;
	TSPMT* pmt;
	Mp4DecoderConfig* mp4_dec;
	TSStreamInfo* si;
	AVFIFO* avfifo = NULL;
	off_t init_pos = GxStream_Tell(demuxer->stream);

	while (!priv->abort) {
		bad = ts_error = 0;
		avfifo = NULL;
		rap_flag = 0;
		mp4_dec = NULL;
		es->is_synced = 0;
		si = NULL;
		if (ts_interruptcbk && ts_interruptcbk()) {
			priv->abort = 1;
			return 0;
		}

		if (demuxer->stream->pos - init_pos >= TS_MAX_PROBE_SIZE) {
			if (broken_cnt++ > 0) {
				gxlogd("Ts file borken at pos:%lld\n", init_pos);
				demuxer->stream->eof = 1;
				return 0;
			}
			init_pos = demuxer->stream->pos;
		}

		junk = priv->ts.packet_size - TS_PACKET_SIZE;
		buf_size = priv->ts.packet_size - junk;

		if (GxStream_Eof(stream)) {
			if (!probe) {
				ts_dump_streams(priv);
				demuxer->filepos = GxStream_Tell(demuxer->stream);
			}
			return 0;
		}

		if (!ts_sync_N(stream, priv->ts.packet_size, 1, 1)) {
			if (!ts_sync_N(stream, priv->ts.packet_size, 2, 0)) {
				return 0;
			}
		}

		packet[0] = GxStream_ReadChar(stream);
		len = GxStream_Read(stream, &packet[1], 3);
		if (len != 3)
			return 0;
		buf_size -= 4;

		pid = ((packet[1] & 0x1f) << 8) | packet[2];
		if (priv->time_insert) {
			if (pid == priv->hw.ts_info.timestamp_pid) {
				GxStream_Skip(stream, 5);
				priv->timetick = GxStream_ReadDword_Le(stream);
				buf_size -= 9;
				GxStream_Skip(stream, buf_size);
				continue;
			}
		}

		if (!probe) {
			if (ts_filter_pid(stream, priv, pid, packet, junk) == 0)
				continue;
		}

		if ((packet[1] >> 7) & 0x01)	//transport error
			ts_error = 1;
		is_start = packet[1] & 0x40;

		tss = new_pid(priv, pid);
		if (tss == NULL)
			continue;

		cc = (packet[3] & 0xf);
		cc_ok = (tss->last_cc < 0) || ((((tss->last_cc + 1) & 0x0f) == cc));
		tss->last_cc = cc;

		bad = ts_error;	// || (! cc_ok);
		if (bad) {
			if (priv->keep_broken == 0) {
				GxStream_Skip(stream, buf_size + junk);
				continue;
			}
			is_start = 0;	//queued to the packet data
		}

		if (is_start)
			tss->is_synced = 1;

		if ((!is_start && !tss->is_synced) || ((pid > 1) && (pid < 16)) || (pid == NB_PID_MAX-1))	//invalid pid
		{
			GxStream_Skip(stream, buf_size + junk);
			continue;
		}

		afc = (packet[3] >> 4) & 3;
		if (!(afc % 2))	//no payload in this TS packet
		{
			GxStream_Skip(stream, buf_size + junk);
			continue;
		}

		if (afc > 1) {
			int c;
			c = GxStream_ReadChar(stream);
			buf_size--;
			if (c < 0 || c > 183)	//broken from the stream layer or invalid
			{
				GxStream_Skip(stream, buf_size + junk);
				continue;
			}
			//c==0 is allowed!
			if (c > 0) {
				rap_flag = (GxStream_ReadChar(stream) & 0x40) >> 6;
				buf_size--;
				c--;
				GxStream_Skip(stream, c);
				buf_size -= c;
				if (buf_size == 0)
					continue;
			}
		}
		//find the program that the pid belongs to; if (it's the right one or -1) && pid_type==SL_SECTION
		//call parse_sl_section()
		pmt = pmt_of_pid(priv, pid, &mp4_dec);
		if (mp4_dec) {
			fill_extradata(mp4_dec, tss);
			if (IS_VIDEO(mp4_dec->object_type) || IS_AUDIO(mp4_dec->object_type)) {
				tss->type = SL_PES_STREAM;
				tss->subtype = mp4_dec->object_type;
			}
		}
		//TABLE PARSING

		base = priv->ts.packet_size - buf_size;

		priv->last_pid = pid;

		is_video = IS_VIDEO(tss->type) || (tss->type == SL_PES_STREAM && IS_VIDEO(tss->subtype));
		is_audio = IS_AUDIO(tss->type) || (tss->type == SL_PES_STREAM && IS_AUDIO(tss->subtype))
			|| (tss->type == PES_PRIVATE1);
		is_sub = IS_SUBT(tss->type);
		pid_type = pid_type_from_pmt(priv, pid);

		// PES CONTENT STARTS HERE
		if (!probe) {
			if ((is_video || is_audio || is_sub) && is_start && !priv->ts.streams[pid].sh) {
				if ((priv->prog == 0) || (priv->prog == progid_for_pid(priv, pid, 0)))
					ts_add_stream(demuxer, tss);
			}

			if (is_video && (demuxer->video->id == priv->ts.streams[pid].id)) {
				avfifo = &priv->fifo[1];
				si = &priv->vstr;
			} else if (is_audio) {
				if (demuxer->audio && demuxer->audio->id == priv->ts.streams[pid].id) {
					if (NEEDDROP(demuxer->audio->dropmode)) {
						GxStream_Skip(stream, buf_size + junk);
						continue;
					} else {
						avfifo = &priv->fifo[0];
						si = &priv->astr;
					}
				}else if (priv->audio1_running && demuxer->audio1 && demuxer->audio1->id == priv->ts.streams[pid].id) {
					if (NEEDDROP(demuxer->audio1->dropmode)) {
						GxStream_Skip(stream, buf_size + junk);
						continue;
					}else{
						avfifo = &priv->fifo[3];
						si = NULL;
					}
				}else{
					GxStream_Skip(stream, buf_size + junk);
					continue;
				}
			} else if (is_sub || IS_SUBT(pid_type)) {
				GxStream_Skip(stream, buf_size + junk);
				continue;
			}
			//IS IT TIME TO QUEUE DATA to the dp_packet?
			if (is_start && avfifo  && avfifo->offset > 0) {
				retv = fill_packet(demuxer, avfifo, si);
			}
		}

		if (is_start && avfifo && avfifo->synced == 0) {
			avfifo->synced = 1;
		}

		if (probe || !avfifo) {
			p = &packet[base];
		}
		else {
			if (avfifo)
				p = &(avfifo->buffer[avfifo->offset]);
			else
				p = NULL;
		}

		len = GxStream_Read(stream, p, buf_size);
		if (len < buf_size) {
			continue;
		}
		GxStream_Skip(stream, junk);

		if (probe)
		{
			if (external_mpegts_descramble_callback(packet, TS_FEC_PACKET_SIZE, tmp_packet, TS_FEC_PACKET_SIZE) < 0) {
				continue;
			}
			memset(packet, 0, TS_FEC_PACKET_SIZE);
			memcpy(packet, tmp_packet, TS_FEC_PACKET_SIZE);

			if (pid == 0) {
				parse_pat(priv, is_start, p, buf_size);
				continue;
			} else if ((tss->type == SL_SECTION) && pmt) {
				int k, mp4_es_id = -1;
				TSSection* section;
				for (k = 0; k < pmt->mp4es_cnt; k++) {
					if (pmt->mp4es[k].decoder.object_type == MP4_OD
							&& pmt->mp4es[k].decoder.stream_type == MP4_OD)
						mp4_es_id = pmt->mp4es[k].id;
				}
				for (k = 0; k < pmt->es_cnt; k++) {
					if (pmt->es[k].mp4_es_id == mp4_es_id) {
						section = &(tss->section);
						parse_sl_section(pmt, section, is_start, &packet[base], buf_size);
					}
				}
				continue;
			} else {
				progid = prog_id_in_pat(priv, pid);
				if (progid != -1) {
					int idx = priv->ts.streams[pid].id;
					if (idx != demuxer->video->id && idx != demuxer->audio->id && idx != demuxer->sub->id) {
						parse_pmt(priv, progid, pid, is_start, &packet[base], buf_size);
						continue;
					}
				}
			}
		}

		if (!probe && !avfifo)
			continue;

		if (is_start) {
			uint8_t* lang = NULL;

			len = pes_parse2(priv,p, buf_size, es, pid_type, pmt, pid);
			if (!len || es->size < 0) {
				tss->is_synced = 0;
				continue;
			}
			es->pid = tss->pid;
			tss->is_synced |= es->is_synced || rap_flag;
			tss->payload_size = es->payload_size;

			if (is_audio && (lang = pid_lang_from_pmt(priv, es->pid))) {
				memcpy(es->lang, lang, 3);
				es->lang[3] = 0;
			} else
				es->lang[0] = 0;

			if (probe) {
				if (es->type == UNKNOWN)
					return 0;

				tss->type = es->type;
				tss->subtype = es->subtype;

				return 1;
			} else {
				if (es->pts == 0.0f)
					es->pts = tss->pts = tss->last_pts;
				else
					tss->pts = tss->last_pts = es->pts;

				demuxer->filepos = GxStream_Tell(demuxer->stream) - es->size;
				if (p != es->start)
					memmove(p, es->start, es->size);
				avfifo->extended_stream = 0;
				avfifo->offset += es->size;
				avfifo->pos = GxStream_Tell(demuxer->stream);
				avfifo->pts = es->pts;
				avfifo->ad_tags      = es->ad_tags;
				avfifo->ad_fade_byte = es->ad_fade_byte;
				avfifo->ad_pan_byte  = es->ad_pan_byte;
				if (1) {
					uint8_t stream_type = pid_stream_type_from_pmt(priv, es->pid);

					if ((stream_type == 0x83) && (es->extended_stream_id == 0x72)) { //TRUEHD ES
						avfifo->extended_stream = 1;
					} else if ((stream_type == 0x83) && (es->extended_stream_id == 0x76)) {//DOLBY ES
						avfifo->extended_stream = 0;
					}
				}
				if (retv > 0)
					return retv;
				else
					continue;
			}
		} else {
			uint16_t sz;

			es->pid = tss->pid;
			es->type = tss->type;
			es->subtype = tss->subtype;
			es->pts = tss->pts = tss->last_pts;
			es->start = &packet[base];

			if (tss->payload_size > 0) {
				sz = GXMIN(tss->payload_size, buf_size);
				tss->payload_size -= sz;
				es->size = sz;
			} else {
				if (is_video) {
					sz = es->size = buf_size;
				} else {
					continue;
				}
			}

			if (!probe) {
				avfifo->offset += sz;

				if (avfifo->offset + priv->ts.packet_size >= MAX_PES_SIZE) {
					avfifo->pts = tss->last_pts;
					retv = fill_packet(demuxer, avfifo, si);
					return 1;
				}

				continue;
			} else {
				memmove(es->start, p, sz);
				if (es->size)
					return es->size;
				else
					continue;
			}
		}
	}

	return 0;
}

static void reset_fifos(DemuxTSPriv *priv, int a, int v, int s, int a1)
{
	ESStream *es = NULL;

	if (a) {
		priv->fifo[0].pts = GX_NOPTS_VALUE;
		priv->fifo[0].pos = 0;
		priv->fifo[0].offset = 0;
		priv->fifo[0].buffer_size = MAX_PES_SIZE;
		es = new_pid(priv, priv->ts_a_pid);
		if (es)
			es->pts = es->last_pts = GX_NOPTS_VALUE;
	}

	if (v) {
		priv->fifo[1].pts = GX_NOPTS_VALUE;
		priv->fifo[1].pos = 0;
		priv->fifo[1].offset = 0;
		priv->fifo[1].buffer_size = MAX_PES_SIZE;
		es = new_pid(priv, priv->ts_v_pid);
		if (es)
			es->pts = es->last_pts = GX_NOPTS_VALUE;
	}

	if (s) {
		priv->fifo[2].pts = GX_NOPTS_VALUE;
		priv->fifo[2].pos = 0;
		priv->fifo[2].offset = 0;
		priv->fifo[2].buffer_size = MAX_PES_SIZE;
	}

	if (a1) {
		priv->fifo[3].pts = GX_NOPTS_VALUE;
		priv->fifo[3].pos = 0;
		priv->fifo[3].offset = 0;
		priv->fifo[3].buffer_size = MAX_PES_SIZE;
	}
}

static int is_usable_program(DemuxTSPriv *priv, TSPMT * pmt)
{
	int j;

	for (j = 0; j < pmt->es_cnt; j++) {
		if (priv->ts.streams[pmt->es[j].pid].sh == NULL)
			continue;
		if (priv->ts.streams[pmt->es[j].pid].type == TYPE_VIDEO ||
				priv->ts.streams[pmt->es[j].pid].type == TYPE_AUDIO)
			return 1;
	}

	return 0;
}

static int demux_ts_check_file(GxDemuxer* demuxer)
{
	uint8_t packet_size = ts_check_file(demuxer);

	if(packet_size > 0) {
		DemuxTSPriv *priv = av_mallocz(sizeof(DemuxTSPriv));
		if (priv == NULL)
			return 0;

		priv->ts.packet_size = packet_size;
		memset(&priv->filter,     0, sizeof(DemuxDataFilter));
		memset(&priv->sub_filter, 0, sizeof(DemuxSubFilter));

		demuxer->priv = priv;
		demuxer->type = GX_DEMUXER_TYPE_MPEG_TS;
		return GX_DEMUXER_TYPE_MPEG_TS;
	}

	return 0;
}

static void demux_ts_close(GxDemuxer* demuxer)
{
	uint16_t i;
	DemuxTSPriv* priv = (DemuxTSPriv *) demuxer->priv;

	if (priv)
	{
		ESStream* ess = priv->ts.pids_hdr;
		ESStream* ess2 = priv->ts.pids_hdr;

		if (priv->pat.section.buffer)
			av_free(priv->pat.section.buffer);
		if (priv->pat.progs)
			av_free(priv->pat.progs);

		ts_dump_streams(priv);

		if (priv->pmt) {
			for (i = 0; i < priv->pmt_cnt; i++) {
				if (priv->pmt[i].section.buffer)
					av_free(priv->pmt[i].section.buffer);
				if (priv->pmt[i].es)
					av_free(priv->pmt[i].es);
			}
			av_free(priv->pmt);
		}

		while (ess) {
			ess2 = ess->next;
			av_free(ess);
			ess = ess2;
		}

		av_free(priv);
	}

	if (demuxer->durstream) {
		GxStream_Close(demuxer->durstream);
		demuxer->durstream = NULL;
	}

	demuxer->priv = NULL;
}

static int64_t get_newpos(GxDemuxer *demuxer, int64_t newpts)
{
	int flag = 0;
	int64_t offset = 0, newpos = 0;
	int64_t to_start, to_last, to_end;
	DemuxTSPriv* priv = (DemuxTSPriv *) demuxer->priv;

	to_start = newpts - demuxer->start_pts;
	to_last  = labs(newpts - priv->lastseekpts);
	to_end   = demuxer->start_pts + (uint64_t)demuxer->duration - newpts;

	flag = (to_start <= to_end) ? (to_start <= to_last ? 1 : 2) : (to_end <= to_last ? 3 : 2);

	switch(flag)
	{
	case 1: // near start
		newpos = demuxer->movi_start;
		offset = to_start*priv->bps;
		break;
	case 2: // near lastseek
		newpos = priv->lastseekpos;
		offset = (newpts - priv->lastseekpts)*priv->bps;
		break;
	case 3: // near end
		newpos = demuxer->movi_end;
		offset = -(to_end*priv->bps);
		break;
	default:
		break;
	}

	while (newpos+offset > demuxer->movi_end)
	{
		offset /= 2;
	}
	newpos += offset;

	return newpos;
}

static int64_t ts_get_pts_wantpid(GxDemuxer* d, int wantpid, int probe_count)
{
	uint64_t pts;
	DemuxTSPriv* priv=NULL;
	unsigned char packet[204], *pes=NULL;
	int read_len, is_start=0, pid=0, afc, packet_len;
	off_t oldpos = GxStream_Tell(d->durstream);
	int64_t ret_pts = -1;

	if (d && d->priv)
		priv = (DemuxTSPriv *) d->priv;

	packet_len = priv->ts.packet_size;
	if (packet_len != 204 && packet_len != 188 && packet_len != 192)
		return -1;

	if (d->durstream)
	{
		if (GxStream_Tell(d->durstream)-oldpos >= priv->ts_dur_probe_size)
			return -1;
		//synced!

research:
		while ((wantpid != -1 && pid != wantpid) || !is_start) {
			if (GxStream_Tell(d->durstream)-oldpos >= (priv->ts_dur_probe_size * probe_count))
				return ret_pts;

			if (!ts_sync_N(d->durstream, packet_len, 1, 1)) {
				if (!ts_sync_N(d->durstream, packet_len, 2, 0)) {
					return ret_pts;
				}
			}
			packet[0] = GxStream_ReadChar(d->durstream);
			read_len = GxStream_Read(d->durstream, &packet[1], packet_len-1);
			if (read_len != packet_len-1)
				return ret_pts;
			is_start = packet[1] & 0x40;
			pid = ((packet[1] & 0x1f) << 8) | packet[2];
		}

		//get es packet!
		pes = &packet[4];
		afc = (packet[3] >> 4) & 3;
		if (!(afc % 2)) {//no payload in this TS packet
			is_start = 0;
			goto research;
		}

		if (afc == 3) {
			int c;
			c = packet[4];
			if (c < 0 || c > 183) { //broken from the stream layer or invalid
				is_start = 0;
				goto research;
			}
			//c==0 is allowed!
			if (c > 0) {
				pes += (c+1);
			}
		}
		if (pes[0] || pes[1] || (pes[2] != 1)) {
			is_start = 0;
			goto research;
		}
		if (pes[7] & 0x80) {    /* pts available*/
			pts = (int64_t) (pes[9] & 0x0E) << 29;
			pts |= pes[10] << 22;
			pts |= (pes[11] & 0xFE) << 14;
			pts |= pes[12] << 7;
			pts |= (pes[13] & 0xFE) >> 1;
		}
		else {
			is_start = 0;
			goto research;
		}

		ret_pts = pts/90.0;
		if (probe_count > 1) {
			is_start = 0;
			goto research;
		} else
			return ret_pts;
	}
	else
		return -1;
}

static void ts_search_vheader(GxDemuxer* demuxer, off_t search_range)
{
	int i, pos;
	off_t start_pos = 0, cur_pos = 0;
	GxDemuxStream       *d_video = demuxer->video;
	GxStreamVideoHeader *sh_video = d_video->sh;
	DemuxTSPriv *priv = (DemuxTSPriv*)demuxer->priv;

	if (!sh_video)
		return;

	if ((sh_video->format == VIDEO_CODEC_H264) || (sh_video->format == VIDEO_CODEC_H265))
		GxParseES_Reset(&priv->es_parser);

	start_pos = GxStream_Tell(demuxer->stream);
	while (!GxStream_Eof(demuxer->stream))
	{
		cur_pos = GxStream_Tell(demuxer->stream);
		if (cur_pos - start_pos > search_range) {
			gxlogd("no sps in %lld Byte\n", search_range);
			break;
		}

		priv->es_parser.ds = d_video;
		i = GxParseES_SyncVideoPacket(&priv->es_parser);
		if (sh_video->format == VIDEO_CODEC_VC1) {
			if (i == 0x10E || i == 0x10F)//entry point or sequence header
				break;
		}
		else if (sh_video->format == VIDEO_CODEC_MPEG4) {
			if (i == 0x1B6) {//vop (frame) startcode
				pos = priv->es_parser.videobuf_len;
				if (!GxParseES_ReadVideoPacket(&priv->es_parser))
					break;//eof
				if ((priv->es_parser.videobuffer[pos + 4] & 0x3F) == 0)
					break;//I-frame
			}
		}
		else if (sh_video->format == VIDEO_CODEC_H264) {
			if ((i & ~0x60)== 0x107) {
				d_video->buffer_pos = 0;
				break;
			}
		}
		else if (sh_video->format == VIDEO_CODEC_AVS) {
			if (i == 0x100)
				break;
		}
		else if (sh_video->format==VIDEO_CODEC_MPEG12) {
			if (i == 0x1B3 || i == 0x1B8)
				break;//found it!
		}
		else if (sh_video->format == VIDEO_CODEC_H265) {
			//if (i == 0x100)
			break;
		}
		else {
			gxlogf("\n---%s:%d, unsupport format!\n", __func__, __LINE__);
		}

		if (!i || !GxParseES_SkipVideoPacket(&priv->es_parser))
			break;//eof?
	}
}

static int time_insert_ts_seek(GxDemuxer *demuxer, int64_t newpts_ms, int nFlags)
{
	int ret = GX_PLAYER_OK;
	DemuxTSPriv* priv	= (DemuxTSPriv*)demuxer->priv;
	GxDemuxStream *d_sub	= demuxer->sub;
	GxDemuxStream *d_audio	= demuxer->audio;
	GxDemuxStream *d_video	= demuxer->video;
	GxDemuxStream *d_audio1	= demuxer->audio1;
	GxStreamAudioHeader	*sh_audio  = d_audio->sh;
	GxStreamAudioHeader	*sh_audio1 = d_audio1->sh;
	GxStreamVideoHeader	*sh_video  = d_video->sh;
	GxStreamSubHeader	*sh_sub    = d_sub->sh;

	off_t pos, file_size = demuxer->stream->end_pos;
	int64_t oldpts=0;
	off_t pos_interval = 0;
	off_t oldpos = demuxer->filepos;

	//get newpts
	if (priv) {
		priv->abort = 0;
		oldpts = priv->last_pts;
	}

	if (nFlags & GX_DEMUXER_SEEK_PERCENT) {
		if (oldpts<=0)return GX_PLAYER_ERROR;
		pos = newpts_ms* oldpos / oldpts;
	} else {
		pos = (double)(newpts_ms - priv->ts_start_pts) * file_size / demuxer->duration;

		if (newpts_ms > demuxer->last_seekms) {
			if (pos < demuxer->last_seekpos) {
				pos_interval = (newpts_ms - demuxer->last_seekms) * 1.0 / demuxer->duration * file_size;
				pos = demuxer->last_seekpos + pos_interval;
			}
		} else {
			if (pos > demuxer->last_seekpos) {
				pos_interval = (demuxer->last_seekms - newpts_ms) * 1.0 / demuxer->duration * file_size;
				pos = demuxer->last_seekpos - pos_interval;
				pos = pos < 0 ? 0 : pos;
			}
		}

		demuxer->last_seekms = newpts_ms;
		demuxer->last_seekpos = pos;
	}

	GxStream_Seek(demuxer->stream, pos);
	if (sh_video && !priv->hw.ts_info.ext_info.is_encrypt)
		ts_search_vheader(demuxer, SPS_SEARCH_RANGE);
	demuxer->last_seekpos = GxStream_Tell(demuxer->stream);

	reset_fifos(priv, sh_audio != NULL, sh_video != NULL, sh_sub != NULL, sh_audio1 != NULL);
	if (sh_audio != NULL)
		GxDemuxStream_FreePacks(d_audio);
	if (sh_video != NULL)
		GxDemuxStream_FreePacks(d_video);
	if (sh_audio1 != NULL)
		GxDemuxStream_FreePacks(d_audio1);
	if (sh_sub != NULL)
		GxDemuxStream_FreePacks(d_sub);

	return ret;
}

static int demux_ts_seek(GxDemuxer* demuxer, int64_t rel_seek_ms, int32_t audio_delay, int flags)
{
	int ret = GX_PLAYER_OK;
	GxDemuxStream* d_audio = demuxer->audio;
	GxDemuxStream* d_audio1 = demuxer->audio1;
	GxDemuxStream* d_video = demuxer->video;
	GxDemuxStream* d_sub = demuxer->sub;
	GxStreamAudioHeader *sh_audio  = d_audio->sh;
	GxStreamAudioHeader *sh_audio1 = d_audio1->sh;
	GxStreamVideoHeader *sh_video  = d_video->sh;
	GxStreamSubHeader   *sh_sub    = d_sub->sh;
	DemuxTSPriv* priv = (DemuxTSPriv *) demuxer->priv;
	int64_t oldpts=0, newpts=0, last_pts=0;
	off_t oldpos = demuxer->filepos;
	off_t left_pos = 0, right_pos = 0;
	off_t newpos = (flags & GX_DEMUXER_SEEK_ABSOLUTE) ? demuxer->movi_start : oldpos;;
	int max_search = 20;
	int i = 0;

	if (priv) {
		priv->abort = 0;
		oldpts = priv->last_pts;
	}
	newpts = (flags & GX_DEMUXER_SEEK_ABSOLUTE) ? 0 : oldpts;
	if (flags & GX_DEMUXER_SEEK_PERCENT) {
		if (oldpos <= 0)
			return GX_PLAYER_ERROR;
		newpts = (demuxer->movi_end - demuxer->movi_start)* rel_seek_ms/100* oldpts / oldpos;
	}
	else {
		if (flags & GX_DEMUXER_SEEK_ABSOLUTE)
			newpts += priv->ts_start_pts;
		newpts += rel_seek_ms;
	}

	if (demuxer->duration && newpts >= demuxer->duration + priv->ts_start_pts)
		return GX_PLAYER_ERROR;

	if (newpts < 0)
		newpts = 0;

	if (priv->time_insert) {
		ret = time_insert_ts_seek(demuxer, newpts, flags);
		if (!ts_sync_N(demuxer->stream, priv->ts.packet_size, 4, 0))
			return GX_PLAYER_ERROR;
		priv->lastseekpos         = GxStream_Tell(demuxer->stream);
		priv->audio_lastseekbytes = 0;
		priv->video_lastseekbytes = 0;
		return ret;
	}

	if (flags & GX_DEMUXER_SEEK_PERCENT) {
		newpos = demuxer->movi_start+(demuxer->movi_end - demuxer->movi_start) * rel_seek_ms/100;
		GxStream_Seek(demuxer->stream, newpos);
		reset_fifos(priv, sh_audio != NULL, sh_video != NULL, sh_sub != NULL, sh_audio1 != NULL);
		if (sh_audio != NULL)
			GxDemuxStream_FreePacks(d_audio);
		if (sh_audio1 != NULL)
			GxDemuxStream_FreePacks(d_audio1);
		if (sh_video != NULL)
			GxDemuxStream_FreePacks(d_video);
		if (sh_sub != NULL)
			GxDemuxStream_FreePacks(d_sub);
		return ret;
	}
	else {
		newpos = get_newpos(demuxer, newpts);
	}

	reset_fifos(priv, sh_audio != NULL, sh_video != NULL, sh_sub != NULL, sh_audio1 != NULL);
	if (sh_audio != NULL)
		GxDemuxStream_FreePacks(d_audio);
	if (sh_audio1 != NULL)
		GxDemuxStream_FreePacks(d_audio1);
	if (sh_video != NULL)
		GxDemuxStream_FreePacks(d_video);
	if (sh_sub != NULL)
		GxDemuxStream_FreePacks(d_sub);

	if (priv->sub_filter.fifo) {
		GxFifo_Reset(priv->sub_filter.fifo);
		priv->sub_filter.len = 0;
	}

	for (i = 0; i < PLAYER_FILTER_MAX; i++) {
		if (priv->filter.data[i].using) {
			if (priv->filter.data[i].p_data)
				av_free(priv->filter.data[i].p_data);
			priv->filter.data[i].p_data = 0;
			priv->filter.data[i].p_len = 0;
			priv->filter.data[i].finish = 0;
		}
	}

	if (priv->filter.fifo) {
		GxFifo_Reset(priv->filter.fifo);
	}

	if (demuxer->stream->file_format==GX_STREAMTYPE_STREAM) {
		if ((demuxer->stream->flags & GX_STREAM_SEEK_TIME) == GX_STREAM_SEEK_TIME)
			GxStream_SeekTime(demuxer->stream, newpts);
		else
			GxStream_Seek(demuxer->stream, newpos);
		demuxer->stream->eof = 0;
		d_video->eof = 0;
		d_audio->eof = 0;
		d_audio1->eof = 0;
		return ret;
	}

	while (!GxStream_Eof(demuxer->stream))
	{
		int i;
		GxStream* s = demuxer->durstream;

		if (newpos < demuxer->movi_start)
			newpos = demuxer->movi_start;

		i = 0;
		GxStream_Seek(s, newpos);
		last_pts = ts_get_pts_wantpid(demuxer, priv->ts_dur_pid, 1);
		while ((last_pts <= 0) && i < 3) {
			s->eof = 0;
			if (GxStream_Seek(s, newpos+priv->ts_dur_probe_size*(++i)) != GX_PLAYER_OK)
				break;
			last_pts = ts_get_pts_wantpid(demuxer, priv->ts_dur_pid, 1);
		}

		if (last_pts <= 0 || abs(newpts - last_pts) <= 500)
			break;

		if (left_pos && right_pos) {
			if (newpts > last_pts) {
				left_pos = GxStream_Tell(s);
			}
			else {
				right_pos = GxStream_Tell(s);
			}
		}
		else {
			off_t pos_interval;

			if (newpts > last_pts) {
					pos_interval = (newpts - last_pts) * 1.0 /
						(demuxer->duration + priv->ts_start_pts - last_pts) * (demuxer->movi_end - newpos);
				left_pos = GxStream_Tell(s);
				newpos += pos_interval;
			}
			else {
				pos_interval = (last_pts - newpts) * 1.0 /
					(last_pts - priv->ts_start_pts) * (newpos - demuxer->movi_start);
				right_pos = GxStream_Tell(s);
				newpos -= pos_interval;
			}

			if (pos_interval == 0)
				break;
		}

		if (left_pos && right_pos) {
			newpos = (left_pos + right_pos)/2;
			if (--max_search < 0)
				break;
			if (abs(newpos - GxStream_Tell(s)) < priv->ts_dur_probe_size)
				break;
		}
	}

	if (demuxer->duration && priv->lastseekpts > 0) {
		if (newpts > priv->lastseekpts) {
			if (newpos < priv->lastseekpos) {
				newpos = priv->lastseekpos + 4096;
			}
		} else {
			if (newpos > priv->lastseekpos) {
				newpos = priv->lastseekpos - 4096;
			}
		}
		newpos = newpos < demuxer->movi_start ? demuxer->movi_start : newpos;
		newpos = newpos > demuxer->movi_end ? demuxer->movi_end : newpos;
	}

	GxStream_Seek(demuxer->stream, newpos);
	ts_search_vheader(demuxer,  SPS_SEARCH_RANGE);
	priv->lastseekpts = newpts;
	priv->lastseekpos = newpos;
	priv->audio_lastseekbytes = 0;
	priv->video_lastseekbytes = 0;
	if (!ts_sync_N(demuxer->stream, priv->ts.packet_size, 4, 0))
		return GX_PLAYER_ERROR;

	return ret;
}

/*
 * ts_parse may be error when fill buffer in the stream eof,
 * we should fill the tail data saved in the fifo which had't filled before
 */
static void fill_tail_data(GxDemuxer* demuxer)
{
	DemuxTSPriv* priv = (DemuxTSPriv *) demuxer->priv;

	fill_packet(demuxer, &priv->fifo[0], &priv->astr);
	fill_packet(demuxer, &priv->fifo[1], &priv->vstr);
	fill_packet(demuxer, &priv->fifo[2], NULL);
	fill_packet(demuxer, &priv->fifo[3], NULL);
}

static int demux_ts_fill_buffer(GxDemuxer* demuxer,GxDemuxStream* ds)
{
	ESStream es;
	DemuxTSPriv* priv = (DemuxTSPriv *) demuxer->priv;

	memset(&es, 0, sizeof(ESStream));
	if (-ts_parse(demuxer, &es, priv->packet, 0)) {
		if (demuxer->start_pts < 0) {
			if (priv->ts_v_pid > 0 && priv->ts_start_v_pts > 0)
				demuxer->start_pts = priv->ts_start_v_pts;
			else
				demuxer->start_pts = priv->ts_start_a_pts;
			priv->lastseekpts = demuxer->start_pts;
			priv->lastseekpos = demuxer->movi_start;
			priv->audio_lastseekbytes = 0;
			priv->video_lastseekbytes = 0;
		}
		return GX_PLAYER_OK;
	} else {
		/* fill the tail data which had't filled in ds */
		fill_tail_data(demuxer);
	}

	return GX_PLAYER_ERROR;
}

#define GET_FLAG_CHARS(stream, read_char) \
	do{\
		int i;\
		for (i = 0; i < 5; i ++) {\
			read_char[i] = GxStream_ReadChar(stream);\
		}\
	}while (0)
static int64_t ts_get_priv_timestamp(GxStream *stream, int pkt_size)
{
	int  endpts_sec=0;
	char read_char[5];
	char pkt_flags[5] = {'-', 'T', 'I', 'M', 'E'};
	off_t npos;

	if (stream == NULL)
		return 0;

	if (!ts_sync_N(stream, pkt_size, 4, 0)) {
		gxlogf("%s : pkt_size = %d, can not sync.\n", __func__, pkt_size);
		return -1;
	}

	GxStream_Skip(stream, 3);//skip pid

	npos = GxStream_Tell(stream);
	while (!stream->eof) {
		GET_FLAG_CHARS(stream, read_char);
		if (!memcmp((read_char + 1), (pkt_flags + 1), 4)) {
			endpts_sec = GxStream_ReadDword_Le(stream);
			gxlogf(">> pts_sec = %d\n", endpts_sec);
			break;
		}
		else
			GxStream_Skip(stream, pkt_size-5);

		if (GxStream_Tell(stream) - npos >= 2*MAX_CHECK_SIZE)
			return 0;
	}

	return endpts_sec*1000;
}

static uint64_t ts_get_duration(GxDemuxer *d)
{
#define DURATION_PROBE_COUNT 50
	int ret, i=0;
	GxStream* s = d->durstream;
	int64_t endpts = 0, duration = 0;
	DemuxTSPriv* priv = (DemuxTSPriv *) d->priv;
	off_t end_pos;

	if (d && s)
	{
		ret = GxStream_Control(s,GX_STREAM_CTRL_GET_SIZE, &end_pos);
		if (ret != GX_PLAYER_OK)
			return 0;

		if (end_pos == priv->end_pos)
			return d->duration;

		priv->end_pos = end_pos;
		if (priv->end_pos <= priv->ts_dur_probe_size) {
			d->duration = 0;
			return 0;
		}

		d->stream->end_pos = s->end_pos;
		d->movi_end =  s->end_pos;

reget:
		while ((endpts <= 0) &&
				(i < DURATION_PROBE_COUNT) &&
				(s->end_pos > (priv->ts_dur_probe_size * (++i)))) {
			s->eof = 0;
			ret = GxStream_Seek(s, s->end_pos - priv->ts_dur_probe_size * i);
			if (ret != GX_PLAYER_OK)
				continue;
			if (priv->time_insert)
				endpts = ts_get_priv_timestamp(d->durstream, priv->ts.packet_size);
			else {
				endpts = ts_get_pts_wantpid(d, priv->ts_dur_pid, i);
				i += (DURATION_PROBE_COUNT - 2);
			}

			gxlogf("*********%d******%d*****%lld\n", i,  priv->ts_start_pts, endpts);
		}

		if (endpts > 0) {
			duration = endpts - priv->ts_start_pts;
		} else {
			if (!priv->time_insert && priv->ts_dur_pid > 0) {
				i = 0;
				priv->ts_dur_pid = -1;
				goto reget;
			}
		}

		if (s->eof)
			s->eof = 0;
	}

	duration = (duration>=0 ? duration : 0);
	d->duration = duration;

	return duration;

}

static int ts_filter_sub(DemuxSubFilter* sub_filter, uint8_t* buffer, int size, GxDemuxerDataType type)
{
	int read_size = 0;
	uint8_t data_p[TS_PACKET_SIZE];

	if (sub_filter->len >= size) {
		memcpy(buffer, sub_filter->data, size);
		sub_filter->len -= size;
		if (sub_filter->len) {
			memmove(sub_filter->data, sub_filter->data + size, sub_filter->len);
		}
		return size;
	}

	while ((GxFifo_GetLength(sub_filter->fifo) >= TS_PACKET_SIZE)) {
		int pid = 0, data_len = 0, payload_start = 0;
		uint8_t *pdata = NULL;

		GxFifo_Read(sub_filter->fifo, data_p, TS_PACKET_SIZE, -1);
		if (data_p[0] != 0x47)
			continue;

		pid = ((((uint16_t)(data_p[1] & 0x1f)) << 8) | ((uint16_t)data_p[2]));
		if (pid != sub_filter->pid)
			continue;

		if ((data_p[1] & 0x80) != 0)
			continue;

		if ((data_p[1] & 0x40) != 0)
			payload_start = 1;
		else
			payload_start = 0;

		if ((data_p[3] & 0x30) == 0x30) {
			data_len = TS_PACKET_SIZE - 4 - 1 - data_p[4];
			pdata = data_p + TS_PACKET_SIZE - data_len;
		} else if ((data_p[3] & 0x30) == 0x10) {
			data_len = 184;
			pdata = data_p + TS_PACKET_SIZE - data_len;
		} else {
			data_len = 0;
			continue;
		}

		if ((data_len <= 0) || (data_len > 184))
			continue;

		if (payload_start) {
			if (type == DEMUX_PES_TYPE) {
				if ((data_len > 5) &&
						(pdata[0] == 0x00) &&
						(pdata[1] == 0x00) &&
						(pdata[2] == 0x01) &&
						(pdata[3] == 0xbd)) {
					sub_filter->right = 1;
					sub_filter->total_len = ((pdata[4]<<8)|(pdata[5])) + 6;
					if (sub_filter->total_len > SUB_FILTER_SIZE) {
						sub_filter->total_len = SUB_FILTER_SIZE;
						gxlogd("pesbuffer too small, or pespacket too big\n");
					}
				} else {
					sub_filter->right = 0;
					sub_filter->total_len = 0;
				}
			} else if (type == DEMUX_PSI_TYPE) {
				if ((pdata[0] > 0) && pdata[0] < (data_len - 1)) {
					unsigned int pad_len = pdata[0];

					memcpy(buffer, sub_filter->data, sub_filter->len);
					memcpy(buffer + sub_filter->len, &pdata[1], pad_len);
					read_size = sub_filter->len + pad_len;
					pdata += pad_len;
					if (pdata[1] == 0xc6) {
						sub_filter->len = data_len - pad_len - 1;
						memcpy(sub_filter->data, &pdata[1], sub_filter->len);
						sub_filter->right = 1;
						sub_filter->total_len = (((pdata[2]<<8) | pdata[3]) & 0xfff) + 3;
					} else {
						sub_filter->right = 0;
						sub_filter->total_len = 0;
					}
					return read_size;
				} else if (pdata[0] >= data_len) {
					sub_filter->right = 0;
					sub_filter->total_len = 0;
				} else {
					if ((data_len > 3) && (pdata[1] == 0xc6)) {
						sub_filter->right = 1;
						sub_filter->total_len = (((pdata[2]<<8) | pdata[3]) & 0xfff) + 3;
						pdata++;
						data_len--;
						if (sub_filter->total_len > SUB_FILTER_SIZE) {
							sub_filter->total_len = SUB_FILTER_SIZE;
							gxlogd("psibuffer too small, or pespacket too big\n");
						}
					}
				}
			} else {
				sub_filter->right = 0;
				sub_filter->total_len = 0;
			}
		}

		if (sub_filter->right) {
			if (payload_start) {
				unsigned char parse_complete = 0;
				if (sub_filter->len) {
					read_size = sub_filter->len;
					memcpy(buffer, sub_filter->data, read_size);
					memcpy(sub_filter->data, pdata, data_len);
					sub_filter->len = data_len;
					parse_complete = 1;
				}
				memcpy(sub_filter->data, pdata, data_len);
				sub_filter->len = data_len;

				if ((parse_complete == 0) && (sub_filter->total_len <= data_len)) {
					memcpy(buffer, sub_filter->data, sub_filter->len);
					read_size = sub_filter->len;
					sub_filter->total_len = 0;
					sub_filter->len = 0;
					parse_complete = 1;
				}

				if (parse_complete) return read_size;
			}
			else if ((sub_filter->total_len > 6) &&
					(sub_filter->len + data_len >= sub_filter->total_len)) {
				int tocopy = sub_filter->total_len - sub_filter->len;
				memcpy(sub_filter->data + sub_filter->len, pdata, tocopy);
				memcpy(buffer, sub_filter->data, sub_filter->total_len);
				read_size = sub_filter->total_len;
				sub_filter->len       = 0;
				sub_filter->right     = 0;
				sub_filter->total_len = 0;
				if (sub_filter->len > 0) {
					memcpy(sub_filter->data, pdata + tocopy, sub_filter->len);
				}
				return read_size;
			} else {
				if (sub_filter->len + data_len >= SUB_FILTER_SIZE) {
					gxlogf("\n%s %d, PES Len error\n", __FUNCTION__, __LINE__);
					sub_filter->right = 0;
					sub_filter->total_len = 0;
					continue;
				}
				memcpy(sub_filter->data+sub_filter->len, pdata, data_len);
				sub_filter->len += data_len;
			}
		}

		if (sub_filter->len >= size) {
			memcpy(buffer, sub_filter->data, size);
			sub_filter->len -= size;
			if (sub_filter->len) {
				memmove(sub_filter->data, sub_filter->data + size, sub_filter->len);
			}
			return size;
		}
	}

	return read_size;
}

static int demux_ts_control(GxDemuxer* demuxer, int cmd, void* arg)
{
	DemuxTSPriv* priv = (DemuxTSPriv *) demuxer->priv;

	switch (cmd) {
	case GX_DEMUXER_CTRL_PRIVATE_RESET:
		{
			DemuxTSPriv* priv = (DemuxTSPriv*)demuxer->priv;
			reset_fifos(priv, 1, 1, 1, 1);
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_GET_SEEKMIN_TIME:
		{
			struct vol_info info;
			DemuxTSPriv* priv = (DemuxTSPriv*)demuxer->priv;

			if (demuxer->stream->file_format == GX_STREAMTYPE_STREAM)
				return GX_DEMUXER_CTRL_NOTIMPL;

			if (priv->time_insert && demuxer->durstream &&
					!GxStream_Control(demuxer->durstream, GX_STREAM_CTRL_GET_VOL_INFO, &info)) {
				if (demuxer->duration>0) {
					int volume_sizemb   = priv->pvr_config.volume_sizemb;
					int volume_maxnum   = priv->pvr_config.volume_maxnum;
					int volume_maxtimes = priv->pvr_config.volume_maxtimes;

					if ((info.size > 0) && (volume_maxnum > 0)) {
						uint64_t seek_minims = 0;
						int64_t volume_seekmb = 0;

						volume_seekmb = (int64_t)(volume_maxnum - 1) * volume_sizemb * (1024 * 1024);
						if ((volume_seekmb > 0) && (info.truesize > volume_seekmb))
							info.truesize = volume_seekmb;
						seek_minims = (double)demuxer->duration * (info.size - info.truesize) / info.size;
						if (volume_maxtimes > 0) {
							if (demuxer->duration > (1000 * volume_maxtimes)) {
								uint64_t seek_ms = demuxer->duration - (1000 * volume_maxtimes);
								seek_minims = (seek_minims > seek_ms) ? seek_minims : seek_ms;
							}
						}
						*(uint64_t*)arg = seek_minims;
					}else{
						*(uint64_t*)arg = 0;
					}
				}
				return GX_DEMUXER_CTRL_OK;
			}
			else
				return GX_DEMUXER_CTRL_NOTIMPL;
		}
	case GX_DEMUXER_CTRL_GET_CURRENT_TIME:
		{
			DemuxTSPriv* priv = (DemuxTSPriv*)demuxer->priv;

			if (demuxer->stream->file_format == GX_STREAMTYPE_STREAM)
				return GX_DEMUXER_CTRL_NOTIMPL;

			if (priv->time_insert && demuxer->durstream) {
				off_t cur_pos  = 0, es_len = 0, pkt_len = 0;
				off_t end_pos  = 0, estimate_len = 0;
				off_t tell_pos = GxStream_Tell(demuxer->stream);

				if (demuxer->video && (demuxer->video->fill_pkt_sync_pos > 0)) {
					cur_pos = demuxer->video->fill_pkt_sync_pos;
					es_len  = GxDemuxStream_GetEsBufSize(demuxer->video);
					pkt_len = priv->video_lastseekbytes;
				} else if (demuxer->audio && (demuxer->audio->fill_pkt_sync_pos > 0)) {
					cur_pos = demuxer->audio->fill_pkt_sync_pos;
					es_len  = GxDemuxStream_GetEsBufSize(demuxer->audio);
					pkt_len = priv->audio_lastseekbytes;
				}

				if (pkt_len > 0) {
					int64_t dis_len = (int64_t)(tell_pos - priv->lastseekpos);
					if ((dis_len >> 30) && (pkt_len >> 30)) {
						dis_len = (dis_len >> 30);
						pkt_len = (pkt_len >> 30);
					}
					estimate_len = (es_len * dis_len / pkt_len);
					estimate_len = (estimate_len < es_len) ? es_len : estimate_len;
				}

				cur_pos = (cur_pos < estimate_len) ? 0 : (cur_pos - estimate_len);
				GxStream_Control(demuxer->durstream, GX_STREAM_CTRL_GET_SIZE, &end_pos);
				if (demuxer->duration > 0 && (end_pos > 0)) {
					*(int64_t*)arg = (double)demuxer->duration * cur_pos / end_pos;
					if (cur_pos > demuxer->tell_pos) {
						if (*(int64_t*)arg < demuxer->cur_timems)
							*(int64_t*)arg = demuxer->cur_timems;
					} else if (cur_pos == demuxer->tell_pos) {
						*(int64_t*)arg = demuxer->cur_timems;
					} else {
						if (*(int64_t*)arg > demuxer->cur_timems)
							*(int64_t*)arg = demuxer->cur_timems;
					}
					demuxer->cur_timems = *(int64_t*)arg;
					demuxer->tell_pos = cur_pos;
				}

				return GX_DEMUXER_CTRL_OK;
			}
			else
				return GX_DEMUXER_CTRL_NOTIMPL;
		}
	case GX_DEMUXER_CTRL_GET_CURRENT_PERCENT:
		{
			off_t cur_pos, end_pos;
			cur_pos  = GxStream_Tell(demuxer->stream);
			cur_pos -= GxDemuxer_GetInBufSize(demuxer);
			if (demuxer->stream->file_format == GX_STREAMTYPE_STREAM)
				GxStream_Control(demuxer->stream, GX_STREAM_CTRL_GET_SIZE, &end_pos);
			else
				GxStream_Control(demuxer->durstream, GX_STREAM_CTRL_GET_SIZE, &end_pos);
			*(uint8_t*)arg = (uint8_t)100 * cur_pos / end_pos;
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_GET_TIME_LENGTH:
		{
			if (demuxer->stream->file_format == GX_STREAMTYPE_STREAM)
			{
				int total = 0;
				int ret = GxStream_Control(demuxer->stream, GX_STREAM_CTRL_GET_TOTAL_TIME, &total);
				if (ret == GX_PLAYER_OK)
					demuxer->duration = *(int64_t*)arg = total*1000;
				else
					*(int64_t*)arg = demuxer->duration;
			}
			else {
				uint64_t duration = ts_get_duration(demuxer);
				if (duration > 0)
					demuxer->duration = duration;
				*(uint64_t*)arg = (uint64_t)(demuxer->duration);
			}
			if (demuxer->duration > 0)
			{
				if (demuxer->video->sh && priv)
				{
					GxStreamVideoHeader *hdr = demuxer->video->sh;
					hdr->i_bps = demuxer->movi_end/demuxer->duration;
					priv->bps =  hdr->i_bps;
				}
				else if (demuxer->audio->sh && priv)
				{
					GxStreamAudioHeader *hdr = demuxer->audio->sh;
					hdr->i_bps = demuxer->movi_end/demuxer->duration;
					priv->bps =  hdr->i_bps;
				}
			}
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_FOURC_STOP:
		{
			if(priv) priv->abort = *(int *)arg;
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_SWITCH_AUDIO:
	case GX_DEMUXER_CTRL_SWITCH_VIDEO:
		{
			int pid, reftype, areset = 0, vreset = 0;
			GxDemuxStream* ds;
			GxDemuxerStreamSwitch* StreamSwitch = arg;

			if (!arg)
				return GX_DEMUXER_CTRL_ERROR;

			pid = StreamSwitch->pid;

			if (cmd == GX_DEMUXER_CTRL_SWITCH_VIDEO)
			{
				reftype = TYPE_VIDEO;
				ds = demuxer->video;
				priv->ts_v_pid = pid;
				vreset = 1;
			}
			else
			{
				reftype = TYPE_AUDIO;
				ds = demuxer->audio;
				priv->ts_a_pid = pid;
				areset = 1;
			}

			if (VAILD_PID(pid) && priv->ts.streams[pid].sh && priv->ts.streams[pid].type == reftype) {
				if (ds->id != priv->ts.streams[pid].id) {
					reset_fifos(priv, areset, vreset, 0, areset);
					ds->id = priv->ts.streams[pid].id;
					ds->sh = priv->ts.streams[pid].sh;
					GxDemuxStream_FreePacks(ds);
				}

				if (ds->sh && reftype == TYPE_AUDIO) {
					GxStreamAudioHeader *sh_audio = ds->sh;

					if (sh_audio->format == AUDIO_CODEC_PCM) {
						while (sh_audio->samplerate == 0) {
							if (ts_interruptcbk && ts_interruptcbk())
								break;
							demux_ts_fill_buffer(demuxer, NULL);
						}
					}
				}
			}
			else {
				reset_fifos(priv, areset, vreset, 0, areset);
				ds->id = -2;
				ds->sh = NULL;
				GxDemuxStream_FreePacks(ds);
				if (cmd == GX_DEMUXER_CTRL_SWITCH_VIDEO)
					priv->ts_v_pid = -2;
				else
					priv->ts_a_pid = -2;
			}

			return GX_DEMUXER_CTRL_OK;
		}

	case GX_DEMUXER_CTRL_IDENTIFY_PROGRAM:	//returns in prog->{aid,vid} the new ids that comprise a program
		{
			int i, j, cnt = 0;
			int vid_done = 0, aid_done = 0;
			TSPMT* pmt = NULL;
			TSProgram* prog = (TSProgram *) arg;

			if (priv->pmt_cnt < 2)
				return GX_DEMUXER_CTRL_ERROR;

			if (prog->progid == -1) {
				int cur_pmt_idx = 0;

				for (i = 0; i < priv->pmt_cnt; i++)
					if (priv->pmt[i].progid == priv->prog) {
						cur_pmt_idx = i;
						break;
					}

				i = (cur_pmt_idx + 1) % priv->pmt_cnt;
				while (i != cur_pmt_idx) {
					pmt = &priv->pmt[i];
					cnt = is_usable_program(priv, pmt);
					if (cnt)
						break;
					i = (i + 1) % priv->pmt_cnt;
				}
			} else {
				for (i = 0; i < priv->pmt_cnt; i++)
					if (priv->pmt[i].progid == prog->progid) {
						pmt = &priv->pmt[i];	//required program
						cnt = is_usable_program(priv, pmt);
					}
			}

			if (!cnt)
				return GX_DEMUXER_CTRL_ERROR;

			//finally some food
			prog->aid = prog->vid = -2;	//no audio and no video by default
			for (j = 0; j < pmt->es_cnt; j++) {
				if (priv->ts.streams[pmt->es[j].pid].sh == NULL)
					continue;

				if (!vid_done && priv->ts.streams[pmt->es[j].pid].type == TYPE_VIDEO) {
					vid_done = 1;
					prog->vid = priv->ts.streams[pmt->es[j].pid].id;
				} else if (!aid_done && priv->ts.streams[pmt->es[j].pid].type == TYPE_AUDIO) {
					aid_done = 1;
					prog->aid = priv->ts.streams[pmt->es[j].pid].id;
				}
			}

			priv->prog = prog->progid = pmt->progid;
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_GET_TSRECORDINFO:
		{
			if (priv && arg) {
				PlayerRecordConfig *config = (PlayerRecordConfig *)arg;

				memcpy(&config->ext_info, &priv->hw.ts_info.ext_info,  sizeof(GxDemuxerRecordExtInfo));
				memcpy(&config->ext_data, &priv->hw.ts_info.user_info, sizeof(GxDemuxerRecordUserInfo));
			}
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_SET_PLAY_TS_CONFIG:
		{
			int i = 0;
			GxDemuxerPlayTsExtInfo* config = (GxDemuxerPlayTsExtInfo*)arg;
			if (priv && config) {
				for (i = 0; i < PLAYER_FILTER_MAX; i++) {
					if (config->callback[i].writeback) {
						priv->filter.data[i].pid  = config->callback[i].pid;
						priv->filter.data[i].type = config->callback[i].type;
						priv->filter.data[i].crc  = config->callback[i].crc;
						priv->filter.data[i].writeback = config->callback[i].writeback;
						priv->filter.data[i].using= 1;
					}
				}
			}
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_SUB_PSI_ON:
	case GX_DEMUXER_CTRL_SUB_ON:
		{
			int pid = *(int*)arg;

			if (!VAILD_PID(pid))
				return GX_DEMUXER_CTRL_ERROR;

			if (!priv->sub_filter.fifo)
				priv->sub_filter.fifo = GxFifo_Create(128*1024, GX_PINFLAG_SW);
			else
				GxFifo_Reset(priv->sub_filter.fifo);

			if (priv->sub_filter.fifo) {
				if (!priv->sub_filter.data) {
					priv->sub_filter.data = av_malloc(SUB_FILTER_SIZE);
					if (priv->sub_filter.data == NULL) {
						GxFifo_Destroy(priv->sub_filter.fifo);
						priv->sub_filter.fifo = NULL;
						return GX_DEMUXER_CTRL_ERROR;
					}
				}
				priv->sub_filter.len = 0;
				priv->sub_filter.pid = pid;
				return GX_DEMUXER_CTRL_OK;
			}
			return GX_DEMUXER_CTRL_ERROR;
		}
	case GX_DEMUXER_CTRL_SUB_PSI_OFF:
	case GX_DEMUXER_CTRL_SUB_OFF:
		{
			priv->sub_filter.pid = 0;
			if (priv->sub_filter.fifo) {
				GxFifo_Destroy(priv->sub_filter.fifo);
				priv->sub_filter.fifo = NULL;
			}
			if (priv->sub_filter.data) {
				av_free(priv->sub_filter.data);
				priv->sub_filter.data = NULL;
			}
			priv->sub_filter.len = 0;
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_SWITCH_SUB:
		{
			int pid = *(int*)arg;

			if (!VAILD_PID(pid))
				return GX_DEMUXER_CTRL_ERROR;

			GxFifo_Reset(priv->sub_filter.fifo);
			priv->sub_filter.pid = pid;
			priv->sub_filter.len = 0;
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_SUB_PSI_READ_DATA:
	case GX_DEMUXER_CTRL_SUB_READ_DATA:
		{
			GxDemuxerPesData* data = (GxDemuxerPesData*)arg;
			GxDemuxerDataType type;

			if (data == NULL || data->buffer == NULL)
				return GX_DEMUXER_CTRL_ERROR;

			if (!VAILD_PID(priv->sub_filter.pid) ||
					(priv->sub_filter.data == NULL) ||
					(priv->sub_filter.fifo == NULL)) {
				data->size = 0;
				return GX_DEMUXER_CTRL_ERROR;
			}

			if (cmd == GX_DEMUXER_CTRL_SUB_PSI_READ_DATA)
				type = DEMUX_PSI_TYPE;
			else
				type = DEMUX_PES_TYPE;
			data->size = ts_filter_sub(&priv->sub_filter, data->buffer, data->size, type);
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_OPEN_AD_AUDIO:
		{
			GxDemuxerStreamSwitch* StreamSwitch= (GxDemuxerStreamSwitch*)arg;
			if (StreamSwitch) {
				GxStreamAudioHeader *sh_audio1=NULL;
				int ad_apid  = StreamSwitch->pid;
				int ad_atype = 0;
				uint8_t extra_type = 0;

				if (!VAILD_PID(ad_apid))
					return GX_DEMUXER_CTRL_ERROR;

				if (!HAVE_AUDIO(demuxer))
					return GX_DEMUXER_CTRL_ERROR;

				if (HAVE_AUDIO1(demuxer))
					return GX_DEMUXER_CTRL_ERROR;

				pid_extra_type_from_pmt(priv, ad_apid, &extra_type);
				if (extra_type != EXTRA_VISUAL_IMPAIRED)
					return GX_DEMUXER_CTRL_ERROR;

				ad_atype = pid_type_from_pmt(priv, ad_apid);
				sh_audio1 = priv->ts.streams[ad_apid].sh;
				if (sh_audio1) {
					sh_audio1->ds     = demuxer->audio1;
					if (ad_atype == AUDIO_AAC)
						sh_audio1->format = AUDIO_CODEC_AAC_LATM;
					else if (ad_atype == AUDIO_ADTS)
						sh_audio1->format = AUDIO_CODEC_AAC_ADTS;
					else
						sh_audio1->format = acodec_fourcc2std(ad_atype);
					demuxer->audio1->id = priv->ts.streams[ad_apid].id;
					demuxer->audio1->sh = sh_audio1;
					priv->audio1_running = 0;
				}

				return GX_DEMUXER_CTRL_OK;
			}
		}
	case GX_DEMUXER_CTRL_CLOSE_AD_AUDIO:
		{
			if (!HAVE_AUDIO(demuxer))
				return GX_DEMUXER_CTRL_OK;

			demuxer->audio1->sh     = NULL;
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_RUN_AD_AUDIO:
		{
			GxDemuxerStreamSwitch* StreamSwitch= (GxDemuxerStreamSwitch*)arg;
			GxStreamAudioHeader *sh_audio1=NULL;

			if (StreamSwitch) {
				int ad_apid  = StreamSwitch->pid;

				if (!demuxer->audio1->pin)
					return GX_DEMUXER_CTRL_ERROR;

				if (!VAILD_PID(ad_apid))
					return GX_DEMUXER_CTRL_ERROR;

				if (priv->audio1_running == 0) {
					sh_audio1 = priv->ts.streams[ad_apid].sh;
					if (sh_audio1 && demuxer->audio1->sh == sh_audio1) {
						if (demuxer->audio1->pin->fifo) {
							GxFifoConfig FifoConfig;
							FifoConfig.empty_gate = 7*demuxer->audio1->pin->fifo->size/8;
							FifoConfig.full_gate = 1024;
							GxFifo_Config(demuxer->audio1->pin->fifo, &FifoConfig);
							GxFifo_Reset(demuxer->audio1->pin->fifo);
						}

						reset_fifos(priv, 0, 0, 0, 1);
						GxDemuxStream_FreePacks(demuxer->audio1);
						demuxer->audio1->wbytes = 0;
						priv->audio1_running    = 1;
						return GX_DEMUXER_CTRL_OK;
					}
				}
			}
			return GX_DEMUXER_CTRL_ERROR;
		}
		case GX_DEMUXER_CTRL_STOP_AD_AUDIO:
		{
			priv->audio1_running = 0;
			return GX_DEMUXER_CTRL_OK;
		}
		case GX_DEMUXER_CTRL_GET_PROG_STREAM_INFO:
		{
			unsigned int i = 0;
			PlayerProgStreamInfo *info = (PlayerProgStreamInfo *)arg;

			for (i = 0; i < priv->pmt_cnt; i++) {
				TSPMT *pmt = &priv->pmt[i];
				unsigned int vc = 0, ac = 0, sc = 0;
				unsigned int k = 0, j = 0;

				if (i >= PLAYER_MAX_PROG_COUNT)
					break;

				for (k = 0; k < 4; k++) {
					for (j = 0; j < pmt->es_cnt; j++) {
						TSPMTES *es = &pmt->es[j];

						switch (es->type) {
						case VIDEO_MPEG1:
						case VIDEO_MPEG2:
						case VIDEO_MPEG4:
						case VIDEO_H264:
						case VIDEO_AVC:
							if (vc == 0) {
								if (es->type == VIDEO_HEVC)
									info->prog[i].video_stream.codec_type = VIDEO_CODEC_H265;
								else if ((es->type == VIDEO_H264) || (es->type == VIDEO_AVC))
									info->prog[i].video_stream.codec_type = VIDEO_CODEC_H264;
								else if (es->type == VIDEO_MPEG4)
									info->prog[i].video_stream.codec_type = VIDEO_CODEC_MPEG4;
								else if (es->type == VIDEO_VC1)
									info->prog[i].video_stream.codec_type = VIDEO_CODEC_VC1;
								else if (es->type == VIDEO_AVS)
									info->prog[i].video_stream.codec_type = VIDEO_CODEC_AVS;
								else
									info->prog[i].video_stream.codec_type = VIDEO_CODEC_MPEG12;
								info->prog[i].video_stream.pid        = es->pid;
								memcpy(info->prog[i].video_stream.lang, es->lang, 4);
								vc++;
							}
							break;
						case AUDIO_MP2:
						case AUDIO_A52:
						case AUDIO_DTS:
						case AUDIO_LPCM_BE:
						case AUDIO_AAC:
						case AUDIO_ADTS:
						case AUDIO_DRA1:
						case AUDIO_OPUS:
							if (ac < PLAYER_MAX_PROG_STREAM_COUNT) {
								if (es->type == AUDIO_MP2)
									info->prog[i].audio_stream[ac].codec_type = AUDIO_CODEC_MPEG2;
								else if (es->type == AUDIO_A52)
									info->prog[i].audio_stream[ac].codec_type = AUDIO_CODEC_AC3;
								else if (es->type == AUDIO_DTS)
									info->prog[i].audio_stream[ac].codec_type = AUDIO_CODEC_DTS;
								else if (es->type == AUDIO_LPCM_BE)
									info->prog[i].audio_stream[ac].codec_type = AUDIO_CODEC_PCM;
								else if (es->type == AUDIO_ADTS)
									info->prog[i].audio_stream[ac].codec_type = AUDIO_CODEC_AAC_ADTS;
								else if (es->type == AUDIO_DRA1)
									info->prog[i].audio_stream[ac].codec_type = AUDIO_CODEC_DRA1;
								else if (es->type == AUDIO_OPUS)
									info->prog[i].audio_stream[ac].codec_type = AUDIO_CODEC_OPUS;
								info->prog[i].audio_stream[ac].pid        = es->pid;
								memcpy(info->prog[i].audio_stream[ac].lang, es->lang, 4);
								ac++;
							}
							break;
						case SPU_DVD:
						case SPU_DVB:
						case SPU_SCTE:
						case SPU_ARIB_A:
						case SPU_ARIB_C:
							if (sc < PLAYER_MAX_PROG_STREAM_COUNT) {
								if (es->type == SPU_DVD) {
									info->prog[i].subtitle_stream[sc].codec_type = SUBTITLE_CODEC_VOB;
									info->prog[i].subtitle_stream[sc].type       = PLAYER_SUB_TYPE_INSIDE;
								} else if (es->type == SPU_DVB) {
									info->prog[i].subtitle_stream[sc].codec_type = SUBTITLE_CODEC_DVB;
									info->prog[i].subtitle_stream[sc].type       = PLAYER_SUB_TYPE_DVB;
								} else if (es->type == SPU_SCTE) {
									info->prog[i].subtitle_stream[sc].codec_type = SUBTITLE_CODEC_SCTE_CC;
									info->prog[i].subtitle_stream[sc].type       = PLAYER_SUB_TYPE_SCTE;
								} else if (es->type == SPU_ARIB_A || es->type == SPU_ARIB_C) {
									info->prog[i].subtitle_stream[sc].codec_type = SUBTITLE_CODEC_ARIB_CC;
									info->prog[i].subtitle_stream[sc].type       = PLAYER_SUB_TYPE_ARIB_CC;
								}
								info->prog[i].subtitle_stream[sc].pid  = es->pid;
								memcpy(info->prog[i].subtitle_stream[sc].lang, es->lang, 4);
								sc++;
							}
							break;
						case SPU_TXT:
							{
								int l = 0;

								for (l = 0; l < es->sub_es_num; l++) {
									if (sc >= PLAYER_MAX_PROG_STREAM_COUNT)
										break;

									info->prog[i].subtitle_stream[sc].pid   = es->pid;
									info->prog[i].subtitle_stream[sc].major = es->sub_es_stream[l].major;
									info->prog[i].subtitle_stream[sc].minor = es->sub_es_stream[l].minor;
									if (es->sub_es_stream[l].type == 0x01)
										info->prog[i].subtitle_stream[sc].type   = PLAYER_SUB_TYPE_DVB_MAG;
									else
										info->prog[i].subtitle_stream[sc].type   = PLAYER_SUB_TYPE_DVB_TTX;
									info->prog[i].subtitle_stream[sc].codec_type = SUBTITLE_CODEC_TELETEXT;
									memcpy(info->prog[i].subtitle_stream[sc].lang, es->sub_es_stream[l].lang, 4);
									sc++;
								}
							}
							break;
						default:
							break;
						}
					}
				}
				info->prog[i].prog_id = pmt->progid;
				info->prog[i].video_stream_count    = vc;
				info->prog[i].audio_stream_count    = ac;
				info->prog[i].subtitle_stream_count = sc;
			}
			info->prog_count = priv->pmt_cnt;
			break;
		}
		default:
		return GX_DEMUXER_CTRL_NOTIMPL;
	}
	return GX_DEMUXER_CTRL_OK;
}

static int demux_ts_sync_iframe(GxDemuxer* demuxer)
{
	int ret = 0;
	off_t pos = 0;
	off_t init_pos = GxStream_Tell(demuxer->stream);

	if (!HAVE_VIDEO(demuxer))
		return 0;

	while (!GxStream_Eof(demuxer->stream)) {
		if (ts_interruptcbk && ts_interruptcbk()) {
			ret = 0;
			goto SYNC_IFRAME;
		}

		pos = GxStream_Tell(demuxer->stream);
		if (pos - init_pos < TS_MAX_PROBE_SIZE) {
			int size;
			unsigned char* start = NULL;

			size = GxDemuxStream_GetPacket(demuxer->video, &start);
			if (size <= 0) {
				ret = 0;
				goto SYNC_IFRAME;
			}

			if (avpacket_parse_header(demuxer, start, size) != 0) {
				ret = 1;
				goto SYNC_IFRAME;
			}
		}
		else {
			ret = 0;
			goto SYNC_IFRAME;
		}
	}

SYNC_IFRAME:
	if (demuxer->audio != NULL)
		GxDemuxStream_FreePacks(demuxer->audio);
	if (demuxer->video != NULL)
		GxDemuxStream_FreePacks(demuxer->video);
	if (demuxer->sub != NULL)
		GxDemuxStream_FreePacks(demuxer->sub);

	GxStream_Reset(demuxer->stream);
	GxStream_Seek(demuxer->stream, 0);

	return ret;
}

static GxDemuxer* demux_ts_open(GxDemuxer* demuxer)
{
	int i, ret;
	GxStreamVideoHeader *sh_video=NULL;
	GxStreamAudioHeader *sh_audio=NULL;
	GxStreamAudioHeader *sh_audio1=NULL;
	GxStreamSubHeader   *sh_sub=NULL;
	int start_pos;
	int sync = 0;
	int cur_seq_no = 0;
	TSDemuxInit params;
	DemuxTSPriv *priv = demuxer->priv;

	/*try analysis NC-private TS info*/
	GxStream_Reset(demuxer->stream);
	GxStream_Seek(demuxer->stream, 0);

	if (demuxer->stream->no_check_priv_packet) {
		priv->time_insert = 0;
	}else{
		int hdr_size = 0;
		GxStream_Control(demuxer->stream, GX_STREAM_CTRL_PROBE_HDR, &hdr_size);
		if (hdr_size > 0) {
			struct vol_hdr hdr;

			hdr.size   = hdr_size;
			hdr.buffer = av_malloc(hdr_size);
			GxStream_Control(demuxer->stream, GX_STREAM_CTRL_READ_HDR, &hdr);
			ret = GxDemuxer_PrivateAnalysis(hdr.buffer, hdr.size, &priv->hw.ts_info, &priv->pvr_config);
		} else {
			sync = GxDemuxer_PrivatePacketSyncExtInfo(demuxer->stream);
			if (!sync)
				GxStream_Seek(demuxer->stream, 0);
			ret = GxDemuxer_PrivatePacketTryAnalysis(demuxer->stream, &priv->hw.ts_info, &priv->pvr_config);
		}
		if (ret == GX_PLAYER_OK) {
			if (VAILD_PID(priv->hw.ts_info.timestamp_pid)) {
				if (demuxer->info.audio_pid < 0)
					demuxer->info.audio_pid = priv->hw.ts_info.a_pid;
				if (demuxer->info.video_pid < 0)
					demuxer->info.video_pid = priv->hw.ts_info.v_pid;
				priv->time_insert = 1;
			}else
				priv->time_insert = 0;
		}
	}
	gxlogf("====================== time_insert = %d\n", priv->time_insert);

	if (!priv->hw.ts_info.ext_info.is_encrypt) {
		for (i = 0; i < NB_PID_MAX; i ++)
			priv->ts.streams[i].id = -3;

		demux_avfifo_reset(demuxer);
		GxParseES_Init(&priv->es_parser, demuxer->video);
		priv->ts.pids_hdr = av_malloc(sizeof(ESStream));
		memset(priv->ts.pids_hdr,0,sizeof(ESStream));
		priv->ts.pids_hdr->tail = priv->ts.pids_hdr;

		priv->pat.progs = NULL;
		priv->pat.progs_cnt = 0;
		priv->pat.section.buffer = NULL;
		priv->pat.section.buffer_len = 0;

		priv->pmt = NULL;
		priv->pmt_cnt = 0;
		priv->pmt_index = -1;
		priv->keep_broken = 0;
		priv->last_pid = NB_PID_MAX;

		priv->pat.section.buffer_len = 0;
		for (i = 0; i < priv->pmt_cnt; i++)
			priv->pmt[i].section.buffer_len = 0;

		priv->ts_start_v_pts = -1;
		priv->ts_start_a_pts = -1;
		priv->ts_start_pts = -1;

		if (demuxer->stream->prog_max > 0) {
			priv->atype = demuxer->stream->prog[demuxer->stream->prog_now].acodec;
			priv->vtype = demuxer->stream->prog[demuxer->stream->prog_now].vcodec;
		} else {
			priv->atype = UNKNOWN;
			priv->vtype = UNKNOWN;
		}

		memset(&params, 0, sizeof(params));
		params.atype = params.vtype = params.stype = params.ad_atype = UNKNOWN;
		params.apid    = (demuxer->info.audio_pid > 0) ? demuxer->info.audio_pid  : -1;
		params.vpid    = (demuxer->info.video_pid > 0) ? demuxer->info.video_pid  : -1;
		params.spid    = (demuxer->info.sub_pid   > 0) ? demuxer->info.sub_pid    : -1;
		params.ad_apid = (demuxer->info.ad_pid    > 0) ? demuxer->info.ad_pid : -1;
		params.prog  = 0;
		params.probe = 0;

		if (demuxer->stream->file_format == GX_STREAMTYPE_FILE) {
			ts_prepare_detect(demuxer);
			GxStream_Reset(demuxer->stream);
			GxStream_Seek(demuxer->stream, 0);
		}

		start_pos = ts_detect_streams(demuxer, &params);
		if ((params.apid <= 0 && params.vpid <= 0) || (params.atype == AUDIO_OPUS) || priv->abort || start_pos < 0) {
			demux_ts_close(demuxer);
			return NULL;
		}

		gxlogd_pmt_info(priv);

		priv->prog = params.prog;
		priv->ts_a_pid = params.apid;
		priv->ts_v_pid = params.vpid;

		ESStream* es = priv->ts.pids_hdr->next;
		while (es) {
			if ((params.prog == 0) || ((params.vpid != -1) && (params.prog != progid_for_pid(priv, params.vpid, 0)))) {
				if (es->pid == params.apid &&
						(es->pid == demuxer->info.audio_pid || demuxer->info.audio_pid== -1)) {
					if (es->pid == params.apid && params.atype == UNKNOWN)
						params.atype = IS_AUDIO(es->type) ? es->type : es->subtype;
					ts_add_stream(demuxer, es);
				} else if (es->pid == params.vpid &&
						(es->pid == demuxer->info.video_pid || demuxer->info.video_pid == -1)) {
					if (es->pid == params.vpid && params.vtype == UNKNOWN)
						params.vtype = IS_VIDEO(es->type) ? es->type : es->subtype;
					ts_add_stream(demuxer, es);
				} else if (es->pid == params.spid) {
					ts_add_stream(demuxer, es);
				} else if ((params.ad_apid != -1) && (es->pid == params.ad_apid)) {
						params.ad_atype = es->type;
						params.ad_apid  = es->pid;
				}
				priv->prog = params.prog = 0;
			} else {
				int idx = progid_idx_in_pmt(priv, params.prog);
				for (i = 0; i < priv->pmt[idx].es_cnt; i++) {
					if (es->pid == priv->pmt[idx].es[i].pid) {
						ts_add_stream(demuxer, es);
						if ((params.ad_apid != -1) && (es->pid == params.ad_apid)) {
							params.ad_atype = es->type;
							params.ad_apid  = es->pid;
						}
					}
				}
			}
			es = es->next;
		}

		gxlogf("=====> vtype:0x%x atype:0x%x adtype:0x%x\n", params.vtype, params.atype, params.ad_atype);
		if (params.vtype != UNKNOWN) {
			if ((priv->time_insert && params.vpid == demuxer->info.video_pid) || !priv->time_insert) {
				sh_video = priv->ts.streams[params.vpid].sh;
				if (sh_video) {
					ESStream* es = new_pid(priv,params.vpid);
					ts_add_stream(demuxer, es);
					demuxer->video->id = priv->ts.streams[params.vpid].id;
					sh_video->ds = demuxer->video;
					sh_video->format = vcodec_fourcc2std(params.vtype);
					demuxer->video->sh = sh_video;
				} else {
					params.vtype = UNKNOWN;
				}
			}else{
				params.vtype = UNKNOWN;
			}
		}

		if (params.atype != UNKNOWN) {
			if ((priv->time_insert && params.apid == demuxer->info.audio_pid) || !priv->time_insert) {
				sh_audio = priv->ts.streams[params.apid].sh;
				if (sh_audio && (sh_audio->audio_id == 0)) {
					ESStream* es = new_pid(priv,params.apid);
					if (!IS_AUDIO(es->type) && !IS_AUDIO(es->subtype) && IS_AUDIO(params.atype))
						es->subtype = params.atype;
					ts_add_stream(demuxer, es);
					demuxer->audio->id = priv->ts.streams[params.apid].id;
					sh_audio->ds = demuxer->audio;
					if (params.atype == AUDIO_AAC)
						sh_audio->format = AUDIO_CODEC_AAC_LATM;
					else if (params.atype == AUDIO_ADTS)
						sh_audio->format = AUDIO_CODEC_AAC_ADTS;
					else
						sh_audio->format = acodec_fourcc2std(params.atype);
					demuxer->audio->sh = sh_audio;
				} else
					params.atype = UNKNOWN;
			}else{
				params.atype = UNKNOWN;
			}
		}

		if ((params.ad_atype != UNKNOWN) && (params.ad_apid > 0)) {
			sh_audio1 = priv->ts.streams[params.ad_apid].sh;
			if (sh_audio1 && (sh_audio1->audio_id == 1)) {
				ESStream* es = new_pid(priv, params.ad_apid);
				if (!IS_AUDIO(es->type) && !IS_AUDIO(es->subtype) && IS_AUDIO(params.ad_atype))
					es->subtype = params.ad_atype;
				ts_add_stream(demuxer, es);
				demuxer->audio1->id = priv->ts.streams[params.ad_apid].id;
				sh_audio1->ds = demuxer->audio1;
				if (params.ad_atype == AUDIO_AAC)
					sh_audio1->format = AUDIO_CODEC_AAC_LATM;
				else if (params.ad_atype == AUDIO_ADTS)
					sh_audio1->format = AUDIO_CODEC_AAC_ADTS;
				else
					sh_audio1->format = acodec_fourcc2std(params.ad_atype);
				demuxer->audio1->sh = sh_audio1;
				priv->audio1_running = 1;
			} else
				params.ad_atype = UNKNOWN;
		} else
			params.ad_atype = UNKNOWN;

		GxStream_Reset(demuxer->stream);
		GxStream_Seek(demuxer->stream, 0);
		demux_ts_sync_iframe(demuxer);

		if (demuxer->stream->file_format == GX_STREAMTYPE_STREAM) {
			int total = 0;
			demuxer->start_pts = -1;

			while (demuxer->start_pts == -1 && !GxStream_Eof(demuxer->stream)) {
				if (ts_interruptcbk && ts_interruptcbk()) {
					demux_ts_close(demuxer);
					return NULL;
				}
				demux_ts_fill_buffer(demuxer,NULL);
				if (sh_audio != NULL)
					GxDemuxStream_FreePacks(demuxer->audio);
				if (sh_video != NULL)
					GxDemuxStream_FreePacks(demuxer->video);
				if (sh_sub != NULL)
					GxDemuxStream_FreePacks(demuxer->sub);
			}

			if ((demuxer->stream->flags & GX_STREAM_SEEK_TIME) != GX_STREAM_SEEK_TIME) {
				priv->ts_start_pts = (demuxer->start_pts == -1)?0:demuxer->start_pts;
				ret = GxStream_Control(demuxer->stream, GX_STREAM_CTRL_GET_TOTAL_TIME, &total);
				if (ret != GX_PLAYER_OK) {
					if (priv->ts_a_pid > 0) {
						priv->ts_dur_pid = priv->ts_a_pid;
						priv->ts_dur_probe_size = 8192*4;
					} else if (priv->ts_v_pid > 0) {
						priv->ts_dur_pid = priv->ts_v_pid;
						priv->ts_dur_probe_size = 81920;
					}
					demuxer->durstream = demuxer->stream;
					demuxer->duration  = ts_get_duration(demuxer);
					demuxer->durstream = NULL;
				}
			}

			GxStream_Reset(demuxer->stream);
			GxStream_Seek(demuxer->stream, 0);
			if (GxStream_Control(demuxer->stream, GX_STREAM_CTRL_GET_CUR_SEQ, &cur_seq_no) == GX_PLAYER_OK) {
				if (cur_seq_no != demuxer->stream->cur_seq_no) {
					cur_seq_no  = demuxer->stream->cur_seq_no;
					GxStream_Control(demuxer->stream, GX_STREAM_CTRL_CACHE_RESET, &cur_seq_no);
				}
			}
			return demuxer;
		}

		start_pos = (start_pos <= priv->ts.packet_size ? 0 : start_pos - priv->ts.packet_size);
		demuxer->movi_start = start_pos;
		GxStream_Reset(demuxer->stream);
		GxStream_Seek(demuxer->stream,start_pos);

		{
			int i = 0;
			int index = priv->pmt_index < 0 ? 0 : priv->pmt_index;
			TSPMT *pmt = index < priv->pmt_cnt ? &priv->pmt[index] : NULL;

re_find_audio:
			if (params.atype != UNKNOWN) {
				off_t init_pos = GxStream_Tell(demuxer->stream);
				off_t end_pos = init_pos + (params.probe ? params.probe : TS_MAX_PROBE_SIZE);

				while (priv->ts_start_a_pts == -1 && !GxStream_Eof(demuxer->stream)) {
					sh_audio = demuxer->audio->sh;

					if (ts_interruptcbk && ts_interruptcbk()) {
						demux_ts_close(demuxer);
						return NULL;
					}

					demux_ts_fill_buffer(demuxer,NULL);
					if (sh_audio != NULL)
						GxDemuxStream_FreePacks(demuxer->audio);
					if (sh_video != NULL)
						GxDemuxStream_FreePacks(demuxer->video);
					if (sh_sub != NULL)
						GxDemuxStream_FreePacks(demuxer->sub);

					if (sh_audio && sh_audio->format == AUDIO_CODEC_PCM) {
						if ((sh_audio->samplerate == 0) || (sh_audio->channels == 0))
							priv->ts_start_a_pts = -1;
					}

					/*
					 * the a_pid/v_pid in the ts stream of the dvb program may have some
					 * problems sometimes
					 * so, we should make condition judgment if we want the priv->ts_start_pts
					 * if not, it may be into the loop
					 */
					if (demuxer->stream->pos > end_pos) {
						/*
						 * the video pid or audio pid may be wrong
						 * the programs info parsed by gx_demux_ts is not accurate
						 */
						if (pmt && pmt->es_cnt > 1) {
							for (; i < pmt->es_cnt; i++) {
								if (IS_AUDIO(pmt->es[i].type) && pmt->es[i].pid != priv->ts_a_pid) {
									int apid_new = pmt->es[i].pid;

									if (priv->ts.streams[apid_new].sh) {
										priv->ts_a_pid = apid_new;
										demuxer->info.audio_pid = apid_new;
										demuxer->audio->id = priv->ts.streams[apid_new].id;
										demuxer->audio->sh = priv->ts.streams[apid_new].sh;
										GxStream_Reset(demuxer->stream);
										GxStream_Seek(demuxer->stream, init_pos);
										goto re_find_audio;
									}
								}
							}
						}
					#if 0
						params.atype = UNKNOWN;
						demuxer->audio->dropmode = DROPMODE_UNSUPPORT;
						demuxer->audio->sh = NULL;
					#endif
						break;
					}
				}
			}
		}

		{
			int i = 0;
			int index = priv->pmt_index < 0 ? 0 : priv->pmt_index;
			TSPMT *pmt = index < priv->pmt_cnt ? &priv->pmt[index] : NULL;
re_find_video:
			if (params.vtype != UNKNOWN) {
				off_t init_pos = GxStream_Tell(demuxer->stream);
				off_t end_pos = init_pos + (params.probe ? params.probe : TS_MAX_PROBE_SIZE);

				while (priv->ts_start_v_pts == -1 && !GxStream_Eof(demuxer->stream)) {
					if (ts_interruptcbk && ts_interruptcbk()) {
						demux_ts_close(demuxer);
						return NULL;
					}
					demux_ts_fill_buffer(demuxer,NULL);
					if (sh_audio != NULL)
						GxDemuxStream_FreePacks(demuxer->audio);
					if (sh_video != NULL)
						GxDemuxStream_FreePacks(demuxer->video);
					if (sh_sub != NULL)
						GxDemuxStream_FreePacks(demuxer->sub);

					/*
					 * the a_pid/v_pid in the ts stream of the dvb program may have some
					 * problems sometimes
					 * so, we should make condition judgment if we want the priv->ts_start_pts
					 * if not, it may be into the loop
					 */
					if (demuxer->stream->pos > end_pos) {
						/*
						 * the video pid or audio pid may be wrong
						 * the programs info parsed by gx_demux_ts is not accurate
						 */
						if (pmt && pmt->es_cnt > 1) {
							for (; i < pmt->es_cnt; i++) {
								if (IS_VIDEO(pmt->es[i].type) && pmt->es[i].pid != priv->ts_v_pid) {
									int vpid_new = pmt->es[i].pid;

									if (priv->ts.streams[vpid_new].sh) {
										priv->ts_v_pid = vpid_new;
										demuxer->info.video_pid = vpid_new;
										demuxer->video->id = priv->ts.streams[vpid_new].id;
										demuxer->video->sh = priv->ts.streams[vpid_new].sh;
										GxStream_Reset(demuxer->stream);
										GxStream_Seek(demuxer->stream,init_pos);
										goto re_find_video;
									}
								}
							}
						}
						break;
					}
				}
			}
		}

		/* pid and pts for get duration */
		if (priv->ts_a_pid > 0 && priv->ts_start_a_pts != -1) {
			priv->ts_dur_pid = priv->ts_a_pid;
			priv->ts_start_pts = priv->ts_start_a_pts;
		} else if (priv->ts_v_pid > 0 && priv->ts_start_v_pts != -1) {
			priv->ts_dur_pid = priv->ts_v_pid;
			priv->ts_start_pts = priv->ts_start_v_pts;
		}
		if (!priv->time_insert) {
			if (priv->ts_start_pts < 0 || priv->ts_dur_pid < 0) {
				demux_ts_close(demuxer);
				return NULL;
			}
		}

		/* step for get duration */
		if (priv->ts_v_pid <= 0 || priv->ts_v_pid >= 0x1fff)
			priv->ts_dur_probe_size = 8192*4;
		else
			priv->ts_dur_probe_size = 81920;

		/* for skip audio packet or seek */
		if (priv->ts_a_pid > 0 && priv->ts_start_a_pts != -1)
			priv->lastseekpts = priv->ts_start_a_pts;
		else
			priv->lastseekpts = priv->ts_start_v_pts;
		priv->lastseekpos = demuxer->movi_start;
		priv->audio_lastseekbytes = 0;
		priv->video_lastseekbytes = 0;

		GxStream_Reset(demuxer->stream);
		GxStream_Seek(demuxer->stream,start_pos);
		demux_avfifo_reset(demuxer);
	}
	else {
		demux_ts_close(demuxer);
		return NULL;
	}

	if (priv->time_insert)
		demuxer->start_pts = 0;
	else
		demuxer->start_pts = priv->lastseekpts;

	demuxer->durstream = GxStream_Open(demuxer->stream->original_url,GX_STREAM_READ, 0);
	if (!demuxer->durstream) {
		demux_ts_close(demuxer);
		return NULL;
	}

	if (priv->time_insert) {
		struct vol_info info;

		memset(&info, 0, sizeof(struct vol_info));
		GxStream_Control(demuxer->durstream, GX_STREAM_CTRL_GET_VOL_INFO, &info);

		GxStream_Reset(demuxer->durstream);
		GxStream_Seek(demuxer->durstream, 0);

		if ((info.truesize > 0) && (info.size > info.truesize))
			priv->ts_start_pts = 0;
		else
			priv->ts_start_pts = ts_get_priv_timestamp(demuxer->durstream, priv->ts.packet_size);
	}

	return demuxer;
}

static void to_copy_data(uint8_t **dst_adrr, int* dst_len, uint8_t* src, int src_len)
{
	uint8_t* to_copy_addr = *dst_adrr;
	int to_copy_len = *dst_len;

	if (to_copy_addr == NULL)
		to_copy_addr = av_malloc(src_len);
	else
		to_copy_addr = av_realloc(to_copy_addr, (to_copy_len + src_len));

	if (to_copy_addr == NULL) {
		*dst_len = 0;
		*dst_adrr = NULL;
		return;
	}
	memcpy(to_copy_addr+to_copy_len, src, src_len);
	to_copy_len +=  src_len;

	*dst_adrr = (uint8_t*)to_copy_addr;
	*dst_len = to_copy_len;
	return;
}

static void ts_send_data(DemuxTSPriv* priv, int index, uint8_t* data_p, int len)
{
	int data_len = 0;
	uint8_t* pdata = NULL;
	int i = index, payload_start = 0;

	if (priv->filter.data[i].bck_data) {
		to_copy_data(&priv->filter.data[i].p_data, &priv->filter.data[i].p_len,
				priv->filter.data[i].bck_data, priv->filter.data[i].bck_len);

		av_free(priv->filter.data[i].bck_data);
		priv->filter.data[i].bck_data = NULL;
		priv->filter.data[i].bck_len = 0;
	}

	if ((data_p[1] & 0x80) != 0)
		return;

	if ((data_p[1] & 0x40) != 0) {
		payload_start = 1;
	} else
		payload_start = 0;

	if ((data_p[3] & 0x30) == 0x30) {
		data_len = TS_PACKET_SIZE - 4 - 1 - data_p[4];
		pdata = data_p + TS_PACKET_SIZE - data_len;
	} else if ((data_p[3] & 0x30) == 0x10) {
		data_len = 184;
		pdata = data_p + TS_PACKET_SIZE - data_len;
	} else
		return;

	if ((data_len > 0) && (data_len <= 184)) {
		if (payload_start) {
			priv->filter.data[i].finish = 1;
			priv->filter.data[i].p_copy = 1;
			to_copy_data(&priv->filter.data[i].bck_data, &priv->filter.data[i].bck_len, pdata, data_len);
		}else if (priv->filter.data[i].p_copy) {
			if ((priv->filter.data[i].p_len + data_len) > 64*1024) {
				priv->filter.data[i].finish = 1;
				priv->filter.data[i].p_copy = 0;
			}else
				to_copy_data(&priv->filter.data[i].p_data, &priv->filter.data[i].p_len, pdata, data_len);
		}
	}

	if (priv->filter.data[i].finish && priv->filter.data[i].p_data) {
		if (priv->filter.data[i].writeback) {
			int      len = 0;
			uint8_t* buf = NULL;

			if (priv->filter.data[i].type == DEMUX_PSI_TYPE) {
				buf = priv->filter.data[i].p_data+1;
				len = (((buf[1]<<8) | buf[2]) & 0xfff) + 3;
				if (len <= priv->filter.data[i].p_len) {
#if defined(CONFIG_FFMPEG_ENABLE)
					if (!priv->filter.data[i].crc || av_crc(av_crc_get_table(AV_CRC_32_IEEE), -1, buf, len) == 0)
#else
					if (!priv->filter.data[i].crc || gx_crc(gx_crc_get_table(AV_CRC_32_IEEE), -1, buf, len) == 0)
#endif
					{
						priv->filter.data[i].writeback(priv->filter.data[i].pid, DEMUX_PSI_TYPE, buf, len);
					}
				}
			} else if (priv->filter.data[i].type == DEMUX_PES_TYPE) {
				buf = priv->filter.data[i].p_data;
				len = priv->filter.data[i].p_len;
				priv->filter.data[i].writeback(priv->filter.data[i].pid, DEMUX_PES_TYPE, buf, len);
			}
		}
		av_free(priv->filter.data[i].p_data);
		priv->filter.data[i].p_data = NULL;
		priv->filter.data[i].p_len = 0;
		priv->filter.data[i].finish = 0;
	}

	return;
}

static void ts_send_ts(DemuxTSPriv* priv, int index, uint8_t* data_p, int len)
{
	int i = index;

	if (priv->filter.data[i].writeback)
		priv->filter.data[i].writeback(priv->filter.data[i].pid, priv->filter.data[i].type, data_p, len);

	return;
}

static int ts_send_filter(uint8_t* buffer, int len, DemuxTSPriv* priv)
{
	int index = 0, pid = 0, num = len/TS_PACKET_SIZE;
	uint8_t* data_p = buffer;

	while (num) {
		if (data_p[0] != 0x47) {
			goto NEXT_TS;
		}

		pid = ((((uint16_t)(data_p[1] & 0x1f)) << 8) | ((uint16_t)data_p[2]));
		for (index = 0; index < PLAYER_FILTER_MAX; index++) {
			if (pid == priv->filter.data[index].pid && priv->filter.data[index].using)
				break;
		}
		if (index >= PLAYER_FILTER_MAX) {
			goto NEXT_TS;
		}

		if (priv->filter.data[index].type == DEMUX_PES_TYPE || priv->filter.data[index].type == DEMUX_PSI_TYPE)
			ts_send_data(priv, index, data_p, TS_PACKET_SIZE);
		else if(priv->filter.data[index].type == DEMUX_TS_TYPE)
			ts_send_ts(priv, index, data_p, TS_PACKET_SIZE);

NEXT_TS:
		data_p += TS_PACKET_SIZE;
		num--;
	}
	return GX_PLAYER_OK;
}

static void demuxer_ts_filter(void* data)
{
	int i;
	DemuxTSPriv* priv = (DemuxTSPriv*)data;
	uint8_t *buffer = av_malloc(64*TS_PACKET_SIZE);

	while (priv && !priv->abort && priv->filter.running && priv->filter.fifo) {
		int num;

		if (ts_interruptcbk && ts_interruptcbk())
			break;

		num = GxFifo_GetLength(priv->filter.fifo)/TS_PACKET_SIZE;
		if (num <= 0) {
			GxCore_ThreadDelay(100);
			continue;
		}

		if (num > 64)
			num = 64;

		GxFifo_Read(priv->filter.fifo, buffer, num*TS_PACKET_SIZE, -1);
		ts_send_filter(buffer, num*TS_PACKET_SIZE, priv);
	}

	for (i = 0; i < PLAYER_FILTER_MAX; i++) {
		if (priv->filter.data[i].p_data) {
			av_free(priv->filter.data[i].p_data);
			priv->filter.data[i].p_data = NULL;
		}

		if (priv->filter.data[i].bck_data) {
			av_free(priv->filter.data[i].bck_data);
			priv->filter.data[i].bck_data = NULL;
		}
	}
	av_free(buffer);
	return;
}

static int demux_ts_sub_run(GxDemuxer* demuxer)
{
	DemuxTSPriv* priv = demuxer->priv;

	if (priv && !priv->filter.running) {
		priv->filter.fifo = GxFifo_Create(128*1024, GX_PINFLAG_SW);
		priv->filter.running = 1;
		GxCore_ThreadCreate("demux_ts_filter", &priv->filter.pthread_id,
				demuxer_ts_filter, (void*)priv, 32*1024, GXOS_DEFAULT_PRIORITY-1);
	}
	return GX_PLAYER_OK;
}

static int demux_ts_sub_stop(GxDemuxer* demuxer)
{
	int i = 0;
	DemuxTSPriv* priv = demuxer->priv;

	if (priv && priv->filter.running) {
		for (i = 0; i < PLAYER_FILTER_MAX; i++)
			priv->filter.data[i].using = 0;

		priv->filter.running   = 0;
		GxCore_ThreadJoin(priv->filter.pthread_id);
		priv->filter.pthread_id = 0;
		GxFifo_Destroy(priv->filter.fifo);
		priv->filter.fifo = NULL;
	}

	if (demuxer->durstream) {
		GxStream_Close(demuxer->durstream);
		demuxer->durstream = NULL;
	}

	return GX_PLAYER_OK;
}

GxDemuxerClass gx_demux_ts = {
	._inherit = {
		._inherit = {
			.name    = "Demuxer TS",
			.parent  = &gx_DemuxerBase,
			.size    = sizeof(GxDemuxer),
			.create  = NULL,
			.release = NULL,
		},
		.run    = NULL,
		.pause  = NULL,
		.resume = NULL,
		.config = NULL,
		.stop   = NULL,
	},
	.name        = "Demuxer TS",
	.type        = GX_DEMUXER_TYPE_MPEG_TS,
	.check_file  = demux_ts_check_file,
	.open        = demux_ts_open,
	.close       = demux_ts_close,
	.fill_buffer = demux_ts_fill_buffer,
	.seek        = demux_ts_seek,
	.control     = demux_ts_control,
	.sub_run     = demux_ts_sub_run,
	.sub_stop    = demux_ts_sub_stop,
};


