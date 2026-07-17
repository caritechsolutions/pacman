# 常见问题

- Q: 为什么传入的红外遥控键值是对的，但是进入低功耗红外遥控器无法唤醒？
- A:  一般这种情况请检查芯片的外部晶振是不是 24MHz。如果是 24MHz，调用 GxCore_Halt 时设置 cmdline 参数包含 "lowpower_clock_speed=24000000"。

