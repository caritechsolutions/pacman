#ifndef NO_OS
#include <stdio.h>
#include <stdlib.h>
#include "gui.h"
#else /*NO_OS*/
#include "mini_gui_private.h"
#include "gal.h"
#include "hd_gal.h"
#include "gal_blit.h"

#endif /*NO_OS*/

//#include "IMG.h"
#include "gdi_char.h"
#include "gdi_font.h"
#include "gdi_font_utf8.h"

typedef enum
{
	STRING_NORMAL,
	STRING_ISO88591,
	STRING_ISO88592,
	STRING_ISO88593,
	STRING_ISO88594,
	STRING_ISO88595,
	STRING_ISO88596,
	STRING_ISO88597,
	STRING_ISO88598,
	STRING_ISO88599,
	STRING_ISO885910,
	STRING_ISO885911,
	STRING_ISO885912,
	STRING_ISO885913,
	STRING_ISO885914,
	STRING_ISO885915,
	STRING_ISO885916,
	STRING_ISO_IEC10646,
	STRING_ISO6937,
	STRING_ARABIC,
	STRING_HEBREW
}STRING_FLAG;

/*static unsigned short table_iso88592[] =
  {
  0x00A0,0x0104,0x02D8,0x0141,    0x00A4,0x013D,0x015A,0x00A7,
  0x00A8,0x0160,0x015E,0x0164,    0x0179,0x00AD,0x017D,0x017B,
  0x00B0,0x0105,0x02DB,0x0142,    0x00B4,0x013E,0x015B,0x02C7,
  0x00B8,0x0161,0x015F,0x0165,    0x017A,0x02DD,0x017E,0x017C,
  0x0154,0x00C1,0x00C2,0x0102,    0x00C4,0x0139,0x0106,0x00C7,
  0x010C,0x00C9,0x0118,0x00CB,    0x011A,0x00CD,0x00CE,0x010E,
  0x0110,0x0143,0x0147,0x00D3,    0x00D4,0x0150,0x00D6,0x00D7,
  0x0158,0x016E,0x00DA,0x0170,    0x00DC,0x00DD,0x0162,0x00DF,
  0x0155,0x00E1,0x00E2,0x0103,    0x00E4,0x013A,0x0107,0x00E7,
  0x010D,0x00E9,0x0119,0x00EB,    0x011B,0x00ED,0x00EE,0x010F,
  0x0111,0x0144,0x0148,0x00F3,    0x00F4,0x0151,0x00F6,0x00F7,
  0x0159,0x016F,0x00FA,0x0171,    0x00FC,0x00FD,0x0163,0x02D9
  };*/

static unsigned short table_iso88593[] =
{
	0x00A0,0x0126,0x02D8,0x00A3,    0x00A4,0x0000,0x0124,0x00A7,
	0x00A8,0x0130,0x015E,0x011E,    0x0134,0x00AD,0x0000,0x017B,
	0x00B0,0x0127,0x00B2,0x00B3,    0x00B4,0x00B5,0x0125,0x00B7,
	0x00B8,0x0131,0x015F,0x011F,    0x0135,0x00BD,0x0000,0x017C,
	0x00C0,0x00C1,0x00C2,0x0000,    0x00C4,0x010A,0x0108,0x00C7,
	0x00C8,0x00C9,0x00CA,0x00CB,    0x00CC,0x00CD,0x00CE,0x00CF,
	0x0000,0x00D1,0x00D2,0x00D3,    0x00D4,0x0120,0x00D6,0x00D7,
	0x011C,0x00D9,0x00DA,0x00DB,    0x00DC,0x016C,0x015C,0x00DF,
	0x00E0,0x00E1,0x00E2,0x0000,    0x00E4,0x010B,0x0109,0x00E7,
	0x00E8,0x00E9,0x00EA,0x00EB,    0x00EC,0x00ED,0x00EE,0x00EF,
	0x0000,0x00F1,0x00F2,0x00F3,    0x00F4,0x0121,0x00F6,0x00F7,
	0x011D,0x00F9,0x00FA,0x00FB,    0x00FC,0x016D,0x015D,0x02D9
};

static unsigned short table_iso88594[] =
{
	0x00A0,0x0104,0x0138,0x0156,    0x00A4,0x0128,0x013B,0x00A7,
	0x00A8,0x0160,0x0112,0x0122,    0x0166,0x00AD,0x017D,0x00AF,
	0x00B0,0x0105,0x02DB,0x0157,    0x00B4,0x0129,0x013C,0x02C7,
	0x00B8,0x0161,0x0113,0x0123,    0x0167,0x014A,0x017E,0x014B,
	0x0100,0x00C1,0x00C2,0x00C3,    0x00C4,0x00C5,0x00C6,0x012E,
	0x010C,0x00C9,0x0118,0x00CB,    0x0116,0x00CD,0x00CE,0x012A,
	0x0110,0x0145,0x014C,0x0136,    0x00D4,0x00D5,0x00D6,0x00D7,
	0x00D8,0x0172,0x00DA,0x00DB,    0x00DC,0x0168,0x016A,0x00DF,
	0x0101,0x00E1,0x00E2,0x00E3,    0x00E4,0x00E5,0x00E6,0x012F,
	0x010D,0x00E9,0x0119,0x00EB,    0x0117,0x00ED,0x00EE,0x012B,
	0x0111,0x0146,0x014D,0x0137,    0x00F4,0x00F5,0x00F6,0x00F7,
	0x00F8,0x0173,0x00FA,0x00FB,    0x00FC,0x0169,0x016B,0x02D9
};

static unsigned short table_iso88597[] =
{
	0x00A0,0x2018,0x2019,0x00A3,    0x20AC,0x0000,0x00A6,0x00A7,
	0x00A8,0x00A9,0x037A,0x00AB,    0x00AC,0x00AD,0x00AE,0x2015,
	0x00B0,0x00B1,0x00B2,0x00B3,    0x0384,0x0385,0x0386,0x0387,
	0x0388,0x0389,0x038A,0x00BB,    0x038C,0x00BD,0x038E,0x038F,
	0x0390,0x0391,0x0392,0x0393,    0x0394,0x0395,0x0396,0x0397,
	0x0398,0x0399,0x039A,0x039B,    0x039C,0x039D,0x039E,0x039F,
	0x03A0,0x03A1,0x0000,0x03A3,    0x03A4,0x03A5,0x03A6,0x03A7,
	0x03A8,0x03A9,0x03AA,0x03AB,    0x03AC,0x03AD,0x03AE,0x03AF,
	0x03B0,0x03B1,0x03B2,0x03B3,    0x03B4,0x03B5,0x03B6,0x03B7,
	0x03B8,0x03B9,0x03BA,0x03BB,    0x03BC,0x03BD,0x03BE,0x03BF,
	0x03C0,0x03C1,0x03C2,0x03C3,    0x03C4,0x03C5,0x03C6,0x03C7,
	0x03C8,0x03C9,0x03CA,0x03CB,    0x03CC,0x03CD,0x03CE,0x0000,
};

static unsigned short table_iso885910[] =
{
	0x00A0,0x0104,0x0112,0x0122,    0x012A,0x0128,0x0136,0x00A7,
	0x013B,0x0110,0x0160,0x0166,    0x017D,0x00AD,0x016A,0x014A,
	0x00B0,0x0105,0x0113,0x0123,    0x012B,0x0129,0x0137,0x00B7,
	0x013C,0x0111,0x0161,0x0167,    0x017E,0x2015,0x016B,0x014B,
	0x0100,0x00C1,0x00C2,0x00C3,    0x00C4,0x00C5,0x00C6,0x012E,
	0x010C,0x00C9,0x0118,0x00CB,    0x0116,0x00CD,0x00CE,0x00CF,
	0x00D0,0x0145,0x014C,0x00D3,    0x00D4,0x00D5,0x00D6,0x0168,
	0x00D8,0x0172,0x00DA,0x00DB,    0x00DC,0x00DD,0x00DE,0x00DF,
	0x0101,0x00E1,0x00E2,0x00E3,    0x00E4,0x00E5,0x00E6,0x012F,
	0x010D,0x00E9,0x0119,0x00EB,    0x0117,0x00ED,0x00EE,0x00EF,
	0x00F0,0x0146,0x014D,0x00F3,    0x00F4,0x00F5,0x00F6,0x0169,
	0x00F8,0x0173,0x00FA,0x00FB,    0x00FC,0x00FD,0x00FE,0x0138
};

static unsigned short table_iso885911[] =
{
	0x00A0,0x0E01,0x0E02,0x0E03,    0x0E04,0x0E05,0x0E06,0x0E07,
	0x0E08,0x0E09,0x0E0A,0x0E0B,    0x0E0C,0x0E0D,0x0E0E,0x0E0F,
	0x0E10,0x0E11,0x0E12,0x0E13,    0x0E14,0x0E15,0x0E16,0x0E17,
	0x0E18,0x0E19,0x0E1A,0x0E1B,    0x0E1C,0x0E1D,0x0E1E,0x0E1F,
	0x0E20,0x0E21,0x0E22,0x0E23,    0x0E24,0x0E25,0x0E26,0x0E27,
	0x0E28,0x0E29,0x0E2A,0x0E2B,    0x0E2C,0x0E2D,0x0E2E,0x0E2F,
	0x0E30,0x0E31,0x0E32,0x0E33,    0x0E34,0x0E35,0x0E36,0x0E37,
	0x0E38,0x0E39,0x0E3A,0x0E3B,    0x0E3C,0x0E3D,0x0E3E,0x0E3F,
	0x0E40,0x0E41,0x0E42,0x0E43,    0x0E44,0x0E45,0x0E46,0x0E47,
	0x0E48,0x0E49,0x0E4A,0x0E4B,    0x0E4C,0x0E4D,0x0E4E,0x0E4F,
	0x0E50,0x0E51,0x0E52,0x0E53,    0x0E54,0x0E55,0x0E56,0x0E57,
	0x0E58,0x0E59,0x0E5A,0x0E5B,    0x0E5C,0x0E5D,0x0E5E,0x0E5F
};

static unsigned short table_iso885913[] =
{
	0x00A0,0x201D,0x00A2,0x00A3,    0x00A4,0x201E,0x00A6,0x00A7,
	0x00D8,0x00A9,0x0156,0x00AB,    0x00AC,0x00AD,0x00AE,0x00C6,
	0x00B0,0x00B1,0x00B2,0x00B3,    0x201C,0x00B5,0x00B6,0x00B7,
	0x00F8,0x00B9,0x0157,0x00BB,    0x00BC,0x00BD,0x00BE,0x00E6,
	0x0104,0x012E,0x0100,0x0106,    0x00C4,0x00C5,0x0118,0x0112,
	0x010C,0x00C9,0x0179,0x0116,    0x0122,0x0136,0x012A,0x013B,
	0x0160,0x0143,0x0145,0x00D3,    0x014C,0x00D5,0x00D6,0x00D7,
	0x0172,0x0141,0x015A,0x016A,    0x00DC,0x017B,0x017D,0x00DF,
	0x0105,0x012F,0x0101,0x0107,    0x00E4,0x00E5,0x0119,0x0113,
	0x010D,0x00E9,0x017A,0x0117,    0x0123,0x0137,0x012B,0x013C,
	0x0161,0x0144,0x0146,0x00F3,    0x014D,0x00F5,0x00F6,0x00F7,
	0x0173,0x0142,0x015B,0x016B,    0x00FC,0x017C,0x017E,0x2019
};

static unsigned short table_iso885916[] =
{
	0x00A0,0x0104,0x0105,0x0141,    0x20AC,0x00AB,0x0160,0x00A7,
	0x0161,0x00A9,0x0218,0x201E,    0x0179,0x00AD,0x017A,0x017B,
	0x00B0,0x00B1,0x010C,0x010D,    0x017D,0x201D,0x00B6,0x00B7,
	0x017E,0x010D,0x0219,0x00BB,    0x0152,0x0153,0x0178,0x017C,
	0x00C0,0x00C1,0x00C2,0x0102,    0x00C4,0x0106,0x00C6,0x00C7,
	0x00C8,0x00C9,0x00CA,0x00CB,    0x00CC,0x00CD,0x00CE,0x00CF,
	0x0110,0x0143,0x00D2,0x00D3,    0x00D4,0x0150,0x00D6,0x015A,
	0x0170,0x00D9,0x00DA,0x00DB,    0x00DC,0x0118,0x021A,0x00DF,
	0x00E0,0x00E1,0x00E2,0x0103,    0x00E4,0x0107,0x00E6,0x00E7,
	0x00E8,0x00E9,0x00EA,0x00EB,    0x00EC,0x00ED,0x00EE,0x00EF,
	0x0111,0x0144,0x00F2,0x00F3,    0x00F4,0x0151,0x00F6,0x015B,
	0x0171,0x00F9,0x00FA,0x00FB,    0x00FC,0x0119,0x021B,0x00FF
};

static const ArabicMap TurkiShapping[] =
{
	{0xA0, 0x00A0},/*0*/
	{0xA4, 0x00A4},/*1*/
	{0xA1, 0x060C},/*2*/
	{0xAD, 0x00AD},/*3*/
	{0xBA, 0x061B},/*4*/
	{0xBF, 0x061F},/*5*/
	{0xC1, 0x0621},/*6*/
	{0xC2, 0x0622},/*7*/
	{0xC3, 0x0623},/*8*/
	{0xC4, 0x0624},/*9*/
	{0xC5, 0x0625},/*10*/
	{0xC6, 0x0626},/*11*/
	{0xC7, 0x0627},/*12*/
	{0xC8, 0x0628},/*13*/
	{0xC9, 0x0629},/*14*/
	{0xCA, 0x062A},/*15*/
	{0xCB, 0x062B},/*16*/
	{0xCC, 0x062C},/*17*/
	{0xCD, 0x062D},/*18*/
	{0xCE, 0x062E},/*19*/
	{0xCF, 0x062F},/*20*/
	{0xD0, 0x0630},/*21*/
	{0xD1, 0x0631},/*22*/
	{0xD2, 0x0632},/*23*/
	{0xD3, 0x0633},/*24*/
	{0xD4, 0x0634},/*25*/
	{0xD5, 0x0635},/*26*/
	{0xD6, 0x0636},/*27*/
	{0xD8, 0x0637},/*28*/
	{0xD9, 0x0638},/*29*/
	{0xDA, 0x0639},/*30*/
	{0xDB, 0x063A},/*31*/
	{0xDC, 0x0640},/*32*/
	{0xDD, 0x0641},/*33*/
	{0xDE, 0x0642},/*34*/
	{0xDF, 0x0643},/*35*/
	{0xE1, 0x0644},/*36*/
	{0xE3, 0x0645},/*37*/
	{0xE4, 0x0646},/*38*/
	{0xE5, 0x0647},/*39*/
	{0xE6, 0x0648},/*40*/
	{0xEC, 0x0649},/*41*/
	{0xED, 0x064A},/*42*/
	{0xF0, 0x064B},/*43*/
	{0xF1, 0x064C},/*44*/
	{0xF2, 0x064D},/*45*/
	{0xF3, 0x064E},/*46*/
	{0xF5, 0x064F},/*47*/
	{0xF6, 0x0650},/*48*/
	{0xF8, 0x0651},/*49*/
	{0xFA, 0x0652} /*50*/
};

static const KEY_INFO _aKeyInfo[] =
{
	/*    Base      Isol.   Final   Initial Medial */
	{ /* 0  */0x0621, 0xFE80, 0x0000, 0x0000, 0x0000 },
	{ /* 1  */0x0622, 0xFE81, 0xFE82, 0x0000, 0x0000 },
	{ /* 2  */0x0623, 0xFE83, 0xFE84, 0x0000, 0x0000 },
	{ /* 3  */0x0624, 0xFE85, 0xFE86, 0x0000, 0x0000 },
	{ /* 4  */0x0625, 0xFE87, 0xFE88, 0x0000, 0x0000 },
	{ /* 5  */0x0626, 0xFE8B, 0xFE8C, 0xFE8B, 0xFE8C },
	{ /* 6  */0x0627, 0xFE8D, 0xFE8E, 0x0000, 0x0000 },
	{ /* 7  */0x0628, 0xFE8F, 0xFE90, 0xFE91, 0xFE92 },
	{ /* 8  */0x0629, 0xFE93, 0xFE94, 0x0000, 0x0000 },
	{ /* 9  */0x062A, 0xFE95, 0xFE96, 0xFE97, 0xFE98 },
	{ /* 10 */0x062B, 0xFE99, 0xFE9A, 0xFE9B, 0xFE9C },
	{ /* 11 */0x062C, 0xFE9D, 0xFE9E, 0xFE9F, 0xFEA0 },
	{ /* 12 */0x062D, 0xFEA1, 0xFEA2, 0xFEA3, 0xFEA4 },
	{ /* 13 */0x062E, 0xFEA5, 0xFEA6, 0xFEA7, 0xFEA8 },
	{ /* 14 */0x062F, 0xFEA9, 0xFEAA, 0x0000, 0x0000 },
	{ /* 15 */0x0630, 0xFEAB, 0xFEAC, 0x0000, 0x0000 },
	{ /* 16 */0x0631, 0xFEAD, 0xFEAE, 0x0000, 0x0000 },
	{ /* 17 */0x0632, 0xFEAF, 0xFEB0, 0x0000, 0x0000 },
	{ /* 18 */0x0633, 0xFEB1, 0xFEB2, 0xFEB3, 0xFEB4 },
	{ /* 19 */0x0634, 0xFEB5, 0xFEB6, 0xFEB7, 0xFEB8 },
	{ /* 20 */0x0635, 0xFEB9, 0xFEBA, 0xFEBB, 0xFEBC },
	{ /* 21 */0x0636, 0xFEBD, 0xFEBE, 0xFEBF, 0xFEC0 },
	{ /* 22 */0x0637, 0xFEC1, 0xFEC2, 0xFEC3, 0xFEC4 },
	{ /* 23 */0x0638, 0xFEC5, 0xFEC6, 0xFEC7, 0xFEC8 },
	{ /* 24 */0x0639, 0xFEC9, 0xFECA, 0xFECB, 0xFECC },
	{ /* 25 */0x063A, 0xFECD, 0xFECE, 0xFECF, 0xFED0 },
	{ /* 26 */0x0641, 0xFED1, 0xFED2, 0xFED3, 0xFED4 },
	{ /* 27 */0x0642, 0xFED5, 0xFED6, 0xFED7, 0xFED8 },
	{ /* 28 */0x0643, 0xFED9, 0xFEDA, 0xFEDB, 0xFEDC },
	{ /* 29 */0x0644, 0xFEDD, 0xFEDE, 0xFEDF, 0xFEE0 },
	{ /* 30 */0x0645, 0xFEE1, 0xFEE2, 0xFEE3, 0xFEE4 },
	{ /* 31 */0x0646, 0xFEE5, 0xFEE6, 0xFEE7, 0xFEE8 },
	{ /* 32 */0x0647, 0xFEE9, 0xFEEA, 0xFEEB, 0xFEEC },
	{ /* 33 */0x0648, 0xFEED, 0xFEEE, 0x0000, 0x0000 },
	{ /* 34 */0x0649, 0xFEEF, 0xFEF0, 0xFBE8, 0xFBE9 },
	{ /* 35 */0x064A, 0xFEF1, 0xFEF2, 0xFEF3, 0xFEF4 },
	{ /* 36 Uigur*/0x0675, 0x0000, 0xFE8E, 0x0000, 0x0000},
	{ /* 37 Uigur*/0x0676, 0xFE85, 0xFEEE, 0x0000, 0x0000},
	{ /* 38 Uigur*/0x0677, 0xFBDD, 0xFBD8, 0x0000, 0x0000},
	{ /* 39 */0x067E, 0xFB56, 0xFB57, 0xFB58, 0xFB59 },
	{ /* 40 */0x0686, 0xFB7A, 0xFB7F, 0xFB7C, 0xFB7D },
	{ /* 41 */0x0698, 0xFB8A, 0xFB8B, 0x0000, 0x0000 },
	{ /* 42 */0x06A9, 0xFB8E, 0xFB8F, 0xFB90, 0xFB91 },
	{ /* 43 Uigur*/0x06AD, 0xFBD3, 0xFBD4, 0xFBD5, 0xFBD6},
	{ /* 44 */0x06AF, 0xFB92, 0xFB93, 0xFB94, 0xFB95 },
	{ /* 45 Uigur*/0x06BE, 0xFBAA, 0xFBAD, 0xFBAC, 0xFBAB},
	{ /* 46 Uigur*/0x06C5, 0xFBE0, 0xFBE1, 0xFBAC, 0xFBAD},
	{ /* 47 Uigur*/0x06C6, 0xFBD9, 0xFBDA, 0x0000, 0x0000},
	{ /* 48 Uigur*/0x06C7, 0xFBD7, 0xFBD8, 0x0000, 0x0000},
	{ /* 49 Uigur*/0x06C8, 0xFBDB, 0xFBDC, 0x0000, 0x0000},
	{ /* 50 Uigur*/0x06C9, 0xFBE2, 0xFBE2, 0x0000, 0x0000},
	{ /* 51 Uigur*/0x06CB, 0xFBDE, 0xFBDF, 0x0000, 0x0000},
	{ /* 52 */0x06CC, 0xFBFC, 0xFBFD, 0x0000, 0x0000 },
	{ /* 53 Uigur*/0x06D0, 0xFBF4, 0xFBE5, 0xFBE6, 0xFBE7},
	{ /* 54 Uigur*/0x06D5, 0xFEE9, 0xFEEA, 0x0000, 0x0000}
};

static int _check_string_flag(unsigned char **string)
{
	unsigned char *pstr = NULL, iso6936_flag = 0;
	int code = 0;
	unsigned int unicode = 0, accent = 0;

	pstr = *string;
	if((0x0 > (*pstr)) && ((*pstr) <= 0x05))
	{
		code = (*pstr + 4);
		pstr += 1;
	}
	else if(0x10 == (*pstr))
	{
		pstr++;
		code = (((*pstr) << 8) | (*(pstr + 1)));
		pstr += 2;
	}
	else if(0x11 == (*pstr))
	{
		code = *(pstr);
		pstr += 1;
	}
	else
	{
		code = 0;
		while(*pstr)
		{
			gdi_font_utf82unicode(&pstr, &unicode, &accent, &iso6936_flag);
			if(iso6936_flag)
			{
				code = STRING_ISO6937;
				break;
			}
			else if(((0x0E33 < unicode) && (unicode < 0x0E3F)) ||
					(0x0E31 == unicode) ||
					((0x0E46 < unicode) && (unicode < 0x0E4F)))
			{
				code = STRING_ISO6937;
				break;
			}
			else if((unicode >= 0x0600) && (unicode <= 0x06FF))
			{
				code = STRING_ARABIC;
				break;
			}
			else if((unicode >= 0x0590) && (unicode <= 0x05FF))
			{
				code = STRING_HEBREW;
				break;
			}
		}
	}

	*string = pstr;

	return (code);
}

static int _normal_string_len(unsigned char *pstr, int flag)
{
	unsigned int unicode = 0, accent = 0;
	unsigned char iso6937_flag = 0;
	int ret = 0;

	while(*pstr)
	{
		gdi_font_utf82unicode(&pstr, &unicode, &accent, &iso6937_flag);
		ret += gdi_get_charwidth(unicode);
	}

	return (ret);
}

static int _iso6937_string_len(unsigned char *pstr, int flag)
{
	unsigned int unicode = 0, accent = 0;
	unsigned char iso6937_flag = 0;
	int ret = 0, width = 0;

	while(*pstr)
	{
		gdi_font_utf82unicode(&pstr, &unicode, &accent, &iso6937_flag);
		if(iso6937_flag)
		{
			width = (gdi_get_charwidth(unicode) > gdi_get_charwidth(accent)) ?
				gdi_get_charwidth(unicode) : gdi_get_charwidth(accent);
		}
		else
		{
			width = gdi_get_charwidth(unicode);
		}

		ret += width;
	}

	return (ret);
}

static unsigned short _iso8859_get_unicode(unsigned char c, int flag)
{
	unsigned short unicode = 0;
	unsigned short *iso_table = NULL;
	unsigned short base_address = 0;

	switch(flag)
	{
		case STRING_ISO88591:
			break;
		case STRING_ISO88593:
			iso_table = table_iso88593;
			break;
		case STRING_ISO88594:
			iso_table = table_iso88594;
			break;
		case STRING_ISO88595:
			base_address = 0x0400 - 0xA0;
			break;
		case STRING_ISO88596:
			break;
		case STRING_ISO88597:
			iso_table = table_iso88597;
			break;
		case STRING_ISO88598:
			break;
		case STRING_ISO88599:
			break;
		case STRING_ISO885910:
			iso_table = table_iso885910;
			break;
		case STRING_ISO885911:
			iso_table = table_iso885911;
			break;
		case STRING_ISO885912:
			break;
		case STRING_ISO885913:
			iso_table = table_iso885913;
			break;
		case STRING_ISO885914:
		case STRING_ISO885915:
			break;
		case STRING_ISO885916:
			iso_table = table_iso885916;
			break;
		default:
			break;
	}

	if(iso_table)
	{
		unicode = iso_table[c];
	}
	else if(STRING_ISO88595 == flag)
	{
		if(unicode > 0xA0)
		{
			unicode = base_address + c;
		}
		else
		{
			unicode = c;
		}
	}
	else if(STRING_ISO88599 == flag)
	{
		if(0xD0 == c)
		{
			unicode = 0x011E;
		}
		else if(0xDD == c)
		{
			unicode = 0x0130;
		}
		else if(0xDE == c)
		{
			unicode = 0x015E;
		}
		else if(0xF0 == c)
		{
			unicode = 0x011F;
		}
		else if(0xFD == c)
		{
			unicode = 0x0131;
		}
		else if(0xFE == c)
		{
			unicode = 0x015F;
		}
		else
		{
			unicode = c;
		}
	}

	return (unicode);

}

static int _iso8859_string_len(unsigned char *pstr, int flag)
{
	unsigned short unicode = 0;
	int ret = 0;

	while(*pstr)
	{
		if(STRING_ISO_IEC10646 == flag)
		{
			unicode = ((*pstr) << 8) | (*pstr);
			pstr++;
		}
		else
		{
			unicode = _iso8859_get_unicode(*pstr, flag);
		}

		if(((0x0E33 < unicode) && (unicode < 0x0E3F)) ||
				(0x0E31 == unicode) ||
				((0x0E46 < unicode) && (unicode < 0x0E4F)))
		{
			;
		}
		else
		{
			ret += gdi_get_charwidth(unicode);
		}

		pstr++;
	}

	return (ret);
}

static unsigned char* gdi_font_utf8_back(unsigned char* header, unsigned char *string)
{
	unsigned char *pstr = NULL;

	pstr = string;

	if(header == pstr)
	{
		return (NULL);
	}

	while(header < pstr)
	{
		pstr--;
		if((UTF8_110XXXXX == (*pstr & UTF8_110XXXXX)) ||
				(*pstr < 0x80))
		{
			break;
		}
	}

	return (pstr);
}

static unsigned char* _utf8_string_move(unsigned char *header, unsigned char *string, int offset)
{
	unsigned char *pstr = NULL;
	unsigned int unicode = 0, accent = 0;
	unsigned char iso6937_flag = 0;
	int i = 0, ret = 0;

	pstr = string;
	if(offset > 0)
	{
		i = 0;
		while(i < offset)
		{
			ret = gdi_font_utf82unicode(&pstr, &unicode, &accent, &iso6937_flag);
			if(0 != ret)
			{
				break;
			}
			i++;
		}
	}
	else
	{
		i = 0;
		while(i > offset)
		{
			pstr = gdi_font_utf8_back(header, pstr);
			if(NULL == pstr)
			{
				break;
			}
			i--;
		}
	}

	return (pstr);
}

static int _is_arabic(unsigned short c)
{
	return ((c >= 0x0600) && (c <= 0x06ff)) ? 1 : 0;
}

static int _get_table_index(unsigned short Char)
{
	if ((Char >= 0x0621) && (Char <= 0x063a))
	{
		return Char - 0x0621;
	}
	if ((Char >= 0x0641) && (Char <= 0x064a))
	{
		return Char - 0x0641 + 26;
	}
	if((Char >= 0x0675) && (Char <= 0x0677))
	{
		return (Char - 0x0675) + 36;
	}
	if (Char == 0x067e)
	{
		return 39;
	}
	if (Char == 0x0686)
	{
		return 40;
	}
	if (Char == 0x0698)
	{
		return 41;
	}
	if (Char == 0x06a9)
	{
		return 42;
	}
	if(Char == 0x06AD)
	{
		return 43;
	}
	if (Char == 0x06af)
	{
		return 44;
	}
	if (Char == 0x06BE)
	{
		return 45;
	}
	if((Char >= 0x06C5) && (Char <= 0x06C9))
	{
		return (Char - 0x06C5) + 46;
	}
	if(Char == 0x06CB)
	{
		return 51;
	}
	if (Char == 0x06cc)
	{
		return 52;
	}
	if(Char == 0x06D0)
	{
		return 53;
	}
	if(Char == 0x06D5)
	{
		return 54;
	}
	return 0;
}

static int is_transparent(unsigned short c)
{
	if (c >= 0x064b)
	{
		return 1;
	}
	if (c == 0x0670)
	{
		return 1;
	}
	else
		return 0;
}

static unsigned short _get_arabic_ligature(unsigned short prev2,
		unsigned short prev,
		unsigned short cur,
		unsigned short next,
		unsigned short next2)
{
	int next_affects = 0, prev_affects = 0, next_index = 0, prev_index = 0;

	prev_index = _get_table_index(prev);
	next_index = _get_table_index(next2);

	prev_affects = (_get_table_index(prev) || is_transparent(prev)) && (_aKeyInfo[prev_index].Medial);
	next_affects = (_get_table_index(next2) || is_transparent(next2)) && (_aKeyInfo[next_index].Medial);

	if((0x0626 == cur) && (0x0627 == next))
	{
		/*Isolate*/
		if((!prev_affects) && (!next_affects))
		{
			return (0xFBEA);
		}
		/*Final*/
		else if((prev_affects) && (!next_affects))
		{
			return (0xFBBE);
		}
	}
	else if((0x06D5 == cur) && (0x0626 == next))
	{
		/*Isolate*/
		if((!prev_affects) && (!next_affects))
		{
			return (0xFEE9);
		}
		/*Final*/
		else if((prev_affects) && (!next_affects))
		{
			return (0xFEED);
		}
	}
	else if((0x0626 == cur) && (0x06D0 == next))
	{
		/*Isolate*/
		if((!prev_affects) && (!next_affects))
		{
			return (0xFBF8);				//lzz:以Windows显示效果修改为此值
		}
		/*Initial*/
		else if((!prev_affects) && (next_affects))
		{
			return (0xFBF8);
		}
		/*Medium*/
		else if((prev_affects) && (next_affects))
		{
			return (0xFBE7);
		}
		/*Final*/
		else if((prev_affects) && (!next_affects))
		{
			return (0xFBF7);
		}
	}
	else if((0x0626 == cur) && (0x0649 == next))
	{
		/*Isolate*/
		if((!prev_affects) && (!next_affects))
		{
			return (0xFBF8);			//lzz:以Windows显示效果修改为此值
		}
		/*Initial*/
		else if((!prev_affects) && (next_affects))
		{
			return (0xFBFB);
		}
		/*Medium*/
		else if((prev_affects) && (next_affects))
		{
			return (0xFBE9);
		}
		/*Final*/
		else if((prev_affects) && (!next_affects))
		{
			return (0xFBFA);
		}
	}
	else if((0x0626 == cur) && (0x0648 == next))
	{
		/*Isolate*/
		if((!prev_affects) && (!next_affects))
		{
			return (0xFBEE);
		}
		/*Final*/
		else if((prev_affects) && (!next_affects))
		{
			return (0xFBEF);
		}
	}
	else if((0x0626 == cur) && (0x06C7 == next))
	{
		/*Isolate*/
		if((!prev_affects) && (!next_affects))
		{
			return (0xFBF0);
		}
		/*Final*/
		else if((prev_affects) && (!next_affects))
		{
			return (0xFBF1);
		}
	}
	else if((0x0626 == cur) && (0x06C6 == next))
	{
		/*Isolate*/
		if((!prev_affects) && (!next_affects))
		{
			return (0xFBF2);
		}
		/*Final*/
		else if((prev_affects) && (!next_affects))
		{
			return (0xFBF3);
		}
	}
	else if((0x0627 == cur) && (0x0644 == cur))
	{
		/*Isolate*/
		if((!prev_affects) && (!next_affects))
		{
			return (0xFEFB);
		}
		/*Initial*/
		else if((!prev_affects) && (next_affects))
		{
			return (0xFEFC);
		}
		/*Medium*/
		else if((prev_affects) && (next_affects))
		{
			return (0xFEFC);
		}
		/*Final*/
		else if((prev_affects) && (!next_affects))
		{
			return (0xFEFB);
		}
	}

	return (0);
}

static unsigned short _get_arabic_presentform(unsigned short prev2,
		unsigned short prev,
		unsigned short cur,
		unsigned short next,
		unsigned short next2,
		int *ignore)
{
	unsigned short unicode = 0;
	int index = 0, next_affects = 0, prev_affects = 0, prev_index = 0;

	prev_index   = _get_table_index(prev);
	prev_affects = (_get_table_index(prev) || is_transparent(prev)) && (_aKeyInfo[prev_index].Medial);
	next_affects = _get_table_index(next) || is_transparent(next);


	unicode = _get_arabic_ligature(prev2, prev, cur, next, next2);
	if(0 == unicode)
	{
		*ignore = 0;
		index = _get_table_index(cur);
		/*Isolate*/
		if((!prev_affects) && (!next_affects))
		{
			unicode = _aKeyInfo[index].Isolated;
		}
		/*Initial*/
		else if((!prev_affects) && (next_affects))
		{
			unicode = _aKeyInfo[index].Initial;
		}
		/*Medium*/
		else if((prev_affects) && (next_affects))
		{
			unicode = _aKeyInfo[index].Medial;
		}
		/*Final*/
		else if((prev_affects) && (!next_affects))
		{
			unicode = _aKeyInfo[index].Final;
		}

		if(!unicode)
		{
			unicode = _aKeyInfo[index].Isolated;
		}

		*ignore = 0;
	}
	else
	{
		*ignore = 1;
	}

	return (unicode);
}

static unsigned short _arabic_get_unicode(unsigned char *header, unsigned char **string, int flag)
{
	unsigned int unicode = 0, accent = 0;
	unsigned int prev2_code = 0, prev_code = 0, next2_code = 0, next_code = 0;
	unsigned char iso6937_flag = 0;
	unsigned char *pstr = NULL, *pprev = NULL, *pprev2 = NULL, *pnext = NULL, *pnext2 = NULL;
	int ignore = 0;

	pstr = (unsigned char *)(*string);

	if(STRING_ISO88592 == flag)
	{
		unicode = *pstr;
		if(unicode > 0xA0)
		{
			unicode = TurkiShapping[unicode - 0xA0].End;
		}
		else
		{
			;
		}

		pstr++;
	}
	else if(STRING_ARABIC == flag)
	{
		gdi_font_utf82unicode(&pstr, &unicode, &accent, &iso6937_flag);
	}
	else
	{
		gdi_font_utf82unicode(&pstr, &unicode, &accent, &iso6937_flag);;
	}

	if((unicode >= 0x0600) && (unicode <= 0x06FF))
	{
		pprev2 = _utf8_string_move(header, pstr, -3);
		pprev = _utf8_string_move(header, pstr, -2);
		pnext = pstr;
		pnext2 = _utf8_string_move(header, pstr, 1);

		if(pprev2)
		{
			gdi_font_utf82unicode(&pprev2, &prev2_code, &accent, &iso6937_flag);
		}

		if(pprev)
		{
			gdi_font_utf82unicode(&pprev, &prev_code, &accent, &iso6937_flag);
		}

		if(pnext)
		{
			gdi_font_utf82unicode(&pnext, &next_code, &accent, &iso6937_flag);
		}

		if(pnext2)
		{
			gdi_font_utf82unicode(&pnext2, &next2_code, &accent, &iso6937_flag);
		}

		unicode = _get_arabic_presentform(prev2_code,
				prev_code,
				unicode,
				next_code,
				next2_code,
				&ignore);
		if(ignore)
		{
			gdi_font_utf82unicode(&pstr, &next_code, &accent, &iso6937_flag);
			ignore = 0;
		}
	}
	else
	{
		;
	}

	*string = pstr;

	return (unicode);
}

static int _arabic_string_len(unsigned char *pstr, int flag)
{
	unsigned short unicode = 0;
	int ret = 0;
	unsigned char *header = NULL;

	header = pstr;

	while(*pstr)
	{
		unicode = _arabic_get_unicode(header, &pstr, flag);

		ret += gdi_get_charwidth(unicode);
	}

	return (ret);
}

static int string_length(unsigned char *pstr, int flag)
{
	int ret = 0;

	switch(flag)
	{
		case STRING_NORMAL:
		case STRING_HEBREW:
			ret = _normal_string_len(pstr, flag);
			break;
		case STRING_ISO88591:
		case STRING_ISO88593:
		case STRING_ISO88594:
		case STRING_ISO88595:
		case STRING_ISO88596:
		case STRING_ISO88597:
		case STRING_ISO88598:
		case STRING_ISO88599:
		case STRING_ISO885910:
		case STRING_ISO885911:
		case STRING_ISO885912:
		case STRING_ISO885913:
		case STRING_ISO885914:
		case STRING_ISO885915:
		case STRING_ISO885916:
			ret = _iso8859_string_len(pstr, flag);
			break;
		case STRING_ISO6937:
			ret = _iso6937_string_len(pstr, flag);
			break;
		case STRING_ARABIC:
		case STRING_ISO88592:
			ret = _arabic_string_len(pstr, flag);
			break;
		default:
			ret = _normal_string_len(pstr, flag);
			break;
	}

	return (ret);
}

static int font_turki_len(void *string)
{
	int string_flag, ret = 0;
	unsigned char *pstr = NULL;

	if(NULL == string)
	{
		return (1);
	}

	pstr = (unsigned char *)string;

	string_flag = _check_string_flag(&pstr);

	pstr = (unsigned char *)string;

	ret = string_length(pstr, string_flag);

	return (ret);
}

typedef struct _string_para
{
	void *surface;
	unsigned char *string;
	GAL_Rect rect;
	GAL_Interval interval;
	int alignment;
	int color;
}string_para;

static int font_alignment_deal(unsigned char *string, int alignment, GAL_Rect *rect, int format)
{
	unsigned char *pstr = NULL;
	int len = 0;

	if((NULL == string) || (NULL == rect))
	{
		return (1);
	}

	if((STRING_ARABIC == format) || (STRING_HEBREW == format) || (STRING_ISO88592 == format))
	{
		if(GUI_TA_LEFT & alignment)
		{
			alignment |= GUI_TA_RIGHT;
		}
	}

	pstr = string;
	len = string_length(pstr, format);
	if(len < rect->w)
	{
		if(GUI_TA_HCENTRE & alignment)
		{
			rect->x = rect->x + (rect->w - len) / 2;
			rect->w = len;
		}
		else if(GUI_TA_RIGHT & alignment)
		{
			rect->x = rect->x + (rect->w - len);
			rect->w = len;
		}
	}

	return (0);
}

static int _string_normal_display(string_para *para, int format)
{
	unsigned char *pstr = NULL;
	unsigned int unicode = 0, accent = 0;
	unsigned char iso6937_flag = 0;
	GAL_Rect rect = {0};
	int color = 0;
	int xpos = 0, ypos = 0;
	void *surface = NULL;

	if(NULL == para)
	{
		return (0);
	}

	pstr = para->string;
	rect = para->rect;
	color = para->color;
	surface = para->surface;

	xpos = rect.x;
	ypos = rect.y;

	while(*pstr)
	{
		gdi_font_utf82unicode(&pstr, &unicode, &accent, &iso6937_flag);

		if(('\n' == unicode) ||
				((xpos + gdi_get_charwidth(unicode)) > (rect.x + rect.w)))
		{
			break;
		}

		if(0xFEFF == unicode)
		{
			continue;
		}

		gdi_draw_char(surface, &xpos, &ypos, unicode, color);
		xpos += gdi_get_charwidth(unicode);
	}

	return (pstr - para->string);
}

static int _string_iso8859_display(string_para *para, int format)
{
	unsigned char *pstr = NULL;
	unsigned short unicode = 0;
	GAL_Rect rect = {0};
	int color = 0;
	int xpos = 0, ypos = 0;
	void *surface = NULL;

	if(NULL == para)
	{
		return (1);
	}

	pstr = para->string;
	rect = para->rect;
	color = para->color;
	surface = para->surface;

	xpos = rect.x;
	ypos = rect.y;

	while(*pstr)
	{
		if(STRING_ISO_IEC10646 == format)
		{
			unicode = ((*pstr) << 8) | (*pstr);
			pstr++;
		}
		else
		{
			unicode = _iso8859_get_unicode(*pstr, format);
		}

		if(('\n' == unicode) ||
				((xpos + gdi_get_charwidth(unicode)) > (rect.x + rect.w)))
		{
			break;
		}

		if(0xFEFF == unicode)
		{
			continue;
		}

		gdi_draw_char(surface, &xpos, &ypos, unicode, color);
		if(((0x0E33 < unicode) && (unicode < 0x0E3F)) ||
				(0x0E31 == unicode) ||
				((0x0E46 < unicode) && (unicode < 0x0E4F)))
		{
			;
		}
		else
		{
			xpos += gdi_get_charwidth(unicode);
		}

		pstr++;
	}

	return (pstr - para->string);
}

static int _string_iso6937_display(string_para *para, int format)
{
	unsigned char *pstr = NULL;
	unsigned int unicode = 0, accent = 0;
	unsigned char iso6937_flag = 0;
	GAL_Rect rect = {0};
	int color = 0;
	int xpos = 0, ypos = 0;
	void *surface = NULL;

	if(NULL == para)
	{
		return (1);
	}

	pstr = para->string;
	rect = para->rect;
	color = para->color;
	surface = para->surface;

	xpos = rect.x;
	ypos = rect.y;

	while(*pstr)
	{
		gdi_font_utf82unicode(&pstr, &unicode, &accent, &iso6937_flag);

		if(('\n' == unicode) || ((xpos + gdi_get_charwidth(unicode)) > (rect.x + rect.w)))
		{
			break;
		}

		if(0xFEFF == unicode)
		{
			continue;
		}

		gdi_draw_char(surface, &xpos, &ypos, unicode, color);
		if(!iso6937_flag)
		{
			/*Special condition Thai language*/
			if(((0x0E33 < unicode) && (unicode < 0x0E3F)) ||
					(0x0E31 == unicode) ||
					((0x0E46 < unicode) && (unicode < 0x0E4F)))
			{
				;
			}
			else
			{
				xpos += gdi_get_charwidth(unicode);
			}
		}
		else
		{
			gdi_draw_char(surface, &xpos, &ypos, accent, color);
			xpos += gdi_get_charwidth_with_accent(unicode, accent);
		}

		iso6937_flag = 0;
	}

	return (pstr - para->string);

}

int _check_alpha_in_arabic_string(unsigned char *string, int *length)
{
	unsigned char *pstr = NULL;
	unsigned int unicode = 0, accent = 0;
	unsigned char iso6937_flag = 0;
	int pos = 0, len = 0;

	if((NULL == string) || (NULL == length))
	{
		return (0);
	}

	pstr = string;

	while(*pstr)
	{
		gdi_font_utf82unicode(&pstr, &unicode, &accent, &iso6937_flag);
		if(!_is_arabic(unicode) &&
				('\0' != unicode) &&
				(' ' != unicode))
		{
			pstr = _utf8_string_move(string, pstr, -1);
			break;
		}
	}

	if('\0' == *pstr)
	{
		return (0);
	}
	else
	{
		pos = pstr - string;
		pstr = string + pos;
		while(*pstr)
		{
			gdi_font_utf82unicode(&pstr, &unicode, &accent, &iso6937_flag);
			if(_is_arabic(unicode) || ' ' == unicode)
			{
				break;
			}
			len++;
		}
	}

	*length = len;

	return (pos);
}

static int _string_arabic_display(string_para *para, int format)
{
	unsigned char *pstr = NULL;
	unsigned char *header = NULL, *alpha_string = NULL;
	unsigned int unicode = 0, accent = 0;
	unsigned char iso6937_flag = 0;
	GAL_Rect rect = {0}, alpha_rect = {0};
	int color = 0, len = 0, str_length = 0;
	int xpos = 0, ypos = 0;
	unsigned int alpha_pos = 0, alpha_end = 0, alpha_start = 0;
	void *surface = NULL;
	string_para alpha_para = {0};

	if(NULL == para)
	{
		return (0);
	}

	pstr = para->string;
	rect = para->rect;
	color = para->color;
	surface = para->surface;

	xpos = rect.x + rect.w;
	ypos = rect.y;

	header = pstr;

	alpha_pos = _check_alpha_in_arabic_string(pstr, &len);

	if(len)
	{
		alpha_string = (unsigned char *)GxCore_Malloc(len + 1);
		if(NULL == alpha_string)
		{
			return (1);
		}

		str_length = len;
		memset(alpha_string, 0, len + 1);
		strncpy((char *)alpha_string, (char *)(pstr + alpha_pos), len);
		len = font_turki_len(alpha_string);
		alpha_start = alpha_pos;
		alpha_pos = rect.x + rect.w - len;
		alpha_rect = rect;

		alpha_rect.x = alpha_pos;
		alpha_rect.w = len;

		alpha_para.surface = surface;
		alpha_para.alignment = para->alignment;
		alpha_para.color = para->color;
		alpha_para.interval = para->interval;
		alpha_para.rect = alpha_rect;
		alpha_para.string = alpha_string;

		if(*(pstr + alpha_start + str_length) == '\0')
		{
			xpos -= len;
			alpha_end = 1;
		}
	}

	while(*pstr)
	{
		if(STRING_ARABIC == format)
		{
			unicode = _arabic_get_unicode(header, &pstr, format);
		}
		else if(STRING_HEBREW == format)
		{
			gdi_font_utf82unicode(&pstr, &unicode, &accent, &iso6937_flag);
		}

		if(0xFEFF == unicode)
		{
			continue;
		}

		xpos = xpos - gdi_get_charwidth(unicode);
		if(xpos < rect.x)
		{
			break;
		}

		if((unicode > 0x80) || (' ' == unicode))
		{
			gdi_draw_char(surface, &xpos, &ypos, unicode, color);
		}
		else if(NULL != alpha_string)
		{
			xpos += gdi_get_charwidth(unicode);
			if(alpha_end)
			{
				alpha_para.rect.x = rect.x + rect.w - len;
			}
			else
			{
				alpha_para.rect.x = xpos - len;
			}
			_string_normal_display(&alpha_para, STRING_NORMAL);
			GUI_FREE(alpha_string);
			alpha_string = NULL;
			xpos -= len;
			pstr += str_length - 1;
			len = 0;
			str_length = 0;
			alpha_pos = 0;
		}
	}

	return (pstr - para->string);
}

static int font_turki_text(void *screen, void *string, GAL_Rect *rect, GAL_Interval *interval, int alignment,  int color, int flag)
{
	int format = 0, ret = 0;
	unsigned char *pstr = NULL;
	string_para string_trunk = {0};

	if((NULL == screen) || (NULL == string) || (NULL == rect) || (NULL == interval))
	{
		return (1);
	}

	pstr = (unsigned char *)string;

	format = _check_string_flag(&pstr);

	pstr = (unsigned char *)string;

	string_trunk.surface = screen;
	string_trunk.string = string;
	string_trunk.rect = *rect;
	string_trunk.interval = *interval;
	string_trunk.alignment = alignment;
	string_trunk.color = color;

	while(*pstr)
	{
		font_alignment_deal(pstr, alignment, rect, format);
		switch(format)
		{
			case STRING_NORMAL:
				ret = _string_normal_display(&string_trunk, format);
				break;
			case STRING_ISO88591:
			case STRING_ISO88593:
			case STRING_ISO88594:
			case STRING_ISO88595:
			case STRING_ISO88596:
			case STRING_ISO88597:
			case STRING_ISO88598:
			case STRING_ISO88599:
			case STRING_ISO885910:
			case STRING_ISO885911:
			case STRING_ISO885912:
			case STRING_ISO885913:
			case STRING_ISO885914:
			case STRING_ISO885915:
			case STRING_ISO885916:
				ret = _string_iso8859_display(&string_trunk, format);
				break;
			case STRING_ISO6937:
				ret = _string_iso6937_display(&string_trunk, format);
				break;
			case STRING_ARABIC:
			case STRING_HEBREW:
			case STRING_ISO88592:
				ret = _string_arabic_display(&string_trunk, format);
				break;
			default:
				ret = _string_normal_display(&string_trunk, format);
				break;
		}

		pstr += ret;
		if(SINGLELINE_MODE == flag)
		{
			break;
		}
		else
		{
			string_trunk.rect.y += gdi_get_charheight();
			if((string_trunk.rect.y + gdi_get_charheight()) > (rect->y + rect->h))
			{
				return (0);
			}
			string_trunk.string = pstr;
		}

	}

	return (0);

}

unsigned char *_get_line_head(unsigned char *pstart, unsigned char *pstr, unsigned int width)
{
	unsigned char *pline_head = NULL;
	unsigned int line = 0, char_width = 0;
	unsigned char iso6937_flag = 0;
	unsigned int unicode = 0, accent = 0;

	pline_head = pstart;
	pstr = gdi_font_utf8_back(pstart, pstr);

	while(pstart < pstr)
	{
		gdi_font_utf82unicode(&pstart,
				&unicode,
				&accent,
				&iso6937_flag);
		if(0xFEFF == unicode)
		{
			continue;
		}

		if(iso6937_flag)
		{
			char_width = (gdi_get_charwidth(unicode) > gdi_get_charwidth(accent)) ?
				(gdi_get_charwidth(unicode)) : (gdi_get_charwidth(accent));
		}
		else
		{
			char_width = gdi_get_charwidth(unicode);
		}

		line += char_width;
		if((line > width) || ('\n' == unicode))
		{
			if(pstart == pstr)
			{
				break;
			}
			pline_head = pstart;
			line = 0;
		}
		else if(line == width)
		{
			gdi_font_utf82unicode(&pstart,
					&unicode,
					&accent,
					&iso6937_flag);
			pline_head = pstart;
			line = 0;
		}
	}

	return (pline_head);

}

static int font_turki_skip_line(void *p_start, void *string, GAL_Interval *interval, int line_width, int alignment)
{
	unsigned char *pstr = NULL, *phead = NULL, *ptr = NULL;
	unsigned char iso6937_flag = 0;
	unsigned int unicode = 0, accent = 0;
	unsigned int width = 0, char_width = 0, line = 0, offset = 0;
	int arabic_flag = 0;

	if((NULL == p_start) || (NULL == string) || (NULL == interval))
	{
		return (1);
	}

	pstr = (unsigned char *)string;
	phead = (unsigned char *)p_start;

	width = (line_width < 0) ? (-line_width) : (line_width);

	ptr = string;
	if(STRING_ARABIC == _check_string_flag(&ptr))
	{
		arabic_flag = 1;
	}
	line = 0;
	if(line_width > 0)
	{
		do
		{
			if('\0' == *pstr)
			{
				return (0);
			}

			if(arabic_flag)
			{
				unicode = _arabic_get_unicode(phead, &pstr, STRING_ARABIC);
			}
			else
			{
				gdi_font_utf82unicode(&pstr, &unicode, &accent, &iso6937_flag);
			}

			if(0xFEFF == unicode)
			{
				continue;
			}

			if(iso6937_flag)
			{
				char_width = (gdi_get_charwidth(unicode) > gdi_get_charwidth(accent)) ?
					(gdi_get_charwidth(unicode)) : (gdi_get_charwidth(accent));
			}
			else
			{
				char_width = gdi_get_charwidth(unicode);
			}

			line += char_width;
			if(line > width)
			{
				break;
			}

			if('\n' == unicode)
			{
				break;
			}

			char_width = 0;
		}while(line < width);

		if(line == width)
		{
			gdi_font_utf82unicode(&pstr, &unicode, &accent, &iso6937_flag);
		}
		offset = pstr - (unsigned char *)string;
	}
	else if(phead < pstr)
	{
		line = 0;
		do
		{
			pstr = gdi_font_utf8_back(phead, pstr);
			if(arabic_flag)
			{
				unicode = _arabic_get_unicode(phead,
						&pstr,
						STRING_ARABIC);
			}
			else
			{
				gdi_font_utf82unicode(&pstr,
						&unicode,
						&accent,
						&iso6937_flag);
			}

			if(0xFEFF == unicode)
			{
				break;
			}

			if('\n' == unicode)
			{
				break;
			}
			else
			{
				line += gdi_get_charwidth(unicode);
				pstr = gdi_font_utf8_back(phead, pstr);
			}

			if(pstr == phead)
			{
				break;
			}

		}while(line < width);

		if((line < width) && ('\n' == unicode))
		{
			pstr = _get_line_head(phead, pstr, width);
		}
		else if((line == width) && (!arabic_flag))
		{
			pstr = gdi_font_utf8_back(phead, pstr);
		}
		else if((line >= width) && arabic_flag)
		{
			pstr = gdi_font_utf8_back(phead, pstr);
		}

		offset = (unsigned char *)string - pstr;
	}

	return (offset);
}

static int font_turki_roll_text(void *screen, void *string, GAL_Rect* rect, int pos, int color, int flag)
{
	void *surface = NULL;
	unsigned char *pstr = NULL;
	unsigned char iso6937_flag = 0;
	unsigned int unicode = 0, accent = 0;
	unsigned int char_width = 0, line = 0, fore_color = 0;
	GAL_Interval interval = {0};
	GAL_Rect src_rect = {0}, dst_rect = {0};

	pstr = (unsigned char *)string;

	if(pos >= 0)
	{
		while(*pstr)
		{
			gdi_font_utf82unicode(&pstr,
					&unicode,
					&accent,
					&iso6937_flag);
			if(iso6937_flag)
			{
				char_width = (gdi_get_charwidth(unicode) > gdi_get_charwidth(accent)) ?
					(gdi_get_charwidth(unicode)) : (gdi_get_charwidth(accent));
			}
			else
			{
				char_width = gdi_get_charwidth(unicode);
			}

			if(pos > char_width)
			{
				line += char_width;
				if(pos < line)
				{
					pstr = gdi_font_utf8_back(string, pstr);
					break;
				}
				else
				{
					continue;
				}
			}
			else
			{
				pstr = gdi_font_utf8_back(string, pstr);
				line = 0;
				char_width = 0;
				break;
			}
		}
	}

	src_rect = *rect;
	src_rect.x = src_rect.y = 0;
	if(SINGLELINE_MODE == flag)
	{
		line = src_rect.w = src_rect.w + gdi_get_charwidth(unicode) * 2;
	}
	else
	{
		src_rect.h += gdi_get_charheight();
	}

	surface = gal_get_surface(&src_rect, gui.config.bpp);
	gdi_commit();

	if(gui.config.enable_double_buffer)
	{
		src_rect.w = rect->w;
		src_rect.x += pos;
		gal_copy_surface(screen, rect, surface, &src_rect);
		src_rect.x = 0;
		src_rect.w = line;
	}
	else
	{
		fore_color = gui.config.gui_trans;
		fore_color = gal_color2index(screen, fore_color);
		hd_fillrect(surface, &src_rect, fore_color);
	}

	gdi_commit();
	font_turki_text(surface,
			pstr,
			&src_rect,
			&interval,
			GUI_TA_LEFT | GUI_TA_VCENTRE,
			color,
			flag);
	gdi_commit();

	if((SINGLELINE_MODE == flag) && (pos >= 0))
	{
		src_rect.x += pos - (line - char_width);
		src_rect.w = rect->w;
	}
	else if((PARAGRAPH_MODE == flag) && (pos >= 0))
	{
		src_rect.y += (pos % gdi_get_charheight());
	}

	dst_rect = *rect;
	if(pos < 0)
	{
		dst_rect.x = dst_rect.x - pos;
		src_rect.w = dst_rect.w = dst_rect.w + pos;
	}

	gal_stretch_surface(surface, &src_rect, screen, &dst_rect);

	hd_add_blit_element(surface);

	gdi_commit();

	return (0);
}

static int font_turki_lines(void *string, GAL_Rect *rect, GAL_Interval *interval)
{
	unsigned int line_count = 0, text_width = 0, width = 0;
	unsigned char *pstr = NULL;
	unsigned int unicode = 0, accent = 0;
	unsigned char iso6937_flag = 0;

	if((NULL == string) || (NULL == rect) || (NULL == interval))
	{
		return (1);
	}

	pstr = (unsigned char *)string;
	line_count = 1;

	while(*pstr)
	{
		gdi_font_utf82unicode(&pstr, &unicode, &accent, &iso6937_flag);

		if('\n' == unicode)
		{
			text_width = 0;
			line_count++;
		}

		if(iso6937_flag)
		{
			width = (gdi_get_charwidth(unicode) > gdi_get_charwidth(accent)) ?
				(gdi_get_charwidth(unicode)) : (gdi_get_charwidth(accent));
		}
		else
		{
			width = gdi_get_charwidth(unicode);
		}

		text_width += width;

		if(text_width >= (rect->w))
		{
			line_count++;
			text_width = 0;
		}
	}


	return (line_count);
}

struct gdi_font_opt font_turki_opt = {
	"turki",
	font_turki_len,
	font_turki_text,
	font_turki_skip_line,
	font_turki_roll_text,
	font_turki_lines
};
