/*************************************************************************
	> File Name: app/app_keyboard_language_config.c
	> Author:
	> Mail:
	> Created Time: 2020年08月10日 星期一 20时04分12秒
 ************************************************************************/

#include "app.h"
#include "app_config.h"
#include "app_keyboard_language.h"

#ifdef KB_LANG_ENGLISH
char *s_english_keymap[KEY_COUNT][4] = {
	{ "q","Q","1","button_key_0",},
	{ "w","W","2","button_key_1",},
	{ "e","E","3","button_key_2",},
	{ "r","R","4","button_key_3",},
	{ "t","T","5","button_key_4",},
	{ "y","Y","6","button_key_5",},
	{ "u","U","7","button_key_6",},
	{ "i","I","8","button_key_7",},
	{ "o","O","9","button_key_8",},
	{ "p","P","0","button_key_9",},
	{ "[","{","-","button_key_10",},
	{ "]","}","=","button_key_11",},
	{ "a","A","!","button_key_12",},
	{ "s","S","@","button_key_13",},
	{ "d","D","#","button_key_14",},
	{ "f","F","$","button_key_15",},
	{ "g","G","%","button_key_16",},
	{ "h","H","^","button_key_17",},
	{ "j","J","&","button_key_18",},
	{ "k","K","*","button_key_19",},
	{ "l","L","(","button_key_20",},
	{ ";",":",")","button_key_21",},
	{ "\'","\"","_","button_key_22",},
	{ "\\","|","+","button_key_23",},
	{ "z","Z","~","button_key_24",},
	{ "x","X","`","button_key_25",},
	{ "c","C","\"","button_key_26",},
	{ "v","V","\\","button_key_27",},
	{ "b","B","/","button_key_28",},
	{ "n","N","|","button_key_29",},
	{ "m","M",".","button_key_30", },
	{ ",","<","<","button_key_31",},
	{ ".",">",">","button_key_32",},
	{ "/","?","?","button_key_33",},
	{ "English","English","English","button_key_34",},
	{ "a/A","A/a","A/a","button_key_35",},
	{ "123","123","123","button_key_36",},
	{ "\0","\0","\0","button_key_37",},
	{ "\x20","\x20","\x20","button_key_38",},
	{ "\0","\0","\0","button_key_39",},
	{ "OK","OK","OK","button_key_40",},
	{ "\0","\0","\0","button_key_41",},
#if WEBLET_SUPPORT
	{ "QR Code","QR Code","QR Code","button_key_42",},
#else
	{ "Clr(F1)","Clr(F1)","Clr(F1)","button_key_42",},
#endif
};
#else
char *s_english_keymap[KEY_COUNT][4] = {{0}};
#endif

#ifdef KB_LANG_ARABIC
char* s_arabic_keymap[KEY_COUNT][2] = {
	{"\xd8\xa1","\xd9\xb1",},//"\x11\x06\x21\0", //  \x11\x06\x49\0
	{"\xd8\xa2","\xd9\x8a",},//"\x11\x06\x22\0", "\x11\x06\x4a\0",
	{"\xd8\xa3","\xd9\x8b",},//"\x11\x06\x23\0","\x11\x06\x4b\0",
	{"\xd8\xa4","\xd9\x8c",},//"\x11\x06\x24\0","\x11\x06\x4c\0",
	{"\xd8\xa5","\xd9\x8d",},//"\x11\x06\x25\0","\x11\x06\x4d\0",
	{"\xd8\xa6","\xd9\x8e",},//"\x11\x06\x26\0","\x11\x06\x4e\0",
	{"\xd8\xa7","\xd9\x8f",},//"\x11\x06\x27\0","\x11\x06\x4f\0",
	{"\xd8\xa8","\xd9\x80",}, //"\x11\x06\x28\0""\x11\x06\x40\0",
	{"\xd8\xa9","\xd9\x90",}, //"\x11\x06\x29\0","\x11\x06\x50\0",
	{"\xd8\xaa","\xd9\x91",},//"\x11\x06\x2A\0","\x11\x06\x51\0",
	{"\xd8\xab","\xd9\x92",}, //"\x11\x06\x2B\0","\x11\x06\x52\0",},
	{"\xd8\xac","\xd9\x93",},//"\x11\x06\x2C\0""\x11\x06\x53\0",
	{"\xd8\xad","\xd9\x94",},//"\x11\x06\x2D\0","\x11\x06\x54\0"
	{"\xd8\xae","\xd9\x95",},//"\x11\x06\x2E\0","\x11\x06\x55\0",
	{"\xd8\xaf","\xd9\xa0",},//"\x11\x06\x2F\0","\x11\x06\x60\0",
	{"\xd8\xb0","\xd9\xa1",},//"\x11\x06\x30\0","\x11\x06\x61\0",
	{"\xd8\xb1","\xd9\xa2",},//"\x11\x06\x31\0","\x11\x06\x62\0",
	{"\xd8\xb2","\xd9\xa3",},//"\x11\x06\x32\0","\x11\x06\x63\0",
	{"\xd8\xb3","\xd9\xa4",},//"\x11\x06\x33\0","\x11\x06\x64\0",
	{"\xd8\xb4","\xd9\xa5",},//"\x11\x06\x34\0","\x11\x06\x65\0"
	{"\xd8\xb5","\xd9\xa6",},//"\x11\x06\x35\0","\x11\x06\x66\0",
	{"\xd8\xb6","\xd9\xa7",},//"\x11\x06\x36\0","\x11\x06\x67\0",
	{"\xd8\xb7","\xd9\xa8",},//"\x11\x06\x37\0","\x11\x06\x68\0",
	{"\xd8\xb8","\xd9\xa9",},//"\x11\x06\x38\0","\x11\x06\x75\0",
	{"\xd8\xb9","\xd9\xb2",},//"\x11\x06\x39\0","\x11\x06\x76\0",
	{"\xd8\xba","\xd9\xb3",}, //"\x11\x06\x3A\0","\x11\x06\x77\0",
	{"\xd9\x81","\xd9\xb8",},//"\x11\x06\x41\0","\x11\x06\x78\0",
	{"\xd9\x82","\xd9\xb9",},//"\x11\x06\x42\0","\x11\x06\x79\0",
	{"\xd9\x83","\xd9\xba",},//"\x11\x06\x43\0","\x11\x06\x7a\0",
	{"\xd9\x84","\xd9\xbb",},//"\x11\x06\x44\0","\x11\x06\x7b\0",
	{"\xd9\x85","\xd9\xbc",},//"\x11\x06\x45\0","\x11\x06\x7c\0",
	{"\xd9\x86","\xd9\xbd",}, //"\x11\x06\x46\0","\x11\x06\x7d\0",
	{"\xd9\x87","\xd9\xbe",}, //"\x11\x06\x47\0","\x11\x06\x7e\0",
	{"\xd9\x88","\xd9\xbf",},//"\x11\x06\x48\0","\x11\x06\x7f\0",
	{"Arabic","Arabic",},
	{"1/2\0","2/2\0",},  //{"A/a\0","\x11\x06\x49\0",},
	{"123\0","123\0",},
	{"\0","\0",},//{"\xd9\x88","\x11\x06\x48\0",},
	{"\x20","\x20", },
	{"\0","\0",},//{"\xd9\x8a","\x11\x06\x4a\0",},
	{"OK\0","OK\0",},
	{"\0","\0",},
#if WEBLET_SUPPORT
	{"QR Code","QR Code",}
#else
	{ "Clr(F1)","Clr(F1)",}
#endif
};
#else
char* s_arabic_keymap[KEY_COUNT][2] = {{0}};
#endif
#ifdef KB_LANG_FARSI
char* s_persian_keymap[KEY_COUNT][2] = {
	{"\xd8\xa1","\xd9\xb1",},//"\x11\x06\x21\0", //  \x11\x06\x49\0
	{"\xd8\xa2","\xd9\x8a",},//"\x11\x06\x22\0", "\x11\x06\x4a\0",
	{"\xdb\x8c","\xd9\x8b",},//"\x11\x06\x23\0","\x11\x06\x4b\0",
	{"\xda\xaf","\xd9\x8c",},//"\x11\x06\x24\0","\x11\x06\x4c\0",
	{"\xda\x86","\xd9\x8d",},//"\x11\x06\x25\0","\x11\x06\x4d\0",
	{"\xda\x98","\xd9\x8e",},//"\x11\x06\x26\0","\x11\x06\x4e\0",
	{"\xd8\xa7","\xd9\x8f",},//"\x11\x06\x27\0","\x11\x06\x4f\0",
	{"\xd8\xa8","\xd9\x80",}, //"\x11\x06\x28\0""\x11\x06\x40\0",
	{"\xd9\xbe","\xd9\x90",}, //"\x11\x06\x29\0","\x11\x06\x50\0",
	{"\xd8\xaa","\xd9\x91",},//"\x11\x06\x2A\0","\x11\x06\x51\0",
	{"\xd8\xab","\xd9\x92",}, //"\x11\x06\x2B\0","\x11\x06\x52\0",},
	{"\xd8\xac","\xd9\x93",},//"\x11\x06\x2C\0""\x11\x06\x53\0",
	{"\xd8\xad","\xd9\x94",},//"\x11\x06\x2D\0","\x11\x06\x54\0"
	{"\xd8\xae","\xd9\x95",},//"\x11\x06\x2E\0","\x11\x06\x55\0",
	{"\xd8\xaf","\xd9\xa0",},//"\x11\x06\x2F\0","\x11\x06\x60\0",
	{"\xd8\xb0","\xd9\xa1",},//"\x11\x06\x30\0","\x11\x06\x61\0",
	{"\xd8\xb1","\xd9\xa2",},//"\x11\x06\x31\0","\x11\x06\x62\0",
	{"\xd8\xb2","\xd9\xa3",},//"\x11\x06\x32\0","\x11\x06\x63\0",
	{"\xd8\xb3","\xd9\xa4",},//"\x11\x06\x33\0","\x11\x06\x64\0",
	{"\xd8\xb4","\xd9\xa5",},//"\x11\x06\x34\0","\x11\x06\x65\0"
	{"\xd8\xb5","\xd9\xa6",},//"\x11\x06\x35\0","\x11\x06\x66\0",
	{"\xd8\xb6","\xd9\xa7",},//"\x11\x06\x36\0","\x11\x06\x67\0",
	{"\xd8\xb7","\xd9\xa8",},//"\x11\x06\x37\0","\x11\x06\x68\0",
	{"\xd8\xb8","\xd9\xa9",},//"\x11\x06\x38\0","\x11\x06\x75\0",
	{"\xd8\xb9","\xd9\xb2",},//"\x11\x06\x39\0","\x11\x06\x76\0",
	{"\xd8\xba","\xd9\xb3",}, //"\x11\x06\x3A\0","\x11\x06\x77\0",
	{"\xd9\x81","\xd9\xb8",},//"\x11\x06\x41\0","\x11\x06\x78\0",
	{"\xd9\x82","\xd8\xa9",},//"\x11\x06\x42\0","\x11\x06\x79\0",
	{"\xd9\x83","\xd9\xba",},//"\x11\x06\x43\0","\x11\x06\x7a\0",
	{"\xd9\x84","\xd9\xbb",},//"\x11\x06\x44\0","\x11\x06\x7b\0",
	{"\xd9\x85","\xd8\xa6",},//"\x11\x06\x45\0","\x11\x06\x7c\0",
	{"\xd9\x86","\xd8\xa5",}, //"\x11\x06\x46\0","\x11\x06\x7d\0",
	{"\xd9\x87","\xd8\xa4",}, //"\x11\x06\x47\0","\x11\x06\x7e\0",
	{"\xd9\x88","\xd8\xa3",},//"\x11\x06\x48\0","\x11\x06\x7f\0",
	{"Farsi","Farsi",},
	{"1/2\0","2/2\0",},  //{"A/a\0","\x11\x06\x49\0",},
	{"123\0","123\0",},
	{"\0","\0",},//{"\xd9\x88","\x11\x06\x48\0",},
	{"\x20","\x20", },
	{"\0","\0",},//{"\xd9\x8a","\x11\x06\x4a\0",},
	{"OK\0","OK\0",},
	{"\0","\0",},
#if WEBLET_SUPPORT
	{"QR Code","QR Code",}
#else
	{ "Clr(F1)","Clr(F1)",}
#endif
};
#else
char* s_persian_keymap[KEY_COUNT][2] = {{0}};
#endif

#ifdef KB_LANG_VIETNAMESE
char* s_vietnamese_keymap[KEY_COUNT][2] = {
	{"\x61","\x41",},
	{"\xc4\x83","\xc4\x82",},
	{"\xc3\xa2","\xc3\x82",},
	{"\x62","\x42",},
	{"\x63","\x43",},
	{"\x64","\x44",},
	{"\xc4\x91","\xc4\x90",},
	{"\x65","\x45",},
	{"\xc3\xaa","\xc3\x8a",},
	{"\x67","\x47",},//"g,G"
	{"\x68","\x48",},
	{"\x69","\x49",},
	{"\x6b","\x4b",},
	{"\x6c","\x4c",},
	{"\x6d","\x4d",},
	{"\x6e","\x4e",},
	{"\x6f","\x4f",},//"o,O"
	{"\xc3\xb4","\xc3\x94",},
	{"\xc6\xa1","\xc6\xa0",},
	{"\x70","\x50",},
	{"\x71","\x51",},
	{"\x72","\x52",},
	{"\x73","\x53",},
	{"\x74","\x54",},
	{"\x75","\x55",},
	{"\xc6\xb0","\xc6\xaf",},
	{"\x76","\x56",},
	{"\x78","\x58",},
	{"\x79","\x59",},
	{"(","(",},
	{")",")",},
	{".",".",},
	{"#","#",},
	{"$","$",},
	{"Vietnamese","Vietnamese",},
	{"a/A\0","A/a\0",},
	{"123\0","123\0",},
	{"\0","\0",},
	{"\x20","\x20", },
	{"\0","\0",},
	{"OK\0","OK\0",},
	{"\0","\0",},
#if WEBLET_SUPPORT
	{"QR Code","QR Code",}
#else
	{ "Clr(F1)","Clr(F1)",}
#endif
};
#else
char* s_vietnamese_keymap[KEY_COUNT][2] = {{0}};
#endif

#ifdef KB_LANG_RUSSIAN
char* s_russian_keymap[KEY_COUNT][2] = {
	{"\xd0\xB0","\xd0\x90",},
	{"\xd0\xB1","\xd0\x91",},
	{"\xd0\xB2","\xd0\x92",},
	{"\xd0\xB3","\xd0\x93",},
	{"\xd0\xb4","\xd0\x94",},
	{"\xd0\xb5","\xd0\x95",},
	{"\xd0\xb6","\xd0\x96",},
	{"\xd0\xb7","\xd0\x97",},
	{"\xd0\xb8","\xd0\x98",},
	{"\xd0\xb9","\xd0\x99",},
	{"\xd0\xba","\xd0\x9a",},
	{"\xd0\xbb","\xd0\x9b",},
	{"\xd0\xbc","\xd0\x9c",},
	{"\xd0\xbd","\xd0\x9d",},
	{"\xd0\xbe","\xd0\x9e",},
	{"\xd0\xbf","\xd0\x9f",},
	{"\xd1\x80","\xd0\xa0",},
	{"\xd1\x81","\xd0\xa1",},
	{"\xd1\x82","\xd0\xa2",},
	{"\xd1\x83","\xd0\xa3",},
	{"\xd1\x84","\xd0\xa4",},
	{"\xd1\x85","\xd0\xa5",},
	{"\xd1\x86","\xd0\xa6",},
	{"\xd1\x87","\xd0\xa7",},
	{"\xd1\x88","\xd0\xa8",},
	{"\xd1\x89","\xd0\xa9",},
	{"\xd1\x8a","\xd0\xaa",},
	{"\xd1\x8b","\xd0\xab",},
	{"\xd1\x8c","\xd0\xac",},
	{"\xd1\x8d","\xd0\xad",},
	{"\xd1\x8e","\xd0\xae",},
	{"\xd1\x8f","\xd0\xaf",},
	{"\xd1\x91","\xd0\x81",},
	{"\xd1\x94","\xd0\x84",},
	{"Russian","Russian",},
	{"a/A\0","A/a\0",},
	{"123\0","123\0",},
	{"\0","\0",},
	{"\x20","\x20", },
	{"\0","\0",},
	{"OK\0","OK\0",},
	{"\0","\0",},
#if WEBLET_SUPPORT
	{"QR Code","QR Code",}
#else
	{ "Clr(F1)","Clr(F1)",}
#endif
};
#else
char* s_russian_keymap[KEY_COUNT][2] = {{0}};
#endif

#ifdef KB_LANG_TURKISH
char* s_turkish_keymap[KEY_COUNT][2] = {
	{"\xc3\xa0","\xc3\x80",},
	{"\xc3\xa1","\xc3\x81",},
	{"\xc3\xa2","\xc3\x82",},
	{"\xc3\xa3","\xc3\x83",},
	{"\xc3\xa4","\xc3\x84",},
	{"\xc3\xa5","\xc3\x85",},
	{"\xc3\xa6","\xc3\x86",},
	{"\xc3\xa7","\xc3\x87",},
	{"\xc3\xa8","\xc3\x88",},
	{"\xc3\xa9","\xc3\x89",},
	{"\xc3\xaa","\xc3\x8a",},
	{"\xc3\xab","\xc3\x8b",},
	{"\xc3\xac","\xc3\x8c",},
	{"\xc3\xad","\xc3\x8d",},
	{"\xc3\xae","\xc3\x8e",},
	{"\xc3\xaf","\xc3\x8f",},
	{"\xc4\x9f","\xc4\x9e",},
	{"\xc3\xb1","\xc3\x91",},
	{"\xc3\xb2","\xc3\x92",},
	{"\xc3\xb3","\xc3\x93",},
	{"\xc3\xb4","\xc3\x94",},
	{"\xc3\xb5","\xc3\x95",},
	{"\xc3\xb6","\xc3\x96",},
	{"\xc3\xb7","\xc3\x97",},
	{"\xc3\xb8","\xc3\x98",},
	{"\xc3\xb9","\xc3\x99",},
	{"\xc3\xba","\xc3\x9a",},
	{"\xc3\xbb","\xc3\x9b",},
	{"\xc3\xbc","\xc3\x9c",},
	{"\xc4\xb1","\xc4\xb0",},
	{"\xc5\x9f","\xc5\x9e",},
	{"\xc3\xbe","\xc3\x9f",},
	{"\xc5\xa1","\xc5\xa0",},
	{"\xc5\x93","\xc5\x92",},
	//{"\xc2\xa1","\xc5\xb8",},
	{"Turkish","Turkish",},
	{"a/A\0","A/a\0",},
	{"123\0","123\0",},
	{"\0","\0",},
	{"\x20","\x20", },
	{"\0","\0",},
	{"OK\0","OK\0",},
	{"\0","\0",},
#if WEBLET_SUPPORT
	{"QR Code","QR Code",}
#else
	{ "Clr(F1)","Clr(F1)",}
#endif
};
#else
char* s_turkish_keymap[KEY_COUNT][2] = {{0}};
#endif

#ifdef KB_LANG_UKRAINIAN
char* s_ukrainian_keymap[KEY_COUNT][2] = {
	{"\xd0\xb0","\xd0\x90",},
	{"\xd0\xb1","\xd0\x91",},
	{"\xd0\xb2","\xd0\x92",},
	{"\xd0\xb3","\xd0\x93",},
	{"\xd2\x91","\xd2\x90",},
	{"\xd0\xb4","\xd0\x94",},
	{"\xd0\xb5","\xd0\x95",},
	{"\xd1\x94","\xd0\x84",},
	{"\xd0\xb6","\xd0\x96",},
	{"\xd0\xb7","\xd0\x97",},
	{"\xd0\xb8","\xd0\x98",},
	{"\xd1\x96","\xd0\x86",},
	{"\xd1\x97","\xd0\x87",},
	{"\xd0\xb9","\xd0\x99",},
	{"\xd0\xba","\xd0\x9a",},
	{"\xd0\xbb","\xd0\x9b",},
	{"\xd0\xbc","\xd0\x9c",},
	{"\xd0\xbd","\xd0\x9d",},
	{"\xd0\xbe","\xd0\x9e",},
	{"\xd0\xbf","\xd0\x9f",},
	{"\xd1\x80","\xd0\xa0",},
	{"\xd1\x81","\xd0\xa1",},
	{"\xd1\x82","\xd0\xa2",},
	{"\xd1\x83","\xd0\xa3",},
	{"\xd1\x84","\xd0\xa4",},
	{"\xd1\x85","\xd0\xa5",},
	{"\xd1\x86","\xd0\xa6",},
	{"\xd1\x87","\xd0\xa7",},
	{"\xd1\x88","\xd0\xa8",},
	{"\xd1\x89","\xd0\xa9",},
	{"\xd1\x8c","\xd0\xac",},
	{"\xd1\x8e","\xd0\xae",},
	{"\xd1\x8f","\xd0\xaf",},
	{".",".",},
	{"Ukrainian","Ukrainian",},
	{"a/A\0","A/a\0",},
	{"123\0","123\0",},
	{"\0","\0",},
	{"\x20","\x20", },
	{"\0","\0",},
	{"OK\0","OK\0",},
	{"\0","\0",},
#if WEBLET_SUPPORT
	{"QR Code","QR Code",}
#else
	{ "Clr(F1)","Clr(F1)",}
#endif
};
#else
char* s_ukrainian_keymap[KEY_COUNT][2] = {{0}};
#endif

#ifdef KB_LANG_THAI
char* s_thailand_keymap[KEY_COUNT][3] = {
	{"\xe0\xb9\x86","\xe0\xb8\xa0","\x28"},//q
	{"\xe0\xb9\x84","\xe0\xb8\x96","\x29"},//w,2
	{"\xe0\xb8\xb3","\xe0\xb8\x8e","\x2c"},//e
	{"\xe0\xb8\x9e","\xe0\xb8\x91","\xe0\xb9\x85"},//r
	{"\xe0\xb8\xb0","\xe0\xb8\x98","\xe0\xb8\xbf"},//rxe0\xb8\xb6"},//t
	{"\xe0\xb8\xb1","\xe0\xb8\xb6","\x2e"},//4(y)
	{"\xe0\xb8\xb5","\xe0\xb9\x8a","\0"},//5(u)
	{"\xe0\xb8\xa3","\xe0\xb8\x93","\0"},//i
	{"\xe0\xb8\x99","\xe0\xb8\xaf","\0"},//o
	{"\xe0\xb8\xa2","\xe0\xb8\x8d","\0"},//p
	{"\xe0\xb8\x9a","\xe0\xb8\x90","\0"},//[
	{"\xe0\xb8\xa5","\xe0\xb8\x82","\0"},//],3
	{"\xe0\xb8\x9f","\xe0\xb8\xa4","\xe0\xb9\x91"},//a
	{"\xe0\xb8\xab","\xe0\xb8\x86","\xe0\xb9\x92"},//s
	{"\xe0\xb8\x81","\xe0\xb8\x8f","\xe0\xb9\x93"},//d
	{"\xe0\xb8\x94","\xe0\xb9\x82","\xe0\xb9\x94"},//f
	{"\xe0\xb9\x80","\xe0\xb8\x8c","\xe0\xb9\x95"},//g
	{"\xe0\xb9\x89","\xe0\xb9\x87","\xe0\xb9\x96"},//8(h)
	{"\xe0\xb9\x88","\xe0\xb9\x8b","\xe0\xb9\x97"},//9(j)
	{"\xe0\xb8\xb2","\xe0\xb8\xa9","\xe0\xb9\x98"},//k
	{"\xe0\xb8\xaa","\xe0\xb8\xa8","\xe0\xb9\x99"},//L
	{"\xe0\xb8\xa7","\xe0\xb8\x8b","\xe0\xb9\x90"},//;
	{"\xe0\xb8\x87","\xe0\xb8\x84","\0"},//',7
	{"\xe0\xb8\x8a","\xe0\xb8\x95","\0"},//=,+
	{"\xe0\xb8\x9c","\xe0\xb8\xb8","\0"},//z,/,
	{"\xe0\xb8\x9b","\xe0\xb8\xb9","\0"},//x,<
	{"\xe0\xb9\x81","\xe0\xb8\x89","\0"},//c
	{"\xe0\xb8\xad","\xe0\xb8\xae",},//v
	{"\xe0\xb8\xb4","\xe0\xb8\x88","\0"},//0(b)
	{"\xe0\xb8\xb7","\xe0\xb9\x8c","\0"},//-(n)
	{"\xe0\xb8\x97","\x3f","\0"},//m,>
	{"\xe0\xb8\xa1","\xe0\xb8\x92","\0"},//,
	{"\xe0\xb9\x83","\xe0\xb8\xac","\0"},//.
	{"\xe0\xb8\x9d","\xe0\xb8\xa6","\0"},///
	{"Thai","Thai","Thai"},
	{"1/3\0","2/3\0","3/3\0"},
	{"123\0","123\0","123\0"},
	{"\0","\0","\0"},
	{"\x20","\x20", "\x20"},
	{"\0","\0","\0"},
	{"OK\0","OK\0","OK\0"},
	{"\0","\0","\0"},
#if WEBLET_SUPPORT
	{"QR Code","QR Code","QR Code",}
#else
	{"Clr(F1)","Clr(F1)","Clr(F1)"}
#endif
};
#else
char* s_thailand_keymap[KEY_COUNT][3] = {{0}};
#endif

#ifdef KB_LANG_PORTUGUESE
char *s_portuguese_keymap[KEY_COUNT][5] = {
	{ "q","Q","\xc3\x80","1","button_key_0",},
	{ "w","W","\xc3\x83","2","button_key_1",},
	{ "e","E","\xc3\x87","3","button_key_2",},
	{ "r","R","\xc3\x8a","4","button_key_3",},
	{ "t","T","\xc3\x89","5","button_key_4",},
	{ "y","Y","\xc3\x95","6","button_key_5",},
	{ "u","U","\xc3\xa0","7","button_key_6",},
	{ "i","I","\xc3\xa3","8","button_key_7",},
	{ "o","O","\xc3\xa7","9","button_key_8",},
	{ "p","P","\xc3\xaa","0","button_key_9",},
	{ "[","{","\xc3\xb5","-","button_key_10",},
	{ "]","}","\xc3\xa1","=","button_key_11",},
	{ "a","A","\xc3\xa9","!","button_key_12",},
	{ "s","S","\xc3\xad","@","button_key_13",},
	{ "d","D","\xc3\xb3","#","button_key_14",},
	{ "f","F","\xc3\xba","$","button_key_15",},
	{ "g","G","\x20\x20","%","button_key_16",},
	{ "h","H","\x20\x20","^","button_key_17",},
	{ "j","J","\x20\x20","&","button_key_18",},
	{ "k","K","\x20\x20","*","button_key_19",},
	{ "l","L","\x20\x20","(","button_key_20",},
	{ ";",":","\x20\x20",")","button_key_21",},
	{ "\'","\"","\x20\x20","_","button_key_22",},
	{ "\\","|","\x20\x20","+","button_key_23",},
	{ "z","Z","\x20\x20","~","button_key_24",},
	{ "x","X","\x20\x20","`","button_key_25",},
	{ "c","C","\x20\x20","\"","button_key_26",},
	{ "v","V","\x20\x20","\\","button_key_27",},
	{ "b","B","\x20\x20","/","button_key_28",},
	{ "n","N","\x20\x20","|","button_key_29",},
	{ "m","M","\x20\x20",".","button_key_30", },
	{ ",","<","\x20\x20","<","button_key_31",},
	{ ".",">","\x20\x20",">","button_key_32",},
	{ "/","?","\x20\x20","?","button_key_33",},
	{ "Portuguese","Portuguese","Portuguese","Portuguese","button_key_34",},
	{ "1/3","2/3","3/3","1/3","button_key_35",},
	{ "123","123","123","123","button_key_36",},
	{ "","","","","button_key_37",},
	{ "\x20","\x20","\x20","\x20","button_key_38",},
	{ "","","","","button_key_39",},
	{ "OK","OK","OK","OK","button_key_40",},
	{ "","","","","button_key_41",},
	{ "Clr(F1)","Clr(F1)","Clr(F1)","Clr(F1)","button_key_42",},
};
#else
char *s_portuguese_keymap[KEY_COUNT][5] = {{0}};
#endif

#ifdef KB_LANG_SPANISH
char *s_spanish_keymap[KEY_COUNT][5] = {
	{ "q","Q","\xc3\xb1","1","button_key_0",},
	{ "w","W","\xc3\xa1","2","button_key_1",},
	{ "e","E","\xc3\xa7","3","button_key_2",},
	{ "r","R","\xc3\xa9","4","button_key_3",},
	{ "t","T","\xc3\xad","5","button_key_4",},
	{ "y","Y","\xc3\xb3","6","button_key_5",},
	{ "u","U","\xc3\xba","7","button_key_6",},
	{ "i","I","\xc3\xbc","8","button_key_7",},
	{ "o","O","\xc3\x91","9","button_key_8",},
	{ "p","P","\xc3\x81","0","button_key_9",},
	{ "[","{","\xc3\x87","-","button_key_10",},
	{ "]","}","\xc3\x89","=","button_key_11",},
	{ "a","A","\xc3\x8d","!","button_key_12",},
	{ "s","S","\xc3\x93","@","button_key_13",},
	{ "d","D","\xc3\x9a","#","button_key_14",},
	{ "f","F","\xc3\x9c","$","button_key_15",},
	{ "g","G","\xc2\xbf","%","button_key_16",},
	{ "h","H","\xc2\xa1","^","button_key_17",},
	{ "j","J","\x6e\xc2\xb0","&","button_key_18",},
	{ "k","K","\xe2\x82\xac","*","button_key_19",},
	{ "l","L","\xe2\x80\xa6","(","button_key_20",},
	{ ";",":","\x20\x20",")","button_key_21",},
	{ "\'","\"","\x20\x20","_","button_key_22",},
	{ "\\","|","\x20\x20","+","button_key_23",},
	{ "z","Z","\x20\x20","~","button_key_24",},
	{ "x","X","\x20\x20","`","button_key_25",},
	{ "c","C","\x20\x20","\"","button_key_26",},
	{ "v","V","\x20\x20","\\","button_key_27",},
	{ "b","B","\x20\x20","/","button_key_28",},
	{ "n","N","\x20\x20","|","button_key_29",},
	{ "m","M","\x20\x20",".","button_key_30", },
	{ ",","<","\x20\x20","<","button_key_31",},
	{ ".",">","\x20\x20",">","button_key_32",},
	{ "/","?","\x20\x20","?","button_key_33",},
	{ "Spanish","Spanish","Spanish","Spanish","button_key_34",},
	{ "1/3","2/3","3/3","1/3","button_key_35",},
	{ "123","123","123","123","button_key_36",},
	{ "","","","","button_key_37",},
	{ "\x20","\x20","\x20","\x20","button_key_38",},
	{ "","","","","button_key_39",},
	{ "OK","OK","OK","OK","button_key_40",},
	{ "","","","","button_key_41",},
	{ "Clr(F1)","Clr(F1)","Clr(F1)","Clr(F1)","button_key_42",},
};
#else
char *s_spanish_keymap[KEY_COUNT][5] = {{0}};
#endif

#ifdef KB_LANG_BULGARAIN
char* s_bulgarain_keymap[KEY_COUNT][2] = {
	{"\xd1\x8b",",",},
	{"\xd1\x83","\xd0\xa3",},
	{"\xd0\xb5","\xd0\x95",},
	{"\xd0\xb8","\xd0\x98",},
	{"\xd1\x88","\xd0\xa8",},
	{"\xd1\x89","\xd0\xa9",},
	{"\xd0\xba","\xd0\x9a",},
	{"\xd1\x81","\xd0\xa1",},
	{"\xd0\xb4","\xd0\x94",},
	{"\xd0\xb7","\xd0\x97",},
	{"\xd1\x86","\xd0\xa6",},
	{"\xc2\xa7",";",},
	{")","(",},
	{"\xd1\x8c","\xd0\xac",},
	{"\xd1\x8f","\xd0\xaf",},
	{"\xd0\xb0","\xd0\x90",},
	{"\xd0\xbe","\xd0\x9e",},
	{"\xd0\xb6","\xd0\x96",},
	{"\xd0\xb3","\xd0\x93",},
	{"\xd1\x82","\xd0\xa2",},
	{"\xd0\xbd","\xd0\x9d",},
	{"\xd0\xb2","\xd0\x92",},
	{"\xd0\xbc","\xd0\x9c",},
	{"\xd1\x87","\xd0\xa7",},
	{"\xd1\x8e","\xd0\xae",},
	{"\xd0\xb9","\xd0\x99",},
	{"\xd1\x8a","\xd0\xaa",},
	{"\xd1\x8d","\xd0\xad",},
	{"\xd1\x84","\xd0\xa4",},
	{"\xd1\x85","\xd0\xa5",},
	{"\xd0\xbf","\xd0\x9f",},
	{"\xd1\x80","\xd0\xa0",},
	{"\xd0\xbb","\xd0\x9b",},
	{"\xd0\x91","\xd0\xb1",},
	{"Bulgarain","Bulgarain",},
	{"a/A\0","A/a\0",},
	{"123\0","123\0",},
	{"\0","\0",},
	{"\x20","\x20", },
	{"\0","\0",},
	{"OK\0","OK\0",},
	{"\0","\0",},
#if WEBLET_SUPPORT
	{"QR Code","QR Code",}
#else
	{ "Clr(F1)","Clr(F1)",}
#endif
};
#else
char* s_bulgarain_keymap[KEY_COUNT][2] = {{0}};
#endif

#ifdef KB_LANG_POLSKI
char* s_polski_keymap[KEY_COUNT][2] = {
	{"\x61","\x41",},
	{"\xc4\x85","\xc4\x84",},
	{"\x62","\x42",},
	{"\x63","\x43",},
	{"\xc4\x87","\xc4\x86",},
	{"\x64","\x44",},
	{"\x65","\x45",},
	{"\xc4\x99","\xc4\x98",},
	{"\x66","\x46",},
	{"\x67","\x47",},
	{"\x68","\x48",},
	{"\x69","\x49",},
	{"\x6a","\x4a",},
	{"\x6b","\x4b",},
	{"\x6c","\x4c",},
	{"\xc5\x82","\xc5\x81",},
	{"\x6d","\x4d",},
	{"\x6e","\x4e",},
	{"\xc5\x84","\xc5\x83",},
	{"\x6f","\x4f",},
	{"\xc3\xb3","\xc3\x93",},
	{"\x70","\x50",},
	{"\x72","\x52",},
	{"\x73","\x53",},
	{"\xc5\x9b","\xc5\x9a",},
	{"\x74","\x54",},
	{"\x75","\x55",},
	{"\x77","\x57",},
	{"\x79","\x59",},
	{"\x7a","\x5a",},
	{"\xc5\xba","\xc5\xb9",},
	{"\xc5\xbc","\xc5\xbb",},
	{"(","(",},
	{")",")",},
	{"Polski","Polski",},
	{"a/A\0","A/a\0",},
	{"123\0","123\0",},
	{"\0","\0",},
	{"\x20","\x20", },
	{"\0","\0",},
	{"OK\0","OK\0",},
	{"\0","\0",},
#if WEBLET_SUPPORT
	{"QR Code","QR Code",}
#else
	{ "Clr(F1)","Clr(F1)",}
#endif
};
#else
char* s_polski_keymap[KEY_COUNT][2] = {{0}};
#endif

