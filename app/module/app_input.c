/*
 * =====================================================================================
 *
 *       Filename:  app_input.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2011年08月19日 10时54分55秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "gxcore.h"
#include "app_key.h"
#include "app_module.h"

#define INPUT_KEY_LEFT      STBK_LEFT
#define INPUT_KEY_RIGHT     STBK_RIGHT
#define INPUT_KEY_DEL       STBK_GREEN
#define INPUT_KEY_CAPS      STBK_RED
#define INPUT_KEY_NUMBER STBK_BLUE
#define INPUT_KEY_1         STBK_1
#define INPUT_KEY_2         STBK_2
#define INPUT_KEY_3         STBK_3
#define INPUT_KEY_4         STBK_4
#define INPUT_KEY_5         STBK_5
#define INPUT_KEY_6         STBK_6
#define INPUT_KEY_7         STBK_7
#define INPUT_KEY_8         STBK_8
#define INPUT_KEY_9         STBK_9
#define INPUT_KEY_0         STBK_0

char *s_InputChar_lower[INPUT_KEY_NUM] =
{
    "_0",
    "()-1",
    "abc2",
    "def3",
    "ghi4",
    "jkl5",
    "mno6",
    "pqrs7",
    "tuv8",
    "wxyz9",
};

char *s_InputChar_caps[INPUT_KEY_NUM] =
{
    " 0",
    "@*.1",
    "ABC2",
    "DEF3",
    "GHI4",
    "JKL5",
    "MNO6",
    "PQRS7",
    "TUV8",
    "WXYZ9",
};

char s_InputNum[INPUT_KEY_NUM] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

static uint32_t input_numkey_filter(uint32_t key)
{
    uint32_t     i=0;
    uint32_t    input_num_key[INPUT_KEY_NUM] =
    {
        INPUT_KEY_0,
        INPUT_KEY_1,
        INPUT_KEY_2,
        INPUT_KEY_3,
        INPUT_KEY_4,
        INPUT_KEY_5,
        INPUT_KEY_6,
        INPUT_KEY_7,
        INPUT_KEY_8,
        INPUT_KEY_9
    };

    for (i=0; i<INPUT_KEY_NUM; i++)
    {
        if (input_num_key[i] == key)
        {
            return i;
        }
    }

    return key;
}

#define KEY2CHAR(key, caps)    (key>='a' && key<='z' && caps==1) ? (key-('a'-'A')) : key

void input_keypress(uint32_t key_value)
{
    uint32_t key = input_numkey_filter(key_value);
    AppInput *input = &g_AppInput;

    if (input->first_click == 0)
    {
        input->prev_key = key;
    }

    switch(key)
    {
        case INPUT_KEY_LEFT:
            if (input->cur_sel != 0)
            {
                input->cur_sel--;
                input->key_click = 0;
                input->first_click = 0;
            }
            break;

        case INPUT_KEY_RIGHT:
            if((input->cur_max > 0)
                && (input->cur_sel < input->cur_max - 1))
            {
                input->cur_sel++;
                input->key_click = 0;
                input->first_click = 0;
            }
            break;

        case INPUT_KEY_DEL:
            if (input->cur_sel < input->cur_max)
            {
                memcpy((void*)(input->buf+input->cur_sel),
                        (void*)(input->buf+input->cur_sel+1),
                        input->cur_max-input->cur_sel);
                input->key_click = 0;

                if(input->cur_max > 1)
                    input->cur_max--;

                if(input->cur_sel >= input->cur_max)
                {
                    input->cur_sel--;
                }
                input->first_click = 0;
            }
            break;

        case INPUT_KEY_CAPS:
            input->caps ^= 1;
            break;

        case INPUT_KEY_NUMBER:
            input->key_click = 0;
            input->first_click = 0;
            input->number^= 1;
            break;

        default:
            // 0-9
            if (key > INPUT_KEY_NUM-1)
                return;

            if(input->number == 0) //word mode
            {
            char **input_word = NULL;
	    input_word = (char**)GxCore_Malloc(INPUT_KEY_NUM*sizeof(char*));
            (input->caps == 0) ? (input_word = s_InputChar_lower) : (input_word = s_InputChar_caps);
            if (key == input->prev_key)
            {
                if (input->key_click==strlen(input_word[key])-1)
                {
                    input->key_click = 0;
                }
                else
                {
                    if (input->first_click == 0)
                    {
                        input->first_click = 1;
                        if(input->cur_max < input->max_len)
                        {
                            if((input->cur_sel == input->cur_max - 1)
                                ||(input->cur_sel == input->cur_max))
                            {
                                input->cur_max++;
                            }
                        }
                    }
                    else
                    {
                        input->key_click++;
                    }
                }

            }
            else
            {
                input->key_click = 0;

                if(input->cur_max < input->max_len)
                {
                    if(input->cur_sel < input->cur_max)
                    {
                        input->cur_sel++;
                        input->cur_max++;
                    }
                }
                else
                {
                     if(input->cur_sel < input->cur_max - 1)
                    {
                        input->cur_sel++;
                    }
                }

            }

            //input->buf[input->cur_sel] = KEY2CHAR(s_InputChar[key][input->key_click], input->caps);
            input->buf[input->cur_sel] = input_word[key][input->key_click];
	    if(input_word != NULL)
	    {
		    GxCore_Free(input_word);
	    }

            }
            else //number mode
            {
                input->buf[input->cur_sel] = s_InputNum[key];

                if(input->cur_sel < input->max_len - 1)
                {
                     input->cur_sel++;
                     if((input->cur_sel == input->cur_max)
                        && (input->cur_max < input->max_len))
                     {
                         input->cur_max++;
                     }
                }
            }

            break;
    }

    input->prev_key = key;
}

char* input_name_get(void)
{
    AppInput *input = &g_AppInput;

    return input->buf;
}

status_t input_init(char* in_name, uint32_t max_num)
{
    AppInput *input = &g_AppInput;
    size_t cpy_n = 0;

    memset(input->buf, 0, INPUT_NAME_MAX);
    input->first_click  = 0;
    input->prev_key     = INPUT_KEY_NUM;
    input->cur_sel      = 0;
    input->cur_max      = 0;
    input->key_click    = 0;      /* 单个键按了的次数，换键后清零 */
    input->caps         = 1;           /* 0."a-z"  1."A-Z" */
    input->number     = 0;      //0.word  1.number

    if (in_name == NULL)
    {
        cpy_n = 0;
    }
    else
    {
        cpy_n = strlen(in_name);
        if (cpy_n >= INPUT_NAME_MAX)
        {
            cpy_n = INPUT_NAME_MAX - 1;
        }
    }

    if((max_num >= INPUT_NAME_MAX)
        || (max_num == 0))
    {
        max_num = INPUT_NAME_MAX - 1;
    }

    input->max_len = (cpy_n > max_num) ? cpy_n : max_num;

    if(cpy_n ==  input->max_len)
    {
        input->cur_max = cpy_n;
    }
    else
    {
        input->cur_max = cpy_n + 1;
    }

    if (in_name != NULL)
    {
        strncpy(input->buf, in_name, cpy_n);
    }

    return GXCORE_SUCCESS;
}

AppInput g_AppInput =
{
    .init     = input_init,
    .keypress = input_keypress,
    .name_get = input_name_get
};

