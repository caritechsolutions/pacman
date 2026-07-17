#include "t9.h"

static void* create_t9(GuiWidget *self)
{
	GuiT9 *t9 = NULL;
	GuiWidget *parent = NULL, *child = NULL;
	unsigned int text_count = 0;

	if(NULL == self)
	{
		return (NULL);
	}

	t9 = (GuiT9 *)GUI_MALLOC(sizeof(GuiT9));
	if(NULL == t9)
	{
		return (NULL);
	}

	memset(t9, 0, sizeof(GuiT9));

	parent = widget_get_parent(self);
	WIDGET_ADJUST_ACCORDING_PARENT(parent, self);

	t9->base = self;
	self->object = (void *)t9;
	self->state = WIDGET_STATE_FOCUSSABLE;

	child = widget_get_firstchild(self);
	while(child)
	{
		if(0 == strcasecmp("text", child->classname))
		{
			text_count++;
		}

		if((0 != strcasecmp("text", child->classname)) &&
				(0 != strcasecmp("image", child->classname)))
		{
			break;
		}

		switch(text_count)
		{
		case STATE_BOX:
			t9->state_text = child;
			break;
		case INPUT0_BOX:
			t9->input0_text = child;
			break;
		case INPUT1_BOX:
			t9->input1_text = child;
			break;
		case EDIT_BOX:
			t9->edit_text = child;
			break;
		default:
			break;
		}
		widget_create(child);

		child = widget_get_nextsibling(child);
	}

	if(EDIT_BOX != text_count)
	{
		GUI_FREE(self->object);
		return (NULL);
	}

	GUI_SetProperty(t9->state_text->name,
			"string",
			pStateString[t9->input_mode]);
	GUI_SetProperty(t9->edit_text->name,
			"string",
			"");

	if(self->signal)
	{
		t9->create_event = (GuiWidgetSignal *)hash_get(self->signal, "create");
		t9->destroy_event = (GuiWidgetSignal *)hash_get(self->signal, "destroy");
		t9->keypress_event = (GuiWidgetSignal *)hash_get(self->signal, "keypress");
	}

	if(t9->create_event)
	{
		exec_widget_event_data(self, t9->create_event, NULL);
	}

	return (t9);
}

static int t9_release(GuiWidget *self)
{
	GuiT9 *t9 = NULL;
	GuiWidget *child = NULL;

	t9 = (GuiT9 *)(self->object);
	if(NULL == t9)
	{
		return (1);
	}

	child = widget_get_firstchild(self);
	while(child)
	{
		widget_release(child);
		child = widget_get_nextsibling(self);
	}

	GUI_FREE(self->object);

	return (0);
}

static int t9_draw(GuiWidget *self)
{
	GuiT9 *t9 = NULL;
	GuiWidget *child = NULL;

	t9 = (GuiT9 *)(self->object);
	if(NULL == t9)
	{
		return (1);
	}

	child = widget_get_firstchild(self);
	while(child)
	{
		if(child->need_update)
		{
			widget_draw(child);
			widget_set_update(child, GUI_FALSE);
		}
		child = widget_get_nextsibling(child);
	}

	return (0);
}

static int _check_key(int key)
{
	int c;
	switch(key)
	{
	case GUIK_2/*ABC*/:
		c = 1;
		break;
	case GUIK_3/*DEF*/:
		c = 2;
		break;
	case GUIK_4/*GHI*/:
		c = 3;
		break;
	case GUIK_5/*JKL*/:
		c = 4;
		break;
	case GUIK_6/*MNO*/:
		c = 5;
		break;
	case GUIK_7/*PQRS*/:
		c = 6;
		break;
	case GUIK_8/*TUV*/:
		c = 7;
		break;
	case GUIK_9/*WXYZ*/:
		c = 8;
		break;
	default:
		break;
	}

	return (c);
}

int _check_chinese_pinyin(GuiT9* t9)
{
#ifdef GUI_T9
	int i = 0;

	if(NULL == t9)
	{
		return (-1);
	}

	switch(t9->key_index)
	{
	case 0:
		for(i = 0; i < 403;i++)
		{
			if(hzk_index[i][0] == t9->key[0])
			{
				return (i);
			}
		}
		break;
	case 1:
		for(i = 0; i < 403;i++)
		{
			if((hzk_index[i][0] == t9->key[0]) &&
					(hzk_index[i][1] == t9->key[1]))
			{
				return (i);
			}
		}
		break;
	case 2:
		for(i = 0; i < 403;i++)
		{
			if((hzk_index[i][0] == t9->key[0]) &&
					(hzk_index[i][1] == t9->key[1]) &&
					(hzk_index[i][2] == t9->key[2]))
			{
				return (i);
			}
		}
		break;
	case 3:
		for(i = 0; i < 403;i++)
		{
			if(hzk_index[i][0] == t9->key[0] &&
					hzk_index[i][1] == t9->key[1] &&
					hzk_index[i][2] == t9->key[2] &&
					hzk_index[i][3] == t9->key[3])
			{
				return (i);
			}
		}
		break;
	case 4:
		for(i = 0; i < 403;i++)
		{
			if((hzk_index[i][0]==t9->key[0])&&
					(hzk_index[i][1]==t9->key[1])&&
					(hzk_index[i][2]==t9->key[2])&&
					(hzk_index[i][3]==t9->key[3])&&
					(hzk_index[i][4]==t9->key[4]))
			{
				return (i);
			}
		}
		break;
	case 5:
		for(i = 0; i < 403;i++)
		{
			if((hzk_index[i][0]==t9->key[0]) &&
					(hzk_index[i][1]==t9->key[1]) &&
					(hzk_index[i][2]==t9->key[2]) &&
					(hzk_index[i][3]==t9->key[3]) &&
					(hzk_index[i][4]==t9->key[4]) &&
					(hzk_index[i][5]==t9->key[5]))
			{
				return (i);
			}
		}
		break;
	default:
		break;
	}
#endif

	return (0);
}

static int _add_key(GuiWidget* self, GuiT9* t9, int key)
{
	int input_count = 0, input_index = 0, i = 0, j = 0, len = 0;
	unsigned short gb2312_code = 0;
	char *string_value = NULL, *pstr = NULL;
	char symbol_char[90] = {0}, *capital_char = NULL;

	if((NULL == self) || (NULL == t9))
	{
		return (1);
	}

	/*Switch the Input mode such as Pinyin, Uppper case Capital and Low case, symbol and digit*/
	if((GUIK_1 == key) && (SPELL_MODE == t9->write_mode))
	{
		if(DIGIT_MODE == t9->input_mode)
		{
			t9->input_mode = PINYIN_MODE;
		}
		else
		{
			t9->input_mode++;
		}

		if(SYMBOL_MODE == t9->input_mode)
		{
			GUI_SetProperty(t9->input0_text->name,
					"string",
					"");

			pstr = symbol_char;
			for(i = 0; i < 9; i++)
			{
				sprintf(pstr, "%d. ", i + 1);
				pstr += 3;
				*pstr = pSymbolString[i] & 0x00FF;
				pstr++;
				*pstr = pSymbolString[i] >> 8;
				pstr++;
				strcat(pstr, " ");
				pstr++;
			}
			*pstr = '\0';
			GUI_SetProperty(t9->input1_text->name,
					"string",
					symbol_char);
			t9->symbol_page = 0;
		}
		else
		{
			GUI_SetProperty(t9->input0_text->name,
					"string",
					"");
			GUI_SetProperty(t9->input1_text->name,
					"string",
					"");
		}

		GUI_SetProperty(t9->state_text->name,
				"string",
				pStateString[t9->input_mode]);
		self->need_update = GUI_TRUE;
		return (0);
	}

	switch(key)
	{
	case GUIK_0/*Space*/:
		if(DIGIT_MODE != t9->input_mode)
		{
			GUI_GetProperty(t9->edit_text->name,
					"string",
					&string_value);
			len = strlen(string_value);
			pstr = (char *)GUI_MALLOC(len + 10);
			strcpy(pstr, string_value);

			pstr[len] = ' ';
			pstr[len + 1] = '\0';

			GUI_SetProperty(t9->edit_text->name,
					"string",
					pstr);
			GUI_FREE(pstr);
			break;
		}
	case GUIK_1:
	case GUIK_2/*ABC*/:
	case GUIK_3/*DEF*/:
	case GUIK_4/*GHI*/:
	case GUIK_5/*JKL*/:
	case GUIK_6/*MNO*/:
	case GUIK_7/*PQRS*/:
	case GUIK_8/*TUV*/:
	case GUIK_9/*WXYZ*/:
		if(PINYIN_MODE == t9->input_mode)
		{
			if(SPELL_MODE == t9->write_mode)
			{
				t9->key[t9->key_index] = _check_key(key);
				input_index = _check_chinese_pinyin(t9);
				if(0 == input_index)
				{
					return (0);
				}

#ifdef GUI_T9
				input_count = hzk_index[input_index][6];
#endif
				string_value = (char *)GUI_MALLOC(input_count * 10);
				if(NULL == string_value)
				{
					return (1);
				}
				memset(string_value, 0, input_count * 10);

				pstr = string_value;
				for(i = 0; i < input_count; i++)
				{
					sprintf(pstr, "%d.", i + 1);
					pstr += 2;

					j = 0;
					for(j = 0; j < (t9->key_index + 1); j++)
					{
#ifdef GUI_T9
						*pstr = 'a' + hzk_index[input_index][19 + i * (t9->key_index + 1) + j] - 1;
#endif
						pstr++;
					}

					strcat(pstr, " ");
					pstr++;
				}

				GUI_SetProperty(t9->input0_text->name,
						"string",
						string_value);

				GUI_FREE(string_value);

				string_value = (char *)GUI_MALLOC(60);
				if(NULL == string_value)
				{
					return (1);
				}
				memset(string_value, 0, 60);

				pstr = string_value;
#ifdef GUI_T9
				gb2312_code = ((hzk_index[input_index][8] - 1 + 0xA1) << 8) |
					(hzk_index[input_index][7] - 16 + 0xB0);
#endif
				for(i = 0; i < 9; i++)
				{
					sprintf(pstr, "%d.", i + 1);
					pstr += 2;
					memcpy(pstr, &gb2312_code, 2);
					pstr += 2;
					strcat(pstr, " ");
					pstr++;
					gb2312_code = (gb2312_code << 8) | ((gb2312_code >> 8) & 0x00FF);
					if(0xFE == (0x00FE & gb2312_code))
					{
						gb2312_code = ((gb2312_code + 0x0100) & 0xFF00) + 0xA1;
					}
					else
					{
						gb2312_code++;
					}
					gb2312_code = (gb2312_code << 8) | ((gb2312_code >> 8) & 0x00FF);
				}
				GUI_SetProperty(t9->input1_text->name,
						"string",
						string_value);

				GUI_FREE(string_value);

				t9->input_index = input_index;

				t9->key_index++;
			}
			else if(SELECT_MODE == t9->write_mode)
			{
				t9->write_mode = CHARACTER_MODE;
				input_index = key - GUIK_0;

#ifdef GUI_T9
				if(input_index > hzk_index[t9->input_index][6])
				{
					return (0);
				}
#endif

				if(input_index)
				{
					input_index--;
				}

#ifdef GUI_T9
				gb2312_code = ((hzk_index[t9->input_index][8 + input_index * 2] - 1 + 0xA1) << 8) |
					(hzk_index[t9->input_index][7 + input_index * 2] - 16 + 0xB0);
#endif
				t9->offset_code = t9->start_code = gb2312_code;

				string_value = (char *)GUI_MALLOC(60);
				if(NULL == string_value)
				{
					return (1);
				}
				memset(string_value, 0, 60);

				pstr = string_value;
				for(i = 0; i < 9; i++)
				{
					sprintf(pstr, "%d.", i + 1);
					pstr += 2;
					memcpy(pstr, &gb2312_code, 2);
					pstr += 2;
					strcat(pstr, " ");
					pstr++;
					gb2312_code = (gb2312_code << 8) | ((gb2312_code >> 8) & 0x00FF);
					if(0xFE == (0x00FE & gb2312_code))
					{
						gb2312_code = ((gb2312_code + 0x0100) & 0xFF00) + 0xA1;
					}
					else
					{
						gb2312_code++;
					}
					gb2312_code = (gb2312_code << 8) | ((gb2312_code >> 8) & 0x00FF);
				}
				GUI_SetProperty(t9->input1_text->name,
						"string",
						string_value);

				GUI_FREE(string_value);
			}
			else if(CHARACTER_MODE == t9->write_mode)
			{
				input_index = key - GUIK_0;
				if(input_index)
				{
					input_index--;
				}
				gb2312_code = t9->offset_code + (input_index << 8);

				GUI_GetProperty(t9->edit_text->name,
						"string",
						&string_value);
				len = strlen(string_value);
				pstr = (char *)GUI_MALLOC(len + 10);
				strcpy(pstr, string_value);
				memcpy(pstr + len, &gb2312_code, 2);
				pstr[len + 2] = '\0';
				t9->write_mode = SPELL_MODE;
				memset(t9->key, 0, 6);
				t9->key_index = 0;
				t9->character_offset = 0;

				GUI_SetProperty(t9->edit_text->name,
						"string",
						pstr);
				GUI_FREE(pstr);
			}
			else
			{
				;
			}
		}
		else if(LOWER_CASE_MODE == t9->input_mode)
		{
			if(SPELL_MODE == t9->write_mode)
			{
				input_index = key - GUIK_0;
				if(input_index)
				{
					input_index--;
				}

				capital_char = (char *)pCapitalValue[input_index];

				string_value = (char *)GUI_MALLOC(60);
				if(NULL == string_value)
				{
					return (1);
				}
				memset(string_value, 0, 60);

				pstr = string_value;

				for(i = 0; i < capital_char[0]; i++)
				{
					sprintf(pstr, "%d.", i + 1);
					pstr += 2;
					*pstr = capital_char[i + 1];
					pstr++;
					strcat(pstr, " ");
					pstr++;
				}
				*pstr = '\0';

				GUI_SetProperty(t9->input0_text->name,
						"string",
						string_value);

				GUI_FREE(string_value);
				t9->catital_char = capital_char;
			}
			else if(SELECT_MODE == t9->write_mode)
			{
				input_index = key - GUIK_0;

				if(input_index > t9->catital_char[0])
				{
					return (0);
				}

				GUI_GetProperty(t9->edit_text->name,
						"string",
						&string_value);
				len = strlen(string_value);
				pstr = (char *)GUI_MALLOC(len + 10);
				strcpy(pstr, string_value);

				pstr[len] = t9->catital_char[input_index];
				pstr[len + 1] = '\0';

				GUI_SetProperty(t9->edit_text->name,
						"string",
						pstr);
				GUI_FREE(pstr);

				t9->write_mode = SPELL_MODE;
			}
		}
		else if(UPPER_CASE_MODE == t9->input_mode)
		{
			if(SPELL_MODE == t9->write_mode)
			{
				input_index = key - GUIK_0;
				if(input_index)
				{
					input_index--;
				}

				capital_char = (char *)pCapitalValue[input_index];

				string_value = (char *)GUI_MALLOC(60);
				if(NULL == string_value)
				{
					return (1);
				}
				memset(string_value, 0, 60);

				pstr = string_value;

				for(i = 0; i < capital_char[0]; i++)
				{
					sprintf(pstr, "%d.", i + 1);
					pstr += 2;
					*pstr = capital_char[i + 1] - 'a' + 'A';
					pstr++;
					strcat(pstr, " ");
					pstr++;
				}
				*pstr = '\0';

				GUI_SetProperty(t9->input0_text->name,
						"string",
						string_value);

				GUI_FREE(string_value);
				t9->catital_char = capital_char;
			}
			else if(SELECT_MODE == t9->write_mode)
			{
				input_index = key - GUIK_0;

				if(input_index > t9->catital_char[0])
				{
					return (0);
				}

				GUI_GetProperty(t9->edit_text->name,
						"string",
						&string_value);
				len = strlen(string_value);
				pstr = (char *)GUI_MALLOC(len + 10);
				strcpy(pstr, string_value);

				pstr[len] = t9->catital_char[input_index] - 'a' + 'A';
				pstr[len + 1] = '\0';

				GUI_SetProperty(t9->edit_text->name,
						"string",
						pstr);
				GUI_FREE(pstr);

				t9->write_mode = SPELL_MODE;
			}
		}
		else if(SYMBOL_MODE == t9->input_mode)
		{
			if(SELECT_MODE == t9->write_mode)
			{
				input_index = key - GUIK_0;
				if(input_index)
				{
					input_index--;
				}

				gb2312_code = pSymbolString[t9->symbol_page * 9 + input_index];

				GUI_GetProperty(t9->edit_text->name,
						"string",
						&string_value);
				len = strlen(string_value);
				pstr = (char *)GUI_MALLOC(len + 10);
				strcpy(pstr, string_value);
				memcpy(pstr + len, &gb2312_code, 2);
				pstr[len + 2] = '\0';

				GUI_SetProperty(t9->edit_text->name,
						"string",
						pstr);
				GUI_FREE(pstr);

				t9->write_mode = SPELL_MODE;
			}
		}
		else if(DIGIT_MODE == t9->input_mode)
		{
			if(SELECT_MODE == t9->write_mode)
			{
				input_index = key - GUIK_0;

				GUI_GetProperty(t9->edit_text->name,
						"string",
						&string_value);
				len = strlen(string_value);
				pstr = (char *)GUI_MALLOC(len + 10);
				strcpy(pstr, string_value);
				pstr[len] = '0' + input_index;
				pstr[len + 1] = '\0';

				GUI_SetProperty(t9->edit_text->name,
						"string",
						pstr);
				GUI_FREE(pstr);

				t9->write_mode = SPELL_MODE;
			}
		}
		break;
	case GUIK_PAGE_UP/*Page up in Input box*/:
		if(PINYIN_MODE == t9->input_mode)
		{
			t9->character_offset -= 9;
			gb2312_code = t9->start_code;
			gb2312_code = (gb2312_code << 8) | ((gb2312_code >> 8) & 0x00FF);
			gb2312_code += t9->character_offset;
			gb2312_code = (gb2312_code << 8) | ((gb2312_code >> 8) & 0x00FF);
			string_value = (char *)GUI_MALLOC(60);
			if(NULL == string_value)
			{
				return (1);
			}
			memset(string_value, 0, 60);

			t9->offset_code = gb2312_code;

			pstr = string_value;
			for(i = 0; i < 9; i++)
			{
				sprintf(pstr, "%d.", i + 1);
				pstr += 2;
				memcpy(pstr, &gb2312_code, 2);
				pstr += 2;
				strcat(pstr, " ");
				pstr++;
				gb2312_code = (gb2312_code << 8) | ((gb2312_code >> 8) & 0x00FF);
				if(0xFE == (0x00FE & gb2312_code))
				{
					gb2312_code = ((gb2312_code + 0x0100) & 0xFF00) + 0xA1;
				}
				else
				{
					gb2312_code++;
				}
				gb2312_code = (gb2312_code << 8) | ((gb2312_code >> 8) & 0x00FF);
			}
			GUI_SetProperty(t9->input1_text->name,
					"string",
					string_value);

			GUI_FREE(string_value);
		}
		else if(SYMBOL_MODE == t9->input_mode)
		{
			t9->symbol_page ^= 1;

			pstr = symbol_char;
			for(i = 0; i < 9; i++)
			{
				sprintf(pstr, "%d. ", i + 1);
				pstr += 3;
				*pstr = pSymbolString[i + t9->symbol_page * 9] & 0x00FF;
				pstr++;
				*pstr = pSymbolString[i + t9->symbol_page * 9] >> 8;
				pstr++;
				strcat(pstr, " ");
				pstr++;
			}
			*pstr = '\0';
			GUI_SetProperty(t9->input1_text->name,
					"string",
					symbol_char);
		}
		break;
	case GUIK_PAGE_DOWN/*Page down in Input box*/:
		if(PINYIN_MODE == t9->input_mode)
		{
			t9->character_offset += 9;
			gb2312_code = t9->start_code;
			gb2312_code = (gb2312_code << 8) | ((gb2312_code >> 8) & 0x00FF);
			/*Out of range*/
			if((0xFF - (gb2312_code & 0x00FF)) < t9->character_offset)
			{
				gb2312_code = ((gb2312_code + 0x0100) & 0xFF00) + (0xA1 + (t9->character_offset - (0xFF - (gb2312_code & 0x00FF))));
			}
			else
			{
				gb2312_code += t9->character_offset;
			}
			gb2312_code = (gb2312_code << 8) | ((gb2312_code >> 8) & 0x00FF);
			string_value = (char *)GUI_MALLOC(60);
			if(NULL == string_value)
			{
				return (1);
			}
			memset(string_value, 0, 60);

			t9->offset_code = gb2312_code;

			pstr = string_value;
			for(i = 0; i < 9; i++)
			{
				sprintf(pstr, "%d.", i + 1);
				pstr += 2;
				memcpy(pstr, &gb2312_code, 2);
				pstr += 2;
				strcat(pstr, " ");
				pstr++;
				gb2312_code = (gb2312_code << 8) | ((gb2312_code >> 8) & 0x00FF);
				if(0xFE == (0x00FE & gb2312_code))
				{
					gb2312_code = ((gb2312_code + 0x0100) & 0xFF00) + 0xA1;
				}
				else
				{
					gb2312_code++;
				}
				gb2312_code = (gb2312_code << 8) | ((gb2312_code >> 8) & 0x00FF);
			}
			GUI_SetProperty(t9->input1_text->name,
					"string",
					string_value);

			GUI_FREE(string_value);
		}
		else if(SYMBOL_MODE == t9->input_mode)
		{
			t9->symbol_page ^= 1;

			pstr = symbol_char;
			for(i = 0; i < 9; i++)
			{
				sprintf(pstr, "%d. ", i + 1);
				pstr += 3;
				*pstr = pSymbolString[i + t9->symbol_page * 9] & 0x00FF;
				pstr++;
				*pstr = pSymbolString[i + t9->symbol_page * 9] >> 8;
				pstr++;
				strcat(pstr, " ");
				pstr++;
			}
			*pstr = '\0';
			GUI_SetProperty(t9->input1_text->name,
					"string",
					symbol_char);
		}
		break;
	case GUIK_RETURN/*Enter*/:
		if(SPELL_MODE == t9->write_mode)
		{
			t9->write_mode = SELECT_MODE;
		}
		break;
	case GUIK_LEFT/*Delete a character in edit box*/:
		if(PINYIN_MODE == t9->input_mode)
		{
			if(SPELL_MODE == t9->write_mode)
			{
				if(t9->key_index)
				{
					t9->key[t9->key_index - 1] = 0;
					t9->key_index--;

					input_index = _check_chinese_pinyin(t9);
#ifdef GUI_T9
					input_count = hzk_index[input_index][6];
#endif
					if((0 == input_count) || (0 == input_index))
					{
						GUI_SetProperty(t9->input0_text->name,
								"string",
								"");
						GUI_SetProperty(t9->input1_text->name,
								"string",
								"");

						self->need_update = GUI_TRUE;
						return (0);
					}
					string_value = (char *)GUI_MALLOC(input_count * 10);
					if(NULL == string_value)
					{
						return (1);
					}
					memset(string_value, 0, input_count * 10);

					pstr = string_value;
					for(i = 0; i < input_count; i++)
					{
						sprintf(pstr, "%d.", i + 1);
						pstr += 2;

						j = 0;
#ifdef GUI_T9
						for(j = 0; j < t9->key_index; j++)
						{
							*pstr = 'a' + hzk_index[input_index][19 + i * t9->key_index + j] - 1;
							pstr++;
						}
#endif

						strcat(pstr, " ");
						pstr++;
					}

					GUI_SetProperty(t9->input0_text->name,
							"string",
							string_value);

					GUI_FREE(string_value);

					string_value = (char *)GUI_MALLOC(60);
					if(NULL == string_value)
					{
						return (1);
					}
					memset(string_value, 0, 60);

#ifdef GUI_T9
					pstr = string_value;
					gb2312_code = ((hzk_index[input_index][8] - 1 + 0xA1) << 8) |
						(hzk_index[input_index][7] - 16 + 0xB0);
					for(i = 0; i < 9; i++)
					{
						sprintf(pstr, "%d.", i + 1);
						pstr += 2;
						memcpy(pstr, &gb2312_code, 2);
						pstr += 2;
						strcat(pstr, " ");
						pstr++;
						gb2312_code = ((hzk_index[input_index][8] - 1 + 0xA1 + i + 1) << 8) |
							(hzk_index[input_index][7] - 16 + 0xB0);;
					}
#endif
					GUI_SetProperty(t9->input1_text->name,
							"string",
							string_value);

					GUI_FREE(string_value);

					t9->input_index = input_index;
				}
			}
			else if(SELECT_MODE == t9->write_mode)
			{
				t9->write_mode = SPELL_MODE;
			}
			else if(CHARACTER_MODE == t9->write_mode)
			{
				t9->write_mode = SPELL_MODE;
			}
		}
		break;
	case GUIK_BACKSPACE:
		GUI_GetProperty(t9->edit_text->name,
				"string",
				&string_value);
		len = strlen(string_value);
		pstr = (char *)GUI_MALLOC(len + 10);
		strcpy(pstr, string_value);

		if(0x7F < (unsigned char)pstr[len - 1])
		{
			pstr[len - 2] = '\0';
		}
		else
		{
			pstr[len - 1] = '\0';
		}

		GUI_SetProperty(t9->edit_text->name,
				"string",
				pstr);
		GUI_FREE(pstr);
		break;
	default:
		return (EVENT_TRANSFER_KEEPON);
	}

	self->need_update = GUI_TRUE;
	return (0);
}

static int t9_event(GuiWidget *self, void *data)
{
	GuiT9 *t9 = NULL;
	GUI_Event *event = NULL;
	int ret = 0, key = 0;

	WIDGET_CHECK_OBJECT(self, data, t9, GuiT9);

	event = (GUI_Event *)data;

	if(0 == event->type)
	{
		return (1);
	}

	if(t9->keypress_event)
	{
		ret = exec_widget_event_data(self, t9->keypress_event, data);
		if(EVENT_TRANSFER_STOP == ret)
		{
			return ret;
		}
	}

	switch(event->type)
	{
	case GUI_KEYDOWN:
		key = event->key.sym;
		return (_add_key(self, t9, key));
	default:
		break;
	}

	return (0);
}

static int t9_set_property(GuiWidget *self, char *name, void *data)
{
	return (0);
}

static int t9_get_property(GuiWidget *self, char *name, void *data)
{
	return (1);
}

GuiWidgetOps t9_ops =
{
	.owner = "t9",
	.create = create_t9,
	.release = t9_release,
	.draw = t9_draw,
	.event = t9_event,
	.set = t9_set_property,
	.get = t9_get_property
};

