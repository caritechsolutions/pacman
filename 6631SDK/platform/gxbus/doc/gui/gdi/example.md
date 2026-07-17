# GDI应用场景

## 播放音视频时的浮动字幕

当播放时，需要在屏幕底部显示字幕，此时往往需要独占GUI的主surface。

```cpp

GuiSurface *surface = NULL;
GAL_Rect rect = {0, 0, 1280, 200};// 一般在屏幕下班部分
GXGDI_Rect fill_rect = {0, 0, 1280, 200};

surface = hd_get_surface(&rect, 16, NULL, FALSE);
if(NULL == surface) {
    assert("Get surface failed.\n");
}

char *layer_name = GxGDI_LayerRegister(surface, 0, 520); // 在屏幕底部
if(NULL == layer_name) {
    assert("Register surface failed.\n");
}

GXGDI_FillRect(surface, GUI_TRANS_COLOR, &fill_rect);

GXGDI_DrawString(surface,
                 "string",
                 &fill_rect,
                 GUI_TA_HCENTRE | GUI_TA_VCENTRE,
                 0xFFFFFF,
                 GDI_STRING_PARAGRAPH);
//...

GxGDI_LayerUpdate(layer_name);

//...

GxGDI_LayerUnregister(layer_name);

```

## 部分GDI级的CA系统/数据广播系统OSD部分移植

```cpp

// ...

xxx_status_t XXX_CAS_InitOSD(void)
{
    // ...
    GuiSurface *surface = NULL;
    GAL_Rect rect = {0, 0, 1280, 720};

    surface = hd_get_surface(&rect, 32, NULL, FALSE);
    if(NULL == surface)
    {
        assert("Get surface failed.\n");
    }

    char *layer_name = GxGDI_LayerRegister(surface, 0, 0); // 在屏幕底部
    if(NULL == layer_name)
    {
        assert("Register surface failed.\n");
    }

    XXX_CAS_OSD_Block->surface = surface;
    XXX_CAS_OSD_Block->layer_name = layer_name;
    // ...
}

xxx_status_t XXX_CAS_FillRect(XXXRect *rect, XXX_Color color)
{
    GXGDI_Rect fill_rect = {0};

    if(NULL == XXX_CAS_OSD_Block->surface)
    {
        assert("Fill NULL surface.\n");
    }

    fill_rect.x = rect->x;
    fill_rect.y = rect->y;
    fill_rect.w = rect->width;
    fill_rect.h = rect->height;
    GXGDI_FillRect(XXX_CAS_OSD_Block->surface, &fill_rect, color);
    GxGDI_LayerUpdate(layer_name);

    return (xxx_OK);
}

xxx_status_t XXX_CAS_Quit(void)
{
    // ...
    GxGDI_LayerUnregister(XXX_CAS_OSD_Block->layer_name);
    // ...
}

```

数据广播等三方应用移植也是如此。


## 游戏移植

一般移植的游戏源码都是直接采用GDI形式的。

```cpp

// ...

SIGNAL_HANDLER int app_box_game_create(GuiWidget *widget, void *usrdata)
{
    // ...
    GuiSurface *surface = NULL;
    GAL_Rect rect = {0, 0, 1280, 720};

    surface = hd_get_surface(&rect, 32, NULL, FALSE);
    if(NULL == surface)
    {
        assert("Get surface failed.\n");
    }

    char *layer_name = GxGDI_LayerRegister(surface, 0, 0); // 在屏幕底部
    if(NULL == layer_name)
    {
        assert("Register surface failed.\n");
    }

    XXX_Game_Block->surface = surface;
    XXX_Game_Block->layer_name = layer_name;
    // ...
}

SIGNAL_HANDLER int app_box_game_destroy(GuiWidget *widget, void *usrdata)
{
    // ...
    GxGDI_LayerUnregister(XXX_Game_Block->layer_name);
    // ...
}

SIGNAL_HANDLER int app_box_game_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    // ...

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_KEYDOWN:
        {
              switch(event->key.sym)
              {
                  case XXX_KEY:
                      pDesc = gal_img_load(pDesc, "/mnt/ott/ccc.jpg");
                      if(pDesc)
                          GXGDI_DrawImage(XXX_Game_Block->surface, pDesc, x, y);
                      break;
                  default:
                      break; 
              }
        }
    }
}


// ...

```

# GUI应用场景

## 阻塞框

在之前的GUI应用中，经常需要出现阻塞框，特别是收到service消息后被阻塞框阻塞，会导致应用下一批消息无法收到，造成异常。如果能在其他服务（线程）中弹出阻塞框，阻塞调用，不影响主GUI，即不会导致阻塞与消息丢失。

## 成型应用移植

在应用中，如BU1国内项目组开发了一套OTT应用，是1920 * 1280的界面，BU1在不考虑资源消耗的情况下，在1280 * 720的已有界面下，快速添加一个带OTT应用的方案给客户作demo。之前，最多的工作量就是调整XML界面，如果不考虑资源消耗，可在短时直接移入一个OTT应用。这样即不存在重名，也不存在界面调整等情况。只要界面失真在可控范围，即可作为demo推出。

## 需要分开加载资源的应用（如：升级界面）

在如升级界面或某些其他界面中，需要单独的字库文件作为资源。而此字库（一般是点阵字库）在整个方案中，除了该界面都不使用。而在这类界面中，往往可以接受加载相对较慢等问题，在退出这些界面时，释放资源。从而达到资源的最优配置。

theme.xml

```xml

<?xml version="1.0" encoding="UTF8" standalone="no"?>
<config>
    <file_widget>widget/widget.xml</file_widget>
    <file_i18n>language/i18n.xml</file_i18n>
    <file_image>image/image.xml</file_image>
    <file_font>font/font.xml</file_font>
    <file_color>color/color.xml</file_color>
    <key_mode>refuse</key_mode>
</config>

```

可以缺失某些资源，缺失的资源则使用全局资源（font）：

```xml

<?xml version="1.0" encoding="UTF8" standalone="no"?>
<config>
    <file_widget>widget/widget.xml</file_widget>
    <file_i18n>language/i18n.xml</file_i18n>
    <file_image>image/image.xml</file_image>
    <file_color>color/color.xml</file_color>
    <key_mode>block</key_mode>
</config>

```

**如以上两个XML，其中，增加了此GUI是阻塞/拒绝消息模式。此配置，对于主GUI无效。**

```xml

<?xml version="1.0" encoding="GB2312" standalone="no"?>
<interface>
    <x>255</x>
    <y>170</y>
    <width>580</width>
    <height>320</height>
    <bpp>32</bpp>
    <widget class="window" style="default" name="wnd_pop_tip"/>
</interface>

```

**虚拟GUI增加起始x、y位置，若没有该项配置，则默认为0。且在16位色方案中，可采用32位色的theme主题包。**

整体而言，GUI的“高架桥”模式的应用场景还有很多。比如部分CA的界面可作单独开发维护。


