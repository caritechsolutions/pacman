#include "hd_blit.h"
#include "hd_gal.h"
#include "gui_core.h"
#include "SDL.h"
#include "gui_private.h"

static HD_COMMIT_JOB g_commit_job = {0};

int hd_enable_osd(int flag)
{
	return (0);
}

int hd_new_blit_list(void)
{
	int ret = 0;

	if(g_commit_job.list_head)
	{
		hd_commit_blit();
	}
	g_commit_job.total_num = 0;

	return (ret);
}

int hd_end_blit_list(void)
{
	return (0);
}

int hd_add_blit_element(void *surface)
{
	HD_DrawNode *draw_node = NULL;
	HD_DrawNode *p_node = NULL, *p_next = NULL;
	int i = 0, ret = 0;

	g_commit_job.total_num++;
	draw_node = (HD_DrawNode *)GxCore_Malloc(sizeof(HD_DrawNode));
	if(NULL == draw_node)
	{
		return (1);
	}
	memset(draw_node, 0, sizeof(HD_DrawNode));

	draw_node->surface = surface;
	draw_node->next = NULL;

	if((NULL == g_commit_job.list_head) && (NULL == g_commit_job.list_tail))
	{
		g_commit_job.list_head = g_commit_job.list_tail = draw_node;
	}
	else
	{
		g_commit_job.list_tail->next = draw_node;
		g_commit_job.list_tail = g_commit_job.list_tail->next;
	}

	if(MAX_BLIT_COUNT <= g_commit_job.total_num)
	{
		p_node = g_commit_job.list_head;
		i = 0;

		while(p_node)
		{
			p_next = p_node->next;

			GUI_FREE(p_node);

			p_node = p_next;
			g_commit_job.list_head = p_node;
			i++;
			if(MAX_BLIT_COUNT <= i)
			{
				break;
			}
		}

		g_commit_job.total_num -= MAX_BLIT_COUNT;
		if(NULL == g_commit_job.list_head)
		{
			g_commit_job.list_tail = NULL;
		}
	}

	return (ret);
}

int hd_commit_blit(void)
{
	int ret = 0, i = 0;
	HD_DrawNode *p_node = NULL, *p_next = NULL;

	if((0 == g_commit_job.total_num) || (MAX_BLIT_COUNT <= g_commit_job.total_num))
	{
		return (0);
	}

	p_node = g_commit_job.list_head;

	while(p_node)
	{
		p_next = p_node->next;

		GUI_FREE(p_node);

		i++;
		p_node = p_next;
		g_commit_job.list_head = p_node;
	}

	g_commit_job.total_num = 0;
	g_commit_job.list_head = g_commit_job.list_tail = NULL;

	return (ret);
}


void *hd_get_surface(GAL_Rect *rect, int bpp)
{
	void *surface = NULL;

	surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
			rect->w,
			rect->h,
			gui.config.bpp,
			0x0,
			0x0,
			0x0,
			0x0);

	return (surface);
}

int hd_change_surface(void *src0, void *src1)
{
	return (0);
}

int hd_set_main_surface(void *surface)
{
	return (0);
}

int hd_read_surface(void *surface, unsigned short *width, unsigned short *height, void **buffer)
{
	return (0);
}

int hd_stretch_surface(void *src, GAL_Rect *src_rect, void *dst, GAL_Rect *dst_rect)
{
	int ret = 0;

	if((NULL == src) || (NULL == src_rect) || (NULL == dst) || (NULL == dst_rect))
	{
		return (1);
	}

	ret = SDL_SoftStretch(src, (SDL_Rect*)src_rect, dst, (SDL_Rect*)dst_rect);

	return (ret);
}

int hd_copy_surface(void *src, GAL_Rect *src_rect, void *dst, GAL_Rect *dst_rect)
{
	int ret = 0;

	if((NULL == src) || (NULL == src_rect) || (NULL == dst) || (NULL == dst_rect))
	{
		return (1);
	}

	ret = SDL_UpperBlit(src, (SDL_Rect*)src_rect, dst, (SDL_Rect*)dst_rect);

	return (ret);
}

void hd_free_surface(void *surface)
{
	SDL_Surface *free_surface = NULL;

	free_surface = (SDL_Surface *)surface;

	if(free_surface)
	{
		SDL_FreeSurface(free_surface);
	}

}

