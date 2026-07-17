#!/bin/bash

theme=$1
SOURCE_PATH=$PWD/../../
langs=$(grep "id=" $SOURCE_PATH/theme/$theme/language/i18n.xml |cut -d '>' -f 2|cut -d '<' -f 1)
for lang in $langs
do
    echo $lang
    grep "tran id=" $SOURCE_PATH/theme/$theme/language/$lang |cut -d '"' -f 2 >ids
    while read id
    do
        id1=$(echo $id | sed 's/\./\\./')
        count=$(grep -c "^$id1$" ids)
        if [[ $count -gt 1 ]];then
            echo "$theme/language/$lang id $id repeat,please check"
        fi
    done <ids
    rm ids

    grep  "tran id=" $SOURCE_PATH/theme/$theme/language/$lang |cut -d '>' -f 2 |cut -d '<' -f 1 >trans
    while read tran
    do
        tran1=$(echo $tran | sed 's/\./\\./')
        count=$(grep -c "^$tran1$" trans)
        if [[ $count -gt 1 ]];then
            echo "$theme/language/$lang tran $tran repeat,please check"
        fi
    done <trans
    rm trans
done
