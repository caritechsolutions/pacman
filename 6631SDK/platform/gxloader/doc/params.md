

# Stage1配置参数

## 设计stage1配置参数的目的

* 由于同一款芯片，因为不同板级(如taurus的6617U1和6617U5), 不同内存，晶振不同，内存规划不同等，编译生成的gxloader的stage1会不同，这样会需要签名多份stage1。现在为了只签名一份stage1, 必须保证stage1不因前面提到的几个因素不同造成stage1不变。因此增加了配置参数，stage1通过读取配置参数配置内存，时钟等。

## 配置参数解析

配置参数为2KBytes大小，位置在在stage2前面，但是并没有用全部，有些是reserved的，如果有签名，签名的数据会放在2KBytes-256的地方
taurus芯片配置参数:

```
struct board_param_info {
▸       unsigned int extern_clock_xtal; //外部晶振
▸       unsigned int cpu_frequency;  //cpu 主频
▸       unsigned int stage2_stack_top_addr;  //stage2的栈指针
▸       unsigned int stage2_start_addr;      //stage2在内存中的起始位置
▸       unsigned int ddr_type;               //ddr类型 ddr2/ddr3
▸       unsigned int ddr_frequency;          ///ddr频率
▸       unsigned int DENALI_CONFIG_CTL[DENALI_CONFIG_CTL_NUM]; //ddr相位配置
▸       unsigned int DENALI_CONFIG_PHY[DENALI_CONFIG_PHY_NUM]; //ddr相位配置
}__attribute__ ((packed));
```

## 配置参数使用

* stage1启动后会立马读取配置参数，根据配置参数去配置cpu频率和主频，跳转到stage2

