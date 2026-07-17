## 系统工具

### 流媒体服务器搭建

### AVMemCal

- 使用 AvMemCal.sh 计算 GxPlayer 所需的 **[videomem]()** 大小.
- NNE 芯片比较特殊，下面这些字段需要在 **[cmdline]()** 中显式指定出来，例如：

```
  // Irdeto
  videomem = player + ui，player 需要40M（高清PP）/52M(全高清PP). 以下计算假设UI需要10M
  高清PP：  mem=71M videomem=50M esamem=128K esvmem=3880K vfwmem=280K afwmem=2M svpumem=832K mem_end
  全高清PP：mem=59M videomem=62M esamem=128K esvmem=3880K vfwmem=280K afwmem=2M svpumem=832K mem_end
  支持 Ac3 时, afwmem=2560K
  // VMX
  mem=158M videomem=52M esamem=512K esvmem=4M tsrmem=188K tswmem=4136K vfwmem=512K afwmem=2560K svpumem=1308K secmem=1M teemem=32M mem_end
```

