#include "keyboard.h"
#include "string.h"
static uint32_t *gkeymap;
int32_t keyboard_register(uint32_t *key)
{
	if(key == NULL)
		return -1;
	gkeymap = key;
	return 0;
}
key_type get_key(uint32_t code)
{
	int i = 0;
	key_type key = -1;
	for(i = 0; i < OTAK_MAX; i++) {
		if(code == gkeymap[i])
			key = i;
	}
	return key;
}
