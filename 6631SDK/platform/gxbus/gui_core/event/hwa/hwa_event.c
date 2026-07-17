#include "gui_private.h"
#include "gui.h"

unsigned char hd_poll_event(GUI_Event *data)
{
	// TODO:
	unsigned int code;
	unsigned char type = 0;
	unsigned short syscode = 0;
	unsigned short syscode_reverse = 0;
	unsigned short usrcode = 0;
	unsigned short usrcode_reverse = 0;
	GUI_KeySource source;

	code = key_read(&source);

	if ((code) && (!gui.config.irr_key_mute)) {
		//GxTime time;
		if(code > 0xFFFF)
		{
		    syscode = (code >> 24) & 0xff;
		    syscode_reverse = (code >> 16) & 0xff;
		    usrcode = (code >> 8) & 0xff;
		    usrcode_reverse = (code)  & 0xff;

		    if((0xff != syscode + syscode_reverse) && (0 == (0xFF0000 & gui.config.key_mask)))
		    {
			    GUI_Printf("[IRR]syscode verify error: 0x%x, 0x%x\n", syscode, syscode_reverse);
		    }

		    if((0xff != usrcode + usrcode_reverse) && (0 == (0xFF000000 & gui.config.key_mask)))
		    {
			    GUI_Printf("[IRR]usrcode verify error: 0x%x, 0x%x\n", usrcode, usrcode_reverse);
			    return (GUI_NOEVENT);
		    }

		    if(0xFF000000 & gui.config.key_mask)
		    {
				if((0xff != syscode + syscode_reverse) && (0xff != usrcode + usrcode_reverse))
					data->key.scancode = code;
				else if((0xff != syscode + syscode_reverse) && (0xff == usrcode + usrcode_reverse))
					data->key.scancode = (syscode << 16) | (syscode_reverse << 8) | (usrcode);
				else if((0xff == syscode + syscode_reverse) && (0xff == usrcode + usrcode_reverse))
					data->key.scancode = (syscode<<8) + usrcode;
		    }
		    else if(0xFF0000 & gui.config.key_mask)
		    {
				if((0xff != syscode + syscode_reverse) && (0xff == usrcode + usrcode_reverse))
					data->key.scancode = (syscode << 16) | (syscode_reverse << 8) | (usrcode);
				else if((0xff == syscode + syscode_reverse) && (0xff == usrcode + usrcode_reverse))
					data->key.scancode = (syscode<<8) + usrcode;
		    }
		    else
		    {
				data->key.scancode = (syscode<<8) + usrcode;
		    }
		}
		else
		{
		    data->key.scancode = code;
		}

		type = data->key.type = GUI_KEYDOWN;

		data->key.sym = find_key(data->key.scancode);
		data->key.source = source;
		//GxCore_GetTickTime(&time);
		//GUI_Printf("keystart:%u:%u:end", time.seconds * 1000 + time.microsecs / 1000, data->key.sym);
	}
	else {
		code =  Gui_ReadKeyBuf();
		if (code > 0) {
			type = data->key.type = GUI_KEYDOWN;
			data->key.scancode = 0;
			data->key.sym = code;
			data->key.source = GUI_KEY_FROM_SW;
		}
	}

	return (type);
}


