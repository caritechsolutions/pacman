# FAQ

GxDownloader 的工程模式和方案模式升级方式区别？

方案模式升级方式存在一定的风险，由于我们 UI 加载字库，小内存方案不能把全部字库加载到内存，这样在方案模式升级的时候显示进度的时候可能会读取 Flash ，然后这个时候有写 Flash 就存在问题，还有就是文件系统什么时候刷 Flash 不能保证。GxDownloader 提供统一进入工程模式，方案只要设置升级标记然后重启就进入工程模式升级，保证了安全性。

工程模式字库都是一次性加到内存，并且没有使用文件系统，没有方案模式提到的几个风险问题。

---


为什么原来 OTA 通过分区表跳转，GxDownloader 采用固定地址方式引导？

根据分区跳转方便的用户集成，只要关心 flash.conf 的配置就跳转，但是这里存在一定的风险和限制，这样分区表就不能坏，同时产生 App 就不去升级分区表。

---

为什么 gxloader 不直接集成 GxDownloader 功能？

Flash 空间问题，GxDownloader 支持 LZMA 压缩。举个例子 Irdeto 的实际情况，gxloader 集成了GxDownloader 编译出来 bin 需要 598 KBytes，如果分成两个bin，分别为 64 KBytes和 189 KBytes，实际效果显著，如果真的需要空间还可以继续把 gxloader 分区裁剪把 GxDownloader 分区合并起来。


---

OEM 和DLP 分区有什么区别?

OEM 分区是原来 APP 和 OTA 交互的 Flash 分区，但是原来这个块由用户自己定义分区内容和格式，bus 里面有个 OEM 接口，OTA 里面有一份 OEM 的接口，另外 gxloader 也需要一份 OEM 的解析标志位代码，这样有隐患需要3个部分对应。DLP 分区就是把原来3个部分整合为一个部分统一由 GxDownloader 提供接口维护。

---

那 DLP 分区内容，通过 OEM 接口还是可以操作吗？

不可以，因为 OEM 接口操作 DLP 分区是非法的，必须使用 DLP 的 API 接口，OEM 的接口只能读取 OEM 分区，假设有人用的 name 不是OEM，这样 OEM 接口就读失败 了，还有我现在是配套一个接口，假如我的数据更新了 DLP 分区的字段，你 OEM 接口怎么写？

---

工程模式和方案模式 Flash config 的差别？

工程模式需要一个单独的 OTA 分区，方案模式不需要 OTA 分区，但是这两个 Flash.conf 都需要 DLP 分区
