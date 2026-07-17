#!/bin/bash
#####################################################################################################
# Blacker, liujh@nationalchip.com
# Copyright (C), 2013-2020, HangZhou NationalChip Sceince and Technology Corp.
# Parse .config file, generate config header/source file, copy resource needed
#####################################################################################################
echo "---------------------------------------------------------"
echo -e "\033[032mParsing configuration \033[0m"

if [ -e .config ];then
	source .config
else
	echo -e "\033[031mCannot find .config file, config first\033[0m"
	echo -e "\033[032mRun make menuconfig please\033[0m"
	exit 1
fi

if [[ $BR2_PROJ_NAME = "" ]]; then
	echo -e "\033[031mCannot find project name, config first\033[0m"
	echo -e "\033[032mRun make menuconfig please\033[0m"
	exit 1
fi

#####################################################################################################
# Set some important variable by manual
# config_chip_product
source scripts/parse_product.sh
CROSS_COMPILE=$2
config_chip_name=${BR2_CHIP_NAME}
config_os_name=${BR2_OS_NAME}
config_proj_name=${BR2_PROJ_NAME}
config_family_name=${BR2_FAMILY_NAME}
config_build_dir=$(pwd)
config_debug_dir=${config_build_dir}/system/debug/${config_os_name}
config_proj_dir=${config_build_dir}/projects/${config_family_name}/${config_proj_name}
config_output_dir=${config_build_dir}/output
config_theme_dst_dir=${config_output_dir}/theme
config_weblet_dst_dir=${config_build_dir}/app/netapps/weblet/www/

########################################################################################
# select theme template
selected_theme_name=${BR2_THEME_TEMPLATE_NAME}

source scripts/parse_theme.sh $selected_theme_name  $config_family_name $config_os_name $config_proj_name

config_theme_src_dir=${config_build_dir}/theme/$selected_theme_name

echo "---------------------------------------------------------"
echo "Debug Info:"
echo "Chip: " $config_chip_name
echo "Product: " $config_chip_product
echo "OS: " $config_os_name
echo "Family: " $config_family_name
echo "Theme: " $selected_theme_name
echo "Project: " $config_proj_name
echo "Theme source: " $config_theme_src_dir
echo "Theme destin: " $config_theme_dst_dir
echo "Project Dir: " $config_proj_dir
echo "Build Dir: " $config_build_dir

config_bin_src_dir=${config_build_dir}/system/bin_${config_os_name}
config_bin_dst_dir=${config_build_dir}/output/image/bin_${config_os_name}
mkdir -p ${config_bin_dst_dir}

config_file_h=${config_build_dir}/app/include/app_config.h
config_file_c=${config_build_dir}/app/app_config.c
config_file_protect=${config_build_dir}/projects/${config_family_name}/${config_proj_name}/xconfig
config_theme_lang_dir=${config_build_dir}/output/theme/language
config_theme_lang=${config_theme_lang_dir}/language
config_theme_kb_lang=${config_theme_lang_dir}/kb_language

mkdir -p $config_theme_lang_dir
if [ -e $config_file_h ];then
	rm -f $config_file_h
fi
touch $config_file_h

if [ -e $config_file_c ];then
	rm -f $config_file_c
fi
touch $config_file_c

if [ -e $config_theme_lang ];then
    rm -f $config_theme_lang
fi
touch $config_theme_lang
touch $config_theme_kb_lang

#######################################copy resource#################################################
## logo update
echo "---------------------------------------------------------"
echo -e "\033[032mCopy theme, bin sections and generate source file \033[0m"

echo "Copy logo bin file"
if [ -e ${config_proj_dir}/flash/logo.bin ];then
	cp ${config_proj_dir}/flash/logo.bin ${config_bin_dst_dir}/logo.bin
fi

#####################################################################################################
## for JTAG or JLINK
echo "Copy gdb debug init file if needed"
cp ${config_debug_dir}/${config_arch_product}/jtaginit/.gdbinit ${config_output_dir}/ 2>/dev/null

#####################################################################################################
##copy theme to output
echo "Copy configured theme from template"
rm ${config_theme_dst_dir}/* -rf
mkdir -p ${config_theme_dst_dir}
mkdir -p ${config_theme_lang_dir}
touch ${config_theme_lang}
touch ${config_theme_kb_lang}
cp -R ${config_theme_src_dir}/* ${config_theme_dst_dir}
rm ${config_theme_dst_dir}/special -rf
find ${config_theme_dst_dir} -name .svn |xargs rm -rf
rm ${config_theme_dst_dir}/weblet -rf

#####################################################################################################
##copy font to output
echo "Copy font file"
config_font_dst_dir=${config_build_dir}/output/theme/font
select_font_chinese=`cat .config|grep "=y" | grep "BR2_FONT" |awk -F"=" '{print $1}'|awk -F"_" '{print $NF}'`

rm ${config_font_dst_dir}/* -rf
mkdir -p ${config_font_dst_dir}
if [[ $select_font_chinese == "MATRIX" || $select_font_chinese == "VECTOR" ]];then
    if [[ $select_font_chinese == "VECTOR" ]];then
        config_font_src_dir=${config_build_dir}/theme/$selected_theme_name/font/chinese_vector
    else
        config_font_src_dir=${config_build_dir}/theme/$selected_theme_name/font/chinese_matrix
    fi
else
    config_font_src_dir=${config_build_dir}/theme/$selected_theme_name/font/english
    select_font_thailand=`cat .config|grep "BR2_LANG"|grep "=y"|grep "THAI"|grep -v "BR2_LANG_DEFAULT"|awk -F"=" '{print $1}'|awk -F"_" '{print $NF}'`
    if [[ $select_font_thailand == "THAI" ]];then
        config_font_src_dir=${config_build_dir}/theme/$selected_theme_name/font/thailand
    fi
    select_font_armenian=`cat .config|grep "BR2_LANG"|grep "=y"|grep "ARMENIAN"|grep -v "BR2_LANG_DEFAULT"|awk -F"=" '{print $1}'|awk -F"_" '{print $NF}'`
    if [[ $select_font_armenian == "ARMENIAN" ]];then
        config_font_src_dir=${config_build_dir}/theme/$selected_theme_name/font/armenian
    fi
fi
cp -R ${config_font_src_dir}/* ${config_font_dst_dir}
######################################################################################################
#build config file .h and .c
##.h
echo "---------------------------------------------------------"
echo "Generate config header and source files through parsing .config"
echo "#ifndef __APP_CONFIG_H__">$config_file_h
echo "#define __APP_CONFIG_H__">>$config_file_h
echo "">>$config_file_h

## chip
if [[ $BR2_CHIP_GX6602 = y || $BR2_CHIP_GX6601 = y || $BR2_CHIP_GX3201 = y ]];then
	echo "#define GX3201">>$config_file_h
fi

if [[ $BR2_CHIP_GX3200 = y ]];then
	echo "#define GX3200">>$config_file_h
fi

if [[ $BR2_CHIP_GX3211 = y ]];then
	echo "#define GX3211">>$config_file_h
fi

if [[ $BR2_CHIP_SIRIUS = y ]];then
	echo "#define SIRIUS">>$config_file_h
fi

if [[ $BR2_CHIP_CANOPUS = y ]];then
	echo "#define CANOPUS">>$config_file_h
fi

if [[ $BR2_CHIP_VEGA = y ]];then
	echo "#define VEGA">>$config_file_h
fi

if [[ $BR2_CHIP_TAURUS = y ]];then
	echo "#define TAURUS">>$config_file_h
fi

if [[ $BR2_CHIP_SCORPIO = y ]];then
	echo "#define SCORPIO">>$config_file_h
fi

if [[ $BR2_CHIP_SCORPIOTAURUS = y ]];then
	echo "#define SCORPIO_TAURUS">>$config_file_h
fi

if [[ $BR2_CHIP_GEMINI = y ]];then
	echo "#define GEMINI">>$config_file_h
fi

if [[ $BR2_CHIP_CYGNUS = y ]];then
	echo "#define CYGNUS">>$config_file_h
fi

if [[ $BR2_CHIP_GX6605S = y ]];then
	echo "#define GX6605S">>$config_file_h
fi

echo "Board name: "$BR2_BOARD_NAME
if [[ $BR2_BOARD_NAME = "6703X5"  || $BR2_BOARD_NAME = "Cygnus-X5" ]];then
	echo "#define DYNAMIC_DRIVER_CONTROL">>$config_file_h
fi

echo "">>$config_file_h

# NNE CHIP DETECT
if [[ $BR2_MOD_CHIP_NNE = y || $BR2_MOD_CHIP_NNM = y ]];then
    echo "#define ENCRYPTION_CHIP">>$config_file_h
fi

config_theme_list=`ls -l ${config_build_dir}/theme | awk '/^d/{print $NF}'`
for theme in $config_theme_list
do
    def="#define THEME_`echo $theme | tr '[a-z]' '[A-Z]'`"
    if [[ $BR2_THEME_TEMPLATE_NAME = $theme ]];then
        echo "$def 1">>$config_file_h
    else
        echo "$def 0">>$config_file_h
    fi
done

xres=$(grep -r "width" ${config_theme_src_dir}/widget/widget.xml | cut -f 2 -d '>'|cut -f 1 -d '<')
yres=$(grep -r "height" ${config_theme_src_dir}/widget/widget.xml | cut -f 2 -d '>'|cut -f 1 -d '<')
echo "#define APP_XRES  $xres">>$config_file_h
echo "#define APP_YRES  $yres">>$config_file_h

if [[ $BR2_MOD_MINI_MEM_SUPPORT = y ]];then
    echo "#define MINI_MEM_SUPPORT 1">> $config_file_h
else
    echo "#define MINI_MEM_SUPPORT 0">> $config_file_h
fi

if [[ $BR2_MOD_FM = y ]];then
    echo "#define FM_SUPPORT 1">>$config_file_h
else
    echo "#define FM_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_LNB_SHORT_DETECT = y ]]; then
    echo "#define LNB_SHORT_DETECT_SUPPORT 1">> $config_file_h
else
    echo "#define LNB_SHORT_DETECT_SUPPORT 0">> $config_file_h
fi

if [[ $BR2_MOD_LEARN_RCU_SUPPORT = y ]];then
    echo "#define LEARN_RCU_SUPPORT 1">>$config_file_h
else
    echo "#define LEARN_RCU_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_learn_rcu_key.xml -rf;
    sed -i '/\"wnd_learn_rcu_key\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_FM_FREQ_NAME = y ]];then
    echo "#define FM_FREQ_NAME_SUPPORT 1">>$config_file_h
    if [[ -e ${config_proj_dir}/fm_name_list.xml ]]; then
        cp ${config_proj_dir}/fm_name_list.xml ${config_theme_dst_dir}/fm_name_list.xml -f
    else
        echo -e "\033[031m fm_name_list.xml not exist,please check!!!!! \033[0m"
        exit 1
    fi
else
    echo "#define FM_FREQ_NAME_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_SHUTDOWN_MODE_SUPPORT = y ]];then
    echo "#define SHUTDOWN_MODE_SUPPORT 1">>$config_file_h
else
    echo "#define SHUTDOWN_MODE_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_START_CHANNEL_SUPPORT = y ]];then
    echo "#define START_CHANNEL_SUPPORT 1">>$config_file_h
else
    echo "#define START_CHANNEL_SUPPORT 0">>$config_file_h
fi

echo "#ifndef APP_LOG_LEVEL">>$config_file_h
if [[ $loglevel == "" ]];then
    echo "#define APP_LOG_LEVEL APP_LOG_ERROR">>$config_file_h
else
    echo "#define APP_LOG_LEVEL $loglevel">>$config_file_h
fi
echo "#endif">>$config_file_h

echo "">>$config_file_h

#######################################################################
##xtal Configure
echo "//XTAL Configure">>$config_file_h
if [[ $BR2_CHIP_27M_XTAL = y ]];then
    echo "#define CHIP_27M_XTAL_SUPPORT">>$config_file_h
fi
if [[ $BR2_CHIP_24M_XTAL = y ]];then
    echo "#define CHIP_24M_XTAL_SUPPORT">>$config_file_h
fi
if [[ $BR2_TUNER_24M_XTAL = y ]];then
    echo "#define TUNER_24M_XTAL_SUPPORT">>$config_file_h
fi
if [[ $BR2_TUNER_16M_XTAL = y ]];then
    echo "#define TUNER_16M_XTAL_SUPPORT">>$config_file_h
fi
#######################################################################
## demod modulation
## DVBS
echo "//Demod DVBS">>$config_file_h
if [[ $BR2_DEMOD1_DVB_S2 = y || $BR2_DEMOD1_AUTO_ADAPT_DVB_S2 = y ||
    $BR2_DEMOD2_DVB_S2 = y || $BR2_DEMOD2_AUTO_ADAPT_DVB_S2 = y ||
    $BR2_DEMOD3_DVB_S2 = y || $BR2_DEMOD3_AUTO_ADAPT_DVB_S2 = y ||
    $BR2_DEMOD4_DVB_S2 = y || $BR2_DEMOD4_AUTO_ADAPT_DVB_S2 = y ||
    $BR2_DEMOD1_COMBO_S = y || $BR2_DEMOD1_AUTO_ADAPT_COMBO_S = y ||
    $BR2_DEMOD2_COMBO_S = y || $BR2_DEMOD2_AUTO_ADAPT_COMBO_S = y ||
    $BR2_DEMOD3_COMBO_S = y || $BR2_DEMOD3_AUTO_ADAPT_COMBO_S = y ||
    $BR2_DEMOD4_COMBO_S = y || $BR2_DEMOD4_AUTO_ADAPT_COMBO_S = y ]];then
    echo "#define DEMOD_DVB_S 1">>$config_file_h
    if [[ $BR2_DEMOD1_DVB_S2 = y && $BR2_DEMOD2_DVB_S2 = y ]];then
        echo "#define DOUBLE_S2_SUPPORT 1">>$config_file_h
    else
        echo "#define DOUBLE_S2_SUPPORT 0">>$config_file_h
    fi
else
    echo "#define DEMOD_DVB_S 0">>$config_file_h
    echo "#define DOUBLE_S2_SUPPORT 0">>$config_file_h
fi
echo "">>$config_file_h

## DVBT
echo "//Demod DVBT">>$config_file_h
if [[ $BR2_DEMOD1_DVB_T2 = y || $BR2_DEMOD1_AUTO_ADAPT_DVB_T2 = y ||
    $BR2_DEMOD2_DVB_T2 = y || $BR2_DEMOD2_AUTO_ADAPT_DVB_T2 = y ||
    $BR2_DEMOD3_DVB_T2 = y || $BR2_DEMOD3_AUTO_ADAPT_DVB_T2 = y ||
    $BR2_DEMOD4_DVB_T2 = y || $BR2_DEMOD4_AUTO_ADAPT_DVB_T2 = y ||
    $BR2_DEMOD1_COMBO_T = y || $BR2_DEMOD1_AUTO_ADAPT_COMBO_T = y ||
    $BR2_DEMOD2_COMBO_T = y || $BR2_DEMOD2_AUTO_ADAPT_COMBO_T = y ||
    $BR2_DEMOD3_COMBO_T = y || $BR2_DEMOD3_AUTO_ADAPT_COMBO_T = y ||
    $BR2_DEMOD4_COMBO_T = y || $BR2_DEMOD4_AUTO_ADAPT_COMBO_T = y ]];then
    echo "#define DEMOD_DVB_T 1">>$config_file_h
else
    echo "#define DEMOD_DVB_T 0">>$config_file_h
fi
echo "">>$config_file_h

## DVBC
echo "//Demod DVBC">>$config_file_h
if [[ $BR2_DEMOD1_DVB_C = y || $BR2_DEMOD1_AUTO_ADAPT_DVB_C = y ||
    $BR2_DEMOD2_DVB_C = y || $BR2_DEMOD2_AUTO_ADAPT_DVB_C = y ||
    $BR2_DEMOD3_DVB_C = y || $BR2_DEMOD3_AUTO_ADAPT_DVB_C = y ||
    $BR2_DEMOD4_DVB_C = y || $BR2_DEMOD4_AUTO_ADAPT_DVB_C = y ||
    $BR2_DEMOD1_COMBO_C = y || $BR2_DEMOD1_AUTO_ADAPT_COMBO_C = y ||
    $BR2_DEMOD2_COMBO_C = y || $BR2_DEMOD2_AUTO_ADAPT_COMBO_C = y ||
    $BR2_DEMOD3_COMBO_C = y || $BR2_DEMOD3_AUTO_ADAPT_COMBO_C = y ||
    $BR2_DEMOD4_COMBO_C = y || $BR2_DEMOD4_AUTO_ADAPT_COMBO_C = y ]];then
    echo "#define DEMOD_DVB_C 1">>$config_file_h
else
    echo "#define DEMOD_DVB_C 0">>$config_file_h
fi
echo "">>$config_file_h

## DTMB
echo "//Demod DTMB">>$config_file_h
if [[ $BR2_DEMOD1_DTMB = y || $BR2_DEMOD1_AUTO_ADAPT_DTMB = y ||
    $BR2_DEMOD2_DTMB = y || $BR2_DEMOD2_AUTO_ADAPT_DTMB = y ||
    $BR2_DEMOD3_DTMB = y || $BR2_DEMOD3_AUTO_ADAPT_DTMB = y ||
    $BR2_DEMOD4_DTMB = y || $BR2_DEMOD4_AUTO_ADAPT_DTMB = y ]];then
    echo "#define DEMOD_DTMB 1">>$config_file_h
else
    echo "#define DEMOD_DTMB 0">>$config_file_h
fi
echo "">>$config_file_h

## ISDBT
echo "//Demod ISDBT">>$config_file_h
if [[ $BR2_DEMOD1_ISDBT = y || $BR2_DEMOD1_AUTO_ADAPT_ISDBT = y ||
    $BR2_DEMOD2_ISDBT = y || $BR2_DEMOD2_AUTO_ADAPT_ISDBT = y ||
    $BR2_DEMOD3_ISDBT = y || $BR2_DEMOD3_AUTO_ADAPT_ISDBT = y ||
    $BR2_DEMOD4_ISDBT = y || $BR2_DEMOD4_AUTO_ADAPT_ISDBT = y ]];then
    echo "#define DEMOD_ISDBT 1">>$config_file_h
else
    echo "#define DEMOD_ISDBT 0">>$config_file_h
fi
echo "">>$config_file_h

## COMBO
echo "//Demod COMBO">>$config_file_h
if [[ $BR2_DEMOD1_COMBO_C = y || $BR2_DEMOD1_AUTO_ADAPT_COMBO_C = y ||
    $BR2_DEMOD1_COMBO_S = y || $BR2_DEMOD1_AUTO_ADAPT_COMBO_S = y ||
    $BR2_DEMOD1_COMBO_T = y || $BR2_DEMOD1_AUTO_ADAPT_COMBO_T = y ||
    $BR2_DEMOD2_COMBO_C = y || $BR2_DEMOD2_AUTO_ADAPT_COMBO_C = y ||
    $BR2_DEMOD2_COMBO_S = y || $BR2_DEMOD2_AUTO_ADAPT_COMBO_S = y ||
    $BR2_DEMOD2_COMBO_T = y || $BR2_DEMOD2_AUTO_ADAPT_COMBO_T = y ||
    $BR2_DEMOD3_COMBO_C = y || $BR2_DEMOD3_AUTO_ADAPT_COMBO_C = y ||
    $BR2_DEMOD3_COMBO_S = y || $BR2_DEMOD3_AUTO_ADAPT_COMBO_S = y ||
    $BR2_DEMOD3_COMBO_T = y || $BR2_DEMOD3_AUTO_ADAPT_COMBO_T = y ||
    $BR2_DEMOD4_COMBO_C = y || $BR2_DEMOD4_AUTO_ADAPT_COMBO_C = y ||
    $BR2_DEMOD4_COMBO_S = y || $BR2_DEMOD4_AUTO_ADAPT_COMBO_S = y ||
    $BR2_DEMOD4_COMBO_T = y || $BR2_DEMOD4_AUTO_ADAPT_COMBO_T = y ]];then
    echo "#define DEMOD_DVB_COMBO 1">>$config_file_h
else
    echo "#define DEMOD_DVB_COMBO 0">>$config_file_h
fi
echo "">>$config_file_h

if [[ $BR2_DEMOD1_AUTO_ADAPT_DVB_S2 = y || $BR2_DEMOD2_AUTO_ADAPT_DVB_S2 = y ||
    $BR2_DEMOD3_AUTO_ADAPT_DVB_S2 = y || $BR2_DEMOD4_AUTO_ADAPT_DVB_S2 = y ||
    $BR2_DEMOD1_AUTO_ADAPT_DVB_T2 = y || $BR2_DEMOD2_AUTO_ADAPT_DVB_T2 = y ||
    $BR2_DEMOD3_AUTO_ADAPT_DVB_T2 = y || $BR2_DEMOD4_AUTO_ADAPT_DVB_T2 = y ||
    $BR2_DEMOD1_AUTO_ADAPT_DVB_C = y || $BR2_DEMOD2_AUTO_ADAPT_DVB_C = y ||
    $BR2_DEMOD3_AUTO_ADAPT_DVB_C = y || $BR2_DEMOD4_AUTO_ADAPT_DVB_C = y ||
    $BR2_DEMOD1_AUTO_ADAPT_DTMB = y || $BR2_DEMOD2_AUTO_ADAPT_DTMB = y ||
    $BR2_DEMOD3_AUTO_ADAPT_DTMB = y || $BR2_DEMOD4_AUTO_ADAPT_DTMB = y ||
    $BR2_DEMOD1_AUTO_ADAPT_ISDBT = y || $BR2_DEMOD2_AUTO_ADAPT_ISDBT = y ||
    $BR2_DEMOD3_AUTO_ADAPT_ISDBT = y || $BR2_DEMOD4_AUTO_ADAPT_ISDBT = y ||
    $BR2_DEMOD1_AUTO_ADAPT_COMBO_C = y || $BR2_DEMOD2_AUTO_ADAPT_COMBO_C = y ||
    $BR2_DEMOD3_AUTO_ADAPT_COMBO_C = y || $BR2_DEMOD4_AUTO_ADAPT_COMBO_C = y ||
    $BR2_DEMOD1_AUTO_ADAPT_COMBO_S = y || $BR2_DEMOD2_AUTO_ADAPT_COMBO_S = y ||
    $BR2_DEMOD3_AUTO_ADAPT_COMBO_S = y || $BR2_DEMOD4_AUTO_ADAPT_COMBO_S = y ||
    $BR2_DEMOD1_AUTO_ADAPT_COMBO_T = y || $BR2_DEMOD2_AUTO_ADAPT_COMBO_T = y ||
    $BR2_DEMOD3_AUTO_ADAPT_COMBO_T = y || $BR2_DEMOD4_AUTO_ADAPT_COMBO_T = y ]];then
    echo "#define DEMOD_AUTO_ADAPT_SUPPORT 1">>$config_file_h
else
    echo "#define DEMOD_AUTO_ADAPT_SUPPORT 0">>$config_file_h
fi

#######################################################################

if [[ $BR2_DEMOD_MULTY_DEMOD > 1 && $BR2_DEMOD_NDEMOD_WITH_1TUNER = y ]];then
    echo "#define NDEMOD_WITH_1TUNER 1">>$config_file_h
else
    echo "#define NDEMOD_WITH_1TUNER 0">>$config_file_h
fi
echo "">>$config_file_h

if [[ $BR2_TUNER_TYPE == "ott" ]];then
    echo "#define OTT_SUPPORT 1">>$config_file_h
else
    echo "#define OTT_SUPPORT 0">>$config_file_h
fi
echo "">>$config_file_h



#####################################################################################################
## tuner attach configure

if [[ $BR2_TUNER1_AUTO_ADAPT_DVBS = y || $BR2_TUNER1_AUTO_ADAPT_DVBT = y ||
	$BR2_TUNER1_AUTO_ADAPT_DVBC = y || $BR2_TUNER1_AUTO_ADAPT_DTMB = y ||
	$BR2_TUNER1_AUTO_ADAPT_ISDBT = y || $BR2_TUNER1_AUTO_ADAPT_COMBO = y ||
	$BR2_TUNER2_AUTO_ADAPT_DVBS = y || $BR2_TUNER2_AUTO_ADAPT_DVBT = y ||
	$BR2_TUNER2_AUTO_ADAPT_DVBC = y || $BR2_TUNER2_AUTO_ADAPT_DTMB = y ||
	$BR2_TUNER2_AUTO_ADAPT_ISDBT = y || $BR2_TUNER2_AUTO_ADAPT_COMBO = y ||
	$BR2_TUNER3_AUTO_ADAPT_DVBS = y || $BR2_TUNER3_AUTO_ADAPT_DVBT = y ||
	$BR2_TUNER3_AUTO_ADAPT_DVBC = y || $BR2_TUNER3_AUTO_ADAPT_DTMB = y ||
	$BR2_TUNER3_AUTO_ADAPT_ISDBT = y || $BR2_TUNER3_AUTO_ADAPT_COMBO = y ||
	$BR2_TUNER4_AUTO_ADAPT_DVBS = y || $BR2_TUNER4_AUTO_ADAPT_DVBT = y ||
	$BR2_TUNER4_AUTO_ADAPT_DVBC = y || $BR2_TUNER4_AUTO_ADAPT_DTMB = y ||
	$BR2_TUNER4_AUTO_ADAPT_ISDBT = y || $BR2_TUNER4_AUTO_ADAPT_COMBO = y ]];then
    echo "#define TUNER_AUTO_ADAPT_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_AUTO_ADAPT_SUPPORT 0">>$config_file_h
fi

echo "/***** DVBS/S2 Tuner *****/">>$config_file_h
if [[ $BR2_TUNER1_AV2018 = y || $BR2_TUNER2_AV2018 = y || $BR2_TUNER3_AV2018 = y || $BR2_TUNER4_AV2018 = y ]];then
    echo "#define TUNER_AV2018_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_AV2018_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_RDA5815M = y || $BR2_TUNER2_RDA5815M = y || $BR2_TUNER3_RDA5815M = y || $BR2_TUNER4_RDA5815M = y ]];then
    echo "#define TUNER_RDA5815M_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_RDA5815M_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_AV2020 = y || $BR2_TUNER2_AV2020 = y || $BR2_TUNER3_AV2020 = y || $BR2_TUNER4_AV2020 = y ]];then
    echo "#define TUNER_AV2020_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_AV2020_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_RT720 = y || $BR2_TUNER2_RT720 = y || $BR2_TUNER3_RT720 = y || $BR2_TUNER4_RT720 = y ]];then
    echo "#define TUNER_RT720_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_RT720_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_R848_DVBS = y || $BR2_TUNER2_R848_DVBS = y || $BR2_TUNER3_R848_DVBS = y || $BR2_TUNER4_R848_DVBS = y ]];then
    echo "#define TUNER_R848_DVBS_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_R848_DVBS_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_SHARP7306 = y || $BR2_TUNER2_SHARP7306 = y || $BR2_TUNER3_SHARP7306 = y || $BR2_TUNER4_SHARP7306 = y ]];then
    echo "#define TUNER_SHARP7306_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_SHARP7306_SUPPORT 0">>$config_file_h
fi

echo "#define UART_SUPPORT 0">>$config_file_h

echo "/***** DVBT/T2 Tuner *****/">>$config_file_h
if [[ $BR2_TUNER1_MXL608_DVBT = y || $BR2_TUNER2_MXL608_DVBT = y || $BR2_TUNER3_MXL608_DVBT = y || $BR2_TUNER4_MXL608_DVBT = y ]];then
    echo "#define TUNER_MXL608_DVBT_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_MXL608_DVBT_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_R836_DVBT = y || $BR2_TUNER2_R836_DVBT = y || $BR2_TUNER3_R836_DVBT = y || $BR2_TUNER4_R836_DVBT = y ]];then
    echo "#define TUNER_R836_DVBT_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_R836_DVBT_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_R848_DVBT = y || $BR2_TUNER2_R848_DVBT = y || $BR2_TUNER3_R848_DVBT = y || $BR2_TUNER4_R848_DVBT = y ]];then
    echo "#define TUNER_R848_DVBT_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_R848_DVBT_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_R910_DVBT = y || $BR2_TUNER2_R910_DVBT = y || $BR2_TUNER3_R910_DVBT = y || $BR2_TUNER4_R910_DVBT = y ]];then
    echo "#define TUNER_R910_DVBT_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_R910_DVBT_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_SI2141_DVBT = y || $BR2_TUNER2_SI2141_DVBT = y || $BR2_TUNER3_SI2141_DVBT = y || $BR2_TUNER4_SI2141_DVBT = y ]];then
    echo "#define TUNER_SI2141_DVBT_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_SI2141_DVBT_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_ATBM253_DVBT = y || $BR2_TUNER2_ATBM253_DVBT = y || $BR2_TUNER3_ATBM253_DVBT = y || $BR2_TUNER4_ATBM253_DVBT = y ]];then
    echo "#define TUNER_ATBM253_DVBT_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_ATBM253_DVBT_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_RDA5160_DVBT = y || $BR2_TUNER2_RDA5160_DVBT = y || $BR2_TUNER3_RDA5160_DVBT = y || $BR2_TUNER4_RDA5160_DVBT = y ]];then
    echo "#define TUNER_RDA5160_DVBT_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_RDA5160_DVBT_SUPPORT 0">>$config_file_h
fi

echo "/***** DVBC Tuner *****/">>$config_file_h
if [[ $BR2_TUNER1_MXL608_DVBC = y || $BR2_TUNER2_MXL608_DVBC = y || $BR2_TUNER3_MXL608_DVBC = y || $BR2_TUNER4_MXL608_DVBC = y ]];then
    echo "#define TUNER_MXL608_DVBC_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_MXL608_DVBC_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_TDA18250 = y || $BR2_TUNER2_TDA18250 = y || $BR2_TUNER3_TDA18250 = y || $BR2_TUNER4_TDA18250 = y ]];then
    echo "#define TUNER_TDA18250_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_TDA18250_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_TDA18250A = y || $BR2_TUNER2_TDA18250A = y || $BR2_TUNER3_TDA18250A = y || $BR2_TUNER4_TDA18250A = y ]];then
    echo "#define TUNER_TDA18250A_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_TDA18250A_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_TDA18273_DVBC = y || $BR2_TUNER2_TDA18273_DVBC = y || $BR2_TUNER3_TDA18273_DVBC = y || $BR2_TUNER4_TDA18273_DVBC = y ]];then
    echo "#define TUNER_TDA18273_DVBC_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_TDA18273_DVBC_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_R836_DVBC = y || $BR2_TUNER2_R836_DVBC = y || $BR2_TUNER3_R836_DVBC = y || $BR2_TUNER4_R836_DVBC = y ]];then
    echo "#define TUNER_R836_DVBC_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_R836_DVBC_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_R848_DVBC = y || $BR2_TUNER2_R848_DVBC = y || $BR2_TUNER3_R848_DVBC = y || $BR2_TUNER4_R848_DVBC = y ]];then
    echo "#define TUNER_R848_DVBC_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_R848_DVBC_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_R910_DVBC = y || $BR2_TUNER2_R910_DVBC = y || $BR2_TUNER3_R910_DVBC = y || $BR2_TUNER4_R910_DVBC = y ]];then
    echo "#define TUNER_R910_DVBC_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_R910_DVBC_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_SI2141_DVBC = y || $BR2_TUNER2_SI2141_DVBC = y || $BR2_TUNER3_SI2141_DVBC = y || $BR2_TUNER4_SI2141_DVBC = y ]];then
    echo "#define TUNER_SI2141_DVBC_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_SI2141_DVBC_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_ATBM2040_DVBC = y || $BR2_TUNER2_ATBM2040_DVBC = y || $BR2_TUNER3_ATBM2040_DVBC = y || $BR2_TUNER4_ATBM2040_DVBC = y ]];then
    echo "#define TUNER_ATBM2040_DVBC_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_ATBM2040_DVBC_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_ATBM253_DVBC = y || $BR2_TUNER2_ATBM253_DVBC = y || $BR2_TUNER3_ATBM253_DVBC = y || $BR2_TUNER4_ATBM253_DVBC = y ]];then
    echo "#define TUNER_ATBM253_DVBC_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_ATBM253_DVBC_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_RDA5160_DVBC = y || $BR2_TUNER2_RDA5160_DVBC = y || $BR2_TUNER3_RDA5160_DVBC = y || $BR2_TUNER4_RDA5160_DVBC = y ]];then
    echo "#define TUNER_RDA5160_DVBC_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_RDA5160_DVBC_SUPPORT 0">>$config_file_h
fi

echo "/***** DTMB Tuner *****/">>$config_file_h
if [[ $BR2_TUNER1_MXL608_DTMB = y || $BR2_TUNER2_MXL608_DTMB = y || $BR2_TUNER3_MXL608_DTMB = y || $BR2_TUNER4_MXL608_DTMB = y ]];then
    echo "#define TUNER_MXL608_DTMB_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_MXL608_DTMB_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_TDA18273_DTMB = y || $BR2_TUNER2_TDA18273_DTMB = y || $BR2_TUNER3_TDA18273_DTMB = y || $BR2_TUNER4_TDA18273_DTMB = y ]];then
    echo "#define TUNER_TDA18273_DTMB_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_TDA18273_DTMB_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_R836_DTMB = y || $BR2_TUNER2_R836_DTMB = y || $BR2_TUNER3_R836_DTMB = y || $BR2_TUNER4_R836_DTMB = y ]];then
    echo "#define TUNER_R836_DTMB_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_R836_DTMB_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_R848_DTMB = y || $BR2_TUNER2_R848_DTMB = y || $BR2_TUNER3_R848_DTMB = y || $BR2_TUNER4_R848_DTMB = y ]];then
    echo "#define TUNER_R848_DTMB_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_R848_DTMB_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_SI2141_DTMB = y || $BR2_TUNER2_SI2141_DTMB = y || $BR2_TUNER3_SI2141_DTMB = y || $BR2_TUNER4_SI2141_DTMB = y ]];then
    echo "#define TUNER_SI2141_DTMB_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_SI2141_DTMB_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_ATBM2030_DTMB = y || $BR2_TUNER2_ATBM2030_DTMB = y || $BR2_TUNER3_ATBM2030_DTMB = y || $BR2_TUNER4_ATBM2030_DTMB = y ]];then
    echo "#define TUNER_ATBM2030_DTMB_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_ATBM2030_DTMB_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_ATBM2040_DTMB = y || $BR2_TUNER2_ATBM2040_DTMB = y || $BR2_TUNER3_ATBM2040_DTMB = y || $BR2_TUNER4_ATBM2040_DTMB = y ]];then
    echo "#define TUNER_ATBM2040_DTMB_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_ATBM2040_DTMB_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_ATBM253_DTMB = y || $BR2_TUNER2_ATBM253_DTMB = y || $BR2_TUNER3_ATBM253_DTMB = y || $BR2_TUNER4_ATBM253_DTMB = y ]];then
    echo "#define TUNER_ATBM253_DTMB_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_ATBM253_DTMB_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_RDA5160_DTMB = y || $BR2_TUNER2_RDA5160_DTMB = y || $BR2_TUNER3_RDA5160_DTMB = y || $BR2_TUNER4_RDA5160_DTMB = y ]];then
    echo "#define TUNER_RDA5160_DTMB_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_RDA5160_DTMB_SUPPORT 0">>$config_file_h
fi

echo "/***** ISDBT Tuner *****/">>$config_file_h
if [[ $BR2_TUNER1_MXL608_ISDBT = y || $BR2_TUNER2_MXL608_ISDBT = y || $BR2_TUNER3_MXL608_ISDBT = y || $BR2_TUNER4_MXL608_ISDBT = y ]];then
    echo "#define TUNER_MXL608_ISDBT_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_MXL608_ISDBT_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_MXL683_ISDBT = y || $BR2_TUNER2_MXL683_ISDBT = y ]];then
    echo "#define TUNER_MXL683_ISDBT_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_MXL683_ISDBT_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_TS6011 = y || $BR2_TUNER2_TS6011 = y || $BR2_TUNER3_TS6011 = y || $BR2_TUNER4_TS6011 = y ]];then
    echo "#define TUNER_TS6011_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_TS6011_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_M3031 = y || $BR2_TUNER2_M3031 = y || $BR2_TUNER3_M3031 = y || $BR2_TUNER4_M3031 = y ]];then
    echo "#define TUNER_M3031_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_M3031_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_R858_DVBC_1ST = y || $BR2_TUNER1_COMBO_R858_DVBC_1ST = y ||
    $BR2_TUNER2_R858_DVBC_1ST = y || $BR2_TUNER2_COMBO_R858_DVBC_1ST = y ||
    $BR2_TUNER3_R858_DVBC_1ST = y || $BR2_TUNER3_COMBO_R858_DVBC_1ST = y ||
    $BR2_TUNER4_R858_DVBC_1ST = y || $BR2_TUNER4_COMBO_R858_DVBC_1ST = y ]]; then
    echo "#define TUNER_R858_DVBC_1ST_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_R858_DVBC_1ST_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_TUNER1_R858_DVBC_2ND = y || $BR2_TUNER1_COMBO_R858_DVBC_2ND = y ||
    $BR2_TUNER2_R858_DVBC_2ND = y || $BR2_TUNER2_COMBO_R858_DVBC_2ND = y ||
    $BR2_TUNER3_R858_DVBC_2ND = y || $BR2_TUNER3_COMBO_R858_DVBC_2ND = y ||
    $BR2_TUNER4_R858_DVBC_2ND = y || $BR2_TUNER4_COMBO_R858_DVBC_2ND = y ]]; then
    echo "#define TUNER_R858_DVBC_2ND_SUPPORT 1">>$config_file_h
else
    echo "#define TUNER_R858_DVBC_2ND_SUPPORT 0">>$config_file_h
fi

echo "">>$config_file_h

#####################################################################################################
## parse frontend board config
#####################################################################################################
if [[ $BR2_TUNER_TYPE != "ott" ]];then
    frontend_head_file_dir=${config_build_dir}/app/include
    chmod +x ${config_build_dir}/scripts/parse_frontend_config.py
    ${config_build_dir}/scripts/parse_frontend_config.py ${config_build_dir} ${frontend_head_file_dir}
    if [ $? != 0 ]; then
        echo -e "\033[032mConfig frontend failed!!!\033[0m"
        exit 1
    fi
fi

echo "/***** Secure Module *****/">>$config_file_h

if [[ $BR2_MOD_SECURE_PVR = y ]]; then
    echo "#define SECURE_PVR_SUPPORT 1">>$config_file_h
else
    echo "#define SECURE_PVR_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_SECURE_FULL_MODULE = y ]];then
    echo "#define SECURE_FULL_SUPPORT 1">>$config_file_h
else
    echo "#define SECURE_FULL_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_SECURE_RNG_MODULE = y ]];then
    echo "#define SECURE_RNG_SUPPORT 1">>$config_file_h
else
    echo "#define SECURE_RNG_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_SECURE_FIREWALL_MODULE = y ]];then
    echo "#define SECURE_FIREWALL_SUPPORT 1">>$config_file_h
else
    echo "#define SECURE_FIREWALL_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_SECURE_DGST_MODULE = y ]];then
    echo "#define SECURE_DGST_SUPPORT 1">>$config_file_h
else
    echo "#define SECURE_DGST_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_SECURE_SCI_MODULE = y ]];then
    echo "#define SECURE_SCI_SUPPORT 1">>$config_file_h
else
    echo "#define SECURE_SCI_SUPPORT 0">>$config_file_h

fi

if [[ $BR2_SECURE_TEE_MODULE = y ]];then
    echo "#define SECURE_TEE_SUPPORT 1">>$config_file_h
else
    echo "#define SECURE_TEE_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_SECURE_IRDETO_MODUL = y ]];then
    echo "#define SECURE_IRDETO_SUPPORT 1">>$config_file_h
else
    echo "#define SECURE_IRDETO_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_SECURE_ACPU_MODULE = y ]];then
    echo "#define SECURE_ACPU_SUPPORT 1">>$config_file_h
else
    echo "#define SECURE_ACPU_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_SECURE_SCPU_MODULE = y ]];then
    echo "#define SECURE_SCPU_SUPPORT 1">>$config_file_h
else
    echo "#define SECURE_SCPU_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_SECURE_DMA_MODULE = y ]];then
    echo "#define SECURE_DMA_SUPPORT 1">>$config_file_h
else
    echo "#define SECURE_DMA_SUPPORT 0">>$config_file_h
fi

echo "">>$config_file_h

#####################################################################################################
## work path and network configure.

makefile_lib=${config_build_dir}/app/Makefile_w.a
rm -f $makefile_lib

echo "#define WORK_PATH \"/dvb/\"">>$config_file_h

if [[ $BR2_MOD_VOICE_CONTROL = y ]];then
    echo "#define VOICE_CONTROL_SUPPORT 1">>$config_file_h
    mkdir -p ${config_theme_dst_dir}/image/vc
    cp ${config_build_dir}/app/voice_control/menu/image/notify.png -rf ${config_theme_dst_dir}/image/vc/
else
    echo "#define VOICE_CONTROL_SUPPORT 0">>$config_file_h
    rm -rf ${config_theme_dst_dir}/image/vc
fi

if [[ $BR2_MOD_APP_SMART_VOLUME  = y ]];then
	echo "#define SMART_VOLUME_SUPPORT 1">>$config_file_h
else
	echo "#define SMART_VOLUME_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_SMART_VOLUME_TEST  = y ]];then
	echo "#define SMART_VOLUME_TEST_SUPPORT 1">>$config_file_h
else
	echo "#define SMART_VOLUME_TEST_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_ADVANCED_PRINT = y ]];then
    echo "#define ADVANCED_PRINT_SUPPORT 1">>$config_file_h
    if [[ $BR2_PRINT_TO_NUL = y ]];then
        echo "#define PRINT_OUTPUT_DEFAULT 0">>$config_file_h
    elif [[ $BR2_PRINT_TO_TTY = y ]];then
        echo "#define PRINT_OUTPUT_DEFAULT 1">>$config_file_h
    elif [[ $BR2_PRINT_TO_USB = y ]];then
        echo "#define PRINT_OUTPUT_DEFAULT 2">>$config_file_h
    elif [[ $BR2_PRINT_TO_NET = y ]];then
        echo "#define PRINT_OUTPUT_DEFAULT 3">>$config_file_h
    else
        echo "#define PRINT_OUTPUT_DEFAULT 1">>$config_file_h
    fi
else
    echo "#define ADVANCED_PRINT_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_USB_TO_SERIAL = y ]];then
    echo "#define USB_TO_SERIAL_SUPPORT 1">>$config_file_h
else
    echo "#define USB_TO_SERIAL_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_USB_TEST = y ]];then
    echo "#define USB_TEST_SUPPORT 1">>$config_file_h
else
    echo "#define USB_TEST_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_usb_test.xml -rf;
    sed -i '/\"wnd_usb_test\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

#### add sattv ftaepg
if [ -e ${config_theme_dst_dir}/widget/richepg ]; then
    rm -rf ${config_theme_dst_dir}/widget/richepg
fi
if [ -e ${config_theme_dst_dir}/image/richepg ]; then
    rm -rf ${config_theme_dst_dir}/image/richepg
fi
if [ -e ${config_theme_dst_dir}/richepg_ts.json ]; then
    rm -rf ${config_theme_dst_dir}/richepg_ts.json
fi
if [[ $BR2_MOD_APP_RICHEPG = y ]];then
    echo "#define RICHEPG_SUPPORT 1">>$config_file_h

    if [[ "$selected_theme_name" == "classic_dark" ]]; then
       richepg_theme_name="classic_dark"
    elif [[ "$selected_theme_name" == "classic_blue" ]]; then
       richepg_theme_name="classic_blue"
    elif [[ "$selected_theme_name" == "grid_purple" ]]; then
       richepg_theme_name="grid_purple"
    else
       richepg_theme_name="classic_dark"
    fi
    echo "richepg themen name = $richepg_theme_name"

    mkdir -p ${config_theme_dst_dir}/widget/richepg
    cp ${config_build_dir}/app/richepg/mini_theme/$richepg_theme_name/*.xml ${config_theme_dst_dir}/widget/richepg/
    mkdir -p ${config_theme_dst_dir}/image/richepg

    cp ${config_build_dir}/app/richepg/mini_theme/$richepg_theme_name/image/* ${config_theme_dst_dir}/image/richepg/
    cp ${config_build_dir}/app/richepg/mini_theme/$richepg_theme_name/other_image/s_ts_fav.bmp ${config_theme_dst_dir}/image/tip/
    if [[ -e ${config_build_dir}/app/richepg/mini_theme/$richepg_theme_name/other_image/s_win_l_t2.bmp ]]; then
        cp ${config_build_dir}/app/richepg/mini_theme/$richepg_theme_name/other_image/s_win_l_t2.bmp ${config_theme_dst_dir}/image/window/
    fi
    if [[ -e ${config_build_dir}/app/richepg/mini_theme/$richepg_theme_name/other_image/s_win_m_t2.bmp ]]; then
        cp ${config_build_dir}/app/richepg/mini_theme/$richepg_theme_name/other_image/s_win_m_t2.bmp ${config_theme_dst_dir}/image/window/
    fi
    if [[ -e ${config_build_dir}/app/richepg/mini_theme/$richepg_theme_name/other_image/s_win_r_t2.bmp ]]; then
        cp ${config_build_dir}/app/richepg/mini_theme/$richepg_theme_name/other_image/s_win_r_t2.bmp ${config_theme_dst_dir}/image/window/
    fi
    cp ${config_build_dir}/app/richepg/config/richepg_ts.json ${config_theme_dst_dir}

    RICHEPG_BUILD_DATE=$(env LC_TIME=en_US.UTF-8 date "+%a %b %d %H:%M:%S %Z %Y")
    echo "#define RICHEPG_SOFEWARE_TIME \"${RICHEPG_BUILD_DATE}\"">>$config_file_h
else
    echo "#define RICHEPG_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_PVR_RECORDING_LIST_STYLE = y ]];then
    echo "#define PVR_RECORDING_LIST_STYLE 1">>$config_file_h
    echo "#define PVR_RECORDING_POSTER_STYLE 0">>$config_file_h
else
    mkdir -p ${config_theme_dst_dir}/widget/cover_pvr
    cp ${config_build_dir}/app/cover_pvr/theme/${selected_theme_name}/*.xml ${config_theme_dst_dir}/widget/cover_pvr/
    mkdir -p ${config_theme_dst_dir}/image/cover_pvr
    cp ${config_build_dir}/app/cover_pvr/theme/${selected_theme_name}/image/* ${config_theme_dst_dir}/image/cover_pvr/
    echo "#define PVR_RECORDING_LIST_STYLE 0">>$config_file_h
    echo "#define PVR_RECORDING_POSTER_STYLE 1">>$config_file_h
fi

if [[ $BR2_MOD_APP_NETWORK  = y ]];then
    echo "#define NETWORK_SUPPORT 1">>$config_file_h
    if [[ $BR2_MOD_APP_ETH = y ]];then
        echo "#define ETH_SUPPORT 1">>$config_file_h
        #echo "LDFLAGS += -lusbeth" >> $makefile_lib
    else
        echo "#define ETH_SUPPORT 0">>$config_file_h
    fi
    if [[ $BR2_MOD_APP_GPRS = y ]];then
        echo "#define GPRS_SUPPORT 1">>$config_file_h
    else
        echo "#define GPRS_SUPPORT 0">>$config_file_h
    fi
else
    echo "#define NETWORK_SUPPORT 0">>$config_file_h
    echo "#define GPRS_SUPPORT 0">>$config_file_h
    echo "#define ETH_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_network.xml -rf;
    sed -i '/\"wnd_network\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_COMMON_VIEW_V2 = y ]];then
    echo "#define COMMON_VIEW_V2_SUPPORT 1">>$config_file_h
    mv ${config_theme_dst_dir}/widget/wnd_common_view_v2.xml ${config_theme_dst_dir}/widget/wnd_common_view.xml;
else
    echo "#define COMMON_VIEW_V2_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_common_view_v2.xml -rf;
fi


if [[ $BR2_MOD_APP_IPTV_ACCOUNT = y ]];then
    echo "#define IPTV_ACCOUNT_SUPPORT">>$config_file_h
else
    rm ${config_theme_dst_dir}/widget/wnd_iptv_account.xml -rf;
    sed -i '/\"wnd_iptv_account\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_IPTV_FAV = y ]];then
    echo "#define IPTV_FAV_SUPPORT">>$config_file_h
fi

if [[ $BR2_MOD_APP_IPTV_EPG = y ]];then
    echo "#define IPTV_EPG_SUPPORT">>$config_file_h
else
    rm ${config_theme_dst_dir}/widget/wnd_iptv_epg.xml -rf;
    sed -i '/\"wnd_iptv_epg\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_IPTV_EPG_PLAYBACK = y ]];then
    echo "#define IPTV_EPG_PLAYBACK_SUPPORT 1">>$config_file_h
else
    echo "#define IPTV_EPG_PLAYBACK_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_iptv_epg_playback.xml -rf;
    sed -i '/\"wnd_iptv_epg_playback\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_IPTV_CLOUD = y ]];then
    echo "#define IPTV_CLOUD_SUPPORT">>$config_file_h
else
    rm ${config_theme_dst_dir}/widget/wnd_iptv_cloud_input.xml -rf;
    sed -i '/\"wnd_iptv_cloud_input\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_SAT2IP_SERVER = y ]] && [[ -f lib/${config_arch_product}-${config_os_name}/libgx_sat2ip.a ]];then
    echo "#define SAT2IP_SERVER_SUPPORT 1">>$config_file_h
else
    echo "#define SAT2IP_SERVER_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_SAT2IP_SERVER_CONFIGURE = y ]];then
    echo "#define SAT2IP_CONFIGURE_SUPPORT 1">>$config_file_h
else
    echo "#define SAT2IP_CONFIGURE_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_DVB2IP_SERVER = y ]] && [[ -f lib/${config_arch_product}-${config_os_name}/libgx_dvb2ip.a ]];then
    echo "#define DVB2IP_SERVER_SUPPORT 1">>$config_file_h
    if [[ $BR2_MOD_DVB2IP_SERVER_SCREEN = y ]];then
        echo "#define DVB2IP_SERVER_USE_STATIC_SCREEN 1">>$config_file_h
    else
        echo "#define DVB2IP_SERVER_USE_STATIC_SCREEN 0">>$config_file_h
        rm ${config_theme_dst_dir}/widget/wnd_dvb2ip_play.xml -rf;
        sed -i '/\"wnd_dvb2ip_play\"/d' ${config_theme_dst_dir}/widget/widget.xml
    fi
else
    echo "#define DVB2IP_SERVER_SUPPORT 0">>$config_file_h
    echo "#define DVB2IP_SERVER_USE_STATIC_SCREEN 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_dvb2ip_play.xml -rf;
    sed -i '/\"wnd_dvb2ip_play\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_WIFI = y ]];then
    echo "#define WIFI_SUPPORT 1">>$config_file_h
    if [[ $config_os_name = "ecos" ]]; then
        if [[ $BR2_MOD_WIFI_MT7601 = y || $BR2_MOD_WIFI_RT5370 = y || $BR2_MOD_WIFI_DOUBLE = y ]];then
            echo "#This makefile is generated by bash script tool. " > $makefile_lib
            echo "#Please do not edit it by yourself." >> $makefile_lib
            echo "#Ref: scripts/config_parse.sh" >> $makefile_lib
            echo "" >> $makefile_lib
        fi
        if [[ $BR2_MOD_WIFI_MT7601 = y ]];then
           # echo "LDFLAGS += -lusbwifi-MT7601" >> $makefile_lib
	        echo "#define WIFI_MT7601_SUPPORT 1">>$config_file_h
        else
	        echo "#define WIFI_MT7601_SUPPORT 0">>$config_file_h
        fi
        if [[ $BR2_MOD_WIFI_MT7603 = y ]];then
	        echo "#define WIFI_MT7603_SUPPORT 1">>$config_file_h
        else
	        echo "#define WIFI_MT7603_SUPPORT 0">>$config_file_h
        fi
        if [[ $BR2_MOD_WIFI_RT5370 = y ]];then
		    # echo "LDFLAGS += -lusbwifi-RT5370" >> $makefile_lib
		    echo "#define WIFI_RT5370_SUPPORT 1">>$config_file_h
        else
		    echo "#define WIFI_RT5370_SUPPORT 0">>$config_file_h
        fi
	    if [[ $BR2_MOD_WIFI_RTL8188FU = y ]];then
            echo "#define WIFI_RTL8188FU_SUPPORT 1">>$config_file_h
        else
            echo "#define WIFI_RTL8188FU_SUPPORT 0">>$config_file_h
        fi
        if [[ $BR2_MOD_WIFI_RTL8188EU = y ]];then
            echo "#define WIFI_RTL8188EU_SUPPORT 1">>$config_file_h
        else
            echo "#define WIFI_RTL8188EU_SUPPORT 0">>$config_file_h
        fi
        if [[ $BR2_MOD_WIFI_ATBM6032 = y ]];then
            echo "#define WIFI_ATBM6032_SUPPORT 1">>$config_file_h
        else
            echo "#define WIFI_ATBM6032_SUPPORT 0">>$config_file_h
        fi
        if [[ $BR2_MOD_WIFI_RTL8192 = y ]];then
            echo "#define WIFI_RTL8192_SUPPORT 1">>$config_file_h
        else
            echo "#define WIFI_RTL8192_SUPPORT 0">>$config_file_h
        fi
        if [[ $BR2_MOD_WIFI_RTL88X1CU = y ]];then
            echo "#define WIFI_RTL88X1CU_SUPPORT 1">>$config_file_h
        else
            echo "#define WIFI_RTL88X1CU_SUPPORT 0">>$config_file_h
        fi
        if [[ $BR2_MOD_WIFI_RTL88X2BU = y ]];then
            echo "#define WIFI_RTL88X2BU_SUPPORT 1">>$config_file_h
        else
            echo "#define WIFI_RTL88X2BU_SUPPORT 0">>$config_file_h
        fi
        if [[ $BR2_MOD_WIFI_RTL8733 = y ]];then
            echo "#define WIFI_RTL8733_SUPPORT 1">>$config_file_h
        else
            echo "#define WIFI_RTL8733_SUPPORT 0">>$config_file_h
        fi

        if [[ $BR2_MOD_WIFI_DOUBLE = y ]];then
            echo "LDFLAGS += -lusbwifi" >> $makefile_lib
	        echo "#define WIFI_DOUBLE_SUPPORT 1">>$config_file_h
	    else
	        echo "#define WIFI_DOUBLE_SUPPORT 0">>$config_file_h
        fi
	    echo "LDFLAGS += -lusbwifi" >> $makefile_lib
        echo "" >> $makefile_lib
    fi

    if [[ $BR2_MOD_WIFI_SUPER_AP = y ]] && [[ -n $BR2_MOD_WIFI_SUPER_AP_ESSID ]];then
        echo "#define WIFI_SUPER_AP_SUPPORT 1">>$config_file_h
        echo "#define WIFI_SUPER_AP_ESSID \"$BR2_MOD_WIFI_SUPER_AP_ESSID\"">>$config_file_h
        echo "#define WIFI_SUPER_AP_PSK \"$BR2_MOD_WIFI_SUPER_AP_PSK\"">>$config_file_h
    else
        echo "#define WIFI_SUPER_AP_SUPPORT 0">>$config_file_h
    fi
else
    echo "#define WIFI_SUPPORT 0">>$config_file_h
    echo "#define WIFI_SUPER_AP_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_wifi.xml -rf;
    sed -i '/\"wnd_wifi\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_RNDIS = y ]];then
    echo "#define RNDIS_SUPPORT 1">>$config_file_h
else
    echo "#define RNDIS_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_3G = y ]];then
    echo "#define MOD_3G_SUPPORT 1">>$config_file_h
    if [[ $config_os_name = "ecos" ]]; then
        echo "LDFLAGS += -lusb3g" >> $makefile_lib
    fi
else
    echo "#define MOD_3G_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_3g.xml -rf;
    rm ${config_theme_dst_dir}/usb3g.txt -rf;
    rm ${config_theme_dst_dir}/image/other/s_icon_3g_in.bmp -rf;
    rm ${config_theme_dst_dir}/image/other/s_icon_3g_out.bmp -rf;
    sed -i '/\"wnd_3g\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_PPPOE = y ]];then
    echo "#define PPPOE_SUPPORT 1">>$config_file_h
else
    echo "#define PPPOE_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_BLUETOOTH = y ]];then
    echo "#define BLUETOOTH_SUPPORT 1">>$config_file_h
    if [[ $config_os_name = "ecos" ]]; then
        if [[ $BR2_MOD_BT_RTL8761A = y ]];then
	    echo "#define BT_RTL8761A_SUPPORT 1">>$config_file_h
        else
	     echo "#define BT_RTL8761A_SUPPORT 0">>$config_file_h
        fi
      	if [[ $BR2_MOD_BT_RTL8761BU = y ]];then
		echo "#define BT_RTL8761BU_SUPPORT 1">>$config_file_h
        else
		echo "#define BT_RTL8761BU_SUPPORT 0">>$config_file_h
	    fi
	if [[ $BR2_MOD_BT_RTL8733BU = y ]];then
		echo "#define BT_RTL8733BU_SUPPORT 1">>$config_file_h
        else
		echo "#define BT_RTL8733BU_SUPPORT 0">>$config_file_h
	fi
        if [[ $BR2_MOD_BT_BCM20702A1 = y ]];then
	    echo "#define BT_BCM20702A1_SUPPORT 1">>$config_file_h
	    else
	    echo "#define BT_BCM20702A1_SUPPORT 0">>$config_file_h
        fi

   fi
else
    echo "#define BLUETOOTH_SUPPORT 0">>$config_file_h
	rm ${config_theme_dst_dir}/widget/wnd_bluetooth.xml -rf;
	rm ${config_theme_dst_dir}/widget/wnd_sink_view.xml -rf;
    rm ${config_theme_dst_dir}/image/other/s_icon_bt_in.bmp -rf;
    rm ${config_theme_dst_dir}/image/other/s_icon_bt_out.bmp -rf;
    sed -i '/\"wnd_bluetooth\"/d' ${config_theme_dst_dir}/widget/widget.xml
	sed -i '/\"wnd_sink_view\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi



if [[ $BR2_MOD_APP_NETAPPS = y ]];then
    echo "#define NETAPPS_SUPPORT 1">>$config_file_h
else
    echo "#define NETAPPS_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_apps_netvideo_play.xml -rf;
    sed -i '/\"wnd_apps_netvideo_play\"/d' ${config_theme_dst_dir}/widget/widget.xml
    rm ${config_theme_dst_dir}/widget/wnd_netvideo_play_number.xml -rf;
    sed -i '/\"wnd_netvideo_play_number\"/d' ${config_theme_dst_dir}/widget/widget.xml
    rm ${config_theme_dst_dir}/widget/wnd_netvideo_subt_setting.xml -rf;
    sed -i '/\"wnd_netvideo_subt_setting\"/d' ${config_theme_dst_dir}/widget/widget.xml
    rm ${config_theme_dst_dir}/widget/wnd_plugin_play_info.xml -rf;
    sed -i '/\"wnd_plugin_play_info\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_MPEG_DASH = y ]];then
    echo "#define MPEG_DASH_SUPPORT">>$config_file_h
fi

if [[ $BR2_MOD_SOCKS5_PROXY_SET = y ]];then
    echo "#define SOCKS5_SUPPORT 1">>$config_file_h
else
    echo "#define SOCKS5_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_COMPOSE_URL = y ]];then
    echo "#define COMPOSE_URL_SUPPORT">>$config_file_h
fi

if [[ $BR2_MOD_ASSOCIATIONLIBRE_URL = y ]];then
    echo "#define ASSOCIATIONLIBRE_URL_SUPPORT">>$config_file_h
fi

if [[ $BR2_MOD_NET_TIME_GET = y ]];then
    echo "#define GET_NET_TIME_SUPPORT 1">>$config_file_h
else
    echo "#define GET_NET_TIME_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_PROG_NET_GET = y ]];then
    echo "#define PROGRAM_NETUP_SUPPORT 1">>$config_file_h
else
    echo "#define PROGRAM_NETUP_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_NET_AUDIO_SUPPORT = y ]];then
    echo "#define NET_AUDIO_SUPPORT 1">>$config_file_h
else
    echo "#define NET_AUDIO_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_netaudio.xml -rf;
    sed -i '/\"wnd_netaudio\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_PLUGINSTORE = y ]] && [[ -f lib/${config_arch_product}-${config_os_name}/libpluginstore.a ]];then
    echo "#define PLUGINSTORE_SUPPORT 1">>$config_file_h
else
    echo "#define PLUGINSTORE_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_pluginstore.xml -rf;
    sed -i '/\"wnd_pluginstore\"/d' ${config_theme_dst_dir}/widget/widget.xml
    rm ${config_theme_dst_dir}/widget/wnd_plugin_listview_play.xml -rf;
    sed -i '/\"wnd_plugin_listview_play\"/d' ${config_theme_dst_dir}/widget/widget.xml
    rm ${config_theme_dst_dir}/widget/wnd_plugin_source_list.xml -rf;
    sed -i '/\"wnd_plugin_source_list\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_PLUGINONLINE = y ]] && [[ -f lib/${config_arch_product}-${config_os_name}/libpluginstore.a ]];then
    echo "#define PLUGINONLINE_SUPPORT 1">>$config_file_h
else
    echo "#define PLUGINONLINE_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_plugin_manage.xml -rf;
    sed -i '/\"wnd_plugin_manage\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_WEBLET = y ]];then
    echo "#define WEBLET_SUPPORT 1">>$config_file_h
    cp -R ${config_weblet_dst_dir} ${config_theme_dst_dir}
else
    echo "#define WEBLET_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_weblet.xml -rf;
    sed -i '/\"wnd_weblet\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_WEBAPI = y ]] && [[ -f lib/${config_arch_product}-${config_os_name}/libgx_webapi.a ]];then
    echo "#define WEBAPI_SUPPORT 1">>$config_file_h
else
    echo "#define WEBAPI_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_webapi_netplay.xml -rf;
    sed -i '/\"wnd_webapi_netplay\"/d' ${config_theme_dst_dir}/widget/widget.xml
    rm ${config_theme_dst_dir}/widget/wnd_supercast.xml -rf;
    sed -i '/\"wnd_supercast\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_NET_UPGRADE = y ]];then
    echo "#define NET_UPGRADE_SUPPORT 1">>$config_file_h
    if [[ -e ${config_proj_dir}/upgrade_info.xml ]]; then
        cp ${config_proj_dir}/upgrade_info.xml ${config_theme_dst_dir}/upgrade_info.xml -f
    else
        echo -e "\033[031m upgrade_info.xml not exist,please check!!!!! \033[0m"
        exit 1
    fi
else
    echo "#define NET_UPGRADE_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_netupg_process.xml -rf;
    sed -i '/\"wnd_netupg_process\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_TRANSMISSION = y ]] && [[ -f lib/${config_arch_product}-${config_os_name}/libgx_transmission.a ]];then
    echo "#define TRANSMISSION_SUPPORT 1">>$config_file_h
else
    echo "#define TRANSMISSION_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_apps_transmission.xml -rf;
    sed -i '/\"wnd_apps_transmission\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi
if [[ $BR2_MOD_APP_NET_SEARCH = y ]];then
    echo "#define NET_SEARCH_SUPPORT 1">>$config_file_h
else
    echo "#define NET_SEARCH_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_FILE_SEARCH = y ]];then
	echo "#define DVB_FILE_SEARCH_SUPPORT 1">>$config_file_h
else
    echo "#define DVB_FILE_SEARCH_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_SERVER_SEARCH = y ]];then
    echo "#define DVB_SERVER_SEARCH_SUPPORT 1">>$config_file_h
else
    echo "#define DVB_SERVER_SEARCH_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_SAT_FINDER = y ]];then
    echo "#define SAT_FINDER_SUPPORT 1">>$config_file_h
else
    echo "#define SAT_FINDER_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_satfinder.xml -rf;
    sed -i '/\"wnd_satfinder\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_DVB_CLOUD = y ]] && [[ -f lib/${config_arch_product}-${config_os_name}/libgx_dvbonline.a ]];then
    echo "#define DVB_CLOUD_SUPPORT 1">>$config_file_h
else
    echo "#define DVB_CLOUD_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_satellite_online_list.xml -rf;
    sed -i '/\"wnd_satellite_online_list\"/d' ${config_theme_dst_dir}/widget/widget.xml
    rm ${config_theme_dst_dir}/widget/wnd_sat_source_select.xml -rf;
    sed -i '/\"wnd_sat_source_select\"/d' ${config_theme_dst_dir}/widget/widget.xml
    rm ${config_theme_dst_dir}/widget/wnd_tp_source_select.xml -rf;
    sed -i '/\"wnd_tp_source_select\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_IPTV_LITE  = y ]];then
	echo "#define IPTV_LITE_SUPPORT 1">>$config_file_h
else
	echo "#define IPTV_LITE_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_DLNADMR = y ]] && [[ -f lib/${config_arch_product}-${config_os_name}/libgx_dlna.a ]];then
    echo "#define DLNADMR_SUPPORT 1">>$config_file_h
else
    echo "#define DLNADMR_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_dlna_dmr.xml -rf;
    sed -i '/\"wnd_dlna_dmr\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_DLNADMP = y ]] && [[ -f lib/${config_arch_product}-${config_os_name}/libgx_dlna.a ]];then
    echo "#define DLNADMP_SUPPORT 1">>$config_file_h
else
    echo "#define DLNADMP_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_dlna_dmp.xml -rf;
    sed -i '/\"wnd_dlna_dmp\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_DLNASAT2IP = y ]] && [[ -f lib/${config_arch_product}-${config_os_name}/libgx_dlna.a ]];then
    echo "#define DLNASAT2IP_SUPPORT 1">>$config_file_h
else
    echo "#define DLNASAT2IP_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_DLNADMS = y ]] && [[ -f lib/${config_arch_product}-${config_os_name}/libgx_dlna.a ]];then
    echo "#define DLNADMS_SUPPORT 1">>$config_file_h
else
    echo "#define DLNADMS_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_dlna_dms.xml -rf;
    sed -i '/\"wnd_dlna_dms\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_MIRACAST  = y ]];then
	echo "#define MIRACAST_SUPPORT 1">>$config_file_h
else
	echo "#define MIRACAST_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_miracast.xml -rf;
    sed -i '/\"wnd_miracast\"/d' ${config_theme_dst_dir}/widget/widget.xml
    rm ${config_theme_dst_dir}/widget/wnd_miracast_info.xml -rf;
    sed -i '/\"wnd_miracast_info\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

#####################################################################################################
if [[ $BR2_MOD_APP_SAMBA = y ]];then
    echo "#define SAMBA_SUPPORT 1">>$config_file_h
else
    echo "#define SAMBA_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_samba.xml -rf;
    sed -i '/\"wnd_samba\"/d' ${config_theme_dst_dir}/widget/widget.xml
    rm ${config_theme_dst_dir}/widget/wnd_samba_login.xml -rf;
    sed -i '/\"wnd_samba_login\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_APPLETS = y ]];then
    echo "#define APPLETS_SUPPORT 1">>$config_file_h
else
    echo "#define APPLETS_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_applets.xml -rf;
    sed -i '/\"wnd_applets\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi
if [[ $BR2_MOD_APP_APPLETS_AUTO_UPDATE_LIST = y ]];then
    echo "#define APPLETS_AUTO_UPDATE_LIST_SUPPORT 1">>$config_file_h
else
    echo "#define APPLETS_AUTO_UPDATE_LIST_SUPPORT 0">>$config_file_h
fi
if [[ $BR2_MOD_APP_APPLETS_BUILTIN = y ]];then
    echo "#define APPLETS_BUILTIN_SUPPORT 1">>$config_file_h
else
    echo "#define APPLETS_BUILTIN_SUPPORT 0">>$config_file_h
fi
if [[ $BR2_MOD_APP_APPLETS_FAV = y ]];then
    echo "#define APPLETS_FAV_SUPPORT 1">>$config_file_h
else
    echo "#define APPLETS_FAV_SUPPORT 0">>$config_file_h
fi
if [[ $BR2_MOD_APP_APPLETS_INSTALL = y ]];then
    echo "#define APPLETS_INSTALL_SUPPORT 1">>$config_file_h
else
    echo "#define APPLETS_INSTALL_SUPPORT 0">>$config_file_h
fi
if [[ $BR2_MOD_APP_APPLETS_RECENT = y ]];then
    echo "#define APPLETS_RECENT_SUPPORT 1">>$config_file_h
else
    echo "#define APPLETS_RECENT_SUPPORT 0">>$config_file_h
fi
if [[ $BR2_MOD_APP_VPN = y ]];then
    echo "#define VPN_SUPPORT 1">>$config_file_h
else
    echo "#define VPN_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_vpn_setting.xml -rf;
    sed -i '/\"wnd_vpn_setting\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

echo "">>$config_file_h
if [[ $BR2_MOD_APP_USB_ETH = y ]];then
    echo "#define USB_ETH_SUPPORT 1">>$config_file_h
else
   echo "#define USB_ETH_SUPPORT 0">>$config_file_h
fi

##set Recall list support
if [[ $BR2_MOD_APP_RECALL_LIST  = y ]];then
	echo "#define RECALL_LIST_SUPPORT 1">>$config_file_h
else
	echo "#define RECALL_LIST_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_MULTIFEED = y ]]; then
	echo "#define MULTIFEED_LIST_SUPPORT 1">>$config_file_h
else
	echo "#define MULTIFEED_LIST_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_AUTO_STANDBY  = y ]];then
	echo "#define AUTO_STANDBY_SUPPORT 1">>$config_file_h
else
	echo "#define AUTO_STANDBY_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_NO_SIGNAL_AUTO_STANDBY  = y ]];then
	echo "#define NO_SIGNAL_AUTO_STANDBY_SUPPORT 1">>$config_file_h
else
	echo "#define NO_SIGNAL_AUTO_STANDBY_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_UNICABLE  = y ]];then
	echo "#define UNICABLE_SUPPORT 1">>$config_file_h
else
	echo "#define UNICABLE_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_unicable_set_popup.xml -rf;
    sed -i '/\"wnd_unicable_set_popup\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_NO_AVPROG_SAVE  = y ]];then
    echo "#define NO_AVPROG_SAVE_SUPPORT 1">>$config_file_h
else
    echo "#define NO_AVPROG_SAVE_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_RESERVED_SERVICE_NOSAVE = y ]];then
    echo "#define RESERVED_SERVICE_NOSAVE_SUPPORT 1">>$config_file_h
else
    echo "#define RESERVED_SERVICE_NOSAVE_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_MULTISTREAM = y ]];then
    echo "#define MULTISTREAM_SUPPORT 1">>$config_file_h
else
    echo "#define MULTISTREAM_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_LNBF = y ]];then
    echo "#define LNBF_SUPPORT 1">>$config_file_h
else
    echo "#define LNBF_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_PDMX = y ]];then
    echo "#define PDMX_SUPPORT 1">>$config_file_h
    echo "LDFLAGS += -lpdmx" >> $makefile_lib
else
    echo "#define PDMX_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_SUPERBLIND = y ]];then
    echo "#define SUPER_BLIND_SUPPORT 1">>$config_file_h
else
    echo "#define SUPER_BLIND_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_FACTORY_TEST  = y ]];then
	echo "#define FACTORY_TEST_SUPPORT 1">>$config_file_h
else
	echo "#define FACTORY_TEST_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_factory_test.xml -rf;
    sed -i '/\"wnd_factory_test\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_CVBS_TEST  = y ]];then
	echo "#define CVBS_TEST_SUPPORT 1">>$config_file_h
else
	echo "#define CVBS_TEST_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_cvbs_test.xml -rf;
    sed -i '/\"wnd_cvbs_test\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

# memory test
if [[ $BR2_MOD_APP_MEMORY_STABILITY_TEST  = y ]];then
	echo "#define MEMORY_STABILITY_TEST_SUPPORT">>$config_file_h
    # DDR info
    if [[ $BR2_MTEST_DDR2_64MB  = y ]];then
	    echo "#define MEMORY_TEST_DDR2_64MB">>$config_file_h
    elif [[ $BR2_MTEST_DDR2_128MB  = y ]];then
	    echo "#define MEMORY_TEST_DDR2_128MB">>$config_file_h
    elif [[ $BR2_MTEST_DDR3_128MB  = y ]];then
	    echo "#define MEMORY_TEST_DDR3_128MB">>$config_file_h
    elif [[ $BR2_MTEST_DDR3_256MB  = y ]];then
	    echo "#define MEMORY_TEST_DDR3_256MB">>$config_file_h
    elif [[ $BR2_MTEST_DDR3_512MB  = y ]];then
	    echo "#define MEMORY_TEST_DDR3_512MB">>$config_file_h
    fi
    # Test mode
    if [[ $BR2_MTEST_REPEAT  = y ]];then
	    echo "#define MEMORY_TEST_REPEAT">>$config_file_h
    fi
    # Test result output control
    if [[ $BR2_MTEST_OSD  = y ]];then
	    echo "#define MEMORY_TEST_OUTPUT_OSD">>$config_file_h
        cp ${config_build_dir}/app/module/mem_test/ui/wnd_mt_message.xml ${config_theme_dst_dir}/widget/
    fi
fi

if [[ $BR2_MOD_APP_SYS_SAVE_DIRECT = y ]];then
    echo "#define SYS_SETTING_SAVE_DIRECT 0">>$config_file_h
else
    echo "#define SYS_SETTING_SAVE_DIRECT 1">>$config_file_h
fi

if [[ $BR2_MOD_APP_KEY_SCANCODE = y ]];then
	echo "#define KEY_SCANCODE_SUPPORT 1">>$config_file_h
else
	echo "#define KEY_SCANCODE_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_key_scancode.xml -rf;
    sed -i '/\"wnd_key_scancode\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_EPG_TMLIST = y ]];then
	echo "#define TMLIST_EPG_SUPPORT 1">>$config_file_h
else
	echo "#define TMLIST_EPG_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_EPG_LVLIST = y ]];then
	echo "#define LVLIST_EPG_SUPPORT 1">>$config_file_h
else
	echo "#define LVLIST_EPG_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_EPG_REALTIMER_FILTER_DETAIL = y ]];then
	echo "#define EPG_REALTIMER_GET_DETAIL_SUPPORT 1">>$config_file_h
else
	echo "#define EPG_REALTIMER_GET_DETAIL_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_USB_SAFELY_REMOVE = y ]];then
	echo "#define USB_SAFELY_REMOVE_SUPPORT 1">>$config_file_h
else
	echo "#define USB_SAFELY_REMOVE_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_QUICK_SWITCH = y ]];then
	echo "#define QUICK_SWITCH_SUPPORT 1">>$config_file_h
else
	echo "#define QUICK_SWITCH_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_UI_MIRROR = y ]];then
	echo "#define UI_MIRROR_SUPPORT 1">>$config_file_h
else
	echo "#define UI_MIRROR_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_VIDEO_SHARPNESS = y ]];then
	echo "#define APP_SHARPNESS_SUPPORT 1">>$config_file_h
else
	echo "#define APP_SHARPNESS_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_VIDEO_HUE = y ]];then
	echo "#define APP_HUE_SUPPORT 1">>$config_file_h
else
	echo "#define APP_HUE_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_VIDEO_DEINTERLACE = y ]];then
	echo "#define APP_DEINTERLACE_SUPPORT 1">>$config_file_h
else
	echo "#define APP_DEINTERLACE_SUPPORT 0">>$config_file_h
fi


if [[ $BR2_MOD_NO_SIGNAL_ALERT_TIP = y ]];then
	echo "#define NO_SIGNAL_ALERT_TIP_SUPPORT 1">>$config_file_h
else
	echo "#define NO_SIGNAL_ALERT_TIP_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_alert_tip.xml -rf;
    sed -i '/\"wnd_alert_tip\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_MERGE_DB = y ]];then
	echo "#define MERGE_DB_SUPPORT 1">>$config_file_h
else
	echo "#define MERGE_DB_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_merge_db.xml -rf;
    sed -i '/\"wnd_merge_db\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_TKGS  = y ]];then
    echo "#define TKGS_SUPPORT 1">>$config_file_h
else
    echo "#define TKGS_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_tkgs_upgrade_process.xml -rf;
    sed -i '/\"wnd_tkgs_upgrade_process\"/d' ${config_theme_dst_dir}/widget/widget.xml
    rm ${config_theme_dst_dir}/widget/wnd_visiblelocation_popup.xml -rf;
    sed -i '/\"wnd_visiblelocation_popup\"/d' ${config_theme_dst_dir}/widget/widget.xml
    rm ${config_theme_dst_dir}/widget/wnd_visible_location.xml -rf;
    sed -i '/\"wnd_visible_location\"/d' ${config_theme_dst_dir}/widget/widget.xml
    rm ${config_theme_dst_dir}/widget/wnd_preferred_list.xml -rf;
    sed -i '/\"wnd_preferred_list\"/d' ${config_theme_dst_dir}/widget/widget.xml
    rm ${config_theme_dst_dir}/widget/wnd_hidden_location.xml -rf;
    sed -i '/\"wnd_hidden_location\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_CVBS_SCALE  = y ]];then
    echo "#define CVBS_SCALE_SUPPORT 1">>$config_file_h
else
    echo "#define CVBS_SCALE_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_MTP = y ]];then
    echo "#define MTP_SUPPORT 1">>$config_file_h
else
    echo "#define MTP_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_PANEL_SETTING_SUPPORT  = y ]];then
	echo "#define PANEL_SETTING_SUPPORT 1">>$config_file_h
else
	echo "#define PANEL_SETTING_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_STANDBY_SHOW_TIME  = y ]];then
	echo "#define STANDBY_SHOW_TIME_SHPPORT 1">>$config_file_h
else
	echo "#define STANDBY_SHOW_TIME_SHPPORT 0">>$config_file_h
fi


if [[ $BR2_MOD_LONG_PASSWORD  = y ]];then
	echo "#define LONG_PASSWORD_SUPPORT 1">>$config_file_h
else
	echo "#define LONG_PASSWORD_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_PSI_MODULE  = y ]];then
	echo "#define PSI_MODULE_SUPPORT 1">>$config_file_h
else
	echo "#define PSI_MODULE_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_BOUQUET  = y ]];then
	echo "#define BOUQUET_SUPPORT 1">>$config_file_h
else
	echo "#define BOUQUET_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_bouquet.xml -rf;
    sed -i '/\"wnd_bouquet\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_BAT_NOTSID_GROUP  = y ]];then
	echo "#define BAT_MISMATCH_TSID_SUPPORT 1">>$config_file_h
else
    echo "#define BAT_MISMATCH_TSID_SUPPORT 0">>$config_file_h
fi


if [[ $BR2_MOD_NIT_SUPPORT  = y ]];then
	echo "#define NIT_SUPPORT 1">>$config_file_h
else
	echo "#define NIT_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_SDT_SUPPORT  = y ]];then
    echo "#define SDT_SUPPORT 1">>$config_file_h
else
    echo "#define SDT_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_PAT_SUPPORT  = y ]];then
    echo "#define PAT_SUPPORT 1">>$config_file_h
else
	echo "#define PAT_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_PMT_UPDATE_SUPPORT  = y ]];then
    echo "#define PMT_UPDATE_SUPPORT 1">>$config_file_h
else
    echo "#define PMT_UPDATE_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_BACK_UPDATE = y ]];then
    echo "#define AUTO_UPDATE_SUPPORT 1">>$config_file_h
else
    echo "#define AUTO_UPDATE_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_ALL_VERSION_SUPPORT = y ]];then
    echo "#define AUTO_UPDATE_ALL_VERSION_SUPPORT 1">>$config_file_h
else
    echo "#define AUTO_UPDATE_ALL_VERSION_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_MISMATCH_TSID = y ]];then
    echo "#define AUTO_UPDATE_MISMATCH_TSID_SUPPORT 1">>$config_file_h
else
    echo "#define AUTO_UPDATE_MISMATCH_TSID_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_PROPMT_AUTO_UPDATE = y ]];then
    echo "#define AUTO_UPDATE_PROPMT_SUPPORT 1">>$config_file_h
else
    echo "#define AUTO_UPDATE_PROPMT_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_LCN_AUTO_UPDATE = y ]];then
    echo "#define LCN_AUTO_UPDATE 1">>$config_file_h
else
    echo "#define LCN_AUTO_UPDATE 0">>$config_file_h
fi

if [[ $BR2_MOD_OTHER_AUTO_UPDATE = y ]];then
    echo "#define OTHER_TS_UPDATE_SUPPORT 1">>$config_file_h
else
    echo "#define OTHER_TS_UPDATE_SUPPORT 0">>$config_file_h
fi


if [[ $BR2_MOD_APP_HDCP_MODE  = y ]];then
	echo "#define HDMI_HDCP_SUPPORT 1">>$config_file_h
else
	echo "#define HDMI_HDCP_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_MEDIA_SUBTITLE  = y ]];then
	echo "#define MEDIA_SUBTITLE_SUPPORT 1">>$config_file_h
    if [[ "$selected_theme_name" == "classic_blue" ]];then
        sed -i 's/MP_ICON_INFO/MP_ICON_SUB/g' ${config_theme_dst_dir}/widget/win_media_bar.xml
        sed -i 's/MP_ICON_INFO_YELLOW/MP_ICON_SUB_YELLOW/g' ${config_theme_dst_dir}/widget/win_media_bar.xml
    else
        sed -i 's/s_mpicon_info_unfocus/s_mpicon_sub_unfocus/g' ${config_theme_dst_dir}/widget/win_media_bar.xml
        sed -i 's/s_mpicon_info_focus/s_mpicon_sub_focus/g' ${config_theme_dst_dir}/widget/win_media_bar.xml
    fi
    if [[ $BR2_MOD_APP_OSD_SUBTITLE = y ]];then
        echo "#define OSD_SUBTITLE_SUPPORT 1">>$config_file_h
        if [[ $BR2_MOD_APP_OSDTRANS_BG = y ]];then
            echo "#define OSD_TRANS_BG_SUPPORT 1">>$config_file_h
        else
            echo "#define OSD_TRANS_BG_SUPPORT 0">>$config_file_h
        fi
    else
        echo "#define OSD_SUBTITLE_SUPPORT 0">>$config_file_h
    fi

    media_sub_delay=`grep '^[[:digit:]]*$' <<< $BR2_MOD_MEDIA_SUB_DELAY`
    if [[ $media_sub_delay ]]; then
      echo "#define SUBTITLE_DELAY_TIME $BR2_MOD_MEDIA_SUB_DELAY" >> $config_file_h
    else
      echo "#define SUBTITLE_DELAY_TIME 0" >> $config_file_h
    fi
else
	echo "#define MEDIA_SUBTITLE_SUPPORT 0">>$config_file_h
    echo "#define OSD_SUBTITLE_SUPPORT 0">>$config_file_h
    if [[ "$selected_theme_name" == "classic_blue" ]];then
        rm -rf ${config_theme_dst_dir}/image/player/play/MP_ICON_SUB.bmp
        rm -rf ${config_theme_dst_dir}/image/player/play/MP_ICON_SUB_YELLOW.bmp
    else
        rm -rf ${config_theme_dst_dir}/image/player/play/s_mpicon_sub_unfocus.bmp
        rm -rf ${config_theme_dst_dir}/image/player/play/s_mpicon_sub_focus.bmp
    fi

fi
echo "">>$config_file_h

#set record tp
if [[ $BR2_MOD_FUNC_RECORD_TP = y ]];then
    echo "#define PVR_RECORD_TP_SUPPORT 1">>$config_file_h
else
    echo "#define PVR_RECORD_TP_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_FUNC_INDEPEND_RECORD = y ]];then
    echo "#define INDEPEND_RECORD 1">>$config_file_h
else
    echo "#define INDEPEND_RECORD 0">>$config_file_h
fi

if [[ $BR2_MOD_NAND_FLASH = y ]];then
	default_flash_conf=${config_proj_dir}/flash/nand/flash.conf
	echo "#define NAND_FLASH_SUPPORT 1">>$config_file_h
    if [[ $BR2_MOD_SPI_NAND = y ]];then
        echo "#define SPI_NAND_SUPPORT 1">>$config_file_h
        echo "#define P_NAND_SUPPORT 0">>$config_file_h
    elif [[ $BR2_MOD_P_NAND = y ]];then
        echo "#define SPI_NAND_SUPPORT 0">>$config_file_h
        echo "#define P_NAND_SUPPORT 1">>$config_file_h
    fi
    if [[ $BR2_MOD_NAND_YAFS = y ]];then
        echo "#define NAND_YAFS_SUPPORT 1">>$config_file_h
        echo "#define NAND_UBIFS_SUPPORT 0">>$config_file_h
    elif [[ $BR2_MOD_NAND_UBIFS = y ]];then
        echo "#define NAND_YAFS_SUPPORT 0">>$config_file_h
        echo "#define NAND_UBIFS_SUPPORT 1">>$config_file_h
    fi

else
	default_flash_conf=${config_proj_dir}/flash/flash.conf
	echo "#define NAND_FLASH_SUPPORT 0">>$config_file_h
    echo "#define SPI_NAND_SUPPORT 0">>$config_file_h
    echo "#define P_NAND_SUPPORT 0">>$config_file_h
    echo "#define NAND_YAFS_SUPPORT 0">>$config_file_h
    echo "#define NAND_UBIFS_SUPPORT 0">>$config_file_h

fi

if [[ $BR2_MOD_MULTIMEDIA_PREVIEW = y ]];then
    mv ${config_theme_dst_dir}/widget/win_file_browser_video.xml ${config_theme_dst_dir}/widget/win_file_browser.xml
    mv ${config_theme_dst_dir}/widget/wnd_pvr_media_list_video.xml ${config_theme_dst_dir}/widget/wnd_pvr_media_list.xml
    echo "#define MULTIMEDIA_PREVIEW_SUPPORT 1">>$config_file_h
else
    rm -rf ${config_theme_dst_dir}/widget/win_file_browser_video.xml
    rm -rf ${config_theme_dst_dir}/widget/wnd_pvr_media_list_video.xml
    rm -rf ${config_theme_dst_dir}/image/player/browse/icon/MP_ICON_ERROR_PLAY.bmp
    rm -rf ${config_theme_dst_dir}/image/player/browse/icon/MP_ICON_FILE_PLAY.bmp
    rm -rf ${config_theme_dst_dir}/image/player/browse/icon/MP_ICON_RADIO_PLAY.bmp
    rm -rf ${config_theme_dst_dir}/image/player/browse/icon/MP_ICON_MUSIC_PLAY.bmp
    rm -rf ${config_theme_dst_dir}/image/player/browse/icon/MP_ICON_MOVIE_PLAY.bmp
    rm -rf ${config_theme_dst_dir}/image/player/browse/icon/MP_ICON_USB_PLAY.bmp
    echo "#define MULTIMEDIA_PREVIEW_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_USB_MICROPHONE = y ]];then
    echo "#define USB_MICROPHONE_SUPPORT 1">>$config_file_h
else
    rm -rf ${config_theme_dst_dir}/widget/wnd_mike_view.xml
    rm -rf ${config_theme_dst_dir}/image/other/s_icon_micphone_in.bmp
    rm -rf ${config_theme_dst_dir}/image/other/s_icon_micphone_out.bmp
    sed -i '/\"wnd_mike_view\"/d' ${config_theme_dst_dir}/widget/widget.xml
    echo "#define USB_MICROPHONE_SUPPORT 0">>$config_file_h
fi


if [[ $BR2_MOD_PLAYBACK_SPEED_SUPPORT = y ]];then
    echo "#define PLAYBACK_SPEED_SUPPORT 1">>$config_file_h
    if [[ "$selected_theme_name" == "classic_blue" ]];then
        sed -i 's/MP_ICON_VOL/MP_ICON_SPEED/g' ${config_theme_dst_dir}/widget/win_media_bar.xml
        sed -i 's/MP_ICON_VOL_YELLOW/MP_ICON_SPEED_YELLOW/g' ${config_theme_dst_dir}/widget/win_media_bar.xml
    else
        sed -i 's/s_mpicon_vol_unfocus/s_mpicon_speed_unfocus/g' ${config_theme_dst_dir}/widget/win_media_bar.xml
        sed -i 's/s_mpicon_vol_focus/s_mpicon_speed_focus/g' ${config_theme_dst_dir}/widget/win_media_bar.xml
    fi
else
    echo "#define PLAYBACK_SPEED_SUPPORT 0">>$config_file_h
    if [[ "$selected_theme_name" == "classic_blue" ]];then
        rm -rf ${config_theme_dst_dir}/image/player/play/MP_ICON_SPEED.bmp
        rm -rf ${config_theme_dst_dir}/image/player/play/MP_ICON_SPEED_YELLOW.bmp
        rm -rf ${config_theme_dst_dir}/image/pvr/s_pvr_speed.bmp
    else
        rm -rf ${config_theme_dst_dir}/image/player/play/s_mpicon_speed_unfocus.bmp
        rm -rf ${config_theme_dst_dir}/image/player/play/s_mpicon_speed_focus.bmp
        rm -rf ${config_theme_dst_dir}/image/pvr/s_pvr_speed.bmp
    fi
fi


if [[ $BR2_MOD_APP_QR_POP = y ]];then
    echo "#define QR_POP_SUPPORT 1">>$config_file_h
else
    echo "#define QR_POP_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_pop_qr.xml -rf;
    sed -i '/\"wnd_pop_qr\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_RF_MODULATOR = y ]];then
	echo "#define RF_MODULATOR_SUPPORT 1">>$config_file_h
else
	echo "#define RF_MODULATOR_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_FUNC_RT500 = y && $BR2_MOD_RF_MODULATOR = y ]];then
	echo "#define RF_RT500 1">>$config_file_h
else
	echo "#define RF_RT500 0">>$config_file_h
fi

if [[ $BR2_MOD_FUNC_RT534 = y && $BR2_MOD_RF_MODULATOR = y ]];then
	echo "#define RF_RT534 1">>$config_file_h
else
	echo "#define RF_RT534 0">>$config_file_h
fi

if [[ $BR2_MENCENT_FREEE_SPACE = y ]];then
    echo "#define MENCENT_FREEE_SPACE 1">>$config_file_h
else
    echo "#define MENCENT_FREEE_SPACE 0">>$config_file_h
fi

if [[ $BR2_MOD_BOOT_AD_SUPPORT = y ]];then
    echo "#define BOOT_AD_SUPPORT 1">>$config_file_h
else
    echo "#define BOOT_AD_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/image/ad -rf
fi

if [[ $BR2_MOD_BOOT_AD_USER_CONTROL_SUPPORT = y ]];then
    echo "#define BOOT_AD_USER_CONTROL_SUPPORT 1">>$config_file_h
else
    echo "#define BOOT_AD_USER_CONTROL_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_AUDIO_DESCRIPTION = y ]];then
    echo "#define AUDIO_DESCRIPTOR_SUPPORT 1">>$config_file_h
    if [[ $BR2_MOD_AD_VOLUME_SET = y ]];then
        echo "#define AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT 1">>$config_file_h
    else
        echo "#define AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT 0">>$config_file_h
    fi
else
    echo "#define AUDIO_DESCRIPTOR_SUPPORT 0">>$config_file_h
    echo "#define AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_HDMI_CEC_MODE = y ]];then
    echo "#define HDMI_CEC_SUPPORT 1">>$config_file_h
else
    echo "#define HDMI_CEC_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_DA = y ]];then
    echo "#define DOLBY_SUPPORT 1">>$config_file_h
else
    echo "#define DOLBY_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_AUDIO_LANG_QAA2OST_SUPPORT = y ]];then
    echo "#define AUDIO_LANG_QAA2OST_SUPPORT 1">>$config_file_h
else
    echo "#define AUDIO_LANG_QAA2OST_SUPPORT 0">>$config_file_h
fi


if [[ $BR2_MOD_APP_SCART = y ]];then
    echo "#define SUPPORT_SCART">>$config_file_h
fi

if [[ $BR2_MOD_APP_HIGH_DYNAMIC_RANGE_CONFIG = y ]];then
    echo "#define HIGH_DYNAMIC_RANGE_SUPPORT 1">>$config_file_h
else
    echo "#define HIGH_DYNAMIC_RANGE_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_PVR_STANDBY = y ]];then
    echo "#define PVR_STANDBY_SUPPORT 1">>$config_file_h
else
    echo "#define PVR_STANDBY_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_PVR_STANDBY_TIPS = y ]];then
    echo "#define PVR_STANDBY_TIP_SUPPORT 1">>$config_file_h
else
    echo "#define PVR_STANDBY_TIP_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_POWERON_CONTROL = y ]];then
    echo "#define POWER_ON_CONTROL_SUPPORT 1">>$config_file_h
else
    echo "#define POWER_ON_CONTROL_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_SEARCH_AUTO_SAVE = y ]];then
    echo "#define SEARCH_AUTO_SAVE_SUPPORT 1">>$config_file_h
else
    echo "#define SEARCH_AUTO_SAVE_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_SW_FILTER_SUPPORT = y ]];then
    echo "#define SW_FILTER_SUPPORT 1">>$config_file_h
else
    echo "#define SW_FILTER_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_DVBC_TUNE_AUTO = y ]];then
    echo "#define DVBC_TUNE_AUTO_SUPPORT 1">>$config_file_h
else
    echo "#define DVBC_TUNE_AUTO_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_LCN = y ]];then
    echo "#define LCN_SUPPORT 1">>$config_file_h
    if [[ $BR2_MOD_APP_LCN_CONFLICT_HANDLE = y ]];then
        echo "#define LCN_CONFLICT_HANDLE_SUPPORT 1">>$config_file_h

        if [[ $BR2_MOD_APP_LCN_CONFLICT_HANDLE_BY_USER = y ]];then
            echo "#define LCN_CONFLICT_HANDLE_BY_USER 1">>$config_file_h
        else
            echo "#define LCN_CONFLICT_HANDLE_BY_USER 0">>$config_file_h
        fi
        if [[ $BR2_MOD_APP_LCN_CONFLICT_HANDLE_BY_VIRTUAL_LCN = y ]]; then
            echo "#define LCN_CONFLICT_HANDLE_BY_VIRTUAL_LCN 1">>$config_file_h
        else
            echo "#define LCN_CONFLICT_HANDLE_BY_VIRTUAL_LCN 0">>$config_file_h
        fi
        if [[ $BR2_MOD_APP_LCN_CONFLICT_HANDLE_BY_THROW_AWAY = y ]]; then
            echo "#define LCN_CONFLICT_HANDLE_BY_THROW_AWAY 1">>$config_file_h
        else
            echo "#define LCN_CONFLICT_HANDLE_BY_THROW_AWAY 0">>$config_file_h
        fi

        # lcn conflict list sorting mode
        if [[ $BR2_MOD_APP_LCN_SORTED_BY_SEARCH_ORDER = y ]]; then
            echo "#define LCN_CONFLIST_LIST_SORTED_BY_SEARCH_ORDER 1">>$config_file_h
        else
            echo "#define LCN_CONFLIST_LIST_SORTED_BY_SEARCH_ORDER 0">>$config_file_h
        fi
        if [[ $BR2_MOD_APP_LCN_SORTED_BY_RFCH_SQ_ORDER = y ]]; then
            echo "#define LCN_CONFLICT_LIST_SORTED_BY_RFCH_SQ_ORDER 1">> $config_file_h
        else
            echo "#define LCN_CONFLICT_LIST_SORTED_BY_RFCH_SQ_ORDER 0">> $config_file_h
        fi
        if [[ $BR2_MOD_APP_LCN_SORTED_BY_FRE_ORDER = y ]]; then
            echo "#define LCN_CONFLICT_LIST_SORTED_BY_FRE_ORDER 1">> $config_file_h
        else
            echo "#define LCN_CONFLICT_LIST_SORTED_BY_FRE_ORDER 0">> $config_file_h
        fi
        if [[ $BR2_MOD_APP_LCN_SORTED_BY_HD_CHANNEL = y ]]; then
            echo "#define LCN_CONFLICT_LIST_SORTED_BY_HD_CHANNEL 1">> $config_file_h
        else
            echo "#define LCN_CONFLICT_LIST_SORTED_BY_HD_CHANNEL 0">> $config_file_h
        fi
        if [[ $BR2_MOD_APP_LCN_VALID_CHECK = y ]]; then
            echo "#define LCN_VALID_CHECK_SUPPORT 1">>$config_file_h
        else
            echo "#define LCN_VALID_CHECK_SUPPORT 0">>$config_file_h
        fi

        if [[ $BR2_MOD_APP_LCN_ONID_CHECK = y ]]; then
            echo "#define LCN_VALID_CHECK_BY_ONID 1">>$config_file_h
        else
            echo "#define LCN_VALID_CHECK_BY_ONID 0">>$config_file_h
        fi

        if [[ $BR2_MOD_APP_LCN_NID_CHECK = y ]]; then
            echo "#define LCN_VALID_CHECK_BY_NID 1">>$config_file_h
        else
            echo "#define LCN_VALID_CHECK_BY_NID 0">>$config_file_h
        fi
    else
        echo "#define LCN_CONFLICT_HANDLE_SUPPORT 0">>$config_file_h
        echo "#define LCN_CONFLICT_HANDLE_BY_USER 0">>$config_file_h
        echo "#define LCN_CONFLICT_HANDLE_BY_VIRTUAL_LCN 0">>$config_file_h
        echo "#define LCN_CONFLICT_HANDLE_BY_THROW_AWAY 0">>$config_file_h
        echo "#define LCN_CONFLIST_LIST_SORTED_BY_SEARCH_ORDER 0">>$config_file_h
        echo "#define LCN_CONFLICT_LIST_SORTED_BY_RFCH_SQ_ORDER 0">> $config_file_h
        echo "#define LCN_CONFLICT_LIST_SORTED_BY_FRE_ORDER 0">> $config_file_h
        echo "#define LCN_VALID_CHECK_SUPPORT 0">>$config_file_h
        echo "#define LCN_VALID_CHECK_BY_ONID 0">>$config_file_h
        echo "#define LCN_VALID_CHECK_BY_NID 0">>$config_file_h
    fi

    if [[ $BR2_MOD_APP_SEARCH_SHOW_LCN = y ]];then
        echo "#define SEARCH_SHOW_LCN_SUPPORT 1">>$config_file_h
    else
        echo "#define SEARCH_SHOW_LCN_SUPPORT 0">>$config_file_h
    fi

else

    echo "#define LCN_SUPPORT 0">>$config_file_h
    echo "#define LCN_CONFLICT_HANDLE_SUPPORT 0">>$config_file_h
    echo "#define SEARCH_SHOW_LCN_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_LCN_VISIBLE_FLAG_SUPPORT = y ]];then
    echo "#define LCN_VISIBLE_FLAG_SUPPORT 1">>$config_file_h
else
    echo "#define LCN_VISIBLE_FLAG_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_PRESET_PROG_INFO_CORRECT = y ]]; then
    echo "#define PRESET_PROG_INFO_CORRECT_SUPPORT 1">>$config_file_h
else
    echo "#define PRESET_PROG_INFO_CORRECT_SUPPORT 0">>$config_file_h
fi

#advance book time
advance_book_time=`grep '^[[:digit:]]*$' <<< $BR2_MOD_ADVANCE_BOOK_TIME`
if [[ $advance_book_time ]]; then
    echo "#define ADVANCE_BOOK_TIME $BR2_MOD_ADVANCE_BOOK_TIME" >> $config_file_h
else
    echo "#define ADVANCE_BOOK_TIME 0" >> $config_file_h
fi

echo "">>$config_file_h
if [[ $BR2_MOD_APP_PP_FULL_HD_SUPPORT = y ]];then
	echo "#define FULL_HD_PP_MODE_ENABLE">>$config_file_h
fi

if [[ $BR2_MOD_APP_PARENTAL_LOCK = y ]];then
	echo "#define PARENTAL_LOCK_SUPPORT 1">>$config_file_h
else
	echo "#define PARENTAL_LOCK_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_GAME = y ]];then
	echo "#define GAME_ENABLE 1">>$config_file_h
else
	echo "#define GAME_ENABLE 0">>$config_file_h
fi

if [[ $BR2_MOD_SAT_LIST_MOVE_SORT = y ]];then
	echo "#define SAT_LIST_MOVE_SORT_SUPPORT 1">>$config_file_h
else
	echo "#define SAT_LIST_MOVE_SORT_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_TP_LIST_MOVE_SORT = y ]];then
	echo "#define TP_LIST_MOVE_SORT_SUPPORT 1">>$config_file_h
else
	echo "#define TP_LIST_MOVE_SORT_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_TTX_PRE_PROCESS = y ]];then
	echo "#define TTX_PRE_PROCESS_ENABLE 1">>$config_file_h
else
	echo "#define TTX_PRE_PROCESS_ENABLE 0">>$config_file_h
fi

if [[ $BR2_MOD_16BIT_COLOR_TTX = y ]];then
	echo "#define COLOR_16BIT_TTX_SUPPORT">>$config_file_h
fi


if [[ $BR2_MOD_APP_AUTO_TEST = y ]];then
	echo "#define AUTO_TEST_ENABLE 1">>$config_file_h
else
	echo "#define AUTO_TEST_ENABLE 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_PCDEMO_TEST = y ]];then
	echo "#define PCDEMO_TEST_ENABLE 1">>$config_file_h
else
	echo "#define PCDEMO_TEST_ENABLE 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_DEMOD_INFO = y ]];then
	echo "#define DEMOD_INFO_SUPPORT 1">>$config_file_h
else
	echo "#define DEMOD_INFO_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_demod_info.xml -rf;
    sed -i '/\"wnd_demod_info\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi

if [[ $BR2_MOD_APP_FLAC = y ]];then
	echo "#define FLAC_SUPPORT 1">>$config_file_h
else
	echo "#define FLAC_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_OGG = y ]];then
	echo "#define OGG_SUPPORT 1">>$config_file_h
else
	echo "#define OGG_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_FACE_DETECT = y ]]; then
    echo "#define FACE_DETECT_SUPPORT 1">>"$config_file_h"
    if [[ $config_os_name = "ecos"  ]]; then
      echo "Copy face detect model file to rootfs_ecos"
      mkdir -p ${config_bin_dst_dir}/rootfs_ecos/models

      if [[ -e ${config_bin_dst_dir}/rootfs_ecos/models/model_detect.ncnn.bin ]]; then
        rm ${config_bin_dst_dir}/rootfs_ecos/models/model_detect.ncnn.bin
      fi

      if [[ -e ${config_bin_dst_dir}/rootfs_ecos/models/model_detect.ncnn.param ]]; then
        rm ${config_bin_dst_dir}/rootfs_ecos/models/model_detect.ncnn.param
      fi

      if [[ -e ${config_build_dir}/app/face_detect/models/model_detect.ncnn.bin ]]; then
        cp ${config_build_dir}/app/face_detect/models/model_detect.ncnn.bin ${config_bin_dst_dir}/rootfs_ecos/models
      fi

      if [[ -e ${config_build_dir}/app/face_detect/models/model_detect.ncnn.param ]]; then
        cp ${config_build_dir}/app/face_detect/models/model_detect.ncnn.param ${config_bin_dst_dir}/rootfs_ecos/models
      fi
    fi
else
    echo "#define FACE_DETECT_SUPPORT 0">>"$config_file_h"
    if [[ -d ${config_bin_dst_dir}/rootfs_ecos/models ]]; then
      rm -rf ${config_bin_dst_dir}/rootfs_ecos/models
    fi
    rm ${config_theme_dst_dir}/image/other/s_icon_face_detect.bmp
fi

if [[ $BR2_MOD_APP_PVR_PREVIEW = y ]]; then
    echo "#define PVR_PREVIEW_SUPPORT 1">>"$config_file_h"
else
    echo "#define PVR_PREVIEW_SUPPORT 0">>"$config_file_h"
fi

#####################################################################################################
##USB Recovery CONFIG BR2_MOD_USB_RECOVER
if [[ $BR2_MOD_USB_RECOVER = y ]];then
    if [[ -f ${config_proj_dir}/flash/recovery/flash.conf ]]; then
        default_flash_conf=${config_proj_dir}/flash/recovery/flash.conf
    else
        echo "${config_proj_dir}/flash/recovery/flash.conf doesn't exist, will use ${default_flash_conf}"
    fi
fi


## loader update
## OTA CONFIG
cp $default_flash_conf ${config_bin_dst_dir}

if [[ $BR2_MOD_APP_OTA_MENU = y ]];then
	echo "#define OTA_SUPPORT 1">>$config_file_h
    echo "LDFLAGS += -ldownloader" >> $makefile_lib
    if [[ $BR2_MOD_APP_NO_OEM_TABLE = y && $BR2_MOD_FUNC_OTA_LOADER != y ]];then
        echo "#define OTA_NO_OEM_PARTiTION 1">>$config_file_h
        if [[ $config_os_name = "ecos" ]]; then
            cp ${config_proj_dir}/flash/ota/variable_oem.ini ${config_theme_dst_dir}/variable_oem.ini
        fi
        if [[ $config_os_name = "linux" ]]; then
           mkdir -p ${config_build_dir}/output/theme
           cp ${config_proj_dir}/flash/ota/variable_oem.ini ${config_build_dir}/output/theme
        fi
    else
        echo "#define OTA_NO_OEM_PARTiTION 0">>$config_file_h
        cp ${config_proj_dir}/flash/ota/flash_menu_ota.conf ${config_bin_dst_dir}/flash.conf
        cp ${config_proj_dir}/flash/ota/variable_oem.ini ${config_bin_dst_dir}
    fi
else
	echo "#define OTA_SUPPORT 0">>$config_file_h
    rm ${config_theme_dst_dir}/widget/wnd_ota_upgrade.xml -rf;
    sed -i '/\"wnd_ota_upgrade\"/d' ${config_theme_dst_dir}/widget/widget.xml

fi

if [[ $BR2_MOD_FUNC_OTA_LOADER = y ]];then
	cp ${config_proj_dir}/flash/ota/flash_loader_ota.conf ${config_bin_dst_dir}/flash.conf
	cp ${config_proj_dir}/flash/ota/ota.img ${config_bin_dst_dir}
	cp ${config_proj_dir}/flash/ota/variable_oem.ini ${config_bin_dst_dir}
	echo "#define UPDATE_SUPPORT_FLAG 1">>$config_file_h
else
	echo "#define UPDATE_SUPPORT_FLAG 0">>$config_file_h
    sed -i 's/OTA/#OTA/' ${config_bin_dst_dir}/flash.conf 2>/dev/null
fi

if [[ $BR2_MOD_APP_MAIN_MENU_OBJ = y ]];then
    echo "#define APP_MAIN_MENU_OBJ_SUPPORT 1">>$config_file_h
else
    echo "#define APP_MAIN_MENU_OBJ_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_APP_SKIN = y ]];then
    echo "#define SKIN_SUPPORT 1">>$config_file_h
    if [[ $BR2_MOD_APP_SKUSB = y ]];then
        echo "#define SKUSB_SUPPORT 1">>$config_file_h
    else
       echo "#define SKUSB_SUPPORT 0">>$config_file_h
    fi
    cp ${config_proj_dir}/flash/skin/flash.conf ${config_bin_dst_dir}/flash.conf
    mkdir -p ${config_theme_dst_dir}/widget/skin
    mkdir -p ${config_theme_dst_dir}/image/skin
    cp  ${config_build_dir}/app/skin/config/skin.xml  ${config_theme_dst_dir}/
    cp  ${config_build_dir}/app/skin/config/image/*  ${config_theme_dst_dir}/image/skin/

    echo "app theme name = $selected_theme_name"

    if [[ "$selected_theme_name" == "classic_dark" ]]; then
       skin_theme_name="classic_dark"
    elif [[ "$selected_theme_name" == "classic_blue" ]]; then
       skin_theme_name="classic_blue"
    elif [[ "$selected_theme_name" == "classic_purple" ]]; then
       skin_theme_name="classic_purple"
       cp ${config_build_dir}/app/skin/theme/$skin_theme_name/image/*  ${config_theme_dst_dir}/image/skin/
    elif [[ "$selected_theme_name" == "sky_blue" ]]; then
       skin_theme_name="sky_blue"
    else
       skin_theme_name="grid_purple"
       cp ${config_build_dir}/app/skin/theme/$skin_theme_name/image/*  ${config_theme_dst_dir}/image/skin/
    fi
	echo "skin themen name = $skin_theme_name"
    cp ${config_build_dir}/app/skin/theme/$skin_theme_name/widget/*  ${config_theme_dst_dir}/widget/skin/
else
    echo "#define SKIN_SUPPORT 0">>$config_file_h
    echo "#define SKUSB_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_OS_ECOS = y ]]; then
    SQUASHFS_TYPE=$(cat ${GXSRC_PATH}/app/bsp.cpp| grep "SQUASHFS_SUPPER" | grep "1" | wc -l)
    if [[ $SQUASHFS_TYPE -eq 1 ]]; then
        sed -i 's/root_cramfs.img/root_fs.img/' ${config_bin_dst_dir}/flash.conf 2>/dev/null
        sed -i 's/CRAMFS/SQUASHFS/' ${config_bin_dst_dir}/flash.conf 2>/dev/null
    else
        sed -i 's/root_fs.img/root_cramfs.img/' ${config_bin_dst_dir}/flash.conf 2>/dev/null
        sed -i 's/SQUASHFS/CRAMFS/' ${config_bin_dst_dir}/flash.conf 2>/dev/null
    fi
fi

if [[ $BR2_MOD_OTA_BACKEND_POLL = y ]];then
	echo "#define OTA_BACKEND_POLL 1">>$config_file_h
else
	echo "#define OTA_BACKEND_POLL 0">>$config_file_h
fi

if [[  $BR2_OS_LINUX = y ]]; then
	echo "#define KERNEL_SUPPORT_FLAG 1">>$config_file_h
else
	echo "#define KERNEL_SUPPORT_FLAG 0">>$config_file_h
fi
#####################################################################################################
## CASCAM_SUPPORT

#####################################################################################################
if [[ -f app/ca/cascam.sh ]]; then
    source app/ca/cascam.sh
fi

#####################################################################################################
## PVR_SPEED_SUPPORT

#####################################################################################################
if [[ $BR2_MOD_ARIBCC = y ]];then
	echo "#define ARIBCC_SUPPORT 1">>$config_file_h
else
	echo "#define ARIBCC_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_MOD_ATSCCC = y ]];then
	echo "#define ATSCCC_SUPPORT 1">>$config_file_h
else
	echo "#define ATSCCC_SUPPORT 0">>$config_file_h
fi

echo "">>$config_file_h

#####################################################################################################
##CHANNEL LIST TURN OVER MODE
#if [[ $config_family_name = "startron" ]];then
#	echo "#define TURN_OVER_MODE 1">>$config_file_h
#else
echo "#define TURN_OVER_MODE 0">>$config_file_h
#fi;
#######################################################################################################
# OSD Language Setting
if [[ $BR2_FONT_classic_blue_CHINESE_VECTOR  = y || $BR2_FONT_classic_dark_CHINESE_VECTOR = y ||
      $BR2_FONT_classic_purple_CHINESE_VECTOR = y || $BR2_FONT_sky_blue_CHINESE_VECTOR = y ||
      $BR2_FONT_grid_purple_CHINESE_VECTOR = y ]];then
	echo "#define FONT_GB2312_TO_UTF8 1">>$config_file_h
else
	echo "#define FONT_GB2312_TO_UTF8 0">>$config_file_h
fi;

echo "Generate osd and kb language"
source scripts/parse_lang.sh $config_file_h $config_file_c $config_theme_lang $config_theme_kb_lang $config_theme_lang_dir

###############################################################################
# create system info ver
###############################################################################
config_file_version=${config_proj_dir}/config_version
VERSION_FILE=$config_build_dir/projects/$config_family_name/version

cat $config_file_version | grep "VERSION" | awk '{print ($3=="y" ? "#define " $4 " " $5:"#define " $4 )}'>>$config_file_h
GIT_VER_PRE=$(git remote -v | awk '{print $1}' | sed -n '1p' | tr '[:lower:]' '[:upper:]')
GIT_VER_SUFFIX=$(git log --oneline -1 | awk '{print $1}' | tr '[:lower:]' '[:upper:]')
common_s=`cat ${config_theme_dst_dir}/default.xml | grep '<group name="common">' -n | awk -F ':' '{print $1}'`
netapp_ver=`cat ${config_theme_dst_dir}/default.xml | grep '<item display="netapp version"'| awk -F '"' '{print $4}'`

if [[ $common_s > 0 ]]; then
    sed -i '/display="hardware version"/d' ${config_theme_dst_dir}/default.xml
    hw_version=`cat $config_file_h | grep -w "HARDWARE_VER" | awk '{print $3}'`
    hw_xml_line=$(printf "\\\\\t\\\t<item display=\"hardware version\" value=%s/>" $hw_version)
    sed -i "${common_s} a${hw_xml_line}" ${config_theme_dst_dir}/default.xml
fi

if [[ $GIT_VER_PRE = "" ]];then
    echo "Warning : Cannot find git version number. Get from config_version."
    if [[ $common_s > 0 ]]; then
        sed -i '/display="software version"/d' ${config_theme_dst_dir}/default.xml
        sw_version=`cat $config_file_h | grep -w "APP_VER" | awk '{print $3}'`
        sw_xml_line=$(printf "\\\\\t\\\t<item display=\"software version\" value=%s/>" $sw_version)
        sed -i "${common_s} a${sw_xml_line}" ${config_theme_dst_dir}/default.xml
    fi
else
    GIT_VER_PRE=$(echo $GIT_VER_PRE | sed  -e 's/^[ \t]*//' -e 's/[ \t]*$//')
    GIT_VER_SUFFIX=$(echo $GIT_VER_SUFFIX | sed  -e 's/^[ \t]*//' -e 's/[ \t]*$//')
    sed -i '/APP_VER/d' $config_file_h
    echo "#define APP_VER \"${GIT_VER_PRE}_${GIT_VER_SUFFIX}\"">>$config_file_h

    if [[ $common_s > 0 ]]; then
        sed -i '/display="software version"/d' ${config_theme_dst_dir}/default.xml
        sw_version=`cat $config_file_h | grep -w "APP_VER" | awk '{print $3}'`
        sw_xml_line=$(printf "\\\\\t\\\t<item display=\"software version\" value=%s/>" $sw_version)
        sed -i "${common_s} a${sw_xml_line}" ${config_theme_dst_dir}/default.xml
    fi

fi

if [[ $netapp_ver != "" ]];then
    echo "#define NETAPPS_VER \"${netapp_ver}\"">>$config_file_h
fi

# BUILD_DATE=$(env LC_TIME=en_US.UTF-8 date "+%a %b %d %H:%M:%S %Z %Y") # ftaepg
BUILD_DATE=$(env LC_TIME=en_US.UTF-8 date "+%Y-%m-%d %H:%M:%S")         # public
echo "#define SOFEWARE_TIME \"${BUILD_DATE}\"">>$config_file_h

####################################################################################################
# Protect config according to project
#
function parse_protect_config(){
#cat $config_file_protect
source $config_file_protect

echo "#define CA_SUPPORT 1">>$config_file_h

if [[ $BR2_ASF_DUAL_CHANNEL_DESCRAMBLE_SUPPORT = y ]];then
	echo "#define DUAL_CHANNEL_DESCRAMBLE_SUPPORT 1">>$config_file_h
else
	echo "#define DUAL_CHANNEL_DESCRAMBLE_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_ASF_ECM_SUPPORT = y ]];then
	echo "#define ECM_SUPPORT 1">>$config_file_h
else
	echo "#define ECM_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_ASF_EMM_SUPPORT = y ]];then
	echo "#define EMM_SUPPORT 1">>$config_file_h
else
	echo "#define EMM_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_ASF_SHARE_EX_SHIFT_SUPPORT = y ]];then
	echo "#define EX_SHIFT_SUPPORT 1">>$config_file_h
else
	echo "#define EX_SHIFT_SUPPORT 0">>$config_file_h
fi

if [[ $BR2_ASF_CMM_SUPPORT = y ]];then
	echo "#define CMM_SUPPORT 1">>$config_file_h
else
	echo "#define CMM_SUPPORT 0">>$config_file_h
fi
echo "">>$config_file_h
}

function none_protect_config(){

echo "#define CA_SUPPORT 0">>$config_file_h
echo "#define DUAL_CHANNEL_DESCRAMBLE_SUPPORT 0">>$config_file_h
echo "#define ECM_SUPPORT 0">>$config_file_h
echo "#define EMM_SUPPORT 0">>$config_file_h
echo "#define EX_SHIFT_SUPPORT 0">>$config_file_h
echo "#define CMM_SUPPORT 0">>$config_file_h
echo "">>$config_file_h

}

if [[ $BR2_MOD_FUNC_CAS = y ]];then
    EXTEND_FILE=${config_build_dir}/app/extend/config.sh
	if [ -e $config_file_protect ];then
        if [ -f $EXTEND_FILE ];then
            source $EXTEND_FILE "build"
        fi
		parse_protect_config
	else
		echo -e "\033[031m------------------------------------------------------\033[0m"
		echo -e "\033[031mWARNING : Are you sure you donot need CAS supported?\033[0m"
		echo -e "\033[031mWARNING : If so,run make menuconfig and cancel CA support\033[0m"
		echo -e "\033[031m------------------------------------------------------\033[0m"
        if [ -f $EXTEND_FILE ];then
            source $EXTEND_FILE "clean"
        fi
		none_protect_config
	fi
else
    EXTEND_FILE=${config_build_dir}/app/extend/config.sh
    if [ -e $config_file_protect ];then
        if [ -f $EXTEND_FILE ];then
            source $EXTEND_FILE "clean"
        fi
    fi
	none_protect_config
fi

# DVB CI config
CIM_FILE=${config_build_dir}/app/module/dvb_ci/config_ci.sh
if [[ $BR2_MOD_FUNC_CI = y ]];then
    if [ -f $CIM_FILE ];then
        source $CIM_FILE "create"
    fi
	echo "#define CI_SUPPORT 1">>$config_file_h
else
    #echo -e "\033[031m------------------------------------------------------\033[0m"
	#echo -e "\033[031m Are you sure you donot need CI supported?\033[0m"
	#echo -e "\033[031m------------------------------------------------------\033[0m"
    if [ -f $CIM_FILE ];then
        source $CIM_FILE "clean"
    fi
	echo "#define CI_SUPPORT 0">>$config_file_h
fi

# theme different posistion
if [[ -e ${config_theme_src_dir}/widget_offset.xml ]]; then
    echo "" >> $config_file_h
    for line in `cat ${config_theme_src_dir}/widget_offset.xml | awk -F '>' '{print $2}'`;
    do
        def=$(echo $line | cut -f 2 -d "/"); val=$(echo $line | cut -f 1 -d "<"); echo "#define $def $val" >>$config_file_h;
    done
else
    echo -e "\033[031m widget_offset.xml not find,please check!!!!! \033[0m"
    exit 1
fi

# theme color
if [[ -e ${config_theme_src_dir}/color/color.xml ]]; then
    echo "/*color define*/" >> $config_file_h
    OSD_TRANS=`cat  ${config_theme_src_dir}/color/color.xml | grep -w "osd_trans" | awk -F '#' '{print $2}' | awk -F '<' '{print $1}'`
    GUI_TRANS=`cat  ${config_theme_src_dir}/color/color.xml | grep -w "gui_trans" | awk -F '#' '{print $2}' | awk -F '<' '{print $1}'`
    SUBT_BG=`cat  ${config_theme_src_dir}/color/color.xml | grep -w "subt_background" | awk -F '#' '{print $2}' | awk -F '<' '{print $1}'`
    echo "#define UI_OSD_TRANS (0x$OSD_TRANS)" >> $config_file_h
    echo "#define UI_GUI_TRANS (0x$GUI_TRANS)" >> $config_file_h
    echo "#define UI_SUBT_BG (0x$SUBT_BG)" >> $config_file_h
else
    echo -e "\033[031m color.xml not find, please check!!!!! \033[0m"
    exit 1
fi

# theme widget color deep
if [[ -e ${config_theme_src_dir}/widget/widget.xml ]]; then
    THEME_BPP=`cat  ${config_theme_src_dir}/widget/widget.xml | grep -w "bpp" | tr -cd "[0-9]"`
    echo "#define UI_THEME_BPP ($THEME_BPP)" >> $config_file_h
else
    echo -e "\033[031m widget.xml not find,please check!!!!! \033[0m"
    exit 1
fi

#####################################################################################################

#Update area info
if [[ $BR2_DEMOD1_DVB_T2 = y || $BR2_DEMOD1_AUTO_ADAPT_DVB_T2 = y || $BR2_DEMOD2_DVB_T2 = y || $BR2_DEMOD2_AUTO_ADAPT_DVB_T2 = y ||
    $BR2_DEMOD3_DVB_T2 = y || $BR2_DEMOD3_AUTO_ADAPT_DVB_T2 = y || $BR2_DEMOD4_DVB_T2 = y || $BR2_DEMOD4_AUTO_ADAPT_DVB_T2 = y ||
    $BR2_DEMOD1_COMBO_T = y || $BR2_DEMOD1_AUTO_ADAPT_COMBO_T = y || $BR2_DEMOD2_COMBO_T = y || $BR2_DEMOD2_AUTO_ADAPT_COMBO_T = y ||
    $BR2_DEMOD3_COMBO_T = y || $BR2_DEMOD3_AUTO_ADAPT_COMBO_T = y || $BR2_DEMOD4_COMBO_T = y || $BR2_DEMOD4_AUTO_ADAPT_COMBO_T = y ]];then
    config_output_area=${config_build_dir}/output/area_conf
    area_config_file_h=${config_build_dir}/app/include/app_t2_area_config.h
    area_config_file_c=${config_build_dir}/app/app_t2_area_config.c
    if [ -e scripts/area_config.sh ];then
        source scripts/area_config.sh $area_config_file_h $area_config_file_c $config_output_area

        ## country update to default.xml
        factory_country_s=`cat ${config_theme_dst_dir}/default.xml |grep "country select" -n | awk -F ':' '{print $1}'`
        factory_country_e=$((factory_country_s+1))
        find_country=`cat ${config_theme_dst_dir}/default.xml | sed -n "$factory_country_e p" | grep -c "display" `
        while [ $find_country -ne 0 ]
        do
            factory_country_e=$((factory_country_e+1))
            find_country=`cat ${config_theme_dst_dir}/default.xml | sed -n "$factory_country_e p" | grep -c "display" `
        done

        factory_country_e=$((factory_country_e-1))

        if [[ ( $factory_country_s > 0 ) ]];then
            if [[ ( $((factory_country_e-factory_country_s)) > 0 ) ]];then
                sed -i "$((factory_country_s+1)), ${factory_country_e} d" ${config_theme_dst_dir}/default.xml
            fi
            country_list=`cat $config_output_area | awk -F '"' '{print($2)}'|awk -F ' ' '{for(i=1;i<=NF;i++) {if(i!=NF) printf $i"_";else print $i}}'`
            country_index=0
            for each_country in $country_list; do
                country_str=`echo $each_country|awk -F '_' '{printf "\"";for(i=1;i<=NF;i++) {if(i!=NF) printf $i" ";else printf $i};print"\""}'`
                xml_line=$(printf "\\\\\t\\\t\\t<list value=\"%d\" display=$country_str/>" $country_index )
                sed -i "${factory_country_s} a${xml_line}" ${config_theme_dst_dir}/default.xml
                factory_country_s=$((factory_country_s+1))
                country_index=$((country_index+1))
            done;
        fi
    fi
fi

#####################################################################################################
## netapps update to default.xml
echo "Generate netapps config to defaul.xml"
factory_netapp_s=`cat ${config_theme_dst_dir}/default.xml |grep "name=\"netapp\"" -n | awk -F ':' '{print $1}'`
factory_netapp_e=$((factory_netapp_s+1))
find_netapp=`cat ${config_theme_dst_dir}/default.xml | sed -n "$factory_netapp_e p" | grep -c "item display" `
while [ $find_netapp -ne 0 ]
do
       factory_netapp_e=$((factory_netapp_e+1))
       find_netapp=`cat ${config_theme_dst_dir}/default.xml | sed -n "$factory_netapp_e p" | grep -c "item display" `
done

factory_netapp_e=$((factory_netapp_e-1))

if [[ $BR2_MOD_APP_DLNADMR = y ]];then
    netapp_list[netapp_index++]="DMR"
fi

if [[ $BR2_MOD_APP_DLNADMP = y ]];then
    netapp_list[netapp_index++]="DMP"
fi

if [[ $BR2_MOD_APP_DLNASAT2IP = y ]];then
    netapp_list[netapp_index++]="Sat2IP"
fi
echo "add netapp["${netapp_list[@]}"]"
if [[ ( $factory_netapp_s > 0 ) ]];then
    if [[ $((factory_netapp_e-factory_netapp_s)) > 0 ]];then
        sed -i "$((factory_netapp_s+1)), ${factory_netapp_e} d" ${config_theme_dst_dir}/default.xml
    fi
    for each_netapp in ${netapp_list[@]}; do
        netapp_line=$(printf "\\\\\t\\\t<item display=\"%s\" value=\"1\"/>" $each_netapp)
        sed -i "${factory_netapp_s} a${netapp_line}" ${config_theme_dst_dir}/default.xml
        factory_netapp_s=$((factory_netapp_s+1))
    done;
fi;
echo "done!Please Get value from  app_oem.c"


#####################################################################################################
## generate netapps macro define
#
echo "Generate netapps macro define"
chmod +x ${config_build_dir}/scripts/config_netapps.py
${config_build_dir}/scripts/config_netapps.py ${config_build_dir} ${config_proj_dir} ${selected_theme_name} ${ARCH} ${OS}
if [ $? != 0 ]; then
    exit 1
fi
chmod +x ${config_build_dir}/scripts/parse_netapps_definition.py
${config_build_dir}/scripts/parse_netapps_definition.py ${config_build_dir}
if [ $? != 0 ]; then
    exit 1
fi

#####################################################################################################
## generate app_main_menu_ex.h for Main Menu
#
echo "Generate Main Menu ex Information"
chmod +x ${config_build_dir}/scripts/parse_mainmenu.py
chmod +x ${config_build_dir}/scripts/parse_netappsmenu.py
ex_dir=${config_build_dir}
${config_build_dir}/scripts/parse_netappsmenu.py ${config_proj_dir} ${ex_dir}
if [ $? != 0 ]; then
    exit 1
fi
if [ -e ${config_proj_dir}/mainmenu-config.json_bak ];then
    mainmenu_config=${config_proj_dir}/mainmenu-config.json_bak
else
    mainmenu_config=${config_proj_dir}/mainmenu-config.json
fi
if [ ! -e ${mainmenu_config} ];then
    echo -e "\033[31;1m${config_family_name}/${config_proj_name} do not have mainmenu-config.json\033[0m"
    exit 1
fi
if [ -e ${config_build_dir}/app/app_main_menu_ex.c ];then
    rm ${config_build_dir}/app/app_main_menu_ex.c
fi
${config_build_dir}/scripts/parse_mainmenu.py ${mainmenu_config} ${ex_dir}
if [ $? != 0 ]; then
    exit 1
fi
if [ ! -e ${config_build_dir}/app/app_main_menu_ex.c ];then
    echo "app/app_main_menu_ex.c is not exist"
    exit 1
fi
if [ -e ${config_proj_dir}/mainmenu-config.json_bak ];then
    rm ${config_proj_dir}/mainmenu-config.json_bak
fi
if [ $? != 0 ]; then
    exit 1
fi
#####################################################################################################
## modify default according .config
#
echo "modify default according .config"
chmod +x ${config_build_dir}/scripts/parse_netapps_default.py
${config_build_dir}/scripts/parse_netapps_default.py ${config_build_dir} ${config_theme_dst_dir}/default.xml
if [ $? != 0 ]; then
    exit 1
fi

# file end
echo "">>$config_file_h
echo "#endif">>$config_file_h
#####################################################################################################
## modify theme in output
#
echo "Modify theme style"


CHANGE_PATH_SRC=${config_build_dir}/theme/$selected_theme_name

GAME=$(cat ${config_build_dir}/app/include/app_config.h| grep "GAME_ENABLE" | grep "1" | wc -l)
if [ $GAME -eq 0 ]; then
    rm ${config_theme_dst_dir}/image/game -rf;
    rm ${config_theme_dst_dir}/widget/wnd_tetris.xml -rf;
    sed -i '/\"wnd_tetris\"/d' ${config_theme_dst_dir}/widget/widget.xml
    rm ${config_theme_dst_dir}/widget/wnd_snake.xml -rf;
    sed -i '/\"wnd_snake\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi;

if [[ $BR2_MOD_FM != y ]] && test ! -z "$(find ${config_build_dir}/output/theme/widget -name wnd_fm*)"; then
    rm ${config_theme_dst_dir}/widget/wnd_fm* -rf
    sed -i '/\"wnd_fm_play\"/d' ${config_theme_dst_dir}/widget/widget.xml
    sed -i '/\"wnd_fm_play_info\"/d' ${config_theme_dst_dir}/widget/widget.xml
    sed -i '/\"wnd_fm_play_info_step_set\"/d' ${config_theme_dst_dir}/widget/widget.xml
    sed -i '/\"wnd_fm_search\"/d' ${config_theme_dst_dir}/widget/widget.xml
fi
if [[ -e ${GXSRC_PATH}/app/netapps/openvpn/parse_openvpn_conf.sh ]];then
     echo "parse openvpn config file"
     cd ${GXSRC_PATH}/app/netapps/openvpn/
     /bin/bash parse_openvpn_conf.sh
     cd -
fi

# create image list
echo "Generate image file resource xml"
cd ${config_theme_dst_dir}/image
/bin/bash create_image.sh
cd -

## language update to default.xml
echo "Modify default language according to configure"
factory_lang_s=`cat ${config_theme_dst_dir}/default.xml |grep "osd language" -n | awk -F ':' '{print $1}'`
factory_lang_e=$((factory_lang_s+1))
find_lang=`cat ${config_theme_dst_dir}/default.xml | sed -n "$factory_lang_e p" | grep -c "display" `
while [ $find_lang -ne 0 ]
do
	factory_lang_e=$((factory_lang_e+1))
	find_lang=`cat ${config_theme_dst_dir}/default.xml | sed -n "$factory_lang_e p" | grep -c "display" `
done

factory_lang_e=$((factory_lang_e-1))
if [[ ( $factory_lang_s > 0 ) ]];then
	if [[ ( $factory_lang_e > $factory_lang_s) ]];then
        sed -i "$((factory_lang_s+1)), ${factory_lang_e} d" ${config_theme_dst_dir}/default.xml
    fi
    lang_list=`cat $config_theme_lang | grep "^#" | awk -F '#' '{print $2}'`
    lang_default=`cat $config_theme_lang | grep "^[^#]" `

    lang_index=0
    for each_lang in $lang_list; do
        if [ $lang_default = $each_lang ]; then
            xml_line=$(printf "<item display=\"osd language\" value=\"%d\">" $lang_index)
            sed -i "$factory_lang_s c\ ${xml_line}" ${config_theme_dst_dir}/default.xml
        fi;
        xml_line=$(printf "<list value=\"%d\" display=\"%s\"/>" $lang_index $each_lang)

        sed -i "${factory_lang_s} a${xml_line}" ${config_theme_dst_dir}/default.xml
        factory_lang_s=$((factory_lang_s+1))
        lang_index=$((lang_index+1))
    done;
fi;

factory_lang_s=`cat ${config_theme_dst_dir}/default.xml |grep "kb language" -n | awk -F ':' '{print $1}'`
factory_lang_e=$((factory_lang_s+1))
find_lang=`cat ${config_theme_dst_dir}/default.xml | sed -n "$factory_lang_e p" | grep -c "display" `
while [ $find_lang -ne 0 ]
do
	factory_lang_e=$((factory_lang_e+1))
	find_lang=`cat ${config_theme_dst_dir}/default.xml | sed -n "$factory_lang_e p" | grep -c "display" `
done

factory_lang_e=$((factory_lang_e-1))
if [[ ( $factory_lang_s > 0 ) ]];then
    if [[ ( $factory_lang_e > $factory_lang_s) ]];then
        sed -i "$((factory_lang_s+1)), ${factory_lang_e} d" ${config_theme_dst_dir}/default.xml
    fi
    lang_list=`cat $config_theme_kb_lang | grep "^#" | awk -F '#' '{print $2}'`
    lang_default=`cat $config_theme_kb_lang | grep "^[^#]" `

    lang_index=0
    for each_lang in $lang_list; do
        if [ $lang_default = $each_lang ]; then
            xml_line=$(printf "<item display=\"kb language\" value=\"%d\">" $lang_index)
            sed -i "$factory_lang_s c\ ${xml_line}" ${config_theme_dst_dir}/default.xml
        fi;
        xml_line=$(printf "<list value=\"%d\" display=\"%s\"/>" $lang_index $each_lang)

        sed -i "${factory_lang_s} a${xml_line}" ${config_theme_dst_dir}/default.xml
        factory_lang_s=$((factory_lang_s+1))
        lang_index=$((lang_index+1))
    done;
fi;

if [ -e $config_theme_kb_lang ];then
	rm -f $config_theme_kb_lang
fi
if [ -e $config_theme_lang_dir/keyboard.xml ];then
	rm -f $config_theme_lang_dir/keyboard.xml
fi

fromdos ${config_theme_dst_dir}/default.xml 2>/dev/null
#############################
## clear no use translate item
############################
echo "#include\"app_config.h\"" > temp.c
echo "#include\"app_str_id.h\"" >> temp.c
echo "begin" >> temp.c
grep "STR_ID" $config_build_dir/app/include/app_str_id.h | awk -F ' ' '{print $2}' >> temp.c
${CROSS_COMPILE}gcc -E -I $config_build_dir/app/include/ temp.c -o temp
sed '1,/begin/d' temp >temp_out
useless_id=$(grep "^STR_ID" temp_out)
lang_xml=$(grep "#"  $config_theme_lang_dir/language | cut -f2 -d"#")

function strip_xml()
{
    l=$1
    temp_lang=$(cat $config_theme_dst_dir/language/$l.xml)
    for i in $useless_id
    do
        no_use_word=$(grep -w $i $config_build_dir/app/include/app_str_id.h | sed 's/\(.*\)\(".*"\)/\2/')
        repeat_word=$(grep -w "^$no_use_word$" temp_out)
        if [[ $no_use_word != "" || $no_use_word != " " ]];then
            if [[ -z $repeat_word ]];then

                temp_lang=$(echo "$temp_lang" | grep -v "id=$no_use_word")
            else
                echo -e "\e[43;31;1m !!!!!! Current theme $l.xml :  $repeat_word  repeat,please check \e[0m"
            fi
        fi
    done
    echo "$temp_lang" > $config_theme_dst_dir/language/$l.xml
}
for l in $lang_xml
do
    strip_xml $l &
done
wait
rm temp.c temp temp_out

#####################################################################################################
## db update and keymap  update
echo "Copy default db file from project"
if [[ $config_os_name = "ecos"  ]]; then
    mkdir -p ${config_bin_dst_dir}/rootfs_ecos

    if [[ -e ${config_bin_dst_dir}/rootfs_ecos/default_data.db ]]; then
        rm ${config_bin_dst_dir}/rootfs_ecos/default_data.db
    fi

    if [[ -e ${config_proj_dir}/default_data.db ]]; then
        cp ${config_proj_dir}/default_data.db ${config_bin_dst_dir}/rootfs_ecos/default_data.db
    fi
fi
#update panel and gpio config
echo "Copy panel gpio and keymap config"
if [[ -e ${config_proj_dir}/panel.xml && -e ${config_proj_dir}/gpio.xml && -e ${config_proj_dir}/keymap.xml ]]; then
    cp ${config_proj_dir}/panel.xml ${config_theme_dst_dir}/panel.xml -f
    cp ${config_proj_dir}/gpio.xml ${config_theme_dst_dir}/gpio.xml -f
    cp ${config_proj_dir}/keymap.xml ${config_theme_dst_dir}/keymap.xml -f
else
    echo -e "\033[031m panel.xml gpio.xml or keymap.xml not exist,please check!!!!! \033[0m"
    exit 1
fi

if [[ $BR2_MOD_HDMI_CEC_MODE = y ]];then
    if [[  -e ${config_proj_dir}/cec_keymap.xml ]]; then
        cp ${config_proj_dir}/cec_keymap.xml ${config_theme_dst_dir}/cec_keymap.xml -f
    fi
fi

# For Linux, app nfs debug
if [[ $config_os_name = "linux" ]]; then
	mkdir -p ${config_build_dir}/output/theme
	if [ -e $config_proj_dir/default_data.db ];then
		cp ${config_proj_dir}/default_data.db ${config_build_dir}/output/theme
	fi
fi

#####################################################################################################
## rootfs.bin uImage cfg.bin update for linux
#if [[ $config_os_name = "linux" ]]; then
#    cp ${config_proj_dir}/flash/rootfs.bin  ${config_bin_dst_dir}
#    cp ${config_proj_dir}/flash/uImage      ${config_bin_dst_dir}
#    cp ${config_proj_dir}/flash/cfg.bin     ${config_bin_dst_dir}
#fi
#####################################################################################################
