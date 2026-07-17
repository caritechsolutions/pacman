
### 编程指南

- 驱动加载
- videomem计算
- 模块初始化（流程）

	- 修改默认配置(GxBUS_ConfigSet...)
	- 模块初始化

		- GxBus_Init()
		- GxPlayerModuleInit()

	- 模块注册

		- 功能注册(GxPlayerRegister...)
		- 音视频固件注册
		- 回调注册(GxPlayer_SetxxCallback...)

	- 回调注册

		- 事件回调

	- 模块配置

		- GxPlayer_Setting...

	- 视频输出
