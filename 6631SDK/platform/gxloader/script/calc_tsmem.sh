#!/bin/bash

# 定义函数来计算最大公约数
gcd() {
    if [ $2 -eq 0 ]; then
        echo $1
    else
        gcd $2 $(($1 % $2))
    fi
}

# 读取用户输入
read -p "请输入 blockLen : " blockLen
read -p "请输入 tswmem 大小 (byte): " tswmemLen
read -p "请输入 tsrmem 大小 (byte): " tsrmemLen

let k1=1024
let k2=64

lcm1=$((blockLen * k1 / $(gcd $blockLen $k1)))
lcm2=$((blockLen * k2 / $(gcd $blockLen $k2)))

echo ""
echo "最小 K 对齐长度为: $lcm1  // 如果不设置 tsxmem_sub 字段，则 tswmem 、tsrmem 都需要设置成这个大小"
echo "最小 block 对齐长度为: $lcm2"
echo "tswmem_sub 长度为: $((tswmemLen / lcm2 * lcm2))"
echo "tsrmem_sub 长度为: $((tsrmemLen / lcm2 * lcm2))"

