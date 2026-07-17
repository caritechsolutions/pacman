#!/bin/bash
##############################################################################
# Blacker, liujh@nationalchip.com
# Copyright(C) 2013-2020, HangZhou NationalChip Science and Technology Corp.
# parse project language config and generate app_config.h, app_config.c
##############################################################################

output_config_file_h=$1
output_config_file_c=$2
output_config_osd_lang=$3
output_config_kb_lang=$4
output_config_theme_lang_dir=$5

if [[ ( ! -e $output_config_file_h ) || ( ! -e $output_config_file_c ) || ( ! -e $output_config_osd_lang ) ]];then
	echo "Cannot find output file : "
	echo "   $output_config_file_h"
	echo "   $output_config_file_c"
	echo "   $output_config_osd_lang"
	echo "   $output_config_kb_lang"
	exit 1
fi

#######################################################################################################
# OSD Language Setting

echo "#include \"app_config.h\"">> $output_config_file_c
echo "char *g_LangName[] = { ">> $output_config_file_c

#parsing default language
config_lang_num=0

select_lang_default=`cat .config|grep "=y" | grep "BR2_LANG_DEFAULT_" |awk -F"=" '{print $1}'|awk -F"_" '{print $NF}'`
select_lang_capital=`echo $select_lang_default | tr [:upper:] [:lower:] | sed 's/\b[a-z]/\U&/g'`

echo -e "Default Language:\033[032m["$select_lang_default"]\033[0m"
if [[ -n $select_lang_default ]];then
	echo "#define LANG_${select_lang_default} \"${select_lang_capital}\"," >> $output_config_file_h
	echo "LANG_${select_lang_default}" >> $output_config_file_c
	echo "${select_lang_capital}" >> $output_config_osd_lang
	echo "#${select_lang_capital}" >> $output_config_osd_lang
	config_lang_num=$((config_lang_num+1))
fi

# parsing language
select_lang_list=`cat .config|grep "=y" | grep "BR2_LANG_" | grep -v "$select_lang_default" | awk -F"=" '{print $1}'|awk -F"_" '{print $NF}' |sort `

echo -e "Selected Lanuage:\033[032m["$select_lang_default $select_lang_list"]\033[0m"
for each_lang in $select_lang_list
do
	each_lang_capital=`echo $each_lang | tr [:upper:] [:lower:] | sed 's/\b[a-z]/\U&/g'`
	echo "#define OSD_LANG_${each_lang} \"${each_lang_capital}\"," >> $output_config_file_h
	echo "OSD_LANG_${each_lang}" >> $output_config_file_c
	echo "#${each_lang_capital}" >> $output_config_osd_lang
	config_lang_num=$((config_lang_num+1))
done

echo "#define LANG_MAX ${config_lang_num}" >> $output_config_file_h
echo "" >> $output_config_file_h
echo "extern char *g_LangName[];" >> $output_config_file_h
echo "" >> $output_config_file_h

echo "};" >> $output_config_file_c
echo "" >> $output_config_file_c

# At least, choose one language
if [[ $config_lang_num == "0" ]];then
        echo -e "\033[031mAt least  select one OSD Language\033[0m"
        exit 1
fi

config_kb_lang_num=0
echo "char *g_KB_LangName[] = { ">> $output_config_file_c

select_kb_lang_default=`cat .config|grep "=y" | grep "BR2_KB_LANG_DEFAULT_" |awk -F"=" '{print $1}'|awk -F"_" '{print $NF}'`
select_kb_lang_capital=`echo $select_kb_lang_default | tr [:upper:] [:lower:] | sed 's/\b[a-z]/\U&/g'`

echo -e "Default KB Language:\033[032m["$select_kb_lang_default"]\033[0m"
if [[ -n $select_kb_lang_default ]];then
	echo "#define KB_LANG_${select_kb_lang_default} \"${select_kb_lang_capital}\"," >> $output_config_file_h
	echo "KB_LANG_${select_kb_lang_default}" >> $output_config_file_c
	echo "${select_kb_lang_capital}" >> $output_config_kb_lang
	echo "#${select_kb_lang_capital}" >> $output_config_kb_lang
	config_kb_lang_num=$((config_kb_lang_num+1))
fi
# parsing language
select_kb_lang_list=`cat .config|grep "=y" | grep "BR2_KB_LANG_" | grep -v "$select_kb_lang_default" | awk -F"=" '{print $1}'|awk -F"_" '{print $NF}' |sort `

echo -e "Selected KB Lanuage:\033[032m["$select_kb_lang_default $select_kb_lang_list"]\033[0m"
for each_lang in $select_kb_lang_list
do
	each_lang_capital=`echo $each_lang | tr [:upper:] [:lower:] | sed 's/\b[a-z]/\U&/g'`
	echo "#define KB_LANG_${each_lang} \"${each_lang_capital}\"," >> $output_config_file_h
	echo "KB_LANG_${each_lang}" >> $output_config_file_c
	echo "#${each_lang_capital}" >> $output_config_kb_lang
	config_kb_lang_num=$((config_kb_lang_num+1))
done

echo "#define KB_LANG_MAX ${config_kb_lang_num}" >> $output_config_file_h
echo "" >> $output_config_file_h
echo "extern char *g_KB_LangName[];" >> $output_config_file_h
echo "" >> $output_config_file_h


echo "};" >> $output_config_file_c
echo "" >> $output_config_file_c

# At least, choose one language
if [[ $config_kb_lang_num == "0" ]];then
        echo -e "\033[031mAt least  select one KEYBOARD Language\033[0m"
        exit 1
fi


## for kb lang item
kb_lang_list=$(cat $config_theme_lang_dir/keyboard.xml|grep "language"| awk -F"\"" '{print $2}' | sort)
echo "enum TIME_SET_LANG_NAME {" >> $output_config_file_h
if [[ -n $select_kb_lang_default ]];then
    echo "  ITEM_$select_kb_lang_default = 0," >> $output_config_file_h
fi

for each_lang in $select_kb_lang_list
do
	echo "  ITEM_${each_lang}," >>$output_config_file_h
done
for each_lang in $kb_lang_list
do
    each_lang_capital=`echo $each_lang | tr '[a-z]' '[A-Z]'`
    if [[ $each_lang_capital = $select_kb_lang_default ]];then
        continue
    fi
    flag="true"
    for i in $select_kb_lang_list
    do
        if [[ $each_lang_capital = $i ]];then
            flag="false"
            break
        fi
    done
    if [[ $flag = "true" ]];then
        echo "  ITEM_${each_lang_capital}," >>$output_config_file_h
    fi
done
echo "  ITEM_LANGUAGE_TOTAL
};" >> $output_config_file_h

echo "" >>$output_config_file_h
