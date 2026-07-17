#include "gui_private.h"
#include "gal.h"

int hd_char(void *surface, char *pData, int x, int y, int xsize, int ysize, int byteperline, int color)
{
	int i = 0, j = 0, diff = 0;

	if(NULL == pData || NULL == surface)
	{
		return (1);
	}

	//hd_add_blit_element(surface);

	//hd_create_palette();
	if(16 == gui.config.bpp)
	{
		color = ((color & 0x00F0) << 8) |
			((color & 0x0F00) >> 1) |
			((color & 0xF000) >> 11);

		/*color = ((color & 0x000F) << 1) |
		  ((color & 0x00F0) << 3) |
		  ((color & 0x0F00) << 4);*/
	}

	for (i = 0; i < ysize; i++) {
		for (j = 0; j < xsize; j++) {
			if ((*pData) & (0x80 >> diff)) {
				gal_putpixel(surface, x + j, y + i, color);
			}
			diff++;

			if (8 == diff) {
				diff = 0;
				if(j != (xsize - 1))
				{
					pData++;
				}
				else
				{
					break;
				}
			}
		}
		pData++;
		diff = 0;
	}
	return (0);
}

