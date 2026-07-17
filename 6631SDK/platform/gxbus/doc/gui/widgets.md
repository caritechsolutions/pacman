# 概要

GxGUI的组件，在使用中主要有：属性和信号函数两部分组成。属性又可分为：固化属性（XML一次性配置，不可再修改）、可设/读属性和动作属性（所谓动作属性是并非真正属性，而是为了完成某个动作而作为属性对应用提供的）。其中，可设/读属性既可以在XML中设置，也可以在C语言中动态设置动作属性只能在C语言中设置，不能在XML中设置。信号函数，针对每个组件的特性和应用场景的不同，各有不同。接下来详细介绍个组件的属性和信号函数。

# button

## 属性

- 固化属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
|    name       | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，用以表示组件名 |
|    style      | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，名字与style.xml中某一组件风格名相同 |
|    rect       | 组件坐标，顺序为[左，上，宽，高]  |
|    link       | 对应其他窗口名称，当按键响应到clicked事件，即按下时弹出link的对话框 |

- 可设/读属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
|    state      | 可设置disable/enable/hide/show来调整：失效/有效/隐藏/显示属性 |
| forecolor     |表示组件的前景色，一般情况下是组件的文字颜色。由三个颜色值组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名，当对应的图片生效后此属性对应的颜色即失效 。 |
| backcolor      |表示组件的背景色，一般情况下是组件的底色。由三个颜色值组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名, 当对应的图片生效后此属性对应的颜色即失效。 |
| font           | 在相应的font.xml中描述的字体名称，可支持点阵字库，也能支持矢量字库 |
| alignment      | 横向：left、right、hcentre 纵向：top、bottom、vcentre。水平横向和垂直纵向可同时选择一个，同向不可同时选择，通过“&#124;”来操作 |
| string         | utf8或gb2312标准字符串  |
| unfocus_img    | 对应image.xml中的图片ID名称，此属性既可以写成单个图片属性，也可写成左中右三个图片拼接的属性([tt_left, tt_middle, tt_right]) |
| focus_img      | 对应image.xml中的图片ID名称，此属性既可以写成单个图片属性，也可写成左中右三个图片拼接的属性([tt_left, tt_middle, tt_right]) |
| disable_img    | 对应image.xml中的图片ID名称，此属性既可以写成单个图片属性，也可写成左中右三个图片拼接的属性([tt_left, tt_middle, tt_right])  |
| launch         | 按下按键向目标组件发送虚拟按键值，此属性为目标组件名          |
| virtual_key    | 按下按键向目标组件发送虚拟按键值，词属性为虚拟按键名，如：GUIK_A、GUIK_B等    |

- 动作属性

无

## 信号函数

|   信号      |             说明                    |
|-------------|-------------------------------------|
| create    |  当此函数被注册，则在该组件被创建时调用相应的注册函数 |
| destroy   |  当此函数被注册，则在该组件被销毁时调用相应的注册函数 |
| clicked   |  当此函数被注册，则在按下OK按键时调用相应的注册函数  |
| keypress  |  当此函数被注册，则在当前聚焦组件为此组件时调用相应的注册函数 |


## Tips:

- 当既注册了keypress，也注册了clicked，属性中的link也设置了，效果是：

```flow
st=>start: 开始
op0=>operation: 按键接收
cond0=>condition: 是否有kepress信号
op1=>operation: 处理keypress信号
cond1=>condition: 是否继续传递
cond2=>condition: 是否有clicked信号
op2=>operation: 处理clicked信号
cond3=>condition: 是否继续传递
cond4=>condition: 是否有link属性
op3=>operation: 打开link对话框
e=>end
st->op0->cond0->op1->cond1->cond2->op2->cond3->cond4->op3->e
cond0(yes)->op1
cond0(no)->e
cond1(yes)->cond2
cond1(no)->e
cond2(yes)->op2
cond2(no)->e
cond3(yes)->cond4
cond3(no)->e
cond4(yes)->op3
cond4(no)->e

&```

# text

## 属性

- 固化属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| name          | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，用以表示组件名 |
| style    | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，名字与style.xml中某一组件风格名相同 |
| rect          | 组件坐标，顺序为[左，上，宽，高] |

- 可设/读属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| state         | 可设置disable/enable/hide/show来调整：失效/有效/隐藏/显示属性 |
| forecolor     | 表示组件的前景色，一般情况下是组件的文字颜色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 ** 在此组件中只有第0个颜色有效，其他两个颜色无效** 。  |
| backcolor     | 表示组件的背景色，一般情况下是组件的底色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 ** 在此组件中只有第0个颜色有效，其他两个颜色无效 **。  |
| string        | utf8或gb2312标准字符串  |
| font          | 在相应的font.xml中描述的字体名称，可支持点阵字库，也能支持矢量字库 |
| alignment     | 横向：left、right、hcentre 纵向：top、bottom、vcentre。水平横向和垂直纵向可同时选择一个，同向不可同时选择，通过“&#124;”来操作 |
| format        | 可设置属性为：static（单行，不换行）、roll_r2l（自右至左滚动）、fix_r2l（自右向左滚动，过长时才会滚动）、roll_l2r（自左至右滚动）、automatic（自动换行） 当需要实现滚动效果（暂时支持自左至右滚动）时，设置滚动时间间隔，该值直接决定滚动速度，此值为一个整型。  |
| time          | 在format为roll_r2l、roll_l2r、fix_r2l的情况下，此属性定义为滚动时间间隔，时间间隔越小滚动速度越快 |
| auto_width    | 若不清楚字符串宽度，可设置此属性，但对齐方式（alignment）即失效  |
| auto_height   | 若不清楚文字的高度，可设置此属性，但对齐方式（alignment）即失效  |
| inter_char    | 横向字间距（单位像素），此属性在非中日韩文字不推荐使用  |
| inter_line    | 纵向字符行间距（单位像素），一般不推荐使用   |
| step          | 滚动移位像素，越大滚动速度越快，但效果越差  |

- 动作属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| reset_rolling | 重启滚动时间, 仅对滚动效果有效      |
| rolling_stop  | 停止滚动                            |
| continue_rolling | 继续rolling_stop的滚动           |
| rolling_stop_soon |   与rolling_stop区别在于立即停止滚动，可能会造成残留等问题 |
| rolling_position  | 滚动位置 |

## 信号函数

|   信号      |             说明                    |
|-------------|-------------------------------------|
| create      | 当此函数被注册，则在该组件被创建时调用相应的注册函数 |
| destroy     | 当此函数被注册，则在该组件被销毁时调用相应的注册函数 |

## Tips:

- 由于电视机的效果问题，若需要滚动，一般time会被GUI系统**强制**设置为20的整数倍，以确保最佳滚动效果。故存在一些对time的调整对滚动效果影响不大。

# image

## 属性

- 固化属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| name          | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，表示组件的名称 |
| style         | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，名称与style.xml中某个 组件风格名相同 |
| rect          | 组件坐标，顺序为[左，上，宽，高] |

- 可设/读属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| state         | 可设置disable/enable/hide/show来调整：失效/有效/隐藏/显示属性 |
| forecolor     | 表示组件的前景色，一般情况下是组件的文字颜色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 |
| backcolor     | 表示组件的背景色，一般情况下是组件的底色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 |
| mode          | 用single和multiple来设置是单图模式还是多图模式，多图模式为四个角，四个边拼接而成， 其中single模式以下的img属性有效，multiple模式以下的lt_img、rt_img和lb_img等八张图有效 |
| img           | 对应image.xml中的图片ID名称 |
| lt_img        | 对应image.xml中的图片ID名称 |
| rt_img        | 对应image.xml中的图片ID名称 |
| lb_img        | 对应image.xml中的图片ID名称 |
| rb_img        | 对应image.xml中的图片ID名称 |
| l_img         | 对应image.xml中的图片ID名称 |
| r_img         | 对应image.xml中的图片ID名称 |
| t_img         | 对应image.xml中的图片ID名称 |
| b_img         | 对应image.xml中的图片ID名称 |

- 动作属性 (仅限set属性)

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| surface       | 获取来自GDI的surface，显示在image组件中（GX3201开始有所放功能）. |
| capture       | 从视频上通过抓屏获得图片来显示 |
| save_image    | 将当前正在组件中显示的图片保存为bmp格式图片 |

## 信号函数

|   信号      |             说明                    |
|-------------|-------------------------------------|
| create      | 当此函数被注册，则在该组件被创建时调用相应的注册函数 |
| destroy     | 当此函数被注册，则在该组件被销毁时调用相应的注册函数 |

## Tips:

- 一般应用场景，图片大多来自image.xml中的描述。如果需要利用image组件显示来自网络、U盘等任何三方的图片时，在保存完后，建议调用：

````cpp
gal_add_key_path("test_net_file", "/tmp/test_net0.png");
GUI_SetProperty("ad_image", "img", "test_net_file");

````

- 注意：图片的ID必须在整个方案中唯一，不能重复，调用完GUI_SetProperty后不能立即删除图片（因为尚未完成解码）。

# edit

## 属性

- 固化属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| name          | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，表示组件的名称 |
| style         | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，名称与style.xml中某组件名相同 |
| rect          | 组件坐标，顺序为[左，上，宽，高] |

- 可设/读属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| state         | 可设置disable/enable/hide/show来调整：失效/有效/隐藏/显示属性 |
| forecolor     | 表示组件的前景色，一般情况下是组件的文字颜色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 **三个颜色分别表示:非聚焦、聚焦和失效三种状态的颜色。** |
| backcolor     | 表示组件的背景色，一般情况下是组件的底色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 **三个眼色分别表示：非聚焦、聚焦和失效三种状态的颜色。** |
| font          | 在相应的font.xml中描述的字体名称，可支持点阵字库，也能支持矢量字库 |
| alignment     | 横向：left、right、hcentre 纵向：top、bottom、vcentre。水平横向和垂直纵向可同时选择一个，同向不可同时选择，通过“&#124;”来操作 |
| string        | utf8或gb2312标准字符串  |
| intaglio      | 一旦此属性设置了相应的值，当前的edit就变成了密码模式 |
| default_intaglio | 当intaglio为" "时，显示输入字符，否则密码显示为intaglio所定义的字符 |
| maxlen        | 当密码未输入时， 显示该字符，长度由maxlen定义 |
| format        | 用户可根据需要设置edit_string（字符串模式）、edit_digit（数字模式）、edit_ip（IP地址模式，如192.168.120.171）、edit_time（时间模式,如：12：00：00或者12：00）、edit_date（日期模式，如：2010/03/17）、edit_float（小数模式，如：123.45）。 **其中， 当格式为edit_ip、edit_time、edit_date和edit_float时，string的初始值形式就是该种格式的形式，如：edit_float形式，123.45就是指3位整数，2位小数** 。 |
| unfocus_img    | 对应image.xml中的图片ID名称，此属性既可以写成单个图片属性，也可写成左中右三个图片拼接的属性([tt_left, tt_middle, tt_right]) |
| focus_img      | 对应image.xml中的图片ID名称，此属性既可以写成单个图片属性，也可写成左中右三个图片拼接的属性([tt_left, tt_middle, tt_right]) |
| disable_img    | 对应image.xml中的图片ID名称，此属性既可以写成单个图片属性，也可写成左中右三个图片拼接的属性([tt_left, tt_middle, tt_right])  |

- 动作属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| clear(仅限set)| 清除edit组件全部内容，且光标恢复到0位置  |
| select        | 选中光标位置，不得大于maxlen的值 |

## 信号函数

|   信号      |             说明                    |
|-------------|-------------------------------------|
| create      | 当此函数被注册，则在该组件被创建时调用相应的注册函数 |
| destroy     | 当此函数被注册，则在该组件被销毁时调用相应的注册函数 |
| keypress    | 当此函数被注册，则在当前聚焦组件为此组件情况下，按键到来时，调用相应的注册函数 |
| clicked     | 当此函数被注册，鼠标或点击触摸屏时调用相应的注册函数 |
| change      | 当组件里的值被改变时，发送此信号 |
| reach_end   | 当此函数被注册，则当光标到达最后一个字节后调用此注册函数 |

## Tips:

- edit组件的各种模式下，有对应的maxlen，若格式、maxlen以及模式之间有冲突，或有出错提示，或不显示。

- 使用edit的string模式，改造成各种其他特殊需求，比如系统应用想采用“2014-06-30”格式或英式格式“30/06/2014”，建议应用采用字符串模式自行处理。例如：

````xml

<widget class="edit" style="default" name="edit_british_mode_date">
        <property>
                <rect>[10,54,120,40]</rect>
                <forecolor>[#333333,#333333,#333333]</forecolor>
                <backcolor>[#cc9219,#cc9219,#cc9219]</backcolor>
                <alignment>hcentre|vcentre</alignment>
                <maxlen>10</maxlen>
                <format>edit_string</format>
        </property>
        <signal>
             <keypress>app_british_mode_date_keypress</keypress>
        </signal>
</widget>

````

````cpp
SIGNAL_HANDLER int app_british_mode_date_keypress(void *data, void *event)
{
        GuiWidget *self = NULL;
        GUI_Event *event_data = NULL;
        int select = 0;

        if((NULL == data) || (NULL == event))
                return EVENT_TRANSFER_STOP;

        self = (GuiWidget *)data;
        event_data = (GUI_Event *)event;

        if(GUI_KEYDOWN == event_data->type)
        {
                /*处理对特殊位置按键的屏蔽*/
                switch(event_data->key.sym)
                {
                        case GUIK_0:
                        case GUIK_1:
                        case GUIK_2:
                        case GUIK_3:
                        case GUIK_4:
                        case GUIK_5:
                        case GUIK_6:
                        case GUIK_7:
                        case GUIK_8:
                        case GUIK_9:
                               GUI_GetProperty("edit_british_mode_date", "select", &select);
                               if((1 == select) || (4 == select))
                               {
                                       select += 2;
                                       GUI_SetProperty("edit_british_mode_date", "select", &select);
                               }
                               return EVENT_TRANSFER_KEEPON;
                        case GUIK_LEFT:
                               GUI_GetProperty("edit_british_mode_date", "select", &select);
                               if((3 == select) || (6 == select))
                               {
                                       select -= 2;
                                       GUI_SetProperty("edit_british_mode_date", "select", &select);
                                       return EVENT_TRANSFER_STOP;
                               }
                              else
                                      return EVENT_TRANSFER_KEEPON;
                        case GUIK_RIGHT:
                               GUI_GetProperty("edit_british_mode_date", "select", &select);
                               if((1 == select) || (4 == select))
                               {
                                       select += 2;
                                       GUI_SetProperty("edit_british_mode_date", "select", &select);
                                       return EVENT_TRANSFER_STOP;
                               }
                               else
                                       return EVENT_TRANSFER_KEEPON;
                        default:
                               return EVENT_TRANSFER_STOP;
                }
        }
        else
            return EVENT_TRANSFER_KEEPON;
}
````

# combobox

## 属性

- 固化属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| name          | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成,表示组件名 |
| style         | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成,与style.xml中的某组件风格名相同 |
| rect          | 组件坐标，顺序为[左，上，宽，高] |

- 可设/读属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| state         | 可设置disable/enable/hide/show来调整：失效/有效/隐藏/显示属性 |
| forecolor     | 表示组件的前景色，一般情况下是组件的文字颜色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 **三个颜色分别表示:非聚焦、聚焦和失效三种状态的颜色** 。 |
| backcolor     | 表示组件的背景色，一般情况下是组件的底色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 **三个颜色分别表示:非聚焦、聚焦和失效三种状态的颜色**  。 |
| font          | 在相应的font.xml中描述的字体名称，可支持点阵字库，将来也能支持矢量字库 |
| select        | 大于等于0的任意数字 |
| unfocus_img    | 对应image.xml中的图片ID名称，此属性既可以写成单个图片属性，也可写成左中右三个图片拼接的属性([tt_left, tt_middle, tt_right]) |
| focus_img      | 对应image.xml中的图片ID名称，此属性既可以写成单个图片属性，也可写成左中右三个图片拼接的属性([tt_left, tt_middle, tt_right]) |
| disable_img    | 对应image.xml中的图片ID名称，此属性既可以写成单个图片属性，也可写成左中右三个图片拼接的属性([tt_left, tt_middle, tt_right])  |
| unfocus_left_img | 对应image.xml中的图片ID名称，非聚焦左箭头 |
| focus_left_img   | 对应image.xml中的图片ID名称，聚焦左箭头 |
| disable_left_img | 对应image.xml中的图片ID名称，失效左箭头 |
| unfocus_right_img| 对应image.xml中的图片ID名称，非聚焦右箭头 |
| focus_right_img  | 对应image.xml中的图片ID名称，聚焦右箭头 |
| disable_right_img| 对应image.xml中的图片ID名称，失效右箭头 |
| content          | 用“[]”括起来，用“,”隔开每一个选项 |

- 动作属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| select_content(仅限get) | 获取出当前选中项的字符串  |

## 信号函数

|   信号      |             说明                    |
|-------------|-------------------------------------|
| create      | 当此函数被注册，则在该组件被创建时调用相应的注册函数 |
| destroy     | 当此函数被注册，则在该组件被销毁时调用相应的注册函数 |
| keypress    | 当此函数被注册，则在当前聚焦组件为此组件情况下，按键到来时，调用相应的注册函数 |
| clicked     | 当此函数被注册，鼠标或点击触摸屏时调用相应的注册函数 |
| change      | 当此函数被注册，在组件选中项被改变时，调用相应的注册函数 |

## Tips:

无

# progbar

## 属性

- 固化属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| name          | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，表示组件的名称 |
| style         | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，与style.xml中的某个组件风格名相同 |
| rect          | 组件坐标，顺序为[左，上，宽，高] |
| alignment     | 横向：left、right、hcentre 纵向：top、bottom、vcentre。水平横向和垂直纵向可同时选择一个，同向不可同时选择，通过“&#124;”来操作 |

- 可设/读属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| state         | 可设置disable/enable/hide/show来调整：失效/有效/隐藏/显示属性 |
| forecolor     | 表示组件的前景色，一般情况下是组件的文字颜色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 该组件只有0号颜色有效,表示组件在显示文字时(percentage或者decimal)的文字颜色 |
| backcolor     | 表示组件的背景色，一般情况下是组件的底色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 该组件只有0号和1号颜色有效，表示进度条的背景颜色和条的颜色 。 |
| min           | 进度条所显示的最小值 |
| max           | 进度条所显示的最大值 |
| value         | 进度条显示的值   |
| back_image    | 对应image.xml中的图片ID名称 |
| back_image_l  | 对应image.xml中的图片ID名称 |
| back_image_m  | 对应image.xml中的图片ID名称 |
| back_image_r  | 对应image.xml中的图片ID名称 |
| fore_image    | 对应image.xml中的图片ID名称 |
| fore_image_l  | 对应image.xml中的图片ID名称 |
| fore_image_m  | 对应image.xml中的图片ID名称 |
| fore_image_r  | 对应image.xml中的图片ID名称 |
| format        | 分为normal、water和vertical三种模式，其中normal为正常横向进度条，water为水柱型横向进度条，vertical为竖自下而上直进度条 |
| text_format   | 分为percentage、decimal和hide三种选项，其中percentage显示为百分比形式，decimal显示为十进制形式，hide不现实数字 |
| font          | 在相应的font.xml中描述的字体名称，可支持点阵字库，也能支持矢量字库 |

- 动作属性

无

## 信号函数

|   信号      |             说明                    |
|-------------|-------------------------------------|
| create      | 当此函数被注册，则在该组件被创建时调用相应的注册函数 |
| destroy     | 当此函数被注册，则在该组件被销毁时调用相应的注册函数 |
| reach_end   | 当此函数被注册，则当到达max时调用此注册函数 |

## Tips:

- back_image和后面的fore_image只在格式为normal和vertical的时候生效；在water模式下组件如需贴图必须由back_image_l、back_image_m和back_image_r三张图来组成背景图，以及fore_image_l、fore_image_m和fore_image_r三张图来组成前景图。

# listview

## 属性

### listview属性

- 固化属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| name          | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，表示组件名称 |
| style         | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，与style.xml中某一组件的风格名相同 |
| rect          | 组件坐标，顺序为[左，上，宽，高] |

- 可设/读属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| state         | 可设置disable/enable/hide/show来调整：失效/有效/隐藏/显示属性 |
| forecolor     | 表示组件的前景色，一般情况下是组件的文字颜色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 |
| backcolor     | 表示组件的背景色，一般情况下是组件的底色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。 或者可以在color.xml中添加颜色，并选择对应颜色名。 |
| format        | 当格式设为enable_roll时表示开启过长字符串滚动功能。, page表示选项为翻页形式 |
| interval      | 每一个ITEM之间的间隔 |
| select        | LISTVIEW当前选中项 |
| unfocus_img   | 对应image.xml中的图片ID名称，为对应每个LISTIETM的图片，也可写成左中右三个图片拼接的属性([tt_left, tt_middle, tt_right]) |
| focus_img     | 对应image.xml中的图片ID名称，为对应每个LISTIETM的图片，也可写成左中右三个图片拼接的属性([tt_left, tt_middle, tt_right]) |
| disable_img   | 对应image.xml中的图片ID名称，为对应每个LISTIETM的图片，也可写成左中右三个图片拼接的属性([tt_left, tt_middle, tt_right]) |
| item_active_image | 当前项被激活的背景图片，也可写成左中右三个图片拼接的属性([tt_left, tt_middle, tt_right]) |
| item_active_color | 当前项被激活的字体颜色 |
| item_rect     | 每一个ITEM的坐标和宽、高，其中坐标X、Y值无效，根据每个ITEM以及间隔（interval）决定 |
| item_back_color | 每个item的背景色，表示组件的背景色，一般情况下是组件的底色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。 |
| item_fore_color | 每个item的前景色，表示组件的前景色，一般情况下是组件的文字颜色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。 |
| roll_time       | 当开启过长字符串滚动功能时设置字符串滚动时间 |
| time_out        | 当此属性设置为非零时，可设置listview聚焦超时，即：当在某一选项聚焦长时间后自动调用timeout信号。此值为一整型。 |
| i18n            | 设置是否打开多国语言，true为打开，false为关闭 |

- 动作属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| update_all    | 通知组件更新所有单元项 |
| update_row    | 设置更新listview中的对应行 |
| active        | 设置某行为激活行（非选中状态） |
| total_number  | 设置item总数，若设置了此属性为非零值，则GUI系统不在会调用get_total信号函数来获取总数，总数被固定为此非零值 |

### header属性

- 固化属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| name          | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成 |
| style         | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成 |
| rect          | 组件坐标，顺序为[左，上，宽，高] |
| forecolor     | 表示组件的前景色，一般情况下是组件的文字颜色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 |
| backcolor     | 表示组件的背景色，一般情况下是组件的底色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 |
| format        | headershow/headerhide，其中headershow表示显示header组件，headerhide表示隐藏header组件 |
| colum_number  | 总列数 |
| grid_width    | 小于header宽度的任意数字 |


### listitem属性

- 固化属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| name          | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成 |
| style         | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成 |
| font          | 在相应的font.xml中描述的字体名称，可支持点阵字库，也能支持矢量字库，如果不写明此属性则服从listview的属性。 |
| alignment     | 横向：left、right、hcentre 纵向：top、bottom、vcentre。水平横向和垂直纵向可同时选择一个，同向不可同时选择，通过“|”来操作，如果不写明此属性则服从listview的属性。 |
| string        | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成 |

### scrollbar属性

- 固化属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| name          | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成 |
| style         | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成 |
| rect          | 组件坐标，顺序为[左，上，宽，高] |
| forecolor     | 表示组件的前景色，一般情况下是组件的文字颜色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 |
| backcolor     | 表示组件的背景色，一般情况下是组件的底色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 |
| back_img      | 对应image.xml中的图片ID名称，表示Scrollbar的背景图片 |
| arrow_up      | 对应image.xml中的图片ID名称，表示Scrollbar的上箭头图片 |
| arrow_down    | 对应image.xml中的图片ID名称，表示Scrollbar的下箭头图片 |
| fore_img      | 对应image.xml中的图片ID名称，表示Scrollbar的滚动条图片 |

## 信号函数

|   信号      |             说明                    |
|-------------|-------------------------------------|
| create      | 当此函数被注册，则在该组件被创建时调用相应的注册函数 |
| destroy     | 当此函数被注册，则在该组件被销毁时调用相应的注册函数 |
| keypress    | 当此函数被注册，则在当前聚焦组件为此组件情况下，按键到来时，调用相应的注册函数 |
| clicked     | 当此函数被注册，鼠标或点击触摸屏时调用相应的注册函数 |
| get_total   | 当此函数被注册，当组件需要动态获取数据时，获取数据项的总数 |
| get_data    | 当此函数被注册，当组件需要动态获取数据时，通过调用此函数获取数据 |
| change      | 当此函数被注册，在组件选中项被改变时，调用相应的注册函数 |
| timeout     | 当此函数被注册，在组件中某项聚焦时间超过time_out属性约定时间后调用相应注册函数。|

## Tips:

- GUI不对lisview每行数据做记录；
- 对于一些特殊应用需求，需要每行有不同字体，不同颜色可以在getdata信号函数中自行选择填写：

````cpp

typedef struct _ListItemPara {
	int x_offset;
	int sel;
	char *string;
	char *image;
	char *fore_color;
	char *back_color;
	char *font;
	int zoom;
	struct _ListItemPara *next
} ListItemPara;

````

# box

## 属性

- 固化属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| name          | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成 |
| style         | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成 |
| rect          | 组件坐标，顺序为[左，上，宽，高] |

### box属性

- 可设/读属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| state         | 可设置disable/enable/hide/show来调整：失效/有效/隐藏/显示属性 |
| forecolor     | 表示组件的前景色，一般情况下是组件的文字颜色。由三个颜色值组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 |
| backcolor     | 表示组件的背景色，一般情况下是组件的底色。由三个颜色值组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 |
| left_arrow    | 对应image.xml中的图片ID名称，如果是vertical形式的，此箭头为上箭头，如果是horizontal形式的，此箭头为左箭头。 |
| right_arrow   | 对应image.xml中的图片ID名称，如果是vertical形式的，此箭头为下箭头，如果是horizontal形式的，此箭头为右箭头。 |
| interval      | 每一个boxitem之间的间隔 |
| format        | 若为vertical形式则每个boxitem按竖直方向排列，切换响应上/下键，若为horizontal形式每个boxitem按水平方向排列，切换相应左/右键 |

- 动作属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| update        | 通知组件更新所有单元项              |

### boxitem属性

- 可设/读属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| forecolor     | 表示组件的前景色，一般情况下是组件的文字颜色。由三个颜色值组成，每个颜色值为#RRGGBB形式。 |
| backcolor     | 表示组件的背景色，一般情况下是组件的底色。由三个颜色值组成，每个颜色值为#RRGGBB形式。 |
| unfocus_image | 对应image.xml中的图片ID名称，也可写成左中右三个图片拼接的属性([tt_left, tt_middle, tt_right]) |
| focus_image   | 对应image.xml中的图片ID名称，也可写成左中右三个图片拼接的属性([tt_left, tt_middle, tt_right]) |
| disable_image | 对应image.xml中的图片ID名称，也可写成左中右三个图片拼接的属性([tt_left, tt_middle, tt_right]) |

- 动作属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| active        |  聚焦对应的boxitem                  |
| unactive      |  失焦对应的boxtiem                  |
| enable        |  使能对应的boxitem                  |
| disable       |  失效对应的boxitem                  |

**由于boxitem也是一个容器组件，所以不使用state属性来控制enable与disalbe，因为这样控制会需要多一步对容器内各组件的操作。**

## 信号函数

|   信号      |             说明                    |
|-------------|-------------------------------------|
| create      | 当此函数被注册，则在该组件被创建时调用相应的注册函数 |
| destroy     | 当此函数被注册，则在该组件被销毁时调用相应的注册函数 |
| change      | 当此函数被注册，在组件选中项被改变时，调用相应的注册函数 |
| keypress    | 当此函数被注册，则在当前聚焦组件为此组件情况下，按键到来时，调用相应的注册函数 |

## Tips:

无

# notepad

## 属性

- 固化属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| name          | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，表示组件名称 |
| style         | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，与style.xml文件中某一组件风格名相同style |
| rect          | 组件坐标，顺序为[左，上，宽，高] |

- 可设/读属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| state         | 可设置disable/enable/hide/show来调整：失效/有效/隐藏/显示属性 |
| forecolor     | 表示组件的前景色，一般情况下是组件的文字颜色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 |
| backcolor     | 表示组件的背景色，一般情况下是组件的底色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 |
| alignment     | 横向：left、right、hcentre 纵向：top、bottom、vcentre。水平横向和垂直纵向可同时选择一个，同向不可同时选择，通过“|”来操作 |
| font          | 在相应的font.xml中描述的字体名称，可支持点阵字库，将来也能支持矢量字库 |
| file_path     | 对应文件的路径 |
| image         | 对应image.xml中的图片ID名称 |
| vertical_margin | 显示文本的纵向边白 |
| horizontal_margin | 显示文本的横向边白 |
| scrollbar_auto_hide | 此标记置为true时，右边的scrollbar将隐藏，默认为false |
| buffer_size   | 配置显示所用到的缓存，不配默认为4096Bytes，越大切换效果越好，应用设置此属性需综合慎重考虑 |

- 动作属性

|   属性名               |             说明                    |
|------------------------|-------------------------------------|
| line_up(仅限set)       | 向上翻动行数，参数为整形数          |
| line_down(仅限set)     | 向下翻动行数，参数为整形数          |
| page_up(仅限set)       | 向上翻动一页                        |
| page_down(仅限set)     | 向下翻动一页                        |
| file(仅限set)          | 改变文件路径                        |
| active(仅限set)        | 参数为行号，设置某一行为高亮，以突出重点，其高亮色为该组件的聚焦色 |
| cur_pos                | 设置到整个文本的位置 |
| current_line           | 设置到整个文本的行号 |

## 信号函数

|   信号      |             说明                    |
|-------------|-------------------------------------|
| create      | 当此函数被注册，则在该组件被创建时调用相应的注册函数 |
| destroy     | 当此函数被注册，则在该组件被销毁时调用相应的注册函数 |

## Tips:

无

# canvas

## 属性

- 固化属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| name          | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，表示组件名称 |
| style         | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，与style.xml文件中某一组件风格名相同style |
| rect          | 组件坐标，顺序为[左，上，宽，高] |

- 可设/读属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| state         | 可设置disable/enable/hide/show来调整：失效/有效/隐藏/显示属性 |
| backcolor     | 表示组件的背景色，一般情况下是组件的底色。由三个颜色值组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 |
| font          | 在相应的font.xml中描述的字体名称，可支持点阵字库，将来也能支持矢量字库 |

- 动作属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| pixel         | 对指定区域绘制点，颜色取自颜色映射表color.xml |
| rectangle     | 对指定区域绘制矩形，颜色取自颜色映射表color.xml |
| image         | 对指定区域绘制图片 |
| image_file    | 对指定区域绘制图片文件,参数为图片文件路径 |
| string        | 对指定区域绘制字符串 |

## 信号函数

|   信号      |             说明                    |
|-------------|-------------------------------------|
| create      | 当此函数被注册，则在该组件被创建时调用相应的注册函数 |
| destroy     | 当此函数被注册，则在该组件被销毁时调用相应的注册函数 |

## Tips:

- 所有利用canvas组件绘制的内容都不记录，故现在游戏、字幕等应用建议使用“高架桥”（Virtual Layer）来实现。

# sliderbar

## 属性

- 固化属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| name          | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，表示组件名称 |
| style         | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，与style.xml中某一组件的风格名相同 |
| rect          | 组件坐标，顺序为[左，上，宽，高] |

- 可设/读属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| state         | 可设置disable/enable/hide/show来调整：失效/有效/隐藏属性 |
| backcolor     | 背景[0]号色表示整体背景颜色，背景[1]号色表示中间滑动槽颜色。 |
| forecolor     | 前景[0]号色表示刻度颜色，前景[1]号色表示游标颜色。 |
| min           | 最小值 |
| max           | 最大值 |
| value         | 初始值 |
| step          | 步进量 |
| format格式：  | horizontal_scale表示带刻度的水平滑动条，horizontal_no_scale表示不带刻度的水平滑动条， water表示水柱型，normal表示贴图非水柱型。|
| back_image_l  | 水柱型滑动槽背景左图 |
| back_image_m  | 水柱型滑动槽背景中图 |
| back_image_r  | 水柱型滑动条背景右图 |
| back_image    | 非水柱型滑动条背景图片 |
| fore_image_l  | 水柱型滑动条前景左图 |
| fore_image_m  | 水柱型滑动条前景中图 |
| fore_image_r  | 水柱型滑动条前景右图 |
| fore_image    | 非水柱型滑动条前景图片 |
| cursor_image  | 游标图片 |

- 动作属性

无

## 信号函数

|   信号      |             说明                    |
|-------------|-------------------------------------|
| create      | 当此函数被注册，则在该组件被创建时调用相应的注册函数 |
| destroy     | 当此函数被注册，则在该组件被销毁时调用相应的注册函数 |
| keypress    | 当此函数被注册，则在当前聚焦组件为此组件时调用相应的注册函数 |

## Tips:

- 水柱型（water）必须设置back_image_l、back_image_m、back_image_r和fore_image_l、fore_image_m、fore_image_r六张图片，非水柱型必须设置back_image和fore_image两张图片。

# table

## 属性

- 固化属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| name          | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，表示组件名称 |
| style_name    | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，与style.xml中某一组件风格名相同 |
| rect          | 组件坐标，顺序为[左，上，宽，高] |

- 可设/读属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| state         | 可设置disable/enable/hide/show来调整：失效/有效/隐藏/显示属性 |
| backcolor     | 背景[0]号色表示整体背景颜色，背景[1]号色表示中间滑动槽颜色。 |
| forecolor     | 前景[0]号色表示刻度颜色，前景[1]号色表示游标颜色。 |
| row_num       | 表示容器排列有几行 |
| column_num    | 表示容器排列有几列 |
| interval      | 容器所容纳的每个button之间的上下和左右间隔。 |

- 动作属性

无

## 信号函数

|   信号      |             说明                    |
|-------------|-------------------------------------|
| create      | 当此函数被注册，则在该组件被创建时调用相应的注册函数 |
| destroy     | 当此函数被注册，则在该组件被销毁时调用相应的注册函数 |
| keypress    | 当此函数被注册，则在当前聚焦组件为此组件时调用相应的注册函数 |

## Tips:

- 该容器组件只能包含button组件。

# timelist

## 属性

- 固化属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| name          | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，表示组件的名称 |
| style         | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，名称与style.xml中某组件名相同 |
| rect          | 组件坐标，顺序为[左，上，宽，高] |

### timelist属性

- 可设/读属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| state         | 可设置disable/enable/hide/show来调整：失效/有效/隐藏/显示属性 |
| backcolor     | 表示组件的背景色，一般情况下是组件的底色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 |
| font          | 在相应的font.xml中描述的字体名称，可支持点阵字库，也能支持矢量字库 |
| alignment     | 横向：left、right、hcentre 纵向：top、bottom、vcentre。水平横向和垂直纵向可同时选择一个，同向不可同时选择，通过“&#124;”来操作 |
| grid_width    | 表示每一个单元格之间的间隔，隔线颜色即为底色 |
| format        | 有flop_move和glid_move可选，flop_move表示切换时跳动，glid_move表示平滑滚动 |

- 动作属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| start_time(get时用start属性名，因为start_time被另一内部属性使用了) | 基准开始时间 |
| update_all(set) | 更新全列表 |
| select_row    | 选择聚焦行 |
| total_col(get) | 总列数，其值等于timelist中header组件所包含的text组件个数 |
| select_column(get) | 当前选中的列号 |

### timeitem属性

- 固化属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| name          | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，表示组件的名称 |
| style         | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，名称与style.xml中某组件名相同 |
| rect          | 组件坐标，顺序为[左，上，宽，高] |
| forecolor     | 表示组件的前景色，一般情况下是组件的文字颜色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 |
| backcolor     | 表示组件的背景色，一般情况下是组件的底色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 |

## 信号函数

|   信号      |             说明                    |
|-------------|-------------------------------------|
| create      | 当此函数被注册，则在该组件被创建时调用相应的注册函数 |
| destroy     | 当此函数被注册，则在该组件被销毁时调用相应的注册函数 |
| keypress    | 当此函数被注册，则在当前聚焦组件为此组件情况下，按键到来时，调用相应的注册函数 |
| get_data    | 当此函数被注册，当组件需要动态获取数据时，通过调用此函数获取数据 |
| change      | 当此函数被注册，当组件需要改变当前起始时间时调用此函数 |

## Tips:

- 通过信号(signal)get_data获取的数据格式为字符串形式，以当前的基准开始时间为准，次日在时间后写”+1”，前日的在时间后写”-1”，以此类推，如：
	前日：

	````xml
		“<0><22:00-1><23:30-1><Security>”
	````

	当日：

	````xml
		“<12><19:30><20:00><China Report>”
	````

	次日：

	````xml
		“<32><12:00+1><12:50+1><China News>”
	````

	第一项<数字>表示列号，获取的select_column属性即为获取的列号，第二项为起始时间，第三项为结束时间，第四项为节目名。

# file_image

## 属性

- 固化属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| name          | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，表示组件的名称 |
| style         | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，名称与style.xml中某个 组件风格名相同 |
| rect          | 组件坐标，顺序为[左，上，宽，高] |

- 可设/读属性

无

- 动作属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| load_img      | 加载要显示的图片路径                |
| load_zoom_img | 加载2/4/8倍放大的JPEG图片路径       |
| load_scal_img | 加载图片能自由缩放                  |
| draw_gif      | 绘制gif图片一帧                     |

## 信号函数

|   信号      |             说明                    |
|-------------|-------------------------------------|
| create      | 当此函数被注册，则在该组件被创建时调用相应的注册函数 |
| destroy     | 当此函数被注册，则在该组件被销毁时调用相应的注册函数 |

## Tips:

- 可以通过路径显示的图片组件，但设完属性后该路径下的图片不能立即删除。

# window

## 属性

- 固化属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| name          | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成, 表示组件名称 |
| style_name    | 由26个大（小）写英文字母、阿拉伯数字以及下划线、符号组成，与style.xml中组件风格名相同 |
| rect          | 组件坐标，顺序为[左，上，宽，高] |

- 可设/读属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| forecolor     | 表示组件的前景色，一般情况下是组件的文字颜色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 |
| backcolor     | 表示组件的背景色，一般情况下是组件的底色。由三个颜色分量组成，每个颜色值为#RRGGBB形式。或者可以在color.xml中添加颜色，并选择对应颜色名。 |
| format        | 配置普通对话框(dialog)或弹出式(popup)对话框,其中弹出式对话框在消失时只更新遮挡部分,普通对话框则全部更新 |
| back_ground   | 打到SPP层上的背景图片，为JPEG图片 |
| image         | 对应image.xml中的图片ID名称,在OSD层上绘制 |

- 动作属性

|   属性名      |             说明                    |
|---------------|-------------------------------------|
| move_window_x | 设置需要向X方向移动的像素，以16位无符号整型传入 |
| move_window_y | 设置需要向Y方向移动的像素，以16位无符号整型传入 |
| update        | 更新窗体内所有的组件 |

## 信号函数

|   信号      |             说明                    |
|-------------|-------------------------------------|
| create      | 当此函数被注册，则在该组件被创建时调用相应的注册函数 |
| destroy     | 当此函数被注册，则在该组件被销毁时调用相应的注册函数 |
| show        | 当此函数被注册,则在绘制整个窗口完成后被调用 |
| got_focus   | 当此函数被注册，此窗体获取焦点时会调用相应注册函数 |
| lost_focus  | 当此函数被注册，此窗体失去焦点是会调用相应注册函数 |
| change      | 当在容器中聚焦组件改变时，发出此信号 |
| service     | 当需要相应经过GUI_View传来的，其他Serviece的消息时发出此信号 |
| keypress    | 当此函数被注册，则在当前聚焦组件为此组件情况下，按键到来时，调用相应的注册函数 |

## Tips:

- window作为所有组件的总容器，所有对话框必须以window作为最外层容器；

- 既有back_ground属性，也有image属性，区别在于back_ground是往SPP层上贴的，image是往OSD层上贴的。在32位色两者无区别，在32位色以下，向OSD层贴背景图片（贴别是渐变较明显的背景图片），失真较大。

#prompt

## 属性

prompt无特别属性，属性与window同，作为容器可以包含其他任何组件, 通常将所有属性配置写在style.xml中。

## 信号函数

|   信号      |             说明                    |
|-------------|-------------------------------------|
| create      | 当此函数被注册，则在该组件被创建时调用相应的注册函数 |
| destroy     | 当此函数被注册，则在该组件被销毁时调用相应的注册函数 |

## Tips:

- 一般将组件描述放在style.xml中；

- 创建采用GUI_CreatePrompt接口：

   **单键阻塞型**

	````cpp
		GUI_CreatePrompt(270, 210, "prompt_single_ok", "style_single_ok", "Is it OK?", "OK");
	````

   **双键阻塞型**

	````cpp
		GUI_CreatePrompt(270, 210, "prompt_double_yesno", "style_double_yesno", "Do you want to save?", "Yes|No");
	````

   **飘窗型**

	````cpp
		GUI_CreatePrompt(270, 210, "prompt_ontop", "style_ontop", "Information", "ontop");
	````
	三种形态，均在最后一个mode参数中体现：单键采用除ontop外的任何字符串、双键用"&#124;"分隔、飘窗统一用"ontop"。

