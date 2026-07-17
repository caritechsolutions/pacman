 |  属性    |          说明           |
 |----------|-------------------------|
 |  osd_language  |  设置OSD语言      |
 | osd_alpha_global | 设置OSD全局透明度 |
 | osd_alpha |   设置OSD透明度(与之前的属性区别在于切换透明度时此属性不会用矩形填充) |
 | flush     | 立即完成绘制           |
 | video_enable | 设置视频层使能      |
 | video_disable | 设置视频层实效即使能image层 |
 | logic_clut  | 设置逻辑色表(当8位色方案，需要切换到新CLUT界面时调用) |
 | clear_image | 将image层清黑  |
 | free_space  | 暂时将osd、spp或图片数据(fragment)空间释放，使用"&#124;"来分隔，其中fragment属性与flush效果一致  |
