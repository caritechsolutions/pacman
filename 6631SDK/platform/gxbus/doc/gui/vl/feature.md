# 概述

在应用开发中，遇到需要独占GDI或GUI所有资源，与GUI互不干扰，完成显示的应用场景。在GUI只有一个层次的情况下，需要虚拟出多个GDI层次或者虚拟的GUI来，从应用角度，感觉有多个GDI层次或多个GUI可供应用独占使用。灵活地提供了GDI与GUI的接口，供应用视场景、需求而定，选择具体的应用方式。

另外，尤其是第三方开发，如CA数据广播既要求独立开发，也要求与主应用并存，就需要在GUI与GDI资源上能够具有独占性。这样，这些第三方的软件运行对于内存的越界、泄漏对主GUI的影响会减小很多。同时，也增强了应用代码的复用性以及可移植性。

开发缘由，通俗地说就是在某些应用场景中，一个OSD层或一个GUI已经不能满足应用需求了。

# GDI部分说明

## 虚拟OSD层的引入

在应用需要直接通过GDI接口，完成绘制的情况下，应用希望的是独占GUI的绘制资源，即可以认为是显示buffer。应用只须申请、释放surface，并将其注册到虚拟层。注册与反注册的过程以及数据配置和层次命名，都由GUI完成。

虚拟OSD层，可以认为是浮在GUI所在的主OSD层上的虚拟层，与主OSD层不互相影响，与下（主/虚拟）层之间的透明，使用清层操作或采用填充GUI透明色。各虚拟层之间顺序可以交换，与主OSD层的顺序不能交换，即虚拟层永远在主层上。

在芯片有硬件OSD层的情况下，会先使用硬件OSD层，然后使用软件虚拟OSD层。在内存足够的条件下，允许开无限多层OSD层。所有GDI一级的具体操作，与之前gdi_core.h中所有接口相同。

## 虚拟OSD层与主OSD层的异同

- 1) 虚拟OSD层的宽高可以小于、等于或大于实际OSD层的宽高。当大于实际OSD层的宽高时，会进行缩放，可能会影响显示效果。当小于实际OSD层的宽高时，不进行缩放，可以指定虚拟层的X、Y起始显示位置；

- 2) 所有虚拟层之间的前后关系可以调整；

- 3) 凡是实际的虚拟层（非硬件OSD层）都不能调整层的透明度；

- 4) 虚拟层的色深（16位/32位）可与主OSD层色深不同，但可能会带来失真；

- 5) 主层可以使用索引色（8位色）方案，虚拟层考虑到8位混合方面需要色板统一，故虚拟层不可以使用8位色。即“高架桥”多GUI模式，不适用于8位色方案。

## 主要接口

### 物理层接口

[GxGDI_HardwareLayerRegister](./group__GDI_1gab9dc7d7e713208b8f22682875f79547c.md)

[GxGDI_HardwareLayerGet](./group__GDI_1ga5d99aadcc84faa1de7f134ccfb290038.md)

[GxGDI_HardwareLayerEnable](./group__GDI_1ga8d954a3f1522e32846ad547cb4e0e4bb.md)

[GxGDI_HardwareLayerAlpha](./group__GDI_1ga0550d57cc10af8fa9144176de19655c5.md)

[GxGDI_HardwareLayerColorKey](./group__GDI_1ga36a17fe082a0b62ed081bcc905697311.md)

[GxGDI_HardwareLayerSetPriority](./group__GDI_1gad73a0192406a2021187924c4a27d2aa8.md)

[GxGDI_HardwareLayerGetPriority](./group__GDI_1gad73a0192406a2021187924c4a27d2aa8.md)

### 虚拟层接口

[GxGDI_LayerRegister](./group__GDI_1ga33915d808690497b8a58fa6d916391ae.md)

[GxGDI_LayerUnregister](./group__GDI_1ga41de3b1e107d342eeed475e1ef232fa8.md)

[GXGDI_LayerClear](./group__GDI_1ga31ebd8c4bc151ecbbf95ab0cf62e9bc4.md)

[GxGDI_LayerUpdate](./group__GDI_1gad5e1becbe14accec822c307a5f8c5842.md)

[GxGDI_LayerUpdateSync](./group__GDI_1gafcd61f5153b3241da34d21059c888c95.md)

[GxGDI_LayerGetSurface](./group__GDI_1ga0926eb9a1c76a67c5008eac656f53ae6.md)

[GxGDI_LayerEnable](./group__GDI_1ga817fa8eebd207266e3a3a9925280b335.md)

[GxGDI_LayerInfomation](./group__GDI_1ga3ba2630c2516e0619233cb915f08c1b2.md)

[GxGDI_LayerSetPosition](./group__GDI_1ga117ff5c70dc2c8fbb49227c0b0ef165c.md)

[GXGDI_SetLayerPriority](./group__GDI_1ga3ec203a7ac2f9c4734abedb09d8d95b4.md)

[GXGDI_GetLayerPriority](./group__GDI_1gaf2cf0953e006bd2c0c51c8ac11696b54.md)

### 字幕滚动接口

[GXGDI_StartRollString](./group__GDI_1ga8c7c263338c24c050ae3ef9560bcbe23.md)

[GXGDI_StopRollString](./group__GDI_1ga52021a8057f729ebcba43cd9be418511.md)

[GXGDI_ResetRollString](./group__GDI_1gad5b571ac1752bcc7d2f07536405fb837.md)

[GXGDI_PauseRollString](./group__GDI_1ga9e13b593061a07342b075987c7c7bcfd.md)

[GXGDI_GetRollStringPos](./group__GDI_1gaf1ed258b41a7e257da0f56b717ba863d.md)

[GXGDI_GetRollTimes](./group__GDI_1ga38498f5ae31460afbfdbce9bafb73f3d.md)

# GUI部分说明

GUI部分，是在GDI基础上增加了对虚拟GUI与主GUI分开的数据管理策略。

## 为虚拟GUI专设的接口

[GUI_InitByName](./group__GUI_1ga0a83a7f4c2cd71c33f6fc88c3ad96779.md)

[GUI_QuitByName](./group__GUI_1gab5d38bfbe390b5b63c98100e1ecb53f3.md)

[GUI_CurrentLink](./group__GUI_1gafa009dba31c395b48c761453931336b3.md)

**其他在主GUI中的接口都能在虚拟GUI中调用。**

## XML中与主GUI的异同

在配置GUI宽高的XML（theme.xml或widget.xml）中，增加了对于GUI位置x、y参数的配置。在theme.xml中，增加对按键、消息模式的配置：key_mode与message_mode，有三种配置：refuse(不接受任何此类消息)、block(阻塞接受此类消息)和noblock(非阻塞接受此消息)。其中，按键/消息接收模式，可以使用GUI_SetInterface接口来动态改变。

## 全局与局部数据

主GUI与虚拟GUI中，属于主GUI的数据可作为全局数据，属于虚拟GUI的数据只能作为该虚拟GUI的局部数据。所有XML配置中，在虚拟GUI中没有的，都会从主GUI的全局配置中得到，即主GUI为默认数据。包括没有的组件、图片和字体。但有许多数据，必须是全局的。

必须是全局的数据（资源）有：

- 1) SPP层，所有的SPP层背景图片，都是全屏贴的，所以不存在局部，都是全局的；

- 2) 两个透明色，即GUI透明色与OSD透明色；

- 3) 全局透明度；

- 4) 芯片的chip ID；

- 5) FB的管理模块；

- 6) 虚拟按键映射（keymap.xml）；

- 7) 若需要在虚拟GUI中设置语言，则虚拟GUI必须有多国语言的XML，否则默认为主GUI当前语言翻译。

# 使用利弊

## 利

- 1) 很好地解决了阻塞框、成型应用开发等问题；

- 2) 解决了需要独占或虚拟独占GUI/GDI资源的问题；

- 3) 统一了浮窗效果；

- 4) 有利于整个方案的资源配置优化；

- 5) 对于第三方库的移植更加清晰化。

## 弊

1. 存在界面加载速度慢的问题；

2. 在进入“高架桥”到退出“高架桥”模式，占用的内存量成倍增加；

3. 由于GUI的XML三方解析库的问题，解析注释会出现内存泄漏，故经常进入和退出的虚拟GUI的XML描述一旦有注释，必将导致内存泄漏。


