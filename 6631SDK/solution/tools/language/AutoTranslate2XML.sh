#!/bin/bash

# 检查参数数量
if [ "$#" -lt 3 ]; then
    echo "用法: ./AutoTranslate2XML.sh <目录路径> -e <翻译词条>"
    echo "    : ./AutoTranslate2XML.sh <目录路径> -f <文件名>"
    echo "       <文件名> 内容为翻译词条集合，一行一个词条，每行词条需带引号"
    echo " eg: ./AutoTranslate2XML.sh ../../theme -e \"tanslationword1\""
    echo " eg: ./AutoTranslate2XML.sh ../../theme -f trans.txt"
    exit 1
fi

# 获取参数
DIRECTORY_PATH=$1
OPTION=$2
ARGUMENT=$3

# 根据选项执行不同的命令
if [ "$OPTION" == "-e" ] && [ "$#" -eq 4 ]; then
    STRING=$4
    echo "执行: python3 auto_translate_all_theme.py $DIRECTORY_PATH -e \"$STRING\""
    python3 auto_translate_all_theme.py "$DIRECTORY_PATH" -e "$STRING"
    
    echo "执行: python3 i18n_multi_all_theme.py $DIRECTORY_PATH"
    python3 i18n_multi_all_theme.py "$DIRECTORY_PATH"

elif [ "$OPTION" == "-f" ] && [ "$#" -eq 3 ]; then
    FILENAME=$ARGUMENT
    echo "执行: python3 auto_translate_all_theme.py $DIRECTORY_PATH -f \"$FILENAME\""
    python3 auto_translate_all_theme.py "$DIRECTORY_PATH" -f "$FILENAME"
    
    echo "执行: python3 i18n_multi_all_theme.py $DIRECTORY_PATH"
    python3 i18n_multi_all_theme.py "$DIRECTORY_PATH"

else
    echo "无效的参数组合。"
    echo "用法: ./AutoTranslate2XML.sh <目录路径> -e <字符串> 或 ./AutoTranslate2XML.sh <目录路径> -f <文件名>"
    exit 1
fi

