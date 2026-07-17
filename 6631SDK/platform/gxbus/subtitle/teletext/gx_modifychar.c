#include "libzvbi/libzvbi.h"
#include "gx_magazine.h"
#include "gx_modifychar.h"

static nation_modifychar cze_modifychar =
{
	.count    = 2,
	.mchar[0] =
	{
		.char_set   = LATIN_G0,              ///< Teletext-Magazine 字幕字符库
		.sub_set    = CZECH_SLOVAK,          ///< Teletext-Magazine 字幕语言
		.char_val   = 0x10f,                 ///< Teletext-Magazine 字幕字符
		.char_pic   =
		{                      // 012345670123
			0x00,0x0c,         // 000000000011
			0x80,0x07,         // 000000011110
			0x80,0x01,         // 000000011000
			0xfc,0x01,         // 001111111000
			0x86,0x01,         // 011000011000
			0x86,0x01,         // 011000011000
			0x86,0x01,         // 011000011000
			0xfc,0x01,         // 001111111000
			0x00,0x00,         // 000000000000
			0x00,0x00,         // 000000000000
		},
	},
	.mchar[1] =
	{
		.char_set   = LATIN_G0,              ///< Teletext-Magazine 字幕字符库
		.sub_set    = CZECH_SLOVAK,          ///< Teletext-Magazine 字幕语言
		.char_val   = 0x5b,                  ///< Teletext-Magazine 字幕字符
		.char_pic   =
		{                      // 012345670123
			0x00,0x0c,         // 000000000011
			0x18,0x06,         // 000110000110
			0x18,0x00,         // 000110000000
			0xfe,0x00,         // 011111110000
			0x18,0x00,         // 000110000000
			0x18,0x00,         // 000110000000
			0x18,0x03,         // 000110001100
			0xf0,0x01,         // 000011111000
			0x00,0x00,         // 000000000000
			0x00,0x00,         // 000000000000
		},
	}
};

int GxMagazine_CZEModifyChar(void)
{
	unsigned int i = 0;

	for(i = 0; i < cze_modifychar.count; i++) {
		GxMagazine_VBIModifyChar(&(cze_modifychar.mchar[i]));
	}

	return 0;
}
