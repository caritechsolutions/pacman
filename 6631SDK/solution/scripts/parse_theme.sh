#!/bin/bash
##############################################################################
# Blacker, liujh@nationalchip.com
# Copyright(C) 2013-2020, HangZhou NationalChip Science and Technology Corp.
# parse theme
##############################################################################

theme_name=$1
family_name=$2
os_name=$3
proj_name=$4

function message_error()
{
	local lv_vendor_conf=$1
	local lv_proj_conf=$2
	local lv_os_conf=$3
	local lv_theme_conf=$4 
	local lv_theme_match=$5

        echo -e "Vendor: $lv_vendor_conf "
        echo -e "Project: $lv_proj_conf "
        echo -e "OS: $lv_os_conf "
        echo -e "Input theme: \e[031m $lv_theme_conf \e[0m"
        echo -e "Theme must be: \e[032m $lv_theme_match\e[0m"
}

if [[ ( $theme_name == "" ) || ( $family_name == "" ) || ( $os_name == "" ) || ( $proj_name == "" ) ]];then
	echo -e "\e[031mrun $0 error.\e[0m"
	echo -e "\e[031mPlease check input param.\e[0m"
	exit 1
fi

echo -e "Current configured theme : \e[43;31;1m   $theme_name    \e[0m"
echo ""

#get theme name from rules config file: $theme_config_file
theme_config_file=scripts/rule_theme.conf
get_theme_list=`cat $theme_config_file | grep -v "^#" | awk -F":" -v family=$family_name -v os=$os_name 'BEGIN{str="";}{if(match($1,family) > 0 && match($2,os) > 0) str=str" "$3;} END{print str}' `
echo "Get Theme List:" $get_theme_list

if [[ $get_theme_list == "" ]]; then
	echo -e "\e[031mError when get theme from $theme_config_file\e[0m"
	exit 1
fi

find_theme=`echo $get_theme_list | grep -c $theme_name `
if [[ $find_theme == 0 ]];then
	message_error $family_name $proj_name $os_name $theme_name "$get_theme_list"
	exit 1
fi

select_font_chinese=`cat .config|grep "=y" | grep "BR2_FONT" |awk -F"=" '{print $1}'|awk -F"_" '{print $NF}'`
build_dir=$(pwd)
theme_lang_path=$build_dir/theme/${BR2_THEME_TEMPLATE_NAME}/language		     
if [[ $select_font_chinese == "MATRIX" ]];then
	iconv -c -f UTF-8 -t gb2312 $theme_lang_path/Chinese.xml -o $theme_lang_path/Chinese_gb2312
	mv $theme_lang_path/Chinese_gb2312 $theme_lang_path/Chinese.xml
fi

