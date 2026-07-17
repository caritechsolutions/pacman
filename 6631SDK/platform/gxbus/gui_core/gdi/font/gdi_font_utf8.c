#ifndef NO_OS
#include <stdio.h>
#include <stdlib.h>
#include "gui.h"
#include "gui_private.h"
#else

#include "common/gxmalloc.h"
#include "mini_gui_private.h"
#include "gal_blit.h"
#include "gal_ttf.h"
#include "hd_gal.h"

#endif /*NO_OS*/

#include "gdi_char.h"
#include "gdi_font.h"
#include "gdi_font_utf8.h"

#include "khmer.h"
#include "devanagari.h"

static const ArabicMap ArabicShapping[] =
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
	{0xD7, 0x0637},/*28*/
	{0xD8, 0x0638},/*29*/
	{0xD9, 0x0639},/*30*/
	{0xDA, 0x063A},/*31*/
	{0xE0, 0x0640},/*32*/
	{0xE1, 0x0641},/*33*/
	{0xE2, 0x0642},/*34*/
	{0xE3, 0x0643},/*35*/
	{0xE4, 0x0644},/*36*/
	{0xE5, 0x0645},/*37*/
	{0xE6, 0x0646},/*38*/
	{0xE7, 0x0647},/*39*/
	{0xE8, 0x0648},/*40*/
	{0xE9, 0x0649},/*41*/
	{0xEA, 0x064A},/*42*/
	{0xEB, 0x064B},/*43*/
	{0xEC, 0x064C},/*44*/
	{0xED, 0x064D},/*45*/
	{0xEE, 0x064E},/*46*/
	{0xEF, 0x064F},/*47*/
	{0xF0, 0x0650},/*48*/
	{0xF1, 0x0651},/*49*/
	{0xF2, 0x0652} /*50*/
};

static KEY_INFO _aKeyInfo[] =
{
	/*    Base      Isol.   Final   Initial Medial */
	{ /* 0  */0x0621, 0xFE80, 0x0000, 0x0000, 0x0000 },
	{ /* 1  */0x0622, 0xFE81, 0xFE82, 0x0000, 0x0000 },
	{ /* 2  */0x0623, 0xFE83, 0xFE84, 0x0000, 0x0000 },
	{ /* 3  */0x0624, 0xFE85, 0xFE86, 0x0000, 0x0000 },
	{ /* 4  */0x0625, 0xFE87, 0xFE88, 0x0000, 0x0000 },
	{ /* 5  */0x0626, 0xFE89, 0xFE8A, 0xFE8B, 0xFE8C },
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
	{ /* 34 */0x0649, 0xFEEF, 0xFEF0, 0x0000, 0x0000 },
	{ /* 35 */0x064A, 0xFEF1, 0xFEF2, 0xFEF3, 0xFEF4 },
	{ /* 36 Uigur*/0x0675, 0x0000, 0xFE8E, 0x0000, 0x0000},
	{ /* 37 Uigur*/0x0676, 0xFE85, 0xFEEE, 0x0000, 0x0000},
	{ /* 38 Uigur*/0x0677, 0xFBDD, 0xFBD8, 0x0000, 0x0000},
	{ /* 39 */0x0678, 0xFE89, 0xFE8A, 0xFE8B, 0xFE8C },
	{ /* 40 */0x067E, 0xFB56, 0xFB57, 0xFB58, 0xFB59 },
	{ /* 41 */0x0686, 0xFB7A, 0xFB7B, 0xFB7C, 0xFB7D },
	{ /* 42 */0x0698, 0xFB8A, 0xFB8B, 0x0000, 0x0000 },
	{ /* 43 */0x06A9, 0xFB8E, 0xFB8F, 0xFB90, 0xFB91 },
	{ /* 44 Uigur*/0x06AD, 0xFBD3, 0xFBD4, 0xFBD5, 0xFBD6},
	{ /* 45 */0x06AF, 0xFB92, 0xFB93, 0xFB94, 0xFB95 },
	{ /* 46 Uigur*/0x06BE, 0xFBAA, 0xFBAD, 0xFBAC, 0xFBAB},
	{ /* 47 Uigur*/0x06C5, 0xFBE0, 0xFBE1, 0xFBAC, 0xFBAD},
	{ /* 48 Uigur*/0x06C6, 0xFBD9, 0xFBDA, 0x0000, 0x0000},
	{ /* 49 Uigur*/0x06C7, 0xFBD7, 0xFBD8, 0x0000, 0x0000},
	{ /* 50 Uigur*/0x06C8, 0xFBDB, 0xFBDC, 0x0000, 0x0000},
	{ /* 51 Uigur*/0x06C9, 0xFBE2, 0xFBE2, 0x0000, 0x0000},
	{ /* 52 Uigur*/0x06CB, 0xFBDE, 0xFBDF, 0x0000, 0x0000},
	{ /* 53 */0x06CC, 0xFBFC, 0xFBFD, 0xFBFE, 0xFBFF},
	{ /* 54 Uigur*/0x06D0, 0xFBF4, 0xFBE5, 0xFBE6, 0xFBE7},
	{ /* 55 Uigur*/0x06D5, 0xFEE9, 0xFEEA, 0x0000, 0x0000}
};
unsigned int table_iso6937[] =
{
	0x0000, 0x0300, 0x0301, 0x0302, 0x0303,
	0x0304, 0x0306, 0x0307, 0x0308, 0x0000,
	0x030A, 0x0327, 0x0000, 0x030B, 0x0328,
	0x030C
};

static int first_mark_set[14] = {
	0x0300, 0x0301, 0x0302, 0x0303, 0x0304, 0x0306, 0x0307, 0x0308, 0x030A, 0x0327, 0x030B, 0x0328, 0x030C, 0
};

static int second_letter_set[13][30] = {
	/*GRAVE*/
	{'A', 'a', 'E', 'e', 'I', 'i', 'O', 'o', 'U', 'u', 0},
	/*ACUTE*/
	{'A', 'a', 'C', 'c', 'E', 'e', 'I', 'i', 'L', 'l', 'N', 'n', 'O', 'o', 'R', 'r', 'S', 's', 'U', 'u', 'Y', 'y',
	'Z', 'z', 0},
	/*CIRCUMFLEX*/
	{'A', 'a', 'C', 'c', 'E', 'e', 'G', 'g', 'H', 'h', 'I', 'i', 'J', 'j', 'O', 'o', 'S', 's', 'U', 'u', 'W', 'w',
	'Y', 'y', 0},
	/*TILDE*/
	{'A', 'a', 'I', 'i', 'N', 'n', 'O', 'o', 'N', 'n', 0},
	/*MACRON*/
	{'A', 'a', 'E', 'e', 'I', 'i', 'O', 'o', 'U', 'u', 0},
	/*BREVE*/
	{'A', 'a', 'G', 'g', 'U', 'u', 0},
	/*DOT ABOVE*/
	{'C', 'c', 'E', 'e', 'G', 'g', 'I', 'Z', 'z', 0},
	/*DIAERESIS*/
	{'A', 'a', 'E', 'e', 'I', 'i', 'O', 'o', 'U', 'u', 'Y', 'y', 0},
	/*RING ABOVE*/
	{'A', 'a', 'U', 'u', 0},
	/*CEDILLA*/
	{'C', 'c', 'G', 'g', 'K', 'k', 'L', 'l', 'N', 'n', 'R', 'r', 'S', 's', 'T', 't', 0},
	/*DOUBLE ACUTE*/
	{'O', 'o', 'U', 'u', 0},
	/*OGONEK*/
	{'A', 'a', 'E', 'e', 'I', 'i', 'U', 'u', 0},
	/*CARON*/
	{'C', 'c', 'D', 'd', 'E', 'e', 'L', 'l', 'N', 'n', 'R', 'r', 'S', 's', 'T', 't', 'Z', 'z', 0}
};

static int convert_latin_letter_set[13][30] = {
	/*GRAVE*/
	{/*À*/0x00C0, /*à*/0x00E0, /*È*/0x00C8, /*è*/0x00E8, /*Ì*/0x00CC, /*ì*/0x00EC, /*Ò*/0x00D2, /*ò*/0x00F2, /*Ù*/0x00DA, /*ù*/0x00FA},
	/*ACUTE*/
	{/*Á*/0x00C1, /*á*/0x00E1, /*Ć*/0x0106, /*ć*/0x0107, /*É*/0x00C9, /*é*/0x00E9, /*Í*/0x00CD, /*í*/0x00ED, /*Ĺ*/0x0139, /*ĺ*/0x013A,
	 /*Ń*/0x0143, /*ń*/0x0144, /*Ó*/0x00D3, /*ó*/0x00F3, /*Ŕ*/0x0154, /*ŕ*/0x0155, /*Ś*/0x015A, /*ś*/0x015B, /*Ú*/0x00DA, /*ú*/0x00FA,
	 /*Ý*/0x00DD, /*ý*/0x00FD, /*Ź*/0x0179, /*ź*/0x017A},
	/*CIRCUMFLEX*/
	{/*Â*/0x00C2, /*â*/0x00E2, /*Ĉ*/0x0108, /*ĉ*/0x0109, /*Ê*/0x00CA, /*ê*/0x00EA, /*Ĝ*/0x011C, /*ĝ*/0x011D, /*Ĥ*/0x0124, /*ĥ*/0x0125,
	 /*Î*/0x00CE, /*î*/0x00EE, /*Ĵ*/0x0134, /*ĵ*/0x0135, /*Ô*/0x00D4, /*ô*/0x00F4, /*Ŝ*/0x015C, /*š*/0x0161, /*Û*/0x00DB, /*û*/0x00FB,
	 /*Ŵ*/0x0174, /*ŵ*/0x0175, /*Ŷ*/0x0176, /*ŷ*/0x0177},
	/*TILDE*/
	{/*Ã*/0x00C3, /*ã*/0x00E3, /*Ĩ*/0x0128, /*ĩ*/0x0129, /*Ñ*/0x00D1, /*ñ*/0x00F1, /*Õ*/0x00D5, /*õ*/0x00F5, /*Ñ*/0x00D1, /*ñ*/0x00F1},
	/*MACRON*/
	{/*Ā*/0x0100, /*ā*/0x0101, /*Ē*/0x0112, /*ē*/0x0113, /*Ī*/0x012A, /*ī*/0x012B, /*Ō*/0x014C, /*ō*/0x014D, /*Ū*/0x016A, /*ū*/0x016B},
	/*BREVE*/
	{/*Ă*/0x0102, /*ă*/0x0103, /*Ğ*/0x011E, /*ğ*/0x011F, /*Ŭ*/0x016C, /*ŭ*/0x016D},
	/*DOT ABOVE*/
	{/*Ċ*/0x010A, /*ċ*/0x010B, /*Ė*/0x0116, /*ė*/0x0117, /*Ġ*/0x0120, /*ġ*/0x0121, /*İ*/0x0130, /*Ż*/0x017B, /*ż*/0x017C},
	/*DIAERESIS*/
	{/*Ä*/0x00C4, /*ä*/0x00E4, /*Ë*/0x00CB, /*ë*/0x00EB, /*Ï*/0x00CF, /*ï*/0x00EF, /*Ö*/0x00D6, /*ö*/0x00F6, /*Ü*/0x00DC, /*ü*/0x00FC,
	 /*Ÿ*/0x0178, /*ÿ*/0x00FF},
	/*RING ABOVE*/
	{/*Å*/0x00C5, /*å*/0x00E5, /*Ů*/0x016E, /*ů*/0x016F},
	/*CEDILLA*/
	{/*Ç*/0x00C7, /*ç*/0x00E7, /*Ģ*/0x0122, /*ģ*/0x0123, /*Ķ*/0x0136, /*ķ*/0x0137, /*Ļ*/0x013B, /*ļ*/0x013C, /*Ņ*/0x0145, /*ņ*/0x0146,
	 /*Ŗ*/0x0156, /*ŗ*/0x0157, /*Ş*/0x015E, /*ş*/0x015F, /*Ţ*/0x0162, /*ţ*/0x0163},
	/*DOUBLE ACUTE*/
	{/*Ő*/0x0150, /*ő*/0x0151, /*Ű*/0x0170, /*ű*/0x0171},
	/*OGONEK*/
	{/*Ą*/0x0104, /*ą*/0x0105, /*Ę*/0x0118, /*ę*/0x0119, /*Į*/0x012E, /*į*/0x012F, /*Ų*/0x0172, /*ų*/0x0173},
	/*CARON*/
	{/*Č*/0x010C, /*č*/0x010D, /*Ď*/0x010E, /*ď*/0x010F, /*Ě*/0x011A, /*ě*/0x011B, /*Ľ*/0x013D, /*ľ*/0x013E, /*Ň*/0x0147, /*ň*/0x0148,
	 /*Ř*/0x0158, /*ř*/0x0159, /*Š*/0x0160, /*š*/0x0161, /*Ť*/0x0164, /*ť*/0x0165, /*Ž*/0x017D, /*ž*/0x017E},
};

int _get_latin_letter_by_mark(int mark, int letter)
{
	int i = 0,j = 0, *letter_ptr = NULL;

	for(i = 0; i < 14; i++) {
		if(mark == first_mark_set[i]) {
			break;
		}
	}

	if(i == 13) {
		return (0);
	}

	letter_ptr = second_letter_set[i];
	j = 0;
	while(*letter_ptr) {
		if(*letter_ptr == letter) {
			break;
		}
		j++;
		letter_ptr++;
	}

	if(*letter_ptr) {
		return (convert_latin_letter_set[i][j]);
	} else {
		return (0);
	}
}

#ifndef NO_OS
#ifdef GUI_STR_ICONV

static unsigned int table_iso88591[] =
{
	0x00A0,0x00A1,0x00A2,0x00A3,	0x00A4,0x00A5,0x00A6,0x00A7,
	0x00A8,0x00A9,0x00AA,0x00AB,	0x00AC,0x00AD,0x00AE,0x00AF,
	0x00B0,0x00B1,0x00B2,0x00B3,	0x00B4,0x00B5,0x00B6,0x00B7,
	0x00B8,0x00B9,0x00BA,0x00BB,	0x00BC,0x00BD,0x00BE,0x00BF,
	0x00C0,0x00C1,0x00C2,0x00C3,	0x00C4,0x00C5,0x00C6,0x00C7,
	0x00C8,0x00C9,0x00CA,0x00CB,	0x00CC,0x00CD,0x00CE,0x00CF,
	0x00D0,0x00D1,0x00D2,0x00D3,	0x00D4,0x00D5,0x00D6,0x00D7,
	0x00D8,0x00D9,0x00DA,0x00DB,	0x00DC,0x00DD,0x00DE,0x00DF,
	0x00E0,0x00E1,0x00E2,0x00E3,	0x00E4,0x00E5,0x00E6,0x00E7,
	0x00E8,0x00E9,0x00EA,0x00EB,	0x00EC,0x00ED,0x00EE,0x00EF,
	0x00F0,0x00F1,0x00F2,0x00F3,	0x00F4,0x00F5,0x00F6,0x00F7,
	0x00F8,0x00F9,0x00FA,0x00FB,	0x00FC,0x00FD,0x00FE,0x00FF
};

static unsigned int table_iso88592[] =
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
};

static unsigned int table_iso88593[] =
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

static unsigned int table_iso88594[] =
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

static unsigned int table_iso88595[] =
{
	0x00A0,0x0401,0x0402,0x0403,    0x0404,0x0405,0x0406,0x0407,
	0x0408,0x0409,0x040A,0x040B,    0x040C,0x00AD,0x040E,0x040F,
	0x0410,0x0411,0x0412,0x0413,    0x0414,0x0415,0x0416,0x0417,
	0x0418,0x0419,0x041A,0x041B,    0x041C,0x041D,0x041E,0x041F,
	0x0420,0x0421,0x0422,0x0423,    0x0424,0x0425,0x0426,0x0427,
	0x0428,0x0429,0x042A,0x042B,    0x042C,0x042D,0x042E,0x042F,
	0x0430,0x0431,0x0432,0x0433,    0x0434,0x0435,0x0436,0x0437,
	0x0438,0x0439,0x043A,0x043B,    0x043C,0x043D,0x043E,0x043F,
	0x0440,0x0441,0x0442,0x0443,    0x0444,0x0445,0x0446,0x0447,
	0x0448,0x0449,0x044A,0x044B,    0x044C,0x044D,0x044E,0x044F,
	0x2116,0x0451,0x0452,0x0453,    0x0454,0x0455,0x0456,0x0457,
	0x0458,0x0459,0x045A,0x045B,    0x045C,0x00A7,0x045E,0x045F
};

static unsigned int table_iso88596[] =
{
	0x0000,0x0000,0x0000,0x0000,	0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,	0x060C,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,	0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x061B,	0x0000,0x0000,0x0000,0x061F,
	0x0620,0x0621,0x0622,0x0623,	0x0624,0x0625,0x0626,0x0627,
	0x0628,0x0629,0x062A,0x062B,	0x062C,0x062D,0x062E,0x062F,
	0x0630,0x0631,0x0632,0x0633,	0x0634,0x0635,0x0636,0x0637,
	0x0638,0x0639,0x063A,0x0000,	0x0000,0x0000,0x0000,0x0000,
	0x0640,0x0641,0x0642,0x0643,	0x0644,0x0645,0x0646,0x0647,
	0x0648,0x0649,0x064A,0x064B,	0x064C,0x064D,0x064E,0x064F,
	0x0650,0x0651,0x0652,0x0653,	0x0654,0x0655,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,    0x0000,0x0000,0x0000,0x0000
};

static unsigned int table_iso88597[] =
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

static unsigned int table_iso88598[] =
{
	0x00A0,0x0000,0x00A2,0x00A3,	0x00A4,0x00A5,0x00A6,0x00A7,
	0x00A8,0x00A9,0x00D7,0x00AB,	0x00AC,0x00AD,0x00AE,0x00AF,
	0x00B0,0x00B1,0x00B2,0x00B3,	0x00B4,0x00B5,0x00B6,0x00B7,
	0x00B8,0x00B9,0x00F7,0x00BB,	0x00BC,0x00BD,0x00BE,0x0000,
	0x0000,0x0000,0x0000,0x0000,	0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,	0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,	0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,	0x0000,0x0000,0x0000,0x2017,
	0x05D0,0x05D1,0x05D2,0x05D3,	0x05D4,0x05D5,0x05D6,0x05D7,
	0x05D8,0x05D9,0x05DA,0x05DB,	0x05DC,0x05DD,0x05DE,0x05DF,
	0x05E0,0x05E1,0x05E2,0x05E3,	0x05E4,0x05E5,0x05E6,0x05E7,
	0x05E8,0x05E9,0x05EA,0x0000,	0x0000,0x200E,0x200F,0x0000
};

static unsigned int table_iso88599[] =
{
	0x00A0,0x00A1,0x00A2,0x00A3,	0x00A4,0x00A5,0x00A6,0x00A7,
	0x00A8,0x00A9,0x00AA,0x00AB,	0x00AC,0x00AD,0x00AE,0x00AF,
	0x00B0,0x00B1,0x00B2,0x00B3,	0x00B4,0x00B5,0x00B6,0x00B7,
	0x00B8,0x00B9,0x00BA,0x00BB,	0x00BC,0x00BD,0x00BE,0x00BF,
	0x00C0,0x00C1,0x00C2,0x00C3,	0x00C4,0x00C5,0x00C6,0x00C7,
	0x00C8,0x00C9,0x00CA,0x00CB,	0x00CC,0x00CD,0x00CE,0x00CF,
	0x011E,0x00D1,0x00D2,0x00D3,	0x00D4,0x00D5,0x00D6,0x00D7,
	0x00D8,0x00D9,0x00DA,0x00DB,	0x00DC,0x0130,0x015E,0x00DF,
	0x00E0,0x00E1,0x00E2,0x00E3,	0x00E4,0x00E5,0x00E6,0x00E7,
	0x00E8,0x00E9,0x00EA,0x00EB,	0x00EC,0x00ED,0x00EE,0x00EF,
	0x011F,0x00F1,0x00F2,0x00F3,	0x00F4,0x00F5,0x00F6,0x00F7,
	0x00F8,0x00F9,0x00FA,0x00FB,	0x00FC,0x0131,0x015F,0x00FF
};

static unsigned int table_iso885910[] =
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

static unsigned int table_iso885911[] =
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

static unsigned int table_iso885913[] =
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

static unsigned int table_iso885914[] =
{
	0x00A0,0x1E02,0x1E03,0x00A3,	0x010A,0x010B,0x1E0A,0x00A7,
	0x1E80,0x00A9,0x1E82,0x1E0B,	0x1EF2,0x00AD,0x00AE,0x0178,
	0x1E1E,0x1E1F,0x0120,0x0121,	0x1E40,0x1E41,0x00B6,0x1E56,
	0x1E81,0x1E57,0x1E83,0x1E60,	0x1EF3,0x1E84,0x1E85,0x1E61,
	0x00C0,0x00C1,0x00C2,0x00C3,	0x00C4,0x00C5,0x00C6,0x00C7,
	0x00C8,0x00C9,0x00CA,0x00CB,	0x00CC,0x00CD,0x00CE,0x00CF,
	0x0174,0x00D1,0x00D2,0x00D3,	0x00D4,0x00D5,0x00D6,0x1E6A,
	0x00D8,0x00D9,0x00DA,0x00DB,	0x00DC,0x00DD,0x0176,0x00DF,
	0x00E0,0x00E1,0x00E2,0x00E3,	0x00E4,0x00E5,0x00E6,0x00E7,
	0x00E8,0x00E9,0x00EA,0x00EB,	0x00EC,0x00ED,0x00EE,0x00EF,
	0x0175,0x00F1,0x00F2,0x00F3,	0x00F4,0x00F5,0x00F6,0x1E6B,
	0x00F8,0x00F9,0x00FA,0x00FB,	0x00FC,0x00FD,0x0177,0x00FF
};

static unsigned int table_iso885915[] =
{
	0x00A0,0x00A1,0x00A2,0x00A3,	0x20AC,0x00A5,0x0160,0x00A7,
	0x0161,0x00A9,0x00AA,0x00AB,	0x00AC,0x00AD,0x00AE,0x00AF,
	0x00B0,0x00B1,0x00B2,0x00B3,	0x017D,0x00B5,0x00B6,0x00B7,
	0x017E,0x00B9,0x00BA,0x00BB,	0x0152,0x0153,0x0178,0x00BF,
	0x00C0,0x00C1,0x00C2,0x00C3,	0x00C4,0x00C5,0x00C6,0x00C7,
	0x00C8,0x00C9,0x00CA,0x00CB,	0x00CC,0x00CD,0x00CE,0x00CF,
	0x00D0,0x00D1,0x00D2,0x00D3,	0x00D4,0x00D5,0x00D6,0x00D7,
	0x00D8,0x00D9,0x00DA,0x00DB,	0x00DC,0x00DD,0x00DE,0x00DF,
	0x00E0,0x00E1,0x00E2,0x00E3,	0x00E4,0x00E5,0x00E6,0x00E7,
	0x00E8,0x00E9,0x00EA,0x00EB,	0x00EC,0x00ED,0x00EE,0x00EF,
	0x00F0,0x00F1,0x00F2,0x00F3,	0x00F4,0x00F5,0x00F6,0x00F7,
	0x00F8,0x00F9,0x00FA,0x00FB,	0x00FC,0x00FD,0x00FE,0x00FF
};

static unsigned int table_iso885916[] =
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

static unsigned int iso6937_char_set[] =
{
	0x00A0,0x00A1,0x00A2,0x00A3,    0x20AC,0x00A5,0x0000,0x00A7,
	0x00A4,0x2018,0x201C,0x00AB,	0x2190,0x2191,0x2192,0x2193,
	0x00B0,0x00B1,0x00B2,0x00B3,	0x00D7,0x00B5,0x00B6,0x00B7,
	0x00F7,0x2019,0x201D,0x00BB,	0x00BC,0x00BD,0x00BE,0x00BF,
	0x0000,0x0300,0x0301,0x0302,	0x0303,0x0304,0x0306,0x0307,
	0x0308,0x0000,0x030A,0x0327,	0x0000,0x030B,0x0328,0x030C,
	0x2015,0x00B9,0x00AE,0x00A9,	0x2122,0x266A,0x00AC,0x00A6,
	0x0000,0x0000,0x0000,0x0000,	0x215B,0x215C,0x215D,0x215E,
	0x2126,0x00C6,0x0110,0x00AA,	0x0126,0x0000,0x0132,0x013F,
	0x0141,0x00D8,0x0152,0x00BA,	0x00DE,0x0166,0x014A,0x0149,
	0x0138,0x00E6,0x0111,0x00F0,	0x0127,0x0131,0x0133,0x0140,
	0x0142,0x00F8,0x0153,0x00DF,	0x00FE,0x0167,0x014B,0x00AD
};

#endif /*GUI_STR_ICONV*/
#endif /*NO_OS*/

#ifndef NO_OS
#ifdef GUI_STR_ICONV
static unsigned int *_get_iso_table(int code)
{
	unsigned int *iso_table = NULL;

	switch(code)
	{
		case 1:
			iso_table = table_iso88591;
			break;
		case 2:
			iso_table = table_iso88592;
			break;
		case 3:
			iso_table = table_iso88593;
			break;
		case 4:
			iso_table = table_iso88594;
			break;
		case 5:
			iso_table = table_iso88595;
			break;
		case 6:
			iso_table = table_iso88596;
			break;
		case 7:
			iso_table = table_iso88597;
			break;
		case 8:
			iso_table = table_iso88598;
			break;
		case 9:
			iso_table = table_iso88599;
			break;
		case 10:
			iso_table = table_iso885910;
			break;
		case 11:
			iso_table = table_iso885911;
			break;
		case 13:
			iso_table = table_iso885913;
			break;
		case 14:
			iso_table = table_iso885914;
			break;
		case 15:
			iso_table = table_iso885915;
			break;
		case 16:
			iso_table = table_iso885916;
			break;
		case 31:
			iso_table = iso6937_char_set;
			break;
		case 111:
			return (0);
		default:
			break;
	}

	return (iso_table);
}
#endif /*GUI_STR_ICONV*/
#endif /*NO_OS*/

unsigned int *get_iso2unicode_table(int code)
{
	unsigned int *iso_table = NULL;

#ifndef NO_OS
#ifdef GUI_STR_ICONV
	switch(code)
	{
		case 0:
			iso_table = iso6937_char_set;
			break;
		case 1:
			iso_table = table_iso88591;
			break;
		case 2:
			iso_table = table_iso88592;
			break;
		case 3:
			iso_table = table_iso88593;
			break;
		case 4:
			iso_table = table_iso88594;
			break;
		case 5:
			iso_table = table_iso88595;
			break;
		case 6:
			iso_table = table_iso88596;
			break;
		case 7:
			iso_table = table_iso88597;
			break;
		case 8:
			iso_table = table_iso88598;
			break;
		case 9:
			iso_table = table_iso88599;
			break;
		case 10:
			iso_table = table_iso885910;
			break;
		case 11:
			iso_table = table_iso885911;
			break;
		case 13:
			iso_table = table_iso885913;
			break;
		case 14:
			iso_table = table_iso885914;
			break;
		case 15:
			iso_table = table_iso885915;
			break;
		case 16:
			iso_table = table_iso885916;
			break;
		case 111:
			return (0);
		default:
			break;
	}
#endif /*GUI_STR_ICONV*/
#endif /*NO_OS*/

	return (iso_table);
}


static unsigned int _get_arabic_unicode(unsigned char ascii_code)
{
	unsigned int unicode = 0, table_size = 0;
	unsigned int i = 0;

	table_size = sizeof(ArabicShapping) / sizeof(ArabicMap);
	unicode = ascii_code;
	while(i < table_size)
	{
		if(ascii_code == ArabicShapping[i].Start)
		{
			unicode = ArabicShapping[i].End;
			return (unicode);
		}
		i++;
	}

	return (unicode);
}

static int _GetTableIndex(unsigned int Char);

static int _is_cjk_symbol(unsigned int unicode)
{
    if(((unicode >= 0x2E00) && (unicode <= 0x2EFF)) ||
       ((unicode >= 0xF900) && (unicode <= 0xFAFF)) ||
       ((unicode >= 0xFE30) && (unicode <= 0xFE4F)) ||
       ((unicode >= 0x20000) && (unicode <= 0x2FA1F)))
	return GUI_TRUE;
    else
	return GUI_FALSE;
}

static unsigned int GxGetPresentationForm(unsigned int *p_uc, unsigned int Char, unsigned int Next, unsigned int Prev, int* pIgnoreNext);
static int _calc_word_len(unsigned char *str, int *len, int width, int tail)
{
    unsigned char is_iso6937 = 0, *pstr = NULL, *tmp_str = NULL;
    unsigned int arabic_code = 0, accent_code = 0, prev_code = 0, next_code = 0, tmp_uc[5], max_width = 0;
    int offset = 0, flag = 0;

    if((NULL == str) && (NULL == len))
    {
	    return (0);
    }

    *len = 0;
    while(*str)
    {
	pstr = str;

	prev_code = arabic_code;
	gdi_font_utf82unicode(&str, &arabic_code, &accent_code, &is_iso6937);
	tmp_str = str;
	gdi_font_utf82unicode(&tmp_str, &next_code, &accent_code, &is_iso6937);
	memset(tmp_uc, 0, sizeof(tmp_uc));
	tmp_uc[0] = next_code;
	tmp_uc[1] = arabic_code;
	tmp_uc[2] = prev_code;

	if((' ' == arabic_code) || (_is_cjk_symbol(arabic_code)))
	{
	    *len += gdi_get_charwidth(arabic_code);
	    offset += (str - pstr);
	    break;
	}
	else if(('\n' == arabic_code) ||
		('\0' == arabic_code))
	{
	    offset += (str - pstr);
	    offset -= tail;
	    break;
	}

	max_width = 0;//gdi_get_charwidth(arabic_code);

	arabic_code = GxGetPresentationForm(tmp_uc, arabic_code, prev_code, next_code, &flag);
	max_width = gdi_get_charwidth(arabic_code);
	if(flag) {
		str = tmp_str;
	}

	if(width)
	{
	    if(((*len) + max_width) > width)
	    {
		break;
	    }
	}

	*len += max_width;

	offset += (str - pstr);
    }

    return (offset);
}

static int _font_fix_arabic_keyinfo(void)
{
#ifndef NO_OS
	if(gui.config.arabic_vary_mode == ARABIC_UIGUR_MODE) {
		_aKeyInfo[34].Initial = 0xFBE8;
		_aKeyInfo[34].Medial  = 0xFBE9;
	} else {
		_aKeyInfo[34].Initial = 0;
		_aKeyInfo[34].Medial  = 0;
	}
#endif /*NO_OS*/
	return 0;
}

void gdi_arabic_multilines(char *src_string, char *dst_string, int width)
{
	char *src_ptr = NULL, *dst_ptr = NULL;
	unsigned char is_iso6937 = 0, *tmp_ptr = NULL;
	char *tmp_word = NULL;
	unsigned int arabic_code = 0, accent_code = 0;
	int line_width = 0, word_len = 0, word_offset = 0;

	if((NULL == src_string) && (NULL == dst_string))
	{
		return;
	}

	src_ptr = src_string;
	dst_ptr = dst_string;

	while(*src_ptr)
	{
	    word_offset = _calc_word_len((unsigned char *)src_ptr, &word_len, 0, 0);
		tmp_word = GUI_MALLOCZ(word_offset + 10);
		memcpy(tmp_word, src_ptr, word_offset);
//		word_len = gdi_text_len(tmp_word);
		GUI_FREE(tmp_word);

	    if((line_width + word_len) > width)
	    {
			if(0 == line_width)
			{
				word_offset = _calc_word_len((unsigned char *)src_ptr, &word_len, 0, 0);
				tmp_word = GUI_MALLOCZ(word_offset + 10);
				memcpy(tmp_word, src_ptr, word_offset);
				word_len = gdi_text_len(tmp_word);
				GUI_FREE(tmp_word);
				memcpy(dst_ptr, src_ptr, word_offset);
				dst_ptr += word_offset;
				src_ptr += word_offset;
			}
			*dst_ptr = '\n';
			dst_ptr++;
			line_width = 0;
			word_len = 0;
	    }
	    else
	    {
		    memcpy(dst_ptr, src_ptr, word_offset);
		    dst_ptr += word_offset;
		    src_ptr += word_offset;
		    line_width += word_len;
	    }
	    tmp_ptr = (unsigned char *)(src_ptr - 1);
	    gdi_font_utf82unicode(&tmp_ptr, &arabic_code, &accent_code, &is_iso6937);

	    if(('\n' == arabic_code) && (*(dst_ptr - word_offset - 1) != '\n'))
	    {
			src_ptr = (char *)tmp_ptr;
			*dst_ptr = '\n';
			dst_ptr++;
			line_width = 0;
			continue;
	    }
	    else if('\0' == arabic_code)
	    {
		break;
	    }

	    word_offset = word_len = 0;
	}
}

static int GxIsArabicCharacter(unsigned int Char)
{
	int ret = 0;

	ret = ((Char >= 0x0600) && (Char <= 0x06ff)) ? 1 : 0;
	ret |= ((Char >= 0x0750) && (Char <= 0x07ff)) ? 1 : 0;
	ret |= ((Char >= 0xFB50) && (Char <= 0x0FBf)) ? 1 : 0;
	ret |= ((Char >= 0xFE70) && (Char <= 0xFEFF)) ? 1 : 0;

	return (ret);
}

static int GxIsDigit(unsigned int Char)
{
	return ((Char >= 0x0660) && (Char <= 0x066f)) ? 1 : 0;
}

static int GxIsRightToLeft(unsigned int unicode)
{
	if ((0x0600 <= unicode) && (unicode <= 0x065F))
		return GUI_TRUE;
	else if ((0x0670 <= unicode) && (unicode <= 0x06FF))
		return GUI_TRUE;
	else if ((0x0590 < unicode) && (unicode < 0x05FF))
		return GUI_TRUE;
	else if ((0x0780 < unicode) && (unicode < 0x07BF))
		return GUI_TRUE;
	else
		return GUI_FALSE;
}

static int _check_arabic_wstring(unsigned int *pwstr)
{
	while(*pwstr) {
		if(GxIsRightToLeft(*pwstr)) {
			return (GUI_TRUE);
		}
		pwstr++;
	}
	return (GUI_FALSE);
}

typedef enum {
	LETTER_UNKNOWN,
	LETTER_ALPHA_DIGIT,
	LETTER_ARABIC,
	LETTER_TERMINAL,
} LetterType;

static LetterType _check_letter_type(unsigned int c) {
	if((c >= '0') && (c <= '9')) {
		return (LETTER_ALPHA_DIGIT);
	} else if((c >= 'A') && (c <= 'Z')) {
		return (LETTER_ALPHA_DIGIT);
	} else if((c >= 'a') && (c <= 'z')) {
		return (LETTER_ALPHA_DIGIT);
	} else if((c >= 0x0800) && (c <= 0x058F)) {
		return (LETTER_ALPHA_DIGIT);
	} else if(GxIsArabicCharacter(c)) {
		return (LETTER_ARABIC);
	} else if(0 == c) {
		return (LETTER_TERMINAL);
	} else {
		return (LETTER_UNKNOWN);
	}
}

static unsigned int _symmetrical_symbol_map(unsigned int c)
{
	unsigned int ret = 0;

	switch(c) {
		case '(':
			ret = ')';
			break;
		case ')':
			ret = '(';
			break;
		case '[':
			ret = ']';
			break;
		case ']':
			ret = '[';
			break;
		case '<':
			ret = '>';
			break;
		case '>':
			ret = '<';
			break;
		case '{':
			ret = '}';
			break;
		case '}':
			ret = '{';
			break;
		case '"':
			ret = '"';
			break;
		case 0xab:
			ret = 0xbb;
			break;
		case 0xbb:
			ret = 0xab;
			break;
		default:
			break;
	}

	return (ret);
}

#ifndef NO_OS
static int _terminal_symbol(unsigned int src, unsigned int *dst, int dst_size)
{
	int i = 0;

	for(i = 0; i < dst_size; i++) {
		if(src == dst[i]) {
			return (GUI_FALSE);
		}
	}
	return (GUI_TRUE);
}
#endif /*NO_OS*/

static int _check_blank_size(unsigned int *pustr, int size)
{
#ifndef NO_OS
	unsigned int terminal_char[10]  = {0};
	int len = size, terminal_size = 0;
	int fix_size = size;
	GuiCore *gui_core = GUI_GetCurrent();

	memset(terminal_char, 0, 10 * sizeof(unsigned int));
	terminal_char[0] = '.';
	terminal_char[1] = '!';
	terminal_char[2] = '"';
	terminal_char[3] = ',';
	terminal_char[4] = ':';
	if(ARABIC_PUNCTUATION_MODE1 == gui_core->config.arabic_punctuation_mode) {
		terminal_size = 5;
	} else {
		terminal_size = 4;
	}
	while(len) {
		if(!GxIsArabicCharacter(*pustr) &&
		   (_terminal_symbol(*pustr, terminal_char, terminal_size)) &&
		   (!_symmetrical_symbol_map(*pustr))) {
			size--;
		} else if(!GxIsArabicCharacter(*pustr) &&
			  ('.' == *pustr || '"' == *pustr)) {
			if(((*(pustr - 1) && *(pustr + 1)) &&
			   (_check_letter_type(*(pustr - 1)) == _check_letter_type(*(pustr + 1)))) ||
			   ('"' == *pustr)) {
				len--;
				pustr--;
				size--;
				continue;
			} else if(*(pustr + 1) == 0){
				return (fix_size);
			}
		} else if((*pustr != '\n') &&
			  (*pustr != '\r')) {
			break;
		}
		len--;
		pustr--;
	}

	if(size < 0) {
		size = 0;
	}
#endif /*NO_OS*/

	return (size);
}

static int _arabic_string_line_len(unsigned int *unicode)
{
	unsigned int *pustr = unicode;
	int size = 0;

	while(*pustr) {
		if((*pustr == '\n') ||
		   (*pustr == '\r'))
			break;
		pustr++;
	}

	size = pustr - unicode;
	pustr--;

	size = _check_blank_size(pustr, size);

	return (size);
}

static int _is_left_symmetrical_symbol(unsigned int unicode)
{
	if(('(' == unicode) ||
	   ('[' == unicode) ||
	   ('<' == unicode) ||
	   ('{' == unicode) ||
	   ('"' == unicode)) {
		return (GUI_TRUE);
	} else {
		return (GUI_FALSE);
	}
}

static int _is_right_symmetrical_symbol(unsigned int unicode)
{
	if((')' == unicode) ||
	   (']' == unicode) ||
	   ('>' == unicode) ||
	   ('}' == unicode) ||
	   ('"' == unicode)) {
		return (GUI_TRUE);
	} else {
		return (GUI_FALSE);
	}
}

static int _arabic_string_offset(unsigned int *unicode, unsigned int *offset, int arabic_flag)
{
	unsigned int *pustr = unicode;
	unsigned int flag = 0;
	bool has_left_symmetric = false;

	while(*pustr) {
		if((arabic_flag) && (!GxIsRightToLeft(*pustr)) && (flag))
			break;
		else if((arabic_flag) && (GxIsRightToLeft(*pustr)) && (!flag)) {
			*offset = (pustr - unicode);
			flag = GUI_TRUE;
		}
		else if((!arabic_flag) &&
			(GxIsRightToLeft(*pustr) ||
			((' ' == *pustr) && (GxIsRightToLeft(*(pustr + 1)))) ||
			('.' == *pustr)) &&
			(flag)) {
				if((GxIsRightToLeft(*(pustr -1)) != GxIsRightToLeft(*(pustr))) &&
				    flag)
					break;
				pustr++;
				if(_check_letter_type(*(pustr - 1)) == _check_letter_type(*(pustr + 1))) {
					continue;
				} else {
					if(_check_letter_type(*pustr) != _check_letter_type(*(pustr + 1))) {
						if(LETTER_TERMINAL == _check_letter_type(*(pustr + 1))) {
							if(' ' == *(pustr - 1))
								pustr--;
							break;
						} else
							pustr++;
					} else {
						continue;
					}
				}
			}
		else if((!arabic_flag) &&
			(!GxIsRightToLeft(*pustr) && (' ' != *pustr) &&
			 ('.' != *pustr)) && ('!' != *pustr) && (',' != *pustr) &&
			 (!_is_left_symmetrical_symbol(*pustr)) &&
			 (!_is_right_symmetrical_symbol(*pustr)) &&
			(!flag)) {
				*offset = (pustr - unicode);
				flag = GUI_TRUE;
		}
		if((!has_left_symmetric) && _is_left_symmetrical_symbol(*pustr))
			has_left_symmetric = true;
		pustr++;
		if(has_left_symmetric && _is_right_symmetrical_symbol(*pustr)) {
			break;
		}
	}

	if(flag) {
		if(_is_left_symmetrical_symbol(*pustr)) {
			pustr--;
		} else if(_is_right_symmetrical_symbol(*pustr) && !has_left_symmetric) {
			pustr++;
		}
		return (pustr - unicode - (*offset));
	} else
		return (0);
}

static int _has_arabic_alpha(unsigned int *unicode, unsigned int size)
{
	int i = 0;

	for(i = 0; i < size; i++) {
		if(GxIsArabicCharacter(unicode[i]))
			return (GUI_TRUE);
	}
	return (GUI_FALSE);
}

static void _arabic_revert(unsigned int *unicode, unsigned int size)
{
	unsigned int *pustr = unicode, tmp_code = 0;
	unsigned int i = 0;

	if(!_has_arabic_alpha(unicode, size))
		return;

	while((*pustr) && (i < (size / 2))) {
		tmp_code = pustr[i];
		if(_symmetrical_symbol_map(tmp_code)) {
			tmp_code = _symmetrical_symbol_map(tmp_code);
		}
		pustr[i] = pustr[size - i - 1];
		if(_symmetrical_symbol_map(pustr[i])) {
			pustr[i] = _symmetrical_symbol_map(pustr[i]);
		}
		pustr[size - i - 1] = tmp_code;
		i++;
	}
	if(size % 2) {
		if(_symmetrical_symbol_map(pustr[i]))
			pustr[i] = _symmetrical_symbol_map(pustr[i]);
	}
}

static void _revert_arabic_wstring(unsigned int *unicode, unsigned int size, int arabic_flag)
{
	unsigned int *pustr = unicode;
	unsigned int revert_size = 0, revert_offset = 0;

	while(pustr < (unicode + size)) {
		revert_size = _arabic_string_offset(pustr, &revert_offset, arabic_flag);
		if((pustr + revert_offset + revert_size) > (unicode + size))
			break;
		if(revert_size) {
			_arabic_revert(pustr + revert_offset, revert_size);
		} else if((0 == revert_size) && (0 == revert_offset)) {
			return;
		}
		pustr += (revert_offset + revert_size);
	}
}

static int _GetTableIndex(unsigned int Char)
{
	if (Char < 0x0621)
	{
		return 0;
	}
	if (Char > 0x06FF)
	{
		return 0;
	}
	if ((Char >= 0x0621) && (Char <= 0x063a))
	{
		return Char - 0x0621;
	}
	if ((Char >= 0x0641) && (Char <= 0x064a))
	{
		return Char - 0x0641 + 26;
	}
	if ((Char >= 0x0675) && (Char <= 0x0678))
	{
		return (Char - 0x0675 + 36);
	}
	if (Char == 0x067e)
	{
		return 40;
	}
	if (Char == 0x0686)
	{
		return 41;
	}
	if (Char == 0x0698)
	{
		return 42;
	}
	if (Char == 0x06a9)
	{
		return 43;
	}
	if (Char == 0x06ad)
	{
		return 44;
	}
	if (Char == 0x06af)
	{
		return 45;
	}
	if (Char == 0x06be)
	{
		return 46;
	}
	if ((Char >= 0x06C5) && (Char <= 0x06C9))
	{
		return (Char - 0x06C5 + 47);
	}
	if (Char == 0x06cb)
	{
		return 52;
	}
	if (Char == 0x06cc)
	{
		return 53;
	}
	if (Char == 0x06d0)
	{
		return 54;
	}
	if (Char == 0x06d5)
	{
		return 55;
	}
	return 0;
}

static int _IsTransparent(unsigned int Char)
{
	if ((Char >= 0x064b) && (Char <= 0x065F))
	{
		return 1;
	} else if ((Char >= 0x066A) && (Char <= 0x0674)) {
		return 1;
	} else if((Char >= 0x06D6) && (Char <= 0x06FF)) {
		return 1;
	} else {
		return 0;
	}
}

static int _GetLigature(unsigned int Char, unsigned int Next, int PrevAffectsJoining)
{
	if (((Next != 0x0622) && (Next != 0x0623) && (Next != 0x0625) && (Next != 0x0627)) || (Char != 0x0644))
	{
		return 0;
	}
	if (PrevAffectsJoining)
	{
		switch (Next)
		{
			case 0x0622:
				return 0xfef6;
			case 0x0623:
				return 0xfef8;
			case 0x0625:
				return 0xfefa;
			case 0x0627:
				return 0xfefc;
		}
	}
	else
	{
		switch (Next)
		{
			case 0x0622:
				return 0xfef5;
			case 0x0623:
				return 0xfef7;
			case 0x0625:
				return 0xfef9;
			case 0x0627:
				return 0xfefb;
		}
	}
	return 0;
}

static unsigned int GxGetPresentationForm(unsigned int *p_uc, unsigned int Char, unsigned int Next, unsigned int Prev, int* pIgnoreNext)
{
	int CharIndex = 0, PrevIndex = 0, NextAffectsJoining = 0, PrevAffectsJoining = 0, Final = 0, Initial = 0, Medial = 0, Ligature = 0;
	int PrePrev = 0, PrePreIndex = 0, PrePreAffectiosJoining = 0;
	char *charname = NULL;
	struct gdi_font *pFont = NULL;

	pFont = gxgdi_get_font();
	if(NULL == pFont)
		return 0;

	CharIndex = _GetTableIndex(Char);

	if (!CharIndex)
	{
		if (Char == 0x0621)
		{
			return 0xfe80;
		}
		else
		{
			return Char;
		}
	}

	PrePrev = *(p_uc + 2);
	PrePreIndex = _GetTableIndex(PrePrev);
	PrePreAffectiosJoining = (_GetTableIndex(PrePrev) || _IsTransparent(PrePrev)) && (_aKeyInfo[PrePreIndex].Medial);

	PrevIndex = _GetTableIndex(Char);
#ifdef GUI_TTF
#ifndef NO_OS
	if(0 == PrevIndex && pFont->gsub_opt) {
		charname = pFont->gsub_opt->get_tans(p_uc + 1, *p_uc, (unsigned int *)&Ligature, GSUB_SINLE_MODE);
		if(charname)
			PrevAffectsJoining = GUI_TRUE;
		else
			PrevAffectsJoining = GUI_FALSE;
	} else
#endif /*NO_OS*/
		PrevAffectsJoining = (_GetTableIndex(Char) || _IsTransparent(Char)) && (_aKeyInfo[CharIndex].Medial);
#endif /*GUI_TTF*/

	Ligature = _GetLigature(Prev, Char, PrevAffectsJoining | PrePreAffectiosJoining);

	if (Ligature)
	{
		if (pIgnoreNext)
		{
			*pIgnoreNext = 1;
		}
		return Ligature;
	}
	else
	{
		if (pIgnoreNext)
		{
			*pIgnoreNext = 0;
		}
	}
	PrevIndex = _GetTableIndex(Prev);
	PrevAffectsJoining = (_GetTableIndex(Prev) || _IsTransparent(Prev)) && (_aKeyInfo[PrevIndex].Medial);
	if(_IsTransparent(Prev))
		PrevAffectsJoining = PrePreAffectiosJoining;

	NextAffectsJoining = (_GetTableIndex(Next)||_IsTransparent(Next)) && (_aKeyInfo[CharIndex].Medial);
	if(((Char > 0x06C0) || (0x0647 == Char)) && _IsTransparent(Next))
		NextAffectsJoining = 0;
	if ((!PrevAffectsJoining) && (!NextAffectsJoining))
	{
		return _aKeyInfo[CharIndex].Isolated;
	}
	else if ((!PrevAffectsJoining) && (NextAffectsJoining))
	{
		Initial = _aKeyInfo[CharIndex].Initial;
		if (Initial)
		{
			return Initial;
		}
		else
		{
			return _aKeyInfo[CharIndex].Isolated;
		}
	}
	else if ((PrevAffectsJoining) && (NextAffectsJoining))
	{
		Medial = _aKeyInfo[CharIndex].Medial;
		if (Medial)
		{
			return Medial;
		}
		else
		{
			return _aKeyInfo[CharIndex].Isolated;
		}
	}
	else if ((PrevAffectsJoining) && (!NextAffectsJoining))
	{
		Final = _aKeyInfo[CharIndex].Final;
		if (Final)
		{
			return Final;
		}
		else
		{
			return _aKeyInfo[CharIndex].Isolated;
		}
	}
	return Char;
}

static int _arabic_get_first_byte(unsigned char **pstr)
{
	unsigned char *str = NULL;
	int first_code = 0;

	if(NULL == pstr)
	{
		return (0);
	}

	str = *pstr;
	if(NULL == str)
	{
		return (0);
	}

	if((*str > 0) && (*str <= 0x5))
	{
		first_code = *str;
		first_code += 4;
		str++;
	}
	else if(0x10 == *str)
	{
		str++;
		first_code = ((*str) << 8) | (*(str + 1));
		str += 2;
	}
	else if(0x11 == *str)
	{
		first_code = *str;
		str++;
	}
	else if(0x1F == *str)
	{
		first_code = *str;
	}
	else
	{
		first_code = 0;
	}

	*pstr = str;

	return (first_code);

}
static int _is_accent(unsigned int unicode)
{
    if((0x0E48 <= unicode) && (unicode <= 0x0E4C))
	return (GUI_TRUE);
    else
	return (GUI_FALSE);
}

static int _is_up_vowel(unsigned int unicode)
{
    if((0x0E31 == unicode) ||
       ((0x0E34 <= unicode) && (unicode <= 0x0E37)) ||
       (0x0E47 == unicode) ||
       ((0x0E4C <= unicode) && (unicode <= 0x0E4E)))
	return (GUI_TRUE);
    else
	return (GUI_FALSE);
}

static int _is_vowel(unsigned int unicode)
{
    if(((0x0E33 < unicode) && (unicode < 0x0E3F)) ||
	(0x0E31 == unicode) ||
	((0x0E46 < unicode) && (unicode < 0x0E4F)) ||
	((unicode >= 0x0300) && (unicode <= 0x036F)) ||
	(0x102B == unicode) ||
	((0x102C < unicode) && (0x1031 > unicode)) ||
	((0x1031 < unicode) && (0x1038 > unicode)) ||
	((0x1038 < unicode) && (0x103F > unicode)) ||
	((0x1057 < unicode) && (0x105A > unicode)) ||
	((0x105D < unicode) && (0x1061 > unicode)) ||
	((0x1070 < unicode) && (0x1075 > unicode)) ||
	(0x1082 == unicode) ||
	((0x1084 < unicode) && (0x1087 > unicode)) ||
	(0x108D == unicode) ||
	((0x17B5 < unicode) && (0x17D4 > unicode)))
    {
	return (GUI_TRUE);
    }
    else
    {
	return (GUI_FALSE);
    }
}

static int _is_unicode_symbol(unsigned int c)
{
	if(_symmetrical_symbol_map(c) ||
	   (c == ','))
	{
		return (GUI_TRUE);
	}
	else
	{
		return (GUI_FALSE);
	}
}

static int _special_symbol(unsigned int unicode)
{
	if((unicode >= 0x2002) && (unicode <= 0x200F)) {
		return (GUI_TRUE);
	} else if((unicode >= 0x2028) && (unicode <= 0x202F)) {
		return (GUI_TRUE);
	} else if((unicode >= 0x205F) && (unicode <= 0x206F)) {
		return (GUI_TRUE);
	} else {
		return (GUI_FALSE);
	}
}

static unsigned int _double_word_width(unsigned int *pstr)
{
	unsigned int *ptr = NULL;
	unsigned int word = 0, prev_c = 0, next_c = 0, tmp_c = 0;
	unsigned int word_width = 0;
	int ignore_next = 0, xoff = 0;

	ptr = pstr;
	while(' ' != *ptr)
	{
		if('\n' == *ptr)
		{
			break;
		}
		else if('\0' == *ptr)
		{
			break;
		}
		else if(_is_cjk_symbol(*ptr))
		{
		    break;
		}
		else if((_special_symbol(*ptr))) {
			ptr++;
			continue;
		} else
		{
			word = *ptr;
			if(_is_vowel(word))
			{
				;
			}
			else
			{
				if(GxIsArabicCharacter(word)) {
					if(_IsTransparent(next_c))
						next_c = 0;
					else
						next_c = *(ptr + 1);
					tmp_c = GxGetPresentationForm((unsigned int *)ptr,
								      word,
								      prev_c,
								      next_c,
								      &ignore_next);
					if(!_IsTransparent(word))
						prev_c = word;
					word = tmp_c;
				}
				if(!_IsTransparent(word))
					word_width += gdi_get_charwidth(word);
#ifdef GUI_TTF
				xoff = ttf_get_xoff(word);
				if(xoff < 0) {
					word_width += xoff;
				}
#endif
			}
		}
		ptr++;
	}

	return (word_width);
}

static bool _is_control_character(unsigned int code) {
	if((code >= 0x200B) && (code <= 0x200D)) {
		return (GUI_TRUE);
	} else {
		return (GUI_FALSE);
	}
}

#define CHECK_UNICODE(uc)	    if((*uc == 0xFF) && \
				       (*(uc + 1) == 0xFF) && \
				       (*(uc + 2) == 0xFF) && \
				       (*(uc + 3) == 0xFF))  \
				    {	    \
					GUI_FREE(unicode_flag);	\
					goto DISPLAY;	\
				    }

static int font_utf8_len(void *string)
{
	unsigned char *pstr = NULL, *puc = NULL, *ptr = NULL;
	unsigned int len = 0;
	unsigned char iso6937_flag = 0;
	unsigned int unicode = 0, isarabic = 0, *p_dchar = NULL, accent = 0;
	int ignore_next = 0, i = 0;
	unsigned int prev_char = 0, cur_char = 0, next_char = 0;
	int pixlen = 0, iso6937_str = 0, class_code = 0, is_arabic = 0;
	struct gdi_font *pFont;

	pstr = (unsigned char *)string;
	if(NULL == pstr)
	{
		return (0);
	}
	is_arabic = gxgdi_is_right2left(pstr);
	if(is_arabic)
	{
		//return (_arabic_get_pixel_len(pstr));
	}

	if(((*pstr) & 0xE0) == UTF8_110XXXXX)
	{
		unicode = (((*pstr & 0x1F) << 6) |
				(*(pstr + 1) & 0x3F));
	}

	_font_fix_arabic_keyinfo();

	if(0x2 == *pstr)
	{
		/*ISO 8859-6*/
		pstr++;

		len = strlen((char *)pstr);
		while(*pstr)
		{
			unicode = _get_arabic_unicode(*pstr);
			pixlen += gdi_get_charwidth(unicode);

			pstr++;
		}
	}
	else if(0x11 == *pstr)
	{
		/*ISO/IEC 10646-1*/
		pstr++;

		do
		{
			unicode = ((*pstr) << 8) | *(pstr + 1);
			pixlen += gdi_get_charwidth(unicode);

			pstr += 2;
		}while(unicode);
	}
	else if(is_arabic)
	//else if((unicode >= 0x0600) && (unicode <= 0x06FF))
	{
		len = strlen((char *)pstr);

		ptr = puc = (unsigned char *)GUI_MALLOCZ((len + 1) * 4 + 400);
		memset(ptr, 0, (len + 1) * 4);

		while(*pstr)
		{
			gdi_font_utf82unicode(&pstr, &unicode, &accent, &iso6937_flag);
			if(_special_symbol(unicode))
				continue;

			*ptr = unicode & 0x000000FF;
			ptr++;
			i++;
			*ptr = (unicode >> 8) & 0x000000FF;
			ptr++;
			i++;
			*ptr = (unicode >> 16) & 0x000000FF;
			i++;
			ptr++;
			*ptr = (unicode >> 24) & 0x000000FF;
			i++;
			ptr++;

			//pixlen += gdi_get_charwidth(unicode);
		}

		int arabic_len = 0, total_len = 0;
		unsigned int *p_wunicode = (unsigned int *)puc;
		while((void *)p_wunicode < (void *)(puc + len)) {
			arabic_len = _arabic_string_line_len(p_wunicode);
			if(_check_arabic_wstring(p_wunicode)) {
				_arabic_revert(p_wunicode, arabic_len);
				_revert_arabic_wstring(p_wunicode, arabic_len, GUI_FALSE);
			}
			p_wunicode += (arabic_len + 1);
			total_len += (arabic_len + 1);
		}
		p_dchar = (unsigned int * )puc;

		while(*p_dchar)
		{
			cur_char = *p_dchar;

			if('\n' == cur_char)
				break;

			if(*(p_dchar + 1))
			{
				next_char = *(p_dchar + 1);
			}
			else
			{
				next_char = 0;
			}

			if((cur_char >= 0x0600) && (cur_char <= 0x06FF))
			{
				isarabic = GxIsArabicCharacter(cur_char);
				if(isarabic)
				{
					unicode = GxGetPresentationForm(p_dchar, cur_char,
							prev_char,
							next_char,
							&ignore_next);
				}
			}
			else
			{
				unicode = cur_char;
			}

			if((ignore_next) && ((unicode >= 0xfef5) && (unicode <= 0xfefc)))
			{
				p_dchar += 2;
			}
			else
			{
				p_dchar++;
			}

			if(!_is_control_character(unicode)) {
				if(pixlen && _IsTransparent(unicode))
					pixlen += 0;
				else
					pixlen += gdi_get_charwidth(unicode);
			}
#ifdef GUI_TTF
			if((ttf_get_xoff(unicode) < 0)/* && (!GxIsArabicCharacter(unicode))*/)
			{
				pixlen += ttf_get_xoff(unicode);
			}
#endif
			if(!_is_control_character(unicode)) {
				prev_char = cur_char;
			} else {
				prev_char = 0;
			}
		}

		GUI_FREE(puc);
	}
	else
	{
		class_code = _arabic_get_first_byte(&pstr);
		len = strlen((char *)pstr);
		if(0x1F == class_code)
		{
			iso6937_str = GUI_TRUE;//_check_is_iso6937((char *)pstr);
		}
		while(*pstr)
		{
			gdi_font_utf82unicode(&pstr, &unicode, &accent, &iso6937_flag);

			if('\n' == unicode)
			{
				break;
			}

			if(iso6937_str && iso6937_flag)
			{
				if(_is_vowel(unicode))
				{
					;
				}
				else if((0xA0 < unicode) && (unicode < 0xFF))
				{
#ifndef NO_OS
#ifdef GUI_STR_ICONV
					if(iso6937_char_set[unicode - 0x00A0])
					{
						unicode = iso6937_char_set[unicode - 0x00A0];
					}
					else
					{
						pixlen += gdi_get_charwidth(unicode);
					}
#endif /*GUI_STR_ICONV*/
#endif /*NO_OS*/
					//pixlen += gdi_get_charwidth(unicode);
				}
				else
				{
					pixlen += gdi_get_charwidth(unicode);
				}
			}
			else
			{
				if(_is_vowel(unicode))
				{
#ifdef GUI_TTF
					pixlen += gdi_get_charwidth(unicode) + ttf_get_xoff(unicode);
#endif /*GUI_TTF*/
				}
				else
				{
					pixlen += gdi_get_charwidth_with_accent(unicode, accent);
				}
			}

		}
	}

	pFont = gxgdi_get_font();
	return (pixlen);
}

static int _arabic_check_line_full(const unsigned char *string, int width, unsigned short horizontal, int is_arabic)
{
	int text_width = 0, ignore_next = 0;
	char *pstr = NULL;
	unsigned int wc = 0, accent = 0, prev_c = 0, next_c = 0, map_c = 0;
	struct gdi_font *pFont;

	if(NULL == string)
	{
		return (0);
	}

	pstr = (char *)string;
	while(*(unsigned int*)pstr)
	{
		if(!_IsTransparent(wc))
			prev_c = wc;
		wc = ((*(pstr + 3)) << 24) | ((*(pstr+ 2)) << 16) | ((*(pstr + 1)) << 8) | (*pstr);
		next_c = accent = ((*(pstr + 7)) << 24) | ((*(pstr + 6)) << 16) | ((*(pstr + 5)) << 8) | (*(pstr + 4));
		if(GxIsArabicCharacter(wc))
		{
			map_c = GxGetPresentationForm((unsigned int *)pstr,
										wc,
										prev_c,
										next_c,
										&ignore_next);
		}
		else
		{
			map_c = wc;
		}

		if(!_is_vowel(map_c) && !_IsTransparent(map_c))
		{
//			printf("0x%x\t0x%x\t", wc, map_c);
			text_width += (gdi_get_charwidth(map_c) + horizontal);
//			printf("%d\t", gdi_get_charwidth(map_c));
#ifdef GUI_TTF
			if(ttf_get_xoff(map_c) < 0)
			{
//				printf("%d ", text_width);
//				printf("- %d = ", -ttf_get_xoff(map_c));
				text_width += ttf_get_xoff(map_c);
			}
//			printf("%d\n", text_width);
#endif
		} else if(_IsTransparent(map_c)) {
#ifdef GUI_TTF
			if(ttf_get_xoff(map_c) < 0) {
				text_width += ttf_get_xoff(map_c);
			}
#endif
		}
		if(_is_vowel(accent) ||
		   ((ignore_next) && ((map_c >= 0xfef5) && (map_c <= 0xfefc))))
		{
			pstr += 8;
		}
		else
		{
			pstr += 4;
		}
		if(text_width >= width)
		{
			break;
		}
		else if((' ' == map_c) || _is_cjk_symbol(map_c))
		{
			if((' ' != *pstr) || (_is_cjk_symbol(*pstr)))
			{
				if((text_width + _double_word_width((unsigned int *)pstr)) > width)
				{
					break;
				}
			}
		}
		else if('\n' == map_c)
		{
			break;
		}
		else if('\0' == map_c)
		{
			break;
		}
	}

	pFont = gxgdi_get_font();
	if(is_arabic)
	{
		text_width += pFont->xsize;
	}
	if(text_width >= width)
	{
		return (0);
	}

	return (width - text_width);
}

#ifdef GUI_TTF
static char *_get_char_name(TTF_Font *font, FT_ULong  charcode)
{
	struct gdi_font *pFont;
	FT_UInt index = 0;
	static char char_name[100] = {0};

	pFont = gxgdi_get_font();

	memset(char_name, 0, 100);
	index = FT_Get_Char_Index(font->face, charcode);
	if(pFont &&
	   pFont->info &&
	   pFont->info->glyph_info &&
	   pFont->info->glyph_info->glyph &&
	   index < pFont->info->glyph_info->count) {
		return (pFont->info->glyph_info->glyph[index].name);
	} else {
		FT_Get_Glyph_Name(font->face, index, char_name, 100);
	}

	return (char_name);
}

static int _compare_lig(char **src, char **dst, unsigned int count)
{
	unsigned int i = 0;

	for(i = 0; i < count; i++)
	{
		if((src[i]) && (dst[i]))
		{
			if(0 != strcasecmp(src[i], dst[i]))
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	if(i == count)
		return (GUI_TRUE);
	else
		return (GUI_FALSE);
}

char *_get_transformation_lig(unsigned int *uni_array,
			      unsigned int prev_char,
			      unsigned int *offset,
			      Transform_Mode mode)
{
	struct gdi_font *pFont;
	TTF_Font *font_data = NULL;
#ifndef NO_OS
	GSUB_Info *info = NULL;
	LigatureSub table;
#endif /*NO_OS*/
	char tag[10] = {0};
	char *ret_name = NULL;
#ifndef NO_OS
	char *uni_name[10] = {NULL};
	int i = 0, j = 0, index = -1, lig_total = 0;
#endif /*NO_OS*/

	pFont = gxgdi_get_font();
	if (NULL == pFont)
	{
		return (NULL);
	}

#ifndef NO_OS
	info = pFont->info;
	if(NULL == info)
	{
		return (NULL);
	}
	memset(&table, 0, sizeof(LigatureSub));
#endif /*NO_OS*/
	font_data = (TTF_Font *)(pFont->font_data);

	if(GSUB_SINLE_MODE == mode) {
#ifndef NO_OS
		info = pFont->info;
		if(NULL == info)
			return (NULL);
		if(prev_char) {
			if((*(uni_array + 1))) {
				//medi
				if(info->medi_table)
					ret_name = hash_get(info->medi_table, _get_char_name(font_data, uni_array[0]));
			} else {
				//fina
				if(info->fina_table)
					ret_name = hash_get(info->fina_table, _get_char_name(font_data, uni_array[0]));
			}
		} else {
			if((*(uni_array + 1))) {
				//init
				if(info->init_table)
					ret_name = hash_get(info->init_table, _get_char_name(font_data, uni_array[0]));
			} else {
				//isol
				if(info->isol_table)
					ret_name = hash_get(info->isol_table, _get_char_name(font_data, uni_array[0]));
			}
		}
#endif /*endef NO_OS*/

		return (ret_name);
	}

	if(prev_char) {
		if((*(uni_array + 1))) {
			strcpy(tag, "medi");
		} else {
			strcpy(tag, "fina");
		}
	} else {
		strcpy(tag, "liga");
	}

#ifndef NO_OS
	for(i = 0; i < info->count; i++)
	{
		table = info->table[i];
		for(j = 0; j < table.src_count; j++)
		{
			uni_name[j] = GxCore_Strdup(_get_char_name(font_data, uni_array[j]));
		}

		if((_compare_lig(uni_name, table.src_name, table.src_count)) &&
		   (0 == strcasecmp(tag, table.tag)) &&
		   (table.src_count > lig_total))
		{
			index = i;
			lig_total = table.src_count;
		}
		for(j = 0; j < table.src_count; j++)
		{
			GxCore_Free(uni_name[j]);
		}
	}

	if((index != -1) && (index < info->count))
	{
		ret_name = info->table[index].dst_name;
		*offset = info->table[index].src_count;
	}
#endif /*NO_OS*/

	return (ret_name);
}
#endif /*GUI_TTF*/

int _is_mark(unsigned int charcode)
{
#ifdef GUI_TTF
	FT_UInt index;
	struct gdi_font *pFont;
#ifndef NO_OS
	GSUB_Info *info = NULL;
#endif /*NO_OS*/
	GlyphInfo *glyph_info = NULL;

	pFont = gxgdi_get_font();
	if (NULL == pFont)
	{
		return (GUI_FALSE);
	}

#ifndef NO_OS
	info = pFont->info;
	if(NULL == info)
	{
		return (GUI_FALSE);
	}
	glyph_info = info->glyph_info;
#endif /*NO_OS*/
	if(NULL == glyph_info)
	{
		return (GUI_FALSE);
	}

	index = FT_Get_Char_Index(((TTF_Font *)(pFont->font_data))->face, charcode);
	if(glyph_info->glyph[index].type == GLYPH_MARK)
	{
		return (GUI_TRUE);
	}
	else
	{
		return (GUI_FALSE);
	}
#else
	return (GUI_FALSE);
#endif /*GUI_TTF*/
}

int _is_mark_byname(char* name)
{
#ifdef GUI_TTF
	FT_UInt index;
	struct gdi_font *pFont;
#ifndef NO_OS
	GSUB_Info *info = NULL;
#endif /*NO_OS*/
	GlyphInfo *glyph_info = NULL;

	pFont = gxgdi_get_font();
	if (NULL == pFont)
	{
		return (GUI_FALSE);
	}

#ifndef NO_OS
	info = pFont->info;
	if(NULL == info)
	{
		return (GUI_FALSE);
	}
	glyph_info = info->glyph_info;
	if(NULL == glyph_info)
	{
		return (GUI_FALSE);
	}
#endif /*NO_OS*/

	index = FT_Get_Name_Index(((TTF_Font *)(pFont->font_data))->face, name);
	if((glyph_info) && (glyph_info->glyph[index].type == GLYPH_MARK))
	{
		return (GUI_TRUE);
	}
	else
	{
		return (GUI_FALSE);
	}
#else
	return (GUI_FALSE);
#endif /*GUI_TTF*/
}

static int _is_arabic_tashkil(unsigned int c)
{
	if((c >= 0x064B) && (c <= 0x065F)) {
		return (GUI_TRUE);
	} else {
		return (GUI_FALSE);
	}
}

static int _arabic_display_presentation(void *surface,
										const unsigned int *unicode,
										GAL_Rect *rect,
										int interval,
										int size,
										int color,
										int alignment,
										int flag,
										int isarabic)
{
	int ret = 0, ignore_next = 0;
	unsigned char *p_line = NULL;
	unsigned int *p_uc = NULL;
	unsigned int prev_char = 0, cur_char = 0, next_char = 0;
	unsigned int map_value = 0, accent = 0, iso6937_flag = 0, prev_map = 0;
	int xPos = 0, yPos = 0, xoff = 0, xdelta = 0, mark_pos = 0, has_trans = GUI_FALSE;
	int text_off = 0, is_mark = GUI_FALSE;
	unsigned int lig_offset = 0;
	struct gdi_font *pFont;
	char *charname = NULL;
	//unsigned int *penglish_char = NULL;

	if(NULL == unicode)
	{
		return (-1);
	}

	pFont = gxgdi_get_font();
	if (NULL == pFont)
	{
		return (-1);
	}

	p_uc = (unsigned int *)unicode;

	if(PARAGRAPH_MODE == flag)
	{
		if(GUI_TA_HCENTRE & alignment)
		{
			text_off = _arabic_check_line_full((const unsigned char *)p_uc, rect->w, 0, isarabic);
			xdelta = text_off / 2;
			xPos = xdelta + rect->x;
		}
		else if(GUI_TA_RIGHT & alignment)
		{
			text_off = _arabic_check_line_full((const unsigned char *)p_uc, rect->w, 0, isarabic);
//			printf("\n\n\n\n\n");
			xdelta = text_off;
			xPos = xdelta + rect->x;
		}
		else
		{
			xdelta = 0;
			xPos = rect->x;
		}

		yPos = rect->y;
	}
	else
	{
		xPos = rect->x;
		yPos = rect->y;
		xdelta = 0;
	}

	if(0xFEFF == *p_uc)
	{
		p_uc++;
	}

	while(*p_uc)
	{
		cur_char = *p_uc;
		if(_is_control_character(cur_char) || (_special_symbol(cur_char))) {
			prev_char = 0;
			p_uc++;
			continue;
		}

		if(*(p_uc + 1))
		{
			next_char = *(p_uc + 1);
		}
		else
		{
			next_char = 0;
		}

		if((cur_char >= 0x0600) && (cur_char <= 0x06FF))
		{
			if(ignore_next)
			{
				ignore_next = 0;
			}

			if((isarabic) || (GxIsDigit(cur_char)))
			{
				map_value = GxGetPresentationForm(p_uc, cur_char,
						prev_char,
						next_char,
						&ignore_next);
			}
			else
			{
				map_value = 0xFE7C;
			}

#ifdef GUI_TTF
#ifndef NO_OS
			charname = NULL;
			if((map_value >= 0x0600) && (map_value <= 0x06FF) &&
			   (pFont->gsub_opt)) {
				charname = pFont->gsub_opt->get_tans(p_uc, prev_char, &lig_offset, GSUB_SINLE_MODE);
			}

			if(charname) {
				if(pFont->gsub_opt) {
					is_mark = pFont->gsub_opt->is_mark_name(charname);
					has_trans = GUI_FALSE;
				}
			}
#endif /*ndef NO_OS*/
#endif /*ndef GUI_TTF*/

			if((cur_char != map_value) || (GxIsDigit(cur_char)))
			{
				prev_char = cur_char;
			}

			if(ignore_next)
			{
				p_uc += 2;
			}
			else
			{
				p_uc++;
			}
		}
		else
		{
			prev_char = cur_char;

			if((cur_char >= 0x0300) && (cur_char <= 0x036F))
			{
				accent = cur_char;
				cur_char = *(p_uc + 1);

				p_uc++;
				iso6937_flag = 1;
			}

			map_value = cur_char;
#ifdef GUI_TTF
#ifndef NO_OS
			if(pFont->gsub_opt) {
				is_mark = pFont->gsub_opt->is_mark_code(map_value);
			}

			lig_offset = 0;
			if(pFont->gsub_opt) {
				charname = pFont->gsub_opt->get_tans(p_uc, 0, &lig_offset, GSUB_LIGAT_MODE);
			}
			if(charname)
			{
				if(pFont->gsub_opt) {
					is_mark = pFont->gsub_opt->is_mark_name(charname);
				}
				p_uc += lig_offset;
			}
			else
#endif /*NO_OS*/
#endif /*ndef GUI_TTF*/
				p_uc++;
		}

		if((map_value >= 0xfef5) && (map_value <= 0xfefc))
		{
			prev_char = next_char;
			ignore_next = 0;
		}
		else if(0 == map_value)
		{
			ret = ((unsigned int *)p_uc - (unsigned int *)unicode + 1);
			break;
		}

		// TODO:
		if(((xPos + xoff + gdi_get_charwidth(map_value)) > (rect->x + rect->w)) ||
				('\n' == map_value))
		{
			if(PARAGRAPH_MODE == flag)
			{
				yPos += (pFont->ysize + interval);
				if((xPos + xoff + gdi_get_charwidth(map_value)) > (rect->x + rect->w)) {
					p_line = (unsigned char *)p_uc - 4;
				} else {
					p_line = (unsigned char *)p_uc;
				}
				if(GUI_TA_HCENTRE & alignment)
				{
					text_off = _arabic_check_line_full((const unsigned char *)p_line, rect->w, 0, isarabic);
					xdelta = text_off / 2;
					xPos = xdelta + rect->x;

				}
				else if(GUI_TA_RIGHT & alignment)
				{
					text_off = _arabic_check_line_full((const unsigned char *)p_line, rect->w, 0, isarabic);
					xdelta = text_off;
					xPos = xdelta + rect->x;
				}
				else
				{
					xPos = rect->x;
					xdelta = 0;
				}

				if(yPos > (rect->y + rect->h - (pFont->ysize + interval)))
				{
					ret = ((unsigned int *)p_uc - (unsigned int *)unicode + 1);
					break;
				}
			}
			else
			{
				xdelta = 0;
				ret = ((unsigned int *)p_uc - (unsigned int *)unicode + 1);
				break;
			}

			xoff = 0;
		}

		if(0xFEFF != map_value)
		{
			if(iso6937_flag)
			{
				xPos += xoff;
				gdi_draw_char(surface, &xPos, &yPos, map_value, color);
				gdi_draw_char(surface, &xPos, &yPos, accent, color);
				prev_map = accent;
				xPos += (gdi_get_charwidth_with_accent(map_value, accent));
				xoff = 0;
			}
			else
			{
				if(_is_vowel(map_value))
				{
					xPos += xoff;
					xoff = gdi_get_charwidth(map_value);
					if(((0x107E <= map_value) && (map_value <= 0x1080)) ||
					   (0x1084 == map_value) ||
					   (0x103B == map_value))
						xoff = 2;
				}
				else if(_IsTransparent(prev_map)) {
					if(!_IsTransparent(map_value))
						xoff = gdi_get_charwidth(map_value);
					else
						xoff = 0;
				}
				else
				{
					if((isarabic) && (_is_arabic_tashkil(prev_map))) {
						;
					} else {
						xPos += xoff;
					}
					if(!_IsTransparent(map_value))
						xoff = gdi_get_charwidth(map_value);
					else
						xoff = 0;
					if(((0x107E <= map_value) && (map_value <= 0x1080)) ||
					   (0x1084 == map_value) ||
					   (0x103B == map_value))
						xoff = 2;
				}
				/*For Thailand language modified in Dec 31st, 2014 temporarily by shencz.*/
				if((_is_accent(map_value)) && (!_is_up_vowel(prev_map) && (0x0E33 != next_char)))
				{
				    int tmp_y = yPos;

				    tmp_y += (gdi_get_charheight() / 4 - 1);
				    gdi_draw_char(surface, &xPos, &tmp_y, map_value, color);
				}
				else
				{
#ifdef GUI_TTF
					if((xPos == rect->x) && (ttf_get_xoff(map_value) < 0))
					{
						xPos -= ttf_get_xoff(map_value);
					}
#endif
					if(charname)
					{
#ifdef GUI_TTF
						if((!is_mark) && (!has_trans))
						{
							//has_trans = GUI_TRUE;
							gdi_draw_sub_byname(surface, &xPos, &yPos, charname, color);
							mark_pos = xPos;
							xoff = gdi_get_charwidth_byname(charname);
						}
						else if(0 == strcasecmp("ivowelsigndeva_anusvaradeva", charname))
						{
							mark_pos -= (xoff / 4);
							gdi_draw_sub_byname(surface, &mark_pos, &yPos, charname, color);
							has_trans = GUI_FALSE;
							xoff = 0;
						}
						else
						{
							gdi_draw_sub_byname(surface, &xPos, &yPos, charname, color);
							mark_pos = xPos;
							if(is_mark)
								xoff = 0;
						}
#endif
					}
					else
					{
						if((!is_mark) && (!has_trans))
						{
							gdi_draw_char(surface, &xPos, &yPos, map_value, color);
							mark_pos = xPos;
						}
						else if(is_mark)
						{
							// TODO:
							if(0x093F == map_value)
							{
								mark_pos -= (xoff / 4);
								gdi_draw_mark(surface, mark_pos, yPos, map_value, color);
								xoff = 0;
							}
							else if((0x0947 == map_value) || (0x0948 == map_value))
							{
								gdi_draw_mark(surface, mark_pos, yPos, map_value, color);
								xoff = 0;
							}
							else
							{
								gdi_draw_char(surface, &xPos, &yPos, map_value, color);
							}
							has_trans = GUI_FALSE;
						}
						else
						{
							gdi_draw_char(surface, &xPos, &yPos, map_value, color);
							if(!has_trans)
								mark_pos = xPos;
						}

						if(next_char == 0x064E) {
							is_mark = GUI_TRUE;
							has_trans = GUI_TRUE;
						}
					}
				}
				prev_map = map_value;
				if((charname) && (!is_mark) && (!has_trans))
					map_value = cur_char = 0;

				if((' ' == map_value) || (_is_cjk_symbol(map_value)))
				{
					if((' ' != *p_uc) || (_is_cjk_symbol(*p_uc)))
					{
						if((xPos + xoff + _double_word_width(p_uc)) > (rect->x + rect->w))
						{
							if(PARAGRAPH_MODE == flag)
							{
								yPos += (pFont->ysize + interval);
								if(GUI_TA_HCENTRE & alignment)
								{
									text_off = _arabic_check_line_full((const unsigned char *)p_uc, rect->w, 0, isarabic);
									xdelta = text_off / 2;
									xPos = xdelta + rect->x;
								}
								else if(GUI_TA_RIGHT & alignment)
								{
									text_off = _arabic_check_line_full((const unsigned char *)p_uc, rect->w, 0, isarabic);
									xdelta = text_off;
									xPos = xdelta + rect->x;
								}
								else
								{
									xdelta = 0;
									xPos = rect->x;
								}
								if(yPos > (rect->y + rect->h - (pFont->ysize + interval)))
								{
									ret = ((unsigned int *)p_uc - (unsigned int *)unicode + 1);
									break;
								}
								xoff = 0;
							}
							else
							{
								//xdelta = 0;
								//break;
#ifdef GUI_TTF
								if((xPos == rect->x) && (ttf_get_xoff(map_value) < 0))
								{
									xPos -= ttf_get_xoff(map_value);
								}
#endif
								gdi_draw_char(surface, &xPos, &yPos, map_value, color);
								prev_map = map_value;
								xoff = gdi_get_charwidth(map_value);
								if((0x107E <= map_value) && (map_value <= 0x1080))
									xoff = 2;
								continue;
							}
						}
					}
				}
			}

			iso6937_flag = 0;
		}

	}

	ret = p_uc - unicode;
	return (ret);
}

#ifndef NO_OS
#ifdef GUI_ROLL_STR
static int _get_word_width(const unsigned char *string,
		int code,
		unsigned int width,
		unsigned int *word_len)
{
	unsigned int *iso_table = NULL;
	unsigned char *pstr = NULL;
	unsigned int unicode_c = 0;
	unsigned int c_width = 0, char_count = 0;

	if((NULL == string) || (NULL == word_len))
	{
		return (0);
	}

	pstr = (unsigned char *)string;

	iso_table = _get_iso_table(code);

	char_count = 0;

	while(*pstr)
	{
		unicode_c = *pstr;
		pstr++;
		char_count++;

		if(unicode_c > 0x7F)
		{
			if(iso_table)
			{
				unicode_c = iso_table[unicode_c - 0xA0];
			}
			else if(5 == code)
			{
				unicode_c = 0x400 + unicode_c - 0xA0;
			}
			else if(0x11 == code)
			{
				unicode_c = ((*pstr) << 8) | (*(pstr + 1));
				pstr++;
				char_count++;
			}
			else
			{
				;
			}
		}

		if((' ' == unicode_c) ||
		    ('\n' == unicode_c) ||
		    ('\0' == unicode_c) ||
		    _is_cjk_symbol(unicode_c))
		{
			if(0x11 == code)
			{
				pstr -= 2;
				char_count -= 2;
			}
			else
			{
				pstr--;
				char_count--;
			}
			break;
		}

		c_width += gdi_get_charwidth(unicode_c);
		if(width)
		{
			if(c_width > width)
			{
				c_width -= gdi_get_charwidth(unicode_c);
				if(0x11 == code)
				{
					pstr -= 2;
					char_count -= 2;
				}
				else
				{
					pstr--;
					char_count--;
				}
				break;
			}
		}
	}

	*word_len = c_width;

	return (char_count);
}

static int _getback_word_width(const unsigned char *string,
		const unsigned char *head,
		int code,
		unsigned int *word_len)
{
//	unsigned int *iso_table = NULL;
	unsigned char *pstr = NULL;
	unsigned int c_width = 0, char_count = 0;

	if((NULL == string) || (NULL == head) || (NULL == word_len))
	{
		return (0);
	}

	pstr = (unsigned char *)string;

	//iso_table = _get_iso_table(code);

	char_count = 0;

	while(pstr != head)
	{
		if(0x11 == code)
		{
			pstr -= 2;
		}
		else
		{
			pstr--;
		}

		if('\n' == *pstr)
		{
			pstr--;
			char_count--;
			if('\r' == *pstr)
			{
				pstr--;
				char_count--;
			}
			break;
		}
		else if((' ' == *pstr) || (_is_cjk_symbol(*pstr)))
		{
			break;
		}
		else
		{
			c_width += gdi_get_charwidth(*pstr);
			char_count--;
		}

		if(c_width > *word_len)
		{
			char_count++;
			c_width -= gdi_get_charwidth(*pstr);
			*word_len = c_width;
			return (char_count);
		}
	}

	*word_len = c_width;

	return (char_count);
}

static int _ascii_check_line_full(const unsigned char *string,
		int width,
		unsigned short horizontal,
		int code)
{
	int text_width = 0;
	unsigned int char_width = 0, offset = 0;
	char *pstr = NULL;
	unsigned int wc = 0;

	if(NULL == string)
	{
		return (0);
	}

	pstr = (char *)string;
	while(*pstr)
	{
		char_width = 0;
		offset = _get_word_width((const unsigned char *)pstr, code, width, &char_width);
		if(((char_width == 0) && (' ' == *pstr)) ||
		    ((char_width == 0) && ('\n' == *pstr)) ||
		    ((char_width == 0) && ('\r' == *pstr)) ||
		    ((0 == char_width) && _is_cjk_symbol(*pstr)))
		{;}
		else
		{
			pstr += offset;
		}

		text_width += (char_width + horizontal);

		if(text_width > width)
		{
			text_width -= (char_width + horizontal);
			break;
		}

		wc = *pstr;

		while((' ' == *pstr) || (_is_cjk_symbol(*pstr)))
		{
			text_width += gdi_get_charwidth(*pstr);

			if(text_width > width)
			{
				text_width -= gdi_get_charwidth(*pstr);
				pstr++;
				break;
			}

			pstr++;
		}

		if(text_width > width)
		{
			text_width -= (gdi_get_charwidth(*pstr) + horizontal);
			break;
		}

		if('\n' == wc)
		{
			break;
		}
		else if('\r' == wc)
		{
			break;
		}
		else if('\0' == wc)
		{
			break;
		}
	}

	return (width - text_width);

}
#endif /*GUI_ROLL_STR*/
#endif /*NO_OS*/

static int _get_position(unsigned char *pstr, int cursor)
{
    int i = 0;
    unsigned char *ptr = pstr, iso6937_flag = 0;
    unsigned int code = 0, accent = 0;

    for(i =0; i < cursor; i++)
    {
	gdi_font_utf82unicode(&ptr, &code, &accent, &iso6937_flag);
    }

    return (ptr - pstr);
}

static int _is_alpha_digit(unsigned int uc)
{
	if(((uc >= 'A') && (uc <= 'Z')) ||
	   ((uc >= 'a') && (uc <= 'z')) ||
	   ((uc >= '0') && (uc <= '9')) ||
	   _is_cjk_symbol(uc) ||
	   ((!_is_unicode_symbol(uc) && (!GxIsArabicCharacter(uc)) && ('\n' != uc)))) {
		return (GUI_TRUE);
	} else {
		return (GUI_FALSE);
	}
}

static int _alpha_digit_offset(unsigned int *p_uc)
{
	int offset = 0;
	while(*p_uc) {
		if(GUI_TRUE == _is_alpha_digit(*p_uc)) {
			offset++;
		} else {
			if(((' ' == *p_uc) || ('-' == *p_uc)) && (_is_alpha_digit(*(p_uc + 1)))) {
				offset++;
			} else {
				break;
			}
		}
		p_uc++;
	}

	return (offset);
}

typedef enum _SubStringType {
	SUB_STRING_SKIP,
	SUB_STRING_REVERT
} SubStringType;

#ifndef NO_OS
static SubStringType _substring_len(unsigned int *unicode, int* len)
{
	unsigned int *p_uc = unicode;

	if(GxIsArabicCharacter(*p_uc)) {
		while(GxIsArabicCharacter(*p_uc) ||
		      (_is_unicode_symbol(*p_uc)) ||
		      (' ' == *p_uc)) {
			p_uc++;
			if(('.' == *p_uc) ||
			   (' ' == *p_uc) ||
			   (_is_unicode_symbol(*p_uc))) {
				p_uc++;
				continue;
			} else if(('\0' == *p_uc) ||
				  ('\n' == *p_uc) ||
				  ('\r' == *p_uc)) {
				break;
			}
		}

		if(' ' == *(p_uc - 1)) {
			p_uc--;
		}

		*len = p_uc - unicode;
		return (SUB_STRING_REVERT);
	} else {
		while(!GxIsArabicCharacter(*p_uc)) {
			if(('\0' == *p_uc) ||
			   ('\n' == *p_uc) ||
			   ('\r' == *p_uc)) {
				break;
			}
			p_uc++;
		}

		*len = p_uc - unicode;
		return (SUB_STRING_SKIP);
	}
}

static void _substring_revert(unsigned int *unicode, int len)
{
	unsigned int *p_uc = unicode, tmp = 0;
	int i = 0;

	if(len < 1) {
		return;
	}

	while(i < len / 2) {
		if(_symmetrical_symbol_map(p_uc[i])) {
			tmp = _symmetrical_symbol_map(p_uc[i]);
		} else {
			tmp = p_uc[i];
		}

		if(_symmetrical_symbol_map(p_uc[len - i - 1])) {
			p_uc[i] = _symmetrical_symbol_map(p_uc[len - i - 1]);
		} else {
			p_uc[i] = p_uc[len - i - 1];
		}
		p_uc[len - i - 1] = tmp;
		i++;
	}
}

static int _revert_string_line(unsigned int *unicode)
{
	unsigned int *p_uc = unicode;
	int len = 0, type = 0;

	while(*p_uc) {
		if(('\n' == *p_uc) ||
		   ('\r' == *p_uc)) {
			return (p_uc - unicode);
		}

		type = _substring_len(p_uc, &len);

		if(SUB_STRING_SKIP == type) {
			;
		} else if (SUB_STRING_REVERT == type) {
			_substring_revert(p_uc, len);
		}
		p_uc += len;
	}

	return (p_uc - unicode);
}
#endif

static void _process_digit(unsigned int *uc, int len)
{
	if('-' == uc[0]) {
		memmove(uc, uc + 1, len * 4 - 4);
		uc[len - 1] = '-';
	}
}

static void _process_arabic_vowel(unsigned int *unicode)
{
	unsigned int *p_uc = unicode, tmp = 0;

	if('\0' == *(p_uc + 1))
		return;
	while(*p_uc) {
		if(*p_uc == 0x064E) {
			tmp = *p_uc;
			*p_uc = *(p_uc + 1);
			*(p_uc + 1) = tmp;
			p_uc++;
		}
		p_uc++;
	}
}

static int font_utf8_text(void *screen, void *string, GAL_Rect *rect, GAL_Interval *interval, int alignment,  int color, int flag)
{
	unsigned char *pstr = NULL, *p_uc = NULL, *p_uc_flag = NULL, *unicode = NULL, *unicode_flag = NULL, *tmpstr = NULL;
	unsigned char iso6937_flag = 0, is_iso6937_str = 0, is_arabic = 0, is_right2left = 0;
	unsigned int len = 0, i = 0, tmpint = 0;
	unsigned int arabic_code = 0, accent = 0, ret = 0;
	struct gdi_font *pFont = NULL;

	pstr = (unsigned char *)string;
	if(NULL == pstr)
	{
		return (-1);
	}

	pFont = gxgdi_get_font();
	/*UTF-8*/
	is_arabic = gxgdi_is_arabic(pstr);
	is_right2left = gxgdi_is_right2left(pstr);
	if((is_right2left) && (is_arabic) && ((rect->x + rect->w + pFont->xsize) <= ((GuiSurface *)screen)->sf.width)) {
		rect->w = rect->w + pFont->xsize;
	}
	_font_fix_arabic_keyinfo();

	len = strlen((char *)pstr);

	unicode = (unsigned char *)GUI_MALLOCZ((len + 1) * 4 + 100);
	if(NULL == unicode)
	{
		return (-1);
	}
	unicode_flag = (unsigned char *)GUI_MALLOCZ((len + 1) * 4 + 100) ;
	if(NULL == unicode_flag)
	{
		GUI_FREE(unicode);
		return (-1);
	}

	unicode[(len + 1) * 4] = 0xFF;
	unicode[(len + 1) * 4 + 1] = 0xFF;
	unicode[(len + 1) * 4 + 2] = 0xFF;
	unicode[(len + 1) * 4 + 3] = 0xFF;
	unicode_flag[(len + 1) * 4] = 0xFF;
	unicode_flag[(len + 1) * 4 + 1] = 0xFF;
	unicode_flag[(len + 1) * 4 + 2] = 0xFF;
	unicode_flag[(len + 1) * 4 + 3] = 0xFF;

	p_uc = unicode;

	i = 0;
	while(*pstr)
	{
		gdi_font_utf82unicode(&pstr, &arabic_code, &accent, &iso6937_flag);
		if((0x1039 < arabic_code) &&
		   (0x103F > arabic_code))
			arabic_code -= 1;
		if(0x1039 == arabic_code)
			continue;

		if(iso6937_flag)
		{
			CHECK_UNICODE(unicode);
			unicode[i] = accent & 0x000000FF;
			i++;
			unicode[i] = (accent >> 8) & 0x000000FF;
			i++;
			unicode[i] = (accent >> 16) & 0x000000FF;
			i++;
			unicode[i] = (accent >> 24) & 0x000000FF;
			i++;
			CHECK_UNICODE(unicode);
			iso6937_flag = 0;
		}

		if((arabic_code >= 0x0600) && (arabic_code <= 0x06FF))
		{
			CHECK_UNICODE(unicode);
			CHECK_UNICODE(unicode_flag);
			unicode[i] = arabic_code & 0x000000FF;
			unicode_flag[i] = '1';
			i++;
			unicode[i] = (arabic_code >> 8) & 0x000000FF;
			unicode_flag[i] = '1';
			i++;
			unicode[i] = (arabic_code >> 16) & 0x000000FF;
			unicode_flag[i] = '1';
			i++;
			unicode[i] = (arabic_code >> 24) & 0x000000FF;
			unicode_flag[i] = '1';
			CHECK_UNICODE(unicode);
			CHECK_UNICODE(unicode_flag);
		}
		else if((arabic_code >= 0x0590) && (arabic_code <= 0x05FF))
		{
			CHECK_UNICODE(unicode);
			CHECK_UNICODE(unicode_flag);
			unicode[i] = arabic_code & 0x000000FF;
			unicode_flag[i] = '1';
			i++;
			unicode[i] = (arabic_code >> 8) & 0x000000FF;
			unicode_flag[i] = '1';
			i++;
			unicode[i] = (arabic_code >> 16) & 0x000000FF;
			unicode_flag[i] = '1';
			i++;
			unicode[i] = (arabic_code >> 24) & 0x000000FF;
			unicode_flag[i] = '1';
			CHECK_UNICODE(unicode);
			CHECK_UNICODE(unicode_flag);
		}
		else if((arabic_code == 0x0020) && (string != (pstr - 1)))
		{
			CHECK_UNICODE(unicode);
			CHECK_UNICODE(unicode_flag);
			unicode[i] = arabic_code & 0x000000FF;
			unicode_flag[i] = '1';
			i++;
			unicode[i] = (arabic_code >> 8) & 0x000000FF;
			unicode_flag[i] = '1';
			i++;
			unicode[i] = (arabic_code >> 16) & 0x000000FF;
			unicode_flag[i] = '1';
			i++;
			unicode[i] = (arabic_code >> 24) & 0x000000FF;
			unicode_flag[i] = '1';
			CHECK_UNICODE(unicode);
			CHECK_UNICODE(unicode_flag);
		}
		else
		{
			if((is_iso6937_str) && ((arabic_code >= 0x00A0) && (arabic_code <= 0x00FF)))
			{
#ifndef NO_OS
#ifdef GUI_STR_ICONV
				if(iso6937_char_set[arabic_code - 0x00A0])
				{
					arabic_code = iso6937_char_set[arabic_code - 0x00A0];
				}
				else
				{
					arabic_code = 0xFFFFFFFF;
				}
#endif /*GUI_STR_ICONV*/
#endif /*NO_OS*/
			}
			CHECK_UNICODE(unicode);
			CHECK_UNICODE(unicode_flag);
			unicode[i] = arabic_code & 0x000000FF;
			if((is_arabic) &&
				('.' != arabic_code) &&
				('\n' != arabic_code) &&
				('\r' != arabic_code))
			{
				unicode_flag[i] = '0';
			}
			i++;
			unicode[i] = (arabic_code >> 8) & 0x000000FF;
			if((is_arabic) &&
				('.' != arabic_code) &&
				('\n' != arabic_code) &&
				('\r' != arabic_code))
			{
				unicode_flag[i] = '0';
			}
			i++;
			unicode[i] = (arabic_code >> 16) & 0x000000FF;
			if((is_arabic) &&
				('.' != arabic_code) &&
				('\n' != arabic_code) &&
				('\r' != arabic_code))
			{
				unicode_flag[i] = '0';
			}
			i++;
			unicode[i] = (arabic_code >> 24) & 0x000000FF;
			if((is_arabic) &&
				('.' != arabic_code) &&
				('\n' != arabic_code) &&
				('\r' != arabic_code))
			{
				unicode_flag[i] = '0';
			}
			CHECK_UNICODE(unicode);
			CHECK_UNICODE(unicode_flag);
		}

		i++;
	}

	CHECK_UNICODE(unicode);
	CHECK_UNICODE(unicode_flag);
	memset(unicode + i, 0, 4 * sizeof(unsigned char));
	memset(unicode_flag + i, 0, 4 * sizeof(unsigned char));
	i += 4;

	if(0xFEFF == ((((*p_uc) << 8) && 0xFF00) | (*(p_uc + 1))))
	{
		p_uc += 4;
		i -= 4;
	}

	/*Convert to presentation format*/
//	len = i;

	p_uc = unicode;
	p_uc_flag = unicode_flag;

	if(is_right2left) {
		while(*p_uc)
		{
			unsigned int tmpunicode = 0;
			tmpstr = p_uc;
			tmpint = 0;

			tmpunicode = ((*(tmpstr + 3)) << 24) | ((*(tmpstr + 2)) << 16) | ((*(tmpstr + 1)) << 8) | (*tmpstr);
			if(GxIsArabicCharacter(tmpunicode) || _symmetrical_symbol_map(tmpunicode)) {
				while(GxIsArabicCharacter(tmpunicode) || (' ' == tmpunicode) || _is_unicode_symbol(tmpunicode))
				{
					tmpunicode = ((*(tmpstr + 3)) << 24) | ((*(tmpstr + 2)) << 16) | ((*(tmpstr + 1)) << 8) | (*tmpstr);
					if(('\n' == tmpunicode) || (_is_unicode_symbol(tmpunicode))/* || ('1' != *p_uc_flag)*/)
					{
						break;
					}
					tmpstr += 4;
					p_uc_flag += 4;
					tmpint += 4;
					CHECK_UNICODE(tmpstr);
				}

				if((!GxIsArabicCharacter(tmpunicode)) && (tmpint)) {
					tmpint -= 4;
				}
			} else {
				tmpstr += 4;
				p_uc_flag += 4;
				tmpint += 4;
				CHECK_UNICODE(tmpstr);
			}

			unsigned int *p_unicode = (unsigned int *)p_uc;
#ifndef NO_OS
			ARABIC_STR_PRINT_MODE tmp_mode = gui.config.arabic_mode;
			if(ARABIC_PRINT_MODE0 == tmp_mode) {
#endif /*NO_OS*/

				if(GxIsArabicCharacter(*p_unicode) || _symmetrical_symbol_map(*p_unicode))
				{
					int arabic_len = 0, total_len = 0, str_num = 0, alpha_digit_len = 0;
					unsigned int *p_wunicode = (unsigned int *)p_uc, *p_ascii = NULL, *p_arabic = 0;

					str_num = gxgdi_string_num(string) - (p_unicode - (unsigned int *)unicode) / 4;
					while((*p_wunicode) && (!GxIsArabicCharacter(*p_wunicode))) {
						if(_symmetrical_symbol_map(*p_wunicode) && GxIsArabicCharacter(*(p_wunicode + 1)))
							break;
						p_wunicode++;
					}
					while(p_wunicode < (p_unicode + str_num)) {
						if(0 == *p_wunicode)
							break;
						arabic_len = _arabic_string_line_len(p_wunicode);
						if(_check_arabic_wstring(p_wunicode)) {
							if(arabic_len) {
								p_arabic = p_wunicode + arabic_len;
								while(*p_arabic) {
									if(' ' != *p_arabic) {
										break;
									}
									p_arabic++;
									arabic_len++;
								}
								_arabic_revert(p_wunicode, arabic_len);
								_revert_arabic_wstring(p_wunicode, arabic_len, GUI_FALSE);
							}

							alpha_digit_len = _alpha_digit_offset(p_wunicode + arabic_len);
							if(0 == alpha_digit_len) {
								p_wunicode += (arabic_len + 1);
								total_len += (arabic_len + 1);
							} else {
								p_ascii = GUI_MALLOCZ((alpha_digit_len + 4) * sizeof(unsigned int));
								if(p_ascii) {
									memcpy(p_ascii,
									       p_wunicode + arabic_len,
									       alpha_digit_len * sizeof(unsigned int));
									if(is_arabic) {
										_process_digit(p_ascii, alpha_digit_len);
									}
									memmove(p_wunicode + alpha_digit_len,
										p_wunicode,
										arabic_len * sizeof(unsigned int));
									memcpy(p_wunicode, p_ascii, alpha_digit_len * sizeof(unsigned int));
									GUI_FREE(p_ascii);
								}
								p_wunicode += (arabic_len + alpha_digit_len);
								total_len += (arabic_len + alpha_digit_len);
							}
						} else {
							p_wunicode += arabic_len;
							total_len += arabic_len;
						}
						if(arabic_len)
							p_uc += arabic_len;
						else if(GxIsArabicCharacter(*p_wunicode))
							continue;
						else {
							p_wunicode++;
							total_len++;
							p_uc++;
						}
						CHECK_UNICODE(unicode);
						CHECK_UNICODE(unicode);
						tmpint = 0;
					}
					goto DISPLAY;
				}
#ifndef NO_OS
			} else {
				int substr_len = 0;

				while(*p_unicode) {
					if(('\n' == *p_unicode) ||
					('\r' == *p_unicode)) {
						p_unicode++;
					}
					substr_len = _revert_string_line(p_unicode);
					p_unicode += substr_len;
				}

				if(*p_unicode == '\0') {
					break;
				}
			}
#endif /*NO_OS*/

			if('\0' != tmpunicode)
			{
				p_uc = tmpstr;
			}
			else
			{
				break;
			}
		}
	}
DISPLAY:
	if(is_arabic)
		_process_arabic_vowel((unsigned int *)unicode);
	GUI_FREE(unicode_flag);

	if((check_khmer((const unsigned int *)unicode))
#ifdef GUI_TTF
#ifndef NO_OS
	   && (NULL == pFont->info)
#endif /*NO_OS*/
#endif /*ndef GUI_TTF*/
	   )
	{
	    ret = khmer_display_presentation(screen,
			    (const unsigned int *)unicode,
			    rect,
			    len,
			    color,
			    alignment,
			    flag);
	}
	else if((check_devanagari((const unsigned int *)unicode))
#ifdef GUI_TTF
#ifndef NO_OS
			&& (NULL == pFont->info)
#endif /*NO_OS*/
#endif /*ndef GUI_TTF*/
			)
	{
	    ret = devanagari_display_presentation(screen,
			    (const unsigned int *)unicode,
			    rect,
			    len,
			    color,
			    alignment,
			    flag);
	}
	else
	{
	    if(font_utf8_len(string) == rect->w) {
		alignment = (alignment & GUI_TA_VERTICAL) | GUI_TA_LEFT;
	    }
	    ret = _arabic_display_presentation(screen,
			    (const unsigned int *)unicode,
			    rect,
			    interval->vertical,
			    len,
			    color,
			    alignment,
			    flag,
			    is_arabic);
	    ret = _get_position((unsigned char *)string, ret);
	    if(ret > strlen((char *)string))
	    {
		ret = strlen((char *)string);
	    }
	}

	GUI_FREE(unicode);

	return (ret);
}

#ifndef NO_OS
#ifdef GUI_ROLL_STR
static int _ascii_skip_line(unsigned char *p_start,
		unsigned char *string,
		GAL_Interval *interval,
		int line_width,
		int code)
{
	int width = 0, line = 0, offset = 0, line_off = 0;
	unsigned int word_len = 0, text_off = 0;
	unsigned char *pstr = NULL;

	if(line_width < 0)
	{
		width = -line_width;
	}
	else
	{
		width = line_width;
	}

	line = 0;
	line_off = 0;
	if(line_width > 0)
	{
		if('\n' == *string)
		{
			line_off++;
			string++;
			return (line_off);
		}
		else if('\r' == *string)
		{
			string++;
			line_off++;
			if('\n' == *string)
			{
				string++;
				line_off++;
			}
			return (line_off);
		}

		text_off = _ascii_check_line_full((const unsigned char *)string,
				width,
				interval->horizontal,
				code);
		if(text_off < width)
		{
			width -= text_off;
		}

		while(line < width)
		{
			if('\n' == *string)
			{
				string++;
				line_off++;
				break;
			}
			else if('\r' == *string)
			{
				string++;
				line_off++;
				if('\n' == *string)
				{
					string++;
					line_off++;
				}
			}
			else if('\0' == *string)
			{
				return (line_off);
			}

			word_len = width;
			offset = _get_word_width(string, code, width, &word_len);
			if((line + word_len) > width)
			{
				break;
			}
			string += offset;
			line_off += offset;
			line += word_len;

			if(line > width)
			{
				line_off -= offset;
				break;
			}

			while((' ' == *string) || (_is_cjk_symbol(*string)))
			{
				line += gdi_get_charwidth(*string);
				string++;
				line_off++;

				if(line > width)
				{
					if(0x11 == code)
					{
						line_off -= 2;
					}
					else
					{
						line_off--;
					}
					break;
				}
			}

			if(line > width)
			{
				break;
			}
			else if('\n' == *string)
			{
				string++;
				line_off++;
				break;
			}
			else if('\r' == *string)
			{
				string++;
				line_off++;
				if('\n' == *string)
				{
					string++;
					line_off++;
				}
			}
			else if('\0' == *string)
			{
				if(line <= width)
				{
					return (0);
				}
				else
				{
					return (line_off);
				}
			}
		}

	}
	else
	{
		line = 0;
		pstr = string;
		if((' ' == *pstr) || (_is_cjk_symbol(*pstr)))
		{
			line += gdi_get_charwidth(*pstr);
		}
		else if(('\n' != *pstr) && ('\r' != *pstr))
		{
			_get_word_width(pstr, code, width, &word_len);
			line += word_len;
		}
		line_off = 0;

		while(line < width)
		{
			if(pstr == p_start)
			{
				break;
			}

			if((' ' == *(pstr - 1)) || (_is_cjk_symbol(*(pstr - 1))))
			{
				while((' ' == *(pstr - 1)) || (_is_cjk_symbol(*(pstr - 1))))
				{
					word_len = gdi_get_charwidth(*(pstr - 1));
					line += word_len;
					pstr--;
					line_off--;

					if(line > width)
					{
						pstr++;
						line_off++;
						line_off = -line_off;
						return (line_off);
					}
				}
			}
			else
			{
				word_len = width;
				offset = _getback_word_width(pstr,
						p_start,
						code,
						&word_len);
				line += word_len;
				pstr += offset;
				line_off += offset;

				if(('\r' == *pstr) || ('\n' == *pstr))
				{
					if(('\r' != *(pstr - 1)) &&
							('\n' != *(pstr - 1)))
					{
						line = 0;
						continue;
					}
					else
					{
						break;
					}
				}
				else if('\0' == *pstr)
				{
					return (line_off);
				}
			}

			if(line > width)
			{
				pstr -= offset;
				line_off -= offset;
				break;
			}
		}
		line_off = -line_off;
	}

	return (line_off);
}

static int _get_utf8_word_width(unsigned char *pstr, unsigned int *length)
{
	unsigned char *ptr = NULL;
	unsigned int count = 0, word_length = 0;
	unsigned int wc = 0, accent = 0;
	unsigned char iso6937_flag = 0;

	if((NULL == pstr) || (NULL == length))
	{
		return (0);
	}

	ptr = pstr;
	while(*ptr)
	{
		gdi_font_utf82unicode(&ptr, &wc, &accent, &iso6937_flag);
		if((!iso6937_flag) &&
				(('\0' == wc) ||
				 ('\n' == wc) ||
				 (' ' == wc) ||
				 _is_cjk_symbol(wc)))
		{
			if(('\n' == wc) || (' ' == wc))
			{
				//word_length += gdi_get_charwidth(wc);
				ptr--;
			}

			break;
		}

		if(_is_vowel(wc))
		{
			;
		}
		else
		{
			word_length += gdi_get_charwidth(wc);
			count++;
			if(iso6937_flag && accent)
			{
				count++;
				iso6937_flag = 0;
				accent = 0;
			}
		}
	}

	if('\n' == wc)
	{
		count++;
	}

	*length = word_length;

	return (ptr - pstr);
}

static int font_utf8_skip_line(void *p_start, void *string, GAL_Interval *interval, int line_width, int alignment)
{
	int width = 0, offset = 0, line = 0, char_width = 0, line_num = 0, line_end = 0;
	int code = 0, char_count = 0;
	unsigned int word_length = 0;
	unsigned char * p_str = NULL, *p_head = NULL, *ptr = NULL, *p_primitive = NULL, *p_tmp = NULL;
	unsigned int wc = 0, horizontal = 0, accent = 0;
	unsigned char iso6937_flag = 0;

	if(NULL == string || NULL == p_start || NULL == interval)
	{
		return (0);
	}

	p_str = (unsigned char *)string;
	p_head = p_start;

	horizontal = interval->horizontal;
	//vertical = interval->vertical;

	if(line_width < 0)
	{
		width = -line_width;
	}
	else
	{
		width = line_width;
	}

	if(p_head == p_str)
	{
		_arabic_get_first_byte(&p_str);
	}
	code = _arabic_get_first_byte(&p_head);
	offset = p_str - (unsigned char *)string;
	if(code)
	{
		p_head = p_start;
		p_str = string;
		return (offset + _ascii_skip_line(p_head, p_str, interval, line_width, code));
	}

	/*text_off = _utf8_check_line_full((const unsigned char *)p_str, line_width, horizontal);

	  if(text_off < width)
	  {
	  width -= text_off;
	  }*/

	p_head = p_start;

	line = 0;
	if(line_width > 0)
	{
		do
		{
			if('\0' == *(p_str))
			{
				return (p_str - (unsigned char *)string);
			}

			if((' ' == *(p_str)) || (_is_cjk_symbol(*p_str)))
			{
				while((' ' == *(p_str)) || (_is_cjk_symbol(*p_str)))
				{
					gdi_font_utf82unicode(&p_str,
							&wc,
							&accent,
							&iso6937_flag);
					word_length = gdi_get_charwidth(wc);
					if((line + word_length) > width)
					{
						if(0x20 == wc)
						{
							p_str -= 1;
						}
						break;
					}
					else
					{
						line += word_length;
					}
				}

				if((line + word_length) > width)
				{
					if('\n' == *p_str)
					{
						p_str++;
					}
					break;
				}
			}

			if(gxgdi_is_arabic(p_str)) {
				char_count = _calc_word_len(p_str, (int *)&word_length, 0, 1);
			} else {
				char_count = _get_utf8_word_width(p_str, &word_length);
			}

			p_tmp = GxCore_Mallocz(char_count + 10);
			memcpy(p_tmp, p_str, char_count);
			if((p_tmp) && (!gxgdi_is_arabic(p_tmp))) {
				word_length = gdi_text_surfacewidth(p_tmp);
				GxCore_Free(p_tmp);
				p_tmp = NULL;
			}

			if((0 == line) && (width < word_length))
			{
				word_length = 0;
				char_count = 0;
				ptr = p_str;
				while(*ptr)
				{
					p_primitive = ptr;
					gdi_font_utf82unicode(&ptr, &wc, &accent, &iso6937_flag);
					word_length += gdi_get_charwidth(wc);
					if(word_length >= width)
					{
						if(word_length > width)
						{
						    ptr = p_primitive;
						}
						break;
					}
				}

				char_count = ptr - p_str;
				p_str = ptr;
				break;
			}

			if((line + word_length) > width)
			{
				break;
			}
			else
			{
				p_str += char_count;
				wc = *p_str;

				/*gdi_font_utf82unicode(&p_str,
				  &wc,
				  &accent,
				  &iso6937_flag);*/

				//p_str += char_count;
				line += word_length;
			}

			if(line > width)
			{
				break;
			}

			if('\n' == wc)
			{
				if('\n' == *p_str)
				{
					p_str++;
				}
				break;
			}
		}while(line < width);

		offset = (int)(p_str - (unsigned char *)string);
	}
	else if(p_head <= p_str)
	{
		line_num = 0;
		do
		{
			gdi_font_utf82unicode(&p_head, &wc, &accent, &iso6937_flag);

			char_width = gdi_get_charwidth(wc) + horizontal;

			line += char_width;

			offset++;

			if(line > width)
			{
				offset --;
			}

			if((line >= width) || ('\n' == wc))
			{
				line_end = line;
				line_num++;
				line = 0;
			}
		}while(p_head < (p_str ));

		if(line > line_end)
		{
			line_end = line;
		}

		char_width = 0;
		offset = 0;

		if('\n' == *(p_str - 1))
		{
			if(((*p_str) & 0x80) == UTF8_0XXXXXXX)
			{
				p_str--;
				offset++;
			}
			else if(((*p_str) & 0xE0) == UTF8_110XXXXX)
			{
				p_str -= 2;
				offset += 2;
			}
			else if(((*p_str) & 0xF0) == UTF8_1110XXXX)
			{
				p_str -= 3;
				offset += 3;
			}
			else if(((*p_str) & 0xF8) == UTF8_11110XXX)
			{
				p_str -= 4;
				offset += 4;
			}
			else if(((*p_str) & 0xFC) == UTF8_111110XX)
			{
				p_str -= 5;
				offset += 5;
			}
			else if(((*p_str) & 0xfc) == UTF8_1111110X)
			{
				p_str -= 6;
				offset += 6;
			}
			else
			{
				p_str--;
				offset++;
			}
		}

		do
		{
			wc = *(p_str - 1);

			char_width += (gdi_get_charwidth(wc) + horizontal);

			if(char_width > line_end)
			{
				break;
			}

			if('\n' == wc)
			{
				break;
			}

			if(((*p_str) & 0x80) == UTF8_0XXXXXXX)
			{
				p_str--;
				offset++;
			}
			else if(((*p_str) & 0xE0) == UTF8_110XXXXX)
			{
				p_str -= 2;
				offset += 2;
			}
			else if(((*p_str) & 0xF0) == UTF8_1110XXXX)
			{
				p_str -= 3;
				offset += 3;
			}
			else if(((*p_str) & 0xF8) == UTF8_11110XXX)
			{
				p_str -= 4;
				offset += 4;
			}
			else if(((*p_str) & 0xFC) == UTF8_111110XX)
			{
				p_str -= 5;
				offset += 5;
			}
			else if(((*p_str) & 0xfc) == UTF8_1111110X)
			{
				p_str -= 6;
				offset += 6;
			}
			else
			{
				p_str--;
				offset++;
			}
		}while(char_width <= line_end);

	}

	return (offset);
}

static int _get_string_pos(unsigned char **pstr, int pos)
{
	unsigned char *tmp_str = *pstr, *p_prev = NULL;
	unsigned int unicode = 0, accent = 0;
	unsigned char is_6937 = 0;
	unsigned int str_width = 0, prev_width = 0;

	gdi_font_utf82unicode(&tmp_str, &unicode, &accent, &is_6937);
	prev_width = gdi_get_charwidth(unicode);
	tmp_str = *pstr;
	while(*tmp_str)
	{
		p_prev = tmp_str;
		gdi_font_utf82unicode(&tmp_str, &unicode, &accent, &is_6937);
		prev_width = gdi_get_charwidth(unicode);
		str_width += gdi_get_charwidth(unicode);
		if(str_width > pos)
		{
			tmp_str = p_prev;
			str_width -= prev_width;
			break;
		}
		else if(str_width == pos)
		{
			break;
		}
	}

	*pstr = tmp_str;

	return (pos - str_width);
}

static int _arabic_roll_text(GuiSurface *surface, char *string, GAL_Rect *rect, int pos, int color, int flag)
{
	GAL_Rect surface_rect = {0}, src_rect = {0}, dst_rect = {0};
	int text_len = 0;
	GAL_Interval interval = {0};
	GuiSurface *tmp_surface = NULL;

	text_len = gdi_text_len(string);
	surface_rect = *rect;
	surface_rect.x = surface_rect.y = 0;
	surface_rect.w = text_len;
	surface_rect.w = ((((surface_rect.w * surface->bpp) + 31) >> 5) << 2) / (surface->bpp / 8);

	tmp_surface = gal_get_surface(&surface_rect, surface->bpp);
	if(NULL == tmp_surface) {
		return (-1);
	}

	src_rect = *rect;
	dst_rect = *rect;
	dst_rect.x = pos;
	dst_rect.y = 0;
	if((dst_rect.x + dst_rect.w) > tmp_surface->sf.width) {
		dst_rect.w = tmp_surface->sf.width - dst_rect.x;
	}
	src_rect.w = dst_rect.w;
	gal_copy_surface(surface, &src_rect, tmp_surface, &dst_rect);
	gdi_begin();
	font_utf8_text(tmp_surface,
		       string,
		       &surface_rect,
		       &interval,
		       GUI_TA_LEFT | GUI_TA_VCENTRE,
		       color,
		       flag);
	gdi_end();

	if((dst_rect.x + dst_rect.w) > tmp_surface->sf.width) {
		dst_rect.w = tmp_surface->sf.width - dst_rect.x;
	}
	gdi_begin();
	gal_stretch_surface(tmp_surface, &dst_rect, surface, &src_rect);
	gdi_end();

	gal_free_surface(tmp_surface);

	return (0);
}

static int font_utf8_roll_text(void *screen, void *string, GAL_Rect* rect, int pos, int color, int flag)
{
	GuiCore *gui_core = NULL;
	void *surface = NULL;
	unsigned char *pstr = NULL, *tmp_str = NULL;
	unsigned int line = 0, fore_color = 0, tmp_offset = 0;
	GAL_Interval interval = {0};
	GAL_Rect src_rect = {0}, dst_rect = {0};
	struct gdi_font *pFont = NULL;

	gui_core = GUI_GetCurrent();
	pstr = (unsigned char *)string;

	src_rect = *rect;
	src_rect.x = src_rect.y = 0;

	if(gxgdi_is_arabic((const unsigned char *)string)) {
		_arabic_roll_text((GuiSurface *)screen, string, rect, pos, color, flag);
		return (0);
	}

	pFont = gxgdi_get_font();

	tmp_str = pstr;
	tmp_offset = _get_string_pos(&tmp_str, pos);

	if(SINGLELINE_MODE == flag)
	{
		line = font_utf8_len(pstr) + pFont->xsize * 3;
		src_rect.w = rect->w + tmp_offset * 10;
	}
	else
	{
		src_rect.h += gdi_get_charheight();
	}

	src_rect.w = ((((src_rect.w * gui_core->config.bpp) + 31) >> 5) << 2) / (gui_core->config.bpp / 8);
	surface = gal_get_surface(&src_rect, ((GuiSurface *)screen)->bpp);
	//src_rect.w = line;

	fore_color = gui.config.gui_trans;
	fore_color = gal_color2index(screen, fore_color);
	gdi_begin();
	if(gui.config.enable_double_buffer)
	{
		dst_rect = *rect;
		src_rect.w = rect->w;
		src_rect.x += tmp_offset;
		if((src_rect.x + src_rect.w) > line)
		{
			dst_rect.w = src_rect.w = line - src_rect.x;
		}
		gal_copy_surface(screen, &dst_rect, surface, &src_rect);
		src_rect.x = 0;
		src_rect.w = ((GuiSurface*)surface)->sf.width;
	}
	else
	{
		hd_fillrect(surface, &src_rect, fore_color);
	}

	font_utf8_text(surface,
			tmp_str,
			&src_rect,
			&interval,
			GUI_TA_LEFT | GUI_TA_VCENTRE,
			color,
			flag);
	gdi_end();

	if((SINGLELINE_MODE == flag) && (tmp_offset >= 0))
	{
		src_rect.x += tmp_offset;
		src_rect.w = rect->w;

		if((src_rect.x + src_rect.w) > line)
		{
			src_rect.w = line - src_rect.x;
		}
	}
	else if((PARAGRAPH_MODE == flag) && (pos >= 0))
	{
		src_rect.y += (pos % gdi_get_charheight());
	}

	dst_rect = *rect;
	if(tmp_offset < 0)
	{
		dst_rect.x = dst_rect.x - tmp_offset;
		src_rect.w = dst_rect.w = dst_rect.w + tmp_offset;
	}

	gal_stretch_surface(surface, &src_rect, screen, &dst_rect);

	gdi_commit();

	gal_free_surface(surface);

	return (0);
}

static int font_utf8_lines(void *string, GAL_Rect *rect, GAL_Interval *interval)
{
	int lines = 0, text_width = 0, code = 0;
	unsigned int word_width = 0, word_offset = 0;
	char *pStr = NULL, *ptr = NULL;
	unsigned int wc = 0, horizontal = 0, accent = 0;
	unsigned char iso6937_flag = 0;

	if((NULL == string) || (NULL == rect) || (NULL == interval))
	{
		return (0);
	}

	pStr = (char *)string;
	lines = 1;

	horizontal = interval->horizontal;

	code = _arabic_get_first_byte((unsigned char **)&pStr);

	while(*pStr != '\0')
	{
		if(code)
		{
			ptr = pStr;
			wc = *pStr;
			pStr++;
			accent = 0;
		}
		else
		{
			ptr = pStr;
			gdi_font_utf82unicode((unsigned char **)&pStr, &wc, &accent, &iso6937_flag);
		}

		if(('\n' == wc))
		{
			lines++;
			text_width = 0;
			continue;
		}

		if(_is_cjk_symbol(wc) || (' ' == wc)) {
			text_width += (gdi_get_charwidth(wc) + horizontal);
		} else {
			pStr = ptr;
			word_offset = _get_utf8_word_width((unsigned char *)pStr, &word_width);
			text_width += word_width;
			pStr += word_offset;
		}
		if(text_width >= (rect->w))
		{
			if(word_width > rect->w) {
				lines += (word_width / rect->w);
			} else {
				if(text_width > rect->w) {
					pStr = ptr;
				}
				lines++;
			}
			text_width = 0;
		}
	}

	return (lines);
}
#endif /*GUI_ROLL_STR*/
#endif /*NO_OS*/

#ifndef NO_OS
struct gdi_font_opt font_utf8_opt = {
	"utf8",
	font_utf8_len,
	font_utf8_text,
#ifdef GUI_ROLL_STR
	font_utf8_skip_line,
	font_utf8_roll_text,
	font_utf8_lines
#else
	NULL,
	NULL,
	NULL
#endif /*GUI_ROLL_STR*/
};
#else
struct gdi_font_opt font_utf8_opt = {
	"utf8",
	font_utf8_len,
	font_utf8_text,
	NULL,
	NULL,
	NULL
};
#endif /*NO_OS*/
