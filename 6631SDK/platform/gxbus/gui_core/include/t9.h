#ifndef __T9_H__
    #define __T9_H__
#include "gui_private.h"
#include "widget.h"

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    STATE_BOX = 1,
    INPUT0_BOX,
    INPUT1_BOX,
    EDIT_BOX
};

typedef enum
{
    PINYIN_MODE,
    UPPER_CASE_MODE,
    LOWER_CASE_MODE,
    SYMBOL_MODE,
    DIGIT_MODE
}T9IputMode;

typedef enum
{
    SPELL_MODE,
    SELECT_MODE,
    CHARACTER_MODE
}T9WriteMode;

typedef struct _GuiT9
{
    GuiWidget *base;
    GuiWidget *state_text;
    GuiWidget *input0_text;
    GuiWidget *input1_text;
    GuiWidget *edit_text;
    char *catital_char;
    unsigned short start_code;
    unsigned short offset_code;
    int key[6];
    int key_index;
    int character_offset;
    int input_index;
    int symbol_page;
    T9IputMode input_mode;
    T9WriteMode write_mode;
    GuiWidgetSignal *create_event;
	GuiWidgetSignal *destroy_event;
	GuiWidgetSignal *keypress_event;
}GuiT9;

extern GuiWidgetOps t9_ops;
extern unsigned int hzk_index[437][43];
extern char *pStateString[5];
extern unsigned short pSymbolString[18];
extern unsigned char pCapitalValue[9][5];

#ifdef __cplusplus
}
#endif


#endif  /*__T9_H__*/


