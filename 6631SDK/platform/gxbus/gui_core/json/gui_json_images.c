#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "gui_private.h"
#include "gui_json_parse.h"

gx_stack_t *images_stack = NULL;

int gui_json_images(void* ctx, int type, const JSON_value* value)
{
	int ret = 1;
	GuiImgs *images = NULL;
	static char *key = NULL, *variable = NULL, *key_value = NULL;

	images = (GuiImgs *)ctx;
	if(NULL == images)
	{
		return (0);
	}

	if(NULL == images_stack)
	{
		images_stack = stack_new(images_stack);
	}

	switch(type) {
		case JSON_T_OBJECT_BEGIN:
			if(key)
			{
				key_value = GxCore_Strdup(key);
			}
			else
			{
				key_value = NULL;
			}
			stack_push(images_stack, (void *)key_value);
			break;
		case JSON_T_OBJECT_END:
			key_value = stack_pop(images_stack);
			GxCore_Free(key_value);
			break;
		case JSON_T_KEY:
			GUI_FREE(key);
			key = GxCore_Strdup(value->vu.str.value);
			key_value = stack_peek(images_stack);
			break;
		case JSON_T_STRING:
			key_value = stack_peek(images_stack);
			if(key_value)
			{
				if(0 == strcasecmp(JSON_IMAGE, key_value))
				{
					variable = (char *)value->vu.str.value;
					imgs_add_img(images, key, variable, 1);
				}
			}
			GUI_FREE(key);
			break;
		default:
			break;
	}

	return (ret);
}


status_t gui_json_image_freedata(GuiImgs* img)
{
	if(NULL == img)
	{
		return GXCORE_ERROR;
	}

	hash_release(img->hash_imgs);

	return GXCORE_SUCCESS;
}


