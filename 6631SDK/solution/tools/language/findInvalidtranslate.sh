#!/bin/bash
#功能:找出已翻译但未使用的词组

CURRENT_PATH=$(pwd)
SOLUTION_PATH=$(cd $CURRENT_PATH/../../; pwd)

if [ -d temp ];then
    rm -rf temp
    mkdir temp
else
    mkdir temp
fi

if [ -f invalid_translate ];then
    rm -f invalid_translate
fi

function find_all_translated_str()
{
    grep -r "id=" $SOLUTION_PATH/theme/linux/language/English.xml | cut -f 2 -d "\"" | sort -u > temp/all_translated_str
}

#找出所有被定义为宏且使用过的字符串
function find_all_used_define_str()
{
    egrep -r " +STR_.+" $SOLUTION_PATH/app/include/app_str_id.h | \
    awk '{
        print $2
    }'>temp/all_define

    egrep -r " +STR_.+" $SOLUTION_PATH/app/include/app_str_id.h | cut -f 2 -d "\"" >temp/all_define_str

    cat temp/all_define | while read -r line
    do
        grep -rq $line $SOLUTION_PATH/app/ --include "*.c"
        if [ $? -eq 0 ];then
            echo $line
        else
            echo ""
        fi
    done >temp/all_used_define

    i=0
    cat temp/all_used_define | while read -r line_id
    do
        let i++
        if [ -n "$line_id" ];then
            sed -n ${i}p temp/all_define_str >>temp/all_define_used_str
        fi
    done
}


function clear_used_define_str()
{
    mv temp/all_define_used_str temp/all_define_used_str_bak
    sort -u temp/all_define_used_str_bak > temp/all_define_used_str
    comm temp/all_translated_str temp/all_define_used_str -2 -3  >temp/first_clear_translate
}

#其它字符串使用包括GUI_Set(xxx,"string",xxx),直接设置在xml文件中，以及指针引用字符串等
function clear_other_used_str()
{
    cat temp/first_clear_translate | while read -r line_other
    do
        grep -rq "$line_other" $SOLUTION_PATH/app/ --include "*.c" || grep -rq "$line_other" $SOLUTION_PATH/theme/linux/widget/ --include "*.xml"
        if [ $? -ne 0 ];then
            echo $line_other >>invalid_translate
        fi
    done
}


find_all_translated_str
find_all_used_define_str
clear_used_define_str
clear_other_used_str

echo -e "\033[1;33m Some of the extra translations are as follows..............\033[0m\n"
cat -n invalid_translate

rm -rf temp
