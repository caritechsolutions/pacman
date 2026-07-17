#include "hd_gal.h"
#include "SDL.h"

int hd_img_desc(void *screen, const image_desc * pDesc, int x, int y, int w, int h)
{
	char *pBmpData = NULL;
	unsigned short *pHighData = NULL, color = 0, temp_value = 0;
	int BytesPerLine = 0;
	int xPos = 0, yPos = 0;
	SDL_Surface *surface = (SDL_Surface *) screen;

	if (NULL == surface || NULL == pDesc) {
		return (1);
	}

	hd_add_blit_element(screen);

	if(8 == pDesc->bpp)
		pBmpData = (char *)(pDesc->data);
	else if(16 == pDesc->bpp)
		pHighData = (unsigned short *)(pDesc->data);

	switch (pDesc->bpp) {
		case 1:
			BytesPerLine = ((pDesc->width + 31) >> 5) << 2;
			break;
		case 4:
			BytesPerLine = (((pDesc->width << 2) + 31) >> 5) << 2;
			break;
		case 8:
			BytesPerLine = ((pDesc->width + 3) >> 2) << 2;
			break;
		case 16:
			BytesPerLine = (((pDesc->width * 16) + 31) >> 5)<<2;
			break;
		case 32:
			break;
		default:
			break;
	}

	if (pBmpData || pHighData) {
		for (yPos = 0; yPos < pDesc->height; yPos++) {
			for (xPos = 0; xPos < pDesc->width; xPos++) {
				if(pBmpData) {
					gal_putpixel(surface,
							x + xPos,
							y + yPos,
							pBmpData[yPos * pDesc->width + xPos]);
				}
				else if(pHighData) {
					if(NULL == pDesc->filename) {
						temp_value = pHighData[yPos * pDesc->width + xPos] >> 4;
						temp_value = ((temp_value & 0x00F) << 8) |
							(temp_value & 0x0F0) | ((temp_value & 0xF00) >> 8);
					}
					else
						temp_value = pHighData[yPos * pDesc->width + xPos];
					color = ((temp_value & 0x000F) << 1) |
						((temp_value & 0x00F0) << 3) |
						((temp_value & 0x0F00) << 4);

					if(gal_color2index(screen, gui.config.gui_trans) != (Color)(pHighData[yPos * pDesc->width + xPos])) {
						gal_putpixel(surface,
								x + xPos,
								y + yPos,
								color);
					}
				}
			}
		}
	}

	return (0);
}


int hd_img_copy(void *surface, char *buffer, int x, int y, int x_size, int y_size, int component)
{
	return (0);
}

int hd_img_clear(void)
{
	return (0);
}

int hd_enable_img(int flag)
{
	return (0);
}


