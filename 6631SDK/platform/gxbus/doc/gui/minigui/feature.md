# API集合

[minigui_init_byxml](./group__GDI_1ga0951da0b6308ca853696daa6c91f89e2.md)

[minigui_create_dialog](./group__GDI_1ga354c4e9cda765eb54a6b5996630a4b5a.md)

[minigui_end_dialog](./group__GDI_1ga3ce250b158a6b2cf11a727de8d26af17.md)

[minigui_widget_set](./group__GDI_1gab4096b0b144cbd9397e9073247aa64c1.md)

[minigui_widget_get](./group__GDI_1ga4105fd1e499febe1d6f57a797c5461ce.md)

[minigui_widget_add_data](./group__GDI_1ga5bee0d5deffe665e16ac05783602e070.md)

[minigui_widget_clear_data](./group__GDI_1ga038bcb172ff6d910c280a0ebdd475e09.md)

[minigui_widget_set_focus](./group__GDI_1gab4096b0b144cbd9397e9073247aa64c1.md)

[minigui_widget_get_focus](./group__GDI_1ga4105fd1e499febe1d6f57a797c5461ce.md)

[minigui_flush](./group__GDI_1ga350ebe6f5c28945bac1c3e7a39ea54a1.md)

[minigui_connect_signal](./group__GDI_1gadbd1d649e46fa55d1d3b10ee38c55c75.md)

# 组件介绍

## frame组件

	为了与普通GUI中的window相区别，在miniGUI中，总的容器组件命名为frame（框架）。可添加miniGUI的其他若干个组件。示例XML如下：

```XML
<widget class="frame" name="frm_upgrade">
    <property>
        <rect>[0,0,1280,720]</rect>
        <forecolor>[#00FF00,#00FF00,#00FF00]</forecolor>
        <backcolor>[#909092,#909092,#909092]</backcolor>
    </property>

    <children>
    </childre>
</widget>

```

在miniGUI中，除了示例中的属性，无其他属性，即无背景图，只有create与destroy信号回调。

## label组件

	为与普通GUI区分，将普通GUI中的text与image组件功能合并为label组件，即可贴图、可写字符串的标签组件。组件XML示例如下：

```XML

<widget class="label" name="upgrade_back_label">
        <property>
        <rect>[200,100,300,40]</rect>
        <backcolor>[#FFFF00,#FFFF00,#FFFF00]</backcolor>
    </property>
</widget>

<widget class="label" name="upgrade_title_label">
    <property>
        <rect>[200,100,300,40]</rect>
        <backcolor>[#FFFF00,#FFFF00,#FFFF00]</backcolor>
        <forecolor>[#FFFFFF,#FFFFFF,#FFFFFF]</forecolor>
        <font>font_main</font>
        <string>Title</string>
    </property>
</widget>

<widget class="label" name="upgrade_icon_label">
    <property>
        <rect>[200,200,20,20]</rect>
        <backcolor>[#FFFF00,#FFFF00,#FFFF00]</backcolor>
        <forecolor>[#FFFFFF,#FFFFFF,#FFFFFF]</forecolor>
        <img>s_pvr_dot_grey</img>
    </property>
</widget>

```

## progress组件

	为与普通GUI区分，进度条组件命名为progress，示例XML如下：

```XML

<widget class="progress" name="upgrade_progress">
    <property>
        <rect>[200,500,1000,40]</rect>
        <backcolor>[#808080,#00FFFF,#00FFFF]</backcolor>
        <forecolor>[#FFFFFF,#FFFFFF,#FFFFFF]</forecolor>
        <value>12</value>
        <string>false</string>
    </property>
</widget>

```

根据示例中，backcolor与forecolor两个属性的解释与其他组件不同。其中将进度条进度扫过部分称为注满部分，为扫过部分成为未注满部分。


backcolor [0]: 底部未注满部分颜色值

backcolor [1]: 上部注满部分颜色值

backcolor [2]: Reserve for further use


forecolor [0]: 上部文字（如果有, string属性与label不同，只能设true与false，表示是否显示进度百分比文字）

forecolor [1]: Reserve for further use

forecolor [2]: Reserve for further use


## list组件

为与普通GUI中的listview相区分，列表组件命名为list，示例XML如下：

```XML

<widget class="list" name="upgrade_list">
    <property>
        <rect>[300,100,500,400]</rect>
        <backcolor>[#808080,#00FFFF,#00FFFF]</backcolor>
        <forecolor>[#FFFFFF,#FFFFFF,#FF0000]</forecolor>
        <itemcolor>[#F00F00,#00F700,#000000]</itemcolor>
        <itemfocus></itemfocus>    <!--聚焦项图片-->
        <itemunfocus></itemunfocus>   <!--非聚焦项图片-->
        <column>[0,50,150,300]</column>
        <rows>10</rows>
        <space>2</space>
        <grid>1<grid>
        <scroll>10<scroll>
    </property>
</widget>

```

针对本例颜色，backcolor、forecolor和itemcolor三种，解释与前几种组件以及普通GUI不同。

backcolor[0]:未聚焦时隔线颜色

backcolor[1]:聚焦时隔线颜色

backcolor[2]:scroll底色（如果有，即有scroll属性，且数值不为零）


forecolor[0]:未聚焦时item条的颜色

forecolor[1]:聚焦时item条的颜色

forecolor[2]:scroll条的颜色（同上，如果有）

itemcolor[0]:未聚焦时文字颜色

itemcolor[1]:聚焦时文字颜色

itemcolor[2]:Reserve for further use


对于有聚焦项图片（itemfocus）与非聚焦项图片（itemunfocus）属性的，对应的颜色值默认为无效，即forecolor[0]、forecolor[1]。

space:垂直方向隔线象素数

grid:水平方向隔线象素数

colomn:主要传递了两个信息：1. 有几列； 2. 每列起始的相对坐标位置。若水平方向有隔线，则隔线象素算在前一个列的宽度中。若此属性不加，则认为是单列。若按此例，分四列，第一列从0到50，水平隔线1象素，即第一列宽度为49。另外，最后一列还有减去scroll的宽度（如果有scroll的话）。以此类推，接下去几列的宽度为99、149、89。

rows: 最多划分几行，若按此例，划分10行，list总高度400，垂直方向隔线为2，则每行的高度为38。


# 使用范例

## 设置/获取属性

```CPP

minigui_widget_set("upgrade_tips_label", "string", "Load...");
minigui_widget_set("upgrade_tips_bottom", "backcolor", "[#0000FF,#0000FF,#0000FF]");
minigui_widget_set("upgrade_tips_middle", "img", "MP_DICON_BLUE");

char *wdg_state = minigui_widget_get("upgrade_tips_label", "state");
if((wdg_state) && (0 == strcasecmp("hide", wdg_state))) {
	// ...
} else {
	// ...
}

//...

char prog_value[10] = {0};
sprintf(prog_value, "%d\%", value);
minigui_widget_set("upgrade_prog", "value". prog_value);
minigui_flush(); //需要刷新即调flush

//...

char *value = NULL;
int upgrade_value = 0;

value = minigui_widget_get("upgrede_prog", "value");
if(value) {
	upgrade_value = atoi(value);
}

```

## 焦点管理

与普通GUI会自动聚焦不同，轻量级mini GUI现在只有一个组件，即list可以聚焦，但需要调用接口主动聚焦。

```CPP

minigui_widget_set_focus("freq_set_list", TRUE);

```

这样就完成了对list的聚焦，调用其他组件无效。

```CPP

char *focus = minigui_widget_get_focus();
if((focus) && (0 == strcasecmp(focus, "freq_set_list"))) {
	//...
}

```

## list使用

list是参考普通GUI的listview做的，与普通GUI的listview有以下几点不同：

- 1) mini GUI作为轻量级GUI，不再向list引入容器组件概念，所以list不在XML编写时体现子组件概念；

- 2) list分列在XML中完成配置；

- 3) 不再通过get_total与get_data获取总数与数据，而是通过直接调用minigui_widget_add_data加入数据。

list其他属性的设置与使用与其他组件相同，有三点不同的，一个是加数据，一个是获取行内容，最后一个是清空数据：

### 加入数据

加入的数据，以字符串形式提供。考虑到list可能会有多列，每列的数据以”|“分隔。另外，可能既有图片，又有字符串。所以，约定图片加一对"<"、">"来表示。若字符串所提供的列数小于list指定列时，显示时多余列为空，不显示。若字符串所提供列数大于list指定列时，显示时抛弃多余的列，不作显示。

```CPP

if(GXCORE_ERROR == minigui_widget_add_data("upgrade_list", "1 | 40800/2800 | <MP_FRE_ICON>")) {
	ASSERT("Add data failed.\n");
}

if(GXCORE_ERROR == minigui_widget_add_data("upgrade_list", "2 | 50900/2800 | <MP_FRE_ICON> | <MP_GREEN_ICON> | <MP_YELLOW_ICON>")) {
	ASSERT("Add data failed.\n");
}

if(GXCORE_ERROR == minigui_widget_add_data("upgrade_list", "3 | 60100/2800")) {
	ASSERT("Add data failed.\n");
}

```

### 获取行内容

几乎每一个组件获取的属性与XML中的属性名都是一样的，但是list有一个属性，select_content，是获取当前选中项的内容的。比如：

```CPP

char *select = NULL, *select_content = NULL;

select = minigui_widget_get("upgrade_list", "select");

select_content = minigui_widget_get("upgrade_list", "select_content");

/*作行比较*/
if((select) && (0 == strcasecmp("0", select))) {
	//...
}

/*作行内容比较*/
if((select_content) && (0 == strcasecmp("3 | 60100/2800", select_content))) {
	//...
}


```

### 清空数据

当实际数据内容已经改变时，则调用minigui_widget_clear_data接口，清空所有数据，而后再做添加。

```CPP

minigui_widget_clear_data("upgrade_list");

```


## signal应用

mini GUI不提供kepress与service的signal，只针对每一个frame提供create与destroy的signal，即只在进入与退出对话框时回调。

用法与普通GUI相同：

### 连接信号

```CPP

MINIGUI_SIGNAL_CONNECT(upgrade_create);
MINIGUI_SIGNAL_CONNECT(upgrade_destroy);

```

### XML中加入信号

```XML

<widget class="frame" name="frm_upgrade">
    <property>
        <rect>[0,0,1280,720]</rect>
        <forecolor>[#00FF00,#00FF00,#00FF00]</forecolor>
        <backcolor>[#909092,#909092,#909092]</backcolor>
    </property>

    <signal>
        <create>upgrade_create</create>
        <destroy>upgrade_destroy</destroy>
    </signal>
</widget>


```

### 信号函数中的传入参数为frame名称

```CPP

int upgrade_create(void *data)
{
	if((data) && (0 == strcasecmp("frm_upgrade", (const char *)data))) {
		//...
	}
}

int upgrade_destroy(void *data)
{
	if((data) && (0 == strcasecmp("frm_upgrade", (const char *)data))) {
		//...
	}
}

```


