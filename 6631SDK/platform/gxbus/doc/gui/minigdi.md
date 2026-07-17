# API集合

[minigui_init](./group__GDI_1gad5734dffd4b3f73e75adfd70d7f53045.md)

[minigui_get_osd](./group__GDI_1gadcfc49d90368889fe4e88f4a03e1d34b.md)

[minigui_show_logo](./group__GDI_1gad55685b381ff65c3344ae0912d3198a1.md)

[minigui_show_logo_from_partition](./group__GDI_1ga4b727e7f828f6b647fb3377e4e98b032.md)

[minigui_show_logo_from_data](./group__GDI_1ga1bafa65b1a23b03d221d1f470630690a.md)

[minigui_load_font](./group__GDI_1gaff3db215efd8f2253cb3ef33c62b3fb1.md)

[minigui_get_font](./group__GDI_1ga2480fbcddf958b39d2ff1c09aef385a8.md)

[minigui_set_font](./group__GDI_1ga1996a25aef9a75b42d930428807a4afb.md)

[minigui_get_surface](./group__GDI_1ga43f649ce01b0fd7bcf9bdfbd3abfe1ca.md)

[minigui_free_surface](./group__GDI_1ga1e28cd2a0c47938d2dc9fd6e92394fb6.md)

[minigui_fillrect](./group__GDI_1ga1c80b33821580f28e084d7a106618249.md)

[minigui_copy](./group__GDI_1ga476fad65975011d54fbe17e359e3f17b.md)

[minigui_draw_string](./group__GDI_1gad5ebf81c70471ea476c8b2d8e772ceae.md)

[minigui_get_string_pixel](./group__GDI_1ga5465e6c9e86b3dbb97ed3380aef8883c.md)

[minigui_get_font_height](./group__GDI_1ga1da85eada5a21f25c6b8306811b6a49c.md)

[minigui_load_image](./group__GDI_1gada0140ef9f1451c26879ef59bf3b3788.md)

[minigui_draw_image](./group__GDI_1gac02e36109978f4e072acd714279404b6.md)

[minigui_free_image](./group__GDI_1gaa7b132d487b53d39236c84a71e4d29aa.md)

# 使用范例

```cpp

#include "mini_gui.h"

MiniGUI_Rect src_rect = {0}, dst_rect = {0};
GuiSurface *src_surface = NULL, *dst_surface = NULL;

if(NULL == (dst_surface = minigui_init(1024, 576, 16))) {
	gxlogd("[OTA Error] Init MINI GUI failed.\n");
	return (GXCORE_ERROR);
}

if(GXCORE_ERROR == minigui_load_font("arial", "arial.ttf", 30, MINI_TTF_STYLE_NORMAL)) {
	gxlogd("[OTA Error] Load Font failed.\n");
	return (GXCORE_ERROR);
}

minigui_set_font("arial");

src_rect.x = src_rect.y = 0;
src_rect.w = 1000;
src_rect.h = 200;
src_surface = minigui_get_surface(&src_rect, GX_COLOR_FMT_RGB565);
if(NULL == src_surface) {
	gxlogd("[OTA Error] Get surface failed.\n");
	return (GXCORE_ERROR);
}

if(GXCORE_ERROR == minigui_fillrect(src_surface, &src_rect, 0x0000FF)) {
	gxlogd("[OTA Error] Fill Rectangle failed.\n");
	return (GXCORE_ERROR);
}

if(GXCORE_ERROR == minigui_draw_string(src_surface, "MINI GUI Test!\n", &src_rect, 0xFFFFFF, MINI_TA_LEFT | MINI_TA_VCENTRE)) {
	gxlogd("[OTA Error] Draw String failed.\n");
	return (GXCORE_ERROR);
}

dst_rect.x = 12;
dst_rect.y = 218;
dst_rect.w = 1000;
dst_rect.h = 200;
if(GXCORE_ERROR == minigui_copy(src_surface, &src_rect, dst_surface, &dst_rect)) {
	gxlogd("[OTA Error] Copy failed.\n");
	return (GXCORE_ERROR);
}

if(GXCORE_ERROR == minigui_free_surface(src_surface)) {
	gxlogd("[OTA Error] Free surface failed.\n");
	return (GXCORE_ERROR);
}

return (GXCORE_SUCCESS);

```

