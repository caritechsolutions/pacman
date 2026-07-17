#include "widget.h"
#include "all_widget.h"
#include "image_cache.h"
#include "hd_system.h"
#ifndef NO_OS
#include "color.h"
#include "memmgr.h"
#else
#include "hd_gal.h"
#include "mini_gui_private.h"
#endif

#if 0
struct widget_ops_node {
	const char *widget_type;
	GuiWidgetOps *ops;
};

static struct widget_ops_head {
	struct widget_ops_node *widget_ops_array;
	int widget_ops_array_count;
	int widget_ops_num;
	int need_sort;
} ops_head = {NULL, 0, 0, 0};

static int inline ops_comp(const void *m1, const void *m2)
{
	return strcmp(((struct widget_ops_node *)m1)->widget_type,
			((struct widget_ops_node *)m2)->widget_type);
}

static void inline widget_ops_sort(void)
{
	if (ops_head.need_sort != 0) {
		qsort(ops_head.widget_ops_array,
				ops_head.widget_ops_num, sizeof(struct widget_ops_node), ops_comp);

		ops_head.need_sort = 0;
	}
}

static int _register_widget(const char *widget_type, GuiWidgetOps *ops)
{
	if (ops) {
		if (ops_head.widget_ops_num >= ops_head.widget_ops_array_count) {
			ops_head.widget_ops_array_count += 16;
			ops_head.widget_ops_array = GUI_REALLOC(ops_head.widget_ops_array,
					ops_head.widget_ops_array_count * sizeof(struct widget_ops_node));
		}
		ops_head.widget_ops_array[ops_head.widget_ops_num].widget_type = widget_type;
		ops_head.widget_ops_array[ops_head.widget_ops_num].ops         = ops;
		ops_head.widget_ops_num++;
		ops_head.need_sort = 1;

		return 0;
	}

	return -1;
}

void widget_ops_clean(void)
{
	if (ops_head.widget_ops_array)
		GUI_FREE(ops_head.widget_ops_array);
	ops_head.widget_ops_array = NULL;
	ops_head.widget_ops_array_count = ops_head.widget_ops_num = ops_head.need_sort = 0;
}

GuiWidgetOps* widget_get_ops(const char* type)
{
	struct widget_ops_node key;

	widget_ops_sort();
	key.widget_type = type;
	return (GuiWidgetOps*)bsearch(&key, ops_head.widget_ops_array,
			ops_head.widget_ops_num, sizeof(struct widget_ops_node), ops_comp);
}

#endif

#define CHECK_WIDGET(parent, widget)	    if((parent) && (widget) && (parent->name) && (widget->name)) \
{		\
	if((widget->from_resource) && (0 != strcasecmp(widget->from_resource, parent->name)))   \
	{			\
		if(((widget->rect.x+widget->rect.w)>(parent->rect.w)) || \
				(widget->rect.y+widget->rect.h)>(parent->rect.h)) \
		{	\
			return (NULL); \
		}	\
	}  \
}

#if 1
hash_t *widget_ops_hash = NULL;

static void widget_Free(void *data)
{
	//if (data) GUI_FREE((char *)data);
}

static int _register_widget(const char *widget_type, GuiWidgetOps *ops)
{
	if(NULL == widget_ops_hash)
	{
		widget_ops_hash = hash_new(80, widget_Free);
	}

	if (ops) {
		hash_add(widget_ops_hash, widget_type, ops);

		return 0;
	}

	return -1;
}

int check_rect_not_zero(GUI_Rect *rect)
{
	if(NULL == rect)
	{
		return (GUI_FALSE);
	}

	if((rect->w) && (rect->h))
	{
		return (GUI_TRUE);
	}
	else
	{
		return (GUI_FALSE);
	}
}

void widget_ops_clean(void)
{
	hash_release(widget_ops_hash);
	widget_ops_hash = NULL;
}

GuiWidgetOps* widget_get_ops(const char* type)
{
	return (GuiWidgetOps*)hash_get(widget_ops_hash, type);
}
#endif

#if 0
GuiWidgetOps* widget_ops_list = NULL;

static int _register_widget(const char *widget_type, GuiWidgetOps *ops)
{
	if (ops)
	{
		ops->next = widget_ops_list;
		widget_ops_list = ops;

		return 0;
	}

	return -1;
}

void widget_ops_clean(void)
{
}

GuiWidgetOps* widget_get_ops(const char* type)
{
	GuiWidgetOps* pWidgetOps = NULL;

	if(NULL == widget_ops_list)
	{
		gxlogd("There is no widget operation in the list!\n");
		return (NULL);
	}

	pWidgetOps = widget_ops_list;

	while(pWidgetOps)
	{
		if(0 == strcasecmp(type, pWidgetOps->owner))
		{
			return (pWidgetOps);
		}

		pWidgetOps = pWidgetOps->next;
	}

	gxlogd("There is no widget operation with '%s'!\n", type);

	return (NULL);
}

#endif

static int atoidef(const char *nptr, int v)
{
	if (nptr)
		return atoi(nptr);
	else
		return v;
}

GuiWidget *widget_get_root_from_resource(const char *resource_name)
{
#ifndef NO_OS
	GuiCore *gui_core = NULL;
	GuiConfig *config = NULL;

	gui_core = GUI_GetCurrent();

	if(NULL == resource_name) {
		return (gui_core->config.root_widget);
	} else if (gui_core->resource_config_table) {
		config = hash_get(gui_core->resource_config_table, resource_name);
		if(config && !config->invalid) {
			return (config->root_widget);
		} else {
			return (gui_core->config.root_widget);
		}
	} else {
		return (gui_core->config.root_widget);
	}
#else
	return NULL;
#endif
}

extern status_t widget_init(GuiWidget *widget);
static GuiWidget *_check_widgets(GuiWidget *widget, const char *name);

void *widget_create(GuiWidget *widget)
{
	const char *value = NULL;
	char *string = NULL;
	void* ret = NULL;
	GuiWidget *style = NULL, *parent = NULL, *root = NULL;
	GuiCore *gui_core = NULL;

	if(NULL == widget)
		return (NULL);

	gui_core = GUI_GetCurrent();
	if(widget->widget_mutex <= 0) {
		GxCore_MutexCreate(&(widget->widget_mutex));
		if(widget->widget_mutex < 0)
		{
			return (NULL);
		}
	}

	if ((widget->init == 0)
#ifndef NO_OS
	    && (((gui_core->resource_config_table) &&
	    (widget->classname) &&
	    (0 != strcasecmp("listitem", widget->classname)))
	    || (NULL == gui_core->resource_config_table))
#endif /*NO_OS*/
	    ) {
#ifndef NO_OS
		root = widget_get_root_from_resource(widget->from_resource);
		if(NULL == _check_widgets(root, widget->name))
			return (NULL);
		widget_init(widget);
#endif /*NO_OS*/
	} else if((widget->init == 0) && (0 == strcasecmp("listitem", widget->classname))) {
		widget->init = GUI_TRUE;
	}
	style = (GuiWidget *)widget_get_style(widget->stylename);

	value = (const char *)widget_get_property(widget, "original_state");
	if(value)
	{
		widget_set_property(widget, "state", value);
	}

	value = widget_get_property(widget, "mirror");
	if((value) && (0 == strcasecmp("disable", value))) {
		widget->disable_mirror = GUI_TRUE;
	} else if((value) && (0 == strcasecmp("enable", value))){
		widget->disable_mirror = GUI_FALSE;
	} else if(NULL == value) {
		parent = widget_get_parent(widget);
		if(parent) {
			widget->disable_mirror = parent->disable_mirror;
		} else {
			widget->disable_mirror = GUI_FALSE;
		}
	}

	value = widget_get_property(widget, "keypress_revert");
	if((value) && (0 == strcasecmp("enable", value))) {
		widget->keypress_revert = GUI_TRUE;
	} else if((value) && (0 == strcasecmp("disable", value))) {
		widget->keypress_revert = GUI_FALSE;
	} else {
		widget->keypress_revert = gui_core->config.keypress_revert;
	}

	widget_get_rect(widget, &(widget->rect));
	CHECK_WIDGET(widget->parent, widget);

	if(((widget->rect.x + widget->rect.w) > gui_core->config.width) ||
	   ((widget->rect.y + widget->rect.h) > gui_core->config.height) ||
	   (widget->rect.x < 0) ||
	   (widget->rect.y < 0) ||
	   (widget->rect.w < 0) ||
	   (widget->rect.h < 0))
	{
		GUI_Debug_Print("[GUI] The rectagle area of %s is out of bound.", widget->name);
		return (NULL);
	}

	value = (const char *)widget_get_property(widget, "backcolor");
	if(value)
	{
		getColorFromString(value, widget->back_color);
	}
	else if(style)
	{
		memcpy(widget->back_color, style->back_color, 3 * sizeof(int));
	}
	else
	{
		widget->back_color[0] = gui_core->config.osd_trans;
		widget->back_color[1] = gui_core->config.osd_trans;
		widget->back_color[2] = gui_core->config.osd_trans;
	}

	value = (const char *)widget_get_property(widget, "forecolor");
	if(value)
	{
		getColorFromString(value, widget->fore_color);
	}
	else if(style)
	{
		memcpy(widget->fore_color, style->fore_color, 3 * sizeof(int));
	}
	else
	{
		widget->fore_color[0] = 0xffffff;
		widget->fore_color[1] = 0xffffff;
		widget->fore_color[2] = 0xffffff;
	}

	if(NULL == widget_get_property(widget, "original_state"))
	{
		string = (char *)widget_get_property(widget, "state");
		if(string)
		{
			widget_set_property(widget, "original_state", string);
		}
		else
		{
			widget_set_property(widget, "original_state", "show");
		}

	}

	if(widget->ops && widget->ops->create)
	{

		ret = widget->ops->create(widget);

#ifndef NO_OS
		string = (char *)widget_get_property(widget, "state");
		if(string)
		{
			if(0 == strcasecmp("disable", string))
			{
				wm_state_set_disable(widget, GUI_TRUE);
			}
			else if(0 == strcasecmp("hide", string))
			{
				wm_state_set_hide(widget, GUI_TRUE, NULL);
			}
		}
#endif /*NO_OS*/

		if(NULL != ret)
		{
			widget_set_update(widget, GUI_TRUE);
		}

		return ret;
	}

	return (NULL);
}

int widget_set_active(GuiWidget *widget, int enable)
{
	GuiWidget *child = NULL;

	if(widget) {
		child = widget_get_firstchild(widget);
		while(child) {
			widget_set_active(child, enable);
			child = widget_get_nextsibling(child);
		}

		widget->active = enable;
		widget->checked = GUI_FALSE;
	}

	return (0);
}

int widget_update(GuiWidget *widget, GUI_Rect *rect)
{
	if((NULL == widget) ||
			(NULL == rect))
	{
		return (1);
	}

	if(widget->state & WIDGET_STATE_HIDE)
		return (0);

	if (widget->ops->prepare_image)
		widget->ops->prepare_image(widget);

	if(widget->ops->update)
	{
		GxCore_MutexLock(widget->widget_mutex);
		widget->ops->update(widget, rect);
		GxCore_MutexUnlock(widget->widget_mutex);
	}

	if (widget->ops->clear_image)
		widget->ops->clear_image(widget);
	return (0);
}

#ifdef NO_OS
static void _parse_rect(const char *value, int *arg0, int *arg1, int *arg2, int *arg3)
{
	char *pstr = NULL, *num_str = NULL;
	char *new_str = NULL, *new_ptr = NULL;

	pstr = (char *)value;
	while(*pstr)
	{
		if(*pstr == '[')
		{
			pstr++;
			break;
		}

		pstr++;
	}

	new_str = GxCore_Strdup(pstr);
	new_ptr = &new_str[strlen(new_str) -1];
	while(new_ptr != new_str)
	{
		if(*new_ptr == ']')
			break;
		new_ptr--;
	}

	if(*new_ptr == ']')
	{
		*new_ptr = '\0';
	}

	num_str = strtok(new_str, ",");
	*arg0 = atoi(num_str);
	num_str = strtok(NULL, ",");
	*arg1 = atoi(num_str);
	num_str = strtok(NULL, ",");
	*arg2 = atoi(num_str);
	num_str = strtok(NULL, ",");
	*arg3 = atoi(num_str);
}
#endif /*NO_OS*/

GuiSurface *widget_get_surface(GUI_Rect *rect)
{
	GuiSurface *surface = NULL;
#ifndef NO_OS
	GAL_Rect src_rect = {0}, dst_rect = {0};

	dst_rect.x = dst_rect.y = 0;
	dst_rect.w = rect->w;
	dst_rect.h = rect->h;
	surface = hd_get_surface(&dst_rect, gui.config.bpp, NULL, GUI_FALSE);
	if(NULL == surface)
	{
		return (NULL);
	}

	src_rect = dst_rect;
	src_rect.x = rect->x;
	src_rect.y = rect->y;
	gdi_commit();
	hd_copy_surface(screen, &src_rect, surface, &dst_rect);
#endif /*NO_OS*/

	return (surface);
}

GuiSurface *widget_get_current_surface(GuiSurface *surface)
{
#ifndef NO_OS
	GuiCore *gui_core = GUI_GetCurrent();
	if(gui_core->dst_surface) {
		return (gui_core->dst_surface);
	} else {
		return (surface);
	}
#else
	return (surface);
#endif /*NO_OS*/
}

void widget_mirror_rect(GuiWidget *widget)
{
#ifndef NO_OS
	GuiCore *gui_core = GUI_GetCurrent();
	GuiWidget *parent = widget_get_parent(widget);
	GuiWidget *child = NULL;

	if((0 == strcasecmp("prompt", widget->classname)) ||
	   (0 == strcasecmp("timeitem", widget->classname)) ||
	   (0 == strcasecmp("header", widget->classname)) ||
	   ((parent) && (0 == strcasecmp("prompt", parent->classname)))) {
		if(0 == strcasecmp("header", widget->classname) && gui_core->config.mirror) {
			child = widget_get_firstchild(widget);
			while(child) {
				child->mirrored = GUI_TRUE;
				child = widget_get_nextsibling(child);
			}
		}
		return;
	}

	if((gui_core->config.mirror) && (!widget->mirrored)) {
		widget->rect.x = gui_core->config.width - widget->rect.x - widget->rect.w;
		widget->mirrored = GUI_TRUE;
	} else if((!gui_core->config.mirror) && (widget->mirrored)){
		widget->rect.x = gui_core->config.width - widget->rect.x - widget->rect.w;
		widget->mirrored = GUI_FALSE;
	}
#endif /*NO_OS*/
}

int widget_is_keypress_revert(GuiWidget *widget)
{
	int ret = 0;
#ifndef NO_OS
	if(NULL == widget) {
		return (ret);
	}

	GuiCore *gui_core = GUI_GetCurrent();

	if(gui_core->config.mirror) {
		if(gui_core->config.keypress_revert != widget->keypress_revert) {
			return (GUI_TRUE);
		} else {
			return (GUI_FALSE);
		}
	} else {
		return (GUI_FALSE);
	}

#endif /*NO_OS*/
	return (ret);
}

#ifndef NO_OS
static int widget_is_container(GuiWidget *widget)
{
	if((0 == strcasecmp("window", widget->classname)) ||
	   (0 == strcasecmp("box", widget->classname)) ||
	   (0 == strcasecmp("listview", widget->classname)) ||
	   (0 == strcasecmp("prompt", widget->classname))) {
		return (GUI_TRUE);
	} else {
		return (GUI_FALSE);
	}
}
#endif /*NO_OS*/

int widget_draw(GuiWidget *widget)
{
#ifndef NO_OS
	GuiCore *gui_core = GUI_GetCurrent();
	ARABIC_STR_PRINT_MODE tmp_mode;
#endif /*NO_OS*/
	GuiWidget *parent = NULL;
	GuiSurface *old_surface = screen;
	const char *value = NULL;
	int state = 0, old_int_value = 0, ret = 0;

#ifndef NO_OS
	if((gui_core->osd_surface) &&
	   (GUI_NO_VIRTUAL_LAYER == gui.layer_mode) &&
	   (NULL == gui_core->dst_surface))
	{
		screen = gui_core->osd_surface;
	}
#endif /*NO_OS*/
	if(NULL == widget)
	{
		screen = old_surface;
		return 1;
	}
	parent = widget_get_parent(widget);
#ifndef NO_OS
	hd_check_layer();
	value = widget_get_property(widget, "rect_fix");
	if((value) && (0 == strcasecmp("enable", value))) {
		if(gui_core->config.mirror) {
			widget->mirrored = GUI_TRUE;
		} else {
			widget->mirrored = GUI_FALSE;
		}
	}
	widget_mirror_rect(widget);
#endif /*NO_OS*/
	if(widget->state & WIDGET_STATE_HIDE)
	{
		//GUI_Printf("[widget] '%s' is hided\n", widget->name);
		widget_set_update(widget, GUI_FALSE);
		screen = old_surface;
		return 1;
	}
	//GUI_Printf("=DRAW=widget '%s'\n", widget->name);

	if (widget->ops->prepare_image)
		widget->ops->prepare_image(widget);

	if(widget->ops->draw) {
		GxCore_MutexLock(widget->widget_mutex);

		if((NULL == widget) ||
				((widget) && (NULL == widget->object)))
		{
			screen = old_surface;
			GxCore_MutexUnlock(widget->widget_mutex);
			return 1;
		}

		value = (const char *)widget_get_property(widget, "backcolor");
		if(value)
			getColorFromString(value, widget->back_color);

		value = (const char *)widget_get_property(widget, "forecolor");
		if(value)
			getColorFromString(value, widget->fore_color);

		value = widget_get_property(widget, "encoding");
		if(0 != strcasecmp("notepad", widget->classname))
		{
			gdi_set_local((char *)value);
		}

		value = (const char *)widget_get_property(widget, "string_rect");
		if(value)
		{
#ifdef NO_OS
			_parse_rect(value, &widget->string_rect.x,
							   &widget->string_rect.y,
							   &widget->string_rect.w,
							   &widget->string_rect.h);
#else
			sscanf(value, "[%d,%d,%d,%d]", &widget->string_rect.x,
					&widget->string_rect.y,
					&widget->string_rect.w,
					&widget->string_rect.h);
#endif /*NO_OS*/
			if((widget->string_rect.x < widget->rect.x) ||
					(widget->string_rect.y < widget->rect.y) ||
					((widget->string_rect.x + widget->string_rect.w) > (widget->rect.x + widget->rect.w)) ||
					((widget->string_rect.y + widget->string_rect.h) > (widget->rect.y + widget->rect.h)))
			{
				GxCore_MutexUnlock(widget->widget_mutex);
				screen = old_surface;
				widget_set_update(widget, GUI_FALSE);
				return (1);
			}
		}
		state = widget->state;
		value = widget_get_property(widget, "virtual_focus");
		if((value && (0 == strcasecmp("enable", value)))){
			if(widget_get_focussable(widget)){
				widget->state |= WIDGET_STATE_FOCUSED;
			}
		}

#ifndef NO_OS
		value = widget_get_property(widget, (const char *)FONT_OVERLAY_STR);
		if(value) {
			old_int_value = gui_core->config.font_overlay;
			if(0 == strcasecmp("enable", value)) {
				gui_core->config.font_overlay = GUI_TRUE;
			} else if(0 == strcasecmp("disable", value)) {
				gui_core->config.font_overlay = GUI_FALSE;
			}
		}

		tmp_mode = gui_core->config.arabic_mode;
		value = widget_get_property(widget, (const char *)"typeset");
		if(value) {
			if(0 == strcasecmp("l2r", value))
				gui_core->config.arabic_mode = ARABIC_PRINT_MODE1;
			else if(0 == strcasecmp("r2l", value))
				gui_core->config.arabic_mode = ARABIC_PRINT_MODE0;
		}
		if((gui_core->config.mirror) &&
		   (widget_is_container(widget)) &&
		   (widget->disable_mirror)) {
			ret = gui_set_mirror0(GUI_FALSE);
		} else if((!widget_is_container(widget)) &&
			  (parent) &&
			  (parent->disable_mirror)) {
			ret = gui_set_mirror0(GUI_FALSE);
		}
		widget->ops->draw(widget);
		if(value) {
			gui_core->config.font_overlay = old_int_value;
		}
		gui_core->config.arabic_mode = tmp_mode;

		if((widget_is_container(widget)) &&
		   (widget->disable_mirror)) {
			gui_set_mirror0(ret);
		} else if((!widget_is_container(widget)) &&
			  (parent) &&
			  (parent->disable_mirror)) {
			gui_set_mirror0(ret);
		}
#endif /*NO_OS*/
		widget->state = state;
		if(0 != strcasecmp("notepad", widget->classname))
		{
			gdi_set_local(NULL);
		}
		widget_set_update(widget, GUI_FALSE);
		GxCore_MutexUnlock(widget->widget_mutex);
		if (widget->ops->clear_image)
			widget->ops->clear_image(widget);

		screen = old_surface;
		return (0);
	}

#ifndef NO_OS
	if(GUI_NO_VIRTUAL_LAYER == gui.layer_mode) {
		screen = old_surface;
	}
#endif /*NO_OS*/
	return (1);
}

int widget_release(GuiWidget *widget)
{
	if(NULL == widget)
	{
		return (1);
	}
	//GUI_Printf("=RELEASE=widget '%s'\n", widget->name);

	GxCore_MutexLock(widget->widget_mutex);
	if(widget->ops && widget->ops->release)
	{
		widget->ops->release(widget);
	}
	GxCore_MutexUnlock(widget->widget_mutex);
	//widget->object = NULL;

	GxCore_MutexDelete(widget->widget_mutex);
	widget->widget_mutex = 0;

	widget_set_update(widget, GUI_FALSE);

	return (0);
}


GuiWidget *widget_new(const char *name, const char *type, const char *style)
{
	GuiWidget *widget;
	GuiWidgetOps *widget_ops = NULL;


	widget_ops = widget_get_ops(type);
	if(NULL == widget_ops)
	{
		return (NULL);
	}

	widget = (GuiWidget *)GUI_MALLOC(sizeof(GuiWidget));
	if (NULL == widget)
	{
		return (NULL);
	}
	memset(widget, 0, sizeof(GuiWidget));
	if(widget){
		widget->classname = GUI_STRDUP(type);
		widget->ops = widget_ops;
		widget->name = GUI_STRDUP(name);
		widget->parent = NULL;
		if(style)
		{
			widget->stylename = GUI_STRDUP(style);
		}
	}

	return (widget);
}

status_t widget_destroy(GuiWidget *widget)
{
	GuiWidget *prev_child = NULL, *child = NULL;

	if((NULL == widget) || (GUI_FALSE == widget->init))
	{
		return GXCORE_ERROR;
	}

	child = widget_get_firstchild(widget);
	while(child)
	{
		prev_child = child;
		widget_destroy(child);
		GUI_FREE(child->name);
		GUI_FREE(child->classname);
		GUI_FREE(child->stylename);
		GUI_FREE(child->xml_filename);
		GUI_FREE(child->from_resource);
		child = widget_get_nextsibling(child);
		GUI_FREE(prev_child);
	}
	widget->last_child = widget->first_child = NULL;

	if(widget->property)
	{
		hash_release(widget->property);
		widget->property = NULL;
	}

	if(widget->signal)
	{
		hash_release(widget->signal);
		widget->signal = NULL;
	}

	widget->init = GUI_FALSE;

	return GXCORE_SUCCESS;
}

int widget_del(GuiWidget *widget)
{
	if(NULL == widget)
	{
		return (1);
	}
	//GUI_Printf("=DELETE=widget '%s'\n", widget->name);

	if(widget)
	{
		widget_release(widget);
	}
	widget->ops= NULL;


	GUI_FREE(widget->name);
	GUI_FREE(widget->classname);
	GUI_FREE(widget->stylename);
	GUI_FREE(widget->xml_filename);
	GUI_FREE(widget->from_resource);
	if(widget->property)
	{
		hash_release(widget->property);
	}

	if(widget->signal)
	{
		hash_release(widget->signal);
	}

	GUI_FREE(widget);

	return (0);
}

int widget_del_tree(GuiWidget *widget)
{
	GuiWidget *del = NULL;
	GuiWidget *next = NULL;

	if(NULL == widget)
	{
		return 1;
	}


	del = widget->first_child;
	widget->first_child = NULL;
	while (del)
	{
		next = del->next_sibling;
		del->next_sibling = NULL;
		widget_del_tree(del);
		del = next;
	}

	widget_del(widget);
	widget = NULL;
	return 0;
}

GuiWidget *widget_new_child(GuiWidget *parent, const char *name, const char *type, const char *style)
{
	if(parent)
	{
		GuiWidget *child = widget_new(name, type, style);
		if(child)
		{
			widget_add_child(parent, child);
			return (child);
		}
	}

	return (NULL);
}

int widget_add_child(GuiWidget *widget, GuiWidget *child)
{
	if(NULL == widget || NULL == child)
	{
		return (-1);
	}

	child->next_sibling = NULL;
	if(NULL != widget->last_child)
	{
		widget->last_child->next_sibling = child;
		child->prior_sibling = widget->last_child;
	}
	else
	{
		child->prior_sibling = NULL;
	}

	widget->last_child = child;
	if(NULL == widget->first_child)
	{
		widget->first_child = child;
	}
	child->parent = widget;

	return (0);
}


int widget_del_child(GuiWidget *widget, const char *name)
{
	GuiWidget *work = widget->first_child;

	if(NULL == widget)
	{
		return (1);
	}

	while(work)
	{
		if(0 == strcasecmp(name, work->name))
		{
			if(work->prior_sibling)
			{
				if(work->prior_sibling)
				{
					work->prior_sibling->next_sibling = work->next_sibling;
				}

				if(work->next_sibling)
				{
					work->next_sibling->prior_sibling = work->prior_sibling;
				}
				if(work == work->parent->first_child)
				{
					work->parent->first_child = NULL;
				}
				else if(work == work->parent->last_child)
				{
					work->parent->last_child = work->prior_sibling;
				}
				return (0);
			}
		}
		work = work->next_sibling;
	}

	return (-1);
}



GuiWidget *widget_rename(const char *name, const char* new_name)
{
	GuiWidget* widget = NULL;
#ifndef NO_OS
	GuiCore *gui_core = NULL;

	gui_core = GUI_GetCurrent();
	widget = gui_get_widget(gui_core->config.root_widget, new_name);
	if(widget)
	{
		return NULL;
	}

	widget = gui_get_widget(gui_core->config.root_widget, name);
	if(NULL == widget)
	{
		return NULL;
	}
	GUI_FREE(widget->name);
	widget->name = GUI_STRDUP(new_name);
#endif /*NO_OS*/

	return widget;
}


int widget_get_int(GuiWidget *widget, const char *name, int defult)
{
	const char *value = NULL;

	if(NULL == widget || NULL == name)
	{
		return (0);
	}

	if(widget->property)
	{
		value = (const char *)hash_get(widget->property, name);
	}

	return  atoidef(value, defult);
}

int widget_alignment_revert(GuiWidget *widget, char *string, int alignment)
{
#ifdef GUI_ADAPTIVE_ALIGNMENT
	int is_arabic;

	if(NULL == string) {
		return (alignment);
	}

	is_arabic = gxgdi_is_arabic((const unsigned char *)string);
	if((widget->alignment_revert) && (!is_arabic)) {
		if(GUI_TA_LEFT == (alignment  & 0x3)) {
			alignment &= (~0x3);
			alignment |= GUI_TA_RIGHT;
			widget->alignment_revert = GUI_FALSE;
		} else if(GUI_TA_RIGHT == (alignment  & 0x3)) {
			alignment &= (~0x3);
			alignment |= GUI_TA_LEFT;
			widget->alignment_revert = GUI_FALSE;
		}
	} else {
		return (alignment);
	}
#endif /*GUI_ADAPTIVE_ALIGNMENT*/

	return (alignment);
}

int widget_get_alignment(GuiWidget *widget)
{
	GuiCore *gui_core = NULL;
	const char *value = NULL;
	char*p = NULL;
	int alignment = 0;

	if(NULL == widget)
	{
		return (-1);
	}

	gui_core = GUI_GetCurrent();

	if(widget->property)
	{
		value = (const char *)hash_get(widget->property, "alignment");
	}

	if(NULL == value)
	{
		return (GUI_TA_HCENTRE | GUI_TA_VCENTRE);
	}

	p = (char *)value;
	if(strstr(p, "left"))
	{
		alignment |= GUI_TA_LEFT;
	}
	else if(strstr(p, "right"))
	{
		alignment |= GUI_TA_RIGHT;
	}
	else if(strstr(p, "hcentre"))
	{
		alignment |= GUI_TA_HCENTRE;
	}

	if(strstr(p, "top"))
	{
		alignment |= GUI_TA_TOP;
	}
	else if(strstr(p, "bottom"))
	{
		alignment |= GUI_TA_BOTTOM;
	}
	else if(strstr(p, "vcentre"))
	{
		alignment |= GUI_TA_VCENTRE;
	}

	if((gui_core->config.mirror) && (!widget->disable_mirror)) {
		if(GUI_TA_LEFT == (alignment  & 0x3)) {
			alignment &= (~0x3);
			alignment |= GUI_TA_RIGHT;
			widget->alignment_revert = GUI_TRUE;
		} else if(GUI_TA_RIGHT == (alignment  & 0x3)) {
			alignment &= (~0x3);
			alignment |= GUI_TA_LEFT;
			widget->alignment_revert = GUI_TRUE;
		}
	}

	return (alignment);
}


GuiWidget *widget_get_parent(GuiWidget *widget)
{
	GuiWidget *parent = NULL;

	if(NULL == widget)
	{
		return (NULL);
	}

	parent = widget->parent;

	return (parent);
}

GuiWidget *widget_get_firstchild(GuiWidget *widget)
{
	GuiWidget *firstchild = NULL;

	if(NULL == widget)
	{
		return (NULL);
	}

	firstchild = widget->first_child;

	return (firstchild);
}

GuiWidget *widget_get_lastchild(GuiWidget *widget)
{
	GuiWidget *lastchild = NULL;

	if(NULL == widget)
	{
		return (NULL);
	}

	lastchild = widget->last_child;

	return (lastchild);
}

GuiWidget *widget_get_priorsibling(GuiWidget *widget)
{
	GuiWidget *priorsibling = NULL;

	if(NULL == widget)
	{
		return (NULL);
	}

	priorsibling = widget->prior_sibling;

	return (priorsibling);
}

GuiWidget *widget_get_nextsibling(GuiWidget *widget)
{
	GuiWidget *nextsibling = NULL;

	if(NULL == widget)
	{
		return (NULL);
	}

	nextsibling = widget->next_sibling;

	return (nextsibling);
}

const char *widget_get_name(GuiWidget *widget)
{
	return widget ? widget->name : NULL;
}

int widget_get_rect(GuiWidget *widget, GUI_Rect *rect)
{
	if (widget && rect)
	{
		const char *value = NULL;

		if (widget->property)
			value = (const char *)hash_get(widget->property, "rect");

		if (value)
		{
#ifdef NO_OS
			_parse_rect(value, &rect->x,
							   &rect->y,
							   &rect->w,
							   &rect->h);
#else
			sscanf(value, "[%d,%d,%d,%d]", &rect->x, &rect->y, &rect->w, &rect->h);
#endif /*NO_OS*/
		}
		else if(widget->property)
		{
			rect->x = widget_get_int(widget, "left"  , 0);
			rect->y = widget_get_int(widget, "top"   , 0);
			rect->w = widget_get_int(widget, "width" , 0);
			rect->h = widget_get_int(widget, "height", 0);
		}
	}

	widget->mirrored = GUI_FALSE;

	return 0;
}


int widget_get_state_index(GuiWidget* widget)
{
	if(NULL == widget)
	{
		return 0;
	}

	if(widget->state & WIDGET_STATE_DISABLE)
	{
		return 2;//disable
	}
	else if(widget->state & WIDGET_STATE_FOCUSED)
	{
		return 1;//focused
	}

	return 0;//unfocus
}

int widget_get_focussable(GuiWidget* widget)
{
	/*shencz add*/
	if(NULL == widget)
	{
		return 0;
	}

	if( (0 == (widget->state & WIDGET_STATE_HIDE))
			&& (0 == (widget->state & WIDGET_STATE_DISABLE))
			&& (widget->state & WIDGET_STATE_FOCUSSABLE))
	{
		return 1;
	}

	return 0;
}

GuiWidget* widget_get_window(GuiWidget* widget)
{
	GuiWidget* window = NULL;

	if(NULL == widget)// || NULL == widget->object)
	{
		return NULL;
	}

	window = widget;
	while((window) && (!WIDGET_IS_WINDOW(window)))
	{
		window = widget_get_parent(window);
	}

	return window;
}

#ifdef NO_OS
int				widget_need_update_count;
#endif /*NO_OS*/
void widget_children_set_update(GuiWidget *widget, int update)
{
#ifndef NO_OS
	GuiCore *gui_core = GUI_GetCurrent();
#endif /*NO_OS*/
	GuiWidget *child = NULL;

	child = widget_get_firstchild(widget);
	while(child)
	{
		widget_set_update(child, update);
		widget_children_set_update(child, update);

		child = widget_get_nextsibling(child);
	}

	if(update)
	{
#ifdef NO_OS
		widget_need_update_count++;
#else /*NO_OS*/
		gui_core->widget_need_update_count++;
#endif /*NO_OS*/
	}
}

void widget_set_virtual_focus(GuiWidget *widget)
{
#ifndef NO_OS
	GuiWidget *focus = NULL;
	GuiCore *gui_core = NULL;

	if(NULL == widget)
	{
		return;
	}

	gui_core = GUI_GetCurrent();
	if(gui_core->config.focus_widget == widget)
	{
		focus = widget_get_nextsibling(widget);
		if(NULL == focus)
		{
			focus = widget_get_firstchild(focus->parent);
		}
		while(focus && widget_get_focussable(focus))
		{
			focus = widget_get_nextsibling(focus);
			if(NULL == focus)
			{
				focus = widget_get_firstchild(focus->parent);
			}
		}

		if((NULL == focus) || (focus == widget))
		{
			gui_core->config.focus_widget = focus->parent;
		}
		else
		{
			gui_core->config.focus_widget = focus;
		}

	}

	if(widget_get_focussable(widget) &&
			!(widget->state & WIDGET_STATE_FOCUSED))
	{
		widget->state |= WIDGET_STATE_FOCUSED;
		widget->state &= ~(WIDGET_STATE_HIDE);
		widget_set_property(widget, "virtual_focus", "enable");
	}
#endif /*NO_OS*/

	return;
}

void widget_set_update(GuiWidget *widget, int update)
{
#ifndef NO_OS
	GuiCore *gui_core = GUI_GetCurrent();
#endif /*NO_OS*/
	GuiWidget *child = NULL, *parent = NULL;

	if(NULL == widget->object) {
		return;
	}

#ifndef NO_OS
	if(0 == strcasecmp("window", widget->classname))
		GUI_SetProperty(widget->name, "need_fade", "true");
#endif /*NO_OS*/
	if((widget->need_update == update) &&
	   (0 != strcasecmp("listview", widget->classname)) &&
	   (0 != strcasecmp("box", widget->classname)))
	{
		return;
	}

	if (widget)
	{
		widget->need_update = update;

		child = widget_get_firstchild(widget);
		while(child)
		{
			if(0 == strcasecmp("window", widget->classname)) {
				if(update != child->need_update)
					widget_set_update(child, update);
			} else {
				widget_set_update(child, update);
			}
			child = widget_get_nextsibling(child);
		}
		if(update)
		{
#ifdef NO_OS
			widget_need_update_count++;
#else /*NO_OS*/
			gui_core->widget_need_update_count++;
#endif /*NO_OS*/
		}
	}

	parent = widget_get_parent(widget);
	if(parent)
	{
		if((0 == strcasecmp("boxitem", parent->classname)) && (update == GUI_TRUE))
		{
			widget_set_update(parent, GUI_TRUE);
		}
	}
}

int widget_get_update(GuiWidget *widget)
{
	if (widget)
	{
		return (int)(widget->need_update);
	}

	return 0;
}

static void char_free(void *data)
{
	if (data)
	{
		GUI_FREE(data);
	}
}

int widget_set_property(GuiWidget *widget, const char *name, const char *value)
{
	if(NULL == widget || NULL == name || NULL == value)
	{
		return (1);
	}

	if(NULL == widget->property)
	{
		widget->property = hash_new(100, char_free);
	}
	if(widget->property)
	{
		hash_add(widget->property, name, GUI_STRDUP(value));
	}

	return (0);
}

const char *widget_get_property(GuiWidget *widget, const char *name)
{
	if(widget && widget->property && name)
	{
		return (const char *)hash_get(widget->property, name);
	}

	return (NULL);
}

const GuiWidget *widget_get_style(const char *name)
{
	GuiWidget *style = NULL;
#ifndef NO_OS
	GuiCore *gui_core = NULL;
	GuiConfig *config = NULL;
	const char *value = NULL;
	int i = 0, count = 0;

	if(NULL == name)
	{
		return (NULL);
	}

	gui_core = GUI_GetCurrent();

	if((gui_core->resource_config_table) && (gui_core->resource_config_stack)) {
		count = stack_count(gui_core->resource_config_stack);
		for(i = 0; i < count; i++) {
			config = (GuiConfig *)stack_get(gui_core->resource_config_stack, i);
			if(config && !config->invalid && config->style_widget) {
				style = gui_get_widget(config->style_widget, name);
			}
		}
	}

	if(NULL == gui_core->config.style_widget)
	{
		return (NULL);
	}
	if(NULL == style)
		style = gui_get_widget(gui_core->config.style_widget, name);
	if(NULL == style) {
		gui_core = &gui;
		style = gui_get_widget(gui_core->config.style_widget, name);
	}

	if(NULL == style)
	{
		return (NULL);
	}

	value = (const char *)widget_get_property(style, "backcolor");
	if(value)
	{
		getColorFromString(value, style->back_color);
	}
	else
	{
		style->back_color[0] = gui_core->config.gui_trans;
		style->back_color[1] = gui_core->config.gui_trans;
		style->back_color[2] = gui_core->config.gui_trans;
	}

	value = (const char *)widget_get_property(style, "forecolor");
	if(value)
	{
		getColorFromString(value, style->fore_color);
	}
	else
	{
		style->fore_color[0] = 0xffffff;
		style->fore_color[1] = 0xffffff;
		style->fore_color[2] = 0xffffff;
	}

#endif /*NO_OS*/
	return (style);
}

int widget_exec_event(GuiWidget *widget, void *usrdata)
{
	GuiCore *gui_core = NULL;
	GUI_Event *event = NULL;

	gui_core = GUI_GetCurrent();
	if ( (NULL == widget) || (NULL == usrdata) )
	{
		gxlogd("[GUI]widget_exec_event argument is NULL\n");
		return EVENT_TRANSFER_STOP;
	}

#ifndef NO_OS
	if(gui.config.root_widget == widget)
	{
		if(gui_core->root_event)
		{
			gui_core->root_event(widget->name, usrdata);
			return EVENT_TRANSFER_STOP;
		}
	}
#endif /*NO_OS*/

	if(widget->ops && widget->ops->event)
	{
#ifndef NO_OS
		event = (GUI_Event *)usrdata;
#endif /*NO_OS*/
		return (widget->ops->event(widget, usrdata));
	}

	return EVENT_TRANSFER_KEEPON;
}

int widget_got_focus(GuiWidget *widget)
{
	GuiWidget *focused = NULL;

	if(widget && widget->ops && widget->ops->gotfocus) {
		focused = wm_state_get_focus();
		if(focused && focused == widget) {
			return (widget->ops->gotfocus(widget));
		} else {
			return (0);
		}
	}
	return (0);
}

void widget_ops_init(void)
{
	//	widget_ops_hash = hash_new(32, NULL);
#ifndef NO_OS
	_register_widget("text",      &text_ops);
	_register_widget("button",    &button_ops);
	_register_widget("edit",      &edit_ops);
	_register_widget("image",     &image_ops);
	_register_widget("progbar",   &progbar_ops);
	_register_widget("header",    &header_ops);
	_register_widget("listitem",  &listitem_ops);
	_register_widget("listview",  &listview_ops);
	_register_widget("scrollbar", &scrollbar_ops);
	_register_widget("window",    &window_ops);
	_register_widget("combobox",  &combobox_ops);
	_register_widget("boxitem",   &boxitem_ops);
	_register_widget("box",       &box_ops);
	_register_widget("file_image", &file_image_ops);
	_register_widget("notepad",   &notepad_ops);
	_register_widget("spectrum",  &spectrum_ops);
	_register_widget("canvas",    &canvas_ops);
	_register_widget("sliderbar", &sliderbar_ops);
	_register_widget("timelist", &timelist_ops);
	_register_widget("timeitem", &timeitem_ops);
	_register_widget("prompt", &prompt_ops);
#else
	_register_widget("frame",			&frame_ops);
	_register_widget("label",			&label_ops);
	_register_widget("progress",		&progress_ops);
	_register_widget("list",			&list_ops);
	_register_widget("edit_box",	    &edit_box_ops);
#endif /*NO_OS*/
}

int widget_register_widget(const char *widget_type)
{
	int ret = 0;

	if(NULL == widget_type)
	{
		return (-1);
	}

#ifndef NO_OS
	if(0 == strcasecmp("t9", widget_type))
	{
		ret = _register_widget("t9", &t9_ops);
	}
	else if(0 == strcasecmp("file_image", widget_type))
	{
		ret = _register_widget("file_image", &file_image_ops);
	}
	else if(0 == strcasecmp("menu", widget_type)) {
		_register_widget("menu",      &menu_ops);
		_register_widget("menulist", &menulist_ops);
		_register_widget("menuitem",  &menuitem_ops);
	} else if(0 == strcasecmp("table", widget_type)) {
		_register_widget("table",     &table_ops);
	}

#endif

	return (ret);
}

int widget_register(const char *widget_name, GuiWidgetOps *ops)
{
	if((NULL == widget_name) || (NULL == ops))
	{
		return (1);
	}

	return (_register_widget(widget_name, ops));
}

int widget_notify_parent(GuiWidget *widget, void *data)
{
	GuiWidget *parent = NULL;

	if(NULL == widget)
	{
		return EVENT_TRANSFER_STOP;
	}

	parent = widget_get_parent(widget);
	if(NULL == parent)
	{
		return EVENT_TRANSFER_STOP;
	}

	if(parent->ops && parent->ops->event)
	{
		return parent->ops->event(parent, data);
	}

	return EVENT_TRANSFER_STOP;
}


int widget_notify_root(GuiWidget* widget, void* data)
{
#ifndef NO_OS
	GuiCore *gui_core = NULL;
	if(NULL == widget || NULL == data)
	{
		return 1;
	}

	gui_core = GUI_GetCurrent();

	if(gui_core->root_event)
	{
		gui_core->root_event(widget->name, data);
	}
#endif /*NO_OS*/

	return 0;
}

int widget_notify_top(GuiWidget* widget, void* data)
{
#ifndef NO_OS
	GuiCore *gui_core = NULL;
	GuiSignal *top_signal = NULL;

	if(NULL == data)
	{
		return EVENT_TRANSFER_STOP;
	}

	gui_core = GUI_GetCurrent();
	if(gui_core->top_event)
	{
		top_signal = (GuiSignal*)hash_get(gui_core->config.signal_table, "top");
		if(NULL == top_signal)
			return EVENT_TRANSFER_STOP;
		return gui_core->top_event((const char *)(top_signal->userdata), data);
	}
#endif /*NO_OS*/

	return EVENT_TRANSFER_KEEPON;
}



int widget_string_alignment(int alignment, int text_len, int font_height, GUI_Rect * src_rect, GUI_Rect * dst_rect)
{
	if (NULL == src_rect || NULL == dst_rect) {
		return (1);
	}

	*dst_rect = *src_rect;

	if((text_len < src_rect->w) && (0 != text_len))
	{
		switch (alignment & 0x3) {
		case GUI_TA_HCENTRE:
			if (text_len < src_rect->w) {
				dst_rect->x += (src_rect->w - text_len) / 2;
				dst_rect->w -= (src_rect->w - text_len) / 2;
			}
			break;
		case GUI_TA_RIGHT:
			if (text_len < src_rect->w) {
				dst_rect->x += (src_rect->w - text_len);
				dst_rect->w = text_len;
			}
			break;
		case GUI_TA_LEFT:
			break;
		default:
			break;
		}
	}

	switch (alignment & 0xc) {
	case GUI_TA_VCENTRE:
		if (font_height < src_rect->h) {
			dst_rect->y += (src_rect->h - font_height) / 2;
			dst_rect->h -= (src_rect->h - font_height) / 2;
		}
		break;
	case GUI_TA_BOTTOM:
		if (font_height < src_rect->h) {
			dst_rect->y += (src_rect->h - font_height);
			dst_rect->h -= (src_rect->h - font_height);
		}
		break;
	case GUI_TA_TOP:
		break;
	default:
		break;
	}

	return (0);
}

static void widget_signal_free(void *data)
{
	if(data)
	{
		GuiWidgetSignal *signal = (GuiWidgetSignal *)data;
		GUI_FREE(signal->handler_name);
		GUI_FREE(signal);
	}
}

void* widget_signal_new(GuiWidget *widget, const char *name, const char *handler)
{
	GuiWidgetSignal *widget_signal = NULL;
	GuiSignal *gui_signal = NULL;

	if(NULL == widget || NULL == name || NULL == handler)
	{
		return NULL;
	}

	//find signal app func in signal_table
#ifdef NO_OS
	gui_signal = (GuiSignal*)hash_get(minigui_config.signal_hash, handler);
	if(NULL == gui_signal) {
		return (NULL);
	}
#else
	GuiCore *gui_core = GUI_GetCurrent();
	GuiConfig *config = NULL;

	if(widget->from_resource && gui_core->resource_config_table) {
		config = (GuiConfig *)hash_get(gui_core->resource_config_table, widget->from_resource);
	}
	if(NULL == config)
		config = &(gui_core->config);
	gui_signal = (GuiSignal*)hash_get(config->signal_table, handler);
	if(NULL == gui_signal)
	{
		if(config != &(gui_core->config)) {
			config = &(gui_core->config);
		}
		gui_signal = (GuiSignal*)hash_get(config->signal_table, handler);
		if(NULL == gui_signal) {
			gxlogd("[GUI][SIG] cant find signal ( %s )'s app function\n", handler);
			return NULL;
		}
	}
#endif /*NO_OS*/

	//create widget's signal hash
	if(NULL == widget->signal)
	{
		widget->signal = hash_new(50, widget_signal_free);
		if (widget->signal == NULL) return NULL;
	}

	//add signal to widget
	widget_signal = (GuiWidgetSignal *)GUI_MALLOC(sizeof(GuiWidgetSignal));
	if(NULL == widget_signal) return NULL;
	widget_signal->handler_name = GUI_STRDUP(handler);
	widget_signal->signal = gui_signal;
	hash_add(widget->signal, name, widget_signal);

	return (void*)(widget_signal);
}

#ifndef NO_OS
static void gui_signal_free(void *data)
{
	if (data)
	{
		GUI_FREE(data);
	}
}
#endif /*NO_OS*/

void* gui_signal_new(const char *handlername, GuiSignalFunc func, void *userdata)
{
	GuiSignal *gui_signal = NULL;
#ifndef NO_OS
	GuiConfig *config = NULL;
	GuiSignalTable *signal_table = NULL;

	GuiCore *gui_core = GUI_GetCurrent();
	//create widget's signal hash
	if((gui_core->resource_config_table) && (gui_core->resource_config_stack)) {
		config = (GuiConfig *)stack_peek(gui_core->resource_config_stack);
		if(config && !config->invalid) {
			if(NULL == config->signal_table) {
				config->signal_table = hash_new(40,gui_signal_free);
				if(config->signal_table)
					return NULL;
			}
			signal_table = config->signal_table;
		}
	} else if(NULL == gui_core->config.signal_table) {
		gui_core->config.signal_table = hash_new(40,gui_signal_free);
		if (gui_core->config.signal_table == NULL) return NULL;
	}
	if(NULL == signal_table)
		signal_table = gui_core->config.signal_table;

	gui_signal = (GuiSignal *)GUI_MALLOC(sizeof(GuiSignal));
	if(NULL == gui_signal) return NULL;
	gui_signal->handler = func;
	gui_signal->userdata = userdata;
	hash_add(signal_table, handlername, gui_signal);
#endif /*NO_OS*/

	return gui_signal;
}

static int exec_global_event_data(GuiWidget *widget, const char *signal_name, void *userdata)
{
#ifdef NO_OS
	return (0);
#else /*NO_OS*/
	GuiCore *gui_core = NULL;
	GuiConfig *config = NULL;
	GuiSignal* signal_node = NULL;
	char *handler_name = NULL;

	gui_core = GUI_GetCurrent();
	if(gui_core && signal_name) {
		if((gui_core->resource_config_table) && (gui_core->resource_config_stack)) {
			config = (GuiConfig *)stack_peek(gui_core->resource_config_stack);
			if(NULL == config->signal)
				config = &(gui_core->config);
		} else {
			config = &(gui_core->config);
		}

		if(config->signal) {
			handler_name = hash_get(config->signal, signal_name);
			if(handler_name) {
				signal_node = (GuiSignal*)hash_get(config->signal_table, handler_name);
			} else if((gui_core->resource_config_table) && (gui_core->resource_config_stack)) {
				config = &(gui_core->config);
				signal_node = (GuiSignal*)hash_get(config->signal_table, handler_name);
			}

			if(signal_node && signal_node->handler && widget)
				return (signal_node->handler(widget->name, userdata));
			else
				return (-1);
		}
		return (-1);
	}

	return (-1);
#endif /*NO_OS*/
}

int exec_widget_signal_data(GuiWidget *widget, const char *signal_name, void *userdata)
{
	GuiWidgetSignal *widget_signal = NULL;

	if(NULL == widget || NULL == widget->signal)
	{
		return (1);
	}

	exec_global_event_data(widget, signal_name, userdata);

	widget_signal = (GuiWidgetSignal *)hash_get(widget->signal, signal_name);

	if(widget_signal)
		return (exec_widget_event_data(widget, widget_signal, userdata));
	else
		return (-1);
}

int exec_widget_event_data(GuiWidget *widget, GuiWidgetSignal *signal, void *userdata)
{
#ifndef NO_OS
	int ret = 0;
	unsigned int start = 0, end = 0;

	if((NULL == widget) ||
			(NULL == widget->object) ||
			(NULL == signal) ||
			(NULL == signal->signal) ||
			(NULL == signal->signal->handler))
	{
		return EVENT_TRANSFER_KEEPON;
	}

	GUI_Debug_Print("[GUI] ****************From %s******** signal : %s ****************start\n", widget->name,
					 signal->handler_name);

	if(userdata)
	{
		start = hd_get_tick();
		ret = signal->signal->handler(widget->name, userdata);
		end = hd_get_tick();
		if((end - start) > 1000)
		{
			gxlogd("[GUI] ######## widget : %s ###### signal : %s excute too long ########\n", widget->name,
					signal->handler_name);
		}
		GUI_Debug_Print("[GUI] ****************From %s******** signal : %s ****************end\n", widget->name,
						signal->handler_name);
		return (ret);
	}
	else
	{
		start = hd_get_tick();
		ret = signal->signal->handler(widget->name, signal->signal->userdata);
		end = hd_get_tick();
		if((end - start) > 1000)
		{
			gxlogd("[GUI] ######## widget : %s ###### signal : %s excute too long ########\n", widget->name,
					signal->handler_name);
		}
		GUI_Debug_Print("[GUI] ****************From %s******** signal : %s ****************end\n", widget->name,
						signal->handler_name);
		return (ret);
	}

	GUI_Debug_Print("[GUI] ****************From %s******** signal : %s ****************end\n", widget->name,
					 signal->handler_name);
#endif /*NO_OS*/
	return EVENT_TRANSFER_STOP;
}

GuiWidget *gui_get_window(const char *name)
{
	GuiWidget *window = NULL, *child = NULL;
	GuiCore *gui_core = NULL;
	GuiConfig *config = NULL;
	int i = 0, count = 0;

	if ( NULL == name) return NULL;
	gui_core = GUI_GetCurrent();

#ifdef NO_OS
	window = minigui_config.root_widget;
#else
	// From Resource Package
	if((gui_core->resource_config_table) && (gui_core->resource_config_stack)) {
		count = stack_count(gui_core->resource_config_stack);
		for(i = 0; i < count; i++) {
			config = (GuiConfig *)stack_get(gui_core->resource_config_stack, i);
			if(config && !config->invalid) {
				window = config->root_widget;

				if(window)
					child = widget_get_firstchild(window);
				while (child) {
					if (strcasecmp(child->name, name) == 0) {
						return child;
					}
					child = widget_get_nextsibling(child);
				}
			}
		}
	}

	window = gui_core->config.root_widget;
#endif /*NO_OS*/

#ifdef NO_OS
	window = minigui_config.root_widget;
#else
	window = gui.config.root_widget;
#endif /*NO_OS*/
	if(NULL == window)
	{
		gxlogd("[GUI]Maybe you have not initialized the GUI, Please check it!\n");
		return (NULL);
	}

	if (0 == strcasecmp(name, "root"))
	{
#ifdef NO_OS
		return minigui_config.root_widget;
#else
		return gui_core->config.root_widget;
#endif /*NO_OS*/
	}
	else if((gui_core != &gui) &&
#ifdef NO_OS
			(0 == strcasecmp(name, minigui_config.root_widget->name))
#else
			(0 == strcasecmp(name, gui_core->config.root_widget->name))
#endif /*NO_OS*/
	       )
	{
#ifdef NO_OS
		return minigui_config.root_widget;
#else
		return gui_core->config.root_widget;
#endif /*NO_OS*/
	}

	child = window->first_child;
	while (child)
	{
		if (strcasecmp(child->name, name) == 0)
		{
			return child;
		}
		child = child->next_sibling;
	}

	return NULL;
}

static GuiWidget *_check_widgets(GuiWidget *widget, const char *name)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiWidget *result_widget = NULL;
	GuiWidget *child = widget->first_child;

	if((gui_core != &gui) &&
#ifdef NO_OS
	   (0 == strcasecmp(name, minigui_config.root_widget->name))
#else
	   (0 == strcasecmp(gui_core->config.root_widget->name, name))
#endif /*NO_OS*/
	   )
	{
#ifdef NO_OS
		return (minigui_config.root_widget);
#else
		return (gui_core->config.root_widget);
#endif /*NO_OS*/
	}

	while (child)
	{
		if (child->name && (strcasecmp(child->name, name) == 0))
		{
			if(result_widget)
			{
#ifdef GXDBG
				GUI_Debug_Print("[GUI] There are more than 1 widgets name as %s, Please check!\n", name);
#endif
				return (NULL);
			}
			else
			{
				result_widget = child;
			}
		}
		child = child->next_sibling;
	}

	child = widget->first_child;
	while (child)
	{
		GuiWidget *find = _check_widgets(child, name);
		if (find != NULL)
		{
			if(result_widget)
			{
#ifdef GXDBG
				GUI_Debug_Print("[GUI] There are more than 1 widgets name as %s, Please check!\n", name);
#endif
				return (NULL);
			}
			else
			{
				result_widget = find;
			}
		}
		child = child->next_sibling;
	}

	return (result_widget);
}

GuiWidget *gui_get_widget(GuiWidget *widget, const char *name)
{
	GuiCore *gui_core = NULL;
	GuiConfig *config = NULL;
	GuiWidget *child = NULL;
	int i = 0, count = 0;

	if ( NULL == name) return NULL;

	gui_core = GUI_GetCurrent();
	if ( NULL == widget) {
#ifdef NO_OS
		widget = minigui_config.root_widget;
#else
		if(NULL == gui_core->config.focus_widget) {
			widget = gui_core->config.root_widget;
		} else {
			widget = gui_core->config.focus_widget->parent;
		}
		if(NULL == widget) {
			if((gui_core->resource_config_table) && (gui_core->resource_config_stack)) {
				count = stack_count(gui_core->resource_config_stack);
				for(i = 0; i < count; i++) {
					config = (GuiConfig *)stack_get(gui_core->resource_config_stack, i);
					if(config && !config->invalid && config->root_widget) {
						widget = gui_get_widget(config->root_widget, name);
						if(widget)
							return (widget);
					}
				}
			} else {
				widget = gui.config.root_widget;
			}
		} else if(0 == strcasecmp(widget->name, name)) {
			return (widget);
		}
#endif /*NO_OS*/
	}

	if (0 == strcasecmp(name, "root"))
	{
#ifdef NO_OS
		return minigui_config.root_widget;
#else
		return gui.config.root_widget;
#endif /*NO_OS*/
	}

	if(NULL == widget)
	{
		gxlogd("[GUI]Maybe you have not Initial the GUI, Please check it!\n");
		return (NULL);
	}

	child = _check_widgets(widget, name);
#ifndef NO_OS
	if((NULL == child) &&
	   (gui_core->config.focus_widget) &&
	   (widget == gui_core->config.focus_widget->parent)) {
		widget = gui.config.root_widget;
		child = _check_widgets(widget, name);
	}
#endif /*NO_OS*/

#ifndef NO_OS
	if(NULL == child)
	{
		widget = gui_core->config.style_widget;
		child = _check_widgets(widget, name);
	}
#endif /*NO_OS*/

	if((NULL == child) && (gui_core != &gui))
	{
#ifdef NO_OS
		widget = minigui_config.root_widget;
#else
		widget = gui.config.root_widget;
#endif /*NO_OS*/
		child = _check_widgets(widget, name);
	}

	return (child);
}

const char *widget_get_signal(GuiWidget *widget, const char *signal_name)
{
#ifndef NO_OS
	GuiCore *gui_core = NULL;
	GXxmlNodePtr node;
	GXxmlDocPtr doc;
	char *path, basename[256];
	static char result_name[256];

	if (widget->init != 0)
		return NULL;

	gui_core = GUI_GetCurrent();
	GxCore_BasePath(gui_core->config.files.file_xml_widget, basename);
	path = GxCore_CatPath(basename, widget->xml_filename);
	doc = GXxmlParseFile(path);

	if (doc == NULL) {
		GUI_FREE(path);
		return NULL;
	}
	GUI_FREE(path);

	node = XML_GET_ROOT(doc);
	node = XML_GET_CHILD(node);
	//widget_parse_GXxmlnode(widget, );
	while (node) {
		if (0 == GXxmlStrcmp(node->name, SIGNAL_STR) ) {
			GXxmlNodePtr signal_node = XML_GET_CHILD(node);
			while (NULL != signal_node) {
				xmlChar *content = GXxmlNodeGetContent(signal_node);
				if(0 == GXxmlStrcmp((unsigned char *)signal_name, signal_node->name))
				{
					memset(result_name, 0, 256);
					strcpy((char *)result_name, (char *)content);
					XML_FREE(content);
					GXxmlFreeDoc(doc);
					return (result_name);
				}
				XML_FREE(content);

				signal_node = signal_node->next;
			}
		}
		node = node->next;
	}
	GXxmlFreeDoc(doc);

#endif /*NO_OS*/
	return NULL;
}

#ifndef NO_OS
static void widget_img_check(GuiCore *gui_core, const char * img_id, const image_desc *fix_desc)
{
	image_desc *desc = NULL;
	GuiConfig *config = NULL;
	int i = 0, count = 0;

	desc = image_cache_check(gui_core->config.image_cache, img_id);
	if(desc) {
		widget_img_clear(&desc);
	}

	desc = NULL;
	if((gui_core->resource_config_table) && (gui_core->resource_config_stack)) {
		count = stack_count(gui_core->resource_config_stack);
		for(i = 0; i < count; i++) {
			config = (GuiConfig *)stack_get(gui_core->resource_config_stack, i);
			if(config && !config->invalid && config->image_cache)
				desc = image_cache_check(config->image_cache, img_id);

			if((desc) && (fix_desc) && (fix_desc != desc)) {
				widget_img_clear(&desc);
				desc = NULL;
			}
		}
	}
}
#endif /*NO_OS*/

image_desc *widget_img_load(const char* img_id)
{
	GuiCore *gui_core = NULL;
	image_desc *desc = NULL;
	GuiConfig *config = NULL;
	int i = 0, count = 0;

	gui_core = GUI_GetCurrent();
#ifndef NO_OS
	if((gui_core->resource_config_table) && (gui_core->resource_config_stack)) {
		count = stack_count(gui_core->resource_config_stack);
		for(i = 0; i < count; i++) {
			config = (GuiConfig *)stack_get(gui_core->resource_config_stack, i);
			if(config && !config->invalid && config->image_cache && img_id) {
				image_cache_get((ImageCache *)(config->image_cache), img_id, &desc);
				if(desc) {
					// 获取完当前 desc 后，为节省内存，将同等资源包、主题包下相同 ID 的图片放入 free 队列
					widget_img_check(gui_core, img_id, desc);
					return (desc);
				}
			}
		}
	}

	if (img_id)
		image_cache_get((ImageCache*)gui_core->config.image_cache, img_id, &desc);

	if((NULL == desc) ||
	   ((desc) && (NULL == desc->data)))
	{
		if((img_id) && (gui_core != &gui))
			image_cache_get((ImageCache*)gui.config.image_cache, img_id, &desc);
	}
#endif /*NO_OS*/

	return desc;
}

int widget_check_img_mode(const char *value)
{
	if(NULL == value)
	{
		return (GUI_FALSE);
	}

	while(*value)
	{
		if('[' == *value)
			return (GUI_TRUE);
		value++;
	}
	return (GUI_FALSE);
}

static void _get_img_group_value(const char* img_id, char **value0, char **value1, char **value2)
{
	char *id = GUI_STRDUP(img_id);
	char *ptail = NULL,  *value  = NULL, *id_value = NULL;
	char *img_value = NULL;
	int offset = 0;

	id_value = id;
	while(*id)
	{
		if(*id == '[')
			break;
		id++;
	}

	if('[' == *id)
		id++;

	ptail = id + strlen(id) - 1;
	while(ptail != id)
	{
		if(*ptail == ']')
			break;
		ptail--;
	}

	value = GUI_MALLOCZ(ptail - id + 10);
	memcpy(value, id, ptail - id);

	img_value = strtok(value, ",");
	if(img_value)
	{
		*value0 = GUI_STRDUP(img_value);
		if(NULL == *value0)
			goto EXIT;
		id = *value0;
		while(*id == ' ')
		{
			id++;
		}
		offset = strlen(id);
		memmove(*value0, id, offset);
		(*value0)[offset] = '\0';
	}
	img_value = strtok(NULL, ",");
	if(img_value)
	{
		*value1 = GUI_STRDUP(img_value);
		if(NULL == *value1)
			goto EXIT;
		id = *value1;
		while(*id == ' ')
		{
			id++;
		}
		offset = strlen(id);
		memmove(*value1, id, offset);
		(*value1)[offset] = '\0';
	}
	img_value = strtok(NULL, ",");
	if(img_value)
	{
		*value2 = GUI_STRDUP(img_value);
		if(NULL == value2)
			goto EXIT;
		id = *value2;
		while(*id == ' ')
		{
			id++;
		}
		offset = strlen(id);
		memmove(*value2, id, offset);
		(*value2)[offset] = '\0';
	}


EXIT:
	GUI_FREE(id_value);
	GUI_FREE(value)
}

void widget_img_group_load(const char* img_id, image_desc **img_l, image_desc **img_m, image_desc **img_r)
{
	char *value[3] = {NULL, NULL, NULL};

	if(NULL == img_id)
	{
		return;
	}

	_get_img_group_value(img_id, &(value[0]), &(value[1]), &(value[2]));
	if((NULL != value[0]) || (NULL == *img_l))
	{
		*img_l = widget_img_load(value[0]);
	}
	else
	{
		*img_l = NULL;
	}

	if((NULL != value[1]) || (NULL == *img_m))
	{
		*img_m = widget_img_load(value[1]);
	}
	else
	{
		*img_m = NULL;
	}

	if((NULL != value[2]) || (NULL == *img_r))
	{
		*img_r = widget_img_load(value[2]);
	}
	else
	{
		*img_r = NULL;
	}

	GUI_FREE(value[0]);
	GUI_FREE(value[1]);
	GUI_FREE(value[2]);
}

int widget_img_tile_draw(GuiSurface *surface,
			 GUI_Rect *rect,
			 image_desc *img_l,
			 image_desc *img_m,
			 image_desc *img_r,
			 int color)
{
	int ret = 0;
#ifndef NO_OS
	int x = 0, y = 0, w = 0, h = 0;
	GuiSurface *img_surface = NULL;
	GAL_Rect src_rect = {0}, dst_rect = {0};

	if((NULL == surface) || (NULL == rect))
	{
		return (1);
	}

	if((img_l) && (img_r)) {
		if((img_l->width + img_r->width) > rect->w) {
			goto DRAW_RECT;
		}
	}

	x = rect->x;
	y = rect->y;
	w = rect->w;
	h = rect->h;
	if(img_l)
	{
		ret = gal_img_desc(surface, img_l, x, y, img_l->width, h);
		x += img_l->width;
		w -= img_l->width;
	}

	if(w <= 0)
		return (1);

	if(img_r)
	{
		ret |= gal_img_desc(surface, img_r, x + w - img_r->width, y, img_r->width, h);
		w -= img_r->width;
	}

	if(w <= 0)
		return (1);

	if(img_m) {
		img_surface = image_desc_surface(img_m);
		if(NULL == img_surface) {
			return (-1);
		}

		src_rect.x = src_rect.y = 0;
		src_rect.w = img_surface->sf.width;
		src_rect.h = img_surface->sf.height;

		dst_rect.x = x;
		dst_rect.y = y;
		dst_rect.w = w;
		dst_rect.h = h;
		gdi_begin();
		ret |= hd_tile_surface(img_surface, &src_rect, surface, &dst_rect);
		gdi_end();
		hd_free_surface(img_surface);
	} else
DRAW_RECT:
	{
		GAL_Rect rect = {0};

		rect.x = x;
		rect.y = y;
		rect.w = w;
		rect.h = h;
		color = hd_color2index(surface, color);
		ret |= gal_fillrect(surface, &rect, color);
	}
#endif /*NO_OS*/
	return (ret);
}

int widget_img_group_draw(GuiSurface *surface,
			  GUI_Rect *rect,
			  image_desc *img_l,
			  image_desc *img_m,
			  image_desc *img_r,
			  int color)
{
	int ret = 0;
#ifndef NO_OS
	int x = 0, y = 0, w = 0, h = 0;

	if((NULL == surface) || (NULL == rect))
	{
		return (1);
	}

	x = rect->x;
	y = rect->y;
	w = rect->w;
	h = rect->h;
	if(img_l)
	{
		ret = gal_img_desc(surface, img_l, x, y, img_l->width, h);
		x += img_l->width;
		w -= img_l->width;
	}

	if(w <= 0)
		return (1);

	if(img_r)
	{
		ret |= gal_img_desc(surface, img_r, x + w - img_r->width, y, img_r->width, h);
		w -= img_r->width;
	}

	if(w <= 0)
		return (1);

	if(img_m) {
		ret |= gal_img_desc(surface, img_m, x, y, w, h);
	} else {
		GAL_Rect rect = {0};

		rect.x = x;
		rect.y = y;
		rect.w = w;
		rect.h = h;
		color = hd_color2index(surface, color);
		ret |= gal_fillrect(surface, &rect, color);
	}
#endif /*NO_OS*/

	return (ret);
}

int widget_img_group_draw_mirror(GuiSurface *surface,
				 GUI_Rect *rect,
				 image_desc *img_l,
				 image_desc *img_m,
				 image_desc *img_r,
				 int color,
				 GxReverseMode mode)
{
	GuiSurface *tmp_surface = NULL, *mirror_surface = NULL;
	GAL_Rect tmp_rect = {0}, dst_rect = {0};
	GUI_Rect com_rect = {0};
	int ret = 0;

	if((NULL == surface) || (NULL == rect))
	{
		return (1);
	}

	tmp_rect.x = tmp_rect.y = 0;
	tmp_rect.w = rect->w;
	tmp_rect.h = rect->h;
	tmp_surface = hd_get_surface0(&tmp_rect, surface->bpp, surface->sf.color_format, NULL, GUI_FALSE);
	if(NULL == tmp_surface) {
		return (1);
	}

	mirror_surface = hd_get_surface0(&tmp_rect, surface->bpp, surface->sf.color_format, NULL, GUI_FALSE);
	if(NULL == mirror_surface) {
		return (1);
	}

	com_rect.x = com_rect.y = 0;
	com_rect.w = tmp_rect.w;
	com_rect.h = tmp_rect.h;
	gdi_begin();
	ret = widget_img_group_draw(tmp_surface, &com_rect, img_l, img_m, img_r, color);
	gdi_end();

	ret |= gal_mirror_image_surface(tmp_surface, &tmp_rect, mirror_surface, &tmp_rect, REVERSE_HORIZONTAL);
	gal_free_surface(tmp_surface);
	dst_rect.x = rect->x;
	dst_rect.y = rect->y;
	dst_rect.w = rect->w;
	dst_rect.h = rect->h;
	gdi_begin();
	ret |= gal_stretch_surface(mirror_surface, &tmp_rect, surface, &dst_rect);
	gdi_end();
	gal_free_surface(mirror_surface);

	return (ret);
}

int widget_img_clear(image_desc **desc)
{
#ifdef NO_OS
	return (0);
#else
	GuiCore *gui_core = NULL;
	GuiConfig *config = NULL;

	if((NULL == desc) || (NULL == *desc))
	{
		return (0);
	}

	gui_core = GUI_GetCurrent();
	if (gui_core->resource_config_table && (*desc)->theme) {
		config = hash_get(gui_core->resource_config_table, (*desc)->theme);
	}
	if(config) {
		return image_cache_clear((ImageCache *)(config->image_cache), (image_desc**)desc);
	} else {
		return image_cache_clear((ImageCache *)gui_core->config.image_cache, (image_desc**)desc);
	}
#endif /*NO_OS*/
}

void widget_img_group_clear(image_desc **img_l, image_desc **img_m, image_desc **img_r)
{
	if((img_l) && (*img_l))
		widget_img_clear(img_l);

	if((img_m) && (*img_m))
		widget_img_clear(img_m);

	if((img_r) && (*img_r))
		widget_img_clear(img_r);
}

extern status_t create_widget_list(GuiWidget *parent_widget, GXxmlNodePtr GXxml_node);

static status_t widget_parse_nodetree(GuiWidget *widget, GXxmlNodePtr node)
{
#ifndef NO_OS
	while (node) {
		if ( 0 == GXxmlStrcmp(node->name, PROPERTY_STR) ) {
			GXxmlNodePtr property_node = XML_GET_CHILD(node);
			while (NULL != property_node) {
				xmlChar *content = GXxmlNodeGetContent(property_node);
				//GUI_XML_Printf("set_property: %s, %s\n",(char *)property_node->name, (char *)content);
				widget_set_property(widget, (char *)property_node->name, (char *)content);//"tabstop" ??
				XML_FREE(content);

				property_node = property_node->next;
			}
		}
		else if ( 0 == GXxmlStrcmp(node->name, SIGNAL_STR) ) {
			GXxmlNodePtr signal_node = XML_GET_CHILD(node);
			while (NULL != signal_node) {
				xmlChar *content = GXxmlNodeGetContent(signal_node);
				//GUI_XML_Printf("set_signal: %s, %s\n",(char *)signal_node->name, (char *)content);
				widget_signal_new(widget, (char *)signal_node->name, (char *)content);
				XML_FREE(content);

				signal_node = signal_node->next;
			}
		}
		else if ( 0 == GXxmlStrcmp(node->name, CHILDREN_STR) ) {
			GXxmlNodePtr child_node = XML_GET_CHILD(node);
			while (NULL != child_node) {
				if (0 == GXxmlStrcmp(child_node->name, WIDGET_STR))
					create_widget_list(widget, child_node);

				child_node = child_node->next;
			}
		}
		node = node->next;
	}
#endif /*NO_OS*/

	return GXCORE_SUCCESS;
}

GuiWidget *widget_load_from_GXxml(const unsigned char* string)
{
	GuiCore *gui_core = NULL;
	GuiWidget *widget = NULL;
	int size = 0;
	GXxmlDocPtr ret;
	GXxmlNodePtr root, cur;
	xmlChar *widget_name = NULL, *widget_classid = NULL, *widget_style = NULL;

	if(NULL == string)
	{
		return (NULL);
	}

	size = strlen((char *)string) + 1;
	ret = GXxmlParseMemory((char *)string, size);

	root = XML_GET_ROOT(ret);
	if (root == NULL)
	{
		gxlogd( "[GUI]XML string is wrong!\n");
		GXxmlFreeDoc(ret);
		return (NULL);
	}

	gui_core = GUI_GetCurrent();
	cur = root;
	while (NULL != cur)
	{
		if (0 == GXxmlStrcmp(cur->name, WIDGET_STR))
		{
			widget_name = GXxmlGetProp(cur, NAME_STR );
			widget_classid = GXxmlGetProp(cur, CLASS_STR);
			widget_style = GXxmlGetProp(cur, STYLE_STR);

			widget = widget_new((char *)widget_name, (char *)widget_classid, (char *)widget_style);
			if(NULL == widget)
			{
				return (NULL);
			}

			XML_FREE(widget_style);
			XML_FREE(widget_classid);
			XML_FREE(widget_name);

			widget_parse_nodetree(widget, XML_GET_CHILD(cur));
		}
		cur = cur->next;
	}

	GXxmlFreeDoc(ret);

	if(0 == strcasecmp("window", widget->classname))
	{
#ifdef NO_OS
		widget_add_child(minigui_config.root_widget, widget);
#else
		widget_add_child(gui.config.root_widget, widget);
#endif /*NO_OS*/
	}

	return (widget);
}

int stringlist_index(const char **list, const char *name)
{
	int i = 0;

	while (list[i] != NULL) {
		if (strcasecmp(name, list[i]) == 0)
			return i;
		i++;
	}

	return -1;
}
