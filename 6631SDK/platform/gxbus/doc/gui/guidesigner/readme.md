
# 概要

随着杭州国信科技芯片产品类别增多、功能扩充和应用范围增大，应用软件的开发规模也随之增大。在界面设计阶段，GUI Designer 作
为可视化辅助界面设计工具，将界面设计与软件业务逻辑开发分开，真正做到界面布局设计的可视化。

- 界面基础元素可视化配置：从界面宽、高、透明色等基本信息的配置，到组件颜色、图标的配置，均在工具的可视化环境下进行，为界
面设计提供了清晰、直观的设计环境
- 可拖拽组件：组件即可通过拖曳至指定区域，也可通过调整位置、宽高参数在可视化环境下进行
- 一键生成主题包：所有操作完成后，一键保存即可导出用 XML 描述的界面描述集，可直接用于方案的界面显示

主题包生成后，可在主题包基础上，进行进一步的应用业务逻辑开发。

## 组织结构

- [第一部分](readme.md)：概要及基础的版本信息，及每个版本的 change log。其中 GUI Designer 工具的帮助信息有链接到当前版本信息。
- [第二部分](introduction.md)：GUI Designer 的简介，会结合 GxGUI 简要介绍 GUI Designer 工具的基本信息。
- [第三部分](guide.md)：从 GUI Designer 新创建一个 GUI 界面工程、新创建一个窗口和调整属性及信号介绍 GUI Designer 工具的详细使用。
- [第四部分](faq.md)：针对使用中遇到的问题、场景介绍 GUI Designer 工具的处理方式。

## 读者对象

本小节内容为使用 GoXceed 平台 GxGUI 的工程师而写，主要为使用、测试和维护使用 GxGUI 来设计、测试界面而设计，GUI Designer
只与 GxGUI 配套使用。

- 技术支持工程师
- 软件开发工程师
- 测试开发工程师

## 修订记录

### v1.0.1

- 初始版本

### v1.0.2

- [313142](https://git.nationalchip.com/redmine/issues/313142) GUI Designer 里缺少button的focus_font属性
- [315008](https://git.nationalchip.com/redmine/issues/315008) GIEC杰科-GX3211-GX6611-数码无卡-CA的OSD滚动字符串字符抖动问题

### v1.0.3

- [340585](https://git.nationalchip.com/redmine/issues/340585) {PVR-RECF}对免费频道重命名后再进行即时录制保存，录制文件的名称显示有误(Cygnus_GX6706S5_eCos_方案)
- [302222](https://git.nationalchip.com/redmine/issues/302222) GUI_Designer 工具添加帮助文档

