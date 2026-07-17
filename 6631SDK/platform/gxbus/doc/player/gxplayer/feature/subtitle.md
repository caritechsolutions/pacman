# 字幕功能

## 概述

- [字幕背景知识](../../background/background.md#字幕)
  - 字幕根据应用方式可分为[外挂字幕](../../background/background.md#外挂字幕)、[内置字幕](../../background/background.md#内置字幕)、[DVB 字幕](../../background/background.md#dvb-字幕)、[CC 字幕](../../background/background.md#cc-字幕) 等四种。

- gxplayer 支持的字幕类型包括以下:
  - 外挂字幕:
    - 文本类型字幕, 常见后缀：[".lrc"](../../background/background.md#lrc) , [".ssa](../../background/background.md#ass)" , [".ass"](../../background/background.md#ass) , [".srt"](../../background/background.md#srt) , ".smi" , ".txt" , [".vtt"](../../background/background.md#vtt) .
    - 位图类型字幕, 常见后缀: [".idx" + ".sub" + ".ifo"](../../background/background.md#idx) , ".ifo" 可有可无 .
  - 内置字幕:
    - 文本类型字幕, 常见编码: [SSA / ASS](../../background/background.md#ass) , TEXT , [SRT](../../background/background.md#srt) ．
    - 位图类型字幕, 常见编码: [DVD](../../background/background.md#dvd) , [XSUB](../../background/background.md#xsub) , [PGS](../../background/background.md#pgs) .
  - DVB 字幕
    - 文本类型字幕, 常见编码有: [DVB-TELETEXT](../../background/background.md#dvb-teletext) , [DVB-MAGEZINE](../../background/background.md#dvb-teletext) .
    - 位图类型字幕, 常见编码有: [DVB-SUBTITLE](../../background/background.md#dvb-subtitle) .
  - CC 字幕
    - 文本类型字幕, 常见编码有: [ATSC-CC](../../background/background.md#atsc-cc) , [ARIB-CC](../../background/background.md#arib-cc) .
    - 位图类型字幕, 常见编码有: [SCTE-CC](../../background/background.md#scte-cc) .

- gxplayer 支持的字幕默认在 SPP 层显示, 应用可以通过自定义绘制来实现 OSD 层显示, 应用需要实现[自定义绘制接口](#自定义绘制接口).

- gxplayer 支持的字幕默认使用 GUI 的 GDI 绘制接口实现绘制功能.

### 配置

- **注册模块**:
  - 可以通过选择注册模块来缩减 flash 以及内存大小. [Flash 相关数据](), [内存相关数据]().

| 功能                                 | 接口                                              |
| ------------------------------------ | ------------------------------------------------- |
| 注册所有字幕模块                     | [**GxPlayer_ModuleRegisterSubtitleALL**](../group__gxplayer__register_1gab246bf7782e80b3e9dbd9d0a0bee7b0d.md)  |
| 注册内置字幕模块                     | [**GxPlayer_ModuleRegisterSubtitleInside**](../group__gxplayer__register_1ga87f2513d55047f7a12bbba64267f333b.md) |
| 注册外挂字幕模块                     | [**GxPlayer_ModuleRegisterSubtitleOutside**](../group__gxplayer__register_1ga0cdc874d24e3f7e1e85c16277eeeb3b1.md) |
| 注册 DVB-SUBTITLE 模块               | [**GxPlayer_ModuleRegisterSubtitleDVB**](../group__gxplayer__register_1gadaa2f9868c06e0256f7c0f6e4f1f5168.md)   |
| 注册 DVB-TELETEXT, DVB-MAGAZINE 模块 | [**GxPlayer_ModuleRegisterSubtitleTeletext**](../group__gxplayer__register_1gae2a91813789e36152a565d08b267b7a0.md) |
| 注册 SCTE 模块                       | [**GxPlayer_ModuleRegisterSubtitleScte**](../group__gxplayer__register_1ga5857e26ee106c211e087c30c740cea93.md)  |
| 注册 ARIB-CC 模块                    | [**GxPlayer_ModuleRegisterSubtitleAribCC**](../group__gxplayer__register_1ga05b3a43707a1e555353012c08eef4b10.md) |
| 注册 ATSC-CC 模块                    | [**GxPlayer_ModuleRegisterSubtitleAtscCC**](../group__gxplayer__register_1ga2f4d0b22bff490cef6a35e9c23752ad4.md) |

- <span id="自定义绘制接口">**注册自定义绘制**</span>:
  - 默认绘制满足应用需求时, 可以不注册.
  - 默认绘制不满足应用需求时, 需要应用根据需求实现绘制功能, 并注册对应实现的绘制功能.
  - 取消对应功能注册时, 恢复 gxplayer 的默认绘制功能.

| 控制                     | 接口                                           | 说明                                                    |
| ------------------------ | ---------------------------------------------- | ------------------------------------------------------- |
| 注册文本自定义绘制       | [**GxSubtitle_ModuleRegisterShowText**](../group__gxsubtitle_1gac3219e3a1718312c5d973423e2be1910.md)     | 只对 **内置字幕、外挂字幕** 下的 **文本类型字幕** 有效  |
| 取消注册文本自定义绘制   | [**GxSubtitle_ModuleUnRegisterShowText**](../group__gxsubtitle_1ga3397e026f2a46f29a35370bc7e0172e1.md)    | 只对 **内置字幕、外挂字幕** 下的 **文本类型字幕** 有效 |
| 注册位图自定义绘制       | [**GxSubtitle_ModuleRegisterShowPixel**](../group__gxsubtitle_1gaaddb7298e9d3492f89bbf0a3dde496a2.md)    | 只对 **内置字幕、外挂字幕** 下的 **位图类型字幕** , **DVB 字幕** , **CC 字幕** 有效 |
| 取消注册位图自定义绘制   | [**GxSubtitle_ModuleUnRegisterShowPixel**](../group__gxsubtitle_1gab3e66e925be532d277daf43cd8ff6bfc.md)  | 只对 **内置字幕、外挂字幕** 下的 **位图类型字幕** , **DVB 字幕** , **CC 字幕** 有效 |

- **通用配置**:
  - 以上配置都有默认配置值, 应用根据应用场景做相应配置, 无特殊需求可以不用配置.

| 功能                       | 接口                                    |
| -------------------------- | --------------------------------------- |
| 配置字幕显示区域           | [**GxSubtitle_ConfigScreen**](../group__gxsubtitle_1gae044256e6863601574931dd159ef04f6.md)        |
| 配置字幕画布大小           | [**GxSubtitle_SetDisplay**](../group__gxsubtitle_1gaa5203798442e55b558e99524a8a50afc.md)          |
| 配置字幕画布绘制区域       | [**GxSubtitle_SetDisplay**](../group__gxsubtitle_1gaa5203798442e55b558e99524a8a50afc.md)          |
| 配置字幕画布绘制区域下边距 | [**GxSubtitle_SetDisplay**](../group__gxsubtitle_1gaa5203798442e55b558e99524a8a50afc.md)DefaultYB |
| 配置字幕画布绘制背景颜色   | [**GxSubtitle_SetBackColor**](../group__gxsubtitle_1ga714eb2464aff019e193ae60cc37a7f69.md)        |

- **特定配置**:
  - 以上配置字幕都有默认配置值, 应用根据显示效果做相应配置,也可以不用配置.

| 功能                     | 接口                                 | 说明                                           |
| ------------------------ | ------------------------------------ | ---------------------------------------------- |
| 配置字幕的字体和大小     | [**GxSubtitle_SetFontLib**](../group__gxsubtitle_1gafa7f755dd44c9448a19b9216996c6e09.md)       | 只对 **内置字幕、外挂字幕** 下的 **文本类型字幕** 有效 |
| 配置字幕的字体颜色       | [**GxSubtitle_SetFontColor**](../group__gxsubtitle_1ga981df879f309a6117b2b8627f7876f93.md)     | 只对 **内置字幕、外挂字幕** 下的 **文本类型字幕** 有效 |
| 配置字幕的国家语言       | [**GxSubtitle_SetLang**](../group__gxsubtitle_1ga89ae3b6f16396862a7677eeaf9b90582.md)          | 只对 **DVB-TELETEXT、DVB-MAGEZINE** 有效           |
| 配置字幕的显示方式       | [**GxSubtitle_ConfigAtscccMode**](../group__gxsubtitle_1gab019f2d09c8aad9d15a3bb6334574419.md) | 只对 **ATSC-CC** 有效                              |
| 配置字幕的源宽高大小限制 | [**GxSubtitle_ConfigLimitSrcCV**](../group__gxsubtitle_1gabb03603368bf12411275e00671ee310e.md) | 只对 **DVB-SUBTILTE** 有效                         |
| 配置字幕的调色板         | [**GxSubtitle_SetDvbSubtClut**](../group__gxsubtitle_1gac7cee9328a4ace393110fad0e04b1a58.md)   | 只对 **DVB-SUBTILTE** 有效                         |

## 启动与停止

- **启动与停止**:
  - 字幕的加载需要在[启动播放节目]()之后才能加载成功, 字幕的卸载需要在[停止播放节目]()之前才能卸载成功.

| 功能                   | 接口                             | 说明                                           |
| ---------------------- | -------------------------------- | ---------------------------------------------- |
| 字幕启动, 即字幕的加载 | [**GxPlayer_MediaSubLoad**](../group__gxplayer__subtitle_1gabc81964dd6ade7708849d6b6bb4a4a41.md)   | 对所有字幕有效 |
| 字幕停止, 即字幕的卸载 | [**GxPlayer_MediaSubUnLoad**](../group__gxplayer__subtitle_1gadc2f118cb3f307370b064c9593058a61.md) | 对所有字幕有效 |

## 控制

- **通用控制**:

| 功能           | 接口                             |
| -------------- | -------------------------------- |
| 字幕的隐藏     | [**GxPlayer_MediaSubHide**](../group__gxplayer__subtitle_1ga71f94bbc3e7a3d50038a4a1fb24ee142.md)   | 对所有字幕有效 |
| 字幕的显示     | [**GxPlayer_MediaSubShow**](../group__gxplayer__subtitle_1gadda0347686e008ae478dbad1ebc9c208.md)   | 对所有字幕有效 |
| 字幕的同步调节 | [**GxPlayer_MediaSubSync**](../group__gxplayer__subtitle_1gaa258822378f1ce37bec053363abfe08c.md)   | 对所有字幕有效 |
| 字幕的切换     | [**GxPlayer_MediaSubSwitch**](../group__gxplayer__subtitle_1gaf39fb8659e00b46c1fc8a4752e6afa04.md) | 对所有字幕有效 |
| 字幕的时间获取 | \ref **GxPlayer_MediaSubGetTime** | 对外挂字幕时有效 |
| 字幕的跳转     | \ref **GxPlayer_MediaSubGotoLocalTime** | 对外挂字幕时有效 |

- **ATSC-CC 字幕控制**:

| 功能                 | 接口                                   |
| -------------------- | -------------------------------------- |
| 获取服务信息         | [**GxSubtitle_ATSCGetServiceInfo**](../group__gxsubtitle_1ga0edecc10571f8981dcd425635cff8de4.md) |
| 切换服务, 即切换字幕 | [**GxSubtitle_ATSCSwitchService**](../group__gxsubtitle_1ga762a9a61cd6df5758952aa04ebe0b88b.md)  |

- **DVB-MAGZINE 字幕**:
  - 字幕首页信息由应用加载 DVB-MAGZINE 字幕时传入.
  - 加载 DVB-MAGZINE 字幕后, 显示页面时默认页, 例如: "Please wait ...".
  - 显示首页需要调用跳转首页接口, 并且需要有首页字幕数据时, 才能显示.

| 控制                                   | 接口                                     |
| -------------------------------------- | ---------------------------------------- |
| 跳转首页                               | [**GxSubtitle_MagFirstPage**](../group__gxsubtitle_1gaf69f62a8762d061a1a4162744d1fc0dc.md)         |
| 跳转指定页面                           | [**GxSubtitle_MagJumpPage**](../group__gxsubtitle_1ga6928ad8676b9c7747dbccc617f313469.md)          |
| 跳转下一页面                           | [**GxSubtitle_MagNextPage**](../group__gxsubtitle_1ga0d03ad43d0a2cf936c9c02f6c6e57268.md)          |
| 跳转上一页面                           | [**GxSubtitle_MagPrevPage**](../group__gxsubtitle_1ga3dbe944ad8d567d7fd8583f36259ff50.md)          |
| 跳转当前页面下一子页                   | [**GxSubtitle_MagNextSubPage**](../group__gxsubtitle_1ga43897ef26335008e3861fc7be26340ea.md)       |
| 跳转当前页面上一子页                   | [**GxSubtitle_MagPrevSubPage**](../group__gxsubtitle_1gadb3a7aed678f205ed296b18769b24581.md)        |
| 自定义区域显示字符, 一般显示跳转页面号 | [**GxSubtitle_MagShowString**](../group__gxsubtitle_1gab9ee9f9d44206c9b16d476557e5b0119.md)        |
| 获取首页页面号                         | [**GxSubtitle_MagGetFirstPgNo**](../group__gxsubtitle_1ga88e006aa82ceeb9f682294efe22cad8a.md)      |
| 获取当前页面号                         | [**GxSubtitle_MagGetCurrentPgNo**](../group__gxsubtitle_1ga9101c36ce698dd228ea8173fb7c562a1.md)    |
| 获取当前页面子页号                     | [**GxSubtitle_MagGetCurrentSubPgNo**](../group__gxsubtitle_1gae5af438c7b3ee83c55683b8cabcda6ef.md) |
| 获取下一页面号                         | [**GxSubtitle_MagGetNextPgNo**](../group__gxsubtitle_1ga0eaf9eda80dd265c2edfe9f90d32fc33.md)       |
| 获取上一页面号                         | [**GxSubtitle_MagGetPrevPgNo**](../group__gxsubtitle_1ga9224b40ab85c3fe125f10237af08ce46.md)       |
| 获取六个链接页面                       | [**GxSubtitle_MagGetSixLinkPgNo**](../group__gxsubtitle_1ga74cb2a8bdcd0a04e441cf45b72573217.md)    |
| 控制页面透明度                         | [**GxSubtitle_MagSetOpacity**](../group__gxsubtitle_1ga17842cdeea31191545fe3f899fd1a93d.md)        |
