#ifndef __APP_CA_OSD_LAYER_H__
#define __APP_CA_OSD_LAYER_H__


#define FORE_COLOR	0x000011
#define BACK_COLOR	0xFFFFFF
#define TRANS_COLOR	0xFF00FF
#define DEFAULT_FONT	"font_22"
#define DEFAULT_ROLL_DELAY	30
#define DEFAULT_BPP		16 // due to app

typedef enum{
	CASCAM_UI_OTHER,
	CASCAM_UI_OSD,
	CASCAM_UI_POP,
	CASCAM_UI_FINGER,
}CascamLayerType;

typedef struct{
	char*		layer_name;
	CascamLayerType layer_type;
	void* next_ptr;
}CascamLayerBlock;

typedef struct{
	CascamLayerBlock*	layer_block;
	int					num;
}CascamLayerCtrl;

typedef struct{
	GuiSurface* surface;
	char*		layer_name;
	CascamLayerType layer_type;
	int			roll_handle;
	void*		img_desc;
	uint32_t	x;
	uint32_t	y;
	uint32_t	w;
	uint32_t	h;
	uint8_t*	font_name;
	int			fore_color;
	int			back_color;
}CascamOsdLayerBlock;

typedef struct{
	int		x;
	int		y;
	int		w;
	int		h;
}CascamRect;

typedef enum{
	CASCAM_STRING_PARAGRAPH,
	CASCAM_STRING_SINGLE,
}CascamStringType;

typedef enum{
	CASCAM_ROLL_RIGHT_LEFT,
	CASCAM_ROLL_LEFT_RIGHT,
	CASCAM_ROLL_AUTO,
}CascamRollDirect;
/*Text alignment flags, horizontal*/
#define CASCAM_TA_LEFT				(0<<0)
#define CASCAM_TA_RIGHT            (1<<0)
#define CASCAM_TA_HCENTRE          (2<<0)

/*Text alignment flags, vertical*/
#define CASCAM_TA_TOP      (0<<2)
#define CASCAM_TA_BOTTOM   (1<<2)
#define CASCAM_TA_VCENTRE  (2<<2)

int cascam_osd_layer_init(CascamOsdLayerBlock* layer);
int cascam_osd_layer_fillrect(CascamOsdLayerBlock* layer, CascamRect rect);
int cascam_osd_layer_draw_string(CascamOsdLayerBlock* layer, void* string, CascamRect rect, int alignment, CascamStringType flag);
int cascam_osd_layer_start_rolling_string(CascamOsdLayerBlock* layer, void* string, CascamRect rect, CascamRollDirect flag);
int cascam_osd_layer_set_rolling_delay(uint32_t delay_time);
int cascam_osd_layer_stop_rolling_string(CascamOsdLayerBlock* layer);
int cascam_osd_layer_resume_rolling_string(int handle);
int cascam_osd_layer_pause_rolling_string(int handle);
int cascam_osd_layer_get_roll_times(int handle);
int cascam_osd_layer_get_image_width(char* filename);
int cascam_osd_layer_get_image_height(char* filename);
int cascam_osd_layer_start_draw_image(CascamOsdLayerBlock* layer, uint32_t x_pos, uint32_t y_pos, char* filename);
int cascam_osd_layer_stop_draw_image(CascamOsdLayerBlock* layer);
int cascam_osd_layer_quit(CascamOsdLayerBlock* layer);
int cascam_osd_layer_get_font_height(void* font);
int cascam_osd_layer_get_string_pixels(void* string, uint8_t* font);
int cascam_osd_layer_zoom(CascamOsdLayerBlock* layer, CascamRect src_rect, CascamRect dst_rect, void* string, int alignment, CascamStringType flag);
#endif
